/*H**************************************************************************/
/*!

    \File protossl.c

    \Description
        This module is an SSL3 implementation derived from the from-scratch
        SSL2 implementation currently in Dirtysock (see ProtoSSL2). It does
        not use any third-party code of any kind and was developed entirely
        by EA.

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

        For a description of the ASN.1 encoding rules see
          <http://www.itu.int32_t/ITU-T/studygroups/com17/languages/X.690-0207.pdf>

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002,2004,2005.  ALL RIGHTS RESERVED.

    \Version 1.0 03/08/2002 gschaefer   Initial SSL 2.0 implementation
    \Version 2.0 03/03/2004 sbevan      Added certificate validation
    \Version 3.0 11/05/2005 gschaefer   Rewritten to follow SSL 3.0 specification
*/
/***************************************************************************H*/

/*** Include files ***********************************************************/

#include <stdio.h>      // this module requires sprintf, strstr and some
#include <stdlib.h>     // other basic string functions
#include <string.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "dirtycert.h"
#include "protossl.h"
#include "cryptmd2.h"
#include "cryptmd5.h"
#include "cryptsha1.h"
#include "cryptarc4.h"
#include "cryptaes.h"
#include "cryptrsa.h"

#include "lobbybase64.h"

/*** Defines ***************************************************************************/

#define DEBUG_ANY_CERT  (DIRTYCODE_LOGGING && 0)  // allow any cert (use for testing only)
#define DEBUG_RAW_DATA  (DIRTYCODE_LOGGING && 0)  // display raw debug data
#define DEBUG_ENC_PERF  (DIRTYCODE_LOGGING && 0)  // show verbose crypto performance
#define DEBUG_MSG_LIST  (DIRTYCODE_LOGGING && 0)  // list the message states
#define DEBUG_ALL_OBJS  (DIRTYCODE_LOGGING && 0)  // display all headers/objects while parsing
#define DEBUG_VAL_CERT  (DIRTYCODE_LOGGING && 1)  // display verbose certificate validation info

#define SSL_MIN_PACKET      5                               // smallest packet (5 bytes framing)
#define SSL_CRYPTO_PAD      1024                            // extra space for crypto overhead
#define SSL_RAW_PACKET      16384                           // max raw data size
#define SSL_RCVMAX_PACKET   (SSL_RAW_PACKET+SSL_CRYPTO_PAD) // max recv packet buffer
#define SSL_SNDMAX_PACKET   (SSL_RAW_PACKET)                // max send packet buffer
#define SSL_SNDOVH_PACKET   (384)                           // reserve space for header/mac in send packet
#define SSL_SNDLIM_PACKET   (SSL_SNDMAX_PACKET-SSL_SNDOVH_PACKET) // max send user payload size

#define SSL3_VERSION        0x0300      // ssl version 3

#define SSL3_REC_CIPHER         20  // cipher change record
#define SSL3_REC_ALERT          21  // alert record
#define SSL3_REC_HANDSHAKE      22  // handshake record
#define SSL3_REC_APPLICATION    23  // application data record

#define SSL3_MSG_CLIENT_HELLO   1   // client hello
#define SSL3_MSG_SERVER_HELLO   2   // server hello
#define SSL3_MSG_CERTIFICATE    11  // server certificate
#define SSL3_MSG_SERVER_KEY     12  // server key data
#define SSL3_MSG_CERT_REQ       13  // server certificate request
#define SSL3_MSG_SERVER_DONE    14  // server handshake done
#define SSL3_MSG_CERT_VERIFY    15  // verify certificate
#define SSL3_MSG_CLIENT_KEY     16  // client key data
#define SSL3_MSG_FINISHED       20  // handshake finished

#define SSL3_MAC_NULL           0   // no record mac
#define SSL3_MAC_MD5            16  // md5 (ident is size in bytes)
#define SSL3_MAC_SHA            20  // sha (ident is size in bytes)

#define SSL3_KEY_NULL           0   // no record key exchange
#define SSL3_KEY_RSA            1   // use rsa

#define SSL3_KEYLEN_128         16  // 128-bit key length in bytes
#define SSL3_KEYLEN_256         32  // 256-bit key length in bytes

#define SSL3_ENC_NULL           0   // no record encryption
#define SSL3_ENC_RC4            1   // use rc4
#define SSL3_ENC_AES            2   // use aes

#define SSL3_ALERT_LEVEL_WARNING   1
#define SSL3_ALERT_LEVEL_FATAL     2

#define SSL3_ALERT_DESC_CLOSE_NOTIFY              0
#define SSL3_ALERT_DESC_UNEXPECTED_MESSAGE        10
#define SSL3_ALERT_DESC_BAD_RECORD_MAC            20
#define SSL3_ALERT_DESC_DECOMPRESSION_FAILURE     30
#define SSL3_ALERT_DESC_HANDSHAKE_FAILURE         40
#define SSL3_ALERT_DESC_NO_CERTIFICATE            41
#define SSL3_ALERT_DESC_BAD_CERTFICIATE           42
#define SSL3_ALERT_DESC_UNSUPPORTED_CERTIFICATE   43
#define SSL3_ALERT_DESC_CERTIFICATE_REVOKED       44
#define SSL3_ALERT_DESC_CERTIFICATE_EXPIRED       45
#define SSL3_ALERT_DESC_CERTIFICATE_UNKNOWN       46
#define SSL3_ALERT_DESC_ILLEGAL_PARAMETER         47

// certificate setup
#define SSL_CERT_X509   1

#define ST_IDLE                 0
#define ST_ADDR                 1
#define ST_CONN                 2
#define ST_WAIT_CONN            3
#define ST_WAIT_CA              4 // waiting for the CA to be fetched
#define ST3_SEND_HELLO          20
#define ST3_RECV_HELLO          21
#define ST3_SEND_KEY            22
#define ST3_SEND_CHANGE         23
#define ST3_SEND_FINISH         24
#define ST3_RECV_CHANGE         25
#define ST3_RECV_FINISH         26
#define ST3_SECURE              30
#define ST_UNSECURE             31
#define ST_FAIL                 0x1000
#define ST_FAIL_DNS             0x1001
#define ST_FAIL_CONN            0x1002
#define ST_FAIL_CERT_INVALID    0x1003
#define ST_FAIL_CERT_HOST       0x1004
#define ST_FAIL_CERT_NOTRUST    0x1005
#define ST_FAIL_SETUP           0x1006
#define ST_FAIL_SECURE          0x1007
#define ST_FAIL_CERT_REQUEST    0x1008

#define ASN_CLASS_UNIV      0x00
#define ASN_CLASS_APPL      0x40
#define ASN_CLASS_CONT      0x80
#define ASN_CLASS_PRIV      0xc0

#define ASN_PRIMITIVE       0x00
#define ASN_CONSTRUCT       0x20

#define ASN_TYPE_BOOLEAN    0x01
#define ASN_TYPE_INTEGER    0x02
#define ASN_TYPE_BITSTRING  0x03
#define ASN_TYPE_OCTSTRING  0x04
#define ASN_TYPE_NULL       0x05
#define ASN_TYPE_OBJECT     0x06
#define ASN_TYPE_UTF8STR    0x0c
#define ASN_TYPE_SEQN       0x10
#define ASN_TYPE_SET        0x11
#define ASN_TYPE_PRINTSTR   0x13
#define ASN_TYPE_T61        0x14
#define ASN_TYPE_IA5        0x16
#define ASN_TYPE_UTCTIME    0x17
#define ASN_TYPE_UNICODESTR 0x1e
#define ASN_TYPE_GENERALIZEDTIME 0x18

#define ASN_IMPLICIT_TAG   0x80
#define ASN_EXPLICIT_TAG   0xa0

enum {
    ASN_OBJ_NONE = 0,
    ASN_OBJ_COUNTRY,
    ASN_OBJ_STATE,
    ASN_OBJ_CITY,
    ASN_OBJ_ORGANIZATION,
    ASN_OBJ_UNIT,
    ASN_OBJ_COMMON,
    ASN_OBJ_SUBJECT_ALT,
    ASN_OBJ_BASIC_CONSTRAINTS,
    ASN_OBJ_RSA_PKCS_KEY,
    ASN_OBJ_RSA_PKCS_MD2,
    ASN_OBJ_RSA_PKCS_MD5,
    ASN_OBJ_RSA_PKCS_SHA1
};

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/



//! note: this layout differs somewhat from x.509, but I think it
//! makes more sense this way
typedef struct X509CertificateT
{
    ProtoSSLCertIdentT Issuer;      //!< certificate was issued by (matches subject in another cert)
    ProtoSSLCertIdentT Subject;     //!< certificate was issued for this site/authority

    char strGoodFrom[32];           //!< good from this date
    char strGoodTill[32];           //!< until this date

    char strSubjectAlt[512];        //!< subject alternative name(s)

    int32_t iSerialSize;            //!< certificate serial number
    uint8_t SerialData[32];         //!< certificate serial data

    int32_t iSigType;               //!< digital signature type
    int32_t iSigSize;               //!< size of signature data
    uint8_t SigData[512];           //!< digital signature data

    int32_t iKeyType;               //!< public key algorithm type
    int32_t iKeyModSize;            //!< size of public key modulus
    uint8_t KeyModData[512];        //!< public key modulus
    int32_t iKeyExpSize;            //!< size of public key exponent
    uint8_t KeyExpData[16];         //!< public key exponent

    // iMaxHeight is valid only if iCertIsCA is set
    int32_t iCertIsCA;              //!< whether this cert can be used as a CA cert
    int32_t iMaxHeight;             //!< the pathLenConstraints of a CA, 0 means no limit/field not present

    int32_t iHashSize;              //!< size of certificate hash
    uint8_t HashData[CRYPTSHA1_HASHSIZE];
} X509CertificateT;

//! minimal certificate authority data (just enough to validate another certificate)
typedef struct ProtoSSLCACertT
{
    ProtoSSLCertIdentT Subject;     //!< identity of this certificate authority

    int32_t iKeyModSize;            //!< size of public key modulus
    const uint8_t *pKeyModData;     //!< public key modulus for signature verification

    int32_t iKeyExpSize;            //!< size of public key exponent
    uint8_t KeyExpData[16];         //!< public key exponent

    int32_t iMemGroup;              //!< memgroup cert was allocated with (zero == static)
    void   *pMemGroupUserData;      //!< memgroup user data

    X509CertificateT *pX509Cert;    //!< X509 certificate, if this cert has not yet been validated
    struct ProtoSSLCACertT *pNext;  //!< link to next cert in list
} ProtoSSLCACertT;

//! cipher parameter lookup structure
typedef struct CipherSuiteT
{
    uint8_t uIdent[2];              //!< two-byte identifier
    uint8_t uKey;                   //!< key exchange algorithm
    uint8_t uLen;                   //!< key length
    uint8_t uEnc;                   //!< encryption algorithm
    uint8_t uMac;                   //!< MAC digest algorithm
    uint8_t uId;                    //!< PROTOSSL_CIPHER_*
} CipherSuiteT;

//! extra state information required for secure connections
//! (not allocated for unsecure connections)
typedef struct SecureStateT
{
    uint32_t uTimer;                //!< base of setup timing

    uint32_t uSendSeqn;             //!< send sequence number
    int32_t iSendProg;              //!< progress within send
    int32_t iSendSize;              //!< total bytes to send

    uint32_t uRecvSeqn;             //!< receive sequence number
    int32_t iRecvProg;              //!< progress within receive
    int32_t iRecvSize;              //!< total bytes to receive
    int32_t iRecvBase;              //!< start of receive data
    int32_t iRecvDone;              //!< a packet has been fully received and processed

    const CipherSuiteT *pCipher;    //!< selected cipher suite

    uint8_t ClientRandom[32];       //!< clients random seed
    uint8_t ServerRandom[32];       //!< servers random seed

    uint8_t PreMasterKey[48];       //!< pre-master-key
    uint8_t MasterKey[48];          //!< master key

    uint8_t KeyBlock[192];          //!< key block
    uint8_t *pServerMAC;            //!< server mac secret
    uint8_t *pClientMAC;            //!< client mac secret
    uint8_t *pServerKey;            //!< server key secret
    uint8_t *pClientKey;            //!< client key secret
    uint8_t *pServerInitVec;        //!< init vector (CBC ciphers)
    uint8_t *pClientInitVec;        //!< init vector (CBC ciphers)

    CryptMD5T HandshakeMD5;         //!< MD5 of all handshake data
    CryptSha1T HandshakeSHA;        //!< SHA of all handshake data

    CryptArc4T ReadArc4;            //!< arc4 read cipher state
    CryptArc4T WriteArc4;           //!< arc4 write cipher state

    CryptAesT ReadAes;              //!< aes read cipher state
    CryptAesT WriteAes;             //!< aes write cipher state

    X509CertificateT Cert;          //!< the x509 certificate

    uint8_t SendData[SSL_SNDMAX_PACKET];   //! <put at end to make references efficient
    uint8_t RecvData[SSL_RCVMAX_PACKET];   //! <put at end to make references efficient
} SecureStateT;

//! module state
struct ProtoSSLRefT
{
    SocketT *pSock;                 //!< comm socket
    HostentT *pHost;                //!< host entry

    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    char strHost[256];              //!< host that we connect to.
    struct sockaddr PeerAddr;       //!< peer info

    int32_t iState;                 //!< protocol state
    int32_t iClosed;                //!< socket closed flag
    SecureStateT *pSecure;          //!< secure state reference
    X509CertificateT *pCertToVal;   //!< server cert to be validated (used in ST_WAIT_CA state)
    ProtoSSLCertInfoT CertInfo;     //!< certificate info (used on failure)

    uint32_t uEnabledCiphers;       //!< enabled ciphers
    int32_t iRecvBufSize;           //!< TCP recv buffer size; 0=default
    int32_t iSendBufSize;           //!< TCP send buffer size; 0=default
    int32_t iLastSocketError;       //!< Last socket error before closing the socket
    int32_t iCARequestId;           //!< CA request id (valid if bCARequested is TRUE)
    uint8_t bAllowAnyCert;          //!< bypass certificate validation
    uint8_t bAllowXenonLookup;      //!< allow xenon secure lookup
    uint8_t bCertInfoSet;           //!< TRUE if cert info has been set, else FALSE
    uint8_t bCARequested;           //!< TRUE if we have made a CA request
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// random data for master key generation
static uint32_t _ProtoSSL_Random[4] = { 0, 0, 0, 0 };

// SSL3 pad1 sequence
const static uint8_t _SSL3_Pad1[48] =
{
    0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
    0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
    0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36
};

// SSL3 pad2 sequence
const static uint8_t _SSL3_Pad2[48] =
{
    0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
    0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
    0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c
};

// SSL3 cipher parameters
const static CipherSuiteT _SSL3_CipherSuite[] =
{
    { { 0, 0x05 }, SSL3_KEY_RSA, SSL3_KEYLEN_128 , SSL3_ENC_RC4, SSL3_MAC_SHA, PROTOSSL_CIPHER_RSA_WITH_RC4_128_SHA },      // suite  5: rsa+rc4+sha
    { { 0, 0x04 }, SSL3_KEY_RSA, SSL3_KEYLEN_128 , SSL3_ENC_RC4, SSL3_MAC_MD5, PROTOSSL_CIPHER_RSA_WITH_RC4_128_MD5 },      // suite  4: rsa+rc4+md5
    { { 0, 0x2f }, SSL3_KEY_RSA, SSL3_KEYLEN_128 , SSL3_ENC_AES, SSL3_MAC_SHA, PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA },  // suite 47: rsa+aes+sha
    { { 0, 0x35 }, SSL3_KEY_RSA, SSL3_KEYLEN_256 , SSL3_ENC_AES, SSL3_MAC_SHA, PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA },  // suite 53: rsa+aes+sha
};

#if DIRTYCODE_LOGGING
const static char *_SSL3_strCipherSuite[] =
{
    "TLS_RSA_WITH_RC4_128_SHA",
    "TLS_RSA_WITH_RC4_128_MD5",
    "TLS_RSA_WITH_AES_128_CBC_SHA",
    "TLS_RSA_WITH_AES_256_CBC_SHA",
};
#endif

// ASN object identification table
const static struct
{
    int32_t iType;              //!< symbolic type
    int32_t iSize;              //!< size of signature
    uint8_t strData[16];        //!< signature data
}
_SSL_ObjectList[] =
{
    { ASN_OBJ_COUNTRY, 3, { 0x55, 0x04, 0x06 } },
    { ASN_OBJ_CITY, 3, { 0x55, 0x04, 0x07 } },
    { ASN_OBJ_STATE, 3, { 0x55, 0x04, 0x08 } },
    { ASN_OBJ_ORGANIZATION, 3, { 0x55, 0x04, 0x0a } },
    { ASN_OBJ_UNIT, 3, { 0x55, 0x04, 0x0b } },
    { ASN_OBJ_COMMON, 3, { 0x55, 0x04, 0x03 } },
    { ASN_OBJ_SUBJECT_ALT, 3, { 0x55, 0x1d, 0x11 } },
    { ASN_OBJ_BASIC_CONSTRAINTS, 3, { 0x55, 0x1d, 0x13 } },
    // RSA (PKCS #1 v1.5) key transport algorithm, OID 1.2.840.113349.1.1.1
    { ASN_OBJ_RSA_PKCS_KEY, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 } },
    // RSA (PKCS #1 v1.5) with MD2 signature, OID 1.2.840.113349.1.1.2
    { ASN_OBJ_RSA_PKCS_MD2, 9, { 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x02 } },
    // RSA (PKCS #1 v1.5) with MD5 signature, OID 1.2.840.113349.1.1.4
    { ASN_OBJ_RSA_PKCS_MD5, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04 } },
    // RSA (PKCS #1 v1.5) with SHA-1 signature, OID 1.2.840.113349.1.1.5
    { ASN_OBJ_RSA_PKCS_SHA1, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05 } },
    { ASN_OBJ_NONE, 0, { 0 } }
};

// The 1024-bit public key modulus for (old) GOS CA Cert with an exponent of 3
const static uint8_t _ProtoSSL_GOSServerModulus1024[] =
{
    0x92, 0x75, 0xa1, 0x5b, 0x08, 0x02, 0x40, 0xb8, 0x9b, 0x40, 0x2f, 0xd5, 0x9c, 0x71, 0xc4, 0x51,
    0x58, 0x71, 0xd8, 0xf0, 0x2d, 0x93, 0x7f, 0xd3, 0x0c, 0x8b, 0x1c, 0x7d, 0xf9, 0x2a, 0x04, 0x86,
    0xf1, 0x90, 0xd1, 0x31, 0x0a, 0xcb, 0xd8, 0xd4, 0x14, 0x12, 0x90, 0x3b, 0x35, 0x6a, 0x06, 0x51,
    0x49, 0x4c, 0xc5, 0x75, 0xee, 0x0a, 0x46, 0x29, 0x80, 0xf0, 0xd5, 0x3a, 0x51, 0xba, 0x5d, 0x6a,
    0x19, 0x37, 0x33, 0x43, 0x68, 0x25, 0x2d, 0xfe, 0xdf, 0x95, 0x26, 0x36, 0x7c, 0x43, 0x64, 0xf1,
    0x56, 0x17, 0x0e, 0xf1, 0x67, 0xd5, 0x69, 0x54, 0x20, 0xfb, 0x3a, 0x55, 0x93, 0x5d, 0xd4, 0x97,
    0xbc, 0x3a, 0xd5, 0x8f, 0xd2, 0x44, 0xc5, 0x9a, 0xff, 0xcd, 0x0c, 0x31, 0xdb, 0x9d, 0x94, 0x7c,
    0xa6, 0x66, 0x66, 0xfb, 0x4b, 0xa7, 0x5e, 0xf8, 0x64, 0x4e, 0x28, 0xb1, 0xa6, 0xb8, 0x73, 0x95
};

