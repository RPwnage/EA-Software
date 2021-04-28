/*H********************************************************************************/
/*!
    \File serverdiagnostic.h

    \Description
        HTTP diagnostic page that displays server status summary.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 03/13/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _serverdiagnostic_h
#define _serverdiagnostic_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state
typedef struct ServerDiagnosticT ServerDiagnosticT;

//! responsetype enum
typedef enum ServerDiagnosticResponseTypeE
{
    SERVERDIAGNOSTIC_RESPONSETYPE_DIAGNOSTIC,
    SERVERDIAGNOSTIC_RESPONSETYPE_XML
} ServerDiagnosticResponseTypeE;

//! get response type callback
typedef ServerDiagnosticResponseTypeE (ServerDiagnosticGetResponseTypeCallbackT)(ServerDiagnosticT *pServerDiagnostic, const char *pUrl);

//! status page info format callback
typedef int32_t (ServerDiagnosticCallbackT)(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
ServerDiagnosticT *ServerDiagnosticCreate(uint32_t uStatusPort, ServerDiagnosticCallbackT *pCallback, void *pUserData, const char *pServerName, int32_t iBufSize, const char *pCommandTags, const char *pConfigTags, const char *pPrefix);

// destroy the module
void ServerDiagnosticDestroy(ServerDiagnosticT *pServerDiagnostic);

// set serverdiagnostic callbacks
void ServerDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, ServerDiagnosticCallbackT *pCallback, ServerDiagnosticGetResponseTypeCallbackT *pGetResponseTypeCb);

// update the module
void ServerDiagnosticUpdate(ServerDiagnosticT *pServerDiagnostic);

// status func
int32_t ServerDiagnosticStatus(ServerDiagnosticT *pServerDiagnostic, int32_t iStatus, int32_t iValue, void *pBuffer, int32_t iBufLen);

// control function
int32_t ServerDiagnosticControl(ServerDiagnosticT *pServerDiagnostic, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// helper function to format uptime string
int32_t ServerDiagnosticFormatUptime(char *pBuffer, int32_t iBufSize, time_t uCurTime, time_t uStartTime);

// helper function to format uptime string for blaze
int32_t ServerDiagnosticFormatUptimeBlaze(char *pBuffer, int32_t iBufSize, time_t uCurTime, time_t uStartTime);

#ifdef __cplusplus
};
#endif

#endif // _serverdiagnostic_h

