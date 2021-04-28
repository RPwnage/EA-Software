/*H********************************************************************************/
/*!
    \File qosmetrics.c

    \Description
        This module implements metrics datagram formatting for the QOS Server

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <stdio.h>
#include <string.h>

#include "qosmetrics.h"
#include "commonmetrics.h"
#include "qosserver.h"
#include "voipserverconfig.h"
#include "voipserver.h"

/*** Macros ***********************************************************************/

// format of the dimensions string, if this value changes to sure to update the size calculation and values where it is used
#define QOSMETRICS_COMMON_DIMENSIONS_FORMAT ("host:%s,port:%d,mode:qos,pool:%s,deployinfo:%s,deployinfo2:%s,site:%s,env:%s")

/*** Type Definitions *************************************************************/

//! metric handler type
typedef int32_t(QosMetricHandlerT)(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

//! per-metric handler
typedef struct QosMetricHandlerEntryT
{
    char strMetricName[64];
    QosMetricHandlerT *pMetricHandler;
} QosMetricHandlerEntryT;


/*** Function Prototypes ***************************************************************/

static int32_t _QosMetricsArtificialLatency(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsArtificialLatencyCount(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsCapacityMax(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsReceivedBytes(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsReceivedPackets(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsReceivedExtra(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsSentPackets(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsSentBytes(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsSendFailed(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsReceivedDistributionIndex0(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsReceivedDistributionBucket0(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsReceivedDistributionBucket1(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsReceivedDistributionBucket2(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailSecondHmac(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailuServiceRequestIdTooOld(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailFromPort(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailRecvUnexpectedNumBytes(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailRecvTooFewBytes(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailUnexpectedAddress(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailUnexpectedReceiveTime(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailUnexpectedSendDelta(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);
static int32_t _QosMetricsPacketValidationFailProbeConstraints(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions);

/*** Variables ********************************************************************/

static QosMetricHandlerEntryT _QosMetrics_MetricHandlers[] =
{
    { "invalid",                                                    NULL },
    { "voipmetrics.qos.artficiallatency",                           _QosMetricsArtificialLatency },
    { "voipmetrics.qos.artficiallatencycount",                      _QosMetricsArtificialLatencyCount },
    { "voipmetrics.qos.capcity.max",                                _QosMetricsCapacityMax },
    { "voipmetrics.qos.received.bytes",                             _QosMetricsReceivedBytes },
    { "voipmetrics.qos.received.packets",                           _QosMetricsReceivedPackets },
    { "voipmetrics.qos.received.extra",                             _QosMetricsReceivedExtra },
    { "voipmetrics.qos.sent.packets",                               _QosMetricsSentPackets },
    { "voipmetrics.qos.sent.bytes",                                 _QosMetricsSentBytes },
    { "voipmetrics.qos.send.failed",                                _QosMetricsSendFailed },
    { "voipmetrics.qos.received.distribution.0",                    _QosMetricsReceivedDistributionIndex0 },
    { "voipmetrics.qos.received.distribution.0-9",                  _QosMetricsReceivedDistributionBucket0 },
    { "voipmetrics.qos.received.distribution.10-19",                _QosMetricsReceivedDistributionBucket1 },
    { "voipmetrics.qos.received.distribution.20-29",                _QosMetricsReceivedDistributionBucket2 },
    { "voipmetrics.qos.packetvalidationfail.SecondHmac",            _QosMetricsPacketValidationFailSecondHmac },
    { "voipmetrics.qos.packetvalidationfail.ServiceRequestIdTooOld",_QosMetricsPacketValidationFailuServiceRequestIdTooOld },
    { "voipmetrics.qos.packetvalidationfail.FromPort",              _QosMetricsPacketValidationFailFromPort },
    { "voipmetrics.qos.packetvalidationfail.RecvUnexpectedNumBytes",_QosMetricsPacketValidationFailRecvUnexpectedNumBytes },
    { "voipmetrics.qos.packetvalidationfail.RecvTooFewBytes",       _QosMetricsPacketValidationFailRecvTooFewBytes },
    { "voipmetrics.qos.packetvalidationfail.UnexpectedAddress",     _QosMetricsPacketValidationFailUnexpectedAddress },
    { "voipmetrics.qos.packetvalidationfail.UnexpectedReceiveTime", _QosMetricsPacketValidationFailUnexpectedReceiveTime },
    { "voipmetrics.qos.packetvalidationfail.UnexpectedSendDelta",   _QosMetricsPacketValidationFailUnexpectedSendDelta },
    { "voipmetrics.qos.packetvalidationfail.ProbeConstraints",      _QosMetricsPacketValidationFailProbeConstraints }
};

//! short strings identifying the probe type
static const char *_strShortQosServerProbeType[] =
{
    "LAT",
    "BWU",
    "BWD",
    "ERR",
    "CNT"
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _QosMetricsReceivedBytes

    \Description
        Adds the "voipmetrics.qos.received.bytes" metric to the specified buffer.
        Metric description: bytes received of each probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsReceivedBytes(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;


    for (uProbeType = 0; uProbeType < PROBE_TYPE_COUNT; uProbeType++)
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosMetrics->aReceivedBytes[uProbeType], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsArtificialLatencyCount

    \Description
        Adds the "voipmetrics.qos.artficiallatencycount" metric to the specified buffer.
        Metric description: number of times artificial latency happened

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsArtificialLatencyCount(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;


    for (uProbeType = 0; uProbeType < PROBE_TYPE_VALIDATION_FAILED; uProbeType++)
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosMetrics->aArtificialLatencyCount[uProbeType], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsArtificialLatency

    \Description
        Adds the "voipmetrics.qos.artficiallatency" metric to the specified buffer.
        Metric description: total amount of artificial latency time

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsArtificialLatency(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;


    for (uProbeType = 0; uProbeType < PROBE_TYPE_VALIDATION_FAILED; uProbeType++)
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosMetrics->aArtificialLatency[uProbeType], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsReceivedPackets

    \Description
        Adds the "voipmetrics.qos.received.packets" metric to the specified buffer.
        Metric description: packets received of each probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsReceivedPackets(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;


    for (uProbeType = 0; uProbeType < PROBE_TYPE_COUNT; uProbeType++)
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosMetrics->aReceivedCount[uProbeType], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsReceivedExtra

    \Description
        Adds the "voipmetrics.qos.received.extra" metric to the specified buffer.
        Metric description: extra packets received of each probe type than what we expected
        probably indicating lost packets and retries.

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsReceivedExtra(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;


    for (uProbeType = 0; uProbeType < PROBE_TYPE_VALIDATION_FAILED; uProbeType++)   //we don't go all the way up to PROBE_TYPE_COUNT because on an error we don't know how many probes we should have expected 
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosMetrics->aExtraProbes[uProbeType], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsSentPackets

    \Description
        Adds the "voipmetrics.qos.sent.packets" metric to the specified buffer.
        Metric description: packets sent of each probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsSentPackets(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;


    for (uProbeType = 0; uProbeType < PROBE_TYPE_VALIDATION_FAILED; uProbeType++)   //we don't go all the way up to PROBE_TYPE_COUNT because we don't attempt to send the validation failed probes 
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosMetrics->aSendCountSuccess[uProbeType], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsSentBytes

    \Description
        Adds the "voipmetrics.qos.sent.bytes" metric to the specified buffer.
        Metric description: bytes sent of each probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsSentBytes(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;


    for (uProbeType = 0; uProbeType < PROBE_TYPE_VALIDATION_FAILED; uProbeType++)   //we don't go all the way up to PROBE_TYPE_COUNT because we don't attempt to send the validation failed probes 
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosMetrics->aSentBytes[uProbeType], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsSendFailed

    \Description
        Adds the "voipmetrics.qos.send.failed" metric to the specified buffer.
        Metric description: packets failed to send of each probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsSendFailed(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;


    for (uProbeType = 0; uProbeType < PROBE_TYPE_VALIDATION_FAILED; uProbeType++)   //we don't go all the way up to PROBE_TYPE_COUNT because we don't attempt to send the validation failed probes 
    {
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, pQosMetrics->aSendCountFailed[uProbeType], SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsReceivedDistributionBuckets

    \Description
        Adds the "voipmetrics.qos.received.distribution.X" metric to the specified buffer.
        Metric description: distribution of indexes of packets received per probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsReceivedDistributionBuckets(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions, uint32_t uBucketStart, uint32_t uBucketEnd)
{
    char strCustomDimensions[512];
    int32_t iErrorCode = 0;
    int32_t iOffset = 0;
    int32_t iResult = 0;
    int32_t uProbeType;

    for (uProbeType = 0; uProbeType < PROBE_TYPE_VALIDATION_FAILED; uProbeType++)   //we don't go all the way up to PROBE_TYPE_COUNT because we don't trust indexes provided by probes that failed validation (instead see validation failures)
    {
        uint32_t uBucketSum = 0;
        uint32_t uIndex = 0;
        ds_snzprintf(strCustomDimensions, sizeof(strCustomDimensions), "%s,probeType:%s", pCommonDimensions, _strShortQosServerProbeType[uProbeType]);
        for (uIndex = uBucketStart; uIndex <= uBucketEnd; uIndex++)
        {
            uBucketSum += pQosMetrics->aSendPacketCountDistribution[uProbeType][uIndex];
        }
        if ((iResult = ServerMetricsFormat(pBuffer + iOffset, iBufSize - iOffset, pMetricName, eFormat, uBucketSum, SERVERMETRICS_TYPE_COUNTER, 100, strCustomDimensions)) > 0)
        {
            // move forward in the buffer
            iOffset += iResult;
        }
        else
        {
            // error - abort!
            iErrorCode = iResult;
            break;
        }
    }

    //there was an error clear everything this function attempted to add
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
    \Function _QosMetricsReceivedDistributionIndex0

    \Description
        Adds the "voipmetrics.qos.received.distribution.0" metric to the specified buffer.
        Metric description: number of packets arriving with packet index 0, a rough estimate on
        how many QOS requests we are getting.

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsReceivedDistributionIndex0(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(_QosMetricsReceivedDistributionBuckets(pQosServer, pQosMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 0, 0));
}


/*F********************************************************************************/
/*!
    \Function _QosMetricsReceivedDistributionBucket0

    \Description
        Adds the "voipmetrics.qos.received.distribution.0-9" metric to the specified buffer.
        Metric description: distribution of indexes of packets received per probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsReceivedDistributionBucket0(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(_QosMetricsReceivedDistributionBuckets(pQosServer, pQosMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 0, 9));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsReceivedDistributionBucket1

    \Description
        Adds the "voipmetrics.qos.received.distribution.10-19" metric to the specified buffer.
        Metric description: distribution of indexes of packets received per probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsReceivedDistributionBucket1(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(_QosMetricsReceivedDistributionBuckets(pQosServer, pQosMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 10, 19));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsReceivedDistributionBucket2

    \Description
        Adds the "voipmetrics.qos.received.distribution.20-29" metric to the specified buffer.
        Metric description: distribution of indexes of packets received per probe type

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsReceivedDistributionBucket2(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(_QosMetricsReceivedDistributionBuckets(pQosServer, pQosMetrics, pMetricName, eFormat, pBuffer, iBufSize, pCommonDimensions, 20, 29));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsCapacityMax

    \Description
        Adds the "voipmetrics.qos.capcity.max" metric to the specified buffer.
        Metric description: number of simultaneous qos clients we expect to be able to handle

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
    int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsCapacityMax(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->bRegisterdWithCoordinator ? pQosMetrics->uCapacityMax : 0, SERVERMETRICS_TYPE_GAUGE, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailSecondHmac

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.SecondHmac" metric to the specified buffer.
        Metric description: number of packets that have failed validation for this reason

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailSecondHmac(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->ProbeValidationFailures.uSecondHmacFailed, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailuServiceRequestIdTooOld

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.ServiceRequestIdTooOld" metric to the specified buffer.
        Metric description: number of packets that have failed validation for this reason

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailuServiceRequestIdTooOld(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->ProbeValidationFailures.uServiceRequestIdTooOld, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailFromPort

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.FromPort" metric to the specified buffer.
        Metric description: number of packets that have failed validation for this reason

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailFromPort(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->ProbeValidationFailures.uFromPort, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailRecvTooFewBytes

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.RecvTooFewBytes" metric to the specified buffer.
        Metric description: number of packets that have failed validation for this reason

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 08/09/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailRecvTooFewBytes(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->ProbeValidationFailures.uRecvTooFewBytes, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailRecvUnexpectedNumBytes

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.RecvUnexpectedNumBytes" metric to the specified buffer.
        Metric description: number of packets that have failed validation for this reason

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailRecvUnexpectedNumBytes(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->ProbeValidationFailures.uRecvUnexpectedNumBytes, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailUnexpectedAddress

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.UnexpectedAddress" metric to the specified buffer.
        Metric description: number of packets that have failed validation for this reason

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailUnexpectedAddress(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->ProbeValidationFailures.uUnexpectedExternalAddress, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailUnexpectedReceiveTime

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.UnexpectedReceiveTime" metric to the specified buffer.
        Metric description: number of packets that have failed validation for this reason

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailUnexpectedReceiveTime(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->ProbeValidationFailures.uUnexpectedServerReceiveTime, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailUnexpectedSendDelta

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.UnexpectedSendDelta" metric to the specified buffer.
        Metric description: number of packets that have failed validation for this reason

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/25/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailUnexpectedSendDelta(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, pQosMetrics->ProbeValidationFailures.uUnexpectedServerSendDelta, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*F********************************************************************************/
/*!
    \Function _QosMetricsPacketValidationFailProbeConstraints

    \Description
        Adds the "voipmetrics.qos.packetvalidationfail.ProbeConstraints" metric to the specified buffer.
        Metric description: number of packets that have failed validation because they attempt to go beyond expected ranges.

    \Input *pQosServer          - module state
    \Input *pQosMetrics         - stats from QosServer
    \Input *pMetricName         - metric name
    \Input eFormat              - metric report format
    \Input *pBuffer             - pointer to buffer to format data into
    \Input iBufSize             - buffer size
    \Input *pCommonDimensions   - dimensions common to all metrics

    \Output
        int32_t                 - >=0: success (number of bytes written);  <0: SERVERMETRICS_ERR_XXX

    \Version 01/25/2018 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosMetricsPacketValidationFailProbeConstraints(QosServerRefT *pQosServer, QosServerMetricsT *pQosMetrics, const char *pMetricName, ServerMetricsFormatE eFormat, char *pBuffer, int32_t iBufSize, const char *pCommonDimensions)
{
    uint32_t uTotalFailures = pQosMetrics->ProbeValidationFailures.uProtocol
        + pQosMetrics->ProbeValidationFailures.uVersion
        + pQosMetrics->ProbeValidationFailures.uProbeSizeUpTooSmall
        + pQosMetrics->ProbeValidationFailures.uProbeSizeDownTooSmall
        + pQosMetrics->ProbeValidationFailures.uProbeSizeUpTooLarge
        + pQosMetrics->ProbeValidationFailures.uProbeSizeDownTooLarge
        + pQosMetrics->ProbeValidationFailures.uProbeCountDownTooHigh
        + pQosMetrics->ProbeValidationFailures.uProbeCountUpTooHigh
        + pQosMetrics->ProbeValidationFailures.uExpectedProbeCountUpTooHigh;
    return(ServerMetricsFormat(pBuffer, iBufSize, pMetricName, eFormat, uTotalFailures, SERVERMETRICS_TYPE_COUNTER, 100, pCommonDimensions));
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function QosMetricsCallback

    \Description
        Callback to format voip server stats into a datagram to be sent to an
        external metrics aggregation

    \Input *pServerMetrics  - [IN] servermetrics module state
    \Input *pBuffer         - [IN,OUT] pointer to buffer to format data into
    \Input *pSize           - [IN,OUT] in:size of buffer to format data into, out:amount of bytes written in the buffer
    \Input iMetricIndex     - [IN] what metric index to start with
    \Input *pUserData       - [IN]VoipServer module state

    \Output
        int32_t             - 0: success and complete;  >0: value to specify in iMetricIndex for next call

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

    \Version 05/01/2017 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData)
{
    QosServerRefT *pQosServer = (QosServerRefT *)pUserData;
    VoipServerRefT *pVoipServer = QosServerGetBase(pQosServer);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);
    int32_t iRetCode = 0;   // default to "success and complete"
    int32_t iOffset = 0;
    int32_t iBufSize = *pSize;
    char strHostName[128];
    QosServerMetricsT QosServerMetrics;
    char strCommonDimensions[sizeof(QOSMETRICS_COMMON_DIMENSIONS_FORMAT) +  // size of the format string
                             sizeof(strHostName) +                          // size of the hostname
                             5 +                                            // size of the port
                             VOIPSERVERCONFIG_POOL_SIZE +                   // size of the pool value
                             VOIPSERVERCONFIG_DEPLOYINFO_SIZE +             // size of deployinfo value
                             VOIPSERVERCONFIG_DEPLOYINFO2_SIZE +            // size of deployinfo value 2
                             VOIPSERVERCONFIG_SITE_SIZE +                   // size of site value
                             VOIPSERVERCONFIG_ENV_SIZE +                    // size of env value
                             1];                                            // NULL


    //describe the order the different metrics sources are added to our list
    const int32_t iSystemMetricsStart = 0;
    const int32_t iQosMetricsStart = CommonSystemMetricsGetCount();

    VoipServerStatus(pVoipServer, 'host', 0, strHostName, sizeof(strHostName));
 
    // prepare dimensions to be associated with all metrics
    ds_memclr(strCommonDimensions, sizeof(strCommonDimensions));
    ds_snzprintf(strCommonDimensions, sizeof(strCommonDimensions), QOSMETRICS_COMMON_DIMENSIONS_FORMAT,
                                                                   strHostName,
                                                                   pConfig->uDiagnosticPort,
                                                                   pConfig->strPool,
                                                                   pConfig->strDeployInfo,
                                                                   pConfig->strDeployInfo2,
                                                                   pConfig->strPingSite,
                                                                   pConfig->strEnvironment);
    
    //get the delta stats from QOS module
    QosServerStatus(pQosServer, 'mdlt', 0, &QosServerMetrics, sizeof(QosServerMetrics));

    // is iMetricIndex pointing to a "Common System" metric?
    if ((iMetricIndex >= iSystemMetricsStart) && (iMetricIndex < iQosMetricsStart))
    {
        if (iMetricIndex == iSystemMetricsStart)   //the first index of a set is invalid
        {
            iMetricIndex++;
        }
        if ((iRetCode = CommonSystemMetricsCallback(pVoipServer, pServerMetrics, pBuffer, pSize, iMetricIndex - iSystemMetricsStart, strCommonDimensions)) == 0)
        {
            // we are done with common system metrics, make sure we now take care of qos-specific metrics
            iMetricIndex = iRetCode = iQosMetricsStart;
            iOffset = *pSize;
        }
    }

    // is iMetricIndex pointing to a "QOS-specific" metric?
    else if (iMetricIndex >= iQosMetricsStart)
    {
        // *** QOS-specific metrics ***

        // rescale the metric index
        iMetricIndex -= iQosMetricsStart;
        if (iMetricIndex == 0)
        {
            iMetricIndex++;
        }

        // loop through
        while (iMetricIndex < (signed)DIRTYCAST_CalculateArraySize(_QosMetrics_MetricHandlers))
        {
            int32_t iResult;
            QosMetricHandlerEntryT *pEntry = &_QosMetrics_MetricHandlers[iMetricIndex];

            if ((iResult = pEntry->pMetricHandler(pQosServer, &QosServerMetrics, pEntry->strMetricName, pConfig->eMetricsFormat, pBuffer + iOffset, iBufSize - iOffset, strCommonDimensions)) > 0)
            {
                DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "qosmetrics: adding QOS metric %s (#%d) to report succeeded\n", _QosMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);
                iMetricIndex++;
                iOffset += iResult;
            }
            else if (iResult == 0)
            {
                DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 3, "qosmetrics: adding QOS metric %s (#%d) to report skipped because metric unavailable\n", _QosMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex);
                iMetricIndex++;
            }
            else if (iResult == SERVERMETRICS_ERR_INSUFFICIENT_SPACE)
            {
                // return "scaled" metric index
                iRetCode = iMetricIndex + iQosMetricsStart;
                break;
            }
            else
            {
                DirtyCastLogPrintf("qosmetrics: adding QOS metric %s (#%d) to report failed with error %d - metric skipped!\n", _QosMetrics_MetricHandlers[iMetricIndex].strMetricName, iMetricIndex, iResult);
                iMetricIndex++;
            }
        }

        // let the caller know how many bytes were written to the buffer
        *pSize = iOffset;

        // if all metrics have been dealt with, then let the call know
        if (iMetricIndex == (sizeof(_QosMetrics_MetricHandlers) / sizeof(_QosMetrics_MetricHandlers[0])))
        {
            iRetCode = 0;
        }
    }

    if (iRetCode == 0)
    {
        // tell the server to reset stats
        VoipServerControl(pVoipServer, 'stat', TRUE, 0, NULL);
        QosServerControl(pQosServer, 'mset', 0, 0, NULL);
    }

    return(iRetCode);
}

