/*H********************************************************************************/
/*!
    \File   crypt25519.h

    \Description
        This module implements the shared math for elliptic curve cryptography
        using 25519 prime

    \Copyright
        Copyright (c) Electronic Arts 2020. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _crypt25519_h
#define _crypt25519_h

/*!
\Moduledef Crypt25519 Crypt25519
\Modulemember Crypt
*/
//@{

#include <DirtySDK/platform.h>

/*** Type Definitions **************************************************************/

/*! Here the field is Z (2^255-19).
    An element t, entries t[0]...t[9], represents the integer
    t[0]+2^26 t[1]+2^51 t[2]+2^77 t[3]+2^102 t[4]+...+2^230 t[9].
    Bounds on each t[i] vary depending on context. */
typedef struct FieldElementT
{
    int32_t aData[10];
} FieldElementT;

/*** Functions ********************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

// loads uint64_t from 3-byte array
uint64_t Crypt25519Load3(const uint8_t *pInput);

// loads uint64_t from 4-byte array
uint64_t Crypt25519Load4(const uint8_t *pInput);

// initializes a field element
void Crypt25519FieldInit(FieldElementT *pOutput, int32_t iInitValue);

// copies a field element to another
void Crypt25519FieldCopy(FieldElementT *pDst, const FieldElementT *pSrc);

// conditionally swaps two fields based on the swap parameter
void Crypt25519FieldConditionSwap(FieldElementT *pP1, FieldElementT *pP2, uint32_t uSwap);

// conditionally moves one field to another based on the swap parameter
void Crypt25519FieldConditionalMove(FieldElementT *pP1, const FieldElementT *pP2, uint32_t uSwap);

// negates the values of the source field into the destinate field
void Crypt25519FieldNegate(FieldElementT *pDst, const FieldElementT *pSrc);

// copies the contents of a field into a byte array
void Crypt25519FieldToBytes(uint8_t *pDst, const FieldElementT *pSrc);

// initializes a field from a byte array
void Crypt25519FieldFromBytes(FieldElementT *pDst, const uint8_t *pSrc);

// checks if a field is negative
uint8_t Crypt25519FieldIsNegative(const FieldElementT *pInput);

// checks if a field is non-zero
uint8_t Crypt25519FieldIsNonZero(const FieldElementT *pInput);

// adds two fields together into the output
void Crypt25519FieldAdd(FieldElementT *pOutput, const FieldElementT *pLhs, const FieldElementT *pRhs);

// subtracts one field from another into the output
void Crypt25519FieldSubtract(FieldElementT *pOutput, const FieldElementT *pLhs, const FieldElementT *pRhs);

// multiples one field to another into the output
void Crypt25519FieldMultiply(FieldElementT *pOutput, const FieldElementT *pLhs, const FieldElementT *pRhs);

// squares a field into the output
void Crypt25519FieldSquare(FieldElementT *pOutput, const FieldElementT *pInput);

// performs the modular invert on a field
void Crypt25519FieldInvert(FieldElementT *pOutput, const FieldElementT *pZ);

// double and square a field
void Crypt25519FieldDoubleSquare(FieldElementT *pOutput, const FieldElementT *pInput);

// computes field ^ 255-23
void Crypt25519FieldPow22523(FieldElementT *pOutput, const FieldElementT *pZ);

#if defined(__cplusplus)
}
#endif

//@}

#endif // _crypt25519_h
