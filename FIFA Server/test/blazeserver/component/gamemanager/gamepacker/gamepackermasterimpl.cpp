/*! ************************************************************************************************/
/*!
    \file gamepackermasterimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/storage/storagemanager.h"
#include "framework/usersessions/usersession.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/util/random.h"
#include "framework/util/quantize.h"
#include "framework/event/eventmanager.h"
#include "gamemanager/gamepacker/gamepackermasterimpl.h"
#include "gamemanager/tdf/gamepacker_internal_config_server.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/pinutil/pinutil.h"
#include "gamemanager/templateattributes.h"

#pragma push_macro("LOGGER_CATEGORY")
#undef LOGGER_CATEGORY
#define LOGGER_CATEGORY Blaze::BlazeRpcLog::gamepacker
#define LOG_PACKER_SESSION(session) "PackerSession(" << session.getPackerSessionId() << ":" << session.getSubSessionName() << ")"

namespace Blaze
{

namespace GameManager
{
    extern bool bypassNetworkTopologyQosValidation(GameNetworkTopology networkTopology);
}

namespace Metrics
{
    namespace Tag
    {
        extern TagInfo<GameManager::ScenarioName>* scenario_name;
        extern TagInfo<GameManager::SubSessionName>* subsession_name;

        //TagInfo<GamePacker::GamePackerMasterImpl::WorkerInfo::WorkerLayer>* packer_worker_layer = BLAZE_NEW TagInfo<GamePacker::GamePackerMasterImpl::WorkerInfo::WorkerLayer>("packer_worker_layer", Blaze::Metrics::uint32ToString);
        //TagInfo<GamePacker::GamePackerMasterImpl::WorkerInfo::WorkerState>* packer_worker_state = BLAZE_NEW TagInfo<GamePacker::GamePackerMasterImpl::WorkerInfo::WorkerState>("packer_worker_state", [](const GamePacker::GamePackerMasterImpl::WorkerInfo::WorkerState& value, TagValue&) { return GamePacker::GamePackerMasterImpl::WorkerInfo::getWorkerStateString(value); });
        TagInfo<GameManager::PackerScenarioData::TerminationReason>* packer_scenario_result = 
            BLAZE_NEW TagInfo<GameManager::PackerScenarioData::TerminationReason>("packer_scenario_result", 
                [](const GameManager::PackerScenarioData::TerminationReason& value, Metrics::TagValue&) { return GameManager::PackerScenarioData::TerminationReasonToString(value); });

        TagInfo<GameManager::PackerSessionData::SessionResult>* packer_session_result = 
            BLAZE_NEW TagInfo<GameManager::PackerSessionData::SessionResult>("packer_session_result", 
                [](const GameManager::PackerSessionData::SessionResult& value, Metrics::TagValue&) { return GameManager::PackerSessionData::SessionResultToString(value); });

        TagInfo<uint32_t>* packer_scenario_duration_pct = BLAZE_NEW TagInfo<uint32_t>("packer_scenario_duration", Blaze::Metrics::uint32ToString);
        TagInfo<uint32_t>* packer_session_duration_pct = BLAZE_NEW TagInfo<uint32_t>("packer_session_duration", Blaze::Metrics::uint32ToString);
    }
}

namespace GamePacker
{

const char8_t PRIVATE_PACKER_SCENARIO_FIELD[] = "pri.scenario";
const char8_t PRIVATE_PACKER_SESSION_FIELD[] = "pri.session";
const char8_t PRIVATE_PACKER_REQUEST_FIELD[] = "pri.request";
const char8_t PRIVATE_PACKER_REQUEST_FIELD_FMT[] = "pri.request.%" PRId64;
const char8_t PRIVATE_PACKER_SESSION_FIELD_FMT[] = "pri.session.%" PRId64;
const char8_t* VALID_PACKER_FIELD_PREFIXES[] = { PRIVATE_PACKER_SCENARIO_FIELD, PRIVATE_PACKER_SESSION_FIELD, PRIVATE_PACKER_REQUEST_FIELD, nullptr };
static uint32_t PACKER_DURATION_PERCENTILES[] = { 0, 1, 2, 5, 10, 20, 30, 40, 50, 60, 80, 100 }; // geometric distribution used to restrict metrics growth (array must be sorted)

static uint32_t quantizeRatioAsDurationPercentile(double numerator, double denominator)
{
    uint32_t quantizedRatio = 101; // NOTE: default should never be returned, but if it is, we want to know and trace it to this spot
    if (denominator > 0 && numerator >= 0)
    {
        const auto percent = (uint32_t)round((100.0f * numerator) / denominator);
        quantizedRatio = quantizeToNearestValue(percent, PACKER_DURATION_PERCENTILES, EAArrayCount(PACKER_DURATION_PERCENTILES));
    }
    return quantizedRatio;
}

EA_THREAD_LOCAL GamePackerMasterImpl* gGamePackerMaster = nullptr;

// static factory GamePackerMaster
GamePackerMaster* GamePackerMaster::createImpl()
{
    return BLAZE_NEW_NAMED("GamePackerMasterImpl") GamePackerMasterImpl();
}

GamePackerMasterImpl::GamePackerMasterImpl()
    : GamePackerMasterStub()
    , mScenarioTable(GamePackerMaster::COMPONENT_ID, GamePackerMaster::COMPONENT_MEMORY_GROUP, GamePackerMaster::LOGGING_CATEGORY)
    , mDedicatedServerOverrideMap(RedisId(GameManager::GameManagerSlave::COMPONENT_INFO.name, "dedicatedServerOverrideMap"))
    //, mWorkersActive(getMetricsCollection(), "workersActive", Metrics::Tag::packer_worker_layer, Metrics::Tag::packer_worker_state)
    , mScenariosActive(getMetricsCollection(), "scenariosActive", Metrics::Tag::scenario_name)
    , mScenariosStarted(getMetricsCollection(), "scenariosStarted", Metrics::Tag::scenario_name)
    , mScenariosCompleted(getMetricsCollection(), "scenariosCompleted", Metrics::Tag::scenario_name, Metrics::Tag::packer_scenario_result, Metrics::Tag::packer_scenario_duration_pct)
    , mSessionsActive(getMetricsCollection(), "sessionsActive", Metrics::Tag::scenario_name, Metrics::Tag::subsession_name)
    , mSessionsStarted(getMetricsCollection(), "sessionsStarted", Metrics::Tag::scenario_name, Metrics::Tag::subsession_name)
    , mSessionsCompleted(getMetricsCollection(), "sessionsCompleted", Metrics::Tag::scenario_name, Metrics::Tag::subsession_name, Metrics::Tag::packer_session_result, Metrics::Tag::packer_session_duration_pct)
{
    gGamePackerMaster = this;
    EA::TDF::TypeDescriptionSelector<GameManager::PropertyValueMap>::get(); // PACKER_TODO: hacky registration, fix this
    EA::TDF::TypeDescriptionSelector<GameManager::PropertyValueMapList>::get(); // PACKER_TODO: hacky registration, fix this
}

GamePackerMasterImpl::~GamePackerMasterImpl()
{
    gGamePackerMaster = nullptr;
}

bool GamePackerMasterImpl::onValidateConfig(GamePackerConfig& config, const GamePackerConfig* referenceConfigPtr, ::Blaze::ConfigureValidationErrors& validationErrors) const
{
    auto& gameTemplatesMap = config.getCreateGameTemplatesConfig().getCreateGameTemplates();
    for (auto& gameTemplateItr : gameTemplatesMap)
    {
        const auto& packerConfig = gameTemplateItr.second->getPackerConfig();
        if (packerConfig.getPauseDurationAfterAcquireFinalizationLocks() >= packerConfig.getFinalizationDuration())
        {
            StringBuilder sb;
            sb << "pauseDurationAfterAcquireFinalizationLocks(" << packerConfig.getPauseDurationAfterAcquireFinalizationLocks().getMicroSeconds() << ") must be less than finalizationDuration(" << packerConfig.getFinalizationDuration().getMicroSeconds() << ") in GameTemplate(" << gameTemplateItr.first.c_str() << ")";
            validationErrors.getErrorMessages().push_back(sb.c_str());
            ERR_LOG("[GamePackerMasterImpl].onValidateConfig: " << sb.c_str());
            return false;
        }
    }

    return true;
}

bool GamePackerMasterImpl::onConfigure()
{
    return onReconfigure();
}

bool GamePackerMasterImpl::onPrepareForReconfigure(const GamePackerConfig& newConfig)
{
    return true;
}

bool GamePackerMasterImpl::onReconfigure()
{
    return true;
}

bool GamePackerMasterImpl::onResolve()
{
    mScenarioTable.registerFieldPrefixes(VALID_PACKER_FIELD_PREFIXES);
    mScenarioTable.registerRecordHandlers(
        UpsertStorageRecordCb(this, &GamePackerMasterImpl::onImportStorageRecord),
        EraseStorageRecordCb(this, &GamePackerMasterImpl::onExportStorageRecord));

    if (gStorageManager == nullptr)
    {
        FATAL_LOG("[GamePackerMasterImpl] Storage manager is missing.  This shouldn't happen (was it in the components.cfg?)");
        return false;
    }
    gStorageManager->registerStorageTable(mScenarioTable);

    gController->registerDrainStatusCheckHandler(Blaze::GamePacker::GamePackerMaster::COMPONENT_INFO.name, *this);

    mDedicatedServerOverrideMap.registerCollection();
    gUserSessionManager->addSubscriber(*this);
    return true;
}

void GamePackerMasterImpl::onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot)
{
    // Explicitly suppress blocking here, we don't want to accidentally let timers execute while the game is beign unpacked...
    Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;

    ScenarioInfoMap::insert_return_type scenarioInfoRet = mScenarioByIdMap.insert(snapshot.recordId);
    if (scenarioInfoRet.second)
    {
        StorageRecordSnapshot::FieldEntryMap::const_iterator snapFieldItr = getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_PACKER_SCENARIO_FIELD);
        if (snapFieldItr != snapshot.fieldMap.end())
        {
            GameManager::PackerScenarioData& packerScenarioData = static_cast<GameManager::PackerScenarioData&>(*snapFieldItr->second);

            ScenarioInfo& scenarioInfo = scenarioInfoRet.first->second;
            scenarioInfo.mScenarioId = snapshot.recordId;
            scenarioInfo.mScenarioData = &packerScenarioData;

            snapFieldItr = Blaze::getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_PACKER_SESSION_FIELD);
            if (snapFieldItr != snapshot.fieldMap.end())
            {
                size_t privateSessionFieldPrefixLen = strlen(PRIVATE_PACKER_SESSION_FIELD);
                do
                {
                    // build corresponding private field key
                    StorageFieldName privateRequestFieldName(PRIVATE_PACKER_REQUEST_FIELD);
                    privateRequestFieldName.append(snapFieldItr->first.c_str() + privateSessionFieldPrefixLen);
                    StorageRecordSnapshot::FieldEntryMap::const_iterator privateRequestFieldItr = snapshot.fieldMap.find(privateRequestFieldName);
                    if (privateRequestFieldItr != snapshot.fieldMap.end())
                    {
                        GameManager::PackerSessionData& packerSessionData = static_cast<GameManager::PackerSessionData&>(*snapFieldItr->second);
                        GameManager::StartMatchmakingInternalRequest& requestInfo = static_cast<GameManager::StartMatchmakingInternalRequest&>(*privateRequestFieldItr->second);

                        if (scenarioInfo.mOwnerUserSessionInfo == nullptr)
                        {
                            const auto expiryTimeWithGracePeriod = getScenarioExpiryWithGracePeriod(scenarioInfo.mScenarioData->getExpiresTimestamp());
                            scenarioInfo.mOwnerUserSessionInfo = requestInfo.getOwnerUserSessionInfo().clone(); // PACKER_TODO: for now we must copy, because scenario info cannot safely point to a non-heap allocated member of its session object, in the future session should not own this, the scenario should own the ownerUserSession object and the sessions should not even have it!
                            scenarioInfo.mExpiryTimerId = gSelector->scheduleTimerCall(expiryTimeWithGracePeriod,
                                this, &GamePackerMasterImpl::onScenarioExpiredTimeout, scenarioInfo.mScenarioId, "GamePackerMasterImpl::onScenarioExpiredTimeout");

                            scenarioInfo.mUserJoinInfoList.reserve(requestInfo.getUsersInfo().size());
                            for (auto& userJoinInfo : requestInfo.getUsersInfo())
                            {
                                BlazeId blazeId = userJoinInfo->getUser().getUserInfo().getId();
                                if (blazeId != INVALID_BLAZE_ID)
                                    mScenarioIdByBlazeIdMap.insert(blazeId)->second = scenarioInfo.mScenarioId;
                                else
                                    WARN_LOG("[GamePackerMasterImpl].onImportStorageRecord: Expected valid BlazeId for Scenario(" << scenarioInfo.mScenarioId << ")");
                                scenarioInfo.mUserJoinInfoList.push_back(userJoinInfo);
                            }
                        }

                        SessionInfoMap::insert_return_type sessionInfoRet = mSessionByIdMap.insert(requestInfo.getMatchmakingSessionId());
                        if (sessionInfoRet.second)
                        {
                            SessionInfo& sessionInfo = sessionInfoRet.first->second;

                            sessionInfo.mParentScenario = &scenarioInfo;
                            sessionInfo.mRequestInfo = &requestInfo;
                            sessionInfo.mSessionData = &packerSessionData;
                            // PACKER_TODO: format sessionInfo.mSessionInfoDesc as (sessId=12345/scenId=5345)

                            scenarioInfo.mSessionByIdSet.insert(sessionInfo.getPackerSessionId());

                            if (scenarioInfo.mScenarioData->getPackerSessionSequence() == 0)
                            {
                                switch (sessionInfo.mSessionData->getState())
                                {
                                    case GameManager::PackerSessionData::SESSION_DELAYED:
                                    {
                                        auto expiry = sessionInfo.mSessionData->getStartedTimestamp() + sessionInfo.mRequestInfo->getRequest().getSessionData().getStartDelay();
                                        sessionInfo.mDelayedStartTimerId = gSelector->scheduleTimerCall(
                                            expiry, this, &GamePackerMasterImpl::startDelayedSession, sessionInfo.getPackerSessionId(), "GamePackerMasterImpl::startDelayedSession");
                                        break;
                                    }
                                    case GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT:
                                    {
                                        mUnassignedSessionList.push_back(sessionInfo);
                                        signalSessionAssignment(LayerInfo::INVALID_LAYER_ID);
                                        break;
                                    }
                                    case GameManager::PackerSessionData::SESSION_ASSIGNED:
                                    {
                                        const auto workerInstanceId = sessionInfo.mSessionData->getAssignedWorkerInstanceId();
                                        auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
                                        if (workerItr == mWorkerByInstanceId.end())
                                        {
                                            ERR_LOG("[GamePackerMasterImpl].onImportStorageRecord: Worker instance(" << workerInstanceId << ") is not registered");
                                            EA_FAIL(); // PACKER_TODO: Handle this
                                        }

                                        auto& worker = *workerItr->second;
                                        assignWorkerSession(worker, sessionInfo);
                                        break;
                                    }
                                    default:
                                        break;
                                }
                            }
                        }
                    }
                    else
                    {
                        ERR_LOG("[GamePackerMasterImpl].onImportStorageRecord: Missing private player data in game(" << snapshot.recordId << "), storage corrupt.");
                    }

                    if (++snapFieldItr == snapshot.fieldMap.end())
                        break;
                } while (blaze_strncmp(snapFieldItr->first.c_str(), PRIVATE_PACKER_SESSION_FIELD, privateSessionFieldPrefixLen) == 0);
            }
            else
            {
                TRACE_LOG("[GamePackerMasterImpl].onImportStorageRecord: Field prefix=" << PRIVATE_PACKER_SESSION_FIELD << " matches no fields in Scenario(" << snapshot.recordId << ").");
            }
        }
        else
        {
            mScenarioByIdMap.erase(snapshot.recordId);
            ERR_LOG("[GamePackerMasterImpl].onImportStorageRecord: Field prefix=" << PRIVATE_PACKER_SCENARIO_FIELD << " matches no fields in Scenario(" << snapshot.recordId << ").");
        }
    }
    else
    {
        ERR_LOG("[GamePackerMasterImpl].onImportStorageRecord: Unexpected collision inserting Scenario(" << snapshot.recordId << ").");
    }
}

void GamePackerMasterImpl::onExportStorageRecord(StorageRecordId recordId)
{
    ScenarioInfoMap::iterator scenarioIt = mScenarioByIdMap.find(recordId);
    if (scenarioIt == mScenarioByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].onExportStorageRecord: Failed to export Scenario(" << recordId << "), not found.");
        return;
    }

    ScenarioInfo& scenarioInfo = scenarioIt->second;
    const auto cleanupScenarioId = scenarioInfo.mScenarioId;

    for (auto& userJoinInfo : scenarioInfo.mUserJoinInfoList)
    {
        BlazeId blazeId = userJoinInfo->getUser().getUserInfo().getId();
        if (blazeId == INVALID_BLAZE_ID)
        {
            WARN_LOG("[GamePackerMasterImpl].onExportStorageRecord: Expected valid BlazeId for Scenario(" << scenarioInfo.mScenarioId << ")");
            continue;
        }
        // remove all blazeId->scenarioId mapping pairs that reference this scenario
        auto itrPair = mScenarioIdByBlazeIdMap.equal_range(blazeId);
        while (itrPair.first != itrPair.second)
        {
            auto scenarioId = itrPair.first->second;
            if (cleanupScenarioId == scenarioId)
                itrPair.first = mScenarioIdByBlazeIdMap.erase(itrPair.first);
            else
                ++itrPair.first;
        }
    }

    for (auto erasePackerSessionId : scenarioInfo.mSessionByIdSet)
    {
        auto sessionItr = mSessionByIdMap.find(erasePackerSessionId);
        if (sessionItr == mSessionByIdMap.end())
            continue;

        auto& eraseSessionInfo = sessionItr->second;
        eraseSessionInfo.cancelDelayStartTimer();
        SessionInfoList::remove(eraseSessionInfo);

        mSessionByIdMap.erase(sessionItr);
    }

    scenarioInfo.cancelExpiryTimer();
    scenarioInfo.cancelFinalizationTimer();

    mScenarioByIdMap.erase(cleanupScenarioId);
}


void GamePackerMasterImpl::onScenarioExpiredTimeout(GameManager::MatchmakingScenarioId scenarioId)
{
    auto scenarioItr = mScenarioByIdMap.find(scenarioId);
    if (scenarioItr == mScenarioByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].onScenarioExpiredTimeout: Expected Scenario(" << scenarioId << ") not found");
        return;
    }

    auto& scenarioInfo = scenarioItr->second;
    scenarioInfo.mExpiryTimerId = INVALID_TIMER_ID;
    auto& scenarioData = *scenarioInfo.mScenarioData;
    auto lockedByPackerSessionId = scenarioData.getLockedForFinalizationByPackerSessionId();
    if (lockedByPackerSessionId != INVALID_PACKER_SESSION_ID)
    {
        // PACKER_MAYBE: #ARCHITECTURE We may need to significantly delay the scenario expiration timeout in the event the timer fires while locked for finalization because finalization should have a separate timeout that applies above and beyond the scenarios 'active packing phase' timeout. This comes from conversations with Jesse where it was proposed that the specified matchmaking duration does not really include the finalization time (if needed it should be separate but still short).

        TRACE_LOG("[GamePackerMasterImpl].onScenarioExpiredTimeout: Scenario(" << scenarioId << "), scenarioInfo(" << scenarioData << ") expired while locked for finalization, no-op");
        // PACKER_TODO: add metric
        return;
    }

    for (auto packerSessionId : scenarioInfo.mSessionByIdSet)
    {
        auto sessionItr = mSessionByIdMap.find(packerSessionId);
        if (sessionItr == mSessionByIdMap.end())
        {
            continue;
        }

        auto& sessionInfo = sessionItr->second;
        auto assignedInstanceId = sessionInfo.mSessionData->getAssignedWorkerInstanceId();

        INFO_LOG("[GamePackerMasterImpl].onScenarioExpiredTimeout: PackerSession(" << packerSessionId << ") assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ") expired without completing, sessionData(" << *sessionInfo.mSessionData << ")"); // PACKER_TODO: Change to trace
#if 0
        // PACKER_TODO: Figure out if we need this in case of a timeout, the worker itself knows the time, and he can remove the session when it has been expired completely without getting updates from the master about it.
        if (assignedInstanceId != INVALID_INSTANCE_ID)
        {
            // notify assigned worker ?
            Blaze::GameManager::NotifyWorkerMatchmakingSessionRemoved notifyRemoved;
            notifyRemoved.setMatchmakingSessionId(packerSessionId);
            sendNotifyWorkerMatchmakingSessionRemovedToInstanceById(assignedInstanceId, &notifyRemoved);
        }
#endif
        auto now = TimeValue::getTimeOfDay();
        auto duration = now - sessionInfo.mSessionData->getStartedTimestamp();
        auto* hostJoinInfo = sessionInfo.getHostJoinInfo();
        auto hostUserSessionId = hostJoinInfo->getUser().getSessionId();
        auto originatingScenarioId = hostJoinInfo->getOriginatingScenarioId();
        GameManager::NotifyMatchmakingFailed notifyFailed;
        notifyFailed.setUserSessionId(hostUserSessionId);
        notifyFailed.setSessionId(packerSessionId);
        notifyFailed.setScenarioId(originatingScenarioId);
        notifyFailed.setMatchmakingResult(GameManager::SESSION_TIMED_OUT);
        notifyFailed.setMaxPossibleFitScore(100);
        notifyFailed.getSessionDuration().setSeconds(duration.getSec());
        sendNotifyMatchmakingFailedToSliver(UserSession::makeSliverIdentity(hostUserSessionId), &notifyFailed); // PACKER_TODO: Packer session notifications should be changed to occur per scenario not per session
    }

    scenarioInfo.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_TIMED_OUT);
    cleanupScenario(scenarioInfo);
}

void GamePackerMasterImpl::onScenarioFinalizationTimeout(GameManager::MatchmakingScenarioId scenarioId)
{
    auto scenarioItr = mScenarioByIdMap.find(scenarioId);
    if (scenarioItr == mScenarioByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].onScenarioFinalizationTimeout: Scenario(" << scenarioId << ") expected!");
        return;
    }

    auto& scenarioInfo = scenarioItr->second;
    scenarioInfo.mFinalizationTimerId = INVALID_TIMER_ID;
    auto& scenarioData = *scenarioInfo.mScenarioData;
    auto lockedByPackerSessionId = scenarioData.getLockedForFinalizationByPackerSessionId();
    if (lockedByPackerSessionId == INVALID_PACKER_SESSION_ID)
    {
        WARN_LOG("[GamePackerMasterImpl].onScenarioFinalizationTimeout: Scenario(" << scenarioId << ") unexpected not locked by packer session!");
        return;
    }

    auto sessionItr = mSessionByIdMap.find(lockedByPackerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        ERR_LOG("[GamePackerMasterImpl].onScenarioFinalizationTimeout: PackerSession(" << lockedByPackerSessionId << ") that locked Scenario(" << scenarioId << ") for finalization unexpectedly missing!");
        return;
    }

    auto& sessionInfo = sessionItr->second;
    auto now = TimeValue::getTimeOfDay();
    auto delta = now - scenarioData.getLockedTimestamp();
    WARN_LOG("[GamePackerMasterImpl].onScenarioFinalizationTimeout: Scenario(" << scenarioId << "), scenarioInfo(" << scenarioData << ") finalization timeout(" << delta.getMillis() << "ms) exceeded, abort PackerSession(" << lockedByPackerSessionId << "), sessionData(" << *sessionInfo.mSessionData << "), requestInfo(" << *sessionInfo.mRequestInfo << ")");

    abortSession(sessionInfo);
}

void GamePackerMasterImpl::signalSessionAssignment(uint32_t layerId)
{
    uint32_t layerIndex = LayerInfo::layerIndexFromLayerId(layerId);
    uint32_t slotIdStart = 1;
    uint32_t slotIdEnd = LayerInfo::MAX_SLOTS + 1;
    if (layerIndex < mLayerCount)
    {
        slotIdStart = 1U << (layerIndex);
        slotIdEnd = 1U << (layerIndex + 1);
    }

    uint32_t loadSum = 0;
    eastl::vector<WorkerInfo*> workers;
    workers.reserve(mWorkerBySlotId.size());
    // iterate active workers by slot id
    for (auto workerItr = mWorkerBySlotId.lower_bound(slotIdStart), workerEnd = mWorkerBySlotId.end(); (workerItr != workerEnd) && (workerItr->first < slotIdEnd); ++workerItr)
    {
        auto& worker = *workerItr->second;
        auto* remoteInstance = gController->getRemoteServerInstance(worker.mWorkerInstanceId);
        if (remoteInstance != nullptr)
        {
            if (remoteInstance->isDraining())
                continue; // Respect draining state
            auto instanceLoad = remoteInstance->getTotalLoad();
            if (instanceLoad < 1)
                instanceLoad = 1;
            loadSum += instanceLoad;
            workers.push_back(&worker);
        }
        else
        {
            WARN_LOG("[GamePackerMasterImpl].signalSessionAssignment: Unknown worker(" << worker.mWorkerInstanceId << ") on layer(" << layerId << "), skipping"); // this should never happen because we always immediately clean up workers when they go away
        }
    }

    if (workers.empty())
    {
        WARN_LOG("[GamePackerMasterImpl].signalSessionAssignment: No instances on layer(" << layerId << ")!"); // PACKER_TODO: change to trace
        return;
    }
    
    size_t numWorkers = 0;
    auto cpuThreshold = (float) getConfig().getWorkerSettings().getTargetCpuThreshold();
    if (cpuThreshold > 0)
    {
        EA_ASSERT(loadSum > 0);
        numWorkers = (size_t) ceilf(loadSum / cpuThreshold);
        if (numWorkers > workers.size())
            numWorkers = workers.size();
    }
    else
    {
        // This is useful for testing the system, it forces utilization of the entire set of available workers
        numWorkers = workers.size();
    }

#if 1
    // ALGORITHM A: unweighted random index into load map (avoids aggressively targeting the least loaded instance)
    // This algorithm works but it does not try to correct for the fact that the root worker also has to handle all migrated sessions which results in additional load there.
    uint32_t startIndex = Random::getRandomNumber(numWorkers);
    auto* workerToWake = workers.at(startIndex);
#else
    // ALGORITHM B: weighted random index into the inverse load map (compensates for load imbalances)
    // PACKER_TODO: move most of this algorithm into Component::getInstanceByLoad()
    // PACKER_TODO: #ALGORITHM This algorithm works but when tested in the load testing environment there were some unexpected anomalies that need to be explained/resolved:
    // 1) The rate(blaze_matchmaker_scenario_sessionStarted_count{service_name="$ServiceName",instance_type="mmSlave"}[20m]) metrics were highly skewed to the top 2/3 active mm slaves
    // 2) .. Yet, the max_over_time(blaze_fibers_cpuUsageForProcessPercent{service_name="$ServiceName",instance_type="mmSlave"}[20m]) is actually more narrow between all 3/3 active mm slaves
    // These may well be due to invalid metrics. Once the OIV metrics system actually works correctly, we can reenable the algorithm and 
    auto* workerToWake = workers.front();
    if (numWorkers > 1)
    {
        uint32_t inverseLoadSum = 0;
        eastl::vector_map<uint32_t, InstanceId> instanceIdByInverseLoadMap;
        instanceIdByInverseLoadMap.reserve(numWorkers);
        for (auto workerItr = workers.begin(), workerEnd = workers.begin() + numWorkers; workerItr != workerEnd; ++workerItr)
        {
            auto& worker = **workerItr;
            auto* remoteServer = gController->getRemoteServerInstance(worker.mWorkerInstanceId);
            if (remoteServer)
            {
                // NOTE: Intentionally use user load(vs. total load) to load balance because it is statistically much more directly correlated to amount of work the instance is actually performing itself
                auto inverseLoad = RemoteServerInstance::MAX_LOAD - eastl::min(RemoteServerInstance::MAX_LOAD, remoteServer->getUserLoad());
                if (inverseLoad == 0)
                    inverseLoad = 1; // Always add one since we don't want to avoid 100% loaded instances entirely if they are all that is left
                inverseLoadSum += inverseLoad;
                instanceIdByInverseLoadMap[inverseLoadSum] = worker.mWorkerInstanceId);
            }
            else
                EA_FAIL_MSG("Remove server worker instance disappeared unexpectedly!");
        }

        if (!instanceIdByInverseLoadMap.empty())
        {
            uint32_t startIndex = Random::getRandomNumber(inverseLoadSum); // randomly index into load map, to avoid too aggressively targeting the least loaded intance
            auto loadItr = instanceIdByInverseLoadMap.lower_bound(startIndex);
            if (loadItr != instanceIdByInverseLoadMap.end())
                workerToWake = mWorkerByInstanceId.find(loadItr->second)->second;
            else
                workerToWake = mWorkerByInstanceId.find(instanceIdByInverseLoadMap.back().second)->second;
        }
    }
#endif

    if (workerToWake->mWorkerState == WorkerInfo::IDLE)
    {
        wakeIdleWorker(*workerToWake);
    }
    else
    {
        // PACKER_TODO: add a metric when a worker for the number of times a worker that was selected for assignment was already working.
    }
}

void GamePackerMasterImpl::setWorkerState(WorkerInfo& worker, GamePackerMasterImpl::WorkerInfo::WorkerState state)
{
    auto& layer = getLayerByWorker(worker);

    worker.cancelResetStateTimer();
    WorkerInfoList::remove(worker);       // Remove from layer.mWorkerByState
    worker.mWorkerState = state;
    layer.mWorkerByState[worker.mWorkerState].push_back(worker);
//    if (state == WorkerInfo::SIGNALLED)
//        worker.mWorkerSignalledTimestamp = TimeValue::getTimeOfDay();

    worker.startResetStateTimer();
}

void GamePackerMasterImpl::assignWorkerSession(WorkerInfo& worker, SessionInfo& sessionInfo)
{
    // There's no reason to switch to a different state here.  The Session's WORKING state indicates nothing, since it's still safe to assign new sessions to that worker. 
    // setWorkerState(worker, WorkerInfo::WORKING);

    auto& layer = getLayerByWorker(worker);
    layer.mSessionListByState[SessionInfo::WORKING].push_back(sessionInfo);
}


bool GamePackerMasterImpl::wakeIdleWorker(WorkerInfo& worker)
{
    if (worker.mWorkerState != WorkerInfo::IDLE && worker.mWorkerState != WorkerInfo::BUSY)
    {
        ERR_LOG("[GamePackerMasterImpl].wakeIdleWorker: Only idle/busy workers may be woken.  Worker '" << worker.mWorkerName << "'  state is: " << WorkerInfo::getWorkerStateString(worker.mWorkerState));
        return false;
    }

    auto& layer = getLayerByWorker(worker);
    if (mUnassignedSessionList.empty() && layer.mSessionListByState[SessionInfo::IDLE].empty())
        return false; // nothing to do

    TRACE_LOG("[GamePackerMasterImpl].wakeIdleWorker: Notify worker(" << worker.mWorkerName << ")");

    // There's no reason to switch to a different state here.  The cost of sending extra notifications is low, relative to the cost of obtaining sessions.
    // setWorkerState(worker, WorkerInfo::SIGNALLED);

    GameManager::NotifyWorkerPackerSessionAvailable notif;
    notif.setInstanceId(gController->getInstanceId());
    sendNotifyWorkerPackerSessionAvailableToInstanceById(worker.mWorkerInstanceId, &notif);
    return true;
}

// PACKER_TODO: Move this into a common utility so that it can be accessed in both the search slaves (when returning games matched by findgames), and also here in packer master.
BlazeError GamePackerMasterImpl::initializeAggregateProperties(GameManager::StartMatchmakingInternalRequest& internalRequest, GameManager::MatchmakingCriteriaError& criteriaError)
{
    // validate request ...
    auto* gameTemplateName = internalRequest.getCreateGameTemplateName();
    auto& createGameTemplatesMap = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates();
    auto cfgItr = createGameTemplatesMap.find(gameTemplateName);
    if (cfgItr == createGameTemplatesMap.end())
    {
        ASSERT_LOG("[GamePackerMasterImpl].initializeAggregateProperties: unknown game template(" << gameTemplateName << ")");
        return StartPackerSessionError::ERR_SYSTEM;
    }
    
    auto& filters = internalRequest.getMatchmakingFilters();
    auto& propertyMap = internalRequest.getMatchmakingPropertyMap();

    // PACKER_TODO:
    /**
     * Currently this method is used to construct the Filter TDFs that will be used in the PropertyFiltersRule. 
       This functionality could be done on the coreSlave, in the Scenarios code, prior to sending the request to the Master. 
       
       Updating values dynamically presents a variety of problems, all of which have to be solved in order to avoid introducing impossible to diagnose problems.  
         Examples:  Changing UED values while matchmaking (dynamic packer values).  Entering a 'large-gamegroup' subsession, then having group members leave (Scenario Variant choice).  Pingsite Qos becomes invalid (Input Sanitizers).  Etc.

       I think we probably want to avoid this anyway to ensure that matchmaking is predictable.
       A simpler option, if it needs to be refreshed, is to just have the coreSlave cancel MM and reissue a fresh scenario reusing the original request.
    */
    EA::TDF::GenericTdfType* platformFilter = nullptr;
    for (GameManager::MatchmakingFilterCriteriaMap::iterator curFilter = filters.getMatchmakingFilterCriteriaMap().begin(); curFilter != filters.getMatchmakingFilterCriteriaMap().end(); )
    {
        auto& curFilterName = curFilter->first;
        auto& curFilterGeneric = curFilter->second;

        const auto filtersConfig = getConfig().getFilters().find(curFilterName);
        if (filtersConfig == getConfig().getFilters().end())
        {
            ERR_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Property Filter " << curFilterName << " no longer exists.");
            return StartPackerSessionError::GAMEPACKER_ERR_INVALID_MATCHMAKING_CRITERIA;
        }

        EA::TDF::TdfPtr outFilter;
        const char8_t* failingAttr = nullptr;
        BlazeRpcError blazeErr = applyTemplateTdfAttributes(outFilter, filtersConfig->second->getRequirement(), internalRequest.getScenarioAttributes(), propertyMap, failingAttr, "[MatchmakingScenarioManager].createScenario: ");
        if (blazeErr != ERR_OK)
        {
            // PACKER_TODO - All rules should have an explicit way to be disabled, rather than relying on missing attributes:
            if (blazeErr == GAMEMANAGER_ERR_MISSING_TEMPLATE_ATTRIBUTE)
            {
                TRACE_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Property Filter " << curFilterName << ", missing value for the attribute " << failingAttr << ".  Disabling/Removing that Filter.");
                curFilter = filters.getMatchmakingFilterCriteriaMap().erase(curFilter);
                continue;
            }
            ERR_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Property Filter " << curFilterName << ", unable to apply value from config for the attribute " << failingAttr << ".");
            return blazeErr;
        }

        curFilterGeneric->set(*outFilter);
        ++curFilter;

        // Special case for Platform Filter:
        if (outFilter->getTdfId() == GameManager::PlatformFilter::TDF_ID)
        {
            if (platformFilter == nullptr)
            {
                platformFilter = curFilterGeneric;
            }
            else
            {
                ERR_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Property Filter " << curFilterName << " defines a Platform Rule when one is already in use for this Scenario.");
                return blazeErr;
            }
        }
    }

    // Platform Logic:  For Crossplay support, we have to check that the platforms of the users and configs are all valid.
    // PACKER_TODO - For consistency, the values (XP enabled flag, Platform) used here should be Properties, rather than requiring the full UsersInfo.
    if (platformFilter != nullptr)
    {
        // Crossplay validation check:  (Overrides for Crossplay settings are not available for Scenarios)
        bool isCrossplayEnabled = false;
        GameManager::PlatformFilter& filter = *(GameManager::PlatformFilter*)&platformFilter->getValue().asTdf();
        auto errResult = GameManager::ValidationUtils::validateRequestCrossplaySettings(isCrossplayEnabled, internalRequest.getUsersInfo(), internalRequest.getRequest().getSessionData().getSessionMode().getCreateGame(),
            internalRequest.getRequest().getGameCreationData(), getConfig().getGameSession(),
            filter.getClientPlatformListOverride(), filter.getCrossplayMustMatch());
        if (errResult != ERR_OK)
        {
            criteriaError.setErrMessage((StringBuilder() << "Crossplay enabled user attempted to create a game with invalid settings. " << ErrorHelp::getErrorDescription(errResult)).c_str());
            ERR_LOG("[MatchmakingScenarioManager].startMatchmaking:  Crossplay enabled user attempted to create a game with invalid settings.  Unable to just disable crossplay to fix this.");
            return errResult;
        }

        // Set a platform restriction for the default use case where a CP user was trying to create a CP Enabled Game, but something forced it to be CP disabled.
        if (!isCrossplayEnabled)
        {
            filter.setCrossplayMustMatch(true);
        }

        // Set the RequiredPlatformList:
        if (filter.getCrossplayMustMatch())
        {
            filter.getClientPlatformListOverride().copyInto(filter.getRequiredPlatformList());
        }
        else
        {
            // Add all platforms used by Users
            filter.getRequiredPlatformList().clear();
            for (auto& userInfo : internalRequest.getUsersInfo())
            {
                ClientPlatformType plat = userInfo->getUser().getUserInfo().getPlatformInfo().getClientPlatform();

                // Note:  We don't add all non-crossplay platforms, since non-crossplay only applies in cases where XP is disabled.  (And thus would hit the MustMatch check above)
                // It doesn't apply if you have explicitly set platforms - so [pc, xone] will NOT create a Game with [(pc, steam), xone], since the Game was already XP with explicit platforms.
                filter.getRequiredPlatformList().insertAsSet(plat);
            }
        }
    }
    else
    {
        criteriaError.setErrMessage((StringBuilder() << " Missing Platform Filter for incoming request. ").c_str());
        ERR_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Missing Platform Filter for incoming request.");
        return GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
    }


    int64_t hash = (int64_t) EA::StdC::FNV64_String8(gameTemplateName, EA::StdC::kFNV64InitialValue, EA::StdC::kCharCaseLower);
    for (auto& gameQualiftyFactorConfig : cfgItr->second->getPackerConfig().getQualityFactors()) 
    {
        hash = (int64_t)EA::StdC::FNV64_String8(gameQualiftyFactorConfig->getGameProperty(), (uint64_t)hash, EA::StdC::kCharCaseLower);
        if (gameQualiftyFactorConfig->getScoreShaper().size() != 1)
        {
            // This shouldn't happen.  Check at config time. 
            ERR_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Score shaper set incorrectly for quality factor "<< gameQualiftyFactorConfig->getGameProperty() << " in " << gameTemplateName);
            continue;
        }

        // An alternative to this is to build the ScoreShaper, using applyTemplateTdfAttributes, and hash any values that did not have a default set (or had a mapped value...).
        auto& scoreShaperTdfMapping = gameQualiftyFactorConfig->getScoreShaper().begin()->second;
        for (auto curTdfMapping : *scoreShaperTdfMapping)
        {
            float value = 0;
            auto& paramName = curTdfMapping.first;

            // Only care about attributes:
            auto curAttr = internalRequest.getScenarioAttributes().find(curTdfMapping.second->getAttrName());
            if (curAttr != internalRequest.getScenarioAttributes().end())
            {
                if (curAttr->second->convertToResult(value))
                {
                    // If a value is being mapped, hash both the value's name, and the value itself, so that the hash will remain unique:
                    hash = (int64_t)EA::StdC::FNV64_String8(paramName.c_str(), (uint64_t)hash, EA::StdC::kCharCaseLower);
                    hash = (int64_t)EA::StdC::FNV64(&value, sizeof(value), (uint64_t)hash);
                }
            }

            auto curProp = propertyMap.find(curTdfMapping.second->getPropertyName());
            if (curProp != propertyMap.end())
            {
                if (curProp->second->convertToResult(value))
                {
                    // If a value is being mapped, hash both the value's name, and the value itself, so that the hash will remain unique:
                    hash = (int64_t)EA::StdC::FNV64_String8(paramName.c_str(), (uint64_t)hash, EA::StdC::kCharCaseLower);
                    hash = (int64_t)EA::StdC::FNV64(&value, sizeof(value), (uint64_t)hash);
                }
            }
        }
    }

    if (filters.getMatchmakingFilterCriteriaMap().find(GameManager::PropertyManager::FILTER_NAME_QF_HASH) != filters.getMatchmakingFilterCriteriaMap().end())
    {
        const EA::TDF::TdfPtr& filterCriteria = (const EA::TDF::TdfPtr&)& filters.getMatchmakingFilterCriteriaMap()[GameManager::PropertyManager::FILTER_NAME_QF_HASH]->getValue().asTdf();   // PACKER_TODO: Clean up this conversion code
        GameManager::IntEqualityFilter& filter = (GameManager::IntEqualityFilter&)*filterCriteria;
        filter.setValue(hash);
        filter.setOperation(GameManager::EQUAL);
        TRACE_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Set filter (qfHashFilter), gameProperty(game." << GameManager::PROPERTY_NAME_QUALITY_FACTORS_HASH << ") to value " << hash);
    }
    else
    {
        ERR_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Missing qfHashFilter for " << gameTemplateName);
    }
    
    return ERR_OK;
}

