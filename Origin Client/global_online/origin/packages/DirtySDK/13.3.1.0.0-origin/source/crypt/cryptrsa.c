/*H********************************************************************************/
/*!
    \File cryptrsa.c

    \Description
        This module is a from-scratch RSA implementation in order to avoid
        any intellectual property issues. The 1024 bit RSA public key
        encryption algorithm was implemented from a specification provided
        by Netscape for SSL implementation (see protossl.h).

    \Copyright
        Copyright (c) Electronic Arts 2002-2011.

    \Todo
        64bit word support (UCRYPT_SIZE=8) has been tested to work with the unix64-gcc
        (Linux) and ps3-gcc (PS3) compilers; however the current implementation does
        not work with certain odd modulus sizes (for example the 1000-bit modulus of
        the built-in RSA CA certificate), so it is not currently enabled.

    \Version 1.0 03/08/2002 (gschaefer) First Version (protossl)
    \Version 1.1 03/05/2003 (jbrookes)  Split RSA encryption from protossl
    \Version 1.2 11/16/2011 (jbrookes)  Optimizations to improve dbg&opt performance
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <string.h>             // memset

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/crypt/cryptrsa.h"

/*** Defines **********************************************************************/

// define UCRYPT_SIZE; most platforms will use 4 (32bit words).  valid options are 2, 4, and 8.
#if defined(DIRTYCODE_LINUX)
#define UCRYPT_SIZE     (4) //$$todo - fix this so we can bump it to 8
#else
#define UCRYPT_SIZE     (4)
#endif

// validate size, define additional constants
#if UCRYPT_SIZE == 2
#define UCRYPT_BITSHIFT 4
#elif UCRYPT_SIZE == 4
#define UCRYPT_BITSHIFT 5
#elif UCRYPT_SIZE == 8
#define UCRYPT_BITSHIFT 6
#else
#error Unsupported crypt size selected!
#endif

// bits per word
#define UCRYPT_BITSIZE  (UCRYPT_SIZE*8)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

#if (UCRYPT_SIZE == 2)
typedef uint16_t ucrypt_t;
typedef uint32_t ucrypt2_t;
#elif (UCRYPT_SIZE == 4)
typedef uint32_t ucrypt_t;
typedef uint64_t ucrypt2_t;
#elif (UCRYPT_SIZE == 8)
typedef uint64_t ucrypt_t;
typedef __uint128_t ucrypt2_t;
#endif

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables


// Public variables


/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _Accumulate

    \Description
        Accumulate one large number into another.

    \Input *pAccum  - pointer to large number accumulation buffer
    \Input iWidth   - array width of both numbers, in ucrypt_t units
    \Input *pAdd    - pointer to large number to accumulate to accumulation buffer

    \Output
        ucrypt_t    - carry flag

    \Notes
        pAccum:iWidth <-- pAccum:iWidth + pAdd:iWidth

    \Version 11/16/2011 (jbrookes)
*/
/********************************************************************************H*/
static ucrypt_t _Accumulate(ucrypt_t *pAccum, int32_t iWidth, ucrypt_t *pAdd)
{
    register ucrypt2_t uAccum;

    // point to lsw of data
    pAdd += iWidth;
    pAccum += iWidth;

    // do the add
    for (uAccum = 0; iWidth > 0; --iWidth)
    {
        uAccum += ((ucrypt2_t)*--pAccum + (ucrypt2_t)*--pAdd);
        *pAccum = (ucrypt_t)uAccum;
        uAccum >>= UCRYPT_BITSIZE;
    }

    // return carry flag
    return(uAccum);
}

/*F*************************************************************************************************/
/*!
    \Function _LeftShiftBy1

    \Description
        Left shifts a large number by 1

    \Input *pIO     - pointer to input and output large number array
    \Input iWidth   - array width, in shorts

    \Output
        ucrypt_t    - carry flag

    \Notes
        pIO:iWidth <-- pIO:iWidth << 1

    \Version 10/11/2011 (mdonais)
 */
