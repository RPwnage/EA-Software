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

/*** Include files *********************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/crypt/cryptbn.h"

/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function _CryptBnLzCnt

    \Description
        Implements the function to find the MSB

    \Input uValue   - the input we are finding the msb for

    \Output
        int32_t     - the index of the MSB

    \Notes
        Uses BitScanReverse on PC as some processors that completely support
        the lzcnt instruction.

        Durango supports the lzcnt instruction so it can be used.

        The remaining platforms use the builtin as it has been supported by gcc
        for some time.

    \Version 03/06/2017 (eesponda)
*/
/*************************************************************************************H*/
static int32_t _CryptBnLzCnt(ucrypt_t uValue)
{
#if defined(DIRTYCODE_PC)
    unsigned long uResult;

    #if UCRYPT_SIZE == 4
    _BitScanReverse(&uResult, uValue);
    #else
    _BitScanReverse64(&uResult, uValue);
    #endif
    return(uResult + 1);
#elif defined(DIRTYCODE_XBOXONE)
    #if UCRYPT_SIZE == 4
    return(UCRYPT_BITSIZE - __lzcnt(uValue));
    #else
    return(UCRYPT_BITSIZE - __lzcnt64(uValue));
    #endif
#else
    #if UCRYPT_SIZE == 4
    return(UCRYPT_BITSIZE - __builtin_clz(uValue));
    #elif EA_PLATFORM_PTR_SIZE == 8
    return(UCRYPT_BITSIZE - __builtin_clzl(uValue));
    #else
    return(UCRYPT_BITSIZE - __builtin_clzll(uValue));
    #endif
#endif
}

/*F*************************************************************************************/
/*!
    \Function _CryptBnExpand

    \Description
        Expands the width of the big number by one prepending an entry

    \Input *pState      - the big number
    \Input uNewEntry    - the entry we are adding

    \Version 02/27/2017 (eesponda)
*/
/*************************************************************************************H*/
static void _CryptBnExpand(CryptBnT *pState, ucrypt_t uNewEntry)
{
    if (pState->iWidth < CRYPTBN_MAX_WIDTH)
    {
        pState->aData[pState->iWidth++] = uNewEntry;
    }
    else
    {
        NetPrintf(("cryptbn: tried to increase the size of number's width past the max\n"));
    }
}

/*F*************************************************************************************/
/*!
    \Function _CryptBnShrink

    \Description
        Shrink until you find a non-zero entry or the width is one

    \Input *pState      - the big number

    \Version 02/27/2017 (eesponda)
*/
/*************************************************************************************H*/
static void _CryptBnShrink(CryptBnT *pState)
{
    int32_t iWidth = pState->iWidth;

    while ((iWidth > 1) && (pState->aData[iWidth-1] == 0))
    {
        iWidth -= 1;
    }
    pState->iWidth = iWidth;
}

/*F*************************************************************************************/
/*!
    \Function _CryptBnSet

    \Description
        Set an entry from the big number

    \Input *pState  - the big number
    \Input iIndex   - the index of the entry
    \Input uValue   - the entry value

    \Notes
        This helps us safely retrieve entries when the index is outside our bounds

    \Version 02/27/2017 (eesponda)
*/
/*************************************************************************************H*/
static void _CryptBnSet(CryptBnT *pState, int32_t iIndex, ucrypt_t uValue)
{
    if (iIndex < pState->iWidth)
    {
        pState->aData[iIndex] = uValue;
    }
    else if (iIndex < CRYPTBN_MAX_WIDTH)
    {
        pState->aData[iIndex] = uValue;
        pState->iWidth = iIndex + 1;
    }
}

