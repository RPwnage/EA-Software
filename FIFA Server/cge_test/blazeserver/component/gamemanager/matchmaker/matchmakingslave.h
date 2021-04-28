/*! ************************************************************************************************/
/*!
\file matchmakingslave.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_MATCHMAKING_MATCHMAKINGSLAVE_H
#define BLAZE_MATCHMAKING_MATCHMAKINGSLAVE_H

#include "gamemanager/matchmaker/matchmaker.h"
#include "gamemanager/rete/retenetwork.h"
#include "gamemanager/tdf/search_server.h"
#include "gamemanager/idlerutil.h"

namespace Blaze
{
namespace Search
{
    class SearchSlaveImpl;
    class GameSessionSearchSlave; // for createGameInstrusiveNode(GameSessionSearchSlave..)
}

namespace GameManager
{
    struct MmSessionGameIntrusiveNode;

namespace Matchmaker
{
    class MatchmakingSessionSlave;

    struct GameMatchInfo
    {
        GameId gameId;
        FitScore fitScore;
        FitScore maxFitScore;
    };

    class MatchmakingSlave : private IdlerUtil
    {
        NON_COPYABLE(MatchmakingSlave);

    public:



        MatchmakingSlave(Search::SearchSlaveImpl &searchSlaveImpl);
        ~MatchmakingSlave() override;

        void onShutdown();

        MatchmakingSessionSlave* createMatchmakingSession(
            MatchmakingSessionId sessionId,
            const StartMatchmakingInternalRequest& req, 
            BlazeRpcError& err, 
            MatchmakingCriteriaError& criteriaError,
            GameManager::MatchmakingSubsessionDiagnostics& diagnostics, 
            InstanceId originatingInstanceId);
        void addToMatchingSessions(MatchmakingSessionSlave& session);
        void removeFromMatchingSessions(MatchmakingSessionSlave& session);

        void processMatchmakingSession(MatchmakingSessionId sessionId);

        /*! ************************************************************************************************/
        /*!
            \brief Find a matchmaking session by Id.
        
            \param[in] id - The id of the matchmaking session.
        
            \return The slave matchmaking session.  nullptr if none exists by this id.
        *************************************************************************************************/
        MatchmakingSessionSlave* findMatchmakingSession(Rete::ProductionId id);

        /*! ************************************************************************************************/
        /*!
            \brief cancel the supplied matchmaking session.
        
            \param[in] matchmakingSession - The matchmaking session to cancel.
        *************************************************************************************************/
        void cancelMatchmakingSession(Rete::ProductionId id, GameManager::MatchmakingResult result);

        bool configure(const MatchmakingServerConfig& config);
        bool reconfigure(const MatchmakingServerConfig& config);
        bool validateConfig(const MatchmakingServerConfig& config, const MatchmakingServerConfig *referenceConfig, ConfigureValidationErrors& validationErrors) const;

        const MatchmakingConfig& getMatchmakingConfig() const { return mMatchmakingConfig; }

        void onUserSessionExtinction(UserSessionId sessionId);

        void addSessionForUser(UserSessionId userSessionId, MatchmakingSessionSlave *session);
        void removeSessionForUser(UserSessionId userSessionId, MatchmakingSessionSlave *session);

        Search::SearchSlaveImpl& getSearchSlave() const { return mSearchSlave; }

        const RuleDefinitionCollection& getRuleDefinitionCollection() const { return *mRuleDefinitionCollection; }
        
        RuleDefinitionCollection& getRuleDefinitionCollection() { return *mRuleDefinitionCollection; }

        void onUpdate(const Search::GameSessionSearchSlave &game);

        void getStatusInfo(ComponentStatus& status) const;
        void dumpDebugInfo();

        void deleteGameIntrusiveNode(Blaze::GameManager::MmSessionGameIntrusiveNode *node);
        Blaze::GameManager::MmSessionGameIntrusiveNode* createGameInstrusiveNode(Search::GameSessionSearchSlave& game, MatchmakingSessionSlave& sessionSlave);

        void incrementWorkingMemoryUpdatesAtIdle() { ++mIdleMetrics.mWorkingMemoryUpdatesAtIdle; }

        void dedicatedServerOverride(BlazeId blazeId, GameId gameId);
        void fillServersOverride(const GameManager::GameIdList& fillServersOverrideList);
        const GameManager::GameIdList& getFillServersOverride() const { return mFillServersOverrideList; } 

        // For AvoidPlayersRule and PreferredPlayersRule
        PlayerToGamesMapping& getPlayerGamesMapping() { return mPlayerGamesMapping; }
        XblAccountIdToGamesMapping& getXblAccountIdGamesMapping() { return mXblAccountIdGamesMapping; }
        void onUserJoinGame(const BlazeId playerId, const PlatformInfo& platformInfo, const Search::GameSessionSearchSlave& gameSession);
        void onUserLeaveGame(const BlazeId playerId, const PlatformInfo& platformInfo, const Search::GameSessionSearchSlave& gameSession);

        uint32_t getNumOfActiveMatchMakingSessions() const { return (uint32_t)mSessionSlaveMap.size(); }

        void queueSessionForTimeout(MatchmakingSessionSlave& session);
        void cleanupSession(MatchmakingSessionId sessionId,  MatchmakingResult result);
        
    private:

        IdleResult idle() override;

        typedef eastl::hash_map < Rete::ProductionId, MatchmakingSessionSlave* > SessionSlaveMap;
        typedef eastl::hash_set < Rete::ProductionId > SessionMatchMap;
        typedef eastl::list< Rete::ProductionId > SessionMatchQueue;

        void updateSessionAges(const TimeValue& now, const TimeValue& prevIdle);

        uint64_t processDirtyGames();

        SessionSlaveMap::iterator& destroySession(SessionSlaveMap::iterator& iter);

        void addToMatchingSessionsQueue(Rete::ProductionId id);
    
    private:
        typedef eastl::set<MatchmakingSessionSlave*> MatchmakingSessionSlaveSet;
        typedef eastl::set<GameId> GameIdSet;

        typedef eastl::set<Rete::ProductionId> MatchmakingSessionSlaveIdSet;
        typedef eastl::hash_map<UserSessionId, MatchmakingSessionSlaveIdSet> UserSessionMMSessionSlaveMap;
        UserSessionMMSessionSlaveMap mUserSessionMMSessionSlaveMap;

        Search::SearchSlaveImpl& mSearchSlave;
        
        // Matchmakers configuration values
        MatchmakingConfig mMatchmakingConfig;
        RuleDefinitionCollection* mRuleDefinitionCollection;

        // Idle Time values
        TimeValue mLastIdleLength;
        TimeValue mLastIdleStartTime;
        Metrics::Counter mTotalIdles;
        Metrics::PolledGauge mLastIdleTimeMs;
        Metrics::PolledGauge mLastIdleTimeToIdlePeriodRatioPercent;

        SessionSlaveMap mSessionSlaveMap;
        SessionMatchMap mMatchedSessionIdSet;// used to ensure uniqueness in mMatchedSessionIdQueue

        GameIdSet mDirtyGameSet;

        // MatchmakingDedicatedServerOverride map
        DedicatedServerOverrideMap mDedicatedServerOverrideMap;
        GameManager::GameIdList mFillServersOverrideList;

        // data for MM idle status message
        struct MatchmakingSlaveIdleMetrics
        {
            MatchmakingSlaveIdleMetrics()
                : mNewSessionsAtIdleStart(0),
                mFinalizedSessionsAtIdleStart(0),
                mJoinedSessionsAtIdleStart(0),
                mImmediateFinalizeAttemptsAtIdleStart(0),
                mWorkingMemoryUpdatesAtIdle(0),
                mLoggedOutSessionsAtIdleStart(0),
                mTimedOutSessionsAtIdleStart(0)
            {
            }

            void reset() 
            {
                mNewSessionsAtIdleStart = 0;
                mFinalizedSessionsAtIdleStart = 0;
                mJoinedSessionsAtIdleStart = 0;
                mImmediateFinalizeAttemptsAtIdleStart = 0;
                mWorkingMemoryUpdatesAtIdle = 0;
                mLoggedOutSessionsAtIdleStart = 0;
                mTimedOutSessionsAtIdleStart = 0;
            }

            uint32_t mNewSessionsAtIdleStart;
            uint32_t mFinalizedSessionsAtIdleStart;
            uint32_t mJoinedSessionsAtIdleStart;
            uint32_t mImmediateFinalizeAttemptsAtIdleStart;
            uint32_t mWorkingMemoryUpdatesAtIdle;
            uint32_t mLoggedOutSessionsAtIdleStart;
            uint32_t mTimedOutSessionsAtIdleStart;
        };
        MatchmakingSlaveIdleMetrics mIdleMetrics;


        NodePoolAllocator mGameIntrusiveNodeAllocator;

        //
        // Matchmaking Metrics
        //
        // move to own class if this gets big
        struct MatchmakingSlaveMetrics
        {
        public:
            MatchmakingSlaveMetrics(Metrics::MetricsCollection& collection)
                : mMetricsCollection(collection)
                , mTotalSessionsCanceled(mMetricsCollection, "sessionsCanceled")
                , mTotalSessionsTimedOut(mMetricsCollection, "sessionsTimedOut")
                , mTotalSessionSuccessDurationMs(mMetricsCollection, "sessionSuccessDurationMs")
                , mTotalSessionCanceledDurationMs(mMetricsCollection, "sessionCanceledDurationMs")
                , mTotalSessionsStarted(mMetricsCollection, "sessionsStarted")
            {
            }

        private:
            Metrics::MetricsCollection& mMetricsCollection;

        public:
            Metrics::Counter mTotalSessionsCanceled;
            Metrics::Counter mTotalSessionsTimedOut;
            Metrics::Counter mTotalSessionSuccessDurationMs;
            Metrics::Counter mTotalSessionCanceledDurationMs;
            Metrics::Counter mTotalSessionsStarted;
        };
        MatchmakingSlaveMetrics mMetrics;
        Metrics::PolledGauge mActiveFGSessions;
        Metrics::PolledGauge mFGSessionMatches;
        Metrics::PolledGauge mGarbageCollectableNodes;

        PlayerToGamesMapping mPlayerGamesMapping;
        XblAccountIdToGamesMapping mXblAccountIdGamesMapping;

        FiberJobQueue mSessionJobQueue;
    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze
#endif
