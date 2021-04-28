/*H********************************************************************************/
/*!
    \File commonmetrics.c

    \Description
        Contains metric formatting handlers for metrics that are common to all
        voipserver operating modes.

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <string.h>

#include "commonmetrics.h"

#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtynet.h"

#include "voipserver.h"
#include "voipserverconfig.h"
#include "udpmetrics.h"
#include "tokenapi.h"

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! metric handler type
typedef int32_t(CommonMetricHandlerT)(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

//! metric handler type (udp)
typedef int32_t (CommonMetricUdpHandlerT)(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const UdpMetricsT *pUdpMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

//! per-metric handler
typedef struct CommonMetricHandlerEntryT
{
    const char strMetricName[64];
    CommonMetricHandlerT *const pMetricHandler;
    const uint32_t uInterval;
    uint32_t uWhen;
} CommonMetricHandlerEntryT;

//! per-metric handler (udp)
typedef struct CommonMetricUdpHandlerEntryT
{
    char strMetricName[64];
    CommonMetricUdpHandlerT *pMetricHandler;
} CommonMetricUdpHandlerEntryT;


/*** Function Prototypes ***************************************************************/

static int32_t _CommonMetricsProcessState(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsClientsTalking(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsClientsAdded(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsAutoScaleBufferConst(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsGamesActive(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsGamesCreated(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsBandwidth(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsGameBandwidth(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsVoipBandwidth(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsPackets(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsGamePackets(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsVoipPackets(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsRecvCalls(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsTotalRecv(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsTotalRecvSub(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsTotalDiscard(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsServerCpuPct(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsServerCpuTime(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsServerCores(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsSystemLoad(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsLateBin(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsUdp(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsLatencyMin(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsLatencyMax(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsLatencyAvg(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
#if DIRTYSDK_VERSION >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 6)
static int32_t _CommonMetricsCertExpiry(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
#endif
static int32_t _CommonMetricsInbKernLateGameMin(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions);
static int32_t _CommonMetricsInbKernLateGameMax(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions);
static int32_t _CommonMetricsInbKernLateGameAvg(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions);
static int32_t _CommonMetricsInbKernLateVoipMin(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions);
static int32_t _CommonMetricsInbKernLateVoipMax(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions);
static int32_t _CommonMetricsInbKernLateVoipAvg(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions);
static int32_t _CommonMetricsUdpRecvQueueLen(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const UdpMetricsT *pUdpMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsUdpSendQueueLen(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const UdpMetricsT *pUdpMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _CommonMetricsCallback(VoipServerRefT *pVoipServer, ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, const char *pCommonDimensions, CommonMetricHandlerEntryT *pHandlerList, int32_t iHandlerArraySize);

/*** Variables ********************************************************************/

static CommonMetricHandlerEntryT _CommonMetrics_TrafficMetricHandlers[] =
{
    { "invalid",                                NULL, 0, 0 },
    { "voipmetrics.state",                      _CommonMetricsProcessState, 0, 0 },
    { "voipmetrics.clients.talking",            _CommonMetricsClientsTalking, 0, 0 },
    { "voipmetrics.clients.added",              _CommonMetricsClientsAdded, 0, 0 },
    { "voipmetrics.autoscale.bufferconst",      _CommonMetricsAutoScaleBufferConst, 0, 0 },
    { "voipmetrics.games.active",               _CommonMetricsGamesActive, 0, 0 },
    { "voipmetrics.games.created",              _CommonMetricsGamesCreated, 0, 0 },
    { "voipmetrics.games.bandwidth",            _CommonMetricsBandwidth, 0, 0 },
    { "voipmetrics.games.gamebandwidth",        _CommonMetricsGameBandwidth, 0, 0 },
    { "voipmetrics.games.voipbandwidth",        _CommonMetricsVoipBandwidth, 0, 0 },
    { "voipmetrics.games.packets",              _CommonMetricsPackets, 0, 0 },
    { "voipmetrics.games.gamepackets",          _CommonMetricsGamePackets, 0, 0 },
    { "voipmetrics.games.voippackets",          _CommonMetricsVoipPackets, 0, 0 },
    { "voipmetrics.tunnelinfo.recvcalls",       _CommonMetricsRecvCalls, 0, 0 },
    { "voipmetrics.tunnelinfo.totalrecv",       _CommonMetricsTotalRecv, 0, 0 },
    { "voipmetrics.tunnelinfo.totalrecvsub",    _CommonMetricsTotalRecvSub, 0, 0 },
    { "voipmetrics.tunnelinfo.totaldiscard",    _CommonMetricsTotalDiscard, 0, 0 },
    { "voipmetrics.latebin",                    _CommonMetricsLateBin, 0, 0},
    { "voipmetrics.latency.min",                _CommonMetricsLatencyMin, 0, 0 },
    { "voipmetrics.latency.max",                _CommonMetricsLatencyMax, 0, 0 },
    { "voipmetrics.latency.avg",                _CommonMetricsLatencyAvg, 0, 0 },
    #if DIRTYSDK_VERSION >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 6)
    { "voipmetrics.certexpiry",                 _CommonMetricsCertExpiry, 0, 0 },
    #endif
    { "voipmetrics.inbkernlate.game.min",       _CommonMetricsInbKernLateGameMin, 0, 0 },
    { "voipmetrics.inbkernlate.game.max",       _CommonMetricsInbKernLateGameMax, 0, 0 },
    { "voipmetrics.inbkernlate.game.avg",       _CommonMetricsInbKernLateGameAvg, 0, 0 },
    { "voipmetrics.inbkernlate.voip.min",       _CommonMetricsInbKernLateVoipMin, 0, 0 },
    { "voipmetrics.inbkernlate.voip.max",       _CommonMetricsInbKernLateVoipMax, 0, 0 },
    { "voipmetrics.inbkernlate.voip.avg",       _CommonMetricsInbKernLateVoipAvg, 0, 0 }
};

// a subset of common metrics which just describe machine health
static CommonMetricHandlerEntryT _CommonMetrics_SystemMetricHandlers[] =
{
    { "invalid",                                NULL, 0, 0 },
    { "voipmetrics.server.cpu.pct",             _CommonMetricsServerCpuPct, 0, 0 },
    { "voipmetrics.server.cpu.time",            _CommonMetricsServerCpuTime, 0, 0 },
    { "voipmetrics.server.cores",               _CommonMetricsServerCores, 60*5*1000, 0 },
    { "voipmetrics.systemload",                 _CommonMetricsSystemLoad, 0, 0 },
    { "voipmetrics.<misc_udp_info>",            _CommonMetricsUdp, 0, 0 }
};

// a subset of udp metrics called from the _CommonMetricsUdp function
static CommonMetricUdpHandlerEntryT _CommonMetrics_UdpMetricHandlers[] =
{
    { "voipmetrics.udpstats.udprecvqueue.len",  _CommonMetricsUdpRecvQueueLen },
    { "voipmetrics.udpstats.udpsendqueue.len",  _CommonMetricsUdpSendQueueLen }
};


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _CommonMetricsProcessState

    \Description
        Adds the "voipmetrics.state" metric to the specified buffer.
        Metric description: is the process running or in the process of shutting down

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/07/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsProcessState(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    VoipServerProcessStateE eProcessState = VoipServerStatus(pVoipServer, 'psta', 0, NULL, 0);
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)eProcessState, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsClientsTalking

    \Description
        Adds the "voipmetrics.clients.talking" metric to the specified buffer.
        Metric description: average number of "clients" that are "talking" over the sampling interval

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsClientsTalking(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uTalkingClientsAvg;

    // query average count of talking clients
    uTalkingClientsAvg = (uint64_t)pVoipServerStats->iTalkingClientsAvg;

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uTalkingClientsAvg, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsClientsAdded

    \Description
        Adds the "voipmetrics.clients.added" metric to the specified buffer.
        Metric description: total number of clients serviced by the voipserver since start

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsClientsAdded(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uClientsAdded;
    int32_t iUsersInGames = VoipTunnelStatus(VoipServerGetVoipTunnel(pVoipServer), 'nusr', -1, NULL, 0);

    // total number of users tracked is clients added minus current number of clients in game (who have not left yet)
    uClientsAdded = (uint64_t)((pVoipServerStats->uTotalClientsAdded - (uint32_t)iUsersInGames) - pVoipServerStats->uLastTotalClientsAdded);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uClientsAdded, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsAutoScaleBufferConst

    \Description
        Adds the "voipmetrics.autoscale.bufferconst" metric to the specified buffer.
        Metric description: the number of pods worth of capacity cloud fabric should 
        always have ready, for scaleup demand.  This is equivalent to the "bufferconst" 
        value in the .app file, but this has the benifit of being reflected in OI.

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/24/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsAutoScaleBufferConst(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pConfig->iAutoScaleBufferConst, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsGamesActive

    \Description
        Adds the "voipmetrics.games.active" metric to the specified buffer.
        Metric description: current number of "games" that are serviced by the voipserver

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsGamesActive(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uGamesActive;

    // query max clients supported
    uGamesActive = (uint64_t)VoipTunnelStatus(VoipServerGetVoipTunnel(pVoipServer), 'ngam', 0, NULL, 0);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uGamesActive, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsGamesCreated

    \Description
        Adds the "voipmetrics.games.created" metric to the specified buffer.
        Metric description: total number of games serviced by the voipserver since start

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsGamesCreated(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uGamesCreated;
    int32_t iGamesActive = VoipTunnelStatus(VoipServerGetVoipTunnel(pVoipServer), 'ngam', 0, NULL, 0);

    // total number of games tracked is games created minus current number of active games
    uGamesCreated = (uint64_t)((pVoipServerStats->uTotalGamesCreated - (uint32_t)iGamesActive) - pVoipServerStats->uLastTotalGamesCreated);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uGamesCreated, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsBandwidth

    \Description
        Adds the "voipmetrics.bandwidth.<curr|peak|total>.<up|dn>" metrics to the specified buffer.
        Metric description: bandwidth trackers (game + voip)

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsBandwidth(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iResult;
    uint64_t uValue;
    char strMetricName[64];
    const int32_t iStatRate = VOIPSERVER_VS_STATRATE / 1000;

    // add metric id suffix to metric name and calculate metric value
    ds_memclr(strMetricName, sizeof(strMetricName));
    ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.up");
    uValue = (uint64_t)((pVoipServerStats->GameStatsCur.GameStat.uByteRecv + pVoipServerStats->GameStatsCur.VoipStat.uByteRecv) / (iStatRate * 1024));
    if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
    {
        iOffset += iResult;
    }
    else
    {
        iErrorCode = iResult;
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metriC value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsCur.GameStat.uByteSent + pVoipServerStats->GameStatsCur.VoipStat.uByteSent) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.GameStat.uByteRecv + pVoipServerStats->GameStatsMax.VoipStat.uByteRecv) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.GameStat.uByteSent + pVoipServerStats->GameStatsMax.VoipStat.uByteSent) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.GameStat.uByteRecv + pVoipServerStats->GameStatsSum.VoipStat.uByteRecv) / (1024 * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.GameStat.uByteSent + pVoipServerStats->GameStatsSum.VoipStat.uByteSent) / (1024 * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsGameBandwidth

    \Description
        Adds the "voipmetrics.gamebandwidth.<curr|peak|total>.<up|dn>" metrics to the specified buffer.
        Metric description: bandwidth trackers (game only)

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsGameBandwidth(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iResult;
    uint64_t uValue;
    char strMetricName[64];
    const int32_t iStatRate = VOIPSERVER_VS_STATRATE / 1000;

    // add metric id suffix to metric name and calculate metric value
    ds_memclr(strMetricName, sizeof(strMetricName));
    ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.up");
    uValue = (uint64_t)((pVoipServerStats->GameStatsCur.GameStat.uByteRecv) / (iStatRate * 1024));
    if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
    {
        iOffset += iResult;
    }
    else
    {
        iErrorCode = iResult;
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsCur.GameStat.uByteSent) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.GameStat.uByteRecv) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.GameStat.uByteSent) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.GameStat.uByteRecv) / (1024 * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.GameStat.uByteSent) / (1024 * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsVoipBandwidth

    \Description
        Adds the "voipmetrics.voipbandwidth.<curr|peak|total>.<up|dn>" metrics to the specified buffer.
        Metric description: bandwidth trackers (voip only)

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsVoipBandwidth(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iResult;
    uint64_t uValue;
    char strMetricName[64];
    const int32_t iStatRate = VOIPSERVER_VS_STATRATE / 1000;

    // add metric id suffix to metric name and calculate metric value
    ds_memclr(strMetricName, sizeof(strMetricName));
    ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.up");
    uValue = (uint64_t)((pVoipServerStats->GameStatsCur.VoipStat.uByteRecv) / (iStatRate * 1024));
    if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
    {
        iOffset += iResult;
    }
    else
    {
        iErrorCode = iResult;
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsCur.VoipStat.uByteSent) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.VoipStat.uByteRecv) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.VoipStat.uByteSent) / (iStatRate * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.VoipStat.uByteRecv) / (1024 * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.VoipStat.uByteSent) / (1024 * 1024));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsPackets

    \Description
        Adds the "voipmetrics.packets.<curr|peak|total>.<up|dn>" metrics to the specified buffer.
        Metric description: packet rate trackers (game + voip)

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsPackets(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iResult;
    uint64_t uValue;
    char strMetricName[64];
    const int32_t iStatRate = VOIPSERVER_VS_STATRATE / 1000;

    // add metric id suffix to metric name and calculate metric value
    ds_memclr(strMetricName, sizeof(strMetricName));
    ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.up");
    uValue = (uint64_t)((pVoipServerStats->GameStatsCur.GameStat.uPktsRecv + pVoipServerStats->GameStatsCur.VoipStat.uPktsRecv) / iStatRate);
    if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
    {
        iOffset += iResult;
    }
    else
    {
        iErrorCode = iResult;
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metriC value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsCur.GameStat.uPktsSent + pVoipServerStats->GameStatsCur.VoipStat.uPktsSent) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.GameStat.uPktsRecv + pVoipServerStats->GameStatsMax.VoipStat.uPktsRecv) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.GameStat.uPktsSent + pVoipServerStats->GameStatsMax.VoipStat.uPktsSent) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.GameStat.uPktsRecv + pVoipServerStats->GameStatsSum.VoipStat.uPktsRecv) / (1024*1024));   // in millions of packets
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.GameStat.uPktsSent + pVoipServerStats->GameStatsSum.VoipStat.uPktsSent) / (1024*1024));   // in millions of packets
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsGamePackets

    \Description
        Adds the "voipmetrics.gamepackets.<curr|peak|total>.<up|dn>" metrics to the specified buffer.
        Metric description: packet rate trackers (game only)

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsGamePackets(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iResult;
    uint64_t uValue;
    char strMetricName[64];
    const int32_t iStatRate = VOIPSERVER_VS_STATRATE / 1000;

    // add metric id suffix to metric name and calculate metric value
    ds_memclr(strMetricName, sizeof(strMetricName));
    ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.up");
    uValue = (uint64_t)((pVoipServerStats->GameStatsCur.GameStat.uPktsRecv) / iStatRate);
    if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
    {
        iOffset += iResult;
    }
    else
    {
        iErrorCode = iResult;
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsCur.GameStat.uPktsSent) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.GameStat.uPktsRecv) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.GameStat.uPktsSent) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.GameStat.uPktsRecv) / (1024*1024));   // in millions of packets
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.GameStat.uPktsSent) / (1024*1024));   // in millions of packets
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsVoipPackets

    \Description
        Adds the "voipmetrics.voippackets.<curr|peak|total>.<up|dn>" metrics to the specified buffer.
        Metric description: packet rate trackers (voip only)

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsVoipPackets(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iResult;
    uint64_t uValue;
    char strMetricName[64];
    const int32_t iStatRate = VOIPSERVER_VS_STATRATE / 1000;

    // add metric id suffix to metric name and calculate metric value
    ds_memclr(strMetricName, sizeof(strMetricName));
    ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.up");
    uValue = (uint64_t)((pVoipServerStats->GameStatsCur.VoipStat.uPktsRecv) / iStatRate);
    if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
    {
        iOffset += iResult;
    }
    else
    {
        iErrorCode = iResult;
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "curr.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsCur.VoipStat.uPktsSent) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.VoipStat.uPktsRecv) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "peak.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsMax.VoipStat.uPktsSent) / iStatRate);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.up");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.VoipStat.uPktsRecv) / (1024*1024));   // in millions of packets
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode == 0)
    {
        // add metric id suffix to metric name and calculate metric value
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%s", pMetricName, "total.dn");
        uValue = (uint64_t)((pVoipServerStats->GameStatsSum.VoipStat.uPktsSent) / (1024*1024));   // in millions of packets
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uValue, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iErrorCode = iResult;
        }
    }

    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsRecvCalls

    \Description
        Adds the "voipmetrics.tunnelinfo.recvcalls" metric to the specified buffer.
        Metric description: number of packet "recv" calls performed by the voipserver (over the sampling interval)

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsRecvCalls(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uRecvCallsDelta;

    // calculate metric value
    uRecvCallsDelta = (uint64_t)(ProtoTunnelStatus(VoipServerGetProtoTunnel(pVoipServer), 'rcal', 0, NULL, 0) - pVoipServerStats->uRecvCalls);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uRecvCallsDelta, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsTotalRecv

    \Description
        Adds the "voipmetrics.tunnelinfo.totalrecv" metric to the specified buffer.
        Metric description: number of packets received by the voipserver

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsTotalRecv(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uTotalRecvDelta;

    // calculate metric value
    uTotalRecvDelta = (uint64_t)(ProtoTunnelStatus(VoipServerGetProtoTunnel(pVoipServer), 'rtot', 0, NULL, 0) - pVoipServerStats->uTotalRecv);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uTotalRecvDelta, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsTotalRecvSub

    \Description
        Adds the "voipmetrics.tunnelinfo.totalrecv" metric to the specified buffer.
        Metric description: number of sub-packets received by the voipserver

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsTotalRecvSub(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uTotalRecvSubDelta;

    // calculate metric value
    uTotalRecvSubDelta = (uint64_t)(ProtoTunnelStatus(VoipServerGetProtoTunnel(pVoipServer), 'rsub', 0, NULL, 0) - pVoipServerStats->uTotalRecvSub);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uTotalRecvSubDelta, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsTotalDiscard

    \Description
        Adds the "voipmetrics.tunnelinfo.totaldiscard" metric to the specified buffer.
        Metric description: number of packets discarded by the voipserver

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsTotalDiscard(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uTotalDiscardDelta;

    // calculate metric value
    uTotalDiscardDelta = (uint64_t)(ProtoTunnelStatus(VoipServerGetProtoTunnel(pVoipServer), 'dpkt', 0, NULL, 0) - pVoipServerStats->uTotalPktsDiscard);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uTotalDiscardDelta, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsServerCpuPct

    \Description
        Adds the "voipmetrics.server.cpu.pct" metric to the specified buffer.
        Metric description: cpu percentage consumed by the voipserver

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsServerCpuPct(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    if (VoipServerStatus(pVoipServer, 'fcpu', 0, NULL, 0) == -1)
    {
        return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)(pVoipServerStats->ServerInfo.fCpu * 100.0f), SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
    }
    else
    {
        return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)(VoipServerStatus(pVoipServer, 'fcpu', 0, NULL, 0) * 100), SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
    }
}


/*F********************************************************************************/
/*!
    \Function _CommonMetricsServerCpuTime

    \Description
        Adds the "voipmetrics.server.cpu.time" metric to the specified buffer.
        Metric description: cpu time allocated to the voipserver over the sampling interval

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsServerCpuTime(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)(pVoipServerStats->ServerInfo.uTime / 1000), SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsServerCores

    \Description
        Adds the "voipmetrics.server.cores" metric to the specified buffer.
        Metric description: number of cores on the host where the voipserver is running

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsServerCores(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCommonMetricsServerCoresDim[1024];

    ds_snzprintf(strCommonMetricsServerCoresDim, sizeof(strCommonMetricsServerCoresDim), "%s,sdkver:%s", pCommonDimensions, _SDKVersion);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->ServerInfo.iNumCPUCores, SERVERMETRICS_TYPE_GAUGE, 100, strCommonMetricsServerCoresDim));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsSystemLoad

    \Description
        Adds the "voipmetrics.systemload.<n>" metrics to the specified buffer.
        Metric description: system load

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsSystemLoad(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    int32_t iResult;
    uint64_t uSystemLoad;
    char strMetricName[64];

    // --- voipmetrics.systemload.1 ---

    // calculate system load metric value
    uSystemLoad = (uint64_t)VOIPSERVER_LoadPct(pVoipServerStats->ServerInfo.aLoadAvg[0]);

    // build metric name
    ds_memclr(strMetricName, sizeof(strMetricName));
    ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.1", pMetricName);

    if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uSystemLoad, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
    {
        // metric successfully added
        iOffset += iResult;
    }
    else
    {
        // error - abort!
        iErrorCode = iResult;
    }


    // --- voipmetrics.systemload.5 ---

    if (iErrorCode == 0)
    {
        // calculate system load metric value
        uSystemLoad = (uint64_t)VOIPSERVER_LoadPct(pVoipServerStats->ServerInfo.aLoadAvg[1]);

        // build metric name
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.5", pMetricName);

        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uSystemLoad, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            // metric successfully added
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }
    }


    // --- voipmetrics.systemload.15 ---

    if (iErrorCode == 0)
    {
        // calculate system load metric value
        uSystemLoad = (uint64_t)VOIPSERVER_LoadPct(pVoipServerStats->ServerInfo.aLoadAvg[2]);

        // build metric name
        ds_memclr(strMetricName, sizeof(strMetricName));
        ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.15", pMetricName);

        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uSystemLoad, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions)) >= 0)
        {
            // metric successfully added
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }
    }


    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsLateBin

    \Description
        Adds the "voipmetrics.latebin_<n>" metrics to the specified buffer.
        Metric description: latency buckets

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsLateBin(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    int32_t iLateBinIndex = 0;
    int32_t iOffset = 0;
    int32_t iErrorCode = 0;
    uint64_t uLateBin;

    // loop through all the versions found
    while ((iErrorCode == 0) && (iLateBinIndex < (signed)DIRTYCAST_CalculateArraySize(pVoipServerStats->StatInfo.LateBin.aBins)))
    {
        // query latebin metric value
        uLateBin = (uint64_t)pVoipServerStats->StatInfo.LateBin.aBins[iLateBinIndex];

        if (uLateBin != 0)
        {
            char strMetricName[64];
            int32_t iResult;

            // build metric name
            ds_snzprintf(strMetricName, sizeof(strMetricName), "%s.%d", pMetricName, iLateBinIndex);

            if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, strMetricName, eFormat, uLateBin, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions)) >= 0)
            {
                // metric successfully added
                iOffset += iResult;
            }
            else
            {
                // error - abort!
                iErrorCode = iResult;
            }
        }

        iLateBinIndex++;
    }

    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsLatencyMin

    \Description
        Adds the "voipmetrics.latency.min" metrics to the specified buffer.
        Metric description: minimum latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsLatencyMin(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->StatInfo.E2eLate.uMinLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsLatencyMax

    \Description
        Adds the "voipmetrics.latency.max" metrics to the specified buffer.
        Metric description: maximum latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsLatencyMax(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->StatInfo.E2eLate.uMaxLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsLatencyAvg

    \Description
        Adds the "voipmetrics.latency.avg" metrics to the specified buffer.
        Metric description: average latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsLatencyAvg(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint64_t uLatencyAvg;

    // calculate metric value
    uLatencyAvg = (uint64_t)((pVoipServerStats->StatInfo.E2eLate.uNumStat != 0) ? pVoipServerStats->StatInfo.E2eLate.uSumLate / pVoipServerStats->StatInfo.E2eLate.uNumStat : 0);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uLatencyAvg, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

#if DIRTYSDK_VERSION >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 6)
/*F********************************************************************************/
/*!
    \Function _CommonMetricsCertExpiry

    \Description
        Adds the "voipmetrics.certexpiry" metrics to the specified buffer.
        Metric description: expiration date of our certificate

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 11/02/2020 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsCertExpiry(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    TokenApiRefT *pTokenApi = VoipServerGetTokenApi(pVoipServer);
    uint64_t uExpiry;

    // query the metric value
    TokenApiStatus(pTokenApi, 'cexp', &uExpiry, sizeof(uExpiry));

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uExpiry, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}
#endif

/*F********************************************************************************/
/*!
    \Function _CommonMetricsInbKernLateGameMin

    \Description
        Adds the "voipmetrics.inbkernlate.game.min" metrics to the specified buffer.
        Metric description: minimum kernel-induced inbound latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 09/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsInbKernLateGameMin(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->InbKernLate.Game.uMinLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsInbKernLateGameMax

    \Description
        Adds the "voipmetrics.inbkernlate.game.max" metrics to the specified buffer.
        Metric description: maximum kernel-induced inbound latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 09/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsInbKernLateGameMax(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->InbKernLate.Game.uMaxLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsInbKernLateGameAvg

    \Description
        Adds the "voipmetrics.inbkernlate.game.avg" metrics to the specified buffer.
        Metric description: average kernel-induced inbound latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 09/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsInbKernLateGameAvg(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions)
{
    uint64_t uLatencyAvg;

    // calculate metric value
    uLatencyAvg = (uint64_t)((pVoipServerStats->InbKernLate.Game.uNumStat != 0) ? pVoipServerStats->InbKernLate.Game.uSumLate / pVoipServerStats->InbKernLate.Game.uNumStat : 0);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uLatencyAvg, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsInbKernLateVoipMin

    \Description
        Adds the "voipmetrics.inbkernlate.voip.min" metrics to the specified buffer.
        Metric description: minimum kernel-induced inbound latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 09/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsInbKernLateVoipMin(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->InbKernLate.Voip.uMinLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsInbKernLateVoipMax

    \Description
        Adds the "voipmetrics.inbkernlate.voip.max" metrics to the specified buffer.
        Metric description: maximum kernel-induced inbound latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 09/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsInbKernLateVoipMax(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, (uint64_t)pVoipServerStats->InbKernLate.Voip.uMaxLate, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsInbKernLateVoipAvg

    \Description
        Adds the "voipmetrics.inbkernlate.voip.avg" metrics to the specified buffer.
        Metric description: average kernel-induced inbound latency

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 09/17/2020 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsInbKernLateVoipAvg(VoipServerRefT* pVoipServer, VoipServerStatT* pVoipServerStats, const char* pMetricName, ServerMetricsFormatE eFormat, char* pBuffer, int32_t iBufSize, const char* pCommonDimensions)
{
    uint64_t uLatencyAvg;

    // calculate metric value
    uLatencyAvg = (uint64_t)((pVoipServerStats->InbKernLate.Voip.uNumStat != 0) ? pVoipServerStats->InbKernLate.Voip.uSumLate / pVoipServerStats->InbKernLate.Voip.uNumStat : 0);

    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uLatencyAvg, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsUdpRecvQueueLen

    \Description
        Adds the "voipmetrics.udpstats.udprecvqueue.len" metrics to the specified buffer.
        Metric description: host-wide port-specific udp recv queue len

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pUdpMetrics         - udp stats for prototunnel port used by the server
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Notes
        Here we use SERVERMETRICS_TYPE_HISTOGRAM because we are interested in the max and 95th percentile
        to eventually assess when udp is no longer behaving fine under heavy load.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsUdpRecvQueueLen(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const UdpMetricsT *pUdpMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pUdpMetrics->uRecvQueueLen, SERVERMETRICS_TYPE_TIMER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsUdpSendQueueLen

    \Description
        Adds the "voipmetrics.udpstats.udpsendqueue.len" metrics to the specified buffer.
        Metric description: host-wide port-specific udp send queue len

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module stats
    \Input *pUdpMetrics         - udp stats for prototunnel port used by the server
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Notes
        Here we use SERVERMETRICS_TYPE_HISTOGRAM because we are interested in the max and 95th percentile
        to eventually assess when udp is no longer behaving fine under heavy load.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsUdpSendQueueLen(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const UdpMetricsT *pUdpMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pUdpMetrics->uSendQueueLen, SERVERMETRICS_TYPE_TIMER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsUdp

    \Description
        Adds the udp metrics to the specified buffer.

    \Input *pVoipServer         - module state
    \Input *pVoipServerStats    - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Notes
        For specific information about the metric being reported please see the documentation
        within the function we call.

    \Version 02/27/2018 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsUdp(VoipServerRefT *pVoipServer, VoipServerStatT *pVoipServerStats, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    UdpMetricsT UdpMetrics;
    int32_t iMetricIndex, iOffset = 0;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    // get udp stats
    VoipServerStatus(pVoipServer, 'udpm', 0, &UdpMetrics, sizeof(UdpMetrics));

    for (iMetricIndex = 0; iMetricIndex < (signed)DIRTYCAST_CalculateArraySize(_CommonMetrics_UdpMetricHandlers); iMetricIndex += 1)
    {
        int32_t iResult;
        const CommonMetricUdpHandlerEntryT *pEntry = &_CommonMetrics_UdpMetricHandlers[iMetricIndex];

        if ((iResult = pEntry->pMetricHandler(pVoipServer, pVoipServerStats, &UdpMetrics, pEntry->strMetricName, pConfig->eMetricsFormat, pBuffer + iOffset, iBufSize - iOffset, pCommonDimensions)) >= 0)
        {
            iOffset += iResult;
        }
        else
        {
            iOffset = iResult;
            break;
        }
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _CommonMetricsCallback

    \Description
        Callback to format voipserver stats into a datagram to be sent to an
        external metrics aggregator

    \Input *pVoipServer         - [IN] voipserver base ref
    \Input *pServerMetrics      - [IN] servermetrics module state
    \Input *pBuffer             - [IN,OUT] pointer to buffer to format data into
    \Input *pSize               - [IN,OUT] in:size of buffer to format data into, out:amount of bytes written in the buffer
    \Input iMetricIndex         - [IN] what metric index to start with
    \Input *pCommonDimensions   - [IN] common dimensions to tag all metrics with
    \Input pHandlerList         - [IN] handler list
    \Input iHandlerArraySize    - [IN] handler list size

    \Output
        int32_t                 - 0: success and complete;  >0: value to specify in iMetricIndex for next call

    \Notes
        Each single metric consumes a "line" in the text-formatted datagram. The number of dimensions that the metric is
        tagged with, and their respective length, can significantly impact the number of characters consumed by each
        line... and, consequently, the number of metrics that fit in a datagram buffer.

        This function loops through multiple handlers and call them in sequence until the datagram is full... in which
        case the function exits with SERVERMETRICS_ERR_INSUFFICIENT_SPACE and expects to be called again with a
        new buffer to fill in and the metrics index where it needs to resume the packing from.

        For this mechanism to work, it is CRITICAL that each single metric handler limits the number of lines
        it writes into the packet buffer (not more than 4). If a single handler writes too many lines, it may
        immediately exhaust the datagram capacity and that data will never fit into any datagram... resulting in
        the respective metrics to never be reported.

    \Version 02/14/2018 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _CommonMetricsCallback(VoipServerRefT *pVoipServer, ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, const char *pCommonDimensions, CommonMetricHandlerEntryT *pHandlerList, int32_t iHandlerArraySize)
{
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    int32_t iRetCode = 0;   // default to "success and complete"
    int32_t iOffset = 0;
    int32_t iBufSize = *pSize;
    VoipServerStatT VoipServerStats;

    // get stats from voipserver
    VoipServerStatus(pVoipServer, 'stat', 0, &VoipServerStats, sizeof(VoipServerStats));

    while (iMetricIndex < iHandlerArraySize)
    {
        int32_t iResult;
        CommonMetricHandlerEntryT *pEntry = &pHandlerList[iMetricIndex];
        const uint32_t uCurTick = NetTick();

        // set when so we always send the first time
        if (pEntry->uWhen == 0)
        {
            pEntry->uWhen = uCurTick;
        }
        // if it isn't time yet then just move to the next metric
        if (NetTickDiff(uCurTick, pEntry->uWhen) < 0)
        {
            iMetricIndex++;
            continue;
        }

        if ((iResult = pEntry->pMetricHandler(pVoipServer, &VoipServerStats, pEntry->strMetricName, pConfig->eMetricsFormat, pBuffer + iOffset, iBufSize - iOffset, pCommonDimensions)) > 0)
        {
            DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "commonmetrics: adding common metric %s (#%d) to report succeeded\n", (pHandlerList)[iMetricIndex].strMetricName, iMetricIndex);
            iMetricIndex++;
            iOffset += iResult;
            pEntry->uWhen = pEntry->uInterval + uCurTick;
        }
        else if (iResult == 0)
        {
            DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "commonmetrics: adding common metric %s (#%d) to report skipped because metric unavailable\n", (pHandlerList)[iMetricIndex].strMetricName, iMetricIndex);
            iMetricIndex++;
        }
        else if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
        {
            iRetCode = iMetricIndex;
            break;
        }
        else
        {
            DirtyCastLogPrintf("commonmetrics: adding common metric %s (#%d) to report failed with error %d - metric skipped!\n", (pHandlerList)[iMetricIndex].strMetricName, iMetricIndex, iResult);
            iMetricIndex++;
        }
    }

    // let the caller know how many bytes were written to the buffer
    *pSize = iOffset;

    return(iRetCode);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function CommonTrafficMetricsCallback

    \Description
        Callback to format voipserver stats into a datagram to be sent to an
        external metrics aggregator

    \Input *pVoipServer         - [IN] voipserver base ref
    \Input *pServerMetrics      - [IN] servermetrics module state
    \Input *pBuffer             - [IN,OUT] pointer to buffer to format data into
    \Input *pSize               - [IN,OUT] in:size of buffer to format data into, out:amount of bytes written in the buffer
    \Input iMetricIndex         - [IN] what metric index to start with
    \Input *pCommonDimensions   - [IN] common dimensions to tag all metrics with

    \Output
        int32_t                 - 0: success and complete;  >0: value to specify in iMetricIndex for next call

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
int32_t CommonTrafficMetricsCallback(VoipServerRefT *pVoipServer, ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, const char *pCommonDimensions)
{
    return(_CommonMetricsCallback(pVoipServer, pServerMetrics, pBuffer, pSize, iMetricIndex, pCommonDimensions, _CommonMetrics_TrafficMetricHandlers, DIRTYCAST_CalculateArraySize(_CommonMetrics_TrafficMetricHandlers)));
}

/*F********************************************************************************/
/*!
    \Function CommonSystemMetricsCallback

    \Description
    Callback to format voipserver stats into a datagram to be sent to an
    external metrics aggregator

    \Input *pVoipServer         - [IN] voipserver base ref
    \Input *pServerMetrics      - [IN] servermetrics module state
    \Input *pBuffer             - [IN,OUT] pointer to buffer to format data into
    \Input *pSize               - [IN,OUT] in:size of buffer to format data into, out:amount of bytes written in the buffer
    \Input iMetricIndex         - [IN] what metric index to start with
    \Input *pCommonDimensions   - [IN] common dimensions to tag all metrics with

    \Output
    int32_t             - 0: success and complete;  >0: value to specify in iMetricIndex for next call

    \Version 05/02/2017 (cvienneau)
*/
/********************************************************************************F*/
int32_t CommonSystemMetricsCallback(VoipServerRefT *pVoipServer, ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, const char *pCommonDimensions)
{
    return(_CommonMetricsCallback(pVoipServer, pServerMetrics, pBuffer, pSize, iMetricIndex, pCommonDimensions, _CommonMetrics_SystemMetricHandlers, DIRTYCAST_CalculateArraySize(_CommonMetrics_SystemMetricHandlers)));
}

/*F********************************************************************************/
/*!
    \Function CommonTrafficMetricsGetCount

    \Description
        Returns the number of common metrics.

    \Output
        int32_t - metrics count

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************F*/
int32_t CommonTrafficMetricsGetCount(void)
{
    return(DIRTYCAST_CalculateArraySize(_CommonMetrics_TrafficMetricHandlers));
}

/*F********************************************************************************/
/*!
    \Function CommonSystemMetricsGetCount

    \Description
    Returns the number of common metrics.

    \Output
    int32_t - metrics count

    \Version 05/02/2017 (cvienneau)
*/
/********************************************************************************F*/
int32_t CommonSystemMetricsGetCount(void)
{
    return(DIRTYCAST_CalculateArraySize(_CommonMetrics_SystemMetricHandlers));
}
