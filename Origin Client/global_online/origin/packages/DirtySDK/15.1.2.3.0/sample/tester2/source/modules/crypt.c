/*H********************************************************************************/
/*!
    \File crypt.c

    \Description
        Test the crypto modules.

    \Copyright
        Copyright (c) 2013 Electronic Arts Inc.

    \Version 01/11/2013 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdlib.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/cryptgcm.h"
#include "DirtySDK/crypt/cryptrand.h"
#include "DirtySDK/crypt/cryptrsa.h"
#include "DirtySDK/crypt/cryptsha2.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"
#include "DirtySDK/util/murmurhash3.h"

#include "testermodules.h"

#include "zlib.h"
#include "zmem.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//! type to encapsulate gcm test data
typedef struct CryptTestGcmT
{
    const uint8_t aKey[32];
    int32_t iKeySize;
    const uint8_t aInitVec[12];
    const uint8_t aInput[256];
    int32_t iInputSize;
    const uint8_t aData[256];
    int32_t iDataSize;
    const uint8_t aOutput[256];
    int32_t iOutputSize;
    const uint8_t aTagRslt[16];
} CryptTestGcmT;

//! type to encapsulate sha2 test data
typedef struct CryptTestSha2
{
    const char *pString;
    const uint8_t strHashRslt[4][CRYPTHASH_MAXDIGEST];
} CryptTestSha2;

//! type to encapsulate sha2 test data
typedef struct CryptTestHmac
{
    const char *pKey;
    const char *pString;
    const uint8_t strHmacRslt[8][CRYPTHASH_MAXDIGEST];
} CryptTestHmac;

/*** Variables ********************************************************************/

