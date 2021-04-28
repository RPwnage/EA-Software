/*************************************************************************************************/
/*!
\file
    connapiadapter.cpp

\note
    \verbatim

    ConnApiAdapter is a specialization of the network mesh adapter. It wraps DirtySDK's connapi.

    This file mostly contains glue code in charge of mapping BlazeSDK's concepts into
    connapi's concepts. In particular, the following mappings are enforced:
        * For each BlazeSDK mesh, a connapi instance is created.
        * Each endpoint of a BlazeSDK's mesh represents a console. In an MLU scenario, if there are
          multiple players on a single console, then they will be same mesh endpoints 
          and share the same connection group id. However, the players can be individually identified by MeshMember. 
                --> For each connection group, a connapi client is created.
    \endverbatim

\attention
    (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/


#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/version.h"
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/internal/blazenetworkadapter/clientconnstats.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/blazenetworkadapter/connapivoipmanager.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/internal/blazenetworkadapter/network.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/blazesdkdefs.h"
#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"

#include "DirtySDK/voip/voip.h"
#include "DirtySDK/voip/voipgroup.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/dirtysock/dirtymem.h"

namespace Blaze
{
namespace BlazeNetworkAdapter
{
ConnApiAdapterConfig::ConnApiAdapterConfig()
:   mPacketSize(NETGAME_DATAPKT_DEFSIZE),
    mMaxNumVoipPeers(DEFAULT_MAX_NET_SIZE),
    mMaxNumTunnels(DEFAULT_MAX_TUNNELS),
    mVirtualGamePort(DEFAULT_VIRTUAL_GAME_PORT),
    mVirtualVoipPort(DEFAULT_VIRTUAL_VOIP_PORT),
    mEnableDemangler(true),
    mEnableVoip(true),
    mVoipPlatformSpecificParam(0),
    mTimeout(0),
    mConnectionTimeout(0),
    mUnackLimit(0),
    mTunnelSocketRecvBuffSize(0),
    mTunnelSocketSendBuffSize(0),
    mMultiHubTestMode(false),
    mLocalBind(false)
#if !defined(EA_PLATFORM_PS4_CROSSGEN)
    ,mDirtySessionManager(nullptr)
#endif
{   
}

void ConnApiAdapterConfig::printSettings()
{
    BLAZE_SDK_DEBUGF("[CANA] ConnApiAdapterConfig(%p)\n", this);
    BLAZE_SDK_DEBUGF("[CANA] .mPacketSize=%d\n", mPacketSize);
    BLAZE_SDK_DEBUGF("[CANA] .mMaxNumVoipPeers=%d\n", mMaxNumVoipPeers);
    BLAZE_SDK_DEBUGF("[CANA] .mMaxNumTunnels=%d\n", mMaxNumTunnels);
    BLAZE_SDK_DEBUGF("[CANA] .mVirtualGamePort=%d\n", mVirtualGamePort);
    BLAZE_SDK_DEBUGF("[CANA] .mVirtualVoipPort=%d\n", mVirtualVoipPort);
    BLAZE_SDK_DEBUGF("[CANA] .mEnableDemangler=%s\n", mEnableDemangler ? "true" : "false");
    BLAZE_SDK_DEBUGF("[CANA] .mEnableVoip=%s\n", mEnableVoip ? "true" : "false");
    BLAZE_SDK_DEBUGF("[CANA] .mVoipPlatformSpecificParam=%d\n", mVoipPlatformSpecificParam);
    BLAZE_SDK_DEBUGF("[CANA] .mTimeout=%d\n", mTimeout);
    BLAZE_SDK_DEBUGF("[CANA] .mConnectionTimeout=%d\n", mConnectionTimeout);
    BLAZE_SDK_DEBUGF("[CANA] .mUnackLimit=%d\n", mUnackLimit);
    BLAZE_SDK_DEBUGF("[CANA] .mTunnelSocketRecvBuffSize=%d\n", mTunnelSocketRecvBuffSize);
    BLAZE_SDK_DEBUGF("[CANA] .mTunnelSocketSendBuffSize=%d\n", mTunnelSocketSendBuffSize);
    BLAZE_SDK_DEBUGF("[CANA] .mMultiHubTestMode=%s\n", mMultiHubTestMode ? "true" : "false");
    BLAZE_SDK_DEBUGF("[CANA] .mLocalBind=%s\n", mLocalBind ? "true" : "false");
#if !defined(EA_PLATFORM_PS4_CROSSGEN)
    BLAZE_SDK_DEBUGF("[CANA] .mDirtySessionManager=%p\n", mDirtySessionManager);
#endif
}

ConnApiAdapterData::ConnApiAdapterData(const ConnApiAdapterConfig& config)
:   mConfig(config),
    mBlazeHub(nullptr),
    mProtoTunnel(nullptr)
{
}

int32_t ConnApiAdapter::sActiveVoipCount = 0;
bool ConnApiAdapter::sOwnsVoip = false;

ConnApiAdapter::ConnApiAdapter(ConnApiAdapterConfig config)
:   mData(config),
    mNetworkMap(MEM_GROUP_NETWORKADAPTER, MEM_NAME(MEM_GROUP_NETWORKADAPTER, "ConnApiAdapter::mNetworkMap")),
    mVoipManager(nullptr),
    mAcquiredResources(false),
    mIsInitialized(false),
    mOwnsDirtySessionManager(false),
    mHasDoneOverrideConfigs(false)
{
    config.printSettings();
    for (uint32_t iIndex = 0; iIndex < NETCONN_MAXLOCALUSERS; ++iIndex)
    {
        mLastHeadsetStatus[iIndex] = HEADSET_STATUS_UNKNOWN;
    }

    // Cross gen ps4 uses PS5 PlayerSessions instead of NpSessions
    #if (defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN))
    //be sure we have a DSM
    if (mData.mConfig.mDirtySessionManager == nullptr)
    {
        DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_NETWORKADAPTER));
        if ((mData.mConfig.mDirtySessionManager = DirtySessionManagerCreate()) == nullptr)
        {
            BLAZE_SDK_DEBUGF("[CANA] Error creating session manager.\n");
        }
        else
        {
            mOwnsDirtySessionManager = true;
        }
        DirtyMemGroupLeave();
    }
    #else
    EA_UNUSED(mOwnsDirtySessionManager);
    #endif

    #if !defined(EA_PLATFORM_WINDOWS) && !defined(EA_PLATFORM_LINUX)
    EA_UNUSED(mVoipManager);
    #endif
}

ConnApiAdapter::~ConnApiAdapter()
{
    // understanding that we _should_ never hit a case where mIsInitialized is true when calling the destructor
    // we are leaving this code in place to help document the flow is expected to behave
    //
    // title code owns the instance of the adapter but the state is being initialized and destroyed by the gamemanagerapi
    // destroy will not be called here in the following cases:
    // * initialize never called
    // * gamemanagerapi does the initialized and destroy properly
    //
    // the case where we can expect destroy to be called is if the title code destroys the adapter before gamemangerapi
    // in this case you risk the possibility of a crash since you rip the adapter from under the gamemanagerapi
    if (mIsInitialized)
    {
        destroy();
    }

    // Cross gen ps4 uses PS5 PlayerSessions instead of NpSessions
    #if (defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN))
    if ((mData.mConfig.mDirtySessionManager != nullptr) && (mOwnsDirtySessionManager == true))
    {
        DirtySessionManagerDestroy(mData.mConfig.mDirtySessionManager);
        mData.mConfig.mDirtySessionManager = nullptr;
        mOwnsDirtySessionManager = false;
    }
    #endif
}

void ConnApiAdapter::destroy()
{    
    BLAZE_SDK_DEBUGF("[CANA] destroy ConnApiAdapter for BlazeHub(%p)\n", mData.mBlazeHub); 
    
    for (NetworkMap::const_iterator it = mNetworkMap.begin(); it != mNetworkMap.end();)
    {
        BLAZE_SDK_DEBUGF("[CANA] NetworkMap, erasing %p -> %p\n", it->first, it->second);

        Network* pNetwork = it->second;
        if (pNetwork)
        {
            // we assume only one network at a time for dedicated server games so lets destroy the voipmanager
            destroyVoipManager(it->first);

            pNetwork->destroyNetwork();
        }
        it = mNetworkMap.erase(it);
    }

    if (mData.mProtoTunnel != nullptr)
    {
        ProtoTunnelDestroy(mData.mProtoTunnel);
        mData.mProtoTunnel = nullptr;
    }

    if (mAcquiredResources)
    {
        mData.mBlazeHub->getScheduler()->removeByAssociatedObject(this);
    }
    releaseResources();

    mIsInitialized = false;
}

void ConnApiAdapter::initialize(BlazeHub* pBlazeHub)
{
    BLAZE_SDK_DEBUGF("[CANA] initialize ConnApiAdapter for BlazeHub(%p)\n", pBlazeHub); 

    //cache our blaze hub ref
    mData.mBlazeHub = pBlazeHub;

    //Cache off our dispatcher for the networks managed by this adapter
    mData.mListenerDispatcher = &mListenerDispatcher;
    mData.mUserListenerDispatcher = &mUserListenerDispatcher;

    // earliest operatunity to override the configs
    if ((mData.mBlazeHub != nullptr) && !mHasDoneOverrideConfigs)
    {
        mHasDoneOverrideConfigs = true;
        //use config overrides found in util.cfg
        Blaze::Util::UtilAPI::createAPI(*mData.mBlazeHub);
        mData.mBlazeHub->getUtilAPI()->OverrideConfigs(&mData.mConfig);
        // Cross gen ps4 uses PS5 PlayerSessions instead of NpSessions
        #if (defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN))
        mData.mBlazeHub->getUtilAPI()->OverrideConfigs(mData.mConfig.mDirtySessionManager);
        #endif
    }

    //setup voip early so we can get headset status
    acquireResources();
    memset(mLastHeadsetStatus, 0, sizeof(mLastHeadsetStatus));

    //only do headset checks if we're using voip
    if (mData.mConfig.mEnableVoip)
    {
        mData.mBlazeHub->getScheduler()->scheduleMethod("headsetCheck", this, &ConnApiAdapter::headsetCheck, this, 0);
    }

    mIsInitialized = true;
}

bool ConnApiAdapter::isInitialized() const
{
    return mIsInitialized;
}

bool ConnApiAdapter::isInitialized(const Mesh* pMesh) const
{
    Network* pNetwork = findNetwork(pMesh);
    return(pNetwork != nullptr);
}


const char8_t* ConnApiAdapter::getVersionInfo() const
{
    return getBlazeSdkVersionString();
}

const Blaze::NetworkAddress& ConnApiAdapter::getLocalAddress() const
{
    BLAZE_SDK_DEBUGF("[CANA] getLocalAddress()\n");
    if (!mData.mConfig.mLocalBind)
    {
        return(*mData.mBlazeHub->getConnectionManager()->getNetworkAddress());
    }
    else
    {
        static Blaze::NetworkAddress address;
        address.switchActiveMember(Blaze::NetworkAddress::MEMBER_IPADDRESS);
        address.getIpAddress()->setIp((uint32_t)NetConnStatus('addr', 0, nullptr, 0));
        address.getIpAddress()->setPort(mData.mBlazeHub->getInitParams().GamePort);
        return(address);
    }
}

void ConnApiAdapter::startGame(const Mesh *mesh)
{
    BLAZE_SDK_DEBUGF("[CANA] startGame(%p) - no-op\n", mesh); 

} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void ConnApiAdapter::endGame(const Mesh *mesh)
{
    BLAZE_SDK_DEBUGF("[CANA] endGame(%p) - no-op\n", mesh); 

} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void ConnApiAdapter::resetGame(const Mesh *mesh)
{
    BLAZE_SDK_DEBUGF("[CANA] resetGame(%p)\n", mesh); 
    Network* network = findNetwork(mesh);
    if (network)
    {
        network->resetGame();
    }
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void ConnApiAdapter::migrateTopologyHost(const Mesh *mesh)
{
    BLAZE_SDK_DEBUGF("[CANA] migrateTopologyHost(%p)\n", mesh); 
    Network* network = findNetwork(mesh);
    if (network)
    {
        network->migrateTopologyHost();

        mListenerDispatcher.dispatch("migratedTopologyHost", &NetworkMeshAdapterListener::migratedTopologyHost, mesh, NetworkMeshAdapter::ERR_OK);
    }
}  /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void ConnApiAdapter::migratePlatformHost(const Mesh *mesh)
{
    BLAZE_SDK_DEBUGF("[CANA] migratePlatformHost(%p)\n", mesh); 
    Network* network = findNetwork(mesh);
    if (network)
    {
        mListenerDispatcher.dispatch("migratedPlatformHost", &NetworkMeshAdapterListener::migratedPlatformHost, mesh, NetworkMeshAdapter::ERR_OK);
    }
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

NetworkMeshAdapter::NetworkMeshAdapterError ConnApiAdapter::setActiveSession(const Mesh* mesh)
{
    // Cross gen ps4 uses PS5 PlayerSessions instead of NpSessions
#if (defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN))
    // To enable DirtySock invites with server driven NP sessions, DirtySessionManager's active session must be set using this method

    if (EA_UNLIKELY(mesh == nullptr))
    {
        return ERR_INTERNAL;
    }
    const char8_t* npSessionId = (mesh->getNpSessionId() != nullptr ? mesh->getNpSessionId() : "");
    if (EA_UNLIKELY(npSessionId[0] == '\0'))
    {
        BLAZE_SDK_DEBUGF("[CANA] WARNING: server driven session id was empty for mesh(%" PRIu64 "). Possibly issues calling Sony.\n", mesh->getId());
    }

    NetworkMeshAdapter::NetworkMeshAdapterError err = ERR_OK;
    BLAZE_SDK_DEBUGF("[CANA] Setting active server driven session id(%s) for mesh(%" PRIu64 ").\n", npSessionId, mesh->getId());
    if (-1 == DirtySessionManagerControl(mData.mConfig.mDirtySessionManager, 'sess', 0, 0, npSessionId))
    {
        BLAZE_SDK_DEBUGF("[CANA] ERR: did NOT set active session(%s) for mesh(%" PRIu64 "), was server driven mode disabled at the DirtySDK level?\n", npSessionId, mesh->getId());
        err = ERR_INTERNAL;
    }
    // In case anyone still listens to these (deprecated):
    mListenerDispatcher.dispatch("presenceChanged", &NetworkMeshAdapterListener::presenceChanged, mesh, err);
    mListenerDispatcher.dispatch<const Mesh*, const Blaze::NpSessionId&>("npSessionAvailable", &NetworkMeshAdapterListener::npSessionAvailable, mesh, npSessionId);
    return err;
#else
    return ERR_OK;
#endif
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

NetworkMeshAdapter::NetworkMeshAdapterError ConnApiAdapter::clearActiveSession()
{
    // Cross gen ps4 uses PS5 PlayerSessions instead of NpSessions
#if (defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS4_CROSSGEN))
    BLAZE_SDK_DEBUGF("[CANA] clearing current active session (if present).\n");
    if (mData.mConfig.mDirtySessionManager == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA] WARNING: did NOT clear any active session, DirtySessionManager was not available.\n");
        return ERR_INTERNAL;
    }
    if (-1 == DirtySessionManagerControl(mData.mConfig.mDirtySessionManager, 'sess', 0, 0, ""))
    {
        BLAZE_SDK_DEBUGF("[CANA] WARNING: did NOT clear any active session, was server driven mode disabled at the DirtySDK level?\n");
        return ERR_INTERNAL;
    }
    return ERR_OK;
#else
    return ERR_OK;
#endif
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/


void ConnApiAdapter::getConnectionStatusFlagsForMeshEndpoint(const MeshEndpoint *meshEndpoint, uint32_t& connStatFlags)
{
   NetworkMeshAdapterListener::ConnectionFlagsBitset connectionStatusFlags;
   const ConnApiClientT* pConnApiClient = getClientHandleForEndpoint(meshEndpoint);
   if (pConnApiClient != nullptr)
   {
       if (pConnApiClient->GameInfo.uConnFlags & CONNAPI_CONNFLAG_DEMANGLED)
       {
           connectionStatusFlags.set(Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::CONNECTION_FLAG_DEMANGLED);
       }
       if (pConnApiClient->GameInfo.uConnFlags & CONNAPI_CONNFLAG_PKTRECEIVED)
       {
           connectionStatusFlags.set(Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::CONNECTION_FLAG_PKTRECEIVED);
       }
       connStatFlags = connectionStatusFlags.to_uint32();
   }
   else
   {
       connStatFlags = 0;
   }
}

void ConnApiAdapter::headsetCheck()
{
    //headset status updates to the server
    for (uint32_t userIndex = 0; userIndex < NETCONN_MAXLOCALUSERS; ++userIndex)
    {
        Blaze::UserManager::LocalUser* localUser = mData.mBlazeHub->getUserManager()->getLocalUser(userIndex);
        if ((VoipGetRef() != nullptr) && mAcquiredResources && (localUser != nullptr))
        {
            int32_t iDirtySockIndex = 0;

            iDirtySockIndex =  mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex();
            HeadsetStatus newHeadsetStatus = (VoipLocalUserStatus(VoipGetRef(), iDirtySockIndex) & VOIP_LOCAL_USER_HEADSETOK ? HEADSET_STATUS_ON : HEADSET_STATUS_OFF);

            if (newHeadsetStatus != mLastHeadsetStatus[iDirtySockIndex])
            {
                if (newHeadsetStatus == HEADSET_STATUS_ON)
                {
                    localUser->getHardwareFlags()->setVoipHeadsetStatus();
                }
                else
                {
                    localUser->getHardwareFlags()->clearVoipHeadsetStatus();
                }
                localUser->updateHardwareFlags(mData.mBlazeHub);
                mLastHeadsetStatus[iDirtySockIndex] = newHeadsetStatus;
            }
        }
    }

    //end of headset status updates

    //schedule new headsetCheck
    //it's possible for VoipGetRef() to return nullptr when mAcquiredResources is true
    //we still need to schedule new headsetCheck if VoipGetRef() returned nullptr
    if (mAcquiredResources)
    {
        int32_t newHeadsetRateValue = DEFAULT_HEADSET_CHECK_RATE;
        mData.mBlazeHub->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_VOIP_HEADSET_UPDATE_RATE, &newHeadsetRateValue);
        mData.mBlazeHub->getScheduler()->scheduleMethod("headsetCheck", this, &ConnApiAdapter::headsetCheck, this, static_cast<uint32_t>(newHeadsetRateValue));
    }
}

bool ConnApiAdapter::registerVoipLocalUser(uint32_t userIndex)
{
    // set the account id within DirtySDK
    int64_t iAccountId = mData.mBlazeHub->getLoginManager(userIndex)->getAccountId();
    int64_t iPersonaId = mData.mBlazeHub->getLoginManager(userIndex)->getPersonaId();
    NetConnControl('acid', mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex(), sizeof(iAccountId), &iAccountId, nullptr);
    NetConnControl('peid', mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex(), sizeof(iPersonaId), &iPersonaId, nullptr);

    if (mData.mConfig.mEnableVoip)
    {
        if (VoipGetRef() != nullptr)
        {
            BLAZE_SDK_DEBUGF("[CANA] registering local user %d with voip sub-system\n", userIndex);
            VoipSetLocalUser(VoipGetRef(), mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex(), TRUE);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[CANA] can't register local user %d with voip sub-system because voip is not initialized\n", userIndex);
        }
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA] can't register local user %d with voip sub-system because voip is disabled\n", userIndex);
    }

    #if defined(EA_PLATFORM_WINDOWS)
    // set underage flag for PC Users
    NetConnControl('chat', mData.mBlazeHub->getLoginManager(userIndex)->getIsUnderage(), 0, nullptr, nullptr);
    #endif

    return(true);
}

bool ConnApiAdapter::unregisterVoipLocalUser(uint32_t userIndex)
{
    // clear the users AccountID from DirtySDK
    int64_t iAccountId = 0;
    int64_t iPersonaId = 0;
    NetConnControl('acid', mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex(), sizeof(iAccountId), &iAccountId, nullptr);
    NetConnControl('peid', mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex(), sizeof(iPersonaId), &iPersonaId, nullptr);

    if (mData.mConfig.mEnableVoip)
    {
        if (VoipGetRef() != nullptr)
        {
            BLAZE_SDK_DEBUGF("[CANA] unregistering local user %d from voip sub-system\n", userIndex);
            VoipGroupActivateLocalUser(nullptr, mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex(), FALSE);
            VoipSetLocalUser(VoipGetRef(), mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex(), FALSE);

            // clear the previous headset status
            int32_t iDirtySockIndex = mData.mBlazeHub->getLoginManager(userIndex)->getDirtySockUserIndex();
            mLastHeadsetStatus[iDirtySockIndex] = HEADSET_STATUS_UNKNOWN;
        }
        else
        {
            BLAZE_SDK_DEBUGF("[CANA] can't unregister local user %d with voip sub-system because voip is not initialized\n", userIndex);
        }
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA] can't unregister local user %d with voip sub-system because voip is disabled\n", userIndex);
    }

    return(true);
}

void ConnApiAdapter::initNetworkMesh(const Mesh* mesh, const NetworkMeshAdapter::Config &config)
{
    BLAZE_SDK_DEBUGF("[CANA] initNetworkMesh(%p) meshId:%" PRIu64 "\n", mesh, mesh->getId());

    // make sure number of endpoints is large enough for game max capacity
    GameManager::Game* pGame = (GameManager::Game*)mesh;

    if (pGame->getMaxPlayerCapacity() > mData.mConfig.mMaxNumVoipPeers)
    {
        if ((mData.mConfig.mEnableVoip == true) && (mesh->getVoipTopology() != VOIP_DISABLED))
        {
            BLAZE_SDK_DEBUGF("[CANA] ERR initNetworkMesh(), max number of voip peers is not large enough (%d vs %d)\n", mData.mConfig.mMaxNumVoipPeers, pGame->getMaxPlayerCapacity());
            mListenerDispatcher.dispatch("networkMeshCreated", &NetworkMeshAdapterListener::networkMeshCreated, mesh, config.CreatorUserIndex, NetworkMeshAdapter::ERR_CANNOT_INIT_NETWORK);

            return;
        }
        else
        {
            BLAZE_SDK_DEBUGF("[CANA] mesh with capacity (%d) larger than max number of voip peers (%d) allowed because it has voip disabled\n", pGame->getMaxPlayerCapacity(), mData.mConfig.mMaxNumVoipPeers);
        }
    }

    // for dedicated servers enable voip is required to be false
    if (mesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED && mesh->isTopologyHost() == true)
    {
        if (mData.mConfig.mEnableVoip == true)
        {
            BLAZE_SDK_DEBUGF("[CANA] ERR initNetworkMesh(), mEnableVoip cannot be true on a non-dirtycast dedicated server\n");
            mListenerDispatcher.dispatch("networkMeshCreated", &NetworkMeshAdapterListener::networkMeshCreated, mesh, config.CreatorUserIndex, NetworkMeshAdapter::ERR_CANNOT_INIT_NETWORK);

            return;
        }
    }
    // warn if connapiadpater voip enable setting conflicts with mesh voip setting
    else if (mData.mConfig.mEnableVoip == false && mesh->getVoipTopology() != VOIP_DISABLED)
    {
        BLAZE_SDK_DEBUGF("[CANA] WARNING initNetworkMesh(), mEnableVoip is false but the mesh does not have VOIP_DISABLED as the Voip Topology\n");
    }

    Network* pNetwork = nullptr;
    bool retVal = (findNetwork(mesh, true) == nullptr);

    // network for mesh not found, we should make one
    if (retVal)
    {
        pNetwork = Network::createNetwork();
        retVal = (pNetwork != nullptr);
        if (retVal)
        {
            mNetworkMap[mesh] = pNetwork;

            BLAZE_SDK_DEBUGF("[CANA] NetworkMap, adding %p -> %p\n", mesh, pNetwork);

            retVal = pNetwork->initNetworkMesh(mesh, &mData, config);
            if (retVal)
            {
                createVoipManager(mesh);
            }
        }
    }

    if (retVal)
    {
        mListenerDispatcher.dispatch("networkMeshCreated", &NetworkMeshAdapterListener::networkMeshCreated, mesh, config.CreatorUserIndex, NetworkMeshAdapter::ERR_OK);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA] ERR initNetworkMesh(), pNetwork=%p\n", pNetwork); 
        mListenerDispatcher.dispatch("networkMeshCreated", &NetworkMeshAdapterListener::networkMeshCreated, mesh, config.CreatorUserIndex, NetworkMeshAdapter::ERR_CANNOT_INIT_NETWORK);
    }
}

void ConnApiAdapter::connectToEndpoint(const Blaze::MeshEndpoint *meshEndpoint)
{
    BLAZE_SDK_DEBUGF("[CANA] connectToEndpoint(%p)\n", meshEndpoint); 
    Network* network = findNetwork(meshEndpoint);

    if (network)
    {
        network->connectToEndpoint(meshEndpoint);
    }

    // if the voipmanager was created pass the connect to it
    if (mVoipManager != nullptr)
    {
        mVoipManager->connectToEndpoint(meshEndpoint);
    }
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void ConnApiAdapter::disconnectFromEndpoint(const Blaze::MeshEndpoint *meshEndpoint)
{
    BLAZE_SDK_DEBUGF("[CANA] disconnectFromEndpoint(%p)\n", meshEndpoint); 
    Network* network = findNetwork(meshEndpoint);
    if (network)
    {
        Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::ConnectionFlagsBitset connectionFlags;
        const ConnApiClientT * connApiClient  = getClientHandleForEndpoint(meshEndpoint);

        //make sure client is still existing at the ConnApi level at this point
        if (connApiClient)
        {
            //set game-connection specific flags
            if (connApiClient->GameInfo.uConnFlags & CONNAPI_CONNFLAG_DEMANGLED)
            {
                connectionFlags.set(Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::CONNECTION_FLAG_DEMANGLED);
            }
            if (connApiClient->GameInfo.uConnFlags & CONNAPI_CONNFLAG_PKTRECEIVED)
            {
                connectionFlags.set(Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::CONNECTION_FLAG_PKTRECEIVED);
            }
        }
        else
        {
            /* mclouatre, march 2019
               I suspect that GOS-30708 can be caused by this code path being exercised with connApiClient = nullptr. Unfortunately,
               with a code inspection, I have not managed to find a realistic scenario that can lead to this. By adding this debug
               printf, I am hopeful that someone will eventually report a repro to me. 
               
               If after a while that repro never comes, then we should remove the nullptr check on connApiClient here because
               it is misleading. 
               
               If we do get a repro, then we should evaluate if it is correct to dispatch 'disconnectedFromEndpoin' with 
               status ERR_OK upon such circunstances. */
            BLAZE_SDK_DEBUGF("[CANA] WARNING - disconnectFromEndpoint(%p) is skipping initialization of connection flags - you may have found the root cause for GOS-30708. Please report to mclouatre@ea.com\n", meshEndpoint);
        }

        network->disconnectFromEndpoint(meshEndpoint);

        mListenerDispatcher.dispatch<const MeshEndpoint*, NetworkMeshAdapterListener::ConnectionFlagsBitset, NetworkMeshAdapterError>("disconnectedFromEndpoint", &NetworkMeshAdapterListener::disconnectedFromEndpoint, meshEndpoint,
            connectionFlags, NetworkMeshAdapter::ERR_OK);
    }

    // if the voipmanager was created pass the disconnect to it
    if (mVoipManager != nullptr)
    {
        mVoipManager->disconnectFromEndpoint(meshEndpoint);
    }
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void ConnApiAdapter::addMemberOnEndpoint(const MeshMember* meshMember)
{
    BLAZE_SDK_DEBUGF("[CANA] addMemberOnEndpoint()  %s member %s added to conn group 0x%016" PRIX64 "\n", 
        (meshMember->isLocal()?"local":"remote"), meshMember->getName(), meshMember->getMeshEndpoint()->getConnectionGroupId());
    Network* network = findNetwork(meshMember->getMeshEndpoint());

    if (network)
    {
        network->addMemberOnEndpoint(meshMember);
    }
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void ConnApiAdapter::removeMemberOnEndpoint(const MeshMember* meshMember)
{
    BLAZE_SDK_DEBUGF("[CANA] removeUserOnEndpoint()  %s member %s removed from conn group 0x%016" PRIX64 "\n",
        (meshMember->isLocal()?"local":"remote"), meshMember->getName(), meshMember->getMeshEndpoint()->getConnectionGroupId());
    Network* network = findNetwork(meshMember->getMeshEndpoint());

    if (network)
    {
        network->removeMemberOnEndpoint(meshMember);
    }
} /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

void ConnApiAdapter::destroyNetworkMesh(const Mesh *mesh)
{
    BLAZE_SDK_DEBUGF("[CANA] destroyNetworkMesh(%p) blazeid of mesh:%" PRId64 "/0x%016" PRIX64 ", name:%s\n", mesh, mesh->getId(), mesh->getId(), mesh->getName());

    NetworkMap::const_iterator it = mNetworkMap.find(mesh);
    if (it != mNetworkMap.end())
    {
        BLAZE_SDK_DEBUGF("[CANA] removing game with Blaze ID %" PRIu64 "/0x%016" PRIX64 ":%s from NetworkMap, erasing %p -> %p.\n", mesh->getId(), mesh->getId(), mesh->getName(), it->first, it->second);

        Network* pNetwork = it->second;
        if (pNetwork)
        {
            // we assume only one network at a time for dedicated server games so lets destroy the voipmanager
            destroyVoipManager(mesh);

            pNetwork->destroyNetwork();
        }
        mNetworkMap.erase(it);
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA] destroyNetworkMesh(): no such game.\n");
    }
    mListenerDispatcher.dispatch("networkMeshDestroyed", &NetworkMeshAdapterListener::networkMeshDestroyed, mesh, NetworkMeshAdapter::ERR_OK);
}

