/*H********************************************************************************/
/*!
    \File commondiagnostic.h

    \Description
        The module interface for common diagnostic page formatting for the VoipServer

    \Copyright
        Copyright (c) 2018 Electronic Arts Inc.

    \Version 02/26/2018 (eesponda)
*/
/********************************************************************************H*/

#ifndef _commondiagnostic_h
#define _commondiagnostic_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Type Definitions *************************************************************/

// forward declarations
typedef struct VoipServerRefT VoipServerRefT;
typedef struct VoipMetricsT VoipMetricsT;
typedef struct VoipServerDiagnosticMonitorT VoipServerDiagnosticMonitorT;

/*** Functions ********************************************************************/

// populate the servulator status for the application
int32_t CommonDiagnosticServulator(VoipServerRefT *pVoipServer, const char *pMode, uint32_t uPort, int32_t iPlayers, char *pBuffer, int32_t iBufSize);

// common diagnostic actions we support
int32_t CommonDiagnosticAction(VoipServerRefT *pVoipServer, const char *pStrCommand, char *pBuffer, int32_t iBufSize);

// populate common game stats
int32_t CommonDiagnosticGameStats(VoipServerRefT *pVoipServer, uint8_t bReset, char *pBuffer, int32_t iBufSize);

// populate common system stats
int32_t CommonDiagnosticSystem(VoipServerRefT *pVoipServer, uint32_t uPort, char *pBuffer, int32_t iBufSize);

// handle the monitor page
int32_t CommonDiagnosticMonitor(VoipServerRefT *pVoipServer, const VoipServerDiagnosticMonitorT *pMonitor, int32_t iNumDiagnostics, char *pBuffer, int32_t iBufSize);

// populate the common status
int32_t CommonDiagnosticStatus(VoipServerRefT *pVoipServer, char *pBuffer, int32_t iBufSize);

#endif // _commondiagnostic_h
