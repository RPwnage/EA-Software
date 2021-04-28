/*H********************************************************************************/
/*!
    \File   crypted.h

    \Description
        This module implements the math for elliptic curve cryptography
        using edwards curves

    \Copyright
        Copyright (c) Electronic Arts 2018. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _crypted_h
#define _crypted_h

/*!
\Moduledef CryptEd CryptEd
\Modulemember Crypt
*/
//@{

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptbn.h"
#include "DirtySDK/crypt/cryptdef.h"

/*** Type Definitions **************************************************************/

//! module state for the eddsa operation
typedef struct CryptEdT
{
    uint32_t uCryptUsecs;   //!< cached timing result
} CryptEdT;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init the curve state
DIRTYCODE_API int32_t CryptEdInit(CryptEdT *pState, int32_t iCurveType);

// cleanup the curve state
DIRTYCODE_API void CryptEdCleanup(CryptEdT *pState);

// generate an ecdsa signature
DIRTYCODE_API int32_t CryptEdSign(CryptEdT *pState, const CryptBnT *pPrivateKey, const uint8_t *pMessage, int32_t iMessageSize, CryptEccPointT *pSignature, uint32_t *pCryptUsecs);

// verify an ecdsa signature
DIRTYCODE_API int32_t CryptEdVerify(CryptEdT *pState, const CryptEccPointT *pPublicKey, const uint8_t *pMessage, int32_t iMessageSize, const CryptEccPointT *pSignature, uint32_t *pCryptUsecs);

// initialize a point on the curve from a buffer
DIRTYCODE_API int32_t CryptEdPointInitFrom(CryptEccPointT *pPoint, const uint8_t *pBuffer, int32_t iBufSize);

#ifdef __cplusplus
}
#endif

//@}

#endif // _crypted_h

