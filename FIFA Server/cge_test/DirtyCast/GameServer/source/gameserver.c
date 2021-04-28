/*H********************************************************************************/
/*!
    \File gameserver.c

    \Description
        This module implements the main logic for the GameServer server.

        Description forthcoming.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 04/03/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/proto/prototunnel.h"

#include "configfile.h"
#include "dirtycast.h"
#include "gameserverblaze.h"
#include "gameserverconfig.h"
#include "gameserverdiagnostic.h"
#include "gameserverpkt.h"
#include "gameserverstat.h"
#include "serverdiagnostic.h"
#include "gameserver.h"
#include "gameserveroimetrics.h"
#include "gameserverdist.h"

/*** Defines **********************************************************************/

//! memory id
#define GAMESERVER_MEMID ('gsrv')

//! elapsed time after disconnect before gameserver tries to reconnect
#define GAMESERVER_VOIPSERVER_RECONNECTTIME     (30*1000)
#define GAMESERVER_RECREATETIME     (30*1000)
#define GAMESERVER_MAXPLAYERS       (30)
#define GAMESERVER_TOKENTIME        (30*1000)

/*** Type Definitions *************************************************************/

typedef void (GameServerDispatchFuncT)(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize);

typedef struct GameServerDispatchT
{
    uint32_t uKind;
    GameServerDispatchFuncT *pDispatch;
} GameServerDispatchT;

//! module state
struct GameServerRefT
{
    //! config file reader instance
    ConfigFileT *pConfig;

    //! pid filename
    char strPidFileName[256];

    //! start time
    time_t uStartTime;

    //! signal flag state
    uint32_t uSignalFlags;

    //! local ip address
    uint32_t uLocalAddr;

    //! gameserver stats to report to lobbby server
    GameServerStatT Statistics;

    //! gameserver oi metrics (reports metrics to aggregator)
    uint32_t uPrevMetricsQueryTime;
    GameServerOiMetricsRefT *pOiMetricsRef;

    /*
        voip server state
    */
    BufferedSocketT *pVoipServerSocket;       //!< voipserver socket
    GameServerVoipStateE eVoipServerState;    //!< voipserver state
    uint32_t uLastVoipServerRecv;             //!< last update time from voipserver
    uint32_t uLastVoipServerSend;             //!< last time sent to voipserver
    int32_t iLastVoipServerSendResult;        //!< result of last send to voipserver
    uint32_t uVoipReconnectTime;              //!< gameserver reconnect time (after disconnect)
    GameServerPacketT PacketData;             //!< storage for packet data received from voipserver
    int32_t iPacketSize;                      //!< size of most recently received packet data
    uint32_t uFrontAddr;                      //!< front-end address of voipserver
    uint32_t uTunnelPort;                     //!< tunnel port of voipserver
    uint8_t bConnected;                       //!< are we currently connected
    uint8_t bCreatedGame;                     //!< did we create the game
    uint8_t bGotServerConf;                   //!< did we get the config
    uint8_t _pad;
    uint32_t uLastTokenRetrieve;              //!< last time we sent a 'tokn' request to the voipserver

    /*
        lobby state
    */
    uint8_t bGameStarted;                     //!< has current game been started yet?
    int32_t iGameCb;                          //!< lobby event callback
    GameServerLoginHistoryT aLoginHistory[GAMESERVER_LOGINHISTORYCNT]; //!< history of login failures
    int32_t iLoginHistoryCt;

    /*
        peer connection state
    */
    uint32_t uLastFixedUpdateTime;            //!< fixed update rate timer
    GameServerStatInfoT StatInfo;             //!< accumulated statistics over measurement period
    uint32_t uLastLateUpdateTime;             //!< most recent latency update time
    uint32_t uLastRedundancyUpdateTime;       //!< most recent redundancy update time

    //! diagnostic module
    ServerDiagnosticT *pServerDiagnostic;

    GameServerBlazeStateE eOldBlazeState;
    uint16_t uVoipServerDiagPort;

    uint8_t bGameCreated;                     //!< TRUE after sending 'ngam' to the VS, FALSE after sending 'dgam'

    //! timing debugging
    int32_t aTimeDistribution[GAMESERVER_MAX_TIME_DISTR];
    uint32_t uDebugLastFixedUpdate;
    uint32_t uDebugLastDistributionDisplay;
    uint32_t uGameCreateTime;

    //! last total redundancy limit change count read from gameserverlink
    uint32_t uLastTotalRlmtChangeCount;

    //! memory allocation info
    int32_t iMemGroup;
    void *pMemGroupUserData;

    GameServerConfigT Config;
};

/*** Function Prototypes **********************************************************/

static void _GameServerDispatch_conf(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize);
static void _GameServerDispatch_data(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize);
static void _GameServerDispatch_ping(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize);
static void _GameServerDispatch_stat(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize);
static void _GameServerDispatch_tokn(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize);
static void _GameServerDispatch_null(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize);

/*** Variables ********************************************************************/