//! gcm test data from the FIPS standard
static const CryptTestGcmT _Crypt_GcmTest[] =
{
    // Test cases 0-3 use 128 bit keys

    // Test Case 0: validate tag result with zero key and no data
    {
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 }, // key
        16,                                                                                     // key size
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 },                      // iv
        { 0 }, 0,                                                                               // input, input size
        { 0 }, 0,                                                                               // data, data size
        { 0 }, 0,                                                                               // output, output size
        { 0x58,0xe2,0xfc,0xce, 0xfa,0x7e,0x30,0x61, 0x36,0x7f,0x1d,0x57, 0xa4,0xe7,0x45,0x5a }  // tag result
    },
    // Test Case 1: validate encryption and tag result with zero key and zero data
    {
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 }, // key
        16,                                                                                     // key size
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 },                      // iv
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 }, // input
        16,                                                                                     // input size
        { 0 }, 0,                                                                               // data, data size
        { 0x03,0x88,0xda,0xce, 0x60,0xb6,0xa3,0x92, 0xf3,0x28,0xc2,0xb9, 0x71,0xb2,0xfe,0x78 }, // output
        16,                                                                                     // output size
        { 0xab,0x6e,0x47,0xd4, 0x2c,0xec,0x13,0xbd, 0xf5,0x3a,0x67,0xb2, 0x12,0x57,0xbd,0xdf }  // tag result
    },
    // Test Case 2: validate encryption and tag result with non-zero key and data
    {
        { 0xfe,0xff,0xe9,0x92, 0x86,0x65,0x73,0x1c, 0x6d,0x6a,0x8f,0x94, 0x67,0x30,0x83,0x08 }, // key
        16,                                                                                     // key size
        { 0xca,0xfe,0xba,0xbe, 0xfa,0xce,0xdb,0xad, 0xde,0xca,0xf8,0x88  },                     // iv
        {
            0xd9,0x31,0x32,0x25, 0xf8,0x84,0x06,0xe5, 0xa5,0x59,0x09,0xc5, 0xaf,0xf5,0x26,0x9a,
            0x86,0xa7,0xa9,0x53, 0x15,0x34,0xf7,0xda, 0x2e,0x4c,0x30,0x3d, 0x8a,0x31,0x8a,0x72,
            0x1c,0x3c,0x0c,0x95, 0x95,0x68,0x09,0x53, 0x2f,0xcf,0x0e,0x24, 0x49,0xa6,0xb5,0x25,
            0xb1,0x6a,0xed,0xf5, 0xaa,0x0d,0xe6,0x57, 0xba,0x63,0x7b,0x39, 0x1a,0xaf,0xd2,0x55
        }, 64,                                                                                  // input, input size
        { 0 }, 0,                                                                               // data, data size
        {
            0x42,0x83,0x1e,0xc2, 0x21,0x77,0x74,0x24, 0x4b,0x72,0x21,0xb7, 0x84,0xd0,0xd4,0x9c,
            0xe3,0xaa,0x21,0x2f, 0x2c,0x02,0xa4,0xe0, 0x35,0xc1,0x7e,0x23, 0x29,0xac,0xa1,0x2e,
            0x21,0xd5,0x14,0xb2, 0x54,0x66,0x93,0x1c, 0x7d,0x8f,0x6a,0x5a, 0xac,0x84,0xaa,0x05,
            0x1b,0xa3,0x0b,0x39, 0x6a,0x0a,0xac,0x97, 0x3d,0x58,0xe0,0x91, 0x47,0x3f,0x59,0x85
        }, 64,                                                                                  // output, output size
        { 0x4d,0x5c,0x2a,0xf3, 0x27,0xcd,0x64,0xa6, 0x2c,0xf3,0x5a,0xbd, 0x2b,0xa6,0xfa,0xb4 }  // tag result
    },
    // Test Case 3: validate encryption and tag result with non-zero key, un-aligned data, and additional data
    {
        { 0xfe,0xff,0xe9,0x92, 0x86,0x65,0x73,0x1c, 0x6d,0x6a,0x8f,0x94, 0x67,0x30,0x83,0x08 }, // key
        16,                                                                                     // key size
        { 0xca,0xfe,0xba,0xbe, 0xfa,0xce,0xdb,0xad, 0xde,0xca,0xf8,0x88  },                     // iv
        {
            0xd9,0x31,0x32,0x25, 0xf8,0x84,0x06,0xe5, 0xa5,0x59,0x09,0xc5, 0xaf,0xf5,0x26,0x9a,
            0x86,0xa7,0xa9,0x53, 0x15,0x34,0xf7,0xda, 0x2e,0x4c,0x30,0x3d, 0x8a,0x31,0x8a,0x72,
            0x1c,0x3c,0x0c,0x95, 0x95,0x68,0x09,0x53, 0x2f,0xcf,0x0e,0x24, 0x49,0xa6,0xb5,0x25,
            0xb1,0x6a,0xed,0xf5, 0xaa,0x0d,0xe6,0x57, 0xba,0x63,0x7b,0x39
        }, 60,                                                                                  // input, input size
        {
            0xfe,0xed,0xfa,0xce, 0xde,0xad,0xbe,0xef, 0xfe,0xed,0xfa,0xce, 0xde,0xad,0xbe,0xef,
            0xab,0xad,0xda,0xd2
        }, 20,                                                                                  // data, data size
        {                                                                                       // output
            0x42,0x83,0x1e,0xc2, 0x21,0x77,0x74,0x24, 0x4b,0x72,0x21,0xb7, 0x84,0xd0,0xd4,0x9c,
            0xe3,0xaa,0x21,0x2f, 0x2c,0x02,0xa4,0xe0, 0x35,0xc1,0x7e,0x23, 0x29,0xac,0xa1,0x2e,
            0x21,0xd5,0x14,0xb2, 0x54,0x66,0x93,0x1c, 0x7d,0x8f,0x6a,0x5a, 0xac,0x84,0xaa,0x05,
            0x1b,0xa3,0x0b,0x39, 0x6a,0x0a,0xac,0x97, 0x3d,0x58,0xe0,0x91
        },
        60,                                                                                     // output size
        { 0x5b,0xc9,0x4f,0xbc, 0x32,0x21,0xa5,0xdb, 0x94,0xfa,0xe9,0x5a, 0xe7,0x12,0x1a,0x47 }  // tag result
    },

    // Test cases 4-7 use 256 bit keys

    // Test Case 4: validate tag result with zero key and no data
    {
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   // key
          0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 },
        32,                                                                                     // key size
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 },                      // iv
        { 0 }, 0,                                                                               // input, input size
        { 0 }, 0,                                                                               // data, data size
        { 0 }, 0,                                                                               // output, output size
        { 0x53,0x0f,0x8a,0xfb, 0xc7,0x45,0x36,0xb9, 0xa9,0x63,0xb4,0xf1, 0xc4,0xcb,0x73,0x8b }  // tag result
    },
    // Test Case 5: validate encryption and tag result with zero key and zero data
    {
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,   // key
          0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 },
        32,                                                                                     // key size
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 },                      // iv
        { 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00 }, // input
        16,                                                                                     // input size
        { 0 }, 0,                                                                               // data, data size
        { 0xce,0xa7,0x40,0x3d, 0x4d,0x60,0x6b,0x6e, 0x07,0x4e,0xc5,0xd3, 0xba,0xf3,0x9d,0x18 }, // output
        16,                                                                                     // output size
        { 0xd0,0xd1,0xc8,0xa7, 0x99,0x99,0x6b,0xf0, 0x26,0x5b,0x98,0xb5, 0xd4,0x8a,0xb9,0x19 }  // tag result
    },
    // Test Case 6: validate encryption and tag result with non-zero key and data
    {
        { 0xfe,0xff,0xe9,0x92, 0x86,0x65,0x73,0x1c, 0x6d,0x6a,0x8f,0x94, 0x67,0x30,0x83,0x08,   // key
          0xfe,0xff,0xe9,0x92, 0x86,0x65,0x73,0x1c, 0x6d,0x6a,0x8f,0x94, 0x67,0x30,0x83,0x08 },
        32,                                                                                     // key size
        { 0xca,0xfe,0xba,0xbe, 0xfa,0xce,0xdb,0xad, 0xde,0xca,0xf8,0x88  },                     // iv
        {
            0xd9,0x31,0x32,0x25, 0xf8,0x84,0x06,0xe5, 0xa5,0x59,0x09,0xc5, 0xaf,0xf5,0x26,0x9a,
            0x86,0xa7,0xa9,0x53, 0x15,0x34,0xf7,0xda, 0x2e,0x4c,0x30,0x3d, 0x8a,0x31,0x8a,0x72,
            0x1c,0x3c,0x0c,0x95, 0x95,0x68,0x09,0x53, 0x2f,0xcf,0x0e,0x24, 0x49,0xa6,0xb5,0x25,
            0xb1,0x6a,0xed,0xf5, 0xaa,0x0d,0xe6,0x57, 0xba,0x63,0x7b,0x39, 0x1a,0xaf,0xd2,0x55
        }, 64,                                                                                  // input, input size
        { 0 }, 0,                                                                               // data, data size
        {
            0x52,0x2d,0xc1,0xf0, 0x99,0x56,0x7d,0x07, 0xf4,0x7f,0x37,0xa3, 0x2a,0x84,0x42,0x7d,
            0x64,0x3a,0x8c,0xdc, 0xbf,0xe5,0xc0,0xc9, 0x75,0x98,0xa2,0xbd, 0x25,0x55,0xd1,0xaa,
            0x8c,0xb0,0x8e,0x48, 0x59,0x0d,0xbb,0x3d, 0xa7,0xb0,0x8b,0x10, 0x56,0x82,0x88,0x38,
            0xc5,0xf6,0x1e,0x63, 0x93,0xba,0x7a,0x0a, 0xbc,0xc9,0xf6,0x62, 0x89,0x80,0x15,0xad,
        }, 64,                                                                                  // output, output size
        { 0xb0,0x94,0xda,0xc5, 0xd9,0x34,0x71,0xbd, 0xec,0x1a,0x50,0x22, 0x70,0xe3,0xcc,0x6c }  // tag result
    },
    // Test Case 3: validate encryption and tag result with non-zero key, un-aligned data, and additional data
    {
        { 0xfe,0xff,0xe9,0x92, 0x86,0x65,0x73,0x1c, 0x6d,0x6a,0x8f,0x94, 0x67,0x30,0x83,0x08,   // key
          0xfe,0xff,0xe9,0x92, 0x86,0x65,0x73,0x1c, 0x6d,0x6a,0x8f,0x94, 0x67,0x30,0x83,0x08 },
        32,                                                                                     // key size
        { 0xca,0xfe,0xba,0xbe, 0xfa,0xce,0xdb,0xad, 0xde,0xca,0xf8,0x88  },                     // iv
        {
            0xd9,0x31,0x32,0x25, 0xf8,0x84,0x06,0xe5, 0xa5,0x59,0x09,0xc5, 0xaf,0xf5,0x26,0x9a,
            0x86,0xa7,0xa9,0x53, 0x15,0x34,0xf7,0xda, 0x2e,0x4c,0x30,0x3d, 0x8a,0x31,0x8a,0x72,
            0x1c,0x3c,0x0c,0x95, 0x95,0x68,0x09,0x53, 0x2f,0xcf,0x0e,0x24, 0x49,0xa6,0xb5,0x25,
            0xb1,0x6a,0xed,0xf5, 0xaa,0x0d,0xe6,0x57, 0xba,0x63,0x7b,0x39
        }, 60,                                                                                  // input, input size
        {
            0xfe,0xed,0xfa,0xce, 0xde,0xad,0xbe,0xef, 0xfe,0xed,0xfa,0xce, 0xde,0xad,0xbe,0xef,
            0xab,0xad,0xda,0xd2
        }, 20,                                                                                  // data, data size
        {                                                                                       // output
            0x52,0x2d,0xc1,0xf0, 0x99,0x56,0x7d,0x07, 0xf4,0x7f,0x37,0xa3, 0x2a,0x84,0x42,0x7d,
            0x64,0x3a,0x8c,0xdc, 0xbf,0xe5,0xc0,0xc9, 0x75,0x98,0xa2,0xbd, 0x25,0x55,0xd1,0xaa,
            0x8c,0xb0,0x8e,0x48, 0x59,0x0d,0xbb,0x3d, 0xa7,0xb0,0x8b,0x10, 0x56,0x82,0x88,0x38,
            0xc5,0xf6,0x1e,0x63, 0x93,0xba,0x7a,0x0a, 0xbc,0xc9,0xf6,0x62,
        },
        60,                                                                                     // output size
        { 0x76,0xfc,0x6e,0xce, 0x0f,0x4e,0x17,0x68, 0xcd,0xdf,0x88,0x53, 0xbb,0x2d,0x55,0x1b }  // tag result
    },
};

