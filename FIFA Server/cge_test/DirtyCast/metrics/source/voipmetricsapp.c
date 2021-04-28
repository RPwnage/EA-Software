/*H********************************************************************************/
/*!
    \File voipmetricsapp.c

    \Description
        This application polls one or more VoipServers using the metrics diagnostic
        page.  It is intended to fulfill two major purposes:

           - stress testing: recording metrics information including an aggregation
             of all servers at regular intervals for later analysis (e.g. for
             monitoring a stress test)
           - graphing: reporting metrics via diagnostic page for the servers it is
             polling, acting as a concentrator of VoipServer metrics reports

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 03/19/2010 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_LINUX)
 #include <unistd.h>
 #include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "LegacyDirtySDK/util/tagfield.h"
#include "DirtySDK/proto/protohttp.h"

#include "dirtycast.h"
#include "serverdiagnostic.h"
#include "configfile.h"
#include "memtrack.h"

#include "voipmetrics.h"
#include "serveraction.h"

// shutdown flags
#define VOIPMETRICS_SHUTDOWN_IMMEDIATE   (1)
#define VOIPMETRICS_SHUTDOWN_IFEMPTY     (2)

#if defined(DIRTYCODE_PC)
extern uint32_t _NetLib_bUseHighResTimer;
#endif

/*** Macros ***********************************************************************/

