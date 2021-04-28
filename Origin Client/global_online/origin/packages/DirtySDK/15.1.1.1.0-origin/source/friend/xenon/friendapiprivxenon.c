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

#define FRIEND_XENON_MIN_QUERY_RETRY_TIME 60000

/*** Type Definitions *************************************************************/

typedef struct EnumerationT
{
    int8_t bEnumerating;
    HANDLE hEventListener;
    HANDLE hEnumHandle;
    XOVERLAPPED xOverlapped;
    ULONG ulUserId;
    DWORD dwFriendsEnumerated;
    uint32_t uLastEnumErrorTime;
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
    uint8_t bNotifyAllFriendUpdates;
    uint8_t _pad[2];

    FriendsCollectionT aFriendsCollection[NETCONN_MAXLOCALUSERS];

    EnumerationT enumeration;
};

/*** Function Prototypes **********************************************************/

/*** Private functions ************************************************************/

/*F*************************************************************************************************/
/*!
    \Function _FriendApiPrivCloseHandle

    \Description
        Isolate XCloseHandle from multiple places so that it can log the failure without code duplication

    \Input hHandle - handle to be closed

    \Version 08/20/2013 (abaldeva)
*/
/*************************************************************************************************F*/
static void _FriendApiPrivCloseHandle(HANDLE hHandle)
{
    if(XCloseHandle(hHandle) == 0)
    {
        NetPrintf(("friendapiprivxenon: XCloseHandle() failed to close handle\n"));
    }
}

/*F*************************************************************************************************/
/*!
    \Function _FriendApiPrivFree

    \Description
        Isolate FriendApi clean up code required in multiple functions

    \Input *pFriendApiRef   - pointer to FriendApiRefT instance

    \Version 08/20/2013 (mclouatre)
*/
/*************************************************************************************************F*/
static void _FriendApiPrivFree(FriendApiRefT *pFriendApiRef)
{
    NetPrintf(("friendapiprivxenon: the friend api module is being destroyed\n"));

    _FriendApiPrivCloseHandle(pFriendApiRef->enumeration.hEventListener);

    NetCritKill(&pFriendApiRef->common.notifyCrit);

    DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);
}