// The 2048-bit public key modulus for (new) GOS CA Cert with an exponent of 17
const static uint8_t _ProtoSSL_GOSServerModulus2048[] =
{
    0xc2, 0x0d, 0x06, 0xa5, 0xd7, 0x11, 0xac, 0xe9, 0xbb, 0x31, 0x05, 0x2c, 0x82, 0x50, 0xdc, 0x6f,
    0xb9, 0x78, 0x81, 0x93, 0x1e, 0x1d, 0x70, 0xa0, 0x73, 0x16, 0xd5, 0xfb, 0x89, 0x9a, 0xcf, 0x67,
    0x00, 0x03, 0x64, 0xc6, 0x14, 0xdd, 0x17, 0x1f, 0x2e, 0x8e, 0x33, 0xb3, 0x5c, 0x3c, 0x3b, 0x62,
    0x20, 0x2e, 0xb3, 0x6a, 0xcb, 0xd8, 0x72, 0xc8, 0x55, 0xe6, 0xb2, 0xc6, 0x65, 0xa3, 0x03, 0x35,
    0xf9, 0x16, 0x3c, 0x4b, 0x78, 0x74, 0xad, 0x3a, 0x08, 0x52, 0x69, 0xf8, 0xa5, 0xf3, 0x89, 0x78,
    0x38, 0x16, 0x62, 0xc3, 0xed, 0xe2, 0x01, 0x32, 0xbc, 0x6e, 0xeb, 0xd9, 0xb8, 0xfc, 0x3f, 0xc7,
    0xee, 0xf2, 0xe8, 0xec, 0x1b, 0x2b, 0xa8, 0xb8, 0xb1, 0x3f, 0xa1, 0x18, 0x71, 0x76, 0x17, 0xae,
    0x61, 0xa4, 0x3e, 0xea, 0xe8, 0x73, 0x19, 0x98, 0x9f, 0x7e, 0xc8, 0x6e, 0x3a, 0x86, 0x09, 0x75,
    0x2e, 0x7e, 0xb5, 0xa3, 0x6e, 0x7a, 0x14, 0x8b, 0x7b, 0x81, 0xd0, 0x22, 0x5c, 0x6d, 0x01, 0xef,
    0x8d, 0x88, 0x6c, 0x89, 0x61, 0x12, 0x5c, 0x68, 0x32, 0x67, 0xc2, 0xe6, 0x8b, 0xad, 0xf3, 0xef,
    0x02, 0xfc, 0x86, 0xde, 0xda, 0x47, 0xbd, 0xf8, 0xb8, 0x55, 0xbb, 0x75, 0xe6, 0x40, 0x34, 0x8b,
    0x8e, 0xc6, 0x15, 0x9c, 0x59, 0xaf, 0x73, 0xc0, 0x83, 0xce, 0x45, 0x83, 0xbb, 0xe0, 0xeb, 0x2c,
    0x59, 0x9b, 0x79, 0xdc, 0x56, 0x36, 0x74, 0x28, 0x82, 0x8b, 0xe3, 0xc3, 0x85, 0x29, 0x10, 0xd6,
    0xd0, 0x65, 0xe5, 0x62, 0x93, 0x49, 0x22, 0xbb, 0x0c, 0x4b, 0x3c, 0x75, 0x8c, 0x97, 0xdf, 0x87,
    0x0a, 0x13, 0xca, 0xe9, 0x4b, 0x5b, 0x11, 0xe5, 0x29, 0x29, 0x73, 0xd8, 0x11, 0xe1, 0x91, 0x7c,
    0x3b, 0x2d, 0xd7, 0xdc, 0x8d, 0xfd, 0xcd, 0x87, 0x0e, 0x68, 0x62, 0x77, 0x6d, 0xaa, 0xd9, 0x01
};

// The 2048-bit modulus for the embedded VeriSign CA Cert
const static uint8_t _ProtoSSL_VeriSignServerModulus[] =
{
    0x95, 0xc3, 0x21, 0x12, 0x8e, 0x40, 0xc5, 0x0d, 0x01, 0x5f, 0x76, 0x5e, 0x66, 0x94, 0xd9, 0x73,
    0x2c, 0x58, 0x19, 0x22, 0xb8, 0xc9, 0xfc, 0x7a, 0x39, 0x90, 0x2a, 0x77, 0x72, 0x7c, 0x1d, 0x3e,
    0xf7, 0xd8, 0x55, 0xe3, 0xaf, 0x42, 0xcb, 0x87, 0x30, 0x02, 0xdc, 0x5b, 0xac, 0x70, 0xe6, 0xb8,
    0x44, 0xb4, 0x2b, 0x35, 0xeb, 0x93, 0xd2, 0x17, 0x05, 0x7e, 0xcb, 0x46, 0xd6, 0x5c, 0x53, 0xa0,
    0x32, 0x51, 0x9d, 0x74, 0x64, 0x58, 0xf9, 0x0c, 0x9a, 0x00, 0xea, 0x5e, 0x44, 0x49, 0x64, 0x72,
    0xf4, 0xcd, 0x10, 0xe2, 0x85, 0x0a, 0xf9, 0x34, 0xee, 0xb3, 0x88, 0x66, 0xa9, 0xa5, 0xa4, 0x5a,
    0xd0, 0x0e, 0x98, 0x7f, 0x58, 0x0d, 0x2b, 0x52, 0xbb, 0x86, 0xa9, 0x7e, 0x2e, 0xfa, 0xb2, 0x48,
    0x7c, 0x8d, 0xdb, 0x2d, 0x5f, 0x01, 0x75, 0xa2, 0x8d, 0x06, 0x3b, 0x8b, 0xb4, 0x61, 0x07, 0xc9,
    0xbe, 0x22, 0x99, 0xf8, 0x1b, 0xd1, 0xb5, 0x57, 0x66, 0x04, 0x4d, 0x35, 0xf4, 0x91, 0x71, 0x96,
    0xb5, 0x99, 0x08, 0x25, 0x9b, 0x97, 0xc8, 0x3a, 0xf3, 0x20, 0xb1, 0xdd, 0x9e, 0x98, 0x0c, 0x4a,
    0x63, 0xb7, 0xa6, 0xce, 0xb0, 0x01, 0xce, 0xf8, 0x93, 0x6a, 0xf3, 0x0c, 0x6e, 0x9f, 0xb1, 0xe9,
    0x84, 0x7b, 0x81, 0x98, 0x41, 0xe6, 0x81, 0xdc, 0x3d, 0x2c, 0xe7, 0xb4, 0x6b, 0xe3, 0x9e, 0xfc,
    0x08, 0x16, 0xd7, 0xb3, 0xd5, 0xb9, 0x66, 0x12, 0x99, 0x7c, 0x6d, 0x71, 0xc8, 0x4d, 0xbe, 0xc7,
    0x0f, 0xe3, 0xfb, 0x37, 0xad, 0xd5, 0x75, 0x87, 0x21, 0x6b, 0x86, 0xd0, 0x44, 0x14, 0x5a, 0x54,
    0x79, 0x39, 0x96, 0x69, 0x56, 0xc9, 0xb9, 0x31, 0xcd, 0x89, 0x61, 0x58, 0xe1, 0xd9, 0x76, 0x05,
    0x05, 0xad, 0xf7, 0xb9, 0x02, 0xaf, 0xa7, 0xfd, 0x47, 0x91, 0xa2, 0x22, 0x34, 0x5a, 0x31, 0xd1
};

// The 2048-bit public key modulus for the EA GeoTrust CA, used for latest EA certs
const static uint8_t _ProtoSSL_GeoTrustServerModulus[] =
{
    0x90, 0xb3, 0x80, 0xc1, 0xe4, 0xe5, 0x46, 0xad, 0x70, 0x60, 0x3d, 0xba, 0xe5, 0x14, 0xdd, 0x9e,
    0x8a, 0x5e, 0x8b, 0x75, 0x5a, 0xe6, 0xca, 0x6d, 0x41, 0xa5, 0x23, 0xe8, 0x39, 0x85, 0x26, 0x7a,
    0xa7, 0x55, 0x77, 0x9a, 0x48, 0xa1, 0x92, 0x7e, 0x3a, 0x1e, 0x1a, 0xf1, 0x27, 0xab, 0xa3, 0x4c,
    0x39, 0xcc, 0xcb, 0x3d, 0x47, 0xaf, 0x81, 0xae, 0x16, 0x6a, 0x5c, 0x37, 0xef, 0x45, 0x41, 0xfd,
    0xfb, 0x9a, 0x97, 0x3c, 0xa0, 0x43, 0x9d, 0xc6, 0xdf, 0x17, 0x21, 0xd1, 0x8a, 0xa2, 0x56, 0xc2,
    0x03, 0x49, 0x84, 0x12, 0x81, 0x3e, 0xc9, 0x0a, 0x54, 0x60, 0x66, 0xb9, 0x8c, 0x54, 0xe4, 0xf9,
    0xe6, 0xf9, 0x94, 0xf1, 0xe0, 0x5f, 0x75, 0x11, 0xf2, 0x29, 0xb9, 0xe4, 0x86, 0xa2, 0xb1, 0x89,
    0xad, 0xa6, 0x1e, 0x83, 0x29, 0x63, 0xb2, 0xf0, 0x54, 0x1c, 0x85, 0x0b, 0x7a, 0xe7, 0xe1, 0x2e,
    0x0d, 0xaf, 0xa4, 0xbd, 0xcd, 0xe7, 0xb1, 0x5a, 0xd7, 0x8c, 0x05, 0x5a, 0x0e, 0x4b, 0x73, 0x28,
    0x8b, 0x75, 0x5d, 0x34, 0xd8, 0x77, 0x0b, 0xe1, 0x74, 0x62, 0xe2, 0x71, 0x30, 0x62, 0xd8, 0xbc,
    0x8a, 0x05, 0xe5, 0x31, 0x63, 0x4a, 0x54, 0x89, 0x6a, 0x33, 0x78, 0xa7, 0x4e, 0x55, 0x24, 0x1d,
    0x97, 0xef, 0x1a, 0xe4, 0x12, 0xc6, 0x0f, 0x30, 0x18, 0xb4, 0x34, 0x4d, 0xe1, 0xd8, 0x23, 0x3b,
    0x21, 0x5b, 0x2d, 0x30, 0x19, 0x25, 0x0e, 0x74, 0xf7, 0xa4, 0x21, 0x4b, 0xa0, 0xa4, 0x20, 0xc9,
    0x6c, 0xcd, 0x98, 0x56, 0xc0, 0xf2, 0xa8, 0x5f, 0x3e, 0x26, 0x75, 0xa0, 0x0d, 0xf8, 0x36, 0x88,
    0x8a, 0x2c, 0x5a, 0x7d, 0x67, 0x30, 0xa9, 0x0f, 0xd1, 0x99, 0x70, 0x2e, 0x78, 0xe1, 0x51, 0x26,
    0xaf, 0x55, 0x7a, 0x24, 0xbe, 0x8c, 0x39, 0x0d, 0x77, 0x9d, 0xde, 0x02, 0xc3, 0x0c, 0xbd, 0x1f
};

// The 2048-bit modulus for the GeoTrust SSL intermediate cert
// Used by legacy Ebisu-related services
const static uint8_t _ProtoSSL_GeoTrustSSLModulus[] =
{
    0x90, 0xb3, 0x80, 0xc1, 0xe4, 0xe5, 0x46, 0xad,
    0x70, 0x60, 0x3d, 0xba, 0xe5, 0x14, 0xdd, 0x9e,
    0x8a, 0x5e, 0x8b, 0x75, 0x5a, 0xe6, 0xca, 0x6d,
    0x41, 0xa5, 0x23, 0xe8, 0x39, 0x85, 0x26, 0x7a,
    0xa7, 0x55, 0x77, 0x9a, 0x48, 0xa1, 0x92, 0x7e,
    0x3a, 0x1e, 0x1a, 0xf1, 0x27, 0xab, 0xa3, 0x4c,
    0x39, 0xcc, 0xcb, 0x3d, 0x47, 0xaf, 0x81, 0xae,
    0x16, 0x6a, 0x5c, 0x37, 0xef, 0x45, 0x41, 0xfd,
    0xfb, 0x9a, 0x97, 0x3c, 0xa0, 0x43, 0x9d, 0xc6,
    0xdf, 0x17, 0x21, 0xd1, 0x8a, 0xa2, 0x56, 0xc2,
    0x03, 0x49, 0x84, 0x12, 0x81, 0x3e, 0xc9, 0x0a,
    0x54, 0x60, 0x66, 0xb9, 0x8c, 0x54, 0xe4, 0xf9,
    0xe6, 0xf9, 0x94, 0xf1, 0xe0, 0x5f, 0x75, 0x11,
    0xf2, 0x29, 0xb9, 0xe4, 0x86, 0xa2, 0xb1, 0x89,
    0xad, 0xa6, 0x1e, 0x83, 0x29, 0x63, 0xb2, 0xf0,
    0x54, 0x1c, 0x85, 0x0b, 0x7a, 0xe7, 0xe1, 0x2e,
    0x0d, 0xaf, 0xa4, 0xbd, 0xcd, 0xe7, 0xb1, 0x5a,
    0xd7, 0x8c, 0x05, 0x5a, 0x0e, 0x4b, 0x73, 0x28,
    0x8b, 0x75, 0x5d, 0x34, 0xd8, 0x77, 0x0b, 0xe1,
    0x74, 0x62, 0xe2, 0x71, 0x30, 0x62, 0xd8, 0xbc,
    0x8a, 0x05, 0xe5, 0x31, 0x63, 0x4a, 0x54, 0x89,
    0x6a, 0x33, 0x78, 0xa7, 0x4e, 0x55, 0x24, 0x1d,
    0x97, 0xef, 0x1a, 0xe4, 0x12, 0xc6, 0x0f, 0x30,
    0x18, 0xb4, 0x34, 0x4d, 0xe1, 0xd8, 0x23, 0x3b,
    0x21, 0x5b, 0x2d, 0x30, 0x19, 0x25, 0x0e, 0x74,
    0xf7, 0xa4, 0x21, 0x4b, 0xa0, 0xa4, 0x20, 0xc9,
    0x6c, 0xcd, 0x98, 0x56, 0xc0, 0xf2, 0xa8, 0x5f,
    0x3e, 0x26, 0x75, 0xa0, 0x0d, 0xf8, 0x36, 0x88,
    0x8a, 0x2c, 0x5a, 0x7d, 0x67, 0x30, 0xa9, 0x0f,
    0xd1, 0x99, 0x70, 0x2e, 0x78, 0xe1, 0x51, 0x26,
    0xaf, 0x55, 0x7a, 0x24, 0xbe, 0x8c, 0x39, 0x0d,
    0x77, 0x9d, 0xde, 0x02, 0xc3, 0x0c, 0xbd, 0x1f,
};

// The 2048-bit modulus for the VeriSign Secure Server G3 intermediate cert
// Used by Origin branded Ebisu services
const static uint8_t _ProtoSSL_VeriSignSecureServerG3Modulus[] =
{
    0xb1, 0x87, 0x84, 0x1f, 0xc2, 0x0c, 0x45, 0xf5,
    0xbc, 0xab, 0x25, 0x97, 0xa7, 0xad, 0xa2, 0x3e,
    0x9c, 0xba, 0xf6, 0xc1, 0x39, 0xb8, 0x8b, 0xca,
    0xc2, 0xac, 0x56, 0xc6, 0xe5, 0xbb, 0x65, 0x8e,
    0x44, 0x4f, 0x4d, 0xce, 0x6f, 0xed, 0x09, 0x4a,
    0xd4, 0xaf, 0x4e, 0x10, 0x9c, 0x68, 0x8b, 0x2e,
    0x95, 0x7b, 0x89, 0x9b, 0x13, 0xca, 0xe2, 0x34,
    0x34, 0xc1, 0xf3, 0x5b, 0xf3, 0x49, 0x7b, 0x62,
    0x83, 0x48, 0x81, 0x74, 0xd1, 0x88, 0x78, 0x6c,
    0x02, 0x53, 0xf9, 0xbc, 0x7f, 0x43, 0x26, 0x57,
    0x58, 0x33, 0x83, 0x3b, 0x33, 0x0a, 0x17, 0xb0,
    0xd0, 0x4e, 0x91, 0x24, 0xad, 0x86, 0x7d, 0x64,
    0x12, 0xdc, 0x74, 0x4a, 0x34, 0xa1, 0x1d, 0x0a,
    0xea, 0x96, 0x1d, 0x0b, 0x15, 0xfc, 0xa3, 0x4b,
    0x3b, 0xce, 0x63, 0x88, 0xd0, 0xf8, 0x2d, 0x0c,
    0x94, 0x86, 0x10, 0xca, 0xb6, 0x9a, 0x3d, 0xca,
    0xeb, 0x37, 0x9c, 0x00, 0x48, 0x35, 0x86, 0x29,
    0x50, 0x78, 0xe8, 0x45, 0x63, 0xcd, 0x19, 0x41,
    0x4f, 0xf5, 0x95, 0xec, 0x7b, 0x98, 0xd4, 0xc4,
    0x71, 0xb3, 0x50, 0xbe, 0x28, 0xb3, 0x8f, 0xa0,
    0xb9, 0x53, 0x9c, 0xf5, 0xca, 0x2c, 0x23, 0xa9,
    0xfd, 0x14, 0x06, 0xe8, 0x18, 0xb4, 0x9a, 0xe8,
    0x3c, 0x6e, 0x81, 0xfd, 0xe4, 0xcd, 0x35, 0x36,
    0xb3, 0x51, 0xd3, 0x69, 0xec, 0x12, 0xba, 0x56,
    0x6e, 0x6f, 0x9b, 0x57, 0xc5, 0x8b, 0x14, 0xe7,
    0x0e, 0xc7, 0x9c, 0xed, 0x4a, 0x54, 0x6a, 0xc9,
    0x4d, 0xc5, 0xbf, 0x11, 0xb1, 0xae, 0x1c, 0x67,
    0x81, 0xcb, 0x44, 0x55, 0x33, 0x99, 0x7f, 0x24,
    0x9b, 0x3f, 0x53, 0x45, 0x7f, 0x86, 0x1a, 0xf3,
    0x3c, 0xfa, 0x6d, 0x7f, 0x81, 0xf5, 0xb8, 0x4a,
    0xd3, 0xf5, 0x85, 0x37, 0x1c, 0xb5, 0xa6, 0xd0,
    0x09, 0xe4, 0x18, 0x7b, 0x38, 0x4e, 0xfa, 0x0f,
};

// only certificates from these authorities are supported
static ProtoSSLCACertT _ProtoSSL_CACerts[] =
{
    { { "US", "California", "Redwood City", "Electronic Arts, Inc.",
        "Online Technology Group", "OTG3 Certificate Authority" },
      128, _ProtoSSL_GOSServerModulus1024,
      1, { 0x03 },
      0, NULL, NULL, &_ProtoSSL_CACerts[1] },
    { { "US", "California", "Redwood City", "Electronic Arts, Inc.",
        "Global Online Studio/emailAddress=GOSDirtysockSupport@ea.com",
        "GOS 2011 Certificate Authority" },
      256, _ProtoSSL_GOSServerModulus2048,
      1, { 0x11 },
      0, NULL, NULL, &_ProtoSSL_CACerts[2] },
    { { "US", "", "", "VeriSign, Inc.",
        "VeriSign Trust Network, Terms of use at https://www.verisign.com/rpa (c)05",
        "VeriSign Class 3 Secure Server CA" },
      256, _ProtoSSL_VeriSignServerModulus,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, &_ProtoSSL_CACerts[3]},
    { { "US", "", "", "GeoTrust, Inc.", "", "GeoTrust SSL CA" },
      256, _ProtoSSL_GeoTrustServerModulus,
      3, { 0x01, 0x00, 0x01 },
        0, NULL, NULL, &_ProtoSSL_CACerts[5]},
    { { "US", "", "", "GeoTrust, Inc.",
        "", "GeoTrust SSL CA"},
        256, _ProtoSSL_GeoTrustSSLModulus,
        3, { 0x01, 0x00, 0x01 },
        0, NULL, NULL, &_ProtoSSL_CACerts[6]},
    { { "US", "", "", "VeriSign, Inc.",
        "VeriSign Trust Network, Terms of use at https://www.verisign.com/rpa (c)10",
        "VeriSign Class 3 Secure Server CA - G3" },
        256, _ProtoSSL_VeriSignSecureServerG3Modulus,
        3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, NULL},
};

/*** Private functions ******************************************************/


#if DIRTYCODE_LOGGING
/*F**************************************************************************/
/*!
    \Function _DebugAlert

    \Description
        Debug-only display of info following server alert

    \Input *pState      - module state
    \Input uAlertLevel  - alert level
    \Input uAlertType   - alert type

    \Version 04/04/2009 (jbrookes)
*/
/**************************************************************************F*/
static void _DebugAlert(ProtoSSLRefT *pState, uint8_t uAlertLevel, uint8_t uAlertType)
{
    SecureStateT *pSecure = pState->pSecure;
    NetPrintf(("protossl: ALERT: level=%d, type=%d\n", uAlertLevel, uAlertType));
    NetPrintMem(_SSL3_Pad1, sizeof(_SSL3_Pad1), "_SSL3_Pad1");
    NetPrintMem(_SSL3_Pad2, sizeof(_SSL3_Pad2), "_SSL3_Pad2");
    if (pSecure != NULL)
    {
        NetPrintf(("protossl: pSecure->uSendSeqn=%d\n", pSecure->uSendSeqn));
        if (pSecure->pCipher != NULL)
        {
            NetPrintf(("protossl: pSecure->pCipher->uMac=%d\n", pSecure->pCipher->uMac));
            NetPrintf(("protossl: pSecure->pCipher->uEnc=%d\n", pSecure->pCipher->uEnc));
        }
    }
}
#else
#define _DebugAlert(_pState, _uAlertLevel, _uAlertType)
#endif

