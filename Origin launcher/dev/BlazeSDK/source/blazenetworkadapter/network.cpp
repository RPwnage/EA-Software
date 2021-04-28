/*************************************************************************************************/
/*!
\file network.cpp


\attention
(c) Electronic Arts. All Rights Reserved
*************************************************************************************************/

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/internal/blazenetworkadapter/network.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/util/utilapi.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/gamemanager/game.h"

#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtysessionmanager.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/voip/voipgroup.h"

#include "EAStdC/EAString.h"

namespace Blaze
{
namespace BlazeNetworkAdapter
{
    static const size_t DEFERRED_EVENTS_COUNT_THRESHOLD = 10; // a reasonable magic number

void Network::staticConnApiCallBack(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData)
{
    static_cast<Network*>(pUserData)->onConnApiEvent(pCbInfo);
}

Network::Network()
:   mConnApi(nullptr),
    mMesh(nullptr),
    mData(nullptr),
    mClientListSize(0),
    mServerTunnelPort(0),
    mDeferredEventList(MEM_GROUP_NETWORKADAPTER, "DeferredEventList"),
    mGameTopology(CONNAPI_GAMETOPOLOGY_DISABLED),
    mVoipTopology(CONNAPI_VOIPTOPOLOGY_DISABLED)
{
    memset(mConnapiClientList, 0, sizeof(mConnapiClientList));
    memset(mParticipatingVoipLocalUsers, 0, sizeof(mParticipatingVoipLocalUsers));
}

Network::~Network()
{
}

bool Network::isConnApiNeeded() const
{
    //we always need the connapi for all topologies except game groups. And we need it for game group if the voip is enabled. 
     return ((mMesh->getNetworkTopology() != NETWORK_DISABLED) || ((mMesh->getVoipTopology() != VOIP_DISABLED) && (mData->mConfig.mEnableVoip == true) && (VoipGetRef() != nullptr)));
}

Network* Network::createNetwork()
{
    Network* pNetwork = BLAZE_NEW(MEM_GROUP_NETWORKADAPTER, "Network") Network();
    return(pNetwork);
}

void Network::destroyNetwork()
{
    handleDeferredEvents();

    if (mConnApi)
    {
        //cleanup by disconnecting
        ConnApiDisconnect(mConnApi);

        //cleanup by stopping our connapi callbacks
        ConnApiControl(mConnApi, 'cbfp', 0, 0, nullptr);
    }

    // remove tunnel port, if set
    if (mServerTunnelPort != 0)
    {
        ProtoTunnelControl(mData->mProtoTunnel, 'bndr', mServerTunnelPort, 0, nullptr);
    }

    mData->mUserListenerDispatcher->dispatch("onUninitialize", &NetworkMeshAdapterUserListener::onUninitialize, mMesh, NetworkMeshAdapter::ERR_OK);

    //todo, handle async destroy behaviour
    //cleanup by destroying connapi
    if (mConnApi)
    {
        ConnApiDestroy(mConnApi);
    }
    BLAZE_DELETE(MEM_GROUP_NETWORKADAPTER, this);
}

/*
The mesh passed to initNetworkMesh() has the following characteristics:
* Each mesh endpoint corresponds to a console or dedicated server in the game.
* The list of members in the mesh can contain multiple players (members) existing on the same remote console (endpoint).


The implementation of initNetworkMesh() creates a single connapi client per mesh endpoint.
*/
bool Network::initNetworkMesh(const Mesh *pMesh, ConnApiAdapterData* pData, const BlazeNetworkAdapter::NetworkMeshAdapter::Config &config)
{
    bool retVal = false;
    mMesh = pMesh;
    mData = pData;
    mNetworkMeshConfig = config;

    if (isConnApiNeeded())
    {
        if (!setupConnApi())
        {
            retVal = false;
        }
        else if (!setupTunnel())
        {
            ConnApiDestroy(mConnApi);
            mConnApi = nullptr;
            retVal = false;
        }
        else
        {
            retVal = true;
        }

        if (retVal)
        {
            setupClients();
            setupDemangler();
            setupMesh();
            setupVoip();
            activateNetwork();
        }
    }
    else
    {
        retVal = true;
    }

    return(retVal);
}

ConnApiRefT* Network::getConnApiRefT() const
{
    return(mConnApi);
}

const ConnApiClientT* Network::getClientHandleForEndpoint(const MeshEndpoint* meshEndpoint) const
{
    const ConnApiClientT *pClient = nullptr;

    if (mConnApi == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] getClientHandleForEndpoint(): cannot retrieve client handle for mesh endpoint 0x%016" PRIx64 " as local ConnApi is not available.", this, ((meshEndpoint != nullptr)? meshEndpoint->getConnectionGroupId() : 0));
        return(nullptr);
    }

