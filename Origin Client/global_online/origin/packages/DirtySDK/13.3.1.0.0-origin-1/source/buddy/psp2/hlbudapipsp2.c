/*H*************************************************************************************/
/*!
    \File hlbudapipsp2.c

    \Description
        This module implements High Level Buddy, an application level interface 
        to Friends for PSP2. This is LARGELY stubbed out, as PSP2 does 
        not expose many Friends functions, and expect clients to use HUD.

    \Copyright
        Copyright (c) 2006 Electronic Arts, Inc.

    \Version 07/28/06 (tdills) First Version

*/
/*************************************************************************************H*/

/*** Include files *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <np.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/buddy/hlbudapi.h"
#include "DirtySDK/util/hasher.h"
#include "DirtySDK/friend/friendapi.h"

/*** Defines ***************************************************************************/

#define HLB_PRESENCE_STATE_INGAME   0x00000001
#define HLB_PRESENCE_STATE_JOINABLE 0x00000002

#define HLB_MSG_READ_FLAG 1     // uUserFlag in msg -- bit 0 indicates read or not.

#define HLB_GAME_INVITE "HLB Game Invite"
#define HLB_INVITE_BODY "This is the PSP2 HLB Game Invite Message"

/*** Macros ****************************************************************************/
//To compile out debug print strings in Production release. 
#if DIRTYCODE_LOGGING
#define HLBPrintf(x) _HLBPrintfCode x
#else
#define HLBPrintf(x)
#endif


/*** Type Definitions ******************************************************************/

//! presence data for PSP2 EA Titles
typedef struct HLBPresenceT
{
    int32_t iTitleId;         //!< title identifier

    // TODO: we currently have no way to set our state bits; some of them require more information
    // that we probably have access to. We may need to rework this some.
    int32_t iStateBits;       //!< is the user currently playing a game?

    char    strEAData[56];    //!< reserved for generic EA-Presence data to be added later
    char    strTitleData[64]; //!< 64-bytes of title-specific presence data
} HLBPresenceT;

//! message data
typedef struct HLBMessageT
{
    int32_t iLength;       //!< how much data is here
    BuddyApiMsgT BuddyMsg; //!< message data for signalling callback
    int8_t  *pData;        //!< the data
} HLBMessageT;

//! buddy data
struct HLBBudT     
{
    HLBPresenceT Presence;     //!< EA-Presence data
    SceNpId      UserId;       //!< sony user id for sending chat messages
    DispListRef *pMessageList; //!< list of messages received from this buddy
    HLBApiRefT *pHLBApi;       //!< pointer back to the HLBApi reference

    enum
    {
        ST_OFFLINE,
        ST_ONLINE,
        ST_BLOCKED
    } eState;              //!< the state of this buddy
};

//! current module state
struct HLBApiRefT
{
    BuddyMemAllocT *pBuddyAlloc;  //!< buddy memory allocation function.
    BuddyMemFreeT  *pBuddyFree;   //!< buddy memory free function.

    // module memory group
    int32_t iMemGroup;            //!< module mem group id
    void    *pMemGroupUserData;   //!< user data associated with mem group

    int32_t iStat;                //!<  status of this api.
    HLBConnectStateE eConnectState; //!< are we connected to the Sony library?

    HLBOpCallbackT   *pChangeCallback;  //! buddy list or presence change callback function  
    void             *pChangeUserData;  //! client context data for buddy list callback

    HLBMsgCallbackT *pMsgCallback; //!< callback for receiving chat message
    void            *pMsgUserData; //!< user data to include in chat message callback

    HLBBudActionCallbackT *pMsgAttachmentCallback; //!< callback for receiving message with attachment
    void            *pMsgAttachmentUserData; //!< user data to include in message attachment callback

    HLBBudActionCallbackT *pGameInviteCallback; //!< callback for receiving game invite
    void            *pGameInviteUserData; //!< user data to include in game invite callback

    int32_t bRosterDirty;	       //!< had buddy list or presence been updated?
    int32_t bBuddyListDirty;       //!< have we recieved updated presence or message data?

    //! function for sorting the buddy list
    int32_t (*pBuddySortFunc) (void *pSortref, int32_t iSortcon, void *pRecptr1, void *pRecptr2);
    void (*pDebugProc)(void *pData, const char *pText); //!< client provided debug for printing errors.

    HLBPresenceCallbackT *pBuddyPresProc; //!< callback for when presence changes.
    void *pBuddyPresData;                 //!< context data for buddy presence callback.

    HasherRef *pHash;           //!< hash table for the "roster" of friends
    DispListRef *pDisp;         //!< display list "roster" of friends; list of HLBBudT*

    HLBBudT BuddyData;              //!< presence data for the local user
    int32_t iMaxMessagesPerBuddy;   //!< maximum number of chat messages allowed per buddy
    int32_t iMaxMessages;           //!< maximum number of total chat messages allowed
    int32_t iTotalMessages;         //!< current total number of chat messages for all buddies
    int32_t bGetBlockedList;    //!< indicates that it is time to go get the blocked list from Sony

    Utf8TransTblT *pTransTbl;   //!< utf8 optional transtable for utf8 to 8bit.
    
    //! Callback to forward sceNpBasic library events to.
    #if 0 //$$ deprecated
    HLBSceNpBasicEventCallbackT *pSceNpBasicEventCallback;
    #endif

    //! Arbitrary caller-specified context to pass to the HLBSceNpBasicEventCallbackT.
    void *pSceNpBasicEventCallbackContext;
};

typedef struct HLBudAddPlyrHistDataT
{
    HLBApiRefT *pRef;
    int32_t iNumIds;
    SceNpId NpIds[1];       //!< variable-length array (must come last)
} HLBudAddPlyrHistDataT;

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables

//*** Private Functions *****************************************************************

