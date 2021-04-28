/*H********************************************************************************/
/*!
    \File gameserverdiagnostic.h

    \Description
        VoipServer HTTP diagnostic page that displays server status summary for
        GameServer mode

    \Copyright
        Copyright (c) 2007-2017 Electronic Arts Inc.

    \Version 03/13/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _gameserverdiagnostic_h
#define _gameserverdiagnostic_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// monitor flags
#define VOIPSERVER_MONITORFLAG_GAMESERVOFFLN    (VOIPSERVER_MONITORFLAG_LAST << 1)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// diagnostic callback processing for voipserver
int32_t GameServerDiagnosticCallback(ServerDiagnosticT *pServerDiagnostic, const char *pUrl, char *pBuffer, int32_t iBufSize, void *pUserData);

// diagnostic get type callback
ServerDiagnosticResponseTypeE GameServerDiagnosticGetResponseTypeCb(ServerDiagnosticT *pServerDiagnostic, const char *pUrl);

#endif // _gameserverdiagnostic_h