//! sha2 test data
static const CryptTestSha2 _Crypt_Sha2Test[] =
{
    {
        "",
        {
            {   // sha224
                0xd1,0x4a,0x02,0x8c, 0x2a,0x3a,0x2b,0xc9, 0x47,0x61,0x02,0xbb, 0x28,0x82,0x34,0xc4,
                0x15,0xa2,0xb0,0x1f, 0x82,0x8e,0xa6,0x2a, 0xc5,0xb3,0xe4,0x2f
            },
            {   // sha256
                0xe3,0xb0,0xc4,0x42, 0x98,0xfc,0x1c,0x14, 0x9a,0xfb,0xf4,0xc8, 0x99,0x6f,0xb9,0x24,
                0x27,0xae,0x41,0xe4, 0x64,0x9b,0x93,0x4c, 0xa4,0x95,0x99,0x1b, 0x78,0x52,0xb8,0x55
            },
            {   // sha384
                0x38,0xb0,0x60,0xa7, 0x51,0xac,0x96,0x38, 0x4c,0xd9,0x32,0x7e, 0xb1,0xb1,0xe3,0x6a,
                0x21,0xfd,0xb7,0x11, 0x14,0xbe,0x07,0x43, 0x4c,0x0c,0xc7,0xbf, 0x63,0xf6,0xe1,0xda,
                0x27,0x4e,0xde,0xbf, 0xe7,0x6f,0x65,0xfb, 0xd5,0x1a,0xd2,0xf1, 0x48,0x98,0xb9,0x5b
            },
            {   // sha512
                0xcf,0x83,0xe1,0x35, 0x7e,0xef,0xb8,0xbd, 0xf1,0x54,0x28,0x50, 0xd6,0x6d,0x80,0x07,
                0xd6,0x20,0xe4,0x05, 0x0b,0x57,0x15,0xdc, 0x83,0xf4,0xa9,0x21, 0xd3,0x6c,0xe9,0xce,
                0x47,0xd0,0xd1,0x3c, 0x5d,0x85,0xf2,0xb0, 0xff,0x83,0x18,0xd2, 0x87,0x7e,0xec,0x2f,
                0x63,0xb9,0x31,0xbd, 0x47,0x41,0x7a,0x81, 0xa5,0x38,0x32,0x7a, 0xf9,0x27,0xda,0x3e
            }
        }
    },
    {
        "The quick brown fox jumps over the lazy dog",
        {
            {   // sha224
                0x73,0x0e,0x10,0x9b, 0xd7,0xa8,0xa3,0x2b, 0x1c,0xb9,0xd9,0xa0, 0x9a,0xa2,0x32,0x5d,
                0x24,0x30,0x58,0x7d, 0xdb,0xc0,0xc3,0x8b, 0xad,0x91,0x15,0x25
            },
            {   // sha256
                0xd7,0xa8,0xfb,0xb3, 0x07,0xd7,0x80,0x94, 0x69,0xca,0x9a,0xbc, 0xb0,0x08,0x2e,0x4f,
                0x8d,0x56,0x51,0xe4, 0x6d,0x3c,0xdb,0x76, 0x2d,0x02,0xd0,0xbf, 0x37,0xc9,0xe5,0x92
            },
            {   // sha384
                0xca,0x73,0x7f,0x10, 0x14,0xa4,0x8f,0x4c, 0x0b,0x6d,0xd4,0x3c, 0xb1,0x77,0xb0,0xaf,
                0xd9,0xe5,0x16,0x93, 0x67,0x54,0x4c,0x49, 0x40,0x11,0xe3,0x31, 0x7d,0xbf,0x9a,0x50,
                0x9c,0xb1,0xe5,0xdc, 0x1e,0x85,0xa9,0x41, 0xbb,0xee,0x3d,0x7f, 0x2a,0xfb,0xc9,0xb1
            },
            {   // sha512
                0x07,0xe5,0x47,0xd9, 0x58,0x6f,0x6a,0x73, 0xf7,0x3f,0xba,0xc0, 0x43,0x5e,0xd7,0x69,
                0x51,0x21,0x8f,0xb7, 0xd0,0xc8,0xd7,0x88, 0xa3,0x09,0xd7,0x85, 0x43,0x6b,0xbb,0x64,
                0x2e,0x93,0xa2,0x52, 0xa9,0x54,0xf2,0x39, 0x12,0x54,0x7d,0x1e, 0x8a,0x3b,0x5e,0xd6,
                0xe1,0xbf,0xd7,0x09, 0x78,0x21,0x23,0x3f, 0xa0,0x53,0x8f,0x3d, 0xb8,0x54,0xfe,0xe6
            }
        }
    },
    {
        "The quick brown fox jumps over the lazy dog.",
        {
            {   // sha224
                0x61,0x9c,0xba,0x8e, 0x8e,0x05,0x82,0x6e, 0x9b,0x8c,0x51,0x9c, 0x0a,0x5c,0x68,0xf4,
                0xfb,0x65,0x3e,0x8a, 0x3d,0x8a,0xa0,0x4b, 0xb2,0xc8,0xcd,0x4c
            },
            {   // sha256
                0xef,0x53,0x7f,0x25, 0xc8,0x95,0xbf,0xa7, 0x82,0x52,0x65,0x29, 0xa9,0xb6,0x3d,0x97,
                0xaa,0x63,0x15,0x64, 0xd5,0xd7,0x89,0xc2, 0xb7,0x65,0x44,0x8c, 0x86,0x35,0xfb,0x6c
            },
            {   // sha384
                0xed,0x89,0x24,0x81, 0xd8,0x27,0x2c,0xa6, 0xdf,0x37,0x0b,0xf7, 0x06,0xe4,0xd7,0xbc,
                0x1b,0x57,0x39,0xfa, 0x21,0x77,0xaa,0xe6, 0xc5,0x0e,0x94,0x66, 0x78,0x71,0x8f,0xc6,
                0x7a,0x7a,0xf2,0x81, 0x9a,0x02,0x1c,0x2f, 0xc3,0x4e,0x91,0xbd, 0xb6,0x34,0x09,0xd7
            },
            {   // sha512
                0x91,0xea,0x12,0x45, 0xf2,0x0d,0x46,0xae, 0x9a,0x03,0x7a,0x98, 0x9f,0x54,0xf1,0xf7,
                0x90,0xf0,0xa4,0x76, 0x07,0xee,0xb8,0xa1, 0x4d,0x12,0x89,0x0c, 0xea,0x77,0xa1,0xbb,
                0xc6,0xc7,0xed,0x9c, 0xf2,0x05,0xe6,0x7b, 0x7f,0x2b,0x8f,0xd4, 0xc7,0xdf,0xd3,0xa7,
                0xa8,0x61,0x7e,0x45, 0xf3,0xc4,0x63,0xd4, 0x81,0xc7,0xe5,0x86, 0xc3,0x9a,0xc1,0xed
            }
        }
    },
    // test to validate sha384/sha512 128-bit length pad is correct
    {
        "The quick brown fox jumps over the lazy dog...\r\n"
        "The quick brown fox jumps over the lazy dog...\r\n"
        "The quick brown fox j\r\n",
        {
            {   // sha224
                0x6b,0x7c,0x3d,0xfb, 0xe4,0x3d,0x5c,0x0a, 0x9d,0xe5,0xa0,0x9e, 0x2c,0x90,0xc1,0x62,
                0xa5,0x1c,0xb3,0x32, 0x99,0x9e,0x88,0xce, 0xdb,0x5f,0xbc,0x57
            },
            {   // sha256
                0x0d,0x4c,0xaf,0x0a, 0x31,0x1c,0xb6,0x26, 0xdf,0x91,0x02,0xc2, 0x01,0xc1,0x8a,0x12,
                0x8a,0x0b,0x4b,0x4d, 0x1d,0xfa,0xa3,0xa9, 0xd9,0x43,0xc6,0x47, 0xd3,0x92,0xb6,0x54
            },
            {   // sha384
                0xed,0xae,0xd3,0xa8, 0xc6,0x3b,0x44,0x63, 0xb9,0xd6,0x34,0x8b, 0x86,0xda,0x9f,0xaf,
                0x69,0xe9,0x91,0x48, 0xe6,0xef,0xfc,0x71, 0x8a,0xf9,0x64,0x1e, 0x46,0x69,0x05,0x97,
                0x32,0xb8,0x0f,0xc8, 0x0b,0x84,0x39,0x7f, 0x58,0x3e,0xc3,0x50, 0x2b,0xa4,0x91,0xd8
            },
            {   // sha512
                0xb6,0x39,0x3a,0x1a, 0x27,0x4d,0x62,0xe3, 0x6c,0x9b,0xbf,0x83, 0xc4,0x39,0x68,0xcd,
                0x7b,0xaa,0x36,0x04, 0xb7,0x05,0xea,0x28, 0xd8,0x43,0x23,0x20, 0xe1,0xf5,0x29,0x6b,
                0xc1,0x93,0x6c,0xcb, 0x08,0x4e,0xad,0xfe, 0xd8,0xc9,0x38,0xd2, 0x91,0x9c,0x47,0xbd,
                0x04,0xdc,0xa1,0x3c, 0x66,0x7b,0xaf,0x47, 0xbd,0xc7,0xa4,0xfd, 0xa8,0xa2,0xc0,0xde
            }
        }
    },
    {
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567"
        "0123456701234567012345670123456701234567012345670123456701234567",
        {
            {   // sha224
                0x56,0x7F,0x69,0xF1, 0x68,0xCD,0x78,0x44, 0xE6,0x52,0x59,0xCE, 0x65,0x8F,0xE7,0xAA,
                0xDF,0xA2,0x52,0x16, 0xE6,0x8E,0xCA,0x0E, 0xB7,0xAB,0x82,0x62
            },
            {   // sha256
                0x59,0x48,0x47,0x32, 0x84,0x51,0xBD,0xFA, 0x85,0x05,0x62,0x25, 0x46,0x2C,0xC1,0xD8,
                0x67,0xD8,0x77,0xFB, 0x38,0x8D,0xF0,0xCE, 0x35,0xF2,0x5A,0xB5, 0x56,0x2B,0xFB,0xB5
            },
            {   // sha384
                0x2F,0xC6,0x4A,0x4F, 0x50,0x0D,0xDB,0x68, 0x28,0xF6,0xA3,0x43, 0x0B,0x8D,0xD7,0x2A,
                0x36,0x8E,0xB7,0xF3, 0xA8,0x32,0x2A,0x70, 0xBC,0x84,0x27,0x5B, 0x9C,0x0B,0x3A,0xB0,
                0x0D,0x27,0xA5,0xCC, 0x3C,0x2D,0x22,0x4A, 0xA6,0xB6,0x1A,0x0D, 0x79,0xFB,0x45,0x96
            },
            {   // sha512
                0x89,0xD0,0x5B,0xA6, 0x32,0xC6,0x99,0xC3, 0x12,0x31,0xDE,0xD4, 0xFF,0xC1,0x27,0xD5,
                0xA8,0x94,0xDA,0xD4, 0x12,0xC0,0xE0,0x24, 0xDB,0x87,0x2D,0x1A, 0xBD,0x2B,0xA8,0x14,
                0x1A,0x0F,0x85,0x07, 0x2A,0x9B,0xE1,0xE2, 0xAA,0x04,0xCF,0x33, 0xC7,0x65,0xCB,0x51,
                0x08,0x13,0xA3,0x9C, 0xD5,0xA8,0x4C,0x4A, 0xCA,0xA6,0x4D,0x3F, 0x3F,0xB7,0xBA,0xE9
            }
        }
    }
};

