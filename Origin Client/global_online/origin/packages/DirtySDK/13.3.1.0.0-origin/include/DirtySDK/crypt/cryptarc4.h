/*H*************************************************************************************************/
/*!

    \File    cryptarc4.h

    \Description
        This module is a from-scratch ARC4 implementation designed to avoid
        any intellectual property complications. The ARC4 stream cipher is
        known to produce output that is compatible with the RC4 stream cipher.

    \Notes
        This algorithm from this cypher was taken from the web site: ciphersaber.gurus.com
        It is based on the RC4 compatible algorithm that was published in the 2nd ed of
        the book Applied Cryptography by Bruce Schneier. This is a private-key stream
        cipher which means that some other secure way of exchanging cipher keys prior
        to algorithm use is required. Its strength is directly related to the key exchange
        algorithm strengh. In operation, each individual stream message must use a unique
        key. This is handled by appending on 10 byte random value onto the private key.
        This 10-byte data can be sent by public means to the receptor (or included at the
        start of a file or whatever). When the private key is combined with this public
        key, it essentially puts the cypher into a random starting state (it is like
        using a message digest routine to generate a random hash for password comparison).
        The code below was written from scratch using only a textual algorithm description.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2000-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        02/25/2000 (GWS) First Version
    \Version    1.1        11/06/2002 (JLB) Removed Create()/Destroy() to eliminate mem alloc dependencies.

*/
/*************************************************************************************************H*/


#ifndef _cryptarc4_h
#define _cryptarc4_h

/*!
    \Moduledef Crypt Crypt

    \Description
        The Crypt module provides routines for encryption and decryption.

        <b>Overview</b>

        <b>Module Dependency Graph</b>
    
            <img alt="" src="crypt.png">

*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct CryptArc4T CryptArc4T;

//! all fields are PRIVATE
struct CryptArc4T
{
    unsigned char state[256];
    unsigned char walk;
    unsigned char swap;
};

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create state for ARC4 encryption/decryption module.
void CryptArc4Init(CryptArc4T *pState, const unsigned char *pKeyBuf, int32_t iKeyLen, int32_t iIter);

// apply the cipher to the data. calling twice undoes the uncryption
void CryptArc4Apply(CryptArc4T *pState, unsigned char *pBuffer, int32_t iLength);

// advance the cipher state by iLength bytes
void CryptArc4Advance(CryptArc4T *pState, int32_t iLength);

// encrypt an asciiz string, with a 7-bit asciiz string result
void CryptArc4StringEncrypt(char *pDst, int32_t iLen, const char *pSrc, const uint8_t *pKey, int32_t iKey, int32_t iIter);

// decrypt an asciiz string, from a 7-bit asciiz encrypted string
void CryptArc4StringDecrypt(char *pDst, int32_t iLen, const char *pSrc, const uint8_t *pKey, int32_t iKey, int32_t iIter);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptarc4_h

