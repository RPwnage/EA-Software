/*H**************************************************************************/
/*!

    \File protossl.c

    \Description
        This module is an SSL implementation derived from the from-scratch
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

    \Todo
        Handshake processing functions should be hardened to deal with junk
        or malicious data.

    \Copyright
        Copyright (c) Electronic Arts 2002-2014

    \Version 03/08/2002 (gschaefer) Initial SSL 2.0 implementation
    \Version 03/03/2004 (sbevan)    Added certificate validation
    \Version 11/05/2005 (gschaefer) Rewritten to follow SSL 3.0 specification
    \Version 10/12/2012 (jbrookes)  Updated to support TLS1.0 & TLS1.1
    \Version 10/20/2013 (jbrookes)  Added server handshake, client cert support
    \Version 11/06/2013 (jbrookes)  Updated to support TLS1.2

*/
/***************************************************************************H*/

/*** Include files ***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtycert.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/crypt/cryptdef.h"
#include "DirtySDK/crypt/cryptaes.h"
#include "DirtySDK/crypt/cryptarc4.h"
#include "DirtySDK/crypt/cryptecc.h"
#include "DirtySDK/crypt/cryptgcm.h"
#include "DirtySDK/crypt/crypthash.h"
#include "DirtySDK/crypt/crypthmac.h"
#include "DirtySDK/crypt/cryptmd2.h"
#include "DirtySDK/crypt/cryptmd5.h"
#include "DirtySDK/crypt/cryptrand.h"
#include "DirtySDK/crypt/cryptrsa.h"
#include "DirtySDK/crypt/cryptsha1.h"
#include "DirtySDK/util/base64.h"

#include "DirtySDK/proto/protossl.h"


/*** Defines ***************************************************************************/

#define DEBUG_RAW_DATA  (DIRTYCODE_LOGGING && 0)  // display raw debug data
#define DEBUG_ENC_PERF  (DIRTYCODE_LOGGING && 0)  // show verbose crypto performance
#define DEBUG_MSG_LIST  (DIRTYCODE_LOGGING && 0)  // list the message states
#define DEBUG_ALL_OBJS  (DIRTYCODE_LOGGING && 0)  // display all headers/objects while parsing
#define DEBUG_VAL_CERT  (DIRTYCODE_LOGGING && 0)  // display verbose certificate validation info
#define DEBUG_RES_SESS  (DIRTYCODE_LOGGING && 0)  // display verbose session resume info
#define DEBUG_MOD_PRNT  (DIRTYCODE_LOGGING && 0)  // display public key modulus when parsing certificate (useful when adding new CA)

#define SSL_VERBOSE_DEFAULT         (1)

#define SSL_CACERTFLAG_NONE         (0)
#define SSL_CACERTFLAG_GOSCA        (1)                 //!< identifies a GOS CA, for use on internal (*.ea.com) servers only
#define SSL_CACERTFLAG_CAPROVIDER   (2)                 //!< identifies a CA that can function as a CA provider (used for cert loading)

#define SSL_ERR_GOSCA_INVALIDUSE    (-100)          //!< error indicating invalid use of a GOS CA to access a non ea.com domain
#define SSL_ERR_CERT_INVALIDDATE    (-101)          //!< error indicating a CA certificate is expired
#define SSL_ERR_CAPROVIDER_INVALID  (-102)          //!< attempt to use an invalid CA for CA provider use

#define SSL_MIN_PACKET      5                               // smallest packet (5 bytes framing)
#define SSL_CRYPTO_PAD      2048                            // maximum crypto overhead ref: http://tools.ietf.org/html/rfc5246#section-6.2.3
#define SSL_RAW_PACKET      16384                           // max raw data size ref: http://tools.ietf.org/html/rfc5246#section-6.2.1
#define SSL_RCVMAX_PACKET   (SSL_RAW_PACKET+SSL_CRYPTO_PAD) // max recv packet buffer
#define SSL_SNDMAX_PACKET   (SSL_RAW_PACKET)                // max send packet buffer
#define SSL_SNDOVH_PACKET   (384)                           // reserve space for header/mac in send packet
#define SSL_SNDLIM_PACKET   (SSL_SNDMAX_PACKET-SSL_SNDOVH_PACKET) // max send user payload size
#define SSL_BUFFER_DBGFENCE (8)                             //!< eight byte fence at end of send/recv buffers

#define SSL_SESSHIST_MAX    (32)                            // max number of previous sessions that will be tracked for possible later resumption
#define SSL_SESSID_SIZE     (32)

#define SSL_CERTVALID_MAX     (32)
#define SSL_CERTVALIDTIME_MAX (2*60*60*1000)                // certificate validity cached for a max of two hours

#define SSL_FINGERPRINT_SIZE  (CRYPTSHA1_HASHSIZE)

// min/default/max protocol versions supported
#define SSL3_VERSION_MIN        (PROTOSSL_VERSION_SSLv3)
#define SSL3_VERSION            (PROTOSSL_VERSION_TLS1_2)
#define SSL3_VERSION_MAX        (PROTOSSL_VERSION_TLS1_2)

// internal names for TLS protocol versions
#define SSL3_SSLv3              (PROTOSSL_VERSION_SSLv3)
#define SSL3_TLS1_0             (PROTOSSL_VERSION_TLS1_0)
#define SSL3_TLS1_1             (PROTOSSL_VERSION_TLS1_1)
#define SSL3_TLS1_2             (PROTOSSL_VERSION_TLS1_2)

#define SSL3_REC_CIPHER         20  // cipher change record
#define SSL3_REC_ALERT          21  // alert record
#define SSL3_REC_HANDSHAKE      22  // handshake record
#define SSL3_REC_APPLICATION    23  // application data record

#define SSL3_MSG_HELLO_REQUEST  0   // hello request
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
#define SSL3_MAC_SHA256         32  // sha2-256 (ident is size in bytes)
#define SSL3_MAC_SHA384         48  // sha2-384 (ident is size in bytes)
#define SSL3_MAC_MAXSIZE        (CRYPTHASH_MAXDIGEST) // maximum MAC size

#define SSL3_PRF_NULL           0   // no cipher-suite defined prf
#define SSL3_PRF_SHA256         CRYPTHASH_SHA256
#define SSL3_PRF_SHA384         CRYPTHASH_SHA384

#define SSL3_KEY_NULL           0   // no record key exchange
#define SSL3_KEY_RSA            1   // use rsa
#define SSL3_KEY_ECDHE          2   // use ecdhe

#define SSL3_KEYLEN_128         16  // 128-bit key length in bytes
#define SSL3_KEYLEN_256         32  // 256-bit key length in bytes

#define SSL3_ENC_NULL           0   // no record encryption
#define SSL3_ENC_RC4            1   // use rc4
#define SSL3_ENC_AES            2   // use aes
#define SSL3_ENC_GCM            3   // use gcm

// hash ids used in TLS1.2+ CertificateRequest
#define SSL3_HASHID_MD5         (1)
#define SSL3_HASHID_SHA1        (2)
#define SSL3_HASHID_SHA224      (3)
#define SSL3_HASHID_SHA256      (4)
#define SSL3_HASHID_SHA384      (5)
#define SSL3_HASHID_SHA512      (5)

// signature algorithm ids used in TLS1.2+ CertificateRequest
#define SSL3_SIGALG_RSA         (1)
#define SSL3_SIGALG_DSA         (2)
#define SSL3_SIGALG_ECDSA       (3)

// curve types used in TLS1.2+ ServerKeyExchange
#define SSL3_CURVETYPE_EXPLICIT_PRIME   (1)
#define SSL3_CURVETYPE_EXPLICIT_CHAR    (2)
#define SSL3_CURVETYPE_NAMED_CURVE      (3)

#define SSL3_ALERT_LEVEL_WARNING   1
#define SSL3_ALERT_LEVEL_FATAL     2

#define SSL3_ALERT_DESC_CLOSE_NOTIFY                0
#define SSL3_ALERT_DESC_UNEXPECTED_MESSAGE          10
#define SSL3_ALERT_DESC_BAD_RECORD_MAC              20
#define SSL3_ALERT_DESC_DECRYPTION_FAILED           21  // TLS only; reserved
#define SSL3_ALERT_DESC_RECORD_OVERFLOW             22  // TLS only
#define SSL3_ALERT_DESC_DECOMPRESSION_FAILURE       30
#define SSL3_ALERT_DESC_HANDSHAKE_FAILURE           40
#define SSL3_ALERT_DESC_NO_CERTIFICATE              41  // SSL3 only; reserved
#define SSL3_ALERT_DESC_BAD_CERTFICIATE             42
#define SSL3_ALERT_DESC_UNSUPPORTED_CERTIFICATE     43
#define SSL3_ALERT_DESC_CERTIFICATE_REVOKED         44
#define SSL3_ALERT_DESC_CERTIFICATE_EXPIRED         45
#define SSL3_ALERT_DESC_CERTIFICATE_UNKNOWN         46
#define SSL3_ALERT_DESC_ILLEGAL_PARAMETER           47
// the following alert types are all TLS only
#define SSL3_ALERT_DESC_UNKNOWN_CA                  48
#define SSL3_ALERT_DESC_ACCESS_DENIED               49
#define SSL3_ALERT_DESC_DECODE_ERROR                50
#define SSL3_ALERT_DESC_DECRYPT_ERROR               51
#define SSL3_ALERT_DESC_EXPORT_RESTRICTION          60 // reserved
#define SSL3_ALERT_DESC_PROTOCOL_VERSION            70
#define SSL3_ALERT_DESC_INSUFFICIENT_SECURITY       71
#define SSL3_ALERT_DESC_INTERNAL_ERROR              80
#define SSL3_ALERT_DESC_USER_CANCELLED              90
#define SSL3_ALERT_DESC_NO_RENEGOTIATION            100
#define SSL3_ALERT_DESC_UNSUPPORTED_EXTENSION       110
#define SSL3_ALERT_DESC_CERTIFICATE_UNOBTAINABLE    111
#define SSL3_ALERT_DESC_UNRECOGNIZED_NAME           112
#define SSL3_ALERT_DESC_BAD_CERTIFICATE_STATUS      113
#define SSL3_ALERT_DESC_BAD_CERTIFICATE_HASH        114
#define SSL3_ALERT_DESC_UNKNOWN_PSK_IDENTITY        115
#define SSL3_ALERT_DESC_NO_APPLICATION_PROTOCOL     120

// certificate setup
#define SSL_CERT_X509   1

// Identifier for ALPN (Application Level Protocol Negotiation) extension
#define SSL_ALPN_EXTN           0x0010
#define SSL_ALPN_MAX_PROTOCOLS  4

// identifier for elliptic curves extension and supported curves
#define SSL_ELLIPTIC_CURVES_EXTN        (0x000a)

// identifier for signature algorithms extension
#define SSL_SIGNATURE_ALGORITHMS_EXTN   (0x000d)

#define ST_IDLE                 0
#define ST_ADDR                 1
#define ST_CONN                 2
#define ST_WAIT_CONN            3
#define ST_WAIT_CA              4 // waiting for the CA to be fetched
#define ST3_SEND_HELLO          20
#define ST3_RECV_HELLO          21
#define ST3_SEND_CERT           22
#define ST3_SEND_CERT_REQ       23
#define ST3_SEND_KEY            24
#define ST3_SEND_DONE           25
#define ST3_RECV_KEY            26
#define ST3_SEND_VERIFY         27
#define ST3_SEND_CHANGE         28
#define ST3_SEND_FINISH         29
#define ST3_RECV_CHANGE         30
#define ST3_RECV_FINISH         31
#define ST3_SECURE              32
#define ST_UNSECURE             33
#define ST_FAIL                 0x1000
#define ST_FAIL_DNS             0x1001
#define ST_FAIL_CONN            0x1002
#define ST_FAIL_CONN_SSL2       0x1003
#define ST_FAIL_CONN_NOTSSL     0x1004
#define ST_FAIL_CONN_MINVERS    0x1005
#define ST_FAIL_CONN_MAXVERS    0x1006
#define ST_FAIL_CONN_NOCIPHER   0x1007
#define ST_FAIL_CERT_NONE       0x1008
#define ST_FAIL_CERT_INVALID    0x1009
#define ST_FAIL_CERT_HOST       0x100A
#define ST_FAIL_CERT_NOTRUST    0x100B
#define ST_FAIL_CERT_BADDATE    0x100C
#define ST_FAIL_SETUP           0x100D
#define ST_FAIL_SECURE          0x100E
#define ST_FAIL_CERT_REQUEST    0x100F

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
#define ASN_TYPE_GENERALIZEDTIME 0x18
#define ASN_TYPE_UNICODESTR 0x1e

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
    ASN_OBJ_MD5,
    ASN_OBJ_SHA1,
    ASN_OBJ_SHA256,
    ASN_OBJ_SHA384,
    ASN_OBJ_SHA512,
    ASN_OBJ_RSA_PKCS_KEY,
    ASN_OBJ_RSA_PKCS_MD2,
    ASN_OBJ_RSA_PKCS_MD5,
    ASN_OBJ_RSA_PKCS_SHA1,
    ASN_OBJ_RSA_PKCS_SHA256,
    ASN_OBJ_RSA_PKCS_SHA384,
    ASN_OBJ_RSA_PKCS_SHA512,
    ASN_OBJ_COUNT,
};

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! ASN object table
typedef struct ASNObjectT
{
    int32_t iType;              //!< symbolic type
    int32_t iSize;              //!< size of object
    uint8_t strData[16];        //!< object identifier
} ASNObjectT;

//! note: this layout differs somewhat from x.509, but I think it
//! makes more sense this way
typedef struct X509CertificateT
{
    ProtoSSLCertIdentT Issuer;      //!< certificate was issued by (matches subject in another cert)
    ProtoSSLCertIdentT Subject;     //!< certificate was issued for this site/authority

    char strGoodFrom[32];           //!< good from this date
    char strGoodTill[32];           //!< until this date
    uint64_t uGoodFrom;
    uint64_t uGoodTill;

    const uint8_t *pSubjectAlt;     //!< subject alternative name, if present
    int32_t iSubjectAltLen;         //!< subject alternative length

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
    uint8_t HashData[CRYPTHASH_MAXDIGEST];
} X509CertificateT;

//! private key definition
typedef struct X509PrivateKeyT
{
    CryptBinaryObjT Modulus;            //!< key modulus
    CryptBinaryObjT PublicExponent;     //!< public key exponent
    CryptBinaryObjT PrivateExponent;    //!< private key exponent
    CryptBinaryObjT PrimeP;             //!< prime factor (p) of modulus
    CryptBinaryObjT PrimeQ;             //!< prime factor (q) of modulus
    CryptBinaryObjT ExponentP;          //!< exponent d mod p-1
    CryptBinaryObjT ExponentQ;          //!< exponent d mod q-1
    CryptBinaryObjT Coefficient;        //!< inverse of q mod p
    char strPrivKeyData[4096];          //!< buffer to hold private key data
} X509PrivateKeyT;

//! defines data for formatting/processing Finished packet
struct SSLFinished
{
    char strLabelSSL[5];
    char strLabelTLS[16];
    uint8_t uNextState[2]; // [not resuming, resuming]
};

//! minimal certificate authority data (just enough to validate another certificate)
typedef struct ProtoSSLCACertT
{
    ProtoSSLCertIdentT Subject;     //!< identity of this certificate authority
    uint32_t uFlags;                //!< SSL_CACERTFLAG_*

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
    uint16_t uMinVers;              //!< minimum required SSL version
    uint8_t uKey;                   //!< key exchange algorithm
    uint8_t uLen;                   //!< key length
    uint8_t uEnc;                   //!< encryption algorithm
    uint8_t uMac;                   //!< MAC digest size
    uint8_t uMacType;               //!< MAC digest type (CRYPTHASH_*)
    uint8_t uVecSize;               //!< explicit IV size
    uint8_t uPrfType;               //!< cipher-suite PRF type (TLS1.2+ ciphers only)
    uint16_t uId;                   //!< PROTOSSL_CIPHER_*
    char    strName[66];            //!< cipher name
} CipherSuiteT;

//! elliptic curve used for key exchange
typedef struct EllipticCurveT
{
    uint16_t uIdent;                //!< two-byte identifier
    uint16_t uKeySize;              //!< key size
    char strName[32];               //!< curve name
    uint8_t aPrime[64];             //!< prime
    uint8_t aCoefficientA[64];      //!< coefficient a
    uint8_t aCoefficientB[64];      //!< coefficient b
    uint8_t aGenerator[128];        //!< generator
    uint8_t aOrder[64];             //!< order
} EllipticCurveT;

//! secure session history, used for ssl resume
typedef struct SessionHistoryT
{
    uint32_t uSessionTick;          //!< tick when session was last used
    struct sockaddr SessionAddr;    //!< addr of server session is owned by
    uint8_t MasterSecret[48];       //!< master secret generated for session
    uint8_t SessionId[SSL_SESSID_SIZE]; //!< session id used to identify session
} SessionHistoryT;

//! certificate validation history, used to skip validation of certificates we've already valdiated
typedef struct CertValidHistoryT
{
    uint32_t uCertValidTick;        //!< tick when certificate was validated
    uint32_t uCertSize;             //!< size of certificate data
    uint8_t FingerprintId[SSL_FINGERPRINT_SIZE]; //!< certificate fingerprint
} CertValidHistoryT;

//! alpn extension protocol information
typedef struct AlpnProtocolT
{
    uint8_t uLength;
    uint8_t _pad[3];
    char strName[256];
} AlpnProtocolT;

//! signature algorithm information
typedef struct SignatureAlgorithmT
{
    uint8_t uHash;
    uint8_t uSignature;
} SignatureAlgorithmT;

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
    int32_t iRecvPackSize;          //!< size of current packet being processed

    const CipherSuiteT *pCipher;    //!< selected cipher suite
    const EllipticCurveT *pEllipticCurve; //!< selected elliptic curve
    const SignatureAlgorithmT *pSigAlg; //!< selected signature algorithm

    uint8_t ClientRandom[32];       //!< clients random seed
    uint8_t ServerRandom[32];       //!< servers random seed
    uint8_t SessionId[SSL_SESSID_SIZE]; //!< session id
    uint16_t uSslVersion;           //!< ssl version of connection
    uint16_t uSslClientVersion;     //!< client-requested ssl version (validated in premaster secret)
    uint8_t bSessionResume;         //!< indicates session is resumed during handshaking
    uint8_t bHandshakeHashed;       //!< true=handshake packet has been hashed, else false
    uint8_t bSendSecure;            //!< true=sending secure, else false
    uint8_t bRecvSecure;            //!< true=receiving secure, else false
    uint8_t bDateVerifyFailed;      //!< true if date verification of a cert in chain failed
    uint8_t bRSAContextInitialized;
    uint8_t uCertVerifyHashType;    //!< certificate verify hash type as selected from received CertificateRequest options (TLS1.2 only)
    uint8_t bEccContextInitialized; //!< true if we have initialized the EccContext
    uint8_t _pad[3];
    uint8_t uPubKeyLength;          //!< length of pubkey used for ECDHE key exchange
    uint8_t PubKey[256];            //!< pubkey used for ECDHE key exchange

    uint8_t PreMasterKey[48];       //!< pre-master-key
    uint8_t MasterKey[48];          //!< master key

    CryptRSAT RSAContext;

    uint8_t KeyBlock[192];          //!< key block
    uint8_t *pServerMAC;            //!< server mac secret
    uint8_t *pClientMAC;            //!< client mac secret
    uint8_t *pServerKey;            //!< server key secret
    uint8_t *pClientKey;            //!< client key secret
    uint8_t *pServerInitVec;        //!< init vector (CBC ciphers)
    uint8_t *pClientInitVec;        //!< init vector (CBC ciphers)

    CryptMD5T HandshakeMD5;         //!< MD5 of all handshake data
    CryptSha1T HandshakeSHA;        //!< SHA of all handshake data
    CryptSha2T HandshakeSHA256;     //!< SHA256 of all handshake data (TLS1.2+)
    CryptSha2T HandshakeSHA384;     //!< SHA384 of all handshake data (TLS1.2 AES-GCM-256 cipher suite)

    CryptArc4T ReadArc4;            //!< arc4 read cipher state
    CryptArc4T WriteArc4;           //!< arc4 write cipher state

    CryptAesT ReadAes;              //!< aes read cipher state
    CryptAesT WriteAes;             //!< aes write cipher state

    CryptGcmT ReadGcm;              //!< gcm read cipher state
    CryptGcmT WriteGcm;             //!< gcm write cipher state

    X509CertificateT Cert;          //!< the x509 certificate

    CryptEccT EccContext;           //!< elliptic curve state

    char strAlpnProtocol[256];      //!< protocol negotiated using the alpn extension

    uint8_t SendData[SSL_SNDMAX_PACKET+SSL_BUFFER_DBGFENCE];   //!< put at end to make references efficient, include space for debug fence
    uint8_t RecvData[SSL_RCVMAX_PACKET+SSL_BUFFER_DBGFENCE];   //!< put at end to make references efficient, include space for debug fence
} SecureStateT;

//! module state
struct ProtoSSLRefT
{
    SocketT *pSock;                 //!< comm socket
    HostentT *pHost;                //!< host entry

    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    NetCritT SecureCrit;            //!< for guarding multithreaded access to secure state

    char strHost[256];              //!< host that we connect to.
    struct sockaddr PeerAddr;       //!< peer info

    int32_t iState;                 //!< protocol state
    int32_t iClosed;                //!< socket closed flag
    SecureStateT *pSecure;          //!< secure state reference
    X509CertificateT *pCertToVal;   //!< server cert to be validated (used in ST_WAIT_CA state)
    ProtoSSLCertInfoT CertInfo;     //!< certificate info (used on failure)
    char *pCertificate;             //!< certificate
    int32_t iCertificateLen;        //!< certificate length
    char *pPrivateKey;              //!< private key
    int32_t iPrivateKeyKen;         //!< private key length

    uint32_t uEnabledCiphers;       //!< enabled ciphers
    int32_t iRecvBufSize;           //!< TCP recv buffer size; 0=default
    int32_t iSendBufSize;           //!< TCP send buffer size; 0=default
    int32_t iLastSocketError;       //!< Last socket error before closing the socket
    int32_t iCARequestId;           //!< CA request id (valid if positive)

    int32_t iMaxSendRate;           //!< max send rate (0=uncapped)
    int32_t iMaxRecvRate;           //!< max recv rate (0=uncapped)

    uint16_t uSslVersion;           //!< ssl version application wants us to use
    uint16_t uSslVersionMin;        //!< minimum ssl version application will accept
    uint8_t bAllowAnyCert;          //!< bypass certificate validation
    uint8_t bSessionResumeEnabled;  //!< TRUE if session resume is enabled (default), else FALSE
    uint8_t bCertInfoSet;           //!< TRUE if cert info has been set, else FALSE
    uint8_t bServer;                //!< TRUE=server, FALSE=client
    uint8_t bReuseAddr;             //!< if TRUE set SO_REUSEADDR
    int8_t  iClientCertLevel;       //!< 0=no client cert required; 1=client cert requested, 2=client cert required
    uint8_t bCertSent;              //!< TRUE if a certificate was sent in handshaking, else FALSE
    uint8_t bCertRecv;              //!< TRUE if a certificate was received in handshaking, else FALSE
    int8_t  iVerbose;               //!< spam level
    uint8_t uAlertLevel;            //!< most recent alert level
    uint8_t uAlertValue;            //!< most recent alert value
    uint8_t bAlertSent;             //!< TRUE if most recent alert was sent, else FALSE if it was received
    uint8_t uHelloExtn;             //!< enabled ClientHello extensions
    uint8_t _pad;
    uint8_t bNoDelay;               //!< enable TCP_NODELAY on the ssl socket
    uint8_t bKeepAlive;             //!< tcp keep-alive override; 0=default
    uint32_t uKeepAliveTime;        //!< tcp keep-alive time; 0=default

    /* for alpn extension;
       on the client: these are the protocol preferences
       on the server: these are the protocols it supports */
    AlpnProtocolT aAlpnProtocols[SSL_ALPN_MAX_PROTOCOLS];
    uint16_t uNumAlpnProtocols;      //!< the number of alpn protocols in the list
    uint16_t uAlpnExtensionLength;   //!< the size of the list we encode in ClientHello (we calculate when we build the list)
};

//! global state
typedef struct ProtoSSLStateT
{
    //! critical section for locking access to state memory
    NetCritT StateCrit;

    //! previous session info, used for secure session share/resume
    SessionHistoryT SessionHistory[SSL_SESSHIST_MAX];

    //! validated certificate info
    CertValidHistoryT CertValidHistory[SSL_CERTVALID_MAX];

    int32_t iMemGroup;
    void *pMemGroupUserData;

}ProtoSSLStateT;

/*** Function Prototypes ***************************************************************/

// issue a CA request
static int32_t _ProtoSSLInitiateCARequest(ProtoSSLRefT *pState);

/*** Variables *************************************************************************/

static ProtoSSLStateT *_ProtoSSL_pState = NULL;

// SSL3 pad1 sequence
static const uint8_t _SSL3_Pad1[48] =
{
    0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
    0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,
    0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36,0x36
};

// SSL3 pad2 sequence
static const uint8_t _SSL3_Pad2[48] =
{
    0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
    0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,
    0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c,0x5c
};

//! supported ssl cipher suites; see http://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-4
static const CipherSuiteT _SSL3_CipherSuite[] =
{
    { { 0,    0x9d }, SSL3_TLS1_2, SSL3_KEY_RSA,   SSL3_KEYLEN_256, SSL3_ENC_GCM, SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA384, PROTOSSL_CIPHER_RSA_WITH_AES_256_GCM_SHA384,       "TLS_RSA_WITH_AES_256_GCM_SHA384" },  // suite 157: rsa+gcm+sha384
    { { 0,    0x9c }, SSL3_TLS1_2, SSL3_KEY_RSA,   SSL3_KEYLEN_128, SSL3_ENC_GCM, SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_128_GCM_SHA256,       "TLS_RSA_WITH_AES_128_GCM_SHA256" },  // suite 156: rsa+gcm+sha256
    { { 0,    0x3d }, SSL3_TLS1_2, SSL3_KEY_RSA,   SSL3_KEYLEN_256, SSL3_ENC_AES, SSL3_MAC_SHA256, CRYPTHASH_SHA256, 16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA256,       "TLS_RSA_WITH_AES_256_CBC_SHA256" },  // suite 61: rsa+aes+sha256
    { { 0,    0x3c }, SSL3_TLS1_2, SSL3_KEY_RSA,   SSL3_KEYLEN_128, SSL3_ENC_AES, SSL3_MAC_SHA256, CRYPTHASH_SHA256, 16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA256,       "TLS_RSA_WITH_AES_128_CBC_SHA256" },  // suite 60: rsa+aes+sha256
    { { 0xc0, 0x30 }, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_256, SSL3_ENC_GCM, SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA384, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_GCM_SHA384, "TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384" }, // ecdhe+rsa+gcm+sha384
    { { 0xc0, 0x2f }, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_128, SSL3_ENC_GCM, SSL3_MAC_NULL,   CRYPTHASH_NULL,    4, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_GCM_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256" }, // ecdhe+rsa+gcm+sha256
    { { 0xc0, 0x28 }, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_256, SSL3_ENC_AES, SSL3_MAC_SHA384, CRYPTHASH_SHA384, 16, SSL3_PRF_SHA384, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA384, "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384" }, // ecdhe+rsa+aes+sha384
    { { 0xc0, 0x27 }, SSL3_TLS1_2, SSL3_KEY_ECDHE, SSL3_KEYLEN_128, SSL3_ENC_AES, SSL3_MAC_SHA256, CRYPTHASH_SHA256, 16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA256, "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256" }, // ecdhe+rsa+aes+sha256
    { { 0xc0, 0x14 }, SSL3_SSLv3,  SSL3_KEY_ECDHE, SSL3_KEYLEN_256, SSL3_ENC_AES, SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_256_CBC_SHA,    "TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA" },    // ecdhe+rsa+aes+sha
    { { 0xc0, 0x13 }, SSL3_SSLv3,  SSL3_KEY_ECDHE, SSL3_KEYLEN_128, SSL3_ENC_AES, SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_ECDHE_RSA_WITH_AES_128_CBC_SHA,    "TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA" },    // ecdhe+rsa+aes+sha
    { { 0,    0x35 }, SSL3_SSLv3,  SSL3_KEY_RSA,   SSL3_KEYLEN_256, SSL3_ENC_AES, SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_256_CBC_SHA,          "TLS_RSA_WITH_AES_256_CBC_SHA" },     // suite 53: rsa+aes+sha
    { { 0,    0x2f }, SSL3_SSLv3,  SSL3_KEY_RSA,   SSL3_KEYLEN_128, SSL3_ENC_AES, SSL3_MAC_SHA,    CRYPTHASH_SHA1,   16, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_AES_128_CBC_SHA,          "TLS_RSA_WITH_AES_128_CBC_SHA" },     // suite 47: rsa+aes+sha
    { { 0,    0x05 }, SSL3_SSLv3,  SSL3_KEY_RSA,   SSL3_KEYLEN_128, SSL3_ENC_RC4, SSL3_MAC_SHA,    CRYPTHASH_SHA1,    0, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_RC4_128_SHA,              "TLS_RSA_WITH_RC4_128_SHA" },         // suite  5: rsa+rc4+sha
    { { 0,    0x04 }, SSL3_SSLv3,  SSL3_KEY_RSA,   SSL3_KEYLEN_128, SSL3_ENC_RC4, SSL3_MAC_MD5,    CRYPTHASH_MD5,     0, SSL3_PRF_SHA256, PROTOSSL_CIPHER_RSA_WITH_RC4_128_MD5,              "TLS_RSA_WITH_RC4_128_MD5" }          // suite  4: rsa+rc4+md5
};

//! supported elliptic curves; see http://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-8
static const EllipticCurveT _SSL3_EllipticCurves[] =
{
    {
        0x0017, 32, "SECP256R1", // curve 23: P-256
        { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC },
        { 0x5A, 0xC6, 0x35, 0xD8, 0xAA, 0x3A, 0x93, 0xE7, 0xB3, 0xEB, 0xBD, 0x55, 0x76, 0x98, 0x86, 0xBC, 0x65, 0x1D, 0x06, 0xB0, 0xCC, 0x53, 0xB0, 0xF6, 0x3B, 0xCE, 0x3C, 0x3E, 0x27, 0xD2, 0x60, 0x4B },
        { 0x04, 0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47, 0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2, 0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0, 0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96,
          0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B, 0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16, 0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE, 0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5 },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84, 0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51 }
    },
    {
        0x0018, 48, "SECP384R1", // curve 24: P-384
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
          0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE,
          0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFC },
        { 0xB3, 0x31, 0x2F, 0xA7, 0xE2, 0x3E, 0xE7, 0xE4, 0x98, 0x8E, 0x05, 0x6B, 0xE3, 0xF8, 0x2D, 0x19, 0x18, 0x1D, 0x9C, 0x6E, 0xFE, 0x81, 0x41, 0x12, 0x03, 0x14, 0x08, 0x8F, 0x50, 0x13, 0x87, 0x5A,
          0xC6, 0x56, 0x39, 0x8D, 0x8A, 0x2E, 0xD1, 0x9D, 0x2A, 0x85, 0xC8, 0xED, 0xD3, 0xEC, 0x2A, 0xEF },
        { 0x04, 0xAA, 0x87, 0xCA, 0x22, 0xBE, 0x8B, 0x05, 0x37, 0x8E, 0xB1, 0xC7, 0x1E, 0xF3, 0x20, 0xAD, 0x74, 0x6E, 0x1D, 0x3B, 0x62, 0x8B, 0xA7, 0x9B, 0x98, 0x59, 0xF7, 0x41, 0xE0, 0x82, 0x54, 0x2A, 0x38,
          0x55, 0x02, 0xF2, 0x5D, 0xBF, 0x55, 0x29, 0x6C, 0x3A, 0x54, 0x5E, 0x38, 0x72, 0x76, 0x0A, 0xB7, 0x36, 0x17, 0xDE, 0x4A, 0x96, 0x26, 0x2C, 0x6F, 0x5D, 0x9E, 0x98, 0xBF, 0x92, 0x92, 0xDC, 0x29,
          0xF8, 0xF4, 0x1D, 0xBD, 0x28, 0x9A, 0x14, 0x7C, 0xE9, 0xDA, 0x31, 0x13, 0xB5, 0xF0, 0xB8, 0xC0, 0x0A, 0x60, 0xB1, 0xCE, 0x1D, 0x7E, 0x81, 0x9D, 0x7A, 0x43, 0x1D, 0x7C, 0x90, 0xEA, 0x0E, 0x5F },
        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC7, 0x63, 0x4D, 0x81, 0xF4, 0x37, 0x2D, 0xDF,
          0x58, 0x1A, 0x0D, 0xB2, 0x48, 0xB0, 0xA7, 0x7A, 0xEC, 0xEC, 0x19, 0x6A, 0xCC, 0xC5, 0x29, 0x73 }
    }
};

//! supported signature algorithms
static const SignatureAlgorithmT _SSL3_SignatureAlgorithms[] =
{
    { SSL3_HASHID_SHA256,   SSL3_SIGALG_RSA },
    { SSL3_HASHID_SHA1,     SSL3_SIGALG_RSA },
    { SSL3_HASHID_MD5,      SSL3_SIGALG_RSA }
};

//! ssl version name table
static const char *_SSL3_strVersionNames[] =
{
    "SSLv3",
    "TLSv1",
    "TLSv1.1",
    "TLSv1.2"
};

#if DIRTYCODE_LOGGING
#if DEBUG_ALL_OBJS
static const char *_SSL3_strAsnTypes[] =
{
    "ASN_00",
    "ASN_TYPE_BOOLEAN",         // 0x01
    "ASN_TYPE_INTEGER",         // 0x02
    "ASN_TYPE_BITSTRING",       // 0x03
    "ASN_TYPE_OCTSTRING",       // 0x04
    "ASN_TYPE_NULL",            // 0x05
    "ASN_TYPE_OBJECT",          // 0x06
    "ASN_07", "ASN_08", "ASN_09",
    "ASN_0A", "ASN_0B",
    "ASN_TYPE_UTF8STR",         // 0x0c
    "ASN_0D", "ASN_0E", "ASN_0F",
    "ASN_TYPE_SEQN",            // 0x10
    "ASN_TYPE_SET",             // 0x11
    "ASN_12",                   // 0x12
    "ASN_TYPE_PRINTSTR",        // 0x13
    "ASN_TYPE_T61",             // 0x14 (Teletex string)
    "ASN_15",
    "ASN_TYPE_IA5",             // 0x16 (IA5 string)
    "ASN_TYPE_UTCTIME",         // 0x17
    "ASN_TYPE_GENERALIZEDTIME", // 0x18
    "ASN_19", "ASN_1A", "ASN_1B",
    "ASN_1C", "ASN_1D",
    "ASN_TYPE_UNICODESTR",      // 0x1e
    "ASN_1F",
};
static const char *_SSL3_strAsnObjs[ASN_OBJ_COUNT] =
{
    "ASN_OBJ_NONE",
    "ASN_OBJ_COUNTRY",
    "ASN_OBJ_STATE",
    "ASN_OBJ_CITY",
    "ASN_OBJ_ORGANIZATION",
    "ASN_OBJ_UNIT",
    "ASN_OBJ_COMMON",
    "ASN_OBJ_SUBJECT_ALT",
    "ASN_OBJ_BASIC_CONSTRAINTS",
    "ASN_OBJ_MD5",
    "ASN_OBJ_SHA1",
    "ASN_OBJ_SHA256",
    "ASN_OBJ_SHA384",
    "ASN_OBJ_SHA512",
    "ASN_OBJ_RSA_PKCS_KEY",
    "ASN_OBJ_RSA_PKCS_MD2",
    "ASN_OBJ_RSA_PKCS_MD5",
    "ASN_OBJ_RSA_PKCS_SHA1",
    "ASN_OBJ_RSA_PKCS_SHA256",
    "ASN_OBJ_RSA_PKCS_SHA384",
    "ASN_OBJ_RSA_PKCS_SHA512"
};
#endif // DEBUG_ALL_OBJS

// signature types - subtract type from ASN_OBJ_RSA_PKCS_MD2 to get offset for this table
static const char *_SSL3_strSignatureTypes[] =
{
    "md2WithRSAEncryption",
    "md5WithRSAEncryption",
    "sha1WithRSAEncryption",
    "sha256WithRSAEncryption",
    "sha384WithRSAEncryption",
    "sha512WithRSAEncryption"
};

#endif // DIRTYCODE_LOGGING