    const ConnApiClientListT *clist = ConnApiGetClientList(mConnApi);
    for (int32_t iClientIndex = 0; iClientIndex < clist->iMaxClients; iClientIndex++)
    {
        if (static_cast<uint32_t>(meshEndpoint->getConnectionGroupId()) == clist->Clients[iClientIndex].ClientInfo.uId)
        {
            pClient = &clist->Clients[iClientIndex];
            break;
        }
    }
    return(pClient);
}

ClientConnStats* Network::getConnApiStatsForEndpoint (const MeshEndpoint *meshEndpoint)
{
    return(&mConnApiConnClientStatsList[meshEndpoint->getConnectionSlotId()]);
}

void Network::onConnApiEvent(const ConnApiCbInfoT* pCbInfo)
{
    // Defer this event until the normal idle event:
    mDeferredEventList.push_back(*pCbInfo);

    if (mDeferredEventList.size() > DEFERRED_EVENTS_COUNT_THRESHOLD)
    {
        BLAZE_SDK_DEBUGF("[CANA] Error: mDeferredEventList size (%u) is larger than expected. Make sure that ConnApiAdapter::idle is being ticked. \n", (unsigned int) (mDeferredEventList.size()));
    }
}

void Network::handleDeferredEvents()
{
    if (!mDeferredEventList.empty())
    {
        for (DeferredEventList::const_iterator iter = mDeferredEventList.begin(), end = mDeferredEventList.end(); iter != end; ++iter)
        {
            handleConnApiEvent(&(*iter));
        }
        mDeferredEventList.clear();
    }
}
void Network::handleConnApiEvent(const ConnApiCbInfoT* pCbInfo)
{
    switch (pCbInfo->eType)
    {
    case CONNAPI_CBTYPE_GAMEEVENT:
        onConnApiGameEvent(pCbInfo);
        break;
    case CONNAPI_CBTYPE_DESTEVENT:
        break;
    case CONNAPI_CBTYPE_VOIPEVENT:
        onConnApiVoipEvent(pCbInfo);
        break;
    default:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Error: Unexpected ConnApi event type(%u) state(%u=>%u) client(%u)...\n", this, pCbInfo->eType, pCbInfo->eOldStatus, pCbInfo->eNewStatus, pCbInfo->iClientIndex);
    }
}

void Network::onConnApiVoipEvent(const ConnApiCbInfoT* pCbInfo)
{
    int32_t iClientIndex = pCbInfo->iClientIndex;
    uint64_t uConnectionGroupId = mConnectionGroupByIndex[iClientIndex];
    const ConnApiClientT * connApiClient = pCbInfo->pClient;
    Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::ConnectionFlagsBitset connectionFlags(0);
    uint32_t uId = connApiClient->ClientInfo.uId; (void)uId;

    // set voip-connection specific flags
    if (connApiClient)
    {
        if (connApiClient->VoipInfo.uConnFlags & CONNAPI_CONNFLAG_DEMANGLED)
        {
            connectionFlags.set(Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::CONNECTION_FLAG_DEMANGLED);
        }
        if (connApiClient->VoipInfo.uConnFlags & CONNAPI_CONNFLAG_PKTRECEIVED)
        {
            connectionFlags.set(Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::CONNECTION_FLAG_PKTRECEIVED);
        }

    }

    switch (pCbInfo->eNewStatus)
    {
    case CONNAPI_STATUS_INIT:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Voip channel[%d] for connapi client (0x%08x) INIT\n", this, iClientIndex, uId);
        break;
    case CONNAPI_STATUS_CONN:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Voip channel[%d] for connapi client (0x%08x) CONNECTING\n", this, iClientIndex, uId);
        break;
    case CONNAPI_STATUS_MNGL:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Voip channel[%d] for connapi client (0x%08x) MNGL\n", this, iClientIndex, uId);
        break;
    case CONNAPI_STATUS_ACTV:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Voip channel[%d] for connapi client (0x%08x) CONNECTED\n", this, iClientIndex, uId);
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Notifying listeners that we have connected to an endpoint\n", this);
        mData->mListenerDispatcher->dispatch("connectedToVoipEndpoint", &NetworkMeshAdapterListener::connectedToVoipEndpoint, mMesh, uConnectionGroupId, connectionFlags, NetworkMeshAdapter::ERR_OK);
        break;
    case CONNAPI_STATUS_DISC:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Voip channel[%d] for connapi client (0x%08x) DISCONNECTED\n", this, iClientIndex, uId);
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Notifying listeners that an endpoint has been disconnected.\n", this);
        mData->mListenerDispatcher->dispatch("connectionToVoipEndpointLost", &NetworkMeshAdapterListener::connectionToVoipEndpointLost, mMesh, uConnectionGroupId, connectionFlags, NetworkMeshAdapter::ERR_OK);
        break;
    default:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] VOIPEVENT(%u) state(%u=>%u) client(%u)...\n", this, pCbInfo->eType, pCbInfo->eOldStatus, pCbInfo->eNewStatus, iClientIndex);
    }
}

void Network::onConnApiGameEvent(const ConnApiCbInfoT* pCbInfo)
{
    int32_t iClientIndex = pCbInfo->iClientIndex;
    uint64_t uConnectionGroupId = mConnectionGroupByIndex[iClientIndex];
    const ConnApiClientT * connApiClient = pCbInfo->pClient;
    Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::ConnectionFlagsBitset connectionFlags(0);
    uint32_t uId = connApiClient->ClientInfo.uId; (void)uId;

    switch (pCbInfo->eNewStatus)
    {
    case CONNAPI_STATUS_INIT:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Game channel[%d] for connapi client (0x%08x) INIT\n", this, iClientIndex, uId);
        break;
    case CONNAPI_STATUS_CONN:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Game channel[%d] for connapi client (0x%08x) CONNECTING\n", this, iClientIndex, uId);
        break;
    case CONNAPI_STATUS_MNGL:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Game channel[%d] for connapi client (0x%08x) MNGL\n", this, iClientIndex, uId);
        break;
    case CONNAPI_STATUS_ACTV:
    case CONNAPI_STATUS_DISC:
        // set game-connection specific flags
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

        if (pCbInfo->eNewStatus == CONNAPI_STATUS_ACTV)
        {
            BLAZE_SDK_DEBUGF("[CANA:Network:%p] Game channel[%d] for connapi client (0x%08x) CONNECTED\n", this, iClientIndex, uId);
            BLAZE_SDK_DEBUGF("[CANA:Network:%p] Notifying listeners that we have connected to an endpoint\n", this);
            mData->mListenerDispatcher->dispatch("connectedToEndpoint", &NetworkMeshAdapterListener::connectedToEndpoint, mMesh, uConnectionGroupId, connectionFlags, NetworkMeshAdapter::ERR_OK);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[CANA:Network:%p] Game channel[%d] for connapi client (0x%08x) LOST CONNECTION\n", this, iClientIndex, uId);
            BLAZE_SDK_DEBUGF("[CANA:Network:%p] Notifying listeners that connection to an endpoint has been lost.\n", this);
            mData->mListenerDispatcher->dispatch("connectionToEndpointLost", &NetworkMeshAdapterListener::connectionToEndpointLost, mMesh, uConnectionGroupId, connectionFlags, NetworkMeshAdapter::ERR_OK);
        }
        break;
    default:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] GAMEEVENT(%u) state(%u=>%u) client(%u)...\n", this, pCbInfo->eType, pCbInfo->eOldStatus, pCbInfo->eNewStatus, iClientIndex);
    }
}