//! dispatch table for packets received from voipserver
static const GameServerDispatchT _GameServer_aDispatch[] =
{
    { 'data', _GameServerDispatch_data },   // first as this is the most commonly expected packet
    { 'ping', _GameServerDispatch_ping },
    { 'stat', _GameServerDispatch_stat },
    { 'conf', _GameServerDispatch_conf },
    { 'tokn', _GameServerDispatch_tokn },
    { 0     , _GameServerDispatch_null }
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _GameServerGetFixedUpdateTime

    \Description
        Returns configured fixed update rate if a game is started, else 200.

    \Input *pGameServer - module state

    \Output
        int32_t         - update interval

    \Version  02/07/2013 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _GameServerGetFixedUpdateRate(GameServerRefT *pGameServer)
{
    return (pGameServer->bGameStarted ? pGameServer->Config.uFixedUpdateRate : 200);
}

/*F********************************************************************************/
/*!
    \Function _GameServerNextFixedUpdateTime

    \Description
        Calculate number of milliseconds until the next fixed-rate update.

    \Input *pGameServer - module state
    \Input uCurTick     - current millisecond tick count

    \Output
        int32_t         - time until next fixed-rate update, in milliseconds

    \Version  02/21/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerNextFixedUpdateTime(GameServerRefT *pGameServer, uint32_t uCurTick)
{
    int32_t iNextUpdateTime;

    // init timer?
    if (pGameServer->uLastFixedUpdateTime == 0)
    {
        pGameServer->uLastFixedUpdateTime = uCurTick;
    }

    // calculate delta between current time and previous update
    iNextUpdateTime = NetTickDiff(uCurTick, pGameServer->uLastFixedUpdateTime);

    // calculate time remaining until next update
    iNextUpdateTime = NetTickDiff(_GameServerGetFixedUpdateRate(pGameServer), iNextUpdateTime);

    // return time to caller
    return(iNextUpdateTime);
}

/*F********************************************************************************/
/*!
    \Function _GameServerDebugFixedRate

    \Description
        Debug fixed-rate updating.

    \Input *pGameServer - module state

    \Version  02/21/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerDebugFixedRate(GameServerRefT *pGameServer)
{
    uint32_t uCurTick = NetTick(), uElapsedTick;

    // init last update tracker
    if (pGameServer->uDebugLastFixedUpdate == 0)
    {
        pGameServer->uDebugLastFixedUpdate = uCurTick;
        pGameServer->uDebugLastDistributionDisplay = uCurTick;
    }

    // calculate elapsed tick value (milliseconds elapsed since previous update)
    uElapsedTick = NetTickDiff(uCurTick, pGameServer->uDebugLastFixedUpdate);

    // display last update time window (only warn if a game is started to limit spam)
    if ((uElapsedTick > (_GameServerGetFixedUpdateRate(pGameServer) + 10)) && (pGameServer->bGameStarted != 0))
    {
        DirtyCastLogPrintfVerbose(pGameServer->Config.uDebugLevel, 0, "gameserver: warning -- last fixed update took %dms\n", uElapsedTick);
    }
    pGameServer->uDebugLastFixedUpdate = uCurTick;

    // update tick histogram with current elapsed tick value
    if (uElapsedTick >= GAMESERVER_MAX_TIME_DISTR)
    {
        uElapsedTick = (GAMESERVER_MAX_TIME_DISTR - 1);
    }
    pGameServer->aTimeDistribution[uElapsedTick]++;

    // check to see if we should display tick histogram
    if (NetTickDiff(uCurTick, pGameServer->uDebugLastDistributionDisplay ) > 300000)
    {
        int32_t iIndex;
        char strBuffer[2048] = {0};

        for(iIndex = 0; iIndex < GAMESERVER_MAX_TIME_DISTR; iIndex++)
        {
            sprintf(strBuffer + strlen(strBuffer), "%d, ", pGameServer->aTimeDistribution[iIndex]);
            // exponential decay filter to smooth out past values into current ones.
            pGameServer->aTimeDistribution[iIndex] = pGameServer->aTimeDistribution[iIndex] * 4 / 5;
        }
        // only display histogram if a game is started to limit spam while inactive
        if (pGameServer->bGameStarted)
        {
            DirtyCastLogPrintfVerbose(pGameServer->Config.uDebugLevel, 0, "gameserver: update tick distribution: %s\n", strBuffer);
        }

        pGameServer->uDebugLastDistributionDisplay = uCurTick;
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerVSAddress

    \Description
        Returns the VS address

    \Input *pGameServer - pointer to GameServer ref
    \Input *pAddrText   - [out] storage for textual ip address
    \Input iAddrSize    - size of output buffer

    \Output
        char *          - VoipServer address

    \Version 06/05/2009 (jrainy)
*/
/********************************************************************************F*/
static char *_GameServerVSAddress(GameServerRefT *pGameServer, char *pAddrText, int32_t iAddrSize)
{
    // if voip server is localhost use front address so that its more meaningful
    if (ds_strnicmp("127.0.0.1", pGameServer->Config.strVoipServerAddr, 9) == 0)
    {
        SocketInAddrGetText(pGameServer->uFrontAddr, pAddrText, iAddrSize);
    }
    else
    {
        ds_strnzcpy(pAddrText, pGameServer->Config.strVoipServerAddr, iAddrSize);
    }
    return(pAddrText);
}

/*F********************************************************************************/
/*!
    \Function _GameServerSendPacket

    \Description
        Send a packet to VoipServer.

    \Input *pGameServer - gameserver module state
    \Input *pPacket     - packet to send
    \Input iPacketSize  - size of packet

    \Output
        int32_t         - GameServerPacketSend() return code

    \Version  04/17/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerSendPacket(GameServerRefT *pGameServer, const GameServerPacketT *pPacket, int32_t iPacketSize)
{
    int32_t iPayloadSize = iPacketSize - sizeof(pPacket->Header), iResult = -1;
    #if DIRTYCODE_LOGGING
    uint32_t uKind = SocketNtohl(pPacket->Header.uKind);
    uint32_t uVerb = ((uKind == 'data') || (uKind == 'stat')) ? 2 : 1;
    uVerb = (uKind == 'ping') ? 4 : uKind;
    #endif

    NetPrintfVerbose((pGameServer->Config.uDebugLevel, uVerb, "gameserver: [send] '%c%c%c%c' packet (%d bytes)\n",
        (uKind >> 24) & 0xff, (uKind >> 16) & 0xff, (uKind >>  8) & 0xff, (uKind & 0xff), iPacketSize));
    if ((iPayloadSize > 0) && (pGameServer->Config.uDebugLevel > 3))
    {
        if ((iPayloadSize > 64) && (pGameServer->Config.uDebugLevel < 5))
        {
            iPayloadSize = 64;
        }
        NetPrintMem(&pPacket->Packet, iPayloadSize, "gameserver->voipserver");
    }
    if (pGameServer->pVoipServerSocket != NULL)
    {
        iResult = GameServerPacketSend(pGameServer->pVoipServerSocket, pPacket);
        pGameServer->iLastVoipServerSendResult = iResult;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _GameServerBroadcast

    \Description
        Broadcast all received game data to all other peers (simple rebroadcasting).

    \Input *pPacket     - packet data to send
    \Input *pUserData   - gameserver ref

    \Version 04/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerBroadcast(GameServerPacketT *pPacket, void *pUserData)
{
    GameServerRefT *pGameServer = (GameServerRefT *)pUserData;
    uint32_t uSize;

    // format packet to send back to voipserver
    uSize = pPacket->Header.uSize;
    pPacket->Header.uKind = SocketHtonl(pPacket->Header.uKind);
    pPacket->Header.uCode = SocketHtonl(pPacket->Header.uCode);
    pPacket->Header.uSize = SocketHtonl(pPacket->Header.uSize);
    pPacket->Packet.GameData.iClientId = SocketHtonl(pPacket->Packet.GameData.iClientId);
    pPacket->Packet.GameData.iDstClientId = SocketHtonl(pPacket->Packet.GameData.iDstClientId);
    pPacket->Packet.GameData.uFlag = SocketHtons(pPacket->Packet.GameData.uFlag);
    pPacket->Packet.GameData.uTimestamp = SocketHtonl(pPacket->Packet.GameData.uTimestamp);

    // send back to voipserver for broadcast to all other clients
    _GameServerSendPacket(pGameServer, pPacket, uSize);
}

/*F********************************************************************************/
/*!
    \Function _GameServerDisconnect

    \Description
        Handle disconnection notification from peer module

    \Input *pClient     - disconnected client
    \Input *pUserData   - gameserver ref

    \Version 04/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerDisconnect(GameServerLinkClientT *pClient, void *pUserData)
{
    GameServerRefT *pGameServer = (GameServerRefT *)pUserData;
    if (pGameServer->bConnected)
    {
        GameServerBlazeKickClient(pClient->iClientId, pClient->uDisconnectReason);
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerLinkEvent

    \Description
        Event triggered from GameServerLink module

    \Input *pClientList - client list from gameserverlink
    \Input eEvent       - identifier of the event type
    \Input iClient      - which client in the list does this pertain to
    \Input *pUserData   - callback user data

    \Version  03/26/2019 (eesponda)
*/
/********************************************************************************F*/
static void _GameServerLinkEvent(GameServerLinkClientListT *pClientList, GameServLinkClientEventE eEvent, int32_t iClient, void *pUserData)
{
    int32_t iClientIndex, iDeletedClient = 0;
    uint32_t uSize;
    uint8_t bChanged = FALSE;
    GameServerPacketT GamePacket;
    GameServerRefT *pGameServer = (GameServerRefT *)pUserData;

    // new client was added, let the vs know
    if (eEvent == GSLINK_CLIENTEVENT_ADD)
    {
        ds_memclr(&GamePacket, sizeof(GamePacket));

        // set up packet to tell voipserver about new client
        GamePacket.Header.uKind = SocketHtonl('nusr');
        GamePacket.Packet.NewUser.iClientId = pClientList->Clients[iClient].iClientId;
        GamePacket.Packet.NewUser.iClientIdx = iClient;
        GamePacket.Packet.NewUser.iGameServerId = pClientList->Clients[iClient].iGameServerId;
        ds_strnzcpy(GamePacket.Packet.NewUser.strTunnelKey, pClientList->Clients[iClient].strTunnelKey, sizeof(GamePacket.Packet.NewUser.strTunnelKey));
        uSize = GameServerPacketHton(&GamePacket, &GamePacket, 'nusr') + sizeof(GamePacket.Header);
        GamePacket.Header.uSize = SocketHtonl(uSize);
        _GameServerSendPacket(pGameServer, &GamePacket, uSize);

        bChanged = TRUE;
    }
    // client was removed, let the vs know
    else if (eEvent == GSLINK_CLIENTEVENT_DEL)
    {
        ds_memclr(&GamePacket, sizeof(GamePacket));

        // set up packet to tell voipserver to remove user
        GamePacket.Header.uKind = SocketHtonl('dusr');
        GamePacket.Packet.DelUser.iClientId = iDeletedClient = pClientList->Clients[iClient].iClientId;
        GamePacket.Packet.DelUser.RemovalReason.uRemovalReason = GameServerBlazeGetLastRemovalReason(GamePacket.Packet.DelUser.RemovalReason.strRemovalReason,
                                                                                                     sizeof(GamePacket.Packet.DelUser.RemovalReason.strRemovalReason),
                                                                                                     &GamePacket.Packet.DelUser.uGameState);
        uSize = GameServerPacketHton(&GamePacket, &GamePacket, 'dusr') + sizeof(GamePacket.Header);
        GamePacket.Header.uSize = SocketHtonl(uSize);
        _GameServerSendPacket(pGameServer, &GamePacket, uSize);

        bChanged = TRUE;
    }

    // update client send list if client list has changed
    if ((bChanged) && (pClientList->iNumClients > 1))
    {
        DirtyCastLogPrintfVerbose(pGameServer->Config.uDebugLevel, DIRTYCAST_DBGLVL_MISCINFO, "gameserver: updating client list\n");

        ds_memclr(&GamePacket, sizeof(GamePacket));
        GamePacket.Header.uKind = SocketHtonl('clst');
        GamePacket.Packet.SetList.iNumClients = SocketHtonl(VOIPTUNNEL_MAXGROUPSIZE);
        uSize = sizeof(GamePacket.Header) + sizeof(GamePacket.Packet.SetList);
        GamePacket.Header.uSize = SocketHtonl(uSize);

        // create updated client list
        ds_memclr(GamePacket.Packet.SetList.aClientIds, sizeof(GamePacket.Packet.SetList.aClientIds));
        for (iClientIndex = 0; iClientIndex < pClientList->iMaxClients; iClientIndex += 1)
        {
            GamePacket.Packet.SetList.aClientIds[iClientIndex] = SocketHtonl(pClientList->Clients[iClientIndex].iClientId);
        }

        // send updated client list to voipserver
        for (iClientIndex = 0; iClientIndex < pClientList->iMaxClients; iClientIndex += 1)
        {
            if ((pClientList->Clients[iClientIndex].iClientId != 0) && (pClientList->Clients[iClientIndex].iClientId != iDeletedClient))
            {
                GamePacket.Packet.SetList.iClientId = SocketHtonl(pClientList->Clients[iClientIndex].iClientId);
                _GameServerSendPacket(pGameServer, &GamePacket, uSize);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerDispatch_conf

    \Description
        Handle 'conf' VoipServer packet.

    \Input *pGameServer - gameserver ref
    \Input *pPacket     - 'conf' packet
    \Input iPacketSize  - size of packet

    \Version 03/12/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerDispatch_conf(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize)
{
    char pBuffer[256];
    char strAddrText[20];

    DirtyCastLogPrintf("gameserver: got conf from voipserver (frontaddr=%a, tunlport=%d, diagport=%d)\n",
        pPacket->Packet.Config.uFrontAddr, pPacket->Packet.Config.uTunnelPort, pPacket->Packet.Config.uDiagPort);

    pGameServer->uFrontAddr = pPacket->Packet.Config.uFrontAddr;
    pGameServer->uTunnelPort = pPacket->Packet.Config.uTunnelPort;
    pGameServer->uVoipServerDiagPort = pPacket->Packet.Config.uDiagPort;
    pGameServer->eVoipServerState = GAMESERVER_VOIPSERVER_ONLINE;
    ds_strnzcpy(pGameServer->Config.strHostName, pPacket->Packet.Config.strHostName, GAMESERVER_MAX_HOSTNAME_LENGTH);
    ds_strnzcpy(pGameServer->Config.strDeployInfo, pPacket->Packet.Config.strDeployInfo, GAMESERVER_MAX_DEPLOYINFO_LENGTH);
    ds_strnzcpy(pGameServer->Config.strDeployInfo2, pPacket->Packet.Config.strDeployInfo2, GAMESERVER_MAX_DEPLOYINFO2_LENGTH);

    GameServerBlazeControl('addr', pGameServer->uFrontAddr, 0, NULL);
    GameServerBlazeControl('port', pGameServer->uTunnelPort, 0, NULL);

    ds_snzprintf(pBuffer, sizeof(pBuffer), "http://%s:%d", _GameServerVSAddress(pGameServer, strAddrText, sizeof(strAddrText)), pGameServer->uVoipServerDiagPort);

    GameServerBlazeControl('surl', 0, 0, (void *)pBuffer);

    pGameServer->bGotServerConf = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _GameServerDispatch_data

    \Description
        Handle 'data' (game data) VoipServer packet.

    \Input *pGameServer - gameserver ref
    \Input *pPacket     - 'data' packet
    \Input iPacketSize  - size of packet

    \Version 03/12/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerDispatch_data(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize)
{
    // show that we got the packet
    NetPrintfVerbose((pGameServer->Config.uDebugLevel, 3, "gameserver: got %d byte packet from user 0x%08x\n",
        iPacketSize - sizeof(pPacket->Header) - sizeof(pPacket->Packet.GameData) + sizeof(pPacket->Packet.GameData.aData),
        pGameServer->PacketData.Packet.GameData.iClientId));

    // let gamepeer handle it
    GameServerLinkRecv(GameServerBlazeGetGameServerLink(), pPacket);
}

/*F********************************************************************************/
/*!
    \Function _GameServerDispatch_ping

    \Description
        Handle 'ping' VoipServer packet.

    \Input *pGameServer - gameserver ref
    \Input *pPacket     - 'ping' packet
    \Input iPacketSize  - size of packet

    \Version 03/12/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerDispatch_ping(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize)
{
    uint32_t uSize;
    PingDataT PingData;

    NetPrintfVerbose((pGameServer->Config.uDebugLevel, 3, "gameserver: got ping from voipserver\n"));

    ds_memclr(&PingData, sizeof(PingData));

    GameServerBlazeStateE gameState = GameServerBlazeStatus('stat', 0, NULL, 0);

    // if the voip server is telling us to be in a zombie state, tell blaze
    if (pPacket->Packet.PingData.eState == kZombie) 
    {
        GameServerBlazeControl('zomb', TRUE, 0, NULL);
    }
    
    // map the blaze state the lobby state
    if ((GameServerBlazeStatus('zomb', 0, NULL, 0) == TRUE) && (gameState == kDisconnected))
    {
        PingData.eState = kZombie;  // if we are disconnected and in zombie state, report that we are now zombie
    }
#if DIRTYCODE_DEBUG
    else if (pPacket->Packet.PingData.eState == kActive)
    {
        // the voip server has asked us to lie to it, and just say we are active
        PingData.eState = kActive;
    }
#endif    
    else if (gameState < kCreatedGame)
    {
        PingData.eState = kOffline;
    }
    else if (gameState > kWaitingForPlayers)
    {
        PingData.eState = kActive;
    }
    else
    {
        PingData.eState = kAvailable;
    }

    // send response to voipserver, including state
    uSize = GameServerPacketEncode(pPacket, 'ping', 0, &PingData, sizeof(PingData));
    _GameServerSendPacket(pGameServer, pPacket, uSize);
}

/*F********************************************************************************/
/*!
    \Function _GameServerDispatch_stat

    \Description
        Handle 'stat' VoipServer packet.

    \Input *pGameServer - gameserver ref
    \Input *pPacket     - 'stat' packet
    \Input iPacketSize  - size of packet

    \Version 03/12/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerDispatch_stat(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize)
{
    #if DIRTYCODE_LOGGING
    // ref game/voip stat structures
    GameServerProtoStatT *pGameStat = &pPacket->Packet.GameStats.GameStat;
    GameServerProtoStatT *pVoipStat = &pPacket->Packet.GameStats.VoipStat;
    #endif

    // display stats to debug output
    if (pGameServer->Config.uDebugLevel > 1)
    {
        NetPrintf(("gameserver: got updated stat dump from voipserver\n"));
        NetPrintf(("gameserver:    %4s %5s %5s %5s %5s\n", "type", "psent", "precv", "bsent", "brecv"));
        NetPrintf(("gameserver:    %4s %5d %5d %5d %5d\n", "game",
            pGameStat->uPktsSent, pGameStat->uPktsRecv,
            pGameStat->uByteSent, pGameStat->uByteRecv));
        NetPrintf(("gameserver:    %4s %5d %5d %5d %5d\n", "voip",
            pVoipStat->uPktsSent, pVoipStat->uPktsRecv,
            pVoipStat->uByteSent, pVoipStat->uByteRecv));
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerDispatch_tokn

    \Description
        Handle 'tokn' VoipServer packet

    \Input *pGameServer - gameserver ref
    \Input *pPacket     - unknown packet
    \Input iPacketSize  - size of packet
*/
/********************************************************************************F*/
static void _GameServerDispatch_tokn(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize)
{
    DirtyCastLogPrintf("gameserver: received token from voipserver\n");

    // cleanup the blaze connection
    GameServerBlazeDisconnect();

    /* Because GameServerBlazeDisconnect() dispatches the onDisconnected event, which results in gameserverblaze
       invalidating its internallly cached token, it is important to use 'accs' to set the new token AFTER the 
       call to GameServerBlazeDisconnect(). */
    GameServerBlazeControl('accs', 0, 0, (void*)pPacket->Packet.Token.strAccessToken);

    /* attempt to login again and then set must login to false. we always set must login to make sure we are updating
       after the initial login */
    if (!GameServerBlazeLogin())
    {
        pGameServer->uSignalFlags = GAMESERVER_SHUTDOWN_IMMEDIATE;
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerDispatch_null

    \Description
        Handle an unrecognized GameServer command.

    \Input *pGameServer - gameserver ref
    \Input *pPacket     - unknown packet
    \Input iPacketSize  - size of packet

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerDispatch_null(GameServerRefT *pGameServer, GameServerPacketT *pPacket, int32_t iPacketSize)
{
    uint32_t uKind = pPacket->Header.uKind;
    DirtyCastLogPrintf("gameserver: unhandled voipserver packet of type '%c%c%c%c'\n",
        (uKind>>24)&0xff, (uKind>>16)&0xff, (uKind>>8)&0xff, (uKind>>0)&0xff);
}

/*F********************************************************************************/
/*!
    \Function _GameServerGameEvent

    \Description
        Handle connapi events.

    \Input *pGameServer - module state
    \Input iEvent       - event type
    \Input *pGame       - GameServerGameT object for the game to create
    \Input iClient      - index of client event is for

    \Output
        int32_t         - result of _GameServerSendPacket()

    \Version  04/17/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerGameEvent(GameServerRefT *pGameServer, int32_t iEvent, GameServerGameT *pGame, int32_t iClient)
{
    GameServerPacketT GamePacket;
    uint32_t uSize=0;

    // clear the packet
    ds_memclr(&GamePacket, sizeof(GamePacket));

    if (iEvent == 'ngam')
    {
        if (pGameServer->bGameCreated)
        {
            DirtyCastLogPrintf("gameserver: INCONSISTENCY in 'dgam' and 'ngam' messages. Skipping requested 'ngam'\n");
            return(-1);
        }

        pGameServer->bGameCreated = TRUE;
        // issue creation notification to voipserver
        GamePacket.Header.uKind = SocketHtonl('ngam');
        GamePacket.Packet.NewGame.iIdent = SocketHtonl(pGame->iIdent);
        ds_strnzcpy(GamePacket.Packet.NewGame.strGameUUID, pGame->strUUID, sizeof(GamePacket.Packet.NewGame.strGameUUID));
        uSize = sizeof(GamePacket.Header) + sizeof(GamePacket.Packet.NewGame);
    }
    if (iEvent == 'dgam')
    {
        // query total redundancy change count during lifetime of gameserverlink module
        uint32_t uNewTotalRlmtChangeCount = 0;
        if (GameServerBlazeGetGameServerLink() != NULL)
        {
            uNewTotalRlmtChangeCount = GameServerLinkStatus(GameServerBlazeGetGameServerLink(), 'rlmt', 0, NULL, 0);
        }

        if (!pGameServer->bGameCreated)
        {
            DirtyCastLogPrintf("gameserver: INCONSISTENCY in 'dgam' and 'ngam' messages. Skipping requested 'dgam'\n");
            return(-1);
        }

        pGameServer->bGameCreated = FALSE;

        // issue deletion notification to voipserver
        GamePacket.Header.uKind = SocketHtonl('dgam');
        GameServerBlazeGetLastRemovalReason(NULL, 0, &GamePacket.Packet.DelGame.uGameEndState);
        GamePacket.Packet.DelGame.uRlmtChangeCount = uNewTotalRlmtChangeCount - pGameServer->uLastTotalRlmtChangeCount;   // calculate change count for this game specifically
        pGameServer->uLastTotalRlmtChangeCount = uNewTotalRlmtChangeCount;                                                // save new total count
        uSize = GameServerPacketHton(&GamePacket, &GamePacket, 'dgam') + sizeof(GamePacket.Header);
    }
    if (iEvent == 'tokn')
    {
        // This is a request packet we don't fill out the strAccessToken (done by the response from the voipserver)
        GamePacket.Header.uKind = SocketHtonl('tokn');
        uSize = sizeof(GamePacket.Header) + sizeof(GamePacket.Packet.Token);
    }

    // make sure we're connected to voipserver before sending packet
    if ((pGameServer->eVoipServerState != GAMESERVER_VOIPSERVER_CONNECTED) && (pGameServer->eVoipServerState != GAMESERVER_VOIPSERVER_ONLINE))
    {
        return(-1);
    }

    // set size and send packet
    GamePacket.Header.uSize = SocketHtonl(uSize);
    return(_GameServerSendPacket(pGameServer, &GamePacket, uSize));
}

/*F********************************************************************************/
/*!
    \Function _GameServerCreateGame

    \Description
        Create a game based on lobby 'game' message.

    \Input *pGameServer - pointer to GameServer ref
    \Input *pGameObject - pointer to the game object

    \Version 08/26/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerCreateGame(GameServerRefT *pGameServer, GameServerGameT *pGameObject)
{
    struct tm when;

    // log game creation info
    ds_secstotime(&when, pGameObject->uWhen);
    DirtyCastLogPrintf("gameserver: creating game IDENT=%d WHEN=%d.%d.%d-%d:%02d:%02d NAME=%s UUID=%s GAMEMODE=%s\n", pGameObject->iIdent,
        when.tm_year+1900, when.tm_mon+1, when.tm_mday, when.tm_hour, when.tm_min, when.tm_sec,
        pGameObject->strName, pGameObject->strUUID, pGameObject->strGameMode);

    // update stats
    GameServerUpdateStatsGameCreate(&pGameServer->Statistics);

    // init latency accumulators
    GameServerPacketUpdateLateStats(&pGameServer->StatInfo.E2eLate, NULL);
    GameServerPacketUpdateLateStats(&pGameServer->StatInfo.InbLate, NULL);

    // init other state
    pGameServer->bGameStarted = FALSE;

    // send event notification to voipserver
    _GameServerGameEvent(pGameServer, 'ngam', pGameObject, -1);
}

/*F********************************************************************************/
/*!
    \Function _GameServerDeleteGame

    \Description
        Delete the currently active game

    \Input *pGameServer - pointer to GameServer ref

    \Version 08/15/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerDeleteGame(GameServerRefT *pGameServer)
{
    // skip deletion of an empty game
    if (!pGameServer->bGameCreated)
    {
        return;
    }

    DirtyCastLogPrintf("gameserver: deleting game\n");

    pGameServer->bGameStarted = FALSE;

    // update stats
    GameServerUpdateStatsGameDelete(&pGameServer->Statistics);

    // attempt to reduce the memory used by the buffered sockets
    if (pGameServer->pVoipServerSocket != NULL)
    {
        BufferedSocketControl(pGameServer->pVoipServerSocket, 'rset', 0, NULL, NULL);
    }

    // notify gameserveroimetrics of game end
    if (pGameServer->pOiMetricsRef != NULL)
    {
        GameServerOiMetricsControl(pGameServer->pOiMetricsRef, 'gend', 0, NULL, 0);
    }

    // notify voipserver of game deletion
    _GameServerGameEvent(pGameServer, 'dgam', NULL, -1);
}

/*F********************************************************************************/
/*!
    \Function _GameServerEvent_game

    \Description
        Handle lobby 'game' event message.

    \Input *pGameserver - pointer to GameServer ref
    \Input *pGameObject - the game object

    \Version 08/26/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _GameServerEvent_game(GameServerRefT *pGameServer, GameServerGameT *pGameObject)
{
    DirtyCastLogPrintfVerbose(pGameServer->Config.uDebugLevel, DIRTYCAST_DBGLVL_MISCINFO, "gameserver: got 'game' event: COUNT=%d\n", pGameObject->iCount);

    // if there is no name included, it is a delete event, so disconnect and bail
    if (pGameObject->strName[0] == '\0')
    {
        DirtyCastLogPrintf("gameserver: warning -- unhandled delete\n");
        //_GameManagerDisconnect(pGameManager);
        return(FALSE);
    }

    return(TRUE);
}
/*F********************************************************************************/
/*!
    \Function _GameServerEvent_play

    \Description
        Handle lobby 'play' event message.

    \Input *pGameServer - pointer to GameServer ref
    \Input *pGameObject - game object

    \Version 08/26/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerEvent_play(GameServerRefT *pGameServer, GameServerGameT *pGameObject)
{
    DirtyCastLogPrintf("gameserver: got 'play' event IDENT=%d\n", pGameObject->iIdent);

    // remember that game has been started
    pGameServer->bGameStarted = TRUE;

    /* Reset uDebugLastFixedUpdate to avoid _GameServerDebugFixedRate() incorrectly 
       spammming "gameserver: warning -- last fixed update took xxxx ms" immediately
       after the game is started and fixed rate is changed from low rate (200)
       to high rate (pGameServer->Config.uFixedUpdateRate). */
    pGameServer->uDebugLastFixedUpdate = 0;
}

//*F********************************************************************************/
/*!
    \Function _GameServerRecv

    \Description
        Process incoming data on the voipserver connection.

    \Input *pGameServer     - gameserver ref

    \Output
        int32_t             - negative=error, zero=no data or incomplete packet, one=read a packet

    \Version 04/25/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerRecv(GameServerRefT *pGameServer)
{
    int32_t iPayloadSize, iDispatch, iResult;
    #if DIRTYCODE_LOGGING
    uint32_t uVerb;
    #endif

    // recv packet from a remote host
    if ((iResult = GameServerPacketRecv(pGameServer->pVoipServerSocket, &pGameServer->PacketData, (unsigned *)&pGameServer->iPacketSize)) <= 0)
    {
        return(iResult);
    }

    // update last received time
    pGameServer->uLastVoipServerRecv = NetTick();

    // calculate payload size
    iPayloadSize = pGameServer->iPacketSize - sizeof(pGameServer->PacketData.Header);

    // display received packet in network order
    #if DIRTYCODE_LOGGING
    uVerb = (pGameServer->PacketData.Header.uKind == 'ping') ? 4 : 2;
    #endif
    NetPrintfVerbose((pGameServer->Config.uDebugLevel, uVerb, "gameserver: [recv] '%c%c%c%c' packet (%d bytes)\n",
        (pGameServer->PacketData.Header.uKind >> 24) & 0xff, (pGameServer->PacketData.Header.uKind >> 16) & 0xff,
        (pGameServer->PacketData.Header.uKind >>  8) & 0xff, (pGameServer->PacketData.Header.uKind & 0xff),
        pGameServer->iPacketSize));
    if ((iPayloadSize > 0) && (pGameServer->Config.uDebugLevel > 3))
    {
        if ((iPayloadSize > 64) && (pGameServer->Config.uDebugLevel < 5))
        {
            iPayloadSize = 64;
        }
        NetPrintMem(&pGameServer->PacketData.Packet.aData, iPayloadSize, "gameserver<-voipserver");
    }

    // convert packet body from network to host order in-place
    GameServerPacketNtoh(&pGameServer->PacketData, &pGameServer->PacketData, pGameServer->PacketData.Header.uKind);

    // dispatch the packet
    for (iDispatch = 0; ; iDispatch++)
    {
        const GameServerDispatchT *pDispatch = &_GameServer_aDispatch[iDispatch];
        if ((pGameServer->PacketData.Header.uKind == pDispatch->uKind) || (pDispatch->uKind == 0))
        {
            _GameServer_aDispatch[iDispatch].pDispatch(pGameServer, &pGameServer->PacketData, pGameServer->iPacketSize);
            break;
        }
    }

    // consume the packet
    pGameServer->iPacketSize = 0;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _GameServerVoipServerDisconnect

    \Description
        Disconnect from VoipServer

    \Input *pGameServer - module state
    \Input iError       - error (or zero)
    \Input *pReason     - reason for disconnect (or NULL)

    \Version 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerVoipServerDisconnect(GameServerRefT *pGameServer, int32_t iError, const char *pReason)
{
    // display disconnect reason
    DirtyCastLogPrintf("gameserver: disconnected from voipserver (err=%d reason=%s)\n", iError, pReason);

    // set to disconnected state
    pGameServer->eVoipServerState = GAMESERVER_VOIPSERVER_DISCONNECTED;

    // destroy voipserver socket
    if (pGameServer->pVoipServerSocket != NULL)
    {
        BufferedSocketClose(pGameServer->pVoipServerSocket);
        pGameServer->pVoipServerSocket = NULL;
    }

    // if there's an active game, kill it
    _GameServerDeleteGame(pGameServer);

    GameServerBlazeDestroyGame();
    pGameServer->bCreatedGame = FALSE;
    pGameServer->uGameCreateTime = 0;

    // start reconnection timer
    pGameServer->uVoipReconnectTime = NetTick() + GAMESERVER_VOIPSERVER_RECONNECTTIME;

    pGameServer->bGotServerConf = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _GameServerUpdateVoipServer

    \Description
        Update VoipServer connection.

    \Input *pGameServer - module state

    \Output
        int32_t         - zero

    \Version 04/03/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _GameServerUpdateVoipServer(GameServerRefT *pGameServer)
{
    GameServerPacketT GamePacket;
    int32_t iResult, iRecv;
    uint32_t uSize;

    if (pGameServer->eVoipServerState == GAMESERVER_VOIPSERVER_INIT)
    {
        struct sockaddr ConnAddr;

        // create voipserver socket
        if ((pGameServer->pVoipServerSocket = BufferedSocketOpen(SOCK_STREAM, 0, 32*1024, NULL)) == NULL)
        {
            DirtyCastLogPrintf("gameserver: unable to create voipserver socket\n");
            _GameServerVoipServerDisconnect(pGameServer, 0, "BufferedSocketOpen() failed");
            return(0);
        }
        // disable nagle on voipserver socket
        BufferedSocketControl(pGameServer->pVoipServerSocket, 'ndly', 1, NULL, NULL);

        // init connect address
        SockaddrInit(&ConnAddr, AF_INET);
        SockaddrInSetAddr(&ConnAddr, SocketInTextGetAddr(pGameServer->Config.strVoipServerAddr));
        SockaddrInSetPort(&ConnAddr, pGameServer->Config.uVoipServerPort);

        // connect to voipserver
        DirtyCastLogPrintf("gameserver: connecting to voipserver %s:%d\n", pGameServer->Config.strVoipServerAddr, pGameServer->Config.uVoipServerPort);
        if ((iResult = BufferedSocketConnect(pGameServer->pVoipServerSocket, &ConnAddr, sizeof(ConnAddr))) == SOCKERR_NONE)
        {
            pGameServer->eVoipServerState = GAMESERVER_VOIPSERVER_CONNECTING;
        }
        else
        {
            _GameServerVoipServerDisconnect(pGameServer, iResult, "BufferedSocketConnect() failed");
        }
    }
    if (pGameServer->eVoipServerState == GAMESERVER_VOIPSERVER_CONNECTING)
    {
        if ((iResult = BufferedSocketInfo(pGameServer->pVoipServerSocket, 'stat', 0, NULL, 0)) > 0)
        {
            GameServerConfigInfoT ConfigInfo;

            DirtyCastLogPrintf("gameserver: connected to voipserver\n");
            pGameServer->eVoipServerState = GAMESERVER_VOIPSERVER_CONNECTED;

            pGameServer->uLastVoipServerRecv = NetTick();

            // send configuration info to VoipServer
            ds_memclr(&ConfigInfo, sizeof(ConfigInfo));
            ConfigInfo.uDiagPort = pGameServer->Config.uDiagnosticPort;
            ds_strnzcpy(ConfigInfo.strProductId, pGameServer->Config.strServer, sizeof(ConfigInfo.strProductId));
            uSize = GameServerPacketEncode(&GamePacket, 'conf', 0, &ConfigInfo, sizeof(ConfigInfo));
            _GameServerSendPacket(pGameServer, &GamePacket, uSize);
        }
        else if (iResult < 0)
        {
            _GameServerVoipServerDisconnect(pGameServer, iResult, "BufferedSocketInfo() failed");
        }
    }
    if ((pGameServer->eVoipServerState == GAMESERVER_VOIPSERVER_CONNECTED) || (pGameServer->eVoipServerState == GAMESERVER_VOIPSERVER_ONLINE))
    {
        // try and receive data
        for (iRecv = 0; iRecv < 16; iRecv++)
        {
            if ((iResult = _GameServerRecv(pGameServer)) < 0)
            {
                _GameServerVoipServerDisconnect(pGameServer, iResult, "_GameServerRecv() failed");
                break;
            }
            else if (iResult == 0)
            {
                break;
            }
        }
        // check for timeout
        if ((signed)(NetTick() - pGameServer->uLastVoipServerRecv) > (signed)pGameServer->Config.uVoipServTime)
        {
            _GameServerVoipServerDisconnect(pGameServer, 0, "timeout");
        }
        // check to see if previous send attempt failed
        if (pGameServer->iLastVoipServerSendResult < 0)
        {
            _GameServerVoipServerDisconnect(pGameServer, 0, "send failed");
        }
        // check to see if we need to ask for another token
        if (!GameServerBlazeStatus('accs', 0, NULL, 0))
        {
            if ((uint32_t)NetTickDiff(NetTick(), pGameServer->uLastTokenRetrieve) > GAMESERVER_TOKENTIME)
            {
                DirtyCastLogPrintf("gameserver: timer expired, requesting a new token from voipserver\n");
                _GameServerGameEvent(pGameServer, 'tokn', NULL, -1);
                pGameServer->uLastTokenRetrieve = NetTick();
            }
        }
    }
    if (pGameServer->eVoipServerState == GAMESERVER_VOIPSERVER_DISCONNECTED)
    {
        if ((signed)(NetTick() - pGameServer->uVoipReconnectTime) > (signed)pGameServer->Config.uVoipServTime)
        {
            DirtyCastLogPrintf("gameserver: attempting reconnect to voipserver\n");
            pGameServer->eVoipServerState = GAMESERVER_VOIPSERVER_INIT;
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _GameServerAddLoginHistory

    \Description
        Add login failure code to login history

    \Input *pGameServer - module state
    \Input uFailKind    - failure kind (fourcc)
    \Input uFailCode    - failure code (integer)

    \Version 07/16/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerAddLoginHistory(GameServerRefT *pGameServer, uint32_t uFailKind, uint32_t uFailCode)
{
    // do not add if not an error or if we are simply moving to disconnected state before reconneciton attempt
    if ((uFailKind == 0) || (uFailKind == 'POOL'))
    {
        return;
    }

    // save login failure
    if ((uFailKind != pGameServer->aLoginHistory[0].uFailKind) || (uFailCode != pGameServer->aLoginHistory[0].uFailCode))
    {
        memmove(&pGameServer->aLoginHistory[1], &pGameServer->aLoginHistory[0], sizeof(pGameServer->aLoginHistory)-sizeof(pGameServer->aLoginHistory[0]));
        pGameServer->aLoginHistory[0].uFailTime = time(NULL);
        pGameServer->aLoginHistory[0].uFailKind = uFailKind;
        pGameServer->aLoginHistory[0].uFailCode = uFailCode;
        pGameServer->aLoginHistory[0].uFailCount = 1;
        if (pGameServer->iLoginHistoryCt < GAMESERVER_LOGINHISTORYCNT)
        {
            pGameServer->iLoginHistoryCt += 1;
        }
    }
    else
    {
        pGameServer->aLoginHistory[0].uFailCount += 1;
    }
}

/*F********************************************************************************/
/*!
    \Function _GameServerUpdateBlazeServer

    \Description
        Update the Blaze server, and track its state

    \Input *pGameServer - module state
    \Input uCurTick     - current tick count

    \Version ??/??/???? (jrainy)
*/
/********************************************************************************F*/
static void _GameServerUpdateBlazeServer(GameServerRefT *pGameServer, uint32_t uCurTick)
{
    GameServerBlazeStateE eNewState;
    uint8_t bDisconnection;

    // confirm that we have a valid blaze hub. we are waiting for the token to come back from the VS to create the blazehub
    if (!GameServerBlazeStatus('bhub', 0, NULL, 0))
    {
        return;
    }

    bDisconnection = GameServerBlazeUpdate();
    eNewState = (GameServerBlazeStateE)GameServerBlazeStatus('stat', 0, NULL, 0);

    // if we have a disconnection event, update our state and log the event
    if (bDisconnection)
    {
        uint32_t uKind = GameServerBlazeStatus('lknd', 0, NULL, 0);
        uint32_t uCode = GameServerBlazeStatus('lcde', 0, NULL, 0);

        if ((uKind != 0) && (uKind != 'POOL'))
        {
            DirtyCastLogPrintf("gameserver: blaze connection failure: %C/%08x (%s)\n", uKind, uCode, GameServerBlazeGetErrorCodeName(uCode));
        }

        pGameServer->bConnected = FALSE;

        // if there's an active game, kill it
        _GameServerDeleteGame(pGameServer);

        // add to login history
        _GameServerAddLoginHistory(pGameServer, uKind, uCode);

        pGameServer->bCreatedGame = FALSE;
        pGameServer->uGameCreateTime = uCurTick;
    }

    // watch for connection event and update our state accordingly
    if ((pGameServer->eOldBlazeState < kConnected) && (eNewState >= kConnected))
    {
        pGameServer->bConnected = TRUE;
    }

    // issue game creation request (recurring until successful)
    if (pGameServer->bConnected && !pGameServer->bCreatedGame && pGameServer->bGotServerConf && (GameServerBlazeStatus('sspr', 0, NULL, 0) == FALSE))
    {
        if ((pGameServer->uGameCreateTime == 0) || ((unsigned)NetTickDiff(uCurTick, pGameServer->uGameCreateTime) > GAMESERVER_RECREATETIME))
        {
            GameServerBlazeCreateGame();
            pGameServer->bCreatedGame = TRUE;
            pGameServer->uGameCreateTime = uCurTick;
        }
    }

    pGameServer->eOldBlazeState = eNewState;
}

/*F********************************************************************************/
/*!
    \Function GameServerEventCallback

    \Description
        Callback handler for lobby events.

    \Input *pGameServer - GameServer ref
    \Input iEvent       - type of event
    \Input *GameServerGameT - associated game object, when applicable

    \Version 08/26/2005 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t _GameServerEventCallback(void *pUserData, int32_t iEvent, GameServerGameT *pGameObject)
{
    GameServerRefT *pGameServer = (GameServerRefT *)pUserData;

    uint8_t bUpdate = FALSE;
    if (iEvent == GAMESERVER_EVENT_CREATE)
    {
        _GameServerCreateGame(pGameServer, pGameObject);
        bUpdate = TRUE;
    }
    if (iEvent == GAMESERVER_EVENT_DELETE)
    {
        _GameServerDeleteGame(pGameServer);
    }
    if (iEvent == GAMESERVER_EVENT_GAME)
    {
        bUpdate = _GameServerEvent_game(pGameServer, pGameObject);
    }
    if (iEvent == GAMESERVER_EVENT_PLAY)
    {
        _GameServerEvent_play(pGameServer, pGameObject);
    }
    if (iEvent == GAMESERVER_EVENT_END)
    {
        pGameServer->bGameStarted = FALSE;

        // notify gameserveroimetrics of game end
        if (pGameServer->pOiMetricsRef != NULL)
        {
            GameServerOiMetricsControl(pGameServer->pOiMetricsRef, 'gend', 0, NULL, 0);
        }

        DirtyCastLogPrintf("gameserver: cleaning up disconnected clients and resetting game started state\n");
    }
    if (iEvent == GAMESERVER_EVENT_LOGIN_FAILURE)
    {
        uint32_t uKind = GameServerBlazeStatus('lknd', 0, NULL, 0);
        uint32_t uCode = GameServerBlazeStatus('lcde', 0, NULL, 0);

        // add failure to login history
        _GameServerAddLoginHistory(pGameServer, uKind, uCode);
    }
    if (iEvent == GAMESERVER_EVENT_GAME_CREATION_FAILURE)
    {
        uint32_t uKind = GameServerBlazeStatus('lknd', 0, NULL, 0);
        uint32_t uCode = GameServerBlazeStatus('lcde', 0, NULL, 0);

        // set game created state
        pGameServer->bCreatedGame = FALSE;
        pGameServer->uGameCreateTime = NetTick();
        // add failure to login history
        _GameServerAddLoginHistory(pGameServer, uKind, uCode);
    }
    if (iEvent == GAMESERVER_EVENT_DRAIN)
    {
        // set shutdown flag to GAMESERVER_SHUTDOWN_IFEMPTY
        pGameServer->uSignalFlags = GAMESERVER_SHUTDOWN_IFEMPTY;
    }

    return(bUpdate);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function GameServerCreate

    \Description
        Initialize the GameServer.  Read the config file, setup the network,
        and setup data structures.  Once this method completes successfully...

    \Input iArgCt           - number of command-line arguments
    \Input **pArgs          - command-line argument list
    \Input *pConfigTagsFile - config file name

    \Output
        GameServerRefT *    - pointer to the gameserver module state, NULL on failure

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
GameServerRefT *GameServerCreate(int32_t iArgCt, const char **pArgs, const char *pConfigTagsFile)
{
    const char *pConfigTags, *pCommandTags;
    char strCmdTagBuf[4096], strCustomAttributes[1024];
    const char *pCustomAttributes = strCustomAttributes;
    int32_t iMaxPacket, iMemGroup;
    uint16_t uCustomAttr;
    GameServerRefT *pGameServer;
    void *pMemGroupUserData;

    // query memory system
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    if ((pGameServer = (GameServerRefT *)DirtyMemAlloc(sizeof(*pGameServer), GAMESERVER_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("gameserver: could not allocate module state\n");
        return(NULL);
    }
    ds_memclr(pGameServer, sizeof(*pGameServer));
    pGameServer->iMemGroup = iMemGroup;
    pGameServer->pMemGroupUserData = pMemGroupUserData;

    // save startup time
    pGameServer->uStartTime = time(NULL);

    // parse command-line parameters into tagfields
    pCommandTags = DirtyCastCommandLineProcess(strCmdTagBuf, sizeof(strCmdTagBuf), 2, iArgCt, pArgs, "gameserver.");

    // log startup and build options
    DirtyCastLogPrintf("gameserver: started debug=%d profile=%d strCmdTagBuf=%s\n", DIRTYCODE_DEBUG, DIRTYCODE_PROFILE, strCmdTagBuf);

    // get local ip address
    pGameServer->uLocalAddr = NetConnStatus('addr', 0, NULL, 0);

    // create config file instance
    if ((pGameServer->pConfig = ConfigFileCreate()) == NULL)
    {
        GameServerDestroy(pGameServer);
        return(NULL);
    }

    // load the config file
    DirtyCastLogPrintf("gameserver: loading config file %s\n", pConfigTagsFile);
    ConfigFileLoad(pGameServer->pConfig, pConfigTagsFile, "");
    ConfigFileWait(pGameServer->pConfig);

    // make sure we got a config file
    pConfigTags = ConfigFileData(pGameServer->pConfig, "");
    if ((pConfigTags == NULL) || (*pConfigTags == '\0'))
    {
        DirtyCastLogPrintf("gameserver: no config file found\n");
        GameServerDestroy(pGameServer);
        return(NULL);
    }

    // setup the logger
    DirtyCastLoggerConfig(pConfigTags);

    // get diagnostic port
    pGameServer->Config.uDiagnosticPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.diagnosticport", 0);

    // create pid file using diagnostic port in name
    if (DirtyCastPidFileCreate(pGameServer->strPidFileName, sizeof(pGameServer->strPidFileName), pArgs[0], pGameServer->Config.uDiagnosticPort) < 0)
    {
        DirtyCastLogPrintf(("gameserver: error creating pid file -- exiting\n"));
        GameServerDestroy(pGameServer);
        return(NULL);
    }

    // get debuglevel from config file
    pGameServer->Config.uDebugLevel = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.debuglevel", 1);

    // load voipserver address and port from config file
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.voipserveraddr", pGameServer->Config.strVoipServerAddr, sizeof(pGameServer->Config.strVoipServerAddr), "127.0.0.1");

    pGameServer->Config.uVoipServerPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.voipserverport", 0);
    pGameServer->Config.uVoipServTime = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.voipservertimeout", 40) * 1000;

    // get lobbyserver parameters from config file
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.lobbyname", pGameServer->Config.strServer, sizeof(pGameServer->Config.strServer), "pecan.online.ea.com");
    pGameServer->Config.bSecure = (uint8_t)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.secure", TRUE);
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.environment", pGameServer->Config.strEnvironment, sizeof(pGameServer->Config.strEnvironment), "prod");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.pingsite", pGameServer->Config.strPingSite, sizeof(pGameServer->Config.strPingSite), "");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.rdirname", pGameServer->Config.strRdirName, sizeof(pGameServer->Config.strRdirName), "");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.gameprotocolversion", pGameServer->Config.strGameProtocolVersion, sizeof(pGameServer->Config.strGameProtocolVersion), "");

    // get redundancy settings
    pGameServer->Config.iRedundancyPeriod = (signed)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.redundancyperiod", 5 * 1000);
    pGameServer->Config.uRedundancyThreshold = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.redundancythreshold", 1000);
    pGameServer->Config.uRedundancyLimit = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.redundancylimit", 64);
    pGameServer->Config.iRedundancyTimeout = (signed)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.redundancytimeout", 60);

    // get the maximum number of clients supported
    pGameServer->Config.uMaxClients = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.maxclients", 0);
    //alias
    pGameServer->Config.uMaxClients = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.maxgamesize", pGameServer->Config.uMaxClients);
    // clamp to max supported players
    if (pGameServer->Config.uMaxClients > GAMESERVER_MAXPLAYERS)
    {
        DirtyCastLogPrintf("gameserver: configured gameserver.maxclients or gameserver.maxgamesize is too large - clamping %d to %d\n", pGameServer->Config.uMaxClients, GAMESERVER_MAXPLAYERS);
        pGameServer->Config.uMaxClients = GAMESERVER_MAXPLAYERS;
    }

    // get the maximum number of spectators supported
    pGameServer->Config.uMaxSpectators = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.maxspectators", 0);

    // clamp to max supported players
    if ((pGameServer->Config.uMaxClients + pGameServer->Config.uMaxSpectators) > GAMESERVER_MAXPLAYERS)
    {
        DirtyCastLogPrintf("gameserver: configured gameserver.maxspectators plus gameserver.maxclients or gameserver.maxgamesize is too large - clamping spectators from %d to %d\n", pGameServer->Config.uMaxSpectators, GAMESERVER_MAXPLAYERS - pGameServer->Config.uMaxClients);
        pGameServer->Config.uMaxSpectators = GAMESERVER_MAXPLAYERS - pGameServer->Config.uMaxClients;
    }

    // get update rate
    pGameServer->Config.uFixedUpdateRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.fixedrate", 33);

    // get nucleus cert configurations
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.certfilename", pGameServer->Config.strCertFile, sizeof(pGameServer->Config.strCertFile), "");
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.keyfilename", pGameServer->Config.strKeyFile, sizeof(pGameServer->Config.strKeyFile), "");

    // initialize fixed update timer
    pGameServer->uLastFixedUpdateTime = 0;
    pGameServer->uLastTokenRetrieve = 0;

    // initialize latency and redundancy update timer
    pGameServer->uLastLateUpdateTime = pGameServer->uLastRedundancyUpdateTime = NetTick();

    // metrics aggregator settings / metrics settings
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.metricsaggregator.addr", pGameServer->Config.strMetricsAggregatorHost, sizeof(pGameServer->Config.strMetricsAggregatorHost), "127.0.0.1");
    pGameServer->Config.uMetricsAggregatorPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.metricsaggregator.port", 0);
    pGameServer->Config.eMetricsFormat = (ServerMetricsFormatE)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.metricsaggregator.format", SERVERMETRICS_FORMAT_DATADOG);
    pGameServer->Config.uMetricsPushingRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.metricsaggregator.rate", 1000);

    /* The "max accumulation cycles" config parameter is used to limit the number of times a unique metric handler can "accumulate" metric values instead of
       populating it immediately in the metrics datagram. This mechanism allows for later packing of multiple values on a single line in the metrics datagram
       resulting in an optimized datagram payload and a decreased reporting frequency. 
        Example of payload without accumulation (max accumulation cycles = 1):
            gameserver.idpps:30|ms|#mode:gameserver,host:ec2-3-89-36-64.compute-1.amazonaws.com,site:aws-iad,product:dirtycast-testbed-ps4,env:test
            gameserver.idpps:30|ms|#mode:gameserver,host:ec2-3-89-36-64.compute-1.amazonaws.com,site:aws-iad,product:dirtycast-testbed-ps4,env:test
        Example of payload with accumulation (max accumulation cycles > 1): 
            gameserver.idpps:30,30,31,30|ms|#mode:gameserver,host:ec2-3-89-36-64.compute-1.amazonaws.com,site:aws-iad,product:dirtycast-testbed-ps4,env:test

        Warning: Some metric handler accumulates more than one value per accumulation cycle. For instance, _GameServerOiMetricsIdpps() can accumulate up to 
                 22 values (one for each players in the game) in a single cycle. Consequently, every cycle is growing the accumulator with 22 more entries.
                 It is therefore important to keep maxaccumulationcycles small enough (i.e. < 12) to avoid creating metric entries that can potentially
                 not fit in a single metric datagram. 

        Possible values:
        0 --> Feature Disabled: multi-value lines not used in metrics payload
        N --> Feature Enabled: multi-value lines used with N accumulation cycles */
    pGameServer->Config.uMetricsMaxAccumulationCycles = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.metricsaggregator.maxaccumulationcycles", 0);

    // create the oi metrics module
    if (pGameServer->Config.uMetricsAggregatorPort != 0)
    {
        if ((pGameServer->pOiMetricsRef = GameServerOiMetricsCreate(pGameServer)) != NULL)
        {
            DirtyCastLogPrintf("gameserver: created oi metrics module, reporting to aggregator port: %d\n", pGameServer->Config.uMetricsAggregatorPort);
            GameServerOiMetricsControl(pGameServer->pOiMetricsRef, 'accl', pGameServer->Config.uMetricsMaxAccumulationCycles, NULL, 0);
        }
        else
        {
            DirtyCastLogPrintf("gameserver: unable to create oi metrics module\n");
            return(NULL);
        }
    }

    // get subspace sidekick app settings
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.subspace.sidekick.addr", pGameServer->Config.strSubspaceSidekickAddr, sizeof(pGameServer->Config.strSubspaceSidekickAddr), "127.0.0.1");
    pGameServer->Config.uSubspaceSidekickPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.subspace.sidekick.port", 0);        // when port is 0, subspace sidekick is disabled
    pGameServer->Config.uSubspaceProvisionRateMin = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.subspace.provisionmin", 60) * 60000;  // the minimum number of minutes between subspace provisions
    pGameServer->Config.uSubspaceProvisionRateMax = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.subspace.provisionmax", 30) * 1000;  // the required number of seconds between subspace provisions

    /* get any custom attributes that we need to associate our gameservers with when creating the game with blaze
       the format expected is as follows gameserver.customattributes=key1:value1,key2:value2, etc */
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "gameserver.customattributes", strCustomAttributes, sizeof(strCustomAttributes), "");
    for (uCustomAttr = 0; uCustomAttr < DIRTYCAST_CalculateArraySize(pGameServer->Config.aCustomAttributes); uCustomAttr += 1)
    {
        char strCustomAttribute[512];
        const char *pCustomAttribute = strCustomAttribute;
        GameServerCustomAttrT *pCurrentAttr = &pGameServer->Config.aCustomAttributes[uCustomAttr];

        // get the sequence which is comma delimited
        if (ds_strsplit(pCustomAttributes, ',', strCustomAttribute, sizeof(strCustomAttribute), &pCustomAttributes) == 0)
        {
            break;
        }

        // split the sequence which is delimited by a colon
        if (ds_strsplit(pCustomAttribute, ':', pCurrentAttr->strKey, sizeof(pCurrentAttr->strKey), &pCustomAttribute) == 0)
        {
            break;
        }
        if (ds_strsplit(pCustomAttribute, ':', pCurrentAttr->strValue, sizeof(pCurrentAttr->strValue), &pCustomAttribute) == 0)
        {
            break;
        }
    }
    pGameServer->Config.uNumAttributes = (uint8_t)uCustomAttr;

    // create gameserver lobby module and initialize state
    if (GameServerBlazeCreate(&pGameServer->Config, pCommandTags, pConfigTags) < 0) 
    {
        DirtyCastLogPrintf("gameserver: unable to create gameserverblaze\n");
        GameServerDestroy(pGameServer);
        return(NULL);
    }
    GameServerBlazeCallback(&_GameServerEventCallback, _GameServerBroadcast, _GameServerDisconnect, _GameServerLinkEvent, pGameServer);

    // create module to listen for diagnostic requests, web page
    if (pGameServer->Config.uDiagnosticPort != 0)
    {
        if ((pGameServer->pServerDiagnostic = ServerDiagnosticCreate(pGameServer->Config.uDiagnosticPort, GameServerDiagnosticCallback, pGameServer, "GameServer", 8*1024, "gameserver.", pCommandTags, pConfigTags)) != NULL)
        {
            DirtyCastLogPrintf("gameserver: created diagnostic socket bound to port %d\n", pGameServer->Config.uDiagnosticPort);
        }
        else
        {
            DirtyCastLogPrintf("gameserver: unable to create serverdiagnostic module\n");
            return(NULL);
        }
    }

    // get maximum packet size that can be tunneled
    if ((iMaxPacket = ProtoTunnelStatus(NULL, 'maxp', 0, NULL, 0)) < 0)
    {
        // hard-code a safe max packet limit for any DirtySDK version that doesn't support the prototunnel 'maxp' selector
        iMaxPacket = 1236;
    }
    // limit maximum packet size (in case we have tunneling enabled on the VoipServer)
    NetConnControl('maxp', iMaxPacket, 0, NULL, NULL);

    // done
    DirtyCastLogPrintf("gameserver: server initialized\n");
    return(pGameServer);
}

/*F********************************************************************************/
/*!
    \Function GameServerDestroy

    \Description
        Shutdown the GameServer.

    \Input *pGameServer - module state

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void GameServerDestroy(GameServerRefT *pGameServer)
{
    DirtyCastLogPrintf("gameserver: shutting down\n");

    GameServerBlazeRelease();

    if (pGameServer->pServerDiagnostic != NULL)
    {
        ServerDiagnosticDestroy(pGameServer->pServerDiagnostic);
        pGameServer->pServerDiagnostic = NULL;
    }
    if (pGameServer->pOiMetricsRef != NULL)
    {
        GameServerOiMetricsDestroy(pGameServer->pOiMetricsRef);
        pGameServer->pOiMetricsRef = NULL;
    }
    if (pGameServer->pVoipServerSocket != NULL)
    {
        BufferedSocketClose(pGameServer->pVoipServerSocket);
        pGameServer->pVoipServerSocket = NULL;
    }
    if (pGameServer->pConfig != NULL)
    {
        ConfigFileDestroy(pGameServer->pConfig);
        pGameServer->pConfig = NULL;
    }
    // remove pid file
    DirtyCastPidFileRemove(pGameServer->strPidFileName);
    DirtyMemFree(pGameServer, GAMESERVER_MEMID, pGameServer->iMemGroup, pGameServer->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function GameServerUpdate

    \Description
        Main server process loop.

    \Input *pGameServer     - module state
    \Input uShutdownFlags   - GAMESERVER_SHUTDOWN_* flags indicating shutdown state

    \Output
        uint8_t             - TRUE to continue processing, FALSE to exit

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
uint8_t GameServerUpdate(GameServerRefT *pGameServer, uint32_t uShutdownFlags)
{
    int32_t iNextUpdateTime;
    uint32_t uCurTick;
    volatile uint32_t uShutdownFlags0 = uShutdownFlags;

    // give some time to registered idle functions
    NetConnIdle();

    // get current tick count
    uCurTick = NetTick();

    // get next update time
    if ((iNextUpdateTime = _GameServerNextFixedUpdateTime(pGameServer, uCurTick)) > 0)
    {
        // update server diagnostic processing
        if (pGameServer->pServerDiagnostic != NULL)
        {
            ServerDiagnosticUpdate(pGameServer->pServerDiagnostic);
        }
        
        // give time for metrics system to send to the aggregator
        if (pGameServer->pOiMetricsRef != NULL)
        {
            GameServerOiMetricsUpdate(pGameServer->pOiMetricsRef);
        }

        // update signal flags (may be set by diagnostic action)
        if (pGameServer->uSignalFlags != 0)
        {
            uShutdownFlags0 = uShutdownFlags = pGameServer->uSignalFlags;
        }

        // update voip server connection
        _GameServerUpdateVoipServer(pGameServer);

        // update lobby server connection
        _GameServerUpdateBlazeServer(pGameServer, uCurTick);

        // update gamelink connections
        if (GameServerBlazeGetGameServerLink() != NULL)
        {
            GameServerStatInfoT *pStat;
            if ((pStat = GameServerLinkUpdate(GameServerBlazeGetGameServerLink())) != NULL)
            {
                GameServerPacketT GamePacket;
                uint32_t uSize;
                // create packet to send to voipserver
                uSize = GameServerPacketEncode(&GamePacket, 'stat', 0, pStat, sizeof(*pStat));
                // send the packet
                _GameServerSendPacket(pGameServer, &GamePacket, uSize);

                // high frequency update (disabled by default)
                #if 0
                GameServerPacketLogLateStats(&pLate->E2eLate, "gameserver: [e2e]", "    ");
                GameServerPacketLogLateStats(&pLate->InbLate, "[inb]", "\n");
                #endif

                // accumulate e2d and inbound latency
                GameServerPacketUpdateLateStats(&pGameServer->StatInfo.E2eLate, &pStat->E2eLate);
                GameServerPacketUpdateLateStats(&pGameServer->StatInfo.InbLate, &pStat->InbLate);
            }
            if ((signed)(uCurTick - pGameServer->uLastLateUpdateTime) > 60 * 1000)
            {
                // output accumulated latency info, if we got any samples
                if ((pGameServer->StatInfo.E2eLate.uNumStat != 0) || (pGameServer->StatInfo.InbLate.uNumStat != 0))
                {
                    GameServerPacketLogLateStats(&pGameServer->StatInfo.E2eLate, "gameserver: [e2e]", "\n");
                    GameServerPacketLogLateStats(&pGameServer->StatInfo.InbLate, "gameserver: [inb]", "\n");
                    // reset latency accumulators
                    GameServerPacketUpdateLateStats(&pGameServer->StatInfo.E2eLate, NULL);
                    GameServerPacketUpdateLateStats(&pGameServer->StatInfo.InbLate, NULL);
                    // output per-client latency info
                    GameServerLinkLogClientLatencies(GameServerBlazeGetGameServerLink());
                }
                // reset timer
                pGameServer->uLastLateUpdateTime = uCurTick;
            }

            if ((signed)(uCurTick - pGameServer->uLastRedundancyUpdateTime) > pGameServer->Config.iRedundancyPeriod)
            {
                GameServerLinkUpdateRedundancy(GameServerBlazeGetGameServerLink());
                // reset timer
                pGameServer->uLastRedundancyUpdateTime = uCurTick;
            }
        }

        // flush any pending voipserver writes
        if (pGameServer->pVoipServerSocket != NULL)
        {
            BufferedSocketFlush(pGameServer->pVoipServerSocket);
        }

        // block waiting on input
        NetConnControl('poll', iNextUpdateTime, 0, NULL, NULL);
    }
    else // fixed-rate update
    {
        // if we have a gamelink ref do the fixed-rate update
        if (GameServerBlazeGetGameServerLink() != NULL)
        {
            GameServerLinkUpdateFixed(GameServerBlazeGetGameServerLink());
        }
        // update last fixed update time
        pGameServer->uLastFixedUpdateTime += _GameServerGetFixedUpdateRate(pGameServer);
        _GameServerDebugFixedRate(pGameServer);

        // flush any pending voipserver writes
        if (pGameServer->pVoipServerSocket != NULL)
        {
            BufferedSocketFlush(pGameServer->pVoipServerSocket);
        }
    }

    // determine shutdown status
    if (uShutdownFlags & GAMESERVER_SHUTDOWN_IMMEDIATE)
    {
        DirtyCastLogPrintf("gameserver: executing immediate shutdown (0x%08x, 0x%08x)\n", uShutdownFlags, uShutdownFlags0);
        return(FALSE);
    }

    uint8_t bCanShutDown = FALSE;
    if (GameServerBlazeStatus('nusr', 0, NULL, 0) == 0)
    {
        bCanShutDown = TRUE;
    }

    if ((uShutdownFlags & GAMESERVER_SHUTDOWN_IFEMPTY) && (bCanShutDown))
    {
        DirtyCastLogPrintf("gameserver: executing ifempty shutdown\n");
        return(FALSE);
    }
    // continue processing
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function GameServerStatus

    \Description
        Get module status

    \Input *pGameServer     - pointer to module state
    \Input iSelect          - status selector
    \Input iValue           - selector specific
    \Input *pBuf            - [out] - selector specific
    \Input iBufSize         - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'addr' - returns our local addr
            'conf' - returns Config pointer in pBuf
            'gsta' - returns game started bool
            'logn' - returns the count of login history with login history being copied via pBuf
            'rate' - returns fixed update rate
            'star' - returns the start time of the server
            'vsta' - returns the voip server state
        \endverbatim

    \Version 04/05/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t GameServerStatus(GameServerRefT *pGameServer, int32_t iStatus, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iStatus == 'addr')
    {
        return(pGameServer->uFrontAddr);
    }
    if (iStatus == 'conf')
    {
        if (pBuf != NULL)
        {
            *(GameServerConfigT **)pBuf = &pGameServer->Config;
            return(0);
        }
        return(-1);
    }
    if (iStatus == 'gsta')
    {
        return(pGameServer->bGameStarted);
    }
    if (iStatus == 'logn')
    {
        if (pBuf != NULL)
        {
            ds_memcpy(pBuf, pGameServer->aLoginHistory, iBufSize);
        }
        return(pGameServer->iLoginHistoryCt);
    }
    if (iStatus == 'rate')
    {
        return(_GameServerGetFixedUpdateRate(pGameServer));
    }
    if (iStatus == 'star')
    {
        return(pGameServer->uStartTime);
    }
    if (iStatus == 'vsta')
    {
        return(pGameServer->eVoipServerState);
    }

    // fallthrough to buffersocket or treat as unhandled
    return(pGameServer->pVoipServerSocket != NULL ? BufferedSocketInfo(pGameServer->pVoipServerSocket, iStatus, iValue, pBuf, iBufSize) : -1);
}

/*F********************************************************************************/
/*!
    \Function GameServerControl

    \Description
        Module control function

    \Input *pGameServer     - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'sgnl' - sets the cached signal flags
            'spam' - sets the debug level
        \endverbatim

    \Version 04/05/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t GameServerControl(GameServerRefT *pGameServer, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'sgnl')
    {
        pGameServer->uSignalFlags = (uint32_t)iValue;
        return(0);
    }
    if (iControl == 'spam')
    {
        pGameServer->Config.uDebugLevel = (uint32_t)iValue;

        // set oi metrics module spam level
        if (pGameServer->pOiMetricsRef != NULL)
        {
            GameServerOiMetricsControl(pGameServer->pOiMetricsRef, 'spam', iValue, NULL, 0);
        }

        // fallthrough to lower modules
        SocketControl(NULL, iControl, iValue, NULL, NULL);
        return(0);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function GameServerGetConfig

    \Description
        Get the pointer to the configuration module

    \Input *pGameServer     - module state

    \Output
        GameServerConfigT * - configuration module pointer

    \Version 04/05/2017 (eesponda)
*/
/********************************************************************************F*/
const GameServerConfigT *GameServerGetConfig(GameServerRefT *pGameServer)
{
    return(&pGameServer->Config);
}
