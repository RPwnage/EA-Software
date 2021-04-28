/*H********************************************************************************/
/*!
    \File friendapiprivpsp2.c

    \Description
        PSP2-specific portion of the friend api module

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/19/2010 (mclouatre)
    \Version 11/18/2010 (jbrookes)   Initial port PS3 to PSP2
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <np.h>

#include <sdk_version.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/friend/friendapi.h"
#include "friendapipriv.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//! PSP2 friend structure
typedef struct Psp2FriendT
{
    SceNpBasicGamePresence gamePresence;
    SceNpId friendId;
    uint8_t bValid;        //!< whether this friend entry is valid or not
    uint8_t bPresent;
} Psp2FriendT;

//! PSP2 blocked user structure
typedef struct Psp2BlockedUserT
{
    SceNpId npId;
    uint8_t bValid;        //!< whether this user entry is valid or not
} Psp2BlockedUserT;

//! PSP2 friendslist
typedef struct Psp2FriendsListT
{
    Psp2FriendT aFriends[FRIENDAPI_MAX_FRIENDS];
} Psp2FriendsListT;

//! PSP2 blocklist
typedef struct Psp2BlockListT
{
    Psp2BlockedUserT aUsers[FRIENDAPI_MAX_BLOCKED_USERS];
} Psp2BlockListT;

//! friend api module state
struct FriendApiRefT
{
    FriendApiCommonRefT common;         //!< platform-independent portion

    uint8_t bNpBasicHandlerRegistered;  //!< whether _NpBasicHandler was registered or not

    Psp2FriendsListT friendsList;       //!< list of friends
    Psp2BlockListT blockList;           //!< list of block users

    FriendEventDataT eventData;         //!< event message data buffer - to avoid stack allocation

    int8_t bEndOfInitFriendList;        //!< whether initial friendlist update is completed or not
    int8_t bEndOfInitBlockedList;       //!< whether initial blocklist update is completed or not
};

/*** Function Prototypes **********************************************************/

/*** Private functions ************************************************************/


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

    NetPrintf(("friendapiprivpsp2: friendslist updated (new friend count = %d, new version = %d)\n",
        iFriendsCount, pFriendApiRef->common.uFriendListVersion));
#endif
}

