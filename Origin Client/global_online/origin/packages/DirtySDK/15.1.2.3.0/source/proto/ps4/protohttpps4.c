/*H********************************************************************************/
/*!
    \File protohttpps4.c

    \Description
        This module implements an HTTP client that can perform basic transactions
        (get/put) with an HTTP server using the PS4 SDK.  

    \Copyright
        Copyright (c) Electronic Arts 2016-2020. ALL RIGHTS RESERVED.

    \Notes
        Proxy servers are not supported in the PS4 implementation.

    \Version 0.5 07/05/2016 (amakoukji) First Version
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <net.h>
#include <libhttp.h>

#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/ps4/dirtycontextmanagerps4.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/proto/protohttp.h"

/*** Defines **********************************************************************/

//! default ProtoHttp timeouts
#define PROTOHTTP_TIMEOUT               (30*1000)
#define PROTOHTTP_RESOLVE_RETRIES       (1)

//! default maximum allowed redirections
#define PROTOHTTP_MAXREDIRECT   (3)

//! size of "last-received" header cache
#define PROTOHTTP_HDRCACHESIZE  (1024)

//! protohttp revision number (maj.min)
#define PROTOHTTP_VERSION       (0x0103)          // update this for major bug fixes or protocol additions/changes

//! pool size, Sony only supports multiples of 16KiB
#define PROTOHTTP_POOLSIZE      (16*1024)
#define PROTOHTTP_SSL_POOLSIZE  (304 * 1024)

#define PROTOHTTP_GOSCA_COUNT   (3)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct ProtoHttpCACertT
{
    uint8_t *pCACert;
    int32_t iCertSize;
    struct ProtoHttpCACertT *pNext;  //!< link to next cert in list
} ProtoHttpCACertT;

//! http module state
struct ProtoHttpRefT
{
    int32_t iNetMemId;      //!< sce net context id
    int32_t iSslContextId;  //!< sce ssl context id
    int32_t iHttpContextId; //!< sce http context id
    int32_t iSceConnId;     //!< sce connection id
    int32_t iSceTemplateId; //!< sce template id
    int32_t iSceRequestId;  //!< sce template id
    SceHttpEpollHandle EpollHandle; //!< epoll handle
    char *pSceRespHeader;
    size_t iSceRespHeaderLength;
    uint32_t uCaListVersionLoaded;
    SceSslVersion Vers;
    uint32_t uSceHttpFlags;
    SceSslData PrivateKeyData;
    SceSslData ClientCertData;
    uint8_t bUsePrivateKey;
    uint8_t bUseClientCert;

    ProtoHttpCustomHeaderCbT *pCustomHeaderCb;      //!< callback for modifying request header
    ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb;    //!< callback for viewing recv header on recepit
    void *pCallbackRef;     //!< user ref for callback

    ProtoHttpWriteCbT *pWriteCb;    //!< optional data write callback
    void *pWriteCbUserData;         //!< user data for write callback

    // module memory group
    int32_t iMemGroup;      //!< module mem group id
    void *pMemGroupUserData;//!< user data associated with mem group

    NetCritT HttpCrit;      //!< critical section for guarding update from send/recv

    ProtoHttpRequestTypeE eRequestType;  //!< request type of current request
    int32_t iPort;          //!< server port
    int32_t iBasePort;      //!< base port (used for partial urls)
    int32_t iSecure;        //!< secure connection
    int32_t iBaseSecure;    //!< base security setting (used for partial urls)

    enum
    {
        ST_IDLE,            //!< idle
        ST_SEND,            //!< sending buffered data
        ST_RESP,            //!< waiting for initial response (also sending any data not buffered if POST or PUT)
        ST_HEAD,            //!< getting header
        ST_BODY,            //!< getting body
        ST_DONE,            //!< transaction success
        ST_FAIL             //!< transaction failed
    } eState;               //!< current state

    int32_t iSslFail;       //!< ssl failure code, if any
    uint32_t uSslFailDet;   //!< ssl failure detail, if any
    int32_t iHresult;       //!< ssl hresult code, if any
    int32_t iHdrCode;       //!< result code
    int32_t iHdrDate;       //!< last modified date

    int32_t iHeadSize;      //!< size of head data
    int64_t iPostSize;      //!< amount of data being sent in a POST or PUT operation
    int64_t iBodySize;      //!< size of body data
    int64_t iBodyRcvd;      //!< size of body data received by caller
    int32_t iRecvSize;      //!< amount of data received by ProtoHttpRecvAll
    int32_t iRecvRslt;      //!< last receive result

    char *pInpBuf;          //!< input buffer
    int32_t iInpMax;        //!< maximum buffer size
    int32_t iInpOff;        //!< offset into buffer
    int32_t iInpLen;        //!< total length in buffer
    int64_t iInpCnt;        //!< ongoing count
    int32_t iInpOvr;        //!< input overflow amount
    int32_t iChkLen;        //!< chunk length (if chunked encoding)
    int32_t iHdrOff;        //!< temp offset used when receiving header

    int32_t iNumRedirect;   //!< number of redirections processed
    int32_t iMaxRedirect;   //!< maximum number of redirections allowed

    uint32_t uConnectTimeout;   //!< connection timeout
    uint32_t uRecvTimeout;      //!< receive timeout
    uint32_t uResolveTimeout;   //!< resolve timeout
    uint32_t uSendTimeout;      //!< send timeout
    int32_t iResolveRetries;    //!< resolve retry timout
    int32_t iKeepAlive;     //!< indicate if we should try to use keep-alive
    int32_t iKeepAliveDflt; //!< keep-alive default (keep-alive will be reset to this value; can be overridden by user)

    char *pAppendHdr;       //!< append header buffer pointer
    int32_t iAppendLen;     //!< size of append header buffer

    char strUserAgent[64];  //!< user agent string 

    char strRequestHdr[PROTOHTTP_HDRCACHESIZE];         //!< storage for most recent HTTP request header
    char strHdr[PROTOHTTP_HDRCACHESIZE]; //!< storage for most recently received HTTP header
    char strHost[256];      //!< server name
    char strBaseHost[256];  //!< base server name (used for partial urls)
    char strProxy[256];     //!< proxy server name/address (including port)
    char strUrl[512];       //!< full url, includes host (or proxy host), port and url
    //char strFinalUrl[256];  //!< full url without proxy

    uint8_t bWaitForData;   //!< boolean indicating we are waiting for a POST's data source to be specified
    uint8_t bTimeout;       //!< boolean indicating whether a timeout occurred or not
    uint8_t bChunked;       //!< if TRUE, transfer is chunked
    uint8_t bRecvEndChunk;  //!< if TRUE, we must deferred end chunk processing
    uint8_t bHeadOnly;      //!< if TRUE, only get header
    uint8_t bCloseHdr;      //!< server wants close after this
    uint8_t bClosed;        //!< connection has been closed
    uint8_t bConnOpen;      //!< connection is open
    uint8_t iVerbose;       //!< debug output verbosity
    uint8_t bVerifyHdr;     //!< perform header type verification
    uint8_t bHttp1_0;       //!< TRUE if HTTP/1.0, else FALSE
    uint8_t bCompactRecv;   //!< compact receive buffer
    uint8_t bInfoHdr;       //!< TRUE if a new informational header has been cached; else FALSE
    uint8_t bNewConnection; //!< TRUE if a new connection should be used, else FALSE (if using keep-alive), 
                            //   note that the Sony scehttplib can handle this on its own but we use similar logic to the protohttp.c implementation to maintain similar behavior
    uint8_t bReuseOnPost;   //!< TRUE if reusing a previously established connection on PUT/POST is allowed, else FALSE
    uint8_t bSetHttpsFlags; //!< TRUE if we should overwrite the default SCE_HTTPS_FLAG_* flags
    uint8_t bSetSslVers;    //!< TRUE if we wish to call sceHttpsSetSslVersion, FALSE otherwise
                            // see https://ps4.scedev.net/resources/documents/SDK/3.500/Http-Reference/0077.html for reference

};

/*** Function Prototypes **********************************************************/

static int _ProtoHttpRedirectCallback(int request, int32_t statusCode, int32_t *method, const char *location, void *userArg);

/*** Variables ********************************************************************/