void GamePackerMasterImpl::assignSession(SessionInfo& assignSessionInfo)
{
    EA_ASSERT(assignSessionInfo.mSessionData->getState() == GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
    SessionInfoList::remove(assignSessionInfo);
    auto assignedLayerId = assignSessionInfo.mSessionData->getAssignedLayerId();
    auto layerIndex = LayerInfo::layerIndexFromLayerId(assignedLayerId);
    mLayerList[layerIndex].mSessionListByState[SessionInfo::IDLE].push_back(assignSessionInfo);
    signalSessionAssignment(assignedLayerId);
    upsertSessionData(assignSessionInfo);
}

void GamePackerMasterImpl::abortSession(SessionInfo& abortSessionInfo)
{
    auto abortPackerSessionId = abortSessionInfo.mRequestInfo->getMatchmakingSessionId();
    auto& scenarioInfo = *abortSessionInfo.mParentScenario;
    auto lockedForFinalizationByPackerSessionId = scenarioInfo.mScenarioData->getLockedForFinalizationByPackerSessionId();
    bool wasLockedForFinalizaion = (lockedForFinalizationByPackerSessionId == abortPackerSessionId);
    if (wasLockedForFinalizaion)
    {
        TRACE_LOG("[GamePackerMasterImpl].abortSession: Aborted PackerSession(" << abortPackerSessionId << ") is locking Scenario(" << scenarioInfo.mScenarioId << ") for finalization");
        scenarioInfo.mScenarioData->setLockedForFinalizationByPackerSessionId(INVALID_PACKER_SESSION_ID);
        scenarioInfo.cancelFinalizationTimer();
        // we are aborting a session that had the scenario locked, if any other sibling sessions are currently suspended we should resume them (in order of priority)

        for (auto resumeSessionId : scenarioInfo.mSessionByIdSet)
        {
            if (resumeSessionId == abortPackerSessionId)
                continue;
                
            auto sessionItr = mSessionByIdMap.find(resumeSessionId);
            if (sessionItr == mSessionByIdMap.end())
                continue;

            auto& resumeSessionInfo = sessionItr->second;
            auto sessionState = resumeSessionInfo.mSessionData->getState();
            if (sessionState != GameManager::PackerSessionData::SESSION_SUSPENDED)
            {
                TRACE_LOG("[GamePackerMasterImpl].abortSession: PackerSession(" << abortPackerSessionId << ") locking Scenario(" << resumeSessionInfo.mParentScenario->mScenarioId << ") has been aborted, skipping sibling PackerSession(" << resumeSessionInfo.getPackerSessionId() << ") in state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ")" );
                continue; // only suspended sessions can be resumed
            }

            TRACE_LOG("[GamePackerMasterImpl].abortSession: PackerSession(" << abortPackerSessionId << ") locking Scenario(" << resumeSessionInfo.mParentScenario->mScenarioId << ") has been aborted, resuming suspended sibling PackerSession(" << abortPackerSessionId << ") on layer(" << resumeSessionInfo.mSessionData->getAssignedLayerId() << ")");
        
            resumeSessionInfo.mSessionData->setState(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);

            assignSession(resumeSessionInfo); // takes care of calling upsertSessionData()
        }
    }

    auto* hostJoinInfo = abortSessionInfo.getHostJoinInfo();
    EA_ASSERT(hostJoinInfo != nullptr);
    auto hostUserSessionId = hostJoinInfo->getUser().getSessionId();
    auto originatingScenarioId = hostJoinInfo->getOriginatingScenarioId();

    // PACKER_TODO: #ARCHITECTURE the interface between core slave and packer master no longer includes creating individual subsessions it would make more sense to avoid sending this notification when the scenario itself can still complete by finalizing one of the remaining sub-sessions...

    auto now = TimeValue::getTimeOfDay();
    auto duration = now - abortSessionInfo.mSessionData->getStartedTimestamp();

    GameManager::NotifyMatchmakingFailed notifyFailed;
    notifyFailed.setUserSessionId(hostUserSessionId);
    notifyFailed.setSessionId(abortPackerSessionId);
    notifyFailed.setScenarioId(originatingScenarioId);
    notifyFailed.setMatchmakingResult(GameManager::SESSION_ERROR_GAME_SETUP_FAILED); // PACKER_TODO: This is way too generic of an error, we need to provide a much more contextually useful type of error
    notifyFailed.setMaxPossibleFitScore(100);
    notifyFailed.getSessionDuration().setSeconds(duration.getSec());
    sendNotifyMatchmakingFailedToSliver(UserSession::makeSliverIdentity(hostUserSessionId), &notifyFailed);

    bool hasRemainingValidSessions = false;
    for (auto sessionId : scenarioInfo.mSessionByIdSet)
    {
        if (sessionId == abortPackerSessionId)
            continue;
        auto sessionItr = mSessionByIdMap.find(sessionId);
        if (sessionItr == mSessionByIdMap.end())
            continue;
        if (sessionItr->second.mSessionData->getState() != GameManager::PackerSessionData::SESSION_ABORTED)
        {
            hasRemainingValidSessions = true;
            break;
        }
    }

    abortSessionInfo.mSessionData->setState(GameManager::PackerSessionData::SESSION_ABORTED);
    abortSessionInfo.mSessionData->setReleaseCount(abortSessionInfo.mSessionData->getReleaseCount() + 1);
    abortSessionInfo.mSessionData->setReleasedTimestamp(now);
    abortSessionInfo.mSessionData->setAssignedWorkerInstanceId(INVALID_INSTANCE_ID); // we always clear assigned instance because cleanup scenario tries to send back session removal notifications to workers that remain and we want to avoid that

    if (hasRemainingValidSessions)
    {
        TRACE_LOG("[GamePackerMasterImpl].abortSession: Aborted PackerSession(" << abortPackerSessionId << "), Scenario(" << scenarioInfo.mScenarioId << ") has remaining valid sessions, update scenario");
        upsertSessionData(abortSessionInfo);
        if (wasLockedForFinalizaion)
            upsertScenarioData(scenarioInfo);
    }
    else
    {
        WARN_LOG("[GamePackerMasterImpl].abortSession: Aborted PackerSession(" << abortPackerSessionId << "), Scenario(" << scenarioInfo.mScenarioId << ") has no remaining valid sessions, cleanup scenario");
        scenarioInfo.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_FAILED);
        cleanupScenario(scenarioInfo);
    }
}