ConnApiRefT* ConnApiAdapter::getConnApiRefT(const Mesh* pMesh, bool bExpectNull) const
{
    Network* pNetwork = findNetwork(pMesh, bExpectNull);
    if (pNetwork)
    {
        return(pNetwork->getConnApiRefT());
    }
    return(nullptr);
}

ProtoTunnelRefT* ConnApiAdapter::getTunnelRefT() const
{
    return(mData.mProtoTunnel);
}

#if !defined(EA_PLATFORM_PS4_CROSSGEN)
DirtySessionManagerRefT* ConnApiAdapter::getDirtySessionManagerRefT() const
{
    return(mData.mConfig.mDirtySessionManager);
}
#endif

const ConnApiClientT* ConnApiAdapter::getClientHandleForEndpoint(const MeshEndpoint* meshEndpoint) const
{
    Network* network = findNetwork(meshEndpoint);
    if (network)
    {
        return(network->getClientHandleForEndpoint(meshEndpoint));
    } 
    BLAZE_SDK_DEBUGF("[CANA] getClientHandleForEndpoint() failed to findNetwork()\n"); 
    return(nullptr);
}

Network* ConnApiAdapter::findNetwork(const Mesh* mesh, bool bExpectNull) const
{
    BlazeAssert(mesh != nullptr);
    if (mesh == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA] findNetwork() failed, nullptr mesh\n"); 
        return nullptr;
    }

    NetworkMap::const_iterator it = mNetworkMap.find(mesh);
    if (it != mNetworkMap.end())
    {
        return(it->second);
    }
    else
    {
        if (!bExpectNull)
        {
            BLAZE_SDK_DEBUGF("[CANA] findNetwork(%p) failed\n", mesh); 
        }
        return(nullptr);
    }
}