#if DIRTYCODE_LOGGING
/*F**************************************************************************************/
/*!
    \Function HLBPrintfCode

    \Description
        Handle debug output

    \Input *pState  - reference pointer
    \Input *pFormat - format string
    \Input ...      - varargs

    \Version 09/01/2006 (jbrookes)
*/
/**************************************************************************************F*/
static void _HLBPrintfCode(HLBApiRefT *pHLBApi, const char *pFormat, ...)
{
    char strText[4096];
    int32_t iPrefixLen;
    va_list args;

    // set up output string
    ds_strnzcpy(strText, "hlbudapipsp2: ", sizeof(strText));
    iPrefixLen = strlen(strText);

    // parse the data
    va_start(args, pFormat);
    ds_vsnzprintf(strText+iPrefixLen, sizeof(strText)-iPrefixLen, pFormat, args);
    va_end(args);

    // pass to caller routine if provied
    if (pHLBApi->pDebugProc != NULL)
    {
        pHLBApi->pDebugProc("", strText);
    }
    else
    {
        NetPrintf(("%s", strText));
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _HLBSignalBuddyListChanged

    \Description
        Tell the client that the buddy list has changed

    \Input *pHLBApi - pointer to HLBApi module reference

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
static void _HLBSignalBuddyListChanged(HLBApiRefT *pHLBApi)
{
    pHLBApi->bRosterDirty = TRUE;
    if  ((pHLBApi) && (pHLBApi->pChangeCallback))
    {
        pHLBApi->pChangeCallback(pHLBApi, HLB_OP_LOAD, HLB_ERROR_NONE, pHLBApi->pChangeUserData); 
    }
}

/*F********************************************************************************/
/*!
    \Function _HLBSignalMessageReceived

    \Description
        Tell the client that we've recieved a chat message

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pMessage    - the message that we received

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
static void _HLBSignalMessageReceived(HLBApiRefT *pHLBApi, HLBMessageT *pMessage)
{
    if  ((pHLBApi) && (pHLBApi->pMsgCallback))
    {
        pHLBApi->pMsgCallback(pHLBApi, &pMessage->BuddyMsg, pHLBApi->pMsgUserData); 
    }
}


/*F********************************************************************************/
/*!
    \Function _HLBRosterCount

    \Description
        Get the number of buddies in the roster

    \Input *pHLBApi - pointer to HLBApi module reference
    \Input iFlag    - flag

    \Output
        int32_t     - the number of buddies in the roster

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _HLBRosterCount(HLBApiRefT *pHLBApi, uint32_t iFlag)
{
    if  (( pHLBApi ==NULL ) || pHLBApi->pDisp == NULL)
    {
        return(0);
    }
    // TODO: add support for BUDDY_FLAG_SAME - determine which buddies are running the same title.
    return(DispListCount(pHLBApi->pDisp));
}
 
/*F********************************************************************************/
/*!
    \Function _HLBRosterDestroy

    \Description
        Frees the memory used by the friends data.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
static void _HLBRosterDestroy(HLBApiRefT *pHLBApi)
{
    DispListT *pRec;
    HLBBudT *pBuddy;

    // make sure we actually have a list
    if (pHLBApi->pDisp != NULL)
    {
        // free the resources
        while ((pRec = HasherFlush(pHLBApi->pHash)) != NULL)
        {
            pBuddy =(DispListDel(pHLBApi->pDisp, pRec));

            if (pBuddy->pMessageList != NULL)
            {
                HLBMsgListDeleteAll(pBuddy, FALSE);
                DispListDestroy(pBuddy->pMessageList);
            }

            pHLBApi->pBuddyFree(pBuddy, HLBUDAPI_MEMID, pHLBApi->iMemGroup, pHLBApi->pMemGroupUserData);
        }

        // done with resources
        HasherDestroy(pHLBApi->pHash);
        pHLBApi->pHash = NULL;
        DispListDestroy(pHLBApi->pDisp);
        pHLBApi->pDisp = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _HLBRosterSort

    \Description
        Default sort function. Can be overridden by supplying a sort function to 
        HLBListSetSortFunction

    \Input *pSortref - ignored
    \Input iSortcon  - ignored
    \Input *pRecptr1 - record 1
    \Input *pRecptr2 - record 2

    \Output
        int32_t - (-1) if record 1 goes before record 2;
                  (1)  if record 1 goes after record 2;
                  (0)  if the two records are equivelant

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _HLBRosterSort(void *pSortref, int32_t iSortcon, void *pRecptr1, void *pRecptr2)
{
    int32_t iResult;

    // convert the two in pointer to BuddyRosterT 
    HLBBudT *pBuddy1 = (HLBBudT *)pRecptr1;
    HLBBudT *pBuddy2 = (HLBBudT *)pRecptr2;

    // get the state for both buddies
    int32_t on1 = HLBBudGetState(pBuddy1);
    int32_t on2 = HLBBudGetState(pBuddy2);

    if (on1 == on2)
    {
        // if the states are the same then sort by name
        iResult = ds_stricmp(pBuddy1->UserId.handle.data, pBuddy2->UserId.handle.data);
    }
    else
    {
        // online goes on top, so make it lower.
        if ((on1== BUDDY_STAT_CHAT) || (on2 == BUDDY_STAT_CHAT ))
        {
            iResult = (on1 == BUDDY_STAT_CHAT) ? -1 : 1;
        }
        else 
        {
            //passive (in game) goes above  offline, so make it lower.
            iResult =  (on1 > on2) ? -1 : 1;
        }
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HLBApiInit

    \Description
        Initialize the HLB module.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        HLB_ERROR_*.

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _HLBApiInit(HLBApiRefT *pHLBApi)
{
    if (pHLBApi == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }

    pHLBApi->pBuddySortFunc = _HLBRosterSort;

    // allocate the roster list and make it active 
	pHLBApi->pHash = HasherCreate(1000, 2000);
	pHLBApi->pDisp = DispListCreate(100, 100, 0);

    //connect and set state..
    return(HLBApiConnect(pHLBApi, "", 0, "", ""));
}

/*F********************************************************************************/
/*!
    \Function _HLBSendPresenceData

    \Description
        Sends presence data to the PSP2 libraries.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t - HLB_ERROR_*

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _HLBSendPresenceData(HLBApiRefT *pHLBApi)
{
    return(HLB_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function _HLBGetBuddyRec

    \Description
        Gets a buddy by his user name.

    \Input *pHLBApi - pointer to HLBApi module reference
    \Input *pNpId   - the Sony NpId for the user to get/add
    \Input bAdd     - add a new record if the buddy isn't in the list?

    \Output
        pointer to the HLBBudT object corresponding user name passed in

    \Version 08/01/2006 (tdills)
*/
/********************************************************************************F*/
static HLBBudT *_HLBGetBuddyRec(HLBApiRefT *pHLBApi, const SceNpId *pNpId, int32_t bAdd)
{
    HLBBudT *pBuddy = NULL;
    DispListT *pRec;

    // find user record
    pRec = HashStrFind(pHLBApi->pHash, pNpId->handle.data);
    if (pRec == NULL)
    {
        HLBPrintf((pHLBApi, "could not find '%s' in hashtable", pNpId->handle.data));
        if (bAdd)
        {
            // new one, so alloc it
            HLBPrintf((pHLBApi, "adding buddy %s to list.", pNpId->handle.data));
            pBuddy = pHLBApi->pBuddyAlloc(sizeof(*pBuddy), HLBUDAPI_MEMID, pHLBApi->iMemGroup, pHLBApi->pMemGroupUserData);

            // initialize the buddy record
            memset(pBuddy, 0, sizeof(HLBBudT));
            memcpy(&pBuddy->UserId, pNpId, sizeof(pBuddy->UserId));
            pBuddy->pHLBApi = pHLBApi;

            // add to the disp list & hash table
            pRec = DispListAdd(pHLBApi->pDisp, pBuddy);
            if (pRec == NULL)
            {
                HLBPrintf((pHLBApi, "buddy add failed."));
            }
            else
            {
                if (HashStrAdd(pHLBApi->pHash, pBuddy->UserId.handle.data, pRec) < 0)
                {
                    HLBPrintf((pHLBApi, "buddy add(hash) failed."));
                }
            }
        }
    }
    else
    {
        pBuddy = DispListGet(pHLBApi->pDisp, pRec);
    }

    return(pBuddy);
}

/*F********************************************************************************/
/*!
    \Function _HLBUpdateBuddy

    \Description
        Caches buddy data receieved from the presence event

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *pBuddyData      - buddy data received from Sony library
    \Input bOnline          - is this buddy online?
    \Input bBlocked         - is this buddy on the blocked list?
    \Input *pPresenceData   - presence data; NULL if offline.

    \Output
        pointer to the HLBBudT object corresponding to the user that sent presence data

    \Version 08/01/2006 (tdills)
*/
/********************************************************************************F*/
static HLBBudT *_HLBUpdateBuddy(HLBApiRefT *pHLBApi, const SceNpId *pBuddyData, int32_t bOnline, int32_t bBlocked, int8_t *pPresenceData)
{
    HLBBudT *pBuddy = NULL;

    // extract name and look for matching record
    pBuddy = _HLBGetBuddyRec(pHLBApi, pBuddyData, TRUE);

    if (pBuddy != NULL)
    {
        HLBStatE eOldState = HLBBudGetState(pBuddy);

        if (bBlocked)
        {
            pBuddy->eState = ST_BLOCKED;
        }
        else if (bOnline)
        {
            pBuddy->eState = ST_ONLINE;
            memcpy(&pBuddy->Presence, pPresenceData, sizeof(pBuddy->Presence));
        }
        else
        {
            pBuddy->eState = ST_OFFLINE;
        }

        pHLBApi->bBuddyListDirty = TRUE;

        if (pHLBApi->pBuddyPresProc)
        {
            pHLBApi->pBuddyPresProc(pHLBApi, pBuddy, eOldState, pHLBApi->pBuddyPresData);
        }
    }
    return(pBuddy);
}

/*F********************************************************************************/
/*!
    \Function _HLBRemoveBuddy

    \Description
        Removes a buddy

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pNpId       - Np Id

    \Output
        TRUE if the buddy was found and removed. FALSE otherwise.

    \Version 05/08/2008 (ACS)
*/
/********************************************************************************F*/
static int32_t _HLBRemoveBuddy(HLBApiRefT *pHLBApi, const SceNpId *pNpId)
{
    int32_t iResult;
    DispListT *pRec = NULL;
    HLBBudT *pBuddy = NULL;
    const char *pBuddyName = pNpId->handle.data;

    pRec = HashStrFind(pHLBApi->pHash, pBuddyName);
    if (pRec)
    {
        pBuddy = DispListGet(pHLBApi->pDisp, pRec);
        HLBPrintf((pHLBApi, "removing user %s who had state %i", pBuddyName, pBuddy->eState));
        HashStrDel(pHLBApi->pHash, pBuddyName);
        DispListDel(pHLBApi->pDisp, pRec);
        pHLBApi->pBuddyFree(pBuddy, HLBUDAPI_MEMID, pHLBApi->iMemGroup, pHLBApi->pMemGroupUserData);
        pHLBApi->bBuddyListDirty = TRUE;
        iResult = TRUE;
    }
    else
    {
        HLBPrintf((pHLBApi, "user %s cannot be removed because it cannot be found", pBuddyName, pBuddy->eState));
        iResult = FALSE;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HLBGetOldestBuddyMessage

    \Description
        Finds the oldest message from a particular buddy's message queue.

    \Input *pBuddy  - reference to buddy from whom find oldest message

    \Output
        int32_t     - -1 if there are no messages in the queue; otherwise index of oldest message.

    \Version 08/03/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _HLBGetOldestBuddyMessage(HLBBudT *pBuddy)
{
    int32_t iMsgIndex, iMaxMsgs;
    HLBMessageT *pMessage = NULL;
    uint32_t uOldestTime = NetTick();
    int32_t iOldestMessage = -1;

    if (pBuddy->pMessageList == NULL)
    {
        return(-1);
    }

    // iterate over the message in this buddy's queue
    for (iMsgIndex=0, iMaxMsgs=DispListCount(pBuddy->pMessageList), pMessage = DispListIndex(pBuddy->pMessageList, 0);
        (iMsgIndex < iMaxMsgs) && (pMessage != NULL);
        pMessage = DispListIndex(pBuddy->pMessageList, ++iMsgIndex))
    {
        // if this is the oldest one we've found so far...
        if (pMessage->BuddyMsg.uTimeStamp < uOldestTime)
        {
            // ...then store the message pointer and update oldest time
            iOldestMessage = iMsgIndex-1;
            uOldestTime = pMessage->BuddyMsg.uTimeStamp;
        }
    }

    return(iOldestMessage);
}

/*F********************************************************************************/
/*!
    \Function _HLBDeleteOldestBuddyMessage

    \Description
        Deletes the oldest message from a particular buddy's message queue to make 
        room for newer messages

    \Input *pBuddy  - reference to buddy from whom find and delete oldest message

    \Output
        int32_t     - TRUE if a message has been removed from the list

    \Version 08/03/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _HLBDeleteOldestBuddyMessage(HLBBudT *pBuddy)
{
    int32_t iIndex = _HLBGetOldestBuddyMessage(pBuddy);

    if ((iIndex < 0) || (pBuddy->pMessageList == NULL))
    {
        return(FALSE);
    }

    // delete the message
    DispListDelByIndex(pBuddy->pMessageList, iIndex);
    pBuddy->pHLBApi->iTotalMessages--;

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _HLBDeleteOldestMessage

    \Description
        Deletes the oldest message from a message queue to make room for newer messages

    \Input *pHLBApi - module reference

    \Output
        int32_t     - TRUE if a message has been removed from the list

    \Version 08/03/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _HLBDeleteOldestMessage(HLBApiRefT *pHLBApi)
{
    int32_t iBuddyIndex, iMaxBuddies;
    int32_t iMsgIndex;
    HLBBudT *pBuddy = NULL;
    HLBMessageT *pMessage = NULL;

    HLBBudT* pBuddyWithOldestMessage = NULL;
    int32_t iOldestIndex = -1;
    uint32_t uOldestTime = NetTick();

    // iterate over all the buddies
    for (iBuddyIndex=0, iMaxBuddies=DispListCount(pHLBApi->pDisp), pBuddy = DispListIndex(pHLBApi->pDisp, 0);
         (iBuddyIndex < iMaxBuddies) && (pBuddy != NULL);
         pBuddy = DispListIndex(pHLBApi->pDisp, ++iBuddyIndex))
    {
        iMsgIndex = _HLBGetOldestBuddyMessage(pBuddy);

        // if we found a valid message
        if (iMsgIndex >= 0)
        {
            pMessage = DispListIndex(pBuddy->pMessageList, iMsgIndex);

            // if this buddy's oldest message is older than the current oldest...
            if ((pMessage != NULL) && (pMessage->BuddyMsg.uTimeStamp < uOldestTime))
            {
                // ...then store the info and update uOldestTime
                pBuddyWithOldestMessage = pBuddy;
                iOldestIndex = iMsgIndex;
                uOldestTime = pMessage->BuddyMsg.uTimeStamp;
            }
        }
    }

    if ((pBuddyWithOldestMessage == NULL) || (iOldestIndex < 0))
    {
        // this should only happen if we have no messages at all
        return(FALSE);
    }

    // delete the oldest message
    DispListDelByIndex(pBuddyWithOldestMessage->pMessageList, iOldestIndex);
    pHLBApi->iTotalMessages--;

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _HLBReceiveMessage

    \Description
        Removes events from the PSP2 event queue.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pBuddyData  - buddy data received from Sony library
    \Input *pMessageBuf - the message data received from the peer
    \Input iDataSize    - the length of the message data received from the peer.

    \Version 08/01/2006 (tdills)
*/
/********************************************************************************F*/
static void _HLBReceiveMessage(HLBApiRefT *pHLBApi, SceNpId *pBuddyData, int8_t *pMessageBuf, int32_t iDataSize)
{
    HLBBudT *pBuddy = NULL;

    // extract name and look for matching record
    pBuddy = _HLBGetBuddyRec(pHLBApi, pBuddyData, TRUE);

    if (pBuddy != NULL)
    {
        // construct a new message record and populate with the message data
        HLBMessageT *pNewMessage = pHLBApi->pBuddyAlloc(sizeof(HLBMessageT)+iDataSize+1, HLBUDAPI_MEMID, pHLBApi->iMemGroup, pHLBApi->pMemGroupUserData);
        memset(pNewMessage, 0, sizeof(HLBMessageT)+iDataSize+1);
        pNewMessage->iLength = iDataSize;
        pNewMessage->BuddyMsg.iKind = HLB_CHAT_KIND_CHAT;
        pNewMessage->BuddyMsg.uTimeStamp = NetTick();
        pNewMessage->pData = (void*)((int32_t)&pNewMessage->pData + (int32_t)sizeof(void*));
        pNewMessage->BuddyMsg.pData = pNewMessage->pData;
        memcpy(pNewMessage->pData, pMessageBuf, iDataSize);

        HLBPrintf((pHLBApi, "pNewMessage(0x%08x) - allocated %d bytes; pData is at 0x%08x", pNewMessage, 
            sizeof(HLBMessageT)+iDataSize+1, pNewMessage->pData));
        HLBPrintf((pHLBApi, "stored message text: %s", pNewMessage->pData));

        // if we don't have a message list yet create one
        if (pBuddy->pMessageList == NULL)
        {
            pBuddy->pMessageList = DispListCreate(5, 5, 0);
        }

        // make sure we don't have too many messages for this buddy
        while (DispListCount(pBuddy->pMessageList) >= pHLBApi->iMaxMessagesPerBuddy)
        {
            if (!_HLBDeleteOldestBuddyMessage(pBuddy))
            {
                HLBPrintf((pHLBApi, "FAILURE - we reached our max messages for this buddy but cannot purge any old ones."));
                return;
            }
        }

        // make sure we don't have too many messages altogether
        while (pHLBApi->iTotalMessages >= pHLBApi->iMaxMessages)
        {
            if (!_HLBDeleteOldestMessage(pHLBApi))
            {
                HLBPrintf((pHLBApi, "FAILURE - we reached our max messages but cannot purge any old ones."));
                return;
            }
        }

        // add the message to the message list
        DispListAdd(pBuddy->pMessageList, pNewMessage);
        pHLBApi->iTotalMessages += 1;

        // signal the client that we've received a new chat message
        _HLBSignalMessageReceived(pHLBApi, pNewMessage);
    }
}

#if 0
/*F********************************************************************************/
/*!
    \Function _HLBSignalMsgAttachmentReceived

    \Description
        Tell the client that we've recieved a message attachment

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pBuddyData  - buddy data

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
static void _HLBSignalMsgAttachmentReceived(HLBApiRefT *pHLBApi, SceNpId *pBuddyData) //, HLBMessageT *pMessage)
{
    HLBBudT *pBuddy = NULL;

    // extract name and look for matching record
    pBuddy = _HLBGetBuddyRec(pHLBApi, pBuddyData, TRUE);

    if (pBuddy != NULL)
    {
        if  ((pHLBApi) && (pHLBApi->pMsgAttachmentCallback))
        {
            pHLBApi->pMsgAttachmentCallback(pHLBApi, NULL, 0, NULL); 
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _HLBRecvMsgAttachmentResult

    \Description
        Get the message attachment, and handle game invite.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *pBuddyData      - buddy data
    \Input *ad              - attachment data

    \Output
        int32_t             - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
static int32_t _HLBRecvMsgAttachmentResult(HLBApiRefT *pHLBApi, SceNpId *pBuddyData, SceNpBasicAttachmentData *ad)
{
    int32_t iNpStatus = 0;
    int32_t iStatus = HLB_ERROR_NONE;
    HLBBudT *pBuddy = NULL;

    // extract name and look for matching record
    pBuddy = _HLBGetBuddyRec(pHLBApi, pBuddyData, TRUE);

    if (pBuddy != NULL)
    {
        uint8_t *pData;
        size_t iDataSize = ad->size;
        pData = (uint8_t *) DirtyMemAlloc(iDataSize, HLBUDAPI_MEMID, pHLBApi->iMemGroup, pHLBApi->pMemGroupUserData);

        if ((iNpStatus = sceNpBasicRecvMessageAttachmentLoad(ad->id, pData, &iDataSize)) >= 0)
        {
            // trigger GameInvite callback
            if (!strcmp(pData, HLB_INVITE_BODY) && (pHLBApi != NULL) && (pHLBApi->pGameInviteCallback != NULL))
            {
                pHLBApi->pGameInviteCallback(pHLBApi, NULL, 0, NULL); 
            }
            
            // free data load
            DirtyMemFree(pData, HLBUDAPI_MEMID, pHLBApi->iMemGroup, pHLBApi->pMemGroupUserData);
        }
        else
        {
            HLBPrintf((pHLBApi, "sceNpBasicRecvMessageAttachmentLoad failed: %s", DirtyErrGetName(iNpStatus)));
            iStatus = HLB_ERROR_UNKNOWN;
        }
    }

    return(iStatus);
}
#endif

/*F********************************************************************************/
/*!
    \Function _FriendApiNotificationCb

    \Description
        Callback registered with the Friend API module

    \Input *pFriendApiRef       - module reference
    \Input pEventData           - pointer to event data
    \Input *pNotifyCbUserData   - pointer to user data

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
static void _FriendApiNotificationCb(FriendApiRefT *pFriendApiRef, FriendEventDataT *pEventData, void *pNotifyCbUserData)
{
    HLBApiRefT *pHLBApi = (HLBApiRefT *) pNotifyCbUserData;
    SceNpId *pUserId = NULL;

    switch(pEventData->eventType)
    {
        case FRIENDAPI_EVENT_UNKNOWN:
            break;

        case FRIENDAPI_EVENT_FRIEND_UPDATED:
            pUserId = (SceNpId *)(pEventData->eventTypeData.FriendUpdated.pFriendData->pRawData1);

            if (pEventData->eventTypeData.FriendUpdated.bPresence == TRUE)
            {
                _HLBUpdateBuddy(pHLBApi, pUserId, TRUE, FALSE, pEventData->eventTypeData.FriendUpdated.pRawData2);
            }
            else
            {
                _HLBUpdateBuddy(pHLBApi, pUserId, FALSE, FALSE, NULL);
            }
            break;

        case FRIENDAPI_EVENT_FRIEND_REMOVED:
            pUserId = (SceNpId *)(pEventData->eventTypeData.FriendRemoved.pFriendData->pRawData1);
            _HLBRemoveBuddy(pHLBApi, pUserId);
            break;

        case FRIENDAPI_EVENT_BLOCKLIST_UPDATED:
            NetPrintf(("hlbudapipsp2: it is time to update the blocked list\n"));
            pHLBApi->bGetBlockedList = TRUE;
            break;

        case FRIENDAPI_EVENT_MSG_RCVED:
            pUserId = (SceNpId *)(pEventData->eventTypeData.MsgRcved.pFriendData->pRawData1);
            _HLBReceiveMessage(pHLBApi, pUserId, pEventData->eventTypeData.MsgRcved.pRawData2, *(int *)(pEventData->eventTypeData.MsgRcved.pRawData1));
            break;

        default:
            NetPrintf(("hlbudapipsp2: invalid friendapi event type\n"));
            break;
    }
}

/*F********************************************************************************/
/*!
    \Function _HLBDeleteMessageAndFree

    \Description
        Deletes a chat message from a buddy's message queue and frees the memory.

    \Input *pBuddy      - pointer to Buddy objecty that contains message to delete
    \Input iMsgIndex    - index of message to delete and free

    \Version 08/01/2006 (tdills)
*/
/********************************************************************************F*/
static void _HLBDeleteMessageAndFree(HLBBudT *pBuddy, int32_t iMsgIndex)
{
    HLBMessageT *pMessage = NULL;

    // make sure the buddy has a message to delete
    if (pBuddy->pMessageList == NULL)
    {
        return;
    }

    // remove the message from the list
    pMessage = DispListDelByIndex(pBuddy->pMessageList, iMsgIndex);

    if (pMessage != NULL)
    {
        // free the memory
        pBuddy->pHLBApi->pBuddyFree(pMessage, HLBUDAPI_MEMID, pBuddy->pHLBApi->iMemGroup, pBuddy->pHLBApi->pMemGroupUserData);
        // keep track of our total message count
        pBuddy->pHLBApi->iTotalMessages--;
    }
}

//*** Public Functions *****************************************************************


/*F********************************************************************************/
/*!
    \Function HLBListCancelAllInvites

    \Description
        Drastic, no callback function to nuke all invites, prior to say, Block a buddy.
        Not implemented on PSP2.

    \Input *pHLBApi - pointer to HLBApi module reference
    \Input *pName   - name of buddy from whom to block invites

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListCancelAllInvites(HLBApiRefT *pHLBApi, const char *pName)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListDisableSorting

    \Description
        Disable sorting of the buddy list.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBListDisableSorting(HLBApiRefT *pHLBApi)
{
    if ((pHLBApi) && ( pHLBApi->pDisp))
    {
        // Just before we disable sorting we should sort the list one last time.
        // Force the list to appear to be changed so it will get resorted.
        DispListChange(pHLBApi->pDisp, 1);

        // Resort the list.
        DispListOrder(pHLBApi->pDisp);

        // Now disable sorting by setting the sort function to NULL.
        DispListSort(pHLBApi->pDisp, NULL, 0, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function HLBApiCreate2

    \Description
        Create HLB Api, override the default memory alloc/free functions

    \Input *pAlloc                  - function to allocate memory
    \Input *pFree                   - function to free memory
    \Input *pProduct                - product name - ignored
    \Input *pRelease                - release version - ignored
    \Input iRefMemGroup             - the memory group in which the memory is allocated
    \Input *pRefMemGroupUserData    - user data associated with the mem group

    \Output
        HLBApiRefT*                 - a pointer to the newly allocated HLB module

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
HLBApiRefT *HLBApiCreate2(BuddyMemAllocT *pAlloc, BuddyMemFreeT *pFree,
                          const char *pProduct, const char *pRelease, 
                          int32_t iRefMemGroup, void *pRefMemGroupUserData)
{
    HLBApiRefT *pHLBApi;
    // int32_t iNpStatus = 0;
    FriendApiRefT *pFriendApiRef;

    if ((pAlloc == NULL ) || (pFree == NULL))
        return(NULL); // use HLBApiCreate instead..

    // if no ref memgroup specified, use current group
    if (iRefMemGroup == 0)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;
     
        // Query mem group data 
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);  
        
        iRefMemGroup = iMemGroup;
        pRefMemGroupUserData = pMemGroupUserData;
    }

    // allocate new state record
    pHLBApi = (HLBApiRefT*)pAlloc(sizeof(*pHLBApi), HLBUDAPI_MEMID, iRefMemGroup, pRefMemGroupUserData); 
    if (pHLBApi != NULL)
    {
        memset(pHLBApi, 0, sizeof(*pHLBApi));
        pHLBApi->iMemGroup = iRefMemGroup;
        pHLBApi->pMemGroupUserData = pRefMemGroupUserData;
        pHLBApi->pBuddyAlloc = pAlloc;
        pHLBApi->pBuddyFree = pFree;
        pHLBApi->iMaxMessages = HLB_MAX_MESSAGES_ALLOWED;
        pHLBApi->iMaxMessagesPerBuddy = HLB_MAX_MESSAGES_ALLOWED;
    }

    if (_HLBApiInit(pHLBApi) != HLB_ERROR_NONE)
    {
        pFree(pHLBApi, HLBUDAPI_MEMID, iRefMemGroup, pRefMemGroupUserData);
        pHLBApi = NULL;
    }

    // register notification callback with friend api module
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));
    FriendApiAddCallback(pFriendApiRef, _FriendApiNotificationCb, (void *)pHLBApi);

    return(pHLBApi);
}

/*F********************************************************************************/
/*!
    \Function HLBApiCreate

    \Description
        Create HLB Api.

    \Input *pProduct                - product name - ignored
    \Input *pRelease                - release version - ignored
    \Input iRefMemGroup             - the memory group in which the memory is allocated
    \Input *pRefMemGroupUserData    - user data associated with the memory group

    \Output
        HLBApiRefT*                 - a pointer to the newly allocated HLB module

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
HLBApiRefT *HLBApiCreate(const char *pProduct, const char *pRelease,int32_t iRefMemGroup, void *pRefMemGroupUserData)
{
    return(HLBApiCreate2(DirtyMemAlloc, DirtyMemFree, pProduct, pRelease, iRefMemGroup, pRefMemGroupUserData));
}

/*F********************************************************************************/
/*!
    \Function HLBApiSetDebugFunction

    \Description
        Set high-level debug function.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pDebugData  - ignored
    \Input *pDebugProc  - the user-supplied debug function

    \Notes
        Caller sets this so debug info from High-level buddy api will show up
        in whatever screen or log the caller has.

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiSetDebugFunction(HLBApiRefT *pHLBApi, void *pDebugData, void (*pDebugProc)(void *pData, const char *pText))
{
    if ((pHLBApi == NULL) || (pDebugProc == NULL)) 
        return;
    NetPrintf(("HLBApi: setting debug function\n"));
    pDebugProc(NULL, "hlbudapipsp2: testing debug proc\n");
    pHLBApi->pDebugProc = pDebugProc;
}

/*F********************************************************************************/
/*!
    \Function HLBApiOverrideXblIsSameProductCheck

    \Description
        Not supported on PSP2

    \Input *pHLBApi                     - pointer to HLBApi module reference
    \Input bDontUseXBLSameTitleIdCheck  - ignored

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiOverrideXblIsSameProductCheck(HLBApiRefT *pHLBApi, int32_t bDontUseXBLSameTitleIdCheck)
{
    return;  // HLB_ERROR_UNSUPPORTED;
}


/*F********************************************************************************/
/*!
    \Function HLBApiOverrideMaxMessagesPerBuddy

    \Description
        Not supported on PSP2

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input iMaxMessages - sets the maximum number of messages to be stored per buddy - default is 100

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiOverrideMaxMessagesPerBuddy(HLBApiRefT *pHLBApi, int32_t iMaxMessages) 
{
    if (pHLBApi == NULL)
    {
        return;
    }

    pHLBApi->iMaxMessagesPerBuddy = iMaxMessages;
    return;
}


/*F********************************************************************************/
/*!
    \Function HLBApiOverrideConstants

    \Description
        Not supported on PSP2

    \Input *pHLBApi                     - pointer to HLBApi module reference
    \Input bNoTextSupport               - ignored
    \Input connect_timeout              - ignored
    \Input op_timeout                   - ignored
    \Input max_perm_buddies             - ignored
    \Input max_temp_buddies             - ignored
    \Input max_msgs_allowed             - ignored
    \Input bTalk_to_xbox                - ignored
    \Input max_msg_len                  - ignored
    \Input bTrackSendMsgInOpState       - ignored
    \Input iGameInviteAccept_timeout    - ignored

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
//!Application  may override default constants. -1 leaves it as default. Only valid before HLBApiConfig is called
void HLBApiOverrideConstants(HLBApiRefT *pHLBApi,
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
{
    return;  // HLB_ERROR_UNSUPPORTED;
}


/*F********************************************************************************/
/*!
    \Function HLBApiSetUtf8TransTbl

    \Description
        Sets the UTF8 to 8-bit translation table.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pTransTbl   - user-supplied translation table

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiSetUtf8TransTbl(HLBApiRefT *pHLBApi, Utf8TransTblT *pTransTbl)
{
    pHLBApi->pTransTbl = pTransTbl;
    return; 
}


/*F********************************************************************************/
/*!
    \Function HLBApiInitialize

    \Description
        Configure and initialize any data structures in the API. No-OP on PSP2.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *prodPresence    - ignored
    \Input uLocality        - ignored

    \Output
        int32_t             - HLB_ERROR_MISSING if pHLBApi is NULL; HLB_ERROR_NONE otherwise.

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiInitialize(HLBApiRefT *pHLBApi, const char *prodPresence , uint32_t uLocality)   

{
    if (!pHLBApi)
        return(HLB_ERROR_MISSING);
    return(HLB_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function HLBApiConnect

    \Description
        Connect to the PSP2

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strServer   - server
    \Input iPort        - port
    \Input *strKey      - key
    \Input *strRsrc     - Resource (domain/sub-domain) uniquely identified product.

    \Output
        int32_t         - see HLB_ERROR_*

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiConnect(HLBApiRefT *pHLBApi, const char *strServer, int32_t iPort, const char *strKey, const char *strRsrc )
{
    FriendApiRefT *pFriendApiRef;

    if (pHLBApi == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }
   
    pHLBApi->iStat = BUDDY_STAT_CHAT; //anything but PASSive -- as doing connect.

    // let the FriendApi know that it can now register the NP Basic event handler
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));
    FriendApiControl(pFriendApiRef, 'strt', 0, 0, NULL);

    pHLBApi->eConnectState = HLB_CS_Connected;

    // send initial (null) presence data to the library
    _HLBSendPresenceData(pHLBApi);

    return(HLB_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function HLBApiRegisterConnectCallback

    \Description
        No-Op on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *cb          - ignored
    \Input *pContext    - ignored

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiRegisterConnectCallback(HLBApiRefT *pHLBApi, HLBOpCallbackT *cb , void *pContext)
{
	return(HLB_ERROR_UNSUPPORTED);
}


/*F********************************************************************************/
/*!
    \Function HLBApiGetConnectState

    \Description
        Get state of the connection to the Sony friends library.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - see HLBConnectStateE.

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiGetConnectState(HLBApiRefT *pHLBApi)
{
    if (pHLBApi == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }
    return(pHLBApi->eConnectState);
}

/*F********************************************************************************/
/*!
    \Function HLBApiGetConnectState

    \Description
        Disconnect from the PSP2 friends library.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiDisconnect(HLBApiRefT *pHLBApi)
{
    FriendApiRefT *pFriendApiRef;

    if (pHLBApi == NULL)
        return;

    // let the FriendApi know that it can now unregister the NP Basic event handler
    NetConnStatus('fref', 0, &pFriendApiRef, sizeof(pFriendApiRef));
    FriendApiControl(pFriendApiRef, 'stop', 0, 0, NULL);

    pHLBApi->eConnectState = HLB_CS_Disconnected;
}

/*F********************************************************************************/
/*!
    \Function HLBApiDestroy

    \Description
        Destroy the API and free all memory.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiDestroy(HLBApiRefT *pHLBApi)
{
    if ( pHLBApi == NULL )
    {
        return;  //nothing to do..
    }

    if (pHLBApi->eConnectState == HLB_CS_Connected)
    {
        HLBApiDisconnect(pHLBApi);
    }

    // free the roster data
    _HLBRosterDestroy(pHLBApi);

    // clear the callback
	pHLBApi->pChangeUserData = NULL;
	pHLBApi->pChangeCallback = NULL;
        
    // free the module reference data
    pHLBApi->pBuddyFree(pHLBApi, HLBUDAPI_MEMID, pHLBApi->iMemGroup, pHLBApi->pMemGroupUserData); 
}

/*F********************************************************************************/
/*!
    \Function HLBApiUpdate

    \Description
        Give the API some time to process. Must be polled regularly by client.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiUpdate(HLBApiRefT *pHLBApi)
{
    if (pHLBApi == NULL)
    {
        // this is bad
        return;
    }

    if (pHLBApi->bBuddyListDirty)
    {
        pHLBApi->bBuddyListDirty = FALSE;
        _HLBSignalBuddyListChanged(pHLBApi);
    }


    if (pHLBApi->bGetBlockedList)
    {
        #if 0
        int32_t iNpStatus;
        int32_t iCount, iIndex;
        SceNpId npId;

        if ((iNpStatus = sceNpBasicGetBlockListEntryCount(&iCount)) < 0)
        {
            HLBPrintf((pHLBApi, "sceNpBasicGetBlockListEntryCount() failed (err=%s)", DirtyErrGetName(iNpStatus)));
            return;
        }

        for (iIndex=0; iIndex<iCount; ++iIndex)
        {
            memset((void*)&npId, 0, sizeof(npId));
            if ((iNpStatus = sceNpBasicGetBlockListEntry(iIndex, &npId)) < 0)
            {
                HLBPrintf((pHLBApi, "sceNpBasicGetBlockListEntry(%d) failed (err=%s)", iIndex, DirtyErrGetName(iNpStatus)));
            }
            else
            {
                _HLBUpdateBuddy(pHLBApi, &npId, FALSE, TRUE, NULL);
            }
        }
        #endif

        pHLBApi->bGetBlockedList = FALSE;
    }
}

/*F********************************************************************************/
/*!
    \Function HLBApiSuspend

    \Description
        Free up buddy memory,and stop enumerating, while client goes in game.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - see HLB_ERROR_*

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiSuspend(HLBApiRefT *pHLBApi) 
{ 
    if (pHLBApi == NULL)
    {
        return(HLB_ERROR_MISSING); //nothing to do
    }

    if (pHLBApi->eConnectState == HLB_CS_Connected)
    {
        HLBApiDisconnect(pHLBApi);
    }
    _HLBRosterDestroy(pHLBApi);

    return(HLB_ERROR_NONE);
}
 
/*F********************************************************************************/
/*!
    \Function HLBApiResume

    \Description
        Resume the module after leaving a game.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        void*       - the roster held by this API. NULL if errors.

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void *HLBApiResume(HLBApiRefT *pHLBApi)
{
    // Allow a resume even if NOT connected, as need to clear PASSive state. If not,
    // will be in a bad state if suspend/resume happened when server had disconnected.

    // if already connected ignore and have memory allocated ignore re-allocation
    if(pHLBApi->pDisp == NULL)
    {
        _HLBApiInit(pHLBApi);
    }

    pHLBApi->iStat = BUDDY_STAT_CHAT; //any other than PASSive
    return(pHLBApi->pDisp); 
}

/*F********************************************************************************/
/*!
    \Function HLBApiGetLastOpStatus

    \Description
        Not implemented on PSP2

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiGetLastOpStatus(HLBApiRefT *pHLBApi)
{
	return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiCancelOp

    \Description
        Not implemented on PSP2

    \Input *pHLBApi - pointer to HLBApi module reference

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiCancelOp(HLBApiRefT *pHLBApi)
{
    return; //HLB_ERROR_UNSUPPORTED
}

/*F********************************************************************************/
/*!
    \Function HLBApiFindUsers

    \Description
        Not implemented on PSP2

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pName       - ignored
    \Input *pOpCallback - ignored
    \Input *context     - ignored

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiFindUsers(HLBApiRefT *pHLBApi, const char *pName, HLBOpCallbackT *pOpCallback, void *context)
{
	return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiUserFound

    \Description
        Not implemented on PSP2

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input iItem        - ignored

    \Output
        BuddyApiUserT*  - NULL

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
BuddyApiUserT *HLBApiUserFound(HLBApiRefT *pHLBApi, int32_t iItem)
{
	return(NULL); // HLB_ERROR_UNSUPPORTED;
}

/*F********************************************************************************/
/*!
    \Function HLBReceiveMsgAttachment

    \Description
        Get the message attachment.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pContext    - context

    \Output
        int32_t         - HLB_ERROR_NONE

    \Version 04/02/2007 (danielcarter)
*/
/********************************************************************************F*/
int32_t HLBReceiveMsgAttachment(HLBApiRefT *pHLBApi, void *pContext)
{ 	 
    return(HLB_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function HLBApiRegisterNewMsgCallback

    \Description
        Register callback for new chat messages

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *pMsgCallback    - pointer to callback function
    \Input *pAppData        - pointer to data to send to callback function

    \Output
        int32_t             - see HLB_ERROR_*

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiRegisterNewMsgCallback( HLBApiRefT  * pHLBApi, HLBMsgCallbackT * pMsgCallback, void * pAppData ) 
{
    if (pHLBApi == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }
    if ((pHLBApi->pMsgCallback != NULL) && (pMsgCallback != NULL))
    {
        HLBPrintf((pHLBApi, "ERROR: Cannot have more than one message callback - clear the callback first"));
        return(HLB_ERROR_FULL);
    }

    pHLBApi->pMsgCallback = pMsgCallback;
    pHLBApi->pMsgUserData = pAppData;
    return(HLB_ERROR_NONE);
}	

/*F********************************************************************************/
/*!
    \Function HLBApiRegisterBuddyChangeCallback

    \Description
        Register callback for changes to the roster

    \Input *pHLBApi             - pointer to HLBApi module reference
    \Input *pChangeCallback     - pointer to callback function
    \Input *pAppData            - pointer to data to send to callback function

    \Output
        int32_t                 - see HLB_ERROR_*

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiRegisterBuddyChangeCallback(HLBApiRefT *pHLBApi, HLBOpCallbackT *pChangeCallback, void *pAppData) 
{
    if (pHLBApi == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }
    if ((pHLBApi->pChangeCallback != NULL) && (pChangeCallback != NULL))
    {
        HLBPrintf((pHLBApi, "ERROR: Cannot have more than one buddy change callback - clear the callback first"));
        return(HLB_ERROR_FULL);
    }
    pHLBApi->pChangeCallback = pChangeCallback;
    pHLBApi->pChangeUserData = pAppData;
    return(HLB_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function HLBApiRegisterMsgAttachmentCallback

    \Description
        Register callback for receiving message attachments.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *pActionCallback - pointer to callback function
    \Input *pAppData        - pointer to data to send to callback function

    \Output
        int32_t             - see HLB_ERROR_*

    \Notes
        TODO: we could implement cross-game invites by implementing a higher-level
        protocol on top of the PSP2 messages.

    \Version 04/02/2007 (danielcarter)
*/
/********************************************************************************F*/
int32_t HLBApiRegisterMsgAttachmentCallback(HLBApiRefT *pHLBApi, HLBBudActionCallbackT *pActionCallback, void *pAppData)
{
    if (pHLBApi == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }
    if ((pHLBApi->pMsgAttachmentCallback != NULL) && (pActionCallback != NULL))
    {
        HLBPrintf((pHLBApi, "ERROR: Message Attachment Cannot have more than one message callback - clear the callback first"));
        return(HLB_ERROR_FULL);
    }

    pHLBApi->pMsgAttachmentCallback = pActionCallback;
    pHLBApi->pMsgUserData = pAppData;
    return(HLB_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function HLBApiRegisterGameInviteCallback

    \Description
        Register callback for receiving cross-game invites. Not implemented on PSP2.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *pActionCallback - pointer to callback function
    \Input *pAppData        - pointer to data to send to callback function

    \Output
        int32_t             - see HLB_ERROR_*

    \Notes
        TODO: we could implement cross-game invites by implementing a higher-level
        protocol on top of the PSP2 messages.

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiRegisterGameInviteCallback(HLBApiRefT *pHLBApi, HLBBudActionCallbackT *pActionCallback, void *pAppData)
{
    if (pHLBApi == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }
    if ((pHLBApi->pGameInviteCallback != NULL) && (pActionCallback != NULL))
    {
        HLBPrintf((pHLBApi, "ERROR: Game Invite Cannot have more than one message callback - clear the callback first"));
        return(HLB_ERROR_FULL);
    }

    pHLBApi->pGameInviteCallback = pActionCallback;
    pHLBApi->pGameInviteUserData = pAppData;
    return(HLB_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function HLBApiRegisterBuddyPresenceCallback

    \Description
        Register callback for  when buddy presence changes--state or text can change.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *pPresCallback   - pointer to callback function
    \Input *pAppData        - pointer to data to send to callback function
    
    \Output
        int32_t             - see HLB_ERROR_*

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiRegisterBuddyPresenceCallback(HLBApiRefT *pHLBApi, HLBPresenceCallbackT *pPresCallback, void *pAppData)
{
    if ((pHLBApi->pBuddyPresProc != NULL) && (pPresCallback != NULL))
    {
        HLBPrintf((pHLBApi, "ERROR: Cannot have more than one Buddy Presence Callback"));
        return(HLB_ERROR_FULL); //caller must clear first -- only one supported.
    }
    pHLBApi->pBuddyPresProc = pPresCallback;
    pHLBApi->pBuddyPresData = pAppData; // data for message callback
    return(HLB_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudGetName

    \Description
        Return screen/persona name of this buddy.

    \Input *pBuddy  - pointer to HLBBudT structure containing buddy data

    \Output
        char*       - the name

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
char *HLBBudGetName(HLBBudT *pBuddy)
{
    return(pBuddy->UserId.handle.data);
}

/*F********************************************************************************/
/*!
    \Function HLBBudGetXenonXUID

    \Description
        Return XUID of buddy - not supported on PSP2.

    \Input *pBuddy  - pointer to HLBBudT structure containing buddy data

    \Output
        uint64_t    - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
uint64_t HLBBudGetXenonXUID(HLBBudT *pBuddy)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F*************************************************************************************************/
/*!
    \Function HLBBudGetSceNpId

    \Description
        Returns SceNpId of buddy –PSP2 only! 

    \Input *pBuddy      - pointer to the buddy
    \Input *pSceNpId    - return buffer

    \Version 09/14/2009 (mclouatre)
*/
/*************************************************************************************************F*/
void HLBBudGetSceNpId(HLBBudT *pBuddy, SceNpId *pSceNpId)
{
    memcpy(pSceNpId, &pBuddy->UserId, sizeof(*pSceNpId));
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsTemporary

    \Description
        Determine if buddy is temporary. Not supported on PSP2.

    \Input *pBuddy  - pointer to HLBBudT structure containing buddy data

    \Output
        int32_t     - FALSE

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsTemporary(HLBBudT *pBuddy)
{   
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsBlocked

    \Description
        Determine if buddy is ignored. 

    \Input *pBuddy - pointer to HLBBudT structure containing buddy data

    \Output
        int32_t     - FALSE;

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsBlocked(HLBBudT  *pBuddy)
{
    return(pBuddy->eState == ST_BLOCKED);
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsWannaBeMyBuddy

    \Description
        Has this buddy invited me to be his buddy? Not supported on PSP2.

    \Input *pBuddy  - pointer to HLBBudT structure containing buddy data

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED;

    \Notes
        TODO: can we implement this on PSP2? I think it has to be handled in the OSD.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsWannaBeMyBuddy(HLBBudT *pBuddy)
{
    return(HLB_ERROR_UNSUPPORTED);
}
 
/*F********************************************************************************/
/*!
    \Function HLBBudIsIWannaBeHisBuddy

    \Description
        Have I invited this use to be my buddy? Not supported on PSP2.

    \Input *pBuddy  - pointer to HLBBudT structure containing buddy data

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED;

    \Notes
        TODO: can we implement this on PSP2? I think it has to be handled in the OSD.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsIWannaBeHisBuddy(HLBBudT *pBuddy)
{
    return(HLB_ERROR_UNSUPPORTED);
}
 
/*F********************************************************************************/
/*!
    \Function HLBBudGetTitle

    \Description
        Return the name of the title currently being played by this buddy.
        Not supported on PSP2.

    \Input *pBuddy  - pointer to HLBBudT structure containing buddy data

    \Output
        char*       - NULL;

    \Notes
        TODO: We store the title id as an int32_t and have no method for converting to a string.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
char *HLBBudGetTitle(HLBBudT *pBuddy)
{
    return(NULL);
}
 

/*F********************************************************************************/
/*!
    \Function HLBBudTempBuddyIs

    \Description
        Is this buddy a temporary buddy? Not supported on PSP2.

    \Input *pBuddy      - pointer to HLBBudT structure containing buddy data
    \Input iBuddyType   - type of buddy to check for

    \Output
        int32_t         - FALSE;

    \Notes
        TODO: we could implement something like this on PSP2 using the presence data.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudTempBuddyIs(HLBBudT *pBuddy, int32_t iBuddyType)
{
	return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudTempBuddyIs

    \Description
        Is this buddy a "real" buddy - i.e. not someone on this list due to being blocked
        or added temporarily. PSP2 has no temp buddies so everyone on the list that isn't
        blocked is a real buddy.

    \Input *pBuddy  - pointer to HLBBudT structure containing buddy data

    \Output
        int32_t     - TRUE;

    \Notes
        TODO: Do we need a better implementation?

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsRealBuddy(HLBBudT *pBuddy)
{
    if (pBuddy->eState == ST_BLOCKED)
    {
        return(FALSE);
    }
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsInGroup

    \Description
        Not supported on PSP2.

    \Input *pBuddy  - pointer to HLBBudT structure containing buddy data

    \Output
        int32_t     - FALSE;

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsInGroup(HLBBudT *pBuddy)
{
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudGetState

    \Description
        Get the online/offline state of this buddy.

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - see HLBStatE enum; only HLB_STAT_DISC and HLB_STAT_CHAT are implemented for PSP2

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudGetState(HLBBudT *pBuddy)
{
    if (pBuddy->eState == ST_ONLINE)
    {
        return(HLB_STAT_CHAT);
    }
    return(HLB_STAT_DISC);
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsPassive

    \Description
        Indicate if buddy is passive i.e. is online but playing a game.

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - bool - 1 if buddy is in passive state

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsPassive(HLBBudT *pBuddy)
{
    return(((pBuddy->eState == ST_ONLINE) && (pBuddy->Presence.iStateBits & HLB_PRESENCE_STATE_INGAME)) ? TRUE : FALSE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsAvailableForChat

    \Description
        Indicate if buddy is online and available for chat.

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - bool - 1 if buddy is online and available for chat

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsAvailableForChat(HLBBudT *pBuddy)
{
    return((pBuddy->eState == ST_ONLINE) ? TRUE : FALSE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsSameProduct

    \Description
        Indicate if buddy is online and logged into same product. Not implemented on PSP2.

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - bool - FASLE

    \Notes
        TODO: if we had some way of determine what our title id is then we could
        implement this using the title id in the presence data of the buddy.

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsSameProduct(HLBBudT *pBuddy)
{
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudGetPresence

    \Description
        Return the buddy's title-specific presence data.

    \Input *pBuddy      - pointer to buddy record
    \Input *strPresence - [out] receives the presence data; must be at least 64-btyes

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBBudGetPresence(HLBBudT *pBuddy , char *strPresence)
{
    if (pBuddy)
    {
        ds_strnzcpy(strPresence, pBuddy->Presence.strTitleData, sizeof(pBuddy->Presence.strTitleData));
    }
}

/*F********************************************************************************/
/*!
    \Function HLBBudGetPresenceExtra

    \Description
        Not supported on PSP2.

    \Input *pBuddy          - pointer to buddy record
    \Input *strExtraPres    - [out] receives the presence data
    \Input iStrSize         - length of strPresence - ignored

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
void HLBBudGetPresenceExtra(HLBBudT *pBuddy , char *strExtraPres, int32_t iStrSize)
{
    return;
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsNoReplyBud

    \Description
        Is 'special' temp (i.e.admin)  buddy who WONT accept messages or invites etc.
        Not supported on PSP2.

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - FALSE

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsNoReplyBud(HLBBudT *pBuddy)
{
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudIsNoReplyBud

    \Description
        Indicates if buddy can be "joined" (aka found and gone to) in an Xbox game.  
        Checks the Joinable bit in Presence data.

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - FALSE

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudIsJoinable(HLBBudT *pBuddy)
{
    return((pBuddy->Presence.iStateBits & HLB_PRESENCE_STATE_JOINABLE) ? TRUE : FALSE);
}



/*F********************************************************************************/
/*!
    \Function HLBBudGetVOIPState

    \Description
        Not implemented on PSP2.

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudGetVOIPState ( HLBBudT  *pBuddy )
{ 
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBBudCanVoiceChat

    \Description
        Indicates if buddy can be talked to. Not implemented on PSP2

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - TRUE if buddy can be talked to

    \Notes
        TODO: we could implement this with iStateBits or other HLB-held data.
        TODO: the comments mention that the buddy may need to be in the same game
        in order to talk to him. We may need to add that.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudCanVoiceChat(HLBBudT *pBuddy)
{
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function HLBBudJoinGame

    \Description
        Join buddy in another game; not implmeneted on PSP2.

    \Input *pBuddy  - pointer to buddy record
    \Input *cb      - callback function - ignored
    \Input pContext - ignored

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBBudJoinGame(HLBBudT *pBuddy, HLBOpCallbackT *cb, void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBMsgListGetMsgByIndex

    \Description
        Get message from indicated buddy at index position. 

    \Input *pBuddy      - pointer to buddy record
    \Input iIndex       - zero based index position to read message from
    \Input bRead        - boolean - if true, message will be marked as read - ignored.

    \Output
        BuddyApiMsgT*   - NULL

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
BuddyApiMsgT *HLBMsgListGetMsgByIndex(HLBBudT *pBuddy, int32_t iIndex, int32_t bRead)
{
    HLBMessageT *pMessage = NULL;

    if ((pBuddy == NULL) || (pBuddy->pMessageList == NULL))
    {
        return(NULL);
    }

    pMessage = DispListIndex(pBuddy->pMessageList, iIndex);

    if (pMessage != NULL)
    {
        if (bRead)
        {
            pMessage->BuddyMsg.uUserFlags |= HLB_MSG_READ_FLAG;
        }

        return(&pMessage->BuddyMsg);
    }
	return(NULL);
}

/*F********************************************************************************/
/*!
    \Function HLBMsgListGetFirstUnreadMsg

    \Description
        Get first unread message from the indicated buddy.

    \Input *pBuddy      - pointer to buddy record
    \Input iStartPos    - zero based index position to start looking for unread messages
    \Input bRead        - boolean - if true, message will be marked as read.

    \Output
        BuddyApiMsgT* - NULL

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
BuddyApiMsgT *HLBMsgListGetFirstUnreadMsg(HLBBudT *pBuddy, int32_t iStartPos, int32_t bRead)
{
    int32_t iListCount, iIndex;
    HLBMessageT *pMessage = NULL;

    if ((pBuddy == NULL) || (pBuddy->pMessageList == NULL))
    {
        return(NULL);
    }

    iListCount = DispListCount(pBuddy->pMessageList);

    for (iIndex=iStartPos, pMessage=DispListIndex(pBuddy->pMessageList, iIndex); 
         iIndex < iListCount; 
         iIndex++, pMessage=DispListIndex(pBuddy->pMessageList, iIndex))
    {
        if ((pMessage != NULL) && ((pMessage->BuddyMsg.uUserFlags & HLB_MSG_READ_FLAG) == 0))
        {
            if (bRead)
            {
                pMessage->BuddyMsg.uUserFlags |= HLB_MSG_READ_FLAG;
            }
            return(&pMessage->BuddyMsg);
        }
    }

	return(NULL);
}

/*F********************************************************************************/
/*!
    \Function HLBMsgListGetMsgText

    \Description
        Get message text from message. Not implemented on PSP2.

    \Input *pHLBApi - pointer to HLBApi module reference
    \Input *pMsg    - message to read text from
    \Input *strText - buffer to hold text
    \Input iLen     - size of buffer

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
void HLBMsgListGetMsgText(HLBApiRefT *pHLBApi, BuddyApiMsgT *pMsg, char *strText, int32_t iLen)
{	
    if ((pHLBApi == NULL) || (pMsg == NULL))
    {
        return;
    }

    if (pHLBApi->pTransTbl != NULL)
    {
        Utf8TranslateTo8Bit(strText, iLen, pMsg->pData, '*', pHLBApi->pTransTbl);
    }
    else
    {
        memcpy(strText, pMsg->pData, iLen);
    }
}

/*F********************************************************************************/
/*!
    \Function HLBMsgListGetUnreadCount

    \Description
        Get number of unread messages from this buddy.

    \Input *pBuddy - pointer to buddy record

    \Output
        int32_t     - the number of unread messages from this buddy

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBMsgListGetUnreadCount(HLBBudT *pBuddy)
{
    int32_t iListCount, iIndex, iUnreadCount=0;
    HLBMessageT *pMessage = NULL;

    if (pBuddy == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }

    if (pBuddy->pMessageList == NULL)
    {
        return(0);
    }

    iListCount = DispListCount(pBuddy->pMessageList);

    for (iIndex=0, pMessage=DispListIndex(pBuddy->pMessageList, iIndex); 
         iIndex < iListCount; 
         iIndex++, pMessage=DispListIndex(pBuddy->pMessageList, iIndex))
    {
        if ((pMessage != NULL) && ((pMessage->BuddyMsg.uUserFlags & HLB_MSG_READ_FLAG) == 0))
        {
            iUnreadCount++;
        }
    }

	return(iUnreadCount);
}

/*F********************************************************************************/
/*!
    \Function HLBMsgListGetTotalCount

    \Description
        Get number of total messages from this buddy.

    \Input *pBuddy  - pointer to buddy record

    \Output
        int32_t     - the total number of messages from this buddy

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBMsgListGetTotalCount(HLBBudT *pBuddy)
{
    if (pBuddy == NULL)
    {
        return(HLB_ERROR_BADPARM);
    }

    if (pBuddy->pMessageList == NULL)
    {
        return(0);
    }

    return(DispListCount(pBuddy->pMessageList));
}

/*F********************************************************************************/
/*!
    \Function HLBMsgListDelete

    \Description
        Delete the indicated message from this buddy.

    \Input *pBuddy  - pointer to buddy record
    \Input iIndex   - index of message to delete.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
void HLBMsgListDelete(HLBBudT *pBuddy , int32_t iIndex)
{
    if (pBuddy == NULL)
    {
        return;
    }

    _HLBDeleteMessageAndFree(pBuddy, iIndex);
}

/*F********************************************************************************/
/*!
    \Function HLBMsgListDeleteAll

    \Description
        Delete all messages from this buddy.

    \Input *pBuddy      - pointer to buddy record
    \Input bZapReadOnly - delete only the messages that have been read.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
void HLBMsgListDeleteAll(HLBBudT *pBuddy, int32_t bZapReadOnly)
{
    int32_t iListCount, iIndex;
    HLBMessageT *pMessage = NULL;

    if (pBuddy == NULL)
    {
        return;
    }
    if (pBuddy->pMessageList == NULL)
    {
        return;
    }

    iListCount = DispListCount(pBuddy->pMessageList);

    for (iIndex=0, pMessage=DispListIndex(pBuddy->pMessageList, iIndex); 
         iIndex < iListCount; 
         iIndex++, pMessage=DispListIndex(pBuddy->pMessageList, iIndex))
    {
        if (pMessage != NULL)
        {
            if ((!bZapReadOnly) || ((pMessage->BuddyMsg.uUserFlags & HLB_MSG_READ_FLAG) > 0))
            {
                _HLBDeleteMessageAndFree(pBuddy, iIndex);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function HLBListGetBuddyByName

    \Description
        Get a buddy from the buddy list by name. Exact match

    \Input *pHLBApi - pointer to HLBApi module reference
    \Input *pName   - name of the buddy to get

    \Output
        HLBBudT*    - pointer to buddy. NULL if not found.

    \Version 07/28/2006 (tdills)
*/
/********************************************************************************F*/
HLBBudT *HLBListGetBuddyByName(HLBApiRefT* pHLBApi, const char *pName)
{
    HLBBudT *pBuddy = NULL;
    DispListT *pRec;

    // find user record
    pRec = HashStrFind(pHLBApi->pHash, pName);
    if (pRec != NULL)
    {
        pBuddy = DispListGet(pHLBApi->pDisp, pRec);
    }
    return(pBuddy);
}

/*F********************************************************************************/
/*!
    \Function HLBListGetBuddyByIndex

    \Description
        Get a Buddy by index into the buddy list.

    \Input *pHLBApi - pointer to HLBApi module reference
    \Input iIndex   - index into the buddy list.

    \Output
        HLBBudT*    - pointer to the buddy record.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
HLBBudT* HLBListGetBuddyByIndex(HLBApiRefT *pHLBApi, int32_t iIndex)
{ 
    HLBBudT *pBuddy = DispListIndex(pHLBApi->pDisp, iIndex);

    if (pBuddy == NULL)
    {
        HLBPrintf((pHLBApi, "buddy #%d not found.", iIndex));
    }

    return(pBuddy);
}

/*F********************************************************************************/
/*!
    \Function HLBListGetBuddyCount

    \Description
        Get count of all buddies in the Roster. Also see HLBListGetBuddyCountByFlags.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - number of buddies in the list.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListGetBuddyCount (HLBApiRefT *pHLBApi)
{
    //set all flags to 1, to count all buddies in the list.
    return(_HLBRosterCount(pHLBApi, -1));
}
 
/*F********************************************************************************/
/*!
    \Function HLBListGetBuddyCountByFlags

    \Description
        Get count of buddies that match. Not supported on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input uTypeFlag    - flags to match

    \Output
        int32_t         - number of buddies in the list.

    \Notes
        TODO: if we implement the flags in _HLBRosterCount this will work.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListGetBuddyCountByFlags(HLBApiRefT *pHLBApi, uint32_t uTypeFlag)
{     
    return(_HLBRosterCount(pHLBApi, uTypeFlag));
}
   
/*F********************************************************************************/
/*!
    \Function HLBListBlockBuddy

    \Description
        Add a buddy to the list of ignored buddies. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - name of buddy
    \Input *cb          - completion callback function - ignored
    \Input *pContext    - context data for callback function - ignored.
    
    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListBlockBuddy(HLBApiRefT *pHLBApi, const char *strName, HLBOpCallbackT *cb, void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListUnBlockBuddy

    \Description
        Remove a buddy from the list of ignored buddies. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - name of buddy
    \Input *cb          - completion callback function - ignored
    \Input *pContext    - context data for callback function - ignored.

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListUnBlockBuddy(HLBApiRefT *pHLBApi, const char *strName, HLBOpCallbackT *cb, void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListUnMakeBuddy

    \Description
        Ensure user is no longer a buddy of mine. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - name of buddy
    \Input *cb          - completion callback function - ignored
    \Input *pContext    - context data for callback function - ignored.

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListUnMakeBuddy(HLBApiRefT  *pHLBApi, const char *strName, HLBOpCallbackT *cb, void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}

 
/*F********************************************************************************/
/*!
    \Function HLBListInviteBuddy

    \Description
        Ask this user to be my buddy. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - name of buddy
    \Input *cb          - completion callback function - ignored
    \Input *pContext    - context data for callback function - ignored.

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListInviteBuddy(HLBApiRefT *pHLBApi, const char *strName, HLBOpCallbackT *cb, void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListAnswerInvite

    \Description
        Respond to a buddy invitation. Not implemented on PSP2.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *strName         - name of buddy
    \Input eInviteAction    - action to take for the invitation
    \Input *cb              - completion callback function - ignored
    \Input *pContext        - context data for callback function - ignored.

    \Output
        int32_t             - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListAnswerInvite(HLBApiRefT *pHLBApi, const char *strName,
                            /*HLBInviteAction_Enum */int32_t eInviteAction,
                            HLBOpCallbackT *cb, void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListAnswerGameInvite

    \Description
        Respond to a game invitation.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *strName         - name of buddy
    \Input eInviteAction    - action to take for the invitation
    \Input *cb              - completion callback function - ignored
    \Input *pContext        - context data for callback function - ignored.

    \Output
        int32_t             - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListAnswerGameInvite(HLBApiRefT *pHLBApi, const char *strName,
                                /*HLBInviteAnswer_Enum */int32_t eInviteAction, 
                                HLBOpCallbackT *cb , void *pContext)
{
    // int32_t iNpStatus = 0;
    // int32_t iStatus = HLB_ERROR_NONE;

    HLBPrintf((pHLBApi, "HLBListAnswerGameInvite started Action = %d", eInviteAction));

    return(HLB_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function HLBListCancelGameInvite

    \Description
        Cancel any game invites I sent. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *cb          - completion callback function - ignored
    \Input *pContext    - context data for callback function - ignored.

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListCancelGameInvite(HLBApiRefT *pHLBApi, HLBOpCallbackT *cb, void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListGameInviteBuddy

    \Description
        Invite buddy to game.

    \Input *pHLBApi             - pointer to HLBApi module reference
    \Input *strName             - name of buddy
    \Input *pGameInviteState    - ignored
    \Input *cb                  - completion callback function
    \Input *pContext            - context data for callback function

    \Output
        int32_t                 - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListGameInviteBuddy(HLBApiRefT *pHLBApi, const char *strName,
                               const char *pGameInviteState, HLBOpCallbackT *cb,
                               void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}
 
/*F********************************************************************************/
/*!
    \Function HLBListFlagTempBuddy

    \Description
        Temporarily flag a buddy as a temp or tourney buddy for convenience. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - name of buddy
    \Input iBuddyType   - buddy type to set - see TempBuddyType enum

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListFlagTempBuddy(HLBApiRefT *pHLBApi, const char *strName, int32_t iBuddyType)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListUnFlagTempBuddy

    \Description
        Remove a temporary buddy type flag from a buddy if set. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - name of buddy
    \Input iBuddyType   - buddy type to unset - see TempBuddyType enum

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListUnFlagTempBuddy(HLBApiRefT *pHLBApi, const char *strName, int32_t iBuddyType)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListDeleteTempBuddy

    \Description
        Delete a temporary buddy. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - name of buddy

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListDeleteTempBuddy(HLBApiRefT *pHLBApi, const char *strName)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListSetSortFunction

    \Description
        Set the sort function if the client wants to use a custom sort for the buddy list.
        The API has a reasonable default sort.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *pSortFunction   - pointer to custom sort function

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
void HLBListSetSortFunction(HLBApiRefT *pHLBApi, HLBRosterSortFunctionT *pSortFunction)
{
    pHLBApi->pBuddySortFunc = pSortFunction;
}

/*F********************************************************************************/
/*!
    \Function HLBListSort

    \Description
        Sort the buddy list now.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - BUDDY_ERROR_NONE

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListSort(HLBApiRefT *pHLBApi)
{
	if ( pHLBApi->pDisp != NULL)
	{
		DispListSort(pHLBApi->pDisp ,  pHLBApi , 0,  pHLBApi->pBuddySortFunc);
		// Force the list to appear to be changed so it will get resorted.
		DispListChange(pHLBApi->pDisp , 1);
		// Resort the list.
		DispListOrder(pHLBApi->pDisp);
	}
	return(BUDDY_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function HLBListChanged

    \Description
        Has the list of buddies or the presence data changed? Clears the flag.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - HLB_CHANGED if it has changed; HLB_NO_CHANGES if not.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListChanged(HLBApiRefT *pHLBApi)
{
    int32_t rc = HLB_NO_CHANGES;
    if (pHLBApi->bRosterDirty)
    {
        DispListOrder(pHLBApi->pDisp);
        pHLBApi->bRosterDirty = FALSE;
        rc = HLB_CHANGED;
    }
    return(rc);
}

/*F********************************************************************************/
/*!
    \Function HLBListBuddyWithMsg

    \Description
        Do we have messages from any buddies? Not implemented on PSP2.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED 

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListBuddyWithMsg(HLBApiRefT *pHLBApi)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListDeleteTempBuddies

    \Description
        Clear temp buddies without messages. Not implemented on PSP2.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED 

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListDeleteNoMsgTempBuddies(HLBApiRefT *pHLBApi)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListAddToGroup

    \Description
        Add a buddy to the group to which I will send a common message. Not implemented on PSP2.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *strBuddyName    - buddy to add

    \Output
        int32_t             - HLB_ERROR_UNSUPPORTED 

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListAddToGroup(HLBApiRefT *pHLBApi, const char *strBuddyName) 
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListRemoveFromGroup

    \Description
        Remove a buddy from the group to which I will send a common message. Not implemented on PSP2.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *strBuddyName    - buddy to remove

    \Output
        int32_t             - HLB_ERROR_UNSUPPORTED 

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListRemoveFromGroup(HLBApiRefT *pHLBApi, const char *strBuddyName)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListClearGroup

    \Description
        Remove all users from the group. Not implemented on PSP2.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED 

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListClearGroup(HLBApiRefT *pHLBApi) 
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListSendMsgToGroup

    \Description
        Send a message to the group of buddies. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input eMsgType     - type of message to send
    \Input strMessage   - message text
    \Input bClearGroup  - clear the group after sending?
    \Input *cb          - completion callback function
    \Input *pContext    - context data to send to callback function

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListSendMsgToGroup(HLBApiRefT  *pHLBApi, /*HLBMsgType_Enum*/int32_t eMsgType, const char *strMessage, 
                              int32_t bClearGroup, HLBOpCallbackT *cb, void *pContext)  
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiPresenceDiff

    \Description
        Set presence for other products, in appropriate language. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pGlobalPres - presence data

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiPresenceDiff(HLBApiRefT *pHLBApi, const char *pGlobalPres)
{
    return;  //HLB_ERROR_UNSUPPORTED
}

/*F********************************************************************************/
/*!
    \Function HLBApiPresenceSame

    \Description
        Set my presence state and presence string for this user.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input iPresState   - presence state flags see HLB_PRESENCE_STATE_*
    \Input strPresence  - 64-bytes of title-specific presence data

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiPresenceSame(HLBApiRefT *pHLBApi, int32_t iPresState, const char *strPresence)
{
    pHLBApi->BuddyData.Presence.iStateBits = iPresState;
    memcpy(pHLBApi->BuddyData.Presence.strTitleData, strPresence, sizeof(pHLBApi->BuddyData.Presence.strTitleData));
    return;
}

/*F********************************************************************************/
/*!
    \Function HLBApiPresenceJoinable

    \Description
        Set or clear the joinable flag in the current user's presence data

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input bOn              - set (TRUE) or clear (FALSE)
    \Input bSendImmediate   - immediately send the updated presence data?

    \Output
        see HLB_ERROR_*

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiPresenceJoinable(HLBApiRefT *pHLBApi, int32_t bOn, int32_t bSendImmediate)
{
    int32_t bDirty = FALSE;

    if (((pHLBApi->BuddyData.Presence.iStateBits & HLB_PRESENCE_STATE_JOINABLE) > 0) == bOn)
    {
        if (bOn)
        {
            pHLBApi->BuddyData.Presence.iStateBits &= HLB_PRESENCE_STATE_JOINABLE;
            bDirty = TRUE;
        }
        else
        {
            pHLBApi->BuddyData.Presence.iStateBits &= ~HLB_PRESENCE_STATE_JOINABLE;
            bDirty = TRUE;
        }
    }

    if (bDirty && bSendImmediate)
    {
        return(_HLBSendPresenceData(pHLBApi));
    }

    return(HLB_ERROR_NONE);
}


/*F********************************************************************************/
/*!
    \Function HLBApiPresenceExtra

    \Description
        Set extra presence data. No implemented on PSP2.

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input *strExtraPres    - extra presence data

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
void HLBApiPresenceExtra(HLBApiRefT *pHLBApi, const char *strExtraPres)
{
    return;
}

/*F********************************************************************************/
/*!
    \Function HLBApiPresenceOffline

    \Description
        Set presence to offline. Not implemented on PSP2.

    \Input *pHLBApi - pointer to HLBApi module reference
    \Input bOnOff   - set to offline (TRUE) or online (FALSE)

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t  HLBApiPresenceOffline (  HLBApiRefT  * pHLBApi, int32_t bOnOff)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiPresenceSend

    \Description
        Send updated presence data

    \Input *pHLBApi         - pointer to HLBApi module reference
    \Input iBuddyState      - see HLBStatE enumeration
    \Input iVOIPState       - ignored
    \Input strTitleData     - sets the title presence data
    \Input iWaitMsecs       - ignored

    \Output
        int32_t             - HLB_ERROR_*

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiPresenceSend(HLBApiRefT *pHLBApi, /*HLBStatE*/int32_t iBuddyState, int32_t iVOIPState, 
                           const char *strTitleData, int32_t iWaitMsecs) 
{
    if (!pHLBApi)
    {
        return(HLB_ERROR_BADPARM);
    }

    if (pHLBApi->eConnectState != HLB_CS_Connected)
    {
        return(HLB_ERROR_OFFLINE);
    }

    memcpy(pHLBApi->BuddyData.Presence.strTitleData, strTitleData, sizeof(pHLBApi->BuddyData.Presence.strTitleData));

    return(_HLBSendPresenceData(pHLBApi));
}

/*F********************************************************************************/
/*!
    \Function HLBApiPresenceVOIPSend

    \Description
        Update voice state and send presence data. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input iVOIPState   - ignored

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiPresenceVOIPSend(HLBApiRefT *pHLBApi, /*HLBVOIPE*/int32_t iVOIPState)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiPresenceSendSetPresence

    \Description
        Send to server, previously set presence state and presence strings.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - HLB_ERROR_*

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiPresenceSendSetPresence(HLBApiRefT *pHLBApi)
{
    return(_HLBSendPresenceData(pHLBApi));
}

/*F********************************************************************************/
/*!
    \Function HLBListSendChatMsg

    \Description
        Send a chat message to a buddy in the list. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - name of buddy to send message to
    \Input *strMessage  - the message
    \Input *cb          - completion callback function
    \Input *pContext    - context data to send to completion callback

    \Output
        int32_t         - HLB_ERROR_*

    \Notes
        TODO: Can be implemented using sony libraries.

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListSendChatMsg(HLBApiRefT *pHLBApi, const char *strName, const char *strMessage,
                           HLBOpCallbackT *cb , void *pContext)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiSetEmailForwarding

    \Description
        Turn email forwarding on or off. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input bEnable      - turn on or off.
    \Input *strEmail    - the email address to forward to
    \Input *pOpCallback - optional completion callback
    \Input *pCalldata   - context data to send to completion callback

    \Output
        int32_t         - HLB_ERROR_*

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiSetEmailForwarding(HLBApiRefT *pHLBApi, int32_t bEnable, const char *strEmail,
                                 HLBOpCallbackT *pOpCallback, void *pCalldata)
{
    return(HLB_ERROR_UNSUPPORTED);
}
 
/*F********************************************************************************/
/*!
    \Function HLBApiGetEmailForwarding

    \Description
        Get email forwarding state. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *bEnable     - [out] indicates the state of the enabled flag
    \Input *strEmail    - [out] the email address

    \Output
        int32_t         - HLB_ERROR_*

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiGetEmailForwarding(HLBApiRefT *pHLBApi, int32_t *bEnable, char *strEmail)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBBudGetGameInviteFlags

    \Description
        Get state of cross game invites. Not implemented on PSP2.

    \Input *pBuddy  - buddy with which invites are associated

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
uint32_t HLBBudGetGameInviteFlags(HLBBudT *pBuddy)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiSetUserIndex

    \Description
        Set the enumerator index for XFriendsCreateEnumerator. Not implemented on PSP2.

    \Input *pHLBApi - pointer to HLBApi module reference
    \Input iIndex   - the enumerator index

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiSetUserIndex(HLBApiRefT *pHLBApi, int32_t iIndex)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiGetUserIndex

    \Description
        Get the enumerator index for XFriendsCreateEnumerator. Not implemented on PSP2.

    \Input *pHLBApi - pointer to HLBApi module reference

    \Output
        int32_t     - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiGetUserIndex(HLBApiRefT *pHLBApi)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiGetTitleName

    \Description
        Convert a title id into a title string. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input uTitleId     - title id for which to retrieve the name
    \Input iBufSize     - size of buffer pointed to by pTitleName
    \Input pTitleName   - buffer to receive the title

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiGetTitleName(HLBApiRefT *pHLBApi, const uint32_t uTitleId, const int32_t iBufSize, char *pTitleName)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBApiGetMyTitleName

    \Description
        Get the title string for the currently running title. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *pLang       - ignored
    \Input iBufSize     - size of buffer pointed to by pTitleName
    \Input pTitleName   - buffer to receive the title

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBApiGetMyTitleName(HLBApiRefT *pHLBApi, const char *pLang, const int32_t iBufSize, char *pTitleName)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F********************************************************************************/
/*!
    \Function HLBListSetGameInviteSessionID

    \Description
        Set the session id that is passed to the game invitation. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *sSessionID  - the session ID

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListSetGameInviteSessionID(HLBApiRefT *pHLBApi, const char *sSessionID)
{
    return(HLB_ERROR_UNSUPPORTED);
}

// Get the SessionID for a particular user for game invites 
/*F********************************************************************************/
/*!
    \Function HLBListGetGameSessionID

    \Description
        Get the SessionID for a particular user for game invites. Not implemented on PSP2.

    \Input *pHLBApi     - pointer to HLBApi module reference
    \Input *strName     - the user's name
    \Input *sSessionID  - the buffer to receive the session id
    \Input iBufSize     - the size of the buffer pointed to by sSessionID

    \Output
        int32_t         - HLB_ERROR_UNSUPPORTED

    \Version 07/31/2006 (tdills)
*/
/********************************************************************************F*/
int32_t HLBListGetGameSessionID(HLBApiRefT *pHLBApi, const char *strName, char *sSessionID, const int32_t iBufSize)
{
    return(HLB_ERROR_UNSUPPORTED);
}

/*F**************************************************************************************/
/*!
    \Function HLBAddPlayersToHistory

    \Description
        Add player(s) to player history list

    \Input *pHLBApi     - hlbud ref
    \Input *pPlyrAddrs[]- array of pointers to DirtyAddrTs for players to add
    \Input iNumAddrs    - number of addresses in array

    \Version 09/01/2006 (jbrookes)
*/
/**************************************************************************************F*/
void HLBAddPlayersToHistory(HLBApiRefT *pHLBApi, DirtyAddrT *pPlyrAddrs[], int32_t iNumAddrs)
{
}
