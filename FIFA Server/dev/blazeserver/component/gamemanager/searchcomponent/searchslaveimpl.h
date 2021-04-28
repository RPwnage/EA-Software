/*! ************************************************************************************************/
/*!
    \file searchslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_SEARCH_SLAVE_IMPL_H
#define BLAZE_SEARCH_SLAVE_IMPL_H

#include "framework/replication/replicationcallback.h"
#include "framework/controller/controller.h"
#include "framework/storage/storagelistener.h"

#include "gamemanager/rpc/searchslave_stub.h"
#include "gamemanager/rpc/gamemanagermaster.h" // for GameManager master notification listeners
#include "gamemanager/rpc/gamemanagerslave_stub.h" // for GameManager master notification listeners
#include "gamemanager/playerinfo.h" 
#include "gamemanager/matchmaker/matchmakingslave.h"
#include "gamemanager/matchmaker/ruledefinitioncollection.h"
#include "gamemanager/matchmaker/matchmakingconfig.h"
#include "gamemanager/rete/retenetwork.h"
#include "gamemanager/gamemanagervalidationutils.h" // for GameIdByPersistedIdMap
#include "gamemanager/searchcomponent/usersetsearchmanager.h"
#include "gamemanager/idlerutil.h"
#include "EASTL/vector_set.h"
#include "gamemanager/scenario.h"
#include "gamemanager/matchmakingfiltersutil.h"
#include "framework/usersessions/usersessionsubscriber.h" // for UserSessionSubscriber

namespace Blaze
{


namespace Search
{
    class GameList;
    class GameSessionSearchSlave;


    // TODO:
    // See what needs to be here
    struct SearchIdleMetrics
    {
    public:
        SearchIdleMetrics(Metrics::MetricsCollection& collection)
            : mMetricsCollection(collection)
            , mTotalGamesSentToGameLists(mMetricsCollection, "totalGamesSentToGameListsInLastIdle")
            , mUniqueGamesSentToGameLists(mMetricsCollection, "uniqueGamesSentToGameListsInLastIdle")
            , mTotalNewGamesSentToGameLists(mMetricsCollection, "totalNewGamesSentToGameListsInLastIdle")
            , mUniqueNewGamesSentToGameLists(mMetricsCollection, "uniqueNewGamesSentToGameListsInLastIdle")
            , mTotalUpdatedGamesSentToGameLists(mMetricsCollection, "totalUpdatedGamesSentToGameListsInLastIdle")
            , mUniqueUpdatedGamesSentToGameLists(mMetricsCollection, "uniqueUpdatedGamesSentToGameListsInLastIdle")
            , mTotalRemovedGamesSentToGameLists(mMetricsCollection, "totalRemovedGamesSentToGameListsInLastIdle")
            , mTotalGameListsDeniedUpdates(mMetricsCollection, "totalGameListsDeniedUpdatesInLastIdle")
            , mTotalGameListsUpdated(mMetricsCollection, "totalGameListsUpdatedLastIdle")
            , mTotalGameListsNotUpdated(mMetricsCollection, "totalGameListsNotUpdatedLastIdle")
        {
            reset();
        }

    private:
        Metrics::MetricsCollection& mMetricsCollection;

    public:
        uint64_t mNewListsAtIdleStart;
        uint64_t mListsDestroyedAtIdleStart;
        uint64_t mGamesMatchedAtCreateList;
        TimeValue mGamesMatchedAtCreateList_SumTimes;

        uint64_t mGameUpdatesByIdleEnd;
        uint64_t mGamesRemovedFromMatchesByIdleEnd;
        uint64_t mVisibleGamesUpdatesByIdleEnd;
        TimeValue mLastIdleEndTime;

        TimeValue mFilledAtCreateList_SumTimes;
        TimeValue mFilledAtCreateList_MaxTime;
        TimeValue mFilledAtCreateList_MinTime;

        Metrics::Gauge mTotalGamesSentToGameLists;
        Metrics::Gauge mUniqueGamesSentToGameLists;
        Metrics::Gauge mTotalNewGamesSentToGameLists;
        Metrics::Gauge mUniqueNewGamesSentToGameLists;
        Metrics::Gauge mTotalUpdatedGamesSentToGameLists;
        Metrics::Gauge mUniqueUpdatedGamesSentToGameLists;
        Metrics::Gauge mTotalRemovedGamesSentToGameLists;
        Metrics::Gauge mTotalGameListsDeniedUpdates;
        Metrics::Gauge mTotalGameListsUpdated;
        Metrics::Gauge mTotalGameListsNotUpdated;


        void reset()
        {
            mNewListsAtIdleStart = 0;
            mListsDestroyedAtIdleStart = 0;
            mGamesMatchedAtCreateList = 0;
            mGamesMatchedAtCreateList_SumTimes = 0;

            mGameUpdatesByIdleEnd = 0;
            mGamesRemovedFromMatchesByIdleEnd = 0;
            mVisibleGamesUpdatesByIdleEnd = 0;
            mLastIdleEndTime = 0;

            mFilledAtCreateList_SumTimes = 0;
            mFilledAtCreateList_MaxTime = 0;
            mFilledAtCreateList_MinTime = 0;

            mTotalGamesSentToGameLists.set(0);
            mUniqueGamesSentToGameLists.set(0);
            mTotalNewGamesSentToGameLists.set(0);
            mUniqueNewGamesSentToGameLists.set(0);
            mTotalUpdatedGamesSentToGameLists.set(0);
            mUniqueUpdatedGamesSentToGameLists.set(0);
            mTotalRemovedGamesSentToGameLists.set(0);
            mTotalGameListsDeniedUpdates.set(0);
            mTotalGameListsUpdated.set(0);
            mTotalGameListsNotUpdated.set(0);
        }
    };


    class SearchSlaveImpl : public SearchSlaveStub,
                            private UserSessionSubscriber,
                            private Blaze::Controller::RemoteServerInstanceListener,
                            private Blaze::Controller::DrainStatusCheckHandler,
                            public GameManager::IdlerUtil
    {
    public:
        SearchSlaveImpl();
        ~SearchSlaveImpl() override;

        //Game fetchers
        GameSessionSearchSlave* getGameSessionSlave(GameManager::GameId id) const;
        const GameManager::UserSessionGameSet* getUserSessionGameSetBySessionId(UserSessionId userSessionId) const;
        const GameManager::UserSessionGameSet* getUserSessionGameSetByBlazeId(BlazeId blazeId) const;
        const GameManager::UserSessionGameSet* getUserSessionGameSetByUserId(BlazeId blazeId) const { return getUserSessionGameSetByBlazeId(blazeId); } // DEPPRECATED
        const GameManager::UserSessionGameSet* getDisconnectReservationsByBlazeId(BlazeId blazeId) const;
        const GameManager::UserSessionGameSet* getDisconnectReservationsByUserId(BlazeId blazeId) const { return getDisconnectReservationsByBlazeId(blazeId); } // DEPPRECATED

        // rete network
        bool isGameForRete(const GameSessionSearchSlave& gameSessionSlave) const;
        void insertWorkingMemoryElements(GameSessionSearchSlave& gameSessionSlave);
        void removeWorkingMemoryElements(GameSessionSearchSlave& gameSessionSlave);
        void updateWorkingMemoryElements(GameSessionSearchSlave& gameSessionSlave);

        // public accessors
        GameManager::Rete::ReteNetwork& getReteNetwork() { return mReteNetwork; }
        const GameManager::Rete::ReteNetwork& getReteNetwork() const { return mReteNetwork; }
        size_t getReplicatedGamesCount() const { return mGameSessionSlaveMap.size(); }
        GameManager::Matchmaker::MatchmakingSlave& getMatchmakingSlave() { return mMatchmakingSlave; }
        const GameManager::Matchmaker::MatchmakingSlave& getMatchmakingSlave() const { return mMatchmakingSlave; }
        const GameManager::GameBrowserListConfig *getListConfig(const char8_t *name) const;

        size_t getNumberOfGames() const { return mGameSessionSlaveMap.size(); }

        // other public methods
        GameManager::FitScore evaluateAdditionalFitScoreForGame( GameSessionSearchSlave &gameSession, 
            const GameManager::Rete::ProductionInfo& info, GameManager::DebugRuleResultMap& debugResultMap, bool useDebug = false) const;

        void onGameSessionUpdated(const GameSessionSearchSlave &gameSession);

        const GameManager::MatchmakingServerConfig& getMatchmakingConfig() const;
        bool getDoGameProtocolVersionStringCheck() const;

        // notifications
        void sendFindGameFinalizationUpdateNotification(SlaveSession *session, const Blaze::Search::NotifyFindGameFinalizationUpdate &data);
        void sendGameListUpdateNotification(SlaveSession *session, const Blaze::Search::NotifyGameListUpdate& data);
        
        void addList(GameListBase& list);
        GameListBase* getList(GameManager::GameBrowserListId listId);

        // game browser features
        // intrusive list node handling
        Blaze::GameManager::GbListGameIntrusiveNode* createGameInstrusiveNode(GameSessionSearchSlave& game, GameList& gbList);
        void deleteGameIntrusiveNode(Blaze::GameManager::GbListGameIntrusiveNode *node);

        GameMatch* createGameMatch(GameManager::FitScore fitScore, GameManager::GameId gameId, GameManager::Rete::ProductionInfo* pInfo);
        void deleteGameMatch(GameMatch* node);

        // metrics
        void incrementMetric_GameUpdatesByIdleEnd() { ++mIdleMetrics.mGameUpdatesByIdleEnd; }
        void incrementMetric_VisibleGamesUpdatesByIdleEnd() { ++mIdleMetrics.mVisibleGamesUpdatesByIdleEnd; }
        void incrementMetric_GamesRemovedFromMatchesByIdleEnd() { ++mIdleMetrics.mGamesRemovedFromMatchesByIdleEnd; }

        // metrics
        void getStatusInfo(ComponentStatus& status) const override;

        void initGameBrowserGameData(GameManager::GameBrowserGameData &gameBrowserGameData, const eastl::string &listConfigName, const Search::GameSessionSearchSlave& gameSession, const GameManager::PropertyNameList* propertiesUsed = nullptr);

        UserSetSearchManager& getUserSetSearchManager() { return mUserSetSearchManager; }

        TimeValue getDefaultIdlePeriod() const;
 
        void addGameForUser(UserSessionId userSessionId, Search::GameSessionSearchSlave &game);
        virtual void removeGameForUser(UserSessionId userSessionId, BlazeId blazeId, Search::GameSessionSearchSlave &game);

        //DrainStatusCheckHandler interface
        bool isDrainComplete() override { return (mMatchmakingSlave.getNumOfActiveMatchMakingSessions() == 0); }

        size_t getNumberOfLists() const { return mAllLists.size(); }

        void printReteJoinNodeBackTrace(const GameManager::Rete::JoinNode& joinNode, const GameManager::Rete::ProductionId& id, const GameManager::Rete::ProductionListener* listener, GameManager::DebugRuleResultMap& debugResultMap, bool recurse = true) const;
        void printReteBackTrace(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info, GameManager::DebugRuleResultMap& debugResultMap) const;
        bool printReteBackTrace(const GameManager::Rete::ProductionBuildInfo& info, GameManager::GameId gameId, const GameManager::Rete::ProductionListener* listener, GameManager::DebugRuleResultMap& debugResultMap, bool recurse) const;
        const GameManager::Rete::JoinNode* scanReteForToken(const GameManager::Rete::ProductionBuildInfo& info, GameManager::GameId gameId) const;

        // return true if the given topology needs QoS validation
        bool shouldPerformQosCheck(GameNetworkTopology topology) const;

        const GameManager::Matchmaker::DiagnosticsSearchSlaveHelper& getDiagnosticsHelper() const { return mDiagnosticsHelper; }
        GameManager::Matchmaker::DiagnosticsSearchSlaveHelper& getDiagnosticsHelper() { return mDiagnosticsHelper; }

        StorageRecordVersion getGameRecordVersion(GameManager::GameId gameId) const;

    private:
        // fiberless commands        
        TerminateFindGameMatchmakingError::Error processTerminateFindGameMatchmaking(const Blaze::Search::TerminateFindGameMatchmakingRequest &request, const Message* message) override;
        CreateGameListError::Error processCreateGameList(const Blaze::Search::CreateGameListRequest &request, Blaze::Search::CreateGameListResponse &response, Blaze::GameManager::MatchmakingCriteriaError &error, const ::Blaze::Message* message) override;
        TerminateGameListError::Error processTerminateGameList(const Blaze::Search::TerminateGameListRequest &,const Blaze::Message *) override;
        MatchmakingDedicatedServerOverrideError::Error processMatchmakingDedicatedServerOverride(const Blaze::GameManager::MatchmakingDedicatedServerOverrideRequest &, const Blaze::Message *) override;
        MatchmakingFillServersOverrideError::Error processMatchmakingFillServersOverride(const Blaze::GameManager::MatchmakingFillServersOverrideList &request, const Message* message) override;
        DumpDebugInfoError::Error processDumpDebugInfo(const Message* message) override;
        GetGamesError::Error processGetGames(const Blaze::Search::GetGamesRequest &request, Blaze::Search::GetGamesResponse &response, const Message* message) override;        
        typedef eastl::set<Blaze::GameManager::GameType> GameTypeSet;
        void storeGamesInGetGameResponse(const Blaze::Search::GetGamesRequest& request, Blaze::Search::GetGamesResponse &response, const GameTypeSet& gameTypeSet, const GameManager::UserSessionGameSet* userGameSet, bool needOnlyOne, bool& found, bool& mismatchedProtocolVer);
        void storeGameInGetGameResponse(const Blaze::Search::GetGamesRequest& request, Blaze::Search::GetGamesResponse &response, const eastl::string& listConfigName, GameSessionSearchSlave& game);
        CreateUserSetGameListError::Error processCreateUserSetGameList(const Blaze::Search::CreateUserSetGameListRequest &request, Blaze::Search::CreateGameListResponse &response, const Message* message) override { return mUserSetSearchManager.processCreateUserSetGameList(request, response, message); }
        UpdateUserSetGameListError::Error processUpdateUserSetGameList(const Blaze::Search::UserSetWatchListUpdate &request, const Message* message) override { return mUserSetSearchManager.processUpdateUserSetGameList(request, message); } 

    public:
        // commands that require fiber
        BlazeRpcError processStartFindGameMatchmaking(const StartFindGameMatchmakingRequest &request, Blaze::Search::StartFindGameMatchmakingResponse& response, Blaze::GameManager::MatchmakingCriteriaError &criteriaError, InstanceId originatingInstanceId);

    private:
        //Component functions

        bool onValidatePreconfig(SearchSlavePreconfig& config, const SearchSlavePreconfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
        bool onPreconfigure() override;
        bool onResolve() override;
        bool onConfigure() override;
        bool onReconfigure() override;
        void onShutdown() override;


        bool validatePreconfig(const SearchSlavePreconfig& config) const;

        void onUpsertGame(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue);
        void onUpsertPlayer(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue);
        void onErasePlayer(StorageRecordId recordId, const char8_t* fieldName, EA::TDF::Tdf& fieldValue);
        void onEraseGame(StorageRecordId recordId);

        bool onValidateConfig(SearchConfig& config, const SearchConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;
        bool onPrepareForReconfigure(const SearchConfig& newConfig) override;

        // user session game set
        void processDisconnectReservations(UserSessionId dyingUserSessionId, BlazeId dyingBlazeId);

        // UserSessionSubscriber impls
        void onUserSessionExtinction(const UserSession& userSession) override;

        // self notifications
        void onNotifyFindGameFinalizationUpdate(const Blaze::Search::NotifyFindGameFinalizationUpdate& data, UserSession*) override { };
        void onNotifyGameListUpdate(const Blaze::Search::NotifyGameListUpdate& data, UserSession*) override { };    

        // remote server instance listener
        void onRemoteInstanceChanged(const RemoteServerInstance& instance) override;

        GameManager::IdlerUtil::IdleResult idle() override;
        void sendListUpdateNotifications();

        // utilities
        bool destroyGameList(GameManager::GameBrowserListId listId);
        void destroyAllGameLists();

        // persisted id helpers
        GameSessionSearchSlave* getGameSessionSlaveByPersistedId(const char8_t* persistedGameId) const;

        void initGameBrowserPlayerData(GameManager::GameBrowserPlayerData &gameBrowserPlayerData, const eastl::string &listConfigName, GameManager::PlayerInfo &playerInfo);
        static void copyNamedAttributes(Collections::AttributeMap &destAttribMap, const Collections::AttributeMap &srcAttribMap, const Collections::AttributeNameList &attribNameList);

        void recurseReteForLowestToken(const GameManager::Rete::JoinNode* curNode, uint32_t& curLevel, GameManager::GameId gameId, const GameManager::Rete::JoinNode*& furthestJoinNode, uint32_t& furthestLevel) const;

    private:
        typedef eastl::intrusive_ptr<GameSessionSearchSlave> GameSessionSearchSlavePtr;
        typedef eastl::hash_map<GameManager::GameId, GameSessionSearchSlavePtr> GameSessionSlaveMap;
        typedef eastl::hash_map<UserSessionId, GameManager::UserSessionGameSet> UserSessionGamesSlaveMap;
        typedef eastl::map<BlazeId, GameManager::UserSessionGameSet> BlazeIdGamesSlaveMap;

        StorageListener::FieldHandler mGameFieldHandler;
        StorageListener::FieldHandler mPlayerFieldHandler;
        StorageListener::RecordHandler mGameRecordHandler;
        StorageListener mGameListener;
        GameSessionSlaveMap mGameSessionSlaveMap;

        // We maintain a separate list of games a particular user is in for fast removal on logout/die.
        //     We don't use users extended data as we don't want to wait for master's replication.
        //     Can't use const GameSet above as will be calling removeDyingSession() on the stored GameSessionSearchSlave pointers.
        UserSessionGamesSlaveMap mUserSessionGamesSlaveMap;

        BlazeIdGamesSlaveMap mDisconnectedBlazeIdGamesSlaveMap;

        // IMPORTANT: Pooled allocators members intentionally placed at the top
        // of the class declaration to ensure they are the first ctors and last dtors
        // called. This is because other data structures (e.g. ReteNetwork) depend on
        // the allocators.
        NodePoolAllocator mGameIntrusiveNodeAllocator;
        NodePoolAllocator mGameMatchAllocator;
        
        // idling
        typedef eastl::set<GameManager::GameId> GameIdSet;
        TimeValue mLastIdleLength;
        GameIdSet mDirtyGameSet;

        // rete network
        GameManager::Rete::ReteNetwork mReteNetwork;

         // from Matchmaker 
        GameManager::Matchmaker::MatchmakingSlave mMatchmakingSlave;


        GameManager::GameIdByPersistedIdMap mGameIdByPersistedIdMap; // a helper map used to do fast persisted game id lookup

        UserSetSearchManager mUserSetSearchManager;
        GameManager::PropertyManager mPropertyManager;

   private:
        // rete network
        void configureReteSubstringTries();

        // game browser
        typedef eastl::hash_map<GameManager::GameBrowserListId, GameListBase*> GameListByGameBrowserListId;
        typedef eastl::multimap<UserSessionId, GameManager::GameBrowserListId> GameListIdsByOwnerSessionId;
        typedef eastl::hash_set<GameManager::GameBrowserListId> GameListIdSet;
        typedef eastl::hash_map<InstanceId, GameListIdSet> GameListIdsByInstanceId;
        typedef eastl::hash_map<GameManager::GameBrowserListId, uint32_t> NumOfNewGameRequiredbyGameBrowserListId;

        GameListByGameBrowserListId mSubscriptionLists;
        GameListByGameBrowserListId mAllLists;
        GameListIdsByOwnerSessionId mGameListIdsByOwnerSessionId;
        GameListIdsByInstanceId     mGameListIdsByInstanceId;
        GameManager::GameBrowserListId mLastListProcessed;

        // metrics
        struct SearchMetrics
        {
        public:
            SearchMetrics(Metrics::MetricsCollection& collection)
                : mMetricsCollection(collection)
                , mTotalGamesMatchedAtCreateList(0)
                , mTotalSearchSessionsStarted(mMetricsCollection, "searchSessionsStarted")
                , mTotalGameListCreated(0)
                , mTotalIdles(mMetricsCollection, "idles")
                , mTotalGameUpdates(0)
                , mDirtyGameSetSize(0)
                , mTotalGameMatchesSent(mMetricsCollection, "gameMatchesSent")
                , mTotalGameNotifsSent(mMetricsCollection, "gameNotifsSent")
                , mGamesTrackedOnThisSlaveCount(mMetricsCollection, "gamesTrackedOnThisSlaveCount")
                , mTotalMatchUpdatesInGameListsSent(mMetricsCollection, "matchUpdatesInGameListsSent")
                , mTotalUniqueMatchUpdatesInGameListsSent(mMetricsCollection, "uniqueMatchUpdatesInGameListsSent")
                , mTotalGamesSentToGameLists(mMetricsCollection, "totalGamesSentToGameListsInLastIdle")
                , mUniqueGamesSentToGameLists(mMetricsCollection, "uniqueGamesSentToGameListsInLastIdle")
                , mTotalNewGamesSentToGameLists(mMetricsCollection, "totalNewGamesSentToGameListsInLastIdle")
                , mUniqueNewGamesSentToGameLists(mMetricsCollection, "uniqueNewGamesSentToGameListsInLastIdle")
                , mTotalUpdatedGamesSentToGameLists(mMetricsCollection, "totalUpdatedGamesSentToGameListsInLastIdle")
                , mUniqueUpdatedGamesSentToGameLists(mMetricsCollection, "uniqueUpdatedGamesSentToGameListsInLastIdle")
                , mTotalRemovedGamesSentToGameLists(mMetricsCollection, "totalRemovedGamesSentToGameListsInLastIdle")
                , mTotalGameListsDeniedUpdates(mMetricsCollection, "totalGameListsDeniedUpdatesInLastIdle")
                , mTotalGameListsUpdated(mMetricsCollection, "totalGameListsUpdatedLastIdle")
                , mTotalGameListsNotUpdated(mMetricsCollection, "totalGameListsNotUpdatedLastIdle")
            {
            }

        private:
            Metrics::MetricsCollection& mMetricsCollection;

        public:
            uint64_t mTotalGamesMatchedAtCreateList;
            TimeValue mFilledAtCreateList_SumTimes;
            TimeValue mTotalGamesMatchedAtCreateList_SumTimes;
            TimeValue mFilledAtCreateList_MaxTime;
            TimeValue mFilledAtCreateList_MinTime;
            Metrics::Counter mTotalSearchSessionsStarted;
            uint64_t mTotalGameListCreated;
            Metrics::Counter mTotalIdles;
            uint64_t mTotalGameUpdates;
            uint64_t mDirtyGameSetSize;
            Metrics::Counter mTotalGameMatchesSent;
            Metrics::Counter mTotalGameNotifsSent;
            Metrics::Gauge mGamesTrackedOnThisSlaveCount;
            Metrics::Counter mTotalMatchUpdatesInGameListsSent;
            Metrics::Counter mTotalUniqueMatchUpdatesInGameListsSent;
            
            // gb metrics
            Metrics::Counter mTotalGamesSentToGameLists;
            Metrics::Counter mUniqueGamesSentToGameLists;
            Metrics::Counter mTotalNewGamesSentToGameLists;
            Metrics::Counter mUniqueNewGamesSentToGameLists;
            Metrics::Counter mTotalUpdatedGamesSentToGameLists;
            Metrics::Counter mUniqueUpdatedGamesSentToGameLists;
            Metrics::Counter mTotalRemovedGamesSentToGameLists;
            Metrics::Counter mTotalGameListsDeniedUpdates;
            Metrics::Counter mTotalGameListsUpdated;
            Metrics::Counter mTotalGameListsNotUpdated;
        };

        SearchIdleMetrics mIdleMetrics;
        SearchMetrics mMetrics;
        Metrics::PolledGauge mAllGameLists;
        Metrics::PolledGauge mSubscriptionGameLists;
        Metrics::PolledGauge mLastIdleTimeMs;
        Metrics::PolledGauge mLastIdleTimeToIdlePeriodRatioPercent;
        GameManager::Matchmaker::DiagnosticsSearchSlaveHelper mDiagnosticsHelper;
        GameManager::MatchmakingScenarioManager mScenarioManager;
};


} // namespace Search
} // namespace Blaze

#endif // BLAZE_SEARCH_SLAVE_IMPL_H
