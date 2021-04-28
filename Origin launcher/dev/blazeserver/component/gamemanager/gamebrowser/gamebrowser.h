/*! ************************************************************************************************/
/*!
    \file gamebrowser.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMEBROWSER_H
#define BLAZE_GAMEMANAGER_GAMEBROWSER_H

#include "EATDF/time.h"
#include "gamemanager/matchmaker/matchmakingcriteria.h"
#include "gamemanager/tdf/gamebrowser.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/tdf/gamebrowser_server.h"
#include "gamemanager/rpc/gamemanagerslave_stub.h" // for RPC slave error enumerations
#include "gamemanager/gamemanagermetrics.h"
#include "gamemanager/searchcomponent/searchshardingbroker.h"
#include "gamemanager/idlerutil.h"
#include "gamemanager/gamebrowser/gamebrowserlistowner.h"
#include "EASTL/set.h"
#include "framework/util/safe_ptr_ext.h"
#include "framework/userset/userset.h" // Userset subscriber


namespace Blaze
{
    class UserSession;

namespace Metrics
{
    namespace Tag
    {
        extern TagInfo<GameManager::ListType>* list_type;
        extern TagInfo<GameManager::GameBrowserListConfigName>* gamebrowser_list_config_name;
    }
}

namespace GameManager
{
    class GameManagerSlaveImpl;    
    class GameBrowserList;
    class IGameBrowserListOwner;
    class UserSetGameBrowserList;
    class NotifyGameListUpdate;
    struct GameBrowserMetrics;
    struct GbListGameIntrusiveNode;
    class GameBrowserScenarioConfigInfo;
    class GameBrowserSubSessionConfigInfo;

    typedef safe_ptr_ext<class GameBrowserScenarioConfigInfo> GameBrowserScenarioConfigInfoRef;

    class GameBrowserSlaveMetrics
    {
    public:
        GameBrowserSlaveMetrics(Metrics::MetricsCollection& collection)
            : mMetricsCollection(collection)
            , mCreatedLists(mMetricsCollection, "createdLists", Metrics::Tag::list_type, Metrics::Tag::gamebrowser_list_config_name)
            , mActiveLists(mMetricsCollection, "activeLists", Metrics::Tag::list_type, Metrics::Tag::gamebrowser_list_config_name)
            , mInitialGamesSent(mMetricsCollection, "initialGamesSent", Metrics::Tag::list_type, Metrics::Tag::gamebrowser_list_config_name)
        {
        }

    private:
        Metrics::MetricsCollection& mMetricsCollection;

    public:
        using GameBrowserMetricCounter = Metrics::TaggedCounter<ListType, GameBrowserListConfigName>;
        using GameBrowserMetricGauge = Metrics::TaggedGauge<ListType, GameBrowserListConfigName>;

        GameBrowserMetricCounter mCreatedLists;
        GameBrowserMetricGauge mActiveLists;
        GameBrowserMetricCounter mInitialGamesSent;

    public:
        void getStatuses(ComponentStatus& outStatus) const
        {
            Blaze::ComponentStatus::InfoMap& map = outStatus.getInfo();
            const char8_t* metricName = "";
            char8_t buf[64];

            metricName = "GBGauge_CREATED_LISTS";
            blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mCreatedLists.getTotal()); // aggregate
            map[metricName] = buf;
            mCreatedLists.iterate([&map, &metricName](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(map, metricName, tagList, value.getTotal()); });

            metricName = "GBGauge_ACTIVE_LISTS";
            blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, mActiveLists.get()); // aggregate
            map[metricName] = buf;
            mActiveLists.iterate([&map, &metricName](const Metrics::TagPairList& tagList, const Metrics::Gauge& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(map, metricName, tagList, value.get()); });

            metricName = "GBGauge_INITIAL_GAMES_SENT";
            // don't need an aggregate for this metric
            mInitialGamesSent.iterate([&map, &metricName](const Metrics::TagPairList& tagList, const Metrics::Counter& value) { Metrics::addTaggedMetricToComponentStatusInfoMap(map, metricName, tagList, value.getTotal()); });
        }
    };

    /*! ************************************************************************************************/
    /*! \brief The GameBrowser is a slave only "pseudo" component that lives inside the game manager component.
    *************************************************************************************************/
    class GameBrowser : public Blaze::Search::SearchShardingBroker, public UserSetSubscriber, private GameManager::IdlerUtil
    {
        NON_COPYABLE(GameBrowser);

    public:

        enum NotifyGameListReason
        {
            LIST_UPDATED,
            LIST_DESTROYED
        };

        static const GameBrowserListConfigName DEDICATED_SERVER_LIST_CONFIG;
        static const GameBrowserListConfigName USERSET_GAME_LIST_CONFIG;

        GameBrowser(GameManagerSlaveImpl& gmSlave);
        
        ~GameBrowser() override;

        bool hostingComponentResolved();
        void onShutdown();
        bool validateConfig(const GameManagerServerConfig& configTdf, const GameManagerServerConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const;
        bool configure(const GameManagerServerConfig& configTdf, bool evaluateGameProtocolVersionString);
        bool reconfigure(const GameManagerServerConfig& configTdf, bool evaluateGameProtocolVersionString);
        const GameBrowserListConfig *getListConfig(const char8_t *name) const;
       
        void onUserSessionExtinction(UserSessionId sessionId);

        Blaze::BlazeRpcError processCreateGameListForScenario(GetGameListScenarioRequest& request,
            GetGameListResponse &response, MatchmakingCriteriaError &error, uint32_t msgNum);
        Blaze::BlazeRpcError getGameBrowserScenarioAttributesConfig(GetGameBrowserScenariosAttributesResponse& response);

        Blaze::BlazeRpcError processCreateGameList(GetGameListRequest& request, ListType listType, 
                                                GetGameListResponse &response, MatchmakingCriteriaError &error, uint32_t msgNum, IGameBrowserListOwnerPtr listOwner);
        Blaze::BlazeRpcError processGetUserSetGameListSubscription(const GetUserSetGameListSubscriptionRequest &request, 
            GetGameListResponse &response, MatchmakingCriteriaError &error,const Message *message);

        Blaze::BlazeRpcError processGetGameListSnapshotSync(GetGameListRequest &request, GetGameListSyncResponse &response, MatchmakingCriteriaError &error, 
            uint16_t requiredPlayerCapacity = 0, const char8_t* preferredPingSite = nullptr, GameNetworkTopology netTopology = NETWORK_DISABLED, const UserSessionInfo* ownerUserInfo = nullptr);

        Blaze::BlazeRpcError processDestroyGameList(const DestroyGameListRequest& request);

        TimeValue getDefaultIdlePeriod() const { return mGameBrowserConfig->getDefaultIdlePeriod(); }
        uint32_t getUserSetListCapacity() const { return mGameBrowserConfig->getUserSetListCapacity(); }
        uint32_t getMaxReserveSortedListCapacity() const {  return mGameBrowserConfig->getMaxReserveSortedListCapacity(); }
        uint32_t getMaxGamesPerListUpdateNotification() const { return mGameBrowserConfig->getMaxGamesPerListUpdateNotification(); }
        uint32_t getMaxGameListSyncSize() const { return mGameBrowserConfig->getMaxGameListSyncSize(); }
        uint32_t getMaxListUpdateNotificationsPerIdle() const { return mGameBrowserConfig->getMaxListUpdateNotificationsPerIdle(); }
        uint32_t getGameSessionContainerSize(uint32_t visibleListGameSize) const;

        void getStatusInfo(ComponentStatus& status) const;

        void incrementMetric_NewGameUpdatesByIdleEnd(uint32_t value) { mIdleMetrics.mNewGameUpdatesByIdleEnd += value; }

        void onAddSubscribe(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession*) override;
        void onRemoveSubscribe(EA::TDF::ObjectId compObjectId, BlazeId blazeId, const UserSession*) override;
        BlazeRpcError DEFINE_ASYNC_RET(destroyGameList(GameBrowserListId gameBrowserListId));

        void sendNotifyGameListUpdateToUserSessionById(UserSessionId ownerSessionId, const NotifyGameListUpdate* notifyGameListUpdate, bool sendImmediately, uint32_t msgNum);

        void cleanupDataForInstance(InstanceId searchInstanceId);

        Blaze::BlazeRpcError transformScenarioRequestToBaseRequest(GameBrowserScenarioData& scenarioData, GetGameListScenarioRequest& request, MatchmakingCriteriaError &error);

    private:
        typedef eastl::map<GameBrowserListId, GameBrowserList *> GameBrowserListMap;
        typedef eastl::map<GameBrowserListId, IGameBrowserListOwnerPtr> GameBrowserListOwnerMap; 

        Blaze::BlazeRpcError createGameList(GetGameListRequest& request, ListType listType,
            GameBrowserList*& newList, MatchmakingCriteriaError &error, uint32_t msgNum, IGameBrowserListOwnerPtr listOwner, uint32_t& numGamesToBeDownloaded, FitScore& maxPossibleFitScore,
            uint16_t maxPlayerCapacity = 0, const char8_t* preferredPingSite = nullptr, GameNetworkTopology netTopology = NETWORK_DISABLED, const UserSessionInfo* ownerUserInfo = nullptr);

        void deleteGameList(GameBrowserList* list);
        void destroyGameList(const GameBrowserListMap::iterator& listIter);
        void destroyUserSetGameList(UserSetGameBrowserList& list);
        //void destroyUserSetGameList(const UserSetGameBrowserListMap::iterator& listIter);
        void removeGameListFromUserOwnership(UserSessionId userSessionId, GameBrowserListId listId);

        IdleResult idle() override;
        void addAttributeMapping(ScenarioAttributeMapping* attrMapping, const ScenarioAttributeMapping& scenarioAttributeMapping);
        void sendListUpdateNotifications();

        // Sharding broker stuff.
        void onNotifyGameListUpdate(const Blaze::Search::NotifyGameListUpdate& data, UserSession*) override;  
        void onNotifyFindGameFinalizationUpdate(const Blaze::Search::NotifyFindGameFinalizationUpdate& data,UserSession *associatedUserSession) override{} // never called, these notifications are targeted by session id

    private:
        typedef eastl::hash_set<UserSetGameBrowserList*> GameBrowserListSet;

        BlazeRpcError createGameBrowserList(GameBrowserListId listId, GameBrowserList &list, const GetGameListRequest &request,  bool isSnapshot, 
            bool evaluateVersionProtocol, FitScore &maxPossibleFitScore, uint32_t &numberOfGamesToBeDownloaded, MatchmakingCriteriaError &error,
            uint16_t maxPlayerCapacity = 0, const char8_t* preferredPingSite = nullptr, GameNetworkTopology netTopology = NETWORK_DISABLED, const UserSessionInfo* ownerUserInfo = nullptr);
        BlazeRpcError createUserSetGameList(GameBrowserList& list, const Search::CreateUserSetGameListRequest& req, uint32_t &numberOfGamesToBeDownloaded);
        void updateUserSetGameList(EA::TDF::ObjectId compObjectId, BlazeId blazeId, bool add);
        BlazeRpcError terminateGameBrowserList(GameBrowserList& list, bool terminateRemoteList);

    private:
        // map so each game can track the lists its a member of - for fast list fixup when a game is destroyed.
        
        typedef eastl::map<EA::TDF::ObjectId, GameBrowserListSet> BlazeObjectIdToGameListMap;
        BlazeObjectIdToGameListMap mObjectIdToGameListMap;

        typedef eastl::hash_map<ComponentId, size_t> ComponentSubscriptionCountMap;
        ComponentSubscriptionCountMap mComponentSubscriptionCounts;
 
        GameManagerSlaveImpl& mGameManagerSlave;
        GameBrowserListMap mAllLists; // map that owns the list object, all lists are present in this map
        GameBrowserListMap mSubscriptionLists; // all subscription lists, some are also scanning
        GameBrowserListOwnerMap mAllListOwners;

        // idle metrics
        uint64_t mTotalIdles;

        struct GameBrowserMetrics
        {
            GameBrowserMetrics()
                : mTotalGamesMatchedAtCreateList(0)
                , mTotalGameUpdates(0)
            {
            }

            uint32_t mTotalGamesMatchedAtCreateList;
            uint32_t mTotalGameUpdates;
        };
        struct GameBrowserIdleMetrics
        {
            GameBrowserIdleMetrics()
            {
                reset();
            }

            uint32_t mNewListsAtIdleStart;
            uint32_t mListsDestroyedAtIdleStart;
            uint32_t mGamesMatchedAtCreateList;
            TimeValue mGamesMatchedAtCreateList_SumTimes;

            uint32_t mNewGameUpdatesByIdleEnd;
            TimeValue mLastIdleEndTime;

            TimeValue mFilledAtCreateList_SumTimes;
            TimeValue mFilledAtCreateList_MaxTime;
            TimeValue mFilledAtCreateList_MinTime;
            void reset() 
            {
                mNewListsAtIdleStart = 0;
                mListsDestroyedAtIdleStart = 0;
                mGamesMatchedAtCreateList = 0;
                mGamesMatchedAtCreateList_SumTimes = 0;

                mNewGameUpdatesByIdleEnd = 0;
                mLastIdleEndTime = 0;

                mFilledAtCreateList_SumTimes = 0;
                mFilledAtCreateList_MaxTime = 0;
                mFilledAtCreateList_MinTime = 0;
            }
        };

        //keep track of GameBrowserList owners for speeding destruction onUserSessionLogout
        typedef eastl::multimap<UserSessionId, GameBrowserListId> GameBrowserListUserSessionMap;
        GameBrowserListUserSessionMap mGameBrowserListUserSessionMap;
        
        GameBrowserSlaveMetrics mGBMetrics;
        GameBrowserMetrics mMetrics;
        GameBrowserIdleMetrics mIdleMetrics;
        bool mEvaluateGameProtocolVersionString;
        
        typedef eastl::map<ScenarioName, GameBrowserScenarioConfigInfoRef> ScenarioConfigByNameMap;
        ScenarioConfigByNameMap mScenarioConfigByNameMap;

        Metrics::PolledGauge mActiveAllLists;
        Metrics::PolledGauge mActiveSubscriptionLists;
        Metrics::Gauge mLastIdleLength; // in milliseconds

        const GameBrowserServerConfig* mGameBrowserConfig;
        const GameBrowserScenariosConfig* mGameBrowserScenarioConfig;
    };

    class GameBrowserScenarioConfigInfo :
        public eastl::safe_object
    {
        NON_COPYABLE(GameBrowserScenarioConfigInfo);
        typedef eastl::list<GameBrowserSubSessionConfigInfo*> SessionConfigList;

    public:
        GameBrowserScenarioConfigInfo(const ScenarioName& scenarioName, const GameBrowser* manager);
        ~GameBrowserScenarioConfigInfo();

        void deleteIfUnreferenced(bool clearRefs = false);
        bool configure(const GameBrowserScenariosConfig& config);
        const ScenarioName& getScenarioName() const { return mScenarioName; }
        const GameBrowserScenariosConfig* getOverallConfig() const { return mOverallConfig; }
        const GameBrowserScenarioConfig* getScenarioConfig() const { return mScenarioConfig; }
        SessionConfigList getScenarioSessionConfigList() { return mScenarioSessionConfigList; }

    private:
        ScenarioName mScenarioName;
        const GameBrowser* mScenarioManager;
        const GameBrowserScenariosConfig* mOverallConfig;
        const GameBrowserScenarioConfig* mScenarioConfig;
        SessionConfigList mScenarioSessionConfigList;
        void clearSessionConfigs();
    };

    class GameBrowserSubSessionConfigInfo
    {
        NON_COPYABLE(GameBrowserSubSessionConfigInfo);

    public:
        GameBrowserSubSessionConfigInfo(const SubSessionName& subSessionName,
            const GameBrowserScenarioConfigInfo* owner);
        ~GameBrowserSubSessionConfigInfo() {};

        bool configure(const GameBrowserScenariosConfig& config);
        bool addRule(const ScenarioRuleAttributes& ruleAttributes);
        const ScenarioName& getScenarioName() const { return mOwner->getScenarioName(); }
        SubSessionName getSubsessionName() { return mSubSessionName; }
        ScenarioAttributeMapping& getScenarioAttributeMapping() { return mScenarioAttributeMapping; }

    private:
        SubSessionName mSubSessionName;
        const GameBrowserScenarioConfigInfo* mOwner;

        const GameBrowserScenariosConfig* mOverallConfig;
        const GameBrowerSubSessionConfig* mSubSessionConfig;
        ScenarioAttributeMapping mScenarioAttributeMapping;

    };

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAMEBROWSER_H
