/*H********************************************************************************/
/*!
    \File gameserverdist.h

    \Description
        This module implements dist functionality in the game server.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 02/04/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _gameserverdist_h
#define _gameserverdist_h

/*** Include files ****************************************************************/

#include "DirtySDK/game/netgamedistserv.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state
typedef struct GameServerDistT GameServerDistT;

//! oi metric data
typedef struct GameServerOiMetricDataT GameServerOiMetricDataT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
GameServerDistT *GameServerDistCreate(const char *pCommandTags, const char *pConfigTags, int32_t iMaxClients, int32_t iVerbosity);

// destroy the module
void GameServerDistDestroy(GameServerDistT *pDist);

// handle client event
void GameServerDistClientEvent(GameServerLinkClientListT *pClientList, GameServLinkClientEventE eEvent, int32_t iClient, void *pUpdateData);

// update client
int32_t GameServerDistUpdateClient(GameServerLinkClientListT *pClientList, int32_t iClient, void *pUpdateData, char *pLogBuffer);

// handle start game event
void GameServerDistStartGame(GameServerLinkClientListT *pClientList, void *pUpdateData);

// handle stop game event
void GameServerDistStopGame(GameServerLinkClientListT *pClientList, void *pUpdateData);

// gameserver dist status 
int32_t GameServerDistStatus(GameServerDistT * pDist, int32_t iSelect, int32_t iValue, void *pValue, int32_t iBufSize);

// gameserver dist control
int32_t GameServerDistControl(GameServerDistT * pDist, int32_t iControl, int32_t iValue, void *pValue, int32_t iBufSize);

#ifdef __cplusplus
}
#endif

#endif // _gameserverdist_h

