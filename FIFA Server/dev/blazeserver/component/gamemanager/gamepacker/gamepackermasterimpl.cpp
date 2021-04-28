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
const char8_t PRIVATE_PACKER_SESSION_FIELD_FMT[] = "pri.session.%" PRId64;
const char8_t* VALID_PACKER_FIELD_PREFIXES[] = { PRIVATE_PACKER_SCENARIO_FIELD, PRIVATE_PACKER_SESSION_FIELD, PRIVATE_PACKER_REQUEST_FIELD, nullptr };
// PACKER_TODO - Make this a configurable list
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
    , mUsersActive(getMetricsCollection(), "playersMatchmaking", Metrics::Tag::scenario_name, Metrics::Tag::subsession_name)
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
    mTimeToMatchEstimator.clearTtmEstimates();
    return true;
}

bool GamePackerMasterImpl::onPrepareForReconfigure(const GamePackerConfig& newConfig)
{
    // if we'll be able to persist any scenarios across the reconfigure, we'll identify their scenario names here
    // default is that we terminate all scenarios
    return true;
}

bool GamePackerMasterImpl::onReconfigure()
{
    // build our list of scenarios to be terminated
    // currently, that's all of them!

    PackerScenarioIdList terminatingScenarios;
    terminatingScenarios.reserve(mPackerScenarioByIdMap.size());
    for (const auto& packerScenarioIter : mPackerScenarioByIdMap)
    {
        terminatingScenarios.push_back(packerScenarioIter.first);
    }

    terminateScenarios(terminatingScenarios, GameManager::PackerScenarioData::SCENARIO_SYSTEM_TERMINATED);

    mTimeToMatchEstimator.clearTtmEstimates();
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

void GamePackerMasterImpl::addScenarioBlazeIdMapping(const GamePackerMasterImpl::PackerScenarioMaster& packerScenario)
{
    for (auto& userJoinInfo : packerScenario.getUserJoinInfoList())
    {
        BlazeId blazeId = userJoinInfo->getUser().getUserInfo().getId();
        if (blazeId != INVALID_BLAZE_ID)
            mPackerScenarioIdByBlazeIdMap.insert(blazeId)->second = packerScenario.getPackerScenarioId();
        else
            WARN_LOG("[GamePackerMasterImpl].onImportStorageRecord: Expected valid BlazeId for PackerScenario(" << packerScenario.getPackerScenarioId() << ")");
    }
}

void GamePackerMasterImpl::removeScenarioBlazeIdMapping(const GamePackerMasterImpl::PackerScenarioMaster& packerScenario)
{
    for (auto& userJoinInfo : packerScenario.getUserJoinInfoList())
    {
        BlazeId blazeId = userJoinInfo->getUser().getUserInfo().getId();
        if (blazeId == INVALID_BLAZE_ID)
        {
            WARN_LOG("[GamePackerMasterImpl].onExportStorageRecord: Expected valid BlazeId for PackerScenario(" << packerScenario.getPackerScenarioId() << ")");
            continue;
        }
        // remove all blazeId->scenarioId mapping pairs that reference this scenario
        auto itrPair = mPackerScenarioIdByBlazeIdMap.equal_range(blazeId);
        while (itrPair.first != itrPair.second)
        {
            auto scenarioId = itrPair.first->second;
            if (packerScenario.getPackerScenarioId() == scenarioId)
                itrPair.first = mPackerScenarioIdByBlazeIdMap.erase(itrPair.first);
            else
                ++itrPair.first;
        }
    }
}

void GamePackerMasterImpl::onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot)
{
    // Explicitly suppress blocking here, we don't want to accidentally let timers execute while the game is being unpacked...
    Fiber::BlockingSuppressionAutoPtr suppressAutoPtr;
    PackerScenarioId packerScenarioId = snapshot.recordId;

    auto packerScenarioIter = mPackerScenarioByIdMap.find(packerScenarioId);
    if (packerScenarioIter != mPackerScenarioByIdMap.end())
    {
        ERR_LOG("[GamePackerMasterImpl].onImportStorageRecord: Unexpected collision inserting PackerScenario(" << packerScenarioId << ").");
        return;
    }

    // Check that the Scenario's data exists:
    StorageRecordSnapshot::FieldEntryMap::const_iterator snapScenarioFieldItr = getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_PACKER_SCENARIO_FIELD);
    StorageRecordSnapshot::FieldEntryMap::const_iterator snapSessionFieldItr  = getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_PACKER_SESSION_FIELD);
    StorageRecordSnapshot::FieldEntryMap::const_iterator snapRequestFieldItr  = getStorageFieldByPrefix(snapshot.fieldMap, PRIVATE_PACKER_REQUEST_FIELD);
    if (snapScenarioFieldItr == snapshot.fieldMap.end() || snapSessionFieldItr == snapshot.fieldMap.end() || snapRequestFieldItr == snapshot.fieldMap.end())
    {
        ERR_LOG("[GamePackerMasterImpl].onImportStorageRecord: Field prefix=" << PRIVATE_PACKER_SCENARIO_FIELD << " or " << PRIVATE_PACKER_SESSION_FIELD << " or " << PRIVATE_PACKER_REQUEST_FIELD << " matches no fields in PackerScenario(" << packerScenarioId << ").");
        return;
    }

    {
        GameManager::PackerScenarioData& packerScenarioData = static_cast<GameManager::PackerScenarioData&>(*snapScenarioFieldItr->second);
        GameManager::StartPackerScenarioRequest& packerScenarioRequest = static_cast<GameManager::StartPackerScenarioRequest&>(*snapRequestFieldItr->second);

        // Add the Scenario:
        PackerScenarioMaster& packerScenario = *addPackerScenario(packerScenarioRequest.getPackerScenarioId(), &packerScenarioRequest, &packerScenarioData);

        // Add the Sessions:
        size_t privateSessionFieldPrefixLen = strlen(PRIVATE_PACKER_SESSION_FIELD);
        for (; snapSessionFieldItr != snapshot.fieldMap.end() && (blaze_strncmp(snapSessionFieldItr->first.c_str(), PRIVATE_PACKER_SESSION_FIELD, privateSessionFieldPrefixLen) == 0); ++snapSessionFieldItr)
        {
            GameManager::PackerSessionData& packerSessionData = static_cast<GameManager::PackerSessionData&>(*snapSessionFieldItr->second);
            addPackerSession(packerSessionData.getPackerSessionId(), packerScenario, &packerSessionData);
        }

        // Reactivate the Sessions: 
        activatePackerScenario(packerScenario);
    }
}

void GamePackerMasterImpl::activatePackerScenario(PackerScenarioMaster& packerScenario)
{
    for (PackerSessionMaster* curPackerSession : packerScenario.mPackerSessionMasterList)
    {
        auto& packerSession = *curPackerSession;
        activatePackerSession(packerSession);
    }
}
void GamePackerMasterImpl::activatePackerSession(PackerSessionMaster& packerSession, bool forceAssignNewWorker)
{
    switch (packerSession.getState())
    {
    case GameManager::PackerSessionData::SESSION_DELAYED:
    {
        packerSession.addToIntrusiveList(mDelayedSessionList);
        if (packerSession.mDelayedStartTimerId == INVALID_TIMER_ID)
        {
            auto expiry = packerSession.getStartedTimestamp() + packerSession.getStartDelay();
            packerSession.mDelayedStartTimerId = gSelector->scheduleTimerCall(
                expiry, this, &GamePackerMasterImpl::startDelayedSession, packerSession.getPackerSessionId(), "GamePackerMasterImpl::startDelayedSession");
        }
        break;
    }
    case GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT:
    {
        signalSessionAssignment(packerSession);
        break;
    }
    case GameManager::PackerSessionData::SESSION_ASSIGNED:
    {
        // After importing the sessions, we should make sure that they are part of the correct layers and workers:
        // We try to maintain layer, even if the worker is no longer around:
        uint32_t layerId = packerSession.getAssignedLayerId();
        if (layerId != LayerInfo::INVALID_LAYER_ID)
        {
            InstanceId workerInstanceId = packerSession.getAssignedWorkerInstanceId();
            auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
            if (forceAssignNewWorker || workerItr == mWorkerByInstanceId.end())
            {
                auto* layerInfo = getLayerFromId(layerId);
                if (!layerInfo->hasWorkers())
                    layerId = LayerInfo::INVALID_LAYER_ID;

                if (!forceAssignNewWorker)
                {
                    WARN_LOG("[GamePackerMasterImpl].activatePackerSession: Worker instance(" << workerInstanceId << ") is not registered.  Attempting to use a new worker on layer (" << layerId << ").");
                }

                // If the Worker doesn't exist anymore, we switch the Session back to SESSION_AWAITING_ASSIGNMENT:
                packerSession.releaseSessionFromWorker(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
                assignSessionToLayer(packerSession, layerId); // takes care of calling upsertSessionData()
                return;
            }
            else
            {
                // Note:  This doesn't check to ensure that the worker is on the same layer as listed.  It assumes that this is an import, and the worker already has the session. 
                auto& worker = *workerItr->second;
                worker.assignWorkerSession(packerSession);
            }
        }
        else
        {
            ERR_LOG("[GamePackerMasterImpl].activatePackerSession: Session (" << packerSession.getPackerSessionId() << ") is using State SESSION_ASSIGNED, but no layer was assigned.  This should not be possible.");
        }
        break;
    }
    default:
        WARN_LOG("[GamePackerMasterImpl].activatePackerSession: Session ("<< packerSession.getPackerSessionId() <<") is using State(" << packerSession.getState() << "). "
                    << " No additional layer/worker assignment will occur (the values tracked through Redis are unused), the session will not be tracked on the unassigned or idle lists.");
        break;
    }
}

void GamePackerMasterImpl::onExportStorageRecord(StorageRecordId recordId)
{
    PackerScenarioId exportScenarioId = (PackerScenarioId)recordId;
    PackerScenarioMasterMap::iterator scenarioIt = mPackerScenarioByIdMap.find(exportScenarioId);
    if (scenarioIt == mPackerScenarioByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].onExportStorageRecord: Failed to export PackerScenario(" << exportScenarioId << "), not found.");
        return;
    }

    PackerScenarioMaster& packerScenario = *scenarioIt->second;

    notifyAssignedSessionsTermination(packerScenario);        // PACKER_TODO - It's unclear if the assigned worker Slaves need to be told to terminate the sessions or not, when an export occurs. 

    removeScenarioAndSessions(packerScenario, false);     // false == Redis export, no completion metrics sent:
}


void GamePackerMasterImpl::onScenarioExpiredTimeout(PackerScenarioId packerScenarioId)
{
    auto scenarioItr = mPackerScenarioByIdMap.find(packerScenarioId);
    if (scenarioItr == mPackerScenarioByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].onScenarioExpiredTimeout: Expected PackerScenario(" << packerScenarioId << ") not found");
        return;
    }

    auto& packerScenario = *scenarioItr->second;
    packerScenario.mExpiryTimerId = INVALID_TIMER_ID;
    auto& scenarioData = *packerScenario.mScenarioData;
    auto lockedByPackerSessionId = scenarioData.getLockedForFinalizationByPackerSessionId();
    if (lockedByPackerSessionId != INVALID_PACKER_SESSION_ID)
    {
        // PACKER_MAYBE: #ARCHITECTURE We may need to significantly delay the scenario expiration timeout in the event the timer fires while locked for finalization because finalization should have a separate timeout that applies above and beyond the scenarios 'active packing phase' timeout. This comes from conversations with Jesse where it was proposed that the specified matchmaking duration does not really include the finalization time (if needed it should be separate but still short).

        TRACE_LOG("[GamePackerMasterImpl].onScenarioExpiredTimeout: PackerScenario(" << packerScenarioId << "), PackerScenario(" << scenarioData << ") expired while locked for finalization, no-op");
        // PACKER_TODO: add metric
        return;
    }

    for (PackerSessionMaster* packerSessionIter : packerScenario.mPackerSessionMasterList)
    {
        PackerSessionMaster& packerSession = *packerSessionIter;
        PackerSessionId packerSessionId = packerSession.getPackerSessionId();
        auto assignedInstanceId = packerSession.getAssignedWorkerInstanceId();

        INFO_LOG("[GamePackerMasterImpl].onScenarioExpiredTimeout: PackerSession(" << packerSessionId << ") assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ") expired without completing, sessionData(" << *packerSession.mSessionData << ")"); // PACKER_TODO: Change to trace
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

        sendMatchmakingFailedNotification(packerSession, GameManager::SESSION_TIMED_OUT);
    }

    packerScenario.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_TIMED_OUT);
    cleanupScenarioAndSessions(packerScenario);
}

void GamePackerMasterImpl::onScenarioFinalizationTimeout(PackerScenarioId packerScenarioId)
{
    auto scenarioItr = mPackerScenarioByIdMap.find(packerScenarioId);
    if (scenarioItr == mPackerScenarioByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].onScenarioFinalizationTimeout: PackerScenario(" << packerScenarioId << ") expected!");
        return;
    }

    auto& packerScenario = *scenarioItr->second;
    packerScenario.mFinalizationTimerId = INVALID_TIMER_ID;
    auto& scenarioData = *packerScenario.mScenarioData;
    auto lockedByPackerSessionId = scenarioData.getLockedForFinalizationByPackerSessionId();
    if (lockedByPackerSessionId == INVALID_PACKER_SESSION_ID)
    {
        WARN_LOG("[GamePackerMasterImpl].onScenarioFinalizationTimeout: PackerScenario(" << packerScenarioId << ") unexpected not locked by packer session!");
        return;
    }

    auto sessionItr = mSessionByIdMap.find(lockedByPackerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        ERR_LOG("[GamePackerMasterImpl].onScenarioFinalizationTimeout: PackerSession(" << lockedByPackerSessionId << ") that locked PackerScenario(" << packerScenarioId << ") for finalization unexpectedly missing!");
        return;
    }

    auto& packerSession = *sessionItr->second;
    auto now = TimeValue::getTimeOfDay();
    auto delta = now - scenarioData.getLockedTimestamp();
    WARN_LOG("[GamePackerMasterImpl].onScenarioFinalizationTimeout: PackerScenario(" << packerScenarioId << "), scenarioData(" << scenarioData << ") finalization timeout(" << delta.getMillis() << "ms) exceeded, " <<
             "abort PackerSession(" << lockedByPackerSessionId << "), sessionData(" << *packerSession.mSessionData << "), sessionRequest(" << *packerSession.mSessionRequest << ")");

    abortSession(packerSession);
}

