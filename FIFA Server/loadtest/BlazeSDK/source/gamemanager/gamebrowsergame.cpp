/*! ************************************************************************************************/
/*!
    \file gamebrowsergame.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/gamemanager/gamebrowsergame.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/gamebrowserplayer.h"
#include "BlazeSDK/gamemanager/gamemanagerhelpers.h" // for SlotType helpers
#include "BlazeSDK/blaze_eastl/map.h"

namespace Blaze
{
namespace GameManager
{

    /*! ************************************************************************************************/
    /*! \brief construct and update a GameBrowserGame from the supplied data.

        \param[in] gameBrowserList the owning list
        \param[in] gameBrowserGameData the gameBrowserGameData to init the obj from
        \param[in] memGroupId mem group to be used by this class to allocate memory 
        ***************************************************************************************************/
        GameBrowserGame::GameBrowserGame(GameBrowserList *gameBrowserList, const GameBrowserMatchData *gameBrowserMatchData, MemoryGroupId memGroupId)
            : GameBase(gameBrowserMatchData->getGameData(), memGroupId),
            mQueueCount(gameBrowserMatchData->getGameData().getQueueCount()),
            mPlayerVector(memGroupId, MEM_NAME(memGroupId, "GameBrowserGame::mPlayerVector")),
            mGameBrowserList(gameBrowserList),
            mMemGroup(memGroupId),
            mGameMode(gameBrowserMatchData->getGameData().getGameMode(), memGroupId),
            mNetworkAddressList(memGroupId, MEM_NAME(memGroupId, "GameBrowserGame::mNetworkAddressList"))
        {
            mFitScore = gameBrowserMatchData->getFitScore();
            updatePlayerRoster(gameBrowserMatchData->getGameData());
            gameBrowserMatchData->getGameData().getHostNetworkAddressList().copyInto(mNetworkAddressList);
        }

    //! \brief destructor
    GameBrowserGame::~GameBrowserGame()
    {
        // Remove any outstanding txns.  Canceling here can be dangerous here as it will still attempt
        // to call the callback.  Since the object is being deleted, we go with the remove.
        JobScheduler *scheduler = mGameBrowserList->getGameManagerAPI()->getBlazeHub()->getScheduler();
        scheduler->removeByAssociatedObject(this);

        clearPlayerRoster();
    }

    /*! ************************************************************************************************/
    /*! \brief get all game and player data for this game from the blazeServer. This overrides this list's
        configuration (pulls down all data, even data excluded in the config).

        Use this method to download/refresh all of the data for this game (including player roster and
        all player data).

        For example, many games omit the player roster from gameBrowser lists by default (in the list's
        configuration).  If a user double clicks on a game entry, however you can call this method
        to get the game's roster from the server.

        NOTE: any future async update for the game will obey the list's config, meaning that additional
        data can become stale.
    
        \param[in] titleCb the callback functor dispatched when the RPC returns.
        \return a jobId, allowing you to cancel the RPC
    ***************************************************************************************************/
    JobId GameBrowserGame::downloadAllDataFields(const DownloadAllDataFieldsCbFunctor &titleCb)
    {
        // format & send up the request
        GetFullGameDataRequest request;
        request.getGameIdList().push_back(mGameId);

        GameManagerComponent *gameManagerComponent = mGameBrowserList->getGameManagerAPI()->getGameManagerComponent();
        JobId jobId = gameManagerComponent->getFullGameData(request,
            MakeFunctor(this, &GameBrowserGame::internalDownloadFullGameDataCb), titleCb);
        Job::addTitleCbAssociatedObject(mGameBrowserList->getGameManagerAPI()->getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief internal callback for GetFullGameData RPC (see downloadAllDataFields)
    ***************************************************************************************************/
    void GameBrowserGame::internalDownloadFullGameDataCb(const GetFullGameDataResponse *response, 
        BlazeError error, JobId id, DownloadAllDataFieldsCbFunctor titleCb)
    {
        // job not canceled, do update if successful then dispatch results
        if (error == ERR_OK)
        {
            BlazeAssert(response->getGames().size() == 1);
            const ListGameData *fullGameData = response->getGames().front();
            BlazeAssert(fullGameData->getGame().getGameId() == mGameId);

            // init this game with the full replicated game & playerRoster data
            initGameBaseData(&fullGameData->getGame());
            // gamebrowser game owns the network address list
            fullGameData->getGame().getTopologyHostNetworkAddressList().copyInto(mNetworkAddressList);
            updatePlayerRoster(&fullGameData->getGameRoster());
        }

        titleCb(error, this);
    } /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


    /*! ************************************************************************************************/
    /*! \brief update this GameBrowserGame with new GameBrowserGameData (game and roster info).
        Note: removed players are queued for deletion, not deleted outright.

        \param[in] gameBrowserGameData the data to init this game from
    ***************************************************************************************************/
    void GameBrowserGame::update(const GameBrowserMatchData *gameBrowserMatchData)
    {
        // init (or update) the game's base data & player roster
        GameBase::initGameBaseData(gameBrowserMatchData->getGameData());
        // gamebrowser game owns the network address list
        gameBrowserMatchData->getGameData().getHostNetworkAddressList().copyInto(mNetworkAddressList);
        mFitScore = gameBrowserMatchData->getFitScore();
        mGameMode = gameBrowserMatchData->getGameData().getGameMode();
        updatePlayerRoster(gameBrowserMatchData->getGameData());
    }

    /*! ************************************************************************************************/
    /*! \brief update the game's player roster (and the cached game size var). 
        Note: removed players are queued for deletion, not deleted outright.

        \param[in] replicatedPlayerList the full player data to init the player roster from
    ***************************************************************************************************/
    void GameBrowserGame::updatePlayerRoster(const ReplicatedGamePlayerList *replicatedPlayerList)
    {
        uint16_t totalParticipantCount = 0;
        uint16_t totalSpectatorCount = 0;

        ReplicatedGamePlayerList::const_iterator replicatedPlayerListIter = replicatedPlayerList->begin();
        ReplicatedGamePlayerList::const_iterator replicatedPlayerListEnd = replicatedPlayerList->end();
        for ( ; replicatedPlayerListIter != replicatedPlayerListEnd; ++replicatedPlayerListIter )
        {
            if (isParticipantSlot((*replicatedPlayerListIter)->getSlotType()))
            {
                ++totalParticipantCount;
            }
            else // spectator
            {
                ++totalSpectatorCount;
            }

        }

        updatePlayerRoster<ReplicatedGamePlayerList>( replicatedPlayerList, totalParticipantCount, totalSpectatorCount);
    }


    /*! ************************************************************************************************/
    /*! \brief update the game's player roster (and the cached game size var). 
        Note: removed players are queued for deletion, not deleted outright.

        \param[in] gameBrowserGameData the gameBrowserGameData to init the player roster from
    ***************************************************************************************************/
    void GameBrowserGame::updatePlayerRoster(const GameBrowserGameData& gameBrowserGameData)
    {
        // Update the queue count
        mQueueCount = gameBrowserGameData.getQueueCount();

        uint16_t totalParticipantCount = 0;
        uint16_t totalSpectatorCount = 0;
        const SlotCapacitiesVector &gamePlayerCounts = gameBrowserGameData.getPlayerCounts();
        for (SlotCapacitiesVector::size_type slotType = SLOT_PUBLIC_PARTICIPANT; slotType < MAX_PARTICIPANT_SLOT_TYPE; ++slotType)
        {
            totalParticipantCount = static_cast<uint16_t>(totalParticipantCount + gamePlayerCounts[slotType]);
        }

        for (SlotCapacitiesVector::size_type slotType = SLOT_PUBLIC_SPECTATOR; slotType < MAX_SLOT_TYPE; ++slotType)
        {
            totalSpectatorCount = static_cast<uint16_t>(totalSpectatorCount + gamePlayerCounts[slotType]);
        }

        updatePlayerRoster<GameBrowserGameData::GameBrowserPlayerDataList>(&gameBrowserGameData.getGameRoster(), totalParticipantCount, totalSpectatorCount);
    }

    /*! ************************************************************************************************/
    /*! \brief template function to update the player roster from either a GameBroserGameData::GameRosterList
        or a ReplicatedGamePlayerList

        \param[in] playerDataList either a GameBroserGameData::GameRosterList or a ReplicatedGamePlayerList
        \param[in] totalParticipantCount the total number of players in the game
        \param[in] totalSpectatorCount the total number of spectators in the game
    ***************************************************************************************************/
    template<class PlayerDataList>
    void GameBrowserGame::updatePlayerRoster(const PlayerDataList *playerDataList, uint16_t totalParticipantCount, uint16_t totalSpectatorCount)
    {

        // GB_LOAD_TESTING: constructing a temp map of players so we can tell who's a new addition, an update, or a delete.
        //     We might be able to restructure the TDF from the blazeServer to send us this info up front so we wouldn't have
        //     to recreate it client-side.

        // Note: we can't rely on the list size for the totalParticipantCount, since we don't always download the full roster (list may be empty)
        mParticipantCount = totalParticipantCount;
        // Same for spectators- we may not have the actual roster coming down from the Blaze server
        mSpectatorCount = totalSpectatorCount;


        // we put spectators and players in the player vector.
        mPlayerVector.reserve(mParticipantCount + mSpectatorCount);

        // we build a map of the player data so we can search efficiently
        typedef Blaze::map<PlayerId, typename PlayerDataList::const_value_type> PlayerDataMap;
        PlayerDataMap playerDataMap(MEM_GROUP_FRAMEWORK_TEMP, "GameBrowserGame::updatedGameMembers");

        eastl::set<PlayerId> reservedAndQueuedPlayers;

        //default queuedPlayerIndex to playerDataList's size (out of bounds)
        size_t queuedPlayerIndex = playerDataList->size();

        // if playerDataList size > mParticipantCount, this means the list contains both queued and spectators. Queued players are added at the end of the list (playerDataList) by the BlazeServer.
        if (queuedPlayerIndex > mParticipantCount && mQueueCount != 0)
        {
            // adjust index to by 1 (starts from 0)
            queuedPlayerIndex = playerDataList->size() - mQueueCount - 1;
        }

        typename PlayerDataList::const_iterator playerDataListIter = playerDataList->begin();
        typename PlayerDataList::const_iterator playerDataListEnd = playerDataList->end();
        for ( size_t i = 0; playerDataListIter != playerDataListEnd; ++playerDataListIter, ++i )
        {
            const typename PlayerDataList::const_value_type playerData = *playerDataListIter;
            playerDataMap[playerData->getPlayerId()] = playerData;
            if ((i >= queuedPlayerIndex) && (playerData->getPlayerState() == RESERVED))
            {
                reservedAndQueuedPlayers.insert(playerData->getPlayerId());
            }
        }

        typename PlayerDataMap::const_iterator playerDataMapEnd = playerDataMap.end();


        // First scan:
        // find game members who are no longer in the game and queue them for removal
        // update existing game members (who are still in the game), and remove them from the map
        //   This will leave us with a map containing only new players who can be added to the game
        GameBrowserPlayerVector::iterator playerIter = mPlayerVector.begin();
        while (playerIter != mPlayerVector.end()) // note: can't cache the end iterator; we're mutating the vector
        {
            GameBrowserPlayer *player = *playerIter;
            typename PlayerDataMap::iterator playerDataMapIter = playerDataMap.find(player->getId());
            if ( playerDataMapIter == playerDataMapEnd )
            {
                // player was deleted from the game (existing player not found in new replicated player map)
                mGameBrowserList->queuePlayerForDelete(player);
                playerIter = mPlayerVector.erase(playerIter);
            }
            else
            {
                // update existing player, and remove them from the map (so our 2nd scan only has new players)
                player->updatePlayerData( playerDataMapIter->second );
                playerDataMap.erase(playerDataMapIter);
                playerDataMapEnd = playerDataMap.end();
                ++playerIter;

                // For players who are reserved and queued - we check if they are in the reservedAndQueuedPlayers' set. 
                if ((player->getPlayerState() == RESERVED) && (reservedAndQueuedPlayers.find(player->getId()) != reservedAndQueuedPlayers.end()))
                {
                    player->setIsQueued(true);
                }
            }
        }

        // Second scan:
        // add new players to the game
        typename PlayerDataMap::iterator playerDataMapIter = playerDataMap.begin();
        for ( ; playerDataMapIter != playerDataMapEnd; ++playerDataMapIter )
        {
            // add the new player
            const typename PlayerDataList::const_value_type newPlayerData = playerDataMapIter->second;
            GameBrowserPlayer *newPlayer = BLAZE_NEW(mMemGroup, "GameBrowserGamePlayer") 
                                                GameBrowserPlayer(mGameBrowserList->getGameManagerAPI(), newPlayerData, mMemGroup);
            mPlayerVector.push_back(newPlayer);

            // For players who are reserved and queued - we check if they are in the reservedAndQueuedPlayers' set. 
            if ((newPlayer->getPlayerState() == RESERVED) && (reservedAndQueuedPlayers.find(newPlayer->getId()) != reservedAndQueuedPlayers.end()))
            {
                newPlayer->setIsQueued(true);
            }
        }
    }


        /*! ************************************************************************************************/
        /*! \brief clear the player roster, deleting all GameBrowserPlayers associated with this game.
        ***************************************************************************************************/
        void GameBrowserGame::clearPlayerRoster()
        {
            GameBrowserPlayerVector::iterator playerListIter = mPlayerVector.begin();
            GameBrowserPlayerVector::iterator playerListEnd = mPlayerVector.end();
            for ( ; playerListIter != playerListEnd; ++playerListIter )
            {
                GameBrowserPlayer *player = *playerListIter;
                BLAZE_DELETE_PRIVATE(mMemGroup, GameBrowserPlayer, player);
            }
            mPlayerVector.clear();
            mPlayerSlotCounts.assign(Blaze::GameManager::MAX_SLOT_TYPE, 0);            
        }


    /*! **********************************************************************************************************/
    /*! \brief  Returns the GameBrowserPlayer object for the supplied BlazeId (or nullptr, if the no player exists for that blazeId)
        \param    blazeId    The BlazeId (playerId) to get the player for.
        \return    Returns the GameBrowserPlayer representing the supplied user (or nullptr, if the blazeId isn't a game member).
    **************************************************************************************************************/
    GameBrowserPlayer* GameBrowserGame::getPlayerById(BlazeId blazeId) const
    {
        GameBrowserPlayerVector::const_iterator playerListIter = mPlayerVector.begin();
        GameBrowserPlayerVector::const_iterator playerListEnd = mPlayerVector.end();
        for ( ; playerListIter != playerListEnd; ++playerListIter )
        {
            GameBrowserPlayer *player = *playerListIter;
            if (EA_UNLIKELY( player->getId() == blazeId ))
            {
                return player;
            }
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief An eastl Compare struct; operator() return true if itemA should come before itemB.
        We sort by: player name (ascending, case-insensitive), then playerId.

        \param[in] a the first item to compare
        \param[in] b the second item to compare
        \return true if a should come before b
    ***************************************************************************************************/
    bool GameBrowserGame::DefaultPlayerSortCompare::operator()(const GameBrowserPlayer *a, const GameBrowserPlayer *b) const
    {
        // primary sort key is player's name
        // Note: case insensitive so that "abba" comes before "ZZ Top"
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

        // secondary sort by playerId (in case we have duplicate names)
        return a->getId() < b->getId();
    }


} //GameManager
} // namespace Blaze

