/*H********************************************************************************/
/*!
    \File commondiagnostic.c

    \Description
        This module implements common diagnostic page formatting for the VoipServer

    \Copyright
        Copyright (c) 2018 Electronic Arts Inc.

    \Version 02/26/2018 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/voip/voiptunnel.h"
#include "LegacyDirtySDK/util/tagfield.h"

#include "serveraction.h"
#include "serverdiagnostic.h"
#include "voipserver.h"
#include "voipserverconfig.h"
#include "udpmetrics.h"
#include "commondiagnostic.h"

/*** Private functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _CommonDiagnosticLateStatus

    \Description
        Callback to format voipserver late stats for diagnostic request.

    \Input *pLateStat       - late stat to output
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pText           - header text for style 0
    \Input uStyle           - style (0=status, 1=metrics)

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 04/09/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CommonDiagnosticLateStatus(const GameServerLateT *pLateStat, char *pBuffer, int32_t iBufSize, const char *pText, uint32_t uStyle)
{
    uint32_t uAvgLate;
    int32_t iOffset = 0;

    // calc average latency
    uAvgLate = (pLateStat->uNumStat != 0) ? pLateStat->uSumLate/pLateStat->uNumStat : 0;

    // print latency stats
    if (uStyle == 0)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%s: (%d samples)\n", pText, pLateStat->uNumStat);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "    min: %3dms\n", pLateStat->uMinLate);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "    max: %3dms\n", pLateStat->uMaxLate);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "    avg: %3dms\n", uAvgLate);
    }
    else if (uStyle == 1)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "    <%s unit=\"ms\" games=\"%d\" min=\"%d\" max=\"%d\" avg=\"%d\" />\n", pText,
            0, pLateStat->uMinLate, pLateStat->uMaxLate, uAvgLate);
    }

    // return updated buffer size to caller
    return(iOffset);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CommonDiagnosticServulator

    \Description
        Formats the servlator information that shows up in the GUI

    \Input *pVoipServer     - voipserver module state
    \Input *pMode           - which mode is the application running in
    \Input uPort            - port we are serving on
    \Input iPlayers         - players stat which differs based on mode
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 02/26/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CommonDiagnosticServulator(VoipServerRefT *pVoipServer, const char *pMode, uint32_t uPort, int32_t iPlayers, char *pBuffer, int32_t iBufSize)
{
    int32_t iOffset = 0;

    // start filling in the message
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<servulator>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Init>true</Init>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Name>Voip Server (%s)</Name>\n", pMode);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Version>%s</Version>\n", _SDKVersion);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Port>%u</Port>\n", uPort);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <State>na</State>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <GameMode>na</GameMode>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Ranked>na</Ranked>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Private>na</Private>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Players>%d</Players>\n", iPlayers);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Observers>0</Observers>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <PlayerList>na</PlayerList>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <ObserverList>na</ObserverList>\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</servulator>\n");

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function CommonDiagnosticAction

    \Description
        Take an action based on specified command

    \Input *pVoipServer     - voipserver module state
    \Input *pStrAction      - action string
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 02/26/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CommonDiagnosticAction(VoipServerRefT *pVoipServer, const char *pStrCommand, char *pBuffer, int32_t iBufSize)
{
    ServerActionT ServerAction;
    int32_t iOffset = 0;

    // parse action and value
    if (ServerActionParse(pStrCommand, &ServerAction) < 0)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "unable to parse action\n");
        return(iOffset);
    }

    if (strncmp(ServerAction.strName, "debuglevel", 10) == 0)
    {
        int32_t iDebugLevel;
        iOffset += ServerActionParseInt(pBuffer, iBufSize, &ServerAction, 0, 10, &iDebugLevel);
        VoipServerControl(pVoipServer, 'spam', iDebugLevel, 0, NULL);
    }
    else if (strncmp(ServerAction.strName, "state", 5) == 0)
    {
        LobbyStateE eState;
        ServerActionEnumT StateEnum[] = { { "offline", 0 }, { "available", 1 }, { "active", 2 }, { "zombie", 3 } };
        iOffset += ServerActionParseEnum(pBuffer, iBufSize, &ServerAction, StateEnum, sizeof(StateEnum) / sizeof(StateEnum[0]), (int32_t*)&eState);
        
        if (eState == kZombie)
        {
            VoipServerControl(pVoipServer, 'asdz', TRUE, 0, NULL);
        }
#if DIRTYCODE_DEBUG
        else if (eState == kActive)
        {
            VoipServerControl(pVoipServer, 'asac', TRUE, 0, NULL);
        }
        else if (eState == kAvailable)
        {
            VoipServerControl(pVoipServer, 'asac', FALSE, 0, NULL);
        }
#endif

    }
    else if (strncmp(ServerAction.strName, "shutdown", 8) == 0)
    {
        int32_t iSignalFlags;
        ServerActionEnumT ShutdownEnum[] = { { "immediate", VOIPSERVER_SIGNAL_SHUTDOWN_IMMEDIATE }, { "graceful", VOIPSERVER_SIGNAL_SHUTDOWN_IFEMPTY } };
        iOffset += ServerActionParseEnum(pBuffer, iBufSize, &ServerAction, ShutdownEnum, sizeof(ShutdownEnum)/sizeof(ShutdownEnum[0]), &iSignalFlags);
        VoipServerControl(pVoipServer, 'sgnl', iSignalFlags, 0, NULL);
    }
#if DIRTYCODE_DEBUG
    else if (strncmp(ServerAction.strName, "fake_pctserverload", 18) == 0)
    {
        uint32_t u4ByteCode = 0xFFFFFFFF;

        if (strcmp(ServerAction.strData, "off") == 0)
        {
            u4ByteCode = 0;
        }
        else if (strcmp(ServerAction.strData, "normal") == 0)
        {
            u4ByteCode = 'norm';
        }
        else if (strcmp(ServerAction.strData, "warning") == 0)
        {
            u4ByteCode = 'warn';
        }
        else if (strcmp(ServerAction.strData, "error") == 0)
        {
            u4ByteCode = 'erro';
        }

        if (u4ByteCode != 0xFFFFFFFF)
        {
            VoipServerControl(pVoipServer, 'fpct', u4ByteCode, 0, NULL);

            // print the action name and result
            iOffset = ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "%s set to %s\n", ServerAction.strName, ServerAction.strData);
            // also log action to debug output
            DirtyCastLogPrintf("serveraction: %s=%s\n", ServerAction.strName, ServerAction.strData);
        }
        else
        {
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "%s is not valid with action %s\n", ServerAction.strData, ServerAction.strName);
        }
    }
    else if (strncmp(ServerAction.strName, "fake_cpu", 8) == 0)
    {
        int32_t iFakeCpu = -2;
        iOffset += ServerActionParseInt(pBuffer, iBufSize, &ServerAction, -1, 100, &iFakeCpu);

        if ((iFakeCpu >= -1) && (iFakeCpu <= 100))
        {
            VoipServerControl(pVoipServer, 'fcpu', iFakeCpu, 0, NULL);
        }
        else
        {
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "%s is out of validity range [-1,100] for action %s\n", ServerAction.strData, ServerAction.strName);
        }
    }
    else if (strncmp(ServerAction.strName, "fake_crash", 10) == 0)
    {
        int32_t *pCrash = NULL;
        int32_t iCrash = 0;

        DirtyCastLogPrintf("serveraction: %s\n", ServerAction.strName);

        iCrash = *pCrash;

        // this point should never be reached as previous line should throw a "read access violation" exception
    }
#endif
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "action '%s' is not recognized\n", ServerAction.strName);
    }

    // return updated buffer size to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function CommonDiagnosticGameStats

    \Description
        Callback to format client statistics

    \Input *pVoipServer     - voipserver module state
    \Input bReset           - true=reset stats, else false
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 02/26/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CommonDiagnosticGameStats(VoipServerRefT *pVoipServer, uint8_t bReset, char *pBuffer, int32_t iBufSize)
{
    int32_t iOffset = 0;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    VoipServerStatT Stats;
    uint32_t uTotalUsers, uTotalGames;

    // tell module to set game stats
    if (bReset == TRUE)
    {
        VoipServerControl(pVoipServer, 'gsta', 0, 0, NULL);
    }
    VoipServerStatus(pVoipServer, 'stat', 0, &Stats, sizeof(Stats));

    // total number of users tracked is clients added minus current number of clients in game (who have not left yet)
    uTotalUsers = (uint32_t)Stats.uTotalClientsAdded - (uint32_t)VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0);
    uTotalGames = (uint32_t)Stats.uTotalGamesCreated - (uint32_t)VoipTunnelStatus(pVoipTunnel, 'ngam', 0, NULL, 0);

    // print overall stats
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "clients added: %d\n", uTotalUsers);
    // print game statistics
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "games created: %d\n", uTotalGames);

    // return updated buffer size to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function CommonDiagnosticSystem

    \Description
        Callback to format voipserver stats for system diagnostic request

    \Input *pVoipServer     - voipserver module state
    \Input uPort            - port we using to do work (probe or tunnel)
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 02/26/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t CommonDiagnosticSystem(VoipServerRefT *pVoipServer, uint32_t uPort, char *pBuffer, int32_t iBufSize)
{
    VoipServerStatT VoipServerStats;
    UdpMetricsT UdpMetrics;
    uint32_t uHours, uMinutes, uSeconds;
    int32_t iOffset = 0;

    // gets stats from base module
    VoipServerStatus(pVoipServer, 'stat', 0, &VoipServerStats, sizeof(VoipServerStats));
    VoipServerStatus(pVoipServer, 'udpm', 0, &UdpMetrics, sizeof(UdpMetrics));

    uSeconds = VoipServerStats.ServerInfo.uTime / 1000;
    uHours = uSeconds/(60*60);
    uSeconds -= uHours*60*60;
    uMinutes = uSeconds/60;
    uSeconds -= uMinutes*60;

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server load: %%cpu=%0.2f time=%02d:%02d:%02d\n",
        VoipServerStats.ServerInfo.fCpu, uHours, uMinutes, uSeconds);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "system load average: %d.%02d %d.%02d %d.%02d (%d cpu cores)\n",
        VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[0]), VOIPSERVER_LoadFrac(VoipServerStats.ServerInfo.aLoadAvg[0]),
        VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[1]), VOIPSERVER_LoadFrac(VoipServerStats.ServerInfo.aLoadAvg[1]),
        VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[2]), VOIPSERVER_LoadFrac(VoipServerStats.ServerInfo.aLoadAvg[2]),
        VoipServerStats.ServerInfo.iNumCPUCores);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server memory: total=%u free=%u shared=%u buffer=%u\n",
        VoipServerStats.ServerInfo.uTotalRam, VoipServerStats.ServerInfo.uFreeRam, VoipServerStats.ServerInfo.uSharedRam, VoipServerStats.ServerInfo.uBufferRam);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server memory: totalhigh=%u freehigh=%u\n",
        VoipServerStats.ServerInfo.uTotalHighMem, VoipServerStats.ServerInfo.uFreeHighMem);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server swap: total=%u free=%u\n",
        VoipServerStats.ServerInfo.uTotalSwap, VoipServerStats.ServerInfo.uFreeSwap);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server blocksize: %u\n",
        VoipServerStats.ServerInfo.uMemBlockSize);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server procs: %u\n",
        VoipServerStats.ServerInfo.uNumProcs);

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "udp queue lengths: send=%u recv=%u\n", UdpMetrics.uSendQueueLen, UdpMetrics.uRecvQueueLen);

    // return updated buffer size to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function CommonDiagnosticMonitor

    \Description
        Callback that outputs monitor state for monitor diagnostic page.

    \Input *pVoipServer     - voipserver module state
    \Input *pMonitor        - monitor information we are checking
    \Input iNumDiagnostics  - number of diagnostics we are checking for
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 02/26/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t CommonDiagnosticMonitor(VoipServerRefT *pVoipServer, const VoipServerDiagnosticMonitorT *pMonitor, int32_t iNumDiagnostics, char *pBuffer, int32_t iBufSize)
{
    uint32_t uStatus = 0;
    int32_t iOffset = 0, iDiagnostic;

    static const char *_strStatus[] =
    {
        "ok",
        "warn",
        "error"
    };

    // loop through monitor diagnostics
    for (iDiagnostic = 0; iDiagnostic < iNumDiagnostics; iDiagnostic++)
    {
        iOffset += VoipServerMonitorFlagCheck(pVoipServer, &pMonitor[iDiagnostic], &uStatus, pBuffer+iOffset, iBufSize-iOffset);
    }

    // make sure server build is a final build
    #if DIRTYCODE_DEBUG || DIRTYCODE_PROFILE
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "ERR: %s\n", "Server build is not a final build.");
    uStatus = 2;
    #endif

    // output status
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "status: %s\n", _strStatus[uStatus]);
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function CommonDiagnosticStatus

    \Description
        Callback to format voipserver stats for diagnostic request.

    \Input *pVoipServer     - voipserver module state
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 02/26/2018 (jbrookes)
*/
/********************************************************************************F*/
int32_t CommonDiagnosticStatus(VoipServerRefT *pVoipServer, char *pBuffer, int32_t iBufSize)
{
    const int32_t iStatRate = VOIPSERVER_VS_STATRATE / 1000;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    int32_t iUsersInGames = VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0);
    uint32_t uNumVoipPacketsDropped = VoipTunnelStatus(pVoipTunnel, 'vddm', 0, NULL, 0);
    uint32_t uNumVoipMaxTalkerEvents = VoipTunnelStatus(pVoipTunnel, 'vmtm', 0, NULL, 0);
    uint32_t uRecvCalls, uTotalRecv, uTotalRecvSub, uTotalPktsDiscard;
    int32_t iOffset = 0;
    char strAddrText[20], strHostName[128] = "";
    VoipServerStatT VoipServerStats;
    ProtoTunnelRefT *pProtoTunnel = VoipServerGetProtoTunnel(pVoipServer);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    // pull stats from the voipserver base
    VoipServerStatus(pVoipServer, 'stat', 0, &VoipServerStats, sizeof(VoipServerStats));

    // show general voipserver information
    iOffset += ServerDiagnosticFormatUptime(pBuffer+iOffset, iBufSize-iOffset, time(NULL), VoipServerStats.uStartTime);

    // show server name
    VoipServerStatus(pVoipServer, 'host', 0, strHostName, sizeof(strHostName));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server name: %s\n", strHostName);

    // server versioning info
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "buildversion: %s (%s)\n", _SDKVersion, DIRTYCAST_BUILDTYPE);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "buildtime: %s\n", _ServerBuildTime);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "buildlocation: %s\n", _ServerBuildLocation);

    #if defined(DIRTYCODE_LINUX)
    {
        uint32_t uHours, uMinutes, uSeconds;

        uSeconds = VoipServerStats.ServerInfo.uTime / 1000;
        uHours = uSeconds/(60*60);
        uSeconds -= uHours*60*60;
        uMinutes = uSeconds/60;
        uSeconds -= uMinutes*60;

        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server load: %%cpu=%0.2f time=%02d:%02d:%02d\n",
            VoipServerStats.ServerInfo.fCpu, uHours, uMinutes, uSeconds);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "system load average: %d.%02d %d.%02d %d.%02d (%d cpu cores)\n",
            VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[0]), VOIPSERVER_LoadFrac(VoipServerStats.ServerInfo.aLoadAvg[0]),
            VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[1]), VOIPSERVER_LoadFrac(VoipServerStats.ServerInfo.aLoadAvg[1]),
            VOIPSERVER_LoadInt(VoipServerStats.ServerInfo.aLoadAvg[2]), VOIPSERVER_LoadFrac(VoipServerStats.ServerInfo.aLoadAvg[2]),
            VoipServerStats.ServerInfo.iNumCPUCores);
    }
    #endif

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "debuglevel: %d\n", pConfig->uDebugLevel);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "frontendaddr: %s\n", SocketInAddrGetText(pConfig->uFrontAddr, strAddrText, sizeof(strAddrText)));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "apiport: %d\n", pConfig->uApiPort);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "gameport: %d, voipport: %d\n", pConfig->TunnelInfo.aRemotePortList[0], pConfig->TunnelInfo.aRemotePortList[1]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "tunnelport: %d\n", pConfig->uTunnelPort);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "clients connected: %d/%d   clients talking: %d/%d\n", iUsersInGames, pConfig->iMaxClients, VoipServerStats.iTalkingClientsAvg, VoipServerStats.iTalkingClientsCur);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "voip packets dropped: %d (%d events)\n", uNumVoipPacketsDropped, uNumVoipMaxTalkerEvents);

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bandwidth:\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   curr: %5dk/s up %5dk/s dn\n",
        (VoipServerStats.GameStatsCur.GameStat.uByteRecv+VoipServerStats.GameStatsCur.VoipStat.uByteRecv)/(iStatRate*1024),
        (VoipServerStats.GameStatsCur.GameStat.uByteSent+VoipServerStats.GameStatsCur.VoipStat.uByteSent)/(iStatRate*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   peak: %5dk/s up %5dk/s dn\n",
        (VoipServerStats.GameStatsMax.GameStat.uByteRecv+VoipServerStats.GameStatsMax.VoipStat.uByteRecv)/(iStatRate*1024),
        (VoipServerStats.GameStatsMax.GameStat.uByteSent+VoipServerStats.GameStatsMax.VoipStat.uByteSent)/(iStatRate*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "  total: %5dmb  up %5dmb  dn\n",
        (VoipServerStats.GameStatsSum.GameStat.uByteRecv+VoipServerStats.GameStatsSum.VoipStat.uByteRecv)/(1024*1024),
        (VoipServerStats.GameStatsSum.GameStat.uByteSent+VoipServerStats.GameStatsSum.VoipStat.uByteSent)/(1024*1024));

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "game bandwidth:\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   curr: %5dk/s up %5dk/s dn\n",
        VoipServerStats.GameStatsCur.GameStat.uByteRecv/(iStatRate*1024),
        VoipServerStats.GameStatsCur.GameStat.uByteSent/(iStatRate*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   peak: %5dk/s up %5dk/s dn\n",
        VoipServerStats.GameStatsMax.GameStat.uByteRecv/(iStatRate*1024),
        VoipServerStats.GameStatsMax.GameStat.uByteSent/(iStatRate*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "  total: %5dmb  up %5dmb  dn\n",
        VoipServerStats.GameStatsSum.GameStat.uByteRecv/(1024*1024),
        VoipServerStats.GameStatsSum.GameStat.uByteSent/(1024*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "voip bandwidth:\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   curr: %5dk/s up %5dk/s dn\n",
        VoipServerStats.GameStatsCur.VoipStat.uByteRecv/(iStatRate*1024),
        VoipServerStats.GameStatsCur.VoipStat.uByteSent/(iStatRate*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   peak: %5dk/s up %5dk/s dn\n",
        VoipServerStats.GameStatsMax.VoipStat.uByteRecv/(iStatRate*1024),
        VoipServerStats.GameStatsMax.VoipStat.uByteSent/(iStatRate*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "  total: %5dmb  up %5dmb  dn\n",
        VoipServerStats.GameStatsSum.VoipStat.uByteRecv/(1024*1024),
        VoipServerStats.GameStatsSum.VoipStat.uByteSent/(1024*1024));

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "packets:\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   curr: %5dp/s up %5dp/s dn\n",
        (VoipServerStats.GameStatsCur.GameStat.uPktsRecv+VoipServerStats.GameStatsCur.VoipStat.uPktsRecv)/iStatRate,
        (VoipServerStats.GameStatsCur.GameStat.uPktsSent+VoipServerStats.GameStatsCur.VoipStat.uPktsSent)/iStatRate);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   peak: %5dp/s up %5dp/s dn\n",
        (VoipServerStats.GameStatsMax.GameStat.uPktsRecv+VoipServerStats.GameStatsMax.VoipStat.uPktsRecv)/iStatRate,
        (VoipServerStats.GameStatsMax.GameStat.uPktsSent+VoipServerStats.GameStatsMax.VoipStat.uPktsSent)/iStatRate);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "  total: %5dmp  up %5dmp  dn\n",
        (VoipServerStats.GameStatsSum.GameStat.uPktsRecv+VoipServerStats.GameStatsSum.VoipStat.uPktsRecv)/(1024*1024),
        (VoipServerStats.GameStatsSum.GameStat.uPktsSent+VoipServerStats.GameStatsSum.VoipStat.uPktsSent)/(1024*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "game packets:\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   curr: %5dp/s up %5dp/s dn\n",
        VoipServerStats.GameStatsCur.GameStat.uPktsRecv/iStatRate,
        VoipServerStats.GameStatsCur.GameStat.uPktsSent/iStatRate);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   peak: %5dp/s up %5dp/s dn\n",
        VoipServerStats.GameStatsMax.GameStat.uPktsRecv/iStatRate,
        VoipServerStats.GameStatsMax.GameStat.uPktsSent/iStatRate);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "  total: %5dmp  up %5dmp  dn\n",
        VoipServerStats.GameStatsSum.GameStat.uPktsRecv/(1024*1024),
        VoipServerStats.GameStatsSum.GameStat.uPktsSent/(1024*1024));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "voip packets:\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   curr: %5dp/s up %5dp/s dn\n",
        VoipServerStats.GameStatsCur.VoipStat.uPktsRecv/iStatRate,
        VoipServerStats.GameStatsCur.VoipStat.uPktsSent/iStatRate);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   peak: %5dp/s up %5dp/s dn\n",
        VoipServerStats.GameStatsMax.VoipStat.uPktsRecv/iStatRate,
        VoipServerStats.GameStatsMax.VoipStat.uPktsSent/iStatRate);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "  total: %5dkp  up %5dkp  dn\n",
        VoipServerStats.GameStatsSum.VoipStat.uPktsRecv/(1024*1024),
        VoipServerStats.GameStatsSum.VoipStat.uPktsSent/(1024*1024));

    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "tunnel info:\n");
    uRecvCalls = ProtoTunnelStatus(pProtoTunnel, 'rcal', 0, NULL, 0) - VoipServerStats.uRecvCalls;
    uTotalRecv = ProtoTunnelStatus(pProtoTunnel, 'rtot', 0, NULL, 0) - VoipServerStats.uTotalRecv;
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "    recv calls:   %d/%d=%2.2f\n", uTotalRecv, uRecvCalls,
        (uRecvCalls != 0) ? (float)uTotalRecv/(float)uRecvCalls : 0.0f);
    uTotalRecvSub = ProtoTunnelStatus(pProtoTunnel, 'rsub', 0, NULL, 0) - VoipServerStats.uTotalRecvSub;
    uTotalPktsDiscard = ProtoTunnelStatus(pProtoTunnel, 'dpkt', 0, NULL, 0) - VoipServerStats.uTotalPktsDiscard;
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "  recv subpkts:   %d/%d=%2.2f\n", uTotalRecvSub, uRecvCalls,
        (uRecvCalls != 0) ? (float)uTotalRecvSub/(float)uRecvCalls : 0.0f);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "      discards:   %d\n", uTotalPktsDiscard);

    // print latency stats
    iOffset += _CommonDiagnosticLateStatus(&VoipServerStats.StatInfo.E2eLate, pBuffer + iOffset, iBufSize - iOffset, "net latency", 0);
    iOffset += _CommonDiagnosticLateStatus(&VoipServerStats.StatInfo.InbLate, pBuffer + iOffset, iBufSize - iOffset, "inb latency", 0);
    iOffset += _CommonDiagnosticLateStatus(&VoipServerStats.StatInfo.OtbLate, pBuffer + iOffset, iBufSize - iOffset, "out latency", 0);

    // return updated buffer size to caller
    return(iOffset);
}