bool GamePackerMasterImpl::getWorkersForLayer(uint32_t layerId, eastl::vector<GamePackerMasterImpl::WorkerInfo*>& workers, uint32_t& loadSum)
{
    uint32_t slotIdStart = 1;
    uint32_t slotIdEnd = LayerInfo::MAX_SLOTS + 1;              // Default INVALID_LAYER checks all slots.

    if (layerId != LayerInfo::INVALID_LAYER_ID && (layerId - 1) < mLayerCount)
    {
        slotIdStart = 1U << (layerId - 1);          // Pattern here goes: [1-2), [2-4), [4-8) ... 
        slotIdEnd = 1U << (layerId);
    }

    loadSum = 0;
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
            uint32_t instanceLoad = remoteInstance->getTotalLoad();
            if (instanceLoad < 1)
                instanceLoad = 1;
            loadSum += instanceLoad;
            workers.push_back(&worker);
        }
        else
        {
            WARN_LOG("[GamePackerMasterImpl].getWorkersForLayer: Unknown worker(" << worker.mWorkerInstanceId << ") on layer(" << layerId << "), skipping"); // this should never happen because we always immediately clean up workers when they go away
        }
    }

    return !workers.empty();
}

void GamePackerMasterImpl::signalSessionAssignment(PackerSessionMaster& packerSession)
{
    // Make sure the correct layer/list knows about the PackerSession:
    uint32_t layerId = packerSession.getAssignedLayerId();
    if (layerId != LayerInfo::INVALID_LAYER_ID)
    {
        auto* layer = getLayerFromId(layerId);
        packerSession.addToIntrusiveList(layer->mIdleSessionList);
    }
    else
    {
        packerSession.addToIntrusiveList(mUnassignedSessionList);
    }


    // Gather the workers:  (First from the assigned layer, then from any layer if the assigned layer is empty)
    uint32_t loadSum = 0;
    eastl::vector<WorkerInfo*> workers;
    workers.reserve(mWorkerBySlotId.size());
    if (!getWorkersForLayer(layerId, workers, loadSum))
    {
        // NOTE: We don't change the layerId held by the packer session here.  That will be done when the session is actually obtained by a worker.
        layerId = LayerInfo::INVALID_LAYER_ID;
        packerSession.addToIntrusiveList(mUnassignedSessionList);
        WARN_LOG("[GamePackerMasterImpl].signalSessionAssignment: No instances on assigned layer(" << layerId << "), attempting to use any layer (invalid layer).");

        if (!getWorkersForLayer(layerId, workers, loadSum))
        {
            ERR_LOG("[GamePackerMasterImpl].signalSessionAssignment:  All known workers are missing.  This packer session will not be added to any");
            return;
        }
    }


    // Limit the workers by CPU threshold:
    size_t numWorkers = 0;
    float cpuThreshold = (float) getConfig().getWorkerSettings().getTargetCpuThreshold();
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

    // Choose a worker to signal:
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
            {
                // This indicates that the session was lost, but we still have an entry in the mWorkerBySlotId.  
                //  PACKER_TODO - Update the mWorkerBySlotId at the same time that the gController->getRemoteServerInstance value changes via a RemoteServerInstanceListener.
                //  There are no blocking actions between the start of the function (where we gather the workers list) and here, so the workers will not have disappeared. 
                EA_FAIL_MSG("Remote server worker instance disappeared unexpectedly!");
                
            }
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

    if (layerId != LayerInfo::INVALID_LAYER_ID && workerToWake->getLayerId() != layerId)
    {
        ERR_LOG("[GamePackerMasterImpl].signalSessionAssignmentInternal: Session attempting to assign to layer(" << layerId << ") was using worker ("<< workerToWake->mWorkerInstanceId <<"), but that worker is on layer ("<< workerToWake->getLayerId() <<")!");
    }

    if (workerToWake->mWorkerState == WorkerInfo::IDLE)
    {
        wakeIdleWorker(*workerToWake);
    }
    else
    {
        // PACKER_TODO: add a metric when a worker for the number of times a worker that was selected for assignment was already working.
    }
}

void GamePackerMasterImpl::WorkerInfo::setWorkerState(GamePackerMasterImpl::WorkerInfo::WorkerState state)
{
    mWorkerState = state;
    WorkerInfoList::remove(*this);       // Removes from mWorkerLayerInfo.mWorkerByState[mWorkerState]
    mWorkerLayerInfo->mWorkerByState[mWorkerState].push_back(*this);
}

void GamePackerMasterImpl::WorkerInfo::assignWorkerSession(PackerSessionMaster& packerSession)
{
    // PACKER_TODO - This function should be doing more, or part of something else. 
    packerSession.addToIntrusiveList(mWorkingSessionList);
}

void GamePackerMasterImpl::PackerSessionMaster::addToIntrusiveList(eastl::intrusive_list<PackerSessionMaster>& sessionList)
{
    // Remove from any older list: 
    if (mpPrev != nullptr || mpNext != nullptr)
    {
        PackerSessionList::remove(*this);
    }
    sessionList.push_back(*this);
}

bool GamePackerMasterImpl::wakeIdleWorker(WorkerInfo& worker)
{
    if (worker.mWorkerState != WorkerInfo::IDLE)
    {
        ERR_LOG("[GamePackerMasterImpl].wakeIdleWorker: Only idle/busy workers may be woken.  Worker '" << worker.mWorkerName << "'  state is: " << WorkerInfo::getWorkerStateString(worker.mWorkerState));
        return false;
    }

    auto& layer = getLayerByWorker(worker);
    if (mUnassignedSessionList.empty() && layer.mIdleSessionList.empty())
    {
        ERR_LOG("[GamePackerMasterImpl].wakeIdleWorker: Wake called while no sessions are in the assignable idle/unassigned lists.  No action performed.");
        return false; // nothing to do
    }

    TRACE_LOG("[GamePackerMasterImpl].wakeIdleWorker: Notify worker(" << worker.mWorkerName << ")");

    GameManager::NotifyWorkerPackerSessionAvailable notif;
    notif.setInstanceId(gController->getInstanceId());
    sendNotifyWorkerPackerSessionAvailableToInstanceById(worker.mWorkerInstanceId, &notif);
    return true;
}

/// This function compliments MatchmakingScenarioManager::convertGamePackerErrorToGameManagerError:
BlazeRpcError convertGameManagerErrorToGamePackerError(BlazeRpcError error)
{
    switch (error)
    {
    case GAMEMANAGER_ERR_CROSSPLAY_DISABLED:          return GAMEPACKER_ERR_CROSSPLAY_DISABLED;
    case GAMEMANAGER_ERR_CROSSPLAY_DISABLED_USER:     return GAMEPACKER_ERR_CROSSPLAY_DISABLED_USER;
    case GAMEMANAGER_ERR_INVALID_TEMPLATE_NAME:       return GAMEPACKER_ERR_INVALID_TEMPLATE_NAME;
    case GAMEMANAGER_ERR_MISSING_TEMPLATE_ATTRIBUTE:  return GAMEPACKER_ERR_MISSING_TEMPLATE_ATTRIBUTE;
    case GAMEMANAGER_ERR_INVALID_TEMPLATE_ATTRIBUTE:  return GAMEPACKER_ERR_INVALID_TEMPLATE_ATTRIBUTE;

    default:                                          return error;
    }
}


