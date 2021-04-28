/*H********************************************************************************/
/*!
    \File voipprivxboxone.c

    \Description
        Private functions for VoIP module use.

    \Copyright
        Copyright (c) Electronic Arts 2013. ALL RIGHTS RESERVED.

    \Version 1.0 07/12/2013 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include "DirtySDK/platform.h"
#include "voippriv.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function   VoipXuidFromUser

    \Description
        Convert "machine address" (textual form of XUID) into an actual XUID.

    \Input *pXuid           - [out] storage for decoded XUID
    \Input *pVoipUser       - source textual XUID to convert to a binary XUID

    \Version 05/27/2005 (jbrookes) First Version
*/
/********************************************************************************F*/
void VoipXuidFromUser(uint64_t *pXuid, VoipUserT *pVoipUser)
{
    const char *pAddrText;
    char cNum;
    int32_t iChar;
    uint64_t *pXuidData = (uint64_t *)pXuid;

    // convert from string to qword
    for (iChar = 0, *pXuidData = 0, pAddrText = pVoipUser->strUserId + 1; iChar < 16; iChar++)
    {
        // make room for next entry
        *pXuidData <<= 4;

        // add in character
        cNum = pAddrText[iChar];
        cNum -= ((cNum >= 0x30) && (cNum < 0x3a)) ? '0' : 0x57;
        *pXuidData |= (uint64_t)cNum;
    }
}
