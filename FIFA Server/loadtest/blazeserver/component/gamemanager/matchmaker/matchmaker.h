/*! ************************************************************************************************/
/*!
    \file   matchmaker.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_MATCHMAKER_H
#define BLAZE_GAMEMANAGER_MATCHMAKER_H

#include "gamemanager/tdf/matchmaker.h"
#include "gamemanager/rpc/gamemanagermaster_stub.h" // for RPC master error enumerations
#include "gamemanager/rpc/matchmakerslave_stub.h"
#include "gamemanager/tdf/matchmaker_server.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/matchmaker/matchmakermetrics.h"
#include "gamemanager/matchmaker/matchmakingsession.h"
#include "gamemanager/tdf/search_server.h"
#include "gamemanager/rpc/searchslave_stub.h"
#include "gamemanager/searchcomponent/findgameshardingbroker.h"
#include "gamemanager/rete/retenetwork.h" // for Matchmaker::mReteNetwork
#include "gamemanager/idlerutil.h"
#include "framework/blazeallocator.h"
#include "framework/system/fiberjobqueue.h"

#include "EASTL/intrusive_ptr.h" // for intrusive_ptr
#include "EASTL/map.h"
#include "EASTL/vector.h"

namespace Blaze
{
    class UserSession;

namespace Matchmaker
{
    class MatchmakerSlaveImpl;
    class RuleDefinitionCollection;
    class MatchmakerConfig;
}

namespace GameManager
{

namespace Matchmaker
{
    class MatchmakingSession;
    class MatchmakingConfig;


    /*! ************************************************************************************************/
    /*!
        \brief Matchmaker handles Blaze Matchmaking tasks; it contains the collection of matchmaking rules
            and matchmaking sessions.  It's not a true component, however; access the matchmaker through
            the GameManager component.

            see the Matchmaking TDD for details: http://easites.ea.com/gos/FY09%20Requirements/Blaze/TDD%20Approved/Matchmaker%20TDD.doc
    *************************************************************************************************/
    class Matchmaker : public Search::FindGameShardingBroker, private GameManager::IdlerUtil
    {
        NON_COPYABLE(Matchmaker);
    public:

        // NOTE: MM makes assumptions that these maps are sorted by SessionId, we could potentiall change this to a hash_map
        typedef eastl::map<MatchmakingSessionId, MatchmakingSession *> MatchmakingSessionMap;

        /*! ************************************************************************************************/
        /*!
            \brief Construct the Matchmaker obj.
        
            \param[in] matchmakerSlave - pointer back to the MatchmakerSlave
        *************************************************************************************************/
        Matchmaker(Blaze::Matchmaker::MatchmakerSlaveImpl& matchmakerSlave);

        //! destructor
        ~Matchmaker() override;

        /*! ************************************************************************************************/
        /*! \brief called when the owning component is shutting down.
        ***************************************************************************************************/
        void onShutdown();

        /*! ************************************************************************************************/
        /*!
            \brief Process a startSession command; creates a new matchmaking session and fills out a
                StartMatchmakingResponse obj.
                
            Note: session isn't evaluated until the next time the matchmaker is idled.
        
            \param[in]  request - the startMatchmakingRequest (from gameManagerMaster)
            \param[out] externalSessionIdentification - if using external sessions, stores resulting external session's identification
            \param[out] sessionId - the unique id of the new session
            \param[out] err - an error response object (only initialized if the function also returns false)
            \return ERR_OK if the session was started successfully; otherwise an error
        *************************************************************************************************/
        BlazeRpcError startSession(const StartMatchmakingInternalRequest &request, Blaze::ExternalSessionIdentification& externalSessionIdentification, MatchmakingCriteriaError &err);

        /*! ************************************************************************************************/
        /*!
            \brief Process a cancelSession command; cancels an existing matchmaking session; generates
                async notification. 

            \param[in]  sessionId - the id of the matchmaking session to cancel.
            \param[in]  ownerSession - the owner user session who start this MM session, should never be nullptr
            \return ERR_OK, if the session was canceled; otherwise a cancelMatchmaking error to return to the client
        *************************************************************************************************/
        BlazeRpcError cancelSession(MatchmakingSessionId sessionId, UserSessionId ownerSessionId);

        bool validateConfig(const Blaze::Matchmaker::MatchmakerConfig& config, const Blaze::Matchmaker::MatchmakerConfig *referenceConfig, ConfigureValidationErrors& validationErrors);

        /*! ************************************************************************************************/
        /*!
            \brief perform initial matchmaking configuration (parse the matchmaking config file & init
                all rule definitions).
        
            \param[in]  configMap - the config map containing the matchmaking config block
            \param[out] true if all settings configed properly, false if default settings are used
        *************************************************************************************************/
        bool configure(const Blaze::Matchmaker::MatchmakerConfig& config);

        /*! ************************************************************************************************/
        /*!
            \brief Reconfigure matchmaking using the new matchmaking configuration.
                - parse the matchmaking config file & make sure there are no errors.
                - If there are no errors kick everyone out of matchmaking and clean up the old data.
                - Set the new rule definitions and other matchmaking data.
        
            \param[in]  configMap - the config map containing the matchmaking config block
            \param[out] true if the new settings configed properly, false if the old settings are used.
        *************************************************************************************************/
        bool reconfigure(const Blaze::Matchmaker::MatchmakerConfig& config);

        /*! ************************************************************************************************/
        /*!
            \brief This method needs to be invoked by host component in its onResolve() method
        
            \param[out] true if everything went well and matchmaker has setup all remote connections
        *************************************************************************************************/
        bool hostingComponentResolved();

        const MatchmakingConfig& getMatchmakingConfig() const { return mMatchmakingConfig; }

        const RuleDefinitionCollection& getRuleDefinitionCollection() const { return *mRuleDefinitionCollection; }
        
        RuleDefinitionCollection& getRuleDefinitionCollection() { return *mRuleDefinitionCollection; }

        /*! ************************************************************************************************/
        /*!
            \brief Destroy all matchmaking sessions owned by the supplied user.
            \param[in]  dyingUserSession The userSession that's disconnecting (or being destroyed).
        *************************************************************************************************/
        void destroySessionForUserSessionId(UserSessionId dyingUserSessionId);

        /*! ************************************************************************************************/
        /*!
            \brief Destroy the matchmaking session specified by Id.
            \param[in]  sessionId The matchmaking session id of the user to destroy.
            \param[in]  result The result of the matchmaking session.
            \param[in]  the matched game's ID (if successful matchmaking)
        *************************************************************************************************/
        void destroySession(MatchmakingSessionId sessionId, MatchmakingResult result, GameId gameId = GameManager::INVALID_GAME_ID);

        /*! ************************************************************************************************/
        /*! \brief return a pointer to a cached version of the matchmaking configuration

        \param[in\out] response the TDF object to pack the matchmaking configuration into
        \return ERR_OK
        ***************************************************************************************************/
        Blaze::Matchmaker::GetMatchmakingConfigError::Error setMatchmakingConfigResponse(GetMatchmakingConfigResponse *response) const;

        /*! ************************************************************************************************/
        /*! \brief return total number of matchmaking sessions attempting to create games 
        
            \return total number of CG matchmaking sessions
        ***************************************************************************************************/
        uint32_t getNumOfCreateGameMatchmakingSessions() const
        {
            return (uint32_t)mScenarioMetrics.mCreateGameMatchmakingSessions.get();
        }

        /*! ************************************************************************************************/
        /*! \brief return total number of matchmaking sessions attempting to find games.
        
            \return total number of FG matchmaking sessions
        ***************************************************************************************************/
        uint32_t getNumOfFindGameMatchmakingSessions() const
        {
            return (uint32_t)mScenarioMetrics.mFindGameMatchmakingSessions.get();
        }

        // Global total including all product names and session types.
        uint32_t getTotalMatchmakingSessions() const
        {
            return (uint32_t)(mScenarioMetrics.mCreateGameMatchmakingSessions.get() + mScenarioMetrics.mFindGameMatchmakingSessions.get());
        }

        MatchmakerMasterMetrics &getMatchmakerMetrics(const MatchmakingSession* mmSession = nullptr)
        {
            return mScenarioMetrics;
        }

        MatchmakingMetricsNew &getMatchmakingMetrics()
        {
            return mMatchmakingMetrics;
        }

        void addSessionDiagnosticsToMetrics(const MatchmakingSession& session, const MatchmakingSubsessionDiagnostics& countsToAdd, bool addCreateMode, bool addFindMode, bool mergeSessionCount);

        void getStatusInfo(ComponentStatus& status) const;

        /*! ************************************************************************************************/
        /*! \brief Override the give users matchmaking sessions to attempt to match into the provided game
        ***************************************************************************************************/
        void dedicatedServerOverride(BlazeId blazeId, GameId gameId);
        const DedicatedServerOverrideMap& getDedicatedServerOverrides() const;
        const GameManager::GameIdList& getFillServersOverride() const;
        void fillServersOverride(const GameManager::GameIdList& fillServersOverrideList);

        FullUserSessionsMap &getUserSessionsToMatchmakingMemberInfoMap() { return mUserSessionsToMatchmakingMemberInfoMap; }

        void addSession(MatchmakingSession &session)
        {
            mFullSessionList.insert(&session);
        }

        void removeSession(MatchmakingSession &session)
        {
            mFullSessionList.erase(&session);
        }

        MatchmakingSessionMatchNode * allocateMatchNode();
        void deallocateMatchNode(MatchmakingSessionMatchNode &node);

        // remove the finalizing session and advance the supplied iterator if it was invalidated by the removal
        void removeFinalizedSessions(MatchmakingSessionList &finalizedSessionList, MatchmakingSessionId finalizingSessionId);

        enum ReinsertionActionType
        {
            NO_ACTION,                    // don't do anything special for this reinsertion
            STOP_EVALUATING_CREATE_GAME,  // disable create game evaluation for dual-mode sessions (such as when reset dedicated server fails due to capacity issues.)
            SKIP_FIND_GAME_NEXT_IDLE, // sessions with find game enabled will not attempt to finalize in FG mode next idle (discarding any outstanding matches)
            EVALUATE_SESSION // Session has been modified and needs to re evaluate.  Used by QOS rule, as failing QoS adds users to block list
        };
        /*! ************************************************************************************************
            \brief take a list of matchmaking session ids to either expire (if timed out), or reactivate at the start of the next idle
            \param[in] reinsertedSessionList the set of session ids to reinsert
            \param[in] restartFindGameSessions if true, start up find game sessions that were previously terminated
            \param[in] reinsertionBehavior - determines if any special handling of this reinsertion is required.
        ************************************************************************************************/
        void reinsertFailedToFinalizeSessions(Blaze::Search::MatchmakingSessionIdList &reinsertedSessionList, bool restartFindGameSessions, ReinsertionActionType reinsertionAction, Rete::ProductionNodeId productionNodeId);

        void filloutMatchmakingStatus(MatchmakingStatus& status);
        void clearLastSentMatchmakingStatus();
        void getMatchmakingInstanceData(PlayerMatchmakingRateByScenarioPingsiteGroup& pmrMetrics, MatchmakingSessionsByScenario& scenarioData);


        // find game sharding broker override
        void onNotifyFindGameFinalizationUpdate(const Blaze::Search::NotifyFindGameFinalizationUpdate& data,UserSession *associatedUserSession) override;

        /*! ************************************************************************************************
            \brief returns true if the matchmaking session is found in either the create game or find game session map
        ************************************************************************************************/
        bool isMatchmakingSessionInMap(MatchmakingSessionId mmSessionId);

        /*! ************************************************************************************************
            \brief returns true if the user is a member of the matchmaking session
        ************************************************************************************************/
        bool isUserInCreateGameMatchmakingSession(UserSessionId userSessionId, MatchmakingSessionId mmSessionId);

        uint16_t getGlobalMinPlayerCountInGame() const { return (uint16_t)getMatchmakingConfig().getServerConfig().getGameSizeRange().getMin(); }

        void leaveMatchmakingExternalSession(MatchmakingSessionId mmSessionId, const XblSessionTemplateName& sessionTemplateName, ExternalUserLeaveInfoList& externalUserInfos) const;

        void fillNotifyReservedExternalPlayers(MatchmakingSessionId mmSessionId, GameId joinedGameId, NotifyMatchmakingReservedExternalPlayersMap& notificationMap);

        Blaze::Matchmaker::MatchmakerSlaveImpl& getMatchmakerComponent() { return mMatchmakerSlave; }

        MatchmakingSession* findSession(MatchmakingSessionId sessionId) const;
        MatchmakingSession* findCreateGameSession(MatchmakingSessionId sessionId) const;
        MatchmakingSession* findFindGameSession(MatchmakingSessionId sessionId) const;

        void dispatchMatchmakingConnectionVerifiedForSession(MatchmakingSessionId mmSessionId, GameId gameId, bool dispatchClientListener);

        bool validateQosForMatchmakingSessions(const Blaze::Search::MatchmakingSessionIdList &matchmakingSessionIds, const Blaze::GameManager::GameSessionConnectionComplete &request, Blaze::GameManager::QosEvaluationResult &response);

        // issues terminate requests to search slaves to stop FG search while QoS validation is in-progress
        void stopOutstandingFindGameSessions(MatchmakingSessionId mmSessionId, GameManager::MatchmakingResult result);

        // Lockdown commands:
        void setSessionLockdown(const MatchmakingSession* mmSession);
        void clearSessionLockdown(const MatchmakingSession* mmSession);
        bool isSessionLockedDown(const MatchmakingSession* mmSession, bool ignoreOtherSessionLock = false) const;
        void addSessionLock(const MatchmakingSession* mmSession);
        void removeSessionLock(const MatchmakingSession* mmSession);
        bool isOnlyScenarioSubsession(const MatchmakingSession* mmSession) const;
        Fiber::FiberGroupId getFiberGroupId() const { return mFiberGroupId; }

        void queueSessionForNewEvaluation(MatchmakingSessionId sessionId);
        void queueSessionForDirtyEvaluation(MatchmakingSessionId sessionId);
        void evaluateDirtyCreateGameSession(MatchmakingSessionId existingSessionId);
        void timeoutExpiredSession(MatchmakingSessionId expiredSessionId);

private:
        NodePoolAllocator &getMatchNodeAllocator() { return mMatchNodeAllocator; }
       
        /*! ************************************************************************************************/
        /*!
            \brief 'runs' the matchmaker for one simulation tick.  Scans matchmaking session list,
                evaluating & finalizing the sessions it can.
            \return SCHEDULE_IDLE_IMMEDIATELY if there are still queued sessions, SCHEDULE_IDLE_NORMALLY if there are other unfinalized sessions.
        *************************************************************************************************/
        IdleResult idle() override;

        /*! ************************************************************************************************/
        /*!
        \brief cache a matchmaking config response object for filling client requests.

        *************************************************************************************************/
        void initializeMatchmakingConfigResponse();

        /*! ************************************************************************************************/
        /*!
            \brief Clean up data in the config response including deleting all of the predefined rules,
                generic rules, and removing all of the threshold names from the special case ping site rule.

        *************************************************************************************************/
        void cleanUpConfigResponse();

        void evaluateNewCreateGameSession(MatchmakingSessionId newSessionId);


        /*! ************************************************************************************************/
        /*!
            \brief retrieves find game matches sent by search slaves, and locks down sessions with matches
            those sessions then attempt to join matched games in a scheduled job
        *************************************************************************************************/
        void processMatchedFindGameSessions();


        void startFinalizingSession(const MatchmakingSession& session);

        /*! ************************************************************************************************/
        /*!
            \brief Terminate all MM sessions.  This will delete and erase all MM sessions and send a 
                notification of MM finished with a result of SESSION_TERMINATED.
        *************************************************************************************************/
        void terminateAllSessions();


        /*! ************************************************************************************************/
        /*!
            \brief update the age of each matchmaking session, and mark sessions as dirty if a rule threshold changed.
            \param[in] now - the current time
        ***************************************************************************************************/
        void updateSessionAges(const TimeValue &now);
        void updateSessionAge(const TimeValue &now, MatchmakingSession &session);

        /*! ************************************************************************************************/
        /*! \brief create a new matchmaking session upon the request
        
            \param[in]request - StartMatchmakingInternalRequest, will be cached off && updated with mm session properties
            \param[in]userSession - user session for the matchmaking session to be created
            \param[in]matchmakingSessionId - match making session id generated for the new session
            \param[out]matchmakingSession - new created matchmaking session
            \param[out]err - error obj passed out
            \return StartMatchmakingMasterError::Error
        ***************************************************************************************************/
        BlazeRpcError createNewSession(const StartMatchmakingInternalRequest &request, MatchmakingSessionId matchmakingSessionId, MatchmakingSession** matchmakingSession, Blaze::ExternalSessionIdentification& externalSessionIdentification, MatchmakingCriteriaError &err);
        void onStatusUpdateTimer();

        // Tracks known information on game capacities retrieved from search slaves
        // enabling the matchmaker to avoid local contention by skipping games
        // that have pending join count >= capacity.
        struct GameCapacityInfo
        {
            GameId gameId;
            uint32_t refCount;
            uint32_t participantCapacity;
            uint32_t participantCount;
            uint32_t pendingJoinCount;
            GameCapacityInfo() : gameId(INVALID_GAME_ID), refCount(0), participantCapacity(0), participantCount(0), pendingJoinCount(0) {}
        };

        typedef eastl::intrusive_ptr<GameCapacityInfo> GameCapacityInfoPtr; // reference used to auto-remove game capacity information that is not used by any MM session
        typedef eastl::vector<MatchedGamePtr> MatchedGamePtrList;
        typedef eastl::map<FitScore, MatchedGamePtrList> MatchedGameListByFitScoreMap;

        void finalizeDebuggingSession(MatchmakingSessionId mmSessionId);

        void sendFoundGameToGameManagerMaster(Search::NotifyFindGameFinalizationUpdatePtr foundGamesUpdate, TimeValue matchmakingTimeoutDuration);

        void addStartMetrics(MatchmakingSession* newSession, bool setStartMetrics);
        void trackPlayerMatchmakingRate(const ScenarioName& scenarioName, const PingSiteAlias& pingSite, const DelineationGroupType& delineationGroup);

        uint64_t getTotalFinishedMatchmakingSessions(const MatchmakerMasterMetrics& metrics) const { return (metrics.mTotalMatchmakingSessionSuccess.getTotal() + metrics.mTotalMatchmakingSessionCanceled.getTotal() + metrics.mTotalMatchmakingSessionTerminated.getTotal() + metrics.mTotalMatchmakingSessionTimeout.getTotal()); }
        void incrementTotalFinishedMatchmakingSessionsMetric(MatchmakingResult result, MatchmakingSession& dyingSession, bool onReconfig = false);

        void incrementFinishedMmSessionDiagnostics(MatchmakingResult result, MatchmakingSession& dyingSession);

        void userSessionInfosToExternalUserLeaveInfos(const GameManager::UserSessionInfoList& members, Blaze::ExternalUserLeaveInfoList& externalUserLeaveInfos) const;

        void restartFindGameSessionAfterReinsertion(Blaze::GameManager::MatchmakingSessionId mmSessionId);

        void clearQosMetrics();


    private:
        // Job queue configuration vars.  Don't currently see a need to expose via configuration
        const static uint32_t WORKER_FIBERS = FiberJobQueue::ONE_WORKER_LIMIT;
        const static uint32_t JOB_QUEUE_CAPACITY = 10000;
        Blaze::Matchmaker::MatchmakerSlaveImpl& mMatchmakerSlave;
        MatchmakingConfig mMatchmakingConfig; // Previous Config Map used to check on reconfigure.

        MatchmakerMasterMetrics mScenarioMetrics;   // Like normal metrics, but more scenario-y.
        MatchmakingMetricsNew mMatchmakingMetrics;

        typedef eastl::map<ScenarioHash, ScenarioMatchmakingMetrics*> ScenarioMatchmakingMetricsByScenarioHashMap;
        ScenarioMatchmakingMetricsByScenarioHashMap mScenarioMatchmakingMetricsByScenarioHashMap;

        struct PMRMetricsHelper
        {
            PMRMetricsHelper() : mUserCount(0), mLastUpdateTime(0) {}
            uint64_t mUserCount;
            TimeValue mLastUpdateTime;
        };
        typedef eastl::list<PMRMetricsHelper> PMRMetricsList;
        typedef eastl::map<DelineationGroupType, PMRMetricsList> PMRMetricsListByGroup;
        typedef eastl::map<PingSiteAlias, PMRMetricsListByGroup> PMRMetricsListByPingSiteGroup;
        typedef eastl::map<ScenarioName, PMRMetricsListByPingSiteGroup> PMRMetricsListByScenarioPingSiteGroup;
        PMRMetricsListByScenarioPingSiteGroup mScenarioPMRMetrics;  // Mapping by platform/Scenario/pingsite.
        PMRMetricsHelper* cleanupOldPlayerMatchmakingRateMetrics(PMRMetricsList& currentMetrics, bool createNewEntry);

        // Map of a gauge of mm sessions by Scenario
        typedef eastl::map<PingSiteAlias, uint32_t> MatchmakingSessionsByPingSiteMap;
        typedef eastl::map<ScenarioName, MatchmakingSessionsByPingSiteMap> MatchmakingSessionsByScenarioMap;
        MatchmakingSessionsByScenarioMap mSessionsByScenarioMap;

        struct SessionLockData
        {
            SessionLockData() : mIsLocked(false), mHasSetStartMetrics(false), mHasSetEndMetrics(false), mUsageCount(1), mScenarioSubsessionCount(0), mLockOwner(INVALID_MATCHMAKING_SESSION_ID) {}

            bool mIsLocked;             // 
            bool mHasSetStartMetrics;   // Has this session set metrics yet? 
            bool mHasSetEndMetrics;     // Has this session set metrics yet? 
            uint16_t mUsageCount;       // How many sessions are using this lock. (When 0, remove the entry)
            uint16_t mScenarioSubsessionCount; // How many scenario subsessions are in use (will be < mUsageCount if mm subsessions or mixed mode is used)
            MatchmakingSessionId mLockOwner;   // Which session triggered the lock (Scenario usage only)
        };
        typedef eastl::map<MatchmakingScenarioId, SessionLockData> ScenarioLockDataMap;
        typedef eastl::map<MatchmakingSessionId, SessionLockData> SessionLockDataMap;
        ScenarioLockDataMap mScenarioLockData;
        SessionLockDataMap mSessionLockData;
        SessionLockData* getSessionLockData(const MatchmakingSession* mmSession);
        SessionLockData const* getSessionLockDataConst(const MatchmakingSession* mmSession) const;

        typedef eastl::hash_map<InstanceId, Search::NotifyFindGameFinalizationUpdatePtr> FindGameUpdateByInstanceIdMap;
        typedef eastl::map<MatchmakingSessionId, FindGameUpdateByInstanceIdMap> FindGameUpdatesByMmSessionIdMap;
        FindGameUpdatesByMmSessionIdMap mFindGameUpdatesByMmSessionIdMap;

        // matchmaking sessions with find game enabled will always be inserted into mFindGameSessionMap upon creation
        // sessions with create game enabled will be in either the mCreateGameSessionMap or the mQueuedSession list
        // sessions may be in both the CG session map and the FG session map
        MatchmakingSessionMap mCreateGameSessionMap;
        MatchmakingSessionMap mFindGameSessionMap;

        // typedef eastl::map<ClientPlatformType, uint64_t> PlatformCount;


        FullMatchmakingSessionSet mFullSessionList;

        Rete::ReteNetwork mReteNetwork; //used by legacy matchmaking
        
        RuleDefinitionCollection* mRuleDefinitionCollection;

        GetMatchmakingConfigResponse mMatchmakingConfigResponse;

        DedicatedServerOverrideMap mDedicatedServerOverrideMap;
        GameManager::GameIdList mFillServersOverrideList;

        FullUserSessionsMap mUserSessionsToMatchmakingMemberInfoMap;

        NodePoolAllocator mMatchNodeAllocator;

        // data for MM idle status message
        uint32_t mNewSessionsAtIdleStart; // new sessions created, might be queued or active

        uint32_t mDirtySessionsAtIdleStart;
        uint32_t mCleanSessionsAtIdleStart;

        TimerId mStatusUpdateTimerId;
        Fiber::FiberGroupId mFiberGroupId;

        // Helper bool for distinguishing externalSessionTermination metric from the normla termination metric:
        bool mExternalSessionTerminated;

        typedef eastl::hash_map<GameId, GameCapacityInfo> GameCapacityInfoMap;
        GameCapacityInfoMap mGameCapacityInfoMap;

        friend void intrusive_ptr_add_ref(Matchmaker::GameCapacityInfo* ptr);
        friend void intrusive_ptr_release(Matchmaker::GameCapacityInfo* ptr);

        FiberJobQueue mEvaluateSessionQueue;

        MatchmakingStatus mLastSentStatus;
    };

    extern EA_THREAD_LOCAL Matchmaker* gMatchmaker;

    void intrusive_ptr_add_ref(Matchmaker::GameCapacityInfo* ptr);
    void intrusive_ptr_release(Matchmaker::GameCapacityInfo* ptr);

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_MATCHMAKER_H
