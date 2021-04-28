/*! ************************************************************************************************/
/*!
    \file gamebrowserlist.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/gamemanager/gamebrowserlist.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief default params are: Snapshot List, 50 game capacity, default game data config, default criteria (all games)
    
        \param[in] memGroupId mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    GameBrowserList::GameBrowserListParametersBase::GameBrowserListParametersBase(MemoryGroupId memGroupId)
        : mListType(GameBrowserList::LIST_TYPE_SNAPSHOT),
        mListCapacity(50),
        mListConfigName("default", memGroupId),
        mListCriteria(memGroupId),
        mIgnoreGameEntryCriteria(false)
    {
    }

    GameBrowserList::GameBrowserListParameters::GameBrowserListParameters(MemoryGroupId memGroupId)
        : GameBrowserListParametersBase(memGroupId),
        mTeamId(ANY_TEAM_ID),
        mRoleMap(memGroupId, MEM_NAME(memGroupId, "GameBrowserList::GameBrowserListParameters.mRoleMap"))
    {
    }

    /*! ************************************************************************************************/
    /*! \brief construct a list from a GetGameList RPC response.  Note: lists are empty after construction,
            and must be populated by async notifications (see onNotifyGameListUpdate).
    
        \param[in] gameManagerApi       the 'owning' gameManagerApi object
        \param[in] listType             subscription or snapshot
        \param[in] listCapacity         the max number of games to include in this list.  Note: GAME_LIST_CAPACITY_UNLIMITED is max uint32
        \param[in] getGameListResponse  the RPC response
        \param[in] memGroupId           mem group to be used by this class to allocate memory
    ***************************************************************************************************/
    GameBrowserList::GameBrowserList(GameManagerAPI *gameManagerApi, GameBrowserListId clientListId, ListType listType, uint32_t listCapacity, 
                                     GetGameListRequestPtr getGameListRequest, UserGroupId userGroupId,
                                     const GetGameListResponse *getGameListResponse, MemoryGroupId memGroupId, GetGameListScenarioRequestPtr getGameListScenarioRequest)
        : mGameManagerAPI(gameManagerApi),
          mGameMemoryPool(memGroupId),
          mClientListId(clientListId),
          mServerListId(getGameListResponse->getListId()),
          mGetGameListRequest(getGameListRequest),
          mGetGameListScenarioRequest(getGameListScenarioRequest),
          mUserGroupId(userGroupId),
          mListType(listType),
          mListCapacity(listCapacity),
          mMaxPossibleFitScore(getGameListResponse->getMaxPossibleFitScore()),
          mListUpdatesFinished(false),
          mLastUpdateTimeMS(0),
          mMaxUpdateInterval(getGameListResponse->getMaxUpdateInterval()),
          mGameBrowserGameMap(memGroupId, MEM_NAME(memGroupId, "GameBrowserList::mGameBrowserGameMap")),
          mDefaultGameView(memGroupId, MEM_NAME(memGroupId, "GameBrowserList::mDefaultGameView")),
          mRemovedPlayerList(memGroupId, MEM_NAME(memGroupId, "GameBrowserList::mRemovedPlayerList")),
          mMemGroup(memGroupId),
          mNumberOfGamesToBeDownloaded(getGameListResponse->getNumberOfGamesToBeDownloaded())
    {
        BlazeAssert(mGameManagerAPI != nullptr);

        uint32_t maxGameBrowserGames = getGameManagerAPI()->getApiParams().mMaxGameBrowserGamesInList;
        if (maxGameBrowserGames > 0)
        {
            mGameBrowserGameMap.reserve(maxGameBrowserGames);
        }
        mGameMemoryPool.reserve(maxGameBrowserGames, "GameBrowserList::GamePool");
        mLastUpdateTimeMS = NetTick();
    }


    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameBrowserList::~GameBrowserList()
    {
        // delete any/all games
        GameBrowserGameMap::iterator gameMapIter = mGameBrowserGameMap.begin();
        GameBrowserGameMap::iterator gameMapEnd = mGameBrowserGameMap.end();
        for ( ; gameMapIter != gameMapEnd; ++gameMapIter )
        {
            mGameMemoryPool.free(gameMapIter->second);
        }
    }
    
    /*! ************************************************************************************************/
    /*! \brief returns a specified GameBrowserGame given its id (or nullptr, if the game isn't in this list)
    
        \param[in] gameId The id of a specific game to get.
        \return returns a specified GameBrowserGame given its id (or nullptr, if the game isn't in this list)
    ***************************************************************************************************/
    const GameBrowserGame* GameBrowserList::getGameById(GameId gameId) const
    {
        GameBrowserGameMap::const_iterator mapIter = mGameBrowserGameMap.find(gameId);
        if (mapIter != mGameBrowserGameMap.end())
        {
            return mapIter->second;
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief destroys this list, freeing all associated GameBrowserGames.
    ***************************************************************************************************/
    void GameBrowserList::destroy()
    {
        mGameManagerAPI->destroyGameBrowserList(this);
    }

    void GameBrowserList::resetSubscription(GameBrowserListId serverListId)
    {
        mLastUpdateTimeMS = NetTick();

        NotifyGameListUpdate update;
        update.setListId(mServerListId); // onNotifyGameListUpdate enforces the update's list id must match mServerListId
        for (GameBrowserGameMap::const_iterator it = mGameBrowserGameMap.begin(); it != mGameBrowserGameMap.end(); ++it)
            update.getRemovedGameList().push_back(it->first);
        onNotifyGameListUpdate(&update);

        mServerListId = serverListId;
    }

    void GameBrowserList::sendFinalUpdate()
    {
        // This method will mark a game browser list as having received the final update and dispatch a notification informing the title code,
        // normally this would happen if we detected the need to resubscribe but failed to do so, on the off-chance that we received a final
        // update notification in the midst of our resubscribe flow though, no need to dispatch a second call to the client if a final update already
        // arrived during that resubscribe flow
        if (mListUpdatesFinished == true)
            return;

        GameBrowserGameVector removedGameVector(MEM_GROUP_FRAMEWORK_TEMP, "GameBrowserList::resetSubscription::removedGameVector");
        GameBrowserGameVector updatedGameVector(MEM_GROUP_FRAMEWORK_TEMP, "GameBrowserList::resetSubscription::updatedGameVector");

        mListUpdatesFinished = true;
        mGameManagerAPI->dispatchOnGameBrowserListUpdated(this, removedGameVector, updatedGameVector);
    }

    GameBrowserList::GameBrowserGameConstVector& GameBrowserList::getGameVectorByUser(BlazeId user, GameBrowserGameConstVector& games) const
    {                   
        for (GameBrowserList::GameBrowserGameVector::const_iterator thisGameItor=mDefaultGameView.begin(), itEnd=mDefaultGameView.end();thisGameItor!=itEnd;++thisGameItor)
        {            
            GameBrowserGame::GameBrowserPlayerVector& playerVector=(*thisGameItor)->getPlayerVector();
            for (GameBrowserGame::GameBrowserPlayerVector::const_iterator playerItor=playerVector.begin(), pitEnd=playerVector.end(); playerItor!=pitEnd; ++playerItor)
            {
                if ((*playerItor)->getId()==user)
                {
                    games.push_back(*thisGameItor);                        
                }
            }
        }
        return games;
    }

    /*! ************************************************************************************************/
    /*! \brief handles async NotifyGameListUpdate messages by updating the list's games; 
            dispatches GameManagerAPIListener::onGameBrowserListUpdated
    
            Note: can invalidate cached GameBrowser game and player pointers.

        \param[in] notifyGameListUpdate the server's update notification
    ***************************************************************************************************/
    void GameBrowserList::onNotifyGameListUpdate(const NotifyGameListUpdate *notifyGameListUpdate)
    {
        BlazeAssert(notifyGameListUpdate->getListId() == mServerListId);

        mLastUpdateTimeMS = NetTick();

        // first, we remove any games that have been bumped off of the list
        //   (caching the soon-to-be-deleted games in a temp vector)
        const GameIdList &removedGameIdList = notifyGameListUpdate->getRemovedGameList();
        GameBrowserGameVector removedGameVector(MEM_GROUP_FRAMEWORK_TEMP, "GameBrowserList::onNotifyGameListUpdate::removedGameVector");
        removedGameVector.reserve(removedGameIdList.size());
        GameIdList::const_iterator removedGameIdIter = removedGameIdList.begin();
        GameIdList::const_iterator removedGameIdEnd = removedGameIdList.end();
        for ( ; removedGameIdIter != removedGameIdEnd; ++removedGameIdIter )
        {
            GameBrowserGame *removedGame = removeGame( *removedGameIdIter );
            if ( removedGame != nullptr )
            {
                removedGameVector.push_back(removedGame);
            }
        }

        // next, we add/update games, building up a vector of updated GameBrowserGames (for the listener dispatch)
        const GameBrowserMatchDataList &updatedGameDataList = notifyGameListUpdate->getUpdatedGames();
        GameBrowserGameVector updatedGameList(MEM_GROUP_FRAMEWORK_TEMP, "onNofityGameListUpdate.updatedGameList");
        updatedGameList.reserve(updatedGameDataList.size());

        GameBrowserMatchDataList::const_iterator updatedGameDataIter = updatedGameDataList.begin();
        GameBrowserMatchDataList::const_iterator updatedGameDataEnd = updatedGameDataList.end();
        for ( ; updatedGameDataIter != updatedGameDataEnd; ++updatedGameDataIter )
        {
            GameBrowserGame *game = updateGame( *updatedGameDataIter ); // add/update game
            updatedGameList.push_back(game); // tracking a separate list of updated/added games for the listener dispatch
        }

        // finally, we update the list's 'final update' status, and dispatch to the listener(s)
        mListUpdatesFinished = (notifyGameListUpdate->getIsFinalUpdate() != 0);
        mGameManagerAPI->dispatchOnGameBrowserListUpdated(this, removedGameVector, updatedGameList);

        // delete the games that were removed from the list (after the dispatch)
        GameBrowserGameVector::iterator removedGameIter = removedGameVector.begin();
        GameBrowserGameVector::iterator removedGameEnd = removedGameVector.end();
        for ( ; removedGameIter != removedGameEnd; ++removedGameIter )
        {
            mGameMemoryPool.free(*removedGameIter);
        }

        // delete the players that were removed from existing games (after the dispatch)
        while (!mRemovedPlayerList.empty())
        {
            BLAZE_DELETE_PRIVATE(mMemGroup, GameBrowserPlayer, mRemovedPlayerList.front());
            mRemovedPlayerList.pop_front();
        }
       
    }

    /*! ************************************************************************************************/
    /*! \brief removes the supplied gameId from the list's gameMap and all views; returns the game pointer

        \param[in] gameId the gameId to remove from this list.
        \return the GameBrowserGame that was removed from the list (to be deleted)
    ***************************************************************************************************/
    inline GameBrowserGame* GameBrowserList::removeGame(GameId gameId)
    {
        GameBrowserGameMap::iterator gameMapIter = mGameBrowserGameMap.find(gameId);
        if (gameMapIter != mGameBrowserGameMap.end())
        {
            // remove game from game map
            GameBrowserGame *removedGame = gameMapIter->second;
            mGameBrowserGameMap.erase(gameMapIter);

            // remove game from the default view
            GameBrowserGameVector::iterator viewEnd = mDefaultGameView.end();
            GameBrowserGameVector::iterator viewIter = eastl::find(mDefaultGameView.begin(), viewEnd, removedGame);
            if (viewIter != viewEnd)
            {
                mDefaultGameView.erase(viewIter);
            }
            else
            {
                BlazeAssertMsg(false,  "Error: game is in gameMap, but not in the default view...");
            }

            return removedGame;
        }
        else
        {
            BLAZE_SDK_DEBUGF("Error: game '%" PRIu64 "' not found in gameMap...", gameId);
            return nullptr;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief adds a new (or updates an existing) gameBrowserGame from the supplied gameBrowserGameData.
            Modifies the game map and the default view.
    
        \param[in] gameBrowserGameData the game data to create/update a GameBrowserGame with.
    ***************************************************************************************************/
    inline GameBrowserGame* GameBrowserList::updateGame(const GameBrowserMatchData *gameBrowserMatchData)
    {
        // lookup the game, if it already exists
        GameBrowserGameMap::iterator mapIter = mGameBrowserGameMap.find( gameBrowserMatchData->getGameData().getGameId() );
        if (mapIter != mGameBrowserGameMap.end())
        {
             // update existing game
            GameBrowserGame *existingGame = mapIter->second;
            existingGame->update(gameBrowserMatchData);
            return existingGame;
        }
        else
        {
            // alloc/init new game
            if (getGameManagerAPI()->getApiParams().mMaxGameBrowserGamesInList > 0)
            {
                BlazeAssertMsg(mGameBrowserGameMap.size() < getGameManagerAPI()->getApiParams().mMaxGameBrowserGamesInList,
                        "Object count exceeds the memory cap specified in GameManagerApiParams");
            }

            GameBrowserGame *newGame = new (mGameMemoryPool.alloc()) GameBrowserGame(this, gameBrowserMatchData, mMemGroup);
            mGameBrowserGameMap[newGame->getId()] = newGame;
            mDefaultGameView.push_back(newGame);
            return newGame;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return true if game a should come before game b (eastl sort comparison struct).
            We sort by: FitScore (descending), gameName (ascending, case-insensitive), gameId (ascending).
    
        \param[in] a the first item to compare
        \param[in] b the second item to compare
        \return true if a should come before b
    ***************************************************************************************************/
    bool GameBrowserList::DefaultGameSortCompare::operator()(const GameBrowserGame *a, const GameBrowserGame *b) const
    {
        // primary sort key is the game's fitScore (higher is better)
        if (a->getFitScore() != b->getFitScore())
        {
            return ( a->getFitScore() > b->getFitScore() );
        }

        // secondary sort key is the game's name
        // Note: case insensitive, so we don't get "ZZZ" before "aaa"
        int nameCmpResult = blaze_stricmp(a->getName(), b->getName());
        if (nameCmpResult != 0)
        {
            return (nameCmpResult < 0);
        }

        // case sensitive so that "ABBA" comes before "abba"
        nameCmpResult = blaze_strcmp(a->getName(), b->getName());
        if (nameCmpResult != 0)
        {
            return (nameCmpResult < 0);
        }

        // last sort key is gameId (game names are not unique, btw)
        return (a->getId() < b->getId());
    }


} //GameManager
} // namespace Blaze
