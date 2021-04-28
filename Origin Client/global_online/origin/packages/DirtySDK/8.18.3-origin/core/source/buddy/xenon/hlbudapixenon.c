/*H*************************************************************************************/
/*!
    \File hlbudapixenon.c

    \Description
        This module implements High Level Buddy, an application level interface 
        to Friends for Xenon. This is LARGELY stubbed out, as Xenon does 
        not expose many Friends functions, and expect clients to use HUD.

    \Copyright
        Copyright (c) Electronic Arts 2005.  ALL RIGHTS RESERVED.
*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <xtl.h>
#include <xonline.h> 

#include "dirtysock.h"
#include "dirtymem.h"
#include "lobbydisplist.h"
#include "lobbyhasher.h"
#include "hlbudapi.h"
#include "xnfriends.h"

/*** Defines ***************************************************************************/

#define true 1
#define false 0

/*** Macros ****************************************************************************/
//To compile out debug print strings in Production release. 
#if DIRTYCODE_LOGGING
#define HLBPrintf(x) _HLBPrintfCode x
#else
#define HLBPrintf(x)
#endif



/*** Type Definitions ******************************************************************/

//! current module state
struct HLBApiRefT                  
{
    BuddyMemAllocT *pBuddyAlloc;    //!< buddy memory allocation function.
    BuddyMemFreeT *pBuddyFree;      //!< buddy memory free function.
	XnFriendsRefT *pLLRef;          //low level api.

    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group
    
	int32_t iStat;                  //!<  status of this api.
	
	void *pListData;                //! client context data for buddy list callback
	HLBOpCallbackT   *pListProc;    //! buddy list  or presence change callback function  

	void *pOpData;                  //!< client context  data for Operation callback
	HLBOpCallbackT *pOpProc; 
	void *pConnectData;             //!< client context  data for Connect callback
	HLBOpCallbackT *pConnectProc; 

	int32_t rosterDirty;	        //!< had buddy list or presence been updated?
	int32_t (*pBuddySortFunc) (void *pSortref, int32_t iSortcon, void *pRecptr1, void *pRecptr2);
	void (*pDebugProc)(void *pData, const char *pText); //client provide debug for printing errors.
	void * pUserRef;                //for any user data ..

	HLBPresenceCallbackT *pBuddyPresProc; //!< c/b for when presence changes.
	void *pBuddyPresData;           //! context data for buddy presence callback.

    HasherRef *pHash;               //!< hash table for the "roster" of friends
    DispListRef *pDisp;             //!< display list  "roster" of friends
   
	//configurable constants ..
};

//actually, for now is a XONLINE_FRIEND struct..
struct  HLBBudT     
{
    XONLINE_FRIEND  bud; //!< private to lower level
};

/*** Function Prototypes ***************************************************************/
void _HLBRosterCallback(BuddyApiRefT *pLLRef, BuddyApiMsgT *pMsg, void *pUserData);
int32_t _HLBListGetIndexByName (HLBApiRefT *pApiRef, const char *name);
void _reloadFriends (HLBApiRefT *pApiRef);
HLBBudT *_updateFriendInRoster( HLBApiRefT  * pApiRef, PXONLINE_FRIEND pFriend);
int32_t _XlateState (int32_t XOnlineState);
/*** Variables *************************************************************************/

// Private variables

// Public variables

//*** Private Functions *****************************************************************

#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function HLBPrintfCode

    \Description
        Handle debug output

    \Input *pApiRef - reference pointer
    \Input *pFormat - format string
    \Input ...      - varargs

    \Output None.
*/
/*************************************************************************************************F*/
static void _HLBPrintfCode( HLBApiRefT *pApiRef, const char *pFormat, ...)
{

	va_list args;
	char strText[4000];
	char finalText[4096+12]; 

	if (pApiRef->pDebugProc == NULL)
		return; // nothing to do, no client debug proc.
	// parse the data
	va_start(args, pFormat);
	vsprintf(strText, pFormat, args);
	va_end(args);
	sprintf (finalText, "HLB: %s \n", strText); //todo fixup -- we dont need
	// pass to caller routine if provied
	(pApiRef->pDebugProc)("", finalText);
}
#endif


void _signalBuddyListChanged ( HLBApiRefT *pApiRef )
{
	pApiRef->rosterDirty =true;
	if  ( ( pApiRef ) && (pApiRef->pListProc ))
    {
        pApiRef->pListProc ( pApiRef, HLB_OP_LOAD,HLB_ERROR_NONE, pApiRef->pListData ); 
    }
}

// List wrappers -- until we come up with appropriate list implementation.