void GamePackerMasterImpl::cleanupScenario(ScenarioInfo& scenarioInfo)
{
    const auto cleanupScenarioId = scenarioInfo.mScenarioId;
    const auto terminationReason = scenarioInfo.mScenarioData->getTerminationReason();
    TRACE_LOG("[GamePackerMasterImpl].cleanupScenario: Scenario(" << cleanupScenarioId << "), reason(" << GameManager::PackerScenarioData::TerminationReasonToString(terminationReason) << ")");

    if (terminationReason != GameManager::PackerScenarioData::SCENARIO_COMPLETED)
    {
        auto matchmakingResult = GameManager::SESSION_ERROR_GAME_SETUP_FAILED;
        switch (terminationReason)
        {
        case GameManager::PackerScenarioData::SCENARIO_TIMED_OUT:
            matchmakingResult = GameManager::SESSION_TIMED_OUT;
            break;
        case GameManager::PackerScenarioData::SCENARIO_CANCELED:
            matchmakingResult = GameManager::SESSION_CANCELED;
            break;
        case GameManager::PackerScenarioData::SCENARIO_OWNER_LEFT:
            matchmakingResult = GameManager::SESSION_TERMINATED;
            break;
        case GameManager::PackerScenarioData::SCENARIO_FAILED:
            matchmakingResult = GameManager::SESSION_ERROR_GAME_SETUP_FAILED;
            break;
        default:
            WARN_LOG("[GamePackerMasterImpl].cleanupScenario: Scenario(" << cleanupScenarioId << "), unexpected result(" << GameManager::MatchmakingResultToString(matchmakingResult) << ")");
            break;
        }

        // submit matchmaking failed event, this will catch the game creator and all joining sessions.
        GameManager::FailedMatchmakingSession failedMatchmakingSession;
        //failedMatchmakingSession.setMatchmakingSessionId(abortPackerSessionId); /// @deprecated intentionally omitted and should be removed with legacy MM
        failedMatchmakingSession.setMatchmakingScenarioId(scenarioInfo.mScenarioId);
        failedMatchmakingSession.setUserSessionId(scenarioInfo.mOwnerUserSessionInfo->getSessionId());
        failedMatchmakingSession.setMatchmakingResult(GameManager::MatchmakingResultToString(matchmakingResult));
        gEventManager->submitEvent((uint32_t)GameManager::GameManagerMaster::EVENT_FAILEDMATCHMAKINGSESSIONEVENT, failedMatchmakingSession);

        // get any session belonging to this scenario (needed to fill out the PIN event)
        // the first session will do
        EA_ASSERT(!scenarioInfo.mSessionByIdSet.empty());
        auto sessionItr = mSessionByIdMap.find(*(scenarioInfo.mSessionByIdSet.begin()));
        auto& sessionInfo = sessionItr->second;
        auto now = TimeValue::getTimeOfDay();
        auto duration = now - scenarioInfo.mScenarioData->getStartedTimestamp();

        // send PIN events for failed matchmaking
        for (auto& userJoinInfo : scenarioInfo.mUserJoinInfoList)
        {
            PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
            GameManager::PINEventHelper::buildMatchJoinForFailedUserSessionInfo(*(sessionInfo.mRequestInfo.get()), *(sessionInfo.getHostJoinInfo()),
                scenarioInfo.mUserJoinInfoList, duration, matchmakingResult, userJoinInfo->getUser(), *pinSubmission);
            gUserSessionManager->sendPINEvents(pinSubmission);
        }
    }

    for (auto& userJoinInfo : scenarioInfo.mUserJoinInfoList)
    {
        BlazeId blazeId = userJoinInfo->getUser().getUserInfo().getId();

        // remove all blazeId->scenarioId mapping pairs that reference this scenario
        auto itrPair = mScenarioIdByBlazeIdMap.equal_range(blazeId);
        while (itrPair.first != itrPair.second)
        {
            auto scenarioId = itrPair.first->second;
            if (cleanupScenarioId == scenarioId)
                itrPair.first = mScenarioIdByBlazeIdMap.erase(itrPair.first);
            else
                ++itrPair.first;
        }
    }

    const auto now = TimeValue::getTimeOfDay();
    GameManager::NotifyWorkerPackerSessionTerminated notifyTerminated;
    notifyTerminated.setTerminationReason(terminationReason);
    for (auto erasePackerSessionId : scenarioInfo.mSessionByIdSet)
    {
        auto sessionItr = mSessionByIdMap.find(erasePackerSessionId);
        if (sessionItr == mSessionByIdMap.end())
            continue;
        auto& eraseSessionInfo = sessionItr->second;
        auto assignedWorkerInstanceId = eraseSessionInfo.mSessionData->getAssignedWorkerInstanceId();
        if (assignedWorkerInstanceId == INVALID_INSTANCE_ID)
        {
            TRACE_LOG("[GamePackerMasterImpl].cleanupScenario: PackerSession(" << erasePackerSessionId<< ") in Scenario(" << cleanupScenarioId << ") erased.");
        }
        else
        {
            TRACE_LOG("[GamePackerMasterImpl].cleanupScenario: PackerSession(" << erasePackerSessionId<< ") in Scenario(" << cleanupScenarioId << ") erased, notify assigned worker(" << getWorkerNameByInstanceId(assignedWorkerInstanceId) << ")");
            notifyTerminated.setPackerSessionId(erasePackerSessionId);
            sendNotifyWorkerPackerSessionTerminatedToInstanceById(assignedWorkerInstanceId, &notifyTerminated);
        }

        auto sessionResult = GameManager::PackerSessionData::FAILED;
        switch (eraseSessionInfo.mSessionData->getState())
        {
        case GameManager::PackerSessionData::SESSION_COMPLETED:
            sessionResult = GameManager::PackerSessionData::SUCCESS;
            break;
        case GameManager::PackerSessionData::SESSION_ABORTED:
            sessionResult = GameManager::PackerSessionData::FAILED;
            break;
        default:
            switch (terminationReason)
            {
            case GameManager::PackerScenarioData::SCENARIO_COMPLETED:
                sessionResult = GameManager::PackerSessionData::ENDED;
                break;
            case GameManager::PackerScenarioData::SCENARIO_TIMED_OUT:
                sessionResult = GameManager::PackerSessionData::TIMED_OUT;
                break;
            case GameManager::PackerScenarioData::SCENARIO_CANCELED:
                sessionResult = GameManager::PackerSessionData::CANCELED;
                break;
            case GameManager::PackerScenarioData::SCENARIO_OWNER_LEFT:
                sessionResult = GameManager::PackerSessionData::TERMINATED;
                break;
            case GameManager::PackerScenarioData::SCENARIO_FAILED:
                sessionResult = GameManager::PackerSessionData::FAILED;
                break;
            default:
                WARN_LOG("[GamePackerMasterImpl].cleanupScenario: PackerSession(" << erasePackerSessionId<< ") in Scenario(" << cleanupScenarioId << ") unhandled termination reason(" << GameManager::PackerScenarioData::TerminationReasonToString(terminationReason) << ")");
            }
            break;
        }

        const auto duration = (double) (now - eraseSessionInfo.mSessionData->getStartedTimestamp()).getMicroSeconds();
        const auto maxDuration = (double) eraseSessionInfo.mSessionData->getSessionDuration().getMicroSeconds();
        const auto durationPercentile = quantizeRatioAsDurationPercentile(duration, maxDuration);
        const auto* sessionScenarioInfo = eraseSessionInfo.getScenarioInfo();
        mSessionsCompleted.increment(1, sessionScenarioInfo->getScenarioName(), sessionScenarioInfo->getSubSessionName(), sessionResult, durationPercentile);
        mSessionsActive.decrement(1, sessionScenarioInfo->getScenarioName(), sessionScenarioInfo->getSubSessionName());
        mSessionByIdMap.erase(sessionItr);
    }
    const auto startedTime = scenarioInfo.mScenarioData->getStartedTimestamp();
    const auto duration = (double)(now - startedTime).getMicroSeconds();
    const auto maxDuration = (double)(scenarioInfo.mScenarioData->getExpiresTimestamp() - startedTime).getMicroSeconds();
    const auto durationPercentile = quantizeRatioAsDurationPercentile(duration, maxDuration);
    mScenariosCompleted.increment(1, scenarioInfo.getScenarioInfo()->getScenarioName(), terminationReason, durationPercentile);
    mScenariosActive.decrement(1, scenarioInfo.getScenarioInfo()->getScenarioName());
    mScenarioByIdMap.erase(cleanupScenarioId);
    mScenarioTable.eraseRecord(cleanupScenarioId);
}

