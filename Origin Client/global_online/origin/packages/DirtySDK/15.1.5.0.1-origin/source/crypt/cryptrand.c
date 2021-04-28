/*H********************************************************************************/
/*!
    \File cryptrand.c

    \Description
        Cryptographic random number generator.  Uses system-defined rng where
        available.

    \Notes
        Native implementations used on: Android, Apple OSX, Apple iOS, Linux, PS4, Windows

        References: http://tools.ietf.org/html/rfc4086
                    http://randomnumber.org/links.htm

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 12/05/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/cryptrand.h"

#if defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(DIRTYCODE_APPLEIOS)
#include <errno.h>
#include <Security/Security.h>
#elif defined(DIRTYCODE_XBOXONE)
#include <bcrypt.h>
#elif defined(DIRTYCODE_PS4)
#include <sce_random.h>
#elif defined(DIRTYCODE_PC)
#include <wincrypt.h>
#endif

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#if defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
static int32_t _CryptRand_iFd = -1;
#elif defined(DIRTYCODE_APPLEIOS)
// no variables required
#elif defined(DIRTYCODE_XBOXONE)
static BCRYPT_ALG_HANDLE _CryptRand_hAlgProvider = 0;
#elif defined(DIRTYCODE_PS4)
// no variables required
#elif defined(DIRTYCODE_PC)
static HCRYPTPROV _CryptRand_hCryptCtx = 0;
#endif

// random data buffer for generic implementation
static uint32_t _CryptRand_aRandom[4] = { 0, 0, 0, 0 };
static CryptArc4T _CryptRand_Arc4;

/*** Private Functions ************************************************************/


/*F**************************************************************************/
/*!
    \Function _CryptRandGenSeed

    \Description
        Generate seed for generic random

    \Version 05/31/2017 (eesponda)
*/
/**************************************************************************F*/
static void _CryptRandGenSeed(void)
{
    int32_t iCount;

    #if defined(DIRTYCODE_PS4)
    int32_t iResult;

    /* since the ps4 random number generator can only generate up to 64 bytes we instead use it
       to seed the generic random implementation */
    if ((iResult = sceRandomGetRandomNumber(_CryptRand_aRandom, sizeof(_CryptRand_aRandom))) == SCE_OK)
    {
        return;
    }
    NetPrintf(("cryptrand: sceRandomGetRandomNumer failed (%s); general-purpose code will be seeding using default NetTick()\n", DirtyErrGetName(iResult)));
    #endif

    // if first time through, init the base ticker
    if (_CryptRand_aRandom[0] == 0)
    {
        _CryptRand_aRandom[0] = NetTick();
    }

    // always set second time
    _CryptRand_aRandom[1] += NetTick();

    // increment sequence numver
    _CryptRand_aRandom[2] += 1;

    // grab data from stack (might be random, might not)
    for (iCount = 0; iCount < 32; ++iCount)
    {
        _CryptRand_aRandom[3] += (&iCount)[iCount];
    }
}

