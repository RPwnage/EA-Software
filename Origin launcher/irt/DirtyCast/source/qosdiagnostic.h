/*H********************************************************************************/
/*!
    \File qosdiagnostic.h

    \Description
        VoipServer (QOS Mode) HTTP diagnostic page that displays server status summary.

    \Copyright
        Copyright (c) 2017 Electronic Arts Inc.

    \Version 06/12/2017 (cvienneau)
*/
/********************************************************************************H*/

#ifndef _qosdiagnostic_h
#define _qosdiagnostic_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "serverdiagnostic.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// diagnostic callback processing for voip server
int32_t QosServerDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData);

// diagnostic get type callback
ServerDiagnosticResponseTypeE QosServerDiagnosticGetResponseTypeCb(ServerDiagnosticT *pServerDiagnostic, const char *pUrl);

#endif // _qosdiagnostic_h


