/*! ************************************************************************************************/
/*!
    \file gamebrowserlist.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEBROWSER_GAME_LIST_H
#define BLAZE_GAMEBROWSER_GAME_LIST_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/component/gamemanager/tdf/gamebrowser.h"
#include "BlazeSDK/gamemanager/gamebrowsergame.h"
#include "BlazeSDK/blaze_eastl/vector.h"
#include "BlazeSDK/blaze_eastl/vector_map.h"
#include "BlazeSDK/blaze_eastl/slist.h"

namespace Blaze
{

namespace GameManager
{
    class GameManagerAPI;

    /*! ************************************************************************************************/
    /*! \brief A GameBrowserList contains a collection of games that satisfy a particular matchmaking criteria.

            GameBrowserLists are created from GameManagerAPI::createGameBrowserList, and are updated
                asynchronously by the blazeServer as games are found that match the list's criteria.

            List updates are dispatched via GameManagerAPIListener::onGameBrowserListUpdated.

            We support two types of list: subscription and snapshot.
            \li Snapshot lists are built at a single point in time; once the list is finished, no further updates are sent.
            \li Subscription lists continue to evaluate games and update themselves until the list is destroyed.

            You can specify a list's max size during creation (or make the size unlimited).
    ***************************************************************************************************/
    class BLAZESDK_API GameBrowserList
    {
        //! \cond INTERNAL_DOCS
        NON_COPYABLE(GameBrowserList);
        friend class GameManagerAPI;
        friend class GameBrowserGame; // so players can be added to the list's removedPlayerList
        friend class MemPool<GameBrowserList>;
        //! \endcond

    public:

        /*! ************************************************************************************************/
        /*! \brief GameBrowserLists have two 'types': snapshot and subscription. 

                Snapshot GameBrowserLists are a list build at a single point in time; once completed, 
                    the snapshot lists don't update themselves any further.

                Subscription GameBrowserLists will continue to evaluate games and update themselves 
                    until the list is destroyed.
        ***************************************************************************************************/
        enum ListType
        {
            LIST_TYPE_SNAPSHOT,        //!< Snapshot GameBrowserLists are a list build at a single point in time; once completed, the snapshot lists don't update themselves any further.
            LIST_TYPE_SUBSCRIPTION    //!< Subscription GameBrowserLists will continue to evaluate games and update themselves until the list is destroyed.
        };

        /*! ************************************************************************************************/
        /*! \brief Contains the params needed to create a GameBrowserList: the list's type, capacity, configName, and criteria.
        ***************************************************************************************************/
        struct BLAZESDK_API GameBrowserListParametersBase
        {
            /*! ************************************************************************************************/
            /*! \brief default params are: Snapshot List, 50 game capacity, default game data config, default criteria (all games)
            
                \param[in] memGroupId   mem group to be used by this class to allocate memory 
            ***************************************************************************************************/
            GameBrowserListParametersBase(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

            GameBrowserList::ListType mListType; //!< The 'type' of this list.  Subscription lists updates themselves until the list is destroyed; snapshot lists initialize themselves once.
            uint32_t mListCapacity; //!< The max number of matching games this list will contain (or GAME_LIST_CAPACITY_UNLIMITED)
            GameBrowserListConfigName mListConfigName; //!< Each list can specify a named list data configuration, which details which game/player data fields are downloaded by default for a game.  These configurations are setup in the blazeServer's game browser config file; the default config is named "default".
            MatchmakingCriteriaData mListCriteria; //!< The matchmaking criteria (rules and desired values) to build this list with.
            bool mIgnoreGameEntryCriteria; //!< If true, ignore the game entry criteria for matching games (returns games you can't join due to the game entry criteria). Default value is false.
        };

        /*! ************************************************************************************************/
        /*! \brief Older version of GameBrowserListParameters which contains data now held in PlayerJoinData. Maintained for backwards compatibility. 
        ***************************************************************************************************/
        struct BLAZESDK_API GameBrowserListParameters : public GameBrowserListParametersBase
        {
            GameBrowserListParameters(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

            TeamId mTeamId; // If specified, this list will only match games containing the specified team.
            RoleMap mRoleMap; // If non-empty, this list will only match games that allow the specified roles.
        };

        /*! ************************************************************************************************/
        /*! \brief the set of games that match this list's criteria (UNSORTED).  A simple eastl::vector subclass.
        
            Note: the included GameBrowserGame is non-const, since you might want to sort the game's player roster.
        ***************************************************************************************************/
        typedef Blaze::vector<GameBrowserGame *> GameBrowserGameVector;

        typedef Blaze::vector<const GameBrowserGame *> GameBrowserGameConstVector;

        /*! ************************************************************************************************/
        /*! \brief destroys this list, freeing all associated GameBrowserGames.
        ***************************************************************************************************/
        void destroy();

        /*! ************************************************************************************************/
        /*! \brief return this list's id.
            \return this list's id (unique identifier)
        ***************************************************************************************************/
        GameBrowserListId getListId() const { return mClientListId; }

        /*! ************************************************************************************************/
        /*! \brief return the list's type
            \return return the list's type
        ***************************************************************************************************/
        ListType getListType() const { return mListType; }

        /*! ************************************************************************************************/
        /*! \brief returns true if this list is finished getting async updates. Note: subscription lists only
            return true if the list has been destroyed by the server.
            \return returns true if this list is finished getting async updates.  Note: subscription lists only
            return true if the list has been destroyed by the server.
        ***************************************************************************************************/
        bool isListFinishedUpdating() const { return mListUpdatesFinished; }

        /*! ************************************************************************************************/
        /*! \brief return the max size (capacity) of this list.  Note: GAME_LIST_CAPACITY_UNLIMITED is max uint32
            \return return the max size (capacity) of this list.
        ***************************************************************************************************/
        uint32_t getListCapacity() const { return mListCapacity; }

        /*! ************************************************************************************************/
        /*! \brief returns a specified GameBrowserGame given its id (or nullptr, if the game isn't in this list)
        
            \param[in] gameId The id of a specific game to get.
            \return returns a specified GameBrowserGame given its id (or nullptr, if the game isn't in this list)
        ***************************************************************************************************/
        const GameBrowserGame* getGameById(GameId gameId) const;

        /*! ************************************************************************************************/
        /*! \brief Return the unsorted set of games in this list.

            Each list exposes the vector of games that make up the list, allowing titles to sort the list
            when and how they see fit (typically using one of the eastl sort algorithms in EASTL\sort.h).

            We provide a default sort comparison struct (sort by highest fitScore, then gameName, then gameId).
            See DefaultGameSortCompare.

            Note: you must use a sort comparison struct when sorting the game vector, otherwise you'll be
                sorting games based off of their location in memory (sorting by pointer value).
        
            \return Return the unsorted set of games in this list.
        ***************************************************************************************************/
        GameBrowserGameVector& getGameVector() { return mDefaultGameView; }

        /*! ************************************************************************************************/
        /*! \brief Fill a caller supplied vector for games a user is it. 

            The config for this list must be set to download roster

            \return the passed-in games
        ***************************************************************************************************/
        GameBrowserGameConstVector& getGameVectorByUser(BlazeId user, GameBrowserGameConstVector& games) const;

        /*! ************************************************************************************************/
        /*! \brief We provide a default sort comparison struct for the GameBrowserGameVector.  
                We sort by: FitScore (descending), gameName (ascending, case-insensitive), gameId (ascending).

            See EASTL/sort.h, and the eastl FAQ for more info on sorting (in packages/eastl/version/doc)
        ***************************************************************************************************/
        struct BLAZESDK_API DefaultGameSortCompare
        {
            
            /*! ************************************************************************************************/
            /*! \brief return true if game a should come before game b (eastl sort comparison struct).
                    We sort by: FitScore (descending), gameName (ascending, case-insensitive), gameId (ascending).
            
                \param[in] a the first item to compare
                \param[in] b the second item to compare
                \return true if a should come before b
            ***************************************************************************************************/
            bool operator()(const GameBrowserGame *a, const GameBrowserGame *b) const;
        };

        /*! ************************************************************************************************/
        /*! \brief Return the max possible fitScore for this list's MatchmakingCriteria.
            \return returns the max possible fitScore as reported by the server.
        ***************************************************************************************************/
        FitScore getMaxPossibleFitScore() const { return mMaxPossibleFitScore; }

        /*! ************************************************************************************************/
        /*! \brief helper to return the GameManagerAPI associated with this list
            \return the GameManagerAPI associated with this list
        ***************************************************************************************************/
        GameManagerAPI* getGameManagerAPI() const { return mGameManagerAPI; }

        /*! ************************************************************************************************/
        /*! \brief return the number of games to be downloaded.
            \return the Total number of games to be downloaded (by a SNAPSHOT List) from the server.
        ***************************************************************************************************/
        uint32_t getNumberOfGamesToBeDownloaded() const { return mNumberOfGamesToBeDownloaded; }

        /*! ************************************************************************************************/
        /*! \brief return the last time this list received an update from the server
            \return the last time this list received an update from the server
        ***************************************************************************************************/
        uint32_t getLastUpdateTime() const { return mLastUpdateTimeMS; }

        /*! ************************************************************************************************/
        /*! \brief return the max time interval between updates from the server
            \return the max time interval between updates from the server
        ***************************************************************************************************/
        const TimeValue& getMaxUpdateInterval() const { return mMaxUpdateInterval; }

    private:

        GameBrowserList(GameManagerAPI *gameManagerApi, GameBrowserListId clientListId, ListType listType, uint32_t listCapacity, GetGameListRequestPtr getGameListRequest, UserGroupId userGroupId,
            const GetGameListResponse *getGameListResponse, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP, GetGameListScenarioRequestPtr getGameListScenarioRequest = nullptr);

        ~GameBrowserList();

        void resetSubscription(GameBrowserListId serverListId);
        void sendFinalUpdate();

        void onNotifyGameListUpdate(const NotifyGameListUpdate *notifyGameListUpdate);
        inline GameBrowserGame* removeGame(GameId gameId);
        inline GameBrowserGame* updateGame(const GameBrowserMatchData *gameBrowserGameData);
        void queuePlayerForDelete(GameBrowserPlayer *player) { mRemovedPlayerList.push_front(player); }

    private:
        GameManagerAPI *mGameManagerAPI; // we need the GMApi so we can init user (via the UserManager), as well as for dispatching
        MemPool<GameBrowserGame> mGameMemoryPool;
        GameBrowserListId mClientListId;         // A client-only identifier, never changes
        GameBrowserListId mServerListId; // The actual id of the game browser list on the Blaze server, may change over the lifetime of this object
        GetGameListRequestPtr mGetGameListRequest;
        GetGameListScenarioRequestPtr mGetGameListScenarioRequest;
        UserGroupId mUserGroupId;
        ListType mListType;
        uint32_t mListCapacity;
        FitScore mMaxPossibleFitScore;
        bool mListUpdatesFinished;
        uint32_t mLastUpdateTimeMS;
        TimeValue mMaxUpdateInterval;

        typedef Blaze::vector_map<GameId, GameBrowserGame*> GameBrowserGameMap;
        GameBrowserGameMap mGameBrowserGameMap; // 'owns' the GameBrowserGame memory, map for quick access
        GameBrowserGameVector mDefaultGameView; // unsorted list of games (title code can access/sort this list as they see fit)

        // we maintain a list of removed GameBrowserPlayers, so that we can persist player pointers until
        //   after we update the title code (giving the game team a chance to clean up any cached pointers).
        typedef Blaze::slist<GameBrowserPlayer*> RemovedPlayerList;
        RemovedPlayerList mRemovedPlayerList;
        MemoryGroupId mMemGroup;   //!< memory group to be used by this class, initialized with parameter passed to constructor
        uint32_t mNumberOfGamesToBeDownloaded;
    };

} // namespace GameManager
} // namespace Blaze
#endif // BLAZE_GAMEBROWSER_GAME_LIST_H
