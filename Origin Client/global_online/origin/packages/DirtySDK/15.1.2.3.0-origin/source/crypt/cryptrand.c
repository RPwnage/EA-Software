/*H********************************************************************************/
/*!
    \File cryptrand.c

    \Description
        Cryptographic random number generator.  Uses system-defined rng where
        available.

    \Notes
        Native implementations used on: Apple OSX, Apple iOS, Linux, Windows

        References: http://tools.ietf.org/html/rfc4086
                    http://randomnumber.org/links.htm

    \Todo
        - Native implementations for: Android, PS4, WinPRT
        - Improve generic prng

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

#if defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX)
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(DIRTYCODE_APPLEIOS)
#include <errno.h>
#include <Security/Security.h>
#elif defined(DIRTYCODE_XBOXONE)
#include <bcrypt.h>
#elif defined(DIRTYCODE_PS4)
#include <libsecure.h>
#elif defined(DIRTYCODE_PC)
#include <wincrypt.h>
#endif

/*** Defines **********************************************************************/

// not enabled currently due to singleton nature of libsecure; to enable comment in and link against libSceSecure
#define CRYPTRAND_PS4_ENABLED (FALSE)

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#if defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX)
static int32_t _CryptRand_iFd = -1;
#elif defined(DIRTYCODE_APPLEIOS)
// no variables required
#elif defined(DIRTYCODE_XBOXONE)
static BCRYPT_ALG_HANDLE _CryptRand_hAlgProvider = 0;
#elif defined(DIRTYCODE_PS4) && CRYPTRAND_PS4_ENABLED
static uint8_t _CryptRand_strBuffer[5 * 1024];
static uint8_t _CryptRand_bLibActive = FALSE;
#elif defined(DIRTYCODE_PC)
static HCRYPTPROV _CryptRand_hCryptCtx = 0;
#endif

// random data buffer for generic implementation
static uint32_t _CryptRand_aRandom[4] = { 0, 0, 0, 0 };
static CryptArc4T _CryptRand_Arc4;

/*** Private Functions ************************************************************/


/*F**************************************************************************/
/*!
    \Function _CryptRandGetGeneric

    \Description
        Generate random data

    \Input *pRandomBuf - output of random data
    \Input iRandomLen  - length of random data
    \Input *pArc4      - temp buffer for ARC4 use

    \Version 11/05/2004 gschaefer (From ProtoSSL)
*/
/**************************************************************************F*/
static void _CryptRandGetGeneric(CryptArc4T *pArc4, uint8_t *pRandomBuf, int32_t iRandomLen)
{
    int32_t iCount;

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
    #elif defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX)
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
    #elif defined(DIRTYCODE_PS4) && CRYPTRAND_PS4_ENABLED
    {
        SceLibSecureBlock SceMemBlock = {_CryptRand_strBuffer, sizeof(_CryptRand_strBuffer)};
        SceLibSecureErrorType iError;
        // buffer passed to sceLibSecureInit() should contain random data
        _CryptRandGetGeneric(&_CryptRand_Arc4, _CryptRand_strBuffer, sizeof(_CryptRand_strBuffer));
        iError = sceLibSecureInit(SCE_LIBSECURE_FLAGS_RANDOM_GENERATOR, &SceMemBlock);
        if (iError == SCE_LIBSECURE_OK)
        {
            NetPrintf(("cryptrand: sceLibSecureInit succeeded (err=%s)\n", DirtyErrGetName(iError)));
            _CryptRand_bLibActive = TRUE;
        }
        else
        {
            NetPrintf(("cryptrand: sceLibSecureInit failed (err=%s); general-purpose code will be used instead\n", DirtyErrGetName(iError)));
        }
    }
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
    #elif defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX)
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
    #elif defined(DIRTYCODE_PS4) && CRYPTRAND_PS4_ENABLED
    if (_CryptRand_bLibActive)
    {
        sceLibSecureDestroy();
        _CryptRand_bLibActive = FALSE;
    }
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
    #elif defined(DIRTYCODE_APPLEOSX) || defined(DIRTYCODE_LINUX)
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
    #elif defined(DIRTYCODE_PS4) && CRYPTRAND_PS4_ENABLED
    if (_CryptRand_bLibActive)
    {
        SceLibSecureBlock SceSecureBlock = { pBuffer, iBufSize };
        int32_t iResult;
        if ((iResult = sceLibSecureRandom(&SceSecureBlock)) == SCE_LIBSECURE_OK)
        {
            NetPrintMem(pBuffer, iBufSize, "cryptrand");
            return;
        }
        NetPrintf(("cryptrand: sceLibSecureRandom() failed (err=%s); falling through to general-purpose code\n", DirtyErrGetName(iResult)));
    }
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