bool Network::initConnApiClient(ConnApiClientInfoT *pClient, const MeshEndpoint* pMeshEndpoint)
{
    DirtyAddrT dirtyAddr;
    memset(&dirtyAddr, 0, sizeof(dirtyAddr));

    switch (pMeshEndpoint->getNetworkAddress()->getActiveMember())
    {
    case NetworkAddress::MEMBER_UNSET:
        break;

    case NetworkAddress::MEMBER_IPPAIRADDRESS:
        pClient->uLocalAddr = pMeshEndpoint->getNetworkAddress()->getIpPairAddress()->getInternalAddress().getIp();
        pClient->uAddr = pMeshEndpoint->getNetworkAddress()->getIpPairAddress()->getExternalAddress().getIp();

        DirtyAddrFromHostAddr(&dirtyAddr, (const void*)&pClient->uAddr);
        blaze_strnzcpy(pClient->DirtyAddr.strMachineAddr, (const char*)&dirtyAddr, sizeof(pClient->DirtyAddr.strMachineAddr));

        pClient->uLocalTunnelPort = pMeshEndpoint->getNetworkAddress()->getIpPairAddress()->getInternalAddress().getPort();
        pClient->uTunnelPort = pMeshEndpoint->getNetworkAddress()->getIpPairAddress()->getExternalAddress().getPort();

        if (pClient->uTunnelPort == 0)
        {
            pClient->uTunnelPort = pClient->uLocalTunnelPort;
        }
        break;

    case NetworkAddress::MEMBER_IPADDRESS:
        pClient->uLocalAddr = pMeshEndpoint->getNetworkAddress()->getIpAddress()->getIp();
        pClient->uAddr = pMeshEndpoint->getNetworkAddress()->getIpAddress()->getIp();

        DirtyAddrFromHostAddr(&dirtyAddr, (const void*)&pClient->uAddr);
        blaze_strnzcpy(pClient->DirtyAddr.strMachineAddr, (const char*)&dirtyAddr, sizeof(pClient->DirtyAddr.strMachineAddr));

        pClient->uLocalTunnelPort = pMeshEndpoint->getNetworkAddress()->getIpAddress()->getPort();
        pClient->uTunnelPort = pMeshEndpoint->getNetworkAddress()->getIpAddress()->getPort();

        if (pClient->uTunnelPort == 0)
        {
            pClient->uTunnelPort = pClient->uLocalTunnelPort;
        }
        break;

    case NetworkAddress::MEMBER_XBOXCLIENTADDRESS:
        {
            BlazeAssertMsg(pMeshEndpoint->getNetworkAddress()->getXboxClientAddress()->getXnAddr().getData() != nullptr, "[CANA:Network] FATAL: MEMBER_XBOXCLIENTADDRESS address data null");

            #if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
            uint64_t aXuid = pMeshEndpoint->getNetworkAddress()->getXboxClientAddress()->getXuid();
            DirtyAddrSetInfoXboxOne(&pClient->DirtyAddr, &aXuid, pMeshEndpoint->getNetworkAddress()->getXboxClientAddress()->getXnAddr().getData(), pMeshEndpoint->getNetworkAddress()->getXboxClientAddress()->getXnAddr().getSize());
            #endif

            // in general, the addresses are not really needed when establishing connections to/from xbox one, except for when we determine if we should use the external or local address. here we set our external address to
            // what blaze knows so that when making the determination when compared with the dedicated server's address we make the correct choice. without this when on the same local network as the dedicated server, it will
            // attempt to use the external address instead.
            pClient->uAddr = pMeshEndpoint->isLocal() ? mData->mBlazeHub->getConnectionManager()->getExternalAddress()->getIp() : 0;
            pClient->uLocalAddr = 0;
        }
        break;

    default:
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] initConnApiClient() pMeshEndpoint %p has an unsupported network address type\n", this, pMeshEndpoint);
        return(false);
    }

    if (!pMeshEndpoint->isLocal())
    {
        if (pMeshEndpoint->isConnectivityHosted() && (mMesh->getNetworkTopology() != PEER_TO_PEER_FULL_MESH) && (mMesh->getNetworkTopology() != CLIENT_SERVER_PEER_HOSTED) && (mMesh->getNetworkTopology() != NETWORK_DISABLED))
        {
            BLAZE_SDK_DEBUGF("[CANA] FATAL: cannot use connectivity hosting for network topologies other than PEER_TO_PEER_FULL_MESH, or CLIENT_SERVER_PEER_HOSTED or NETWORK_DISABLED(Game Groups Voip)\n");
            return(false);
        }
        if (pMeshEndpoint->getTunnelKey() == NULL)
        {
            BLAZE_SDK_DEBUGF("[CANA] FATAL: endpoint returned a NULL tunnel key. that indicates that the game was improperly configured. failing here to prevent crashing\n");
            return(false);
        }
    }

    mConnectionGroupByIndex[pMeshEndpoint->getConnectionSlotId()] = pMeshEndpoint->getConnectionGroupId();
    pClient->uId = static_cast<uint32_t>(pMeshEndpoint->getConnectionGroupId());
    pClient->bEnableQos = pMeshEndpoint->getPerformQosDuringConnection();
    pClient->bIsVoipContributer = !pMeshEndpoint->isVoipDisabled();
    if (!pMeshEndpoint->isLocal())
    {
        ds_strnzcpy(pClient->strTunnelKey, pMeshEndpoint->getTunnelKey(), sizeof(pClient->strTunnelKey));
        pClient->bIsConnectivityHosted = pMeshEndpoint->isConnectivityHosted();
        pClient->uLocalClientId = pMeshEndpoint->getLocalLowLevelConnectivityId();
        pClient->uRemoteClientId = pMeshEndpoint->getRemoteLowLevelConnectivityId();
        pClient->uHostingServerId = pClient->bIsConnectivityHosted ? pMeshEndpoint->getHostingServerConnectivityId() : 0;
        if (pClient->bIsConnectivityHosted)
        {
            /* The string returned by pMeshEndpoint->getHostingServerConnectionSetId() has this format:
                    "ec2-54-189-45-117.us-west-2.compute.amazonaws.com:21823:10027"
                    <fully qualified public host name>:<UDP port>:<32-bit connection set id unique to the CC hosting server> 
               We want to initialize pClient->uHostingServerConnSetId with the number after the second colon */
            pClient->uHostingServerConnSetId = atoi(strchr(strchr(pMeshEndpoint->getHostingServerConnectionSetId(), ':') + 1, ':') + 1);
        }
        else
        {
            pClient->uHostingServerConnSetId = 0;
        }
    }

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    if(!pMeshEndpoint->isLocal() && pClient->bIsConnectivityHosted)
    {
        // if port is not already associated with server socket, add it
        if (!ProtoTunnelStatus(mData->mProtoTunnel, 'bnds', pClient->uTunnelPort, nullptr, 0))
        {
            ProtoTunnelControl(mData->mProtoTunnel, 'bnds', pClient->uTunnelPort, 0, nullptr);

            if ((mServerTunnelPort != 0) && (mServerTunnelPort != pClient->uTunnelPort))
            {
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] CRITICAL ERR - Scenario involving clients hosted by different servers not yet supported.\n", this);
            }
            mServerTunnelPort = pClient->uTunnelPort;
        }
    }