//! quick way to format the data for ds_snzprintf
#define MONITOR_FormatGraphiteData(bUseStatsD, pKey) (!bUseStatsD ? \
    ("eadp.gos.voipmetrics.%s.connconcierge.%d.%s.%s.%s.clients." #pKey " %llu %u\n") : \
    ("eadp.gos.voipmetrics.%s.connconcierge.%d.%s.%s.%s.clients." #pKey ":%llu|g\n"))

/*** Defines **********************************************************************/

//! min debuglevel to show debug memory info
#define MONITOR_DBGLVL_MEMORY   (2)

//! max number of servers that can be monitored
#define MONITOR_MAX_SERVERS     (64)

//! max xml buffer size
#define MONITOR_MAX_XMLSIZE     (64*1024)

//! max http query time
#define MONITOR_HTTPTIMEOUT     (30*1000)

//! default graphite reporting rate
#define MONITOR_GRAPHITE_DEFAULT_RATE (300*1000)

//! default graphite connection retry rate
#define MONITOR_GRAPHITE_CONN_RATE    (60*1000)

//! default host we used for graphite reporting
#define MONITOR_GRAPHITE_DEFAULT_HOST ("eadp.gos.graphite-linedata.ea.com")

/*** Type Definitions *************************************************************/

//! monitor states
typedef enum MonitorStateE
{
    MONITOR_STATE_IDLE,
    MONITOR_STATE_QUERY
} MonitorStateE;

//! monitor query states for individual VoipServers
typedef enum MonitorQueryStateE
{
    MONITOR_QUERYSTATE_IDLE,
    MONITOR_QUERYSTATE_ACTV,
    MONITOR_QUERYSTATE_DONE
} MonitorQueryStateE;

//! monitor graphite connection state
typedef enum MonitorGraphiteStateE
{
    MONITOR_GRAPHITESTATE_ADDR,
    MONITOR_GRAPHITESTATE_WAIT_CONN,
    MONITOR_GRAPHITESTATE_ACTV,
    MONITOR_GRAPHITESTATE_INACTV,
    MONITOR_GRAPHITESTATE_FAIL,
    MONITOR_GRAPHITESTATE_MAX
} MonitorGraphiteStateE;

//! monitor server; representing a single VoipServer
typedef struct MonitorServerT
{
    ProtoHttpRefT   *pProtoHttp;            //!< http ref for querying server
    MonitorQueryStateE  eQueryState;        //!< query state for this server
    char            strQueryName[128];      //!< name of server to query
    uint32_t        uQueryPort;             //!< diagnostic port to query server on
    uint32_t        uTunnelPort;            //!< tunnel port associated with the server (used when extracting udp send/recv queues from /proc/net/udp6)
    int32_t         iLastHttpResult;        //!< most recent protohttp result
    time_t          uLastQueryTime;         //!< time of last successful query
    char            strQueryBuf[MONITOR_MAX_XMLSIZE];   //!< query result buffer
} MonitorServerT;

//! monitor application state
typedef struct MonitorStateT
{
    MonitorStateE   eMonitorState;          //!< monitor state

    time_t          uStartTime;             //!< start time
    char            strPidFileName[256];    //!< pid file name
    char            strHostName[128];       //!< hostname of this server
    char            strHostNameSafe[128];   //!< hostname that is safe to use for graphite

    char            strQueryName[128];      //!< base server name (can be overridden on a per-server basis)
    uint32_t        uQueryInterval;         //!< how often we are to query
    uint32_t        uQueryTimeout;          //!< how long we wait for a query response
    uint32_t        uQueryTimer;            //!< timer to track query rate
    uint32_t        uDiagnosticPort;        //!< diagnostic port
    char            strRegion[16];           //!< region tag (sjc, iad, gva, ...)
    char            strEnvironment[16];     //!< environment tag (dev, test, cert, prod)
    int32_t         iTier;                  //!< metrics "tier" (0=VS, 1=Local, 2=Regional, 3=Title)

    ServerDiagnosticT *pServerDiagnostic;   //!< monitor diagnostic module

    char            strCmdTagBuf[2048];     //!< tagfield for processing command-line options
    uint8_t         bOutputSummary;         //!< whether to output a recurring summary
    uint8_t         bNoReset;               //!< do not reset query source (can be used to safely hit prod servers without mucking with their metrics collection)
    uint8_t         _pad[2];

    VoipMetricsT    MonitorSummary;         //!< aggregate metrics

    uint32_t        uNumServers;            //!< number of servers we are querying (default one)
    MonitorServerT  MonitorServers[MONITOR_MAX_SERVERS];    //!< server info
    VoipMetricsT    MonitorMetrics[MONITOR_MAX_SERVERS];    //!< metric info

    SocketT         *pGraphiteSock;         //!< socket to graphite host
    HostentT        *pGraphiteHost;         //!< hostname entry for graphite
    MonitorGraphiteStateE eGraphiteState;   //!< state of the graphite connection
    uint8_t         bUseStatsD;             //!< should we reporting using statsd?
    uint32_t        uReportTime;            //!< tick of last time we reported
    uint32_t        uConnectTime;           //!< tick of last time we attempted connection
    uint32_t        uReportRate;            //!< rate at which we report to graphite
    char            strGraphiteHost[256];   //!< address for the graphite instance
    struct sockaddr GraphiteAddr;           //!< socket address for graphite instance
} MonitorStateT;

/*** Variables ********************************************************************/

//! used to signal application shutdown
static uint32_t     _bShutdown = FALSE;

//! shutdown signal
static int32_t      _iSignalFlags = 0;

//! start debug level high until we can read it in from the config file
static uint32_t     _uDebugLevel = MONITOR_DBGLVL_MEMORY+1;

//! default plain text port for graphite reporting
static int32_t      _iDefaultGraphitePort = 2003;
//! default statsd port for graphite reporting
static int32_t      _iDefaultStatsDPort = 8125;

//! used for logging
static const char *_strGraphiteStates[MONITOR_GRAPHITESTATE_MAX] =
{
    "MONITOR_GRAPHITESTATE_ADDR",
    "MONITOR_GRAPHITESTATE_WAIT_CONN",
    "MONITOR_GRAPHITESTATE_ACTV",
    "MONITOR_GRAPHITESTATE_INACTV",
    "MONITOR_GRAPHITESTATE_FAIL"
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _MonitorDiagnosticMetrics

    \Description
        Callback to format monitor metrics for diagnostic request.

    \Input *pMonitorState   - monitor state
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input uCurTime         - current time

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 09/01/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MonitorDiagnosticMetrics(MonitorStateT *pMonitorState, char *pBuffer, int32_t iBufSize, uint32_t uCurTime)
{
    MonitorServerT *pMonitorServer;
    int32_t iOffset = 0;
    uint32_t uServer;

    // open metrics response
    iOffset += VoipMetricsOpenXml(pBuffer+iOffset, iBufSize-iOffset);

    // display metrics summary
    iOffset += VoipMetricsFormatXml(&pMonitorState->MonitorSummary, pBuffer+iOffset, iBufSize-iOffset, pMonitorState->strHostName, pMonitorState->uDiagnosticPort, pMonitorState->uNumServers, TRUE, uCurTime);

    // display metrics for individual servers
    for (uServer = 0; uServer < pMonitorState->uNumServers; uServer += 1)
    {
        pMonitorServer = &pMonitorState->MonitorServers[uServer];
        iOffset += VoipMetricsFormatXml(&pMonitorState->MonitorMetrics[uServer], pBuffer+iOffset, iBufSize-iOffset, pMonitorServer->strQueryName, pMonitorServer->uQueryPort, uServer, FALSE, uCurTime);
    }

    // close metrics response
    iOffset += VoipMetricsCloseXml(pBuffer+iOffset, iBufSize-iOffset);
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _MonitorDiagnosticServulator

    \Description
        Callback to format monitor metrics for diagnostic request.

    \Input *pMonitorState   - monitor state
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input uCurTime         - current time

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 11/15/2010 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _MonitorDiagnosticServulator(MonitorStateT *pMonitorState, char *pBuffer, int32_t iBufSize, uint32_t uCurTime)
{
    //MonitorServerT *pMonitorServer;
    int32_t iOffset = 0;
    //uint32_t uServer;

    // open servulator response
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<servulator>");

    // if we've gotten this far we're init'ed
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Init>true</Init>\n");

    // Name of the process
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Name>Voip Metrics Server</Name>\n");

    // Game Version
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Version>na</Version>\n");

    // Port the server is running on
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Port>%d</Port>\n", pMonitorState->uDiagnosticPort);

    // Program State
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <State>na</State>\n");

    // Game Mode
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <GameMode>na</GameMode>\n");

    // Ranked
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Ranked>na</Ranked>\n");

    // Private Game
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Private>na</Private>\n");

    // Number of players
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Players>%d</Players>\n", pMonitorState->uNumServers);

    //  Number of observers
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <Observers>0</Observers>\n");

    // List of Players
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <PlayerList>na</PlayerList>\n");

    // List of Observers
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "   <ObserverList>na</ObserverList>\n");


    // close servulator response
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</servulator>");

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _MonitorDiagnosticStatus

    \Description
        Callback to format monitor metrics for diagnostic request.

    \Input *pMonitorState   - monitor state
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input uCurTime         - current time

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 09/08/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MonitorDiagnosticStatus(MonitorStateT *pMonitorState, char *pBuffer, int32_t iBufSize, uint32_t uCurTime)
{
    char strStatus[192], *pUrlHost;
    MonitorServerT *pMonitorServer;
    int32_t iOffset = 0;
    uint32_t uServer;

    // show general voipserver information
    iOffset += ServerDiagnosticFormatUptime(pBuffer+iOffset, iBufSize-iOffset, uCurTime, pMonitorState->uStartTime);

    // show server name
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server name: %s\n", pMonitorState->strHostName);

    // show misc server info
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "query interval: %dms\n", pMonitorState->uQueryInterval);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "output summary: %s\n", pMonitorState->bOutputSummary ? "enabled" : "disabled");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server list (%d servers):\n", pMonitorState->uNumServers);

    // show list of target servers and most recent query time
    for (uServer = 0; uServer < pMonitorState->uNumServers; uServer += 1)
    {
        pMonitorServer = &pMonitorState->MonitorServers[uServer];

        // format based on status
        if (pMonitorServer->iLastHttpResult == PROTOHTTP_RESPONSE_SUCCESSFUL)
        {
            ds_snzprintf(strStatus, sizeof(strStatus), "%s", "<font color=\"#088a08\">online</font>\n");
        }
        else
        {
            if (pMonitorServer->uLastQueryTime != 0)
            {
                char strTime[32];

                ds_snzprintf(strStatus, sizeof(strStatus), "<font color=\"#df0101\">offline</font> (http %d result; last successful query %s)\n",
                    pMonitorServer->iLastHttpResult, DirtyCastCtime(strTime, sizeof(strTime), pMonitorServer->uLastQueryTime));
            }
            else
            {
                ds_snzprintf(strStatus, sizeof(strStatus), "<font color=\"#df0101\">offline</font> (http %d result)\n",
                    pMonitorServer->iLastHttpResult);
            }
        }

        // get hostname
        pUrlHost = !strcmp(pMonitorServer->strQueryName, "127.0.0.1") ? pMonitorState->strHostName : pMonitorServer->strQueryName;

        // display status
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<a href=\"http://%s:%d/status\">%11s %2d</a>: status=%s",
            pUrlHost, pMonitorServer->uQueryPort, pMonitorState->MonitorMetrics[uServer].strServerType, uServer, strStatus);
    }
    return(iOffset);
}


/*F********************************************************************************/
/*!
    \Function _MonitorDiagnosticAction

    \Description
        Take an action based on specified command

    \Input *pMonitorState   - monitor module state
    \Input *pStrCommand     - command to parse
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 11/16/2010 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _MonitorDiagnosticAction(MonitorStateT *pMonitorState, const char *pStrCommand, char *pBuffer, int32_t iBufSize)
{
    ServerActionT ServerAction;
    int32_t iOffset = 0;

    // parse action and value
    if (ServerActionParse(pStrCommand, &ServerAction) < 0)
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "unable to parse action\n");
        return(iOffset);
    }

    if (!strcmp(ServerAction.strName, "debuglevel"))
    {
        iOffset += ServerActionParseInt(pBuffer, iBufSize, &ServerAction, 0, 10, (int32_t *)&_uDebugLevel);
    }
    else if (!strcmp(ServerAction.strName, "shutdown"))
    {
        //VM doesn't really support shutdown modes, but to maintain protocol with GS VS we'll read that in anyways
        //use mode varaiable if we choose to add drain functionality.
        int32_t mode = 0;
        ServerActionEnumT ShutdownEnum[] = { { "immediate", VOIPMETRICS_SHUTDOWN_IMMEDIATE }, { "graceful", VOIPMETRICS_SHUTDOWN_IFEMPTY } };
        iOffset += ServerActionParseEnum(pBuffer, iBufSize, &ServerAction, ShutdownEnum, sizeof(ShutdownEnum)/sizeof(ShutdownEnum[0]), &mode);
        _bShutdown = TRUE;
    }
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "action '%s' is not recognized\n", ServerAction.strName);
    }

    // return updated buffer size to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _MonitorDiagnosticCallback

    \Description
        Callback to format monitor metrics for diagnostic request.

    \Input *pServerDiagnostic - server diagnostic module state
    \Input *pUrl            - request url
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pUserData       - VoipMonitor module state

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 09/01/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MonitorDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData)
{
    MonitorStateT *pMonitorState = (MonitorStateT *)pUserData;
    int32_t iOffset = 0;
    time_t uCurTime = time(NULL);
    char strTime[32];

    // display server time for all requests except for metrics and servulator (xml) requests
    if (strncmp(pUrl, "/metrics", 8) && strncmp(pUrl, "/servulator", 11))
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server time: %s\n", DirtyCastCtime(strTime, sizeof(strTime), uCurTime));
    }

    // act based on request type
    if (!strncmp(pUrl, "/status", 7))
    {
        iOffset += _MonitorDiagnosticStatus(pMonitorState, pBuffer+iOffset, iBufSize-iOffset, uCurTime);
    }
    else if (!strncmp(pUrl, "/metrics", 8))
    {
        iOffset += _MonitorDiagnosticMetrics(pMonitorState, pBuffer+iOffset, iBufSize-iOffset, uCurTime);
    }
    else if (!strncmp(pUrl, "/servulator", 11))
    {
        iOffset += _MonitorDiagnosticServulator(pMonitorState, pBuffer+iOffset, iBufSize-iOffset, uCurTime);
    }
    else if (!strncmp(pUrl, "/action", 7))
    {
        iOffset += _MonitorDiagnosticAction(pMonitorState, strstr(pUrl, "/action"), pBuffer+iOffset, iBufSize-iOffset);
    }
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "unknown request type\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"metrics\">/metrics</a>             -- display voipserver metrics\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       <a href=\"servulator\">/servulator</a>          -- display voipserver servulator\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "       <a href=\"status\">/status</a>               -- display voipmetrics status\n");
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _MonitorDiagnosticGetResponseTypeCb

    \Description
        Get response type based on specified url

    \Input *pServerDiagnostic           - server diagnostic module state
    \Input *pUrl                        - request url

    \Output
        ServerDiagnosticResponseTypeE   - SERVERDIAGNOSTIC_RESPONSETYPE_*

    \Version 10/13/2010 (jbrookes)
*/
/********************************************************************************F*/
static ServerDiagnosticResponseTypeE _MonitorDiagnosticGetResponseTypeCb(ServerDiagnosticT *pServerDiagnostic, const char *pUrl)
{
    ServerDiagnosticResponseTypeE eResponseType = SERVERDIAGNOSTIC_RESPONSETYPE_DIAGNOSTIC;
    if (!strncmp(pUrl, "/metrics", 8) || !strncmp(pUrl, "/servulator", 11))
    {
        eResponseType = SERVERDIAGNOSTIC_RESPONSETYPE_XML;
    }
    return(eResponseType);
}

/*F********************************************************************************/
/*!
    \Function _MonitorSignalProcess

    \Description
        Process SIGTERM/SIGINT (graceful shutdown).

    \Input iSigNum  - Which signal was sent

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _MonitorSignalProcess(int32_t iSignal)
{
    // handle graceful shutdown
    if ((iSignal == SIGTERM) || (iSignal == SIGINT))
    {
        NetPrintf(("voipmetrics: _SignalProcess=%s\n", (iSignal == SIGTERM) ? "SIGTERM" : "SIGINT"));
        _bShutdown = TRUE;
        _iSignalFlags = iSignal;
    }
}

/*F********************************************************************************/
/*!
    \Function _MonitorPrintMetrics

    \Description
        Print a set of metrics to standard output.

    \Input *pMonitorState   - monitor state
    \Input *pMonitorMetrics - metrics to print (if NULL, prints header)
    \Input iMetricIndex     - index of server instace if (-1, represents summary)

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _MonitorPrintMetrics(MonitorStateT *pMonitorState, VoipMetricsT *pMonitorMetrics, int32_t iMetricIndex)
{
    // early out if not printing summary
    if (!pMonitorState->bOutputSummary)
    {
        return;
    }

    // don't log if we have no clients connected
    if ((pMonitorMetrics != NULL) && (pMonitorMetrics->aMetrics[VOIPMETRIC_ClientsConnected] == 0))
    {
        return;
    }

    // print metrics
    if (pMonitorMetrics != NULL)
    {
        uint64_t *pMetrics = pMonitorMetrics->aMetrics;
        char strServer[5] = "TOTL";

        if (iMetricIndex >= 0)
        {
            ds_snzprintf(strServer, sizeof(strServer), "[%02d]", iMetricIndex);
        }

        DirtyCastLogPrintf("%s %5llu %5llu %6llu %4llu %2llu.%02llu %2llu.%02llu %2llu.%02llu %2llu.%02llu %6llu %6llu %6llu %6llu %5llu %5llu %6llu %6llu %6llu %6llu %5llu %5llu %6llu %6llu %6llu %6llu %5llu %5llu %6llu %6llu %6llu %6llu %5llu %5llu %6llu %6llu %6llu %6llu %5llu %5llu %6llu %6llu %6llu %6llu %5llu %5llu %4llu %4llu %4llu %4llu %4llu %4llu %4llu %4llu %4llu %6llu %7llu %8llu %6llu %7llu %7llu %7llu %7llu %7llu %7llu %7llu %7llu %7llu %8llu %8llu\n", strServer,
            pMetrics[VOIPMETRIC_ClientsConnected], pMetrics[VOIPMETRIC_ClientsTalking], pMetrics[VOIPMETRIC_ClientsSupported], pMetrics[VOIPMETRIC_GamesActive],
            pMetrics[VOIPMETRIC_ServerCpuPct]/100, pMetrics[VOIPMETRIC_ServerCpuPct]%100,
            pMetrics[VOIPMETRIC_SystemLoad_1]/100, pMetrics[VOIPMETRIC_SystemLoad_1]%100,
            pMetrics[VOIPMETRIC_SystemLoad_5]/100, pMetrics[VOIPMETRIC_SystemLoad_5]%100,
            pMetrics[VOIPMETRIC_SystemLoad_15]/100, pMetrics[VOIPMETRIC_SystemLoad_15]%100,
            
            pMetrics[VOIPMETRIC_CurrBandwidthKpsUp], pMetrics[VOIPMETRIC_CurrBandwidthKpsDn],
            pMetrics[VOIPMETRIC_PeakBandwidthKpsUp], pMetrics[VOIPMETRIC_PeakBandwidthKpsDn],
            pMetrics[VOIPMETRIC_TotalBandwidthMbUp], pMetrics[VOIPMETRIC_TotalBandwidthMbDn],
            pMetrics[VOIPMETRIC_CurrGameBandwidthKpsUp], pMetrics[VOIPMETRIC_CurrGameBandwidthKpsDn],
            pMetrics[VOIPMETRIC_PeakGameBandwidthKpsUp], pMetrics[VOIPMETRIC_PeakGameBandwidthKpsDn],
            pMetrics[VOIPMETRIC_TotalGameBandwidthMbUp], pMetrics[VOIPMETRIC_TotalGameBandwidthMbDn],
            pMetrics[VOIPMETRIC_CurrVoipBandwidthKpsUp], pMetrics[VOIPMETRIC_CurrVoipBandwidthKpsDn],
            pMetrics[VOIPMETRIC_PeakVoipBandwidthKpsUp], pMetrics[VOIPMETRIC_PeakVoipBandwidthKpsDn],
            pMetrics[VOIPMETRIC_TotalVoipBandwidthMbUp], pMetrics[VOIPMETRIC_TotalVoipBandwidthMbDn],

            pMetrics[VOIPMETRIC_CurrPacketRateUp], pMetrics[VOIPMETRIC_CurrPacketRateDn],
            pMetrics[VOIPMETRIC_PeakPacketRateUp], pMetrics[VOIPMETRIC_PeakPacketRateDn],
            pMetrics[VOIPMETRIC_TotalPacketsUp], pMetrics[VOIPMETRIC_TotalPacketsDn],
            pMetrics[VOIPMETRIC_CurrGamePacketRateUp], pMetrics[VOIPMETRIC_CurrGamePacketRateDn],
            pMetrics[VOIPMETRIC_PeakGamePacketRateUp], pMetrics[VOIPMETRIC_PeakGamePacketRateDn],
            pMetrics[VOIPMETRIC_TotalGamePacketsUp], pMetrics[VOIPMETRIC_TotalGamePacketsDn],
            pMetrics[VOIPMETRIC_CurrVoipPacketRateUp], pMetrics[VOIPMETRIC_CurrVoipPacketRateDn],
            pMetrics[VOIPMETRIC_PeakVoipPacketRateUp], pMetrics[VOIPMETRIC_PeakVoipPacketRateDn],
            pMetrics[VOIPMETRIC_TotalVoipPacketsUp], pMetrics[VOIPMETRIC_TotalVoipPacketsDn],

            pMetrics[VOIPMETRIC_LatencyMin], pMetrics[VOIPMETRIC_LatencyMax], pMetrics[VOIPMETRIC_LatencyAvg],
            pMetrics[VOIPMETRIC_InbLateMin], pMetrics[VOIPMETRIC_InbLateMax], pMetrics[VOIPMETRIC_InbLateAvg],
            pMetrics[VOIPMETRIC_OtbLateMin], pMetrics[VOIPMETRIC_OtbLateMax], pMetrics[VOIPMETRIC_OtbLateAvg],
            pMetrics[VOIPMETRIC_RecvCalls], pMetrics[VOIPMETRIC_TotalRecv], pMetrics[VOIPMETRIC_TotalRecvSub], pMetrics[VOIPMETRIC_TotalDiscard],
            pMetrics[VOIPMETRIC_LateBin_0], pMetrics[VOIPMETRIC_LateBin_1], pMetrics[VOIPMETRIC_LateBin_2], pMetrics[VOIPMETRIC_LateBin_3],
            pMetrics[VOIPMETRIC_LateBin_4], pMetrics[VOIPMETRIC_LateBin_5], pMetrics[VOIPMETRIC_LateBin_6], pMetrics[VOIPMETRIC_LateBin_7],
            pMetrics[VOIPMETRIC_RlmtChangeCount],
            pMetrics[VOIPMETRIC_UdpSendQueueLen], pMetrics[VOIPMETRIC_UdpRecvQueueLen]
            );
    }
    else
    {
       DirtyCastLogPrintf("%4s %5s %5s %6s %4s %5s %5s %5s %5s %6s %6s %6s %6s %5s %5s %6s %6s %6s %6s %5s %5s %6s %6s %6s %6s %5s %5s %6s %6s %6s %6s %5s %5s %6s %6s %6s %6s %5s %5s %6s %6s %6s %6s %5s %5s %4s %4s %4s %4s %4s %4s %4s %4s %4s %6s %7s %8s %6s %7s %7s %7s %7s %7s %7s %7s %7s %7s %8s %8s %8s %8s %8s\n",
            "SERV", "CON", "TLK", "MAX", "GM", "PCT", "LOAD1", "LOAD5", "LOADF",
            " CKU", " CKD", " PKU", " PKD", " TMU", " TMD", "GCKU", "GCKD", "GPKU", "GPKD", "GTMU", "GTMD", "VCKU", "VCKD", "VPKU", "VPKD", "VTMU", "VTMD",
            " CPU", " CPD", " PPU", " PPD", " TPU", " TPD", "GCPU", "GCPD", "GPPU", "GPPD", "GTPU", "GTPD", "VCPU", "VCPD", "VPPU", "VPPD", "VTPU", "VTPD",
            "LMIN", "LMAX", "LAVG", "ILMN", "ILMX", "ILAV", "OLMN", "OLMX", "OLAV", "RCAL", "RTOT", "RSUB", "DPKT",
            "LBIN0", "LBIN1", "LBIN2", "LBIN3", "LBIN4", "LBIN5", "LBIN6", "LBIN7", "RLMT",
            "UERR", "USENT", "URECV", "USENTQ", "URECVQ");
    }
}

/*F********************************************************************************/
/*!
    \Function _MonitorRecvHeaderCallback

    \Description
        ProtoHttp recv header callback.

    \Input *pProtoHttp  - protohttp module state
    \Input *pHeader     - received header
    \Input uHeaderSize  - header size
    \Input *pUserData   - user ref (HttpRefT)

    \Output
        int32_t         - zero

    \Version 02/27/2019 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _MonitorRecvHeaderCallback(ProtoHttpRefT *pProtoHttp, const char *pHeader, uint32_t uHeaderSize, void *pUserData)
{
    MonitorServerT *pMonitorServer = (MonitorServerT *)pUserData;
    char strTunnelPort[32];

    // check for tunnelport header
    if (ProtoHttpGetHeaderValue(pProtoHttp, pHeader, "TunnelPort", strTunnelPort, sizeof(strTunnelPort), NULL) == 0)
    {
        uint32_t uTunnelPort;
        sscanf(strTunnelPort, "%u", &uTunnelPort);

        if (pMonitorServer->uTunnelPort == 0)
        {
            DirtyCastLogPrintf("voipmetrics: [%08x] tunnel port is %u\n", pMonitorServer, uTunnelPort);
        }
        else if (pMonitorServer->uTunnelPort != uTunnelPort)
        {
            DirtyCastLogPrintf("voipmetrics: [%08x] critical error: tunnel port changed from %u to %u\n", pMonitorServer, pMonitorServer->uTunnelPort, uTunnelPort);
        }
        pMonitorServer->uTunnelPort = uTunnelPort;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _MonitorInitialize

    \Description
        Initialize the voipmetrics server.

    \Input *pMonitorState   - monitor state
    \Input iArgc            - argument list count
    \Input *pArgv           - argument list

    \Output
        int32_t             - zero=failure, else success

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MonitorInitialize(MonitorStateT *pMonitorState, int32_t iArgc, const char *pArgv[])
{
    uint32_t uServer;
    uint32_t uChar;
    const char *pCommandTags, *pConfigTags;
    const char *pPortList;
    ConfigFileT *pConfig;
    const char *pConfigFile = pArgv[1];

    // init monitor state memory
    ds_memclr(pMonitorState, sizeof(*pMonitorState));

    // save startup time
    pMonitorState->uStartTime = time(NULL);

    // get hostname
    DirtyCastGetHostName(pMonitorState->strHostName, sizeof(pMonitorState->strHostName));

    // copy the hostname
    ds_memcpy(pMonitorState->strHostNameSafe, pMonitorState->strHostName, sizeof(pMonitorState->strHostNameSafe));
    // replace the '.' with '_'
    for (uChar = 0; uChar < sizeof(pMonitorState->strHostNameSafe); uChar += 1)
    {
        if (pMonitorState->strHostNameSafe[uChar] == '.')
        {
            pMonitorState->strHostNameSafe[uChar] = '_';
        }
    }

    // parse command-line parameters into tagfields
    pCommandTags = DirtyCastCommandLineProcess(pMonitorState->strCmdTagBuf, sizeof(pMonitorState->strCmdTagBuf), 2, iArgc, pArgv, "voipmetrics.");

    // log startup and build options
    DirtyCastLogPrintf("voipmetrics: started debug=%d strCmdTagBuf=%s\n", DIRTYCODE_DEBUG, pMonitorState->strCmdTagBuf);

    // create config file instance
    if ((pConfig = ConfigFileCreate()) == NULL)
    {
        DirtyCastLogPrintf("voipmetrics: could not create configfile instance\n");
        return(FALSE);
    }

    // load the config file
    DirtyCastLogPrintf("voipmetrics: loading config file %s\n", pConfigFile);
    ConfigFileLoad(pConfig, pConfigFile, "");
    ConfigFileWait(pConfig);

    // make sure we got a config file
    if ((pConfigTags = ConfigFileData(pConfig, "")) == NULL)
    {
        pConfigTags = "";
    }
    if (*pConfigTags == '\0')
    {
        DirtyCastLogPrintf("voipmetrics: warning; no config file found\n");
    }

    // save input params
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipmetrics.servername", pMonitorState->strQueryName, sizeof(pMonitorState->strQueryName), "127.0.0.1");
    pMonitorState->uDiagnosticPort = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.diagnosticport", 0);

    // create pid file using diagnostic port in name
    if (DirtyCastPidFileCreate(pMonitorState->strPidFileName, sizeof(pMonitorState->strPidFileName), pArgv[0],  pMonitorState->uDiagnosticPort) < 0)
    {
        DirtyCastLogPrintf("voipmetrics: error creating pid file -- exiting\n");
        return(FALSE);
    }

    // find port list
    if ((pPortList = DirtyCastConfigFind(pCommandTags, pConfigTags, "voipmetrics.portlist")) != NULL)
    {
        char strServerInfo[192], *pInfo;
        MonitorServerT *pMonitorServer;

        for (uServer = 0; uServer < MONITOR_MAX_SERVERS; uServer += 1, pMonitorState->uNumServers += 1)
        {
            pMonitorServer = &pMonitorState->MonitorServers[uServer];

            // get serverinfo string
            if (TagFieldGetDelim(pPortList, strServerInfo, sizeof(strServerInfo), "", uServer, ',') == 0)
            {
                break;
            }

            // two formats are supported: "server:port" and "port"
            if ((pInfo = strchr(strServerInfo, ':')) != NULL)
            {
                // copy out server name
                *pInfo = '\0';
                ds_strnzcpy(pMonitorServer->strQueryName, strServerInfo, sizeof(pMonitorServer->strQueryName));
                // index to port string
                pInfo += 1;
            }
            else
            {
                // use base server name
                ds_strnzcpy(pMonitorServer->strQueryName, pMonitorState->strQueryName, sizeof(pMonitorServer->strQueryName));
                // reference port string
                pInfo = strServerInfo;
            }

            // parse server port info
            pMonitorServer->uQueryPort = atoi(pInfo);
        }
    }

    // set query interval
    pMonitorState->uQueryInterval = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.queryrate", 300) * 1000;

    // get query timeout
    pMonitorState->uQueryTimeout = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.querytimeout", MONITOR_HTTPTIMEOUT);

    // if query timeout > query interval / 2, clamp it
    if (pMonitorState->uQueryTimeout > (pMonitorState->uQueryInterval/2))
    {
        DirtyCastLogPrintf("voipmetricsapp: clamping query timeout to %ds\n", pMonitorState->uQueryInterval/2);
        pMonitorState->uQueryTimeout = (pMonitorState->uQueryInterval/2);
    }

    // allocate http modules
    for (uServer = 0; uServer < pMonitorState->uNumServers; uServer += 1)
    {
        // create protohttp ref
        if ((pMonitorState->MonitorServers[uServer].pProtoHttp = ProtoHttpCreate(0)) == NULL)
        {
            return(0);
        }
        // configure timeout
        ProtoHttpControl(pMonitorState->MonitorServers[uServer].pProtoHttp, 'time', pMonitorState->uQueryTimeout, 0, NULL);

        ProtoHttpCallback(pMonitorState->MonitorServers[uServer].pProtoHttp, NULL, _MonitorRecvHeaderCallback, &pMonitorState->MonitorServers[uServer]);
    }

    // create module to listen for diagnostic requests
    if (pMonitorState->uDiagnosticPort != 0)
    {
        if ((pMonitorState->pServerDiagnostic = ServerDiagnosticCreate(pMonitorState->uDiagnosticPort, _MonitorDiagnosticCallback, pMonitorState, "VoipMetrics", MONITOR_MAX_XMLSIZE, "voipmetrics.", pCommandTags, pConfigTags)) != NULL)
        {
            DirtyCastLogPrintf("voipmetrics: created diagnostic socket bound to port %d\n", pMonitorState->uDiagnosticPort);
            // set additional callback type
            ServerDiagnosticCallback(pMonitorState->pServerDiagnostic, _MonitorDiagnosticCallback, _MonitorDiagnosticGetResponseTypeCb);
        }
        else
        {
            DirtyCastLogPrintf("voipmetrics: unable to create serverdiagnostic module\n");
            return(0);
        }
    }

    // check for quiet operation
    pMonitorState->bOutputSummary = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.summary", 1) ? TRUE : FALSE;

    // get region
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipmetrics.region", pMonitorState->strRegion, sizeof(pMonitorState->strRegion), "unk");

    // get tier
    pMonitorState->iTier = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.tier", 0);

    // get environment
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipmetrics.env", pMonitorState->strEnvironment, sizeof(pMonitorState->strEnvironment), "prod");

    // get noreset flag
    pMonitorState->bNoReset = (uint8_t)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.noreset", 0);

    // get graphite settings
    DirtyCastConfigGetString(pCommandTags, pConfigTags, "voipmetrics.graphite.host", pMonitorState->strGraphiteHost, sizeof(pMonitorState->strGraphiteHost), MONITOR_GRAPHITE_DEFAULT_HOST);
    pMonitorState->uReportRate = DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.graphite.rate", MONITOR_GRAPHITE_DEFAULT_RATE);

    // set up for immediate report (after connection is complete)
    pMonitorState->uReportTime = NetTick() - pMonitorState->uReportRate;

    pMonitorState->bUseStatsD = (uint8_t)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.graphite.statsd", FALSE);
    if (((uint8_t)DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "voipmetrics.graphite.enabled", FALSE)) == TRUE)
    {
        DirtyCastLogPrintf("voipmetrics: reporting to graphite enabled, setting up immediate connection\n");

        // set the state to fail, the process logic will automatically try to reconnect
        pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_FAIL;

        // set up for immediate connect
        pMonitorState->uConnectTime = NetTick() - MONITOR_GRAPHITE_CONN_RATE;
    }
    else
    {
        DirtyCastLogPrintf("voipmetrics: reporting to graphite disabled\n");
        pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_INACTV;
    }

    // set up for immediate query
    pMonitorState->uQueryTimer = NetTick() - pMonitorState->uQueryInterval;

    // set state
    pMonitorState->eMonitorState = MONITOR_STATE_IDLE;

    // destroy config file reader instance
    ConfigFileDestroy(pConfig);

    // make sure we parsed at least one server
    if (pMonitorState->uNumServers == 0)
    {
        DirtyCastLogPrintf("voipmetrics: no target server(s) specified\n");
        return(0);
    }
    // return success
    DirtyCastLogPrintf("voipmetrics: initialized\n");

    // display initial metrics header
    _MonitorPrintMetrics(pMonitorState, NULL, 0);

    return(1);
}

/*F********************************************************************************/
/*!
    \Function _MonitorRequestStart

    \Description
        Start an http metric request for the specified server

    \Input *pMonitorState   - monitor state
    \Input *pMonitorServer  - server to start request for
    \Input bShutdown        - if true, shut down

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _MonitorRequestStart(MonitorStateT *pMonitorState, MonitorServerT *pMonitorServer, uint32_t uServer)
{
    char strQueryUrl[512];
    const char *pReset = ((pMonitorState->iTier == 1) && !pMonitorState->bNoReset) ? "?reset" : ""; // tier1 metrics request reset their source (VS) on query

    // format query url
    ds_snzprintf(strQueryUrl, sizeof(strQueryUrl), "http://%s:%d/metrics%s", pMonitorServer->strQueryName, pMonitorServer->uQueryPort, pReset);
    NetPrintf(("voipmetrics: [%08x] querying status url '%s'\n", pMonitorServer, strQueryUrl));

    // make the request
    ProtoHttpRequest(pMonitorServer->pProtoHttp, strQueryUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_GET);
    pMonitorServer->eQueryState = MONITOR_QUERYSTATE_ACTV;
}

/*F********************************************************************************/
/*!
    \Function _MonitorRequestUpdate

    \Description
        Update an http metric request for the specified server

    \Input *pMonitorState   - monitor state
    \Input *pMonitorServer  - server to procses request for
    \Input uServer          - index of server to process

    \Output
        int32_t             - http recv result

    \Version 08/11/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MonitorRequestUpdate(MonitorStateT *pMonitorState, MonitorServerT *pMonitorServer, uint32_t uServer)
{
    int32_t iRecvResult;

    // update http status
    ProtoHttpUpdate(pMonitorServer->pProtoHttp);

    // check for data
    if ((iRecvResult = ProtoHttpRecvAll(pMonitorServer->pProtoHttp, pMonitorServer->strQueryBuf, sizeof(pMonitorServer->strQueryBuf))) >= 0)
    {
        // echo response to debug output
        #if DIRTYCODE_LOGGING && 0
        NetPrintf(("voipmetrics: [%08x] ------------------------------------------------------------------\n", pMonitorServer));
        NetPrintWrap(pMonitorServer->strQueryBuf, 80);
        #endif

        // make sure we have a 200/OK result
        if ((pMonitorServer->iLastHttpResult = ProtoHttpStatus(pMonitorServer->pProtoHttp, 'code', NULL, 0)) == PROTOHTTP_RESPONSE_SUCCESSFUL)
        {
            // parse response
            VoipMetricsParseXml(&pMonitorState->MonitorMetrics[uServer], pMonitorServer->strQueryBuf);

            // save query time
            pMonitorServer->uLastQueryTime = time(NULL);
        }
        else
        {
            // display error response
            DirtyCastLogPrintf("[%02d] http %d response\n", uServer, pMonitorServer->iLastHttpResult);
            // clear previous metrics
            ds_memclr(pMonitorState->MonitorMetrics[uServer].aMetrics, sizeof(pMonitorState->MonitorMetrics[uServer].aMetrics));
        }
    }
    else if ((iRecvResult < 0) && (iRecvResult != PROTOHTTP_RECVWAIT))
    {
        // save result
        pMonitorServer->iLastHttpResult = iRecvResult;
        // display error response
        DirtyCastLogPrintf("[%02d] http recv result %d\n", uServer, pMonitorServer->iLastHttpResult);
        // clear previous metrics
        ds_memclr(pMonitorState->MonitorMetrics[uServer].aMetrics, sizeof(pMonitorState->MonitorMetrics[uServer].aMetrics));
    }

    return(iRecvResult);
}

/*F********************************************************************************/
/*!
    \Function _MonitorProcessServer

    \Description
        Update a single server

    \Input *pMonitorState   - monitor state
    \Input *pMonitorServer  - server to procses
    \Input uServer          - index of server to process

    \Output
        int32_t             - true=busy, false=done

    \Version 08/11/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MonitorProcessServer(MonitorStateT *pMonitorState, MonitorServerT *pMonitorServer, uint32_t uServer)
{
    if (pMonitorServer->eQueryState == MONITOR_QUERYSTATE_IDLE)
    {
        _MonitorRequestStart(pMonitorState, pMonitorServer, uServer);
    }

    if (pMonitorServer->eQueryState == MONITOR_QUERYSTATE_ACTV)
    {
        // update the query
        int32_t iRecvResult = _MonitorRequestUpdate(pMonitorState, pMonitorServer, uServer);

        // if we're done, reset for next query
        if (iRecvResult != PROTOHTTP_RECVWAIT)
        {
            pMonitorServer->eQueryState = MONITOR_QUERYSTATE_DONE;
        }
    }

    return(pMonitorServer->eQueryState != MONITOR_QUERYSTATE_DONE);
}

/*F********************************************************************************/
/*!
    \Function _MonitorGraphiteConnect

    \Description
        Connects to the graphite server

    \Input *pMonitorState   - monitor state
    \Input uCurTick         - current tick count

    \Output
        uint8_t             - TRUE on success, FALSE otherwise

    \Notes
        If the server requires a hostname lookup it will start the process
        here and the update will then finish the connection

    \Version 02/10/2016 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _MonitorGraphiteConnect(MonitorStateT *pMonitorState, uint32_t uCurTick)
{
    // check to see if we can try a connection
    if (NetTickDiff(uCurTick, pMonitorState->uConnectTime) < 0)
    {
        return(TRUE);
    }

    // setup the address information
    SockaddrInit(&pMonitorState->GraphiteAddr, AF_INET);
    SockaddrInSetAddrText(&pMonitorState->GraphiteAddr, pMonitorState->strGraphiteHost);
    SockaddrInSetPort(&pMonitorState->GraphiteAddr, (pMonitorState->bUseStatsD ? _iDefaultStatsDPort : _iDefaultGraphitePort));

    // check if we need to do a lookup of the address
    if (SockaddrInGetAddr(&pMonitorState->GraphiteAddr) == 0)
    {
        if ((pMonitorState->pGraphiteHost = SocketLookup(pMonitorState->strGraphiteHost, 30*1000)) == NULL)
        {
            DirtyCastLogPrintf("voipmetrics: cannot start address lookup for graphite (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
            return(FALSE);
        }

        DirtyCastLogPrintf("voipmetrics: address lookup started (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_ADDR]);
        pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_ADDR;
    }
    // if we are communicating with graphite directly attempt to create the socket and connect
    else if (pMonitorState->bUseStatsD == FALSE)
    {
        int32_t iResult;

        if ((pMonitorState->pGraphiteSock = SocketOpen(AF_INET, SOCK_STREAM, IPPROTO_IP)) == NULL)
        {
            DirtyCastLogPrintf("voipmetrics: cannot open socket for graphite communication (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
            return(FALSE);
        }

        if ((iResult = SocketConnect(pMonitorState->pGraphiteSock, &pMonitorState->GraphiteAddr, sizeof(pMonitorState->GraphiteAddr))) != SOCKERR_NONE)
        {
            DirtyCastLogPrintf("voipmetrics: connection to graphite failed (error=%d) (%s->%s)\n", iResult, _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
            return(FALSE);
        }

        DirtyCastLogPrintf("voipmetrics: connection to graphite started (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_WAIT_CONN]);
        pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_WAIT_CONN;
    }
    // if we are communicating with statsd then we just create the socket, we do not establish a connection (uses udp)
    else
    {
        if ((pMonitorState->pGraphiteSock = SocketOpen(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == NULL)
        {
            DirtyCastLogPrintf("voipmetrics: cannot open socket for graphite communication (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
            return(FALSE);
        }

        DirtyCastLogPrintf("voipmetrics: statsd udp socket opened (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_ACTV]);
        pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_ACTV;
    }

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _MonitorProcessGraphite

    \Description
        Processes the graphite connection and report data based on our current
        tick

    \Input *pMonitorState   - monitor state
    \Input uCurTick         - current time used to know when to report data

    \Output
        uint8_t             - FALSE on connection wide error, TRUE otherwsie

    \Version 02/10/2016 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _MonitorProcessGraphite(MonitorStateT *pMonitorState, uint32_t uCurTick)
{
    int32_t iResult;
    struct sockaddr *pTo = NULL;
    int32_t iAddrLen = 0;

    // we are in the process of looking up the address
    if (pMonitorState->eGraphiteState == MONITOR_GRAPHITESTATE_ADDR)
    {
        // check if we are done resolving the hostname
        if (pMonitorState->pGraphiteHost->Done(pMonitorState->pGraphiteHost) == FALSE)
        {
            return(TRUE);
        }

        // update the address from the lookup
        SockaddrInSetAddr(&pMonitorState->GraphiteAddr, pMonitorState->pGraphiteHost->addr);

        // free the lookup info before try to connect
        pMonitorState->pGraphiteHost->Free(pMonitorState->pGraphiteHost);
        pMonitorState->pGraphiteHost = NULL;

        // check if we received a valid address back
        if (SockaddrInGetAddr(&pMonitorState->GraphiteAddr) == 0)
        {
            DirtyCastLogPrintf("voipmetrics: lookup of graphite address failed (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
            return(FALSE);
        }

        if ((pMonitorState->pGraphiteSock = SocketOpen(AF_INET, (pMonitorState->bUseStatsD ? SOCK_DGRAM : SOCK_STREAM), IPPROTO_IP)) == NULL)
        {
            DirtyCastLogPrintf("voipmetrics: cannot open socket for graphite communication (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
            return(FALSE);
        }

        if (pMonitorState->bUseStatsD == FALSE)
        {
            if ((iResult = SocketConnect(pMonitorState->pGraphiteSock, &pMonitorState->GraphiteAddr, sizeof(pMonitorState->GraphiteAddr))) != SOCKERR_NONE)
            {
                DirtyCastLogPrintf("voipmetrics: connection to graphite failed (error=%d) (%s->%s)\n", iResult, _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
                return(FALSE);
            }

            DirtyCastLogPrintf("voipmetrics: connection to graphite started (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_WAIT_CONN]);
            pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_WAIT_CONN;
        }
        else
        {
            DirtyCastLogPrintf("voipmetrics: statsd udp socket opened (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_ACTV]);
            pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_ACTV;
        }
    }

    // we are in the process of connecting to the graphite server (only when connecting tcp)
    if (pMonitorState->eGraphiteState == MONITOR_GRAPHITESTATE_WAIT_CONN)
    {
        if ((iResult = SocketInfo(pMonitorState->pGraphiteSock, 'stat', 0, NULL, 0)) > 0)
        {
            DirtyCastLogPrintf("voipmetrics: connection to graphite complete (%s->%s)\n", _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_ACTV]);
            pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_ACTV;
        }
        else if (iResult < 0)
        {
            DirtyCastLogPrintf("voipmetrics: connection to graphite failed (error=%d) (%s->%s)\n", iResult, _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
            return(FALSE);
        }
    }

    // our connection is active and we can report
    if (pMonitorState->eGraphiteState == MONITOR_GRAPHITESTATE_ACTV && (NetTickDiff(uCurTick, pMonitorState->uReportTime) > 0))
    {
        int32_t iVersionOffset;
        char strBuffer[512];
        int32_t iBufLen = 0;

        pMonitorState->uReportTime = uCurTick + pMonitorState->uReportRate;

        // make sure we have a pool set and are in concierge mode
        if (pMonitorState->MonitorSummary.strServerType[0] == '\0' ||
            ds_stristr(pMonitorState->MonitorSummary.strServerType, "concierge") == NULL)
        {
            DirtyCastLogPrintf("voipmetrics: invalid type for reporting graphite metrics\n");
            return(TRUE); // non-fatal error, we just don't want to report
        }
        if (pMonitorState->MonitorSummary.strPool[0] == '\0')
        {
            DirtyCastLogPrintf("voipmetrics: invalid pool specified\n");
            return(TRUE); // non-fatal error, we just don't want to report
        }

        for (iVersionOffset = 0; iVersionOffset < VOIPMETRIC_MAXVERS; iVersionOffset += 1)
        {
            uint16_t uVersion;

            // skip nonexistent versions
            if ((uVersion = (uint16_t)pMonitorState->MonitorSummary.aMetrics[VOIPMETRIC_ClientVersion_0+iVersionOffset]) == 0)
            {
                continue;
            }

            iBufLen += ds_snzprintf(strBuffer+iBufLen, sizeof(strBuffer)-iBufLen, MONITOR_FormatGraphiteData(pMonitorState->bUseStatsD, %u_%u.active),
                pMonitorState->strEnvironment, pMonitorState->iTier, pMonitorState->strRegion, pMonitorState->strHostNameSafe, pMonitorState->MonitorSummary.strPool,
                uVersion >> 8, uVersion & 0xff, pMonitorState->MonitorSummary.aMetrics[VOIPMETRIC_ClientsActive_0+iVersionOffset], time(NULL));
        }

        iBufLen += ds_snzprintf(strBuffer+iBufLen, sizeof(strBuffer)-iBufLen, MONITOR_FormatGraphiteData(pMonitorState->bUseStatsD, maximum),
            pMonitorState->strEnvironment, pMonitorState->iTier, pMonitorState->strRegion, pMonitorState->strHostNameSafe, pMonitorState->MonitorSummary.strPool,
            pMonitorState->MonitorSummary.aMetrics[VOIPMETRIC_ClientsSupported], time(NULL));

        DirtyCastLogPrintfVerbose(_uDebugLevel, 2, "voipmetrics: reporting to graphite %s", strBuffer);

        // if we are using statsd then update our destination information
        if (pMonitorState->bUseStatsD == TRUE)
        {
            pTo = &pMonitorState->GraphiteAddr;
            iAddrLen = sizeof(pMonitorState->GraphiteAddr);
        }

        if ((iResult = SocketSendto(pMonitorState->pGraphiteSock, strBuffer, iBufLen, 0, pTo, iAddrLen)) < 0)
        {
            DirtyCastLogPrintf("voipmetrics: reporting to graphite failed (error=%d) (%s->%s)\n", iResult, _strGraphiteStates[pMonitorState->eGraphiteState], _strGraphiteStates[MONITOR_GRAPHITESTATE_FAIL]);
            return(FALSE);
        }
    }

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _MonitorProcess

    \Description
        Run the voipmetrics server.

    \Input *pMonitorState   - monitor state
    \Input bShutdown        - if true, shut down

    \Output
        int32_t             - zero=done, else continue processing

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _MonitorProcess(MonitorStateT *pMonitorState, uint32_t bShutdown)
{
    // get current tick
    uint32_t uCurrTick = NetTick();

    // make a request?
    if ((pMonitorState->eMonitorState == MONITOR_STATE_IDLE) && (NetTickDiff(uCurrTick, pMonitorState->uQueryTimer) >= (signed)pMonitorState->uQueryInterval))
    {
        // set up to start queries
        pMonitorState->eMonitorState = MONITOR_STATE_QUERY;
    }

    // process a request?
    if (pMonitorState->eMonitorState == MONITOR_STATE_QUERY)
    {
        uint32_t uServer, uServersBusy;
        for (uServer = 0, uServersBusy = 0; uServer < pMonitorState->uNumServers; uServer += 1)
        {
            uServersBusy += (unsigned)_MonitorProcessServer(pMonitorState, &pMonitorState->MonitorServers[uServer], uServer);
        }

        // create metrics average
        if (uServersBusy == 0)
        {
            // output parsed source metrics data
            for (uServer = 0; uServer < pMonitorState->uNumServers; uServer += 1)
            {
                _MonitorPrintMetrics(pMonitorState, &pMonitorState->MonitorMetrics[uServer], uServer);
            }

            // aggregate server metrics
            VoipMetricsAggregate(&pMonitorState->MonitorSummary, pMonitorState->MonitorMetrics, pMonitorState->uNumServers);

            // set report info
            ds_strnzcpy(pMonitorState->MonitorSummary.strServerRegion, pMonitorState->strRegion, sizeof(pMonitorState->MonitorSummary.strServerRegion));
            pMonitorState->MonitorSummary.iServerTier = pMonitorState->iTier;

            if (pMonitorState->MonitorMetrics[0].strServerType[0] != '\0')
            {
                // check the type of the first vs and set your type based on that
                if (ds_stristr(pMonitorState->MonitorMetrics[0].strServerType, "concierge") != NULL)
                {
                    ds_strnzcpy(pMonitorState->MonitorSummary.strServerType, "voipmetrics/concierge", sizeof(pMonitorState->MonitorSummary.strServerType));
                    ds_strnzcpy(pMonitorState->MonitorSummary.strPool, pMonitorState->MonitorMetrics[0].strPool, sizeof(pMonitorState->MonitorSummary.strPool));
                }
                else
                {
                    ds_strnzcpy(pMonitorState->MonitorSummary.strServerType, "voipmetrics/gameserver", sizeof(pMonitorState->MonitorSummary.strServerType));
                    ds_memclr(pMonitorState->MonitorSummary.strPool, sizeof(pMonitorState->MonitorSummary.strPool));
                }
            }

            // output aggregate metric data
            if (pMonitorState->uNumServers > 1)
            {
                _MonitorPrintMetrics(pMonitorState, &pMonitorState->MonitorSummary, -1);
            }

            // set back to idle state
            pMonitorState->eMonitorState = MONITOR_STATE_IDLE;
            pMonitorState->uQueryTimer += pMonitorState->uQueryInterval;

            // reset individual server states
            for (uServer = 0; uServer < pMonitorState->uNumServers; uServer += 1)
            {
                pMonitorState->MonitorServers[uServer].eQueryState = MONITOR_QUERYSTATE_IDLE;
            }
        }
    }

    // if we are idle and graphite reporting is enabled
    // the reason we don't update during MONITOR_STATE_IDLE is we want to make sure we get the most up to take summary data
    if (pMonitorState->eMonitorState == MONITOR_STATE_IDLE && pMonitorState->eGraphiteState != MONITOR_GRAPHITESTATE_INACTV)
    {
        uint8_t bResult = TRUE;

        if (pMonitorState->eGraphiteState != MONITOR_GRAPHITESTATE_FAIL)
        {
            bResult = _MonitorProcessGraphite(pMonitorState, uCurrTick);
        }
        else
        {
            bResult = _MonitorGraphiteConnect(pMonitorState, uCurrTick);
        }

        /* if either the active connection failed or connection attempt failed, then program next
           attempt before a minimal delay */
        if (bResult == FALSE)
        {
            if (pMonitorState->pGraphiteSock != NULL)
            {
                SocketClose(pMonitorState->pGraphiteSock);
                pMonitorState->pGraphiteSock = NULL;
            }

            pMonitorState->eGraphiteState = MONITOR_GRAPHITESTATE_FAIL;
            pMonitorState->uConnectTime = uCurrTick + MONITOR_GRAPHITE_CONN_RATE;
        }
    }

    // update diagnostic
    if (pMonitorState->pServerDiagnostic != NULL)
    {
        ServerDiagnosticUpdate(pMonitorState->pServerDiagnostic);
    }

    // return status
    return(!bShutdown);
}

