/*H********************************************************************************/
/*!
    \File friendapiprivxenon.c

    \Description
        Xenon-specific portion of the friend api module

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <string.h>
#include <xtl.h>
#include <xonline.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/friend/friendapi.h"
#include "friendapipriv.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct EnumerationT
{
    int8_t bEnumerating;
    HANDLE hEventListener;
    HANDLE hEnumHandle;
    XOVERLAPPED xOverlapped;
    ULONG ulUserId;
    DWORD dwFriendsEnumerated;
} EnumerationT;

typedef struct FriendsUpdateT
{
    int32_t     iFriends;
    FriendDataT aFriends[FRIENDAPI_MAX_FRIENDS];
    uint32_t    aStates [FRIENDAPI_MAX_FRIENDS];
} FriendsUpdateT;

typedef struct FriendsCollectionT
{
    uint8_t bOldFriendsCollectionIndex;                     //!< index of the collection of friends in aFriendsCollection
    FriendsUpdateT aFriendsUpdate[2];
    XONLINE_FRIEND aOnlineFriends[FRIENDAPI_MAX_FRIENDS];   //! TODO: Find out if this should be allocated dynamically to save memory. It's size of 19600 bytes
    int8_t bEndOfInitialPresence;                           //!< whether initial presence update is completed or not
} FriendsCollectionT;

struct FriendApiRefT
{
    FriendApiCommonRefT common;         //!< platform-independent portion

    uint8_t bNotifyBlockUserResult;

    FriendsCollectionT aFriendsCollection[NETCONN_MAXLOCALUSERS];

    EnumerationT enumeration;
};

/*** Function Prototypes **********************************************************/

/*** Private functions ************************************************************/

/*F*************************************************************************************************/
/*!
    \Function _FriendApiPrivFreeCallback

    \Description
        Clean up friendapiprivxenon instance if asynchronous enumeration completed

    \Input *pFriendApiRefInstance - pointer to friendapiprivxenon instance

    \Output
        int32_t         - zero = success; -1 = try again; other negative = error

    \Version 09/13/2012 (akirchner)
*/
/*************************************************************************************************F*/
static int32_t _FriendApiPrivFreeCallback(void *pFriendApiRefInstance)
{
    int32_t iRetVal = 0;
    FriendApiRefT *pFriendApiRef = (FriendApiRefT *)pFriendApiRefInstance;

    if (pFriendApiRef->enumeration.bEnumerating == TRUE)
    {
        DWORD dwResult = XGetOverlappedResult(&pFriendApiRef->enumeration.xOverlapped, &pFriendApiRef->enumeration.dwFriendsEnumerated, FALSE);

        switch(dwResult)
        {
            case ERROR_SUCCESS:
                NetPrintf(("friendapiprivxenon: completion of synchronous enumeration succeed\n"));

                NetPrintf(("friendapiprivxenon: the friend api module is being destroyed\n"));

                // releave friends event handler
                CloseHandle(pFriendApiRef->enumeration.hEventListener);

                // destroy critical section used to ensure thread safety in handler registered with NP basic utility
                NetCritKill(&pFriendApiRef->common.notifyCrit);

                // release module memory
                DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);

                iRetVal = 0;
                break;

            case ERROR_IO_PENDING:
                NetPrintf(("friendapiprivxenon: completion of synchronous enumeration still pending\n", dwResult));

                iRetVal = -1;
                break;

            default:
                NetPrintf(("friendapiprivxenon: failed to wait for completion of synchronous enumeration. error = %d\n", dwResult));

                NetPrintf(("friendapiprivxenon: the friend api module is being destroyed\n"));

                // releave friends event handler
                CloseHandle(pFriendApiRef->enumeration.hEventListener);

                // destroy critical section used to ensure thread safety in handler registered with NP basic utility
                NetCritKill(&pFriendApiRef->common.notifyCrit);

                // release module memory
                DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);

                iRetVal = -2;
                break;
        }
    }

    return(iRetVal);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiPrivNotifyEventInitComplete

    \Description
        Generate events until the entire friendlist was signaled to a newly registered callback, followed by a
        FRIENDAPI_EVENT_INIT_COMPLETE event

    \Input *pFriendApiRef   - module reference

    \Version 08/25/2012 (akirchner)
