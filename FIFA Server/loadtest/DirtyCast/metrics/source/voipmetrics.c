/*H********************************************************************************/
/*!
    \File voipmetrics.

    \Description
        Routines for managing voipserver metrics.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 08/26/2010 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/xml/xmlparse.h"
#include "LegacyDirtySDK/util/tagfield.h"

#include "voipmetrics.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _VoipMetricsXmlAttribGetPct

    \Description
        Get a percent attribute in %d.%d format, and return percent*100.

    \Input *pXml            - pointer to xml element
    \Input *pAttrib         - attribute name
    \Input iDefault         - default value

    \Output
        int32_t             - percent*100

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipMetricsXmlAttribGetPct(const char *pXml, const char *pAttrib, int32_t iDefault)
{
    char strBuf[32];
    int32_t iPct, iPctFrac, iResult = iDefault;

    if ((XmlAttribGetString(pXml, pAttrib, strBuf, sizeof(strBuf), "") != -1) && (strBuf[0] != '\0'))
    {
        sscanf(strBuf, "%d.%d", &iPct, &iPctFrac);
        iResult = (iPct * 100) + iPctFrac;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipMetricsXmlAttribGetTime

    \Description
        Get a time attribute in hour:min:sec format, and return seconds.

    \Input *pXml            - pointer to xml element
    \Input *pAttrib         - attribute name
    \Input iDefault         - default value

    \Output
        int32_t             - number of seconds input time represents

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipMetricsXmlAttribGetTime(const char *pXml, const char *pAttrib, int32_t iDefault)
{
    char strBuf[32];
    int32_t iHours, iMinutes, iSeconds, iResult = iDefault;

    if ((XmlAttribGetString(pXml, pAttrib, strBuf, sizeof(strBuf), "") != -1) && (strBuf[0] != '\0'))
    {
        sscanf(strBuf, "%d:%d:%d", &iHours, &iMinutes, &iSeconds);
        iResult = (iHours * 60 * 60) + (iMinutes * 60) + iSeconds;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _VoipMetricsFormatXmlGameServer

    \Description
        Format voipserver metrics into xml output (gameserver)

    \Input *pMetrics        - source metrics data
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pHostName       - name of host providing metrics report
    \Input uHostPort        - port for metrics query
    \Input iCount           - number of results (=1 if this is a single result, >1 if this is an aggregation)
    \Input bSummary         - TRUE if this is a summary report, else FALSE
    \Input uCurTime         - current time

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 12/17/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipMetricsFormatXmlGameServer(const VoipMetricsT *pMetrics, char *pBuffer, int32_t iBufSize, const char *pHostName, uint32_t uHostPort, int32_t iCount, uint8_t bSummary, time_t uCurTime)
{
    uint32_t uHours, uMinutes, uSeconds;
    int32_t iOffset = 0;
    const char *pClass, *pName;

    // start filling in the message
    if (bSummary)
    {
        pClass = "summary";
        pName = "count";
    }
    else
    {
        pClass = "single";
        pName = "index";
    }

    // metrics header
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<metrics host=\"%s\" port=\"%d\" region=\"%s\" tier=\"%d\" type=\"%s\" class=\"%s\" %s=\"%d\">",
        pHostName, uHostPort, pMetrics->strServerRegion, pMetrics->iServerTier, pMetrics->strServerType, pClass, pName, iCount);

    // output user info
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<clients connected=\"%llu\" talking=\"%llu\" supported=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_ClientsConnected], pMetrics->aMetrics[VOIPMETRIC_ClientsTalking], pMetrics->aMetrics[VOIPMETRIC_ClientsSupported]);

    // output game info
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<games active=\"%llu\" available=\"%llu\" stuck=\"%llu\" offline=\"%llu\" disconnected=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_GamesActive], pMetrics->aMetrics[VOIPMETRIC_GamesAvailable],
        pMetrics->aMetrics[VOIPMETRIC_GamesStuck], pMetrics->aMetrics[VOIPMETRIC_GamesOffline],
        pMetrics->aMetrics[VOIPMETRIC_GamesDisconnected]);

    // get server time in hours, minutes, seconds
    uSeconds = (uint32_t)pMetrics->aMetrics[VOIPMETRIC_ServerCpuTime];
    uHours = uSeconds/(60*60);
    uSeconds -= uHours*60*60;
    uMinutes = uSeconds/60;
    uSeconds -= uMinutes*60;

    // output server load and system load
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<server_load cpu=\"%0.2f\" time=\"%02d:%02d:%02d\"/>",
            (float)pMetrics->aMetrics[VOIPMETRIC_ServerCpuPct] / 100.0f, uHours, uMinutes, uSeconds);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<system_load one=\"%0.2f\" five=\"%0.2f\" fifteen=\"%0.2f\" cores=\"%llu\"/>",
            (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_1] / 100.0f,
            (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_5] / 100.0f,
            (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_15] / 100.0f,
            pMetrics->aMetrics[VOIPMETRIC_ServerCores]);

    // output bandwidth stats (up/down)
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<bandwidth>");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<curr unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_CurrBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<peak unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_PeakBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<total unit=\"mb\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalBandwidthMbUp], pMetrics->aMetrics[VOIPMETRIC_TotalBandwidthMbDn]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</bandwidth>");

    // output game bandwidth stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<gamebandwidth>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrGameBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_CurrGameBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakGameBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_PeakGameBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mb\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalGameBandwidthMbUp], pMetrics->aMetrics[VOIPMETRIC_TotalGameBandwidthMbDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</gamebandwidth>");

    // output voip bandwidth stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<voipbandwidth>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrVoipBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_CurrVoipBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakVoipBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_PeakVoipBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mb\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalVoipBandwidthMbUp], pMetrics->aMetrics[VOIPMETRIC_TotalVoipBandwidthMbDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</voipbandwidth>");

    // output packet stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<packets>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrPacketRateUp], pMetrics->aMetrics[VOIPMETRIC_CurrPacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakPacketRateUp], pMetrics->aMetrics[VOIPMETRIC_PeakPacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mp\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalPacketsUp], pMetrics->aMetrics[VOIPMETRIC_TotalPacketsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</packets>");

    // output game packets stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<gamepackets>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrGamePacketRateUp], pMetrics->aMetrics[VOIPMETRIC_CurrGamePacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakGamePacketRateUp], pMetrics->aMetrics[VOIPMETRIC_PeakGamePacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mp\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalGamePacketsUp], pMetrics->aMetrics[VOIPMETRIC_TotalGamePacketsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</gamepackets>");

    // output voip packet stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<voippackets>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrVoipPacketRateUp], pMetrics->aMetrics[VOIPMETRIC_CurrVoipPacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakVoipPacketRateUp], pMetrics->aMetrics[VOIPMETRIC_PeakVoipPacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mp\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalVoipPacketsUp], pMetrics->aMetrics[VOIPMETRIC_TotalVoipPacketsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</voippackets>");

    // output latency stats
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<latency unit=\"ms\" games=\"%llu\" min=\"%llu\" max=\"%llu\" avg=\"%llu\" bin=\"%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\"/>", 0,
        pMetrics->aMetrics[VOIPMETRIC_LatencyMin], pMetrics->aMetrics[VOIPMETRIC_LatencyMax], pMetrics->aMetrics[VOIPMETRIC_LatencyAvg],
        pMetrics->aMetrics[VOIPMETRIC_LateBin_0], pMetrics->aMetrics[VOIPMETRIC_LateBin_1], pMetrics->aMetrics[VOIPMETRIC_LateBin_2], pMetrics->aMetrics[VOIPMETRIC_LateBin_3],
        pMetrics->aMetrics[VOIPMETRIC_LateBin_4], pMetrics->aMetrics[VOIPMETRIC_LateBin_5], pMetrics->aMetrics[VOIPMETRIC_LateBin_6], pMetrics->aMetrics[VOIPMETRIC_LateBin_7]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<inblate unit=\"ms\" games=\"%llu\" min=\"%llu\" max=\"%llu\" avg=\"%llu\"/>", 0,
        pMetrics->aMetrics[VOIPMETRIC_InbLateMin], pMetrics->aMetrics[VOIPMETRIC_InbLateMax], pMetrics->aMetrics[VOIPMETRIC_InbLateAvg]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<otblate unit=\"ms\" games=\"%llu\" min=\"%llu\" max=\"%llu\" avg=\"%llu\"/>", 0,
        pMetrics->aMetrics[VOIPMETRIC_OtbLateMin], pMetrics->aMetrics[VOIPMETRIC_OtbLateMax], pMetrics->aMetrics[VOIPMETRIC_OtbLateAvg]);

    // output game packet metric
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<tunnelinfo rcal=\"%d\" rtot=\"%llu\" rsub=\"%llu\" dpkt=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_RecvCalls], pMetrics->aMetrics[VOIPMETRIC_TotalRecv],
        pMetrics->aMetrics[VOIPMETRIC_TotalRecvSub], pMetrics->aMetrics[VOIPMETRIC_TotalDiscard]);

    // output gamestats
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<gamestats>");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<clients total=\"%llu\" pregame=\"%llu\" ingame=\"%llu\" postgame=\"%llu\" migrating=\"%llu\" other=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_ClientsAdded], pMetrics->aMetrics[VOIPMETRIC_ClientsRemovedPregame], pMetrics->aMetrics[VOIPMETRIC_ClientsRemovedIngame],
        pMetrics->aMetrics[VOIPMETRIC_ClientsRemovedPostgame], pMetrics->aMetrics[VOIPMETRIC_ClientsRemovedMigrating], pMetrics->aMetrics[VOIPMETRIC_ClientsRemovedOther]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<games total=\"%llu\" pregame=\"%llu\" ingame=\"%llu\" postgame=\"%llu\" migrating=\"%llu\" other=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_GamesCreated], pMetrics->aMetrics[VOIPMETRIC_GamesEndedPregame], pMetrics->aMetrics[VOIPMETRIC_GamesEndedIngame],
        pMetrics->aMetrics[VOIPMETRIC_GamesEndedPostgame], pMetrics->aMetrics[VOIPMETRIC_GamesEndedMigrating], pMetrics->aMetrics[VOIPMETRIC_GamesEndedOther]);

    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</gamestats>");

    // output udp net stats
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<udpstats>");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<udprecvqueue len=\"%llu\"/>", pMetrics->aMetrics[VOIPMETRIC_UdpRecvQueueLen]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<udpsendqueue len=\"%llu\"/>", pMetrics->aMetrics[VOIPMETRIC_UdpSendQueueLen]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</udpstats>");

    // output count of redundancy limit changes over the sampling period
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<rlmt count=\"%llu\" />", pMetrics->aMetrics[VOIPMETRIC_RlmtChangeCount]);

    // done filling in the message
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</metrics>");
    // return updated buffer size to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _VoipMetricsFormatXmlConciergeServer

    \Description
        Format voipserver metrics into xml output (concierge)

    \Input *pMetrics        - source metrics data
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pHostName       - name of host providing metrics report
    \Input uHostPort        - port for metrics query
    \Input iCount           - number of results (=1 if this is a single result, >1 if this is an aggregation)
    \Input bSummary         - TRUE if this is a summary report, else FALSE
    \Input uCurTime         - current time

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 12/17/2015 (eesponda)
*/
/********************************************************************************F*/
static int32_t _VoipMetricsFormatXmlConciergeServer(const VoipMetricsT *pMetrics, char *pBuffer, int32_t iBufSize, const char *pHostName, uint32_t uHostPort, int32_t iCount, uint8_t bSummary, time_t uCurTime)
{
    uint32_t uHours, uMinutes, uSeconds;
    int32_t iOffset = 0;
    const char *pClass, *pName;

    // start filling in the message
    if (bSummary)
    {
        pClass = "summary";
        pName = "count";
    }
    else
    {
        pClass = "single";
        pName = "index";
    }

    // metrics header
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<metrics host=\"%s\" port=\"%d\" region=\"%s\" tier=\"%d\" type=\"%s\" pool=\"%s\" class=\"%s\" %s=\"%d\">",
        pHostName, uHostPort, pMetrics->strServerRegion, pMetrics->iServerTier, pMetrics->strServerType, pMetrics->strPool, pClass, pName, iCount);

    // output user info
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<clients connected=\"%llu\" talking=\"%llu\" supported=\"%llu\" active=\"%llu,%llu,%llu\" vers=\"%04x,%04x,%04x\"/>",
        pMetrics->aMetrics[VOIPMETRIC_ClientsConnected], pMetrics->aMetrics[VOIPMETRIC_ClientsTalking], pMetrics->aMetrics[VOIPMETRIC_ClientsSupported],
        pMetrics->aMetrics[VOIPMETRIC_ClientsActive_0], pMetrics->aMetrics[VOIPMETRIC_ClientsActive_1], pMetrics->aMetrics[VOIPMETRIC_ClientsActive_2],
        pMetrics->aMetrics[VOIPMETRIC_ClientVersion_0], pMetrics->aMetrics[VOIPMETRIC_ClientVersion_1], pMetrics->aMetrics[VOIPMETRIC_ClientVersion_2]);

    // output game info
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<games active=\"%llu\" available=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_GamesActive], pMetrics->aMetrics[VOIPMETRIC_GamesAvailable]);

    // get server time in hours, minutes, seconds
    uSeconds = pMetrics->aMetrics[VOIPMETRIC_ServerCpuTime];
    uHours = uSeconds/(60*60);
    uSeconds -= uHours*60*60;
    uMinutes = uSeconds/60;
    uSeconds -= uMinutes*60;

    // output server load and system load
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<server_load cpu=\"%0.2f\" time=\"%02d:%02d:%02d\"/>",
            (float)pMetrics->aMetrics[VOIPMETRIC_ServerCpuPct] / 100.0f, uHours, uMinutes, uSeconds);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<system_load one=\"%0.2f\" five=\"%0.2f\" fifteen=\"%0.2f\" cores=\"%llu\"/>",
            (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_1] / 100.0f,
            (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_5] / 100.0f,
            (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_15] / 100.0f,
            pMetrics->aMetrics[VOIPMETRIC_ServerCores]);

    // output bandwidth stats (up/down)
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<bandwidth>");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<curr unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_CurrBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<peak unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_PeakBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<total unit=\"mb\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalBandwidthMbUp], pMetrics->aMetrics[VOIPMETRIC_TotalBandwidthMbDn]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</bandwidth>");

    // output game bandwidth stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<gamebandwidth>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrGameBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_CurrGameBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakGameBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_PeakGameBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mb\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalGameBandwidthMbUp], pMetrics->aMetrics[VOIPMETRIC_TotalGameBandwidthMbDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</gamebandwidth>");

    // output voip bandwidth stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<voipbandwidth>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrVoipBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_CurrVoipBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"k/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakVoipBandwidthKpsUp], pMetrics->aMetrics[VOIPMETRIC_PeakVoipBandwidthKpsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mb\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalVoipBandwidthMbUp], pMetrics->aMetrics[VOIPMETRIC_TotalVoipBandwidthMbDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</voipbandwidth>");

    // output packet stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<packets>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrPacketRateUp], pMetrics->aMetrics[VOIPMETRIC_CurrPacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakPacketRateUp], pMetrics->aMetrics[VOIPMETRIC_PeakPacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mp\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalPacketsUp], pMetrics->aMetrics[VOIPMETRIC_TotalPacketsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</packets>");

    // output game packets stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<gamepackets>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrGamePacketRateUp], pMetrics->aMetrics[VOIPMETRIC_CurrGamePacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakGamePacketRateUp], pMetrics->aMetrics[VOIPMETRIC_PeakGamePacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mp\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalGamePacketsUp], pMetrics->aMetrics[VOIPMETRIC_TotalGamePacketsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</gamepackets>");

    // output voip packet stats (up/down)
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<voippackets>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<curr unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_CurrVoipPacketRateUp], pMetrics->aMetrics[VOIPMETRIC_CurrVoipPacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<peak unit=\"p/s\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_PeakVoipPacketRateUp], pMetrics->aMetrics[VOIPMETRIC_PeakVoipPacketRateDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<total unit=\"mp\" up=\"%llu\" down=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_TotalVoipPacketsUp], pMetrics->aMetrics[VOIPMETRIC_TotalVoipPacketsDn]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</voippackets>");

    // output latency stats
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<latency unit=\"ms\" min=\"%llu\" max=\"%llu\" avg=\"%llu\" bin=\"%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_LatencyMin], pMetrics->aMetrics[VOIPMETRIC_LatencyMax], pMetrics->aMetrics[VOIPMETRIC_LatencyAvg],
        pMetrics->aMetrics[VOIPMETRIC_LateBin_0], pMetrics->aMetrics[VOIPMETRIC_LateBin_1], pMetrics->aMetrics[VOIPMETRIC_LateBin_2], pMetrics->aMetrics[VOIPMETRIC_LateBin_3],
        pMetrics->aMetrics[VOIPMETRIC_LateBin_4], pMetrics->aMetrics[VOIPMETRIC_LateBin_5], pMetrics->aMetrics[VOIPMETRIC_LateBin_6], pMetrics->aMetrics[VOIPMETRIC_LateBin_7]);

    // output game packet metric
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<tunnelinfo rcal=\"%llu\" rtot=\"%llu\" rsub=\"%llu\" dpkt=\"%llu\"/>",
        pMetrics->aMetrics[VOIPMETRIC_RecvCalls], pMetrics->aMetrics[VOIPMETRIC_TotalRecv],
        pMetrics->aMetrics[VOIPMETRIC_TotalRecvSub], pMetrics->aMetrics[VOIPMETRIC_TotalDiscard]);

    // output gamestats
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<gamestats>");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<clients total=\"%llu\"/>", pMetrics->aMetrics[VOIPMETRIC_ClientsAdded]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<games total=\"%llu\"/>", pMetrics->aMetrics[VOIPMETRIC_GamesCreated]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</gamestats>");

    // output udp net stats
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<udpstats>");
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<udprecvqueue len=\"%llu\"/>", pMetrics->aMetrics[VOIPMETRIC_UdpRecvQueueLen]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "<udpsendqueue len=\"%llu\"/>", pMetrics->aMetrics[VOIPMETRIC_UdpSendQueueLen]);
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</udpstats>");

    // done filling in the message
    iOffset += ds_snzprintf(pBuffer+iOffset, iBufSize-iOffset, "</metrics>");
    // return updated buffer size to caller
    return(iOffset);
}

