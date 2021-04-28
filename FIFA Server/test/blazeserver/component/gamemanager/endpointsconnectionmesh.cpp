/*! ************************************************************************************************/
/*!
    \file endpointsconnectionmesh.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/endpointsconnectionmesh.h"
#include "gamemanager/gamemanagermasterimpl.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/tdf/matchmaker_server.h"

#include "gamemanager/ccsutil.h"
#include "framework/util/networktopologycommoninfo.h"
#include "framework/tdf/networkaddress.h"

namespace Blaze 
{
namespace GameManager 
{

    EndPointsConnectionMesh::EndPointsConnectionMesh()
        : mGameSession(nullptr)
    {
    }

    /*! ************************************************************************************************/
    /*! \brief add a new endpoint to the mesh (in the ESTABLISHING_CONNECTION state)
    
        \param[in] playerId - the playerId of the new player.
        \param[in] rosterPlayer - the player endpoint is for. nullptr if endpoint is for a dedicated server host
    ***************************************************************************************************/
    void EndPointsConnectionMesh::addEndpoint(ConnectionGroupId newConnectionGroupId, PlayerId newPlayerId, ClientPlatformType newPlayerPlatform,
        ConnectionJoinType joinType, bool resetClientConnectionStatus /*= false*/)
    {
        GameDataMaster::ClientNetConnectionStatusMesh::mapped_type& newEndpointMap = mGameSession->getGameDataMaster()->getClientConnMesh()[newConnectionGroupId];
        PlayerIdList* newConnectionGroupIdPlayerList = mGameSession->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap()[newConnectionGroupId];
        
        bool addNewPlayer = true;
        if (newEndpointMap != nullptr && newConnectionGroupIdPlayerList != nullptr)
        {
            // In the case of MLU, we might be adding a new player from a client.
            PlayerIdList::iterator playerIdListItr = newConnectionGroupIdPlayerList->begin();
            PlayerIdList::iterator playerIdListEnd = newConnectionGroupIdPlayerList->end();
            for (; playerIdListItr!=playerIdListEnd; ++playerIdListItr)
            {
                if (*playerIdListItr == newPlayerId)
                {
                    //this would occur during host migration.
                    addNewPlayer = false;
                    break;  
                }
            }

            if (addNewPlayer)
            {
                newConnectionGroupIdPlayerList->push_back(newPlayerId);
                addNewPlayer = false;
            }
            
            if (!resetClientConnectionStatus)
            {
                return;
            }

        }
        else
        {
            EA_ASSERT_MSG(newEndpointMap == nullptr, "Endpoint map should be null");
            EA_ASSERT_MSG(newConnectionGroupIdPlayerList == nullptr, "PlayerId list of this Endpoint should be null");

            newEndpointMap = mGameSession->getGameDataMaster()->getClientConnMesh().allocate_element();
            newConnectionGroupIdPlayerList =  mGameSession->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().allocate_element();
        }

        if (addNewPlayer)
        {
            newConnectionGroupIdPlayerList->push_back(newPlayerId);
        }
        mGameSession->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap()[newConnectionGroupId] = newConnectionGroupIdPlayerList;

        // populate the new player's map with the existing mesh members (and vice-versa)
        GameDataMaster::ClientNetConnectionStatusMesh::iterator meshIter = mGameSession->getGameDataMaster()->getClientConnMesh().begin();
        GameDataMaster::ClientNetConnectionStatusMesh::iterator meshEnd = mGameSession->getGameDataMaster()->getClientConnMesh().end();
        TimeValue connInitTime = TimeValue::getTimeOfDay();
        for (; meshIter != meshEnd; ++meshIter)
        {
            ConnectionGroupId currentConnectionGroupId = meshIter->first;
            // get the current platform 
            ClientPlatformType currentClientPlatform = mGameSession->getClientPlatformForConnection(currentConnectionGroupId);

            if (currentConnectionGroupId != newConnectionGroupId)
            {
                bool isConnectionBiDirectional = true;
                if (mGameSession->shouldEndpointsConnect(newConnectionGroupId, currentConnectionGroupId))
                {
                    // we won't have QoS data at this stage, so just use the defaults
                    GameDataMaster::ClientConnectionDetailsPtr& newConnDetails = (*newEndpointMap)[currentConnectionGroupId];
                    if (newConnDetails == nullptr)
                        newConnDetails = newEndpointMap->allocate_element();
                    // update the new player's map with an entry for the existing player
                    newConnDetails->setStatus(ESTABLISHING_CONNECTION);
                    newConnDetails->setConnInitTime(connInitTime);
                    newConnDetails->setSourceConnectionGroupId(newConnectionGroupId);
                    newConnDetails->setTargetConnectionGroupId(currentConnectionGroupId);
                    initConnectionMetricFlags(*newConnDetails, joinType);
                }
                else
                {
                    isConnectionBiDirectional = false;
                }

                if (mGameSession->shouldEndpointsConnect(currentConnectionGroupId, newConnectionGroupId))
                {
                    GameDataMaster::ClientNetConnectionStatusMesh::mapped_type& existingEndpointMap = meshIter->second;
                    GameDataMaster::ClientConnectionDetailsPtr& existingConnDetails = (*existingEndpointMap)[newConnectionGroupId];
                    if (existingConnDetails == nullptr)
                        existingConnDetails = existingEndpointMap->allocate_element();
                    existingConnDetails->setStatus(ESTABLISHING_CONNECTION);
                    existingConnDetails->setConnInitTime(connInitTime);
                    existingConnDetails->setSourceConnectionGroupId(currentConnectionGroupId);
                    existingConnDetails->setTargetConnectionGroupId(newConnectionGroupId);
                    initConnectionMetricFlags(*existingConnDetails, joinType);
                }
                else
                {
                    isConnectionBiDirectional = false;
                }


                bool requestCCS = false;
                if (isConnectionBiDirectional && mGameSession->doesLeverageCCS())
                {
                    // also force CCS if the platforms mismatch
                    if ((mGameSession->getCCSMode() == CCS_MODE_HOSTEDONLY) || (newPlayerPlatform != currentClientPlatform))
                    {
                        requestCCS = true;
                    }
                }

                if (requestCCS)
                {
                    // is CC_HOSTEDONLY_MODE the right metric? even if we're not hosted only, this particular connection is treated as hosted only because of crossplay.
                    ConnConciergeModeMetricEnum ccMode = CC_HOSTEDONLY_MODE;
                    setCCSTriggerWasReceived(currentConnectionGroupId, newConnectionGroupId, ccMode);
                    setCCSTriggerWasReceived(newConnectionGroupId, currentConnectionGroupId, ccMode);

                    CCSRequestPair requestPair; 
                    requestPair.setConsoleFirstConnGrpId(currentConnectionGroupId);
                    requestPair.setConsoleSecondConnGrpId(newConnectionGroupId);

                    TRACE_LOG("[EndPointsConnectionMesh].addEndpoint: game("<< mGameSession->getGameId() <<"). Request hosted connectivity in hostedOnly mode for request pair(" << currentConnectionGroupId <<"," << newConnectionGroupId << ")");
                    mGameSession->addPairToPendingCCSRequests(requestPair);
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief remove an endpoint from the mesh.
    
        \param[in] dyingPlayerId - the playerId of the player to remove.
        \return true if the player is found and removed from mesh, false if the player if not found
    ***************************************************************************************************/
    bool EndPointsConnectionMesh::removeEndpoint(ConnectionGroupId dyingConnectionGroupId, PlayerId dyingPlayerId, PlayerRemovedReason reason)
    {
        TRACE_LOG("[EndPointsConnectionMesh].removeEndpoint: Removing endpoint(" << dyingConnectionGroupId << ") from mesh");
        GameDataMaster::ClientNetConnectionStatusMesh::iterator meshEnd = mGameSession->getGameDataMaster()->getClientConnMesh().end();
        GameDataMaster::ClientNetConnectionStatusMesh::iterator meshIter = mGameSession->getGameDataMaster()->getClientConnMesh().find(dyingConnectionGroupId);
        GameDataMaster::ClientNetConnectionStatusMesh::iterator dyingConnectionGroupIter = meshEnd;

        GameDataMaster::PlayerIdListByConnectionGroupIdMap::iterator playerIdListItr = mGameSession->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(dyingConnectionGroupId);
        PlayerIdList* dyingConnectionGroupIdPlayerList = (playerIdListItr != mGameSession->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end()) ? playerIdListItr->second : nullptr;
        if (dyingConnectionGroupIdPlayerList != nullptr && dyingConnectionGroupIdPlayerList->size() > 1)
        {
            //More than a single player in the game are using the same connection group id, do not delete connection group information.
            PlayerIdList::iterator playerIdListIt = dyingConnectionGroupIdPlayerList->begin();
            for(; playerIdListIt != dyingConnectionGroupIdPlayerList->end(); ++playerIdListIt)
            {
                if (*playerIdListIt == dyingPlayerId)
                {
                    playerIdListIt = dyingConnectionGroupIdPlayerList->erase(playerIdListIt);
                    return true;
                }
            }
            WARN_LOG("[EndPointsConnectionMesh].removeEndpoint: Removing player(" << dyingPlayerId << ") not part of connection group id(" << dyingConnectionGroupId << ") which has "<< dyingConnectionGroupIdPlayerList->size() <<" members.");
        }
        PINSubmissionPtr qosEvents = BLAZE_NEW PINSubmission;
        for (meshIter = mGameSession->getGameDataMaster()->getClientConnMesh().begin(); meshIter != meshEnd; ++meshIter)
        {
            ConnectionGroupId currentConnectionGroupId = meshIter->first;
            if (currentConnectionGroupId != dyingConnectionGroupId)
            {
                // Need to check whether the mesh even required a connection to the dyingPlayer
                GameDataMaster::ClientConnectionDetailsMap* existingClientMap = meshIter->second;
                GameDataMaster::ClientConnectionDetailsMap::iterator detailsIter = existingClientMap->find(dyingConnectionGroupId);
                if (detailsIter == existingClientMap->end())
                    continue;

                GameDataMaster::ClientConnectionDetails* targetConnDetails = detailsIter->second;
                GameDataMaster::ClientConnectionDetails* sourceConnDetails = getConnectionDetails(dyingConnectionGroupId, currentConnectionGroupId);
                TimeValue endTime = TimeValue::getTimeOfDay();

                targetConnDetails->setRemoveEndpointCalled(true);
                gGameManagerMaster->updateConnectionQosMetrics(*targetConnDetails, mGameSession->getGameId(), endTime, reason, qosEvents);
                targetConnDetails->setWasConnected(false);//reset historic connection status
                if (sourceConnDetails != nullptr)
                {
                    sourceConnDetails->setRemoveEndpointCalled(true);
                    gGameManagerMaster->updateConnectionQosMetrics(*sourceConnDetails, mGameSession->getGameId(), endTime, reason, qosEvents);
                    sourceConnDetails->setWasConnected(false);
                }

                existingClientMap->erase(dyingConnectionGroupId);
            }
            else
            {
                // defer removing the dyingPlayer until after we have finished iterating through the mesh as we need to continue checking
                // the connection status between each player in the mesh and the dying player
                dyingConnectionGroupIter = meshIter;
            }
        }

        gUserSessionManager->sendPINEvents(qosEvents);
        if (dyingConnectionGroupIter != meshEnd)
        {
            if (mGameSession->doesLeverageCCS() && reason != GAME_DESTROYED) //skip individual connectivity free request in favor of the bulk free request which will happen in the game's destruction call.
            {
                TRACE_LOG("[EndPointsConnectionMesh].removeEndpoint: game("<< mGameSession->getGameId() <<"). Free hosted connectivity of console(" << dyingConnectionGroupId <<") due to " 
                    << PlayerRemovedReasonToString(reason) <<".");

                freeHostedConnectivity(dyingConnectionGroupId);
            }
            mGameSession->getGameDataMaster()->getClientConnMesh().erase(dyingConnectionGroupIter);
            mGameSession->getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().erase(dyingConnectionGroupId);
            return true;
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief set the connection status between a source (myself) and target (another) player.
        NOTE: the mesh is a directed graph, so a->b may have a different status than b->a.
    
        A client is always considered to be connected to himself, so we don't support setting a->a to
        anything other than connected. (ie: assert if sourceConnectionGroupId == targetConnectionGroupId, and status != CONNECTED).

        \param[in] sourceConnectionGroupId - the playerId of the 'source' player in the directed graph.
        \param[in] targetConnectionGroupId - the playerId of the 'target' player in the directed graph.
        \param[in] status - the status of the connection
        \param[in] latency - the latency on the connection (in milliseconds)
        \param[in] packetLoss - the packet loss on this connection
        \return true if the status is set properly, false if source or target player is not found
    ***************************************************************************************************/
    bool EndPointsConnectionMesh::setConnectionStatus(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, PlayerNetConnectionStatus status, uint32_t latency, float packetLoss)
    {
        // no-op if we're trying to set our own connection
        if (sourceConnectionGroupId == targetConnectionGroupId)
        {
            return true;
        }

        // get the source player's map
        GameDataMaster::ClientNetConnectionStatusMesh::iterator meshIter = mGameSession->getGameDataMaster()->getClientConnMesh().find(sourceConnectionGroupId);
        if (meshIter == mGameSession->getGameDataMaster()->getClientConnMesh().end())
        {
            // log if source player is not in the mesh, could be player disconnected
            SPAM_LOG("[EndPointsConnectionMesh] Mesh connection update failed, source player(" << sourceConnectionGroupId << ") not found in mesh.");
            return false;
        }

        GameDataMaster::ClientConnectionDetailsMap* sourceClientMap = meshIter->second;

        // Check if target player in mesh, if not, remove from source map.
        if (mGameSession->getGameDataMaster()->getClientConnMesh().end() == mGameSession->getGameDataMaster()->getClientConnMesh().find(targetConnectionGroupId))
        {
            TRACE_LOG("[EndpointsConnectionMesh] Mesh connection update ignored (" << PlayerNetConnectionStatusToString(status) <<
                ") for player(" << sourceConnectionGroupId << ") -> player(" << targetConnectionGroupId << "): target already removed from mesh, erasing from source.");
            sourceClientMap->erase(targetConnectionGroupId);
            return false;
        }

        // find the target player in the source player's map
        GameDataMaster::ClientConnectionDetailsMap::iterator sourceClientMapIter = sourceClientMap->find(targetConnectionGroupId);
        if (sourceClientMapIter == sourceClientMap->end())
        {
            // log message if target player not in the source player's map!
            // this may happen if the target player drops out of the mesh before getting to this point.
            TRACE_LOG("[EndPointsConnectionMesh] Mesh connection update ignored (" << PlayerNetConnectionStatusToString(status) << "), latency (" 
                << latency << "ms), packet loss (" << packetLoss << "%%) failed for player(" << sourceConnectionGroupId << ") -> player(" 
                << targetConnectionGroupId << "): target player not found in source player's map.");
            return false;
        }

        GameDataMaster::ClientConnectionDetails* sourcePlayerDetails = sourceClientMapIter->second;

        // update the status
        if (status == CONNECTED)
        {
            // if bidirectionally connected, set metrics flag
            GameDataMaster::ClientConnectionDetails* targetPlayerDetails = getConnectionDetails(targetConnectionGroupId, sourceConnectionGroupId);
            if ((targetPlayerDetails != nullptr) && (targetPlayerDetails->getStatus() == CONNECTED))
            {
                sourcePlayerDetails->setWasConnected(true);
                targetPlayerDetails->setWasConnected(true);
            }
        }

        sourcePlayerDetails->setStatus(status);
        sourcePlayerDetails->setLatency(latency);
        sourcePlayerDetails->setPacketLoss(packetLoss);
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Sets the connection flags for the sourcePlayer in the targetPlayer's map

        \param[in] sourceConnectionGroupId - the playerId of the 'source' player in the directed graph.
        \param[in] targetConnectionGroupId - the playerId of the 'target' player in the directed graph.
        \param[in] flags - the connection flags
    ***************************************************************************************************/
    bool EndPointsConnectionMesh::setConnectionFlags(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, PlayerNetConnectionFlags flags)
    {
        // no-op if we're trying to set our own connection
        if (sourceConnectionGroupId == targetConnectionGroupId)
        {
            return true;
        }

        // find the source player in the target player's map
        GameDataMaster::ClientConnectionDetails* targetClientMapEntry = getConnectionDetails(targetConnectionGroupId, sourceConnectionGroupId);
        if (targetClientMapEntry == nullptr)
        {
            // if target player is not in the mesh, could be player disconnected.
            // if source player not in the target player's map, this may happen if the source player drops out of the mesh before getting to this point.
            TRACE_LOG("[EndPointsConnectionMesh] Unable to set connection flags for player(" << sourceConnectionGroupId << ") -> player(" 
                << targetConnectionGroupId << "): target player not found in mesh, or, source player not found in target player's map.");
            return false;
        }

        targetClientMapEntry->getFlags().setBits(flags.getBits());

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Return the status of the connection between source & target players (uni-directional).
    
        \param[in] sourceConnectionGroupId - the playerId of the 'source' player in the directed graph.
        \param[in] targetConnectionGroupId - the playerId of the 'target' player in the directed graph.
        \return the connection status between source & target players.
    ***************************************************************************************************/
    PlayerNetConnectionStatus EndPointsConnectionMesh::getConnectionStatus(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, LinkQosData *linkQos /* = nullptr*/) const
    {
        // find the target player in the source player's map
        const GameDataMaster::ClientConnectionDetails* sourceConnDetails = getConnectionDetails(sourceConnectionGroupId, targetConnectionGroupId);
        if (sourceConnDetails == nullptr)
        {
            return DISCONNECTED;
        }

        if (linkQos != nullptr)
        {
            linkQos->setLatencyMs(sourceConnDetails->getLatency());
            linkQos->setPacketLoss(sourceConnDetails->getPacketLoss());
            linkQos->setLinkConnectionStatus(sourceConnDetails->getStatus());
            linkQos->setConnectivityHosted(isConnectivityHosted(*sourceConnDetails));
        }

        // return the status
        return sourceConnDetails->getStatus();
    }


    /*! ************************************************************************************************/
    /*! \brief Perform a bi-directional connection check.  Returns the overall connection state between
            the two endpoints.
    
        Note: a client is always connected to himself; so if sourceConnectionGroupId == targetConnectionGroupId, the result
        is always CONNECTED.

        \param[in] sourceConnectionGroupId - the playerId of the 'source' player in the directed graph.
        \param[in] targetConnectionGroupId - the playerId of the 'target' player in the directed graph.
        \return if there's disagreement between the endpoints, we return ESTABLISHING_CONNECTION, otherwise
            we return the agreed upon connection status
    ***************************************************************************************************/
    PlayerNetConnectionStatus EndPointsConnectionMesh::getBiDirectionalConnectionStatus(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, LinkQosData *linkQos /* = nullptr*/) const
    {
        // we early out if we're testing against ourself
        if (sourceConnectionGroupId == targetConnectionGroupId)
        {
            if (linkQos != nullptr)
            {
                linkQos->setLatencyMs(0);
                linkQos->setPacketLoss(0);
                linkQos->setLinkConnectionStatus(CONNECTED);
            }

            return CONNECTED; // you're always connected to yourself
        }

        // otherwise, actually test if both players think they are disconnected.
        PlayerNetConnectionStatus sourceToTargetStatus = getConnectionStatus(sourceConnectionGroupId, targetConnectionGroupId, linkQos);
        PlayerNetConnectionStatus targetToSourceStatus = getConnectionStatus(targetConnectionGroupId, sourceConnectionGroupId);

        if (sourceToTargetStatus == targetToSourceStatus)
        {
            if (linkQos != nullptr)
            {
                // already set the latency and packet loss in the call to getConnectionStatus
                linkQos->setLinkConnectionStatus(sourceToTargetStatus);
            }
            // same value in both directions
            return sourceToTargetStatus;
        }
        else
        {
            // only return establishing if either is actually still establishing...
            if ( (sourceToTargetStatus == ESTABLISHING_CONNECTION) || (targetToSourceStatus == ESTABLISHING_CONNECTION) )
            {
                if (linkQos != nullptr)
                {
                    linkQos->setLinkConnectionStatus(ESTABLISHING_CONNECTION);
                }
                return ESTABLISHING_CONNECTION;
            }
            else
            {
                // otherwise, the endpoints don't agree, and neither still thinks it's establishing connection (our default state)
                return DISCONNECTED;
            }

        }
    }

    /*! ************************************************************************************************/
    /*! \brief Returns the MeshConnectionStatus of a player vs a full p2p game mesh (source player vs all
                existing active players in the game).

        \param[in] sourceConnectionGroupId - the playerId to check for full connectivity
        \param[in] gameSessionMaster - the game object to check the player against
        \return the overall mesh connection status for the source player
    ***************************************************************************************************/
    Blaze::GameManager::PlayerNetConnectionStatus EndPointsConnectionMesh::getFullMeshConnectionStatus( ConnectionGroupId sourceConnectionGroupId, 
        const GameSessionMaster &gameSessionMaster ) const
    {
        TRACE_LOG("[EndPointsConnectionMesh] Checking peer connection status for player connection group id(" << sourceConnectionGroupId << ")\n");
        // see if the player is fully connected to all active game members
        const PlayerRoster::PlayerInfoList& connectedPlayerList = gameSessionMaster.getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_CONNECTED_PLAYERS);
        PlayerRoster::PlayerInfoList::const_iterator connectedPlayerListIter = connectedPlayerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator connectedPlayerListEnd = connectedPlayerList.end();
        bool fullyConnected = true;
        for ( ; connectedPlayerListIter!=connectedPlayerListEnd; ++connectedPlayerListIter)
        {
            ConnectionGroupId activePlayerConnectionGroupId = (*connectedPlayerListIter)->getConnectionGroupId();

            PlayerNetConnectionStatus peerConnectionStatus = getBiDirectionalConnectionStatus(sourceConnectionGroupId, activePlayerConnectionGroupId);
            if (peerConnectionStatus == DISCONNECTED)
            {
                TRACE_LOG("[EndPointsConnectionMesh] Player(" << sourceConnectionGroupId << " -> " << activePlayerConnectionGroupId << ") failed to connect.\n");
                return DISCONNECTED; // we failed to connect to an active player, game over for full mesh topology
            }

            if (peerConnectionStatus == ESTABLISHING_CONNECTION)
            {
                // not fully connected to all active players yet
                TRACE_LOG("[EndPointsConnectionMesh] Player(" << sourceConnectionGroupId << " -> " << activePlayerConnectionGroupId << ") connected pending\n");
                fullyConnected = false;
            }
        }
        TRACE_LOG("[EndPointsConnectionMesh] player(" << sourceConnectionGroupId << ") " << (fullyConnected ? "is":"is not") << " fully connected in game(" 
                  << gameSessionMaster.getGameId() << ").\n");
        return fullyConnected ? CONNECTED : ESTABLISHING_CONNECTION;
    }

    /*! ************************************************************************************************/
    /*! \brief fetch the source -> target connection's details.
    ***************************************************************************************************/
    GameDataMaster::ClientConnectionDetails* EndPointsConnectionMesh::getConnectionDetails(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const
    {
        // get the source player's map
        GameDataMaster::ClientNetConnectionStatusMesh::iterator meshIter = mGameSession->getGameDataMaster()->getClientConnMesh().find(sourceConnectionGroupId);
        if (meshIter == mGameSession->getGameDataMaster()->getClientConnMesh().end())
        {
            return nullptr;
        }
        GameDataMaster::ClientConnectionDetailsMap* sourceClientMap = meshIter->second;

        // find the target player in the source player's map
        GameDataMaster::ClientConnectionDetailsMap::iterator sourceClientMapIter = sourceClientMap->find(targetConnectionGroupId);
        if (sourceClientMapIter == sourceClientMap->end())
        {
            return nullptr;
        }
        return sourceClientMapIter->second;
    }

    void EndPointsConnectionMesh::getConnInitTime(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, TimeValue& initTime) const
    {
        const GameDataMaster::ClientConnectionDetails* sourceConnDetails = getConnectionDetails(sourceConnectionGroupId, targetConnectionGroupId);
        if (sourceConnDetails == nullptr)
        {
            return;
        }
        initTime = sourceConnDetails->getConnInitTime();
    }

    /*! ************************************************************************************************/
    /*! \brief for a hosted network mesh (c/s or partial p2p), get the mesh connection status for a source player.
            since the only 'required' endpoint is the game host, we just check source vs host (and vice versa).
    
        \param[in] sourceConnectionGroupId - the playerId to check for full connectivity
        \param[in] hostPlayerId - the playerId of the game's host user
        \return the overall mesh connection status for the source player
    ***************************************************************************************************/
    PlayerNetConnectionStatus EndPointsConnectionMesh::getHostedMeshConnectionStatus(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId hostConnectionGroupId) const
    {
        // just check the bidirectional conn status between the source & host
        return getBiDirectionalConnectionStatus(sourceConnectionGroupId, hostConnectionGroupId);
    }

    /*! ************************************************************************************************/
    /*! \brief get the connection status of a player as well as QoS data, for its connection to the game host
    ***************************************************************************************************/
    void EndPointsConnectionMesh::getHostedMeshConnectionQos(ConnectionGroupId sourceConnectionGroupId, const GameSessionMaster &gameSessionMaster, PlayerQosData &qosData) const
    {
        PlayerId topologyHostId = gameSessionMaster.getTopologyHostInfo().getPlayerId();
        if (qosData.getLinkQosDataMap().find(topologyHostId) == qosData.getLinkQosDataMap().end())
        {
            // allocate a new element
            LinkQosData* linkQosData = qosData.getLinkQosDataMap().allocate_element();
            qosData.getLinkQosDataMap().insert(eastl::make_pair(topologyHostId, linkQosData));
        }
        PlayerNetConnectionStatus hostConnectionStatus = getBiDirectionalConnectionStatus(sourceConnectionGroupId, gameSessionMaster.getTopologyHostInfo().getConnectionGroupId(),
            qosData.getLinkQosDataMap()[topologyHostId]);

        qosData.setGameConnectionStatus(hostConnectionStatus);
    }

    /*! ************************************************************************************************/
    /*! \brief get the connection status of a player as well as QoS data, for its connections to other peers
        \param[in] sourceConnectionGroupId - connection group id of the player to get QoS data for
        \param[out] qosData the player's QoS data to get
    ***************************************************************************************************/
    void EndPointsConnectionMesh::getFullMeshConnectionQos(ConnectionGroupId sourceConnectionGroupId, const GameSessionMaster &gameSessionMaster, PlayerQosData &qosData) const
    {
        // see if the player is fully connected to all active game members
        const PlayerRoster::PlayerInfoList& connectedPlayerList = gameSessionMaster.getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        PlayerRoster::PlayerInfoList::const_iterator connectedPlayerListIter = connectedPlayerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator connectedPlayerListEnd = connectedPlayerList.end();
        PlayerNetConnectionStatus gameConnectionStatus = CONNECTED;
        for ( ; connectedPlayerListIter!=connectedPlayerListEnd; ++connectedPlayerListIter)
        {
            PlayerId activePlayerId = (*connectedPlayerListIter)->getPlayerId();
            ConnectionGroupId activeConnectionGroupId = (*connectedPlayerListIter)->getConnectionGroupId();

            if (qosData.getLinkQosDataMap().find(activePlayerId) == qosData.getLinkQosDataMap().end())
            {
                // allocate a new element
                LinkQosData* linkQosData = qosData.getLinkQosDataMap().allocate_element();
                qosData.getLinkQosDataMap().insert(eastl::make_pair(activePlayerId, linkQosData));
            }

            PlayerNetConnectionStatus peerConnectionStatus = getBiDirectionalConnectionStatus(sourceConnectionGroupId, activeConnectionGroupId,
                qosData.getLinkQosDataMap()[activePlayerId]);

            if (peerConnectionStatus == DISCONNECTED)
            {
                gameConnectionStatus = DISCONNECTED;
            }
        }

        qosData.setGameConnectionStatus(gameConnectionStatus);
    }





    GameId EndPointsConnectionMesh::getGameId() const
    {
        return mGameSession->getGameId();
    }
    
    const HostedConnectivityInfo* EndPointsConnectionMesh::getHostedConnectivityInfo(ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnGrpId) const
    {
        GameDataMaster::ClientConnectionDetailsPtr connDetails = getConnectionDetails(localConnGrpId, remoteConnGrpId);
        if (connDetails != nullptr && connDetails->getCCSInfo().getHostedConnectivityInfo().getHostingServerConnectivityId() != INVALID_CCS_CONNECTIVITY_ID)
        {
            return &(connDetails->getCCSInfo().getHostedConnectivityInfo());
        }
        
        return nullptr;
    }
    
    void EndPointsConnectionMesh::setHostedConnectivityInfo(ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnGrpId, const HostedConnectivityInfo& info)
    {
        GameDataMaster::ClientConnectionDetails* connDetailsInRemoteMap = getConnectionDetails(remoteConnGrpId, localConnGrpId); //locate the connDetails of the local end point in the remote Conn grp map
        GameDataMaster::ClientConnectionDetails* connDetailsInLocalMap =  getConnectionDetails(localConnGrpId, remoteConnGrpId); //vice-versa

        if (connDetailsInRemoteMap != nullptr && connDetailsInLocalMap != nullptr)
        {
            // for the local map, its just the info passed in
            info.copyInto(connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo());

            info.copyInto(connDetailsInRemoteMap->getCCSInfo().getHostedConnectivityInfo());
            // for the remote map, swap local/remote connectivity ids
            connDetailsInRemoteMap->getCCSInfo().getHostedConnectivityInfo().setLocalLowLevelConnectivityId(info.getRemoteLowLevelConnectivityId());
            connDetailsInRemoteMap->getCCSInfo().getHostedConnectivityInfo().setRemoteLowLevelConnectivityId(info.getLocalLowLevelConnectivityId());
            
            ASSERT_COND_LOG((connDetailsInLocalMap->getCCSInfo().getConnConciergeMode() != CC_UNUSED) && (connDetailsInRemoteMap->getCCSInfo().getConnConciergeMode() != CC_UNUSED), "Unexpected internal state: connection concierge reason was unset when hosted connectivity info was set. The reason was expected to be set up front.");
        }
        else
        {
            ERR_LOG("[EndPointsConnectionMesh].setHostedConnectivityInfo: calling setHostedConnectivityInfo on request pair ("<< localConnGrpId << "," <<remoteConnGrpId <<") but it does not exist in the connection mesh");
        }
    }

    void EndPointsConnectionMesh::freeHostedConnectivityHelper(CCSFreeRequest ccsFreeRequest)
    {
        Blaze::GameManager::GameManagerSlave* gmgrSlave = static_cast<Blaze::GameManager::GameManagerSlave*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, false, true));
        if (gmgrSlave != nullptr)
        {
            TRACE_LOG("[EndPointsConnectionMesh].freeHostedConnectivityHelper: Issue CCS free request to the slave. " << ccsFreeRequest);
            RpcCallOptions opts;
            opts.ignoreReply = true;
            BlazeRpcError error = gmgrSlave->freeConnectivityViaCCS(ccsFreeRequest, opts);
            if (error != Blaze::ERR_OK)
            {
                ERR_LOG("[EndPointsConnectionMesh].freeHostedConnectivityHelper: error calling freeConnectivityViaCCS on slave. " << ccsFreeRequest);
            }
        }
    }
    
    
    void EndPointsConnectionMesh::freeHostedConnectivityMesh(bool immediate)
    {
        CCSFreeRequest ccsFreeRequest;
        ccsFreeRequest.setGameId(mGameSession->getGameId());

        CCSUtil ccsUtil;
        BlazeRpcError error = ccsUtil.buildFreeHostedConnectionRequest(*mGameSession, ccsFreeRequest);
        if (error == Blaze::ERR_OK)
        {
            if (immediate)
            {
                freeHostedConnectivityHelper(ccsFreeRequest);
            }
            else
            {
                TimeValue delay = gGameManagerMaster->getConfig().getGameSession().getCcsFreeRequestDelay();
                gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + delay,
                    &EndPointsConnectionMesh::freeHostedConnectivityHelper, ccsFreeRequest,
                    "EndPointsConnectionMesh::freeHostedConnectivityHelper");
            }
        }
        // We can't wait around for any failure on the slave's or CCS end. For example, if we were to report those back to the master, the game session might have gone away. As far as the master is concerned, the game 
        // session is done with the hosted connectivity and does not need it.
        for (GameDataMaster::ClientNetConnectionStatusMesh::const_iterator it = mGameSession->getGameDataMaster()->getClientConnMesh().begin(), itEnd = mGameSession->getGameDataMaster()->getClientConnMesh().end();
            it != itEnd; ++it)
        {
            const GameDataMaster::ClientConnectionDetailsMap* connGrpConnMap = it->second;
            for (GameDataMaster::ClientConnectionDetailsMap::const_iterator itConnDetails = connGrpConnMap->begin(), itConnDetailsEnd = connGrpConnMap->end(); itConnDetails != itConnDetailsEnd; ++itConnDetails)
            {
                clearHostedConnectivityInfo(it->first, itConnDetails->first);
            }
        }
    }

   
    void EndPointsConnectionMesh::freeHostedConnectivity(ConnectionGroupId localConnGrpId)
    {
        GameDataMaster::ClientNetConnectionStatusMesh::const_iterator meshIter = mGameSession->getGameDataMaster()->getClientConnMesh().find(localConnGrpId);
        if (meshIter != mGameSession->getGameDataMaster()->getClientConnMesh().end())
        {
            const GameDataMaster::ClientConnectionDetailsMap* connGrpConnMap = meshIter->second;
            for (GameDataMaster::ClientConnectionDetailsMap::const_iterator it = connGrpConnMap->begin(), itEnd = connGrpConnMap->end(); it != itEnd; ++it)
            {
                GameDataMaster::ClientConnectionDetailsPtr connDetails = it->second;
                if (connDetails != nullptr && connDetails->getCCSInfo().getHostedConnectivityInfo().getHostingServerConnectivityId() != INVALID_CCS_CONNECTIVITY_ID)
                {
                    freeHostedConnectivity(localConnGrpId, it->first, connDetails->getCCSInfo().getHostedConnectivityInfo().getLocalLowLevelConnectivityId(), connDetails->getCCSInfo().getHostedConnectivityInfo().getRemoteLowLevelConnectivityId());
                    clearHostedConnectivityInfo(localConnGrpId, it->first);
                }
            }
        }
    }

    void EndPointsConnectionMesh::freeHostedConnectivity(ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnnGrpId, uint32_t localConnectivityId, uint32_t remoteConnectivityId)
    {
        CCSFreeRequest ccsFreeRequest;
        ccsFreeRequest.setGameId(mGameSession->getGameId());

        CCSUtil ccsUtil;

        BlazeRpcError error = ccsUtil.buildFreeHostedConnectionRequest(*mGameSession,ccsFreeRequest,localConnectivityId,remoteConnectivityId);
        if (error == Blaze::ERR_OK)
        {
            Blaze::GameManager::GameManagerSlave* gmgrSlave = static_cast<Blaze::GameManager::GameManagerSlave*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, false, true));
            if (gmgrSlave != nullptr)
            {
                TRACE_LOG("[EndPointsConnectionMesh].freeHostedConnectivity: Issue CCS free request to the slave. " << ccsFreeRequest);
                RpcCallOptions opts;
                opts.ignoreReply = true;
                error = gmgrSlave->freeConnectivityViaCCS(ccsFreeRequest, opts);
                if (error != Blaze::ERR_OK) 
                {
                    ERR_LOG("[EndPointsConnectionMesh].freeHostedConnectivity: error calling freeConnectivityViaCCS on slave. " << ccsFreeRequest);
                }
            }
        }

        if (mGameSession->getGameDataMaster()->getNumHostedConnections() > 0)
        {
            mGameSession->getGameDataMaster()->setNumHostedConnections(mGameSession->getGameDataMaster()->getNumHostedConnections()-1);
        }
        else
        {
            ERR_LOG("[EndPointsConnectionMesh].freeHostedConnectivity: invalid value for hosted connections while trying to decrement:"<< mGameSession->getGameDataMaster()->getNumHostedConnections());
        }
    }

    void EndPointsConnectionMesh::clearHostedConnectivityInfo(ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnGrpId)
    {
        GameDataMaster::ClientConnectionDetails* connDetailsInRemoteMap = getConnectionDetails(remoteConnGrpId, localConnGrpId); //locate the connDetails of the local end point in the remote Conn grp map
        GameDataMaster::ClientConnectionDetails* connDetailsInLocalMap =  getConnectionDetails(localConnGrpId, remoteConnGrpId); //vice-versa

        if (connDetailsInRemoteMap != nullptr && connDetailsInLocalMap != nullptr)
        {
            connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().setLocalLowLevelConnectivityId(INVALID_CCS_CONNECTIVITY_ID);
            connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().setRemoteLowLevelConnectivityId(INVALID_CCS_CONNECTIVITY_ID);
            connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().setHostingServerConnectivityId(INVALID_CCS_CONNECTIVITY_ID);
            connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().setHostingServerConnectionSetId("");
            if (connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().getHostingServerNetworkAddress().getIpAddress() != nullptr)
            {
                connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().getHostingServerNetworkAddress().getIpAddress()->setIp(0);
                connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().getHostingServerNetworkAddress().getIpAddress()->setPort(0);
            }
            connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().getHostingServerNetworkAddress().switchActiveMember(NetworkAddress::MEMBER_UNSET);

            // to also clear the info in remote map, just copy the already cleared info from local map
            connDetailsInLocalMap->getCCSInfo().getHostedConnectivityInfo().copyInto(connDetailsInRemoteMap->getCCSInfo().getHostedConnectivityInfo());
        }
    }

    bool EndPointsConnectionMesh::getCCSTriggerWasReceived(ConnectionGroupId sourceConnGrpId, ConnectionGroupId targetConnGrpId) const
    {
        GameDataMaster::ClientConnectionDetails* connDetailsInSourceMap =  getConnectionDetails(sourceConnGrpId, targetConnGrpId);
        if (connDetailsInSourceMap != nullptr)
        {
            return connDetailsInSourceMap->getCCSInfo().getCCSTriggerWasReceived();
        }
        return false;
    }

    void EndPointsConnectionMesh::setCCSTriggerWasReceived(ConnectionGroupId sourceConnGrpId, ConnectionGroupId targetConnGrpId, ConnConciergeModeMetricEnum ccMode)
    {
        GameDataMaster::ClientConnectionDetails* connDetailsInSourceMap =  getConnectionDetails(sourceConnGrpId, targetConnGrpId);
        if (connDetailsInSourceMap != nullptr)
        {
            if (ccMode == CC_HOSTEDFALLBACK_MODE)
            {
                // increment metrics for the direct connection attempt that failed, before assigning a CC mode below,
                // as the CC mode is only for subsequent hosted connection's metrics
                gGameManagerMaster->updateConnectionQosMetrics(*connDetailsInSourceMap, getGameId(), TimeValue::getTimeOfDay(), PLAYER_CONN_LOST, nullptr);
            }

            connDetailsInSourceMap->getCCSInfo().setCCSTriggerWasReceived(true);
            connDetailsInSourceMap->getCCSInfo().setConnConciergeMode(ccMode);
            connDetailsInSourceMap->getCCSInfo().setCCSRequestInProgress(true);
        }
    }
    
    /*! ************************************************************************************************/
    /*! \brief Init server based connection metrics flags for the sourcePlayer in targetPlayer's map.
        \param[in,out] connDetails - contains the flags to update
        \param[in] rosterPlayer - the player whose join triggered the adding of this connection. nullptr if it was a dedicated server host
    ***************************************************************************************************/
    void EndPointsConnectionMesh::initConnectionMetricFlags(GameDataMaster::ClientConnectionDetails& connDetails, ConnectionJoinType joinType)
    {
        ConnectionGroupId sourceId = connDetails.getSourceConnectionGroupId();
        ConnectionGroupId targetId = connDetails.getTargetConnectionGroupId();
        ASSERT_COND_LOG(((mGameSession->getGameNetworkTopology() != NETWORK_DISABLED) || (mGameSession->getVoipNetwork() != VOIP_DISABLED)), "[EndPointsConnectionMesh].initConnectionMetricFlags: internal error, endpoints connection mesh initialized without game, or without known network topology. Metrics may not be properly updated");

        connDetails.setJoinType(joinType);
        connDetails.setConnectionGame(mGameSession->isConnectionRequired(sourceId, targetId));

        // count as QoSd, if *either* side of the bi-dir connection is in QoS map (note Find Game MM may only have searcher side in the map).
        if ((mGameSession->isConnectionGroupInMatchmakingQosDataMap(sourceId) ||
                mGameSession->isConnectionGroupInMatchmakingQosDataMap(targetId)))
        {    
            connDetails.setQosEnabled(true);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return whether connection is player impacting for metrics
    ***************************************************************************************************/
    bool EndPointsConnectionMesh::isConnectionPlayerImpacting(const GameDataMaster::ClientConnectionDetails& connDetails) const
    {
        bool isPlayerImpacting = true;        
        // note: order of below checks is priority-relevant

        if (connDetails.getQosEnabled() && gGameManagerMaster->getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getContinueMatchingAfterValidationFailure())
        {
            // if QoS continuation enabled, its player impacting iff the validation passed, which would lead to a player facing join
            isPlayerImpacting = connDetails.getQosValidationPassed();
        }
        else if (!connDetails.getRemoveEndpointCalled())
        {
            // handle non impacting failed connections, where player is not removed from game. side: failed connections that were pre-demangler aren't handled, as that's only known at client level
            // (side: if remove player *was* called, we don't set impacting false below, as any non-QoSd connection success/fail is impacting, and, even if don't have connection result yet (e.g. leaving while connecting) its impacting)
            if (mGameSession->doesLeverageCCS() && !isConnectivityHosted(connDetails) && !connDetails.getWasConnected())
            {
                // if not connected, but have CCS hosted connectivity to fall back on, this is not player impacting. If connected, given QoS disabled, set impacting
                isPlayerImpacting = false;
            }
        }
        TRACE_LOG("[EndPointsConnectionMesh].isConnectionPlayerImpacting: connection " << connDetails.getSourceConnectionGroupId() << " -> " << connDetails.getTargetConnectionGroupId() << " player impacting for metrics is " << (isPlayerImpacting ? "true" : "false") << ", for game(" << getGameId() << ").");
        return isPlayerImpacting;
    }

    // set metric flag indicating client was notified that QoS validation passed
    void EndPointsConnectionMesh::setQosValidationPassedMetricFlag(ConnectionGroupId sourceConnectionGroupId)
    {
        GameDataMaster::ClientNetConnectionStatusMesh::iterator meshIter = mGameSession->getGameDataMaster()->getClientConnMesh().find(sourceConnectionGroupId);
        if (meshIter == mGameSession->getGameDataMaster()->getClientConnMesh().end())
        {
            return;
        }
        TRACE_LOG("[EndPointsConnectionMesh].setQosValidationPassedMetricFlag: setting qos validation passed flags for " << sourceConnectionGroupId << " connections, in game(" << getGameId() << ").");
        GameDataMaster::ClientConnectionDetailsMap* sourceMap = meshIter->second;
        for (GameDataMaster::ClientConnectionDetailsMap::iterator itr = sourceMap->begin(), end = sourceMap->end(); itr != end; ++itr)
        {
            // source -> target connection
            GameDataMaster::ClientConnectionDetails* sourceConnDetails = itr->second;
            sourceConnDetails->setQosValidationPassed(true);

            // target -> source connection (side: Find Mode MM may only have searcher side of a bidir connection in game's QoS map, but still consider/update this the same as 'player facing' for both sides)
            GameDataMaster::ClientConnectionDetails* targetConnDetails = getConnectionDetails(itr->first, sourceConnectionGroupId);
            if (targetConnDetails != nullptr)
            {
                targetConnDetails->setQosValidationPassed(true);
            }
        }
    }
} // namespace GameManager
} // namespace Blaze

