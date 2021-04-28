/*! ************************************************************************************************/
/*!
    \file gamenetworkbehavior.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/event/eventmanager.h"
#include "gamemanager/gamenetworkbehavior.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/endpointsconnectionmesh.h"

#include "gamemanager/rpc/gamemanagermetricsmaster.h"

namespace Blaze
{
namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief process the common part of updateFullMeshConnection/updateHostedMeshConnection request.

        \param[in] request - an updateMeshConnection request (contains a list of clients to update)
        \param[in] gameSessionMaster - the game the player is in
        \return true if no further processing is needed. false otherwise.
    ***************************************************************************************************/
    bool GameNetworkBehavior::updateMeshConnectionCommon(const UpdateMeshConnectionMasterRequest &request, GameSessionMaster* gameSessionMaster)
    {
        EndPointsConnectionMesh* playerConnectionMesh = gameSessionMaster->getPlayerConnectionMesh();

        ConnectionGroupId sourceConnectionGroupId = request.getSourceConnectionGroupId();
        ConnectionGroupId targetConnectionGroupId = request.getTargetConnectionGroupId();

        if (sourceConnectionGroupId == targetConnectionGroupId)
        {
            // check if we should bypass the join complete notifications for MM qos validation
            // if this is the 'host' player, we may need to update mm qos data, do that here
            if ((gameSessionMaster->getGameState() == CONNECTION_VERIFICATION) && (gameSessionMaster->getTopologyHostInfo().getConnectionGroupId() == sourceConnectionGroupId))
            {
                // if this is the 'host' client, since its active connected by default, we'll just decrement its MM QoS tracking refcount here (we won't hit processMeshConnectionEstablished() to do it later).
                // Note: its ok doing it here, we won't prematurely do final QoS validation without having received all pending QoS measurements because we still wait for everyone
                // in its match to get active connected first. See getFullMeshConnectionStatus(), updateMatchmakingQosData() and setConnectionStatus() for details.

                const PlayerNetConnectionStatus status = CONNECTED;
                if (updateConnGroupMatchmakingQosData(sourceConnectionGroupId, gameSessionMaster, &status))
                {
                    TRACE_LOG("[GameNetworkBehavior] Update mesh request for 'host' set connection status for self, updating matchmaking QoS data."); 
                }
            }
        }

        if (!shouldEndpointsConnect(gameSessionMaster, sourceConnectionGroupId, targetConnectionGroupId))
        {
            return true; // skip, because endpoints should not connect 
        }
        // update the peer connection status within the mesh (uni-directional)
        if (!playerConnectionMesh->setConnectionStatus(sourceConnectionGroupId, targetConnectionGroupId, request.getPlayerNetConnectionStatus(), request.getQosInfo().getLatencyMs(), request.getQosInfo().getPacketLoss()))
        {
            return true;
        }
        if (!playerConnectionMesh->setConnectionFlags(sourceConnectionGroupId, targetConnectionGroupId, request.getPlayerNetConnectionFlags()))
        {
            return true;
        }
        
        if (!isConnectionRequired(gameSessionMaster, sourceConnectionGroupId, targetConnectionGroupId))
        {
            return true; // skip further processing, because endpoints connection status is non-impacting.
        }
        // nothing special happened during this update
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief process an updateMeshConnection request for a 'full mesh peer to peer' network topologies.
            If a player's connection to the active players was established or lost,
            take appropriate action (activate/kick player, or trigger host migration).

        \param[in] request - an updateMeshConnection request (contains a list of clients to update)
        \param[in] gameSessionMaster - the game the player is in
        \param[in] sourcePlayerId - the id of the user who sent the mesh update
        \return ERR_OK on success
    ***************************************************************************************************/
    BlazeRpcError GameNetworkBehavior::updateFullMeshConnection(const UpdateMeshConnectionMasterRequest &request, 
        GameSessionMaster *gameSessionMaster)
    {
        if (!gameSessionMaster->isNetworkFullMesh())
        {
            FAIL_LOG("[GameNetworkBehavior::updateFullMeshConnection]: Function only intended for use with full mesh topologies (gameid = " << gameSessionMaster->getGameId() << ")")
            return Blaze::ERR_SYSTEM;
        }
        
        bool done = updateMeshConnectionCommon(request, gameSessionMaster);
        if (done)
            return Blaze::ERR_OK;

        EndPointsConnectionMesh* playerConnectionMesh = gameSessionMaster->getPlayerConnectionMesh();
        ConnectionGroupId sourceConnectionGroupId = request.getSourceConnectionGroupId();
        ConnectionGroupId targetConnectionGroupId = request.getTargetConnectionGroupId();

       
        // Determine if this update made the source or target fully connected to the mesh (complete/break/fail a bidirectional connection)
        // full mesh games require all players to be fully bidirectionally connected to active players

        // When we encounter one player on a network disconnect, target -> source needs to be checked first since
        // the remaining player in the game (source) is the one sending us the notification. Checking target's
        // connection first will show that it is no longer connected to the mesh and trigger the timeout.
        // Since it's a full mesh, all other occasions should not matter when comparing one before the other.

        // target -> entire mesh
        PlayerNetConnectionStatus targetPlayerMeshStatus = playerConnectionMesh->getFullMeshConnectionStatus(targetConnectionGroupId, *gameSessionMaster);
        if (!processMeshConnectionStateChange(targetConnectionGroupId, gameSessionMaster, targetPlayerMeshStatus, sourceConnectionGroupId))
        {
            // game was destroyed due to player's connection state change, gameSessionMaster is invalid now
            return Blaze::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE;
        }

        // source -> entire mesh
        PlayerNetConnectionStatus sourcePlayerMeshStatus = playerConnectionMesh->getFullMeshConnectionStatus(sourceConnectionGroupId, *gameSessionMaster);
        if (!processMeshConnectionStateChange(sourceConnectionGroupId, gameSessionMaster, sourcePlayerMeshStatus, sourceConnectionGroupId))
        {
            // game was destroyed due to player's connection state change, gameSessionMaster is invalid now
            return Blaze::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE;
        }

        // If the player was not already marked as disconnected from the entire mesh above,
        // see if the majority of players think they are disconnected from them and boot the target player.
        if (request.getPlayerNetConnectionStatus() == DISCONNECTED)
        {
            if (!processMajorityRulesOnDisconnect(targetConnectionGroupId, gameSessionMaster))
            {
                return Blaze::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE;
            }
        }

        // nothing special happened during this update
        return Blaze::ERR_OK;
    }


    /*! ************************************************************************************************/
    /*!
        \brief process an updateMeshConnection request for one of the 'hosted' network topologies
        (p2p partial, or client/server).  If a player's connection to the host was established or lost,
        take appropriate action (activate/kick player, or trigger host migration).

        Note: p2p partial is nearly identical to client/server, EXCEPT that C/S will warn if connecting
        to a non-host.  (p2p partial will connect to non hosts w/o issue).

        \param[in] request - an updateMeshConnection request (contains a list of clients to update)
        \param[in] gameSessionMaster - the game the player is in
        \param[in] sourcePlayerId - the player id of the player updating the mesh status
        \return ERR_OK on success
    ***************************************************************************************************/
    BlazeRpcError GameNetworkBehavior::updateHostedMeshConnection(const UpdateMeshConnectionMasterRequest &request,
                                                                                           GameSessionMaster* gameSessionMaster)
    {
        if (gameSessionMaster->isNetworkFullMesh())
        {
            FAIL_LOG("[GameNetworkBehavior::updateHostedMeshConnection]: Function only intended for use with hosted topologies (gameid = " << gameSessionMaster->getGameId() << ")")
            return Blaze::ERR_SYSTEM;
        }

        bool done = updateMeshConnectionCommon(request, gameSessionMaster);
        if (done)
            return Blaze::ERR_OK;

        EndPointsConnectionMesh* playerConnectionMesh = gameSessionMaster->getPlayerConnectionMesh();
        ConnectionGroupId sourceConnectionGroupId = request.getSourceConnectionGroupId();
        ConnectionGroupId targetConnectionGroupId = request.getTargetConnectionGroupId();

        PlayerId hostPlayerId = gameSessionMaster->getTopologyHostInfo().getPlayerId();
        ConnectionGroupId hostConnectionGroupId = gameSessionMaster->getTopologyHostInfo().getConnectionGroupId();

        TRACE_LOG("[GameNetworkBehavior] Updated hosted mesh connection in game(" << gameSessionMaster->getGameId() << ") hosted by(" 
            << hostPlayerId << ":conngroup=" << hostConnectionGroupId << "), client(" << sourceConnectionGroupId << ") -> client(" << targetConnectionGroupId << ") now " 
            << PlayerNetConnectionStatusToString(request.getPlayerNetConnectionStatus()) << ".");

        if ( targetConnectionGroupId == hostConnectionGroupId )
        {
             PlayerNetConnectionStatus sourcePlayerMeshStatus = playerConnectionMesh->getHostedMeshConnectionStatus(sourceConnectionGroupId, hostConnectionGroupId);
             if (!processMeshConnectionStateChange(sourceConnectionGroupId, gameSessionMaster, sourcePlayerMeshStatus, sourceConnectionGroupId))
             {
                 // game was destroyed due to player's connection state change, gameSessionMaster is invalid now
                 return Blaze::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE;
             }

             if((gameSessionMaster->getGameNetworkTopology() != CLIENT_SERVER_DEDICATED) && (request.getPlayerNetConnectionStatus() == DISCONNECTED))
             {
                 if (!processMajorityRulesOnDisconnect(targetConnectionGroupId, gameSessionMaster))
                 {
                     return Blaze::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE;
                 }
             }
        }
                 
        // source (host) -> target
        if (sourceConnectionGroupId == hostConnectionGroupId)
        {
            PlayerNetConnectionStatus targetPlayerMeshStatus = playerConnectionMesh->getHostedMeshConnectionStatus(targetConnectionGroupId, hostConnectionGroupId);
            if (!processMeshConnectionStateChange(targetConnectionGroupId, gameSessionMaster, targetPlayerMeshStatus, sourceConnectionGroupId))
            {
                // game was destroyed due to player's connection state change, gameSessionMaster is invalid now
                return Blaze::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE;
            }
        }

        // nothing special happened during this update
        return Blaze::ERR_OK;
    }

    void GameNetworkBehavior::getHostedMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const
    {
        // function only intended for use with hosted topologies (client server or partial mesh)
        if (game.isNetworkFullMesh())
        {
            FAIL_LOG("[GameNetworkBehavior::getHostedMeshQos]: Function only intended for use with hosted topologies (gameid = " << game.getGameId() << ")")
            return;
        }

        const EndPointsConnectionMesh* connectionMesh = game.getPlayerConnectionMesh();

        if (connectionGroupId == game.getTopologyHostInfo().getConnectionGroupId())
        {
            // host gets the mesh qos, since the topology host should have a connection to all members
            connectionMesh->getFullMeshConnectionQos(connectionGroupId, game, playerQos);
        }
        else
        {
            connectionMesh->getHostedMeshConnectionQos(connectionGroupId, game, playerQos);
        }
        
    }

    void GameNetworkBehavior::getFullMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const
    {
        const EndPointsConnectionMesh* connectionMesh = game.getPlayerConnectionMesh();
        connectionMesh->getFullMeshConnectionQos(connectionGroupId, game, playerQos);
    }

    /*! ************************************************************************************************/
    /*! \brief Detect/handle changes to a player's overall connection state to a game's network mesh.
                Finalizes joining players, triggers host migration, and kick players who fail to connect.

        If a connection is fully established, the player is activated (and their join is completed).
        If a connection was lost, the player is either kicked, or host migration is triggered.

        \param[in] connectionGroupId - the connectionGroupId who just gained (or lost) connection
        \param[in] gameSessionMaster - the game the player is in
        \param[in] meshConnectionStatus - the overall connection status between the player and the game's network mesh
        \param[in] reportingConnectionGroupId - the connection group id of the player reporting the action.
        \return returns false if the game was destroyed as a side effect of the connection state change (destroyed due to player removal)
    ***************************************************************************************************/
    bool GameNetworkBehavior::processMeshConnectionStateChange(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, 
        PlayerNetConnectionStatus meshConnectionStatus, ConnectionGroupId reportingConnectionGroupId)
    {
        // Player was no longer in the game's roster.
        GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator playerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupId);
        PlayerIdList* connectionGroupIdPlayerList = (playerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? playerIdListItr->second : nullptr;

        if (connectionGroupIdPlayerList == nullptr)
        {
            return true;
        }

        PlayerIdList::const_iterator itr = connectionGroupIdPlayerList->begin();
        PlayerIdList::const_iterator endItr = connectionGroupIdPlayerList->end();
        
        bool areAllPlayersReserved = true;
        bool areAllPlayerConnected = true;

        for (; itr != endItr; ++itr)
        {
            PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(*itr);
            if (playerInfo == nullptr)
            {
                continue;
            }
            if (playerInfo->getPlayerState() != RESERVED)
            {
                areAllPlayersReserved = false;
            }

            // we're checking the connection group's previous connection state against the new connection state update.
            if (!playerInfo->isConnected())
            {
                areAllPlayerConnected = false;
            }
        }

        if (areAllPlayersReserved)
        {
            return true;
        }

        // check if we just established a connection (and need to finish the player's gameSessionMaster join)
        bool connectionEstablished = ((meshConnectionStatus == CONNECTED) && (!areAllPlayerConnected) );
        if (connectionEstablished)
        {
            processMeshConnectionEstablished(connectionGroupId, gameSessionMaster);
            return true; // game is still valid
        }

        bool shouldRemoveConnectionGroup = (meshConnectionStatus == DISCONNECTED);

        // check if we just lost an existing connection
        bool connectionLost = ( (meshConnectionStatus != CONNECTED) && (areAllPlayerConnected) );
        if (connectionLost)
        {
            shouldRemoveConnectionGroup = processMeshConnectionLost(connectionGroupId, gameSessionMaster, reportingConnectionGroupId);
        }

        // check if we've failed to connect (and given up trying)
        if (shouldRemoveConnectionGroup)
        {
            // boot the player, and her connection group (may trigger host migration or game destruction)
            itr = connectionGroupIdPlayerList->begin();
            for (; itr != endItr; ++itr)
            {
                RemovePlayerMasterRequest removePlayerRequest;
                removePlayerRequest.setGameId(gameSessionMaster->getGameId());
                removePlayerRequest.setPlayerId(*itr);
                if (connectionLost || gameSessionMaster->getGameState() == MIGRATING)
                {
                    removePlayerRequest.setPlayerRemovedReason(PLAYER_CONN_LOST);
                }
                else
                {
                    removePlayerRequest.setPlayerRemovedReason(PLAYER_JOIN_TIMEOUT);
                    const auto* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(*itr);
                    if (playerInfo != nullptr)
                    {
                        // must set for disconnect reservation check:
                        removePlayerRequest.setPlayerRemovedTitleContext(static_cast<PlayerRemovedTitleContext>(playerInfo->getPlayerDataMaster()->getGameEntryType()));
                    }
                }

                RemovePlayerMasterError::Error removePlayerError;
                if (gGameManagerMaster->removePlayer(&removePlayerRequest, *itr, removePlayerError) == GameManagerMasterImpl::REMOVE_PLAYER_GAME_DESTROYED)
                {
                    return false; // game was destroyed, game pointer is invalidated
                }
            }
        }
       
        return true; // game is still valid
    }

    /*! ************************************************************************************************/
    /*! \brief update a connection groups' (and the game's) status now that a new player is fully connected to the game's network mesh.
    
        \param[in] connectionGroupId who just established a full connection to the game's peer network mesh
        \param[in] gameSessionmaster the game
    ***************************************************************************************************/
    void GameNetworkBehavior::processMeshConnectionEstablished(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster)
    {
        GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator playerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupId);
        PlayerIdList* connectionGroupIdPlayerList = (playerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? playerIdListItr->second : nullptr;

        if ( connectionGroupIdPlayerList != nullptr)
        {
            PlayerIdList::iterator playerIdItr = connectionGroupIdPlayerList->begin();
            PlayerIdList::iterator playerIdEndItr = connectionGroupIdPlayerList->end();
            for (; playerIdItr != playerIdEndItr; ++playerIdItr)
            {
                
                PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(*playerIdItr);

                if (playerInfo == nullptr)
                {
                    continue;
                }

                // player established connection to host, change the status.
                PlayerState prevPlayerState = playerInfo->getPlayerState();

                // check if we should bypass the join complete notifications for MM qos validation
                if ((prevPlayerState == ACTIVE_CONNECTING) && gameSessionMaster->isPlayerInMatchmakingQosDataMap(playerInfo->getPlayerId()))
                {
                    const PlayerNetConnectionStatus status = CONNECTED;
                    if (gameSessionMaster->updateMatchmakingQosData(playerInfo->getPlayerId(), &status, playerInfo->getConnectionGroupId()))
                    {
                        playerInfo->cancelJoinGameTimer();
                        return;
                    }
                }

                // note: player info's joined game timestamp (used below) is init by this changePlayerState/updatePlayerData
                gameSessionMaster->changePlayerState(playerInfo, ACTIVE_CONNECTED);

                switch (prevPlayerState)
                {
                case ACTIVE_CONNECTING:
                    {
                        playerInfo->cancelJoinGameTimer();

                        // send join complete notification for the player.
                        gameSessionMaster->sendPlayerJoinCompleted(*playerInfo);
                        break;
                    }
                case ACTIVE_MIGRATING:
                    {
                        if (gameSessionMaster->isHostMigrationDone())
                        {
                            // last migrating player is now fully connected
                            TRACE_LOG("[GameNetworkBehavior.processMeshConnectionEstablished] host migration finish when last player connect");
                            gameSessionMaster->hostMigrationCompleted();
                        }
                        if (playerInfo->isJoinCompletePending())
                        {
                            playerInfo->cancelJoinGameTimer();

                            // send join complete notification for the player.
                            gameSessionMaster->sendPlayerJoinCompleted(*playerInfo);
                        }
                        break;
                    }
                case ACTIVE_KICK_PENDING:
                    {
                        playerInfo->cancelPendingKickTimer();
                        break;
                    }
                default:
                    break; // intentional no-op
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief handle a peer conn loss (from a previously fully connected player) to a DEDICATED SERVER game's network mesh.

        We trust the dedicated server, so we immediately kick a player who's lost connection to it.

        \param[in] playerInfo - the previously fully connected players
        \return true if caller should immediately remove the player from the game, false otherwise.
    ***************************************************************************************************/
    bool NetworkBehaviorDedicatedServer::processMeshConnectionLost(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, ConnectionGroupId reportingConnectionGroupId) const
    {
        GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator playerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupId);
        PlayerIdList* connectionGroupIdPlayerList = (playerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? playerIdListItr->second : nullptr;

        if ( connectionGroupIdPlayerList != nullptr)
        {
            PlayerIdList::iterator playerIdItr = connectionGroupIdPlayerList->begin();
            PlayerIdList::iterator playerIdEndItr = connectionGroupIdPlayerList->end();
            for (; playerIdItr != playerIdEndItr; ++playerIdItr)
            {
                // we trust the dedicated server, so we can immediately kick a player who's lost connection to it.
                RemovePlayerMasterRequest removePlayerRequest;
                removePlayerRequest.setGameId(gameSessionMaster->getGameId());
                removePlayerRequest.setPlayerId(*playerIdItr);
                removePlayerRequest.setPlayerRemovedReason(PLAYER_CONN_LOST);

                RemovePlayerMasterError::Error removePlayerError;
                GameManagerMasterImpl::RemovePlayerGameSideEffect sideEffect = gGameManagerMaster->removePlayer(&removePlayerRequest, *playerIdItr, removePlayerError);
                // we expect that removing a player from a dedicated server game may either cause no side effect or cause
                // a platform host migration. However, if the game is destroyed or some new possible side effect is added to
                // remove player, that may be an error, which we log here.
                if ( (sideEffect != GameManagerMasterImpl::REMOVE_PLAYER_NO_GAME_SIDE_EFFECT) && 
                    (sideEffect != GameManagerMasterImpl::REMOVE_PLAYER_MIGRATION_TRIGGERED) ) 
                {
                    ERR_LOG("NetworkBehaviorDedicatedServer::processConnectionLost - player removal triggered unintended side effect: RemovePlayerGameSideEffect: " 
                            << sideEffect << "!");
                }
            }
        }
        return false;//already removed above
    }

    /*! ************************************************************************************************/
    /*! \brief Determines if two players should have attempted connecting to one another

        \param[in] game - The game sessions
        \param[in] sourcePlayerId - The source playerId
        \param[in] targetPlayerId - The target playerId
    ***************************************************************************************************/
    bool NetworkBehaviorDedicatedServer::shouldEndpointsConnect(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const
    {
        return isConnectionRequired(game, sourceConnectionGroupId, targetConnectionGroupId) || (game->getVoipNetwork() == VOIP_PEER_TO_PEER);
    }

    /*! ************************************************************************************************/
    /*! \brief Determines if the connection between endpoints is necessary.

        \param[in] game - The game sessions
        \param[in] sourceConnectionGroupId - The source ConnectionGroupId
        \param[in] targetConnectionGroupId - The target targetConnectionGroupId
    ***************************************************************************************************/
    bool NetworkBehaviorDedicatedServer::isConnectionRequired(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const
    {
        ConnectionGroupId hostConnectionGroupId = game->getDedicatedServerHostInfo().getConnectionGroupId();
        if (sourceConnectionGroupId == hostConnectionGroupId || targetConnectionGroupId == hostConnectionGroupId)
            return true;

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief handle a peer conn loss (from a previously fully connected player) to a P2P FULL mesh game.
    
            Put this player in the "pending kick" state and wait for more info (ie: a blazeServer conn loss)
                before we can kick the player from the game.

        \param[in] playerInfo - one of the previously fully connected players
        \return true if caller should immediately remove the player from the game, false otherwise.
    ***************************************************************************************************/
    bool NetworkBehaviorFullMesh::processMeshConnectionLost(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, ConnectionGroupId reportingConnectionGroupId) const
    {

        ConnectionGroupId connectionGroupIdToKick = shouldKickHost(gameSessionMaster, reportingConnectionGroupId) ? gameSessionMaster->getTopologyHostInfo().getConnectionGroupId() : connectionGroupId;


        bool reporterIsSpectator = false;
        if(!isConnectionGroupSpectating(gameSessionMaster, connectionGroupIdToKick))
        {
            // if the player to kick isn't a spectator, make sure the reporter isn't a spectator
            // we will defer to the player, and set the kick timer on the spectator if a spectator reports a disconnect from a player
            if (isConnectionGroupSpectating(gameSessionMaster, reportingConnectionGroupId))
            {
                connectionGroupIdToKick = reportingConnectionGroupId;
                reporterIsSpectator = true;
            }
        }
       
        // In a full mesh with faster disconnects, we no longer set the timer on the reporting player.
        // However, we do set the timer if the reporter was a spectator
        if ((connectionGroupIdToKick != reportingConnectionGroupId) || reporterIsSpectator)
        {
            startPendingKickForConnectionGroup(gameSessionMaster, connectionGroupIdToKick);
            // returning false (and relying on pending kick timer) prevents caller from kicking us before majority rules checked.
            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief handle a peer conn loss (from a previously fully connected player) to the P2P PARTIAL or CS PEER HOSTED game's network mesh.
    
        \param[in] playerInfo - the previously fully connected players
        \return true if caller should immediately remove the player from the game, false otherwise.
    ***************************************************************************************************/
    bool GameNetworkBehavior::processMeshConnectionLost(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, ConnectionGroupId reportingConnectionGroupId) const
    {
        
        if (gameSessionMaster->getGameState() == MIGRATING)
        {
            GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator playerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupId);
            PlayerIdList* connectionGroupIdPlayerList = (playerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? playerIdListItr->second : nullptr;

            if ( connectionGroupIdPlayerList != nullptr)
            {
                PlayerIdList::iterator playerIdItr = connectionGroupIdPlayerList->begin();
                PlayerIdList::iterator playerIdEndItr = connectionGroupIdPlayerList->end();
                for (; playerIdItr != playerIdEndItr; ++playerIdItr)
                {
                    PlayerInfoMaster* playerInfo = gameSessionMaster->getPlayerRoster()->getPlayer(*playerIdItr);
                    if (playerInfo != nullptr)
                    {
                        gameSessionMaster->changePlayerState(playerInfo, ACTIVE_MIGRATING);
                    }
                }
            }
        }
        else
        {
            ConnectionGroupId connectionGroupIdToKick = connectionGroupId;
            if (!isConnectionGroupSpectating(gameSessionMaster, connectionGroupIdToKick) && isConnectionGroupSpectating(gameSessionMaster, reportingConnectionGroupId))
            {
                connectionGroupIdToKick = reportingConnectionGroupId;

                PlayerId hostId = gameSessionMaster->getTopologyHostInfo().getPlayerId();
                ConnectionGroupId hostConnectionGroupId = gameSessionMaster->getTopologyHostInfo().getConnectionGroupId();
                PlayerInfoMaster* hostToKick = gameSessionMaster->getPlayerRoster()->getPlayer(hostId);
                if (hostToKick && hostConnectionGroupId != reportingConnectionGroupId && !gameSessionMaster->isPingingHostConnection())
                {
                    UserSessionId hostSessionId = hostToKick->getUserInfo().getSessionId();
                    gameSessionMaster->setIsPingingHostConnection(true);
                    // NOTE: pingHostKickIfGone pins gameSessionMaster from being migrated until ping succeeds or fails
                    gSelector->scheduleFiberCall(const_cast<GameNetworkBehavior*>(this), &GameNetworkBehavior::pingHostKickIfGone, hostId, hostSessionId, gameSessionMaster, "GameNetworkBehavior::pingHostKickIfGone");
                }
            }

            startPendingKickForConnectionGroup(gameSessionMaster, connectionGroupIdToKick);
            // returning false (and relying on pending kick timer) prevents caller from kicking us before majority rules / ping host checked.
            return false;
        }
        
        return true;
    }

    void GameNetworkBehavior::pingHostKickIfGone(PlayerId hostPlayerId, UserSessionId hostSessionId, GameSessionMaster* game) const
    {
        // Pin the game to the pingHostKickIfGone fiber because we don't want to migrate the game while pinging the host
        // In the future we may want to allow this, but in order to do that we'd need to retry the pingHostKickIfGone
        // if getIsPingingHostConnection() == true on the new master after the game has been successfully imported.
        GameSessionMasterPtr gameSessionMaster = game;

        bool pingSuccess = gUserSessionManager->checkConnectivity(hostSessionId, gGameManagerMaster->getHostPendingKickTimeout());

        gameSessionMaster->setIsPingingHostConnection(false);

        if (pingSuccess)
            return;

        // In this case, the host timed out before responding, we therefore kick the host:  
        RemovePlayerMasterRequest removePlayerRequest;
        removePlayerRequest.setGameId(gameSessionMaster->getGameId());
        removePlayerRequest.setPlayerId(hostPlayerId);
        removePlayerRequest.setPlayerRemovedReason(PLAYER_CONN_LOST);

        RemovePlayerMasterError::Error removePlayerError;
        gGameManagerMaster->removePlayer(&removePlayerRequest, hostPlayerId, removePlayerError);

        if (removePlayerError != RemovePlayerMasterError::Error::ERR_OK)
        {
            WARN_LOG("[GameNetworkbehavior::pingHostKickIfGone] Error (" << removePlayerError << ") occurred while removing player(" << hostPlayerId << ") from game(" << gameSessionMaster->getGameId() << ") roster.");
        }
    }

    bool GameNetworkBehavior::shouldKickHost(GameSessionMaster* gameSessionMaster, ConnectionGroupId reportingConnectionGroupId) const
    {
        return (!gGameManagerMaster->getTrustHostForTwoPlayerPeerTimeout() && (gameSessionMaster->getPlayerRoster()->getRosterPlayerCount() == 2)
            && (reportingConnectionGroupId != gameSessionMaster->getTopologyHostInfo().getConnectionGroupId())
            && (gameSessionMaster->getPlayerRoster()->getPlayer(gameSessionMaster->getTopologyHostInfo().getPlayerId()) != nullptr));
    }

    bool GameNetworkBehavior::updateConnGroupMatchmakingQosData(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, const PlayerNetConnectionStatus* status)
    {
        GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator playerIdListItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupId);
        PlayerIdList* connectionGroupIdPlayerList = (playerIdListItr != gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? playerIdListItr->second : nullptr;

        if (connectionGroupIdPlayerList == nullptr)
        {
            return false;
        }

        bool result = true;
        PlayerIdList::const_iterator itr = connectionGroupIdPlayerList->begin();
        PlayerIdList::const_iterator endItr = connectionGroupIdPlayerList->end();
        for (; itr != endItr; itr++)
        {
            result = result && gameSessionMaster->updateMatchmakingQosData(*itr, status, connectionGroupId);
        }

        return result;
    }

    /*! ************************************************************************************************/
    /*! \brief after a player is removed from a full mesh p2p game, we need to see if his removal means other 
            (existing) players are now fully joined.
    ***************************************************************************************************/
    void NetworkBehaviorFullMesh::onActivePlayerRemoved(GameSessionMaster* gameSessionMaster)
    {
        if (!gameSessionMaster->isNetworkFullMesh())
        {
            FAIL_LOG("[NetworkBehaviorFullMesh::onActivePlayerRemoved]: Function only intended for use with full mesh topologies (gameid = " << gameSessionMaster->getGameId() << ")")
            return;
        }
        
        // if a fully connected player was removed, we must scan over the remaining connecting/migrating players to see
        //   if anyone connecting/migrating/pendingKick is now fully connected (after the loss of the removed player)

        // WARNING: we iterate over a COPY of the roster, since calling processMeshConnectionEstablished can update a player's status &
        //   invalidate a player roster iterator.
        const PlayerRoster::PlayerInfoList activePlayerList = gameSessionMaster->getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        PlayerRoster::PlayerInfoList::const_iterator activePlayerIter = activePlayerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator activePlayerEnd = activePlayerList.end();
        for ( ; activePlayerIter!=activePlayerEnd; ++activePlayerIter )
        {
            PlayerInfoMaster *existingPlayer = static_cast<PlayerInfoMaster*>(*activePlayerIter);

            // if the current active player is now fully connected (due to the removal of another fully connected player)
            if ( (!existingPlayer->isConnected()) && isPlayerConnectedToFullMesh(existingPlayer, gameSessionMaster))
            {
                processMeshConnectionEstablished(existingPlayer->getConnectionGroupId(), gameSessionMaster);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief For NETWORK_DISABLED only, if a player was added, cancel the join timer of the player. 
        The server considers all active players as connected since this topology does not use any networking,
        i.e. no updateMeshConnection will be sent up by connected players
    ***************************************************************************************************/
    void NetworkBehaviorDisabled::onActivePlayerJoined(GameSessionMaster* game, PlayerInfoMaster* player)
    {
        if (EA_UNLIKELY(player == nullptr))
        {
            TRACE_LOG("[NetworkBehaviorDisabled::onActivePlayerJoined: unexpected error - Player id (" << player->getPlayerId() << ") is not found in the game.");
            return;
        }

        PlayerState prevPlayerState = player->getPlayerState();
        game->changePlayerState(player, ACTIVE_CONNECTED);        

        if (prevPlayerState == ACTIVE_CONNECTING)
        {
            player->cancelJoinGameTimer();

            // send join complete notification for the player.
            game->sendPlayerJoinCompleted(*player);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief For P2P PARTIAL or CS PEER HOSTED, if player was removed, cancel any kick timer on host, 
        if there are no more players DISCONNECTED from the host, otherwise, let the kick timer play out.
    ***************************************************************************************************/
    void NetworkBehaviorPeerHosted::onActivePlayerRemoved(GameSessionMaster* gameSessionMaster)
    {
        PlayerInfoMaster* host = gameSessionMaster->getPlayerRoster()->getPlayer(gameSessionMaster->getTopologyHostInfo().getPlayerId());
        if (host != nullptr)
        {
            const EndPointsConnectionMesh* connectionMesh = gameSessionMaster->getPlayerConnectionMesh();
            const PlayerRoster::PlayerInfoList activePlayerList = gameSessionMaster->getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
            PlayerRoster::PlayerInfoList::const_iterator activePlayerIter = activePlayerList.begin();
            PlayerRoster::PlayerInfoList::const_iterator activePlayerEnd = activePlayerList.end();
            for ( ; activePlayerIter!=activePlayerEnd; ++activePlayerIter )
            {
                PlayerInfoMaster *existingPlayer = static_cast<PlayerInfoMaster*>(*activePlayerIter);
                if (connectionMesh->getHostedMeshConnectionStatus(existingPlayer->getConnectionGroupId(), host->getConnectionGroupId()) == DISCONNECTED )
                {
                    // found a disconnected player, take no action.
                    return;
                }
            }

            // If we got here, all players are either connecting or connected to the host.  Clear kick on host
            gameSessionMaster->changePlayerState(host, ACTIVE_CONNECTED);
            host->cancelPendingKickTimer();
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Determines if two players should have attempted connecting to one another

        \param[in] game - The game sessions
        \param[in] sourcePlayerId - The source playerId
        \param[in] targetPlayerId - The target playerId
    ***************************************************************************************************/
    bool NetworkBehaviorPeerHosted::shouldEndpointsConnect(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const
    {
        return isConnectionRequired(game, sourceConnectionGroupId, targetConnectionGroupId) || (game->getVoipNetwork() == VOIP_PEER_TO_PEER);
    }

    /*! ************************************************************************************************/
    /*! \brief Determines if the connection between endpoints is necessary.

        \param[in] game - The game sessions
        \param[in] sourceConnectionGroupId - The source ConnectionGroupId
        \param[in] targetConnectionGroupId - The target ConnectionGroupId
    ***************************************************************************************************/
    bool NetworkBehaviorPeerHosted::isConnectionRequired(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const
    {
        ConnectionGroupId hostConnectionGroupId = game->getTopologyHostInfo().getConnectionGroupId();
        if (sourceConnectionGroupId == hostConnectionGroupId || targetConnectionGroupId == hostConnectionGroupId)
            return true;

        return false;
    }
    
    /*! ************************************************************************************************/
    /*! \brief return true if the supplied player is fully connected to the game's peer network mesh.
    ***************************************************************************************************/
    bool GameNetworkBehavior::isPlayerConnectedToFullMesh(const PlayerInfo *player, const GameSessionMaster* game) const
    {
        if (!game->isNetworkFullMesh())
        {
            FAIL_LOG("[GameNetworkBehavior::isPlayerConnectedToFullMesh]: Function only intended for use with full mesh topologies (gameid = " << game->getGameId() << ")")
            return false;
        }

        if (player->isConnected())
        {
            return true;
        }

        // scan over the mesh; for a full mesh, we must be connected to every other connected member
        const EndPointsConnectionMesh* connectionMesh = game->getPlayerConnectionMesh();
        const PlayerRoster::PlayerInfoList &connectedPlayerList = game->getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_CONNECTED_PLAYERS);

        PlayerRoster::PlayerInfoList::const_iterator connectedPlayerIter = connectedPlayerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator connectedPlayerEnd = connectedPlayerList.end();
        for ( ; connectedPlayerIter!=connectedPlayerEnd; ++connectedPlayerIter )
        {
            PlayerInfoMaster *connectedPlayer = static_cast<PlayerInfoMaster*>(*connectedPlayerIter);

            if (!connectionMesh->areEndpointsConnected(player->getConnectionGroupId(), connectedPlayer->getConnectionGroupId()) )
            {
                return false;
            }
        }

        // note: if there are no active connected players, we return true (since you're always connected to yourself)
        return true;
    }


    /*! ************************************************************************************************/
    /*! \brief returns true if the supplied player is fully connected to the game's peer network mesh.
        \return returns false if the game was destroyed as a side effect of the connection state change 
            (destroyed due to player removal)
    ***************************************************************************************************/
    bool GameNetworkBehavior::processMajorityRulesOnDisconnect(ConnectionGroupId connectionGroupId, const GameSessionMaster* gameSessionMaster)
    {
        GameDataMaster::PlayerIdListByConnectionGroupIdMap::const_iterator playerIdListByConnectionGroupIdMapItr = gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupId);
        const PlayerIdList* connectionGroupIdPlayerList = nullptr;
        if (playerIdListByConnectionGroupIdMapItr == gameSessionMaster->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
        {
            return true;
        }
        else
        {
            connectionGroupIdPlayerList = playerIdListByConnectionGroupIdMapItr->second;
        }

        if (connectionGroupIdPlayerList == nullptr || connectionGroupIdPlayerList->empty())
        {
            return true;
        }

        // Don't bother with 2 mesh members or less, there is no majority.
        if (gameSessionMaster->getPlayerRoster()->getRosterPlayerCount() > 2)
        {
            // game is still around after updating mesh status, check if the player has 
            // lost connection to a majority of users in-game.  If so, remove him from the game session
            bool clientDisconnected = isClientDisconnectedFromMajorityOfMesh(connectionGroupId, gameSessionMaster);
            if (clientDisconnected)
            {
                PlayerIdList::const_iterator itr = connectionGroupIdPlayerList->begin();
                PlayerIdList::const_iterator endItr = connectionGroupIdPlayerList->end();
                for ( ; itr != endItr; ++itr)
                {
                    PlayerInfoMaster* player = gameSessionMaster->getPlayerRoster()->getPlayer(*itr);
                    if (player == nullptr)
                    {
                        continue;
                    }

                    TRACE_LOG("[GameNetworkBehavior] Updated hosted mesh connection in game(" << gameSessionMaster->getGameId() << ") hosted by(" 
                        << gameSessionMaster->getTopologyHostInfo().getPlayerId() << "), player(" << player->getPlayerId() 
                        << ") being removed due to a majority of players losing their connection.");

                    // boot the player (may trigger host migration or game destruction)
                    RemovePlayerMasterRequest removePlayerRequest;
                    removePlayerRequest.setGameId(gameSessionMaster->getGameId());
                    removePlayerRequest.setPlayerId(player->getPlayerId());
                    removePlayerRequest.setPlayerRemovedReason(PLAYER_CONN_LOST);

                    RemovePlayerMasterError::Error removePlayerError;
                    if (gGameManagerMaster->removePlayer(&removePlayerRequest, player->getPlayerId(), removePlayerError) == GameManagerMasterImpl::REMOVE_PLAYER_GAME_DESTROYED)
                    {
                        return false; // game was destroyed, game pointer is invalidated
                    }
                }
            }
        }

        return true;
    }


    /*! ************************************************************************************************/
    /*! \brief return true if the supplied player is disconnected from a majority of the game's peer network mesh.
    ***************************************************************************************************/
    bool GameNetworkBehavior::isClientDisconnectedFromMajorityOfMesh(ConnectionGroupId connectionGroupId, const GameSessionMaster* game) const
    {
        // scan over the mesh; for a full mesh, we must be connected to every other connected player
        const EndPointsConnectionMesh* connectionMesh = game->getPlayerConnectionMesh();
        const PlayerRoster::PlayerInfoList &activeMemberList = game->getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);

        uint32_t disconnectedPlayerCount = 0;
        const uint32_t activeMembers = activeMemberList.size();
        const uint32_t majoritySize = (activeMembers / 2) + 1; // losing fractional result intended

        PlayerRoster::PlayerInfoList::const_iterator activePlayerIter = activeMemberList.begin();
        PlayerRoster::PlayerInfoList::const_iterator activePlayerEnd = activeMemberList.end();
        for (; activePlayerIter != activePlayerEnd; ++activePlayerIter)
        {
            PlayerInfoMaster *activePlayer = static_cast<PlayerInfoMaster*>(*activePlayerIter);

            if (!connectionMesh->areEndpointsConnected(connectionGroupId, activePlayer->getConnectionGroupId()) )
            {
                disconnectedPlayerCount++;
            }
        }

        TRACE_LOG("[GameNetworkBehavior] In game(" << game->getGameId() << ") hosted by(" 
            << game->getTopologyHostInfo().getPlayerId() << "), of (" << activeMembers << "), (" << disconnectedPlayerCount << ") have lost their connection.");

        if (disconnectedPlayerCount >= majoritySize)
        {
            return true;
        }

        // note: if there are no active connected players, we return false (since you're always connected to yourself)
        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief starts pending kick timer on all players part of the connection group
    ***************************************************************************************************/
    void GameNetworkBehavior::startPendingKickForConnectionGroup(GameSessionMasterPtr gameSessionMasterPtr, ConnectionGroupId connectionGroupIdToKick) const
    {
        if (gameSessionMasterPtr != nullptr)
        {   
            GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator playerIdListByConnGroupIdMapItr = gameSessionMasterPtr->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupIdToKick);
            if (playerIdListByConnGroupIdMapItr == gameSessionMasterPtr->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
            {
                WARN_LOG("[GameNetworkBehavior].startPendingKickForConnectionGroup In game(" << gameSessionMasterPtr->getGameId() << ") connectionGroupIdToKick (" << connectionGroupIdToKick << ") does not exist");
                return;
            }

             PlayerIdList* playerIdList = playerIdListByConnGroupIdMapItr->second;
             if (playerIdList != nullptr)
             {
                 PlayerIdList::const_iterator playerIdListItr = playerIdList->begin();
                 for (; playerIdListItr != playerIdList->end(); ++playerIdListItr)
                 {
                     PlayerInfoMaster* player = gameSessionMasterPtr->getPlayerRoster()->getPlayer(*playerIdListItr);
                     if (player != nullptr)
                     {
                         player->startPendingKickTimer(gGameManagerMaster->getPendingKickTimeout());
                         gameSessionMasterPtr->changePlayerState(player, ACTIVE_KICK_PENDING);
                     }
                 }
             }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return true if all the players in the connection group are spectators, queued or reserved.
    ***************************************************************************************************/

    bool GameNetworkBehavior::isConnectionGroupSpectating(GameSessionMasterPtr gameSessionMasterPtr, ConnectionGroupId connectionGroupId) const
    {
        if (gameSessionMasterPtr != nullptr)
        {   
            GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator playerIdListByConnGroupIdMapItr = gameSessionMasterPtr->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupId);
            if (playerIdListByConnGroupIdMapItr == gameSessionMasterPtr->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
            {
                WARN_LOG("[GameNetworkBehavior].isConnectionGroupSpectating In game(" << gameSessionMasterPtr->getGameId() << ") connectionGroupIdToKick (" << connectionGroupId << ") does not exist");
                return true;
            }

            PlayerIdList* playerIdList = playerIdListByConnGroupIdMapItr->second;
            PlayerIdList::const_iterator playerIdListItr = playerIdList->begin();
            for (; playerIdListItr != playerIdList->end(); ++playerIdListItr)
            {
                PlayerInfoMaster* player = gameSessionMasterPtr->getPlayerRoster()->getPlayer(*playerIdListItr);
                if (player != nullptr && !player->isSpectator() && player->isActive())
                {
                    return false;
                }
            }
        }
        return true;
    }
} // namespace GameManager
} // namespace Blaze