*/
/********************************************************************************F*/
static void _FriendApiPrivNotifyEventInitComplete(FriendApiRefT *pFriendApiRef)
{
    int32_t iUserIndex;
    int32_t iCallbackIndex;
    int32_t iFriendIndex;

    NetCritEnter(&pFriendApiRef->common.notifyCrit);

    if (pFriendApiRef->enumeration.bEnumerating == FALSE)
    {
        for (iUserIndex = 0; iUserIndex < NETCONN_MAXLOCALUSERS; iUserIndex++)
        {
            if (pFriendApiRef->aFriendsCollection[iUserIndex].bEndOfInitialPresence == TRUE)
            {
                for (iCallbackIndex = 0; iCallbackIndex < FRIENDAPI_MAX_CALLBACKS; iCallbackIndex++)
                {
                    if ((pFriendApiRef->common.callback[iCallbackIndex].pNotifyCb) && (pFriendApiRef->common.callback[iCallbackIndex].bInitFinished[iUserIndex] == FALSE))
                    {
                        FriendEventDataT eventData;

                        uint8_t oldFriendsCollectionIndex =  pFriendApiRef->aFriendsCollection[iUserIndex].bOldFriendsCollectionIndex;
                        FriendsUpdateT *pOldFriendsUpdate = &pFriendApiRef->aFriendsCollection[iUserIndex].aFriendsUpdate[oldFriendsCollectionIndex];

                        for (iFriendIndex = 0; iFriendIndex < pOldFriendsUpdate->iFriends; iFriendIndex++)
                        {
                            memset(&eventData, 0, sizeof(FriendEventDataT));

                            eventData.eventType  = FRIENDAPI_EVENT_FRIEND_UPDATED;
                            eventData.uUserIndex = iUserIndex;
                            eventData.eventTypeData.FriendUpdated.bPresence = pFriendApiRef->aFriendsCollection[iUserIndex].aOnlineFriends[iFriendIndex].dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE;
                            eventData.eventTypeData.FriendUpdated.pFriendData = &pOldFriendsUpdate->aFriends[iFriendIndex];

                            FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);
                        }

                        memset(&eventData, 0, sizeof(FriendEventDataT));

                        eventData.eventType  = FRIENDAPI_EVENT_INIT_COMPLETE;
                        eventData.uUserIndex = iUserIndex;

                        pFriendApiRef->common.callback[iCallbackIndex].pNotifyCb(pFriendApiRef, &eventData, pFriendApiRef->common.callback[iCallbackIndex].pNotifyCbUserData);
                        pFriendApiRef->common.callback[iCallbackIndex].bInitFinished[iUserIndex] = TRUE;
                    }
                }
            }
        }
    }

    NetCritLeave(&pFriendApiRef->common.notifyCrit);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiPrivFindFriend

    \Description
        Find the friend's index in the array of friends using the xuid provided 

    \Input pFriendsUpdate - Friends' state
    \Input pFriendData    - Searched friend

    \Output
        int32_t               - index of searched friend (-1 = not found)

    \Version 07/24/2012 (akirchner)
