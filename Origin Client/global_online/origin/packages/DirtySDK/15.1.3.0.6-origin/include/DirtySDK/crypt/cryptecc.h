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

#ifndef _cryptecc_h
#define _cryptecc_h

/*!
\Moduledef CryptEcc CryptEcc
\Modulemember Crypt
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/crypt/cryptbn.h"

/*** Defines **********************************************************************/

//! window size used for sliding window
#define CRYPTECC_WINDOW_SIZE (5)

//! precomputed table size based on the window size
#define CRYPTECC_TABLE_SIZE (1 << (CRYPTECC_WINDOW_SIZE - 1))

/*** Type Definitions *************************************************************/

//! used for the point calculations that are used for elliptic curve crypto
typedef struct CryptEccPointT
{
    CryptBnT X;
    CryptBnT Y;
} CryptEccPointT;

//! private state that defines the curve
typedef struct CryptEccT
{
    CryptBnT Prime;             //!< the finite field that the curve is defined over
    CryptBnT CoefficientA;      //!< cofficient that defines the curve
    CryptBnT CoefficientB;      //!< cofficient that defines the curve
    CryptEccPointT BasePoint;   //!< generator point on the curve
    CryptBnT Order;             //!< the number of point operations on the curve until the resultant line is vertical
    CryptBnT PrivateKey;        //!< private key used for curve operations

    CryptEccPointT Result;      //!< working state
    CryptEccPointT aTable[CRYPTECC_TABLE_SIZE]; //!< precomputed values used for computation
    int32_t iKeyIndex;          //!< current bit index state when generating

    uint32_t uCryptUSecs;       //!< number of total usecs the operation took
} CryptEccT;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// initializes the crypt state given the parameters of the curve
DIRTYCODE_API int32_t CryptEccInit(CryptEccT *pState, int32_t iSize, const uint8_t *pPrime, const uint8_t *pCoefficientA, const uint8_t *pCoefficientB, const uint8_t *pBasePoint, const uint8_t *pOrder);

// generates a public key based on the curve parameters: BasePoint * Secret
DIRTYCODE_API int32_t CryptEccPublic(CryptEccT *pState, CryptEccPointT *pResult);

// generate a shared secret based on the curve parameters and other parties public key: PublicKey * Secret
DIRTYCODE_API int32_t CryptEccSecret(CryptEccT *pState, CryptEccPointT *pPublicKey, CryptEccPointT *pResult);

// initialize a point on the curve from a buffer
DIRTYCODE_API int32_t CryptEccPointInitFrom(CryptEccPointT *pPoint, const uint8_t *pBuffer, int32_t iBufLen);

// output a point to a buffer
DIRTYCODE_API int32_t CryptEccPointFinal(CryptEccPointT *pPoint, uint8_t *pBuffer, int32_t iBufLen);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptecc_h
