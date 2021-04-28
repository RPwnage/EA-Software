/*H*************************************************************************************/
/*!
    \File cryptmd2.c

    \Description
        The MD2 message digest algorithm developed by Ron Rivest and documented
        in RFC1319. This implementation is based on the RFC but does not use the
        sample code.  It should be free from intellectual property concerns and
        a reference is included below which further clarifies this point.

        Note that this implementation is limited to hashing no more than 2^32
        bytes after which its results would be incompatible with a fully compliant
        implementation.
        
    \Notes
        http://www.ietf.org/ietf/IPR/RSA-MD-all

        The following was received Fenbruary 23,2000
        From: "Linn, John" <jlinn@rsasecurity.com>

        February 19, 2000

                The purpose of this memo is to clarify the status of intellectual
        property rights asserted by RSA Security Inc. ("RSA") in the MD2, MD4 and
        MD5 message-digest algorithms, which are documented in RFC-1319, RFC-1320,
        and RFC-1321 respectively.

                Implementations of these message-digest algorithms, including
        implementations derived from the reference C code in RFC-1319, RFC-1320, and
        RFC-1321, may be made, used, and sold without license from RSA for any
        purpose.

                No rights other than the ones explicitly set forth above are
        granted.  Further, although RSA grants rights to implement certain
        algorithms as defined by identified RFCs, including implementations derived
        from the reference C code in those RFCs, no right to use, copy, sell, or
        distribute any other implementations of the MD2, MD4, or MD5 message-digest
        algorithms created, implemented, or distributed by RSA is hereby granted by
        implication, estoppel, or otherwise.  Parties interested in licensing
        security components and toolkits written by RSA should contact the company
        to discuss receiving a license.  All other questions should be directed to
        Margaret K. Seif, General Counsel, RSA Security Inc., 36 Crosby Drive,
        Bedford, Massachusetts 01730.

                Implementations of the MD2, MD4, or MD5 algorithms may be subject to
        United States laws and regulations controlling the export of technical data,
        computer software, laboratory prototypes and other commodities (including
        the Arms Export Control Act, as amended, and the Export Administration Act
        of 1970).  The transfer of certain technical data and commodities may
        require a license from the cognizant agency of the United States Government.
        RSA neither represents that a license shall not be required for a particular
        implementation nor that, if required, one shall be issued.


                DISCLAIMER: RSA MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES
        OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
        WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, VALIDITY OF
        INTELLECTUAL PROPERTY RIGHTS, ISSUED OR PENDING, OR THE ABSENCE OF LATENT OR
        OTHER DEFECTS, WHETHER OR NOT DISCOVERABLE, IN CONNECTION WITH THE MD2, MD4,
        OR MD5 ALGORITHMS.  NOTHING IN THIS GRANT OF RIGHTS SHALL BE CONSTRUED AS A
        REPRESENTATION OR WARRANTY GIVEN BY RSA THAT THE IMPLEMENTATION OF THE
        ALGORITHM WILL NOT INFRINGE THE INTELLECTUAL PROPERTY RIGHTS OF ANY THIRD
        PARTY.  IN NO EVENT SHALL RSA, ITS TRUSTEES, DIRECTORS, OFFICERS, EMPLOYEES,
        PARENTS AND AFFILIATES BE LIABLE FOR INCIDENTAL OR CONSEQUENTIAL DAMAGES OF
        ANY KIND RESULTING FROM IMPLEMENTATION OF THIS ALGORITHM, INCLUDING ECONOMIC
        DAMAGE OR INJURY TO PROPERTY AND LOST PROFITS, REGARDLESS OF WHETHER RSA
        SHALL BE ADVISED, SHALL HAVE OTHER REASON TO KNOW, OR IN FACT SHALL KNOW OF

    \Copyright
        Copyright Electronic Arts 2009.

    \Version 03/19/2009 (jbrookes) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/crypt/cryptmd2.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

/*  from the rfc; a permutation of [0...255] generated from the digits of pi, which
    gives a psuedo-random nonlinear byte substitution operation */