#endif

#if ENABLE_BLAZE_SDK_LOGGING
    printConnApiClient(pClient, pMeshEndpoint);
#endif

    return(true);
}

#if ENABLE_BLAZE_SDK_LOGGING
void Network::printConnApiClient(ConnApiClientInfoT *pClient, const MeshEndpoint* pMeshEndpoint) const
{
    // dump info about client
    char strAddr[32];
    char strLocalAddr[32];
    char strConnSetId[32];
    SocketInAddrGetText(static_cast<uint32_t>(pClient->uAddr), strAddr, sizeof(strAddr));
    SocketInAddrGetText(static_cast<uint32_t>(pClient->uLocalAddr), strLocalAddr, sizeof(strLocalAddr));
    ds_snzprintf(strConnSetId, sizeof(strConnSetId), "%d", pClient->uHostingServerConnSetId);

    BLAZE_SDK_DEBUGF("[CANA:Network:%p] ConnApiClientInfoT:\n  DirtyAddr.strMachineAddr = %s\n  uAddr = %s\n  uId = 0x%08x\n  uLocalClientId = 0x%08x\n  uRemoteClientId = 0x%08x\n  uLocalAddr = %s\n  uLocalGamePort = %d\n  uLocalTunnelPort = %d\n  LocalVoipPort = %d\n  uTunnelPort = %d\n  bEnableQoS = %s\n  bIsConnectivityHosted = %s\n  uHostingServerConnSetId = %s\n  local client = %s\n",
        this,
        pClient->DirtyAddr.strMachineAddr,
        strAddr,
        pClient->uId,
        pClient->uLocalClientId,
        pClient->uRemoteClientId,
        strLocalAddr,
        pClient->uLocalGamePort,
        pClient->uLocalTunnelPort,
        pClient->uLocalVoipPort,
        pClient->uTunnelPort,
        pClient->bEnableQos ? "TRUE" : "FALSE",
        pClient->bIsConnectivityHosted ? "TRUE" : "FALSE",
        pClient->bIsConnectivityHosted ? strConnSetId : "n/a",
        pMeshEndpoint->isLocal() ? "TRUE" : "FALSE");
}

void Network::printMeshMembersForMeshEndpoint(const MeshEndpoint* pMeshEndpoint)
{
    eastl::string meshMembers;
    const Mesh* pMesh = pMeshEndpoint->getMesh();
    for (uint16_t uIndex = 0; uIndex < pMesh->getMeshMemberCount(); uIndex++)
    {
        const MeshMember* pMeshMember = pMesh->getMeshMemberByIndex(uIndex);

        if (pMeshMember->getMeshEndpoint() == pMeshEndpoint)
        {
            meshMembers.append_sprintf("%s  ", pMeshMember->getName());
        }
    }

    BLAZE_SDK_DEBUGF("[CANA:Network:%p] %s endpoint with conn group id 0x%016" PRIx64 " now has member(s): %s\n", this, (pMeshEndpoint->isLocal()?"local":"remote"), pMeshEndpoint->getConnectionGroupId(), meshMembers.c_str());
}
#endif

void Network::resetGame()
{
    if (mMesh != nullptr && mMesh->isTopologyHost() && (mMesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED))
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] reacting to resetGame.", this);
        setupMesh();
    }
}

void Network::migrateTopologyHost()
{
    if (mConnApi == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] migrateTopologyHost(): migrateTopologyHost is skipped because local ConnApi is not available.\n", this);
        return;
    }

    if (mMesh == nullptr)
    {
        BlazeAssert(mMesh != nullptr);
        return;
    }

    BlazeAssert(mMesh->isMigrating());

    BLAZE_SDK_DEBUGF("[CANA:Network:%p] migrateTopologyHost: new topology host at connection slot id %d\n", this, mMesh->getTopologyHostConnectionSlotId());

    if (mMesh->getNetworkTopology() == PEER_TO_PEER_FULL_MESH  ||
        mMesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED ||
        mMesh->getNetworkTopology() == NETWORK_DISABLED)
    {
        // no need to migrate host for those topologies
        return;
    }

    // topology host migration only feasible in a peer-hosted context.
    BlazeAssert((mMesh->getNetworkTopology() == CLIENT_SERVER_PEER_HOSTED));

    ConnApiMigrateGameHost(mConnApi, mMesh->getTopologyHostMeshEndpoint()->getConnectionSlotId());
}

void Network::activateDelayedVoipOnEndpoint(const Blaze::MeshEndpoint* endpoint)
{
    if (mConnApi == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::activateDelayedVoipOnEndpoint (0x%016" PRIX64 ") for mesh (%" PRId64 "/0x%016" PRIX64 ":%s) ignored because conn api ref not available\n",
            this, endpoint->getConnectionGroupId(), endpoint->getMesh()->getId(), endpoint->getMesh()->getId(), endpoint->getMesh()->getName());
        return;
    }

    ConnApiControl(mConnApi, 'estv', endpoint->getConnectionSlotId(), 0, nullptr);
}

/*
The mesh endpoint passed to connectToEnpdoint() represents a console in the game.
All players (mesh members) on the same console belong to the same connection group (console).

Blaze is not supposed to invoke this function with a mesh endpoint belonging to a
connection group that already has a connapi client allocated for.
*/
void Network::connectToEndpoint(const Blaze::MeshEndpoint *meshEndpoint)
{
    if (mConnApi == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::connectToEndpoint(0x%016" PRIX64 ") for mesh (%" PRId64 "/0x%016" PRIX64 ":%s) ignored because conn api ref not available\n",
            this, meshEndpoint->getConnectionGroupId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getName());
        return;
    }

    // there is no need to add a new connapi client if there already exists one for the connection group
    // to which the player belongs
    if (getClientHandleForEndpoint(meshEndpoint))
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::connectToEndpoint(0x%016" PRIX64 ") for mesh (%" PRId64 "/0x%016" PRIX64 ":%s) ignored because connection group %" PRId64 " already has a connapi client\n",
            this, meshEndpoint->getConnectionGroupId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getName(), meshEndpoint->getConnectionGroupId());
        return;
    }

    addConnApiClient(meshEndpoint);
    
    #if ENABLE_BLAZE_SDK_LOGGING
    printMeshMembersForMeshEndpoint(meshEndpoint);
    #endif
}

