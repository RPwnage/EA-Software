/*H********************************************************************************/
/*!
    \File   buddytickerglue.h

    \Description
        buddytickerglue passes events from the buddy module to the ticker module.

    \Notes
        This library is separate to keep from introducing dependencies between the
        modules that would be otherwise required.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 05/05/2009 (cvienneau) First Version
*/
/********************************************************************************H*/

#ifndef _lobbyglue_h
#define _lobbyglue_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct BuddyTickerGlueRefT BuddyTickerGlueRefT;

//! presence formatting function callback type
typedef void (BuddyTickerGluePresCallbackT)(void *pCbData, char *pBuffer, int32_t iBufSize, const char *pPersona, uint32_t bSignedIn);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// initialize buddytickerglue module 
BuddyTickerGlueRefT *BuddyTickerGlueCreate(HLBApiRefT *pHLBRef, TickerApiT *pTickerRef);

// shut down lobbyglue module
void BuddyTickerGlueDestroy(BuddyTickerGlueRefT *pGlueRef);

// set localization
void BuddyTickerGlueSetLocalization(BuddyTickerGlueRefT *pGlueRef, BuddyTickerGluePresCallbackT *pPresCb, void *pPresCbData, uint32_t bHomeAway, const char *pFriendsStr);

// set a buddy->ticker score
int32_t BuddyTickerGlueSetScore(HLBApiRefT *pHLBRef, const char *pHomeUser, int32_t iHomeScore, const char *pAwayUser, int32_t iAwayScore, const char *pStatus, const char *pPresSame);

#ifdef __cplusplus
};
#endif

#endif // _lobbyglue_h

