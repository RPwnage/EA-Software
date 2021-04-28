/*H********************************************************************************/
/*!
    \File gameserverdist.c

    \Description
        This module implements dist functionality in the game server.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 02/04/2007 (jbrookes) First Version
    \Version 11/25/2009 (mclouatre) Added support for mem group user data
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/game/netgamedistserv.h"

#include "dirtycast.h"
#include "gameserverconfig.h"
#include "gameserverlink.h"
#include "gameserverdist.h"
#include "gameserveroimetrics.h"


/*** Defines **********************************************************************/

//! memid for gameservercust module
#define GAMESERVERDIST_MEMID            ('gsds')

/*** Type Definitions *************************************************************/

//! module state
struct GameServerDistT
{
    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //! max client count
    int32_t iMaxClients;

    //! verbosity level
    int32_t iVerbosity;

    //! used to track flow control
    uint32_t uFlowTime;

    //! tracks if no-input mode is enabled
    uint8_t bNoInputMode;
    
    //! prev no input mode when metrics was queried
    uint8_t bPrevNoInputMode;

    //! no input mode changed between metrics queries
    uint8_t bNoInputModeChanged;

    //! used to snuff logging of initial no-input at start of game
    uint8_t bInitialStall;
    
    //! DistServ ref
    NetGameDistServT *pDistServ;
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function GameServerDistCreate

    \Description
        Create the module state.

    \Input *pCommandTags    - command-line config overrides
    \Input *pConfigTags     - config parameters
    \Input iMaxClients      - maximum number of clients
    \Input iVerbosity       - debug output verbosity
    
    \Output
        GameServerDistT *   - new module or NULL if creation failed