// PACKER_TODO: Move this into a common utility so that it can be accessed in both the search slaves (when returning games matched by findgames), and also here in packer master.
BlazeError GamePackerMasterImpl::initializeAggregateProperties(GamePackerMasterImpl::PackerSessionMaster& packerSession, GameManager::MatchmakingCriteriaError& criteriaError)
{
    // validate request ...
    auto& gameTemplateName = packerSession.getCreateGameTemplateName();
    auto& createGameTemplatesMap = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates();
    auto cfgItr = createGameTemplatesMap.find(gameTemplateName);
    if (cfgItr == createGameTemplatesMap.end())
    {
        ASSERT_LOG("[GamePackerMasterImpl].initializeAggregateProperties: unknown game template(" << gameTemplateName << ")");
        return StartPackerScenarioError::GAMEPACKER_ERR_INVALID_TEMPLATE_NAME;
    }
    
    auto& filters = packerSession.getMatchmakingFilters();
    auto& propertyMap = packerSession.getMatchmakingPropertyMap();

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
            return StartPackerScenarioError::GAMEPACKER_ERR_INVALID_MATCHMAKING_CRITERIA;
        }

        EA::TDF::TdfPtr outFilter;
        const char8_t* failingAttr = nullptr;
        BlazeRpcError blazeErr = applyTemplateTdfAttributes(outFilter, filtersConfig->second->getRequirement(), packerSession.getPackerScenarioMaster().getScenarioAttributes(), propertyMap, failingAttr, "[MatchmakingScenarioManager].createScenario: ");
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
            return convertGameManagerErrorToGamePackerError(blazeErr);
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
                // This should already be handled by the Scenarios config. 
                ERR_LOG("[GamePackerMasterImpl].initializeAggregateProperties: Property Filter " << curFilterName << " defines a Platform Rule when one is already in use for this Scenario.");
                return GAMEPACKER_ERR_DUPLICATE_REQUIRED_FILTERS_PROVIDED;
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
        auto errResult = GameManager::ValidationUtils::validateRequestCrossplaySettings(isCrossplayEnabled, packerSession.getPackerScenarioMaster().getUserJoinInfoList(), 
            packerSession.mSessionRequest->getMatchmakingSessionData().getSessionMode().getCreateGame(),
            packerSession.mSessionRequest->getGameCreationData(), getConfig().getGameSession(),
            filter.getClientPlatformListOverride(), packerSession.mParentPackerScenario->getOwnerUserSessionInfo().getUserInfo().getId(), filter.getCrossplayMustMatch());
        if (errResult != ERR_OK)
        {
            criteriaError.setErrMessage((StringBuilder() << "Crossplay enabled user attempted to create a game with invalid settings. " << ErrorHelp::getErrorDescription(errResult)).c_str());
            ERR_LOG("[GamePackerMasterImpl].initializeAggregateProperties:  Crossplay enabled user attempted to create a game with invalid settings.  Unable to just disable crossplay to fix this.");
            return convertGameManagerErrorToGamePackerError(errResult);
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
            for (auto& userInfo : packerSession.getPackerScenarioMaster().getUserJoinInfoList())
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
        return GAMEPACKER_ERR_MISSING_REQUIRED_FILTER;
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
        for (auto& curTdfMapping : *scoreShaperTdfMapping)
        {
            float value = 0;
            auto& paramName = curTdfMapping.first;

            // Only care about attributes:
            auto curAttr = packerSession.getPackerScenarioMaster().getScenarioAttributes().find(curTdfMapping.second->getAttrName());
            if (curAttr != packerSession.getPackerScenarioMaster().getScenarioAttributes().end())
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

void GamePackerMasterImpl::assignSessionToLayer(GamePackerMasterImpl::PackerSessionMaster& assignPackerSession, uint32_t assignedLayerId)
{
    auto sessionState = assignPackerSession.getState();
    if (sessionState != GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT)
    {
        TRACE_LOG("[GamePackerMasterImpl].assignSessionToLayer: PackerSession(" << assignPackerSession.getPackerSessionId() << ") in state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ") cannot be assigned.");
        return; // only suspended sessions can be resumed
    }

    // Switch from Unassigned to Idle: 
    assignPackerSession.mSessionData->setAssignedLayerId(assignedLayerId);

    signalSessionAssignment(assignPackerSession);
    upsertSessionData(assignPackerSession);
}

void GamePackerMasterImpl::resumeSuspendedSession(GamePackerMasterImpl::PackerSessionMaster& resumePackerSession)
{
    auto sessionState = resumePackerSession.getState();
    if (sessionState != GameManager::PackerSessionData::SESSION_SUSPENDED)
    {
        TRACE_LOG("[GamePackerMasterImpl].resumeSuspendedSession: PackerSession(" << resumePackerSession.getPackerSessionId() << ") in state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ") cannot be resumed.");
        return; // only suspended sessions can be resumed
    }

    resumePackerSession.mSessionData->setState(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
    assignSessionToLayer(resumePackerSession, resumePackerSession.getAssignedLayerId());
}

void GamePackerMasterImpl::abortSession(PackerSessionMaster& abortPackerSession)
{
    auto abortPackerSessionId = abortPackerSession.getPackerSessionId();
    auto& packerScenario = abortPackerSession.getPackerScenarioMaster();

    // Switch the session to ABORTED early to avoid various checks:
    abortPackerSession.releaseSessionFromWorker(GameManager::PackerSessionData::SESSION_ABORTED);
    
    auto lockedForFinalizationByPackerSessionId = packerScenario.getLockedForFinalizationByPackerSessionId();
    bool wasLockedForFinalizaion = (lockedForFinalizationByPackerSessionId == abortPackerSessionId);
    if (wasLockedForFinalizaion)
    {
        TRACE_LOG("[GamePackerMasterImpl].abortSession: Aborted PackerSession(" << abortPackerSessionId << ") is locking PackerScenario(" << packerScenario.getPackerScenarioId() << ") for finalization");
        packerScenario.mScenarioData->setLockedForFinalizationByPackerSessionId(INVALID_PACKER_SESSION_ID);
        packerScenario.cancelFinalizationTimer();

        if (packerScenario.getTerminationReason() != GameManager::PackerScenarioData::SCENARIO_SYSTEM_TERMINATED)
        {
            // we are aborting a session that had the scenario locked, if any other sibling sessions are currently suspended we should resume them (in order of priority)
            for (PackerSessionMaster* packerSessionMaster : packerScenario.mPackerSessionMasterList)
            {
                resumeSuspendedSession(*packerSessionMaster);
            }
        }
    }


    bool hasRemainingValidSessions = false;
    if (packerScenario.getTerminationReason() != GameManager::PackerScenarioData::SCENARIO_SYSTEM_TERMINATED)
    {
        for (auto* packerSession : packerScenario.mPackerSessionMasterList)
        {
            if (packerSession->mSessionData->getState() != GameManager::PackerSessionData::SESSION_ABORTED)
            {
                hasRemainingValidSessions = true;
                break;
            }
        }
    }

    if (hasRemainingValidSessions)
    {
        // PACKER_TODO - Just do upserts when the values change instead of sprinkling them randomly throughout the code where they can be accidentally missed.
        TRACE_LOG("[GamePackerMasterImpl].abortSession: Aborted PackerSession(" << abortPackerSessionId << "), PackerScenario(" << packerScenario.getPackerScenarioId() << ") has remaining valid sessions, update scenario");
        upsertSessionData(abortPackerSession);
        if (wasLockedForFinalizaion)
            upsertScenarioData(packerScenario);
    }
    else
    {
        // since the scenario manager doesn't know about the subsessions in packer, we hold this notification unless it was the last valid session.
        // PACKER_TODO: is SESSION_ERROR_GAME_SETUP_FAILED always the correct reason here?
        sendMatchmakingFailedNotification(abortPackerSession, GameManager::SESSION_ERROR_GAME_SETUP_FAILED);
        WARN_LOG("[GamePackerMasterImpl].abortSession: Aborted PackerSession(" << abortPackerSessionId << "), PackerScenario(" << packerScenario.getPackerScenarioId() << ") has no remaining valid sessions, cleanup scenario");
        packerScenario.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_FAILED);
        cleanupScenarioAndSessions(packerScenario);
    }
}

void GamePackerMasterImpl::terminateScenarios(const PackerScenarioIdList& terminatedScenarioIds, GameManager::PackerScenarioData::TerminationReason terminationReason)
{
    for (const auto& scenarioIdItr : terminatedScenarioIds)
    {
        PackerScenarioId packerScenarioId = scenarioIdItr;
        auto packerScenarioIter = mPackerScenarioByIdMap.find(packerScenarioId);
        if (packerScenarioIter == mPackerScenarioByIdMap.end())
        {
            SPAM_LOG("[GamePackerMasterImpl].terminateScenarios: PackerScenario(" << packerScenarioId << "), was not in mPackerScenarioByIdMap, skipping");
            continue;
        }
        else 
        {
            auto& packerScenario = *packerScenarioIter->second;
            
            packerScenario.mExpiryTimerId = INVALID_TIMER_ID;
            auto& scenarioData = *packerScenario.mScenarioData;

            auto lockedByPackerSessionId = scenarioData.getLockedForFinalizationByPackerSessionId();
            if (lockedByPackerSessionId != INVALID_PACKER_SESSION_ID)
            {
                // we skip over finalizing scenario sessions, but we send the scenario manager MM failed notifiations for the other scenarios, and cancel delayed start timers, if any.
                packerScenario.mScenarioData->setTerminationReason(terminationReason);
                // notify workers to treat the scenario as terminated (but permit outstanding finalization attempt to complete)
                notifyAssignedSessionsTermination(packerScenario);

                TRACE_LOG("[GamePackerMasterImpl].terminateScenarios: PackerScenario(" << packerScenarioId << "), packerScenario(" << scenarioData << ") terminated while locked for finalization, skipping");
                // PACKER_TODO: add metric

                // If there are any delayed subsessions, cancel their timers here
                for (PackerSessionMaster* packerSessionIter : packerScenario.mPackerSessionMasterList)
                {
                    PackerSessionMaster& packerSession = *packerSessionIter;
                    PackerSessionId packerSessionId = packerSession.getPackerSessionId();
                    if (lockedByPackerSessionId != packerSessionId)
                    {
                        if (packerSession.getState() == GameManager::PackerSessionData::SESSION_DELAYED)
                        {
                            TRACE_LOG("[GamePackerMasterImpl].terminateScenarios: Delayed PackerSession(" << packerSessionId << ") cancelled timer due to reconfigure impacting sessionData(" << *packerSession.mSessionData << ")");

                            packerSession.cancelDelayStartTimer();
                        }

                        sendMatchmakingFailedNotification(packerSession, GameManager::SESSION_TERMINATED);
                    }
                }
                continue;
            }


            for (PackerSessionMaster* packerSessionIter : packerScenario.mPackerSessionMasterList)
            {
                PackerSessionMaster& packerSession = *packerSessionIter;
                PackerSessionId packerSessionId = packerSession.getPackerSessionId();
                auto assignedInstanceId = packerSession.getAssignedWorkerInstanceId();

                TRACE_LOG("[GamePackerMasterImpl].terminateScenarios: PackerSession(" << packerSessionId << ") assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ") terminated without completing, sessionData(" << *packerSession.mSessionData << ")");

                sendMatchmakingFailedNotification(packerSession, GameManager::SESSION_TERMINATED);
            }

            packerScenario.mScenarioData->setTerminationReason(terminationReason);
            cleanupScenarioAndSessions(packerScenario);
        }
    }
    
}

void GamePackerMasterImpl::notifyAssignedSessionsTermination(PackerScenarioMaster& packerScenario)
{
    GameManager::NotifyWorkerPackerSessionTerminated notifyTerminated;
    notifyTerminated.setTerminationReason(packerScenario.getTerminationReason());
    for (PackerSessionMaster* PackerSessionMaster : packerScenario.mPackerSessionMasterList)
    {
        PackerSessionId erasePackerSessionId = PackerSessionMaster->getPackerSessionId();
        auto assignedWorkerInstanceId = PackerSessionMaster->getAssignedWorkerInstanceId();
        if (assignedWorkerInstanceId != INVALID_INSTANCE_ID)
        {
            // Tell the PackerSlaves to terminate any assigned Sessions:
            notifyTerminated.setPackerSessionId(erasePackerSessionId);
            notifyTerminated.setPackerScenarioId(packerScenario.getPackerScenarioId());
            sendNotifyWorkerPackerSessionTerminatedToInstanceById(assignedWorkerInstanceId, &notifyTerminated);
        }
        TRACE_LOG("[GamePackerMasterImpl].cleanupScenario: PackerSession(" << erasePackerSessionId << ") in PackerScenario(" << packerScenario.getPackerScenarioId() << ") erased, notify assigned worker(" << getWorkerNameByInstanceId(assignedWorkerInstanceId) << ").");
    }
}

void GamePackerMasterImpl::removeScenarioAndSessions(PackerScenarioMaster& packerScenario, bool requestCompleted)
{
    // Remove the Scenario AND Sessions 
    PackerScenarioId cleanupScenarioId = packerScenario.getPackerScenarioId();
    const auto terminationReason = packerScenario.getTerminationReason();
    TRACE_LOG("[GamePackerMasterImpl].cleanupScenario: PackerScenario(" << cleanupScenarioId << "), reason(" << GameManager::PackerScenarioData::TerminationReasonToString(terminationReason) << ")");

    for (PackerSessionMaster* packerSessionMaster : packerScenario.mPackerSessionMasterList)
    {
        PackerSessionId erasePackerSessionId = packerSessionMaster->getPackerSessionId();
        auto& erasePackerSession = *packerSessionMaster;
        auto sessionResult = GameManager::PackerSessionData::FAILED;
        switch (erasePackerSession.getState())
        {
        case GameManager::PackerSessionData::SESSION_COMPLETED: sessionResult = GameManager::PackerSessionData::SUCCESS; break;
        case GameManager::PackerSessionData::SESSION_ABORTED:   sessionResult = GameManager::PackerSessionData::FAILED;  break;
        default:
            switch (terminationReason)
            {
            case GameManager::PackerScenarioData::SCENARIO_COMPLETED:  sessionResult = GameManager::PackerSessionData::ENDED;      break;
            case GameManager::PackerScenarioData::SCENARIO_TIMED_OUT:  sessionResult = GameManager::PackerSessionData::TIMED_OUT;  break;
            case GameManager::PackerScenarioData::SCENARIO_CANCELED:   sessionResult = GameManager::PackerSessionData::CANCELED;   break;
            case GameManager::PackerScenarioData::SCENARIO_OWNER_LEFT: sessionResult = GameManager::PackerSessionData::TERMINATED; break;
            case GameManager::PackerScenarioData::SCENARIO_SYSTEM_TERMINATED: sessionResult = GameManager::PackerSessionData::TERMINATED; break;
            case GameManager::PackerScenarioData::SCENARIO_FAILED:     sessionResult = GameManager::PackerSessionData::FAILED;     break;
            default:
                WARN_LOG("[GamePackerMasterImpl].cleanupScenario: PackerSession(" << erasePackerSessionId << ") in PackerScenario(" << cleanupScenarioId << ") unhandled termination reason(" << GameManager::PackerScenarioData::TerminationReasonToString(terminationReason) << ")");
            }
            break;
        }

        // Metrics:
        if (requestCompleted)
            mSessionsCompleted.increment(1, packerScenario.getScenarioName(), erasePackerSession.getSubSessionName(), sessionResult, erasePackerSession.getDurationPercentile());
        mSessionsActive.decrement(1, packerScenario.getScenarioName(), erasePackerSession.getSubSessionName());
        mUsersActive.decrement(packerScenario.getUserJoinInfoList().size(), packerScenario.getScenarioName(), erasePackerSession.getSubSessionName());

        // Remove/Delete the Session:
        mSessionByIdMap.erase(erasePackerSessionId);    // Owning map
        delete packerSessionMaster;                       // Session itself  (Cancels timers, removes intrusive list)
    }
    packerScenario.mPackerSessionMasterList.clear();        // List held by Scenario


    // Metrics:
    if (requestCompleted)
        mScenariosCompleted.increment(1, packerScenario.getScenarioName(), terminationReason, packerScenario.getDurationPercentile());
    mScenariosActive.decrement(1, packerScenario.getScenarioName());

    // Remove/Delete the Scenario:
    removeScenarioBlazeIdMapping(packerScenario);

    mPackerScenarioByIdMap.erase(cleanupScenarioId);  // Owning map
    delete &packerScenario;                             // Scenario itself  (Cancels timers)

    // Only remove the Redis entry if this was a normal completion, not a migration:
    if (requestCompleted)
        mScenarioTable.eraseRecord(cleanupScenarioId);
}

void GamePackerMasterImpl::cleanupScenarioAndSessions(PackerScenarioMaster& packerScenario)
{
    PackerScenarioId cleanupScenarioId = packerScenario.getPackerScenarioId();
    const auto terminationReason = packerScenario.getTerminationReason();
    TRACE_LOG("[GamePackerMasterImpl].cleanupScenarioAndSessions: PackerScenario(" << cleanupScenarioId << "), reason(" << GameManager::PackerScenarioData::TerminationReasonToString(terminationReason) << ")");

    if (mPackerScenarioByIdMap.find(cleanupScenarioId) == mPackerScenarioByIdMap.end())
    {
        ERR_LOG("[GamePackerMasterImpl].cleanupScenarioAndSessions: PackerScenario(" << cleanupScenarioId << "), already removed from mPackerScenarioByIdMap.");
        return;
    }

    // Submit Matchmaking and PIN Events in case of failure: 
    if (terminationReason != GameManager::PackerScenarioData::SCENARIO_COMPLETED)
    {
        auto matchmakingResult = GameManager::SESSION_ERROR_GAME_SETUP_FAILED;
        switch (terminationReason)
        {
        case GameManager::PackerScenarioData::SCENARIO_TIMED_OUT:  matchmakingResult = GameManager::SESSION_TIMED_OUT;               break;
        case GameManager::PackerScenarioData::SCENARIO_CANCELED:   matchmakingResult = GameManager::SESSION_CANCELED;                break;
        case GameManager::PackerScenarioData::SCENARIO_OWNER_LEFT: matchmakingResult = GameManager::SESSION_TERMINATED;              break;
        case GameManager::PackerScenarioData::SCENARIO_FAILED:     matchmakingResult = GameManager::SESSION_ERROR_GAME_SETUP_FAILED; break;
        default:
            WARN_LOG("[GamePackerMasterImpl].cleanupScenarioAndSessions: PackerScenario(" << cleanupScenarioId << "), unexpected result(" << GameManager::MatchmakingResultToString(matchmakingResult) << ")");
            break;
        }

        // submit matchmaking failed event, this will catch the game creator and all joining sessions.
        // PACKER_TODO  - Make a Packer version rather than reusing the old MM version. 
        GameManager::FailedMatchmakingSession failedMatchmakingSession;
        //failedMatchmakingSession.setMatchmakingSessionId(abortPackerSessionId); /// @deprecated intentionally omitted and should be removed with legacy MM
        failedMatchmakingSession.setMatchmakingScenarioId(packerScenario.getOriginatingScenarioId());
        failedMatchmakingSession.setUserSessionId(packerScenario.getOwnerUserSessionInfo().getSessionId());
        failedMatchmakingSession.setMatchmakingResult(GameManager::MatchmakingResultToString(matchmakingResult));
        gEventManager->submitEvent((uint32_t)GameManager::GameManagerMaster::EVENT_FAILEDMATCHMAKINGSESSIONEVENT, failedMatchmakingSession);

        // get any session belonging to this scenario (needed to fill out the PIN event)
        // the first session will do
        auto now = TimeValue::getTimeOfDay();
        auto duration = now - packerScenario.getStartedTimestamp();

        // get the best scoring session of the scenario in order to submit its data to PIN
        PackerSessionMaster* bestScoringSession = nullptr;
        if (packerScenario.mPackerSessionMasterList.empty())
        {
            WARN_LOG("[GamePackerMasterImpl].cleanupScenarioAndSessions: PackerScenario(" << cleanupScenarioId << "), has no sessions associated. Session related PIN data cannot be submitted.");
        }
        else
        {
            bestScoringSession = packerScenario.mPackerSessionMasterList.front();
            if (packerScenario.mPackerSessionMasterList.size() > 1)
            {
                for (auto packerSessionItr = packerScenario.mPackerSessionMasterList.begin() + 1; packerSessionItr != packerScenario.mPackerSessionMasterList.end(); packerSessionItr++)
                {
                    if ((*packerSessionItr)->getOverallScoringInfo().getGameQualityScore() > bestScoringSession->getOverallScoringInfo().getGameQualityScore())
                        bestScoringSession = *packerSessionItr;
                }
            }
        }

        // send PIN events for failed matchmaking
        for (auto& userJoinInfo : packerScenario.getUserJoinInfoList())
        {
            PINSubmissionPtr pinSubmission = BLAZE_NEW PINSubmission;
            GameManager::PINEventHelper::buildMatchJoinForFailedPackerScenario(packerScenario.getScenarioRequest().getPackerScenarioRequest(), *(packerScenario.getHostJoinInfo()),
                packerScenario.getUserJoinInfoList(), duration, matchmakingResult, userJoinInfo->getUser(), *pinSubmission);

            // submit best scoring session data to PIN
            if (bestScoringSession)
            {
                auto* pinEventList = GameManager::PINEventHelper::getUserSessionPINEventList(*pinSubmission, userJoinInfo->getUser().getSessionId());
                for (auto& pinEvent : *pinEventList)
                {
                    if (pinEvent->get()->getTdfId() != RiverPoster::MPMatchJoinEvent::TDF_ID)
                        continue;

                    auto* matchJoinEvent = static_cast<RiverPoster::MPMatchJoinEvent*>(pinEvent->get());
                    matchJoinEvent->setEstimatedTimeToMatchSeconds(mEstimatedTimeToMatch.getSec());
                    matchJoinEvent->setSessionMigrations(bestScoringSession->getMigrationCount());
                    matchJoinEvent->setSiloCount(bestScoringSession->getTotalSiloCount());
                    matchJoinEvent->setTotalUsersPotentiallyMatched(bestScoringSession->getMaxSiloPlayerCount());
                    matchJoinEvent->setEvictionCount(bestScoringSession->getTotalEvictionCount());
                    matchJoinEvent->setMaxProvisionalFitscore((uint32_t)roundf(bestScoringSession->getOverallScoringInfo().getBestGameQualityScore() * 100.0f));
                    matchJoinEvent->setMinAcceptedFitscore((uint32_t)roundf(bestScoringSession->getOverallScoringInfo().getViableQualityScore() * 100.0f));
                    bestScoringSession->getFactorDetailsList().copyInto(matchJoinEvent->getGqfDetails());
                    matchJoinEvent->setTotalUsersMatched(bestScoringSession->getMatchedPlayersInGameCount());
                }
            }

            gUserSessionManager->sendPINEvents(pinSubmission);
        }

        auto& internalRequest = packerScenario.mScenarioRequest->getPackerScenarioRequest();
        auto* pingSite = packerScenario.getHostJoinInfo()->getUser().getBestPingSiteAlias();
        auto* delineationGroup = internalRequest.getCommonGameData().getDelineationGroup();
        mTimeToMatchEstimator.updateScenarioTimeToMatch(packerScenario.getScenarioName(), duration, pingSite, delineationGroup, internalRequest.getScenarioMatchEstimateFalloffWindow(), internalRequest.getGlobalMatchEstimateFalloffWindow(), matchmakingResult);
    }

    notifyAssignedSessionsTermination(packerScenario);

    removeScenarioAndSessions(packerScenario, true);     // true == Scenario / Sessions completed:
}

void GamePackerMasterImpl::upsertSessionData(PackerSessionMaster& packerSession)
{
    StorageFieldName fieldName;
    fieldName.sprintf(PRIVATE_PACKER_SESSION_FIELD_FMT, packerSession.getPackerSessionId());
    mScenarioTable.upsertField(packerSession.getParentPackerScenarioId(), fieldName.c_str(), *packerSession.mSessionData);
}

void GamePackerMasterImpl::upsertScenarioRequest(PackerScenarioMaster& packerScenario)
{
    mScenarioTable.upsertField(packerScenario.getPackerScenarioId(), PRIVATE_PACKER_REQUEST_FIELD, *packerScenario.mScenarioRequest);
}

void GamePackerMasterImpl::upsertScenarioData(PackerScenarioMaster& packerScenario)
{
    mScenarioTable.upsertField(packerScenario.getPackerScenarioId(), PRIVATE_PACKER_SCENARIO_FIELD, *packerScenario.mScenarioData);
}

EA::TDF::TimeValue GamePackerMasterImpl::getScenarioExpiryWithGracePeriod(EA::TDF::TimeValue expiry) const
{
    return expiry + getConfig().getScenarioExpiryGracePeriod();
}

Blaze::GamePacker::GamePackerMasterImpl::LayerInfo& GamePackerMasterImpl::getLayerByWorker(const WorkerInfo& worker)
{
    return *worker.mWorkerLayerInfo;
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
    InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(session.getId());
    drainWorkerInstance(workerInstanceId);
}

WorkerDrainInstanceError::Error GamePackerMasterImpl::processWorkerDrainInstance(const ::Blaze::Message* message)
{
    InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    drainWorkerInstance(workerInstanceId);

    return WorkerDrainInstanceError::ERR_OK;
}


void GamePackerMasterImpl::drainWorkerInstance(InstanceId workerInstanceId, bool reactivateSessions)
{
    // This function is called by a worker that is draining or otherwise removed.
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        WARN_LOG("[GamePackerMasterImpl].drainWorkerInstance: InstanceId=" << workerInstanceId << ". Is not a registered worker - Ignoring.");
        return;
    }

    auto& worker = *workerItr->second;
    auto& layer = getLayerByWorker(worker);
    eastl::set<PackerSessionId> sessionIdsToReactivate;

    // WORKER REMOVAL:
    {
        // Cache the session ids (not the pointers, since we're gonna do lookups anyways to ensure handle cases where the sessions are removed while we're iterating over them.)
        // sessionIdsToReactivate.reserve(worker.mWorkingSessionList.size());
        for (auto& iter : worker.mWorkingSessionList)
        {
            sessionIdsToReactivate.insert(iter.getPackerSessionId());
        }

        // Move all the owned sessions up to the layer:  (Put in 'Other' so they don't get assigned out.)
        layer.mOtherSessionList.splice(layer.mOtherSessionList.end(), worker.mWorkingSessionList);

        // Then, we remove the worker:
        mWorkerBySlotId.erase(worker.mWorkerSlotId);
        mWorkerByInstanceId.erase(workerItr);
        delete &worker;                         // This triggers the removal from layer.mWorkerByState
    }

    // LAYER DEACTIVATION:  (We don't actually remove layers)
    // Check if the sessions on the layer need to be reactivated as well (sent to unassigned)
    if (!layer.hasWorkers()) 
    {
        for (auto& iter : layer.mIdleSessionList)
        {
            sessionIdsToReactivate.insert(iter.getPackerSessionId());
        }

        // All of the Assigned sessions need to be moved out:
        mOtherSessionList.splice(mOtherSessionList.end(), layer.mOtherSessionList);
        mOtherSessionList.splice(mOtherSessionList.end(), layer.mIdleSessionList);
    }

    // We skip reactivation in the case where a worker just obtains a new slot id.  These sessions keep their previous owner, but can be reassigned if needed. 
    if (reactivateSessions)
    {
        // Reactivate all sessions that were assigned or awaiting assignment.
        for (PackerSessionId sessionId : sessionIdsToReactivate)
        {
            auto iter = mSessionByIdMap.find(sessionId);
            if (iter == mSessionByIdMap.end())
                continue;

            // Any sessions that were assigned need to be unassigned.  (Moves sessions from 'Other' to idle)
            PackerSessionMaster& packerSession = *iter->second;
            packerSession.mSessionData->setAssignedWorkerInstanceId(INVALID_INSTANCE_ID);
            activatePackerSession(*iter->second, false);
        }
    }
}

GetPackerInstanceStatusError::Error GamePackerMasterImpl::processGetPackerInstanceStatus(Blaze::GameManager::PackerInstanceStatusResponse& response, const ::Blaze::Message* message)
{
    response.setMatchmakingSessionsCount(mSessionsActive.get());
    response.setMatchmakingUsersCount(mUsersActive.get());
    mTimeToMatchEstimator.populateMatchmakingCensusData(response.getGlobalTimeToMatchEstimateData(), response.getScenarioTimeToMatchEstimateData());

    // PACKER_TODO: Populate PMR data
    //mMatchmaker->getMatchmakingInstanceData(response.getPlayerMatchmakingRateByScenarioPingsiteGroup(), response.getScenarioMatchmakingData());

    return GetPackerInstanceStatusError::ERR_OK;
}

void GamePackerMasterImpl::onUserSessionExtinction(const UserSession& userSession)
{
    BlazeId blazeId = userSession.getBlazeId();
    UserSessionId extinctSessionId = userSession.getSessionId();
    auto scenarioIdByBlazeIdItr = mPackerScenarioIdByBlazeIdMap.find(blazeId);
    if (scenarioIdByBlazeIdItr == mPackerScenarioIdByBlazeIdMap.end())
    {
        TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, no associated scenarios");
        return;
    }
    
    // This for loop is needed since mPackerScenarioIdByBlazeIdMap is a multimap with multiple potential entries.  (Though typically we don't allow multiple matchmaking requests from the same user as that breaks other systems.)
    for (; scenarioIdByBlazeIdItr != mPackerScenarioIdByBlazeIdMap.end(); scenarioIdByBlazeIdItr = mPackerScenarioIdByBlazeIdMap.find(blazeId))
    {
        PackerScenarioId scenarioId = scenarioIdByBlazeIdItr->second;
        auto scenarioItr = mPackerScenarioByIdMap.find(scenarioId);
        if (scenarioItr == mPackerScenarioByIdMap.end())
        {
            ERR_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, expected PackerScenario(" << scenarioId << ") not found, remove user->scenario mapping");
            mPackerScenarioIdByBlazeIdMap.erase(scenarioIdByBlazeIdItr);
            continue;
        }

        auto& packerScenario = *scenarioItr->second;
        bool isScenarioOwnerLeaving = (packerScenario.getOwnerUserSessionInfo().getSessionId() == extinctSessionId);
        if (isScenarioOwnerLeaving || packerScenario.getUserJoinInfoList().size() == 1)
        {
            TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, terminate " << (isScenarioOwnerLeaving ? "explicitly" : "implicitly") << " owned PackerScenario(" << scenarioId << ")");

            // PACKER_TODO: #FUTURE We will need to treat users differently depending on how the scenario is configured when a member unexpectedly logs out. e.g: if the the scenario supports ownership transfer we may be able to issue an update for all the live sub sessions instead of aborting the owned scenario outright, and force the assigned packer slave to repack the corresponding party.

            // PACKER_TODO: ensure notification going back to client is accurate
            packerScenario.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_OWNER_LEFT);
            cleanupScenarioAndSessions(packerScenario);
        }
        else
        {

            // PACKER_TODO:  None of this code works correctly.   The Properties that depend on UserInfo need to be reacquired, and the code should be revised to update the users once on the Scenario.
            // Alternatively, a simpler bit of logic would be to cancel the current scenario, and restart the request on the Scenario side.
            // This would ensure that all of the processing (User Lookup, Input Sanitizers, External Data Sources, Property acquisition) that happens prior to Packer gets reapplied. 
            // Oh, and doing it that way would make it work for old matchmaking as well.  (Win/Win!)
#if 0
            // This handles the case of updating the scenario
            EA_ASSERT(packerScenario.getUserJoinInfoList().size() > 1);

            GameManager::UserJoinInfoList userJoinInfoList;
            userJoinInfoList.reserve(packerScenario.getUserJoinInfoList().size() - 1);

            for (auto& userJoinInfo : packerScenario.getUserJoinInfoList())
            {
                if (userJoinInfo->getUser().getSessionId() != extinctSessionId)
                {
                    userJoinInfoList.push_back(userJoinInfo); // add all remaining user infos
                }
            }

            if (packerScenario.getUserJoinInfoList().size() == userJoinInfoList.size())
            {
                TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, active PackerScenario(" << scenarioId << ") associated with BlazeId(" << blazeId << ") unaffected");
                mPackerScenarioIdByBlazeIdMap.erase(scenarioIdByBlazeIdItr);
                continue;
            }

            TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, update affected PackerScenario(" << scenarioId << ")");
            mPackerScenarioIdByBlazeIdMap.erase(scenarioIdByBlazeIdItr);
            
            // PACKER_TODO: need to move this member to PackerScenarioData so it can be backed by storage manager...
            packerScenario.getUserJoinInfoList().swap(userJoinInfoList); // update scenario info list to now reflect the remaining users

            EA_ASSERT(packerScenario.getUserJoinInfoList().size() + 1 == userJoinInfoList.size());
            
            eastl::vector<PackerSessionId> failedSessionIds;
            GameManager::NotifyWorkerPackerSessionRelinquish updated;
            for (PackerSessionMaster* PackerSessionMaster : packerScenario.mPackerSessionMasterList)
            {
                PackerSessionId updatePackerSessionId = PackerSessionMaster->getPackerSessionId();
                auto& updatePackerSession = *PackerSessionMaster;
                auto sessionState = updatePackerSession.getState();
                if (sessionState == GameManager::PackerSessionData::SESSION_ASSIGNED)
                {
                    auto assignedWorkerInstanceId = updatePackerSession.getAssignedWorkerInstanceId();
                    EA_ASSERT_MSG(assignedWorkerInstanceId != INVALID_INSTANCE_ID, "Assigned session must have a valid instance!");

                    // PACKER_TODO:  THIS CODE IS BROKEN.  This doesn't work the way the code assumes that it does.   
                    //   The Properties don't change since they were not reacquired from the UserSessions, so the underlying PackerSession is still using the full player list, including the removed user.
                    GameManager::MatchmakingCriteriaError criteriaError;
                    auto rc = initializeAggregateProperties(*updatePackerSession, criteriaError);
                    if (rc != ERR_OK)
                    {
                        ERR_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, PackerScenario(" << scenarioId << "), PackerSession(" << updatePackerSessionId << ") was updated, failed to reinitialize due to err(" << rc << "), session will be aborted");
                        failedSessionIds.push_back(updatePackerSessionId);
                        continue;
                    }

                    TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, PackerScenario(" << scenarioId << "), PackerSession(" << updatePackerSessionId << ") was updated, signal assigned worker(" << assignedWorkerInstanceId << ") to relinquish it");

                    auto& sessionUserJoinInfoList = packerScenario.getUserJoinInfoList();
                    sessionUserJoinInfoList.clear();
                    for (auto& userJoinInfo : packerScenario.getUserJoinInfoList())
                    {
                        // PACKER_TODO: once we remove this info from the session's request we will no longer have to update it this way
                        sessionUserJoinInfoList.push_back(userJoinInfo);
                    }
                    updatePackerSession.mSessionData->setState(GameManager::PackerSessionData::SESSION_AWAITING_RELEASE);
                    updatePackerSession.mSessionData->setPendingReleaseTimestamp(TimeValue::getTimeOfDay());

                    updated.setPackerSessionId(updatePackerSessionId);
                    updated.setPackerScenarioId(scenarioId);

                    // signal worker to release the session (master will reassign it to a new worker)
                    sendNotifyWorkerPackerSessionRelinquishToInstanceById(assignedWorkerInstanceId, &updated);

                    upsertSessionData(updatePackerSession);
                }
                else
                {
                    TRACE_LOG("[GamePackerMasterImpl].onUserSessionExtinction: UserSession(" << extinctSessionId << ") logged out, PackerScenario(" << scenarioId << "), PackerSession(" << updatePackerSessionId << ") refreshed, in state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ")");
                }
            }

            // This data is per-scenario now, but we don't really want to update anything about it except the Users and Properties. 
            upsertScenarioRequestData(updatePackerSession);

            for (auto failedPackerSessionId : failedSessionIds)
            {
                auto sessionItr = mSessionByIdMap.find(failedPackerSessionId);
                if (sessionItr == mSessionByIdMap.end())
                    continue;
                auto& failedPackerSession = *sessionItr->second;
                abortSession(failedPackerSession);
            }

            // PACKER_TODO: Once we move packerScenario.getUserJoinInfoList() member to PackerScenarioData so it can be backed by storage manager, we'll need to upsert the changed scenario info here...
            //upsertScenarioData(packerScenario);
#endif
        }

    }
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

const char8_t* GamePackerMasterImpl::getWorkerNameByInstanceId(InstanceId instanceId, const char8_t* unknownName) const
{
    auto itr = mWorkerByInstanceId.find(instanceId);
    if (itr != mWorkerByInstanceId.end())
        return itr->second->mWorkerName.c_str();
    return unknownName;
}

const char8_t* GamePackerMasterImpl::getSubSessionNameByPackerSessionId(PackerSessionId packerSessionId) const
{
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr != mSessionByIdMap.end())
    {
        return sessionItr->second->getSubSessionName();
    }
    return "<invalidSessionId>";
}

