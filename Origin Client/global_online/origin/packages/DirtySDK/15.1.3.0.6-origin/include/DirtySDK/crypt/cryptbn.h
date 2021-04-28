/*H*************************************************************************************/
/*!
    \File    cryptbn.h

    \Description
        This module is implements big integer math needed for our cryptography

    \Copyright
        Copyright (c) Electronic Arts 2017.  ALL RIGHTS RESERVED.

    \Version 01/18/2017 (eesponda) First version split from CryptRSA
*/
/*************************************************************************************H*/

#ifndef _cryptbn_h
#define _cryptbn_h

/*!
\Moduledef CryptBn CryptBn
\Modulemember Crypt
*/
//@{

/*** Include files ********************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **************************************************************************/

// size of a each unit in the large number
#if defined(DIRTYCODE_PC) && !defined(_WIN64)
#define UCRYPT_SIZE         (4)
#else
#define UCRYPT_SIZE         (8)
#endif
// bits per word
#define UCRYPT_BITSIZE      (UCRYPT_SIZE*8)
// maximum width of number
#define CRYPTBN_MAX_WIDTH   ((4096/UCRYPT_BITSIZE)+1)

/*** Type Definitions *****************************************************************/

// define crypt types based on UCRYPT_SIZE
#if (UCRYPT_SIZE == 4)
typedef uint32_t ucrypt_t;
#else
typedef uint64_t ucrypt_t;
#endif

//! big number state
typedef struct CryptBnT
{
    ucrypt_t aData[CRYPTBN_MAX_WIDTH];
    int32_t iWidth;
    uint32_t uSign;
} CryptBnT;

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init big number state at specific width
DIRTYCODE_API void CryptBnInit(CryptBnT *pState, int32_t iWidth);

// init big number state with an unsigned number
DIRTYCODE_API void CryptBnInitSet(CryptBnT *pState, uint32_t uValue);

// init big number state from a buffer
DIRTYCODE_API int32_t CryptBnInitFrom(CryptBnT *pState, int32_t iWidth, const uint8_t *pSource, int32_t iLength);

// left shift the big number by a bit
DIRTYCODE_API void CryptBnLeftShift(CryptBnT *pState);

// tests a bit in the big number and returns if it is set
DIRTYCODE_API uint8_t CryptBnBitTest(const CryptBnT *pState, int32_t iBit);

// set a bit in the big number
DIRTYCODE_API void CryptBnBitSet(CryptBnT *pState, int32_t iBit);

// returns bit length
DIRTYCODE_API int32_t CryptBnBitLen(const CryptBnT *pState);

// accumulate one big number into another
DIRTYCODE_API void CryptBnAccumulate(CryptBnT *pState, const CryptBnT *pAdd);

// increment the number by 1
DIRTYCODE_API void CryptBnIncrement(CryptBnT *pState);

// subtract two big numbers
DIRTYCODE_API void CryptBnSubtract(CryptBnT *pState, const CryptBnT *pSub1, const CryptBnT *pSub2);

// subtract the number by 1
DIRTYCODE_API void CryptBnDecrement(CryptBnT *pState);

// perform modular multiply operation
DIRTYCODE_API void CryptBnModMultiply(CryptBnT *pState, const  CryptBnT *pMul1, const CryptBnT *pMul2, const CryptBnT *pMod);

// multiplication using classical algorithm
DIRTYCODE_API void CryptBnMultiply(CryptBnT *pState, const CryptBnT *pMul1, const CryptBnT *pMul2);

// does a modulus/divide operation using the classical method
DIRTYCODE_API void CryptBnMod(const CryptBnT *pDividend, const CryptBnT *pDivisor, CryptBnT *pRemainder, CryptBnT *pQuotient);

// perform inverse modulo operation
DIRTYCODE_API void CryptBnInverseMod(CryptBnT *pState, const CryptBnT *pMod);

// copy the big number data into output buffer
DIRTYCODE_API void CryptBnFinal(const CryptBnT *pState, uint8_t *pResult, int32_t iLength);

// copy a big number
DIRTYCODE_API void CryptBnClone(CryptBnT *pDst, const CryptBnT *pSrc);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptbn_h
