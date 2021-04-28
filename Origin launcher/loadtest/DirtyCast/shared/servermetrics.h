/*H********************************************************************************/
/*!
    \File servermetrics.h

    \Description
        Module reporting VS metrics to external metrics aggregator.

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/02/2016 (mclouatre) First Version
*/
/********************************************************************************H*/

#ifndef _servermetrics_h
#define _servermetrics_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

//! formatting error codes
#define SERVERMETRICS_ERR_INSUFFICIENT_SPACE        (-1)
#define SERVERMETRICS_ERR_UNSUPPORTED_FORMAT        (-2)
#define SERVERMETRICS_ERR_UNSUPPORTED_METRICTYPE    (-3)
#define SERVERMETRICS_ERR_UNSUPPORTED_SAMPLERATE    (-4)
#define SERVERMETRICS_ERR_INCORRECT_DATA            (-5)
#define SERVERMETRICS_ERR_OUT_OF_MEMORY             (-6)
#define SERVERMETRICS_ERR_ENTRY_TOO_LARGE           (-7)
#define SERVERMETRICS_ERR_ACCUMULATION_OVERFLOW     (-8)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state
typedef struct ServerMetricsT ServerMetricsT;

//! metrics datagram preparation callback
typedef int32_t (ServerMetricsCallbackT)(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData);

//! different datagram formats the formatter helper supports
typedef enum ServerMetricsTypeE
{
    SERVERMETRICS_TYPE_GAUGE = 0,
    SERVERMETRICS_TYPE_COUNTER,
    SERVERMETRICS_TYPE_HISTOGRAM,
    SERVERMETRICS_TYPE_SET,
    SERVERMETRICS_TYPE_TIMER,
    SERVERMETRICS_TYPE_MAX
} ServerMetricsTypeE;

//! different datagram formats the formatter helper supports
typedef enum ServerMetricsFormatE
{
    SERVERMETRICS_FORMAT_DATADOG = 0,
    SERVERMETRICS_FORMAT_MAX
} ServerMetricsFormatE;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
ServerMetricsT *ServerMetricsCreate(const char *pAggregatorHost, uint32_t uMetricsAggregatorPort, uint32_t uReportRate, ServerMetricsFormatE eFormat, ServerMetricsCallbackT *pCallback, void *pUserData);

// destroy the module
void ServerMetricsDestroy(ServerMetricsT *pServerMetrics);

// update the module
void ServerMetricsUpdate(ServerMetricsT *pServerMetrics);

// control function
int32_t ServerMetricsControl(ServerMetricsT *pServerMetrics, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// helper function centralizing tech-specific metrics datagram formatting
int32_t ServerMetricsFormat(char *pBuffer, int32_t iBufSize, const char *pMetricName, ServerMetricsFormatE eFormat, uint64_t uMetricValue, ServerMetricsTypeE eMetricType, uint32_t uSampleRate, const char *pDimensionsStr);

// helper function centralizing tech-specific metrics datagram formatting - supports multiple values on a single metric line
int32_t ServerMetricsFormatEx(char *pBuffer, int32_t iBufSize, const char *pMetricName, ServerMetricsFormatE eFormat, uint64_t *pMetricValues, uint32_t uMetricValueCount, ServerMetricsTypeE eMetricType, uint32_t uSampleRate, const char *pDimensionsStr);

// helper function centralizing tech-specific metrics datagram formatting
int32_t ServerMetricsFormatFloat(char *pBuffer, int32_t iBufSize, const char *pMetricName, ServerMetricsFormatE eFormat, float fMetricValue, ServerMetricsTypeE eMetricType, uint32_t uSampleRate, const char *pDimensionsStr);

// getter for scratchpad
char* ServerMetricsGetScratchPad(ServerMetricsT *pServerMetrics, int32_t iMinSize);

#ifdef __cplusplus
};
#endif

#endif // _servermetrics_h