/*
The mesh member passed to addMemberOnEndpoint() represents a player (not a console) in the game.
All players on the same console belong to the same connection group (console).

In theory, Blaze could invoke this function for both remote users and local users.
In practice there is currently no need for this function to be invoked for remote users.
*/
void Network::addMemberOnEndpoint(const MeshMember *meshMember)
{
    if (mConnApi == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::addMemberOnEndpoint()  adding member 0x%016" PRIX64 ":%s to conn group 0x%016" PRIX64 " ignored because conn api ref not available\n",
            this, meshMember->getId(), meshMember->getName(), meshMember->getMeshEndpoint()->getConnectionGroupId());
        return;
    }

    if (meshMember->isLocal())
    {
        if (mMesh->getVoipTopology() != VOIP_DISABLED)
        {
            if (VoipGetRef() != nullptr)
            {
                // any local user can now be changed into a "participating" voip user 
                VoipGroupRefT *voipGroupRef;
                uint32_t uLocalUserIndex = meshMember->getLocalUserIndex();
                if (uLocalUserIndex < VOIP_MAXLOCALUSERS)
                {
                    ConnApiStatus(mConnApi, 'vgrp', &voipGroupRef, sizeof(voipGroupRef));
                    VoipGroupActivateLocalUser(voipGroupRef, uLocalUserIndex, TRUE);
                    mParticipatingVoipLocalUsers[uLocalUserIndex] = true;
                }
                else
                {
                    BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::addMemberOnEndpoint() local user index out of range (%u)\n",
                        this, uLocalUserIndex);
                }
            }
            else
            {
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::addMemberOnEndpoint() cannot activate voip for member 0x%016" PRIX64 ":%s on conn group 0x%016" PRIX64 " because VoipStartup() was not called or failed\n",
                    this, meshMember->getId(), meshMember->getName(), meshMember->getMeshEndpoint()->getConnectionGroupId()); 
            }
        }
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::addMemberOnEndpoint()  adding member 0x%016" PRIX64 ":%s to conn group 0x%016" PRIX64 " -- no-op \n",
            this, meshMember->getId(), meshMember->getName(), meshMember->getMeshEndpoint()->getConnectionGroupId());
    }

    #if ENABLE_BLAZE_SDK_LOGGING
    printMeshMembersForMeshEndpoint(meshMember->getMeshEndpoint());
    #endif
}

void Network::removeMemberOnEndpoint(const MeshMember *meshMember)
{
    if (mConnApi == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::removeMemberOnEndpoint()  removing member 0x%016" PRIX64 ":%s from conn group 0x%016" PRIX64 " ignored because conn api ref not available\n",
            this, meshMember->getId(), meshMember->getName(), meshMember->getMeshEndpoint()->getConnectionGroupId());
        return;
    }

    // any local user can now be changed into a "non-participating" voip user and leave the first party session
    if (meshMember->isLocal())
    {
        uint32_t uLocalUserIndex = meshMember->getLocalUserIndex();
        if (uLocalUserIndex < VOIP_MAXLOCALUSERS)
        {
            if (mParticipatingVoipLocalUsers[uLocalUserIndex])
            {
                VoipGroupRefT *voipGroupRef;
                ConnApiStatus(mConnApi, 'vgrp', &voipGroupRef, sizeof(voipGroupRef));
                VoipGroupActivateLocalUser(voipGroupRef, uLocalUserIndex, FALSE);
                mParticipatingVoipLocalUsers[uLocalUserIndex] = false;
            }
            else
            {
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] inconsistent state in Network::removeUserOnEndpoint()! local user was supposed to be participating at the voip level", this);
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::addMemberOnEndpoint() local user index out of range (%u)\n", this, uLocalUserIndex);
        }
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::removeMemberOnEndpoint()  removing member 0x%016" PRIX64 ":%s from conn group 0x%016" PRIX64 " -- no-op \n",
            this, meshMember->getId(), meshMember->getName(), meshMember->getMeshEndpoint()->getConnectionGroupId());
    }

    #if ENABLE_BLAZE_SDK_LOGGING
    printMeshMembersForMeshEndpoint(meshMember->getMeshEndpoint());
    #endif
}

/*
The mesh endpoint passed to disconnectFromEnpdoint() represents a console in the game.
All players on the same console belong to the same connection group (console).

Blaze is not supposed to invoke this function with a mesh endpoint belonging to a
connection group that does not already have a connapi client allocated for.
*/
void Network::disconnectFromEndpoint(const Blaze::MeshEndpoint *meshEndpoint)
{
    if (mConnApi == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::disconnectFromEndpoint(0x%016" PRIX64 ") for mesh (%" PRId64 "/0x%016" PRIX64 ":%s) ignored because conn api ref not available\n",
            this, meshEndpoint->getConnectionGroupId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getName());
        return;
    }

    BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::disconnectFromEndpoint(0x%016" PRIX64 ") for mesh (%" PRId64 "/0x%016" PRIX64 ":%s) -- ConnApiRemoveClient() for connection slot id %d\n",
        this, meshEndpoint->getConnectionGroupId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getName(), meshEndpoint->getConnectionSlotId());

    ConnApiRemoveClient(mConnApi, meshEndpoint->getConnectionSlotId());

    #if ENABLE_BLAZE_SDK_LOGGING
    printMeshMembersForMeshEndpoint(meshEndpoint);
    #endif
}

