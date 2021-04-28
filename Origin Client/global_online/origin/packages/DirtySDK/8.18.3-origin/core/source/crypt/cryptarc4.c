/*H********************************************************************************/
/*!
    \File cryptarc4.c

    \Description
        This module is a from-scratch ARC4 implementation designed to avoid
        any intellectual property complications. The ARC4 stream cipher is
        known to produce output that is compatible with the RC4 stream cipher.

    \Notes
        This algorithm from this cypher was taken from the web site:
        ciphersaber.gurus.com.  It is based on the RC4 compatible algorithm that
        was published in the 2nd ed of the book Applied Cryptography by Bruce
        Schneier.  This is a private-key stream cipher which means that some other
        secure way of exchanging cipher keys prior to algorithm use is required.
        Its strength is directly related to the key exchange algorithm strength.
        In operation, each individual stream message must use a unique key.  This
        is handled by appending on 10 byte random value onto the private key.  This
        10-byte data can be sent by public means to the receptor (or included at the
        start of a file or whatever). When the private key is combined with this
        public key, it essentially puts the cypher into a random starting state (it
        is like using a message digest routine to generate a random hash for
        password comparison).  The code below was written from scratch using only
        a textual algorithm description.

    \Copyright
        Copyright (c) 2000-2005 Electronic Arts Inc.

    \Version 1.0 02/25/2000 (gschaefer) First Version
    \Version 1.1 11/06/2002 (jbrookes)  Removed Create()/Destroy() to eliminate mem
                                        alloc dependencies.
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform.h"
#include "cryptarc4.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

// Public variables


/*** Private Functions ************************************************************/

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CryptArc4Init

    \Description
        Initialize state for ARC4 encryption/decryption module.

    \Input *pState  - module state
    \Input *pKeyBuf - pointer to crypt key
    \Input iKeyLen  - length of crypt key (sixteen for RC4-128)
    \Input iIter    - internal iterator (1=RC4 compat, larger for slightly improved
                      security

    \Output
        None.

    \Version 02/26/2000 (gschaefer)
*/
/********************************************************************************F*/
void CryptArc4Init(CryptArc4T *pState, const unsigned char *pKeyBuf, int32_t iKeyLen, int32_t iIter)
{
    uint32_t uWalk;
    unsigned char uSwap;
    unsigned char uTemp0;
    unsigned char uTemp1;

    // clmap iteration count
    if (iIter < 1)
    {
        iIter = 1;
    }

    // reset the permutators
    pState->walk = 0;
    pState->swap = 0;

    // init the state array
    for (uWalk = 0; uWalk < 256; ++uWalk)
    {
        pState->state[uWalk] = (unsigned char)uWalk;
    }

    // only do setup if key is valid
    if (iKeyLen > 0)
    {
        // iterate the setup for added security
        for (uSwap = 0; iIter > 0; --iIter)
        {
            // mixup the state table
            for (uWalk = 0; uWalk < 256; ++uWalk)
            {
                // determine the swap point
                uSwap += pState->state[uWalk] + pKeyBuf[uWalk % iKeyLen];
                // swap the entries
                uTemp0 = pState->state[uWalk];
                uTemp1 = pState->state[uSwap];
                pState->state[uWalk] = uTemp1;
                pState->state[uSwap] = uTemp0;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function CryptArc4Apply

    \Description
        Apply the cipher to the data. Uses reversible XOR so calling twice undoes
        the uncryption (assuming the key state has been reset).

    \Input *pState  - module state
    \Input *pBuffer - buffer to encrypt/decrypt
    \Input iLength  - length of buffer

    \Output
        None.

    \Version 02/26/2000 (gschaefer)
*/
/********************************************************************************F*/
void CryptArc4Apply(CryptArc4T *pState, unsigned char *pBuffer, int32_t iLength)
{
    uint32_t uTemp0;
    uint32_t uTemp1;
    uint32_t uWalk = pState->walk;
    uint32_t uSwap = pState->swap;

    // do each data byte
    for (; iLength > 0; --iLength)
    {
        // determine the swap points
        uWalk = (uWalk+1)&255;
        uSwap = (uSwap+pState->state[uWalk])&255;

        // swap the state entries
        uTemp0 = pState->state[uWalk];
        uTemp1 = pState->state[uSwap];
        pState->state[uWalk] = uTemp1;
        pState->state[uSwap] = uTemp0;

        // apply the cypher
        *pBuffer++ ^= pState->state[(uTemp0+uTemp1)&255];
    }

    // update the state
    pState->walk = uWalk;
    pState->swap = uSwap;
}

/*F********************************************************************************/
/*!
    \Function CryptArc4Advance

    \Description
        Advance the cipher state with by iLength bytes.

    \Input *pState  - module state
    \Input iLength  - length to advance

    \Output
        None.

    \Version 12/06/2005 (jbrookes)
*/
/********************************************************************************F*/
void CryptArc4Advance(CryptArc4T *pState, int32_t iLength)
{
    uint32_t uTemp0;
    uint32_t uTemp1;
    uint32_t uWalk = pState->walk;
    uint32_t uSwap = pState->swap;

    // do each data byte
    for (; iLength > 0; --iLength)
    {
        // determine the swap points
        uWalk = (uWalk+1)&255;
        uSwap = (uSwap+pState->state[uWalk])&255;

        // swap the state entries
        uTemp0 = pState->state[uWalk];
        uTemp1 = pState->state[uSwap];
        pState->state[uWalk] = uTemp1;
        pState->state[uSwap] = uTemp0;
    }

    // update the state
    pState->walk = uWalk;
    pState->swap = uSwap;
}