*/
/********************************************************************************F*/
static int32_t _FriendApiPrivFindFriend(FriendsUpdateT *pFriendsUpdate, FriendDataT *pFriendData)
{
    int32_t iFriend;
    XUID searchedXUID = ((XONLINE_FRIEND *)pFriendData->pRawData1)->xuid;

    for (iFriend = 0; iFriend < pFriendsUpdate->iFriends; iFriend++)
    {
        XUID currentXUID = ((XONLINE_FRIEND *)pFriendsUpdate->aFriends[iFriend].pRawData1)->xuid;

        if (IsEqualXUID(currentXUID, searchedXUID))
        {
            return(iFriend);
        }
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiPrivUpdateFriendLists

    \Description
        Update lists of friend

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - index of user that is going to have his friendlist updated

    \Version 07/24/2012 (akirchner)
*/
/********************************************************************************F*/
static void _FriendApiPrivUpdateFriendLists(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex)
{
    FriendEventDataT eventData;

    uint8_t oldFriendsCollectionIndex = pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex;
    uint8_t newFriendsCollectionIndex = (oldFriendsCollectionIndex + 1) % 2;

    int32_t iOldFriendIndex = 0;
    int32_t iNewFriendIndex = 0;

    FriendsUpdateT *pOldFriendsUpdate = &pFriendApiRef->aFriendsCollection[uUserIndex].aFriendsUpdate[oldFriendsCollectionIndex];
    FriendsUpdateT *pNewFriendsUpdate = &pFriendApiRef->aFriendsCollection[uUserIndex].aFriendsUpdate[newFriendsCollectionIndex];

    int32_t iFriend;

    // populate new collection of friends
    pNewFriendsUpdate->iFriends = pFriendApiRef->enumeration.dwFriendsEnumerated;

    for (iFriend = 0; iFriend < pNewFriendsUpdate->iFriends; iFriend++)
    {
        ds_snzprintf(pNewFriendsUpdate->aFriends[iFriend].strFriendId, sizeof(pNewFriendsUpdate->aFriends[iFriend].strFriendId), "%I64d", pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend].xuid);
        strncpy(pNewFriendsUpdate->aFriends[iFriend].strFriendName, pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend].szGamertag, sizeof(pNewFriendsUpdate->aFriends[iFriend].strFriendName));

        pNewFriendsUpdate->aFriends[iFriend].pRawData1 = &pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend];
        pNewFriendsUpdate->aStates [iFriend]           =  pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend].dwFriendState;
    }

    // search new and updated friends
    for (iNewFriendIndex = 0; iNewFriendIndex < pNewFriendsUpdate->iFriends; iNewFriendIndex++)
    {
        iOldFriendIndex = _FriendApiPrivFindFriend(pOldFriendsUpdate, & pNewFriendsUpdate->aFriends[iNewFriendIndex]);

        if ((iOldFriendIndex == -1) || (pOldFriendsUpdate->aStates[iOldFriendIndex] != ((XONLINE_FRIEND *)pNewFriendsUpdate->aFriends[iNewFriendIndex].pRawData1)->dwFriendState))
        {
            memset(&eventData, 0, sizeof(FriendEventDataT));

            eventData.eventType  = FRIENDAPI_EVENT_FRIEND_UPDATED;
            eventData.uUserIndex = uUserIndex;
            eventData.eventTypeData.FriendUpdated.bPresence = ((XONLINE_FRIEND *)pNewFriendsUpdate->aFriends[iNewFriendIndex].pRawData1)->dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE;
            eventData.eventTypeData.FriendUpdated.pFriendData = &pNewFriendsUpdate->aFriends[iNewFriendIndex];

            FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);

            pNewFriendsUpdate->aStates[iNewFriendIndex] = ((XONLINE_FRIEND *)pNewFriendsUpdate->aFriends[iNewFriendIndex].pRawData1)->dwFriendState;

            pFriendApiRef->common.uFriendListVersion++;
            pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex = newFriendsCollectionIndex;

#if DIRTYCODE_LOGGING
            NetPrintf(("friendapiprivxenon: friendslist updated (user = %d, new friend count = %d, new version = %d)\n", uUserIndex, pNewFriendsUpdate->iFriends, pFriendApiRef->common.uFriendListVersion));
#endif
        }
    }

    // search deleted friends
    for (iOldFriendIndex = 0; iOldFriendIndex < pOldFriendsUpdate->iFriends; iOldFriendIndex++)
    {
        iNewFriendIndex = _FriendApiPrivFindFriend(pNewFriendsUpdate, & pOldFriendsUpdate->aFriends[iOldFriendIndex]);

        if (iNewFriendIndex == -1)
        {
            memset(&eventData, 0, sizeof(FriendEventDataT));

            eventData.eventType  = FRIENDAPI_EVENT_FRIEND_REMOVED;
            eventData.uUserIndex = uUserIndex;
            eventData.eventTypeData.FriendRemoved.pFriendData = &pNewFriendsUpdate->aFriends[iNewFriendIndex];

            FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);

            pFriendApiRef->common.uFriendListVersion++;
            pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex = newFriendsCollectionIndex;

#if DIRTYCODE_LOGGING
            NetPrintf(("friendapiprivxenon: friendslist updated (user = %d, new friend count = %d, new version = %d)\n", uUserIndex, pNewFriendsUpdate->iFriends, pFriendApiRef->common.uFriendListVersion));
#endif
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _FriendApiPrivEnumerateUser

    \Description
        Enumerate friendlist of single user

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - index of user to enumerate

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 09/01/2012 (akirchner)

*/
/********************************************************************************F*/
static int32_t _FriendApiPrivEnumerateUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex)
{
    int32_t iResult;
    DWORD dwBufferSize;

    iResult = XFriendsCreateEnumerator(pFriendApiRef->enumeration.ulUserId, 0, FRIENDAPI_MAX_FRIENDS, &dwBufferSize, &pFriendApiRef->enumeration.hEnumHandle);

    if (iResult != ERROR_SUCCESS)
    {
        NetPrintf(("friendapiprivxenon: XFriendsCreateEnumerator() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(-1);
    }

    memset(pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].aOnlineFriends, 0, sizeof(pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].aOnlineFriends));

    iResult = XEnumerate(pFriendApiRef->enumeration.hEnumHandle,
                         pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].aOnlineFriends,
                         sizeof(pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].aOnlineFriends),
                         0,
                         &pFriendApiRef->enumeration.xOverlapped);

    if (iResult != ERROR_IO_PENDING)
    {
        NetPrintf(("friendapiprivxenon: XEnumerate() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(-1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _FriendApiPrivEnumerate

    \Description
        Enumerate friendlist of all users

    \Input *pFriendApiRef   - module reference

    \Version 09/01/2012 (akirchner)

*/
/********************************************************************************F*/
static void _FriendApiPrivEnumerate(FriendApiRefT *pFriendApiRef)
{
    int32_t iResult;

    // start enumeration
    if(pFriendApiRef->enumeration.bEnumerating == FALSE)
    {
        DWORD dwNotify;

        if (XNotifyGetNext(pFriendApiRef->enumeration.hEventListener, 0, &dwNotify, &pFriendApiRef->enumeration.ulUserId) != FALSE)
        {
            if(_FriendApiPrivEnumerateUser(pFriendApiRef, pFriendApiRef->enumeration.ulUserId) == 0)
            {
                pFriendApiRef->enumeration.bEnumerating = TRUE;
            }
        }
        else
        {
            for (pFriendApiRef->enumeration.ulUserId = 0; pFriendApiRef->enumeration.ulUserId < NETCONN_MAXLOCALUSERS; pFriendApiRef->enumeration.ulUserId++)
            {
                // check if the logged in user has changed
                XUSER_SIGNIN_STATE State = XUserGetSigninState(pFriendApiRef->enumeration.ulUserId);
                if (State != eXUserSigninState_SignedInToLive && pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].bEndOfInitialPresence)
                {
                    pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].bEndOfInitialPresence = FALSE;
                }

                if ((pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].bEndOfInitialPresence == FALSE) && (State == eXUserSigninState_SignedInToLive))
                {
                    if(_FriendApiPrivEnumerateUser(pFriendApiRef, pFriendApiRef->enumeration.ulUserId) == 0)
                    {
                        pFriendApiRef->enumeration.bEnumerating = TRUE;
                        break;
                    }
                }
            }
        }
    }

    // finish enumeration
    if (pFriendApiRef->enumeration.bEnumerating == TRUE)
    {
        DWORD dwEnumerationStatus;
        iResult = XGetOverlappedResult(&pFriendApiRef->enumeration.xOverlapped, &pFriendApiRef->enumeration.dwFriendsEnumerated, FALSE);

        switch(iResult)
        {
            case ERROR_SUCCESS:
                _FriendApiPrivUpdateFriendLists(pFriendApiRef, pFriendApiRef->enumeration.ulUserId);
                pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].bEndOfInitialPresence = TRUE;

                NetPrintf(("friendapiprivxenon: XGetOverlappedResult() completed enumeration for user = %d\n", pFriendApiRef->enumeration.ulUserId));
                pFriendApiRef->enumeration.bEnumerating = FALSE;

                if(XCloseHandle(pFriendApiRef->enumeration.hEnumHandle) == 0)
                {
                    NetPrintf(("friendapiprivxenon: XCloseHandle() failed to close handle\n"));
                }

                break;

            case ERROR_IO_INCOMPLETE:
                break;

            default:
                // amakoukji: I believe this first 'if' is not necessary and we should remove it in favor of the first 'else if' below it
                if(pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].bEndOfInitialPresence == FALSE)
                {
                    _FriendApiPrivUpdateFriendLists(pFriendApiRef, pFriendApiRef->enumeration.ulUserId);
                    pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].bEndOfInitialPresence = TRUE;

                    NetPrintf(("friendapiprivxenon: XGetOverlappedResult() completed initial enumeration for user = %d\n", pFriendApiRef->enumeration.ulUserId));
                }
                else if ((dwEnumerationStatus = XGetOverlappedExtendedError(&pFriendApiRef->enumeration.xOverlapped)) == (unsigned)HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
                {
                    // friends list is empty
                    _FriendApiPrivUpdateFriendLists(pFriendApiRef, pFriendApiRef->enumeration.ulUserId);
                    pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].bEndOfInitialPresence = TRUE;

                    NetPrintf(("friendapiprivxenon: XGetOverlappedResult() completed enumeration for user = %d\n", pFriendApiRef->enumeration.ulUserId));
                    pFriendApiRef->enumeration.bEnumerating = FALSE;

                    if(XCloseHandle(pFriendApiRef->enumeration.hEnumHandle) == 0)
                    {
                        NetPrintf(("friendapiprivxenon: XCloseHandle() failed to close handle\n"));
                    }

                    break;
                }
                else
                {
                    NetPrintf(("friendapiprivxenon: XGetOverlappedResult() failed (err=%s)\n", DirtyErrGetName(iResult)));
                }

                pFriendApiRef->enumeration.bEnumerating = FALSE;

                if(XCloseHandle(pFriendApiRef->enumeration.hEnumHandle) == 0)
                {
                    NetPrintf(("friendapiprivxenon: XCloseHandle() failed to close handle\n"));
                }

                break;
        }
    }
}

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function FriendApiPrivCreate

    \Description
        Create the xenon friendapi object.

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
        NetPrintf(("friendapiprivxenon: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pFriendApiRef, 0, sizeof(*pFriendApiRef));
    NetPrintf(("friendapiprivxenon: memory consumed by module state = %d bytes\n", sizeof(*pFriendApiRef)));

    // create listener for live events
    if ((pFriendApiRef->enumeration.hEventListener = XNotifyCreateListener(XNOTIFY_FRIENDS)) == NULL)
    {
        NetPrintf(("friendapiprivxenon: unable to create live event listener\n"));
        DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);
        return(NULL);
    }

    // save parms
    pFriendApiRef->common.iMemGroup = iMemGroup;
    pFriendApiRef->common.pMemGroupUserData = pMemGroupUserData;

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
        _FriendApiPrivNotifyEventInitComplete(pFriendApiRef);

        _FriendApiPrivEnumerate(pFriendApiRef);

        // notify asynchronous block user operation completion if need be
        if (pFriendApiRef->bNotifyBlockUserResult)
        {
            FriendEventDataT eventData;
            memset(&eventData, 0, sizeof(eventData));
            eventData.eventType = FRIENDAPI_EVENT_BLOCK_USER_RESULT;
            eventData.eventTypeData.BlockUserResult.bSuccess = TRUE;
            FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);

            pFriendApiRef->bNotifyBlockUserResult = FALSE;
        }
    }
    else
    {
        // flag all friends lists for a refresh
        for (pFriendApiRef->enumeration.ulUserId = 0; pFriendApiRef->enumeration.ulUserId < NETCONN_MAXLOCALUSERS; ++pFriendApiRef->enumeration.ulUserId)
        {
            pFriendApiRef->aFriendsCollection[pFriendApiRef->enumeration.ulUserId].bEndOfInitialPresence = FALSE;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivGetFriendsList

    \Description
        Returns the number of friends and the list of friends in the user-provided buffers.
        Set pFriend to NULL to only find out the number of friends.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - user index
    \Input *pFriendsList    - [out] user-provided buffer to be filled with the friends list (can be NULL)
    \Input *pRawDataList1   - [out] user-provided buffer to be filled with array of XONLINE_FRIEND data structures (can be NULL)
    \Input *pFriendsCount   - [out] user-provided buffer to be filled with the number of friends in the list (can be NULL)

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivGetFriendsList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, FriendDataT *pFriendsList, void *pRawDataList1, int32_t *pFriendsCount)
{
    if (pFriendsCount)
    {
        *pFriendsCount = pFriendApiRef->aFriendsCollection[uUserIndex].aFriendsUpdate[pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex].iFriends;
    }

    if (pRawDataList1)
    {
        int32_t iFriend;

        XONLINE_FRIEND * pOnlineFriends = (XONLINE_FRIEND *)pRawDataList1;

        for (iFriend = 0; iFriend < pFriendApiRef->aFriendsCollection[uUserIndex].aFriendsUpdate[pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex].iFriends; iFriend++)
        {
            pOnlineFriends[iFriend] = pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend];
        }
    }

    if (pFriendsList)
    {
        int32_t iFriend;

        for (iFriend = 0; iFriend < pFriendApiRef->aFriendsCollection[uUserIndex].aFriendsUpdate[pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex].iFriends; iFriend++)
        {
            ds_snzprintf(pFriendsList[iFriend].strFriendId, sizeof(pFriendsList[iFriend].strFriendId), "%I64d", pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend].xuid);
            strncpy(pFriendsList[iFriend].strFriendName, pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend].szGamertag, sizeof(pFriendsList[iFriend].strFriendName));

            if (pRawDataList1)
            {
                pFriendsList[iFriend].pRawData1 = &((XONLINE_FRIEND *)pRawDataList1)[iFriend];
            }
            else
            {
                pFriendsList[iFriend].pRawData1 = 0;
            }
        }
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
        // maximum possible friend count is unknown on xenon
    }

    if (iSelect == 'eoip')
    {
        uint32_t uUserIndex;

        memcpy(&uUserIndex, pBuf, iBufSize);

        return(pFriendApiRef->aFriendsCollection[uUserIndex].bEndOfInitialPresence);
    }

    NetPrintf(("friendapiprivxenon: unhandled status selector ('%c%c%c%c')\n",
            (uint8_t)(iSelect>>24), (uint8_t)(iSelect>>16), (uint8_t)(iSelect>>8), (uint8_t)(iSelect)));

    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivDestroy

    \Description
        Release friend api component if enumeration is done, otherwise queue release of the xenon friend api component

    \Input *pFriendApiRef   - module reference

    \Version 04/19/2010 (mclouatre) - Initial version
    \Version 09/13/2012 (akirchner) - Queue release of friend api component if there is an enumeration going on
