/*H********************************************************************************/
/*!
    \File cryptrsa.c

    \Description
        This module is a from-scratch RSA implementation in order to avoid
        any intellectual property issues. The 1024 bit RSA public key
        encryption algorithm was implemented from a specification provided
        by Netscape for SSL implementation (see protossl.h).

    \Copyright
        Copyright (c) Electronic Arts 2002-2011.

    \Todo
        64bit word support (UCRYPT_SIZE=8) has been tested to work with the unix64-gcc
        (Linux) and ps3-gcc (PS3) compilers; however the current implementation does
        not work with certain odd modulus sizes (for example the 1000-bit modulus of
        the built-in RSA CA certificate), so it is not currently enabled.

    \Version 1.0 03/08/2002 (gschaefer) First Version (protossl)
    \Version 1.1 03/05/2003 (jbrookes)  Split RSA encryption from protossl
    \Version 1.2 11/16/2011 (jbrookes)  Optimizations to improve dbg&opt performance
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <string.h>             // memset

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/crypt/cryptrand.h"

#include "DirtySDK/crypt/cryptrsa.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables


// Public variables


/*** Private functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _Exponentiate

    \Description
        Do the actual encryption: result = (value ^ exponent) % modulus.
        Exponentiation is done one exponent bit at a time, this function needs to
        be called iteratively until the function returns zero.

    \Input *pState      - crypt state

    \Output
        int32_t         - zero=done, else call again

    \Version 03/08/2002 (jbrookes) Rewrote original gschaefer version to be iterative
*/
/********************************************************************************H*/
static int32_t _Exponentiate(CryptRSAT *pState)
{
    uint64_t uTickUsecs = NetTickUsec();
    int32_t iResult = 1;

    // read next exponent byte
    if (pState->iExpBitIndex == 0)
    {
        pState->uExponent = pState->KeyExpData[pState->iExpByteIndex];
    }
        
    // perform exponentiation
    if (pState->uExponent & 1)
    {
        if (!pState->bAccumulOne)
        {
            CryptBnModMultiply(&pState->Accumul, &pState->Accumul, &pState->Powerof, &pState->Modulus);
        }
        else // optimization; we can skip the first multiply if the accumulator is set to one
        {
            CryptBnClone(&pState->Accumul, &pState->Powerof);
            pState->bAccumulOne = FALSE;
        }
    }

    // consume exponent bit
    pState->iExpBitIndex += 1;
    pState->uExponent >>= 1;

    // square the input for each exponent bit
    if ((pState->iExpByteIndex > 0) || (pState->uExponent != 0))
    {
        // this would be a shift in normal multiplication, but we use modular multiply in order to preserve the modulus property
        CryptBnModMultiply(&pState->Powerof, &pState->Powerof, &pState->Powerof, &pState->Modulus);
    }
    else
    {
        // we can skip unnecessary multiplies if there are no more exponent bits
        pState->iExpByteIndex = -1;
    }

    // get the next exponent byte
    if (pState->iExpBitIndex == 8)
    {
        pState->iExpByteIndex -= 1;
        pState->iExpBitIndex = 0;
    }

    // return encrypted data when we're done
    if (pState->iExpByteIndex < 0)
    {
        CryptBnFinal(&pState->Accumul, pState->EncryptBlock, pState->iKeyModSize);
        pState->uCryptMsecs = (pState->uCryptUsecs+500)/1000;
        iResult = 0;
    }

    // update timing
    pState->uCryptUsecs += (uint32_t)NetTickDiff(NetTickUsec(), uTickUsecs);
    pState->uNumExpCalls += 1;

    return(iResult);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptRSAInit

    \Description
        Init RSA state.

    \Input *pState      - module state
    \Input *pModulus    - crypto modulus
    \Input iModSize     - size of modulus
    \Input *pExponent   - crypto exponent
    \Input iExpSize     - size of exponent

    \Output
        int32_t         - zero for success, negative for error

    \Notes
        This module supports a max modulus size of 4096 (iModSize=512) and requires
        a minimum size of 1024 (iModSize=64).  The exponent must be between 1
        and 512 bytes in size, inclusive.

    \Version 03/05/03 (jbrookes) Split from protossl
*/
/********************************************************************************H*/
int32_t CryptRSAInit(CryptRSAT *pState, const uint8_t *pModulus, int32_t iModSize, const uint8_t *pExponent, int32_t iExpSize)
{
    int32_t iResult = 0, iWidth;

    // validate modulus size
    if ((iModSize < 64) || (iModSize > (signed)(sizeof(pState->KeyModData)-1)))
    {
        NetPrintf(("cryptrsa: iModSize of %d is invalid\n", iModSize));
        return(-1);
    }
    // validate exponent size
    if ((iExpSize < 1) || (iExpSize > (signed)(sizeof(pState->KeyExpData)-1)))
    {
        NetPrintf(("cryptrsa: iExpSize of %d is invalid\n", iExpSize));
        return(-2);
    }

    // initialize state
    ds_memclr(pState, sizeof(*pState));

    // save params and init state
    pState->iKeyModSize = iModSize;
    pState->iKeyExpSize = iExpSize;
    ds_memcpy(pState->KeyModData, pModulus, iModSize);
    ds_memcpy(pState->KeyExpData, pExponent, iExpSize);

    // copy over the modulus
    iWidth = CryptBnInitFrom(&pState->Modulus, -1, pState->KeyModData, pState->iKeyModSize);

    // initialize accumulator to 1 (and remember for first-multiply optimization)
    pState->bAccumulOne = TRUE;

    // set starting exponent byte index
    pState->iExpByteIndex = pState->iKeyExpSize-1;

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitMaster

    \Description
        Setup the master shared secret for encryption

    \Input *pState      - module state
    \Input *pMaster     - the master shared secret to encrypt
    \Input iMasterLen   - the length of the master shared secret

    \Version 02/07/2014 (jbrookes) Rewritten to use CryptRand
*/
/********************************************************************************H*/
void CryptRSAInitMaster(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen)
{
    int32_t iIndex;
    uint32_t uRandom;

    // fill encrypt block with random data
    CryptRandGet(pState->EncryptBlock, pState->iKeyModSize);
    /* As per PKCS1 http://www.emc.com/emc-plus/rsa-labs/pkcs/files/h11300-wp-pkcs-1v2-2-rsa-cryptography-standard.pdf section 7.2.1,
       the pseudo-random padding octets must be non-zero.  Failing to adhere to this restriction (or any other errors in pkcs1 encoding)
       will typically result in a bad_record_mac fatal alert from the remote host.  The following code gets four random bytes as seed
       and uses a simple Knuth RNG to fill in any zero bytes that were originally generated. */
    CryptRandGet((uint8_t *)&uRandom, sizeof(uRandom));
    for (iIndex = 0; iIndex < pState->iKeyModSize; iIndex += 1)
    {
        while (pState->EncryptBlock[iIndex] == 0)
        {
            uRandom = (uRandom * 69069) + 69069;
            pState->EncryptBlock[iIndex] ^= (uint8_t)uRandom;
        }
    }
    // set PKCS public key signature
    pState->EncryptBlock[0] = 0;        // zero pad
    pState->EncryptBlock[1] = 2;        // public key is type 2
    // add data to encrypt block
    iIndex = pState->iKeyModSize - iMasterLen;
    pState->EncryptBlock[iIndex-1] = 0; // zero byte prior to data
    ds_memcpy(pState->EncryptBlock+iIndex, pMaster, iMasterLen);
    // convert data to encrypt to native word size we will operate in
    CryptBnInitFrom(&pState->Powerof, -1, pState->EncryptBlock, pState->iKeyModSize);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitPrivate

    \Description
        Setup the master shared secret for encryption using PKCS1.5

    \Input *pState      - module state
    \Input *pMaster     - the master shared secret to encrypt
    \Input iMasterLen   - the length of the master shared secret

    \Version 11/03/2013 (jbrookes)
*/
/********************************************************************************H*/
void CryptRSAInitPrivate(CryptRSAT *pState, const uint8_t *pMaster, int32_t iMasterLen)
{
    int32_t iIndex;
    // put in PKCS1.5 signature
    pState->EncryptBlock[0] = 0;
    pState->EncryptBlock[1] = 1;
    // PKCS1.5 signing pads with 0xff
    ds_memset(pState->EncryptBlock+2, 0xff, pState->iKeyModSize-2);
    // add data to encrypt block
    iIndex = pState->iKeyModSize - iMasterLen;
    pState->EncryptBlock[iIndex-1] = 0; // zero byte prior to data
    ds_memcpy(pState->EncryptBlock+iIndex, pMaster, iMasterLen);
    // convert data to encrypt to native word size we will operate in
    CryptBnInitFrom(&pState->Powerof, -1, pState->EncryptBlock, pState->iKeyModSize);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAInitSignature

    \Description
        Setup the encrypted signature for decryption.

    \Input *pState   - module state
    \Input *pSig     - the encrypted signature
    \Input *iSigLen  - the length of the encrypted signature.

    \Version 03/03/2004 (sbevan)
*/
/********************************************************************************H*/
void CryptRSAInitSignature(CryptRSAT *pState, const uint8_t *pSig, int32_t iSigLen)
{
    ds_memcpy(pState->EncryptBlock, pSig, iSigLen);
    // convert data to encrypt to native word size we will operate in
    CryptBnInitFrom(&pState->Powerof, -1, pState->EncryptBlock, pState->iKeyModSize);
}

/*F********************************************************************************/
/*!
    \Function CryptRSAEncrypt

    \Description
        Encrypt data.

    \Input *pState      - module state
    \Input iIter        - number of iterations to execute (zero=do all)

    \Output
        int32_t         - zero=operation complete, else call again

    \Version 03/05/2003 (jbrookes) Split from protossl
*/
/********************************************************************************H*/
int32_t CryptRSAEncrypt(CryptRSAT *pState, int32_t iIter)
{
    int32_t iCount, iResult;
    if (iIter == 0)
    {
        iIter = 0x7fffffff;
    }
    for (iCount = 0, iResult = 1; (iCount < iIter) && (iResult > 0); iCount += 1)
    {
        iResult = _Exponentiate(pState);
    }
    return(iResult);
}