//! return count of low level roster list.
 int32_t _rosterCount (HLBApiRefT *pApiRef)
 {
     if  (( pApiRef ==NULL ) || pApiRef->pDisp == NULL)
     {
        return(0);
     }
    return(DispListCount(pApiRef->pDisp));
 }
 
 int32_t _countBuddies ( HLBApiRefT *pApiRef,  uint32_t flag)
 {
     return(_rosterCount(pApiRef));
 }

 void _RosterDestroy(HLBApiRefT *pApiRef)
{
	DispListT *pRec;
    BuddyRosterT *pBuddy;

    // make sure flush is enabled
    if (pApiRef->pDisp != NULL)
    {
	    // free the resources
	    while ((pRec = HasherFlush(pApiRef->pHash)) != NULL)
        {
            pBuddy =(DispListDel(pApiRef->pDisp, pRec));
            //give caller a chance to free mem, if needed.
            //if (pApiRef->pBuddyDelProc )
            //{
               // pApiRef->pBuddyDelProc (pApiRef, pBuddy, pApiRef->pBuddyDelData);
            //}
            pApiRef->pBuddyFree(pBuddy, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
        }


        // done with resources
	    HasherDestroy(pApiRef->pHash);
	    pApiRef->pHash = NULL;
	    DispListDestroy(pApiRef->pDisp);
	    pApiRef->pDisp = NULL;

	    
    }
}


//! Helper killer function to nuke all invites -- NO Callbacks. To be
//! used when blocking a buddy, and want all invites nuked. 
int32_t HLBListCancelAllInvites ( HLBApiRefT  *pApiRef, const char *pName)
{
    return(HLB_ERROR_UNSUPPORTED);   
}

void HLBListDisableSorting( HLBApiRefT  *pApiRef  )
{
	if ((pApiRef) && ( pApiRef->pDisp))
	{
		// Just before we disable sorting we should sort the list one last time.
		// Force the list to appear to be changed so it will get resorted.
      
		DispListChange(pApiRef->pDisp, 1);

		// Resort the list.
		DispListOrder( pApiRef->pDisp);

		// Now disable sorting by setting the sort function to NULL.
		DispListSort( pApiRef->pDisp, NULL, 0, NULL);
	}
}

void _HLBEnableSorting( HLBApiRefT  *pApiRef )
{
     
	if ( pApiRef->pDisp  != NULL)
	{
		DispListSort( pApiRef->pDisp ,  pApiRef , 0,  pApiRef->pBuddySortFunc);
		// Force the list to appear to be changed so it will get resorted.
		DispListChange( pApiRef->pDisp, 1);
		// Resort the list.
		DispListOrder( pApiRef->pDisp );
	}
}

int32_t _HLBRosterSort(void *pSortref, int32_t iSortcon, void *pRecptr1, void *pRecptr2)
{
	const BuddyRosterT *pBuddy1 = (BuddyRosterT *)pRecptr1;
	const BuddyRosterT *pBuddy2 = (BuddyRosterT *)pRecptr2;
    HLBBudT *pHBuddy1 = (HLBBudT *)pBuddy1->pUserRef;
    HLBBudT *pHBuddy2 = (HLBBudT *)pBuddy2->pUserRef;
	int32_t iResult;
    int32_t on1 = HLBBudGetState (pHBuddy1);
    int32_t on2 = HLBBudGetState (pHBuddy2);
    if (on1 == on2)
	{ // are same, so sort by name
		iResult = ds_stricmp (pBuddy1->strUser, pBuddy2->strUser);
	}
    else
    { //online goes on top, so make it lower.
        if ((on1== BUDDY_STAT_CHAT) || (on2 == BUDDY_STAT_CHAT ))
            iResult = (on1 == BUDDY_STAT_CHAT)?-1:1;
        else //passive (in game) goes above  offline, so make it lower.
           iResult =  (on1 >on2 )? -1:1;
    }
	return(iResult);
}


//! When Xenon friends determines state of friends changes, we get told in aChange array..

static void _FriendsChangeCB ( XnFriendsRefT * pRef, int32_t count, int32_t bFirstEnumDone, XnFriendsChangeT *aChange, void * pData)
{
    int32_t i=0;
    HLBStatE eOldState;

    HLBBudT *pBuddy;
    
    HLBApiRefT *pApiRef = (HLBApiRefT*) pData;

    if ((pData == NULL) || (pApiRef == NULL))
    {
        return;
    }
    for ( i =0; i <count; i++)
    {
        XONLINE_FRIEND *pXonlineFriend;
        if ( aChange[i].eAction == ACTION_CHANGE)
        { //presence callback --must have changed state..
            pXonlineFriend = XnFriendsGetFriendByName(pApiRef->pLLRef, aChange[i].strGamertag);
            pBuddy = HLBListGetBuddyByName(pApiRef,aChange[i].strGamertag);
            if ((pXonlineFriend == NULL) || (pBuddy == NULL))
            {
                continue;
            }
            eOldState = HLBBudGetState(pBuddy);
            pBuddy = _updateFriendInRoster(pApiRef, pXonlineFriend); //new one
            if  (aChange[i].uChangeFlags)
            {
                //presence changed -- pass buddy and old state.
                if (pApiRef->pBuddyPresProc)
                {
                    pApiRef->pBuddyPresProc(pApiRef, pBuddy, eOldState, pApiRef->pBuddyPresData);
                }
            }
        }
    }

    //best to reload all ..but skip changed friends,since we updated.
    _reloadFriends(pApiRef);  //skip=1;
    for (i=0;i<count; i++)
    {
         HLBPrintf(( pApiRef, "HLBApi::**Friend Changed CB, count = %d, name =%s, act=%c\n", count, aChange[i].strGamertag, aChange[i].eAction));
    }
    if (count >0 )
         _signalBuddyListChanged(pApiRef);

}

HLBApiRefT *_HLBApiInit(HLBApiRefT *pApiRef)
{
    if (pApiRef == NULL)
        return(NULL);

    pApiRef->pBuddySortFunc = _HLBRosterSort;
    if  (pApiRef->pLLRef  == NULL )
    {
        DirtyMemGroupEnter(pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
        pApiRef->pLLRef = XnFriendsCreate(NULL,NULL);
        DirtyMemGroupLeave();
    }
    
    if  (pApiRef->pLLRef  == NULL )
    {  //nogo pogo, clean and return
        pApiRef->pBuddyFree(pApiRef, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData) ; 
        return(NULL);
    }
    DirtyMemGroupEnter(pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
    // allocate the roster list and make it active 
    pApiRef->pHash = HasherCreate(1000, 2000);
    pApiRef->pDisp = DispListCreate(100, 100, 0);
    DirtyMemGroupLeave();

    //connect and set state..
    HLBApiConnect(pApiRef,"",0,"","");
    return(pApiRef);
}


/*** Public Functions *****************************************************************/




/*F*************************************************************************************************/
/*!
\Function    HLBApiCreate

\Description
Create an instance of the High Level Buddy API. 

\Input pProduct                 - char * - Product name.      
\Input *pRelease                - char * - Release version
\Input iRefMemGroup             - mem group for hlbapiref -api ref. (zero=default)
\Input *pRefMemGroupUserData    - user data associated with mem group

\Output *HLBApiRefT             - API reference pointer, to be used on subsequent calls.
*/
/*************************************************************************************************F*/

HLBApiRefT *HLBApiCreate(const char *pProduct, const char *pRelease, int32_t iRefMemGroup, void *pRefMemGroupUserData)
{
	HLBApiRefT *pApiRef;

    // if no ref memgroup specified, use current group
    if (iRefMemGroup == 0)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;
    
        // Query current mem group data
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
        
        iRefMemGroup = iMemGroup;
        pRefMemGroupUserData = pMemGroupUserData; 
    }
    
    // allocate new state record
	pApiRef = ( HLBApiRefT*) DirtyMemAlloc (sizeof(*pApiRef), HLBUDAPI_MEMID, iRefMemGroup, pRefMemGroupUserData); 
	if (pApiRef != NULL)
	{
		memset(pApiRef, 0, sizeof(*pApiRef));
        pApiRef->iMemGroup = iRefMemGroup;
        pApiRef->pMemGroupUserData = pRefMemGroupUserData;
        pApiRef->pBuddyAlloc = DirtyMemAlloc; //see HLBApiCreate2 to register user mem funcs.
        pApiRef->pBuddyFree = DirtyMemFree;
    }

    return(_HLBApiInit ( pApiRef));
}

/*F*************************************************************************************************/
/*!
\Function    HLBApiCreate2

\Description
Create HL Buddy Api, overriding the default memory alloc/free functions.

\Input pAlloc                   - char * - function to allocate memory.
\Input pFree                    - char * - function to free memory.
\Input pProduct                 - char * - Product name.      
\Input *pRelease                - char * - Release version
\Input iRefMemGroup             - memgroup for hlbudapi ref (zero=default)
\Input *pRefMemGroupUserData    - user data associated with mem group

\Output *HLBApiRefT             - API reference pointer, to be used on subsequent calls.
*/
/*************************************************************************************************F*/

HLBApiRefT *HLBApiCreate2( BuddyMemAllocT *pAlloc,  BuddyMemFreeT *pFree,
                           const char *pProduct, const char *pRelease, int32_t iRefMemGroup, void *pRefMemGroupUserData)
{
    HLBApiRefT *pApiRef;

    if ((pAlloc == NULL ) || (pFree== NULL))
        return(NULL); // use HLBApiCreate instead..

    // if no ref memgroup specified, use current group
    if (iRefMemGroup == 0)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;

        // Query current mem group data
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    
        iRefMemGroup = iMemGroup;
        pRefMemGroupUserData = pMemGroupUserData;
    }

    // allocate new state record
	pApiRef = ( HLBApiRefT*) DirtyMemAlloc (sizeof(*pApiRef), HLBUDAPI_MEMID, iRefMemGroup, pRefMemGroupUserData); 
	if (pApiRef != NULL)
	{
		memset(pApiRef, 0, sizeof(*pApiRef));
        pApiRef->iMemGroup = iRefMemGroup;
        pApiRef->pMemGroupUserData = pRefMemGroupUserData;
        pApiRef->pBuddyAlloc = pAlloc;
        pApiRef->pBuddyFree = pFree;
    }
    return(_HLBApiInit ( pApiRef));
}

/*F*************************************************************************************************/
/*!
\Function    HLBApiSetDebugFunction 

\Description
Set a callback to write out  debug messages within API.

\Input *pApiRef - API reference pointer
\Input *pDebugData    -pointer to any caller data to be output with the prints. 
\Input *pDebugProc  - Function pointer that will write debug info. 

\Output
None.

\Notes
Caller sets this so debug info from buddy api will show up in
whatever screen or log caller has.
*/
/*************************************************************************************************F*/

void HLBApiSetDebugFunction (  HLBApiRefT  * pApiRef, void *pDebugData, void (*pDebugProc)(void *pData, const char *pText))
{
    if ( (pApiRef ==NULL ) || (pDebugProc ==NULL ) ) 
        return;
    pApiRef->pDebugProc = pDebugProc; //use it here, and in lower levels..
    //BuddyApiDebug( pApiRef->pLLRef, pDebugData, pDebugProc );
}

//!Application  may override if XOnlineTitleIdIsSameTitle is also to be used to check for
//! is same product. Usually use PROD field from server, and for Xbl, the XOnlineTitleIdIsSameTitle too
//! to allow for "offline" users, who wont have a PROD field, in presence, when offline.
//!Set to true(1), if say, NTSC and PAL cannot play each other, but, both have same XBL TitleId.
 
void HLBApiOverrideXblIsSameProductCheck (HLBApiRefT  * pApiRef, int32_t bDontUseXBLSameTitleIdCheck)
{
    return;  // HLB_ERROR_UNSUPPORTED;   
}


//!Application  may override default for max msg per permanent buddy  
void HLBApiOverrideMaxMessagesPerBuddy (HLBApiRefT  * pApiRef, 
		                                int32_t  max_msgs_per_perm_buddy) 

{
    return;  // HLB_ERROR_UNSUPPORTED;   
}
//!Application  may override default constants. -1 leaves it as default. Only valid before HLBApiConfig is called
void HLBApiOverrideConstants (HLBApiRefT  * pApiRef, 
        int32_t bNoTextSupport,
		int32_t connect_timeout, 
		int32_t op_timeout,  
		int32_t max_perm_buddies, 
		int32_t max_temp_buddies,
		int32_t max_msgs_allowed,
		int32_t bTalk_to_xbox,
		int32_t max_msg_len,
		int32_t bTrackSendMsgInOpState,
		int32_t iGameInviteAccept_timeout)
//only valid before HLBApiInitialize is called..
{
    return;  // HLB_ERROR_UNSUPPORTED;
}


//!Set if want api to translate utf8 chars to 8 bit characters in Presence, and Message Text.
void HLBApiSetUtf8TransTbl ( HLBApiRefT  * pApiRef, Utf8TransTblT *pTransTbl)
{
    return;  // HLB_ERROR_UNSUPPORTED;
}


/*F*************************************************************************************************/
/*!
    \Function    HLBApiInitialize

    \Description
        Configure  and initialize any data structures in the API, and set some basic product and presence info. 

    \Input *pApiRef         - HL api reference pointer
    \Input *prodPresence    - Identifies the product and used for init presence.
    \Input uLocality        - the language this client is running in. Will be used to extract "other product" presence information --typically, NOT used.

    \Output
        return status -- HLB_ERROR_NONE if ok.

    \Notes
        By server magic, the field prodPresence controls HLBBudIsSameProduct, so make 
        sure this is unique for your product. This Might also be used to set initial presence, 
        but, use the Presence Api to set your presence on connecting, and all will be fine.
*/
/*************************************************************************************************F*/
int32_t HLBApiInitialize( HLBApiRefT  * pApiRef, const char * prodPresence , uint32_t uLocality )

{   
    if (!pApiRef)
        return(HLB_ERROR_MISSING);
    return(0);

		//return(BuddyApiConfig( pApiRef ->pLLRef,  prodPresence , NOTUSED,  lang,  1 , 1)); 
								 //=1 todo  wont load MAIN.fiso11 if not  ; bReconnect == 1
	//ProductId is used with ProdPresence to determine which presence to  return from same/other.
	// langList -- is like "enfr" where first item is language  you want other presence in.

}
/*F*************************************************************************************************/
/*!
    \Function  HLBApiConnect

    \Description
        Not needed for Xenon. Creating API connect to XDK as well.

    \Input *pApiRef     - reference pointer
    \Input *pServer     - Not relevant for Xenon
    \Input port         - Not relevant for Xenon
    \Input *key         - Not relevant for Xenon
    \Input *pRsrc       - Not relevant for Xenon

    \Output
        int32_t         - HLB_ERROR_NONE if ok.
*/
/*************************************************************************************************F*/

int32_t HLBApiConnect( HLBApiRefT  * pApiRef, const char * pServer, int32_t port, const  char *key, const char *pRsrc )
{
	if  ( (  pApiRef  == NULL ) ||   (  pApiRef->pLLRef == NULL ) )
		return(HLB_ERROR_BADPARM);
   
  	//XX _HLBRosterDestroy( pApiRef ); //just in case, they had mem around, clear it.

    pApiRef->iStat = BUDDY_STAT_CHAT; //anything but PASSive -- as doing connect.
//	pApiRef->connectState = HLB_CS_Connected;
    if (!pApiRef->pLLRef)
    {
        DirtyMemGroupEnter(pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
        pApiRef->pLLRef = XnFriendsCreate(pApiRef->pBuddyAlloc, pApiRef->pBuddyFree);
        DirtyMemGroupLeave();
    }
    XnFriendsConnect(pApiRef->pLLRef,0,_FriendsChangeCB,pApiRef);
    return(0);
}

int32_t HLBApiRegisterConnectCallback( HLBApiRefT  * pApiRef,  HLBOpCallbackT *cb , void * pContext)
{
    if  (   pApiRef  == NULL )
        return(HLB_ERROR_BADPARM);
    pApiRef->pConnectProc = cb;
    pApiRef->pConnectData = pContext;
    return(HLB_ERROR_NONE);
}

/*F*************************************************************************************************/
/*!
\Function    HLBApiGetConnectState

\Description
Get state of connection to the server, returns int32_t, but see HLBConnectState_Enum. 

\Input *pApiRef     -HL api reference pointer
\Output
connect state -- int32_t, but see HLBConnectState_Enum.

\Notes
None.
*/
/*************************************************************************************************F*/

//! What is state of connection to the server.  
int32_t HLBApiGetConnectState ( HLBApiRefT  * pApiRef)
{
	//return saved state, or Disconnected if api is null.
    int32_t iCS = HLB_CS_Disconnected;
	if (pApiRef)
    {
        iCS = (pApiRef->iStat == BUDDY_STAT_CHAT)? HLB_CS_Connected : HLB_CS_Disconnected;
    }
	return(iCS);
}

/*F*************************************************************************************************/
/*!
\Function    HLBApiDisconnect

\Description
Disconnect from buddy server.

\Input *pApiRef     -HL api reference pointer
\Output
none  
*/
/*************************************************************************************************F*/

//! Disconnect  from server.
void  HLBApiDisconnect ( HLBApiRefT  * pApiRef) 
{
    if (pApiRef == NULL)
        return;

    if (pApiRef->pLLRef )
    {
        XnFriendsDisconnect( pApiRef->pLLRef );
        pApiRef->iStat = BUDDY_STAT_DISC;
    }
}

/*F*************************************************************************************************/
/*!
\Function    HLBApiDestroy

\Description
Destroy this API and any memory held by it.  Also destroy lower level BuddyApi held by this.
\Input *pApiRef     -API reference pointer
\Output
none  
*/
/*************************************************************************************************F*/

//! Destroy api and any memory held..
void HLBApiDestroy( HLBApiRefT *pApiRef)  
{
	if ( pApiRef == NULL )
    {
		return; //aint much to do..
    }

	if (pApiRef->pLLRef )
    {
		XnFriendsDestroy (pApiRef->pLLRef);
    }
    _RosterDestroy(pApiRef);
    // clear the callback
    pApiRef->pListData = NULL;
    pApiRef->pListProc = NULL;

    DirtyMemFree(pApiRef, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);  //now i free me..
}
/*F*************************************************************************************************/
/*!
    \Function HLBApiUpdate

    \Description
      Give API and api below some time to process. 
      Must be called periodically by client.

    \Input *pApiRef - reference pointer

    \Output None.

*/
/*************************************************************************************************F*/

void HLBApiUpdate (HLBApiRefT  * pApiRef) 
{
	 // idle time tasks
	if ( !pApiRef)
		return;

	 XnFriendsUpdate(pApiRef->pLLRef );  
	
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiSuspend

    \Description
       Free up buddy memory,and stop enumerating, while client goes in game.

    \Input *pApiRef    - reference pointer

    \Output None.

*/
/*************************************************************************************************F*/

int32_t  HLBApiSuspend( HLBApiRefT  * pApiRef) 
{ 
    // Suspend goes thru even if not connected. This is because, if server disconnected
    // and went it game, we want to be in PASSive state, when server connects. If we 
    // are not, we will end up loading rosters, and game will very unhappy.

    if ( pApiRef == NULL )
    {
		return(HLB_ERROR_MISSING); //aint much to do..
    }

	if (pApiRef->pLLRef )
    {
        XnFriendsDisconnect( pApiRef->pLLRef ); //stop enumerating..
    }
    _RosterDestroy(pApiRef);
   
	return(HLB_ERROR_NONE);
}
 
/*F*************************************************************************************************/
/*!
    \Function HLBApiResume

    \Description
        Reconnect after being in game.

    \Input *pApiRef    - reference pointer

    \Output The Roster held onto by this API. NULL if  errors.
*/
/*************************************************************************************************F*/

void * HLBApiResume(HLBApiRefT  * pApiRef)  
{
    
    // Allow a resume even if NOT connected, as need to clear PASSive state. If not,
    // will be in a bad state if suspend/resume happened when server had disconnected.

    // if already connected ignore and have memory allocated ignore re-allocation
    if(pApiRef->pDisp == NULL)
    {
        _HLBApiInit(pApiRef);
    }
	pApiRef->iStat = BUDDY_STAT_CHAT; //any other than PASSive
    return(pApiRef->pDisp); 
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiGetLastOpStatus

    \Description
    State of last operation --implies only one operation at  a time.  
      **** NOT FOR XENON ***

    \Input *pApiRef    - reference pointer

    \Output  int32_t -- state of last operation. See HLB_ERROR_*. Returns HLB_ERROR_NOOP if nothing is outstanding.
*/
/*************************************************************************************************F*/

int32_t HLBApiGetLastOpStatus (HLBApiRefT  * pApiRef) 
{
	return(HLB_ERROR_UNSUPPORTED);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiCancelOp

    \Description
      NOT FOR XENON
 
	 
    \Input *pApiRef    - reference pointer

    \Output  NONE.
*/
/*************************************************************************************************F*/

void HLBApiCancelOp (HLBApiRefT  * pApiRef )
{
    return;
/*
	if (pApiRef->pOpProc )
	{
		pApiRef->pOpProc (pApiRef, HLB_OP_ANY, HLB_ERROR_CANCEL, pApiRef->pOpData  );
		pApiRef->pOpProc = NULL;
	}	  
	pApiRef->iLastOpState =HLB_ERROR_CANCEL; //clear pending state unconditionally.
*/
}
/*F*************************************************************************************************/
/*!
    \Function HLBApiFindUsers

    \Description
     **** NOT FOR XENON ***

    \Input *pApiRef     - reference pointer
    \Input *pName       -  User name pattern to search. Min 3 characters, server add the wildcard.
    \Input *pOpCallback - Callback function to be called when search is completed.
    \Input *context     - context
*/
/*************************************************************************************************F*/
int32_t HLBApiFindUsers( HLBApiRefT  * pApiRef, const char * pName , HLBOpCallbackT * pOpCallback, void * context)
{
    return(HLB_ERROR_UNSUPPORTED);
}
/*F*************************************************************************************************/
/*!
    \Function HLBApiUserFound

    \Description
        **** NOT FOR XENON ***

    \Input *pApiRef     - reference pointer
    \Input iItem        - zero based item number in list.

    \Output 
       BuddyApiUserT*   -user name and domain, null if  not responed yet, or item not there.
*/
/*************************************************************************************************F*/
BuddyApiUserT *HLBApiUserFound ( HLBApiRefT  * pApiRef , int32_t iItem)
{
    return(NULL);   // HLB_ERROR_UNSUPPORTED;
}


/*F*************************************************************************************************/
/*!
    \Function HLBApiRegisterNewMsgCallback

    \Description
        Register callback for new (chat) messages. 
       **** NOT FOR XENON ***

    \Input *pApiRef         - reference pointer
    \Input *pMsgCallback    - Callback function to be called  a chat message arrives.
    \Input *pAppData        - User data or context that will be passed back to caller in the callback.  

    \Output
        int32_t             - HLB_ERROR_NONE if success. BUDDY_ERROR_FULL if try to register more than one callback.
*/
/*************************************************************************************************F*/
//! Register callback to inform caller on any new message waiting
int32_t HLBApiRegisterNewMsgCallback( HLBApiRefT  * pApiRef, HLBMsgCallbackT * pMsgCallback, void * pAppData )
{
	return(HLB_ERROR_UNSUPPORTED);
}	

/*F*************************************************************************************************/
/*!
    \Function HLBApiRegisterBuddyChangeCallback

    \Description
        Register callback for any buddy list (aka roster) change.

    \Input *pApiRef         - reference pointer
    \Input *pListCallback   - callback function to be called  a chat message arrives.
    \Input *pAppData        - user data or context that will be passed back to caller in the callback.  

    \Output return code -- HLB_ERROR_NONE if success. BUDDY_ERROR_FULL if try to register more than one callback.   
*/
/*************************************************************************************************F*/
int32_t HLBApiRegisterBuddyChangeCallback( HLBApiRefT  * pApiRef, HLBOpCallbackT * pListCallback, void * pAppData) 
{
    if ( (pApiRef->pListProc != NULL) && (pListCallback != NULL) )
    {
        HLBPrintf( ( pApiRef, "ERROR: Cannot have more than one buddy change callback"));
        return(HLB_ERROR_FULL); //caller must clear first -- only one supported.
    }
    pApiRef->pListProc = pListCallback;
    pApiRef->pListData = pAppData; // data for message callback
    return(HLB_ERROR_NONE);
}

// Informs caller when Cross Game Invite activity happens. Not neeeded if polling. Local.


/*F*************************************************************************************************/
/*!
    \Function HLBApiRegisterBuddyChangeCallback

    \Description
        **** NOT FOR XENON ***

    \Input *pApiRef             - reference pointer
    \Input *pActionCallback     - Function called when game invite changes for a bud.
    \Input *pAppData            - User data or context that will be passed back to caller in the callback.  

    \Output 
        int32_t                 - HLB_ERROR_NONE if success. BUDDY_ERROR_FULL if try to register more than one callback.   
*/
/*************************************************************************************************F*/
  
int32_t HLBApiRegisterGameInviteCallback( HLBApiRefT  * pApiRef, HLBBudActionCallbackT * pActionCallback, void * pAppData)
{
	return(HLB_ERROR_UNSUPPORTED);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiRegisterBuddyPresenceCallback

    \Description
      Register callback for  when buddy presence changes--state or text can change. 

    \Input *pApiRef         - reference pointer
    \Input *pPresCallback   - Function called when presence changes for a bud.
    \Input *pAppData        - User data or context that will be passed back to caller in the callback.  

    \Output
        int32_t             - HLB_ERROR_NONE if success. BUDDY_ERROR_FULL if try to register more than one callback.   
*/
/*************************************************************************************************F*/
int32_t HLBApiRegisterBuddyPresenceCallback( HLBApiRefT * pApiRef, HLBPresenceCallbackT * pPresCallback, void * pAppData)
{
    if ( (pApiRef->pBuddyPresProc != NULL) && (pPresCallback != NULL) )
    {
        HLBPrintf( ( pApiRef, "ERROR: Cannot have more than one Buddy Presence Callback"));
        return(HLB_ERROR_FULL); //caller must clear first -- only one supported.
    }
    pApiRef->pBuddyPresProc = pPresCallback;
    pApiRef->pBuddyPresData = pAppData; // data for message callback
    return(HLB_ERROR_NONE);
}


/*F*************************************************************************************************/
/*!
    \Function HLBBudGetName

    \Description
        Return screen/persona name of this buddy.
    \Input *pBuddy - pointer to the buddy.
     
    \Output - char * -- the name.
*/
/*************************************************************************************************F*/
  

char * HLBBudGetName(  HLBBudT  *pBuddy )
{
    XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
    return(pFriend->szGamertag);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudGetXenonXUID

    \Description
        Return XUID of buddy --Xenon only! 
 
    \Input *pBuddy - pointer to the buddy.
     
    \Output - uint64_t  --  the XUID.
*/
/*************************************************************************************************F*/
  

uint64_t HLBBudGetXenonXUID(  HLBBudT  *pBuddy )
{
    XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
    return(pFriend->xuid);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsTemporary

    \Description
       **** NOT FOR XENON ***
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t --(boolean) 1 -- if buddy is temporary and hence not persistent.
*/
/*************************************************************************************************F*/

int32_t HLBBudIsTemporary(  HLBBudT *pBuddy )  
{
    return(0); //NO temp buds in this release...
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsBlocked

    \Description

        Indicates if messages from this 'buddy' is ignored 
        **** NOT FOR XENON ***
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t --(boolean) 1 -- if buddy is  ignored or blocked.
*/
/*************************************************************************************************F*/

int32_t HLBBudIsBlocked(  HLBBudT  *pBuddy )  
{
    return(false); //Xbox does NOT keep list of blocked buds..	 
}

 //  Has this buddy, has invited me to be his buddy. Local.
 int32_t HLBBudIsWannaBeMyBuddy( HLBBudT  *pBuddy )
 {
     XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
	 return( (pFriend->dwFriendState &  XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST)?1:0 );
 }
 
 //  Have I invited this user to be my buddy. Local.
 int32_t HLBBudIsIWannaBeHisBuddy( HLBBudT  *pBuddy )
 {
	 XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
	 return(  (pFriend->dwFriendState &  XONLINE_FRIENDSTATE_FLAG_SENTREQUEST )? 1:0);
 }
 
 /*F*************************************************************************************************/
/*!
    \Function HLBBudGetTitle

    \Description

       Return Title played name by this buddy, 
       **** NOT FOR XENON ***
		
    \Input *pBuddy - pointer to the buddy.
     
    \Output - char * -- The utf-8 encoded title name.
	
	\Notes
		NOTE: Depends on server, and should be tied to language set in presence.
		NOTE: Not clear of server implementation for PS2. Simply may not be there.
		NOTE: If user is not online, title will be "empty".

*/
/*************************************************************************************************F*/

 
char * HLBBudGetTitle( HLBBudT  *pBuddy)
{
     return(NULL); //NO API to get title from title id ;-(
}


//! Is this buddy one of these temp buddy kinds -- can be true for permanent buddies too.
int32_t HLBBudTempBuddyIs(  HLBBudT  *pBuddy, int32_t tempBuddyType )
{
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsRealBuddy

    \Description
        Indicates if the buddy is really a buddy -- ie. not someone on the list due to being blocked, or added temporarily. 
		 
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t --(boolean) 1 -- if buddy is a real buddy of mine.
*/
/*************************************************************************************************F*/

int32_t HLBBudIsRealBuddy( HLBBudT  *pBuddy )  
{ 
    int32_t dwMaskPend =    XONLINE_FRIENDSTATE_FLAG_SENTREQUEST   | 
                            XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST;
    XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
    // if no request pending, and not guest must be true buddy -- 
    return(((pFriend->dwFriendState  & dwMaskPend ) ==0) ? 1:0 );
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsInGroup

    \Description
        **** NOT FOR XENON ***
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t --(boolean) 1 -- if buddy is in group.
*/
/*************************************************************************************************F*/

int32_t HLBBudIsInGroup(  HLBBudT  *pBuddy )  
{
    return(0);
}

// get chat state..
int32_t _XlateState (int32_t XOnlineState)
{
    //this overrides -- if playing is probably online so check playing first..
    if ( XOnlineState & XONLINE_FRIENDSTATE_FLAG_PLAYING )
        return(BUDDY_STAT_PASS); //passiva aka playing..

    if ( XOnlineState &  XONLINE_FRIENDSTATE_FLAG_ONLINE )
    { //check for sub states of online..
        if ( XOnlineIsUserAway(XOnlineState) )
            return(BUDDY_STAT_AWAY);
        //if (XOnlineIsUserBusy(XOnlineState))
        //    return(BUDDY_STAT_BUSY);
        return(BUDDY_STAT_CHAT);
    }

    //nothing fit, so must be gonzo..
    return(BUDDY_STAT_DISC);

}
/*F*************************************************************************************************/
/*!
    \Function HLBBudGetState

    \Description
      Return   buddy's state, such as is online, disconnected, in game etc. -- see HLB_Buddy_State_Enum.    
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t -- Buddy's state -- see HLB_Buddy_State_Enum.
*/
/*************************************************************************************************F*/
 
int32_t HLBBudGetState(  HLBBudT  *pBuddy)
{
    XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
    return(_XlateState (pFriend->dwFriendState));
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsPassive

    \Description
      Indicate if buddy is passive i.e. is online but playing a game.
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t -- (boolean) -  1 if buddy is in passive state, i.e. playing a game.
*/
/*************************************************************************************************F*/
 
int32_t HLBBudIsPassive(  HLBBudT  *pBuddy)
{
    XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
    //if playing then same as "passive"..
    return(( pFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_PLAYING)? 1:0 );
}


/*F*************************************************************************************************/
/*!
    \Function HLBBudIsAvailableForChat

    \Description
      Indicate if buddy is online and available for chat.
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t -- (boolean) -  1 if buddy is online and available for chat.
*/
/*************************************************************************************************F*/
 
int32_t HLBBudIsAvailableForChat (  HLBBudT  *pBuddy)
{
    //not disconnected, then ok? TBD passive state ok?
    return((HLBBudGetState (pBuddy) == BUDDY_STAT_DISC)?0:1); 
}


/*F*************************************************************************************************/
/*!
    \Function HLBBudIsSameProduct

    \Description
        **** NOT FOR XENON ***
    \Input *pBuddy - pointer to the buddy.

    \Output - int32_t -- (boolean) -  1 if buddy is online and logged into same product.
*/
/*********************************************************************************************F*/
int32_t HLBBudIsSameProduct (  HLBBudT  *pBuddy) 
{
    return(0); //no way to tell
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudGetPresence

    \Description
        **** NOT FOR XENON ***

    \Input *pBuddy      - pointer to the buddy.
    \Input *strPresence - presence string

    \Output
        None
*/
/*********************************************************************************************F*/

void HLBBudGetPresence( HLBBudT  *pBuddy , char * strPresence)
{
    return;
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudGetPresenceExtra

    \Description
        **** NOT FOR XENON ***

    \Input *pBuddy          - pointer to the buddy.
    \Input *strExtraPres    - the buddy's extra presence string.
    \Input iStrSize         - string size

    \Output
        None
*/
/*********************************************************************************************F*/
void HLBBudGetPresenceExtra( HLBBudT  *pBuddy , char * strExtraPres, int32_t iStrSize )
{
    return;
}

// Is 'special' temp (i.e.admin)  buddy who WONT accept messages or invites etc.   
int32_t HLBBudIsNoReplyBud(  HLBBudT  *pBuddy);  

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsNoReplyBud

    \Description
         **** NOT FOR XENON ***
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t -- (boolean) -  1 if buddy wont accept messages or invites.
*/
/*********************************************************************************************F*/

int32_t HLBBudIsNoReplyBud(  HLBBudT  *pBuddy)
{
    return(false);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsJoinable

    \Description
      Indicates if buddy can be "joined" (aka found and gone to)
	  in an Xbox game.  Checks the Joinable bit in Xbox Presence.
     
    \Output - int32_t -- (boolean) -  1 if buddy wont accept messages or invites.
*/
/*********************************************************************************************F*/

int32_t HLBBudIsJoinable(  HLBBudT  *pBuddy)
{
    XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
    return((pFriend->dwFriendState & XONLINE_FRIENDSTATE_FLAG_JOINABLE)? 1:0 );
}


/*F*************************************************************************************************/
/*!
    \Function HLBBudGetVOIPState

    \Description
      **** NOT FOR XENON ***
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t  --   The buddy's presence voip state.   
*/
/*********************************************************************************************F*/
 
int32_t HLBBudGetVOIPState ( HLBBudT  *pBuddy )
{ 
    return(0); //TBD --is there some equivalent?
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudCanVoiceChat

    \Description
      Indicates if buddy is ready to be talked to. i.e. plugged in, and in same product etc. 
    \Input *pBuddy - pointer to the buddy.
     
    \Output - int32_t  --  (boolean) -1- if plugged in and can be talked to.   
*/
/*********************************************************************************************F*/
 
int32_t HLBBudCanVoiceChat (  HLBBudT  *pBuddy)
{
    XONLINE_FRIEND *pFriend = (PXONLINE_FRIEND) pBuddy;
    int32_t dwMaskChat = XONLINE_FRIENDSTATE_FLAG_VOICE |  XONLINE_FRIENDSTATE_FLAG_ONLINE;
    return((pFriend->dwFriendState & dwMaskChat)? 1:0 );   //?online and can do voice..
}

//Join buddy in another game; **** NOT FOR XENON ***
int32_t HLBBudJoinGame( HLBBudT *pBuddy, HLBOpCallbackT *cb , void * pContext )
{
    return(HLB_ERROR_UNSUPPORTED);    
}
/* LIST functions */

//! Get message at index position, if read is true, mark message as read.

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetMsgByIndex

    \Description
       **** NOT FOR XENON ***
    \Input *pBuddy - pointer to the buddy.
	\Input index - Sero based index position to read message from.
	\Input read - int32_t - (boolean) - if true, message will be marked as read.
     
    \Output - *BuddyApiMsgT  --  Message from buddy at that index pos. NULL if not there. 
*/
/*********************************************************************************************F*/
 

BuddyApiMsgT *  HLBMsgListGetMsgByIndex(  HLBBudT  *pBuddy, int32_t index, int32_t read ) // if true, mark msg as read.
{
    return(NULL);
}
/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetFirstUnreadMsg

    \Description
      **** NOT FOR XENON ***
    \Input *pBuddy - pointer to the buddy.
	\Input startPos - Sero based index position to start looking for unread messages.
	\Input read - int32_t - (boolean) - if true, message will be marked as read.
     
    \Output - *BuddyApiMsgT  --  First unread  Message from buddy. NULL if no more. 
*/
/*********************************************************************************************F*/
 

BuddyApiMsgT *HLBMsgListGetFirstUnreadMsg (  HLBBudT  *pBuddy, int32_t startPos, int32_t read ) // if true, mark msg as read.
{
    return(NULL);
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetMsgText

    \Description
         NOT FOR XENON

    \Input *pApiRef     - module reference
    \Input *pMsg        - pointer to the msg.
    \Input *strText     - Text buffer msg body will be written to.   
    \Input iLen         - Length of text buffer.

    \Output
        None
*/
/*********************************************************************************************F*/
 
void HLBMsgListGetMsgText (HLBApiRefT  * pApiRef,BuddyApiMsgT *pMsg, char * strText, int32_t iLen)
{	
	return;
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetUnreadCount

    \Description
         NOT FOR XENON
    \Input *pBuddy - pointer to the buddy.
	 
     
    \Output - int32_t  -- count of unread messages.
*/
/*********************************************************************************************F*/
 

//! Get number of unread messages from this buddy.
int32_t HLBMsgListGetUnreadCount( HLBBudT  *pBuddy  ) 
{
	return(HLB_ERROR_UNSUPPORTED);
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetTotalCount

    \Description
         NOT FOR XENON
    \Input *pBuddy - pointer to the buddy.
	 
     
    \Output - int32_t  -- count of  messages.
*/
/*********************************************************************************************F*/
 
int32_t HLBMsgListGetTotalCount ( HLBBudT  *pBuddy  )   
{
	return(HLB_ERROR_UNSUPPORTED);  
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListDelete

    \Description
        NOT FOR XENON

    \Input *pBuddy  - pointer to the buddy.
    \Input index    - index

    \Output
        None
*/
/*********************************************************************************************F*/
void HLBMsgListDelete(  HLBBudT  *pBuddy , int32_t index )
{
    return;
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListDeleteAll

    \Description
          NOT FOR XENON
    \Input *pBuddy - pointer to the buddy.
	\Input bZapReadOnly - int32_t (boolean) - if 1, will delete Read messages.
*/
/*********************************************************************************************F*/
 
void HLBMsgListDeleteAll(  HLBBudT  *pBuddy, int32_t bZapReadOnly   )
{
    return;
}

/*F*************************************************************************************************/
/*!
    \Function HLBListGetBuddyByName

    \Description
       Get a buddy from the buddy list (roster) by name. Exact match.

    \Input *pApiRef - module reference
    \Input *name    - name string.

    \Output
        HLBBudT*    - pointer to Buddy structure.  NULL if not there.
*/
/*********************************************************************************************F*/
 
HLBBudT*   HLBListGetBuddyByName ( HLBApiRefT *  pApiRef, const char * name  )
{
    int32_t iIndex = _HLBListGetIndexByName ( pApiRef,  name );
    return(HLBListGetBuddyByIndex(pApiRef, iIndex));

}

/*F*************************************************************************************************/
/*!
    \Function HLBListGetBuddyByIndex

    \Description
        Get a Buddy by index into the buddy list.

    \Input pApiRef  - module reference
    \Input index    - zero based index.

    \Output
        HLBBudT*    - pointer to Buddy structure.  NULL if not there.
*/
/*********************************************************************************************F*/
 
HLBBudT* HLBListGetBuddyByIndex ( HLBApiRefT  * pApiRef, int32_t index )
{ 
    XONLINE_FRIEND *pFriend;
    if (!pApiRef)
    {
        return(NULL);
    }
    pFriend = XnFriendsGetFriend(pApiRef->pLLRef, index);
	return(DispListIndex(pApiRef->pDisp, index));
}

/*F*************************************************************************************************/
/*!
    \Function HLBListGetIndexByName

    \Description
       Given a buddy name, find the index into the list.

    \Input *pApiRef     - API reference pointer
    \Input *name        - name

    \Output int32_t     - index to buddy; negative value ( HLB_ERROR_MISSING ) if not there.
*/
/*********************************************************************************************F*/
 
//! Given a buddy name, find the index into the list. 
int32_t _HLBListGetIndexByName ( HLBApiRefT  * pApiRef, const char * name )
{
    // Go through all entries in the list to find the closest user
    int32_t index, max;
    PXONLINE_FRIEND pHLBBud = NULL;
    for ( index=0, max = HLBListGetBuddyCount ( pApiRef); index <max; index++ )
    {
        pHLBBud  = (PXONLINE_FRIEND) HLBListGetBuddyByIndex (pApiRef, index);
        if ( ( pHLBBud ) && (ds_stricmp(name, pHLBBud->szGamertag ) == 0))
        {
            break;
        }
    }
    if (index >= max )
        index =  HLB_ERROR_MISSING;

    return(index);
}

/*F*************************************************************************************************/
/*!
     \Function HLBListGetBuddyCount 
     \Description
        Get count of all buddies in the Roster. Also see HLBListGetBuddyCountByFlags. 

    \Input *pApiRef     - API Reference pointer.

    \Output
        int32_t         - Total number of buddies.
*/
/***************************************************************************************************/
int32_t HLBListGetBuddyCount ( HLBApiRefT  *pApiRef)
{    //set all flags to 1, to count all buddies in the list.
    return( _countBuddies (pApiRef, (uint32_t)-1) );
}

/*F***********************************************************************************************F*/
/*!
    \Function HLBListGetBuddyCountByFlags

    \Description
       Get count of buddies  that match **** NOT FOR XENON ***

    \Input *pApiRef     -API Reference pointer.
    \Input uTypeFlag    - unsigned bit mask of all buddy  types to look for.

    \Output
        int32_t         - Total number of buddies  that match  any of the type flags specified.
*/
/*********************************************************************************************F*/
int32_t HLBListGetBuddyCountByFlags ( HLBApiRefT  *pApiRef, uint32_t uTypeFlag)
{
    return( _countBuddies (pApiRef, uTypeFlag) );
}

/*F*************************************************************************************************/
/*!
    \Function HLBListBlockBuddy

    \Description
         NOT FOR XENON

    \Input *pApiRef     - API Reference pointer.
    \Input *name        - name
    \Input *cb          - Pointer to optional completion callback function
    \Input *context     - Optional user context that will be passed to callback.  

    \Output
        int32_t     - HLB_ERROR_NONE if command was sent.  

    \Notes 
        Client can use callback, or, use polling to determine status of operation,
        as reported later by the server.
*/
/*********************************************************************************************F*/
 
int32_t HLBListBlockBuddy (   HLBApiRefT  *pApiRef, const char * name ,  HLBOpCallbackT *cb , void * context)  
{
	return(HLB_ERROR_UNSUPPORTED);  
}

/*F*************************************************************************************************/
/*!
    \Function HLBListUnBlockBuddy

    \Description
     No longer Ignore this buddy.  NOT FOR XENON

    \Input *pApiRef     - API Reference pointer.
    \Input *name        - name
    \Input *cb          - HLBOpCallbackT- Pointer to optional completion callback function
    \Input *context     -  Optional user context that will be passed to callback.  

    \Output
        int32_t         - HLB_ERROR_NONE if command was sent.  

    \Notes
        Client can use callback, or, use polling to determine status of operation,
        as reported later by the server. If this "buddy" was only Blocked, it will now be removed 
        from the local buddy list.
*/
/*********************************************************************************************F*/
int32_t HLBListUnBlockBuddy ( HLBApiRefT  *pApiRef, const char * name, HLBOpCallbackT *cb , void * context ) 
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F*************************************************************************************************/
/*!
    \Function HLBListUnMakeBuddy

    \Description
        Ensure user is no longer a buddy of mine.  NOT FOR XENON

    \Input *pApiRef     - API Reference pointer.
    \Input *name        - buddy name.
    \Input *cb          - HLBOpCallbackT- Pointer to optional completion callback function
    \Input *context     - Optional user context that will be passed to callback.  

    \Output
        int32_t         - HLB_ERROR_NONE if command was sent.  

    \Notes
        Client can use callback, or, use polling to determine status of operation,
        as reported later by the server.
*/
/*********************************************************************************************F*/
 
int32_t HLBListUnMakeBuddy( HLBApiRefT  *pApiRef, const char * name , HLBOpCallbackT *cb , void * context )  
{
    return(HLB_ERROR_UNSUPPORTED);
}

 
 //! Ask this use to be my buddy.  NOT FOR XENON
 int32_t HLBListInviteBuddy( HLBApiRefT  *pApiRef, const char * name , HLBOpCallbackT *cb , void * pContext )
 {
     return(HLB_ERROR_UNSUPPORTED);
 }

 //! Respond to an invite or change state-  NOT FOR XENON
 int32_t HLBListAnswerInvite  (  HLBApiRefT  *pApiRef, const char * name , 
									/*HLBInviteAction_Enum */int32_t eInviteAction, 
 						 					HLBOpCallbackT *cb , void * pContext )
 {
     return(HLB_ERROR_UNSUPPORTED);
 }

//! Respond to an invite   NOT FOR XENON
int32_t HLBListAnswerGameInvite  (  HLBApiRefT  *pApiRef, const char * name , 
									/*HLBInviteAnswer_Enum */int32_t eInviteAction, 
 						 					HLBOpCallbackT *cb , void * pContext )
{ 	 
	 return(HLB_ERROR_UNSUPPORTED);
}


//! Cancel any game invites I sent - NOT FOR XENON
int32_t HLBListCancelGameInvite  (  HLBApiRefT  *pApiRef, HLBOpCallbackT *cb , void * pContext )
{
    return(HLB_ERROR_UNSUPPORTED);
}

//Invite buddy to game.  NOT FOR XENON

int32_t HLBListGameInviteBuddy( HLBApiRefT  *pApiRef, const char * name , const char * pGameInviteState,
                                        HLBOpCallbackT *cb , void * pContext )

{
	return(HLB_ERROR_UNSUPPORTED);
}



/*F*************************************************************************************************/
/*!
    \Function HLBListFlagTempBuddy

    \Description
        Add as a tourney or other  temp buddy for convenience.  NOT FOR XENON

    \Input *pApiRef     - module reference
    \Input *name        - Buddies name
    \Input buddyType    - the TempBuddyType -- see enum. 

    \Output
        int32_t         - HLB_ERROR_NONE if ok.
*/
/*********************************************************************************************F*/
 
int32_t HLBListFlagTempBuddy (  HLBApiRefT  *pApiRef, const char * name, int32_t buddyType )
{
	return(HLB_ERROR_UNSUPPORTED); 
}

/*F*************************************************************************************************/
/*!
    \Function HLBListUnFlagTempBuddy

    \Description
        Clear the particular temp buddy flag if set.  NOT FOR XENON

    \Input *pApiRef         - module reference
    \Input *name            - Buddies name.
    \Input tempBuddyType    - the TempBuddyType flag to clear (see enum)

    \Output
        int32_t             - HLB_ERROR_NONE if ok.
*/
/*********************************************************************************************F*/
 
int32_t HLBListUnFlagTempBuddy (  HLBApiRefT  *pApiRef , const char * name,  int32_t tempBuddyType  )
{
	return(HLB_ERROR_UNSUPPORTED);
}


/*F*************************************************************************************************/
/*!
    \Function HLBListDeleteTempBuddy

    \Description
       **** NOT FOR XENON ***

    \Input *pApiRef - module reference
    \Input *name    - Buddy name.

    \Output
        int32_t     - Return code-- HLB_ERROR_NONE if ok.  
*/
/*********************************************************************************************F*/
  
 int32_t HLBListDeleteTempBuddy (  HLBApiRefT  *pApiRef, const char * name)
 {
	return(HLB_ERROR_UNSUPPORTED);
 }


//! Sort function to be set if client wants custom sort of buddy list -- However API will have a reasonable default sort.
void HLBListSetSortFunction (HLBApiRefT  * pApiRef, HLBRosterSortFunctionT  * sortCallbackFunction  )    
{
	pApiRef->pBuddySortFunc = sortCallbackFunction;
}

//! Sort list of buddies. Once sorted, list will be frozen until re-sorted, or provide a freeze() call.
int32_t  HLBListSort (HLBApiRefT  * pApiRef)                          //sort buddy  list using sort function.
{
    
	if ( pApiRef->pDisp != NULL)
	{
		DispListSort(pApiRef->pDisp ,  pApiRef , 0,  pApiRef->pBuddySortFunc);
		// Force the list to appear to be changed so it will get resorted.
		DispListChange(pApiRef->pDisp , 1);
		// Resort the list.
		DispListOrder(pApiRef->pDisp);
	}
	return(BUDDY_ERROR_NONE);
}


//! Has the  list of  buddies  or their presence changed? Will clear the flag. Local.
int32_t  HLBListChanged(HLBApiRefT  * pApiRef)
{
   
	int32_t rc = HLB_NO_CHANGES;
	if (  pApiRef->rosterDirty )
	{									
        DispListOrder( pApiRef->pDisp);
		pApiRef->rosterDirty = false;
		rc = HLB_CHANGED;
	}
	return(rc);
}

//! Has messages from any buddies changed? NOT FOR XENON
int32_t HLBListBuddyWithMsg(HLBApiRefT  * pApiRef)  
{
	return(HLB_ERROR_UNSUPPORTED);
}


//! On closing screen, need to clear temp buddies w/o messages.  NOT FOR XENON
int32_t HLBListDeleteTempBuddies (HLBApiRefT  * pApiRef )   //todo --implement if needed.
{
	return(HLB_ERROR_UNSUPPORTED);
}

// Can create a list of buddies, to send a message to - NOT FOR XENON

//! Add user to group to whom i will send a common msg.
int32_t HLBListAddToGroup ( HLBApiRefT  * pApiRef, const char * buddyName) 
{
	return(HLB_ERROR_UNSUPPORTED);
}

//! Remove this buddy from the group Iam going to send messages to.  NOT FOR XENON
int32_t HLBListRemoveFromGroup ( HLBApiRefT  * pApiRef, const char * buddyName)
{
	return(HLB_ERROR_UNSUPPORTED);
}

//! Clear group of users so I can re-use it.  NOT FOR XENON
int32_t  HLBListClearGroup ( HLBApiRefT  * pApiRef ) 
{
	return(HLB_ERROR_UNSUPPORTED);
}

//! Send message to group of users I created earlier.  NOT FOR XENON
int32_t HLBListSendMsgToGroup (HLBApiRefT  * pApiRef,   /*HLBMsgType_Enum*/ int32_t msgType , const char * msg , int32_t bClearGroup,
										HLBOpCallbackT *cb , void * pContext)  
{
	return(HLB_ERROR_UNSUPPORTED);
}

//! Set presence for other products, in appropriate language.  NOT FOR XENON
void HLBApiPresenceDiff (  HLBApiRefT  * pApiRef, const char * globalPres)
{
	return;  //HLB_ERROR_UNSUPPORTED
}

//! Set presence my presence state and presence string for this product NOT FOR XENON
void HLBApiPresenceSame (  HLBApiRefT  * pApiRef, int32_t iPresState,const char * strSamePres)
{
	return;  //HLB_ERROR_UNSUPPORTED
}
/*F*************************************************************************************************/
/*!
    \Function HLBApiPresenceJoinable

    \Description
       **** NOT FOR XENON ***

    \Input pApiRef          - Pointer to this api.
    \Input bOn              - 1 if want joinable on, 0 if want it off.
    \Input bSendImmediate   - 1 will send it now, 0 if client will call HLBApiPresenceSend later

    \Output
        int32_t             - HLB_ERROR_NONE if ok, HLB_ERROR_OFFLINE  in offline.  
*/
/*********************************************************************************************F*/
  

int32_t HLBApiPresenceJoinable ( HLBApiRefT  * pApiRef, int32_t bOn , int32_t bSendImmediate)
{
    return(HLB_ERROR_UNSUPPORTED);
}


//set "extra"  presence for scores etc.  NOT FOR XENON
void HLBApiPresenceExtra (  HLBApiRefT  * pApiRef, const char * strExtraPres)
{
    return;
}

//!Set presence as "Offline" if OnOff is true.  NOT FOR XENON
int32_t  HLBApiPresenceOffline (  HLBApiRefT  * pApiRef, int32_t bOnOff)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiPresenceSend

    \Description
        Send presence msg.  NOT FOR XENON

    \Input *pApiRef      - The pointer to this API.
    \Input iBuddyState   - The chat state of this buddy. See HLBStatE.
    \Input iVOIPState    - The headset status to send. Set to -1 if dont want to change it. VOIPState may be changed independantly -- see HLBApiPresenceVOIPSend.
    \Input gamePres      - the presence string for other in this (same) game to see.
    \Input waitMsecs     - time to wait before sending, in msecs. 0 means send now.

    \Output
        int32_t         - Return code-- HLB_ERROR_NONE if ok, HLB_ERROR_OFFLINE  regular presence not yet set.
*/
/*********************************************************************************************F*/
int32_t HLBApiPresenceSend ( HLBApiRefT  * pApiRef,  /*HLBStatE*/ int32_t iBuddyState , int32_t iVOIPState, const char * gamePres,   int32_t waitMsecs) 
{
    return(HLB_ERROR_UNSUPPORTED);
}
 
/*F*************************************************************************************************/
/*!
    \Function HLBApiPresenceVOIPSend

    \Description
        **** NOT FOR XENON ***

    \Input *pApiRef     - Pointer to this api.
    \Input iVOIPState   - The headset status to send.

    \Output
        int32_t         - HLB_ERROR_NONE if ok, HLB_ERROR_OFFLINE  in offline.
*/
/*********************************************************************************************F*/
int32_t  HLBApiPresenceVOIPSend ( HLBApiRefT  * pApiRef, /*HLBVOIPE*/ int32_t iVOIPState )
{
    return(HLB_ERROR_UNSUPPORTED);
}


/*F*************************************************************************************************/
/*!
    \Function HLBApiPresenceSendSetPresence

    \Description
        **** NOT FOR XENON ***

    \Input pApiRef  - Pointer to this api.

    \Output
        int32_t     - HLB_ERROR_NONE if ok, HLB_ERROR_OFFLINE  in offline.  
*/
/*********************************************************************************************F*/
  
int32_t HLBApiPresenceSendSetPresence ( HLBApiRefT  * pApiRef)
{
	return(HLB_ERROR_UNSUPPORTED);
}

//! Send an actual chat message.
int32_t HLBListSendChatMsg(  HLBApiRefT  * pApiRef, const char * name, const char * msg,
									HLBOpCallbackT *cb , void * pContext )
{
    return(HLB_ERROR_UNSUPPORTED);
}
 

/*F*************************************************************************************************/
/*!
    \Function HLBApiSetEmailForwarding

    \Description
      Turn email forwarding on or off **** NOT FOR XENON ***

    \Input *pApiRef     - Ptr to API State.  
    \Input bEnable      - Turn forwarding on or off . 1 is on.
    \Input *strEmail    - The email address to set it to. Will be set even if bEnable is off, due to server issue. 
    \Input *pOpCallback - Optional, callback to get status of operation. Can also poll LastOpStatus.
    \Input *pCalldata   - Optional, user context for callback.

    \Output
        int32_t             - HLB_ERROR_NONE if ok, HLB_ERROR_PEND if some other operation is pending.  
*/
/*********************************************************************************************F*/
 
int32_t HLBApiSetEmailForwarding( HLBApiRefT  *pApiRef,  int32_t bEnable, const char *strEmail, 
															 HLBOpCallbackT *pOpCallback, void *pCalldata )
{
	return(HLB_ERROR_UNSUPPORTED);
}
 

/*F*************************************************************************************************/
/*!
    \Function HLBApiGetEmailForwarding

    \Description
        Get  email forwarding state, **** NOT FOR XENON ***

    \Input *pApiRef     - Ptr to API State.  
    \Input *bEnable     - Returned enable flag. Ignore if return code is "pending".
    \Input *strEmail    - The email address.

    \Output 
        int32_t         - HLB_ERROR_NONE if ok, HLB_ERROR_PEND if not loaded yet.
*/
/*********************************************************************************************F*/
int32_t HLBApiGetEmailForwarding( HLBApiRefT  * pApiRef, int32_t *bEnable, char * strEmail )
{
	return(HLB_ERROR_UNSUPPORTED);
}

//Get the state of Cross Game invites --Xbox only! Will return 0 for PS2.
uint32_t HLBBudGetGameInviteFlags( HLBBudT  *pBuddy  ) 
{
    return((uint32_t)HLB_ERROR_UNSUPPORTED);
}

//! update friend in  internal  "roster" disp list -- add if not there etc and return new buddy.
HLBBudT *_updateFriendInRoster( HLBApiRefT  * pApiRef, PXONLINE_FRIEND pFriend)
{
    char strTemp[32];
    PXONLINE_FRIEND pBuddy = NULL;
    DispListT *pRec;

    // extract name and look for matching record
    strnzcpy( strTemp, pFriend->szGamertag, sizeof(strTemp) );
    // find user record
    pRec = HashStrFind(pApiRef->pHash, strTemp);
    if (pRec == NULL)
    {
        // new one, so alloc and add it
        HLBPrintf(( pApiRef, "HLBApi::New, adding bud %s ", strTemp));
        pBuddy = pApiRef->pBuddyAlloc(sizeof(*pBuddy), HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
        memset(pBuddy, 0, sizeof(*pBuddy));
        memcpy(pBuddy,pFriend,sizeof(*pBuddy));
        pRec = DispListAdd(pApiRef->pDisp, pBuddy);
        HashStrAdd(pApiRef->pHash, pBuddy->szGamertag, pRec);
    }
    // update existing or new buddy  record
    else if ((pRec != NULL) && ((pBuddy = DispListGet(pApiRef->pDisp, pRec)) != NULL))
    {
        HLBPrintf(( pApiRef, "HLBApi::Updated bud %s ", pBuddy->szGamertag));
        memcpy(pBuddy,pFriend,sizeof(*pBuddy));
    }
    return((HLBBudT*)(pBuddy)); //Convert pFriend to pBuddy -- they are the same --
}

//Given a new enum list from XnFriends, update the internal roster..
void _reloadFriends (HLBApiRefT *pApiRef)
{
    int32_t index = 0;
    int32_t totalBud = 0;
    PXONLINE_FRIEND pFriend = XnFriendsGetFriend(pApiRef->pLLRef, index);
    PXONLINE_FRIEND pBud;   
    DispListT *pRec;
	while (pFriend !=  NULL)
    {
        _updateFriendInRoster(pApiRef, pFriend);
        pFriend = XnFriendsGetFriend(pApiRef->pLLRef, ++index);
    }
    // now, go backwards and zap any buddy that is missing..
    totalBud = DispListCount (pApiRef->pDisp)-1;
    for (index=totalBud; index >= 0; index--)
    {
        pBud = (PXONLINE_FRIEND) HLBListGetBuddyByIndex(pApiRef,index);
        if (( pBud) && ( XnFriendsGetFriendByName (pApiRef->pLLRef, pBud->szGamertag) == NULL))
        {// not in new friends enum, so zap it..
            HLBPrintf(( pApiRef, "HLBApi::Zap bud %s ", pBud->szGamertag));
            pRec = HashStrFind(pApiRef->pHash, pBud->szGamertag);
            HashStrDel(pApiRef->pHash, pBud->szGamertag);
			DispListDel(pApiRef->pDisp, pRec);
            pApiRef->pBuddyFree(pBud, BUDDYAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
        }
    }
}

// Set the enumerator index for the XFriendsCreateEnumerator function
int32_t HLBApiSetUserIndex(HLBApiRefT *pApiRef, int32_t iIndex)
{
    return(XnFriendsSetUserIndex(pApiRef->pLLRef, iIndex));
}

//Get the current index used by XFriendsCreateEnumerator
int32_t HLBApiGetUserIndex(HLBApiRefT *pApiRef)
{
    return(XnFriendsGetUserIndex(pApiRef->pLLRef));
}

//!Get the " Official" Xbox title name for a given title. XBOX ONLY. pLang ignored. 
int32_t HLBApiGetTitleName (HLBApiRefT  * pApiRef, const uint32_t  uTitleId, const int32_t iBufSize, char * pTitleName)
{
    return(HLB_ERROR_UNSUPPORTED); //NO XDK for this??
 
}

//!Get the " Official" Title name for this title. pLang ignored -- defaults to English. 
int32_t HLBApiGetMyTitleName (HLBApiRefT  * pApiRef, const char * pLang, const int32_t iBufSize, char * pTitleName)
{
    return(HLB_ERROR_UNSUPPORTED); //NO XDK for this??
 
}

// Set the SessionID that is passed to the game invitation **** NOT FOR XENON ***
int32_t HLBListSetGameInviteSessionID(HLBApiRefT *pApiRef, const char *sSessionID)
{
    return(HLB_ERROR_UNSUPPORTED);
}

// Get the SessionID for a particular user for game invites 
int32_t HLBListGetGameSessionID(HLBApiRefT *pApiRef, const char * name, char *sSessionID, const int32_t iBufSize)
{
    return(XnFriendsGetUserSessionID(pApiRef->pLLRef, name, sSessionID, iBufSize));
}

