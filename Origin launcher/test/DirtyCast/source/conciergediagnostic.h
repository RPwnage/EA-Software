/*H********************************************************************************/
/*!
    \File conciergediagnostic.h

    \Description
        VoipServer (Concierge Mode) HTTP diagnostic page that displays server status summary.

    \Copyright
        Copyright (c) 2015 Electronic Arts Inc.

    \Version 12/02/2015 (eesponda)
*/
/********************************************************************************H*/

#ifndef _conciergediagnostic_h
#define _conciergediagnostic_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "serverdiagnostic.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// diagnostic callback processing for voipserver
int32_t VoipConciergeDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData);

// diagnostic get type callback
ServerDiagnosticResponseTypeE VoipConciergeDiagnosticGetResponseTypeCb(ServerDiagnosticT *pServerDiagnostic, const char *pUrl);

#endif // _conciergediagnostic_h