void GamePackerMasterImpl::upsertSessionData(SessionInfo& sessionInfo)
{
    StorageFieldName fieldName;
    fieldName.sprintf(PRIVATE_PACKER_SESSION_FIELD_FMT, sessionInfo.getPackerSessionId());
    mScenarioTable.upsertField(sessionInfo.mParentScenario->mScenarioId, fieldName.c_str(), *sessionInfo.mSessionData);
}

void GamePackerMasterImpl::upsertRequestData(SessionInfo& sessionInfo)
{
    StorageFieldName fieldName;
    fieldName.sprintf(PRIVATE_PACKER_REQUEST_FIELD_FMT, sessionInfo.getPackerSessionId());
    mScenarioTable.upsertField(sessionInfo.mParentScenario->mScenarioId, fieldName.c_str(), *sessionInfo.mRequestInfo);
}

void GamePackerMasterImpl::upsertScenarioData(ScenarioInfo& scenarioInfo)
{
    StorageFieldName fieldName;
    mScenarioTable.upsertField(scenarioInfo.mScenarioId, PRIVATE_PACKER_SCENARIO_FIELD, *scenarioInfo.mScenarioData);
}

EA::TDF::TimeValue GamePackerMasterImpl::getScenarioExpiryWithGracePeriod(EA::TDF::TimeValue expiry) const
{
    return expiry + getConfig().getScenarioExpiryGracePeriod();
}

Blaze::GamePacker::GamePackerMasterImpl::LayerInfo& GamePackerMasterImpl::getLayerByWorker(const WorkerInfo& worker)
{
    auto& layer = mLayerList[LayerInfo::layerIndexFromLayerId(worker.mWorkerLayerId)];
    EA_ASSERT_M(worker.mWorkerLayerId == layer.mLayerId, "Worker layerId mismatch!");
    return layer;
}

void GamePackerMasterImpl::onShutdown()
{
    gUserSessionManager->removeSubscriber(*this);

    mDedicatedServerOverrideMap.deregisterCollection();

    // detach the storage table, no slivers will be imported or exported anymore
    if (gStorageManager != nullptr)
    {
        gStorageManager->deregisterStorageTable(mScenarioTable);
    }

    gController->deregisterDrainStatusCheckHandler(Blaze::GamePacker::GamePackerMaster::COMPONENT_INFO.name);
}

void GamePackerMasterImpl::onSlaveSessionRemoved(SlaveSession& session)
{
    const auto workerInstanceId = SlaveSession::getSessionIdInstanceId(session.getId());

    TRACE_LOG("[GamePackerMasterImpl].onSlaveSessionRemoved: sessionId="
        << SbFormats::HexLower(session.getId()) << ", instanceId=" << workerInstanceId);

    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr != mWorkerByInstanceId.end())
    {
        auto& worker = *workerItr->second;
        auto& layer = getLayerByWorker(worker);
        mWorkerBySlotId.erase(worker.mWorkerSlotId);
        mWorkerByInstanceId.erase(workerItr);
        delete &worker;
        if (mWorkerByInstanceId.empty())
        {
            if (!layer.hasWorkers())
            {
                // if we remove the last worker from the layer we need to reassign all the sessions assigned to this layer back into the unassigned list
                for (auto& fromList : layer.mSessionListByState)
                {
                    mUnassignedSessionList.splice(mUnassignedSessionList.end(), fromList);
                    // PACKER_TODO: we need to pump the assignSessions() logic here, because we have just added to the unassigned sessions list.
                }
            }
        }
    }

    // PACKER_TODO: #IMPORTANT We need to walk all sessions that were assigned to the removed worker, and if they are not locked we must put them back into the unassigned session queue, and if they are locked we probably need to abort them. Alternatively it may still be possible to recover locked sessions and reassign them but they would need to deal with issues such as how to deal with progressing game creation, etc.
}

void GamePackerMasterImpl::onUserSessionExtinction(const UserSession& userSession)
{
    BlazeId blazeId = userSession.getBlazeId();
    UserSessionId extinctSessionId = userSession.getSessionId();
    auto scenarioIdByBlazeIdItr = mScenarioIdByBlazeIdMap.find(blazeId);
    if (scenarioIdByBlazeIdItr == mScenarioIdByBlazeIdMap.end())
    {
        TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, no associated scenarios");
        return;
    }
    
    do {
        auto scenarioId = scenarioIdByBlazeIdItr->second;
        auto scenarioItr = mScenarioByIdMap.find(scenarioId);
        if (scenarioItr == mScenarioByIdMap.end())
        {
            ERR_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, expected Scenario(" << scenarioId << ") not found, remove user->scenario mapping");
            mScenarioIdByBlazeIdMap.erase(scenarioIdByBlazeIdItr);
            continue;
        }

        auto& scenarioInfo = scenarioItr->second;
        bool isScenarioOwnerLeaving = scenarioInfo.mOwnerUserSessionInfo->getSessionId() == extinctSessionId;
        if (isScenarioOwnerLeaving || scenarioInfo.mUserJoinInfoList.size() == 1)
        {
            TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, terminate " << (isScenarioOwnerLeaving ? "explicitly" : "implicitly") << " owned Scenario(" << scenarioId << ")");

            // PACKER_TODO: #FUTURE We will need to treat users differently depending on how the scenario is configured when a member unexpectedly logs out. e.g: if the the scenario supports ownership transfer we may be able to issue an update for all the live sub sessions instead of aborting the owned scenario outright, and force the assigned packer slave to repack the corresponding party.

            // PACKER_TODO: ensure notification going back to client is accurate
            scenarioInfo.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_OWNER_LEFT);
            cleanupScenario(scenarioInfo);
        }
        else
        {
            // This handles the case of updating the scenario
            EA_ASSERT(scenarioInfo.mUserJoinInfoList.size() > 1);

            GameManager::UserJoinInfoList userJoinInfoList;
            userJoinInfoList.reserve(scenarioInfo.mUserJoinInfoList.size() - 1);

            for (auto& userJoinInfo : scenarioInfo.mUserJoinInfoList)
            {
                if (userJoinInfo->getUser().getSessionId() != extinctSessionId)
                {
                    userJoinInfoList.push_back(userJoinInfo); // add all remaining user infos
                }
            }

            if (scenarioInfo.mUserJoinInfoList.size() == userJoinInfoList.size())
            {
                TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, active Scenario(" << scenarioId << ") associated with BlazeId(" << blazeId << ") unaffected");
                mScenarioIdByBlazeIdMap.erase(scenarioIdByBlazeIdItr);
                continue;
            }

            TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, update affected Scenario(" << scenarioId << ")");
            mScenarioIdByBlazeIdMap.erase(scenarioIdByBlazeIdItr);
            
            // PACKER_TODO: need to move this member to PackerScenarioData so it can be backed by storage manager...
            scenarioInfo.mUserJoinInfoList.swap(userJoinInfoList); // update scenario info list to now reflect the remaining users

            EA_ASSERT(scenarioInfo.mUserJoinInfoList.size() + 1 == userJoinInfoList.size());
            
            eastl::vector<PackerSessionId> failedSessionIds;
            GameManager::NotifyWorkerPackerSessionRelinquish updated;
            for (auto updatePackerSessionId : scenarioInfo.mSessionByIdSet)
            {
                auto sessionItr = mSessionByIdMap.find(updatePackerSessionId);
                if (sessionItr == mSessionByIdMap.end())
                    continue;

                auto& updateSessionInfo = sessionItr->second;
                auto sessionState = updateSessionInfo.mSessionData->getState();
                if (sessionState == GameManager::PackerSessionData::SESSION_ASSIGNED)
                {
                    auto assignedWorkerInstanceId = updateSessionInfo.mSessionData->getAssignedWorkerInstanceId();
                    EA_ASSERT_MSG(assignedWorkerInstanceId != INVALID_INSTANCE_ID, "Assigned session must have a valid instance!");

                    // PACKER_TODO:  THIS CODE IS BROKEN.  This doesn't work the way the code assumes that it does.   
                    //   The Properties don't change since they were not reacquired from the UserSessions, so the underlying PackerSession is still using the full player list, including the removed user.
                    GameManager::MatchmakingCriteriaError criteriaError;
                    auto rc = initializeAggregateProperties(*updateSessionInfo.mRequestInfo, criteriaError);
                    if (rc != ERR_OK)
                    {
                        ERR_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, Scenario(" << scenarioId << "), PackerSession(" << updatePackerSessionId << ") was updated, failed to reinitialize due to err(" << rc << "), session will be aborted");
                        failedSessionIds.push_back(updatePackerSessionId);
                        continue;
                    }

                    TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, Scenario(" << scenarioId << "), PackerSession(" << updatePackerSessionId << ") was updated, signal assigned worker(" << assignedWorkerInstanceId << ") to relinquish it");

                    auto& sessionUserJoinInfoList = updateSessionInfo.mRequestInfo->getUsersInfo();
                    sessionUserJoinInfoList.clear();
                    for (auto& userJoinInfo : scenarioInfo.mUserJoinInfoList)
                    {
                        // PACKER_TODO: once we remove this info from the session's request we will no longer have to update it this way
                        sessionUserJoinInfoList.push_back(userJoinInfo);
                    }
                    updateSessionInfo.mSessionData->setState(GameManager::PackerSessionData::SESSION_AWAITING_RELEASE);
                    updateSessionInfo.mSessionData->setPendingReleaseTimestamp(TimeValue::getTimeOfDay());

                    updated.setPackerSessionId(updatePackerSessionId);
                    // signal worker to release the session (master will reassign it to a new worker)
                    sendNotifyWorkerPackerSessionRelinquishToInstanceById(assignedWorkerInstanceId, &updated);

                    upsertSessionData(updateSessionInfo);
                    upsertRequestData(updateSessionInfo);
                }
                else
                {
                    TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, Scenario(" << scenarioId << "), PackerSession(" << updatePackerSessionId << ") refreshed, in state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ")");
                }
            }

            for (auto failedPackerSessionId : failedSessionIds)
            {
                auto sessionItr = mSessionByIdMap.find(failedPackerSessionId);
                if (sessionItr == mSessionByIdMap.end())
                    continue;
                auto& failedSessionInfo = sessionItr->second;
                abortSession(failedSessionInfo);
            }

            // PACKER_TODO: Once we move scenarioInfo.mUserJoinInfoList member to PackerScenarioData so it can be backed by storage manager, we'll need to upsert the changed scenario info here...
            //upsertScenarioData(scenarioInfo);
        }

    } while ((scenarioIdByBlazeIdItr = mScenarioIdByBlazeIdMap.find(blazeId)) != mScenarioIdByBlazeIdMap.end());
}

void GamePackerMasterImpl::getStatusInfo(ComponentStatus& status) const
{
    Blaze::ComponentStatus::InfoMap& parentStatusMap = status.getInfo();

    StringBuilder tempNameString;
    StringBuilder tempValueString;
#define ADD_VALUE(name, value) { parentStatusMap[ (tempNameString << name).get() ]= (tempValueString << value).get(); tempNameString.reset(); tempValueString.reset(); }
    for (uint32_t i = 0; i < mLayerCount; ++i)
    {
        auto& layer = mLayerList[i];
        for (int j = 0; j < WorkerInfo::STATE_MAX; ++j)
        {
            if (layer.mWorkerMetricsByState[j] > 0) {
                ADD_VALUE("MMTotal_layer_" << layer.mLayerId << "_workerState_" << j, layer.mWorkerMetricsByState[j]);
            }
        }
    }
#undef ADD_VALUE
}

const char8_t* GamePackerMasterImpl::getWorkerNameByInstanceId(InstanceId instanceId, const char8_t* unkownName) const
{
    auto itr = mWorkerByInstanceId.find(instanceId);
    if (itr != mWorkerByInstanceId.end())
        return itr->second->mWorkerName.c_str();
    return unkownName;
}

const char8_t* GamePackerMasterImpl::getSubSessionNameByPackerSessionId(PackerSessionId packerSessionId, const char8_t* unkownName) const
{
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr != mSessionByIdMap.end())
    {
        return sessionItr->second.getSubSessionName(unkownName);
    }
    return unkownName;
}

