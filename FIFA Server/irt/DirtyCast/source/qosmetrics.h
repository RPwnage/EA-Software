/*H********************************************************************************/
/*!
    \File qosmetrics.h

    \Description
        VoipServer (Qos Mode) metrics report formatting.

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 05/02/2017 (cvienneau)
*/
/********************************************************************************H*/

#ifndef _qosmetrics_h
#define _qosmetrics_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"


/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! forward declaration
typedef struct ServerMetricsT ServerMetricsT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// metrics callback processing for QOS server
int32_t QosMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData);


#endif // _qosmetrics_h