/*F********************************************************************************/
/*!
    \Function _MonitorShutdown

    \Description
        Shut down the voipmetrics server.

    \Input *pMonitorState   - monitor state

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _MonitorShutdown(MonitorStateT *pMonitorState)
{
    uint32_t uServer;

    DirtyCastLogPrintf("voipmetrics: shutting down (bShutdown=%d, iSignalFlags=%d)\n", _bShutdown, _iSignalFlags);

    // destroy diagnostic module
    if (pMonitorState->pServerDiagnostic != NULL)
    {
        ServerDiagnosticDestroy(pMonitorState->pServerDiagnostic);
        pMonitorState->pServerDiagnostic = NULL;
    }

    // destroy hostname lookup if we have one in progress
    if (pMonitorState->pGraphiteHost != NULL)
    {
        pMonitorState->pGraphiteHost->Free(pMonitorState->pGraphiteHost);
        pMonitorState->pGraphiteHost = NULL;
    }

    // destroy graphite socket
    if (pMonitorState->pGraphiteSock != NULL)
    {
        SocketClose(pMonitorState->pGraphiteSock);
        pMonitorState->pGraphiteSock = NULL;
    }

    // destroy protohttp modules
    for (uServer = 0; uServer < pMonitorState->uNumServers; uServer += 1)
    {
        if (pMonitorState->MonitorServers[uServer].pProtoHttp != NULL)
        {
            ProtoHttpDestroy(pMonitorState->MonitorServers[uServer].pProtoHttp);
            pMonitorState->MonitorServers[uServer].pProtoHttp = NULL;
        }
    }

    // remove pid file
    DirtyCastPidFileRemove(pMonitorState->strPidFileName);
}

/*F********************************************************************************/
/*!
    \Function _MonitorExecute

    \Description
        Start up networking and run the server.

    \Input *iArgc           - arg count
    \Input *pArgv[]         - arg list

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static int _MonitorExecute(int32_t iArgc, const char *pArgv[])
{
    static MonitorStateT MonitorState;
    int returnValue = DIRTYCAST_EXIT_OK;

    // start memory tracking
    MemtrackStartup();

    // use high-performance timer under windows, to get millisecond-accurate timing
    #if defined(DIRTYCODE_PC)
    _NetLib_bUseHighResTimer = TRUE;
    #endif

    // start DirtySock with no UPnP and in single-threaded mode
    NetConnStartup("-noupnp -singlethreaded");

    // initialize the monitor
    if (_MonitorInitialize(&MonitorState, iArgc, pArgv) != 0)
    {
        // ignore broken pipes
        #if defined(DIRTYCODE_LINUX)
        signal(SIGPIPE, SIG_IGN);
        #endif

        // set up to handle immediate server shutdown
        signal(SIGTERM, _MonitorSignalProcess);
        signal(SIGINT, _MonitorSignalProcess);

        // start the monitor running
        while (_MonitorProcess(&MonitorState, _bShutdown))
        {
            // execute a blocking poll on all of our possible inputs
            NetConnControl('poll', 100, 0, NULL, NULL);

            // give some idle time to network
            NetConnIdle();
        }
    }
    else
    {
        DirtyCastLogPrintf("_MonitorExecute: _MonitorInitialize failed returning %d\n", DIRTYCAST_EXIT_ERROR);
        returnValue = DIRTYCAST_EXIT_ERROR;
    }

    // shut down monitor
    _MonitorShutdown(&MonitorState);

    // shut down DirtySock
    NetConnShutdown(0);

    // shut down memory tracking
    MemtrackShutdown();

    return returnValue;
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function main

    \Description
        Main routine for application.

    \Input argc     - argument count
    \Input *argv[]  - argument list

    \Output
        int32_t     - application return code

    \Version 03/18/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t main(int32_t argc, const char *argv[])
{
    int32_t iReturnValue = DIRTYCAST_EXIT_OK;

    // validate argument count
    if (argc < 3)
    {
        printf("usage: %s [configfile] -servername=<server> -serverport=<port> -diagnosticport=<diagport> -region=<region> -tier=<tier> -portlist=<server1:port1|port1,server2:port2|port2,...,serverN:portN|portN> -queryrate=<rate> -summary=<1|0>\n", argv[0]);
        exit(DIRTYCAST_EXIT_ERROR);
    }

    // first thing we do is start up the logger
    if (DirtyCastLoggerCreate(argc, argv) < 0)
    {
        exit(DIRTYCAST_EXIT_ERROR);
    }

    // display version info
    DirtyCastLogPrintf("===============================================\n");
    DirtyCastLogPrintf("VoipMetrics build: %s (%s)\n", _SDKVersion, DIRTYCAST_BUILDTYPE);
    DirtyCastLogPrintf("BuildTime: %s\n", _ServerBuildTime);
    DirtyCastLogPrintf("BuildLocation: %s\n", _ServerBuildLocation);
    DirtyCastLogPrintf("===============================================\n");
    DirtyCastLogPrintf("main: pid=%d\n", DirtyCastGetPid());

    // run the monitor
    iReturnValue = _MonitorExecute(argc, argv);

    // shut down logging
    DirtyCastLoggerDestroy();

    // done
    printf("main completed exit: %d\n", iReturnValue);
    return(iReturnValue);
}

/*F********************************************************************************/
/*!
     \Function DirtyMemAlloc

     \Description
        Required memory allocation function for DirtySock client code.

     \Input iSize               - size of memory to allocate
     \Input iMemModule          - memory module
     \Input iMemGroup           - memory group
     \Input *pMemGroupUserData  - user data associated with memory group

     \Output
        void *                  - pointer to memory, or NULL

     \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    void *pMem;
    if ((pMem = (void *)malloc(iSize)) != NULL)
    {
        MemtrackAlloc(pMem, iSize, iMemModule);
    }
    if (_uDebugLevel > MONITOR_DBGLVL_MEMORY)
    {
        DirtyMemDebugAlloc(pMem, iSize, iMemModule, iMemGroup, pMemGroupUserData);
    }
    return(pMem);
}

/*F********************************************************************************/
/*!
     \Function DirtyMemFree

     \Description
        Required memory free function for DirtySock client code.

     \Input *pMem               - pointer to memory to dispose of
     \Input iMemModule          - memory module
     \Input iMemGroup           - memory group
     \Input *pMemGroupUserData  - user data associated with memory group

     \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    uint32_t uSize = 0;
    MemtrackFree(pMem, &uSize);
    if (_uDebugLevel > MONITOR_DBGLVL_MEMORY)
    {
        DirtyMemDebugFree(pMem, uSize, iMemModule, iMemGroup, pMemGroupUserData);
    }
    free(pMem);
}
