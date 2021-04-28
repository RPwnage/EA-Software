/*H********************************************************************************/
/*!
    \File   cryptecc.h

    \Description
        This module implements the math for elliptic curve cryptography
        using curves in short Weierstrass form

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/crypt/cryptrand.h"
#include "DirtySDK/crypt/cryptecc.h"

/*** Defines **********************************************************************/

//! the marker that marks uncompressed point format
#define CRYPTECC_UNCOMPRESSED_MARKER (0x04)

//! number of iterations to do per call
#define CRYPTECC_NUM_ITERATIONS (0x10)

/*** Variables ********************************************************************/

//! used to easily checked for default points
static const CryptEccPointT _Zero = { { { 0x0 }, 1, 0 }, { { 0x0 }, 1, 0 } };

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CryptEccPointCalculate

    \Description
        Does the math for doubling / adding points given they you have calculated
        S over the curve's prime

    \Input *pState  - curve state
    \Input *pP      - point P
    \Input *pQ      - point Q
    \Input *pS      - S used in calculation
    \Input *pResult - [out] point R that is calculation of P+Q

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
static void _CryptEccPointCalculate(CryptEccT *pState, CryptEccPointT *pP, CryptEccPointT *pQ, CryptBnT *pS, CryptEccPointT *pResult)
{
    CryptEccPointT Result;

    // X = S² - P.X - Q.X
    CryptBnInitSet(&Result.X, 0);
    CryptBnMultiply(&Result.X, pS, pS);
    CryptBnSubtract(&Result.X, &Result.X, &pP->X);
    CryptBnSubtract(&Result.X, &Result.X, &pQ->X);

    // Y = S * (P.X - X) - P.Y
    CryptBnInitSet(&Result.Y, 0);
    CryptBnSubtract(&Result.Y, &pP->X, &Result.X);
    CryptBnMultiply(&Result.Y, &Result.Y, pS);
    CryptBnSubtract(&Result.Y, &Result.Y, &pP->Y);

    // X %= Curve.Prime
    CryptBnMod(&Result.X, &pState->Prime, &Result.X, NULL);
    // Y %= Curve.Prime
    CryptBnMod(&Result.Y, &pState->Prime, &Result.Y, NULL);

    // write out the result
    ds_memcpy(pResult, &Result, sizeof(*pResult));
}

