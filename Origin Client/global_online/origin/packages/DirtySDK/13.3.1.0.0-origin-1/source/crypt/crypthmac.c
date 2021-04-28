/*H********************************************************************************/
/*!
    \File crypthmac.c

    \Description
        Implements HMAC as defined in http://tools.ietf.org/html/rfc4346#section-5

    \Copyright
        Copyright (c) 2013 Electronic Arts Inc.

    \Version 01/14/2013 (jbrookes) First Version; refactored from ProtoSSL
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptmd5.h"
#include "DirtySDK/crypt/cryptsha1.h"

#include "DirtySDK/crypt/crypthmac.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptHmacCalcMulti

    \Description
        Implements HMAC as defined in http://tools.ietf.org/html/rfc4346#section-5

    \Input *pBuffer     - [out] storage for generated digest
    \Input iBufLen      - size of output buffer (must be large enough to hold digest)
    \Input *pMessage    - input data to hash
    \Input iMessageLen  - size of input data
    \Input *pKey        - seed
    \Input iKeyLen      - seed length
    \Input iHashSize    - hash size (used to determine MD5/SHA)

    \Output
        int32_t         - negative=error, else success

    \Version 01/14/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptHmacCalc(uint8_t *pBuffer, int32_t iBufLen, const uint8_t *pMessage, int32_t iMessageLen, const uint8_t *pKey, int32_t iKeyLen, int32_t iHashSize)
{
    CryptHmacMsgT Message;
    Message.pMessage = pMessage;
    Message.iMessageLen = iMessageLen;
    return(CryptHmacCalcMulti(pBuffer, iBufLen, &Message, 1, pKey, iKeyLen, iHashSize));
}

/*F********************************************************************************/
/*!
    \Function CryptHmacCalcMulti

    \Description
        Implements HMAC as defined in http://tools.ietf.org/html/rfc4346#section-5.
        This version allows multiple buffers to be specified in one call, which is
        useful when the input to be hashed is not in a single contiguous buffer.

    \Input *pBuffer     - [out] storage for generated MAC
    \Input iBufLen      - size of output buffer (must be large enough to hold digest)
    \Input *pMessageList- list of messages to hash
    \Input iNumMessages - number of messages to hash
    \Input *pKey        - seed
    \Input iKeyLen      - seed length
    \Input iHashSize    - hash size (used to determine MD5/SHA)

    \Output
        int32_t         - negative=error, else success

    \Version 01/14/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t CryptHmacCalcMulti(uint8_t *pBuffer, int32_t iBufLen, const CryptHmacMsgT *pMessageList, int32_t iNumMessages, const uint8_t *pKey, int32_t iKeyLen, int32_t iHashSize)
{
    uint8_t aKiPad[64], aKoPad[64];
    int32_t iBufCnt, iMessage;

    // copy key to ipad and opad, pad with zero
    memcpy(aKiPad, pKey, iKeyLen);
    memset(aKiPad+iKeyLen, 0, sizeof(aKiPad)-iKeyLen);
    memcpy(aKoPad, pKey, iKeyLen);
    memset(aKoPad+iKeyLen, 0, sizeof(aKoPad)-iKeyLen);

    // calculate xor of ipad and opad with 0x36/0x5c respectively
    for (iBufCnt = 0; iBufCnt < (signed)sizeof(aKiPad); iBufCnt += 1)
    {
        aKiPad[iBufCnt] ^= 0x36;
        aKoPad[iBufCnt] ^= 0x5c;
    }

    // do MD5 hash?
    if (iHashSize == CRYPTHMAC_MD5)
    {
        CryptMD5T MD5Context;
        // execute the MD5 hash of ipad
        CryptMD5Init(&MD5Context);
        CryptMD5Update(&MD5Context, aKiPad, sizeof(aKiPad));
        for (iMessage = 0; iMessage < iNumMessages; iMessage += 1)
        {
            CryptMD5Update(&MD5Context, pMessageList[iMessage].pMessage, pMessageList[iMessage].iMessageLen);
        }
        CryptMD5Final(&MD5Context, pBuffer, iHashSize);
        // execute the MD5 hash of ipad+opad
        CryptMD5Init(&MD5Context);
        CryptMD5Update(&MD5Context, aKoPad, sizeof(aKoPad));
        CryptMD5Update(&MD5Context, pBuffer, iHashSize);
        CryptMD5Final(&MD5Context, pBuffer, iHashSize);
    }

    // do SHA1 hash?
    if (iHashSize == CRYPTHMAC_SHA)
    {
        CryptSha1T SHA1Context;
        // execute the SHA1 hash of ipad
        CryptSha1Init(&SHA1Context);
        CryptSha1Update(&SHA1Context, aKiPad, sizeof(aKiPad));
        for (iMessage = 0; iMessage < iNumMessages; iMessage += 1)
        {
            CryptSha1Update(&SHA1Context, pMessageList[iMessage].pMessage, pMessageList[iMessage].iMessageLen);
        }
        CryptSha1Final(&SHA1Context, pBuffer, iHashSize);
        // execute the SHA1 hash of ipad+opad
        CryptSha1Init(&SHA1Context);
        CryptSha1Update(&SHA1Context, aKoPad, sizeof(aKoPad));
        CryptSha1Update(&SHA1Context, pBuffer, iHashSize);
        CryptSha1Final(&SHA1Context, pBuffer, iHashSize);
    }

    // success
    return(0);
}