GamePackerMasterImpl::PackerScenarioMaster* GamePackerMasterImpl::addPackerScenario(PackerScenarioId packerScenarioId, GameManager::StartPackerScenarioRequestPtr scenarioRequest, GameManager::PackerScenarioDataPtr packerScenarioData)
{
    PackerScenarioMasterMap::insert_return_type packerScenarioRet = mPackerScenarioByIdMap.insert(packerScenarioId);
    if (packerScenarioRet.second)
    {
        packerScenarioRet.first->second = BLAZE_NEW PackerScenarioMaster(packerScenarioId, scenarioRequest, packerScenarioData);
        PackerScenarioMaster& packerScenario = (*packerScenarioRet.first->second);

        addScenarioBlazeIdMapping(packerScenario);

        EA::TDF::TimeValue expiryTimeWithGracePeriod = getScenarioExpiryWithGracePeriod(packerScenario.getExpiresTimestamp());
        packerScenario.mExpiryTimerId = gSelector->scheduleTimerCall(expiryTimeWithGracePeriod,
            this, &GamePackerMasterImpl::onScenarioExpiredTimeout, packerScenario.getPackerScenarioId(), "GamePackerMasterImpl::onScenarioExpiredTimeout");

        // Metrics:
        mScenariosActive.increment(1, packerScenario.getScenarioName());

        // If this was a new Scenario (not gather data from Redis) add some additional metrics, update redis and initialize TTM estimates:
        if (packerScenarioData == nullptr)
        {
            mScenariosStarted.increment(1, packerScenario.getScenarioName());
            upsertScenarioData(packerScenario); // Redis:

            mTimeToMatchEstimator.initializeTtmEstimatesForPackerScenario(packerScenario.getScenarioName(), packerScenario.getScenarioDuration());
        }
        return &packerScenario;
    }

    WARN_LOG("[GamePackerMasterImpl].addPackerScenario: Scenario (" << packerScenarioId << ") already exists in the PackerMaster.");
    return nullptr;
}

