/*H********************************************************************************/
/*!
    \File voipgameserver.c

    \Description
        This module implements the main logic for the VoipServer when
        acting as a dedicated server for Blaze

    \Copyright
        Copyright (c) 2006-2015 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>


#if defined(DIRTYCODE_LINUX)
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/times.h>
#endif

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "LegacyDirtySDK/util/tagfield.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/voip/voiptunnel.h"
#include "DirtySDK/dirtyvers.h"

#include "tokenapi.h"
#include "configfile.h"
#include "serverdiagnostic.h"
#include "voipserver.h"
#include "voipserverconfig.h"
#include "voipservercomm.h"
#include "gameserverdiagnostic.h"
#include "udpmetrics.h"
#include "tcpmetrics.h"
#include "voipgameserver.h"

/*** Defines **********************************************************************/

#define VOIPGAMESERVER_MEMID                 ('vgsv')
#define VOIPGAMESERVER_TOKEN_RETRY_TIME      (30*1000)  // 30 sec
#define VOIPGAMESERVER_STUCK_DETECTION_DELAY (5)        // 5 min

//! how often we query TCP metrics
#define VOIPGAMESERVER_TCPMETRICS_RATE       (1000)

/*** Type Definitions *************************************************************/

//! module state
struct VoipGameServerRefT
{
    //! base voipserver module
    VoipServerRefT *pBase;

    //! socket to listen for incoming gameserver connections on
    SocketT *pListenSocket;

    //! netlink socket fd (for querying buffer stats of all vs-to-gs tcp sockets)
    int32_t iTcpMetricsNetlinkSocket;

    //! last time we updated TCP metrics
    uint32_t uLastTcpMetricsUpdate;

    // gameserver info
    GameServerT *pGameServers;  //!< list of gameservers
    int32_t iNumGameServers;    //!< gameserver count
    int32_t iProcGameServer;    //!< first gameserver in list to process

    // metrics update tracking info
    uint32_t uLastMetricsUpdate;//!< last time metrics accumulators were updated
    uint32_t uLastMetricsReset; //!< last time metrics accumulators were reset
    uint8_t bResetMetrics;      //!< if true, clear metrics accumulators
    uint8_t _pad[3];

    uint32_t uLastUpdateAvail;  //!< last time _VoipServerUpdateAvail() was executed

    //! memgroup
    int32_t iMemGroup;
    void *pMemGroupUserdata;

    TokenApiAccessDataT TokenData;//!< acquired access token data

    //! voipgameserver metrics
    VoipGameServerStatT Stats;

#if DIRTYCODE_DEBUG
    uint8_t bDbgActiveState;      //!< if true, the GS's should claim to be in an active state, for debug purposes
#endif

    //! common blaze service name associated with the gameservers connected to this voipserver
    char strProductId[GAMESERVER_MAX_PRODUCTID_LENGTH];
};

typedef void (VoipServerDispatchFuncT)(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);

typedef struct VoipServerDispatchT
{
    uint32_t uKind;
    VoipServerDispatchFuncT *pDispatch;
} VoipServerDispatchT;

/*** Function Prototypes **********************************************************/

