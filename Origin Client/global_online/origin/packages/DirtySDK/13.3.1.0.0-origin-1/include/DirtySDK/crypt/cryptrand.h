/*H********************************************************************************/
/*!
    \File cryptrand.h

    \Description
        Cryptographic random number generator.  Uses system-defined rng where
        available.

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 12/05/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _cryptrand_h
#define _cryptrand_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// initialize the module
void CryptRandInit(void);

// destroy the module
void CryptRandShutdown(void);

// get random data
void CryptRandGet(uint8_t *pBuffer, int32_t iBufSize);

#ifdef __cplusplus
}
#endif

#endif // _cryptrand_h