/*F********************************************************************************/
/*!
    \Function _CryptEccPointAddition

    \Description
        Adds two points together

    \Input *pState  - curve state
    \Input *pPoint1 - first point in addition
    \Input *pPoint2 - second point in addtion
    \Input *pResult - [out] point R that is result of addition

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
static void _CryptEccPointAddition(CryptEccT *pState, CryptEccPointT *pPoint1, CryptEccPointT *pPoint2, CryptEccPointT *pResult)
{
    if (memcmp(pPoint1, &_Zero, sizeof(*pPoint1)) == 0)
    {
        ds_memcpy(pResult, pPoint2, sizeof(*pResult));
    }
    else if (memcmp(pPoint2, &_Zero, sizeof(*pPoint2)) == 0)
    {
        ds_memcpy(pResult, pPoint1, sizeof(*pResult));
    }
    else
    {
        CryptBnT A, B;

        CryptBnInitSet(&A, 0);
        CryptBnInitSet(&B, 0);

        // A = y1 - y2
        CryptBnSubtract(&A, &pPoint1->Y, &pPoint2->Y);

        // B = (x1 - x2)⁻¹ % Curve.Prime [Inverse Modulus]
        CryptBnSubtract(&B, &pPoint1->X, &pPoint2->X);
        CryptBnInverseMod(&B, &pState->Prime);

        // A *= B
        CryptBnModMultiply(&A, &A, &B, &pState->Prime);

        // calculate the result
        _CryptEccPointCalculate(pState, pPoint1, pPoint2, &A, pResult);
    }
}

/*F********************************************************************************/
/*!
    \Function _CryptEccPointDouble

    \Description
        Doubles a point

    \Input *pState  - curve state
    \Input *pPoint  - point that we are doubling
    \Input *pResult - [out] result of the doubling

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
static void _CryptEccPointDouble(CryptEccT *pState, CryptEccPointT *pPoint, CryptEccPointT *pResult)
{
    CryptBnT A, B;

    // if attempting to double zero, return
    if (memcmp(pPoint, &_Zero, sizeof(*pPoint)) == 0)
    {
        return;
    }

    // A = 3 * x * x + a
    CryptBnInitSet(&A, 3);
    CryptBnMultiply(&A, &A, &pPoint->X);
    CryptBnMultiply(&A, &A, &pPoint->X);
    CryptBnAccumulate(&A, &pState->CoefficientA);

    // B = (2 * y)⁻¹ % Curve.Prime [Inverse Modulus]
    CryptBnClone(&B, &pPoint->Y);
    CryptBnLeftShift(&B);
    CryptBnInverseMod(&B, &pState->Prime);

    // A *= B
    CryptBnModMultiply(&A, &A, &B, &pState->Prime);

    // calculate the result
    _CryptEccPointCalculate(pState, pPoint, pPoint, &A, pResult);
}

/*F********************************************************************************/
/*!
    \Function _CryptEccDoubleAndAdd

    \Description
        Performs the scalar multiplication using double and add
        operations using a sliding window for the private key

    \Input *pState  - curve state
    \Input *pInput  - point we are multiplying by the private key

    \Notes
        Adapted from the sliding window algorithm used for exponentiation
        in RSA
        See Handbook of Applied Cryptography Chapter 14.6.1 (14.85):
        http://cacr.uwaterloo.ca/hac/about/chap14.pdf

    \Version 05/08/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _CryptEccDoubleAndAdd(CryptEccT *pState, CryptEccPointT *pInput)
{
    int32_t iIter = CRYPTECC_NUM_ITERATIONS;

    do
    {
        const uint64_t uTick = NetTickUsec();

        if (pState->iKeyIndex < 0)
        {
            int32_t iTableIndex;
            CryptEccPointT Double;

            // init working state
            ds_memcpy(&pState->Result, &_Zero, sizeof(pState->Result));

            /* put the input into the first
               index of the table, this will make it so further
               entries in the table will not be too large
               for our modulus operations */
            ds_memcpy(&pState->aTable[0], pInput, sizeof(pState->aTable[0]));

            // calculate doubled input as the basis for the other calculations
            _CryptEccPointDouble(pState, pInput, &Double);

            // calculate the further added points for the rest of the table
            for (iTableIndex = 1; iTableIndex < CRYPTECC_TABLE_SIZE; iTableIndex += 1)
            {
                _CryptEccPointAddition(pState, &pState->aTable[iTableIndex - 1], &Double, &pState->aTable[iTableIndex]);
            }

            // set the defaults
            pState->iKeyIndex = CryptBnBitLen(&pState->PrivateKey) - 1;

            // clear the profiling information
            pState->uCryptUSecs = 0;
        }

        /* scan backwards from the current private key bit
           until a set bit is found to denote the start of the window
           doubling the results until such a bit is found */
        if (CryptBnBitTest(&pState->PrivateKey, pState->iKeyIndex))
        {
            int32_t iWindowBit, iWindowValue, iWindowEnd;

            /* scan backwards from the start of the window until the last set bit in the range of the window size is found which denotes the end bit index of the window
               calculate the value of the window that will be used when adding against our precomputed table.
               we skip the first bit as we know it is set based on the current private key bit check we make right above */
            for (iWindowBit = 1, iWindowValue = 1, iWindowEnd = 0; (iWindowBit < CRYPTECC_WINDOW_SIZE) && ((pState->iKeyIndex - iWindowBit) >= 0); iWindowBit += 1)
            {
                if (CryptBnBitTest(&pState->PrivateKey, pState->iKeyIndex - iWindowBit))
                {
                    iWindowValue <<= (iWindowBit - iWindowEnd);
                    iWindowValue  |= 1; // force odd
                    iWindowEnd = iWindowBit;
                }
            }

            // double for each bits in the window and moving the private key bit index down each time
            for (iWindowBit = 0; iWindowBit < iWindowEnd + 1; iWindowBit += 1, pState->iKeyIndex -= 1)
            {
                _CryptEccPointDouble(pState, &pState->Result, &pState->Result);
            }

            // use the window value to get which precomputed power we need to multiply
            _CryptEccPointAddition(pState, &pState->Result, &pState->aTable[iWindowValue/2], &pState->Result);
        }
        else
        {
            _CryptEccPointDouble(pState, &pState->Result, &pState->Result);
            pState->iKeyIndex -= 1;
        }

        // update profiler information
        pState->uCryptUSecs += NetTickDiff(NetTickUsec(), uTick);
    }
    while ((pState->iKeyIndex >= 0) && (--iIter > 0));

    return(pState->iKeyIndex >= 0);
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptEccInit

    \Description
        Initializes the curve state

    \Input *pState          - curve state
    \Input iSize            - size of the input parameters in bytes
    \Input *pPrime          - the curve's prime
    \Input *pCoefficientA   - the curve's coefficient a
    \Input *pCoefficientB   - the curve's coefficient b
    \Input *pBasePoint      - the curve's generator point
    \Input *pOrder          - the curve's order

    \Output
        int32_t             - 0=success, negative=failure

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptEccInit(CryptEccT *pState, int32_t iSize, const uint8_t *pPrime, const uint8_t *pCoefficientA, const uint8_t *pCoefficientB, const uint8_t *pBasePoint, const uint8_t *pOrder)
{
    uint8_t aSecret[128];

    CryptBnInitFrom(&pState->Prime, -1, pPrime, iSize);
    CryptBnInitFrom(&pState->CoefficientA, -1, pCoefficientA, iSize);
    CryptBnInitFrom(&pState->CoefficientB, -1, pCoefficientB, iSize);
    CryptBnInitFrom(&pState->Order, -1, pOrder, iSize);
    if (CryptEccPointInitFrom(&pState->BasePoint, pBasePoint, iSize * 2 + 1) < 0)
    {
        return(-1);
    }

    // generate a secret to the specified size
    CryptRandGet(aSecret, sizeof(aSecret));
    CryptBnInitFrom(&pState->PrivateKey, -1, aSecret, DS_MIN((int32_t)sizeof(aSecret), iSize));

    // random number = (rand() % order-1 + 1)
    CryptBnDecrement(&pState->Order);
    CryptBnMod(&pState->PrivateKey, &pState->Order, &pState->PrivateKey, NULL);
    CryptBnIncrement(&pState->PrivateKey);

    // init state
    pState->iKeyIndex = -1;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function CryptEccPublic

    \Description
        Generates a public key

    \Input *pState  - curve state
    \Input *pResult - [out] point on the curve that represents the public key

    \Output
        int32_t     - zero=success, otherwise=pending

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptEccPublic(CryptEccT *pState, CryptEccPointT *pResult)
{
    int32_t iResult;

    if ((iResult = _CryptEccDoubleAndAdd(pState, &pState->BasePoint)) == 0)
    {
        ds_memcpy(pResult, &pState->Result, sizeof(*pResult));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptEccSecret

    \Description
        Generates a shared secret

    \Input *pState      - curve state
    \Input *pPublicKey  - point as a basis to compute the shared secret
    \Input *pResult     - [out] point on the curve that represents the shared secret

    \Output
        int32_t         - zero=success, otherwise=pending

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptEccSecret(CryptEccT *pState, CryptEccPointT *pPublicKey, CryptEccPointT *pResult)
{
    int32_t iResult;

    if ((iResult = _CryptEccDoubleAndAdd(pState, pPublicKey)) == 0)
    {
        ds_memcpy(pResult, &pState->Result, sizeof(*pResult));
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptEccPointInitFrom

    \Description
        Loads a point from a buffer

    \Input *pPoint      - [out] the point we are writing the information to
    \Input *pBuffer     - buffer we are reading the point data from
    \Input iBufLen      - size of the buffer

    \Output
        int32_t         - 0=success, negative=failure

    \Notes
        We only support reading uncompressed points

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptEccPointInitFrom(CryptEccPointT *pPoint, const uint8_t *pBuffer, int32_t iBufLen)
{
    int32_t iPointLen = (iBufLen-1)/2;

    // we only support uncompressed points, so ensure that this is the case
    if (*pBuffer++ != CRYPTECC_UNCOMPRESSED_MARKER)
    {
        NetPrintf(("cryptecc: could not initialize curve, base point in wrong format\n"));
        return(-1);
    }
    CryptBnInitFrom(&pPoint->X, -1, pBuffer, iPointLen);
    CryptBnInitFrom(&pPoint->Y, -1, pBuffer+iPointLen, iPointLen);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function CryptEccPointFinal

    \Description
        Loads a point into a buffer

    \Input *pPoint      - the point we are encoding to the buffer
    \Input *pBuffer     - buffer we are writing the point data to
    \Input iBufLen      - size of the buffer

    \Output
        int32_t         - 0=success, negative=failure

    \Notes
        We only support writing uncompressed points

    \Version 02/17/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t CryptEccPointFinal(CryptEccPointT *pPoint, uint8_t *pBuffer, int32_t iBufLen)
{
    const int32_t iXSize = (pPoint->X.iWidth * UCRYPT_SIZE);
    const int32_t iYSize = (pPoint->Y.iWidth * UCRYPT_SIZE);
    const int32_t iOutputSize = iXSize + iYSize + 1;

    if (iBufLen < iOutputSize)
    {
        NetPrintf(("cryptecc: not enough space in output buffer to encode points\n"));
        return(-1);
    }

    *pBuffer++ = CRYPTECC_UNCOMPRESSED_MARKER;
    CryptBnFinal(&pPoint->X, pBuffer, iXSize);
    CryptBnFinal(&pPoint->Y, pBuffer+iXSize, iYSize);
    return(iOutputSize);
}
