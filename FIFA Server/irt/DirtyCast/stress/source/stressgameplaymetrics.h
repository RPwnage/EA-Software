/*H********************************************************************************/
/*!
    \File stressgameplaymetrics.h

    \Description
        Game bot metrics reporting system.

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 07/20/2019 (cvienneau)
*/
/********************************************************************************H*/

#ifndef _stressgameplaymetrics_h
#define _stressgameplaymetrics_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/


typedef struct StressGameplayGaugeMetricDataT
{
    uint32_t uLatencyMs;
    uint32_t uLatencyPackets;
    uint32_t uSentBps;      // mirrors uSentBytes counter
    uint32_t uReceivedBps;  // mirrors uReceivedBytes counter
    uint32_t uSentPps;      // mirrors uSentPackets counter
    uint32_t uReceivedPps;  // mirrors uReceivedPackets counter
    uint32_t uSendQueueLength;
    uint32_t uReceiveQueueLength;
    uint32_t uProcessingTimeMaxMs;
    uint32_t uProcessingTimeMinMs;
    uint32_t uProcessingTimeAvgMs;
    uint32_t uSentInputs;
    uint32_t uReceivedInputs;
    uint32_t uInputsSentQueued;
    uint32_t uInputsReceivedQueued;
} StressGameplayGaugeMetricDataT;

typedef struct StressGameplayCountMetricDataT
{
    uint32_t uSentBytes;
    uint32_t uReceivedBytes;
    uint32_t uSentPackets;
    uint32_t uReceivedPackets;
    uint32_t uSentNak;
    uint32_t uReceivedNak;
    uint32_t uReceivedPacketsLost;
    uint32_t uReceivedPacketsSaved;
    uint32_t uSentPacketsLost;
    uint32_t uDroppedInputs;
} StressGameplayCountMetricDataT;

//! gather metrics about performance behaviour
typedef struct StressGameplayMetricDataT
{
    StressGameplayGaugeMetricDataT Gauge;
    StressGameplayCountMetricDataT Count;
} StressGameplayMetricDataT;

//! forward declaration
typedef struct ServerMetricsT ServerMetricsT;
typedef struct StressGameplayMetricsRefT StressGameplayMetricsRefT;
typedef struct StressConfigC StressConfigC;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

// create this helper module to manage metrics
StressGameplayMetricsRefT* StressGameplayMetricsCreate(StressConfigC *mpConfig);

// take the latest data and apply them to internal state, to be shared with metrics agent at next push
void StressGameplayMetricData(StressGameplayMetricsRefT *pMetrics, StressGameplayMetricDataT *pResults);

// give time to the metrics system to send its data 
void StressGameplayMetricsUpdate(StressGameplayMetricsRefT *pMetrics);

//destroy this module
void StressGameplayMetricsDestroy(StressGameplayMetricsRefT *pMetrics);

#if defined(__cplusplus)
}
#endif

#endif // _stressgameplaymetrics_h


