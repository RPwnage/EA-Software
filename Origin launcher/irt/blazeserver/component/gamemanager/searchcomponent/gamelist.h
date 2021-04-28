/*! ************************************************************************************************/
/*!
    \file gamelist.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_SEARCH_GAME_LIST_H
#define BLAZE_SEARCH_GAME_LIST_H

#include "gamemanager/gamebrowser/gamebrowser.h"
#include "gamemanager/rete/legacyproductionlistener.h"
#include "gamemanager/searchcomponent/gamematchcontainer.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"

namespace Blaze
{

namespace GameManager
{
namespace Matchmaker
{
    struct MatchmakingSupplementalData;
}
}

namespace Search
{
    class SearchSlaveImpl;
    struct SearchIdleMetrics;

    /*! ************************************************************************************************/
    /*! \brief Game Lists created on Search component link into RETE tree and obtain list content
    *************************************************************************************************/
    class GameListBase
    {
        NON_COPYABLE(GameListBase);

    public:
        GameListBase(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId mOriginalInstanceSessionId, GameManager::ListType listType, UserSessionId ownerSessionId);
        virtual ~GameListBase() {}

        size_t getUpdatedGamesSize() const { return mUpdatedGames.size() + mNewGames.size() + mRemovedGames.size(); }
        
        virtual void cleanup() = 0;

        GameManager::GameBrowserListId getListId() const { return mGameBrowserListId; }
        GameManager::ListType getListType() const { return mListType; }
        SlaveSessionId getOriginalInstanceSessionId() const { return mOriginalInstanceSessionId; }
        
        bool hasUpdates() const;

        typedef eastl::hash_set<Blaze::GameManager::GameId> EncodedGamesSet;
        typedef eastl::hash_map<eastl::string, EncodedGamesSet> EncodedGamesByListConfig;
        typedef eastl::map<InstanceId, EncodedGamesByListConfig> EncodedGamesPerInstanceId;
        void getUpdates(NotifyGameListUpdate& instanceUpdate, EncodedGamesByListConfig& encodedGamesByListConfig, SearchIdleMetrics& searchIdleMetrics);
                
        UserSessionId getOwnerUserSessionId() const { return mOwnerSessionId; }

        virtual bool isUserList() const { return false; }
        virtual const GameManager::PropertyNameList* getPropertyNameList() const { return nullptr; }

    protected:
        void markGameAsUpdated(GameManager::GameId gameId, GameManager::FitScore fitScore, bool newGame);
        void markGameAsRemoved(GameManager::GameId gameId);

    protected:
        SearchSlaveImpl &mSearchSlave;
        
        GameManager::GameBrowserListId mGameBrowserListId;
        GameManager::ListType mListType;

        UserSessionId mOwnerSessionId;

        eastl::string mListConfigName;

    protected:
        // set of game ids for games in this list whose status in the list changed
        // since last idle
        typedef eastl::set<GameManager::GameId> GameIdSet;
        typedef eastl::map<GameManager::GameId, GameManager::FitScore> FitScoreByGameId;
        GameIdSet mRemovedGames;
        FitScoreByGameId mNewGames;
        FitScoreByGameId mUpdatedGames;

        const SlaveSessionId mOriginalInstanceSessionId;
    };


    /*! ************************************************************************************************/
    /*! \brief Game Lists created on Search component link into RETE tree and obtain list content
    *************************************************************************************************/
    class GameList : public GameListBase, public GameManager::Rete::LegacyProductionListener
    {
        NON_COPYABLE(GameList);

    public:
        GameList(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId mOriginalInstanceSessionId, GameManager::ListType listType, UserSessionId ownerSessionId, bool filterOnly);
        ~GameList() override;

        Blaze::BlazeRpcError initialize(const GameManager::UserSessionInfo& ownerInfo, const GameManager::GetGameListRequest& request, bool evaluateGameProtocolVersionString, GameManager::MatchmakingCriteriaError &err,
                                        uint16_t maxPlayerCapacity, const char8_t* preferredPingSite, GameNetworkTopology netTopology, const GameManager::MatchmakingFilterCriteriaMap& filterMap);
        void cleanup() override;

        virtual void destroyIntrusiveNode(Blaze::GameManager::GbListGameIntrusiveNode& node) = 0;        
        virtual void updateGame(const GameSessionSearchSlave& gameSession) = 0;
        virtual void removeGame(GameManager::GameId gameId) = 0;
        

             // Listens to the Tokens coming out of the Rete Network
        GameManager::Rete::ProductionId getProductionId() const override { return mGameBrowserListId; }
        const GameManager::Rete::ReteRuleContainer& getReteRules() const override { return mCriteria.getReteRules(); }
        void onProductionNodeHasTokens(GameManager::Rete::ProductionNode& pNode, GameManager::Rete::ProductionInfo& pInfo) override;
        void onProductionNodeDepletedTokens(GameManager::Rete::ProductionNode& pNode) override;
        bool onTokenAdded(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override = 0;
        void onTokenRemoved(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override = 0;
        void onTokenUpdated(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override = 0;
        
        bool notifyOnTokenAdded(GameManager::Rete::ProductionInfo& pInfo) const override = 0;
        bool notifyOnTokenRemoved(GameManager::Rete::ProductionInfo& pInfo) override = 0;
        void initialSearchComplete() override;

        virtual void processMatchingGame(const GameSessionSearchSlave& game, GameManager::FitScore fitScore, GameManager::Rete::ProductionInfo& pInfo) = 0;
  
        GameManager::FitScore calcMaxPossibleFitScore() const { return mCriteria.calcMaxPossibleFitScore(); }
       
        const GameManager::PropertyNameList* getPropertyNameList() const override { return &mPropertyNameList; }
     
    protected:
        virtual bool isListAtCapacity() const { return mVisibleGameSessionContainer.size() == mVisibleListCapacity; }

    protected:

        class ProductionInfoByFitScore
        {
            NON_COPYABLE(ProductionInfoByFitScore)
        public:
            typedef eastl::list<GameManager::Rete::ProductionInfo*> ProductionInfoList;
            typedef ProductionInfoList::iterator iterator;

        public:
            ProductionInfoByFitScore() {}
            virtual ~ProductionInfoByFitScore() {}

            void insert(GameManager::Rete::ProductionInfo& pInfo);
            iterator begin() { return mProductionInfoList.begin(); }
            iterator end() { return mProductionInfoList.end(); }

        private:
            ProductionInfoList mProductionInfoList;
        };

    protected:
        Search::GameMatchContainer mVisibleGameSessionContainer; 
        uint32_t mVisibleListCapacity;        

        GameManager::MatchmakingAsyncStatus mMatchmakingAsyncStatus;
        GameManager::Matchmaker::MatchmakingCriteria mCriteria;

        bool mIgnoreGameEntryCriteria;
        bool mIgnoreGameJoinMethod;
        GameManager::GameStateList mGameStateWhitelist;
        GameManager::GameTypeList mGameTypeList;

        ProductionInfoByFitScore mProductionInfoByFitscore;
        ProductionInfoByFitScore::iterator mWorstPInfoItr;

        bool mInitialSearchComplete;

        GameManager::UserSessionInfo mOwnerSessionInfo;
        GameManager::UserSessionInfoList mMembersUserSessionInfo;//for GB lists using UEDRules (may only contain owner).

        GameManager::PropertyNameList mPropertyNameList;     // List of Properties to lookup for Packer response. 
    };

} // namespace Search
} // namespace Blaze
#endif // BLAZE_SEARCH_GAME_LIST_H
