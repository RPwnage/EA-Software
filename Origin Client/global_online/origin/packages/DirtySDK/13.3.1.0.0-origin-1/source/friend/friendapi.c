/*H********************************************************************************/
/*!
    \File friendapi.c

    \Description
        The FriendApi module abstracts the different first-party friend apis. It
        locally maintains a generic local list of friends, and it provides higher-level
        modules with the ability of registering a notification callback reporting
        changes in the list of friends on a per-friend basis.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/friend/friendapi.h"
#include "friendapipriv.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//! friend api module state
struct FriendApiRefT
{
    FriendApiCommonRefT common;     //!< platform-independent portion
};

/*** Function Prototypes **********************************************************/

/*** Private functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function FriendApiCreate

    \Description
        Create the friendapi object.

    \Output
        FriendApiRefT*  - module reference (NULL = failed)

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
FriendApiRefT *FriendApiCreate()
{
    FriendApiRefT *pFriendApiRef;

    NetPrintf(("friendapi: module being created\n"));

    // allocate the module state struct
    if ((pFriendApiRef = FriendApiPrivCreate()) == NULL)
    {
        NetPrintf(("friendapi: unable to create module state\n"));
    }

    // return the module ref
    return(pFriendApiRef);
}

/*F********************************************************************************/
/*!
    \Function FriendApiAddCallback

    \Description
        Register callback for friends notifications.

    \Input *pFriendApiRef       - module reference
    \Input *pNotifyCb           - user-provided callback (cannot be NULL)
    \Input *pNotifyCbUserData   - user data to be passed when invoking the callback (can be NULL)

    \Output
        int32_t                 - negative means error. 0 or greater: slot used

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiAddCallback(FriendApiRefT *pFriendApiRef, FriendApiNotifyCbT *pNotifyCb, void *pNotifyCbUserData)
{
    int32_t iIndex;
    int32_t iRetCode = -1;

#if defined(DIRTYCODE_PS3)
    NetCritEnter(&pFriendApiRef->common.notifyCrit);
#endif

    for(iIndex = 0; iIndex < FRIENDAPI_MAX_CALLBACKS; iIndex++)
    {
        if (pFriendApiRef->common.callback[iIndex].pNotifyCb == NULL)
        {
            pFriendApiRef->common.callback[iIndex].pNotifyCb = pNotifyCb;
            pFriendApiRef->common.callback[iIndex].pNotifyCbUserData = pNotifyCbUserData;

            NetPrintf(("friendapi: new notification callback registered at index %d\n", iIndex));

            iRetCode = iIndex;
            break;
        }
    }

#if defined(DIRTYCODE_PS3)
    NetCritLeave(&pFriendApiRef->common.notifyCrit);
#endif

#if DIRTYCODE_LOGGING
    if (iRetCode == -1)
    {
        NetPrintf(("friendapi: critical error! internal list of notification callbacks is full\n"));
    }
#endif

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function FriendApiRemoveCallback

    \Description
        Unregister a callback

    \Input *pFriendApiRef       - module reference
    \Input *pNotifyCb           - the callback to remove (cannot be NULL)
    \Input *pNotifyCbUserData   - the user data that was originally passed in (can be NULL)

    \Output
        int32_t                 - negative means error. 0 or greater: slot freed

    \Version 06/15/2008 (mclouatre) First Version
*/
/********************************************************************************F*/
int32_t FriendApiRemoveCallback(FriendApiRefT *pFriendApiRef, FriendApiNotifyCbT *pNotifyCb, void *pNotifyCbUserData)
{
    int32_t iIndex;
    int32_t iRetCode = -1;

#if defined(DIRTYCODE_PS3)
    NetCritEnter(&pFriendApiRef->common.notifyCrit);
#endif

    for(iIndex = 0; iIndex < FRIENDAPI_MAX_CALLBACKS; iIndex++)
    {
        if ((pFriendApiRef->common.callback[iIndex].pNotifyCb == pNotifyCb) && (pFriendApiRef->common.callback[iIndex].pNotifyCbUserData == pNotifyCbUserData))
        {
            int32_t iUser;

            for(iUser = 0; iUser < NETCONN_MAXLOCALUSERS; iUser++)
            {
                pFriendApiRef->common.callback[iIndex].bInitFinished[iUser] = FALSE;
            }

            pFriendApiRef->common.callback[iIndex].pNotifyCb = NULL;
            pFriendApiRef->common.callback[iIndex].pNotifyCbUserData = NULL;

            NetPrintf(("friendapi: notification callback at index %d unregistered \n", iIndex));

            iRetCode = iIndex;
            break;;
        }
    }

#if defined(DIRTYCODE_PS3)
    NetCritLeave(&pFriendApiRef->common.notifyCrit);
#endif

#if DIRTYCODE_LOGGING
    if (iRetCode == -1)
    {
        NetPrintf(("friendapi: unregistration of notification callback failed\n"));
    }
#endif

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function FriendApiUpdate

    \Description
        Give module time to perform periodic processing

    \Input *pFriendApiRef       - module reference

    \Version 04/21/2010 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiUpdate(FriendApiRefT *pFriendApiRef)
{
    FriendApiPrivUpdate(pFriendApiRef);
}

/*F********************************************************************************/
/*!
    \Function FriendApiGetFriendsList

    \Description
        Returns the number of friends and list of friends in the user-provided buffers
        (also returns corresponding list of 1st-party friend data structures).

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - user index
    \Input *pFriendsList    - [out] user-provided buffer to be filled with the friends list (can be NULL)
    \Input *pRawDataList1   - [out] user-provided buffer to be filled with array of 1st-party friend data structure 1 (can be NULL)
    \Input *pFriendsCount   - [out] user-provided buffer to be filled with the number of friends in the list (can be NULL)

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Notes
        Set all pointers to NULL except pFriendsCount to find out the number of entries
        that each of the buffers you provide will be filled-in with.

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiGetFriendsList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, FriendDataT *pFriendsList, void *pRawDataList1, int32_t *pFriendsCount)
{
    return(FriendApiPrivGetFriendsList(pFriendApiRef, uUserIndex, pFriendsList, pRawDataList1, pFriendsCount));
}

/*F********************************************************************************/
/*!
    \Function FriendApiGetFriendsListVersion

    \Description
        Returns the current version of friendslist

    \Input *pFriendApiRef   - module reference
    \Input *pVersion        - [out] pointer to buffer to be filled with version number

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 05/20/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiGetFriendsListVersion(FriendApiRefT *pFriendApiRef, uint32_t *pVersion)
{
#if defined(DIRTYCODE_XENON)
    // $todo v9 for now we return the value retrieved from NetConnXenon
    // but once friends support in NetConnXenon is deprecated and the equivalent
    // is embedded in the xenon-flavor of the FriendApi we should no more do this.
    *pVersion = NetConnStatus('frnv', 0, NULL, 0);
#else
    *pVersion = pFriendApiRef->common.uFriendListVersion;
#endif

    return(0);
}

/*F********************************************************************************/
/*!
    \Function FriendApiStatus

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
            'eoip' - stands for "End Of Initial Presence"; returns TRUE if initial presence completed, FALSE otherwise (PS3 only)
        \endverbatim

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiStatus(FriendApiRefT *pFriendApiRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    return(FriendApiPrivStatus(pFriendApiRef, iSelect, pBuf, iBufSize));
}

/*F********************************************************************************/
/*!
    \Function FriendApiControl

    \Description
        Control behavior of module.

    \Input *pFriendApiRef   - module reference
    \Input iControl         - status selector
    \Input iValue           - control value
    \Input iValue2          - control value
    \Input *pValue          - control value

    \Output
        int32_t            - 0 for success, negative for failure

    \Notes
        iControl can be one of the following:

        \verbatim
        \endverbatim

    \Version 05/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiControl(FriendApiRefT *pFriendApiRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    return(0);
}

/*F********************************************************************************/
/*!
    \Function FriendApiDestroy

    \Description
        Destroy the friend api object.

    \Input *pFriendApiRef   - module reference

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiDestroy(FriendApiRefT *pFriendApiRef)
{
    NetPrintf(("friendapi: module is being destroyed\n"));

    FriendApiPrivDestroy(pFriendApiRef);
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
                                on PS3: pUserId needs to point to a SceNpId struct

    \Output
        uint32_t            - TRUE if user is friend, FALSE otherwise

    \Version 09/13/2012 (akirchner)
*/
/********************************************************************************F*/
uint32_t FriendApiIsUserFriend(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    return(FriendApiPrivIsUserFriend(pFriendApiRef, uUserIndex, pUserId));
}

/*F********************************************************************************/
/*!
    \Function FriendApiIsUserBlocked

    \Description
        Returns whether a given user is blocked or not

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user for which "blocked" status needs to be queried
                                on xenon: pUserId needs to point to a XUID
                                on PS3: pUserId needs to point to a SceNpId struct

    \Output
        uint32_t            - TRUE if user is blocked, FALSE otherwise

    \Version 02/24/2011 (mclouatre)
*/
/********************************************************************************F*/
uint32_t FriendApiIsUserBlocked(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    return(FriendApiPrivIsUserBlocked(pFriendApiRef, uUserIndex, pUserId));
}

/*F********************************************************************************/
/*!
    \Function FriendApiBlockUser

    \Description
        Initiates a block user operation. After calling this function, if the return
        code is "success", the caller needs to wait for the FRIENDAPI_EVENT_BLOCK_USER_RESULT
        completion event.

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user to be blocked
                                on xenon: pUserId needs to point to a XUID
                                on PS3: pUserId needs to point to a SceNpId struct

    \Output
        uint32_t            - 0 for success, negative for failure

    \Version 03/07/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiBlockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    return(FriendApiPrivBlockUser(pFriendApiRef, uUserIndex, pUserId));
}

/*F********************************************************************************/
/*!
    \Function FriendApiUnblockUser

    \Description
        Unblock user

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user to be unblocked
                                on xenon: pUserId needs to point to a XUID
                                on PS3: pUserId needs to point to a SceNpId struct

    \Output
        uint32_t            - 0 for success, negative for failure

    \Version 03/07/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiUnblockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    return(FriendApiPrivUnblockUser(pFriendApiRef, uUserIndex, pUserId));
}

/*F********************************************************************************/
/*!
    \Function FriendApiGetBlockListVersion

    \Description
        Returns the current version of the list of blocked users. PS3 only, returns
        0 for other platforms.

    \Input *pFriendApiRef   - module reference
    \Input *pVersion        - [out] pointer to buffer to be filled with version number

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 05/10/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiGetBlockListVersion(FriendApiRefT *pFriendApiRef, uint32_t *pVersion)
{
    return(FriendApiPrivGetBlockListVersion(pFriendApiRef, pVersion));
}

/*F********************************************************************************/
/*!
    \Function FriendApiGetBlockList

    \Description
        Returns the list of blocked users in the user-provided buffers PS3 only, always
        returns empty list on other platforms.

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
int32_t FriendApiGetBlockList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, BlockedUserDataT *pBlockList, int32_t *pUsersCount)
{
    return(FriendApiPrivGetBlockList(pFriendApiRef, uUserIndex, pBlockList, pUsersCount));
}
