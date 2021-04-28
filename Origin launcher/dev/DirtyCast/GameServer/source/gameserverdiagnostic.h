/*H********************************************************************************/
/*!
    \File gameserverdiagnostic.h

    \Description
        GameServer HTTP diagnostic page that displays server status summary.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/27/2010 (jbrookes) Split from gameserver.cpp
*/
/********************************************************************************H*/

#ifndef _gameserverdiagnostic_h
#define _gameserverdiagnostic_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! server diagnostic module forward declaration
typedef struct ServerDiagnosticT ServerDiagnosticT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// diagnostic callback processing for voipserver
int32_t GameServerDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData);

#endif // _gameserverdiagnostic_h