StartPackerScenarioError::Error GamePackerMasterImpl::processStartPackerScenario(const GameManager::StartPackerScenarioRequest &req, GameManager::StartPackerSessionResponse &response, GameManager::StartPackerSessionError &error, const ::Blaze::Message* message)
{
    PackerScenarioId packerScenarioId = req.getPackerScenarioId();
    SliverId sliverId = GetSliverIdFromSliverKey(packerScenarioId);
    OwnedSliver* ownedSliver = mScenarioTable.getSliverOwner().getOwnedSliver(sliverId);
    if (ownedSliver == nullptr)
    {
        ASSERT_LOG("[GamePackerMasterImpl].processStartPackerScenario: PackerScenario(" << packerScenarioId << ") could not obtain owned sliver for SliverId(" << sliverId << ")");
        return StartPackerScenarioError::ERR_SYSTEM;
    }

    Sliver::AccessRef sliverAccess;
    if (!ownedSliver->getPrioritySliverAccess(sliverAccess))
    {
        ASSERT_LOG("[GamePackerMasterImpl].processStartPackerScenario: PackerScenario(" << packerScenarioId << ") could not get priority sliver access for SliverId(" << ownedSliver->getSliverId() << ")");
        return StartPackerScenarioError::ERR_SYSTEM;
    }

    if (mPackerScenarioByIdMap.find(packerScenarioId) != mPackerScenarioByIdMap.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processStartPackerScenario: PackerScenario(" << packerScenarioId << ") already exists on SliverId(" << ownedSliver->getSliverId() << ").  Check why id is being reused.");
        return StartPackerScenarioError::ERR_SYSTEM;
    }

    // Add the Scenario: 
    PackerScenarioMaster* packerScenario = addPackerScenario(packerScenarioId, req.clone());

    // Add the Sessions:
    for (auto& sessionData : packerScenario->getScenarioRequest().getPackerSessionRequestList())
    {
        auto packerSessionId = sessionData->getPackerSessionId();
        PackerSessionMaster* packerSessionMaster = addPackerSession(packerSessionId, *packerScenario);
        if (packerSessionMaster == nullptr)
        {
            ERR_LOG("[GamePackerMasterImpl].processStartPackerScenario: Failed to create PackerSession(" << packerSessionId << "), unexpected id collision");
            return StartPackerScenarioError::ERR_SYSTEM;
        }

        // Update the Properties:  
        BlazeError rc = initializeAggregateProperties(*packerSessionMaster, error.getInternalError());
        if (rc != ERR_OK)
        {
            ERR_LOG("[GamePackerMasterImpl].processStartPackerScenario: Failed to setup PackerSession(" << packerSessionId << "), err(" << rc << ".");
            return StartPackerScenarioError::commandErrorFromBlazeError(rc.code);
        }

        TRACE_LOG("[GamePackerMasterImpl].processStartPackerScenario: Created PackerSession(" << packerSessionId << "), PackerScenario(" << req.getPackerScenarioId() << ")");
    }

    // We only upsert the Request once we have updated all of the Properties in the Session startup:
    upsertScenarioRequest(*packerScenario);

    TRACE_LOG("[GamePackerMasterImpl].processStartPackerScenario: PackerScenario(" << packerScenarioId << ") created all sessions(" << packerScenario->mPackerSessionMasterList.size() << "), assign sessions to workers");

    // After upserting the ScenarioRequest, we go through the sessions and either signal for immediate assignment, or delay if needed:
    activatePackerScenario(*packerScenario);

    auto& internalRequest = req.getPackerScenarioRequest();
    auto* pingSite = packerScenario->getHostJoinInfo()->getUser().getBestPingSiteAlias();
    auto* delineationGroup = internalRequest.getCommonGameData().getDelineationGroup();
    mEstimatedTimeToMatch = mTimeToMatchEstimator.getTimeToMatchEstimateForPacker(packerScenario->getScenarioName(), packerScenario->getScenarioDuration(), pingSite, delineationGroup, internalRequest.getScenarioMatchEstimateFalloffWindow(), internalRequest.getGlobalMatchEstimateFalloffWindow());
    
    return StartPackerScenarioError::ERR_OK;
}

GamePackerMasterImpl::PackerSessionMaster* GamePackerMasterImpl::addPackerSession(PackerSessionId packerSessionId, GamePackerMasterImpl::PackerScenarioMaster& packerScenario, GameManager::PackerSessionDataPtr packerSessionData)
{
    PackerSessionMap::insert_return_type packerSessionRet = mSessionByIdMap.insert(packerSessionId);
    if (packerSessionRet.second)
    {
        packerSessionRet.first->second = BLAZE_NEW PackerSessionMaster(packerSessionId, packerScenario, packerSessionData);
        PackerSessionMaster& packerSession = (*packerSessionRet.first->second);

        // Metrics:
        mSessionsActive.increment(1, packerScenario.getScenarioName(), packerSession.getSubSessionName());
        mUsersActive.increment(packerScenario.getUserJoinInfoList().size(), packerScenario.getScenarioName(), packerSession.getSubSessionName());

        // If this was a new Scenario (not gather data from Redis) add some additional metrics, and update redis:
        if (packerSessionData == nullptr)
        {
            // If the session is delayed, note that before setting the Redis data: 
            if (packerSession.getStartDelay() != 0)
            {
                packerSession.mSessionData->setState(GameManager::PackerSessionData::SESSION_DELAYED);
            }

            mSessionsStarted.increment(1, packerScenario.getScenarioName(), packerSession.getSubSessionName());
            upsertSessionData(packerSession); // Redis:
        }
        return &packerSession;
    }

    WARN_LOG("[GamePackerMasterImpl].addPackerSession: PackerSession(" << packerSessionData->getPackerSessionId() << ") already exists in the PackerMaster.");
    return nullptr;
}

void GamePackerMasterImpl::startDelayedSession(PackerSessionId sessionId)
{
    auto sessionItr = mSessionByIdMap.find(sessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].startDelayedSession: PackerSession(" << sessionId << ") has disappeared");
        return;
    }

    auto& packerSession = *sessionItr->second;

    TRACE_LOG("[GamePackerMasterImpl].startDelayedSession: PackerSession(" << sessionId << "), PackerScenario(" << packerSession.getParentPackerScenarioId() << ") started after "
        << (TimeValue::getTimeOfDay() - packerSession.getStartedTimestamp()).getMillis() << "ms delay (configured for " << packerSession.getStartDelay().getMillis() << "ms)");

    packerSession.mDelayedStartTimerId = INVALID_TIMER_ID;
    packerSession.mSessionData->setState(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);

    signalSessionAssignment(packerSession);
    upsertSessionData(packerSession);
}

CancelPackerScenarioError::Error GamePackerMasterImpl::processCancelPackerScenario(const Blaze::GameManager::CancelPackerScenarioRequest &request, const ::Blaze::Message* message)
{
    PackerScenarioId packerScenarioId = request.getPackerScenarioId();
    auto scenarioItr = mPackerScenarioByIdMap.find(packerScenarioId);
    if (scenarioItr == mPackerScenarioByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].processCancelPackerScenario: Attempted to cancel PackerScenario(" << packerScenarioId << ") when no such PackerScenario exists.");
        return CancelPackerScenarioError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& packerScenario = *scenarioItr->second;
    PackerSessionId lockingSessionId = packerScenario.getLockedForFinalizationByPackerSessionId();
    if (lockingSessionId != INVALID_PACKER_SESSION_ID)
    {
        // Finalization is occurring, so we assume that finalization will succeed.  (If not, the user can just hit cancel again.)
        // PACKER_TODO: the cancel should be respected if finalization fails. 
        WARN_LOG("[GamePackerMasterImpl].processCancelPackerScenario: PackerScenario(" << packerScenarioId << "), cannot be canceled while locked for finalization by PackerSessionId(" << lockingSessionId << ").  Waiting for completion.");
        return CancelPackerScenarioError::GAMEPACKER_ERR_SCENARIO_FINALIZING;
    }

    for (PackerSessionMaster* sessionItr : packerScenario.mPackerSessionMasterList)
    {
        auto& packerSession = *sessionItr;
        PackerSessionId packerSessionId = packerSession.getPackerSessionId();
        auto assignedInstanceId = packerSession.getAssignedWorkerInstanceId();
        auto* printableWorkerName = getWorkerNameByInstanceId(assignedInstanceId);
        if (assignedInstanceId != INVALID_INSTANCE_ID)
        {
            TRACE_LOG("[GamePackerMasterImpl].processCancelPackerScenario: PackerScenario(" << packerScenarioId << "), PackerSession(" << packerSessionId << ") assigned to worker(" << printableWorkerName << "), sessionData(" << *packerSession.mSessionData << ") canceled before completion");
        }

        // PACKER_TODO: figure out if canceling a session in the future, canceling the scenario) should generate a notification still
        sendMatchmakingFailedNotification(packerSession, GameManager::SESSION_CANCELED);
    }
    packerScenario.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_CANCELED);
    cleanupScenarioAndSessions(packerScenario); // PACKER_TODO: first cancel of subsession currently always cancels the whole scenario, because coreslave always tries to cancel the rest afterwards and that is the intent anyway, and those are all no-ops. In the future this RPC will be renamed cancelScenario and called only once!

    return CancelPackerScenarioError::ERR_OK;
}

void GamePackerMasterImpl::sendMatchmakingFailedNotification(const GamePackerMasterImpl::PackerSessionMaster& packerSession, GameManager::MatchmakingResult failureReason)
{
    auto now = TimeValue::getTimeOfDay();
    auto duration = now - packerSession.getStartedTimestamp();
    auto hostUserSessionId = packerSession.getHostJoinInfo()->getUser().getSessionId();

    GameManager::NotifyMatchmakingFailed notifyFailed;
    notifyFailed.setUserSessionId(hostUserSessionId);
    notifyFailed.setSessionId(packerSession.getPackerSessionId());
    notifyFailed.setScenarioId(packerSession.getOriginatingScenarioId());
    notifyFailed.setMatchmakingResult(failureReason);
    notifyFailed.setMaxPossibleFitScore(100);
    notifyFailed.getSessionDuration().setSeconds(duration.getSec());

    sendNotifyMatchmakingFailedToSliver(UserSession::makeSliverIdentity(hostUserSessionId), &notifyFailed);
}

WorkerObtainPackerSessionError::Error GamePackerMasterImpl::processWorkerObtainPackerSession(const Blaze::GameManager::WorkerObtainPackerSessionRequest &request, Blaze::GameManager::WorkerObtainPackerSessionResponse &response, const ::Blaze::Message* message)
{
    InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerObtainPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerObtainPackerSessionError::GAMEPACKER_ERR_WORKER_NOT_FOUND;
    }

    auto& worker = *workerItr->second;
    auto& layer = getLayerByWorker(worker);
    auto& fromList = layer.mIdleSessionList.empty() ? mUnassignedSessionList : layer.mIdleSessionList;

    if (fromList.empty())
    {
        worker.setWorkerState(WorkerInfo::IDLE);
    }
    else
    {
        // Update the State of the session and worker:  (to WORKING)
        auto& packerSession = fromList.front();
        fromList.pop_front();
        worker.assignWorkerSession(packerSession);


        // Update the Session Data (to be sent in response & Redis)
        packerSession.workerClaimSession(worker);

        // Update Redis:
        auto& ownerUserPackerSession = packerSession.getPackerScenarioMaster().getOwnerUserSessionInfo();
        LogContextOverride logAudit(ownerUserPackerSession.getSessionId());
        upsertSessionData(packerSession);


        // Update the Response:
        packerSession.getPackerScenarioMaster().getScenarioRequest().getPackerScenarioRequest().copyInto(response.getPackerScenarioRequest());
        packerSession.mSessionRequest->copyInto(response.getPackerSessionRequest());
        response.setExpiryDeadline(packerSession.getStartedTimestamp() + packerSession.getPackerScenarioMaster().getScenarioDuration());
        response.setPackerScenarioId(packerSession.getParentPackerScenarioId());
        response.setVersion(0); // PACKER_TODO: add a version of the session that we can use for efficient resync in the future
    }
    
    return WorkerObtainPackerSessionError::ERR_OK;
}