static uint8_t _CryptMD2_aPiSubst[256] =
{
     41,  46,  67, 201, 162, 216, 124,   1,  61,  54,  84, 161, 236, 240,   6,  19,
     98, 167,   5, 243, 192, 199, 115, 140, 152, 147,  43, 217, 188,  76, 130, 202,
     30, 155,  87,  60, 253, 212, 224,  22, 103,  66, 111, 24,  138,  23, 229,  18,
    190,  78, 196, 214, 218, 158, 222,  73, 160, 251, 245, 142, 187,  47, 238, 122,
    169, 104, 121, 145,  21, 178,   7,  63, 148, 194,  16, 137,  11,  34,  95,  33,
    128, 127,  93, 154,  90, 144,  50,  39,  53,  62, 204, 231, 191, 247, 151,   3,
    255,  25,  48, 179,  72, 165, 181, 209, 215,  94, 146,  42, 172,  86, 170, 198,
     79, 184,  56, 210, 150, 164, 125, 182, 118, 252, 107, 226, 156, 116,   4, 241,
     69, 157, 112,  89, 100, 113, 135,  32, 134,  91, 207, 101, 230,  45, 168,   2,
     27,  96,  37, 173, 174, 176, 185, 246,  28,  70,  97, 105,  52,  64, 126,  15,
     85,  71, 163,  35, 221,  81, 175,  58, 195,  92, 249, 206, 186, 197, 234,  38,
     44,  83,  13, 110, 133,  40, 132,   9, 211, 223, 205, 244, 65,  129,  77,  82,
    106, 220,  55, 200, 108, 193, 171, 250,  36, 225, 123,   8,  12, 189, 177,  74,
    120, 136, 149, 139, 227,  99, 232, 109, 233, 203, 213, 254,  59,   0,  29,  57,
    242, 239, 183,  14, 102,  88, 208, 228, 166, 119, 114, 248, 235, 117,  75,  10,
     49,  68,  80, 180, 143, 237,  31,  26, 219, 153, 141,  51, 159,  17, 131,  20
};

/*  padding table, from rfc */
static const char *_CryptMD2_aPadding[] =
{
    "",
    "\001",
    "\002\002",
    "\003\003\003",
    "\004\004\004\004",
    "\005\005\005\005\005",
    "\006\006\006\006\006\006",
    "\007\007\007\007\007\007\007",
    "\010\010\010\010\010\010\010\010",
    "\011\011\011\011\011\011\011\011\011",
    "\012\012\012\012\012\012\012\012\012\012",
    "\013\013\013\013\013\013\013\013\013\013\013",
    "\014\014\014\014\014\014\014\014\014\014\014\014",
    "\015\015\015\015\015\015\015\015\015\015\015\015\015",
    "\016\016\016\016\016\016\016\016\016\016\016\016\016\016",
    "\017\017\017\017\017\017\017\017\017\017\017\017\017\017\017",
    "\020\020\020\020\020\020\020\020\020\020\020\020\020\020\020\020"
};


// Public variables


/*** Private Functions *****************************************************************/

/*F*************************************************************************************/
/*!
    \Function _CryptMD2Transform

    \Description
        Execute MD2 transformation.

    \Input *pContext    - target MD2 context
    \Input *pInput      - input data

    \Output
        None.

    \Version 03/19/2009 (jbrookes)
*/
/*************************************************************************************F*/
static void _CryptMD2Transform(CryptMD2T *pContext, const uint8_t *pInput)
{
    uint8_t aCryptBlock[48];
    int32_t iBlock, iIter, iTemp;

    //  init crypto block
    ds_memcpy(&aCryptBlock[0 ], pContext->aState, 16);
    ds_memcpy(&aCryptBlock[16], pInput, 16);
    for (iBlock = 0; iBlock < 16; iBlock += 1)
    {
        aCryptBlock[iBlock+32] = pContext->aState[iBlock] ^ pInput[iBlock];
    }

    // encrypt block -- 18 iterations
    for (iIter = 0, iTemp = 0; iIter < 18; iIter += 1)
    {
        for (iBlock = 0; iBlock < 48; iBlock += 1)
        {
            aCryptBlock[iBlock] ^= _CryptMD2_aPiSubst[iTemp];
            iTemp = aCryptBlock[iBlock];
        }
        iTemp = (iTemp + iIter) & 0xff;
    }

    // update state
    ds_memcpy(pContext->aState, aCryptBlock, sizeof(pContext->aState));

    // update checksum
    for (iBlock = 0, iTemp = pContext->aChecksum[15]; iBlock < 16; iBlock += 1)
    {
        iTemp = pContext->aChecksum[iBlock] ^= _CryptMD2_aPiSubst[pInput[iBlock] ^ iTemp];
    }
    
    // clear crypto block
    memset(aCryptBlock, 0, sizeof(aCryptBlock));
}