/*F**************************************************************************/
/*!
    \Function _WildcardMatchNoCase

    \Description
        Perform wildcard case-insensitive string comparison of given input
        strings.  This implementation assumes the first string does not
        include a wildcard character and the second string includes exactly
        zero or one wildcard characters, which must be an asterisk.

    \Input *pString1    - first string to compare
    \Input *pString2    - second string to compare

    \Output
        int32_t         - like strcmp()

    \Notes
       As specified per RFC 2818:

       \verbatim
       Matching is performed using the matching rules specified by
       [RFC2459].  If more than one identity of a given type is present in
       the certificate (e.g., more than one dNSName name, a match in any one
       of the set is considered acceptable.) Names may contain the wildcard
       character * which is considered to match any single domain name
       component or component fragment. E.g., *.a.com matches foo.a.com but
       not bar.foo.a.com. f*.com matches foo.com but not bar.com.
       \endverbatim

    \Version 03/19/2009 (jbrookes)
*/
/**************************************************************************F*/
static int32_t _WildcardMatchNoCase(const char *pString1, const char *pString2)
{
    int32_t r;
    char c1, c2;

    do {
        c1 = *pString1++;
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 ^= 32;
        c2 = *pString2;
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 ^= 32;
        if ((c2 == '*') && c1 != ('.'))
        {
            r = _WildcardMatchNoCase(pString1, pString2+1);
            if (r == 0)
            {
                break;
            }
            r = 0;
        }
        else
        {
            pString2 += 1;
            r = c1-c2;
        }
    } while ((c1 != 0) && (c2 != 0) && (r == 0));

    return(r);
}

/*F**************************************************************************/
/*!
    \Function _GenerateRandom

    \Description
        Generate random data

    \Input *pRandomBuf - output of random data
    \Input iRandomLen  - length of random data
    \Input *pArc4      - temp buffer for ARC4 use

    \Version 11/05/2004 gschaefer
*/
/**************************************************************************F*/
static void _GenerateRandom(uint8_t *pRandomBuf, int32_t iRandomLen, CryptArc4T *pArc4)
{
    int32_t iCount;

    // if first time through, init the base ticker
    if (_ProtoSSL_Random[0] == 0)
    {
        _ProtoSSL_Random[0] = NetTick();
    }

    // always set second time
    _ProtoSSL_Random[1] += NetTick();

    // increment sequence numver
    _ProtoSSL_Random[2] += 1;

    // grab data from stack (might be random, might not)
    for (iCount = 0; iCount < 32; ++iCount)
    {
        _ProtoSSL_Random[3] += (&iCount)[iCount];
    }

    // generate data if desired
    if (pRandomBuf != NULL)
    {
        // init the stream cipher for use as random generator
        CryptArc4Init(pArc4, (uint8_t *)&_ProtoSSL_Random, sizeof(_ProtoSSL_Random), 3);
        // output pseudo-random data
        CryptArc4Apply(pArc4, pRandomBuf, iRandomLen);
    }
}

/*F**************************************************************************/
/*!
    \Function _SubjectAlternativeMatch

    \Description
        To be completed

    \Input *pStrHost    - to be completed
    \Input *pStrSubjAlt - to be completed

    \Output
        int32_t         - like strcmp()

    \Version 08/06/2010 (jbrookes)
*/
/**************************************************************************F*/
static int32_t _SubjectAlternativeMatch(const char *pStrHost, const char *pStrSubjAlt)
{
    // strSubject: match the capacity of X509CertificateT::strSubjectAlt
    char strSubject[sizeof(_ProtoSSL_CACerts[0].pX509Cert->strSubjectAlt)];
    char *pSubject = strSubject, *pNext;

    // make a copy of pStrSubjAlt
    strnzcpy(strSubject, pStrSubjAlt, sizeof(strSubject));
    while ((pSubject != NULL) && (*pSubject != '\0'))
    {
        if ((pNext = strchr(pSubject, ',')) != NULL)
        {
            *pNext++ = '\0'; // "www.a.com,www.b.com"-->"www.a.com\0www.b.com"
        }

        // skip space and control characters
        while ((*pSubject > 0) && (*pSubject <= ' '))
        {
            pSubject++;
        }

        // call _WildcardMatchNoCase to do comparison
        if (_WildcardMatchNoCase(pStrHost, pSubject) == 0)
        {
            return(0);
        }

        // move to: www.b.com
        pSubject = pNext;
    }

    return(-1);
}

/*F**************************************************************************/
/*!
    \Function _ResetSecureState

    \Description
        Reset secure state.  Does not affect the connection, if any.

    \Input *pState  - Reference pointer
    \Input iSecure  - secure status (0=disabled, 1=enabled)

    \Output
        int32_t     - SOCKERR_NONE on success, SOCKERR_NOMEM on failure

    \Version 03/17/2010 (jbrookes)
*/
/**************************************************************************F*/
static int32_t _ResetSecureState(ProtoSSLRefT *pState, int32_t iSecure)
{
    SecureStateT *pSecure;

    // see if we need to get rid of secure state
    if (!iSecure && (pState->pSecure != NULL))
    {
        DirtyMemFree(pState->pSecure, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pSecure = NULL;
    }

    // see if we need to alloc secure state
    if (iSecure && (pState->pSecure == NULL))
    {
        pState->pSecure = DirtyMemAlloc(sizeof(*pState->pSecure), PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        if (pState->pSecure != NULL)
        {
            memset(pState->pSecure, 0, sizeof(*pState->pSecure));
        }
    }

    // reset secure state if present
    if ((pSecure = pState->pSecure) != NULL)
    {
        // initialize random number generator
        _GenerateRandom(NULL, 0, &pState->pSecure->ReadArc4);

        // no data to send
        pSecure->iSendProg = 0;
        pSecure->iSendSize = 0;
        pSecure->uSendSeqn = 0;

        // reset receive stuff
        pSecure->iRecvProg = 0;
        pSecure->iRecvSize = 0;
        pSecure->uRecvSeqn = 0;

        // reset the handshake hash data
        CryptMD5Init(&pSecure->HandshakeMD5);
        CryptSha1Init(&pSecure->HandshakeSHA);
    }

    // return allocate error if secure wanted and failed
    return((iSecure && !pSecure) ? SOCKERR_NOMEM : SOCKERR_NONE);
}

/*F**************************************************************************/
/*!
    \Function _ResetState

    \Description
        Reset connection back to unconnected state (will disconnect from server).

    \Input *pState  - Reference pointer
    \Input iSecure  - to be completed

    \Output
        int32_t     - SOCKERR_NONE on success, SOCKERR_NOMEM on failure

    \Version 03/25/2004 (GWS)
*/
/**************************************************************************F*/
static int32_t _ResetState(ProtoSSLRefT *pState, int32_t iSecure)
{
    // close socket if needed
    if (pState->pSock != NULL)
    {
        pState->iLastSocketError = SocketInfo(pState->pSock, 'serr', 0, NULL, 0);
        SocketClose(pState->pSock);
        pState->pSock = NULL;
    }

    // done with resolver record
    if (pState->pHost != NULL)
    {
        pState->pHost->Free(pState->pHost);
        pState->pHost = NULL;
    }

    // free server cert memory
    if (pState->pCertToVal != NULL)
    {
        DirtyMemFree(pState->pCertToVal, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pCertToVal = NULL;
    }

    // done with dirtycert
    if (pState->bCARequested && (pState->iCARequestId >= 0))
    {
        DirtyCertCARequestFree(pState->iCARequestId);
        pState->iCARequestId = -1;
    }

    // reset the state
    pState->iState = ST_IDLE;
    pState->iClosed = 1;
    pState->bCARequested = FALSE;

    // reset secure state
    return(_ResetSecureState(pState, iSecure));
}

/*F**************************************************************************/
/*!
    \Function _SendPacket

    \Description
        Build an outgoing SSL packet. Accepts head/body buffers to allow easy
        contruction of header / data in individual buffers (or set one to null
        if data already combind).

    \Input *pState   - ssl state ptr
    \Input uType     - record type
    \Input *pHeadPtr - pointer to head buffer
    \Input iHeadLen  - length of head buffer
    \Input *pBodyPtr - pointer to data to send
    \Input iBodyLen  - length of data to send

    \Output int32_t - -1=invalid length, zero=no error

    \Version 11/10/2005 gschaefer
*/
/**************************************************************************F*/
static int32_t _SendPacket(ProtoSSLRefT *pState, uint8_t uType, const void *pHeadPtr, int32_t iHeadLen, const void *pBodyPtr, int32_t iBodyLen)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t *pSend;
    int32_t iSize;

    // verify if the input buffer length is good
    if ((iHeadLen + iBodyLen + SSL_SNDOVH_PACKET) > (signed)sizeof(pSecure->SendData))
    {
        NetPrintf(("protossl: _SendPacket: potential buffer overrun (iHeadLen=%d, iBodyLen=%d)\n", iHeadLen, iBodyLen));
        return(-1);
    }

    // setup record frame
    pSecure->SendData[0] = uType;
    pSecure->SendData[1] = (uint8_t)(SSL3_VERSION>>8);
    pSecure->SendData[2] = (uint8_t)(SSL3_VERSION>>0);

    // point to data area
    pSend = pSecure->SendData+5;
    iSize = 0;

    // copy over the head
    memcpy(pSend+iSize, pHeadPtr, iHeadLen);
    iSize += iHeadLen;

    // copy over the body
    memcpy(pSend+iSize, pBodyPtr, iBodyLen);
    iSize += iBodyLen;

    // hash all handshake data for "finish" packet
    if (uType == SSL3_REC_HANDSHAKE)
    {
        CryptMD5Update(&pSecure->HandshakeMD5, pSend, iSize);
        CryptSha1Update(&pSecure->HandshakeSHA, pSend, iSize);
    }

    // handle encryption
    if ((pState->iState >= ST3_SEND_FINISH) && (pState->iState <= ST3_SECURE) && (pSecure->pCipher != NULL))
    {
        uint8_t MacTemp[20];

        struct {
            uint8_t uSeqn[8];
            uint8_t uType;
            uint8_t uLength[2];
        } MacData;

        // calc the mac address
        MacData.uSeqn[0] = 0;
        MacData.uSeqn[1] = 0;
        MacData.uSeqn[2] = 0;
        MacData.uSeqn[3] = 0;
        MacData.uSeqn[4] = (uint8_t)((pSecure->uSendSeqn>>24)&255);
        MacData.uSeqn[5] = (uint8_t)((pSecure->uSendSeqn>>16)&255);
        MacData.uSeqn[6] = (uint8_t)((pSecure->uSendSeqn>>8)&255);
        MacData.uSeqn[7] = (uint8_t)((pSecure->uSendSeqn>>0)&255);
        MacData.uType = pSecure->SendData[0];
        MacData.uLength[0] = (uint8_t)(iSize>>8);
        MacData.uLength[1] = (uint8_t)(iSize>>0);

        // handle MD5 MAC
        if (pSecure->pCipher->uMac == SSL3_MAC_MD5)
        {
            CryptMD5T MD5Context;
            CryptMD5Init(&MD5Context);
            CryptMD5Update(&MD5Context, pSecure->pClientMAC, 16);
            CryptMD5Update(&MD5Context, _SSL3_Pad1, 48);
            CryptMD5Update(&MD5Context, (unsigned char *)&MacData, sizeof(MacData));
            CryptMD5Update(&MD5Context, pSend, iSize);
            CryptMD5Final(&MD5Context, MacTemp, 16);

            CryptMD5Init(&MD5Context);
            CryptMD5Update(&MD5Context, pSecure->pClientMAC, 16);
            CryptMD5Update(&MD5Context, _SSL3_Pad2, 48);
            CryptMD5Update(&MD5Context, MacTemp, 16);
            CryptMD5Final(&MD5Context, pSend+iSize, 16);
            iSize += 16;
        }

        // handle SHA MAC
        if (pSecure->pCipher->uMac == SSL3_MAC_SHA)
        {
            CryptSha1T SHAContext;
            CryptSha1Init(&SHAContext);
            CryptSha1Update(&SHAContext, pSecure->pClientMAC, 20);
            CryptSha1Update(&SHAContext, _SSL3_Pad1, 40);
            CryptSha1Update(&SHAContext, (unsigned char *)&MacData, sizeof(MacData));
            CryptSha1Update(&SHAContext, pSend, iSize);
            CryptSha1Final(&SHAContext, MacTemp, 20);

            CryptSha1Init(&SHAContext);
            CryptSha1Update(&SHAContext, pSecure->pClientMAC, 20);
            CryptSha1Update(&SHAContext, _SSL3_Pad2, 40);
            CryptSha1Update(&SHAContext, MacTemp, 20);
            CryptSha1Final(&SHAContext, pSend+iSize, 20);
            iSize += 20;
        }

        #if DEBUG_MSG_LIST
        NetPrintf(("protossl: _SendPacket (secure enc=%d mac=%d): type=%d, size=%d, seq=%d\n",
            pSecure->pCipher->uEnc, pSecure->pCipher->uMac, pSecure->SendData[0], iSize, pSecure->uSendSeqn));
        #endif
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->SendData, iSize+5, "_SendPacket");
        #endif

        // encrypt the data/mac portion
        if (pSecure->pCipher->uEnc == SSL3_ENC_RC4)
        {
            CryptArc4Apply(&pSecure->WriteArc4, pSend, iSize);
        }
        if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
        {
            int32_t iPadBytes;

            // calculate padding
            if ((iPadBytes = 16 - (iSize % 16)) == 0)
            {
                iPadBytes = 16;
            }

            // set the padding data
            memset(pSend+iSize, iPadBytes-1, iPadBytes);
            iSize += iPadBytes;

            // encrypt the message
            CryptAesEncrypt(&pSecure->WriteAes, pSend, iSize);
        }
    }
    else
    {
        #if DEBUG_MSG_LIST
        NetPrintf(("protossl: _SendPacket (unsecure): type=%d, size=%d, seq=%d\n", pSecure->SendData[0], iSize, pSecure->uSendSeqn));
        #endif
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->SendData, iSize+5, "_SendPacket");
        #endif
    }

    // setup total record size
    pSecure->SendData[3] = (uint8_t)(iSize>>8);
    pSecure->SendData[4] = (uint8_t)(iSize>>0);

    // setup buffer pointers
    pSecure->iSendProg = 0;
    pSecure->iSendSize = iSize+5;

    // increment the sequence
    pSecure->uSendSeqn += 1;
    return(0);
}

/*F**************************************************************************/
/*!
    \Function _RecvPacket

    \Description
        Decode ssl3 record header, decrypt data, verify mac.

    \Input *pState  - ssl state ptr

    \Output
        int32_t     - 0 for success, negative for error

    \Version 11/10/2005 gschaefer
*/
/**************************************************************************F*/
static int32_t _RecvPacket(ProtoSSLRefT *pState)
{
    int32_t iSize;
    SecureStateT *pSecure = pState->pSecure;

    // default to bogus base (no data)
    pSecure->iRecvBase = pSecure->iRecvSize;

    // make sure we have a packet
    if (pSecure->iRecvProg != pSecure->iRecvSize)
    {
        NetPrintf(("protossl: _RecvPacket: no packet queued\n"));
        return(-1);
    }

    // check the record type
    if ((pSecure->RecvData[0] < SSL3_REC_CIPHER) || (pSecure->RecvData[0] > SSL3_REC_APPLICATION))
    {
        NetPrintf(("protossl: _RecvPacket: unknown record type %d\n", pSecure->RecvData[0]));
        return(-2);
    }

    // check the version
    if ((pSecure->RecvData[1] != (SSL3_VERSION>>8)) || (pSecure->RecvData[2] != (SSL3_VERSION&255)))
    {
        NetPrintf(("protossl: _RecvPacket: unsupported version 0x%02x%02x\n", pSecure->RecvData[1], pSecure->RecvData[2]));
        return(-3);
    }

    // validate the length
    iSize = (pSecure->RecvData[3]<<8)|pSecure->RecvData[4];
    if (pSecure->iRecvSize != iSize+5)
    {
        NetPrintf(("protossl: _RecvPacket: invalid length (%d != %d)\n", pSecure->iRecvSize, iSize+5));
        return(-4);
    }

    // point to data
    pSecure->iRecvBase = 5;

    // handle decryption
    if ((pState->iState >= ST3_RECV_FINISH) && (pState->iState <= ST3_SECURE) && (pSecure->pCipher != NULL))
    {
        uint8_t MacTemp[20];

        struct {
            uint8_t uSeqn[8];
            uint8_t uType;
            uint8_t uLength[2];
        } MacData;

        iSize = pSecure->iRecvSize-pSecure->iRecvBase;

        // decrypt the data/mac portion
        if (pSecure->pCipher->uEnc == SSL3_ENC_RC4)
        {
            CryptArc4Apply(&pSecure->ReadArc4, pSecure->RecvData+pSecure->iRecvBase, iSize);
        }
        if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
        {
            int32_t iPadBytes;
            // decrypt the data
            CryptAesDecrypt(&pSecure->ReadAes, pSecure->RecvData+pSecure->iRecvBase, iSize);
            // read pad bytes
            iPadBytes = pSecure->RecvData[pSecure->iRecvBase+iSize-1] + 1;
            // remove pad bytes from input if the range is valid
            if ((iPadBytes > 0) && (iPadBytes < 17))
            {
                iSize -= iPadBytes;
            }
        }

        #if DEBUG_MSG_LIST
        NetPrintf(("protossl: _RecvPacket (secure enc=%d mac=%d): type=%d, size=%d, seq=%d\n",
            pSecure->pCipher->uEnc, pSecure->pCipher->uMac, pSecure->RecvData[0], iSize, pSecure->uRecvSeqn));
        #endif
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->RecvData, pSecure->iRecvSize, "_RecvPacket");
        #endif

        // make sure there is room for mac
        if (iSize < pSecure->pCipher->uMac)
        {
            NetPrintf(("protossl: _RecvPacket: no room for mac (%d < %d)\n", iSize, pSecure->pCipher->uMac));
            return(-4);
        }
        iSize -= pSecure->pCipher->uMac;

        // remove the mac from size
        pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase+iSize;

        // calc the mac
        MacData.uSeqn[0] = 0;
        MacData.uSeqn[1] = 0;
        MacData.uSeqn[2] = 0;
        MacData.uSeqn[3] = 0;
        MacData.uSeqn[4] = (uint8_t)((pSecure->uRecvSeqn>>24)&255);
        MacData.uSeqn[5] = (uint8_t)((pSecure->uRecvSeqn>>16)&255);
        MacData.uSeqn[6] = (uint8_t)((pSecure->uRecvSeqn>>8)&255);
        MacData.uSeqn[7] = (uint8_t)((pSecure->uRecvSeqn>>0)&255);
        MacData.uType = pSecure->RecvData[0];
        MacData.uLength[0] = (uint8_t)(iSize>>8);
        MacData.uLength[1] = (uint8_t)(iSize>>0);

        // handle MD5 MAC
        if (pSecure->pCipher->uMac == SSL3_MAC_MD5)
        {
            CryptMD5T MD5Context;
            CryptMD5Init(&MD5Context);
            CryptMD5Update(&MD5Context, pSecure->pServerMAC, 16);
            CryptMD5Update(&MD5Context, _SSL3_Pad1, 48);
            CryptMD5Update(&MD5Context, (char *)&MacData, sizeof(MacData));
            CryptMD5Update(&MD5Context, pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase);
            CryptMD5Final(&MD5Context, MacTemp, 16);

            CryptMD5Init(&MD5Context);
            CryptMD5Update(&MD5Context, pSecure->pServerMAC, 16);
            CryptMD5Update(&MD5Context, _SSL3_Pad2, 48);
            CryptMD5Update(&MD5Context, MacTemp, 16);
            CryptMD5Final(&MD5Context, MacTemp, 16);

            if (memcmp(MacTemp, pSecure->RecvData+pSecure->iRecvSize, 16) != 0)
            {
                NetPrintf(("protossl: _RecvPacket: bad MD5 MAC\n"));
                return(-5);
            }
        }

        // handle SHA MAC
        if (pSecure->pCipher->uMac == SSL3_MAC_SHA)
        {
            CryptSha1T SHAContext;
            CryptSha1Init(&SHAContext);
            CryptSha1Update(&SHAContext, pSecure->pServerMAC, 20);
            CryptSha1Update(&SHAContext, _SSL3_Pad1, 40);
            CryptSha1Update(&SHAContext, (unsigned char *)&MacData, sizeof(MacData));
            CryptSha1Update(&SHAContext, pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase);
            CryptSha1Final(&SHAContext, MacTemp, 20);

            CryptSha1Init(&SHAContext);
            CryptSha1Update(&SHAContext, pSecure->pServerMAC, 20);
            CryptSha1Update(&SHAContext, _SSL3_Pad2, 40);
            CryptSha1Update(&SHAContext, MacTemp, 20);
            CryptSha1Final(&SHAContext, MacTemp, 20);

            if (memcmp(MacTemp, pSecure->RecvData+pSecure->iRecvSize, 20) != 0)
            {
                NetPrintf(("protossl: _RecvPacket: bad SHA MAC\n"));
                return(-6);
            }
        }
    }
    else
    {
        #if DEBUG_MSG_LIST
        NetPrintf(("protossl: _RecvPacket (unsecure): type=%d, size=%d, seq=%d\n", pSecure->RecvData[0], iSize, pSecure->uRecvSeqn));
        #endif
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->RecvData, pSecure->iRecvSize, "_RecvPacket");
        #endif
    }

    // hash all handshake data for "finish" packet
    if ((pSecure->RecvData[0] == SSL3_REC_HANDSHAKE) && (pState->iState < ST3_RECV_FINISH))
    {
        CryptMD5Update(&pSecure->HandshakeMD5, pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase);
        CryptSha1Update(&pSecure->HandshakeSHA, pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase);
    }

    // increment the sequence number
    pSecure->uRecvSeqn += 1;
    pSecure->iRecvDone = 1;
    // return success
    return(0);
}

