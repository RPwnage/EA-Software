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
#include "DirtySDK/crypt/cryptrand.h"

#include "DirtySDK/crypt/cryptrsa.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

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
    return((ucrypt_t)uAccum);
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
        *pIO = (ucrypt_t)(uVal << 1) + (ucrypt_t)uAccum;
        uAccum = uVal >> (UCRYPT_BITSIZE-1);
    }

    // return carry flag
    return((ucrypt_t)uAccum);
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
    return((ucrypt_t)uAccum);
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
    ds_memcpy(pResult, pCur, iWidth * sizeof(*pResult));
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
        Do the actual encryption: result = (value ^ exponent) % modulus.
        Exponentiation is done one exponent bit at a time, this function needs to
        be called iteratively until the function returns zero.

    \Input *pState      - crypt state

    \Output
        int32_t         - zero=done, else call again

    \Version 03/08/2002 (jbrookes) Rewrote original gschaefer version to be iterative
*/
/********************************************************************************H*/
static int32_t _Exponentiate(CryptRSAT *pState)
{
    uint64_t uTickUsecs = NetTickUsec();
    int32_t iResult = 1;

    // read next exponent byte
    if (pState->iExpBitIndex == 0)
    {
        pState->uExponent = pState->KeyExpData[pState->iExpByteIndex];
    }
        
    // perform exponentiation
    if (pState->uExponent & 1)
    {
        if (!pState->bAccumulOne)
        {
            _Multiply(pState->Accumul, pState->iWidth, pState->Accumul, pState->Powerof, pState->Modulus);
        }
        else // optimization; we can skip the first multiply if the accumulator is set to one
        {
            ds_memcpy(pState->Accumul, pState->Powerof, pState->iWidth*sizeof(pState->Accumul[0]));
            pState->bAccumulOne = FALSE;
        }
    }

    // consume exponent bit
    pState->iExpBitIndex += 1;
    pState->uExponent >>= 1;

    // square the input for each exponent bit
    if ((pState->iExpByteIndex > 0) || (pState->uExponent != 0))
    {
        // this would be a shift in normal multiplication, but we use modular multiply in order to preserve the modulus property
        _Multiply(pState->Powerof, pState->iWidth, pState->Powerof, pState->Powerof, pState->Modulus);
    }
    else
    {
        // we can skip unnecessary multiplies if there are no more exponent bits
        pState->iExpByteIndex = -1;
    }

    // get the next exponent byte
    if (pState->iExpBitIndex == 8)
    {
        pState->iExpByteIndex -= 1;
        pState->iExpBitIndex = 0;
    }

    // return encrypted data when we're done
    if (pState->iExpByteIndex < 0)
    {
        _FromWords(pState->Accumul, pState->iWidth, pState->EncryptBlock, pState->iKeyModSize);
        pState->uCryptMsecs = (pState->uCryptUsecs+500)/1000;
        iResult = 0;
    }

    // update timing
    pState->uCryptUsecs += (uint32_t)NetTickDiff(NetTickUsec(), uTickUsecs);
    pState->uNumExpCalls += 1;

    return(iResult);
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

    \Output
        int32_t         - zero for success, negative for error

    \Notes
        This module supports a max modulus size of 4096 (iModSize=512) and requires
        a minimum size of 1024 (iModSize=64).  The exponent must be between 1
        and 512 bytes in size, inclusive.

    \Version 03/05/03 (jbrookes) Split from protossl
*/
/********************************************************************************H*/
int32_t CryptRSAInit(CryptRSAT *pState, const uint8_t *pModulus, int32_t iModSize, const uint8_t *pExponent, int32_t iExpSize)
{
    int32_t iResult = 0;

    // validate modulus size
    if ((iModSize < 64) || (iModSize > (signed)(sizeof(pState->KeyModData)-1)))
    {
        NetPrintf(("cryptrsa: iModSize of %d is invalid\n", iModSize));
        return(-1);
    }
    // validate exponent size
    if ((iExpSize < 1) || (iExpSize > (signed)(sizeof(pState->KeyExpData)-1)))
    {
        NetPrintf(("cryptrsa: iExpSize of %d is invalid\n", iExpSize));
        return(-2);
    }

    // initialize state
    memset(pState, 0, sizeof(*pState));

    // save params and init state
    pState->iKeyModSize = iModSize;
    pState->iKeyExpSize = iExpSize;
    ds_memcpy(pState->KeyModData, pModulus, iModSize);
    ds_memcpy(pState->KeyExpData, pExponent, iExpSize);

    // copy over the modulus
    pState->iWidth = _ToWords(pState->Modulus, -1, pState->KeyModData, pState->iKeyModSize);

    // initialize accumulator to 1 (and remember for first-multiply optimization)
    pState->Accumul[pState->iWidth-1] = 1;
    pState->bAccumulOne = TRUE;

    // set starting exponent byte index
    pState->iExpByteIndex = pState->iKeyExpSize-1;

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitMaster

    \Description
        Setup the master shared secret for encryption

    \Input *pState      - module state
    \Input *pMaster     - the master shared secret to encrypt
    \Input iMasterLen   - the length of the master shared secret

    \Version 02/07/2014 (jbrookes) Rewritten to use CryptRand
*/
/********************************************************************************H*/
void CryptRSAInitMaster(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen)
{
    int32_t iIndex;
    uint32_t uRandom;

    // fill encrypt block with random data
    CryptRandGet(pState->EncryptBlock, pState->iKeyModSize);
    /* As per PKCS1 http://www.emc.com/emc-plus/rsa-labs/pkcs/files/h11300-wp-pkcs-1v2-2-rsa-cryptography-standard.pdf section 7.2.1,
       the pseudo-random padding octets must be non-zero.  Failing to adhere to this restriction (or any other errors in pkcs1 encoding)
       will typically result in a bad_record_mac fatal alert from the remote host.  The following code gets four random bytes as seed
       and uses a simple Knuth RNG to fill in any zero bytes that were originally generated. */
    CryptRandGet((uint8_t *)&uRandom, sizeof(uRandom));
    for (iIndex = 0; iIndex < pState->iKeyModSize; iIndex += 1)
    {
        while (pState->EncryptBlock[iIndex] == 0)
        {
            uRandom = (uRandom * 69069) + 69069;
            pState->EncryptBlock[iIndex] ^= (uint8_t)uRandom;
        }
    }
    // set PKCS public key signature
    pState->EncryptBlock[0] = 0;        // zero pad
    pState->EncryptBlock[1] = 2;        // public key is type 2
    // add data to encrypt block
    iIndex = pState->iKeyModSize - iMasterLen;
    pState->EncryptBlock[iIndex-1] = 0; // zero byte prior to data
    ds_memcpy(pState->EncryptBlock+iIndex, pMaster, iMasterLen);
    // convert data to encrypt to native word size we will operate in
    _ToWords(pState->Powerof, -1, pState->EncryptBlock, pState->iKeyModSize);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitPrivate

    \Description
        Setup the master shared secret for encryption using PKCS1.5

    \Input *pState      - module state
    \Input *pMaster     - the master shared secret to encrypt
    \Input iMasterLen   - the length of the master shared secret

    \Version 11/03/2013 (jbrookes)
*/
/********************************************************************************H*/
void CryptRSAInitPrivate(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen)
{
    int32_t iIndex;
    // put in PKCS1.5 signature
    pState->EncryptBlock[0] = 0;
    pState->EncryptBlock[1] = 1;
    // PKCS1.5 signing pads with 0xff
    memset(pState->EncryptBlock+2, 0xff, pState->iKeyModSize-2);
    // add data to encrypt block
    iIndex = pState->iKeyModSize - iMasterLen;
    pState->EncryptBlock[iIndex-1] = 0; // zero byte prior to data
    ds_memcpy(pState->EncryptBlock+iIndex, pMaster, iMasterLen);
    // convert data to encrypt to native word size we will operate in
    _ToWords(pState->Powerof, -1, pState->EncryptBlock, pState->iKeyModSize);
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
    ds_memcpy(pState->EncryptBlock, pSig, iSigLen);
    // convert data to encrypt to native word size we will operate in
    _ToWords(pState->Powerof, -1, pState->EncryptBlock, pState->iKeyModSize);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAEncrypt

    \Description
        Encrypt data.

    \Input *pState      - module state
    \Input iIter        - number of iterations to execute (zero=do all)

    \Output
        int32_t         - zero=operation complete, else call again

    \Version 03/05/2003 (jbrookes) Split from protossl
*/
/********************************************************************************H*/
int32_t CryptRSAEncrypt(CryptRSAT *pState, int32_t iIter)
{
    int32_t iCount, iResult;
    if (iIter == 0)
    {
        iIter = 0x7fffffff;
    }
    for (iCount = 0, iResult = 1; (iCount < iIter) && (iResult > 0); iCount += 1)
    {
        iResult = _Exponentiate(pState);
    }
    return(iResult);
}


