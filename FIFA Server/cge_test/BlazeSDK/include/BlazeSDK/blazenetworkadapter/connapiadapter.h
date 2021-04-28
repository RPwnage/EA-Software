/*************************************************************************************************/
/*!
\file connapiadapter.h


\attention
(c) Electronic Arts. All Rights Reserved.

*************************************************************************************************/

#ifndef BLAZE_NETWORKADAPTER_CONNAPIADAPTER_H
#define BLAZE_NETWORKADAPTER_CONNAPIADAPTER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtysessionmanager.h"

#include "DirtySDK/game/connapi.h"

#include "BlazeSDK/blaze_eastl/hash_map.h"
#include "BlazeSDK/blaze_eastl/string.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/player.h"

struct ProtoTunnelRefT;

namespace Blaze
{

class ConnApiVoipManager;

namespace BlazeNetworkAdapter
{

class Network;

class BLAZESDK_API ConnApiAdapterConfig
{
public:
    ConnApiAdapterConfig();
    void printSettings();

    int32_t mPacketSize;                // the maximum packet size configured for the netgamelink layer must be <= NETGAME_DATAPKT_MAXSIZE otherwise it is ignored
    int32_t mMaxNumVoipPeers;           // ends up being passed as the iMaxPeers param of DirtySDK VoipStartup()
    int32_t mMaxNumTunnels;             // ends up being passed as the iMaxTunnels param of DirtySDK ProtoTunnelCreate() 
    uint16_t mVirtualGamePort;          // the port we bind to for game traffic, it is virtual in the sense that a physical socket is not created
    uint16_t mVirtualVoipPort;          // the port we bind to for voip traffic, it is virtual in the sense that a physical socket is not created
    bool mEnableDemangler;              // if we should use the demangler service to try to figure out peer's address when having issues connecting (cannot be disabled on xb1 as it is used for all connections)
    bool mEnableVoip;                   // should voip be enabled for clients, has no effect for dedicated servers. if you do not want to have voip connectivity this should be configured by in the voip topology of the game.
    int32_t mVoipPlatformSpecificParam; // parameter to pass along to VoipStartup()
    int32_t mTimeout;                   // DirtySDK timeout used when connected to another session (and VOIP) (defaults to 15*1000 ms)
    int32_t mConnectionTimeout;         // DirtySDK timeout used when establishing a connection    (defaults to 10*1000 ms)
    int32_t mUnackLimit;                // the maximum amount of unacknowledged data that can be sent in one go, used at the commudp level 
    int32_t mTunnelSocketRecvBuffSize;  // receive buffer size for the prototunnel socket. A value <= 0 will be ignored and the default used.
    int32_t mTunnelSocketSendBuffSize;  // send buffer size for the prototunnel socket. A value <= 0 will be ignored and the default used.
    bool mMultiHubTestMode;             // should commudp metadata be enabled for routing packets correctly when multiple blazehubs are in use
    bool mLocalBind;                    // should we bind the adapter locally, used for dedicated servers that don't want to advertise their external address (used for local testing).
#if !defined(EA_PLATFORM_PS4_CROSSGEN)
    DirtySessionManagerRefT* mDirtySessionManager; // the module that handles invite and session data calls on the client
#endif

    static const int32_t DEFAULT_VIRTUAL_GAME_PORT = 1899;  // default for mVirtualGamePort
    static const int32_t DEFAULT_VIRTUAL_VOIP_PORT = 6000;  // default for mVirtualVoipPort
    static const int32_t DEFAULT_MAX_NET_SIZE = 16;         // default for mMaxNumVoipPeers
    static const int32_t DEFAULT_MAX_TUNNELS = 16;          // default for mMaxNumTunnels

};//ConnApiAdapterConfig

class BLAZESDK_API ConnApiAdapterData
{
public:
    ConnApiAdapterData(const ConnApiAdapterConfig& config);
    
    //members
    ConnApiAdapterConfig mConfig;
    BlazeHub *mBlazeHub;
    ProtoTunnelRefT *mProtoTunnel;

