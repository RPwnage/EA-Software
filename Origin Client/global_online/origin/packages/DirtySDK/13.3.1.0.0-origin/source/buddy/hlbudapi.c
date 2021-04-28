/*H*************************************************************************************************/
/*!
    \File hlbudapi.c

    \Description
        This module implements High Level Buddy, an application level interface 
        to the Buddy server.

    \Copyright
        Copyright (c) Electronic Arts 2003.  ALL RIGHTS RESERVED.

    \Notes
        Depends on and encapsulates, the lower level dirty sock buddy api. 
        (buddyapi.h and .c modules). Clients should ONLY use hlbudapi.c and NOT buddyapi.c
*/
/*************************************************************************************************H*/

/*** Include files *********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/util/displist.h"
#include "DirtySDK/util/tagfield.h"
#include "DirtySDK/dirtylang.h"
#include "DirtySDK/buddy/hlbudapi.h"

#define HLBUDAPI_XBFRIENDS 0

/*** Defines ***************************************************************************/

#define true 1
#define false 0

#define FUTURE_TS 0xFFFFFFFF
#define ADMIN_USER "ea_admin"

#define MakeMultiCharacterCode(a,b,c,d) ( (((a)&0xff)<<24) | (((b)&0xff)<<16) | (((c)&0xff)<<8) | (((d)&0xff)<<0) )


#define BUDDYAPI_STATUS_SIZE    (signed)(sizeof(_BuddyApi_Status)/sizeof(_BuddyApi_Status[0]))

#define BUDDY_CODE_SUCCESS  (0)
#define BUDDY_CODE_AUTH     MakeMultiCharacterCode('a','u','t','h')
#define BUDDY_CODE_MISS     MakeMultiCharacterCode('m','i','s','s')
#define BUDDY_CODE_USER     MakeMultiCharacterCode('u','s','e','r')
#define BUDDY_CODE_HACK     MakeMultiCharacterCode('h','a','c','k')
#define BUDDY_CODE_RSRC     MakeMultiCharacterCode('m','s','r','c')
#define BUDDY_CODE_NETW     MakeMultiCharacterCode('n','e','t','w')
#define BUDDY_CODE_BSOD     MakeMultiCharacterCode('b','s','o','d')
#define BUDDY_CODE_FULL     MakeMultiCharacterCode('f','u','l','l')

/*** Macros ****************************************************************************/

//To compile out debug print strings in Production release. 
#if DIRTYCODE_LOGGING
#define HLBPrintf(x) _HLBPrintfCode x
#else
#define HLBPrintf(x)
#endif

/*** Type Definitions ******************************************************************/

#define HLB_MSG_READ_FLAG 1     // uUserFlag in msg -- bit 0 indicates read or not.
#define HLB_MSG_OWN_FLAG  2   // uUserFlag in msg -- bit 1 indicates own msg.
#define MAX_BRDC_USERS_LEN 500 //field for formatting list of users in Group msg.

//! current module state
struct HLBApiRefT
{
    BuddyMemAllocT *pBuddyAlloc;    //!< buddy memory allocation function.
    BuddyMemFreeT *pBuddyFree;      //!< buddy memory free function.
    BuddyApiRefT *pLLRef;           //low level api.

    // dirtymem memory group used for "front-end" allocationsmodule memory group
    int32_t iMemGroup;              //!< module mem group id (front-end alloc)
    void *pMemGroupUserData;        //!< user data associated with mem group
    
    // dirtymem memory group used for "in-game" allocationsmodule memory group
    int32_t iRefMemGroup;           //!< ref module mem group id (in-game alloc)
    void *pRefMemGroupUserData;     //!< user data associated with ref mem group
    
    int32_t iStat;                  //!< connection status
    int32_t iLastOpState ;
    int32_t iPresBuddyState;        //! <current buddy state for Presence -- offline, in game etc.
    
    void *pMesgData;                //! client context data for message callback
    HLBMsgCallbackT *pMesgProc;     //! message callback function

    void *pListData;                //! client context data for buddy list callback
    HLBOpCallbackT  *pListProc;     //! buddy list  or presence change callback function  

    void *pOpData;                  //!< client context  data for Operation callback
    HLBOpCallbackT *pOpProc; 
    void *pConnectData;             //!< client context  data for Connect callback
    HLBOpCallbackT *pConnectProc; 
    int32_t connectState;
    int32_t rosterDirty;            //!< had buddy list or presence been updated?
    int32_t (*pBuddySortFunc) (void *pSortref, int32_t iSortcon, void *pRecptr1, void *pRecptr2);
    void (*pDebugProc)(void *pData, const char *pText); //client provide debug for printing errors.
    void * pUserRef;                //!< for any user data ..
    int32_t msgCount;
    uint32_t uTick ;                //!< current game tick, updated from lower level.
    uint32_t uBudCounter;           //!< counter for temp buds, to track oldest.

    HLBBudActionCallbackT *pGameInviteProc; //!< c/b for when game invites come in or sent.
    void *pGameInviteData; 
    HLBPresenceCallbackT *pBuddyPresProc; //!< c/b for when presence changes.
    void *pBuddyPresData;           //!< context data for buddy presence callback.
    uint32_t uGIAcceptTimeout;      //!< wait for accept of Game invite
    char strGIAcceptBuddy[32];      //!< buddy waiting for accept.
    uint32_t uGISessionId;          //!< counter for game invite session, for ps2.

    //configurable constants ..
    int32_t max_perm_buddies ;      //!< max number of permanent buddies.
    int32_t max_temp_buddies;       //!< max number of temporary buddies.
    int32_t max_block_buddies;      //!< max number of blocked buddies.
    int32_t max_msgs_allowed;       //!< total msgs allowed...//msgs_per_temp_buddy;    //!< each temp bud is allowed this many msgs, max.
    int32_t max_msgs_per_perm_buddy;    //!< each real bud is allowed this many msgs, max.
    int32_t max_users_found;        //!< max number users returned on a wildcard hit on "find user".
    int32_t max_msg_len;   
    int32_t connect_timeout;
    int32_t op_timeout;
    int32_t iGIAccept_timeout_param;//!< how long to wait-after buddy accepted invite.
    int32_t bMakeOffline;           //!< Has user asked to be offline --wont send presence updates
    int32_t bTrackSendMsgInOpState; //!< Status SendMsg to be tracked in LastOpState.
    Utf8TransTblT *pTransTbl;       //!< utf8 optional transtable for utf8 to 8bit.

    int32_t eFakeAsyncOp;           //!< for Async ops that became sync, track the Op.
    int32_t iFakeAsyncOpCode;       //!< for Async ops that became sync, track the code.
    DispListRef *pPreConnTempBudList; //!< list of buddies added before connected.
    DispListRef *pPreConnMsgList;   //!< list  injected msgs, added before connected.
    
    int32_t bDontUseXBLSameTitleIdCheck; //!< For Xbl, tell to not use XOnlineIsSameTitle for IsSameProduct.
    int32_t bRosterLoaded;          //!< Have we re-loaded the buddy list?
    uint32_t   uLocale;             //!< country&lang for getting Title--Actually, only care about lang.
};

typedef struct preConnTempBudT {
    char        strName [BUDDY_NAME_LEN+1]; 
    uint32_t    uTempBuddyTypesFlag;
}preConnTempBudT;

typedef struct preConnMsgT {
    char             strName [BUDDY_NAME_LEN+1]; 
    BuddyApiMsgT     msg;  
}preConnMsgT;


//! Defines my buddy --private, use HLBBud* accessors. todo move this. 
struct HLBBudT
{
    BuddyRosterT *pLLBud;       //!< private to lower level
    int16_t msgListDirty;       //!< any unread msgs?
    int16_t groupId;            //!< private, temporal for msgs.
    int16_t msgCount;           //!< total number of msg received -- from all buddies.
    uint16_t uTempBuddyFlags;   //!< kind of temp buddy types associated?
    void *msgList;              //!< private list of messages - 
    HLBApiRefT *pHLBApi;        //!< some "HLBBudxxx" calls need access to parent ref. api.
};


typedef struct ErrCodeMap_t
{
    int32_t iCode;              //!< packed-char error code
    int32_t hlbCode;            //!< corresponding textual code
} ErrCodeMapT;


/*** Function Prototypes ***************************************************************/

static void _HLBMsgListDestroy(HLBBudT *pBuddy ) ;
static void _HLBRosterCallback(BuddyApiRefT *pLLRef, BuddyApiMsgT *pMsg, void *pUserData);
static int32_t  _HLBMsgListAddMsg(HLBApiRefT *pApiRef, const char * strUser , BuddyApiMsgT *pMsg, int32_t bNoReply);

BuddyApiRefT * _HLBApiGetXXX (  HLBApiRefT  * pApiRef );

/*** Variables *************************************************************************/

// Private variables



// Public variables

//*** Private Functions *****************************************************************

//! translate message.iType to the op enum.
static int32_t _msgTypeToOp ( int32_t msg )
{
    if ( msg == BUDDY_MSG_SEND)
        return(HLB_OP_SEND_MSG);
    if ( msg == BUDDY_MSG_BROADCAST)
        return(HLB_OP_SEND_MSG);
    if ( msg == BUDDY_MSG_ADDUSER)
        return(HLB_OP_ADD_BUD);
    if ( msg == BUDDY_MSG_DELUSER)
        return(HLB_OP_DELETE_BUD);
    if ( msg == HLB_OP_CONNECT)
        return(HLB_OP_CONNECT);
    if ( msg == BUDDY_MSG_USERS)
        return(HLB_OP_FIND_USERS);
    if ( msg == BUDDY_MSG_SET_FORWARDING)
        return(HLB_OP_SET_FORWARDING);
    if ( msg == BUDDY_MSG_BUD_INVITE)
        return(HLB_OP_INVITE);
    if ( msg == BUDDY_MSG_GAME_INVITE)
        return(HLB_OP_INVITE);
    return(HLB_OP_ANY);
}



#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function HLBPrintfCode

    \Description
        Handle debug output

    \Input *pState  - reference pointer
    \Input *pFormat - format string
    \Input ...      - varargs
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
    ds_snzprintf (finalText, sizeof(finalText), "HLB: %s \n", strText); //todo fixup -- we dont need
    // pass to caller routine if provied
    (pApiRef->pDebugProc)("", finalText);
}
#endif

//! Return the buddy roster disp list--DONT cache, as buddyapi can null it.
static DispListRef * _getRost ( HLBApiRefT *pApiRef )
{
    return( BuddyApiRosterList(pApiRef->pLLRef ));
} 

#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function _verifyBuddyList

    \Description
         debug function to validate buddy list is ok, and that ptrs are set properly.

    \Input *pApiRef - reference pointer
*/
/*************************************************************************************************F*/
//! validate buddy list is ok, and that ptrs are set properly.
static void _verifyBuddyList (HLBApiRefT *pApiRef)
{
    int32_t idx =0, val=0;
    BuddyRosterT *pLLBud;
    HLBBudT *pBud;


        if ( _getRost(pApiRef ) == NULL)
        return;

    for (idx = 0, val = DispListShown(_getRost(pApiRef)); idx < val; ++idx)
    {
        pLLBud = (BuddyRosterT *)DispListIndex( _getRost(pApiRef) , idx);
        if (pLLBud->pUserRef == NULL )
            HLBPrintf( ( pApiRef, "LL dude missing HLB ptr %s", pLLBud->strUser ));
        else
        {
            pBud = ( HLBBudT*) pLLBud->pUserRef;
            if (pBud->pLLBud != pLLBud )
                HLBPrintf( ( pApiRef, "HL bud, has ptr but not to LL bud %s", pLLBud->strUser));
            //else
            //  HLBPrintf( ( pApiRef, " OK for %s :%d:%d %s", pLLBud->strUser, pLLBud->uFlags, pLLBud->iPend,  pLLBud->strPres));
        }
    }
}
#endif

//set last operation state as pending-- if command got initiated.
static void _setLastOpState ( HLBApiRefT *pApiRef, int32_t rc)
{
    if (rc == HLB_ERROR_NONE)
        pApiRef->iLastOpState  = HLB_ERROR_PEND;
    else
        pApiRef->iLastOpState =rc;
    HLBPrintf( ( pApiRef, "Op State is now  %d \n",  pApiRef->iLastOpState));

}

static void _signalBuddyListChanged ( HLBApiRefT *pApiRef )
{
    pApiRef->rosterDirty =true;
    if  ( ( pApiRef ) && (pApiRef->pListProc ))
        pApiRef->pListProc ( pApiRef, HLB_OP_LOAD,HLB_ERROR_NONE, pApiRef->pListData ); 
}

// List wrappers -- until we come up with appropriate list implementation.
 