/*F**************************************************************************/
/*!
    \Function _RecvHandshake

    \Description
        Decode ssl3 handshake packet.

    \Input *pState  - ssl state ptr
    \Input uType    - desired handshake packet type

    \Output
        void *      - pointer to handshake packet start

    \Version 11/10/2005 gschaefer
*/
/**************************************************************************F*/
static const void *_RecvHandshake(ProtoSSLRefT *pState, uint8_t uType)
{
    int32_t iSize;
    SecureStateT *pSecure = pState->pSecure;
    uint8_t *pRecv = pSecure->RecvData+pSecure->iRecvBase;

    // make sure we got desired packet
    if (pRecv[0] != uType)
    {
        return(NULL);
    }

    // make sure we got entire packet
    iSize = (pRecv[1]<<16)|(pRecv[2]<<8)|(pRecv[3]<<0);
    if (pSecure->iRecvBase+4+iSize > pSecure->iRecvSize)
    {
        NetPrintf(("protossl: _RecvHandshake: packet at base %d is too long (%d vs %d)\n", pSecure->iRecvBase,
            iSize, pSecure->iRecvSize-pSecure->iRecvBase-4));
        return(NULL);
    }

    // advance to next item
    pSecure->iRecvBase += iSize+4;

    // point to data
    return(pRecv+4);
}

#if DIRTYCODE_LOGGING
/*F***************************************************************************/
/*!
    \Function _DebugFormatCertIdent

    \Description
        Print cert ident info to debug output.

    \Input *pIdent  - cert ident info
    \Input *pStrBuf - [out] buffer to format cert ident into
    \Input iBufLen  - length of buffer

    \Output
        char *      - pointer to result string

    \Version 01/13/2009 (jbrookes)
*/
/***************************************************************************F*/
static char *_DebugFormatCertIdent(const ProtoSSLCertIdentT *pIdent, char *pStrBuf, int32_t iBufLen)
{
    snzprintf(pStrBuf, iBufLen, "C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s",
        pIdent->strCountry, pIdent->strState, pIdent->strCity,
        pIdent->strOrg, pIdent->strUnit, pIdent->strCommon);
    return(pStrBuf);
}
#else
#define _DebugPrintCertIdent(__pCert, __pMessage)
#endif

#if DIRTYCODE_LOGGING
/*F***************************************************************************/
/*!
    \Function _DebugPrintCert

    \Description
        Print cert ident info to debug output.

    \Input *pCert       - cert to print debug info for
    \Input *pMessage    - string identifier

    \Version 01/13/2009 (jbrookes)
*/
/***************************************************************************F*/
static void _DebugPrintCert(const X509CertificateT *pCert, const char *pMessage)
{
    char strCertIdent[1024];
    NetPrintf(("protossl: %s\n", pMessage));
    NetPrintf(("protossl:  issuer: %s\n", _DebugFormatCertIdent(&pCert->Issuer, strCertIdent, sizeof(strCertIdent))));
    NetPrintf(("protossl: subject: %s\n", _DebugFormatCertIdent(&pCert->Subject, strCertIdent, sizeof(strCertIdent))));
    NetPrintf(("protossl: keytype: %d\n", pCert->iKeyType));
    NetPrintf(("protossl: keyexp size: %d\n", pCert->iKeyExpSize));
    NetPrintf(("protossl: keymod size: %d\n", pCert->iKeyModSize));
    NetPrintf(("protossl: sigtype: %d\n", pCert->iSigType));
    NetPrintf(("protossl: sigsize: %d\n", pCert->iSigSize));
    NetPrintf(("protossl: CertIsCA(%d): pathLenConstraints(%d)\n", pCert->iCertIsCA, pCert->iMaxHeight - 1));
}
#else
#define _DebugPrintCert(__pCert, __pMessage)
#endif

/*F***************************************************************************/
/*!
    \Function _FindPEMSignature

    \Description
        Find PEM signature in the input data, if it exists, and return the
        offset of the signature.

    \Input *pCertData       - pointer to data to scan
    \Input iCertSize        - size of input data
    \Input *pSigText        - signature text to find

    \Output
        int32_t             - offset of PEM signature text, or negative if not found

    \Version 01/14/2009 (jbrookes)
*/
/***************************************************************************F*/
static const uint8_t *_FindPEMSignature(const uint8_t *pCertData, int32_t iCertSize, const char *pSigText)
{
    int32_t iSigLen = (int32_t)strlen(pSigText);
    int32_t iCertIdx;

    for (iCertIdx = 0; iCertIdx < iCertSize; iCertIdx += 1)
    {
        if ((pCertData[iCertIdx] == *pSigText) && ((iCertSize - iCertIdx) >= iSigLen))
        {
            if (!strncmp((const char *)pCertData+iCertIdx, pSigText, iSigLen))
            {
                return(pCertData+iCertIdx);
            }
        }
    }
    return(NULL);
}

/*F***************************************************************************/
/*!
    \Function _FindPEMCertificateData

    \Description
        Finds PEM signature in the input data, if it exists, and stores the
        offsets to the beginning and end of the embedded signature data.

    \Input *pCertData       - pointer to data to scan
    \Input iCertSize        - size of input data
    \Input **pCertBeg       - [out] - storage for index to beginning of certificate data
    \Input **pCertEnd       - [out] - storage for index to end of certificate data

    \Output
        int32_t             - 0=did not find certificate, else did find certificate

    \Version 01/14/2009 (jbrookes)
*/
/***************************************************************************F*/
static int32_t _FindPEMCertificateData(const uint8_t *pCertData, int32_t iCertSize, const uint8_t **pCertBeg, const uint8_t **pCertEnd)
{
    static const char _strPemBeg[] = "-----BEGIN CERTIFICATE-----";
    static const char _strPemEnd[] = "-----END CERTIFICATE-----";
    static const char _str509Beg[] = "-----BEGIN X509 CERTIFICATE-----";
    static const char _str509End[] = "-----END X509 CERTIFICATE-----";

    // make sure "end-cert" occurs after start since we support bundles
    if (((*pCertBeg = _FindPEMSignature(pCertData, iCertSize, _strPemBeg)) != NULL) &&
        ((*pCertEnd = _FindPEMSignature(*pCertBeg, pCertData+iCertSize-*pCertBeg, _strPemEnd)) != NULL))
    {
        *pCertBeg += strlen(_strPemBeg);
        return(*pCertEnd-*pCertBeg);
    }
    else if (((*pCertBeg = _FindPEMSignature(pCertData, iCertSize, _str509Beg)) != NULL) &&
             ((*pCertEnd = _FindPEMSignature(*pCertBeg, pCertData+iCertSize-*pCertBeg, _str509End)) != NULL))
    {
        *pCertBeg += strlen(_str509Beg);
        return(*pCertEnd-*pCertBeg);
    }
    return(0);
}

/*F***************************************************************************/
/*!
    \Function _CompareIdent

    \Description
        Compare two identities

    \Input *pIdent1     - pointer to ProtoSSLCertIdentT structure
    \Input *pIdent2     - pointer to ProtoSSLCertIdentT structure
    \Input bMatchUnit   - if TRUE compare the Unit string

    \Output
        int32_t         - zero=match, non-zero=no-match

    \Notes
        Unit is considered an optional match criteria by openssl.

    \Version 03/11/2009 (jbrookes)
*/
/***************************************************************************F*/
static int32_t _CompareIdent(const ProtoSSLCertIdentT *pIdent1, const ProtoSSLCertIdentT *pIdent2, uint8_t bMatchUnit)
{
    int32_t iResult;
    iResult  = strcmp(pIdent1->strCountry, pIdent2->strCountry) != 0;
    iResult += strcmp(pIdent1->strState, pIdent2->strState) != 0;
    iResult += strcmp(pIdent1->strCity, pIdent2->strCity) != 0;
    iResult += strcmp(pIdent1->strOrg, pIdent2->strOrg) != 0;
    iResult += strcmp(pIdent1->strCommon, pIdent2->strCommon) != 0;
    if (bMatchUnit)
    {
        iResult += strcmp(pIdent1->strUnit, pIdent2->strUnit) != 0;
    }
    return(iResult);
}

/*F***************************************************************************/
/*!
    \Function _CheckDuplicateCA

    \Description
        Check to see if the given CA cert already exists in our list of
        CA certs.

    \Input *pNewCACert  - pointer to new CA cert to check for duplicates of

    \Output
        int32_t         - zero=not duplicate, non-zero=duplicate

    \Version 05/11/2011 (jbrookes)
*/
/***************************************************************************F*/
static int32_t _CheckDuplicateCA(const X509CertificateT *pNewCACert)
{
    const ProtoSSLCACertT *pCACert;

    for (pCACert = _ProtoSSL_CACerts; pCACert != NULL; pCACert = pCACert->pNext)
    {
        if (!_CompareIdent(&pCACert->Subject, &pNewCACert->Subject, TRUE) && (pCACert->iKeyModSize == pNewCACert->iKeyModSize) &&
            !memcmp(pCACert->pKeyModData, pNewCACert->KeyModData, pCACert->iKeyModSize))
        {
            break;
        }
    }

    return(pCACert != NULL);
}

/*F*************************************************************************************************/
/*!
    \Function _ParseHeader

    \Description
        Parse an asn.1 header

    \Input *pData    - pointer to header data to parse
    \Input *pLast    - pointer to end of header
    \Input *pType    - pointer to storage for header type
    \Input *pSize    - pointer to storage for data size

    \Output uint8_t * - pointer to next block, or NULL if end of stream

    \Version 03/08/2002 (GWS)
*/
/*************************************************************************************************F*/
static const unsigned char *_ParseHeader(const unsigned char *pData, const unsigned char *pLast, int32_t *pType, int32_t *pSize)
{
    int32_t iCnt;
    uint32_t uLen;
    int32_t iType, iSize;
    #if DEBUG_ALL_OBJS
    const unsigned char *pDataStart = pData;
    #endif

    // reset the output
    if (pSize != NULL)
    {
        *pSize = 0;
    }
    if (pType != NULL)
    {
        *pType = 0;
    }

    // handle end of data
    if (pData == NULL)
    {
        return(NULL);
    }

    // make sure there is a byte available
    if (pData == pLast)
    {
        return(NULL);
    }

    // get the type
    iType = *pData;
    if (pType != NULL)
    {
        *pType = iType;
    }
    pData += 1;

    // make sure there is a byte available
    if (pData == pLast)
    {
        return(NULL);
    }

    // figure the length
    uLen = *pData++;
    if (uLen > 127)
    {
        // get number of bits
        iCnt = (uLen & 127);
        // calc the length
        for (uLen = 0; iCnt > 0; --iCnt)
        {
            // make sure there is data
            if (pData == pLast)
            {
                return(NULL);
            }
            // process the length byte
            uLen = (uLen << 8) | *pData++;
        }
    }
    // save the size
    iSize = (signed)uLen;
    if (pSize != NULL)
    {
        *pSize = iSize;
    }

    #if DEBUG_ALL_OBJS
    {
        NetPrintf(("protossl: _ParseHeader type=0x%02x size=%d\n", iType, iSize));
        NetPrintMem(pDataStart, iSize, "memory");
    }
    #endif

    // return pointer to next
    return(pData);
}

/*F*************************************************************************************************/
/*!
    \Function _ParseObject

    \Description
        Parse an object type

    \Input *pData   - pointer to object
    \Input iSize    - size of object

    \Output
        int32_t     - type of object

    \Version 03/08/2002 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _ParseObject(const unsigned char *pData, int32_t iSize)
{
    int32_t iType = 0;
    int32_t iIndex;

    // locate the matching type
    for (iIndex = 0; _SSL_ObjectList[iIndex].iType != ASN_OBJ_NONE; ++iIndex)
    {
        if ((iSize >= _SSL_ObjectList[iIndex].iSize) && (memcmp(pData, _SSL_ObjectList[iIndex].strData, _SSL_ObjectList[iIndex].iSize) == 0))
        {
            // save the type and return it
            iType = _SSL_ObjectList[iIndex].iType;

            #if DEBUG_ALL_OBJS
            NetPrintf(("protossl: _ParseObject obj=%d\n", iType));
            NetPrintMem(pData, _SSL_ObjectList[iIndex].iSize, "memory");
            #endif

            break;
        }
    }

    return(iType);
}

/*F*************************************************************************************************/
/*!
    \Function _ParseString

    \Description
        Extract a string

    \Input *pData   - pointer to data to extract string from
    \Input iSize    - size of source data
    \Input *pString - pointer to buffer to copy string into
    \Input iLength  - size of buffer

    \Notes
        The number of characters copied will be whichever is smaller between
        iSize and iLength-1.  If iLength-1 is greater than iSize (ie, the buffer
        is larger than the source string) the string output in pString will be
        NULL-terminated.

    \Version 03/08/2002 (GWS)
*/
/*************************************************************************************************F*/
static void _ParseString(const unsigned char *pData, int32_t iSize, char *pString, int32_t iLength)
{
    // extract
    for (; (iSize > 0) && (iLength > 1); --iSize, --iLength)
    {
        *pString++ = *pData++;
    }
    if (iLength > 0)
    {
        *pString = 0;
    }
}

/*F*************************************************************************************************/
/*!
    \Function _ParseStringMulti

    \Description
        Extract a string, appending to output instead of overwriting.

    \Input *pData   - pointer to data to extract string from
    \Input iSize    - size of source data
    \Input *pString - pointer to buffer to copy string into
    \Input iLength  - size of buffer

    \Notes
        The number of characters copied will be whichever is smaller between
        iSize and iLength-1.  If iLength-1 is greater than iSize (ie, the buffer
        is larger than the source string) the string output in pString will be
        NULL-terminated.

    \Version 03/25/2009 (jbrookes)
*/
/*************************************************************************************************F*/
static void _ParseStringMulti(const unsigned char *pData, int32_t iSize, char *pString, int32_t iLength)
{
    // find append point
    for (; (*pString != '\0') && (iLength > 1); --iLength)
    {
        pString += 1;
    }
    // extract
    for (; (iSize > 0) && (iLength > 1); --iSize, --iLength)
    {
        *pString++ = *pData++;
    }
    // termiante
    if (iLength > 0)
    {
        *pString = '\0';
    }
}

