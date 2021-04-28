/*H********************************************************************************/
/*!
    \File    gameservernetworkadapter.cpp

    \Description
        This module implements the NetworkMeshAdapter for the GameServer.
        It is a private header that should only be included by blaze specific
        files.

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "BlazeSDK/internetaddress.h"
#include "BlazeSDK/networkmeshadapterlistener.h"
#include "BlazeSDK/gamemanager/game.h"

#include "DirtySDK/dirtysock/dirtylib.h"

#include "dirtycast.h"
#include "gameserverconfig.h"
#include "gameservernetworkadapter.h"

#include "gameserverdist.h"

/*** Private Functions ************************************************************/

int32_t GameServerBlazeNetworkAdapterC::GameLinkUpdate(GameServerLinkClientListT *pClientList, int32_t iClient, void *pUpdateData, char *pLogBuffer)
{
    GameServerBlazeNetworkAdapterC *pAdaptor = static_cast<GameServerBlazeNetworkAdapterC *>(pUpdateData);
    return(GameServerDistUpdateClient(pClientList, iClient, pAdaptor->m_pGameDist, pLogBuffer));
}

void GameServerBlazeNetworkAdapterC::GameClientEvent(GameServerLinkClientListT *pClientList, GameServLinkClientEventE eEvent, int32_t iClient, void *pUpdateData)
{
    GameServerBlazeNetworkAdapterC *pAdaptor = static_cast<GameServerBlazeNetworkAdapterC *>(pUpdateData);

    if (pAdaptor->m_aConnections[iClient] != 0)
    {
        Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::ConnectionFlagsBitset ConnectionFlags;

        switch (eEvent)
        {
        case GSLINK_CLIENTEVENT_ADD:
            // ignored as it is driven by connectToEndpoint and thus doesn't need to be acted on
            break;

        case GSLINK_CLIENTEVENT_CONN:
            DirtyCastLogPrintfVerbose(pAdaptor->m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: dispatching connectToEndpoint, (conn slot %d, 0x%016llx)\n", iClient, pAdaptor->m_aConnections[iClient]);
            pAdaptor->mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::connectedToEndpoint, pAdaptor->m_pMesh, pAdaptor->m_aConnections[iClient], ConnectionFlags, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK);
            break;

        case GSLINK_CLIENTEVENT_DSC:
            DirtyCastLogPrintfVerbose(pAdaptor->m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: dispatching connectionToEndpointLost, (conn slot %d, 0x%016llx)\n", iClient, pAdaptor->m_aConnections[iClient]);
            pAdaptor->mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::connectionToEndpointLost, pAdaptor->m_pMesh, pAdaptor->m_aConnections[iClient], ConnectionFlags, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::ERR_CONNECTION_FAILED);
            pAdaptor->m_aConnections[iClient] = 0;
            break;

        case GSLINK_CLIENTEVENT_DEL:
            DirtyCastLogPrintfVerbose(pAdaptor->m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: dispatching disconnectedFromEndpoint, (conn slot %d, 0x%016llx)\n", iClient, pAdaptor->m_aConnections[iClient]);
            pAdaptor->mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::disconnectedFromEndpoint, pAdaptor->m_pMesh->getMeshEndpointByConnectionGroupId(pAdaptor->m_aConnections[iClient]), ConnectionFlags, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK);
            pAdaptor->m_aConnections[iClient] = 0;
            break;

        default:
            DirtyCastLogPrintf("gameservernetworkadaptor: GameClientEvent unhandled reason\n");
            break;
        }
    }

    // fire the event for any higher level listeners
    pAdaptor->m_pLinkParams->pEventFunc(pClientList, eEvent, iClient, pAdaptor->m_pLinkParams->pUserData);
    return(GameServerDistClientEvent(pClientList, eEvent, iClient, pAdaptor->m_pGameDist));
}

/*** Public Functions *************************************************************/