static void _VoipServerDispatch_ngam(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_dgam(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_nusr(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_dusr(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_clst(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_conf(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_data(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_ping(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_stat(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_tokn(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);
static void _VoipServerDispatch_null(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick);

static void _VoipServerGameServerSendStats(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, int32_t iGameServer, GameServerPacketT *pGamePacket, uint32_t uCurTick);

/*** Variables ********************************************************************/

static const VoipServerDispatchT _VoipServer_aDispatch[] =
{
    { 'data', _VoipServerDispatch_data },
    { 'stat', _VoipServerDispatch_stat },
    { 'ping', _VoipServerDispatch_ping },
    { 'clst', _VoipServerDispatch_clst },
    { 'conf', _VoipServerDispatch_conf },
    { 'nusr', _VoipServerDispatch_nusr },
    { 'dusr', _VoipServerDispatch_dusr },
    { 'ngam', _VoipServerDispatch_ngam },
    { 'dgam', _VoipServerDispatch_dgam },
    { 'tokn', _VoipServerDispatch_tokn },
    { 0     , _VoipServerDispatch_null }
};

static GameServerStatsT _VoipServer_EmptyStats;

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipServerVoipTunnelEventCallback

    \Description
        ProtoTunnel event callback handler.

    \Input *pVoipTunnel     - voiptunnel module state
    \Input *pEventData      - event data
    \Input *pUserData       - user callback ref (voipserver module state)

    \Notes
        The code in this file is only used with the OTP operational mode. In that mode,
        we can assume that a client is only involved in a single game at a time. Therefore,
        with DirtySDK 15.1.7.0.1 and beyond, we assume only the first entry of the
        aGameMappings[] array is used.

    \Version 03/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerVoipTunnelEventCallback(VoipTunnelRefT *pVoipTunnel, const VoipTunnelEventDataT *pEventData, void *pUserData)
{
    VoipGameServerRefT *pVoipServer = (VoipGameServerRefT *)pUserData;
    GameServerProtoStatT *pGameServerStat;
    #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
    GameServerT *pGameServer = (pEventData->pClient != NULL) ? &pVoipServer->pGameServers[pEventData->pClient->iGameIdx] : NULL;
    #else
    GameServerT *pGameServer = (pEventData->pClient != NULL) ? &pVoipServer->pGameServers[pEventData->iGameIdx] : NULL;
    #endif
    uint32_t uNetworkBytes;
    ProtoTunnelRefT *pProtoTunnel = VoipServerGetProtoTunnel(pVoipServer->pBase);

    // ref gameserver stats structure for this client
    pGameServerStat = (pGameServer != NULL) ? &pGameServer->GameStats.VoipStat : NULL;

    // del client event
    if (pEventData->eEvent == VOIPTUNNEL_EVENT_DELCLIENT)
    {
        // if tunneling, clean up prototunnel tunnel
        VoipTunnelClientT *pClient = (VoipTunnelClientT *)pEventData->pClient;
        if (pClient->uTunnelId != 0)
        {
            ProtoTunnelFree(pProtoTunnel, pClient->uTunnelId, NULL);
            pClient->uTunnelId = 0;
        }
        else
        {
            DirtyCastLogPrintf("voipgameserver: voiptunnel delclient event for clientId=0x%08x with no prototunnel tunnel allocated\n", pEventData->pClient->uClientId);
        }
    }

    // accumulate recv stats
    if (pEventData->eEvent == VOIPTUNNEL_EVENT_RECVVOICE)
    {
        uNetworkBytes = VOIPSERVER_EstimateNetworkPacketSize(pEventData->iDataSize);
        pGameServerStat->uByteRecv += uNetworkBytes;
        pGameServerStat->uPktsRecv += 1;

        // clear "have not received any data" tracker
        pVoipServer->Stats.iGamesFailedToRecv = 0;
    }
    // accumulate send stats
    if (pEventData->eEvent == VOIPTUNNEL_EVENT_SENDVOICE)
    {
        uNetworkBytes = VOIPSERVER_EstimateNetworkPacketSize(pEventData->iDataSize);
        pGameServerStat->uByteSent += uNetworkBytes;
        pGameServerStat->uPktsSent += 1;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerAddClientRemovalReason

    \Description
        Add a client removal event to tracking

    \Input *pVoipServer     - voipserver state
    \Input uRemovalReason   - removal reason code
    \Input uGameState       - GAMESERVER_GAMESTATE_*
    \Input *pStrRemovalReason - string version of removal reason
    \Input uCount           - number of clients

    \Version 09/30/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerAddClientRemovalReason(VoipGameServerRefT *pVoipServer, uint32_t uRemovalReason, uint32_t uGameState, const char *pStrRemovalReason, uint32_t uCount)
{
    GameServerClientRemovalInfoT *pRemovalInfo;
    uint32_t uIndex;

    // make sure we have a count
    if (uCount == 0)
    {
        return;
    }

    // validate check game state
    if (uGameState >= GAMESERVER_NUMGAMESTATES)
    {
        NetPrintf(("voipgameserver: game state %d in client removal notification is invalid\n", uGameState));
        return;
    }

    // search through buffer to see if we already have an event of this type
    for (uIndex = 0; uIndex < VOIPSERVER_MAXCLIENTREMOVALINFO; uIndex += 1)
    {
        pRemovalInfo = &pVoipServer->Stats.aClientRemovalInfo[uIndex];
        if (pRemovalInfo->RemovalReason.uRemovalReason == uRemovalReason)
        {
            pRemovalInfo->aRemovalStateCt[uGameState] += uCount;
            return;
        }
    }

    // if we get here, we don't already have an event of this type, so find an empty slot and add it
    for (uIndex = 0; uIndex < VOIPSERVER_MAXCLIENTREMOVALINFO; uIndex += 1)
    {
        pRemovalInfo = &pVoipServer->Stats.aClientRemovalInfo[uIndex];
        if (pRemovalInfo->RemovalReason.strRemovalReason[0] == '\0')
        {
            ds_strnzcpy(pRemovalInfo->RemovalReason.strRemovalReason, pStrRemovalReason, sizeof(pRemovalInfo->RemovalReason.strRemovalReason));
            pRemovalInfo->RemovalReason.uRemovalReason = uRemovalReason;
            pRemovalInfo->aRemovalStateCt[uGameState] += uCount;
            DirtyCastLogPrintf("voipgameserver: added client removal event %s/0x%08x to tracking\n", pStrRemovalReason, uRemovalReason);
            return;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateAvail

    \Description
        Update gameserver availability stats

    \Input *pVoipServer     - module state

    \Version 09/07/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateAvail(VoipGameServerRefT *pVoipServer)
{
    GameServerT *pGameServer;
    int32_t iGameServer;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer->pBase);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);
    
    // clear out current stats before re-calculating them
    pVoipServer->Stats.iGamesOffline = 0;
    pVoipServer->Stats.iGamesActive = 0;
    pVoipServer->Stats.iGamesZombie = 0;
    pVoipServer->Stats.iGamesAvailable = 0;
    pVoipServer->Stats.iGamesStuck = 0; 

    for (iGameServer = 0; iGameServer < pConfig->iMaxGameServers; iGameServer++)
    {
        pGameServer = &pVoipServer->pGameServers[iGameServer];
        if (pGameServer->bConnected == FALSE)
        {
            continue;
        }
        if (pGameServer->eLobbyState == kOffline)
        {
            pVoipServer->Stats.iGamesOffline += 1;
        }
        else if (pGameServer->eLobbyState == kAvailable)
        {
            pVoipServer->Stats.iGamesAvailable += 1;
        }
        else if (pGameServer->eLobbyState == kActive)
        {
            pVoipServer->Stats.iGamesActive += 1;
        }
        else if (pGameServer->eLobbyState == kZombie)
        {
            pVoipServer->Stats.iGamesZombie += 1;
        }

        // determine if a game is "stuck" or not
        if (pGameServer->bGameActive) // note eLobbyState == kActive is a bit different, the GS can fake eLobbyState for debugging
        {
            uint32_t uNewActvClientsCnt = VoipTunnelStatus(pVoipTunnel, 'nusr', iGameServer, NULL, 0);

            // is the game empty?
            if (uNewActvClientsCnt == 0)
            {
                // was the game empty in the previous iteration
                if (pGameServer->uLastActvClientsCnt == 0)
                {
                    // here NetTickDiff() is used to find the delta between two times in seconds, we then divide by 60 to get minutes
                    int32_t iElapsedTimeInMinutes = NetTickDiff(ds_timeinsecs(), pGameServer->uStuckDetectionStartTime) / 60;
                    // consider a game stuck if there has been no player for more than VOIPGAMESERVER_STUCK_DETECTION_DELAY
                    if (iElapsedTimeInMinutes > VOIPGAMESERVER_STUCK_DETECTION_DELAY)
                    {
                        // warn if game is newly stuck
                        if (pGameServer->bGameStuck == FALSE)
                        {
                            DirtyCastLogPrintf("voipgameserver: [gs%03d] stuck game detected!\n", iGameServer);
                            pGameServer->bGameStuck = TRUE;
                        }

                        pVoipServer->Stats.iGamesStuck += 1;
                    }
                }
                else
                {
                    pGameServer->uStuckDetectionStartTime = ds_timeinsecs();
                }
            }
            else
            {
                // warn if game was stuck and no longer is
                if (pGameServer->bGameStuck == TRUE)
                {
                    // here NetTickDiff() is used to find the delta between two times in seconds, we then divide by 60 to get minutes
                    int32_t iElapsedTimeInMinutes = NetTickDiff(ds_timeinsecs(), pGameServer->uStuckDetectionStartTime) / 60;
                    DirtyCastLogPrintf("voipgameserver: [gs%03d] game no longer stuck (client count > 0) - remained stuck for %d min\n", iGameServer, iElapsedTimeInMinutes);
                    pGameServer->bGameStuck = FALSE;
                }
            }

            pGameServer->uLastActvClientsCnt = uNewActvClientsCnt;
        }
    }

    pVoipServer->uLastUpdateAvail = NetTick();
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDeleteGame

    \Description
        Delete a GameServer game and do any required cleanup.

    \Input *pVoipServer     - voipserver ref
    \Input *pGameServer     - gameserver ref
    \Input iGameServer      - gameserver index
    \Input uRlmtChangeCount - count of redundancy limit changes (across all clients during that game)
    \Input uCurTick         - millisecond tick count

    \Version 04/12/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDeleteGame(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, int32_t iGameServer, uint32_t uRlmtChangeCount, uint32_t uCurTick)
{
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer->pBase);

    // delete all clients associated with this gameserver and subtract from overall count
    VoipTunnelGameListDel(pVoipTunnel, iGameServer);

    // do final stat update and clear stat structure
    GameServerUpdateGameStats(&pGameServer->GameStats, &pGameServer->GameStatsCur, &pGameServer->GameStatsSum, &pGameServer->GameStatsMax, FALSE);

    // track if no packets were received for this game
    if ((pGameServer->GameStatsSum.GameStat.uPktsRecv == 0) && (pGameServer->GameStatsSum.VoipStat.uPktsRecv == 0))
    {
        pVoipServer->Stats.iGamesFailedToRecv += 1;
    }

    // update history
    pGameServer->uTotalGames += 1;
    pGameServer->GameStatHistory[0].uGameEnd = time(NULL);
    pGameServer->uTotalRlmtChangeCount += uRlmtChangeCount;
    pGameServer->GameStatHistory[0].uRlmtChangeCount = uRlmtChangeCount;
    ds_memcpy(&pGameServer->GameStatHistory[0].GameStatsSum, &pGameServer->GameStatsSum, sizeof(pGameServer->GameStatsSum));
    ds_memcpy(&pGameServer->GameStatHistory[0].GameStatsMax, &pGameServer->GameStatsMax, sizeof(pGameServer->GameStatsMax));
    // clear accumulated stat structures
    ds_memclr(&pGameServer->GameStatsSum, sizeof(pGameServer->GameStatsSum));
    ds_memclr(&pGameServer->GameStatsMax, sizeof(pGameServer->GameStatsMax));
    ds_memclr(&pGameServer->PackLostSent, sizeof(pGameServer->PackLostSent));

    // clear active/stuck status
    pGameServer->bGameActive = FALSE;

    // warn if game was stuck and no longer is
    if (pGameServer->bGameStuck == TRUE)
    {
        // here NetTickDiff() is used to find the delta between two times in seconds, we then divide by 60 to get minutes
        int32_t iElapsedTimeInMinutes = NetTickDiff(ds_timeinsecs(), pGameServer->uStuckDetectionStartTime) / 60;
        DirtyCastLogPrintf("voipgameserver: [gs%03d] game no longer stuck (game deleted) - remained stuck for %d min\n", iGameServer, iElapsedTimeInMinutes);
        pGameServer->bGameStuck = FALSE;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerTokenAcquisitionCb

    \Description
        Callback that is fired when the access token request is complete

    \Input *pData       - the response information
    \Input *pUserData   - contains our module ref

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _VoipServerTokenAcquisitionCb(const TokenApiAccessDataT *pData, void *pUserData)
{
    VoipGameServerRefT *pVoipServer = (VoipGameServerRefT *)pUserData;

    if (pData != NULL)
    {
        // copy the new data
        ds_memcpy(&pVoipServer->TokenData, pData, sizeof(pVoipServer->TokenData));
    }
    else
    {
        // failure, reset the data
        ds_memclr(&pVoipServer->TokenData, sizeof(pVoipServer->TokenData));
        pVoipServer->TokenData.iExpiresIn = NetTick() + VOIPGAMESERVER_TOKEN_RETRY_TIME; // wait another 30s
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_ngam

    \Description
        Handle 'ngam' (new game) GameServer command.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_ngam(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    int32_t iGameServer = pGameServer - pVoipServer->pGameServers;
    int32_t iGameIdent;
    int32_t iResult;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer->pBase);

    // make sure there's not a game active
    if (pGameServer->bGameActive == TRUE)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] warning, got 'ngam' when a game is active\n", pGameServer - pVoipServer->pGameServers);
        return;
    }
    iGameIdent = SocketNtohl(pGameServer->PacketData.Packet.NewGame.iIdent);

    DirtyCastLogPrintf("voipgameserver: [gs%03d] processing 'ngam' command IDENT=%d UUID=%s\n", iGameServer,
        iGameIdent, pGameServer->PacketData.Packet.NewGame.strGameUUID);
    ds_strnzcpy(pGameServer->strGameUUID, pGameServer->PacketData.Packet.NewGame.strGameUUID, sizeof(pGameServer->strGameUUID));

    // let VoipTunnel know about game
    if ((iResult = VoipTunnelGameListAdd(pVoipTunnel, iGameServer)) < 0)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] could not add game to voiptunnel (error %d)\n", iGameServer, iResult);
        return;
    }

    // make room in gamehistory array for new entry (lose last entry)
    memmove(&pGameServer->GameStatHistory[1], &pGameServer->GameStatHistory[0], sizeof(pGameServer->GameStatHistory) - sizeof(pGameServer->GameStatHistory[0]));

    // clear entry
    ds_memclr(&pGameServer->GameStatHistory[0], sizeof(pGameServer->GameStatHistory[0]));

    // save start time
    pGameServer->GameStatHistory[0].uGameStart = time(NULL);

    // set initial state
    pGameServer->bGameActive = TRUE;
    pGameServer->bGameStuck = FALSE;

    pGameServer->uStuckDetectionStartTime = ds_timeinsecs();
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_dgam

    \Description
        Handle 'dgam' (delete game) GameServer command.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_dgam(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    GameServerPacketT GamePacket;
    int32_t iGameServer = pGameServer - pVoipServer->pGameServers;
    uint32_t uGameEndState = SocketNtohl(pGameServer->PacketData.Packet.DelGame.uGameEndState);
    uint32_t uRlmtChangeCount = SocketNtohl(pGameServer->PacketData.Packet.DelGame.uRlmtChangeCount);
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer->pBase);

    // make sure there's a game active
    if (pGameServer->bGameActive == FALSE)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] warning, got 'dgam' when there is no game\n", iGameServer);
        return;
    }
    DirtyCastLogPrintf("voipgameserver: [gs%03d] processing 'dgam' command (state=%d   rlmt=%d)\n", pGameServer - pVoipServer->pGameServers, uGameEndState, uRlmtChangeCount);

    // update tracking
    if (uGameEndState < GAMESERVER_NUMGAMESTATES)
    {
        pVoipServer->Stats.aGameEndStateCt[uGameEndState] += 1;
    }

    // send final stats to gameserver
    _VoipServerGameServerSendStats(pVoipServer, pGameServer, iGameServer, &GamePacket, uCurTick);

    //$$ tmp - update client removal stats - this should be passed along by the GameServer with more correct/detailed information
    _VoipServerAddClientRemovalReason(pVoipServer, 0xffffffff, GAMESERVER_GAMESTATE_POSTGAME, "GAME DELETED", VoipTunnelStatus(pVoipTunnel, 'nusr', iGameServer, NULL, 0));

    // delete the game
    _VoipServerDeleteGame(pVoipServer, pGameServer, iGameServer, uRlmtChangeCount, uCurTick);

    // update overall availability stats
    _VoipServerUpdateAvail(pVoipServer);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_nusr

    \Description
        Handle 'nusr' (new user) GameServer command.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_nusr(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    VoipTunnelClientT *pClient;
    ProtoTunnelInfoT TunnelInfo;
    GameServerPacketT *pPacket = &pGameServer->PacketData;
    int32_t iResult, iClientIdx;
    uint32_t uGameServerId;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer->pBase);
    ProtoTunnelRefT *pProtoTunnel = VoipServerGetProtoTunnel(pVoipServer->pBase);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
    VoipTunnelClientT Client;

    // format client to add
    ds_memclr(&Client, sizeof(Client));
    Client.uClientId = SocketNtohl(pPacket->Packet.NewUser.iClientId);
    Client.iGameIdx = pGameServer - pVoipServer->pGameServers;
    ds_strnzcpy(Client.strTunnelKey, pPacket->Packet.NewUser.strTunnelKey, sizeof(Client.strTunnelKey));
    uGameServerId = (uint32_t)SocketNtohl(pPacket->Packet.NewUser.iGameServerId);
    iClientIdx = SocketNtohl(pPacket->Packet.NewUser.iClientIdx);

    // log client add request
    DirtyCastLogPrintf("voipgameserver: [gs%03d] processing 'nusr' command clientId=0x%08x GSID=0x%08x (%2d) key=%s\n", Client.iGameIdx, Client.uClientId,
        uGameServerId, VoipTunnelStatus(pVoipTunnel, 'nusr', Client.iGameIdx, NULL, 0), Client.strTunnelKey);

    // see if they are already in the list
    if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, Client.uClientId)) != NULL)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] warning -- clientId=0x%08x is already registered by gameserver %d -- deleting user for re-add\n",
            Client.iGameIdx, pClient->uClientId, pClient->iGameIdx);
        VoipTunnelClientListDel(pVoipTunnel, pClient, -1);
    }

    // add client to client list
    if ((iResult = VoipTunnelClientListAdd2(pVoipTunnel, &Client, &pClient, iClientIdx)) != 0)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] 'nusr' command failed to register clientId 0x%08x at index %d (error %d)\n",
            Client.iGameIdx, Client.uClientId, iClientIdx, iResult);
        return;
    }
    #else
    uint32_t uClientId;
    int32_t iGameServer = pGameServer - pVoipServer->pGameServers;

    uGameServerId = (uint32_t)SocketNtohl(pPacket->Packet.NewUser.iGameServerId);
    uClientId = SocketNtohl(pPacket->Packet.NewUser.iClientId);
    iClientIdx = SocketNtohl(pPacket->Packet.NewUser.iClientIdx);

    // log client add request
    DirtyCastLogPrintf("voipgameserver: [gs%03d] processing 'nusr' command clientId=0x%08x GSID=0x%08x (%2d) key=%s\n", iGameServer, uClientId,
        uGameServerId, VoipTunnelStatus(pVoipTunnel, 'nusr', iGameServer, NULL, 0), pPacket->Packet.NewUser.strTunnelKey);

    // see if they are already in the list
    if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId)) != NULL)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] warning -- clientId=0x%08x is already registered by gameserver %d -- deleting user for re-add\n",
            iGameServer, pClient->uClientId, pClient->aGameMappings[0].iGameIdx);
        VoipTunnelClientListDel(pVoipTunnel, pClient, pClient->aGameMappings[0].iGameIdx);
    }

    // add client to client list
    if ((iResult = VoipTunnelClientListAdd(pVoipTunnel, uClientId, iGameServer, pPacket->Packet.NewUser.strTunnelKey, iClientIdx, &pClient)) != 0)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] 'nusr' command failed to register clientId 0x%08x at index %d (error %d)\n",
            iGameServer, uClientId, iClientIdx, iResult);
        return;
    }
    #endif

    // format tunnel information for alloc
    ds_memcpy(&TunnelInfo, &pConfig->TunnelInfo, sizeof(TunnelInfo));
    TunnelInfo.uRemoteClientId = pClient->uClientId;

    // allocate the tunnel
    if ((pClient->uTunnelId = ProtoTunnelAlloc(pProtoTunnel, &TunnelInfo, pPacket->Packet.NewUser.strTunnelKey)) != (unsigned)-1)
    {
        ProtoTunnelControl(pProtoTunnel, 'tcid', pClient->uTunnelId, uGameServerId, NULL);
    }
    else
    {
        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        DirtyCastLogPrintf("voipgameserver: tunnel alloc for clientId=0x%08x failed\n", Client.uClientId);
        #else
        DirtyCastLogPrintf("voipgameserver: tunnel alloc for clientId=0x%08x failed\n", uClientId);
        #endif
    }
}
/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_dusr

    \Description
        Handle 'dusr' (delete user) GameServer command.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_dusr(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    VoipTunnelClientT *pClient;
    int32_t iClientId = SocketNtohl(pGameServer->PacketData.Packet.DelUser.iClientId);
    int32_t iGameServer = pGameServer - pVoipServer->pGameServers;
    uint32_t uRemovalReason = SocketNtohl(pGameServer->PacketData.Packet.DelUser.RemovalReason.uRemovalReason);
    uint32_t uRemovalState = SocketNtohl(pGameServer->PacketData.Packet.DelUser.uGameState);
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer->pBase);

    // log delete request
    DirtyCastLogPrintf("voipgameserver: [gs%03d] processing 'dusr' command clientId=0x%08x reason=0x%08x, (%2d)\n", iGameServer, iClientId, uRemovalReason,
        VoipTunnelStatus(pVoipTunnel, 'nusr', iGameServer, NULL, 0));

    // add delete request to client removal tracker
    _VoipServerAddClientRemovalReason(pVoipServer, uRemovalReason, uRemovalState, pGameServer->PacketData.Packet.DelUser.RemovalReason.strRemovalReason, 1);

    // get client index from client id
    if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, iClientId)) != NULL)
    {
        // make sure delete request is from the same GameServer
        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        if (pClient->iGameIdx == iGameServer)
        {
            // delete the client
            VoipTunnelClientListDel(pVoipTunnel, pClient, -1);
        }
        else
        {
            DirtyCastLogPrintf("voipgameserver: [gs%03d] ignoring delete request that does not match client-registered gameserver %d\n",
                iGameServer, pClient->iGameIdx);
        }
        #else
        if (pClient->aGameMappings[0].iGameIdx == iGameServer)
        {
            // delete the client
            VoipTunnelClientListDel(pVoipTunnel, pClient, pClient->aGameMappings[0].iGameIdx);
        }
        else
        {
            DirtyCastLogPrintf("voipgameserver: [gs%03d] ignoring delete request that does not match client-registered gameserver %d\n",
                iGameServer, pClient->aGameMappings[0].iGameIdx);
        }
        #endif
    }
    else
    {
        DirtyCastLogPrintf("voipgameserver: could not match client 0x%08x for delete\n", iClientId);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_clst

    \Description
        Handle 'clst' (set user client list) GameServer command.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Notes
        The code in this file is only used with the OTP operational mode. In that mode,
        we can assume that a client is only involved in a single game at a time. Therefore,
        with DirtySDK 15.1.7.0.1 and beyond, we assume only the first entry of the
        aGameMappings[] array is used.

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_clst(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    VoipTunnelClientT *pClient;
    GameServerPacketT *pPacket = &pGameServer->PacketData;
    uint32_t uClientId;
    int32_t iClientId, iNumClients;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer->pBase);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    // extract client reference and number of clients in client list
    uClientId = SocketNtohl(pPacket->Packet.SetList.iClientId);
    iNumClients = SocketNtohl(pPacket->Packet.SetList.iNumClients);

    // ref client
    if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId)) != NULL)
    {
        DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 1, "voipgameserver: [gs%03d] processing 'clst' command for clientId=0x%08x (%d clients)\n",
            pGameServer - pVoipServer->pGameServers, pClient->uClientId, iNumClients);
        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        pClient->iNumClients = iNumClients;
        for (iClientId = 0; iClientId < pClient->iNumClients; iClientId++)
        {
            pClient->aClientIds[iClientId] = SocketNtohl(pPacket->Packet.SetList.aClientIds[iClientId]);
            if ((pClient->aClientIds[iClientId] != 0) && (pConfig->uDebugLevel > 1))
            {
                DirtyCastLogPrintf("voipgameserver: [%d] 0x%08x\n", iClientId, pClient->aClientIds[iClientId]);
            }
        }
        VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient);
        DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 2, "voipgameserver: set client send mask to 0x%08x\n", pClient->uGameSendMask);
        #else
        pClient->aGameMappings[0].iNumClients = iNumClients;
        for (iClientId = 0; iClientId < pClient->aGameMappings[0].iNumClients; iClientId++)
        {
            pClient->aGameMappings[0].aClientIds[iClientId] = SocketNtohl(pPacket->Packet.SetList.aClientIds[iClientId]);
            if ((pClient->aGameMappings[0].aClientIds[iClientId] != 0) && (pConfig->uDebugLevel > 1))
            {
                DirtyCastLogPrintf("voipgameserver: [%d] 0x%08x\n", iClientId, pClient->aGameMappings[0].aClientIds[iClientId]);
            }
        }
        VoipTunnelClientRefreshSendMask(pVoipTunnel, pClient, pGameServer - pVoipServer->pGameServers);
        DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 2, "voipgameserver: set client send mask to 0x%08x\n", pClient->aGameMappings[0].uTopologyVoipSendMask);
        #endif
    }
    else
    {
        DirtyCastLogPrintf("voipgameserver: unable to set client list for clientid=0x%08x\n", uClientId);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_conf

    \Description
        Handle 'conf' (configuration) GameServer command.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 03/16/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_conf(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    pGameServer->uDiagnosticPort = SocketNtohs(pGameServer->PacketData.Packet.Config.uDiagPort);

    // if strProductId hasn't been set, or if this is the first game server re-connecting allow setting a new strProductId
    if ((pVoipServer->strProductId[0] == '\0') || (pVoipServer->iNumGameServers == 1))
    {
        ds_strnzcpy(pVoipServer->strProductId, pGameServer->PacketData.Packet.Config.strProductId, sizeof(pVoipServer->strProductId));
    }
    else if (strncmp(pVoipServer->strProductId, pGameServer->PacketData.Packet.Config.strProductId, sizeof(pVoipServer->strProductId)) != 0)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] CRITICAL ERROR - registering gameserver has product id different from what registered gameservers have (%s vs %s)\n",
            pGameServer - pVoipServer->pGameServers, pVoipServer->strProductId, pGameServer->PacketData.Packet.Config.strProductId);
    }

    DirtyCastLogPrintf("voipgameserver: [gs%03d] processing 'conf' command diag=%d, blaze service name=%s\n",
        pGameServer - pVoipServer->pGameServers, pGameServer->uDiagnosticPort, pGameServer->PacketData.Packet.Config.strProductId);

    // recalculate global offline and available stats
    _VoipServerUpdateAvail(pVoipServer);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_data

    \Description
        Handle 'data' (user client data) GameServer command.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_data(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    GameServerPacketT *pPacket = &pGameServer->PacketData;
    uint32_t uSize, uOtbLate;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    // translate incoming packet
    pPacket->Packet.GameData.iClientId = SocketNtohl(pPacket->Packet.GameData.iClientId);
    pPacket->Packet.GameData.iDstClientId = SocketNtohl(pPacket->Packet.GameData.iDstClientId);
    pPacket->Packet.GameData.uFlag = SocketNtohs(pPacket->Packet.GameData.uFlag);
    pPacket->Packet.GameData.uTimestamp = SocketNtohl(pPacket->Packet.GameData.uTimestamp);

    // calculate user data packet size
    uSize = pPacket->Header.uSize - sizeof(pPacket->Header) - sizeof(pPacket->Packet.GameData) + sizeof(pPacket->Packet.GameData.aData);

    /* Intercept netgamelink sync messages and update the sync.echo field to contain a voisperver timestamp
       instead of a gameserver timestamp. We do this to evacuate the gameserver-to-voipserver latency from
       the netgamelink rtt calculation as the rtt is meant to be network-only latency. */ 
    VoipServerTouchNetGameLinkEcho((CommUDPPacketHeadT *)pPacket->Packet.GameData.aData, uSize);

    // send data based on flags
    VoipServerSend(pVoipServer->pBase, pPacket->Packet.GameData.iClientId, pPacket->Packet.GameData.iDstClientId, (const char *)pPacket->Packet.GameData.aData, uSize);

    // update outbound latency stats in one-second outbound latency accumulator
    uOtbLate = NetTickDiff(uCurTick, pPacket->Packet.GameData.uTimestamp);
    if ((signed)uOtbLate < 0)
    {
        if ((signed)uOtbLate < -1)
        {
            DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 1, "voipgameserver: clamping outbound latency measurement of %d to zero\n", uOtbLate);
        }
        uOtbLate = 0;
    }
    #if 0 // debug code to help debug outbound latency issue
    if (uOtbLate > 2)
    {
        DirtyCastLogPrintf("voipgameserver: [otb] late=%d (tick=%d, time=%d)\n", uOtbLate, uCurTick, pPacket->Packet.GameData.uTimestamp);
    }
    #endif
    GameServerPacketUpdateLateStatsSingle(&pGameServer->OtbLate, uOtbLate);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_ping

    \Description
        Handle GameServer 'ping' response.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_ping(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    LobbyStateE eLobbyState;
    static const char *_strStates[] = { "offline", "available", "active", "zombie" };

    NetPrintfVerbose((VoipServerGetConfig(pVoipServer->pBase)->uDebugLevel, 3, "voipgameserver: [gs%03d] got ping response\n", pGameServer - pVoipServer->pGameServers));

    // read lobby state and compare against previous
    if ((eLobbyState = SocketNtohl(pGameServer->PacketData.Packet.PingData.eState)) != pGameServer->eLobbyState)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] state change (%s->%s)\n", pGameServer - pVoipServer->pGameServers,
            _strStates[pGameServer->eLobbyState], _strStates[eLobbyState]);

        // copy new state
        pGameServer->eLobbyState = eLobbyState;

        // recalculate global offline and available stats
        _VoipServerUpdateAvail(pVoipServer);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_stat

    \Description
        Handle GameServer 'stat' response.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 06/28/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_stat(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    GameServerPacketT *pPacket = &pGameServer->PacketData;

    // translate packet data from network to host order in-place
    GameServerPacketNtoh(pPacket, pPacket, pPacket->Header.uKind);

    // accumulate end-to-end latency info
    GameServerPacketUpdateLateStats(&pGameServer->StatInfo.E2eLate, &pPacket->Packet.GameStatInfo.E2eLate);
    // accumulate inbound latency info
    GameServerPacketUpdateLateStats(&pGameServer->StatInfo.InbLate, &pPacket->Packet.GameStatInfo.InbLate);
    // accumulate outbound latency info
    GameServerPacketUpdateLateStats(&pGameServer->StatInfo.OtbLate, &pGameServer->OtbLate);
    // accumulate late bin info
    GameServerPacketUpdateLateBinStats(&pGameServer->StatInfo.LateBin, &pPacket->Packet.GameStatInfo.LateBin);

    // acccumulate packet lost and sent delta
    GameServerPacketUpdatePackLostSentStats(&pGameServer->PackLostSent, &pVoipServer->Stats.DeltaPackLostSent, &pPacket->Packet.GameStatInfo.PackLostSent);

    // accumulate rlmt change info
    pGameServer->uAccRlmtChangeCount += pPacket->Packet.GameStatInfo.uRlmtChangeCount;

    // high frequency latency logging, disabled by default
    #if 0
    {
        char strTemp[32];
        ds_snzprintf(strTemp, sizeof(strTemp), "voipgameserver: [gs%03d] [e2e]", pGameServer - pVoipServer->pGameServers);
        // output end-to-end latency info
        GameServerPacketLogLateStats(&pPacket->Packet.GameLate.E2eLate, strTemp, "    ");
        // output inbound server latency
        GameServerPacketLogLateStats(&pPacket->Packet.GameLate.InbLate, "[inb]","\n");
        // output outbound server latency
        //GameServerPacketLogLateStats(&pGameServer->OtbLate, "[otb]", "\n");
    }
    #endif

    // clear one-second outbound latency accumulator
    GameServerPacketUpdateLateStats(&pGameServer->OtbLate, NULL);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_tokn

    \Description
        Handle GameServer 'tokn' packet

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count
*/
/********************************************************************************F*/
static void _VoipServerDispatch_tokn(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    GameServerPacketT GamePacket;
    TokenApiRefT *pTokenRef = VoipServerGetTokenApi(pVoipServer->pBase);

    if (NetTickDiff(uCurTick, (uint32_t)pVoipServer->TokenData.iExpiresIn) > 0)
    {
        uint8_t bDone = TokenApiStatus(pTokenRef, 'done', NULL, 0);

        /* token is expired, the main loop will kick off a new request,
           return and the gameserver will request again
           log here so we know when these types of cases are being hit,
           this should not happen very often */
        DirtyCastLogPrintf("voipgameserver: [gs%03d] token requested but currently expired (request=%s)\n", pGameServer - pVoipServer->pGameServers, bDone == TRUE ? "idle" : "in-progress"); 
        return;
    }

    // create packet to send to gameserver
    GamePacket.Header.uKind = SocketHtonl('tokn');
    GamePacket.Header.uCode = 0;
    ds_memclr(&GamePacket.Packet.Token, sizeof(GamePacket.Packet.Token));

    ds_strnzcpy((char*)GamePacket.Packet.Token.strAccessToken, pVoipServer->TokenData.strAccessToken, sizeof(GamePacket.Packet.Token.strAccessToken));
    if (*GamePacket.Packet.Token.strAccessToken == '\0')
    {
        // If the accessToken is invalid, the token module will try to request a new one
        // The gameserver will request again
        return;
    }

    GamePacket.Header.uSize = SocketHtonl(sizeof(GamePacket.Header) + sizeof(GamePacket.Packet.Token));
    pGameServer->iLastSendResult = GameServerPacketSend(pGameServer->pBufferedSocket, &GamePacket);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDispatch_null

    \Description
        Handle an unrecognized GameServer command.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input uCurTick     - millisecond tick count

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerDispatch_null(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    uint32_t uGameServer = pGameServer - pVoipServer->pGameServers;

    DirtyCastLogPrintf("voipgameserver: [gs%03d] warning -- unhandled %d byte gameserver packet of type '%C'\n",
        uGameServer, pGameServer->iPacketSize, pGameServer->PacketData.Header.uKind);
    NetPrintMem(&pGameServer->PacketData.Header, pGameServer->PacketData.Header.uSize < 128 ? pGameServer->PacketData.Header.uSize : 128, "unhandled game packet data");
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGameServerSendPacket

    \Description
        Send a packet to GameServer.

    \Input *pGameServer - gameserver state
    \Input *pPacket     - packet to send
    \Input iPacketSize  - size of packet

    \Output
        int32_t         - GameServerPacketSend() return code

    \Version  04/17/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerGameServerSendPacket(GameServerT *pGameServer, const GameServerPacketT *pPacket)
{
    pGameServer->iLastSendResult = GameServerPacketSend(pGameServer->pBufferedSocket, pPacket);
    return(pGameServer->iLastSendResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGameServerSend

    \Description
        Forward input data on to gameserver.

    \Input *pVoipServer - voipserver ref
    \Input *pClient     - pointer to client that sent the data
    \Input *pPacketData - pointer to packet data
    \Input iPacketSize  - size of packet data
    \Input uCurTick     - current millisecond tick count

    \Notes
        The code in this file is only used with the OTP operational mode. In that mode,
        we can assume that a client is only involved in a single game at a time. Therefore,
        with DirtySDK 15.1.7.0.1 and beyond, we assume only the first entry of the
        aGameMappings[] array is used.

    \Version 03/31/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerGameServerSend(VoipGameServerRefT *pVoipServer, const VoipTunnelClientT *pClient, const char *pPacketData, int32_t iPacketSize, uint32_t uCurTick)
{
    GameServerPacketT GamePacket;
    GameServerT *pGameServer;
    uint32_t uSize;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
    NetPrintfVerbose((pConfig->uDebugLevel, 2, "voipgameserver: [gs%02d-inp] %d byte packet fm %a:%d clientId=0x%08x\n", pClient->iGameIdx, iPacketSize,
        pClient->uRemoteAddr, pClient->uRemoteGamePort, pClient->uClientId));
    #else
    NetPrintfVerbose((pConfig->uDebugLevel, 2, "voipgameserver: [gs%02d-inp] %d byte packet fm %a:%d clientId=0x%08x\n",
        pClient->aGameMappings[0].iGameIdx, iPacketSize, pClient->uRemoteAddr, pClient->uRemoteGamePort, pClient->uClientId));
    #endif

    if (pConfig->uDebugLevel > 3)
    {
        int32_t iPrintSize = iPacketSize;
        if ((iPrintSize > 64) && (pConfig->uDebugLevel < 5))
        {
            iPrintSize = 64;
        }
        NetPrintMem(pPacketData, iPrintSize, "voipserver<-client");
    }

    // create packet to send to gameserver
    GamePacket.Header.uKind = SocketHtonl('data');
    GamePacket.Header.uCode = 0;
    ds_memclr(&GamePacket.Packet.GameData, sizeof(GamePacket.Packet.GameData));
    GamePacket.Packet.GameData.iClientId = SocketHtonl(pClient->uClientId);
    GamePacket.Packet.GameData.uTimestamp = SocketHtonl(uCurTick);
    GamePacket.Packet.GameData.uFlag = 0;
    ds_memcpy(GamePacket.Packet.GameData.aData, pPacketData, iPacketSize);
    uSize = sizeof(GamePacket.Header) + sizeof(GamePacket.Packet.GameData) - sizeof(GamePacket.Packet.GameData.aData) + iPacketSize;

    #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
    // ref gameserver
    pGameServer = &pVoipServer->pGameServers[pClient->iGameIdx];
    #else
    // ref gameserver
    pGameServer = &pVoipServer->pGameServers[pClient->aGameMappings[0].iGameIdx];
    #endif

    // set size and send packet
    GamePacket.Header.uSize = SocketHtonl(uSize);
    _VoipServerGameServerSendPacket(pGameServer, &GamePacket);
    pGameServer->uLastSend = uCurTick;
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGameServerSendStats

    \Description
        Send updated statistics to GameServer.

    \Input *pVoipServer - voipserver ref
    \Input *pGameServer - gameserver ref
    \Input iGameServer  - gameserver index
    \Input *pGamePacket - game packet buffer to use to send stats to gameserver
    \Input uCurTick     - current millisecond tick count

    \Version 03/29/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerGameServerSendStats(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, int32_t iGameServer, GameServerPacketT *pGamePacket, uint32_t uCurTick)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    // don't send empty stats
    if (!memcmp(&pGameServer->GameStats, &_VoipServer_EmptyStats, sizeof(pGameServer->GameStats)))
    {
        return;
    }

    // display stat dump to debug out
    if (pConfig->uDebugLevel > 1)
    {
        NetPrintf(("voipgameserver: [gs%03d] sending updated stats\n", iGameServer));
        NetPrintf(("voipgameserver:    %4s %5s %5s %5s %5s\n", "type", "psent", "precv", "bsent", "brecv"));
        NetPrintf(("voipgameserver:    %4s %5d %5d %5d %5d\n", "game",
            pGameServer->GameStats.GameStat.uPktsSent, pGameServer->GameStats.GameStat.uPktsRecv,
            pGameServer->GameStats.GameStat.uByteSent, pGameServer->GameStats.GameStat.uByteRecv));
        NetPrintf(("voipgameserver:    %4s %5d %5d %5d %5d\n", "voip",
            pGameServer->GameStats.VoipStat.uPktsSent, pGameServer->GameStats.VoipStat.uPktsRecv,
            pGameServer->GameStats.VoipStat.uByteSent, pGameServer->GameStats.VoipStat.uByteRecv));
    }

    // create stat packet to send to gameserver
    GameServerPacketEncode(pGamePacket, 'stat', 0, &pGameServer->GameStats, sizeof(pGameServer->GameStats));

    // send packet
    _VoipServerGameServerSendPacket(pGameServer, pGamePacket);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGameServerConnect

    \Description
        Add new GameServer to list.

    \Input *pVoipServer - voipserver state
    \Input *pSocket     - new socket connection to gameserver
    \Input *pRecvAddr   - remote connection address
    \Input uCurTick     - current millisecond tick count

    \Version 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerGameServerConnect(VoipGameServerRefT *pVoipServer, SocketT *pSocket, struct sockaddr *pConnAddr, uint32_t uCurTick)
{
    GameServerT *pGameServer;
    int32_t iGameServer;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    DirtyCastLogPrintf("voipgameserver: gameserver %a:%d attempting to connect\n",
        SockaddrInGetAddr(pConnAddr), SockaddrInGetPort(pConnAddr));

    // allocate new gameserver
    for (iGameServer = 0; iGameServer < pConfig->iMaxGameServers; iGameServer++)
    {
        if (pVoipServer->pGameServers[iGameServer].pBufferedSocket == NULL)
        {
            break;
        }
    }
    if (iGameServer < pConfig->iMaxGameServers)
    {
        DirtyCastLogPrintf("voipgameserver: [gs%03d] registering\n", iGameServer);

        // ref and init gameserver
        pGameServer = &pVoipServer->pGameServers[iGameServer];
        ds_memclr(pGameServer, sizeof(*pGameServer));
        if ((pGameServer->pBufferedSocket = BufferedSocketOpen(0, 0, 32*1024, pSocket)) == NULL)
        {
            DirtyCastLogPrintf("voipgameserver: unable to create buffered socket from raw socket\n");
            SocketClose(pSocket);
            return;
        }
        pGameServer->eLobbyState = kOffline;
        pGameServer->uConnectTime = time(NULL);
        pGameServer->uLastPing = pGameServer->uLastSend = pGameServer->uLastRecv = pGameServer->uLastStat = uCurTick;
        ds_memcpy(&pGameServer->Address, pConnAddr, sizeof(pGameServer->Address));

        // add to gameserver count
        pVoipServer->iNumGameServers += 1;
    }
    else
    {
        DirtyCastLogPrintf("voipgameserver: rejecting gameserver connection: list full\n");
        SocketClose(pSocket);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGameServerDisconnect

    \Description
        Disconnect from the specified GameServer.

    \Input *pVoipServer - voipserver state
    \Input *pGameServer - gameserver info
    \Input iGameServer  - index of gameserver
    \Input uCurTick     - current millisecond tick count
    \Input *pReason     - reason for the disconnect

    \Version 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerGameServerDisconnect(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, int32_t iGameServer, uint32_t uCurTick, const char *pReason)
{
    uint8_t bGameActive = pGameServer->bGameActive;

    // make sure there is something to do
    if (pGameServer->pBufferedSocket == NULL)
    {
        return;
    }
    DirtyCastLogPrintf("voipgameserver: [gs%03d] disconnect (%s)\n", iGameServer, pReason);

    // make sure we have sole access to buffered socket for close
    NetCritEnter(NULL);

    // close gameserver socket and clear ref
    BufferedSocketClose(pGameServer->pBufferedSocket);
    pGameServer->pBufferedSocket = NULL;

    // release buffered socket access
    NetCritLeave(NULL);

    // if a game is currently active, delete it
    if (bGameActive == TRUE)
    {
        _VoipServerDeleteGame(pVoipServer, pGameServer, iGameServer, 0, uCurTick);
    }
    ds_memclr(pGameServer, sizeof(*pGameServer));

    // remove from gameserver count
    pVoipServer->iNumGameServers -= 1;

    // mark as not connected
    pGameServer->bConnected = FALSE;

    // count the disconnect and save the disconnect time
    pVoipServer->Stats.uGameServDiscCount += 1;
    pVoipServer->Stats.uLastGameServDiscTime = time(NULL);

    // update overall availability stats
    _VoipServerUpdateAvail(pVoipServer);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGameServerRecv

    \Description
        Process incoming data on a GameServer connection.

    \Input *pVoipServer     - voipserver ref
    \Input *pGameServer     - gameserver ref
    \Input uCurTick         - current millisecond tick count

    \Output
        int32_t             - negative=error, zero=no data or incomplete packet, one=read a packet

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerGameServerRecv(VoipGameServerRefT *pVoipServer, GameServerT *pGameServer, uint32_t uCurTick)
{
    int32_t iDispatch, iPayloadSize, iResult;
    uint32_t uDebugLevel;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    // recv packet from a remote host
    if ((iResult = GameServerPacketRecv(pGameServer->pBufferedSocket, &pGameServer->PacketData, (unsigned *)&pGameServer->iPacketSize)) < 0)
    {
        BufferedSocketInfoT BSocInfo;
        BufferedSocketInfo(pGameServer->pBufferedSocket, 'bnfo', 0, &BSocInfo, sizeof(BSocInfo));
        if (BSocInfo.uNumRead == 1)
        {
            if (pGameServer->PacketData.Header.uKind == 'GET ')
            {
                DirtyCastLogPrintf("voipgameserver: invalid connection attempt appears to be an http request\n");
            }
            else
            {
                DirtyCastLogPrintf("voipgameserver: invalid connection attempt is not in a valid format\n");
            }
        }
        return(iResult);
    }
    else if (iResult == 0)
    {
        return(iResult);
    }

    // update last received time
    pGameServer->uLastRecv = uCurTick;

    // calculate payload size
    iPayloadSize = pGameServer->iPacketSize - sizeof(pGameServer->PacketData.Header);

    // display received packet in network order
#if 1
    if (iPayloadSize > 0)
    {
        // intentionally blank
    }

    uDebugLevel = (pGameServer->PacketData.Header.uKind == 'ping') ? 0 : pConfig->uDebugLevel;
    GameServerPacketDebug(&pGameServer->PacketData, pGameServer->iPacketSize, uDebugLevel,
        "voipgameserver: [recv]", "voipserver<-gameserver");
#else
    NetPrintfVerbose((pConfig->uDebugLevel, 2, "voipgameserver: [recv] '%c%c%c%c' packet (%d bytes)\n",
        (pGameServer->PacketData.Header.uKind >> 24) & 0xff, (pGameServer->PacketData.Header.uKind >> 16) & 0xff,
        (pGameServer->PacketData.Header.uKind >>  8) & 0xff, (pGameServer->PacketData.Header.uKind & 0xff),
        pGameServer->iPacketSize));
    if ((iPayloadSize > 0) && (pConfig->uDebugLevel > 3))
    {
        if ((iPayloadSize > 64) && (pConfig->uDebugLevel < 5))
        {
            iPayloadSize = 64;
        }
        NetPrintMem(&pGameServer->PacketData.Packet.aData, iPayloadSize, "voipserver<-gameserver");
    }
#endif

    // dispatch the packet
    for (iDispatch = 0; ; iDispatch++)
    {
        const VoipServerDispatchT *pDispatch = &_VoipServer_aDispatch[iDispatch];
        if ((pGameServer->PacketData.Header.uKind == pDispatch->uKind) || (pDispatch->uKind == 0))
        {
            _VoipServer_aDispatch[iDispatch].pDispatch(pVoipServer, pGameServer, uCurTick);
            break;
        }
    }

    // consume the packet
    pGameServer->iPacketSize = 0;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerGameServerUpdate

    \Description
        Listen for new GameServer connections and process active GameServer connections.

    \Input *pVoipServer     - voipserver state
    \Input uCurTick         - current millisecond tick count

    \Version 04/20/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerGameServerUpdate(VoipGameServerRefT *pVoipServer, uint32_t uCurTick)
{
    struct sockaddr RecvAddr;
    int32_t iAddrLen = sizeof(RecvAddr), iGameServer, iGameServCt, iRecv, iResult = 0, iStat;
    GameServerT *pGameServer;
    GameServerPacketT GamePacket;
    SocketT *pNew;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    // if no gameserver socket to listen on, no gameserver processing
    if (pVoipServer->pListenSocket == NULL)
    {
        return;
    }
    SockaddrInit(&RecvAddr, AF_INET);

    // listen for a new gameserver connection
    if ((SocketInfo(pVoipServer->pListenSocket, 'read', 0, NULL, 0) != 0) && (pNew = SocketAccept(pVoipServer->pListenSocket, &RecvAddr, &iAddrLen)) != NULL)
    {
        // disable nagle on new socket
        SocketControl(pNew, 'ndly', 1, NULL, NULL);
        // add gameserver to list
        _VoipServerGameServerConnect(pVoipServer, pNew, &RecvAddr, uCurTick);
    }

    // process incoming data from game servers
    for (iGameServer = pVoipServer->iProcGameServer, iGameServCt = 0; iGameServCt < pConfig->iMaxGameServers; iGameServer++, iGameServCt++)
    {
        // wrap gameserver index?
        if (iGameServer >= pConfig->iMaxGameServers)
        {
            iGameServer = 0;
        }

        // ref gameserver
        pGameServer = &pVoipServer->pGameServers[iGameServer];

        // only update active gameservers
        if (pGameServer->pBufferedSocket == NULL)
        {
            continue;
        }

        // poll for connection completion
        if (pGameServer->bConnected == FALSE)
        {
            if ((iStat = BufferedSocketInfo(pGameServer->pBufferedSocket, 'stat', 0, NULL, 0)) > 0)
            {
                GameServerConfigInfoT ConfigInfo;

                NetPrintf(("voipgameserver: [gs%03d] connected\n", iGameServer));
                pGameServer->bConnected = TRUE;
                pGameServer->bGameActive = FALSE;

                // create config packet to send to gameserver
                ds_memclr(&ConfigInfo, sizeof(ConfigInfo));
                ConfigInfo.uFrontAddr = pConfig->uFrontAddr;
                ConfigInfo.uTunnelPort = pConfig->uTunnelPort;
                ConfigInfo.uDiagPort = pConfig->uDiagnosticPort;

                VoipServerStatus(pVoipServer->pBase, 'host', 0, ConfigInfo.strHostName, sizeof(ConfigInfo.strHostName));
                ds_strnzcpy(ConfigInfo.strDeployInfo, pConfig->strDeployInfo, sizeof(ConfigInfo.strDeployInfo));
                ds_strnzcpy(ConfigInfo.strDeployInfo2, pConfig->strDeployInfo2, sizeof(ConfigInfo.strDeployInfo2));

                GameServerPacketEncode(&GamePacket, 'conf', 0, &ConfigInfo, sizeof(ConfigInfo));

                // send packet and update last send time
                _VoipServerGameServerSendPacket(pGameServer, &GamePacket);
                pGameServer->uLastStat = uCurTick;
            }
            else if (iStat < 0)
            {
                DirtyCastLogPrintf("voipgameserver: [gs%03d] error %d connecting\n", iGameServer, iStat, iGameServer);
            }
            else
            {
                continue;
            }
        }

        // try and receive data
        for (iRecv = 0; iRecv < 64; iRecv++)
        {
            if ((iResult = _VoipServerGameServerRecv(pVoipServer, pGameServer, uCurTick)) < 0)
            {
                _VoipServerGameServerDisconnect(pVoipServer, pGameServer, iGameServer, uCurTick, "remote close");
                break;
            }
            else if (iResult == 0)
            {
                break;
            }
        }
        // update tick count if we received any data
        if (iRecv > 0)
        {
            uint32_t uNewTick = NetTick();
            if (NetTickDiff(uNewTick, uCurTick) > 3)
            {
                DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 0, "voipgameserver: [gs%03d] iRecv=%d in %ums\n", iGameServer, iRecv, NetTickDiff(uNewTick, uCurTick));
                uCurTick = uNewTick;
            }
        }
        // if we've disconnected, move on to next gameserver
        if (iResult < 0)
        {
            continue;
        }
        // check for timeout
        if (NetTickDiff(uCurTick, pGameServer->uLastRecv) > (signed)pConfig->uPeerTimeout)
        {
            char strTimeout[32];
            ds_snzprintf(strTimeout, sizeof(strTimeout), "timeout (%dms)", (signed)(uCurTick - pGameServer->uLastRecv));
            _VoipServerGameServerDisconnect(pVoipServer, pGameServer, iGameServer, uCurTick, strTimeout);
            continue;
        }

        // see if we should send a keep-alive (sent at rate of 1/2 the timeout)
        if (NetTickDiff(uCurTick, pGameServer->uLastPing) > (signed)(pConfig->uPeerTimeout/2))
        {
            PingDataT PingData;

            NetPrintfVerbose((pConfig->uDebugLevel, 3, "voipgameserver: [gs%03d] pinging\n", iGameServer));

            // create packet to send to gameserver
            ds_memclr(&PingData, sizeof(PingData));

            // tell the game server about the state we are trying to achieve
            if (VoipServerStatus(pVoipServer->pBase,'psta', 0, NULL, 0) >= VOIPSERVER_PROCESS_STATE_DRAINING)
            {
                PingData.eState = kZombie;
            }
            else
            {
                PingData.eState = kAvailable;
            }
#if DIRTYCODE_DEBUG
            // let the debug active state override the desire to go to the zombie state, to simulate people still being in game
            if (pVoipServer->bDbgActiveState)
            {
                PingData.eState = kActive;
            }
#endif

            GameServerPacketEncode(&GamePacket, 'ping', 0, &PingData, sizeof(PingData));

            // send packet and update last send time
            _VoipServerGameServerSendPacket(pGameServer, &GamePacket);
            pGameServer->uLastPing = uCurTick;
        }

        // send updated stats every fifteen seconds
        if (NetTickDiff(uCurTick, pGameServer->uLastStat) > VOIPSERVER_GS_STATRATE)
        {
            // send updated stats
            _VoipServerGameServerSendStats(pVoipServer, pGameServer, iGameServer, &GamePacket, uCurTick);

            // update accumulated stats and clear current stat structure
            GameServerUpdateGameStats(&pGameServer->GameStats, &pGameServer->GameStatsCur, &pGameServer->GameStatsSum, &pGameServer->GameStatsMax, FALSE);

            // update last send time
            pGameServer->uLastStat = uCurTick;
        }

        // flush any pending buffered packets
        if (pGameServer->pBufferedSocket != NULL)
        {
            BufferedSocketFlush(pGameServer->pBufferedSocket);
        }
    }

    // start with next gameserver next loop
    pVoipServer->iProcGameServer += 1;
    if (pVoipServer->iProcGameServer >= pConfig->iMaxGameServers)
    {
        pVoipServer->iProcGameServer = 0;
    }

    // pump tcpmetrics module
    if ((pVoipServer->iTcpMetricsNetlinkSocket >= 0) && (NetTickDiff(uCurTick, pVoipServer->uLastTcpMetricsUpdate) > VOIPGAMESERVER_TCPMETRICS_RATE))
    {
        // update our metrics update tick
        pVoipServer->uLastTcpMetricsUpdate = uCurTick;

        // recv data from the last query and kick off new request
        TcpMetricsQueryRecv(pVoipServer->iTcpMetricsNetlinkSocket);
        TcpMetricsQuerySend(pVoipServer->iTcpMetricsNetlinkSocket, pConfig->uApiPort);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateLate

    \Description
        Calculate summed latency for all GameServers

    \Input *pVoipServer     - module state
    \Input uCurTick         - current tick count

    \Version 06/28/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateLate(VoipGameServerRefT *pVoipServer, uint32_t uCurTick)
{
    GameServerT *pGameServer;
    int32_t iGameServer;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    // accumulate latency info from all active gameservers and clear source latency info
    for (iGameServer = 0; iGameServer < pConfig->iMaxGameServers; iGameServer++)
    {
        pGameServer = &pVoipServer->pGameServers[iGameServer];
        if (pGameServer->bGameActive == TRUE)
        {
            // update stats and reset source for each late stat
            VoipServerControl(pVoipServer->pBase, 'e2el', 0, 0, &pGameServer->StatInfo.E2eLate);
            GameServerPacketUpdateLateStats(&pGameServer->StatInfo.E2eLate, NULL);

            VoipServerControl(pVoipServer->pBase, 'inbl', 0, 0, &pGameServer->StatInfo.InbLate);
            GameServerPacketUpdateLateStats(&pGameServer->StatInfo.InbLate, NULL);

            VoipServerControl(pVoipServer->pBase, 'otbl', 0, 0, &pGameServer->StatInfo.OtbLate);
            GameServerPacketUpdateLateStats(&pGameServer->StatInfo.OtbLate, NULL);

            VoipServerControl(pVoipServer->pBase, 'lbin', 0, 0, &pGameServer->StatInfo.LateBin);
            GameServerPacketUpdateLateBinStats(&pGameServer->StatInfo.LateBin, NULL);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateMisc

    \Description
        Update misc metrics

    \Input *pVoipServer     - module state
    \Input uCurTick         - current tick count
    \Input bResetMetrics    - if true, reset metrics accumulator

    \Version 06/28/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateMisc(VoipGameServerRefT *pVoipServer, uint32_t uCurTick, uint8_t bResetMetrics)
{
    int32_t iGameServer;
    GameServerT *pGameServer;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);

    if (bResetMetrics)
    {
        pVoipServer->Stats.uGameServDiscCount = 0;
        pVoipServer->Stats.uAccRlmtChangeCount = 0;
        ds_memclr(&pVoipServer->Stats.DeltaPackLostSent, sizeof(pVoipServer->Stats.DeltaPackLostSent));
        ds_memcpy(pVoipServer->Stats.aLastGameEndStateCt, pVoipServer->Stats.aGameEndStateCt, sizeof(pVoipServer->Stats.aLastGameEndStateCt));
        ds_memcpy(pVoipServer->Stats.aLastClientRemovalInfo, pVoipServer->Stats.aClientRemovalInfo, sizeof(pVoipServer->Stats.aLastClientRemovalInfo));
    }

    // accumulate rlmt info from all active gameservers and clear source info
    for (iGameServer = 0; iGameServer < pConfig->iMaxGameServers; iGameServer++)
    {
        pGameServer = &pVoipServer->pGameServers[iGameServer];
        if (pGameServer->bGameActive == TRUE)
        {
            // update stats and reset source for each late stat
            pVoipServer->Stats.uAccRlmtChangeCount += pGameServer->uAccRlmtChangeCount;
            pGameServer->uAccRlmtChangeCount = 0;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateMonitor

    \Description
        Update monitor flags

    \Input *pVoipGameServer     - voipserver module state
    \Input  uCurTick        - current tick for this frame

    \Output
        None.

    \Notes
        VoipServer Warnings/Errors
        -----------------
        Number of available GameServers < threshold (config-file specified, percentage).
        Number of available client slots < threshold (config-file specified, percentage).
        Average client latency > threshold (config-file specified, milliseconds)
        Server Load CPU % > threshold (config-file specified, percentage)
        System Load Average 5min > threshold (config-file specified, integer load average)
        No clients connected but users > 0 (suggests client-facing ports are not reachable)
        Any registered GameServers being offline (usually means not connected to EAFN).

    \Version 08/17/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateMonitor(VoipGameServerRefT *pVoipGameServer, uint32_t uCurTick)
{
    VoipServerStatT VoipServerStats;
    VoipServerRefT *pVoipServer = VoipGameServerGetBase(pVoipGameServer);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    ServerMonitorConfigT *pWrn = (ServerMonitorConfigT *)&pConfig->MonitorWarnings;
    ServerMonitorConfigT *pErr = (ServerMonitorConfigT *)&pConfig->MonitorErrors;
    uint32_t uPrevFlagWarnings = (uint32_t)VoipServerStatus(pVoipServer, 'mwrn', 0, NULL, 0);
    uint32_t uPrevFlagErrors = (uint32_t)VoipServerStatus(pVoipServer, 'merr', 0, NULL, 0);
    uint32_t uSignalFlags = (uint32_t)VoipServerStatus(pVoipServer, 'sgnl', 0, NULL, 0);
    uint32_t uValue, bClearOnly, uMonitorFlagWarnings, uMonitorFlagErrors;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    UdpMetricsT UdpMetrics;

    // get voipserver stats
    VoipServerStatus(pVoipServer, 'stat', 0, &VoipServerStats, sizeof(VoipServerStats));
    VoipServerStatus(pVoipServer, 'udpm', 0, &UdpMetrics, sizeof(UdpMetrics));

    // check gameserver percentage available (skip this if shutting down)
    bClearOnly = (uSignalFlags & VOIPSERVER_SIGNAL_SHUTDOWN_IFEMPTY) ? TRUE : FALSE;
    uValue = (pConfig->iMaxGameServers > 0) ? (pVoipGameServer->Stats.iGamesAvailable * 100) / pConfig->iMaxGameServers : 0;
    VoipServerUpdateMonitorFlag(pVoipServer, uValue < pWrn->uPctGamesAvail, uValue < pErr->uPctGamesAvail, VOIPSERVER_MONITORFLAG_PCTGAMESAVAIL, bClearOnly);

    // check client slot percentage available
    uValue = ((pConfig->iMaxClients - VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0)) * 100) / pConfig->iMaxClients;
    VoipServerUpdateMonitorFlag(pVoipServer, uValue < pWrn->uPctClientSlot, uValue < pErr->uPctClientSlot, VOIPSERVER_MONITORFLAG_PCTCLIENTSLOT, FALSE);

    // check average client latency, if system load is 1/2 of warning threshold or greater
    bClearOnly = (VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[1]) < (pWrn->uAvgSystemLoad/2)) ? TRUE : FALSE;
    uValue = (VoipServerStats.StatInfo.E2eLate.uNumStat != 0) ? VoipServerStats.StatInfo.E2eLate.uSumLate / VoipServerStats.StatInfo.E2eLate.uNumStat : 0;
    VoipServerUpdateMonitorFlag(pVoipServer, uValue > pWrn->uAvgClientLate, uValue > pErr->uAvgClientLate, VOIPSERVER_MONITORFLAG_AVGCLIENTLATE, bClearOnly);

    // check pct server load
    uValue = (uint32_t)VoipServerStats.ServerInfo.fCpu;
    VoipServerUpdateMonitorFlag(pVoipServer, uValue > pWrn->uPctServerLoad, uValue > pErr->uPctServerLoad, VOIPSERVER_MONITORFLAG_PCTSERVERLOAD, FALSE);

    // check pct system load
    uValue = VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[1]);
    VoipServerUpdateMonitorFlag(pVoipServer, uValue > pWrn->uAvgSystemLoad, uValue > pErr->uAvgSystemLoad, VOIPSERVER_MONITORFLAG_AVGSYSTEMLOAD, FALSE);

    // check to see if clients are having trouble connecting to the server
    uValue = pVoipGameServer->Stats.iGamesFailedToRecv;
    VoipServerUpdateMonitorFlag(pVoipServer, uValue > pWrn->uFailedGameCnt, uValue > pErr->uFailedGameCnt, VOIPSERVER_MONITORFLAG_FAILEDGAMECNT, FALSE);

    // check to see if any gameservers are offline
    uValue = pVoipGameServer->Stats.iGamesOffline > 0;
    VoipServerUpdateMonitorFlag(pVoipServer, uValue != 0, 0, VOIPSERVER_MONITORFLAG_GAMESERVOFFLN, FALSE);

    // check the UDP stats
    VoipServerUpdateMonitorFlag(pVoipServer, UdpMetrics.uRecvQueueLen > pWrn->uUdpRecvQueueLen, UdpMetrics.uRecvQueueLen > pErr->uUdpRecvQueueLen, VOIPSERVER_MONITORFLAG_UDPRECVQULEN, FALSE);
    VoipServerUpdateMonitorFlag(pVoipServer, UdpMetrics.uSendQueueLen > pWrn->uUdpSendQueueLen, UdpMetrics.uSendQueueLen > pErr->uUdpSendQueueLen, VOIPSERVER_MONITORFLAG_UDPSENDQULEN, FALSE);

    // update the values
    uMonitorFlagWarnings = (uint32_t)VoipServerStatus(pVoipServer, 'mwrn', 0, NULL, 0);
    uMonitorFlagErrors = (uint32_t)VoipServerStatus(pVoipServer, 'merr', 0, NULL, 0);   

    // debug output if monitor state has changed
    if (uMonitorFlagWarnings != uPrevFlagWarnings)
    {
        DirtyCastLogPrintf("voipserverdiagnostic: monitor warning state change: 0x%08x->0x%08x\n", uPrevFlagWarnings, uMonitorFlagWarnings);
    }
    if (uMonitorFlagErrors != uPrevFlagErrors)
    {
        DirtyCastLogPrintf("voipserverdiagnostic: monitor error state change: 0x%08x->0x%08x\n", uPrevFlagErrors, uMonitorFlagErrors);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipServerUpdateMetrics

    \Description
        Update various server metrics, surfased by diagnostic pages.

    \Input *pVoipServer     - module state
    \Input uCurTick         - current tick count

    \Version 06/02/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipServerUpdateMetrics(VoipGameServerRefT *pVoipServer, uint32_t uCurTick)
{
    int32_t iResetTicks;

    // rate limit metrics updating to once a second
    if (NetTickDiff(uCurTick, pVoipServer->uLastMetricsUpdate) < 1000)
    {
        return;
    }
    pVoipServer->uLastMetricsUpdate = uCurTick;

    // if metrics accumulators have not been reset for a while, do it now
    if ((iResetTicks = NetTickDiff(uCurTick, pVoipServer->uLastMetricsReset)) > VOIPSERVER_MAXMETRICSRESETRATE)
    {
        pVoipServer->bResetMetrics = TRUE;
    }

    // update latency calcs
    _VoipServerUpdateLate(pVoipServer, uCurTick);

    // update misc metrics
    _VoipServerUpdateMisc(pVoipServer, uCurTick, pVoipServer->bResetMetrics);

    // update diagnostic module (updates monitor metrics)
    _VoipServerUpdateMonitor(pVoipServer, uCurTick);

    // if we reset metrics, update our tracking
    if (pVoipServer->bResetMetrics)
    {
        pVoipServer->bResetMetrics = FALSE;
        pVoipServer->uLastMetricsReset = uCurTick;
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipGameServerGameEventCb

    \Description
        Callback handler for data received on virtual sockets that are not the VoIP port

    \Input *pSocket     - pointer to socket
    \Input iFlags       - unused
    \Input *pUserData   - user data

    \Output
        int32_t         - zero if packet not consumed, >zero otherwise

    \Notes
        The code in this file is only used with the OTP operational mode. In that mode,
        we can assume that a client is only involved in a single game at a time. Therefore,
        with DirtySDK 15.1.7.0.1 and beyond, we assume only the first entry of the
        aGameMappings[] array is used.

    \Version 03/31/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipGameServerGameEventCb(const GameEventDataT *pEventData, void *pUserData)
{
    VoipGameServerRefT *pVoipServer = (VoipGameServerRefT *)pUserData;
    #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
    GameServerT *pGameServer = &pVoipServer->pGameServers[pEventData->pClient->iGameIdx];
    #else
    GameServerT *pGameServer = &pVoipServer->pGameServers[pEventData->pClient->aGameMappings[0].iGameIdx];
    #endif
    const VoipTunnelClientT *pClient = pEventData->pClient;

    if (pEventData->eEvent == VOIPSERVER_EVENT_RECVGAME)
    {
        const uint32_t uByteRecv = VOIPSERVER_EstimateNetworkPacketSize(pEventData->iDataSize);

        // clear "have not received any data" tracker
        pVoipServer->Stats.iGamesFailedToRecv = 0;

        // update stats
        pGameServer->GameStats.GameStat.uByteRecv += uByteRecv;
        pGameServer->GameStats.GameStat.uPktsRecv += 1;

        // forward data to gameserver
        NetCritEnter(NULL);
        if (pGameServer->pBufferedSocket != NULL)
        {
            _VoipServerGameServerSend(pVoipServer, pClient, pEventData->pData, pEventData->iDataSize, pEventData->uCurTick);
        }
        NetCritLeave(NULL);

        // let the base module know that we consumed the packet
        return(1);

    }
    else if (pEventData->eEvent == VOIPSERVER_EVENT_SENDGAME)
    {
        const uint32_t uByteSent = VOIPSERVER_EstimateNetworkPacketSize(pEventData->iDataSize);

        pGameServer->GameStats.GameStat.uByteSent += uByteSent;
        pGameServer->GameStats.GameStat.uPktsSent += 1;
        return(0);
    }

    // unhandled
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipGameServerTcpMetricsCb

    \Description
        Callback fired when new mem info is available for a specific AF_INET
        TCP socket.

    \Input *pTcpMetrics - pointer to TCP metrics
    \Input *pUserData   - user data

    \Notes
        The voipserver binds the 'listening socket' to port X and listens for gameserver-initiated
        connection attempts on that that port. Upon connection detection, the voipserver
        invokes accept() to create a new TCP socket to serve a specific gameserver-to-voipserver
        connection. As a result of this process completing, the voipserver has N sockets bound
        to port X: 1 listening socket and (N-1) per-gameserver TCP sockets all sharing the same source
        port but distinguishable because thay all have a different destination port. The code in this
        function uses source port and destination port to match the event to the right BufferedSocket.
        The listening socket info will always be ignored because there is no BufferedSocket associated
        with it.

    \Version 01/21/2021 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipGameServerTcpMetricsCb(const TcpMetricsT *pTcpMetrics, void *pUserData)
{
    VoipGameServerRefT *pVoipServer = (VoipGameServerRefT *)pUserData;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer->pBase);
    int32_t iGameServer;

    // find the matching buffered socket
    for (iGameServer = 0; iGameServer < pConfig->iMaxGameServers; iGameServer++)
    {
        if (pVoipServer->pGameServers[iGameServer].pBufferedSocket != NULL)
        {
            uint16_t uSrcPort = (uint16_t)BufferedSocketInfo(pVoipServer->pGameServers[iGameServer].pBufferedSocket, 'srcp', 0, NULL, 0);
            uint16_t uDstPort = (uint16_t)BufferedSocketInfo(pVoipServer->pGameServers[iGameServer].pBufferedSocket, 'dstp', 0, NULL, 0);

            if ((uSrcPort == pTcpMetrics->uSrcPort) && (uDstPort == pTcpMetrics->uDstPort))
            {
                // pass new samples to BufferedSocket
                BufferedSocketControl(pVoipServer->pGameServers[iGameServer].pBufferedSocket, 'sndq', pTcpMetrics->uSendQueueLen, NULL, 0);
                BufferedSocketControl(pVoipServer->pGameServers[iGameServer].pBufferedSocket, 'rcvq', pTcpMetrics->uSendQueueLen, NULL, 0);
                break;
           }
        }
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipGameServerInitialize

    \Description
        Initialize a VoipServer.  Read the config file, setup the network,
        and setup data structures.  Once this method completes successfully...

    \Input *pVoipServer     - module state for base module (shared functionality)
    \Input *pCommandTags    - tags from command line
    \Input *pConfigTags     - tags from config file

    \Output
        VoipGameServerRefT * - module state for mode

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
VoipGameServerRefT *VoipGameServerInitialize(VoipServerRefT *pVoipServer, const char *pCommandTags, const char *pConfigTags)
{
    const VoipServerConfigT *pConfig;
    VoipGameServerRefT *pVoipGameServer;
    int32_t iMemGroup;
    void *pMemGroupUserdata;
    TokenApiRefT *pTokenRef = VoipServerGetTokenApi(pVoipServer);

    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserdata);

    // try to allocate the mode state
    if ((pVoipGameServer = (VoipGameServerRefT *)DirtyMemAlloc(sizeof(VoipGameServerRefT), VOIPGAMESERVER_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        return(NULL);
    }
    ds_memclr(pVoipGameServer, sizeof(VoipGameServerRefT));
    pVoipGameServer->iMemGroup = iMemGroup;
    pVoipGameServer->pMemGroupUserdata = pMemGroupUserdata;

    // save base module
    pVoipGameServer->pBase = pVoipServer;

    // metrics update tracking info
    pVoipGameServer->uLastMetricsUpdate = pVoipGameServer->uLastMetricsReset = NetTick();

    // initialize stats struct
    ds_memclr(&_VoipServer_EmptyStats, sizeof(_VoipServer_EmptyStats));

    pConfig = VoipServerGetConfig(pVoipServer);

    // create netlink socket to query 
    if ((pVoipGameServer->iTcpMetricsNetlinkSocket = TcpMetricsSocketOpen(_VoipGameServerTcpMetricsCb, pVoipGameServer)) < 0)
    {
        DirtyCastLogPrintf("voipgameserver: unable to create netlink socket with TcpMetricsSocketOpen()\n");
    }
    pVoipGameServer->uLastTcpMetricsUpdate = NetTick();

    if (pConfig->uApiPort != 0)
    {
        if ((pVoipGameServer->pListenSocket = VoipServerSocketOpen(SOCK_STREAM, 0, pConfig->uApiPort, TRUE, NULL, NULL)) != NULL)
        {
            DirtyCastLogPrintf("voipgameserver: created gameserver socket bound to port %d\n", pConfig->uApiPort);
            if (SocketListen(pVoipGameServer->pListenSocket, 2) != SOCKERR_NONE)
            {
                DirtyCastLogPrintf("voipgameserver: error listening on gameserver socket\n");
            }
        }
        else
        {
            DirtyCastLogPrintf("voipgameserver: unable to create gameserver socket\n");
            VoipGameServerShutdown(pVoipGameServer);
            return(NULL);
        }
        if ((pVoipGameServer->pGameServers = (GameServerT *)DirtyMemAlloc(pConfig->iMaxGameServers * sizeof(GameServerT), VOIPGAMESERVER_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
        {
            DirtyCastLogPrintf("voipgameserver: unable to allocate gameserver array\n");
            VoipGameServerShutdown(pVoipGameServer);
            return(NULL);
        }
        ds_memclr(pVoipGameServer->pGameServers, pConfig->iMaxGameServers * sizeof(GameServerT));
    }
    else
    {
        DirtyCastLogPrintf("voipgameserver: no gameserver listen port configured; gameservers will be unable to connect to this server\n");
    }

    // set game/voip callback
    VoipServerCallback(pVoipServer, _VoipGameServerGameEventCb, _VoipServerVoipTunnelEventCallback, pVoipGameServer);

    // make request for new token, this as a preload for request from the GS
    TokenApiAcquisitionAsync(pTokenRef, _VoipServerTokenAcquisitionCb, pVoipGameServer);

    // done
    DirtyCastLogPrintf("voipgameserver: server initialized\n");
    return(pVoipGameServer);
}

/*F********************************************************************************/
/*
    \Function VoipGameServerProcess

    \Description
        Main server process loop.

    \Input *pVoipGameServer - module state
    \Input *pSignalFlags    - VOIPSERVER_SIGNAL_* flags passing signal state to process.
    \Input uCurTick         - current process tick

    \Output
        uint8_t             - TRUE to continue processing, FALSE to exit

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
uint8_t VoipGameServerProcess(VoipGameServerRefT *pVoipGameServer, uint32_t *pSignalFlags, uint32_t uCurTick)
{
    TokenApiRefT *pTokenApi = VoipServerGetTokenApi(pVoipGameServer->pBase);

    // check if we need to acquire a new token when there is not already a request in progress
    if ((TokenApiStatus(pTokenApi, 'done', NULL, 0) == TRUE) &&
        (NetTickDiff(uCurTick, pVoipGameServer->TokenData.iExpiresIn) > 0))
    {
        TokenApiAcquisitionAsync(pTokenApi, _VoipServerTokenAcquisitionCb, pVoipGameServer);
    }

    // listen for new game server connections, and process active ones
    _VoipServerGameServerUpdate(pVoipGameServer, uCurTick);

    // update metrics
    _VoipServerUpdateMetrics(pVoipGameServer, uCurTick);

    /* make sure we recalculate global offline and available stats at least every 10 sec to catch stuck games early
       this call is needed here to avoid _VoipServerUpdateAvail() not executing when dgam, ping, conf are not flowing */
    if (NetTickDiff(NetTick(), pVoipGameServer->uLastUpdateAvail) > 10000)
    {
        _VoipServerUpdateAvail(pVoipGameServer);
    }

    // check signal flag status
    if ((*pSignalFlags & VOIPSERVER_SIGNAL_SHUTDOWN_IFEMPTY))
    {
        // if we aren't already draining, start draining
        if (VoipServerStatus(pVoipGameServer->pBase, 'psta', 0, NULL, 0) < VOIPSERVER_PROCESS_STATE_DRAINING)
        {
            VoipServerProcessMoveToNextState(pVoipGameServer->pBase, VOIPSERVER_PROCESS_STATE_DRAINING);
        }
    }

    if (VoipServerStatus(pVoipGameServer->pBase, 'psta', 0, NULL, 0) == VOIPSERVER_PROCESS_STATE_EXITED)
    {
        return(FALSE);
    }

    // continue processing
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function VoipGameServerGetBase

    \Description
        Gets the base module for getting shared functionality

    \Input *pVoipGameServer   - module state

    \Output
        VoipServerRefT *    - the pointer to the VoipServerRefT module

    \Version 12/07/2015 (eesponda)
*/
/********************************************************************************F*/
VoipServerRefT *VoipGameServerGetBase(VoipGameServerRefT *pVoipGameServer)
{
    return(pVoipGameServer->pBase);
}

/*F********************************************************************************/
/*!
    \Function VoipGameServerMatchIndex

    \Description
        Get the gameserver data based on index

    \Input *pVoipGameServer   - module state
    \Input iGameServer        - index of gameserver

    \Output
        GameServerT *    - the pointer to the gameserver data or NULL if outside max gameservers

    \Version 12/07/2015 (eesponda)
*/
/********************************************************************************F*/
GameServerT *VoipGameServerMatchIndex(VoipGameServerRefT *pVoipGameServer, int32_t iGameServer)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipGameServer->pBase);
    if (iGameServer > pConfig->iMaxGameServers)
    {
        return(NULL);
    }
    return(pVoipGameServer->pGameServers + iGameServer);
}

/*F********************************************************************************/
/*!
    \Function VoipGameServerStatus

    \Description
        Get module status

    \Input *pVoipGameServer - pointer to module state
    \Input iSelect          - status selector
    \Input iValue           - selector specific
    \Input *pBuf            - [out] - selector specific
    \Input iBufSize         - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'done' - return if we completed all time critical operations
            'ngam' - return number of gameservers
            'prod' - copy product id string in caller's buffer
            'read' - return TRUE if kubernetes readiness probe can succeed, FALSE otherwise
            'stat' - return the voipserver stats via pBuf and reset based on iValue (>=0)
        \endverbatim

    \Version 12/08/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipGameServerStatus(VoipGameServerRefT *pVoipGameServer, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'done')
    {
        TokenApiRefT *pTokenRef = VoipServerGetTokenApi(pVoipGameServer->pBase);
        return(TokenApiStatus(pTokenRef, 'done', NULL, 0));
    }
    if (iSelect == 'ngam')
    {
        return(pVoipGameServer->iNumGameServers);
    }
    if (iSelect == 'prod')
    {
        if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(pVoipGameServer->strProductId)))
        {
            if (pVoipGameServer->strProductId[0] != '\0')
            {
                ds_strnzcpy(pBuf, pVoipGameServer->strProductId, sizeof(pVoipGameServer->strProductId));
            }
            else
            {
                ds_strnzcpy(pBuf, "uninitialized", sizeof(pVoipGameServer->strProductId));
            }
            return(0);
        }
        return(-1);
    }
    if (iSelect == 'read')
    {
        // we return 'ready' when all gameservers are connected and ready to host a game
        return((pVoipGameServer->iNumGameServers == VoipServerGetConfig(pVoipGameServer->pBase)->iMaxGameServers) && (pVoipGameServer->Stats.iGamesOffline == 0));
    }
    if (iSelect == 'stat')
    {
        if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(pVoipGameServer->Stats)))
        {
            ds_memcpy(pBuf, &pVoipGameServer->Stats, sizeof(pVoipGameServer->Stats));
            return(0);
        }
        return(-1);
    }
    // selector not supported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipGameServerControl

    \Description
        Module control function

    \Input *pVoipGameServer     - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'asac' - pretend to be in active game state, for debug purposes
            'asdz' - start draining and move towards the zombie state
            'gsta' - reset gamestats
            'stat' - queue resetting of stats based on iValue (TRUE/FALSE)
        \endverbatim

    \Version 12/02/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipGameServerControl(VoipGameServerRefT *pVoipGameServer, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
#if DIRTYCODE_DEBUG
    if (iControl == 'asac')
    {
        pVoipGameServer->bDbgActiveState = (uint8_t)iValue;
        return(0);
    }
#endif
    if (iControl == 'asdz')
    {
        DirtyCastLogPrintf("voipgameserver: starting drain due to VoipGameServerControl('asdz')\n");
        VoipServerProcessMoveToNextState(pVoipGameServer->pBase, VOIPSERVER_PROCESS_STATE_DRAINING);
        return(0);
    }    
    if (iControl == 'gsta')
    {
        ds_memclr(pVoipGameServer->Stats.aClientRemovalInfo, sizeof(pVoipGameServer->Stats.aClientRemovalInfo));
        ds_memclr(pVoipGameServer->Stats.aGameEndStateCt, sizeof(pVoipGameServer->Stats.aGameEndStateCt));
        return(0);
    }
    if (iControl == 'stat')
    {
        pVoipGameServer->bResetMetrics = (uint8_t)iValue;
        return(0);
    }

    // control not supported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipGameServerDrain

    \Description
        Perform drain actions.

    \Input *pVoipGameServer - module state

    \Output
        uint32_t            - TRUE if drain complete, FALSE otherwise

    \Version 01/02/2020 (cvienneau)
*/
/********************************************************************************F*/
uint32_t VoipGameServerDrain(VoipGameServerRefT *pVoipGameServer)
{
    if ((pVoipGameServer->Stats.iGamesAvailable == 0) && (pVoipGameServer->Stats.iGamesActive == 0))
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

/*F********************************************************************************/
/*!
    \Function VoipGameServerShutdown

    \Description
        Shutdown this VoipServer.

    \Input *pVoipGameServer - module state

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void VoipGameServerShutdown(VoipGameServerRefT *pVoipGameServer)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipGameServer->pBase);

    // destroy gameserver array
    if (pVoipGameServer->pGameServers != NULL)
    {
        int32_t iGameServer;
        // disconnect from any gameservers we're connected to
        for (iGameServer = 0; iGameServer < pConfig->iMaxGameServers; iGameServer++)
        {
            _VoipServerGameServerDisconnect(pVoipGameServer, &pVoipGameServer->pGameServers[iGameServer], iGameServer, NetTick(), "shutdown");
        }
        // dispose of gameserver array
        DirtyMemFree(pVoipGameServer->pGameServers, VOIPGAMESERVER_MEMID, pVoipGameServer->iMemGroup, pVoipGameServer->pMemGroupUserdata);
        pVoipGameServer->pGameServers = NULL;
    }
    // destroy gameserver socket
    if (pVoipGameServer->pListenSocket != NULL)
    {
        SocketClose(pVoipGameServer->pListenSocket);
        pVoipGameServer->pListenSocket = NULL;
    }

    // close the netlink socket
    if (pVoipGameServer->iTcpMetricsNetlinkSocket >= 0)
    {
        TcpMetricsSocketClose(pVoipGameServer->iTcpMetricsNetlinkSocket);
        pVoipGameServer->iTcpMetricsNetlinkSocket = -1;
    }

    // cleanup mode state
    DirtyMemFree(pVoipGameServer, VOIPGAMESERVER_MEMID, pVoipGameServer->iMemGroup, pVoipGameServer->pMemGroupUserdata);
}
