/*H********************************************************************************/
/*!
    \File friendapistub.c

    \Description
        Stub version of the friend api module for platform where the module is not
        really used.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

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

/*** Privat  functions ************************************************************/


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
FriendApiRefT *FriendApiCreate(void)
{
    FriendApiRefT *pFriendApiRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate the module state struct
    if ((pFriendApiRef = DirtyMemAlloc(sizeof(*pFriendApiRef), FRIENDAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("friendapistub: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pFriendApiRef, 0, sizeof(*pFriendApiRef));

    // save parms
    pFriendApiRef->common.iMemGroup = iMemGroup;
    pFriendApiRef->common.pMemGroupUserData = pMemGroupUserData;

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

    for(iIndex = 0; iIndex < FRIENDAPI_MAX_CALLBACKS; iIndex++)
    {
        if (pFriendApiRef->common.callback[iIndex].pNotifyCb == NULL)
        {
            pFriendApiRef->common.callback[iIndex].pNotifyCb = pNotifyCb;
            pFriendApiRef->common.callback[iIndex].pNotifyCbUserData = pNotifyCbUserData;

            NetPrintf(("friendapistub: new notification callback registered at index %d\n", iIndex));

            return(iIndex);
        }
    }
    NetPrintf(("friendapistub: critical error! internal list of notification callbacks is full\n"));
    return(-1);
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

    for(iIndex = 1; iIndex < FRIENDAPI_MAX_CALLBACKS; iIndex++)
    {
        if ((pFriendApiRef->common.callback[iIndex].pNotifyCb == pNotifyCb) && (pFriendApiRef->common.callback[iIndex].pNotifyCbUserData == pNotifyCbUserData))
        {
            pFriendApiRef->common.callback[iIndex].pNotifyCb = NULL;
            pFriendApiRef->common.callback[iIndex].pNotifyCbUserData = NULL;

            NetPrintf(("friendapistub: notification callback at index %d unregistered \n", iIndex));

            return(iIndex);
        }
    }
    NetPrintf(("friendapistub: unregistration of notification callback failed\n"));
    return(-1);
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
    \Input *pFriendsCount   - [out] user-provided buffer to be filled with the number of friends in the list

    \Notes
        Set all pointers to NULL except pFriendsCount to find out the number of entries
        that each of the buffers you provide will be filled-in with.

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiGetFriendsList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, FriendDataT *pFriendsList, void *pRawDataList1, int32_t *pFriendsCount)
{
    *pFriendsCount = 0;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function FriendApiGetFriendsListVersion

    \Description
        Returns the current version of friendslist

    \Input *pFriendApiRef   - module reference
    \Input *pVersion        - pointer to version

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiGetFriendsListVersion(FriendApiRefT *pFriendApiRef, uint32_t *pVersion)
{
    *pVersion = 0;

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

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiStatus(FriendApiRefT *pFriendApiRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    NetPrintf(("friendapistub: unhandled status selector ('%c%c%c%c')\n",
            (uint8_t)(iSelect>>24), (uint8_t)(iSelect>>16), (uint8_t)(iSelect>>8), (uint8_t)(iSelect)));

    // unhandled
    return(-1);
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
        int32_t             - selector specific

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiControl(FriendApiRefT *pFriendApiRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    NetPrintf(("friendapistub: unhandled control selector ('%c%c%c%c')\n",
            (uint8_t)(iControl>>24), (uint8_t)(iControl>>16), (uint8_t)(iControl>>8), (uint8_t)(iControl)));

    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function FriendApiDestroy

    \Description
        Destroy the friend api object.

    \Input *pFriendApiRef   - module reference

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiDestroy(FriendApiRefT *pFriendApiRef)
{
    NetPrintf(("friendapistub: the friend api module is being destroyed\n"));

    // release module memory
    DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);
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

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
uint32_t FriendApiIsUserBlocked(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function FriendApiBlockUser

    \Description
        Block user

    \Input *pFriendApiRef   - module reference
    \Input uUserIndex       - local user index
    \Input *pUserId         - user to be blocked
                                on xenon: pUserId needs to point to a XUID
                                on PS3: pUserId needs to point to a SceNpId struct

    \Output
        uint32_t            - 0 for success, negative for failure

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiBlockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    return(-1);
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

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiUnblockUser(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, void *pUserId)
{
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function FriendApiGetBlockListVersion

    \Description
        Returns the current version of the list of blocked users. PS3 only, returns
        0 for other platforms.

    \Input *pFriendApiRef - module reference
    \Input *pVersion      - pointer to version

    \Output
        int32_t             - return negative in case of error, and 0 otherwise

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiGetBlockListVersion(FriendApiRefT *pFriendApiRef, uint32_t *pVersion)
{
    *pVersion = 0;

    return(0);
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

    \Version 06/01/2011 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiGetBlockList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, BlockedUserDataT *pBlockList, int32_t *pUsersCount)
{
    *pUsersCount = 0;

    return(0);
}

