/*H********************************************************************************/
/*!
    \File conciergemetrics.h

    \Description
        VoipServer (Concierge Mode) metrics report formatting.

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************H*/

#ifndef _conciergemetrics_h
#define _conciergemetrics_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"


/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! forward declaration
typedef struct ServerMetricsT ServerMetricsT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// metrics callback processing for voipserver
int32_t ConciergeMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData);


#endif // _conciergemetrics_h


