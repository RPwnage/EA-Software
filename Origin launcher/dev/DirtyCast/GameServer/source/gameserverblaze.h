/*H********************************************************************************/
/*!
    \File    gameserverblaze.h

    \Description
        This module handles the interactions with the BlazeSDK and manages our Blaze
        game, blazeserver connection

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _gameserverblaze_h
#define _gameserverblaze_h

/*** Include files ****************************************************************/

#include "gameserverconfig.h"
#include "gameserverlink.h"

/*** Type Definitions *************************************************************/

//! state of the module
typedef enum GameServerBlazeStateE
{
    kDisconnected,
    kConnecting,
    kConnected,
    kCreatedGame,
    kWaitingForPlayers,
    kGameStarted
} GameServerBlazeStateE;

//! callback fired for different gameserverblaze events
typedef uint8_t (GameServerBlazeCallbackT)(void *pUserData, int32_t iEvent, GameServerGameT *pGame);

typedef struct GameServerDistT GameServerDistT;

/*** Functions ********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// startup the gameserver blaze module
int32_t GameServerBlazeCreate(GameServerConfigT *pConfig, const char *pCommandTags, const char *pConfigTags);

// shutdown the gameserver blaze module
void GameServerBlazeRelease(void);

// update the gameserver blaze module
uint8_t GameServerBlazeUpdate(void);

// status selector function
int32_t GameServerBlazeStatus(int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// control function
int32_t GameServerBlazeControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// set the callbacks
void GameServerBlazeCallback(GameServerBlazeCallbackT *pCallback, GameServerLinkSendT *pSend, GameServerLinkDiscT *pDisc, GameServerLinkClientEventT *pEvent, void *pCallbackData);

// login into the blazeserver
uint8_t GameServerBlazeLogin(void);

// disconnect from the blazeserver
void GameServerBlazeDisconnect(void);

// create a game
void GameServerBlazeCreateGame(void);

// destroy the game
void GameServerBlazeDestroyGame(void);

// kick a client from the game
void GameServerBlazeKickClient(int32_t iClientId, uint16_t uReason);

// get the last removed reason
uint32_t GameServerBlazeGetLastRemovalReason(char *pBuffer, int32_t iBufSize, uint32_t *pGameState);

// get the error code name
const char *GameServerBlazeGetErrorCodeName(uint32_t uErrorCode);

// get gameserverlink module
GameServerLinkT *GameServerBlazeGetGameServerLink(void);

// get gameserverdis module
GameServerDistT *GameServerBlazeGetGameServerDist(void);

#if defined(__cplusplus)
}
#endif

#endif // _gameserverblaze_h

 