*/
/********************************************************************************F*/
void FriendApiPrivDestroy(FriendApiRefT *pFriendApiRef)
{
    if (pFriendApiRef->enumeration.bEnumerating == TRUE)
    {
        NetPrintf(("friendapiprivxenon: postponing destruction of friend api module\n"));
        NetConnControl('recu', 0, 0, (void *)_FriendApiPrivFreeCallback, pFriendApiRef);
    }
    else
    {
        NetPrintf(("friendapiprivxenon: the friend api module is being destroyed\n"));

        // release friends event handler
        CloseHandle(pFriendApiRef->enumeration.hEventListener);

        // destroy critical section used to ensure thread safety in handler registered with NP basic utility
        NetCritKill(&pFriendApiRef->common.notifyCrit);

        // release module memory
        DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function FriendApiIsUserFriend

    \Description
        Returns whether a given user is friend

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user for which "friend" status needs to be queried
                                on xenon: pUserId needs to point to a XUID

    \Output
        uint32_t            - TRUE if user is friend, FALSE otherwise

    \Version 09/13/2012 (akirchner)
*/
/********************************************************************************F*/
uint32_t FriendApiPrivIsUserFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    XUID * pXuid = (XUID *) pUserId;
    int32_t iFriend;

    for (iFriend = 0; iFriend < pFriendApiRef->aFriendsCollection[uUserIndex].aFriendsUpdate[pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex].iFriends; iFriend++)
    {
        if (IsEqualXUID(pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend].xuid, *pXuid))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivIsUserBlocked

    \Description
        Returns whether a given user is blocked or not.
        On Xenon, a "blocked" user is a "muted" user.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user for which "blocked" status needs to be queried (pointer to XUID)

    \Output
        uint32_t            - TRUE if user is blocked, FALSE otherwise

    \Version 02/24/2011 (mclouatre)
*/
/********************************************************************************F*/
uint32_t FriendApiPrivIsUserBlocked(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    XUID * pXuid = (XUID *) pUserId;
    BOOL bMuted = FALSE;
    DWORD dwResult = 0;

    if ((dwResult = XUserMuteListQuery(uUserIndex, *pXuid, &bMuted)) != ERROR_SUCCESS)
    {
        NetPrintf(("friendapiprivxenon: XUserMuteListQuery() failed with %s\n", DirtyErrGetName(dwResult)));
    }

    return(bMuted);
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

    \Version 02/24/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivBlockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    XUID *pXuid = (XUID *) pUserId;
    DWORD dwResult = 0;

    if (pFriendApiRef->bNotifyBlockUserResult)
    {
        NetPrintf(("friendapiprivxenon: warning - cannot initiate two asynchronous block user operations concurrently\n"));
        return(-1);
    }

    if ((dwResult = XUserMuteListSetState(uUserIndex, *pXuid, TRUE)) != ERROR_SUCCESS)
    {
        NetPrintf(("friendapiprivxenon: XUserMuteListSetState(TRUE) failed with %s\n", DirtyErrGetName(dwResult)));
        return(-1);
    }

    NetPrintf(("friendapiprivxenon: asynchronous block user operation successfully initiated\n"));

    pFriendApiRef->bNotifyBlockUserResult = TRUE;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivUnblockUser

    \Description
        Unblock user

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user to be unblocked (pointer to SceNpId)

    \Output
        int32_t            - 0 for success, negative for failure

    \Version 02/24/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivUnblockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    XUID * pXuid = (XUID *) pUserId;
    DWORD dwResult = 0;

    if ((dwResult = XUserMuteListSetState(uUserIndex, *pXuid, FALSE)) != ERROR_SUCCESS)
    {
        NetPrintf(("friendapiprivxenon: XUserMuteListSetState(FALSE) failed with %s\n", DirtyErrGetName(dwResult)));
        return(-1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivGetBlockListVersion

    \Description
        Returns the current version of the list of blocked users. Unsupported on Xenon.
        Always returns 0.

    \Input *pFriendApiRef   - module reference
    \Input *pVersion        - [out] pointer to buffer to be filled with version number

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 05/10/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivGetBlockListVersion(FriendApiRefT *pFriendApiRef, uint32_t *pVersion)
{
    *pVersion = 0;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivGetBlockList

    \Description
        Returns the list of blocked users in the user-provided buffers. Unsupported on Xenon.
        Always returns an empty list.

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
    *pUsersCount = 0;

    return(0);
}
