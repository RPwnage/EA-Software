/*! **********************************************************************************************/
/*!
    \file mesh.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_MESH_H
#define BLAZE_MESH_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/component/gamemanagercomponent.h"
#include "BlazeSDK/component/framework/tdf/network.h"
#include "BlazeSDK/component/framework/tdf/networkaddress.h"
#include "BlazeSDK/component/gamemanager/tdf/gamemanager.h"

#ifdef EA_PLATFORM_XBOXONE
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#endif

namespace Blaze
{
    namespace GameManager
    {
         class Player;
         class GameManagerAPI;
         class Game;
    } // namespace GameManager

class MeshEndpoint;
class MeshMember;


/*! **********************************************************************************************************/
/*! \class Mesh
    \brief This class represents a generic interface to the gamemanager network adapter
**************************************************************************************************************/

class BLAZESDK_API Mesh
{
public:

    /*! **********************************************************************************************************/
    /*! \name Destructor
    **************************************************************************************************************/
    virtual ~Mesh() {}

    /*! ************************************************************************************************/
    /*! \brief returns the topology hosts connection group id.
    ***************************************************************************************************/
    virtual uint64_t getTopologyHostConnectionGroupId() const = 0;

        /*! ************************************************************************************************/
    /*! \brief returns the connection slot id of the topology host 
    ***************************************************************************************************/
    virtual uint8_t getTopologyHostConnectionSlotId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the slot id of the platform host
    ***************************************************************************************************/
    virtual uint8_t getPlatformHostSlotId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the connection slot id of the platform host
    ***************************************************************************************************/
    virtual uint8_t getPlatformHostConnectionSlotId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the blaze name of the mesh
    ***************************************************************************************************/
    virtual const char *getName() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the blaze id of the mesh
    ***************************************************************************************************/
    virtual uint64_t getId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the UUID of the mesh
    ***************************************************************************************************/
    virtual const char8_t* getUUID() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the blaze id of the owner of the mesh
    ***************************************************************************************************/
    virtual BlazeId getTopologyHostId() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the topology of the mesh such as PEER to PEER or DEDICATED
    ***************************************************************************************************/
    virtual Blaze::GameNetworkTopology getNetworkTopology() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the topology of the voip mesh
    ***************************************************************************************************/
    virtual Blaze::VoipTopology getVoipTopology() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns if the mesh is changing host/owners
    ***************************************************************************************************/
    virtual bool isMigrating() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if the mesh is changing platform host/owners
    ***************************************************************************************************/
    virtual bool isMigratingPlatformHost() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if the local endpoint is the owner of the mesh.  Could be any of the
        local users.
        Integrators note: this method was previously named isHosting() in older versions of Blaze.
    ***************************************************************************************************/
    virtual bool isTopologyHost() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if the local endpoint is a dedicated server in the mesh
    ***************************************************************************************************/
    virtual bool isDedicatedServerHost() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if the local endpoint contains the platform host of the mesh.  Could be any of the
        local users
    ***************************************************************************************************/
    virtual bool isPlatformHost() const = 0;

    /*! ************************************************************************************************/
    /*! \brief DEPRECATED - No longer used. This functionality is handled server side now by default. 
        returns true if the presence session status for the current mesh is changing.
    ***************************************************************************************************/
    virtual bool isPresenceChanging() const { return false; }

    /*! ************************************************************************************************/
    /*! \brief returns the endpoint in the mesh identified by index
        \param[in] index the index of the endpoint
    ***************************************************************************************************/
    virtual const MeshEndpoint* getMeshEndpointByIndex(uint16_t index) const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the number of current endpoints (machines) in the mesh
    ***************************************************************************************************/
    virtual uint16_t getMeshEndpointCount() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the member of the mesh identified by index
        \param[in] index the index of the MeshMember
    ***************************************************************************************************/
    virtual const MeshMember* getMeshMemberByIndex(uint16_t index) const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the number of current members in the mesh
    ***************************************************************************************************/
    virtual uint16_t getMeshMemberCount() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the total number of potential members in the mesh
        \param[out] publicCapacity the number of public members the mesh will allow
        \param[out] privateCapacity the number of private members the mesh will allow
    ***************************************************************************************************/
    virtual uint16_t getMeshMemberCapacity(uint16_t &publicCapacity, uint16_t &privateCapacity) const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the total number of potential members in the first party representation 
               of the mesh.
        \param[out] publicCapacity the number of public members the mesh will allow
        \param[out] privateCapacity the number of private members the mesh will allow
    ***************************************************************************************************/
    virtual uint16_t getFirstPartyCapacity(uint16_t &publicCapacity, uint16_t &privateCapacity) const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if this is a ranked match
    ***************************************************************************************************/
    virtual bool isRanked() const = 0;
    
