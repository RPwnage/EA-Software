/*H*************************************************************************************/
/*!
    \File    cryptrsa.h

    \Description
        This module is a from-scratch RSA implementation in order to avoid any
        intellectual property issues. The 1024 bit RSA public key encryption algorithm
        was implemented from a specification provided by Netscape for SSL implementation
        (see protossl.h).

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version 03/08/2002 (gschaefer) First Version (protossl)
    \Version 11/18/2002 (jbrookes)  Names changed to include "Proto"
    \Version 03/05/2003 (jbrookes)  Split RSA encryption from protossl
*/
/*************************************************************************************H*/

#ifndef _cryptrsa_h
#define _cryptrsa_h

/*!
\Moduledef CryptRSA CryptRSA
\Modulemember Crypt
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptbn.h"
#include "DirtySDK/crypt/cryptdef.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! cryptrsa state
typedef struct CryptRSAT
{
    // input state
    int32_t iKeyModSize;            //!< size of public key modulus
    int32_t iKeyExpSize;            //!< size of public key exponent
    uint8_t EncryptBlock[1024];     //!< input data
    uint8_t KeyModData[512+1];      //!< public key modulus
    uint8_t KeyExpData[512+1];      //!< public key exponent

    // exponentiation state
    int32_t iExpBitIndex;
    int32_t iExpByteIndex;
    uint32_t uExponent;
    uint32_t bAccumulOne;

    // rsa profiling info
    uint32_t uCryptMsecs;
    uint32_t uCryptUsecs;
    uint32_t uNumExpCalls;

    // working memory for modular exponentiation
    CryptBnT Modulus;
    CryptBnT Powerof;
    CryptBnT Accumul;
} CryptRSAT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init rsa state
DIRTYCODE_API int32_t CryptRSAInit(CryptRSAT *pState, const uint8_t *pModulus, int32_t iModSize, const uint8_t *pExponent, int32_t iExpSize);

// setup the master shared secret for encryption
DIRTYCODE_API void CryptRSAInitMaster(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen);

// setup the master shared secret for encryption using PKCS1.5
DIRTYCODE_API void CryptRSAInitPrivate(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen);

// setup the encrypted signature for decryption.
DIRTYCODE_API void CryptRSAInitSignature(CryptRSAT *pState, const uint8_t *pSig, int32_t iSigLen);

// do the encryption/decryption
DIRTYCODE_API int32_t CryptRSAEncrypt(CryptRSAT *pState, int32_t iIter);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptrsa_h

