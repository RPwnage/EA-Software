/*H********************************************************************************/
/*!

    \File    dirtyuserxboxone.c

    \Description
        Platform-specific user functions.

    \Copyright
        Copyright (c) Electronic Arts 2013.    ALL RIGHTS RESERVED.

    \Version 05/02/13 (amakoukji) First Version from dirtyuserps4
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyuser.h"
#include "DirtySDK/util/tagfield.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _DirtyUserSetBinary

    \Description
        Wrapper to use tagfield to set binary or binary7 data

    \Input *pBuf        - [out] output buffer
    \Input iBufSize     - size of output buffer
    \Input *pInput      - input data to encode
    \Input uInputSize   - size of input data
    \Input bBinary7     - true=use binary7 encoding

    \Output
        int32_t         - length of encoded string data

    \Version 04/25/2013 (mclouatre)
*/
/********************************************************************************F*/
static int32_t _DirtyUserSetBinary(char *pBuf, int32_t iBufSize, const uint8_t *pInput, uint32_t uInputSize, uint32_t bBinary7)
{
    char strTemp[128];
    TagFieldClear(strTemp, sizeof(strTemp));
    if (bBinary7)
    {
        TagFieldSetBinary7(strTemp, sizeof(strTemp), "T", pInput, uInputSize);
    }
    else
    {
        TagFieldSetBinary(strTemp, sizeof(strTemp), "T", pInput, uInputSize);
    }
    TagFieldGetString(TagFieldFind(strTemp, "T"), pBuf, iBufSize, "");
    return((signed)strlen(pBuf));
}

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
        On Xbox One, a DirtyUserT is a string-encoded XUID, which 
        is a 64-bit unsigned integer.
        This function converts that input into an XUID.

    \Version 04/25/2013 (mclouatre)
*/
/********************************************************************************F*/
uint32_t DirtyUserToNativeUser(void *pOutput, int32_t iBufLen, const DirtyUserT *pUser)
{
    // make sure output buffer is big enough
    if (iBufLen < (signed)sizeof(uint64_t))
    {
        return(FALSE);
    }

    // decode contents of pUser
    TagFieldGetBinary(pUser->strNativeUser, (uint64_t *)pOutput, sizeof(uint64_t));

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
        On Xbox One, a DirtyUserT is a string-encoded XUID, which 
        is a 64-bit unsigned integer.
        This function converts an XUID into a DirtyUsertT.

    \Version 04/25/2013 (mclouatre)
*/
/********************************************************************************F*/
uint32_t DirtyUserFromNativeUser(DirtyUserT *pUser, const void *pInput)
{
    uint64_t *pXuId = (uint64_t *)pInput;

    // clear output data
    memset(pUser, 0, sizeof(*pUser));

    // encode input xuid into pUser
    _DirtyUserSetBinary(pUser->strNativeUser, sizeof(*pUser), (uint8_t *)pXuId, sizeof(uint64_t), TRUE);

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

    \Version 06/19/2013 (amakoukji)
*/
/********************************************************************************F*/
int32_t DirtyUserCompare(DirtyUserT *pUser1, DirtyUserT *pUser2)
{
    int32_t iResult = strcmp((pUser1)->strNativeUser, (pUser2)->strNativeUser);

    if (iResult == 0)
    {
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}


