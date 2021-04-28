/*H********************************************************************************/
/*!
    \File friendapiprivps3.c

    \Description
        PS3-specific portion of the friend api module

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <string.h>
#include <np.h>
#include <np\basic.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/friend/friendapi.h"
#include "friendapipriv.h"

/*** Defines **********************************************************************/

#define SCE_NP_BASIC_GET_EVENT_MESSAGE_LENGTH   (1024) // We need to have enough space to receive entire message

/*** Type Definitions *************************************************************/

//! PS3 friend structure
typedef struct Ps3FriendT
{
    uint8_t bValid;        //!< whether this friend entry is valid or not
    SceNpUserInfo userInfo;
    uint8_t bPresent;
    int32_t iPresenceDataSize;
    int8_t strPresenceData[SCE_NP_BASIC_MAX_PRESENCE_SIZE]; // TODO:This is making the friendslist a lot bigger on PS3 now
} Ps3FriendT;

//! PS3 friendslist
typedef struct Ps3FriendsListT
{
    Ps3FriendT aFriends[FRIENDAPI_MAX_FRIENDS];
} Ps3FriendsListT;

//! PS3 blocked user structure
typedef struct Ps3BlockedUserT
{
    SceNpId npId;
    uint8_t bValid;        //!< whether this user entry is valid or not
} Ps3BlockedUserT;

//! PS3 blocklist
typedef struct Ps3BlockListT
{
    Ps3BlockedUserT aUsers[FRIENDAPI_MAX_BLOCKED_USERS];
} Ps3BlockListT;

//! PS3 variables that should be accessed only when resource locker is acquired
typedef struct Ps3CallbackCritT
{
    NetCritT locker;                    //!< critical section used to ensure thread safety in handler registered with NP Basic utility

    int32_t iEventsInQueue;             //!< number of NP basic events queued
    int8_t bGetBlockList;               //!< indicates that it is time to go get the block list from Sony
    int8_t bEndOfInitialPresence;       //!< whether initial presence update is completed or not
} Ps3CallbackCritT;

//! friend api module state
struct FriendApiRefT
{
    FriendApiCommonRefT common;         //!< platform-independent portion. This member should always be the first member of FriendApiRefT

    Ps3FriendsListT friendsList;        //!< list of friends
    Ps3BlockListT blockList;            //!< list of block users

    Ps3CallbackCritT callbackCrit;

    uint8_t bNpBasicHandlerRegistered;  //!< whether _NpBasicHandler was registered or not
    uint8_t _pad[3];                    //!< padding
};

/*** Function Prototypes **********************************************************/

/*** Private functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _FriendApiPrivNotifyEventInitComplete

    \Description
        Generate events until the entire friendlist and blocklist were signaled to a newly registered callback, followed by a
        FRIENDAPI_EVENT_INIT_COMPLETE event

    \Input *pFriendApiRef   - module reference

    \Version 08/25/2012 (akirchner)
*/
/********************************************************************************F*/
static void _FriendApiPrivNotifyEventInitComplete(FriendApiRefT *pFriendApiRef)
{
    FriendDataT friendData;
    FriendEventDataT eventData;
    int32_t iCallbackIndex;
    int32_t iFriendIndex;
    int32_t iNpEvent;
    int32_t iNpStatus = 0;
    int8_t bEndOfInitialPresence;

    NetCritEnter(&pFriendApiRef->callbackCrit.locker);
    bEndOfInitialPresence = pFriendApiRef->callbackCrit.bEndOfInitialPresence;
    NetCritLeave(&pFriendApiRef->callbackCrit.locker);

    NetCritEnter(&pFriendApiRef->common.notifyCrit);

    if (bEndOfInitialPresence == TRUE)
    {
        for (iCallbackIndex = 0; iCallbackIndex < FRIENDAPI_MAX_CALLBACKS; iCallbackIndex++)
        {
            if ((pFriendApiRef->common.callback[iCallbackIndex].pNotifyCb) && (pFriendApiRef->common.callback[iCallbackIndex].bInitFinished[0] == FALSE))
            {
                for (iFriendIndex = 0; iFriendIndex < FRIENDAPI_MAX_FRIENDS; iFriendIndex++)
                {
                    if (pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid)
                    {
                        if (pFriendApiRef->friendsList.aFriends[iFriendIndex].bPresent == TRUE)
                        {
                            iNpEvent = SCE_NP_BASIC_EVENT_PRESENCE;
                        }
                        else
                        {
                            iNpEvent = SCE_NP_BASIC_EVENT_OFFLINE;
                        }

                        memset(&friendData, 0, sizeof(FriendDataT));
                        strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
                        strncpy(friendData.strFriendName, pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo.userId.handle.data, sizeof(friendData.strFriendId));
                        friendData.pRawData1 = &pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo;

                        memset(&eventData, 0, sizeof(FriendEventDataT));

                        eventData.eventType = FRIENDAPI_EVENT_FRIEND_UPDATED;
                        eventData.eventTypeData.FriendUpdated.pFriendData = &friendData;
                        eventData.eventTypeData.FriendUpdated.bPresence = pFriendApiRef->friendsList.aFriends[iFriendIndex].bPresent;
                        eventData.eventTypeData.FriendUpdated.pRawData1 = & pFriendApiRef->friendsList.aFriends[iFriendIndex].iPresenceDataSize;
                        eventData.eventTypeData.FriendUpdated.pRawData2 = pFriendApiRef->friendsList.aFriends[iFriendIndex].strPresenceData;
                        eventData.pRawData1 = &iNpEvent;
                        eventData.pRawData2 = &iNpStatus;

                        pFriendApiRef->common.callback[iCallbackIndex].pNotifyCb(pFriendApiRef, &eventData, pFriendApiRef->common.callback[iCallbackIndex].pNotifyCbUserData);
                    }
                }

                memset(&eventData, 0, sizeof(FriendEventDataT));

                eventData.eventType = FRIENDAPI_EVENT_INIT_COMPLETE;

                pFriendApiRef->common.callback[iCallbackIndex].pNotifyCb(pFriendApiRef, &eventData, pFriendApiRef->common.callback[iCallbackIndex].pNotifyCbUserData);
                pFriendApiRef->common.callback[iCallbackIndex].bInitFinished[0] = TRUE;
            }
        }
    }

    NetCritLeave(&pFriendApiRef->common.notifyCrit);
}

