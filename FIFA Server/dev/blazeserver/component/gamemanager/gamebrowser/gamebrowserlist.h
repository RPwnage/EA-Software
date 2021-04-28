/*! ************************************************************************************************/
/*!
    \file gamebrowserlist.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_H
#define BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_H

#include "gamemanager/gamebrowser/gamebrowser.h"
#include "gamemanager/gamebrowser/gamebrowserlistowner.h"
namespace Blaze
{
   
namespace GameManager
{
     
    /*! ************************************************************************************************/
    /*! \brief Each GameBrowserList is identified by an id, and contains a unique set of matchmaking criteria.

        From the client's perspective, each list has a max size (can be limited to return only the best X matches).
        However, each list actually tracks all matching games (regardless of the list's capacity).  The games
        that we show to the client are from the 'visible' container and the rest of the matching games are kept in the
        reserve container.  This lets us efficiently find the best reserved game and make it visible when a visible game
        is destroyed (or it stops being a match).
    *************************************************************************************************/
    class GameBrowserList
    {
        NON_COPYABLE(GameBrowserList);

        // Additional time (15 seconds) we add to our configured max update interval in order to tell the client
        // how long they should wait before giving up on a game browser list.  We guarantee to send clients updates
        // at least as frequently as our configured max update interval, but due to delays in game browser idle as well
        // as network delays, the actual interval we tell the client needs to be longer so they don't jump the gun
        static const int64_t UPDATE_GRACE_PERIOD = 15 * 1000 * 1000;

    public:

        GameBrowserList(GameBrowser& gameBrowser, GameBrowserListId gameBrowserListId, ListType listType, 
            const eastl::string& listConfigName, size_t visibleListCapacity, TimeValue maxUpdateInterval,
            IGameBrowserListOwner& mOwner);
        virtual ~GameBrowserList();

        /*! ************************************************************************************************/
        /*! \brief returns the id for this list
        *************************************************************************************************/
        GameBrowserListId getListId() const { return mListId; }

        /*! ************************************************************************************************/
        /*! \brief accessors to determine list ownership info.
        *************************************************************************************************/
        IGameBrowserListOwner& getOwner() const { return mOwner; }
        GameBrowserListOwnerType getOwnerType() const { return mOwner.getOwnerType(); }

        /*! ************************************************************************************************/
        /*! \brief returns the ListType for this list (snapshot or subscription)
        *************************************************************************************************/
        ListType getListType() const { return mListType; }

        /*! ************************************************************************************************/
        /*! \brief returns if the list should be processed by Game Browser's idle
        *************************************************************************************************/
        bool getHiddenFromIdle() const { return mHiddenFromIdle; }

        /*! ************************************************************************************************/
        /*! \brief sets if the list should be processed by Game Browser's idle
        *************************************************************************************************/
        void setHiddenFromIdle(bool value) { mHiddenFromIdle = value; }

        const eastl::string& getListConfigName() { return mListConfigName; }

        virtual bool isUserSetList() const { return false; }

        Search::CreateGameListRequest& getCreateGameListRequest() { return mCreateGameListRequest; }

        virtual void removeGame(GameId gameId);
        virtual void removeAllGames();
        virtual void removeSearchInstance(InstanceId searchInstanceId);

        size_t getMaxVisibleGameCount() const { return mMaxVisibleGames; }
        void setActualResultSetSize(size_t actualResultSetSize) { mActualResultSetSize = actualResultSetSize; }

        // called via GameBrowser class
        bool sendListUpdateNotifications(uint32_t maxGameUpdatesPerNotification, uint32_t maxListUpdateNotifications, GameBrowser::NotifyGameListReason notifyReason, uint32_t msgNum);
        BlazeRpcError fillSyncGameList(uint32_t maxGameListSize, GameBrowserMatchDataList& gameList);

        size_t getUpdatedGamesSize() const { return mUpdatedGames.size(); }

        /*! ************************************************************************************************/
        /*! \brief returns original get list rpc's msg num, for async notification cache's fetch by sequence
        *************************************************************************************************/
        uint32_t getListCreateMsgNum() const;

        //Methods called by gamebrowser when our list is updated
        virtual void onGameAdded(GameManager::GameId gameId, GameManager::FitScore fitScore, GameManager::GameBrowserGameDataPtr& gameData);
        virtual void onGameRemoved(GameManager::GameId gameId);
        virtual void onGameUpdated(GameManager::GameId gameId, GameManager::FitScore fitScore, GameManager::GameBrowserGameDataPtr& gameData);

        // synchronization functions
        void signalGameListUpdate();

        void setListOfHostingSearchSlaves(const InstanceIdList &list) { list.copyInto(mHostingSlaveInstanceIdList); }
        const InstanceIdList& getListOfHostingSearchSlaves() const { return mHostingSlaveInstanceIdList; }

        const TimeValue getMaxUpdateIntervalPlusGrace() const { return mMaxUpdateInterval + UPDATE_GRACE_PERIOD; }
    
    public:     
        virtual bool isFullResultSetReceived() const { return (mActualResultSetSize > 0 && mReceivedGamesCount >= eastl::min_alt(mActualResultSetSize, getMaxVisibleGameCount())); }
        void incrementReceivedGamesCount() { ++mReceivedGamesCount; }

    protected:
        void processMatchingGame(GameId gameId, FitScore fitScore, GameManager::GameBrowserGameDataPtr& gameData );
        void markGameAsRemoved(GameId gameId, bool isNew);
        void markGameAsUpdated(GameId gameId);


    protected:
        GameBrowser& mGameBrowser;
        GameBrowserListId mListId;
        ListType mListType;
        eastl::string mListConfigName;
        IGameBrowserListOwner& mOwner;
        size_t mActualResultSetSize;
        size_t mReceivedGamesCount;
    private:

        Search::CreateGameListRequest mCreateGameListRequest;

        //A set of game id's corresponding to game's that have been removed from the visible session
        // container since the last idle.
        typedef eastl::hash_set<GameId> GameIdSet;
        GameIdSet mRemovedGames;
        GameIdSet mUpdatedGames;


        struct Match
        {
            Match() : score(0), gameId(INVALID_GAME_ID), isNew(false) {}
            Match(FitScore _score, GameId _gameId, GameBrowserGameDataPtr& _data, bool _new) : score(_score), gameId(_gameId), data(_data), isNew(_new) {}
            FitScore score;
            GameId gameId;
            GameBrowserGameDataPtr data;
            bool isNew;
        };

        typedef eastl::multimap<FitScore, Match, eastl::greater<FitScore> > MatchingGameSessionMapDescend;

        struct MatchingGameSessionInfo
        {
            bool mRanked;
            MatchingGameSessionMapDescend::iterator mUnrankedGameSessionMapIterator;
            MatchingGameSessionMapDescend::iterator mMatchingGameSessionMapIterator;
        };
        typedef eastl::hash_map<GameId, MatchingGameSessionInfo> MatchingGameSessionMapInv;

        // controls if the list should be handled by Game Browser's Idle function
        bool mHiddenFromIdle;

        //Used to track someone waiting on this list (snapshot updates)
        Fiber::EventHandle mUpdateEventHandle;
       
        //List of all the slaves our list has created queries on.
        InstanceIdList mHostingSlaveInstanceIdList;

        TimeValue mLastUpdateTime;
        TimeValue mMaxUpdateInterval;

        MatchingGameSessionMapDescend mUnrankedGameSessionMap;
        MatchingGameSessionMapDescend mTopRankedMatchingGameSessionMap;
        MatchingGameSessionMapInv mMatchingGameSessionMapInv;
        size_t mMaxVisibleGames;        
    };



    class UserSetGameBrowserList : public GameBrowserList
    {
        NON_COPYABLE(UserSetGameBrowserList);

    public:
        UserSetGameBrowserList(GameBrowser& gameBrowser, GameBrowserListId gameBrowserListId,
            const eastl::string& listConfigName, const EA::TDF::ObjectId& userSetId,
            TimeValue maxUpdateInterval, IGameBrowserListOwner& owner)
            : GameBrowserList(gameBrowser, gameBrowserListId, GAME_BROWSER_LIST_SUBSCRIPTION, listConfigName, gameBrowser.getUserSetListCapacity(), maxUpdateInterval, owner),
            mUserSetId(userSetId) {}
        ~UserSetGameBrowserList() override {}

        bool isUserSetList() const override { return true; }

        const EA::TDF::ObjectId& getUserSetId() const { return mUserSetId; }

    protected:    
        EA::TDF::ObjectId mUserSetId;

    };
} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEMANAGER_GAME_BROWSER_LIST_H