     /*! ************************************************************************************************/
    /*! \brief returns the member of the mesh identified by BlazeID
        \return the MeshMember if found, otherwise nullptr.
    ***************************************************************************************************/
    virtual const MeshMember* getMeshMemberById(BlazeId playerId) const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the member of the mesh identified by name
        \return the MeshMember if found, otherwise nullptr.
    ***************************************************************************************************/
    virtual const MeshMember *getMeshMemberByName(const char *name) const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the endpoint of the mesh for the given connection group.
        \return the MeshEndpoint if found, otherwise nullptr.
    ***************************************************************************************************/
    virtual const MeshEndpoint *getMeshEndpointByConnectionGroupId(ConnectionGroupId connectionGroupId) const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the endpoint address of the mesh owner
    ***************************************************************************************************/
    virtual const NetworkAddress* getNetworkAddress() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the owner/host of the mesh
    ***************************************************************************************************/
    virtual const MeshEndpoint *getTopologyHostMeshEndpoint() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the dedicated server host of the mesh (null for peer, non-failover topologies.)
    ***************************************************************************************************/
    virtual const MeshEndpoint *getDedicatedServerHostMeshEndpoint() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the platform host of the mesh.
    ***************************************************************************************************/
    virtual const MeshMember *getPlatformHostMeshMember() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns if this mesh can be migrated
    ***************************************************************************************************/
    virtual bool isHostMigrationEnabled() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the local endpoint in the mesh.
    ***************************************************************************************************/
    virtual const MeshEndpoint *getLocalMeshEndpoint() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the custom attributes of the mesh
    ***************************************************************************************************/
    virtual const Blaze::Collections::AttributeMap* getMeshAttributes() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if join in progress is specified as supported for the mesh.
    ***************************************************************************************************/
    virtual bool isJoinInProgressSupported() const = 0;

#if defined(EA_PLATFORM_PS4)
    /*! ************************************************************************************************/
    /*! \brief returns the np session id of the mesh
    ***************************************************************************************************/    
    virtual const char8_t* getNpSessionId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief DEPRECATED - No longer used. This functionality is handled server side now by default.
        returns if the mesh owns presence
    ***************************************************************************************************/    
    virtual bool getOwnsLocalPresence() const { return false; }
#endif

#if defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX)
    /*! ************************************************************************************************/
    /*! \brief returns the service config identifier for the external session.
    ***************************************************************************************************/
    virtual const char *getScid() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the external session name.
    ***************************************************************************************************/
    virtual const char *getExternalSessionName() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the external session template name.
    ***************************************************************************************************/
    virtual const char *getExternalSessionTemplateName() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the external session correlation id.
    ***************************************************************************************************/
    virtual const char *getExternalSessionCorrelationId() const = 0;
#endif // EA_PLATFORM_XBOXONE || EA_PLATFORM_XBSX
};

/*! **********************************************************************************************************/
/*! \class MeshEndpoint
    \brief This class represents a generic interface to the network adapter per physical device on the mesh.
**************************************************************************************************************/
class BLAZESDK_API MeshEndpoint
{
public:
    virtual ~MeshEndpoint() {}
    
    /*! ************************************************************************************************/
    /*! \brief returns the mesh this endpoint is part of
    ***************************************************************************************************/
    virtual const Mesh* getMesh() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns true if this endpoint is the local machine. Other local users will share this Endpoint
        will have the same connection group id.
    ***************************************************************************************************/
    virtual bool isLocal() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the network address of the current endpoint
    ***************************************************************************************************/
    virtual const NetworkAddress* getNetworkAddress() const = 0;

    /*! **********************************************************************************************************/
    /*! \brief Returns true if this endpoint is the topology host for this mesh.
        \return    True if this is the network host.
    **************************************************************************************************************/
    virtual bool isTopologyHost() const = 0;

