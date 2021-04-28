/*H********************************************************************************/
/*!
    \File cryptaes.h

    \Description
        An implementation of the AES-128 and AES-256 cipher, based on the FIPS
        standard, intended for use with the TLS AES cipher suites.

        This implementation deliberately uses the naming conventions from the
        standard where possible in order to aid comprehension.

    \Notes
        References:
            FIPS197 standard: http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf

    \Copyright
        Copyright (c) 2011 Electronic Arts Inc.

    \Version 01/20/2011 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _cryptaes_h
#define _cryptaes_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define CRYPTAES_MAXROUNDS          (14)

#define CRYPTAES_KEYTYPE_ENCRYPT    (0)
#define CRYPTAES_KEYTYPE_DECRYPT    (1)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

// opaque module state
typedef struct CryptAesT CryptAesT;

//! all fields are PRIVATE
struct CryptAesT
{
    uint16_t uNumRounds;
    uint16_t uKeyWords;
    uint32_t aKeySchedule[(CRYPTAES_MAXROUNDS+1)*8];
    uint8_t  aInitVec[16];                  //!< initialization vector/CBC state
};

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// initialize state for AES encryption/decryption module
void CryptAesInit(CryptAesT *pAes, const uint8_t *pKeyBuf, int32_t iKeyLen, uint32_t uKeyType, const uint8_t *pInitVec);

// encrypt data with the AES cipher
void CryptAesEncrypt(CryptAesT *pAes, uint8_t *pBuffer, int32_t iLength);

// decrypt data with the AES cipher
void CryptAesDecrypt(CryptAesT *pAes, uint8_t *pBuffer, int32_t iLength);

#ifdef __cplusplus
}
#endif

#endif