/*F***************************************************************************/
/*!
    \Function _ParseCertificate

    \Description
        Parse the x.509 certificate in nasty asn.1 form and put into structure

    \Input *pCert   - pointer to certificate to fill in from header data
    \Input *pData   - pointer to header data
    \Input iSize    - size of header

    \Output
        int32_t     - negative=error, zero=no error

    \Version 03/08/2002 (GWS)
*/
/***************************************************************************F*/
static int32_t _ParseCertificate(X509CertificateT *pCert, const uint8_t *pData, int32_t iSize)
{
    int32_t iType;
    int32_t iObjType;
    const uint8_t *pInfData;
    const uint8_t *pInfSkip;
    const uint8_t *pSigSkip;
    const uint8_t *pIssSkip;
    const uint8_t *pSubSkip;
    const uint8_t *pKeySkip;
    const uint8_t *pLast = pData+iSize;
    int32_t iKeySize;
    const uint8_t *pKeyData;

    // clear the certificate
    memset(pCert, 0, sizeof(*pCert));

    // process the base sequence
    pData = _ParseHeader(pData, pLast, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not parse base sequence (iType=%d)\n", iType));
        return(-1);
    }

    // process the info sequence
    pData = _ParseHeader(pInfData = pData, pLast, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not parse info sequence (iType=%d)\n", iType));
        return(-2);
    }
    pInfSkip = pData+iSize;

    // skip any non-integer tag (optional version)
    if (*pData != ASN_TYPE_INTEGER+ASN_PRIMITIVE)
    {
        pData = _ParseHeader(pData, pLast, NULL, &iSize);
        if (pData == NULL)
        {
            NetPrintf(("protossl: _ParseCertificate: could not skip non-integer tag\n"));
            return(-3);
        }
        pData += iSize;
    }

    // grab the certificate serial number
    pData = _ParseHeader(pData, pInfSkip, &iType, &iSize);
    if ((pData == NULL) || (iSize < 0) || ((unsigned)iSize > sizeof(pCert->SerialData)))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get certificate serial number (iSize=%d)\n", iSize));
        return(-4);
    }
    pCert->iSerialSize = iSize;
    memcpy(pCert->SerialData, pData, iSize);
    pData += iSize;

    // find signature algorithm
    pData = _ParseHeader(pData, pInfSkip, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature algorithm (iType=%d)\n", iType));
        return(-5);
    }
    pSigSkip = pData+iSize;

    // get the signature algorithm type
    pData = _ParseHeader(pData, pInfSkip, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_OBJECT+ASN_PRIMITIVE))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature algorithm type (iType=%d)\n", iType));
        return(-6);
    }

    // scan our known list
    pCert->iSigType = _ParseObject(pData, iSize);
    if (pCert->iSigType == ASN_OBJ_NONE)
    {
        NetPrintMem(pData, iSize, "protossl: unsupported signature algorithm");
        return(-7);
    }
    pData += iSize;

    // move on to the issuer
    pData = _ParseHeader(pSigSkip, pInfSkip, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get issuer (iType=%d)\n", iType));
        return(-8);
    }
    pIssSkip = pData+iSize;

    // parse all the issuer fields
    iObjType = 0;
    while ((pData = _ParseHeader(pData, pIssSkip, &iType, &iSize)) != NULL)
    {
        // skip past seqn/set references
        if ((iType != ASN_TYPE_SEQN+ASN_CONSTRUCT) && (iType != ASN_TYPE_SET+ASN_CONSTRUCT))
        {
            if (iType == ASN_TYPE_OBJECT+ASN_PRIMITIVE)
            {
                iObjType = _ParseObject(pData, iSize);
            }
            if ((iType == ASN_TYPE_PRINTSTR+ASN_PRIMITIVE) || (iType == ASN_TYPE_UTF8STR+ASN_PRIMITIVE) || (iType == ASN_TYPE_T61+ASN_PRIMITIVE))
            {
                if (iObjType == ASN_OBJ_COUNTRY)
                {
                    _ParseString(pData, iSize, pCert->Issuer.strCountry, sizeof(pCert->Issuer.strCountry));
                }
                if (iObjType == ASN_OBJ_STATE)
                {
                    _ParseString(pData, iSize, pCert->Issuer.strState, sizeof(pCert->Issuer.strState));
                }
                if (iObjType == ASN_OBJ_CITY)
                {
                    _ParseString(pData, iSize, pCert->Issuer.strCity, sizeof(pCert->Issuer.strCity));
                }
                if (iObjType == ASN_OBJ_ORGANIZATION)
                {
                    _ParseString(pData, iSize, pCert->Issuer.strOrg, sizeof(pCert->Issuer.strOrg));
                }
                if (iObjType == ASN_OBJ_UNIT)
                {
                    if (pCert->Issuer.strUnit[0] != '\0')
                    {
                        strnzcat(pCert->Issuer.strUnit, ", ", sizeof(pCert->Issuer.strUnit));
                    }
                    _ParseStringMulti(pData, iSize, pCert->Issuer.strUnit, sizeof(pCert->Issuer.strUnit));
                }
                if (iObjType == ASN_OBJ_COMMON)
                {
                    _ParseString(pData, iSize, pCert->Issuer.strCommon, sizeof(pCert->Issuer.strCommon));
                }
                iObjType = 0;
            }
            pData += iSize;
        }
    }

    // parse the validity info
    pData = _ParseHeader(pIssSkip, pLast, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get validity info (iType=%d)\n", iType));
        return(-9);
    }

    // get the from date
    pData = _ParseHeader(pData, pLast, &iType, &iSize);
    if ((pData == NULL) || ((iType != ASN_TYPE_UTCTIME) && (iType != ASN_TYPE_GENERALIZEDTIME)))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get from date (iType=%d)\n", iType));
        return(-10);
    }
    _ParseString(pData, iSize, pCert->strGoodFrom, sizeof(pCert->strGoodFrom));
    pData += iSize;

    // get the to date
    pData = _ParseHeader(pData, pLast, &iType, &iSize);
    if ((pData == NULL) || ((iType != ASN_TYPE_UTCTIME) && (iType != ASN_TYPE_GENERALIZEDTIME)))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get to date (iType=%d)\n", iType));
        return(-11);
    }
    _ParseString(pData, iSize, pCert->strGoodTill, sizeof(pCert->strGoodTill));
    pData += iSize;

    // move on to the subject
    pData = _ParseHeader(pData, pLast, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get subject (iType=%d)\n", iType));
        return(-12);
    }
    pSubSkip = pData+iSize;

    // parse all the subject fields
    iObjType = 0;
    while ((pData = _ParseHeader(pData, pSubSkip, &iType, &iSize)) != NULL)
    {
        // skip past seqn/set references
        if ((iType != ASN_TYPE_SEQN+ASN_CONSTRUCT) && (iType != ASN_TYPE_SET+ASN_CONSTRUCT))
        {
            if (iType == ASN_TYPE_OBJECT+ASN_PRIMITIVE)
            {
                iObjType = _ParseObject(pData, iSize);
            }
            if ((iType == ASN_TYPE_PRINTSTR+ASN_PRIMITIVE) || (iType == ASN_TYPE_UTF8STR+ASN_PRIMITIVE) || (iType == ASN_TYPE_T61+ASN_PRIMITIVE))
            {
                if (iObjType == ASN_OBJ_COUNTRY)
                {
                    _ParseString(pData, iSize, pCert->Subject.strCountry, sizeof(pCert->Subject.strCountry));
                }
                if (iObjType == ASN_OBJ_STATE)
                {
                    _ParseString(pData, iSize, pCert->Subject.strState, sizeof(pCert->Subject.strState));
                }
                if (iObjType == ASN_OBJ_CITY)
                {
                    _ParseString(pData, iSize, pCert->Subject.strCity, sizeof(pCert->Subject.strCity));
                }
                if (iObjType == ASN_OBJ_ORGANIZATION)
                {
                    _ParseString(pData, iSize, pCert->Subject.strOrg, sizeof(pCert->Subject.strOrg));
                }
                if (iObjType == ASN_OBJ_UNIT)
                {
                    if (pCert->Subject.strUnit[0] != '\0')
                    {
                        strnzcat(pCert->Subject.strUnit, ", ", sizeof(pCert->Subject.strUnit));
                    }
                    _ParseStringMulti(pData, iSize, pCert->Subject.strUnit, sizeof(pCert->Subject.strUnit));
                }
                if (iObjType == ASN_OBJ_COMMON)
                {
                    _ParseString(pData, iSize, pCert->Subject.strCommon, sizeof(pCert->Subject.strCommon));
                }
                iObjType = 0;
            }
            pData += iSize;
        }
    }

    // parse the public key
    pData = _ParseHeader(pSubSkip, pLast, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get public key (iType=%d)\n", iType));
        return(-13);
    }

    // find the key algorithm sequence
    pData = _ParseHeader(pData, pLast, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get key algorithm sequence (iType=%d)\n", iType));
        return(-14);
    }
    pKeySkip = pData+iSize;

    // grab the public key algorithm
    pData = _ParseHeader(pData, pKeySkip, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_OBJECT+ASN_PRIMITIVE))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get public key algorithm (iType=%d)\n", iType));
        return(-15);
    }
    pCert->iKeyType = _ParseObject(pData, iSize);

    // find the actual public key
    pData = _ParseHeader(pKeySkip, pLast, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_BITSTRING+ASN_PRIMITIVE) || (iSize < 1))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get actual public key (iType=%d, iSize=%d)\n", iType, iSize));
        return(-16);
    }

    // skip the "extra bits" indicator
    pKeyData = pData+1;
    iKeySize = iSize-1;
    pData += iSize;

    // move on to the optional info object
    pData = _ParseHeader(pData, pLast, &iType, &iSize);
    if ((pData != NULL) && (iType == ASN_EXPLICIT_TAG+ASN_TYPE_BITSTRING))
    {
        const uint8_t *pOptSkip = pData + iSize;
        int32_t iCritical = 0, iLen;
        iObjType = 0;
        while ((pData = _ParseHeader(pData, pOptSkip, &iType, &iSize)) != NULL)
        {
            // skip past seqn/set references
            if ((iType != ASN_TYPE_SEQN+ASN_CONSTRUCT) && (iType != ASN_TYPE_SET+ASN_CONSTRUCT))
            {
                if (iType == ASN_TYPE_OBJECT+ASN_PRIMITIVE)
                {
                    iObjType = _ParseObject(pData, iSize);
                }
                if (iObjType == ASN_OBJ_SUBJECT_ALT)
                {
                    if (iType == ASN_TYPE_OCTSTRING+ASN_PRIMITIVE)
                    {
                        continue;
                    }
                    if (iType == ASN_IMPLICIT_TAG+2)
                    {
                        if (pCert->strSubjectAlt[0] != '\0')
                        {
                            strnzcat(pCert->strSubjectAlt, ",", sizeof(pCert->strSubjectAlt));
                        }
                        _ParseStringMulti(pData, iSize, pCert->strSubjectAlt, sizeof(pCert->strSubjectAlt));
                    }
                }
                if (iObjType == ASN_OBJ_BASIC_CONSTRAINTS)
                {
                    // obj_basic_constraints: [boolean: critical]<oct><seq><boolean: isCA>[integer: pathLenConstraints]
                    if (iType == ASN_TYPE_OCTSTRING+ASN_PRIMITIVE)
                    {
                        // if no critical flag is present, mark it as critical. ex: https://www.wellsfargo.com/
                        if (iCritical == 0)
                        {
                            iCritical = 1;
                        }
                        // do not add pData for oct
                        continue;
                    }

                    if ((iType == ASN_TYPE_BOOLEAN+ASN_PRIMITIVE) && (iSize == 1))
                    {
                        if (!iCritical)
                        {
                            // check if the critical flag is set (the basic constraints MUST be critical)
                            if ((iCritical = *pData) == 0)
                            {
                                return(-100); // what error code should I use...
                            }
                        }
                        else
                        {
                            // this is the flag to indicate whether it's a CA
                            pCert->iCertIsCA = (*pData != 0) ? TRUE : FALSE;
                        }
                    }

                    if ((iType == ASN_TYPE_INTEGER+ASN_PRIMITIVE) && (pCert->iCertIsCA != FALSE)
                        && (iSize <= (signed)sizeof(pCert->iMaxHeight)))
                    {
                        for (iLen = 0; iLen < iSize; iLen++)
                        {
                            pCert->iMaxHeight = (pCert->iMaxHeight << 8) | pData[iLen];
                        }
                        /*
                        As per http://tools.ietf.org/html/rfc2459#section-4.2.1.10:
                        A value of zero indicates that only an end-entity certificate may follow in the path.
                        Where it appears, the pathLenConstraint field MUST be greater than or equal to zero.
                        Where pathLenConstraint does not appear, there is no limit to the allowed length of
                        the certification path.

                        In our case (iMaxHeight), a value of zero means no pathLenConstraint is present,
                        1 means the pathLenConstraint is 0,
                        2 means the pathLenConstraint is 1,
                        ...
                        */
                        if (pCert->iMaxHeight++ < 0)
                        {
                            return(-101);
                        }
                    }
                }
                pData += iSize;
            }
        }
    }
    if (pCert->strSubjectAlt[0] != '\0')
    {
        NetPrintf(("protossl: parsed SAN='%s'\n", pCert->strSubjectAlt));
    }

    // parse signature algorithm sequence
    pData = _ParseHeader(pInfSkip, pSigSkip, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature algorithm sequence (iType=%d)\n", iType));
        return(-18);
    }
    pSigSkip = pData+iSize;

    // parse signature algorithm idenfier
    pData = _ParseHeader(pData, pSigSkip,  &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_OBJECT+ASN_PRIMITIVE))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature algorithm identifier (iType=%d)\n", iType));
        return(-19);
    }
    pCert->iSigType = _ParseObject(pData, iSize);

    // parse the signature data
    pData = _ParseHeader(pSigSkip, pLast, &iType, &iSize);
    if ((pData == NULL) || (iType != ASN_TYPE_BITSTRING+ASN_PRIMITIVE) || (iSize < 1))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature data (iType=%d, iSize=%d)\n", iType, iSize));
        return(-20);
    }
    if (iSize > (signed)(sizeof(pCert->SigData)+1))
    {
        NetPrintf(("protossl: _ParseCertificate: signature data is too large (sig=%d, max=%d)\n", iSize-1, sizeof(pCert->SigData)));
        return(-21);
    }
    pCert->iSigSize = iSize-1;
    memcpy(pCert->SigData, pData+1, iSize-1);
    pData += iSize;

    // parse the public key data (extract modulus & exponent)
    if (pCert->iKeyType == ASN_OBJ_RSA_PKCS_KEY)
    {
        int32_t iKeyModSize, iKeyExpSize;
        const uint8_t *pKeyModData, *pKeyExpData;

        // parse the sequence
        pData = _ParseHeader(pKeyData, pKeyData+iKeySize, &iType, &iSize);
        if ((pData == NULL) || (iType != ASN_TYPE_SEQN+ASN_CONSTRUCT))
        {
            NetPrintf(("protossl: _ParseCertificate: could not get sequence (iType=%d)\n", iType));
            return(-22);
        }

        // parse the modulus
        pData = _ParseHeader(pData, pKeyData+iKeySize, &iType, &iSize);
        if ((pData == NULL) || (iType != ASN_TYPE_INTEGER+ASN_PRIMITIVE) || (iSize < 4))
        {
            NetPrintf(("protossl: _ParseCertificate: could not get modulus (iType=%d, iSize=%d)\n", iType, iSize));
            return(-23);
        }

        // point to module data
        iKeyModSize = iSize;
        pKeyModData = pData;

        // skip leading zero if present
        if (*pKeyModData == 0)
        {
            pKeyModData += 1;
            iKeyModSize -= 1;
        }

        // save copy of modulus into cert structure
        if ((unsigned)iKeyModSize > sizeof(pCert->KeyModData))
        {
            NetPrintf(("protossl: _ParseCertificate: modulus data is too large (mod=%d, max=%d)\n", iKeyModSize, sizeof(pCert->KeyModData)));
            return(-24);
        }
        pCert->iKeyModSize = iKeyModSize;
        memcpy(pCert->KeyModData, pKeyModData, iKeyModSize);
        pData += iSize;

        // parse the exponent
        pData = _ParseHeader(pData, pKeyData+iKeySize, &iType, &iSize);
        if ((pData == NULL) || (iType != ASN_TYPE_INTEGER+ASN_PRIMITIVE) || (iSize < 1))
        {
            NetPrintf(("protossl: _ParseCertificate: could not get exponent (iType=%d, iSize=%d)\n", iType, iSize));
            return(-25);
        }

        // point to exponent data
        iKeyExpSize = iSize;
        pKeyExpData = pData;

        // skip leading zero if present
        if (*pKeyExpData == 0)
        {
            pKeyExpData += 1;
            iKeyExpSize -= 1;
        }

        // save copy of exponent into cert structure
        if (iKeyExpSize > (signed)sizeof(pCert->KeyExpData))
        {
            NetPrintf(("protossl: _ParseCertificate: exponent data is too large (exp=%d, max=%d)\n", iKeyExpSize, sizeof(pCert->KeyExpData)));
            return(-26);
        }
        pCert->iKeyExpSize = iKeyExpSize;
        memcpy(pCert->KeyExpData, pKeyExpData, iKeyExpSize);
        pData += iSize;
    }

    // generate the certificate hash
    switch (pCert->iSigType)
    {
        case ASN_OBJ_RSA_PKCS_MD2:
        {
            CryptMD2T MD2;
            CryptMD2Init(&MD2);
            CryptMD2Update(&MD2, pInfData, pInfSkip-pInfData);
            CryptMD2Final(&MD2, pCert->HashData, pCert->iHashSize = MD2_BINARY_OUT);
            break;
        }

        case ASN_OBJ_RSA_PKCS_MD5:
        {
            CryptMD5T MD5;
            CryptMD5Init(&MD5);
            CryptMD5Update(&MD5, pInfData, pInfSkip-pInfData);
            CryptMD5Final(&MD5, pCert->HashData, pCert->iHashSize = MD5_BINARY_OUT);
            break;
        }

        case ASN_OBJ_RSA_PKCS_SHA1:
        {
            CryptSha1T Sha1;
            CryptSha1Init(&Sha1);
            CryptSha1Update(&Sha1, pInfData, pInfSkip-pInfData);
            CryptSha1Final(&Sha1, pCert->HashData, pCert->iHashSize = CRYPTSHA1_HASHSIZE);
            break;
        }

        default:
            NetPrintf(("protossl: unknown signature algorithm, should never get here\n"));
            pCert->iHashSize = 0;
    }

    // certificate parsed successfully
    return(0);
}

/*F***************************************************************************/
/*!
    \Function _VerifySignature

    \Description
        Check an X.509 signature for validity

    \Input *pState      - module state
    \Input *pCert       - pointer to certificate to validate
    \Input *pKeyModData - pointer to key modulus data
    \Input iKeyModSize  - size of key modulus data
    \Input *pKeyExpData - pointer to key exponent data
    \Input iKeyExpSize  - size of key exponent data

    \Output
        int32_t         - zero=success, else failure

    \Notes
        Once the signature has been decrypted, should really decode the
        padding to determine the location of the hash.  However, here we
        cheat and rely on the fact that the hash will be at a fixed position
        from the end of the decrypted signature.  For signatures whose length
        is even then the signature will be the last uHashLen bytes, for odd
        length signatures then a one byte adjustment has to be made.
        -sbevan, 03/03/2004

    \Version 07/22/2009 (jbrookes)
*/
/***************************************************************************F*/
static int32_t _VerifySignature(ProtoSSLRefT *pState, X509CertificateT *pCert, const uint8_t *pKeyModData, int32_t iKeyModSize, const uint8_t *pKeyExpData, int32_t iKeyExpSize)
{
    CryptRSAT RSA;
    uint32_t uTick = NetTick();

    // decrypt certificate signature block using CA public key
    if (CryptRSAInit(&RSA, pKeyModData, iKeyModSize, pKeyExpData, iKeyExpSize))
    {
        return(-1);
    }
    CryptRSAInitSignature(&RSA, pCert->SigData, pCert->iSigSize);
    CryptRSAEncrypt(&RSA);

    uTick = NetTickDiff(NetTick(), uTick);
    #if DEBUG_ENC_PERF
    NetPrintf(("protossl: SSL Perf (rsa sig) %dms\n", uTick));
    #endif
    if (pState != NULL)
    {
        pState->pSecure->uTimer += uTick;
    }

    // compare decrypted hash data (from signature block) with precalculated certificate body hash
    return(memcmp(pCert->HashData, RSA.EncryptBlock+(pCert->iSigSize&65534)-pCert->iHashSize, pCert->iHashSize));
}

/*F***************************************************************************/
/*!
    \Function _SetFailureCertInfo

    \Description
        Set the certificate info (used on failure)

    \Input *pState      - module state (may be NULL)
    \Input *pCert       - pointer to certificate to fill in from header data (may be NULL)

    \Version 01/24/2012 (szhu)
*/
/***************************************************************************F*/
static void _SetFailureCertInfo(ProtoSSLRefT *pState, X509CertificateT *pCert)
{
    if ((pState != NULL) && (pCert != NULL) && (pState->bCertInfoSet == FALSE))
    {
        memcpy(&pState->CertInfo.Ident, &pCert->Issuer, sizeof(pState->CertInfo.Ident));
        pState->CertInfo.iKeyModSize = pCert->iSigSize; // CACert::KeyModSize == Cert::SigSize
        pState->bCertInfoSet = TRUE;
    }
}