WorkerMigratePackerSessionError::Error GamePackerMasterImpl::processWorkerMigratePackerSession(const Blaze::GameManager::WorkerMigratePackerSessionRequest &request, const ::Blaze::Message* message)
{
    // When the session is released, the worker slave has given up, and the session needs to be propagated to the next layer
    // The propagation is always going towards layer 0 which is the root layer that must only have a single worker assigned.
    // The layer hierarchy is a binary heap whereby the number of nodes on successive layers is always x2 the number of nodes on the previous layers.
    const InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerMigratePackerSessionError::GAMEPACKER_ERR_WORKER_NOT_FOUND;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to migrate unknown PackerSession(" << packerSessionId << ")");
        return WorkerMigratePackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& packerSession = *sessionItr->second;
    auto& packerScenario = packerSession.getPackerScenarioMaster();
    auto& ownerUserPackerSession = packerScenario.getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserPackerSession.getSessionId());

    auto& sessionData = *packerSession.mSessionData;
    auto sessionState = sessionData.getState();
    const auto assignedInstanceId = packerSession.getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to migrate PackerSession(" << packerSessionId << ") in state(" << packerSessionId << ") currently assigned to different worker(" <<  getWorkerNameByInstanceId(assignedInstanceId) << "), request ignored");
        return WorkerMigratePackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    bool validState = (sessionState == GameManager::PackerSessionData::SESSION_ASSIGNED) || (sessionState == GameManager::PackerSessionData::SESSION_AWAITING_RELEASE);
    if (!validState)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to migrate PackerSession(" << packerSessionId << ") in unexpected state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ") on layer(" << sessionData.getAssignedLayerId() << "), request ignored");
        return WorkerMigratePackerSessionError::GAMEPACKER_ERR_SESSION_INVALID_STATE;
    }

    auto lockedForFinalizationBySessionId = packerScenario.getLockedForFinalizationByPackerSessionId();
    if (lockedForFinalizationBySessionId != INVALID_PACKER_SESSION_ID)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to migrate PackerSession(" << packerSessionId << ") while locked for finalization by PackerSession(" << lockedForFinalizationBySessionId << ") on layer(" << sessionData.getAssignedLayerId() << "), request ignored");
        return WorkerMigratePackerSessionError::GAMEPACKER_ERR_SCENARIO_FINALIZING;
    }

    auto now = TimeValue::getTimeOfDay();
    auto assignedLayerId = sessionData.getAssignedLayerId();
    if (assignedLayerId > 1)
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") released PackerSession(" << packerSessionId << "), session remaining lifetime(" << packerSession.getRemainingLifetime(now).getMillis() << "ms) on layer(" << assignedLayerId << "), will migrate to layer(" << sessionData.getAssignedLayerId() << ")"); 

        // session has been un-assigned from current worker, we'll need to re-assign it out
        packerSession.releaseSessionFromWorker(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
        assignSessionToLayer(packerSession, assignedLayerId - 1); // takes care of calling upsertSessionData()

        sessionData.setMigrationCount(sessionData.getMigrationCount() + 1);
        return WorkerMigratePackerSessionError::ERR_OK;
    }
    else
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerMigratePackerSession: worker(" << worker.mWorkerName << ") attempted to release PackerSession(" << packerSessionId << "), session remaining lifetime(" << packerSession.getRemainingLifetime(now).getMillis() << "ms) on layer(" << assignedLayerId << "), root layer not allowed to migrate sessions, session aborted");

        abortSession(packerSession);
        return WorkerMigratePackerSessionError::GAMEPACKER_ERR_LAYER_FAILURE;
    }
}

WorkerRelinquishPackerSessionError::Error GamePackerMasterImpl::processWorkerRelinquishPackerSession(const Blaze::GameManager::WorkerRelinquishPackerSessionRequest &request, const ::Blaze::Message* message)
{
    // When the session is released, release was requested due to owner triggered update and the session should remain on the same layer.
    // The propagation is always going towards layer 0 which is the root layer that must only have a single worker assigned.
    // The layer hierarchy is a binary heap whereby the number of nodes on successive layers is always x2 the number of nodes on the previous layers.
    InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerRelinquishPackerSessionError::GAMEPACKER_ERR_WORKER_NOT_FOUND;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: worker(" << worker.mWorkerName << ") attempted to release unknown PackerSession(" << packerSessionId << ")");
        return WorkerRelinquishPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& packerSession = *sessionItr->second;
    auto& packerScenario = packerSession.getPackerScenarioMaster();
    auto& ownerUserPackerSession = packerScenario.getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserPackerSession.getSessionId());

    auto& sessionData = *packerSession.mSessionData;
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
        TRACE_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: worker(" << worker.mWorkerName << ") released PackerSession(" << packerSessionId << ") due to owner triggered update, remaining lifetime(" << packerSession.getRemainingLifetime(now).getMillis() << "ms) on layer(" << assignedLayerId << "), will reassign to worker on same layer");

        // session has been un-assigned from current worker, we'll need to re-assign it out
        packerSession.releaseSessionFromWorker(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
        assignSessionToLayer(packerSession, assignedLayerId); // takes care of calling upsertSessionData()
        
        return WorkerRelinquishPackerSessionError::ERR_OK;
    }
    else
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerRelinquishPackerSession: worker(" << worker.mWorkerName << ") attempted to release PackerSession(" << packerSessionId << ") in unexpected state(" << GameManager::PackerSessionData::SessionStateToString(sessionState) << ") on layer(" << sessionData.getAssignedLayerId() << "), ignored");
        return WorkerRelinquishPackerSessionError::GAMEPACKER_ERR_SESSION_INVALID_STATE;
    }
}

WorkerUpdatePackerSessionStatusError::Error GamePackerMasterImpl::processWorkerUpdatePackerSessionStatus(const Blaze::GameManager::WorkerUpdatePackerSessionStatusRequest &request, const ::Blaze::Message* message)
{
    InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerUpdatePackerSessionStatus: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerUpdatePackerSessionStatusError::GAMEPACKER_ERR_WORKER_NOT_FOUND;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        return WorkerUpdatePackerSessionStatusError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& packerSession = *sessionItr->second;
    auto& packerScenario = packerSession.getPackerScenarioMaster();
    auto& ownerUserPackerSession = packerScenario.getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserPackerSession.getSessionId());

    auto& pinData = request.getPinData();
    auto& requestScoringInfo = pinData.getOverallScoringInfo();
    auto& overallScoringInfo = packerSession.getOverallScoringInfo();

    auto lockedForFinalizationByPackerSessionId = packerScenario.getLockedForFinalizationByPackerSessionId();
    auto lockedForFinalization = (lockedForFinalizationByPackerSessionId != INVALID_PACKER_SESSION_ID) && (lockedForFinalizationByPackerSessionId != packerSessionId);
    auto now = TimeValue::getTimeOfDay();
    if (lockedForFinalization)
    {
        // PACKER_TODO:  If this check fails, it's a critical failure - but it's too late to handle safely.   We should ensure that the mSessionByIdMap can never lose the finalizing sessions, and then remove redundant checks like this. 
        auto siblingSessionItr = mSessionByIdMap.find(lockedForFinalizationByPackerSessionId);
        if (siblingSessionItr == mSessionByIdMap.end())
        {
            ERR_LOG("[GamePackerMasterImpl].processWorkerUpdatePackerSessionStatus: worker(" << worker.mWorkerName << ") failed to update PackerSession(" << packerSessionId << ":" << packerSession.getSubSessionName() << ") with game score(" 
                << requestScoringInfo.getGameQualityScore() << ") because sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ") locking the parent PackerScenario(" << packerSession.getParentPackerScenarioId() << ") for finalization was not found.");
            return WorkerUpdatePackerSessionStatusError::ERR_SYSTEM;
        }
        auto& siblingPackerSession = *siblingSessionItr->second;
        TRACE_LOG("[GamePackerMasterImpl].processWorkerUpdatePackerSessionStatus: worker(" << worker.mWorkerName << ") attempt to update PackerSession(" << packerSessionId << ":" << packerSession.getSubSessionName() << ") with game score(" 
            << requestScoringInfo.getGameQualityScore() << ") while parent PackerScenario(" << packerSession.getParentPackerScenarioId() << ") is locked for finalization by sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ":"
            << siblingPackerSession.getSubSessionName() << ") with game score(" << siblingPackerSession.getOverallScoringInfo().getGameQualityScore() << "), ignoring update");
    }
    else
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerUpdatePackerSessionStatus: worker(" << worker.mWorkerName << ") updated PackerSession(" << packerSessionId << ":" << packerSession.getSubSessionName() << ") game score(" << overallScoringInfo.getGameQualityScore() << ") -> score(" << requestScoringInfo.getGameQualityScore() << ")");

        auto* hostJoinInfo = packerSession.getHostJoinInfo();
        EA_ASSERT(hostJoinInfo != nullptr);
        auto hostUserSessionId = hostJoinInfo->getUser().getSessionId();
        auto originatingScenarioId = packerSession.getOriginatingScenarioId();

        GameManager::NotifyMatchmakingAsyncStatus notifyMatchmakingStatus;
        notifyMatchmakingStatus.setInstanceId(gController->getInstanceId());
        notifyMatchmakingStatus.setMatchmakingSessionId(request.getPackerSessionId());
        notifyMatchmakingStatus.setMatchmakingScenarioId(originatingScenarioId);

        notifyMatchmakingStatus.setUserSessionId(hostUserSessionId);
        auto age = (now - packerSession.getStartedTimestamp()); // PACKER_TODO: Round to nearest second
        notifyMatchmakingStatus.setMatchmakingSessionAge((uint32_t) age.getSec());
        auto& packerAsyncStatus = notifyMatchmakingStatus.getPackerStatus();
        requestScoringInfo.getGameFactorScores().copyInto(packerAsyncStatus.getQualityFactorScores());
        packerAsyncStatus.setOverallQualityFactorScore(requestScoringInfo.getGameQualityScore() * 100.0f);
        auto sliverIdentity = UserSession::makeSliverIdentity(hostUserSessionId);
        // PACKER_MAYBE: we may be able to get away with fewer dependencies on MM header files if we rely only on the generated lower level stubs that define the actual types.
        //sendNotificationToSliver(Matchmaker::MatchmakerSlave::NOTIFICATION_INFO_NOTIFYMATCHMAKINGASYNCSTATUS, sliverIdentity, &notifyMatchmakingStatus, ::Blaze::NotificationSendOpts());
        sendNotifyMatchmakingAsyncStatusToSliver(sliverIdentity, &notifyMatchmakingStatus);
    }

    packerSession.updatePackerSession(pinData);

    // PACKER_TODO: store the last time the score was updated
    packerSession.mSessionData->setUpdatedStatusTimestamp(now);
    if (overallScoringInfo.getGameQualityScore() < requestScoringInfo.getGameQualityScore())
        packerSession.mSessionData->setImprovedScoreTimestamp(now);

    upsertSessionData(packerSession);

    return WorkerUpdatePackerSessionStatusError::ERR_OK;
}

WorkerAbortPackerSessionError::Error GamePackerMasterImpl::processWorkerAbortPackerSession(const Blaze::GameManager::WorkerAbortPackerSessionRequest &request, const ::Blaze::Message* message)
{
    InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerAbortPackerSessionError::GAMEPACKER_ERR_WORKER_NOT_FOUND;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: worker(" << worker.mWorkerName << ") attempted to abort non-existent PackerSession(" << packerSessionId << ")");
        return WorkerAbortPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND; // PACKER_TODO: figure out if we need to return ERR_OK if benign...
    }

    auto& packerSession = *sessionItr->second;
    auto& packerScenario = packerSession.getPackerScenarioMaster();
    auto& ownerUserPackerSession = packerScenario.getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserPackerSession.getSessionId());

    const auto assignedInstanceId = packerSession.getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: worker(" << worker.mWorkerName << ") attempted to abort PackerSession(" << packerSessionId << ") which is currently assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerAbortPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto lockedForFinalizationByPackerSessionId = packerScenario.getLockedForFinalizationByPackerSessionId();
    if ((lockedForFinalizationByPackerSessionId != INVALID_PACKER_SESSION_ID) && 
        (lockedForFinalizationByPackerSessionId != packerSessionId))
    {
        // Worker should only be attempting to release sessions that it locked itself
        ERR_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: worker(" << worker.mWorkerName << ") attempted to abort PackerSession(" << packerSessionId << ":" << packerSession.getSubSessionName() << ") while PackerScenario(" << packerSession.getParentPackerScenarioId() << ") is currently locked for finalization by sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << getSubSessionNameByPackerSessionId(lockedForFinalizationByPackerSessionId) << ")");
        return WorkerAbortPackerSessionError::GAMEPACKER_ERR_SCENARIO_FINALIZING;
    }

    WARN_LOG("[GamePackerMasterImpl].processWorkerAbortPackerSession: worker(" << worker.mWorkerName << ") aborted PackerSession(" << packerSessionId << "), sessionData(" << *packerSession.mSessionData << "), sessionRequest(" << *packerSession.mSessionRequest << ")");

    abortSession(packerSession);

    return WorkerAbortPackerSessionError::ERR_OK;
}