Network* ConnApiAdapter::findNetwork(const MeshEndpoint* meshEndpoint) const
{
    BlazeAssert(meshEndpoint != nullptr);
    if (meshEndpoint == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA] findNetwork() failed, nullptr endpoint\n"); 
        return nullptr;
    }
    return findNetwork(meshEndpoint->getMesh());
}

void ConnApiAdapter::createVoipManager(const Mesh* mesh)
{
    // creation of the voip manager, voip tunnel for dedicated servers (non-dirtycast)
    // todo this solution was from  blaze 2.x but not used, is this the solution we want
#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_LINUX)
    if ((mVoipManager == nullptr) && (mesh->getVoipTopology() == VOIP_DEDICATED_SERVER)
        && (mesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED) && mesh->isTopologyHost())
    {
        mVoipManager = BLAZE_NEW(MEM_GROUP_NETWORKADAPTER, "ConnApiVoipManager") ConnApiVoipManager(mData.mBlazeHub, mesh, 
            mData.mConfig.mVirtualVoipPort, MEM_GROUP_NETWORKADAPTER);
    }
#endif
}

void ConnApiAdapter::destroyVoipManager(const Mesh* mesh)
{
#if defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_LINUX)
    // make sure that the mesh matches the one this was created for
    if ((mVoipManager != nullptr) && (mesh->getVoipTopology() == VOIP_DEDICATED_SERVER)
        && (mesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED) && mesh->isTopologyHost())
    {
        BLAZE_DELETE(MEM_GROUP_NETWORKADAPTER, mVoipManager);
        mVoipManager = nullptr;
    }
#endif
}

