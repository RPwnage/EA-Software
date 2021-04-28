/*H********************************************************************************/
/*!
    \File conciergediagnostic.c

    \Description
        This module implements diagnostic page formatting for the VoipServer (Concierge)

    \Copyright
        Copyright (c) 2015-2018 Electronic Arts Inc.

    \Version 12/02/2015 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/voip/voiptunnel.h"
#include "DirtySDK/dirtyvers.h"

#include "commondiagnostic.h"
#include "conciergediagnostic.h"
#include "dirtycast.h"
#include "serveraction.h"
#include "voipserverconfig.h"
#include "voipserver.h"
#include "voipconcierge.h"

/*** Defines **********************************************************************/

//! number of connection sets we display per page
#define CONCIERGEDIAGNOSTIC_SETS_PER_PAGE (16)

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
};

/*** Private functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipConciergeDiagnosticQueryConnSets

    \Description
        Queries summary information about the connection sets

    \Input *pVoipServer - voipserver module state
    \Input iFirstConnSet- index of the first conn set to start at (used for paging)
    \Input *pBuffer     - [out] buffer we write the information to
    \Input iBufSize     - size of the buffer

    \Output
        int32_t         - number of bytes written to output buffer

    \Version 07/25/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeDiagnosticQueryConnSets(VoipServerRefT *pVoipServer, int32_t iFirstConnSet, char *pBuffer, int32_t iBufSize)
{
    int32_t iOffset = 0;
    int32_t iConnSet, iMaxConnSets, iDisplayCount;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);

    for (iConnSet = iFirstConnSet, iMaxConnSets = VoipTunnelStatus(pVoipTunnel, 'mgam', 0, NULL, 0), iDisplayCount = 0; iConnSet < iMaxConnSets; iConnSet += 1, iDisplayCount += 1)
    {
        char strAddrText[20];
        const int32_t iNumClients = VoipTunnelStatus(pVoipTunnel, 'nusr', iConnSet, NULL, 0);

        // if we reach our limit, print the link to next page and break out
        if (iDisplayCount == CONCIERGEDIAGNOSTIC_SETS_PER_PAGE)
        {
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<a href=\"http://%s:%d/sets=%d\">next</a>\n",
                SocketInAddrGetText(pConfig->uFrontAddr, strAddrText, sizeof(strAddrText)), pConfig->uDiagnosticPort, iConnSet);
            break;
        }

        // print the links to the set specific page
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<a href=\"http://%s:%d/set=%d\">connection set %d</a>: %d clients (%s)\n",
            SocketInAddrGetText(pConfig->uFrontAddr, strAddrText, sizeof(strAddrText)), pConfig->uDiagnosticPort, iConnSet, iConnSet, iNumClients,
            iNumClients != -1 ? "active" : "inactive");
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _VoipConciergeDiagnosticQueryConnSet

    \Description
        Queries information about the connection set to print to the diagnostic page

    \Input *pVoipServer - voipserver module state
    \Input iConnSetIdx  - index of the connection set we are querying
    \Input *pBuffer     - [out] buffer we write the information to
    \Input iBufSize     - size of the buffer

    \Output
        int32_t         - number of bytes written to output buffer

    \Version 07/25/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipConciergeDiagnosticQueryConnSet(VoipServerRefT *pVoipServer, int32_t iConnSetIdx, char *pBuffer, int32_t iBufSize)
{
    int32_t iOffset = 0;
    VoipTunnelGameT Game;
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);
    ProtoTunnelRefT *pProtoTunnel = VoipServerGetProtoTunnel(pVoipServer);

    VoipTunnelStatus(pVoipTunnel, 'game', iConnSetIdx, &Game, sizeof(Game));
    if (Game.iNumClients >= 0)
    {
        int32_t iClient;
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "%d clients\n", Game.iNumClients);

        for (iClient = 0; iClient < VOIPTUNNEL_MAXGROUPSIZE; iClient += 1)
        {
            struct sockaddr ClientAddr;
            VoipTunnelClientT *pClient;
            #if DIRTYVERS > DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
            VoipTunnelGameMappingT* pGameMapping;
            #endif

            if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, Game.aClientList[iClient])) == NULL)
            {
                continue;
            }
            ProtoTunnelStatus(pProtoTunnel, 'vtop', pClient->uRemoteAddr, &ClientAddr, sizeof(ClientAddr));

            #if DIRTYVERS <= DIRTYSDK_VERSION_MAKE(15, 1, 7, 0, 1)
            iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "[%d] id=0x%08x sendmask=0x%08x addr=%A status=%s talkers=%d flags=%s\n",
                iClient, pClient->uClientId, pClient->uSendMask, &ClientAddr, Game.bClientActive[iClient] ? "active" : "suspended",
                pClient->iNumTalker, (pClient->uFlags & VOIPTUNNEL_CLIENTFLAG_RECVVOICE) != 0 ? "talking" : "silent");
            #else
            pGameMapping = VoipTunnelClientMatchGameIdx(pClient, iConnSetIdx);
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "[%d] id=0x%08x topology voip sendmask=0x%08x addr=%A talkers=%d flags=%s\n",
                iClient, pClient->uClientId, pGameMapping->uTopologyVoipSendMask, &ClientAddr,
                pGameMapping->iNumTalkers, (pClient->uFlags & VOIPTUNNEL_CLIENTFLAG_RECVVOICE) != 0 ? "talking" : "silent");
            #endif
        }
    }
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "connection set inactive\n");
    }

    return(iOffset);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipConciergeDiagnosticCallback

    \Description
        Callback to format voipserver stats for diagnostic request.

    \Input *pServerDiagnostic - server diagnostic module state
    \Input *pUrl            - request url
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pUserData       - VoipServer module state

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 12/02/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipConciergeDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData)
{
    int32_t iOffset = 0;
    VoipConciergeRefT *pVoipConcierge = (VoipConciergeRefT *)pUserData;
    VoipServerRefT *pVoipServer = VoipConciergeGetBase(pVoipConcierge);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    VoipTunnelRefT *pVoipTunnel = VoipServerGetVoipTunnel(pVoipServer);

    if (strncmp(pUrl, "/servulator", 11) == 0)
    {
        iOffset += CommonDiagnosticServulator(pVoipServer, "Concierge", pConfig->uTunnelPort, VoipTunnelStatus(pVoipTunnel, 'nusr', -1, NULL, 0), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/action", 7) == 0)
    {
        iOffset += CommonDiagnosticAction(pVoipServer, strstr(pUrl, "/action"), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/gamestats", 10) == 0)
    {
        iOffset += CommonDiagnosticGameStats(pVoipServer, strstr(pUrl, "?reset") != NULL, pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/monitor", 8) == 0)
    {
        iOffset += CommonDiagnosticMonitor(pVoipServer, _MonitorDiagnostics, DIRTYCAST_CalculateArraySize(_MonitorDiagnostics), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/system", 7) == 0)
    {
        iOffset += CommonDiagnosticSystem(pVoipServer, pConfig->uTunnelPort, pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/status", 7) == 0)
    {
        VoipServerProcessStateE eProcessState;
        uint32_t bOffline;

        iOffset += CommonDiagnosticStatus(pVoipServer, pBuffer+iOffset, iBufSize-iOffset);

        // query server's state (running, draining, exiting)
        eProcessState = VoipServerStatus(pVoipServer, 'psta', 0, NULL, 0);
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "process state: %s\n", _VoipServer_strProcessStates[eProcessState]);

        // query CCS status (online or offline)
        bOffline = VoipConciergeStatus(pVoipConcierge, 'offl', 0, NULL, 0);
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ccs status: %s\n", (bOffline ? "offline (unknown to ccs)" : "online (known to ccs)"));
    }
    else if (strncmp(pUrl, "/sets", 5) == 0)
    {
        const int32_t iFirstConnSet = (pUrl[5] == '=') ? strtol(pUrl+6, NULL, 10) : 0;
        iOffset += _VoipConciergeDiagnosticQueryConnSets(pVoipServer, iFirstConnSet, pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/set", 4) == 0)
    {
        const int32_t iConnSet = (pUrl[4] == '=') ? strtol(pUrl+5, NULL, 10) : 0;
        iOffset += _VoipConciergeDiagnosticQueryConnSet(pVoipServer, iConnSet, pBuffer+iOffset, iBufSize-iOffset);
    }
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "unknown request type\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"gamestats\">/gamestats</a>\t-- display voipserver metrics\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "                 ?reset -- reset stats\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"metrics\">/metrics</a>\t\t-- display voipserver metrics\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "               ?reset\t-- reset stats\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"monitor\">/monitor</a>\t\t-- display voipserver monitor status\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"status\">/status</a>\t\t-- display voipserver status\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"system\">/system</a>\t\t-- display system status\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"sets\">/sets</a>=[first]\t-- display status summary for up to %d connection sets\n", CONCIERGEDIAGNOSTIC_SETS_PER_PAGE);
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"set\">/set</a>=[index]\t-- display info for specified connection set\n");
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function VoipConciergeDiagnosticGetResponseTypeCb

    \Description
        Get response type based on specified url

    \Input *pServerDiagnostic           - server diagnostic module state
    \Input *pUrl                        - request url

    \Output
        ServerDiagnosticResponseTypeE   - SERVERDIAGNOSTIC_RESPONSETYPE_*

    \Version 12/02/2015 (eesponda)
*/
/********************************************************************************F*/
ServerDiagnosticResponseTypeE VoipConciergeDiagnosticGetResponseTypeCb(ServerDiagnosticT *pServerDiagnostic, const char *pUrl)
{
    if ((strncmp(pUrl, "/servulator", 11) == 0) ||
        (strncmp(pUrl, "/metrics", 8) == 0))
    {
        return(SERVERDIAGNOSTIC_RESPONSETYPE_XML);
    }
    return(SERVERDIAGNOSTIC_RESPONSETYPE_DIAGNOSTIC);
}

