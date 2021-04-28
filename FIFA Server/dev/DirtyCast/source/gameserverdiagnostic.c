/*H********************************************************************************/
/*!
    \File gameserverdiagnostic.c

    \Description
        This module implements diagnostic page formatting for the VoipServer GameServer
        mode

    \Copyright
        Copyright (c) 2007-2018 Electronic Arts Inc.

    \Version 03/13/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_LINUX)
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#endif

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtyvers.h"

#include "commondiagnostic.h"
#include "serverdiagnostic.h"
#include "serveraction.h"
#include "voipserverconfig.h"
#include "voipgameserver.h"
#include "voipserver.h"
#include "gameserverdiagnostic.h"

/*** Defines **********************************************************************/

// status flags
#define VOIPSERVERDIAGNOSTIC_STATUS_DISCONNECTED    (1)
#define VOIPSERVERDIAGNOSTIC_STATUS_OFFLINE         (2)
#define VOIPSERVERDIAGNOSTIC_STATUS_AVAILABLE       (4)
#define VOIPSERVERDIAGNOSTIC_STATUS_ACTIVE          (8)
#define VOIPSERVERDIAGNOSTIC_STATUS_STUCK           (16)
#define VOIPSERVERDIAGNOSTIC_STATUS_ZOMBIE          (32)
#define VOIPSERVERDIAGNOSTIC_STATUS_ALL             (VOIPSERVERDIAGNOSTIC_STATUS_DISCONNECTED|VOIPSERVERDIAGNOSTIC_STATUS_OFFLINE|VOIPSERVERDIAGNOSTIC_STATUS_AVAILABLE|VOIPSERVERDIAGNOSTIC_STATUS_ACTIVE|VOIPSERVERDIAGNOSTIC_STATUS_ZOMBIE)

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