bool ConnApiAdapter::isVoipRunning()
{
    return(VoipGetRef() != nullptr);
}

int32_t ConnApiAdapter::getVoipRefCount()
{
    return(sActiveVoipCount);
}

const char8_t* ConnApiAdapter::getTunnelVersion()
{
    static char8_t sTunnelVersion[16] = {0};
    ds_snzprintf(sTunnelVersion, (int32_t)sizeof(sTunnelVersion), "%d.%d", PROTOTUNNEL_VERSION >> 8, PROTOTUNNEL_VERSION & 0xFF);
    return(sTunnelVersion);
}

uint64_t ConnApiAdapter::getFirstPartyIdByPersonaId(uint64_t uPersonaID, void *pUserData)
{
    ConnApiAdapter *pConnApiAdapter = (ConnApiAdapter *)pUserData;
    Blaze::UserManager::UserManager *pUserManager = pConnApiAdapter->mData.mBlazeHub->getUserManager();
    const Blaze::UserManager::User *pUser = pUserManager->getUserById(uPersonaID);
    if (pUser != nullptr)
    {
        #if !defined(EA_PLATFORM_PS4) && !defined(EA_PLATFORM_PS5) && !defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBSX) && !defined(EA_PLATFORM_STADIA)
        // we will just return the persona id which is unique
        return(uPersonaID);
        #else
        return(pConnApiAdapter->mData.mBlazeHub->getExternalIdFromPlatformInfo(pUser->getPlatformInfo()));
        #endif
    }
    else
    {
        return(0);
    }
}