/*F********************************************************************************/
/*!
    \Function _FriendApiIncrementBlocklistVersion

    \Description
        Increment the version of the blocklist

    \Input *pFriendApiRef   - module reference

    \Version 10/21/2011 (mclouatre)
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

    NetPrintf(("friendapiprivpsp2: blocklist updated (new blocked user count = %d, new version = %d)\n",
        iBlockedUsersCount, pFriendApiRef->common.uBlockListVersion));
#endif
}

/*F********************************************************************************/
/*!
    \Function _FriendApiIsFriendInList

    \Description
        Check whether the specified friend is already in the friendslist or not

    \Input *pFriendApiRef   - module reference
    \Input *pFriendId       - friend's NP ID

    \Output
        int32_t             - returns friend index if friend is in list, returns -1 if friend is not in list

    \Version 06/29/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiIsFriendInList(FriendApiRefT *pFriendApiRef, const SceNpId *pFriendId)
{
    int32_t iFriendIndex;
    int32_t iResult = -1;

    for (iFriendIndex = 0; iFriendIndex < FRIENDAPI_MAX_FRIENDS; iFriendIndex++)
    {
        if (pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid)
        {
            if (strcmp(pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId.handle.data, pFriendId->handle.data) == 0)
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
    \Function _FriendApiResetFriendlist

    \Description
        Reset list of friends maintained internally.

    \Input *pFriendApiRef   - module reference

    \Version 07/28/2010 (mclouatre)
*/
/********************************************************************************F*/
static void _FriendApiResetFriendlist(FriendApiRefT *pFriendApiRef)
{
    memset(&pFriendApiRef->friendsList, 0, sizeof(pFriendApiRef->friendsList));
    _FriendApiIncrementFriendslistVersion(pFriendApiRef);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiUpdateFriend

    \Description
        Add a friend to the friendlist if it is not part of the list yet, and update its presence

    \Input *pFriendApiRef   - module reference
    \Input *pFriendId       - friend's NpId
    \Input bPresent         - TRUE if user is present, and FALSE if it is not
    \Input pGamePresence    - pointer to SceNpBasicGamePresence structure passed by _NpBasicFriendInGamePresenceHandler callback

    \Output
        int32_t             - returns friends index if friends successfully added, returns -1 if list is full

    \Version 06/29/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiUpdateFriend(FriendApiRefT *pFriendApiRef, const SceNpId *pFriendId, uint8_t bPresent, const SceNpBasicGamePresence * pGamePresence)
{
    int32_t iFriendIndex;
    int32_t iResult;

    // is the friend already in the friendslist
    if ((iResult = _FriendApiIsFriendInList(pFriendApiRef, pFriendId)) < 0)
    {
        // add friend to a free spot in the friendslist
        for (iFriendIndex = 0; iFriendIndex < FRIENDAPI_MAX_FRIENDS; iFriendIndex++)
        {
            if (!pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid)
            {
                // add friend to free spot
                memcpy(&pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId, pFriendId, sizeof(pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId));

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

        if(pGamePresence)
        {
            pFriendApiRef->friendsList.aFriends[iResult].gamePresence = *pGamePresence;
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
    \Input *pFriendId       - friend's NpId

    \Output
        int32_t             - returns 0 if friend found and deleted, returns -1 if friend is not in list

    \Version 06/29/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiDeleteFriend(FriendApiRefT *pFriendApiRef, const SceNpId *pFriendId)
{
    int32_t iFriendIndex;

    // is the friend in the friendlist
    if ((iFriendIndex = _FriendApiIsFriendInList(pFriendApiRef, pFriendId)) < 0)
    {
        // signal that the specified friend is not in the list
        return(-1);
    }

    // invalidate the friendlist entry
    memset(&pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId, 0, sizeof(pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId));
    pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid = FALSE;

    // update the friendslist version
    _FriendApiIncrementFriendslistVersion(pFriendApiRef);

    return(0);
}


/*F********************************************************************************/
/*!
    \Function _FriendApiIsUserInBlocklist

    \Description
        Check whether the specified user is already in the blocklist or not

    \Input *pFriendApiRef   - module reference
    \Input *pUserId         - user's NP ID

    \Output
        int32_t             - returns user index if user is in list, returns -1 if user is not in list

    \Version 10/21/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiIsUserInBlocklist(FriendApiRefT *pFriendApiRef, const SceNpId *pUserId)
{
    int32_t iUserIndex;
    int32_t iResult = -1;

    for (iUserIndex = 0; iUserIndex < FRIENDAPI_MAX_BLOCKED_USERS; iUserIndex++)
    {
        if (pFriendApiRef->blockList.aUsers[iUserIndex].bValid)
        {
            if (strcmp(pFriendApiRef->blockList.aUsers[iUserIndex].npId.handle.data, pUserId->handle.data) == 0)
            {
                iResult = iUserIndex;
                break;
            }
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiResetBlocklist

    \Description
        Reset list of blocked users maintained internally.

    \Input *pFriendApiRef   - module reference

    \Version 10/21/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _FriendApiResetBlocklist(FriendApiRefT *pFriendApiRef)
{
    memset(&pFriendApiRef->blockList, 0, sizeof(pFriendApiRef->blockList));
    _FriendApiIncrementBlocklistVersion(pFriendApiRef);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiAddBlockedUser

    \Description
        Add a user to the blocklist. Does not duplicate if user is already in list.

    \Input *pFriendApiRef   - module reference
    \Input *pUserId         - user's NpId

    \Output
        int32_t             - returns user index if user successfully added, returns -1 if list is full

    \Version 10/21/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiAddBlockedUser(FriendApiRefT *pFriendApiRef, const SceNpId *pUserId)
{
    int32_t iUserIndex;
    int32_t iResult;

    // is the user already in the blocklist
    if ((iResult = _FriendApiIsUserInBlocklist(pFriendApiRef, pUserId)) >= 0)
    {
        return(iResult);
    }

    // add user to a free spot in the blocklist
    iResult = -1;
    for (iUserIndex = 0; iUserIndex < FRIENDAPI_MAX_BLOCKED_USERS; iUserIndex++)
    {
        if (!pFriendApiRef->blockList.aUsers[iUserIndex].bValid)
        {
            // add user to free spot
            memcpy(&pFriendApiRef->blockList.aUsers[iUserIndex].npId, pUserId, sizeof(pFriendApiRef->blockList.aUsers[iUserIndex].npId));

            // mark user as valid
            pFriendApiRef->blockList.aUsers[iUserIndex].bValid = TRUE;

            // increment blocklist version number
            _FriendApiIncrementBlocklistVersion(pFriendApiRef);

            iResult = iUserIndex;

            break;
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiDeleteBlockedUser

    \Description
        Delete a user from the blocklist.

    \Input *pFriendApiRef   - module reference
    \Input *pUserId         - user's NpId

    \Output
        int32_t             - returns 0 if user found and deleted, returns -1 if user is not in list

    \Version 10/21/2010 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _FriendApiDeleteBlockedUser(FriendApiRefT *pFriendApiRef, const SceNpId *pUserId)
{
    int32_t iUserIndex;

    // is the friend in the friendlist
    if ((iUserIndex = _FriendApiIsUserInBlocklist(pFriendApiRef, pUserId)) < 0)
    {
        // signal that the specified useris not in the list
        return(-1);
    }

    // invalidate the blocklist entry
    memset(&pFriendApiRef->blockList.aUsers[iUserIndex].npId, 0, sizeof(pFriendApiRef->blockList.aUsers[iUserIndex].npId));
    pFriendApiRef->blockList.aUsers[iUserIndex].bValid = FALSE;

    // update the friendslist version
    _FriendApiIncrementBlocklistVersion(pFriendApiRef);

    return(0);
}

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

    NetCritEnter(&pFriendApiRef->common.notifyCrit);

    if((pFriendApiRef->bEndOfInitFriendList == TRUE) && (pFriendApiRef->bEndOfInitBlockedList == TRUE))
    {
        for (iCallbackIndex = 0; iCallbackIndex < FRIENDAPI_MAX_CALLBACKS; iCallbackIndex++)
        {
            if ((pFriendApiRef->common.callback[iCallbackIndex].pNotifyCb) && (pFriendApiRef->common.callback[iCallbackIndex].bInitFinished == FALSE))
            {
                for (iFriendIndex = 0; iFriendIndex < FRIENDAPI_MAX_FRIENDS; iFriendIndex++)
                {
                    if (pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid)
                    {
                        if(pFriendApiRef->friendsList.aFriends[iFriendIndex].bPresent == TRUE)
                        {
                            iNpEvent = SCE_NP_BASIC_ONLINE_STATUS_ONLINE;
                        }
                        else
                        {
                            iNpEvent = SCE_NP_BASIC_ONLINE_STATUS_OFFLINE;
                        }

                        memset(&friendData, 0, sizeof(FriendDataT));
                        strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
                        strncpy(friendData.strFriendName, pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId.handle.data, sizeof(friendData.strFriendId));
                        friendData.pRawData1 = &pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId;

                        memset(&eventData, 0, sizeof(FriendEventDataT));

                        eventData.eventType = FRIENDAPI_EVENT_FRIEND_UPDATED;
                        eventData.eventTypeData.FriendUpdated.pFriendData = &friendData;
                        eventData.eventTypeData.FriendUpdated.bPresence = pFriendApiRef->friendsList.aFriends[iFriendIndex].bPresent;
                        eventData.pRawData1 = &iNpEvent;
                        eventData.pRawData2 = &pFriendApiRef->friendsList.aFriends[iFriendIndex].gamePresence;

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
    \Function _NpBasicFriendListHandler

    \Description
        Handler for NpBasic friend list status updates

    \Input eEventType   - the event type
    \Input *pFriendId   - NpId of friend
    \Input *pUserData   - user data

    \Version 11/18/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _NpBasicFriendListHandler(SceNpBasicFriendListEventType eEventType, const SceNpId *pFriendId, void *pUserData)
{
    FriendApiRefT* pFriendApiRef = (FriendApiRefT*)pUserData;
    FriendApiEventsE friendApiEvent = FRIENDAPI_EVENT_UNKNOWN;
    FriendDataT friendData;
    FriendApiPSP2EventsE ePSP2EventType = FRIENDAPI_PSP2EVENT_FRIENDLIST;

    #if DIRTYCODE_LOGGING
    static char *_strEventNames[] =
    {
        "SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC_START",
        "SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC",
        "SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC_DONE",
        "SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_ADDED",
        "SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_DELETED"
    };

    if ((unsigned)eEventType < (sizeof(_strEventNames)/sizeof(_strEventNames[0])))
    {
        NetPrintf(("friendapiprivpsp2: valid friendlist event -- friend=%s | event=%s/%d\n",
            ((eEventType==SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC_START || eEventType==SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC_DONE) ? "n/a" : pFriendId->handle.data),
            _strEventNames[eEventType], eEventType));
    }
    else
    {
        NetPrintf(("friendapiprivpsp2: unknown friendlist event -- friend=%s | event=%d\n",
            (pFriendId ? pFriendId->handle.data : "null"), eEventType));
    }
    #endif

    memset(&pFriendApiRef->eventData, 0, sizeof(pFriendApiRef->eventData));

    switch (eEventType)
    {
        case SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC_START:
            _FriendApiResetFriendlist(pFriendApiRef);
            pFriendApiRef->bEndOfInitFriendList = FALSE;
            break;

        case SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC_DONE:
            pFriendApiRef->bEndOfInitFriendList = TRUE;
            break;

        case SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_SYNC:
        case SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_ADDED:
            friendApiEvent = FRIENDAPI_EVENT_FRIEND_UPDATED;
            _FriendApiUpdateFriend(pFriendApiRef, pFriendId, FALSE, 0);
            break;

        case SCE_NP_BASIC_FRIEND_LIST_EVENT_TYPE_DELETED:
            friendApiEvent = FRIENDAPI_EVENT_FRIEND_REMOVED;
            _FriendApiDeleteFriend(pFriendApiRef, pFriendId);
            break;

        default:
            break;
    }

    // on psp2, the pRawData1 field of the eventData structure points to a "homemade" psp2 event type value
    pFriendApiRef->eventData.pRawData1 = &ePSP2EventType;

    // on psp2, the pRawData2 field of the eventData structure points to the psp2 event data value
    pFriendApiRef->eventData.pRawData2 = &eEventType;

    // initialize the generic friend structure
    memset(&friendData, 0, sizeof(friendData));
    strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
    strncpy(friendData.strFriendName, pFriendId->handle.data, sizeof(friendData.strFriendId));
    friendData.pRawData1 = pFriendId;

    if(friendApiEvent == FRIENDAPI_EVENT_UNKNOWN)
    {
        pFriendApiRef->eventData.eventTypeData.Unkown.pFriendData = & friendData;
    }
    else if(friendApiEvent == FRIENDAPI_EVENT_FRIEND_UPDATED)
    {
        pFriendApiRef->eventData.eventTypeData.FriendUpdated.pFriendData = & friendData;
    }
    else if(friendApiEvent == FRIENDAPI_EVENT_FRIEND_REMOVED)
    {
        pFriendApiRef->eventData.eventTypeData.FriendRemoved.pFriendData = & friendData;
    }

    pFriendApiRef->eventData.eventType = friendApiEvent;

    FriendApiPrivNotifyEvent(pFriendApiRef, &pFriendApiRef->eventData);
}

/*F********************************************************************************/
/*!
    \Function _NpBasicFriendOnlineStatusHandler

    \Description
        Handler for NpBasic friend online status updates

    \Input *pFriendId       - NpId of friend
    \Input eOnlineStatus    - new online status for this friend
    \Input *pUserData       - user data

    \Version 02/17/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NpBasicFriendOnlineStatusHandler(const SceNpId *pFriendId, SceNpBasicOnlineStatus eOnlineStatus, void *pUserData)
{
    FriendApiRefT* pFriendApiRef = (FriendApiRefT*)pUserData;
    FriendApiEventsE friendApiEvent = FRIENDAPI_EVENT_FRIEND_UPDATED;
    FriendDataT friendData;
    FriendApiPSP2EventsE ePSP2EventType = FRIENDAPI_PSP2EVENT_ONLINE_STATUS;

    #if DIRTYCODE_LOGGING
    static char *_strOnlineStatusNames[] =
    {
        "SCE_NP_BASIC_ONLINE_STATUS_UNKNOWN",
        "SCE_NP_BASIC_ONLINE_STATUS_OFFLINE",
        "SCE_NP_BASIC_ONLINE_STATUS_AFK",
        "SCE_NP_BASIC_ONLINE_STATUS_ONLINE"
    };

    if ((unsigned)eOnlineStatus < (sizeof(_strOnlineStatusNames)/sizeof(_strOnlineStatusNames[0])))
    {
        NetPrintf(("friendapiprivpsp2: valid friend online status event -- friend=%s | onlineStatus=%s/%d\n",
            pFriendId->handle.data, _strOnlineStatusNames[eOnlineStatus], eOnlineStatus));
    }
    else
    {
        NetPrintf(("friendapiprivpsp2: unknown friend online status event -- friend=%s | onlineStatus=%d\n",
            (pFriendId ? pFriendId->handle.data : "null"), eOnlineStatus));
    }
    #endif

    memset(&pFriendApiRef->eventData, 0, sizeof(pFriendApiRef->eventData));

    switch (eOnlineStatus)
    {
        case SCE_NP_BASIC_ONLINE_STATUS_UNKNOWN:
        case SCE_NP_BASIC_ONLINE_STATUS_OFFLINE:
        case SCE_NP_BASIC_ONLINE_STATUS_AFK:
            if(_FriendApiUpdateFriend(pFriendApiRef, pFriendId, FALSE, 0) < 0)
            {
                NetPrintf(("friendapiprivpsp2: failed to update friend=%s | onlineStatus=%s/%d\n", pFriendId->handle.data));
            }

            pFriendApiRef->eventData.eventTypeData.FriendUpdated.bPresence = FALSE;
            break;

        case SCE_NP_BASIC_ONLINE_STATUS_ONLINE:
            if(_FriendApiUpdateFriend(pFriendApiRef, pFriendId, TRUE, 0) < 0)
            {
                NetPrintf(("friendapiprivpsp2: failed to update friend=%s | onlineStatus=%s/%d\n", pFriendId->handle.data));
            }

            pFriendApiRef->eventData.eventTypeData.FriendUpdated.bPresence = TRUE;
            break;

        default:
            friendApiEvent = FRIENDAPI_EVENT_UNKNOWN;
            break;
    }

    // on psp2, the pRawData1 field of the eventData structure points to a "homemade" psp2 event type value
    pFriendApiRef->eventData.pRawData1 = &ePSP2EventType;

    // on psp2, the pRawData2 field of the eventData structure points to the psp2 event data
    pFriendApiRef->eventData.pRawData2 = &eOnlineStatus;

    // initialize the generic friend structure
    memset(&friendData, 0, sizeof(friendData));
    strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
    strncpy(friendData.strFriendName, pFriendId->handle.data, sizeof(friendData.strFriendId));
    friendData.pRawData1 = pFriendId;

    if(friendApiEvent == FRIENDAPI_EVENT_UNKNOWN)
    {
        pFriendApiRef->eventData.eventTypeData.Unkown.pFriendData = & friendData;
    }
    else if(friendApiEvent == FRIENDAPI_EVENT_FRIEND_UPDATED)
    {
        pFriendApiRef->eventData.eventTypeData.FriendUpdated.pFriendData = & friendData;
    }

    pFriendApiRef->eventData.eventType = friendApiEvent;

    FriendApiPrivNotifyEvent(pFriendApiRef, &pFriendApiRef->eventData);
}

/*F********************************************************************************/
/*!
    \Function _NpBasicBlockListHandler

    \Description
        Handler for NpBasic blocklist updates

    \Input eEventType       - the event type
    \Input *pPlayerId       - NpId of blocked player
    \Input *pUserData       - user data

    \Version 02/17/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NpBasicBlockListHandler(SceNpBasicBlockListEventType eEventType, const SceNpId *pPlayerId, void *pUserData)
{
    FriendApiRefT* pFriendApiRef = (FriendApiRefT*)pUserData;
    FriendApiEventsE friendApiEvent = FRIENDAPI_EVENT_UNKNOWN;
    FriendApiPSP2EventsE ePSP2EventType = FRIENDAPI_PSP2EVENT_BLOCKLIST;

    #if DIRTYCODE_LOGGING
    static char *_strEventNames[] =
    {
        "SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC_START",
        "SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC",
        "SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC_DONE",
        "SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_ADDED",
        "SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_DELETED"
    };

    if ((unsigned)eEventType < (sizeof(_strEventNames)/sizeof(_strEventNames[0])))
    {
        NetPrintf(("friendapiprivpsp2: valid blocklist event -- user=%s | event=%s/%d\n",
            ((eEventType==SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC_START || eEventType==SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC_DONE) ? "n/a" : pPlayerId->handle.data),
            _strEventNames[eEventType], eEventType));
    }
    else
    {
        NetPrintf(("friendapiprivpsp2: unknown blocklist event -- user=%s | event=%d\n",
            (pPlayerId ? pPlayerId->handle.data : "null"), eEventType));
    }
    #endif

    memset(&pFriendApiRef->eventData, 0, sizeof(pFriendApiRef->eventData));

    switch (eEventType)
    {
        case SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC_START:
            _FriendApiResetBlocklist(pFriendApiRef);
            pFriendApiRef->bEndOfInitBlockedList = FALSE;
            break;

        case SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC_DONE:
            pFriendApiRef->bEndOfInitBlockedList = TRUE;
            break;

        case SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_SYNC:
        case SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_ADDED:
            _FriendApiAddBlockedUser(pFriendApiRef, pPlayerId);
            friendApiEvent = FRIENDAPI_EVENT_BLOCKLIST_UPDATED;
            break;

        case SCE_NP_BASIC_BLOCK_LIST_EVENT_TYPE_DELETED:
            _FriendApiDeleteBlockedUser(pFriendApiRef, pPlayerId);
            friendApiEvent = FRIENDAPI_EVENT_BLOCKLIST_UPDATED;
            break;

        default:
            break;
    }

    // on psp2, the pRawData1 field of the eventData structure points to a "homemade" psp2 event type value
    pFriendApiRef->eventData.pRawData1 = &ePSP2EventType;

    // on psp2, the pRawData2 field of the eventData structure points to the psp2 event data
    pFriendApiRef->eventData.pRawData2 = &eEventType;

    if(friendApiEvent == FRIENDAPI_EVENT_UNKNOWN)
    {
        FriendDataT friendData;

        // initialize the generic friend structure
        memset(&friendData, 0, sizeof(friendData));
        strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
        strncpy(friendData.strFriendName, pPlayerId->handle.data, sizeof(friendData.strFriendId));
        friendData.pRawData1 = pPlayerId;

        pFriendApiRef->eventData.eventType = friendApiEvent;
        pFriendApiRef->eventData.eventTypeData.Unkown.pFriendData = & friendData;

        FriendApiPrivNotifyEvent(pFriendApiRef, &pFriendApiRef->eventData);
    }
    else // (friendApiEvent == FRIENDAPI_EVENT_BLOCKLIST_UPDATED)
    {
        BlockedUserDataT blockedUserData;

        // initialize the generic block user structure
        memset(&blockedUserData, 0, sizeof(blockedUserData));
        strncpy(blockedUserData.strBlockedUserId, "unavailable", sizeof(blockedUserData.strBlockedUserId));
        blockedUserData.pRawData1 = pPlayerId;

        pFriendApiRef->eventData.eventType = friendApiEvent;
        pFriendApiRef->eventData.eventTypeData.BlockListUpdated.pBlockedUserData = & blockedUserData;

        FriendApiPrivNotifyEvent(pFriendApiRef, &pFriendApiRef->eventData);
    }
}

/*F********************************************************************************/
/*!
    \Function _NpBasicInGamePresenceHandler

    \Description
        Handler for NpBasic friend ingame presence updates

    \Input *pFriendId       - NpId of friend
    \Input *pPresence       - ingame presence
    \Input *pUserData       - user data

    \Version 02/17/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NpBasicFriendInGamePresenceHandler(const SceNpId *pFriendId, const SceNpBasicGamePresence *pPresence, void *pUserData)
{
    FriendApiRefT* pFriendApiRef = (FriendApiRefT*)pUserData;
    FriendApiEventsE friendApiEvent = FRIENDAPI_EVENT_FRIEND_UPDATED;
    FriendDataT friendData;
    FriendApiPSP2EventsE ePSP2EventType = FRIENDAPI_PSP2EVENT_INGAME_PRESENCE;

    NetPrintf(("friendapiprivpsp2: friend ingame presence update -- friend=%s\n", pFriendId->handle.data));

    memset(&pFriendApiRef->eventData, 0, sizeof(pFriendApiRef->eventData));

    // on psp2, the pRawData1 field of the eventData structure points to a "homemade" psp2 event type value
    pFriendApiRef->eventData.pRawData1 = &ePSP2EventType;

    // on psp2, the pRawData2 field of the eventData structure points to the psp2 event data
    pFriendApiRef->eventData.pRawData2 = pPresence;

    // initialize the generic friend structure
    memset(&friendData, 0, sizeof(friendData));
    strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
    strncpy(friendData.strFriendName, pFriendId->handle.data, sizeof(friendData.strFriendId));
    friendData.pRawData1 = pFriendId;

    pFriendApiRef->eventData.eventType = friendApiEvent;
    pFriendApiRef->eventData.eventTypeData.FriendUpdated.pFriendData = & friendData;

    if(_FriendApiUpdateFriend(pFriendApiRef, pFriendId, TRUE, pPresence) < 0)
    {
         NetPrintf(("friendapiprivpsp2: failed to update friend=%s | onlineStatus=%s/%d\n", pFriendId->handle.data));
    }

    FriendApiPrivNotifyEvent(pFriendApiRef, &pFriendApiRef->eventData);
}

/*F********************************************************************************/
/*!
    \Function _NpBasicInGameDataMessageHandler

    \Description
        Handler for NpBasic ingame data message

    \Input *pFrom       - NpId of sender
    \Input *pMessage    - message
    \Input *pUserData   - user data

    \Version 02/17/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NpBasicInGameDataMessageHandler(const SceNpId *pFrom, const SceNpBasicInGameDataMessage *pMessage, void *pUserData)
{
    FriendApiRefT* pFriendApiRef = (FriendApiRefT*)pUserData;
    FriendApiEventsE friendApiEvent = FRIENDAPI_EVENT_MSG_RCVED;
    FriendDataT friendData;
    FriendApiPSP2EventsE ePSP2EventType = FRIENDAPI_PSP2EVENT_INGAME_DATA_MESSAGE;
    int32_t iDataSize;

    NetPrintf(("friendapiprivpsp2: received ingame data message from %s\n", pFrom->handle.data));

    memset(&pFriendApiRef->eventData, 0, sizeof(pFriendApiRef->eventData));

    iDataSize = sizeof(pMessage->data);

    pFriendApiRef->eventData.eventTypeData.MsgRcved.pRawData1 = & iDataSize;
    pFriendApiRef->eventData.eventTypeData.MsgRcved.pRawData2 = pMessage->data;

    // on psp2, the pRawData1 field of the eventData structure points to a "homemade" psp2 event type value
    pFriendApiRef->eventData.pRawData1 = &ePSP2EventType;

    // on psp2, the pRawData2 field of the eventData structure points to the psp2 event data
    pFriendApiRef->eventData.pRawData2 = pMessage;

    // initialize the generic friend structure
    memset(&friendData, 0, sizeof(friendData));
    strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
    strncpy(friendData.strFriendName, pFrom->handle.data, sizeof(friendData.strFriendId));
    friendData.pRawData1 = pFrom;

    pFriendApiRef->eventData.eventType = friendApiEvent;
    pFriendApiRef->eventData.eventTypeData.MsgRcved.pFriendData = &friendData;

    FriendApiPrivNotifyEvent(pFriendApiRef, &pFriendApiRef->eventData);
}

/*F********************************************************************************/
/*!
    \Function _NpBasicFriendContextEventHandler

    \Description
        Handler for NpBasic friend context event

    \Input *pNpId           - NpId of friend
    \Input eContextState    - state
    \Input *pUserData       - user data

    \Version 07/27/2011 (mclouatre)
*/
/********************************************************************************F*/
static void _NpBasicFriendContextEventHandler(const SceNpId *pNpId, const SceNpBasicFriendContextState eContextState, void *pUserData)
{
    FriendApiRefT* pFriendApiRef = (FriendApiRefT*)pUserData;
    FriendApiEventsE friendApiEvent = FRIENDAPI_EVENT_FRIEND_UPDATED;
    FriendDataT friendData;
    FriendApiPSP2EventsE ePSP2EventType = FRIENDAPI_PSP2EVENT_FRIEND_CONTEXT;

    #if DIRTYCODE_LOGGING
    static char *_strContextStateNames[] =
    {
        "SCE_NP_BASIC_FRIEND_CONTEXT_STATE_UNKNOWN",
        "SCE_NP_BASIC_FRIEND_CONTEXT_STATE_OUT_OF_CONTEXT",
        "SCE_NP_BASIC_FRIEND_CONTEXT_STATE_IN_CONTEXT"
    };

    if ((unsigned)eContextState < (sizeof(_strContextStateNames)/sizeof(_strContextStateNames[0])))
    {
        NetPrintf(("friendapiprivpsp2: valid friend context event -- friend=%s | state=%s/%d\n",
            pNpId->handle.data, _strContextStateNames[eContextState], eContextState));
    }
    else
    {
        NetPrintf(("friendapiprivpsp2: unknown friend context event -- friend=%s | state=%d\n",
            (pNpId ? pNpId->handle.data : "null"), eContextState));
    }
    #endif

    memset(&pFriendApiRef->eventData, 0, sizeof(pFriendApiRef->eventData));

    // on psp2, the pRawData1 field of the eventData structure points to a "homemade" psp2 event type value
    pFriendApiRef->eventData.pRawData1 = &ePSP2EventType;

    // on psp2, the pRawData2 field of the eventData structure points to the psp2 context state value
    pFriendApiRef->eventData.pRawData2 = &eContextState;

    // initialize the generic friend structure
    memset(&friendData, 0, sizeof(friendData));
    strncpy(friendData.strFriendId, "unavailable", sizeof(friendData.strFriendId));
    strncpy(friendData.strFriendName, pNpId->handle.data, sizeof(friendData.strFriendId));
    friendData.pRawData1 = pNpId;

    pFriendApiRef->eventData.eventType = friendApiEvent;
    pFriendApiRef->eventData.eventTypeData.FriendUpdated.pFriendData = &friendData;

    FriendApiPrivNotifyEvent(pFriendApiRef, &pFriendApiRef->eventData);
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

    // register _NpBasicHandler callback
    if(pFriendApiRef->bNpBasicHandlerRegistered == FALSE)
    {
        SceNpBasicEventHandlers _EventHandlers;
        SceNpCommunicationId _CommId;

        // init handlers structure
        sceNpBasicEventHandlersInit(&_EventHandlers);
        _EventHandlers.friendListEventHandler = _NpBasicFriendListHandler;
        _EventHandlers.friendOnlineStatusEventHandler = _NpBasicFriendOnlineStatusHandler;
        _EventHandlers.blockListEventHandler = _NpBasicBlockListHandler;
        _EventHandlers.friendGamePresenceEventHandler = _NpBasicFriendInGamePresenceHandler;
        _EventHandlers.inGameDataMessageEventHandler = _NpBasicInGameDataMessageHandler;
        _EventHandlers.friendContextEventHandler = _NpBasicFriendContextEventHandler;

        // get commid
        NetConnStatus('npcm', 0, &_CommId, sizeof(_CommId));

        // set handlers
        NetPrintf(("friendapiprivpsp2: registering NP basic event handler\n"));
        if ((iResult = sceNpBasicRegisterHandler(&_EventHandlers, &_CommId, pFriendApiRef)) != 0)
        {
            NetPrintf(("friendapiprivpsp2: sceNpBasicRegisterHandler() failed: err=%s\n", DirtyErrGetName(iResult)));
        }
        else
        {
            NetPrintf(("friendapiprivpsp2: registered NP basic context sensitive event handler\n"));
            pFriendApiRef->bNpBasicHandlerRegistered = TRUE;
        }
    }
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function FriendApiPrivCreate

    \Description
        Create the psp2 friendapi object.

    \Output
        FriendApiRefT*  - module reference (NULL = failed)

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
FriendApiRefT *FriendApiPrivCreate(void)
{
    int32_t iResult;
    FriendApiRefT *pFriendApiRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate the module state struct
    if ((pFriendApiRef = DirtyMemAlloc(sizeof(*pFriendApiRef), FRIENDAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("friendapiprivpsp2: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pFriendApiRef, 0, sizeof(*pFriendApiRef));
    NetPrintf(("friendapiprivpsp2: memory consumed by module state = %d bytes\n", sizeof(*pFriendApiRef)));

    // save parms
    pFriendApiRef->common.iMemGroup = iMemGroup;
    pFriendApiRef->common.pMemGroupUserData = pMemGroupUserData;

    // initialize the NP basic utility
    if ((iResult = sceNpBasicInit(NULL)) != SCE_OK)
    {
        NetPrintf(("friendapiprivpsp2: sceNpBasicInit() failed: err=%s\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);
        return(NULL);
    }

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
        // register _NpBasicHandler callback
        _RegisterBasicHandler(pFriendApiRef);

        // upload friendlist if it was completly received
        _FriendApiPrivNotifyEventInitComplete(pFriendApiRef);

        sceNpBasicCheckCallback();
    }
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivGetFriendsList

    \Description
        Returns the number of friends and list of friends in the user-provided buffers.
        Set pFriend to NULL to find out the number of friends that will be returned
        in the friendslist.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - ignored on PSP2
    \Input *pFriendsList    - [out] user-provided buffer to be filled with the generic friends list (can be NULL)
    \Input *pRawDataList1   - [out] user-provided buffer to be filled with array of SceNpUserInfo structures (can be NULL)
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
    SceNpId *pNpIdList = (SceNpId *)pRawDataList1;

    for (iFriendIndex = 0; iFriendIndex < FRIENDAPI_MAX_FRIENDS; iFriendIndex++)
    {
        if (pFriendApiRef->friendsList.aFriends[iFriendIndex].bValid)
        {
            if (pFriendsList)
            {
                // populate generic friend name
                strncpy(pFriendsList[iFriendsCount].strFriendName,
                    pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId.handle.data,
                    sizeof(pFriendsList[iFriendsCount].strFriendName));

                // populate generic friend id with "unavailable" because on PSP2 we do not have 
                // a guaranteed-unique-and-unchanging numerical value for friends
                strncpy(pFriendsList[iFriendsCount].strFriendId,
                  "unavailable",
                  sizeof(pFriendsList[iFriendsCount].strFriendId));

                if (pRawDataList1)
                {
                    // copy friend's SceNpUserInfo struct in user's buffer
                    memcpy(&pNpIdList[iFriendsCount], &pFriendApiRef->friendsList.aFriends[iFriendIndex].friendId, sizeof(SceNpId));
                    pFriendsList[iFriendsCount].pRawData1 = &pNpIdList[iFriendsCount];
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
            'maxf' - returns maximum possible number of friends in the friendslist
            'eoip' - stands for "End Of Initial Presence"; returns TRUE if initial presence completed, FALSE otherwise
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
        return(pFriendApiRef->bEndOfInitFriendList && pFriendApiRef->bEndOfInitBlockedList);
    }

    NetPrintf(("friendapiprivpsp2: unhandled status selector ('%c%c%c%c')\n",
            (uint8_t)(iSelect>>24), (uint8_t)(iSelect>>16), (uint8_t)(iSelect>>8), (uint8_t)(iSelect)));

    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivDestroy

    \Description
        Destroy the psp2 friend api object.

    \Input *pFriendApiRef   - module reference

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiPrivDestroy(FriendApiRefT *pFriendApiRef)
{
    int32_t iResult;

    NetPrintf(("friendapiprivpsp2: the friend api module is being destroyed\n"));

    // remove np basic callback
    if(pFriendApiRef->bNpBasicHandlerRegistered == TRUE)
    {
        if ((iResult = sceNpBasicUnregisterHandler()) != 0)
        {
            NetPrintf(("friendapiprivpsp2: sceNpBasicUnregisterHandler() failed: err=%s\n", DirtyErrGetName(iResult)));
        }
        else
        {
            NetPrintf(("friendapiprivpsp2: unregistered NP basic context sensitive event handler\n"));
            pFriendApiRef->bNpBasicHandlerRegistered = FALSE;
        }
    }

    // terminate the NP basic utility
    if ((iResult = sceNpBasicTerm()) != SCE_OK)
    {
        NetPrintf(("friendapiprivpsp2: sceNpBasicTerm() failed: err=%s\n", DirtyErrGetName(iResult)));
    }

    // destroy critical section used to ensure thread safety in handler registered with NP basic utility
    NetCritKill(&pFriendApiRef->common.notifyCrit);

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
                                on PSP2: pUserId needs to point to a SceNpId struct

    \Output
        uint32_t            - TRUE if user is friend, FALSE otherwise

    \Version 09/13/2012 (akirchner)
*/
/********************************************************************************F*/
uint32_t FriendApiPrivIsUserFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    // $$todo: There is a backlog task to implement this function

    return(FALSE);
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
            if (strcmp(pNpId->handle.data, pFriendApiRef->blockList.aUsers[iBlockedUserIndex].npId.handle.data) == 0)
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
        Block user. Currently not supported on PSP2.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user to be blocked (pointer to SceNpId)

    \Output
        int32_t             - always returns -1 for failure

    \Notes
        The PSP2 API does not provide any function to block a user, i.e add
        a user to the blocklist. At the moment, the only way to do this is via
        the system menu.

    \Version 10/21/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivBlockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    NetPrintf(("friendapiprivpsp2: blocking a user not supported on PSP2\n"));
    return(-1);
}


/*F********************************************************************************/
/*!
    \Function FriendApiPrivUnblockUser

    \Description
        Unblock user. Currently not supported on PSP2.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user to be unblocked (pointer to SceNpId)

    \Output
        int32_t             - always returns -1 for failure

    \Notes
        The PSP2 API does not provide any function to unblock a user, i.e remove
        a user from the blocklist. At the moment, the only way to do this is via
        the system menu.

    \Version 10/21/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivUnblockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    NetPrintf(("friendapiprivpsp2: unblocking a user not supported on PSP2\n"));
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

    \Version 21/10/2011 (mclouatre)
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

    \Version 21/10/2011 (mclouatre)
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
