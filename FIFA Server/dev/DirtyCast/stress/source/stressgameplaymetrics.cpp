/*H********************************************************************************/
/*!
    \File stressgameplaymetrics.c

    \Description
        This module implements OIv2 metrics for the stress client

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 07/20/2019 (cvienneau)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "DirtySDK/platform.h"
#include "dirtycast.h"
#include "servermetrics.h"
#include "stressgameplaymetrics.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "stressblaze.h"
#include "stress.h"

/*** Macros ***********************************************************************/

// format of the dimensions string, if this value changes to sure to update the size calculation and values where it is used
#define STRESS_GAMEPLAY_METRICS_COMMON_DIMENSIONS_FORMAT ("host:%s,port:%d,mode:%d,deployinfo:%s,deployinfo2:%s,product:%s,env:%s,gpvs:%s")

//! allocation identifier
#define STRESS_GAMEPLAY_MEMID        ('gstr')

/*** Type Definitions *************************************************************/

//! manage the metrics system
typedef struct StressGameplayMetricsRefT
{
    StressGameplayMetricDataT MetricsData;
    StressGameplayCountMetricDataT PreviousCountValues;
    StressBlazeC::StressBlazeStateE eLastReportedState;

    StressConfigC *pConfig;
    ServerMetricsT *pServerMetrics;
    void *pMemGroupUserData;
    int32_t iMemGroup;
} StressGameplayMetricsRefT;