/*F**************************************************************************/
/*!
    \Function _CryptRandGetGeneric

    \Description
        Generate random data

    \Input *pArc4       - temp buffer for ARC4 use
    \Input *pRandomBuf  - output of random data
    \Input iRandomLen   - length of random data

    \Version 11/05/2004 gschaefer (From ProtoSSL)
*/
/**************************************************************************F*/
static void _CryptRandGetGeneric(CryptArc4T *pArc4, uint8_t *pRandomBuf, int32_t iRandomLen)
{
    // generate the random seed
    _CryptRandGenSeed();

    // generate data if desired
    if (pRandomBuf != NULL)
    {
        // init the stream cipher for use as random generator
        CryptArc4Init(pArc4, (uint8_t *)_CryptRand_aRandom, sizeof(_CryptRand_aRandom), 3);
        // output pseudo-random data
        CryptArc4Apply(pArc4, pRandomBuf, iRandomLen);
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptRandInit

    \Description
        Initialize CryptRand module

    \Version 12/05/2012 (jbrookes)
*/
/********************************************************************************F*/
void CryptRandInit(void)
{
    // init generic rng, used as fallback if a platform-specific version is not provided or is unavailable
    _CryptRandGetGeneric(&_CryptRand_Arc4, NULL, 0);

    // now init platform-specific RNG

    #if defined(DIRTYCODE_APPLEIOS)
    // no initialization required
    #elif defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
    if ((_CryptRand_iFd = open("/dev/urandom", O_RDONLY)) < 0)
    {
        NetPrintf(("cryptrand: open of /dev/urandom failed (err=%s); general-purpose code will be used instead\n", DirtyErrGetName(errno)));
    }
    #elif defined(DIRTYCODE_XBOXONE)
    if (BCryptOpenAlgorithmProvider(&_CryptRand_hAlgProvider, BCRYPT_RNG_ALGORITHM, NULL, 0) != S_OK)
    {
        NetPrintf(("cryptrand: BCryptOpenAlgorithmProvider failed (err=%s); general-purpose code will be used instead\n", DirtyErrGetName(GetLastError())));
        _CryptRand_hAlgProvider = 0;
    }
    #elif defined(DIRTYCODE_PS4)
    // no initialization required
    #elif defined(DIRTYCODE_PC)
    {
        BOOL bResult = CryptAcquireContext(&_CryptRand_hCryptCtx, NULL, NULL, PROV_RSA_FULL, 0);
        if ((bResult == FALSE) && (GetLastError() == NTE_BAD_KEYSET))
        {
            if (!CryptAcquireContext(&_CryptRand_hCryptCtx, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET))
            {
                NetPrintf(("cryptrand: CryptAcquireContext failed (err=%d); general-purpose code will be used instead\n", GetLastError()));
                _CryptRand_hCryptCtx = 0;
            }
        }
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function CryptRandShutdown

    \Description
        Shut down the CryptRand module

    \Version 12/05/2012 (jbrookes)
*/
/********************************************************************************F*/
void CryptRandShutdown(void)
{
    #if defined(DIRTYCODE_APPLEIOS)
    // no shutdown required
    #elif defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
    if (_CryptRand_iFd >= 0)
    {
        close(_CryptRand_iFd);
        _CryptRand_iFd = -1;
    }
    #elif defined(DIRTYCODE_XBOXONE)
    if (_CryptRand_hAlgProvider != 0)
    {
        BCryptCloseAlgorithmProvider(_CryptRand_hAlgProvider, 0);
        _CryptRand_hAlgProvider = 0;
    }
    #elif defined(DIRTYCODE_PS4)
    // no shutdown required
    #elif defined(DIRTYCODE_PC)
    if (_CryptRand_hCryptCtx != 0)
    {
        CryptReleaseContext(_CryptRand_hCryptCtx, 0);
        _CryptRand_hCryptCtx = 0;
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function CryptRandGet

    \Description
        Get random data

    \Version 12/05/2012 (jbrookes)
*/
/********************************************************************************F*/
void CryptRandGet(uint8_t *pBuffer, int32_t iBufSize)
{
    #if defined(DIRTYCODE_APPLEIOS)
    int32_t iResult;
    if ((iResult = SecRandomCopyBytes(kSecRandomDefault, iBufSize, pBuffer)) >= 0)
    {
        return;
    }
    NetPrintf(("cryptrand: SecRandomCopyBytes() failed (err=%s); falling through to general-purpose code\n", DirtyErrGetName(errno)));
    #elif defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
    int32_t iResult;
    if (_CryptRand_iFd >= 0)
    {
        if ((iResult = read(_CryptRand_iFd, pBuffer, iBufSize)) == iBufSize)
        {
            return;
        }
        NetPrintf(("cryptrand: read(/dev/urandom) failed (err=%s); falling through to general-purpose code\n", DirtyErrGetName(errno)));
    }
    #elif defined(DIRTYCODE_XBOXONE)
    if (_CryptRand_hAlgProvider != 0)
    {
        if (BCryptGenRandom(_CryptRand_hAlgProvider, pBuffer, iBufSize, 0) == S_OK)
        {
            return;
        }
        NetPrintf(("cryptrand: BCryptGenRandom() failed (err=%d); falling through to general-purpose code\n", GetLastError()));
    }
    #elif defined(DIRTYCODE_PS4)
    // fallthrough to generic rand
    #elif defined(DIRTYCODE_PC)
    if (_CryptRand_hCryptCtx != 0)
    {
        if (CryptGenRandom(_CryptRand_hCryptCtx, iBufSize, pBuffer))
        {
            return;
        }
        NetPrintf(("cryptrand: CryptGenRandom() failed (err=%d); falling through to general-purpose code\n", GetLastError()));
    }
    #endif

    // general-purpose code; may be used on platforms with specific implementations in case of a failure condition
    _CryptRandGetGeneric(&_CryptRand_Arc4, pBuffer, iBufSize);
}