/*F********************************************************************************/
/*!
    \Function _NpBasicHandler

    \Description
        Receives callback events from PS3 library.

    \Input iEvent       - the event type
    \Input iRetCode     - return code
    \Input uRequestId   - id of the request
    \Input *pArg        - user data

    \Output
        int             - 0 if handled.

    \Notes
        This callback is now thread-safe. It uses a critical section to access
        variables that are shared with other functions of friendapiprivps3. Thread-safety
        is required because we cannot guarantee that cellSysutilCheckCallback()
        is going to be called from the same thread as DirtySDK... and that function
        is the one that internally invokes the callback we register with the NP 
        Basic utility.

    \Version 04/14/2010 (mclouatre)
*/
/********************************************************************************F*/
static int _NpBasicHandler(int iEvent, int iRetCode, uint32_t uRequestId, void *pArg)
{
    FriendEventDataT eventData;

    FriendApiRefT* pFriendApiRef = (FriendApiRefT*)pArg;
    uint8_t bDelayedNotifyEvent = FALSE;
    FriendApiEventsE eventType = FRIENDAPI_EVENT_UNKNOWN;

    #if DIRTYCODE_LOGGING
    static char *_strEventNames[] =
    {
        "SCE_NP_BASIC_EVENT_UNKNOWN",
        "SCE_NP_BASIC_EVENT_OFFLINE",
        "SCE_NP_BASIC_EVENT_PRESENCE",
        "SCE_NP_BASIC_EVENT_MESSAGE",
        "SCE_NP_BASIC_EVENT_ADD_FRIEND_RESULT",
        "SCE_NP_BASIC_EVENT_INCOMING_ATTACHMENT",
        "SCE_NP_BASIC_EVENT_INCOMING_INVITATION",
        "SCE_NP_BASIC_EVENT_END_OF_INITIAL_PRESENCE",
        "SCE_NP_BASIC_EVENT_SEND_ATTACHMENT_RESULT",
        "SCE_NP_BASIC_EVENT_RECV_ATTACHMENT_RESULT",
        "SCE_NP_BASIC_EVENT_OUT_OF_CONTEXT",
        "SCE_NP_BASIC_EVENT_FRIEND_REMOVED",
        "SCE_NP_BASIC_EVENT_ADD_BLOCKLIST_RESULT",
        "SCE_NP_BASIC_EVENT_SEND_MESSAGE_RESULT",
        "SCE_NP_BASIC_EVENT_SEND_INVITATION_RESULT",
        "SCE_NP_BASIC_EVENT_RECV_INVITATION_RESULT",
        "SCE_NP_BASIC_EVENT_MESSAGE_MARKED_AS_USED_RESULT",
        "SCE_NP_BASIC_EVENT_INCOMING_CUSTOM_INVITATION",
        "SCE_NP_BASIC_EVENT_INCOMING_CLAN_MESSAGE",
        "SCE_NP_BASIC_EVENT_ADD_PLAYERS_HISTORY_RESULT",
        "SCE_NP_BASIC_EVENT_SEND_CUSTOM_DATA_RESULT",
        "SCE_NP_BASIC_EVENT_RECV_CUSTOM_DATA_RESULT",
        "SCE_NP_BASIC_EVENT_INCOMING_CUSTOM_DATA_MESSAGE",
        "SCE_NP_BASIC_EVENT_SEND_URL_ATTACHMENT_RESULT",
        "SCE_NP_BASIC_EVENT_INCOMING_BOOTABLE_INVITATION",
        "SCE_NP_BASIC_EVENT_BLOCKLIST_UPDATE",
        "SCE_NP_BASIC_EVENT_INCOMING_BOOTABLE_CUSTOM_DATA_MESSAGE"
    };

    if ((unsigned)(iEvent+1) < (sizeof(_strEventNames)/sizeof(_strEventNames[0])) )
    {
        NetPrintf(("friendapiprivps3: received NpBasicEvent iEvent=%s/%d; iRetCode=%s; uRequestId=%d\n", _strEventNames[iEvent+1], iEvent, DirtyErrGetName(iRetCode), uRequestId));
    }
    else
    {
        NetPrintf(("friendapiprivps3: received NpBasicEvent iEvent=%d; iRetCode=%s; uRequestId=%d\n", iEvent, DirtyErrGetName(iRetCode), uRequestId));
    }
    #endif

    // prepare event data
    memset(&eventData, 0, sizeof(eventData));
    // on ps3, the pRawData1 field of the eventData structure points to the PS3 event type value
    // and the pRawData2 field of the eventData structure points to the PS3 event error code
    // and the pRawData3 field of the eventData structure points to the request id
    eventData.pRawData1 = &iEvent;
    eventData.pRawData2 = &iRetCode;
    eventData.pRawData3 = &uRequestId;

    // the following variables can only be accessed in the context of the callbackCrit.locker
    // critical section: iEventsInQueue, bEndOfInitialPresence, bGetBlockList
    NetCritEnter(&pFriendApiRef->callbackCrit.locker);

    if (iEvent == SCE_NP_BASIC_EVENT_END_OF_INITIAL_PRESENCE)
    {
        // flag initial presence as completed
        pFriendApiRef->callbackCrit.bEndOfInitialPresence = TRUE;

        // tell the update function that it needs to get the list of blocked players
        pFriendApiRef->callbackCrit.bGetBlockList = TRUE;

        // this event type does not require a call to sceNpBasicGetEvent(), therefore we can report it to our clients immediately
        bDelayedNotifyEvent = FALSE;
    }
    else if (iEvent == SCE_NP_BASIC_EVENT_BLOCKLIST_UPDATE)
    {
        // tell the update function that it needs to get the list of blocked players
        pFriendApiRef->callbackCrit.bGetBlockList = TRUE;

        // this event type will be notified to our clients only when the locally maintained blocklist cache has been updated
        bDelayedNotifyEvent = TRUE;
    }
    else if (iEvent == SCE_NP_BASIC_EVENT_ADD_BLOCKLIST_RESULT)
    {
        /*
        Notes:
            --> This event occurs following a call to sceNpBasicAddBlockListEntry()
            --> no need to set bGetBlockList here. we will get another event SCE_NP_BASIC_EVENT_BLOCKLIST_UPDATE if the blocklist was indeed altered
        */

        if (iRetCode == 0)
        {
            eventData.eventTypeData.BlockUserResult.bSuccess = TRUE;
        }

        // this event type does not require a call to sceNpBasicGetEvent(), therefore we can report it to our clients immediately
        bDelayedNotifyEvent = FALSE;
        eventType = FRIENDAPI_EVENT_BLOCK_USER_RESULT;
    }
    else if (iEvent == SCE_NP_BASIC_EVENT_UNKNOWN ||
             iEvent == SCE_NP_BASIC_EVENT_ADD_FRIEND_RESULT ||
             iEvent == SCE_NP_BASIC_EVENT_SEND_ATTACHMENT_RESULT ||
             iEvent == SCE_NP_BASIC_EVENT_SEND_MESSAGE_RESULT ||
             iEvent == SCE_NP_BASIC_EVENT_SEND_INVITATION_RESULT ||
             iEvent == SCE_NP_BASIC_EVENT_MESSAGE_MARKED_AS_USED_RESULT ||
             iEvent == SCE_NP_BASIC_EVENT_ADD_PLAYERS_HISTORY_RESULT ||
             iEvent == SCE_NP_BASIC_EVENT_SEND_CUSTOM_DATA_RESULT ||
             iEvent == SCE_NP_BASIC_EVENT_SEND_URL_ATTACHMENT_RESULT)
    {
        // these event types do not require a call to sceNpBasicGetEvent(), therefore we can report them to our clients immediately
        bDelayedNotifyEvent = FALSE;
    }
    else
    {
        // all other event types require a call to sceNpBasicGetEvent(), therefore we cannot report them to our clients immediately
        bDelayedNotifyEvent = TRUE;
    }

    // if the PS3 event does not require a call to sceNpBasicGetEvent(), we can propagate that event to our clients right away
    if ( (iRetCode < 0) || (!bDelayedNotifyEvent) )
    {
        eventData.eventType = eventType;

        FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
    }
    else
    {
        // increment the number of events we need to pop off the queue
        pFriendApiRef->callbackCrit.iEventsInQueue++;
    }

    NetCritLeave(&pFriendApiRef->callbackCrit.locker);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiIncrementFriendslistVersion

    \Description
        Increment the version of the friendslist

    \Input *pFriendApiRef   - module reference

    \Version 06/29/2010 (mclouatre)
*/
/********************************************************************************F*/
static void _FriendApiIncrementFriendslistVersion(FriendApiRefT *pFriendApiRef)
{
#if DIRTYCODE_LOGGING
    int32_t iFriendsCount;
#endif

    pFriendApiRef->common.uFriendListVersion++;

#if DIRTYCODE_LOGGING
    FriendApiPrivGetFriendsList(pFriendApiRef, 0, NULL, NULL, &iFriendsCount);

    NetPrintf(("friendapiprivps3: friendslist updated (new friend count = %d, new version = %d)\n",
        iFriendsCount, pFriendApiRef->common.uFriendListVersion));
#endif
}

/*F********************************************************************************/
/*!
    \Function _FriendApiIncrementBlocklistVersion

    \Description
        Increment the version of the blocklist

    \Input *pFriendApiRef   - module reference

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _FriendApiIncrementBlocklistVersion(FriendApiRefT *pFriendApiRef)
{
#if DIRTYCODE_LOGGING
    int32_t iBlockedUsersCount;
#endif

    pFriendApiRef->common.uBlockListVersion++;

#if DIRTYCODE_LOGGING
    FriendApiPrivGetBlockList(pFriendApiRef, 0, NULL, &iBlockedUsersCount);

    NetPrintf(("friendapiprivps3: blocklist updated (new blocked user count = %d, new version = %d)\n",
        iBlockedUsersCount, pFriendApiRef->common.uBlockListVersion));
#endif
}

/*F********************************************************************************/
/*!
    \Function _FriendApiIsFriendInList

    \Description
        Check whether the specified friends is already in the friendslist or not

    \Input *pFriendApiRef   - module reference
    \Input npId             - friend's NP ID

    \Output
        int32_t             - returns friends index if friends is in list, returns -1 if friends is not in list

    \Version 06/29/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiIsFriendInList(FriendApiRefT *pFriendApiRef, SceNpId npId)
{
    int32_t iFriendIndex;
    int32_t iResult = -1;

    for (iFriendIndex = 0; iFriendIndex < FRIENDAPI_MAX_FRIENDS; iFriendIndex++)
    {
        if (pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid)
        {
            if (sceNpUtilCmpNpId(&pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo.userId, &npId) == 0)
            {
                iResult = iFriendIndex;
                break;
            }
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiUpdateFriend

    \Description
        Add a friend to the friendlist if it is not part of the list yet, update its presence, and cache the data received

    \Input *pFriendApiRef   - module reference
    \Input userInfo         - friend's NP user info
    \Input bPresent         - TRUE if user is present, and FALSE if it is not
    \Input iBufferSize      - size of strBuffer in bytes
    \Input strBuffer        - event data buffer

    \Output
        int32_t             - returns friends index if friends successfully added, returns -1 if list is full

    \Version 06/29/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiUpdateFriend(FriendApiRefT *pFriendApiRef, SceNpUserInfo userInfo, uint8_t bPresent, int32_t iBufferSize, int8_t *strBuffer)
{
    int32_t iFriendIndex;
    int32_t iResult;

    // is the friend already in the friendslist
    if ((iResult = _FriendApiIsFriendInList(pFriendApiRef, userInfo.userId)) < 0)
    {
        // add friend to a free spot in the friendslist
        for (iFriendIndex = 0; iFriendIndex < FRIENDAPI_MAX_FRIENDS; iFriendIndex++)
        {
            if (!pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid)
            {
                // add friend to free spot
                ds_memcpy_s(&pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo, sizeof(pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo), &userInfo, sizeof(userInfo));

                // mark friend as valid
                pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid = TRUE;

                // increment friendslist version number
                _FriendApiIncrementFriendslistVersion(pFriendApiRef);

                iResult = iFriendIndex;

                break;
            }
        }
    }

    if (iResult >= 0)
    {
        pFriendApiRef->friendsList.aFriends[iResult].bPresent = bPresent;
        pFriendApiRef->friendsList.aFriends[iResult].iPresenceDataSize = iBufferSize;

        if(strBuffer)
        {
            ds_memcpy_s(pFriendApiRef->friendsList.aFriends[iResult].strPresenceData, sizeof(pFriendApiRef->friendsList.aFriends[iResult].strPresenceData), strBuffer, iBufferSize);
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiDeleteFriend

    \Description
        Delete a friend from the friendlist.

    \Input *pFriendApiRef   - module reference
    \Input userInfo         - friend's NP user info

    \Output
        int32_t             - returns 0 if friends found and deleted, returns -1 if friends is not in list

    \Version 06/29/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiDeleteFriend(FriendApiRefT *pFriendApiRef, SceNpUserInfo userInfo)
{
    int32_t iFriendIndex;

    // is the friend in the friendlist
    if ((iFriendIndex = _FriendApiIsFriendInList(pFriendApiRef, userInfo.userId)) < 0)
    {
        // signal that the specified friend is not in the list
        return(-1);
    }

    // invalidate the friendlist entry
    memset(&pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo, 0, sizeof(pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo));
    pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid = FALSE;

    // update the friendslist version
    _FriendApiIncrementFriendslistVersion(pFriendApiRef);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiProcessEvent

    \Description
        Notify higher-level module of a new friend event.

    \Input *pFriendApiRef       - module reference

    \Version 04/25/2010 (mclouatre)
*/
/********************************************************************************F*/
static void _FriendApiProcessEvent(FriendApiRefT *pFriendApiRef)
{
    int32_t iNpStatus, iNpEvent;
    SceNpUserInfo userInfo;
    FriendEventDataT eventData;
    FriendDataT friendData;

    int8_t aDataBuf[SCE_NP_BASIC_GET_EVENT_MESSAGE_LENGTH]; 
    int32_t iDataBufSize = sizeof(aDataBuf);

    memset(&eventData, 0, sizeof(eventData));
    memset(&aDataBuf, 0, sizeof(aDataBuf));

    if ((iNpStatus = sceNpBasicGetEvent(&iNpEvent, &userInfo, &aDataBuf, (size_t *)&iDataBufSize)) == 0)
    {
        NetPrintf(("friendapiprivps3: popped event from queue: iNpEvent=%d\n", iNpEvent));

        if (iNpEvent == SCE_NP_BASIC_EVENT_UNKNOWN)
        {
            NetPrintf(("friendapiprivps3: an invalid event was received\n"));
        }
        else
        {
            int32_t iRetCode = 0;  // fake a success error code

            memset(&friendData, 0, sizeof(friendData));
            strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
            strncpy(friendData.strFriendName, userInfo.userId.handle.data, sizeof(friendData.strFriendId));
            friendData.pRawData1 = &userInfo;

            // on ps3, the pRawData1 field of the eventData structure points to the PS3 event type value
            // and the pRawData2 field of the eventData structure points to the PS3 event error code
            eventData.pRawData1 = &iNpEvent;
            eventData.pRawData2 = &iRetCode;

            switch (iNpEvent)
            {
            case SCE_NP_BASIC_EVENT_OFFLINE:
                NetPrintf(("friendapiprivps3: user %s is logged off\n", userInfo.userId.handle.data));
                eventData.eventType = FRIENDAPI_EVENT_FRIEND_UPDATED;
                eventData.eventTypeData.FriendUpdated.bPresence = FALSE;
                eventData.eventTypeData.FriendUpdated.pRawData1 = & iDataBufSize;
                eventData.eventTypeData.FriendUpdated.pFriendData = & friendData;

                _FriendApiUpdateFriend(pFriendApiRef, userInfo, FALSE, 0, NULL);
                FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
                break;

            case SCE_NP_BASIC_EVENT_OUT_OF_CONTEXT:
                NetPrintf(("friendapiprivps3: user %s is online but in a different NP communication ID\n", userInfo.userId.handle.data));
                eventData.eventType = FRIENDAPI_EVENT_FRIEND_UPDATED;
                eventData.eventTypeData.FriendUpdated.bPresence = FALSE;
                eventData.eventTypeData.FriendUpdated.pRawData1 = & iDataBufSize;
                eventData.eventTypeData.FriendUpdated.pFriendData = & friendData;

                _FriendApiUpdateFriend(pFriendApiRef, userInfo, TRUE, 0, NULL);
                FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
                break;

            case SCE_NP_BASIC_EVENT_PRESENCE:
                NetPrintf(("friendapiprivps3: received presence data from user %s: %s\n", userInfo.userId.handle.data, aDataBuf));
                eventData.eventType = FRIENDAPI_EVENT_FRIEND_UPDATED;
                eventData.eventTypeData.FriendUpdated.bPresence = TRUE;
                eventData.eventTypeData.FriendUpdated.pRawData1 = & iDataBufSize;
                eventData.eventTypeData.FriendUpdated.pRawData2 = aDataBuf;
                eventData.eventTypeData.FriendUpdated.pFriendData = & friendData;

                _FriendApiUpdateFriend(pFriendApiRef, userInfo, TRUE, iDataBufSize, aDataBuf);
                FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
                break;

            case SCE_NP_BASIC_EVENT_MESSAGE:
                NetPrintf(("friendapiprivps3: received a message from user %s: %s\n", userInfo.userId.handle.data, eventData.eventTypeData.MsgRcved.pRawData2));
                eventData.eventType = FRIENDAPI_EVENT_MSG_RCVED;
                eventData.eventTypeData.MsgRcved.pRawData1 = & iDataBufSize;
                eventData.eventTypeData.MsgRcved.pRawData2 = aDataBuf;
                eventData.eventTypeData.MsgRcved.pFriendData = &friendData;
                
                FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
                break;

            case SCE_NP_BASIC_EVENT_FRIEND_REMOVED:
                NetPrintf(("friendapiprivps3: user %s has been removed from the PS3 friends list\n", userInfo.userId.handle.data));
                eventData.eventType = FRIENDAPI_EVENT_FRIEND_REMOVED;
                eventData.eventTypeData.FriendRemoved.pFriendData = & friendData;

                _FriendApiDeleteFriend(pFriendApiRef, userInfo);
                FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
                break;

            default:
                NetPrintf(("friendapiprivps3: received a unhandled event (%d) from user %s. event data is: %s\n", iNpEvent, userInfo.userId.handle.data));
                eventData.eventType = FRIENDAPI_EVENT_UNKNOWN;
                eventData.eventTypeData.Unknown.pRawData1 = & iDataBufSize;
                eventData.eventTypeData.Unknown.pRawData2 = aDataBuf;
                eventData.eventTypeData.Unknown.pFriendData = & friendData;

                FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
                break;
            }
        }
    }
    else
    {
        if (iNpStatus != (signed)SCE_NP_BASIC_ERROR_NO_EVENT)
        {
            NetPrintf(("friendapiprivps3: sceNpBasicGetEvent failed: %s\n", DirtyErrGetName(iNpStatus)));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _RegisterBasicHandler

    \Description
        Register basic handler

    \Input *pFriendApiRef   - module reference

    \Version 08/25/2012 (akirchner)
*/
/********************************************************************************F*/
static void _RegisterBasicHandler(FriendApiRefT *pFriendApiRef)
{
    int32_t iResult;
    SceNpCommunicationId npCommId;

    // register _NpBasicHandler callback
    if(pFriendApiRef->bNpBasicHandlerRegistered == FALSE)
    {
        // get registered np communications id
        NetConnStatus('npcm', 0, &npCommId, sizeof(npCommId));

        // add np basic callback
        if ((iResult = sceNpBasicRegisterContextSensitiveHandler(&npCommId, _NpBasicHandler, pFriendApiRef)) != 0)
        {
            NetPrintf(("friendapiprivps3: sceNpBasicRegisterContextSensitiveHandler() failed: err=%s\n", DirtyErrGetName(iResult)));
        }
        else
        {
            NetPrintf(("friendapiprivps3: registered NP basic context sensitive event handler\n"));
            pFriendApiRef->bNpBasicHandlerRegistered = TRUE;
        }
    }
}

#if DIRTYCODE_DEBUG
/*F********************************************************************************/
/*!
    \Function _FriendApiDisplayBlocklist

    \Description
        Display contents of blocklist

    \Input *pFriendApiRef       - module reference

    \Version 08/03/2010 (mclouatre)
*/
/********************************************************************************F*/
static void _FriendApiDisplayBlocklist(FriendApiRefT *pFriendApiRef)
{
    int32_t iBlockedUserIndex;

    for (iBlockedUserIndex = 0; iBlockedUserIndex < FRIENDAPI_MAX_BLOCKED_USERS; iBlockedUserIndex++)
    {
        if (pFriendApiRef->blockList.aUsers[iBlockedUserIndex].bValid)
        {
            NetPrintf(("      [%d] %s\n", iBlockedUserIndex, pFriendApiRef->blockList.aUsers[iBlockedUserIndex].npId.handle.data));
        }
    }
}
#endif

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function FriendApiPrivCreate

    \Description
        Create the ps3 friendapi object.

    \Output
        FriendApiRefT*  - module reference (NULL = failed)

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
FriendApiRefT *FriendApiPrivCreate()
{
    FriendApiRefT *pFriendApiRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate the module state struct
    if ((pFriendApiRef = DirtyMemAlloc(sizeof(*pFriendApiRef), FRIENDAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("friendapiprivps3: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pFriendApiRef, 0, sizeof(*pFriendApiRef));
    NetPrintf(("friendapiprivps3: memory consumed by module state = %d bytes\n", sizeof(*pFriendApiRef)));

    // save parms
    pFriendApiRef->common.iMemGroup = iMemGroup;
    pFriendApiRef->common.pMemGroupUserData = pMemGroupUserData;

    // init critical section used to ensure thread safety in handler registered with NP basic utility
    NetCritInit(&pFriendApiRef->callbackCrit.locker, "friendapiprivps3 callback crit");

    // init critical section used to ensure thread safety in handler registered with NP basic utility
    NetCritInit(&pFriendApiRef->common.notifyCrit, "friendapi notify crit");

    // return the module ref
    return(pFriendApiRef);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivUpdate

    \Description
        Give module time to perform periodic processing

    \Input *pFriendApiRef       - module reference

    \Version 04/21/2010 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiPrivUpdate(FriendApiRefT *pFriendApiRef)
{
    if (NetConnStatus('onln', 0, NULL, 0) == TRUE)
    {
        int32_t iResult;
        int32_t iEventsInQueue;
        int8_t bGetBlockList;

        // register _NpBasicHandler callback
        _RegisterBasicHandler(pFriendApiRef);

        // iEventsInQueue and bGetBlockList can only be accessed in the context of the callbackCrit.locker
        NetCritEnter(&pFriendApiRef->callbackCrit.locker);
        iEventsInQueue = pFriendApiRef->callbackCrit.iEventsInQueue;
        if (iEventsInQueue > 0)
        {
            --pFriendApiRef->callbackCrit.iEventsInQueue;
        }

        bGetBlockList = pFriendApiRef->callbackCrit.bGetBlockList;
        pFriendApiRef->callbackCrit.bGetBlockList = FALSE;
        NetCritLeave(&pFriendApiRef->callbackCrit.locker);

        if (iEventsInQueue > 0)
        {
            _FriendApiProcessEvent(pFriendApiRef);
        }

        // upload friendlist if it was completly received
        _FriendApiPrivNotifyEventInitComplete(pFriendApiRef);

        if (bGetBlockList)
        {
            FriendEventDataT eventData;
            uint32_t uBlockedUserIndex;
            uint32_t uCount;
            int32_t iNpEvent = SCE_NP_BASIC_EVENT_BLOCKLIST_UPDATE;
            int32_t iRetCode = 0;   // fake a success error code

            iResult = sceNpBasicGetBlockListEntryCount(&uCount);
            if (iResult == 0)
            {
                // reset array of blocked users in blocklist
                memset(&pFriendApiRef->blockList.aUsers, 0x00, sizeof(pFriendApiRef->blockList.aUsers));

                for (uBlockedUserIndex = 0; uBlockedUserIndex < uCount; ++uBlockedUserIndex)
                {
                    iResult = sceNpBasicGetBlockListEntry(uBlockedUserIndex, &pFriendApiRef->blockList.aUsers[uBlockedUserIndex].npId);
                    if (iResult == 0)
                    {
                        pFriendApiRef->blockList.aUsers[uBlockedUserIndex].bValid = TRUE;
                    }
                    else
                    {
                        NetPrintf(("friendapiprivps3: sceNpBasicGetBlockListEntry(index=%d) failed: err=%s\n", uBlockedUserIndex, DirtyErrGetName(iResult)));
                    }
                }

                _FriendApiIncrementBlocklistVersion(pFriendApiRef);

                #if DIRTYCODE_DEBUG
                _FriendApiDisplayBlocklist(pFriendApiRef);
                #endif
            }
            else
            {
                NetPrintf(("friendapiprivps3: sceNpBasicGetBlockListEntryCount() failed: err=%s\n", DirtyErrGetName(iResult)));
            }

            memset(&eventData, 0, sizeof(eventData));

            eventData.eventType = FRIENDAPI_EVENT_BLOCKLIST_UPDATED;
            eventData.eventTypeData.BlockListUpdated.bPresence = FALSE;
            eventData.pRawData1 = &iNpEvent;
            eventData.pRawData2 = &iRetCode;

            FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivGetFriendsList

    \Description
        Returns the number of friends and list of friends in the user-provided buffers.
        Set pFriendsList to NULL to find out the number of friends that will be returned
        in the friendslist.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - ignored on PS3
    \Input *pFriendsList    - [out] user-provided buffer to be filled with the generic friends list (can be NULL)
    \Input *pRawDataList1   - [out] user-provided buffer to be filled with array of SceNpUserInfo structures (can be NULL; only used if pFriendsList is valid)
    \Input *pFriendsCount   - [out] user-provided buffer to be filled with the number of friends in the list (can be NULL)

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivGetFriendsList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, FriendDataT *pFriendsList, void *pRawDataList1, int32_t *pFriendsCount)
{
    int32_t iFriendIndex;
    int32_t iFriendsCount = 0;
    SceNpUserInfo *pNpUserInfoList = (SceNpUserInfo *)pRawDataList1;

    for (iFriendIndex = 0; iFriendIndex < FRIENDAPI_MAX_FRIENDS; iFriendIndex++)
    {
        if (pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid)
        {
            if (pFriendsList)
            {
                // populate generic friend name
                strncpy(pFriendsList[iFriendsCount].strFriendName,
                    pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo.userId.handle.data,
                    sizeof(pFriendsList[iFriendsCount].strFriendName));

                // populate generic friend id with "unavailable" because on PS3 we do not have 
                // a guaranteed-unique-and-unchanging numerical value for friends
                strncpy(pFriendsList[iFriendsCount].strFriendId,
                  "unavailable",
                  sizeof(pFriendsList[iFriendsCount].strFriendId));

                if (pRawDataList1)
                {
                    // copy friend's SceNpUserInfo struct in user's buffer
                    ds_memcpy(&pNpUserInfoList[iFriendsCount], &pFriendApiRef->friendsList.aFriends[iFriendIndex].userInfo, sizeof(SceNpUserInfo));
                    pFriendsList[iFriendsCount].pRawData1 = &pNpUserInfoList[iFriendsCount];
                }
            }

            iFriendsCount++;
        }
    }

    if (pFriendsCount)
    {
        *pFriendsCount = iFriendsCount;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivStatus

    \Description
        Get status information.

    \Input *pFriendApiRef   - module reference
    \Input iSelect          - status selector
    \Input *pBuf            - [out] storage for selector-specific output
    \Input iBufSize         - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'eoip' - stands for "End Of Initial Presence"; returns TRUE if initial presence completed, FALSE otherwise
            'maxf' - returns maximum possible number of friends in the friendslist
            'nphr' - returns TRUE if sceNpBasicRegisterContextSensitiveHandler(), had been called successfully, FALSE otherwise
        \endverbatim

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivStatus(FriendApiRefT *pFriendApiRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'maxf')
    {
        return(FRIENDAPI_MAX_FRIENDS);
    }

    if (iSelect == 'eoip')
    {
        int8_t bEndOfInitialPresence;

        // bEndOfInitialPresence can only be accessed in the context of the callbackCrit.locker
        NetCritEnter(&pFriendApiRef->callbackCrit.locker);
        bEndOfInitialPresence = pFriendApiRef->callbackCrit.bEndOfInitialPresence;
        NetCritLeave(&pFriendApiRef->callbackCrit.locker);

        return(bEndOfInitialPresence);
    }

    if (iSelect == 'nphr')
    {
        return((int8_t)pFriendApiRef->bNpBasicHandlerRegistered);
    }

    NetPrintf(("friendapiprivps3: unhandled status selector ('%c%c%c%c')\n",
            (uint8_t)(iSelect>>24), (uint8_t)(iSelect>>16), (uint8_t)(iSelect>>8), (uint8_t)(iSelect)));

    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivControl

    \Description
        Control behavior of module.

    \Input *pFriendApiRef   - module reference
    \Input iControl         - status selector
    \Input iValue           - control value
    \Input iValue2          - control value
    \Input *pValue          - control value

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'stop' - notifies the Friend API that it is time to unregister the handler to get PS3 friends events (if nobody else relies on it)
        \endverbatim

    \Version 05/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivControl(FriendApiRefT *pFriendApiRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    int32_t iResult = 0;
    if (iControl == 'stop')
    {
        // remove np basic callback
        if(pFriendApiRef->bNpBasicHandlerRegistered == TRUE)
        {
            NetCritEnter(&pFriendApiRef->callbackCrit.locker);
            if(pFriendApiRef->bNpBasicHandlerRegistered == TRUE)
            {
                if ((iResult = sceNpBasicUnregisterHandler()) != 0)
                {
                    NetPrintf(("friendapiprivps3: sceNpBasicUnregisterHandler() failed: err=%s\n", DirtyErrGetName(iResult)));
                }
                else
                {
                    NetPrintf(("friendapiprivps3: unregistered NP basic context sensitive event handler\n"));
                    pFriendApiRef->bNpBasicHandlerRegistered = FALSE;
                }
            }
            NetCritLeave(&pFriendApiRef->callbackCrit.locker);
        }

        return(iResult);
    }

    NetPrintf(("friendapiprivps3: unhandled control selector ('%c%c%c%c')\n",
            (uint8_t)(iControl>>24), (uint8_t)(iControl>>16), (uint8_t)(iControl>>8), (uint8_t)(iControl)));

    // unhandled
    return(iResult);
}



/*F********************************************************************************/
/*!
    \Function FriendApiPrivDestroy

    \Description
        Destroy the ps3 friend api object.

    \Input *pFriendApiRef   - module reference

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiPrivDestroy(FriendApiRefT *pFriendApiRef)
{
    int32_t iResult;

    NetPrintf(("friendapiprivps3: the friend api module is being destroyed\n"));

    // remove np basic callback
    iResult = FriendApiPrivControl(pFriendApiRef, 'stop', 0, 0 , NULL);

    // destroy critical section used to ensure thread safety in handler registered with NP basic utility
    NetCritKill(&pFriendApiRef->common.notifyCrit);

    // destroy critical section used to ensure thread safety in handler registered with NP basic utility
    NetCritKill(&pFriendApiRef->callbackCrit.locker);

    // release module memory
    DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function FriendApiIsUserFriend

    \Description
        Returns whether a given user is friend

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user for which "friend" status needs to be queried
                                on PS3: pUserId needs to point to a SceNpId struct

    \Output
        uint32_t            - TRUE if user is friend, FALSE otherwise

    \Version 09/13/2012 (akirchner)
*/
/********************************************************************************F*/
uint32_t FriendApiPrivIsUserFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    SceNpId * pNpId = (SceNpId *) pUserId;
    int32_t iFriend;

    for (iFriend = 0; iFriend < FRIENDAPI_MAX_FRIENDS; ++iFriend)
    {
        if (sceNpUtilCmpNpId(pNpId, &(pFriendApiRef->friendsList.aFriends[iFriend].userInfo.userId)) == 0)
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function FriendApiIsUserRealFriend

    \Description
        Returns whether a given user is a 'real' friend

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user for which "friend" status needs to be queried
                                on PS3: pUserId needs to point to a SceNpId struct

    \Output
        uint32_t            - TRUE if user is friend, FALSE otherwise

    \Version 07/17/2013 (jbrookes)
*/
/********************************************************************************F*/
uint32_t FriendApiPrivIsUserRealFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    return(FriendApiPrivIsUserFriend(pFriendApiRef, uUserIndex, pUserId));
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivIsUserBlocked

    \Description
        Returns whether a given user is blocked or not

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index (ignored)
    \Input *pUserId         - user for which "blocked" status needs to be queried (pointer to SceNpId)

    \Output
        uint32_t            - TRUE if user is blocked, FALSE otherwise

    \Version 02/24/2011 (mclouatre)
*/
/********************************************************************************F*/
uint32_t FriendApiPrivIsUserBlocked(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    int32_t iBlockedUserIndex;
    uint8_t bBlocked = FALSE;
    SceNpId * pNpId = (SceNpId *) pUserId;

    for (iBlockedUserIndex = 0; iBlockedUserIndex < FRIENDAPI_MAX_BLOCKED_USERS; iBlockedUserIndex++)
    {
        if (pFriendApiRef->blockList.aUsers[iBlockedUserIndex].bValid)
        {
            if (sceNpUtilCmpNpId(pNpId, &pFriendApiRef->blockList.aUsers[iBlockedUserIndex].npId) == 0)
            {
                // user is in the blocklist
                bBlocked = TRUE;
                break;
            }
        }
    }

    return(bBlocked);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivBlockUser

    \Description
        Initiates a block user operation. After calling this function, if the return
        code is "success", the caller needs to wait for the FRIENDAPI_EVENT_BLOCK_USER_RESULT
        completion event.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user to be blocked (pointer to SceNpId)

    \Output
        int32_t            - 0 for success, negative for failure

    \Notes
        Experimentation showed that, in a scenario where the specified user is
        already a friend, sceNpBasicAddBlockListEntry() returns 0 for success,
        but later in time the NP Basic handler is invoked by the system with:
            iEvent=SCE_NP_BASIC_EVENT_ADD_BLOCKLIST_RESULT/11
            iRetCode=SCE_NP_BASIC_ERROR_BLOCKLIST_ADD_FAILED/0x8002a675

    \Version 02/24/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivBlockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    SceNpId *pNpId = (SceNpId *) pUserId;
    int32_t iResult;

    iResult = sceNpBasicAddBlockListEntry(pNpId);
    if (iResult < 0)
    {
        NetPrintf(("friendapiprivps3: sceNpBasicAddBlockListEntry() failed: err=%s\n", DirtyErrGetName(iResult)));
        return(-1);
    }

    NetPrintf(("friendapiprivps3: asynchronous block user operation successfully initiated for user %s\n", pNpId->handle.data));

    return(0);
}


/*F********************************************************************************/
/*!
    \Function FriendApiPrivUnblockUser

    \Description
        Unblock user. Currently a NO-OP on PS3.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user to be unblocked (pointer to SceNpId)

    \Output
        int32_t            - 0 for success, negative for failure

    \Notes
        The PS3 API does not provide any function to unblock a user, i.e remove
        a user from the blocklist. At the moment, the only way to do this is via
        the XMB: under the Friends menu, select "Block list", 

    \Version 02/24/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivUnblockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    NetPrintf(("friendapiprivps3: unblocking a user can only be done via the friends menu of the XMB\n"));

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivGetBlockListVersion

    \Description
        Returns the current version of the list of blocked users.

    \Input *pFriendApiRef   - module reference
    \Input *pVersion        - [out] pointer to buffer to be filled with version number

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 05/10/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivGetBlockListVersion(FriendApiRefT *pFriendApiRef, uint32_t *pVersion)
{
    *pVersion = pFriendApiRef->common.uBlockListVersion;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivGetBlockList

    \Description
        Returns the list of blocked users in the user-provided buffers 

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pBlockList      - [out] user-provided buffer to be filled with block list (can be NULL)
    \Input pUsersCount      - [out] user-provided variable to be filled with number of entries in the block list.

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Notes
        Set pBlockList to NULL to find out the number of entries that pBlockList will be
        filled-in with.

    \Version 05/10/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivGetBlockList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, BlockedUserDataT *pBlockList, int32_t *pUsersCount)
{
    int32_t iBlockedUserIndex;
    int32_t iBlockedUsersCount = 0;

    for (iBlockedUserIndex = 0; iBlockedUserIndex < FRIENDAPI_MAX_BLOCKED_USERS; iBlockedUserIndex++)
    {
        if (pFriendApiRef->blockList.aUsers[iBlockedUserIndex].bValid)
        {
            if (pBlockList)
            {
                // populate generic blocked user name
                strncpy(pBlockList[iBlockedUsersCount].strBlockedUserId,
                    pFriendApiRef->blockList.aUsers[iBlockedUserIndex].npId.handle.data,
                    sizeof(pBlockList[iBlockedUsersCount].strBlockedUserId));

                // populate pointer to first party data structure
                pBlockList[iBlockedUsersCount].pRawData1 = &pFriendApiRef->blockList.aUsers[iBlockedUserIndex];
            }

            iBlockedUsersCount++;
        }
    }

    if (pUsersCount)
    {
        *pUsersCount = iBlockedUsersCount;
    }

    return(0);
}
