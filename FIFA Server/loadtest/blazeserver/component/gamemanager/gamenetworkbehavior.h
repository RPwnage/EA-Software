/*! ************************************************************************************************/
/*!
    \file gamenetworkbehavior.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAME_NETWORK_BEHAVIOR_H
#define BLAZE_GAMEMANAGER_GAME_NETWORK_BEHAVIOR_H

#include "gamemanager/tdf/gamemanager.h"
#include "gamemanager/rpc/gamemanagermaster_stub.h" // for RPC error codes
#include "gamemanager/endpointsconnectionmesh.h"
#include "gamemanager/gamesessionmaster.h"

namespace Blaze
{
namespace GameManager
{
    class GameManagerMasterImpl;
    class PlayerInfoMaster;
    class CreateGameRequest;
    class RemovePlayerRequest;
    class UpdateMeshConnectionRequest;
    class GameSessionMaster;

    /*! ************************************************************************************************/
    /*! \brief Abstract interface for network-topology specific behaviors.

        GM_AUDIT: refactor: it doesn't make sense to have this logic separated out onto a separate class
            (since we have to pass in the gameSessionMaster anways to take action on the game).
            These virtual methods should probably just live on the GameSessionMaster itself.
    ***************************************************************************************************/
    class GameNetworkBehavior
    {
        NON_COPYABLE(GameNetworkBehavior);

    public:
        virtual ~GameNetworkBehavior() {};

        // mesh connection behavior depends on network topology (when are we fully connected to a game)
        virtual BlazeRpcError updateClientMeshConnection(const UpdateMeshConnectionMasterRequest &request, 
                                                                                  GameSessionMaster* game) = 0;

        virtual void onActivePlayerJoined(GameSessionMaster* game, PlayerInfoMaster* player) {}

        virtual void onActivePlayerRemoved(GameSessionMaster* game) = 0;

        virtual void getMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const = 0;

        // returns true if the endpoints should connect with each other. 
        virtual bool shouldEndpointsConnect(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const { return true; }

        // returns true if the connection between endpoints is necessary. For example, connection against a game's host is necessary in CSPH whereas the connection against a non-host peer
        // for VOIP traffic is not. 
        virtual bool isConnectionRequired(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const 
        { 
            return shouldEndpointsConnect(game, sourceConnectionGroupId, targetConnectionGroupId); 
        }

    protected:
        GameNetworkBehavior() {}

        BlazeRpcError updateHostedMeshConnection(const UpdateMeshConnectionMasterRequest &request, GameSessionMaster* gameSessionMaster);
        BlazeRpcError updateFullMeshConnection(const UpdateMeshConnectionMasterRequest &request, GameSessionMaster* gameSessionMaster);
        bool updateMeshConnectionCommon(const UpdateMeshConnectionMasterRequest &request, GameSessionMaster* gameSessionMaster);

        void getHostedMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const;

        void getFullMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const;


        bool processMeshConnectionStateChange(ConnectionGroupId connectionGroupId, GameSessionMaster* game,  PlayerNetConnectionStatus meshConnectionStatus, ConnectionGroupId reportingConnectionGroupId);
        bool processMajorityRulesOnDisconnect(ConnectionGroupId connectionGroupId, const GameSessionMaster* game);

        void processMeshConnectionEstablished(ConnectionGroupId connectionGroupId, GameSessionMaster* game);
        virtual bool processMeshConnectionLost(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, ConnectionGroupId reportingConnectionGroupId) const;

        bool isPlayerConnectedToFullMesh(const PlayerInfo *player, const GameSessionMaster* game) const;
        bool isClientDisconnectedFromMajorityOfMesh(ConnectionGroupId connectionGroupId, const GameSessionMaster* game) const;

        void pingHostKickIfGone(PlayerId hostId, UserSessionId hostSessionId, GameSessionMaster* game) const;

        bool shouldKickHost(GameSessionMaster* gameSessionMaster, ConnectionGroupId reportingConnectionGroupId) const;

        bool updateConnGroupMatchmakingQosData(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, const PlayerNetConnectionStatus* status);

        void startPendingKickForConnectionGroup(GameSessionMasterPtr gameSessionMasterPtr, ConnectionGroupId connectionGroupIdToKick) const;

        bool isConnectionGroupSpectating(GameSessionMasterPtr gameSessionMasterPtr, ConnectionGroupId connectionGroupId) const;
    };

    /*! ************************************************************************************************/
    /*! \brief Behavior specific to dedicated server games [CLIENT_SERVER_DEDICATED]
    ***************************************************************************************************/
    class NetworkBehaviorDedicatedServer : public GameNetworkBehavior
    {
    public:
        NetworkBehaviorDedicatedServer() {}
        ~NetworkBehaviorDedicatedServer() override {}

        BlazeRpcError updateClientMeshConnection(const UpdateMeshConnectionMasterRequest &request, 
                                                                          GameSessionMaster *gameSessionMaster) override
        {
            // dedicated server games have a hosted client-server topology
            return updateHostedMeshConnection(request, gameSessionMaster);
        }

        void getMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const override
        {
            getHostedMeshQos(connectionGroupId, game, playerQos);
        }

        void onActivePlayerRemoved(GameSessionMaster* game) override { }

    protected:
        bool processMeshConnectionLost(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, ConnectionGroupId reportingConnectionGroupId) const override;
        bool shouldEndpointsConnect(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const override;
        bool isConnectionRequired(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const override; 
    };

    /*! ************************************************************************************************/
    /*! \brief Behavior specific to client/server peer hosted games [CLIENT_SERVER_PEER_HOSTED]
    ***************************************************************************************************/
    class NetworkBehaviorPeerHosted : public GameNetworkBehavior
    {
    public:
        NetworkBehaviorPeerHosted() {}
        ~NetworkBehaviorPeerHosted() override {};

        BlazeRpcError updateClientMeshConnection(const UpdateMeshConnectionMasterRequest &request, 
                                                                          GameSessionMaster *gameSessionMaster) override
        {
            // peer hosted game have a client-server topology
            return updateHostedMeshConnection(request, gameSessionMaster);
        }

        void getMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const override
        {
            getHostedMeshQos(connectionGroupId, game, playerQos);
        }

        void onActivePlayerRemoved(GameSessionMaster* game) override;

    protected:
        bool shouldEndpointsConnect(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const override;
        bool isConnectionRequired(const GameSessionMaster* game, ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const override; 

    };

    /*! ************************************************************************************************/
    /*! \brief Behavior specific to peer to peer full mesh [PEER_TO_PEER_FULL_MESH]
    ***************************************************************************************************/
    class NetworkBehaviorFullMesh : public GameNetworkBehavior
    {
    public:
        NetworkBehaviorFullMesh() {}
        ~NetworkBehaviorFullMesh() override {}

        BlazeRpcError updateClientMeshConnection(const UpdateMeshConnectionMasterRequest &request, 
                                                                          GameSessionMaster *gameSessionMaster) override
        {
            // 'p2p full mesh' requires a full mesh connection (to all active players)
            return updateFullMeshConnection(request, gameSessionMaster);
        }

        void getMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const override
        {
            getFullMeshQos(connectionGroupId, game, playerQos);
        }

        void onActivePlayerRemoved(GameSessionMaster* game) override;

    protected:
        bool processMeshConnectionLost(ConnectionGroupId connectionGroupId, GameSessionMaster* gameSessionMaster, ConnectionGroupId reportingConnectionGroupId) const override;
    };

    /*! ************************************************************************************************/
    /*! \brief Behavior specific to topology that does not establish any game data networking [NETWORK_DISABLED]
    To consider: Rename the NETWORK_DISABLED topology to GAME_GROUP as they can establish/track network connections now. 
    ***************************************************************************************************/
    class NetworkBehaviorDisabled : public GameNetworkBehavior
    {
    public:
        NetworkBehaviorDisabled() {}
        ~NetworkBehaviorDisabled() override {}

        BlazeRpcError updateClientMeshConnection(const UpdateMeshConnectionMasterRequest &request, GameSessionMaster *gameSessionMaster) override
        {
            // Track the PlayerNetConnectionState between end points. The player's state against the game group is always ACTIVE_CONNECTED and is not impacted by
            // PlayerNetConnectionState changes between any two players/consoles.
            updateMeshConnectionCommon(request, gameSessionMaster);
            return Blaze::ERR_OK;
        }

        void getMeshQos(ConnectionGroupId connectionGroupId, const GameSessionMaster &game, PlayerQosData &playerQos) const override {}

        void onActivePlayerJoined(GameSessionMaster* game, PlayerInfoMaster* player) override;

        void onActivePlayerRemoved(GameSessionMaster* game) override {}

    protected:
        bool shouldEndpointsConnect(const GameSessionMaster* game, ConnectionGroupId /*sourceConnectionGroupId*/, ConnectionGroupId /*targetConectionGroupId*/) const override 
        { 
            return (game->getVoipNetwork() == VOIP_PEER_TO_PEER); // The only supported voip topology for the game groups
        }
        bool isConnectionRequired(const GameSessionMaster* game, ConnectionGroupId /*sourceConnectionGroupId*/, ConnectionGroupId /*targetConectionGroupId*/) const override 
        {
            return false;
        }

    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_GAME_NETWORK_BEHAVIOR_H
