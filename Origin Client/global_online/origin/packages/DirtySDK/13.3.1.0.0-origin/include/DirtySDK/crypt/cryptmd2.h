/*H*************************************************************************************************/
/*!
    \File cryptmd2.h

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
/*************************************************************************************************H*/

#ifndef _cryptmd3_h
#define _cryptmd3_h

/*!
\Module Crypt
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

#define MD2_BINARY_OUT 16       //!< length of binary output
#define MD2_STRING_OUT 33       //!< length of string output

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef struct CryptMD2T CryptMD2T;

//! all fields are PRIVATE
struct CryptMD2T
{
    int32_t iCount;         //!< total byte count
    uint8_t aState[16];     //!< md2 state
    uint8_t aChecksum[16];  //!< md2 checksum
    uint8_t aBuffer[16];    //!< input buffer
};

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// init the MD5 context.
void CryptMD2Init(CryptMD2T *pContext);

// add data to the MD2 context (hash the data).
void CryptMD2Update(CryptMD2T *pContext, const void *pBuffer, int32_t iLength);

// convert MD2 state into final output form
void CryptMD2Final(CryptMD2T *pContext, void *pBuffer, int32_t iLength);

#ifdef __cplusplus
}
#endif

//@}

#endif // _cryptmd2_h


