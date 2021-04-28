/*H********************************************************************************/
/*!
    \File gameserverstat.c

    \Description
        This module implements stat reporting to the lobby server.
        
    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 02/28/2007 (jbertrand) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "gameserverstat.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function GameServerUpdateStatsGameCreate
    
    \Description
        Update GameServer statistics on a game creation.
    
    \Input *pGameServer   - game server ref
    
    \Output
        None.
            
    \Version  02/28/2007 (jbertrand)
*/
/********************************************************************************F*/
void GameServerUpdateStatsGameCreate(GameServerStatT *pGameStats)
{
    pGameStats->uGameCreateTime = NetTick();
    pGameStats->uGamesHosted++;
}

/*F********************************************************************************/
/*!
    \Function GameServerUpdateStatsGameDelete
    
    \Description
        Update GameServer statistics on a game deletion.
    
    \Input *pGameServer   - game server ref
    
    \Output
        None.
            
    \Version  02/28/2007 (jbertrand)
*/
/********************************************************************************F*/
void GameServerUpdateStatsGameDelete(GameServerStatT *pGameStats)
{
    int32_t iGameTime;
    pGameStats->uIdleStartTime = NetTick();
    iGameTime = (pGameStats->uIdleStartTime - pGameStats->uGameCreateTime);
    if (iGameTime > 0)
    {
        pGameStats->uTimeHosting += (unsigned)iGameTime;
    }
}
