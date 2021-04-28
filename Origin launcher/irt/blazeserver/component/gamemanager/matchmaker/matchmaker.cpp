/*! ************************************************************************************************/
/*!
    \file matchmaker.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

// framework includes
#include "framework/blaze.h"

#include "EATDF/time.h"

#include "framework/util/random.h"
#include "framework/connection/selector.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/event/eventmanager.h"

#include "gamemanager/tdf/matchmaker_component_config_server.h"
#include "gamemanager/matchmakercomponent/matchmakerslaveimpl.h"
#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/matchmaker/matchmakingconfig.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"

#include "EASTL/sort.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/matchmaker/rules/propertiesrules/propertyruledefinition.h"
#include "gamemanager/commoninfo.h"

namespace Blaze
{

namespace Metrics
{
namespace Tag
{
TagInfo<GameManager::QosValidationMetricResult>* qos_validation_metric_result = BLAZE_NEW TagInfo<GameManager::QosValidationMetricResult>("qos_validation_metric_result", [](const GameManager::QosValidationMetricResult& value, Metrics::TagValue&) { return QosValidationMetricResultToString(value); });

TagInfo<GameManager::RuleName>* rule_name = BLAZE_NEW TagInfo<GameManager::RuleName>("rule_name", [](const GameManager::RuleName& value, Metrics::TagValue&) { return value.c_str(); });

TagInfo<RuleCategory>* rule_category = BLAZE_NEW TagInfo<RuleCategory>("rule_category");

TagInfo<RuleCategoryValue>* rule_category_value = BLAZE_NEW TagInfo<RuleCategoryValue>("rule_category_value");

TagInfo<GameManager::MatchmakingResult>* match_result = BLAZE_NEW TagInfo<GameManager::MatchmakingResult>("match_result", [](const GameManager::MatchmakingResult& value, Metrics::TagValue&) { return GameManager::MatchmakingResultToString(value); });
}
}

namespace GameManager
{
namespace Matchmaker
{

    EA_THREAD_LOCAL Matchmaker* gMatchmaker = nullptr;

    const TimeValue STATUS_UPDATE_TIME = 5 * 1000 * 1000; //5s


    void intrusive_ptr_add_ref(Matchmaker::GameCapacityInfo* ptr)
    {
        ++ptr->refCount;
    }

    void intrusive_ptr_release(Matchmaker::GameCapacityInfo* ptr)
    {
        if (ptr->refCount > 0)
        {
            --ptr->refCount;
            if (ptr->refCount == 0)
            {
                EA_ASSERT(ptr->pendingJoinCount == 0);
                GameId gameId = ptr->gameId; // Need this copy because hashtable::erase(const key_type&) takes a reference to the key and uses it multiple times when performing search iteration after having freed the internal node which matches the key.
                gMatchmaker->mGameCapacityInfoMap.erase(gameId);
            }
        }
    }

    /*! ************************************************************************************************/
    /*!
        \brief Construct the Matchmaker obj
    
        \param[in] matchmakerSlave - pointer back to the MatchmakerSlave
    *************************************************************************************************/
    Matchmaker::Matchmaker(Blaze::Matchmaker::MatchmakerSlaveImpl& matchmakerSlave)
        : IdlerUtil("[Matchmaker]")
        , mMatchmakerSlave(matchmakerSlave)
        , mScenarioMetrics("scenario.", matchmakerSlave.getMetricsCollection())
        , mMatchmakingMetrics(matchmakerSlave.getMetricsCollection())
        , mScenarioMatchmakingMetricsByScenarioHashMap(BlazeStlAllocator("Matchmaker::ScenarioMatchmakingMetrics", Blaze::Matchmaker::MatchmakerSlave::COMPONENT_MEMORY_GROUP))
        , mScenarioLockData(BlazeStlAllocator("Matchmaker::ScenarioLockData", Blaze::Matchmaker::MatchmakerSlave::COMPONENT_MEMORY_GROUP))
        , mSessionLockData(BlazeStlAllocator("Matchmaker::SessionLockData", Blaze::Matchmaker::MatchmakerSlave::COMPONENT_MEMORY_GROUP))
        , mCreateGameSessionMap(BlazeStlAllocator("Matchmaker::CreateGameSessionMap", Blaze::Matchmaker::MatchmakerSlave::COMPONENT_MEMORY_GROUP))
        , mFindGameSessionMap(BlazeStlAllocator("Matchmaker::FindGameSessionMap", Blaze::Matchmaker::MatchmakerSlave::COMPONENT_MEMORY_GROUP))
        , mReteNetwork(matchmakerSlave.getMetricsCollection(), matchmakerSlave.LOGGING_CATEGORY)
        , mRuleDefinitionCollection(nullptr)
        , mMatchNodeAllocator("MatchNodeAllocator", sizeof(MatchmakingSessionMatchNode), Blaze::Matchmaker::MatchmakerSlave::COMPONENT_MEMORY_GROUP)
        , mNewSessionsAtIdleStart(0)
        , mDirtySessionsAtIdleStart(0)
        , mCleanSessionsAtIdleStart(0)
        , mStatusUpdateTimerId(INVALID_TIMER_ID)
        , mFiberGroupId(Fiber::INVALID_FIBER_GROUP_ID)
        , mExternalSessionTerminated(false)
        , mEvaluateSessionQueue("Matchmaker::newEvaluateCreateGameSession")
    {
        EA_ASSERT(gMatchmaker == nullptr);
        gMatchmaker = this;
    }

    //! Destructor - cancel outstanding timers and shutdown
    Matchmaker::~Matchmaker()
    {
        // delete any outstanding sessions
        auto createSessionIter = mCreateGameSessionMap.begin();
        auto createSessionMapEnd = mCreateGameSessionMap.end();
        while(createSessionIter != createSessionMapEnd)
        {
            if(createSessionIter->second->isFindGameEnabled())
            {
                auto fgIter = mFindGameSessionMap.find(createSessionIter->second->getMMSessionId());
                if(fgIter != mFindGameSessionMap.end())
                {
                    // need to set the pointed-to iter to nullptr to prevent a delete of freed memory in the FG session map
                    fgIter->second = nullptr;
                    // we're deleting the actual session just below
                }
            }
            delete createSessionIter->second;
            createSessionIter->second = nullptr;
            ++createSessionIter;
        }

        // delete any outstanding sessions
        auto findSessionIter = mFindGameSessionMap.begin();
        auto findSessionMapEnd = mFindGameSessionMap.end();
        while(findSessionIter != findSessionMapEnd)
        {
            delete findSessionIter->second;
            findSessionIter->second = nullptr;
            ++findSessionIter;
        }

        // delete the rule definition collection after the sessions 
        // so the rules have access to their definitions during shutdown.
        delete mRuleDefinitionCollection;

        gMatchmaker = nullptr;
    }

    // Note: components are shut down in a specific (safe) order, so we know the selector still exists.
    void Matchmaker::onShutdown()
    {
        shutdownIdler();

        if(mStatusUpdateTimerId != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mStatusUpdateTimerId);
            mStatusUpdateTimerId = INVALID_TIMER_ID;
        }

        mReteNetwork.onShutdown();

        // wait for all scheduled fibers to exit
        Fiber::join(mFiberGroupId);

        mEvaluateSessionQueue.join();
    }


    Matchmaker::SessionLockData* Matchmaker::getSessionLockData(const MatchmakingSession* mmSession)
    {
        if (mmSession)
        {
            // All sessions with the same Scenario Id use the same lock:
            if (mmSession->getMMScenarioId() != INVALID_SCENARIO_ID)
            {
                ScenarioLockDataMap::iterator iter = mScenarioLockData.find(mmSession->getMMScenarioId());
                if (iter != mScenarioLockData.end())
                {
                    return &iter->second;
                }
            }
            else
            {
                ERR_LOG("[getSessionLockData] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
            }

        }
            
        return nullptr;
    }

    Matchmaker::SessionLockData const* Matchmaker::getSessionLockDataConst(const MatchmakingSession* mmSession) const
    {
        if (mmSession)
        {
            // All sessions with the same Scenario Id use the same lock:
            if (mmSession->getMMScenarioId() != INVALID_SCENARIO_ID)
            {
                ScenarioLockDataMap::const_iterator iter = mScenarioLockData.find(mmSession->getMMScenarioId());
                if (iter != mScenarioLockData.end())
                {
                    return &iter->second;
                }
            }
            else
            {
                ERR_LOG("[getSessionLockDataConst] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
            }
        }
            
        return nullptr;
    }

    void Matchmaker::setSessionLockdown(const MatchmakingSession* mmSession)
    {
        SessionLockData* lockData = getSessionLockData(mmSession);
        if (lockData)
        {
            lockData->mIsLocked = true;
            lockData->mLockOwner = mmSession->getMMSessionId();
        }
    }
    void Matchmaker::clearSessionLockdown(const MatchmakingSession* mmSession)
    {
        // Only the owner of the lock can clear it:  (should happen in destructor in worst case)
        SessionLockData* lockData = getSessionLockData(mmSession);
        if (lockData && mmSession->getMMSessionId() == lockData->mLockOwner)
        {
            lockData->mIsLocked = false;
            lockData->mLockOwner = INVALID_MATCHMAKING_SESSION_ID;
        }
    }
    bool Matchmaker::isSessionLockedDown(const MatchmakingSession* mmSession, bool ignoreOtherSessionLock) const
    {
        SessionLockData const* lockData = getSessionLockDataConst(mmSession);
        if (lockData)
        {
            // We have this special case so we can cancel the other sessions while the 'winning' session is finalizing. 
            if (ignoreOtherSessionLock == false ||
                lockData->mLockOwner == mmSession->getMMSessionId())
            {                
                return lockData->mIsLocked;
            }
        }
        return false;
    }
    void Matchmaker::addSessionLock(const MatchmakingSession* mmSession)
    {
        SessionLockData* lockData = getSessionLockData(mmSession);
        if (lockData)
        {
            ++lockData->mUsageCount;
        }
        else
        {
            if (mmSession != nullptr)
            {
                if (mmSession->getMMScenarioId() != INVALID_SCENARIO_ID)
                {
                    mScenarioLockData.insert(mmSession->getMMScenarioId());
                }
                else
                {
                    ERR_LOG("[addSessionLock] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
                }
            }
        }

    }
    void Matchmaker::removeSessionLock(const MatchmakingSession* mmSession)
    {
        // Check for a
        SessionLockData* lockData = getSessionLockData(mmSession);
        if (lockData)
        {
            --lockData->mUsageCount;
            if (lockData->mUsageCount <= 0)
            {
                if (mmSession->getMMScenarioId() != INVALID_SCENARIO_ID)
                {
                    mScenarioLockData.erase(mmSession->getMMScenarioId());
                }
                else
                {
                    ERR_LOG("[removeSessionLock] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
                }
            }
        }
    }

    bool Matchmaker::isOnlyScenarioSubsession(const MatchmakingSession* mmSession) const
    {
        if ((mmSession == nullptr) || (mmSession->getMMScenarioId() == INVALID_SCENARIO_ID))
        {
            return false;
        }

        const Matchmaker::SessionLockData* lockData = getSessionLockDataConst(mmSession);
        if (lockData == nullptr || lockData->mScenarioSubsessionCount <= 1)
            return true;

        return false;

    }

    /*! ************************************************************************************************/
    /*!
        \brief Process a cancelSession command; cancels an existing matchmaking session; generates
            async notification.

        \param[in]    sessionId - the id of the matchmaking session to cancel.
        \param[in]    ownerSession - the owner user session who trigger the cancel MM session, should never be nullptr
        \return    ERR_OK, if the session was canceled; otherwise a cancelMatchmaking error to return to the client
    *************************************************************************************************/
    BlazeRpcError Matchmaker::cancelSession(MatchmakingSessionId sessionId, UserSessionId ownerSessionId)
    {
        MatchmakingSession* canceledSession = findSession(sessionId);
        if (canceledSession == nullptr)
        {
            TRACE_LOG(LOG_PREFIX << "Unable to cancel matchmaking session " << sessionId << " - unknown sessionId...");
            return Blaze::MATCHMAKER_ERR_UNKNOWN_MATCHMAKING_SESSION_ID;
        }

        // NOTE: With sharded matchmaker there is the potential that the session is finalizing and waiting for the create or join job to complete.
        // This is a minimal time slice, and matchmaking is considered "done" at this point,
        //  so the cancel returns. (even though the session technically still exists)
        if (canceledSession->isLockedDown(true))  // (Pass in true because we want to allow scenario subsession to be canceled if they're not the one that started the lock)
        {
            canceledSession->markForCancel();
            TRACE_LOG(LOG_PREFIX << "Unable to cancel matchmaking session " << sessionId << " - session is locked down and in the process of finalizing " 
                << ownerSessionId << ", owning user: " << canceledSession->getOwnerUserSessionId() << ")...");
            return Blaze::MATCHMAKER_ERR_UNKNOWN_MATCHMAKING_SESSION_ID;
        }

        // check permission (session owner), and cancel the session
        if (ownerSessionId == canceledSession->getOwnerUserSessionId())
        {
            TRACE_LOG(LOG_PREFIX << "User(" << canceledSession->getOwnerBlazeId() << ":" << canceledSession->getOwnerPersonaName() 
                      << ") canceling matchmaking session(" << sessionId << ")");

            // finalize the session (send the async notification) & destroy it
            canceledSession->finalizeAsCanceled();

            // destroy session will handle canceling the FG session on the search slaves
            destroySession(sessionId, SESSION_CANCELED);
            
            return Blaze::ERR_OK;
        }
        else
        {
            // error: current context doesn't own the matchmaking session they're trying to cancel...
            TRACE_LOG(LOG_PREFIX << "Unable to cancel matchmaking session " << sessionId << " - session owned by another user ( attempted user: " 
                      << ownerSessionId << ", owning user: " << canceledSession->getOwnerUserSessionId() << ")...");
            return Blaze::MATCHMAKER_ERR_NOT_MATCHMAKING_SESSION_OWNER;
        }
    }

    void Matchmaker::dispatchMatchmakingConnectionVerifiedForSession(MatchmakingSessionId mmSessionId, GameId gameId, bool dispatchClientListener)
    {
        auto session = findSession(mmSessionId);
        if (session != nullptr)
        {
            session->sendNotifyMatchmakingSessionConnectionValidated(gameId, dispatchClientListener);
        }
    }

    bool Matchmaker::isMatchmakingSessionInMap(MatchmakingSessionId mmSessionId)
    {
        return findSession(mmSessionId) != nullptr; 
    }

    bool Matchmaker::isUserInCreateGameMatchmakingSession(UserSessionId userSessionId, MatchmakingSessionId mmSessionId)
    {
        // we only need this method for create mode matchmaking's validation of the auto join member.
        // no need to check in find game matchmaking sessions.
        auto session = findCreateGameSession(mmSessionId);
        if (session == nullptr)
            return false;

        // scan the current member info list, for the user session.
        for (MatchmakingSession::MemberInfoList::const_iterator itr = session->getMemberInfoList().begin();
            itr != session->getMemberInfoList().end(); ++itr)
        {
            if (static_cast<const MatchmakingSession::MemberInfo &>(*itr).mUserSessionInfo.getSessionId() == userSessionId)
                return true;
        }
        return false;
    }

    Blaze::GameManager::Matchmaker::MatchmakingSession* Matchmaker::findSession(MatchmakingSessionId sessionId) const
    {
        MatchmakingSession* session = findCreateGameSession(sessionId);

        if (session == nullptr)
        {
            session = findFindGameSession(sessionId);
        }

        return session;
    }

    Blaze::GameManager::Matchmaker::MatchmakingSession* Matchmaker::findCreateGameSession(MatchmakingSessionId sessionId) const
    {
        auto sessionMapItr = mCreateGameSessionMap.find(sessionId);
        if (sessionMapItr != mCreateGameSessionMap.end())
        {
            return sessionMapItr->second;
        }

        return nullptr;
    }

    Blaze::GameManager::Matchmaker::MatchmakingSession* Matchmaker::findFindGameSession(MatchmakingSessionId sessionId) const
    {
        auto sessionMapItr = mFindGameSessionMap.find(sessionId);
        if (sessionMapItr != mFindGameSessionMap.end())
        {
            return sessionMapItr->second;
        }
        return nullptr;
    }

    static bool isWithinThreshold(uint32_t a, uint32_t b, uint32_t threshold)
    {
        return ((a > b) ? (a - b) : (b - a) <= threshold);
    }

    void Matchmaker::onStatusUpdateTimer()
    {
#if 1 // #HACK Use to disable spammy matchmaker status notifications...
        MatchmakingStatus status;
        filloutMatchmakingStatus(status);
        if (mLastSentStatus.getInstanceId() != status.getInstanceId() || 
            !isWithinThreshold(mLastSentStatus.getNumSessions(), status.getNumSessions(), 1) || 
            !isWithinThreshold(mLastSentStatus.getCpuUtilization(), status.getCpuUtilization(), 1))
        {
            status.copyInto(mLastSentStatus);
            mMatchmakerSlave.sendNotifyMatchmakingStatusUpdate(status);
        }
        else
        {
            SPAM_LOG(LOG_PREFIX << "MatchmakingStatus is within previously sent thresholds, skipping.");
        }
        
        mStatusUpdateTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + mMatchmakingConfig.getServerConfig().getStatusNotificationPeriod(), this, &Matchmaker::onStatusUpdateTimer, "Matchmaker::onStatusUpdateTimer");
#endif
    }


    void Matchmaker::filloutMatchmakingStatus(MatchmakingStatus& status)
    {
        status.setInstanceId(gController->getInstanceId());

        // we shard based on CPU load
        status.setCpuUtilization(gFiberManager->getCpuUsageForProcessPercent());
        status.setNumSessions(getTotalMatchmakingSessions());
    }

    void Matchmaker::clearLastSentMatchmakingStatus()
    {
        mLastSentStatus.setInstanceId(INVALID_INSTANCE_ID);
    }

    Matchmaker::PMRMetricsHelper* Matchmaker::cleanupOldPlayerMatchmakingRateMetrics(PMRMetricsList& currentMetrics, bool createNewEntry)
    {
        TimeValue updateInterval = getMatchmakingConfig().getServerConfig().getPlayersMatchmakingMetricUpdateInterval();
        TimeValue historyDuration = getMatchmakingConfig().getServerConfig().getPlayersMatchmakingMetricHistoryDuration();

        TimeValue curTime = TimeValue::getTimeOfDay();
        curTime.setMicroSeconds((curTime.getMicroSeconds() / updateInterval.getMicroSeconds()) * updateInterval.getMicroSeconds());

        PMRMetricsHelper* pmrHelper = nullptr;
        if (!currentMetrics.empty())
        {
            pmrHelper = &currentMetrics.front();
            // If time has passed, pop all the old ones that aren't needed anymore.
            if (pmrHelper->mLastUpdateTime != curTime)
            {
                pmrHelper = nullptr;
                // list version:
                PMRMetricsList::const_iterator oldMetrics = currentMetrics.begin();
                while (oldMetrics != currentMetrics.end() && (curTime.getMicroSeconds() - oldMetrics->mLastUpdateTime.getMicroSeconds() < historyDuration.getMicroSeconds()))
                    ++oldMetrics;

                if (oldMetrics != currentMetrics.end())
                    currentMetrics.erase(oldMetrics, currentMetrics.end());
            }
        }

        if (createNewEntry)
        {
            if (pmrHelper == nullptr)
            {
                // Add a new entry if the front wasn't valid:
                pmrHelper = &currentMetrics.push_front();
                pmrHelper->mLastUpdateTime = curTime;
            }
        }

        return pmrHelper;
    }

    void Matchmaker::trackPlayerMatchmakingRate(const ScenarioName& scenarioName, const PingSiteAlias& pingSite, const DelineationGroupType& delineationGroup)
    {
        PMRMetricsList& currentMetrics = mScenarioPMRMetrics[scenarioName][pingSite][delineationGroup];
        PMRMetricsHelper* pmrHelper = cleanupOldPlayerMatchmakingRateMetrics(currentMetrics, true);
        pmrHelper->mUserCount += 1;
    }

    void Matchmaker::getMatchmakingInstanceData(PlayerMatchmakingRateByScenarioPingsiteGroup& pmrMetrics, MatchmakingSessionsByScenario& scenarioData)
    {
        // Iterate over all scenario/pingsites: 
        for (auto& curScenarioIter : mScenarioPMRMetrics)
        {
            const ScenarioName& scenarioName = curScenarioIter.first;

            auto pmrIter = pmrMetrics.find(scenarioName);
            if (pmrIter == pmrMetrics.end())
                pmrIter = pmrMetrics.insert(eastl::make_pair(scenarioName, pmrMetrics.allocate_element())).first;

            for (auto& curPingSiteIter : curScenarioIter.second)
            {
                const PingSiteAlias& pingSiteAlias = curPingSiteIter.first;
                auto iterOut = pmrIter->second->find(pingSiteAlias);
                if (iterOut == pmrIter->second->end())
                    iterOut = pmrIter->second->insert(eastl::make_pair(pingSiteAlias, pmrIter->second->allocate_element())).first;

                for (auto& curDelineationGroupIter : curPingSiteIter.second)
                {
                    const GameManager::DelineationGroupType& delineationGroup = curDelineationGroupIter.first;
                    PMRMetricsList& metricList = curDelineationGroupIter.second;

                    // Remove outdated metrics: 
                    cleanupOldPlayerMatchmakingRateMetrics(metricList, false);

                    // Iterate over, and add in calculate the metrics: 
                    // Interval count is needs so we don't undercount at the start
                    float totalPlayers = 0.0f;
                    for (PMRMetricsList::const_iterator curIter = metricList.begin(); curIter != metricList.end(); ++curIter)
                    {
                        totalPlayers += curIter->mUserCount;
                    }

                    float pmrValue = totalPlayers;
                    if (pmrIter->second->find(delineationGroup) == pmrIter->second->end())
                        (*iterOut->second)[delineationGroup] = pmrValue;
                    else
                        (*iterOut->second)[delineationGroup] += pmrValue;
                }
            }
        }



        for (auto curItr : mSessionsByScenarioMap)
        {
            const ScenarioName& scenarioName = curItr.first;

            auto scenarioIter = scenarioData.find(scenarioName);
            if (scenarioIter == scenarioData.end())
                scenarioIter = scenarioData.insert(eastl::make_pair(scenarioName, scenarioData.allocate_element())).first;

            MatchmakingSessionsByPingSiteMap& currentSessionsBySite = curItr.second;
            for (auto curSiteItr : currentSessionsBySite)
            {
                const PingSiteAlias& pingSiteAlias = curSiteItr.first;
                if (scenarioIter->second->find(pingSiteAlias) == scenarioIter->second->end())
                    (*scenarioIter->second)[pingSiteAlias] = curSiteItr.second;
                else
                    (*scenarioIter->second)[pingSiteAlias] += curSiteItr.second;
            }
        }
        
    }

    void Matchmaker::addStartMetrics(MatchmakingSession* newSession, bool setStartMetrics)
    {
        if (newSession->getMMScenarioId() == INVALID_SCENARIO_ID)
        {
            ERR_LOG("[addStartMetrics] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }

        mScenarioMetrics.mGaugeMatchmakingUsersInSessions.increment(newSession->getPlayerCount());

        if (setStartMetrics)
        {
            MatchmakerMasterMetrics& metrics = mScenarioMetrics;
            metrics.mTotalMatchmakingSessionStarted.increment();

            PingSiteAlias pingSite = UNKNOWN_PINGSITE;
            if (newSession->getPrimaryUserSessionInfo() != nullptr)
                pingSite = newSession->getPrimaryUserSessionInfo()->getBestPingSiteAlias();
            ScenarioName scenarioName = newSession->getScenarioInfo().getScenarioName();
            DelineationGroupType delineationGroup = newSession->getCachedStartMatchmakingInternalRequestPtr()->getRequest().getCommonGameData().getDelineationGroup();

            trackPlayerMatchmakingRate(scenarioName, pingSite, delineationGroup);
            mSessionsByScenarioMap[scenarioName][pingSite]++;
        }

        const char8_t* scenarioName = newSession->getScenarioInfo().getScenarioName();
        const char8_t* variantName = newSession->getScenarioInfo().getScenarioVariant();
        ScenarioVersion scenarioVersion = newSession->getScenarioInfo().getScenarioVersion();
        const char8_t* subsessionName = newSession->getScenarioInfo().getSubSessionName();

        if (setStartMetrics)
        {
            if (!newSession->isSinglePlayerSession())
            {
                mMatchmakingMetrics.mMetrics.mTotalPlayersInGroupRequests.increment(newSession->getPlayerCount(), scenarioName, variantName, scenarioVersion);
            }

            mMatchmakingMetrics.mMetrics.mTotalRequests.increment(1, scenarioName, variantName, scenarioVersion, newSession->getPlayerCount());
        }

        if (!newSession->isSinglePlayerSession())
        {
            mMatchmakingMetrics.mSubsessionMetrics.mTotalPlayersInGroupRequests.increment(newSession->getPlayerCount(), scenarioName, variantName, scenarioVersion, subsessionName);
        }

        mMatchmakingMetrics.mSubsessionMetrics.mTotalRequests.increment(1, scenarioName, variantName, scenarioVersion, subsessionName, newSession->getPlayerCount());
    }

    BlazeRpcError Matchmaker::startSession(const StartMatchmakingInternalRequest &originalRequest, Blaze::ExternalSessionIdentification& externalSessionIdentification, MatchmakingCriteriaError &err)
    {
        const UserSessionInfo* ownerUserSessionInfo = Blaze::GameManager::getHostSessionInfo(originalRequest.getUsersInfo());
        if (EA_UNLIKELY(!ownerUserSessionInfo || !gUserSessionManager->getSessionExists(ownerUserSessionInfo->getSessionId())))
        {
            // Bail out early if the user session owning the matchmaking session is no longer available.
            TRACE_LOG("[Matchmaker].startSession(): Matchmaking session owner user session(" << (ownerUserSessionInfo ? ownerUserSessionInfo->getSessionId() : 0) << ") not found");
            return Blaze::MATCHMAKER_ERR_MATCHMAKING_USERSESSION_NOT_FOUND;
        }

        // Session Ids are now owned by the Scenario (coreSlaves):
        auto sessionId = originalRequest.getMatchmakingSessionId();

        MatchmakingSession* newSession = nullptr;

        TRACE_LOG(LOG_PREFIX << "User(" << ownerUserSessionInfo->getUserInfo().getId() << ":" << ownerUserSessionInfo->getUserInfo().getPersonaName()
                   << ") starting matchmaking session(" << sessionId << "), mode(" << originalRequest.getRequest().getSessionData().getSessionMode().getBits()
                   << "), duration(" << originalRequest.getRequest().getSessionData().getSessionDuration().getMillis() << ")");

        // create/init/validate the matchmaking session
        auto error = createNewSession(originalRequest, sessionId, &newSession, externalSessionIdentification, err);
        if (error != Blaze::ERR_OK)
        {
            TRACE_LOG(LOG_PREFIX << "Start Matchmaking Session failed, error: " << err.getErrMessage());
            return error;
        }

        // Use createNewSession's updated request
        const StartMatchmakingInternalRequestPtr request = newSession->getCachedStartMatchmakingInternalRequestPtr();

        if (newSession->getMMScenarioId() == INVALID_SCENARIO_ID)
        {
            ERR_LOG("[startSession] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }

        MatchmakerMasterMetrics& metrics = mScenarioMetrics;
        mScenarioMetrics.mTotalMatchmakingScenarioSubsessionStarted.increment();

        bool setStartMetrics = false;
        SessionLockData* lockData = getSessionLockData(newSession);
        if (lockData != nullptr)
        {
            if (lockData->mHasSetStartMetrics == false)
            {
                setStartMetrics = true;
                lockData->mHasSetStartMetrics = true;
            }
            ++lockData->mScenarioSubsessionCount;
        }
        // increment these metrics, before any destroySession calls below, as destroySession assumes this and may re-decrement gauges etc
        addStartMetrics(newSession, setStartMetrics);

        const StartMatchmakingRequest &startRequest = request->getRequest();
        if (startRequest.getSessionData().getSessionMode().getCreateGame())
        {
            mCreateGameSessionMap[newSession->getMMSessionId()] = newSession; // This insert should always succeed
            mScenarioMetrics.mCreateGameMatchmakingSessions.increment();
            // Note that the session is not yet in the full session list
            queueSessionForNewEvaluation(newSession->getMMSessionId());
        }

        // if in find game mode start remote find game session on search slave
        // can't use newSession after this point, as startMatchmakingFindGameSession() makes a blocking call
        // user logout during the block can potentially destroy the matchmaking session
        if (startRequest.getSessionData().getSessionMode().getFindGame())
        {
            // We expect this to always be true (session ids should not be reused)
            mFindGameSessionMap[sessionId] = newSession;
            mScenarioMetrics.mFindGameMatchmakingSessions.increment();

            error = startMatchmakingFindGameSession(sessionId, *request, err, mMatchmakerSlave.getConfig().getMatchmakerSettings().getSearchMultiRpcTimeout(), &newSession->getDiagnostics());
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG(LOG_PREFIX << "Start Matchmaking Session failed while trying to start Find Game session on search slave with error " << ErrorHelp::getErrorName(error) << ", criteria message: " << err.getErrMessage());

                if (!startRequest.getSessionData().getSessionMode().getCreateGame())
                {
                    // if this is find game only mode then error out here
                    // and destroy the session; in mixed mode we must not do this
                    // because the session may have already started to finalize in
                    // create game mode
                    destroySession(sessionId, SESSION_TERMINATED);
                    return convertSearchComponentErrorToMatchmakerError(error);
                }
            }
        }

        // If we didn't find a matching game or session(s)
        // schedule an idle, if needed
        scheduleNextIdle(TimeValue::getTimeOfDay(), mMatchmakingConfig.getServerConfig().getIdlePeriod());

        if (setStartMetrics)
        {
            switch (startRequest.getCriteriaData().getReputationRulePrefs().getReputationRequirement())
            {
                case REJECT_POOR_REPUTATION :
                    {
                        metrics.mTotalMatchmakingSessionsRejectPoorReputation.increment();
                        break;
                    }
                case ACCEPT_ANY_REPUTATION :
                    {
                        metrics.mTotalMatchmakingSessionsAcceptAnyReputation.increment();
                        break;
                    }
                case MUST_ALLOW_ANY_REPUTATION :
                    {
                        metrics.mTotalMatchmakingSessionsMustAllowAnyReputation.increment();
                        break;
                    }
                default:
                    break;
            }
        }

        ++mNewSessionsAtIdleStart;

        // add initial counts for create mode diagnostics
        if (mMatchmakingConfig.getServerConfig().getTrackDiagnostics())
        {
            auto mmSession = findCreateGameSession(sessionId);
            if (mmSession != nullptr)
            {
                mmSession->getDiagnostics().setSessions(1);
                // Add rule use counts. ILT: This also caches pointers on rules to their diagnostics,
                // to avoid excessive map lookups during evaluation
                if (mmSession->isCreateGameEnabled())
                {
                    for (auto& rule : mmSession->getCriteria().getRules())
                    {
                        rule->tallyRuleDiagnosticsCreateGame(mmSession->getDiagnostics(), nullptr);
                    }
                }
            }
        }
        return Blaze::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief create a new matchmaking session upon the request

        \param[in]request - StartMatchmakingRequest
        \param[in]userSession - user session for the matchmaking session to be created
        \param[in]matchmakingSessionId - match making session id generated for the new session
        \param[out]matchmakingSession - new created matchmaking session
        \param[out]err - error obj passed out
        \return StartMatchmakingMasterError::Error
    ***************************************************************************************************/
    BlazeRpcError Matchmaker::createNewSession(const StartMatchmakingInternalRequest &request, MatchmakingSessionId matchmakingSessionId,
        MatchmakingSession** matchmakingSession, Blaze::ExternalSessionIdentification& externalSessionIdentification, MatchmakingCriteriaError &err )
    {
        // Everyone in a Scenario has the originatingScenarioId set, we just grab it from the host. 
        MatchmakingScenarioId originatingScenarioId = INVALID_SCENARIO_ID;
        for (UserJoinInfoList::const_iterator itr = request.getUsersInfo().begin(), end = request.getUsersInfo().end(); itr != end; ++itr)
        {
            if ((*itr)->getIsHost())
            {
                originatingScenarioId = (*itr)->getOriginatingScenarioId();
            }
        }


        // create/init/validate the matchmaking session
        *matchmakingSession = BLAZE_NEW MatchmakingSession(mMatchmakerSlave, *this, matchmakingSessionId, originatingScenarioId);

        // Note: initialize adds the matchmaking session to mUserSessionsToMatchmakingMemberInfoMap which owns it, until moved to the matchmaking queue, mCreateGameSessionMap, or mFindGameSessionMap.
        if (!(*matchmakingSession)->initialize(request, err, mDedicatedServerOverrideMap))
        {
            delete (*matchmakingSession);
            return Blaze::MATCHMAKER_ERR_INVALID_MATCHMAKING_CRITERIA;
        }

        // Update *Matchmaking session's* external session as appropriate for the platform/its guidelines

        // cache off the user session id, so we can clean up after blocking call to external service without crashing if the user has logged out during the wait.
        UserSessionId ownerUserSessionId = (*matchmakingSession)->getOwnerUserSessionId();
        ExternalUserLeaveInfoList externalUserInfos;
        userSessionInfosToExternalUserLeaveInfos((*matchmakingSession)->getMembersUserSessionInfo(), externalUserInfos);


        // update external session for matchmaking. Any failures here are non-impacting because blaze does not really use the results of mm external sessions. 
        // We silently eat the error here (just update the metric, no other action). 
        BlazeRpcError sessionErr = mMatchmakerSlave.joinMatchmakingExternalSession((*matchmakingSession)->getMMSessionId(), request.getRequest().getSessionData().getExternalMmSessionTemplateName(),
                                                                                   (*matchmakingSession)->getMemberInfoList(), externalSessionIdentification);

        // The join was a blocking call, make sure the user didn't logout and thus destroy our nice new memory.
        // The matchmaking session was linked to the user session in our initialize call above.
        // we do an equal_range on the userSession (getting all of that user's sessions), and find/erase the entry for this MM session.
        FullUserSessionMapEqualRangePair iterPair = getUserSessionsToMatchmakingMemberInfoMap().equal_range(ownerUserSessionId);
        for ( ; iterPair.first != iterPair.second; ++iterPair.first )
        {
            const MatchmakingSession::MemberInfo &fullUserSessionMapMemberInfo = static_cast<MatchmakingSession::MemberInfo &>(*iterPair.first);
            if (fullUserSessionMapMemberInfo.mMMSession.getMMSessionId() == matchmakingSessionId)
            {
                // our user is still online & linked to this matchmaking session.
                // Noting happened to our matchmaking session while we were blocked, return

                (*matchmakingSession)->setTrackingTag(request.getTrackingTag());
                return Blaze::ERR_OK;
            }
        }

        // If we get here, our user has logged out (he is not in the usersession -> mm session map), destroying our session from underneath us, exit with error.
        if (sessionErr == ERR_OK)
        {
            mMatchmakerSlave.leaveMatchmakingExternalSession(matchmakingSessionId, request.getRequest().getSessionData().getExternalMmSessionTemplateName(), externalUserInfos);
        }

        return Blaze::ERR_SYSTEM;
    }

    /*! ************************************************************************************************/
    /*!
        \brief 'runs' the matchmaker for one simulation tick.  Scans matchmaking session list,
            evaluating & finalizing the sessions it can.
        \return SCHEDULE_IDLE_IMMEDIATELY if there are still queued sessions, SCHEDULE_IDLE_NORMALLY if there are other unfinalized sessions.
    *************************************************************************************************/
    IdlerUtil::IdleResult Matchmaker::idle()
    {

        mScenarioMetrics.mTotalMatchmakingIdles.increment();
        uint64_t totalSessionEvalsPreIdle = mScenarioMetrics.mTotalMatchmakingCreateGameEvaluationsByNewSessions.getTotal() + mScenarioMetrics.mTotalMatchmakingCreateGameEvaluationsByNewSessions.getTotal();
        uint32_t sessionsAtIdleStart = (uint32_t)mCreateGameSessionMap.size();

        TimeValue idleStartTime = TimeValue::getTimeOfDay();

        TRACE_LOG("[Matchmaker] Starting matchmaker idle " << mScenarioMetrics.mTotalMatchmakingIdles.getTotal() << " at time " << idleStartTime.getMillis());

        // update each existing session's age, and mark it as dirty if the new session age changed a rule
        updateSessionAges(idleStartTime);

        // evaluate find game mode first
        TimeValue findGameEvalStartTime = TimeValue::getTimeOfDay();
        processMatchedFindGameSessions();
        TimeValue findGameDuration = TimeValue::getTimeOfDay() - findGameEvalStartTime;

        uint64_t totalSessionEvalsPostIdle = mScenarioMetrics.mTotalMatchmakingCreateGameEvaluationsByNewSessions.getTotal() + mScenarioMetrics.mTotalMatchmakingCreateGameEvaluationsByNewSessions.getTotal();

        TimeValue endTime = TimeValue::getTimeOfDay();
        mScenarioMetrics.mLastIdleLength.set((uint64_t)(endTime - idleStartTime).getMillis());
        mScenarioMetrics.mLastIdleFGLength.set((uint64_t)findGameDuration.getMillis());
        mScenarioMetrics.mLastIdleDirtySessions.set((uint64_t)mDirtySessionsAtIdleStart);
        uint32_t activeSessions = (uint32_t)mCreateGameSessionMap.size();

        TRACE_LOG("[Matchmaker] Exiting matchmaker idle " << mScenarioMetrics.mTotalMatchmakingIdles.getTotal() << "; idle duration '"
                  << mScenarioMetrics.mLastIdleLength.get() << "' ms ( '" << findGameDuration.getMillis() << "' FG), '"
                  << activeSessions << "' active sessions at idle end, '" 
                  << mNewSessionsAtIdleStart << "' new sessions, '" 
                  << mCleanSessionsAtIdleStart << "' clean sessions, '" << mDirtySessionsAtIdleStart << "' dirty sessions, '" 
                  << totalSessionEvalsPostIdle - totalSessionEvalsPreIdle << "' session evals, '"
                  << sessionsAtIdleStart - activeSessions << "' sessions finalized.");
        
        mNewSessionsAtIdleStart = 0;
        mCleanSessionsAtIdleStart = 0;
        mDirtySessionsAtIdleStart = 0;

        if (!mCreateGameSessionMap.empty() || !mFindGameSessionMap.empty())
        {
            // there are sessions that still need to be processed, but we're not falling behind
            return SCHEDULE_IDLE_NORMALLY;
        }

        return NO_NEW_IDLE;

    }



    /*! ************************************************************************************************/
    /*! \brief update the age of each matchmaking session, and mark sessions as dirty if a rule threshold changed.
        \param[in] now - the current time
    ***************************************************************************************************/
    void Matchmaker::updateSessionAges(const TimeValue &now)
    {
        MatchmakingSessionMap::iterator cgSessionMapIter = mCreateGameSessionMap.begin();
        while (cgSessionMapIter != mCreateGameSessionMap.end())
        {
            MatchmakingSession &session = *(cgSessionMapIter->second);
            updateSessionAge(now, session);

            ++cgSessionMapIter;

            // If the session is expired, and it's not locked down attempting to create a game, there is nothing left to do other than schedule
            // it to be destroyed.
            if (session.isExpired() && !session.isLockedDown(true))
            {
                timeoutExpiredSession(session.getMMSessionId());
            }
        }

        MatchmakingSessionMap::iterator fgSessionMapIter = mFindGameSessionMap.begin();
        while (fgSessionMapIter != mFindGameSessionMap.end())
        {
            MatchmakingSession &session = *(fgSessionMapIter->second);

            if (session.isCreateGameEnabled())
            {
                //we already updated this session's age when traversing mCreateGameSessionMap
                //skip doing it so we don't send a double async or inappropriately clear his dirty flag
                ++fgSessionMapIter;
                continue;
            }

            updateSessionAge(now, session);
            ++fgSessionMapIter;

            // We only check if the local subsession is locked down; we don't delay expiration if another subsession is finalizing.
            if (session.isExpired() && !session.isLockedDown(true))
            {
                timeoutExpiredSession(session.getMMSessionId());
            }
        }
    }

    void Matchmaker::updateSessionAge(const TimeValue &now, MatchmakingSession &session)
    {
        session.updateSessionAge(now);
        bool isDirty = session.updateCreateGameSession();

        if (isDirty)
        {
            evaluateDirtyCreateGameSession(session.getMMSessionId());
            ++mDirtySessionsAtIdleStart;
        }
        else
        {
            ++mCleanSessionsAtIdleStart;
        }

        if ((isDirty || session.isTimeForAsyncNotifiation(now, (uint32_t)mMatchmakingConfig.getServerConfig().getStatusNotificationPeriod().getMillis()))
            && !session.isLockedDown() && !session.isExpired())
        {
            // we send async notification when session async time is due or session criteria is changed
            // unless the session is lockedDown (finalizing)
            session.sendNotifyMatchmakingStatus();
        }
    }


    void Matchmaker::processMatchedFindGameSessions()
    {
        EA_ASSERT(mFiberGroupId != Fiber::INVALID_FIBER_GROUP_ID);

        uint32_t viableMatchSetCount = 0;
        uint32_t totalMatchSetCount = 0;
        uint32_t foundGameListCount = 0;

        TimeValue time1 = TimeValue::getTimeOfDay();

        while (!mFindGameUpdatesByMmSessionIdMap.empty())
        {
            FindGameUpdateByInstanceIdMap gameUpdatesByInstanceIdMap;
            {
                FindGameUpdatesByMmSessionIdMap::iterator gameUpdatesItr = mFindGameUpdatesByMmSessionIdMap.begin();
                gameUpdatesItr->second.swap(gameUpdatesByInstanceIdMap);
                mFindGameUpdatesByMmSessionIdMap.erase(gameUpdatesItr);
            }

            if (gameUpdatesByInstanceIdMap.empty())
                continue;

            ++foundGameListCount;
            FindGameUpdateByInstanceIdMap::iterator gameUpdatesByInstanceItr = gameUpdatesByInstanceIdMap.begin();
            Search::NotifyFindGameFinalizationUpdatePtr foundGamesUpdate = gameUpdatesByInstanceItr->second;
            MatchmakingFoundGames& foundGames = foundGamesUpdate->getFoundGames();

            MatchmakingSessionId mmSessionId = foundGames.getMatchmakingSessionId();
            MatchmakingSessionMap::iterator sessionIter = mFindGameSessionMap.find(mmSessionId);
            if (sessionIter == mFindGameSessionMap.end())
                continue;

            MatchedGameList& gameList = foundGames.getMatchedGameList();
            ++gameUpdatesByInstanceItr;

            // combine all the updates into a single object
            for (FindGameUpdateByInstanceIdMap::iterator gameUpdatesByInstanceEnd = gameUpdatesByInstanceIdMap.end(); gameUpdatesByInstanceItr != gameUpdatesByInstanceEnd; ++gameUpdatesByInstanceItr)
            {
                for (MatchedGameList::iterator i = gameUpdatesByInstanceItr->second->getFoundGames().getMatchedGameList().begin(), e = gameUpdatesByInstanceItr->second->getFoundGames().getMatchedGameList().end(); i != e; ++i)
                {
                    gameList.push_back(*i);
                }
            }

            ++totalMatchSetCount;

            MatchmakingSession *mmSession = sessionIter->second;

            LogContextOverride logAudit(mmSession->getOwnerUserSessionId());
                    
            // add the game to our tracked sorted map of games for log output later
            if (IS_LOGGING_ENABLED(Logging::TRACE) || mmSession->getDebugCheckOnly())
            {
                MatchedGameList::const_iterator iter = foundGames.getMatchedGameList().begin();
                MatchedGameList::const_iterator end = foundGames.getMatchedGameList().end();
                for (; iter != end; ++iter)
                {
                    const MatchedGame& matchedGame = **iter;
                    mmSession->addFindGameMatchScore(matchedGame.getFitScore(), matchedGame.getGameId());
                    mmSession->addDebugTopResult(matchedGame);
                }
            }

            if (mmSession->isLockedDown())
            {
                // session has been locked down for finalization
                TRACE_LOG("[Matchmaker].processMatchedFindGameSessions matchmaking session '" << mmSessionId
                    << "' found, but is currently locked down.");
            }
            else if (EA_UNLIKELY(mMatchmakerSlave.getMatchmakingJob(mmSessionId) != nullptr))
            {
                ERR_LOG("[Matchmaker].processMatchedFindGameSessions matchmaking session '" << mmSessionId
                    << "' tried finalize as find game with an existing job, current lockdown status is '"
                    << mmSession->isLockedDown() << "'.");
                // this will lock down the owning session, so it can't continue to create duplicate jobs
                mmSession->setLockdown();
            }
            else if (mmSession->shouldSkipFindGame(mScenarioMetrics.mTotalMatchmakingIdles.getTotal()))
            {
                // session has been told to not process FG matches for a bit
                TRACE_LOG("[Matchmaker].processMatchedFindGameSessions matchmaking session '" << mmSessionId
                    << "' found, but is currently skipping find game finalization.");
            }
            else
            {
                size_t matchedGameListSize = foundGames.getMatchedGameList().size();

                if (mmSession->getDebugCheckOnly())
                {
                    // Debug sessions need to go into findGameFinalizaitonSessList (above) for cleanup steps below.
                    // Note that audited sessions do not belong in this list as they should finalize normally.
                    TRACE_LOG("[Matchmaker].processMatchedFindGameSession matchmaking session '" << mmSessionId
                        << "' is debug check only, not joining matches.");

                    Fiber::CreateParams params;
                    params.groupId = mFiberGroupId;
                    gSelector->scheduleFiberCall(this, &Matchmaker::finalizeDebuggingSession, mmSessionId, "Matchmaker::finalizeDebuggingSession", params);
                }
                else if (!foundGames.getMatchedGameList().empty())
                {
                    mmSession->prepareForFinalization(mmSessionId);

                    // create a FinalizationJob to track QoS validation
                    TRACE_LOG(LOG_PREFIX << " matchmaking session(" << mmSessionId << ") adding job for find game finalization after (" << mmSession->getMatchmakingRuntime().getMillis() << ") ms.");
                    Blaze::Matchmaker::MatchmakingJob *job = BLAZE_NEW Blaze::Matchmaker::FinalizeFindGameMatchmakingJob(mMatchmakerSlave, mmSession->getOwnerUserSessionId(), mmSessionId);

                    mMatchmakerSlave.addMatchmakingJob(*job);

                    Fiber::CreateParams params;
                    params.groupId = mFiberGroupId;
                    gSelector->scheduleFiberCall(this, &Matchmaker::sendFoundGameToGameManagerMaster, foundGamesUpdate, mmSession->getTotalSessionDuration(), "Matchmaker::sendFoundGameToGameManagerMasterFiber", params);

                    ++viableMatchSetCount;
                }
                else
                {
                    TRACE_LOG("[Matchmaker].processMatchedFindGameSession matchmaking session '" << mmSessionId
                        << "' had empty matched game list.");

                }

                mmSession->updateFindGameAsyncStatus(matchedGameListSize);
            }
        }

        if (viableMatchSetCount > 0)
        {
            TimeValue time2 = TimeValue::getTimeOfDay();
            TimeValue buildRequestTime = time2 - time1;

            TRACE_LOG("[Matchmaker].processMatchedFindGameSessions '" << viableMatchSetCount << "' of '" << totalMatchSetCount
                << "' match sets were viable, from '" << foundGameListCount << "' foundGameLists, " 
                << buildRequestTime.getMillis() << " ms buildRequest");
        }
    }

    void Matchmaker::finalizeDebuggingSession(MatchmakingSessionId mmSessionId) 
    {
        // make sure the session still exists
        MatchmakingSessionMap::iterator sessionIter = mFindGameSessionMap.find(mmSessionId);
        if (sessionIter != mFindGameSessionMap.end())
        {
            MatchmakingSession *mmSession = sessionIter->second;

            // The session is now done, send debugged notification and cleanup.
            mMatchmakerSlave.removeMatchmakingJob(mmSessionId);
            mmSession->finalizeAsDebugged(SUCCESS_PSEUDO_FIND_GAME);
            destroySession(mmSessionId, SUCCESS_PSEUDO_FIND_GAME);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Sends a found game request to the master.  Found game request includes a Matchmaking Session id
        and list of matching games that we would like the master to attempt to join
    
        \param[in] foundGamesUpdate - contains a list of matched games
        \param[in] matchmakingTimeoutDuration - the maximum duration (timeout) of the matchmaking scenario (or session for legacy matchmaking)
    ***************************************************************************************************/
    void Matchmaker::sendFoundGameToGameManagerMaster(Search::NotifyFindGameFinalizationUpdatePtr foundGamesUpdate, TimeValue matchmakingTimeoutDuration)
    {
        const MatchmakingFoundGames& foundGames = foundGamesUpdate->getFoundGames();

        mScenarioMetrics.mTotalMatchmakingFindGameFinalizeBatches.increment();

        MatchedGameListByFitScoreMap matchedGameListByFitScoreMap;
        // bucketize matched games by fit score
        for (MatchedGameList::const_iterator gameIt = foundGames.getMatchedGameList().begin(), 
            gameEnd = foundGames.getMatchedGameList().end(); gameIt != gameEnd; ++gameIt)
        {
            matchedGameListByFitScoreMap[(*gameIt)->getFitScore()].push_back(*gameIt);
            mScenarioMetrics.mTotalMatchmakingFindGameFinalizeMatches.increment();
        }

        // flag indicating whether we should reinsert, and also whether we are done with this finalization attempt
        bool continueMm = true;

        for (MatchedGameListByFitScoreMap::const_reverse_iterator matchedGamesItr = matchedGameListByFitScoreMap.rbegin(),
            matchedGamesEnd = matchedGameListByFitScoreMap.rend(); 
            (continueMm && (matchedGamesItr != matchedGamesEnd)); ++matchedGamesItr)
        {
            const MatchedGamePtrList& gameList = matchedGamesItr->second;

            TRACE_LOG("[Matchmaker:sendFoundGameToGameManagerMaster] Process " << gameList.size() << " games @ fitscore " <<  matchedGamesItr->first);
            for (MatchedGamePtrList::const_iterator matchedGameItr = gameList.begin(), matchedGameEnd = gameList.end(); matchedGameItr != matchedGameEnd; ++matchedGameItr)
            {
                const MatchedGame& matchedGame = **matchedGameItr;

                MatchmakingSessionMap::const_iterator fgSessionMapIter = mFindGameSessionMap.find(foundGames.getMatchmakingSessionId());
                if ((fgSessionMapIter == mFindGameSessionMap.end()) || (fgSessionMapIter->second == nullptr))
                {
                    // if the session doesn't exist, we are done
                    continueMm = false;
                    break;
                }

                GameCapacityInfoMap::insert_return_type ret = mGameCapacityInfoMap.insert(matchedGame.getGameId());
                GameCapacityInfoPtr gameCapacityInfo = &ret.first->second;
                if (ret.second)
                {
                    gameCapacityInfo->gameId = matchedGame.getGameId();
                    gameCapacityInfo->participantCapacity = matchedGame.getParticipantCapacity();
                    gameCapacityInfo->participantCount = matchedGame.getParticipantCount();
                }
                else
                {
                    // Update if the game became more restrictive since we started pending joins against it
                    if (gameCapacityInfo->participantCount < matchedGame.getParticipantCount())
                        gameCapacityInfo->participantCount = matchedGame.getParticipantCount();
                    if (gameCapacityInfo->participantCapacity > matchedGame.getParticipantCapacity())
                        gameCapacityInfo->participantCapacity = matchedGame.getParticipantCapacity();
                }


                uint32_t mmSessionMemberCount = fgSessionMapIter->second->getPlayerCount();
                TimeValue fgTimeToMatch = fgSessionMapIter->second->getMatchmakingRuntime();

                if ((gameCapacityInfo->pendingJoinCount + gameCapacityInfo->participantCount + mmSessionMemberCount) > gameCapacityInfo->participantCapacity)
                {
                    TRACE_LOG("[Matchmaker:sendFoundGameToGameManagerMaster] Bypass Game(" << gameCapacityInfo->gameId << ") because participantCount(" 
                        << gameCapacityInfo->participantCount << ") + pendingJoinCount(" << gameCapacityInfo->pendingJoinCount << ") + session member count(" 
                        << mmSessionMemberCount << ") >= maxPlayerCapacity(" << gameCapacityInfo->participantCapacity << ")");
                    // already have enough pending joiners, skip this game
                    mScenarioMetrics.mTotalMatchmakingFindGameFinalizeSkippedAttempts.increment();
                    continue;
                }

                MatchmakingFoundGameRequest nextRequest;
                nextRequest.setMatchmakingSessionId(foundGames.getMatchmakingSessionId());
                nextRequest.setMatchmakingScenarioId(foundGames.getMatchmakingScenarioId());
                nextRequest.setOwnerUserSessionId(foundGames.getOwnerUserSessionId());
                nextRequest.setMatchmakingTimeout(matchmakingTimeoutDuration);
                foundGames.getFoundTeam().copyInto(nextRequest.getFoundTeam());
                foundGames.getGameRequest().copyInto(nextRequest.getGameRequest());
                matchedGame.copyInto(nextRequest.getMatchedGame());
                nextRequest.getMatchedGame().setTimeToMatch(fgTimeToMatch);
                nextRequest.getMatchedGame().setEstimatedTimeToMatch(fgSessionMapIter->second->getEstimatedTimeToMatch());
                nextRequest.setTrackingTag(fgSessionMapIter->second->getTrackingTag().c_str());
                nextRequest.setTotalUsersOnline(fgSessionMapIter->second->getTotalGameplayUsersAtSessionStart());
                nextRequest.setTotalUsersInGame(fgSessionMapIter->second->getTotalUsersInGameAtSessionStart());
                nextRequest.setTotalUsersInMatchmaking(fgSessionMapIter->second->getTotalUsersInMatchmakingAtSessionStart());

                // count the finalization attempts
                mScenarioMetrics.mTotalMatchmakingFindGameFinalizeAttempts.increment();
                gameCapacityInfo->pendingJoinCount += mmSessionMemberCount;

                MatchmakingFoundGameResponse response;
                BlazeRpcError err = mMatchmakerSlave.getGameManagerMaster()->matchmakingFoundGame(nextRequest, response);

                if (gameCapacityInfo->pendingJoinCount > mmSessionMemberCount)
                {
                    gameCapacityInfo->pendingJoinCount -= mmSessionMemberCount;
                }
                else
                {
                    gameCapacityInfo->pendingJoinCount = 0;
                }

                if (err != ERR_OK)
                {
                    TRACE_LOG("[Matchmaker:sendFoundGameToGameManagerMaster] Found game request to master failed with error(" << (ErrorHelp::getErrorName(err)) << ").");
                    mScenarioMetrics.mTotalMatchmakingFindGameFinalizeFailedToJoinFoundGame.increment();
                    continue;
                }

                // update game capacity info from the master response (should be more up to date than the original search slave info)
                gameCapacityInfo->participantCount = response.getFoundGameParticipantCount();
                gameCapacityInfo->participantCapacity = response.getFoundGameParticipantCapacity();

                // check if we have an outstanding FG qos validation finalization job
                Blaze::Matchmaker::MatchmakingJob* mmJob = mMatchmakerSlave.getMatchmakingJob(response.getMatchmakingSessionId());
                if (mmJob != nullptr)
                {
                    // re-fetch the ptr in case original one was invalidated after blocking above
                    auto session = findFindGameSession(response.getMatchmakingSessionId());
                    if (session != nullptr)
                    {
                        mmJob->setPinSubmission(response.getPinSubmission(), session);
                        if (response.getIsAwaitingQosValidation())
                        {
                            // this cleans up outstanding FG sessions so we can create new ones if the QoS failed and have the new avoid lists
                            mmJob->start();
                        }
                    }
                }

                // block & update external session.
                ExternalSessionApiError apiErr;
                BlazeRpcError sessErr = ERR_OK;
                if (!response.getExternalSessionParams().getExternalUserJoinInfos().empty())
                {
                    ExternalSessionJoinInitError joinInitErr;
                    sessErr = mMatchmakerSlave.joinGameExternalSession(response.getExternalSessionParams(), true, joinInitErr, apiErr);
                    for (const auto& platformErr : apiErr.getPlatformErrorMap())
                    {
                        mMatchmakingMetrics.mMetrics.mExternalSessionFailuresImpacting.increment(1, Metrics::Tag::NON_SCENARIO_NAME, Metrics::Tag::NON_SCENARIO_NAME, Metrics::Tag::NON_SCENARIO_VERSION, platformErr.first);
                        ERR_LOG("[Matchmaker::sendFoundGameToGameManagerMaster] Failed to joining external session after a match into game(" << response.getJoinedGameId()
                            << "). Error: " << ErrorHelp::getErrorName(platformErr.second->getBlazeRpcErr()) << " platform(" << ClientPlatformTypeToString(platformErr.first) << ")"); 
                    }
                }

                MatchmakingResult mmResult = SUCCESS_JOINED_EXISTING_GAME;
                // Update metrics, send final async status. If there was an external session error terminate the session.
                auto mmSession = findFindGameSession(response.getMatchmakingSessionId());
                if (mmSession != nullptr)
                {
                    LogContextOverride logAudit(mmSession->getOwnerUserSessionId());

                    TRACE_LOG("[Matchmaker:sendFoundGameToGameManagerMaster] send final async status for " << ((sessErr==ERR_OK)?"successful":"failed") << " find game, for user " << response.getOwnerUserSessionId() << ", matchmaking session " << response.getMatchmakingSessionId());
                    mmSession->sendNotifyMatchmakingStatus();
                    if (sessErr != ERR_OK)
                    {
                        mmSession->finalizeAsTerminated();
                        mmResult = SESSION_TERMINATED;
                    }
                    else
                    {
                        // notify client about your reserved external players. reuse helper to populate single notification
                        NotifyMatchmakingReservedExternalPlayersMap notifs;
                        mmSession->fillNotifyReservedExternalPlayers(response.getJoinedGameId(), notifs);
                        if (!notifs.empty())
                        {
                            mMatchmakerSlave.sendNotifyServerMatchmakingReservedExternalPlayersToSliver(UserSession::makeSliverIdentity(notifs.begin()->first), &notifs.begin()->second);
                        }

                        // on success, print out top matches.
                        TRACE_LOG("[MatchmakingResult Session '" << mmSession->getMMSessionId() << "'] Joined Game '" << response.getJoinedGameId() << "'");
                        mmSession->outputFindGameMatchScores();

                        // We also log out debugging information if the session is audited.
                        if (gUserSessionManager->shouldAuditSession(mmSession->getOwnerUserSessionId()))
                        {
                            mmSession->outputFindGameDebugResults();
                        }

                        mmSession->updateMatchmakedGameMetrics(response);
                    }
                }

                // reget the job, because we had blocked joining the external session above
                mmJob = mMatchmakerSlave.getMatchmakingJob(response.getMatchmakingSessionId());

                // only destroy the session if we're not waiting on Qos
                if (!response.getIsAwaitingQosValidation() && (mmJob != nullptr))
                {
                    mmJob->sendNotifyMatchmakingSessionConnectionValidated(response.getJoinedGameId(), true);
                    mMatchmakerSlave.removeMatchmakingJob(response.getMatchmakingSessionId());
                }
                else if (mmResult == SESSION_TERMINATED)
                {
                    mExternalSessionTerminated = true;
                    mMatchmakerSlave.removeMatchmakingJob(response.getMatchmakingSessionId());
                    destroySession(response.getMatchmakingSessionId(), mmResult);

                }
                else if (response.getIsAwaitingQosValidation() && (mmJob != nullptr))
                {
                    mmJob->setAwaitingQos();
                    mmJob->sendDelayedNotifyMatchmakingSessionConnectionValidatedIfAvailable();
                }

                continueMm = false;//exits outer loop
                break;
            }
        }

        if (continueMm)
        {
            mScenarioMetrics.mTotalMatchmakingFindGameFinalizeFailedToJoinAnyGame.increment();

            // remove job, no match
            mMatchmakerSlave.removeMatchmakingJob(foundGames.getMatchmakingSessionId());

            // return the remaining sessions to MM, they failed to join any games
            Blaze::Search::MatchmakingSessionIdList failedToJoinSessionList;
            failedToJoinSessionList.push_back(foundGames.getMatchmakingSessionId());

            // schedule another idle and reinsert if cg sessions were re-inserted.
            reinsertFailedToFinalizeSessions(failedToJoinSessionList, false, SKIP_FIND_GAME_NEXT_IDLE, Rete::INVALID_PRODUCTION_NODE_ID);
        }
    }


    void Matchmaker::startFinalizingSession(const MatchmakingSession& session)
    {
        Blaze::Matchmaker::MatchmakingJob *newJob = mMatchmakerSlave.getMatchmakingJob(session.getMMSessionId());
        if(newJob != nullptr)
        {
            // kick off the job AFTER the remove
            newJob->start();
        }
        else
        {
            // log error, job went missing somehow
            ERR_LOG(LOG_PREFIX << " Matchmaker::startFinalizationJob - finalization job for new matchmaking session (" 
                << session.getMMSessionId() << "), not found.");
        }
    }

    void Matchmaker::evaluateNewCreateGameSession(MatchmakingSessionId newSessionId) 
    {
        // find the session in the create game map
        MatchmakingSession* newSession = findCreateGameSession(newSessionId);
        if (newSession == nullptr)
        {
            TRACE_LOG(LOG_PREFIX << "[Matchmaker::evaluateNewCreateGameSession] Unable to find session id '" << newSessionId << "' in create game map.");
            return;
        }

        bool isStartDelayOver = newSession->getIsStartDelayOver();
        if (!isStartDelayOver)
        {
            // Delay the start of the session, 
            SPAM_LOG(LOG_PREFIX << "[Matchmaker::evaluateNewCreateGameSession] Session id '" << newSessionId << "' is still waiting for startDelay to end.");
            gSelector->scheduleTimerCall(newSession->getSessionStartTime(), this, &Matchmaker::evaluateNewCreateGameSession, newSessionId, "Matchmaker::evaluateNewCreateGameSession");
            return;
        }
        
        LogContextOverride logAudit(newSession->getOwnerUserSessionId());
        TRACE_LOG(LOG_PREFIX << " evaluating new Session " << newSession->getMMSessionId() << " created by " << newSession->getOwnerBlazeId() << ".");

        if (!newSession->isCreateGameEnabled())
        {
            TRACE_LOG(LOG_PREFIX << " skipping new Session " << newSession->getMMSessionId() << " (session not enabled for createGame)");
            return;
        }
        if (newSession->isLockedDown())
        {
            TRACE_LOG(LOG_PREFIX << " skipping new Session " << newSession->getMMSessionId() << " (session is already locked down by other scenario subsession)");
            return;
        }
       

        MatchedSessionsList matchedSessionsList;

        // Evaluate the session, then add it to the full session list, so other sessions can evaluate against it:
        newSession->evaluateCreateGame(mFullSessionList, matchedSessionsList);
        for (auto matchedSessionId : matchedSessionsList)
        {
            queueSessionForDirtyEvaluation(matchedSessionId);
        }
        addSession(*newSession); 

        // make sure some session I just matched didn't lock me down.
        if (!newSession->isLockedDown())
        {
            // all new sessions are considered dirty, attempt to finalize now that we've evaluated everyone
            // and let any matches finalize
            queueSessionForDirtyEvaluation(newSession->getMMSessionId());
        }
    }


    void Matchmaker::queueSessionForNewEvaluation(MatchmakingSessionId sessionId) 
    {
        mEvaluateSessionQueue.queueFiberJob<Matchmaker, MatchmakingSessionId>(this, &Matchmaker::evaluateNewCreateGameSession, sessionId, "Matchmaker::evaluateNewCreateGameSession");
    }

    void Matchmaker::queueSessionForDirtyEvaluation(MatchmakingSessionId sessionId)
    {
        FiberJobQueue::JobParams jobParams("Matchmaker::evaluateDirtyCreateGameSession");
        jobParams.uniqueJobTag = (uint64_t)sessionId;
        mEvaluateSessionQueue.queueFiberJob<Matchmaker, MatchmakingSessionId>(this, &Matchmaker::evaluateDirtyCreateGameSession, sessionId, jobParams);
    }

    void Matchmaker::evaluateDirtyCreateGameSession(MatchmakingSessionId existingSessionId)
    {
        // find the session in the create game map
        MatchmakingSession* existingSession = findCreateGameSession(existingSessionId);
        if (existingSession == nullptr)
        {
            TRACE_LOG(LOG_PREFIX << "[Matchmaker::evaluateDirtyCreateGameSession] Unable to find session id '" << existingSessionId << "' in create game map.");
            return;
        }

        bool isStartDelayOver = existingSession->getIsStartDelayOver();
        if (!isStartDelayOver)
        {
            SPAM_LOG(LOG_PREFIX << "[Matchmaker::evaluateDirtyCreateGameSession] Session id '" << existingSessionId << "' is still waiting for startDelay to end.");
            // We don't need to reinsert the session here, updateSessionAge will call evaluateDirtyCreateGameSession regularly for us. 
            // (And the call in evaluateNewCreateGameSession already is after the delay)
            return;
        }

        LogContextOverride logAudit(existingSession->getOwnerUserSessionId());
        TRACE_LOG(LOG_PREFIX << " Checking for finalization on Session " << existingSession->getMMSessionId() << " created by " << existingSession->getOwnerBlazeId() << ".");

        if (!existingSession->isCreateGameEnabled())
        {
            TRACE_LOG(LOG_PREFIX << " skipping existing Session " << existingSession->getMMSessionId() << " (session not enabled for createGame)");
            return;
        }
        if (existingSession->isLockedDown())
        {
            TRACE_LOG(LOG_PREFIX << " skipping existing Session " << existingSession->getMMSessionId() << " (session is already locked down by other scenario subsession)");
            return;
        }

        TRACE_LOG(LOG_PREFIX << " Attempting to finalize existingSession " << existingSession->getMMSessionId() << ".");

        // if we moved sessions, see if we can finalize
        // MM_AUDIT: Potential optimization is to bubble up which rule has changed and only try to finalize if 
        // we have a new matched session or if the game size rule has decayed to allow me to match myself.
        MatchmakingSessionList finalizedSessionList;
        existingSession->attemptGreedyCreateGameFinalization(finalizedSessionList);
        if (!finalizedSessionList.empty())
        {
            // chosen sessions are locked down here
            // important that they are not locked till chosen.
            removeFinalizedSessions(finalizedSessionList, existingSession->getMMSessionId());
            startFinalizingSession(*existingSession);
        }
    }

    // remove the finalizing session and advance the supplied iterator if it was invalidated by the destroy
    void Matchmaker::removeFinalizedSessions( MatchmakingSessionList &finalizedSessionList, MatchmakingSessionId finalizingSessionId )
    {
        MatchmakingSessionList::const_iterator finalizedListIter = finalizedSessionList.begin();
        MatchmakingSessionList::const_iterator finalizedListEnd = finalizedSessionList.end();
        for ( ; finalizedListIter != finalizedListEnd; ++finalizedListIter )
        {
            // we need to set the lockdown on the parent session, only available as const.
            MatchmakingSession* finalizingSession = const_cast<MatchmakingSession*>(*finalizedListIter);

            TRACE_LOG(LOG_PREFIX << " matchmaking session(" << finalizingSession->getMMSessionId() << ") locking down for finalization after (" 
                      << finalizingSession->getMatchmakingRuntime().getMillis() << ") ms.");

            finalizingSession->prepareForFinalization(finalizingSessionId);
        }
    }

    void Matchmaker::reinsertFailedToFinalizeSessions(Blaze::Search::MatchmakingSessionIdList &reinsertedSessionList, 
        bool restartFindGameSessions, ReinsertionActionType reinsertionAction, Rete::ProductionNodeId productionNodeId)
    {
        bool timedOutSessions = false;
        Blaze::Search::MatchmakingSessionIdList::iterator reinsertedListIter = reinsertedSessionList.begin();
        Blaze::Search::MatchmakingSessionIdList::iterator reinsertedListEnd = reinsertedSessionList.end();
        for (; reinsertedListIter != reinsertedListEnd; ++reinsertedListIter)
        {
            const auto mmSessionId = *reinsertedListIter;
            MatchmakingSession* reinsertedSession = findCreateGameSession(mmSessionId);

            if (reinsertedSession != nullptr)
            {
                TRACE_LOG(LOG_PREFIX << "[Matchmaker::reinsertFailedToFinalizeSessions()] found session id '" << mmSessionId << "' in create game map.");
                if (reinsertionAction == STOP_EVALUATING_CREATE_GAME)
                {
                    // If FG MM was enabled for this session, we keep it alive but turn off the CG mode:
                    if (reinsertedSession->isFindGameEnabled())
                    {
                        // reinsertedSession will still be valid after this, and it will exist in the FG session map, so it won't be lost.
                        INFO_LOG(LOG_PREFIX << "[Matchmaker::reinsertFailedToFinalizeSessions()] session id '" << mmSessionId << "' will have create game mode disabled for reinsertion.");

                        mCreateGameSessionMap.erase(mmSessionId);
                        mScenarioMetrics.mCreateGameMatchmakingSessions.decrement();

                        // Remove the session from Full List so that other CG MMing sessions won't try to evaluate against it. 
                        removeSession(*reinsertedSession);

                        // need to 'disable' create game mode on the session itself so we don't try to remove it from the queue when the session is actually destroyed!
                        // no other clean up is required, as that was handled when this session entered finalization
                        reinsertedSession->clearCreateGameEnabled();

                        mScenarioMetrics.mTotalMatchmakingSessionsDisabledCreateGameMode.increment();
                    }
                    else
                    {
                        // Since no dedicated servers were available, we just destroy this session, sending this ERR back to the client. 
                        reinsertedSession->finalizeAsGameSetupFailed();
                        reinsertedSession->activateReinsertedSession();   // We need to clear the lock that we had.  (Also would happen in destroy call below).
                        destroySession(reinsertedSession->getMMSessionId(), SESSION_ERROR_GAME_SETUP_FAILED);
                        reinsertedSession = nullptr;
                        
                    }
                }
            }
            else if ((reinsertedSession = findFindGameSession(mmSessionId)) != nullptr)
            {
                TRACE_LOG(LOG_PREFIX << "[Matchmaker::reinsertFailedToFinalizeSessions()] found session id '" << mmSessionId << "' in find game map.");
            }

            if (reinsertedSession == nullptr)
            {
                // session has been deleted (logout or cancel)
                // skip ahead to next session in the list
                continue;
            }

            if (reinsertionAction != STOP_EVALUATING_CREATE_GAME)
            {
                reinsertedSession->activateReinsertedSession();
                reinsertedSession->scheduleExpiry(); // ensure that expiry is rescheduled (in case session expiry timer no-op'ed) while locked down
                if (reinsertedSession->isExpired())
                {
                    TRACE_LOG(LOG_PREFIX << "[Matchmaker].reinsertFailedToFinalizeSessions: session(" << mmSessionId << ") was un locked and scheduled for timeout.");
                }
                continue;
            }
            
            if ((reinsertionAction == SKIP_FIND_GAME_NEXT_IDLE) && (reinsertedSession->isFindGameEnabled() && reinsertedSession->isCreateGameEnabled()))
            {
                reinsertedSession->setNextIdleForFindGameProcessing(mScenarioMetrics.mTotalMatchmakingIdles.getTotal() + 1);
            }
            else
            {
                reinsertedSession->setNextIdleForFindGameProcessing(0);
            }

            reinsertedSession->updateSessionAge(TimeValue::getTimeOfDay());

            // expired sessions will timeout next idle
            if (!reinsertedSession->isExpired())
            {
                if (!reinsertedSession->isLockedDown())
                {
                    ERR_LOG(LOG_PREFIX << "[Matchmaker::reinsertFailedToFinalizeSession()] session '" << mmSessionId << "' was reinserted, but not locked down.");
                }

                // unlock the session, and check it as dirty as new sessions have been evaluating it
                //  while it has been locked down.
                reinsertedSession->activateReinsertedSession();

                if (reinsertedSession->isCreateGameEnabled())
                {
                    // perhaps we should somehow delaying session's ability to finalize
                    // right away.
                    if (reinsertionAction == EVALUATE_SESSION)
                    {
                        // clean out any matches and go back and re evaluate everybody
                        // This is required for qos failures because they modify the qos avoid players rule
                        reinsertedSession->sessionCleanup();
                        TRACE_LOG(LOG_PREFIX << "[Matchmaker::reinsertFailedToFinalizeSession()] session '" << mmSessionId << "' was un locked and reinserted for new evaluation.");
                        queueSessionForNewEvaluation(reinsertedSession->getMMSessionId());
                    }
                    else
                    {
                        TRACE_LOG(LOG_PREFIX << "[Matchmaker::reinsertFailedToFinalizeSession()] session '" << mmSessionId << "' was un locked and reinserted for dirty evaluation.");
                        queueSessionForDirtyEvaluation(reinsertedSession->getMMSessionId());
                    }

                }   

                if (restartFindGameSessions && reinsertedSession->isFindGameEnabled())
                {
                    reinsertedSession->updateStartMatchmakingInternalRequestForFindGameRestart();
                    Fiber::CreateParams params;
                    params.namedContext = "MatchmakerSlaveImpl::restartFindGameSessionAfterReinsertion";
                    gFiberManager->scheduleCall<Blaze::GameManager::Matchmaker::Matchmaker, Blaze::GameManager::MatchmakingSessionId>(this, 
                        &Matchmaker::restartFindGameSessionAfterReinsertion, reinsertedSession->getMMSessionId(), params);
                }
            }
            else
            {
                // unlock to be timed out on next idle
                reinsertedSession->activateReinsertedSession();
                TRACE_LOG(LOG_PREFIX << "[Matchmaker::reinsertFailedToFinalizeSession()] session '" << mmSessionId << "' was un locked and idle scheduled for timeout.");
                timedOutSessions = true;
            }
        }

        // schedule an idle to time out sessions
        if (timedOutSessions)
        {
            scheduleNextIdle(TimeValue::getTimeOfDay(), mMatchmakingConfig.getServerConfig().getIdlePeriod());
        }
    }

    void Matchmaker::timeoutExpiredSession(MatchmakingSessionId expiredSessionId)
    {
        MatchmakingSession* expiredSession = findSession(expiredSessionId);
        if (expiredSession == nullptr)
        {
            TRACE_LOG(LOG_PREFIX << "[Matchmaker::timeoutExpiredSession] Unable to find session id '" << expiredSessionId << "' in game map.");
            return;
        }

        if (expiredSession->isExpired())
        {
            // we send async notification when session expired
            expiredSession->sendNotifyMatchmakingStatus();
            expiredSession->finalizeAsTimeout();
            // if the session is in the FG map & queued, this will clean it up
            destroySession(expiredSessionId, SESSION_TIMED_OUT);
        }
    }



    bool Matchmaker::validateConfig(const Blaze::Matchmaker::MatchmakerConfig& config, const Blaze::Matchmaker::MatchmakerConfig *referenceConfig, ConfigureValidationErrors& validationErrors)
    {
        if (MatchmakingConfig::isUsingLegacyRules(config))
        {
            GameManager::Rete::StringTable stringTable;
            RuleDefinitionCollection collection(mMatchmakerSlave.getMetricsCollection(), stringTable, mMatchmakerSlave.getLogCategory(), false);
            collection.validateConfig(config.getMatchmakerSettings(), &referenceConfig->getMatchmakerSettings(), validationErrors);
        }

        return validationErrors.getErrorMessages().empty();
    }

    /*! ************************************************************************************************/
    /*!
        \brief perform initial matchmaking configuration (parse the matchmaking config file & setup
            all rule definitions).
    
        \param[in]    configMap - the config map containing the matchmaking config block
        \param[out] true if all settings configured properly, false if default settings are used
    *************************************************************************************************/
    bool Matchmaker::configure(const Blaze::Matchmaker::MatchmakerConfig& config)
    {
        auto& mmServerConfig = config.getMatchmakerSettings();

        if (mFiberGroupId == Fiber::INVALID_FIBER_GROUP_ID)
            mFiberGroupId = Fiber::allocateFiberGroupId();

        TRACE_LOG("[Matchmaker].configure: Starting Matchmaking Configuration.");
        mMatchmakingConfig.setConfig(mmServerConfig);

        EA_ASSERT(mRuleDefinitionCollection == nullptr);
        mRuleDefinitionCollection = BLAZE_NEW RuleDefinitionCollection(mMatchmakerSlave.getMetricsCollection(), mReteNetwork.getWMEManager().getStringTable(), mMatchmakerSlave.getLogCategory(), false);
        if (!(mRuleDefinitionCollection->initRuleDefinitions(mMatchmakingConfig, mmServerConfig, &config)))
        {
            // init failed
            return false;
        }

        clearQosMetrics();

        //set configs into response object for client queries
        initializeMatchmakingConfigResponse();

        // start matchmaking status updates
        mStatusUpdateTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + STATUS_UPDATE_TIME, this, &Matchmaker::onStatusUpdateTimer, "MM::MasterStatusUpdateTimer");

        mEvaluateSessionQueue.setQueuedJobTimeout(TimeValue(0)); // no job timeouts
        mEvaluateSessionQueue.setMaximumWorkerFibers(Matchmaker::WORKER_FIBERS);
        mEvaluateSessionQueue.setJobQueueCapacity(Matchmaker::JOB_QUEUE_CAPACITY);


        return true;
    }


    /*! ************************************************************************************************/
    /*!
        \brief Clears the QOS metrics so they can be used once again.
    *************************************************************************************************/
    void Matchmaker::clearQosMetrics()
    {
        // prep metrics counts for QosValidation
        mScenarioMetrics.mQosValidationMetrics.mTotalMatchmakingSessions.reset();
    }

    /*! ************************************************************************************************/
    /*!
        \brief Reconfigure matchmaking using the new matchmaking configuration.
            - parse the matchmaking config file & make sure there are no errors.
            - If there are no errors kick everyone out of matchmaking and clean up the old data.
            - Set the new rule definitions and other matchmaking data.
    
        \param[in]    configMap - the config map containing the matchmaking config block
        \param[out] true if the new settings configed properly, false if the old settings are used.
    *************************************************************************************************/
    bool Matchmaker::reconfigure(const Blaze::Matchmaker::MatchmakerConfig& config)
    {
        gSelector->cancelTimer(mStatusUpdateTimerId);

        // Nuclear Option: (Kill those pointers. Kill'em dead)
        terminateAllSessions();
        
        // Clear the old metrics, because we may now have new metrics: 
        clearQosMetrics();

        cleanUpConfigResponse();

        delete mRuleDefinitionCollection;
        mRuleDefinitionCollection = nullptr;
        
        configure(config);
        
        // We just always return true on reconfigure
        return true;
    }


    /*! ************************************************************************************************/
    /*!
        \brief Terminate all MM sessions.  This will delete and erase all MM sessions and send a 
            notification of MM finished with a result of SESSION_TERMINATED.
    *************************************************************************************************/
    void Matchmaker::terminateAllSessions()
    {
        uint32_t numExpiredSessions = 0;

        while (!mCreateGameSessionMap.empty())
        {
            auto terminatedSessionItr = mCreateGameSessionMap.begin();
            auto terminatedSession = terminatedSessionItr->second;

            if (terminatedSession->isFindGameEnabled())
            {
                // creates a fiber for the termination, so we don't wind up having the session deleted while blocking
                cancelMatchmakingFindGameSession(terminatedSession->getMMSessionId(), GameManager::SESSION_TERMINATED);
            }

            // asyn notification to let the client know their session has been terminated.
            terminatedSession->sendNotifyMatchmakingStatus();
            terminatedSession->finalizeAsTerminated();

            // increment counters
            ++numExpiredSessions;
            incrementTotalFinishedMatchmakingSessionsMetric(SESSION_TERMINATED, *terminatedSession, true);

            // destroy the session
            // Do not need to use destroySession(Session*) as we are removing all sessions, destroySession
            // is currently used to destroy individual sessions and cleanup create game sessions.
            // This is inefficient when all sessions will be destroyed.
            mCreateGameSessionMap.erase(terminatedSessionItr);
            mScenarioMetrics.mCreateGameMatchmakingSessions.decrement();

            // however, if it's a find game session, we need to remove it from that map before deleting it.
            if (terminatedSession->isFindGameEnabled())
            {
                mFindGameSessionMap.erase(terminatedSession->getMMSessionId());
                mScenarioMetrics.mFindGameMatchmakingSessions.decrement();
            }

            // If this is left over in any list (queue, session list, etc) get rid of it.
            removeSession(*terminatedSession);

            delete terminatedSession;
        }

        while (!mFindGameSessionMap.empty())
        {
            auto terminatedSessionItr = mFindGameSessionMap.begin();
            auto terminatedSession = terminatedSessionItr->second;

            // creates a fiber for the termination, so we don't wind up having the session deleted while blocking
            cancelMatchmakingFindGameSession(terminatedSession->getMMSessionId(), GameManager::SESSION_TERMINATED);

            // asyn notification to let the client know their session has been terminated.
            terminatedSession->sendNotifyMatchmakingStatus();
            terminatedSession->finalizeAsTerminated();

            // increment counters
            ++numExpiredSessions;
            incrementTotalFinishedMatchmakingSessionsMetric(SESSION_TERMINATED, *terminatedSession, true);

            // destroy the session
            // Do not need to use destroySession(Session*) as we are removing all sessions, destroySession
            // is currently used to destroy individual sessions and cleanup create game sessions.
            // This is inefficient when all sessions will be destroyed.
            mFindGameSessionMap.erase(terminatedSessionItr);
            mScenarioMetrics.mFindGameMatchmakingSessions.decrement();

            // we've already deleted CG sessions from that map,
            // but it may be queued, so we check and remove it from any intrusive lists
            removeSession(*terminatedSession);

            
            delete terminatedSession;
        }

        if (mScenarioMetrics.mGaugeMatchmakingUsersInSessions.get() > 0)
        {
            WARN_LOG(LOG_PREFIX << "terminateAllSessions: mGaugeMatchmakingUsersInSessions should be zero if there are no CG/FG sessions");
            mScenarioMetrics.mGaugeMatchmakingUsersInSessions.set(0);
        }

        // Any matchmaking sessions that were not yet in the above maps/lists, should be in the below map.
        while (!getUserSessionsToMatchmakingMemberInfoMap().empty())
        {
            FullUserSessionsMap::const_iterator itr = getUserSessionsToMatchmakingMemberInfoMap().begin();
            const MatchmakingSession::MemberInfo &member = static_cast<const MatchmakingSession::MemberInfo &>(*itr);
            destroySessionForUserSessionId(member.mUserSessionId);
        }

        TRACE_LOG(LOG_PREFIX << ".terminateAllSessions: " << numExpiredSessions << " active " 
                  << " sessions terminated, " << mCreateGameSessionMap.size() << " remain.");
    }


    /*! ************************************************************************************************/
    /*!
    \brief cache a matchmaking config response object for filling client requests.

    *************************************************************************************************/
    void Matchmaker::initializeMatchmakingConfigResponse()
    {
        // initGenericRuleConfig for Custom Rules (GameAttributeRules and PlayerAttributeRules)
        const RuleDefinitionList& ruleDefinitionList = mRuleDefinitionCollection->getRuleDefinitionList();
        size_t numRules = ruleDefinitionList.size();
        for(size_t i=0; i<numRules; ++i)
        {
            if(!ruleDefinitionList[i]->isDisabled())
            {
                ruleDefinitionList[i]->initRuleConfig(mMatchmakingConfigResponse);
            }
        }
    }

    /*! ************************************************************************************************/
    /*!
        \brief Clean up data in the config response including deleting all of the predefined rules and generic rules.

    *************************************************************************************************/
    void Matchmaker::cleanUpConfigResponse()
    {
        // Cleanup Old Config Responses
        mMatchmakingConfigResponse.getPredefinedRules().clear();
        mMatchmakingConfigResponse.getGenericRules().clear();
    }

    Blaze::Matchmaker::GetMatchmakingConfigError::Error Matchmaker::setMatchmakingConfigResponse(GetMatchmakingConfigResponse *response) const 
    { 
        mMatchmakingConfigResponse.copyInto(*response);

        return Blaze::Matchmaker::GetMatchmakingConfigError::ERR_OK;
    }

    // MM_SHARDING_TODO - need to ensure we clean up from both maps
    /*! ************************************************************************************************/
    /*!
        \brief delete the supplied session and erase it from the session map.  Also cleans up any
            other createGame pools that the session was a part of.
        \param[in]  result The result of the matchmaking session. This value is used to increment the MM result & duration metrics.
        \param[in]  the matched game's ID (if successful matchmaking) to dispatch to members of group sessions
    *************************************************************************************************/
    void Matchmaker::destroySession(MatchmakingSessionId sessionId, MatchmakingResult result, GameId gameId /*= GameManager::INVALID_GAME_ID*/)
    {
        // remove the dying session from any other createGame sessions that it was matched into.
        MatchmakingSession* dyingSession = findSession(sessionId);
        if (dyingSession == nullptr)
        {
            return;
        }

        TRACE_LOG(LOG_PREFIX << "Destroying matchmaking session " << dyingSession->getMMSessionId() << "...");


        //If this is left over in any list (queue, session list, etc) get rid of it.
        removeSession(*dyingSession);

        // save off values for updating external sessions before deleting
        const MatchmakingSessionId mmSessionId = dyingSession->getMMSessionId();
        const MatchmakingScenarioId mmScenarioId = dyingSession->getMMScenarioId();
        const XblSessionTemplateName sessionTemplateName = dyingSession->getExternalSessionTemplateName();
        const UserSessionId ownerUserSessionId = dyingSession->getOwnerUserSessionId();
        ExternalUserLeaveInfoList externalUserInfos;
        userSessionInfosToExternalUserLeaveInfos(dyingSession->getMembersUserSessionInfo(), externalUserInfos);


        // remove from all maps before deleting
        mCreateGameSessionMap.erase(sessionId);
        mScenarioMetrics.mCreateGameMatchmakingSessions.decrement();

        bool hadFindGame = mFindGameSessionMap.erase(sessionId) > 0;
        if (hadFindGame)
            mScenarioMetrics.mFindGameMatchmakingSessions.decrement();


        // Update the metrics:
        incrementTotalFinishedMatchmakingSessionsMetric(result, *dyingSession);

        incrementFinishedMmSessionDiagnostics(result, *dyingSession);

        if (mCreateGameSessionMap.empty() && mFindGameSessionMap.empty() && mScenarioMetrics.mGaugeMatchmakingUsersInSessions.get() > 0)
        {
            WARN_LOG(LOG_PREFIX << "destroySession: mGaugeMatchmakingUsersInSessions should be zero if there are no CG/FG sessions. createGameSessionMap size(" << mCreateGameSessionMap.size() << "), findGameSessionMap size(" << mFindGameSessionMap.size() << "), num users in sessions(" << mScenarioMetrics.mGaugeMatchmakingUsersInSessions.get() << ")");
            mScenarioMetrics.mGaugeMatchmakingUsersInSessions.set(0);
        }
        
        // the dying sessions ping site, tracking count of sessions by site
        PingSiteAlias pingSite = UNKNOWN_PINGSITE;
        if (dyingSession->getPrimaryUserSessionInfo() != nullptr)
        {
            pingSite = dyingSession->getPrimaryUserSessionInfo()->getBestPingSiteAlias();
        }


        auto sessionCountItr = mSessionsByScenarioMap.find(pingSite);

        if (sessionCountItr != mSessionsByScenarioMap.end())
        {
            MatchmakingSessionsByPingSiteMap& sessionsBySite = sessionCountItr->second;
            auto siteCountItr = sessionsBySite.find(pingSite);
            if (siteCountItr != sessionsBySite.end())
            {
                if (siteCountItr->second > 0)
                {
                    siteCountItr->second--;
                }
                else
                {
                    siteCountItr->second = 0;
                }
            }
        }


        if (result == SESSION_QOS_VALIDATION_FAILED)
        {
            dyingSession->finalizeAsFailedQosValidation();
        }

        // tell the members that MM ended

        // delete the session before possibly blocking below to ensure we're the ones deleting it
        delete dyingSession;

        if (hadFindGame)
        {
            cancelMatchmakingFindGameSession(mmSessionId, result);
        }

        // Send a notification that the matchmaking is done, and does not need to be tracked for the user anymore. 
        {
            // init the notify matchmaking finished tdf
            NotifyMatchmakingFinished notifyResult;
            notifyResult.setUserSessionId(ownerUserSessionId);
            notifyResult.setSessionId(mmSessionId);
            notifyResult.setScenarioId(mmScenarioId);
            notifyResult.setMatchmakingResult(result);

            // send the notification
            mMatchmakerSlave.sendNotifyMatchmakingFinishedToSliver(UserSession::makeSliverIdentity(ownerUserSessionId), &notifyResult);
        }

        // update external session
        leaveMatchmakingExternalSession(mmSessionId, sessionTemplateName, externalUserInfos);
    }

    // issues terminate requests to search slaves to stop FG search while QoS validation is in-progress
    void Matchmaker::stopOutstandingFindGameSessions(MatchmakingSessionId mmSessionId, GameManager::MatchmakingResult result)
    {
        if(mFindGameSessionMap.find(mmSessionId) != mFindGameSessionMap.end())
        {
            // we'll restart the FG search if QoS fails, but with a new session that has the updated avoid list.
            cancelMatchmakingFindGameSession(mmSessionId, result);
        }
    }


    // helper, spawns new blockable fiber, to avoid blocking calling code, to prevent any potential timing issues/crashes etc.
    void Matchmaker::leaveMatchmakingExternalSession(MatchmakingSessionId mmSessionId, const XblSessionTemplateName& sessionTemplateName, ExternalUserLeaveInfoList& externalUserInfos) const
    {
        // need to be careful to manage usage of sessionTemplateName and externalUserInfos within MatchmakerSlaveImpl::leaveMatchmakingExternalSession() where blocking calls are involved
        Fiber::CreateParams params;
        params.namedContext = "MatchmakerSlaveImpl::leaveMatchmakingExternalSession";
        gFiberManager->scheduleCall<Blaze::Matchmaker::MatchmakerSlaveImpl, Blaze::GameManager::MatchmakingSessionId, const XblSessionTemplateName&, Blaze::ExternalUserLeaveInfoList&>(&mMatchmakerSlave,
            &Blaze::Matchmaker::MatchmakerSlaveImpl::leaveMatchmakingExternalSession, mmSessionId, sessionTemplateName, externalUserInfos, params);
    }

    /*! ************************************************************************************************/
    /*!
        \brief destroy all matchmaking sessions owned by the supplied user.
    
        \param[in]    user The user who's disconnected (or being destroyed)
    *************************************************************************************************/
    void Matchmaker::destroySessionForUserSessionId(UserSessionId dyingUserSessionId)
    {
        FullUserSessionsMap::iterator iter;

        // Supports multiple User Sessions for the same user.
        // Can't use equal range because the second iterator can be invalidated.
        while ((iter = getUserSessionsToMatchmakingMemberInfoMap().find(dyingUserSessionId)) != getUserSessionsToMatchmakingMemberInfoMap().end())
        {
            MatchmakingSession::MemberInfo &member = static_cast<MatchmakingSession::MemberInfo &>(*iter);

            MatchmakingSessionId mmSessionId = member.mMMSession.getMMSessionId();
            {
                if (member.isMMSessionOwner())
                {
                    //If this is the session owner, kill the whole session.
                    auto mmSession = findSession(mmSessionId);
                    if (mmSession != nullptr)
                    {
                        // For those users whose MM sessions got destroyed because MM group initiator left, send NotifyMatchmakingFailed.
                        member.mMMSession.finalizeAsTerminated();
                        destroySession(mmSessionId, SESSION_TERMINATED);
                    }
                    else
                    {
                        // It is possible the finalization job is still active when we logged out. Or, MM reconfigured before the matchmaking session could get into any map/queued-list above.
                        // Just delete the matchmaking session.  The finalization job will always check that the user is
                        // logged in still before executing further actions.  It will also timeout if not completed.
                        TRACE_LOG("[Matchmaker] Matchmaking session(" << mmSessionId << ") being destroyed before matchmaking finalization job completed.");
                        delete &(member.mMMSession);
                    }
                }
                else
                {
                    // removing a member, immediately makes the session potentially eligible to finalize.
                    member.mMMSession.removeMember(member);
                    // Remove the player from the gauge group players metric:
                    mScenarioMetrics.mGaugeMatchmakingUsersInSessions.decrement();

                    // removing a member, immediately makes the session potentially eligible to finalize.
                    evaluateDirtyCreateGameSession(mmSessionId);
                }
            }
        }
    }

    void Matchmaker::getStatusInfo(ComponentStatus& status) const 
    {
        Blaze::ComponentStatus::InfoMap& parentStatusMap=status.getInfo();

        StringBuilder tempNameString;
        StringBuilder tempValueString;
#define ADD_VALUE(name, value) { parentStatusMap[ (tempNameString << name).get() ]= (tempValueString << value).get(); tempNameString.reset(); tempValueString.reset(); }

        // Global Metrics:
        ADD_VALUE("MMGaugeActiveMatchmakingUsers", mScenarioMetrics.mGaugeMatchmakingUsersInSessions.get());
        ADD_VALUE("MMGaugeActiveMatchmakingSession", getTotalMatchmakingSessions());
        ADD_VALUE("MMGaugeActiveCreateGameMatchmakingSession", mScenarioMetrics.mCreateGameMatchmakingSessions.get());
        ADD_VALUE("MMGaugeActiveFindGameMatchmakingSession", mScenarioMetrics.mFindGameMatchmakingSessions.get());
        ADD_VALUE("MMGaugeActivePackerMatchmakingSession", mScenarioMetrics.mPackerMatchmakingSessions.get());
        ADD_VALUE("MMGaugeLastIdleLength", mScenarioMetrics.mLastIdleLength.get());
        ADD_VALUE("MMGaugeLastIdleFGLength", mScenarioMetrics.mLastIdleFGLength.get());
        ADD_VALUE("MMGaugeLastIdleDirtySessions", mScenarioMetrics.mLastIdleDirtySessions.get());

        ADD_VALUE("MMTotalMatchmakingFindGameFinalizeNotifsAccepted", mScenarioMetrics.mTotalMatchmakingFindGameFinalizeNotifsAccepted.getTotal());
        ADD_VALUE("MMTotalMatchmakingFindGameFinalizeNotifsOverwritten", mScenarioMetrics.mTotalMatchmakingFindGameFinalizeNotifsOverwritten.getTotal());
        ADD_VALUE("MMTotalMatchmakingFindGameFinalizeBatches", mScenarioMetrics.mTotalMatchmakingFindGameFinalizeBatches.getTotal());
        ADD_VALUE("MMTotalMatchmakingFindGameFinalizeMatches", mScenarioMetrics.mTotalMatchmakingFindGameFinalizeMatches.getTotal());
        ADD_VALUE("MMTotalMatchmakingFindGameFinalizeAttempts", mScenarioMetrics.mTotalMatchmakingFindGameFinalizeAttempts.getTotal());
        ADD_VALUE("MMTotalMatchmakingFindGameFinalizeSkippedAttempts", mScenarioMetrics.mTotalMatchmakingFindGameFinalizeSkippedAttempts.getTotal());
        ADD_VALUE("MMTotalMatchmakingFindGameFinalizeFailedToJoinFoundGame", mScenarioMetrics.mTotalMatchmakingFindGameFinalizeFailedToJoinFoundGame.getTotal());
        ADD_VALUE("MMTotalMatchmakingFindGameFinalizeFailedToJoinAnyGame", mScenarioMetrics.mTotalMatchmakingFindGameFinalizeFailedToJoinAnyGame.getTotal());
        ADD_VALUE("MMTotalCreateGameFinalizationsFailedToResetDedicatedServer", mScenarioMetrics.mTotalMatchmakingCreateGameFinalizationsFailedToResetDedicatedServer.getTotal());
        ADD_VALUE("MMTotalMatchmakingSessionsDisabledCreateGameModeAfterFailedFinalization", mScenarioMetrics.mTotalMatchmakingSessionsDisabledCreateGameMode.getTotal());

        // New Sub-Session specific metrics: 
        ADD_VALUE("ScenarioMMTotalMatchmakingScenarioSubSessionStarted", mScenarioMetrics.mTotalMatchmakingScenarioSubsessionStarted.getTotal());
        ADD_VALUE("ScenarioMMTotalMatchmakingScenarioSubSessionFinished", mScenarioMetrics.mTotalMatchmakingScenarioSubsessionFinished.getTotal());

        // Scenario Metrics:
        const char8_t* scenarios = "Scenario";
        const MatchmakerMasterMetrics& metrics = mScenarioMetrics;

        uint64_t uniqueMatchmakingSessions = metrics.mTotalMatchmakingSessionStarted.getTotal() - getTotalFinishedMatchmakingSessions(metrics);
        ADD_VALUE(scenarios << "MMGaugeUniqueMatchmakingSessions", uniqueMatchmakingSessions);

        // Counters
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionStarted", metrics.mTotalMatchmakingSessionStarted.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionSuccess", metrics.mTotalMatchmakingSessionSuccess.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionCanceled", metrics.mTotalMatchmakingSessionCanceled.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionTerminated", metrics.mTotalMatchmakingSessionTerminated.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionTerminatedReconfig", metrics.mTotalMatchmakingSessionTerminatedReconfig.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionTerminatedExternalSession", metrics.mTotalMatchmakingSessionTerminatedExternalSession.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionStartFailed", metrics.mTotalMatchmakingSessionStartFailed.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionsEndedFailedQosValidation", metrics.mTotalMatchmakingSessionsEndedFailedQosValidation.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionsRejectPoorReputation", metrics.mTotalMatchmakingSessionsRejectPoorReputation.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionsAcceptAnyReputation", metrics.mTotalMatchmakingSessionsAcceptAnyReputation.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionsMustAllowAnyReputation", metrics.mTotalMatchmakingSessionsMustAllowAnyReputation.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionTimeout", metrics.mTotalMatchmakingSessionTimeout.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionsCreatedNewGames", metrics.mTotalMatchmakingSessionsCreatedNewGames.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionsJoinedNewGames", metrics.mTotalMatchmakingSessionsJoinedNewGames.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSessionsJoinedExistingGames", metrics.mTotalMatchmakingSessionsJoinedExistingGames.getTotal());

        ADD_VALUE(scenarios << "MMTotalCreateGameFinalizeAttempts", metrics.mTotalMatchmakingCreateGameFinalizeAttempts.getTotal());
        ADD_VALUE(scenarios << "MMTotalCreateGameEvalsByNewSessions", metrics.mTotalMatchmakingCreateGameEvaluationsByNewSessions.getTotal());

        // Timings
        ADD_VALUE(scenarios << "MMTotalMatchmakingCancelDurationMs", metrics.mTotalMatchmakingCancelDurationMs.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingSuccessDurationMs", metrics.mTotalMatchmakingSuccessDurationMs.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingTerminatedDurationMs", metrics.mTotalMatchmakingTerminatedDurationMs.getTotal());
        ADD_VALUE(scenarios << "MMTotalMatchmakingEndedFailedQosValidationDurationMs", metrics.mTotalMatchmakingEndedFailedQosValidationDurationMs.getTotal());

        // Health checks for Qos
        typedef struct QosValidationMetrics
        {
            QosValidationMetrics()
                : mTotalUsersInQosRequests(0)
                , mTotalUsersInQosRequestsWithCC(0)
                , mTotalUsersInQosRequestsFailed(0)
                , mTotalUsersInQosRequestsFailedWithCC(0)
                , mTotalUsersNotFullyConnectedFailed(0)
                , mTotalUsersNotFullyConnectedFailedWithCC(0)
                , mTotalUsersNotFullyConnectedIncomplete(0)
                , mTotalUsersNotFullyConnectedIncompleteWithCC(0)
                , mTotalUsersPassedQosValidation(0)
                , mTotalUsersPassedQosValidationWithCC(0)
                , mTotalUsersFailedLatencyCheck(0)
                , mTotalUsersFailedLatencyCheckWithCC(0)
                , mTotalUsersFailedPacketLossCheck(0)
                , mTotalUsersFailedPacketLossCheckWithCC(0)
                , mTotalMatchmakingSessionsFullyConnected(0)
                , mTotalMatchmakingSessionsNotFullyConnected(0)
            {
            }

            // per-user stats

            //total users in requests subject to the connectivity/latency/packet-loss checks
            //Note: the outputted 'TotalUsersInQosRequestsSuccess' metric is calculated from TotalUsersInQosRequests - mTotalUsersInQosRequestsFailed
            uint64_t mTotalUsersInQosRequests;
            uint64_t mTotalUsersInQosRequestsWithCC;
            uint64_t mTotalUsersInQosRequestsFailed;
            uint64_t mTotalUsersInQosRequestsFailedWithCC;
            uint64_t mTotalUsersNotFullyConnectedFailed;
            uint64_t mTotalUsersNotFullyConnectedFailedWithCC;
            uint64_t mTotalUsersNotFullyConnectedIncomplete;
            uint64_t mTotalUsersNotFullyConnectedIncompleteWithCC;
            //total that passed the latency and packet-loss checks (does not include connectivity). Equals ('total users that did QoS latency/packet-loss' - mTotalUsersFailedQosValidation), i.e. ((mTotalUsersInQosRequests - mTotalUsersNotFullyConnected) - mTotalUsersFailedQosValidation)
            uint64_t mTotalUsersPassedQosValidation;
            uint64_t mTotalUsersPassedQosValidationWithCC;
            uint64_t mTotalUsersFailedLatencyCheck;
            uint64_t mTotalUsersFailedLatencyCheckWithCC;
            uint64_t mTotalUsersFailedPacketLossCheck;
            uint64_t mTotalUsersFailedPacketLossCheckWithCC;

            // per matchmaking session stats

            //total times matchmaking sessions passed the connectivity check. side: for avg users per MM session, equals (mTotalUsersInQosRequests / (mTotalMatchmakingSessionsFullyConnected + mTotalMatchmakingSessionsNotFullyConnected))
            uint64_t mTotalMatchmakingSessionsFullyConnected;
            uint64_t mTotalMatchmakingSessionsNotFullyConnected;

            typedef eastl::map<uint32_t, uint64_t> FailuresAtTierList;
            //total times matchmaking sessions failed latency checks. side: the number of times a MM session did the latency check equals mTotalMatchmakingSessionsFullyConnected. So number that passed latency is that - (sum of all mTotalMatchmakingSessionsFailedLatencyCheckPerTier tier values)
            FailuresAtTierList mTotalMatchmakingSessionsFailedLatencyCheckPerTier;
            //total times matchmaking sessions failed packet-loss checks. side: the number of times a MM session did the packet-loss check equals mTotalMatchmakingSessionsFullyConnected - (sum of all mTotalMatchmakingSessionsFailedLatencyCheckPerTier tier values)
            FailuresAtTierList mTotalMatchmakingSessionsFailedPacketLossCheckPerTier;

        } QosValidationMetrics;

        typedef eastl::map<GameNetworkTopology, QosValidationMetrics> QosValidationMetricsByTopologyMap;
        QosValidationMetricsByTopologyMap qosValidationMetricsPerTopology;

        metrics.mQosValidationMetrics.mTotalUsersInQosRequests.iterate([&qosValidationMetricsPerTopology](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameNetworkTopology topology;
            QosValidationMetricResult result;
            ConnConciergeModeMetricEnum ccMode;
            if (ParseGameNetworkTopology(tagList[0].second.c_str(), topology) && ParseQosValidationMetricResult(tagList[1].second.c_str(), result) && ParseConnConciergeModeMetricEnum(tagList[2].second.c_str(), ccMode))
            {
                QosValidationMetrics& qosMetrics = qosValidationMetricsPerTopology[topology];
                qosMetrics.mTotalUsersInQosRequests += value.getTotal();
                if (result != PASSED_QOS_VALIDATION)
                {
                    qosMetrics.mTotalUsersInQosRequestsFailed += value.getTotal();
                }
                if (ccMode != CC_UNUSED)
                {
                    qosMetrics.mTotalUsersInQosRequestsWithCC += value.getTotal();
                    if (result != PASSED_QOS_VALIDATION)
                    {
                        qosMetrics.mTotalUsersInQosRequestsFailedWithCC += value.getTotal();
                    }
                }
            }
        });
        metrics.mQosValidationMetrics.mTotalUsers.iterate([&qosValidationMetricsPerTopology](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameNetworkTopology topology;
            QosValidationMetricResult result;
            ConnConciergeModeMetricEnum ccMode;
            if (ParseGameNetworkTopology(tagList[0].second.c_str(), topology) && ParseQosValidationMetricResult(tagList[1].second.c_str(), result) && ParseConnConciergeModeMetricEnum(tagList[2].second.c_str(), ccMode))
            {
                QosValidationMetrics& qosMetrics = qosValidationMetricsPerTopology[topology];
                switch (result)
                {
                case PASSED_QOS_VALIDATION:
                    qosMetrics.mTotalUsersPassedQosValidation += value.getTotal();
                    break;
                case FAILED_PACKET_LOSS_CHECK:
                    qosMetrics.mTotalUsersFailedPacketLossCheck += value.getTotal();
                    break;
                case FAILED_LATENCY_CHECK:
                    qosMetrics.mTotalUsersFailedLatencyCheck += value.getTotal();
                    break;
                case NOT_FULLY_CONNECTED_FAILED:
                    qosMetrics.mTotalUsersNotFullyConnectedFailed += value.getTotal();
                    break;
                case NOT_FULLY_CONNECTED_INCOMPLETE:
                default:
                    qosMetrics.mTotalUsersNotFullyConnectedIncomplete += value.getTotal();
                    break;
                }
                if (ccMode != CC_UNUSED)
                {
                    switch (result)
                    {
                    case PASSED_QOS_VALIDATION:
                        qosMetrics.mTotalUsersPassedQosValidationWithCC += value.getTotal();
                        break;
                    case FAILED_PACKET_LOSS_CHECK:
                        qosMetrics.mTotalUsersFailedPacketLossCheckWithCC += value.getTotal();
                        break;
                    case FAILED_LATENCY_CHECK:
                        qosMetrics.mTotalUsersFailedLatencyCheckWithCC += value.getTotal();
                        break;
                    case NOT_FULLY_CONNECTED_FAILED:
                        qosMetrics.mTotalUsersNotFullyConnectedFailedWithCC += value.getTotal();
                        break;
                    case NOT_FULLY_CONNECTED_INCOMPLETE:
                    default:
                        qosMetrics.mTotalUsersNotFullyConnectedIncompleteWithCC += value.getTotal();
                        break;
                    }
                }
            }
        });
        metrics.mQosValidationMetrics.mTotalMatchmakingSessions.iterate([&qosValidationMetricsPerTopology](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            GameNetworkTopology topology;
            QosValidationMetricResult result;
            uint32_t tier = 0;
            if (ParseGameNetworkTopology(tagList[0].second.c_str(), topology) && ParseQosValidationMetricResult(tagList[1].second.c_str(), result) && blaze_str2int(tagList[2].second.c_str(), &tier) != tagList[2].second.c_str())
            {
                QosValidationMetrics& qosMetrics = qosValidationMetricsPerTopology[topology];
                if (result == NOT_FULLY_CONNECTED_FAILED)
                {
                    qosMetrics.mTotalMatchmakingSessionsNotFullyConnected += value.getTotal();
                }
                else if (result == NOT_FULLY_CONNECTED_INCOMPLETE)
                {
                    ERR_LOG("matchmaker.getStatusInfo() NOT_FULLY_CONNECTED_INCOMPLETE is invalid QoS result for metrics.mQosValidationMetrics.mTotalMatchmakingSessions");
                }
                else
                {
                    qosMetrics.mTotalMatchmakingSessionsFullyConnected += value.getTotal();

                    if (result == FAILED_LATENCY_CHECK)
                    {
                        qosMetrics.mTotalMatchmakingSessionsFailedLatencyCheckPerTier[tier] += value.getTotal();
                    }
                    else if (result == FAILED_PACKET_LOSS_CHECK)
                    {
                        qosMetrics.mTotalMatchmakingSessionsFailedPacketLossCheckPerTier[tier] += value.getTotal();
                    }
                }
            }
        });

        QosValidationMetricsByTopologyMap::const_iterator qosMetricsIter = qosValidationMetricsPerTopology.begin();
        QosValidationMetricsByTopologyMap::const_iterator qosMetricsEnd = qosValidationMetricsPerTopology.end();
        for (; qosMetricsIter != qosMetricsEnd; ++qosMetricsIter)
        {
            // avoid adding 0 count metrics
            if (qosMetricsIter->second.mTotalUsersInQosRequests == 0)
            {
                continue;
            }
            const char8_t* topology = GameNetworkTopologyToString(qosMetricsIter->first);

            // all the per-tier metrics are vectors of the same size, per topology

            ADD_VALUE(scenarios << "TotalUsersInQosRequests_" << topology, qosMetricsIter->second.mTotalUsersInQosRequests);
            ADD_VALUE(scenarios << "TotalUsersInQosRequestsSuccess_" << topology, (qosMetricsIter->second.mTotalUsersInQosRequests - qosMetricsIter->second.mTotalUsersInQosRequestsFailed));
            ADD_VALUE(scenarios << "TotalUsersInQosRequestsFailed_" << topology, qosMetricsIter->second.mTotalUsersInQosRequestsFailed);
            ADD_VALUE(scenarios << "TotalUsersNotFullyConnectedFailed_" << topology, qosMetricsIter->second.mTotalUsersNotFullyConnectedFailed);
            ADD_VALUE(scenarios << "TotalUsersNotFullyConnectedIncomplete_" << topology, qosMetricsIter->second.mTotalUsersNotFullyConnectedIncomplete);
            ADD_VALUE(scenarios << "TotalUsersPassedQosValidation_" << topology, qosMetricsIter->second.mTotalUsersPassedQosValidation);
            ADD_VALUE(scenarios << "TotalUsersFailedQosValidation_" << topology, qosMetricsIter->second.mTotalUsersFailedLatencyCheck + qosMetricsIter->second.mTotalUsersFailedPacketLossCheck);
            ADD_VALUE(scenarios << "TotalUsersFailedLatencyCheck_" << topology, qosMetricsIter->second.mTotalUsersFailedLatencyCheck);
            ADD_VALUE(scenarios << "TotalUsersFailedPacketLossCheck_" << topology, qosMetricsIter->second.mTotalUsersFailedPacketLossCheck);
            if (qosMetricsIter->second.mTotalUsersInQosRequestsWithCC != 0)
            {
                ADD_VALUE(scenarios << "TotalUsersInQosRequestsWithCC_" << topology, qosMetricsIter->second.mTotalUsersInQosRequestsWithCC);
                ADD_VALUE(scenarios << "TotalUsersInQosRequestsSuccessWithCC_" << topology, (qosMetricsIter->second.mTotalUsersInQosRequestsWithCC - qosMetricsIter->second.mTotalUsersInQosRequestsFailedWithCC));
                ADD_VALUE(scenarios << "TotalUsersInQosRequestsFailedWithCC_" << topology, qosMetricsIter->second.mTotalUsersInQosRequestsFailedWithCC);
                ADD_VALUE(scenarios << "TotalUsersNotFullyConnectedFailedWithCC_" << topology, qosMetricsIter->second.mTotalUsersNotFullyConnectedFailedWithCC);
                ADD_VALUE(scenarios << "TotalUsersNotFullyConnectedIncompleteWithCC_" << topology, qosMetricsIter->second.mTotalUsersNotFullyConnectedIncompleteWithCC);
                ADD_VALUE(scenarios << "TotalUsersPassedQosValidationWithCC_" << topology, qosMetricsIter->second.mTotalUsersPassedQosValidationWithCC);
                ADD_VALUE(scenarios << "TotalUsersFailedQosValidationWithCC_" << topology, qosMetricsIter->second.mTotalUsersFailedLatencyCheckWithCC + qosMetricsIter->second.mTotalUsersFailedPacketLossCheckWithCC);
                ADD_VALUE(scenarios << "TotalUsersFailedLatencyCheckWithCC_" << topology, qosMetricsIter->second.mTotalUsersFailedLatencyCheckWithCC);
                ADD_VALUE(scenarios << "TotalUsersFailedPacketLossCheckWithCC_" << topology, qosMetricsIter->second.mTotalUsersFailedPacketLossCheckWithCC);
            }

            ADD_VALUE(scenarios << "TotalMatchmakingSessionsFullyConnected_" << topology, qosMetricsIter->second.mTotalMatchmakingSessionsFullyConnected);
            ADD_VALUE(scenarios << "TotalMatchmakingSessionsNotFullyConnected_" << topology, qosMetricsIter->second.mTotalMatchmakingSessionsNotFullyConnected);

            // per-tier stats
            for (auto& tier : qosMetricsIter->second.mTotalMatchmakingSessionsFailedLatencyCheckPerTier)
            {
                ADD_VALUE(scenarios << "TotalMatchmakingSessionsFailedLatencyCheckPerTier_" << topology << "_" << tier.first, tier.second);
            }
            for (auto& tier : qosMetricsIter->second.mTotalMatchmakingSessionsFailedPacketLossCheckPerTier)
            {
                ADD_VALUE(scenarios << "TotalMatchmakingSessionsFailedPacketLossCheckPerTier_" << topology << "_" << tier.first, tier.second);
            }
        }

        // teams based matchmaking metrics. Note: for efficiency metrics omitted if all teams finalization rules disabled
        if (metrics.mTeamsBasedMetrics.mTotalTeamFinalizeAttempt.getTotal() != 0)
        {
            ADD_VALUE(scenarios << "MMTotalMatchmakingSessionTimeoutSessionSize", metrics.mTeamsBasedMetrics.mTotalMatchmakingSessionTimeoutSessionSize.getTotal());
            ADD_VALUE(scenarios << "MMTotalMatchmakingSessionTimeoutTeamUed", metrics.mTeamsBasedMetrics.mTotalMatchmakingSessionTimeoutSessionTeamUed.getTotal());

            const uint64_t finalized = metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccess.getTotal();

            ADD_VALUE(scenarios << "MMTotalTeamFinalizeAttempt", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeAttempt.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccess", finalized);
            ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessTeamCount", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessTeamCount.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessPlayerCount", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessPlayerCount.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessTeamUedDiff", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessTeamUedDiff.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessGameMemberUedDelta", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessGameMemberUedDelta.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessTeamMemberUedDelta", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessTeamMemberUedDelta.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessRePicksLeft", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessRePicksLeft.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessCompnsLeft", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessCompnsLeft.getTotal());

            ADD_VALUE(scenarios << "MMTotalTeamFinalizeCompnsTry", metrics.mTeamsBasedMetrics.mTotalTeamFinalizeCompnsTry.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizePickSequenceTry", metrics.mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceTry.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizePickSequenceFail", metrics.mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceFail.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizePickSequenceFailTeamSizeDiff", metrics.mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceFailTeamSizeDiff.getTotal());
            ADD_VALUE(scenarios << "MMTotalTeamFinalizePickSequenceFailTeamUedDiff", metrics.mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceFailTeamUedDiff.getTotal());

            // averages
            ADD_VALUE(scenarios << "MMAvgTeamFinalizeSuccessPlayerCount", ((finalized == 0)? 0 : (int64_t)((metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessPlayerCount.getTotal() / finalized) + 0.5)));
            ADD_VALUE(scenarios << "MMAvgTeamFinalizeSuccessTeamUedDiff", ((finalized == 0)? 0 : (int64_t)((metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessTeamUedDiff.getTotal() / finalized) + 0.5)));
            ADD_VALUE(scenarios << "MMAvgTeamFinalizeSuccessRePicksLeft", ((finalized == 0)? 0 : (int64_t)((metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessRePicksLeft.getTotal() / finalized) + 0.5)));
            const uint64_t pickSequenceFails = metrics.mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceFail.getTotal();
            ADD_VALUE(scenarios << "MMAvgTeamFinalizePickSequenceFailTeamUedDiff", ((pickSequenceFails == 0)? 0 : (int64_t)((metrics.mTeamsBasedMetrics.mTotalTeamFinalizePickSequenceFailTeamUedDiff.getTotal() / pickSequenceFails) + 0.5)));

            // by group sizes
            metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessByGroupOfX.iterate([&parentStatusMap, &tempNameString, &tempValueString, &scenarios](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
                ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessByGroupOf" << tagList[0].second.c_str(), value.getTotal());
            });
            metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessByGroupOfXCompnSame.iterate([&parentStatusMap, &tempNameString, &tempValueString, &scenarios](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
                ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessByGroupOf" << tagList[0].second.c_str() << "CompnSame", value.getTotal());
            });
            metrics.mTeamsBasedMetrics.mTotalTeamFinalizeSuccessByGroupOfXCompnOver50FitPct.iterate([&parentStatusMap, &tempNameString, &tempValueString, &scenarios](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
                ADD_VALUE(scenarios << "MMTotalTeamFinalizeSuccessByGroupOf" << tagList[0].second.c_str() << "CompnOver50FitPct", value.getTotal());
            });
        }
#undef ADD_VALUE

        const char8_t* metricName = "MMTotalRuleUses";
        mRuleDefinitionCollection->getRuleDefinitionTotalUseCount().iterate([&parentStatusMap, &metricName](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(parentStatusMap, metricName, tagList, value.get()); });
    }

    void Matchmaker::dedicatedServerOverride(BlazeId blazeId, GameId gameId)
    {
        DedicatedServerOverrideMap::iterator overrideIter = mDedicatedServerOverrideMap.find(blazeId);

        // we don't need to tell the RETE slaves about an override, because we can just do the join attempt directly from the MM slave

        // if the game id is INVALID_GAME_ID and they are already in the map, clear the value.
        if (gameId == GameManager::INVALID_GAME_ID)
        {
            if (overrideIter != mDedicatedServerOverrideMap.end())
            {
                mDedicatedServerOverrideMap.erase(overrideIter);
                INFO_LOG("matchmaker.dedicatedServerOverride() Player(" << blazeId << ") cleared from dedicated server override matchmaking.");
            }
            else
            {
                INFO_LOG("matchmaker.dedicatedServerOverride() Player(" << blazeId << ") was not in the list, but was clearing the game id, so we ignored it. (dedicated server override matchmaking)");
            }
            return;
        }

        mDedicatedServerOverrideMap[blazeId] = gameId;
        INFO_LOG("Matchmaker.dedicatedServerOverride() Player(" << blazeId << ") will now matchmake into game(" << gameId << ").");
    }

    const DedicatedServerOverrideMap& Matchmaker::getDedicatedServerOverrides() const
    {
        return mDedicatedServerOverrideMap;
    }

    void Matchmaker::fillServersOverride(const GameManager::GameIdList& fillServersOverrideList)
    {
        fillServersOverrideList.copyInto(mFillServersOverrideList);
        INFO_LOG("Matchmaker.fillServersOverride() List of (" << mFillServersOverrideList.size() << ") game ids will now be priority filled by matchmaking.");
    }

    const GameManager::GameIdList& Matchmaker::getFillServersOverride() const
    {
        return mFillServersOverrideList;
    }

    bool Matchmaker::hostingComponentResolved()
    {
        return Search::FindGameShardingBroker::registerForSearchSlaveNotifications();
    }

    void Matchmaker::onNotifyFindGameFinalizationUpdate(const Blaze::Search::NotifyFindGameFinalizationUpdate& data,UserSession *associatedUserSession)
    {
        TRACE_LOG("[Matchmaker].onNotifyFindGameFinalizationUpdate received NotifyFindGameFinalizationUpdate containing matches for '" << data.getFoundGames().getMatchedGameList().size() << "' matchmaking sessions.");
        MatchmakingSessionId mmSessionId = data.getFoundGames().getMatchmakingSessionId();
        MatchmakingSessionMap::iterator sessionIter = mFindGameSessionMap.find(mmSessionId);
        if (sessionIter != mFindGameSessionMap.end())
        {
            Blaze::Search::NotifyFindGameFinalizationUpdate* update = data.clone();
            if (update != nullptr)
            {
                mScenarioMetrics.mTotalMatchmakingFindGameFinalizeNotifsAccepted.increment();

                FindGameUpdateByInstanceIdMap& updatesByInstanceIdMap = mFindGameUpdatesByMmSessionIdMap[mmSessionId];

                FindGameUpdateByInstanceIdMap::insert_return_type ret = updatesByInstanceIdMap.insert(data.getUpdateSourceInstanceId());
                if (!ret.second)
                    mScenarioMetrics.mTotalMatchmakingFindGameFinalizeNotifsOverwritten.increment();

                ret.first->second = update; // clobbers any updates that had been queued but not processed by the matchmaker

                scheduleNextIdle(TimeValue::getTimeOfDay(), mMatchmakingConfig.getServerConfig().getIdlePeriod());
            }
        }
    }

    // validate can increment the metric before doing so.
    void Matchmaker::incrementTotalFinishedMatchmakingSessionsMetric(MatchmakingResult result, MatchmakingSession& dyingSession, bool onReconfig)
    {
        const uint64_t sessionDuration = dyingSession.getMatchmakingRuntime().getMillis();
        const uint16_t sessionSize = dyingSession.getPlayerCount();
        uint32_t gamePercentFull = 0;
        if (dyingSession.getMatchmakedGameCapacity() != 0)
        {
            gamePercentFull = (dyingSession.getMatchmakedGamePlayerCount() * 100)/dyingSession.getMatchmakedGameCapacity();
        }

        if (mScenarioMetrics.mGaugeMatchmakingUsersInSessions.get() >= sessionSize)
        {
            mScenarioMetrics.mGaugeMatchmakingUsersInSessions.decrement(sessionSize);
        }
        else
        {
            WARN_LOG(LOG_PREFIX << "mGaugeMatchmakingUsersInSessions(" << mScenarioMetrics.mGaugeMatchmakingUsersInSessions.get() << ") cannot be decremented(" << sessionSize << ") below zero");
            mScenarioMetrics.mGaugeMatchmakingUsersInSessions.set(0);
        }

        // Scenario Subsessions (other than first success/last failure) do not contribute to the mTotalMatchmakingSessionStarted, so we can skip the early out. 
        bool updateResultMetrics = true;
        if (dyingSession.getMMScenarioId() == INVALID_SCENARIO_ID)
        {
            ERR_LOG("[incrementTotalFinishedMatchmakingSessionsMetric] No Scenario Id provided.  Check for possible side-effects from old MMing removal.")
        }

        mScenarioMetrics.mTotalMatchmakingScenarioSubsessionFinished.increment();

        SessionLockData* lockData = getSessionLockData(&dyingSession);
        if (lockData == nullptr)
            return;

        // The one that sets the metrics is either the last one alive, or the owner of the lock (success case), don't set metrics again if we've already done it
        updateResultMetrics = !lockData->mHasSetEndMetrics 
            && (lockData->mLockOwner == dyingSession.getMMSessionId() || lockData->mScenarioSubsessionCount == 1);
        --lockData->mScenarioSubsessionCount;

        if (updateResultMetrics)
            lockData->mHasSetEndMetrics = true;

        if (updateResultMetrics)
        {
            const char8_t* pingSite = UNKNOWN_PINGSITE;
            if (dyingSession.getPrimaryUserSessionInfo() != nullptr)
                pingSite = dyingSession.getPrimaryUserSessionInfo()->getBestPingSiteAlias();

            auto delineationGroup = dyingSession.getCachedStartMatchmakingInternalRequestPtr()->getRequest().getCommonGameData().getDelineationGroup();
            mMatchmakerSlave.updateTimeToMatch(dyingSession.getMatchmakingRuntime(), pingSite, delineationGroup, dyingSession.getScenarioInfo().getScenarioName(), result);
        }

        MatchmakerMasterMetrics& metrics = mScenarioMetrics;

        // Verify that the matchmaking metrics are still sane:
        if (updateResultMetrics && metrics.mTotalMatchmakingSessionStarted.getTotal() <= getTotalFinishedMatchmakingSessions(metrics))
        {
            // Error if mTotalMatchmakingSessionStarted somehow not incremented before its finish metric
            ERR_LOG(LOG_PREFIX << "Metric " << MatchmakingResultToString(result) << " not incremented due to potential metrics underflow errors. mTotalMatchmakingSessionStarted("
                               << metrics.mTotalMatchmakingSessionStarted.getTotal() << "), mTotalMatchmakingSessionSuccess(" << metrics.mTotalMatchmakingSessionSuccess.getTotal()
                               << "), mTotalMatchmakingSessionCanceled(" << metrics.mTotalMatchmakingSessionCanceled.getTotal() << "), mTotalMatchmakingSessionTerminated("
                               << metrics.mTotalMatchmakingSessionTerminated.getTotal() << "), mTotalMatchmakingSessionTimeout(" << metrics.mTotalMatchmakingSessionTimeout.getTotal() << ").");
            return;
        }

        const char8_t* scenarioName = dyingSession.getScenarioInfo().getScenarioName();
        const char8_t* variantName = dyingSession.getScenarioInfo().getScenarioVariant();
        ScenarioVersion scenarioVersion = dyingSession.getScenarioInfo().getScenarioVersion();
        const char8_t* subsessionName = dyingSession.getScenarioInfo().getSubSessionName();

        switch(result)
        {
        case SUCCESS_CREATED_GAME:
            // inc CG metrics
            if (updateResultMetrics)
            {
                metrics.mTotalMatchmakingSessionSuccess.increment();
                metrics.mTotalMatchmakingSessionsCreatedNewGames.increment();
                metrics.mTotalMatchmakingSuccessDurationMs.increment(sessionDuration);
            }

            // should always be true for a success case
            if (updateResultMetrics)
            {
                mMatchmakingMetrics.mMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, SUCCESS_CREATED_GAME);
                mMatchmakingMetrics.mMetrics.mTotalFitScore.increment(dyingSession.getCreateGameFinalizationFitScore(), scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalSuccessDurationMs.increment(sessionDuration, scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalGamePercentFullCapacity.increment(gamePercentFull, scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalMaxPlayerCapacity.increment(dyingSession.getMatchmakedGameCapacity(), scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalPlayerCountInGame.increment(dyingSession.getMatchmakedGamePlayerCount(), scenarioName, variantName, scenarioVersion);
            }

            mMatchmakingMetrics.mSubsessionMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, subsessionName, SUCCESS_CREATED_GAME);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalFitScore.increment(dyingSession.getCreateGameFinalizationFitScore(), scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalSuccessDurationMs.increment(sessionDuration, scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalGamePercentFullCapacity.increment(gamePercentFull, scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalMaxPlayerCapacity.increment(dyingSession.getMatchmakedGameCapacity(), scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalPlayerCountInGame.increment(dyingSession.getMatchmakedGamePlayerCount(), scenarioName, variantName, scenarioVersion, subsessionName);
            break;
        case SUCCESS_JOINED_NEW_GAME:
            // inc CG metrics
            if (updateResultMetrics)
            {
                metrics.mTotalMatchmakingSessionSuccess.increment();
                metrics.mTotalMatchmakingSessionsJoinedNewGames.increment();
                metrics.mTotalMatchmakingSuccessDurationMs.increment(sessionDuration);
            }

            // should always be true for a success case
            if (updateResultMetrics)
            {
                mMatchmakingMetrics.mMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, SUCCESS_JOINED_NEW_GAME);
                mMatchmakingMetrics.mMetrics.mTotalFitScore.increment(dyingSession.getCreateGameFinalizationFitScore(), scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalSuccessDurationMs.increment(sessionDuration, scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalGamePercentFullCapacity.increment(gamePercentFull, scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalMaxPlayerCapacity.increment(dyingSession.getMatchmakedGameCapacity(), scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalPlayerCountInGame.increment(dyingSession.getMatchmakedGamePlayerCount(), scenarioName, variantName, scenarioVersion);
            }

            mMatchmakingMetrics.mSubsessionMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, subsessionName, SUCCESS_JOINED_NEW_GAME);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalFitScore.increment(dyingSession.getCreateGameFinalizationFitScore(), scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalSuccessDurationMs.increment(sessionDuration, scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalGamePercentFullCapacity.increment(gamePercentFull, scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalMaxPlayerCapacity.increment(dyingSession.getMatchmakedGameCapacity(), scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalPlayerCountInGame.increment(dyingSession.getMatchmakedGamePlayerCount(), scenarioName, variantName, scenarioVersion, subsessionName);
            break;
        case SUCCESS_JOINED_EXISTING_GAME:
            // inc FG metrics
            if (updateResultMetrics)
            {
                metrics.mTotalMatchmakingSessionSuccess.increment();
                metrics.mTotalMatchmakingSessionsJoinedExistingGames.increment();
                metrics.mTotalMatchmakingSuccessDurationMs.increment(sessionDuration);
            }

            // should always be true for a success case
            if (updateResultMetrics)
            {
                mMatchmakingMetrics.mMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, SUCCESS_JOINED_EXISTING_GAME);
                mMatchmakingMetrics.mMetrics.mTotalFitScore.increment(dyingSession.getFitScoreForMatchmakedGame(), scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalSuccessDurationMs.increment(sessionDuration, scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalGamePercentFullCapacity.increment(gamePercentFull, scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalMaxPlayerCapacity.increment(dyingSession.getMatchmakedGameCapacity(), scenarioName, variantName, scenarioVersion);
                mMatchmakingMetrics.mMetrics.mTotalPlayerCountInGame.increment(dyingSession.getMatchmakedGamePlayerCount(), scenarioName, variantName, scenarioVersion);
            }

            mMatchmakingMetrics.mSubsessionMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, subsessionName, SUCCESS_JOINED_EXISTING_GAME);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalFitScore.increment(dyingSession.getFitScoreForMatchmakedGame(), scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalSuccessDurationMs.increment(sessionDuration, scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalGamePercentFullCapacity.increment(gamePercentFull, scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalMaxPlayerCapacity.increment(dyingSession.getMatchmakedGameCapacity(), scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mSubsessionMetrics.mTotalPlayerCountInGame.increment(dyingSession.getMatchmakedGamePlayerCount(), scenarioName, variantName, scenarioVersion, subsessionName);
            break;
        case SESSION_TIMED_OUT:
            if (updateResultMetrics)
            {
                metrics.mTotalMatchmakingSessionTimeout.increment();
                metrics.mTeamsBasedMetrics.mTotalMatchmakingSessionTimeoutSessionSize.increment(sessionSize);
                metrics.mTeamsBasedMetrics.mTotalMatchmakingSessionTimeoutSessionTeamUed.increment((uint64_t) dyingSession.getCriteria().getTeamUEDContributionValue());
            }

            // ensure a subsession timeout doesn't tick the scenario timeout metric, unless the whole scenario is complete
            if (updateResultMetrics)
            {
                mMatchmakingMetrics.mMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, SESSION_TIMED_OUT);
            }

            mMatchmakingMetrics.mSubsessionMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, subsessionName, SESSION_TIMED_OUT);
            break;
        case SESSION_CANCELED:
            if (updateResultMetrics)
            {
                metrics.mTotalMatchmakingSessionCanceled.increment();
                metrics.mTotalMatchmakingCancelDurationMs.increment(sessionDuration);
            }

            // ensure a subsession being cancelled (which occurs if another subsession finds a match)
            // doesn't tick the scenario timeout metric, unless the whole scenario is complete
            if (updateResultMetrics)
            {
                mMatchmakingMetrics.mMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, SESSION_CANCELED);
            }

            mMatchmakingMetrics.mSubsessionMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, subsessionName, SESSION_CANCELED);
            break;
        case SESSION_TERMINATED:
            if (updateResultMetrics)
            {
                metrics.mTotalMatchmakingSessionTerminated.increment();
            
                if (onReconfig)
                    metrics.mTotalMatchmakingSessionTerminatedReconfig.increment();
                if (mExternalSessionTerminated)
                {
                    metrics.mTotalMatchmakingSessionTerminatedExternalSession.increment();
                    mExternalSessionTerminated = false;
                }

                metrics.mTotalMatchmakingTerminatedDurationMs.increment(sessionDuration);
            }

            // ensure a subsession termination doesn't tick the scenario timeout metric, unless the whole scenario is complete
            if (updateResultMetrics)
            {
                mMatchmakingMetrics.mMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, SESSION_TERMINATED);
            }

            mMatchmakingMetrics.mSubsessionMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, subsessionName, SESSION_TERMINATED);
            break;
        case SESSION_QOS_VALIDATION_FAILED:
            if (updateResultMetrics)
            {
                metrics.mTotalMatchmakingSessionsEndedFailedQosValidation.increment();
                metrics.mTotalMatchmakingEndedFailedQosValidationDurationMs.increment(sessionDuration);
            }

            // ensure a subsession QoS failure doesn't tick the scenario timeout metric, unless the whole scenario is complete
            if (updateResultMetrics)
            {
                mMatchmakingMetrics.mMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, SESSION_QOS_VALIDATION_FAILED);
            }

            mMatchmakingMetrics.mSubsessionMetrics.mTotalResults.increment(1, scenarioName, variantName, scenarioVersion, subsessionName, SESSION_QOS_VALIDATION_FAILED);
            break;
        default:
            break;
        }
    }

    // get external leave infos for the members of the matchmaking session
    void Matchmaker::userSessionInfosToExternalUserLeaveInfos(const GameManager::UserSessionInfoList& members, Blaze::ExternalUserLeaveInfoList& externalUserLeaveInfos) const
    {
        for (GameManager::UserSessionInfoList::const_iterator itr = members.begin(); itr != members.end(); ++itr)
        {
            const GameManager::UserSessionInfo &member = *(*itr);
            if (member.getHasExternalSessionJoinPermission())
            {
                setExternalUserLeaveInfoFromUserSessionInfo(*externalUserLeaveInfos.pull_back(), member);
            }
        }
    }

    void Matchmaker::restartFindGameSessionAfterReinsertion(Blaze::GameManager::MatchmakingSessionId mmSessionId)
    {
        MatchmakingSession* session = findSession(mmSessionId);
        if (session != nullptr)
        {
            const StartMatchmakingInternalRequestPtr reqPtr = session->getCachedStartMatchmakingInternalRequestPtr();
            MatchmakingCriteriaError err;
            startMatchmakingFindGameSession(mmSessionId, *reqPtr, err, mMatchmakerSlave.getConfig().getMatchmakerSettings().getSearchMultiRpcTimeout(), &session->getDiagnostics());
        }
    }

    // add filled out notification entry to map for the matchmaking session. If the mm session has no reserved players, will not add an entry.
    void Matchmaker::fillNotifyReservedExternalPlayers(MatchmakingSessionId mmSessionId, GameId joinedGameId,
        NotifyMatchmakingReservedExternalPlayersMap& notificationMap)
    {
        // find the session to update
        MatchmakingSession* mmSession = findCreateGameSession(mmSessionId);
        if (mmSession == nullptr)
        {
            // something went wrong
            ERR_LOG("[Matchmaker].filloutNotifyReservedExternalPlayers: session(" << mmSessionId << ") not found.");
            return;
        }

        mmSession->fillNotifyReservedExternalPlayers(joinedGameId, notificationMap);
    }

    bool Matchmaker::validateQosForMatchmakingSessions(const Blaze::Search::MatchmakingSessionIdList &matchmakingSessionIds, const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response)
    {
        bool passedQosValidation = true;
        Blaze::Search::MatchmakingSessionIdList::const_iterator mmSessionIdIter = matchmakingSessionIds.begin();
        Blaze::Search::MatchmakingSessionIdList::const_iterator mmSessionIdEnd = matchmakingSessionIds.end();
        for (; mmSessionIdIter != mmSessionIdEnd; ++mmSessionIdIter)
        {
            // NOTE: we don't early out because validateQos also updates the avoid list
            auto session = findSession(*mmSessionIdIter);
            if (session == nullptr || !session->validateQosAndUpdateAvoidList(request, response))
                passedQosValidation = false;
        }

        return passedQosValidation;
    }

    MatchmakingSessionMatchNode * Matchmaker::allocateMatchNode() 
    { 
        MatchmakingSessionMatchNode *node = new (getMatchNodeAllocator().allocate(sizeof(MatchmakingSessionMatchNode))) MatchmakingSessionMatchNode(); 
        node->sRuleResultMap = BLAZE_NEW DebugRuleResultMap();
        return node;
    }

    void Matchmaker::deallocateMatchNode(MatchmakingSessionMatchNode &node) 
    { 
        node.sRuleResultMap->Release();
        getMatchNodeAllocator().deallocate(&node); 
    }

    void Matchmaker::addSessionDiagnosticsToMetrics(const MatchmakingSession& session, const MatchmakingSubsessionDiagnostics& countsToAdd, bool addCreateMode, bool addFindMode, bool mergeSessionCount)
    {
        const char8_t* scenarioName = session.getScenarioInfo().getScenarioName();
        const char8_t* variantName = session.getScenarioInfo().getScenarioVariant();
        ScenarioVersion scenarioVersion = session.getScenarioInfo().getScenarioVersion();
        const char8_t* subsessionName = session.getScenarioInfo().getSubSessionName();

        Metrics::TagValue scenarioVersionStr;
        Metrics::uint32ToString(scenarioVersion, scenarioVersionStr);

        if (mergeSessionCount)
        {
            uint64_t currentTotal = mMatchmakingMetrics.mDiagnostics.mSessions.getTotal({ { Metrics::Tag::scenario_name, scenarioName },{ Metrics::Tag::scenario_variant_name, variantName },{ Metrics::Tag::scenario_version, scenarioVersionStr.c_str() },{ Metrics::Tag::subsession_name, subsessionName } });
            if (countsToAdd.getSessions() > currentTotal)
            {
                mMatchmakingMetrics.mDiagnostics.mSessions.increment(countsToAdd.getSessions() - currentTotal, scenarioName, variantName, scenarioVersion, subsessionName);
            }
        }
        else
        {
            mMatchmakingMetrics.mDiagnostics.mSessions.increment(countsToAdd.getSessions(), scenarioName, variantName, scenarioVersion, subsessionName);
        }

        if (addCreateMode)
        {
            mMatchmakingMetrics.mDiagnostics.mCreateEvaluations.increment(countsToAdd.getCreateEvaluations(), scenarioName, variantName, scenarioVersion, subsessionName);
            mMatchmakingMetrics.mDiagnostics.mCreateEvaluationsMatched.increment(countsToAdd.getCreateEvaluationsMatched(), scenarioName, variantName, scenarioVersion, subsessionName);
        }

        if (addFindMode)
        {
            mMatchmakingMetrics.mDiagnostics.mFindRequestsGamesAvailable.increment(countsToAdd.getFindRequestsGamesAvailable(), scenarioName, variantName, scenarioVersion, subsessionName);
        }

        // add rule breakdown diagnostic entries
        for (auto& ruleDiagosticsToAdd : countsToAdd.getRuleDiagnostics())
        {
            const auto* ruleName = ruleDiagosticsToAdd.first.c_str();
            const auto& ruleCategoriesToAdd = ruleDiagosticsToAdd.second;

            if (ruleCategoriesToAdd == nullptr)
            {
                // really shouldn't happen, but ...
                ASSERT_LOG("addSessionDiagnosticsToMetrics: no categories for rule(" << ruleName << ") to add. No op.");
                continue;
            }

            // scan the source categories and add each category to metrics
            for (auto& ruleCategoryToAdd : *ruleCategoriesToAdd)
            {
                // scan the source category's values map and add each map entry to metrics
                const auto* categoryTag = ruleCategoryToAdd.first.c_str();
                const auto& ruleValuesToAdd = ruleCategoryToAdd.second;

                for (auto& ruleValueToAdd : *ruleValuesToAdd)
                {
                    const auto* valueTag = ruleValueToAdd.first.c_str();
                    const auto& ruleValueCountToAdd = ruleValueToAdd.second;

                    if (ruleValueCountToAdd == nullptr)
                    {
                        // really shouldn't happen, but ...
                        ASSERT_LOG("addSessionDiagnosticsToMetrics: no counts for rule(" << ruleName << ") category(" << categoryTag << ") value(" << valueTag << ") to add. No op.");
                        continue;
                    }

                    if (mergeSessionCount)
                    {
                        uint64_t currentTotal = mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mSessions.getTotal({ { Metrics::Tag::scenario_name, scenarioName },{ Metrics::Tag::scenario_variant_name, variantName },{ Metrics::Tag::scenario_version, scenarioVersionStr.c_str() },{ Metrics::Tag::subsession_name, subsessionName },{ Metrics::Tag::rule_name, ruleName },{ Metrics::Tag::rule_category, categoryTag },{ Metrics::Tag::rule_category_value, valueTag } });
                        if (ruleValueCountToAdd->getSessions() > currentTotal)
                        {
                            mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mSessions.increment(ruleValueCountToAdd->getSessions() - currentTotal, scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                        }
                    }
                    else
                    {
                        mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mSessions.increment(ruleValueCountToAdd->getSessions(), scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                    }

                    if (addCreateMode)
                    {
                        mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mCreateEvaluations.increment(ruleValueCountToAdd->getCreateEvaluations(), scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                        mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mCreateEvaluationsMatched.increment(ruleValueCountToAdd->getCreateEvaluationsMatched(), scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                    }

                    if (addFindMode)
                    {
                        if (mergeSessionCount)
                        {
                            uint64_t currentTotal = mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mFindRequests.getTotal({ { Metrics::Tag::scenario_name, scenarioName },{ Metrics::Tag::scenario_variant_name, variantName },{ Metrics::Tag::scenario_version, scenarioVersionStr.c_str() },{ Metrics::Tag::subsession_name, subsessionName },{ Metrics::Tag::rule_name, ruleName },{ Metrics::Tag::rule_category, categoryTag },{ Metrics::Tag::rule_category_value, valueTag } });
                            if (ruleValueCountToAdd->getFindRequests() > currentTotal)
                            {
                                mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mFindRequests.increment(ruleValueCountToAdd->getFindRequests() - currentTotal, scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                            }
                        }
                        else
                        {
                            mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mFindRequests.increment(ruleValueCountToAdd->getFindRequests(), scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                        }

                        if (mergeSessionCount)
                        {
                            uint64_t currentTotal = mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mFindRequestsHadGames.getTotal({ { Metrics::Tag::scenario_name, scenarioName },{ Metrics::Tag::scenario_variant_name, variantName },{ Metrics::Tag::scenario_version, scenarioVersionStr.c_str() },{ Metrics::Tag::subsession_name, subsessionName },{ Metrics::Tag::rule_name, ruleName },{ Metrics::Tag::rule_category, categoryTag },{ Metrics::Tag::rule_category_value, valueTag } });
                            if (ruleValueCountToAdd->getFindRequestsHadGames() > currentTotal)
                            {
                                mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mFindRequestsHadGames.increment(ruleValueCountToAdd->getFindRequestsHadGames() - currentTotal, scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                            }
                        }
                        else
                        {
                            mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mFindRequestsHadGames.increment(ruleValueCountToAdd->getFindRequestsHadGames(), scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                        }

                        mMatchmakingMetrics.mDiagnostics.mRuleDiagnostics.mFindRequestsGamesVisible.increment(ruleValueCountToAdd->getFindRequestsGamesVisible(), scenarioName, variantName, scenarioVersion, subsessionName, ruleName, categoryTag, valueTag);
                    }
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief when MM session is done, add its diagnostic metric counts to the system totals
    ***************************************************************************************************/
    void Matchmaker::incrementFinishedMmSessionDiagnostics(MatchmakingResult result, MatchmakingSession& dyingSession)
    {
        // some (FG) diagnostics may start tracking before a subsession actually starts. Only increment if started
        if (!mMatchmakingConfig.getServerConfig().getTrackDiagnostics() || !dyingSession.getIsStartDelayOver())
        {
            return;
        }

        // to avoid skewing data, metrics skipped in cases:
        auto mmSessionId = dyingSession.getMMSessionId();
        if (result == SESSION_TERMINATED)
        {
            TRACE_LOG("[Matchmaker].incrementFinishedMmSessionDiagnostics: NOT adding diagnostics for session(" << 
                mmSessionId << ") due to result(" << MatchmakingResultToString(result) << ")");
            return;
        }

        TRACE_LOG("[Matchmaker].incrementFinishedMmSessionDiagnostics: Adding diagnostics for session(" << mmSessionId <<
            ")'s for (" << dyingSession.getDiagnostics().getRuleDiagnostics().size() << ") rules");

        // add session's counts to system wide totals
        addSessionDiagnosticsToMetrics(dyingSession, dyingSession.getDiagnostics(), true, true, false);
    }

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
