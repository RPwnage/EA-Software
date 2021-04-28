/*H********************************************************************************/
/*!
    \File gameservermetrics.h

    \Description
        VoipServer (gameserver mode - not concierge mode) metrics report formatting.

    \Copyright
        Copyright (c) 2016 Electronic Arts Inc.

    \Version 12/02/2016 (mclouatre)
*/
/********************************************************************************H*/

#ifndef _gameservermetrics_h
#define _gameservermetrics_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! forward declaration
typedef struct ServerMetricsT ServerMetricsT;


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// server metrics report formatting for voipserver in gameserver mode
int32_t GameServerMetricsCallback(ServerMetricsT *pServerMetrics, char *pBuffer, int32_t *pSize, int32_t iMetricIndex, void *pUserData);

#endif // _gameservermetrics_h


