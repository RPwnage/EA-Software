/*H********************************************************************************/
/*!
    \File netgamedistserv.h

    \Description
        Server module to handle 2+ NetGameDist connections in a client/server
        architecture.

    \Notes
        None.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 02/01/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _netgamedistserv_h
#define _netgamedistserv_h

/*** Include files ****************************************************************/

#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamedist.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module state
typedef struct NetGameDistServT NetGameDistServT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module state
NetGameDistServT *NetGameDistServCreate(int32_t iMaxClients, int32_t iVerbosity);

// destroy the module state
void NetGameDistServDestroy(NetGameDistServT *pDistServ);

// add a client to module
int32_t NetGameDistServAddClient(NetGameDistServT *pDistServ, int32_t iClient, NetGameLinkRefT *pLinkRef, const char *pClientName);

// del a client from module
int32_t NetGameDistServDelClient(NetGameDistServT *pDistServ, int32_t iClient);

// notify that a client disconnected
int32_t NetGameDistServDiscClient(NetGameDistServT *pDistServ, int32_t iClient);

// update a client
int32_t NetGameDistServUpdateClient(NetGameDistServT *pDistServ, int32_t iClient);

// start the game
void NetGameDistServStartGame(NetGameDistServT *pDistServ);

// stop the game
void NetGameDistServStopGame(NetGameDistServT *pDistServ);

// update the module (must be called at fixed rate)
void NetGameDistServUpdate(NetGameDistServT *pDistServ);

// whether the highwater mark changed, and the current highwater values.
uint8_t NetGameDistServHighWaterChanged(NetGameDistServT *pDistServ, int32_t* pHighWaterInputQueue, int32_t* pHighWaterOutputQueue);

// return the lastest error reported by netgamedist, for client iClient
char* NetGameDistServExplainError(NetGameDistServT *pDistServ, int32_t iClient);

// netgamedistserv control
int32_t NetGameDistServControl(NetGameDistServT *pDistServ, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue); 

// netgamedistserv status
int32_t NetGameDistServStatus(NetGameDistServT *pDistServ, int32_t iSelect, void *pBuf, int32_t iBufSize);


#ifdef __cplusplus
}
#endif

#endif // _netgamedistserv_h

