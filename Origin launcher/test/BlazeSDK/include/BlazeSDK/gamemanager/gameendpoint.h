/*! ************************************************************************************************/
/*!
    \file gameendpoint.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMEENDPOINT_H
#define BLAZE_GAMEMANAGER_GAMEENDPOINT_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/jobid.h"
#include "BlazeSDK/mesh.h"
#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"
#include "BlazeSDK/gamemanager/game.h"


namespace Blaze
{

namespace GameManager
{
    class GameManagerAPI;

    /*! **********************************************************************************************************/
    /*! \class GameEndpoint
    **************************************************************************************************************/
    class BLAZESDK_API GameEndpoint : public MeshEndpoint 
    {
    public:

        
    /*! **********************************************************************************************************/
    /*! \name MeshEndpoint Interface Implementation

    Clients in a game implement the MeshEndpoint interface so they can interact with a NetworkMeshAdapter instance.
    The Mesh interface corresponds to the game, a MeshEndpoint to a endpoint in the game mesh, while the MeshMember interface corresponds to a Player.
        
    **************************************************************************************************************/

    /*! ************************************************************************************************/
    /*! \brief returns the mesh this endpoint is part of
    ***************************************************************************************************/
    virtual const Mesh* getMesh() const override { return mGame; }
    
    /*! ************************************************************************************************/
    /*! \brief returns true if this endpoint is the local machine. Other local users will share this Endpoint
        will have the same connection group id.
    ***************************************************************************************************/
    virtual bool isLocal() const override;
    
    /*! ************************************************************************************************/
    /*! \brief returns the network address of the endpoint. If remote end point and connectivity is hosted, the network address of hosted resource is returned.
    ***************************************************************************************************/
    virtual const NetworkAddress* getNetworkAddress() const override;

    /*! **********************************************************************************************************/
    /*! \brief Returns true if this endpoint is the topology host for this mesh.
        \return    True if this is the network host.
    **************************************************************************************************************/
    virtual bool isTopologyHost() const override;

    /*! **********************************************************************************************************/
    /*! \brief Returns true if this endpoint is the dedicated server in this mesh
        \return    True if this is the network host.
    **************************************************************************************************************/
    virtual bool isDedicatedServerHost() const override;

    /*! ************************************************************************************************/
    /*! \brief returns the connection group id.  A unique id per connection to the blaze server
        which will be equal for multiple players/members connected to Blaze on the same connection.
    ***************************************************************************************************/
    virtual uint64_t getConnectionGroupId() const override { return mConnectionGroupId; }

    /*! ************************************************************************************************/
    /*! \brief returns the slot id of the connection group
    ***************************************************************************************************/
    virtual uint8_t getConnectionSlotId() const override { return mConnectionSlotId; }

    /*! ************************************************************************************************/
    /*! \brief returns true if a Qos check should happen when establishing connection to this endpoint
    ***************************************************************************************************/
    virtual bool getPerformQosDuringConnection() const override { return mPerformQosDuringConnection; }

    /*! ************************************************************************************************/
    /*! \brief returns the UUID for this player
    ***************************************************************************************************/
    virtual const char8_t* getUUID() const override { return mUUID.c_str(); }

    /*! ************************************************************************************************/
    /*! \brief returns the member count for this endpoint
    ***************************************************************************************************/
    virtual uint8_t getMemberCount() const override { return mMemberCount; }

    /*! ************************************************************************************************/
    /*! \brief returns the tunnel key for this mesh endpoint and the calling mesh endpoint
    ***************************************************************************************************/
    virtual const char8_t* getTunnelKey() const override;

    /*! ************************************************************************************************/
    /*! \brief returns true if the endpoint is being serviced by CC
    ***************************************************************************************************/
    virtual bool isConnectivityHosted() const override;

    /*! ************************************************************************************************/
    /*! \brief returns the CC identifier for replicated player or dedicated host endpoint.
    ***************************************************************************************************/
    virtual uint32_t getRemoteLowLevelConnectivityId() const override;
    
     /*! ************************************************************************************************/
    /*! \brief returns the CC identifier for local player or dedicated host endpoint.
    ***************************************************************************************************/
    virtual uint32_t getLocalLowLevelConnectivityId() const override;

    /*! ************************************************************************************************/
    /*! \brief returns the CC identifier for hosting server providing the connectivity to this endpoint.
    ***************************************************************************************************/
    virtual uint32_t getHostingServerConnectivityId() const override;

    /*! ************************************************************************************************/
    /*! \brief returns the CC connection set id for hosting server providing the connectivity to this endpoint.
    ***************************************************************************************************/
    virtual const char8_t* getHostingServerConnectionSetId() const override;

    const HostedConnectivityInfo& getHostedConnectivityInfo() const { return mHostedConnectivityInfo; }

    /*! ************************************************************************************************/
    /*! \brief returns true if the endpoint is VoIP-disabled
    ***************************************************************************************************/
    virtual bool isVoipDisabled() const override { return mVoipDisabled; }

    /*! **********************************************************************************************************/
    /*! End MeshMember Interface Implementation
    **************************************************************************************************************/

    /*! ************************************************************************************************/
    /*! \brief update the connectivity info of this endpoint with the information from CC
    ***************************************************************************************************/
    void updateHostedConnectivityInfo(const HostedConnectivityInfo& hostedConnectivityInfo);

 
    void setPerformQosDuringConnection(bool doQos) { mPerformQosDuringConnection = doQos; }
    void clearPerformQosDuringConnection() { mPerformQosDuringConnection = false; }

    void setNetConnectionStatus(PlayerNetConnectionStatus connStatus) { mMeshConnectionStatus = connStatus; }
    PlayerNetConnectionStatus getNetConnectionStatus() const { return mMeshConnectionStatus; }

    const NetworkAddressList* getNetworkAddressList() const { return &mNetworkAddressList; }

     /*! ************************************************************************************************/
    /*! \brief increments the member count for this endpoint
    ***************************************************************************************************/
    virtual void incMemberCount() { ++mMemberCount; }

    /*! ************************************************************************************************/
    /*! \brief decrements the member count for this endpoint
    ***************************************************************************************************/
    virtual void decMemberCount() 
    { 
        if (mMemberCount == 0)
        {
            BlazeAssertMsg(false, "Attempted to decrement GameEndpoint member count below 0!");
            return;
        }
        --mMemberCount; 
    }

    /*! ************************************************************************************************/
    /*! \brief returns the platform type of this endpoint
    ***************************************************************************************************/
    ClientPlatformType getEndpointPlatform() const override { return mEndpointPlatform; }

    void setVoipDisabled(bool voipDisabled) { mVoipDisabled = voipDisabled; }
    void clearVoipDisabled() { mVoipDisabled = false; }

    private:
        friend class Game;
        friend class MemPool<GameEndpoint>;

        NON_COPYABLE(GameEndpoint); // disable copy ctor & assignment operator.

        GameEndpoint(Game* game, const ReplicatedGamePlayer *replicatedPlayerData, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP); // player endpoint
        
        GameEndpoint(Game* game, ConnectionGroupId connectionGroupId, SlotId connectionSlotId, const NetworkAddressList &networkAddressList, 
            const UUID & uuid, PlayerNetConnectionStatus meshConnectionStatus, MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP); //dedicated host endpoint
            
        virtual ~GameEndpoint();

        NetworkAddressList* getNetworkAddressList() { return &mNetworkAddressList; }

        uint32_t getConnectivityIdHelper(bool local) const;
    private:
        
        ConnectionGroupId mConnectionGroupId;
        SlotId mConnectionSlotId; // Connection group slot id
        
        // Note: after the removal of XLSP from the deprecation of Xbox360 (Gen3 removal), the notion of XboxServerAddr goes away and hence the "list"ness of NetworkAddress is no longer required.
        // At the next opportunity we will make it not be a list.
        NetworkAddressList mNetworkAddressList;
        UUID mUUID;
        bool mPerformQosDuringConnection;
        PlayerNetConnectionStatus mMeshConnectionStatus;
        uint8_t mMemberCount;
        Game *mGame;

        HostedConnectivityInfo mHostedConnectivityInfo;

        bool mVoipDisabled;

        mutable char8_t mTunnelKey[MAX_TUNNELKEY_LEN];
        ClientPlatformType mEndpointPlatform;
    };
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_GAMEENDPOINT_H

