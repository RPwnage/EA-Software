/*H*************************************************************************************************/
/*!

    \File xnfriends.h

    \Description
	\Description
		 Implements any Xenon Friends specific operations,for Xbox 360.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2004.  ALL RIGHTS RESERVED.

    \Version 1.0 08/20/2005 (TE) First Version
*/
/*************************************************************************************************H*/

#ifndef _xnfriends_h
#define _xnfriends_h

 

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/


//from xonline.h, but, dont want to include, so will re-define.
//TODO find proper def.
#define XONLINE_GAMERTAG_SIZE 16

#define XONLINE_MAX_STATEDATA_SIZE MAX_STATEDATA_SIZE

// arbitrary max-but we will split and feed ALL changes to uppper level.
#define XBFRIENDS_MAX_CHANGED_FRIENDS 25

// game invites -- can set or receive max of this as game state.
#define XBFRIENDS_GAME_STATE_LEN 32

 //! what actions that occured to friends, as in change callback  
typedef enum XBFRIENDS_Action { ACTION_NONE=0, 
  			ACTION_ADD='A', 
  			ACTION_CHANGE='C', 
  			ACTION_DELETE='D'
            };
 
 
// container for what changed in a friend, since last enumeration.
typedef struct XnFriendsChangeT
{
	char  strGamertag [ XONLINE_GAMERTAG_SIZE] ;
	int32_t  eAction; //Add, Change or Delete
	//uint32_t  uState; //current state of Friends 
	uint32_t uChangeFlags ;//what changed since last enum.
} XnFriendsChangeT;

typedef struct XnFriendsRefT XnFriendsRefT;

//! Function prototype for custom mem alloc function.
typedef void *(MemAllocT)(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);

//! Function prototype for custom mem free function.
typedef void (MemFreeT)(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData);

//The callback signatures..
//! whenever a friend changes state -- for the tickle (TCKL) to server.
typedef void (XnFriendsChangeCBT) ( XnFriendsRefT * pRef, int32_t count, int32_t bFirstEnumDone, XnFriendsChangeT *aChange, void * pData);


//!lets us know specifics on a game invite flag change.
typedef void (XnFriendsSignalCBT)(XnFriendsRefT *pRef, char * strGamertag, int32_t action, int32_t dwState, uint32_t dwTitleId, void *pUserData); 


/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Create a new Xbox Friend Interface
XnFriendsRefT *XnFriendsCreate( MemAllocT *pAllocFn,  MemFreeT *pFreeFn  );

// Destroy an existing interface
void XnFriendsDestroy(XnFriendsRefT *pState);

// Connect to the Friends Service -- set up Enumeration.
int32_t XnFriendsConnect(XnFriendsRefT *pRef,  int32_t iTimeout,  
				  XnFriendsChangeCBT *pCallback, void *pCalldata);

//get friend at this index position..
XONLINE_FRIEND *XnFriendsGetFriend (XnFriendsRefT *pRef, int32_t iIndex)  ;

//get friend by gamertag name
XONLINE_FRIEND*	XnFriendsGetFriendByName ( XnFriendsRefT *pRef, char *pName);


// how many in my list?
int32_t XnFriendsGetFriendCount (XnFriendsRefT *pRef)  ;
// Assign the debug callback
void XnFriendsDebug(XnFriendsRefT *pState, void *pDebugData, void (*pDebugProc)(void *pData, const char *pText));

// Disconnect from the Friends Service.
int32_t XnFriendsDisconnect(XnFriendsRefT *pState);

// Handle periodic update -- will call the change callback if Friends change.
void XnFriendsUpdate(XnFriendsRefT *pState);

// Give it a function for dumping debug output.
void XnFriendsSetDebugFunction (  XnFriendsRefT  * pApiRef, void *pDebugData, void (*pDebugProc)(void *pData, const char *pText));

//! Talk to XDK for a cross game invite operation, like accept or reject.
//int32_t XnFriendsDoGameAction (XnFriendsRefT *pState, char * gamertag, int32_t eAction );

//!Optionally, set the State String for a Game Invite.  
//void XnFriendsSetGameInviteStateString(XnFriendsRefT *pRef, const char * pState);

// Callback for when game is in ready state,after game invite--
//void xbfSetGameReadyCallback (XnFriendsRefT *pStat, XnFriendsSignalCBT *pProc, void * pData );


// 1 means cross game there, 2 join game, 0 means no game, -1 or less -- some error
//int32_t XnFriendsHasAcceptedGame  (  char * strGameStateint,int32_t iBufLen  );

// check for accepted game, while offline. see XnFriendsHasAcceptedGame.
//int32_t XnFriendsOfflineHasAcceptedGame  (  char * strGameState,int32_t iBufLen );

// Test method --to do buddy action on Xdk. NOT used for Production.
int32_t XnFriendsDoAction (XnFriendsRefT *pState, char * gamertag, int32_t eAction );

// Get name of Title -- pLang is ignored, returns English name.
//int32_t XnFriendsGetTitleName (XnFriendsRefT *pRef, const uint32_t uTitleId, const char * pLang, const int32_t iBufSize, char * pTitleName);

// Resume processing (enumerating) after a suspend.
//xx int32_t	 XnFriendsResume (XnFriendsRefT *pState);

// Suspend processing to save cycles and/or memory during a game.
//xx void	 XnFriendsSuspend (XnFriendsRefT *pState);

// See if this title ID is the same as mine
//xx int32_t XnFriendsIsSameTitleID(XnFriendsRefT *pRef, const uint32_t uTitleID);

// Get the TitleID for a particular user --returns 0 if user not found, or other error.
uint32_t XnFriendsGetUserTitleID(XnFriendsRefT *pRef, const char * strGamertag);
 
// Set the SessionID that is passed to the game invitation
//void XnFriendsSetGameInviteSessionID(XnFriendsRefT *pRef, const char * sSessionID);
 
// Get the SessionID for a particular user
int32_t XnFriendsGetUserSessionID(XnFriendsRefT *pRef, const char * strGamertag, char * sSessionID, const int32_t iBufSize);
 
// Set the enumerator index for the XFriendsCreateEnumerator function
int32_t XnFriendsSetUserIndex(XnFriendsRefT  * pApiRef,int32_t iIndex);

// Get the current index used by XFriendsCreateEnumerator
int32_t XnFriendsGetUserIndex(XnFriendsRefT  * pApiRef);


//test method
//char * xbfMe ( XnFriendsRefT *pRef );

#ifdef __cplusplus
}
#endif

//@}
#endif // _xnfriends_h