/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function CryptMD2Init

    \Description
        Init the MD2 context.

    \Input *pContext    - target MD2 context

    \Output
        None.

    \Version 03/19/2009 (jbrookes)
*/
/*************************************************************************************F*/
void CryptMD2Init(CryptMD2T *pContext)
{
    memset(pContext, 0, sizeof(*pContext));
}

/*F*************************************************************************************/
/*!
    \Function CryptMD2Init2

    \Description
        Init the MD2 context, alternate version

    \Input *pContext    - target MD2 context
    \Input iHashSize    - hash size (ignored)

    \Output
        None.

    \Version 03/07/2014 (jbrookes)
*/
/*************************************************************************************F*/
void CryptMD2Init2(CryptMD2T *pContext, int32_t iHashSize)
{
    CryptMD2Init(pContext);
}

/*F*************************************************************************************/
/*!
    \Function CryptMD2Update

    \Description
        Add data to the MD2 context (hash the data).

    \Input *pContext    - target MD2 context
    \Input *_pBuffer    - input data to hash
    \Input iLength      - length of buffer (-1=treat pBuffer as asciiz)

    \Output
        None.

    \Version 03/19/2009 (jbrookes)
*/
/*************************************************************************************F*/
void CryptMD2Update(CryptMD2T *pContext, const void *_pBuffer, int32_t iLength)
{
    const uint8_t *pBuffer = _pBuffer;
    int32_t iCount, iIndex, iPartLen;
    
    // allow easy string access
    if (iLength < 0)
    {
        for (iLength = 0; pBuffer[iLength] != 0; ++iLength)
            ;
    }
    
    // update number of bytes mod 16
    iCount = pContext->iCount;
    pContext->iCount = (iCount + iLength) & 0xf;
    iPartLen = 16 - iCount;
    
    // transform as many times as possible
    if (iLength >= iPartLen)
    {
        ds_memcpy(&pContext->aBuffer[iCount], pBuffer, iPartLen);
        _CryptMD2Transform(pContext, pContext->aBuffer);
        for (iIndex = iPartLen; iIndex + 15 < iLength; iIndex += 16)
        {
            _CryptMD2Transform(pContext, pBuffer + iIndex);
        }
        iCount = 0;
    }
    else
    {
        iIndex = 0;
    }
    
    // buffer remaining input
    ds_memcpy(&pContext->aBuffer[iCount], &pBuffer[iIndex], iLength - iIndex);
}

/*F*************************************************************************************/
/*!
    \Function CryptMD2Final

    \Description
        Convert MD2 state into final output form

    \Input *pContext    - the MD2 state (from create)
    \Input *_pBuffer    - the digest output
    \Input iLength      - length of output buffer

    \Output
        None.

    \Version 03/19/2009 (jbrookes)
*/
/*************************************************************************************F*/
void CryptMD2Final(CryptMD2T *pContext, void *_pBuffer, int32_t iLength)
{
    uint8_t *pBuffer = _pBuffer;
    int32_t iPadLen;
    
    // make sure we have room
    if (iLength < MD2_BINARY_OUT)
    {
        NetPrintf(("cryptmd2: insufficient buffer for digest output\n"));
        return;
    }
    
    // pad out to a multiple of 16 bytes
    iPadLen = 16 - pContext->iCount;
    CryptMD2Update(pContext, _CryptMD2_aPadding[iPadLen], iPadLen);
    
    // extend with checksum
    CryptMD2Update(pContext, pContext->aChecksum, 16);
    
    // output state in digest
    ds_memcpy(pBuffer, pContext->aState, MD2_BINARY_OUT);
    
    // reset context
    memset(pContext, 0, sizeof(*pContext));
}
