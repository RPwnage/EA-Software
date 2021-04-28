/*H********************************************************************************/
/*!

    \File    dirtyuserps4.c

    \Description
        Platform-specific user functions.

    \Copyright
        Copyright (c) Electronic Arts 2013.    ALL RIGHTS RESERVED.

    \Version 04/25/13 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include <np/np_npid.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyuser.h"
#include "DirtySDK/util/tagfield.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function DirtyUserToNativeUser

    \Description
        Convert a DirtyUserT to native format.

    \Input *pOutput     - [out] storage for native format user
    \Input iBufLen      - length of output buffer
    \Input *pUser       - source user to convert

    \Output
        uint32_t        - TRUE if successful, else FALSE

    \Notes
        On PS4, a DirtyUserT is a string-encoded network-order sceNpId.
        This function converts that input into a sceNpId.

    \Version 04/25/2013 (mclouatre)
*/
/********************************************************************************F*/
uint32_t DirtyUserToNativeUser(void *pOutput, int32_t iBufLen, const DirtyUserT *pUser)
{
    // make sure output buffer is big enough
    if (iBufLen < (signed)sizeof(SceNpId))
    {
        return(FALSE);
    }

    // decode contents of pUser
    ds_memcpy(pOutput, pUser->strNativeUser, sizeof(SceNpId));

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyUserFromNativeUser

    \Description
        Convert native format user data to a DirtyUserT.

    \Input *pUser       - [out] storage for output DirtyUserT
    \Input *pInput      - pointer to native format user

    \Output
        uint32_t        - TRUE if successful, else FALSE

    \Notes
        On PS4, a DirtyUserT is a string-encoded network-order sceNpId.
        This function converts a sceNpId into a DirtyUsertT.

    \Version 04/25/2013 (mclouatre)
*/
/********************************************************************************F*/
uint32_t DirtyUserFromNativeUser(DirtyUserT *pUser, const void *pInput)
{
    // clear output data
    memset(pUser, 0, sizeof(*pUser));

    // encode input sceNpId into pUser
    ds_memcpy(pUser->strNativeUser, pInput, sizeof(SceNpId));

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function DirtyUserGetLocalUser

    \Description
        Convert a DirtyUserT to native format.

    \Input *pUser       - [out] storage for output DirtyUserT
    \Input *iUserIndex  - local user index

    \Output
        uint32_t        - TRUE if successful, else FALSE

    \Version 04/25/2013 (mclouatre)
*/
/********************************************************************************F*/
uint32_t DirtyUserGetLocalUser(DirtyUserT *pUser, int32_t iUserIndex)
{
    // to be completed
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function DirtyUserCompare

    \Description
        Compare two DirtyUserT users for equality.

    \Input *pUser1 - user 1
    \Input *pUser2 - user 2

    \Output
        int32_t    - TRUE if successful, else FALSE

    \Notes
        On PS4, a DirtyUserT is a string-encoded network-order sceNpId.
        To compare for equality certain portions of the sceNpId need to be 
        ignored. Only the sceOnlineId's data portion is relevant.

    \Version 06/19/2013 (amakoukji)
*/
/********************************************************************************F*/
int32_t DirtyUserCompare(DirtyUserT *pUser1, DirtyUserT *pUser2)
{
    SceNpId SceNpId1;
    SceNpId SceNpId2;
    int32_t iResult = 0;

    DirtyUserToNativeUser(&SceNpId1, (signed)sizeof(SceNpId1), pUser1);
    DirtyUserToNativeUser(&SceNpId2, (signed)sizeof(SceNpId2), pUser2);

    //special handling since SceNpOnlineId's .data field is not always terminated within its char array, but instead in the next .term character field
    iResult = ds_strnicmp(SceNpId1.handle.data, SceNpId2.handle.data, SCE_NP_ONLINEID_MAX_LENGTH + 1);

    if (iResult == 0)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