// GOS Certs
// The 2048-bit public key modulus for GOS 2011 CA Cert signed with md5 and an exponent of 17
static char _ProtoSSL_GOS2011ServerModulus2048[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIFVTCCBD2gAwIBAgIJAIvGsPwZMRr1MA0GCSqGSIb3DQEBBAUAMIHMMScwJQYD\n"
"VQQDEx5HT1MgMjAxMSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxCzAJBgNVBAYTAlVT\n"
"MRMwEQYDVQQIEwpDYWxpZm9ybmlhMRUwEwYDVQQHEwxSZWR3b29kIENpdHkxHjAc\n"
"BgNVBAoTFUVsZWN0cm9uaWMgQXJ0cywgSW5jLjEdMBsGA1UECxMUR2xvYmFsIE9u\n"
"bGluZSBTdHVkaW8xKTAnBgkqhkiG9w0BCQEWGkdPU0RpcnR5c29ja1N1cHBvcnRA\n"
"ZWEuY29tMCAXDTExMTIwOTEwMjI1OVoYDzIxMTExMTE1MTAyMjU5WjCBzDEnMCUG\n"
"A1UEAxMeR09TIDIwMTEgQ2VydGlmaWNhdGUgQXV0aG9yaXR5MQswCQYDVQQGEwJV\n"
"UzETMBEGA1UECBMKQ2FsaWZvcm5pYTEVMBMGA1UEBxMMUmVkd29vZCBDaXR5MR4w\n"
"HAYDVQQKExVFbGVjdHJvbmljIEFydHMsIEluYy4xHTAbBgNVBAsTFEdsb2JhbCBP\n"
"bmxpbmUgU3R1ZGlvMSkwJwYJKoZIhvcNAQkBFhpHT1NEaXJ0eXNvY2tTdXBwb3J0\n"
"QGVhLmNvbTCCASAwDQYJKoZIhvcNAQEBBQADggENADCCAQgCggEBAMINBqXXEazp\n"
"uzEFLIJQ3G+5eIGTHh1woHMW1fuJms9nAANkxhTdFx8ujjOzXDw7YiAus2rL2HLI\n"
"VeayxmWjAzX5FjxLeHStOghSafil84l4OBZiw+3iATK8buvZuPw/x+7y6OwbK6i4\n"
"sT+hGHF2F65hpD7q6HMZmJ9+yG46hgl1Ln61o256FIt7gdAiXG0B742IbIlhElxo\n"
"MmfC5out8+8C/Ibe2ke9+LhVu3XmQDSLjsYVnFmvc8CDzkWDu+DrLFmbedxWNnQo\n"
"govjw4UpENbQZeVik0kiuwxLPHWMl9+HChPK6UtbEeUpKXPYEeGRfDst19yN/c2H\n"
"Dmhid22q2QECARGjggE2MIIBMjAdBgNVHQ4EFgQUQcKsSzrIBxZidMC2ebz0/u59\n"
"+f0wggEBBgNVHSMEgfkwgfaAFEHCrEs6yAcWYnTAtnm89P7uffn9oYHSpIHPMIHM\n"
"MScwJQYDVQQDEx5HT1MgMjAxMSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxCzAJBgNV\n"
"BAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRUwEwYDVQQHEwxSZWR3b29kIENp\n"
"dHkxHjAcBgNVBAoTFUVsZWN0cm9uaWMgQXJ0cywgSW5jLjEdMBsGA1UECxMUR2xv\n"
"YmFsIE9ubGluZSBTdHVkaW8xKTAnBgkqhkiG9w0BCQEWGkdPU0RpcnR5c29ja1N1\n"
"cHBvcnRAZWEuY29tggkAi8aw/BkxGvUwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0B\n"
"AQQFAAOCAQEAdmiZuvUJ8Rq/Ds9T7GaKdvZMy/97uAV+pU3adbnSJFGP3f4q3p3K\n"
"q4qMWVbxjkKVUyvVB/ZacR8iaTlH7bBaqryRjPPNc+05GzBwcYwDaRBbCmeSgkIR\n"
"vAxw1PbGWGRVvC4K117lGw1vQWYbd5eUi5csowhTkaVGOqJjZ9ZELd2X3h89jwp4\n"
"wpK5/8FG0G127/lMn+oyKHDovN+3F450ASPjFXOFnQixxHDwu39rhBFTgYsryy+p\n"
"VtJ/TBlU0hKJ26pGkaCX4mZIq5rT2PGmQvA2zcMGfKNalzb+sil2vq8zZ6+HhGAy\n"
"L8cRGoA1rGWOgTxJ/IQ5zQ+T7aRbBxTCVg==\n"
"-----END CERTIFICATE-----";

// The 2048-bit public key modulus for 2013 GOS CA Cert signed with sha1 and an exponent of 65537
static char _ProtoSSL_GOS2013ServerModulus2048[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIFVTCCBD2gAwIBAgIJALsrFLWX3CSQMA0GCSqGSIb3DQEBBQUAMIHMMScwJQYD\n"
"VQQDEx5HT1MgMjAxMyBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxCzAJBgNVBAYTAlVT\n"
"MRMwEQYDVQQIEwpDYWxpZm9ybmlhMRUwEwYDVQQHEwxSZWR3b29kIENpdHkxHjAc\n"
"BgNVBAoTFUVsZWN0cm9uaWMgQXJ0cywgSW5jLjEdMBsGA1UECxMUR2xvYmFsIE9u\n"
"bGluZSBTdHVkaW8xKTAnBgkqhkiG9w0BCQEWGkdPU0RpcnR5c29ja1N1cHBvcnRA\n"
"ZWEuY29tMB4XDTEzMTAwMjE2NDEyNloXDTMzMDkyNzE2NDEyNlowgcwxJzAlBgNV\n"
"BAMTHkdPUyAyMDEzIENlcnRpZmljYXRlIEF1dGhvcml0eTELMAkGA1UEBhMCVVMx\n"
"EzARBgNVBAgTCkNhbGlmb3JuaWExFTATBgNVBAcTDFJlZHdvb2QgQ2l0eTEeMBwG\n"
"A1UEChMVRWxlY3Ryb25pYyBBcnRzLCBJbmMuMR0wGwYDVQQLExRHbG9iYWwgT25s\n"
"aW5lIFN0dWRpbzEpMCcGCSqGSIb3DQEJARYaR09TRGlydHlzb2NrU3VwcG9ydEBl\n"
"YS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDJNqey6ELagNUA\n"
"iR7m8roP4bmUUQ/KBUmenaS/nwMWyde3bpCckNYXhP9mAT5fjF5Fe1lgQbc0RP9A\n"
"RjbVMW3ipUxMK5y4LSbx1vycA8UijexwVkbA22bOxArvPPXzSbSd2nrvMQseoyBF\n"
"K8XH+TH0bP9baJOE0ako17xBJY3Bl0c7rp/2oNsTalo6zWzWBwRIsbZj1ZBuOC1c\n"
"rTa+SYb65jvA61HLgMnmajumc59ZUmTOrWouSRE/dn9XYbqB6O2L7PGPi7h2upmd\n"
"h7fH/yGicthg0tMb7bj6e2/KiOTq4Pmh6gCNpJwNF9JwSQZDboG4Z5tZEOcr8Au+\n"
"YdbTzEydAgMBAAGjggE2MIIBMjAdBgNVHQ4EFgQUIEY2HcInPy4iOUwcvuWIBgAY\n"
"0j0wggEBBgNVHSMEgfkwgfaAFCBGNh3CJz8uIjlMHL7liAYAGNI9oYHSpIHPMIHM\n"
"MScwJQYDVQQDEx5HT1MgMjAxMyBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxCzAJBgNV\n"
"BAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRUwEwYDVQQHEwxSZWR3b29kIENp\n"
"dHkxHjAcBgNVBAoTFUVsZWN0cm9uaWMgQXJ0cywgSW5jLjEdMBsGA1UECxMUR2xv\n"
"YmFsIE9ubGluZSBTdHVkaW8xKTAnBgkqhkiG9w0BCQEWGkdPU0RpcnR5c29ja1N1\n"
"cHBvcnRAZWEuY29tggkAuysUtZfcJJAwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0B\n"
"AQUFAAOCAQEASrRo/S2IWSMJ452It3dDNjw2Imxt7l+hecKbbkvL5xJ81l5aVt4T\n"
"zwrh2GW8hNUvEOQj4qpdLJWcPmsmTmZReUFwoBxpw66NnkqxzX5jKszlCKDgV8q5\n"
"f3nfoF91R3lHZO9+3ZJGApRRNFkMQNyBll//w1yXcXemfPElNF97XfB0OEY9oZar\n"
"heYrMWk98sUFnfcpBX7tVZADt8We9UABsASSncK6TYwIx1uLvfrwui+D3u3hePNe\n"
"1fsmy4wLm/ECuZbryK9+ocPz47fFKsUiU9uT2I7+vVRUFsJt34U7P2z7S26V767o\n"
"5aj6RyllT4n1e4FMFlHaqtrgkU5R6qS9yg==\n"
"-----END CERTIFICATE-----";

// The 2048-bit public key modulus for 2015 GOS CA Cert signed with sha256 and an exponent of 65537
static char _ProtoSSL_GOS2015ServerModulus2048[] =
"-----BEGIN CERTIFICATE-----\n"
"MIIFVTCCBD2gAwIBAgIJAK+NJsjYxWDQMA0GCSqGSIb3DQEBCwUAMIHMMScwJQYD\n"
"VQQDEx5HT1MgMjAxNSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxCzAJBgNVBAYTAlVT\n"
"MRMwEQYDVQQIEwpDYWxpZm9ybmlhMRUwEwYDVQQHEwxSZWR3b29kIENpdHkxHjAc\n"
"BgNVBAoTFUVsZWN0cm9uaWMgQXJ0cywgSW5jLjEdMBsGA1UECxMUR2xvYmFsIE9u\n"
"bGluZSBTdHVkaW8xKTAnBgkqhkiG9w0BCQEWGkdPU0RpcnR5c29ja1N1cHBvcnRA\n"
"ZWEuY29tMB4XDTE1MDExMzA5MzAxNVoXDTIwMDExMjA5MzAxNVowgcwxJzAlBgNV\n"
"BAMTHkdPUyAyMDE1IENlcnRpZmljYXRlIEF1dGhvcml0eTELMAkGA1UEBhMCVVMx\n"
"EzARBgNVBAgTCkNhbGlmb3JuaWExFTATBgNVBAcTDFJlZHdvb2QgQ2l0eTEeMBwG\n"
"A1UEChMVRWxlY3Ryb25pYyBBcnRzLCBJbmMuMR0wGwYDVQQLExRHbG9iYWwgT25s\n"
"aW5lIFN0dWRpbzEpMCcGCSqGSIb3DQEJARYaR09TRGlydHlzb2NrU3VwcG9ydEBl\n"
"YS5jb20wggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDMbVS29OSE5yB2\n"
"AteXSHV+fvtDnD2hlkfBXQeLMHO/nf51lFUh0Ih0Zkyit/6fwDvwYKDbCDMrbvgC\n"
"BbmHnaxl1QadBejRtvXefaWkfYrLmTG2hZuizjnijGWqB/wVMwcA0XIVEw2HD1yi\n"
"XtDVv9kDMmKv9e9TNqg02raj7FxqwGf4vjefs8gt8DZKb2sG7reF8n9zbAGEg+Ta\n"
"RtAjmm3xd3wFgZBPakSDeDtxrRLASMhzifGYeHu0CEq66FlX4vwprL/1op1PLGTc\n"
"15IZHMX625LAkEuo6fINlBqyX90zrv9mkJeyqKUb+m9BsoS6UjSXStPHsj/d28mx\n"
"E4J36GrNAgMBAAGjggE2MIIBMjAdBgNVHQ4EFgQUIG5PT8/9501rZblXMe3TJTAX\n"
"hkMwggEBBgNVHSMEgfkwgfaAFCBuT0/P/edNa2W5VzHt0yUwF4ZDoYHSpIHPMIHM\n"
"MScwJQYDVQQDEx5HT1MgMjAxNSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkxCzAJBgNV\n"
"BAYTAlVTMRMwEQYDVQQIEwpDYWxpZm9ybmlhMRUwEwYDVQQHEwxSZWR3b29kIENp\n"
"dHkxHjAcBgNVBAoTFUVsZWN0cm9uaWMgQXJ0cywgSW5jLjEdMBsGA1UECxMUR2xv\n"
"YmFsIE9ubGluZSBTdHVkaW8xKTAnBgkqhkiG9w0BCQEWGkdPU0RpcnR5c29ja1N1\n"
"cHBvcnRAZWEuY29tggkAr40myNjFYNAwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0B\n"
"AQsFAAOCAQEAVUBj4PoeGzua+WXRB2+ISWKcqFyUwUrotkJsIaCSHW+Th1Q/OnJI\n"
"3Ad6se8W96JoGjXjWMtJeUa1wkey9L5Ek+VppbRD4n9DnAsPVGbnvmwby00RxmrU\n"
"0mdw/arydVvQ3Mp+cL31Aruv+Mh2TMlJ2ghORc1ZDfSOP7p1yECECSFTPFJB4gVO\n"
"J612cFq6Xl8wVSU2YHAAS5rUwR3BuFYIMyPsLcZhK2ZgpK4tTz6/UbIDkCioyleU\n"
"pKCsmCOKcBlhjUWytLOdRzOWMkntMpxYhsXBgG4T+D3LMspwmmfq93IMExSyGLkU\n"
"k+mEFzpz0aaPl32trGdasj4jdmdteC4HdQ==\n"
"-----END CERTIFICATE-----";

// Private variables

static const SceSslData _ProtoHttp_aGosCACerts[] = 
{
    // gos2011 CA
    {
        _ProtoSSL_GOS2011ServerModulus2048,
        sizeof(_ProtoSSL_GOS2011ServerModulus2048)
    },

    // gos2013 CA
    {
        _ProtoSSL_GOS2013ServerModulus2048,
        sizeof(_ProtoSSL_GOS2013ServerModulus2048)
    },

    // gos2015 CA
    {
        _ProtoSSL_GOS2015ServerModulus2048,
        sizeof(_ProtoSSL_GOS2015ServerModulus2048)
    }
};

// update this when PROTOHTTP_NUMREQUESTTYPES changes
static const char _ProtoHttp_strRequestNames[PROTOHTTP_NUMREQUESTTYPES][16] =
{
    "HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS"
};

#if DIRTYCODE_LOGGING
//! ssl version name table
static const char *_HTTP_strVersionNames[] =
{
    "SSLv3",
    "TLSv1",
    "TLSv1.1",
    "TLSv1.2"
};
#endif

// list of CAs
static ProtoHttpCACertT *_pProtoHttpPs4_CACerts = NULL;
static uint32_t uCaListVersion = 0;

// Public variables


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _ProtoHttpProtoSslToSceVers

    \Description
        Translate the protossl ssl version to the equivalent sce ssl version. 

    \Input iProtoSslVers - protossl sso version


    \Version 07/08/2016 (amakoukji)
*/
/********************************************************************************F*/
SceSslVersion _ProtoHttpProtoSslToSceVers(int32_t iProtoSslVers)
{
    SceSslVersion retValue;
    switch (iProtoSslVers)
    {
        case PROTOSSL_VERSION_SSLv3:
            retValue = SCE_SSL_VERSION_SSL3_0;
        break;
        case PROTOSSL_VERSION_TLS1_0:
            retValue = SCE_SSL_VERSION_TLS1_0;
        break;
        case PROTOSSL_VERSION_TLS1_1:
            retValue = SCE_SSL_VERSION_TLS1_1;
        break;
        case PROTOSSL_VERSION_TLS1_2:
            retValue = SCE_SSL_VERSION_TLS1_2;
        break;
        default:
            retValue = SCE_SSL_VERSION_TLS1_2;
        break;
    }
    return(retValue);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpIsMethodNativelySupported

    \Description
        Returns whether the http method is supported natively by the SCE HTTP lib.
        Updated as of PS4 SDK 3.508.041

    \Input eRequestType - request type

    \Output
        uint8_t         - TRUE if supported, FALSE otherwise

    \Version 07/08/2016 (amakoukji)
*/
/********************************************************************************F*/
uint8_t _ProtoHttpIsMethodNativelySupported(ProtoHttpRequestTypeE eRequestType)
{
    if (eRequestType == PROTOHTTP_REQUESTTYPE_OPTIONS)
    {
        return(FALSE);
    }

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttp_reqTypeToSceReqType

    \Description
        Map the ProtoHttpRequestTypeE enum to the SceHttpMethods enum 

    \Input eRequestType - request type

    \Output
        int32_t         - equivalent sce type

    \Version 07/08/2016 (amakoukji)
*/
/********************************************************************************F*/
int32_t _ProtoHttp_reqTypeToSceReqType(ProtoHttpRequestTypeE eRequestType)
{
    switch(eRequestType)
    {
        case PROTOHTTP_REQUESTTYPE_HEAD:
        {
            return(SCE_HTTP_METHOD_HEAD);
        }
        case PROTOHTTP_REQUESTTYPE_GET:
        {
            return(SCE_HTTP_METHOD_GET);
        }
        case PROTOHTTP_REQUESTTYPE_POST:
        {
            return(SCE_HTTP_METHOD_POST);
        }
        case PROTOHTTP_REQUESTTYPE_PUT:
        {
            return(SCE_HTTP_METHOD_PUT);
        }
        case PROTOHTTP_REQUESTTYPE_DELETE:
        {
            return(SCE_HTTP_METHOD_DELETE);
        }
        case PROTOHTTP_REQUESTTYPE_OPTIONS:
        {
            return(SCE_HTTP_METHOD_OPTIONS);
        }

        case PROTOHTTP_NUMREQUESTTYPES:
        default:
        {
            NetPrintf(("protohttpps4: invalid request type %d\n", eRequestType));
            return(-1);
        }
    }

    // fallthrough, should never hit this code
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpApplyCAs

    \Description
        Apply CAs

    \Input pState - module ref

    \Version 07/08/2016 (amakoukji)
*/
/********************************************************************************F*/
void _ProtoHttpApplyCAs(ProtoHttpRefT *pState)
{
    int32_t iResult = 0;
    int32_t iCert = 0;
    const SceSslData* caList;
    SceSslData caCert;
    ProtoHttpCACertT *pCert = NULL;

    // if we're not using the current CaListVersion, reload all the certs
    if (pState->uCaListVersionLoaded != uCaListVersion)
    {
        // assume iHttpContextId is set at this point
        if ((iResult = sceHttpsUnloadCert(pState->iHttpContextId)) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpsUnloadCert failed with error 0x%08x\n", pState, iResult));
        }

        // load GOS certs
        for (iCert = 0; iCert < PROTOHTTP_GOSCA_COUNT; ++iCert)
        {
            caList = &_ProtoHttp_aGosCACerts[iCert];
            iResult = sceHttpsLoadCert(pState->iHttpContextId, 1, &caList, NULL, NULL);
            if (iResult < 0)
            {
                NetPrintf(("protohttpps4: [0x%08x] sceHttpsLoadCert failed to load a GOS CA with error 0x%08x\n", pState, iResult));
            }
        }

        // load custom certs
        pCert = _pProtoHttpPs4_CACerts;
        while (pCert != NULL)
        {
            caCert.ptr = (char*)pCert->pCACert;
            caCert.size = pCert->iCertSize;
            caList = &caCert;
            iResult = sceHttpsLoadCert(pState->iHttpContextId, 1, &caList, NULL, NULL);
            if (iResult < 0)
            {
                NetPrintf(("protohttpps4: [0x%08x] sceHttpsLoadCert failed to load a CA with error 0x%08x\n", pState, iResult));
            }
            pCert = pCert->pNext;
        }

        pState->uCaListVersionLoaded = uCaListVersion;
    }

    if (pState->bUsePrivateKey)
    {
        if ((iResult = sceHttpsLoadCert(pState->iHttpContextId, 0, NULL, NULL, &pState->PrivateKeyData)) < 0)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpsLoadCert failed with code 0x%08x\n", (uintptr_t)pState, iResult));
        }
    }

    if (pState->bUseClientCert)
    {
        if ((iResult = sceHttpsLoadCert(pState->iHttpContextId, 0, NULL, &pState->ClientCertData, NULL)) < 0)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpsLoadCert failed with code 0x%08x\n", (uintptr_t)pState, iResult));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpCreateConnectionTemplate

    \Description
        Create and configure a sce template id

    \Input pState           - module ref

    \Version 07/08/2016 (amakoukji)
*/
/********************************************************************************F*/
void _ProtoHttpCreateConnectionTemplate(ProtoHttpRefT *pState)
{
    int32_t iResult = 0;
    pState->iSceTemplateId = sceHttpCreateTemplate(pState->iHttpContextId, pState->strUserAgent, SCE_HTTP_VERSION_1_1, SCE_TRUE);
    if (pState->iSceTemplateId < 0)
    {
        NetPrintf(("protohttpps4: [0x%08x] sceHttpCreateTemplate failed with code 0x%08x\n", (uintptr_t)pState, pState->iSceConnId));
        pState->eState = ST_FAIL;
        pState->bClosed = TRUE;
        pState->iSceTemplateId = 0;
    }
    else
    {
        // template create, configure it
        NetPrintfVerbose((pState->iVerbose, 3, "protohttpps4: [0x%08x] template created with id 0x%08x\n", (uintptr_t)pState, pState->iSceTemplateId));

        // set SSL version if required
        if (pState->bSetSslVers)
        {
            if ((iResult = sceHttpsSetSslVersion(pState->iSceTemplateId, pState->Vers)) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpsSetSslVersion failed with code 0x%08x\n", (uintptr_t)pState, iResult));
                pState->bSetSslVers = FALSE;
                pState->eState = ST_FAIL;
                pState->bClosed = TRUE;
                sceHttpDeleteTemplate(pState->iSceTemplateId);
                pState->iSceTemplateId = 0;
            }
        }

        if (pState->bSetHttpsFlags)
        {
            if (pState->iSceTemplateId > 0)
            {
                // reset flags and apply the new state
                if ((iResult = sceHttpsDisableOption(pState->iSceTemplateId, SCE_HTTPS_FLAG_SERVER_VERIFY|SCE_HTTPS_FLAG_CLIENT_VERIFY|SCE_HTTPS_FLAG_CN_CHECK|SCE_HTTPS_FLAG_NOT_AFTER_CHECK|SCE_HTTPS_FLAG_NOT_BEFORE_CHECK|SCE_HTTPS_FLAG_KNOWN_CA_CHECK|SCE_HTTPS_FLAG_SESSION_REUSE|SCE_HTTPS_FLAG_SNI)) < 0)
                {
                    NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpsDisableOption failed with code 0x%08x\n", (uintptr_t)pState, iResult));
                    pState->bSetHttpsFlags = FALSE;
                    pState->eState = ST_FAIL;
                    pState->bClosed = TRUE;
                    sceHttpDeleteTemplate(pState->iSceTemplateId);
                    pState->iSceTemplateId = 0;
                }

                if ((iResult = sceHttpsEnableOption(pState->iSceTemplateId, pState->uSceHttpFlags)) < 0)
                {
                    NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpsEnableOption failed with code 0x%08x, flags 0x%08x\n", (uintptr_t)pState, iResult, pState->uSceHttpFlags));
                    pState->bSetHttpsFlags = FALSE;
                    pState->eState = ST_FAIL;
                    pState->bClosed = TRUE;
                    sceHttpDeleteTemplate(pState->iSceTemplateId);
                    pState->iSceTemplateId = 0;
                }
            }
        }

        // set up callbacks
        if (pState->eState != ST_FAIL && (iResult = sceHttpSetRedirectCallback(pState->iSceTemplateId, &_ProtoHttpRedirectCallback, (void*)pState)) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpSetRedirectCallback failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpDeleteTemplate(pState->iSceTemplateId);
            pState->iSceTemplateId = 0;
        }
        
    
        // set the template as non-blocking
        if (pState->eState != ST_FAIL && (iResult = sceHttpSetNonblock(pState->iSceTemplateId, SCE_TRUE)) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpSetNonblock failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpDeleteTemplate(pState->iSceTemplateId);
            pState->iSceTemplateId = 0;
        }
    
        if (pState->eState != ST_FAIL && (iResult = sceHttpCreateEpoll(pState->iHttpContextId, &pState->EpollHandle)) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpCreateEpoll failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpDeleteTemplate(pState->iSceTemplateId);
            pState->iSceTemplateId = 0;
        }
    
        // set the connection timeout
        if (pState->eState != ST_FAIL && (iResult = sceHttpSetConnectTimeOut(pState->iSceTemplateId, (pState->uConnectTimeout * 1000))) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpSetConnectTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpDeleteTemplate(pState->iSceTemplateId);
            pState->iSceTemplateId = 0;
        }
    
        // set the receive timeout
        if (pState->eState != ST_FAIL && (iResult = sceHttpSetRecvTimeOut(pState->iSceTemplateId, (pState->uRecvTimeout * 1000))) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpSetRecvTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpDeleteTemplate(pState->iSceTemplateId);
            pState->iSceTemplateId = 0;
        }
    
        // set the name resolution timeout
        if (pState->eState != ST_FAIL && (iResult = sceHttpSetResolveTimeOut(pState->iSceTemplateId, (pState->uResolveTimeout * 1000))) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpSetResolveTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpDeleteTemplate(pState->iSceTemplateId);
            pState->iSceTemplateId = 0;
        }
    
        // set the max name resolution retries
        if (pState->eState != ST_FAIL && (iResult = sceHttpSetResolveRetry(pState->iSceTemplateId, (pState->iResolveRetries))) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpSetResolveRetry failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpDeleteTemplate(pState->iSceTemplateId);
            pState->iSceTemplateId = 0;
        }
    
        // set the send timeout
        if (pState->eState != ST_FAIL && (iResult = sceHttpSetSendTimeOut(pState->iSceTemplateId, (pState->uSendTimeout * 1000))) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpSetSendTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpDeleteTemplate(pState->iSceTemplateId);
            pState->iSceTemplateId = 0;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpApplyBaseUrl

    \Description
        Apply base url elements (if set) to any url elements not specified (relative
        url support).

    \Input *pState      - module state
    \Input *pKind       - parsed http kind ("http" or "https")
    \Input *pHost       - [in/out] parsed URL host
    \Input iHostSize    - size of pHost buffer
    \Input *pPort       - [in/out] parsed port
    \Input *pSecure     - [in/out] parsed security (0 or 1)
    \Input bPortSpecified - TRUE if a port is explicitly specified in the url, else FALSE

    \Output
        uint32_t        - non-zero if changed, else zero

    \Version 02/03/2010 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _ProtoHttpApplyBaseUrl(ProtoHttpRefT *pState, const char *pKind, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure, uint8_t bPortSpecified)
{
    uint8_t bChanged = FALSE;
    if ((*pHost == '\0') && (pState->strBaseHost[0] != '\0'))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: host not present; setting to %s\n", pState->strBaseHost));
        ds_strnzcpy(pHost, pState->strBaseHost, iHostSize);
        bChanged = TRUE;
    }
    if ((bPortSpecified == FALSE) && (pState->iBasePort != 0))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: port not present; setting to %d\n", pState->iBasePort));
        *pPort = pState->iBasePort;
        bChanged = TRUE;
    }
    if (*pKind == '\0')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: kind (protocol) not present; setting to %d\n", pState->iBaseSecure));
        *pSecure = pState->iBaseSecure;
        // if our port setting is default and incompatible with our security setting, override it
        if (((*pPort == 80) && (*pSecure == 1)) || ((*pPort == 443) && (*pSecure == 0)))
        {
            *pPort = *pSecure ? 443 : 80;
        }
        bChanged = TRUE;
    }
    return(bChanged);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpClose

    \Description
        Close connection to server, if open.

    \Input *pState      - module state
    \Input *pReason     - reason connection is being closed (for debug output)

    \Output
        None.

    \Version 10/07/2005 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _ProtoHttpClose(ProtoHttpRefT *pState, const char *pReason)
{
    int32_t iResult = 0;
    if (pState->bClosed)
    {
        // already issued disconnect, don't need to do it again
        return;
    }

    NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] closing connection: %s\n", (uintptr_t)pState, pReason));

    if (pState->iSceRequestId > 0)
    {
        sceHttpUnsetEpoll(pState->iSceRequestId);
        iResult = sceHttpDeleteRequest(pState->iSceRequestId);
        if (iResult != SCE_OK)
        {
            NetPrintf(("protohttpps4: [0x%08x] unable to delete request %d, err=0x%08x\n", (uintptr_t)pState, pState->iSceRequestId, iResult));
        }
        pState->iSceRequestId = 0;
    }
    if (pState->iSceConnId > 0)
    {
        sceHttpDeleteConnection(pState->iSceConnId);
        pState->iSceConnId = 0;
    }
    pState->bCloseHdr = FALSE;
    pState->bConnOpen = FALSE;
    pState->bClosed = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpReset

    \Description
        Reset state before a transaction request.

    \Input  *pState     - reference pointer

    \Output
        None.

    \Version 11/22/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
static void _ProtoHttpReset(ProtoHttpRefT *pState)
{
    int32_t iResult = 0;
    memset(pState->strHdr, 0, sizeof(pState->strHdr));
    pState->eState = ST_IDLE;
    pState->iSslFail = 0;
    pState->uSslFailDet = 0;
    pState->iHresult = 0;
    pState->iHdrCode = -1;
    pState->iHdrDate = 0;
    pState->iHeadSize = 0;
    pState->iBodySize = pState->iBodyRcvd = 0;
    pState->iRecvSize = 0;
    pState->iInpOff = 0;
    pState->iInpLen = 0;
    pState->iInpOvr = 0;
    pState->iChkLen = 0;
    pState->iInpCnt = 0;
    pState->bTimeout = FALSE;
    pState->bChunked = FALSE;
    pState->bRecvEndChunk = FALSE;
    pState->bHeadOnly = FALSE;

    if (pState->iSceRequestId > 0)
    {
        sceHttpUnsetEpoll(pState->iSceRequestId);
        iResult = sceHttpDeleteRequest(pState->iSceRequestId);
        if (iResult != SCE_OK)
        {
            NetPrintf(("protohttpps4: [0x%08x] unable to delete request %d, err=0x%08x\n", (uintptr_t)pState, pState->iSceRequestId, iResult));
        }
        pState->iSceRequestId = 0;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSetAppendHeader

    \Description
        Set given string as append header, allocating memory as required.

    \Input *pState      - reference pointer
    \Input *pAppendHdr  - append header string

    \Output
        int32_t         - zero=success, else error

    \Version 11/11/2004 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSetAppendHeader(ProtoHttpRefT *pState, const char *pAppendHdr)
{
    int32_t iAppendBufLen, iAppendStrLen;

    // check for empty append string, in which case we free the buffer
    if ((pAppendHdr == NULL) || (*pAppendHdr == '\0'))
    {
        if (pState->pAppendHdr != NULL)
        {
            DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pState->pAppendHdr = NULL;
        }
        pState->iAppendLen = 0;
        return(0);
    }

    // check to see if append header is already set
    if ((pState->pAppendHdr != NULL) && (!strcmp(pAppendHdr, pState->pAppendHdr)))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: ignoring set of append header '%s' that is already set\n", pAppendHdr));
        return(0);
    }

    // get append header length
    iAppendStrLen = (int32_t)strlen(pAppendHdr);
    // append buffer size includes null and space for \r\n if not included by submitter
    iAppendBufLen = iAppendStrLen + 3;

    // see if we need to allocate a new buffer
    if (iAppendBufLen > pState->iAppendLen)
    {
        if (pState->pAppendHdr != NULL)
        {
            DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }
        if ((pState->pAppendHdr = DirtyMemAlloc(iAppendBufLen, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) != NULL)
        {
            pState->iAppendLen = iAppendBufLen;
        }
        else
        {
            NetPrintf(("protohttpps4: could not allocate %d byte buffer for append header\n", iAppendBufLen));
            pState->iAppendLen = 0;
            return(-1);
        }
    }

    // copy append header
    ds_strnzcpy(pState->pAppendHdr, pAppendHdr, iAppendStrLen+1);

    // if append header is not \r\n terminated, do it here
    if (pState->pAppendHdr[iAppendStrLen-2] != '\r' || pState->pAppendHdr[iAppendStrLen-1] != '\n')
    {
        ds_strnzcat(pState->pAppendHdr, "\r\n", pState->iAppendLen);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFormatRequestHeader

    \Description
        Format a request header based on given input data.

    \Input *pState      - reference pointer
    \Input *pUrl        - pointer to user-supplied url
    \Input *pHost       - pointer to hostname
    \Input iPort        - port, or zero if unspecified
    \Input iSecure      - 1=enabled, 0=disabled
    \Input *pRequest    - pointer to request type ("GET", "HEAD", "POST", "PUT")
    \Input *pData       - pointer to included data, or NULL if none
    \Input iDataLen     - size of included data; zero if none, negative if streaming put/post

    \Output
        int32_t         - zero=success, else error

    \Version 11/11/2004 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpFormatRequestHeader(ProtoHttpRefT *pState, const char *pUrl, const char *pHost, int32_t iPort, int32_t iSecure, const char *pRequest, const char *pData, int64_t iDataLen)
{
    int32_t iOffset = 0;
    int32_t iInpLen = 0;
    int32_t iInpMax = (int32_t)sizeof(pState->strRequestHdr);
    int32_t iResult = 0;
    char *pInpBuf = pState->strRequestHdr;
    
    // On PS4 it is not possible the access and alter the entire header before sending to the server.
    // Create an approximation and parse header differences to apply them as possible

    // add HTTP/1.1 and crlf after url
    iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, " HTTP/1.1\r\n");

    // append host 
    if ((pState->iSecure && (pState->iPort == 443)) || (pState->iPort == 80))
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "Host: %s\r\n", pState->strHost);
    }
    else
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "Host: %s:%d\r\n", pState->strHost, pState->iPort);
    }

    // disable keep-alive?
    if (pState->iKeepAlive == 0)
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "Connection: Close\r\n");
    }

    // append user-agent, if not specified in append header, 
    // note that this has already been set in the template and cannot actually be changed at this point
    if ((pState->pAppendHdr == NULL) || !ds_stristr(pState->pAppendHdr, "User-Agent:"))
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "User-Agent:%s", pState->strUserAgent);
    }
    
    // add append headers, if specified
    if ((pState->pAppendHdr != NULL) && (pState->pAppendHdr[0] != '\0'))
    {
        iInpLen += ds_snzprintf(pInpBuf+iInpLen, iInpMax-iInpLen, "%s", pState->pAppendHdr);
    }

    if (pState->pCustomHeaderCb != NULL)
    {
        if ((iOffset = pState->pCustomHeaderCb(pState, pInpBuf, iInpMax, pData, iDataLen, pState->pCallbackRef)) < 0)
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: custom header callback error %d\n", iOffset));
            return(iOffset);
        }
    }

    // whether the header was altered or not parse through and extract the relevant data and add it to the Sony header
    if (iOffset == 0)
    {
        iOffset = (int32_t)ds_strnlen(pInpBuf, iInpMax);
    }

    if (iOffset > 0)
    {
        char strHeaderName[256];
        char strValuerName[256];
        const char *pCurrentHdr = pState->strRequestHdr;
        uint8_t bFirst = TRUE;

        for ( ; ProtoHttpGetNextHeader(pState, pCurrentHdr, strHeaderName, sizeof(strHeaderName), strValuerName, sizeof(strValuerName), &pCurrentHdr) == 0; )
        {
            // ignore values we cannot set
            if (ds_strnicmp(strHeaderName, " HTTP/", 6) == 0)
            {
                // noop
            }
            else if (ds_strnicmp(strHeaderName, "Host", (int32_t)sizeof(strHeaderName)) == 0)
            {
                // noop
            }
            else if (ds_strnicmp(strHeaderName, "Content-Length", (int32_t)sizeof(strHeaderName)) == 0)
            {
                // noop
            }
            else if (ds_strnicmp(strHeaderName, "Connection", (int32_t)sizeof(strHeaderName)) == 0)
            {
                // update keep-alive value
                if (ds_strnicmp(strValuerName, "Close", (int32_t)sizeof(strHeaderName)) == 0)
                {
                    pState->iKeepAlive = 0;
                }
                // if not closed explicitely, consider it open as per HTTP 1.1
                // see https://tools.ietf.org/html/rfc7230#section-6.3
                else 
                {
                    pState->iKeepAlive = 1;
                }
            }
            else if (ds_strnicmp(strHeaderName, "Proxy-Connection", (int32_t)sizeof(strHeaderName)) == 0)
            {
                // noop
            }
            else 
            {
                if ((iResult = sceHttpAddRequestHeader(pState->iSceTemplateId, strHeaderName, strValuerName, bFirst ? SCE_HTTP_HEADER_OVERWRITE : SCE_HTTP_HEADER_ADD)) < 0)
                {
                    NetPrintf(("protohttpps4: unable to add header %s, error 0x%08x\n", strHeaderName, iResult));
                }
                else
                {
                    bFirst = FALSE;
                }
                
            }
        }
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFormatRequest

    \Description
        Format a request into the local buffer.

    \Input *pState      - reference pointer
    \Input *pUrl        - pointer to user-supplied url
    \Input *pData       - pointer to data to include with request, or NULL
    \Input iDataLen     - size of data pointed to by pData, or zero if no data
    \Input eRequestType - type of request (PROTOHTTP_REQUESTTYPE_*)

    \Output
        int32_t         - bytes of userdata included in request

    \Version 10/07/2005 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpFormatRequest(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataLen, ProtoHttpRequestTypeE eRequestType)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iResult, iSecure;
    int32_t eState = pState->eState;
    uint8_t bPortSpecified;

    NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] %s %s\n", (uintptr_t)pState, _ProtoHttp_strRequestNames[eRequestType], pUrl));
    pState->eRequestType = eRequestType;

    // reset various state
    if (pState->eState != ST_IDLE)
    {
        _ProtoHttpReset(pState);
    }

    // assume we don't want a new connection to start with (if this is a pipelined request, don't override the original selection)
    if (pState->iInpLen == 0)
    {
        pState->bNewConnection = FALSE;
    }

    // copy the whole URL as provided
    ds_strnzcpy(pState->strUrl, pUrl, (int32_t)sizeof(pState->strUrl));
    
    // parse the url for kind, host, and port
    pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    /* Unable to support with sceHttp library
    if (pState->strProxy[0] == '\0')
    {
        pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
    }
    else
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: using proxy server %s\n", pState->strProxy));
        pUrl2 = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
        ProtoHttpUrlParse2(pState->strProxy, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
        ds_snzprintf(pState->strFinalUrl, (int32_t)sizeof(pState->strFinalUrl), "%s://%s:%d%s", iSecure ? "https" : "http", strHost, iPort, pUrl2);
    }
    */

    // fill in any missing info (relative url) if available
    if (_ProtoHttpApplyBaseUrl(pState, strKind, strHost, sizeof(strHost), &iPort, &iSecure, bPortSpecified) != 0)
    {
        // copy the whole URL with base URL info prepended
        ds_snzprintf(pState->strUrl, (int32_t)sizeof(pState->strUrl), "%s://%s:%d%s", iSecure ? "https" : "http", strHost, iPort, pUrl);
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] %s %s\n", (uintptr_t)pState, _ProtoHttp_strRequestNames[eRequestType], pState->strUrl));
    }

    // determine if host, port, or security settings have changed since the previous request
    if ((iSecure != pState->iSecure) || (ds_stricmp(strHost, pState->strHost) != 0) || (iPort != pState->iPort))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] requesting new connection -- url change to %s\n", (uintptr_t)pState, strHost));

        // reset keep-alive
        pState->iKeepAlive = pState->iKeepAliveDflt;

        // save new server/port/security state
        ds_strnzcpy(pState->strHost, strHost, sizeof(pState->strHost));
        pState->iPort = iPort;
        pState->iSecure = iSecure;

        // make sure we use a new connection
        pState->bNewConnection = TRUE;
    }

    // check to see if previous connection (if any) is still active
    if ((pState->bNewConnection == FALSE) && pState->bClosed)
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] requesting new connection -- previous connection was closed\n", (uintptr_t)pState));
        pState->bNewConnection = TRUE;
    }

    // check to make sure we are in a known valid state
    if ((pState->bNewConnection == FALSE) && (eState != ST_IDLE) && (eState != ST_DONE))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] requesting new connection -- current state of %d does not allow connection reuse\n", (uintptr_t)pState, eState));
        pState->bNewConnection = TRUE;
    }

    // if executing put/post, check to see if connection reuse on request is allowed
    if ((pState->bNewConnection == FALSE) && (pState->bReuseOnPost == FALSE) && ((eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (eRequestType == PROTOHTTP_REQUESTTYPE_POST)))
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] requesting new connection -- reuse on put/post disabled\n", (uintptr_t)pState));
        pState->bNewConnection = TRUE;
    }

    // if using a proxy server, parse original url to get target host and port for Host header
    /* Unable to support with sceHttp library
    if (pState->strProxy[0] != '\0')
    {
        ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
    }
    */

    // write the user agent string for the template
    if (!ds_stristr(pState->pAppendHdr, "User-Agent:"))
    {
        ds_snzprintf(pState->strUserAgent, (int32_t)sizeof(pState->strUserAgent), "ProtoHttp %d.%d/DS %d.%d.%d.%d.%d (" DIRTYCODE_PLATNAME ")\r\n",
                     (PROTOHTTP_VERSION>>8)&0xff, PROTOHTTP_VERSION&0xff, DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    }
    else
    {
        // assume the user agent string in the append headers is terminated by \r\n or the end of the append headers
        const char *pUserAgentStart = ds_stristr(pState->pAppendHdr, "User-Agent:") + strlen("User-Agent:");
        const char *pUserAgentEnd   = ds_stristr(pUserAgentStart, "\r\n");
        if (pUserAgentEnd == NULL)
        {
            ds_snzprintf(pState->strUserAgent, (int32_t)(sizeof(pState->strUserAgent)), "%s", pUserAgentStart);
        }
        else
        {
            ds_snzprintf(pState->strUserAgent, DS_MIN((int32_t)(pUserAgentEnd - pUserAgentStart) + 1, (int32_t)(sizeof(pState->strUserAgent))), "%s", pUserAgentStart);
        }
    }

    // create the template if none exists
    if (pState->iSceTemplateId <= 0)
    {
        _ProtoHttpCreateConnectionTemplate(pState);  
    }

    // if using a proxy server, parse original url to get target host and port for Host header
    /* Unable to support with sceHttp library
    if (pState->strProxy[0] != '\0')
    {
        ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);
    }
    */ 

    // format the request header
    if ((iResult = _ProtoHttpFormatRequestHeader(pState, pUrl, strHost, iPort, iSecure, _ProtoHttp_strRequestNames[eRequestType], pData, iDataLen)) < 0)
    {
        return(iResult);
    }

    // append data to header?
    if ((pData != NULL) && (iDataLen > 0))
    {
        // see how much data will fit into the buffer
        if (iDataLen > (pState->iInpMax - pState->iInpLen))
        {
            iDataLen = (pState->iInpMax - pState->iInpLen);
        }

        // copy data into buffer (must happen after _ProtoHttpFormatRequestHeader())
        ds_memcpy(pState->pInpBuf + pState->iInpLen, pData, (int32_t)iDataLen);
        pState->iInpLen += iDataLen;
    }
    else if (iDataLen < 0)
    {
        // for a streaming post, return no data written
        iDataLen = 0;
    }
    else if (iDataLen > 0 && pData == NULL)
    {
        // special case
        // ProtoHttp allows the user to specify the location of the data later while specifying its total size right away
        pState->bWaitForData = TRUE;
    }


    // set headonly status
    pState->bHeadOnly = (eRequestType == PROTOHTTP_REQUESTTYPE_HEAD) ? TRUE : FALSE;
    return((int32_t)iDataLen);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSendRequest

    \Description
        Send a request (already formatted in buffer) to the server.

    \Input *pState      - reference pointer

    \Output
        None.

    \Version 05/19/2009 (jbrookes) Split from _ProtoHttpFormatRequest()
*/
/********************************************************************************F*/
static void _ProtoHttpSendRequest(ProtoHttpRefT *pState)
{
    int32_t iResult;
    
    // see if we need a new connection
    // note that the Sony scehttplib can handle this on its own but we use similar logic to the protohttp.c implementation to maintain similar behavior
    if (pState->bNewConnection == TRUE)
    {
        // close the existing connection, if not already closed
        _ProtoHttpClose(pState, "new connection");

        // start connect
        NetPrintfVerbose((pState->iVerbose, 2, "protohttpps4: [0x%08x] connect start (tick=%u)\n", (uintptr_t)pState, NetTick()));
        
        // call the connection creation
        if (pState->iSceTemplateId > 0)
        {
            pState->iSceConnId = sceHttpCreateConnectionWithURL(pState->iSceTemplateId, pState->strUrl, pState->iKeepAlive > 0);
            if (pState->iSceConnId > 0)
            {
                NetPrintfVerbose((pState->iVerbose, 3, "protohttpps4: [0x%08x] connection created with id 0x%08x\n", (uintptr_t)pState, pState->iSceConnId));
                pState->eState = ST_SEND;
                pState->bClosed = FALSE;
            }
            else
            {
                NetPrintf(("protohttpps4: [0x%08x] sceHttpCreateConnectionWithURL failed with code 0x%08x\n", (uintptr_t)pState, pState->iSceConnId));
                pState->eState = ST_FAIL;
                pState->bClosed = TRUE;
                pState->iSceConnId = 0;
            }
        }
        else
        {
            // sceTemplate was not created properly, impossible to create a request
            NetPrintf(("protohttpps4: [0x%08x] unable to create a connection, template doesn't exist\n", (uintptr_t)pState));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            pState->iSceConnId = 0;
        }
    }
    else
    {
        // advance state
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] reusing previous connection (keep-alive)\n", (uintptr_t)pState));
        pState->eState = ST_SEND;
    }

    _ProtoHttpApplyCAs(pState);

    if (pState->iSceConnId > 0)
    {
        if (_ProtoHttpIsMethodNativelySupported(pState->eRequestType))
        {
            pState->iSceRequestId = sceHttpCreateRequestWithURL(pState->iSceConnId, _ProtoHttp_reqTypeToSceReqType(pState->eRequestType), pState->strUrl, (uint64_t)pState->iPostSize);
        }
        else
        {
            pState->iSceRequestId = sceHttpCreateRequestWithURL2(pState->iSceConnId, _ProtoHttp_strRequestNames[pState->eRequestType], pState->strUrl, (uint64_t)pState->iPostSize);
        }

        if (pState->iSceRequestId < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpCreateRequestWithURL failed with type %s and error code 0x%08x\n", (uintptr_t)pState, _ProtoHttp_strRequestNames[pState->eRequestType], pState->iSceRequestId));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            pState->iSceRequestId = 0;
            sceHttpDeleteConnection(pState->iSceConnId);
            pState->iSceConnId = 0;
        }
        else
        {
            NetPrintfVerbose((pState->iVerbose, 3, "protohttpps4: [0x%08x] request created with id 0x%08x\n", (uintptr_t)pState, pState->iSceRequestId));
        }
    }

    if (pState->iSceRequestId > 0)
    {
        // setup chunking
        if (pState->iPostSize < 0)
        {
            sceHttpSetChunkedTransferEnabled(pState->iSceRequestId, TRUE);
            pState->bChunked = TRUE;
            pState->bRecvEndChunk = FALSE;
        }
        else
        {
            sceHttpSetChunkedTransferEnabled(pState->iSceRequestId, FALSE);
            pState->bChunked = FALSE;
        }

        // associate the Epoll
        if ((iResult = sceHttpSetEpoll(pState->iSceRequestId, pState->EpollHandle, NULL)) < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpSetEpoll failed with code 0x%08x\n", (uintptr_t)pState, pState->iSceConnId));
            pState->eState = ST_FAIL;
            pState->bClosed = TRUE;
            sceHttpUnsetEpoll(pState->iSceRequestId);
            iResult = sceHttpDeleteRequest(pState->iSceRequestId);
            if (iResult != SCE_OK)
            {
                NetPrintf(("protohttpps4: [0x%08x] unable to delete request %d, err=0x%08x\n", (uintptr_t)pState, pState->iSceRequestId, iResult));
            }
            pState->iSceRequestId = 0;
            sceHttpDeleteConnection(pState->iSceConnId);
            pState->iSceConnId = 0;
        }
    }

    // if we requested a connection close, the server may not tell us, so remember it here
    if (pState->iKeepAlive == 0)
    {
        pState->bCloseHdr = TRUE;
    }

    // count the attempt
    pState->iKeepAlive += 1;

    // call the update routine just in case operation can complete
    ProtoHttpUpdate(pState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpResizeBuffer

    \Description
        Resize the buffer

    \Input *pState      - reference pointer
    \Input iBufMax      - new buffer size

    \Output
        int32_t         - zero=success, else failure

    \Version 02/21/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpResizeBuffer(ProtoHttpRefT *pState, int32_t iBufMax)
{
    int32_t iCopySize;
    char *pInpBuf;

    NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] resizing input buffer from %d to %d bytes\n", (uintptr_t)pState, pState->iInpMax, iBufMax));
    if ((pInpBuf = DirtyMemAlloc(iBufMax, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpps4: [0x%08x] could not resize input buffer\n", (uintptr_t)pState));
        return(-1);
    }

    // calculate size of data to copy from old buffer to new
    if ((iCopySize = pState->iInpLen - pState->iInpOff) > iBufMax)
    {
        NetPrintf(("protohttpps4: [0x%08x] warning; resize of input buffer is losing %d bytes of data\n", iCopySize - iBufMax));
        iCopySize = iBufMax;
    }
    // copy valid contents of input buffer, if any, to new buffer
    ds_memcpy(pInpBuf, pState->pInpBuf+pState->iInpOff, iCopySize);

    // dispose of old buffer
    DirtyMemFree(pState->pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // update buffer variables
    pState->pInpBuf = pInpBuf;
    pState->iInpOff = 0;
    pState->iInpLen = iCopySize;
    pState->iInpMax = iBufMax;

    // clear overflow count
    pState->iInpOvr = 0;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpCompactBuffer

    \Description
        Compact the buffer

    \Input *pState      - reference pointer

    \Output
        int32_t         - amount of space freed by compaction

    \Version 07/02/2009 (jbrookes) Extracted from ProtoHttpRecv()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpCompactBuffer(ProtoHttpRefT *pState)
{
    int32_t iCompacted = 0;
    // make sure it needs compacting
    if (pState->iInpOff > 0)
    {
        // compact the buffer
        if (pState->iInpOff < pState->iInpLen)
        {
            memmove(pState->pInpBuf, pState->pInpBuf+pState->iInpOff, pState->iInpLen-pState->iInpOff);
            iCompacted = pState->iInpOff;
        }
        pState->iInpLen -= pState->iInpOff;
        pState->iInpOff = 0;
        pState->bCompactRecv = FALSE;
    }
    return(iCompacted);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpParseHeader

    \Description
        Parse incoming HTTP header.  The HTTP header is presumed to be at the
        beginning of the input buffer.

    \Input  *pState     - reference pointer

    \Output
        int32_t         - negative=not ready or error, else success

    \Version 11/12/2004 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpParseHeader(ProtoHttpRefT *pState)
{
    char *s = pState->pSceRespHeader;
    char *t = pState->pSceRespHeader+pState->iSceRespHeaderLength-3;
    char strTemp[128];

    // scan for blank line marking body start
    while ((s != t) && ((s[0] != '\r') || (s[1] != '\n') || (s[2] != '\r') || (s[3] != '\n')))
    {
        s++;
    }
    if (s == t)
    {
        // header is incomplete
        return(-1);
    }

    // save the head size
    pState->iHeadSize = (int32_t)(s+4-pState->pSceRespHeader);
    // terminate header for easy parsing
    s[2] = s[3] = 0;

    // make sure the header is valid
    if (pState->bVerifyHdr)
    {
        if (strncmp(pState->pSceRespHeader, "HTTP", 4))
        {
            // header is invalid
            NetPrintf(("protohttpps4: [0x%08x] invalid result type\n", (uintptr_t)pState));
            pState->eState = ST_FAIL;
            return(-2);
        }
    }

    // detect if it is a 1.0 response
    pState->bHttp1_0 = !strncmp(pState->pSceRespHeader, "HTTP/1.0", 8);

    // parse header code
    pState->iHdrCode = ProtoHttpParseHeaderCode(pState->pSceRespHeader);

    #if DIRTYCODE_LOGGING
    NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] received %d response (%d bytes)\n", (uintptr_t)pState, pState->iHdrCode, pState->iHeadSize));
    if (pState->iVerbose > 1)
    {
        NetPrintWrap(pState->pSceRespHeader, 80);
    }
    #endif

    // parse content-length field
    if (ProtoHttpGetHeaderValue(pState, pState->pSceRespHeader, "content-length", strTemp, sizeof(strTemp), NULL) != -1)
    {
        pState->iBodySize = ds_strtoll(strTemp, NULL, 10);
        pState->bChunked = FALSE;
    }
    else
    {
        pState->iBodySize = -1;
    }

    // parse last-modified header
    if (ProtoHttpGetHeaderValue(pState, pState->pSceRespHeader, "last-modified", strTemp, sizeof(strTemp), NULL) != -1)
    {
        pState->iHdrDate = (int32_t)ds_strtotime(strTemp);
    }
    else
    {
        pState->iHdrDate = 0;
    }

    // parse transfer-encoding header
    if (ProtoHttpGetHeaderValue(pState, pState->pSceRespHeader, "transfer-encoding", strTemp, sizeof(strTemp), NULL) != -1)
    {
        pState->bChunked = !ds_stricmp(strTemp, "chunked");
    }

    // parse connection header
    if (pState->bCloseHdr == FALSE)
    {
        ProtoHttpGetHeaderValue(pState, pState->pSceRespHeader, "connection", strTemp, sizeof(strTemp), NULL);
        pState->bCloseHdr = !ds_stricmp(strTemp, "close");
    }

    // note if it is an informational header
    pState->bInfoHdr = PROTOHTTP_GetResponseClass(pState->iHdrCode) == PROTOHTTP_RESPONSE_INFORMATIONAL;

    // copy header to header cache
    ds_strnzcpy(pState->strHdr, pState->pSceRespHeader, sizeof(pState->strHdr));

    // trigger recv header user callback, if specified
    if (pState->pReceiveHeaderCb != NULL)
    {
        pState->pReceiveHeaderCb(pState, pState->strHdr, (uint32_t)strlen(pState->strHdr), pState->pCallbackRef);
    }

    // header successfully parsed
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpProcessInfoHeader

    \Description
        Handles an informational response header (response code=1xx)

    \Input  *pState     - reference pointer

    \Output
        None.

    \Version 05/15/2008 (jbrookes) First Version.
*/
/********************************************************************************F*/
static void _ProtoHttpProcessInfoHeader(ProtoHttpRefT *pState)
{
    // ignore the response
    NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] ignoring %d response from server\n", (uintptr_t)pState, pState->iHdrCode));
    
    // reset state to process next header
    pState->eState = ST_HEAD;
}


/*F********************************************************************************/
/*!
    \Function _ProtoHttpRedirectCallback

    \Description
        Handle redirection callback (response code=3xx)

    \Input request      - sce request id
    \Input statusCode   - status code 
    \Input *method      - http method
    \Input *location    - location
    \Input *userArg     - reference pointer

    \Notes
        This method wraps determines whether the sceHttp lib should process the redirect

    \Version 07/20/2016 (amakoukji) First Version.
*/
/********************************************************************************F*/
static int _ProtoHttpRedirectCallback(int32_t request, int32_t statusCode, int32_t *method, const char *location, void *userArg)
{
    ProtoHttpRefT *pState = (ProtoHttpRefT*)userArg;
    NetPrintfVerbose((pState->iVerbose, 2, "protohttpps4: [0x%08x] processing redirect for request 0x%08x code %d\n", (uintptr_t)pState, request, statusCode));
    if ((pState->iMaxRedirect == 0) || (++pState->iNumRedirect > pState->iMaxRedirect))
    {
        if (pState->iMaxRedirect == 0) 
        {
            NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] auto-redirection disabled\n", (uintptr_t)pState));
        }
        else
        {
            NetPrintf(("protohttpps4: [0x%08x] maximum number of redirections (%d) exceeded\n", (uintptr_t)pState, pState->iMaxRedirect));
        }
        pState->eState = ST_DONE;
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSend

    \Description
        Try and send some data.  If data is sent, the timout value is updated.

    \Input  *pState     - reference pointer
    \Input  *pStrBuf    - pointer to buffer to send from
    \Input  iSize       - amount of data to try and send

    \Output
        int32_t         - negative=error, else number of bytes sent

    \Version 11/18/2004 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSend(ProtoHttpRefT *pState, const char *pStrBuf, int32_t iSize)
{
    int32_t iResult = 0, iReturn = 0;
    const void *pPostData = NULL;
    
    // note the SCE HTTP LIB support DELETE sending data but the ProtoHttp Api does not, otherwise it would be included here
    if (pState->eRequestType == PROTOHTTP_REQUESTTYPE_POST || pState->eRequestType == PROTOHTTP_REQUESTTYPE_PUT)
    {
        if (iSize == 0 && pState->bChunked == FALSE)
        {
            // if we're expecting to have some data to post but there's none skip this, sceHttpSendRequest will fail and the transfer will be in a bad state
            // the exception is for chunked transfers where this denotes the end of the data
            return(0);
        }
        else if (pState->bRecvEndChunk)
        {
            // process defered end chunk
            pPostData = NULL;
            pState->bRecvEndChunk = FALSE;
        }
        else
        {
            // otherwise we can data and an expected data size, setup the send
            pPostData = (const void *)pStrBuf;
        }
    }

    // try and send some data
    iResult = sceHttpSendRequest(pState->iSceRequestId, pPostData, iSize);
    if (pState->eState == ST_FAIL)
    {
        // during sceHttpSendRequest's processing it might call a callback where we set the state to ST_FAIL
        // check this first to avoid processing as though a send occured
        pState->iSslFail = 0;
        pState->uSslFailDet = 0;
        pState->iHresult = 0;
        sceHttpsGetSslError(pState->iSceRequestId, &pState->iSslFail, &pState->uSslFailDet);
        sceHttpGetLastErrno(pState->iSceRequestId, &pState->iHresult);
        iReturn = -1;
    }
    else if (iResult == 0)
    {
        #if DIRTYCODE_LOGGING
        if (pState->iVerbose > 2)
        {
            NetPrintf(("protohttpps4: [0x%08x] sent %d bytes\n", (uintptr_t)pState, iSize));
        }
        if (pState->iVerbose > 3)
        {
            NetPrintMem(pStrBuf, iResult, "http-send");
        }
        #endif
        iReturn = iSize;
        
        if (pState->eRequestType != PROTOHTTP_REQUESTTYPE_POST && pState->eRequestType != PROTOHTTP_REQUESTTYPE_PUT)
        {
            // if there's no data to push jump into the ST_RESP state
            // this is required because at a higher level we cannot distinguish between a return code 0 meaning 
            // try again, and a return code 0 meaning 0 bytes were successfully send
            pState->eState = ST_RESP;
        } 
    }
    else if (iResult == SCE_HTTP_ERROR_EAGAIN || iResult == SCE_HTTP_ERROR_BUSY)
    {
        iReturn = 0;
        if (pState->bChunked && iSize == 0 && pPostData == NULL)
        {
            // we've received the end chunk but the scehttp library was unable to process it
            pState->bRecvEndChunk = TRUE;
        }
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttpps4: [0x%08x] error 0x%08x sending %d bytes\n", (uintptr_t)pState, iResult, iSize));
        pState->eState = ST_FAIL;
        pState->iSslFail = 0;
        pState->uSslFailDet = 0;
        pState->iHresult = 0;
        sceHttpsGetSslError(pState->iSceRequestId, &pState->iSslFail, &pState->uSslFailDet);
        sceHttpGetLastErrno(pState->iSceRequestId, &pState->iHresult);
        iReturn = iResult;
    }

    return(iReturn);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSendBuff

    \Description
        Send data queued in buffer.

    \Input  *pState     - reference pointer

    \Output
        int32_t         - negative=error, else number of bytes sent

    \Version 01/29/2010 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSendBuff(ProtoHttpRefT *pState)
{
    int32_t iResult, iSentSize = 0;

    if ((iResult = _ProtoHttpSend(pState, pState->pInpBuf+pState->iInpOff, pState->iInpLen)) > 0)
    {
        pState->iInpOff += iResult;
        pState->iInpLen -= iResult;
        if (pState->iInpLen == 0)
        {
            pState->iInpOff = 0;
        }
        iSentSize = iResult;
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttpps4: [0x%08x] failed to send request data (err=%d)\n", (uintptr_t)pState, iResult));
        pState->iInpLen = 0;
        pState->eState = ST_FAIL;
        iSentSize = -1;
    }
    return(iSentSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRecvData

    \Description
        Try and receive some data.  If data is received, the timout value is
        updated.

    \Input *pState      - reference pointer
    \Input *pStrBuf     - [out] pointer to buffer to receive into
    \Input iSize        - amount of data to try and receive

    \Output
        int32_t         - negative=error, else success

    \Version 11/12/2004 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRecvData(ProtoHttpRefT *pState, char *pStrBuf, int32_t iSize)
{
    // if we have no buffer space, don't try to receive
    if (iSize == 0)
    {
        return(0);
    }

    // try and receive some data
    if ((pState->iRecvRslt = sceHttpReadData(pState->iSceRequestId, (void*)pStrBuf, (size_t)iSize)) > 0)
    {
        #if DIRTYCODE_LOGGING
        if (pState->iVerbose > 2)
        {
            NetPrintf(("protohttpps4: [0x%08x] recv %d bytes\n", (uintptr_t)pState, pState->iRecvRslt));
        }
        if (pState->iVerbose > 3)
        {
            NetPrintMem(pStrBuf, pState->iRecvRslt, "http-recv");
        }
        #endif
    }
    return(pState->iRecvRslt);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpHeaderProcess

    \Description
        Check for header completion and process header data

    \Input *pState          - reference pointer

    \Version 05/03/2012 (jbrookes) Refactored out of ProtoHttpUpdate()
*/
/********************************************************************************F*/
static void _ProtoHttpHeaderProcess(ProtoHttpRefT *pState)
{
    // try parsing header
    if (_ProtoHttpParseHeader(pState) < 0)
    {
        // was there a prior SOCKERR_CLOSED error?
        if (pState->iRecvRslt < 0)
        {
            NetPrintf(("protohttpps4: [0x%08x] ST_HEAD append got ST_FAIL (err=%d, len=%d)\n", (uintptr_t)pState, pState->iRecvRslt, pState->iInpLen));
            pState->eState = ST_FAIL;
        }
        // if the buffer is full, we don't have enough room to receive the header
        if (pState->iInpLen == pState->iInpMax)
        {
            if (pState->iInpOvr == 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] input buffer too small for response header\n", (uintptr_t)pState));
            }
            pState->iInpOvr = pState->iInpLen+1;
        }
        return;
    }

    /* workaround for non-compliant 1.0 servers - some 1.0 servers specify a
       Content Length of zero invalidly.  if the response is a 1.0 response
       and the Content Length is zero, and we've gotten body data, we set
       the Content Length to -1 (indeterminate) */
    if ((pState->bHttp1_0 == TRUE) && (pState->iBodySize == 0) && (pState->iInpCnt > 0))
    {
        pState->iBodySize = -1;
    }

    // handle final processing
    if ((pState->bHeadOnly == TRUE) || (pState->iHdrCode == PROTOHTTP_RESPONSE_NOCONTENT) || (pState->iHdrCode == PROTOHTTP_RESPONSE_NOTMODIFIED))
    {
        // only needed the header (or response did not include a body; see HTTP RFC 1.1 section 4.4) -- all done
        pState->eState = ST_DONE;
    }
    else if ((pState->iBodySize >= 0) && (pState->iInpCnt >= pState->iBodySize))
    {
        // we got entire body with header -- all done
        pState->eState = ST_DONE;
    }
    else
    {
        // wait for rest of body
        pState->eState = ST_BODY;
    }

    // handle response code?
    if (PROTOHTTP_GetResponseClass(pState->iHdrCode) == PROTOHTTP_RESPONSE_INFORMATIONAL)
    {
        _ProtoHttpProcessInfoHeader(pState);
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRecvBody

    \Description
        Attempt to recevive and process body data

    \Input *pState          - reference pointer

    \Output
        int32_t             - zero=no data available

    \Version 05/03/2012 (jbrookes) Refactored out of ProtoHttpUpdate()
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRecvBody(ProtoHttpRefT *pState)
{
    int32_t iResult;

    // reset pointers if needed
    if ((pState->iInpLen > 0) && (pState->iInpOff == pState->iInpLen))
    {
        pState->iInpOff = pState->iInpLen = 0;
    }

    // check for data
    iResult = pState->iInpMax - pState->iInpLen;
    if (iResult <= 0)
    {
        // always return zero bytes since buffer is full
        return(0);
    }
    else
    {
        // try to add to buffer
        iResult = _ProtoHttpRecvData(pState, pState->pInpBuf+pState->iInpLen, iResult);
    }
    if (iResult == 0)
    {
        // this marks the end of a chunked transaction
        pState->eState = ST_DONE;
        return(0);
    }
    // check for connection close
    else if (iResult > 0)
    {
        // add to buffer
        pState->iInpLen += iResult;
        pState->iInpCnt += iResult;

        // check for end of body
        if ((pState->iBodySize >= 0) && (pState->iInpCnt >= pState->iBodySize))
        {
            pState->eState = ST_DONE;
        }
    }
    else if (iResult < 0 && iResult != SCE_HTTP_ERROR_BUSY && iResult != SCE_HTTP_ERROR_EAGAIN)
    {
        NetPrintf(("protohttpps4: [0x%08x] ST_FAIL (err=%d)\n", (uintptr_t)pState, iResult));
        pState->eState = ST_FAIL;
        pState->iSslFail = 0;
        pState->uSslFailDet = 0;
        pState->iHresult = 0;
        sceHttpsGetSslError(pState->iSceRequestId, &pState->iSslFail, &pState->uSslFailDet);
        sceHttpGetLastErrno(pState->iSceRequestId, &pState->iHresult);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRecv

    \Description
        Return the actual url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufMin  - minimum number of bytes to return (returns zero if this number is not available)
    \Input iBufMax  - maximum number of bytes to return (buffer size)

    \Output
        int32_t     - negative=error, zero=no data available or bufmax <= 0, positive=number of bytes read

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRecv(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    int32_t iLen;

    // early out for failure result
    if (pState->eState == ST_FAIL)
        return(PROTOHTTP_RECVFAIL);
    // check for input buffer too small for header
    if (pState->iInpOvr > 0)
        return(PROTOHTTP_MINBUFF);
    // waiting for data
    if ((pState->eState != ST_BODY) && (pState->eState != ST_DONE))
        return(PROTOHTTP_RECVWAIT);

    // if they only wanted head, thats all they get
    if (pState->bHeadOnly == TRUE)
        return(PROTOHTTP_RECVHEAD);

    // if they are querying only for done state when no more data is available to be read
    if((iBufMax == 0) && (pState->eState == ST_DONE && pState->iBodyRcvd == pState->iBodySize))
        return(PROTOHTTP_RECVDONE);

    // make sure range is valid
    if (iBufMax < 1)
        return(0);

    // clamp the range
    if (iBufMin < 1)
        iBufMin = 1;
    if (iBufMax < iBufMin)
        iBufMax = iBufMin;
    if (iBufMin > pState->iInpMax)
        iBufMin = pState->iInpMax;
    if (iBufMax > pState->iInpMax)
        iBufMax = pState->iInpMax;

    // see if we need to shift buffer
    if ((iBufMin > pState->iInpMax-pState->iInpOff) || (pState->bCompactRecv == TRUE))
    {
        // compact receive buffer
        _ProtoHttpCompactBuffer(pState);
        // give chance to get more data
        _ProtoHttpRecvBody(pState);
    }

    // figure out how much data is available
    if ((iLen = (pState->iInpLen - pState->iInpOff)) > iBufMax)
    {
        iLen = iBufMax;
    }

    // check for end of data
    if ((iLen == 0) && (pState->eState == ST_DONE))
    {
        return(PROTOHTTP_RECVDONE);
    }

    // see if there is enough to return
    if ((iLen >= iBufMin) || (pState->iInpCnt == pState->iBodySize))
    {
        // return data to caller
        if (pBuffer != NULL)
        {
            ds_memcpy(pBuffer, pState->pInpBuf+pState->iInpOff, iLen);

            #if DIRTYCODE_LOGGING
            if (pState->iVerbose > 3)
            {
                NetPrintf(("protohttpps4: [0x%08x] read %d bytes\n", (uintptr_t)pState, iLen));
            }
            if (pState->iVerbose > 4)
            {
                NetPrintMem(pBuffer, iLen, "http-read");
            }
            #endif
        }
        pState->iInpOff += iLen;
        pState->iBodyRcvd += iLen;
        
        // return bytes read
        return(iLen);
    }

    // nothing available
    return(0);
}


/*F********************************************************************************/
/*!
    \Function _ProtoHttpWriteCbProcess

    \Description
        User write callback processing, if write callback is set

    \Input *pState          - reference pointer

    \Version 05/03/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpWriteCbProcess(ProtoHttpRefT *pState)
{
    ProtoHttpWriteCbInfoT WriteCbInfo;
    int32_t iResult;

    memset(&WriteCbInfo, 0, sizeof(WriteCbInfo));
    WriteCbInfo.eRequestType = pState->eRequestType;
    WriteCbInfo.eRequestResponse = (ProtoHttpResponseE)pState->iHdrCode;

    if (pState->eState == ST_BODY)
    {
        char strTempRecv[1024];
        while ((iResult = _ProtoHttpRecv(pState, strTempRecv, 1, sizeof(strTempRecv))) > 0)
        {
            pState->pWriteCb(pState, &WriteCbInfo, strTempRecv, iResult, pState->pWriteCbUserData);
        }
    }
    else if (pState->eState > ST_BODY)
    {
        if (pState->eState == ST_DONE)
        {
            pState->pWriteCb(pState, &WriteCbInfo, "", pState->bHeadOnly ? PROTOHTTP_HEADONLY : PROTOHTTP_RECVDONE, pState->pWriteCbUserData);
        }
        if (pState->eState == ST_FAIL)
        {
            pState->pWriteCb(pState, &WriteCbInfo, "", PROTOHTTP_RECVFAIL, pState->pWriteCbUserData);
        }
        pState->pWriteCb = NULL;
        pState->pWriteCbUserData = NULL;
    }
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoHttpCreate

    \Description
        Allocate module state and prepare for use

    \Input iBufSize     - length of receive buffer

    \Output
        ProtoHttpRefT * - pointer to module state, or NULL

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
ProtoHttpRefT *ProtoHttpCreate(int32_t iBufSize)
{
    ProtoHttpRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    int32_t iNetMemId, iSslContextId, iHttpContextId; // Sony library ids


    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp the buffer size
    if (iBufSize < 4096)
    {
        iBufSize = 4096;
    }
        
    // create the netpool for this ref
    iNetMemId = DirtyContextManagerCreateNetPoolContext("protohttpps4", iBufSize);
    if (iNetMemId < 0)
    {
        NetPrintf(("protohttpps4: unable to allocate sceNetPool, error 0x%08x\n", iNetMemId));
        return(NULL);
    }

    // create the ssl context for this ref
    if ((iSslContextId = DirtyContextManagerCreateSslContext(PROTOHTTP_SSL_POOLSIZE)) < 0)
    {
        NetPrintf(("protohttpps4: unable to initialize sceSslContext, error 0x%08x\n", iSslContextId));
        DirtyContextManagerFreeNetPoolContext(iNetMemId);
        return(NULL);
    }

    // create the http context for this ref
    // note that we need extra room in this buffer since it deals not only with the request but the response
    // it has been observed that multiple requests with input near the buffer size can run the system out of  
    // memory quickly even if the previous requests are deleted successfully
    if ((iHttpContextId = DirtyContextManagerCreateHttpContext(iNetMemId, iSslContextId, iBufSize*2)) < 0)
    {
        NetPrintf(("protohttpps4: unable to initialize sceHttpContext, error 0x%08x\n", iHttpContextId));
        DirtyContextManagerFreeSslContext(iSslContextId);
        DirtyContextManagerFreeNetPoolContext(iNetMemId);
        return(NULL);
    }

    // allocate the resources
    if ((pState = DirtyMemAlloc(sizeof(*pState), PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpps4: [0x%08x] unable to allocate module state\n", (uintptr_t)pState));
        DirtyContextManagerFreeHttpContext(iHttpContextId);
        DirtyContextManagerFreeSslContext(iSslContextId);
        DirtyContextManagerFreeNetPoolContext(iNetMemId);
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));
    // save memgroup (will be used in ProtoHttpDestroy)
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iNetMemId = iNetMemId; 
    pState->iSslContextId = iSslContextId;
    pState->iHttpContextId = iHttpContextId;

    if ((pState->pInpBuf = DirtyMemAlloc(iBufSize, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpps4: [0x%08x] unable to allocate protohttp buffer\n", (uintptr_t)pState));
        ProtoHttpDestroy(pState);
        return(NULL);
    }

    // set the loaded list version to -1 to be sure the GOS CAs get loaded
    pState->uCaListVersionLoaded = -1;

    // init crit
    NetCritInit(&pState->HttpCrit, "ProtoHttp");

    // save parms & set defaults
    pState->eState = ST_IDLE;
    pState->iInpMax = iBufSize;
    pState->uConnectTimeout = PROTOHTTP_TIMEOUT;
    pState->uRecvTimeout = PROTOHTTP_TIMEOUT;
    pState->uSendTimeout = PROTOHTTP_TIMEOUT;
    pState->uResolveTimeout = PROTOHTTP_TIMEOUT;
    pState->iResolveRetries = PROTOHTTP_RESOLVE_RETRIES;

    pState->bVerifyHdr = TRUE;
    pState->iVerbose = 1;
    pState->iMaxRedirect = PROTOHTTP_MAXREDIRECT;
    pState->Vers = SCE_SSL_VERSION_TLS1_2;
    // note: SCE_HTTPS_FLAGS_DEFAULT would normally be used but seems to contain an undeclared flag, use the equivalent
    pState->uSceHttpFlags = SCE_HTTPS_FLAG_SERVER_VERIFY | SCE_HTTPS_FLAG_CN_CHECK | SCE_HTTPS_FLAG_KNOWN_CA_CHECK | SCE_HTTPS_FLAG_SNI;
    pState->bSetSslVers = FALSE;
    pState->bSetHttpsFlags = FALSE;

    // return the state
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpCallback

    \Description
        Set header callbacks.

    \Input *pState          - module state
    \Input *pCustomHeaderCb - pointer to custom send header callback (may be NULL)
    \Input *pReceiveHeaderCb- pointer to recv header callback (may be NULL)
    \Input *pCallbackRef    - user-supplied callback ref (may be NULL)

    \Notes
        The ProtHttpCustomHeaderCbt callback is used to allow customization of
        the HTTP header before sending.  It is more powerful than the append
        header functionality, allowing to make changes to any part of the header
        before it is sent.  The callback should return a negative code if an error
        occurred, zero can be returned if the application wants ProtoHttp to
        calculate the header size, or the size of the header can be returned if
        the application has already calculated it.  The header should *not* be
        terminated with the double \\r\\n that indicates the end of the entire
        header, as protohttp appends itself.

        The ProtoHttpReceiveHeaderCbT callback is used to view the header
        immediately on reception.  It can be used when the built-in header
        cache (retrieved with ProtoHttpStatus('htxt') is too small to hold
        the entire header received.  It is also possible with this method
        to view redirection response headers that cannot be retrieved
        normally.  This can be important if, for example, the application
        wishes to attach new cookies to a redirection response.  The
        custom response header and custom header callback can be used in
        conjunction to implement this type of functionality.

    \Version 1.0 02/16/2007 (jbrookes) First Version
*/
/********************************************************************************F*/
void ProtoHttpCallback(ProtoHttpRefT *pState, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pCallbackRef)
{
    pState->pCustomHeaderCb = pCustomHeaderCb;
    pState->pReceiveHeaderCb = pReceiveHeaderCb;
    pState->pCallbackRef = pCallbackRef;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpDestroy

    \Description
        Destroy the module and release its state

    \Input *pState      - reference pointer

    \Output
        None.

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
void ProtoHttpDestroy(ProtoHttpRefT *pState)
{
    int32_t iResult = 0;
    if (pState->pInpBuf != NULL)
    {
        DirtyMemFree(pState->pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
    if (pState->pAppendHdr != NULL)
    {
        DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
    
    if (pState->iSceRequestId != 0)
    {
        sceHttpUnsetEpoll(pState->iSceRequestId);
        iResult = sceHttpDeleteRequest(pState->iSceRequestId);
        if (iResult != SCE_OK)
        {
            NetPrintf(("protohttpps4: [0x%08x] unable to delete request %d, err=0x%08x\n", (uintptr_t)pState, pState->iSceRequestId, iResult));
        }
    }
    if (pState->iSceConnId != 0)
    {
        sceHttpDeleteConnection(pState->iSceConnId);
    }
    if (pState->iSceTemplateId != 0)
    {
        sceHttpDeleteTemplate(pState->iSceTemplateId);
    }
    if (pState->EpollHandle != NULL)
    {
        sceHttpDestroyEpoll(pState->iHttpContextId, pState->EpollHandle);
    }

    NetCritKill(&pState->HttpCrit);
    DirtyContextManagerFreeHttpContext(pState->iHttpContextId);
    DirtyContextManagerFreeSslContext(pState->iSslContextId);
    DirtyContextManagerFreeNetPoolContext(pState->iNetMemId);
    DirtyMemFree(pState, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGet

    \Description
        Initiate an HTTP transfer. Pass in a URL and the module starts a transfer
        from the appropriate server.

    \Input *pState      - reference pointer
    \Input *pUrl        - the url to retrieve
    \Input bHeadOnly    - if TRUE only get header

    \Output
        int32_t         - negative=failure, else success

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpGet(ProtoHttpRefT *pState, const char *pUrl, uint32_t bHeadOnly)
{
    int32_t iResult;

    // reset redirection count
    pState->iNumRedirect = 0;

    pState->iPostSize = 0;

    // format the request
    if (pUrl != NULL)
    {
        if ((iResult = _ProtoHttpFormatRequest(pState, pUrl, NULL, 0, bHeadOnly ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_GET)) < 0)
        {
            return(iResult);
        }
        _ProtoHttpSendRequest(pState);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRecv

    \Description
        Return the actual url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufMin  - minimum number of bytes to return (returns zero if this number is not available)
    \Input iBufMax  - maximum number of bytes to return (buffer size)

    \Output
        int32_t     - negative=error, zero=no data available or bufmax <= 0, positive=number of bytes read

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpRecv(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    int32_t iResult;

    NetCritEnter(&pState->HttpCrit);
    iResult = _ProtoHttpRecv(pState, pBuffer, iBufMin, iBufMax);
    NetCritLeave(&pState->HttpCrit);

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRecvAll

    \Description
        Return all of the url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufSize - size of buffer

    \Output
        int32_t     - PROTOHTTP_RECV*, or positive=bytes in response

    \Version 12/14/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpRecvAll(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufSize)
{
    int32_t iRecvMax, iRecvResult;

    // try to read as much data as possible (leave one byte for null termination)
    for (iRecvMax = iBufSize-1; (iRecvResult = ProtoHttpRecv(pState, pBuffer + pState->iRecvSize, 1, iRecvMax - pState->iRecvSize)) > 0; )
    {
        // add to amount received
        pState->iRecvSize += iRecvResult;
    }

    // check response code
    if (iRecvResult == PROTOHTTP_RECVDONE)
    {
        // null-terminate response & return completion success
        pBuffer[pState->iRecvSize] = '\0';
        iRecvResult = pState->iRecvSize;
    }
    else if ((iRecvResult < 0) && (iRecvResult != PROTOHTTP_RECVWAIT))
    {
        // an error occurred
        NetPrintf(("protohttpps4: [0x%08x] error %d receiving response\n", (uintptr_t)pState, iRecvResult));
    }
    else if (iRecvResult == 0)
    {
        iRecvResult = (pState->iRecvSize < iRecvMax) ? PROTOHTTP_RECVWAIT : PROTOHTTP_RECVBUFF;
    }

    // return result to caller
    return(iRecvResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpPost

    \Description
        Initiate transfer of data to to the server via a HTTP POST command.

    \Input *pState      - reference pointer
    \Input *pUrl        - the URL that identifies the POST action.
    \Input *pData       - pointer to URL data (optional, may be NULL)
    \Input iDataSize    - size of data being uploaded (see Notes below)
    \Input bDoPut       - if TRUE, do a PUT instead of a POST

    \Output
        int32_t         - negative=failure, else number of data bytes sent

    \Notes
        Any data that is not sent as part of the Post transaction should be sent
        with ProtoHttpSend().  ProtoHttpSend() should be called at poll rate (i.e.
        similar to how often ProtoHttpUpdate() is called) until all of the data has
        been sent.  The amount of data bytes actually sent is returned by the
        function.

        If pData is not NULL and iDataSize is less than or equal to zero, iDataSize
        will be recalculated as the string length of pData, to allow for easy string
        sending.

        If pData is NULL and iDataSize is negative, the transaction is assumed to
        be a streaming transaction and a chunked transfer will be performed.  A
        subsequent call to ProtoHttpSend() with iDataSize equal to zero will end
        the transaction.

    \Version 03/03/2004 (sbevan) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpPost(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize, uint32_t bDoPut)
{
    int32_t iDataSent;
    // allow for easy string sending
    if ((pData != NULL) && (iDataSize <= 0))
    {
        iDataSize = (int32_t)strlen(pData);
    }
    // save post size (or -1 to indicate that this is a variable-length stream)
    pState->iPostSize = iDataSize;
    // make the request
    if ((iDataSent = _ProtoHttpFormatRequest(pState, pUrl, pData, iDataSize, bDoPut ? PROTOHTTP_REQUESTTYPE_PUT : PROTOHTTP_REQUESTTYPE_POST)) >= 0)
    {
        // send the request
        _ProtoHttpSendRequest(pState);
    }
    return(iDataSent);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSend

    \Description
        Send data during an ongoing Post transaction.

    \Input *pState      - reference pointer
    \Input *pData       - pointer to data to send
    \Input iDataSize    - size of data being sent

    \Output
        int32_t         - negative=failure, else number of data bytes sent

    \Version 11/18/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpSend(ProtoHttpRefT *pState, const char *pData, int32_t iDataSize)
{
    int32_t iResult;

    // handle use case where data is specified after the ProtoHttpPost() call
    if (pState->bWaitForData > 0 && pData != NULL)
    {
        pState->eState = ST_RESP;
        pState->bWaitForData = FALSE;
    }

    // make sure we are in a sending state
    if (pState->eState < ST_RESP)
    {
        // not ready to send data yet
        return(0);
    }
    else if (pState->eState != ST_RESP)
    {
        // if we're past ST_RESP, an error occurred.
        return(-1);
    }

    /* clamp to max ProtoHttp buffer size - even though
       we don't queue the data in the ProtoHttp buffer, this
       gives us a reasonable max size to send in one chunk */
    if (iDataSize > pState->iInpMax)
    {
        iDataSize = pState->iInpMax;
    }

    // get sole access to httpcrit
    NetCritEnter(&pState->HttpCrit);
    // send the data
    
    iResult = _ProtoHttpSend(pState, pData, iDataSize);
    // release access to httpcrit
    NetCritLeave(&pState->HttpCrit);

    // return result
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpDelete

    \Description
        Request deletion of a server-based resource.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url describing reference to delete

    \Output
        int32_t         - negative=failure, zero=success

    \Version 06/01/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpDelete(ProtoHttpRefT *pState, const char *pUrl)
{
    int32_t iResult;

    // reset redirection count
    pState->iNumRedirect = 0;

    // delete with data not supported
    pState->iPostSize = 0;

    // format the request
    if ((iResult = _ProtoHttpFormatRequest(pState, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_DELETE)) >= 0)
    {
        // issue the request
        _ProtoHttpSendRequest(pState);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpOptions

    \Description
        Request options from a server.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url describing reference to get options on

    \Output
        int32_t         - negative=failure, zero=success

    \Version 07/14/2010 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpOptions(ProtoHttpRefT *pState, const char *pUrl)
{
    int32_t iResult;

    // reset redirection count
    pState->iNumRedirect = 0;

    pState->iPostSize = 0;

    // format the request
    if ((iResult = _ProtoHttpFormatRequest(pState, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_OPTIONS)) >= 0)
    {
        // issue the request
        _ProtoHttpSendRequest(pState);
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRequestCb

    \Description
        Initiate an HTTP transfer. Pass in a URL and the module starts a transfer
        from the appropriate server.  Use ProtoHttpRequest() macro wrapper if
        write callback is not required.

    \Input *pState      - reference pointer
    \Input *pUrl        - the url to retrieve
    \Input *pData       - user data to send to server (PUT and POST only)
    \Input iDataSize    - size of user data to send to server (PUT and POST only)
    \Input eRequestType - request type to make
    \Input *pWriteCb    - write callback (optional)
    \Input *pWriteCbUserData - write callback user data (optional)

    \Output
        int32_t         - negative=failure, zero=success, >0=number of data bytes sent (PUT and POST only)

    \Version 06/01/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpRequestCb(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize, ProtoHttpRequestTypeE eRequestType, ProtoHttpWriteCbT *pWriteCb, void *pWriteCbUserData)
{
    // save write callback
    pState->pWriteCb = pWriteCb;
    pState->pWriteCbUserData = pWriteCbUserData;

    // make the request
    if ((eRequestType == PROTOHTTP_REQUESTTYPE_GET) || (eRequestType == PROTOHTTP_REQUESTTYPE_HEAD))
    {
        return(ProtoHttpGet(pState, pUrl, eRequestType == PROTOHTTP_REQUESTTYPE_HEAD));
    }
    else if ((eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (eRequestType == PROTOHTTP_REQUESTTYPE_POST))
    {
        return(ProtoHttpPost(pState, pUrl, pData, iDataSize, eRequestType == PROTOHTTP_REQUESTTYPE_PUT));
    }
    else if (eRequestType == PROTOHTTP_REQUESTTYPE_DELETE)
    {
        return(ProtoHttpDelete(pState, pUrl));
    }
    else if (eRequestType == PROTOHTTP_REQUESTTYPE_OPTIONS)
    {
        return(ProtoHttpOptions(pState, pUrl));
    }
    else
    {
        NetPrintf(("protohttpps4: [0x%08x] unrecognized request type %d\n", (uintptr_t)pState, eRequestType));
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpAbort

    \Description
        Abort current operation, if any.

    \Input *pState      - reference pointer

    \Output
        None.

    \Version 12/01/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
void ProtoHttpAbort(ProtoHttpRefT *pState)
{
    // acquire sole access to http crit
    NetCritEnter(&pState->HttpCrit);

    // terminate current connection, if any
    _ProtoHttpClose(pState, "abort");

    // reset state
    _ProtoHttpReset(pState);

    // release sole access to http crit
    NetCritLeave(&pState->HttpCrit);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetBaseUrl

    \Description
        Set base url that will be used for any relative url references.

    \Input *pState      - module state
    \Input *pUrl        - base url

    \Output
        None

    \Version 02/03/2010 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpSetBaseUrl(ProtoHttpRefT *pState, const char *pUrl)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;

    // parse the url for kind, host, and port
    ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // set base info
    NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] setting base url to %s://%s:%d\n", (uintptr_t)pState, iSecure ? "https" : "http", strHost, iPort));
    ds_strnzcpy(pState->strBaseHost, strHost, sizeof(pState->strBaseHost));
    pState->iBasePort = iPort;
    pState->iBaseSecure = iSecure;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGetLocationHeader

    \Description
        Get location header from the input buffer.  The Location header requires
        some special processing.

    \Input *pState  - reference pointer
    \Input *pInpBuf - buffer holding header text
    \Input *pBuffer - [out] output buffer for parsed location header, null for size request
    \Input iBufSize - size of output buffer, zero for size request
    \Input **pHdrEnd- [out] pointer past end of parsed header (optional)

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 03/24/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetLocationHeader(ProtoHttpRefT *pState, const char *pInpBuf, char *pBuffer, int32_t iBufSize, const char **pHdrEnd)
{
    const char *pLocHdr;
    int32_t iLocLen, iLocPreLen=0;

    // get a pointer to header
    if ((pLocHdr = ProtoHttpFindHeaderValue(pInpBuf, "location")) == NULL)
    {
        return(-1);
    }

    /* according to RFC location headers should be absolute, but some webservers respond with relative
       URL's.  we assume any url that does not include "://" is a relative url, and if we find one, we
       assume the request keeps the same security, port, and host as the previous request */
    if ((pState != NULL) && (!strstr(pLocHdr, "://")))
    {
        char strTemp[288]; // space for max DNS name (253 chars) plus max http url prefix

        // format http url prefix
        if ((pState->iSecure && (pState->iPort == 443)) || (pState->iPort == 80))
        {
            ds_snzprintf(strTemp, sizeof(strTemp), "%s://%s", pState->iSecure ? "https" : "http", pState->strHost);
        }
        else
        {
            ds_snzprintf(strTemp, sizeof(strTemp), "%s://%s:%d", pState->iSecure ? "https" : "http", pState->strHost, pState->iPort);
        }

        // make sure relative URL includes '/' as the first character, required when we format the redirection url
        if (*pLocHdr != '/')
        {
            ds_strnzcat(strTemp, "/", sizeof(strTemp));
        }

        // calculate url prefix length
        iLocPreLen = (int32_t)strlen(strTemp);

        // copy url prefix text if a buffer is specified
        if (pBuffer != NULL)
        {
            ds_strnzcpy(pBuffer, strTemp, iBufSize);
            pBuffer = (char *)((uint8_t *)pBuffer + iLocPreLen);
            iBufSize -= iLocPreLen;
        }
    }

    // extract location header and return size
    iLocLen = ProtoHttpExtractHeaderValue(pLocHdr, pBuffer, iBufSize, pHdrEnd);
    // if it's a size request add in (possible) url header length
    if ((pBuffer == NULL) && (iBufSize == 0))
    {
        iLocLen += iLocPreLen;
    }

    // return to caller
    return(iLocLen);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpControl

    \Description
        ProtoHttp control function.  Different selectors control different behaviors.

    \Input *pState  - reference pointer
    \Input iSelect  - control selector ('time')
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'apnd'      The given buffer will be appended to future headers sent
                        by ProtoHttp.  Note that the User-Agent and Accept lines
                        in the default header will be replaced, so if these lines
                        are desired, they should be supplied in the append header.
            'disc'      Close current connection, if any.
            'hver'      Sets header type verification: zero=disabled, else enabled
            'ires'      Resize input buffer
            'keep'      Sets keep-alive; zero=disabled, else enabled
            'resr'      Sets ProtoHttp client max connect dns name resolution retries (1)
            'rmax'      Sets maximum number of redirections (default=3; 0=disable)
            'rput'      Sets connection-reuse on put/post (TRUE=enabled, default FALSE)
            'spam'      Sets debug output verbosity (0...n)
            'time'      Sets ProtoHttp client milliseconds (default=30s)
        \endverbatim

        Unhandled selectors are passed on to ProtoSSL.

    \Version 11/12/2004 (jbrookes) First Version
*/
/*******************************************************************************F*/
int32_t ProtoHttpControl(ProtoHttpRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    int32_t iResult = 0;

    if (iSelect == 'apnd')
    {
        return(_ProtoHttpSetAppendHeader(pState, (const char *)pValue));
    }
    if (iSelect == 'disc')
    {
        _ProtoHttpClose(pState, "user request");
        return(0);
    }
    if (iSelect == 'hver')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] header type verification %s\n", (uintptr_t)pState, iValue ? "enabled" : "disabled"));
        pState->bVerifyHdr = iValue;
    }
    if (iSelect == 'ires')
    {
        return(_ProtoHttpResizeBuffer(pState, iValue));
    }
    if (iSelect == 'keep')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] setting keep-alive to %d\n", (uintptr_t)pState, iValue));
        pState->iKeepAlive = pState->iKeepAliveDflt = iValue;
        return(0);
    }
    if (iSelect == 'prxy')
    {
        /* Unable to support with sceHttp library
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] setting %s as proxy server\n", (uintptr_t)pState, pValue));
        ds_strnzcpy(pState->strProxy, pValue, sizeof(pState->strProxy));
        return(0);
        */
        return(-1);
    }
    if (iSelect == 'rmax')
    {
        pState->iMaxRedirect = iValue;
        return(0);
    }
    if (iSelect == 'rput')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] connection reuse on put/post %s\n", (uintptr_t)pState, iValue ? "enabled" : "disabled"));
        pState->bReuseOnPost = iValue ? TRUE : FALSE;
        return(0);
    }
    if (iSelect == 'spam')
    {
        pState->iVerbose = iValue;
        // fall through to protossl
    }
    if (iSelect == 'time')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] setting connection timeout to %d ms\n", (uintptr_t)pState, iValue));
        pState->uConnectTimeout = (unsigned)iValue;
        pState->uRecvTimeout    = (unsigned)iValue;
        pState->uResolveTimeout = (unsigned)iValue;
        pState->uSendTimeout    = (unsigned)iValue;
       
        if (pState->iSceTemplateId > 0)
        {
            if ((iResult = sceHttpSetConnectTimeOut(pState->iSceTemplateId, (pState->uConnectTimeout * 1000))) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [0x%08x] sceHttpSetConnectTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            }
            if ((iResult = sceHttpSetRecvTimeOut(pState->iSceTemplateId, (pState->uRecvTimeout * 1000))) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [0x%08x] sceHttpSetRecvTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            }
            if ((iResult = sceHttpSetResolveTimeOut(pState->iSceTemplateId, (pState->uResolveTimeout * 1000))) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [0x%08x] sceHttpSetResolveTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            }
            if ((iResult = sceHttpSetSendTimeOut(pState->iSceTemplateId, (pState->uSendTimeout * 1000))) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttp: [0x%08x] sceHttpSetSendTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            }
        }
        return(0);
    }
    if (iSelect == 'resr')
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpps4: [0x%08x] dns resolve retry attempts to %d\n", (uintptr_t)pState, iValue));
        pState->iResolveRetries = iValue;
       
        // set the connection timeout
        if (pState->iSceTemplateId > 0)
        {
            if ((iResult = sceHttpSetResolveRetry(pState->iSceTemplateId, pState->iResolveRetries)) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpSetSendTimeOut failed with code 0x%08x\n", (uintptr_t)pState, iResult));
            }
        }
        return(0);
    }

    // inherited from ProtoSSL, these now need to be handled here
    if (iSelect == 'scrt')
    {
        if (iValue > 0)
        {
            NetPrintf(("protohttpps4: setting cert (%d bytes)\n", iValue));
            pState->ClientCertData.size = (size_t)iValue;
            pState->ClientCertData.ptr = (char*)pValue;
            pState->bUseClientCert = TRUE;
        }
        else
        {
            NetPrintf(("protohttpps4: removing cert\n"));
            pState->bUseClientCert = FALSE;
        }
    }
    if (iSelect == 'skey')
    {
        if (iValue > 0)
        {
            NetPrintf(("protohttpps4: setting private key (%d bytes)\n", iValue));
            pState->PrivateKeyData.size = (size_t)iValue;
            pState->PrivateKeyData.ptr = (char*)pValue;
            pState->bUsePrivateKey = TRUE;
        }
        else
        {
            NetPrintf(("protohttpps4: removing private key\n", iValue));
            pState->bUsePrivateKey = FALSE;
        }
    }
    if (iSelect == 'shsf')
    {
        pState->bSetHttpsFlags = TRUE;
        if (iValue2 > 0)
        {
            pState->uSceHttpFlags |= (uint32_t)iValue;
        }
        else
        {
            pState->uSceHttpFlags &= ~((uint32_t)iValue);
        }
        
        if (pState->iSceTemplateId > 0)
        {
            // reset flags and apply the new state
            if ((iResult = sceHttpsDisableOption(pState->iSceTemplateId, SCE_HTTPS_FLAG_SERVER_VERIFY|SCE_HTTPS_FLAG_CLIENT_VERIFY|SCE_HTTPS_FLAG_CN_CHECK|SCE_HTTPS_FLAG_NOT_AFTER_CHECK|SCE_HTTPS_FLAG_NOT_BEFORE_CHECK|SCE_HTTPS_FLAG_KNOWN_CA_CHECK|SCE_HTTPS_FLAG_SESSION_REUSE|SCE_HTTPS_FLAG_SNI)) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpsDisableOption failed with code 0x%08x\n", (uintptr_t)pState, iResult));
                pState->bSetHttpsFlags = FALSE;
            }

            if ((iResult = sceHttpsEnableOption(pState->iSceTemplateId, pState->uSceHttpFlags)) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpsEnableOption failed with code 0x%08x, flags 0x%08x\n", (uintptr_t)pState, iResult, pState->uSceHttpFlags));
                pState->bSetHttpsFlags = FALSE;
            }
        }
    }
    if (iSelect == 'vers')
    {
        NetPrintfVerbose((pState->iVerbose, 2, "protohttpps4: [0x%08x] using SSL version %s\n", (uintptr_t)pState, _HTTP_strVersionNames[iValue&0xff]));
        pState->Vers = _ProtoHttpProtoSslToSceVers(iValue);
        pState->bSetSslVers = TRUE;

        if (pState->iSceTemplateId > 0)
        {
            if ((iResult = sceHttpsSetSslVersion(pState->iSceTemplateId, pState->Vers)) < 0)
            {
                NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] sceHttpsSetSslVersion failed with code 0x%08x\n", (uintptr_t)pState, iResult));
                pState->bSetSslVers = FALSE;
            }
        }
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpStatus

    \Description
        Return status of current HTTP transfer.  Status type depends on selector.

    \Input *pState  - reference pointer
    \Input iSelect  - info selector (see Notes)
    \Input *pBuffer - [out] storage for selector-specific output
    \Input iBufSize - size of buffer

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'addr'      returns target address, if available
        'body'      negative=failed or pending, else size of body (for 64bit size, get via pBuffer)
        'code'      negative=no response, else server response code (ProtoHttpResponseE)
        'data'      negative=failed, zero=pending, positive=amnt of data ready
        'date'      last-modified date, if available
        'done'      negative=failed, zero=pending, positive=done
        'essl'      returns protossl error state
        'head'      negative=failed or pending, else size of header
        'host'      current host copied to output buffer
        'hres'      return hResult containing either the socket error or ssl error or the http status code 
        'htxt'      current received http header text copied to output buffer
        'info'      copies most recent info header received, if any, to output buffer (one time only)
        'imax'      size of input buffer
        'iovr'      returns input buffer overflow size (valid on PROTOHTTP_MINBUFF result code)
        'port'      current port
        'rtxt'      most recent http request header text copied to output buffer
        'rmax'      returns currently configured max redirection count
        'serr'      return socket error
        'time'      TRUE if the client timed out the connection, else FALSE
    \endverbatim

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpStatus(ProtoHttpRefT *pState, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{


    // return protossl error state (we cache this since we reset the state when we disconnect on error)
    if (iSelect == 'essl')
    {
        return(pState->iSslFail);
    }
    // return current host
    if (iSelect == 'host')
    {
        ds_strnzcpy(pBuffer, pState->strHost, iBufSize);
        return(0);
    }
    
    //return hResult containing either the socket error or ssl error or the http status code 
    if (iSelect == 'hres')
    {
        if (pState->iHdrCode > 0)
        {
            return(DirtyErrGetHResult(DIRTYAPI_PROTO_HTTP, pState->iHdrCode, (pState->iHdrCode >= PROTOHTTP_RESPONSE_CLIENTERROR) ? TRUE : FALSE));
        }
        else
        {
            return(pState->iHresult);
        }
    }

    // return size of input buffer
    if (iSelect == 'imax')
    {
        return(pState->iInpMax);
    }

    // return input overflow amount (valid after PROTOHTTP_MINBUFF result code)
    if (iSelect == 'iovr')
    {
        return(pState->iInpOvr);
    }

    // return current port
    if (iSelect == 'port')
    {
        return(pState->iPort);
    }

    // return current redirection max
    if (iSelect == 'rmax')
    {
        return(pState->iMaxRedirect);
    }
    
    // done check: negative=failed, zero=pending, positive=done
    if (iSelect == 'done')
    {
        if (pState->eState == ST_FAIL)
            return(-1);
        if (pState->eState == ST_DONE)
            return(1);
        return(0);
    }

    // data check: negative=failed, zero=pending, positive=data ready
    if (iSelect == 'data')
    {
        if (pState->eState == ST_FAIL)
            return(-1);
        if ((pState->eState == ST_BODY) || (pState->eState == ST_DONE))
            return(pState->iInpLen);
        return(0);
    }

    // return response code
    if (iSelect == 'code')
        return(pState->iHdrCode);

    // return timeout indicator
    if (iSelect == 'time')
        return(pState->bTimeout);

    // copies info header (if available) to output buffer
    if (iSelect == 'info')
    {
        if (pState->bInfoHdr)
        {
            if (pBuffer != NULL)
            {
                ds_strnzcpy(pBuffer, pState->strHdr, iBufSize);
            }
            pState->bInfoHdr = FALSE;
            return(pState->iHdrCode);
        }
        return(0);
    }

    // check the state
    if (pState->eState == ST_FAIL)
        return(-1);
    if ((pState->eState != ST_BODY) && (pState->eState != ST_DONE))
        return(-2);

    // parse the tokens
    if (iSelect == 'head')
    {
        return(pState->iHeadSize);
    }
    if (iSelect == 'body')
    {
        if ((pBuffer != NULL) && (iBufSize == sizeof(pState->iBodySize)))
        {
            ds_memcpy(pBuffer, &pState->iBodySize, iBufSize);
        }
        return((int32_t)pState->iBodySize);
    }
    if (iSelect == 'date')
    {
        return(pState->iHdrDate);
    }
    if (iSelect == 'htxt')
    {
        ds_strnzcpy(pBuffer, pState->strHdr, iBufSize);
        return(0);
    }
    if (iSelect == 'rtxt')
    {
        ds_strnzcpy((char *)pBuffer, pState->strRequestHdr, iBufSize);
        return(0);
    }
    // invalid token
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpCheckKeepAlive

    \Description
        Checks whether a request for the given Url would be a keep-alive transaction.

    \Input *pState      - reference pointer
    \Input *pUrl        - Url to check

    \Output
        int32_t         - TRUE if a request to this Url would use keep-alive, else FALSE

    \Version 05/12/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpCheckKeepAlive(ProtoHttpRefT *pState, const char *pUrl)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;

    // parse the url
    pUrl = ProtoHttpUrlParse2(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // refresh open status
    if (pState->bConnOpen)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpps4: [0x%08x] check for keep-alive detected connection close\n", (uintptr_t)pState));
        pState->bConnOpen = FALSE;
    }

    // see if a request for this url would use keep-alive
    if (pState->bConnOpen && (pState->iKeepAlive > 0) && (pState->iPort == iPort) && (pState->iSecure == iSecure) && !ds_stricmp(pState->strHost, strHost))
    {
        return(1);
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUpdate

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pState      - reference pointer

    \Output
        None.

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
void ProtoHttpUpdate(ProtoHttpRefT *pState)
{
    int32_t iResult;
    SceHttpNBEvent nbEvent;
    memset(&nbEvent, 0x00, sizeof(nbEvent));
    
    // acquire sole access to http crit
    NetCritEnter(&pState->HttpCrit);
    
    // poll the sce epoll handle
    if (pState->eState != ST_IDLE && pState->eState != ST_DONE && pState->eState != ST_FAIL)
    {
        if (pState->iSceRequestId > 0)
        {
            iResult = sceHttpWaitRequest(pState->EpollHandle, &nbEvent, 1, 0);
            if ( iResult > 0 && nbEvent.id == pState->iSceRequestId ) 
            {
                if ( nbEvent.events & (SCE_HTTP_NB_EVENT_SOCK_ERR | SCE_HTTP_NB_EVENT_HUP | SCE_HTTP_NB_EVENT_RESOLVER_ERR) ) 
                {
                    NetPrintf(("protohttpps4: [0x%08x] sceHttpWaitRequest failed (err=%d)\n", (uintptr_t)pState, iResult));
                    pState->eState = ST_FAIL;
                    pState->iSslFail = 0;
                    pState->uSslFailDet = 0;
                    pState->iHresult = 0;
                    sceHttpsGetSslError(pState->iSceRequestId, &pState->iSslFail, &pState->uSslFailDet);
                    sceHttpGetLastErrno(pState->iSceRequestId, &pState->iHresult);
                    NetPrintf(("protohttpps4: [0x%08x] sceHttpWaitRequest failed with code 0x%08x for request id %d\n", (uintptr_t)pState, nbEvent.events, pState->iSceRequestId));
                }
            }
        }
    }

    // send buffered header+data to webserver
    if (pState->eState == ST_SEND)
    {
        if (pState->bChunked)
        {
            // if we're sending chunked data, we have no data to send in the inital pass, jump into the ST_RESP state to allow ProtoHttpSend to start pushing data
            pState->eState = ST_RESP;
        }
        else if (((iResult = _ProtoHttpSendBuff(pState)) > 0) && (pState->iInpLen == 0))
        {
            pState->iInpOff = 0;
            pState->iHdrOff = 0;
            pState->eState = ST_RESP;
        }
    }

    // wait for initial response
    if (pState->eState == ST_RESP)
    {
        int32_t iStatusCode = 0;

        // try to send any remaining buffered data if there is any
        if (pState->iInpLen > 0 || pState->bRecvEndChunk)
        {
            _ProtoHttpSendBuff(pState);
        }

        // check if we are done sending buffered data
        iResult = sceHttpGetStatusCode(pState->iSceRequestId, &iStatusCode);
        if (iResult == SCE_HTTP_ERROR_BEFORE_SEND)
        {
            // not done sending, noop
        }
        else if (iResult < 0)
        {
            pState->eState = ST_FAIL;
            pState->iSslFail = 0;
            pState->uSslFailDet = 0;
            pState->iHresult = 0;
            sceHttpsGetSslError(pState->iSceRequestId, &pState->iSslFail, &pState->uSslFailDet);
            sceHttpGetLastErrno(pState->iSceRequestId, &pState->iHresult);
            NetPrintf(("protohttpps4: [0x%08x] sceHttpGetStatusCode failed with code 0x%08x for request id %d\n", (uintptr_t)pState, iResult, pState->iSceRequestId));
        }
        // regardless of whether the header code is a success or failure
        else
        {
            NetPrintf(("protohttpps4: [0x%08x] sceHttpGetStatusCode returned http code %d for request id %d\n", (uintptr_t)pState, iStatusCode, pState->iSceRequestId));
            pState->eState = ST_HEAD;
        }
    }

    // try to receive header data
    if (pState->eState == ST_HEAD)
    {
        int32_t contentLengthType = 0;
        if ((iResult = sceHttpGetAllResponseHeaders(pState->iSceRequestId, &(pState->pSceRespHeader), &(pState->iSceRespHeaderLength))) < 0)
        {
            pState->eState = ST_FAIL;
            pState->iSslFail = 0;
            pState->uSslFailDet = 0;
            pState->iHresult = 0;
            sceHttpsGetSslError(pState->iSceRequestId, &pState->iSslFail, &pState->uSslFailDet);
            sceHttpGetLastErrno(pState->iSceRequestId, &pState->iHresult);
            NetPrintf(("protohttpps4: [0x%08x] sceHttpGetAllResponseHeaders failed with code 0x%08x for request id %d\n", (uintptr_t)pState, iResult, pState->iSceRequestId));
        }
        
        pState->iBodySize = 0ll;
        if ((iResult = sceHttpGetResponseContentLength(pState->iSceRequestId, &contentLengthType, (uint64_t*)(&(pState->iBodySize)))) < 0)
        {
            pState->eState = ST_FAIL;
            pState->iSslFail = 0;
            pState->uSslFailDet = 0;
            pState->iHresult = 0;
            sceHttpsGetSslError(pState->iSceRequestId, &pState->iSslFail, &pState->uSslFailDet);
            sceHttpGetLastErrno(pState->iSceRequestId, &pState->iHresult);
            NetPrintf(("protohttpps4: [0x%08x] sceHttpGetResponseContentLength failed with code 0x%08x for request id %d\n", (uintptr_t)pState, iResult, pState->iSceRequestId));
        }
        if (contentLengthType != SCE_HTTP_CONTENTLEN_EXIST)
        {
            pState->iBodySize = -1ll;
        }
    }

    // check for header completion, process it
    if ((pState->eState == ST_HEAD) && (pState->iSceRespHeaderLength > 4))
    {
        _ProtoHttpHeaderProcess(pState);
    }

    // parse incoming body data
    while ((pState->eState == ST_BODY) && (_ProtoHttpRecvBody(pState) > 0))
        ;

    // write callback processing
    if (pState->pWriteCb != NULL)
    {
        _ProtoHttpWriteCbProcess(pState);
    }

    // force disconnect in failure state
    if (pState->eState == ST_FAIL)
    {
        _ProtoHttpClose(pState, "error");
    }

    // handle completion
    if (pState->eState == ST_DONE)
    {
        if (pState->bCloseHdr)
        {
            _ProtoHttpClose(pState, "server request");
        }
    }

    // release access to httpcrit
    NetCritLeave(&pState->HttpCrit);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetCACert

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all HTTP instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

    \Input *pCACert - pointer to certificate data
    \Input iCertSize- certificate size in bytes

    \Output
        int32_t     - negative=failure, zero=success

    \Version 01/13/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpSetCACert(const uint8_t *pCACert, int32_t iCertSize)
{
    return(ProtoHttpSetCACert2(pCACert, iCertSize));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpSetCACert2

    \Description
        Add one or more X.509 CA certificates to the trusted list. A
        certificate added will be available to all HTTP instances for
        the lifetime of the application. This function can add one or more
        PEM certificates or a single DER certificate.

        This version of the function does not validate the CA at load time.
        The X509 certificate data will be copied and retained until the CA
        is validated, either by use of ProtoHttpValidateAllCA() or by the CA
        being used to validate a certificate.

    \Input *pCACert - pointer to certificate data
    \Input iCertSize- certificate size in bytes

    \Output
        int32_t     - negative=failure, zero=success

    \Version 04/21/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpSetCACert2(const uint8_t *pCACert, int32_t iCertSize)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    ProtoHttpCACertT *pCert = _pProtoHttpPs4_CACerts;
    ProtoHttpCACertT *pNewCert = NULL;

    // get memgroup settings for certificate blob
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // check for duplicates
    pCert = _pProtoHttpPs4_CACerts;
    while (pCert != NULL && pCert->pNext != NULL)
    {
        if (pCert->iCertSize == iCertSize)
        {
            if (memcmp(pCACert, pCert->pCACert, iCertSize) == 0)
            {
                NetPrintf(("protohttpps4: ignoring redundant add of CA cert\n"));
                return(0);
            }
        }
        pCert = pCert->pNext;
    }
 
    // create memory
    pNewCert = (ProtoHttpCACertT*)DirtyMemAlloc(sizeof(ProtoHttpCACertT), PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData);
    if (pNewCert == NULL)
    {
        NetPrintf(("protohttpps4: failed to allocate memory for CA cert\n"));
    }
    pNewCert->pCACert = (uint8_t*)DirtyMemAlloc(iCertSize, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData);
    if (pNewCert->pCACert == NULL)
    {
        NetPrintf(("protohttpps4: failed to allocate memory for CA cert\n"));
    }
    memset(pNewCert->pCACert, 0, iCertSize);
    ds_memcpy_s(pNewCert->pCACert, iCertSize, pCACert, iCertSize);
    pNewCert->iCertSize = iCertSize;
    pNewCert->pNext = NULL;

    // find the end of the list
    if (_pProtoHttpPs4_CACerts == NULL)
    {
        _pProtoHttpPs4_CACerts = pNewCert;
    }
    else
    {
        pCert = _pProtoHttpPs4_CACerts;
        while (pCert->pNext != NULL)
        {
            pCert = pCert->pNext;
        }
        pCert->pNext = pNewCert;
    }

    // increment the version number
    ++uCaListVersion;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpValidateAllCA

    \Description
        Validate all CA that have been added but not yet been validated.  Validation
        is a one-time process and disposes of the X509 certificate that is retained
        until validation.

    \Output
        int32_t     - negative=failure, zero=success

    \Version 04/21/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpValidateAllCA(void)
{
    // all certs are validated when added
    return(0);
}

/*F*************************************************************************/
/*!
    \Function ProtoHttpClrCACerts

    \Description
        Clears all dynamic CA certs from the list.

    \Version 01/14/2009 (jbrookes)
*/
/**************************************************************************F*/
void ProtoHttpClrCACerts(void)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    ProtoHttpCACertT *pCert = _pProtoHttpPs4_CACerts;
  
    // get memgroup settings for certificate blob
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
  
    while (pCert != NULL)
    {
        ProtoHttpCACertT *pNext = pCert->pNext;
        if (pCert->pCACert)
        {
            DirtyMemFree(pCert->pCACert, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData);
        }
        DirtyMemFree(pCert, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData);
  
        pCert = pNext;
    }
    _pProtoHttpPs4_CACerts = NULL;

    // increment the version number
    ++uCaListVersion;
}