// IDEA: we can change the protocol to ensure that the verbs the worker slave can perform are: acquire/release/fail/complete
WorkerCompletePackerSessionError::Error GamePackerMasterImpl::processWorkerCompletePackerSession(const Blaze::GameManager::WorkerCompletePackerSessionRequest &request, const ::Blaze::Message* message)
{
    InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerCompletePackerSessionError::GAMEPACKER_ERR_WORKER_NOT_FOUND;
    }

    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") attempted to complete non-existent PackerSession(" << packerSessionId << ")");
        return WorkerCompletePackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND; // PACKER_TODO: figure out if we need to return ERR_OK if benign...
    }

    auto& packerSession = *sessionItr->second;
    auto& packerScenario = packerSession.getPackerScenarioMaster();
    auto& ownerUserPackerSession = packerScenario.getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserPackerSession.getSessionId());

    const auto assignedInstanceId = packerSession.getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") attempted to complete PackerSession(" << packerSessionId << ") which is currently assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerCompletePackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto lockedForFinalizationByPackerSessionId = packerScenario.getLockedForFinalizationByPackerSessionId();
    if ((lockedForFinalizationByPackerSessionId != INVALID_PACKER_SESSION_ID) &&
        (lockedForFinalizationByPackerSessionId != packerSessionId))
    {
        // Worker should only be attempting to release sessions that it locked itself
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") attempted to complete PackerSession(" << packerSessionId << ":" << packerSession.getSubSessionName() << ") while PackerScenario(" << packerSession.getParentPackerScenarioId() << ") is currently locked for finalization by sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << getSubSessionNameByPackerSessionId(lockedForFinalizationByPackerSessionId) << ")");
        return WorkerCompletePackerSessionError::GAMEPACKER_ERR_SCENARIO_FINALIZING;
    }

    if (request.getGameId() == GameManager::INVALID_GAME_ID)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") attempted to complete PackerSession(" << packerSessionId << ") with INVALID_GAME_ID");
        return WorkerCompletePackerSessionError::ERR_SYSTEM;
    }

    auto* hostJoinInfo = packerSession.getHostJoinInfo();
    EA_ASSERT(hostJoinInfo != nullptr);
    auto hostUserSessionId = hostJoinInfo->getUser().getSessionId(); // NOTE: Host and owner session *may* not be the same
    auto originatingScenarioId = packerSession.getOriginatingScenarioId();
    auto completionResult = (packerSessionId == request.getInitiatorPackerSessionId()) ? GameManager::SUCCESS_CREATED_GAME : GameManager::SUCCESS_JOINED_NEW_GAME;

    TRACE_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") completed PackerSession(" << packerSessionId << "), PackerScenario(" << originatingScenarioId << ") with result(" << GameManager::MatchmakingResultToString(completionResult) << "), game(" << request.getGameId() << ")");

    const auto networkTopology = packerSession.mSessionRequest->getGameCreationData().getNetworkTopology();
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
    for (auto& userJoinInfo : packerScenario.getUserJoinInfoList())
    {
        auto& userPackerSession = userJoinInfo->getUser();
        if (userJoinInfo->getIsOptional())
        {
            if (userPackerSession.getHasExternalSessionJoinPermission())
            {
                UserInfo::filloutUserIdentification(userPackerSession.getUserInfo(), *externalReservedUsers.pull_back());
            }
        }
        else
        {
            // NOTE: Currently this notification is required for the scenario manager (on core slave) to correctly clean up a successful matchmaking scenario.
            // This is irrespective of whether the network topology actually requires QoS validation, and can be thought of as a 'connection validated, or no validation was required' notification.
            auto userSessionId = userPackerSession.getSessionId();
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
        notifyFinished.setMatchmakingGameId(request.getGameId());
        sendNotifyMatchmakingFinishedToSliver(UserSession::makeSliverIdentity(hostUserSessionId), &notifyFinished);
    }

    auto& pinData = request.getPinData();

    {
        // submit matchmaking succeeded event, this will catch the game creator and all joining sessions.
        GameManager::SuccesfulMatchmakingSession succesfulMatchmakingSession;
        //succesfulMatchmakingSession.setMatchmakingSessionId(packerSessionId); /// @deprecated intentionally omitted and should be removed with legacy MM
        succesfulMatchmakingSession.setMatchmakingScenarioId(originatingScenarioId);
        succesfulMatchmakingSession.setUserSessionId(ownerUserPackerSession.getSessionId());
        succesfulMatchmakingSession.setPersonaName(ownerUserPackerSession.getUserInfo().getPersonaName());
        succesfulMatchmakingSession.setPersonaNamespace(ownerUserPackerSession.getUserInfo().getPersonaNamespace());
        succesfulMatchmakingSession.setMatchmakingResult(MatchmakingResultToString(completionResult));
        succesfulMatchmakingSession.setFitScore((uint32_t)roundf(pinData.getOverallScoringInfo().getGameQualityScore() * 100.0f));
        succesfulMatchmakingSession.setMaxPossibleFitScore(100);
        succesfulMatchmakingSession.setGameId(request.getGameId());
        gEventManager->submitEvent((uint32_t)GameManager::GameManagerMaster::EVENT_SUCCESFULMATCHMAKINGSESSIONEVENT, succesfulMatchmakingSession);
    }

    const auto now = TimeValue::getTimeOfDay();
    const auto duration = now - packerScenario.getStartedTimestamp();
    auto& requestInfo = packerScenario.getScenarioRequest().getPackerScenarioRequest();

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
        request.getPinSubmission().copyInto(*outgoingPinSubmission, visitOptions);
        packerSession.updatePackerSession(pinData);

        // Update any completion events to note the real time it took to complete:
        for (auto& userJoinInfo : packerScenario.getUserJoinInfoList())
        {
            auto pinEventsMapItr = outgoingPinSubmission->getEventsMap().find(userJoinInfo->getUser().getSessionId());
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
                    matchJoinEvent->setEstimatedTimeToMatchSeconds(mEstimatedTimeToMatch.getSec());
                    matchJoinEvent->setTotalUsersOnline(requestInfo.getTotalUsersOnline());
                    matchJoinEvent->setTotalUsersInGame(requestInfo.getTotalUsersInGame());
                    matchJoinEvent->setTotalUsersInMatchmaking(requestInfo.getTotalUsersInMatchmaking());
                    matchJoinEvent->setSessionMigrations(packerSession.getMigrationCount());
                    matchJoinEvent->setSiloCount(packerSession.getTotalSiloCount());
                    matchJoinEvent->setTotalUsersPotentiallyMatched(packerSession.getMaxSiloPlayerCount());
                    matchJoinEvent->setEvictionCount(packerSession.getTotalEvictionCount());
                    matchJoinEvent->setMaxProvisionalFitscore((uint32_t)roundf(packerSession.getOverallScoringInfo().getBestGameQualityScore() * 100.0f));
                    matchJoinEvent->setMinAcceptedFitscore((uint32_t)roundf(packerSession.getOverallScoringInfo().getViableQualityScore() * 100.0f));
                    packerSession.getFactorDetailsList().copyInto(matchJoinEvent->getGqfDetails());
                    matchJoinEvent->setTotalUsersMatched(packerSession.getMatchedPlayersInGameCount());
                }
            }
        }

        gUserSessionManager->sendPINEvents(outgoingPinSubmission);
    }

    auto* pingSite = hostJoinInfo->getUser().getBestPingSiteAlias();
    auto* delineationGroup = requestInfo.getCommonGameData().getDelineationGroup();
    mTimeToMatchEstimator.updateScenarioTimeToMatch(packerScenario.getScenarioName(), duration, pingSite, delineationGroup, requestInfo.getScenarioMatchEstimateFalloffWindow(), requestInfo.getGlobalMatchEstimateFalloffWindow(), completionResult);

    packerSession.releaseSessionFromWorker(GameManager::PackerSessionData::SESSION_COMPLETED);

    // We do not remove the FinalizationLock - since we want to ensure that no other Sessions can attempt finalization before the cleanup occurs. 
    packerScenario.cancelExpiryTimer();
    packerScenario.cancelFinalizationTimer();

    TRACE_LOG("[GamePackerMasterImpl].processWorkerCompletePackerSession: worker(" << worker.mWorkerName << ") completed PackerSession(" << packerSessionId << "), PackerScenario(" << originatingScenarioId << ") finished, total_duration(" << duration.getMillis() << "ms), finalization_duration(" << (now - packerSession.getPackerScenarioMaster().getLockedTimestamp()).getMillis() << "ms)");

    packerScenario.mScenarioData->setTerminationReason(GameManager::PackerScenarioData::SCENARIO_COMPLETED);
    cleanupScenarioAndSessions(*packerSession.mParentPackerScenario);

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
        return WorkerLockPackerSessionError::GAMEPACKER_ERR_WORKER_NOT_FOUND;
    }
    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        return WorkerLockPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& packerSession = *sessionItr->second;
    auto& packerScenario = packerSession.getPackerScenarioMaster();
    auto& ownerUserPackerSession = packerScenario.getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserPackerSession.getSessionId());

    const auto assignedInstanceId = packerSession.getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: worker(" << worker.mWorkerName << ") attempted to lock PackerSession(" << packerSessionId << ") currently assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerLockPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto lockedForFinalizationByPackerSessionId = packerScenario.getLockedForFinalizationByPackerSessionId();
    auto now = TimeValue::getTimeOfDay();
    if (lockedForFinalizationByPackerSessionId == INVALID_PACKER_SESSION_ID)
    {
        EA_ASSERT(packerScenario.mFinalizationTimerId == INVALID_TIMER_ID);
        auto& gameTemplatesMap = getConfig().getCreateGameTemplatesConfig().getCreateGameTemplates();
        auto gameTemplateItr = gameTemplatesMap.find(packerSession.getCreateGameTemplateName());
        EA_ASSERT(gameTemplateItr != gameTemplatesMap.end()); // PACKER_TODO: ensure we always validate game template info associated with each packer session, altertnative it may even be good to just link it directly via tdf_ptr...
        const auto& packerConfig = gameTemplateItr->second->getPackerConfig();
        const auto finalizationDuration = eastl::max_alt(packerConfig.getPauseDurationAfterAcquireFinalizationLocks(), packerConfig.getFinalizationDuration());
        const auto finalizationDeadline = now + finalizationDuration; // PACKER_TODO: Need to disable scenario timeout when finalization will exceed it to avoid aborting finalization too early.
        packerScenario.mScenarioData->setLockedForFinalizationByPackerSessionId(packerSessionId);
        packerScenario.mScenarioData->setLockedTimestamp(now);
        packerScenario.mFinalizationTimerId = gSelector->scheduleTimerCall(finalizationDeadline, this, &GamePackerMasterImpl::onScenarioFinalizationTimeout, packerSession.getParentPackerScenarioId(), "GamePackerMasterImpl::onScenarioFinalizationTimeout");

        upsertScenarioData(*packerSession.mParentPackerScenario);

        return GamePacker::WorkerLockPackerSessionError::ERR_OK;
    }
    else if (lockedForFinalizationByPackerSessionId == packerSessionId)
    {
        // This should not happen.  It indicates that the PackerSlave didn't bother to unlock the Session after a previous Lock.  (Returning ERR_OK only because it shouldn't hurt anything else, though it indicates an underlying problem.)
        WARN_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: PackerSession(" << packerSessionId << ":" << packerSession.getSubSessionName() << ") on worker(" << worker.mWorkerName << ") attempted to lock PackerScenario(" << packerSession.getParentPackerScenarioId() << ") already locked by this session for(" << (now - packerSession.getPackerScenarioMaster().getLockedTimestamp()).getMillis() << "ms), no-op");
        return GamePacker::WorkerLockPackerSessionError::ERR_OK;
    }
    else
    {
        TRACE_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: PackerSession(" << packerSessionId << ":" << packerSession.getSubSessionName() << ") on worker(" << worker.mWorkerName << ") attempted to lock PackerScenario(" << packerSession.getParentPackerScenarioId() << ") currently locked by sibling PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << getSubSessionNameByPackerSessionId(lockedForFinalizationByPackerSessionId) << ") on worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ") for(" << (now - packerSession.getPackerScenarioMaster().getLockedTimestamp()).getMillis() << "ms), PackerSession(" << packerSessionId << ") will be suspended");

        EA_ASSERT(packerScenario.mFinalizationTimerId != INVALID_TIMER_ID);

        packerSession.releaseSessionFromWorker(GameManager::PackerSessionData::SESSION_SUSPENDED);

        upsertSessionData(packerSession);
        return WorkerLockPackerSessionError::GAMEPACKER_ERR_SCENARIO_FINALIZING;
    }
}

Blaze::GamePacker::WorkerUnlockPackerSessionError::Error GamePackerMasterImpl::processWorkerUnlockPackerSession(const Blaze::GameManager::WorkerUnlockPackerSessionRequest &request, const ::Blaze::Message* message)
{
    auto workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
    auto workerItr = mWorkerByInstanceId.find(workerInstanceId);
    if (workerItr == mWorkerByInstanceId.end())
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerUnlockPackerSession: Worker instance(" << workerInstanceId << ") is not registered");
        return WorkerUnlockPackerSessionError::GAMEPACKER_ERR_WORKER_NOT_FOUND;
    }
    auto& worker = *workerItr->second;
    auto packerSessionId = request.getPackerSessionId();
    auto sessionItr = mSessionByIdMap.find(packerSessionId);
    if (sessionItr == mSessionByIdMap.end())
    {
        return WorkerUnlockPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_FOUND;
    }

    auto& packerSession = *sessionItr->second;
    auto& packerScenario = packerSession.getPackerScenarioMaster();
    auto& ownerUserPackerSession = packerScenario.getOwnerUserSessionInfo();
    LogContextOverride logAudit(ownerUserPackerSession.getSessionId());

    const auto assignedInstanceId = packerSession.getAssignedWorkerInstanceId();
    if (assignedInstanceId != workerInstanceId)
    {
        WARN_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: worker(" << worker.mWorkerName << ") attempted to unlock PackerSession(" << packerSessionId << ") currently assigned to worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerUnlockPackerSessionError::GAMEPACKER_ERR_SESSION_NOT_ASSIGNED;
    }

    auto lockedForFinalizationByPackerSessionId = packerScenario.getLockedForFinalizationByPackerSessionId();
    if (lockedForFinalizationByPackerSessionId != packerSessionId)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerLockPackerSession: PackerSession(" << packerSessionId << ":" << packerSession.getSubSessionName() << ") on worker(" << worker.mWorkerName << ") attempted to unlock PackerScenario(" << packerSession.getParentPackerScenarioId() << ") currently locked by PackerSession(" << lockedForFinalizationByPackerSessionId << ":" << getSubSessionNameByPackerSessionId(lockedForFinalizationByPackerSessionId) << ") on worker(" << getWorkerNameByInstanceId(assignedInstanceId) << ")");
        return WorkerUnlockPackerSessionError::GAMEPACKER_ERR_SCENARIO_FINALIZING;
    }

// Cancel the timer, and update the Data:
    packerScenario.mScenarioData->setLockedForFinalizationByPackerSessionId(INVALID_PACKER_SESSION_ID);
    packerScenario.cancelFinalizationTimer();

    upsertScenarioData(*packerSession.mParentPackerScenario);

    return GamePacker::WorkerUnlockPackerSessionError::ERR_OK;
}