GameServerBlazeNetworkAdapterC::GameServerBlazeNetworkAdapterC()
    : m_pMesh(NULL)
    , m_pConfig(NULL)
    , m_pLinkParams(NULL)
    , m_pGameLink(NULL)
    , m_pGameLinkCust(NULL)
    , m_pGameDist(NULL)
{
    ds_memclr(m_aConnections, sizeof(m_aConnections));
}

void GameServerBlazeNetworkAdapterC::initialize(Blaze::BlazeHub* pBlazeHub)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: initialize\n");
}

void GameServerBlazeNetworkAdapterC::destroy()
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: destroy\n");
}

bool GameServerBlazeNetworkAdapterC::isInitialized(const Blaze::Mesh* pMesh) const
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: isInitialized\n");
    return(true);
}

bool GameServerBlazeNetworkAdapterC::isInitialized() const
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: isInitialized (adapter) \n");
    return(true);
}

const Blaze::NetworkAddress &GameServerBlazeNetworkAdapterC::getLocalAddress() const
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: getLocalAddress\n");
    return(m_LocalAddress);
}

#if BLAZE_SDK_VERSION < BLAZE_SDK_MAKE_VERSION(15, 1, 1, 8, 0)
const Blaze::NetworkAddress &GameServerBlazeNetworkAdapterC::getLocalAddress(uint32_t uUserIndex) const
{
    return(getLocalAddress());
}
#endif

void GameServerBlazeNetworkAdapterC::idle(const uint32_t uCurrentTime, const uint32_t uElapsedTime)
{
}

void GameServerBlazeNetworkAdapterC::initNetworkMesh(const Blaze::Mesh *pMesh, const Config &Config)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: initNetworkMesh\n");

    m_pMesh = pMesh;
    const uint32_t uMaxClients = static_cast<const Blaze::GameManager::Game *>(pMesh)->getMaxPlayerCapacity();

    // allocate the gameserverlink module
    if ((m_pGameLink = GameServerLinkCreate(m_pLinkParams->aCommandTags, m_pLinkParams->aConfigsTags, uMaxClients, m_pConfig->uDebugLevel, m_pConfig->uRedundancyThreshold,
                                            m_pConfig->uRedundancyLimit, m_pConfig->iRedundancyTimeout, m_pLinkParams->pSendFunc, m_pLinkParams->pDisFunc, m_pLinkParams->pUserData)) != NULL)
    {
        GameServerLinkCallback(m_pGameLink, &GameServerBlazeNetworkAdapterC::GameClientEvent, &GameServerBlazeNetworkAdapterC::GameLinkUpdate, this);
    }
    else
    {
        mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::networkMeshCreated, pMesh, 0u, NetworkMeshAdapter::ERR_CANNOT_INIT_NETWORK);
        return;
    }

    // add the endpoints that exist already to gameserverlink
    for (uint16_t uMeshEndpoint = 0; uMeshEndpoint < pMesh->getMeshEndpointCount(); uMeshEndpoint += 1)
    {
        const Blaze::MeshEndpoint *pMeshEndpoint = pMesh->getMeshEndpointByIndex(uMeshEndpoint);
        if (pMeshEndpoint->isLocal())
        {
            continue;
        }
        connectToEndpoint(pMeshEndpoint);
    }

    // create the dist ref
    if ((m_pGameDist = GameServerDistCreate(m_pLinkParams->aCommandTags, m_pLinkParams->aConfigsTags, uMaxClients, m_pConfig->uDebugLevel)) != NULL)
    {
        mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::networkMeshCreated, pMesh, 0u, NetworkMeshAdapter::ERR_OK);
    }
    else
    {
        mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::networkMeshCreated, pMesh, 0u, NetworkMeshAdapter::ERR_CANNOT_INIT_NETWORK);
    }
}