/*************************************************************************************************F*/
static ucrypt_t _LeftShiftBy1(ucrypt_t *pIO, int32_t iWidth)
{
    register ucrypt2_t uAccum, uVal;

    // point to lsw of data
    pIO += iWidth;

    // do the shift
    for (uAccum = 0; iWidth > 0; --iWidth)
    {
        uVal = *--pIO;
        *pIO = (uVal << 1) + uAccum;
        uAccum = uVal >> (UCRYPT_BITSIZE-1);
    }

    // return carry flag
    return(uAccum);
}

/*F********************************************************************************/
/*!
    \Function _Subtract

    \Description
        Compare/subtract two lage numbers

    \Input *pResult - pointer to output large number array
    \Input iWidth   - array width of both numbers, in ucrypt_t units
    \Input *pSub1   - pointer to large number to subtract pSub2 from.
    \Input *pSub2   - pointer to second large number

    \Output
        ucrypt_t    - carry flag

    \Notes
        pResult:iWidth <-- pSub1:iWidth - pSub2:iWidth

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************H*/
static ucrypt_t _Subtract(ucrypt_t *pResult, int32_t iWidth, ucrypt_t *pSub1, ucrypt_t *pSub2)
{
    register ucrypt2_t uAccum;

    // point to lsw of data
    pSub1 += iWidth;
    pSub2 += iWidth;
    pResult += iWidth;

    // do the subtact
    for (uAccum = 0; iWidth > 0; --iWidth)
    {
        uAccum = (((ucrypt2_t)*--pSub1 - (ucrypt2_t)*--pSub2) - uAccum);
        *--pResult = (ucrypt_t)uAccum;
        uAccum = (uAccum >> UCRYPT_BITSIZE) & 1;
    }

    // return carry flag
    return(uAccum);
}

/*F********************************************************************************/
/*!
    \Function _BitTest

    \Description
        Check to see if a particular bit is set within a large bit vector

    \Input *pValue  - pointer to input bit vector
    \Input iWidth   - width of vector in ucrypt_t units
    \Input iBit     - bit number to test (zero-relative)

    \Output
        int32_t     - non-zero if set, else zero

    \Notes
        uResult <-- pValue:iWidth & (1 << iBit)

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************H*/
static ucrypt_t _BitTest(ucrypt_t *pValue, int32_t iWidth, int32_t iBit)
{
    int32_t iBitOff = iBit & (UCRYPT_BITSIZE-1);
    int32_t iOffset = ((iWidth << UCRYPT_BITSHIFT) - iBit - 1) >> UCRYPT_BITSHIFT;
    ucrypt_t uResult = (ucrypt_t)(pValue[iOffset] & ((ucrypt_t)1 << iBitOff));
    return(uResult);
}

/*F********************************************************************************/
/*!
    \Function _Multiply

    \Description
        Modular multiple

    \Input *pResult - pointer to output large number array
    \Input iWidth   - array width of both numbers, in ucrypt_t units
    \Input *pMul1   - pointer to large number to multiply with pMul2
    \Input *pMul2   - pointer to second large number
    \Input *pMod    - pointer to modulous large number

    \Notes
        pResult:iWidth <-- pMul1:iWidth * pMul2:iWidth % pMod:iWidth

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************H*/
static void _Multiply(ucrypt_t *pResult, int32_t iWidth, ucrypt_t *pMul1, ucrypt_t *pMul2, ucrypt_t *pMod)
{
    int32_t iCount;
    int32_t iCarry;
    ucrypt_t Temp1[4096/UCRYPT_BITSIZE];
    ucrypt_t Temp2[4096/UCRYPT_BITSIZE];
    ucrypt_t *pCur = Temp1;
    ucrypt_t *pAlt = Temp2;

    // clear the current result field
    memset(pCur, 0, iWidth * sizeof(*pResult));

    // do all the bits
    for (iCount = (iWidth*UCRYPT_BITSIZE)-1; iCount >= 0; --iCount)
    {
        // left shift the result
        iCarry = _LeftShiftBy1(pCur, iWidth);

        // do modulus reduction?
        if (iCarry || (*pCur >= *pMod))
        {
            ucrypt_t *pSwap = pCur;
            _Subtract(pAlt, iWidth, pCur, pMod);
            pCur = pAlt;
            pAlt = pSwap;
        }

        // see if we need to add in mutiplicand
        if (_BitTest(pMul1, iWidth, iCount))
        {
            // add it in
            iCarry = _Accumulate(pCur, iWidth, pMul2);

            // do modulus reduction?
            if (iCarry || (*pCur >= *pMod))
            {
                ucrypt_t *pSwap = pCur;
                _Subtract(pAlt, iWidth, pCur, pMod);
                pCur = pAlt;
                pAlt = pSwap;
            }
        }
    }

    // copy over the result
    memcpy(pResult, pCur, iWidth * sizeof(*pResult));
}