StartPackerSessionError::Error GamePackerMasterImpl::processStartPackerSession(const Blaze::GameManager::StartPackerSessionRequest &req, Blaze::GameManager::StartPackerSessionResponse &response, Blaze::GameManager::StartPackerSessionError &error, const ::Blaze::Message* message)
{
    auto* requestTdf = const_cast<EA::TDF::Tdf*>(req.getInternalRequest());
    auto& internalRequest = *static_cast<Blaze::GameManager::StartMatchmakingInternalRequest*>(requestTdf);
    EA_ASSERT(internalRequest.getTdfId() == Blaze::GameManager::StartMatchmakingInternalRequest::TDF_ID);
    auto packerSessionId = req.getPackerSessionId();
    auto sliverId = GetSliverIdFromSliverKey(packerSessionId);
    OwnedSliver* ownedSliver = mScenarioTable.getSliverOwner().getOwnedSliver(sliverId);
    if (ownedSliver == nullptr)
    {
        ASSERT_LOG("[GamePackerMasterImpl].processStartPackerSession: PackerSession(" << packerSessionId << ") could not obtain owned sliver for SliverId(" << sliverId << ")");
        return StartPackerSessionError::ERR_SYSTEM;
    }

    Sliver::AccessRef sliverAccess;
    if (!ownedSliver->getPrioritySliverAccess(sliverAccess))
    {
        ASSERT_LOG("[GamePackerMasterImpl].processStartPackerSession: PackerSession(" << packerSessionId << ") could not get priority sliver access for SliverId(" << ownedSliver->getSliverId() << ")");
        return StartPackerSessionError::ERR_SYSTEM;
    }

    // PACKER_TODO: since this RPC is called by a coreSlave, is this LogContextOverride needed?
    auto& ownerUserSessionInfo = internalRequest.getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

    // Everyone should have the OriginatingScenarioId set, we just grab it from the host. 
    auto originatingScenarioId = GameManager::INVALID_SCENARIO_ID;
    for (auto& userJoinInfo : internalRequest.getUsersInfo())
    {
        if (userJoinInfo->getIsHost())
        {
            originatingScenarioId = userJoinInfo->getOriginatingScenarioId();
        }
    }

    // This also contructs the PropertyFilters' requirement mapped values:
    GameManager::MatchmakingCriteriaError criteriaError;
    auto rc = initializeAggregateProperties(internalRequest, criteriaError);
    if (rc != ERR_OK)
    {
        error.setInternalError(*criteriaError.clone());
        return StartPackerSessionError::commandErrorFromBlazeError(rc);
    }

    auto ret = mSessionByIdMap.insert(packerSessionId);
    if (ret.second)
    {
        ret.first->second.mRequestInfo = &internalRequest; // take ownership of request, much cheaper than copyInto()
        ret.first->second.mSessionData = BLAZE_NEW GameManager::PackerSessionData;
        ret.first->second.mSessionData->setState(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
    }
    else
    {
        ERR_LOG("[GamePackerMasterImpl].processStartPackerSession: unexpected PackerSession(" << packerSessionId << ") collision");

        return StartPackerSessionError::ERR_SYSTEM;
    }

    auto& sessionInfo = ret.first->second;
    auto& requestInfo = *sessionInfo.mRequestInfo;
    auto& sessionData = *sessionInfo.mSessionData;
    sessionData.setStartedTimestamp(TimeValue::getTimeOfDay());
    sessionData.setSessionDuration(requestInfo.getScenarioTimeoutDuration()); // PACKER_TODO: Replace this with scenario duration
    const auto* gmScenarioInfo = sessionInfo.getScenarioInfo();
    auto& scenarioInfo = mScenarioByIdMap[originatingScenarioId];
    bool isNewScenario = scenarioInfo.mScenarioId == 0;
    if (isNewScenario)
    {
        scenarioInfo.mScenarioId = originatingScenarioId;
        scenarioInfo.mScenarioData = BLAZE_NEW GameManager::PackerScenarioData;
        scenarioInfo.mScenarioData->setStartedTimestamp(TimeValue::getTimeOfDay());
        //{
        // PACKER_TODO: We should rewrite this when we ensure that the scenario is constructed in a single request that does not involve sending the user info separately as part of each subsession request
        scenarioInfo.mOwnerUserSessionInfo = requestInfo.getOwnerUserSessionInfo().clone(); // PACKER_TODO: for now we must copy, because scenario info cannot safely point to a non-heap allocated member of its session object, in the future session should not own this, the scenario should own the ownerUserSession object and the sessions should not even have it!
        scenarioInfo.mUserJoinInfoList.reserve(requestInfo.getUsersInfo().size());
        for (auto& userJoinInfo : requestInfo.getUsersInfo())
        {
            BlazeId blazeId = userJoinInfo->getUser().getUserInfo().getId();
            if (blazeId != INVALID_BLAZE_ID)
                mScenarioIdByBlazeIdMap.insert(blazeId)->second = originatingScenarioId;
            else
                WARN_LOG("[GamePackerMasterImpl].processStartPackerSession: PackerSession(" << packerSessionId << ") expected valid BlazeId for Scenario(" << originatingScenarioId << ")");
            scenarioInfo.mUserJoinInfoList.push_back(userJoinInfo);
        }
        //}

        const auto expiryTime = sessionData.getStartedTimestamp() + requestInfo.getScenarioTimeoutDuration();
        const auto expiryTimeWithGracePeriod = getScenarioExpiryWithGracePeriod(expiryTime);
        scenarioInfo.mScenarioData->setExpiresTimestamp(expiryTime); // NOTE: grace period can be reconfigured, snapshot the time without it
        scenarioInfo.mExpiryTimerId = gSelector->scheduleTimerCall(expiryTimeWithGracePeriod, this, &GamePackerMasterImpl::onScenarioExpiredTimeout, originatingScenarioId, "GamePackerMasterImpl::onScenarioExpiredTimeout");
        mScenariosStarted.increment(1, gmScenarioInfo->getScenarioName());
        mScenariosActive.increment(1, gmScenarioInfo->getScenarioName());
    }
    else
    {
        // PACKER_TODO: Robustify this validation, note: we should get rid of it *entirely* when we ensure that the scenario is constructed in a single request that does not involve sending the user info separately as part of each subsession request
        EA_ASSERT(requestInfo.getUsersInfo().size() == scenarioInfo.mUserJoinInfoList.size());
        auto userJoinInfoScenarioItr = scenarioInfo.mUserJoinInfoList.begin();
        for (auto& userJoinInfo : requestInfo.getUsersInfo())
        {
            auto userId = userJoinInfo->getUser().getUserInfo().getId();
            auto scenarioUserId = (*userJoinInfoScenarioItr)->getUser().getUserInfo().getId();
            EA_ASSERT(userId == scenarioUserId);
            ++userJoinInfoScenarioItr;
        }
    }

    mSessionsStarted.increment(1, gmScenarioInfo->getScenarioName(), gmScenarioInfo->getSubSessionName());
    mSessionsActive.increment(1, gmScenarioInfo->getScenarioName(), gmScenarioInfo->getSubSessionName());

    // establish direct back and forth linkage between session and scenario
    scenarioInfo.mSessionByIdSet.insert(packerSessionId);
    sessionInfo.mParentScenario = &scenarioInfo;

    // Get rid of this, and instead create a new object that contains members that we actually need
    GameManager::StartMatchmakingInternalResponsePtr responsePtr = BLAZE_NEW GameManager::StartMatchmakingInternalResponse;
    auto& responseInfo = responsePtr->getStartMatchmakingResponse();
    response.setInternalResponse(*responsePtr);

    scenarioInfo.mScenarioData->setPackerSessionSequence(req.getPackerSessionSequence());

    responseInfo.setSessionId(packerSessionId); // PACKER_TODO: shouldn't need this anymore since we accept a session id generated by the caller, in fact the main reference on the callers side should be the scenario id, but the session id can also be useful...

    upsertRequestData(sessionInfo);
    upsertSessionData(sessionInfo);
    upsertScenarioData(*sessionInfo.mParentScenario);

    const bool finalSubSessionInScenario = req.getPackerSessionSequence() == 0;
    if (finalSubSessionInScenario)
    {
        const auto now = TimeValue::getTimeOfDay();
        // PACKER_TODO: Once we change the packer RPC interface to remove the startSession in favor of startScenario, we won't need the silly sequence number anymore, the whole scenario will be delivered as a single message and constructed and managed atomically.
        TRACE_LOG("[GamePackerMasterImpl].processStartPackerSession: created final PackerSession(" << packerSessionId << "), Scenario(" << originatingScenarioId << "), sequence(0), sessions(" << scenarioInfo.mSessionByIdSet.size() << ") assign sessions to workers");
        for (auto assignSessionId : scenarioInfo.mSessionByIdSet)
        {
            auto sessionItr = mSessionByIdMap.find(assignSessionId);
            if (sessionItr == mSessionByIdMap.end())
            {
                EA_FAIL(); // PACKER_TODO bad news
                scenarioInfo.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_FAILED);
                cleanupScenario(scenarioInfo);
                return StartPackerSessionError::ERR_SYSTEM;
            }
            auto& assignSessionInfo = sessionItr->second;
            auto expiry = assignSessionInfo.mSessionData->getStartedTimestamp() + assignSessionInfo.mRequestInfo->getRequest().getSessionData().getStartDelay();
            if (now < expiry)
            {
                assignSessionInfo.mSessionData->setState(GameManager::PackerSessionData::SESSION_DELAYED);
                upsertSessionData(assignSessionInfo);

                assignSessionInfo.mDelayedStartTimerId = gSelector->scheduleTimerCall(
                    expiry, this, &GamePackerMasterImpl::startDelayedSession, assignSessionId, "GamePackerMasterImpl::startDelayedSession");
                continue;
            }
            mUnassignedSessionList.push_back(assignSessionInfo);
            signalSessionAssignment(LayerInfo::INVALID_LAYER_ID);
        }
    }
    else
    {
        TRACE_LOG("[GamePackerMasterImpl].processStartPackerSession: created interim PackerSession(" << packerSessionId << "), Scenario(" << originatingScenarioId << "), sequence(" << req.getPackerSessionSequence() << "), holding assignment until final session is received");
    }

    return StartPackerSessionError::ERR_OK;
}

void GamePackerMasterImpl::startDelayedSession(PackerSessionId sessionId)
{
    auto sessionItr = mSessionByIdMap.find(sessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].startDelayedSession: PackerSession(" << sessionId << ") has disappeared");
        return;
    }

    auto& sessionInfo = sessionItr->second;

    TRACE_LOG("[GamePackerMasterImpl].startDelayedSession: PackerSession(" << sessionId << "), Scenario(" << sessionInfo.mParentScenario->mScenarioId << ") started after "
        << (TimeValue::getTimeOfDay() - sessionInfo.mSessionData->getStartedTimestamp()).getMillis() << "ms delay (configured for " << sessionInfo.mRequestInfo->getRequest().getSessionData().getStartDelay().getMillis() << "ms)");

    sessionInfo.mDelayedStartTimerId = INVALID_TIMER_ID;
    sessionInfo.mSessionData->setState(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
    mUnassignedSessionList.push_back(sessionInfo);
    signalSessionAssignment(LayerInfo::INVALID_LAYER_ID);

    upsertSessionData(sessionInfo);
}

CancelPackerSessionError::Error GamePackerMasterImpl::processCancelPackerSession(const Blaze::GameManager::CancelPackerSessionRequest &request, const ::Blaze::Message* message)
{
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr != mSessionByIdMap.end())
    {
        auto& sessionInfo = sessionItr->second;
        // PACKER_TODO: since this RPC is called by a coreSlave, is this LogContextOverride needed?
        auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
        LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

        auto& scenarioInfo = *sessionInfo.mParentScenario;
        auto assignedInstanceId = sessionInfo.mSessionData->getAssignedWorkerInstanceId();
        auto* printableWorkerName = getWorkerNameByInstanceId(assignedInstanceId);
        if (scenarioInfo.mScenarioData->getLockedForFinalizationByPackerSessionId() != INVALID_PACKER_SESSION_ID)
        {
            // PACKER_TODO: Some weird stuff we have to debug here, session seems to be getting canceled by the core slave when it itself is still finalizing. We need to print out session duration, finalization duration(locked), also we need to figure out why other bad stuff is happening, seems like we might have some problems with some ids, or notifications or something, stress clients are not behaving well right now...

            WARN_LOG("[GamePackerMasterImpl].processCancelPackerSession: PackerSession(" << packerSessionId << ") assigned to worker(" << printableWorkerName << "), sessionData(" << *sessionInfo.mSessionData << ") cannot be canceled while locked for finalization by(" << scenarioInfo.mScenarioData->getLockedForFinalizationByPackerSessionId() << ") before completion");

            // PACKER_TODO: can't cancel a locked session, it can expire however, we may have to adjust a grace period for that and and setup a lock timer...
            return CancelPackerSessionError::GAMEPACKER_ERR_SCENARIO_ALREADY_FINALIZING;
        }

        if (assignedInstanceId != INVALID_INSTANCE_ID)
        {
            TRACE_LOG("[GamePackerMasterImpl].processCancelPackerSession: PackerSession(" << packerSessionId << ") assigned to worker(" << printableWorkerName << "), sessionData(" << *sessionInfo.mSessionData << ") canceled before completion");
        }

        // PACKER_TODO: figure out if canceling a session(in the future, canceling the scenario) should generate a notification still
        {
            auto now = TimeValue::getTimeOfDay();
            auto duration = now - sessionInfo.mSessionData->getStartedTimestamp();
            auto* hostJoinInfo = sessionInfo.getHostJoinInfo();
            auto hostUserSessionId = hostJoinInfo->getUser().getSessionId();
            auto originatingScenarioId = hostJoinInfo->getOriginatingScenarioId();
            GameManager::NotifyMatchmakingFailed notifyFailed;
            notifyFailed.setUserSessionId(hostUserSessionId);
            notifyFailed.setSessionId(packerSessionId);
            notifyFailed.setScenarioId(originatingScenarioId);
            notifyFailed.setMatchmakingResult(GameManager::SESSION_CANCELED);
            notifyFailed.setMaxPossibleFitScore(100);
            notifyFailed.getSessionDuration().setSeconds(duration.getSec());
            sendNotifyMatchmakingFailedToSliver(UserSession::makeSliverIdentity(hostUserSessionId), &notifyFailed);
        }

        scenarioInfo.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_CANCELED);
        cleanupScenario(scenarioInfo); // PACKER_TODO: first cancel of subsession currently always cancels the whole scenario, because coreslave always tries to cancel the rest afterwards and that is the intent anyway, and those are all no-ops. In the future this RPC will be renamed cancelScenario and called only once!
    }
    return CancelPackerSessionError::ERR_OK;
}

WorkerObtainPackerSessionError::Error GamePackerMasterImpl::processWorkerObtainPackerSession(const Blaze::GameManager::WorkerObtainPackerSessionRequest &request, Blaze::GameManager::WorkerObtainPackerSessionResponse &response, const ::Blaze::Message* message)
{
    const auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerObtainPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerObtainPackerSessionError::ERR_SYSTEM;
    }

    auto& worker = *workerItr->second;
    auto& layer = getLayerByWorker(worker);
    auto& fromList = layer.mSessionListByState[WorkerInfo::IDLE].empty() ? mUnassignedSessionList : layer.mSessionListByState[WorkerInfo::IDLE];

    if (fromList.empty())
    {
        setWorkerState(worker, WorkerInfo::IDLE);
    }
    else
    {
        // Update the State of the session and worker:  (to WORKING)
        auto& sessionInfo = fromList.front();
        fromList.pop_front();
        assignWorkerSession(worker, sessionInfo);


        // Update the Session Data (to be sent in response & Redis)
        auto& sessionData = *sessionInfo.mSessionData;
        // session has been assigned to worker
        sessionData.setState(GameManager::PackerSessionData::SESSION_ASSIGNED);
        sessionData.setAssignedWorkerInstanceId(worker.mWorkerInstanceId);
        sessionData.setAssignedLayerId(worker.mWorkerLayerId);
        sessionData.setAcquiredTimestamp(TimeValue::getTimeOfDay());
        if (sessionData.getReleaseCount() == 0 && sessionData.getAcquiredTimestamp() > sessionData.getStartedTimestamp())
        {
            sessionData.setUnassignedDuration(sessionData.getUnassignedDuration() + sessionData.getAcquiredTimestamp() - sessionData.getStartedTimestamp());
        }
        else if (sessionData.getReleaseCount() > 0 && sessionData.getAcquiredTimestamp() > sessionData.getReleasedTimestamp())
        {
            sessionData.setUnassignedDuration(sessionData.getUnassignedDuration() + sessionData.getAcquiredTimestamp() - sessionData.getReleasedTimestamp());
        }


        // Update the Response:
        auto packerSessionId = sessionInfo.mRequestInfo->getMatchmakingSessionId();
        response.setInternalRequest(*sessionInfo.mRequestInfo);
        response.setExpiryDeadline(sessionData.getStartedTimestamp() + sessionData.getSessionDuration());
        response.setPackerSessionId(packerSessionId);
        response.setVersion(0); // PACKER_TODO: add a version of the session that we can use for efficient resync in the future


        // Update Redis:
        auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
        LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());
        upsertSessionData(sessionInfo);
    }
    
    return WorkerObtainPackerSessionError::ERR_OK;
}

