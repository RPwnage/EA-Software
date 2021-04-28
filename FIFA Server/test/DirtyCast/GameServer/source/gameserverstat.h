/*H********************************************************************************/
/*!
    \File gameserverstat.h

    \Description
        This module implements stat reporting to the lobby server.
        
        Description forthcoming.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 02/28/2007 (jbertrand) First Version
*/
/********************************************************************************H*/

#ifndef _gameserverstat_h
#define _gameserverstat_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//! GameServer stat structure used to report statistics to lobby server
typedef struct GameServerStatT
{
    uint32_t uTimeHosting;      //!< Total time spent hosting games
    uint32_t uGameCreateTime;   //!< Time of game creation
    uint32_t uIdleStartTime;    //!< Time since last hosted game ended
    uint8_t uGamesHosted;       //!< Total number of games hosted
} GameServerStatT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// update GameServer statistics on a game creation
void GameServerUpdateStatsGameCreate(GameServerStatT *pGameStats);

// update GameServer statistics on a game deletion
void GameServerUpdateStatsGameDelete(GameServerStatT *pGameStats);

#ifdef __cplusplus
}
#endif

#endif // _gameserverstat_h