/*F***************************************************************************/
/*!
    \Function _VerifyCertificate

    \Description
        Verify a previously parsed x.509 certificate

    \Input *pState      - module state (may be NULL)
    \Input *pCert       - pointer to certificate to fill in from header data
    \Input bCertIsCA    - is the certificate a CA?

    \Output int32_t     - zero=success, positive=duplicate, else error

    \Version 03/11/2009 (gschaefer)
*/
/***************************************************************************F*/
static int32_t _VerifyCertificate(ProtoSSLRefT *pState, X509CertificateT *pCert, uint8_t bCertIsCA)
{
    int32_t iResult = 0;

    // if self-signed permitted and certificate is self-signed, then point to its key info (mod+exp)
    if ((bCertIsCA == TRUE) && (_CompareIdent(&pCert->Subject, &pCert->Issuer, TRUE) == 0))
    {
        if (_VerifySignature(pState, pCert, pCert->KeyModData, pCert->iKeyModSize, pCert->KeyExpData, pCert->iKeyExpSize) != 0)
        {
            NetPrintf(("protossl: _VerifyCertificate: signature hash mismatch on self-signed cert\n"));
            _DebugPrintCert(pCert, "failed cert");
            return(-30);
        }
    }
    else
    {
        ProtoSSLCACertT *pCACert;

        #if DEBUG_VAL_CERT
        if ((bCertIsCA == TRUE) && (pCert->iCertIsCA == FALSE)) // when ProtoSSLSetCACert* is called
        {
            NetPrintf(("protossl: _VerifyCertificate: warning --- trying to add a non-CA cert as a CA.\n"));
            _DebugPrintCert(pCert, "warning cert");
        }
        #endif

        // locate a matching CA
        for (pCACert = _ProtoSSL_CACerts; pCACert != NULL; pCACert = pCACert->pNext)
        {
            // first, compare to see if our subject matches their issuer
            if ((_CompareIdent(&pCACert->Subject, &pCert->Issuer, (bCertIsCA||pCert->iCertIsCA)) != 0) || (pCACert->iKeyModSize != pCert->iSigSize))
            {
                continue;
            }

            // verify against this CA
            if (_VerifySignature(pState, pCert, pCACert->pKeyModData, pCACert->iKeyModSize, pCACert->KeyExpData, pCACert->iKeyExpSize) != 0)
            {
                continue;
            }

            // if the CA hasn't been verified already, do that now
            if (pCACert->pX509Cert != NULL)
            {
                if ((iResult = _VerifyCertificate(pState, pCACert->pX509Cert, TRUE)) == 0)
                {
                    #if DEBUG_VAL_CERT
                    char strIdentSubject[512], strIdentIssuer[512];
                    NetPrintf(("protossl: ca (%s) validated by ca (%s)\n", _DebugFormatCertIdent(&pCACert->pX509Cert->Subject, strIdentSubject, sizeof(strIdentSubject)),
                        _DebugFormatCertIdent(&pCACert->pX509Cert->Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
                    #endif

                    // cert successfully verified
                    DirtyMemFree(pCACert->pX509Cert, PROTOSSL_MEMID, pCACert->iMemGroup, pCACert->pMemGroupUserData);
                    pCACert->pX509Cert = NULL;
                }
                else
                {
                    _SetFailureCertInfo(pState, pCACert->pX509Cert);
                    continue;
                }
            }

            // verified
            break;
        }
        if (pCACert == NULL)
        {
            #if DIRTYCODE_LOGGING
            // output debug logging only if we're manually loading a CA cert
            if (bCertIsCA)
            {
                _DebugPrintCert(pCert, "_VerifyCertificate() -- no CA available for this certificate");
            }
            #endif
            _SetFailureCertInfo(pState, pCert);
            return(-28);
        }
    }
    // success
    return(iResult);
}


/*F***************************************************************************/
/*!
    \Function _AddCertificate

    \Description
        Add a new CA certificate to certificate list

    \Input *pCert       - pointer to new cert to add
    \Input bVerified    - TRUE if verified, else false
    \Input iMemGroup    - memgroup to use for alloc
    \Input *pMemGroupUserData - memgroup userdata for alloc

    \Output
        int32_t         - zero=error/duplicate, one=added

    \Version 01/13/2009 (jbrookes)
*/
/***************************************************************************F*/
static int32_t _AddCertificate(X509CertificateT *pCert, uint8_t bVerified, int32_t iMemGroup, void *pMemGroupUserData)
{
    ProtoSSLCACertT *pCACert;
    int32_t iCertSize = sizeof(*pCACert) + pCert->iKeyModSize;

    // see if this certificate already exists
    if (_CheckDuplicateCA(pCert))
    {
        _DebugPrintCert(pCert, "ignoring redundant add of CA cert");
        return(0);
    }

    // find append point for new CA
    for (pCACert = _ProtoSSL_CACerts; pCACert->pNext != NULL; pCACert = pCACert->pNext)
        ;

    // allocate new record
    if ((pCACert->pNext = (ProtoSSLCACertT *)DirtyMemAlloc(iCertSize, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        _DebugPrintCert(pCert, "failed to allocate memory for cert");
        return(0);
    }

    // clear allocated memory
    pCACert = pCACert->pNext;
    memset(pCACert, 0, iCertSize);

    // if this cert has not already been verified, allocate memory for X509 cert data and copy the X509 cert data for later validation
    if (!bVerified)
    {
        if ((pCACert->pX509Cert = (X509CertificateT *)DirtyMemAlloc(sizeof(*pCert), PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
        {
            _DebugPrintCert(pCert, "failed to allocate memory for X509 cert");
            DirtyMemFree(pCACert->pNext, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);
            pCACert->pNext = NULL;
            return(0);
        }
        // copy cert data
        memcpy(pCACert->pX509Cert, pCert, sizeof(*pCert));
    }

    // copy textual identity of this certificate
    // (don't need to save issuer since we already trust this certificate)
    memcpy(&pCACert->Subject, &pCert->Subject, sizeof(pCACert->Subject));

    // copy exponent data
    pCACert->iKeyExpSize = pCert->iKeyExpSize;
    memcpy(pCACert->KeyExpData, pCert->KeyExpData, pCACert->iKeyExpSize);

    // copy modulus data, immediately following header
    pCACert->iKeyModSize = pCert->iKeyModSize;
    pCACert->pKeyModData = (uint8_t *)pCACert + sizeof(*pCACert);
    memcpy((uint8_t *)pCACert->pKeyModData, pCert->KeyModData, pCACert->iKeyModSize);

    // save memgroup and user info used to allocate
    pCACert->iMemGroup = iMemGroup;

    _DebugPrintCert(pCert, "added new certificate authority");
    return(1);
}

/*F*************************************************************************/
/*!
    \Function _SetCACert

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all ProtoSSL modules for
        the lifetime of the application. This functional can add one or more
        PEM certificates or a single DER certificate.

    \Input *pCertData   - pointer to certificate data (PEM or DER)
    \Input iCertSize    - size of certificate data
    \Input bVerify      - if TRUE verify cert chain on add

    \Output
        int32_t         - negative=error, positive=count of CAs added

    \Notes
        The certificate must be in .DER (binary) or .PEM (base64-encoded)
        format.

    \Version 01/13/2009 (jbrookes)
*/
/**************************************************************************F*/
static int32_t _SetCACert(const uint8_t *pCertData, int32_t iCertSize, uint8_t bVerify)
{
    int32_t iResult;
    int32_t iCount = -1;
    X509CertificateT Cert;
    uint8_t *pCertBuffer = NULL;
    const int32_t _iMaxCertSize = 4096;
    const uint8_t *pCertBeg, *pCertEnd;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    #if DIRTYCODE_LOGGING
    uint32_t uTick = NetTick();
    #endif

    // process PEM signature if present
    if (_FindPEMCertificateData(pCertData, iCertSize, &pCertBeg, &pCertEnd) == 0)
    {
        // no markers -- consume all the data
        pCertBeg = pCertData;
        pCertEnd = pCertData+iCertSize;
    }

    // remember remaining data for possible further parsing
    iCertSize -= pCertEnd-pCertData;
    pCertData = pCertEnd;

    // get memgroup settings for certificate blob
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // is the cert base64 encoded?
    if ((iResult = LobbyBase64Decode(pCertEnd-pCertBeg, (const char *)pCertBeg, NULL)) > 0)
    {
        if (iResult > _iMaxCertSize)
        {
            return(-111);
        }
        // allocate cert buffer
        if ((pCertBuffer = (uint8_t *)DirtyMemAlloc(_iMaxCertSize, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
        {
            return(-112);
        }
        // decode the cert
        LobbyBase64Decode(pCertEnd-pCertBeg, (const char *)pCertBeg, (char *)pCertBuffer);
        pCertBeg = pCertBuffer;
        pCertEnd = pCertBeg+iResult;
    }

    // parse the x.509 certificate onto stack
    if ((iResult = _ParseCertificate(&Cert, pCertBeg, pCertEnd-pCertBeg)) == 0)
    {
        // verify signature of this certificate (self-signed allowed)
        if (!bVerify || ((iResult = _VerifyCertificate(NULL, &Cert, TRUE)) == 0))
        {
            // add certificate to CA list
            iCount = _AddCertificate(&Cert, bVerify, iMemGroup, pMemGroupUserData);
        }
    }

    // if CA was PEM encoded and there is extra data, check for more CAs
    while ((iResult == 0) && (iCertSize > 0) && (_FindPEMCertificateData(pCertData, iCertSize, &pCertBeg, &pCertEnd) != 0))
    {
        // remember remaining data for possible further parsing
        iCertSize -=  pCertEnd-pCertData;
        pCertData = pCertEnd;

        // cert must be base64 encoded
        if (((iResult = LobbyBase64Decode(pCertEnd-pCertBeg, (const char *)pCertBeg, NULL)) <= 0) || (iResult > _iMaxCertSize))
        {
            break;
        }
        LobbyBase64Decode(pCertEnd-pCertBeg, (const char *)pCertBeg, (char *)pCertBuffer);

        // parse the x.509 certificate onto stack
        if ((iResult = _ParseCertificate(&Cert, pCertBuffer, iResult)) < 0)
        {
            continue;
        }

        // verify signature of this certificate (self-signed allowed)
        if (bVerify && ((iResult = _VerifyCertificate(NULL, &Cert, TRUE)) < 0))
        {
            continue;
        }

        // add certificate to CA list
        iCount += _AddCertificate(&Cert, bVerify, iMemGroup, pMemGroupUserData);
    }

    // cleanup temp allocation
    if (pCertBuffer != NULL)
    {
        DirtyMemFree(pCertBuffer, PROTOSSL_MEMID, iMemGroup, pMemGroupUserData);
    }

    NetPrintf(("protossl: SSL Perf (CA load) %dms\n", NetTickDiff(NetTick(), uTick)));

    return(iCount);
}


/*** Public functions ********************************************************/


/*F*************************************************************************************************/
/*!
    \Function ProtoSSLCreate

    \Description
        Allocate an SSL connection and prepare for use

    \Output ProtoSSLRefT * - Reference pointer (must be passed to all other functions) (NULL=failure)

    \Version 1.0 03/08/2002 (GWS) First Version
*/
/*************************************************************************************************F*/
ProtoSSLRefT *ProtoSSLCreate(void)
{
    ProtoSSLRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protossl: could not allocate module state\n"));
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iLastSocketError = SOCKERR_NONE;
    pState->iCARequestId = -1;
    pState->bCARequested = FALSE;

    // all ciphers enabled by default
    pState->uEnabledCiphers = PROTOSSL_CIPHER_ALL;

    // return module state
    return(pState);
}

/*F**************************************************************************/
/*!
    \Function    ProtoSSLReset

    \Description
        Reset connection back to unconnected state (will disconnect from server).

    \Input *pState    - Reference pointer

    \Version 1.0 03/08/2002 (GWS) First Version
*/
/**************************************************************************F*/
void ProtoSSLReset(ProtoSSLRefT *pState)
{
    // reset to unsecure mode
    _ResetState(pState, 0);
}

/*F************************************************************************/
/*!
    \Function ProtoSSLDestroy

    \Description
        Destroy the module and release its state. Will disconnect from the
        server if required.

    \Input *pState - Reference pointer

    \Version 03/08/2002 (GWS)
*/
/*************************************************************************F*/
void ProtoSSLDestroy(ProtoSSLRefT *pState)
{
    // reset to unsecure mode (free all secure resources)
    _ResetState(pState, 0);
    // free remaining state
    DirtyMemFree(pState, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLAccept

    \Description
        Accept an incoming connection.

    \Input *pState   - module state reference
    \Input iSecure   - flag indicating use of secure connection: 1=secure, 0=unsecure
    \Input *pAddr    - where the client's address should be written
    \Input *pAddrlen - the length of the client's address space

    \Output ProtoSSLRefT* - accepted connection or 0 if not available

    \Notes
        iSecure = 1 is not currently supported and will always return 0

    \Version 02/02/2004 (SJB)
*/
/*************************************************************************F*/
ProtoSSLRefT *ProtoSSLAccept(ProtoSSLRefT *pState, int32_t iSecure, struct sockaddr *pAddr, int32_t *pAddrlen)
{
    ProtoSSLRefT *pClient;
    SocketT *pSocket;

    if (iSecure)
    {
        return(NULL);
    }
    pSocket = SocketAccept(pState->pSock, pAddr, pAddrlen);
    if (pSocket == NULL)
    {
        return(NULL);
    }
    DirtyMemGroupEnter(pState->iMemGroup, pState->pMemGroupUserData);
    pClient = ProtoSSLCreate();
    DirtyMemGroupLeave();
    if (pClient == NULL)
    {
        SocketClose(pSocket);
        return(NULL);
    }
    pClient->pSock = pSocket;
    memcpy(&pClient->PeerAddr, pAddr, *pAddrlen);
    pClient->iState = ST_UNSECURE;
    return pClient;
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLBind

    \Description
        Create a socket bound to the given address.

    \Input *pState  - module state reference
    \Input *pAddr   - the IPv4 address
    \Input iAddrlen - the size of the IPv4 address.

    \Output SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/03/2004 (SJB)
*/
/*************************************************************************F*/
int32_t ProtoSSLBind(ProtoSSLRefT *pState, const struct sockaddr *pAddr, int32_t iAddrlen)
{
    int32_t iResult;

    if (pState->pSock != NULL)
    {
        pState->iLastSocketError = SocketInfo(pState->pSock, 'serr', 0, NULL, 0);
        SocketClose(pState->pSock);
    }

    pState->pSock = SocketOpen(AF_INET, SOCK_STREAM, 0);
    if (pState->pSock == NULL)
    {
        return(SOCKERR_OTHER);
    }

    // set recv/send buffer size?
    if (pState->iRecvBufSize != 0)
    {
        SocketControl(pState->pSock, 'rbuf', pState->iRecvBufSize, NULL, NULL);
    }
    if (pState->iSendBufSize != 0)
    {
        SocketControl(pState->pSock, 'sbuf', pState->iSendBufSize, NULL, NULL);
    }

    iResult = SocketBind(pState->pSock, pAddr, iAddrlen);
    return(iResult);
}

/*F**************************************************************************/
/*!
    \Function ProtoSSLConnect

    \Description
        Make a secure connection to a server using SSL 2.0 protocol.

    \Input *pState  - module state reference
    \Input iSecure  - flag indicating use of secure connection (1=secure, 0=unsecure)
    \Input *pAddr   - textual form of address (1.2.3.4 or www.ea.com)
    \Input uAddr    - the IP address of the server (if not in textual form)
    \Input iPort    - the TCP port of the server (if not in textual form)

    \Output SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/08/2002 (GWS)
*/
/***************************************************************************F*/
int32_t ProtoSSLConnect(ProtoSSLRefT *pState, int32_t iSecure, const char *pAddr, uint32_t uAddr, int32_t iPort)
{
    int32_t iIndex;
    int32_t iError;

    // reset connection state
    iError = _ResetState(pState, iSecure);
    if (iError != SOCKERR_NONE)
    {
        return(iError);
    }

    // allocate the socket
    pState->pSock = SocketOpen(AF_INET, SOCK_STREAM, 0);
    if (pState->pSock == NULL)
    {
        return(SOCKERR_NORSRC);
    }

    // set recv/send buffer size?
    if (pState->iRecvBufSize != 0)
    {
        SocketControl(pState->pSock, 'rbuf', pState->iRecvBufSize, NULL, NULL);
    }
    if (pState->iSendBufSize != 0)
    {
        SocketControl(pState->pSock, 'sbuf', pState->iSendBufSize, NULL, NULL);
    }

    // if we're allowing insecure lookups, make the socket allow insecure use
    if (pState->bAllowXenonLookup)
    {
        SocketControl(pState->pSock, 'xins', 1, NULL, NULL);
    }

    // init peer structure
    SockaddrInit(&pState->PeerAddr, AF_INET);

    // clear previous cert info, if any
    pState->bCertInfoSet = FALSE;
    memset(&pState->CertInfo, 0, sizeof(pState->CertInfo));

    // handle default address case
    if (pAddr == NULL)
    {
        pAddr = "";
    }

    // parse the address string
    for (iIndex = 0; (pAddr[iIndex] != 0) && (pAddr[iIndex] != ':') && ((unsigned)iIndex < sizeof(pState->strHost)-1); ++iIndex)
    {
        // copy over to host
        pState->strHost[iIndex] = pAddr[iIndex];
    }
    pState->strHost[iIndex] = 0;

    // attempt to set host address
    SockaddrInSetAddrText(&pState->PeerAddr, pState->strHost);
    if (SockaddrInGetAddr(&pState->PeerAddr) == 0)
    {
        SockaddrInSetAddr(&pState->PeerAddr, uAddr);
    }

    // attempt to set peer address
    if (pAddr[iIndex] == ':')
    {
        SockaddrInSetPort(&pState->PeerAddr, atoi(pAddr+iIndex+1));
    }
    else
    {
        SockaddrInSetPort(&pState->PeerAddr, iPort);
    }

    // see if we need to start DNS request
    if (SockaddrInGetAddr(&pState->PeerAddr) == 0)
    {
        // do dns lookup prior to connect
        pState->pHost = SocketLookup2(pState->strHost, 30*1000, pState->bAllowXenonLookup);
        pState->iState = ST_ADDR;
    }
    else
    {
        // set to connect state
        pState->iState = ST_CONN;
    }

    // return error code
    return(SOCKERR_NONE);
}

/*F**************************************************************************/
/*!
    \Function ProtoSSLListen

    \Description
        Start listening for an incoming connection.

    \Input *pState  - module state reference
    \Input iBacklog - number of pending connections allowed

    \Output SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/03/2004 (SJB)
*/
/**************************************************************************F*/
int32_t ProtoSSLListen(ProtoSSLRefT *pState, int32_t iBacklog)
{
    int32_t iResult;
    iResult = SocketListen(pState->pSock, iBacklog);
    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function ProtoSSLDisconnect

    \Description
        Disconnect from the server

    \Input *pState - module state reference

    \Output SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/08/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t ProtoSSLDisconnect(ProtoSSLRefT *pState)
{
    // reset existing secure/nonsecure state
    _ResetState(pState, (pState->pSecure != 0));
    return(SOCKERR_NONE);
}

/*F*************************************************************************************************/
/*!
    \Function ProtoSSLUpdate

    \Description
        Give time to module to do its thing (should be called
        periodically to allow module to perform work). Calling
        once per 100ms or so should be enough.

        This is actually a collection of state functions plus
        the overall update function. They are kept together for
        ease of reading.

    \Input *pState - module state reference

    \Version 11/10/2005 gschaefer
*/
/*************************************************************************************************F*/

// send the client hello packet to the server
static int32_t _ProtoSSLUpdateSendClientHello(ProtoSSLRefT *pState)
{
    int32_t iResult, iCiphers;
    uint8_t strHead[4];
    uint8_t strBody[256];
    uint8_t *pData = strBody, *pCiphSize;
    SecureStateT *pSecure = pState->pSecure;

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Send Client Hello\n"));
    #endif

    // initialize the ssl performance timer
    pSecure->uTimer = 0;

    // set the version
    *pData++ = (uint8_t)(SSL3_VERSION>>8);
    *pData++ = (uint8_t)(SSL3_VERSION>>0);

    // set random data
    _GenerateRandom(pSecure->ClientRandom, sizeof(pSecure->ClientRandom), &pSecure->ReadArc4);
    memcpy(pData, pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
    pData += 32;

    // add session identifier (we don't support one)
    *pData++ = 0;

    // add the cipher suite list
    *pData++ = 0;
    pCiphSize = pData++; // save and skip cipher size location
    for (iResult = 0, iCiphers = 0; iResult < (signed)(sizeof(_SSL3_CipherSuite)/sizeof(_SSL3_CipherSuite[0])); ++iResult)
    {
        if ((_SSL3_CipherSuite[iResult].uId & pState->uEnabledCiphers) != 0)
        {
            *pData++ = _SSL3_CipherSuite[iResult].uIdent[0];
            *pData++ = _SSL3_CipherSuite[iResult].uIdent[1];
            iCiphers += 1;
        }
    }
    // write cipher suite list size
    *pCiphSize = iCiphers*2;

    // add the compress list (we don't support compression)
    *pData++ = 1;
    *pData++ = 0;

    // setup the header
    strHead[0] = SSL3_MSG_CLIENT_HELLO;
    strHead[1] = 0;
    strHead[2] = (uint8_t)((pData-strBody)>>8);
    strHead[3] = (uint8_t)((pData-strBody)>>0);

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, pData-strBody);
    return(ST3_RECV_HELLO);
}

// process the server hello packet
static int32_t _ProtoSSLUpdateRecvServerHello(ProtoSSLRefT *pState, const uint8_t *pData)
{
    int32_t iIndex;
    SecureStateT *pSecure = pState->pSecure;

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Process Server Hello\n"));
    #endif

    // check the version
    if ((pData[0] != (SSL3_VERSION>>8)) || (pData[1] != (SSL3_VERSION&255)))
    {
        NetPrintf(("protossl: _ServerHello: version %d,%d\n", pData[0], pData[1]));
        return(ST_FAIL_SETUP);
    }
    pData += 2;

    // save server random data
    memcpy(pSecure->ServerRandom, pData, sizeof(pSecure->ServerRandom));
    pData += 32;

    // skip past session id (we don't want it)
    pData += *pData+1;

    // locate the cipher
    pSecure->pCipher = NULL;
    for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_CipherSuite)/sizeof(_SSL3_CipherSuite[0])); ++iIndex)
    {
        if ((pData[0] == _SSL3_CipherSuite[iIndex].uIdent[0]) && (pData[1] == _SSL3_CipherSuite[iIndex].uIdent[1]))
        {
            pSecure->pCipher = _SSL3_CipherSuite+iIndex;
            break;
        }
    }

    if (pSecure->pCipher == NULL)
    {
        NetPrintf(("protossl: no matching cipher (ident=%d,%d)\n", pData[0], pData[1]));
    }
    else
    {
        NetPrintf(("protossl: using cipher suite %s (ident=%d,%d)\n", _SSL3_strCipherSuite[iIndex], pData[0], pData[1]));
    }

    // still in hello state
    return(ST3_RECV_HELLO);
}

// initialize a CA fetch request, 0=success, other=error or not allowed
static int32_t _ProtoSSLUpdateRequestCA(ProtoSSLRefT *pState)
{
    if (!pState->bCARequested && pState->bCertInfoSet && !pState->bAllowAnyCert)
    {
        // we allow only one fetch request for each ssl negotiation attempt
        pState->bCARequested = TRUE;
        if ((pState->iCARequestId = DirtyCertCARequestCert(&pState->CertInfo)) >= 0)
        {
            // save the failure cert, it will be validated again after fetching CA cert
            if (pState->pCertToVal == NULL)
            {
                pState->pCertToVal = DirtyMemAlloc(sizeof(*pState->pCertToVal), PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            }
            // continue to fetch CA cert even if we fail to allocate cert memory
            if (pState->pCertToVal != NULL)
            {
                memcpy(pState->pCertToVal, &pState->pSecure->Cert, sizeof(*pState->pCertToVal));
            }
            return(0);
        }
    }
    return(-1);
}

// process the server certificate
static int32_t _ProtoSSLUpdateRecvServerCert(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iSize1, iSize2, iParse;
    int32_t iResult = ST_FAIL_SETUP; // if cert validated, iResult is ST3_RECV_HELLO
    int32_t iCertHeight = 0; // the height of current cert, the leaf cert's height is 0.
    X509CertificateT leafCert, prevCert;

    // ref: http://tools.ietf.org/html/rfc5246#section-7.4.2

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Process Server Cert\n"));
    #endif

    memset(&leafCert, 0, sizeof(leafCert));
    memset(&prevCert, 0, sizeof(prevCert));

    // get outer size
    iSize1 = (pData[0]<<16)|(pData[1]<<8)|(pData[2]<<0);
    while (iSize1 > 3)
    {
        // get certificate size
        iSize2 = (pData[3]<<16)|(pData[4]<<8)|(pData[5]<<0);

        // at least validate that the two sizes seem to correspond
        if (iSize2 > iSize1-3)
        {
            NetPrintf(("protossl: _ServerCert: certificate is larger than envelope (%d/%d)\n", iSize1, iSize2));
            return(ST_FAIL_SETUP);
        }

        // parse the certificate
        iParse = _ParseCertificate(&pSecure->Cert, pData+6, iSize2);
        if (iParse < 0)
        {
            NetPrintf(("protossl: _ServerCert: x509 cert invalid (error=%d)\n", iParse));
            return(ST_FAIL_CERT_INVALID);
        }

        // if it's the leaf cert
        if (iCertHeight == 0)
        {
            // ensure certificate was issued to this host
            if (_WildcardMatchNoCase(pState->strHost, pSecure->Cert.Subject.strCommon) != 0)
            {
                // check against alternative names, if present
                if (_SubjectAlternativeMatch(pState->strHost, pSecure->Cert.strSubjectAlt) != 0)
                {
                    NetPrintf(("protossl: _ServerCert: certificate not issued to this host (%s != %s)\n", pState->strHost, pSecure->Cert.Subject.strCommon));
                    _DebugPrintCert(&pSecure->Cert, "cert");
                    if (!pState->bAllowAnyCert)
                    {
                        return(ST_FAIL_CERT_HOST);
                    }
                }
            }
        }
        else
        {
            /*
            This is not the leaf cert. We need to do five things here:
                1. do the basic constraints check (CA);
                2. do the basic constraints check (pathLenConstraints);
                3. check if the cert's subject is equal to previous cert's issuer;
                4. check if the keymodsize is the same with previous cert's signature size;
                5. verify previous cert's signature with current cert's key;

                For the cert that is directly certified by root/valid CA, its signature
                is verified in _VerifyCertificate.
            */
            iParse = -1;
            if (pSecure->Cert.iCertIsCA != FALSE) // 1 pass
            {
                if ((pSecure->Cert.iMaxHeight == 0) || (iCertHeight <= pSecure->Cert.iMaxHeight)) // 2 pass
                {
                    if ((_CompareIdent(&pSecure->Cert.Subject, &prevCert.Issuer, prevCert.iCertIsCA) == 0) &&
                        (pSecure->Cert.iKeyModSize == prevCert.iSigSize)) // 3 & 4 pass
                    {
                        // verify prev cert's signature.
                        iParse = _VerifySignature(pState, &prevCert, pSecure->Cert.KeyModData, pSecure->Cert.iKeyModSize,
                                                  pSecure->Cert.KeyExpData, pSecure->Cert.iKeyExpSize); // 5 pass
                    }
                }
            }

            if (iParse != 0)
            {
                NetPrintf(("protossl: _ServerCert: certificate (height=%d) untrusted.\n", iCertHeight));
                _DebugPrintCert(&pSecure->Cert, "cert");
                // skip bAllowAnyCert check because this is not a valid cert chain
                return(ST_FAIL_CERT_NOTRUST);
            }
        }

        // force _VerifyCertificate to update pState->CertInfo if failed to verify the cert
        pState->bCertInfoSet = FALSE;
        // verify the certificate signature to ensure we trust it (no self-signed allowed)
        iParse = _VerifyCertificate(pState, &pSecure->Cert, FALSE);
        // if the cert was validated or we reached the last cert
        if ((iParse == 0) || (iSize1 == (iSize2 + 3)))
        {
            #if DEBUG_VAL_CERT
            if (iParse == 0)
            {
                char strIdentSubject[512], strIdentIssuer[512];
                NetPrintf(("protossl: cert (%s) validated by ca (%s)\n", _DebugFormatCertIdent(&pSecure->Cert.Subject, strIdentSubject, sizeof(strIdentSubject)),
                    _DebugFormatCertIdent(&pSecure->Cert.Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
            }
            else
            {
                _DebugPrintCert(&pSecure->Cert, "_VerifyCertificate() failed -- no CA available for this certificate");
                NetPrintf(("protossl: _ServerCert: x509 cert untrusted (error=%d)\n", iParse));
            }
            #endif
            // if we need to fetch CA
            if ((iParse != 0) && (_ProtoSSLUpdateRequestCA(pState) == 0))
            {
                iResult = ST_WAIT_CA;
            }
            else
            {
                // done with server certs
                iResult = ((iParse==0)||(pState->bAllowAnyCert)) ? ST3_RECV_HELLO : ST_FAIL_CERT_NOTRUST;
            }
            // restore the leaf cert
            if ((iCertHeight != 0) && (iResult != ST_FAIL_CERT_NOTRUST))
            {
                memcpy(&pSecure->Cert, &leafCert, sizeof(pSecure->Cert));
            }
            break;
        }
        else
        {
            // save the leaf cert for final use
            if (iCertHeight++ == 0)
            {
                memcpy(&leafCert, &pSecure->Cert, sizeof(leafCert));
            }
            // save current cert and move to next (the issuer's cert)
            memcpy(&prevCert, &pSecure->Cert, sizeof(prevCert));
            pData += iSize2 + 3;
            iSize1 -= iSize2 + 3;
        }
    }

    // still in hello state (if validated)
    return(iResult);
}

// process the server key exchange (not needed if cert supplied)
static int32_t _ProtoSSLUpdateRecvServerKey(ProtoSSLRefT *pState, const uint8_t *pData)
{
    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Process Server Key\n"));
    #endif

    // ensure using RSA for key exchange
    if (pData[0] != 1)
    {
        NetPrintf(("protossl: _ServerKey: wrong key alg %d\n", pData[0]));
        return(ST_FAIL_SETUP);
    }

    return(ST3_RECV_HELLO);
}

// process the server done message (marks end of server hello sequence)
static int32_t _ProtoSSLUpdateRecvServerDone(ProtoSSLRefT *pState, const uint8_t *pData)
{
    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Process Server Done\n"));
    #endif

    return(ST3_SEND_KEY);
}

// send client key data to server (use public key to encrypt)
static int32_t _ProtoSSLUpdateSendClientKey(ProtoSSLRefT *pState)
{
    int32_t iLoop;
    CryptRSAT RSAContext;
    CryptMD5T MD5Context;
    CryptSha1T SHA1Context;
    uint8_t *pData;
    uint8_t strHead[4];
    uint8_t strBody[256];
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uTick = NetTick();

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Send Client Key\n"));
    #endif

    // build pre-master secret
    _GenerateRandom(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey), &pSecure->ReadArc4);
    pSecure->PreMasterKey[0] = (uint8_t)(SSL3_VERSION>>8);
    pSecure->PreMasterKey[1] = (uint8_t)(SSL3_VERSION>>0);

    // init rsa state
    if (CryptRSAInit(&RSAContext, pSecure->Cert.KeyModData, pSecure->Cert.iKeyModSize,
                     pSecure->Cert.KeyExpData, pSecure->Cert.iKeyExpSize))
    {
        return(ST_FAIL_SETUP);
    }
    CryptRSAInitMaster(&RSAContext, pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
    // encrypt the pre master key
    CryptRSAEncrypt(&RSAContext);

    // update ssl perf timer
    uTick = NetTickDiff(NetTick(), uTick);
    #if DEBUG_ENC_PERF
    NetPrintf(("protossl: SSL Perf (rsa key) %dms\n", uTick));
    #endif
    pSecure->uTimer += uTick;
    uTick = NetTick();

    // build the master secret
    for (iLoop = 0; iLoop < (signed)(sizeof(pSecure->MasterKey)/16); ++iLoop)
    {
        CryptMD5Init(&MD5Context);
        CryptMD5Update(&MD5Context, pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
        CryptSha1Init(&SHA1Context);
        strBody[0] = strBody[1] = strBody[2] = 'A'+iLoop;
        CryptSha1Update(&SHA1Context, strBody, iLoop+1);
        CryptSha1Update(&SHA1Context, pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
        CryptSha1Update(&SHA1Context, pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
        CryptSha1Update(&SHA1Context, pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
        CryptSha1Final(&SHA1Context, strBody, 20);
        CryptMD5Update(&MD5Context, strBody, 20);
        CryptMD5Final(&MD5Context, pSecure->MasterKey+iLoop*16, 16);
    }

    // get rid of premaster secret from memory
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey), "PreMaster");
    NetPrintMem(pSecure->MasterKey, sizeof(pSecure->MasterKey), "Master");
    #endif
    memset(pSecure->PreMasterKey, 0, sizeof(pSecure->PreMasterKey));

    // build the key material
    for (iLoop = 0; iLoop < (signed)(sizeof(pSecure->KeyBlock)/16); ++iLoop)
    {
        CryptMD5Init(&MD5Context);
        CryptMD5Update(&MD5Context, pSecure->MasterKey, sizeof(pSecure->MasterKey));
        CryptSha1Init(&SHA1Context);
        memset(strBody, 'A'+iLoop, iLoop+1);
        CryptSha1Update(&SHA1Context, strBody, iLoop+1);
        CryptSha1Update(&SHA1Context, pSecure->MasterKey, sizeof(pSecure->MasterKey));
        CryptSha1Update(&SHA1Context, pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
        CryptSha1Update(&SHA1Context, pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
        CryptSha1Final(&SHA1Context, strBody, 20);
        CryptMD5Update(&MD5Context, strBody, 20);
        CryptMD5Final(&MD5Context, pSecure->KeyBlock+16*iLoop, 16);

    }
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->KeyBlock, sizeof(pSecure->KeyBlock), "Key Block");
    #endif

    // update ssl perf timer
    uTick = NetTickDiff(NetTick(), uTick);
    #if DEBUG_ENC_PERF
    NetPrintf(("protossl: SSL Perf (md5+sha key) %dms\n", uTick));
    #endif
    pSecure->uTimer += uTick;

    // distribute the key material
    pData = pSecure->KeyBlock;
    pSecure->pClientMAC = pData;
    pData += pSecure->pCipher->uMac;
    pSecure->pServerMAC = pData;
    pData += pSecure->pCipher->uMac;
    pSecure->pClientKey = pData;
    pData += pSecure->pCipher->uLen;
    pSecure->pServerKey = pData;
    pData += pSecure->pCipher->uLen;
    pSecure->pClientInitVec = pData;
    pData += 16;
    pSecure->pServerInitVec = pData;
    pData += 16;

    // setup the header
    strHead[0] = SSL3_MSG_CLIENT_KEY;
    strHead[1] = 0;
    strHead[2] = (uint8_t)(pSecure->Cert.iKeyModSize>>8);
    strHead[3] = (uint8_t)(pSecure->Cert.iKeyModSize>>0);

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, RSAContext.EncryptBlock, pSecure->Cert.iKeyModSize);
    return(ST3_SEND_CHANGE);
}

// send cipher change to server
static int32_t _ProtoSSLUpdateSendChange(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];
    SecureStateT *pSecure = pState->pSecure;

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Send Cipher Change\n"));
    #endif

    // initialize write cipher
    if (pSecure->pCipher->uEnc == SSL3_ENC_RC4)
    {
        CryptArc4Init(&pSecure->WriteArc4, pSecure->pClientKey, pSecure->pCipher->uLen, 1);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
    {
        CryptAesInit(&pSecure->WriteAes, pSecure->pClientKey, pSecure->pCipher->uLen, CRYPTAES_KEYTYPE_ENCRYPT, pSecure->pClientInitVec);
    }

    // mark as cipher change
    strHead[0] = 1;
    _SendPacket(pState, SSL3_REC_CIPHER, strHead, 1, NULL, 0);

    // reset the sequence number
    pSecure->uSendSeqn = 0;
    return(ST3_SEND_FINISH);
}

// send alert to server
static int32_t _ProtoSSLUpdateSendAlert(ProtoSSLRefT *pState, int32_t iLevel, int32_t iDescription)
{
    uint8_t strHead[4];

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Send Alert: level=%d desc=%d\n", iLevel, iDescription));
    #endif

    strHead[0] = iLevel;
    strHead[1] = iDescription;
    _SendPacket(pState, SSL3_REC_ALERT, strHead, 2, NULL, 0);

    return(pState->iState);
}

// send client finished packet to server
static int32_t _ProtoSSLUpdateSendClientFinish(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];
    uint8_t strBody[256];
    CryptMD5T MD5Context;
    CryptSha1T SHAContext;
    uint8_t MacTemp[20];
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uTick = NetTick();

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Send Client Finished\n"));
    #endif

    // setup the finish verification hashes
    memcpy(&MD5Context, &pSecure->HandshakeMD5, sizeof(MD5Context));
    CryptMD5Update(&MD5Context, "CLNT", 4);
    CryptMD5Update(&MD5Context, pSecure->MasterKey, sizeof(pSecure->MasterKey));
    CryptMD5Update(&MD5Context, _SSL3_Pad1, 48);
    CryptMD5Final(&MD5Context, MacTemp, 16);

    CryptMD5Init(&MD5Context);
    CryptMD5Update(&MD5Context, pSecure->MasterKey, sizeof(pSecure->MasterKey));
    CryptMD5Update(&MD5Context, _SSL3_Pad2, 48);
    CryptMD5Update(&MD5Context, MacTemp, 16);
    CryptMD5Final(&MD5Context, strBody+0, 16);
    #if DEBUG_RAW_DATA
    NetPrintMem(strBody+0, 16, "HS finish MD5");
    #endif

    memcpy(&SHAContext, &pSecure->HandshakeSHA, sizeof(SHAContext));
    CryptSha1Update(&SHAContext, (unsigned char *)"CLNT", 4);
    CryptSha1Update(&SHAContext, pSecure->MasterKey, sizeof(pSecure->MasterKey));
    CryptSha1Update(&SHAContext, _SSL3_Pad1, 40);
    CryptSha1Final(&SHAContext, MacTemp, 20);

    CryptSha1Init(&SHAContext);
    CryptSha1Update(&SHAContext, pSecure->MasterKey, sizeof(pSecure->MasterKey));
    CryptSha1Update(&SHAContext, _SSL3_Pad2, 40);
    CryptSha1Update(&SHAContext, MacTemp, 20);
    CryptSha1Final(&SHAContext, strBody+16, 20);
    #if DEBUG_RAW_DATA
    NetPrintMem(strBody+16, 20, "HS finish SHA");
    #endif

    uTick = NetTickDiff(NetTick(), uTick);
    #if DEBUG_ENC_PERF
    NetPrintf(("protossl: SSL Perf (client finish) %dms\n", uTick));
    #endif
    pSecure->uTimer += uTick;

    // setup the header
    strHead[0] = SSL3_MSG_FINISHED;
    strHead[1] = 0;
    strHead[2] = 0;
    strHead[3] = 16+20;

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, 16+20);
    return(ST3_RECV_CHANGE);
}

// process server cipher change request
static int32_t _ProtoSSLUpdateRecvChange(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Process Cipher Change\n"));
    #endif

    // initialize read cipher
    if (pSecure->pCipher->uEnc == SSL3_ENC_RC4)
    {
        CryptArc4Init(&pSecure->ReadArc4, pSecure->pServerKey, pSecure->pCipher->uLen, 1);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
    {
        CryptAesInit(&pSecure->ReadAes, pSecure->pServerKey, pSecure->pCipher->uLen, CRYPTAES_KEYTYPE_DECRYPT, pSecure->pServerInitVec);
    }

    // reset sequence number
    pSecure->uRecvSeqn = 0;

    // just gobble packet -- we assume next state is crypted regardless
    return(ST3_RECV_FINISH);
}

// process server finished packet
static int32_t _ProtoSSLUpdateRecvServerFinish(ProtoSSLRefT *pState, const uint8_t *pData)
{
    CryptMD5T MD5Context;
    CryptSha1T SHAContext;
    uint8_t MacTemp[20];
    uint8_t strMD5[16];
    uint8_t strSHA[20];
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uTick = NetTick();

    #if DEBUG_MSG_LIST
    NetPrintf(("protossl: SSL Msg: Process Client Finish\n"));
    #endif

    // setup the finish verification hashes
    memcpy(&MD5Context, &pSecure->HandshakeMD5, sizeof(MD5Context));
    CryptMD5Update(&MD5Context, "SRVR", 4);
    CryptMD5Update(&MD5Context, pSecure->MasterKey, sizeof(pSecure->MasterKey));
    CryptMD5Update(&MD5Context, _SSL3_Pad1, 48);
    CryptMD5Final(&MD5Context, MacTemp, 16);

    CryptMD5Init(&MD5Context);
    CryptMD5Update(&MD5Context, pSecure->MasterKey, sizeof(pSecure->MasterKey));
    CryptMD5Update(&MD5Context, _SSL3_Pad2, 48);
    CryptMD5Update(&MD5Context, MacTemp, 16);
    CryptMD5Final(&MD5Context, strMD5, 16);

    memcpy(&SHAContext, &pSecure->HandshakeSHA, sizeof(SHAContext));
    CryptSha1Update(&SHAContext, (unsigned char *)"SRVR", 4);
    CryptSha1Update(&SHAContext, pSecure->MasterKey, sizeof(pSecure->MasterKey));
    CryptSha1Update(&SHAContext, _SSL3_Pad1, 40);
    CryptSha1Final(&SHAContext, MacTemp, 20);

    CryptSha1Init(&SHAContext);
    CryptSha1Update(&SHAContext, pSecure->MasterKey, sizeof(pSecure->MasterKey));
    CryptSha1Update(&SHAContext, _SSL3_Pad2, 40);
    CryptSha1Update(&SHAContext, MacTemp, 20);
    CryptSha1Final(&SHAContext, strSHA, 20);

    // verify the headers
    if ((memcmp(strMD5, pData+0, sizeof(strMD5)) != 0) ||
        (memcmp(strSHA, pData+16, sizeof(strSHA)) != 0))
    {
        NetPrintMem(strMD5, sizeof(strMD5), "Server Final MD5 Calc");
        NetPrintMem(pData+0, 16, "Server Final MD5 Recv");
        NetPrintMem(strSHA, sizeof(strSHA), "Server Final SHA Calc");
        NetPrintMem(pData+16, 20, "Server Final SHA Recv");
        return(ST_FAIL_SETUP);
    }

    // finish timing crypto
    uTick = NetTickDiff(NetTick(), uTick);
    #if DEBUG_ENC_PERF
    NetPrintf(("protossl: SSL Perf (server finish) %dms\n", uTick));
    #endif
    pSecure->uTimer += uTick;
    NetPrintf(("protossl: SSL Perf (setup) %dms\n", pSecure->uTimer));

    // change to secure mode
    return(ST3_SECURE);
}

// update CA fetch request status
static int32_t _ProtoSSLUpdateCARequest(ProtoSSLRefT *pState)
{
    int32_t iComplete;
    // see if request completed
    if ((iComplete = DirtyCertCARequestDone(pState->iCARequestId)) != 0)
    {
        DirtyCertCARequestFree(pState->iCARequestId);
        pState->iCARequestId = -1;
        // if CA fetch request failed
        if (iComplete < 0)
        {
            _SetFailureCertInfo(pState, pState->pCertToVal);
            pState->iState = ST_FAIL_CERT_REQUEST;
        }
        // if cert not validated
        else if ((pState->pCertToVal == NULL) || (_VerifyCertificate(pState, pState->pCertToVal, FALSE) != 0))
        {
            _SetFailureCertInfo(pState, pState->pCertToVal);
            pState->iState = ST_FAIL_CERT_NOTRUST;
        }
        else
        {
            // cert validated
            pState->iState = ST3_RECV_HELLO;
        }

        if (pState->pCertToVal != NULL)
        {
            DirtyMemFree(pState->pCertToVal, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pState->pCertToVal = NULL;
        }
    }
    return pState->iState;
}

// the actual update function
void ProtoSSLUpdate(ProtoSSLRefT *pState)
{
    int32_t iXfer;
    int32_t iResult;
    const uint8_t *pData;
    SecureStateT *pSecure = pState->pSecure;

    // resolve the address
    if (pState->iState == ST_ADDR)
    {
        // check for completion
        if (pState->pHost->Done(pState->pHost))
        {
            if (pState->pHost->addr == 0)
            {
                pState->iState = ST_FAIL_DNS;
            }
            else
            {
                pState->iState = ST_CONN;
                SockaddrInSetAddr(&pState->PeerAddr, pState->pHost->addr);
            }
            // free the record
            pState->pHost->Free(pState->pHost);
            pState->pHost = NULL;
        }
    }

    // see if we should start a connection
    if (pState->iState == ST_CONN)
    {
        // start the connection attempt
        iResult = SocketConnect(pState->pSock, &pState->PeerAddr, sizeof pState->PeerAddr);

        if(iResult == SOCKERR_NONE)
        {
            pState->iState = ST_WAIT_CONN;
        }
        else
        {
            pState->iState = ST_FAIL_CONN;
            pState->iClosed = 1;
        }
    }

    // wait for connection
    if (pState->iState == ST_WAIT_CONN)
    {
        iResult = SocketInfo(pState->pSock, 'stat', 0, NULL, 0);
        if (iResult > 0)
        {
            pState->iState = (pSecure ? ST3_SEND_HELLO : ST_UNSECURE);
            pState->iClosed = 0;
        }
        if (iResult < 0)
        {
            pState->iState = ST_FAIL_CONN;
            pState->iClosed = 1;
        }
    }

    // handle secure i/o (non-secure is done in send/receive)
    while ((pState->pSock != NULL) && (pState->iState >= ST3_SEND_HELLO) && (pState->iState <= ST3_SECURE))
    {
        // no data transfer yet
        iXfer = 0;

        // handle send states if output buffer is empty
        if (pSecure->iSendProg == pSecure->iSendSize)
        {
            // deal with send states
            if (pState->iState == ST3_SEND_HELLO)
            {
                pState->iState = _ProtoSSLUpdateSendClientHello(pState);
            }
            else if (pState->iState == ST3_SEND_KEY)
            {
                pState->iState = _ProtoSSLUpdateSendClientKey(pState);
            }
            else if (pState->iState == ST3_SEND_CHANGE)
            {
                pState->iState = _ProtoSSLUpdateSendChange(pState);
            }
            else if (pState->iState == ST3_SEND_FINISH)
            {
                pState->iState = _ProtoSSLUpdateSendClientFinish(pState);
            }
        }

        // see if there is data to send
        if (pSecure->iSendProg < pSecure->iSendSize)
        {
            // try to send
            iResult = SocketSend(pState->pSock, (char *)pSecure->SendData+pSecure->iSendProg, pSecure->iSendSize-pSecure->iSendProg, 0);
            if (iResult > 0)
            {
                pSecure->iSendProg += iResult;
                iXfer = 1;
            }
            // see if the data can be cleared
            if (pSecure->iSendProg == pSecure->iSendSize)
            {
                pSecure->iSendProg = pSecure->iSendSize = 0;
            }
        }

        // try and receive header data
        if (pSecure->iRecvSize < SSL_MIN_PACKET)
        {
            iResult = SocketRecv(pState->pSock, (char *)pSecure->RecvData+pSecure->iRecvSize, SSL_MIN_PACKET-pSecure->iRecvSize, 0);
            if (iResult > 0)
            {
                pSecure->iRecvSize += iResult;
                pSecure->iRecvProg = pSecure->iRecvSize;
            }
            if (iResult < 0)
            {
                if (pState->iState < ST3_SECURE)
                {
                    pState->iState = ST_FAIL_SETUP;
                }
                pState->iClosed = 1;
            }
        }
        // wait for minimum ssl packet before passing this point
        if (pSecure->iRecvSize < SSL_MIN_PACKET)
        {
            break;
        }

        // see if we can determine the full packet size
        if (pSecure->iRecvSize == SSL_MIN_PACKET)
        {
            // validate the version
            if ((pSecure->RecvData[1] != (SSL3_VERSION>>8)) || (pSecure->RecvData[2] != (SSL3_VERSION&255)))
            {
                // proper solution is to turn packet into alert to force shutdown
                // temp hack is to kill length bytes to avoid processing
                pSecure->RecvData[3] = 0;
                pSecure->RecvData[4] = 0;
            }
            // add in the data length
            pSecure->iRecvSize += (pSecure->RecvData[3]<<8)|(pSecure->RecvData[4]<<0);
            // check data length to make sure it is valid
            if (pSecure->iRecvSize > SSL_RCVMAX_PACKET)
            {
                NetPrintf(("protossl: received oversized packet (%d/%d); terminating connection\n", pSecure->iRecvSize, SSL_RCVMAX_PACKET));
                pState->iClosed = 1;
                pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = pSecure->iRecvDone = 0;
                pState->iState = ST_FAIL_SECURE;
            }
        }

        // finish receiving packet
        if (pSecure->iRecvProg < pSecure->iRecvSize)
        {
            iResult = SocketRecv(pState->pSock, (char *)pSecure->RecvData+pSecure->iRecvProg, pSecure->iRecvSize-pSecure->iRecvProg, 0);
            if (iResult > 0)
            {
                pSecure->iRecvProg += iResult;
                iXfer = 1;

                // at end of receive, process the packet
                if (pSecure->iRecvProg == pSecure->iRecvSize)
                {
                    iResult = _RecvPacket(pState);
                }
            }
            if (iResult < 0)
            {
                if (pState->iState < ST3_SECURE)
                {
                    pState->iState = ST_FAIL_SETUP;
                }
                pState->iClosed = 1;
            }
        }

        // if data is pending, process receive states
        if ((pSecure->iRecvProg == pSecure->iRecvSize) && (pSecure->iRecvBase < pSecure->iRecvSize))
        {
            if (pSecure->RecvData[0] == SSL3_REC_ALERT)
            {
                if ((pSecure->RecvData[5] == SSL3_ALERT_LEVEL_WARNING)
                        && (pSecure->RecvData[6] == SSL3_ALERT_DESC_CLOSE_NOTIFY))
                {
                    if (pState->iState < ST3_SECURE)
                    {
                        pState->iState = ST_FAIL_SETUP;
                    }
                    pState->iClosed = 1;
                    pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = pSecure->iRecvDone = 0;
                }
                else
                {
                    _DebugAlert(pState, pSecure->RecvData[5], pSecure->RecvData[6]);
                    pState->iState = ST_FAIL_SETUP;
                }
            }
            else if (pState->iState == ST3_RECV_HELLO)
            {
                if ((pData = _RecvHandshake(pState, SSL3_MSG_SERVER_HELLO)) != NULL)
                {
                    pState->iState = _ProtoSSLUpdateRecvServerHello(pState, pData);
                }
                else if ((pData = _RecvHandshake(pState, SSL3_MSG_CERTIFICATE)) != NULL)
                {
                    pState->iState = _ProtoSSLUpdateRecvServerCert(pState, pData);
                }
                else if ((pData = _RecvHandshake(pState, SSL3_MSG_SERVER_KEY)) != NULL)
                {
                    pState->iState = _ProtoSSLUpdateRecvServerKey(pState, pData);
                }
                else if ((pData = _RecvHandshake(pState, SSL3_MSG_CERT_REQ)) != NULL)
                {
                    pState->iState = _ProtoSSLUpdateSendAlert(
                            pState, SSL3_ALERT_LEVEL_WARNING, SSL3_ALERT_DESC_NO_CERTIFICATE);
                }
                else if ((pData = _RecvHandshake(pState, SSL3_MSG_SERVER_DONE)) != NULL)
                {
                    pState->iState = _ProtoSSLUpdateRecvServerDone(pState, pData);
                }
                else
                {
                    NetPrintf(("protossl: unhandled packet of type %d in state %d\n", pSecure->RecvData[pSecure->iRecvBase], pState->iState));
                }
            }
            else if (pState->iState == ST3_RECV_CHANGE)
            {
                if (pSecure->RecvData[0] == SSL3_REC_CIPHER)
                {
                    pState->iState = _ProtoSSLUpdateRecvChange(pState, pSecure->RecvData+pSecure->iRecvBase);
                    pSecure->iRecvBase = pSecure->iRecvSize;
                }
                else
                {
                    NetPrintf(("protossl: unhandled packet of type %d in state %d\n", pSecure->RecvData[pSecure->iRecvBase], pState->iState));
                }
            }
            else if (pState->iState == ST3_RECV_FINISH)
            {
                if ((pData = _RecvHandshake(pState, SSL3_MSG_FINISHED)) != NULL)
                {
                    pState->iState = _ProtoSSLUpdateRecvServerFinish(pState, pData);
                }
                else
                {
                    NetPrintf(("protossl: unhandled packet of type %d in state %d\n", pSecure->RecvData[pSecure->iRecvBase], pState->iState));
                }
            }

            // see if all data was consumed
            if (pSecure->iRecvBase >= pSecure->iRecvSize)
            {
                pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = pSecure->iRecvDone = 0;
            }
        }

        // break out of loop if no i/o activity
        if (iXfer == 0)
        {
            break;
        }
    }

    // wait for CA cert (it's placed at the last intentionally)
    if (pState->iState == ST_WAIT_CA)
    {
        _ProtoSSLUpdateCARequest(pState);
    }
}

/*F*************************************************************************************************/
/*!
    \Function ProtoSSLSend

    \Description
        Send data to the server over secure TCP connection (actually, whether the
        connection is secure or not is determined by the secure flag passed during
        the SSLConnect call).

    \Input *pState  - module state reference
    \Input *pBuffer - data to send
    \Input iLength  - length of data

    \Output
        int32_t     - negative=error, zero=busy, positive=success

    \Version 06/03/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t ProtoSSLSend(ProtoSSLRefT *pState, const char *pBuffer, int32_t iLength)
{
    int32_t iResult = -1;
    SecureStateT *pSecure = pState->pSecure;

    // allow easy string sends
    if (iLength < 0)
    {
        iLength = (int32_t)strlen(pBuffer);
    }

    // make sure connection established
    if (pState->iState == ST3_SECURE)
    {
        iResult = 0;

        // make sure buffer is empty
        if (pSecure->iSendSize == 0)
        {
            // limit send length
            if (iLength > SSL_SNDLIM_PACKET)
            {
                iLength = SSL_SNDLIM_PACKET;
            }

            // setup packet for send
            if (_SendPacket(pState, SSL3_REC_APPLICATION, NULL, 0, pBuffer, iLength) == 0)
            {
                iResult = iLength;
                // try and send now
                ProtoSSLUpdate(pState);
            }
        }
    }

    // handle unsecure sends
    if (pState->iState == ST_UNSECURE)
    {
        iResult = SocketSend(pState->pSock, pBuffer, iLength, 0);
    }

    // return the result
    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function ProtoSSLRecv

    \Description
        Receive data from the server

    \Input *pState  - module state reference
    \Input *pBuffer - receiver data
    \Input iLength  - maximum buffer length

    \Output
        int32_t     - negative=error, zero=nothing available, positive=bytes received

    \Version 03/08/2002 (GWS)
*/
/*************************************************************************************************F*/
int32_t ProtoSSLRecv(ProtoSSLRefT *pState, char *pBuffer, int32_t iLength)
{
    int32_t iResult = -1;
    SecureStateT *pSecure = pState->pSecure;

    // make sure in right state
    if (pState->iState == ST3_SECURE)
    {
        iResult = 0;

        // check for more data if no packet pending
        if ((pSecure->iRecvProg == 0) || (pSecure->iRecvProg != pSecure->iRecvSize))
        {
            ProtoSSLUpdate(pState);
        }

        // check for end of data
        if (((pSecure->iRecvSize < SSL_MIN_PACKET) || (pSecure->iRecvProg < pSecure->iRecvSize)) &&
            (pState->iClosed))
        {
            iResult = SOCKERR_CLOSED;
        }
        // see if data pending
        else if ((pSecure->iRecvProg == pSecure->iRecvSize) && (pSecure->iRecvBase < pSecure->iRecvSize) &&
            (pSecure->RecvData[0] == SSL3_REC_APPLICATION) && (pSecure->iRecvDone != 0))
        {
            iResult = pSecure->iRecvSize-pSecure->iRecvBase;
            // only return what user can store
            if (iResult > iLength)
            {
                iResult = iLength;
            }
            // return the data
            memcpy(pBuffer, pSecure->RecvData+pSecure->iRecvBase, iResult);
            pSecure->iRecvBase += iResult;

            // see if we can grab a new packet
            if ((pSecure->iRecvBase >= pSecure->iRecvSize) && (pSecure->iRecvDone != 0))
            {
                pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = pSecure->iRecvDone = 0;
            }
        }
    }

    // handle unsecure receive
    if (pState->iState == ST_UNSECURE)
    {
        iResult = SocketRecv(pState->pSock, pBuffer, iLength, 0);
    }

    // terminate buffer if there is room
    if ((iResult > 0) && (iResult < iLength))
    {
        pBuffer[iResult] = 0;
    }

    // return the data size
    return(iResult);
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLStat

    \Description
        Return the current module status (according to selector)

    \Input *pState  - module state reference
    \Input iSelect  - status selector ('conn'=return "am i connected" flag)
    \Input pBuffer  - buffer pointer
    \Input iLength  - buffer size

    \Output
        int32_t     - negative=error, zero=false, positive=true

    \Version 03/08/2002 (GWS)
*/
/**************************************************************************F*/
int32_t ProtoSSLStat(ProtoSSLRefT *pState, int32_t iSelect, void *pBuffer, int32_t iLength)
{
    int32_t iResult = -1;

    // pass-through to SocketInfo(NULL,...)
    if (pState == NULL)
    {
        return(SocketInfo(NULL, iSelect, 0, pBuffer, iLength));
    }

    // return address of peer we are trying to connect to
    if (iSelect == 'addr')
    {
        return(SockaddrInGetAddr(&pState->PeerAddr));
    }
    // return certificate info (valid after 'fail' response)
    if ((iSelect == 'cert') && (pBuffer != NULL) && (iLength == sizeof(pState->CertInfo)))
    {
        memcpy(pBuffer, &pState->CertInfo, sizeof(pState->CertInfo));
        return(0);
    }
    // return if a CA fetch request is in progress
    if (iSelect == 'cfip')
    {
        if (pState->iState == ST_WAIT_CA)
        {
            return(1);
        }
        return(0);
    }
    // return last socket error
    if (iSelect == 'serr')
    {
        // is the socket still valid
        if (pState->pSock)
        {
            // forward to socket layer
            return(SocketInfo(pState->pSock, iSelect, 0, pBuffer, iLength));
        }
        else
        {
            // return last save socket error
            return(pState->iLastSocketError);
        }
    }
    // return socket ref
    if (iSelect == 'sock')
    {
        if ((pBuffer == NULL) || (iLength != sizeof(pState->pSock)))
        {
            return(-1);
        }
        memcpy(pBuffer, &pState->pSock, sizeof(pState->pSock));
        return(0);
    }
    // return failure code
    if (iSelect == 'fail')
    {
        if (pState->iState & ST_FAIL)
        {
            switch(pState->iState)
            {
                case ST_FAIL_DNS:
                    iResult = PROTOSSL_ERROR_DNS;
                    break;
                case ST_FAIL_CONN:
                    iResult = PROTOSSL_ERROR_CONN;
                    break;
                case ST_FAIL_CERT_INVALID:
                    iResult = PROTOSSL_ERROR_CERT_INVALID;
                    break;
                case ST_FAIL_CERT_HOST:
                    iResult = PROTOSSL_ERROR_CERT_HOST;
                    break;
                case ST_FAIL_CERT_NOTRUST:
                    iResult = PROTOSSL_ERROR_CERT_NOTRUST;
                    break;
                case ST_FAIL_CERT_REQUEST:
                    iResult = PROTOSSL_ERROR_CERT_REQUEST;
                    break;
                case ST_FAIL_SETUP:
                    iResult = PROTOSSL_ERROR_SETUP;
                    break;
                case ST_FAIL_SECURE:
                    iResult = PROTOSSL_ERROR_SECURE;
                    break;
                default:
                    iResult = PROTOSSL_ERROR_UNKNOWN;
                    break;
            }
        }
        else
        {
            iResult = 0;
        }
        return(iResult);
    }

    // only pass through if socket is valid
    if (pState->pSock != NULL)
    {
        // special processing for 'stat' selector
        if (iSelect == 'stat')
        {
            // if we're in a failure state, return error
            if (pState->iState >= ST_FAIL)
            {
               return(-1);
            }
            // don't check connected status until we are connected and done with secure negotiation (if secure)
            if (pState->iState < ST3_SECURE)
            {
               return(0);
            }
            // if we're connected (in ST_UNSECURE or ST3_SECURE state) fall through
        }

        // pass through request to the socket module
        iResult = SocketInfo(pState->pSock, iSelect, 0, pBuffer, iLength);
    }
    return(iResult);
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLControl

    \Description
        ProtoSSL control function.  Different selectors control different behaviors.

    \Input *pState  - module state reference
    \Input iSelect  - control selector
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'ciph'      Set enabled/disabled ciphers (PROTOSSL_CIPHER_*)
            'ncrt'      Disable client certificate validation.
            'rbuf'      Set socket recv buffer size (must be called before Connect()).
            'sbuf'      Set socket send buffer size (must be called before Connect()).
            'secu'      Start secure negotiation on an established unsecure connection.
            'xdns'      Enable Xenon secure-mode DNS lookups for this ref.
        \endverbatim

    \Version 01/27/2009 (jbrookes)
*/
/**************************************************************************F*/
int32_t ProtoSSLControl(ProtoSSLRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'ciph')
    {
        NetPrintf(("protossl: enabled ciphers=%d\n", iValue));
        pState->uEnabledCiphers = (uint32_t)iValue;
        return(0);
    }
    if (iSelect == 'ncrt')
    {
        pState->bAllowAnyCert = (uint8_t)iValue;
        return(0);
    }
    if (iSelect == 'rbuf')
    {
        pState->iRecvBufSize = iValue;
        return(0);
    }
    if (iSelect == 'sbuf')
    {
        pState->iSendBufSize = iValue;
        return(0);
    }
    if (iSelect == 'secu')
    {
        if (pState->iState != ST_UNSECURE)
        {
            NetPrintf(("protossl: cannot promote to a secure connection unless connected in unsecure state\n"));
            return(-1);
        }
        _ResetSecureState(pState, 1);
        pState->iState = ST3_SEND_HELLO;
        return(0);
    }
    if (iSelect == 'xdns')
    {
        NetPrintf(("protossl: %s secure mode DNS lookups\n", iValue ? "enabling" : "disabling"));
        pState->bAllowXenonLookup = (uint8_t)iValue;
        return(0);
    }
    // unhandled
    return(-1);
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLSetCACert

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all ProtoSSL instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

    \Input *pCertData   - pointer to certificate data (PEM or DER)
    \Input iCertSize    - size of certificate data

    \Output
        int32_t         - negative=error, positive=count of CAs added

    \Notes
        The certificate must be in .DER (binary) or .PEM (base64-encoded)
        format.

    \Version 01/13/2009 (jbrookes)
*/
/**************************************************************************F*/
int32_t ProtoSSLSetCACert(const uint8_t *pCertData, int32_t iCertSize)
{
    return(_SetCACert(pCertData, iCertSize, TRUE));
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLSetCACert2

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all ProtoSSL instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

        This version of the function does not validate the CA at load time.
        The X509 certificate data will be copied and retained until the CA
        is validated, either by use of ProtoSSLValidateAllCA() or by the CA
        being used to validate a certificate.

    \Input *pCertData   - pointer to certificate data (PEM or DER)
    \Input iCertSize    - size of certificate data

    \Output
        int32_t         - negative=error, positive=count of CAs added

    \Notes
        The certificate must be in .DER (binary) or .PEM (base64-encoded)
        format.

    \Version 04/21/2011 (jbrookes)
*/
/**************************************************************************F*/
int32_t ProtoSSLSetCACert2(const uint8_t *pCertData, int32_t iCertSize)
{
    return(_SetCACert(pCertData, iCertSize, FALSE));
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLValidateAllCA

    \Description
        Validate all CA that have been added but not yet been validated.  Validation
        is a one-time process and disposes of the X509 certificate that is retained
        until validation.

    \Output
        int32_t         - zero on success; else the number of certs that could not be validated

    \Version 04/21/2011 (jbrookes)
*/
/**************************************************************************F*/
int32_t ProtoSSLValidateAllCA(void)
{
    ProtoSSLCACertT *pCACert;
    int32_t iInvalid;

    // validate all installed CA Certs that have not yet been validated
    for (pCACert = _ProtoSSL_CACerts, iInvalid = 0; pCACert != NULL; pCACert = pCACert->pNext)
    {
        // if the CA hasn't been verified already, do that now
        if (pCACert->pX509Cert != NULL)
        {
            if (_VerifyCertificate(NULL, pCACert->pX509Cert, TRUE) == 0)
            {
                #if DEBUG_VAL_CERT
                char strIdentSubject[512], strIdentIssuer[512];
                NetPrintf(("protossl: ca (%s) validated by ca (%s)\n", _DebugFormatCertIdent(&pCACert->pX509Cert->Subject, strIdentSubject, sizeof(strIdentSubject)),
                    _DebugFormatCertIdent(&pCACert->pX509Cert->Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
                #endif

                // cert successfully verified
                DirtyMemFree(pCACert->pX509Cert, PROTOSSL_MEMID, pCACert->iMemGroup, pCACert->pMemGroupUserData);
                pCACert->pX509Cert = NULL;
            }
            else
            {
                _DebugPrintCert(pCACert->pX509Cert, "ca could not be validated");
                iInvalid += 1;
            }
        }
    }
    // return number of certs we could not validate
    return(iInvalid);
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLClrCACerts

    \Description
        Clears all dynamic CA certs from the list.

    \Version 01/14/2009 (jbrookes)
*/
/**************************************************************************F*/
void ProtoSSLClrCACerts(void)
{
    ProtoSSLCACertT *pCACert, *pCACert0=NULL;

    /*
     * This code makes the following assumptions:
     *    1) There is at least one static cert.
     *    2) All static certs come first, followed by all dynamic certs.
     */

    // scan for first dynamic certificate
    for (pCACert = _ProtoSSL_CACerts; (pCACert != NULL) && (pCACert->iMemGroup == 0); )
    {
        pCACert0 = pCACert;
        pCACert = pCACert->pNext;
    }
    // any dynamic certs?
    if ((pCACert != NULL) && (pCACert0 != NULL))
    {
        // null-terminate static list
        pCACert0->pNext = NULL;

        // delete dynamic certs
        for ( ; pCACert != NULL; )
        {
            pCACert0 = pCACert->pNext;
            if (pCACert->pX509Cert != NULL)
            {
                DirtyMemFree(pCACert->pX509Cert, PROTOSSL_MEMID, pCACert->iMemGroup, pCACert->pMemGroupUserData);
            }
            DirtyMemFree(pCACert, PROTOSSL_MEMID, pCACert->iMemGroup, pCACert->pMemGroupUserData);
            pCACert = pCACert0;
        }
    }
}