void GameServerBlazeNetworkAdapterC::connectToEndpoint(const Blaze::MeshEndpoint *pMeshEndpoint)
{
    uint32_t uAddr = 0;

    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: connectToEndpoint, (endpoint %p, blaze slot %d, conn slot %d, conngroup 0x%016llx)\n",
        pMeshEndpoint, pMeshEndpoint->getConnectionSlotId(), pMeshEndpoint->getConnectionSlotId() - 1, pMeshEndpoint->getConnectionGroupId());

    switch (pMeshEndpoint->getNetworkAddress()->getActiveMember())
    {
        case Blaze::NetworkAddress::MEMBER_IPPAIRADDRESS:
        {
            if ((uAddr = pMeshEndpoint->getNetworkAddress()->getIpPairAddress()->getExternalAddress().getIp()) == 0)
            {
                uAddr = pMeshEndpoint->getNetworkAddress()->getIpPairAddress()->getInternalAddress().getIp();
            }
            break;
        }
        case Blaze::NetworkAddress::MEMBER_XBOXCLIENTADDRESS:
        {
            break;
        }
        default:
        {
            DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: no address available for joiner ID 0x%08x\n",
                pMeshEndpoint->getRemoteLowLevelConnectivityId());
            break;
        }
    }

    /* adjust the slot id to account for the game server not being in connapi client list, this ensures that the client
       indices are the same on both ends */
    m_aConnections[pMeshEndpoint->getConnectionSlotId() - 1] = pMeshEndpoint->getConnectionGroupId();
    GameServerLinkAddClient(m_pGameLink, pMeshEndpoint->getConnectionSlotId() - 1, pMeshEndpoint->getRemoteLowLevelConnectivityId(), pMeshEndpoint->getLocalLowLevelConnectivityId(), uAddr, pMeshEndpoint->getTunnelKey());
}

void GameServerBlazeNetworkAdapterC::addMemberOnEndpoint(const Blaze::MeshMember *pMeshMember)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: addMemberOnEndpoint, (user %p, conngroup 0x%016llx)\n", pMeshMember, pMeshMember->getMeshEndpoint()->getConnectionGroupId());
}

void GameServerBlazeNetworkAdapterC::removeMemberOnEndpoint(const Blaze::MeshMember *pMeshMember)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: removeMemberOnEndpoint, (user %p, conngroup 0x%016llx)\n", pMeshMember, pMeshMember->getMeshEndpoint()->getConnectionGroupId());
}

void GameServerBlazeNetworkAdapterC::disconnectFromEndpoint(const Blaze::MeshEndpoint *pMeshEndpoint)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: disconnectFromEndpoint\n");
    if (m_pGameLink != NULL)
    {
        GameServerLinkDelClient(m_pGameLink, static_cast<int32_t>(pMeshEndpoint->getRemoteLowLevelConnectivityId()));
    }
}

void GameServerBlazeNetworkAdapterC::destroyNetworkMesh(const Blaze::Mesh *mesh)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: destroyNetworkMesh\n");

    // unregister all the callback
    GameServerLinkCallback(m_pGameLink, NULL, NULL, NULL);

    // destroy the dist ref
    GameServerDistDestroy(m_pGameDist);
    m_pGameDist = NULL;

    // destroy the link ref
    GameServerLinkDestroy(m_pGameLink);
    m_pGameLink = NULL;

    // clear connection group id array as we have no longer have connections
    ds_memclr(m_aConnections, sizeof(m_aConnections));

    mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::networkMeshDestroyed, mesh, NetworkMeshAdapter::ERR_OK);
}

void GameServerBlazeNetworkAdapterC::startGame(const Blaze::Mesh *pMesh)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: startGame\n");

    GameServerLinkClientListT *pServerLinkClientList = GameServerLinkClientList(m_pGameLink);
    GameServerDistStartGame(pServerLinkClientList, m_pGameDist);
}

void GameServerBlazeNetworkAdapterC::endGame(const Blaze::Mesh *pMesh)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: endGame\n");

    GameServerLinkClientListT *pServerLinkClientList = GameServerLinkClientList(m_pGameLink);
    GameServerDistStopGame(pServerLinkClientList, m_pGameDist);
}