/*F********************************************************************************/
/*!
    \Function _VoipMetricsFormatXmlQosServer

    \Description
        Format voipserver metrics into xml output (qos).
    
    \Note this is only a very small subset since this is 
    not the active metrics path.

    \Input *pMetrics        - source metrics data
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pHostName       - name of host providing metrics report
    \Input uHostPort        - port for metrics query
    \Input iCount           - number of results (=1 if this is a single result, >1 if this is an aggregation)
    \Input bSummary         - TRUE if this is a summary report, else FALSE
    \Input uCurTime         - current time

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 04/26/2017 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _VoipMetricsFormatXmlQosServer(const VoipMetricsT *pMetrics, char *pBuffer, int32_t iBufSize, const char *pHostName, uint32_t uHostPort, int32_t iCount, uint8_t bSummary, time_t uCurTime)
{
    uint32_t uHours, uMinutes, uSeconds;
    int32_t iOffset = 0;

    // metrics header
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<metrics host=\"%s\" port=\"%d\" region=\"%s\" type=\"%s\">",
        pHostName, uHostPort, pMetrics->strServerRegion, pMetrics->strServerType);

    // get server time in hours, minutes, seconds
    uSeconds = pMetrics->aMetrics[VOIPMETRIC_ServerCpuTime];
    uHours = uSeconds / (60 * 60);
    uSeconds -= uHours * 60 * 60;
    uMinutes = uSeconds / 60;
    uSeconds -= uMinutes * 60;

    // output server load and system load
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<server_load cpu=\"%0.2f\" time=\"%02d:%02d:%02d\"/>",
        (float)pMetrics->aMetrics[VOIPMETRIC_ServerCpuPct] / 100.0f, uHours, uMinutes, uSeconds);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<system_load one=\"%0.2f\" five=\"%0.2f\" fifteen=\"%0.2f\" cores=\"%llu\"/>",
        (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_1] / 100.0f,
        (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_5] / 100.0f,
        (float)pMetrics->aMetrics[VOIPMETRIC_SystemLoad_15] / 100.0f,
        pMetrics->aMetrics[VOIPMETRIC_ServerCores]);

    // output udp net stats
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<udpstats>");
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<udprecvqueue len=\"%llu\"/>", pMetrics->aMetrics[VOIPMETRIC_UdpRecvQueueLen]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "<udpsendqueue len=\"%llu\"/>", pMetrics->aMetrics[VOIPMETRIC_UdpSendQueueLen]);
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</udpstats>");

    // done filling in the message
    iOffset += ds_snzprintf(pBuffer + iOffset, iBufSize - iOffset, "</metrics>");
    // return updated buffer size to caller
    return(iOffset);
}


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function VoipMetricsFormatXml

    \Description
        Open an XML metrics response

    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 10/13/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipMetricsOpenXml(char *pBuffer, int32_t iBufSize)
{
    return(ds_snzprintf(pBuffer, iBufSize, "<voipmetrics>"));
}

/*F********************************************************************************/
/*!
    \Function VoipMetricsFormatXml

    \Description
        Format voipserver metrics into xml output.

    \Input *pMetrics        - source metrics data
    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into
    \Input *pHostName       - name of host providing metrics report
    \Input uHostPort        - port for metrics query
    \Input iCount           - number of results (=1 if this is a single result, >1 if this is an aggregation)
    \Input bSummary         - TRUE if this is a summary report, else FALSE
    \Input uCurTime         - current time

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 09/01/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipMetricsFormatXml(const VoipMetricsT *pMetrics, char *pBuffer, int32_t iBufSize, const char *pHostName, uint32_t uHostPort, int32_t iCount, uint8_t bSummary, time_t uCurTime)
{
    if (ds_stristr(pMetrics->strServerType, "concierge") != NULL)
    {
        return(_VoipMetricsFormatXmlConciergeServer(pMetrics, pBuffer, iBufSize, pHostName, uHostPort, iCount, bSummary, uCurTime));
    }
    else if (ds_stristr(pMetrics->strServerType, "qos") != NULL)
    {
        return(_VoipMetricsFormatXmlQosServer(pMetrics, pBuffer, iBufSize, pHostName, uHostPort, iCount, bSummary, uCurTime));
    }    
    // default to gameserver
    else
    {
        return(_VoipMetricsFormatXmlGameServer(pMetrics, pBuffer, iBufSize, pHostName, uHostPort, iCount, bSummary, uCurTime));
    }
}

/*F********************************************************************************/
/*!
    \Function VoipMetricsCloseXml

    \Description
        Close an XML metrics response

    \Input *pBuffer         - pointer to buffer to format data into
    \Input iBufSize         - size of buffer to format data into

    \Output
        int32_t             - number of bytes written to output buffer

    \Version 10/13/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipMetricsCloseXml(char *pBuffer, int32_t iBufSize)
{
    return(ds_snzprintf(pBuffer, iBufSize, "</voipmetrics>"));
}

/*F********************************************************************************/
/*!
    \Function VoipMetricsParseXml

    \Description
        Parse server metrics response.

    \Input *pVoipMetrics    - [out] metrics structure to fill in
    \Input *pResponse       - server response to parse

    \Notes
        A sample server response (reformatted with line breaks and indentation for
        the sake of readability):

        \verbatim
            <voipmetrics>
              <metrics host="2JBROOKS3VY6R51" port="9502" region="gva" tier="2" type="voipmetrics" class="summary" count="2">
                <clients connected="301" talking="2" supported="24000"/>
                <games active="53" available="916" stuck="0" offline="0" disconnected="0"/>
                <users in_games="301"/>
                <server_load cpu="3.03" time="11:41:25"/>
                <system_load one="0.23" five="0.27" fifteen="0.32" cores="0"/>
                <bandwidth>
                  <curr unit="k/s" up="0" down="0"/>
                  <peak unit="k/s" up="0" down="0"/>
                  <total unit="mb" up="0" down="0"/>
                </bandwidth>
                <gamebandwidth>
                  <curr unit="k/s" up="0" down="0"/>
                  <peak unit="k/s" up="0" down="0"/>
                  <total unit="mb" up="0" down="0"/>
                </gamebandwidth>
                <voipbandwidth>
                  <curr unit="k/s" up="0" down="0"/>
                  <peak unit="k/s" up="0" down="0"/>
                  <total unit="mb" up="0" down="0"/>
                </voipbandwidth>
                <packets>
                  <curr unit="p/s" up="0" down="0"/>
                  <peak unit="p/s" up="0" down="0"/>
                  <total unit="mp" up="0" down="0"/>
                </packets>
                <gamepackets>
                  <curr unit="p/s" up="0" down="0"/>
                  <peak unit="p/s" up="0" down="0"/>
                  <total unit="mp" up="0" down="0"/>
                </gamepackets>
                <voippackets>
                  <curr unit="p/s" up="0" down="0"/>
                  <peak unit="p/s" up="0" down="0"/>
                  <total unit="mp" up="0" down="0"/>
                </voippackets>
                <latency unit="ms" games="0" min="15" max="647" avg="46" bin="10,5,18,12,0,0,0,0"/>
                <inblate unit="ms" games="0" min="1" max="50" avg="15"/>
                <otblate unit="ms" games="0" min="1" max="31" avg="0"/>
                <tunnelinfo rcal="5" rtot="5" rsub="5" dpkt="0"/>
                <gamestats>
                  <clients total="730185" pregame="330316" ingame="106674" postgame="293173" migrating="0" other="30"/>
                  <games total="138677" pregame="68366" ingame="8219" postgame="62089" migrating="0" other="2"/>
                </gamestats>
                <udpstats>
                  <errors total="0"/> 
                  <pktssent total="0"/> 
                  <pktsrecv total="0"/> 
                  <udprecvqueue len="0"/> 
                  <udpsendqueue len="0"/>
                </udpstats>
                <rlmt count="0"/>
              </metrics>
            <voipmetrics>
        \endverbatim

        Note that VoipMetrics responses may include more than one <metrics> stanza, but this
        function only parses the first stanza.

    \Version 03/19/2010 (jbrookes)
*/
/********************************************************************************F*/
void VoipMetricsParseXml(VoipMetricsT *pVoipMetrics, const char *pResponse)
{
    const char *pXml;

    NetPrintf(("voipmetrics: parsing xml metrics response\n"));

    // first, look for new-style metrics response
    if ((pXml = XmlFind(pResponse, "voipmetrics.metrics")) == NULL)
    {
        // look for old-style metrics response
        pXml = strstr(pResponse, "<metrics>");
    }
    if (pXml == NULL)
    {
        NetPrintf(("voipmetrics: could not find metrics in response\n"));
        return;
    }
    pResponse = pXml;

    // find metrics response
    {
        uint64_t *pMetrics = pVoipMetrics->aMetrics;

        XmlAttribGetString(pResponse, "type", pVoipMetrics->strServerType, sizeof(pVoipMetrics->strServerType), "");
        XmlAttribGetString(pResponse, "region", pVoipMetrics->strServerRegion, sizeof(pVoipMetrics->strServerRegion), "");
        XmlAttribGetString(pResponse, "pool", pVoipMetrics->strPool, sizeof(pVoipMetrics->strPool), "");
        pVoipMetrics->iServerTier = XmlAttribGetInteger(pResponse, "tier", 0);

        if ((pXml = XmlFind(pResponse, ".clients")) != NULL)
        {
            char strDelimited[64], strValue[8];
            uint32_t uCount;

            pMetrics[VOIPMETRIC_ClientsConnected] = XmlAttribGetInteger64(pXml, "connected", 0);
            pMetrics[VOIPMETRIC_ClientsTalking] = XmlAttribGetInteger64(pXml, "talking", 0);
            pMetrics[VOIPMETRIC_ClientsSupported] = XmlAttribGetInteger64(pXml, "supported", 0);

            // get the number of active connections as a delimited list
            if ((XmlAttribGetString(pXml, "active", strDelimited, sizeof(strDelimited), "") != -1) && (strDelimited[0] != '\0'))
            {
                for (uCount = 0; uCount < VOIPMETRIC_MAXVERS; uCount += 1)
                {
                    if (TagFieldGetDelim(strDelimited, strValue, sizeof(strValue), "", uCount, ',') == 0)
                    {
                        break;
                    }
                    pMetrics[VOIPMETRIC_ClientsActive_0+uCount] = ds_strtoll(strValue, NULL, 10);
                }
            }
            // get the versions as a comma delimited list so we can reference the active data
            if ((XmlAttribGetString(pXml, "vers", strDelimited, sizeof(strDelimited), "") != -1) && (strDelimited[0] != '\0'))
            {
                for (uCount = 0; uCount < VOIPMETRIC_MAXVERS; uCount += 1)
                {
                    if (TagFieldGetDelim(strDelimited, strValue, sizeof(strValue), "", uCount, ',') == 0)
                    {
                        break;
                    }
                    pMetrics[VOIPMETRIC_ClientVersion_0+uCount] = ds_strtoll(strValue, NULL, 16);
                }
            }
        }
        if ((pXml = XmlFind(pResponse, ".games")) != NULL)
        {
            pMetrics[VOIPMETRIC_GamesActive] = XmlAttribGetInteger64(pXml, "active", 0);
            pMetrics[VOIPMETRIC_GamesAvailable] = XmlAttribGetInteger64(pXml, "available", 0);
            pMetrics[VOIPMETRIC_GamesStuck] = XmlAttribGetInteger64(pXml, "stuck", 0);
            pMetrics[VOIPMETRIC_GamesOffline] = XmlAttribGetInteger64(pXml, "offline", 0);
            pMetrics[VOIPMETRIC_GamesDisconnected] = XmlAttribGetInteger64(pXml, "disconnected", 0);
        }
        if ((pXml = XmlFind(pResponse, ".server_load")) != NULL)
        {
            pMetrics[VOIPMETRIC_ServerCpuPct] = (uint64_t)_VoipMetricsXmlAttribGetPct(pXml, "cpu", 0);
            pMetrics[VOIPMETRIC_ServerCpuTime] = (uint64_t)_VoipMetricsXmlAttribGetTime(pXml, "time", 0);
        }
        if ((pXml = XmlFind(pResponse, ".system_load")) != NULL)
        {
            pMetrics[VOIPMETRIC_SystemLoad_1] = (uint64_t)_VoipMetricsXmlAttribGetPct(pXml, "one", 0);
            pMetrics[VOIPMETRIC_SystemLoad_5] = (uint64_t)_VoipMetricsXmlAttribGetPct(pXml, "five", 0);
            pMetrics[VOIPMETRIC_SystemLoad_15] = (uint64_t)_VoipMetricsXmlAttribGetPct(pXml, "fifteen", 0);
        }
        if ((pXml = XmlFind(pResponse, ".bandwidth.curr")) != NULL)
        {
            pMetrics[VOIPMETRIC_CurrBandwidthKpsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_CurrBandwidthKpsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".bandwidth.peak")) != NULL)
        {
            pMetrics[VOIPMETRIC_PeakBandwidthKpsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_PeakBandwidthKpsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".bandwidth.total")) != NULL)
        {
            pMetrics[VOIPMETRIC_TotalBandwidthMbUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_TotalBandwidthMbDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".gamebandwidth.curr")) != NULL)
        {
            pMetrics[VOIPMETRIC_CurrGameBandwidthKpsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_CurrGameBandwidthKpsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".gamebandwidth.peak")) != NULL)
        {
            pMetrics[VOIPMETRIC_PeakGameBandwidthKpsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_PeakGameBandwidthKpsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".gamebandwidth.total")) != NULL)
        {
            pMetrics[VOIPMETRIC_TotalGameBandwidthMbUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_TotalGameBandwidthMbDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".voipbandwidth.curr")) != NULL)
        {
            pMetrics[VOIPMETRIC_CurrVoipBandwidthKpsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_CurrVoipBandwidthKpsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".voipbandwidth.peak")) != NULL)
        {
            pMetrics[VOIPMETRIC_PeakVoipBandwidthKpsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_PeakVoipBandwidthKpsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".voipbandwidth.total")) != NULL)
        {
            pMetrics[VOIPMETRIC_TotalVoipBandwidthMbUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_TotalVoipBandwidthMbDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".packets.curr")) != NULL)
        {
            pMetrics[VOIPMETRIC_CurrPacketRateUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_CurrPacketRateDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".packets.peak")) != NULL)
        {
            pMetrics[VOIPMETRIC_PeakPacketRateUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_PeakPacketRateDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".packets.total")) != NULL)
        {
            pMetrics[VOIPMETRIC_TotalPacketsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_TotalPacketsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".gamepackets.curr")) != NULL)
        {
            pMetrics[VOIPMETRIC_CurrGamePacketRateUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_CurrGamePacketRateDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".gamepackets.peak")) != NULL)
        {
            pMetrics[VOIPMETRIC_PeakGamePacketRateUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_PeakGamePacketRateDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".gamepackets.total")) != NULL)
        {
            pMetrics[VOIPMETRIC_TotalGamePacketsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_TotalGamePacketsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".voippackets.curr")) != NULL)
        {
            pMetrics[VOIPMETRIC_CurrVoipPacketRateUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_CurrVoipPacketRateDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".voippackets.peak")) != NULL)
        {
            pMetrics[VOIPMETRIC_PeakVoipPacketRateUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_PeakVoipPacketRateDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".voippackets.total")) != NULL)
        {
            pMetrics[VOIPMETRIC_TotalVoipPacketsUp] = XmlAttribGetInteger64(pXml, "up", 0);
            pMetrics[VOIPMETRIC_TotalVoipPacketsDn] = XmlAttribGetInteger64(pXml, "down", 0);
        }
        if ((pXml = XmlFind(pResponse, ".latency")) != NULL)
        {
            char strLateBin[64];
            pMetrics[VOIPMETRIC_LatencyMin] = XmlAttribGetInteger64(pXml, "min", 0);
            pMetrics[VOIPMETRIC_LatencyMax] = XmlAttribGetInteger64(pXml, "max", 0);
            pMetrics[VOIPMETRIC_LatencyAvg] = XmlAttribGetInteger64(pXml, "avg", 0);
            if ((XmlAttribGetString(pXml, "bin", strLateBin, sizeof(strLateBin), "") != -1) && (strLateBin[0] != '\0'))
            {
                char strBinVal[6];
                uint32_t uCount;
                for (uCount = 0; uCount < 8; uCount += 1)
                {
                    if (TagFieldGetDelim(strLateBin, strBinVal, sizeof(strBinVal), "", uCount, ',') == 0)
                    {
                        break;
                    }
                    pMetrics[VOIPMETRIC_LateBin_0+uCount] = ds_strtoll(strBinVal, NULL, 10);
                }
            }
        }
        if ((pXml = XmlFind(pResponse, ".inblate")) != NULL)
        {
            pMetrics[VOIPMETRIC_InbLateMin] = XmlAttribGetInteger64(pXml, "min", 0);
            pMetrics[VOIPMETRIC_InbLateMax] = XmlAttribGetInteger64(pXml, "max", 0);
            pMetrics[VOIPMETRIC_InbLateAvg] = XmlAttribGetInteger64(pXml, "avg", 0);
        }
        if ((pXml = XmlFind(pResponse, ".otblate")) != NULL)
        {
            pMetrics[VOIPMETRIC_OtbLateMin] = XmlAttribGetInteger64(pXml, "min", 0);
            pMetrics[VOIPMETRIC_OtbLateMax] = XmlAttribGetInteger64(pXml, "max", 0);
            pMetrics[VOIPMETRIC_OtbLateAvg] = XmlAttribGetInteger64(pXml, "avg", 0);
        }
        if ((pXml = XmlFind(pResponse, ".tunnelinfo")) != NULL)
        {
            pMetrics[VOIPMETRIC_RecvCalls] = XmlAttribGetInteger64(pXml, "rcal", 0);
            pMetrics[VOIPMETRIC_TotalRecv] = XmlAttribGetInteger64(pXml, "rtot", 0);
            pMetrics[VOIPMETRIC_TotalRecvSub] = XmlAttribGetInteger64(pXml, "rsub", 0);
            pMetrics[VOIPMETRIC_TotalDiscard] = XmlAttribGetInteger64(pXml, "dpkt", 0);
        }
        if ((pXml = XmlFind(pResponse, ".gamestats.clients")) != NULL)
        {
            pMetrics[VOIPMETRIC_ClientsAdded] = XmlAttribGetInteger64(pXml, "total", 0);
            pMetrics[VOIPMETRIC_ClientsRemovedPregame] = XmlAttribGetInteger64(pXml, "pregame", 0);
            pMetrics[VOIPMETRIC_ClientsRemovedIngame] = XmlAttribGetInteger64(pXml, "ingame", 0);
            pMetrics[VOIPMETRIC_ClientsRemovedPostgame] = XmlAttribGetInteger64(pXml, "postgame", 0);
            pMetrics[VOIPMETRIC_ClientsRemovedMigrating] = XmlAttribGetInteger64(pXml, "migrating", 0);
            pMetrics[VOIPMETRIC_ClientsRemovedOther] = XmlAttribGetInteger64(pXml, "other", 0);
        }
        if ((pXml = XmlFind(pResponse, ".gamestats.games")) != NULL)
        {
            pMetrics[VOIPMETRIC_GamesCreated] = XmlAttribGetInteger64(pXml, "total", 0);
            pMetrics[VOIPMETRIC_GamesEndedPregame] = XmlAttribGetInteger64(pXml, "pregame", 0);
            pMetrics[VOIPMETRIC_GamesEndedIngame] = XmlAttribGetInteger64(pXml, "ingame", 0);
            pMetrics[VOIPMETRIC_GamesEndedPostgame] = XmlAttribGetInteger64(pXml, "postgame", 0);
            pMetrics[VOIPMETRIC_GamesEndedMigrating] = XmlAttribGetInteger64(pXml, "migrating", 0);
            pMetrics[VOIPMETRIC_GamesEndedOther] = XmlAttribGetInteger64(pXml, "other", 0);
        }
        if ((pXml = XmlFind(pResponse, ".udpstats.udprecvqueue")) != NULL)
        {
            pMetrics[VOIPMETRIC_UdpRecvQueueLen] = XmlAttribGetInteger64(pXml, "len", 0);
        }
        if ((pXml = XmlFind(pResponse, ".udpstats.udpsendqueue")) != NULL)
        {
            pMetrics[VOIPMETRIC_UdpSendQueueLen] = XmlAttribGetInteger64(pXml, "len", 0);
        }
        if ((pXml = XmlFind(pResponse, ".rlmt")) != NULL)
        {
            pMetrics[VOIPMETRIC_RlmtChangeCount] = XmlAttribGetInteger64(pXml, "count", 0);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function VoipMetricsAggregate

    \Description
        Accumulate a set of metrics into a single aggregate metrics record

    \Input *pOutMetrics     - [out] storage for aggregate metrics record
    \Input *pInpMetrics     - pointer to array of servers with metrics to aggregate
    \Input uNumServers      - number of servers in metrics array

    \Notes
        Metrics are classified into three types: SUM, AVG, and LATE.  Summed stats are
        simply accumulated.  Averaged stats are accumulated and then averaged.  LATE
        stats are accumulated and averaged, with the caveat that a latency stat of zero
        from a metrics report with no received packets is not considered.  This is to
        prevent zero latency data that is in fact missing data, not a zero sample, from
        biasing the results. VERS stats are accumulated by unioning the data in all the
        versions, this is done separately from other stats to simply the logic.

    \Version 08/12/2010 (jbrookes)
*/
/********************************************************************************F*/
void VoipMetricsAggregate(VoipMetricsT *pOutMetrics, const VoipMetricsT *pInpMetrics, uint32_t uNumServers)
{
    uint32_t uMetric, uMetricIdx, uNumMetrics, uServer;
    uint64_t *pMetrics = pOutMetrics->aMetrics;

    // accumulate
    for (uMetricIdx = 0; uMetricIdx < VOIPMETRIC_NUMMETRICS; uMetricIdx += 1)
    {
        // skip the version metrics
        if (uMetricIdx >= VOIPMETRIC_VERSTAT)
        {
            continue;
        }

        // zero metric output buffer
        pMetrics[uMetricIdx] = 0;

        // accumulate metric
        for (uNumMetrics = 0, uServer = 0; uServer < uNumServers; uServer += 1)
        {
            uMetric = pInpMetrics[uServer].aMetrics[uMetricIdx];

            if ((uMetricIdx < VOIPMETRIC_LATESTATS) || (uMetric != 0) || (pInpMetrics[uServer].aMetrics[VOIPMETRIC_TotalRecv] != 0))
            {
                pMetrics[uMetricIdx] += uMetric;
                uNumMetrics += 1;
            }
        }
        // average metric
        if (((uMetricIdx >= VOIPMETRIC_AVGSTATS) && (uMetricIdx < VOIPMETRIC_MAXSTAT)) && (uNumMetrics > 0))
        {
            pMetrics[uMetricIdx] /= uNumMetrics;
        }

        // max metric
        /* $$ todo  (mclouatre feb 2019) 
           There are currently only two MAXSTAT metrics: VOIPMETRIC_UdpRecvQueueLen & VOIPMETRIC_UdpSendQueueLen.
           By definition, their aggregation is supposed to result in the 'max' value being reported.
           The current implementation however results in something different. It turns out that both metrics 
           end up being processed in the above FOR loop under the "// accumulate metric" and 
           above the "// average metric" comments. They get processed because (uMetric != 0) evaluates to TRUE.
           Consequently, combined with the below code, the aggregation results for these two metrics is a 
           SUM and not a MAX.
           Interestingly, it turns out that a SUM is however more appropriate than a max for those two metrics
           because they are both specific to a UDP port. Consequently, when aggregating, it makes sense to 
           add all of them to get a "global" queue length. 
           Long story short: When we consider fixing this, instead of improving the implementation to avoid
           the issue described above, it's probably better to just move these two metrics under 
           VOIPMETRIC_SUMSTATS and just eliminate the below support for VOIPMETRIC_MAXSTAT as there will no
           longer be any metric of that type. */
        if (((uMetricIdx >= VOIPMETRIC_MAXSTAT) && (uMetricIdx < VOIPMETRIC_VERSTAT)) && (uNumMetrics > 0))
        {
            for (uServer = 0; uServer < uNumServers; uServer += 1)
            {
                uMetric = pInpMetrics[uServer].aMetrics[uMetricIdx];
                pMetrics[uMetricIdx] = DS_MAX(pMetrics[uMetricIdx], uMetric);
            }
        }
    }

    // clear the metrics in prep for updating
    ds_memclr(&pMetrics[VOIPMETRIC_ClientVersion_0], sizeof(*pMetrics) * VOIPMETRIC_MAXVERS);
    ds_memclr(&pMetrics[VOIPMETRIC_ClientsActive_0], sizeof(*pMetrics) * VOIPMETRIC_MAXVERS);

    /*  to allow the support the gathering of version specific connection metrics we have a put
        a restriction on how many versions we report metrics for (VOIPMETRIC_MAXVERS)

        since each voipserver can support any different values in these version sets we have to union
        the data between all the voipservers when accumulating these metrics.
        the version data at the end of the set will be removed first (lowest versions) and we will
        not increase the size of our output data set */

    for (uServer = 0; uServer < uNumServers; uServer += 1)
    {
        uint32_t uInputOffset = 0;
        uint32_t uOutputOffset = 0;

        while (uInputOffset < VOIPMETRIC_MAXVERS && uOutputOffset < VOIPMETRIC_MAXVERS)
        {
            // utility variables so it makes the code easier to understand
            // the input we just take a copy of the values since that is all we need
            // the output we need the indices because we need to write to these locations
            const uint64_t uInputVersion = pInpMetrics[uServer].aMetrics[VOIPMETRIC_ClientVersion_0+uInputOffset];
            const uint64_t uInputActive  = pInpMetrics[uServer].aMetrics[VOIPMETRIC_ClientsActive_0+uInputOffset];
            const uint32_t uOutputVersionIdx = VOIPMETRIC_ClientVersion_0 + uOutputOffset;
            const uint32_t uOutputActiveIdx = VOIPMETRIC_ClientsActive_0 + uOutputOffset;

            // if the value of the input is greater we want to append this to the beginning of
            // the data set with values at the end of the output falling off
            if (uInputVersion > pMetrics[uOutputVersionIdx])
            {
                memmove(&pMetrics[uOutputVersionIdx+1], &pMetrics[uOutputVersionIdx], sizeof(*pMetrics) * (VOIPMETRIC_MAXVERS - (uOutputOffset + 1)));
                memmove(&pMetrics[uOutputActiveIdx+1], &pMetrics[uOutputActiveIdx], sizeof(*pMetrics) * (VOIPMETRIC_MAXVERS - (uOutputOffset + 1)));

                pMetrics[uOutputVersionIdx] = uInputVersion;
                pMetrics[uOutputActiveIdx] = uInputActive;

                uInputOffset += 1;
                uOutputOffset += 1;
            }
            // if the value of the output is greater than just leave in place and move to the next element
            else if (pMetrics[uOutputVersionIdx] > uInputVersion)
            {
                uOutputOffset += 1;
            }
            // otherwise we found versions that match so accumulate here
            else
            {
                pMetrics[uOutputActiveIdx] += uInputActive;

                uInputOffset += 1;
                uOutputOffset += 1;
            }
        }
    }
}