void ConnApiAdapter::acquireResources()
{
    BLAZE_SDK_DEBUGF("[CANA] acquireResources()\n"); 

    if (mData.mConfig.mEnableVoip)
    {
        if (!mAcquiredResources)
        {
            bool bFoundConfig = true;

            #if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_STADIA)
            // STT config
            int32_t iSTTProfile = -1;
            const char8_t *pSTTUrl = nullptr;
            const char8_t *pSTTCred = nullptr;

            bFoundConfig &= mData.mBlazeHub->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_VOIP_STT_PROFILE, &iSTTProfile);
            bFoundConfig &= mData.mBlazeHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_VOIP_STT_URL, &pSTTUrl);
            bFoundConfig &= mData.mBlazeHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_VOIP_STT_CREDENTIAL, &pSTTCred);
            #endif

            #if defined(EA_PLATFORM_PS4)|| defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_STADIA)
            // TTS config
            int32_t iTTSProvider = -1;
            const char8_t *pTTSUrl = nullptr;
            const char8_t *pTTSCred = nullptr;

            bFoundConfig &= mData.mBlazeHub->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_VOIP_TTS_PROVIDER, &iTTSProvider);
            bFoundConfig &= mData.mBlazeHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_VOIP_TTS_URL, &pTTSUrl);
            bFoundConfig &= mData.mBlazeHub->getConnectionManager()->getServerConfigString(BLAZESDK_CONFIG_KEY_VOIP_TTS_CREDENTIAL, &pTTSCred);
            #endif

            DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_NETWORKADAPTER));
            if ((sActiveVoipCount == 0) && (VoipGetRef() == nullptr) && VoipStartup(mData.mConfig.mMaxNumVoipPeers, mData.mConfig.mVoipPlatformSpecificParam))
            {
                sOwnsVoip = true;
            }
            DirtyMemGroupLeave();

            // register first part id callback
            if (VoipGetRef() != nullptr)
            {
                VoipRegisterFirstPartyIdCallback(VoipGetRef(), ConnApiAdapter::getFirstPartyIdByPersonaId, this);
            }

            // configure stt & tts
            if (bFoundConfig)
            {
                if (VoipGetRef() != nullptr)
                {
                    #if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_STADIA)
                    VoipConfigTranscription(VoipGetRef(), (uint32_t)iSTTProfile, pSTTUrl, pSTTCred);
                    #endif

                    #if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_WINDOWS) || defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBSX) || defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_STADIA)
                    VoipConfigNarration(VoipGetRef(), (uint32_t)iTTSProvider, pTTSUrl, pTTSCred);
                    #endif
                }
            }
            else
            {
                BLAZE_SDK_DEBUGF("[CANA] missing one or more server config values needed to enable TTS/STT.\n");
            }

            if (sOwnsVoip)
            {
                // increment the number of ConnApiAdapters referencing the VOIP module
                sActiveVoipCount++;
            }

            mAcquiredResources = true;
        }
    }
}