//! alert description table; see http://tools.ietf.org/html/rfc5246#section-7.2 for definitions and usage information
static const ProtoSSLAlertDescT _ProtoSSL_AlertList[] =
{
    { SSL3_ALERT_DESC_CLOSE_NOTIFY,               "close notify"              },    // 0
    { SSL3_ALERT_DESC_UNEXPECTED_MESSAGE,         "unexpected message"        },    // 10
    { SSL3_ALERT_DESC_BAD_RECORD_MAC,             "bad record mac"            },    // 20
    { SSL3_ALERT_DESC_DECRYPTION_FAILED,          "decryption failed"         },    // 21 - TLS only; reserved, must not be sent
    { SSL3_ALERT_DESC_RECORD_OVERFLOW,            "record overflow"           },    // 22 - TLS only
    { SSL3_ALERT_DESC_DECOMPRESSION_FAILURE,      "decompression failure"     },    // 30
    { SSL3_ALERT_DESC_HANDSHAKE_FAILURE,          "handshake failure"         },    // 40
    { SSL3_ALERT_DESC_NO_CERTIFICATE,             "no certificate"            },    // 41 - SSL3 only; reserved, must not be sent
    { SSL3_ALERT_DESC_BAD_CERTFICIATE,            "bad certificate"           },    // 42
    { SSL3_ALERT_DESC_UNSUPPORTED_CERTIFICATE,    "unsupported certificate"   },    // 43
    { SSL3_ALERT_DESC_CERTIFICATE_REVOKED,        "certificate revoked"       },    // 44
    { SSL3_ALERT_DESC_CERTIFICATE_EXPIRED,        "certificate expired"       },    // 45
    { SSL3_ALERT_DESC_CERTIFICATE_UNKNOWN,        "certificate unknown"       },    // 46
    { SSL3_ALERT_DESC_ILLEGAL_PARAMETER,          "illegal parameter"         },    // 47
    // the following alert types are all TLS only
    { SSL3_ALERT_DESC_UNKNOWN_CA,                 "unknown ca"                },    // 48
    { SSL3_ALERT_DESC_ACCESS_DENIED,              "access denied"             },    // 49
    { SSL3_ALERT_DESC_DECODE_ERROR,               "decode error"              },    // 50
    { SSL3_ALERT_DESC_DECRYPT_ERROR,              "decrypt error"             },    // 51
    { SSL3_ALERT_DESC_EXPORT_RESTRICTION,         "export restriction"        },    // 60 // reserved, must not be sent
    { SSL3_ALERT_DESC_PROTOCOL_VERSION,           "protocol version"          },    // 70
    { SSL3_ALERT_DESC_INSUFFICIENT_SECURITY,      "insufficient security"     },    // 71
    { SSL3_ALERT_DESC_INTERNAL_ERROR,             "internal error"            },    // 80
    { SSL3_ALERT_DESC_USER_CANCELLED,             "user cancelled"            },    // 90
    { SSL3_ALERT_DESC_NO_RENEGOTIATION,           "no renegotiation"          },    // 100
    { SSL3_ALERT_DESC_UNSUPPORTED_EXTENSION,      "unsupported extension"     },    // 110
    // alert extensions; see http://tools.ietf.org/html/rfc6066#section-9
    { SSL3_ALERT_DESC_CERTIFICATE_UNOBTAINABLE,   "certificate unobtainable"  },    // 111
    { SSL3_ALERT_DESC_UNRECOGNIZED_NAME,          "unrecognized name"         },    // 112
    { SSL3_ALERT_DESC_BAD_CERTIFICATE_STATUS,     "bad certificate status"    },    // 113
    { SSL3_ALERT_DESC_BAD_CERTIFICATE_HASH,       "bad certificate hash"      },    // 114
    // alert extension; see http://tools.ietf.org/search/rfc4279#section-6
    { SSL3_ALERT_DESC_UNKNOWN_PSK_IDENTITY,       "unknown psk identify"      },    // 115
    // alert extension: see http://tools.ietf.org/html/rfc7301#section-3.2
    { SSL3_ALERT_DESC_NO_APPLICATION_PROTOCOL,    "no application protocol"   },    // 120
    { -1, NULL                                                                },    // list terminator
};

// ASN object identification table
static const struct ASNObjectT _SSL_ObjectList[] =
{
    { ASN_OBJ_COUNTRY, 3, { 0x55, 0x04, 0x06 } },
    { ASN_OBJ_CITY, 3, { 0x55, 0x04, 0x07 } },
    { ASN_OBJ_STATE, 3, { 0x55, 0x04, 0x08 } },
    { ASN_OBJ_ORGANIZATION, 3, { 0x55, 0x04, 0x0a } },
    { ASN_OBJ_UNIT, 3, { 0x55, 0x04, 0x0b } },
    { ASN_OBJ_COMMON, 3, { 0x55, 0x04, 0x03 } },
    { ASN_OBJ_SUBJECT_ALT, 3, { 0x55, 0x1d, 0x11 } },
    { ASN_OBJ_BASIC_CONSTRAINTS, 3, { 0x55, 0x1d, 0x13 } },

    // OBJ_md5 - OID 1.2.840.113549.2.5
    { ASN_OBJ_MD5, 8, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x02, 0x05 } },
    // OBJ_sha1 - OID 1.3.14.3.2.26
    { ASN_OBJ_SHA1, 5, { 0x2b, 0x0e, 0x03, 0x02, 0x1a } },
    // OBJ_sha256 - OID 2.16.840.1.101.3.4.2.1
    { ASN_OBJ_SHA256, 9, { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01 } },
    // OBJ_sha384 - OID 2.16.840.1.101.3.4.2.2
    { ASN_OBJ_SHA384, 9, { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02 } },
    // OBJ_sha512 - OID 2.16.840.1.101.3.4.2.3
    { ASN_OBJ_SHA512, 9, { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03 } },

    // RSA (PKCS #1 v1.5) key transport algorithm, OID 1.2.840.113349.1.1.1
    { ASN_OBJ_RSA_PKCS_KEY, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 } },
    // RSA (PKCS #1 v1.5) with MD2 signature, OID 1.2.840.113549.1.1.2
    { ASN_OBJ_RSA_PKCS_MD2, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x02 } },
    // RSA (PKCS #1 v1.5) with MD5 signature, OID 1.2.840.113549.1.1.4
    { ASN_OBJ_RSA_PKCS_MD5, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04 } },
    // RSA (PKCS #1 v1.5) with SHA-1 signature; sha1withRSAEncryption OID 1.2.840.113549.1.1.5
    { ASN_OBJ_RSA_PKCS_SHA1, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05 } },
    /* the following are obsolete alternate definitions of sha1withRSAEncryption; we define them
       here for compatibility, because some certificates are still generated with these ids
       (by makecert.exe, included with WindowsSDK, for example) */
    // RSA (PKCS #1 v1.5) with SHA-1 signature; sha-1WithRSAEncryption (obsolete) OID 1.3.14.3.2.29
    { ASN_OBJ_RSA_PKCS_SHA1, 5, { 0x2b, 0x0e, 0x03, 0x02, 0x1d } },
    // RSA (PKCS #1 v1.5) with SHA-1 signature; rsaSignatureWithsha1 (obsolete) OID 1.3.36.3.3.1.2
    { ASN_OBJ_RSA_PKCS_SHA1, 5, { 0x2b, 0x24, 0x03, 0x03, 0x01, 0x02 } },
    /* sha2+rsa combinations */
    // RSA (PKCS #1 v1.5) with SHA-256 signature; sha256withRSAEncryption OID 1.2.840.113549.1.1.11
    { ASN_OBJ_RSA_PKCS_SHA256, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b } },
    // RSA (PKCS #1 v1.5) with SHA-384 signature; sha384withRSAEncryption OID 1.2.840.113549.1.1.12
    { ASN_OBJ_RSA_PKCS_SHA384, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0c } },
    // RSA (PKCS #1 v1.5) with SHA-512 signature; sha512withRSAEncryption OID 1.2.840.113549.1.1.13
    { ASN_OBJ_RSA_PKCS_SHA512, 9, { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0d } },

    // array terminator
    { ASN_OBJ_NONE, 0, { 0 } }
};

// The 2048-bit public key modulus for 2013 GOS CA Cert signed with sha1 and an exponent of 65537
static const uint8_t _ProtoSSL_GOS2013ServerModulus2048[] =
{
    0xc9, 0x36, 0xa7, 0xb2, 0xe8, 0x42, 0xda, 0x80, 0xd5, 0x00, 0x89, 0x1e, 0xe6, 0xf2, 0xba, 0x0f,
    0xe1, 0xb9, 0x94, 0x51, 0x0f, 0xca, 0x05, 0x49, 0x9e, 0x9d, 0xa4, 0xbf, 0x9f, 0x03, 0x16, 0xc9,
    0xd7, 0xb7, 0x6e, 0x90, 0x9c, 0x90, 0xd6, 0x17, 0x84, 0xff, 0x66, 0x01, 0x3e, 0x5f, 0x8c, 0x5e,
    0x45, 0x7b, 0x59, 0x60, 0x41, 0xb7, 0x34, 0x44, 0xff, 0x40, 0x46, 0x36, 0xd5, 0x31, 0x6d, 0xe2,
    0xa5, 0x4c, 0x4c, 0x2b, 0x9c, 0xb8, 0x2d, 0x26, 0xf1, 0xd6, 0xfc, 0x9c, 0x03, 0xc5, 0x22, 0x8d,
    0xec, 0x70, 0x56, 0x46, 0xc0, 0xdb, 0x66, 0xce, 0xc4, 0x0a, 0xef, 0x3c, 0xf5, 0xf3, 0x49, 0xb4,
    0x9d, 0xda, 0x7a, 0xef, 0x31, 0x0b, 0x1e, 0xa3, 0x20, 0x45, 0x2b, 0xc5, 0xc7, 0xf9, 0x31, 0xf4,
    0x6c, 0xff, 0x5b, 0x68, 0x93, 0x84, 0xd1, 0xa9, 0x28, 0xd7, 0xbc, 0x41, 0x25, 0x8d, 0xc1, 0x97,
    0x47, 0x3b, 0xae, 0x9f, 0xf6, 0xa0, 0xdb, 0x13, 0x6a, 0x5a, 0x3a, 0xcd, 0x6c, 0xd6, 0x07, 0x04,
    0x48, 0xb1, 0xb6, 0x63, 0xd5, 0x90, 0x6e, 0x38, 0x2d, 0x5c, 0xad, 0x36, 0xbe, 0x49, 0x86, 0xfa,
    0xe6, 0x3b, 0xc0, 0xeb, 0x51, 0xcb, 0x80, 0xc9, 0xe6, 0x6a, 0x3b, 0xa6, 0x73, 0x9f, 0x59, 0x52,
    0x64, 0xce, 0xad, 0x6a, 0x2e, 0x49, 0x11, 0x3f, 0x76, 0x7f, 0x57, 0x61, 0xba, 0x81, 0xe8, 0xed,
    0x8b, 0xec, 0xf1, 0x8f, 0x8b, 0xb8, 0x76, 0xba, 0x99, 0x9d, 0x87, 0xb7, 0xc7, 0xff, 0x21, 0xa2,
    0x72, 0xd8, 0x60, 0xd2, 0xd3, 0x1b, 0xed, 0xb8, 0xfa, 0x7b, 0x6f, 0xca, 0x88, 0xe4, 0xea, 0xe0,
    0xf9, 0xa1, 0xea, 0x00, 0x8d, 0xa4, 0x9c, 0x0d, 0x17, 0xd2, 0x70, 0x49, 0x06, 0x43, 0x6e, 0x81,
    0xb8, 0x67, 0x9b, 0x59, 0x10, 0xe7, 0x2b, 0xf0, 0x0b, 0xbe, 0x61, 0xd6, 0xd3, 0xcc, 0x4c, 0x9d
};

// The 2048-bit public key modulus for 2015 GOS CA Cert signed with sha256 and an exponent of 65537
static const uint8_t _ProtoSSL_GOS2015ServerModulus2048[] =
{
    0xcc, 0x6d, 0x54, 0xb6, 0xf4, 0xe4, 0x84, 0xe7, 0x20, 0x76, 0x02, 0xd7, 0x97, 0x48, 0x75, 0x7e,
    0x7e, 0xfb, 0x43, 0x9c, 0x3d, 0xa1, 0x96, 0x47, 0xc1, 0x5d, 0x07, 0x8b, 0x30, 0x73, 0xbf, 0x9d,
    0xfe, 0x75, 0x94, 0x55, 0x21, 0xd0, 0x88, 0x74, 0x66, 0x4c, 0xa2, 0xb7, 0xfe, 0x9f, 0xc0, 0x3b,
    0xf0, 0x60, 0xa0, 0xdb, 0x08, 0x33, 0x2b, 0x6e, 0xf8, 0x02, 0x05, 0xb9, 0x87, 0x9d, 0xac, 0x65,
    0xd5, 0x06, 0x9d, 0x05, 0xe8, 0xd1, 0xb6, 0xf5, 0xde, 0x7d, 0xa5, 0xa4, 0x7d, 0x8a, 0xcb, 0x99,
    0x31, 0xb6, 0x85, 0x9b, 0xa2, 0xce, 0x39, 0xe2, 0x8c, 0x65, 0xaa, 0x07, 0xfc, 0x15, 0x33, 0x07,
    0x00, 0xd1, 0x72, 0x15, 0x13, 0x0d, 0x87, 0x0f, 0x5c, 0xa2, 0x5e, 0xd0, 0xd5, 0xbf, 0xd9, 0x03,
    0x32, 0x62, 0xaf, 0xf5, 0xef, 0x53, 0x36, 0xa8, 0x34, 0xda, 0xb6, 0xa3, 0xec, 0x5c, 0x6a, 0xc0,
    0x67, 0xf8, 0xbe, 0x37, 0x9f, 0xb3, 0xc8, 0x2d, 0xf0, 0x36, 0x4a, 0x6f, 0x6b, 0x06, 0xee, 0xb7,
    0x85, 0xf2, 0x7f, 0x73, 0x6c, 0x01, 0x84, 0x83, 0xe4, 0xda, 0x46, 0xd0, 0x23, 0x9a, 0x6d, 0xf1,
    0x77, 0x7c, 0x05, 0x81, 0x90, 0x4f, 0x6a, 0x44, 0x83, 0x78, 0x3b, 0x71, 0xad, 0x12, 0xc0, 0x48,
    0xc8, 0x73, 0x89, 0xf1, 0x98, 0x78, 0x7b, 0xb4, 0x08, 0x4a, 0xba, 0xe8, 0x59, 0x57, 0xe2, 0xfc,
    0x29, 0xac, 0xbf, 0xf5, 0xa2, 0x9d, 0x4f, 0x2c, 0x64, 0xdc, 0xd7, 0x92, 0x19, 0x1c, 0xc5, 0xfa,
    0xdb, 0x92, 0xc0, 0x90, 0x4b, 0xa8, 0xe9, 0xf2, 0x0d, 0x94, 0x1a, 0xb2, 0x5f, 0xdd, 0x33, 0xae,
    0xff, 0x66, 0x90, 0x97, 0xb2, 0xa8, 0xa5, 0x1b, 0xfa, 0x6f, 0x41, 0xb2, 0x84, 0xba, 0x52, 0x34,
    0x97, 0x4a, 0xd3, 0xc7, 0xb2, 0x3f, 0xdd, 0xdb, 0xc9, 0xb1, 0x13, 0x82, 0x77, 0xe8, 0x6a, 0xcd
};

// The 2048-bit modulus for the VeriSign CA Cert
static const uint8_t _ProtoSSL_VeriSignServerModulus[] =
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

// The 2048-bit modulus for the VeriSign 2006 CA Cert signed with sha1 and an exponent of 65537
static const uint8_t _ProtoSSL_VeriSign2006ServerModulus[] =
{
    0xaf, 0x24, 0x08, 0x08, 0x29, 0x7a, 0x35, 0x9e, 0x60, 0x0c, 0xaa, 0xe7, 0x4b, 0x3b, 0x4e, 0xdc,
    0x7c, 0xbc, 0x3c, 0x45, 0x1c, 0xbb, 0x2b, 0xe0, 0xfe, 0x29, 0x02, 0xf9, 0x57, 0x08, 0xa3, 0x64,
    0x85, 0x15, 0x27, 0xf5, 0xf1, 0xad, 0xc8, 0x31, 0x89, 0x5d, 0x22, 0xe8, 0x2a, 0xaa, 0xa6, 0x42,
    0xb3, 0x8f, 0xf8, 0xb9, 0x55, 0xb7, 0xb1, 0xb7, 0x4b, 0xb3, 0xfe, 0x8f, 0x7e, 0x07, 0x57, 0xec,
    0xef, 0x43, 0xdb, 0x66, 0x62, 0x15, 0x61, 0xcf, 0x60, 0x0d, 0xa4, 0xd8, 0xde, 0xf8, 0xe0, 0xc3,
    0x62, 0x08, 0x3d, 0x54, 0x13, 0xeb, 0x49, 0xca, 0x59, 0x54, 0x85, 0x26, 0xe5, 0x2b, 0x8f, 0x1b,
    0x9f, 0xeb, 0xf5, 0xa1, 0x91, 0xc2, 0x33, 0x49, 0xd8, 0x43, 0x63, 0x6a, 0x52, 0x4b, 0xd2, 0x8f,
    0xe8, 0x70, 0x51, 0x4d, 0xd1, 0x89, 0x69, 0x7b, 0xc7, 0x70, 0xf6, 0xb3, 0xdc, 0x12, 0x74, 0xdb,
    0x7b, 0x5d, 0x4b, 0x56, 0xd3, 0x96, 0xbf, 0x15, 0x77, 0xa1, 0xb0, 0xf4, 0xa2, 0x25, 0xf2, 0xaf,
    0x1c, 0x92, 0x67, 0x18, 0xe5, 0xf4, 0x06, 0x04, 0xef, 0x90, 0xb9, 0xe4, 0x00, 0xe4, 0xdd, 0x3a,
    0xb5, 0x19, 0xff, 0x02, 0xba, 0xf4, 0x3c, 0xee, 0xe0, 0x8b, 0xeb, 0x37, 0x8b, 0xec, 0xf4, 0xd7,
    0xac, 0xf2, 0xf6, 0xf0, 0x3d, 0xaf, 0xdd, 0x75, 0x91, 0x33, 0x19, 0x1d, 0x1c, 0x40, 0xcb, 0x74,
    0x24, 0x19, 0x21, 0x93, 0xd9, 0x14, 0xfe, 0xac, 0x2a, 0x52, 0xc7, 0x8f, 0xd5, 0x04, 0x49, 0xe4,
    0x8d, 0x63, 0x47, 0x88, 0x3c, 0x69, 0x83, 0xcb, 0xfe, 0x47, 0xbd, 0x2b, 0x7e, 0x4f, 0xc5, 0x95,
    0xae, 0x0e, 0x9d, 0xd4, 0xd1, 0x43, 0xc0, 0x67, 0x73, 0xe3, 0x14, 0x08, 0x7e, 0xe5, 0x3f, 0x9f,
    0x73, 0xb8, 0x33, 0x0a, 0xcf, 0x5d, 0x3f, 0x34, 0x87, 0x96, 0x8a, 0xee, 0x53, 0xe8, 0x25, 0x15
};

// The 2048-bit modulus for the VeriSign 2008 CA Cert signed with sha256 and an exponent of 65537
static const uint8_t _ProtoSSL_VeriSign2008ServerModulus[] =
{
    0xC7, 0x61, 0x37, 0x5E, 0xB1, 0x01, 0x34, 0xDB, 0x62, 0xD7, 0x15, 0x9B, 0xFF, 0x58, 0x5A, 0x8C,
    0x23, 0x23, 0xD6, 0x60, 0x8E, 0x91, 0xD7, 0x90, 0x98, 0x83, 0x7A, 0xE6, 0x58, 0x19, 0x38, 0x8C,
    0xC5, 0xF6, 0xE5, 0x64, 0x85, 0xB4, 0xA2, 0x71, 0xFB, 0xED, 0xBD, 0xB9, 0xDA, 0xCD, 0x4D, 0x00,
    0xB4, 0xC8, 0x2D, 0x73, 0xA5, 0xC7, 0x69, 0x71, 0x95, 0x1F, 0x39, 0x3C, 0xB2, 0x44, 0x07, 0x9C,
    0xE8, 0x0E, 0xFA, 0x4D, 0x4A, 0xC4, 0x21, 0xDF, 0x29, 0x61, 0x8F, 0x32, 0x22, 0x61, 0x82, 0xC5,
    0x87, 0x1F, 0x6E, 0x8C, 0x7C, 0x5F, 0x16, 0x20, 0x51, 0x44, 0xD1, 0x70, 0x4F, 0x57, 0xEA, 0xE3,
    0x1C, 0xE3, 0xCC, 0x79, 0xEE, 0x58, 0xD8, 0x0E, 0xC2, 0xB3, 0x45, 0x93, 0xC0, 0x2C, 0xE7, 0x9A,
    0x17, 0x2B, 0x7B, 0x00, 0x37, 0x7A, 0x41, 0x33, 0x78, 0xE1, 0x33, 0xE2, 0xF3, 0x10, 0x1A, 0x7F,
    0x87, 0x2C, 0xBE, 0xF6, 0xF5, 0xF7, 0x42, 0xE2, 0xE5, 0xBF, 0x87, 0x62, 0x89, 0x5F, 0x00, 0x4B,
    0xDF, 0xC5, 0xDD, 0xE4, 0x75, 0x44, 0x32, 0x41, 0x3A, 0x1E, 0x71, 0x6E, 0x69, 0xCB, 0x0B, 0x75,
    0x46, 0x08, 0xD1, 0xCA, 0xD2, 0x2B, 0x95, 0xD0, 0xCF, 0xFB, 0xB9, 0x40, 0x6B, 0x64, 0x8C, 0x57,
    0x4D, 0xFC, 0x13, 0x11, 0x79, 0x84, 0xED, 0x5E, 0x54, 0xF6, 0x34, 0x9F, 0x08, 0x01, 0xF3, 0x10,
    0x25, 0x06, 0x17, 0x4A, 0xDA, 0xF1, 0x1D, 0x7A, 0x66, 0x6B, 0x98, 0x60, 0x66, 0xA4, 0xD9, 0xEF,
    0xD2, 0x2E, 0x82, 0xF1, 0xF0, 0xEF, 0x09, 0xEA, 0x44, 0xC9, 0x15, 0x6A, 0xE2, 0x03, 0x6E, 0x33,
    0xD3, 0xAC, 0x9F, 0x55, 0x00, 0xC7, 0xF6, 0x08, 0x6A, 0x94, 0xB9, 0x5F, 0xDC, 0xE0, 0x33, 0xF1,
    0x84, 0x60, 0xF9, 0x5B, 0x27, 0x11, 0xB4, 0xFC, 0x16, 0xF2, 0xBB, 0x56, 0x6A, 0x80, 0x25, 0x8D
};

// The 2048-bit public key modulus for the GeoTrust CA
static const uint8_t _ProtoSSL_GeoTrustServerModulus[] =
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

// only certificates from these authorities are supported
static ProtoSSLCACertT _ProtoSSL_CACerts[] =
{
    // gos2013 CA
    { { "US", "California", "Redwood City", "Electronic Arts, Inc.",
        "Global Online Studio/emailAddress=GOSDirtysockSupport@ea.com",
        "GOS 2013 Certificate Authority" },
      SSL_CACERTFLAG_GOSCA,
      256, _ProtoSSL_GOS2013ServerModulus2048,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, &_ProtoSSL_CACerts[1] },
    // gos2015 CA
    { { "US", "California", "Redwood City", "Electronic Arts, Inc.",
        "Global Online Studio/emailAddress=GOSDirtysockSupport@ea.com",
        "GOS 2015 Certificate Authority" },
      SSL_CACERTFLAG_GOSCA|SSL_CACERTFLAG_CAPROVIDER,
      256, _ProtoSSL_GOS2015ServerModulus2048,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, &_ProtoSSL_CACerts[2] },
    // verisign CA
    { { "US", "", "", "VeriSign, Inc.",
        "VeriSign Trust Network, Terms of use at https://www.verisign.com/rpa (c)05",
        "VeriSign Class 3 Secure Server CA" },
      SSL_CACERTFLAG_NONE,
      256, _ProtoSSL_VeriSignServerModulus,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, &_ProtoSSL_CACerts[3]},
    // verisign 2006 CA
    { { "US", "", "", "VeriSign, Inc.",
        "VeriSign Trust Network, (c) 2006 VeriSign, Inc. - For authorized use only",
        "VeriSign Class 3 Public Primary Certification Authority - G5" },
      SSL_CACERTFLAG_NONE,
      256, _ProtoSSL_VeriSign2006ServerModulus,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, &_ProtoSSL_CACerts[4]},
    // verisign 2008 CA
    { { "US", "", "", "VeriSign, Inc.",
        "VeriSign Trust Network, (c) 2008 VeriSign, Inc. - For authorized use only",
        "VeriSign Universal Root Certification Authority" },
      SSL_CACERTFLAG_NONE,
      256, _ProtoSSL_VeriSign2008ServerModulus,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, &_ProtoSSL_CACerts[5]},
    // geotrust CA
    { { "US", "", "", "GeoTrust, Inc.", "", "GeoTrust SSL CA" },
      SSL_CACERTFLAG_NONE,
      256, _ProtoSSL_GeoTrustServerModulus,
      3, { 0x01, 0x00, 0x01 },
      0, NULL, NULL, NULL },
};

/*** Private functions ******************************************************/


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

    // acquire access to secure state
    NetCritEnter(&pState->SecureCrit);

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
            ds_memclr(pState->pSecure, sizeof(*pState->pSecure));
        }
    }

    // reset secure state if present
    if ((pSecure = pState->pSecure) != NULL)
    {
        // clear secure state
        ds_memclr(pSecure, sizeof(*pSecure));

        // reset handshake hashes
        CryptMD5Init(&pSecure->HandshakeMD5);
        CryptSha1Init(&pSecure->HandshakeSHA);
        CryptSha2Init(&pSecure->HandshakeSHA256, SSL3_MAC_SHA256);
        CryptSha2Init(&pSecure->HandshakeSHA384, SSL3_MAC_SHA384);
    }

    // release access to secure state
    NetCritLeave(&pState->SecureCrit);

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

    \Version 03/25/2004 (gschaefer)
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
    if (pState->iCARequestId > 0)
    {
        DirtyCertCARequestFree(pState->iCARequestId);
    }
    pState->iCARequestId = 0;

    // reset the state
    pState->iState = ST_IDLE;
    pState->iClosed = 1;
    pState->uAlertLevel = 0;
    pState->uAlertValue = 0;
    pState->bAlertSent = FALSE;

    // reset secure state
    return(_ResetSecureState(pState, iSecure));
}

/*
    X.509 ASN.1 parsing routines
*/

/*F*************************************************************************************************/
/*!
    \Function _GetObject

    \Description
        Return OID based on type

    \Input iType    - Type of OID (ASN_OBJ_*)

    \Output
        ASNObjectT *- pointer to OID, or NULL if not found

    \Version 03/18/2015 (jbrookes)
*/
/*************************************************************************************************F*/
static const ASNObjectT *_GetObject(int32_t iType)
{
    const ASNObjectT *pObject;
    int32_t iIndex;

    // locate the matching type
    for (iIndex = 0, pObject = NULL; _SSL_ObjectList[iIndex].iType != ASN_OBJ_NONE; iIndex += 1)
    {
        if (_SSL_ObjectList[iIndex].iType == iType)
        {
            pObject = &_SSL_ObjectList[iIndex];
            break;
        }
    }

    return(pObject);
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

    \Version 03/08/2002 (gschaefer)
*/
/*************************************************************************************************F*/
static const uint8_t *_ParseHeader(const uint8_t *pData, const uint8_t *pLast, int32_t *pType, int32_t *pSize)
{
    int32_t iCnt;
    uint32_t uLen;
    int32_t iType, iSize;

    // reset the output
    if (pSize != NULL)
    {
        *pSize = 0;
    }
    if (pType != NULL)
    {
        *pType = 0;
    }

    /* handle end of data (including early termination due to data truncation or
       invalid data) and make sure we have at least enough data for the type and
       size  */
    if ((pData == NULL) || (pData+2 > pLast))
    {
        return(NULL);
    }

    // get the type
    iType = *pData++;
    if (pType != NULL)
    {
        *pType = iType;
    }

    // figure the length
    if ((uLen = *pData++) > 127)
    {
        // get number of bits
        iCnt = (uLen & 127);
        // validate length (do not allow overflow) and make sure there is enough data
        if ((iCnt > (int32_t)sizeof(uLen)) || ((pData + iCnt) > pLast))
        {
            return(NULL);
        }
        // calc the length
        for (uLen = 0; iCnt > 0; --iCnt)
        {
            uLen = (uLen << 8) | *pData++;
        }
    }
    iSize = (signed)uLen;
    // validate record size
    if ((iSize < 0) || ((pData+iSize) > pLast))
    {
        return(NULL);
    }
    // save the size
    if (pSize != NULL)
    {
        *pSize = iSize;
    }

    #if DEBUG_ALL_OBJS
    NetPrintf(("protossl: _ParseHeader type=%s (0x%02x) size=%d\n",
        _SSL3_strAsnTypes[iType&~(ASN_CONSTRUCT|ASN_IMPLICIT_TAG|ASN_EXPLICIT_TAG)],
        iType, iSize));
    #if DEBUG_RAW_DATA
    NetPrintMem(pData, iSize, "memory");
    #endif
    #endif

    // return pointer to next
    return(pData);
}

/*F*************************************************************************************************/
/*!
    \Function _ParseHeaderType

    \Description
        Parse an asn.1 header of specified type

    \Input *pData   - pointer to header data to parse
    \Input *pLast   - pointer to end of header
    \Input iType    - type of header to extract
    \Input *pSize   - pointer to storage for data size

    \Output uint8_t * - pointer to next block, or NULL if end of stream or unexpected type

    \Version 10/10/2012 (jbrookes)
*/
/*************************************************************************************************F*/
static const uint8_t *_ParseHeaderType(const uint8_t *pData, const uint8_t *pLast, int32_t iType, int32_t *pSize)
{
    int32_t iTypeParsed;
    pData = _ParseHeader(pData, pLast, &iTypeParsed, pSize);
    if (iTypeParsed != iType)
    {
        return(NULL);
    }
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
        int32_t     - type of object; zero if object is unrecognized

    \Version 03/08/2002 (gschaefer)
*/
/*************************************************************************************************F*/
static int32_t _ParseObject(const uint8_t *pData, int32_t iSize)
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
            NetPrintf(("protossl: _ParseObject obj=%s (%d)\n", _SSL3_strAsnObjs[iType], iType));
            #if DEBUG_RAW_DATA
            NetPrintMem(pData, _SSL_ObjectList[iIndex].iSize, "memory");
            #endif
            #endif

            break;
        }
    }

    #if DEBUG_ALL_OBJS
    if (iType == 0)
    {
        NetPrintMem(pData, iSize, "unrecognized asn.1 object");
    }
    #endif

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

    \Version 03/08/2002 (gschaefer)
*/
/*************************************************************************************************F*/
static void _ParseString(const unsigned char *pData, int32_t iSize, char *pString, int32_t iLength)
{
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
static void _ParseStringMulti(const uint8_t *pData, int32_t iSize, char *pString, int32_t iLength)
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
    // terminate
    if (iLength > 0)
    {
        *pString = '\0';
    }
}

/*F********************************************************************************/
/*!
    \Function _SplitDelimitedString

    \Description
        Splits string on a delimiter

    \Input *pSource     - the string we are splitting
    \Input cDelimiter   - delimited to split on
    \Input *pDest       - where we are writing the data split
    \Input iDstSize     - size of dest string
    \Input **pNewSource - used to save position of iteration

    \Output
        uint32_t    - length of destination or zero if nothing parsed

    \Version 08/04/2016 (eesponda)
*/
/********************************************************************************F*/
static uint32_t _SplitDelimitedString(const char *pSource, char cDelimiter, char *pDest, int32_t iDstSize, const char **pNewSource)
{
    const char *pLocation;
    int32_t iSrcSize;

    // check for validity of inputs
    if ((pSource == NULL) || (*pSource == '\0'))
    {
        return(0);
    }
    iSrcSize = (int32_t)strlen(pSource);

    /* if we start with delimiter skip it
       this happens when you do multiple parses */
    if (pSource[0] == cDelimiter)
    {
        pSource += 1;
        iSrcSize -= 1;
        (*pNewSource) += 1;
    }

    // terminate the destination
    if (pDest != NULL && iDstSize > 0)
    {
        pDest[0] = '\0';
    }

    /* write the value to the destination (if not null)
       when length is less than max, add 1 for nul terminator */
    if ((pLocation = strchr(pSource, cDelimiter)) != NULL)
    {
        const int32_t iCount = (int32_t)(pLocation - pSource);
        (*pNewSource) += iCount;
        if (pDest != NULL)
        {
            ds_strnzcpy(pDest, pSource, DS_MIN(iDstSize, iCount+1));
        }
        return(iCount);
    }
    else
    {
        (*pNewSource) += iSrcSize;
        if (pDest != NULL)
        {
            ds_strnzcpy(pDest, pSource, DS_MIN(iDstSize, iSrcSize+1));
        }
        return(iSrcSize);
    }
}

/*F*************************************************************************************************/
/*!
    \Function _ParseIdent

    \Description
        Extract an Ident (subject/issuer) from certificate

    \Input *pData   - pointer to data to extract string from
    \Input iSize    - size of source data
    \Input *pIdent  - [out] storage for parsed ident fields

    \Output
        const uint8_t *     - pointer past end of ident

    \Version 10/09/2012 (jbrookes)
*/
/*************************************************************************************************F*/
static const uint8_t *_ParseIdent(const uint8_t *pData, int32_t iSize, ProtoSSLCertIdentT *pIdent)
{
    int32_t iObjType, iType;
    const uint8_t *pDataEnd;

    for (iObjType = 0, pDataEnd = pData+iSize; (pData = _ParseHeader(pData, pDataEnd, &iType, &iSize)) != NULL; )
    {
        // skip past seqn/set references
        if ((iType == ASN_TYPE_SEQN+ASN_CONSTRUCT) || (iType == ASN_TYPE_SET+ASN_CONSTRUCT))
        {
            continue;
        }
        if (iType == ASN_TYPE_OBJECT+ASN_PRIMITIVE)
        {
            iObjType = _ParseObject(pData, iSize);
        }
        if ((iType == ASN_TYPE_PRINTSTR+ASN_PRIMITIVE) || (iType == ASN_TYPE_UTF8STR+ASN_PRIMITIVE) || (iType == ASN_TYPE_T61+ASN_PRIMITIVE))
        {
            if (iObjType == ASN_OBJ_COUNTRY)
            {
                _ParseString(pData, iSize, pIdent->strCountry, sizeof(pIdent->strCountry));
            }
            if (iObjType == ASN_OBJ_STATE)
            {
                _ParseString(pData, iSize, pIdent->strState, sizeof(pIdent->strState));
            }
            if (iObjType == ASN_OBJ_CITY)
            {
                _ParseString(pData, iSize, pIdent->strCity, sizeof(pIdent->strCity));
            }
            if (iObjType == ASN_OBJ_ORGANIZATION)
            {
                _ParseString(pData, iSize, pIdent->strOrg, sizeof(pIdent->strOrg));
            }
            if (iObjType == ASN_OBJ_UNIT)
            {
                if (pIdent->strUnit[0] != '\0')
                {
                    ds_strnzcat(pIdent->strUnit, ", ", sizeof(pIdent->strUnit));
                }
                _ParseStringMulti(pData, iSize, pIdent->strUnit, sizeof(pIdent->strUnit));
            }
            if (iObjType == ASN_OBJ_COMMON)
            {
                _ParseString(pData, iSize, pIdent->strCommon, sizeof(pIdent->strCommon));
            }
            iObjType = 0;
        }
        pData += iSize;
    }

    return(pDataEnd);
}

/*F***************************************************************************/
/*!
    \Function _ParseDate

    \Description
        Parse and extract a date object from ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input iSize    - size of object
    \Input *pBuffer - [out] output for date string object
    \Input iBufSize - size of output buffer
    \Input *pDateTime - [out] storage for converted time, optional

    \Output
        const uint8_t * - pointer past object, or NULL if an error occurred

    \Version 10/10/2012 (jbrookes)
*/
/***************************************************************************F*/
static const uint8_t *_ParseDate(const uint8_t *pData, int32_t iSize, char *pBuffer, int32_t iBufSize, uint64_t *pDateTime)
{
    int32_t iType;
    if (((pData = _ParseHeader(pData, pData+iSize, &iType, &iSize)) == NULL) || ((iType != ASN_TYPE_UTCTIME) && (iType != ASN_TYPE_GENERALIZEDTIME)))
    {
        return(NULL);
    }
    _ParseString(pData, iSize, pBuffer, iBufSize);
    if (pDateTime != NULL)
    {
        *pDateTime = ds_strtotime2(pBuffer, iType == ASN_TYPE_UTCTIME ? TIMETOSTRING_CONVERSION_ASN1_UTCTIME : TIMETOSTRING_CONVERSION_ASN1_GENTIME);
    }
    pData += iSize;
    return(pData);
}

