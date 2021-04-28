/*H********************************************************************************/
/*!
    \File gameserver.h

    \Description
        This module implements the main logic for the GameServer server.

        Description forthcoming.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _gameserver_h
#define _gameserver_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// max login failure history
#define GAMESERVER_LOGINHISTORYCNT      (5)

// shutdown flags
#define GAMESERVER_SHUTDOWN_IMMEDIATE   (1)
#define GAMESERVER_SHUTDOWN_IFEMPTY     (2)

#define GAMESERVER_MAX_TIME_DISTR       (64)

/*** Type Definitions *************************************************************/

//! game history stats
typedef struct GameServerLoginHistoryT
{
    time_t uFailTime;               //!< time login failure occurred
    uint32_t uFailKind;             //!< kind of request that failed
    uint32_t uFailCode;             //!< code of request that failed
    uint32_t uFailCount;            //!< number of consecutive failures
} GameServerLoginHistoryT;

//! gameserver opaque module ref
typedef struct GameServerRefT GameServerRefT;

//! configuration module forward declaration
typedef struct GameServerConfigT GameServerConfigT;

/*** Functions *********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// initialize this instance from the provided config file
GameServerRefT *GameServerCreate(int32_t iArgc, const char **pArgv, const char *pConfigFile);

// shut this instance down
void GameServerDestroy(GameServerRefT *pGameServer);

// main process loop
uint8_t GameServerUpdate(GameServerRefT *pGameServer, uint32_t uShutdownFlags);

// status function
int32_t GameServerStatus(GameServerRefT *pGameServer, int32_t iStatus, int32_t iValue, void *pBuf, int32_t iBufSize);

// control function
int32_t GameServerControl(GameServerRefT *pGameServer, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// get configuration
const GameServerConfigT *GameServerGetConfig(GameServerRefT *pGameServer);

#if defined(__cplusplus)
}
#endif

#endif // _gameserver_h