void ConnApiAdapter::releaseResources()
{
    BLAZE_SDK_DEBUGF("[CANA] releaseResources()\n"); 

    if (mData.mConfig.mEnableVoip)
    {
        if (mAcquiredResources)
        {
            if (sOwnsVoip)
            {
                // decrement the number of ConnApiAdapters referencing the VOIP module
                sActiveVoipCount--;

                if (sActiveVoipCount == 0)
                {
                    if (VoipGetRef() != nullptr)
                    {
                        BLAZE_SDK_DEBUGF("[CANA] calling VoipShutdown()\n");
                        VoipShutdown(VoipGetRef(), 0);
                        BLAZE_SDK_DEBUGF("[CANA] VoipShutdown completed.\n");
                    }
                    else
                    {
                        BLAZE_SDK_DEBUGF("[CANA] VoipShutdown skipped.\n");
                    }

                    sOwnsVoip = false;
                }
            }

            mAcquiredResources = false;
        }
    }

    Blaze::Debug::clearLogBuffer();
}

void ConnApiAdapter::activateDelayedVoipOnEndpoint(const MeshEndpoint* endpoint)
{
    if (Network* network = findNetwork(endpoint))
    {
        network->activateDelayedVoipOnEndpoint(endpoint);
    }
}

bool ConnApiAdapter::getPinStatisticsForEndpoint(const Blaze::MeshEndpoint *pEndpoint, MeshEndpointPinStat &PinStatData, uint32_t uPeriod)
{
    Network *pNetwork;
    NetGameLinkRefT* pNetGameLink;
    NetGameDistRefT* pNetGameDist;
    NetGameLinkStatT NetgamelinkStats;
    ClientConnStats *pConnStats;

    BlazeAssert(pEndpoint != nullptr);
    if (!pEndpoint)
    {
        return(false);
    }

    if (pEndpoint->isLocal())
    {
        // can't get pin stats for an local endpoint
        return(false);
    }
   
    pNetwork = findNetwork(pEndpoint);
    pNetGameLink = getNetGameLinkForEndpoint(pEndpoint);
    pNetGameDist = getNetGameDistForEndpoint(pEndpoint);

    if ((pNetGameLink == nullptr) || (pNetwork == nullptr))
    {
        // can't get the link or network
        return(false);
    }

    pConnStats = pNetwork->getConnApiStatsForEndpoint(pEndpoint);

    if (NetGameLinkStatus(pNetGameLink, 'stat', 0, &NetgamelinkStats, sizeof(NetgamelinkStats)) == 0)
    {
        pConnStats->updateLinkStats(&NetgamelinkStats);

        PinStatData.RpackLost = pConnStats->getRpackLost();
        PinStatData.LpackLost = pConnStats->getLpackLost();
        PinStatData.Lnaksent = pConnStats->getLnaksent();
        PinStatData.Rnaksent = pConnStats->getRnaksent();
        PinStatData.Lpacksaved = pConnStats->getLpackSaved();
        PinStatData.LatencyMs = NetgamelinkStats.late;
        PinStatData.JitterMs = NetgamelinkStats.jitter;
        PinStatData.Ipps = NetgamelinkStats.inpps;
        PinStatData.Opps = NetgamelinkStats.outpps;
    }

    if (pNetGameDist == nullptr)
    {
        PinStatData.bDistStatValid = false;
    }
    else
    {
        pConnStats->updateDistPpsStats(NetGameDistStatus(pNetGameDist, 'icnt', 0, NULL, 0), NetGameDistStatus(pNetGameDist, 'ocnt', 0, NULL, 0), uPeriod);
        pConnStats->updateDistTimeStats(NetGameDistStatus(pNetGameDist, 'wait', 0, NULL, 0), NetGameDistStatus(pNetGameDist, 'dcnt', 0, NULL, 0), NetGameDistStatus(pNetGameDist, 'prti', 0, NULL, 0), NetGameDistStatus(pNetGameDist, 'pcnt', 0, NULL, 0));

        PinStatData.Idpps = pConnStats->getIdpps();
        PinStatData.Odpps = pConnStats->getOdmpps();
        PinStatData.DistWaitTime = pConnStats->getDistWaitTime();
        PinStatData.DistProcTime = pConnStats->getDistProcTime();
        PinStatData.bDistStatValid = pConnStats->isDistStatValid();
    }

    return(true);
}

