/*! ************************************************************************************************/
/*!
    \file gamebrowserlist.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamebrowser/gamebrowserlist.h"
#include "gamemanager/matchmaker/matchmakingutil.h"
#include "gamemanager/gamemanagerslaveimpl.h"
#include "gamemanager/gamebrowser/gamebrowserlistownerusersession.h"

namespace Blaze
{

namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief constructor; list is not ready for use until it's also been initialized.
    ***************************************************************************************************/
    GameBrowserList::GameBrowserList(GameBrowser& gameBrowser, GameBrowserListId gameBrowserListId, ListType listType, 
        const eastl::string& listConfigName, size_t visibleListCapacity, TimeValue maxUpdateInterval,
        IGameBrowserListOwner& owner)
        : mGameBrowser(gameBrowser),
          mListId(gameBrowserListId),
          mListType(listType),
          mListConfigName(listConfigName),
          mOwner(owner), 
          mActualResultSetSize(0),
          mReceivedGamesCount(0),
          mHiddenFromIdle(false),
          mLastUpdateTime(TimeValue::getTimeOfDay()),
          mMaxUpdateInterval(maxUpdateInterval),
          mMatchingGameSessionMapInv(BlazeStlAllocator("mMatchingGameSessionMapInv", GameManagerSlave::COMPONENT_MEMORY_GROUP)),
          mMaxVisibleGames(visibleListCapacity)
    {         

    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameBrowserList::~GameBrowserList()
    {
        Fiber::signal(mUpdateEventHandle, Blaze::ERR_SYSTEM);
    }
    
  
    /*! ************************************************************************************************/
    /*! \brief send 1+ listUpdateNotifications to the list owner containing all the changes to the list since
            the last update notification was sent.

        Changes are batched into multiple notifications with maxGameUpdatesPerNotification games sent per notification,
        and maxListUpdateNotifications sent per call to sendListUpdateNotifications
    
        \param[in] gameManagerSlaveImpl used to send the notifications to the user
        \param[in] maxGameUpdatesPerNotification the maximum number of updated games to include in each notification
        \param[in] maxListUpdateNotifications the maximum number of update notifications to send for this list
    ***************************************************************************************************/
    bool GameBrowserList::sendListUpdateNotifications(uint32_t maxGameUpdatesPerNotification, uint32_t maxListUpdateNotifications, GameBrowser::NotifyGameListReason notifyReason, uint32_t msgNum)
    {
        bool sentFinalUpdate = false;

        if (!mOwner.canUpdate())
            return false;

        // GB_AUDIT: if the notification reason is game destroyed, we should pass that down to the client (instead of removing all the games).

        // we can bail if there are no changes and this is NOT a destructing or completed snapshot list
        bool listNotChanged = (mUpdatedGames.empty() && mRemovedGames.empty());
        if (listNotChanged && notifyReason != GameBrowser::LIST_DESTROYED)
        {
            if (mListType == GameManager::GAME_BROWSER_LIST_SNAPSHOT)
            {
                if (!isFullResultSetReceived())
                {
                    // snapshot list did not receive full result set from search slave and
                    // there's nothing new to report, so bail out
                    return false;
                } 
            } 
            else
            {
                if (TimeValue::getTimeOfDay() < (mLastUpdateTime + mMaxUpdateInterval))
                {
                    // list is not being destroyed, we've sent a recent update,
                    // and there's nothing new to send down so bail out
                    return false;
                }
            }
        }

        // send down a series of list update notifications; we break the update into chunks up so no single notification
        //  contains more than maxGameUpdatesPerNotification game updates.  (the game update obj is large).
        //
        //  If we're chunking the update like this, we'll send down a game removal for each update (while removals exist).
        //  This allows the client to maintain the list's max capacity (we've removed a game for each game we're adding/updating).
        //  The last notification will contain the remaining removed games.
        size_t numUpdatedGamesRemaining = mUpdatedGames.size();
        size_t numRemovedGamesRemaining = mRemovedGames.size();

        mGameBrowser.incrementMetric_NewGameUpdatesByIdleEnd(numUpdatedGamesRemaining);

        uint32_t notificationUpdatesSent = 0;
        GameIdSet::iterator updateMapIter = mUpdatedGames.begin();
        GameIdSet::iterator updateMapEnd = mUpdatedGames.end();
        GameIdSet::iterator removeSetIter = mRemovedGames.begin();
        GameIdSet::iterator removeSetEnd = mRemovedGames.end();
        do
        {
            NotifyGameListUpdate notifyGameListUpdate;
            notifyGameListUpdate.setListId(mListId);

            // reserve space for the number of game updates in this notification.
            if (numUpdatedGamesRemaining > 0)
            {
                uint32_t updatesInNotification = (numUpdatedGamesRemaining > maxGameUpdatesPerNotification) ? maxGameUpdatesPerNotification : numUpdatedGamesRemaining;
                notifyGameListUpdate.getUpdatedGames().reserve(updatesInNotification);
            }

            // populate the notification with updated games (up to maxGameUpdatesPerNotification) and removed gameIds.
            uint32_t numUpdatedGamesInNotification = 0;
            // GB_TODO: we have a potentially unbounded number of removed games, this could be an issue if there's a case where a massive number of games would be removed such as a slave going down
            while ((updateMapIter != updateMapEnd || removeSetIter != removeSetEnd) && (numUpdatedGamesInNotification < maxGameUpdatesPerNotification))
            {
                if (updateMapIter != updateMapEnd)
                {
                    // add the updated game info to the notification
                    const GameId updatedGameId = *updateMapIter;
                    MatchingGameSessionMapInv::iterator mitr = mMatchingGameSessionMapInv.find(updatedGameId);
                    if (mitr != mMatchingGameSessionMapInv.end() && mitr->second.mRanked)
                    {
                        GameBrowserMatchData *gameBrowserMatchData = notifyGameListUpdate.getUpdatedGames().pull_back();
                        gameBrowserMatchData->setGameData(*mitr->second.mMatchingGameSessionMapIterator->second.data);
                        gameBrowserMatchData->setFitScore(mitr->second.mMatchingGameSessionMapIterator->second.score);
                        mitr->second.mMatchingGameSessionMapIterator->second.isNew = false;
                        numUpdatedGamesInNotification++;
                    }
                                        
                    numUpdatedGamesRemaining--;
                    updateMapIter++;
                }

                if (removeSetIter != removeSetEnd)
                {
                    // add the removed gameId to the notification
                    GameId removedGameId = *removeSetIter;
                    notifyGameListUpdate.getRemovedGameList().push_back(removedGameId);
                    numRemovedGamesRemaining--;
                    removeSetIter++;
                }
            }

            bool isFinalUpdateForSnapshotList = 
                   (mListType == GameManager::GAME_BROWSER_LIST_SNAPSHOT) 
                && (updateMapIter == updateMapEnd) && (removeSetIter == removeSetEnd)
                && isFullResultSetReceived();

            if (isFinalUpdateForSnapshotList || (notifyReason == GameBrowser::LIST_DESTROYED))
            {
                notifyGameListUpdate.setIsFinalUpdate(1);
            }
            
            mOwner.onUpdate(notifyGameListUpdate, msgNum);
            ++notificationUpdatesSent;
            if (notifyGameListUpdate.getIsFinalUpdate()) 
            {
                sentFinalUpdate = true;
                break;
            }

        } while ( ((numUpdatedGamesRemaining > 0) || (numRemovedGamesRemaining > 0)) && (notificationUpdatesSent < maxListUpdateNotifications));

        if (!mUpdatedGames.empty())
        {
            // delete from mUpdatedGames
            mUpdatedGames.erase(mUpdatedGames.begin(), updateMapIter);
        }
      
        // we'll send down all the removed games
        mRemovedGames.clear();

        mLastUpdateTime = TimeValue::getTimeOfDay();

        return sentFinalUpdate;
    }

    BlazeRpcError GameBrowserList::fillSyncGameList(uint32_t maxGameListSize, GameBrowserMatchDataList& gameList)
    {

        // Game Browser's Idle should not handle this list
        mHiddenFromIdle = true;

        // wait for all the results here
        while (mTopRankedMatchingGameSessionMap.size() < maxGameListSize)
        {
            //no one else should be waiting on our handle
            if (mUpdateEventHandle.isValid())
            {
                return Blaze::ERR_SYSTEM;
            }

            BlazeRpcError err =  Fiber::getAndWait(mUpdateEventHandle, "GameBrowserList::waitForGameListUpdate");
            if (err != Blaze::ERR_OK)
            {
                // most likely timeout/shutdown whatever, just return
                return err;
            }
        }

        // Grab the top entries from highest to lowest FitScore:
        MatchingGameSessionMapDescend::iterator topRankIter = mTopRankedMatchingGameSessionMap.begin();
        MatchingGameSessionMapDescend::iterator topRankEnd = mTopRankedMatchingGameSessionMap.end();
        
        for (; topRankIter != topRankEnd; ++topRankIter)
        {
            GameBrowserMatchData *gameBrowserMatchData = gameList.pull_back();
            topRankIter->second.data->copyInto(gameBrowserMatchData->getGameData());
            gameBrowserMatchData->setFitScore(topRankIter->second.score);
        }

        return Blaze::ERR_OK;
    }

    uint32_t GameBrowserList::getListCreateMsgNum() const
    {
        if (getOwnerType() == GAMEBROWSERLIST_OWNERTYPE_USERSESSION)
            return static_cast<GameBrowserListOwnerUserSession&>(mOwner).getMsgNum();
        else
            return 0;
    }


    void GameBrowserList::onGameAdded(GameManager::GameId gameId, GameManager::FitScore fitScore, GameManager::GameBrowserGameDataPtr& gameData)
    {
        mReceivedGamesCount++;

        processMatchingGame(gameId, fitScore, gameData);
        TRACE_LOG("[SortedFilteredGameBrowserList].onGameAdded MATCH list(" << getListId() << ") game(" << gameId 
            << "),  fitscore '" << fitScore << "'.");

        return;
    }

    void GameBrowserList::onGameUpdated(GameManager::GameId gameId, GameManager::FitScore fitScore, GameBrowserGameDataPtr& gameData)
    {
        //did the match status change?
        //If snapshot, its past initial search, so no op
        if (mListType != GameManager::GAME_BROWSER_LIST_SNAPSHOT)
        {
            TRACE_LOG("[SortedFilteredGameBrowserList].updateGame MATCH list(" << getListId() << ") game(" << gameId
                << ") fit score(" << fitScore << ")");
            processMatchingGame(gameId, fitScore, gameData);        
        } 
    }

    
    /*! ************************************************************************************************/
    /*! \brief add/update a matching game to this list's containers
    ***************************************************************************************************/
    void GameBrowserList::processMatchingGame(GameId gameId, FitScore fitScore, GameManager::GameBrowserGameDataPtr& gameData )
    {
        // deal with a matching game; we need to determine:
        //   1) is this a new match (first insertion) or an update of an existing game (still matching, but an updated fitScore)
        //   2) does this game belong in the visible list

        // if the game is already present in the visible lists (and the fitScore changed), remove it from the list
        
        bool isNew = true;
        MatchingGameSessionMapInv::const_iterator mitr = mMatchingGameSessionMapInv.find(gameId);
        if (mitr != mMatchingGameSessionMapInv.end())        
        {
            Match& updatedMatch = mitr->second.mRanked ?  mitr->second.mMatchingGameSessionMapIterator->second : mitr->second.mUnrankedGameSessionMapIterator->second;
            if (updatedMatch.score == fitScore)
            {
                // just send an game updated notification so the client can get the changes
                //   (no fitScore change, so game position is the same).
                updatedMatch.data = gameData;
                if (mitr->second.mRanked)
                    markGameAsUpdated(gameId);
                return;
            }
            else
            {
                isNew = updatedMatch.isNew;
                removeGame(gameId);
            }        
        }

        Match gameMatch(fitScore, gameId, gameData, isNew);    
        MatchingGameSessionInfo info;

        //if there is room in the top rank match, put it there 
        if (mTopRankedMatchingGameSessionMap.size() < mMaxVisibleGames)
        {
            info.mMatchingGameSessionMapIterator = mTopRankedMatchingGameSessionMap.insert(eastl::make_pair(fitScore, gameMatch));
            info.mRanked = true;       
            markGameAsUpdated(gameId);
        }
        else
        {
            // This is the game session with the lowest FitScore:
            MatchingGameSessionMapDescend::reverse_iterator itr = mTopRankedMatchingGameSessionMap.rbegin();    // Last element
            if (itr->first < fitScore)
            {
                //Bumps someone in the list
                MatchingGameSessionInfo& prevInfo = mMatchingGameSessionMapInv[itr->second.gameId];
                prevInfo.mUnrankedGameSessionMapIterator = mUnrankedGameSessionMap.insert(*itr);
                prevInfo.mRanked = false;

                markGameAsRemoved(itr->second.gameId, itr->second.isNew);
                mTopRankedMatchingGameSessionMap.erase(itr);

                info.mMatchingGameSessionMapIterator = mTopRankedMatchingGameSessionMap.insert(eastl::make_pair(fitScore, gameMatch));
                info.mRanked = true;           
                markGameAsUpdated(gameId);
            }
            else
            {
                info.mRanked = false;
                info.mUnrankedGameSessionMapIterator = mUnrankedGameSessionMap.insert(eastl::make_pair(fitScore, gameMatch));
            }
        }

        mMatchingGameSessionMapInv[gameId] = info;   
    }

    /*! ************************************************************************************************/
    /*! \brief Marks the supplied game as updated in this list (adds the game to the list's set of updated
            games).  Updates are sent to the client by sendListUpdateNotifications().
    ***************************************************************************************************/
    void GameBrowserList::markGameAsUpdated(GameId gameId)
    {
        // add the game to the updated game list and remove it from the removed list (in case it had previously
        //  been removed).  This sort of 'jitter' can occur as dirty games are evaluated and
        //  moved between the visible and reserve containers.
        TRACE_LOG("[GameBrowserList].markGameAsUpdated: Game marked as updated [" << gameId << "]");

        mUpdatedGames.insert(gameId);
        mRemovedGames.erase(gameId);
    }


    void GameBrowserList::onGameRemoved(GameManager::GameId gameId)
    {
        TRACE_LOG("[SortedFilteredGameBrowserList].onGameRemoved GAME BROWSER LIST(" << getListId() << ") TOKEN REMOVED id(" << gameId << ").");        

        removeGame(gameId);
    }

    
    /*! ************************************************************************************************/
    /*! \brief process a game that does not match this list's criteria.
        \param[in] game the game that does not match
    ***************************************************************************************************/
    void GameBrowserList::removeGame(GameId gameId)
    {
        MatchingGameSessionMapInv::iterator itr = mMatchingGameSessionMapInv.find(gameId);
        if (itr != mMatchingGameSessionMapInv.end())
        {
            if (itr->second.mRanked)
            {         
                bool isNew = itr->second.mMatchingGameSessionMapIterator->second.isNew;

                mTopRankedMatchingGameSessionMap.erase(itr->second.mMatchingGameSessionMapIterator);
                markGameAsRemoved(gameId, isNew);
                TRACE_LOG("[GameBrowserList].removeGame: marking game as removed in list(" << getListId() << ") gameid (" << gameId);

                if (mTopRankedMatchingGameSessionMap.size() == mMaxVisibleGames - 1 && !mUnrankedGameSessionMap.empty())
                {
                    //if we are full, promote someone
                    MatchingGameSessionMapDescend::iterator it = mUnrankedGameSessionMap.begin();
                    MatchingGameSessionInfo& newInfo = mMatchingGameSessionMapInv[it->second.gameId];
                    newInfo.mMatchingGameSessionMapIterator = mTopRankedMatchingGameSessionMap.insert(*it);
                    newInfo.mRanked = true;
                    it->second.isNew = true;
                    markGameAsUpdated(it->second.gameId);
                    
                    TRACE_LOG("[GameBrowserList].removeGame: marking game as updated in list(" << getListId() << ") gameid (" << it->second.gameId << ") fitscore (" << it->second.score << ")" );
                    mUnrankedGameSessionMap.erase(it);
                }
            }
            else
            {
                mUnrankedGameSessionMap.erase(itr->second.mUnrankedGameSessionMapIterator);
            }

            mMatchingGameSessionMapInv.erase(itr);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief remove all matching games from this list.  Caller is responsible for calling sendListUpdateNotifications, if needed.
        Marks all games as removed for caller before sendListUpdateNotifications.
    ***************************************************************************************************/
    void GameBrowserList::removeAllGames()
    {
        while (!mMatchingGameSessionMapInv.empty())
        {            
            removeGame(mMatchingGameSessionMapInv.begin()->first);
        }
    }

    void GameBrowserList::removeSearchInstance(InstanceId searchInstanceId)
    {
        InstanceIdList::iterator instanceItr = mHostingSlaveInstanceIdList.begin();
        InstanceIdList::iterator instanceEnd = mHostingSlaveInstanceIdList.end();
        for ( ; instanceItr != instanceEnd; ++instanceItr )
        {
            if (*instanceItr == searchInstanceId)
            {
                // Our list was being serviced by the instance id that is changing, need to clean up all games from the master associated with the search slave
                mHostingSlaveInstanceIdList.erase(instanceItr);
                break;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Marks the supplied game as removed from this list (adds the game to the list's set of
            removed games).  Removals are sent to the client by sendListUpdateNotifications().
        Pre: game was added in list
    ***************************************************************************************************/
    void GameBrowserList::markGameAsRemoved(GameId gameId, bool isNew)
    {
        TRACE_LOG("[GameBrowserList].markGameAsRemoved: Game marked as removed [" << gameId << "]");

        mUpdatedGames.erase(gameId);
        
        //if a game is in the update list with the 'new' flag set, that means it has never been sent down
        //to the client.  In this case we just remove it and pretend it never existed, as otherwise
        //we'll be telling the client to remove a game it has no idea of.
        if (!isNew)
        {
            mRemovedGames.insert(gameId);
        }

    }


    void GameBrowserList::signalGameListUpdate()
    {
        Fiber::signal(mUpdateEventHandle, Blaze::ERR_OK);
    }


} // namespace GameManager
} // namespace Blaze