//create my own msg list.
static void *  _listCreate (HLBApiRefT *pApiRef)
{
    DispListRef  *pDispList;
    DirtyMemGroupEnter(pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
    pDispList= DispListCreate( 5, 5, 0); //start 5, expand 5
    DirtyMemGroupLeave();
    DispListDirty(pDispList, 1); //1 = set it
    return(pDispList);
}

static int32_t _listGetCount  (void * pList )
{
    return(DispListCount(pList));
}

#if 0 // UNUSED
static int32_t _listIsDirty (void * pList )
{
    return(DispListOrder(pList) > 0);
}
#endif

static void _listDestroy (void * pList )
{      
    DispListDestroy( pList);   
}
//caller must allocate  item to add..
static void  * _listItemAdd ( void  * pList,  void *pItem)
//Add a msg to _list.
{
    DispListT *pLRec = NULL;
    if ( pList) 
        pLRec = DispListAdd(pList, pItem);
    //   HashStrAdd(pState->pHash, pOwned->strUser, pRec);
    return(pLRec);
}

static BuddyApiMsgT* _listItemGetByIndex (void * pList, int32_t index )
{
    return(DispListIndex (pList, index));
}

//get an item out of pre-connected temp buddy list (cache of temp buddies while not connected).
static preConnTempBudT* _listItemGetPreConnTempBuddyByIndex(void * pList, int32_t index)
{
    return(DispListIndex(pList, index));
}

//remove item out of pre-connected temp buddy list 
static preConnTempBudT* _listRemovePreConnTempBuddy(void * pList, int32_t index )
{
    // Remove the  indexed element from the list
    return(DispListDelByIndex(pList, index));
}

//delete the list container, and the item pointed to at index.
static void _listMsgItemDestroy (HLBApiRefT  * pApiRef, void * pList, int32_t index )
{
    BuddyApiMsgT * pMsg;

    if (index <0 )
        return; //blow up good, if not.
    pMsg = DispListDelByIndex (pList , index); //deletes container, and ret data ptr.
    if (pMsg )
    {
       //use pBuddyFree here, since Buddy allocated this msg. 
        pApiRef->pBuddyFree((void*)pMsg->pData, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);  
        pApiRef->pBuddyFree(pMsg, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);   
    }
}

//!clone and existing message, allocating memory for it.
static BuddyApiMsgT *_cloneMsg(HLBApiRefT *pApiRef, const BuddyApiMsgT *pMsg)
{
    BuddyApiMsgT *pClonedMsg = NULL;
    int32_t iDataSize = 0;
    //iCount = _listGetCount(pApiRef->pPreConnMsgList);
    //message has TWO parts -- alloc and copy both..
    pClonedMsg = pApiRef->pBuddyAlloc(sizeof (*pClonedMsg ), HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
    if (pClonedMsg == NULL)
    {
        return(NULL);
    }
    memcpy(pClonedMsg,pMsg,sizeof(*pClonedMsg));
    iDataSize = (int32_t)strlen(pMsg->pData)+1;
    pClonedMsg->pData = pApiRef->pBuddyAlloc(iDataSize, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
    if (pClonedMsg->pData == NULL)
    {
        pApiRef->pBuddyFree(pClonedMsg, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData); //free whats already allocated.
        return(NULL);
    }
    ds_strnzcpy((char*)pClonedMsg->pData, pMsg->pData, iDataSize);
    return(pClonedMsg);
}

//! return count of low level roster list.
static int32_t _rosterCount (HLBApiRefT *pApiRef)
{
    if  ( _getRost(pApiRef) == NULL)
        return(0);

    return(DispListCount( _getRost(pApiRef)));
}
 
static  int32_t _countBuddies ( HLBApiRefT *pApiRef, uint32_t flag)
 {
    int32_t idx=0, val =0, count=0;
    HLBBudT *pBuddy;    
    for (idx = 0, val=_rosterCount(pApiRef);  idx < val; ++idx)
    {
        pBuddy = HLBListGetBuddyByIndex (  pApiRef, idx );\
        if (pBuddy != NULL )
        {
            if( ( pBuddy->pLLBud ) &&(( pBuddy->pLLBud->uFlags & flag) >0 ) )//if any bit is set.
                count++;
        }
    }
    return(count);
}


//! Free buddy memory and contained structures like message lists.
static void _HLBDestroyBuddy ( HLBBudT *pBud)
{
    if (pBud  == NULL)
        return;
    _HLBMsgListDestroy ( pBud ); //zap any messages..
    if (pBud->pLLBud)
    {
        pBud->pLLBud->pUserRef = NULL; //my backptr is me -- so clear it.
    }
    pBud->pHLBApi->pBuddyFree(pBud, HLBUDAPI_MEMID, pBud->pHLBApi->iMemGroup, pBud->pHLBApi->pMemGroupUserData); //free this bud.
}

static void _HLBRosterDestroy( HLBApiRefT  * pApiRef )
{
    //free my mem structures  by iterating thru buddies and their messages ..
    int32_t count; //, val =0;
    HLBBudT *pBuddy;

    if ( pApiRef == NULL )
        return;

    //todo callbacks outstanding on flush? ...

    //backwards, just in case  index does  change while zapping..
    for (count = HLBListGetBuddyCount ( pApiRef )-1; count >= 0; --count)
    {
        pBuddy = HLBListGetBuddyByIndex (  pApiRef, count );
        _HLBDestroyBuddy ( pBuddy);
    }

    // Flush LL buddy memory structures
    BuddyApiFlush(pApiRef->pLLRef); 
    //pApiRef->pRoster  = NULL; //roster is no good after ll flush.
    pApiRef->msgCount =0;
}

//! Destroy the "pre connected temp buddy" list, and its contents.
static void _preConnTempBudListDestroy ( HLBApiRefT *pApiRef )
{
    int32_t count;
    void * pItem;
    if ( ! ((pApiRef )&&(pApiRef->pPreConnTempBudList)) )
        return;

    for (count = _listGetCount( pApiRef->pPreConnTempBudList)-1; count >= 0; --count)
    {
        pItem = DispListDelByIndex (pApiRef->pPreConnTempBudList,count);  
        if  (pItem)
        {
            pApiRef->pBuddyFree(pItem, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);    
        }
    }
    //now, the container.
    _listDestroy ( pApiRef->pPreConnTempBudList);
     pApiRef->pPreConnTempBudList = NULL;
}

//! Destroy the "pre connected msg" list, and its contents.
#if 0 // UNUSED
static void _preConnMsgListDestroy ( HLBApiRefT *pApiRef )
{
    int32_t count;
    void * pItem;
    if ( ! ((pApiRef )&&(pApiRef->pPreConnMsgList)) )
        return;

    for (count = _listGetCount( pApiRef->pPreConnMsgList)-1; count >= 0; --count)
    {
        pItem = DispListDelByIndex (pApiRef->pPreConnMsgList,count);  
        if  (pItem)     
        {
                pApiRef->pBuddyFree(pItem, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);   
        }
    }
    //now, the container.
    _listDestroy ( pApiRef->pPreConnMsgList);
     pApiRef->pPreConnMsgList = NULL;
}
#endif

//! Add a message to our internal "pre connected" list, prior to connecting.. Cached for later.
static int32_t _preConnMsgAdd (HLBApiRefT *pApiRef, const char * pName, BuddyApiMsgT *pMsg)
{
    BuddyApiMsgT  *pSaveMsg;
    if (pApiRef == 0)
    {
        return(HLB_ERROR_BADPARM);
    }
    if ( pApiRef->pPreConnMsgList==NULL)
    {
        pApiRef->pPreConnMsgList = _listCreate(pApiRef);
    }

    if ((pSaveMsg = _cloneMsg(pApiRef, pMsg)) == NULL)
    {
        return(HLB_ERROR_MEMORY);
    }
    _listItemAdd( pApiRef->pPreConnMsgList, pSaveMsg);
    return(HLB_ERROR_NONE);
}


//! Add a temp buddy to our internal "pre connected" list. Cache for later.
static void _preConnTempBudAdd (HLBApiRefT *pApiRef, const char * pName, int32_t eTempBudType)
{
    int32_t iCount;
    int32_t iFound;
    int32_t iIndex;
    preConnTempBudT  *pPCTBud;

    if (pApiRef == 0)
         return;

    if ( pApiRef->pPreConnTempBudList==NULL)
    {
        pApiRef->pPreConnTempBudList = _listCreate(pApiRef);
    }

    // Find the buddy record that matches us
    pPCTBud = NULL;
   
    iCount = _listGetCount(pApiRef->pPreConnTempBudList);
    iFound = iCount;

    for(iIndex = 0; iIndex < iCount; iIndex++)
    {
        // Get the current record
        pPCTBud = _listItemGetPreConnTempBuddyByIndex(pApiRef->pPreConnTempBudList, iIndex);

        // Compare it to the temporary buddy that we are looking for
        if (0 == (strcmp(pPCTBud->strName, pName)))
        {
            // We found it!
            iFound = iIndex;
            break;
        }
    }

    if (iFound == iCount) // Not found
    {
        if (( pPCTBud = pApiRef->pBuddyAlloc(sizeof(*pPCTBud), HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData)) == NULL)
        {
            return; //out of memory..  HLB_ERROR_MEMORY
        }
        memset(pPCTBud,0,sizeof(*pPCTBud));
        ds_strnzcpy(pPCTBud->strName, pName, sizeof(pPCTBud->strName));
    }
    else // We found the buddy in the list already
    {
        // Remove it from the list so it will be on the list once
        // We do this so that it is a "new entry" and won't get
        // removed prematurely.
        _listRemovePreConnTempBuddy(pApiRef->pPreConnTempBudList, iFound);
    }

    // Set the buddy type and add the buddy to the list.
    pPCTBud->uTempBuddyTypesFlag |= eTempBudType;
    _listItemAdd( pApiRef->pPreConnTempBudList, pPCTBud);

    // Trim buddy list to HLB_MAX_TEMP_BUDDIES instead of letting it grow until it
    // consumes all the available memory.
    while (_listGetCount(pApiRef->pPreConnTempBudList) >= HLB_MAX_TEMP_BUDDIES)
    {
        pPCTBud = _listRemovePreConnTempBuddy(pApiRef->pPreConnTempBudList, 0);
        pApiRef->pBuddyFree(pPCTBud, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
    }
}

//! Load a set of temp buddies that were saved while offline. Clear the list cache.
static void _preConnTempBudListLoad ( HLBApiRefT *pApiRef )
{
    int32_t count;
    preConnTempBudT* pItem;
    uint32_t uTempBudType;
    if ( ! ((pApiRef )&&(pApiRef->pPreConnTempBudList)) )
        return;

    for (count = _listGetCount( pApiRef->pPreConnTempBudList)-1; count >= 0; --count)
    {
        pItem = DispListIndex ( pApiRef->pPreConnTempBudList , count);  
        if  (pItem)     
        {
            for ( uTempBudType = 1; uTempBudType < HLB_TEMPBUDDYTYPE_MAX ; uTempBudType *= 2)
            {  //check for all possible temp buddy type flags, and set
                if ( ( pItem->uTempBuddyTypesFlag & uTempBudType ) > 0)
                { //add to the set of flags for temp bud.
                    HLBListFlagTempBuddy( pApiRef, pItem->strName,uTempBudType);
                }
            }
            
            DispListDelByIndex(pApiRef->pPreConnTempBudList,count);
            pApiRef->pBuddyFree(pItem, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
        }
    }
    //zap the list too, as, we DONT want any lobby alloc mem around when in game.
    _preConnTempBudListDestroy (pApiRef);
}

//! Load injected messages that were saved while offline. Clear the list cache.
static void _preConnMsgListLoad ( HLBApiRefT *pApiRef )
{
    int32_t count =0;
    int32_t i = 0;
    BuddyApiMsgT* pItem;
    char strUser[HLB_BUDDY_NAME_LEN];
    if ( ! ((pApiRef )&&(pApiRef->pPreConnMsgList)) )
        return;

    count = _listGetCount( pApiRef->pPreConnMsgList);
    //go forwards and add msg --preserve order as temp bud only get one (ie last) msg kept
    for (i = 0; i <count; i++)  
    {
        pItem = DispListIndex(pApiRef->pPreConnMsgList , i);
        if  (pItem)     
        {
            TagFieldGetString(TagFieldFind(pItem->pData, "USER"), strUser, sizeof(strUser), "");
            _HLBMsgListAddMsg(pApiRef, strUser,pItem, (strcmp(strUser,ADMIN_USER) ==0));  
        }
    }
    // now, go backwards, and destroy all.
    for (count = _listGetCount( pApiRef->pPreConnMsgList)-1; count >= 0; --count)
    {
        pItem = DispListIndex(pApiRef->pPreConnMsgList , count);  
        if  (pItem)
        {
            DispListDelByIndex(pApiRef->pPreConnMsgList,count);
            pApiRef->pBuddyFree((char*)pItem->pData, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData); //msg has msg AND data --free both
            pApiRef->pBuddyFree(pItem, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData);
        }
    }

    //done, so zap the list too, as, we DONT want any lobby alloc mem around when in game.
    //now, the container.
    _listDestroy (pApiRef->pPreConnMsgList);
     pApiRef->pPreConnMsgList = NULL;
}

static void _HLBConnectCallback(BuddyApiRefT *pRef, BuddyApiMsgT *pMsg, void *pUserData) // 
{
    HLBApiRefT *pHLBRef = ( HLBApiRefT *)(pUserData);

//hack for offline testing: pMsg->iType= BUDDY_MSG_AUTHORIZE; pMsg->iCode = BUDDY_CODE_SUCCESS;

    switch(pMsg->iType)
    {
    case BUDDY_MSG_CONNECT:
        switch (pMsg->iKind)
        {
        case BUDDY_SERV_IDLE:       //!< idle state (no connection)
            //pHLBRef->connectState =  HLB_CS_Connecting;
            break;

        case BUDDY_SERV_CONN:       //!< connecting to server
            pHLBRef->connectState =  HLB_CS_Connecting;
            break;

        case BUDDY_SERV_ONLN:       //!< server is online
            //pHLBRef->connectState =  HLB_CS_Connecting; //todo this correct ?
            break;

        case BUDDY_SERV_TIME:       //!< server timeout
            pHLBRef->connectState =  HLB_CS_ErrorTimeout;
            break;

        case BUDDY_SERV_NAME:       //!< server name lookup failure
        case BUDDY_SERV_FAIL:       //!< misc connection failure
            pHLBRef ->connectState = HLB_CS_ErrorNetwork;
            break;

        case BUDDY_SERV_DISC:       //!< server has disconnected
            // If we were connected treat this as a disconnection,
            // otherwise it is a failed connection attempt.
            if ( pHLBRef->connectState == HLB_CS_Connected)
            {
                pHLBRef->connectState = HLB_CS_Disconnected;
                HLBPrintf( (pHLBRef, "**** *** ***** The server has disconnected \n"));
            }
            else //leave prev state the same, as get disconnect after auth error etc.  
            {
                //pHLBRef->connectState = HLB_CS_ErrorServer;
            }
            break;

        default:
            HLBPrintf((pHLBRef, "ConnectionCallback - Unknown iKind %d\n", pMsg->iKind));
            pHLBRef ->connectState = HLB_CS_ErrorServer;
        };

        break;

    case BUDDY_MSG_AUTHORIZE:
        switch (pMsg->iCode)
        {
        case BUDDY_CODE_SUCCESS:
            if ( pHLBRef->connectState == HLB_CS_Connecting )
            {
                // Mark as online
                HLBPrintf((pHLBRef, "ConnectionCallback --Connected\n"));

                pHLBRef->connectState = HLB_CS_Connected;
            }
            break;

        case BUDDY_CODE_AUTH:
            pHLBRef->connectState = HLB_CS_ErrorAuth;
            break;

        case BUDDY_CODE_USER:
            pHLBRef->connectState = HLB_CS_ErrorUser ;
            break;

        case BUDDY_CODE_NETW:
            pHLBRef->connectState = HLB_CS_ErrorNetwork ; 
            break;

        case BUDDY_CODE_HACK:
        case BUDDY_CODE_BSOD:
            pHLBRef->connectState = HLB_CS_ErrorServer ; 
            break;

        case BUDDY_CODE_RSRC:
        case BUDDY_CODE_MISS:
            pHLBRef->connectState = HLB_CS_ErrorServer; 
            break;

        default:
            pHLBRef->connectState = HLB_CS_ErrorServer; 
            break;
        }
        break;

    default:
        {
            char message[128];
            ds_snzprintf(message, sizeof(message), "Received unknown response '%c%c%c%c'",
                (char)pMsg->iType>>24,
                (char)pMsg->iType>>16,
                (char)pMsg->iType>>8,
                (char)pMsg->iType);
            HLBPrintf((pHLBRef, message));
        }
        break;

    }

    HLBPrintf((pHLBRef, "***Connect Callback -- connect state now %d" , pHLBRef->connectState));     
    if  (pHLBRef->connectState != HLB_CS_Connecting) 
    {
        if (pHLBRef->connectState != HLB_CS_Connected )
        {
            _HLBRosterDestroy( pHLBRef ); // clear  in case of re-connect..
        }
        else
        {
            pHLBRef->iLastOpState =0; //clear any outstanding stuff...?
            //in case they want to make buddy Offline, DONT send presence here.
            pHLBRef->iPresBuddyState = BUDDY_STAT_CHAT; //client has to send it.

            //only load roster if not passive-in case of server restarts, and we re-connect.
            if ( pHLBRef->iStat != BUDDY_STAT_PASS)
            { 
                pHLBRef->iStat = BUDDY_STAT_CHAT;
                //load the roster..
                pHLBRef->bRosterLoaded = 0;
                BuddyApiRoster(pHLBRef->pLLRef , _HLBRosterCallback, pHLBRef );
                //now finally ready for buds, so and load them
                _preConnTempBudListLoad (pHLBRef); //load any saved temp buds

                //but, wait till blocked list is loaded before processing saved injected msgs ...
            }
        }
        if  (pHLBRef->pConnectProc )
        {
           pHLBRef->pConnectProc ( pHLBRef,   HLB_OP_CONNECT , pHLBRef->connectState  , pHLBRef->pConnectData );
        }
    }
}

static void _HLBOpCallback(BuddyApiRefT *pLLRef, BuddyApiMsgT *pMsg, void *pUserData)
{
    int32_t ignore = false;
    HLBApiRefT  *pApiRef = ( HLBApiRefT * )(pUserData);
    
    switch(pMsg->iType)  
    {
    case BUDDY_MSG_ADDUSER:
    case BUDDY_MSG_DELUSER:
    case BUDDY_MSG_USERS:
    case BUDDY_MSG_SET_FORWARDING:
    case BUDDY_MSG_BUD_INVITE:
    case BUDDY_MSG_GAME_INVITE:
        if (pMsg->iCode !=  BUDDY_ERROR_NONE  )
            HLBPrintf((pApiRef, "Op Add/Del/Find Failed: %s  code=%d " , pMsg->pData,  pMsg->iCode));
        if (pMsg->iCode ==  'full'  )
            pMsg->iCode = HLB_ERROR_FULL;
        break;

    case BUDDY_MSG_SEND:
    case BUDDY_MSG_BROADCAST:
        if (pMsg->iCode !=  BUDDY_ERROR_NONE  )
            HLBPrintf((pApiRef, "Send/Brodcast msg Failed: %s  code=%d " , pMsg->pData,  pMsg->iCode));
        if (pMsg->iCode ==  'full'  )
            pMsg->iCode = HLB_ERROR_FULL;

        //if we are not tracking message responses, please ignore this.
        ignore = (pApiRef->bTrackSendMsgInOpState)?0:1;
        break;

    default:
        HLBPrintf((pApiRef, " ERROR: ignoring unexpected op callback message"));
        ignore = true; 
        break;
    }

    if (! ignore)
    {
        //PEND is used internally, to signal operation in progress,
        //and, dont ever want it as final return code, so protect from it.
        pApiRef->iLastOpState=(pMsg->iCode == HLB_ERROR_PEND)? HLB_ERROR_UNKNOWN: pMsg->iCode;

        HLBPrintf( (pApiRef, "*****\n***Got Response msgType:%d, last opState is now %d \n", _msgTypeToOp (pMsg->iType), pApiRef->iLastOpState));
    }

    //Call callers callbacks, if needed.
    if  ( ( pApiRef ) && (pApiRef->pOpProc ))
    {
            pApiRef->pOpProc ( pApiRef, _msgTypeToOp (pMsg->iType) , pMsg->iCode , pApiRef->pOpData );  //todo convert msg to op enum?
            pApiRef->pOpProc = NULL; //clear, as it is per op only..
    }
}

//! build a HLBBud from the lower level buddy;  and set the pointers to each other.
static HLBBudT  * _constructHLBud ( HLBApiRefT *pApiRef,BuddyRosterT *pLLBud)
{
    HLBBudT *pNewBud = NULL;
    if (pLLBud == NULL )
        return(NULL);

    if ( pLLBud->pUserRef )
        return(pLLBud->pUserRef); //already there, dont leak mem.

    if ((pNewBud = pApiRef->pBuddyAlloc(sizeof(*pNewBud), HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData)) == NULL)
    {
        return(NULL); // no bud memory
    }

    //HLBPrintf( ( pApiRef, "Bud size is %d\n", sizeof (*pNewBud)); //XX  TODO remove   
    memset (pNewBud, 0, sizeof (*pNewBud) ); 
    pNewBud->msgList = _listCreate(pApiRef);
    pNewBud->pHLBApi = pApiRef; //pointer to this api, for HLBBud calls..
    pNewBud->pLLBud = pLLBud; //my ptr to Low level bud..
    pLLBud->pUserRef = pNewBud; //set his ptr back to me
    pLLBud->uBudUserVal =  ++(pApiRef->uBudCounter);  //arb counter so can tell oldest temp bud
    //HLBPrintf( ( pApiRef, "_buildBud %s %s \n" , LLBud.strUser, pNewBud->pLLBud->strUser );
    return(pNewBud);
}


//! internal c/b when roster or presence changes. Will dispatch to client code if needed.
static void _HLBRosterCallback(BuddyApiRefT *pLLRef, BuddyApiMsgT *pMsg, void *pUserData)
{
    HLBApiRefT  *pApiRef = ( HLBApiRefT * )(pUserData);

    // The buddy list has changed so we need to flag this.
    int32_t  idx =0, val = 0;
    BuddyRosterT *pLLBud;

    //HLBPrintf( ( pApiRef, "Roster Change callback %d %s" , pMsg->iType, pMsg->pData );

    //On init, we need to fix up our list of buddies, that got added by lower levels ...
    for (idx = 0, val = _rosterCount ( pApiRef);  idx < val; ++idx)
    {
        if ( (pLLBud = (BuddyRosterT*) DispListIndex( _getRost(pApiRef), idx ) ) != NULL )
        {   
            if ( pLLBud->pUserRef ==  NULL ) //case of initial roster loaded, and managed by low level -- add HL buddy.     
                if (_constructHLBud (  pApiRef, pLLBud) == NULL) //create a HLBud out of it, and set back ptr.
                {//out of mem --this is a bad bud, but not much we can do.. 
                    HLBPrintf (( pApiRef, "Cannot alloc HL bud for:[%s] \n", pLLBud->strUser));
                    break;
                }
        }

         
        //   Ready is set from Xdk level, and not thru our msgs.
        if ( ( pLLBud->uFlags & BUDDY_FLAG_GAME_INVITE_READY)   
                            && (pApiRef->pGameInviteProc ) )
                 pApiRef->pGameInviteProc ( pApiRef, pLLBud->pUserRef , 1, pApiRef->pGameInviteData );
        //we dont worry about it clearing -- they go into game -- in theory.

    }

    if (pMsg->iType == BUDDY_MSG_ENDROSTER)
    { // roster loaded,process preconnected messages, as now we know who was blocked and can discard...
        pApiRef->bRosterLoaded = 1; 
        _preConnMsgListLoad (pApiRef);
         return;
    }

    _signalBuddyListChanged (pApiRef);
#if DIRTYCODE_LOGGING
    _verifyBuddyList ( pApiRef);
#endif
}



static BuddyRosterT *_setUpOp ( HLBApiRefT *pApiRef, const char * name, HLBOpCallbackT * cb, void * pContext)
{
    HLBBudT *pBuddy;

    //should disallow this, as callbacks can get messed up.
    if (pApiRef->iLastOpState == HLB_ERROR_PEND )
        HLBPrintf (( pApiRef, "New op started, but, old Op still pending\n"));

    pApiRef->pOpProc =  cb;
    pApiRef->pOpData = pContext;

    if ((pBuddy = HLBListGetBuddyByName (pApiRef, name)) == NULL)
    {
        return(NULL);
    }
    return(pBuddy->pLLBud);
}


//! build a HLBBud from the lower level; retrieve or add if needed, and set the pointers to each other.
static HLBBudT  *_addChangeLLBud ( HLBApiRefT *pApiRef,BuddyRosterT oldLLBud, BuddyRosterT newLLBud)
{
    int32_t rc;
    BuddyRosterT *pExistLLBud = NULL;

    pExistLLBud  = BuddyApiFind(pApiRef->pLLRef, newLLBud.strUser );
    if (( pExistLLBud == NULL ) || ( oldLLBud.uFlags != newLLBud.uFlags) )
    { //not there, or flag dont match,  so "add". Api uses "add" for add or change ...
        //am i adding a perma buddy  to  an existing temp, if so,clear the temp status..
        if  (  ( pExistLLBud) && ( oldLLBud.uFlags & BUDDY_FLAG_TEMP)  &&
                         ( newLLBud.uFlags &(BUDDY_FLAG_IGNORE | BUDDY_FLAG_BUDDY) ) )
        {
             newLLBud.uFlags &= ~(BUDDY_FLAG_TEMP); //clear exist temp from new bud
             //pExistLLBud->uFlags &= ~(BUDDY_FLAG_TEMP); //clear exist temp flag.
             //have to use existing LLbud too, as BuddyApiAdd wont clear flags..
        }
        rc = BuddyApiAdd(pApiRef->pLLRef, &newLLBud ,_HLBOpCallback, pApiRef, pApiRef->op_timeout );
        _setLastOpState (pApiRef, rc);
        if ( rc == HLB_ERROR_NONE )
        {
            if (pExistLLBud == NULL )
            {
                pExistLLBud  = BuddyApiFind(pApiRef->pLLRef, newLLBud.strUser); //need actual ptr to it..
            }
        }
        if (!pExistLLBud)
        {
            return(NULL); //bad -- failure here.
        }
    }
    return(_constructHLBud(pApiRef, pExistLLBud));
}

//! -- internal to phyically remove a buddy, if needed.
static int32_t _HLBListRemoveBuddy (   HLBApiRefT  *pApiRef, const char * name, uint32_t typeFlag, HLBOpCallbackT *cb , void * context)   
{
    int32_t result = HLB_ERROR_NONE;
    int32_t zap = true;
    HLBBudT* pFoundHLB;
    BuddyRosterT buddy;
    char  buddyName [HLB_BUDDY_NAME_LEN];

    if (pApiRef->iStat == BUDDY_STAT_PASS )
        return(HLB_ERROR_USER_STATE);

    if (HLBApiGetConnectState (pApiRef)  != HLB_CS_Connected )
        return(HLB_ERROR_OFFLINE);

    memset(&buddy, 0, sizeof(buddy));
    ds_strnzcpy(buddy.strUser, name, sizeof(buddy.strUser));
    buddy.uFlags = typeFlag;     
    if ( (pFoundHLB  = HLBListGetBuddyByName ( pApiRef, name  ))  != NULL )
    {  //don't remove a user whose flags don't match what we are trying to remove.
        if ( (buddy.uFlags & pFoundHLB->pLLBud->uFlags) == 0)
            zap = false;  
    }
    else // already missing, so dont  bother..
        zap = false;

    if ( zap )
    { //zap it ... or change it, as case may be. API uses "del" to remove a flag, or really delete a buddy.

        if (  pApiRef->iLastOpState == HLB_ERROR_PEND )
            return(HLB_ERROR_PEND);
        //save the clients callback, if any.
        if (cb)
        {
            pApiRef->pOpProc = cb;
            pApiRef->pOpData = context;
        }
        ds_strnzcpy (buddyName, pFoundHLB->pLLBud->strUser, sizeof (buddyName)); //save  name as LL bud can vanish here..
        //Del api is used to actually delete, or simply change a flag...
        //Need to clear the watch flag for a temp buddy, or else, will timeout, and server wont know.
        if (typeFlag == BUDDY_FLAG_TEMP)
            buddy.uFlags |= BUDDY_FLAG_WATCH;

        result = BuddyApiDel (pApiRef->pLLRef, &(buddy), _HLBOpCallback, pApiRef, pApiRef->op_timeout) ;
        _setLastOpState (pApiRef, result);
        if (result == HLB_ERROR_NONE )
        {
            //not needed, callback will delete bud.
            //if ( ( BuddyApiFind( pApiRef->pLLRef , buddyName) == NULL ) && ( pFoundHLB ) )
                //_HLBDestroyBuddy ( pFoundHLB  ); //contained buddy is gone, so free me

            if (typeFlag == BUDDY_FLAG_TEMP)
            { //bug in api -- c/b does not get called -- so call it  
                BuddyApiMsgT dummyMsg;
                dummyMsg.iCode = HLB_ERROR_NONE;
                dummyMsg.iType = BUDDY_MSG_DELUSER;
                dummyMsg.pData  = 0;
                _HLBOpCallback (pApiRef->pLLRef, &dummyMsg, pApiRef);

            }
            _signalBuddyListChanged (pApiRef);
        }
    }  //zap it ...
    return(result);
}

#if 0 // UNUSED
static int32_t _removeFirstTempBuddy (HLBApiRefT *pApiRef) 
{
    int32_t rc = HLB_ERROR_NONE;
    int32_t idx, val =0;
    HLBBudT *pBuddy;
    for (idx = 0, val = HLBListGetBuddyCount ( pApiRef);  idx < val; ++idx)
    {
        pBuddy = HLBListGetBuddyByIndex (  pApiRef, idx );
        if ( HLBBudIsTemporary (pBuddy ))
        {
            _HLBListRemoveBuddy (pApiRef, pBuddy->pLLBud->strUser, BUDDY_FLAG_TEMP, 0,0);  //signal ??
            return(rc);
        }
    }
    //if here, did not find a temp dude
    return(HLB_ERROR_FULL);
}
#endif

static int32_t _removeOldestTempBuddy (HLBApiRefT *pApiRef)
{
    int32_t rc = HLB_ERROR_NONE;
    int32_t idx, val =0;
    uint32_t uOldestSig =0xFFFFFFFF;
    HLBBudT *pBuddy =NULL, *pOldestTempBud = NULL;
    for (idx = 0, val = HLBListGetBuddyCount ( pApiRef);  idx < val; ++idx)
    {
        pBuddy = HLBListGetBuddyByIndex(pApiRef, idx);
        if ((HLBBudIsTemporary(pBuddy)) && (HLBBudTempBuddyIs(pBuddy, HLB_TEMPBUDDYTYPE_SESSION) == 0) && (pBuddy->pLLBud->uBudUserVal <= uOldestSig))
        {
            uOldestSig = pBuddy->pLLBud->uBudUserVal;
            pOldestTempBud =pBuddy;
        }
    }
    if (pOldestTempBud)
    {
        HLBPrintf((pApiRef,"Zap temp bud,%s of sig:%d\n", pOldestTempBud->pLLBud->strUser,pOldestTempBud->pLLBud->uBudUserVal)); 
        _HLBListRemoveBuddy (pApiRef, pOldestTempBud->pLLBud->strUser, BUDDY_FLAG_TEMP, 0,0); //signal ??
    }
    else  //  did not find a temp dude
    {
        rc = HLB_ERROR_FULL;
    }   
    return(rc);
}



//! -- Private -- Is there room to add this buddy?
static int32_t  _canAddBuddy ( HLBApiRefT *pApiRef,  int32_t typeFlag)
{
    int32_t idx = 0, val = 0, count = 0, rc = HLB_ERROR_NONE;
    HLBBudT *pBuddy;
    uint32_t uPermBudFlag =  BUDDY_FLAG_BUD_REQ_RECEIVED | BUDDY_FLAG_BUD_REQ_SENT |BUDDY_FLAG_BUDDY | BUDDY_FLAG_IGNORE;
         
    if (typeFlag == BUDDY_FLAG_TEMP)
    { //adding temp, count those marked temp, and only temp, and are NOT SESSION buds 
        for (idx = 0, val = _rosterCount(pApiRef); idx < val; ++idx)
        {
            //NOTE: Permanent buds will also have TEMP bit set if got message --too messy to change.
            //      Session temp buds are ignored in count, as MS requires we support at least 10
            //      so cannot do that if get messages from "temp" buddies.
            pBuddy = HLBListGetBuddyByIndex(pApiRef, idx);
            if (pBuddy && pBuddy->pLLBud &&                                     // we have a valid buddy reference
               ((pBuddy->pLLBud->uFlags & BUDDY_FLAG_TEMP) > 0) &&              // they are flagged as a temp buddy
               ((pBuddy->pLLBud->uFlags & uPermBudFlag) == 0) &&                // they are not a permanent buddy
               ((pBuddy->uTempBuddyFlags & HLB_TEMPBUDDYTYPE_SESSION) == 0))    // they are not a session buddy
            {
                count++;
            }

            if (count >= pApiRef->max_temp_buddies)
            { // we have too many temp buddies, try to free up some room
                rc = _removeOldestTempBuddy(pApiRef);
                break;
            }
        }
    }

    if (rc != HLB_ERROR_NONE)
    {
        // we weren't able to make room for the new buddy
        return(false);
    }

    if (( typeFlag == BUDDY_FLAG_IGNORE ) && ( _countBuddies (pApiRef, typeFlag)  >= pApiRef->max_block_buddies ) )
    { // adding blocked buddy, but no room -- client HAS to deal with this..
        return(false);
    }
    //want to add a perma buddy -- check full buddies, and invites..
    if ( typeFlag == BUDDY_FLAG_BUDDY) 
    { // real and invite wannnabes count as "permanent" buddies...
        uint32_t uFlag =BUDDY_FLAG_BUDDY | BUDDY_FLAG_BUD_REQ_SENT |  BUDDY_FLAG_BUD_REQ_RECEIVED;
        if (_countBuddies (pApiRef, uFlag)  >= pApiRef->max_perm_buddies )
            return(false);
    }
    return(true);
}

static int32_t _MsgListGetOldestMsgForBuddy (  HLBBudT *pBuddy, int32_t bReadOnly, int32_t bSkipInjected, uint32_t *puOldTS)
{
    BuddyApiMsgT *pMsg;
    int32_t idx =-1, count, max;
    
    uint32_t uTempTS=FUTURE_TS; //big- its so new, its way in future.

    if ((pBuddy == NULL ) || ( pBuddy->msgList == NULL ))
        return(idx);

    for (count=0, max=_listGetCount( pBuddy->msgList); count < max; count++)
    {
        pMsg = _listItemGetByIndex ( pBuddy->msgList,count);
        //remember NOT(A && B) == NOT A || NOT B
        if (pMsg)
        {
            if ( ((bReadOnly) &&  ((pMsg->uUserFlags & HLB_MSG_READ_FLAG) == 1)) ||
                ((bSkipInjected) && ( pMsg->iType ==  HLB_MSG_TYPE_INJECT))       )
            { //ignore this as a candidate as it can be skipped
            }
            else
            {
                if (pMsg->uTimeStamp < uTempTS )
                {  //older -- so save it
                    uTempTS = pMsg->uTimeStamp;
                    idx = count;
                }
            }
        }
    }
    //did we get new TS? --if so, return it.
    *puOldTS = (uTempTS < FUTURE_TS)? uTempTS: *puOldTS ;
    return(idx);
}

//! Delete an old msg, if matches temp  buddy and read msg status flags passed in.
static int32_t  _HLBMsgListDeleteOldestMsg(  HLBApiRefT * pApiRef,int32_t bTempBuddy, int32_t bReadOnly, int32_t bSkipInjected )
{
    int32_t  rc=-1,  iOldestBudMsgIdx =-1, iTmp;
    HLBBudT *pBuddy = NULL, *pOldestBud = NULL;
    uint32_t uTS=FUTURE_TS, uOldestTS=FUTURE_TS;
    //zap on either just temp buddy's of any buddy  ...
    int32_t idx=0, val =0;
    for (idx = 0, val = HLBListGetBuddyCount ( pApiRef);  idx < val; ++idx)
    {
        pBuddy = HLBListGetBuddyByIndex (  pApiRef, idx );\
        if (( (!bTempBuddy) || ( (bTempBuddy) && (HLBBudIsTemporary(pBuddy)) )))
        {
            if ((iTmp = _MsgListGetOldestMsgForBuddy (pBuddy, bReadOnly, bSkipInjected, &uTS)) >-1  )
            {
                if (uTS <= uOldestTS)
                {
                    uOldestTS = uTS;
                    pOldestBud = pBuddy;
                    iOldestBudMsgIdx = iTmp;
                }
            }
        }
    }
    if ( pOldestBud != NULL )
    {
         rc =0;
         HLBMsgListDelete ( pOldestBud,  iOldestBudMsgIdx);
    }
    return(rc);
}

//! Delete an old msg for a bud --looking for read messages first.
static int32_t  _HLBMsgListDeleteOldestMsgForBud(  HLBApiRefT * pApiRef, HLBBudT *pBuddy)// int32_t bReadOnly  )
{
    int32_t rc=-1,  iOldestBudMsgIdx =-1;
    int32_t bReadOnly=1, bSkipInjected =1;
    uint32_t uTS;
    if (( iOldestBudMsgIdx = _MsgListGetOldestMsgForBuddy (pBuddy, bReadOnly, bSkipInjected, &uTS)) >-1  )
    {
         HLBMsgListDelete ( pBuddy ,  iOldestBudMsgIdx); 
         return(0);
    }
    bReadOnly=0;
    //try to zap an old unread msg..
    if (( iOldestBudMsgIdx = _MsgListGetOldestMsgForBuddy (pBuddy, bReadOnly, bSkipInjected, &uTS)) >-1  )
    {
        HLBMsgListDelete ( pBuddy ,  iOldestBudMsgIdx); 
        return(0);
    }

    bSkipInjected =0;  //now just find any old msg..
    if (( iOldestBudMsgIdx = _MsgListGetOldestMsgForBuddy (pBuddy, bReadOnly, bSkipInjected, &uTS)) >-1  )
    {
        HLBMsgListDelete ( pBuddy ,  iOldestBudMsgIdx);
        return(0);
    }
    return(rc);
} //oldestperbud

//! Find a read message and delete it to free room for a new one.Returns 0 if did so, -1 if failed.
static int32_t _HLBMsgListJettisonOldMsg (HLBApiRefT * pApiRef)//, int32_t bTempBuddy, HLBBudT* pBuddy)
{   
    //first try zap msg on temp buddy's read, then unread, and finally on all buds.
    int32_t rc =-1, bTempBuddy=1;
    if ( (rc =_HLBMsgListDeleteOldestMsg (pApiRef,  bTempBuddy,1,1)) !=0 )
        if ( (rc =_HLBMsgListDeleteOldestMsg (pApiRef,  bTempBuddy,0,1)) !=0 )
        { //temps no msg, so now try all buds, but look for read msg first, and then look for injected
            bTempBuddy =0; 
            if ( (rc =_HLBMsgListDeleteOldestMsg (pApiRef,  bTempBuddy,1,1)) !=0 )
            {
                if ( (rc = _HLBMsgListDeleteOldestMsg (pApiRef,  bTempBuddy,0,1)) !=0)
                {
                    rc =_HLBMsgListDeleteOldestMsg (pApiRef,  bTempBuddy,1,0); 
                }
            }
        }
    return(rc);
}
         


static int32_t _HLBMsgListAddMsg (  HLBApiRefT *pApiRef, const char * strUser , BuddyApiMsgT *pMsg, int32_t bNoReply )
{
    BuddyApiMsgT *pClonedMsg;
    HLBBudT* pFoundBud =0;
    int32_t rc = HLB_ERROR_UNKNOWN;
    int32_t iBudMsgCount = 0;

    if (HLBBudIsBlocked(HLBListGetBuddyByName(pApiRef,strUser)))
    {
        return(HLB_ERROR_BLOCKED);
    }

    //add a buddy as temp, if he aint there..
    if ((rc = HLBListFlagTempBuddy (pApiRef, strUser, HLB_TEMPBUDDYTYPE_MESG))!= HLB_ERROR_NONE)
    {
        return(rc);
    }

    pFoundBud = HLBListGetBuddyByName(pApiRef, strUser) ;
    if( (pFoundBud == NULL)  ||  (pFoundBud->msgList == NULL))
    {
        HLBPrintf((pApiRef, "ERROR: msg, but bud or msg list is null:%s: \n" , strUser));
        return(HLB_ERROR_MISSING); // more like a sw error.
    }


    //"Temp" buds, only get one msg, to avoid spamming me.
    //Someone who invited me should also be "temp" for this purpose.
    // Admin folks are allowed more that 1 msg-- so check for NOREPLY too..
    if (((HLBBudIsTemporary ( pFoundBud)) || HLBBudIsWannaBeMyBuddy( pFoundBud) )
        && (bNoReply ==0) && pMsg->iType == HLB_MSG_TYPE_CHAT )
    { //temp bud,  get's just ONE  chat msg, but  allow for many admin or injected message... 
        int32_t iCount =0, iChat =0, iMax =0;
        for (iCount=0, iMax=_listGetCount( pFoundBud->msgList); iCount < iMax; iCount++)
        {
            BuddyApiMsgT *pTmpMsg = _listItemGetByIndex ( pFoundBud->msgList,iCount);
            if ((pTmpMsg) && (pTmpMsg->iType == HLB_MSG_TYPE_CHAT))
            {
                iChat++;
            }
        }
        if (iChat > 0)
        { // allow  for 1 chat messages -- so zap whats there..
                _HLBMsgListDeleteOldestMsgForBud( pApiRef, pFoundBud);
        }
    }

    //regular  or admin bud, -- gets  config limit 
    iBudMsgCount =  HLBMsgListGetTotalCount (pFoundBud);
    if ( iBudMsgCount >= pApiRef->max_msgs_per_perm_buddy )
    {
        if (  _HLBMsgListDeleteOldestMsgForBud( pApiRef, pFoundBud) < 0)
        {
            HLBPrintf(( pApiRef, "Msg Add failed, could not remove a per bud msg. Cnt=%d\n",pApiRef->msgCount));  
            return(HLB_ERROR_FULL); 
        }
    }

    // now check the global msg count.. and zap if needed.
    if (pApiRef->msgCount >= pApiRef->max_msgs_allowed ) 
    { //full, so, free one
        if (_HLBMsgListJettisonOldMsg(pApiRef) != 0)
        {
            HLBPrintf(( pApiRef, "Msg Add failed, full but could not jettison one\n"));
            return(HLB_ERROR_FULL);
        }
    }
   
    if ((pClonedMsg = _cloneMsg(pApiRef, pMsg)) == NULL)
    {
        HLBPrintf(( pApiRef, "Msg Add failed, could not clone..\n"));
        return(HLB_ERROR_MEMORY); //out of memory;
    }

    pApiRef->msgCount++;  //increment count,  as adding one..
    _listItemAdd (  pFoundBud->msgList, pClonedMsg);
    pFoundBud->msgListDirty = true;
    if (bNoReply ==1)
    {
        pFoundBud->pLLBud->uFlags |= BUDDY_FLAG_NOREPLY;
        _signalBuddyListChanged ( pApiRef); //changed a flag, so tell client.
    }

    //Now  call the callers (ie. clients) callback too..
    if  ( (pApiRef->pMesgProc )   && (pMsg ) )
    {
        pApiRef->pMesgProc(pApiRef, pMsg, pApiRef->pMesgData);
    }
    HLBPrintf(( pApiRef, "Msg, added, cnt=%d, text=%d\n",  pApiRef->msgCount, pMsg->pData));
    return(HLB_ERROR_NONE); 
}


//! -- Private -- To phyically add or change a  buddy, if needed.
static int32_t _HLBListAddChangeBuddy (   HLBApiRefT  *pApiRef, const char * name, uint32_t typeFlag, 
                          int32_t tempBuddyType,  HLBOpCallbackT *cb , void * context)
// note -- based on type, buddy may be persisted, or not.  
{   
    HLBBudT* pNewBud;
    HLBBudT* pExistingBuddy;
    BuddyRosterT oldLLBud; 
    BuddyRosterT newLLBud; 
    memset(&oldLLBud,0, sizeof (oldLLBud));

    if ( pApiRef->iStat == BUDDY_STAT_PASS )
        return(HLB_ERROR_USER_STATE);

    if (HLBApiGetConnectState (pApiRef)  != HLB_CS_Connected )
    { //we can save temp buddies till connected-- LobbyGlue for tourney needs this.
        if ( typeFlag == BUDDY_FLAG_TEMP )
        {
            _preConnTempBudAdd (pApiRef, name, tempBuddyType);
            return(HLB_ERROR_NONE); //we promise to load it later..
        }
        else
            return(HLB_ERROR_OFFLINE);
    }
    // is he in list already?
    if ( (pExistingBuddy = HLBListGetBuddyByName ( pApiRef, name)) != NULL )
    { //found onr -- do we need to do work??
        if (typeFlag == BUDDY_FLAG_TEMP ) //found bud, and adding a temp, so set flag and done...
        {
            uint32_t uOldFlags = pExistingBuddy->uTempBuddyFlags ;
            pExistingBuddy->uTempBuddyFlags |= tempBuddyType;
            // is he just becoming a temp?
            if ( (pExistingBuddy->pLLBud->uFlags  & BUDDY_FLAG_TEMP) ==0 )
            { //was a perma bud, now is also temp. Set it to get Presence, and so zap will work.
                memcpy (&newLLBud,  pExistingBuddy->pLLBud, sizeof (newLLBud));
                newLLBud.uFlags = BUDDY_FLAG_TEMP; //clear all others..
                newLLBud.uFlags |= BUDDY_FLAG_WATCH; //need to subscribe presence...

                //this will add as a temp, and subscribe, and update flags etc.
                BuddyApiAdd(pApiRef->pLLRef, &newLLBud ,NULL, NULL, pApiRef->op_timeout );
                _signalBuddyListChanged (pApiRef); //we changed it. Tell caller. 
            }
            //tell clients that we did change something ... even tho it was them that set it.
            else if ( (uOldFlags & tempBuddyType ) == 0 ) 
                _signalBuddyListChanged (pApiRef); //we set it for real. Tell caller. 

            return(HLB_ERROR_NONE); // done -- with temp buds.
        }
        //for perma bud, does what we want, match in list? Done if so.
        if ( (typeFlag & pExistingBuddy->pLLBud->uFlags) != 0)
            return(HLB_ERROR_NONE); //nothing to do..
    }

    //if need to add -- do we have or can make space for this bud?
    if ( typeFlag == BUDDY_FLAG_IGNORE ) 
    {  //blocked buddy --not blocked if here so check unconditionally.
        if (! _canAddBuddy ( pApiRef, typeFlag))
            return  HLB_ERROR_FULL;
    }
    else if ( ( pExistingBuddy == NULL ) && (tempBuddyType != HLB_TEMPBUDDYTYPE_SESSION) && (! _canAddBuddy ( pApiRef, typeFlag  )) )
        return(HLB_ERROR_FULL);

    //waiting for prev op to complete?
    if (  pApiRef->iLastOpState == HLB_ERROR_PEND ) 
    {
        //Allow to pass if temp bud and no callback otherwise, async
        //messages received from (new) temp bud,while in PEND state, will fail.
        if ((typeFlag != BUDDY_FLAG_TEMP ) || (cb != NULL ))
            return(HLB_ERROR_PEND);
    }

    if ((cb) )
    {
        pApiRef->pOpProc = cb;
        pApiRef->pOpData = context;
    }

    if  ( pExistingBuddy )
    { //change
        HLBPrintf( ( pApiRef, "Changing buddy %s type=%d \n", name, typeFlag));
        oldLLBud = *(pExistingBuddy->pLLBud);
    }

    //if here, we are all ok for add or change..
    HLBPrintf( ( pApiRef, "Add/c buddy %s type=%d \n", name, typeFlag));
    newLLBud = oldLLBud;
    ds_strnzcpy (newLLBud.strUser , name, sizeof( newLLBud.strUser) );
    newLLBud.uFlags=0; //clear all old flags, add new bud only needs whats to be set.
    newLLBud.uFlags |= typeFlag;
    if (typeFlag == BUDDY_FLAG_TEMP )
        newLLBud.uFlags |= BUDDY_FLAG_WATCH; // need to explicitly subscribe for others presence.

    pNewBud  = _addChangeLLBud ( pApiRef, oldLLBud, newLLBud); //change and return HL Bud.
    if (pNewBud == NULL )
    {
        return(HLB_ERROR_MEMORY); //out of mem..
    }
    else
    {   //did add or change
        if (typeFlag == BUDDY_FLAG_TEMP)
        {   
            BuddyApiMsgT dummyMsg;
                //just added a temp buddy, set his temp type..
            pNewBud->uTempBuddyFlags |= tempBuddyType;

            //todo  bug in api -- c/b does not get called -- so call op cb  etc.
            dummyMsg.iCode = HLB_ERROR_NONE;
            dummyMsg.iType = BUDDY_MSG_ADDUSER;
            dummyMsg.pData  = 0;
            _HLBOpCallback (pApiRef->pLLRef, &dummyMsg, pApiRef);
            _signalBuddyListChanged (pApiRef);
        }
    } //did add or change
    return(HLB_ERROR_NONE);
}//! add a message from  a buddy.


//! Helper killer function to nuke all invites -- NO Callbacks. To be
//! used when blocking a buddy, and want all invites nuked. 
int32_t HLBListCancelAllInvites ( HLBApiRefT  *pApiRef, const char *pName)
{
    uint32_t uGameFlags =0;
    int32_t rc=0;
    HLBBudT *pBuddy = HLBListGetBuddyByName ( pApiRef, pName);

    if (!pBuddy)
        return(HLB_ERROR_NOTUSER);

    uGameFlags = HLBBudGetGameInviteFlags( pBuddy );

    // check to see if this buddy has invited me to a game
    if ((uGameFlags & HLB_FLAG_GAME_INVITE_RECEIVED) > 0)
    {
        rc+=BuddyApiRespondGame(pApiRef->pLLRef, pBuddy->pLLBud, (BuddyInviteResponseTypeE)HLB_INVITEANSWER_REJECT, NULL, NULL, pApiRef->op_timeout, pApiRef->uGISessionId);
    }
    // check to see if I invited this buddy to a game
    if ((uGameFlags & HLB_FLAG_GAME_INVITE_SENT) > 0)
    {
        rc+=BuddyApiRespondGame(pApiRef->pLLRef, pBuddy->pLLBud, (BuddyInviteResponseTypeE)HLB_INVITEANSWER_REVOKE, NULL, NULL, pApiRef->op_timeout, pApiRef->uGISessionId);
    }
    if (  HLBBudIsIWannaBeHisBuddy (pBuddy))
    { //this may nuke the bud, so re-fetch
        rc +=BuddyApiRespondBuddy( pApiRef->pLLRef,   pBuddy->pLLBud, (BuddyInviteResponseTypeE)HLB_INVITEANSWER_REVOKE, NULL , NULL, pApiRef->op_timeout);
        pBuddy = HLBListGetBuddyByName ( pApiRef,pName);
    }

    if ( (pBuddy) && (pBuddy->pLLBud) && HLBBudIsWannaBeMyBuddy (pBuddy))
    {
        rc+=BuddyApiRespondBuddy( pApiRef->pLLRef,   pBuddy->pLLBud, (BuddyInviteResponseTypeE)HLB_INVITEANSWER_REJECT, NULL , NULL, pApiRef->op_timeout);
    }
    return(rc);

}


void HLBListDisableSorting( HLBApiRefT  *pApiRef  )
{
    if (  pApiRef != NULL)
    {
        // Just before we disable sorting we should sort the list one last time.
        // Force the list to appear to be changed so it will get resorted.
                if ( _getRost (pApiRef) == NULL )
          return;

        DispListChange(_getRost(pApiRef), 1);

        // Resort the list.
        DispListOrder( _getRost(pApiRef));

        // Now disable sorting by setting the sort function to NULL.
        DispListSort( _getRost(pApiRef), NULL, 0, NULL);
    }
}

#if 0 // UNUSED
static void _HLBEnableSorting( HLBApiRefT  *pApiRef )
{
    if ( _getRost(pApiRef)  != NULL)
    {
        DispListSort( _getRost(pApiRef) ,  pApiRef , 0,  pApiRef->pBuddySortFunc);
        // Force the list to appear to be changed so it will get resorted.
        DispListChange( _getRost(pApiRef), 1);
        // Resort the list.
        DispListOrder( _getRost(pApiRef) );
    }
}
#endif

static int32_t _HLBRosterSort(void *pSortref, int32_t iSortcon, void *pRecptr1, void *pRecptr2)
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

//! Private -- we have got a msg, process it and call higher levels if needed.
static void _HLBMsgCallback(BuddyApiRefT *pRef, BuddyApiMsgT *pMsg, void *pUserData)
{
    HLBApiRefT *pApiRef = ( HLBApiRefT * ) pUserData;
    char strUser [ HLB_BUDDY_NAME_LEN];
    int32_t bNoReply =0;
    TagFieldGetString(TagFieldFind(pMsg->pData,"USER"),strUser,sizeof(strUser),"Not Present");

    switch(pMsg->iType)
    {

    case BUDDY_MSG_CHAT:
                pMsg->iKind = HLB_CHAT_KIND_CHAT;
    case HLB_MSG_TYPE_INJECT:
        
        //silently truncates long messages??...todo
        //TagFieldGetString(TagFieldFind(pMsg->pData, "BODY"),  strTemp , sizeof( strTemp)-1, "");
        //TagFieldGetString(TagFieldFind(pMsg->pData, "TYPE"), strType, sizeof(strType)-1, "");
        bNoReply = TagFieldGetNumber(TagFieldFind(pMsg->pData, "NOREPLY"),0);
        if (bNoReply)
        { // only admin msgs have no reply set -- so set -- UNTIL they fix server..
            pMsg->iKind = HLB_CHAT_KIND_ADMIN;
            HLBPrintf( ( pApiRef, "got special  admin message..."));     
        }

        if (pApiRef->bRosterLoaded == 0)
        { //race condition --server sent saved msg before roster came in...
          // So, save msgs  till connected and have loaded roster..  
            _preConnMsgAdd (pApiRef, strUser,  pMsg);
            return; //dont try to add it--will be temp bud, and only 1 saved.
        }

        //add to this buddy's msg list, and mark list  dirty. ...and add buddy if aint there..
        _HLBMsgListAddMsg ( pApiRef, strUser,  pMsg, bNoReply );
        break;

    default:
        HLBPrintf( ( pApiRef,"Warning: Unknown message type '%c%c%c%c' received.\n",
            ((char *)&pMsg->iType)[3],
            ((char *)&pMsg->iType)[2],
            ((char *)&pMsg->iType)[1],
            ((char *)&pMsg->iType)[0]));
        break;
    }
}

//! Callback that buddyapi calls  when lower level buddy is to be freed.
static void _HLBBuddyDelMemCallback(BuddyApiRefT *pRef, BuddyRosterT *pLLBud,   void *pCalldata)
{
    //HLBApiRefT *pApiRef = ( HLBApiRefT * ) pCalldata;
    HLBBudT *pBuddy = NULL;
    if ( pLLBud && pLLBud->pUserRef )
    {
        pBuddy = pLLBud->pUserRef;
    }
    if (pBuddy != NULL)
    {
        _HLBDestroyBuddy( pBuddy);
    }
}
//! Private -- we have got game  or presence notification.
static void _HLBBuddyChangeCallback(BuddyApiRefT *pRef, BuddyRosterT *pLLBud, int32_t iType, int32_t iAction, void *pUserData)
{
    HLBApiRefT *pApiRef = ( HLBApiRefT * ) pUserData;
    HLBBudT *pBuddy = pLLBud->pUserRef;
    if (pBuddy == NULL)
        return; 
    if  (iType == BUDDY_MSG_PRESENCE)
    { //presence changed -- pass buddy and old state.
        if (pApiRef->pBuddyPresProc)  
        {
            pApiRef->pBuddyPresProc ( pApiRef, pBuddy, iAction, pApiRef->pBuddyPresData );
        }

    }
    else if  (iType == BUDDY_MSG_GAME_INVITE)
    { //game invite
        if (iAction == BUDDY_FLAG_GAME_INVITE_TIMEOUT )
        {
            pBuddy->pLLBud->uFlags |= HLB_FLAG_GAME_INVITE_TIMEOUT; //clear on next poll..
        }
         
        if (iAction == BUDDY_FLAG_GAME_INVITE_ACCEPTED )
        {  
            if ( HLBBudIsSameProduct (pBuddy) )
            {//already online, in my product -- he is ready, so fire ready callbck..
                iAction = BUDDY_FLAG_GAME_INVITE_READY;
                pBuddy->pLLBud->uFlags |= BUDDY_FLAG_GAME_INVITE_READY;
            }
            //else, normal case -- he accepted in gameY, save and wait for him to show in my game.
            else
            {  
                if ( pApiRef->iGIAccept_timeout_param >0)
                    pApiRef->uGIAcceptTimeout=pApiRef->uTick +  pApiRef->iGIAccept_timeout_param *1000;    
                ds_strnzcpy (pApiRef->strGIAcceptBuddy, HLBBudGetName (pBuddy), sizeof (pApiRef->strGIAcceptBuddy));
            }
        }

        if (    (iAction  == BUDDY_FLAG_GAME_INVITE_READY )
            ||  (iAction == BUDDY_FLAG_GAME_INVITE_REVOKE )
            ||  (iAction == BUDDY_FLAG_GAME_INVITE_REJECTED )   )

        { //he accepted my game and  showed up, or it go removed ... cancel timer
            pApiRef->uGIAcceptTimeout = 0;
        }
        if (iAction == BUDDY_FLAG_GAME_INVITE_REJECTED)
        { //clear flags by doing a revoke..
            BuddyApiRespondGame ( pApiRef->pLLRef, pBuddy->pLLBud, BUDDY_INVITE_RESPONSE_REVOKE, NULL,NULL,0,pApiRef->uGISessionId);
        }

        if (iAction == BUDDY_FLAG_GAME_INVITE_RECEIVED)
        { // invited me, but, is he blocked?
            if ( HLBBudIsBlocked (pBuddy) )
            { //silently reject, as I blocked him
                BuddyApiRespondGame ( pApiRef->pLLRef, pBuddy->pLLBud, BUDDY_INVITE_RESPONSE_REJECT, NULL,NULL,0,pApiRef->uGISessionId);
                return; //dont bother user, no change.
            }
        }

        if (pApiRef->pGameInviteProc  )
        {
            pApiRef->pGameInviteProc ( pApiRef, pBuddy, iAction, pApiRef->pGameInviteData );
    
            //now that we told client, for t/o, get both sides to clear things up..
            if (iAction == BUDDY_FLAG_GAME_INVITE_TIMEOUT )
            {
                HLBListAnswerGameInvite( pApiRef, HLBBudGetName (pBuddy), 
                                            HLB_INVITEANSWER_REVOKE,0,0);
            }
        }
        _signalBuddyListChanged( pApiRef); //flags changed..
    }
}

static HLBApiRefT *_HLBApiInit(HLBApiRefT *pApiRef, const char *pProduct, const char *pRelease, int32_t iRefMemGroup, void *pRefMemGroupUserData)
{
    if (pApiRef == NULL)
    {
        return(NULL);
    }

    pApiRef->uLocale = LOBBYAPI_LOCALITY_DEFAULT; //default, in case they dont set it.

    pApiRef->pBuddySortFunc = _HLBRosterSort;
    pApiRef->pLLRef =  BuddyApiCreate2( pApiRef->pBuddyAlloc,
                                        pApiRef->pBuddyFree,
                                        pProduct,  pRelease,
                                        iRefMemGroup,
                                        pRefMemGroupUserData);
    pApiRef->iLastOpState = HLB_ERROR_NOOP;

    if  (pApiRef->pLLRef  == NULL )
    {  //no go pogo, clean and return
         pApiRef->pBuddyFree(pApiRef, HLBUDAPI_MEMID, pApiRef->iMemGroup, pApiRef->pMemGroupUserData); 
        pApiRef = NULL;
    }
    else    
    {
        pApiRef->pPreConnTempBudList  = _listCreate (pApiRef);
        BuddyApiRecv ( pApiRef->pLLRef,_HLBMsgCallback, pApiRef ); 
        BuddyApiRegisterBuddyChangeCallback(pApiRef->pLLRef,_HLBBuddyChangeCallback, pApiRef);
        BuddyApiRegisterBuddyDelCallback(pApiRef->pLLRef,_HLBBuddyDelMemCallback, pApiRef);
        //set the dynaminc constants to their default values, client can change later if they wish.
        HLBApiOverrideConstants ( pApiRef, 
            0,  // we do text messages by default
            HLB_CONNECT_TO, // connect_timeout, 
            HLB_OP_TO, //int32_t op_timeout,  
            HLB_MAX_PERM_BUDDIES, //int32_t max_perm_buddies, 
            HLB_MAX_TEMP_BUDDIES, //int32_t max_temp_buddies,
            HLB_MAX_MESSAGES_ALLOWED,  //int32_t msgs_per_perm_buddy,
            -1,// talk to xbox? --unconditionally send all cmds to xdk, 
            HLB_ABSOLUTE_MAX_MSG_LEN,
            0, //dont track status of sent messages.
            HLB_XGAME_READY_TO);  //GameInvite Accept to
        //unless override, one buddy can send all 100 msgs.
        pApiRef->max_msgs_per_perm_buddy  = HLB_MAX_MESSAGES_ALLOWED; 
        pApiRef->max_block_buddies = HLB_MAX_BLOCKED_BUDDIES; //server allows 50, allow to add 5.

        //set some defaults, in case client dont call HLBApiInitilize..
        BuddyApiConfig( pApiRef->pLLRef,  "unknown" , pApiRef->uLocale);
        BuddyApiPresInit( pApiRef->pLLRef );
    }
    return(pApiRef);
}


/*** Public Functions *****************************************************************/




/*F*************************************************************************************************/
/*!
    \Function    HLBApiCreate

    \Description
        Create an instance of the High Level Buddy API. 

    \Input pProduct                 - Product name.      
    \Input *pRelease                - Release version
    \Input iRefMemGroup             - mem group for hlbapiref (zero=default)
    \Input *pRefMemGroupUserData    - pointer to user data associated with mem group

    \Output *HLBApiRefT             - API reference pointer, to be used on subsequent calls.
*/
/*************************************************************************************************F*/

HLBApiRefT *HLBApiCreate(const char *pProduct, const char *pRelease, int32_t iRefMemGroup, void *pRefMemGroupUserData)
{
    HLBApiRefT *pApiRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // if no ref memgroup specified, use current group
    if (iRefMemGroup == 0)
    {
        iRefMemGroup = iMemGroup;
        pRefMemGroupUserData = pMemGroupUserData;
    }

    // allocate a new state record
    pApiRef = ( HLBApiRefT*) DirtyMemAlloc (sizeof(*pApiRef), HLBUDAPI_MEMID, iRefMemGroup, pRefMemGroupUserData); 
    if (pApiRef != NULL)
    {
        memset(pApiRef, 0, sizeof(*pApiRef));
        pApiRef->pBuddyAlloc = DirtyMemAlloc; //see HLBApiCreate2 to register user mem funcs.
        pApiRef->pBuddyFree = DirtyMemFree;
        pApiRef->iMemGroup = iMemGroup;
        pApiRef->pMemGroupUserData = pMemGroupUserData;
        pApiRef->iRefMemGroup = iRefMemGroup;
        pApiRef->pRefMemGroupUserData = pRefMemGroupUserData; 
    }

    return(_HLBApiInit ( pApiRef, pProduct, pRelease, iRefMemGroup, pRefMemGroupUserData));
}


/*F*************************************************************************************************/
/*!
    \Function    HLBApiCreate2

    \Description
        Create HL Buddy Api, overriding the default memory alloc/free functions.

    \Input pAlloc                   - function to allocate memory.
    \Input pFree                    - function to free memory.
    \Input pProduct                 - Product name.      
    \Input *pRelease                - Release version
    \Input iRefMemGroup             - mem group for hlbudapi ref (zero=default)
    \Input *pRefMemGroupUserData    - pointer to user data associated with mem group

    \Output *HLBApiRefT             - API reference pointer, to be used on subsequent calls.
*/
/*************************************************************************************************F*/

HLBApiRefT *HLBApiCreate2( BuddyMemAllocT *pAlloc,  BuddyMemFreeT *pFree,
                           const char *pProduct, const char *pRelease, 
                           int32_t iRefMemGroup, void *pRefMemGroupUserData)
{
    HLBApiRefT *pApiRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // if no ref memgroup specified, use current group
    if (iRefMemGroup == 0)
    {
        iRefMemGroup = iMemGroup;
        pRefMemGroupUserData = pMemGroupUserData;
    }

    // if no alloc functions, bail
    if ((pAlloc == NULL ) || (pFree== NULL))
    {
        return(NULL); // use HLBApiCreate instead.
    }

    // allocate a new state record  
    pApiRef = ( HLBApiRefT*) DirtyMemAlloc (sizeof(*pApiRef), HLBUDAPI_MEMID, iRefMemGroup, pRefMemGroupUserData); 
    
    if (pApiRef != NULL)
    {
        memset(pApiRef, 0, sizeof(*pApiRef));
        pApiRef->pBuddyAlloc = pAlloc;
        pApiRef->pBuddyFree = pFree;
        pApiRef->iMemGroup = iMemGroup;
        pApiRef->pMemGroupUserData = pMemGroupUserData;
        pApiRef->iRefMemGroup = iRefMemGroup;
        pApiRef->pRefMemGroupUserData = pRefMemGroupUserData;
    }
    return(_HLBApiInit ( pApiRef, pProduct, pRelease, iRefMemGroup, pRefMemGroupUserData));
}

/*F*************************************************************************************************/
/*!
    \Function    HLBApiSetDebugFunction 

    \Description
        Set a callback to write out  debug messages within API.

        \Input *pApiRef - API reference pointer
        \Input *pDebugData    -pointer to any caller data to be output with the prints.
        \Input *pDebugProc  - Function pointer that will write debug info.

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
    BuddyApiDebug( pApiRef->pLLRef, pDebugData, pDebugProc );
}

//!Application  may override if XOnlineTitleIdIsSameTitle is also to be used to check for
//! is same product. Usually use PROD field from server, and for Xbl, the XOnlineTitleIdIsSameTitle too
//! to allow for "offline" users, who wont have a PROD field, in presence, when offline.
//!Set to true(1), if say, NTSC and PAL cannot play each other, but, both have same XBL TitleId.
 
void HLBApiOverrideXblIsSameProductCheck (HLBApiRefT  * pApiRef, int32_t bDontUseXBLSameTitleIdCheck)
{
    pApiRef->bDontUseXBLSameTitleIdCheck = bDontUseXBLSameTitleIdCheck;
}


//!Application  may override default for max msg per permanent buddy  
void HLBApiOverrideMaxMessagesPerBuddy (HLBApiRefT  * pApiRef, 
                                        int32_t  max_msgs_per_perm_buddy)
{
    pApiRef->max_msgs_per_perm_buddy = max_msgs_per_perm_buddy;
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
    if  ( connect_timeout != -1 )
        pApiRef->connect_timeout = connect_timeout;
    if  ( op_timeout != -1 )
        pApiRef->op_timeout = op_timeout;
    if  ( max_perm_buddies != -1 )
        pApiRef->max_perm_buddies  = max_perm_buddies;
    if  ( max_temp_buddies != -1 )
        pApiRef->max_temp_buddies  = max_temp_buddies;
    if  ( max_msgs_allowed != -1 )
        pApiRef->max_msgs_allowed  = max_msgs_allowed;

    if  (( max_msg_len != -1 ) &&  ( max_msg_len <= HLB_ABSOLUTE_MAX_MSG_LEN ) )
        pApiRef->max_msg_len  = max_msg_len;
    
    if  ( bNoTextSupport == 1 )
          BuddyApiPresNoText (pApiRef->pLLRef);


    //talk to Xbox is dangerous -- ONLY allow in debug mode.
#if DIRTYCODE_DEBUG
    if ( bTalk_to_xbox==1)
        _BuddyApiSetTalkToXbox ( pApiRef->pLLRef, 1);
#endif

    if ( bTrackSendMsgInOpState ==1)
        pApiRef->bTrackSendMsgInOpState  = 1;

    if ( iGameInviteAccept_timeout != -1)
        pApiRef->iGIAccept_timeout_param  = iGameInviteAccept_timeout;
}

//!Set if want api to translate utf8 chars to 8 bit characters in Presence, and Message Text.
void HLBApiSetUtf8TransTbl ( HLBApiRefT  * pApiRef, Utf8TransTblT *pTransTbl)
{
    pApiRef->pTransTbl = pTransTbl;
}


/*F*************************************************************************************************/
/*!
    \Function    HLBApiInitialize

    \Description
        Configure  and initialize any data structures in the API, and set some basic product and presence info.

    \Input *pApiRef         - HL api reference pointer
    \Input *prodPresence    - identifies the product and used for init presence.
    \Input uLocality        - The users locale -determines language to title etc.

    \Notes
        By server magic, the prodPresence field controls HLBBudIsSameProduct, so make 
        sure this is unique for your product. This Might also be used to set initial presence,
        but use the Presence Api to set your presence on connecting, and all will be fine.

    \Output
        int32_t             - HLB_ERROR_NONE if ok.
*/
/*************************************************************************************************F*/
int32_t HLBApiInitialize( HLBApiRefT  * pApiRef, const char * prodPresence , uint32_t uLocality )

{
    if (!pApiRef)
    {
        return(HLB_ERROR_MISSING);
    }
    pApiRef->uLocale = uLocality;
    return(BuddyApiConfig(pApiRef ->pLLRef, prodPresence , pApiRef->uLocale));
}
/*F*************************************************************************************************/
/*!
    \Function HLBApiConnect
    \Description
        Connect to buddy server. Note that if connect fails for network or server-unavailability, we will 
            keep retrying  connect within the connect timeout period.

    \Input *pApiRef    - reference pointer
    \Input *pServer    - buddy server hostname
    \Input port        - buddy server port
    \Input *key        - session key (or username:password for dev)
    \Input *pRsrc      - Resource (domain/sub-domain) uniquely identified product.

    \Output
        int32_t         - return status; HLB_ERROR_NONE if ok.

    \Notes
    \verbatim
        1. Will retry to connect if connect fails due to network or server unavailability.
        2. Will retry later too, if it noticed that server connection went down. Uses timeout specified in
           HLBApiOverrideConstants().
        3. If a callback is not specified, client checks for status of request using HLBApiGetConnectState().
        4. After connection is made, this api guarantees reconnect if server or network goes down. Client will know
           if server/came-up went down via  the callback or via  HLBGetConnectionStatus() poll. 
    \endverbatim
*/
/*************************************************************************************************F*/
int32_t HLBApiConnect( HLBApiRefT  * pApiRef, const char * pServer, int32_t port, const  char *key, const char *pRsrc )
{
    char strRsrc[255];
    if  ( (  pApiRef  == NULL ) ||   (  pApiRef->pLLRef == NULL ) )
        return(HLB_ERROR_BADPARM);

    _HLBRosterDestroy( pApiRef ); //just in case, they had mem around, clear it.

    pApiRef->iStat = BUDDY_STAT_DISC; //anything but PASSive -- as doing connect.

    // adding leading slash if there isn't one
    if (pRsrc[0] != '/' )
    {
        strRsrc[0] = '/';
        ds_strnzcpy(&strRsrc[1], pRsrc, sizeof(strRsrc)-1);
        pRsrc = strRsrc;
    }
    pApiRef->connectState = HLB_CS_Connecting;
    pApiRef->msgCount =0; //just in case..
    return(BuddyApiConnect(pApiRef->pLLRef, pServer, port, pApiRef->connect_timeout , key,  pRsrc , _HLBConnectCallback, pApiRef));
}

int32_t HLBApiRegisterConnectCallback( HLBApiRefT  * pApiRef,  HLBOpCallbackT *cb , void * pContext)
{
    if (pApiRef  == NULL)
        return(HLB_ERROR_BADPARM);
    pApiRef->pConnectProc = cb;
    pApiRef->pConnectData = pContext;
    return(HLB_ERROR_NONE);
}

/*F*************************************************************************************************/
/*!
    \Function    HLBApiGetConnectState

    \Description
        Get state of connection to the server, returns int, but see HLBConnectState_Enum.

    \Input *pApiRef     - HL api reference pointer

    \Output
        int32_t         - connect state; int, but see HLBConnectState_Enum.
*/
/*************************************************************************************************F*/
int32_t HLBApiGetConnectState ( HLBApiRefT  * pApiRef)
{
    //return saved state, or Disconnected if api is null.
    int32_t iCS = (pApiRef)? pApiRef->connectState:HLB_CS_Disconnected;
    return(iCS);
}

/*F*************************************************************************************************/
/*!
    \Function    HLBApiDisconnect

    \Description
        Disconnect from buddy server.

    \Input *pApiRef     - HL api reference pointer
*/
/*************************************************************************************************F*/
void  HLBApiDisconnect(HLBApiRefT *pApiRef)
{
    if (pApiRef == NULL)
        return;

    _HLBRosterDestroy( pApiRef ); //destroy first, as roster could be nuked.
    if (pApiRef->pLLRef )
    {
        BuddyApiDisconnect( pApiRef->pLLRef );
        pApiRef->connectState = HLB_CS_Disconnected;
        pApiRef->iStat = BUDDY_STAT_DISC;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    HLBApiDestroy

    \Description
        Destroy this API and any memory held by it.  Also destroy lower level BuddyApi held by this.

    \Input *pState  - API reference pointer
*/
/*************************************************************************************************F*/
void HLBApiDestroy(HLBApiRefT *pState)
{
    if (pState == NULL)
    {
        return;
    }
    if (pState->pLLRef)
    {
        _HLBRosterDestroy( pState);
    }

    if (pState->pLLRef)
    {
        BuddyApiDestroy(pState->pLLRef);
    }

    _preConnTempBudListDestroy (pState);
        
    DirtyMemFree(pState, HLBUDAPI_MEMID, pState->iRefMemGroup, pState->pRefMemGroupUserData);  //now i free me..

}

/*F*************************************************************************************************/
/*!
    \Function HLBApiUpdate

    \Description
      Give API and api below some time to process.
      Must be called periodically by client.

    \Input *pApiRef     - reference pointer
*/
/*************************************************************************************************F*/
void HLBApiUpdate (HLBApiRefT  * pApiRef) 
{
     // idle time tasks
    if ( !pApiRef)
        return;

    pApiRef->uTick = BuddyApiUpdate(pApiRef->pLLRef );  

    // check for timeout
    if ((pApiRef->uGIAcceptTimeout != 0) && (pApiRef->uTick > pApiRef->uGIAcceptTimeout))
    { // invitee did not get ready in time .. treat a buddy change callback, but at app level.
        HLBBudT *pBuddy = HLBListGetBuddyByName ( pApiRef,  pApiRef->strGIAcceptBuddy);
        pApiRef->strGIAcceptBuddy[0]=0;
        pApiRef->uGIAcceptTimeout = 0; //clear  it

        if (pBuddy != NULL)
        {
            _HLBBuddyChangeCallback(pApiRef->pLLRef,pBuddy->pLLBud, BUDDY_MSG_GAME_INVITE ,
                                         BUDDY_FLAG_GAME_INVITE_TIMEOUT,pApiRef);
            _signalBuddyListChanged ( pApiRef);  //changed a flag, so tell client.
        }
    }

    if ( pApiRef->eFakeAsyncOp != HLB_OP_NONE)
    {
        pApiRef->iLastOpState = HLB_ERROR_NONE; //no longer pending
        if (pApiRef->pOpProc )
        {
            pApiRef->pOpProc ( pApiRef, pApiRef->eFakeAsyncOp,pApiRef->iFakeAsyncOpCode,
                                    pApiRef->pOpData);  
            pApiRef->pOpProc = NULL; //clear, as it is per op only..
        }

        pApiRef->eFakeAsyncOp = HLB_OP_NONE; //clear the state
    }
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiSuspend

    \Description
      Suspend buddy api, cleanup and put buddy in passive state. Used when want to go into game .. 

    \Input *pApiRef    - reference pointer
*/
/*************************************************************************************************F*/
int32_t  HLBApiSuspend( HLBApiRefT  * pApiRef) 
{ 
    // Suspend goes thru even if not connected. This is because, if server disconnected
    // and went it game, we want to be in PASSive state, when server connects. If we 
    // are not, we will end up loading rosters, and game will very unhappy.

    // Setting Passive on Suspend assumes  mProd and Diff was set earlier..
    pApiRef->iPresBuddyState = BUDDY_STAT_PASS; 
    pApiRef->uGIAcceptTimeout = 0; //clear any t/o.
    if ( ! pApiRef->bMakeOffline)
    {   //will effectively suspend too..
        BuddyApiPresSend(pApiRef->pLLRef, "",  pApiRef->iPresBuddyState, 0,0);
    }
    
    // must change internal state, of buddy and xbfriends, regardless so  call Suspend too..
    BuddyApiSuspendXDK( pApiRef->pLLRef);

    // Remove the buddy roster and contained msgs  from memory
    _HLBRosterDestroy( pApiRef );

    // Remove the pre connected temporary buddy list if we still aren't connected...
    _preConnTempBudListDestroy( pApiRef );

    pApiRef->iStat = BUDDY_STAT_PASS; 
    return(HLB_ERROR_NONE);
}
 
/*F*************************************************************************************************/
/*!
    \Function HLBApiResume

    \Description
      Refresh roster  and put buddy in online state -- used  on return from game. 

    \Input *pApiRef     - reference pointer

    \Output
        void *          - the Roster held onto by this API. NULL if  errors.
*/
/*************************************************************************************************F*/

void * HLBApiResume(HLBApiRefT  * pApiRef)  
{
    
    // Allow a resume even if NOT connected, as need to clear PASSive state. If not,
    // will be in a bad state if suspend/resume happened when server had disconnected.

    pApiRef->iLastOpState = HLB_ERROR_NONE; //clear this, just in case was st
    pApiRef->iPresBuddyState = BUDDY_STAT_CHAT;

    //DONT clear other presence stuff--its up to client. What's set earlier, is still valid.   

    //Do roster before Presence, else, stored msgs will look like ones from temp buds and be limited to 1.  
    pApiRef->bRosterLoaded = 0;
    BuddyApiRoster(pApiRef->pLLRef , _HLBRosterCallback, pApiRef );

    //now, send presence, if user is not hiding in "offline" state--We got roster,so ask for NO REFRESH
    if ( ! pApiRef->bMakeOffline)
        BuddyApiPresSend( pApiRef->pLLRef, NULL, pApiRef->iPresBuddyState,  0, 1 ); //bDontRefreshRoster=1

    BuddyApiResumeXDK(pApiRef->pLLRef); //regardless of presence, make sure xdk wakes up..

    pApiRef->iStat = BUDDY_STAT_CHAT; //any other than PASSive
    return(_getRost( pApiRef)); // dangerous, as buddyapi can wipe this.
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiGetLastOpStatus

    \Description
    State of last operation --implies only one operation at  a time.  
    If operation is done (i.e. not pending), this will CLEAR the state. 

    \Input *pApiRef     - reference pointer

    \Output
        int32_t         - state of last operation. See HLB_ERROR_*. Returns HLB_ERROR_NOOP if nothing is outstanding.
*/
/*************************************************************************************************F*/

int32_t HLBApiGetLastOpStatus (HLBApiRefT  * pApiRef) 
{
    int32_t opState = (pApiRef)? pApiRef->iLastOpState:HLB_ERROR_NOOP ;
    if (( opState  != HLB_ERROR_PEND ) &&  (pApiRef) )
        pApiRef->iLastOpState = HLB_ERROR_NOOP;  //we are done,so reset.
    return(opState);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiCancelOp

    \Description
        Cancel any pending operations --clear pending state unconditionally. Will call operation
        callbacks with state of HLB_ERROR_CANCEL.

    \Input *pApiRef    - reference pointer
*/
/*************************************************************************************************F*/
void HLBApiCancelOp(HLBApiRefT  * pApiRef)
{
    if(pApiRef->pOpProc)
    {
        pApiRef->pOpProc (pApiRef, HLB_OP_ANY, HLB_ERROR_CANCEL, pApiRef->pOpData  );
        pApiRef->pOpProc = NULL;
    }
    pApiRef->iLastOpState =HLB_ERROR_CANCEL; //clear pending state unconditionally.
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiFindUsers

    \Description
         Request a search of users, of names starting with the pUserPattern. Min 3 chars.

    \Input *pApiRef     - reference pointer
    \Input *pName       - User name pattern to search. Min 3 characters, server add the wildcard.
    \Input *pOpCallback - Callback function to be called when search is completed.
    \Input *context     - User data or context that will be passed back to caller in the callback.

    \Output return code -- BUDDY_ERROR_NONE if request was sent to server.
*/
/*************************************************************************************************F*/
int32_t HLBApiFindUsers( HLBApiRefT  * pApiRef, const char * pName , HLBOpCallbackT * pOpCallback, void * context)
{
    int32_t rc;
    _setUpOp ( pApiRef, pName, pOpCallback, context);
    rc =  BuddyApiFindUsers( pApiRef->pLLRef ,  pName,_HLBOpCallback, pApiRef, 
                                                    pApiRef->op_timeout);
    _setLastOpState (pApiRef, rc);
    return(rc);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiUserFound

    \Description
        Return users found from FindUser request. Allows client to iterate list of returned users.  
        Should be called after callback from BuddyApiFindUsers has returned with a succes status.

    \Input *pApiRef     - reference pointer
    \Input iItem        -  Zero based item number in list.

    \Output BuddyApiUserT*  - user name and domain, null if  not responded yet, or item not there.
*/
/*************************************************************************************************F*/
BuddyApiUserT *HLBApiUserFound ( HLBApiRefT  * pApiRef , int32_t iItem)
{
    return(BuddyApiUserFound(pApiRef->pLLRef, iItem));
}


/*F*************************************************************************************************/
/*!
    \Function HLBApiRegisterNewMsgCallback

    \Description
        Register callback for new (chat) messages. Callback will be called whenever a chat message arrives. 
        Note that clients can optionally use polling instead of this callback mechanism.

    \Input *pApiRef         - reference pointer
    \Input *pMsgCallback    - Callback function to be called  a chat message arrives.
    \Input *pAppData        - User data or context that will be passed back to caller in the callback.

    \Output int32_t         - HLB_ERROR_NONE if success. BUDDY_ERROR_FULL if try to register more than one callback.

    \Notes
        Register callback to inform caller on any new message waiting
*/
/*************************************************************************************************F*/
int32_t HLBApiRegisterNewMsgCallback( HLBApiRefT  * pApiRef, HLBMsgCallbackT * pMsgCallback, void * pAppData )
{
    if ( (pApiRef->pMesgProc != NULL) && (pMsgCallback != NULL) )
    {
        HLBPrintf( ( pApiRef, "ERROR: Cannot have more than one msg callback"));
        return(HLB_ERROR_FULL); //caller must clear first -- only one supported.
    }
    pApiRef->pMesgProc = pMsgCallback;
    pApiRef->pMesgData = pAppData; // data for message callback
    return(HLB_ERROR_NONE);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiRegisterBuddyChangeCallback

    \Description
        Register callback for any buddy list (aka roster) change.

    \Input *pApiRef         - reference pointer
    \Input *pListCallback   - Callback function to be called  a chat message arrives.
    \Input *pAppData        - User data or context that will be passed back to caller in the callback.

    \Output int32_t         - HLB_ERROR_NONE if success. BUDDY_ERROR_FULL if try to register more than one callback.
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
        Register callback for any game invite status change  on buddies.  

    \Input *pApiRef         - reference pointer
    \Input *pActionCallback - Function called when game invite changes for a bud.
    \Input *pAppData        - User data or context that will be passed back to caller in the callback.  

    \Output
        int32_t             - HLB_ERROR_NONE if success. BUDDY_ERROR_FULL if try to register more than one callback.
*/
/*************************************************************************************************F*/
int32_t HLBApiRegisterGameInviteCallback( HLBApiRefT  * pApiRef, HLBBudActionCallbackT * pActionCallback, void * pAppData)
{
    if (( pApiRef->pGameInviteProc != NULL ) && ( pActionCallback != NULL) )
    {
        HLBPrintf( ( pApiRef, "ERROR: Cannot have more than one GI  callback"));
        return(HLB_ERROR_FULL); //caller must clear first -- only one supported.
    }
    pApiRef->pGameInviteProc = pActionCallback;
    pApiRef->pGameInviteData = pAppData; // data for   callback
    return(HLB_ERROR_NONE);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiRegisterBuddyPresenceCallback

    \Description
      Register callback for  when buddy presence changes--state or text can change. 

    \Input *pApiRef         - reference pointer
    \Input *pPresCallback   - function called when presence changes for a bud.
    \Input *pAppData        - user data or context that will be passed back to caller in the callback.

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

    \Input *pBuddy  - pointer to the buddy

    \Output
        char *      - the name
*/
/*************************************************************************************************F*/
char * HLBBudGetName(HLBBudT *pBuddy)
{
    return(pBuddy->pLLBud->strUser);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsTemporary

    \Description
        Indicates if the buddy is not persistent. Could be someone who sent me a message and is not on my buddy or ignore list.

    \Input *pBuddy - pointer to the buddy.

    \Output
        int32_t     -(boolean) 1 -- if buddy is temporary and hence not persistent.
*/
/*************************************************************************************************F*/
int32_t HLBBudIsTemporary(  HLBBudT *pBuddy )  
{
    //To be safe, if he is a bud, but is not regular or blocked, probably is 
    //temp, even tho flag not set .. 
    int32_t temp = 1; //( (pBuddy->pLLBud->uFlags  & BUDDY_FLAG_TEMP) ==0) ? 0:1 ;
    int32_t isBud = ( HLBBudIsIWannaBeHisBuddy (pBuddy) || HLBBudIsWannaBeMyBuddy (pBuddy) );
    isBud |= ( HLBBudIsRealBuddy(pBuddy) || HLBBudIsBlocked(pBuddy) );
    if (isBud )
        temp = 0;
    return(temp);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsBlocked

    \Description
        Indicates if messages from this 'buddy' is ignored or blocked by the server.  

    \Input *pBuddy - pointer to the buddy.

    \Output 
        int32_t     -(boolean) 1 -- if buddy is  ignored or blocked.
*/
/*************************************************************************************************F*/
int32_t HLBBudIsBlocked(HLBBudT *pBuddy)
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
        return(0);

    return(((pBuddy->pLLBud->uFlags & BUDDY_FLAG_IGNORE ) == 0) ? 0:1);
}

//  Has this buddy, has invited me to be his buddy. Local.
int32_t HLBBudIsWannaBeMyBuddy( HLBBudT  *pBuddy )
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
       return(0);

    return((pBuddy->pLLBud->uFlags &  BUDDY_FLAG_BUD_REQ_RECEIVED)?1:0 );
}

//  Have I invited this user to be my buddy. Local.
int32_t HLBBudIsIWannaBeHisBuddy( HLBBudT  *pBuddy )
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
       return(0);
     
    return(  (pBuddy->pLLBud->uFlags &  BUDDY_FLAG_BUD_REQ_SENT )? 1:0);
}

 /*F*************************************************************************************************/
/*!
    \Function HLBBudGetTitle

    \Description
       Return Title played name by this buddy, based on language set in Presence.

    \Input *pBuddy  - pointer to the buddy.

    \Output 
        char *      - The utf-8 encoded title name.

    \Notes
        NOTE: Depends on server, and should be tied to language set in presence.
        NOTE: Not clear of server implementation for PS2. Simply may not be there.
        NOTE: If user is not online, title will be "empty".
*/
/*************************************************************************************************F*/
char * HLBBudGetTitle( HLBBudT  *pBuddy)
{
    if ((!pBuddy) || (!pBuddy->pHLBApi))
       return(NULL);

    BuddyApiRefreshTitle(pBuddy->pHLBApi->pLLRef, pBuddy->pLLBud);

    return( (pBuddy->pLLBud->strTitle));
}

//! Is this buddy one of these temp buddy kinds -- can be true for permanent buddies too.
int32_t HLBBudTempBuddyIs(  HLBBudT  *pBuddy, int32_t tempBuddyType )
{
    int32_t rc =0;
    if ( ( pBuddy)  && (pBuddy->uTempBuddyFlags  & tempBuddyType ) !=0 ) 
        rc =1;
    return(rc);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsTemporary

    \Description
        Indicates if the buddy is really a buddy -- ie. not someone on the list due to being blocked, or added temporarily. 

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     -(boolean) 1 -- if buddy is a real buddy of mine.
*/
/*************************************************************************************************F*/
int32_t HLBBudIsRealBuddy( HLBBudT  *pBuddy )  
{
    if (pBuddy)
        return(((pBuddy->pLLBud->uFlags  & BUDDY_FLAG_BUDDY) ==0) ? 0:1);
    else 
        return(0);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsInGroup

    \Description
        Indicates if this buddy a member of group that's been set up to send msg. See HLBListAddToGroup.

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     -(boolean) 1 -- if buddy is in group.
*/
/*************************************************************************************************F*/
int32_t HLBBudIsInGroup(  HLBBudT  *pBuddy )
{
    return(pBuddy->groupId ==  HLB_GROUP_ID);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudGetState

    \Description
      Return buddy's state, such as is online, disconnected, in game etc. -- see HLB_Buddy_State_Enum.    

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     - Buddy's state -- see HLB_Buddy_State_Enum.
*/
/*************************************************************************************************F*/
int32_t HLBBudGetState(  HLBBudT  *pBuddy)
{
     if ((!pBuddy) || (!pBuddy->pLLBud))
        return(0);

    return(pBuddy->pLLBud->iShow); // maps same
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsPassive

    \Description
      Indicate if buddy is passive i.e. is online but playing a game.

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     - (boolean) -  1 if buddy is in passive state, i.e. playing a game.
*/
/*************************************************************************************************F*/
int32_t HLBBudIsPassive(HLBBudT *pBuddy)
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
        return(0);

    return(pBuddy->pLLBud->iShow ==  BUDDY_STAT_PASS);
}


/*F*************************************************************************************************/
/*!
    \Function HLBBudIsAvailableForChat

    \Description
      Indicate if buddy is online and available for chat.

    \Input *pBuddy  - pointer to the buddy.

    \Output 
        int32_t     - (boolean) -  1 if buddy is online and available for chat.
*/
/*************************************************************************************************F*/
int32_t HLBBudIsAvailableForChat (HLBBudT *pBuddy)
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
        return(0);

    return(pBuddy->pLLBud->iShow == BUDDY_STAT_CHAT);
}


/*F*************************************************************************************************/
/*!
    \Function HLBBudIsSameProduct

    \Description
      Indicate if buddy is logged into same game as I am. Buddy must be online.

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     - (boolean) -  1 if buddy is online and logged into same product.
*/
/*********************************************************************************************F*/
int32_t HLBBudIsSameProduct(HLBBudT *pBuddy)
{
    int32_t bSame = (pBuddy && ((pBuddy->pLLBud->uFlags  & BUDDY_FLAG_SAME)==0) ) ? 0:1 ;
#if HLBUDAPI_XBFRIENDS
    // if not same, could be offline -- check further on xbox, it could know..
    // only do the second check if client has NOT overriden this (for ntsc/pal case where title id is same but not same prod).
    if ((bSame == 0)  && (pBuddy->pHLBApi) && (pBuddy->pHLBApi->bDontUseXBLSameTitleIdCheck == 0)) 
    {
        bSame = (pBuddy && BuddyApiXboxBuddyIsSameTitle(pBuddy->pHLBApi->pLLRef, pBuddy->pLLBud));
    }
#endif
    return(bSame);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudGetPresence

    \Description
      Get the buddy's product presence string, as set by the other client. Will be
      converted to 8 bit from utf8, if a translation table is registered.

    \Input *pBuddy      - pointer to the buddy.
    \Input *strPresence - pointer to buffer to be filled with the buddy presence string
*/
/*********************************************************************************************F*/

void HLBBudGetPresence( HLBBudT  *pBuddy , char * strPresence)
{
    if (( pBuddy ) && (pBuddy->pHLBApi) && (pBuddy->pHLBApi->pTransTbl != NULL))
    { //registered a translater, to use it.
        Utf8TranslateTo8Bit(strPresence,HLB_PRESENCE_LEN,pBuddy->pLLBud->strPres,'*',
                  pBuddy->pHLBApi->pTransTbl);
    }
    else
    {
        ds_strnzcpy (strPresence,pBuddy->pLLBud->strPres, HLB_PRESENCE_LEN) ; 
    }
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudGetPresenceExtra

    \Description
      Get the buddy's extra presence string, as set by the other client. W
      Will be empty if product is not the same.

    \Input *pBuddy          - pointer to the buddy.
    \Input *strExtraPres    - pointer to the buffer to be filled with the buddy's extra presence string.
    \Input iStrSize         - buffer size
*/
/*********************************************************************************************F*/
void HLBBudGetPresenceExtra( HLBBudT  *pBuddy , char * strExtraPres, int32_t iStrSize )
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
        return;

    TagFieldDupl(strExtraPres, iStrSize,pBuddy->pLLBud->strPresExtra);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsNoReplyBud

    \Description
      Indicate if 'special' temp (i.e.admin)  buddy who WONT accept messages or invites etc.
      Generally means I got a message from a "system" user, and it wont take replies.

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     - (boolean) -  1 if buddy wont accept messages or invites.
*/
/*********************************************************************************************F*/
int32_t HLBBudIsNoReplyBud(HLBBudT *pBuddy)
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
        return(0);

    return(((pBuddy->pLLBud->uFlags  & BUDDY_FLAG_NOREPLY) ==0) ? 0:1);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudIsJoinable

    \Description
      Indicates if buddy can be "joined" (aka found and gone to)
      in an Xbox game.  Checks the Joinable bit in Xbox Presence.

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     - (boolean) -  1 if buddy wont accept messages or invites.
*/
/*********************************************************************************************F*/
int32_t HLBBudIsJoinable(HLBBudT *pBuddy)
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
        return(0);

    return(((pBuddy->pLLBud->uFlags & BUDDY_FLAG_JOINABLE) ==0) ? 0:1);
}


/*F*************************************************************************************************/
/*!
    \Function HLBBudGetVOIPState

    \Description
      Return the Voice Over IP state of the buddy -- plugged in or not etc.

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     - The buddy's presence voip state.   
*/
/*********************************************************************************************F*/
int32_t HLBBudGetVOIPState(HLBBudT *pBuddy)
{
    if ((!pBuddy) || (!pBuddy->pLLBud))
        return(0);

    return((int)pBuddy->pLLBud->uPresExFlags);
}


/*F*************************************************************************************************/
/*!
    \Function HLBBudCanVoiceChat

    \Description
      Indicates if buddy is ready to be talked to. i.e. plugged in, and in same product etc. 

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     - (boolean) -1- if plugged in and can be talked to.
*/
/*********************************************************************************************F*/
int32_t HLBBudCanVoiceChat(HLBBudT *pBuddy)
{
    return((HLBBudGetVOIPState(pBuddy) == HLB_VOIP_PLUGGED) && HLBBudIsSameProduct(pBuddy));
}

//Join buddy in another game; He MUST be online,joinable in other title. 
int32_t HLBBudJoinGame(HLBBudT *pBuddy, HLBOpCallbackT *cb , void * pContext)
{
    int32_t rc = 0;
    HLBApiRefT  *pApiRef;
    if (( pBuddy == NULL) || (pBuddy->pHLBApi==NULL ))
        return(HLB_ERROR_MISSING);

    if   ( ! HLBBudIsRealBuddy (pBuddy)  ||
        ! HLBBudIsJoinable (pBuddy)   ||
        HLBBudIsSameProduct (pBuddy) )
        return(HLB_ERROR_USER_STATE);

    pApiRef= pBuddy->pHLBApi;
    if ( _setUpOp (pApiRef,HLBBudGetName(pBuddy),cb,pContext) == NULL)
        return(HLB_ERROR_MISSING);

    rc = BuddyApiJoinGame (pApiRef->pLLRef,pBuddy->pLLBud
        ,_HLBOpCallback,pApiRef, pApiRef->op_timeout );
    _setLastOpState (pApiRef, rc);  
    if (rc == HLB_ERROR_NONE)
    {
        //Thanks to MS, this is now sync, but clients 
        //have a bird-- so fake it -- Will get picked up on next pump.
        pApiRef->eFakeAsyncOp = HLB_OP_INVITE;
        pApiRef->iFakeAsyncOpCode = rc;
        // _signalBuddyListChanged( pApiRef); //new flags set.
    }

    return(rc);
}
/* LIST functions */

//! Get message at index position, if read is true, mark message as read.

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetMsgByIndex

    \Description
      Return message from buddy, at index specified.  

    \Input *pBuddy      - pointer to the buddy.
    \Input index        - Sero based index position to read message from.
    \Input read         - int32_t - (boolean) - if true, message will be marked as read.

    \Output
        BuddyApiMsgT*   -  Message from buddy at that index pos. NULL if not there. 
*/
/*********************************************************************************************F*/
BuddyApiMsgT *HLBMsgListGetMsgByIndex(HLBBudT *pBuddy, int32_t index, int32_t read) // if true, mark msg as read.
{
    BuddyApiMsgT *pMsg;
    pMsg = _listItemGetByIndex ( pBuddy->msgList, index); 
    if ( !(pMsg) )
        return(NULL);
    if (read)
        pMsg->uUserFlags |= HLB_MSG_READ_FLAG;   //set the msg read flag.
    //else  //-- if read if false, should we clear it?
    //  pMsg->uUserFlags = pMsg->uUserFlags & ~HLB_MSG_READ_FLAG;
    return(pMsg);
}
/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetFirstUnreadMsg

    \Description
        Return unread message from buddy, starting from a start position.

    \Input *pBuddy      - pointer to the buddy.
    \Input startPos     - Zero based index position to start looking for unread messages.
    \Input read         - (boolean) if true, message will be marked as read.

    \Output
        BuddyApiMsgT*   - First unread  Message from buddy. NULL if no more. 
*/
/*********************************************************************************************F*/
BuddyApiMsgT *HLBMsgListGetFirstUnreadMsg(HLBBudT  *pBuddy, int32_t startPos, int32_t read) // if true, mark msg as read.
{
    int32_t i=0, max =0;
    BuddyApiMsgT *pMsg = NULL;

    if (pBuddy)
    {
        for ( i=startPos, max= _listGetCount ( pBuddy->msgList ); i<max; i++ )
        {
            pMsg = _listItemGetByIndex ( pBuddy->msgList , i );   
            if (( pMsg) && ( (pMsg->uUserFlags & HLB_MSG_READ_FLAG) == 0  ))  
            {
                if (read )
                    pMsg->uUserFlags |= HLB_MSG_READ_FLAG; //mark msg as being read..
                return(pMsg);
            }
        }
    }
    return(NULL); //if here, there is NO unread msg.
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetMsgText

    \Description
       Return the  BODY of message, text, converting to 8 bit chars from utf8 if needed.

    \Input *pApiRef - reference pointer
    \Input *pMsg    - pointer to the msg.
    \Input strText  - [out] Text buffer msg body will be written to.   
    \Input iLen     - Length of text buffer.
*/
/*********************************************************************************************F*/
void HLBMsgListGetMsgText(HLBApiRefT *pApiRef,BuddyApiMsgT *pMsg, char * strText, int32_t iLen)
{
    char strBuf[ 500];
    TagFieldGetString(TagFieldFind(pMsg->pData,"BODY"),strBuf,sizeof(strBuf),"Not Present");

    if (pApiRef->pTransTbl != NULL )
    {
        Utf8TranslateTo8Bit(strText,iLen,strBuf,'*',pApiRef->pTransTbl);
    }
    else
    {
        ds_strnzcpy ( strText, strBuf, iLen );
    }
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListMsgInject

    \Description
        Inject a message, as if from some buddy. Message callbacks etc will be called.
        If user is not on buddy list, he will be added as "temp" buddy.
        Convenience function, so client can inject lobby control and other messages, and 
        use same "buddy" UI to show such messages. 

    \Input *pApiRef         - reference pointer
    \Input iKind            - Type of msg inject (client can set, see clubapi.h for CLUB_KINDs.)
    \Input *pSenderName     - user, persona name of user on who's behalf msg is injected. Will appear as if "from" that user. May be null for admin message.
    \Input *pMsgBody        - The text body of message to inject.  
    \Input *pMsgParms       - Any "secret" parameters to send, grab using tag field HLB_MSGINJECT_PARMS_TAG
    \Input uMsgInjectFlags  - Flags to control features on msg, see  HLBMsgInjectE.
    \Input uTime            - time

    \Output
        int32_t             - return status, 0 if sucess, BLOCKED,FULL etc based on error condition.   
*/
/*********************************************************************************************F*/
int32_t HLBMsgListMsgInject ( HLBApiRefT  *pApiRef, int32_t iKind, const char *pSenderName,  const char *pMsgBody,
                              const char *pMsgParms, /* HLBMsgInjectE */uint32_t uMsgInjectFlags, uint32_t uTime)
{
    int32_t isAdmin = 0; //aka noreply =0
    const char *pUser = pSenderName;
    BuddyApiMsgT msg;

    char strMsg[BUDDY_EASO_BODYLEN +200];
    memset (&msg, 0, sizeof(msg));
    msg.iType =  HLB_MSG_TYPE_INJECT; //
    msg.iKind = iKind;

    if ((uMsgInjectFlags & HLB_MSGINJECT_AS_ADMIN_MSG) > 0)
    {
        isAdmin =1;
        if ((pUser==NULL ) || (strlen(pUser) == 0))
        {
            pUser = ADMIN_USER;
        }
    }
    TagFieldClear(strMsg, sizeof(strMsg));
    TagFieldSetString(strMsg, sizeof(strMsg), "BODY", pMsgBody);
    TagFieldSetString(strMsg, sizeof(strMsg), "USER", pSenderName);
    TagFieldSetString(strMsg, sizeof(strMsg), HLB_MSGINJECT_PARMS_TAG, pMsgParms);
    TagFieldSetNumber(strMsg, sizeof(strMsg), "TIME", uTime);
    TagFieldSetString(strMsg, sizeof(strMsg), "RSRC",  BuddyApiResource (pApiRef->pLLRef));
    msg.pData =strMsg;

    if (pApiRef->bRosterLoaded == 0)
    { //we can save msgs  till connected and have loaded roster..  
        _preConnMsgAdd (pApiRef, pUser,  &msg);
        return(HLB_ERROR_NONE);
    }

    if (!isAdmin)
    { //regular, and client wants us to honour blocked bud checks
        if (HLBBudIsBlocked( HLBListGetBuddyByName(pApiRef,pUser)))
        {
            return(HLB_ERROR_BLOCKED);
        }
    }
    _HLBMsgListAddMsg(pApiRef, pUser, &msg, isAdmin); //isAdmin --same as "no reply" bud..
    return(HLB_ERROR_NONE);
}


/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetUnreadCount

    \Description
      Return count of unread messages from buddy.

    \Input *pBuddy - pointer to the buddy.

    \Output
        int32_t - count of unread messages.
*/
/*********************************************************************************************F*/
int32_t HLBMsgListGetUnreadCount(HLBBudT *pBuddy)
{
    int32_t i=0, max =0, count=0;
    BuddyApiMsgT *pMsg;

    for ( i=0, max= _listGetCount ( pBuddy->msgList ); i<max; i++ )
    {
        pMsg = _listItemGetByIndex ( pBuddy->msgList , i );
        if (( pMsg) && ( (pMsg->uUserFlags & HLB_MSG_READ_FLAG) == 0  ))
        {
            count++;
        }
    }
    return(count);
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListGetTotalCount

    \Description
        Return total count of  messages, both read and unread, from buddy.

    \Input *pBuddy  - pointer to the buddy.

    \Output
        int32_t     - count of  messages.
*/
/*********************************************************************************************F*/
int32_t HLBMsgListGetTotalCount(HLBBudT *pBuddy)
{
    int32_t rc = (pBuddy) ? ( _listGetCount (pBuddy->msgList )): 0;
    return(rc);
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListDelete

    \Description
       Delete  message at ( zero based ) index position

    \Input *pBuddy  - pointer to the buddy.
    \Input index    - index
*/
/*********************************************************************************************F*/
void HLBMsgListDelete(HLBBudT *pBuddy, int32_t index)
{
    //If we are actually deleting a msg, decrement overall count..
    if ((_listItemGetByIndex ( pBuddy->msgList, index)!= NULL) && (pBuddy->pHLBApi ))
    {
        pBuddy->pHLBApi->msgCount--;
    }
    _listMsgItemDestroy ( pBuddy->pHLBApi, pBuddy->msgList , index);
}

/*F*************************************************************************************************/
/*!
    \Function HLBMsgListDeleteAll

    \Description
       Delete all, or just read msgs from this buddy. Local.
       If bZapReadOnly is true will delete just read msgs.

    \Input *pBuddy      - pointer to the buddy.
    \Input bZapReadOnly - int32_t (boolean) - if 1, will delete Read messages.
*/
/*********************************************************************************************F*/
void HLBMsgListDeleteAll(  HLBBudT  *pBuddy, int32_t bZapReadOnly   )
{
    int32_t count = 0;
    BuddyApiMsgT *pMsg;

    if ((pBuddy == NULL ) || ( pBuddy->msgList == NULL ))
        return;

        //backwards, so index does not change while zapping..
    for (count = _listGetCount( pBuddy->msgList)-1; count >= 0; --count)
    {
        pMsg = _listItemGetByIndex ( pBuddy->msgList , count );   
        if ( ( (pMsg) && (
                (!bZapReadOnly ) || ( (pMsg->uUserFlags & HLB_MSG_READ_FLAG) != 0  ))  ))
        {
            _listMsgItemDestroy ( pBuddy->pHLBApi, pBuddy->msgList , count );   
            pBuddy->pHLBApi->msgCount--;
        }
    }
}



/*F*************************************************************************************************/
/*!
    \Function _HLBMsgListDestroy

    \Description
       Free contents of msg list for this buddy.

    \Input *pBuddy - pointer to the buddy.
*/
/*********************************************************************************************F*/
void _HLBMsgListDestroy ( HLBBudT *pBuddy )
{
    HLBMsgListDeleteAll (pBuddy,0);  //OnlyUnread =0
    _listDestroy (pBuddy->msgList );
    pBuddy->msgList=0;
}

/*F*************************************************************************************************/
/*!
    \Function HLBListGetBuddyByName

    \Description
       Get a buddy from the buddy list (roster) by name. Exact match.

    \Input *pApiRef     - API Reference pointer.
    \Input *name        - name string.

    \Output HLBBudT*    - pointer to Buddy structure.  NULL if not there.
*/
/*********************************************************************************************F*/
HLBBudT*   HLBListGetBuddyByName(HLBApiRefT *pApiRef, const char * name)
{
    HLBBudT* pHLBBud = NULL;
    BuddyRosterT *pBuddy = NULL;
    if ( _getRost(pApiRef) != NULL)
    {
        pBuddy = BuddyApiFind( pApiRef->pLLRef , name);
        if ((pBuddy != NULL) && (pBuddy->pUserRef != NULL))
        {
            pHLBBud = pBuddy->pUserRef;
        }
    }
    return(pHLBBud);
}

/*F*************************************************************************************************/
/*!
    \Function HLBListGetBuddyByIndex

    \Description
       Get a Buddy by index into the buddy list.

    \Input *pApiRef     - API Reference pointer.
    \Input index        - zero based index.

    \Output HLBBudT*    - pointer to Buddy structure.  NULL if not there.
*/
/*********************************************************************************************F*/
HLBBudT* HLBListGetBuddyByIndex(HLBApiRefT  *pApiRef, int32_t index)
{
    HLBBudT* pHLBBud  = NULL;
    //memset (pBud, 0, sizeof(*pBud) );
    if (_getRost(pApiRef)  != NULL)
    {
        BuddyRosterT *pBuddy = (BuddyRosterT*) DispListIndex(_getRost(pApiRef) , index);
        if ( (pBuddy != NULL) &&  ( pBuddy->pUserRef != NULL ) )
        {
            pHLBBud =pBuddy->pUserRef ;  

        }
    }
    return(pHLBBud);
}

/*F*************************************************************************************************/
/*!
    \Function HLBListGetIndexByName

    \Description
       Given a buddy name, find the index into the list.

    \Input *pApiRef     - API Reference pointer.
    \Input *name        - name of the buddy to find from the local roster list.

    \Output int32_t     - index to buddy; negative value ( HLB_ERROR_MISSING ) if not there.
*/
/*********************************************************************************************F*/
int32_t HLBListGetIndexByName(HLBApiRefT *pApiRef, const char * name)
{
    // Go through all entries in the list to find the closest user
    int32_t index, max;
    HLBBudT* pHLBBud = NULL;
    for ( index=0, max = HLBListGetBuddyCount ( pApiRef); index <max; index++ )
    {
        pHLBBud = HLBListGetBuddyByIndex (pApiRef, index);
        if ( ( pHLBBud ) && (ds_stricmp(name, pHLBBud->pLLBud->strUser) == 0))
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

    \Input pApiRef  - API Reference pointer.

    \Output
        int32_t     - Total number of buddies.
*/
/***************************************************************************************************/
int32_t HLBListGetBuddyCount(HLBApiRefT *pApiRef)
{    //set all flags to 1, to count all buddies in the list.
    return(_countBuddies (pApiRef, (uint32_t)-1));
}

/*F***********************************************************************************************F*/
/*!
    \Function HLBListGetBuddyCountByFlags
    \Description
        Get count of buddies  that match the buddy (type) flags, like BUDDY, INVITE_SENT etc.
       .e.g. To count all permanent, but not blocked,  buddies, call with:
        flag=BUDDY_FLAG_BUDDY|BUDDY_FLAG_BUD_REQ_RECEIVED |BUDDY_FLAG_BUD_REQ_SENT

    \Input pApiRef      - API Reference pointer.
    \Input uTypeFlag    - bit mask of all buddy  types to look for.

    \Output
        int32_t         - total number of buddies that match  any of the type flags specified.
*/
/*********************************************************************************************F*/
int32_t HLBListGetBuddyCountByFlags(HLBApiRefT *pApiRef, uint32_t uTypeFlag)
{
    return(_countBuddies (pApiRef, uTypeFlag));
}

/*F*************************************************************************************************/
/*!
    \Function HLBListBlockBuddy

    \Description
       Ask that this buddy be ignored. Server will no longer send messages from this buddy.

    \Input *pApiRef     - API Reference pointer.
    \Input *name        - name pointer
    \Input *cb          - pointer to optional completion callback function
    \Input *context     - optional user context that will be passed to callback.

    \Output int32_t     - HLB_ERROR_NONE if command was sent.

    \Notes 
        Client can use callback, or, use polling to determine status of operation,
        as reported later by the server.
*/
/*********************************************************************************************F*/
int32_t HLBListBlockBuddy(HLBApiRefT  *pApiRef, const char * name , HLBOpCallbackT *cb , void * context)
{
    uint32_t   flag =  BUDDY_FLAG_IGNORE ; //block this dude
    HLBBudT  *pBuddy = HLBListGetBuddyByName ( pApiRef, name);
    if ((pBuddy )  && ( HLBBudIsNoReplyBud(pBuddy)))
    {
         return(HLB_ERROR_NOTUSER); //cannot invite or block such buddies.
    }

    //if doing the invite, you should use the blockinvite..
    //if   ( _stateIs ( pApiRef, name, HE_WANNABE_MY )  )
     //  return HLB_ERROR_USER_STATE;
    return(_HLBListAddChangeBuddy(pApiRef, name, flag ,0, cb, context)); //0 tempType=0
}

/*F*************************************************************************************************/
/*!
    \Function HLBListUnBlockBuddy

    \Description
        No longer Ignore this buddy. Server will no longer block messages from this buddy.

    \Input *pApiRef     - API Reference pointer.
    \Input *name        - name
    \Input *cb          - Pointer to optional completion callback function
    \Input *context     - Optional user context that will be passed to callback.

    \Output int32_t     - HLB_ERROR_NONE if command was sent.

    \Notes
        Client can use callback, or, use polling to determine status of operation,
        as reported later by the server. If this "buddy" was only Blocked, it will now be removed 
        from the local buddy list.
*/
/*********************************************************************************************F*/
int32_t HLBListUnBlockBuddy(HLBApiRefT *pApiRef, const char * name, HLBOpCallbackT *cb, void * context)
{
    uint32_t  flag =  BUDDY_FLAG_IGNORE; //no longer block
    return(_HLBListRemoveBuddy(pApiRef, name, flag , cb, context));
}

/*F*************************************************************************************************/
/*!
    \Function HLBListUnMakeBuddy

    \Description
        Ensure user is no longer a buddy of mine, by clearing "buddy" flag.
        Server operation. Will remain on buddy list if other (eg. temp or Ignore flags
        are set).

    \Input *pApiRef     - API Reference pointer.
    \Input *name        - buddy name.
    \Input *cb          - Pointer to optional completion callback function
    \Input *context     - Optional user context that will be passed to callback.

    \Output int32_t     - HLB_ERROR_NONE if command was sent.

    \Notes
        Client can use callback, or, use polling to determine status of operation,
        as reported later by the server.
*/
/*********************************************************************************************F*/
int32_t HLBListUnMakeBuddy(HLBApiRefT *pApiRef, const char * name, HLBOpCallbackT *cb, void * context)
{
    if (pApiRef->iStat == BUDDY_STAT_PASS )
        return(HLB_ERROR_USER_STATE);
    //cleanup in case a wannabe..
    HLBListDeleteTempBuddy ( pApiRef, name);
    //now do the real thing
    return(_HLBListRemoveBuddy(pApiRef, name, BUDDY_FLAG_BUDDY, cb, context));
}


//! Ask this use to be my buddy. Async. Will be added to my IWannabeHisBuddy list.
int32_t HLBListInviteBuddy( HLBApiRefT  *pApiRef, const char * name , HLBOpCallbackT *cb , void * pContext )
{
    BuddyRosterT llBud;
    int32_t rc;
    HLBBudT  *pBuddy = NULL;

    if (pApiRef->iStat == BUDDY_STAT_PASS )
        return(HLB_ERROR_USER_STATE);

    pBuddy = HLBListGetBuddyByName ( pApiRef, name);
    if ((pBuddy )  && ( HLBBudIsNoReplyBud(pBuddy)))
    {
        return(HLB_ERROR_NOTUSER); //cannot invite  or add such buddies.
    }

    //do error checks before setup op, if not callbacks get screwed up..

    if ( (pBuddy==NULL) || (HLBBudIsTemporary(pBuddy)) || (HLBBudIsBlocked(pBuddy)) )
    { // not in perma list --do we have room?
        if ( ! _canAddBuddy ( pApiRef, BUDDY_FLAG_BUDDY))
            return(HLB_ERROR_FULL);
    }

    memset (&llBud,0, sizeof (llBud));
    _setUpOp ( pApiRef, name, cb, pContext);
    ds_strnzcpy (llBud.strUser, name, sizeof (llBud.strUser));

    rc =  (BuddyApiBuddyInvite( pApiRef->pLLRef, &llBud, _HLBOpCallback, pApiRef, pApiRef->op_timeout));

    _setLastOpState (pApiRef, rc);
    return(rc);
}

//! Respond to an invite or change state of invite I sent, --applies to wannabe buddy invites 
int32_t HLBListAnswerInvite  (  HLBApiRefT  *pApiRef, const char * name , 
                                   /*HLBInviteAction_Enum */int32_t eInviteAction, 
                                           HLBOpCallbackT *cb , void * pContext )
{
    int32_t rc;
    BuddyRosterT *pBuddy;

    //if block, this is like blocking buddies, so check for space..
    if (eInviteAction == HLB_INVITEANSWER_BLOCK)
    {
        if (_countBuddies(pApiRef, BUDDY_FLAG_IGNORE) >= pApiRef->max_block_buddies)
            return(HLB_ERROR_FULL);
    }

    if ( (pBuddy = _setUpOp ( pApiRef, name, cb, pContext)) == NULL)
        return(HLB_ERROR_MISSING);

    //pHLBuddy = pBuddy->pUserRef; //save for delete
    // need to show action immediately, instead of waiting for server-RNOT msg, as it was too slow
    // This will also set/clear needed flags..
    rc = BuddyApiRespondBuddy( pApiRef->pLLRef,   pBuddy,   eInviteAction, _HLBOpCallback ,  pApiRef, pApiRef->op_timeout);
    _setLastOpState (pApiRef, rc);

    //NOT needed, del callback will free the mem.
    //did he get deleted from list? if so make sure to free our mem too.
    //if (BuddyApiFind ( pApiRef->pLLRef, name) == NULL )
     //    _HLBDestroyBuddy ( pHLBuddy );
    return(rc);
}

//! Respond to an invite or change state of invite i sent, --applies to game invites
int32_t HLBListAnswerGameInvite(HLBApiRefT  *pApiRef, const char * name,
                                    /*HLBInviteAnswer_Enum */int32_t eInviteAction, 
                                            HLBOpCallbackT *cb , void * pContext)
{
    BuddyRosterT *pBuddy;
    int32_t rc =0;
    if (    eInviteAction  == HLB_INVITEANSWER_REVOKE )  
    { //revoking, so cancel accept timer
            pApiRef->uGIAcceptTimeout = 0;
            pApiRef->strGIAcceptBuddy[0]=0;
    }

    if (( pBuddy = _setUpOp ( pApiRef, name, cb, pContext)) == NULL)
        return(HLB_ERROR_MISSING);

    rc = BuddyApiRespondGame( pApiRef->pLLRef, pBuddy,eInviteAction, 
                            _HLBOpCallback, pApiRef, pApiRef->op_timeout,pApiRef->uGISessionId);
    _setLastOpState (pApiRef, rc);  
#if HLBUDAPI_XBFRIENDS
    //Thanks to MS, this is now sync, but clients 
    //have a bird-- so fake it -- Will get picked up on next pump.
    pApiRef->eFakeAsyncOp = HLB_OP_INVITE;
    pApiRef->iFakeAsyncOpCode = rc;
#endif
    _signalBuddyListChanged( pApiRef); //new flags set
    return(rc);
}


//! Cancel any game invites I sent -- also see HLBListAnswerGameInvite
int32_t HLBListCancelGameInvite(HLBApiRefT *pApiRef, HLBOpCallbackT *cb , void * pContext)
{
    int32_t rc=0;
    int32_t idx=0, val =0;
    HLBBudT *pBuddy;
    for (idx = 0, val = HLBListGetBuddyCount ( pApiRef);  idx < val; ++idx)
    {
        pBuddy = HLBListGetBuddyByIndex (  pApiRef, idx );
        if ((pBuddy) && ( (pBuddy->pLLBud->uFlags & BUDDY_FLAG_GAME_INVITE_SENT)  >0) )
        {
           rc =HLBListAnswerGameInvite  (  pApiRef, HLBBudGetName (pBuddy), 
                                    HLB_INVITEANSWER_REVOKE, cb , pContext );
        }
    }
    //pApiRef->uGISessionId++; // ready for next one..
    return(rc);
}

//Invite buddy to game, he MUST be in another title. Async.

int32_t HLBListGameInviteBuddy( HLBApiRefT  *pApiRef, const char * name , const char * pGameInviteState,
                                        HLBOpCallbackT *cb , void * pContext )
{
     BuddyRosterT *pBuddy;
    int32_t rc = 0;
    
    if (pApiRef->iStat == BUDDY_STAT_PASS )
        return(HLB_ERROR_USER_STATE);

    if   ( !HLBBudIsRealBuddy (  HLBListGetBuddyByName (pApiRef, name))) 
        return(HLB_ERROR_USER_STATE);
      //TODO check if NOT playing my game
    if (pApiRef->uGIAcceptTimeout != 0 ) //outstanding game accepted, no more
        return(HLB_ERROR_PEND);
        
     if ( (pBuddy = _setUpOp ( pApiRef, name, cb, pContext)) == NULL)
         return(HLB_ERROR_MISSING);

     rc = BuddyApiGameInvite ( pApiRef->pLLRef,  pBuddy, _HLBOpCallback, pApiRef, 
          pApiRef->op_timeout, pApiRef->uGISessionId, pGameInviteState );
     _setLastOpState (pApiRef, rc); 
#if HLBUDAPI_XBFRIENDS
     //Thanks to MS, this is now sync for xbox, but clients 
    //have a bird-- so fake it -- Will get picked up on next pump.
     pApiRef->eFakeAsyncOp = HLB_OP_INVITE;
     pApiRef->iFakeAsyncOpCode = rc;
#endif
     _signalBuddyListChanged( pApiRef); //new flags set.
     return(rc);
}



/*F*************************************************************************************************/
/*!
    \Function HLBListFlagTempBuddy

    \Description
        Add as a tourney or other  temp buddy for convenience. Sync, local call.
        If buddy already exists as a "permanent" buddy, then flag is set, Otherwise, buddy
        is added to the list as a "temp" buddy.

    \Input *pApiRef     - module reference
    \Input *name        - Buddies name.
    \Input buddyType    - the TempBuddyType (see enum)

    \Output
        int32_t         - HLB_ERROR_NONE if ok.
*/
/*********************************************************************************************F*/
int32_t HLBListFlagTempBuddy(HLBApiRefT *pApiRef, const char * name, int32_t buddyType)
{
    if (pApiRef == NULL)
        return(HLB_ERROR_MISSING);

    if (pApiRef->iStat == BUDDY_STAT_PASS )
        return(HLB_ERROR_USER_STATE);
    return(_HLBListAddChangeBuddy(pApiRef, name, BUDDY_FLAG_TEMP, buddyType, NULL, NULL));
}

/*F*************************************************************************************************/
/*!
    \Function HLBListUnFlagTempBuddy

    \Description
      Clear the particular temp buddy flag if set.
      Does not remove from list -- client must explicitly deleteAdd as a tourney or other  temp buddy for convenience. Sync, local call.

    \Input *pApiRef         - module reference
    \Input *name            - Buddies name
    \Input *tempBuddyType   - the TempBuddyType flag to clear (see enum)

    \Output
        int32_t             - HLB_ERROR_NONE if ok.
*/
/*********************************************************************************************F*/
 
int32_t HLBListUnFlagTempBuddy ( HLBApiRefT  *pApiRef , const char * name,  int32_t tempBuddyType )
{
    HLBBudT *pBuddy;

    if (pApiRef->iStat == BUDDY_STAT_PASS )
        return(HLB_ERROR_USER_STATE);

    pBuddy = HLBListGetBuddyByName ( pApiRef, name);
    if (pBuddy)
    {
        uint16_t uOldFlag   = pBuddy->uTempBuddyFlags ;
        pBuddy->uTempBuddyFlags &= ~(tempBuddyType) ; //clear the flag..
        if ( (uOldFlag & tempBuddyType ) !=0 )  //flag was set, so we did clear..
            _signalBuddyListChanged ( pApiRef); //did change so tell caller --EAC gui needs this.
    }
    return(HLB_ERROR_NONE);
}


/*F*************************************************************************************************/
/*!
    \Function HLBListDeleteTempBuddy

    \Description
      Explicitly delete this temp buddy. Will ignore request 
      if buddy is permanent. local call.

    \Input *pApiRef     - module reference
    \Input *name        - buddy name.

    \Output
        int32_t         - HLB_ERROR_NONE if ok.
*/
/*********************************************************************************************F*/
 int32_t HLBListDeleteTempBuddy(HLBApiRefT *pApiRef, const char * name)
 {
    if (pApiRef->iStat == BUDDY_STAT_PASS )
        return(HLB_ERROR_USER_STATE);
    return(_HLBListRemoveBuddy(pApiRef, name, BUDDY_FLAG_TEMP, NULL, NULL));
 }


//! Sort function to be set if client wants custom sort of buddy list -- However API will have a reasonable default sort.
void HLBListSetSortFunction(HLBApiRefT * pApiRef, HLBRosterSortFunctionT *sortCallbackFunction)
{
    pApiRef->pBuddySortFunc = sortCallbackFunction;
}

//! Sort list of buddies. Once sorted, list will be frozen until re-sorted, or provide a freeze() call.
int32_t HLBListSort(HLBApiRefT *pApiRef)                          //sort buddy  list using sort function.
{
    if ( _getRost(pApiRef) != NULL)
    {
        DispListSort( _getRost(pApiRef) ,  pApiRef , 0,  pApiRef->pBuddySortFunc);
        // Force the list to appear to be changed so it will get resorted.
        DispListChange( _getRost(pApiRef) , 1);
        // Resort the list.
        DispListOrder( _getRost(pApiRef) );
    }
    return(BUDDY_ERROR_NONE);
}


//! Has the  list of  buddies  or their presence changed? Will clear the flag. Local.
int32_t HLBListChanged(HLBApiRefT *pApiRef)
{
    int32_t rc = HLB_NO_CHANGES;
    if (pApiRef->rosterDirty)
    {                                   ////(DispListOrder( pApiRef->pRoster )
        pApiRef->rosterDirty = false;
        rc = HLB_CHANGED;
    }
    return(rc);
}

//! Has messages from any buddies changed? Returns buddy index, or HLB_NO_CHANGES.
int32_t HLBListBuddyWithMsg(HLBApiRefT *pApiRef)
{
    int32_t idx, val =0;
    int32_t budIdx = HLB_NO_CHANGES;
    HLBBudT *pBuddy;
    for (idx = 0, val = HLBListGetBuddyCount ( pApiRef );  idx < val; ++idx)
    {
        pBuddy = HLBListGetBuddyByIndex (  pApiRef, idx );
        if ((pBuddy) && (pBuddy->msgListDirty))
        {
            pBuddy->msgListDirty  = false;
            budIdx =idx;
            break;
        }
    }
    return(budIdx);
}


//! On closing screen, need to clear temp buddies w/o messages etc.. 
#if 0 // unused
static int32_t _HLBListDeleteTempBuddies(HLBApiRefT *pApiRef)   //todo --implement if needed.
{
    return(HLB_ERROR_NONE);
}
#endif

// Can create a list of buddies, to send a message to. 
// Do we need to support more than one group? -- If not drop groupId param or  use group id =1.
// Note: this is not the same as groups in Messenger -- production calls it groups 

//! Add user to group to whom i will send a common msg.
int32_t HLBListAddToGroup(HLBApiRefT *pApiRef, const char * buddyName)
{
    int32_t rc = HLB_ERROR_MISSING;
    HLBBudT *pBuddy;

    if ( (pBuddy = HLBListGetBuddyByName ( pApiRef, buddyName  )) !=  NULL )
    {
        pBuddy->groupId = HLB_GROUP_ID; 
        rc = HLB_ERROR_NONE;
    }
    return(rc);
}

//! Remove this buddy from the group Iam going to send messages to.
int32_t HLBListRemoveFromGroup(HLBApiRefT *pApiRef, const char * buddyName)
{
    int32_t rc = HLB_ERROR_MISSING;
    HLBBudT *pBuddy;
    if ( ( pBuddy = HLBListGetBuddyByName( pApiRef, buddyName  )) != NULL )
    {
            pBuddy->groupId =0;
            rc = HLB_ERROR_NONE;
    }
    return(rc);
}

//! Clear group of users so I can re-use it.
int32_t HLBListClearGroup(HLBApiRefT *pApiRef)
{
    int32_t idx=0, val=0;
    HLBBudT *pBuddy;
    for (idx = 0, val = HLBListGetBuddyCount ( pApiRef );  idx < val; ++idx)
    {
        if ( ( pBuddy = HLBListGetBuddyByIndex (  pApiRef, idx )) != NULL )
        {
            pBuddy->groupId =0;
        }
    }
    return(HLB_ERROR_NONE);
}

//! Send message to group of users I created earlier. Reset group if clearGroup is set.
int32_t HLBListSendMsgToGroup (HLBApiRefT  * pApiRef,   /*HLBMsgType_Enum*/ int32_t msgType , const char * msg , int32_t bClearGroup,
                                        HLBOpCallbackT *cb , void * pContext)
{
    int32_t rc =HLB_ERROR_MISSING;
    int32_t  idx =0, val = 0;
    HLBBudT *pBuddy;

    char usersBuff [ MAX_BRDC_USERS_LEN +1 ]; 
    char *buffPtr= usersBuff;
    int32_t iLeft=  MAX_BRDC_USERS_LEN;
    int32_t iNeed=0;
    char strResource[128];
    char * pDiv;
    *buffPtr =0;


    if (HLBApiGetConnectState (pApiRef)  != HLB_CS_Connected )
        return(HLB_ERROR_OFFLINE);

    //add resource or domain if aint there..Ensures we wont send to to users  in other domains.
    //Just get the first part of the resource -- the domain --or else wont go to other games.
    ds_strnzcpy(strResource, BuddyApiResource(pApiRef->pLLRef), sizeof(strResource)); // add resource --check for / tho

    // locate the second divider  
    if ((pDiv = strchr(strResource + 1, '/'))  != NULL)
        *pDiv =0;

    for (idx = 0, val = HLBListGetBuddyCount ( pApiRef);  idx < val; ++idx)
    {
        if ( ( pBuddy = HLBListGetBuddyByIndex (  pApiRef, idx )) != NULL ) 
        {
            if ( pBuddy->groupId ==  HLB_GROUP_ID )
            {   
                iNeed = (int32_t)strlen ( pBuddy->pLLBud->strUser );
                //need space for adding a resource too?
                iNeed += (strchr ( pBuddy->pLLBud->strUser , '/') == 0 ) ?   (int32_t)strlen ( strResource ):0;
                iLeft -=  iNeed;
                if (iLeft <=1 )
                { //full --send it
                    *--buffPtr=0; //zap last , and, now send this one out..
                    rc = HLBListSendChatMsg ( pApiRef,usersBuff,msg,cb,pContext);
                    buffPtr= usersBuff;
                    iLeft=MAX_BRDC_USERS_LEN -iNeed;
                }
                
                strcpy (buffPtr, pBuddy->pLLBud->strUser);
                buffPtr += strlen ( pBuddy->pLLBud->strUser );

                if (strchr ( pBuddy->pLLBud->strUser , '/') == 0 )
                { //add resource name, if its not there..
                    strcpy (buffPtr, strResource);
                    buffPtr += strlen ( strResource );
                }
                *buffPtr++ =','; //add comma seperator..
                iLeft--;
                if (  bClearGroup )
                    pBuddy->groupId =0;
            }
        }
    }

    if   ( buffPtr > usersBuff)  
    { //have more users -- send msg
        *--buffPtr=0; //remove last comma
        rc = HLBListSendChatMsg ( pApiRef,usersBuff,msg,cb,pContext);
        //not an "operation" so dont set the opState...
    }
    return(rc);
}

//! Set presence string for other products.
void HLBApiPresenceDiff(HLBApiRefT *pApiRef, const char * globalPres)
{
    //"en" is the legacy presence lang.
    BuddyApiPresDiff( pApiRef->pLLRef,  globalPres , "en"); 
}
//! Set presence my presence state and presence string for this product
void HLBApiPresenceSame(HLBApiRefT *pApiRef, int32_t iPresState,const char * strSamePres)
{
    BuddyApiPresSame( pApiRef->pLLRef,  strSamePres);
    pApiRef->iPresBuddyState = iPresState;
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiPresenceJoinable

    \Description
      Set or reset and optionally send presence related to this Joinable state for Xbox.

    \Input pApiRef          - Pointer to this api.
    \Input bOn              - 1 if want joinable on, 0 if want it off.
    \Input bSendImmediate   - 1 will send it now, 0 if client will call HLBApiPresenceSend later

    \Output
        int32_t             - HLB_ERROR_NONE if ok, HLB_ERROR_OFFLINE  in offline.  
*/
/*********************************************************************************************F*/
int32_t HLBApiPresenceJoinable(HLBApiRefT *pApiRef, int32_t bOn, int32_t bSendImmediate)
{
    int32_t rc = HLB_ERROR_NONE;
    BuddyApiPresJoinable( pApiRef->pLLRef,  bOn);
    if (bSendImmediate)
    {
        rc =BuddyApiPresSend( pApiRef->pLLRef, NULL, pApiRef->iPresBuddyState,  0,0 ); 
    }
    return(rc);
}


//set "extra"  presence for scores etc, must call HLBApiPresenceSend to send.
void HLBApiPresenceExtra(HLBApiRefT *pApiRef, const char * strExtraPres)
{
    BuddyApiPresExtra (pApiRef->pLLRef,   strExtraPres);
}

//!Set presence as "Offline" if OnOff is true.Reset if bOnOff false. Not Persistent.
int32_t  HLBApiPresenceOffline(HLBApiRefT *pApiRef, int32_t bOnOff)
{
    // make sure we are connected to the buddy server
    if (BuddyApiStatus(pApiRef->pLLRef) != BUDDY_SERV_ONLN)
    {
        return(0);
    }
    if (bOnOff)
    { //make offline...
        pApiRef->bMakeOffline = true;
        return((BuddyApiPresSend(pApiRef->pLLRef, NULL, BUDDY_STAT_DISC, 0, 0)));
    }
    pApiRef->bMakeOffline = false;
    return(BuddyApiPresSend(pApiRef->pLLRef, NULL, pApiRef->iPresBuddyState, 0, 0));
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiPresenceSend

    \Description
        Send presence msg, with string for this game and others. 

    \Input *pApiRef      - the pointer to this API
    \Input iBuddyState   - the chat state of this buddy. See HLBStatE.
    \Input iVOIPState    - the headset status to send. Set to -1 if dont want to change it. VOIPState may be changed independantly, see HLBApiPresenceVOIPSend.
    \Input gamePres      - the presence string for other in this (same) game to see.
    \Input waitMsecs     - time to wait before sending, in msecs. 0 means send now.

    \Output
        int32_t         - HLB_ERROR_NONE if ok, HLB_ERROR_OFFLINE  regular presence not yet set.  
*/
/*********************************************************************************************F*/
int32_t HLBApiPresenceSend(HLBApiRefT *pApiRef, /*HLBStatE*/ int32_t iBuddyState , int32_t iVOIPState, const char * gamePres, int32_t waitMsecs)
{
    if (HLBApiGetConnectState (pApiRef)  != HLB_CS_Connected ) 
    {
        return(HLB_ERROR_OFFLINE);
    }
     
    BuddyApiPresSame( pApiRef->pLLRef,  gamePres );
    if (iVOIPState != -1 )
    {
        BuddyApiPresExFlags( pApiRef->pLLRef,  iVOIPState  );
    }
    pApiRef->iPresBuddyState = iBuddyState; //save it for later too.
    if ( ! pApiRef->bMakeOffline)
    {
        BuddyApiPresSend( pApiRef->pLLRef, NULL, pApiRef->iPresBuddyState,  waitMsecs,0 );
    }
    return(HLB_ERROR_NONE);
}
 
/*F*************************************************************************************************/
/*!
    \Function HLBApiPresenceVOIPSend

    \Description
      Set and send presence related to this user's voip (i.e. headset) status. Must be online.

    \Input *pApiRef     - Pointer to this api.
    \Input iVOIPState   - The headset status to send.

    \Output
        int32_t         - HLB_ERROR_NONE if ok, HLB_ERROR_OFFLINE  in offline.
*/
/*********************************************************************************************F*/
int32_t HLBApiPresenceVOIPSend(HLBApiRefT *pApiRef, /*HLBVOIPE*/ int32_t iVOIPState)
{
    // cache state, unconditionally, in case we are offline.
    BuddyApiPresExFlags(pApiRef->pLLRef, iVOIPState);   //cache value for later

    if (HLBApiGetConnectState (pApiRef)  != HLB_CS_Connected ) 
    {
        return(HLB_ERROR_OFFLINE);
    }

    if ( ! pApiRef->bMakeOffline)
    {
        BuddyApiPresSend(pApiRef->pLLRef,NULL,pApiRef->iPresBuddyState,0,0);
    }
    return(HLB_ERROR_NONE);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiPresenceSendSetPresence

    \Description
      Send to server, previously set presence state and presence strings.

    \Input *pApiRef     - Pointer to this api.

    \Output int32_t     - HLB_ERROR_NONE if ok, HLB_ERROR_OFFLINE  in offline.
*/
/*********************************************************************************************F*/
int32_t HLBApiPresenceSendSetPresence( HLBApiRefT *pApiRef)
{
     //must be online,
    if (HLBApiGetConnectState (pApiRef)  != HLB_CS_Connected ) 
    {
        return(HLB_ERROR_OFFLINE);
    }
    if ( ! pApiRef->bMakeOffline)
    {
        BuddyApiPresSend(pApiRef->pLLRef,NULL,pApiRef->iPresBuddyState,0,0);
    }

    return(HLB_ERROR_NONE);
}

//! Send an actual chat message.
int32_t HLBListSendChatMsg(HLBApiRefT *pApiRef, const char * name, const char * msg,
                           HLBOpCallbackT *cb, void * pContext)
{
    char strMsg [ HLB_ABSOLUTE_MAX_MSG_LEN + MAX_BRDC_USERS_LEN + 256];
    char strBody [ HLB_ABSOLUTE_MAX_MSG_LEN +1];
    char strTemp [128];
    int32_t rc;
    char * pDiv;

    HLBBudT  *pBuddy = HLBListGetBuddyByName ( pApiRef, name);
    if ((pBuddy )  && ( HLBBudIsNoReplyBud(pBuddy)))
    {
         return(HLB_ERROR_NOTUSER); //cannot msg or invite such buddies.
    }

    if (HLBApiGetConnectState (pApiRef)  != HLB_CS_Connected )
        return(HLB_ERROR_OFFLINE);
  
    // Assemble the message (Note: timestamp is inserted by the server)
    TagFieldClear(strMsg, sizeof(strMsg));
    TagFieldSetString(strMsg, sizeof(strMsg), "TYPE", "C"); // goes as C comes at 'chat'. 
    if (strchr ( name, '/') == 0 )
    { 
        //add resource or domain if aint there..Ensures we wont send to to users  in other domains.
        //Just get the first part of the resource -- the domain --or else wont go to other games..
        if (BuddyApiResource (pApiRef->pLLRef)[0]=='/' )
            ds_strnzcpy(strTemp, BuddyApiResource (pApiRef->pLLRef)+1, sizeof(strTemp));
        else
            ds_strnzcpy(strTemp, BuddyApiResource (pApiRef->pLLRef), sizeof(strTemp));   // add resource --check for / tho
        // locate the second divider  
        if ( ( pDiv = strchr(strTemp, '/'))  != NULL)
            *pDiv =0;

        ds_snzprintf (strBody, sizeof(strBody), "%s/%s", name, strTemp);

        TagFieldSetString(strMsg, sizeof(strMsg), "USER", strBody);
    }
    else
        TagFieldSetString(strMsg, sizeof(strMsg), "USER", name);

    ds_strnzcpy (strBody, msg, pApiRef->max_msg_len );
    strBody[ pApiRef->max_msg_len] =0; //in case was truncated.
    TagFieldSetString(strMsg, sizeof (strMsg) , "BODY", strBody );
    TagFieldSetNumber(strMsg, sizeof(strMsg), "SECS", HLB_MSG_KEEP_SECS);
    _setUpOp ( pApiRef, name, cb, pContext); //save their c/b if any.
    if  (strchr( name, ',') == 0 )
        rc = BuddyApiSend( pApiRef->pLLRef , strMsg,  _HLBOpCallback, pApiRef, pApiRef->op_timeout);
    else 
        rc = BuddyApiBroadcast ( pApiRef->pLLRef , strMsg, _HLBOpCallback, pApiRef, pApiRef->op_timeout);

    if (cb)
    { //change state, so msg response recd later,will be tracked
        pApiRef->bTrackSendMsgInOpState =1;
    }
    if (pApiRef->bTrackSendMsgInOpState )
        _setLastOpState (pApiRef, rc); //make track of msg sends 
    return(rc);
}

/*F*************************************************************************************************/
/*!
    \Function HLBApiSetEmailForwarding

    \Description
      Turn email forwarding on or off, and set email address.

    \Input *pApiRef     - ptr to API State.  
    \Input bEnable      - turn forwarding on or off . 1 is on.
    \Input *strEmail    - the email address to set it to. Will be set even if bEnable is off, due to server issue.
    \Input *pOpCallback - optional, callback to get status of operation. Can also poll LastOpStatus.
    \Input *pCalldata   - optional, user context for callback.

    \Output
        int32_t         - HLB_ERROR_NONE if ok, HLB_ERROR_PEND if some other operation is pending.
*/
/*********************************************************************************************F*/
int32_t HLBApiSetEmailForwarding(HLBApiRefT *pApiRef, int32_t bEnable, const char *strEmail, HLBOpCallbackT *pOpCallback, void *pCalldata)
{
    int32_t rc;
    uint32_t  uForwardingFlags = 0;
    if (HLBApiGetConnectState (pApiRef)  != HLB_CS_Connected ) 
    {
        return(HLB_ERROR_OFFLINE);
    }
    if (  pApiRef->iLastOpState == HLB_ERROR_PEND )
        return(HLB_ERROR_PEND);

    //save the clients callback, if any.
    if (( pOpCallback ) )
    {
            pApiRef->pOpProc = pOpCallback;
            pApiRef->pOpData = pCalldata;
    }

    if ( bEnable ==1 )
        uForwardingFlags |= BUDDY_FORWARD_FLAG_EMAIL ;

    rc = BuddyApiSetForwarding(  pApiRef->pLLRef,   uForwardingFlags,  strEmail,
                                        _HLBOpCallback, pApiRef, pApiRef->op_timeout ); 

    //this is an "Operation", set the pending flag if succeeded and all that..
    _setLastOpState (pApiRef, rc); 
    return(rc);
}


/*F*************************************************************************************************/
/*!
    \Function HLBApiGetEmailForwarding

    \Description
        Get  email forwarding state, if already loaded from server. 

    \Input *pApiRef     - ptr to API State.  
    \Input *bEnable     - Returned enable flag. Ignore if return code is "pending".
    \Input *strEmail    - The email address.

    \Output
        int32_t         - HLB_ERROR_NONE if ok, HLB_ERROR_PEND if not loaded yet.
*/
/*********************************************************************************************F*/
int32_t HLBApiGetEmailForwarding(HLBApiRefT *pApiRef, int32_t *bEnable, char * strEmail)
{
    uint32_t  flags=0;

    int32_t rc = BuddyApiGetForwarding(  pApiRef->pLLRef,   &flags,  strEmail );
    *bEnable = 0;
    if ( (flags & BUDDY_FORWARD_FLAG_EMAIL ) != 0 )
        *bEnable = 1;

    if (rc !=0)
        return(HLB_ERROR_PEND); //try later buckaroo.

    return(HLB_ERROR_NONE);
}

//Get the state of Cross Game invites --Xbox only!
uint32_t  HLBBudGetGameInviteFlags(HLBBudT *pBuddy)
{
    uint32_t  flag =0;
    flag = ( pBuddy->pLLBud->uFlags &   BUDDY_MASK_GAME_INVITE );
    
    // fire once flag, so clear it.
    if ((flag & HLB_FLAG_GAME_INVITE_TIMEOUT ) > 0 )
    {
        //now that we told client, get both sides to clear things up...
        //HLBPrintf( (pApiRef, "HLB_FLAG_GAME_INVITE_TIMEOUT set, will now be cleared\n");
        pBuddy->pLLBud->uFlags &= ~BUDDY_MASK_GAME_INVITE;
        //TODO --clear far side after a t/out.
        //HLBListAnswerGameInvite( pApiRef, HLBBudGetName (pBuddy), HLB_INVITEANSWER_REVOKE,0,0);
    }
    return(flag);
}

//!Get the " Official" Xbox title name for a given title based on my locale-language
int32_t HLBApiGetTitleName(HLBApiRefT *pApiRef, const uint32_t uTitleId, const int32_t iBufSize, char *pTitleName)
{
    return(BuddyApiGetTitleName(pApiRef->pLLRef, uTitleId, iBufSize, pTitleName));
}

//!Get the " Official" Title name for this title. pLang ignored -- defaults to English. 
int32_t HLBApiGetMyTitleName(HLBApiRefT *pApiRef, const char *pLang, const int32_t iBufSize, char *pTitleName)
{
    return(BuddyApiGetMyTitleName(pApiRef->pLLRef, pLang, iBufSize, pTitleName));
}

// Set the SessionID that is passed to any future game invites
int32_t HLBListSetGameInviteSessionID(HLBApiRefT *pApiRef, const char *sSessionID)
{
    return(BuddyApiSetGameInviteSessionID(pApiRef->pLLRef, sSessionID));
}

// Get the SessionID for a particular user -- comes with  game invites
int32_t HLBListGetGameSessionID(HLBApiRefT *pApiRef, const char *name, char *sSessionID, const int32_t iBufSize)
{
    return(BuddyApiGetUserSessionID(pApiRef->pLLRef, name, sSessionID, iBufSize));
}

//secret debug for getting low level bud..
BuddyApiRefT * _HLBApiGetXXX(HLBApiRefT *pApiRef)
{
    return((pApiRef)?pApiRef->pLLRef:0);
}

// set the Enumerator index used only by xbox360
int32_t HLBApiSetUserIndex(HLBApiRefT *pApiRef, int32_t iEnumeratorIndex)
{
    return(0);
}

// Get the Enumerator index used only by xbox360 
int32_t HLBApiGetUserIndex(HLBApiRefT *pApiRef)
{
    return(0);
}
