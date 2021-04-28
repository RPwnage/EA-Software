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

#include "dirtysock.h"
#include "dirtymem.h"
#include "dirtyerr.h"
#include "netconn.h"

#include "friendapi.h"
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

    // save parms
    pFriendApiRef->common.iMemGroup = iMemGroup;
    pFriendApiRef->common.pMemGroupUserData = pMemGroupUserData;

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
// $$todo v9 all friends functionality in XnFriends and NetConnXenon should be moved under the FriendApi
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
    \Input *pFriendsCount   - [out] user-provided buffer to be filled with the number of friends in the list

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiPrivGetFriendsList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, FriendDataT *pFriendsList, void *pRawDataList1, int32_t *pFriendsCount)
{
    int32_t iIndex;

    *pFriendsCount = NetConnStatus('frnd', uUserIndex, NULL, 0);

    // $todo v9   there should be no need to provide both of these buffers simultaneously
    //            once the xnfriends module is moved under the FriendApi module.
    if ( ((pRawDataList1 == NULL) && (pFriendsList != NULL)) ||
         ((pRawDataList1 != NULL) && (pFriendsList == NULL)) )
    {
        NetPrintf(("friendapiprivxenon: v8-limitation for xenon flavor of FriendApiPrivGetFriendsList() - both pFriendsList and pRawDataList1 need to be valid\n"));
        return;
    }

    // convert the xenon friend data into FriendApi generic friend data
    if ((pFriendsList != NULL) && (pRawDataList1 != NULL))
    {
        XONLINE_FRIEND *pXOnlineFriend = (XONLINE_FRIEND *)pRawDataList1;

        NetConnStatus('frnd', uUserIndex, pRawDataList1, *pFriendsCount * sizeof(XONLINE_FRIEND));

        for (iIndex = 0; iIndex < *pFriendsCount; iIndex++)
        {
            snzprintf(pFriendsList[iIndex].strFriendId, sizeof(pFriendsList[iIndex].strFriendId), "%I64d", pXOnlineFriend->xuid);
            strncpy(pFriendsList[iIndex].strFriendName, pXOnlineFriend->szGamertag, sizeof(pFriendsList[iIndex].strFriendName));
            pFriendsList[iIndex].pRawData1 = pXOnlineFriend;
            pXOnlineFriend++;
        }
    }
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
        \endverbatim

    \Version 05/19/2010 (mclouatre)
*/
/********************************************************************************F*/
int32_t FriendApiPrivControl(FriendApiRefT *pFriendApiRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    NetPrintf(("friendapiprivxenon: unhandled control selector ('%c%c%c%c')\n",
            (uint8_t)(iControl>>24), (uint8_t)(iControl>>16), (uint8_t)(iControl>>8), (uint8_t)(iControl)));

    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function FriendApiPrivDestroy

    \Description
        Destroy the xenon friend api object.

    \Input *pFriendApiRef   - module reference

    \Version 04/19/2010 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiPrivDestroy(FriendApiRefT *pFriendApiRef)
{
    NetPrintf(("friendapiprivxenon: the friend api module is being destroyed\n"));

    // release module memory
    DirtyMemFree(pFriendApiRef, FRIENDAPI_MEMID, pFriendApiRef->common.iMemGroup, pFriendApiRef->common.pMemGroupUserData);
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
        Block user

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
    XUID * pXuid = (XUID *) pUserId;
    DWORD dwResult = 0;

    if ((dwResult = XUserMuteListSetState(uUserIndex, *pXuid, TRUE)) != ERROR_SUCCESS)
    {
        NetPrintf(("friendapiprivxenon: XUserMuteListSetState(TRUE) failed with %s\n", DirtyErrGetName(dwResult)));
        return(-1);
    }

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

    \Output
        uint32_t            - 0

    \Version 05/10/2011 (mclouatre)
*/
/********************************************************************************F*/
uint32_t FriendApiPrivGetBlockListVersion(FriendApiRefT *pFriendApiRef)
{
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

    \Notes
        Set pBlockList to NULL to find out the number of entries that pBlockList will be
        filled-in with.

    \Version 05/10/2011 (mclouatre)
*/
/********************************************************************************F*/
void FriendApiPrivGetBlockList(FriendApiRefT *pFriendApiRef, uint32_t uUserIndex, BlockedUserDataT *pBlockList, int32_t *pUsersCount)
{
    *pUsersCount = 0;
}
