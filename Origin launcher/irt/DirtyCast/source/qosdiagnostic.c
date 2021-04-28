/*H********************************************************************************/
/*!
    \File qosdiagnostic.c

    \Description
        VoipServer (QOS Mode) HTTP diagnostic page that displays server status summary.

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

        \Version 06/12/2017 (cvienneau)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "commondiagnostic.h"
#include "dirtycast.h"
#include "serveraction.h"
#include "voipserverconfig.h"
#include "voipserver.h"
#include "qosserver.h"
#include "qosdiagnostic.h"

/*** Macros ***********************************************************************/

/*** Variables ********************************************************************/

static const VoipServerDiagnosticMonitorT _MonitorDiagnostics[] =
{
    { VOIPSERVER_MONITORFLAG_PCTSERVERLOAD, "Server CPU percentage is too high." },
    { VOIPSERVER_MONITORFLAG_AVGSYSTEMLOAD, "System load is too high." },
    { VOIPSERVER_MONITORFLAG_UDPRECVQULEN,  "UDP recv queue is almost full, network throughput too high." },
    { VOIPSERVER_MONITORFLAG_UDPSENDQULEN,  "UDP send queue is almost full, network throughput too high." },
};
/*** Private functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _QosServerDiagnosticStatus

    \Description
        Callback to format voip server stats for diagnostic request.

    \Input *pVoipServer     - voip server module state
    \Input *pVoipServer     - QOS module state
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _QosServerDiagnosticStatus(VoipServerRefT *pVoipServer, QosServerRefT *pQosServer, char *pBuffer, int32_t iBufSize)
{
    int32_t iOffset = 0;
    uint32_t uCount;
    char strAddrText[20], strHostName[128] = "";
    VoipServerStatT VoipServerStats;
    QosServerMetricsT QosMetrics;
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    // pull stats from the voip server base
    VoipServerStatus(pVoipServer, 'stat', 0, &VoipServerStats, sizeof(VoipServerStats));
     // pull metrics from QosSever
    QosServerStatus(pQosServer, 'mtrc', 0, &QosMetrics, sizeof(QosMetrics));
    
    // show server info
    VoipServerStatus(pVoipServer, 'host', 0, strHostName, sizeof(strHostName));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "server name: %s\n", strHostName);
    iOffset += ServerDiagnosticFormatUptime(pBuffer+iOffset, iBufSize-iOffset, time(NULL), VoipServerStats.uStartTime);

    // server version info
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "\nBuild\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "version: %s (%s)\n", _SDKVersion, DIRTYCAST_BUILDTYPE);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "time: %s\n", _ServerBuildTime);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "location: %s\n", _ServerBuildLocation);

    //Configuration
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "\nConfig\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "site: %s\n", pConfig->strPingSite);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "frontendaddr: %s\n", SocketInAddrGetText(pConfig->uFrontAddr, strAddrText, sizeof(strAddrText)));
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "probe port: %u\n", pConfig->uApiPort);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "coordinator: %s, %s\n", pConfig->strCoordinatorAddr, QosMetrics.bRegisterdWithCoordinator ? "OK":"DISCONNECTED");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "capacity: %d\n", pConfig->iMaxClients);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "debuglevel: %u\n\n", pConfig->uDebugLevel);

   
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "\nQOS Load\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "last sec:           latency=%llu bwUp=%llu bwDn=%llu unknown=%llu\n", 
        QosMetrics.aPastSecondLoad[PROBE_TYPE_LATENCY], QosMetrics.aPastSecondLoad[PROBE_TYPE_BW_UP], QosMetrics.aPastSecondLoad[PROBE_TYPE_BW_DOWN], QosMetrics.aPastSecondLoad[PROBE_TYPE_VALIDATION_FAILED]);

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "\nQOS Traffic\n");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "packets recv:       latency=%llu bwUp=%llu bwDn=%llu unknown=%llu\n", 
        QosMetrics.aReceivedCount[PROBE_TYPE_LATENCY], QosMetrics.aReceivedCount[PROBE_TYPE_BW_UP], QosMetrics.aReceivedCount[PROBE_TYPE_BW_DOWN], QosMetrics.aReceivedCount[PROBE_TYPE_VALIDATION_FAILED]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bytes recv:         latency=%llu bwUp=%llu bwDn=%llu unknown=%llu\n", 
        QosMetrics.aReceivedBytes[PROBE_TYPE_LATENCY], QosMetrics.aReceivedBytes[PROBE_TYPE_BW_UP], QosMetrics.aReceivedBytes[PROBE_TYPE_BW_DOWN], QosMetrics.aReceivedBytes[PROBE_TYPE_VALIDATION_FAILED]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "packets sent:       latency=%llu bwUp=%llu bwDn=%llu\n", 
        QosMetrics.aSendCountSuccess[PROBE_TYPE_LATENCY], QosMetrics.aSendCountSuccess[PROBE_TYPE_BW_UP], QosMetrics.aSendCountSuccess[PROBE_TYPE_BW_DOWN]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "bytes sent:         latency=%llu bwUp=%llu bwDn=%llu\n", 
        QosMetrics.aSentBytes[PROBE_TYPE_LATENCY], QosMetrics.aSentBytes[PROBE_TYPE_BW_UP], QosMetrics.aSentBytes[PROBE_TYPE_BW_DOWN]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "artificial latency: latency=%llu bwUp=%llu bwDn=%llu\n", 
        QosMetrics.aArtificialLatency[PROBE_TYPE_LATENCY], QosMetrics.aArtificialLatency[PROBE_TYPE_BW_UP], QosMetrics.aArtificialLatency[PROBE_TYPE_BW_DOWN]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "artificial LCount:  latency=%llu bwUp=%llu bwDn=%llu\n", 
        QosMetrics.aArtificialLatencyCount[PROBE_TYPE_LATENCY], QosMetrics.aArtificialLatencyCount[PROBE_TYPE_BW_UP], QosMetrics.aArtificialLatencyCount[PROBE_TYPE_BW_DOWN]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "send failed:        latency=%llu bwUp=%llu bwDn=%llu\n", 
        QosMetrics.aSendCountFailed[PROBE_TYPE_LATENCY], QosMetrics.aSendCountFailed[PROBE_TYPE_BW_UP], QosMetrics.aSendCountFailed[PROBE_TYPE_BW_DOWN]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "extra probes:       latency=%llu bwUp=%llu bwDn=%llu\n", 
        QosMetrics.aExtraProbes[PROBE_TYPE_LATENCY], QosMetrics.aExtraProbes[PROBE_TYPE_BW_UP], QosMetrics.aExtraProbes[PROBE_TYPE_BW_DOWN]);

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "\nSend Failure Error Codes\n");
    if (QosMetrics.aProbeSendErrorCodeCounts[0].uCount == 0)
    {
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "NONE\n");
    }
    for (int32_t i = 0; i < QOS_SERVER_MAX_SEND_ERROR_CODES_MONITORED; i++)
    {
        if (QosMetrics.aProbeSendErrorCodeCounts[i].uCount != 0)
        {
            iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "%5d:        %u\n", QosMetrics.aProbeSendErrorCodeCounts[i].iErrorCode, QosMetrics.aProbeSendErrorCodeCounts[i].uCount);
        }
    }
    
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "\nPacket Distributions");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "\nlatency=    %llu", QosMetrics.aSendPacketCountDistribution[PROBE_TYPE_LATENCY][0]);
    for (uCount = 1; uCount < QOS_COMMON_MAX_PROBE_COUNT; uCount++)
    {
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, ", %llu", QosMetrics.aSendPacketCountDistribution[PROBE_TYPE_LATENCY][uCount]);
    }
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "\nbwUp=       %llu", QosMetrics.aSendPacketCountDistribution[PROBE_TYPE_BW_UP][0]);
    for (uCount = 1; uCount < QOS_COMMON_MAX_PROBE_COUNT; uCount++)
    {
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, ", %llu", QosMetrics.aSendPacketCountDistribution[PROBE_TYPE_BW_UP][uCount]);
    }
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "\nbwDn=       %llu", QosMetrics.aSendPacketCountDistribution[PROBE_TYPE_BW_DOWN][0]);
    for (uCount = 1; uCount < QOS_COMMON_MAX_PROBE_COUNT; uCount++)
    {
        iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, ", %llu", QosMetrics.aSendPacketCountDistribution[PROBE_TYPE_BW_DOWN][uCount]);
    }

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "\n\nPacket Validation Failures\n");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "Protocol:                       %u\n", QosMetrics.ProbeValidationFailures.uProtocol);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "Version:                        %u\n", QosMetrics.ProbeValidationFailures.uVersion);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "FirstHmacFailed:                %u\n", QosMetrics.ProbeValidationFailures.uFirstHmacFailed);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "SecondHmacFailed:               %u\n", QosMetrics.ProbeValidationFailures.uSecondHmacFailed);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "uFromPort:                      %u\n", QosMetrics.ProbeValidationFailures.uFromPort);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ServiceRequestIdTooOld:         %u\n", QosMetrics.ProbeValidationFailures.uServiceRequestIdTooOld);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ProbeCountDownTooHigh:          %u\n", QosMetrics.ProbeValidationFailures.uProbeCountDownTooHigh);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ProbeCountUpTooHigh:            %u\n", QosMetrics.ProbeValidationFailures.uProbeCountUpTooHigh);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ExpectedProbeCountUpTooHigh:    %u\n", QosMetrics.ProbeValidationFailures.uExpectedProbeCountUpTooHigh);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ProbeSizeDownTooLarge:          %u\n", QosMetrics.ProbeValidationFailures.uProbeSizeDownTooLarge);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ProbeSizeDownTooSmall:          %u\n", QosMetrics.ProbeValidationFailures.uProbeSizeDownTooSmall);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ProbeSizeUpTooLarge:            %u\n", QosMetrics.ProbeValidationFailures.uProbeSizeUpTooLarge);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "ProbeSizeUpTooSmall:            %u\n", QosMetrics.ProbeValidationFailures.uProbeSizeUpTooSmall);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "RecvTooFewBytes:                %u\n", QosMetrics.ProbeValidationFailures.uRecvTooFewBytes);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "RecvUnexpectedNumBytes:         %u\n", QosMetrics.ProbeValidationFailures.uRecvUnexpectedNumBytes);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "UnexpectedAddress:              %u\n", QosMetrics.ProbeValidationFailures.uUnexpectedExternalAddress);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "UnexpectedReceiveTime:          %u\n", QosMetrics.ProbeValidationFailures.uUnexpectedServerReceiveTime);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "UnexpectedSendDelta:            %u\n", QosMetrics.ProbeValidationFailures.uUnexpectedServerSendDelta);
        
    // return updated buffer size to caller
    return(iOffset);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function QosServerDiagnosticCallback

    \Description
        Callback to format voip server stats for diagnostic request.

    \Input *pServerDiagnostic - server diagnostic module state
    \Input *pUrl            - request url
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pUserData       - VoipServer module state

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 06/12/2017 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosServerDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData)
{
    int32_t iOffset = 0;
    QosServerRefT *pQosServer = (QosServerRefT *)pUserData;
    VoipServerRefT *pVoipServer = QosServerGetBase(pQosServer);
    const VoipServerConfigT *pConfig = VoipServerGetConfig(pVoipServer);

    if (strncmp(pUrl, "/servulator", 11) == 0)
    {
        iOffset += CommonDiagnosticServulator(pVoipServer, "QOS", pConfig->uApiPort, QosServerStatus(pQosServer, 'load', 0, NULL, 0), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/action", 7) == 0)
    {
        iOffset += CommonDiagnosticAction(pVoipServer, strstr(pUrl, "/action"), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/monitor", 8) == 0)
    {
        iOffset += CommonDiagnosticMonitor(pVoipServer, _MonitorDiagnostics, DIRTYCAST_CalculateArraySize(_MonitorDiagnostics), pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/system", 7) == 0)
    {
        iOffset += CommonDiagnosticSystem(pVoipServer, pConfig->uApiPort, pBuffer+iOffset, iBufSize-iOffset);
    }
    else if (strncmp(pUrl, "/status", 7) == 0)
    {
        iOffset += _QosServerDiagnosticStatus(pVoipServer, pQosServer, pBuffer+iOffset, iBufSize-iOffset);
    }
    else
    {
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"action\">/action</a>      -- perform various admin functions\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "                       (?debuglevel=?       -- set the current logging level)\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "                       (?shutdown=graceful  -- stop accepting clients and shutdown in 1 min)\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "                       (?shutdown=immediate -- exit immediately)\n");
        iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "usage: <a href=\"status\">/status</a>      -- display QOS server status\n");
    }

    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function QosServerDiagnosticGetResponseTypeCb

    \Description
        Get response type based on specified url

    \Input *pServerDiagnostic           - server diagnostic module state
    \Input *pUrl                        - request url

    \Output
        ServerDiagnosticResponseTypeE   - SERVERDIAGNOSTIC_RESPONSETYPE_*

    \Version 06/12/2017 (cvienneau)
*/
/********************************************************************************F*/
ServerDiagnosticResponseTypeE QosServerDiagnosticGetResponseTypeCb(ServerDiagnosticT *pServerDiagnostic, const char *pUrl)
{
    if (strncmp(pUrl, "/servulator", 11) == 0)
    {
        return(SERVERDIAGNOSTIC_RESPONSETYPE_XML);
    }
    return(SERVERDIAGNOSTIC_RESPONSETYPE_DIAGNOSTIC);
}