WorkerMigratePackerSessionError::Error GamePackerMasterImpl::processWorkerMigratePackerSession(const Blaze::GameManager::WorkerMigratePackerSessionRequest &request, const ::Blaze::Message* message)
{
    // When the session is released, the worker slave has given up, and the session needs to be propagated to the next layer
    // The propagation is always going towards layer 0 which is the root layer that must only have a single worker assigned.
    // The layer hierarchy is a binary heap whereby the number of nodes on successive layers is always x2 the number of nodes on the previous layers.
    const auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerMigratePackerSessionError::ERR_SYSTEM;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to migrate unknown PackerSession(" << packerSessionId << ")");
        return WorkerMigratePackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& sessionInfo = sessionItr->second;
    auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

    auto& sessionData = *sessionInfo.mSessionData;
    auto sessionState = sessionData.getState();
    const auto assignedInstanceId = sessionInfo.mSessionData->getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to migrate PackerSession(" << packerSessionId << ") in state(" << packerSessionId << ") currently assigned to different worker(" <<  getWorkerNameByInstanceId(assignedInstanceId) << "), request ignored");
        return WorkerMigratePackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    bool validState = (sessionState == GameManager::PackerSessionData::SESSION_ASSIGNED) || (sessionState == GameManager::PackerSessionData::SESSION_AWAITING_RELEASE);
    if (!validState)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to migrate PackerSession(" << packerSessionId << ") in unexpected state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ") on layer(" << sessionData.getAssignedLayerId() << "), request ignored");
        return WorkerMigratePackerSessionError::ERR_SYSTEM;
    }

    auto lockedForFinalizationBySessionId = sessionInfo.mParentScenario->mScenarioData->getLockedForFinalizationByPackerSessionId();
    if (lockedForFinalizationBySessionId != INVALID_PACKER_SESSION_ID)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to migrate PackerSession(" << packerSessionId << ") while locked for finalization by PackerSession(" << lockedForFinalizationBySessionId << ") on layer(" << sessionData.getAssignedLayerId() << "), request ignored");
        return WorkerMigratePackerSessionError::ERR_SYSTEM;
    }

    auto now = TimeValue::getTimeOfDay();
    auto assignedLayerId = sessionData.getAssignedLayerId();
    if (assignedLayerId > 1)
    {
        // session has been un-assigned from current worker, we'll need to re-assign it out
        sessionData.setAssignedWorkerInstanceId(INVALID_INSTANCE_ID);
        sessionData.setReleaseCount(sessionData.getReleaseCount() + 1);
        sessionData.setReleasedTimestamp(now);
        sessionData.setState(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
        sessionData.setAssignedLayerId(assignedLayerId - 1); // Assign session to next layer

        INFO_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") released PackerSession(" << packerSessionId << "), session remaining lifetime(" << sessionInfo.getRemainingLifetime(now).getMillis() << "ms) on layer(" << assignedLayerId << "), will migrate to layer(" << sessionData.getAssignedLayerId() << ")"); // PACKER_TODO: change to trace

        assignSession(sessionInfo); // takes care of calling upsertSessionData()
        return WorkerMigratePackerSessionError::ERR_OK;
    }
    else
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to release PackerSession(" << packerSessionId << "), session remaining lifetime(" << sessionInfo.getRemainingLifetime(now).getMillis() << "ms) on layer(" << assignedLayerId << "), root layer not allowed to migrate sessions, session aborted");

        abortSession(sessionInfo);
        return WorkerMigratePackerSessionError::ERR_SYSTEM;
    }
}

WorkerRelinquishPackerSessionError::Error GamePackerMasterImpl::processWorkerRelinquishPackerSession(const Blaze::GameManager::WorkerRelinquishPackerSessionRequest &request, const ::Blaze::Message* message)
{
    // When the session is released, release was requested due to owner triggered update and the session should remain on the same layer.
    // The propagation is always going towards layer 0 which is the root layer that must only have a single worker assigned.
    // The layer hierarchy is a binary heap whereby the number of nodes on successive layers is always x2 the number of nodes on the previous layers.
    const auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerRelinquishPackerSessionError::ERR_SYSTEM;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: worker(" << worker.mWorkerName << ") attempted to release unknown PackerSession(" << packerSessionId << ")");
        return WorkerRelinquishPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& sessionInfo = sessionItr->second;
    auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

    auto& sessionData = *sessionInfo.mSessionData;
    auto sessionState = sessionData.getState();
    const auto assignedInstanceId = sessionData.getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: worker(" << worker.mWorkerName << ") attempted to release PackerSession(" << packerSessionId << ") in state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ") currently assigned to different worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerRelinquishPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto now = TimeValue::getTimeOfDay();
    if (sessionState == GameManager::PackerSessionData::SESSION_AWAITING_RELEASE)
    {
        auto assignedLayerId = sessionData.getAssignedLayerId();
        // session has been un-assigned from current worker, we'll need to re-assign it out
        sessionData.setState(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
        sessionData.setAssignedWorkerInstanceId(INVALID_INSTANCE_ID);
        sessionData.setReleaseCount(sessionData.getReleaseCount() + 1);
        sessionData.setReleasedTimestamp(now);

        INFO_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: worker(" << worker.mWorkerName << ") released PackerSession(" << packerSessionId << ") due to owner triggered update, remaining lifetime(" << sessionInfo.getRemainingLifetime(now).getMillis() << "ms) on layer(" << assignedLayerId << "), will reassign to worker on same layer"); // TODO: change to trace

        assignSession(sessionInfo); // takes care of calling upsertSessionData()
        
        return WorkerRelinquishPackerSessionError::ERR_OK;
    }
    else
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: worker(" << worker.mWorkerName << ") attempted to release PackerSession(" << packerSessionId << ") in unexpected state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ") on layer(" << sessionData.getAssignedLayerId() << "), ignored");
        return WorkerRelinquishPackerSessionError::ERR_SYSTEM;
    }
}

WorkerUpdatePackerSessionStatusError::Error GamePackerMasterImpl::processWorkerUpdatePackerSessionStatus(const Blaze::GameManager::WorkerUpdatePackerSessionStatusRequest &request, const ::Blaze::Message* message)
{
    const auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerUpdatePackerSessionStatus: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerUpdatePackerSessionStatusError::ERR_SYSTEM;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        return WorkerUpdatePackerSessionStatusError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& sessionInfo = sessionItr->second;
    auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

    auto lockedForFinalizationByPackerSessionId = sessionInfo.mParentScenario->mScenarioData->getLockedForFinalizationByPackerSessionId();
    auto lockedForFinalization = (lockedForFinalizationByPackerSessionId != INVALID_PACKER_SESSION_ID) && (lockedForFinalizationByPackerSessionId != packerSessionId);
    auto now = TimeValue::getTimeOfDay();
    if (lockedForFinalization)
    {
        auto siblingSessionItr = mSessionByIdMap.find(lockedForFinalizationByPackerSessionId);
        if (siblingSessionItr == mSessionByIdMap.end())
        {
            ERR_LOG("[GamePackerMasterImpl].processWorkerUpdatePackerSessionStatus: worker(" << worker.mWorkerName << ") failed to update PackerSession(" << packerSessionId << ":" << sessionInfo.getSubSessionName() << ") with game score(" << request.getGameQualityScore() << ") because sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ") locking the parent Scenario(" << sessionInfo.mParentScenario->mScenarioId << ") for finalization was not found.");
            return WorkerUpdatePackerSessionStatusError::ERR_SYSTEM;
        }
        TRACE_LOG("[GamePackerMasterImpl].processWorkerUpdatePackerSessionStatus: worker(" << worker.mWorkerName << ") attempt to update PackerSession(" << packerSessionId << ":" << sessionInfo.getSubSessionName() << ") with game score(" << request.getGameQualityScore() << ") while parent Scenario(" << sessionInfo.mParentScenario->mScenarioId << ") is locked for finalization by sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << siblingSessionItr->second.getSubSessionName() << ") with game score(" << siblingSessionItr->second.mSessionData->getGameQualityScore() << "), ignoring update");
    }
    else
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerUpdatePackerSessionStatus: worker(" << worker.mWorkerName << ") updated PackerSession(" << packerSessionId << ":" << sessionInfo.getSubSessionName() << ") game score(" << sessionInfo.mSessionData->getGameQualityScore() << ") -> score(" << request.getGameQualityScore() << ")");

        auto* hostJoinInfo = sessionInfo.getHostJoinInfo();
        EA_ASSERT(hostJoinInfo != nullptr);
        auto hostUserSessionId = hostJoinInfo->getUser().getSessionId();
        auto originatingScenarioId = hostJoinInfo->getOriginatingScenarioId();

        GameManager::NotifyMatchmakingAsyncStatus notifyMatchmakingStatus;
        notifyMatchmakingStatus.setInstanceId(gController->getInstanceId());
        notifyMatchmakingStatus.setMatchmakingSessionId(request.getPackerSessionId());
        notifyMatchmakingStatus.setMatchmakingScenarioId(originatingScenarioId);

        notifyMatchmakingStatus.setUserSessionId(hostUserSessionId);
        auto age = (now - sessionInfo.mSessionData->getStartedTimestamp()); // PACKER_TODO: Round to nearest second
        notifyMatchmakingStatus.setMatchmakingSessionAge((uint32_t) age.getSec());
        auto& packerAsyncStatus = notifyMatchmakingStatus.getPackerStatus();
        request.getGameFactorScores().copyInto(packerAsyncStatus.getQualityFactorScores());
        packerAsyncStatus.setOverallQualityFactorScore(request.getGameQualityScore() * 100.0f);
        auto sliverIdentity = UserSession::makeSliverIdentity(hostUserSessionId);
        // PACKER_MAYBE: we may be able to get away with fewer dependencies on MM header files if we rely only on the generated lower level stubs that define the actual types.
        //sendNotificationToSliver(Matchmaker::MatchmakerSlave::NOTIFICATION_INFO_NOTIFYMATCHMAKINGASYNCSTATUS, sliverIdentity, &notifyMatchmakingStatus, ::Blaze::NotificationSendOpts());
        sendNotifyMatchmakingAsyncStatusToSliver(sliverIdentity, &notifyMatchmakingStatus);
    }

    // PACKER_TODO: store the last time the score was updated
    sessionInfo.mSessionData->setUpdatedStatusTimestamp(now);
    if (sessionInfo.mSessionData->getGameQualityScore() < request.getGameQualityScore())
    {
        sessionInfo.mSessionData->setImprovedScoreTimestamp(now);
    }
    sessionInfo.mSessionData->setGameQualityScore(request.getGameQualityScore());
    request.getGameFactorScores().copyInto(sessionInfo.mSessionData->getGameFactorScores());

    upsertSessionData(sessionInfo);

    return WorkerUpdatePackerSessionStatusError::ERR_OK;
}

WorkerAbortPackerSessionError::Error GamePackerMasterImpl::processWorkerAbortPackerSession(const Blaze::GameManager::WorkerAbortPackerSessionRequest &request, const ::Blaze::Message* message)
{
    const auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerAbortPackerSessionError::ERR_SYSTEM;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: worker(" << worker.mWorkerName << ") attempted to abort non-existent PackerSession(" << packerSessionId << ")");
        return WorkerAbortPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND; // PACKER_TODO: figure out if we need to return ERR_OK if benign...
    }

    auto& sessionInfo = sessionItr->second;
    auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

    const auto assignedInstanceId = sessionInfo.mSessionData->getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: worker(" << worker.mWorkerName << ") attempted to abort PackerSession(" << packerSessionId << ") which is currently assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerAbortPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto lockedForFinalizationByPackerSessionId = sessionInfo.mParentScenario->mScenarioData->getLockedForFinalizationByPackerSessionId();
    if ((lockedForFinalizationByPackerSessionId != INVALID_PACKER_SESSION_ID) && 
        (lockedForFinalizationByPackerSessionId != packerSessionId))
    {
        // Worker should only be attempting to release sessions that it locked itself
        ERR_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: worker(" << worker.mWorkerName << ") attempted to abort PackerSession(" << packerSessionId << ":" << sessionInfo.getSubSessionName() << ") while Scenario(" << sessionInfo.mParentScenario->mScenarioId << ") is currently locked for finalization by sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << getSubSessionNameByPackerSessionId(lockedForFinalizationByPackerSessionId) << ")");
        return WorkerAbortPackerSessionError::ERR_SYSTEM;
    }

    WARN_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: worker(" << worker.mWorkerName << ") aborted PackerSession(" << packerSessionId << "), sessionData(" << *sessionInfo.mSessionData << "), requestInfo(" << *sessionInfo.mRequestInfo << ")");

    abortSession(sessionInfo);

    return WorkerAbortPackerSessionError::ERR_OK;
}

