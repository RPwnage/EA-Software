/*************************************************************************************************/
/*!
    \file networkmeshadapter.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_GAME_NETWORK_ADAPTER_H
#define BLAZE_GAME_NETWORK_ADAPTER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/game/netgamelink.h"

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/component/gamemanagercomponent.h" // for GameId
#include "BlazeSDK/dispatcher.h"
#include "BlazeSDK/idler.h"
#include "BlazeSDK/mesh.h"
#include "BlazeSDK/blaze_eastl/hash_map.h"
#include "EASTL/bitset.h"
#include "EASTL/set.h"

namespace Blaze
{
    class NetworkAddress;

namespace BlazeNetworkAdapter
{
    class Game;
    class Player;
    class NetworkMeshAdapterListener;
    class NetworkMeshAdapterUserListener;

    struct BLAZESDK_API MeshEndpointPinStat
    {
        MeshEndpointPinStat() :
            DistProcTime(0),
            DistWaitTime(0),
            RpackLost(0),
            LpackLost(0),
            Idpps(0),
            Odpps(0),
            LatencyMs(0),
            JitterMs(0),
            Ipps(0),
            Opps(0),
            Lnaksent(0),
            Rnaksent(0),
            Lpacksaved(0),
            bDistStatValid(false)
        {};
      
        // metrics we are sending to pin
        float DistProcTime;
        float DistWaitTime;
        float RpackLost; 
        float LpackLost;
        uint32_t Idpps;
        uint32_t Odpps;
        uint32_t LatencyMs;
        uint32_t JitterMs;
        uint32_t Ipps;
        uint32_t Opps;
        uint32_t Lnaksent;
        uint32_t Rnaksent;
        uint32_t Lpacksaved;
       
        // are samples valid
        bool bDistStatValid;
    };

    struct BLAZESDK_API MeshEndpointQos
    {
    public:
        MeshEndpointQos():
            LatencyMs(0),
            RemotePacketsSent(0),
            LocalPacketsReceived(0),
            LocalPacketsLost(0)
        {};

        MeshEndpointQos(uint32_t latencyMs, uint32_t remotePacketSent, uint32_t localPacketsReceived):
            LatencyMs(latencyMs),
            RemotePacketsSent(remotePacketSent),
            LocalPacketsReceived(localPacketsReceived),
            LocalPacketsLost(remotePacketSent-localPacketsReceived)
        {};

        float getPacketLossPercent() { return (RemotePacketsSent == 0) ? 0.0f : ((LocalPacketsLost/(float)RemotePacketsSent)*100.0f); }

        uint32_t LatencyMs;
        uint32_t RemotePacketsSent;
        uint32_t LocalPacketsReceived;
        uint32_t LocalPacketsLost;
    };

    struct BLAZESDK_API MeshEndpointConnTimer
    {
    public:
        MeshEndpointConnTimer() :
            CreateSATime(0),
            ConnectTime(0),
            DemangleTime(0),
            DemangleConnectTime(0)
        {};

        MeshEndpointConnTimer(uint32_t createSATime, uint32_t connectTime, uint32_t demangleTime, uint32_t demangleConnectTime) :
            CreateSATime(createSATime),
            ConnectTime(connectTime),
            DemangleTime(demangleTime),
            DemangleConnectTime(demangleConnectTime)
        {};

        uint32_t CreateSATime;           //!< time it takes to resolve secure association (xbox one)
        uint32_t ConnectTime;            //!< time it takes for intial connection attempt 
        uint32_t DemangleTime;           //!< time it takes to attempt demangling on the connection
        uint32_t DemangleConnectTime;    //!< time it takes to attempt a connection after demangling
    };

    class BLAZESDK_API NetworkMeshAdapter : public Idler
    {
    public:
        static const uint16_t MESSAGE_FLAG_COUNT = 16;                                  // TO BE DEPRECATED - do not use
        static const uint16_t MESSAGE_FLAG_RELIABLE = 0; //!< message is sent reliably  // TO BE DEPRECATED - do not use
        typedef eastl::bitset<MESSAGE_FLAG_COUNT> MessageFlagsBitset;                   // TO BE DEPRECATED - do not use

        enum NetworkMeshAdapterError
        {
            ERR_OK,
            ERR_CONNECTION_FAILED,
            ERR_ADAPTER_NOT_INITIALIZED,
            ERR_CANNOT_INIT_NETWORK,
            ERR_PLAYER_EXIST,
            ERR_BAD_INPUT,
            ERR_NOT_IMPLEMENTED,
            ERR_INTERNAL
        };

        struct Config
        {
            Config() : QosDurationInMs(0), QosIntervalInMs(50), QosPacketSize(1200), CreatorUserIndex(0) {}
            uint32_t QosDurationInMs;      // duration of NetGameLink QoS, a value of 0 disables QoS
            uint32_t QosIntervalInMs;      // interval between QoS packet sends
            uint32_t QosPacketSize;        // the size of a QoS packet, must be > 50 && < 1200
            uint32_t CreatorUserIndex;     // the creating local user's index. Use 0 if Multiple Local Users is not used.
        };

    public:

        /*! ************************************************************************************************/
        /*! \brief return a log-able string given a NetworkMeshAdapterError enum value.
        ***************************************************************************************************/
        static const char8_t* NetworkMeshAdapterErrorToString(NetworkMeshAdapterError networkMeshAdapterError);

        /*! ************************************************************************************************/
        /*!
            \brief Hook for the networking module to tear itself down.
            Note: destroy is called once per GameManagerApi instance.
        ***************************************************************************************************/
        virtual void destroy() = 0;

        
        /*! ************************************************************************************************/
        /*!
            \brief Prepares the adapter for use.
            \param[in] blazeHub - the interface to blaze vars
        ***************************************************************************************************/

        virtual void initialize(BlazeHub* blazeHub) = 0;

        /*! ************************************************************************************************/
        /*!
            \brief Returns true once if the game network adapter has had initalize called on it

            \return bool - true once the game network adapter has had initialze called.
        ***************************************************************************************************/

        virtual bool isInitialized() const = 0;

        /*! ************************************************************************************************/
        /*!
            \brief This method will return the local networking information for the primary user
            \return NetworkAddress - the networking information
        ***************************************************************************************************/
         virtual const Blaze::NetworkAddress& getLocalAddress() const = 0;

        /*! ************************************************************************************************/
        /*!
            \brief Returns true once the game network adapter has finished initializing 
                   for a specific mesh.

            \param[in] mesh - the mesh that contains the connapi that's initialized
            \return bool - true once the game network adapter has finished initializing.
        ***************************************************************************************************/
        virtual bool isInitialized(const Mesh* mesh) const = 0;

        /*! ************************************************************************************************/
        /*! \brief Returns a string identifying the adapter name (and possibly version info), used to identify the adapter
                    while logging.

            \return a log-able string describing the adapter version: ex: "ConnApiAdapter 1.3".
        ***************************************************************************************************/        
        virtual const char8_t* getVersionInfo() const = 0;

        /*! ****************************************************************************/
        /*! \brief The NetworkMeshAdapter is idled by BlazeHub's idle.  This is where you should
            'tick' your game networking.

            \param currentTime The tick count of the system in milliseconds.
            \param elapsedTime The elapsed count of milliseconds since the last idle call.
        ********************************************************************************/
        virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime) = 0;

        /*! ****************************************************************************/
        /*! \brief Registers/Unregisters local user with voip sub-system.

            \param userIndex the userIndex of the local user to be registered
            \return true if success
        ********************************************************************************/
        virtual bool registerVoipLocalUser(uint32_t userIndex) { return(false); }
        virtual bool unregisterVoipLocalUser(uint32_t userIndex) { return(false); }

        /*! ****************************************************************************/
        /*! \name Game network setup/teardown methods
        ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Sets up the underlying networking for a game, 
            invokes NetworkMeshAdapterListener::networkMeshCreated

            \param mesh Pointer to the interface to the gamemanager network adapter.
            \param config - A Config instance with desired values filled out.
        ********************************************************************************/
        virtual void initNetworkMesh(const Mesh *mesh, const Config &config) = 0;
 
        /*! ****************************************************************************/
        /*! \brief Creates a network connection between the local machine and the given endpoint.
            Should only be called once between each endpoint.  
            invokes NetworkMeshAdapterListener::connectedToEndpoint

            \param meshEndpoint Endpoint to connect to.
        ********************************************************************************/
        virtual void connectToEndpoint(const MeshEndpoint *meshEndpoint) = 0;

        /*! ************************************************************************************************/
        /*! \brief Adds a user specified by the MeshMember to their machine.  Enables multiple
            users on a single endpoint.
        
            \param[in] user - the user being added to their machine endpoint.
        ***************************************************************************************************/
        virtual void addMemberOnEndpoint(const MeshMember* mestMember) = 0;

        /*! ************************************************************************************************/
        /*! \brief Removes a user specified by MeshMember from their machine.
        
            \param[in] user - the user being removed.
        ***************************************************************************************************/
        virtual void removeMemberOnEndpoint(const MeshMember* meshMember) = 0;

        /*! ****************************************************************************/
        /*! \brief Destroys the network connection between the local user, and the given player.
            invokes NetworkMeshAdapterListener::disconnectedFromEndpoint

            \param meshEndpoint Endpoint to disconnect from.
        ********************************************************************************/
        virtual void disconnectFromEndpoint(const MeshEndpoint *meshEndpoint) = 0;

        /*! ****************************************************************************/
        /*! \brief Tears down the underlying networking for a specific game.
            invokes NetworkMeshAdapterListener::networkMeshDestroyed

            \param mesh The pointer to the interface to game manager network adapter.
        ********************************************************************************/
        virtual void destroyNetworkMesh(const Mesh *mesh) = 0;

        /*! ****************************************************************************/
        /*! \brief Start a game session. Mainly for supporting xbox network.

            \param mesh The pointer to the interface to game manager network adapter.
        ********************************************************************************/
        virtual void startGame(const Mesh *mesh) = 0;
        /*! ****************************************************************************/
        /*! \brief End a game session. Mainly for supporting xbox network.

            \param mesh The pointer to the interface to game manager network adapter.
        ********************************************************************************/
        virtual void endGame(const Mesh *mesh) = 0;

        /*! ****************************************************************************/
        /*! \brief  Inform the game has been reset, and will need to start hosting again.
            
            \param mesh The pointer to the interface to game manager network adapter.
        ********************************************************************************/
        virtual void resetGame(const Mesh *mesh) = 0;

        /*! ****************************************************************************/
        /*! \brief DEPRECATED - This functionality is handled server side now by default. Stock Blaze does not handle it
            Enable or disable presence for the specified mesh.
            
            \param mesh The pointer to the interface to game manager network adapter.
            \param bPresenceEnabled Flag to enable or disable presence management on the mesh.
        ********************************************************************************/
        virtual void setPresence(const Mesh *mesh, bool bPresenceEnabled) {}

        /*! ****************************************************************************/
        /*! \brief Gets the Pin statistics for a given endpoint

            \param endpoint The target endpoint
            \param pinStatData a struct to contain the qos values
            \param period sampling periond in ms
            \return a bool true, if the call was successful
        ********************************************************************************/
        virtual bool getPinStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, MeshEndpointPinStat &pinStatData, uint32_t period) { return false; }

        /*! ****************************************************************************/
        /*! \brief Gets the QOS statistics for a given endpoint

            \param endpoint The target endpoint
            \param qosData a struct to contain the qos values
            \return a bool true, if the call was successful
        ********************************************************************************/
        virtual bool getQosStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, MeshEndpointQos &qosData, bool bInitialQosStats = false) = 0;

        /*! ****************************************************************************/
        /*! \brief Gets NetGameLink statistics for a given endpoint

            \param endpoint The target endpoint
            \param gameLinkStat a struct to contain the NetGameLink Stats
            \return a bool true, if the call was successful
        ********************************************************************************/
        virtual bool getGameLinkStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, NetGameLinkStatT &netgamelinkStat) { return false; }

        /*! ****************************************************************************/
        /*! \brief Gets NetGameLink statistics for a given endpoint

            \param endpoint The target endpoint
            \param tunnelStatSend a struct to contain the Send ProtoTunnel Stats
            \param tunnelStatSend a struct to contain the Recv ProtoTunnel Stats
            \return a bool true, if the call was successful
        ********************************************************************************/
        virtual bool getPrototunnelStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, ProtoTunnelStatT &tunnelStatSend, ProtoTunnelStatT &tunnelStatRecv) { return false; }

        /*! ****************************************************************************/
        /*! \brief Gets ConnApi connection timers for a given endpoint

        \param endpoint The target endpoint
        \param gameConnectTimers a struct to contain the game ConnApi connection timers
        \param voipConnectTimers a struct to contain the voip ConnApi connection timers
        \return a bool true, if the call was successful
        ********************************************************************************/
        virtual bool getConnectionTimersForEndpoint(const Blaze::MeshEndpoint *endpoint, MeshEndpointConnTimer &gameConnectTimers, MeshEndpointConnTimer &voipConnectTimers) = 0;

        /*! ****************************************************************************/
        /*! \brief Migrate the topology host of a specific game,
            invokes NetworkMeshAdapterListener::migratedTopologyHost

            \param mesh The pointer to the interface to game manager network adapter.
        ********************************************************************************/
        virtual void migrateTopologyHost(const Mesh *mesh) = 0;
        
        /*! ****************************************************************************/
        /*! \brief Migrate the platform host of a specific game,
            invokes NetworkMeshAdapterListener::migratedPlatformHost

            \param mesh The pointer to the interface to game manager network adapter.
        ********************************************************************************/
        virtual void migratePlatformHost(const Mesh *mesh) = 0;

        /*! ****************************************************************************/
        /*! \brief Update the capacity of the mesh based on the new values in the mesh
            \note This functionality is handled server side now by default. Stock Blaze does not handle it
            \param mesh The pointer to the interface to game manager network adapter.
        ********************************************************************************/
        virtual NetworkMeshAdapterError updateCapacity(const Mesh *mesh) { return ERR_OK; }

        /*! ****************************************************************************/
        /*! \brief Locks the first party session.
            \note This functionality is handled server side now by default. Stock Blaze does not handle it
            \param mesh The pointer to the interface to game manager network adapter.
            \param bLock  set to true to lock and false to unlock the first party session
        ********************************************************************************/
        virtual NetworkMeshAdapterError lockSession(const Mesh *mesh, bool bLock) { return ERR_OK; }

        /*! ****************************************************************************/
        /*! \brief Set the local users' active first party session to be the game's. For server driven sessions only.
            \param mesh The pointer to the interface to game manager network adapter.
        ********************************************************************************/
        virtual NetworkMeshAdapterError setActiveSession(const Mesh* mesh) { return ERR_NOT_IMPLEMENTED; }

        /*! ****************************************************************************/
        /*! \brief Clear the local users' current active first party session. For server driven sessions only.
        ********************************************************************************/
        virtual NetworkMeshAdapterError clearActiveSession() { return ERR_NOT_IMPLEMENTED; }

        virtual void getConnectionStatusFlagsForMeshEndpoint(const MeshEndpoint *meshEndpoint, uint32_t& connStatFlags) = 0;

        /*! ****************************************************************************/
        /*! \brief Establish the voip connection that has been delayed because of QoS
            validation.

            When QoS validation is enabled in matchmaking we need to insure that before
            any connections are establish we have validated that we have a good connection.
            Otherwise voip connection is established but then the connection will get
            cut off if validation fails.              

            \param endpoint What endpoint we are establishing the voip connection on
        ********************************************************************************/
        virtual void activateDelayedVoipOnEndpoint(const MeshEndpoint* endpoint) = 0;


        /*! ****************************************************************************/
        /*! \name Listener methods
        ********************************************************************************/

        /*! ****************************************************************************/
        /*! \brief Adds a listener to receive notifications regarding games.

            \param listener The listener.
        ********************************************************************************/
        void addListener(NetworkMeshAdapterListener *listener) { mListenerDispatcher.addDispatchee(listener); }

        /*! ****************************************************************************/
        /*! \brief Adds a listener to receive notifications intended for the user of the sdk

            \param listener The listener.
        ********************************************************************************/
        void addUserListener(NetworkMeshAdapterUserListener *listener) { mUserListenerDispatcher.addDispatchee(listener); }

        /*! ****************************************************************************/
        /*! \brief Removes a listener to receive notifications regarding games.

            \param listener The listener.
        ********************************************************************************/
        void removeListener(NetworkMeshAdapterListener *listener) { mListenerDispatcher.removeDispatchee(listener); }

        /*! ****************************************************************************/
        /*! \brief Removes a listener to receive notifications intended for the user of the sdk

            \param listener The listener.
        ********************************************************************************/
        void removeUserListener(NetworkMeshAdapterUserListener *listener) { mUserListenerDispatcher.removeDispatchee(listener); }
        
        /*! ****************************************************************************/
        /*! \name Destructor
        ********************************************************************************/
        virtual ~NetworkMeshAdapter() {}

        protected:
            Dispatcher<NetworkMeshAdapterListener> mListenerDispatcher;
            Dispatcher<NetworkMeshAdapterUserListener> mUserListenerDispatcher;

    };


    class NetworkMeshHelper
    {
    public:
        NetworkMeshHelper(MemoryGroupId memGroupId, NetworkMeshAdapter* networkAdapter);
        virtual ~NetworkMeshHelper();

        /*! ************************************************************************************************/
        /*! \brief Initializes the network mesh, and tracks the local users connection group if the local user
            is part of the game.
        
            \param[in] mesh - the mesh to initialize
            \param config - A Config instance with desired values filled out.
        ***************************************************************************************************/
        void initNetworkMesh(Mesh *mesh, const NetworkMeshAdapter::Config &config);

        /*! ************************************************************************************************/
        /*! \brief Disconnects the local user from the provided user.  Handles calling the network adapter
            to disconnect from a remote endpoint, or just remove the user on an endpoint.
        
            \param[in] localUser - the local user
            \param[in] user - the user to connect to (could also be the local user).
        ***************************************************************************************************/
        bool disconnectFromUser(const MeshMember* localUser, const MeshMember* user);

        /*! ************************************************************************************************/
        /*! \brief Removes mesh member from endpoint if they are not the last mesh member from mesh
            endpoint. Returns true, if they are the last member, false otherwise. 

            \param[in] user - the user to remove.
        ***************************************************************************************************/
        bool removeMemberFromMeshEndpoint(const MeshMember* user);

        /*! ************************************************************************************************/
        /*! \brief Connects the local user to the provided user. Handles calling the network adapter
            to connect from the remote endpiont, or just add the user on an endpoint.  Handles various
            mesh considerations as well.
        
            \param[in] mesh - the mesh were in
            \param[in] localUser - the local user
            \param[in] user - the user to connect to (could also be the local user).
            
            \return true if we attempt to physically connect to a new endpoint, and false
                if we only try to add a user on an endpoint.
        ***************************************************************************************************/
        bool connectToUser(const Mesh* mesh, const MeshMember* localUser, const MeshMember* user);

    private:
        /*! ************************************************************************************************/
        /*! \brief Adds a reference against the connection group.
        
            \param[in] user - the connection group to reference's mesh member
            \return true if this is a new connection group / false if the reference was incremented
            on an existing connection group.
        ***************************************************************************************************/
        bool referenceConnGroup(const MeshMember& user);

        /*! ************************************************************************************************/
        /*! \brief Removes a reference against the connection group.
        
            \param[in] user - the connection group to remove reference for's mesh member
            \return true if this is the last reference for the connection group / false if the reference
            was decremented for an exsting connection group.
        ***************************************************************************************************/
        bool dereferenceConnGroup(const MeshMember& user);
        
    private:
        NetworkMeshAdapter *mNetworkAdapter;

        typedef eastl::pair<uint64_t, BlazeId> MeshIdMemberIdPair;
        typedef eastl::set<MeshIdMemberIdPair> MeshIdMemberIdPairSet;//set prevents double counting
        typedef hash_map< uint64_t, MeshIdMemberIdPairSet, eastl::hash<uint64_t> > ConnectionGroupMap;
        ConnectionGroupMap mConnectionGroupMap;

    };

} //namespace Blaze
} //namespace GameManager
#endif //BLAZE_GAME_NETWORK_ADAPTER_H