/*F*************************************************************************************/
/*!
    \Function _CryptBnIsZero

    \Description
        Checks if a big number is zero

    \Input *pState - the big number

    \Output
        uint8_t     - TRUE if zero, FALSE otherwise

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
static uint8_t _CryptBnIsZero(const CryptBnT *pState)
{
    int32_t iWidth;

    for (iWidth = pState->iWidth; iWidth > 0; iWidth -= 1)
    {
        if (pState->aData[iWidth-1] != 0)
        {
            return(FALSE);
        }
    }
    return(TRUE);
}

/*F*************************************************************************************/
/*!
    \Function _CryptBnAdd

    \Description
        Adds two big numbers together

    \Input *pState - the result of the operation
    \Input *pAdd1   - the left side
    \Input *pAdd2   - the right side

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
static void _CryptBnAdd(CryptBnT *pState, const CryptBnT *pAdd1, const CryptBnT *pAdd2)
{
    ucrypt_t uAccum;
    int32_t iWidth = DS_MAX(pAdd1->iWidth, pAdd2->iWidth), iCount;

    // do the add
    for (uAccum = 0, iCount = 0; iCount < iWidth; iCount += 1)
    {
        ucrypt_t uResult = uAccum, uLhs = pAdd1->aData[iCount], uRhs = pAdd2->aData[iCount];
        // do the math, setting carry on overflow
        uAccum  = ((uResult += uLhs) < uLhs);
        uAccum |= ((uResult += uRhs) < uRhs);
        // set the value
        _CryptBnSet(pState, iCount, uResult);
    }

    if (uAccum != 0)
    {
        _CryptBnExpand(pState, uAccum);
    }
}

/*F*************************************************************************************/
/*!
    \Function _CryptBnCompare

    \Description
        Compare two big numbers together

    \Input *pLhs    - the left side
    \Input *pRhs    - the right side

    \Output
        int32_t     - 0=equal, >0=left side larger, <0=right side larger

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
static int32_t _CryptBnCompare(const CryptBnT *pLhs, const CryptBnT *pRhs)
{
    int32_t iWidth;

    for (iWidth = DS_MAX(pLhs->iWidth, pRhs->iWidth); iWidth > 0; iWidth -= 1)
    {
        ucrypt_t uLhs = pLhs->aData[iWidth - 1], uRhs = pRhs->aData[iWidth - 1];
        if (uLhs == uRhs)
        {
            continue;
        }

        return((uLhs > uRhs) ? 1 : -1);
    }
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _CryptBnSubtract

    \Description
        Subtracts two big numbers

    \Input *pState  - result of subtraction
    \Input *pSub1   - the left hand side of the equation
    \Input *pSub2   - the right hand side of the equation

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
static void _CryptBnSubtract(CryptBnT *pState, const CryptBnT *pSub1, const CryptBnT *pSub2)
{
    ucrypt_t uAccum;
    int32_t iWidth = DS_MAX(pSub1->iWidth, pSub2->iWidth), iCount;

    for (uAccum = 0, iCount = 0; iCount < iWidth; iCount += 1)
    {
        ucrypt_t uResult, uFinalResult;
        ucrypt_t uLhs = pSub1->aData[iCount], uRhs = pSub2->aData[iCount];

        // calculate the result without borrow
        uResult = uLhs - uRhs;
        // calculate the result after borrow
        uFinalResult = uResult - uAccum;

        // calculate the new borrow if first or second calculation resulted in overflow
        uAccum = (uResult > uLhs) | (uFinalResult > uResult);

        // save result
        _CryptBnSet(pState, iCount, uFinalResult);
    }
}

/*** Public functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function CryptBnInit

    \Description
        Initializes the large number with zero at a given width

    \Input *pState  - pointer to output large number array of ucrypt_t units
    \Input iWidth   - width of output

    \Version 01/18/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnInit(CryptBnT *pState, int32_t iWidth)
{
    ds_memclr(pState, sizeof(*pState));
    pState->iWidth = iWidth;
}

/*F*************************************************************************************/
/*!
    \Function CryptBnInitSet

    \Description
        Initializes the large number with a number

    \Input *pState  - pointer to output large number array of ucrypt_t units
    \Input uValue   - value we are initializing the big number with

    \Version 01/18/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnInitSet(CryptBnT *pState, uint32_t uValue)
{
    ds_memclr(pState, sizeof(*pState));
    pState->iWidth = 1;
    pState->aData[0] = uValue;
}

/*F*************************************************************************************/
/*!
    \Function CryptBnInitFrom

    \Description
        Convert from msb bytes to int32_t number format

    \Input *pState  - pointer to output large number array of ucrypt_t units
    \Input iWidth   - width of output - if negative, auto-calculate width
    \Input *pSource - pointer to input large number array of bytes
    \Input iLength  - length of input in bytes

    \Output
        int32_t     - width in ucrypt_t units

    \Version 03/08/2002 (gschaefer)
    \Version 03/03/2004 (sbevan) Handle odd iLength
*/
/*************************************************************************************H*/
int32_t CryptBnInitFrom(CryptBnT *pState, int32_t iWidth, const uint8_t *pSource, int32_t iLength)
{
    ucrypt_t *pResult = pState->aData;
    int32_t iFullWordCount = iLength/sizeof(*pResult);
    int32_t iWordCount = (iLength+sizeof(*pResult)-1)/sizeof(*pResult);

    // clear the contents
    ds_memclr(pState, sizeof(*pState));

    if (iWidth < 0)
    {
        iWidth = iWordCount;
    }
    pState->iWidth = iWidth;

    if (iFullWordCount != iWordCount)
    {
        /* calculate how many bytes we need to write and
           generate a left shift amount based on that */
        int32_t iDiff = iLength - (iFullWordCount * UCRYPT_SIZE);
        int32_t iLeftShiftAmount = (iDiff - 1) * 8;

        // encode the bytes into the buffer until there is nothing left
        while (iLeftShiftAmount >= 0)
        {
            pResult[iWidth - 1] |= ((ucrypt_t)*pSource++ << iLeftShiftAmount);
            iLeftShiftAmount -= 8;
        }
        iWidth -= 1;
    }
    for (; iWidth > 0; iWidth -= 1)
    {
        #if (UCRYPT_SIZE == 4)
        pResult[iWidth-1] = (pSource[0] << 24) | (pSource[1] << 16) | (pSource[2] <<  8) | (pSource[3] <<  0);
        #else
        pResult[iWidth-1] = ((ucrypt_t)pSource[0] << 56) | ((ucrypt_t)pSource[1] << 48) | ((ucrypt_t)pSource[2] << 40) | ((ucrypt_t)pSource[3] << 32) |
                            ((ucrypt_t)pSource[4] << 24) | ((ucrypt_t)pSource[5] << 16) | ((ucrypt_t)pSource[6] <<  8) | ((ucrypt_t)pSource[7] <<  0);
        #endif
        pSource += UCRYPT_SIZE;
    }
    return(iWordCount);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnLeftShift

    \Description
        Left shifts a large number by a bit

    \Input *pState  - pointer to input and output large number array

    \Notes
        pState:iWidth <-- pState:iWidth << 1

    \Version 10/11/2011 (mdonais)
*/
/*************************************************************************************H*/
void CryptBnLeftShift(CryptBnT *pState)
{
    ucrypt_t uAccum;
    int32_t iWidth, iCount;

    // do the shift
    for (uAccum = 0, iCount = 0, iWidth = pState->iWidth; iCount < iWidth; iCount += 1)
    {
        ucrypt_t uResult, uFinalResult, uVal;

        uVal = pState->aData[iCount];
        uResult = uVal << 1;
        uFinalResult = uResult + uAccum;
        uAccum = (uResult < uVal);
        pState->aData[iCount] = uFinalResult;
    }

    // expand if necessary
    if (uAccum != 0)
    {
        _CryptBnExpand(pState, uAccum);
    }
}

/*F*************************************************************************************/
/*!
    \Function CryptBnBitTest

    \Description
        Check to see if a particular bit is set within a large bit vector

    \Input *pState  - pointer to input bit vector
    \Input iBit     - bit number to test (zero-relative)

    \Output
        uint8_t     - TRUE if set, else FALSE

    \Notes
        uResult <-- pValue:iWidth & (1 << iBit)

    \Version 03/08/2002 (gschaefer)
*/
/*************************************************************************************H*/
uint8_t CryptBnBitTest(const CryptBnT *pState, int32_t iBit)
{
    int32_t iBitOff = iBit & (UCRYPT_BITSIZE-1);
    int32_t iOffset = iBit / UCRYPT_BITSIZE;
    return((pState->aData[iOffset] & ((ucrypt_t)1 << iBitOff)) != 0);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnBitSet

    \Description
        Set a particular bit in the big number

    \Input *pState  - pointer to input bit vector
    \Input iBit     - bit number to set (zero-relative)

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnBitSet(CryptBnT *pState, int32_t iBit)
{
    int32_t iBitOff, iOffset;

    while (pState->iWidth*UCRYPT_BITSIZE <= iBit)
    {
        _CryptBnExpand(pState, 0);
    }
    iBitOff = iBit & (UCRYPT_BITSIZE - 1);
    iOffset = iBit / UCRYPT_BITSIZE;
    pState->aData[iOffset] |= ((ucrypt_t)1 << iBitOff);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnBitLen

    \Description
        Figures out the bit length of big number

    \Input *pState  - big number state

    \Output
        int32_t     - the number of bits in the big number

    \Version 03/02/2017 (eesponda)
*/
/*************************************************************************************H*/
int32_t CryptBnBitLen(const CryptBnT *pState)
{
    int32_t iResult, iIndex;
    for (iResult = 0, iIndex = pState->iWidth - 1; iIndex >= 0; iIndex -= 1)
    {
        if (pState->aData[iIndex] != 0)
        {
            iResult = _CryptBnLzCnt(pState->aData[iIndex]);
            break;
        }
    }
    iResult += UCRYPT_BITSIZE * iIndex;
    return(iResult);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnAccumulate

    \Description
        Accumulate one large number into another

    \Input *pState  - pointer to large number accumulation buffer
    \Input *pAdd    - pointer to large number to accumulate to accumulation buffer

    \Notes
        pState:iWidth <-- pState:iWidth + pAdd:iWidth

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnAccumulate(CryptBnT *pState, const CryptBnT *pAdd)
{
    _CryptBnAdd(pState, pState, pAdd);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnIncrement

    \Description
        Increment a large number by 1

    \Input *pState  - pointer to large number accumulation buffer

    \Version 02/21/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnIncrement(CryptBnT *pState)
{
    ucrypt_t uAccum;
    int32_t iCount, iWidth;

    // do the add
    for (uAccum = 1, iCount = 0, iWidth = pState->iWidth; iCount < iWidth; iCount += 1)
    {
        ucrypt_t uResult = uAccum, uVal = pState->aData[iCount];

        uAccum = ((uResult += uVal) < uVal);
        pState->aData[iCount] = uResult;

        // if we have nothing to carry over we are done
        if (uAccum == 0)
        {
            break;
        }
    }

    if (uAccum != 0)
    {
        _CryptBnExpand(pState, uAccum);
    }
}

/*F*************************************************************************************/
/*!
    \Function CryptBnSubtract

    \Description
        Subtract two large numbers

    \Input *pState  - pointer to output large number array
    \Input *pSub1   - pointer to large number to subtract pSub2 from.
    \Input *pSub2   - pointer to second large number

    \Notes
        pState:iWidth <-- pSub1:iWidth - pSub2:iWidth

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnSubtract(CryptBnT *pState, const CryptBnT *pSub1, const CryptBnT *pSub2)
{
    // if sub1 - (-sub2) or -sub1 - (sub2) then sub1 + sub2, sign depends on which side is negative
    if (pSub1->uSign ^ pSub2->uSign)
    {
        // add
        _CryptBnAdd(pState, pSub1, pSub2);
        // set sign
        pState->uSign = pSub1->uSign;
    }
    // if -sub1 - (-sub2) or sub1 - sub2
    else
    {
        const int32_t iCompare = _CryptBnCompare(pSub1, pSub2);

        // if sub1 > sub2
        if (iCompare > 0)
        {
            // sub1 - sub2
            _CryptBnSubtract(pState, pSub1, pSub2);
            // set sign
            pState->uSign = pSub1->uSign;
        }
        // else if sub1 < sub2
        else if (iCompare < 0)
        {
            // sub2 - sub1
            _CryptBnSubtract(pState, pSub2, pSub1);
            // set sign
            pState->uSign = ~pSub1->uSign;
        }
        // else they are the same so it is zero
        else
        {
            CryptBnInitSet(pState, 0);
        }
    }

    _CryptBnShrink(pState);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnDecrement

    \Description
        Decrement a large number by 1

    \Input *pState  - pointer to output large number array

    \Version 02/21/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnDecrement(CryptBnT *pState)
{
    ucrypt_t uAccum;
    int32_t iWidth, iCount;

    for (uAccum = 1, iCount = 0, iWidth = pState->iWidth; iCount < iWidth; iCount += 1)
    {
        ucrypt_t uResult, uVal = pState->aData[iCount];

        // calculate the result after borrow
        uResult = uVal - uAccum;
        // calculate the new borrow if first or second calculation resulted in overflow
        uAccum = (uResult > uVal);
        // save result
        _CryptBnSet(pState, iCount, uResult);

        // if nothing left to borrow we are done
        if (uAccum == 0)
        {
            break;
        }
    }
}

/*F*************************************************************************************/
/*!
    \Function CryptBnModMultiply

    \Description
        Modular multiply using the classical algorithm

    \Input *pState  - pointer to output large number array
    \Input *pMul1   - pointer to large number to multiply with pMul2
    \Input *pMul2   - pointer to second large number
    \Input *pMod    - pointer to modulous large number

    \Notes
        pState:iWidth <-- pMul1:iWidth * pMul2:iWidth % pMod:iWidth

    \Version 02/27/2002 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnModMultiply(CryptBnT *pState, const CryptBnT *pMul1, const CryptBnT *pMul2, const CryptBnT *pMod)
{
    int32_t iCount;
    CryptBnT Temp1;
    CryptBnT *pCur = &Temp1;
    uint32_t uSign = pMul2->uSign ^ pMul1->uSign;

    CryptBnInit(pCur, pMul1->iWidth);

    // do all the bits
    for (iCount = CryptBnBitLen(pMul1) - 1; iCount >= 0; iCount -= 1)
    {
        // left shift the result
        CryptBnLeftShift(pCur);

        // do modulus reduction?
        if (_CryptBnCompare(pCur, pMod) > 0)
        {
            CryptBnSubtract(pCur, pCur, pMod);
        }

        // see if we need to add in multiplicand
        if (CryptBnBitTest(pMul1, iCount))
        {
            // add it in
            CryptBnAccumulate(pCur, pMul2);

            // do modulus reduction?
            if (_CryptBnCompare(pCur, pMod) > 0)
            {
                CryptBnSubtract(pCur, pCur, pMod);
            }
        }
    }

    // deal with going negative
    if (uSign != 0)
    {
        CryptBnSubtract(pCur, pMod, pCur);
    }

    // copy over the result
    CryptBnClone(pState, pCur);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnMultiply

    \Description
        Performs multiplication of two big numbers using the classical method

    \Input *pState  - pointer to output large number array
    \Input *pMul1   - pointer to large number to multiply with pMul2
    \Input *pMul2   - pointer to second large number

    \Notes
        pState:iWidth <-- pMul1:iWidth * pMul2:iWidth

    \Version 02/28/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnMultiply(CryptBnT *pState, const CryptBnT *pMul1, const CryptBnT *pMul2)
{
    int32_t iCount;
    CryptBnT Temp1;
    CryptBnT *pCur = &Temp1;

    CryptBnInit(pCur, pMul1->iWidth);

    // do all the bits
    for (iCount = CryptBnBitLen(pMul1) - 1; iCount >= 0; iCount -= 1)
    {
        // left shift the result
        CryptBnLeftShift(pCur);

        // see if we need to add in multiplicand
        if (CryptBnBitTest(pMul1, iCount))
        {
            // add it in
            CryptBnAccumulate(pCur, pMul2);
        }
    }

    pCur->uSign = pMul1->uSign ^ pMul2->uSign;

    // copy over the result
    CryptBnClone(pState, pCur);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnMod

    \Description
        Performs modulo / divison of dividend by divisor

    \Input *pDividend   - the dividend in the equation
    \Input *pDivisor    - the divisor in the equation
    \Input *pRemainder  - [out] the result of the modulo operation can be NULL
    \Input *pQuotient   - [out] the result of the division operation can be NULL

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnMod(const CryptBnT *pDividend, const CryptBnT *pDivisor, CryptBnT *pRemainder, CryptBnT *pQuotient)
{
    int32_t iCount;
    CryptBnT Result, Quotient;

    /* if the dividend is less than the divisor means the result of
    the modulus is the dividend and the quotient is zero */
    const int32_t iCompare = _CryptBnCompare(pDividend, pDivisor);
    if (iCompare < 0)
    {
        CryptBnClone(&Result, pDividend);
        CryptBnInitSet(&Quotient, 0);
    }
    else if (iCompare > 0)
    {
        CryptBnInitSet(&Result, 0);
        CryptBnInitSet(&Quotient, 0);

        for (iCount = CryptBnBitLen(pDividend) - 1; iCount >= 0; iCount -= 1)
        {
            if (CryptBnBitTest(pDividend, iCount))
            {
                CryptBnIncrement(&Result);
            }

            // check if we need to reduce
            if (_CryptBnCompare(&Result, pDivisor) >= 0)
            {
                CryptBnSubtract(&Result, &Result, pDivisor);
                CryptBnBitSet(&Quotient, iCount);
            }

            if (iCount > 0)
            {
                CryptBnLeftShift(&Result);
            }
        }
    }
    else
    {
        CryptBnInitSet(&Result, 0);
        CryptBnInitSet(&Quotient, 1);
    }

    if (pDividend->uSign != 0)
    {
        Result.uSign = 0;
        CryptBnSubtract(&Result, pDivisor, &Result);
    }
    if (pRemainder != NULL)
    {
        CryptBnClone(pRemainder, &Result);
    }
    if (pQuotient != NULL)
    {
        CryptBnClone(pQuotient, &Quotient);
    }
}

/*F*************************************************************************************/
/*!
    \Function CryptBnInverseMod

    \Description
        Calculates the modular multiplicative inverse using extended euclidean

    \Input *pState  - the big number state
    \Input *pMod    - the modulus

    \Version 02/17/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnInverseMod(CryptBnT *pState, const CryptBnT *pMod)
{
    CryptBnT M, N, X, Y;

    if (pState->uSign != 0)
    {
        pState->uSign = 0;
        CryptBnInverseMod(pState, pMod);
        CryptBnSubtract(pState, pMod, pState);
        return;
    }

    CryptBnClone(&M, pMod);
    CryptBnClone(&N, pState);
    CryptBnInitSet(&X, 0);
    CryptBnInitSet(&Y, 1);

    while (!_CryptBnIsZero(&N))
    {
        CryptBnT Temp, Temp2;

        /* X - (M / N) * Y and M % N */
        CryptBnMod(&M, &N, &Temp2, &Temp);
        CryptBnMultiply(&Temp, &Temp, &Y);
        CryptBnSubtract(&Temp, &X, &Temp);

        CryptBnClone(&X, &Y);
        CryptBnClone(&Y, &Temp);
        CryptBnClone(&M, &N);
        CryptBnClone(&N, &Temp2);
    }

    // if -pX then pX = pMod - pX
    if (X.uSign != 0)
    {
        X.uSign = 0;
        CryptBnSubtract(&X, pMod, &X);
    }

    CryptBnClone(pState, &X);
}

/*F*************************************************************************************/
/*!
    \Function CryptBnFinal

    \Description
        Convert from ucrypt_t units back to big-endian bytes

    \Input *pState - the ucrypt_t units to convert
    \Input *pResult - where to write the 8-bit result.
    \Input iLength  - size of the result array in bytes.

    \Version 03/08/2002 (gschaefer)
*/
/*************************************************************************************H*/
void CryptBnFinal(const CryptBnT *pState, uint8_t *pResult, int32_t iLength)
{
    const ucrypt_t *pSource = pState->aData;

    // make the length in terms of words
    iLength /= sizeof(*pSource);

    // word to byte conversion
    for (; iLength > 0; iLength -= 1)
    {
        #if UCRYPT_SIZE > 4
        *pResult++ = (uint8_t)(pSource[iLength-1] >> 56);
        *pResult++ = (uint8_t)(pSource[iLength-1] >> 48);
        *pResult++ = (uint8_t)(pSource[iLength-1] >> 40);
        *pResult++ = (uint8_t)(pSource[iLength-1] >> 32);
        #endif
        *pResult++ = (uint8_t)(pSource[iLength-1] >> 24);
        *pResult++ = (uint8_t)(pSource[iLength-1] >> 16);
        *pResult++ = (uint8_t)(pSource[iLength-1] >>  8);
        *pResult++ = (uint8_t)(pSource[iLength-1]);
    }
}

/*F*************************************************************************************/
/*!
    \Function CryptBnClone

    \Description
        Copies a big number's contents

    \Input *pDst    - where we are copying to
    \Input *pSrc    - where we are copying from

    \Version 02/27/2017 (eesponda)
*/
/*************************************************************************************H*/
void CryptBnClone(CryptBnT *pDst, const CryptBnT *pSrc)
{
    ds_memcpy(pDst, pSrc, sizeof(*pDst));
}