static const VoipServerDiagnosticMonitorT _MonitorDiagnostics[] =
{
    { VOIPSERVER_MONITORFLAG_PCTGAMESAVAIL, "Not enough available GameServers." },
    { VOIPSERVER_MONITORFLAG_PCTCLIENTSLOT, "Not enough available client slots." },
    { VOIPSERVER_MONITORFLAG_AVGCLIENTLATE, "Average client latency is too high." },
    { VOIPSERVER_MONITORFLAG_PCTSERVERLOAD, "Server CPU percentage is too high." },
    { VOIPSERVER_MONITORFLAG_AVGSYSTEMLOAD, "System load is too high." },
    { VOIPSERVER_MONITORFLAG_FAILEDGAMECNT, "Clients unable to connect to server." },
    { VOIPSERVER_MONITORFLAG_UDPRECVQULEN,  "UDP recv queue is almost full, network throughput too high." },
    { VOIPSERVER_MONITORFLAG_UDPSENDQULEN,  "UDP send queue is almost full, network throughput too high." },
    { VOIPSERVER_MONITORFLAG_GAMESERVOFFLN, "Some GameServers are offline." },
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipServerDiagnosticGameServerCurGame

    \Description
        Format active game info for diagnostic display page.

    \Input *pVoipServer         - voipserver ref
    \Input iGameServer          - gameserver index
    \Input *pBuffer             - output buffer
    \Input iBufSize             - size of output buffer

    \Output
        int32_t                 - number of bytes written to output buffer

    \Version 04/06/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerDiagnosticGameServerCurGame(VoipGameServerRefT *pVoipGameServer, int32_t iGameServer, char *pBuffer, int32_t iBufSize)
{
    int32_t iGameClientIdx, iNumGameClients, iOffset = 0;
    const int32_t iStatRate = VOIPSERVER_GS_STATRATE/1000;
    VoipTunnelClientT *pGameClient;
    VoipTunnelGameT GameInfo;
    uint32_t uSeconds, uMinutes;
    VoipServerRefT *pVoipServer = VoipGameServerGetBase(pVoipGameServer);
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    ProtoTunnelRefT *pProtoTunnel = VoipServerGetProtoTunnel(pVoipServer);

    // ref gameserver
    GameServerT *pGameServer = VoipGameServerMatchIndex(pVoipGameServer, iGameServer);

    // active?
    if (pGameServer->bGameActive && (VoipTunnelStatus(pVoipTunnel, 'game', iGameServer, &GameInfo, sizeof(GameInfo)) == 0))
    {
        // display game info
        iOffset = ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       game info:\n");

        // display client info
        for (iGameClientIdx = 0, iNumGameClients = 0; iGameClientIdx < VOIPTUNNEL_MAXGROUPSIZE; iGameClientIdx += 1)
        {
            if (GameInfo.aClientList[iGameClientIdx] != 0)
            {
                if ((pGameClient = VoipTunnelClientListMatchId(pVoipTunnel, GameInfo.aClientList[iGameClientIdx])) != NULL)
                {
                    char strAddrText[20];
                    uint32_t uAddress = pGameClient->uRemoteAddr;
                    uint16_t uPort = pGameClient->uRemoteGamePort;
                    struct sockaddr ClientAddr;
                    if (ProtoTunnelStatus(pProtoTunnel, 'vtop', pGameClient->uRemoteAddr, &ClientAddr, sizeof(ClientAddr)) != 0)
                    {
                        uAddress = SockaddrInGetAddr(&ClientAddr);
                        uPort = SockaddrInGetPort(&ClientAddr);
                    }

                    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       [%02d] client id=0x%08x flags=0x%08x addr=%s:%d\n", iGameClientIdx, pGameClient->uClientId, pGameClient->uFlags, SocketInAddrGetText(uAddress, strAddrText, sizeof(strAddrText)), uPort);
                    iNumGameClients += 1;
                }
                else
                {
                    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       [%02d] ghost client id=0x%08x\n", iGameClientIdx, GameInfo.aClientList[iGameClientIdx]);
                }
            }
        }

        // validate client counts
        if (iNumGameClients != GameInfo.iNumClients)
        {
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "WARNING: ghost clients detected. (%d != %d)\n", iNumGameClients, GameInfo.iNumClients);
        }

        // get elapsed time in minutes and seconds
        uSeconds = (uint32_t)difftime(time(NULL), pGameServer->GameStatHistory[0].uGameStart);
        uMinutes = uSeconds/60;
        uSeconds -= uMinutes*60;

        // display verbose connection stats
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "         elapsed: %3dm %2ds\n", uMinutes, uSeconds);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "            peak: %5db/s up, %5db/s dn\n",
            (pGameServer->GameStatsMax.GameStat.uByteRecv+pGameServer->GameStatsMax.VoipStat.uByteRecv)/iStatRate,
            (pGameServer->GameStatsMax.GameStat.uByteSent+pGameServer->GameStatsMax.VoipStat.uByteSent)/iStatRate);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "            game: %5db/s up, %5db/s dn\n", pGameServer->GameStatsCur.GameStat.uByteRecv/iStatRate, pGameServer->GameStatsCur.GameStat.uByteSent/iStatRate);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "            voip: %5db/s up, %5db/s dn\n", pGameServer->GameStatsCur.VoipStat.uByteRecv/iStatRate, pGameServer->GameStatsCur.VoipStat.uByteSent/iStatRate);

        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       peak game: %5db/s up, %5db/s dn\n", pGameServer->GameStatsMax.GameStat.uByteRecv/iStatRate, pGameServer->GameStatsMax.GameStat.uByteSent/iStatRate);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       peak voip: %5db/s up, %5db/s dn\n", pGameServer->GameStatsMax.VoipStat.uByteRecv/iStatRate, pGameServer->GameStatsMax.VoipStat.uByteSent/iStatRate);

        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "      total game:  %5dkb up,  %5dkb dn\n", pGameServer->GameStatsSum.GameStat.uByteRecv/1024, pGameServer->GameStatsSum.GameStat.uByteSent/1024);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "      total voip:  %5dkb up,  %5dkb dn\n", pGameServer->GameStatsSum.VoipStat.uByteRecv/1024, pGameServer->GameStatsSum.VoipStat.uByteSent/1024);

        #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "    voip dropped: %5d packets\n", GameInfo.uVoiceDataDropMetric);
        #else
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "    voip dropped: %5d packets\n", GameInfo.VoipSessionInfo.uVoiceDataDropMetric);
        #endif
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDiagnosticGameServerHistory

    \Description
        Format gameserver history info for diagnostic display page.

    \Input *pVoipServer         - voipserver ref
    \Input iGameServer          - gameserver index
    \Input *pBuffer             - output buffer
    \Input iBufSize             - size of output buffer

    \Output
        int32_t                 - number of bytes written to output buffer

    \Version 04/06/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerDiagnosticGameServerHistory(VoipGameServerRefT *pVoipServer, int32_t iGameServer, char *pBuffer, int32_t iBufSize)
{
    int32_t iGameCount, iGameHistory, iOffset = 0;
    GameServerStatHistoryT *pGameHistory;
    const int32_t iStatRate = 15;
    uint32_t uSeconds, uMinutes;
    char strTime[32];

    // ref gameserver
    GameServerT *pGameServer = VoipGameServerMatchIndex(pVoipServer, iGameServer);

    // display recent games summary, if there are any
    if (pGameServer->uTotalGames > 0)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "    game history:\n");

        for (iGameHistory = 0, iGameCount = 0; iGameHistory < VOIPSERVER_GAMEHISTORYCNT; iGameHistory++)
        {
            pGameHistory = &pGameServer->GameStatHistory[iGameHistory];
            if ((pGameServer->GameStatHistory[iGameHistory].uGameStart == 0) || (pGameServer->GameStatHistory[iGameHistory].uGameEnd == 0))
            {
                continue;
            }

            // get elapsed time in minutes and seconds
            uSeconds = (uint32_t)difftime(pGameHistory->uGameEnd, pGameHistory->uGameStart);
            uMinutes = uSeconds/60;
            uSeconds -= uMinutes*60;

            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "          game %d: started: %24s, elapsed: %3d:%02d, peak: %5db/s up %5db/s dn, total: %5dkb up %5dkb dn, rlmt: %10d\n",
                iGameCount++, DirtyCastCtime(strTime, sizeof(strTime), pGameHistory->uGameStart), uMinutes, uSeconds,
                (pGameHistory->GameStatsMax.GameStat.uByteRecv+pGameHistory->GameStatsMax.VoipStat.uByteRecv)/iStatRate,
                (pGameHistory->GameStatsMax.GameStat.uByteSent+pGameHistory->GameStatsMax.VoipStat.uByteSent)/iStatRate,
                (pGameHistory->GameStatsSum.GameStat.uByteRecv+pGameHistory->GameStatsSum.VoipStat.uByteRecv)/1024,
                (pGameHistory->GameStatsSum.GameStat.uByteSent+pGameHistory->GameStatsSum.VoipStat.uByteSent)/1024,
                pGameHistory->uRlmtChangeCount);
        }
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDiagnosticGameServer

    \Description
        Callback to format gameserver stats for diagnostic request.

    \Input *pServerDiagnostic - server diagnostic module
    \Input *pVoipServer     - voipserver state
    \Input iGameServer      - gameserver to display status of
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input uCurTime         - current time
    \Input bList            - true if we're in list format, else false

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerDiagnosticGameServer(ServerDiagnosticT *pServerDiagnostic, VoipGameServerRefT *pVoipGameServer, int32_t iGameServer, char *pBuffer, int32_t iBufSize, time_t uCurTime, uint8_t bList)
{
    VoipServerRefT *pVoipServer = VoipGameServerGetBase(pVoipGameServer);
    BufferedSocketInfoT BSocInfo;
    int32_t iNumClients, iOffset;
    uint32_t uGameServerAddr, uValue;
    const char *_strStates[] = { "offline", "available", "active", "zombie" }, *pStrState;
    char strState[48], strAddrText[20];
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    // ref gameserver at given index
    GameServerT *pGameServer = VoipGameServerMatchIndex(pVoipGameServer, iGameServer);

    // if no gameserver connected, report that and return
    if (pGameServer->bConnected == FALSE)
    {
        // display info summary
        iOffset = ds_snzprintf(pBuffer, iBufSize, "gameserver %2d: state=disconnected\n", iGameServer);
        return(iOffset);
    }

    // get number of users active on this GameServer
    iNumClients = VoipTunnelStatus(pVoipTunnel, 'nusr', iGameServer, NULL, 0);

    // determine gameserver address
    uGameServerAddr = SockaddrInGetAddr(&pGameServer->Address);
    if (uGameServerAddr == (unsigned)SocketInTextGetAddr("127.0.0.1"))
    {
        uGameServerAddr = pConfig->uFrontAddr;
    }

    // figure out state string (including "stuck", which is not a lobby state)
    pStrState = pGameServer->bGameStuck ? "stuck" : _strStates[pGameServer->eLobbyState];

    // if active, calculate elapsed time
    if (pGameServer->eLobbyState == kActive)
    {
        uint32_t uSeconds, uMinutes;

        // get elapsed time in minutes and seconds
        uSeconds = (uint32_t)difftime(time(NULL), pGameServer->GameStatHistory[0].uGameStart);
        uMinutes = uSeconds/60;
        uSeconds -= uMinutes*60;

        // display verbose connection stats
        ds_snzprintf(strState, sizeof(strState), "state=%s, elapsed= %3dm %2ds", pStrState, uMinutes, uSeconds);
    }
    else
    {
        ds_snzprintf(strState, sizeof(strState), "state=%s", pStrState);
    }

    // display info summary
    if (bList)
    {
        iOffset  = ds_snzprintf(pBuffer, iBufSize, "<a href=\"http://%s:%d/gameserver=%d\">gameserver %2d</a>: %2d %s %4d games played, %s, total rlmt: %d\n",
            SocketInAddrGetText(pConfig->uFrontAddr, strAddrText, sizeof(strAddrText)), pConfig->uDiagnosticPort,
            iGameServer, iGameServer, iNumClients, iNumClients==1?"client, ":"clients,", pGameServer->uTotalGames, strState, pGameServer->uTotalRlmtChangeCount);
    }
    else
    {
        iOffset  = ds_snzprintf(pBuffer, iBufSize, "<a href=\"http://%s:%d/\">gameserver %2d</a>: %2d %s %4d games played, %s, total rlmt: %d\n",
            SocketInAddrGetText(uGameServerAddr, strAddrText, sizeof(strAddrText)), pGameServer->uDiagnosticPort, iGameServer,
            iNumClients, iNumClients==1?"client, ":"clients,", pGameServer->uTotalGames, strState, pGameServer->uTotalRlmtChangeCount);
    }

    // display buffered socket info
    if ((pGameServer->pBufferedSocket != NULL) && (BufferedSocketInfo(pGameServer->pBufferedSocket, 'bnfo', 0, &BSocInfo, sizeof(BSocInfo)) == 0))
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bsockinfo: read=%u recv=%u rbyte=%u writ=%u sent=%u sbyte=%u maxr=%u maxs=%u\n",
            BSocInfo.uNumRead, BSocInfo.uNumRecv, BSocInfo.uDatRecv,
            BSocInfo.uNumWrit, BSocInfo.uNumSent, BSocInfo.uDatSent,
            BSocInfo.uMaxRBuf, BSocInfo.uMaxSBuf);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bsockinfo: socket recv=%u, max socket recv=%u, socket send=%u, max socket send=%u\n", BSocInfo.uRcvQueue, BSocInfo.uMaxRQueue,
            BSocInfo.uSndQueue, BSocInfo.uMaxSQueue);
        if ((BSocInfo.uNumWrit > 0) && ((uValue = BSocInfo.uWriteDelay / BSocInfo.uNumWrit) > 0))
        {
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bsock warning: flush latency %dms\n", uValue);
        }
    }
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDiagnosticGameServers

    \Description
        Callback to format voipserver stats for diagnostic request.

    \Input *pServerDiagnostic - server diagnostic module
    \Input *pVoipServer     - voipserver state
    \Input iFirstGameServer - first gameserver to start display from
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input uCurTime         - current time
    \Input uDiagStatus      - VOIPSERVERDIAGNOSTIC_STATUS_* flags
    \Input *pUrl            - pointer to base url

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerDiagnosticGameServers(ServerDiagnosticT *pServerDiagnostic, VoipGameServerRefT *pVoipGameServer, int32_t iFirstGameServer, char *pBuffer, int32_t iBufSize, time_t uCurTime, uint8_t uDiagStatus, const char *pUrl)
{
    int32_t iDisplayCount, iGameServer, iOffset = 0;
    GameServerT *pGameServer;
    uint32_t uStatus;
    VoipServerRefT *pVoipServer = VoipGameServerGetBase(pVoipGameServer);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    // generic GameServer output for list of gameservers
    for (iGameServer = iFirstGameServer, iDisplayCount = 0, uStatus = 0; iGameServer < pConfig->iMaxGameServers; iGameServer++)
    {
        // cap results
        if (iDisplayCount == 16)
        {
            char strAddrText[20];
            // locate params, if any
            pUrl = strchr(pUrl, '?');

            // add a link to the next set of results
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<a href=\"http://%s:%d/gameservers=%d%s\">next</a>\n",
                SocketInAddrGetText(pConfig->uFrontAddr, strAddrText, sizeof(strAddrText)),
                pConfig->uDiagnosticPort, iGameServer, pUrl ? pUrl : "");
            break;
        }

        // ref GameServer
        pGameServer = VoipGameServerMatchIndex(pVoipGameServer, iGameServer);

        // apply "disconnected" filter
        uStatus  = (((uDiagStatus & VOIPSERVERDIAGNOSTIC_STATUS_DISCONNECTED) != 0) && (pGameServer->bConnected == FALSE));

        // apply other filters if connected
        if (pGameServer->bConnected == TRUE)
        {
            // apply "offline" filter
            uStatus |= (((uDiagStatus & VOIPSERVERDIAGNOSTIC_STATUS_OFFLINE) != 0) && (pGameServer->eLobbyState == kOffline));
            // apply "available" filter
            uStatus |= (((uDiagStatus & VOIPSERVERDIAGNOSTIC_STATUS_AVAILABLE) != 0) && (pGameServer->eLobbyState == kAvailable));
            // apply "active" filter
            uStatus |= (((uDiagStatus & VOIPSERVERDIAGNOSTIC_STATUS_ACTIVE) != 0) && (pGameServer->eLobbyState == kActive));
            // apply "stuck" filter
            uStatus |= (((uDiagStatus & VOIPSERVERDIAGNOSTIC_STATUS_STUCK) != 0) && (pGameServer->bGameStuck == TRUE));
            // apply "zombie" filter
            uStatus |= (((uDiagStatus & VOIPSERVERDIAGNOSTIC_STATUS_ZOMBIE) != 0) && (pGameServer->eLobbyState == kZombie));
        }

        if (uStatus == 0)
        {
            continue;
        }

        // display GameServer and increment displayed count
        iOffset += _VoipServerDiagnosticGameServer(pServerDiagnostic, pVoipGameServer, iGameServer, pBuffer+iOffset, iBufSize-iOffset, uCurTime, TRUE);
        iDisplayCount += 1;
    }
    // return offset
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDiagnosticStatus

    \Description
        Callback to format voipserver stats for diagnostic request.

    \Input *pVoipServer     - voipserver module state
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pUserData       - VoipServer module state

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerDiagnosticStatus(VoipGameServerRefT *pVoipGameServer, char *pBuffer, int32_t iBufSize, time_t uCurTime)
{
    VoipServerRefT *pVoipServer = VoipGameServerGetBase(pVoipGameServer);
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    int32_t iNumGameServers = VoipGameServerStatus(pVoipGameServer, 'ngam', 0, NULL, 0);
    int32_t iUsersInGames = VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0);
    int32_t iGamesActive = VoipTunnelStatus(pVoipTunnel, 'ngam', 0, NULL, 0);
    VoipServerProcessStateE eProcessState = VoipServerStatus(pVoipServer, 'psta', 0, NULL, 0);

    int32_t iOffset = 0;
    VoipGameServerStatT VoipGameServerStats;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    // get voipgameserver stats
    VoipGameServerStatus(pVoipGameServer, 'stat', 0, &VoipGameServerStats, sizeof(VoipGameServerStats));

    // format the shared information from the common diagnostics
    iOffset += CommonDiagnosticStatus(pVoipServer, pBuffer+iOffset, iBufSize-iOffset);
    
    // process state
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "process state: %s\n", _VoipServer_strProcessStates[eProcessState]);

    // format the gameserver specific information
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "gameservers connected: %d/%d (%d %s with %2.1f users/game, %d available, %d offline, %d active, %d zombie, %d stuck)\n",
        iNumGameServers, pConfig->iMaxGameServers, iGamesActive,
        (iGamesActive == 1) ? "game" : "games",
        (iGamesActive > 0) ? (float)iUsersInGames/(float)iGamesActive : 0.0f,
        VoipGameServerStats.iGamesAvailable, VoipGameServerStats.iGamesOffline, VoipGameServerStats.iGamesActive, VoipGameServerStats.iGamesZombie, VoipGameServerStats.iGamesStuck);
    if (VoipGameServerStats.uLastGameServDiscTime != 0)
    {
        char strTime[32];
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "gameserver disconnections: %d (last %s)\n", VoipGameServerStats.uGameServDiscCount,
            DirtyCastCtime(strTime, sizeof(strTime), VoipGameServerStats.uLastGameServDiscTime));
    }
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "gameserver disconnections: %d\n", VoipGameServerStats.uGameServDiscCount);
    }

    // return updated buffer size to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerDiagnosticGameStats

    \Description
        Callback to format client statistics

    \Input *pVoipServer     - voipserver module state
    \Input bReset           - true=reset stats, else false
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *uCurTime        - current time

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 09/30/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipServerDiagnosticGameStats(VoipGameServerRefT *pVoipGameServer, uint8_t bReset, char *pBuffer, int32_t iBufSize, time_t uCurTime)
{
    static const char *_pStrGameStateNames[GAMESERVER_NUMGAMESTATES] = { "PREGAME", "INGAME", "POSTGAME", "MIGRATING", "OTHER" };
    VoipServerRefT *pVoipServer = VoipGameServerGetBase(pVoipGameServer);
    uint32_t aSumLeaveReasons[VOIPSERVER_MAXCLIENTREMOVALINFO], uNumReasons;
    GameServerClientRemovalInfoT *pRemovalInfo;
    uint32_t uIndex, uTotalUsers, uTotalGames;
    VoipGameServerStatT VoipGameServerStats;
    VoipServerStatT Stats;
    int32_t iOffset = 0;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    int32_t iUsersInGames = VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0);
    int32_t iGamesActive = VoipTunnelStatus(pVoipTunnel, 'ngam', 0, NULL, 0);

    // let the common diagnostic do it's thing
    iOffset += CommonDiagnosticGameStats(pVoipServer, bReset, pBuffer, iBufSize);

    // tell module to reset game stats
    if (bReset)
    {
        VoipGameServerControl(pVoipGameServer, 'gsta', 0, 0, NULL);
    }

    // get voipserver stats
    VoipGameServerStatus(pVoipGameServer, 'stat', 0, &VoipGameServerStats, sizeof(VoipGameServerStats));
    VoipServerStatus(pVoipServer, 'stat', 0, &Stats, sizeof(Stats));

    // total number of users tracked is clients added minus current number of clients in game (who have not left yet)
    uTotalUsers = Stats.uTotalClientsAdded - (uint32_t)iUsersInGames;
    uTotalGames = Stats.uTotalGamesCreated - (uint32_t)iGamesActive;

    // generate some summary statistics
    for (uIndex = 0, uNumReasons = 0; uIndex < VOIPSERVER_MAXCLIENTREMOVALINFO; uIndex += 1)
    {
        pRemovalInfo = &VoipGameServerStats.aClientRemovalInfo[uIndex];
        if (pRemovalInfo->RemovalReason.strRemovalReason[0] != '\0')
        {
            uint32_t uIndex2;
            for (uIndex2 = 0, aSumLeaveReasons[uIndex] = 0; uIndex2 < GAMESERVER_NUMGAMESTATES; uIndex2 += 1)
            {
                aSumLeaveReasons[uIndex] += pRemovalInfo->aRemovalStateCt[uIndex2];
            }
            uNumReasons += 1;
        }
    }


    // print client removal reason summary and stats header
    if (uNumReasons > 0)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "client removal summary\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%6s %5s %7s %6s %8s %9s %6s  %8s  %s\n", "TOTAL", "PCT",
            _pStrGameStateNames[GAMESERVER_GAMESTATE_PREGAME], _pStrGameStateNames[GAMESERVER_GAMESTATE_INGAME],
            _pStrGameStateNames[GAMESERVER_GAMESTATE_POSTGAME], _pStrGameStateNames[GAMESERVER_GAMESTATE_MIGRATING],
            _pStrGameStateNames[GAMESERVER_GAMESTATE_OTHER], "REASONCODE", "REASON");

        // print client removal reason breakdown;
        for (uIndex = 0; uIndex < VOIPSERVER_MAXCLIENTREMOVALINFO; uIndex += 1)
        {
            pRemovalInfo = &VoipGameServerStats.aClientRemovalInfo[uIndex];
            if (pRemovalInfo->RemovalReason.strRemovalReason[0] != '\0')
            {
                char strPct[6];
                ds_snzprintf(strPct, sizeof(strPct), "%3.1f", ((float)aSumLeaveReasons[uIndex] * 100.0f) / (float)uTotalUsers);
                iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%6d %5s %7d %6d %8d %9d %6d  0x%08x  %s\n",
                    aSumLeaveReasons[uIndex], strPct,
                    pRemovalInfo->aRemovalStateCt[GAMESERVER_GAMESTATE_PREGAME], pRemovalInfo->aRemovalStateCt[GAMESERVER_GAMESTATE_INGAME],
                    pRemovalInfo->aRemovalStateCt[GAMESERVER_GAMESTATE_POSTGAME], pRemovalInfo->aRemovalStateCt[GAMESERVER_GAMESTATE_MIGRATING],
                    pRemovalInfo->aRemovalStateCt[GAMESERVER_GAMESTATE_OTHER], pRemovalInfo->RemovalReason.uRemovalReason,
                    pRemovalInfo->RemovalReason.strRemovalReason);
            }
        }
    }

    if (uTotalGames > 0)
    {
        char strPct[6];
        uint32_t uState;

        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "game end summary\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%6s %5s %s\n", "TOTAL", "PCT", "ENDSTATE");
        for (uState = 0; uState < GAMESERVER_NUMGAMESTATES; uState += 1)
        {
            ds_snzprintf(strPct, sizeof(strPct), "%3.1f", ((float)VoipGameServerStats.aGameEndStateCt[uState] * 100.0f) / (float)uTotalGames);
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%6d %5s %s\n", VoipGameServerStats.aGameEndStateCt[uState], strPct, _pStrGameStateNames[uState]);
        }
    }

    // return updated buffer size to caller
    return(iOffset);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function GameServerDiagnosticCallback

    \Description
        Callback to format voipserver stats for diagnostic request.

    \Input *pServerDiagnostic - server diagnostic module state
    \Input *pUrl            - request url
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pUserData       - VoipServer module state

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t GameServerDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData)
{
    VoipGameServerRefT *pVoipGameServer = (VoipGameServerRefT *)pUserData;
    VoipServerRefT *pVoipServer = VoipGameServerGetBase(pVoipGameServer);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    time_t uCurTime = time(NULL);
    int32_t iOffset = 0;
    char strTime[32];

    // display server time for all requests except for metrics requests
    if (strncmp(pUrl, "/metrics", 8))
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server time: %s\n", DirtyCastCtime(strTime, sizeof(strTime), uCurTime));
    }

    // act based on request type
    if (!strncmp(pUrl, "/status", 7))
    {
        iOffset += _VoipServerDiagnosticStatus(pVoipGameServer, pBuffer+iOffset, iBufSize-iOffset, uCurTime);
    }
    else if (!strncmp(pUrl, "/gameservers", 12))
    {
        int32_t iGameServer = (pUrl[12] == '=') ? strtol(&pUrl[13], NULL, 10) : 0;

        if ((iGameServer >= 0) && (iGameServer < pConfig->iMaxGameServers))
        {
            uint32_t uDiagStatus = VOIPSERVERDIAGNOSTIC_STATUS_ALL;
            const char *pUrlStatus = strstr(pUrl, "status=");

            // status override?
            if (pUrlStatus != NULL)
            {
                uDiagStatus = 0;
                uDiagStatus |= strstr(pUrlStatus, "disconnected") ? VOIPSERVERDIAGNOSTIC_STATUS_DISCONNECTED : 0;
                uDiagStatus |= strstr(pUrlStatus, "offline") ? VOIPSERVERDIAGNOSTIC_STATUS_OFFLINE : 0;
                uDiagStatus |= strstr(pUrlStatus, "available") ? VOIPSERVERDIAGNOSTIC_STATUS_AVAILABLE : 0;
                uDiagStatus |= strstr(pUrlStatus, "active") ? VOIPSERVERDIAGNOSTIC_STATUS_ACTIVE : 0;
                uDiagStatus |= strstr(pUrlStatus, "stuck") ? VOIPSERVERDIAGNOSTIC_STATUS_STUCK : 0;
                uDiagStatus |= strstr(pUrlStatus, "zombie") ? VOIPSERVERDIAGNOSTIC_STATUS_ZOMBIE : 0;
            }

            iOffset += _VoipServerDiagnosticGameServers(pServerDiagnostic, pVoipGameServer, iGameServer, pBuffer+iOffset, iBufSize-iOffset, uCurTime, uDiagStatus, pUrl);
        }
        else
        {
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "invalid gameserver %d\n", iGameServer);
        }
    }
    else if (!strncmp(pUrl, "/gameserver", 11))
    {
        int32_t iGameServer = (pUrl[11] == '=') ? strtol(&pUrl[12], NULL, 10) : 0;
        if ((iGameServer >= 0) && (iGameServer < pConfig->iMaxGameServers))
        {
            iOffset += _VoipServerDiagnosticGameServer(pServerDiagnostic, pVoipGameServer, iGameServer, pBuffer+iOffset, iBufSize-iOffset, uCurTime, FALSE);
            iOffset += _VoipServerDiagnosticGameServerCurGame(pVoipGameServer, iGameServer, pBuffer+iOffset, iBufSize-iOffset);
            iOffset += _VoipServerDiagnosticGameServerHistory(pVoipGameServer, iGameServer, pBuffer+iOffset, iBufSize-iOffset);
        }
        else
        {
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "invalid gameserver %d\n", iGameServer);
        }
    }
    else if (!strncmp(pUrl, "/gamestats", 10))
    {
        iOffset += _VoipServerDiagnosticGameStats(pVoipGameServer, strstr(pUrl, "?reset") != NULL, pBuffer+iOffset, iBufSize-iOffset, uCurTime);
    }
    else if (!strncmp(pUrl, "/servulator", 11))
    {
        iOffset += CommonDiagnosticServulator(pVoipServer, "GameServer", pConfig->uTunnelPort, VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (!strncmp(pUrl, "/monitor", 8))
    {
        iOffset += CommonDiagnosticMonitor(pVoipServer, _MonitorDiagnostics, DIRTYCAST_CalculateArraySize(_MonitorDiagnostics), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (!strncmp(pUrl, "/system", 7))
    {
        iOffset += CommonDiagnosticSystem(pVoipServer, pConfig->uTunnelPort, pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (!strncmp(pUrl, "/action", 7))
    {
        iOffset += CommonDiagnosticAction(pVoipServer, strstr(pUrl, "/action"), pBuffer+iOffset, iBufSize-iOffset);
    }
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "unknown request type\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"gameserver\">/gameserver</a>=[server] -- display info for specified gameserver\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       <a href=\"gameservers\">/gameservers</a>=[first] -- display status summary for up to sixteen gameservers, starting with first\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "                   ?refresh=[interval] -- set auto-refresh in seconds\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "                   ?status=[disconnected|offline|active|available|stuck] -- filter output based on status\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       <a href=\"gamestats\">/gamestats</a>             -- display voipserver metrics\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "                   ?reset -- reset stats\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       <a href=\"metrics\">/metrics</a>             -- display voipserver metrics\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       <a href=\"monitor\">/monitor</a>             -- display voipserver monitor status\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       <a href=\"status\">/status</a>               -- display voipserver status\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       <a href=\"system\">/system</a>               -- display system status\n");
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function GameServerDiagnosticGetResponseTypeCb

    \Description
        Get response type based on specified url

    \Input *pServerDiagnostic           - server diagnostic module state
    \Input *pUrl                        - request url

    \Output
        ServerDiagnosticResponseTypeE   - SERVERDIAGNOSTIC_RESPONSETYPE_*

    \Version 10/13/2010 (jbrookes)
*/
/********************************************************************************F*/
ServerDiagnosticResponseTypeE GameServerDiagnosticGetResponseTypeCb(ServerDiagnosticT *pServerDiagnostic, const char *pUrl)
{
    ServerDiagnosticResponseTypeE eResponseType = SERVERDIAGNOSTIC_RESPONSETYPE_DIAGNOSTIC;
    if (!strncmp(pUrl, "/metrics", 8))
    {
        eResponseType = SERVERDIAGNOSTIC_RESPONSETYPE_XML;
    }
    return(eResponseType);
}
