/*H*************************************************************************************************/
/*!

\File xnfriends.c

\Description
This module	implements and Xbox	Friends	specific operations,for the Xbox 360  (Xenon).

\Copyright
Copyright (c)  Electronic Arts 2004.  ALL RIGHTS RESERVED.

\Version 1.0 02/24/2004	(TE) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <xtl.h>
#include <xonline.h> 

#include "dirtysock.h"
#include "dirtyerr.h"
#include "dirtymem.h"
#include "xnfriends.h"
#include "netconn.h"

/*** Defines ***************************************************************************/

// return status from the enumerate call..
#define XBFRIENDS_ENUM_SUCCESS 0
#define XBFRIENDS_ENUM_IGNORE -1
#define XBFRIENDS_ENUM_DISCONNECT -2


/*** Type Definitions ******************************************************************/

//my collection	of friends -- includes the count of	friends.
typedef	struct FCollection
{
    int32_t	iCount;
    XONLINE_FRIEND	aFriends  [	MAX_FRIENDS	];
    //HasherRef		*pHashTable	;
} FCollection;


//!	current	module state
struct XnFriendsRefT				  
{
    // module memory group
    int32_t iMemGroup;          //!< module mem group id
    void *pMemGroupUserData;    //!< user data associated with mem group
    
    MemAllocT *pAllocFn;        //! override function to alloc Mem.
    MemFreeT *pFreeFn;          //! override function to free Mem.
	int32_t bFirstEnumDone;     //! Have we dont initialization, as we dont need to send TCKL initially.
    int32_t bConnected;         //! Connected?
    DWORD dwLastFriendsEnumerate; //! tick count of  when we last ran enumerate..
    int32_t					iCompareIndex; //ptr to old vs new friends buffer.
    FCollection			aCompareBuff [2]; //old,new
    XnFriendsChangeT	aChange	[XBFRIENDS_MAX_CHANGED_FRIENDS +1]; //changed friends I will tell caller.

    void *pChangeData;			//!	data for something changed callback
    XnFriendsChangeCBT *pChangeProc; //! something changed callback	function

    void *pDebugData;			//!	data for debug callback
    void (*pDebugProc)(  void	*pData,	const char *pText);	//!	debug callback function
    
    //char xxMe  [17];  //!  temp -- tracks who iam
    XNKID SessionID;            //! session id, can be set, for game invites.

