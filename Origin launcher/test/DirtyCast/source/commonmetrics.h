/*H********************************************************************************/
/*!
    \File comonetrics.h

    \Description
        Contains metric formatting handlers for metrics that are common to all
        voipserver operating modes.

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************H*/

#ifndef _commonmetrics_h
#define _commonmetrics_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! forward declaration
typedef struct VoipServerRefT VoipServerRefT;
typedef struct ServerMetricsT ServerMetricsT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// metrics formatting callback
int32_t CommonTrafficMetricsCallback(VoipServerRefT *pVoipServer, ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, const char *pCommonDimensions);
int32_t CommonSystemMetricsCallback(VoipServerRefT *pVoipServer, ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, const char *pCommonDimensions);

// returns metrics count
int32_t CommonTrafficMetricsGetCount(void);
int32_t CommonSystemMetricsGetCount(void);

#endif // _commonmetrics_h