WorkerClaimLayerAssignmentError::Error GamePackerMasterImpl::processWorkerClaimLayerAssignment(const Blaze::GameManager::WorkerLayerAssignmentRequest &request, Blaze::GameManager::WorkerLayerAssignmentResponse &response, const ::Blaze::Message* message)
{
    // NOTE: whenever a slave leaves, we must update all the other slaves,
    // the least 'churny' way to do this is to backfill the 'holes' in the topology 
    // by borrowing from the layer with the most instances to fill the holes in the layer with the least,
    // then when instances show up again, they will replace the recently vacated elements. 
    // Imagine we have a strict heap hierarchy. 1, 2, 4, 8, 16...
    InstanceId workerInstanceId = SlaveSession::getSessionIdInstanceId(message->getSlaveSessionId());
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
        return WorkerClaimLayerAssignmentError::GAMEPACKER_ERR_LAYER_FAILURE;
    }

    auto layerId = LayerInfo::layerIdFromSlotId(slotId);
    if (layerId > LayerInfo::MAX_LAYERS || layerId == LayerInfo::INVALID_LAYER_ID)
    {
        ERR_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") specified layer(" << layerId << ") is invalid, claim rejected");
        return WorkerClaimLayerAssignmentError::GAMEPACKER_ERR_LAYER_FAILURE;
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
                TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") unassigned from slotId(" << worker.mWorkerSlotId << "), layer(" << worker.getLayerId() << ")");
                EA_ASSERT_MSG(&worker == workerBySlotItr->second, "Worker instance mismatch!");
                mWorkerBySlotId.erase(workerBySlotItr); // free old slotid this worker held
            }
            else
            {
                EA_ASSERT_MSG(worker.mWorkerSlotId == 0, "Worker must have unset slotId!");
            }
        }

        if (worker.getLayerId() == layerId)
        {
            // slot has changed, but the layer id remains the same, update the slot id
            auto ret = mWorkerBySlotId.insert(slotId);
            if (!ret.second)
            {
                auto& otherWorker = *ret.first->second;
                EA_ASSERT_MSG(otherWorker.getLayerId() == layerId, "Bumped worker layer id mismatch!");
                TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") bumped worker(" << otherWorker.mWorkerName << ") from slotId(" << otherWorker.mWorkerSlotId << "), layer(" << otherWorker.getLayerId() << "), expect new slot claim at version(" << request.getLayerSlotAssignmentVersion() << "+)");
                otherWorker.mWorkerSlotId = 0; // deliberately unassign the slot, while leaving the layer as it was to signal that this is a worker pending slot assignment
            }
            TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") reassigned to slotId(" << worker.mWorkerSlotId << "->" << slotId << "), layer(" << layerId << ")");
            worker.mWorkerSlotId = slotId;
            ret.first->second = &worker;
            return WorkerClaimLayerAssignmentError::ERR_OK;
        }
        else
        {
            // Drain the worker, but don't reactivate the sessions.   This will keep the sessions running on the current worker:
            drainWorkerInstance(workerInstanceId, false);
        }
    }

    // ensure layer list is big enough to accommodate new layer index
    if (mLayerCount < layerId)
        mLayerCount = layerId;

    auto& layer = mLayerList[(layerId-1)];          // This allocates the layer (getLayerFromId does not)
    if (layer.mLayerId == LayerInfo::INVALID_LAYER_ID)
    {
        layer.mLayerId = layerId;
    }
    else
    {
        EA_ASSERT_MSG(layer.mLayerId == layerId, "Cannot change layerId for existing layer!"); // This assert means somehow we've got a bug where layerIndex does not match layerId!
    }

    // Allocate and track the worker:
    auto& worker = *(BLAZE_NEW WorkerInfo(workerInstanceId, &layer, request));
    mWorkerByInstanceId[workerInstanceId] = &worker;
    {
        auto ret = mWorkerBySlotId.insert(slotId);
        if (!ret.second)
        {
            auto& otherWorker = *ret.first->second;
            EA_ASSERT_MSG(otherWorker.mWorkerSlotId == slotId, "Bumped worker slot id mismatch!");
            EA_ASSERT_MSG(otherWorker.getLayerId() == layerId, "Bumped worker layer id mismatch!");
            WARN_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") bumped worker(" << otherWorker.mWorkerName << ") from slotId(" << otherWorker.mWorkerSlotId << "), layer(" << otherWorker.getLayerId() << "), expect new slot claim at version(" << request.getLayerSlotAssignmentVersion() << "+)");
            otherWorker.mWorkerSlotId = 0; // deliberately unassign the slot, while leaving the layer as it was to signal that this is a worker pending slot assignment

            // PACKER_TODO - The old worker is still in the layer mWorkerByState list, still has assigned sessions, and can still claim Sessions (though it will not get new notifications). 
        }
        ret.first->second = &worker; // PACKER_MAYBE: we may want to unlink the bumped worker from the layer it was in, and clear the layerid/slotid pair inside it to keep things consistent, otherwise we can put the bumped worker into a new mUnassignedWorkerList
    }

    TRACE_LOG("[GamePackerMasterImpl].processWorkerClaimLayerAssignment: worker(" << workerName << ") assigned to slotId(" << worker.mWorkerSlotId << "), layer(" << worker.getLayerId() << ")");
    
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
        metricsKey.sprintf("layer.%d.worker.%d.perf", (int32_t)worker.getLayerId(), (int32_t)worker.mWorkerInstanceId);
        auto& metricsInfo = response.getMetricsInfo()[metricsKey.c_str()];
        metricsInfo = response.getMetricsInfo().allocate_element();
        auto& metricsList = metricsInfo->getMetrics();

        //metricsList.reserve(worker.mWorkerResponseStatusRing.size());
        //eastl::string data;
        //data.sprintf("%8s  %5s  %5s  %5s", "resp(us)", "state", "w_cpu", "m_cpu");
        //metricsList.push_back(data.c_str()); // push back header
        //for (auto& responseStatus : worker.mWorkerResponseStatusRing)
        //{
        //    data.sprintf("%8u  %5u  %5u  %5u", (uint32_t)responseStatus.mWorkerResponseTime.getMicroSeconds(), (uint32_t)responseStatus.mSignaledState, responseStatus.mWorkerCpuPercent, responseStatus.mMasterCpuPercent);
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
const uint32_t GamePackerMasterImpl::WorkerInfo::RESET_IDLE_STATE_TIMEOUT_MS = 1000; // Wait this long before suspending a SIGNALED and tardy worker by assigning it to BUSY state.

GamePackerMasterImpl::WorkerInfo::WorkerInfo(InstanceId workerInstanceId, LayerInfo* layerInfo, const Blaze::GameManager::WorkerLayerAssignmentRequest &request)
{
    mpNext = mpPrev = nullptr;
    mWorkerResetStateTimerId = INVALID_TIMER_ID;

    mWorkerLayerInfo = layerInfo;
    mWorkerName = request.getInstanceName();
    mWorkerSlotId = request.getLayerSlotId();
    mWorkerInstanceId = workerInstanceId;
    mWorkerInstanceType = request.getInstanceTypeName();
    mWorkerState = WorkerInfo::IDLE;

    mWorkerLayerInfo->mWorkerByState[mWorkerState].push_back(*this);
}

GamePackerMasterImpl::WorkerInfo::~WorkerInfo()
{
    WorkerInfoList::remove(*this);
}

const char8_t* GamePackerMasterImpl::WorkerInfo::getWorkerStateString(WorkerState state)
{
    switch (state)
    {
    case Blaze::GamePacker::GamePackerMasterImpl::WorkerInfo::IDLE:
        return "idle";
    case Blaze::GamePacker::GamePackerMasterImpl::WorkerInfo::WORKING:
        return "working";
    default:
        return "unknown";
    }
}

GamePackerMasterImpl::PackerSessionMaster::PackerSessionMaster(PackerSessionId packerSessionId, PackerScenarioMaster& packerScenario, GameManager::PackerSessionDataPtr packerSessionData)
{
    // Build/own the PackerScenarioDataPtr if one is not provided via Redis:
    if (packerSessionData == nullptr)
    {
        packerSessionData = BLAZE_NEW GameManager::PackerSessionData;
        packerSessionData->setPackerSessionId(packerSessionId);
        packerSessionData->setStartedTimestamp(TimeValue::getTimeOfDay());
        packerSessionData->setState(GameManager::PackerSessionData::SESSION_AWAITING_ASSIGNMENT);
    }

    mSessionData = packerSessionData;
    packerScenario.assignPackerSession(*this);   // This set the mParentPackerScenario & mSessionRequest values

// Intrusive Pointer:
    mpNext = mpPrev = nullptr;
    mDelayedStartTimerId = INVALID_TIMER_ID;
}

bool GamePackerMasterImpl::PackerScenarioMaster::assignPackerSession(GamePackerMasterImpl::PackerSessionMaster& packerSession)
{
    PackerSessionId sessionId = packerSession.getPackerSessionId();

    // Find the session info in the request: 
    for (auto& curSessionData : mScenarioRequest->getPackerSessionRequestList())
    {
        if (curSessionData->getPackerSessionId() != sessionId)
            continue;

        packerSession.mParentPackerScenario = this;
        packerSession.mSessionRequest = curSessionData;

        mPackerSessionMasterList.push_back(&packerSession);
        return true;
    }

    WARN_LOG("[PackerScenarioMaster].addPackerSession: PackerScenario(" << getPackerScenarioId() << ") has no info on Session (" << sessionId << ").");
    return false;
}

GamePackerMasterImpl::PackerSessionMaster::~PackerSessionMaster()
{
    if (mDelayedStartTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDelayedStartTimerId);
    }
    if (mpNext != nullptr && mpPrev != nullptr)
        PackerSessionList::remove(*this);
}

EA::TDF::TimeValue GamePackerMasterImpl::PackerSessionMaster::getRemainingLifetime(EA::TDF::TimeValue now) const
{
    return mSessionData->getStartedTimestamp() + mParentPackerScenario->getScenarioDuration() - now;
}

void GamePackerMasterImpl::PackerSessionMaster::workerClaimSession(WorkerInfo& worker)
{
    // session has been assigned to worker
    mSessionData->setState(GameManager::PackerSessionData::SESSION_ASSIGNED);
    mSessionData->setAssignedWorkerInstanceId(worker.mWorkerInstanceId);
    mSessionData->setAssignedLayerId(worker.getLayerId());
    mSessionData->setAcquiredTimestamp(TimeValue::getTimeOfDay());
    if (mSessionData->getReleaseCount() == 0 && mSessionData->getAcquiredTimestamp() > mSessionData->getStartedTimestamp())
    {
        mSessionData->setUnassignedDuration(mSessionData->getUnassignedDuration() + mSessionData->getAcquiredTimestamp() - mSessionData->getStartedTimestamp());
    }
    else if (mSessionData->getReleaseCount() > 0 && mSessionData->getAcquiredTimestamp() > mSessionData->getReleasedTimestamp())
    {
        mSessionData->setUnassignedDuration(mSessionData->getUnassignedDuration() + mSessionData->getAcquiredTimestamp() - mSessionData->getReleasedTimestamp());
    }
}

void GamePackerMasterImpl::PackerSessionMaster::releaseSessionFromWorker(GameManager::PackerSessionData::SessionState sessionState)
{
    mSessionData->setState(sessionState);
    mSessionData->setReleaseCount(mSessionData->getReleaseCount() + 1);
    mSessionData->setReleasedTimestamp(TimeValue::getTimeOfDay());
    mSessionData->setAssignedWorkerInstanceId(INVALID_INSTANCE_ID); // we always clear assigned instance because cleanup scenario tries to send back session removal notifications to workers that remain and we want to avoid that
    // New AssignedLayerId gets assigned in assignSessionToLayer
}

uint32_t GamePackerMasterImpl::PackerSessionMaster::getDurationPercentile() const
{
    double duration = (double)(TimeValue::getTimeOfDay() - getStartedTimestamp()).getMicroSeconds();
    double maxDuration = (double)mParentPackerScenario->getScenarioDuration().getMicroSeconds();
    return quantizeRatioAsDurationPercentile(duration, maxDuration);
}

const Blaze::GameManager::UserJoinInfo* GamePackerMasterImpl::PackerScenarioMaster::getHostJoinInfo() const
{
    for (auto& userJoinInfo : getUserJoinInfoList())
    {
        if (userJoinInfo->getIsHost())
        {
            return userJoinInfo.get();
        }
    }
    return nullptr;
}

void GamePackerMasterImpl::PackerSessionMaster::cancelDelayStartTimer()
{
    if (mDelayedStartTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDelayedStartTimerId);
        mDelayedStartTimerId = INVALID_TIMER_ID;
    }
}

void GamePackerMasterImpl::PackerSessionMaster::updatePackerSession(const PackerPinData& pinData)
{
    auto assignedWorkerInstanceId = getAssignedWorkerInstanceId();
    auto& sessionInfo = mSessionInfoByInstanceIdMap[assignedWorkerInstanceId];
    
    auto oldSiloCount = sessionInfo.mSiloCount;
    auto newSiloCount = pinData.getSiloCount();
    // Set total silo count
    uint64_t totalSiloCount = mSessionData->getTotalSiloCount() - oldSiloCount + newSiloCount;
    mSessionData->setTotalSiloCount(totalSiloCount);
    // Set silo count per worker instance
    sessionInfo.mSiloCount = newSiloCount;
    
    auto oldEvictionCount = sessionInfo.mEvictionCount;
    auto newEvictionCount = pinData.getEvictionCount();
    // Set total eviction count
    uint64_t totalEvictionCount = mSessionData->getTotalEvictionCount() - oldEvictionCount + newEvictionCount;
    mSessionData->setTotalEvictionCount(totalEvictionCount);
    // Set eviction count per worker instance
    sessionInfo.mEvictionCount = newEvictionCount;

    if (pinData.getMaxSiloPlayerCount() > getMaxSiloPlayerCount())
        mSessionData->setMaxSiloPlayerCount(pinData.getMaxSiloPlayerCount());
    mSessionData->setMatchedPlayersInGameCount(pinData.getMatchedPlayersInGameCount());
    pinData.getFactorDetailsList().copyInto(getFactorDetailsList());

    // update overall scoring info
    auto& scoringInfo = pinData.getOverallScoringInfo();
    auto oldBestGameQualityScore = getOverallScoringInfo().getBestGameQualityScore();
    scoringInfo.copyInto(getOverallScoringInfo());
    if (scoringInfo.getBestGameQualityScore() < oldBestGameQualityScore)
        getOverallScoringInfo().setBestGameQualityScore(oldBestGameQualityScore);
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

GamePackerMasterImpl::PackerScenarioMaster::PackerScenarioMaster(PackerScenarioId packerScenarioId, GameManager::StartPackerScenarioRequestPtr scenarioRequest, GameManager::PackerScenarioDataPtr scenarioData)
{
    mExpiryTimerId = INVALID_TIMER_ID;
    mFinalizationTimerId = INVALID_TIMER_ID;

    // Build/own the PackerScenarioDataPtr if one is not provided via Redis:
    if (scenarioData == nullptr)
    {
        scenarioData = BLAZE_NEW GameManager::PackerScenarioData;
        scenarioData->setPackerScenarioId(packerScenarioId);
        scenarioData->setStartedTimestamp(TimeValue::getTimeOfDay());
        scenarioData->setExpiresTimestamp(scenarioData->getStartedTimestamp() + scenarioRequest->getPackerScenarioRequest().getScenarioTimeoutDuration()); // NOTE: grace period can be reconfigured, snapshot the time without it
    }

    mScenarioData = scenarioData;
    mScenarioRequest = scenarioRequest;
}

uint32_t GamePackerMasterImpl::PackerScenarioMaster::getDurationPercentile() const
{
    TimeValue startedTime = getStartedTimestamp();
    double duration = (double)(TimeValue::getTimeOfDay() - startedTime).getMicroSeconds();
    double maxDuration = (double)(getExpiresTimestamp() - startedTime).getMicroSeconds();
    return quantizeRatioAsDurationPercentile(duration, maxDuration);
}

GamePackerMasterImpl::PackerScenarioMaster::~PackerScenarioMaster()
{
    cancelExpiryTimer();
    cancelFinalizationTimer();
}

void GamePackerMasterImpl::PackerScenarioMaster::cancelExpiryTimer()
{
    if (mExpiryTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mExpiryTimerId);
        mExpiryTimerId = INVALID_TIMER_ID;
    }
}

void GamePackerMasterImpl::PackerScenarioMaster::cancelFinalizationTimer()
{
    if (mFinalizationTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mFinalizationTimerId);
        mFinalizationTimerId = INVALID_TIMER_ID;
    }
}


} // GamePacker
} // Blaze

#pragma pop_macro("LOGGER_CATEGORY")