/*F***************************************************************************/
/*!
    \Function _ParseBinaryPtr

    \Description
        Parse and extract a binary object from ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input *pLast   - pointer to end of data
    \Input iType    - type of data we are expecting
    \Input **ppObj   - [out] storage for pointer to binary object
    \Input *pObjSize - [out] storage for size of binary object
    \Input *pName   - name of object (used for debug output)

    \Output
        const uint8_t * - pointer past object, or NULL if an error occurred

    \Version 11/18/2013 (jbrookes)
*/
/***************************************************************************F*/
static const uint8_t *_ParseBinaryPtr(const uint8_t *pData, const uint8_t *pLast, int32_t iType, uint8_t **ppObj, int32_t *pObjSize, const char *pName)
{
    int32_t iSize;
    if ((pData = _ParseHeaderType(pData, pLast, iType, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseBinary: could not get %s\n", pName));
        return(NULL);
    }
    // skip leading zero if present
    if (*pData == '\0')
    {
        pData += 1;
        iSize -= 1;
    }
    // save object pointer
    *ppObj = (uint8_t *)pData;
    // save object size
    *pObjSize = iSize;
    // skip object
    return(pData+iSize);
}

/*F***************************************************************************/
/*!
    \Function _ParseBinary

    \Description
        Parse and extract a binary object from ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input *pLast   - pointer to end of data
    \Input iType    - type of data we are expecting
    \Input *pBuffer - [out] output for binary object (may be null to skip)
    \Input iBufSize - size of output buffer
    \Input *pOutSize - [out] output for binary object size
    \Input *pName   - name of object (used for debug output)

    \Output
        const uint8_t * - pointer past object, or NULL if an error occurred

    \Version 10/10/2012 (jbrookes)
*/
/***************************************************************************F*/
static const uint8_t *_ParseBinary(const uint8_t *pData, const uint8_t *pLast, int32_t iType, uint8_t *pBuffer, int32_t iBufSize, int32_t *pOutSize, const char *pName)
{
    const uint8_t *pObjData = NULL, *pNext;
    // parse object info
    pNext = _ParseBinaryPtr(pData, pLast, iType, (uint8_t **)&pObjData, pOutSize, pName);
    // save data
    if ((pObjData != NULL) && (pBuffer != NULL))
    {
        // validate size
        if (*pOutSize > iBufSize)
        {
            NetPrintf(("protossl: _ParseBinary: %s is too large (size=%d, max=%d)\n", pName, *pOutSize, iBufSize));
            return(NULL);
        }
        ds_memcpy(pBuffer, pObjData, *pOutSize);
    }
    // skip object
    return(pNext);
}

/*F***************************************************************************/
/*!
    \Function _ParseOptional

    \Description
        Parse optional object of ASN.1 certificate

    \Input *pData   - pointer to header data
    \Input iSize    - size of header
    \Input *pCert   - pointer to certificate to fill in from header data

    \Output
        int32_t     - negative=error, zero=no error

    \Version 10/10/2012 (jbrookes) Extracted from _ParseCertificate()
*/
/***************************************************************************F*/
static int32_t _ParseOptional(const uint8_t *pData, int32_t iSize, X509CertificateT *pCert)
{
    const uint8_t *pLast;
    int32_t iCritical, iObjType, iLen, iType;

    for (iCritical = iObjType = 0, pLast = pData+iSize; (pData = _ParseHeader(pData, pLast, &iType, &iSize)) != NULL; )
    {
        // ignore seqn/set references
        if ((iType == ASN_TYPE_SEQN+ASN_CONSTRUCT) || (iType == ASN_TYPE_SET+ASN_CONSTRUCT))
        {
            continue;
        }

        // parse the object type
        if (iType == ASN_TYPE_OBJECT+ASN_PRIMITIVE)
        {
            iObjType = _ParseObject(pData, iSize);
        }

        // parse a subject alternative name (SAN) object
        if (iObjType == ASN_OBJ_SUBJECT_ALT)
        {
            if (iType == ASN_TYPE_OCTSTRING+ASN_PRIMITIVE)
            {
                // save reference to subject alternative blob
                pCert->iSubjectAltLen = iSize;
                pCert->pSubjectAlt = pData;
            }
        }

        // parse a basic constraints object
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
                        return(-1);
                    }
                }
                else
                {
                    // this is the flag to indicate whether it's a CA
                    pCert->iCertIsCA = (*pData != 0) ? TRUE : FALSE;
                }
            }

            if ((iType == ASN_TYPE_INTEGER+ASN_PRIMITIVE) && (pCert->iCertIsCA != FALSE) && (iSize <= (signed)sizeof(pCert->iMaxHeight)))
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
                    return(-2);
                }
            }
        }
        pData += iSize;
    }
    if (pCert->pSubjectAlt != NULL)
    {
        NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: parsed SAN; length=%d\n", pCert->iSubjectAltLen));
    }
    return(0);
}

/*F***************************************************************************/
/*!
    \Function _CalcFingerprint

    \Description
        Calculate fingerprint for given data

    \Input *pOutBuf - output buffer to store fingerprint
    \Input iOutSize - size of output buffer
    \Input *pInpData - input data to calculate fingerprint for
    \Input iInpSize - size of input data
    \Input eHashType - hash operation to use

    \Version 04/29/2014 (jbrookes)
*/
/***************************************************************************F*/
static void _CalcFingerprint(uint8_t *pOutBuf, int32_t iOutSize, const uint8_t *pInpData, int32_t iInpSize, CryptHashTypeE eHashType)
{
    const CryptHashT *pHash;

    // calculate the fingerprint using designated hash type
    if ((pHash = CryptHashGet(eHashType)) != NULL)
    {
        uint8_t aHashState[CRYPTHASH_MAXSTATE];
        int32_t iHashSize = CryptHashGetSize(eHashType);
        if (iHashSize > iOutSize)
        {
            iHashSize = iOutSize;
        }
        pHash->Init(aHashState, iHashSize);
        pHash->Update(aHashState, pInpData, iInpSize);
        pHash->Final(aHashState, pOutBuf, iHashSize);
        #if DEBUG_VAL_CERT
        NetPrintMem(pOutBuf, iHashSize, "sha1-fingerprint");
        #endif
    }
    else
    {
        NetPrintf(("protossl: _ParseCertificate: could not calculate fingerprint\n"));
    }
}