bool Network::setupConnApi()
{
    BlazeAssert(mConnApi == nullptr);
    int32_t iMaxClients;
    
    // if we are in multihub mode we set the maximum number of voipgroups to double than usual
    // we need to do this before we create connapi due to being allocated in create
    if (mData->mConfig.mMultiHubTestMode)
    {
        ConnApiControl(nullptr, 'maxg', 16, 0, nullptr);
    }

    // base the maximum client capacity off of the maximum player capacity
    iMaxClients = static_cast<const GameManager::Game *>(mMesh)->getMaxPlayerCapacity();
    // if the game is CSD, we need to save a slot for the server
    if (mMesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED)
    {
        iMaxClients += 1;
    }

    BLAZE_SDK_DEBUGF("[CANA:Network:%p] ConnApiCreate with port %d\n", this, mData->mConfig.mVirtualGamePort);
    DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_NETWORKADAPTER));
    mConnApi = ConnApiCreate(mData->mConfig.mVirtualGamePort, iMaxClients, staticConnApiCallBack, static_cast<void*>(this));
    DirtyMemGroupLeave();

    // use config overrides found in util.cfg, note that in the case of connapi a lot of the settings are also found in connapiadapterconfig,
    // you should consider overriding those
    Blaze::Util::UtilAPI::createAPI(*mData->mBlazeHub);
    if (mConnApi != nullptr)
    {
        mData->mBlazeHub->getUtilAPI()->OverrideConfigs(mConnApi);

        if (mData->mConfig.mTimeout != 0)
        {
            ConnApiControl(mConnApi, 'time', mData->mConfig.mTimeout, 0, nullptr);
        }
        if (mData->mConfig.mConnectionTimeout != 0)
        {
            ConnApiControl(mConnApi, 'ctim', mData->mConfig.mConnectionTimeout, 0, nullptr);
        }
        if (mData->mConfig.mUnackLimit != 0)
        {
            ConnApiControl(mConnApi, 'ulmt', mData->mConfig.mUnackLimit, 0, nullptr);
        }

        ConnApiControl(mConnApi, 'meta', mData->mConfig.mMultiHubTestMode, 0, nullptr);
        ConnApiControl(mConnApi, 'sqos', mNetworkMeshConfig.QosDurationInMs, mNetworkMeshConfig.QosIntervalInMs, nullptr);
        ConnApiControl(mConnApi, 'lqos', mNetworkMeshConfig.QosPacketSize, 0, nullptr);

        BLAZE_SDK_DEBUGF("[CANA:Network:%p] setupNetwork: mConnApi=%p\n", this, mConnApi);

        // tell the user that a create of ConnApi has been attempted (they should still check the error code)
        mData->mUserListenerDispatcher->dispatch("onInitialized", &NetworkMeshAdapterUserListener::onInitialized, mMesh, NetworkMeshAdapter::ERR_OK);

        // configure packet sizes
        ConnApiControl(mConnApi, 'mwid', mData->mConfig.mPacketSize, 0, nullptr);

        // set the CC mode
        switch (static_cast<const GameManager::Game*>(mMesh)->getCCSMode())
        {
            case GameManager::CCS_MODE_PEERONLY:
                ConnApiControl(mConnApi, 'ccmd', CONNAPI_CCMODE_PEERONLY, 0, nullptr);
                break;
            case GameManager::CCS_MODE_HOSTEDONLY:
                ConnApiControl(mConnApi, 'ccmd', CONNAPI_CCMODE_HOSTEDONLY, 0, nullptr);
                break;
            case GameManager::CCS_MODE_HOSTEDFALLBACK:
                ConnApiControl(mConnApi, 'ccmd', CONNAPI_CCMODE_HOSTEDFALLBACK, 0, nullptr);
                break;
            default:
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] unsupported CC mode - default to PEERONLY\n", this);
                ConnApiControl(mConnApi, 'ccmd', CONNAPI_CCMODE_PEERONLY, 0, nullptr);
                break;
        }
    }
    else
    {
        mData->mUserListenerDispatcher->dispatch("onInitialized", &NetworkMeshAdapterUserListener::onInitialized, mMesh, NetworkMeshAdapter::ERR_CANNOT_INIT_NETWORK);
    }

    return(mConnApi != nullptr);
}

void Network::setupClients()
{
    if (mMesh == nullptr)
    {
        BlazeAssert(mMesh != nullptr);
        return;
    }
    
    memset(mConnapiClientList, 0, sizeof(mConnapiClientList));
    mClientListSize = 0;

    for (uint16_t uIndex = 0; uIndex < mMesh->getMeshEndpointCount(); uIndex++)
    {
        if (mMesh->getDedicatedServerHostMeshEndpoint() != nullptr)
        {
            if (mMesh->getDedicatedServerHostMeshEndpoint()->getNetworkAddress() == nullptr)
            {
                BlazeAssertMsg(false, "[CANA] FATAL: the dedicated server's endpoint had a nullptr network address!");
            }
            else
            {
                BlazeAssertMsg((mMesh->getDedicatedServerHostMeshEndpoint()->getNetworkAddress()->getActiveMember() != NetworkAddress::MEMBER_XBOXCLIENTADDRESS),
                    "[CANA] FATAL: a non-dirtycast game server cannot have a network address of type 'MEMBER_XBOXCLIENTADDRESS'");
            }
        }
        const MeshEndpoint* pMeshEndpoint = mMesh->getMeshEndpointByIndex(uIndex);

        int32_t iConnapiIdx = pMeshEndpoint->getConnectionSlotId();

        #if ENABLE_BLAZE_SDK_LOGGING
        printMeshMembersForMeshEndpoint(pMeshEndpoint);
        #endif

        BLAZE_SDK_DEBUGF("[CANA:Network:%p] setting up %s client 0x%016" PRIX64 " -- connection slot: %d   connapi index: %d\n",
            this, pMeshEndpoint->isLocal()?"local":"remote", pMeshEndpoint->getConnectionGroupId(), pMeshEndpoint->getConnectionSlotId(), iConnapiIdx);

        if (initConnApiClient(&mConnapiClientList[iConnapiIdx], pMeshEndpoint))
        {
            if (iConnapiIdx >= mClientListSize)
            {
                mClientListSize = iConnapiIdx + 1;
            }
        }
        else
        {
            // invalidate this entry in the list 
            memset(&mConnapiClientList[iConnapiIdx], 0, sizeof(mConnapiClientList[0]));
        }
    }
}

