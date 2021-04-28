/*! ************************************************************************************************/
/*!
    \file endpointsconnectionmesh.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_ENDPOINTS_CONNECTION_MESH_H
#define BLAZE_GAMEMANAGER_ENDPOINTS_CONNECTION_MESH_H

#include "gamemanager/tdf/gamemanager_server.h"
#include "gamemanager/ccsutil.h"

#include "EASTL/map.h"

namespace Blaze
{
namespace GameManager
{
    class GameSessionMaster;
    class LinkQosData;
    class PlayerQosData;

    /*! ************************************************************************************************/
    /*! \brief The server-side representation of the client's peer connection mesh; each client has a set
        of network status vars for every other client.  NOTE: a client is always considered to be connected
        to himself.

        Conceptually, the connection mesh is a directed symmetric graph.  The vertices are the clients (connectionGroupIds),
        and the edges are the PlayerNetConnectionStatus.

        The mesh object is a container class with a few helpers to determine connectivity.
    ***************************************************************************************************/
    class EndPointsConnectionMesh
    {

    public:
        EndPointsConnectionMesh();
        ~EndPointsConnectionMesh() { }

        bool areEndpointsConnected(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const
        {
            return ( getBiDirectionalConnectionStatus(sourceConnectionGroupId, targetConnectionGroupId) == CONNECTED );
        }
        PlayerNetConnectionStatus getBiDirectionalConnectionStatus(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, LinkQosData *linkQos = nullptr) const;

        void addEndpoint(ConnectionGroupId newConnectionGroupId, PlayerId newPlayerId, ClientPlatformType newPlayerPlatform, ConnectionJoinType joinType, bool resetClientConnectionStatus = false);
        bool removeEndpoint(ConnectionGroupId dyingConnectionGroupId, PlayerId dyingPlayerId, PlayerRemovedReason reason);

        bool setConnectionStatus(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, PlayerNetConnectionStatus status, uint32_t latency, float packetLoss);
        bool setConnectionFlags(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, PlayerNetConnectionFlags flags);

        PlayerNetConnectionStatus getHostedMeshConnectionStatus(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId hostConnectionGroupId) const;
        PlayerNetConnectionStatus getFullMeshConnectionStatus(ConnectionGroupId sourceConnectionGroupId, const GameSessionMaster &gameSessionMaster) const;
        void getConnInitTime(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, TimeValue& initTime) const;

        // get the connection status of a player as well as QoS data, either to the game host or to the full mesh
        void getHostedMeshConnectionQos(ConnectionGroupId sourceConnectionGroupId, const GameSessionMaster &gameSessionMaster, PlayerQosData &qosData) const;
        void getFullMeshConnectionQos(ConnectionGroupId sourceConnectionGroupId, const GameSessionMaster &gameSessionMaster, PlayerQosData &qosData) const;

        GameId getGameId() const;
        void setGameSessionMaster(GameSessionMaster& session) { mGameSession = &session; }

        void setHostedConnectivityInfo(ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnGrpId, const HostedConnectivityInfo& info);
        const HostedConnectivityInfo* getHostedConnectivityInfo(ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnGrpId) const;
        
        void setCCSTriggerWasReceived(ConnectionGroupId sourceConnGrpId, ConnectionGroupId targetConnGrpId, ConnConciergeModeMetricEnum ccMode);
        bool getCCSTriggerWasReceived(ConnectionGroupId sourceConnGrpId, ConnectionGroupId targetConnGrpId) const;
        
        void freeHostedConnectivityMesh(bool immediate); // free hosted connectivity of entire mesh
        void freeHostedConnectivity(ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnnGrpId, uint32_t localConnectivityId, uint32_t remoteConnectivityId); // free hosted connectivity between specific pair

        void setQosValidationPassedMetricFlag(ConnectionGroupId sourceConnectionGroupId);
        GameDataMaster::ClientConnectionDetails* getConnectionDetails(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId) const;
        bool isConnectionPlayerImpacting(const GameDataMaster::ClientConnectionDetails& connDetails) const;

    private:
        void freeHostedConnectivity(ConnectionGroupId localConnGrpId); // free hosted connectivity against everybody else 
        void clearHostedConnectivityInfo(ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnGrpId);

        static void freeHostedConnectivityHelper(CCSFreeRequest ccsFreeRequest);
        PlayerNetConnectionStatus getConnectionStatus(ConnectionGroupId sourceConnectionGroupId, ConnectionGroupId targetConnectionGroupId, LinkQosData *linkQos = nullptr) const;

        bool isConnectivityHosted(const GameDataMaster::ClientConnectionDetails& connDetails) const { return (connDetails.getCCSInfo().getHostedConnectivityInfo().getHostingServerNetworkAddress().getActiveMember() != NetworkAddress::MEMBER_UNSET); }

        void initConnectionMetricFlags(GameDataMaster::ClientConnectionDetails& connDetails, ConnectionJoinType joinType);
    private:
        GameSessionMaster* mGameSession;
    };

} //GameManager
} // Blaze

#endif // BLAZE_GAMEMANAGER_ENDPOINTS_CONNECTION_MESH_H