// IDEA: we can change the protocol to ensure that the verbs the worker slave can perform are: acquire/release/fail/complete
WorkerCompletePackerSessionError::Error GamePackerMasterImpl::processWorkerCompletePackerSession(const Blaze::GameManager::WorkerCompletePackerSessionRequest &request, const ::Blaze::Message* message)
{
    const auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerCompletePackerSessionError::ERR_SYSTEM;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") attempted to complete non-existent PackerSession(" << packerSessionId << ")");
        return WorkerCompletePackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND; // PACKER_TODO: figure out if we need to return ERR_OK if benign...
    }

    auto& sessionInfo = sessionItr->second;
    auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

    auto& requestInfo = *sessionInfo.mRequestInfo;
    const auto assignedInstanceId = sessionInfo.mSessionData->getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") attempted to complete PackerSession(" << packerSessionId << ") which is currently assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerCompletePackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto lockedForFinalizationByPackerSessionId = sessionInfo.mParentScenario->mScenarioData->getLockedForFinalizationByPackerSessionId();
    if ((lockedForFinalizationByPackerSessionId != INVALID_PACKER_SESSION_ID) &&
        (lockedForFinalizationByPackerSessionId != packerSessionId))
    {
        // Worker should only be attempting to release sessions that it locked itself
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") attempted to complete PackerSession(" << packerSessionId << ":" << sessionInfo.getSubSessionName() << ") while Scenario(" << sessionInfo.mParentScenario->mScenarioId << ") is currently locked for finalization by sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << getSubSessionNameByPackerSessionId(lockedForFinalizationByPackerSessionId) << ")");
        return WorkerCompletePackerSessionError::ERR_SYSTEM;
    }

    if (request.getGameId() == GameManager::INVALID_GAME_ID)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") attempted to complete PackerSession(" << packerSessionId << ") with INVALID_GAME_ID");
        return WorkerCompletePackerSessionError::ERR_SYSTEM;
    }

    auto* hostJoinInfo = sessionInfo.getHostJoinInfo();
    EA_ASSERT(hostJoinInfo != nullptr);
    auto hostUserSessionId = hostJoinInfo->getUser().getSessionId(); // NOTE: Host and owner session *may* not be the same
    auto originatingScenarioId = hostJoinInfo->getOriginatingScenarioId();
    auto completionResult = (packerSessionId == request.getInitiatorPackerSessionId()) ? GameManager::SUCCESS_CREATED_GAME : GameManager::SUCCESS_JOINED_NEW_GAME;

    TRACE_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") completed PackerSession(" << packerSessionId << "), Scenario(" << originatingScenarioId << ") with result(" << GameManager::MatchmakingResultToString(completionResult) << "), game(" << request.getGameId() << ")");

    const auto networkTopology = requestInfo.getRequest().getGameCreationData().getNetworkTopology();
    GameManager::NotifyMatchmakingSessionConnectionValidated notifyConnValidated;
    bool qosValidationPerformed = false;
    if (!GameManager::bypassNetworkTopologyQosValidation(networkTopology))
    {
        // PACKER_TODO: For now qos validation is not supported for packer based matchmaking,
        // determining whether we plan to support Blaze server-side QoS validation is an open question at this point.
        // qosValidationPerformed = ...;
    }
    notifyConnValidated.setGameId(request.getGameId());
    notifyConnValidated.setDispatchSessionFinished(true);
    notifyConnValidated.setSessionId(packerSessionId);
    notifyConnValidated.setScenarioId(originatingScenarioId);
    notifyConnValidated.setQosValidationPerformed(qosValidationPerformed);
    notifyConnValidated.getConnectionValidatedResults().setNetworkTopology(networkTopology);

    UserIdentificationList externalReservedUsers;
    for (auto& userJoinInfo : requestInfo.getUsersInfo())
    {
        auto& userSessionInfo = userJoinInfo->getUser();
        if (userJoinInfo->getIsOptional())
        {
            if (userSessionInfo.getHasExternalSessionJoinPermission())
            {
                UserInfo::filloutUserIdentification(userSessionInfo.getUserInfo(), *externalReservedUsers.pull_back());
            }
        }
        else
        {
            // NOTE: Currently this notification is required for the scenario manager (on core slave) to correctly clean up a successful matchmaking scenario.
            // This is irrespective of whether the network topology actually requires QoS validation, and can be thought of as a 'connection validated, or no validation was required' notification.
            auto userSessionId = userSessionInfo.getSessionId();
            notifyConnValidated.setUserSessionId(userSessionId);
            sendNotifyMatchmakingSessionConnectionValidatedToSliver(UserSession::makeSliverIdentity(userSessionId), &notifyConnValidated);
        }
    }

    if (!externalReservedUsers.empty())
    {
        GameManager::NotifyServerMatchmakingReservedExternalPlayers notifyExternalServer;
        notifyExternalServer.setUserSessionId(hostUserSessionId);
        auto& notifyExternalClient = notifyExternalServer.getClientNotification();
        notifyExternalClient.setGameId(request.getGameId());
        notifyExternalClient.getJoinedReservedPlayerIdentifications().swap(externalReservedUsers);
        sendNotifyServerMatchmakingReservedExternalPlayersToSliver(UserSession::makeSliverIdentity(hostUserSessionId), &notifyExternalServer);
    }
    
    {
        GameManager::NotifyMatchmakingFinished notifyFinished;
        notifyFinished.setUserSessionId(hostUserSessionId);
        notifyFinished.setSessionId(packerSessionId);
        notifyFinished.setScenarioId(originatingScenarioId);
        notifyFinished.setMatchmakingResult(completionResult);
        sendNotifyMatchmakingFinishedToSliver(UserSession::makeSliverIdentity(hostUserSessionId), &notifyFinished);
    }

    {
        // submit matchmaking succeeded event, this will catch the game creator and all joining sessions.
        GameManager::SuccesfulMatchmakingSession succesfulMatchmakingSession;
        //succesfulMatchmakingSession.setMatchmakingSessionId(packerSessionId); /// @deprecated intentionally omitted and should be removed with legacy MM
        succesfulMatchmakingSession.setMatchmakingScenarioId(originatingScenarioId);
        succesfulMatchmakingSession.setUserSessionId(ownerUserSessionInfo.getSessionId());
        succesfulMatchmakingSession.setPersonaName(ownerUserSessionInfo.getUserInfo().getPersonaName());
        succesfulMatchmakingSession.setPersonaNamespace(ownerUserSessionInfo.getUserInfo().getPersonaNamespace());
        succesfulMatchmakingSession.setMatchmakingResult(MatchmakingResultToString(completionResult));
        succesfulMatchmakingSession.setFitScore((uint32_t)roundf(request.getGameQualityScore() * 100.0f));
        succesfulMatchmakingSession.setMaxPossibleFitScore(100);
        succesfulMatchmakingSession.setGameId(request.getGameId());
        gEventManager->submitEvent((uint32_t)GameManager::GameManagerMaster::EVENT_SUCCESFULMATCHMAKINGSESSIONEVENT, succesfulMatchmakingSession);
    }

    const auto now = TimeValue::getTimeOfDay();
    const auto duration = now - sessionInfo.mParentScenario->mScenarioData->getStartedTimestamp();

    auto* incomingPinSubmission = static_cast<const Blaze::PINSubmission*>(request.getPinSubmission());
    if (incomingPinSubmission != nullptr)
    {
        auto tdfId = incomingPinSubmission->getTdfId();
        if (tdfId == Blaze::PINSubmission::TDF_ID)
        {
            // PACKER_MAYBE: Determine if the comment below is correct, if we fixed Logger::logPINEvent(), then we wouldn't need to allocate a new 
            // PINSubmission or copyInto or use 'onlyIfNoDefault' setting below and instead would simply simply const_cast() the incomingPinSubmission 
            // and modify it in place, which would be much more efficient. NOTE: this code is taken from matchmakingjob.cpp, fix it there as well if it
            // is found to be unnecessary.
            // {{
            // expressly setting the visit options is to workaround an issue in the heat2encoder/decoder around change tracking.
            EA::TDF::MemberVisitOptions visitOptions;
            visitOptions.onlyIfNotDefault = true;
            PINSubmissionPtr outgoingPinSubmission = BLAZE_NEW PINSubmission;
            incomingPinSubmission->copyInto(*outgoingPinSubmission, visitOptions);
            //}}

            // Update any completion events to note the real time it took to complete:
            auto pinEventsMapItr = outgoingPinSubmission->getEventsMap().find(ownerUserSessionInfo.getSessionId());
            if (pinEventsMapItr != outgoingPinSubmission->getEventsMap().end())
            {
                for (auto& pinEvent : *pinEventsMapItr->second)
                {
                    // Update the match joined events for this game
                    if (pinEvent->get()->getTdfId() != RiverPoster::MPMatchJoinEvent::TDF_ID)
                        continue;

                    auto* matchJoinEvent = static_cast<RiverPoster::MPMatchJoinEvent*>(pinEvent->get());

                    if (requestInfo.getTrackingTag()[0] != '\0')
                        matchJoinEvent->setTrackingTag(requestInfo.getTrackingTag());
                    // PACKER_TODO: for mp_match_join, how to obtain other values (est. time to match, etc.)
                    matchJoinEvent->setMatchmakingDurationSeconds(duration.getSec());
                    //matchJoinEvent->setEstimatedTimeToMatchSeconds(session->getEstimatedTimeToMatch().getSec());
                    matchJoinEvent->setTotalUsersOnline(requestInfo.getTotalUsersOnline());
                    matchJoinEvent->setTotalUsersInGame(requestInfo.getTotalUsersInGame());
                    matchJoinEvent->setTotalUsersInMatchmaking(requestInfo.getTotalUsersInMatchmaking());
                }
            }

            gUserSessionManager->sendPINEvents(outgoingPinSubmission);
        }
        else
        {
            ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") completed PackerSession(" << packerSessionId << "), Scenario(" << originatingScenarioId << "), incoming PIN event has unexpected TdfId(" << tdfId << ")");
        }
    }
    else
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") completed PackerSession(" << packerSessionId << "), Scenario(" << originatingScenarioId << "), incoming PIN event missing");
    }

    sessionInfo.mSessionData->setAssignedWorkerInstanceId(INVALID_INSTANCE_ID); // clear instance id assignment to avoid sending removal notification to the initiating worker instance
    sessionInfo.mSessionData->setReleasedTimestamp(now);
    sessionInfo.mSessionData->setReleaseCount(sessionInfo.mSessionData->getReleaseCount() + 1);
    sessionInfo.mSessionData->setState(GameManager::PackerSessionData::SESSION_COMPLETED);
    sessionInfo.mParentScenario->mScenarioData->setLockedForFinalizationByPackerSessionId(INVALID_PACKER_SESSION_ID);
    sessionInfo.mParentScenario->cancelExpiryTimer();
    sessionInfo.mParentScenario->cancelFinalizationTimer();

    TRACE_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") completed PackerSession(" << packerSessionId << "), Scenario(" << originatingScenarioId << ") finished, total_duration(" << duration.getMillis() << "ms), finalization_duration(" << (now - sessionInfo.mParentScenario->mScenarioData->getLockedTimestamp()).getMillis() << "ms)");

    sessionInfo.mParentScenario->mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_COMPLETED);
    cleanupScenario(*sessionInfo.mParentScenario);

    return WorkerCompletePackerSessionError::ERR_OK;
}

Blaze::GamePacker::WorkerLockPackerSessionError::Error GamePackerMasterImpl::processWorkerLockPackerSession(const Blaze::GameManager::WorkerLockPackerSessionRequest &request, const ::Blaze::Message* message)
{
    // NOTE: Since all our sessions for a given scenario id are mapped to a specific sliver, we are guaranteed that all sessions for a given scenario will be on the same master instance.

    auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerLockPackerSessionError::ERR_SYSTEM;
    }
    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        return WorkerLockPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& sessionInfo = sessionItr->second;
    auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

    const auto assignedInstanceId = sessionInfo.mSessionData->getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: worker(" << worker.mWorkerName << ") attempted to lock PackerSession(" << packerSessionId << ") currently assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerLockPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto lockedForFinalizationByPackerSessionId = sessionInfo.mParentScenario->mScenarioData->getLockedForFinalizationByPackerSessionId();
    auto now = TimeValue::getTimeOfDay();
    if (lockedForFinalizationByPackerSessionId == INVALID_PACKER_SESSION_ID)
    {
        EA_ASSERT(sessionInfo.mParentScenario->mFinalizationTimerId == INVALID_TIMER_ID);
        auto& gameTemplatesMap = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates();
        auto gameTemplateItr = gameTemplatesMap.find(sessionInfo.mRequestInfo->getCreateGameTemplateName());
        EA_ASSERT(gameTemplateItr != gameTemplatesMap.end()); // PACKER_TODO: ensure we always validate game template info associated with each packer session, altertnative it may even be good to just link it directly via tdf_ptr...
        const auto& packerConfig = gameTemplateItr->second->getPackerConfig();
        const auto finalizationDuration = eastl::max_alt(packerConfig.getPauseDurationAfterAcquireFinalizationLocks(), packerConfig.getFinalizationDuration());
        const auto finalizationDeadline = now + finalizationDuration; // PACKER_TODO: Need to disable scenario timeout when finalization will exceed it to avoid aborting finalization too early.
        sessionInfo.mParentScenario->mScenarioData->setLockedForFinalizationByPackerSessionId(packerSessionId);
        sessionInfo.mParentScenario->mScenarioData->setLockedTimestamp(now);
        sessionInfo.mParentScenario->mFinalizationTimerId = gSelector->scheduleTimerCall(finalizationDeadline, this, &GamePackerMasterImpl::onScenarioFinalizationTimeout, sessionInfo.mParentScenario->mScenarioId, "GamePackerMasterImpl::onScenarioFinalizationTimeout");

        upsertScenarioData(*sessionInfo.mParentScenario);

        return GamePacker::WorkerLockPackerSessionError::ERR_OK;
    }
    else if (lockedForFinalizationByPackerSessionId == packerSessionId)
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: PackerSession(" << packerSessionId << ":" << sessionInfo.getSubSessionName() << ") on worker(" << worker.mWorkerName << ") attempted to lock Scenario(" << sessionInfo.mParentScenario->mScenarioId << ") already locked by this session for(" << (now - sessionInfo.mParentScenario->mScenarioData->getLockedTimestamp()).getMillis() << "ms), no-op");
        return GamePacker::WorkerLockPackerSessionError::ERR_OK;
    }
    else
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: PackerSession(" << packerSessionId << ":" << sessionInfo.getSubSessionName() << ") on worker(" << worker.mWorkerName << ") attempted to lock Scenario(" << sessionInfo.mParentScenario->mScenarioId << ") currently locked by sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << getSubSessionNameByPackerSessionId(lockedForFinalizationByPackerSessionId) << ") on worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ") for(" << (now - sessionInfo.mParentScenario->mScenarioData->getLockedTimestamp()).getMillis() << "ms), PackerSession(" << packerSessionId << ") will be suspended");

        EA_ASSERT(sessionInfo.mParentScenario->mFinalizationTimerId != INVALID_TIMER_ID);

        sessionInfo.mSessionData->setAssignedWorkerInstanceId(INVALID_INSTANCE_ID);
        sessionInfo.mSessionData->setReleasedTimestamp(now);
        sessionInfo.mSessionData->setReleaseCount(sessionInfo.mSessionData->getReleaseCount() + 1);
        sessionInfo.mSessionData->setState(GameManager::PackerSessionData::SESSION_SUSPENDED);

        upsertSessionData(sessionInfo);
        return WorkerLockPackerSessionError::GAMEPACKER_ERR_SCENARIO_ALREADY_FINALIZING;
    }
}

Blaze::GamePacker::WorkerUnlockPackerSessionError::Error GamePackerMasterImpl::processWorkerUnlockPackerSession(const Blaze::GameManager::WorkerUnlockPackerSessionRequest &request, const ::Blaze::Message* message)
{
    auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerUnlockPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerUnlockPackerSessionError::ERR_SYSTEM;
    }
    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        return WorkerUnlockPackerSessionError::ERR_SYSTEM; // PACKER_TODO: figure out if we need to return ERR_NOT_FOUND
    }

    auto& sessionInfo = sessionItr->second;
    auto& ownerUserSessionInfo = sessionInfo.mRequestInfo->getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserSessionInfo.getSessionId());

    const auto assignedInstanceId = sessionInfo.mSessionData->getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: worker(" << worker.mWorkerName << ") attempted to unlock PackerSession(" << packerSessionId << ") currently assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerUnlockPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto lockedForFinalizationByPackerSessionId = sessionInfo.mParentScenario->mScenarioData->getLockedForFinalizationByPackerSessionId();
    if (lockedForFinalizationByPackerSessionId != packerSessionId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: PackerSession(" << packerSessionId << ":" << sessionInfo.getSubSessionName() << ") on worker(" << worker.mWorkerName << ") attempted to unlock Scenario(" << sessionInfo.mParentScenario->mScenarioId << ") currently locked by PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << getSubSessionNameByPackerSessionId(lockedForFinalizationByPackerSessionId) << ") on worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerUnlockPackerSessionError::ERR_SYSTEM;
    }
    
    sessionInfo.mParentScenario->mScenarioData->setLockedForFinalizationByPackerSessionId(INVALID_PACKER_SESSION_ID);
    sessionInfo.mParentScenario->cancelFinalizationTimer();

    upsertScenarioData(*sessionInfo.mParentScenario);

    return GamePacker::WorkerUnlockPackerSessionError::ERR_OK;
}

