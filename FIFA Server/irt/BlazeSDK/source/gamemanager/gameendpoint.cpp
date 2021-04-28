/*! ************************************************************************************************/
/*!
    \file gameendpoint.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/gamemanager/gameendpoint.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/shared/framework/util/blazestring.h"


namespace Blaze
{

namespace GameManager
{
    // replicatedPlayerData here is only used to access the information global to all the local players on this endpoint.
    GameEndpoint::GameEndpoint(Game* game, const ReplicatedGamePlayer *replicatedPlayerData, MemoryGroupId memGroupId /* = MEM_GROUP_FRAMEWORK_TEMP*/)
        : mConnectionGroupId(replicatedPlayerData->getConnectionGroupId()),
          mConnectionSlotId(replicatedPlayerData->getConnectionSlotId()),
          mNetworkAddressList(memGroupId, MEM_NAME(memGroupId, "GameEndpoint::mNetworkAddressList")),
          mPerformQosDuringConnection(false),
          mMeshConnectionStatus(ESTABLISHING_CONNECTION),
          mMemberCount(0),
          mGame(game),
          mVoipDisabled(false),
          mEndpointPlatform(replicatedPlayerData->getPlatformInfo().getClientPlatform())
    {
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");
        
        NetworkAddress* networkAddress = mNetworkAddressList.allocate_element();
        replicatedPlayerData->getNetworkAddress().copyInto(*networkAddress);
        mNetworkAddressList.push_back(networkAddress);

        mUUID.set(replicatedPlayerData->getUUID());
        memset(mTunnelKey, 0, sizeof(mTunnelKey));
    }

    GameEndpoint::GameEndpoint(Game* game, ConnectionGroupId connectionGroupId, SlotId connectionSlotId, const NetworkAddressList &networkAddressList, 
        const UUID &uuid, PlayerNetConnectionStatus meshConnectionStatus, MemoryGroupId memGroupId /* = MEM_GROUP_FRAMEWORK_TEMP*/)
        : mConnectionGroupId(connectionGroupId),
        mConnectionSlotId(connectionSlotId),
        mNetworkAddressList(memGroupId, MEM_NAME(memGroupId, "GameEndpoint::mNetworkAddressList")),
        mPerformQosDuringConnection(false),
        mMeshConnectionStatus(meshConnectionStatus),
        mMemberCount(0),
        mGame(game),
        mVoipDisabled(false),
        mEndpointPlatform(INVALID)
    {
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");
        
        networkAddressList.copyInto(mNetworkAddressList);
        // dedicated server end points have no hosted network address
        
        mUUID.set(uuid);
        memset(mTunnelKey, 0, sizeof(mTunnelKey));
    }

    GameEndpoint::~GameEndpoint()
    {
        BlazeAssertMsg((mMemberCount == 0), "GameEndpoint deleted with a non-0 mMemberCount!");
    }

    /*! **********************************************************************************************************/
    /*! \name MeshEndpoint Interface Implementation

    Clients in a game implement the MeshEndpoint interface so they can interact with a NetworkMeshAdapter instance.
    The Mesh interface corresponds to the game, a MeshEndpoint to a endpoint in the game mesh, while the MeshMember interface corresponds to a Player.
        
    **************************************************************************************************************/

    
    /*! ************************************************************************************************/
    /*! \brief returns true if this endpoint is the local machine. Other local users will share this Endpoint
        will have the same connection group id.
    ***************************************************************************************************/
    bool GameEndpoint::isLocal() const
    {
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");

        if ( mGame == nullptr )
            return false;

        return (mGame->getLocalMeshEndpoint() == this);
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns true if this endpoint is the topology host for this mesh.
        \return    True if this is the network host.
    **************************************************************************************************************/
    bool GameEndpoint::isTopologyHost() const
    {
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");
        
        if ( mGame == nullptr )
            return false;

        return ( mGame->getTopologyHostMeshEndpoint() == this );
    }

    /*! **********************************************************************************************************/
    /*! \brief Returns true if this endpoint is the dedicated server in this mesh 
        \return    True if this is the network host.
    **************************************************************************************************************/
    bool GameEndpoint::isDedicatedServerHost() const
    {
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");
        
        if ( mGame == nullptr )
            return false;

        return ( mGame->getDedicatedServerHostMeshEndpoint() == this );
    }

    /*! ************************************************************************************************/
    /*! \brief returns the network address of the endpoint. If remote end point and connectivity is hosted, the network address of hosted resource is returned.
    ***************************************************************************************************/
    const NetworkAddress* GameEndpoint::getNetworkAddress() const 
    { 
        if (!isLocal() && isConnectivityHosted())
        {
            return &mHostedConnectivityInfo.getHostingServerNetworkAddress();
        }
        else if (!mNetworkAddressList.empty())
        {
            return mNetworkAddressList.front();
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief returns the tunnel key for this mesh endpoint and the calling mesh endpoint
    ***************************************************************************************************/
    const char8_t* GameEndpoint::getTunnelKey() const 
    { 
        if (isLocal() && isDedicatedServerHost())
        {
            return nullptr;
        }

        BlazeAssertMsg(!isLocal(), "The method can not be called on the local end point.");
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");

        if (isLocal() || (mGame == nullptr) || (mGame->getLocalMeshEndpoint() == nullptr))
        {
            return nullptr;
        }
        
        const UUID localUUID(mGame->getLocalMeshEndpoint()->getUUID());
        const UUID gameUUID(mGame->getUUID());

        if (isConnectivityHosted())
        {
            if (gameUUID < localUUID)
            {
                blaze_snzprintf(mTunnelKey, sizeof(mTunnelKey), "%s-%s-%s", gameUUID.c_str(), gameUUID.c_str(), localUUID.c_str());
            }
            else
            {
                blaze_snzprintf(mTunnelKey, sizeof(mTunnelKey), "%s-%s-%s", gameUUID.c_str(), localUUID.c_str(), gameUUID.c_str());
            }

            BLAZE_SDK_DEBUGF("Tunnel key for Endpoint with hosted connectivity in Connection Slot(%u) is computed as (%s).\n", getConnectionSlotId(), mTunnelKey);
        }
        else
        {
            const UUID& remoteUUID = mUUID;
            if (remoteUUID < localUUID)
            {
                blaze_snzprintf(mTunnelKey, sizeof(mTunnelKey), "%s-%s-%s", gameUUID.c_str(), remoteUUID.c_str(), localUUID.c_str());
            }
            else
            {
                blaze_snzprintf(mTunnelKey, sizeof(mTunnelKey), "%s-%s-%s", gameUUID.c_str(), localUUID.c_str(), remoteUUID.c_str());
            }
    
            BLAZE_SDK_DEBUGF("Tunnel key for Endpoint without hosted connectivity in Connection Slot(%u) is computed as (%s).\n", getConnectionSlotId(), mTunnelKey);
        }
            
        return mTunnelKey; 
    }
    
    /*! ************************************************************************************************/
    /*! \brief returns true if the endpoint is being serviced by CC
    ***************************************************************************************************/
    bool GameEndpoint::isConnectivityHosted() const 
    { 
        BlazeAssertMsg(!isLocal(), "The method can not be called on the local end point.");
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");

        if (mGame && !mGame->isDedicatedServer() && mHostedConnectivityInfo.getHostingServerNetworkAddress().getActiveMember() != NetworkAddress::MEMBER_UNSET)
        {
            return true;
        }
        
        return false; 
    }
    
    uint32_t GameEndpoint::getConnectivityIdHelper(bool local) const
    {
        BlazeAssertMsg(!isLocal(), "The method can not be called on the local end point.");
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");

        if (isLocal() || (mGame == nullptr))
        {
            return INVALID_CONNECTION_GROUP_ID;
        }

        if (isConnectivityHosted())
        {
            return (local ? mHostedConnectivityInfo.getLocalLowLevelConnectivityId() : mHostedConnectivityInfo.getRemoteLowLevelConnectivityId());
        }
        else
        {
            if (local)
            {
                return static_cast<uint32_t>((mGame->getLocalMeshEndpoint() != nullptr) ? mGame->getLocalMeshEndpoint()->getConnectionGroupId() : INVALID_CONNECTION_GROUP_ID);
            }
            return static_cast<uint32_t>(getConnectionGroupId());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief returns the game console identifier for replicated player's endpoint.
    ***************************************************************************************************/
    uint32_t GameEndpoint::getRemoteLowLevelConnectivityId() const 
    { 
        return getConnectivityIdHelper(false);
    }
    
     /*! ************************************************************************************************/
    /*! \brief returns the game console identifier for local player's endpoint.
    ***************************************************************************************************/
    uint32_t GameEndpoint::getLocalLowLevelConnectivityId() const
    { 
        return getConnectivityIdHelper(true);
    }

/*! ************************************************************************************************/
    /*! \brief returns the CC identifier for hosting server providing the connectivity to this endpoint.
    ***************************************************************************************************/
    uint32_t GameEndpoint::getHostingServerConnectivityId() const
    {
        BlazeAssertMsg(!isLocal(), "The method can not be called on the local end point.");
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");

        if (isLocal() || (mGame == nullptr) || !isConnectivityHosted())
        {
            return INVALID_CONNECTION_GROUP_ID;
        }
        
        return mHostedConnectivityInfo.getHostingServerConnectivityId();
    }

    /*! ************************************************************************************************/
    /*! \brief returns the CC connection set id for hosting server providing the connectivity to this endpoint.
    ***************************************************************************************************/
    const char8_t* GameEndpoint::getHostingServerConnectionSetId() const
    {
        BlazeAssertMsg(!isLocal(), "The method can not be called on the local end point.");
        BlazeAssertMsg(mGame != nullptr, "Game can not be nullptr");

        if (isLocal() || (mGame == nullptr) || !isConnectivityHosted())
        {
            return "";
        }

        return mHostedConnectivityInfo.getHostingServerConnectionSetId();
    }
    /*! **********************************************************************************************************/
    /*! End MeshMember Interface Implementation
    **************************************************************************************************************/

    /*! ************************************************************************************************/
    /*! \brief update the connectivity info of this endpoint with the information from CC
    ***************************************************************************************************/
    void GameEndpoint::updateHostedConnectivityInfo(const HostedConnectivityInfo& hostedConnectivityInfo)
    {
        hostedConnectivityInfo.copyInto(mHostedConnectivityInfo);
    }
} // namespace GameManager
} // namespace Blaze