//! HMAC test data
static const CryptTestHmac _Crypt_HmacTest[] =
{
    {
        "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b",
        "Hi There",
        {
            {   // murmurhash3
                0xE1,0x31,0x7C,0x7B, 0x06,0xAD,0xF3,0xB4, 0xBB,0x26,0x9A,0x83, 0xEF,0x0E,0xA3,0xB4
            },
            {   // md2
                0x7E,0x93,0xEC,0x90, 0x8D,0xB9,0xA4,0xE9, 0x59,0x0F,0xB4,0x3B, 0x72,0xE5,0x78,0x21
            },
            {   // md5
                0x5C,0xCE,0xC3,0x4E, 0xA9,0x65,0x63,0x92, 0x45,0x7F,0xA1,0xAC, 0x27,0xF0,0x8F,0xBC
            },
            {   // sha1
                0xB6,0x17,0x31,0x86, 0x55,0x05,0x72,0x64, 0xE2,0x8B,0xC0,0xB6, 0xFB,0x37,0x8C,0x8E,
                0xF1,0x46,0xBE,0x00
            },
            {   // sha224
                0x89,0x6F,0xB1,0x12, 0x8A,0xBB,0xDF,0x19, 0x68,0x32,0x10,0x7C, 0xD4,0x9D,0xF3,0x3F,
                0x47,0xB4,0xB1,0x16, 0x99,0x12,0xBA,0x4F, 0x53,0x68,0x4B,0x22
            },
            {   // sha256
                0xB0,0x34,0x4C,0x61, 0xD8,0xDB,0x38,0x53, 0x5C,0xA8,0xAF,0xCE, 0xAF,0x0B,0xF1,0x2B,
                0x88,0x1D,0xC2,0x00, 0xC9,0x83,0x3D,0xA7, 0x26,0xE9,0x37,0x6C, 0x2E,0x32,0xCF,0xF7
            },
            {   // sha384
                0xAF,0xD0,0x39,0x44, 0xD8,0x48,0x95,0x62, 0x6B,0x08,0x25,0xF4, 0xAB,0x46,0x90,0x7F,
                0x15,0xF9,0xDA,0xDB, 0xE4,0x10,0x1E,0xC6, 0x82,0xAA,0x03,0x4C, 0x7C,0xEB,0xC5,0x9C,
                0xFA,0xEA,0x9E,0xA9, 0x07,0x6E,0xDE,0x7F, 0x4A,0xF1,0x52,0xE8, 0xB2,0xFA,0x9C,0xB6
            },
            {   // sha512
                0x87,0xAA,0x7C,0xDE, 0xA5,0xEF,0x61,0x9D, 0x4F,0xF0,0xB4,0x24, 0x1A,0x1D,0x6C,0xB0,
                0x23,0x79,0xF4,0xE2, 0xCE,0x4E,0xC2,0x78, 0x7A,0xD0,0xB3,0x05, 0x45,0xE1,0x7C,0xDE,
                0xDA,0xA8,0x33,0xB7, 0xD6,0xB8,0xA7,0x02, 0x03,0x8B,0x27,0x4E, 0xAE,0xA3,0xF4,0xE4,
                0xBE,0x9D,0x91,0x4E, 0xEB,0x61,0xF1,0x70, 0x2E,0x69,0x6C,0x20, 0x3A,0x12,0x68,0x54
            },
        },
    },
    {
        "Jefe",
        "what do ya want for nothing?",
        {
            {   // murmurhash3
                0xD9,0x8B,0x6A,0x6D, 0x61,0x65,0xCB,0xEF, 0x42,0xA4,0x66,0x71, 0x5B,0xD1,0xFE,0xEF
            },
            {   // md2
                0x07,0x6F,0x10,0xBF, 0x54,0x33,0x69,0xF8, 0x15,0xD3,0xAA,0x31, 0xC2,0x4A,0xD5,0x3C
            },
            {   // md5
                0x75,0x0C,0x78,0x3E, 0x6A,0xB0,0xB5,0x03, 0xEA,0xA8,0x6E,0x31, 0x0A,0x5D,0xB7,0x38
            },
            {   // sha1
                0xEF,0xFC,0xDF,0x6A, 0xE5,0xEB,0x2F,0xA2, 0xD2,0x74,0x16,0xD5, 0xF1,0x84,0xDF,0x9C,
                0x25,0x9A,0x7C,0x79
            },
            {   // sha224
                0xA3,0x0E,0x01,0x09, 0x8B,0xC6,0xDB,0xBF, 0x45,0x69,0x0F,0x3A, 0x7E,0x9E,0x6D,0x0F,
                0x8B,0xBE,0xA2,0xA3, 0x9E,0x61,0x48,0x00, 0x8F,0xD0,0x5E,0x44
            },
            {   // sha256
                0x5B,0xDC,0xC1,0x46, 0xBF,0x60,0x75,0x4E, 0x6A,0x04,0x24,0x26, 0x08,0x95,0x75,0xC7,
                0x5A,0x00,0x3F,0x08, 0x9D,0x27,0x39,0x83, 0x9D,0xEC,0x58,0xB9, 0x64,0xEC,0x38,0x43
            },
            {   // sha384
                0xAF,0x45,0xD2,0xE3, 0x76,0x48,0x40,0x31, 0x61,0x7F,0x78,0xD2, 0xB5,0x8A,0x6B,0x1B,
                0x9C,0x7E,0xF4,0x64, 0xF5,0xA0,0x1B,0x47, 0xE4,0x2E,0xC3,0x73, 0x63,0x22,0x44,0x5E,
                0x8E,0x22,0x40,0xCA, 0x5E,0x69,0xE2,0xC7, 0x8B,0x32,0x39,0xEC, 0xFA,0xB2,0x16,0x49
            },
            {   // sha512
                0x16,0x4B,0x7A,0x7B, 0xFC,0xF8,0x19,0xE2, 0xE3,0x95,0xFB,0xE7, 0x3B,0x56,0xE0,0xA3,
                0x87,0xBD,0x64,0x22, 0x2E,0x83,0x1F,0xD6, 0x10,0x27,0x0C,0xD7, 0xEA,0x25,0x05,0x54,
                0x97,0x58,0xBF,0x75, 0xC0,0x5A,0x99,0x4A, 0x6D,0x03,0x4F,0x65, 0xF8,0xF0,0xE6,0xFD,
                0xCA,0xEA,0xB1,0xA3, 0x4D,0x4A,0x6B,0x4B, 0x63,0x6E,0x07,0x0A, 0x38,0xBC,0xE7,0x37
            }
        }
    }
};