bool Network::setupTunnel()
{
    if (mData == nullptr)
    {
        BlazeAssert(mData != nullptr);
        return(false);
    }
    int32_t iPort = 0;
    uint16_t uTunnelPort = 0;

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    iPort = mData->mBlazeHub->getInitParams().GamePort;
#else
    iPort = mData->mBlazeHub->getConnectionManager()->getNetworkAddress()->getIpPairAddress()->getInternalAddress().getPort();
#endif

    // create our ProtoTunnel ref
    if (mData->mProtoTunnel == nullptr)
    {
        DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_NETWORKADAPTER));
        mData->mProtoTunnel = ProtoTunnelCreate(mData->mConfig.mMaxNumTunnels, iPort);
        DirtyMemGroupLeave();

        if (mData->mProtoTunnel)
        {
            // use config overrides found in util.cfg, note that in the case of prototunnel some of the settings come
            // from connapiadapterconfig, you should consider overriding those
            Blaze::Util::UtilAPI::createAPI(*mData->mBlazeHub);
            mData->mBlazeHub->getUtilAPI()->OverrideConfigs(mData->mProtoTunnel);

            // set the send and receive buffer sizes
            if (mData->mConfig.mTunnelSocketRecvBuffSize > 0)
            {
                ProtoTunnelControl(mData->mProtoTunnel, 'rbuf', mData->mConfig.mTunnelSocketRecvBuffSize, 0, nullptr);
            }

            if (mData->mConfig.mTunnelSocketSendBuffSize > 0)
            {
                ProtoTunnelControl(mData->mProtoTunnel, 'sbuf', mData->mConfig.mTunnelSocketSendBuffSize, 0, nullptr);
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[CANA:Network:%p] Error: Could not create needed tunnel!\n", this);

            return(false);
        }
    }

    // hook up our tunnel to our connapi
    ConnApiControl(mConnApi, 'stun', 0, 0, mData->mProtoTunnel);

    uTunnelPort = static_cast<uint16_t>(ConnApiStatus(mConnApi, 'tprt', nullptr, 0));

    // we need the tunnel port information to pass to ProtoTunnel for XboxOne platforms
    if ((mMesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED) && !mMesh->isTopologyHost())
    {
        // ** tunnel port adjustment **

        // retrieve value received from the blaze server
        switch (mMesh->getDedicatedServerHostMeshEndpoint()->getNetworkAddress()->getActiveMember())
        {
            case NetworkAddress::MEMBER_IPADDRESS:
                uTunnelPort = mMesh->getDedicatedServerHostMeshEndpoint()->getNetworkAddress()->getIpAddress()->getPort();
                break;

            case NetworkAddress::MEMBER_IPPAIRADDRESS:
                // if dedicated server is behind same NAT as local console, then specify the server port to be the internal game port instead of the external game port
                if (mMesh->getDedicatedServerHostMeshEndpoint()->getNetworkAddress()->getIpPairAddress()->getExternalAddress().getIp() ==
                    mData->mBlazeHub->getConnectionManager()->getExternalAddress()->getIp())
                {
                    uTunnelPort = mMesh->getDedicatedServerHostMeshEndpoint()->getNetworkAddress()->getIpPairAddress()->getInternalAddress().getPort();
                }
                else
                {
                    uTunnelPort = mMesh->getDedicatedServerHostMeshEndpoint()->getNetworkAddress()->getIpPairAddress()->getExternalAddress().getPort();
                }

                break;

            default:
                // if dirtycast game server advertised an xlsp address, BlazeSDK is in charge of resolving it
                BlazeAssertMsg(false,  "[CANA:Network]  FATAL: when connecting to a dirtycast game server, game network address shall be of type 'MEMBER_IPADDRESS'");
                break;
        }
    }

    // tell ConnApi to enable tunneling, give it our remote tunnel port
    ConnApiControl(mConnApi, 'tunl', TRUE, uTunnelPort, nullptr);

#if defined(EA_PLATFORM_XBOXONE) && !defined(EA_PLATFORM_XBOX_GDK)
    // if dedicated server, force prototunnel to rebind to 0 such that an ephemeral port is selected.  this allows for
    // unsecure traffic to flow through the socket without explicitely altering the network manifest for that purpose.
    if (mMesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED)
    {
        ProtoTunnelControl(mData->mProtoTunnel, 'bnds', uTunnelPort, 0, nullptr);
        mServerTunnelPort = uTunnelPort;
    }
    else
    {
        mServerTunnelPort = 0;
    }
#endif

    BLAZE_SDK_DEBUGF("[CANA:Network:%p] Connectivity settings: 'tprt'=%d  mesh_uuid=%s\n", this, uTunnelPort, mMesh->getUUID());

    return(true);
}

void Network::setupDemangler()
{
#if !defined(EA_PLATFORM_XBOXONE) || defined(EA_PLATFORM_XBOX_GDK)
    // turn on/off demangler use
    ConnApiControl(mConnApi, 'mngl', mData->mConfig.mEnableDemangler, 0, nullptr);
#else
    ConnApiControl(mConnApi, 'exsn', 0, 0, const_cast<char*>(mMesh->getExternalSessionName()));
    ConnApiControl(mConnApi, 'exst', 0, 0, const_cast<char*>(mMesh->getExternalSessionTemplateName()));
    ConnApiControl(mConnApi, 'scid', 0, 0, const_cast<char*>(mMesh->getScid()));
#endif
}

void Network::setupMesh()
{
    // setup our network topology
    if (mMesh->getNetworkTopology() == NETWORK_DISABLED)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] setting up a mesh, with topology NETWORK_DISABLED.\n", this);
        mGameTopology = CONNAPI_GAMETOPOLOGY_DISABLED;
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] setting up a mesh, game type = Game\n", this);
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] network topology used: %s\n", this, GameNetworkTopologyToString(mMesh->getNetworkTopology()));
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] topology host: Blaze(%d)\n", this, mMesh->getTopologyHostConnectionSlotId());

        switch(mMesh->getNetworkTopology())
        {
            case CLIENT_SERVER_PEER_HOSTED:
            {
                mGameTopology = CONNAPI_GAMETOPOLOGY_PEERHOSTED;
                break;
            }
            case CLIENT_SERVER_DEDICATED:
            {
                mGameTopology = CONNAPI_GAMETOPOLOGY_SERVERHOSTED;
                break;
            }
            case PEER_TO_PEER_FULL_MESH:
            {
                mGameTopology = CONNAPI_GAMETOPOLOGY_PEERWEB;
                break;
            }
            default:
            {
                BlazeAssertMsg(false, "Error: Network::setupMesh(), unhandled topology for game mesh (game type = game)\n");
                break;
            }
        } // end of switch
    }

    BLAZE_SDK_DEBUGF("[CANA:Network:%p] platform host -->  player slot id = %d / connection slot id = %d\n", this, mMesh->getPlatformHostSlotId(), mMesh->getPlatformHostConnectionSlotId());
}