bool ConnApiAdapter::getQosStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, MeshEndpointQos &qosData, bool bInitialQosStats)
{
    BlazeAssert(endpoint != nullptr);
    if (!endpoint)
    {
        return(false);
    }

    if (endpoint->isLocal() && endpoint->getMesh()->getNetworkTopology() != CLIENT_SERVER_DEDICATED)
    {
        // can't get QoS stats for a local endpoint
        return(false);
    }

    NetGameLinkRefT* netGameLink = getNetGameLinkForEndpoint(endpoint);

    if (netGameLink == nullptr)
    {
        //couldn't get net game link
        return(false);
    }

    //call the stat selector with the provided netgamelink, put the info into the returned values
    NetGameLinkStatT netgamelinkStats;
    if (NetGameLinkStatus(netGameLink, 'stat', 0, &netgamelinkStats, sizeof(netgamelinkStats)) == 0)
    {
        /* When bInitialQosStats is TRUE, qosData.LatencyMs is filled with NetGameLinkStatus('qlat') instead of
           netgamelink stats.late because the former one is an average obtained during the entire NetGameLink initial
           qos phase, while the latter one is the latency average calculated over a smaller sampling
           period (RTT_SAMPLE_PERIOD = ~ 1 sec).  */
        if (bInitialQosStats)
        {
            qosData.LatencyMs = NetGameLinkStatus(netGameLink, 'qlat', 0, nullptr, 0);
        }
        else
        {
            qosData.LatencyMs = netgamelinkStats.late;
        }
        // number of packets sent from peer - lost packets
        qosData.LocalPacketsReceived = netgamelinkStats.rpacksent - netgamelinkStats.lpacklost;
        // number of packets sent from peer but were lost
        qosData.LocalPacketsLost = netgamelinkStats.lpacklost;
        // number of packets sent from peer
        qosData.RemotePacketsSent = netgamelinkStats.rpacksent;
        return(true);
    
    }
    return(false);
}