/*F***************************************************************************/
/*!
    \Function _ParseCertificate

    \Description
        Parse an x.509 certificate in ASN.1 format

    \Input *pCert   - pointer to certificate to fill in from header data
    \Input *pData   - pointer to header data
    \Input iSize    - size of header

    \Output
        int32_t     - negative=error, zero=no error

    \Version 03/08/2002 (gschaefer)
*/
/***************************************************************************F*/
static int32_t _ParseCertificate(X509CertificateT *pCert, const uint8_t *pData, int32_t iSize)
{
    const uint8_t *pInfData;
    const uint8_t *pInfSkip;
    const uint8_t *pSigSkip;
    const uint8_t *pKeySkip;
    const uint8_t *pKeyData;
    const uint8_t *pLast = pData+iSize;
    const CryptHashT *pHash;
    CryptHashTypeE eHashType;
    int32_t iKeySize, iType;

    // clear the certificate
    ds_memclr(pCert, sizeof(*pCert));

    // process the base sequence
    if ((pData = _ParseHeaderType(pData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not parse base sequence\n"));
        return(-1);
    }

    // process the info sequence
    if ((pData = _ParseHeaderType(pInfData = pData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not parse info sequence\n"));
        return(-2);
    }
    pInfSkip = pData+iSize;

    // skip any non-integer tag (optional version)
    if (*pData != ASN_TYPE_INTEGER+ASN_PRIMITIVE)
    {
        if ((pData = _ParseHeader(pData, pLast, NULL, &iSize)) == NULL)
        {
            NetPrintf(("protossl: _ParseCertificate: could not skip non-integer tag\n"));
            return(-3);
        }
        pData += iSize;
    }

    // grab the certificate serial number
    if (((pData = _ParseHeader(pData, pInfSkip, &iType, &iSize)) == NULL) || ((unsigned)iSize > sizeof(pCert->SerialData)))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get certificate serial number (iSize=%d)\n", iSize));
        return(-4);
    }
    pCert->iSerialSize = iSize;
    ds_memcpy(pCert->SerialData, pData, iSize);
    pData += iSize;

    // find signature algorithm
    if ((pData = _ParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature algorithm\n"));
        return(-5);
    }
    pSigSkip = pData+iSize;

    // get the signature algorithm type
    if ((pData = _ParseHeaderType(pData, pInfSkip, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature algorithm type\n"));
        return(-6);
    }
    if ((pCert->iSigType = _ParseObject(pData, iSize)) == ASN_OBJ_NONE)
    {
        NetPrintMem(pData, iSize, "protossl: unsupported signature algorithm");
        return(-7);
    }
    pData += iSize;

    // parse issuer
    if ((pData = _ParseHeaderType(pSigSkip, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get issuer\n"));
        return(-8);
    }
    pData = _ParseIdent(pData, iSize, &pCert->Issuer);

    // parse the validity info
    if ((pData = _ParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get validity info\n"));
        return(-9);
    }

    // get validity dates
    if ((pData = _ParseDate(pData, iSize, pCert->strGoodFrom, sizeof(pCert->strGoodFrom), &pCert->uGoodFrom)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get from date\n"));
        return(-10);
    }
    if ((pData = _ParseDate(pData, iSize, pCert->strGoodTill, sizeof(pCert->strGoodTill), &pCert->uGoodTill)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get to date\n"));
        return(-11);
    }
    #if DEBUG_VAL_CERT
    {
        char strTimeFrom[32], strTimeTill[32];
        NetPrintf(("protossl: certificate valid from %s to %s\n",
            ds_secstostr(pCert->uGoodFrom, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTimeFrom, sizeof(strTimeFrom)),
            ds_secstostr(pCert->uGoodTill, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTimeTill, sizeof(strTimeTill))));
    }
    #endif

    // get subject
    if ((pData = _ParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get subject\n"));
        return(-12);
    }
    pData = _ParseIdent(pData, iSize, &pCert->Subject);

    // parse the public key
    if ((pData = _ParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get public key\n"));
        return(-13);
    }

    // find the key algorithm sequence
    if ((pData = _ParseHeaderType(pData, pInfSkip, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get key algorithm sequence\n"));
        return(-14);
    }
    pKeySkip = pData+iSize;

    // grab the public key algorithm
    if ((pData = _ParseHeaderType(pData, pKeySkip, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get public key algorithm\n"));
        return(-15);
    }
    pCert->iKeyType = _ParseObject(pData, iSize);

    // find the actual public key
    if (((pData = _ParseHeaderType(pKeySkip, pLast, ASN_TYPE_BITSTRING+ASN_PRIMITIVE, &iSize)) == NULL) || (iSize < 1))
    {
        NetPrintf(("protossl: _ParseCertificate: could not get actual public key\n"));
        return(-16);
    }

    // skip the "extra bits" indicator and save keydata ref
    pKeyData = pData+1;
    iKeySize = iSize-1;
    pData += iSize;

    // parse optional info object, if present
    if ((pData = _ParseHeaderType(pData, pInfSkip, ASN_EXPLICIT_TAG+ASN_TYPE_BITSTRING, &iSize)) != NULL)
    {
        if (_ParseOptional(pData, iSize, pCert) < 0)
        {
            return(-17);
        }
    }

    // parse signature algorithm sequence
    if ((pData = _ParseHeaderType(pInfSkip, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature algorithm sequence\n"));
        return(-18);
    }
    pSigSkip = pData+iSize;

    // parse signature algorithm identifier
    if ((pData = _ParseHeaderType(pData, pLast, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParseCertificate: could not get signature algorithm identifier\n"));
        return(-19);
    }
    pCert->iSigType = _ParseObject(pData, iSize);

    // parse the signature data
    if ((pData = _ParseBinary(pSigSkip, pLast, ASN_TYPE_BITSTRING+ASN_PRIMITIVE, pCert->SigData, sizeof(pCert->SigData), &pCert->iSigSize, "signature data")) == NULL)
    {
        return(-20);
    }

    // parse the public key data (extract modulus & exponent)
    if (pCert->iKeyType == ASN_OBJ_RSA_PKCS_KEY)
    {
        pLast = pKeyData+iKeySize;

        // parse the sequence
        if ((pData = _ParseHeaderType(pKeyData, pLast, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
        {
            NetPrintf(("protossl: _ParseCertificate: could not get public key sequence\n"));
            return(-21);
        }
        // parse the key modulus
        if ((pData = _ParseBinary(pData, pLast, ASN_TYPE_INTEGER+ASN_PRIMITIVE, pCert->KeyModData, sizeof(pCert->KeyModData), &pCert->iKeyModSize, "key modulus")) == NULL)
        {
            return(-22);
        }
        #if DEBUG_MOD_PRNT
        NetPrintMem(pCert->KeyModData, pCert->iKeyModSize, "public key modulus");
        #endif
        // parse the key exponent
        if ((pData = _ParseBinary(pData, pLast, ASN_TYPE_INTEGER+ASN_PRIMITIVE, pCert->KeyExpData, sizeof(pCert->KeyExpData), &pCert->iKeyExpSize, "key exponent")) == NULL)
        {
            return(-23);
        }
    }

    // generate the certificate hash
    switch(pCert->iSigType)
    {
        case ASN_OBJ_RSA_PKCS_MD2:
            eHashType = CRYPTHASH_MD2;
            break;
        case ASN_OBJ_RSA_PKCS_MD5:
            eHashType = CRYPTHASH_MD5;
            break;
        case ASN_OBJ_RSA_PKCS_SHA1:
            eHashType = CRYPTHASH_SHA1;
            break;
        case ASN_OBJ_RSA_PKCS_SHA256:
            eHashType = CRYPTHASH_SHA256;
            break;
        case ASN_OBJ_RSA_PKCS_SHA384:
            eHashType = CRYPTHASH_SHA384;
            break;
        case ASN_OBJ_RSA_PKCS_SHA512:
            eHashType = CRYPTHASH_SHA512;
            break;
        default:
            eHashType = CRYPTHASH_NULL;
            break;
    }

    // get hash function block for this hash type
    if ((pHash = CryptHashGet(eHashType)) != NULL)
    {
        // do the hash
        uint8_t aContext[CRYPTHASH_MAXSTATE];
        pHash->Init(aContext, pHash->iHashSize);
        pHash->Update(aContext, pInfData, (int32_t)(pInfSkip-pInfData));
        pHash->Final(aContext, pCert->HashData, pCert->iHashSize = pHash->iHashSize);
    }
    else
    {
        NetPrintf(("protossl: could not get hash type %d\n", eHashType));
        return(-23);
    }

    // certificate parsed successfully
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ParsePrivateKey

    \Description
        Parse public key modulus and public or private key exponent from private
        key certificate.

    \Input *pPrivateKeyData - private key certificate data in base64 form
    \Input iPrivateKeyLen   - length of private key certificate data
    \Input *pPrivateKey     - [out] parsed private key data

    \Output
        int32_t             - private key certificate binary (decoded) size, or negative on error

    \Notes
        \verbatim
            RSAPrivateKey ::= SEQUENCE {
              version Version,
              modulus INTEGER,
              publicExponent INTEGER,
              privateExponent INTEGER,
              primeP INTEGER,
              primeQ INTEGER,
              exponentP INTEGER,
              exponentQ INTEGER,
              coefficient INTEGER
            }
        \endverbatim

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ParsePrivateKey(const char *pPrivateKeyData, int32_t iPrivateKeyLen, X509PrivateKeyT *pPrivateKey)
{
    const uint8_t *pData;
    int32_t iPrivKeySize, iSize, iTemp = 0;

    // clear output structure
    ds_memclr(pPrivateKey, sizeof(*pPrivateKey));

    // decode private key into private key buffer
    if ((iPrivKeySize = (pPrivateKey != NULL) ? Base64Decode3(pPrivateKeyData, iPrivateKeyLen, pPrivateKey->strPrivKeyData, sizeof(pPrivateKey->strPrivKeyData)) : 0) == 0)
    {
        NetPrintf(("protossl: _ParsePrivateKey: could not decode private key\n"));
        return(-1);
    }
    // parse the sequence
    if ((pData = _ParseHeaderType((uint8_t *)pPrivateKey->strPrivKeyData, (uint8_t *)pPrivateKey->strPrivKeyData+iPrivKeySize, ASN_TYPE_SEQN+ASN_CONSTRUCT, &iSize)) == NULL)
    {
        NetPrintf(("protossl: _ParsePrivateKey: could not get private key sequence\n"));
        return(-2);
    }
    // skip the version
    if ((pData = _ParseBinary(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, NULL, 0, &iTemp, "version")) == NULL)
    {
        return(-3);
    }
    // parse public key modulus
    if ((pData = _ParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->Modulus.pObjData, &pPrivateKey->Modulus.iObjSize, "key modulus")) == NULL)
    {
        return(-4);
    }
    // parse public key exponent
    if ((pData = _ParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->PublicExponent.pObjData, &pPrivateKey->PublicExponent.iObjSize, "public key exponent")) == NULL)
    {
        return(-5);
    }
    // parse private key exponent
    if ((pData = _ParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->PrivateExponent.pObjData, &pPrivateKey->PrivateExponent.iObjSize, "private key exponent")) == NULL)
    {
        return(-6);
    }
    // parse primeP
    if ((pData = _ParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->PrimeP.pObjData, &pPrivateKey->PrimeP.iObjSize, "primeP")) == NULL)
    {
        return(-7);
    }
    // parse primeQ
    if ((pData = _ParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->PrimeQ.pObjData, &pPrivateKey->PrimeQ.iObjSize, "primeQ")) == NULL)
    {
        return(-8);
    }
    // parse exponentP
    if ((pData = _ParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->ExponentP.pObjData, &pPrivateKey->ExponentP.iObjSize, "exponentP")) == NULL)
    {
        return(-9);
    }
    // parse exponentQ
    if ((pData = _ParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->ExponentQ.pObjData, &pPrivateKey->ExponentQ.iObjSize, "exponentQ")) == NULL)
    {
        return(-10);
    }
    // parse coefficient
    if ((pData = _ParseBinaryPtr(pData, pData+iSize, ASN_TYPE_INTEGER+ASN_PRIMITIVE, &pPrivateKey->Coefficient.pObjData, &pPrivateKey->Coefficient.iObjSize, "coefficient")) == NULL)
    {
        return(-11);
    }
    return(iPrivKeySize);
}

/*F***************************************************************************/
/*!
    \Function _FindPEMSignature

    \Description
        Find PEM signature in the input data, if it exists, and return a
        pointer to the encapsulated data.

    \Input *pCertData       - pointer to data to scan
    \Input iCertSize        - size of input data
    \Input *pSigText        - signature text to find

    \Output
        const uint8_t *     - pointer to data, or null if not found

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
        ((*pCertEnd = _FindPEMSignature(*pCertBeg, (int32_t)(pCertData+iCertSize-*pCertBeg), _strPemEnd)) != NULL))
    {
        *pCertBeg += strlen(_strPemBeg);
        return((int32_t)(*pCertEnd-*pCertBeg));
    }
    else if (((*pCertBeg = _FindPEMSignature(pCertData, iCertSize, _str509Beg)) != NULL) &&
             ((*pCertEnd = _FindPEMSignature(*pCertBeg, (int32_t)(pCertData+iCertSize-*pCertBeg), _str509End)) != NULL))
    {
        *pCertBeg += strlen(_str509Beg);
        return((int32_t)(*pCertEnd-*pCertBeg));
    }
    return(0);
}

/*
    Certificate use functions
*/

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
    ds_snzprintf(pStrBuf, iBufLen, "C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s",
        pIdent->strCountry, pIdent->strState, pIdent->strCity,
        pIdent->strOrg, pIdent->strUnit, pIdent->strCommon);
    return(pStrBuf);
}
#else
#define _DebugFormatCertIdent(__pIdent, __pStrBuf, __iBufLen)
#endif

#if DIRTYCODE_LOGGING
/*F***************************************************************************/
/*!
    \Function _DebugPrintCert

    \Description
        Print cert ident info to debug output.

    \Input *pCert       - cert to print debug info for
    \Input *pMessage    - string identifier (may be null)

    \Version 01/13/2009 (jbrookes)
*/
/***************************************************************************F*/
static void _DebugPrintCert(const X509CertificateT *pCert, const char *pMessage)
{
    char strCertIdent[1024];
    #if DEBUG_VAL_CERT
    char strTimeFrom[32], strTimeTill[32];
    #endif
    if (pMessage != NULL)
    {
        NetPrintf(("protossl: %s\n", pMessage));
    }
    NetPrintf(("protossl:  issuer: %s\n", _DebugFormatCertIdent(&pCert->Issuer, strCertIdent, sizeof(strCertIdent))));
    NetPrintf(("protossl: subject: %s\n", _DebugFormatCertIdent(&pCert->Subject, strCertIdent, sizeof(strCertIdent))));
    #if DEBUG_VAL_CERT
    NetPrintf(("protossl:   valid: from %s to %s\n",
            ds_secstostr(pCert->uGoodFrom, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTimeFrom, sizeof(strTimeFrom)),
            ds_secstostr(pCert->uGoodTill, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTimeTill, sizeof(strTimeTill))));
    NetPrintf(("protossl: keytype: %d\n", pCert->iKeyType));
    NetPrintf(("protossl: keyexp size: %d\n", pCert->iKeyExpSize));
    NetPrintf(("protossl: keymod size: %d\n", pCert->iKeyModSize));
    NetPrintf(("protossl: sigtype: %d\n", pCert->iSigType));
    NetPrintf(("protossl: sigsize: %d\n", pCert->iSigSize));
    NetPrintf(("protossl: CertIsCA(%d): pathLenConstraints(%d)\n", pCert->iCertIsCA, pCert->iMaxHeight - 1));
    #endif
}
#else
#define _DebugPrintCert(__pCert, __pMessage)
#endif

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

    #if DEBUG_VAL_CERT
    char strIdent1[256], strIdent2[256];
    _DebugFormatCertIdent(pIdent1, strIdent1, sizeof(strIdent1));
    _DebugFormatCertIdent(pIdent2, strIdent2, sizeof(strIdent2));
    NetPrintf(("protossl: comparing '%s' to '%s'\n", strIdent2, strIdent1));
    #endif

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

/*F**************************************************************************/
/*!
    \Function _WildcardMatchSubdomainNoCase

    \Description
        Perform wildcard case-insensitive string comparison of given input
        strings.  This implementation assumes the first string does not
        include a wildcard character and the second string includes exactly
        zero or one wildcard characters, which must be an asterisk.  The
        wildcard is intentionally limited as per the notes below to only
        match a single domain fragment.

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
static int32_t _WildcardMatchSubdomainNoCase(const char *pString1, const char *pString2)
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
        if ((c2 == '*') && (c1 != '.') && (c1 != '\0'))
        {
            r = _WildcardMatchSubdomainNoCase(pString1, pString2+1);
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
    \Function _SubjectAlternativeMatch

    \Description
        Takes as input a hostname and Subject Alternative Name object,
        returns zero if the hostname matches a SAN object entry and non-zero
        otherwise.

    \Input *pStrHost    - pointer to hostname to compare against
    \Input *pSubject    - pointer to subject alternative object
    \Input iSubjectAltLen - length of subject alternative object

    \Output
        int32_t         - like strcmp()

    \Version 11/01/2012 (jbrookes)
*/
/**************************************************************************F*/
static int32_t _SubjectAlternativeMatch(const char *pStrHost, const uint8_t *pSubject, int32_t iSubjectAltLen)
{
    const uint8_t *pSubjectEnd = pSubject + iSubjectAltLen;
    char strCompare[256], *pStrCompare;
    int32_t iType, iSize;

    // parse the wrapper object
    pSubject = _ParseHeader(pSubject, pSubjectEnd, &iType, &iSize);

    // check against subject alternative strings
    for (pSubjectEnd = pSubject + iSubjectAltLen; pSubject != NULL; pSubject += iSize)
    {
        // parse the object
        pSubject = _ParseHeader(pSubject, pSubjectEnd, &iType, &iSize);

        // if it's not a dnsname object, skip it
        if (iType != ASN_IMPLICIT_TAG+2)
        {
            continue;
        }

        // make a copy of the comparison string
        ds_strsubzcpy(strCompare, sizeof(strCompare), (const char *)pSubject, iSize);

        // skip leading whitespace, if any
        for (pStrCompare = strCompare; ((*pStrCompare > 0) && (*pStrCompare <= ' ')); pStrCompare += 1)
            ;

        // call _WildcardMatchSubdomainNoCase() to do comparison
        if (_WildcardMatchSubdomainNoCase(pStrHost, pStrCompare) == 0)
        {
            return(0);
        }
    }

    return(-1);
}

/*F***************************************************************************/
/*!
    \Function _VerifyPkcs1_5_RSAES

    \Description
        Validate RSAES-PKCS1-v1_5 padding as defined by
        https://tools.ietf.org/html/rfc8017#section-7.2.

    \Input *pData       - data to validate
    \Input iKeySize     - key modulus size, in bytes
    \Input iSecretSize  - premaster secret size, in bytes

    \Output
        int32_t         - zero=invalid, else valid

    \Version 02/18/2014 (jbrookes)
*/
/***************************************************************************F*/
static int32_t _VerifyPkcs1_5_RSAES(const uint8_t *pData, int32_t iKeySize, int32_t iSecretSize)
{
    int32_t iIndex;
    uint8_t bValid = TRUE;

    // validate signature
    if ((pData[0] != 0) || (pData[1] != 2))
    {
        NetPrintf(("protossl: pkcs1 signature invalid\n"));
        bValid = FALSE;
    }
    // validate random padding; must be non-zero
    for (iIndex = 2; iIndex < iKeySize-iSecretSize-1; iIndex += 1)
    {
        if (pData[iIndex] == 0)
        {
            NetPrintf(("protossl: pkcs1 padding validation found zero byte\n"));
            bValid = FALSE;
            break;
        }
    }
    // validate zero byte before premaster key data
    if (pData[iKeySize-iSecretSize-1] != 0)
    {
        NetPrintf(("protossl: pkcs1 premaster key padding not zero\n"));
        bValid = FALSE;
    }
    // return result
    return(bValid);
}

/*F***************************************************************************/
/*!
    \Function _VerifyPkcs1_5_EMSA

    \Description
        As per https://tools.ietf.org/html/rfc5246#section-4.7 validate
        EMSA-PKCS1-v1_5 padding, and return pointer to signature hash if valid.
        Format defined by https://tools.ietf.org/html/rfc8017#section-9.2.

    \Input *pData       - data to validate
    \Input iKeySize     - key modulus size, in bytes
    \Input iSecretSize  - secret size, in bytes

    \Output
        uint8_t *       - pointer to signature data, or NULL if invalid

    \Version 04/27/2017 (jbrookes)
*/
/***************************************************************************F*/
static const uint8_t *_VerifyPkcs1_5_EMSA(const uint8_t *pData, int32_t iKeySize, int32_t iSecretSize)
{
    int32_t iIndex, iSize, iSigSize, iSigType;
    const uint8_t *pObject;

    // validate EMSA signature
    if ((pData[0] != 0) || (pData[1] != 1))
    {
        return(NULL);
    }
    // validate padding; must be 0xff
    for (iIndex = 2; (iIndex < (iKeySize-iSecretSize)) && (pData[iIndex] == 0xff); iIndex += 1)
        ;

    // validate zero byte immediately preceding signature object
    if (pData[iIndex++] != 0)
    {
        return(NULL);
    }

    // asn.1 object validation

    // skip outer object envelope
    if ((pObject = _ParseHeaderType(pData+iIndex, pData+iKeySize, ASN_CONSTRUCT|ASN_TYPE_SEQN, &iSize)) == NULL)
    {
        return(NULL);
    }   
    // skip signature algorithm object envelope
    if ((pObject = _ParseHeaderType(pObject, pData+iKeySize, ASN_CONSTRUCT|ASN_TYPE_SEQN, &iSize)) == NULL)
    {
        return(NULL);
    }

    // get the signature algorithm type
    if ((pObject = _ParseHeaderType(pObject, pData+iKeySize, ASN_TYPE_OBJECT+ASN_PRIMITIVE, &iSize)) == NULL)
    {
        return(NULL);
    }
    if ((iSigType = _ParseObject(pObject, iSize)) == ASN_OBJ_NONE)
    {
        return(NULL);
    }
    pObject += iSize;

    // skip null object
    if ((pObject = _ParseHeaderType(pObject, pData+iKeySize, ASN_TYPE_NULL, &iSize)) == NULL)
    {
        return(NULL);
    }
    // get signature data
    if ((pObject = _ParseHeaderType(pObject, pData+iKeySize, ASN_TYPE_OCTSTRING, &iSigSize)) == NULL)
    {
        return(NULL);
    }
    // validate signature size & total size
    if ((iSigSize != iSecretSize) || ((pObject + iSigSize) != (pData + iKeySize)))
    {
        return(NULL);
    }

    // return pointer to signature hash
    return(pObject);
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

    \Version 07/22/2009 (jbrookes)
*/
/***************************************************************************F*/
static int32_t _VerifySignature(ProtoSSLRefT *pState, X509CertificateT *pCert, const uint8_t *pKeyModData, int32_t iKeyModSize, const uint8_t *pKeyExpData, int32_t iKeyExpSize)
{
    CryptRSAT RSAContext;
    const uint8_t *pSigData;
    int32_t iResult;

    NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: verifying signature\n"));

    // decrypt certificate signature block using CA public key
    if (CryptRSAInit(&RSAContext, pKeyModData, iKeyModSize, pKeyExpData, iKeyExpSize))
    {
        return(-1);
    }
    CryptRSAInitSignature(&RSAContext, pCert->SigData, pCert->iSigSize);
    CryptRSAEncrypt(&RSAContext, 0);

    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (rsa sig) %dms\n", RSAContext.uCryptMsecs));
    if (pState != NULL)
    {
        pState->pSecure->uTimer += RSAContext.uCryptMsecs;
    }

    // validate padding & get signature data
    if ((pSigData = _VerifyPkcs1_5_EMSA(RSAContext.EncryptBlock, pCert->iSigSize, pCert->iHashSize)) != NULL)
    {
        // compare decrypted hash data (from signature block) with precalculated certificate body hash
        iResult = memcmp(pCert->HashData, pSigData, pCert->iHashSize);
    }
    else
    {
        NetPrintf(("protossl: signature padding validation failed\n"));
        iResult = -1;
    }

    return(iResult);
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
        ds_memcpy(&pState->CertInfo.Ident, &pCert->Issuer, sizeof(pState->CertInfo.Ident));
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
            return(-50);
        }
    }
    else
    {
        ProtoSSLCACertT *pCACert;

        #if DIRTYCODE_LOGGING
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

            NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: verifying succeeded\n"));

            // special processing when validating against a GOS CA
            if (pCACert->uFlags & SSL_CACERTFLAG_GOSCA)
            {
                // make sure the domain of the certificate is an EA domain
                if (ds_stricmpwc(pCert->Subject.strCommon, "*.ea.com") && ds_stricmpwc(pCert->Subject.strCommon, "*.easports.com"))
                {
                    return(SSL_ERR_GOSCA_INVALIDUSE);
                }
                // ignore date range validation failure
                if ((pState != NULL) && pState->pSecure->bDateVerifyFailed)
                {
                    NetPrintf(("protossl: ignoring date range validation failure for GOS CA\n"));
                    pState->pSecure->bDateVerifyFailed = FALSE;
                }
            }
            // only CAPROVIDER CAs can be used against gosca
            if (!ds_stricmpwc(pCert->Subject.strCommon, "gosca.ea.com") && !(pCACert->uFlags & SSL_CACERTFLAG_CAPROVIDER))
            {
                return(SSL_ERR_CAPROVIDER_INVALID);
            }

            // if the CA hasn't been verified already, do that now
            if (pCACert->pX509Cert != NULL)
            {
                if ((iResult = _VerifyCertificate(pState, pCACert->pX509Cert, TRUE)) == 0)
                {
                    #if DIRTYCODE_LOGGING
                    char strIdentSubject[512], strIdentIssuer[512];
                    int32_t iVerbose = (pState != NULL) ? pState->iVerbose : 0;
                    NetPrintfVerbose((iVerbose, 0, "protossl: ca (%s) validated by ca (%s)\n", _DebugFormatCertIdent(&pCACert->pX509Cert->Subject, strIdentSubject, sizeof(strIdentSubject)),
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
            return(-51);
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
    ds_memclr(pCACert, iCertSize);

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
        ds_memcpy(pCACert->pX509Cert, pCert, sizeof(*pCert));
    }

    // copy textual identity of this certificate
    // (don't need to save issuer since we already trust this certificate)
    ds_memcpy(&pCACert->Subject, &pCert->Subject, sizeof(pCACert->Subject));

    // copy exponent data
    pCACert->iKeyExpSize = pCert->iKeyExpSize;
    ds_memcpy(pCACert->KeyExpData, pCert->KeyExpData, pCACert->iKeyExpSize);

    // copy modulus data, immediately following header
    pCACert->iKeyModSize = pCert->iKeyModSize;
    pCACert->pKeyModData = (uint8_t *)pCACert + sizeof(*pCACert);
    ds_memcpy((uint8_t *)pCACert->pKeyModData, pCert->KeyModData, pCACert->iKeyModSize);

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
    if ((iResult = Base64Decode((int32_t)(pCertEnd-pCertBeg), (const char *)pCertBeg, NULL)) > 0)
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
        Base64Decode((int32_t)(pCertEnd-pCertBeg), (const char *)pCertBeg, (char *)pCertBuffer);
        pCertBeg = pCertBuffer;
        pCertEnd = pCertBeg+iResult;
    }

    // parse the x.509 certificate onto stack
    if ((iResult = _ParseCertificate(&Cert, pCertBeg, (int32_t)(pCertEnd-pCertBeg))) == 0)
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
        if (((iResult = Base64Decode((int32_t)(pCertEnd-pCertBeg), (const char *)pCertBeg, NULL)) <= 0) || (iResult > _iMaxCertSize))
        {
            break;
        }
        Base64Decode((int32_t)(pCertEnd-pCertBeg), (const char *)pCertBeg, (char *)pCertBuffer);

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

/*
    Session History
*/

/*F********************************************************************************/
/*!
    \Function _SessionHistorySet

    \Description
        Set entry in session history buffer

    \Input *pSessHist   - session history entry to set
    \Input *pPeerAddr   - address of peer session is with
    \Input *pSessionId  - session id of session
    \Input *pMasterSecret- master secret associated with session
    \Input uCurTick     - current tick count

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _SessionHistorySet(SessionHistoryT *pSessHist, struct sockaddr *pPeerAddr, const uint8_t *pSessionId, const uint8_t *pMasterSecret, uint32_t uCurTick)
{
    // update peeraddr if it has changed
    if (SockaddrCompare(&pSessHist->SessionAddr, pPeerAddr))
    {
        NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: session history peer addr changed; updating\n"));
        ds_memcpy(&pSessHist->SessionAddr, pPeerAddr, sizeof(pSessHist->SessionAddr));
    }
    // update sessionid if it has changed
    if (memcmp(pSessHist->SessionId, pSessionId, sizeof(pSessHist->SessionId)))
    {
        NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: session history session id changed; updating\n"));
        ds_memcpy(pSessHist->SessionId, pSessionId, sizeof(pSessHist->SessionId));
    }
    // update master secret if it has changed (if session changed, secret should too)
    if (memcmp(pSessHist->MasterSecret, pMasterSecret, sizeof(pSessHist->MasterSecret)))
    {
        NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: session history master secret changed; updating\n"));
        ds_memcpy(pSessHist->MasterSecret, pMasterSecret, sizeof(pSessHist->MasterSecret));
    }
    // touch access timestamp
    pSessHist->uSessionTick = uCurTick;
}

/*F********************************************************************************/
/*!
    \Function _SessionHistoryGet

    \Description
        Returns pointer to specified session history entry, or NULL if not found.

    \Input *pPeerAddr   - address of peer session is with (may be null)
    \Input *pSessionId  - session id to look for (may be null)

    \Output
        SessionHistoryT * - session history entry, or NULL if not found

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static SessionHistoryT *_SessionHistoryGet(struct sockaddr *pPeerAddr, const uint8_t *pSessionId)
{
    SessionHistoryT *pSessHist, *pSessInfo = NULL;
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    const uint8_t aZeroSessionId[32] = { 0 };
    int32_t iSess;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, session history feature is disabled\n"));
        return(NULL);
    }
    #endif

    // exclude NULL sessionId
    if ((pSessionId != NULL) && !memcmp(pSessionId, aZeroSessionId, sizeof(aZeroSessionId)))
    {
        return(NULL);
    }

    // find session history
    for (iSess = 0; iSess < SSL_SESSHIST_MAX; iSess += 1)
    {
        pSessHist = &pState->SessionHistory[iSess];
        if ((pPeerAddr != NULL) && (!SockaddrCompare(&pSessHist->SessionAddr, pPeerAddr)))
        {
            pSessInfo = pSessHist;
            break;
        }
        if ((pSessionId != NULL) && (!memcmp(pSessHist->SessionId, pSessionId, sizeof(pSessHist->SessionId))))
        {
            pSessInfo = pSessHist;
            break;
        }
    }

    // return session (or null) to caller
    return(pSessInfo);
}

/*F********************************************************************************/
/*!
    \Function _SessionHistoryAdd

    \Description
        Save a new session in the session history buffer

    \Input *pPeerAddr   - address of peer session is with
    \Input *pSessionId  - session id for session
    \Input iSessionLen  - length of session
    \Input pMasterSecret- master secret associated with session

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _SessionHistoryAdd(struct sockaddr *pPeerAddr, const uint8_t *pSessionId, uint32_t iSessionLen, const uint8_t *pMasterSecret)
{
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    SessionHistoryT *pSessHist;
    int32_t iSess, iSessOldest;
    int32_t iTickDiff, iTickDiffMax;
    uint32_t uCurTick = NetTick();

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, session history feature is disabled\n"));
        return;
    }
    #endif

    NetCritEnter(&pState->StateCrit);

    // see if we already have a session for this peer
    for (iSess = 0; iSess < SSL_SESSHIST_MAX; iSess += 1)
    {
        pSessHist = &pState->SessionHistory[iSess];
        // already have this peer in our history?
        if (!SockaddrCompare(&pSessHist->SessionAddr, pPeerAddr))
        {
            NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: updating session history entry %d (addr=%a:%d)\n", iSess, SockaddrInGetAddr(pPeerAddr), SockaddrInGetPort(pPeerAddr)));
            _SessionHistorySet(pSessHist, pPeerAddr, pSessionId, pMasterSecret, uCurTick);
            NetCritLeave(&pState->StateCrit);
            return;
        }
    }

    // try to find an unallocated session
    for (iSess = 0; iSess < SSL_SESSHIST_MAX; iSess += 1)
    {
        pSessHist = &pState->SessionHistory[iSess];
        // empty slot?
        if (SockaddrInGetAddr(&pSessHist->SessionAddr) == 0)
        {
            NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: adding session history entry %d (addr=%a:%d)\n", iSess, SockaddrInGetAddr(pPeerAddr), SockaddrInGetPort(pPeerAddr)));
            _SessionHistorySet(pSessHist, pPeerAddr, pSessionId, pMasterSecret, uCurTick);
            NetCritLeave(&pState->StateCrit);
            return;
        }
    }

    // find the oldest session
    for (iSess = 0, iTickDiffMax = 0, iSessOldest = 0; iSess < SSL_SESSHIST_MAX; iSess += 1)
    {
        pSessHist = &pState->SessionHistory[iSess];
        // find least recently updated session
        if ((iTickDiff = NetTickDiff(uCurTick, pSessHist->uSessionTick)) > iTickDiffMax)
        {
            iTickDiffMax = iTickDiff;
            iSessOldest = iSess;
        }
    }

    NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: replacing session history entry %d (addr=%a:%d)\n", iSessOldest, SockaddrInGetAddr(pPeerAddr), SockaddrInGetPort(pPeerAddr)));
    _SessionHistorySet(&pState->SessionHistory[iSessOldest], pPeerAddr, pSessionId, pMasterSecret, uCurTick);

    NetCritLeave(&pState->StateCrit);
}

/*F********************************************************************************/
/*!
    \Function _SessionHistoryInvalidate

    \Description
        Invalidate specified session from session history cache

    \Input *pSessionId  - session id of session to invalidate

    \Version 02/04/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _SessionHistoryInvalidate(const uint8_t *pSessionId)
{
    SessionHistoryT *pSessInfo = NULL;
    ProtoSSLStateT *pState = _ProtoSSL_pState;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, session history feature is disabled\n"));
        return;
    }
    #endif

    // acquire access to session history
    NetCritEnter(&pState->StateCrit);

    // find session history
    if ((pSessInfo = _SessionHistoryGet(NULL, pSessionId)) != NULL)
    {
        // reset entry
        NetPrintf(("protossl: invalidating session entry\n"));
        ds_memclr(pSessInfo, sizeof(*pSessInfo));
    }

    // release access to session history, return to caller
    NetCritLeave(&pState->StateCrit);
}

/*F********************************************************************************/
/*!
    \Function _SessionHistoryGetInfo

    \Description
        Check to see if we can find a session for the specified address or session
        id in our history buffer, and return sesession info if found.

    \Input *pSessBuff   - [out] storage for session history entry
    \Input *pPeerAddr   - address of peer session is with (may be null)
    \Input *pSessionId  - session id to look for (may be null)

    \Output
        SessionHistoryT * - session history entry, or NULL if not found

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static SessionHistoryT *_SessionHistoryGetInfo(SessionHistoryT *pSessBuff, struct sockaddr *pPeerAddr, const uint8_t *pSessionId)
{
    SessionHistoryT *pSessInfo = NULL;
    ProtoSSLStateT *pState = _ProtoSSL_pState;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, session history feature is disabled\n"));
        return(NULL);
    }
    #endif

    // acquire access to session history
    NetCritEnter(&pState->StateCrit);

    // find session history
    if ((pSessInfo = _SessionHistoryGet(pPeerAddr, pSessionId)) != NULL)
    {
        // update timestamp
        pSessInfo->uSessionTick = NetTick();
        // copy to caller
        ds_memcpy(pSessBuff, pSessInfo, sizeof(*pSessBuff));
        pSessInfo = pSessBuff;
    }

    // release access to session history, return to caller
    NetCritLeave(&pState->StateCrit);
    return(pSessInfo);
}

/*
    Certificate Validation History
*/

/*F********************************************************************************/
/*!
    \Function _CertValidHistorySet

    \Description
        Set entry in certvalid history buffer

    \Input *pCertHist   - session history entry to set
    \Input *pFingerprintId - fingerprint for certificates validated
    \Input uCertSize    - certificate size
    \Input uCurTick     - current tick count

    \Version 04/29/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _CertValidHistorySet(CertValidHistoryT *pCertHist, const uint8_t *pFingerprintId, uint32_t uCertSize, uint32_t uCurTick)
{
    // update fingerprint
    ds_memcpy(pCertHist->FingerprintId, pFingerprintId, sizeof(pCertHist->FingerprintId));
    // save cert size
    pCertHist->uCertSize = uCertSize;
    // set timestamp
    pCertHist->uCertValidTick = uCurTick;
}

/*F********************************************************************************/
/*!
    \Function _CertValidHistoryGet

    \Description
        Returns pointer to specified session history entry, or NULL if not found.

    \Input *pFingerprintId - certificate fingerprint
    \Input uCertSize    - size of certificate to check in validity history

    \Output
        CertValidHistoryT * - certificate validation history entry, or NULL if not found

    \Version 04/29/2014 (jbrookes)
*/
/********************************************************************************F*/
static CertValidHistoryT *_CertValidHistoryGet(const uint8_t *pFingerprintId, uint32_t uCertSize)
{
    CertValidHistoryT *pCertHist, *pCertInfo;
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    const uint8_t aZeroFingerprintId[SSL_FINGERPRINT_SIZE] = { 0 };
    uint32_t uCurTick = NetTick();
    int32_t iCert;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, certificate validation history feature is disabled\n"));
        return(NULL);
    }
    #endif

    // exclude NULL fingerprintId request
    if ((pFingerprintId == NULL) || !memcmp(pFingerprintId, aZeroFingerprintId, sizeof(aZeroFingerprintId)))
    {
        return(NULL);
    }

    // find certificate history
    for (iCert = 0, pCertInfo = NULL; iCert < SSL_CERTVALID_MAX; iCert += 1)
    {
        // ref history entry
        pCertHist = &pState->CertValidHistory[iCert];
        // skip null entries
        if (pCertHist->uCertSize == 0)
        {
            continue;
        }
        // check for expiration
        if (NetTickDiff(uCurTick, pCertHist->uCertValidTick) > SSL_CERTVALIDTIME_MAX)
        {
            NetPrintf(("protossl: expiring certificate validity for entry %d\n", iCert));
            ds_memclr(pCertHist, sizeof(*pCertHist));
            continue;
        }
        // check for match
        if ((pCertHist->uCertSize == uCertSize) && (!memcmp(pCertHist->FingerprintId, pFingerprintId, sizeof(pCertHist->FingerprintId))))
        {
            pCertInfo = pCertHist;
            break;
        }
    }

    // return session (or null) to caller
    return(pCertInfo);
}

/*F********************************************************************************/
/*!
    \Function _CertValidHistoryAdd

    \Description
        Added fingerprint of validated certificate to certificate validity history buffer

    \Input *pFingerprintId  - certificate fingerprint
    \Input uCertSize        - size of certificate

    \Version 03/15/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _CertValidHistoryAdd(const uint8_t *pFingerprintId, uint32_t uCertSize)
{
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    CertValidHistoryT *pCertHist;
    int32_t iCert, iCertOldest;
    int32_t iTickDiff, iTickDiffMax;
    uint32_t uCurTick = NetTick();

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, certificate validation history feature is disabled\n"));
        return;
    }
    #endif

    NetCritEnter(&pState->StateCrit);

    // try to find an unallocated session
    for (iCert = 0; iCert < SSL_CERTVALID_MAX; iCert += 1)
    {
        pCertHist = &pState->CertValidHistory[iCert];
        // empty slot?
        if (pCertHist->uCertSize == 0)
        {
            #if DEBUG_VAL_CERT
            NetPrintf(("protossl: adding certificate validation history entry %d\n", iCert));
            #endif
            _CertValidHistorySet(pCertHist, pFingerprintId, uCertSize, uCurTick);
            NetCritLeave(&pState->StateCrit);
            return;
        }
    }

    // find the oldest session
    for (iCert = 0, iTickDiffMax = 0, iCertOldest = 0; iCert < SSL_CERTVALID_MAX; iCert += 1)
    {
        pCertHist = &pState->CertValidHistory[iCert];
        // find least recently updated session
        if ((iTickDiff = NetTickDiff(uCurTick, pCertHist->uCertValidTick)) > iTickDiffMax)
        {
            iTickDiffMax = iTickDiff;
            iCertOldest = iCert;
        }
    }

    NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: replacing certificate validation entry %d\n", iCertOldest));
    _CertValidHistorySet(&pState->CertValidHistory[iCertOldest], pFingerprintId, uCertSize, uCurTick);

    NetCritLeave(&pState->StateCrit);
}

/*F********************************************************************************/
/*!
    \Function _CertValidHistoryCheck

    \Description
        Check to see if certificate fingerprint is in certificiate validity history

    \Input *pFingerprintId  - certificate fingerprint
    \Input uCertSize        - size of certificate

    \Output
        int32_t             - zero=not validated, else validated

    \Version 04/29/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CertValidHistoryCheck(const uint8_t *pFingerprintId, uint32_t uCertSize)
{
    ProtoSSLStateT *pState = _ProtoSSL_pState;
    CertValidHistoryT *pCertInfo;
    int32_t iResult = 0;

    #if DIRTYCODE_DEBUG
    if (pState == NULL)
    {
        NetPrintf(("protossl: warning, protossl not initialized, certificate validation history feature is disabled\n"));
        return(0);
    }
    #endif

    // acquire access to session history
    NetCritEnter(&pState->StateCrit);

    // find certificate validity history
    if ((pCertInfo = _CertValidHistoryGet(pFingerprintId, uCertSize)) != NULL)
    {
        iResult = 1;
        NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: certificate validation deferred to cache\n"));
    }

    // release access to session history, return to caller
    NetCritLeave(&pState->StateCrit);
    return(iResult);
}


/*
    MAC/HMAC functions, building keys, finish hashes
*/

/*F**************************************************************************/
/*!
    \Function _ProtoSSLGenerateMac

    \Description
        Build the MAC (Message Authentication Code) source data

    \Input *pBuffer - [out] storage for generated MAC
    \Input uSeqn    - sequence number
    \Input uType    - message type
    \Input uSslVers - ssl version
    \Input uSize    - data size

    \Output
        uint8_t *   - pointer past end of generated MAC

    \Version 10/11/2012 (jbrookes) Refactored and added TLS support
*/
/**************************************************************************F*/
static uint8_t *_ProtoSSLGenerateMac(uint8_t *pBuffer, uint32_t uSeqn, uint32_t uType, uint32_t uSslVers, uint32_t uSize)
{
    *pBuffer++ = 0;
    *pBuffer++ = 0;
    *pBuffer++ = 0;
    *pBuffer++ = 0;
    *pBuffer++ = (uint8_t)((uSeqn>>24)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>16)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>8)&255);
    *pBuffer++ = (uint8_t)((uSeqn>>0)&255);
    *pBuffer++ = (uint8_t)uType;
    if (uSslVers >= SSL3_TLS1_0)
    {
        *pBuffer++ = (uint8_t)(uSslVers>>8);
        *pBuffer++ = (uint8_t)(uSslVers>>0);
    }
    *pBuffer++ = (uint8_t)(uSize>>8);
    *pBuffer++ = (uint8_t)(uSize>>0);
    return(pBuffer);
}

/*F**************************************************************************/
/*!
    \Function _ProtoSSLGenerateNonce

    \Description
        Generate AEAD Nonce (IV)

    \Input *pBuffer     - [out] storage for generated nonce
    \Input uSeqn        - sequence number
    \Input *pSeqn       - sequence pointer (NULL to use sequence number)
    \Input *pInitVec    - implicit initialization vector (IV)

    \Output
        uint8_t *       - pointer past end of generated nonce

    \Version 07/08/2014 (jbrookes)
*/
/**************************************************************************F*/
static uint8_t *_ProtoSSLGenerateNonce(uint8_t *pBuffer, uint32_t uSeqn, const uint8_t *pSeqn, const uint8_t *pInitVec)
{
    *pBuffer++ = *pInitVec++;
    *pBuffer++ = *pInitVec++;
    *pBuffer++ = *pInitVec++;
    *pBuffer++ = *pInitVec++;
    if (pSeqn == NULL)
    {
        *pBuffer++ = 0;
        *pBuffer++ = 0;
        *pBuffer++ = 0;
        *pBuffer++ = 0;
        *pBuffer++ = (uint8_t)((uSeqn>>24)&255);
        *pBuffer++ = (uint8_t)((uSeqn>>16)&255);
        *pBuffer++ = (uint8_t)((uSeqn>>8)&255);
        *pBuffer++ = (uint8_t)((uSeqn>>0)&255);
    }
    else
    {
        ds_memcpy(pBuffer, pSeqn, 8);
        pBuffer += 8;
    }
    return(pBuffer);
}

/*F**************************************************************************/
/*!
    \Function _ProtoSSLDoMac

    \Description
        Implement MAC as defined in http://tools.ietf.org/html/rfc6101#section-5.2.3

    \Input *pBuffer     - [out] storage for generated MAC
    \Input iBufLen      - size of output
    \Input *pMessage    - input data to hash
    \Input iMessageLen  - size of input data
    \Input *pMessage2   - more input data to hash
    \Input iMessageLen2 - size of more input data
    \Input pKey         - seed
    \Input iKeyLen      - seed length
    \Input eHashType    - hash type

    \Version 10/11/2012 (jbrookes) Refactored
*/
/**************************************************************************F*/
static void _ProtoSSLDoMac(uint8_t *pBuffer, int32_t iBufLen, const uint8_t *pMessage, int32_t iMessageLen, const uint8_t *pMessage2, int32_t iMessageLen2, const uint8_t *pKey, int32_t iKeyLen, CryptHashTypeE eHashType)
{
    uint8_t aWorkBuf[CRYPTHASH_MAXDIGEST];
    uint8_t aContext[CRYPTHASH_MAXSTATE];
    const CryptHashT *pHash;
    int32_t iPadLen;

    // get hash function block for this hash type
    if ((pHash = CryptHashGet(eHashType)) == NULL)
    {
        NetPrintf(("protossl: invalid hash type %d\n", eHashType));
        return;
    }
    iPadLen = (pHash->iHashSize == SSL3_MAC_MD5) ? 48 : 40;

    // calc first part of mac
    pHash->Init(aContext, pHash->iHashSize);
    pHash->Update(aContext, pKey, iKeyLen);
    pHash->Update(aContext, _SSL3_Pad1, iPadLen);
    pHash->Update(aContext, pMessage, iMessageLen);
    if (pMessage2 != NULL)
    {
        pHash->Update(aContext, pMessage2, iMessageLen2);
    }
    pHash->Final(aContext, aWorkBuf, pHash->iHashSize);

    // second part of mac
    pHash->Init(aContext, pHash->iHashSize);
    pHash->Update(aContext, pKey, iKeyLen);
    pHash->Update(aContext, _SSL3_Pad2, iPadLen);
    pHash->Update(aContext, aWorkBuf, pHash->iHashSize);
    pHash->Final(aContext, pBuffer, iBufLen);
}

/*F**************************************************************************/
/*!
    \Function _ProtoSSLDoHmac

    \Description
        Calculates HMAC using CryptHmac

    \Input *pBuffer     - [out] storage for generated MAC
    \Input iBufLen      - size of output
    \Input *pMessage    - input data to hash
    \Input iMessageLen  - size of input data
    \Input *pMessage2   - more input data to hash (optional)
    \Input iMessageLen2 - size of more input data (if pMessage2 != NULL)
    \Input pKey         - seed
    \Input iKeyLen      - seed length
    \Input eHashType    - hash type

    \Version 10/11/2012 (jbrookes)
*/
/**************************************************************************F*/
static void _ProtoSSLDoHmac(uint8_t *pBuffer, int32_t iBufLen, const uint8_t *pMessage, int32_t iMessageLen, const uint8_t *pMessage2, int32_t iMessageLen2, const uint8_t *pKey, int32_t iKeyLen, CryptHashTypeE eHashType)
{
    if (pMessage2 != NULL)
    {
        CryptHmacMsgT Message[2];
        Message[0].pMessage = pMessage;
        Message[0].iMessageLen = iMessageLen;
        Message[1].pMessage = pMessage2;
        Message[1].iMessageLen = iMessageLen2;
        CryptHmacCalcMulti(pBuffer, iBufLen, Message, 2, pKey, iKeyLen, eHashType);
    }
    else
    {
        CryptHmacCalc(pBuffer, iBufLen, pMessage, iMessageLen, pKey, iKeyLen, eHashType);
    }
}

/*F**************************************************************************/
/*!
    \Function _ProtoSSLDoPHash

    \Description
        Implements P_hash as defined in http://tools.ietf.org/html/rfc4346#section-5

    \Input *pBuffer     - [out] storage for P_hash result
    \Input iOutLen      - size of output expected
    \Input *pSecret     - secret
    \Input iSecretLen   - length of secret
    \Input *pSeed       - seed
    \Input iSeedLen     - length of seed
    \Input eHashType    - hash type

    \Version 10/11/2012 (jbrookes)
*/
/**************************************************************************F*/
static void _ProtoSSLDoPHash(uint8_t *pBuffer, int32_t iOutLen, const uint8_t *pSecret, int32_t iSecretLen, const uint8_t *pSeed, int32_t iSeedLen, CryptHashTypeE eHashType)
{
    uint8_t aWork[128];
    int32_t iHashSize = CryptHashGetSize(eHashType);

    // A(1)
    _ProtoSSLDoHmac(aWork, sizeof(aWork), pSeed, iSeedLen, NULL, 0, pSecret, iSecretLen, eHashType);
    ds_memcpy(aWork+iHashSize, pSeed, iSeedLen);
    _ProtoSSLDoHmac(pBuffer, iOutLen, aWork, iSeedLen+iHashSize, NULL, 0, pSecret, iSecretLen, eHashType);

    // A(n)
    while (iOutLen > iHashSize)
    {
        uint8_t aWork2[128];

        pBuffer += iHashSize;
        iOutLen -= iHashSize;

        _ProtoSSLDoHmac(aWork2, sizeof(aWork2), aWork, iHashSize, NULL, 0, pSecret, iSecretLen, eHashType);
        ds_memcpy(aWork, aWork2, iHashSize);
        _ProtoSSLDoHmac(pBuffer, iOutLen, aWork, iSeedLen+iHashSize, NULL, 0, pSecret, iSecretLen, eHashType);
    }
}

/*F**************************************************************************/
/*!
    \Function _ProtoSSLDoPRF

    \Description
        Implements PRF as defined in http://tools.ietf.org/html/rfc4346#section-5

    \Input *pBuffer     - [out] storage for P_hash result
    \Input iOutLen      - size of output expected
    \Input *pSecret     - secret
    \Input iSecretLen   - length of secret
    \Input *pSeed       - seed
    \Input iSeedLen     - length of seed

    \Version 10/11/2012 (jbrookes)
*/
/**************************************************************************F*/
static void _ProtoSSLDoPRF(uint8_t *pBuffer, int32_t iOutLen, const uint8_t *pSecret, int32_t iSecretLen, const uint8_t *pSeed, int32_t iSeedLen)
{
    uint8_t aMD5Buf[256], aSHABuf[256];
    int32_t iLS = iSecretLen/2;         // split secret in two
    int32_t iBufCnt;
    iLS += iSecretLen & 1;              // handle odd secret lengths

    // execute md5 p_hash
    _ProtoSSLDoPHash(aMD5Buf, iOutLen, pSecret, iLS, pSeed, iSeedLen, CRYPTHASH_MD5);

    // execute sha1 p_hash
    _ProtoSSLDoPHash(aSHABuf, iOutLen, pSecret+iLS, iLS, pSeed, iSeedLen, CRYPTHASH_SHA1);

    // execute XOR of MD5 and SHA hashes
    for (iBufCnt = 0; iBufCnt < iOutLen; iBufCnt += 1)
    {
        pBuffer[iBufCnt] = aMD5Buf[iBufCnt] ^ aSHABuf[iBufCnt];
    }
}

/*F**************************************************************************/
/*!
    \Function _ProtoSSLBuildKey

    \Description
        Builds key material/master secret as appropriate for SSLv3/TLS1

    \Input *pSecure     - secure state
    \Input *pBuffer     - [out] output for generated key data
    \Input iBufSize     - size of output buffer
    \Input *pSource     - source data
    \Input iSourceLen   - length of source data
    \Input *pRandomA    - random data
    \Input *pRandomB    - random data
    \Input iRandomLen   - length of random data
    \Input *pLabel      - label (TLS1 PRF only)
    \Input uSslVersion  - ssl version being used

    \Version 10/11/2012 (jbrookes)
*/
/**************************************************************************F*/
static void _ProtoSSLBuildKey(SecureStateT *pSecure, uint8_t *pBuffer, int32_t iBufSize, uint8_t *pSource, int32_t iSourceLen, uint8_t *pRandomA, uint8_t *pRandomB, int32_t iRandomLen, const char *pLabel, uint16_t uSslVersion)
{
    if (uSslVersion < SSL3_TLS1_0)
    {
        CryptMD5T MD5Context;
        CryptSha1T SHA1Context;
        int32_t iLoop;
        uint8_t strBody[256];

        // SSLv3: build the key material (see http://tools.ietf.org/html/rfc6101#section-6.2.2)
        for (iLoop = 0; iLoop < iBufSize/16; iLoop += 1)
        {
            CryptMD5Init(&MD5Context);
            CryptMD5Update(&MD5Context, pSource, iSourceLen);
            CryptSha1Init(&SHA1Context);
            ds_memset(strBody, 'A'+iLoop, iLoop+1);
            CryptSha1Update(&SHA1Context, strBody, iLoop+1);
            CryptSha1Update(&SHA1Context, pSource, iSourceLen);
            CryptSha1Update(&SHA1Context, pRandomA, iRandomLen);
            CryptSha1Update(&SHA1Context, pRandomB, iRandomLen);
            CryptSha1Final(&SHA1Context, strBody, 20);
            CryptMD5Update(&MD5Context, strBody, 20);
            CryptMD5Final(&MD5Context, pBuffer+16*iLoop, 16);
        }
    }
    else
    {
        // TLS1: build the master secret (see http://tools.ietf.org/html/rfc2246#section-6.3)
        uint8_t aWork[128]; // must hold at least 13+32+32 bytes
        ds_strnzcpy((char *)aWork, pLabel, sizeof(aWork));
        ds_memcpy(aWork+13, pRandomA, iRandomLen);
        ds_memcpy(aWork+45, pRandomB, iRandomLen);
        if (uSslVersion < SSL3_TLS1_2)
        {
            _ProtoSSLDoPRF(pBuffer, iBufSize, pSource, iSourceLen, aWork, 77);
        }
        else
        {
            _ProtoSSLDoPHash(pBuffer, iBufSize, pSource, iSourceLen, aWork, 77, (CryptHashTypeE)pSecure->pCipher->uPrfType);
        }
    }
}

/*F**************************************************************************/
/*!
    \Function _ProtoSSLDoFinishHash

    \Description
        Generates finish hash data as appropriate for SSLv3/TLS1

    \Input *pBuffer     - [out] storage for generated finish hash
    \Input *pSecure     - secure state
    \Input *pLabelSSL   - label to use for SSLv3 finish hash calculation
    \Input *pLabelTLS   - label to use for TLS1 finish hash calculation

    \Output
        int32_t         - finish hash size

    \Version 10/11/2012 (jbrookes)
*/
/**************************************************************************F*/
static int32_t _ProtoSSLDoFinishHash(uint8_t *pBuffer, SecureStateT *pSecure, const char *pLabelSSL, const char *pLabelTLS)
{
    CryptMD5T MD5Context;
    CryptSha1T SHAContext;
    int32_t iHashSize;

    if (pSecure->uSslVersion < SSL3_TLS1_0)
    {
        uint8_t MacTemp[20];

        // setup the finish verification hashes as per http://tools.ietf.org/html/rfc6101#section-5.6.9
        ds_memcpy(&MD5Context, &pSecure->HandshakeMD5, sizeof(MD5Context));
        CryptMD5Update(&MD5Context, pLabelSSL, 4);
        CryptMD5Update(&MD5Context, pSecure->MasterKey, sizeof(pSecure->MasterKey));
        CryptMD5Update(&MD5Context, _SSL3_Pad1, 48);
        CryptMD5Final(&MD5Context, MacTemp, 16);

        CryptMD5Init(&MD5Context);
        CryptMD5Update(&MD5Context, pSecure->MasterKey, sizeof(pSecure->MasterKey));
        CryptMD5Update(&MD5Context, _SSL3_Pad2, 48);
        CryptMD5Update(&MD5Context, MacTemp, 16);
        CryptMD5Final(&MD5Context, pBuffer+0, 16);
        #if DEBUG_RAW_DATA
        NetPrintMem(pBuffer+0, 16, "HS finish MD5");
        #endif

        ds_memcpy(&SHAContext, &pSecure->HandshakeSHA, sizeof(SHAContext));
        CryptSha1Update(&SHAContext, (const uint8_t *)pLabelSSL, 4);
        CryptSha1Update(&SHAContext, pSecure->MasterKey, sizeof(pSecure->MasterKey));
        CryptSha1Update(&SHAContext, _SSL3_Pad1, 40);
        CryptSha1Final(&SHAContext, MacTemp, 20);

        CryptSha1Init(&SHAContext);
        CryptSha1Update(&SHAContext, pSecure->MasterKey, sizeof(pSecure->MasterKey));
        CryptSha1Update(&SHAContext, _SSL3_Pad2, 40);
        CryptSha1Update(&SHAContext, MacTemp, 20);
        CryptSha1Final(&SHAContext, pBuffer+16, 20);
        #if DEBUG_RAW_DATA
        NetPrintMem(pBuffer+16, 20, "HS finish SHA");
        #endif
        iHashSize = 36;
    }
    else
    {
        uint8_t aMacTemp[128];
        iHashSize = 12;
        ds_strnzcpy((char *)aMacTemp, pLabelTLS, sizeof(aMacTemp));

        if (pSecure->uSslVersion < SSL3_TLS1_2)
        {
            ds_memcpy(&MD5Context, &pSecure->HandshakeMD5, sizeof(MD5Context));
            CryptMD5Final(&MD5Context, aMacTemp+15, SSL3_MAC_MD5);
            ds_memcpy(&SHAContext, &pSecure->HandshakeSHA, sizeof(SHAContext));
            CryptSha1Final(&SHAContext, aMacTemp+31, SSL3_MAC_SHA);
            _ProtoSSLDoPRF(pBuffer, iHashSize, pSecure->MasterKey, sizeof(pSecure->MasterKey), aMacTemp, 51);
        }
        else if (pSecure->pCipher->uPrfType == SSL3_PRF_SHA256)
        {
            CryptSha2T SHA2Context;
            ds_memcpy(&SHA2Context, &pSecure->HandshakeSHA256, sizeof(SHA2Context));
            CryptSha2Final(&SHA2Context, aMacTemp+15, SSL3_MAC_SHA256);
            _ProtoSSLDoPHash(pBuffer, iHashSize, pSecure->MasterKey, sizeof(pSecure->MasterKey), aMacTemp, 15+SSL3_MAC_SHA256, CRYPTHASH_SHA256);
        }
        else
        {
            CryptSha2T SHA2Context;
            ds_memcpy(&SHA2Context, &pSecure->HandshakeSHA384, sizeof(SHA2Context));
            CryptSha2Final(&SHA2Context, aMacTemp+15, SSL3_MAC_SHA384);
            _ProtoSSLDoPHash(pBuffer, iHashSize, pSecure->MasterKey, sizeof(pSecure->MasterKey), aMacTemp, 15+SSL3_MAC_SHA384, CRYPTHASH_SHA384);
        }
    }

    return(iHashSize);
}

/*F**************************************************************************/
/*!
    \Function _ProtoSSLBuildKeyMaterial

    \Description
        Build and distribute key material from master secret, server
        random, and client random.

    \Input *pState      - module state reference

    \Version 03/19/2012 (jbrookes)
*/
/**************************************************************************F*/
static void _ProtoSSLBuildKeyMaterial(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t *pData;

    #if DEBUG_RAW_DATA
    NetPrintf(("protossl: building key material\n"));
    NetPrintMem(pSecure->MasterKey, sizeof(pSecure->MasterKey), "MasterKey");
    NetPrintMem(pSecure->ServerRandom, sizeof(pSecure->ServerRandom), "ServerRandom");
    NetPrintMem(pSecure->ClientRandom, sizeof(pSecure->ClientRandom), "ClientRandom");
    #endif

    /// build key material
    _ProtoSSLBuildKey(pSecure, pSecure->KeyBlock, sizeof(pSecure->KeyBlock), pSecure->MasterKey, sizeof(pSecure->MasterKey),
        pSecure->ServerRandom, pSecure->ClientRandom, sizeof(pSecure->ServerRandom), "key expansion",
        pSecure->uSslVersion);

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->KeyBlock, sizeof(pSecure->KeyBlock), "KeyExpansion");
    #endif

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
    pData += pSecure->pCipher->uVecSize;
    pSecure->pServerInitVec = pData;
    pData += pSecure->pCipher->uVecSize;
}

/*
    Alert handling, Packet send/recv
*/

/*F**************************************************************************/
/*!
    \Function _GetAlert

    \Description
        Get alert from table based on alert type

    \Input *pState          - module state
    \Input *pAlertDesc      - [out] storage for alert description
    \Input uAlertLevel      - alert level
    \Input uAlertType       - alert type

    \Output
        int32_t             - 0=no alert, 1=alert

    \Version 10/31/2013 (jbrookes)
*/
/**************************************************************************F*/
static int32_t _GetAlert(ProtoSSLRefT *pState, ProtoSSLAlertDescT *pAlertDesc, uint8_t uAlertLevel, uint8_t uAlertType)
{
    int32_t iErr, iResult = 0;

    // set up default alertdesc
    pAlertDesc->iAlertType = uAlertType;
    pAlertDesc->pAlertDesc = "unknown";

    // find alert description
    if (uAlertLevel != 0)
    {
        for (iErr = 0; _ProtoSSL_AlertList[iErr].iAlertType != -1; iErr += 1)
        {
            if (_ProtoSSL_AlertList[iErr].iAlertType == uAlertType)
            {
                pAlertDesc->pAlertDesc = _ProtoSSL_AlertList[iErr].pAlertDesc;
                iResult = 1;
                break;
            }
        }
    }
    return(iResult);
}

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
    ProtoSSLAlertDescT Alert;
    _GetAlert(pState, &Alert, uAlertLevel, uAlertType);
    NetPrintf(("protossl: ALERT: level=%d, type=%d, name=%s\n", uAlertLevel, uAlertType, Alert.pAlertDesc));
}
#else
#define _DebugAlert(_pState, _uAlertLevel, _uAlertType)
#endif

/*F**************************************************************************/
/*!
    \Function _SendSecure

    \Description
        Send secure queued data

    \Input *pState      - module state
    \Input *pSecure     - secure state

    \Output
        int32_t         - zero if nothing was sent, else one

    \Version 11/07/2013 (jbrookes) Refactored from _ProtoSSLUpdateSend()
*/
/**************************************************************************F*/
static int32_t _SendSecure(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    int32_t iResult, iXfer = 0;

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
        if (iResult < 0)
        {
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
            pState->iClosed = 1;
        }
        // see if the data can be cleared
        if (pSecure->iSendProg == pSecure->iSendSize)
        {
            pSecure->iSendProg = pSecure->iSendSize = 0;
        }
    }

    // return if something was sent
    return(iXfer);
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
    pSecure->SendData[1] = (uint8_t)(pSecure->uSslVersion>>8);
    pSecure->SendData[2] = (uint8_t)(pSecure->uSslVersion>>0);

    // point to data area
    pSend = pSecure->SendData+5;
    iSize = 0;

    /* reserve space for explicit IV if we're TLS1.1 or greater and are using AES (block cipher).  reserving
       space here means we don't have to shuffle packet data around later.  note that the IV is *not* included
       in calculating the handshake data for the finish packet */
    if (pSecure->bSendSecure && (pSecure->pCipher != NULL) && (pSecure->pCipher->uEnc == SSL3_ENC_AES) && (pSecure->uSslVersion >= SSL3_TLS1_1))
    {
        pSend += 16;
    }
    // reserve space for explicit IV for GCM
    if (pSecure->bSendSecure && (pSecure->pCipher != NULL) && (pSecure->pCipher->uEnc == SSL3_ENC_GCM))
    {
        pSend += 8;
    }

    // copy over the head
    ds_memcpy(pSend+iSize, pHeadPtr, iHeadLen);
    iSize += iHeadLen;

    // copy over the body
    ds_memcpy(pSend+iSize, pBodyPtr, iBodyLen);
    iSize += iBodyLen;

    // hash handshake data for "finish" packet
    if (uType == SSL3_REC_HANDSHAKE)
    {
        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _SendPacket handshake update (size=%d)\n", iSize));
        CryptMD5Update(&pSecure->HandshakeMD5, pSend, iSize);
        CryptSha1Update(&pSecure->HandshakeSHA, pSend, iSize);
        CryptSha2Update(&pSecure->HandshakeSHA256, pSend, iSize);
        CryptSha2Update(&pSecure->HandshakeSHA384, pSend, iSize);
    }

    // handle encryption
    if (pSecure->bSendSecure && (pSecure->pCipher != NULL))
    {
        if (pSecure->pCipher->uMacType != SSL3_MAC_NULL)
        {
            uint8_t MacTemp[CRYPTHASH_MAXDIGEST], *pMacTemp;

            // generate the mac source
            pMacTemp = _ProtoSSLGenerateMac(MacTemp, pSecure->uSendSeqn, pSecure->SendData[0], pSecure->uSslVersion, (uint32_t)iSize);

            // hash the mac
            if (pSecure->uSslVersion < SSL3_TLS1_0)
            {
                _ProtoSSLDoMac(pSend+iSize, pSecure->pCipher->uMac, MacTemp, (int32_t)(pMacTemp-MacTemp), pSend, iSize,
                    pState->bServer ? pSecure->pServerMAC : pSecure->pClientMAC,
                    pSecure->pCipher->uMac, (CryptHashTypeE)pSecure->pCipher->uMacType);
            }
            else
            {
                _ProtoSSLDoHmac(pSend+iSize, pSecure->pCipher->uMac, MacTemp, (int32_t)(pMacTemp-MacTemp), pSend, iSize,
                    pState->bServer ? pSecure->pServerMAC : pSecure->pClientMAC,
                    pSecure->pCipher->uMac, (CryptHashTypeE)pSecure->pCipher->uMacType);
            }

            iSize += pSecure->pCipher->uMac;
        }

        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _SendPacket (secure enc=%d mac=%d): type=%d, size=%d, seq=%d\n",
            pSecure->pCipher->uEnc, pSecure->pCipher->uMac, pSecure->SendData[0], iSize, pSecure->uSendSeqn));
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->SendData, iSize+5, "_SendPacket");
        #endif

        // encrypt the data
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
            ds_memset(pSend+iSize, iPadBytes-1, iPadBytes);
            iSize += iPadBytes;

            // fill in the explict IV for TLS1.1
            if (pSecure->uSslVersion >= SSL3_TLS1_1)
            {
                pSend -= 16;
                CryptRandGet(pSend, 16);
                #if DEBUG_RAW_DATA
                NetPrintMem(pSend, 16, "explicit IV");
                #endif
                iSize += 16;
            }

            // do the encryption
            CryptAesEncrypt(&pSecure->WriteAes, pSend, iSize);
        }
        if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
        {
            uint8_t AeadData[13], AeadNonce[12], AeadTag[16];

            // generate aead data (13 bytes, matches SSLv3 MAC)
            _ProtoSSLGenerateMac(AeadData, pSecure->uSendSeqn, pSecure->SendData[0], pSecure->uSslVersion, (uint32_t)iSize);

            // generate nonce (4 bytes implicit salt from the IV; 8 bytes explicit, we use sequence)
            _ProtoSSLGenerateNonce(AeadNonce, pSecure->uSendSeqn, NULL, pState->bServer ? pSecure->pServerInitVec : pSecure->pClientInitVec);

            // write explicit IV to output; this is not encrypted
            ds_memcpy(pSend-8, AeadNonce+4, 8);
            #if DEBUG_RAW_DATA
            NetPrintMem(pSend-8, 8, "explicit IV");
            #endif

            // do the encryption
            iSize = CryptGcmEncrypt(&pSecure->WriteGcm, pSend, iSize, AeadNonce, sizeof(AeadNonce), AeadData, sizeof(AeadData), AeadTag, sizeof(AeadTag));

            // append tag to output
            ds_memcpy(pSend+iSize, AeadTag, sizeof(AeadTag));
            iSize += sizeof(AeadTag);

            // add explicit IV to size *after* encrypt+tag
            iSize += 8;
        }
    }
    else
    {
        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _SendPacket (unsecure): type=%d, size=%d, seq=%d\n", pSecure->SendData[0], iSize, pSecure->uSendSeqn));
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

/*F********************************************************************************/
/*!
    \Function _SendAlert

    \Description
        Send an alert

    \Input *pState      - module state reference
    \Input iLevel       - alert level (1=warning, 2=error)
    \Input iValue       - alert value

    \Output
        int32_t         - current state

    \Version 11/06/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SendAlert(ProtoSSLRefT *pState, int32_t iLevel, int32_t iValue)
{
    /* $$TODO: we can currently only send if the send state is empty; this should
       be addressed so we can queue an alert for sending at any time */
    if ((pState->pSecure != NULL) && (pState->pSecure->iSendProg == 0) && (pState->pSecure->iSendSize == 0))
    {
        uint8_t strHead[2];
        #if DIRTYCODE_LOGGING
        ProtoSSLAlertDescT Alert;
        _GetAlert(pState, &Alert, iLevel, iValue);
        #endif
        pState->uAlertLevel = strHead[0] = (uint8_t)iLevel;
        pState->uAlertValue = strHead[1] = (uint8_t)iValue;
        pState->bAlertSent = TRUE;
        NetPrintf(("protossl: sending alert: level=%d type=%s (%d)\n", iLevel, Alert.pAlertDesc, iValue));
        // stage the alert for sending
        _SendPacket(pState, SSL3_REC_ALERT, strHead, sizeof(strHead), NULL, 0);
        // flush alert
        _SendSecure(pState, pState->pSecure);
        /* As per http://tools.ietf.org/html/rfc5246#section-7.2.2, any error alert sent
           or received must result in current session information being reset (no resume) */
        if (iLevel == SSL3_ALERT_LEVEL_FATAL)
        {
            _SessionHistoryInvalidate(pState->pSecure->SessionId);
        }
    }
    else
    {
        NetPrintf(("protossl: unable to send alert (pSecure=%p, iSendProg=%d, iSendSize=%d)\n", pState->pSecure,
            pState->pSecure->iSendProg, pState->pSecure->iSendSize));
    }

    // return current state
    return(pState->iState);
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
    SecureStateT *pSecure = pState->pSecure;
    int32_t iSize = pSecure->iRecvSize-pSecure->iRecvBase;
    uint8_t bBadMac = FALSE;

    /* Check the record type - as per http://tools.ietf.org/html/rfc5246#section-6,
       if a TLS implementation receives an unexpected record type, it MUST send an
       unexpected_message alert */
    if ((pSecure->RecvData[0] < SSL3_REC_CIPHER) || (pSecure->RecvData[0] > SSL3_REC_APPLICATION))
    {
        NetPrintf(("protossl: _RecvPacket: unknown record type %d\n", pSecure->RecvData[0]));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNEXPECTED_MESSAGE);
        return(-1);
    }

    // clear handshake hashed status
    pSecure->bHandshakeHashed = FALSE;

    // if we are reciving the finished packet, we need to transition to secure receiving
    if (pState->iState == ST3_RECV_FINISH)
    {
        pSecure->bRecvSecure = TRUE;
    }

    // handle decryption
    if (pSecure->bRecvSecure && (pSecure->pCipher != NULL))
    {
        // decrypt the data/mac portion
        if (pSecure->pCipher->uEnc == SSL3_ENC_RC4)
        {
            CryptArc4Apply(&pSecure->ReadArc4, pSecure->RecvData+pSecure->iRecvBase, iSize);
        }
        if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
        {
            int32_t iPad, iPadBytes;

            // decrypt the data
            CryptAesDecrypt(&pSecure->ReadAes, pSecure->RecvData+pSecure->iRecvBase, iSize);

            // if TLS1.1 or greater, skip explicit IV
            if ((pSecure->uSslVersion >= SSL3_TLS1_1) && (pSecure->iRecvSize >= 16))
            {
                pSecure->iRecvBase += 16;
                iSize -= 16;
            }

            /* As per http://tools.ietf.org/html/rfc5246#section-6.2.3.2, padding may be up to 255 bytes
               in length.  Each uint8 in the padding data vector MUST be filled with the padding length
               value, padding MUST be checked, and a padding error MUST result in a bad_record_mac
               alert.  To eliminate a possible timing attack, we note the error here but wait until
               after the MAC is generated to report it. */
            for (iPad = 0, iPadBytes = pSecure->RecvData[pSecure->iRecvBase+iSize-1]; iPad < iPadBytes; iPad += 1)
            {
                if (pSecure->RecvData[pSecure->iRecvBase+iSize-iPadBytes+iPad] != iPadBytes)
                {
                    #if DIRTYCODE_LOGGING
                    if (!bBadMac)
                    {
                        NetPrintf(("protossl: _RecvPacket bad padding (len=%d)\n", iPadBytes));
                        NetPrintMem(pSecure->RecvData+pSecure->iRecvBase+iSize-iPadBytes-1, iPadBytes, "_RecvPacket padding");
                    }
                    #endif
                    bBadMac = TRUE;
                    break;
                }
            }

            // remove pad
            iSize -= iPadBytes+1;
        }
        if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
        {
            uint8_t AeadData[13], AeadNonce[12];

            // generate nonce (4 bytes implicit salt from TLS IV; 8 bytes explicit from packet)
            _ProtoSSLGenerateNonce(AeadNonce, 0, pSecure->RecvData+pSecure->iRecvBase, pState->bServer ? pSecure->pClientInitVec : pSecure->pServerInitVec);

            // skip explicit IV in packet data
            pSecure->iRecvBase += 8;

            // remove authentication tag
            pSecure->iRecvSize -= 16;
            pSecure->iRecvProg = pSecure->iRecvSize;

            // recalculate size
            iSize = pSecure->iRecvSize - pSecure->iRecvBase;

            // generate aead data (13 bytes, matches SSLv3 MAC)
            _ProtoSSLGenerateMac(AeadData, pSecure->uRecvSeqn, pSecure->RecvData[0], pSecure->uSslVersion, (uint32_t)iSize);

            // decrypt data
            if ((iSize = CryptGcmDecrypt(&pSecure->ReadGcm, pSecure->RecvData+pSecure->iRecvBase, iSize, AeadNonce, sizeof(AeadNonce), AeadData, sizeof(AeadData), pSecure->RecvData+pSecure->iRecvBase+iSize, 16)) < 0)
            {
                /* as per http://tools.ietf.org/html/rfc5288#section-3: Implementations MUST send TLS Alert bad_record_mac for all types of
                   failures encountered in processing the AES-GCM algorithm. */
                NetPrintf(("protossl: gcm decrypt of received data failed\n"));
                iSize = 0;
                bBadMac = TRUE;
            }
        }

        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _RecvPacket (secure enc=%d mac=%d): type=%d, size=%d, seq=%d\n",
            pSecure->pCipher->uEnc, pSecure->pCipher->uMac, pSecure->RecvData[0], iSize, pSecure->uRecvSeqn));
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->RecvData, pSecure->iRecvSize, "_RecvPacket");
        #endif

        // validate mac
        if (pSecure->pCipher->uMacType != SSL3_MAC_NULL)
        {
            uint8_t MacTemp[CRYPTHASH_MAXDIGEST], *pMacTemp;

            // make sure there is room for mac
            if (iSize >= (int32_t)pSecure->pCipher->uMac)
            {
                iSize -= pSecure->pCipher->uMac;
                // remove the mac from size
                pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase+iSize;
            }
            else
            {
                NetPrintf(("protossl: _RecvPacket: no room for mac (%d < %d)\n", iSize, pSecure->pCipher->uMac));
                iSize = 0;
            }

            // generate MAC source
            pMacTemp = _ProtoSSLGenerateMac(MacTemp, pSecure->uRecvSeqn, pSecure->RecvData[0], pSecure->uSslVersion, (uint32_t)iSize);

            // do the hash
            if (pSecure->uSslVersion < SSL3_TLS1_0)
            {
                _ProtoSSLDoMac(MacTemp, pSecure->pCipher->uMac, MacTemp, (int32_t)(pMacTemp-MacTemp),
                    pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase,
                    pState->bServer ? pSecure->pClientMAC : pSecure->pServerMAC,
                    pSecure->pCipher->uMac, (CryptHashTypeE)pSecure->pCipher->uMacType);
            }
            else // TLS1
            {
                _ProtoSSLDoHmac(MacTemp, pSecure->pCipher->uMac, MacTemp, (int32_t)(pMacTemp-MacTemp),
                    pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase,
                    pState->bServer ? pSecure->pClientMAC : pSecure->pServerMAC,
                    pSecure->pCipher->uMac, (CryptHashTypeE)pSecure->pCipher->uMacType);
            }

            // validate MAC
            if (!bBadMac)
            {
                bBadMac = memcmp(MacTemp, pSecure->RecvData+pSecure->iRecvSize, pSecure->pCipher->uMac) ? TRUE : FALSE;
            }
        }

        // handle mac error (also includes gcm decrypt failure)
        if (bBadMac)
        {
            NetPrintf(("protossl: _RecvPacket: bad MAC!\n"));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_BAD_RECORD_MAC);
            pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = 0;
            return(-2);
        }
    }
    else
    {
        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _RecvPacket (unsecure): type=%d, size=%d, seq=%d\n", pSecure->RecvData[0], iSize, pSecure->uRecvSeqn));
        #if (DEBUG_RAW_DATA > 1)
        NetPrintMem(pSecure->RecvData, pSecure->iRecvSize, "_RecvPacket");
        #endif
    }

    // increment the sequence number
    pSecure->uRecvSeqn += 1;

    /* check for empty packet; some implementations of SSL emit a zero-length frame followed immediately
       by an n-length frame when TLS1.0 is employed with a CBC cipher like AES.  this is done as a defense
       against the BEAST attack.  we need to detect and clear the empty packet here so the following code
       doesn't use the old packet header with new data */
    if (pSecure->iRecvSize == pSecure->iRecvBase)
    {
        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: detected empty packet\n"));
        pSecure->iRecvSize = pSecure->iRecvBase = 0;
    }

    // return success
    return(0);
}

/*
    Handshaking
*/


/*F********************************************************************************/
/*!
    \Function _ProtoSSLAddHelloExtnServerName

    \Description
        Add a Server Name Indication extension to the ClientHello handshake packet;
        ref http://tools.ietf.org/html/rfc6066#page-6

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 10/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAddHelloExtnServerName(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    uint32_t uHostLen = (uint32_t)strlen(pState->strHost);
    uint8_t *pBufStart = pBuffer;

    // make sure we have enough room
    if ((uHostLen+9) < (unsigned)iBufLen)
    {
        // server name extension ident
        *pBuffer++ = 0;
        *pBuffer++ = 0;
        // server name extension length
        *pBuffer++ = (uint8_t)((uHostLen+5) >> 8);
        *pBuffer++ = (uint8_t)(uHostLen+5);

        // server name list length
        *pBuffer++ = (uint8_t)((uHostLen+3) >> 8);
        *pBuffer++ = (uint8_t)(uHostLen+3);
        // type: hostname
        *pBuffer++ = 0;
        // hostname length
        *pBuffer++ = (uint8_t)(uHostLen >> 8);
        *pBuffer++ = (uint8_t)(uHostLen);

        // hostname
        ds_memcpy(pBuffer, pState->strHost, uHostLen);
        pBuffer += (int32_t)uHostLen;
    }

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAddHelloExtnSignatureAlgorithms

    \Description
        Add a Server Name Indication extension to the ClientHello handshake packet;
        ref http://tools.ietf.org/html/rfc5246

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 10/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAddHelloExtnSignatureAlgorithms(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    const int32_t _iListLength = sizeof(_SSL3_SignatureAlgorithms); // signature hash algorithms list length
    const int32_t _iExtnLength = 2+_iListLength; // ident+length+algorithms list length
    uint8_t *pBufStart = pBuffer;

    // make sure we have enough room
    if (_iExtnLength < iBufLen)
    {
        int32_t iIndex;

        // signature hash algorithms extension ident
        *pBuffer++ = (uint8_t)(SSL_SIGNATURE_ALGORITHMS_EXTN >> 8);
        *pBuffer++ = (uint8_t)(SSL_SIGNATURE_ALGORITHMS_EXTN >> 0);
        // signature hash algorithms extension length
        *pBuffer++ = (uint8_t)(_iExtnLength >> 8);
        *pBuffer++ = (uint8_t)(_iExtnLength);
        // signature hash algorithms extension list length
        *pBuffer++ = (uint8_t)(_iListLength >> 8);
        *pBuffer++ = (uint8_t)(_iListLength);
        // supported signature algorithms
        for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_SignatureAlgorithms)/sizeof(*_SSL3_SignatureAlgorithms)); iIndex += 1)
        {
            *pBuffer++ = _SSL3_SignatureAlgorithms[iIndex].uHash;
            *pBuffer++ = _SSL3_SignatureAlgorithms[iIndex].uSignature;
        }
    }

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAddHelloExtnAlpn

    \Description
        Add the Application Level Protocol Negotiation extension to the ClientHello
        and ServerHello handshake packet;
        ref http://tools.ietf.org/html/rfc7301

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 08/03/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAddHelloExtnAlpn(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    uint16_t uAlpnExtensionLength, uExtnLength;
    uint8_t *pBufStart = pBuffer;
    const SecureStateT *pSecure = pState->pSecure;

    // if no protocols, don't write anything into ClientHello
    if ((!pState->bServer) && (pState->uNumAlpnProtocols == 0))
    {
        return(0);
    }
    // if no protocol selected, don't write anything into ServerHello
    else if ((pState->bServer) && ((pSecure == NULL) || (*pSecure->strAlpnProtocol == '\0')))
    {
        return(0);
    }

    // calculate the size of the extension based on if we are client or server
    uAlpnExtensionLength = (!pState->bServer) ? pState->uAlpnExtensionLength : ((uint8_t)strlen(pSecure->strAlpnProtocol) + sizeof(uint8_t));
    uExtnLength = 2+uAlpnExtensionLength; // ident+length+protocol list length

    // make sure we have room
    if (uExtnLength >= iBufLen)
    {
        return(0);
    }

    // extension ident
    *pBuffer++ = (uint8_t)(SSL_ALPN_EXTN >> 8);
    *pBuffer++ = (uint8_t)(SSL_ALPN_EXTN);
    // extension length
    *pBuffer++ = (uint8_t)(uExtnLength >> 8);
    *pBuffer++ = (uint8_t)(uExtnLength);
    // alpn length
    *pBuffer++ = (uint8_t)(pState->uAlpnExtensionLength >> 8);
    *pBuffer++ = (uint8_t)(pState->uAlpnExtensionLength);

    // write the protocols
    if (!pState->bServer)
    {
        int16_t iProtocol;

        // alpn protocols (length + byte string)
        for (iProtocol = 0; iProtocol < pState->uNumAlpnProtocols; iProtocol += 1)
        {
            const AlpnProtocolT *pProtocol = &pState->aAlpnProtocols[iProtocol];

            *pBuffer++ = pProtocol->uLength;
            ds_memcpy(pBuffer, pProtocol->strName, pProtocol->uLength);
            pBuffer += pProtocol->uLength;
        }
    }
    else
    {
        const uint8_t uProtocolLen = uAlpnExtensionLength-(uint8_t)sizeof(uint8_t);

        // alpn protocol
        *pBuffer++ = uProtocolLen;
        ds_memcpy(pBuffer, pSecure->strAlpnProtocol, uProtocolLen);
        pBuffer += uProtocolLen;
    }

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAddHelloExtnEllipticCurves

    \Description
        Add the Elliptic Curves extension to the ClientHello
        ref http://tools.ietf.org/html/rfc4492

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer

    \Output
        int32_t         - number of bytes added to buffer

    \Version 01/19/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLAddHelloExtnEllipticCurves(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen)
{
    const uint16_t uEllipticCurvesLength = sizeof(_SSL3_EllipticCurves[0].uIdent)*(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves));
    const uint16_t uExtnLength = uEllipticCurvesLength+2;
    uint8_t *pBufStart = pBuffer;
    int32_t iEllipticCurve;

    // make sure we have enough room
    if (uExtnLength >= iBufLen)
    {
        return(0);
    }

    // extension ident
    *pBuffer++ = (uint8_t)(SSL_ELLIPTIC_CURVES_EXTN >> 8);
    *pBuffer++ = (uint8_t)(SSL_ELLIPTIC_CURVES_EXTN);
    // extension length
    *pBuffer++ = (uint8_t)(uExtnLength >> 8);
    *pBuffer++ = (uint8_t)(uExtnLength);
    // elliptic curves length
    *pBuffer++ = (uint8_t)(uEllipticCurvesLength >> 8);
    *pBuffer++ = (uint8_t)(uEllipticCurvesLength);
    // supported elliptic curves
    for (iEllipticCurve = 0; iEllipticCurve < (signed)(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves)); iEllipticCurve += 1)
    {
        *pBuffer++ = (uint8_t)(_SSL3_EllipticCurves[iEllipticCurve].uIdent >> 8);
        *pBuffer++ = (uint8_t)(_SSL3_EllipticCurves[iEllipticCurve].uIdent);
    }

    return((int32_t)(pBuffer-pBufStart));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLAddHelloExtensions

    \Description
        Add any ClientHello extensions

    \Input *pState      - module state reference
    \Input *pBuffer     - buffer to write extension into
    \Input iBufLen      - length of buffer
    \Input uHelloExtn   - flag of which hello extensions to include

    \Output
        uint8_t *       - pointer past end of extension chunk

    \Notes
        ClientHello extension registry:
        http://www.iana.org/assignments/tls-extensiontype-values/tls-extensiontype-values.xhtml

    \Version 10/01/2014 (jbrookes)
*/
/********************************************************************************F*/
static uint8_t *_ProtoSSLAddHelloExtensions(ProtoSSLRefT *pState, uint8_t *pBuffer, int32_t iBufLen, uint32_t uHelloExtn)
{
    uint32_t uExtnLen;
    int32_t iBufOff;

    // skip extensions length field
    iBufOff = 2;

    if (!pState->bServer)
    {
        // add enabled extensions (client only)
        if (uHelloExtn & PROTOSSL_HELLOEXTN_SERVERNAME)
        {
            iBufOff += _ProtoSSLAddHelloExtnServerName(pState, pBuffer+iBufOff, iBufLen-iBufOff);
        }
        if (uHelloExtn & PROTOSSL_HELLOEXTN_SIGALGS)
        {
            iBufOff += _ProtoSSLAddHelloExtnSignatureAlgorithms(pState, pBuffer+iBufOff, iBufLen-iBufOff);
        }
        if (uHelloExtn & PROTOSSL_HELLOEXTN_ELLIPTIC_CURVES)
        {
            iBufOff += _ProtoSSLAddHelloExtnEllipticCurves(pState, pBuffer+iBufOff, iBufLen-iBufOff);
        }
    }

    // add enabled extensions (client & server)
    if (uHelloExtn & PROTOSSL_HELLOEXTN_ALPN)
    {
        iBufOff += _ProtoSSLAddHelloExtnAlpn(pState, pBuffer+iBufOff, iBufLen-iBufOff);
    }

    // update extension length
    if (iBufOff > 2)
    {
        uExtnLen = (uint32_t)(iBufOff - 2);
        pBuffer[0] = (uint8_t)(uExtnLen >> 8);
        pBuffer[1] = (uint8_t)(uExtnLen);
    }
    else
    {
        iBufOff = 0;
    }

    // return updated buffer pointer
    return(pBuffer+iBufOff);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendClientHello

    \Description
        Send ClientHello handshake packet; ref http://tools.ietf.org/html/rfc5246#section-7.4.1.2

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_RECV_HELLO)

    \Version 03/15/2013 (jbrookes) Added session resume support
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendClientHello(ProtoSSLRefT *pState)
{
    int32_t iResult, iCiphers;
    uint8_t strHead[4];
    uint8_t strBody[512];
    uint8_t *pData = strBody, *pCiphSize;
    SecureStateT *pSecure = pState->pSecure;
    SessionHistoryT SessHist;
    uint32_t uUtcTime, uHelloExtn = pState->uHelloExtn;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ClientHello\n"));

    // initialize the ssl performance timer
    pSecure->uTimer = 0;

    // reset client certificate level (server will let us know if we need to send one)
    pState->iClientCertLevel = 0;

    // set to the *lowest* version we support
    pSecure->uSslVersion = pState->uSslVersionMin;

    // set requested protocol version (we set the version we want here, which is typically the *highest* version we support)
    pSecure->uSslClientVersion = pState->uSslVersion;
    *pData++ = (uint8_t)(pSecure->uSslClientVersion>>8);
    *pData++ = (uint8_t)(pSecure->uSslClientVersion>>0);

    // set random data (first four bytes of client random are utc time)
    uUtcTime = (uint32_t)ds_timeinsecs();
    pSecure->ClientRandom[0] = (uint8_t)(uUtcTime >> 24);
    pSecure->ClientRandom[1] = (uint8_t)(uUtcTime >> 16);
    pSecure->ClientRandom[2] = (uint8_t)(uUtcTime >> 8);
    pSecure->ClientRandom[3] = (uint8_t)(uUtcTime >> 0);
    CryptRandGet(&pSecure->ClientRandom[4], sizeof(pSecure->ClientRandom)-4);
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->ClientRandom, sizeof(pSecure->ClientRandom), "ClientRandom");
    #endif
    // set client random in packet
    ds_memcpy(pData, pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
    pData += 32;

    // if we have a previous secure session for this peer, set it
    if ((_SessionHistoryGetInfo(&SessHist, &pState->PeerAddr, NULL) != NULL) && pState->bSessionResumeEnabled)
    {
        NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: setting session id for resume\n"));
        #if DEBUG_RES_SESS && DEBUG_RAW_DATA
        NetPrintMem(SessHist.SessionId, sizeof(SessHist.SessionId), "ClientHello session id");
        #endif
        // add sessionid to ClientHello
        *pData++ = (uint8_t)sizeof(SessHist.SessionId);
        ds_memcpy(pData, SessHist.SessionId, sizeof(SessHist.SessionId));
        pData += sizeof(SessHist.SessionId);

        /* save session id and master secret; the sessionid will be compared when we
           receive the serverhello to determine if we are in the resume flow or not */
        ds_memcpy(pSecure->SessionId, SessHist.SessionId, sizeof(pSecure->SessionId));
        ds_memcpy(pSecure->MasterKey, SessHist.MasterSecret, sizeof(pSecure->MasterKey));
    }
    else // add in empty session identifier
    {
        *pData++ = 0;
    }

    // add the cipher suite list
    *pData++ = 0;
    pCiphSize = pData++; // save and skip cipher size location
    for (iResult = 0, iCiphers = 0; iResult < (signed)(sizeof(_SSL3_CipherSuite)/sizeof(_SSL3_CipherSuite[0])); ++iResult)
    {
        // skip ciphers that require a newer version of SSL than we are offering
        if (_SSL3_CipherSuite[iResult].uMinVers > pState->uSslVersion)
        {
            continue;
        }
        // skip disabled ciphers
        if ((_SSL3_CipherSuite[iResult].uId & pState->uEnabledCiphers) == 0)
        {
            continue;
        }
        // add cipher to list
        *pData++ = _SSL3_CipherSuite[iResult].uIdent[0];
        *pData++ = _SSL3_CipherSuite[iResult].uIdent[1];
        iCiphers += 1;
        // add the elliptic curve extension if we are using any ecdhe ciphers
        if (_SSL3_CipherSuite[iResult].uKey == SSL3_KEY_ECDHE)
        {
            uHelloExtn |= PROTOSSL_HELLOEXTN_ELLIPTIC_CURVES;
        }
    }
    // write cipher suite list size
    *pCiphSize = iCiphers*2;

    // add the compress list (we don't support compression)
    *pData++ = 1;
    *pData++ = 0;

    // add zero-length extensions
    if (uHelloExtn != 0)
    {
        pData = _ProtoSSLAddHelloExtensions(pState, pData, (uint32_t)sizeof(strBody)-(uint32_t)(pData-strBody), uHelloExtn);
    }

    // setup the header
    strHead[0] = SSL3_MSG_CLIENT_HELLO;
    strHead[1] = 0;
    strHead[2] = (uint8_t)((pData-strBody)>>8);
    strHead[3] = (uint8_t)((pData-strBody)>>0);

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, (int32_t)(pData-strBody));
    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLParseAlpnExtension

    \Description
        Parse the ALPN extension from the client & server hello

    \Input *pState  - module state reference
    \Input *pData   - the extension data we are parsing

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 08/05/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLParseAlpnExtension(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    uint16_t uAlpnExtensionLength = (pData[0] << 8) | pData[1];
    pData += 2;

    if (uAlpnExtensionLength == 0)
    {
        /* if the extension has data but the list length is zero
           treat this as handshake failure */
        NetPrintf(("protossl: received invalid alpn extension length in client hello\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL);
    }

    if (!pState->bServer)
    {
        // save the negotiated protocol in the secure state so it can be queried (add 1 for nul terminator)
        ds_strnzcpy(pSecure->strAlpnProtocol, (const char *)&pData[1], *pData+1);
        pData += uAlpnExtensionLength;

        NetPrintfVerbose((pState->iVerbose, 0, "protossl: server negotiated protocol %s using alpn extension\n", pSecure->strAlpnProtocol));
    }
    else
    {
        uint16_t uProtocol;

        // skip parsing if the user has not chosed any protocols
        if (pState->uNumAlpnProtocols == 0)
        {
            return(0);
        }

        // loop through the protocols and pick the first supported
        for (uProtocol = 0; uProtocol < pState->uNumAlpnProtocols; uProtocol += 1)
        {
            int16_t iCount = 0;
            const AlpnProtocolT *pProtocol = &pState->aAlpnProtocols[uProtocol];

            while ((iCount < uAlpnExtensionLength) && (*pSecure->strAlpnProtocol == '\0'))
            {
                const uint8_t *pExtensionData = pData+iCount;

                if (memcmp(&pExtensionData[1], pProtocol->strName, pProtocol->uLength) == 0)
                {
                    // save the negotiated protocol (add 1 for nul terminator)
                    ds_strnzcpy(pSecure->strAlpnProtocol, (const char *)&pExtensionData[1], *pExtensionData+1);
                    NetPrintfVerbose((pState->iVerbose, 0, "protossl: negotiated protocol %s using alpn extension\n", pSecure->strAlpnProtocol));
                }

                iCount += *pExtensionData;
                iCount += sizeof(*pExtensionData);
            }
        }

        if (*pSecure->strAlpnProtocol == '\0')
        {
            /* if we did not match a protocol then we do not support any
               of the protocols that the client prefers; treat this as handshake failure */
            NetPrintf(("protossl: client requested protocols that are not supported by the server via alpn extension\n"));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_NO_APPLICATION_PROTOCOL);
            return(ST_FAIL);
        }
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLParseEllipticCurvesExtension

    \Description
        Parse the elliptic curves extension from the client hello

    \Input *pState  - module state reference
    \Input *pData   - the extension data we are parsing

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 01/19/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLParseEllipticCurvesExtension(ProtoSSLRefT *pState, const uint8_t *pData)
{
    int32_t iEllipticCurve, iIndex;
    SecureStateT *pSecure = pState->pSecure;
    uint16_t uEllipticCurveExtensionLength = (pData[0] << 8) | pData[1];
    pData += 2;

    if (uEllipticCurveExtensionLength == 0)
    {
        /* if the extension has data but the list length is zero
           treat this as handshake failure */
        NetPrintf(("protossl: received invalid elliptic curves extension length in client hello\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL);
    }

    // pick first supported elliptic curve
    for (iEllipticCurve = 0, pSecure->pEllipticCurve = NULL; (iEllipticCurve < (signed)(uEllipticCurveExtensionLength/2)) && (pSecure->pEllipticCurve == NULL); iEllipticCurve += 1, pData += 2)
    {
        for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves)); iIndex += 1)
        {
            // skip non-matching elliptic curve
            if ((pData[0] != (uint8_t)(_SSL3_EllipticCurves[iIndex].uIdent >> 8)) || (pData[1] != (uint8_t)(_SSL3_EllipticCurves[iIndex].uIdent)))
            {
                continue;
            }
            // found an elliptic curve
            pSecure->pEllipticCurve = &_SSL3_EllipticCurves[iIndex];
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: using elliptic curve %s (ident=0x%04x)\n", pSecure->pEllipticCurve->strName, pSecure->pEllipticCurve->uIdent));
            break;
        }
    }

    // make sure we found a supported elliptic curve if we are using ECC cipher suite
    if ((pSecure->pCipher->uKey != SSL3_KEY_RSA) && (pSecure->pEllipticCurve == NULL))
    {
        NetPrintf(("protossl: no matching elliptic curve when using ecc cipher suite\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        /* when using ECC cipher suite make sure that we have a suitable elliptic curve.
           we are allowed to try a different cipher suite but I do not believe it is worth
           the hassle of reparsing.
           note: ECDHE-ECDSA is allowed to use a different curve for the ephemeral ECDH key
           in the ServerKeyExchange message */
        return(ST_FAIL_CONN_NOCIPHER);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLParseSignatureAlgorithmsExtension

    \Description
        Parse the signature algorithms supported by the client

    \Input *pState  - module state reference
    \Input *pData   - the extension data we are parsing

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Notes
        This is needed when sending ServerKeyExchange messages to the client

    \Version 03/03/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLParseSignatureAlgorithmsExtension(ProtoSSLRefT *pState, const uint8_t *pData)
{
    int32_t iSignatureAlgorithm, iIndex;
    SecureStateT *pSecure = pState->pSecure;
    uint16_t uSignatureAlgorithmLength = (pData[0] << 8) | pData[1];
    pData += 2;

    if (uSignatureAlgorithmLength == 0)
    {
        /* if the extension has data but the list length is zero
           treat this as handshake failure */
        NetPrintf(("protossl: received invalid signature algorithms length in client hello\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL);
    }

    // pick first supported signature algorithm
    for (iSignatureAlgorithm = 0, pSecure->pSigAlg = NULL; ((iSignatureAlgorithm < (signed)(uSignatureAlgorithmLength/sizeof(*_SSL3_SignatureAlgorithms))) && (pSecure->pSigAlg == NULL)); iSignatureAlgorithm += 1, pData += 2)
    {
        for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_SignatureAlgorithms)/sizeof(*_SSL3_SignatureAlgorithms)); iIndex += 1)
        {
            // skip non-matching signature algorithms
            if ((pData[0] != _SSL3_SignatureAlgorithms[iIndex].uHash) || (pData[1] != _SSL3_SignatureAlgorithms[iIndex].uSignature))
            {
                continue;
            }
            // found a matching signature algorithm
            pSecure->pSigAlg = &_SSL3_SignatureAlgorithms[iIndex];
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: using signature algorithm (hash=0x%02x, sig=0x%02x)\n", pSecure->pSigAlg->uHash, pSecure->pSigAlg->uSignature));
            break;
        }
    }

    /* if we failed to find a matching signature algorithm and the key exchange uses ecdhe we fail
       as we won't be able to match a signature algorithm to sign with */
    if ((pSecure->pCipher->uKey == SSL3_KEY_ECDHE) && (pSecure->pSigAlg == NULL))
    {
        NetPrintf(("protossl: no valid signature algorithm needed for key exchange\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLParseHelloExtensions

    \Description
        Parse the hello extensions in the client & server hello handshake packets

    \Input *pState              - module state reference
    \Input *pExtensionsBegin    - beginning of the extensions
    \Input *pExtensionsEnd      - end of the extensions

    \Output
        int32_t - result of opertion (zero or ST_FAIL_* on error)

    \Version 08/05/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLParseHelloExtensions(ProtoSSLRefT *pState, const uint8_t *pExtensionsBegin, const uint8_t *pExtensionsEnd)
{
    int32_t iResult = 0;
    const uint8_t *pData = pExtensionsBegin;

    /* parse the complete extension section while
       we have not encountered an error */
    while ((pData < pExtensionsEnd) && (iResult == 0))
    {
        uint16_t uExtensionType, uExtensionLength;

        uExtensionType = (pData[0] << 8) | pData[1];
        uExtensionLength = (pData[2] << 8) | pData[3];
        pData += 4;

        // skip extensions without data
        if (uExtensionLength == 0)
        {
            continue;
        }

        if (pState->bServer)
        {
            // parse client only extensions
            if (uExtensionType == SSL_ELLIPTIC_CURVES_EXTN)
            {
                iResult = _ProtoSSLParseEllipticCurvesExtension(pState, pData);
            }
            else if (uExtensionType == SSL_SIGNATURE_ALGORITHMS_EXTN)
            {
                iResult = _ProtoSSLParseSignatureAlgorithmsExtension(pState, pData);
            }
        }

        // parse the individual extensions
        if (uExtensionType == SSL_ALPN_EXTN)
        {
            iResult = _ProtoSSLParseAlpnExtension(pState, pData);
        }

        // skip to next extension
        pData += uExtensionLength;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvClientHello

    \Description
        Process ClientHello handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_SND_HELLO, or ST_FAIL_* on error)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvClientHello(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iIndex, iCipher, iNumCiphers, iParseResult;
    const uint8_t *pExtensionsEnd;
    SessionHistoryT SessHist;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ClientHello\n"));

    // get protocol version client wants from us
    pSecure->uSslClientVersion = pSecure->uSslVersion = (pData[0] << 8) | pData[1];
    if (pSecure->uSslVersion > pState->uSslVersion)
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protossl: client requested SSL version %d.%d, downgrading to our max supported version\n", pData[0], pData[1]));
        pSecure->uSslVersion = pState->uSslVersion;
    }
    else if (pSecure->uSslVersion < pState->uSslVersionMin)
    {
        NetPrintf(("protossl: client requested SSL version %d.%d is not supported\n", pData[0], pData[1]));
        /* As per http://tools.ietf.org/html/rfc5246#appendix-E.1, if server supports (or is willing to use)
           only versions greater than client_version, it MUST send a "protocol_version" alert message and
           close the connection. */
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_PROTOCOL_VERSION);
        return(ST_FAIL_CONN_MINVERS);
    }
    NetPrintfVerbose((pState->iVerbose, 0, "protossl: using %s\n", _SSL3_strVersionNames[pSecure->uSslVersion&0xff]));
    pData += 2;

    // save client random data
    ds_memcpy(pSecure->ClientRandom, pData, sizeof(pSecure->ClientRandom));
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->ClientRandom, sizeof(pSecure->ClientRandom), "ClientRandom");
    #endif
    pData += 32;

    // check for possible session resume
    if ((*pData == sizeof(pSecure->SessionId)) && (_SessionHistoryGetInfo(&SessHist, NULL, pData+1) != NULL) && pState->bSessionResumeEnabled)
    {
        ds_memcpy(pSecure->SessionId, pData+1, sizeof(pSecure->SessionId));
        #if DEBUG_RAW_DATA
        NetPrintMem(pSecure->SessionId, sizeof(pSecure->SessionId), "ClientHello session id");
        #endif
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: resuming previous session\n"));
        // copy session id and master secret
        ds_memcpy(pSecure->MasterKey, SessHist.MasterSecret, sizeof(pSecure->MasterKey));
        ds_memcpy(pSecure->SessionId, SessHist.SessionId, sizeof(pSecure->SessionId));
        // remember we are resuming
        pSecure->bSessionResume = TRUE;
    }
    else // generate a session id for possible future resume
    {
        CryptRandGet(pSecure->SessionId, sizeof(pSecure->SessionId));
        pSecure->bSessionResume = FALSE;
    }
    pData += *pData+1;

    // read cipher suite list size
    iNumCiphers = ((pData[0] << 8) | pData[1]) / 2;
    pData += 2;

    // pick first supported cipher
    for (iCipher = 0, iIndex = 0, pSecure->pCipher = NULL; iCipher < iNumCiphers; iCipher += 1, pData += 2)
    {
        for (iIndex = 0; (iIndex < (signed)(sizeof(_SSL3_CipherSuite)/sizeof(_SSL3_CipherSuite[0]))) && (pSecure->pCipher == NULL); iIndex += 1)
        {
            // skip non-matching ciphers
            if ((pData[0] != _SSL3_CipherSuite[iIndex].uIdent[0]) || (pData[1] != _SSL3_CipherSuite[iIndex].uIdent[1]))
            {
                continue;
            }
            // skip disabled ciphers
            if ((_SSL3_CipherSuite[iIndex].uId & pState->uEnabledCiphers) == 0)
            {
                continue;
            }
            // skip ciphers that require a newer version of SSL than has been negotiated
            if (_SSL3_CipherSuite[iIndex].uMinVers > pState->uSslVersion)
            {
                continue;
            }
            // found a cipher
            pSecure->pCipher = _SSL3_CipherSuite+iIndex;
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: using cipher suite %s (ident=%d,%d)\n", pSecure->pCipher->strName, pData[0], pData[1]));
        }
    }

    // make sure we got one
    if (pSecure->pCipher == NULL)
    {
        NetPrintf(("protossl: no matching cipher\n"));
        /* As per http://tools.ietf.org/html/rfc5246#section-7.4.1.3, a client providing no
           ciphers we support results in a handshake failure alert */
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_CONN_NOCIPHER);
    }

    // skip compression
    pData += *pData+1;

    // find end of extensions section
    pExtensionsEnd = &pData[2] + ((pData[0] << 8) | pData[1]);
    pData += 2;

    // parse the extensions
    if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pExtensionsEnd)) != 0)
    {
        return(iParseResult);
    }

    return(ST3_SEND_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendServerHello

    \Description
        Send ServerHello handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_CHANGE or ST3_SEND_CERT)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendServerHello(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];
    uint8_t strBody[256];
    uint8_t *pData = strBody;
    SecureStateT *pSecure = pState->pSecure;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ServerHello\n"));

    // set negotiated protocol version
    *pData++ = (uint8_t)(pSecure->uSslVersion>>8);
    *pData++ = (uint8_t)(pSecure->uSslVersion>>0);

    // generate and set server random data
    CryptRandGet(pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
    ds_memcpy(pData, pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->ServerRandom, sizeof(pSecure->ServerRandom), "ServerRandom");
    #endif
    pData += 32;

    // add sessionid to ServerHello
    NetPrintfVerbose((DEBUG_RES_SESS, 0, "protossl: setting session id for resume\n"));
    *pData++ = (uint8_t)sizeof(pSecure->SessionId);
    ds_memcpy(pData, pSecure->SessionId, sizeof(pSecure->SessionId));
    pData += sizeof(pSecure->SessionId);

    // add the selected cipher suite & compress method
    *pData++ = pSecure->pCipher->uIdent[0];
    *pData++ = pSecure->pCipher->uIdent[1];
    *pData++ = 0;

    // add zero-length extensions
    if (pState->uHelloExtn != 0)
    {
        pData = _ProtoSSLAddHelloExtensions(pState, pData, (uint32_t)sizeof(strBody)-(uint32_t)(pData-strBody), pState->uHelloExtn);
    }

    // setup the header
    strHead[0] = SSL3_MSG_SERVER_HELLO;
    strHead[1] = 0;
    strHead[2] = (uint8_t)((pData-strBody)>>8);
    strHead[3] = (uint8_t)((pData-strBody)>>0);

    // send the packet
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, (int32_t)(pData-strBody));
    return(pSecure->bSessionResume ? ST3_SEND_CHANGE : ST3_SEND_CERT);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvServerHello

    \Description
        Process ServerHello handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO or ST_RECV_CHANGE, or ST_FAIL_* on error)

    \Version 03/15/2013 (jbrookes) Added session resume support
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvServerHello(ProtoSSLRefT *pState, const uint8_t *pData)
{
    int32_t iIndex, iParseResult, iState = ST3_RECV_HELLO;
    const uint8_t *pExtensionsEnd, *pHelloEnd;
    SecureStateT *pSecure = pState->pSecure;

    /* peek backwards to get the location of server hello end
       some servers will not send any extension length so we need to make
       sure we don't parse past the end of the packet server hello */
    pHelloEnd = pData + ((pData[-3] << 16) | (pData[-2] << 8) | pData[-1]);

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process Server Hello\n"));

    // get server-specified version of the protocol
    pSecure->uSslVersion = (pData[0] << 8) | pData[1];
    // make sure we support it
    if (pSecure->uSslVersion != pState->uSslVersion)
    {
        if ((pSecure->uSslVersion < pState->uSslVersionMin) || (pSecure->uSslVersion > pState->uSslVersion))
        {
            NetPrintf(("protossl: server specified SSL version %d.%d is not supported\n", pData[0], pData[1]));
            /* As per http://tools.ietf.org/html/rfc5246#appendix-E.1 - If the version chosen by
               the server is not supported by the client (or not acceptable), the client MUST send a
               "protocol_version" alert message and close the connection. */
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_PROTOCOL_VERSION);
            return((pSecure->uSslVersion < pState->uSslVersionMin) ? ST_FAIL_CONN_MINVERS : ST_FAIL_CONN_MAXVERS);
        }
        else
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protossl: downgrading SSL version\n"));
        }
    }
    NetPrintfVerbose((pState->iVerbose, 0, "protossl: using %s\n", _SSL3_strVersionNames[pSecure->uSslVersion&0xff]));

    // skip version and save server random data
    pData += 2;
    ds_memcpy(pSecure->ServerRandom, pData, sizeof(pSecure->ServerRandom));
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->ServerRandom, sizeof(pSecure->ServerRandom), "ServerRandom");
    #endif
    pData += 32;

    // save server session id
    if (*pData == sizeof(pSecure->SessionId))
    {
        #if DEBUG_RAW_DATA
        NetPrintMem(pData+1, sizeof(pSecure->SessionId), "ServerHello session id");
        #endif

        // check for ClientHello/ServerHello session match; a match indicates we are resuming the session and bypassing key exchange
        if (!memcmp(pSecure->SessionId, pData+1, sizeof(pSecure->SessionId)))
        {
            // set resume and update state
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: resuming previous session\n"));
            pSecure->bSessionResume = TRUE;
            iState = ST3_RECV_CHANGE;
        }
        else
        {
            // save session id
            ds_memcpy(pSecure->SessionId, pData+1, sizeof(pSecure->SessionId));
        }
    }
    else
    {
        if (*pData != 0)
        {
            NetPrintf(("protossl: _ServerHello: session id length %d unexpected and not supported\n", *pData));
        }
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: session cannot be resumed\n"));
    }
    pData += *pData+1;

    // clear premaster secret if not resuming
    if (!pSecure->bSessionResume)
    {
        ds_memclr(pSecure->MasterKey, sizeof(pSecure->MasterKey));
    }

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

    if (pSecure->pCipher != NULL)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: using cipher suite %s (ident=%d,%d)\n", pSecure->pCipher->strName, pData[0], pData[1]));
    }
    else
    {
        NetPrintf(("protossl: no matching cipher (ident=%d,%d)\n", pData[0], pData[1]));
        /* a server selecting a cipher we don't support results in a handshake failure alert;
           this should not happen and would indicate broken behavior */
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_CONN_NOCIPHER);
    }

    // skip cipher and compression
    pData += 3;

    if (pData >= pHelloEnd)
    {
        return(iState);
    }

    // find end of extensions section
    pExtensionsEnd = &pData[2] + ((pData[0] << 8) | pData[1]);
    pData += 2;

    // parse the extensions
    if ((iParseResult = _ProtoSSLParseHelloExtensions(pState, pData, pExtensionsEnd)) != 0)
    {
        return(iParseResult);
    }

    // return new state (either still in hello state, or receiving cipher change if session resume)
    return(iState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendCertificate

    \Description
        Send Certificate handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_KEY or ST3_SEND_CERT_REQ or ST3_SEND_DONE)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendCertificate(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];
    uint8_t strBody[4096];
    int32_t iCertSize, iNumCerts;
    int32_t iState;
    SecureStateT *pSecure = pState->pSecure;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send Certificate\n"));

    // base64 decode certificate and add to body
    iCertSize = (pState->pCertificate != NULL) ? Base64Decode2(pState->iCertificateLen, pState->pCertificate, (char *)strBody+6) : 0;
    iNumCerts = (iCertSize) ? 1 : 0;

    // setup the header
    strHead[0] = SSL3_MSG_CERTIFICATE;
    strHead[1] = 0;
    strHead[2] = (uint8_t)((iCertSize+3+(iNumCerts*3))>>8);
    strHead[3] = (uint8_t)((iCertSize+3+(iNumCerts*3))>>0);

    // copy the certificate list into message body
    strBody[0] = 0;
    strBody[1] = (uint8_t)((iCertSize+(iNumCerts*3))>>8);
    strBody[2] = (uint8_t)((iCertSize+(iNumCerts*3))>>0);
    if (iNumCerts)
    {
        strBody[3] = 0;
        strBody[4] = (uint8_t)((iCertSize)>>8);
        strBody[5] = (uint8_t)((iCertSize)>>0);
    }

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, strBody, iCertSize+3+(iNumCerts*3));

    /* remember we sent a certificate (since we will send an empty message if no certificate
       is available, only remember if we *actually* sent a certificate).  this is used later
       to determine if we should send a CertificateVerify handshake message */
    pState->bCertSent = iNumCerts ? TRUE : FALSE;

    // figure out next state transition
    if ((pState->bServer) && (pSecure->pCipher->uKey == SSL3_KEY_RSA))
    {
        iState = (pState->iClientCertLevel > 0) ? ST3_SEND_CERT_REQ : ST3_SEND_DONE;
    }
    else
    {
        iState = ST3_SEND_KEY;
    }
    return(iState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvCertificate

    \Description
        Process Certificate handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO or ST_FAIL_* on error)

    \Version 08/26/2011 (szhu) Added multi-certificate support
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvCertificate(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iSize1, iSize2, iParse, iCertSize;
    int32_t iResult = ST_FAIL_SETUP; // if cert validated, iResult is ST3_RECV_HELLO
    int32_t iCertHeight = 0; // the height of current cert, the leaf cert's height is 0.
    X509CertificateT leafCert, prevCert;
    uint8_t aFingerprintId[SSL_FINGERPRINT_SIZE], bCertCached = FALSE;
    uint64_t uTime;
    #if DIRTYCODE_LOGGING
    char strIdentSubject[512], strIdentIssuer[512];
    #endif


    // ref: http://tools.ietf.org/html/rfc5246#section-7.4.2

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process Certificate\n"));

    ds_memclr(&leafCert, sizeof(leafCert));
    ds_memclr(&prevCert, sizeof(prevCert));

    // get outer size
    if ((iCertSize = iSize1 = (pData[0]<<16)|(pData[1]<<8)|(pData[2]<<0)) <= 3)
    {
        NetPrintf(("protossl: no certificate included in certificate message\n"));
        if (pState->bServer && (pState->iClientCertLevel == 2))
        {
            /* As per http://tools.ietf.org/html/rfc5246#section-7.4.6, a client providing
               an empty certificate response to a certificate request can either be ignored and
               treated as unauthenticated, or responded to with a fatal handshake failure */
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
            iResult = ST_FAIL_CERT_NONE;
        }
        else if (!pState->bServer)
        {
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_BAD_CERTFICIATE);
            iResult = ST_FAIL_CERT_NONE;
        }
        else
        {
            iResult = ST3_RECV_HELLO;
        }
    }
    else
    {
        // remember we received a certificate in the certificate message
        pState->bCertRecv = TRUE;

        // calculate fingerprint of entire certificate envelope
        _CalcFingerprint(aFingerprintId, sizeof(aFingerprintId), pData, iCertSize, CRYPTHASH_SHA1);

        // see if we've already validated
        if (_CertValidHistoryCheck(aFingerprintId, iCertSize))
        {
            NetPrintfVerbose((DEBUG_VAL_CERT, 0, "protossl: matched certificate fingerprint in certificate validity history\n"));
            bCertCached = TRUE;
        }
    }

    // process certificates
    while (iSize1 > 3)
    {
        // get certificate size
        iSize2 = (pData[3]<<16)|(pData[4]<<8)|(pData[5]<<0);

        // at least validate that the two sizes seem to correspond
        if (iSize2 > iSize1-3)
        {
            NetPrintf(("protossl: _ServerCert: certificate is larger than envelope (%d/%d)\n", iSize1, iSize2));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_BAD_CERTFICIATE);
            return(ST_FAIL_CERT_INVALID);
        }

        // parse the certificate
        iParse = _ParseCertificate(&pSecure->Cert, pData+6, iSize2);
        if (iParse < 0)
        {
            NetPrintf(("protossl: _ServerCert: x509 cert invalid (error=%d)\n", iParse));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_BAD_CERTFICIATE);
            return(ST_FAIL_CERT_INVALID);
        }

        // validate date range
        if ((uTime = ds_timeinsecs()) != 0)
        {
            if ((uTime < pSecure->Cert.uGoodFrom) || (uTime > pSecure->Cert.uGoodTill))
            {
                _DebugPrintCert(&pSecure->Cert, "certificate failed date range validation:");
                pSecure->bDateVerifyFailed = TRUE;
            }
        }

        // allow validation bypass (note: this must come after certificate is parsed)
        if (pState->bAllowAnyCert)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: bypassing certificate validation (disabled)\n"));
            return(ST3_RECV_HELLO);
        }

        // if it's the leaf cert
        if (iCertHeight == 0)
        {
            // ensure certificate was issued to this host (server certificate only)
            if (!pState->bServer && (_WildcardMatchSubdomainNoCase(pState->strHost, pSecure->Cert.Subject.strCommon) != 0))
            {
                // check against alternative names, if present
                if (_SubjectAlternativeMatch(pState->strHost, pSecure->Cert.pSubjectAlt, pSecure->Cert.iSubjectAltLen) != 0)
                {
                    NetPrintf(("protossl: _ServerCert: certificate not issued to this host (%s != %s)\n", pState->strHost, pSecure->Cert.Subject.strCommon));
                    _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_CERTIFICATE_UNKNOWN);
                    _DebugPrintCert(&pSecure->Cert, "cert");
                    return(ST_FAIL_CERT_HOST);
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
                    else
                    {
                        // not part of our chain, so skip it
                        _DebugPrintCert(&pSecure->Cert, "_ServerCert: certificate not part of chain, skipping:");
                        pData += iSize2 + 3;
                        iSize1 -= iSize2 + 3;
                        continue;
                    }
                }
            }

            if (iParse != 0)
            {
                NetPrintf(("protossl: _ServerCert: certificate (height=%d) untrusted.\n", iCertHeight));
                _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNKNOWN_CA);
                _DebugPrintCert(&pSecure->Cert, "cert");
                return(ST_FAIL_CERT_NOTRUST);
            }
            else
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protossl: cert (%s) validated by ca (%s)\n", _DebugFormatCertIdent(&prevCert.Subject, strIdentSubject, sizeof(strIdentSubject)),
                    _DebugFormatCertIdent(&prevCert.Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
            }
        }

        // force _VerifyCertificate to update pState->CertInfo if failed to verify the cert
        pState->bCertInfoSet = FALSE;
        // verify the certificate signature to ensure we trust it (no self-signed allowed)
        if (!bCertCached)
        {
            iParse = _VerifyCertificate(pState, &pSecure->Cert, FALSE);
        }
        else
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: bypassing certificate validation (cached)\n"));
            iParse = 0;
        }
        // if the cert was validated or we reached the last cert
        if ((iParse == 0) || (iSize1 == (iSize2 + 3)))
        {
            #if DIRTYCODE_LOGGING
            if (iParse == 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protossl: cert (%s) validated by ca (%s)\n", _DebugFormatCertIdent(&pSecure->Cert.Subject, strIdentSubject, sizeof(strIdentSubject)),
                    _DebugFormatCertIdent(&pSecure->Cert.Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
                NetPrintfVerbose((pState->iVerbose, 0, "protossl: sigalg=%s, size=%d\n", _SSL3_strSignatureTypes[pSecure->Cert.iSigType - ASN_OBJ_RSA_PKCS_MD2], pSecure->Cert.iSigSize*8));
            }
            else if (iParse == SSL_ERR_GOSCA_INVALIDUSE)
            {
                NetPrintf(("protossl: gos ca (%s) may not be used for non-ea domain %s\n",
                    _DebugFormatCertIdent(&pSecure->Cert.Issuer, strIdentSubject, sizeof(strIdentSubject)),
                    pSecure->Cert.Subject.strCommon));
            }
            else if (iParse == SSL_ERR_CAPROVIDER_INVALID)
            {
                NetPrintf(("protossl: ca (%s) may not be used as a ca provider\n", _DebugFormatCertIdent(&pSecure->Cert.Issuer, strIdentSubject, sizeof(strIdentSubject))));
            }
            else
            {
                _DebugPrintCert(&pSecure->Cert, "_VerifyCertificate() failed -- no CA available for this certificate");
                NetPrintf(("protossl: _ServerCert: x509 cert untrusted (error=%d)\n", iParse));
            }
            #endif
            // if we need to fetch CA
            if ((iParse != 0) && (iParse != SSL_ERR_GOSCA_INVALIDUSE) && (_ProtoSSLInitiateCARequest(pState) == 0))
            {
                iResult = ST_WAIT_CA;
            }
            else if (pSecure->bDateVerifyFailed)
            {
                NetPrintf(("protossl: _ServerCert: failed date validation\n"));
                _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_CERTIFICATE_EXPIRED);
                iResult = ST_FAIL_CERT_BADDATE;
            }
            else if (iParse != 0)
            {
                _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_UNKNOWN_CA);
                iResult = ST_FAIL_CERT_NOTRUST;
            }
            else
            {
                // process next handshake packet
                iResult = ST3_RECV_HELLO;
                // add cert fingerprint to cert validation history if it's not already there
                if (!bCertCached)
                {
                    _CertValidHistoryAdd(aFingerprintId, iCertSize);
                }
            }
            // restore the leaf cert
            if ((iCertHeight != 0) && (iResult != ST_FAIL_CERT_NOTRUST))
            {
                ds_memcpy(&pSecure->Cert, &leafCert, sizeof(pSecure->Cert));
            }
            break;
        }
        else
        {
            // save the leaf cert for final use
            if (iCertHeight++ == 0)
            {
                ds_memcpy(&leafCert, &pSecure->Cert, sizeof(leafCert));
            }
            // save current cert and move to next (the issuer's cert)
            ds_memcpy(&prevCert, &pSecure->Cert, sizeof(prevCert));
            pData += iSize2 + 3;
            iSize1 -= iSize2 + 3;
        }
    }

    // still in hello state (if validated)
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendServerKeyExchange

    \Description
        Send the ServerKeyExchange message (only for ECDHE key exchange)

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_KEY, ST3_SEND_DONE or ST_FAIL_SETUP on error)

    \Version 03/03/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendServerKeyExchange(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    CryptEccPointT PublicKey;
    uint8_t aBody[1024], aHead[4];
    uint8_t *pData = aBody;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ServerKeyExchange\n"));

    // if using RSA for key exchange we not send this message, treat as error
    if (pSecure->pCipher->uKey == SSL3_KEY_RSA)
    {
        NetPrintf(("protossl: _SendServerKey: wrong key exchange algorithm RSA"));
        return(ST_FAIL_SETUP);
    }

    if (!pSecure->bEccContextInitialized)
    {
        // initialize elliptic curve data
        CryptEccInit(&pSecure->EccContext, pSecure->pEllipticCurve->uKeySize, pSecure->pEllipticCurve->aPrime,
            pSecure->pEllipticCurve->aCoefficientA, pSecure->pEllipticCurve->aCoefficientB,
            pSecure->pEllipticCurve->aGenerator, pSecure->pEllipticCurve->aOrder);
        pSecure->bEccContextInitialized = TRUE;
    }

    if (pSecure->uPubKeyLength == 0)
    {
        /* generate public key
           elliptic curve generation takes multiple frames
           so stay in the same state until the operation is complete */
        if (CryptEccPublic(&pSecure->EccContext, &PublicKey) > 0)
        {
            return(ST3_SEND_KEY);
        }
        pSecure->uPubKeyLength = CryptEccPointFinal(&PublicKey, pSecure->PubKey, sizeof(pSecure->PubKey));

        NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate public key for server key message) %dms\n",
            pSecure->EccContext.uCryptUSecs/1000));
        pSecure->uTimer += pSecure->EccContext.uCryptUSecs/1000;
    }

    // setup the data that needed to be hashed
    *pData++ = SSL3_CURVETYPE_NAMED_CURVE;
    *pData++ = (uint8_t)(pSecure->pEllipticCurve->uIdent >> 8);
    *pData++ = (uint8_t)(pSecure->pEllipticCurve->uIdent >> 0);
    *pData++ = pSecure->uPubKeyLength;
    ds_memcpy_s(pData, pSecure->uPubKeyLength, pSecure->PubKey, sizeof(pSecure->PubKey));
    pData += pSecure->uPubKeyLength;

    // setup the encryption for the server key exchange signature
    if (!pSecure->bRSAContextInitialized)
    {
        const CryptHashT *pHash;
        CryptHashTypeE eHashType;
        uint8_t uAsnHashType = ASN_OBJ_SHA256;

        /* pick a hash type based on the selected signature algorithm
           start at the first supported hash and then offset */
        eHashType = CRYPTHASH_MD2;
        eHashType += pSecure->pSigAlg->uHash;

        // translate this to the asn object for the hash if not the default
        if (eHashType == CRYPTHASH_SHA1)
        {
            uAsnHashType = ASN_OBJ_SHA1;
        }
        else if (eHashType == CRYPTHASH_MD5)
        {
            uAsnHashType = ASN_OBJ_MD5;
        }

        // calculate signature
        if ((pHash = CryptHashGet(eHashType)) != NULL)
        {
            uint8_t HashState[CRYPTHASH_MAXSTATE], aHashDigest[CRYPTHASH_MAXDIGEST], aHash[sizeof(aHashDigest)+64];
            int32_t iPrivateKeySize, iHashSize = CryptHashGetSize(eHashType);
            X509PrivateKeyT PrivateKey;
            const ASNObjectT *pObject;
            uint8_t *pHash2 = aHash;

            // hash the server key exchange data
            pHash->Init(HashState, iHashSize);
            pHash->Update(HashState, pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
            pHash->Update(HashState, pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
            pHash->Update(HashState, aBody, (uint32_t)(pData-aBody));
            pHash->Final(HashState, aHashDigest, iHashSize);

            // generate DER-encoded DigitalHash object as per http://tools.ietf.org/html/rfc5246#section-4.7
            pObject = _GetObject(uAsnHashType);
            *pHash2++ = ASN_CONSTRUCT|ASN_TYPE_SEQN;
            *pHash2++ = (2)+(2+pObject->iSize)+(2)+(2+iHashSize);
            *pHash2++ = ASN_CONSTRUCT|ASN_TYPE_SEQN;
            *pHash2++ = (2+pObject->iSize)+(2);
            // add hash object
            *pHash2++ = ASN_TYPE_OBJECT;
            *pHash2++ = pObject->iSize;
            // add asn.1 object identifier for hash
            ds_memcpy(pHash2, pObject->strData, pObject->iSize);
            pHash2 += pObject->iSize;
            // add null object
            *pHash2++ = ASN_TYPE_NULL;
            *pHash2++ = 0x00;
            // add hash object (octet string)
            *pHash2++ = ASN_TYPE_OCTSTRING;
            *pHash2++ = iHashSize;
            // add hash data
            ds_memcpy(pHash2, aHashDigest, iHashSize);
            pHash2 += iHashSize;

            // calculate final object size
            iHashSize = (int32_t)(pHash2 - aHash);

            #if DEBUG_RAW_DATA
            NetPrintMem(aHashDigest, iHashSize, "send server key signature hash");
            #endif

            // decode private key, extract key modulus and private exponent
            if ((iPrivateKeySize = _ParsePrivateKey(pState->pPrivateKey, pState->iPrivateKeyKen, &PrivateKey)) < 0)
            {
                NetPrintf(("protossl: unable to decode private key\n"));
                _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
                return(ST_FAIL_SETUP);
            }
            // setup to encrypt using private key
            if (CryptRSAInit(&pSecure->RSAContext, PrivateKey.Modulus.pObjData, PrivateKey.Modulus.iObjSize, PrivateKey.PrivateExponent.pObjData, PrivateKey.PrivateExponent.iObjSize) != 0)
            {
                NetPrintf(("protossl: RSA init failed on private key encrypt\n"));
                _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
                return(ST_FAIL_SETUP);
            }
            CryptRSAInitPrivate(&pSecure->RSAContext, aHash, iHashSize);
            pSecure->bRSAContextInitialized = TRUE;
        }
    }

    /* encrypt handshake hash; break it up into chunks of 64 to prevent blocking thread entire time.
       we don't do them one at a time because for a typical private key that would be 2048 calls and
       would incur unacceptable overhead, so this is a compromise between doing it all at once and
       doing it one step at a time */
    if (CryptRSAEncrypt(&pSecure->RSAContext, 64) > 0)
    {
        return(ST3_SEND_KEY);
    }

    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (rsa encrypt signature hash for server key message) %dms (%d calls)\n",
        pSecure->RSAContext.uCryptMsecs, pSecure->RSAContext.uNumExpCalls));
    pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;

    *pData++ = pSecure->pSigAlg->uHash;
    *pData++ = pSecure->pSigAlg->uSignature;
    *pData++ = (uint8_t)(pSecure->RSAContext.iKeyModSize>>8);
    *pData++ = (uint8_t)(pSecure->RSAContext.iKeyModSize>>0);
    ds_memcpy(pData, pSecure->RSAContext.EncryptBlock, pSecure->RSAContext.iKeyModSize);
    pData += pSecure->RSAContext.iKeyModSize;

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->RSAContext.EncryptBlock, pSecure->RSAContext.iKeyModSize, "encrypted server key exchange signature");
    #endif

    // format the packet header
    aHead[0] = SSL3_MSG_SERVER_KEY;
    aHead[1] = 0;
    aHead[2] = (uint8_t)((pData - aBody) >> 8);
    aHead[3] = (uint8_t)((pData - aBody) >> 0);

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, aHead, sizeof(aHead), aBody, (int32_t)(pData - aBody));
    return((pState->iClientCertLevel > 0) ? ST3_SEND_CERT_REQ : ST3_SEND_DONE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvServerKeyExchange

    \Description
        Process ServerKeyExchange handshake packet; not used with RSA cipher suites

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO or ST_FAIL_SETUP on error)

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvServerKeyExchange(ProtoSSLRefT *pState, const uint8_t *pData)
{
    int32_t iIndex;
    SecureStateT *pSecure = pState->pSecure;
    CryptHashTypeE eHashType;
    const CryptHashT *pHash;
    const uint8_t *pBegin = pData;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ServerKeyExchange\n"));

    // if using RSA for key exchange we do not receive this message, treat as error
    if (pSecure->pCipher->uKey == SSL3_KEY_RSA)
    {
        NetPrintf(("protossl: _ServerKey: wrong key exchange algorithm RSA\n"));
        return(ST_FAIL_SETUP);
    }
    // make sure it is using the curve type we support
    if (pData[0] == SSL3_CURVETYPE_NAMED_CURVE)
    {
        // validate the elliptic curve sent by the server
        for (iIndex = 0; iIndex < (signed)(sizeof(_SSL3_EllipticCurves)/sizeof(*_SSL3_EllipticCurves)); iIndex += 1)
        {
            // skip non-matching elliptic curve
            if ((pData[1] != (uint8_t)(_SSL3_EllipticCurves[iIndex].uIdent >> 8)) || (pData[2] != (uint8_t)(_SSL3_EllipticCurves[iIndex].uIdent)))
            {
                continue;
            }
            // found an elliptic curve
            pSecure->pEllipticCurve = &_SSL3_EllipticCurves[iIndex];
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: _ServerKey: using named curve %s (ident=0x%04x)\n", pSecure->pEllipticCurve->strName, pSecure->pEllipticCurve->uIdent));
            break;
        }
        pData += 3;
    }
    // make sure the server sent us one of our supported elliptic curves
    if (pSecure->pEllipticCurve == NULL)
    {
        NetPrintf(("protossl: _ServerKey: no matching named curve when using ecdhe key exchange\n"));
        return(ST_FAIL_SETUP);
    }

    // copy the public key
    pSecure->uPubKeyLength = *pData++;
    ds_memcpy_s(pSecure->PubKey, sizeof(pSecure->PubKey), pData, pSecure->uPubKeyLength);
    pData += pSecure->uPubKeyLength;

    // get the hash and verify the signature algorithm
    switch (pData[0])
    {
        case SSL3_HASHID_SHA256:
            eHashType = CRYPTHASH_SHA256; break;
        case SSL3_HASHID_SHA1:
            eHashType = CRYPTHASH_SHA1; break;
        case SSL3_HASHID_MD5:
            eHashType = CRYPTHASH_MD5; break;
        default:
            eHashType = CRYPTHASH_NULL; break;
    }
    if ((eHashType == CRYPTHASH_NULL) || (pData[1] != SSL3_SIGALG_RSA))
    {
        NetPrintf(("protossl: _ServerKey: signature algorithm doesn't match\n"));
        return(ST_FAIL_SETUP);
    }

    // calculate and verify the signature
    if ((pHash = CryptHashGet(eHashType)) != NULL)
    {
        uint8_t HashState[CRYPTHASH_MAXSTATE], aHashDigest[CRYPTHASH_MAXDIGEST];
        int32_t iHashSize = CryptHashGetSize(eHashType);
        int16_t iSigSize = (pData[2] << 8) | pData[3];
        const uint8_t *pSig = pData+4;

        pHash->Init(HashState, iHashSize);
        pHash->Update(HashState, pSecure->ClientRandom, sizeof(pSecure->ClientRandom));
        pHash->Update(HashState, pSecure->ServerRandom, sizeof(pSecure->ServerRandom));
        pHash->Update(HashState, pBegin, (uint32_t)(pData-pBegin));
        pHash->Final(HashState, aHashDigest, iHashSize);

        if (CryptRSAInit(&pSecure->RSAContext, pSecure->Cert.KeyModData, pSecure->Cert.iKeyModSize, pSecure->Cert.KeyExpData, pSecure->Cert.iKeyExpSize) != 0)
        {
            NetPrintf(("protossl: _ServerKey: failed to initialize the RSA context\n"));
            return(ST_FAIL_SETUP);
        }
        CryptRSAInitSignature(&pSecure->RSAContext, pSig, iSigSize);
        CryptRSAEncrypt(&pSecure->RSAContext, 0);
        pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;

        #if DEBUG_RAW_DATA
        NetPrintMem(pSecure->RSAContext.EncryptBlock, pSecure->RSAContext.iKeyModSize, "decrypted server key exchange");
        #endif

        if (memcmp(aHashDigest, pSecure->RSAContext.EncryptBlock+iSigSize-iHashSize, iHashSize) != 0)
        {
            NetPrintf(("protossl: _ServerKey: signature verify failed\n"));
            return(ST_FAIL_SETUP);
        }
    }

    CryptEccInit(&pSecure->EccContext, pSecure->pEllipticCurve->uKeySize, pSecure->pEllipticCurve->aPrime, pSecure->pEllipticCurve->aCoefficientA, pSecure->pEllipticCurve->aCoefficientB,
                 pSecure->pEllipticCurve->aGenerator, pSecure->pEllipticCurve->aOrder);
    pSecure->bEccContextInitialized = TRUE;

    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendCertificateRequest

    \Description
        Send CertificateRequest handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_DONE)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendCertificateRequest(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t strHead[4];
    uint8_t strBody[32];
    int32_t iBodySize;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send Certificate Request\n"));

    // setup the body
    strBody[0] = 1;             // number of certificate types
    strBody[1] = SSL3_KEY_RSA;  // certificate type

    // fill in certificate request data
    if (pSecure->uSslVersion < SSL3_TLS1_2)
    {
        // number of certificate authorities (not supported)
        strBody[2] = 0;
        strBody[3] = 0;
        iBodySize = 4;
    }
    else
    {
        // add TLS1.2 required SignatureAndHashAlgorithm as per http://tools.ietf.org/html/rfc5246#section-7.4.4
        strBody[2] = 0;
        strBody[3] = 6;              // three supported_signature_algorithms;
        strBody[4] = SSL3_HASHID_SHA256;
        strBody[5] = SSL3_SIGALG_RSA;
        strBody[6] = SSL3_HASHID_SHA1;
        strBody[7] = SSL3_SIGALG_RSA;
        strBody[8] = SSL3_HASHID_MD5;
        strBody[9] = SSL3_SIGALG_RSA;
        // number of certificate authorities (not supported)
        strBody[10] = 0;
        strBody[11] = 0;
        iBodySize = 12;
    }

    // setup the header
    strHead[0] = SSL3_MSG_CERT_REQ;
    strHead[1] = 0;
    strHead[2] = 0;
    strHead[3] = iBodySize;

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, sizeof(strHead), strBody, iBodySize);
    return(ST3_SEND_DONE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvCertificateRequest

    \Description
        Process CertificateRequest handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_RECV_HELLO)

    \Version 10/18/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvCertificateRequest(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iType, iNumTypes;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process CertificateRequest\n"));

    // make sure an RSA certificate is permissable
    for (iType = 0, iNumTypes = pData[0]; iType < iNumTypes; iType += 1)
    {
        if (pData[iType+1] == SSL3_KEY_RSA)
        {
            break;
        }
    }
    // ensure using RSA for key exchange
    if (iType == iNumTypes)
    {
        NetPrintf(("protossl: no rsa cert option found in CertificateRequest\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
        return(ST_FAIL_SETUP);
    }
    // skip past cert types
    pData += iNumTypes+1;

    // TLS1.2 includes SignatureAndHashAlgorithm specification: http://tools.ietf.org/html/rfc5246#section-7.4.4
    if (pSecure->uSslVersion >= SSL3_TLS1_2)
    {
        pSecure->uCertVerifyHashType = 0;
        // parse types array
        for (iType = 0, iNumTypes = ((pData[0] << 8) | pData[1]), pData += 2; iType < iNumTypes; iType += 2)
        {
            // skip non-RSA signing algorithms
            if (pData[iType+1] != SSL3_KEY_RSA)
            {
                continue;
            }
            // pick first valid hash algorithm we find (prefer server order)
            if ((pData[iType] == SSL3_HASHID_MD5) || (pData[iType] == SSL3_HASHID_SHA1) || (pData[iType] == SSL3_HASHID_SHA256))
            {
                pSecure->uCertVerifyHashType = pData[iType];
                break;
            }
        }
        // make sure we found a signature and hash combination we support
        if (pSecure->uCertVerifyHashType == 0)
        {
            NetPrintf(("protossl: no valid signature/hash combination specified in CertificateRequest message\n"));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
            return(ST_FAIL_SETUP);
        }
    }

    /* As per http://tools.ietf.org/html/rfc5246#section-7.4.6, if no suitable certificate is available,
       the client MUST send a certificate message containing no certificates, so we don't bother to check
       whether we have a certificate here */
    pState->iClientCertLevel = 1;
    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendServerHelloDone

    \Description
        Send ServerHelloDone handshake packet; marks end of server hello sequence

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_RECV_HELLO)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendServerHelloDone(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ServerHelloDone\n"));

    // setup the header
    strHead[0] = SSL3_MSG_SERVER_DONE;
    strHead[1] = 0;
    strHead[2] = 0;
    strHead[3] = 0;

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, 4, NULL, 0);
    return(ST3_RECV_HELLO);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvServerHelloDone

    \Description
        Process ServerHelloDone handshake packet; marks end of server hello sequence

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_SEND_CERT or ST3_SEND_KEY)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvServerHelloDone(ProtoSSLRefT *pState, const uint8_t *pData)
{
    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ServerHelloDone\n"));
    return((pState->iClientCertLevel != 0) ? ST3_SEND_CERT : ST3_SEND_KEY);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendClientKeyExchangeRSA

    \Description
        Send ClientKeyExchange (RSA) handshake packet to the server; uses public key to encrypt

    \Input *pState  - module state reference

    \Output
        int32_t     - next state (ST3_SEND_VERIFY or ST3_SEND_CHANGE on completion,
                      ST3_SEND_KEY if RSA computation is ongoing, ST_FAIL_SETUP on error)

    \Version 10/11/2012 (jbrookes) Updated to support TLS1.0
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendClientKeyExchangeRSA(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t strHead[6];
    uint32_t uHeadSize;

    if (!pSecure->bRSAContextInitialized)
    {
        // build pre-master secret
        CryptRandGet(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
        /* From http://tools.ietf.org/html/rfc5246#section-7.4.7.1, client_version: The version
           requested by the client in the ClientHello.  This is used to detect version roll-back
           attacks */
        pSecure->PreMasterKey[0] = (uint8_t)(pSecure->uSslClientVersion>>8);
        pSecure->PreMasterKey[1] = (uint8_t)(pSecure->uSslClientVersion>>0);

        // init rsa state
        if (CryptRSAInit(&pSecure->RSAContext, pSecure->Cert.KeyModData, pSecure->Cert.iKeyModSize, pSecure->Cert.KeyExpData, pSecure->Cert.iKeyExpSize))
        {
            NetPrintf(("protossl: failure initializing rsa state\n"));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
            return(ST_FAIL_SETUP);
        }
        CryptRSAInitMaster(&pSecure->RSAContext, pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
        pSecure->bRSAContextInitialized = TRUE;
    }

    // encrypt the premaster key
    if (CryptRSAEncrypt(&pSecure->RSAContext, 1))
    {
        return(ST3_SEND_KEY);
    }
    pSecure->bRSAContextInitialized = FALSE;

    // update ssl perf timer
    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (encrypt rsa client premaster secret) %dms (%d calls)\n",
        pSecure->RSAContext.uCryptMsecs, pSecure->RSAContext.uNumExpCalls));
    pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;

    // build master secret
    _ProtoSSLBuildKey(pSecure, pSecure->MasterKey, sizeof(pSecure->MasterKey), pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey),
        pSecure->ClientRandom, pSecure->ServerRandom, sizeof(pSecure->ClientRandom), "master secret",
        pSecure->uSslVersion);

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey), "PreMaster");
    NetPrintMem(pSecure->MasterKey, sizeof(pSecure->MasterKey), "Master");
    #endif

    // get rid of premaster secret from memory
    ds_memclr(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));

    // setup the header
    strHead[0] = SSL3_MSG_CLIENT_KEY;
    strHead[1] = 0;
    if (pSecure->uSslVersion < SSL3_TLS1_0)
    {
        strHead[2] = (uint8_t)(pSecure->Cert.iKeyModSize>>8);
        strHead[3] = (uint8_t)(pSecure->Cert.iKeyModSize>>0);
        uHeadSize = 4;
    }
    else // TLS1
    {
        strHead[2] = (uint8_t)((pSecure->Cert.iKeyModSize+2)>>8);
        strHead[3] = (uint8_t)((pSecure->Cert.iKeyModSize+2)>>0);
        strHead[4] = (uint8_t)(pSecure->Cert.iKeyModSize>>8);
        strHead[5] = (uint8_t)(pSecure->Cert.iKeyModSize>>0);
        uHeadSize = 6;
    }

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, uHeadSize, pSecure->RSAContext.EncryptBlock, pSecure->RSAContext.iKeyModSize);
    return(pState->bCertSent ? ST3_SEND_VERIFY : ST3_SEND_CHANGE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendClientKeyExchangeECDHE

    \Description
        Send ClientKeyExchange (ECDHE) handshake packet to the server

    \Input *pState  - module state reference

    \Output
        int32_t     - next state (ST3_SEND_VERIFY or ST3_SEND_CHANGE on completion,
                      or ST_FAIL_SETUP on error)

    \Version 01/23/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendClientKeyExchangeECDHE(ProtoSSLRefT *pState)
{
    uint8_t aPublicKey[128], aHeader[4];
    int32_t iPublicKeySize;
    CryptEccPointT PublicKey;
    SecureStateT *pSecure = pState->pSecure;

    /* make sure that we have initialized the ecc context before we can
       do any work. this happens when we receive the server key exchange.
       this means that we require that this gets received before we
       can handle sending the client key exchange. */
    if (!pSecure->bEccContextInitialized)
    {
        NetPrintf(("protossl: _SendClientKey: the elliptic curve context has not been initialized\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_HANDSHAKE_FAILURE);
        return(ST_FAIL_SETUP);
    }

    /* generate public key
       elliptic curve generation takes multiple frames
       so stay in the same state until the operation is complete */
    if (CryptEccPublic(&pSecure->EccContext, &PublicKey) > 0)
    {
        return(ST3_SEND_KEY);
    }

    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate public key for client key message) %dms\n",
        pSecure->EccContext.uCryptUSecs/1000));
    pSecure->uTimer += pSecure->EccContext.uCryptUSecs/1000;

    // encode public key into buffer
    iPublicKeySize = CryptEccPointFinal(&PublicKey, aPublicKey+1, sizeof(aPublicKey)-1);
    aPublicKey[0] = iPublicKeySize;

    // format header
    aHeader[0] = SSL3_MSG_CLIENT_KEY;
    aHeader[1] = 0;
    aHeader[2] = 0;
    aHeader[3] = iPublicKeySize+1;

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, aHeader, sizeof(aHeader), aPublicKey, iPublicKeySize+1);
    return(pState->bCertSent ? ST3_SEND_VERIFY : ST3_SEND_CHANGE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendClientKeyExchange

    \Description
        Send ClientKeyExchange handshake packet to the server

    \Input *pState  - module state reference

    \Output
        int32_t     - next state (ST3_SEND_VERIFY or ST3_SEND_CHANGE on completion,
                      ST3_SEND_KEY if RSA computation is ongoing, ST_FAIL_SETUP on error)

    \Version 10/11/2012 (jbrookes) Updated to support TLS1.0
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendClientKeyExchange(ProtoSSLRefT *pState)
{
    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ClientKeyExchange\n"));

    return((pState->pSecure->pCipher->uKey == SSL3_KEY_RSA) ?
        _ProtoSSLUpdateSendClientKeyExchangeRSA(pState) : _ProtoSSLUpdateSendClientKeyExchangeECDHE(pState));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvClientKeyExchangeRSA

    \Description
        Process ClientKeyExchange handshake packet for RSA key exchange

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_RECV_CHANGE, or ST_FAIL_SETUP on error)

    \Version 10/15/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvClientKeyExchangeRSA(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iCryptKeySize, iPrivKeySize;
    uint8_t strRandomSecret[48];
    X509PrivateKeyT PrivateKey;
    const uint8_t *pCryptKeyData;
    uint16_t uSslVersion;

    // read encrypted key data
    if (pSecure->uSslVersion < SSL3_TLS1_0)
    {
        iCryptKeySize = (pData[-2] << 8) | pData[-1]; //$$todo negative indexing is lame, fix this
        pCryptKeyData = pData;
    }
    else
    {
        iCryptKeySize = (pData[0] << 8) | pData[1];
        pCryptKeyData = pData + 2;
    }

    // decode private key and extract key modulus and private exponent
    if ((iPrivKeySize = _ParsePrivateKey(pState->pPrivateKey, pState->iPrivateKeyKen, &PrivateKey)) < 0)
    {
        NetPrintf(("protossl: unable to decode private key\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
        return(ST_FAIL_SETUP);
    }

    // decrypt client premaster secret using private key
    NetPrintfVerbose((pState->iVerbose, 1, "protossl: decrypt client key (iKeySize=%d, iKeyModSize=%d, iKeyExpSize=%d)\n", iPrivKeySize, PrivateKey.Modulus.iObjSize, PrivateKey.PrivateExponent.iObjSize));
    if (CryptRSAInit(&pSecure->RSAContext, PrivateKey.Modulus.pObjData, PrivateKey.Modulus.iObjSize, PrivateKey.PrivateExponent.pObjData, PrivateKey.PrivateExponent.iObjSize))
    {
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
        return(ST_FAIL_SETUP);
    }
    CryptRSAInitSignature(&pSecure->RSAContext, pCryptKeyData, iCryptKeySize);
    CryptRSAEncrypt(&pSecure->RSAContext, 0);

    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (rsa decrypt client premaster secret) %dms\n", pSecure->RSAContext.uCryptMsecs));
    pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;

    // copy out and display decrypted client premaster key data
    ds_memcpy(pSecure->PreMasterKey, pSecure->RSAContext.EncryptBlock+iCryptKeySize-sizeof(pSecure->PreMasterKey), sizeof(pSecure->PreMasterKey));
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey), "client premaster key");
    #endif

    // generate random for use in possible roll-back attack handling
    CryptRandGet(strRandomSecret, sizeof(strRandomSecret));
    // validate client TLS version (first two bytes of premaster secret)
    if ((uSslVersion = (pSecure->PreMasterKey[0] << 8) | pSecure->PreMasterKey[1]) != pSecure->uSslClientVersion)
    {
        NetPrintf(("protossl: detected possible roll-back attack; premaster secret tls version %d.%d does not match client-specified version\n",
            pSecure->PreMasterKey[0], pSecure->PreMasterKey[1]));
    }

    /* As per http://tools.ietf.org/html/rfc5246#section-7.4.7.1, a mismatch in the premaster secret tls version
       and the client-specified version should be handled by generating a random premaster secret and continuing
       on.  The random data should be generated unconditionally to prevent possible timing attacks.  This same
       response is also utilized for any detected PKCS#1 padding errors. */
    if ((uSslVersion != pSecure->uSslClientVersion) || !_VerifyPkcs1_5_RSAES(pSecure->RSAContext.EncryptBlock, iCryptKeySize, sizeof(pSecure->PreMasterKey)))
    {
        ds_memcpy(pSecure->PreMasterKey, strRandomSecret, sizeof(strRandomSecret));
    }

    // build master secret
    _ProtoSSLBuildKey(pSecure, pSecure->MasterKey, sizeof(pSecure->MasterKey), pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey),
        pSecure->ClientRandom, pSecure->ServerRandom, sizeof(pSecure->ClientRandom), "master secret",
        pSecure->uSslVersion);

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey), "PreMaster");
    NetPrintMem(pSecure->MasterKey, sizeof(pSecure->MasterKey), "Master");
    #endif

    // get rid of premaster secret from memory
    ds_memclr(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));

    // move on to next state
    return(((pState->iClientCertLevel > 0) && pState->bCertRecv) ? ST3_RECV_HELLO : ST3_RECV_CHANGE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvClientKeyExchangeECDHE

    \Description
        Process ClientKeyExchange handshake packet for ECDHE key exchange

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_RECV_CHANGE, or ST_FAIL_SETUP on error)

    \Version 03/03/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvClientKeyExchangeECDHE(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;

    // copy the public key
    pSecure->uPubKeyLength = *pData++;
    ds_memcpy_s(pSecure->PubKey, sizeof(pSecure->PubKey), pData, pSecure->uPubKeyLength);
    pData += pSecure->uPubKeyLength;

    // move on to next state
    return(((pState->iClientCertLevel > 0) && (pState->bCertRecv)) ? ST3_RECV_HELLO : ST3_RECV_CHANGE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvClientKeyExchange

    \Description
        Process ClientKeyExchange handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state or error

    \Version 03/03/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvClientKeyExchange(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ClientKeyExchange\n"));

    return((pSecure->pCipher->uKey == SSL3_KEY_RSA) ?
        _ProtoSSLUpdateRecvClientKeyExchangeRSA(pState, pData) : _ProtoSSLUpdateRecvClientKeyExchangeECDHE(pState, pData));
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendCertificateVerify

    \Description
        Send CertificateVerify handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST3_SEND_CHANGE on completion, ST3_SEND_VERIFY
                          if RSA computation is ongoing, or ST_FAIL_SETUP on error)

    \Version 10/30/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendCertificateVerify(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint32_t uHeadSize;
    uint8_t strHead[16];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send CertificateVerify\n"));

    // set up for encryption of certificate verify message
    if (!pSecure->bRSAContextInitialized)
    {
        X509PrivateKeyT PrivateKey;
        CryptSha2T HandshakeSHA2;
        CryptSha1T HandshakeSHA;
        CryptMD5T HandshakeMD5;
        uint8_t strHash[CRYPTHASH_MAXDIGEST+64]; // must be big enough to hold max hash size plus ASN.1 object encoding (TLS1.2)
        int32_t iPrivKeySize, iHashSize=0;

        // set up buffer with current handshake hash
        if (pSecure->uSslVersion < SSL3_TLS1_2)
        {
            ds_memcpy(&HandshakeMD5, &pSecure->HandshakeMD5, sizeof(HandshakeMD5));
            CryptMD5Final(&HandshakeMD5, strHash, SSL3_MAC_MD5);
            ds_memcpy(&HandshakeSHA, &pSecure->HandshakeSHA, sizeof(HandshakeSHA));
            CryptSha1Final(&HandshakeSHA, strHash+SSL3_MAC_MD5, SSL3_MAC_SHA);
            iHashSize = SSL3_MAC_MD5+SSL3_MAC_SHA;
        }
        else
        {
            uint8_t strHash2[CRYPTHASH_MAXDIGEST], *pHash=strHash;
            const ASNObjectT *pObject;

            // produce handshake hash with previously selected hash type
            if (pSecure->uCertVerifyHashType == SSL3_HASHID_MD5)
            {
                ds_memcpy(&HandshakeMD5, &pSecure->HandshakeMD5, sizeof(HandshakeMD5));
                CryptMD5Final(&HandshakeMD5, strHash2, SSL3_MAC_MD5);
                iHashSize = SSL3_MAC_MD5;
                pObject = _GetObject(ASN_OBJ_MD5);
            }
            else if (pSecure->uCertVerifyHashType == SSL3_HASHID_SHA1)
            {
                ds_memcpy(&HandshakeSHA, &pSecure->HandshakeSHA, sizeof(HandshakeSHA));
                CryptSha1Final(&HandshakeSHA, strHash2, SSL3_MAC_SHA);
                iHashSize = SSL3_MAC_SHA;
                pObject = _GetObject(ASN_OBJ_SHA1);
            }
            else // SSL3_HASHID_SHA256
            {
                ds_memcpy(&HandshakeSHA2, &pSecure->HandshakeSHA256, sizeof(HandshakeSHA2));
                CryptSha2Final(&HandshakeSHA2, strHash2, SSL3_MAC_SHA256);
                iHashSize = SSL3_MAC_SHA256;
                pObject = _GetObject(ASN_OBJ_SHA256);
            }

            // generate DER-encoded DigitalHash object as per http://tools.ietf.org/html/rfc5246#section-4.7
            *pHash++ = ASN_CONSTRUCT|ASN_TYPE_SEQN;
            *pHash++ = (2)+(2+pObject->iSize)+(2)+(2+iHashSize);
            *pHash++ = ASN_CONSTRUCT|ASN_TYPE_SEQN;
            *pHash++ = (2+pObject->iSize)+(2);
            // add hash object
            *pHash++ = ASN_TYPE_OBJECT;
            *pHash++ = pObject->iSize;
            // add asn.1 object identifier for hash
            ds_memcpy(pHash, pObject->strData, pObject->iSize);
            pHash += pObject->iSize;
            // add null object
            *pHash++ = ASN_TYPE_NULL;
            *pHash++ = 0x00;
            // add hash object (octet string)
            *pHash++ = ASN_TYPE_OCTSTRING;
            *pHash++ = iHashSize;
            // add hash data
            ds_memcpy(pHash, strHash2, iHashSize);
            pHash += iHashSize;

            // calculate final object size
            iHashSize = (int32_t)(pHash - strHash);
        }

        #if DEBUG_RAW_DATA
        NetPrintMem(strHash, iHashSize, "certificate verify hash");
        #endif

        // decode private key and extract key modulus and private exponent
        if ((iPrivKeySize = _ParsePrivateKey(pState->pPrivateKey, pState->iPrivateKeyKen, &PrivateKey)) < 0)
        {
            NetPrintf(("protossl: unable to decode private key\n"));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
            return(ST_FAIL_SETUP);
        }

        // set up to encrypt using private key
        NetPrintfVerbose((pState->iVerbose, 1, "protossl: encrypt client key (iKeySize=%d, iKeyModSize=%d, iKeyExpSize=%d)\n", iPrivKeySize, PrivateKey.Modulus.iObjSize, PrivateKey.PrivateExponent.iObjSize));
        if (CryptRSAInit(&pSecure->RSAContext, PrivateKey.Modulus.pObjData, PrivateKey.Modulus.iObjSize, PrivateKey.PrivateExponent.pObjData, PrivateKey.PrivateExponent.iObjSize))
        {
            NetPrintf(("protossl: RSA init failed on private key encrypt\n"));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
            return(ST_FAIL_SETUP);
        }
        CryptRSAInitPrivate(&pSecure->RSAContext, strHash, iHashSize);
        pSecure->bRSAContextInitialized = TRUE;
    }

    /* encrypt handshake hash; break it up into chunks of 64 to prevent blocking thread entire time.
       we don't do them one at a time because for a typical private key that would be 2048 calls and
       would incur unacceptable overhead, so this is a compromise between doing it all at once and
       doing it one step at a time */
    if (CryptRSAEncrypt(&pSecure->RSAContext, 64) > 0)
    {
        return(ST3_SEND_VERIFY);
    }
    pSecure->bRSAContextInitialized = FALSE;

    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (rsa encrypt handshake hash for certificate verify message) %dms (%d calls)\n",
        pSecure->RSAContext.uCryptMsecs, pSecure->RSAContext.uNumExpCalls));
    pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;

    // setup the header
    strHead[0] = SSL3_MSG_CERT_VERIFY;
    strHead[1] = 0;
    if (pSecure->uSslVersion < SSL3_TLS1_0)
    {
        strHead[2] = (uint8_t)(pSecure->RSAContext.iKeyModSize>>8);
        strHead[3] = (uint8_t)(pSecure->RSAContext.iKeyModSize>>0);
        uHeadSize = 4;
    }
    else if (pSecure->uSslVersion < SSL3_TLS1_2)
    {
        strHead[2] = (uint8_t)((pSecure->RSAContext.iKeyModSize+2)>>8);
        strHead[3] = (uint8_t)((pSecure->RSAContext.iKeyModSize+2)>>0);
        strHead[4] = (uint8_t)(pSecure->RSAContext.iKeyModSize>>8);
        strHead[5] = (uint8_t)(pSecure->RSAContext.iKeyModSize>>0);
        uHeadSize = 6;
    }
    else
    {
        strHead[2] = (uint8_t)((pSecure->RSAContext.iKeyModSize+4)>>8);
        strHead[3] = (uint8_t)((pSecure->RSAContext.iKeyModSize+4)>>0);
        strHead[4] = pSecure->uCertVerifyHashType;
        strHead[5] = SSL3_KEY_RSA;
        strHead[6] = (uint8_t)(pSecure->RSAContext.iKeyModSize>>8);
        strHead[7] = (uint8_t)(pSecure->RSAContext.iKeyModSize>>0);
        uHeadSize = 8;
    }

    // send the packet
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, uHeadSize, pSecure->RSAContext.EncryptBlock, pSecure->RSAContext.iKeyModSize);

    // transition to next state
    return(ST3_SEND_CHANGE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvCertificateVerify

    \Description
        Process CertificateVerify handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST3_RECV_CHANGE, or ST_FAIL_SETUP on error)

    \Todo
        We can share code with RecvClientKeyExchange

    \Version 10/30/2013 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvCertificateVerify(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iCryptKeySize, iPrivKeySize;
    int32_t iHashSize = SSL3_MAC_MD5+SSL3_MAC_SHA; // default for TLS1.1 or earlier
    uint8_t strHash[SSL3_MAC_MAXSIZE], strHash2[SSL3_MAC_MAXSIZE];
    X509PrivateKeyT PrivateKey;
    const uint8_t *pCryptKeyData;
    CryptSha2T HandshakeSHA2;
    CryptSha1T HandshakeSHA;
    CryptMD5T HandshakeMD5;
    int32_t iHashId = 0;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process CertificateVerify\n"));

    // get encrypted key data size and offset
    if (pSecure->uSslVersion < SSL3_TLS1_0)
    {
        iCryptKeySize = (pData[-2] << 8) | pData[-1]; //$$todo negative indexing is lame, fix this
        pCryptKeyData = pData;
    }
    else if (pSecure->uSslVersion < SSL3_TLS1_2)
    {
        iCryptKeySize = (pData[0] << 8) | pData[1];
        pCryptKeyData = pData + 2;
    }
    else
    {
        // get hashid and derive hash size
        if ((iHashId = pData[0]) == SSL3_HASHID_MD5)
        {
            iHashSize = SSL3_MAC_MD5;
        }
        else if (iHashId == SSL3_HASHID_SHA1)
        {
            iHashSize = SSL3_MAC_SHA;
        }
        else if (iHashId == SSL3_HASHID_SHA256)
        {
            iHashSize = SSL3_MAC_SHA256;
        }
        else
        {
            iHashId = -1;
        }
        // validate hash and key type
        if ((iHashId < 0) || (pData[1] != SSL3_KEY_RSA))
        {
            NetPrintf(("protossl: unsupported hashid or signature algorithm in CertificateVerify\n"));
            _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
            return(ST_FAIL_SETUP);
        }
        // get crypt key size and data
        iCryptKeySize = (pData[2] << 8) | pData[3];
        pCryptKeyData = pData + 4;
    }

    // decode private key and extract key modulus and public exponent
    if ((iPrivKeySize = _ParsePrivateKey(pState->pPrivateKey, pState->iPrivateKeyKen, &PrivateKey)) < 0)
    {
        NetPrintf(("protossl: unable to decode private key\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
        return(ST_FAIL_SETUP);
    }

    // decrypt client key using CA private key
    NetPrintfVerbose((pState->iVerbose, 1, "protossl: decrypt client key (iKeySize=%d, iKeyModSize=%d, iKeyExpSize=%d)\n", iPrivKeySize, PrivateKey.Modulus.iObjSize, PrivateKey.PublicExponent.iObjSize));
    if (CryptRSAInit(&pSecure->RSAContext, PrivateKey.Modulus.pObjData, PrivateKey.Modulus.iObjSize, PrivateKey.PublicExponent.pObjData, PrivateKey.PublicExponent.iObjSize))
    {
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_INTERNAL_ERROR);
        return(ST_FAIL_SETUP);
    }
    CryptRSAInitSignature(&pSecure->RSAContext, pCryptKeyData, iCryptKeySize);
    CryptRSAEncrypt(&pSecure->RSAContext, 0);

    pSecure->uTimer += pSecure->RSAContext.uCryptMsecs;
    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (rsa decrypt client certificate verify hash) %dms\n", pSecure->RSAContext.uCryptMsecs));
    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->RSAContext.EncryptBlock, pSecure->RSAContext.iKeyModSize, "decrypted certificate verify");
    #endif

    // set up buffer with current handshake hash
    if (pSecure->uSslVersion < SSL3_TLS1_2)
    {
        ds_memcpy(&HandshakeMD5, &pSecure->HandshakeMD5, sizeof(HandshakeMD5));
        CryptMD5Final(&HandshakeMD5, strHash2, SSL3_MAC_MD5);
        ds_memcpy(&HandshakeSHA, &pSecure->HandshakeSHA, sizeof(HandshakeSHA));
        CryptSha1Final(&HandshakeSHA, strHash2+SSL3_MAC_MD5, SSL3_MAC_SHA);
        iHashSize = SSL3_MAC_MD5+SSL3_MAC_SHA;
    }
    else
    {
        if (iHashId == SSL3_HASHID_MD5)
        {
            NetPrintf(("protossl: choosing MD5 for certificate verify\n"));
            ds_memcpy(&HandshakeMD5, &pSecure->HandshakeMD5, sizeof(HandshakeMD5));
            CryptMD5Final(&HandshakeMD5, strHash2, SSL3_MAC_MD5);
        }
        else if (iHashId == SSL3_HASHID_SHA1)
        {
            NetPrintf(("protossl: choosing SHA1 for certificate verify\n"));
            ds_memcpy(&HandshakeSHA, &pSecure->HandshakeSHA, sizeof(HandshakeSHA));
            CryptSha1Final(&HandshakeSHA, strHash2, SSL3_MAC_SHA);
        }
        else if (iHashId == SSL3_HASHID_SHA256)
        {
            NetPrintf(("protossl: choosing SHA2 for certificate verify\n"));
            ds_memcpy(&HandshakeSHA2, &pSecure->HandshakeSHA256, sizeof(HandshakeSHA2));
            CryptSha2Final(&HandshakeSHA2, strHash2, SSL3_MAC_SHA256);
        }
    }
    #if DEBUG_RAW_DATA
    NetPrintMem(strHash2, iHashSize, "certificate verify hash 2");
    #endif

    // copy out and display decrypted verification hash
    ds_memcpy(strHash, pSecure->RSAContext.EncryptBlock+iCryptKeySize-iHashSize, iHashSize);
    #if DEBUG_RAW_DATA
    NetPrintMem(strHash, iHashSize, "certificate verify hash");
    #endif

    // validate the hash
    if (memcmp(strHash, strHash2, iHashSize))
    {
        NetPrintf(("protossl: client cert validate failed\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECRYPT_ERROR);
        return(ST_FAIL_SETUP);
    }
    return(ST3_RECV_CHANGE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLGenerateSecret

    \Description
        Generate the ECDHE shared secret if necessary

    \Input *pState      - module state reference

    \Output
        uint8_t         - TRUE=operation complete, FALSE=operation ongoing

    \Version 03/07/2017 (eesponda)
*/
/********************************************************************************F*/
static uint8_t _ProtoSSLGenerateSecret(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    CryptEccPointT PublicKey, Secret;

    if (!pSecure->bEccContextInitialized)
    {
        return(TRUE);
    }

    /* generate shared secret
       elliptic curve generation takes multiple frames
       so stay in the same state until the operation is complete */
    CryptEccPointInitFrom(&PublicKey, pSecure->PubKey, pSecure->uPubKeyLength);
    if (CryptEccSecret(&pSecure->EccContext, &PublicKey, &Secret) > 0)
    {
        return(FALSE);
    }
    CryptBnFinal(&Secret.X, pSecure->PreMasterKey, pSecure->pEllipticCurve->uKeySize);

    NetPrintfVerbose((DEBUG_ENC_PERF, 0, "protossl: SSL Perf (generate shared secret) %dms\n",
        pSecure->EccContext.uCryptUSecs/1000));
    pSecure->uTimer += pSecure->EccContext.uCryptUSecs/1000;

    // build master secret
    _ProtoSSLBuildKey(pSecure, pSecure->MasterKey, sizeof(pSecure->MasterKey), pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey),
        pSecure->ClientRandom, pSecure->ServerRandom, sizeof(pSecure->ClientRandom), "master secret",
        pSecure->uSslVersion);

    #if DEBUG_RAW_DATA
    NetPrintMem(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey), "PreMaster");
    NetPrintMem(pSecure->MasterKey, sizeof(pSecure->MasterKey), "Master");
    #endif

    // get rid of premaster secret from memory
    ds_memclr(pSecure->PreMasterKey, sizeof(pSecure->PreMasterKey));
    // finished with ecc context
    ds_memclr(&pSecure->EccContext, sizeof(pSecure->EccContext));
    pSecure->bEccContextInitialized = FALSE;
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendChangeCipherSpec

    \Description
        Send ChangeCipherSpec handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST_SEND_FINISH)

    \Version 10/15/2013 (jbrookes) Updated to handle client and server
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendChangeCipherSpec(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t strHead[4];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Send ChangeCipherSpec\n"));

    // generate the shared secret if necessary
    if (!_ProtoSSLGenerateSecret(pState))
    {
        return(ST3_SEND_CHANGE);
    }

    // build key material if we haven't already
    if (pSecure->pServerKey == NULL)
    {
        _ProtoSSLBuildKeyMaterial(pState);
    }

    // initialize write cipher
    if (pSecure->pCipher->uEnc == SSL3_ENC_RC4)
    {
        CryptArc4Init(&pSecure->WriteArc4, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen, 1);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
    {
        CryptAesInit(&pSecure->WriteAes, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen, CRYPTAES_KEYTYPE_ENCRYPT,
            pState->bServer ? pSecure->pServerInitVec : pSecure->pClientInitVec);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
    {
        CryptGcmInit(&pSecure->WriteGcm, pState->bServer ? pSecure->pServerKey : pSecure->pClientKey, pSecure->pCipher->uLen);
    }

    // mark as cipher change
    strHead[0] = 1;
    _SendPacket(pState, SSL3_REC_CIPHER, strHead, 1, NULL, 0);

    // reset the sequence number
    pSecure->uSendSeqn = 0;
    return(ST3_SEND_FINISH);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvChangeCipherSpec

    \Description
        Process ChangeCipherSpec handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - next state (ST_RECV_FINISH)

    \Version 10/15/2013 (jbrookes) Updated to handle client and server
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvChangeCipherSpec(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process ChangeCipherSpec\n"));

    // generate the shared secret if necessary
    if (!_ProtoSSLGenerateSecret(pState))
    {
        return(ST3_RECV_CHANGE);
    }

    // build key material if we haven't already
    if (pSecure->pServerKey == NULL)
    {
        _ProtoSSLBuildKeyMaterial(pState);
    }

    // initialize read cipher
    if (pSecure->pCipher->uEnc == SSL3_ENC_RC4)
    {
        CryptArc4Init(&pSecure->ReadArc4, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen, 1);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_AES)
    {
        CryptAesInit(&pSecure->ReadAes, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen, CRYPTAES_KEYTYPE_DECRYPT,
            pState->bServer ? pSecure->pClientInitVec : pSecure->pServerInitVec);
    }
    if (pSecure->pCipher->uEnc == SSL3_ENC_GCM)
    {
        CryptGcmInit(&pSecure->ReadGcm, pState->bServer ? pSecure->pClientKey : pSecure->pServerKey, pSecure->pCipher->uLen);
    }

    // reset sequence number
    pSecure->uRecvSeqn = 0;

    // just gobble packet -- we assume next state is crypted regardless
    return(ST3_RECV_FINISH);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSendFinished

    \Description
        Send Finished handshake packet

    \Input *pState      - module state reference

    \Output
        int32_t         - next state (ST_RECV_CHANGE or ST3_SECURE)

    \Version 10/23/2013 (jbrookes) Rewritten to handle client and server
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSendFinished(ProtoSSLRefT *pState)
{
    uint8_t strHead[4];
    uint8_t strBody[256];
    SecureStateT *pSecure = pState->pSecure;
    static const struct SSLFinished _SendFinished[2] = {
        { "CLNT", "client finished", {ST3_RECV_CHANGE, ST3_SECURE } },
        { "SRVR", "server finished", {ST3_SECURE, ST3_RECV_CHANGE } },
    };
    const struct SSLFinished *pFinished = &_SendFinished[pState->bServer];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: SendFinished\n"));

    // do finish hash, save size in packet header
    strHead[3] = (uint8_t)_ProtoSSLDoFinishHash(strBody, pSecure, pFinished->strLabelSSL, pFinished->strLabelTLS);

    // setup the header
    strHead[0] = SSL3_MSG_FINISHED;
    strHead[1] = 0;
    strHead[2] = 0;

    // all sends from here on out are secure
    pSecure->bSendSecure = TRUE;

    // setup packet for send
    _SendPacket(pState, SSL3_REC_HANDSHAKE, strHead, sizeof(strHead), strBody, strHead[3]);

    // next state will be secure mode (handshaking complete) or recv cipher change depending on client/server and if resuming or not
    return(pFinished->uNextState[pSecure->bSessionResume]);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvFinished

    \Description
        Process Finished handshake packet

    \Input *pState      - module state reference
    \Input *pData       - packet data

    \Output
        int32_t         - one if data was sent, else zero

    \Version 10/23/2013 (jbrookes) Rewritten to handle client and server
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecvFinished(ProtoSSLRefT *pState, const uint8_t *pData)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t MacFinal[36];
    int32_t iMacSize;
    static const struct SSLFinished _RecvFinished[2] = {
        { "SRVR", "server finished", {ST3_SECURE, ST3_SEND_CHANGE } },
        { "CLNT", "client finished", {ST3_SEND_CHANGE, ST3_SECURE } },
    };
    const struct SSLFinished *pFinished = &_RecvFinished[pState->bServer];

    NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: SSL Msg: Process RecvFinished\n"));

    // calculate the finish verification hash
    iMacSize = _ProtoSSLDoFinishHash(MacFinal, pSecure, pFinished->strLabelSSL, pFinished->strLabelTLS);

    // verify the hash
    if (memcmp(MacFinal, pData, iMacSize))
    {
        NetPrintf(("protossl: finish hash mismatch; failed setup\n"));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECRYPT_ERROR);
        return(ST_FAIL_SETUP);
    }

    // save session
    _SessionHistoryAdd(&pState->PeerAddr, pSecure->SessionId, sizeof(pSecure->SessionId), pSecure->MasterKey);

    // next state will be secure mode (handshaking complete) or send cipher change depending on client/server and if resuming or not
    return(pFinished->uNextState[pSecure->bSessionResume]);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateSend

    \Description
        Send data on a secure connection

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Output
        int32_t         - one if data was sent, else zero

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateSend(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    // handle send states if output buffer is empty
    if (pSecure->iSendProg == pSecure->iSendSize)
    {
        // deal with send states
        if (pState->iState == ST3_SEND_HELLO)
        {
            pState->iState = pState->bServer ? _ProtoSSLUpdateSendServerHello(pState) : _ProtoSSLUpdateSendClientHello(pState);
        }
        else if (pState->iState == ST3_SEND_CERT)
        {
            pState->iState = _ProtoSSLUpdateSendCertificate(pState);
        }
        else if (pState->iState == ST3_SEND_CERT_REQ)
        {
            pState->iState = _ProtoSSLUpdateSendCertificateRequest(pState);
        }
        else if (pState->iState == ST3_SEND_DONE)
        {
            pState->iState = _ProtoSSLUpdateSendServerHelloDone(pState);
        }
        else if (pState->iState == ST3_SEND_KEY)
        {
            pState->iState = pState->bServer ? _ProtoSSLUpdateSendServerKeyExchange(pState) : _ProtoSSLUpdateSendClientKeyExchange(pState);
        }
        else if (pState->iState == ST3_SEND_VERIFY)
        {
            pState->iState = _ProtoSSLUpdateSendCertificateVerify(pState);
        }
        else if (pState->iState == ST3_SEND_CHANGE)
        {
            pState->iState = _ProtoSSLUpdateSendChangeCipherSpec(pState);
        }
        else if (pState->iState == ST3_SEND_FINISH)
        {
            pState->iState = _ProtoSSLUpdateSendFinished(pState);
        }
    }

    // send any queued data
    return(_SendSecure(pState, pState->pSecure));
}

/*F**************************************************************************/
/*!
    \Function _RecvHandshake

    \Description
        Decode ssl3 handshake packet.

    \Input *pState      - module state reference
    \Input uType        - desired handshake packet type

    \Output
        void *          - pointer to handshake packet start

    \Version 11/10/2005 (gschaefer)
*/
/**************************************************************************F*/
static const void *_RecvHandshake(ProtoSSLRefT *pState, uint8_t uType)
{
    SecureStateT *pSecure = pState->pSecure;
    uint8_t *pRecv = pSecure->RecvData+pSecure->iRecvBase;

    // make sure it's a handshake message, and that we got desired packet
    if ((pSecure->RecvData[0] != SSL3_REC_HANDSHAKE) || (pRecv[0] != uType))
    {
        return(NULL);
    }

    // make sure we got entire packet
    pSecure->iRecvPackSize = (pRecv[1]<<16)|(pRecv[2]<<8)|(pRecv[3]<<0);
    if (pSecure->iRecvBase+4+pSecure->iRecvPackSize > pSecure->iRecvSize)
    {
        NetPrintf(("protossl: _RecvHandshake: packet at base %d is too long (%d vs %d)\n", pSecure->iRecvBase,
            pSecure->iRecvPackSize, pSecure->iRecvSize-pSecure->iRecvBase-4));
        return(NULL);
    }

    // point to data
    return(pRecv+4);
}

/*F**************************************************************************/
/*!
    \Function _RecvHandshakeFinish

    \Description
        Complete processing of handshake packet, including computation of
        handshake hash.

    \Input *pState      - module state reference

    \Version 03/16/2012 (jbrookes)
*/
/**************************************************************************F*/
static void _RecvHandshakeFinish(ProtoSSLRefT *pState)
{
    SecureStateT *pSecure = pState->pSecure;

    // hash all handshake data for "finish" packet (but only hash handshake packets that include multiple messages once)
    if ((pSecure->RecvData[0] == SSL3_REC_HANDSHAKE) && (pSecure->bHandshakeHashed == FALSE))
    {
        NetPrintfVerbose((DEBUG_MSG_LIST, 0, "protossl: _RecvPacket handshake update (size=%d)\n", pSecure->iRecvSize-pSecure->iRecvBase));
        CryptMD5Update(&pSecure->HandshakeMD5, pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase);
        CryptSha1Update(&pSecure->HandshakeSHA, pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase);
        CryptSha2Update(&pSecure->HandshakeSHA256, pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase);
        CryptSha2Update(&pSecure->HandshakeSHA384, pSecure->RecvData+pSecure->iRecvBase, pSecure->iRecvSize-pSecure->iRecvBase);
        pSecure->bHandshakeHashed = TRUE;
    }

    // consume the packet
    pSecure->iRecvBase += pSecure->iRecvPackSize+4;
    pSecure->iRecvPackSize = 0;

    // see if all the data was consumed
    if (pSecure->iRecvBase >= pSecure->iRecvSize)
    {
        pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = 0;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecvHandshake

    \Description
        Process handshake messages

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Todo
        - As per http://tools.ietf.org/html/rfc5246#section-7.4 "sending handshake
        messages in an unexpected order results in a fatal error" but we do not
        enforce this
        - Handshake functions should take data size argument so data out of range
        conditions can be detected and handled

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static void _ProtoSSLUpdateRecvHandshake(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    const void *pData;
    uint8_t bDecodeError = FALSE;
    int32_t iState = pState->iState;

    if (pSecure->RecvData[0] == SSL3_REC_ALERT)
    {
        pState->uAlertLevel = pSecure->RecvData[pSecure->iRecvBase];
        pState->uAlertValue = pSecure->RecvData[pSecure->iRecvBase+1];
        pState->bAlertSent = FALSE;
        if ((pState->uAlertLevel == SSL3_ALERT_LEVEL_WARNING) && (pState->uAlertValue == SSL3_ALERT_DESC_CLOSE_NOTIFY))
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: received close notification\n"));
            // only an error if we are still in setup
            if (pState->iState < ST3_SECURE)
            {
                pState->iState = ST_FAIL_SETUP;
            }
        }
        else
        {
            _DebugAlert(pState, pSecure->RecvData[pSecure->iRecvBase], pSecure->RecvData[pSecure->iRecvBase+1]);
            /* As per http://tools.ietf.org/html/rfc5246#section-7.2.2 any error alert sent or
               received must result in current session information being reset (no resume) */
            _SessionHistoryInvalidate(pSecure->SessionId);
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
        }
        pState->iClosed = 1;
        pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = 0;
    }
    else if (pState->iState == ST3_RECV_HELLO)
    {
        if ((pData = _RecvHandshake(pState, SSL3_MSG_CLIENT_HELLO)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvClientHello(pState, pData);
        }
        else if ((pData = _RecvHandshake(pState, SSL3_MSG_SERVER_HELLO)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvServerHello(pState, pData);
        }
        else if ((pData = _RecvHandshake(pState, SSL3_MSG_CERTIFICATE)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvCertificate(pState, pData);
        }
        else if ((pData = _RecvHandshake(pState, SSL3_MSG_CERT_REQ)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvCertificateRequest(pState, pData);
        }
        else if ((pData = _RecvHandshake(pState, SSL3_MSG_CERT_VERIFY)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvCertificateVerify(pState, pData);
        }
        else if ((pData = _RecvHandshake(pState, SSL3_MSG_CLIENT_KEY)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvClientKeyExchange(pState, pData);
        }
        else if ((pData = _RecvHandshake(pState, SSL3_MSG_SERVER_KEY)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvServerKeyExchange(pState, pData);
        }
        else if ((pData = _RecvHandshake(pState, SSL3_MSG_SERVER_DONE)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvServerHelloDone(pState, pData);
        }
        else
        {
            bDecodeError = TRUE;
        }
    }
    else if (pState->iState == ST3_RECV_CHANGE)
    {
        if (pSecure->RecvData[0] == SSL3_REC_CIPHER)
        {
            pState->iState = _ProtoSSLUpdateRecvChangeCipherSpec(pState, pSecure->RecvData+pSecure->iRecvBase);
            pSecure->iRecvBase = pSecure->iRecvSize;
        }
        else
        {
            bDecodeError = TRUE;
        }
    }
    else if (pState->iState == ST3_RECV_FINISH)
    {
        if ((pData = _RecvHandshake(pState, SSL3_MSG_FINISHED)) != NULL)
        {
            pState->iState = _ProtoSSLUpdateRecvFinished(pState, pData);
        }
        else
        {
            bDecodeError = TRUE;
        }
    }

    // output final crypto timing, if we have just reached secure state
    if ((iState != pState->iState) && (pState->iState == ST3_SECURE))
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: SSL Perf (setup) %dms\n", pSecure->uTimer));
    }

    if (bDecodeError == TRUE)
    {
        NetPrintf(("protossl: unhandled %d packet of type %d in state %d\n", pSecure->RecvData[0], pSecure->RecvData[pSecure->iRecvBase], pState->iState));
        _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_DECODE_ERROR);
        pState->iClosed = 1;
        pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = 0;
        pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
        return;
    }

    /* finish recv handshake processing
       due to the fact that the recv hello state handles which state we are
       in based on the packet data we want to move forward regardless.
       when we are not in the recv hello state we want to make sure to
       to only move to the next state when the state changes to allow
       for any additional processing that happens over multiple frames */
    if ((pState->iState == ST3_RECV_HELLO) || (iState != pState->iState))
    {
        _RecvHandshakeFinish(pState);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateRecv

    \Description
        Receive data on a secure connection

    \Input *pState      - module state reference
    \Input *pSecure     - secure state reference

    \Output
        int32_t         - 1=data was received, else 0

    \Version 03/08/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateRecv(ProtoSSLRefT *pState, SecureStateT *pSecure)
{
    int32_t iResult, iXfer = 0;

    // try and receive header data
    if (pSecure->iRecvSize < SSL_MIN_PACKET)
    {
        iResult = SocketRecv(pState->pSock, (char *)pSecure->RecvData+pSecure->iRecvSize, SSL_MIN_PACKET-pSecure->iRecvSize, 0);
        if (iResult > 0)
        {
            pSecure->iRecvSize += iResult;
            iXfer = 1;
        }
        if (iResult < 0)
        {
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
            pState->iClosed = 1;
        }
    }
    // wait for minimum ssl packet before passing this point
    if (pSecure->iRecvSize < SSL_MIN_PACKET)
    {
        return(iXfer);
    }

    // see if we can determine the full packet size
    if (pSecure->iRecvSize == SSL_MIN_PACKET)
    {
        pSecure->iRecvProg = pSecure->iRecvSize;

        /* validate the format version, and make sure it is a 3.x protocol version.  we make an assumption here that
           the 3.x packet format will not change in future revisions to the TLS protocol.  validation of the protocol
           version is handled during handshaking */
        if (pSecure->RecvData[1] == (SSL3_VERSION>>8))
        {
            // save data offset
            pSecure->iRecvBase = pSecure->iRecvSize;
            // add in the data length
            pSecure->iRecvSize += (pSecure->RecvData[3]<<8)|(pSecure->RecvData[4]<<0);
            // check data length to make sure it is valid; as per http://tools.ietf.org/html/rfc5246#section-6.2.1: The length MUST NOT exceed 2^14+2^11
            if (pSecure->iRecvSize > SSL_RCVMAX_PACKET)
            {
                NetPrintf(("protossl: received oversized packet (%d/%d); terminating connection\n", pSecure->iRecvSize, SSL_RCVMAX_PACKET));
                _SendAlert(pState, SSL3_ALERT_LEVEL_FATAL, SSL3_ALERT_DESC_RECORD_OVERFLOW);
                pState->iClosed = 1;
                pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = 0;
                pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
            }
        }
        else
        {
            if ((pSecure->RecvData[0] == 0x80) && (pSecure->RecvData[2] == 1))
            {
                NetPrintf(("protossl: received %d byte SSLv2 ClientHello offering SSLv%d.%d; disconnecting\n",
                    pSecure->RecvData[1], pSecure->RecvData[3], pSecure->RecvData[4]));
                pState->iState = ST_FAIL_CONN_SSL2;
            }
            else
            {
                NetPrintf(("protossl: received what appears to be a non-SSL connection attempt; disconnecting\n"));
                pState->iState = ST_FAIL_CONN_NOTSSL;
            }
            pState->iClosed = 1;
            pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = 0;
        }
    }

    // finish receiving packet data
    if (pSecure->iRecvProg < pSecure->iRecvSize)
    {
        iResult = SocketRecv(pState->pSock, (char *)pSecure->RecvData+pSecure->iRecvProg, pSecure->iRecvSize-pSecure->iRecvProg, 0);
        if (iResult > 0)
        {
            pSecure->iRecvProg += iResult;
            /* check for completion -- we do this here so this code is only executed once, as
               _ProtoSSLUpdateRecv can be called multiple times with completed packets */
            if (pSecure->iRecvProg == pSecure->iRecvSize)
            {
                #if DIRTYCODE_DEBUG
                // fill in junk data after end of received packet data to make sure we aren't referencing outside our received data
                ds_memset(pSecure->RecvData+pSecure->iRecvSize, 0xcc, SSL_BUFFER_DBGFENCE);
                #endif
                // at end of receive, process the packet
                iResult = _RecvPacket(pState);
            }
            iXfer = 1;
        }
        if (iResult < 0)
        {
            pState->iState = (pState->iState < ST3_SECURE) ? ST_FAIL_SETUP : ST_FAIL_SECURE;
            pState->iClosed = 1;
        }
    }

    // process handshake packets
    if ((pSecure->iRecvProg == pSecure->iRecvSize) && (pSecure->RecvData[0] != SSL3_REC_APPLICATION) && (pState->iClosed == 0))
    {
        _ProtoSSLUpdateRecvHandshake(pState, pSecure);
    }
    return(iXfer);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLInitiateCARequest

    \Description
        Initiate CA fetch request

    \Input *pState      - module state reference

    \Output
        int32_t         - 0=success, negative=failure

    \Version 02/28/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLInitiateCARequest(ProtoSSLRefT *pState)
{
    // we allow only one fetch request for each ssl negotiation attempt
    if (pState->bCertInfoSet && (pState->iCARequestId <= 0))
    {
        if ((pState->iCARequestId = DirtyCertCARequestCert(&pState->CertInfo, pState->strHost, SockaddrInGetPort(&pState->PeerAddr))) > 0)
        {
            // save the failure cert, it will be validated again after fetching CA cert
            if (pState->pCertToVal == NULL)
            {
                pState->pCertToVal = DirtyMemAlloc(sizeof(*pState->pCertToVal), PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            }
            // continue to fetch CA cert even if we fail to allocate cert memory
            if (pState->pCertToVal != NULL)
            {
                ds_memcpy(pState->pCertToVal, &pState->pSecure->Cert, sizeof(*pState->pCertToVal));
            }
            return(0);
        }
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoSSLUpdateCARequest

    \Description
        Update CA fetch request status

    \Input *pState      - module state reference

    \Output
        int32_t         - updated module state

    \Version 02/28/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _ProtoSSLUpdateCARequest(ProtoSSLRefT *pState)
{
    int32_t iComplete;
    // see if request completed
    if ((iComplete = DirtyCertCARequestDone(pState->iCARequestId)) != 0)
    {
        DirtyCertCARequestFree(pState->iCARequestId);
        pState->iCARequestId = 0;
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
            #if DIRTYCODE_LOGGING
            char strIdentSubject[512], strIdentIssuer[512];
            NetPrintfVerbose((pState->iVerbose, 0, "protossl: cert (%s) validated by ca (%s)\n", _DebugFormatCertIdent(&pState->pCertToVal->Subject, strIdentSubject, sizeof(strIdentSubject)),
                _DebugFormatCertIdent(&pState->pCertToVal->Issuer, strIdentIssuer, sizeof(strIdentIssuer))));
            #endif
            pState->iState = ST3_RECV_HELLO;
        }

        if (pState->pCertToVal != NULL)
        {
            DirtyMemFree(pState->pCertToVal, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pState->pCertToVal = NULL;
        }
    }
    return(pState->iState);
}

/*F***************************************************************************/
/*!
    \Function _ProtoSSLSetSockOpt

    \Description
        Set socket options

    \Input  *pState     - module state

    \Version 09/12/2012 (jbrookes) Refactored from ProtoSSLBind() and ProtoSSLConnect()
*/
/***************************************************************************F*/
static void _ProtoSSLSetSockOpt(ProtoSSLRefT *pState)
{
    // set debug level
    SocketControl(pState->pSock, 'spam', pState->iVerbose, NULL, NULL);

    // set recv/send buffer size?
    if (pState->iRecvBufSize != 0)
    {
        SocketControl(pState->pSock, 'rbuf', pState->iRecvBufSize, NULL, NULL);
    }
    if (pState->iSendBufSize != 0)
    {
        SocketControl(pState->pSock, 'sbuf', pState->iSendBufSize, NULL, NULL);
    }

    // set max send/recv rate?
    if (pState->iMaxRecvRate != 0)
    {
        SocketControl(pState->pSock, 'maxr', pState->iMaxRecvRate, NULL, NULL);
    }
    if (pState->iMaxSendRate != 0)
    {
        SocketControl(pState->pSock, 'maxs', pState->iMaxSendRate, NULL, NULL);
    }

    // set keep-alive options?
    if (pState->bKeepAlive != 0)
    {
        SocketControl(pState->pSock, 'keep', pState->bKeepAlive, &pState->uKeepAliveTime, &pState->uKeepAliveTime);
    }

    // set nodelay?
    if (pState->bNoDelay)
    {
        SocketControl(pState->pSock, 'ndly', TRUE, NULL, NULL);
    }

    // set reuseaddr
    if (pState->bReuseAddr)
    {
        SocketControl(pState->pSock, 'radr', TRUE, NULL, NULL);
    }
}


/*** Public functions ********************************************************/


/*F***************************************************************************/
/*!
    \Function ProtoSSLStartup

    \Description
        Start up ProtoSSL.  Used to create global state shared across SSL refs.

    \Output
        int32_t         - negative=failure, else success

    \Version 09/14/2012 (jbrookes)
*/
/***************************************************************************F*/
int32_t ProtoSSLStartup(void)
{
    ProtoSSLStateT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // make sure we haven't already been called
    if (_ProtoSSL_pState != NULL)
    {
        NetPrintf(("protossl: global state already allocated\n"));
        return(-1);
    }

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pState = DirtyMemAlloc(sizeof(*pState), PROTOSSL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protossl: could not allocate global state\n"));
        return(-1);
    }
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    NetCritInit(&pState->StateCrit, "ProtoSSL Global State");

    // initalize cyptrand module
    CryptRandInit();

    // save state ref
    _ProtoSSL_pState = pState;
    return(0);
}

/*F***************************************************************************/
/*!
    \Function ProtoSSLShutdown

    \Description
        Shut down ProtoSSL.  Cleans up global state.

    \Version 09/14/2012 (jbrookes)
*/
/***************************************************************************F*/
void ProtoSSLShutdown(void)
{
    ProtoSSLStateT *pState = _ProtoSSL_pState;

    // make sure we haven't already been called
    if (pState == NULL)
    {
        NetPrintf(("protossl: global state not allocated\n"));
        return;
    }

    // shutdown cyptrand module
    CryptRandShutdown();

    // shut down, deallocate resources
    NetCritKill(&pState->StateCrit);
    DirtyMemFree(pState, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // clear state pointer
    _ProtoSSL_pState = NULL;
}

/*F***************************************************************************/
/*!
    \Function ProtoSSLCreate

    \Description
        Allocate an SSL connection and prepare for use

    \Output
        ProtoSSLRefT * - module state; NULL=failure

    \Version 03/08/2002 (gschaefer)
*/
/***************************************************************************F*/
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
    ds_memclr(pState, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iLastSocketError = SOCKERR_NONE;
    pState->iVerbose = SSL_VERBOSE_DEFAULT;
    pState->uSslVersion = SSL3_VERSION;
    pState->uSslVersionMin = SSL3_VERSION_MIN;
    pState->bSessionResumeEnabled = TRUE;

    // all ciphers enabled by default
    pState->uEnabledCiphers = PROTOSSL_CIPHER_ALL;
    #ifdef DIRTYCODE_XBOXONE // disable RC4&MD5 suites on xboxone
    pState->uEnabledCiphers &= ~(PROTOSSL_CIPHER_RSA_WITH_RC4_128_SHA|PROTOSSL_CIPHER_RSA_WITH_RC4_128_MD5);
    #endif

    // enable hello extension by default
    pState->uHelloExtn = PROTOSSL_HELLOEXTN_DEFAULT;

    // init secure state critical section
    NetCritInit(&pState->SecureCrit, "ProtoSSL Secure State");

    // return module state
    return(pState);
}

/*F**************************************************************************/
/*!
    \Function ProtoSSLReset

    \Description
        Reset connection back to unconnected state (will disconnect from server).

    \Input *pState      - module state reference

    \Version 03/08/2002 (gschaefer)
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

    \Input *pState      - module state reference

    \Version 03/08/2002 (gschaefer)
*/
/*************************************************************************F*/
void ProtoSSLDestroy(ProtoSSLRefT *pState)
{
    // reset to unsecure mode (free all secure resources)
    _ResetState(pState, 0);
    // kill critical section
    NetCritKill(&pState->SecureCrit);
    // free remaining state
    DirtyMemFree(pState, PROTOSSL_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLAccept

    \Description
        Accept an incoming connection.

    \Input *pState      - module state reference
    \Input iSecure      - flag indicating use of secure connection: 1=secure, 0=unsecure
    \Input *pAddr       - where the client's address should be written
    \Input *pAddrlen    - the length of the client's address space

    \Output
        ProtoSSLRefT *  - accepted connection or NULL if not available

    \Version 10/27/2013 (jbrookes)
*/
/*************************************************************************F*/
ProtoSSLRefT *ProtoSSLAccept(ProtoSSLRefT *pState, int32_t iSecure, struct sockaddr *pAddr, int32_t *pAddrlen)
{
    ProtoSSLRefT *pClient;
    SocketT *pSocket;

    // check for connect
    pSocket = SocketAccept(pState->pSock, pAddr, pAddrlen);
    if (pSocket == NULL)
    {
        return(NULL);
    }

    // we have an incoming connection attempt, so create an ssl object for it
    DirtyMemGroupEnter(pState->iMemGroup, pState->pMemGroupUserData);
    pClient = ProtoSSLCreate();
    DirtyMemGroupLeave();
    if (pClient == NULL)
    {
        SocketClose(pSocket);
        return(NULL);
    }

    // set up new ssl object with the socket we just accepted
    if (_ResetState(pClient, iSecure) != SOCKERR_NONE)
    {
        ProtoSSLDestroy(pClient);
        return(NULL);
    }
    pClient->pSock = pSocket;
    ds_memcpy(&pClient->PeerAddr, pAddr, *pAddrlen);

    // update socket status
    SocketInfo(pClient->pSock, 'stat', 0, NULL, 0);

    // set client state
    pClient->iState = (pClient->pSecure ? ST3_RECV_HELLO : ST_UNSECURE);
    pClient->iClosed = 0;
    pClient->bServer = TRUE;
    return(pClient);
}

/*F*************************************************************************/
/*!
    \Function ProtoSSLBind

    \Description
        Create a socket bound to the given address.

    \Input *pState  - module state reference
    \Input *pAddr   - the IPv4 address
    \Input iAddrlen - the size of the IPv4 address.

    \Output
        int32_t     - SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/03/2004 (sbevan)
*/
/*************************************************************************F*/
int32_t ProtoSSLBind(ProtoSSLRefT *pState, const struct sockaddr *pAddr, int32_t iAddrlen)
{
    // if we had a socket, get last error from it and close it
    if (pState->pSock != NULL)
    {
        pState->iLastSocketError = SocketInfo(pState->pSock, 'serr', 0, NULL, 0);
        SocketClose(pState->pSock);
    }

    // create the socket
    if ((pState->pSock = SocketOpen(AF_INET, SOCK_STREAM, 0)) == NULL)
    {
        return(SOCKERR_OTHER);
    }

    // set socket options
    _ProtoSSLSetSockOpt(pState);

    // do the bind, return result
    return(SocketBind(pState->pSock, pAddr, iAddrlen));
}

/*F**************************************************************************/
/*!
    \Function ProtoSSLConnect

    \Description
        Make a secure connection to an SSL server.

    \Input *pState  - module state reference
    \Input iSecure  - flag indicating use of secure connection (1=secure, 0=unsecure)
    \Input *pAddr   - textual form of address (1.2.3.4 or www.ea.com)
    \Input uAddr    - the IP address of the server (if not in textual form)
    \Input iPort    - the TCP port of the server (if not in textual form)

    \Output
        int32_t     - SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/08/2002 (gschaefer)
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

    // only allow secure connection if dirtycert service name has been set
    if ((iSecure != 0) && (DirtyCertStatus('snam', NULL, 0) < 0))
    {
        NetPrintf(("protossl: ************************************************************************************\n"));
        NetPrintf(("protossl: ProtoSSLConnect() requires a valid DirtyCert service name in the format\n"));
        NetPrintf(("protossl: \"game-year-platform\", set when calling NetConnStartup() by using the -servicename\n"));
        NetPrintf(("protossl: argument, to be set before SSL use is allowed.  If a service name doesn't include\n"));
        NetPrintf(("protossl: dashes it is assumed to simply be the 'game' identifier, in which case DirtySDK will\n"));
        NetPrintf(("protossl: fill in the year and platform.  For titles using BlazeSDK, the service name specified\n"));
        NetPrintf(("protossl: must match the BlazeSDK service name, therefore the full name should be specified.\n"));
        NetPrintf(("protossl: *** PLEASE SET A UNIQUE AND MEANINGFUL SERVICE NAME, EVEN FOR TOOL OR SAMPLE USE ***\n"));
        return(SOCKERR_INVALID);
    }

    // allocate the socket
    if ((pState->pSock = SocketOpen(AF_INET, SOCK_STREAM, 0)) == NULL)
    {
        return(SOCKERR_NORSRC);
    }

    // set socket options
    _ProtoSSLSetSockOpt(pState);

    // init peer structure
    SockaddrInit(&pState->PeerAddr, AF_INET);

    // clear previous cert info, if any
    pState->bCertInfoSet = FALSE;
    ds_memclr(&pState->CertInfo, sizeof(pState->CertInfo));

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
        pState->pHost = SocketLookup(pState->strHost, 30*1000);
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

    \Output
        int32_t     - SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/03/2004 (sbevan)
*/
/**************************************************************************F*/
int32_t ProtoSSLListen(ProtoSSLRefT *pState, int32_t iBacklog)
{
    return(SocketListen(pState->pSock, iBacklog));
}

/*F*************************************************************************************************/
/*!
    \Function ProtoSSLDisconnect

    \Description
        Disconnect from the server

    \Input *pState  - module state reference

    \Output
        int32_t     - SOCKERR_xxx (zero=success, negative=failure)

    \Version 03/08/2002 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t ProtoSSLDisconnect(ProtoSSLRefT *pState)
{
    if (pState->pSock)
    {
        // send a close_notify alert as per http://tools.ietf.org/html/rfc5246#section-7.2.1
        if ((pState->pSecure != NULL) && (pState->iState == ST3_SECURE))
        {
            // send the alert
            _SendAlert(pState, SSL3_ALERT_LEVEL_WARNING, SSL3_ALERT_DESC_CLOSE_NOTIFY);
        }

        /* if in server mode, just close the write side.  on the client we do an immediate hard
           close of the socket as the calling application that is polling to drive our operation
           may stop updating us once they have received all of the data, which would prevent us
           from closing the socket */
        if (pState->bServer)
        {
            SocketShutdown(pState->pSock, SOCK_NOSEND);
        }
        else
        {
            SocketClose(pState->pSock);
            pState->pSock = NULL;
        }
    }

    pState->iState = ST_IDLE;
    pState->iClosed = 1;

    // done with dirtycert
    if (pState->iCARequestId > 0)
    {
        DirtyCertCARequestFree(pState->iCARequestId);
    }
    pState->iCARequestId = 0;
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
void ProtoSSLUpdate(ProtoSSLRefT *pState)
{
    int32_t iXfer;
    int32_t iResult;
    SecureStateT *pSecure = pState->pSecure;

    // resolve the address
    if (pState->iState == ST_ADDR)
    {
        // check for completion
        if (pState->pHost->Done(pState->pHost))
        {
            pState->iState = (pState->pHost->addr != 0) ? ST_CONN : ST_FAIL_DNS;
            SockaddrInSetAddr(&pState->PeerAddr, pState->pHost->addr);
            // free the record
            pState->pHost->Free(pState->pHost);
            pState->pHost = NULL;
        }
    }

    // see if we should start a connection
    if (pState->iState == ST_CONN)
    {
        // start the connection attempt
        if ((iResult = SocketConnect(pState->pSock, &pState->PeerAddr, sizeof pState->PeerAddr)) == SOCKERR_NONE)
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
            pState->iState = pSecure ? ST3_SEND_HELLO : ST_UNSECURE;
            pState->iClosed = 0;
        }
        if (iResult < 0)
        {
            pState->iState = ST_FAIL_CONN;
            pState->iClosed = 1;
        }
    }

    // handle secure i/o (non-secure is done immediately in ProtoSSLSend/ProtoSSLRecv)
    while ((pState->pSock != NULL) && (pState->iState >= ST3_SEND_HELLO) && (pState->iState <= ST3_SECURE))
    {
        // get access to secure state
        NetCritEnter(&pState->SecureCrit);

        // update send processing
        iXfer = _ProtoSSLUpdateSend(pState, pSecure);

        // update recv processing
        iXfer += _ProtoSSLUpdateRecv(pState, pSecure);

        // release access to secure state
        NetCritLeave(&pState->SecureCrit);

        // break out of loop if no i/o activity
        if (iXfer == 0)
        {
            break;
        }
    }

    // wait for CA cert (this comes last intentionally)
    if (pState->iState == ST_WAIT_CA)
    {
        // acquire secure state crit section to guard dirtycert access
        NetCritEnter(&pState->SecureCrit);

        // update CA request processing
        _ProtoSSLUpdateCARequest(pState);

        // release secure state crit section
        NetCritLeave(&pState->SecureCrit);
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
    \Input iLength  - length of data (if negative, input data is assumed to be null-terminated string)

    \Output
        int32_t     - negative=error, otherwise number of bytes sent

    \Version 06/03/2002 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t ProtoSSLSend(ProtoSSLRefT *pState, const char *pBuffer, int32_t iLength)
{
    int32_t iResult = SOCKERR_CLOSED;
    SecureStateT *pSecure = pState->pSecure;

    // allow easy string sends
    if (iLength < 0)
    {
        iLength = (int32_t)strlen(pBuffer);
    }
    // guard against zero-length sends, which can result in an invalid send condition with some stream (e.g. RC4) ciphers
    if (iLength == 0)
    {
        return(0);
    }

    // make sure connection established
    if (pState->iState == ST3_SECURE)
    {
        iResult = 0;

        // get access to secure state
        NetCritEnter(&pState->SecureCrit);

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

        // release access to secure state
        NetCritLeave(&pState->SecureCrit);
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

    \Version 03/08/2002 (gschaefer)
*/
/*************************************************************************************************F*/
int32_t ProtoSSLRecv(ProtoSSLRefT *pState, char *pBuffer, int32_t iLength)
{
    SecureStateT *pSecure = pState->pSecure;
    int32_t iResult = 0;

    // make sure in right state
    if (pState->iState == ST3_SECURE)
    {
        // get access to secure state
        NetCritEnter(&pState->SecureCrit);

        // check for more data if no packet pending
        if ((pSecure->iRecvProg == 0) || (pSecure->iRecvProg != pSecure->iRecvSize))
        {
            ProtoSSLUpdate(pState);
        }

        // check for end of data
        if (((pSecure->iRecvSize < SSL_MIN_PACKET) || (pSecure->iRecvProg < pSecure->iRecvSize)) && (pState->iClosed))
        {
            iResult = SOCKERR_CLOSED;
        }
        // see if data pending
        else if ((pSecure->iRecvProg == pSecure->iRecvSize) && (pSecure->iRecvBase < pSecure->iRecvSize) &&
            (pSecure->RecvData[0] == SSL3_REC_APPLICATION))
        {
            iResult = pSecure->iRecvSize-pSecure->iRecvBase;
            // only return what user can store
            if (iResult > iLength)
            {
                iResult = iLength;
            }
            // return the data
            ds_memcpy(pBuffer, pSecure->RecvData+pSecure->iRecvBase, iResult);
            pSecure->iRecvBase += iResult;

            // see if we can grab a new packet
            if (pSecure->iRecvBase >= pSecure->iRecvSize)
            {
                pSecure->iRecvProg = pSecure->iRecvSize = pSecure->iRecvBase = 0;
            }
        }

        // release access to secure state
        NetCritLeave(&pState->SecureCrit);
    }

    // handle unsecure receive
    if (pState->iState == ST_UNSECURE)
    {
        iResult = SocketRecv(pState->pSock, pBuffer, iLength, 0);
    }

    // return error if in failure state
    if (pState->iState >= ST_FAIL)
    {
        iResult = -1;
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

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'addr'      Address of peer we are connecting/connected to
            'alpn'      Get the negotiated protocol using the ALPN extension from the secure state
            'alrt'      Return alert status 0=no alert, 1=recv alert, 2=sent alert; alert desc copied to pBuffer
            'cert'      Return SSL cert info (valid after 'fail')
            'cfip'      TRUE/FALSE indication if a CA fetch is in progress
            'ciph'      Cipher suite negotiated (string name in output buffer)
            'fail'      Return PROTOSSL_ERROR_* or zero if no error
            'hres'      Return hResult containing either the socket error or ssl error
            'htim'      Return timing of most recent SSL handshake in milliseconds
            'maxr'      Return max recv rate (0=uncapped)
            'maxs'      Return max send rate (0=uncapped)
            'recv'      Return number of bytes in recv buffer (secure only)
            'resu'      Returns whether session was resumed or not
            'rsao'      Returns whether we are performing any RSA operations at this time
            'send'      Return number of bytes in send buffer (secure only)
            'serr'      Return socket error
            'sock'      Copy SocketT ref to output buffer
            'stat'      Return like SocketInfo('stat')
            'vers'      Return current SSL version
        \endverbatim

    \Version 03/08/2002 (gschaefer)
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
        if ((pBuffer != NULL) && (iLength == sizeof(pState->PeerAddr)))
        {
            ds_memcpy(pBuffer, &pState->PeerAddr, iLength);
        }
        return(SockaddrInGetAddr(&pState->PeerAddr));
    }
    if ((iSelect == 'alpn') && (pState->pSecure != NULL))
    {
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, pState->pSecure->strAlpnProtocol, iLength);
        }

        return(0);
    }
    // return most recent alert if any
    if (iSelect == 'alrt')
    {
        if ((pBuffer != NULL) && (iLength == sizeof(ProtoSSLAlertDescT)))
        {
            ProtoSSLAlertDescT Alert;
            if ((iResult = _GetAlert(pState, &Alert, pState->uAlertLevel, pState->uAlertValue)) != 0)
            {
                ds_memcpy(pBuffer, &Alert, sizeof(Alert));
                iResult = pState->bAlertSent ? 2 : 1;
            }
        }
        return(iResult);
    }
    // return certificate info (valid after 'fail' response)
    if ((iSelect == 'cert') && (pBuffer != NULL) && (iLength == sizeof(pState->CertInfo)))
    {
        ds_memcpy(pBuffer, &pState->CertInfo, sizeof(pState->CertInfo));
        return(0);
    }
    // return if a CA fetch request is in progress
    if (iSelect == 'cfip')
    {
        return((pState->iState == ST_WAIT_CA) ? 1 : 0);
    }
    // return current cipher suite
    if ((iSelect == 'ciph') && (pState->pSecure != NULL) && (pState->pSecure->pCipher != NULL))
    {
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, pState->pSecure->pCipher->strName, iLength);
        }
        return(pState->pSecure->pCipher->uId);
    }
    // return timing of most recent SSL handshake in milliseconds
    if ((iSelect == 'htim') && (pState->pSecure != NULL))
    {
        return(pState->pSecure->uTimer);
    }
    // return configured max receive rate
    if (iSelect == 'maxr')
    {
        return(pState->iMaxRecvRate);
    }
    // return configured max send rate
    if (iSelect == 'maxs')
    {
        return(pState->iMaxSendRate);
    }
    // return number of bytes in recv buffer (useful only when connection type is secure)
    if (iSelect == 'recv')
    {
        return((pState->pSecure != NULL) ? pState->pSecure->iRecvSize-pState->pSecure->iRecvProg : 0);
    }
    // return whether session was resumed or not
    if ((iSelect == 'resu') && (pState->pSecure != NULL))
    {
        return(pState->pSecure->bSessionResume);
    }
    // return whether we are performing any rsa operations at this time
    if (iSelect == 'rsao')
    {
        return((pState->iState == ST3_SEND_KEY) || (pState->iState == ST3_SEND_VERIFY));
    }
    // return number of bytes in send buffer (useful only when connection type is secure)
    if (iSelect == 'send')
    {
        return((pState->pSecure != NULL) ? pState->pSecure->iSendSize-pState->pSecure->iSendProg : 0);
    }
    // return last socket error
    if (iSelect == 'serr')
    {
        // pass through to socket module if we have a socket, else return cached last error
        return((pState->pSock != NULL) ? SocketInfo(pState->pSock, iSelect, 0, pBuffer, iLength) : pState->iLastSocketError);
    }
    // return socket ref
    if (iSelect == 'sock')
    {
        if ((pBuffer == NULL) || (iLength != sizeof(pState->pSock)))
        {
            return(-1);
        }
        ds_memcpy(pBuffer, &pState->pSock, sizeof(pState->pSock));
        return(0);
    }
    // return current ssl version for the connection
    if ((iSelect == 'vers') && (pState->pSecure != NULL))
    {
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, _SSL3_strVersionNames[pState->pSecure->uSslVersion&0xff], iLength);
        }
        return(pState->pSecure->uSslVersion);
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
                case ST_FAIL_CONN_SSL2:
                    iResult = PROTOSSL_ERROR_CONN_SSL2;
                    break;
                case ST_FAIL_CONN_NOTSSL:
                    iResult = PROTOSSL_ERROR_CONN_NOTSSL;
                    break;
                case ST_FAIL_CONN_MINVERS:
                    iResult = PROTOSSL_ERROR_CONN_MINVERS;
                    break;
                case ST_FAIL_CONN_MAXVERS:
                    iResult = PROTOSSL_ERROR_CONN_MAXVERS;
                    break;
                case ST_FAIL_CONN_NOCIPHER:
                    iResult = PROTOSSL_ERROR_CONN_NOCIPHER;
                    break;
                case ST_FAIL_CERT_NONE:
                    iResult = PROTOSSL_ERROR_CERT_MISSING;
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
                case ST_FAIL_CERT_BADDATE:
                    iResult = PROTOSSL_ERROR_CERT_BADDATE;
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

    if (iSelect == 'hres')
    {
        uint32_t hResult;
        int32_t iSerr = ProtoSSLStat(pState, 'serr', NULL, 0);
        int32_t iFail = ProtoSSLStat(pState, 'fail', NULL, 0);

        if (iSerr < SOCKERR_CLOSED)
        {
            hResult = DirtyErrGetHResult(DIRTYAPI_SOCKET, iSerr, TRUE);
        }
        else if (iFail != 0)
        {
            hResult = DirtyErrGetHResult(DIRTYAPI_PROTO_SSL, iFail, TRUE);
        }
        else
        {
            hResult = DirtyErrGetHResult(DIRTYAPI_PROTO_SSL, 0, FALSE); //success hResult
        }
        return(hResult);
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
            'alpn'      Set the ALPN protocols (using IANA registerd named) as a comma delimited list
            'ccrt'      Set client certificate level (0=disabled, 1=requested, 2=required)
            'ciph'      Set enabled/disabled ciphers (PROTOSSL_CIPHER_*)
            'extn'      Set enabled ClientHello extensions (PROTOSSL_HELLOEXTN_*), (default=0=disabled)
            'host'      Set remote host
            'maxr'      Set max recv rate (0=uncapped, default)
            'maxs'      Set max send rate (0=uncapped, default)
            'ncrt'      Disable client certificate validation
            'rbuf'      Set socket recv buffer size (must be called before Connect())
            'resu'      Set whether session resume is enabled or disabled (default=1=enabled)
            'sbuf'      Set socket send buffer size (must be called before Connect())
            'scrt'      Set certificate (pValue=cert, iValue=len)
            'snod'      Set whether TCP_NODELAY option is enabled or disabled on the socket (must be called before Connect())
            'secu'      Start secure negotiation on an established unsecure connection
            'skey'      Set private key (pValue=key, iValue=len)
            'skep'      Set socket keep-alive settings (iValue=enable/disable, iValue2=keep-alive time/interval)
            'spam'      Set debug logging level (default=1)
            'vers'      Set client-requested SSL version (default=0x302, TLS1.1)
            'vmin'      Set minimum SSL version application will accept
        \endverbatim

    \Version 01/27/2009 (jbrookes)
*/
/**************************************************************************F*/
int32_t ProtoSSLControl(ProtoSSLRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    int32_t iResult = -1;

    if (iSelect == 'alpn')
    {
        uint16_t uProtocol, uAlpnExtensionLength;
        const char *pSrc = (const char *)pValue;
        if (pSrc == NULL)
        {
            NetPrintf(("protossl: invalid ALPN extension protocol list provided\n"));
            return(-1);
        }
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: setting the ALPN extension protocols using %s\n", (const char *)pValue));
        ds_memclr(pState->aAlpnProtocols, sizeof(pState->aAlpnProtocols));

        for (uProtocol = 0, uAlpnExtensionLength = 0; uProtocol < SSL_ALPN_MAX_PROTOCOLS; uProtocol += 1)
        {
            AlpnProtocolT *pProtocol = &pState->aAlpnProtocols[uProtocol];
            if ((pProtocol->uLength = (uint8_t)_SplitDelimitedString(pSrc, ',', pProtocol->strName, sizeof(pProtocol->strName), &pSrc)) == 0)
            {
                break;
            }

            uAlpnExtensionLength += pProtocol->uLength;
            uAlpnExtensionLength += sizeof(pProtocol->uLength);
        }
        pState->uNumAlpnProtocols = uProtocol;
        pState->uAlpnExtensionLength = uAlpnExtensionLength;

        return(0);
    }
    if (iSelect == 'ccrt')
    {
        NetPrintf(("protossl: setting client cert level to %d\n", iValue));
        pState->iClientCertLevel = iValue;
        return(0);
    }
    if (iSelect == 'ciph')
    {
        pState->uEnabledCiphers = (uint32_t)iValue;
        #ifdef DIRTYCODE_XBOXONE // disable RC4&MD5 suites on xboxone
        pState->uEnabledCiphers &= ~(PROTOSSL_CIPHER_RSA_WITH_RC4_128_SHA|PROTOSSL_CIPHER_RSA_WITH_RC4_128_MD5);
        #endif
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: enabled ciphers=%d\n", iValue));
        return(0);
    }
    if (iSelect == 'extn')
    {
        pState->uHelloExtn = (uint8_t)iValue;
        NetPrintfVerbose((pState->iVerbose, 0, "protossl: clienthello extensions set to 0x%02x\n", pState->uHelloExtn));
        return(0);
    }
    if (iSelect == 'host')
    {
        NetPrintf(("protossl: setting host to '%s'\n", (const char *)pValue));
        ds_strnzcpy(pState->strHost, (const char *)pValue, sizeof(pState->strHost));
        return(0);
    }
    if (iSelect == 'maxr')
    {
        pState->iMaxRecvRate = iValue;
        if (pState->pSock != NULL)
        {
            SocketControl(pState->pSock, iSelect, iValue, NULL, NULL);
        }
        return(0);
    }
    if (iSelect == 'maxs')
    {
        pState->iMaxSendRate = iValue;
        if (pState->pSock != NULL)
        {
            SocketControl(pState->pSock, iSelect, iValue, NULL, NULL);
        }
        return(0);
    }
    if (iSelect == 'ncrt')
    {
        pState->bAllowAnyCert = (uint8_t)iValue;
        return(0);
    }
    if (iSelect == 'radr')
    {
        pState->bReuseAddr = TRUE;
        return(0);
    }
    if (iSelect == 'rbuf')
    {
        pState->iRecvBufSize = iValue;
        return(0);
    }
    if (iSelect == 'resu')
    {
        pState->bSessionResumeEnabled = iValue ? TRUE : FALSE;
        return(0);
    }
    if (iSelect == 'sbuf')
    {
        pState->iSendBufSize = iValue;
        return(0);
    }
    if (iSelect == 'scrt')
    {
        NetPrintf(("protossl: setting cert (%d bytes)\n", iValue));
        pState->pCertificate = (char *)pValue;
        pState->iCertificateLen = iValue;
        return(0);
    }
    if (iSelect == 'snod')
    {
        pState->bNoDelay = (uint8_t)iValue;
        return(0);
    }
    if (iSelect == 'skey')
    {
        NetPrintf(("protossl: setting private key (%d bytes)\n", iValue));
        pState->pPrivateKey = (char *)pValue;
        pState->iPrivateKeyKen = iValue;
        return(0);
    }
    if (iSelect == 'skep')
    {
        pState->bKeepAlive = (uint8_t)iValue;
        pState->uKeepAliveTime = (uint32_t)iValue2;
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
    if (iSelect == 'spam')
    {
        pState->iVerbose = (int8_t)iValue;
        return(0);
    }
    if (iSelect == 'vers')
    {
        uint32_t uSslVersion = DS_CLAMP(iValue, pState->uSslVersionMin, SSL3_VERSION_MAX);
        if (pState->uSslVersion != uSslVersion)
        {
            NetPrintf(("protossl: setting sslvers to %s (0x%04x)\n", _SSL3_strVersionNames[uSslVersion&0xff], uSslVersion));
            pState->uSslVersion = uSslVersion;
        }
        return(0);
    }
    if (iSelect == 'vmin')
    {
        uint32_t uSslVersionMin = DS_CLAMP(iValue, SSL3_VERSION_MIN, SSL3_VERSION_MAX);
        if (pState->uSslVersionMin != uSslVersionMin)
        {
            NetPrintf(("protossl: setting min sslvers to %s (0x%04x)\n", _SSL3_strVersionNames[uSslVersionMin&0xff], uSslVersionMin));
            pState->uSslVersionMin = uSslVersionMin;
            // make sure requested version is at least minimum version
            ProtoSSLControl(pState, 'vers', pState->uSslVersion, 0, NULL);
        }
        return(0);
    }
    // if we have a socket ref, pass unhandled selector through
    if (pState->pSock != NULL)
    {
        iResult = SocketControl(pState->pSock, iSelect, iValue, pValue, NULL);
    }
    else
    {
        NetPrintf(("protossl: ProtoSSLControl('%C') unhandled\n", iSelect));
    }
    return(iResult);
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
                #if DIRTYCODE_LOGGING
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