void Network::setupVoip()
{
    if (mMesh == nullptr || mData == nullptr)
    {
        BlazeAssert(mMesh != nullptr);
        BlazeAssert(mData != nullptr);
        return;
    }

    bool useVoip = true;

    // turn off voip if the mesh is set for no voip, or if the network type wasn't set to include it
    if ((mMesh->getVoipTopology() == VOIP_DISABLED) || !mData->mConfig.mEnableVoip)
    {
        useVoip = false;
    }

    // disable voip if the Player Settings Service value indicates that voip is disabled
    if (mMesh->getLocalMeshEndpoint()->isVoipDisabled())
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::setupVoip() is disabling VoIP due to PSS value\n", this);
        useVoip = false;
    }

    // for voip to be usable, VoipStartup() must have been called (internally by ConnApiAdpater or externally by game code) prior to this point
    // turn off voip if this requirement is not met
    if (VoipGetRef() == nullptr)
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::setupVoip() cannot succeed because VoipStartup() was not called\n", this);
        useVoip = false;
    }

    if (useVoip)
    {
        if (mMesh->getVoipTopology() == VOIP_PEER_TO_PEER)
        {
            mVoipTopology = CONNAPI_VOIPTOPOLOGY_PEERWEB;
        }
        else if (mMesh->getVoipTopology() == VOIP_DEDICATED_SERVER)
        {
            mVoipTopology = CONNAPI_VOIPTOPOLOGY_SERVERHOSTED;
        }

        // set to use crossplay or not; we should only be using cross play in VOIP_DEDICATED_SERVER, but we might be turning off cross play too
        VoipGroupRefT *voipGroupRef;
        ConnApiStatus(mConnApi, 'vgrp', &voipGroupRef, sizeof(voipGroupRef));
        VoipGroupControl(voipGroupRef, 'xply', static_cast<const GameManager::Game*>(mMesh)->isCrossplayEnabled(), 0, nullptr);

        // any local user can now be changed into a "participating" voip user
        for (uint16_t uIndex = 0; uIndex < mMesh->getMeshMemberCount(); uIndex++)
        {
            const MeshMember* pMeshMember = mMesh->getMeshMemberByIndex(uIndex);

            if (pMeshMember->isLocal())
            {
                uint32_t uLocalUserIndex = pMeshMember->getLocalUserIndex();
                
                if (uLocalUserIndex < VOIP_MAXLOCALUSERS)
                {
                    VoipGroupActivateLocalUser(voipGroupRef, uLocalUserIndex, TRUE);
                    mParticipatingVoipLocalUsers[uLocalUserIndex] = true;
                }
                else
                {
                    BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::addMemberOnEndpoint() local user index out of range (%u)\n", this, uIndex);
                }
            }
        }
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::setupVoip() VoIP DISABLED\n", this);
        mVoipTopology = CONNAPI_VOIPTOPOLOGY_DISABLED;
    }
}

void Network::idleNetwork(const uint32_t currentTime, const uint32_t elapsedTime)
{
    handleDeferredEvents();
}

void Network::activateNetwork()
{
    int32_t iSessID;
    int32_t iGameHostIndex = 0, iVoipHostIndex = 0;

    BlazeAssert(mMesh->getLocalMeshEndpoint() != nullptr);
    uint32_t uSelfId;
    if (mMesh->getLocalMeshEndpoint())
    {
        uSelfId = static_cast<uint32_t>(mMesh->getLocalMeshEndpoint()->getConnectionGroupId());
    }
    else
    {
        uSelfId = static_cast<uint32_t>(mMesh->getTopologyHostConnectionGroupId());
    }

    eastl::string gameId;
    gameId.sprintf("0x%016llx", mMesh->getId());

    DirtyMemGroupEnter(MEM_ID_DIRTY_SOCK, (void*)Blaze::Allocator::getAllocator(MEM_GROUP_NETWORKADAPTER));
    ConnApiOnline(mConnApi, gameId.c_str(), uSelfId, mGameTopology, mVoipTopology);

    iSessID = (int32_t)mMesh->getId();

    // set the host indices, connapi will decide when this information is important based on the topology settings
    // we use the same indices for game and host as gamemanager doesn't have the concept of these two hosts being
    // seperate. if the concept gets introduced at any point we will be ready with our API
    iGameHostIndex = iVoipHostIndex = mMesh->getTopologyHostConnectionSlotId();
    // connapi will internally use the game topology settings and the voip topology settings to identify which client(s)
    // in the clientlist it needs to establish a game connection with and which client(s) in the clientlist it needs to
    // establish a voip connection with.
    ConnApiConnect(mConnApi, mConnapiClientList, static_cast<int32_t>(mClientListSize), iGameHostIndex, iVoipHostIndex, iSessID);

    DirtyMemGroupLeave();
}

void Network::addConnApiClient(const MeshEndpoint* meshEndpoint)
{
    ConnApiClientInfoT ClientInfo;
    memset(&ClientInfo, 0, sizeof(ClientInfo));

    if (initConnApiClient(&ClientInfo, meshEndpoint))
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] Network::connectToEndpoint(0x%016" PRIX64 ") for mesh (%" PRId64 "/0x%016" PRIX64 ":%s) -- ConnApiAddClient(0x%08x) for connection slot id %d\n",
            this, meshEndpoint->getConnectionGroupId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getId(), meshEndpoint->getMesh()->getName(), ClientInfo.uId, meshEndpoint->getConnectionSlotId());

        int32_t ret = ConnApiAddClient(mConnApi, &ClientInfo, meshEndpoint->getConnectionSlotId());

        if (ret != 0)
        {
            switch (ret)
            {
            case CONNAPI_ERROR_INVALID_STATE:
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] ERR: ConnApiAddClient() failed with CONNAPI_ERROR_INVALID_STATE.)\n", this);
                break;
            case CONNAPI_ERROR_CLIENTLIST_FULL:
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] ERR: ConnApiAddClient() failed with CONNAPI_ERROR_CLIENTLIST_FULL.)\n", this);
                break;
            case CONNAPI_ERROR_SLOT_USED:
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] ERR: ConnApiAddClient() failed with CONNAPI_ERROR_SLOT_USED.)\n", this);
                break;
            case CONNAPI_ERROR_SLOT_OUT_OF_RANGE:
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] ERR: ConnApiAddClient() failed with CONNAPI_ERROR_SLOT_OUT_OF_RANGE.)\n", this);
                break;
            default:
                BLAZE_SDK_DEBUGF("[CANA:Network:%p] ERR: ConnApiAddClient() failed with default.)\n", this);
                break;
            }

            Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::ConnectionFlagsBitset connectionFlags(0);
            mData->mListenerDispatcher->dispatch("connectedToEndpoint", &NetworkMeshAdapterListener::connectedToEndpoint, mMesh, meshEndpoint->getConnectionGroupId(), connectionFlags, NetworkMeshAdapter::ERR_INTERNAL);
        }
    }
    else
    {
        BLAZE_SDK_DEBUGF("[CANA:Network:%p] ERR: addConnApiClient() initConnApiClient failed.)\n", this);
        Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::ConnectionFlagsBitset connectionFlags(0);
        mData->mListenerDispatcher->dispatch("connectedToEndpoint", &NetworkMeshAdapterListener::connectedToEndpoint, mMesh, meshEndpoint->getConnectionGroupId(), connectionFlags, NetworkMeshAdapter::ERR_INTERNAL);
    }
}

}
}
