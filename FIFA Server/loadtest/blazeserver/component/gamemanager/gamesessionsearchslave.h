/*! ************************************************************************************************/
/*!
    \file gamesessionsearchslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_SESSION_SEARCH_SLAVE_H
#define BLAZE_GAMEMANAGER_GAME_SESSION_SEARCH_SLAVE_H

#include "EASTL/intrusive_ptr.h"
#include "gamemanager/gamesession.h"
#include "gamemanager/matchmaker/matchmakinggameinfocache.h"
#include "gamemanager/playerrosterslave.h"
#include "gamemanager/tdf/matchmaking_properties_config_server.h"



namespace Blaze
{
namespace Search
{
    class GameSessionSearchSlave; // for MmSessionGameIntrusiveNode::mGameSessionSlave
    class GameList;
}
namespace Matchmaker
{
    class MatchmakingGameInfoCache;
}
namespace GameManager
{
    class UserSessionInfo;

    // TODO: consolidate with define in reteTypes
#define INTRUSIVE_NODE_POOL_NEW_DELETE_OVERLOAD() public:\
    /* Node Pool New */ \
    void * operator new (size_t size, NodePoolAllocator& pool) { return pool.allocate(size); } \
    /* Node Pool Delete */ \
    void operator delete(void* ptr, NodePoolAllocator& pool) { if(ptr) pool.deallocate(ptr); } \
private: \
    /* don't allow WME's to be created except by the node pool. */ \
    void * operator new (size_t size) { EA_FAIL(); return ::operator new(size); } \
    void operator delete(void* ptr) { EA_FAIL(); ::operator delete(ptr); }


    /*! ************************************************************************************************/
    /*! \brief Intrusive node with fit scores and links gamebrowser lists with production node games
    *************************************************************************************************/
    struct GbListGameIntrusiveNode : public eastl::intrusive_list_node
    {
        GbListGameIntrusiveNode(Search::GameSessionSearchSlave& gameSession, Search::GameList& gbList);
        ~GbListGameIntrusiveNode();

        Search::GameSessionSearchSlave& mGameSessionSlave;
        Search::GameList& mGameBrowserList;
        Rete::ProductionInfo* mPInfo;
        FitScore mNonReteFitScore;
        FitScore mReteFitScore;
        FitScore mPostReteFitScore;
        FitScore getTotalFitScore() const { return mReteFitScore + mPostReteFitScore + mNonReteFitScore; }
        GameId getGameId() const;
        NON_COPYABLE(GbListGameIntrusiveNode);
        INTRUSIVE_NODE_POOL_NEW_DELETE_OVERLOAD();
    };
       
    typedef GbListGameIntrusiveNode GbListIntrusiveListNode;
    typedef eastl::intrusive_list<GbListIntrusiveListNode> GbListIntrusiveList; 
    typedef eastl::hash_map<Blaze::GameManager::GameId, GbListGameIntrusiveNode*> GameIntrusiveMap;

    namespace Matchmaker
    {
        class MatchmakingSessionSlave; // for MmSessionGameIntrusiveNode::mMatchmakingSessionSlave
    }
    struct MmSessionIntrusiveListNode : eastl::intrusive_list_node {};
    typedef eastl::intrusive_list<MmSessionIntrusiveListNode> MmSessionIntrusiveList;

    // Unsorted heap of all matches excluding the top N
    struct ProductionMatchHeapNode : eastl::intrusive_list_node {};
    typedef eastl::intrusive_list<ProductionMatchHeapNode> ProductionMatchHeap;

    // Duplicate typedef for static remove
    struct ProductionTopMatchHeapNode : eastl::intrusive_list_node {};
    typedef eastl::intrusive_list<ProductionTopMatchHeapNode> ProductionTopMatchHeap;

    /*! ************************************************************************************************/
    /*! \brief Intrusive node with fit scores and links matchmaker sessions with production node games
    *************************************************************************************************/
    struct MmSessionGameIntrusiveNode : 
        public MmSessionIntrusiveListNode, public ProductionMatchHeapNode, public ProductionTopMatchHeapNode
    {
        MmSessionGameIntrusiveNode(Search::GameSessionSearchSlave& gameSession, Blaze::GameManager::Matchmaker::MatchmakingSessionSlave& mmSession);
        ~MmSessionGameIntrusiveNode();

        Search::GameSessionSearchSlave& mGameSessionSlave;
        Matchmaker::MatchmakingSessionSlave& mMatchmakingSessionSlave;
        FitScore mNonReteFitScore;
        FitScore mPostReteFitScore;
        FitScore mReteFitScore;
        DebugEvaluationResult mOverallResult;
        DebugRuleResultMap mReteDebugResultMap;
        DebugRuleResultMap mNonReteDebugResultMap;
        FitScore getTotalFitScore() const { return mReteFitScore + mPostReteFitScore + mNonReteFitScore; }
        bool mIsCurrentlyMatched; // prevents dupes in MatchmakingSessionSlave::mProductionMatchHeap
        bool mIsCurrentlyHeaped;
        bool mIsGameReferenced;
        bool mIsToBeSentToMatchmakerSlave;
        GameId getGameId() const;
        NON_COPYABLE(MmSessionGameIntrusiveNode);
        INTRUSIVE_NODE_POOL_NEW_DELETE_OVERLOAD();
    };
    
    typedef eastl::hash_map<Blaze::GameManager::GameId, MmSessionGameIntrusiveNode*> MMGameIntrusiveMap;
}