/*F********************************************************************************/
/*!
    \Function _ToWords

    \Description
        Convert from msb bytes to int32_t number format

    \Input *pResult - pointer to output large number array of ucrypt_t units
    \Input iWidth   - width of output - if negative, auto-calculate width
    \Input *pSource - pointer to input large number array of bytes
    \Input iLength  - length of input in bytes

    \Output
        int32_t     - width in ucrypt_t units

    \Version 03/08/2002 (gschaefer)
    \Version 03/03/2004 (sbevan) Handle odd iLength
*/
/********************************************************************************H*/
static int32_t _ToWords(ucrypt_t *pResult, int32_t iWidth, const uint8_t *pSource, int32_t iLength)
{
    int32_t iFullWordCount = iLength/sizeof(*pResult);
    int32_t iWordCount = (iLength+sizeof(*pResult)-1)/sizeof(*pResult);

    if (iWidth < 0)
    {
        iWidth = iWordCount;
    }
    // do leading zero fill
    for (; iWidth > iWordCount; --iWidth)
    {
        *pResult++ = 0;
    }
    if (iFullWordCount != iWordCount)
    {
        *pResult++ = pSource[0];
        pSource += 1;
        iWidth -= 1;
    }
    for (; iWidth > 0; --iWidth)
    {
        #if (UCRYPT_SIZE == 2)
        *pResult++ = (pSource[0]<<8)|(pSource[1]<<0);
        #elif (UCRYPT_SIZE == 4)
        *pResult++ = (pSource[0]<<24)|(pSource[1]<<16)|(pSource[2]<<8)|(pSource[3]<<0);
        #elif (UCRYPT_SIZE == 8)
        *pResult++ = ((ucrypt_t)pSource[0]<<56)|((ucrypt_t)pSource[1]<<48)|((ucrypt_t)pSource[2]<<40)|((ucrypt_t)pSource[3]<<32)|
                     ((ucrypt_t)pSource[4]<<24)|((ucrypt_t)pSource[5]<<16)|((ucrypt_t)pSource[6]<<8)|((ucrypt_t)pSource[7]<<0);
        #endif
        pSource += UCRYPT_SIZE;
    }
    return(iWordCount);
}

/*F********************************************************************************/
/*!
    \Function _FromWords

    \Description
        Convert from ucrypt_t units back to big-endian bytes

    \Input *pSource - the ucrypt_t units to convert
    \Input iWidth   - the number of units in the source.
    \Input *pResult - where to write the 8-bit result.
    \Input iLength  - size of the result array in bytes.

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************H*/
static void _FromWords(const ucrypt_t *pSource, int32_t iWidth, uint8_t *pResult, int32_t iLength)
{
    // make length in terms of words
    iLength /= sizeof(*pSource);

    // skip past unused space
    pSource += (iWidth - iLength);

    // word to byte conversion
    for (; iLength > 0; --iLength)
    {
        #if UCRYPT_SIZE > 4
        *pResult++ = (uint8_t)(*pSource>>56);
        *pResult++ = (uint8_t)(*pSource>>48);
        *pResult++ = (uint8_t)(*pSource>>40);
        *pResult++ = (uint8_t)(*pSource>>32);
        #endif
        #if UCRYPT_SIZE > 2
        *pResult++ = (uint8_t)(*pSource>>24);
        *pResult++ = (uint8_t)(*pSource>>16);
        #endif
        *pResult++ = (uint8_t)(*pSource>>8);
        *pResult++ = (uint8_t)(*pSource>>0);
        pSource += 1;
    }
}