    \Version 02/04/2007 (jbrookes)
*/
/********************************************************************************F*/
GameServerDistT *GameServerDistCreate(const char *pCommandTags, const char *pConfigTags, int32_t iMaxClients, int32_t iVerbosity)
{
    GameServerDistT *pDist;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    
    // allocate and init module state
    if ((pDist = (GameServerDistT *)DirtyMemAlloc(sizeof(*pDist), GAMESERVERDIST_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("gameserverdist: could not allocate module state\n");
        return(NULL);
    }
    ds_memclr(pDist, sizeof(*pDist));
    pDist->iMaxClients = iMaxClients;
    pDist->iMemGroup = iMemGroup;
    pDist->pMemGroupUserData = pMemGroupUserData;
    pDist->iVerbosity = iVerbosity;

    // create DistServ ref
    if ((pDist->pDistServ = NetGameDistServCreate(iMaxClients, iVerbosity)) == NULL)
    {
        DirtyCastLogPrintf("gameserverdist: could not allocate DistServ module state\n");
        DirtyMemFree(pDist, GAMESERVERDIST_MEMID, pDist->iMemGroup, pDist->pMemGroupUserData);
        return(NULL);
    }

    NetGameDistServControl(pDist->pDistServ, 'mpty', DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.pausethreshold", 4), 0, NULL);
    NetGameDistServControl(pDist->pDistServ, 'crcc', DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.crcchallenge", 0), 0, NULL);
    NetGameDistServControl(pDist->pDistServ, 'crcr', DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.crcresponse", 0), 0, NULL);
    NetGameDistServControl(pDist->pDistServ, 'rate', DirtyCastConfigGetNumber(pCommandTags, pConfigTags, "gameserver.fixedrate", 0), 0, NULL);
    
    // return ref to caller
    return(pDist);
}

/*F********************************************************************************/
/*!
    \Function GameServerDistDestroy

    \Description
        Destroy the module state.

    \Input *pDist   - module state
    
    \Output
        None.

    \Version 02/04/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerDistDestroy(GameServerDistT *pDist)
{
    if (pDist->pDistServ != NULL)
    {
        NetGameDistServDestroy(pDist->pDistServ);
    }

    DirtyMemFree(pDist, GAMESERVERDIST_MEMID, pDist->iMemGroup, pDist->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function GameServerDistClientEvent

    \Description
        Handle client event

    \Input *pClientList - list of clients
    \Input eEvent       - client event
    \Input iClient      - index of client event is for
    \Input *pUpdateData - module state
    
    \Output
        None.

    \Version 02/04/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerDistClientEvent(GameServerLinkClientListT *pClientList,  GameServLinkClientEventE eEvent, int32_t iClient, void *pUpdateData)
{
    GameServerDistT *pDist = (GameServerDistT *)pUpdateData;
    GameServerLinkClientT *pLinkClient = &pClientList->Clients[iClient];
    
    if (eEvent == GSLINK_CLIENTEVENT_CONN)
    {
        char strClientName[12];
        ds_memclr(strClientName, sizeof(strClientName));
        ds_snzprintf(strClientName, sizeof(strClientName), "0x%08x", pLinkClient->iClientId);
        NetGameDistServAddClient(pDist->pDistServ, iClient, pLinkClient->pGameLink, strClientName);
    }
    else if (eEvent == GSLINK_CLIENTEVENT_DEL)
    {
        NetGameDistServDelClient(pDist->pDistServ, iClient);
    }
    else if (eEvent == GSLINK_CLIENTEVENT_DSC)
    {
        NetGameDistServDiscClient(pDist->pDistServ, iClient);
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerDistUpdateClient

    \Description
        Handle client update.

    \Input *pClientList - list of clients
    \Input iClient      - index of client
    \Input *pUpdateData - module state
    
    \Output
        int32_t         - negative=disconnected from upper layer, else zero

    \Version 02/04/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t GameServerDistUpdateClient(GameServerLinkClientListT *pClientList, int32_t iClient, void *pUpdateData, char *pLogBuffer)
{
    GameServerDistT *pDist = (GameServerDistT *)pUpdateData;
    int32_t iHighWaterInputQueue, iHighWaterOutputQueue;
    int32_t iStatus;

    if (iClient >= 0)
    {
        iStatus = NetGameDistServUpdateClient(pDist->pDistServ, iClient);

        if ((iStatus < 0) && (pLogBuffer != NULL))
        {
            ds_strnzcpy(pLogBuffer, NetGameDistServExplainError(pDist->pDistServ, iClient), GSLINK_DEBUGBUFFERLENGTH);
        }
    }
    else
    {
        int32_t iRet;
        NetGameDistServUpdate(pDist->pDistServ);
        iStatus = 0;

        if ((iRet = NetGameDistServStatus(pDist->pDistServ, 'ninp', NULL, 0)) != -1)
        {
            int32_t iDelay = NetGameDistServStatus(pDist->pDistServ, 'ftim', NULL, 0);

            if ((iRet != 0) && !pDist->bNoInputMode)
            {
                pDist->uFlowTime = NetTick();
            }
            else if ((iRet == 0) && pDist->bNoInputMode)
            {
                if (!pDist->bInitialStall)
                {
                    int32_t iDelta = NetTickDiff(NetTick(), pDist->uFlowTime);
                    if ((iDelta > 500) || (pDist->iVerbosity > 0))
                    {
                        DirtyCastLogPrintf("gameserverdist: no input mode is now disabled (%d ms); last flow packet was seen %d ms ago\n", iDelta, iDelay);
                    }
                }
                pDist->bInitialStall = FALSE;
            }
            pDist->bNoInputMode = (iRet != 0) ? TRUE : FALSE;

            if (pDist->bPrevNoInputMode != pDist->bNoInputMode)
            {
                pDist->bNoInputModeChanged = TRUE;
            }
        }
    }

    if ((pDist->iVerbosity > DIRTYCAST_DBGLVL_CONNINFO) && NetGameDistServHighWaterChanged(pDist->pDistServ, &iHighWaterInputQueue, &iHighWaterOutputQueue))
    {
        DirtyCastLogPrintf("gameserverdist: new highwater: queues (input=%d, output=%d)\n", iHighWaterInputQueue, iHighWaterOutputQueue);
    }

    return(iStatus);
}

/*F********************************************************************************/
/*!
    \Function GameServerDistStartGame

    \Description
        Handle start game event.

    \Input *pClientList - list of clients
    \Input *pUpdateData - module state

    \Version 03/05/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerDistStartGame(GameServerLinkClientListT *pClientList, void *pUpdateData)
{
    GameServerDistT *pDist = (GameServerDistT *)pUpdateData;
    pDist->bNoInputMode = TRUE;
    pDist->bInitialStall = TRUE;
    pDist->uFlowTime = NetTick();
}

/*F********************************************************************************/
/*!
    \Function GameServerDistStopGame

    \Description
        Handle stop game event.

    \Input *pClientList - list of clients
    \Input *pUpdateData - module state

    \Version 05/20/2008 (jbrookes)
*/
/********************************************************************************F*/
void GameServerDistStopGame(GameServerLinkClientListT *pClientList, void *pUpdateData)
{
    GameServerDistT *pDist = (GameServerDistT *)pUpdateData;
    pDist->bInitialStall = TRUE;
}

/*F********************************************************************************/
/*!
    \Function GameServerDistStatus

    \Description
        Get status information.

    \Input *pDist   - pointer to module state
    \Input iSelect  - status selector
    \Input iValue   - selector specific
    \Input pValue   - selector specific

    \Output
        int32_t             - selector specific

    \Notes
        falls through to NetGameDistServStatus2()
        
    \Version 09/18/2019 (tcho)
*/
/********************************************************************************F*/
int32_t GameServerDistStatus(GameServerDistT * pDist, int32_t iSelect, int32_t iValue, void *pValue, int32_t iBufSize)
{   
    // fall through to NetGameDistServStatus2
    return(NetGameDistServStatus2(pDist->pDistServ, iSelect, iValue, pValue, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function GameServerDistControl

    \Description
    Get status information.

    \Input *pDist   - pointer to module state
    \Input iControl - control selector
    \Input iValue   - selector specific
    \Input pValue   - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        falls through to NetGameDistServControl()

    \Version 09/18/2019 (tcho)
*/
/********************************************************************************F*/
int32_t GameServerDistControl(GameServerDistT * pDist, int32_t iControl, int32_t iValue, void *pValue, int32_t iBufSize)
{
    // fall through to NetGameDistControl
    return(NetGameDistServControl(pDist->pDistServ, iControl, iValue, iBufSize, pValue));
}