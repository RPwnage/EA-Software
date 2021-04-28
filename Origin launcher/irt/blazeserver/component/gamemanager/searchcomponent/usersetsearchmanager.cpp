/*! ************************************************************************************************/
/*!
    \file usersetsearch.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/connectionmanager.h"
#include "gamemanager/searchcomponent/usersetsearchmanager.h"
#include "gamemanager/searchcomponent/searchslaveimpl.h"
#include "component/gamemanager/tdf/search_config_server.h"


namespace Blaze
{
namespace Search
{
    const char8_t* USERSET_GAME_LIST_CONFIG_NAME = "usersetGameList";   // Duplicated in GameBrowser

    /*! ************************************************************************************************/
    /*! \brief Construct the GameBrowser obj. Must call configure before attempting to use GameBrowser.
    *************************************************************************************************/
    UserSetSearchManager::UserSetSearchManager(SearchSlaveImpl &searchSlave)
        : mSearchSlave(searchSlave),
        mGameIdUserSetListMap(BlazeStlAllocator("mGameIdUserSetListMap", SearchSlave::COMPONENT_MEMORY_GROUP))
    {
    }

    CreateUserSetGameListError::Error UserSetSearchManager::processCreateUserSetGameList(const Blaze::Search::CreateUserSetGameListRequest &request, Blaze::Search::CreateGameListResponse &response, const Message* message)
    {
        // We no longer track a limit.  Titles are free to create GB lists until they crash.

        const GameManager::GameBrowserListConfig* listConfig = mSearchSlave.getListConfig(USERSET_GAME_LIST_CONFIG_NAME);
        if ( listConfig == nullptr )
        {
            WARN_LOG("[UserSetSearchManager].processCreateUserSetGameList config " << USERSET_GAME_LIST_CONFIG_NAME << " required, aborting user set creation");
            return CreateUserSetGameListError::SEARCH_ERR_INVALID_LIST_CONFIG_NAME;
        }

        SlaveSessionId instanceSessionId = SlaveSession::INVALID_SESSION_ID;
        InstanceId instanceId = GetInstanceIdFromInstanceKey64(request.getGameBrowserListId());
        if (gController->getInstanceId() != instanceId)
        {
            SlaveSessionPtr sess = gController->getConnectionManager().getSlaveSessionByInstanceId(instanceId);
            if (sess != nullptr)
            {
                instanceSessionId = sess->getId();
            }
        }

        UserSetGameList* newList = BLAZE_NEW UserSetGameList(mSearchSlave, request.getGameBrowserListId(), instanceSessionId, request.getUserSessionId(), *listConfig);
        mSearchSlave.addList(*newList);

        TRACE_LOG("[UserSetSearchManager].processGetUserSetGameListSubscription: UserSetGameList GameBrowserList(" << newList->getListId() 
            << ") created, (" << request.getInitialWatchList().size() << ") watchees added");
        for (BlazeIdList::const_iterator friendsIt=request.getInitialWatchList() .begin(), friendsItEnd= request.getInitialWatchList().end(); friendsIt != friendsItEnd; ++friendsIt)
        {
            newList->trackUser(*friendsIt);
        }

        response.setNumberOfGamesToBeDownloaded(newList->getUpdatedGamesSize());

        mSearchSlave.scheduleNextIdle(TimeValue::getTimeOfDay(), mSearchSlave.getDefaultIdlePeriod());

        return CreateUserSetGameListError::ERR_OK;
    }


    UpdateUserSetGameListError::Error UserSetSearchManager::processUpdateUserSetGameList(const Blaze::Search::UserSetWatchListUpdate &request, const Message* message)
    {
        for (UserSetWatchListUpdate::GameBrowserListIdList::const_iterator idItr = request.getGameBrowserLists().begin(), idEnd = request.getGameBrowserLists().end(); idItr != idEnd; ++idItr)
        {
            GameListBase* gameList = mSearchSlave.getList(*idItr);
            if (gameList == nullptr)
            {
                continue;
            }

            if (!gameList->isUserList())
            {
                WARN_LOG("[UserSetSearchManager].processUpdateUserSetGameList: Game list id " << *idItr << " is not a user set list, ignoring.");
                continue;
            }

            UserSetGameList& userSetList = *(UserSetGameList*) gameList;
            for (BlazeIdList::const_iterator itr = request.getUsersAdded().begin(), end = request.getUsersAdded().end(); itr != end; ++itr)
            {
                userSetList.trackUser(*itr);
            }

            for (BlazeIdList::const_iterator itr = request.getUsersRemoved().begin(), end = request.getUsersRemoved().end(); itr != end; ++itr)
            {
                userSetList.stopTrackingUser(*itr);
            }
        }


        mSearchSlave.scheduleNextIdle(TimeValue::getTimeOfDay(), mSearchSlave.getDefaultIdlePeriod());

        return UpdateUserSetGameListError::ERR_OK;
    }

    
    /*! ************************************************************************************************/
    /*! \brief update the internal data for mapping game and user for "userset game list"

        \param[in] gameSession the GameSession object for the updated game.
        \param[in] blazeId the user
    ***************************************************************************************************/
    void UserSetSearchManager::updateGameOnUserAction(const BlazeId& blazeId, GameManager::GameId gameId, bool isJoining)
    {
        TRACE_LOG("[GameBrowser].updateGameOnUserAction: game(" << gameId << ") user(" << blazeId << ")");
        BlazeIdReferenceMap::iterator watcheeIter = mWatcheeUserSetGameListMap.find(blazeId);

        //if the joining user is a user being watched(watchee)
        if (watcheeIter != mWatcheeUserSetGameListMap.end())
        {
            UserSetGameListSet& watcherGameBrowserListSet = watcheeIter->second;
            TRACE_LOG("[GameBrowser].updateGameOnUserAction: notify all (" << watcherGameBrowserListSet.size() 
                      << ") watcher(s) for user(" << blazeId << ")");
            UserSetGameListSet::iterator listSetIter = watcherGameBrowserListSet.begin();
            UserSetGameListSet::iterator listSetEnd = watcherGameBrowserListSet.end();
            for (; listSetIter != listSetEnd; ++listSetIter)
            {
                UserSetGameList* list = const_cast<UserSetGameList *>(*listSetIter);
                if (isJoining)
                {
                    list->addUserMatch(gameId, blazeId);
                }
                else
                {
                    list->removeUserMatch(gameId, blazeId);
                }
            }
        }
    }


    /*! ************************************************************************************************/
    /*! \brief Updates a game in Game Browser's GameBrowserGameMap, marking the game as 'dirty'.
            The various reasons why the game is dirty are held in the game's gameSession's matchmakingGameInfoCache obj

        Not a real listener, GB must be dispatched after a GM game's roster has been updated.

        \param[in] gameSession the GameSession object for the updated game.
    ***************************************************************************************************/
    void UserSetSearchManager::onUpdate(GameManager::GameId gameId)
    {
        TRACE_LOG("[GameBrowser].onUpdate: Processing update for game [" << gameId << "]");
    
        GameIdUserSetListReferenceMap::iterator listSetIter = mGameIdUserSetListMap.find(gameId);
        if (listSetIter != mGameIdUserSetListMap.end())
        {
            UserSetGameListSet& listSet = listSetIter->second;
            for (UserSetGameListSet::iterator it = listSet.begin(), itEnd = listSet.end(); it != itEnd; ++it)
            {
                UserSetGameList *list = const_cast<UserSetGameList *>(*it);
                list->updateGame(gameId);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Removed a game from Game Browser's GameBrowserGameMap.
            Not a real listener, GB must be dispached after a GM game's roster has been updated.
        \param[in] gameSession the GameSession object to be removed.
    ***************************************************************************************************/
    void UserSetSearchManager::onErase(GameManager::GameId gameId)
    {        
        GameIdUserSetListReferenceMap::iterator listSetIter = mGameIdUserSetListMap.find(gameId);
        if (listSetIter != mGameIdUserSetListMap.end())
        {
            UserSetGameListSet& listSet = listSetIter->second;
            for (UserSetGameListSet::iterator it = listSet.begin(), itEnd = listSet.end(); it != itEnd; ++it)
            {
                UserSetGameList *list = const_cast<UserSetGameList *>(*it);
                // unmapGame() does not mutate the listSet during itertion,
                // the listSet will be removed wholesale by erase() below.
                list->unmapGame(gameId);
            }
            mGameIdUserSetListMap.erase(listSetIter);
        }

        TRACE_LOG("[GameBrowser].onErase: gameId(" <<gameId << ")");
    }

    void UserSetSearchManager::addListToGlobalUserMap(BlazeId blazeId, UserSetGameList& list)
    {
        mWatcheeUserSetGameListMap[blazeId].insert(&list);
    }

    void UserSetSearchManager::removeListFromGlobalUserMap(BlazeId blazeId, UserSetGameList& list)
    {
        BlazeIdReferenceMap::iterator itr = mWatcheeUserSetGameListMap.find(blazeId);
        if (itr != mWatcheeUserSetGameListMap.end())
        {
            itr->second.erase(&list);
            if (itr->second.empty())
            {
                mWatcheeUserSetGameListMap.erase(itr);
            }
        }
        
    }

    void UserSetSearchManager::addListToGlobalGameMap(GameManager::GameId gameId, UserSetGameList& list)
    {
        mGameIdUserSetListMap[gameId].insert(&list);
    }

    void UserSetSearchManager::removeListFromGlobalGameMap(GameManager::GameId gameId, UserSetGameList& list)
    {
        GameIdUserSetListReferenceMap::iterator itr = mGameIdUserSetListMap.find(gameId);
        if (itr != mGameIdUserSetListMap.end())
        {
            itr->second.erase(&list);
            if (itr->second.empty())
            {
                mGameIdUserSetListMap.erase(itr);
            }
        }     
    }


    UserSetSearchManager::UserSetGameList::UserSetGameList(SearchSlaveImpl& searchSlave, GameManager::GameBrowserListId gameBrowserListId, SlaveSessionId originalInstanceSessionId, UserSessionId ownerSessionId, const GameManager::GameBrowserListConfig& listConfig)
        : GameListBase(searchSlave, gameBrowserListId, originalInstanceSessionId, GameManager::GAME_BROWSER_LIST_SUBSCRIPTION, ownerSessionId),
          mGameIdToWatchedMembersMap(BlazeStlAllocator("mGameIdToWatchedMembersMap", SearchSlave::COMPONENT_MEMORY_GROUP))
    {
        mListConfigName = USERSET_GAME_LIST_CONFIG_NAME;
    }

    void UserSetSearchManager::UserSetGameList::cleanup()
    {
        for (WatchedMemberSet::const_iterator itr = mTrackedUsers.begin(), end = mTrackedUsers.end(); itr != end; ++itr)
        {
            mSearchSlave.getUserSetSearchManager().removeListFromGlobalUserMap(*itr, *this);
        }        

        for (GameIdToWatchedMembersMap::const_iterator itr = mGameIdToWatchedMembersMap.begin(), end = mGameIdToWatchedMembersMap.end(); itr != end; ++itr)
        {
           mSearchSlave.getUserSetSearchManager().removeListFromGlobalGameMap(itr->first, *this); 
        }
    }

    void UserSetSearchManager::UserSetGameList::removeGame(GameManager::GameId gameId)
    {
        unmapGame(gameId);
        mSearchSlave.getUserSetSearchManager().removeListFromGlobalGameMap(gameId, *this); 
    }

    void UserSetSearchManager::UserSetGameList::unmapGame(GameManager::GameId gameId)
    {
        //List is no longer tracking this game, eliminate it
        mGameIdToWatchedMembersMap.erase(gameId);
        markGameAsRemoved(gameId);
    }

    void UserSetSearchManager::UserSetGameList::trackUser(BlazeId player)
    {
        WatchedMemberSet::insert_return_type insertRet = mTrackedUsers.insert(player);
        if (insertRet.second)
        {
            TRACE_LOG("[UserSetGameList].trackUser: watchee(" << player << ") GameBrowserList(" << getListId() 
                << ") added. Watch count for this list now " << mTrackedUsers.size() );


            mSearchSlave.getUserSetSearchManager().addListToGlobalUserMap(player, *this); 

            const GameManager::UserSessionGameSet* userGameSet = mSearchSlave.getUserSessionGameSetByBlazeId(player);
            if (userGameSet)
            {
                for (GameManager::UserSessionGameSet::const_iterator it = userGameSet->begin(), itEnd = userGameSet->end(); it != itEnd; ++it)
                {
                    addUserMatch(*it, player);
                }
            }

        }
    }

    void UserSetSearchManager::UserSetGameList::stopTrackingUser(BlazeId player)
    {
        WatchedMemberSet::iterator itr = mTrackedUsers.find(player);
        if (itr != mTrackedUsers.end())
        {
            TRACE_LOG("[UserSetGameList].stopTrackingUser: watchee(" << player << ") GameBrowserList(" << getListId() 
                << ") removed. Watch count for this list now " << mTrackedUsers.size() - 1 );

            mTrackedUsers.erase(itr);            
            mSearchSlave.getUserSetSearchManager().removeListFromGlobalUserMap(player, *this);

            const GameManager::UserSessionGameSet* userGameSet = mSearchSlave.getUserSessionGameSetByBlazeId(player);
            if (userGameSet)
            {
                for (GameManager::UserSessionGameSet::const_iterator it = userGameSet->begin(), itEnd = userGameSet->end(); it != itEnd; ++it)
                {
                    removeUserMatch(*it, player);
                }
            }
        }
    }

    void UserSetSearchManager::UserSetGameList::addUserMatch(GameManager::GameId gameId, BlazeId player)
    {
        GameIdToWatchedMembersMap::insert_return_type insertRet = mGameIdToWatchedMembersMap.insert(gameId);            
        insertRet.first->second.insert(player);
        if (insertRet.second)
        {
            TRACE_LOG("[UserSetGameList].processMatchedGame: Adding game (" << gameId
                << ") for user (" << player  << ") to list (" << getListId() << "), there are now " 
                <<  mGameIdToWatchedMembersMap[gameId].size() << " watched users in the game.");

             mSearchSlave.getUserSetSearchManager().addListToGlobalGameMap(gameId, *this);
             markGameAsUpdated(gameId, 0, true);
        }
        else
        {
             markGameAsUpdated(gameId, 0, false);
        }
    }

    void UserSetSearchManager::UserSetGameList::removeUserMatch(GameManager::GameId gameId, BlazeId player)
    {
        GameIdToWatchedMembersMap::iterator itr = mGameIdToWatchedMembersMap.find(gameId);
        if (itr != mGameIdToWatchedMembersMap.end())
        {
            itr->second.erase(player);

            TRACE_LOG("[UserSetGameList].processMatchedGame: : After erasing user (" << player
                << ") in game (" << gameId << ") from list (" << getListId() << "), there are " 
                << itr->second.size() << " watched users remaining in the game.");

            if (itr->second.empty())
            {
                TRACE_LOG("[UserSetGameList].processMatchedGame: : After erasing user (" << player 
                    << ") removing game (" << gameId << ") from list (" << getListId() << ")");
                
                removeGame(gameId);
            }
            else
            {
                markGameAsUpdated(gameId, 0, false);
            }   
        }
    }

} // namespace Search
} // namespace Blaze