    int32_t iEnumeratorIndex;
    XOVERLAPPED XOverlapped;
    int32_t iEnumerationStatus;
    HANDLE hFriendsEnum;
    //XONLINETASK_HANDLE      hMutelistStartupTask; //need to add to list --remove
//    char strGameInviteState [XONLINE_MAX_STATEDATA_SIZE+1]; //! Game Invite State string
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables
 
/*** Private Functions *****************************************************************/

void _HandleChangedFriends(XnFriendsRefT *pRef, int32_t count, FCollection *pNewF, XnFriendsChangeT *aChange);
 
/*F*************************************************************************************************/
/*!
\Function _Printf

\Description
Handle debug output

\Input *pState	- reference	pointer
\Input *pFormat	- format string
\Input ...		- varargs

\Output	None.

*/
/*************************************************************************************************F*/
static void	_Printf(XnFriendsRefT *pState,const char *pFormat,	...)
{
    va_list	args;
    char strText[4096];

    // parse the data
    va_start(args,pFormat);
    vsprintf(strText,pFormat,args);
    va_end(args);

    // pass	to caller routine if provied
    if (pState->pDebugProc != NULL)
    {
        (pState->pDebugProc)(pState->pDebugData,strText);
    }
}

/*-----------------------------------------------------------------------------
// Name: xPrint()
// Desc: Send formatted	output to the debug	window
//TODO remove -- we	use	_Printf	for	debug instead..
//-----------------------------------------------------------------------------
VOID __cdecl xxPrint(const char* strFormat,	...	)
{
int32_t	MAX_OUTPUT_STR = 512;
char strBuffer[512 ];
int32_t	iChars;
va_list	pArglist;
va_start(pArglist,	strFormat);
iChars=	wvsprintf(strBuffer,strFormat,pArglist);
assert(	iChars < MAX_OUTPUT_STR	);
OutputDebugStringA("***	xbf: ");
OutputDebugStringA(strBuffer);
(VOID) iChars;
va_end(	pArglist);
}
*/

//!	Given a	FRIEND,	append relevant	stuff to a "change"	array.
static void _appendChangedFriend (	 int32_t eAction,XONLINE_FRIEND frnd,uint32_t	uChangeFlags,XnFriendsChangeT *aChange,int32_t *pIndex)
{
    aChange	[*pIndex].eAction =	eAction;
    //aChange	[*pIndex].uState =	frnd.dwFriendState;
    aChange	[*pIndex].uChangeFlags =  uChangeFlags;
    strnzcpy	(aChange[*pIndex].strGamertag,frnd.szGamertag,	XONLINE_GAMERTAG_SIZE);
    *pIndex	+=1;
}

//!	Find the friends index,	using xuid provided,in	the	Friends	array.
int32_t _findFriend(FCollection *fc, XUID xId)
{

    int32_t	i;
    for	(i=0; i	< fc->iCount; i++)
    {
        if	(fc->aFriends[i].xuid	== xId	)
            return	i;
    }
    return(-1);
}

//!	Find a friend	in collection,by name.	
XONLINE_FRIEND*	 _findFriendByName (FCollection *fc,	const char * name	)
{
    int32_t	i =0;
    for	(i=0; i	< fc->iCount; i++)
    {
        if	(stricmp (fc->aFriends[i].szGamertag, name)	 ==	0)
        {
            return	&(fc->aFriends[i]); 
        }
    }
    return(NULL);
}

//Given a gamertag, find that friend in the Friends enumeration..
XONLINE_FRIEND*	XnFriendsGetFriendByName ( XnFriendsRefT *pRef, char *pName)
{

    return(_findFriendByName(&(pRef->aCompareBuff[pRef->iCompareIndex]), pName)); //.aFriends[iIndex
}

//!	Compare	an old set of friends with new,	and	find changes to	state,and adds,deletes 
//  and call needed callbacks
void _compareFriendsAndCB(XnFriendsRefT *pRef, FCollection *pOldF, FCollection *pNewF, XnFriendsChangeT *aChange)
{
    int32_t	i = 0;
    int32_t oldi = -1; 
    int32_t	iChangeCount = 0;
    uint32_t changeFlags = 0;

    //first	go thru	new,and find added	or changed...
    for	(i = 0; i < pNewF->iCount; i++)
    {
        oldi = _findFriend (pOldF,pNewF->aFriends[i].xuid);
        if (oldi == -1)
        {
            _appendChangedFriend(ACTION_ADD, pNewF->aFriends[i], pNewF->aFriends[i].dwFriendState, aChange, &iChangeCount);
        }
        else if	(pOldF->aFriends[oldi].dwFriendState !=	pNewF->aFriends[i].dwFriendState)
        {
            changeFlags = (pOldF->aFriends[oldi].dwFriendState ^ pNewF->aFriends[i].dwFriendState);
            _appendChangedFriend(ACTION_CHANGE, pNewF->aFriends[i], changeFlags,aChange, &iChangeCount);
        }
		// the change buffer is limited, so feed what we have..and rested
        if ( iChangeCount >= XBFRIENDS_MAX_CHANGED_FRIENDS)
        {   //callback and reset 
            _HandleChangedFriends(pRef, iChangeCount, pNewF, aChange);
            iChangeCount = 0;
        }
    }

    //go thru old list and find	what was zapped.
    for	(i=0; i	< pOldF->iCount; i++)
    {
        if (_findFriend(pNewF, pOldF->aFriends[i].xuid) == -1)
        { // in	old,but missing in new
            _appendChangedFriend(ACTION_DELETE ,pOldF->aFriends[i], pOldF->aFriends[i].dwFriendState, aChange, &iChangeCount);
        }

        if ( iChangeCount >= XBFRIENDS_MAX_CHANGED_FRIENDS)
        {  //callback and reset
            _HandleChangedFriends(pRef, iChangeCount, pNewF, aChange);
            iChangeCount = 0;
        }
    }

    if (iChangeCount != 0)
    { //if any left, do callback
        _HandleChangedFriends(pRef, iChangeCount, pNewF, aChange);
    }
}

void _HandleChangedFriends(XnFriendsRefT *pRef, int32_t count, FCollection *pNewF, XnFriendsChangeT *aChange)
{
    //check and call game ready state callback..
    //_checkGameReady(pRef, count, pNewF, pRef->aChange);	

	// if cb defined  
    if (pRef->pChangeProc != NULL) 
    {   // also, tell if this is first time or not, as server does not need initial TCKL.
		(pRef->pChangeProc)(pRef, count, pRef->bFirstEnumDone, pRef->aChange, pRef->pChangeData);
    }
}

//! Periodically,	Enumerate tasks,and check if changes show up. 
int32_t _enumerateFriends(XnFriendsRefT *pRef, DWORD dwUser, void *unusedhFriends, void *unusedhEnumerate, FCollection *fc)
{
    int32_t result;

    result = NetConnStatus('frnd', dwUser, fc->aFriends, sizeof(fc->aFriends));
    fc->iCount = result;

    return(XBFRIENDS_ENUM_SUCCESS);
}


//!See if other user who accepted my game invite is back online in my title, and call callback.


/*F*************************************************************************************************/
/*!
    \Function XnFriendsDisconnect

    \Description
        Disconnect from the Friends service

    \Input *pRef    - reference pointer

    \Output
        int32_t     - error code (zero=success, negative=error)
*/
/*************************************************************************************************F*/
int32_t	XnFriendsDisconnect(XnFriendsRefT *pRef)
{
    // close task handles if they are open
    HRESULT hr =0;
    // reset and stop enumerating..
    pRef->iCompareIndex = 0; //ptr to old vs new friends buffer.
    memset(pRef->aCompareBuff, 0, sizeof(pRef->aCompareBuff)); //old,new
    memset(pRef->aChange, 0, sizeof(pRef->aChange));
    pRef->bConnected = 0; // stop enumerate..
    return(hr);
}



/*F*************************************************************************************************/
/*!
    \Function XnFriendsConnect

    \Description
        Connect to the XBOL Friends service and initialize.local call.

    \Input *pRef        - referencepointer
    \Input iTimeout     - time out
    \Input pCallback    - callback
    \Input pCallData    - callback data

    \Output
        int32_t         - error code (zero=success, negative=error)
*/
/*************************************************************************************************F*/
int32_t	XnFriendsConnect(XnFriendsRefT *pRef, int32_t iTimeout,XnFriendsChangeCBT *pCallback, void *pCallData)
{
    int32_t rc=-1;
    if (pRef)
    {
        pRef->pChangeProc	=pCallback;
        pRef->pChangeData	=pCallData;
        
 //clear state too..
        pRef->bFirstEnumDone =0;
        pRef->bConnected =1;
        pRef->dwLastFriendsEnumerate = NetTick();
        pRef->iCompareIndex = 0; //ptr to old vs new friends buffer.
        memset(pRef->aCompareBuff, 0, sizeof(pRef->aCompareBuff)); //old,new
        memset(pRef->aChange, 0, sizeof(pRef->aChange));
        rc = 0;
        pRef->bConnected = 1; // allow enumerate..
    }
    return(rc);
}

void XnFriendsUpdate(XnFriendsRefT *pRef)
{
    int32_t	rc=0;
    FCollection	*pOldF;
    FCollection	*pNewF;
    int32_t oldIndex;

    if (pRef == NULL)
        return;

    if (pRef->bConnected == 0) //connect first..
        return;

    // get the old and new pointers,to	be used	for	comparision. 
    pNewF =	& (pRef->aCompareBuff [pRef->iCompareIndex]);

    //reload new friends --	if Xbox	has	a set for us.
    rc = _enumerateFriends(pRef, pRef->iEnumeratorIndex, NULL, NULL, pNewF);

    if (rc == -1) //nothing happened, ignore..
        return;

    if (rc != XBFRIENDS_ENUM_SUCCESS	)
    { // actual error..
        _Printf(pRef, "Enumerate returnd error %d\n", rc);
        return;
    }

    oldIndex = (pRef->iCompareIndex+1)	%2;
    pOldF =	& (pRef->aCompareBuff [oldIndex]);
    _compareFriendsAndCB(pRef, pOldF, pNewF, pRef->aChange);
    pRef->iCompareIndex = oldIndex; //cb done, so change ptr..
    pRef->bFirstEnumDone = 1; // we are done first enumeration, so change state.

}

/*F*************************************************************************************************/
/*!
\Function	 XnFriendsSetDebugFunction	

\Description
Set	a callback to write	out	 debug messages	within API.

\Input *pApiRef	- API reference	pointer
\Input *pDebugData	  -pointer to any caller data to be	output with	the	prints.	
\Input *pDebugProc	- Function pointer that	will write debug info. 

\Output
None.

\Notes
Caller sets	this so	debug info from	buddy api will show	up in
whatever screen	or log caller has.
*/
/*************************************************************************************************F*/


void XnFriendsSetDebugFunction ( XnFriendsRefT *pApiRef, void *pDebugData, void  (*pDebugProc)(void *pData,const	char *pText))
{
    if (pApiRef == NULL)
        return;

    pApiRef->pDebugProc	= pDebugProc;  
    pApiRef->pDebugData	= pDebugData;
}


//!	Create a new Friends Reference API interface.
XnFriendsRefT *XnFriendsCreate( MemAllocT *pAllocFn,  MemFreeT *pFreeFn  )
{
    XnFriendsRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
     
   // pRef = pAllocFn(sizeof (*pRef));
	pRef = DirtyMemAlloc(sizeof(*pRef), XNFRIENDS_MEMID, iMemGroup, pMemGroupUserData);
    memset(pRef,0,sizeof (*pRef));
    pRef->iMemGroup = iMemGroup;  
    pRef->pMemGroupUserData = pMemGroupUserData;   
    pRef->pAllocFn = pAllocFn;
    pRef->pFreeFn = pFreeFn;
    return(pRef);
}

// how many in my list?
int32_t XnFriendsGetFriendCount (XnFriendsRefT *pRef)  
{//warning, DONT Update between this call and iterating via GetFriend..
    return(pRef->aCompareBuff[pRef->iCompareIndex].iCount);
}

XONLINE_FRIEND *XnFriendsGetFriend (XnFriendsRefT *pRef, int32_t iIndex)  
{
    if (iIndex < 0 )
        return(NULL);

    if (iIndex >= pRef->aCompareBuff[pRef->iCompareIndex].iCount)
        return(NULL);

    return(&(pRef->aCompareBuff[pRef->iCompareIndex].aFriends[iIndex]));
}

//! Free any memory held by api.
void XnFriendsDestroy( XnFriendsRefT *pRef)
{
    if ((pRef) )
    {
        DirtyMemFree(pRef, XNFRIENDS_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
    }
}


int32_t XnFriendsDoAction(XnFriendsRefT *pRef, char * strGamertag, int32_t eAction)
{
    int32_t	hr= -1;
    return(hr);
}
/*F*************************************************************************************************/
/*!
    \Function	 XnFriendsDoGameAction	

    \Description
        Does Game Invite Operations via the client XDK.
        There were MS server issues and hence we do game invites solely through client.

    \Input pRef         - pointer to this app.
    \Input strGamertag  - the buddy  I am inviting/rejecting etc. to game.
    \Input eAction      - Game action like, Invite, Accept to perform. See XBFRIENDS_Action.

    \Output
         0 if not errors. -1 otherwise.
*/
/*************************************************************************************************F*/
int32_t XnFriendsDoGameAction(XnFriendsRefT *pRef, char * strGamertag, int32_t eAction)
{
    int32_t	hr =-1;
    return(hr);
}

int32_t XnFriendsGetUserSessionID(XnFriendsRefT *pRef, const char * strGamertag, char * sSessionID, const int32_t iBufSize)
{
    XONLINE_FRIEND *pFrnd;
    int32_t i;
    if (pRef == NULL)
        return(-1);

    i = pRef->iCompareIndex;

    i = (i+1) %2; //want the old.
    if ((pFrnd = _findFriendByName(&(pRef->aCompareBuff[i]),strGamertag)) != NULL)
    {
        strnzcpy(sSessionID, (char *)pFrnd->sessionID.ab, iBufSize);
        return(0);
    }

    // didn't find our guy
    return(-1);
}



/*F********************************************************************************/
/*!
    \Function    XnFriendsSetUserIndex

    \Description
        Set the enumerator index for the XFriendsCreateEnumerator function

    \Input *pApiRef - reference to the Xenon Friends Api Structure
    \Input iIndex   - the index we want to set it to

    \Output int32_t - success

    \Version 03/02/2006 (ozietman)
*/
/********************************************************************************F*/
int32_t XnFriendsSetUserIndex(XnFriendsRefT  * pApiRef,int32_t iIndex)
{
    _Printf(pApiRef, "Setting new User Index %d\n", iIndex);
    pApiRef->iEnumeratorIndex = iIndex;
    return(0);
}


/*F********************************************************************************/
/*!
    \Function    XnFriendsGetUserIndex

    \Description
        Get the current index used by XFriendsCreateEnumerator

    \Input *pApiRef - reference to the Xenon Friends API Structure

    \Output int32_t - Index being used by XFriendsCreateEnumerator function

    \Version 03/02/2006 (ozietman)
*/
/********************************************************************************F*/
int32_t XnFriendsGetUserIndex(XnFriendsRefT  * pApiRef)
{
    return(pApiRef->iEnumeratorIndex);
}