WorkerClaimLayerAssignmentError::Error GamePackerMasterImpl::processWorkerClaimLayerAssignment(const Blaze::GameManager::WorkerLayerAssignmentRequest &request, Blaze::GameManager::WorkerLayerAssignmentResponse &response, const ::Blaze::Message* message)
{
    // NOTE: whenever a slave leaves, we must update all the other slaves,
    // the least 'churny' way to do this is to backfill the 'holes' in the topology 
    // by borrowing from the layer with the most instances to fill the holes in the layer with the least,
    // then when instances show up again, they will replace the recently vacated elements. 
    // Imagine we have a strict heap hierarchy. 1, 2, 4, 8, 16...
    const auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    const char8_t* workerName = request.getInstanceName();
    const auto slotId = request.getLayerSlotId();
    if (slotId > 0 && slotId <= LayerInfo::MAX_SLOTS)
    {
        const auto slotIndex = LayerInfo::slotIndexFromSlotId(slotId);
        if (mLayerSlotVersionList.size() < slotIndex + 1)
            mLayerSlotVersionList.resize(slotIndex + 1);
        const auto assignedSlotVersion = mLayerSlotVersionList[slotIndex];
        const auto newSlotVersion = request.getLayerSlotAssignmentVersion();
        // validate that the version is up to date, handle upate version conflicts such as when due to delays the master happens to receive out of order layer assignment operations from different instances.
        if (newSlotVersion > assignedSlotVersion)
        {
            TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") slotId(" << slotId << "), version(" << newSlotVersion << ") newer than prevVersion(" << assignedSlotVersion << "), claim accepted");
            mLayerSlotVersionList[slotIndex] = newSlotVersion; // update slot version
            response.setLayerSlotAssignmentVersion(newSlotVersion);
        }
        else if (newSlotVersion < assignedSlotVersion)
        {
            if (newSlotVersion == 1)
            {
                WARN_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") slotId(" << slotId << "), version(" << newSlotVersion << "), overrides prevVersion(" << assignedSlotVersion << "), claim accepted(forced)"); // This should only happen if somehow redis is reset and all version info is lost...
                mLayerSlotVersionList[slotIndex] = newSlotVersion; // update slot version
                response.setLayerSlotAssignmentVersion(newSlotVersion);
            }
            else
            {
                TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") slotId(" << slotId << "), version(" << newSlotVersion << ") older than prevVersion(" << assignedSlotVersion << "), claim igored");
                response.setLayerSlotAssignmentVersion(assignedSlotVersion);
                return WorkerClaimLayerAssignmentError::ERR_OK;
            }
        }
        else
        {
            TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") slotId(" << slotId << "), version(" << newSlotVersion << ") unchanged, claim accepted");
            response.setLayerSlotAssignmentVersion(assignedSlotVersion);
            return WorkerClaimLayerAssignmentError::ERR_OK;
        }
    }
    else
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") specified slotId(" << slotId << ") is out of range, claim rejected");
        return WorkerClaimLayerAssignmentError::ERR_SYSTEM;
    }

    auto layerId = LayerInfo::layerIdFromSlotId(slotId);
    auto layerIndex = LayerInfo::layerIndexFromLayerId(layerId);
    if (layerIndex >= LayerInfo::MAX_LAYERS)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") specified layer(" << layerId << ") is out of range, claim rejected");
        return WorkerClaimLayerAssignmentError::ERR_SYSTEM;
    }

    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr != mWorkerByInstanceId.end())
    {
        auto& worker = *workerItr->second;
        if (worker.mWorkerInstanceType != request.getInstanceTypeName())
        {
            ERR_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") failed to reassign slotId(" << worker.mWorkerSlotId << "->" << slotId << ") because instance type(" << worker.mWorkerInstanceType << ") does not match provided(" << request.getInstanceTypeName() << "), claim rejected");
            return WorkerClaimLayerAssignmentError::ERR_SYSTEM;
        }

        {
            auto workerBySlotItr = mWorkerBySlotId.find(worker.mWorkerSlotId);
            if (workerBySlotItr != mWorkerBySlotId.end())
            {
                TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") unassigned from slotId(" << worker.mWorkerSlotId << "), layer(" << worker.mWorkerLayerId << ")");
                EA_ASSERT_MSG(&worker == workerBySlotItr->second, "Worker instance mismatch!");
                mWorkerBySlotId.erase(workerBySlotItr); // free old slotid this worker held
            }
            else
            {
                EA_ASSERT_MSG(worker.mWorkerSlotId == 0, "Worker must have unset slotId!");
            }
        }

        if (worker.mWorkerLayerId == layerId)
        {
            // slot has changed, but the layer id remains the same, update the slot id
            auto ret = mWorkerBySlotId.insert(slotId);
            if (!ret.second)
            {
                auto& otherWorker = *ret.first->second;
                EA_ASSERT_MSG(otherWorker.mWorkerLayerId == layerId, "Bumped worker layer id mismatch!");
                TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") bumped worker(" << otherWorker.mWorkerName << ") from slotId(" << otherWorker.mWorkerSlotId << "), layer(" << otherWorker.mWorkerLayerId << "), expect new slot claim at version(" << request.getLayerSlotAssignmentVersion() << "+)");
                otherWorker.mWorkerSlotId = 0; // deliberately unassign the slot, while leaving the layer as it was to signal that this is a worker pending slot assignment
            }
            TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") reassigned to slotId(" << worker.mWorkerSlotId << "->" << slotId << "), layer(" << layerId << ")");
            worker.mWorkerSlotId = slotId;
            ret.first->second = &worker;
            return WorkerClaimLayerAssignmentError::ERR_OK;
        }

        // NOTE: When a worker leaves a layer it could 'technically' still be working on some sessions, those are still referencing said worker and expect him to complete them or error out on them.
        // This means that a worker that has switched layers could technically still finish off some sessions for a different layer while beginning to accept sessions from new layer.
        auto& layer = getLayerByWorker(worker);
        mWorkerByInstanceId.erase(workerItr);
        workerItr = nullptr; // null out iterator to ensure accidental misuse gets caught below
        delete &worker;
        if (layer.hasWorkers())
        {
            TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") migrated from layer(" << layer.mLayerId << ")");
        }
        else
        {
            TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: last worker(" << workerName << ") migrated from layer(" << layer.mLayerId << ")");

            // if we remove the last worker from the layer we need to reassign all the sessions assigned to this layer back into the unassigned list
            for (auto& fromList : layer.mSessionListByState)
            {
                mUnassignedSessionList.splice(mUnassignedSessionList.end(), fromList);
            }
        }
    }

    // ensure layer list is big enough to accomodate new layer index
    if (mLayerCount < layerIndex + 1)
        mLayerCount = layerIndex + 1;

    auto& layer = mLayerList[layerIndex];
    if (layer.mLayerId == LayerInfo::INVALID_LAYER_ID)
    {
        layer.mLayerId = layerId;
    }
    else
    {
        EA_ASSERT_MSG(layer.mLayerId == layerId, "Cannot change layerId for existing layer!"); // This assert means somehow we've got a bug where layerIndex does not match layerId!
    }
    auto& worker = *(BLAZE_NEW WorkerInfo);
    worker.mWorkerLayerId = layerId;
    worker.mWorkerSlotId = slotId;
    worker.mWorkerInstanceId = workerInstanceId;
    worker.mWorkerName = workerName;
    worker.mWorkerInstanceType = request.getInstanceTypeName();
    worker.mWorkerState = WorkerInfo::IDLE;
    {
        auto ret = mWorkerByInstanceId.insert(workerInstanceId);
        EA_ASSERT_MSG(ret.second, "Worker instance id already used!");
        ret.first->second = &worker;
    }
    {
        auto ret = mWorkerBySlotId.insert(slotId);
        if (!ret.second)
        {
            auto& otherWorker = *ret.first->second;
            EA_ASSERT_MSG(otherWorker.mWorkerSlotId == slotId, "Bumped worker slot id mismatch!");
            EA_ASSERT_MSG(otherWorker.mWorkerLayerId == layerId, "Bumped worker layer id mismatch!");
            TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") bumped worker(" << otherWorker.mWorkerName << ") from slotId(" << otherWorker.mWorkerSlotId << "), layer(" << otherWorker.mWorkerLayerId << "), expect new slot claim at version(" << request.getLayerSlotAssignmentVersion() << "+)");
            otherWorker.mWorkerSlotId = 0; // deliberately unassign the slot, while leaving the layer as it was to signal that this is a worker pending slot assignment
        }
        ret.first->second = &worker; // PACKER_MAYBE: we may want to unlink the bumped worker from the layer it was in, and clear the layerid/slotid pair inside it to keep things consistent, otherwise we can put the bumped worker into a new mUnassignedWorkerList
    }
    layer.mWorkerByState[worker.mWorkerState].push_back(worker);

    TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") assigned to slotId(" << worker.mWorkerSlotId << "), layer(" << worker.mWorkerLayerId << ")");
    
    // PACKER_TODO: #IMPORTANT: need to exhaustively verify this logic whereby we don't signal workers immediately as they are added, this cannot be allowed to result in starvation in the event something goes wrong on multiple layers... (e.g: bugs causing cross/layer worker rebalancing stalls)
    if (wakeIdleWorker(worker))
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") on layer(" << layerId << ") signalled");
    }
    else
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") on layer(" << layerId << ") idle"); 
    }

    return WorkerClaimLayerAssignmentError::ERR_OK;
}

GetPackerMetricsError::Error GamePackerMasterImpl::processGetPackerMetrics(const Blaze::GameManager::GetPackerMetricsRequest& request, Blaze::GameManager::GetPackerMetricsResponse& response, const::Blaze::Message * message)
{
#if 0 // PACKER_TODO: Need to think about what metrics we actually want to track and report
    for (auto& workerItr : mWorkerByInstanceId)
    {
        auto& worker = *workerItr.second;
        eastl::string metricsKey;
        metricsKey.sprintf("layer.%d.worker.%d.perf", (int32_t)worker.mWorkerLayerId, (int32_t)worker.mWorkerInstanceId);
        auto& metricsInfo = response.getMetricsInfo()[metricsKey.c_str()];
        metricsInfo = response.getMetricsInfo().allocate_element();
        auto& metricsList = metricsInfo->getMetrics();

        //metricsList.reserve(worker.mWorkerResponseStatusRing.size());
        //eastl::string data;
        //data.sprintf("%8s  %5s  %5s  %5s", "resp(us)", "state", "w_cpu", "m_cpu");
        //metricsList.push_back(data.c_str()); // push back header
        //for (auto& responseStatus : worker.mWorkerResponseStatusRing)
        //{
        //    data.sprintf("%8u  %5u  %5u  %5u", (uint32_t)responseStatus.mWorkerResponseTime.getMicroSeconds(), (uint32_t)responseStatus.mSignalledState, responseStatus.mWorkerCpuPercent, responseStatus.mMasterCpuPercent);
        //    metricsList.push_back(data.c_str());
        //}
    }
#endif
    return GetPackerMetricsError::ERR_OK;
}


bool GamePackerMasterImpl::isDrainComplete()
{ 
    return mSessionByIdMap.empty();
}

// PACKER_TODO: Change these constants to more appropriate values that come from load testing
const uint32_t GamePackerMasterImpl::WorkerInfo::RESET_BUSY_STATE_TIMEOUT_MS = 1000; // Wait this long before forcibly resetting a BUSY worker back to IDLE state.
const uint32_t GamePackerMasterImpl::WorkerInfo::RESET_IDLE_STATE_TIMEOUT_MS = 1000; // Wait this long before suspending a SIGNALLED and tardy worker by assigning it to BUSY state.

GamePackerMasterImpl::WorkerInfo::WorkerInfo()
{
    mpNext = mpPrev = nullptr;
    mWorkerLayerId = LayerInfo::INVALID_LAYER_ID;
    mWorkerState = IDLE;
    mWorkerInstanceId = INVALID_INSTANCE_ID;
    mWorkerSlotId = 0;
    mWorkerResetStateTimerId = INVALID_TIMER_ID;
}

GamePackerMasterImpl::WorkerInfo::~WorkerInfo()
{
    cancelResetStateTimer();
    WorkerInfoList::remove(*this);
}

void GamePackerMasterImpl::WorkerInfo::startResetStateTimer()
{
#if 0
    EA_ASSERT(mWorkerResetStateTimerId == INVALID_TIMER_ID);
    TimeValue relativeTimeout;
    switch (mWorkerState)
    {
    case WorkerState::SIGNALLED:
        relativeTimeout = RESET_IDLE_STATE_TIMEOUT_MS * 1000;
        break;
    case WorkerState::BUSY:
        relativeTimeout = RESET_BUSY_STATE_TIMEOUT_MS * 1000;
        break;
    default:
        EA_FAIL_MSG("Invalid state!");
        return;
    }

    mWorkerResetStateTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + relativeTimeout, gGamePackerMaster, &GamePackerMasterImpl::onWorkerStateResetTimeout, mWorkerInstanceId, mWorkerState, "GamePackerMasterImpl::onWorkerStateResetTimeout");
#endif
}

void GamePackerMasterImpl::onWorkerStateResetTimeout(InstanceId workerInstanceId, WorkerInfo::WorkerState state)
{
#if 0
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
        return;
    auto& worker = *workerItr->second;
    auto& layer = getLayerByWorker(worker);
    EA_ASSERT(worker.mWorkerResetStateTimerId != INVALID_TIMER_ID);
    EA_ASSERT(worker.mWorkerState == state);
    worker.mWorkerResetStateTimerId = INVALID_TIMER_ID;
    switch (state)
    {
    case WorkerInfo::WORKING: // If working but haven't heard in a while, mark it as busy, and reschedule another ping,
    case WorkerInfo::SIGNALLED:
    {
        // TODO: need to add metrics for the total number of times that instances time out when working, or after being signalled
        setWorkerState(worker, WorkerInfo::BUSY);
        WARN_LOG("[GamePackerMasterImpl].onWorkerStateResetTimeout: worker(" << worker.mWorkerInstanceId << ") in state(" << WorkerInfo::getWorkerStateString(state) << ") on layer(" << worker.mWorkerLayerId << ") but has not responded for(" << WorkerInfo::RESET_BUSY_STATE_TIMEOUT_MS << "ms), force state(" << WorkerInfo::getWorkerStateString(worker.mWorkerState) << ")");
        // Worker is being reset back to idle from signalled, this means it has not called back in time, which means that it is busy. Maybe: log warning because signalled workers should only rarely be reset to busy!
        worker.startResetStateTimer();
        break;
    }
    case WorkerInfo::BUSY:
    {
        // TODO: need to add workerinfo.mCountPings that go unanswered and unresponsiveness, so that we can use it for logging that the instance is busy, surface as metrics as well.
        // If worker was busy, it's time to try to reset it again
        WARN_LOG("[GamePackerMasterImpl].onWorkerStateResetTimeout: worker(" << worker.mWorkerInstanceId << ") in state(" << WorkerInfo::getWorkerStateString(state) << ") on layer(" << worker.mWorkerLayerId << ") has been busy for(" << WorkerInfo::RESET_IDLE_STATE_TIMEOUT_MS << "ms), force state(" << WorkerInfo::getWorkerStateString(worker.mWorkerState) << ")");

        // signal worker again in case there is pending work to do (we can have a state machine for worker state management, whereby we can simply specify target state and it will transition there.
        wakeIdleWorker(worker);
        break;
    }
    default:
        EA_FAIL_MSG("Invalid state!");
    }
#endif
}

void GamePackerMasterImpl::WorkerInfo::cancelResetStateTimer()
{
#if 0
    if (mWorkerResetStateTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mWorkerResetStateTimerId);
        mWorkerResetStateTimerId = INVALID_TIMER_ID;
    }
#endif
}

const char8_t* GamePackerMasterImpl::WorkerInfo::getWorkerStateString(WorkerState state)
{
    switch (state)
    {
    case Blaze::GamePacker::GamePackerMasterImpl::WorkerInfo::IDLE:
        return "idle";
    case Blaze::GamePacker::GamePackerMasterImpl::WorkerInfo::WORKING:
        return "working";
    case Blaze::GamePacker::GamePackerMasterImpl::WorkerInfo::SIGNALLED:
        return "signalled";
    case Blaze::GamePacker::GamePackerMasterImpl::WorkerInfo::BUSY:
        return "busy";
    default:
        return "unknown";
    }
}

GamePackerMasterImpl::SessionInfo::SessionInfo()
{
    mpNext = mpPrev = nullptr;
    mDelayedStartTimerId = INVALID_TIMER_ID;
}

GamePackerMasterImpl::SessionInfo::~SessionInfo()
{
    if (mDelayedStartTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDelayedStartTimerId);
    }
    if (mpNext != nullptr && mpPrev != nullptr)
        SessionInfoList::remove(*this);
}

EA::TDF::TimeValue GamePackerMasterImpl::SessionInfo::getRemainingLifetime(EA::TDF::TimeValue now) const
{
    return mSessionData->getStartedTimestamp() + mSessionData->getSessionDuration() - now;
}

Blaze::GamePacker::PackerSessionId GamePackerMasterImpl::SessionInfo::getPackerSessionId() const
{
    return mRequestInfo->getMatchmakingSessionId();
}

Blaze::GameManager::UserJoinInfo* GamePackerMasterImpl::SessionInfo::getHostJoinInfo() const
{
    auto& requestInfo = *mRequestInfo;
    for (auto& userJoinInfo : requestInfo.getUsersInfo())
    {
        if (userJoinInfo->getIsHost())
        {
            return userJoinInfo.get();
        }
    }
    return nullptr;
}

const Blaze::GameManager::ScenarioInfo* GamePackerMasterImpl::SessionInfo::getScenarioInfo() const
{
    auto& usersInfo = mRequestInfo->getUsersInfo();
    if (!usersInfo.empty())
        return &(usersInfo.front()->getScenarioInfo()); // PACKER_TODO: this info should be stored once per scenario, or once per subsession, but certainly not once per user!
    return nullptr;
}

const char8_t* GamePackerMasterImpl::SessionInfo::getSubSessionName(const char8_t* unknownName) const
{
    const auto* scenarioInfo = getScenarioInfo();
    if (scenarioInfo != nullptr)
        return scenarioInfo->getSubSessionName();
    return unknownName;
}

void GamePackerMasterImpl::SessionInfo::cancelDelayStartTimer()
{
    if (mDelayedStartTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDelayedStartTimerId);
        mDelayedStartTimerId = INVALID_TIMER_ID;
    }
}

const uint32_t GamePackerMasterImpl::LayerInfo::INVALID_LAYER_ID = 0;
const uint32_t GamePackerMasterImpl::LayerInfo::INVALID_SLOT_ID = 0;
const uint32_t GamePackerMasterImpl::LayerInfo::INVALID_SLOT_VERSION = 0;
const uint32_t GamePackerMasterImpl::LayerInfo::MAX_SLOTS = (1U << MAX_LAYERS); // 4096

GamePackerMasterImpl::LayerInfo::LayerInfo()
{
    mLayerId = INVALID_LAYER_ID;
    mWorkerMetricsByState = {};
    static_assert(GamePackerMasterImpl::LayerInfo::MAX_SLOTS >= INSTANCE_ID_MAX, "When we change the max number of blaze instances in a cluster we'll need to bump this number!");
}

GamePackerMasterImpl::LayerInfo::~LayerInfo()
{
}

bool GamePackerMasterImpl::LayerInfo::hasWorkers() const
{
    for (auto& workerList : mWorkerByState)
    {
        if (!workerList.empty())
            return true;
    }
    return false;
}

uint32_t GamePackerMasterImpl::LayerInfo::layerIndexFromLayerId(uint32_t layerId)
{
    return layerId - 1;
}

uint32_t GamePackerMasterImpl::LayerInfo::layerIdFromLayerIndex(uint32_t layerIndex)
{
    return layerIndex + 1;
}

uint32_t GamePackerMasterImpl::LayerInfo::layerIdFromSlotId(uint32_t slotId)
{
    uint32_t layerId = INVALID_LAYER_ID; // invalid
    while (slotId != 0)
    {
        ++layerId;
        slotId = slotId >> 1;
    }
    return layerId;
}

uint32_t GamePackerMasterImpl::LayerInfo::slotIndexFromSlotId(uint32_t slotId)
{
    return slotId - 1;
}

GamePackerMasterImpl::ScenarioInfo::ScenarioInfo()
{
    mScenarioId = GameManager::INVALID_SCENARIO_ID;
    mExpiryTimerId = INVALID_TIMER_ID;
    mFinalizationTimerId = INVALID_TIMER_ID;
}

GamePackerMasterImpl::ScenarioInfo::~ScenarioInfo()
{
    cancelExpiryTimer();
    cancelFinalizationTimer();
}

void GamePackerMasterImpl::ScenarioInfo::cancelExpiryTimer()
{
    if (mExpiryTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mExpiryTimerId);
        mExpiryTimerId = INVALID_TIMER_ID;
    }
}

void GamePackerMasterImpl::ScenarioInfo::cancelFinalizationTimer()
{
    if (mFinalizationTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mFinalizationTimerId);
        mFinalizationTimerId = INVALID_TIMER_ID;
    }
}

const Blaze::GameManager::ScenarioInfo* GamePackerMasterImpl::ScenarioInfo::getScenarioInfo() const
{
    if (!mUserJoinInfoList.empty())
        return &(mUserJoinInfoList.front()->getScenarioInfo()); // PACKER_TODO: this info should be stored once per scenario, not once per user!
    return nullptr;
}

} // GamePacker
} // Blaze

#pragma pop_macro("LOGGER_CATEGORY")
