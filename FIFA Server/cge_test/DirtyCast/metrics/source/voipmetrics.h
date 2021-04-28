/*H********************************************************************************/
/*!
    \File voipmetrics.h

    \Description
        VoipServer metrics definitions and support functions

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 08/26/2010 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voipmetrics_h
#define _voipmetrics_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

//! maximum versions we support for metrics
#define VOIPMETRIC_MAXVERS (3)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! metrics tracked
typedef enum VoipMetricsE
{
    VOIPMETRIC_SUMSTATS,
    VOIPMETRIC_ClientsConnected = VOIPMETRIC_SUMSTATS,
    VOIPMETRIC_ClientsTalking,
    VOIPMETRIC_ClientsSupported,
    VOIPMETRIC_GamesActive,
    VOIPMETRIC_GamesAvailable,
    VOIPMETRIC_GamesStuck,
    VOIPMETRIC_GamesOffline,
    VOIPMETRIC_GamesDisconnected,
    VOIPMETRIC_CurrBandwidthKpsUp,
    VOIPMETRIC_CurrBandwidthKpsDn,
    VOIPMETRIC_PeakBandwidthKpsUp,
    VOIPMETRIC_PeakBandwidthKpsDn,
    VOIPMETRIC_TotalBandwidthMbUp,
    VOIPMETRIC_TotalBandwidthMbDn,
    VOIPMETRIC_CurrGameBandwidthKpsUp,
    VOIPMETRIC_CurrGameBandwidthKpsDn,
    VOIPMETRIC_PeakGameBandwidthKpsUp,
    VOIPMETRIC_PeakGameBandwidthKpsDn,
    VOIPMETRIC_TotalGameBandwidthMbUp,
    VOIPMETRIC_TotalGameBandwidthMbDn,
    VOIPMETRIC_CurrVoipBandwidthKpsUp,
    VOIPMETRIC_CurrVoipBandwidthKpsDn,
    VOIPMETRIC_PeakVoipBandwidthKpsUp,
    VOIPMETRIC_PeakVoipBandwidthKpsDn,
    VOIPMETRIC_TotalVoipBandwidthMbUp,
    VOIPMETRIC_TotalVoipBandwidthMbDn,
    VOIPMETRIC_CurrPacketRateUp,
    VOIPMETRIC_CurrPacketRateDn,
    VOIPMETRIC_PeakPacketRateUp,
    VOIPMETRIC_PeakPacketRateDn,
    VOIPMETRIC_TotalPacketsUp,
    VOIPMETRIC_TotalPacketsDn,
    VOIPMETRIC_CurrGamePacketRateUp,
    VOIPMETRIC_CurrGamePacketRateDn,
    VOIPMETRIC_PeakGamePacketRateUp,
    VOIPMETRIC_PeakGamePacketRateDn,
    VOIPMETRIC_TotalGamePacketsUp,
    VOIPMETRIC_TotalGamePacketsDn,
    VOIPMETRIC_CurrVoipPacketRateUp,
    VOIPMETRIC_CurrVoipPacketRateDn,
    VOIPMETRIC_PeakVoipPacketRateUp,
    VOIPMETRIC_PeakVoipPacketRateDn,
    VOIPMETRIC_TotalVoipPacketsUp,
    VOIPMETRIC_TotalVoipPacketsDn,
    VOIPMETRIC_RecvCalls,
    VOIPMETRIC_TotalRecv,
    VOIPMETRIC_TotalRecvSub,
    VOIPMETRIC_TotalDiscard,
    VOIPMETRIC_ClientsAdded,
    VOIPMETRIC_ClientsRemovedPregame,
    VOIPMETRIC_ClientsRemovedIngame,
    VOIPMETRIC_ClientsRemovedPostgame,
    VOIPMETRIC_ClientsRemovedMigrating,
    VOIPMETRIC_ClientsRemovedOther,
    VOIPMETRIC_GamesCreated,
    VOIPMETRIC_GamesEndedPregame,
    VOIPMETRIC_GamesEndedIngame,
    VOIPMETRIC_GamesEndedPostgame,
    VOIPMETRIC_GamesEndedMigrating,
    VOIPMETRIC_GamesEndedOther,
    VOIPMETRIC_RlmtChangeCount,

    VOIPMETRIC_AVGSTATS,
    VOIPMETRIC_ServerCpuPct = VOIPMETRIC_AVGSTATS,
    VOIPMETRIC_ServerCpuTime,
    VOIPMETRIC_ServerCores,
    VOIPMETRIC_SystemLoad_1,
    VOIPMETRIC_SystemLoad_5,
    VOIPMETRIC_SystemLoad_15,
    VOIPMETRIC_LateBin_0,
    VOIPMETRIC_LateBin_1,
    VOIPMETRIC_LateBin_2,
    VOIPMETRIC_LateBin_3,
    VOIPMETRIC_LateBin_4,
    VOIPMETRIC_LateBin_5,
    VOIPMETRIC_LateBin_6,
    VOIPMETRIC_LateBin_7,

    VOIPMETRIC_LATESTATS,
    VOIPMETRIC_LatencyMin = VOIPMETRIC_LATESTATS,
    VOIPMETRIC_LatencyMax,
    VOIPMETRIC_LatencyAvg,
    VOIPMETRIC_InbLateMin,
    VOIPMETRIC_InbLateMax,
    VOIPMETRIC_InbLateAvg,
    VOIPMETRIC_OtbLateMin,
    VOIPMETRIC_OtbLateMax,
    VOIPMETRIC_OtbLateAvg,

    VOIPMETRIC_MAXSTAT,
    VOIPMETRIC_UdpRecvQueueLen = VOIPMETRIC_MAXSTAT,
    VOIPMETRIC_UdpSendQueueLen,

    VOIPMETRIC_VERSTAT,
    VOIPMETRIC_ClientsActive_0 = VOIPMETRIC_VERSTAT,
    VOIPMETRIC_ClientsActive_1,
    VOIPMETRIC_ClientsActive_2,
    VOIPMETRIC_ClientVersion_0,
    VOIPMETRIC_ClientVersion_1,
    VOIPMETRIC_ClientVersion_2,

    VOIPMETRIC_NUMMETRICS
} VoipMetricsE;


//! information that is surfaced in metrics reports
typedef struct VoipMetricsT
{
    char strServerType[32];         //!< type of server queried ("voipserver" or "voipmetrics")
    char strServerRegion[16];        //!< type of region queried ("sjc", "iad", "gva", ...)
    char strPool[64];               //!< pool the VM is reporting for (empty for gameservers)
    int32_t iServerTier;            //!< tier of metrics report (0=VS, 1=VM/Local, 2=VM/Regional, 3=VM/Title)
    uint64_t aMetrics[VOIPMETRIC_NUMMETRICS];
} VoipMetricsT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// open an xml response
int32_t VoipMetricsOpenXml(char *pBuffer, int32_t iBufSize);

// format a metrics stanza into xml output
int32_t VoipMetricsFormatXml(const VoipMetricsT *pMetrics, char *pBuffer, int32_t iBufSize, const char *pHostName, uint32_t uHostPort, int32_t iCount, uint8_t bSummary, time_t uCurTime);

// close an xml response
int32_t VoipMetricsCloseXml(char *pBuffer, int32_t iBufSize);

// parse xml response into metrics data
void VoipMetricsParseXml(VoipMetricsT *pVoipMetrics, const char *pResponse);

// aggregate multiple metrics records into a summary metrics record
void VoipMetricsAggregate(VoipMetricsT *pOutMetrics, const VoipMetricsT *pInpMetrics, uint32_t uNumServers);

#ifdef __cplusplus
}
#endif

#endif // _voipmetrics_h
