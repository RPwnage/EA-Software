/*H*************************************************************************************************/
/*!

    \File    protossl.h

    \Description
         This module is a from-scratch SSL2 implementation in order to avoid
         any intellectual property issues. The SSL2 algorithm was implemented
         from a specification provided by Netscape. Netscape has issued the
         statement included below regarding the use of this algorithm:

    \Notes
           Netscape Communications Corporation has been issued the following
           patent in the United States:

               Secure Socket Layer Application Program Apparatus And Method
               ("SSL"), No. 5,657,390

           Netscape Communications has issued the following statement:

               Intellectual Property Rights

               Secure Sockets Layer

               The United States Patent and Trademark Office ("the PTO")
               recently issued U.S. Patent No. 5,657,390 ("the SSL Patent")  to
               Netscape for inventions described as Secure Sockets Layers
               ("SSL"). The IETF is currently considering adopting SSL as a
               transport protocol with security features.  Netscape encourages
               the royalty-free adoption and use of the SSL protocol upon the
               following terms and conditions:

                 * If you already have a valid SSL Ref license today which
                   includes source code from Netscape, an additional patent
                   license under the SSL patent is not required.

                 * If you don't have an SSL Ref license, you may have a royalty
                   free license to build implementations covered by the SSL
                   Patent Claims or the IETF TLS specification provided that you
                   do not to assert any patent rights against Netscape or other
                   companies for the implementation of SSL or the IETF TLS
                   recommendation.

               What are "Patent Claims":

               Patent claims are claims in an issued foreign or domestic patent
               that:

                1) must be infringed in order to implement methods or build
                   products according to the IETF TLS specification;  or

                2) patent claims which require the elements of the SSL patent
                   claims and/or their equivalents to be infringed.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        03/08/2002 (GWS) First Version
    \Version    1.1        11/18/2002 (JLB) Names changed to include "Proto"

*/
/*************************************************************************************************H*/

#ifndef _protossl_h
#define _protossl_h

/*!
\Module Proto
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

// protossl failure codes (retrieve with ProtoSSLStat('fail')
#define PROTOSSL_ERROR_NONE         ( 0) //!< no error
#define PROTOSSL_ERROR_DNS          (-1) //!< DNS failure
#define PROTOSSL_ERROR_CONN         (-2) //!< connection error
#define PROTOSSL_ERROR_CERT_INVALID (-3) //!< certificate invalid
#define PROTOSSL_ERROR_CERT_HOST    (-4) //!< certificate not issued to this host
#define PROTOSSL_ERROR_CERT_NOTRUST (-5) //!< certificate is not trusted (recognized)
#define PROTOSSL_ERROR_SETUP        (-6) //!< failure in secure setup
#define PROTOSSL_ERROR_SECURE       (-7) //!< failure in secure connection after setup
#define PROTOSSL_ERROR_UNKNOWN      (-8) //!< unknown failure

// supported cipher suites
#define PROTOSSL_CIPHER_RSA_WITH_RC4_128_SHA        (1)
#define PROTOSSL_CIPHER_RSA_WITH_RC4_128_MD5        (2)
#define PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA    (4)
#define PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA    (8)
#define PROTOSSL_CIPHER_ALL (PROTOSSL_CIPHER_RSA_WITH_RC4_128_SHA|PROTOSSL_CIPHER_RSA_WITH_RC4_128_MD5|PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA|PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! identity fields for X509 issuer/subject
typedef struct ProtoSSLCertIdentT
{
    char strCountry[32];
    char strState[32];
    char strCity[32];
    char strOrg[32];
    char strUnit[256];
    char strCommon[64];
} ProtoSSLCertIdentT;

//! cert info returned by 
typedef struct ProtoSSLCertInfoT
{
    ProtoSSLCertIdentT Ident;
} ProtoSSLCertInfoT;

// opaque module state ref
typedef struct ProtoSSLRefT ProtoSSLRefT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate an SSL connection and prepare for use
ProtoSSLRefT *ProtoSSLCreate(void);

// reset connection back to base state.
void ProtoSSLReset(ProtoSSLRefT *pState);

// destroy the module and release its state
void ProtoSSLDestroy(ProtoSSLRefT *pState);

// give time to module to do its thing (should be called periodically to allow module to perform work)
void ProtoSSLUpdate(ProtoSSLRefT *pState);

// Accept an incoming connection.
ProtoSSLRefT* ProtoSSLAccept(ProtoSSLRefT *pState, int32_t iSecure,
                             struct sockaddr *pAddr, int32_t *pAddrlen);

// Create a socket bound to the given address.
int32_t ProtoSSLBind(ProtoSSLRefT *pState, const struct sockaddr *pAddr, int32_t pAddrlen);

// make a secure connection to a server.
int32_t ProtoSSLConnect(ProtoSSLRefT *pState, int32_t iSecure, const char *pAddr, uint32_t uAddr, int32_t iPort);

// disconnect from the server.
int32_t ProtoSSLDisconnect(ProtoSSLRefT *pState);

// Start listening for an incoming connection.
int32_t ProtoSSLListen(ProtoSSLRefT *pState, int32_t iBacklog);

// send secure data to the server.
int32_t ProtoSSLSend(ProtoSSLRefT *pState, const char *pBuffer, int32_t iLength);

// receive secure data from the server.
int32_t ProtoSSLRecv(ProtoSSLRefT *pState, char *pBuffer, int32_t iLength);

// return the current module status (according to selector)
int32_t ProtoSSLStat(ProtoSSLRefT *pState, int32_t iSelect, void *pBuffer, int32_t iLength);

// control module behavior
int32_t ProtoSSLControl(ProtoSSLRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// add an X.509 CA certificate that will be recognized in future transactions
int32_t ProtoSSLSetCACert(const uint8_t *pCACert, int32_t iCertSize);

// same as ProtoSSLSetCACert(), but certs are not validated at load time
int32_t ProtoSSLSetCACert2(const uint8_t *pCACert, int32_t iCertSize);

// validate all CAs that have not already been validated
int32_t ProtoSSLValidateAllCA(void);

// clear all CA certs
void ProtoSSLClrCACerts(void);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protossl_h