//! metric handler type
typedef int32_t(StressGameplayMetricsHandlerT)(StressGameplayMetricsRefT *pStressMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

//! per-metric handler
typedef struct StressGameplayMetricsHandlerEntryT
{
    char strMetricName[64];
    StressGameplayMetricsHandlerT *pMetricHandler;
} StressGameplayMetricsHandlerEntryT;


/*** Function Prototypes ***************************************************************/
static int32_t _StressGameplayMetricsState(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsNumPlayers(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsLatency(StressGameplayMetricsRefT *pGameMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsLatencyPackets(StressGameplayMetricsRefT *pGameMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsSentBps(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceivedBps(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsSentPps(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceivedPps(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsSendQueueLength(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceiveQueueLength(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsProcessTimeMax(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsProcessTimeMin(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsProcessTimeAverage(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsSentInputs(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceivedInputs(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsInputsSentQueued(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsInputsReceivedQueued(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

static int32_t _StressGameplayMetricsSentBytes(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceivedBytes(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsSentPackets(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceivedPackets(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsSentNaks(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceivedNaks(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceivedPacketsLost(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsReceivedPacketsSaved(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsSentPacketsLost(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _StressGameplayMetricsDroppedInputs(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

/*** Variables ********************************************************************/

static const StressGameplayMetricsHandlerEntryT _StressGameplayMetricsHandlers[] =
{
    { "invalid",                NULL },
    { "gamebot.state",          _StressGameplayMetricsState },
    { "gamebot.numplayers",     _StressGameplayMetricsNumPlayers },
    { "gamebot.late",           _StressGameplayMetricsLatency },
    { "gamebot.latepack",       _StressGameplayMetricsLatencyPackets },
    { "gamebot.sentbps",        _StressGameplayMetricsSentBps },
    { "gamebot.rcvdbps",        _StressGameplayMetricsReceivedBps },
    { "gamebot.sentpps",        _StressGameplayMetricsSentPps },
    { "gamebot.rcvdpps",        _StressGameplayMetricsReceivedPps },
    { "gamebot.sendqlen",       _StressGameplayMetricsSendQueueLength },
    { "gamebot.rcvdqlen",       _StressGameplayMetricsReceiveQueueLength },
    { "gamebot.proctmax",       _StressGameplayMetricsProcessTimeMax },
    { "gamebot.proctmin",       _StressGameplayMetricsProcessTimeMin },
    { "gamebot.proctavg",       _StressGameplayMetricsProcessTimeAverage },
    { "gamebot.sentinputs",     _StressGameplayMetricsSentInputs },
    { "gamebot.rcvdinputs",     _StressGameplayMetricsReceivedInputs },
    { "gamebot.isqueue",        _StressGameplayMetricsInputsSentQueued },
    { "gamebot.irqueue",        _StressGameplayMetricsInputsReceivedQueued },

    { "gamebot.sentbyte",       _StressGameplayMetricsSentBytes },
    { "gamebot.rcvdbyte",       _StressGameplayMetricsReceivedBytes },
    { "gamebot.sentpack",       _StressGameplayMetricsSentPackets },
    { "gamebot.rcvdpack",       _StressGameplayMetricsReceivedPackets },
    { "gamebot.sentnak",        _StressGameplayMetricsSentNaks },
    { "gamebot.rcvdnak",        _StressGameplayMetricsReceivedNaks },
    { "gamebot.rcvpacklost",    _StressGameplayMetricsReceivedPacketsLost },
    { "gamebot.rcvpacksave",    _StressGameplayMetricsReceivedPacketsSaved },
    { "gamebot.sentpacklost",   _StressGameplayMetricsSentPacketsLost },
    { "gamebot.droppedinput",   _StressGameplayMetricsDroppedInputs }
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceivedPps

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceivedPps(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uReceivedPps, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsSendQueueLength

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsSendQueueLength(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uSendQueueLength, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceiveQueueLength

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceiveQueueLength(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uReceiveQueueLength, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsProcessTimeMax

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsProcessTimeMax(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uProcessingTimeMaxMs, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsProcessTimeMin

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsProcessTimeMin(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uProcessingTimeMinMs, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsProcessTimeAverage

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsProcessTimeAverage(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uProcessingTimeAvgMs, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsSentInputs

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsSentInputs(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uSentInputs, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceivedInputs

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceivedInputs(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uReceivedInputs, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsInputsSentQueued

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsInputsSentQueued(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uInputsSentQueued, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsInputsReceivedQueued

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsInputsReceivedQueued(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uInputsReceivedQueued, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsSentPps

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsSentPps(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uSentPps, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceivedBps

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceivedBps(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uReceivedBps, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsSentBps

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsSentBps(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uSentBps, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsLatency

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsLatency(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uLatencyMs, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsLatencyPackets

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsLatencyPackets(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Gauge.uLatencyPackets, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsSentBytes

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsSentBytes(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uSentBytes, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceivedBytes

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceivedBytes(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uReceivedBytes, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsSentPackets

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsSentPackets(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uSentPackets, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceivedPackets

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceivedPackets(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uReceivedPackets, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsSentNaks

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsSentNaks(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uSentNak, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceivedNaks

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceivedNaks(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uReceivedNak, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceivedPacketsLost

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceivedPacketsLost(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uReceivedPacketsLost, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsReceivedPacketsSaved

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsReceivedPacketsSaved(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uReceivedPacketsSaved, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsSentPacketsLost

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsSentPacketsLost(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uSentPacketsLost, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsDroppedInputs

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsDroppedInputs(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pStressGameplayMetrics->MetricsData.Count.uDroppedInputs, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsNumPlayers

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsNumPlayers(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, StressBlazeC::Get()->GetPlayerCount(), SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsState

    \Description
        Adds the metric to the specified buffer.

    \Input *pStressGameplayMetrics  - module state
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/17/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsState(StressGameplayMetricsRefT *pStressGameplayMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;

    StressBlazeC::StressBlazeStateE eNewState = StressBlazeC::Get()->GetState();

    // clear off the old state, this will make the graph more responsive to the change
    if (pStressGameplayMetrics->eLastReportedState != eNewState)
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,state:%s", pCommonDimensions, StressBlazeC::GetStateText(pStressGameplayMetrics->eLastReportedState));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, 0, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }
    }

    // setup the new state value
    if (iErrorCode == 0)
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,state:%s", pCommonDimensions, StressBlazeC::GetStateText(eNewState));
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, 1, SERVERMETRICS_TYPE_GAUGE, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
        }
    }


    //there was an error clear everything this function attempted to add
    if (iErrorCode != 0)
    {
        iOffset = iErrorCode;

        // reset user buffer
        ds_memclr(pBuffer, iBufSize);
    }
    else
    {
        pStressGameplayMetrics->eLastReportedState = eNewState;
    }
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsState

    \Description
        Clear the deltas calculated for the counters

    \Input *pStressGameplayMetrics  - module state

    \Version 08/01/2019 (cvienneau)
*/
/********************************************************************************F*/
static void _ResetMetrics(StressGameplayMetricsRefT *pStressGameplayMetrics)
{
    //clear everything that has state (only the counts should matter)
    ds_memclr(&pStressGameplayMetrics->MetricsData.Count, sizeof(pStressGameplayMetrics->MetricsData.Count));
}

/*F********************************************************************************/
/*!
    \Function _StressGameplayMetricsCallback

    \Description
        Callback to format voip server stats into a datagram to be sent to an
        external metrics aggregation

    \Input *pServerMetrics  - [IN] servermetrics module state
    \Input *pBuffer         - [IN,OUT] pointer to buffer to format data into
    \Input *pSize           - [IN,OUT] in:size of buffer to format data into, out:amount of bytes written in the buffer
    \Input iMetricIndex     - [IN] what metric index to start with
    \Input *pUserData       - [IN]StressGameplayMetricsRefT module state

    \Output
    int32_t             - 0: success and complete;  >0: value to specify in iMetricIndex for next call

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _StressGameplayMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData)
{
    StressGameplayMetricsRefT *pStressGameplayMetrics = (StressGameplayMetricsRefT *)pUserData;
    int32_t iRetCode = 0;   // default to "success and complete"
    int32_t iOffset = 0;
    int32_t iBufSize = *pSize;
    char strCommonDimensions[sizeof(STRESS_GAMEPLAY_METRICS_COMMON_DIMENSIONS_FORMAT) +  // size of the format string
        sizeof(pStressGameplayMetrics->pConfig->strLocalHostName) +                      // size of the hostname
        5 +                                                                              // size of the port
        1 +                                                                              // size of the mode
        sizeof(pStressGameplayMetrics->pConfig->strDeployInfo) +                         // size of deployinfo value
        sizeof(pStressGameplayMetrics->pConfig->strDeployInfo2) +                        // size of deployinfo value 2
        sizeof(pStressGameplayMetrics->pConfig->strService) +                            // size of site value
        sizeof(pStressGameplayMetrics->pConfig->strEnvironment) +                        // size of env value
        sizeof(pStressGameplayMetrics->pConfig->strGameProtocolVersion) +                // size of the gpvs value
        1];                                            // NULL

    // prepare dimensions to be associated with all metrics
    ds_memclr(strCommonDimensions, sizeof(strCommonDimensions));
    ds_snzprintf(strCommonDimensions, sizeof(strCommonDimensions), STRESS_GAMEPLAY_METRICS_COMMON_DIMENSIONS_FORMAT,
        pStressGameplayMetrics->pConfig->strLocalHostName,
        pStressGameplayMetrics->pConfig->uGamePort,
        pStressGameplayMetrics->pConfig->uGameMode,
        pStressGameplayMetrics->pConfig->strDeployInfo,
        pStressGameplayMetrics->pConfig->strDeployInfo2,
        pStressGameplayMetrics->pConfig->strService,
        pStressGameplayMetrics->pConfig->strEnvironment,
        // Blaze::GameManager::GAME_PROTOCOL_VERSION_MATCH_ANY = "match_any_protocol" but I don't want to create a BlazeSDK dependency in t his file
        pStressGameplayMetrics->pConfig->strGameProtocolVersion[0] != '\0' ? pStressGameplayMetrics->pConfig->strGameProtocolVersion : "match_any_protocol");

    // for now StressGameplayMetrics doesn't make use of any common metrics, so we just loop through our own
    while (iMetricIndex < (signed)DIRTYCAST_CalculateArraySize(_StressGameplayMetricsHandlers))
    {
        int32_t iResult;
        const StressGameplayMetricsHandlerEntryT *pEntry = &_StressGameplayMetricsHandlers[iMetricIndex];

        if ((iResult = pEntry->pMetricHandler(pStressGameplayMetrics, pEntry->strMetricName, pStressGameplayMetrics->pConfig->eMetricsFormat, pBuffer + iOffset, iBufSize - iOffset, strCommonDimensions)) > 0)
        {
            DirtyCastLogPrintfVerbose(pStressGameplayMetrics->pConfig->uDebugLevel, 3, "stressgameplaymetrics: adding metric %s (#%d) to report succeeded\n", _StressGameplayMetricsHandlers[iMetricIndex].strMetricName, iMetricIndex);
            iMetricIndex++;
            iOffset += iResult;
        }
        else if (iResult == 0)
        {
            DirtyCastLogPrintfVerbose(pStressGameplayMetrics->pConfig->uDebugLevel, 3, "stressgameplaymetrics: adding metric %s (#%d) to report skipped because metric unavailable\n", _StressGameplayMetricsHandlers[iMetricIndex].strMetricName, iMetricIndex);
            iMetricIndex++;
        }
        else if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
        {
            DirtyCastLogPrintfVerbose(pStressGameplayMetrics->pConfig->uDebugLevel, 3, "stressgameplaymetrics: adding metric %s (#%d) to report failed because it did not fit - back buffer expansion required\n", _StressGameplayMetricsHandlers[iMetricIndex].strMetricName, iMetricIndex);

            // return "scaled" metric index
            iRetCode = iMetricIndex;
            break;
        }
        else
        {
            StressPrintf("stressgameplaymetrics: adding metric %s (#%d) to report failed with error %d - metric skipped!\n", _StressGameplayMetricsHandlers[iMetricIndex].strMetricName, iMetricIndex, iResult);
            iMetricIndex++;
        }
    }

    // let the caller know how many bytes were written to the buffer
    *pSize = iOffset;

    // if all metrics have been dealt with, then let the call know
    if (iMetricIndex == DIRTYCAST_CalculateArraySize(_StressGameplayMetricsHandlers))
    {
        iRetCode = 0;
        _ResetMetrics(pStressGameplayMetrics);
    }

    return(iRetCode);
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function StressGameplayMetricsCreate

    \Description
        Create the StressGameplayMetricsRefT module.

    \Input *mpConfig   - the config used for the stress client

    \Output
    StressGameplayMetricsRefT* - state pointer on success, NULL on failure

    \Version 04/20/2018 (cvienneau)
*/
/********************************************************************************F*/
StressGameplayMetricsRefT* StressGameplayMetricsCreate(StressConfigC *mpConfig)
{
    StressGameplayMetricsRefT *pStressGameplayMetrics;
    void *pMemGroupUserData;
    int32_t iMemGroup;

    // Query current memory group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    DirtyMemGroupEnter(iMemGroup, pMemGroupUserData);

    // early out if configs aren't appropriate
    if (mpConfig->uMetricsAggregatorPort == 0)
    {
        DirtyMemGroupLeave();
        StressPrintf("stressgameplaymetrics: creation skipped, 0 port used.\n");
        return(NULL);
    }

    // allocate and init module state
    if ((pStressGameplayMetrics = (StressGameplayMetricsRefT*)DirtyMemAlloc(sizeof(*pStressGameplayMetrics), STRESS_GAMEPLAY_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyMemGroupLeave();
        StressPrintf("stressgameplaymetrics: could not allocate module state.\n");
        return(NULL);
    }
    ds_memclr(pStressGameplayMetrics, sizeof(*pStressGameplayMetrics));

    // create module to report metrics to an external metrics aggregator
    if ((pStressGameplayMetrics->pServerMetrics = ServerMetricsCreate(mpConfig->strMetricsAggregatorHost, mpConfig->uMetricsAggregatorPort, mpConfig->uMetricsPushingRate, mpConfig->eMetricsFormat, &_StressGameplayMetricsCallback, pStressGameplayMetrics)) != NULL)
    {
        StressPrintf("stressgameplaymetrics: created module for reporting metrics to external aggregator (report rate = %d ms)\n", mpConfig->uMetricsPushingRate);
        ServerMetricsControl(pStressGameplayMetrics->pServerMetrics, 'spam', (int32_t)mpConfig->uDebugLevel, 0, NULL);
    }
    else
    {
        StressGameplayMetricsDestroy(pStressGameplayMetrics);
        StressPrintf("stressgameplaymetrics: unable to create module for reporting metrics to external aggregator\n");
        return(NULL);
    }

    //init
    pStressGameplayMetrics->pMemGroupUserData = pMemGroupUserData;
    pStressGameplayMetrics->iMemGroup = iMemGroup;
    pStressGameplayMetrics->pConfig = mpConfig;
    _ResetMetrics(pStressGameplayMetrics);

    return(pStressGameplayMetrics);
}

/*F********************************************************************************/
/*!
    \Function StressGameplayMetricsDestroy

    \Description
        Destroy the StressGameplayMetricsRefT module.

    \Input *pStressGameplayMetrics - module state

    \Version 04/20/2018 (cvienneau)
*/
/********************************************************************************F*/
void StressGameplayMetricsDestroy(StressGameplayMetricsRefT *pStressGameplayMetrics)
{
    // release module memory
    if (pStressGameplayMetrics->pServerMetrics != NULL)
    {
        ServerMetricsDestroy(pStressGameplayMetrics->pServerMetrics);
    }
    DirtyMemFree(pStressGameplayMetrics, STRESS_GAMEPLAY_MEMID, pStressGameplayMetrics->iMemGroup, pStressGameplayMetrics->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function StressGameplayMetricsUpdate

    \Description
        Service communication with the metrics agent

    \Input *pMetrics  - module state

    \Version 04/20/2018 (cvienneau)
*/
/********************************************************************************F*/
void StressGameplayMetricsUpdate(StressGameplayMetricsRefT *pMetrics)
{
    if (pMetrics != NULL)
    {
        ServerMetricsUpdate(pMetrics->pServerMetrics);
    }
}

/*F********************************************************************************/
/*!
    \Function StressGameplayMetricData

    \Description
        Apply new metrics information, we will report this information on the next push.

    \Input *pMetrics  - module state
    \Input *pResults  - metrics results gathered

    \Version 07/20/2019 (cvienneau)
*/
/********************************************************************************F*/
void StressGameplayMetricData(StressGameplayMetricsRefT *pMetrics, StressGameplayMetricDataT *pResults)
{
    StressGameplayCountMetricDataT countDelta;
    ds_memclr(&countDelta, sizeof(countDelta));
    
    // apply the gauge as a strait copy
    pMetrics->MetricsData.Gauge = pResults->Gauge;

    // go through all the elements of pResults->Count and find the delta since last time we updated
    // when we start a new game these metrics have a hicup, only update the metric if we'll have a postive result
    countDelta.uSentBytes = (pResults->Count.uSentBytes > pMetrics->PreviousCountValues.uSentBytes) ? pResults->Count.uSentBytes - pMetrics->PreviousCountValues.uSentBytes : 0;
    countDelta.uReceivedBytes = (pResults->Count.uReceivedBytes > pMetrics->PreviousCountValues.uReceivedBytes) ? pResults->Count.uReceivedBytes - pMetrics->PreviousCountValues.uReceivedBytes : 0;
    countDelta.uSentPackets = (pResults->Count.uSentPackets > pMetrics->PreviousCountValues.uSentPackets) ? pResults->Count.uSentPackets - pMetrics->PreviousCountValues.uSentPackets : 0;
    countDelta.uReceivedPackets = (pResults->Count.uReceivedPackets > pMetrics->PreviousCountValues.uReceivedPackets) ? pResults->Count.uReceivedPackets - pMetrics->PreviousCountValues.uReceivedPackets : 0;
    countDelta.uSentNak = (pResults->Count.uSentNak > pMetrics->PreviousCountValues.uSentNak) ? pResults->Count.uSentNak - pMetrics->PreviousCountValues.uSentNak : 0;
    countDelta.uReceivedNak = (pResults->Count.uReceivedNak > pMetrics->PreviousCountValues.uReceivedNak) ? pResults->Count.uReceivedNak - pMetrics->PreviousCountValues.uReceivedNak : 0;
    countDelta.uReceivedPacketsLost = (pResults->Count.uReceivedPacketsLost > pMetrics->PreviousCountValues.uReceivedPacketsLost) ? pResults->Count.uReceivedPacketsLost - pMetrics->PreviousCountValues.uReceivedPacketsLost : 0;
    countDelta.uReceivedPacketsSaved = (pResults->Count.uReceivedPacketsSaved > pMetrics->PreviousCountValues.uReceivedPacketsSaved) ? pResults->Count.uReceivedPacketsSaved - pMetrics->PreviousCountValues.uReceivedPacketsSaved : 0;
    countDelta.uSentPacketsLost = (pResults->Count.uSentPacketsLost > pMetrics->PreviousCountValues.uSentPacketsLost) ? pResults->Count.uSentPacketsLost - pMetrics->PreviousCountValues.uSentPacketsLost : 0;
    countDelta.uDroppedInputs = (pResults->Count.uDroppedInputs > pMetrics->PreviousCountValues.uDroppedInputs) ? pResults->Count.uDroppedInputs - pMetrics->PreviousCountValues.uDroppedInputs : 0;

    // apply the delta to the metrics we plan to report at our next push
    pMetrics->MetricsData.Count.uSentBytes += countDelta.uSentBytes;
    pMetrics->MetricsData.Count.uReceivedBytes += countDelta.uReceivedBytes;
    pMetrics->MetricsData.Count.uSentPackets += countDelta.uSentPackets;
    pMetrics->MetricsData.Count.uReceivedPackets += countDelta.uReceivedPackets;
    pMetrics->MetricsData.Count.uSentNak += countDelta.uSentNak;
    pMetrics->MetricsData.Count.uReceivedNak += countDelta.uReceivedNak;
    pMetrics->MetricsData.Count.uReceivedPacketsLost += countDelta.uReceivedPacketsLost;
    pMetrics->MetricsData.Count.uReceivedPacketsSaved += countDelta.uReceivedPacketsSaved;
    pMetrics->MetricsData.Count.uSentPacketsLost += countDelta.uSentPacketsLost;
    pMetrics->MetricsData.Count.uDroppedInputs += countDelta.uDroppedInputs;

    // store the previous values for the next time the delta needs to be calculated
    pMetrics->PreviousCountValues = pResults->Count;
}