namespace Search
{
    class SearchSlaveImpl;

    class GameSessionSearchSlave : public GameManager::GameSession
    {
        NON_COPYABLE(GameSessionSearchSlave);
    public:
        GameSessionSearchSlave(Search::SearchSlaveImpl &slaveComponentImpl, GameManager::ReplicatedGameDataServer& replicatedGameSession);
        ~GameSessionSearchSlave() override;

        void addReference(GameManager::MmSessionIntrusiveListNode& n) { mMmSessionIntrusiveList.push_back(n); mMmSessionIntrusiveListCount++; }
        GameManager::MmSessionIntrusiveList::size_type getMmSessionIntrusiveListSize() { return mMmSessionIntrusiveListCount; }
        void decrementMmSessionIntrusiveListSize() { mMmSessionIntrusiveListCount--; }
        GameManager::MmSessionIntrusiveList &getMmSessionIntrusiveList() { return mMmSessionIntrusiveList; }

        void addReference(GameManager::GbListIntrusiveListNode& n) { mGbListIntrusiveList.push_back(n); mGbListIntrusiveListCount++; }
        GameManager::GbListIntrusiveList::size_type getGbListIntrusiveListSize() { return mGbListIntrusiveListCount; }
        void decrementGbListIntrusiveListSize() { mGbListIntrusiveListCount--; }
        GameManager::GbListIntrusiveList &getGbListIntrusiveList() { return mGbListIntrusiveList; }

        const GameManager::PlayerRosterSlave *getPlayerRoster() const override { return &mPlayerRoster; }
        const GameManager::SingleGroupMatchIdList& getSingleGroupMatchIds() const { return mGameData->getSingleGroupMatchIds(); }
        const char8_t* getGameMode() const override;
        const char8_t* getPINGameModeType() const override;
        const char8_t* getPINGameType() const override;
        const char8_t* getPINGameMap() const override;

        StorageRecordVersion getGameRecordVersion();

        void setInRete(bool inRete) { mIsInRete = inRete; }
        bool isInRete() const { return mIsInRete; }

        bool isJoinableForUser(const GameManager::UserSessionInfo& userSessionInfo, bool ignoreEntryCriteria, const char8_t* roleName = nullptr, bool checkAlreadyInRoster = false) const;

        // stores game values that are used for RETE and diagnostics updates
        GameManager::Matchmaker::MatchmakingGameInfoCache* getMatchmakingGameInfoCache() { return &mMatchmakingCache; }
        const GameManager::Matchmaker::MatchmakingGameInfoCache* getMatchmakingGameInfoCache() const { return &mMatchmakingCache; }

        void updateCachedGameValues( const GameManager::Matchmaker::RuleDefinitionCollection& ruleDefinitionCollection, 
            const GameManager::Matchmaker::MatchmakingSessionList* memberSessions = nullptr);

        void updateReplicatedGameData(GameManager::ReplicatedGameDataServer& gameData);

        void discardPrevSnapshot();
        const GameManager::ReplicatedGameDataServer* getPrevSnapshot() const { return mPrevSnapshot.get(); }

        void updateGameProperties();
        const GameManager::PropertyDataSources& getGameProperties() const { return mGameProperties; }

        // In theory, we could just generate this from the PrevSnapshot, candidate for removal when Legacy MM is gone(ETA: 2030).
        GameManager::PropertyNameMap& getPrevProperties() const { return mPrevGamePropertyMap; }

        Search::SearchSlaveImpl& getSearchSlave() const { return mSearchSlaveImpl; }

    protected:
        friend class SearchSlaveImpl;

        void onUpsert(GameManager::ReplicatedGamePlayerServer& player);
        void onErase(GameManager::ReplicatedGamePlayerServer& player);

    private:
        // Helpers when handling our two player map subscriber callbacks.
        void insertPlayer(const GameManager::PlayerInfo &joiningPlayer);
        void updatePlayer(const GameManager::PlayerInfo &player);
        void erasePlayer(const GameManager::PlayerInfo& departingPlayer);

        void updateRete();

    private:
        uint32_t mRefCount;
        Search::SearchSlaveImpl &mSearchSlaveImpl;

        GameManager::Matchmaker::MatchmakingGameInfoCache mMatchmakingCache;
        GameManager::PropertyDataSources mGameProperties;
        mutable GameManager::PropertyNameMap mPrevGamePropertyMap;   // Properties as they were entered into RETE.  mutable because it's changed by the PropertyRuleDefinition.
        GameManager::PlayerRosterSlave mPlayerRoster;

        bool mIsInRete;
        GameManager::ReplicatedGameDataServerPtr mPrevSnapshot;

        GameManager::MmSessionIntrusiveList::size_type mMmSessionIntrusiveListCount;
        GameManager::MmSessionIntrusiveList mMmSessionIntrusiveList;
        GameManager::GbListIntrusiveList::size_type mGbListIntrusiveListCount;
        GameManager::GbListIntrusiveList mGbListIntrusiveList;
        
        friend void intrusive_ptr_add_ref(GameSessionSearchSlave* ptr);
        friend void intrusive_ptr_release(GameSessionSearchSlave* ptr);
    };

    void intrusive_ptr_add_ref(GameSessionSearchSlave* ptr);
    void intrusive_ptr_release(GameSessionSearchSlave* ptr);

} //Search
} // Blaze

#endif // BLAZE_GAMEMANAGER_GAME_SESSION_SEARCH_SLAVE_H
