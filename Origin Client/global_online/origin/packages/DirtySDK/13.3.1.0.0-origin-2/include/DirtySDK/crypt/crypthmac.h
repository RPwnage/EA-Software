/*H********************************************************************************/
/*!
    \File crypthmac.h

    \Description

    \Notes
        References:

    \Copyright
        Copyright (c) 2013 Electronic Arts Inc.

    \Version 01/14/2013 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _crypthmac_h
#define _crypthmac_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define CRYPTHMAC_MD5 (16) //!< hash size for MD5
#define CRYPTHMAC_SHA (20) //!< hash size for SHA1

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct CryptHmacMsgT
{
    const uint8_t *pMessage;
    int32_t iMessageLen;
} CryptHmacMsgT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// calculate HMAC message digest algorithm
int32_t CryptHmacCalc(uint8_t *pBuffer, int32_t iBufLen, const uint8_t *pMessage, int32_t iMessageLen, const uint8_t *pKey, int32_t iKeyLen, int32_t iHashSize);

// calculate HMAC message digest algorithm; this version allows multiple buffers to allow for easy hashing of sparse messages
int32_t CryptHmacCalcMulti(uint8_t *pBuffer, int32_t iBufLen, const CryptHmacMsgT *pMessageList, int32_t iNumMessages, const uint8_t *pKey, int32_t iKeyLen, int32_t iHashSize);

#ifdef __cplusplus
}
#endif

#endif