// buffer for large random number used in timing tests
static uint8_t _Crypt_aLargeRandom[2*1024*1024];
const int32_t  _Crypt_iTimingIter = 16;


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _CmdCryptTestArc4

    \Description
        Test the CryptArc4 module

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 01/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestArc4(ZContext *argz, int32_t argc, char *argv[])
{
    static const char *_strEncrypt[] =
    {
        "Test first string encryption.",
        "Another string to test encrypt.",
        "Strings are fun to encrypt!",
        "This string is exactly 127 characters long, sans null.  It is meant to test the behavior of Encrypt/Decrypt with a full buffer.",
        "This string is more than 127 characters long.  It will safely truncate the result string, which will result in the test failing.",
    };
    char strEncryptBuf[128], strDecryptBuf[128];
    uint8_t aEncryptKey[32];
    int32_t iFailed, iString;

    ZPrintf("%s: testing CryptArc4\n", argv[0]);

    // generate a random encryption key
    CryptRandGet(aEncryptKey, sizeof(aEncryptKey));

    // test string encryption/decryption
    for (iString = 0, iFailed = 0; iString < (signed)(sizeof(_strEncrypt)/sizeof(_strEncrypt[0])); iString += 1)
    {
        CryptArc4StringEncrypt(strEncryptBuf, sizeof(strEncryptBuf), _strEncrypt[iString], aEncryptKey, sizeof(aEncryptKey), 1);
        CryptArc4StringDecrypt(strDecryptBuf, sizeof(strDecryptBuf), strEncryptBuf, aEncryptKey, sizeof(aEncryptKey), 1);
        ZPrintf("%s: '%s'->'%s'\n", argv[0], _strEncrypt[iString], strEncryptBuf);
        if (strcmp(strDecryptBuf, _strEncrypt[iString]))
        {
            ZPrintf("%s: encrypt/decrypt failed; '%s' != '%s'\n", argv[0], _strEncrypt[iString], strDecryptBuf);
            iFailed += 1;
        }
    }

    ZPrintf("%s: ---------------------\n", argv[0]);

    // one test intentionally fails, so subtract that out
    iFailed -= 1;
    return(iFailed);
}