/*F********************************************************************************/
/*!
    \Function _Exponentiate

    \Description
        Do the actual encryption: result = (value ^ exponent) % modulus

    \Input *pResult   - [out] resulting value
    \Input *pPowerof  - take this value to power of exponent
    \Input *pModulus  - result is modulus this value
    \Input iModSize   - length in bits
    \Input *pExponent - raise value to this power
    \Input iExpSize   - length in bits

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************H*/
static void _Exponentiate(uint8_t *pResult, uint8_t *pPowerof, const uint8_t *pModulus, int32_t iModSize,  const uint8_t *pExponent, int32_t iExpSize)
{
    int32_t iIndex;
    int32_t iWidth;
    uint32_t uExponent;
    ucrypt_t Modulus[4096/UCRYPT_BITSIZE];
    ucrypt_t Powerof[4096/UCRYPT_BITSIZE];
    ucrypt_t Accumul[4096/UCRYPT_BITSIZE];

    // copy over the modulus
    iWidth = _ToWords(Modulus, -1, pModulus, iModSize);

    // copy over data to encrypt
    _ToWords(Powerof, -1, pPowerof, iModSize);

    // get integer version of exponent
    uExponent = 0;
    for (iIndex = 0; iIndex < iExpSize; ++iIndex)
    {
        uExponent = (uExponent << 8) | pExponent[iIndex];
    }

    // handle specific exponents
    if (uExponent == 3)
    {
        // hardcode to e^3 mod n
        _Multiply(Accumul, iWidth, Powerof, Powerof, Modulus);  // a=e*e=e^2
        _Multiply(Accumul, iWidth, Accumul, Powerof, Modulus);  // a=e*a=e^3
    }
    else if (uExponent == 17)
    {
        // hardcode to e^17 mod n
        _Multiply(Accumul, iWidth, Powerof, Powerof, Modulus);  // a=e*e=e^2
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^4
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^8
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^16
        _Multiply(Accumul, iWidth, Accumul, Powerof, Modulus);  // a=a*e=e^17
    }
    else if (uExponent == 65537)
    {
        // hardcode to e^65537 mod n
        _Multiply(Accumul, iWidth, Powerof, Powerof, Modulus);  // a=e*e=e^2
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^4
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^8
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^16
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^32
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^64
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^128
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^256
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^512
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^1024
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^2048
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^4096
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^8192
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^16384
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^32768
        _Multiply(Accumul, iWidth, Accumul, Accumul, Modulus);  // a=a*a=e^65536
        _Multiply(Accumul, iWidth, Accumul, Powerof, Modulus);  // a=a*e=e^65537
    }
    // lower performing general case
    else
    {
        NetPrintf(("cryptrsa: warning; executing general case code, which is likely to be slow\n"));

        // initialize accumulator to 1
        memset(Accumul, 0, sizeof(Accumul));
        Accumul[iWidth-1] = 1;

        // loop through the multiplies
        for (; uExponent != 0; uExponent >>= 1)
        {
            if (uExponent & 1)
            {
                _Multiply(Accumul, iWidth, Accumul, Powerof, Modulus);
            }
            // this would be a shift in normal multiplication, but we use
            // modular multiply in order to preserve the modulus property
            _Multiply(Powerof, iWidth, Powerof, Powerof, Modulus);
        }
    }

    // return encrypted data
    _FromWords(Accumul, iWidth, pResult, iModSize);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptRSAInit

    \Description
        Init RSA state.

    \Input *pState      - module state
    \Input *pModulus    - crypto modulus
    \Input iModSize     - size of modulus
    \Input *pExponent   - crypto exponent
    \Input iExpSize     - size of exponent

    \Output int32_t     - zero for success, negative for error

    \Notes
        We don't support >4096 bits keys at this time, however pState will
        still be initialized with truncated keys to provide same behaviour
        with previous code, and -1 will be returned. New code should always
        check the return value.

    \Version 03/05/03 (jbrookes) Split from protossl
*/
/********************************************************************************H*/
int32_t CryptRSAInit(CryptRSAT *pState, const uint8_t *pModulus, int32_t iModSize, const uint8_t *pExponent, int32_t iExpSize)
{
    const int32_t iMaxModSize = sizeof(pState->KeyModData)-1;
    const int32_t iMaxExpSize = sizeof(pState->KeyExpData)-1;
    int32_t iResult = 0;

    // validate modulus size and clamp if greater than max
    if (iModSize > iMaxModSize)
    {
        NetPrintf(("cryptrsa: requested iModSize of %d is too large; truncating to %d\n", iModSize, iMaxModSize));
        iModSize = iMaxModSize;
        iResult = -1;
    }
    // validate exponent size and clamp if greater than max
    if (iExpSize > iMaxExpSize)
    {
        NetPrintf(("cryptrsa: requested iExpSize of %d is too large; truncating to %d\n", iExpSize, iMaxExpSize));
        iExpSize = iMaxExpSize;
        iResult = -1;
    }

    // save params and init state
    pState->iKeyModSize = iModSize;
    pState->iKeyExpSize = iExpSize;
    memcpy(pState->KeyModData, pModulus, iModSize);
    memcpy(pState->KeyExpData, pExponent, iExpSize);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitMaster

    \Description
        Setup the master shared secret for encryption

    \Input *pState      - module state
    \Input *pMaster     - the master shared secret to encrypt
    \Input *iMasterLen  - the length of the master shared secret

    \Version 03/03/2004 (sbecan) Factored out from CryptRSAInit
    \Version 05/25/2004 (gschaefer) Replaced Knuth RNG with a simpler RNG
*/
/********************************************************************************H*/
void CryptRSAInitMaster(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen)
{
    int32_t iIndex;
    uint32_t uRandom = NetTick();

    // make sure we extract out all bits from the seed
    for (iIndex = 0; iIndex < pState->iKeyModSize; ++iIndex)
    {
        pState->EncryptBlock[iIndex] = ((uRandom & (1<<(iIndex&31))) != 0);
    }

    // use PRNG to add more "color" to the data
    for (iIndex = 0; iIndex < pState->iKeyModSize; ++iIndex)
    {
        do
        {
            uRandom = (uRandom * 69069) + 69069;
            pState->EncryptBlock[iIndex] ^= (char)uRandom;
        }
        while (pState->EncryptBlock[iIndex] == 0);
    }

    // put in the real data
    pState->EncryptBlock[0] = 0;        // zero pad
    pState->EncryptBlock[1] = 2;        // public key is type 2
    iIndex = pState->iKeyModSize - iMasterLen;
    pState->EncryptBlock[iIndex-1] = 0; // zero byte prior to data
    memcpy(pState->EncryptBlock+iIndex, pMaster, iMasterLen);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitSignature

    \Description
        Setup the encrypted signature for decryption.

    \Input *pState   - module state
    \Input *pSig     - the encrypted signature
    \Input *iSigLen  - the length of the encrypted signature.

    \Version 03/03/2004 (sbevan)
*/
/********************************************************************************H*/
void CryptRSAInitSignature(CryptRSAT *pState, const uint8_t *pSig, int32_t iSigLen)
{
    memcpy(pState->EncryptBlock, pSig, iSigLen);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAEncrypt

    \Description
        Encrypt data.

    \Input *pState      - module state

    \Version 03/05/03 (jbrookes) Split from protossl
*/
/********************************************************************************H*/
void CryptRSAEncrypt(CryptRSAT *pState)
{
    _Exponentiate(pState->EncryptBlock, pState->EncryptBlock,
        pState->KeyModData, pState->iKeyModSize,
        pState->KeyExpData, pState->iKeyExpSize);
}