bool ConnApiAdapter::getGameLinkStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, NetGameLinkStatT &netgamelinkStat)
{
    NetGameLinkRefT *netGameLink = getNetGameLinkForEndpoint(endpoint);

    if (netGameLink == nullptr)
    {
        //couldn't get net game link
        return(false);
    }

    if (NetGameLinkStatus(netGameLink, 'stat', 0, &netgamelinkStat, sizeof(netgamelinkStat)) == 0)
    {
        // we got the netgamelink stat successfully
        return(true);
    }

    return(false);
}

bool ConnApiAdapter::getConnectionTimersForEndpoint(const Blaze::MeshEndpoint *endpoint, MeshEndpointConnTimer &gameConnectTimers, MeshEndpointConnTimer &voipConnectTimers)
{
    const ConnApiClientT * connApiClient = getClientHandleForEndpoint(endpoint);

    if (connApiClient != nullptr)
    {
        gameConnectTimers.ConnectTime = connApiClient->GameInfo.ConnTimers.uConnectTime;
        gameConnectTimers.CreateSATime = connApiClient->GameInfo.ConnTimers.uCreateSATime;
        gameConnectTimers.DemangleConnectTime = connApiClient->GameInfo.ConnTimers.uDemangleConnectTime;
        gameConnectTimers.DemangleTime = connApiClient->GameInfo.ConnTimers.uDemangleTime;

        voipConnectTimers.ConnectTime = connApiClient->VoipInfo.ConnTimers.uConnectTime;
        voipConnectTimers.CreateSATime = connApiClient->VoipInfo.ConnTimers.uCreateSATime;
        voipConnectTimers.DemangleConnectTime = connApiClient->VoipInfo.ConnTimers.uDemangleConnectTime;
        voipConnectTimers.DemangleTime = connApiClient->VoipInfo.ConnTimers.uDemangleTime;
    }

    return(false);
}

bool ConnApiAdapter::getPrototunnelStatisticsForEndpoint(const Blaze::MeshEndpoint *endpoint, ProtoTunnelStatT &tunnelStatSend, ProtoTunnelStatT &tunnelStatsRecv)
{
    const Blaze::Mesh *mesh = endpoint->getMesh();
    const ConnApiClientT * connApiClient = getClientHandleForEndpoint(endpoint);

    if ((ConnApiStatus2(getConnApiRefT(mesh),'tunr', (void *)connApiClient, &tunnelStatsRecv, sizeof(tunnelStatsRecv)) == 0) &&
        (ConnApiStatus2(getConnApiRefT(mesh),'tuns', (void *)connApiClient, &tunnelStatSend, sizeof(tunnelStatsRecv)) == 0))
    {
        // we got the prototunnel stat successfully
        return(true);
    }

    return(false);
}

NetGameDistRefT *ConnApiAdapter::getNetGameDistForEndpoint(const Blaze::MeshEndpoint *endpoint) const
{
    if (getClientHandleForEndpoint(endpoint) != nullptr)
    {
        return(getClientHandleForEndpoint(endpoint)->pGameDistRef);
    }

    return(nullptr);
}

NetGameLinkRefT *ConnApiAdapter::getNetGameLinkForEndpoint(const Blaze::MeshEndpoint *endpoint) const
{
    if (getClientHandleForEndpoint(endpoint) != nullptr)
    {
        return(getClientHandleForEndpoint(endpoint)->pGameLinkRef);
    }

    return(nullptr);
}

void ConnApiAdapter::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    idleNetworks(currentTime, elapsedTime);
}

void ConnApiAdapter::idleNetworks(const uint32_t currentTime, const uint32_t elapsedTime)
{
    NetworkMap netMapCopy = mNetworkMap;

    // give each network a chance to be serviced
    for (NetworkMap::iterator it=netMapCopy.begin(); it!=netMapCopy.end(); it++)
    {
        const Mesh *mesh = it->first;

        Network* network = findNetwork(mesh);
        if (network)
        {
            network->idleNetwork(currentTime, elapsedTime);
        }
        else
        {
            // This means that the NetworkMap/iterator was invalidated during iteration
            BLAZE_SDK_DEBUGF("[CANA] idleNetworks() failed to findNetwork(). Was it recently removed properly?\n"); 
        }
    }
}

void ConnApiAdapter::flushTunnel() const
{
    if (mData.mProtoTunnel)
    {
        ProtoTunnelUpdate(mData.mProtoTunnel);
    }
}


}
}