    Dispatcher<NetworkMeshAdapterListener>* mListenerDispatcher;
    Dispatcher<NetworkMeshAdapterUserListener>* mUserListenerDispatcher;
};//ConnApiAdapterData

class BLAZESDK_API ConnApiAdapter
:   public Blaze::BlazeNetworkAdapter::NetworkMeshAdapter
{

public:
    ConnApiAdapter(ConnApiAdapterConfig config = ConnApiAdapterConfig());
    ~ConnApiAdapter();

//interfaces we implement
    virtual void destroy();
    virtual void idle(const uint32_t currentTime, const uint32_t elapsedTime);
    virtual void initialize(BlazeHub* blazeHub);
    virtual bool isInitialized() const;
    virtual bool isInitialized(const Mesh* mesh) const;
    virtual const char8_t* getVersionInfo() const;
    virtual const Blaze::NetworkAddress& getLocalAddress() const;

    virtual bool registerVoipLocalUser(uint32_t userIndex);
    virtual bool unregisterVoipLocalUser(uint32_t userIndex);

    virtual void initNetworkMesh(const Mesh* mesh, const NetworkMeshAdapter::Config &config);
    virtual void connectToEndpoint(const MeshEndpoint *meshEndpoint);
    virtual void disconnectFromEndpoint(const MeshEndpoint *meshEndpoint);
    virtual void addMemberOnEndpoint(const MeshMember* meshMember);
    virtual void removeMemberOnEndpoint(const MeshMember* meshMember);
    virtual void destroyNetworkMesh(const Mesh *mesh);
    virtual void startGame(const Mesh *mesh);
    virtual void endGame(const Mesh *mesh);
    virtual void resetGame(const Mesh *mesh);
    virtual void migrateTopologyHost(const Mesh *mesh);
    virtual void migratePlatformHost(const Mesh *mesh);

    virtual bool getPinStatisticsForEndpoint(const Blaze::MeshEndpoint *pEndpoint, MeshEndpointPinStat &PinStatData, uint32_t uPeriod);
    virtual bool getQosStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, MeshEndpointQos &qosData, bool bInitialQosStats = false);
    virtual bool getGameLinkStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, NetGameLinkStatT &netgamelinkStat);
    virtual bool getPrototunnelStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, ProtoTunnelStatT &tunnelStatSend, ProtoTunnelStatT &tunnelStatsRecv);
    virtual bool getConnectionTimersForEndpoint(const Blaze::MeshEndpoint *endpoint, MeshEndpointConnTimer &gameConnectTimers, MeshEndpointConnTimer &voipConnectTimers);

    virtual void activateDelayedVoipOnEndpoint(const MeshEndpoint* endpoint);

    virtual NetworkMeshAdapterError setActiveSession(const Mesh* mesh);
    virtual NetworkMeshAdapterError clearActiveSession();

    virtual void getConnectionStatusFlagsForMeshEndpoint(const MeshEndpoint *meshEndpoint, uint32_t& connStatFlags);

// API offered
    ConnApiRefT* getConnApiRefT(const Mesh* mesh, bool bExpectNull = false) const;
    ProtoTunnelRefT* getTunnelRefT() const; 

    // Cross gen ps4 uses PS5 PlayerSessions instead of NpSessions
#if !defined(EA_PLATFORM_PS4_CROSSGEN)
    DirtySessionManagerRefT* getDirtySessionManagerRefT() const;
#endif

    void flushTunnel() const;
    static bool isVoipRunning();
    static int32_t getVoipRefCount();
    static const char8_t* getTunnelVersion();

    // Get the MeshEndpoint from the Player object, or use Game::getDedicatedServerHostMeshEndpoint to get the dedicated server (for non-peer hosted topologies)
    NetGameDistRefT *getNetGameDistForEndpoint(const Blaze::MeshEndpoint *endpoint) const;
    NetGameLinkRefT *getNetGameLinkForEndpoint(const MeshEndpoint *endpoint) const;
    const ConnApiClientT* getClientHandleForEndpoint(const MeshEndpoint* meshEndpoint) const;

private:
    Network* findNetwork(const Mesh* mesh, bool bExpectNull = false) const;
    Network* findNetwork(const MeshEndpoint* meshEndpoint) const;
    void createVoipManager(const Mesh* mesh);
    void destroyVoipManager(const Mesh* mesh);
    void releaseResources();
    void acquireResources();
    void headsetCheck();
    void idleNetworks(const uint32_t currentTime, const uint32_t elapsedTime);
    static uint64_t getFirstPartyIdByPersonaId(uint64_t uPersonaID, void *pUserData);

    typedef Blaze::hash_map<const Mesh*,Network*,eastl::hash<const Mesh*> > NetworkMap;

    enum HeadsetStatus
    {
        HEADSET_STATUS_UNKNOWN,        
        HEADSET_STATUS_ON,
        HEADSET_STATUS_OFF
    };

    static int32_t sActiveVoipCount;
    static bool sOwnsVoip;
    static const int32_t DEFAULT_HEADSET_CHECK_RATE = 1000;

    ConnApiAdapterData mData;
    NetworkMap mNetworkMap;
    
    HeadsetStatus mLastHeadsetStatus[NETCONN_MAXLOCALUSERS];
    ConnApiVoipManager* mVoipManager;
    bool mAcquiredResources;
    bool mIsInitialized;
    bool mOwnsDirtySessionManager;
    bool mHasDoneOverrideConfigs;
};//ConnApiAdapter

}//BlazeNetworkAdapter
}//Blaze

#endif
