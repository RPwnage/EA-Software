/*H********************************************************************************/
/*!
    \File gameserveroimetrics.h

    \Description
        Game Server OI metrics reporting system.

    \Copyright
        Copyright (c) 2019 Electronic Arts Inc.

    \Version 07/30/2019 (tcho)
*/
/********************************************************************************H*/

#ifndef _gameserveroimetrics_h
#define _gameserveroimetrics_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! forward declaration
typedef struct GameServerOiMetricsRefT GameServerOiMetricsRefT;
typedef struct GameServerRefT GameServerRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

// create this helper module to manage metrics
GameServerOiMetricsRefT *GameServerOiMetricsCreate(GameServerRefT* pGameServer);

// update function
void GameServerOiMetricsUpdate(GameServerOiMetricsRefT *pMetricsRef);

// control function
int32_t GameServerOiMetricsControl(GameServerOiMetricsRefT *pMetricsRef, int32_t iControl, int32_t iData, void *pBuf, int32_t iBufSize);

//destroy this module
void GameServerOiMetricsDestroy(GameServerOiMetricsRefT *pMetricsRef);

#if defined(__cplusplus)
}
#endif

#endif // _stressgameplaymetrics_h