    /*! **********************************************************************************************************/
    /*! \brief Returns true if this endpoint is the dedicated server in this mesh 
        \return    True if this is the network host.
    **************************************************************************************************************/
    virtual bool isDedicatedServerHost() const { return false; }

    /*! ************************************************************************************************/
    /*! \brief returns the connection group id.  A unique id per connection to the blaze server
        which will be equal for multiple players/members connected to Blaze on the same connection.
    ***************************************************************************************************/
    virtual uint64_t getConnectionGroupId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the slot id of the connection group
    ***************************************************************************************************/
    virtual uint8_t getConnectionSlotId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if a Qos check should happen when establishing connection to this endpoint
    ***************************************************************************************************/
    virtual bool getPerformQosDuringConnection() const { return false; }

    /*! ************************************************************************************************/
    /*! \brief returns the UUID of the mesh endpoint
    ***************************************************************************************************/
    virtual const char8_t* getUUID() const { return ""; }

   /*! ************************************************************************************************/
   /*! \brief returns the ClientPlatformType of this endpoint
   ***************************************************************************************************/
    virtual ClientPlatformType getEndpointPlatform() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the member count for this endpoint
    ***************************************************************************************************/
    virtual uint8_t getMemberCount() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the tunnel key for this mesh endpoint and the calling mesh endpoint
    ***************************************************************************************************/
    virtual const char8_t* getTunnelKey() const = 0; 
    
    /*! ************************************************************************************************/
    /*! \brief returns true if the endpoint is being serviced by CC
    ***************************************************************************************************/
    virtual bool isConnectivityHosted() const = 0;
    
    /*! ************************************************************************************************/
    /*! \brief returns the CC identifier for replicated player or dedicated host endpoint.
    ***************************************************************************************************/
    virtual uint32_t getRemoteLowLevelConnectivityId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the CC identifier for local player or dedicated host endpoint.
    ***************************************************************************************************/
    virtual uint32_t getLocalLowLevelConnectivityId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the CC identifier for hosting server providing the connectivity to this endpoint.
    ***************************************************************************************************/
    virtual uint32_t getHostingServerConnectivityId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the CC connection set id for hosting server providing the connectivity to this endpoint.
    ***************************************************************************************************/
    virtual const char8_t* getHostingServerConnectionSetId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if the endpoint is VoIP-disabled
    ***************************************************************************************************/
    virtual bool isVoipDisabled() const = 0;

};

/*! **********************************************************************************************************/
/*! \class MeshMember
    \brief This class represents a generic interface to the gamemanager network adapter such as a Player or Member.
**************************************************************************************************************/
class BLAZESDK_API MeshMember
{
public:
    virtual ~MeshMember() {}

    /*! ************************************************************************************************/
    /*! \brief returns the blaze id of the current member
    ***************************************************************************************************/
    virtual BlazeId getId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the name of the current member
    ***************************************************************************************************/
    virtual const char8_t* getName() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if this member is one of the local users.  Other local users
        will have the same connection group id.
   ***************************************************************************************************/
    virtual bool isLocal() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns true if this member is the platform host of the mesh.
   ***************************************************************************************************/
    virtual bool isPlatformHost() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the member's slot type (SLOT_PUBLIC_PARTICIPANT, SLOT_PRIVATE_PARTICIPANT, ...)
    ***************************************************************************************************/
    virtual GameManager::SlotType getMeshMemberSlotType() const = 0;

    /*! **********************************************************************************************************/
       /*! \brief Returns the local user index of the player (for their console).
           \return    The the local user index of the player (for their console).
    **************************************************************************************************************/
    virtual uint32_t getLocalUserIndex() const = 0;

       /*! ************************************************************************************************/
    /*! \brief returns the slotID of the member
    ***************************************************************************************************/
    virtual SlotId getSlotId() const = 0;

    /*! ************************************************************************************************/
    /*! \brief returns the endpoint for this mesh member 
        \return may return nullptr if this member isn’t active in the mesh (queued/reserved)
    ***************************************************************************************************/
    virtual const MeshEndpoint* getMeshEndpoint() const = 0; 
};


}// namespace Blaze

#endif  // BLAZE_MESH_H