/*F********************************************************************************/
/*!
    \Function _CmdCryptTestGcm

    \Description
        Test the CryptGcm module

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 07/08/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestGcm(ZContext *argz, int32_t argc, char *argv[])
{
    uint8_t aBuffer[1024], aTagRslt[16];
    int32_t iFail, iFailed, iTest;
    const CryptTestGcmT *pTest;
    CryptGcmT Gcm;

    ZPrintf("%s: testing CryptGcm\n", argv[0]);

    for (iTest = 0, iFailed = 0; iTest < (signed)(sizeof(_Crypt_GcmTest)/sizeof(_Crypt_GcmTest[0])); iTest += 1)
    {
        pTest = &_Crypt_GcmTest[iTest];
        iFail = 0;

        // do encryption
        CryptGcmInit(&Gcm, pTest->aKey, pTest->iKeySize);
        memcpy(aBuffer, pTest->aInput, pTest->iInputSize);
        CryptGcmEncrypt(&Gcm, aBuffer, pTest->iInputSize, pTest->aInitVec, sizeof(pTest->aInitVec), pTest->aData, pTest->iDataSize, aTagRslt, sizeof(aTagRslt));

        if (memcmp(pTest->aTagRslt, aTagRslt, sizeof(pTest->aTagRslt)))
        {
            ZPrintf("%s: gcm encrypt test %d failed with invalid tag result\n", argv[0], iTest);
            NetPrintMem(pTest->aTagRslt, sizeof(pTest->aTagRslt), "expected");
            NetPrintMem(aTagRslt, sizeof(pTest->aTagRslt), "actual");
            iFail |= 1;
        }
        if ((pTest->iInputSize != 0) && memcmp(pTest->aOutput, aBuffer, pTest->iOutputSize))
        {
            ZPrintf("%s: gcm encrypt test %d failed with invalid output result\n", argv[0], iTest);
            NetPrintMem(pTest->aOutput, pTest->iOutputSize, "expected");
            NetPrintMem(aBuffer, pTest->iOutputSize, "actual");
            iFail |= 1;
        }

        // decrypt
        CryptGcmInit(&Gcm, pTest->aKey, pTest->iKeySize);
        CryptGcmDecrypt(&Gcm, aBuffer, pTest->iInputSize, pTest->aInitVec, sizeof(pTest->aInitVec), pTest->aData, pTest->iDataSize, aTagRslt, sizeof(aTagRslt));

        if (memcmp(pTest->aTagRslt, aTagRslt, sizeof(pTest->aTagRslt)))
        {
            ZPrintf("%s: gcm decrypt test %d failed with invalid tag result\n", argv[0], iTest);
            NetPrintMem(pTest->aTagRslt, sizeof(pTest->aTagRslt), "expected");
            NetPrintMem(aTagRslt, sizeof(pTest->aTagRslt), "actual");
            iFail |= 1;
        }
        if ((pTest->iInputSize != 0) && memcmp(pTest->aInput, aBuffer, pTest->iInputSize))
        {
            ZPrintf("%s: gcm decrypt test %d failed with invalid output result\n", argv[0], iTest);
            NetPrintMem(pTest->aInput, pTest->iInputSize, "expected");
            NetPrintMem(aBuffer, pTest->iOutputSize, "actual");
            iFail |= 1;
        }

        ZPrintf("%s: test #%d %s\n", argv[0], iTest, iFail ? "failed" : "passed");

        // add to failed count
        iFailed += iFail;
    }

    ZPrintf("%s: ---------------------\n", argv[0]);
    return(iFailed);
}

/*F********************************************************************************/
/*!
    \Function _CmdCryptTestMurmurHash3

    \Description
        Test MurmurHash3

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 11/04/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestMurmurHash3(ZContext *argz, int32_t argc, char *argv[])
{
    char strHashBuf0[16], strHashBuf1[16];
    int32_t iFailed = 0, iCount, iString, iDataSize, iNumIter;
    uint32_t uStartTick;
    const uint8_t aKeyBuf[16] = // init vector from MurmurHash3Init, little-endian 64bit words
    {
        0x89,0xab,0xcd,0xef,0x01,0x23,0x45,0x67,
        0x76,0x54,0x32,0x10,0xfe,0xdc,0xba,0x98,
    };
    MurmurHash3T MurmurCtx;

    ZPrintf("%s: testing MurmurHash3 with a %d byte random buffer", argv[0], sizeof(_Crypt_aLargeRandom));

    // test versions for compatibility; first the crypt-style Init/Update/Final
    MurmurHash3Init(&MurmurCtx);
    MurmurHash3Update(&MurmurCtx, _Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom));
    MurmurHash3Final(&MurmurCtx, strHashBuf0, sizeof(strHashBuf0));
    // now the all-in-one-go version
    MurmurHash3(strHashBuf1, sizeof(strHashBuf1), _Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom), aKeyBuf, sizeof(aKeyBuf));
    // test result
    if (memcmp(strHashBuf0, strHashBuf1, sizeof(strHashBuf0)))
    {
        ZPrintf("; hash failed!\n");
        NetPrintMem(strHashBuf0, sizeof(strHashBuf0), "hash0 result");
        NetPrintMem(strHashBuf1, sizeof(strHashBuf1), "hash1 result");
        iFailed += 1;
    }
    else
    {
        ZPrintf("; success\n");
    }

    ZPrintf("%s: testing MurmurHash3 with a %d byte random buffer and random sizes", argv[0], sizeof(_Crypt_aLargeRandom));

    // now test crypt version with multiple updates that are not 16-byte aligned
    MurmurHash3Init(&MurmurCtx);
    for (iCount = 0, iNumIter = 0; iCount <  (int32_t)sizeof(_Crypt_aLargeRandom); iCount += iDataSize, iNumIter += 1)
    {
        CryptRandGet((uint8_t *)&iDataSize, sizeof(iDataSize));
        iDataSize &= (sizeof(_Crypt_aLargeRandom)-1)/16;
        if (iCount+iDataSize > (int32_t)sizeof(_Crypt_aLargeRandom))
        {
            iDataSize = sizeof(_Crypt_aLargeRandom)-iCount;
        }
        MurmurHash3Update(&MurmurCtx, _Crypt_aLargeRandom+iCount, iDataSize);
    }
    MurmurHash3Final(&MurmurCtx, strHashBuf0, sizeof(strHashBuf0));
    // compare to previous result
    if (memcmp(strHashBuf0, strHashBuf1, sizeof(strHashBuf0)))
    {
        ZPrintf("; hash failed!\n");
        NetPrintMem(strHashBuf0, sizeof(strHashBuf0), "hash0 result");
        NetPrintMem(strHashBuf1, sizeof(strHashBuf1), "hash1 result");
        iFailed += 1;
    }
    else
    {
        ZPrintf("; success (%d iterations)\n", iNumIter);
    }

    // test all strings
    for (iString = 0; iString < (signed)(sizeof(_Crypt_Sha2Test)/sizeof(_Crypt_Sha2Test[0])); iString += 1)
    {
        ZPrintf("%s: hashing \"%s\"", argv[0], _Crypt_Sha2Test[iString].pString);

        // test versions for compatibility; first the crypt-style Init/Update/Final
        MurmurHash3Init(&MurmurCtx);
        MurmurHash3Update(&MurmurCtx, (uint8_t *)_Crypt_Sha2Test[iString].pString, (uint32_t)strlen(_Crypt_Sha2Test[iString].pString));
        MurmurHash3Final(&MurmurCtx, strHashBuf0, sizeof(strHashBuf0));
        // now the all-in-one-go version
        MurmurHash3(strHashBuf1, sizeof(strHashBuf1), (uint8_t *)_Crypt_Sha2Test[iString].pString, (uint32_t)strlen(_Crypt_Sha2Test[iString].pString), aKeyBuf, sizeof(aKeyBuf));

        if (memcmp(strHashBuf0, strHashBuf1, sizeof(strHashBuf0)))
        {
            ZPrintf("; hash failed!\n");
            NetPrintMem(strHashBuf0, sizeof(strHashBuf0), "hash0 result");
            NetPrintMem(strHashBuf1, sizeof(strHashBuf1), "hash1 result");
            iFailed += 1;
        }
        else
        {
            ZPrintf("; success\n");
        }
    }

    // timing tests

    // time atomic version
    ZPrintf("%s: hashing %d bytes of random data %d times for timing test (atomic version); ", argv[0], sizeof(_Crypt_aLargeRandom), _Crypt_iTimingIter);
    for (iCount = 0, uStartTick = NetTick(); iCount < _Crypt_iTimingIter; iCount += 1)
    {
        MurmurHash3(strHashBuf0, sizeof(strHashBuf0), _Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom)-16, aKeyBuf, sizeof(aKeyBuf));
    }
    ZPrintf("%dms\n", NetTickDiff(NetTick(), uStartTick));

    // time crypt-style version
    ZPrintf("%s: hashing %d bytes of random data %d times for timing test (crypt version); ", argv[0], sizeof(_Crypt_aLargeRandom), _Crypt_iTimingIter);
    for (iCount = 0, uStartTick = NetTick(); iCount < _Crypt_iTimingIter; iCount += 1)
    {
        MurmurHash3Init(&MurmurCtx);
        MurmurHash3Update(&MurmurCtx, _Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom));
        MurmurHash3Final(&MurmurCtx, strHashBuf0, sizeof(strHashBuf0));
    }
    ZPrintf("%dms\n", NetTickDiff(NetTick(), uStartTick));

    ZPrintf("%s: ---------------------\n", argv[0]);
    return(iFailed);
}

/*F********************************************************************************/
/*!
    \Function _CmdCryptTestMD5

    \Description
        Test the CryptMD5 module

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 11/04/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestMD5(ZContext *argz, int32_t argc, char *argv[])
{
    char strHashBuf[MD5_BINARY_OUT];
    int32_t iFailed = 0, iCount;
    uint32_t uStartTick;
    CryptMD5T MD5;

    ZPrintf("%s: testing CryptMD5\n", argv[0]);

    // do a timing test

    // test all modes
    ZPrintf("%s: hashing %d bytes of random data %d times for timing test; ", argv[0], sizeof(_Crypt_aLargeRandom), _Crypt_iTimingIter);
    uStartTick = NetTick();

    for (iCount = 0; iCount < _Crypt_iTimingIter; iCount += 1)
    {
        CryptMD5Init(&MD5);
        CryptMD5Update(&MD5, _Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom));
        CryptMD5Final(&MD5, (uint8_t *)strHashBuf, sizeof(strHashBuf));
    }

    ZPrintf("%dms\n", NetTickDiff(NetTick(), uStartTick));

    ZPrintf("%s: ---------------------\n", argv[0]);
    return(iFailed);
}

/*F********************************************************************************/
/*!
    \Function _CmdCryptTestSha1

    \Description
        Test the CryptSha1 module

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 11/04/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestSha1(ZContext *argz, int32_t argc, char *argv[])
{
    char strHashBuf[20];
    int32_t iFailed = 0, iCount;
    uint32_t uStartTick;
    CryptSha1T Sha1;

    ZPrintf("%s: testing CryptSha1\n", argv[0]);

    // do a timing test

    // test all modes
    ZPrintf("%s: hashing %d bytes of random data %d times for timing test; ", argv[0], sizeof(_Crypt_aLargeRandom), _Crypt_iTimingIter);
    uStartTick = NetTick();

    for (iCount = 0; iCount < _Crypt_iTimingIter; iCount += 1)
    {
        CryptSha1Init(&Sha1);
        CryptSha1Update(&Sha1, _Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom));
        CryptSha1Final(&Sha1, (uint8_t *)strHashBuf, sizeof(strHashBuf));
    }

    ZPrintf("%dms\n", NetTickDiff(NetTick(), uStartTick));

    ZPrintf("%s: ---------------------\n", argv[0]);
    return(iFailed);
}

/*F********************************************************************************/
/*!
    \Function _CmdCryptTestSha2

    \Description
        Test the CryptSha2 module

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 11/04/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestSha2(ZContext *argz, int32_t argc, char *argv[])
{
    char strHashBuf[128];
    static const uint8_t _HashSizes[] = { CRYPTSHA224_HASHSIZE, CRYPTSHA256_HASHSIZE, CRYPTSHA384_HASHSIZE, CRYPTSHA512_HASHSIZE };
    int32_t iFailed, iString, iMode, iCount;
    uint32_t uStartTick;
    CryptSha2T Sha2;

    ZPrintf("%s: testing CryptSha2\n", argv[0]);

    // test all strings
    for (iFailed = 0, iString = 0; iString < (signed)(sizeof(_Crypt_Sha2Test)/sizeof(_Crypt_Sha2Test[0])); iString += 1)
    {
        // test all modes
        for (iMode = 0; iMode < 4; iMode += 1)
        {
            ZPrintf("%s: hashing \"%s\" (mode %d)", argv[0], _Crypt_Sha2Test[iString].pString, iMode);

            CryptSha2Init(&Sha2, _HashSizes[iMode]);
            CryptSha2Update(&Sha2, (uint8_t *)_Crypt_Sha2Test[iString].pString, (uint32_t)strlen(_Crypt_Sha2Test[iString].pString));
            CryptSha2Final(&Sha2, (uint8_t *)strHashBuf, _HashSizes[iMode]);

            if (memcmp(strHashBuf, _Crypt_Sha2Test[iString].strHashRslt[iMode], (uint32_t)_HashSizes[iMode]))
            {
                ZPrintf("; hash failed!\n");
                NetPrintMem(strHashBuf, (uint32_t)_HashSizes[iMode], "hash result");
                NetPrintMem(_Crypt_Sha2Test[iString].strHashRslt[iMode], (uint32_t)_HashSizes[iMode], "expected result");
                iFailed += 1;
            }
            else
            {
                ZPrintf("; success\n");
            }
        }
    }

    // do a timing test

    // test all modes
    for (iMode = 0; iMode < 4; iMode += 1)
    {
        ZPrintf("%s: hashing %d bytes of random data %d times for timing test (mode %d); ", argv[0], sizeof(_Crypt_aLargeRandom), _Crypt_iTimingIter, iMode);
        uStartTick = NetTick();

        for (iCount = 0; iCount < _Crypt_iTimingIter; iCount += 1)
        {
            CryptSha2Init(&Sha2, _HashSizes[iMode]);
            CryptSha2Update(&Sha2, _Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom));
            CryptSha2Final(&Sha2, (uint8_t *)strHashBuf, _HashSizes[iMode]);
        }

        ZPrintf("%dms\n", NetTickDiff(NetTick(), uStartTick));
    }

    ZPrintf("%s: ---------------------\n", argv[0]);
    return(iFailed);
}

/*F********************************************************************************/
/*!
    \Function _CmdCryptTestHmac

    \Description
        Test the CryptHmac module

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 11/05/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestHmac(ZContext *argz, int32_t argc, char *argv[])
{
    uint8_t strHmacBuf[128];
    int32_t iFailed, iString, iMode, iCount;
    uint32_t uStartTick;
    int32_t iHashSize;

    ZPrintf("%s: testing CryptHmac\n", argv[0]);

    // test all strings
    for (iFailed = 0, iString = 0; iString < (signed)(sizeof(_Crypt_HmacTest)/sizeof(_Crypt_HmacTest[0])); iString += 1)
    {
        // test all modes
        for (iMode = CRYPTHASH_MURMUR3; iMode <= CRYPTHASH_SHA512; iMode += 1)
        {
            ZPrintf("%s: hashing \"%s\" (mode %d)", argv[0], _Crypt_HmacTest[iString].pString, iMode);

            CryptHmacCalc(strHmacBuf, (int32_t)sizeof(strHmacBuf), (uint8_t *)_Crypt_HmacTest[iString].pString, (int32_t)strlen(_Crypt_HmacTest[iString].pString), (uint8_t *)_Crypt_HmacTest[iString].pKey, (int32_t)strlen(_Crypt_HmacTest[iString].pKey), iMode);
            iHashSize = CryptHashGetSize((CryptHashTypeE)iMode);

            if (memcmp(strHmacBuf, _Crypt_HmacTest[iString].strHmacRslt[iMode-1], iHashSize))
            {
                ZPrintf("; hmac failed!\n");
                NetPrintMem(strHmacBuf, iHashSize, "hmac result");
                NetPrintMem(_Crypt_HmacTest[iString].strHmacRslt[iMode-1], iHashSize, "expected result");
                iFailed += 1;
            }
            else
            {
                ZPrintf("; success\n");
            }
        }
    }

    // do a timing test

    // test all modes
    for (iMode = CRYPTHASH_MURMUR3, iString = 0; iMode <= CRYPTHASH_SHA512; iMode += 1)
    {
        ZPrintf("%s: hashing %d bytes of random data %d times for timing test (mode %d); ", argv[0], sizeof(_Crypt_aLargeRandom), _Crypt_iTimingIter, iMode);
        uStartTick = NetTick();

        for (iCount = 0; iCount < _Crypt_iTimingIter; iCount += 1)
        {
            CryptHmacCalc(strHmacBuf, (int32_t)sizeof(strHmacBuf), _Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom), (uint8_t *)_Crypt_HmacTest[iString].pKey, (int32_t)strlen(_Crypt_HmacTest[iString].pKey), (CryptHashTypeE)iMode);
        }

        ZPrintf("%dms\n", NetTickDiff(NetTick(), uStartTick));
    }
    ZPrintf("%s: ---------------------\n", argv[0]);
    return(iFailed);
}

/*F********************************************************************************/
/*!
    \Function _CmdCryptTestRSA

    \Description
        Test the CryptRSA module

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output
        int32_t     - zero=success, else number of failed tests

    \Version 11/22/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdCryptTestRSA(ZContext *argz, int32_t argc, char *argv[])
{
#if 0
/* test with small numbers:
   p=1289
   q=997
   n=pq=1285133
   (p-1)(q-1)=1282848
   e=5
   d=769709

   encryption: c = m^e (mod n) = m^5 % 1285133
   decryption: m = c^d (mod n) = c^769709 % 1285133
*/
    #if defined(__SNC__) && 0 // ps3-sn work-around for invalid compiler error 371; variable "RSAEncrypt" has an uninitialized const field
    CryptRSAT RSAEncrypt = { 0, 0, { 0 }, { 0 }, { 0 }, NULL, NULL, NULL, NULL, NULL, { 0, 0 } };
    CryptRSAT RSADecrypt = { 0, 0, { 0 }, { 0 }, { 0 }, NULL, NULL, NULL, NULL, NULL, { 0, 0 } };
    #else
    CryptRSAT RSAEncrypt, RSADecrypt;
    #endif
    const uint8_t n[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x13, 0x9c, 0x0d }; // 1285133
    const uint8_t e[] = { 0x05 }; // 5
    const uint8_t d[] = { 0x0b, 0xbe, 0xad }; // 769709
    //const uint8_t p[] = { 0x00, 0x00, 0x05, 0x09 }; // 1289
    //const uint8_t q[] = { 0x00, 0x00, 0x03, 0xe5 }; // 997
    const uint8_t m[] = { 0xa1, 0xb2 }; // message to encrypt (41394)

    // 41394^5=121530973472949954664224
    // 121530973472949954664224%1285133=884327=0xd7e67

    // do the encryption
    CryptRSAInit(&RSAEncrypt, n, sizeof(n), d, sizeof(d));
    CryptRSAInitMaster(&RSAEncrypt, m, sizeof(m));
    CryptRSAEncrypt(&RSAEncrypt);

    // decrypt and verify
    CryptRSAInit(&RSADecrypt, n, sizeof(n), e, sizeof(e));
    CryptRSAInitSignature(&RSADecrypt, RSAEncrypt.EncryptBlock, sizeof(n));
    CryptRSAEncrypt(&RSADecrypt);

    // compare result
    return(memcmp(RSAEncrypt.EncryptBlock, RSADecrypt.EncryptBlock, sizeof(m)) != 0 ? 1 : 0);
#endif
    return(0);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdCrypt

    \Description
        Test the crypto modules

    \Input *argz   - environment
    \Input argc    - standard number of arguments
    \Input *argv[] - standard arg list

    \Output standard return value

    \Version 01/11/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdCrypt(ZContext *argz, int32_t argc, char *argv[])
{
    int32_t iFailed = 0;

    // populate random buffer
    CryptRandGet(_Crypt_aLargeRandom, sizeof(_Crypt_aLargeRandom));

    iFailed += _CmdCryptTestArc4(argz, argc, argv);
    iFailed += _CmdCryptTestGcm(argz, argc, argv);
    iFailed += _CmdCryptTestMurmurHash3(argz, argc, argv);
    iFailed += _CmdCryptTestMD5(argz, argc, argv);
    iFailed += _CmdCryptTestSha1(argz, argc, argv);
    iFailed += _CmdCryptTestSha2(argz, argc, argv);
    iFailed += _CmdCryptTestHmac(argz, argc, argv);
    iFailed += _CmdCryptTestRSA(argz, argc, argv);

    ZPrintf("%s: %d tests failed\n", argv[0], iFailed);
    return(0);
}


