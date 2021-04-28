/*H********************************************************************************/
/*!
    \File   crypt25519.c

    \Description
        This module implements the shared math for elliptic curve cryptography
        using 25519 prime

        This is used for our edwards and montgomery curves

        For more details see cryped.c and cryptmont.c

    \Copyright
        Copyright (c) Electronic Arts 2020. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#include <string.h>
#include "crypt25519.h"

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function Crypt25519Load3

    \Description
        Loads a uint64_t from a 3-byte array

    \Input *pInput  - 3-byte array to load from

    \Output
        uint64_t    - the parsed output from the 3-byte array

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
uint64_t Crypt25519Load3(const uint8_t *pInput)
{
    uint64_t uResult;
    uResult = (uint64_t)pInput[0];
    uResult |= ((uint64_t)pInput[1]) << 8;
    uResult |= ((uint64_t)pInput[2]) << 16;
    return(uResult);
}

/*F********************************************************************************/
/*!
    \Function Crypt25519Load4

    \Description
        Loads a uint64_t from a 4-byte array

    \Input *pInput  - 4-byte array to load from

    \Output
        uint64_t    - the parsed output from the 4-byte array

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
uint64_t Crypt25519Load4(const uint8_t *pInput)
{
    uint64_t uResult;
    uResult = (uint64_t)pInput[0];
    uResult |= ((uint64_t)pInput[1]) << 8;
    uResult |= ((uint64_t)pInput[2]) << 16;
    uResult |= ((uint64_t)pInput[3]) << 24;
    return(uResult);
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldInit

    \Description
        Initializes the field element to a value

    \Input pOutput      - [out] field element we are initializing
    \Input iInitValue   - value to initialize with

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldInit(FieldElementT *pOutput, int32_t iInitValue)
{
    int32_t iIndex;

    pOutput->aData[0] = iInitValue;
    for (iIndex = 1; iIndex < 10; iIndex += 1)
    {
        pOutput->aData[iIndex] = 0;
    }
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldCopy

    \Description
        Copies one field element to another

    \Input pDst - [out] destination of the copy
    \Input pSrc - source of the copy

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldCopy(FieldElementT *pDst, const FieldElementT *pSrc)
{
    ds_memcpy(pDst, pSrc, sizeof(*pDst));
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldConditionalMove

    \Description
        Moves one field element to another based on the swap parameter

    \Input pP1      - [out] first field
    \Input pP2      - second field
    \Input uSwap    - determines if a swap should happen

    \Notes
        This function is meant to perform the operation in constant time

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldConditionalMove(FieldElementT *pP1, const FieldElementT *pP2, uint32_t uSwap)
{
    int32_t iIndex;
    uSwap = -(int32_t)uSwap;

    for (iIndex = 0; iIndex < 10; iIndex += 1)
    {
        pP1->aData[iIndex] ^= (pP1->aData[iIndex] ^ pP2->aData[iIndex]) & uSwap;
    }
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldConditionalSwap

    \Description
        Swaps one field element to another based on the swap parameter

    \Input pP1      - [out] first field
    \Input pP2      - second field
    \Input uSwap    - determines if a swap should happen

    \Notes
        This function is meant to perform the operation in constant time

    \Version 07/08/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldConditionSwap(FieldElementT *pP1, FieldElementT *pP2, uint32_t uSwap)
{
    int32_t iIndex;
    uSwap = -(int32_t)uSwap;

    for (iIndex = 0; iIndex < 10; iIndex += 1)
    {
        const int32_t iResult = (pP1->aData[iIndex] ^ pP2->aData[iIndex]) & uSwap;
        pP1->aData[iIndex] ^= iResult;
        pP2->aData[iIndex] ^= iResult;
    }
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldNegate

    \Description
        Negates the value of a field and outputs it to another field

    \Input pDst - [out] destination of the negate
    \Input pSrc - input of the negate

    \Notes
        Input/Output are bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.
        Both inputs may overlap

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldNegate(FieldElementT *pDst, const FieldElementT *pSrc)
{
    int32_t iIndex;

    for (iIndex = 0; iIndex < 10; iIndex += 1)
    {
        pDst->aData[iIndex] = -pSrc->aData[iIndex];
    }
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldToBytes

    \Description
        Writes out the field to a byte array

    \Input pDst - [out] output of the field as a byte array
    \Input pSrc - source field we are converting

    \Notes
        Source is bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.

        Write p=2^255-19; q=floor(src/p)
        Basic claim q=floor(2^(-255)(src + 19 2^(-25)src9 + 2^(-1)))

        Proof:
        Have |src|<=p so |q|<=1 so |19^2 2^(-255) q|<1/4.
        Also have |src-2^230 src9|<2^231 so |19 2^(-255)(src-2^230 src9)|<1/4.

        Write y=2^(-1)-19^2 2^(-255)q-19 2^(-255)(src-2^230 src9).
        Then 0<y<1.

        Write r=src-pq.
        Have 0<=r<=p-1=2^255-20.
        Thus 0<=r+19(2^-255)r<r+19(2^-255)2^255<=2^255-1.

        Write x=r+19(2^-255)r+y.
        Then 0<x<2^255 so floor(2^(-255)x) = 0 so floor(q+2^(-255)x) = q.

        Have q+2^(-255)x = 2^(-255)(src + 19 2^(-25) src9 + 2^(-1))
        so floor(2^(-255)(src + 19 2^(-25) src9 + 2^(-1))) = q.

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldToBytes(uint8_t *pDst, const FieldElementT *pSrc)
{
    int32_t iSrc0 = pSrc->aData[0];
    int32_t iSrc1 = pSrc->aData[1];
    int32_t iSrc2 = pSrc->aData[2];
    int32_t iSrc3 = pSrc->aData[3];
    int32_t iSrc4 = pSrc->aData[4];
    int32_t iSrc5 = pSrc->aData[5];
    int32_t iSrc6 = pSrc->aData[6];
    int32_t iSrc7 = pSrc->aData[7];
    int32_t iSrc8 = pSrc->aData[8];
    int32_t iSrc9 = pSrc->aData[9];
    int32_t iQ, iCarry0, iCarry1, iCarry2, iCarry3, iCarry4, iCarry5, iCarry6, iCarry7, iCarry8, iCarry9;

    iQ = (19 * iSrc9 + (((int32_t) 1) << 24)) >> 25;
    iQ = (iSrc0 + iQ) >> 26;
    iQ = (iSrc1 + iQ) >> 25;
    iQ = (iSrc2 + iQ) >> 26;
    iQ = (iSrc3 + iQ) >> 25;
    iQ = (iSrc4 + iQ) >> 26;
    iQ = (iSrc5 + iQ) >> 25;
    iQ = (iSrc6 + iQ) >> 26;
    iQ = (iSrc7 + iQ) >> 25;
    iQ = (iSrc8 + iQ) >> 26;
    iQ = (iSrc9 + iQ) >> 25;

    /* Goal: Output iSrc-(2^255-19)iQ, wiSrciciSrc is between 0 and 2^255-20. */
    iSrc0 += 19 * iQ;
    /* Goal: Output iSrc-2^255 iQ, wiSrciciSrc is between 0 and 2^255-20. */

    iCarry0 = iSrc0 >> 26;
    iSrc1 += iCarry0;
    iSrc0 -= iCarry0 << 26;

    iCarry1 = iSrc1 >> 25;
    iSrc2 += iCarry1;
    iSrc1 -= iCarry1 << 25;

    iCarry2 = iSrc2 >> 26;
    iSrc3 += iCarry2;
    iSrc2 -= iCarry2 << 26;

    iCarry3 = iSrc3 >> 25;
    iSrc4 += iCarry3;
    iSrc3 -= iCarry3 << 25;

    iCarry4 = iSrc4 >> 26;
    iSrc5 += iCarry4;
    iSrc4 -= iCarry4 << 26;

    iCarry5 = iSrc5 >> 25;
    iSrc6 += iCarry5;
    iSrc5 -= iCarry5 << 25;

    iCarry6 = iSrc6 >> 26;
    iSrc7 += iCarry6;
    iSrc6 -= iCarry6 << 26;

    iCarry7 = iSrc7 >> 25;
    iSrc8 += iCarry7;
    iSrc7 -= iCarry7 << 25;

    iCarry8 = iSrc8 >> 26;
    iSrc9 += iCarry8;
    iSrc8 -= iCarry8 << 26;

    iCarry9 = iSrc9 >> 25;
    iSrc9 -= iCarry9 << 25;
    /* iSrc10 = iCarry9 */

    /*
    Goal: Output iSrc0+...+2^255 iSrc10-2^255 iQ, wiSrciciSrc is between 0 and 2^255-20.
    Have iSrc0+...+2^230 iSrc9 between 0 and 2^255-1;
    evidently 2^255 iSrc10-2^255 iQ = 0.
    Goal: Output iSrc0+...+2^230 iSrc9.
    */

    pDst[0] = iSrc0 >> 0;
    pDst[1] = iSrc0 >> 8;
    pDst[2] = iSrc0 >> 16;
    pDst[3] = (iSrc0 >> 24) | (iSrc1 << 2);
    pDst[4] = iSrc1 >> 6;
    pDst[5] = iSrc1 >> 14;
    pDst[6] = (iSrc1 >> 22) | (iSrc2 << 3);
    pDst[7] = iSrc2 >> 5;
    pDst[8] = iSrc2 >> 13;
    pDst[9] = (iSrc2 >> 21) | (iSrc3 << 5);
    pDst[10] = iSrc3 >> 3;
    pDst[11] = iSrc3 >> 11;
    pDst[12] = (iSrc3 >> 19) | (iSrc4 << 6);
    pDst[13] = iSrc4 >> 2;
    pDst[14] = iSrc4 >> 10;
    pDst[15] = iSrc4 >> 18;
    pDst[16] = iSrc5 >> 0;
    pDst[17] = iSrc5 >> 8;
    pDst[18] = iSrc5 >> 16;
    pDst[19] = (iSrc5 >> 24) | (iSrc6 << 1);
    pDst[20] = iSrc6 >> 7;
    pDst[21] = iSrc6 >> 15;
    pDst[22] = (iSrc6 >> 23) | (iSrc7 << 3);
    pDst[23] = iSrc7 >> 5;
    pDst[24] = iSrc7 >> 13;
    pDst[25] = (iSrc7 >> 21) | (iSrc8 << 4);
    pDst[26] = iSrc8 >> 4;
    pDst[27] = iSrc8 >> 12;
    pDst[28] = (iSrc8 >> 20) | (iSrc9 << 6);
    pDst[29] = iSrc9 >> 2;
    pDst[30] = iSrc9 >> 10;
    pDst[31] = iSrc9 >> 18;
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldFromBytes

    \Description
        Writes out the byte array to a field

    \Input pDst - [out] output of the field
    \Input pSrc - source byte array we are converting

    \Notes
        Ignores top bit of destination

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldFromBytes(FieldElementT *pDst, const uint8_t *pSrc)
{
    int64_t iSrc0 = Crypt25519Load4(pSrc);
    int64_t iSrc1 = Crypt25519Load3(pSrc + 4) << 6;
    int64_t iSrc2 = Crypt25519Load3(pSrc + 7) << 5;
    int64_t iSrc3 = Crypt25519Load3(pSrc + 10) << 3;
    int64_t iSrc4 = Crypt25519Load3(pSrc + 13) << 2;
    int64_t iSrc5 = Crypt25519Load4(pSrc + 16);
    int64_t iSrc6 = Crypt25519Load3(pSrc + 20) << 7;
    int64_t iSrc7 = Crypt25519Load3(pSrc + 23) << 5;
    int64_t iSrc8 = Crypt25519Load3(pSrc + 26) << 4;
    int64_t iSrc9 = (Crypt25519Load3(pSrc + 29) & 8388607) << 2;
    int64_t iCarry0, iCarry1, iCarry2, iCarry3, iCarry4, iCarry5, iCarry6, iCarry7, iCarry8, iCarry9;

    iCarry9 = (iSrc9 + (int64_t) (1<<24)) >> 25;
    iSrc0 += iCarry9 * 19;
    iSrc9 -= iCarry9 << 25;

    iCarry1 = (iSrc1 + (int64_t) (1<<24)) >> 25;
    iSrc2 += iCarry1;
    iSrc1 -= iCarry1 << 25;

    iCarry3 = (iSrc3 + (int64_t) (1<<24)) >> 25;
    iSrc4 += iCarry3;
    iSrc3 -= iCarry3 << 25;

    iCarry5 = (iSrc5 + (int64_t) (1<<24)) >> 25;
    iSrc6 += iCarry5;
    iSrc5 -= iCarry5 << 25;

    iCarry7 = (iSrc7 + (int64_t) (1<<24)) >> 25;
    iSrc8 += iCarry7;
    iSrc7 -= iCarry7 << 25;

    iCarry0 = (iSrc0 + (int64_t) (1<<25)) >> 26;
    iSrc1 += iCarry0;
    iSrc0 -= iCarry0 << 26;

    iCarry2 = (iSrc2 + (int64_t) (1<<25)) >> 26;
    iSrc3 += iCarry2;
    iSrc2 -= iCarry2 << 26;

    iCarry4 = (iSrc4 + (int64_t) (1<<25)) >> 26;
    iSrc5 += iCarry4;
    iSrc4 -= iCarry4 << 26;

    iCarry6 = (iSrc6 + (int64_t) (1<<25)) >> 26;
    iSrc7 += iCarry6;
    iSrc6 -= iCarry6 << 26;

    iCarry8 = (iSrc8 + (int64_t) (1<<25)) >> 26;
    iSrc9 += iCarry8;
    iSrc8 -= iCarry8 << 26;

    pDst->aData[0] = (int32_t)iSrc0;
    pDst->aData[1] = (int32_t)iSrc1;
    pDst->aData[2] = (int32_t)iSrc2;
    pDst->aData[3] = (int32_t)iSrc3;
    pDst->aData[4] = (int32_t)iSrc4;
    pDst->aData[5] = (int32_t)iSrc5;
    pDst->aData[6] = (int32_t)iSrc6;
    pDst->aData[7] = (int32_t)iSrc7;
    pDst->aData[8] = (int32_t)iSrc8;
    pDst->aData[9] = (int32_t)iSrc9;
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldIsNegative

    \Description
        Checks if the field is negative

    \Input pInput   - field we are checking

    \Output
        uint8_t     - TRUE if input is in {1,3,5,...,q-2}, FALSE if input is in {0,2,4,...,q-1}

    \Notes
        Input is bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
uint8_t Crypt25519FieldIsNegative(const FieldElementT *pInput)
{
    uint8_t aInput[32];
    Crypt25519FieldToBytes(aInput, pInput);
    return(aInput[0] & 1);
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldIsNonZero

    \Description
        Checks if the field is non-zero

    \Input pInput   - field we are checking

    \Output
        uint8_t     - TRUE if input == 0, FALSE if input != 0

    \Notes
        Input is bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
uint8_t Crypt25519FieldIsNonZero(const FieldElementT *pInput)
{
    static const uint8_t _aZero[32] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    uint8_t aInput[32];

    Crypt25519FieldToBytes(aInput, pInput);
    return(memcmp(aInput, _aZero, sizeof(_aZero)));
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldAdd

    \Description
        Performs output = lhs + rhs

    \Input pOutput  - [out] output of the operation
    \Input pLhs     - left hand of the addition
    \Input pRhs     - right hand of the addition

    \Notes
        Inputs are bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.
        Output is bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.
        Output can overlap with either input

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldAdd(FieldElementT *pOutput, const FieldElementT *pLhs, const FieldElementT *pRhs)
{
    int32_t iIndex;

    for (iIndex = 0; iIndex < 10; iIndex += 1)
    {
        pOutput->aData[iIndex] = pLhs->aData[iIndex] + pRhs->aData[iIndex];
    }
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldSubtract

    \Description
        Performs output = lhs - rhs

    \Input pOutput  - [out] output of the operation
    \Input pLhs     - left hand of the subtraction
    \Input pRhs     - right hand of the subtraction

    \Notes
        Inputs are bounded by 1.1*2^25,1.1*2^24,1.1*2^25,1.1*2^24,etc.
        Output is bounded by 1.1*2^26,1.1*2^25,1.1*2^26,1.1*2^25,etc.
        Output can overlap with either input

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldSubtract(FieldElementT *pOutput, const FieldElementT *pLhs, const FieldElementT *pRhs)
{
    int32_t iIndex;

    for (iIndex = 0; iIndex < 10; iIndex += 1)
    {
        pOutput->aData[iIndex] = pLhs->aData[iIndex] - pRhs->aData[iIndex];
    }
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldMultiply

    \Description
        Performs output = lhs * rhs

    \Input pOutput  - [out] output of the operation
    \Input pLhs     - left hand of the multiplication
    \Input pRhs     - right hand of the multiplication

    \Notes
        Inputs are bounded by 1.65*2^26,1.65*2^25,1.65*2^26,1.65*2^25,etc.
        Output is bounded by 1.01*2^25,1.01*2^24,1.01*2^25,1.01*2^24,etc.
        Output can overlap with either input

        Using schoolbook multiplication.
        Karatsuba would save a little in some cost models.

        Most multiplications by 2 and 19 are 32-bit precomputations;
        cheaper than 64-bit postcomputations.

        There is one remaining multiplication by 19 in the carry chain;
        one *19 precomputation can be merged into this,
        but the resulting data flow is considerably less clean.

        There are 12 carries below.
        10 of them are 2-way parallelizable and vectorizable.
        Can get away with 11 carries, but then data flow is much deeper.

        With tighter constraints on inputs can squeeze carries into int32.

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldMultiply(FieldElementT *pOutput, const FieldElementT *pLhs, const FieldElementT *pRhs)
{
    int32_t iLhs0 = pLhs->aData[0];
    int32_t iLhs1 = pLhs->aData[1];
    int32_t iLhs2 = pLhs->aData[2];
    int32_t iLhs3 = pLhs->aData[3];
    int32_t iLhs4 = pLhs->aData[4];
    int32_t iLhs5 = pLhs->aData[5];
    int32_t iLhs6 = pLhs->aData[6];
    int32_t iLhs7 = pLhs->aData[7];
    int32_t iLhs8 = pLhs->aData[8];
    int32_t iLhs9 = pLhs->aData[9];
    int32_t iRhs0 = pRhs->aData[0];
    int32_t iRhs1 = pRhs->aData[1];
    int32_t iRhs2 = pRhs->aData[2];
    int32_t iRhs3 = pRhs->aData[3];
    int32_t iRhs4 = pRhs->aData[4];
    int32_t iRhs5 = pRhs->aData[5];
    int32_t iRhs6 = pRhs->aData[6];
    int32_t iRhs7 = pRhs->aData[7];
    int32_t iRhs8 = pRhs->aData[8];
    int32_t iRhs9 = pRhs->aData[9];
    int32_t iRhs1_19 = 19 * iRhs1; /* 1.959375*2^29 */
    int32_t iRhs2_19 = 19 * iRhs2; /* 1.959375*2^30; still ok */
    int32_t iRhs3_19 = 19 * iRhs3;
    int32_t iRhs4_19 = 19 * iRhs4;
    int32_t iRhs5_19 = 19 * iRhs5;
    int32_t iRhs6_19 = 19 * iRhs6;
    int32_t iRhs7_19 = 19 * iRhs7;
    int32_t iRhs8_19 = 19 * iRhs8;
    int32_t iRhs9_19 = 19 * iRhs9;
    int32_t iLhs1_2 = 2 * iLhs1;
    int32_t iLhs3_2 = 2 * iLhs3;
    int32_t iLhs5_2 = 2 * iLhs5;
    int32_t iLhs7_2 = 2 * iLhs7;
    int32_t iLhs9_2 = 2 * iLhs9;
    int64_t iLhs0Rhs0 = iLhs0 * (int64_t) iRhs0;
    int64_t iLhs0Rhs1 = iLhs0 * (int64_t) iRhs1;
    int64_t iLhs0Rhs2 = iLhs0 * (int64_t) iRhs2;
    int64_t iLhs0Rhs3 = iLhs0 * (int64_t) iRhs3;
    int64_t iLhs0Rhs4 = iLhs0 * (int64_t) iRhs4;
    int64_t iLhs0Rhs5 = iLhs0 * (int64_t) iRhs5;
    int64_t iLhs0Rhs6 = iLhs0 * (int64_t) iRhs6;
    int64_t iLhs0Rhs7 = iLhs0 * (int64_t) iRhs7;
    int64_t iLhs0Rhs8 = iLhs0 * (int64_t) iRhs8;
    int64_t iLhs0Rhs9 = iLhs0 * (int64_t) iRhs9;
    int64_t iLhs1Rhs0 = iLhs1 * (int64_t) iRhs0;
    int64_t iLhs1Rhs1_2 = iLhs1_2 * (int64_t) iRhs1;
    int64_t iLhs1Rhs2 = iLhs1 * (int64_t) iRhs2;
    int64_t iLhs1Rhs3_2 = iLhs1_2 * (int64_t) iRhs3;
    int64_t iLhs1Rhs4 = iLhs1 * (int64_t) iRhs4;
    int64_t iLhs1Rhs5_2 = iLhs1_2 * (int64_t) iRhs5;
    int64_t iLhs1Rhs6 = iLhs1 * (int64_t) iRhs6;
    int64_t iLhs1Rhs7_2 = iLhs1_2 * (int64_t) iRhs7;
    int64_t iLhs1Rhs8 = iLhs1 * (int64_t) iRhs8;
    int64_t iLhs1Rhs9_38 = iLhs1_2 * (int64_t) iRhs9_19;
    int64_t iLhs2Rhs0 = iLhs2 * (int64_t) iRhs0;
    int64_t iLhs2Rhs1 = iLhs2 * (int64_t) iRhs1;
    int64_t iLhs2Rhs2 = iLhs2 * (int64_t) iRhs2;
    int64_t iLhs2Rhs3 = iLhs2 * (int64_t) iRhs3;
    int64_t iLhs2Rhs4 = iLhs2 * (int64_t) iRhs4;
    int64_t iLhs2Rhs5 = iLhs2 * (int64_t) iRhs5;
    int64_t iLhs2Rhs6 = iLhs2 * (int64_t) iRhs6;
    int64_t iLhs2Rhs7 = iLhs2 * (int64_t) iRhs7;
    int64_t iLhs2Rhs8_19 = iLhs2 * (int64_t) iRhs8_19;
    int64_t iLhs2Rhs9_19 = iLhs2 * (int64_t) iRhs9_19;
    int64_t iLhs3Rhs0 = iLhs3 * (int64_t) iRhs0;
    int64_t iLhs3Rhs1_2 = iLhs3_2 * (int64_t) iRhs1;
    int64_t iLhs3Rhs2 = iLhs3 * (int64_t) iRhs2;
    int64_t iLhs3Rhs3_2 = iLhs3_2 * (int64_t) iRhs3;
    int64_t iLhs3Rhs4 = iLhs3 * (int64_t) iRhs4;
    int64_t iLhs3Rhs5_2 = iLhs3_2 * (int64_t) iRhs5;
    int64_t iLhs3Rhs6 = iLhs3 * (int64_t) iRhs6;
    int64_t iLhs3Rhs7_38 = iLhs3_2 * (int64_t) iRhs7_19;
    int64_t iLhs3Rhs8_19 = iLhs3 * (int64_t) iRhs8_19;
    int64_t iLhs3Rhs9_38 = iLhs3_2 * (int64_t) iRhs9_19;
    int64_t iLhs4Rhs0 = iLhs4 * (int64_t) iRhs0;
    int64_t iLhs4Rhs1 = iLhs4 * (int64_t) iRhs1;
    int64_t iLhs4Rhs2 = iLhs4 * (int64_t) iRhs2;
    int64_t iLhs4Rhs3 = iLhs4 * (int64_t) iRhs3;
    int64_t iLhs4Rhs4 = iLhs4 * (int64_t) iRhs4;
    int64_t iLhs4Rhs5 = iLhs4 * (int64_t) iRhs5;
    int64_t iLhs4Rhs6_19 = iLhs4 * (int64_t) iRhs6_19;
    int64_t iLhs4Rhs7_19 = iLhs4 * (int64_t) iRhs7_19;
    int64_t iLhs4Rhs8_19 = iLhs4 * (int64_t) iRhs8_19;
    int64_t iLhs4Rhs9_19 = iLhs4 * (int64_t) iRhs9_19;
    int64_t iLhs5Rhs0 = iLhs5 * (int64_t) iRhs0;
    int64_t iLhs5Rhs1_2 = iLhs5_2 * (int64_t) iRhs1;
    int64_t iLhs5Rhs2 = iLhs5 * (int64_t) iRhs2;
    int64_t iLhs5Rhs3_2 = iLhs5_2 * (int64_t) iRhs3;
    int64_t iLhs5Rhs4 = iLhs5 * (int64_t) iRhs4;
    int64_t iLhs5Rhs5_38 = iLhs5_2 * (int64_t) iRhs5_19;
    int64_t iLhs5Rhs6_19 = iLhs5 * (int64_t) iRhs6_19;
    int64_t iLhs5Rhs7_38 = iLhs5_2 * (int64_t) iRhs7_19;
    int64_t iLhs5Rhs8_19 = iLhs5 * (int64_t) iRhs8_19;
    int64_t iLhs5Rhs9_38 = iLhs5_2 * (int64_t) iRhs9_19;
    int64_t iLhs6Rhs0 = iLhs6 * (int64_t) iRhs0;
    int64_t iLhs6Rhs1 = iLhs6 * (int64_t) iRhs1;
    int64_t iLhs6Rhs2 = iLhs6 * (int64_t) iRhs2;
    int64_t iLhs6Rhs3 = iLhs6 * (int64_t) iRhs3;
    int64_t iLhs6Rhs4_19 = iLhs6 * (int64_t) iRhs4_19;
    int64_t iLhs6Rhs5_19 = iLhs6 * (int64_t) iRhs5_19;
    int64_t iLhs6Rhs6_19 = iLhs6 * (int64_t) iRhs6_19;
    int64_t iLhs6Rhs7_19 = iLhs6 * (int64_t) iRhs7_19;
    int64_t iLhs6Rhs8_19 = iLhs6 * (int64_t) iRhs8_19;
    int64_t iLhs6Rhs9_19 = iLhs6 * (int64_t) iRhs9_19;
    int64_t iLhs7Rhs0 = iLhs7 * (int64_t) iRhs0;
    int64_t iLhs7Rhs1_2 = iLhs7_2 * (int64_t) iRhs1;
    int64_t iLhs7Rhs2 = iLhs7 * (int64_t) iRhs2;
    int64_t iLhs7Rhs3_38 = iLhs7_2 * (int64_t) iRhs3_19;
    int64_t iLhs7Rhs4_19 = iLhs7 * (int64_t) iRhs4_19;
    int64_t iLhs7Rhs5_38 = iLhs7_2 * (int64_t) iRhs5_19;
    int64_t iLhs7Rhs6_19 = iLhs7 * (int64_t) iRhs6_19;
    int64_t iLhs7Rhs7_38 = iLhs7_2 * (int64_t) iRhs7_19;
    int64_t iLhs7Rhs8_19 = iLhs7 * (int64_t) iRhs8_19;
    int64_t iLhs7Rhs9_38 = iLhs7_2 * (int64_t) iRhs9_19;
    int64_t iLhs8Rhs0 = iLhs8 * (int64_t) iRhs0;
    int64_t iLhs8Rhs1 = iLhs8 * (int64_t) iRhs1;
    int64_t iLhs8Rhs2_19 = iLhs8 * (int64_t) iRhs2_19;
    int64_t iLhs8Rhs3_19 = iLhs8 * (int64_t) iRhs3_19;
    int64_t iLhs8Rhs4_19 = iLhs8 * (int64_t) iRhs4_19;
    int64_t iLhs8Rhs5_19 = iLhs8 * (int64_t) iRhs5_19;
    int64_t iLhs8Rhs6_19 = iLhs8 * (int64_t) iRhs6_19;
    int64_t iLhs8Rhs7_19 = iLhs8 * (int64_t) iRhs7_19;
    int64_t iLhs8Rhs8_19 = iLhs8 * (int64_t) iRhs8_19;
    int64_t iLhs8Rhs9_19 = iLhs8 * (int64_t) iRhs9_19;
    int64_t iLhs9Rhs0 = iLhs9 * (int64_t) iRhs0;
    int64_t iLhs9Rhs1_38 = iLhs9_2 * (int64_t) iRhs1_19;
    int64_t iLhs9Rhs2_19 = iLhs9   * (int64_t) iRhs2_19;
    int64_t iLhs9Rhs3_38 = iLhs9_2 * (int64_t) iRhs3_19;
    int64_t iLhs9Rhs4_19 = iLhs9   * (int64_t) iRhs4_19;
    int64_t iLhs9Rhs5_38 = iLhs9_2 * (int64_t) iRhs5_19;
    int64_t iLhs9Rhs6_19 = iLhs9   * (int64_t) iRhs6_19;
    int64_t iLhs9Rhs7_38 = iLhs9_2 * (int64_t) iRhs7_19;
    int64_t iLhs9Rhs8_19 = iLhs9   * (int64_t) iRhs8_19;
    int64_t iLhs9Rhs9_38 = iLhs9_2 * (int64_t) iRhs9_19;
    int64_t iOutput0 = iLhs0Rhs0+iLhs1Rhs9_38+iLhs2Rhs8_19+iLhs3Rhs7_38+iLhs4Rhs6_19+iLhs5Rhs5_38+iLhs6Rhs4_19+iLhs7Rhs3_38+iLhs8Rhs2_19+iLhs9Rhs1_38;
    int64_t iOutput1 = iLhs0Rhs1+iLhs1Rhs0+iLhs2Rhs9_19+iLhs3Rhs8_19+iLhs4Rhs7_19+iLhs5Rhs6_19+iLhs6Rhs5_19+iLhs7Rhs4_19+iLhs8Rhs3_19+iLhs9Rhs2_19;
    int64_t iOutput2 = iLhs0Rhs2+iLhs1Rhs1_2+iLhs2Rhs0+iLhs3Rhs9_38+iLhs4Rhs8_19+iLhs5Rhs7_38+iLhs6Rhs6_19+iLhs7Rhs5_38+iLhs8Rhs4_19+iLhs9Rhs3_38;
    int64_t iOutput3 = iLhs0Rhs3+iLhs1Rhs2+iLhs2Rhs1+iLhs3Rhs0+iLhs4Rhs9_19+iLhs5Rhs8_19+iLhs6Rhs7_19+iLhs7Rhs6_19+iLhs8Rhs5_19+iLhs9Rhs4_19;
    int64_t iOutput4 = iLhs0Rhs4+iLhs1Rhs3_2+iLhs2Rhs2+iLhs3Rhs1_2+iLhs4Rhs0+iLhs5Rhs9_38+iLhs6Rhs8_19+iLhs7Rhs7_38+iLhs8Rhs6_19+iLhs9Rhs5_38;
    int64_t iOutput5 = iLhs0Rhs5+iLhs1Rhs4+iLhs2Rhs3+iLhs3Rhs2+iLhs4Rhs1+iLhs5Rhs0+iLhs6Rhs9_19+iLhs7Rhs8_19+iLhs8Rhs7_19+iLhs9Rhs6_19;
    int64_t iOutput6 = iLhs0Rhs6+iLhs1Rhs5_2+iLhs2Rhs4+iLhs3Rhs3_2+iLhs4Rhs2+iLhs5Rhs1_2+iLhs6Rhs0+iLhs7Rhs9_38+iLhs8Rhs8_19+iLhs9Rhs7_38;
    int64_t iOutput7 = iLhs0Rhs7+iLhs1Rhs6+iLhs2Rhs5+iLhs3Rhs4+iLhs4Rhs3+iLhs5Rhs2+iLhs6Rhs1+iLhs7Rhs0+iLhs8Rhs9_19+iLhs9Rhs8_19;
    int64_t iOutput8 = iLhs0Rhs8+iLhs1Rhs7_2+iLhs2Rhs6+iLhs3Rhs5_2+iLhs4Rhs4+iLhs5Rhs3_2+iLhs6Rhs2+iLhs7Rhs1_2+iLhs8Rhs0+iLhs9Rhs9_38;
    int64_t iOutput9 = iLhs0Rhs9+iLhs1Rhs8+iLhs2Rhs7+iLhs3Rhs6+iLhs4Rhs5+iLhs5Rhs4+iLhs6Rhs3+iLhs7Rhs2+iLhs8Rhs1+iLhs9Rhs0;
    int64_t iCarry0, iCarry1, iCarry2, iCarry3, iCarry4, iCarry5, iCarry6, iCarry7, iCarry8, iCarry9;

    /*
       |iOutput0| <= (1.65*1.65*2^52*(1+19+19+19+19)+1.65*1.65*2^50*(38+38+38+38+38))
       i.e. |iOutput0| <= 1.4*2^60; narrower ranges for iOutput2, iOutput4, iOutput6, iOutput8
       |iOutput1| <= (1.65*1.65*2^51*(1+1+19+19+19+19+19+19+19+19))
       i.e. |iOutput1| <= 1.7*2^59; narrower ranges for iOutput3, iOutput5, iOutput7, iOutput9
       */

    iCarry0 = (iOutput0 + (int64_t) (1<<25)) >> 26;
    iOutput1 += iCarry0;
    iOutput0 -= iCarry0 << 26;

    iCarry4 = (iOutput4 + (int64_t) (1<<25)) >> 26;
    iOutput5 += iCarry4;
    iOutput4 -= iCarry4 << 26;
    /* |iOutput0| <= 2^25 */
    /* |iOutput4| <= 2^25 */
    /* |iOutput1| <= 1.71*2^59 */
    /* |iOutput5| <= 1.71*2^59 */

    iCarry1 = (iOutput1 + (int64_t) (1<<24)) >> 25;
    iOutput2 += iCarry1;
    iOutput1 -= iCarry1 << 25;

    iCarry5 = (iOutput5 + (int64_t) (1<<24)) >> 25;
    iOutput6 += iCarry5;
    iOutput5 -= iCarry5 << 25;
    /* |iOutput1| <= 2^24; from now on fits into int32 */
    /* |iOutput5| <= 2^24; from now on fits into int32 */
    /* |iOutput2| <= 1.41*2^60 */
    /* |iOutput6| <= 1.41*2^60 */

    iCarry2 = (iOutput2 + (int64_t) (1<<25)) >> 26;
    iOutput3 += iCarry2;
    iOutput2 -= iCarry2 << 26;

    iCarry6 = (iOutput6 + (int64_t) (1<<25)) >> 26;
    iOutput7 += iCarry6;
    iOutput6 -= iCarry6 << 26;
    /* |iOutput2| <= 2^25; from now on fits into int32 unchanged */
    /* |iOutput6| <= 2^25; from now on fits into int32 unchanged */
    /* |iOutput3| <= 1.71*2^59 */
    /* |iOutput7| <= 1.71*2^59 */

    iCarry3 = (iOutput3 + (int64_t) (1<<24)) >> 25;
    iOutput4 += iCarry3;
    iOutput3 -= iCarry3 << 25;

    iCarry7 = (iOutput7 + (int64_t) (1<<24)) >> 25;
    iOutput8 += iCarry7;
    iOutput7 -= iCarry7 << 25;

    /* |iOutput3| <= 2^24; from now on fits into int32 unchanged */
    /* |iOutput7| <= 2^24; from now on fits into int32 unchanged */
    /* |iOutput4| <= 1.72*2^34 */
    /* |iOutput8| <= 1.41*2^60 */

    iCarry4 = (iOutput4 + (int64_t) (1<<25)) >> 26;
    iOutput5 += iCarry4;
    iOutput4 -= iCarry4 << 26;

    iCarry8 = (iOutput8 + (int64_t) (1<<25)) >> 26;
    iOutput9 += iCarry8;
    iOutput8 -= iCarry8 << 26;
    /* |iOutput4| <= 2^25; from now on fits into int32 unchanged */
    /* |iOutput8| <= 2^25; from now on fits into int32 unchanged */
    /* |iOutput5| <= 1.01*2^24 */
    /* |iOutput9| <= 1.71*2^59 */

    iCarry9 = (iOutput9 + (int64_t) (1<<24)) >> 25;
    iOutput0 += iCarry9 * 19;
    iOutput9 -= iCarry9 << 25;
    /* |iOutput9| <= 2^24; from now on fits into int32 unchanged */
    /* |iOutput0| <= 1.1*2^39 */

    iCarry0 = (iOutput0 + (int64_t) (1<<25)) >> 26;
    iOutput1 += iCarry0;
    iOutput0 -= iCarry0 << 26;
    /* |iOutput0| <= 2^25; from now on fits into int32 unchanged */
    /* |iOutput1| <= 1.01*2^24 */

    pOutput->aData[0] = (int32_t)iOutput0;
    pOutput->aData[1] = (int32_t)iOutput1;
    pOutput->aData[2] = (int32_t)iOutput2;
    pOutput->aData[3] = (int32_t)iOutput3;
    pOutput->aData[4] = (int32_t)iOutput4;
    pOutput->aData[5] = (int32_t)iOutput5;
    pOutput->aData[6] = (int32_t)iOutput6;
    pOutput->aData[7] = (int32_t)iOutput7;
    pOutput->aData[8] = (int32_t)iOutput8;
    pOutput->aData[9] = (int32_t)iOutput9;
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldSquare

    \Description
        Performs output = input * input

    \Input pOutput  - [out] output of the operation
    \Input pInput   - input to square

    \Notes
        Inputs are bounded by 1.65*2^26,1.65*2^25,1.65*2^26,1.65*2^25,etc.
        Output is bounded by 1.01*2^25,1.01*2^24,1.01*2^25,1.01*2^24,etc.
        Output can overlap with input.

        See Crypt25519FieldMultiply for info about the implementation strategy

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldSquare(FieldElementT *pOutput, const FieldElementT *pInput)
{
    int32_t iInput0 = pInput->aData[0];
    int32_t iInput1 = pInput->aData[1];
    int32_t iInput2 = pInput->aData[2];
    int32_t iInput3 = pInput->aData[3];
    int32_t iInput4 = pInput->aData[4];
    int32_t iInput5 = pInput->aData[5];
    int32_t iInput6 = pInput->aData[6];
    int32_t iInput7 = pInput->aData[7];
    int32_t iInput8 = pInput->aData[8];
    int32_t iInput9 = pInput->aData[9];
    int32_t iInput0_2 = 2 * iInput0;
    int32_t iInput1_2 = 2 * iInput1;
    int32_t iInput2_2 = 2 * iInput2;
    int32_t iInput3_2 = 2 * iInput3;
    int32_t iInput4_2 = 2 * iInput4;
    int32_t iInput5_2 = 2 * iInput5;
    int32_t iInput6_2 = 2 * iInput6;
    int32_t iInput7_2 = 2 * iInput7;
    int32_t iInput5_38 = 38 * iInput5; /* 1.959375*2^30 */
    int32_t iInput6_19 = 19 * iInput6; /* 1.959375*2^30 */
    int32_t iInput7_38 = 38 * iInput7; /* 1.959375*2^30 */
    int32_t iInput8_19 = 19 * iInput8; /* 1.959375*2^30 */
    int32_t iInput9_38 = 38 * iInput9; /* 1.959375*2^30 */
    int64_t iInput00 = iInput0 * (int64_t)iInput0;
    int64_t iInput01_2 = iInput0_2 * (int64_t) iInput1;
    int64_t iInput02_2 = iInput0_2 * (int64_t) iInput2;
    int64_t iInput03_2 = iInput0_2 * (int64_t) iInput3;
    int64_t iInput04_2 = iInput0_2 * (int64_t) iInput4;
    int64_t iInput05_2 = iInput0_2 * (int64_t) iInput5;
    int64_t iInput06_2 = iInput0_2 * (int64_t) iInput6;
    int64_t iInput07_2 = iInput0_2 * (int64_t) iInput7;
    int64_t iInput08_2 = iInput0_2 * (int64_t) iInput8;
    int64_t iInput09_2 = iInput0_2 * (int64_t) iInput9;
    int64_t iInput11_2 = iInput1_2 * (int64_t) iInput1;
    int64_t iInput12_2 = iInput1_2 * (int64_t) iInput2;
    int64_t iInput13_4 = iInput1_2 * (int64_t) iInput3_2;
    int64_t iInput14_2 = iInput1_2 * (int64_t) iInput4;
    int64_t iInput15_4 = iInput1_2 * (int64_t) iInput5_2;
    int64_t iInput16_2 = iInput1_2 * (int64_t) iInput6;
    int64_t iInput17_4 = iInput1_2 * (int64_t) iInput7_2;
    int64_t iInput18_2 = iInput1_2 * (int64_t) iInput8;
    int64_t iInput19_76 = iInput1_2 * (int64_t) iInput9_38;
    int64_t iInput22 = iInput2 * (int64_t) iInput2;
    int64_t iInput23_2 = iInput2_2 * (int64_t) iInput3;
    int64_t iInput24_2 = iInput2_2 * (int64_t) iInput4;
    int64_t iInput25_2 = iInput2_2 * (int64_t) iInput5;
    int64_t iInput26_2 = iInput2_2 * (int64_t) iInput6;
    int64_t iInput27_2 = iInput2_2 * (int64_t) iInput7;
    int64_t iInput28_38 = iInput2_2 * (int64_t) iInput8_19;
    int64_t iInput29_38 = iInput2 * (int64_t) iInput9_38;
    int64_t iInput33_2 = iInput3_2 * (int64_t) iInput3;
    int64_t iInput34_2 = iInput3_2 * (int64_t) iInput4;
    int64_t iInput35_4 = iInput3_2 * (int64_t) iInput5_2;
    int64_t iInput36_2 = iInput3_2 * (int64_t) iInput6;
    int64_t iInput37_76 = iInput3_2 * (int64_t) iInput7_38;
    int64_t iInput38_38 = iInput3_2 * (int64_t) iInput8_19;
    int64_t iInput39_76 = iInput3_2 * (int64_t) iInput9_38;
    int64_t iInput44 = iInput4 * (int64_t) iInput4;
    int64_t iInput45_2 = iInput4_2 * (int64_t) iInput5;
    int64_t iInput46_38 = iInput4_2 * (int64_t) iInput6_19;
    int64_t iInput47_38 = iInput4 * (int64_t) iInput7_38;
    int64_t iInput48_38 = iInput4_2 * (int64_t) iInput8_19;
    int64_t iInput49_38 = iInput4 * (int64_t) iInput9_38;
    int64_t iInput55_38 = iInput5 * (int64_t) iInput5_38;
    int64_t iInput56_38 = iInput5_2 * (int64_t) iInput6_19;
    int64_t iInput57_76 = iInput5_2 * (int64_t) iInput7_38;
    int64_t iInput58_38 = iInput5_2 * (int64_t) iInput8_19;
    int64_t iInput59_76 = iInput5_2 * (int64_t) iInput9_38;
    int64_t iInput66_19 = iInput6 * (int64_t) iInput6_19;
    int64_t iInput67_38 = iInput6 * (int64_t) iInput7_38;
    int64_t iInput68_38 = iInput6_2 * (int64_t) iInput8_19;
    int64_t iInput69_38 = iInput6 * (int64_t) iInput9_38;
    int64_t iInput77_38 = iInput7 * (int64_t) iInput7_38;
    int64_t iInput78_38 = iInput7_2 * (int64_t) iInput8_19;
    int64_t iInput79_76 = iInput7_2 * (int64_t) iInput9_38;
    int64_t iInput88_19 = iInput8 * (int64_t) iInput8_19;
    int64_t iInput89_38 = iInput8 * (int64_t) iInput9_38;
    int64_t iInput99_38 = iInput9 * (int64_t) iInput9_38;
    int64_t iOutput0 = iInput00+iInput19_76+iInput28_38+iInput37_76+iInput46_38+iInput55_38;
    int64_t iOutput1 = iInput01_2+iInput29_38+iInput38_38+iInput47_38+iInput56_38;
    int64_t iOutput2 = iInput02_2+iInput11_2+iInput39_76+iInput48_38+iInput57_76+iInput66_19;
    int64_t iOutput3 = iInput03_2+iInput12_2+iInput49_38+iInput58_38+iInput67_38;
    int64_t iOutput4 = iInput04_2+iInput13_4+iInput22+iInput59_76+iInput68_38+iInput77_38;
    int64_t iOutput5 = iInput05_2+iInput14_2+iInput23_2+iInput69_38+iInput78_38;
    int64_t iOutput6 = iInput06_2+iInput15_4+iInput24_2+iInput33_2+iInput79_76+iInput88_19;
    int64_t iOutput7 = iInput07_2+iInput16_2+iInput25_2+iInput34_2+iInput89_38;
    int64_t iOutput8 = iInput08_2+iInput17_4+iInput26_2+iInput35_4+iInput44+iInput99_38;
    int64_t iOutput9 = iInput09_2+iInput18_2+iInput27_2+iInput36_2+iInput45_2;
    int64_t iCarry0, iCarry1, iCarry2, iCarry3, iCarry4, iCarry5, iCarry6, iCarry7, iCarry8, iCarry9;

    iCarry0 = (iOutput0 + (int64_t) (1<<25)) >> 26;
    iOutput1 += iCarry0;
    iOutput0 -= iCarry0 << 26;

    iCarry4 = (iOutput4 + (int64_t) (1<<25)) >> 26;
    iOutput5 += iCarry4;
    iOutput4 -= iCarry4 << 26;

    iCarry1 = (iOutput1 + (int64_t) (1<<24)) >> 25;
    iOutput2 += iCarry1;
    iOutput1 -= iCarry1 << 25;

    iCarry5 = (iOutput5 + (int64_t) (1<<24)) >> 25;
    iOutput6 += iCarry5;
    iOutput5 -= iCarry5 << 25;

    iCarry2 = (iOutput2 + (int64_t) (1<<25)) >> 26;
    iOutput3 += iCarry2;
    iOutput2 -= iCarry2 << 26;

    iCarry6 = (iOutput6 + (int64_t) (1<<25)) >> 26;
    iOutput7 += iCarry6;
    iOutput6 -= iCarry6 << 26;

    iCarry3 = (iOutput3 + (int64_t) (1<<24)) >> 25;
    iOutput4 += iCarry3;
    iOutput3 -= iCarry3 << 25;

    iCarry7 = (iOutput7 + (int64_t) (1<<24)) >> 25;
    iOutput8 += iCarry7;
    iOutput7 -= iCarry7 << 25;

    iCarry4 = (iOutput4 + (int64_t) (1<<25)) >> 26;
    iOutput5 += iCarry4;
    iOutput4 -= iCarry4 << 26;

    iCarry8 = (iOutput8 + (int64_t) (1<<25)) >> 26;
    iOutput9 += iCarry8;
    iOutput8 -= iCarry8 << 26;

    iCarry9 = (iOutput9 + (int64_t) (1<<24)) >> 25;
    iOutput0 += iCarry9 * 19;
    iOutput9 -= iCarry9 << 25;

    iCarry0 = (iOutput0 + (int64_t) (1<<25)) >> 26;
    iOutput1 += iCarry0;
    iOutput0 -= iCarry0 << 26;

    pOutput->aData[0] = (int32_t)iOutput0;
    pOutput->aData[1] = (int32_t)iOutput1;
    pOutput->aData[2] = (int32_t)iOutput2;
    pOutput->aData[3] = (int32_t)iOutput3;
    pOutput->aData[4] = (int32_t)iOutput4;
    pOutput->aData[5] = (int32_t)iOutput5;
    pOutput->aData[6] = (int32_t)iOutput6;
    pOutput->aData[7] = (int32_t)iOutput7;
    pOutput->aData[8] = (int32_t)iOutput8;
    pOutput->aData[9] = (int32_t)iOutput9;
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldInvert

    \Description
        Performs modular invert of the input

    \Input pOutput  - [out] output of the operation
    \Input pZ       - input to invert

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldInvert(FieldElementT *pOutput, const FieldElementT *pZ)
{
    FieldElementT T0, T1, T2, T3;
    int32_t iIndex;

    /* z2 = z1^2^1 */
    Crypt25519FieldSquare(&T0, pZ);

    /* z8 = z2^2^2 */
    Crypt25519FieldSquare(&T1, &T0);
    for (iIndex = 1; iIndex < 2; iIndex += 1)
    {
        Crypt25519FieldSquare(&T1, &T1);
    }

    /* z9 = z1*z8 */
    Crypt25519FieldMultiply(&T1, pZ, &T1);

    /* z11 = z2*z9 */
    Crypt25519FieldMultiply(&T0, &T0, &T1);

    /* z22 = z11^2 */
    Crypt25519FieldSquare(&T2, &T0);

    /* z_5_0 = z9*z22 */
    Crypt25519FieldMultiply(&T1, &T1, &T2);

    /* z_10_5 = z_5_0^2^5 */
    Crypt25519FieldSquare(&T2, &T1);
    for (iIndex = 1; iIndex < 5; iIndex += 1)
    {
        Crypt25519FieldSquare(&T2, &T2);
    }

    /* z_10_0 = z_10_5*z_5_0 */
    Crypt25519FieldMultiply(&T1, &T2, &T1);

    /* z_20_10 = z_10_0^2^10 */
    Crypt25519FieldSquare(&T2, &T1);
    for (iIndex = 1; iIndex < 10; iIndex += 1)
    {
        Crypt25519FieldSquare(&T2, &T2);
    }

    /* z_20_0 = z_20_10*z_10_0 */
    Crypt25519FieldMultiply(&T2, &T2, &T1);

    /* z_40_20 = z_20_0^2^20 */
    Crypt25519FieldSquare(&T3, &T2);
    for (iIndex = 1; iIndex < 20; iIndex += 1)
    {
        Crypt25519FieldSquare(&T3, &T3);
    }

    /* z_40_0 = z_40_20*z_20_0 */
    Crypt25519FieldMultiply(&T2, &T3, &T2);

    /* z_50_10 = z_40_0^2^10 */
    Crypt25519FieldSquare(&T2, &T2);
    for (iIndex = 1; iIndex < 10; iIndex += 1)
    {
        Crypt25519FieldSquare(&T2, &T2);
    }

    /* z_50_0 = z_50_10*z_10_0 */
    Crypt25519FieldMultiply(&T1, &T2, &T1);

    /* z_100_50 = z_50_0^2^50 */
    Crypt25519FieldSquare(&T2, &T1);
    for (iIndex = 1; iIndex < 50; iIndex += 1)
    {
        Crypt25519FieldSquare(&T2, &T2);
    }

    /* z_100_0 = z_100_50*z_50_0 */
    Crypt25519FieldMultiply(&T2, &T2, &T1);

    /* z_200_100 = z_100_0^2^100 */
    Crypt25519FieldSquare(&T3, &T2);
    for (iIndex = 1; iIndex < 100; iIndex += 1)
    {
        Crypt25519FieldSquare(&T3, &T3);
    }

    /* z_200_0 = z_200_100*z_100_0 */
    Crypt25519FieldMultiply(&T2, &T3, &T2);

    /* z_250_50 = z_200_0^2^50 */
    Crypt25519FieldSquare(&T2, &T2);
    for (iIndex = 1; iIndex < 50; iIndex += 1)
    {
        Crypt25519FieldSquare(&T2, &T2);
    }

    /* z_250_0 = z_250_50*z_50_0 */
    Crypt25519FieldMultiply(&T1, &T2, &T1);

    /* z_255_5 = z_250_0^2^5 */
    Crypt25519FieldSquare(&T1, &T1);
    for (iIndex = 1; iIndex < 5; iIndex += 1)
    {
        Crypt25519FieldSquare(&T1, &T1);
    }

    /* z_255_21 = z_255_5*z11 */
    Crypt25519FieldMultiply(pOutput, &T1, &T0);
}


/*F********************************************************************************/
/*!
    \Function Crypt25519FieldDoubleSquare

    \Description
        Performs output = 2 * input * input

    \Input pOutput  - [out] output of the operation
    \Input pInput   - input to square

    \Notes
        Inputs are bounded by 1.65*2^26,1.65*2^25,1.65*2^26,1.65*2^25,etc.
        Output is bounded by 1.01*2^25,1.01*2^24,1.01*2^25,1.01*2^24,etc.
        Output can overlap with input.

        See Crypt25519FieldMultiply for info about the implementation strategy

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldDoubleSquare(FieldElementT *pOutput, const FieldElementT *pInput)
{
    int32_t iInput0 = pInput->aData[0];
    int32_t iInput1 = pInput->aData[1];
    int32_t iInput2 = pInput->aData[2];
    int32_t iInput3 = pInput->aData[3];
    int32_t iInput4 = pInput->aData[4];
    int32_t iInput5 = pInput->aData[5];
    int32_t iInput6 = pInput->aData[6];
    int32_t iInput7 = pInput->aData[7];
    int32_t iInput8 = pInput->aData[8];
    int32_t iInput9 = pInput->aData[9];
    int32_t iInput0_2 = 2 * iInput0;
    int32_t iInput1_2 = 2 * iInput1;
    int32_t iInput2_2 = 2 * iInput2;
    int32_t iInput3_2 = 2 * iInput3;
    int32_t iInput4_2 = 2 * iInput4;
    int32_t iInput5_2 = 2 * iInput5;
    int32_t iInput6_2 = 2 * iInput6;
    int32_t iInput7_2 = 2 * iInput7;
    int32_t iInput5_38 = 38 * iInput5; /* 1.959375*2^30 */
    int32_t iInput6_19 = 19 * iInput6; /* 1.959375*2^30 */
    int32_t iInput7_38 = 38 * iInput7; /* 1.959375*2^30 */
    int32_t iInput8_19 = 19 * iInput8; /* 1.959375*2^30 */
    int32_t iInput9_38 = 38 * iInput9; /* 1.959375*2^30 */
    int64_t iInput00    = iInput0   * (int64_t) iInput0;
    int64_t iInput01_2  = iInput0_2 * (int64_t) iInput1;
    int64_t iInput02_2  = iInput0_2 * (int64_t) iInput2;
    int64_t iInput03_2  = iInput0_2 * (int64_t) iInput3;
    int64_t iInput04_2  = iInput0_2 * (int64_t) iInput4;
    int64_t iInput05_2  = iInput0_2 * (int64_t) iInput5;
    int64_t iInput06_2  = iInput0_2 * (int64_t) iInput6;
    int64_t iInput07_2  = iInput0_2 * (int64_t) iInput7;
    int64_t iInput08_2  = iInput0_2 * (int64_t) iInput8;
    int64_t iInput09_2  = iInput0_2 * (int64_t) iInput9;
    int64_t iInput11_2  = iInput1_2 * (int64_t) iInput1;
    int64_t iInput12_2  = iInput1_2 * (int64_t) iInput2;
    int64_t iInput13_4  = iInput1_2 * (int64_t) iInput3_2;
    int64_t iInput14_2  = iInput1_2 * (int64_t) iInput4;
    int64_t iInput15_4  = iInput1_2 * (int64_t) iInput5_2;
    int64_t iInput16_2  = iInput1_2 * (int64_t) iInput6;
    int64_t iInput17_4  = iInput1_2 * (int64_t) iInput7_2;
    int64_t iInput18_2  = iInput1_2 * (int64_t) iInput8;
    int64_t iInput19_76 = iInput1_2 * (int64_t) iInput9_38;
    int64_t iInput22    = iInput2   * (int64_t) iInput2;
    int64_t iInput23_2  = iInput2_2 * (int64_t) iInput3;
    int64_t iInput24_2  = iInput2_2 * (int64_t) iInput4;
    int64_t iInput25_2  = iInput2_2 * (int64_t) iInput5;
    int64_t iInput26_2  = iInput2_2 * (int64_t) iInput6;
    int64_t iInput27_2  = iInput2_2 * (int64_t) iInput7;
    int64_t iInput28_38 = iInput2_2 * (int64_t) iInput8_19;
    int64_t iInput29_38 = iInput2   * (int64_t) iInput9_38;
    int64_t iInput33_2  = iInput3_2 * (int64_t) iInput3;
    int64_t iInput34_2  = iInput3_2 * (int64_t) iInput4;
    int64_t iInput35_4  = iInput3_2 * (int64_t) iInput5_2;
    int64_t iInput36_2  = iInput3_2 * (int64_t) iInput6;
    int64_t iInput37_76 = iInput3_2 * (int64_t) iInput7_38;
    int64_t iInput38_38 = iInput3_2 * (int64_t) iInput8_19;
    int64_t iInput39_76 = iInput3_2 * (int64_t) iInput9_38;
    int64_t iInput44    = iInput4   * (int64_t) iInput4;
    int64_t iInput45_2  = iInput4_2 * (int64_t) iInput5;
    int64_t iInput46_38 = iInput4_2 * (int64_t) iInput6_19;
    int64_t iInput47_38 = iInput4   * (int64_t) iInput7_38;
    int64_t iInput48_38 = iInput4_2 * (int64_t) iInput8_19;
    int64_t iInput49_38 = iInput4   * (int64_t) iInput9_38;
    int64_t iInput55_38 = iInput5   * (int64_t) iInput5_38;
    int64_t iInput56_38 = iInput5_2 * (int64_t) iInput6_19;
    int64_t iInput57_76 = iInput5_2 * (int64_t) iInput7_38;
    int64_t iInput58_38 = iInput5_2 * (int64_t) iInput8_19;
    int64_t iInput59_76 = iInput5_2 * (int64_t) iInput9_38;
    int64_t iInput66_19 = iInput6   * (int64_t) iInput6_19;
    int64_t iInput67_38 = iInput6   * (int64_t) iInput7_38;
    int64_t iInput68_38 = iInput6_2 * (int64_t) iInput8_19;
    int64_t iInput69_38 = iInput6   * (int64_t) iInput9_38;
    int64_t iInput77_38 = iInput7   * (int64_t) iInput7_38;
    int64_t iInput78_38 = iInput7_2 * (int64_t) iInput8_19;
    int64_t iInput79_76 = iInput7_2 * (int64_t) iInput9_38;
    int64_t iInput88_19 = iInput8   * (int64_t) iInput8_19;
    int64_t iInput89_38 = iInput8   * (int64_t) iInput9_38;
    int64_t iInput99_38 = iInput9   * (int64_t) iInput9_38;
    int64_t iOutput0 = iInput00  +iInput19_76+iInput28_38+iInput37_76+iInput46_38+iInput55_38;
    int64_t iOutput1 = iInput01_2+iInput29_38+iInput38_38+iInput47_38+iInput56_38;
    int64_t iOutput2 = iInput02_2+iInput11_2 +iInput39_76+iInput48_38+iInput57_76+iInput66_19;
    int64_t iOutput3 = iInput03_2+iInput12_2 +iInput49_38+iInput58_38+iInput67_38;
    int64_t iOutput4 = iInput04_2+iInput13_4 +iInput22   +iInput59_76+iInput68_38+iInput77_38;
    int64_t iOutput5 = iInput05_2+iInput14_2 +iInput23_2 +iInput69_38+iInput78_38;
    int64_t iOutput6 = iInput06_2+iInput15_4 +iInput24_2 +iInput33_2 +iInput79_76+iInput88_19;
    int64_t iOutput7 = iInput07_2+iInput16_2 +iInput25_2 +iInput34_2 +iInput89_38;
    int64_t iOutput8 = iInput08_2+iInput17_4 +iInput26_2 +iInput35_4 +iInput44   +iInput99_38;
    int64_t iOutput9 = iInput09_2+iInput18_2 +iInput27_2 +iInput36_2 +iInput45_2;
    int64_t iCarry0, iCarry1, iCarry2, iCarry3, iCarry4, iCarry5, iCarry6, iCarry7, iCarry8, iCarry9;

    iOutput0 += iOutput0;
    iOutput1 += iOutput1;
    iOutput2 += iOutput2;
    iOutput3 += iOutput3;
    iOutput4 += iOutput4;
    iOutput5 += iOutput5;
    iOutput6 += iOutput6;
    iOutput7 += iOutput7;
    iOutput8 += iOutput8;
    iOutput9 += iOutput9;

    iCarry0 = (iOutput0 + (int64_t) (1<<25)) >> 26;
    iOutput1 += iCarry0;
    iOutput0 -= iCarry0 << 26;

    iCarry4 = (iOutput4 + (int64_t) (1<<25)) >> 26;
    iOutput5 += iCarry4;
    iOutput4 -= iCarry4 << 26;

    iCarry1 = (iOutput1 + (int64_t) (1<<24)) >> 25;
    iOutput2 += iCarry1;
    iOutput1 -= iCarry1 << 25;

    iCarry5 = (iOutput5 + (int64_t) (1<<24)) >> 25;
    iOutput6 += iCarry5;
    iOutput5 -= iCarry5 << 25;

    iCarry2 = (iOutput2 + (int64_t) (1<<25)) >> 26;
    iOutput3 += iCarry2;
    iOutput2 -= iCarry2 << 26;

    iCarry6 = (iOutput6 + (int64_t) (1<<25)) >> 26;
    iOutput7 += iCarry6;
    iOutput6 -= iCarry6 << 26;

    iCarry3 = (iOutput3 + (int64_t) (1<<24)) >> 25;
    iOutput4 += iCarry3;
    iOutput3 -= iCarry3 << 25;

    iCarry7 = (iOutput7 + (int64_t) (1<<24)) >> 25;
    iOutput8 += iCarry7;
    iOutput7 -= iCarry7 << 25;

    iCarry4 = (iOutput4 + (int64_t) (1<<25)) >> 26;
    iOutput5 += iCarry4;
    iOutput4 -= iCarry4 << 26;

    iCarry8 = (iOutput8 + (int64_t) (1<<25)) >> 26;
    iOutput9 += iCarry8;
    iOutput8 -= iCarry8 << 26;

    iCarry9 = (iOutput9 + (int64_t) (1<<24)) >> 25;
    iOutput0 += iCarry9 * 19;
    iOutput9 -= iCarry9 << 25;

    iCarry0 = (iOutput0 + (int64_t) (1<<25)) >> 26;
    iOutput1 += iCarry0;
    iOutput0 -= iCarry0 << 26;

    pOutput->aData[0] = (int32_t)iOutput0;
    pOutput->aData[1] = (int32_t)iOutput1;
    pOutput->aData[2] = (int32_t)iOutput2;
    pOutput->aData[3] = (int32_t)iOutput3;
    pOutput->aData[4] = (int32_t)iOutput4;
    pOutput->aData[5] = (int32_t)iOutput5;
    pOutput->aData[6] = (int32_t)iOutput6;
    pOutput->aData[7] = (int32_t)iOutput7;
    pOutput->aData[8] = (int32_t)iOutput8;
    pOutput->aData[9] = (int32_t)iOutput9;
}

/*F********************************************************************************/
/*!
    \Function Crypt25519FieldPow22523

    \Description
        Performs input ^ 225-23

    \Input pOutput  - [out] output of the operation
    \Input pZ       - input to raise

    \Version 05/19/2020 (eesponda)
*/
/********************************************************************************F*/
void Crypt25519FieldPow22523(FieldElementT *pOutput, const FieldElementT *pZ)
{
    FieldElementT Temp0, Temp1, Temp2;
    int32_t iIndex;

    /* z2 = z1^2 */
    Crypt25519FieldSquare(&Temp0, pZ);

    /* z8 = z2^2^2 */
    Crypt25519FieldSquare(&Temp1, &Temp0);
    Crypt25519FieldSquare(&Temp1, &Temp1);

    /* z9 = z1*z8 */
    Crypt25519FieldMultiply(&Temp1, pZ, &Temp1);

    /* z11 = z2*z9 */
    Crypt25519FieldMultiply(&Temp0, &Temp0, &Temp1);

    /* z22 = z11^2 */
    Crypt25519FieldSquare(&Temp0, &Temp0);

    /* z_5_0 = z9*z22 */
    Crypt25519FieldMultiply(&Temp0, &Temp1, &Temp0);

    /* z_10_5 = z_5_0^2^5 */
    Crypt25519FieldSquare(&Temp1, &Temp0);
    for (iIndex = 1; iIndex < 5; iIndex += 1)
    {
        Crypt25519FieldSquare(&Temp1, &Temp1);
    }

    /* z_10_0 = z_10_5*z_5_0 */
    Crypt25519FieldMultiply(&Temp0, &Temp1, &Temp0);

    /* z_20_10 = z_10_0^2^10 */
    Crypt25519FieldSquare(&Temp1, &Temp0);
    for (iIndex = 1; iIndex < 10; iIndex += 1)
    {
        Crypt25519FieldSquare(&Temp1, &Temp1);
    }

    /* z_20_0 = z_20_10*z_10_0 */
    Crypt25519FieldMultiply(&Temp1, &Temp1, &Temp0);

    /* z_40_20 = z_20_0^2^20 */
    Crypt25519FieldSquare(&Temp2, &Temp1);
    for (iIndex = 1; iIndex < 20; iIndex += 1)
    {
        Crypt25519FieldSquare(&Temp2, &Temp2);
    }

    /* z_40_0 = z_40_20*z_20_0 */
    Crypt25519FieldMultiply(&Temp1, &Temp2, &Temp1);

    /* z_50_10 = z_40_0^2^10 */
    Crypt25519FieldSquare(&Temp1, &Temp1);
    for (iIndex = 1; iIndex < 10; iIndex += 1)
    {
        Crypt25519FieldSquare(&Temp1, &Temp1);
    }

    /* z_50_0 = z_50_10*z_10_0 */
    Crypt25519FieldMultiply(&Temp0, &Temp1, &Temp0);

    /* z_100_50 = z_50_0^2^50 */
    Crypt25519FieldSquare(&Temp1, &Temp0);
    for (iIndex = 1; iIndex < 50; iIndex += 1)
    {
        Crypt25519FieldSquare(&Temp1, &Temp1);
    }

    /* z_100_0 = z_100_50*z_50_0 */
    Crypt25519FieldMultiply(&Temp1, &Temp1, &Temp0);

    /* z_200_100 = z_100_0^2^100 */
    Crypt25519FieldSquare(&Temp2, &Temp1);
    for (iIndex = 1; iIndex < 100; iIndex += 1)
    {
        Crypt25519FieldSquare(&Temp2, &Temp2);
    }

    /* z_200_0 = z_200_100*z_100_0 */
    Crypt25519FieldMultiply(&Temp1, &Temp2, &Temp1);

    /* z_250_50 = z_200_0^2^50 */
    Crypt25519FieldSquare(&Temp1, &Temp1);
    for (iIndex = 1; iIndex < 50; iIndex += 1)
    {
        Crypt25519FieldSquare(&Temp1, &Temp1);
    }

    /* z_250_0 = z_250_50*z_50_0 */
    Crypt25519FieldMultiply(&Temp0, &Temp1, &Temp0);

    /* z_252_2 = z_250_0^2^2 */
    Crypt25519FieldSquare(&Temp0, &Temp0);
    Crypt25519FieldSquare(&Temp0, &Temp0);

    /* z_252_3 = z_252_2*z1 */
    Crypt25519FieldMultiply(pOutput, &Temp0, pZ);
}
