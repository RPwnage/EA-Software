/*! ************************************************************************************************/
/*!
    \file playerrostermaster.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/playerrostermaster.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/externalsessions/externaluserscommoninfo.h"

namespace Blaze
{
namespace GameManager
{

    PlayerRosterMaster::PlayerRosterMaster()
    {
    }

    PlayerRosterMaster::~PlayerRosterMaster()
    {
    }

    /*! ************************************************************************************************/
    /*! \brief add a new player to the map based on the state.
        
        \param[in] newPlayerInfo - the Player info of the new player.
        \param[in] playerReplicationReason - the replication reason tdf to use when inserting the player into the map.  Can be nullptr.
        \return false 
                      1) the player already exists in the map 
                      2) the player has an invalid player state, can't add to categorized list
    ***************************************************************************************************/
    bool PlayerRosterMaster::addNewPlayer(PlayerInfoMaster* newPlayerInfo, const GameSetupReason &playerJoinContext)
    {
        // Check if the player already exists in the map
        PlayerId playerId = newPlayerInfo->getPlayerId();

        PlayerInfoMaster *foundPlayer = getPlayer(playerId);
        if (foundPlayer != nullptr)
        {
            // player already in the game.
            WARN_LOG("[PlayerRosterMaster] attempt to add a player(" << playerId << ") in state " 
                     << PlayerStateToString(foundPlayer->getPlayerState()) << " that already exists");
            return false;
        }

        // if enabled, must ensure 1st active player becomes admin
        getOwningGameSession()->updateAutomaticAdminSelection(*newPlayerInfo);
        
        // add player to game player map
        insertPlayerData(*newPlayerInfo, false);

        getOwningGameSession()->notifyExistingMembersOfJoiningPlayer(*newPlayerInfo, PLAYER_ROSTER, playerJoinContext);

        return true;
    }


    bool PlayerRosterMaster::addNewPlayerToQueue(PlayerInfoMaster& newPlayerInfo, const GameSetupReason& playerJoinContext)
    {
        // Check if the player already exists in the map
        PlayerId playerId = newPlayerInfo.getPlayerId();

        PlayerInfoMaster *foundPlayer = getQueuePlayer(playerId);
        if (foundPlayer != nullptr)
        {
            // player already in the game.
            WARN_LOG("[PlayerRosterMaster] attempt to add a player(" << playerId << ") in state " 
                     << PlayerStateToString(foundPlayer->getPlayerState()) << " that already exists");
            return false;
        }

        // slot id for queued players is used to represent your place in the queue.
        newPlayerInfo.setSlotId((SlotId)getQueueCount());

        // Queued players are not actually joining the game network
        newPlayerInfo.cancelJoinGameTimer();

        // add player to game queue map
        insertPlayerData(newPlayerInfo, true);


        getOwningGameSession()->notifyExistingMembersOfJoiningPlayer(newPlayerInfo, QUEUE_ROSTER, playerJoinContext);

        return true;
    }


    bool PlayerRosterMaster::addNewReservedPlayer(PlayerInfoMaster& newPlayerInfo, const GameSetupReason& context)
    {
        // must have the state of the new player set correctly before calling this.
        if (!newPlayerInfo.isReserved())
        {
            ERR_LOG("[PlayerRosterMaster] Unable to add non reserved player(" << newPlayerInfo.getPlayerId() << ") to game(" << getGameId() << ")");
            EA_FAIL();
            return false;
        }

        PlayerId playerId = newPlayerInfo.getPlayerId();
        if (getPlayer(playerId) != nullptr)
        {
            WARN_LOG("[PlayerRosterMaster] attempt to add reserved player(" << playerId << ") that is already exists.");
            return false;
        }

        // start reservation timer.
        newPlayerInfo.startReservationTimer();

        // add player to game player map
        insertPlayerData(newPlayerInfo, false);

        getOwningGameSession()->notifyExistingMembersOfJoiningPlayer(newPlayerInfo, PLAYER_ROSTER, context);

        TRACE_LOG("[PlayerRosterMaster] New Player(" << newPlayerInfo.getPlayerId() << ") joined roster as reserved for game(" << getGameId() << ").");

        return true;
    }


    /*! ************************************************************************************************/
    /*! \brief promote a queued player to the map, moving them from queued to active_connecting
           need to update the local roster lists by first removing, then adding back the player in the newer state
        \param[in] playerInfo - the Player info of the queued player.
        \param[in] playerReplicationReason - the replication reason tdf to use when inserting the player into the map.  Can be nullptr.
        \retrun false if player doesnt exist already, or player is not in the queued state
    ***************************************************************************************************/
    bool PlayerRosterMaster::promoteQueuedPlayer(PlayerInfoMaster *&playerInfo, const GameSetupReason &playerJoinContext)
    {
        // Check that the player already exists in the map
        PlayerId playerId = playerInfo->getPlayerId();

        if (getQueuePlayer(playerId) == nullptr)
        {
            WARN_LOG("[PlayerRosterMaster] Unable to find queued player(" << playerInfo->getPlayerId() << ") to add to game(" << getGameId() << ") roster.");
            return false;
        }

        // We create a copy of the player to move between replicated maps, as erasing from the replicated
        // map deletes the memory.
        PlayerInfoMasterPtr newPlayer = BLAZE_NEW PlayerInfoMaster(*playerInfo);

        // We only attempt to go active connecting if we were not reserved in the queue
        // (and thus reserved in the roster now)
        if (newPlayer->getPlayerState() != RESERVED)
        {
            if (hasConnectedPlayersOnConnection(playerInfo->getConnectionGroupId()))
            {
                newPlayer->setPlayerState(ACTIVE_CONNECTED);
                newPlayer->cancelJoinGameTimer();
            }
            else
            {
                newPlayer->setPlayerState(ACTIVE_CONNECTING);
                newPlayer->resetJoinGameTimer();
            }
        }
        else // player was reserved
        {
            newPlayer->startReservationTimer();
        }

        // if enabled, must ensure 1st active player becomes admin
        getOwningGameSession()->updateAutomaticAdminSelection(*newPlayer);

        // side: in older Xbox One 2014 MP supported versions of Blaze, we prevented queued players from joining ext sessions, until
        // they dequeued here. This is no longer required for Xbox One's 2015 MP (see now-removed fix GOS-28244, and GOS-28245).

        // cache off before we delete.
        SlotId queueIndexWhenPromoted = playerInfo->getQueuedIndexWhenPromoted();
        bool isExternalPlayer = (playerInfo->hasExternalPlayerId() &&
            !UserSession::isValidUserSessionId(playerInfo->getPlayerSessionId()) &&
            gGameManagerMaster->eraseFromExternalPlayerToGameIdsMap(playerInfo->getUserInfo().getUserInfo(), getGameId()));

        // Remove from queue map.
        erasePlayerData(*playerInfo);

        // Ensure re-add to external players tracking map as needed.
        if (isExternalPlayer)
            gGameManagerMaster->insertIntoExternalPlayerToGameIdsMap(newPlayer->getUserInfo().getUserInfo(), getGameId());

        // add to roster map.
        insertPlayerData(*newPlayer, false);

        //send out a notification about the player being promoted
        NotifyPlayerJoining notifyPlayerJoining;
        notifyPlayerJoining.setGameId(getGameId());
        GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();

        // build out our session ids before we decide which notification to send
        // sessionIdSetIdentityFuncProtectedIP contains the session ids that won't get a notification w/ masked IP addresses
        SessionIdSetIdentityFunc sessionIdSetIdentityFuncProtectedIP;
        // sessionIdSetIdentityFuncForUnprotectedIP contains the session ids that won't get a notification w/ unmasked IP address
        SessionIdSetIdentityFunc sessionIdSetIdentityFuncForUnprotectedIP;
        getOwningGameSession()->chooseProtectedIPRecipients(*newPlayer, sessionIdSetIdentityFuncProtectedIP, sessionIdSetIdentityFuncForUnprotectedIP);

        if (getOwningGameSession()->enableProtectedIP())
        {
            newPlayer->fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer(), true);
            gGameManagerMaster->sendNotifyPlayerPromotedFromQueueToUserSessionsById(itPair.begin(false), itPair.end(), sessionIdSetIdentityFuncProtectedIP, &notifyPlayerJoining);

            //Add address back when sending to a host or same-platform players
            newPlayer->getNetworkAddress()->copyInto(notifyPlayerJoining.getJoiningPlayer().getNetworkAddress());

            if (getOwningGameSession()->enablePartialProtectedIP())
            {
                //Add address back when sending to a host or same-platform players
                gGameManagerMaster->sendNotifyPlayerPromotedFromQueueToUserSessionsById(itPair.begin(true), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
            }
            else // dedicated server or CCS_MODE_HOSTED_ONLY
            {
                gGameManagerMaster->sendNotifyPlayerJoiningToUserSessionById(getOwningGameSession()->getDedicatedServerHostSessionId(), &notifyPlayerJoining);
            }
        }
        else // not protecting ip addresses
        {
            newPlayer->fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer());
            gGameManagerMaster->sendNotifyPlayerPromotedFromQueueToUserSessionsById(itPair.begin(), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
        }

        // Update remaining players in queue indexes.
        updateQueueIndexes(queueIndexWhenPromoted);

        // The memory pointed to by playerInfo passed into the function was deleted when we removed from the queue map.
        // Point playerInfo to the new player object.
        playerInfo = newPlayer.get();

        TRACE_LOG("[PlayerRosterMaster] Player(" << newPlayer->getPlayerId() << ") in state(" << 
            PlayerStateToString(newPlayer->getPlayerState()) << ") joined roster from queue for game(" << getGameId() << ").");

        // We send a full notify game setup as the player could have logged out and logged back in.
        // The reservation is only destroyed when it times out.
        NotifyGameSetup notifyGameSetup;
        // MM can't queue a player, so perform Qos validation is always false
        getOwningGameSession()->initNotifyGameSetup(notifyGameSetup, const_cast<GameSetupReason&>(playerJoinContext), false, newPlayer->getPlayerSessionId(), newPlayer->getConnectionGroupId(), newPlayer->getPlatformInfo().getClientPlatform());
        gGameManagerMaster->sendNotifyJoiningPlayerInitiateConnectionsToUserSessionById(newPlayer->getPlayerSessionId(), &notifyGameSetup);

        if (newPlayer->isConnected())
        {
            getOwningGameSession()->sendPlayerJoinCompleted(*newPlayer);
        }
        return true;
    }

   /*! ************************************************************************************************/
    /*! \brief demote a reserved player to the queue, moving them from reserved to queued
           need to update the local roster lists by first removing, then adding back the player in the newer state
        \param[in] playerInfo - the Player info of the queued player.
        \param[in] playerReplicationReason - the replication reason tdf to use when inserting the player into the map.  Can be nullptr.
        \retrun false if player doesnt exist already, or player is already in the queued state
    ***************************************************************************************************/
    bool PlayerRosterMaster::demotePlayerToQueue(PlayerInfoMaster *&playerInfo)
    {
        // Check that the player already exists in the map
        PlayerId playerId = playerInfo->getPlayerId();

        PlayerInfoMaster* demotedPlayer = getPlayer(playerId);
        if (demotedPlayer == nullptr)
        {
            WARN_LOG("[PlayerRosterMaster] Unable to find player(" << playerInfo->getPlayerId() << ") in game (" << getGameId() << ") to demote to queue.");
            return false;
        }
        if (!demotedPlayer->isReserved())
        {
            WARN_LOG("[PlayerRosterMaster] Player(" << playerInfo->getPlayerId() << ") in game (" << getGameId() << ") is not currently reserved. Unable demote to queue.");
            return false;
        }
        if (demotedPlayer->isInQueue())
        {
            WARN_LOG("[PlayerRosterMaster] Player(" << playerInfo->getPlayerId() << ") in game (" << getGameId() << ") is already in the queue. Nothing to demote.");
            return false;
        }

        // Cancel the timer, otherwise it'll cause issues when the player gets promoted from the queue
        playerInfo->cancelReservationTimer();

        // We create a copy of the player to move between replicated maps, as erasing from the replicated
        // map deletes the memory.
        PlayerInfoMasterPtr newPlayer = BLAZE_NEW PlayerInfoMaster(*playerInfo);
        erasePlayerData(*demotedPlayer);

        GameSetupReason playerJoinContext;
        playerJoinContext.switchActiveMember(GameSetupReason::MEMBER_DATALESSSETUPCONTEXT);
        playerJoinContext.getDatalessSetupContext()->setSetupContext(JOIN_GAME_SETUP_CONTEXT);

        // slot id for queued players is used to represent your place in the queue.
        newPlayer->setSlotId((SlotId)getQueueCount());

        // Queued players are not actually joining the game network
        newPlayer->cancelJoinGameTimer();

        // add player to game queue map
        insertPlayerData(*newPlayer, true);

        //send out a notification about the player being demoted  (We send this to the player who was demoted, as well)
        NotifyPlayerJoining notifyPlayerJoining;
        notifyPlayerJoining.setGameId(getGameId());
        GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();

        // build out our session ids before we decide which notification to send
        // sessionIdSetIdentityFuncProtectedIP contains the session ids that won't get a notification w/ masked IP addresses
        SessionIdSetIdentityFunc sessionIdSetIdentityFuncProtectedIP;
        // sessionIdSetIdentityFuncForUnprotectedIP contains the session ids that won't get a notification w/ unmasked IP address
        SessionIdSetIdentityFunc sessionIdSetIdentityFuncForUnprotectedIP;
        getOwningGameSession()->chooseProtectedIPRecipients(*newPlayer, sessionIdSetIdentityFuncProtectedIP, sessionIdSetIdentityFuncForUnprotectedIP);

        if (getOwningGameSession()->enableProtectedIP())
        {
            newPlayer->fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer(), true);
            gGameManagerMaster->sendNotifyPlayerDemotedToQueueToUserSessionsById(itPair.begin(false), itPair.end(), sessionIdSetIdentityFuncProtectedIP, &notifyPlayerJoining);

            //Add address back when sending to a host or same-platform players
            newPlayer->getNetworkAddress()->copyInto(notifyPlayerJoining.getJoiningPlayer().getNetworkAddress());

            if (getOwningGameSession()->enablePartialProtectedIP())
            {
                //Add address back when sending to a host or same-platform players
                gGameManagerMaster->sendNotifyPlayerDemotedToQueueToUserSessionsById(itPair.begin(true), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
            }
            else // dedicated server or CCS_MODE_HOSTED_ONLY
            {
                gGameManagerMaster->sendNotifyPlayerDemotedToQueueToUserSessionById(getOwningGameSession()->getDedicatedServerHostSessionId(), &notifyPlayerJoining);
            }
        }
        else // not protecting ip addresses
        {
            newPlayer->fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer());
            gGameManagerMaster->sendNotifyPlayerDemotedToQueueToUserSessionsById(itPair.begin(), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
        }

        return true;
    }


    void PlayerRosterMaster::updateQueueIndexes(SlotId removedQueueIndex)
    {
        PlayerInfoList playerList = getPlayers(QUEUED_PLAYERS);
        if (removedQueueIndex >= playerList.size())
        {
            return;
        }
        PlayerInfoList::iterator iter = playerList.begin();
        PlayerInfoList::iterator end = playerList.end();
        bool foundSomeone = false;
        eastl::advance(iter, removedQueueIndex);
        for (; iter != end; ++iter)
        {
            PlayerInfoMaster* queuedPlayer = static_cast<PlayerInfoMaster*>(*iter);
            // Double check in case our ordering has gone wrong.
            uint8_t currentQueuedIndex = queuedPlayer->getSlotId();
            if (currentQueuedIndex > removedQueueIndex)
            {
                queuedPlayer->setSlotId(currentQueuedIndex-1);
                updatePlayerData(*static_cast<PlayerInfoMaster*>(queuedPlayer));
                foundSomeone = true;
            }
            else
            {
                WARN_LOG("[PlayerRosterMaster] Queue indexes out of order cleaning up queue after removing index " << removedQueueIndex << ".");
            }
        }
        
        if (foundSomeone != false)
        {
            // Send the data to the  the client.
            NotifyQueueChanged notifyQueueChange;
            notifyQueueChange.setGameId(getGameId());
            PlayerRoster::PlayerInfoList queueRoster = getPlayers(PlayerRoster::QUEUED_PLAYERS);
            for (PlayerRoster::PlayerInfoList::iterator i = queueRoster.begin(), e = queueRoster.end(); i != e; ++i)
            {
                notifyQueueChange.getPlayerIdList().push_back((*i)->getPlayerId());
            }

            if (gGameManagerMaster->getConfig().getGameSession().getQueueChangeNotifications() == INGAME_AND_QUEUED)
            {
                GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();
                gGameManagerMaster->sendNotifyQueueChangedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &notifyQueueChange);
            }
            else 
            {
                gGameManagerMaster->sendNotifyQueueChangedToUserSessionsById( queueRoster.begin(), queueRoster.end(), PlayerInfo::SessionIdGetFunc(), &notifyQueueChange );
                gGameManagerMaster->sendNotifyQueueChangedToUserSessionById(getOwningGameSession()->getTopologyHostSessionId(), &notifyQueueChange );
            }
        }
    }

    void PlayerRosterMaster::populateQueueEvent(PlayerIdList& eventPlayerList) const
    {
        PlayerInfoList playerList = getPlayers(QUEUED_PLAYERS);
        PlayerInfoList::const_iterator iter = playerList.begin();
        PlayerInfoList::const_iterator end = playerList.end();

        for (; iter != end; ++iter)
        {
            const PlayerInfo* playerInfo = *iter;
            eventPlayerList.push_back(playerInfo->getPlayerId());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief promote a reserved player in the map to active_connecting.
           need to update the local roster lists by first removing, then adding back the player in the newer state
        \param[in] playerInfo - the Player info of the reserved player.
        \param[in] playerReplicationReason - the replication reason tdf to use when inserting the player into the map.  Can be nullptr.
        \return false if player doesn't exist already, or player is not in the reserved state
    ***************************************************************************************************/
    bool PlayerRosterMaster::promoteReservedPlayer(PlayerInfoMaster* playerInfo, const GameSetupReason &playerJoinContext, const PlayerState playerState, bool previousTeamIndexUnspecified /*= false*/)
    {
        if (playerInfo == nullptr)
        {
            WARN_LOG("[PlayerRosterMaster] Unable to claim reservation with no playerInfo.");
            return false;
        }
        // Check that the player already exists in the map
        PlayerId playerId = playerInfo->getPlayerId();

        // All reserved players should be in the RESERVED player state, regardless of which map they are in.
        if (!playerInfo->isReserved())
        {
            WARN_LOG("[PlayerRosterMaster] Unable to add reserved player(" << playerInfo->getPlayerId() << ") in state " 
                     << PlayerStateToString(playerInfo->getPlayerState()) << " to the game(" << getGameId() << ") roster.");
            return false;
        }

        playerInfo->cancelReservationTimer();

        // Reserved player was in the roster, they must be signaled to initiate client connections.
        if (playerInfo->isInRoster())
        {
            // HACK: The team index get's decremented when you delete the player from
            // the list, but the player was never part of a team so it shouldn't
            // be decremented.  Need to save it off so can put it on the correct
            // team when it comes off of the reserved list.
            // Is there a better way to do this?
            TeamIndex teamIndex = playerInfo->getTeamIndex();
            if (previousTeamIndexUnspecified)
            {
                playerInfo->setTeamIndex(UNSPECIFIED_TEAM_INDEX);
            }

            playerInfo->setPlayerState(playerState);
            if (playerState != ACTIVE_CONNECTED)
            {
                playerInfo->resetJoinGameTimer();
            }
            
            if (previousTeamIndexUnspecified)
            {
                playerInfo->setTeamIndex(teamIndex);
            }

            // if enabled, must ensure 1st active player becomes admin
            getOwningGameSession()->updateAutomaticAdminSelection(*playerInfo);

            updatePlayerData(*playerInfo);

            GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();

            //If the session claiming the reservation is different from the one used to make the reservation, unsubscribe the old from everyone and subscribe the new one to everyone.
            if (playerInfo->getOriginalPlayerSessionId() != playerInfo->getPlayerSessionId())
            {
                getOwningGameSession()->subscribePlayerRoster(playerInfo->getOriginalPlayerSessionId(), ACTION_REMOVE);
                getOwningGameSession()->subscribePlayerRoster(playerInfo->getPlayerSessionId(), ACTION_ADD);
            }

            //Send everyone but the joiner an update about the reservation claim
            bool performQosValidation = getOwningGameSession()->isPlayerInMatchmakingQosDataMap(playerId);
            NotifyPlayerJoining notifyPlayerJoining;
            notifyPlayerJoining.setGameId(getGameId());
            notifyPlayerJoining.setValidateQosForJoiningPlayer(performQosValidation);

            // build out our session ids before we decide which notification to send
            // sessionIdSetIdentityFuncProtectedIP contains the session ids that won't get a notification w/ masked IP addresses
            SessionIdSetIdentityFunc sessionIdSetIdentityFuncProtectedIP;
            // sessionIdSetIdentityFuncForUnprotectedIP contains the session ids that won't get a notification w/ unmasked IP address
            SessionIdSetIdentityFunc sessionIdSetIdentityFuncForUnprotectedIP;
            getOwningGameSession()->chooseProtectedIPRecipients(*playerInfo, sessionIdSetIdentityFuncProtectedIP, sessionIdSetIdentityFuncForUnprotectedIP);

            if (getOwningGameSession()->enableProtectedIP())
            {
                playerInfo->fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer(), true);
                gGameManagerMaster->sendNotifyPlayerClaimingReservationToUserSessionsById(itPair.begin(false), itPair.end(), sessionIdSetIdentityFuncProtectedIP, &notifyPlayerJoining);

                //Add address back when sending to a host or same-platform players
                playerInfo->getNetworkAddress()->copyInto(notifyPlayerJoining.getJoiningPlayer().getNetworkAddress());

                if (getOwningGameSession()->enablePartialProtectedIP())
                {
                    //Add address back when sending to a host or same-platform players
                    gGameManagerMaster->sendNotifyPlayerClaimingReservationToUserSessionsById(itPair.begin(true), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
                }
                else // dedicated server or CCS_MODE_HOSTED_ONLY
                {
                    gGameManagerMaster->sendNotifyPlayerClaimingReservationToUserSessionById(getOwningGameSession()->getDedicatedServerHostSessionId(), &notifyPlayerJoining);
                }
            }
            else // not protecting ip addresses
            {
                playerInfo->fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer());
                gGameManagerMaster->sendNotifyPlayerClaimingReservationToUserSessionsById(itPair.begin(), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
            }

            // We send a full notify game setup as the player could have logged out and logged back in.
            NotifyGameSetup notifyGameSetup;
            getOwningGameSession()->initNotifyGameSetup(notifyGameSetup, playerJoinContext, performQosValidation, playerInfo->getPlayerSessionId(), playerInfo->getConnectionGroupId(), playerInfo->getPlatformInfo().getClientPlatform());
            gGameManagerMaster->sendNotifyJoiningPlayerInitiateConnectionsToUserSessionById(playerInfo->getPlayerSessionId(), &notifyGameSetup);

            // If we're already connected (MLU) say that it's complete:
            if (playerInfo->isConnected())
            {
                getOwningGameSession()->sendPlayerJoinCompleted(*playerInfo);
            }
        }
        else if (playerInfo->isInQueue())
        {
            playerInfo->setPlayerState(QUEUED); // this player claimed a reservation and is now queued.

            updatePlayerData(*playerInfo);

            GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();
            //If the session claiming the reservation is different from the one used to make the reservation, unsubscribe the old from everyone and subscribe the new one to everyone.
            if (playerInfo->getOriginalPlayerSessionId() != playerInfo->getPlayerSessionId())
            {
                getOwningGameSession()->subscribePlayerRoster(playerInfo->getOriginalPlayerSessionId(), ACTION_REMOVE);
                getOwningGameSession()->subscribePlayerRoster(playerInfo->getPlayerSessionId(), ACTION_ADD);
            }

            //Send everyone but the joiner an update about the reservation claim
            NotifyPlayerJoining notifyPlayerJoining;
            notifyPlayerJoining.setGameId(getGameId());

            // build out our session ids before we decide which notification to send
            // sessionIdSetIdentityFuncProtectedIP contains the session ids that won't get a notification w/ masked IP addresses
            SessionIdSetIdentityFunc sessionIdSetIdentityFuncProtectedIP;
            // sessionIdSetIdentityFuncForUnprotectedIP contains the session ids that won't get a notification w/ unmasked IP address
            SessionIdSetIdentityFunc sessionIdSetIdentityFuncForUnprotectedIP;
            getOwningGameSession()->chooseProtectedIPRecipients(*playerInfo, sessionIdSetIdentityFuncProtectedIP, sessionIdSetIdentityFuncForUnprotectedIP);

            if (getOwningGameSession()->enableProtectedIP())
            {
                playerInfo->fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer(), true);
                gGameManagerMaster->sendNotifyPlayerClaimingReservationToUserSessionsById(itPair.begin(false), itPair.end(), sessionIdSetIdentityFuncProtectedIP, &notifyPlayerJoining);

                //Add address back when sending to a host or same-platform players
                playerInfo->getNetworkAddress()->copyInto(notifyPlayerJoining.getJoiningPlayer().getNetworkAddress());

                if (getOwningGameSession()->enablePartialProtectedIP())
                {
                    //Add address back when sending to a host or same-platform players
                    gGameManagerMaster->sendNotifyPlayerClaimingReservationToUserSessionsById(itPair.begin(true), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
                }
                else // dedicated server or CCS_MODE_HOSTED_ONLY
                {
                    gGameManagerMaster->sendNotifyPlayerClaimingReservationToUserSessionById(getOwningGameSession()->getDedicatedServerHostSessionId(), &notifyPlayerJoining);
                }
            }
            else // not protecting ip addresses
            {
                playerInfo->fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer());
                gGameManagerMaster->sendNotifyPlayerClaimingReservationToUserSessionsById(itPair.begin(), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
            }

            // We send a full notify game setup as the player could have logged out and logged back in.
            NotifyGameSetup notifyGameSetup;
            // qos test is false, since the user is queued
            getOwningGameSession()->initNotifyGameSetup(notifyGameSetup, playerJoinContext, false, playerInfo->getPlayerSessionId(), playerInfo->getConnectionGroupId(), playerInfo->getPlatformInfo().getClientPlatform());
            gGameManagerMaster->sendNotifyJoiningPlayerInitiateConnectionsToUserSessionById(playerInfo->getPlayerSessionId(), &notifyGameSetup);                

            // If we're already connected (MLU) say that it's complete:
            if (playerInfo->isConnected())
            {
                getOwningGameSession()->sendPlayerJoinCompleted(*playerInfo);
            }

        }
        else
        {
            ERR_LOG("[PlayerRosterMaster] Player(" << playerId << ") not found in roster or queue map to be able to claim reservation.");
            EA_FAIL();
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Removes player from the map based on the state. NOTE: this deletes the PlayerInfoMaster object
        
        \param[in,out] player  - player info pointer to be delete; set to nullptr 
        \param[in] playerRemovalContext - reason of the player being removed
    ***************************************************************************************************/
    void PlayerRosterMaster::deletePlayer(PlayerInfoMaster *&player, const PlayerRemovalContext &playerRemovalContext, bool lockedForPreferredJoins /*= false*/, bool isRemovingAllPlayers /*= false*/)
    {
        if ( player != nullptr )
        {
            //unsubscribe this guy from everyone else.                       
            if (player->getSessionExists())
            {
                getOwningGameSession()->subscribePlayerRoster(player->getPlayerSessionId(), ACTION_REMOVE);
            }

            GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();

            //fill out the client notification before we erase the player
            NotifyPlayerRemoved notify;
            notify.setGameId(getGameId());
            notify.setPlayerId(player->getPlayerId());
            notify.setPlayerRemovedReason(playerRemovalContext.mPlayerRemovedReason);
            notify.setPlayerRemovedTitleContext(playerRemovalContext.mPlayerRemovedTitleContext);
            notify.setLockedForPreferredJoins(lockedForPreferredJoins);
            gGameManagerMaster->sendNotifyPlayerRemovedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &notify);

            // cache platform and external session permission
            ClientPlatformType platform = player->getClientPlatform();
            bool hadExternalSessionPermission = player->getUserInfo().getHasExternalSessionJoinPermission();

            //fill out info for updating external sessions before we erase the player
            //no need to update external session here if member will be updated by slaves, or, by caller while removing all players as group leave.
            ExternalUserLeaveInfoList extUserInfoList;
            bool doExternalSession = player->getUserInfo().getHasExternalSessionJoinPermission() &&
                shouldLeaveExternalSessionOnDeleteMember(playerRemovalContext.mPlayerRemovedReason, player) &&
                !isRemovingAllPlayers;
            if (doExternalSession)
            {
                setExternalUserLeaveInfoFromUserSessionInfo(*extUserInfoList.pull_back(), player->getUserInfo());
            }

            // save off player's slot id for updating queue below, before deletion from map.
            const Blaze::SlotId departingPlayerSlotId = player->getSlotId();
            const bool inQueue = player->inQueue();

            // remove from the map
            erasePlayerData(*player);
            if (inQueue)
            {
                // no need to update the queue if the game is getting destroyed
                if (playerRemovalContext.mPlayerRemovedReason != GAME_DESTROYED)
                {
                    updateQueueIndexes(departingPlayerSlotId);
                }
            }

            player = nullptr;

            // Ensure removed from external session
            if (doExternalSession)
            {
                // We create a copy of external session params for the queued job, which later will be owned by GameManagerMasterImpl::handleLeaveExternalSession()
                LeaveGroupExternalSessionParametersPtr leaveGroupExternalSessonParams = BLAZE_NEW LeaveGroupExternalSessionParameters();
                getOwningGameSession()->getExternalSessionIdentification().copyInto(leaveGroupExternalSessonParams->getSessionIdentification());
                extUserInfoList.copyInto(leaveGroupExternalSessonParams->getExternalUserLeaveInfos());

                gGameManagerMaster->getExternalSessionJobQueue().queueFiberJob(gGameManagerMaster, &GameManagerMasterImpl::handleLeaveExternalSession, leaveGroupExternalSessonParams, "GameManagerMasterImpl::handleLeaveExternalSession");
            }

            // check if this was the last player for the platform 
            if (hadExternalSessionPermission && (getPlayerCount(platform) == 0))
            {
                auto byPlatIt = getOwningGameSession()->getGameDataMaster()->getTrackedExternalSessionMembers().find(platform);
                if (byPlatIt == getOwningGameSession()->getGameDataMaster()->getTrackedExternalSessionMembers().end())
                {
                    // we're out of players for the (non-TrackedExternalSessionMembers) platform, clear the externalsession data.
                    // Platforms needing tracked ext sess members are cleared in untrackExternalSessionMember() instead
                    getOwningGameSession()->clearExternalSessionIds(platform);
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief gets the player for a given id. Check both roster and the queue

        \param[in] playerId - identification of the player.
        \return a pointer to the player found, it will be nullptr if the player is not found
    ***************************************************************************************************/
    PlayerInfoMaster* PlayerRosterMaster::getPlayer(const PlayerId playerId) const
    {
        return static_cast<PlayerInfoMaster*>(PlayerRoster::getPlayer(playerId));
    }

    /*! ************************************************************************************************/
    /*! \brief gets the player for a given id. Check both roster and the queue. This overload allows
            also looking in the external players of the roster. Use this overload, during join
            game flow, to check for a pre-existing external player object

        \param[in] playerId - BlazeId of player. Tries searching by this in regular non-external players roster first.
        \param[in] externalPlayerExternalId - If search by playerId in regular roster doesn't find player, tries
            searching the external players of roster, for the player having this external id.
        \return a pointer to the player found, it will be nullptr if the player is not found
    ***************************************************************************************************/
    PlayerInfoMaster* PlayerRosterMaster::getPlayer(const PlayerId playerId, const PlatformInfo& platformInfo) const
    {
        // check non external players
        PlayerInfoMaster* playerInfo = getPlayer(playerId);
        if (playerInfo != nullptr)
        {
            return playerInfo;
        }

        // check external players
        return getExternalPlayer(platformInfo);
    }

    /*! ************************************************************************************************/
    /*! \brief gets the player in the roster for a given id.

        \param[in] playerId - identification of the player.
        \return a pointer to the player found, it will be nullptr if the player is not found
    ***************************************************************************************************/
    PlayerInfoMaster* PlayerRosterMaster::getRosterPlayer(const PlayerId playerId) const
    {
        PlayerInfo* playerInfo = PlayerRoster::getPlayer(playerId);
        if (playerInfo != nullptr && playerInfo->isInRoster())
            return static_cast<PlayerInfoMaster*>(playerInfo); // master can down-cast to PlayerInfoMaster (which we stored in the map).
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief gets the queued player for a given id.
    
        \param[in] playerId - identification of the player.
        \return a pointer to the player found.  nullptr, if player not found.
    ***************************************************************************************************/
    PlayerInfoMaster* PlayerRosterMaster::getQueuePlayer(const PlayerId playerId) const
    {
        PlayerInfo* playerInfo = PlayerRoster::getPlayer(playerId);
        if (playerInfo != nullptr && playerInfo->isInQueue())
            return static_cast<PlayerInfoMaster*>(playerInfo); // master can down-cast to PlayerInfoMaster (which we stored in the map).
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief gets the external player's object for a given external id.
    
        \param[in] externalId - identification of the player.
        \return a pointer to the player found.  nullptr, if player not found.
    ***************************************************************************************************/
    Blaze::GameManager::PlayerInfoMaster* PlayerRosterMaster::getExternalPlayer(const PlatformInfo& platformInfo) const
    {
        for (auto playerInfo : mPlayerInfoMap)
        {
            PlayerInfoMaster* player = static_cast<PlayerInfoMaster*>(playerInfo.second.get());

            // Skip players that have no ids that can be used for external players, or players that actually exist: 
            if (!player->hasExternalPlayerId() || player->getSessionExists())
                continue;

            if (player->getPlatformInfo().getClientPlatform() != platformInfo.getClientPlatform())
                continue;

            switch (platformInfo.getClientPlatform())
            {
            case xone:
            case xbsx:
                if (platformInfo.getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID && platformInfo.getExternalIds().getXblAccountId() == player->getPlatformInfo().getExternalIds().getXblAccountId())
                    return player;
                break;
            case ps4:
            case ps5:
                if (platformInfo.getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID && platformInfo.getExternalIds().getPsnAccountId() == player->getPlatformInfo().getExternalIds().getPsnAccountId())
                    return player;
                break;
            case nx:
                if (!platformInfo.getExternalIds().getSwitchIdAsTdfString().empty() && platformInfo.getExternalIds().getSwitchIdAsTdfString() == player->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString())
                    return player;
                break;
            case pc:
                if (platformInfo.getEaIds().getOriginPersonaId() != INVALID_ORIGIN_PERSONA_ID && platformInfo.getEaIds().getOriginPersonaId() == player->getPlatformInfo().getEaIds().getOriginPersonaId())
                    return player;
                break;
            case steam:
                if (platformInfo.getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID && platformInfo.getExternalIds().getSteamAccountId() == player->getPlatformInfo().getExternalIds().getSteamAccountId())
                    return player;
                break;
            case stadia:
                if (platformInfo.getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID && platformInfo.getExternalIds().getStadiaAccountId() == player->getPlatformInfo().getExternalIds().getStadiaAccountId())
                    return player;
                break;
            default:
                break;
            }
        }
        
        return nullptr;
    }


    /*! ************************************************************************************************/
    /*! \brief changes all of the fully connected players (ACTIVE_CONNECTED) into ACTIVE_CONNECTING
    */
    void PlayerRosterMaster::initPlayerStatesForHostReinjection()
    {
        PlayerInfoList playerList = getPlayers(ACTIVE_CONNECTED_PLAYERS);
        // change all fully connected members into active_migrating members (excluding the new host)
        PlayerRoster::PlayerInfoList::iterator iter = playerList.begin();
        PlayerRoster::PlayerInfoList::iterator end = playerList.end();
        for (; iter != end; ++iter)
        {
            PlayerInfoMaster *currentMember = static_cast<PlayerInfoMaster*>(*iter);

            currentMember->setPlayerState(ACTIVE_CONNECTING);
        }

        playerList.clear();
        PlayerState state = ACTIVE_KICK_PENDING;
        PlayerRoster::getPlayers(playerList, &state);
        // at the beginning of host migration, pending kick timers should be canceled
        // change all fully connected members into active_migrating players (excluding the new host & their connection group)
        PlayerRoster::PlayerInfoList::iterator pkIter = playerList.begin();
        PlayerRoster::PlayerInfoList::iterator pkEnd = playerList.end();
        for (; pkIter != pkEnd; ++pkIter)
        {

            PlayerInfoMaster *currentMember = static_cast<PlayerInfoMaster*>(*iter);

            currentMember->setPlayerState(ACTIVE_CONNECTING);

            currentMember->cancelPendingKickTimer();

        }

        resetConnectingMembersJoinTimer(); 
    }

    /*! ************************************************************************************************/
    /*! \brief changes all of the fully connected players (ACTIVE_CONNECTED) into ACTIVE_MIGRATING, except
            for the newHostPlayer (who remains ACTIVE_CONNECTED)
            also changes all of the pending kick members (ACTIVE_KICK_PENDING) to active migrating, again excepting the new host.
    
        \param[in] newHostPlayer - the new host player for this host migration
    ***************************************************************************************************/
    void PlayerRosterMaster::initPlayerStatesForHostMigrationStart(const PlayerInfo& newHostPlayer)
    {
        PlayerInfoList playerList = getPlayers(ACTIVE_CONNECTED_PLAYERS);
        // change all fully connected members into active_migrating members (excluding the new host)
        PlayerRoster::PlayerInfoList::iterator iter = playerList.begin();
        PlayerRoster::PlayerInfoList::iterator end = playerList.end();
        for (; iter != end; ++iter)
        {
            PlayerInfoMaster *currentMember = static_cast<PlayerInfoMaster*>(*iter);
            if (currentMember->getConnectionGroupId() != newHostPlayer.getConnectionGroupId())
            {
                // move the player to the active migrating list and change his state
                currentMember->setPlayerState(ACTIVE_MIGRATING);
            }
            else
            {
                if ( currentMember != &newHostPlayer )
                {
                    // conn group member
                    currentMember->setPlayerState(ACTIVE_CONNECTED);
                }
            }
        }

        playerList.clear();
        PlayerState state = ACTIVE_KICK_PENDING;
        PlayerRoster::getPlayers(playerList, &state);
        // at the beginning of host migration, pending kick timers should be canceled
        // change all fully connected members into active_migrating players (excluding the new host & their connection group)
        PlayerRoster::PlayerInfoList::iterator pkIter = playerList.begin();
        PlayerRoster::PlayerInfoList::iterator pkEnd = playerList.end();
        for (; pkIter != pkEnd; ++pkIter)
        {
            PlayerInfoMaster *currentMember = static_cast<PlayerInfoMaster*>(*pkIter);
            if ( currentMember != &newHostPlayer )
            {
                // move the player to the active migrating list and change his state
                // also cancel the kick timer
                if (currentMember->getConnectionGroupId() != newHostPlayer.getConnectionGroupId())
                {
                    currentMember->setPlayerState(ACTIVE_MIGRATING);
                }
                else
                {
                    currentMember->setPlayerState(ACTIVE_CONNECTED);
                }
                currentMember->cancelPendingKickTimer();
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Resets the join game timer for all players currently ACTIVE_CONNECTING
    ***************************************************************************************************/
    void PlayerRosterMaster::resetConnectingMembersJoinTimer()
    {
        PlayerInfoList playerList = getPlayers(ACTIVE_CONNECTING_PLAYERS);
        PlayerRoster::PlayerInfoList::const_iterator iter = playerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator end = playerList.end();
        for ( ; iter != end; ++iter )
        {
            PlayerInfoMaster *playerInfo = static_cast<PlayerInfoMaster*>(*iter);
            playerInfo->resetJoinGameTimer();
        }
    }

    /*! ************************************************************************************************/
    /*! \brief update player's state to indicated new state. 
        NOTE: this can invalidate a pointer to a roster iterator

    \param[in] playerInfo       - playerInfo of the player
    \param[in] newState - newstate of the player
    \return BlazeRpcError 
    ***************************************************************************************************/
    BlazeRpcError PlayerRosterMaster::updatePlayerStateNoReplication(PlayerInfoMaster* player, const PlayerState newState)
    {
        if (EA_UNLIKELY(!player))
        {
            WARN_LOG("PlayerRosterMaster::updatePlayerState - Tried to update state for an invalid player for game: " << getGameId() << ".");
            return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }

        PlayerState playerState = player->getPlayerState();
        if ( newState != playerState)
        {
           TRACE_LOG("[PlayerRosterMaster] updating player(" << player->getPlayerId() << ") in state " 
                     << PlayerStateToString(player->getPlayerState()) << " to state " << PlayerStateToString(newState) 
                     << " in game(" << getGameId() << ")");

            // change player state
            player->setPlayerState(newState);
        }

        return ERR_OK;
    }


    /*! ************************************************************************************************/
    /*! \brief update player's state to indicated new state and replicates the update. 
        NOTE: this can invalidate a pointer to a roster iterator

        \param[in] playerInfo       - playerInfo of the player
        \param[in] newState - newstate of the player
        \return BlazeRpcError 
    ***************************************************************************************************/
    BlazeRpcError PlayerRosterMaster::updatePlayerState(PlayerInfoMaster* player, const PlayerState newState)
    {
        PlayerState playerState = player->getPlayerState();

        BlazeRpcError updateErr = updatePlayerStateNoReplication(player, newState);
        if (updateErr != ERR_OK)
        {
            return updateErr;
        }

        if ( newState != playerState)
        {
            updatePlayerData(*player);

            NotifyGamePlayerStateChange nStateChange;
            nStateChange.setGameId(getGameId());
            nStateChange.setPlayerId(player->getPlayerId());
            nStateChange.setPlayerState(player->getPlayerState());
            GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();
            gGameManagerMaster->sendNotifyGamePlayerStateChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nStateChange);
        }

        return ERR_OK;
    }
    
    BlazeRpcError PlayerRosterMaster::updatePlayerTeamRoleAndSlot(PlayerInfoMaster* player, const TeamIndex newTeamIndex, const RoleName& newRoleName, SlotType newSlotType)
    {
        if (EA_UNLIKELY(!player))
        {
            WARN_LOG("PlayerRosterMaster::updatePlayerTeamById - Tried to update state for an invalid player for game: " << getGameId() << ".");
            return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }

        if (isParticipantSlot(newSlotType))
        {
            if ((newTeamIndex >= getOwningGameSession()->getTeamIds().size()) && !((newTeamIndex == UNSPECIFIED_TEAM_INDEX) && player->isInQueue()))
            {
                ERR_LOG("PlayerRosterMaster::updatePlayerTeamById - attempting to change player(" << player->getPlayerId() 
                        << ") in team index (" << player->getTeamIndex() << ") to invalid team index (" 
                        << newTeamIndex << ") in game(" << getGameId() << ")");
                return GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
            }
        }
        // the roster doesn't know what roles are allowed, so we depend on the game session to validate that the role is allowable/not full

        TeamIndex previousTeamIndex = player->getTeamIndex();
        const RoleName &previousRoleName = player->getRoleName();
        SlotType previousSlotType = player->getSlotType();

        if ((newTeamIndex != previousTeamIndex) || (blaze_stricmp(newRoleName, previousRoleName) != 0) || previousSlotType != newSlotType)
        {
            TRACE_LOG("[PlayerRosterMaster] changing player(" << player->getPlayerId() << ") in team index (" << previousTeamIndex 
                      << ") to team index (" << newTeamIndex << ") in game(" << getGameId() << ")");

            if (isParticipantSlot(newSlotType))
            {
                player->setTeamIndex(newTeamIndex);
                player->setRoleName(newRoleName);
            }
            else
            {
                player->setTeamIndex(UNSPECIFIED_TEAM_INDEX);
                player->setRoleName(PLAYER_ROLE_NAME_DEFAULT);
            }
            player->setSlotType(newSlotType);

            updatePlayerData(*player);

            NotifyGamePlayerTeamRoleSlotChange nTeamChange;
            nTeamChange.setGameId(getGameId());
            nTeamChange.setPlayerId(player->getPlayerId());
            nTeamChange.setTeamIndex(newTeamIndex);
            nTeamChange.setPlayerRole(newRoleName);
            nTeamChange.setSlotType(newSlotType);
            GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();
            gGameManagerMaster->sendNotifyGamePlayerTeamRoleSlotChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nTeamChange);
        }

        return ERR_OK;
    }

    BlazeRpcError PlayerRosterMaster::updatePlayerTeamIndexForCapacityChange(PlayerInfoMaster* player, TeamIndex newTeamIndex)
    {
        if (EA_UNLIKELY(!player))
        {
            WARN_LOG("[PlayerRosterMaster::updatePlayerTeamIndexForCapacityChange] - Tried to update state for an invalid player for game: " << getGameId() << ".");
            return GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }

        if (newTeamIndex >= getOwningGameSession()->getTeamIds().size())
        {
            ERR_LOG("[PlayerRosterMaster::updatePlayerTeamIndexForCapacityChange] - attempting to change player(" << player->getPlayerId() 
                << ") in team index (" << player->getTeamIndex() << ") to invalid team index (" 
                << newTeamIndex << ") in game(" << getGameId() << ")");
            return GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
        }

        TeamIndex previousTeamIndex = player->getTeamIndex();

        if ( newTeamIndex != previousTeamIndex)
        {
            TRACE_LOG("[PlayerRosterMaster::updatePlayerTeamIndexForCapacityChange] changing player(" << player->getPlayerId() << ") in team index (" << previousTeamIndex 
                << ") to team index (" << newTeamIndex << ") in game(" << getGameId() << ")");

            player->setTeamIndex(newTeamIndex);

            // skip updating team counts, as they're already up-to date

            updatePlayerData(*player);
        }

        return ERR_OK;
    }


    /*! ************************************************************************************************/
    /*! \brief create a new player based on indicated information

        \param[in] playerId    - user player id 
        \param[in] playerState - the state for the new player (active, reserved)
        \param[in] slotType    - the slot type for the new player
        \param[in] teamIndex   - the team index for the new player
        \param[in] roleName    - the role for the new player
        \param[in] session     - user session who would like to create the new player, could be nullptr(reserved player)
        \param[out]newPlayer   - player created or found in the mGamePlayerMap if it already exists
        \return - BlazeRpcError ERR_OK if player is created, 
    ***************************************************************************************************/
    Blaze::BlazeRpcError PlayerRosterMaster::createNewPlayer( const PlayerId playerId, const PlayerState playerState, SlotType slotType,
        TeamIndex teamIndex, const RoleName &roleName, const UserGroupId& groupId, const UserSessionInfo& playerInfo, const char8_t* encryptedBlazeId, PlayerInfoMasterPtr& newPlayer )
    {
        EA_ASSERT(newPlayer == nullptr);
        newPlayer = getPlayer(playerId, playerInfo.getUserInfo().getPlatformInfo());
        if (newPlayer != nullptr)
        {
            // player with indicated player id already exist in game
            eastl::string platformInfoStr;
            WARN_LOG("[PlayerRosterMaster] Player(id=" << playerId << ", platformInfo=(" << platformInfoToString(playerInfo.getUserInfo().getPlatformInfo(), platformInfoStr) <<")) failed to be created for game(" << getGameId() << "), as it was already a member.");
            return GAMEMANAGER_ERR_ALREADY_GAME_MEMBER;
        }
        else
        {
            newPlayer = BLAZE_NEW PlayerInfoMaster(*getOwningGameSession(), playerState, slotType, teamIndex, roleName, groupId, playerInfo, encryptedBlazeId);
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief create a new player place holder object for a user who is not currently logged into the blazeserver.
            Such players always RESERVED, and do not have user sessions
    ***************************************************************************************************/
    BlazeRpcError PlayerRosterMaster::createExternalPlayer(SlotType slotType, TeamIndex teamIndex, const char8_t *role,
        const UserGroupId& groupId, const UserSessionInfo& userSetupInfo, const char8_t* encryptedBlazeId, PlayerInfoMasterPtr& newPlayer)
    {
        BlazeId negativeBlazeId = userSetupInfo.getUserInfo().getId();
        newPlayer = getPlayer(negativeBlazeId, userSetupInfo.getUserInfo().getPlatformInfo());
        if (newPlayer != nullptr)
        {
            // player with indicated player id already exist in game
            eastl::string platformInfoStr;
            WARN_LOG("[PlayerRosterMaster].createExternalPlayer Player(id=" << negativeBlazeId << ", platformInfo=(" << platformInfoToString(userSetupInfo.getUserInfo().getPlatformInfo(), platformInfoStr) <<
                ")) failed to be created for game(" << getGameId() << "), as it was already a member.");
            return GAMEMANAGER_ERR_ALREADY_GAME_MEMBER;
        }

        if(!role)
        {
            ERR_LOG("[PlayerInfoMaster].createExternalPlayer Unable to find the role for player '" << userSetupInfo.getUserInfo().getId() << "' in join request when joining game session(" << getGameId() << ").");
            return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
        }

        if (getOwningGameSession() == nullptr)
        {
            BLAZE_ASSERT_LOG(GameManagerMaster::LOGGING_CATEGORY, "[PlayerInfoMaster].createExternalPlayer Internal error: Unable to find game session for player '" << userSetupInfo.getUserInfo().getId() << "' in join request when joining game session.");
            return ERR_SYSTEM;
        }

        newPlayer = BLAZE_NEW PlayerInfoMaster(*getOwningGameSession(), RESERVED, slotType, teamIndex,
            role, groupId, userSetupInfo, encryptedBlazeId);

        // Track this so blaze can replace this with a 'real' player object, once the user session is available.
        gGameManagerMaster->insertIntoExternalPlayerToGameIdsMap(userSetupInfo.getUserInfo(), getGameId());

        return ERR_OK;
    }

    PlayerInfoMaster* PlayerRosterMaster::getFirstEligibleNextAdmin(const PlayerInfoList& playerList, PlayerId oldAdminId, ConnectionGroupId connectionGroupToSkip) const
    {
        for (PlayerInfoList::const_iterator it = playerList.begin(), itend = playerList.end(); it != itend; ++it)
        {
            if (!isEligibleToBeNextAdmin((*it)->getPlayerId(), oldAdminId, connectionGroupToSkip))
            {
                continue;
            }
            return static_cast<PlayerInfoMaster*>(*it);
        }
        return nullptr;
    }

    bool PlayerRosterMaster::isEligibleToBeNextAdmin(PlayerId newAdminCandidateId, PlayerId oldAdminId, ConnectionGroupId connectionGroupToSkip) const
    {
        if ((newAdminCandidateId == oldAdminId) || newAdminCandidateId < 0) //A player with negative id is a guest
        {
            return false;
        }

        if (connectionGroupToSkip != INVALID_CONNECTION_GROUP_ID)
        {
            PlayerInfo* newAdmin = PlayerRoster::getPlayer(newAdminCandidateId);
            if (newAdmin != nullptr)
            {
                if (connectionGroupToSkip == newAdmin->getConnectionGroupId())
                {
                    return false;
                }
            }
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief trigger admin migration, pick the player with hosting permissions in the match the longest to be next admin
        
         Note: 
         1) for dedicated server game, we trigger the migration when the last non-host admin is removed 
            from admin list; for non-dedicated server game, we trigger the migration when the last admin is 
            removed from admin list
         2) we pick the next admin based on the mJoinedGameTimestamp in each participantInfo, the participant with earliest
            time value will be the one we want
         3) spectators are not eligible to be picked automatically as an admin
         4) Guests are not eligible to be picked as an admin.  

        \ return the player picked as next admin
    ***************************************************************************************************/
    PlayerInfoMaster* PlayerRosterMaster::pickPossHostByTime(PlayerId oldPlayerId, ConnectionGroupId connectionGroupToSkip)
    {
        PlayerInfoMaster* nextAdmin = nullptr;
        
        // We try to pick a player who has been connected longest and has the right permissions.
        Blaze::TimeValue minTimeValue = TimeValue::getTimeOfDay();
        PlayerInfoList playerList = getPlayers(ACTIVE_CONNECTED_PLAYERS_WITH_HOST_PERMISSION);
        for (PlayerInfoList::const_iterator it = playerList.begin(), itend = playerList.end(); it != itend; ++it)
        {
            if (!isEligibleToBeNextAdmin((*it)->getPlayerId(), oldPlayerId, connectionGroupToSkip))
            {
                continue;
            }
            PlayerInfoMaster* participantInfo = static_cast<PlayerInfoMaster*>(*it);
            Blaze::TimeValue playerTimeValue = participantInfo->getJoinedGameTimestamp();
            if (minTimeValue > playerTimeValue)
            {
                nextAdmin = participantInfo;
                minTimeValue = playerTimeValue;
            }
        }

        if (nextAdmin != nullptr)
        {
            return nextAdmin;
        }

        // NOTE: we are no longer sorting by join game start up time with the selections below.  
        // Long term we may want to look at going through the entire "active" list and picking
        // the longest in game player.

        // Return a connecting player, since they are allowed to become a new host.  This prevents
        // the game from being left with no admins. We search entire list as their can be multiple 
        // players in connecting state and some may not be entitled to become an admin (for example, guests).
        playerList = getPlayers(ACTIVE_CONNECTING_PLAYERS_WITH_HOST_PERMISSION);
        if ((nextAdmin = getFirstEligibleNextAdmin(playerList, oldPlayerId, connectionGroupToSkip)) != nullptr)
        {
            return nextAdmin;
        }

        // Now try a migrating player, since they may also become the new host.
        playerList = getPlayers(ACTIVE_MIGRATING_PLAYERS_WITH_HOST_PERMISSION);
        if ((nextAdmin = getFirstEligibleNextAdmin(playerList, oldPlayerId, connectionGroupToSkip)) != nullptr)
        {
            return nextAdmin;
        }


        // Last ditch effort, try a kick pending player, as they may re-establish once host migration is done.
        playerList.clear();
        PlayerState state = ACTIVE_KICK_PENDING;
        bool canHost = true;
        PlayerRoster::getPlayers(playerList, &state, &canHost);
        if ((nextAdmin = getFirstEligibleNextAdmin(playerList, oldPlayerId, connectionGroupToSkip)) != nullptr)
        {
            return nextAdmin;
        }

        // If nullptr by now, we just don't add an admin.
        return nextAdmin; 
    }

    bool PlayerRosterMaster::hasPlayersOnConnection(const ConnectionGroupId connGroupId) const
    {
        if (connGroupId != 0)
        {
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (player->isInRoster() && (player->getConnectionGroupId() == connGroupId) && !player->isReserved())
                    return true;
            }
        }
        return false;
    }

    bool PlayerRosterMaster::hasConnectedPlayersOnConnection(const ConnectionGroupId connGroupId) const
    {
        if (connGroupId != 0)
        {
            for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
            {
                PlayerInfo* player = i->second.get();
                if (player->isInRoster() && player->getConnectionGroupId() == connGroupId && player->getPlayerState() == ACTIVE_CONNECTED)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool PlayerRosterMaster::hasActiveMigratingPlayers() const
    {
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster() && player->getPlayerState() == ACTIVE_MIGRATING)
            {
                return true;
            }
        }
        return false;
    }

    bool PlayerRosterMaster::hasQueuedPlayers() const
    {
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (!player->isInRoster())
            {
                return true;
            }
        }
        return false;
    }

    bool PlayerRosterMaster::hasRosterPlayers() const
    {
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfo* player = i->second.get();
            if (player->isInRoster())
            {
                return true;
            }
        }
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief Removes the user's old place holder player's object from the game and in its place adds
            a newly created player object.
        NOTE: this deletes the player's old external player's object
        
        \param[in] newUser  - new user session info, used to create the new player to add.
        \return the created new player object. nullptr if failed to create
    ***************************************************************************************************/
    Blaze::GameManager::PlayerInfoMaster* PlayerRosterMaster::replaceExternalPlayer(UserSessionId newSessionId,
        const BlazeId newBlazeId, const PlatformInfo& newPlatformInfo, const Locale newAccountLocale, const uint32_t newAccountCountry,
        const char8_t* serviceName, const char8_t* productName)
    {
        eastl::string newPlatformInfoStr;
        PlayerInfoMaster* oldPlayer = getExternalPlayer(newPlatformInfo);
        if (oldPlayer == nullptr)
        {
            ERR_LOG("[PlayerRosterMaster].replaceExternalPlayer internal error, attempted transfer reservation to player which was not found in external player map. Failed to transfer to new user(id=" <<
                newBlazeId << ", platformInfo=(" << platformInfoToString(newPlatformInfo, newPlatformInfoStr) << ")), for game(" << getGameId() << ")");
            return nullptr;
        }
        eastl::string oldPlatformInfoStr;

        // Note if user comes in with different external session relevant values for its new user session here, we will try upserting/refreshing its
        // external session data at follow up claim. If this means it should no longer *be* in the external session (e.g. no join external session permissions)
        // its Blaze join claim will fail to claim the external session's reservation, which will simply expire, to ensure its removed. Just log msg here about the changes.
        eastl::string extSessionMsg;
        bool oldExtSessPermission = oldPlayer->getUserInfo().getHasExternalSessionJoinPermission();

        TRACE_LOG("[PlayerRosterMaster].replaceExternalPlayer old player object for external user(id=" << oldPlayer->getPlayerId() << ", platformInfo=(" << platformInfoToString(oldPlayer->getPlatformInfo(), oldPlatformInfoStr) <<
            "))  ->  new player object for user(id=" << newBlazeId << ", platformInfo=(" << platformInfoToString(newPlatformInfo, newPlatformInfoStr) << ")), for game(" << getGameId() << "). " <<
            (extSessionMsg.sprintf(" Note permission to join external sessions original value %s, new value to be checked at claim.", (oldExtSessPermission? "true":"false")).c_str()));

        // store off whether the player object we're replacing is in queue
        const bool isQueued = oldPlayer->isInQueue();

        //fill out the client notification before we erase the player
        NotifyPlayerRemoved notify;
        notify.setGameId(getGameId());
        notify.setPlayerId(oldPlayer->getPlayerId());
        notify.setPlayerRemovedReason(RESERVATION_TRANSFER_TO_NEW_USER);
        notify.setLockedForPreferredJoins(false);
        GameSessionMaster::SessionIdIteratorPair itPair = getOwningGameSession()->getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyPlayerRemovedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &notify);

        // create a copy of the old external player object, updated with the new user session data
        PlayerInfoMasterPtr newPlayer = BLAZE_NEW PlayerInfoMaster(*oldPlayer);
        ValidationUtils::setJoinUserInfoFromSessionExistence(newPlayer->getUserInfo(), newSessionId, newBlazeId, newPlatformInfo, newAccountLocale, newAccountCountry, serviceName, productName);

        // First remove the old place holder player object. We propagate the remove all the way down 
        // the chain to slaves and clients, to ensure any mappings by the old temporary negative
        // id are cleared, before adding new replacement player with new valid blaze id below.
        GameSetupReason joinContext;
        joinContext.switchActiveMember(GameSetupReason::MEMBER_INDIRECTJOINGAMESETUPCONTEXT);
        joinContext.getIndirectJoinGameSetupContext()->setRequiresClientVersionCheck(gGameManagerMaster->getConfig().getGameSession().getEvaluateGameProtocolVersionString());

        // Note the reservation timer for the new player object is started *after* the old player object is destroyed below.
        // This avoids the old player object's dtor inadvertently canceling the new player object's timer (which may be keyed by same BlazeId-GameId pair)
        TimeValue origReservationStart = oldPlayer->getReservationTimerStart();
        erasePlayerData(*oldPlayer);

        // start reservation timeout, for the new player object, based off orig start time.
        if (!isQueued)
        {
            newPlayer->cancelReservationTimer();
            newPlayer->startReservationTimer(origReservationStart);
        }
        insertPlayerData(*newPlayer, isQueued);

        getOwningGameSession()->notifyExistingMembersOfJoiningPlayer(*newPlayer, (isQueued? QUEUE_ROSTER : PLAYER_ROSTER), joinContext);
        gGameManagerMaster->incrementExternalReplacedMetric();

        return newPlayer.get();
    }

    // return whether to ensure user removed from external session at deletion, based on PlayerRemoveReason
    bool PlayerRosterMaster::shouldLeaveExternalSessionOnDeleteMember(PlayerRemovedReason reason, const PlayerInfoMaster* player) const
    {
        PlayerId id = 0;
        eastl::string platformInfoStr;
        if (player != nullptr)
            id = player->getPlayerId();

        switch (reason)
        {
        case PLAYER_LEFT:
        case PLAYER_LEFT_CANCELLED_MATCHMAKING:
        case PLAYER_LEFT_SWITCHED_GAME_SESSION:
        case GROUP_LEFT:
        case PLAYER_KICKED:
        case PLAYER_KICKED_WITH_BAN:
        case PLAYER_KICKED_CONN_UNRESPONSIVE:
        case PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN:
        case PLAYER_KICKED_POOR_CONNECTION:
        case PLAYER_KICKED_POOR_CONNECTION_WITH_BAN:
        case PLAYER_JOIN_EXTERNAL_SESSION_FAILED:
        case RESERVATION_TRANSFER_TO_NEW_USER:
            TRACE_LOG("[PlayerRosterMaster].shouldLeaveExternalSessionOnDeleteMember: not removing player " << id << " (platformInfo: " << (player == nullptr ? "unknown" : platformInfoToString(player->getPlatformInfo(), platformInfoStr)) << ") at erase notification, for reason '" << PlayerRemovedReasonToString(reason) << "', for game id " << getGameId());
            return false;
        default:
            TRACE_LOG("[PlayerRosterMaster].shouldLeaveExternalSessionOnDeleteMember: removing player " << id << " (platformInfo: " << (player == nullptr ? "unknown" : platformInfoToString(player->getPlatformInfo(), platformInfoStr)) << ") at erase notification, for reason '" << PlayerRemovedReasonToString(reason) << "', for game id " << getGameId());
            return true;
        }
    }

    void PlayerRosterMaster::updateQueuedPlayersDataOnPlayerRemove(PlayerId leavingPlayer)
    {
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfoMaster* player = static_cast<PlayerInfoMaster*>(i->second.get());
            if (!player->isInRoster() && player->getTargetPlayerId() == leavingPlayer)
            {
                // Queued player is queued for leaving players team - remove requirement
                player->setTargetPlayerId(INVALID_BLAZE_ID);
                player->setTeamIndex(UNSPECIFIED_TEAM_INDEX);
            }
        }
    }

    GameSessionMaster* PlayerRosterMaster::getOwningGameSession()
    {
        return static_cast<GameSessionMaster*>(mOwningGameSession);
    }

    GameId PlayerRosterMaster::getGameId() const
    {
        return mOwningGameSession->getGameId();
    }

    /*! ************************************************************************************************/
    /*! \brief insert the player data. Init join timestamp as needed.
    ***************************************************************************************************/
    void PlayerRosterMaster::insertPlayerData(PlayerInfoMaster& info, bool isQueued)
    {
        info.setInQueue(isQueued);
        
        // by spec at connect or enqueue set join time. Note: dequeues re-insert players to give em a fresh timestamp
        if (isQueued || info.getPlayerState() == ACTIVE_CONNECTED)
            info.setJoinedGameTimeStamp();

        PlayerRoster::insertPlayer(info);
        getOwningGameSession()->refreshDynamicPlatformList();
    }

    /*! ************************************************************************************************/
    /*! \brief update the player data. Init join timestamp as needed.
    ***************************************************************************************************/
    void PlayerRosterMaster::updatePlayerData(PlayerInfoMaster& info)
    {
        if (info.getPlayerState() == ACTIVE_CONNECTED)
            info.setJoinedGameTimeStamp();//sets if not set
    }

    void PlayerRosterMaster::erasePlayerData(PlayerInfoMaster& info)
    {
        PlayerId playerId = info.getPlayerId();
        GameId gameId = info.getGameId();

        if (PlayerRoster::erasePlayer(playerId))
        {
            eastl::string playerFieldName;
            playerFieldName.sprintf(PRIVATE_PLAYER_FIELD_FMT, playerId);
            gGameManagerMaster->getGameStorageTable().eraseField(gameId, playerFieldName.c_str());
            playerFieldName.sprintf(PUBLIC_PLAYER_FIELD_FMT, playerId);
            gGameManagerMaster->getGameStorageTable().eraseField(gameId, playerFieldName.c_str());

            getOwningGameSession()->refreshDynamicPlatformList();
        }
    }

    void PlayerRosterMaster::upsertPlayerFields()
    {
        for (PlayerInfoMap::const_iterator i = mPlayerInfoMap.begin(), e = mPlayerInfoMap.end(); i != e; ++i)
        {
            PlayerInfoMaster* player = static_cast<PlayerInfoMaster*>(i->second.get());
            PlayerId playerId = player->getPlayerId();
            GameId gameId = player->getGameId();
            StorageFieldName playerFieldName;
            PlayerDataMaster* playerDataMaster = player->getPlayerDataMaster();
            playerFieldName.sprintf(PRIVATE_PLAYER_FIELD_FMT, playerId);
            gGameManagerMaster->getGameStorageTable().upsertField(gameId, playerFieldName.c_str(), *playerDataMaster);

            ReplicatedGamePlayerServer* playerData = player->getPlayerData();
            playerFieldName.sprintf(PUBLIC_PLAYER_FIELD_FMT, playerId);
            gGameManagerMaster->getGameStorageTable().upsertField(gameId, playerFieldName.c_str(), *playerData);
        }
    }

} // namespace GameManager
} // namespace Blaze