/*F*************************************************************************************************/
/*!
    \Function _FriendApiPrivFreeCallback

    \Description
        Clean up friendapiprivxenon instance if asynchronous enumeration is pending at FriendApiPrivDestroy call

    \Input *pFriendApiRefInstance - pointer to friendapiprivxenon instance

    \Output
        int32_t   zero = success; -1 = try again; 

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
                NetPrintf(("friendapiprivxenon: asynchronous enumeration completed. Operation succeeded.\n"));
                iRetVal = 0;
                break;

            case ERROR_IO_PENDING:
                NetPrintf(("friendapiprivxenon: asynchronous enumeration still pending ...\n"));
                iRetVal = -1;
                break;

            default:
                NetPrintf(("friendapiprivxenon: asynchronous enumeration completed. Operation failed. error = %s\n", DirtyErrGetName(dwResult)));
                iRetVal = 0;
                break;
        }
    }

    if(iRetVal == 0)
    {
        _FriendApiPrivFree(pFriendApiRef);
        _FriendApiPrivCloseHandle(pFriendApiRef->enumeration.hEnumHandle);
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
    \Function _FriendApiPrivFindRemovedFriend

    \Description
        Find the friend's index in the array of friends using the xuid provided 
        in the FriendDataT. This is a special version for the friend removed case 
        where the rawData is not corrected.

    \Input pFriendsUpdate - Friends' state
    \Input pFriendData    - Searched friend

    \Output
        int32_t               - index of searched friend (-1 = not found)

    \Version 08/21/2013 (amakoukji)
*/
/********************************************************************************F*/
static int32_t _FriendApiPrivFindRemovedFriend(FriendsUpdateT *pFriendsUpdate, FriendDataT *pFriendData)
{
    int32_t iFriend;
    XUID searchedXUID = ds_strtoll(pFriendData->strFriendId, NULL, 10);

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

        if ((pFriendApiRef->bNotifyAllFriendUpdates == TRUE) || (iOldFriendIndex == -1) || (pOldFriendsUpdate->aStates[iOldFriendIndex] != ((XONLINE_FRIEND *)pNewFriendsUpdate->aFriends[iNewFriendIndex].pRawData1)->dwFriendState))
        {
            memset(&eventData, 0, sizeof(FriendEventDataT));

            eventData.eventType  = FRIENDAPI_EVENT_FRIEND_UPDATED;
            eventData.uUserIndex = uUserIndex;
            eventData.eventTypeData.FriendUpdated.bPresence = ((XONLINE_FRIEND *)pNewFriendsUpdate->aFriends[iNewFriendIndex].pRawData1)->dwFriendState & XONLINE_FRIENDSTATE_FLAG_ONLINE;
            eventData.eventTypeData.FriendUpdated.pFriendData = &pNewFriendsUpdate->aFriends[iNewFriendIndex];

            pNewFriendsUpdate->aStates[iNewFriendIndex] = ((XONLINE_FRIEND *)pNewFriendsUpdate->aFriends[iNewFriendIndex].pRawData1)->dwFriendState;

            pFriendApiRef->common.uFriendListVersion++;
            pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex = newFriendsCollectionIndex;

            FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);

            NetPrintf(("friendapiprivxenon: friendslist updated (user = %d, new friend count = %d, new version = %d)\n", uUserIndex, pNewFriendsUpdate->iFriends, pFriendApiRef->common.uFriendListVersion));
        }
    }

    // search deleted friends
    for (iOldFriendIndex = 0; iOldFriendIndex < pOldFriendsUpdate->iFriends; iOldFriendIndex++)
    {
        iNewFriendIndex = _FriendApiPrivFindRemovedFriend(pNewFriendsUpdate, & pOldFriendsUpdate->aFriends[iOldFriendIndex]);

        if (iNewFriendIndex == -1)
        {
            memset(&eventData, 0, sizeof(FriendEventDataT));

            eventData.eventType  = FRIENDAPI_EVENT_FRIEND_REMOVED;
            eventData.uUserIndex = uUserIndex;
            eventData.eventTypeData.FriendRemoved.pFriendData = &pOldFriendsUpdate->aFriends[iOldFriendIndex];

            pFriendApiRef->common.uFriendListVersion++;
            pFriendApiRef->aFriendsCollection[uUserIndex].bOldFriendsCollectionIndex = newFriendsCollectionIndex;

            FriendApiPrivNotifyEvent(pFriendApiRef, &eventData);

            NetPrintf(("friendapiprivxenon: friendslist updated (user = %d, new friend count = %d, new version = %d)\n", uUserIndex, pNewFriendsUpdate->iFriends, pFriendApiRef->common.uFriendListVersion));
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

    // wait a minimum of 60 seconds between failed queries
    if (pFriendApiRef->enumeration.uLastEnumErrorTime != 0)
    {
        if (NetTickDiff(NetTick(), pFriendApiRef->enumeration.uLastEnumErrorTime) < FRIEND_XENON_MIN_QUERY_RETRY_TIME)
        {
            // defer retry
            return(-1);
        }
        else
        {
            // retry timer is up, reset the error timer
            pFriendApiRef->enumeration.uLastEnumErrorTime = 0;
        }
    }

    iResult = XFriendsCreateEnumerator(uUserIndex, 0, FRIENDAPI_MAX_FRIENDS, &dwBufferSize, &pFriendApiRef->enumeration.hEnumHandle);

    if (iResult != ERROR_SUCCESS)
    {
        pFriendApiRef->enumeration.uLastEnumErrorTime = NetTick();
        NetPrintf(("friendapiprivxenon: XFriendsCreateEnumerator() failed (err=%s)\n", DirtyErrGetName(iResult)));
        return(-1);
    }

    memset(pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends, 0, sizeof(pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends));

    NetPrintf(("friendapiprivxenon: XEnumerate() started\n"));
    memset(&pFriendApiRef->enumeration.xOverlapped, 0, sizeof(pFriendApiRef->enumeration.xOverlapped));
    iResult = XEnumerate(pFriendApiRef->enumeration.hEnumHandle,
                         pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends,
                         sizeof(pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends),
                         0,
                         &pFriendApiRef->enumeration.xOverlapped);

    if (iResult != ERROR_IO_PENDING)
    {
        NetPrintf(("friendapiprivxenon: XEnumerate() failed (err=%s)\n", DirtyErrGetName(iResult)));
        _FriendApiPrivCloseHandle(pFriendApiRef->enumeration.hEnumHandle);
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
            uint32_t uUserIndex;
            for (uUserIndex = 0; uUserIndex < NETCONN_MAXLOCALUSERS; uUserIndex++)
            {
                // check if the logged in user has changed
                XUSER_SIGNIN_INFO XuserInfo;
                XUSER_SIGNIN_STATE State = XUserGetSigninState(uUserIndex);

                XUserGetSigninInfo(uUserIndex, XUSER_GET_SIGNIN_INFO_ONLINE_XUID_ONLY, &XuserInfo);
                if (State != eXUserSigninState_SignedInToLive && pFriendApiRef->aFriendsCollection[uUserIndex].bEndOfInitialPresence)
                {
                    pFriendApiRef->aFriendsCollection[uUserIndex].bEndOfInitialPresence = FALSE;
                }

                if ((pFriendApiRef->aFriendsCollection[uUserIndex].bEndOfInitialPresence == FALSE) && (State == eXUserSigninState_SignedInToLive) && !(XuserInfo.dwInfoFlags & XUSER_INFO_FLAG_GUEST))
                {
                    if(_FriendApiPrivEnumerateUser(pFriendApiRef, uUserIndex) == 0)
                    {
                        pFriendApiRef->enumeration.bEnumerating = TRUE;
                        pFriendApiRef->enumeration.ulUserId = uUserIndex;
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

                _FriendApiPrivCloseHandle(pFriendApiRef->enumeration.hEnumHandle);

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
                }
                else
                {
                    NetPrintf(("friendapiprivxenon: XGetOverlappedResult() failed (err=%s)\n", DirtyErrGetName(iResult)));
                }

                pFriendApiRef->enumeration.bEnumerating = FALSE;

                _FriendApiPrivCloseHandle(pFriendApiRef->enumeration.hEnumHandle);

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
        uint32_t uUserIndex;
        for (uUserIndex = 0; uUserIndex < NETCONN_MAXLOCALUSERS; ++uUserIndex)
        {
            pFriendApiRef->aFriendsCollection[uUserIndex].bEndOfInitialPresence = FALSE;
        }

        if (pFriendApiRef->enumeration.bEnumerating != FALSE)
        {
            XCancelOverlapped(&pFriendApiRef->enumeration.xOverlapped);
            pFriendApiRef->enumeration.bEnumerating = FALSE;
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

        ds_memcpy_s(&uUserIndex, sizeof(uUserIndex), pBuf, iBufSize);

        return(pFriendApiRef->aFriendsCollection[uUserIndex].bEndOfInitialPresence);
    }

    NetPrintf(("friendapiprivxenon: unhandled status selector ('%c%c%c%c')\n",
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
            'alfu' - if iValue=1 set the friend api to surface all MS events
        \endverbatim

    \Version 09/25/2014 (amakoukji)
*/
/********************************************************************************F*/
int32_t FriendApiPrivControl(FriendApiRefT *pFriendApiRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    int32_t iResult = 0;
    if (iControl == 'alfu')
    {
        if (iValue > 0)
        {
            pFriendApiRef->bNotifyAllFriendUpdates = TRUE;
        }
        else
        {
            pFriendApiRef->bNotifyAllFriendUpdates = FALSE;
        }
        return(iResult);
    }

    NetPrintf(("friendapiprivxenon: unhandled control selector ('%c%c%c%c')\n",
            (uint8_t)(iControl>>24), (uint8_t)(iControl>>16), (uint8_t)(iControl>>8), (uint8_t)(iControl)));

    // unhandled
    return(iResult);
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
        _FriendApiPrivFree(pFriendApiRef);
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
    \Function FriendApiIsUserRealFriend

    \Description
        Returns whether a given user is a 'real' friend (not pending)

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user for which "friend" status needs to be queried
                                on xenon: pUserId needs to point to a XUID

    \Output
        uint32_t            - TRUE if user is friend, FALSE otherwise

    \Version 07/17/2013 (jbrookes)
*/
/********************************************************************************F*/
uint32_t FriendApiPrivIsUserRealFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    XUID * pXuid = (XUID *) pUserId;
    const DWORD dwMaskPend = XONLINE_FRIENDSTATE_FLAG_SENTREQUEST|XONLINE_FRIENDSTATE_FLAG_RECEIVEDREQUEST;
    int32_t iFriend;

    for (iFriend = 0; iFriend < FRIENDAPI_MAX_FRIENDS; iFriend++)
    {
        if (IsEqualXUID(pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend].xuid, *pXuid))
        {
            if ((pFriendApiRef->aFriendsCollection[uUserIndex].aOnlineFriends[iFriend].dwFriendState & dwMaskPend) == 0)
            {
                return(TRUE);
            }
            break;
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
