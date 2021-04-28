/*H*************************************************************************************************/
/*!

    \File    cryptrsa.h

    \Description
        This module is a from-scratch RSA implementation in order to avoid
        any intellectual property issues. The 1024 bit RSA public key 
        encryption algorithm was implemented from a specification provided
        by Netscape for SSL implementation (see protossl.h).
        
    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        03/08/2002 (GWS) First Version (protossl)
    \Version    1.1        11/18/2002 (JLB) Names changed to include "Proto"
    \Version    1.2        03/05/2003 (JLB) Split RSA encryption from protossl

*/
/*************************************************************************************************H*/

#ifndef _cryptrsa_h
#define _cryptrsa_h

/*!
\Module Crypt
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct CryptRSARandomT CryptRSARandomT;

//! the random number generator state.
struct CryptRSARandomT
{
    uint32_t u0;
    uint32_t u1;
};


typedef struct CryptRSAT CryptRSAT;

//! all fields are PRIVATE
struct CryptRSAT
{
    int32_t iKeyModSize;            //!< size of public key modulus
    int32_t iKeyExpSize;            //!< size of public key exponent
    uint8_t EncryptBlock[1024];
    uint8_t KeyModData[512+1];      //!< public key modulus
    uint8_t KeyExpData[512+1];      //!< public key exponent
    CryptRSARandomT Random;
};

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

int32_t CryptRSAInit(CryptRSAT *pState, const unsigned char *pModulus, int32_t iModSize,
                     const unsigned char *pExponent, int32_t iExpSize);

// Setup the master shared secret for encryption
void CryptRSAInitMaster(CryptRSAT *pState, const unsigned char *pMaster, int32_t iMasterLen);

// Setup the encrypted signature for decryption.
void CryptRSAInitSignature(CryptRSAT *pState, const unsigned char *pSig, int32_t iSigLen);


void CryptRSAEncrypt(CryptRSAT *pState);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptrsa_h