void GameServerBlazeNetworkAdapterC::resetGame(const Blaze::Mesh *pMesh)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: resetGame\n");
}

bool GameServerBlazeNetworkAdapterC::getConnectionTimersForEndpoint(const Blaze::MeshEndpoint *pEndpoint, Blaze::BlazeNetworkAdapter::MeshEndpointConnTimer &GameConnectTimers, Blaze::BlazeNetworkAdapter::MeshEndpointConnTimer &VoipConnectTimers)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: getConnectionTimersForEndpoint\n");

    GameServerLinkClientListT *pClientList = GameServerLinkClientList(m_pGameLink);
    GameConnectTimers.ConnectTime = pClientList->Clients[pEndpoint->getConnectionSlotId() - 1].uConnTime;

    return(true);
}

bool GameServerBlazeNetworkAdapterC::getQosStatisticsForEndpoint(const Blaze::MeshEndpoint *pEndpoint, Blaze::BlazeNetworkAdapter::MeshEndpointQos &QosData, bool bInitialQosStats)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: getQosStatisticsForEndpoint\n");

    NetGameLinkStatT Stat;
    if (GameServerLinkStatus(m_pGameLink, 'stat', (pEndpoint->getConnectionSlotId() - 1), &Stat, sizeof(Stat)) < 0)
    {
        return(false);
    }

    if (bInitialQosStats)
    {
        QosData.LatencyMs = GameServerLinkStatus(m_pGameLink, 'qlat', (pEndpoint->getConnectionSlotId() - 1), NULL, 0);
    }
    else
    {
        QosData.LatencyMs = Stat.late;
    }
    QosData.LocalPacketsReceived = Stat.rpacksent - Stat.lpacklost;
    QosData.LocalPacketsLost = Stat.lpacklost;
    QosData.RemotePacketsSent = Stat.rpacksent;

    return(true);
}

void GameServerBlazeNetworkAdapterC::getConnectionStatusFlagsForMeshEndpoint(const Blaze::MeshEndpoint *pEndpoint, uint32_t &uConnectionFlag)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: getConnectionStatusFlagsForMeshEndpoint\n");
}

void GameServerBlazeNetworkAdapterC::migrateTopologyHost(const Blaze::Mesh *pMesh)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: migrateTopologyHost\n");
    mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::migratedTopologyHost, pMesh, NetworkMeshAdapter::ERR_OK);
}

void GameServerBlazeNetworkAdapterC::migratePlatformHost(const Blaze::Mesh *pMesh)
{
    DirtyCastLogPrintfVerbose(m_pConfig->uDebugLevel, DIRTYCAST_DBGLVL_BLAZE, "gameservernetworkadapter: migratePlatformHost\n");
    mListenerDispatcher.dispatch(&Blaze::BlazeNetworkAdapter::NetworkMeshAdapterListener::migratedPlatformHost, pMesh, NetworkMeshAdapter::ERR_OK);
}

void GameServerBlazeNetworkAdapterC::activateDelayedVoipOnEndpoint(const Blaze::MeshEndpoint *pEndpoint)
{
}

const char8_t *GameServerBlazeNetworkAdapterC::getVersionInfo() const
{
    return "0.0";
}

void GameServerBlazeNetworkAdapterC::SetConfig(const GameServerConfigT *pConfig, const GameServerLinkCreateParamT *pLinkParams)
{
    m_pConfig = pConfig;
    m_pLinkParams = pLinkParams;

    // setup the local address
    Blaze::IpAddress Ip;
    Ip.setIp(m_pLinkParams->uAddr);
    Ip.setPort(m_pLinkParams->uPort);
    m_LocalAddress.setIpAddress(&Ip);
}

GameServerLinkT *GameServerBlazeNetworkAdapterC::GetGameServerLink()
{
    return(m_pGameLink);
}

GameServerDistT *GameServerBlazeNetworkAdapterC::GetGameServerDist()
{
    return(m_pGameDist);
}
