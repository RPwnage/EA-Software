/*H*************************************************************************************************/
/*!

    \File    cryptssc2.h

    \Description
        Simple stream cipher version 2 (SSC2) designed to provide reasonable
        data encryption. This algorithm was designed from scratch by GWS and
        is based on the general methods I have seen used in other stream
        ciphers. As a result, it has obviously not been cryptoanalyzed by
        anyone so problems may exist (and in fact I found a major data leak in
        version 1). However, for data obscuring and things of that nature, this
        should work fine. Since it was designed from scratch it should be free
        from intellectual property issues.

        In order to use a stream cipher like this, the initial key sequence used
        to initialize it must be unique for every usage. This is typically done
        by appending on some random bytes (which are also appended on by the
        decoder). The fact that the random bytes are transmitted as cleartext
        does not matter. If the key is not made unique with random bytes, then
        the data becomes relatively easy to decode (at least by crypto standards).

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        07/23/2001 (GWS) First Version
    \Version    2.0        03/15/2002 (GWS) Code cleanup plus random trailers
    \Version    2.1        11/06/2002 (JLB) Removed Create()/Destroy() to eliminate mem alloc dependencies.

*/
/*************************************************************************************************H*/


#ifndef _cryptssc2_h
#define _cryptssc2_h

/*!
\Module Crypt
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

#define CRYPTSSC2_TEST (/*DIRTYCODE_DEBUG && */ 1) // Changing this from TRUE to 1 since qt defines TRUE as true and VS freaks out

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct CryptSSC2T CryptSSC2T;

//! all fields are PRIVATE
struct CryptSSC2T
{
    unsigned char sbox[256];
    unsigned char idx;
    uint32_t crc;
};

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init the cipher for use based on key/iter count
void CryptSSC2Init(CryptSSC2T *pState, const unsigned char *pKey, int32_t iKey, int32_t iIter);

// apply the cipher to the target data
void CryptSSC2Apply(CryptSSC2T *pState, unsigned char *pData, int32_t iData);

// easy way to encode an asciiz string
int32_t CryptSSC2StringEncrypt(char *pDst, int32_t iLen, const char *pSrc, const unsigned char *pKey, int32_t iKey, int32_t iIter);

// easy way to encode an asciiz string
int32_t CryptSSC2StringDecrypt(char *pDst, int32_t iLen, const char *pSrc, const unsigned char *pKey, int32_t iKey, int32_t iIter);

// simple test code to make sure encrypt/decrypt product
#if CRYPTSSC2_TEST
void CryptSSC2Test(void);
#endif

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptssc2_h

