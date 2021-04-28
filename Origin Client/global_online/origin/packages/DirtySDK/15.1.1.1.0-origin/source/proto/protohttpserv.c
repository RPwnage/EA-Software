/*H********************************************************************************/
/*!
    \File protohttpserv.c

    \Description
        This module implements an HTTP server that can perform basic transactions
        (get/put) with an HTTP client.  It conforms to but does not fully implement
        the 1.1 HTTP spec (http://www.w3.org/Protocols/rfc2616/rfc2616.html), and
        allows for secure HTTP transactions as well as insecure transactions.

    \Copyright
        Copyright (c) Electronic Arts 2013

    \Version 1.0 12/11/2013 (jbrookes) Initial version, based on HttpServ tester2 module
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __linux__
#include <sys/time.h> // gettimeofday
#endif

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h" // NetConnSleep
#include "DirtySDK/proto/protohttputil.h"
#include "DirtySDK/proto/protossl.h"

#include "DirtySDK/proto/protohttpserv.h"

/*** Defines **********************************************************************/

#define HTTPSERV_VERSION                (0x0100)            //!< httpserv revision number (maj.min); update this for major bug fixes or protocol additions/changes
#define HTTPSERV_DISCTIME               (5*1000)            //!< give five seconds after we disconnect before destroying network resources
#define HTTPSERV_CHUNKWAITTEST          (0)                 //!< this tests a corner case in protohttp chunk receive where only one byte of the chunk trailer is available
#define HTTPSERV_IDLETIMEOUT_DEFAULT    (5*1000*1000)       //!< default keep-alive timeout
#define HTTPSERV_BUFSIZE_DEFAULT        (16000)             //!< the most user payload we can send in a single SSL packet

/*** Function Prototypes **********************************************************/

/*** Type Definitions *************************************************************/

//! httpserv response table
typedef struct ProtoHttpServRespT
{
    ProtoHttpResponseE eResponse;
    const char *pResponseText;
}ProtoHttpServRespT;

//! httpserv transaction state for a single transaction
typedef struct ProtoHttpServThreadT
{
    struct ProtoHttpServThreadT *pNext;
    ProtoSSLRefT *pProtoSSL;

    ProtoHttpRequestTypeE eRequestType;
    struct sockaddr RequestAddr;
    int32_t iResponseCode;
    uint32_t uModifiedSince;
    uint32_t uUnmodifiedSince;

    ProtoHttpServRequestInfoT RequestInfo;  //!< user data associated with this thread via request callback

    char *pBuffer;
    int32_t iBufMax;
    int32_t iBufLen;
    int32_t iBufOff;
    int32_t iChkLen;
    int32_t iChkRcv;
    int32_t iChkOvr;        //!< chunk overhead
    int32_t iIdleTimer;
    int32_t iDisconnectTimer;

    int64_t iContentLength;
    int64_t iContentRead;
    int64_t iContentSent;
    int64_t iContentRecv;

    char strUrl[1*1024];
    char strLocation[1*1024];
    char strUserAgent[256];
    char strHeader[16*1024];

    uint8_t bConnected;
    uint8_t bReceivedHeader;
    uint8_t bParsedHeader;
    uint8_t bProcessedRequest;
    uint8_t bReceivedBody;
    uint8_t bFormattedHeader;
    uint8_t bSentHeader;
    uint8_t bSentBody;
    uint8_t bConnectionClose;
    uint8_t bConnectionKeepAlive;
    uint8_t bDisconnecting;
    uint8_t uHttpThreadId;
    uint8_t bHttp1_0;
    uint8_t bChunked;
    uint8_t _pad[2];
} ProtoHttpServThreadT;

//! httpserv module state
struct ProtoHttpServRefT
{
    ProtoSSLRefT *pListenSSL;

    int32_t iMemGroup;
    void *pMemGroupUserData;

    ProtoHttpServRequestCbT *pRequestCb;
    ProtoHttpServReadCbT *pReadCb;
    ProtoHttpServWriteCbT *pWriteCb;
    ProtoHttpServLogCbT *pLogCb;
    void *pUserData;

    char *pServerCert;
    int32_t iServerCertLen;
    char *pServerKey;
    int32_t iServerKeyLen;
    int32_t iSecure;
    uint16_t uSslVersion;
    uint16_t uSslVersionMin;
    uint32_t uSslCiphers;
    int32_t iClientCertLvl;
    int32_t iIdleTimeout;

    char strServerName[128];
    char strFileDir[512];

    ProtoHttpServThreadT *pThreadList;
    uint8_t uCurrThreadId;
};

/*** Variables ********************************************************************/

//! http request names
static const char *_ProtoHttpServ_strRequestNames[PROTOHTTP_NUMREQUESTTYPES] =
{
    "HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS"
};

//! http response name table
static const ProtoHttpServRespT _ProtoHttpServ_Responses[] =
{
    // 1xx - informational reponse
    { PROTOHTTP_RESPONSE_CONTINUE,              "Continue" },                       //!< continue with request, generally ignored
    { PROTOHTTP_RESPONSE_SWITCHPROTO,           "Switching Protocols" },            //!< 101 - OK response to client switch protocol request
    // 2xx - success response
    { PROTOHTTP_RESPONSE_OK,                    "OK" },                             //!< client's request was successfully received, understood, and accepted
    { PROTOHTTP_RESPONSE_CREATED,               "Created" } ,                       //!< new resource has been created
    { PROTOHTTP_RESPONSE_ACCEPTED,              "Accepted" } ,                      //!< request accepted but not complete
    { PROTOHTTP_RESPONSE_NONAUTH,               "Non-Authoritative Information" },  //!< non-authoritative info (ok)
    { PROTOHTTP_RESPONSE_NOCONTENT,             "No Content" } ,                    //!< request fulfilled, no message body
    { PROTOHTTP_RESPONSE_RESETCONTENT,          "Reset Content" } ,                 //!< request success, reset document view
    { PROTOHTTP_RESPONSE_PARTIALCONTENT,        "Partial Content" } ,               //!< server has fulfilled partial GET request
    // 3xx - redirection response
    { PROTOHTTP_RESPONSE_MULTIPLECHOICES,       "Multiple Choices" },               //!< requested resource corresponds to a set of representations
    { PROTOHTTP_RESPONSE_MOVEDPERMANENTLY,      "Moved Permanently" },              //!< requested resource has been moved permanently to new URI
    { PROTOHTTP_RESPONSE_FOUND,                 "Found" },                          //!< requested resources has been moved temporarily to a new URI
    { PROTOHTTP_RESPONSE_SEEOTHER,              "See Other" },                      //!< response can be found under a different URI
    { PROTOHTTP_RESPONSE_NOTMODIFIED,           "Not Modified" },                   //!< response to conditional get when document has not changed
    { PROTOHTTP_RESPONSE_USEPROXY,              "Use Proxy" },                      //!< requested resource must be accessed through proxy
    { PROTOHTTP_RESPONSE_TEMPREDIRECT,          "Temporary Redirect" },             //!< requested resource resides temporarily under a different URI
    // 4xx - client error response
    { PROTOHTTP_RESPONSE_BADREQUEST,            "Bad Request" },                    //!< request could not be understood by server due to malformed syntax
    { PROTOHTTP_RESPONSE_UNAUTHORIZED,          "Unauthorized" },                   //!< request requires user authorization
    { PROTOHTTP_RESPONSE_PAYMENTREQUIRED,       "Payment Required" },               //!< reserved for future user
    { PROTOHTTP_RESPONSE_FORBIDDEN,             "Forbidden" },                      //!< request understood, but server will not fulfill it
    { PROTOHTTP_RESPONSE_NOTFOUND,              "Not Found" },                      //!< Request-URI not found
    { PROTOHTTP_RESPONSE_METHODNOTALLOWED,      "Method Not Allowed" },             //!< method specified in the Request-Line is not allowed
    { PROTOHTTP_RESPONSE_NOTACCEPTABLE,         "Not Acceptable" },                 //!< resource incapable of generating content acceptable according to accept headers in request
    { PROTOHTTP_RESPONSE_PROXYAUTHREQ,          "Proxy Authentication Required" },  //!< client must first authenticate with proxy
    { PROTOHTTP_RESPONSE_REQUESTTIMEOUT,        "Request Timeout" },                //!< client did not produce response within server timeout
    { PROTOHTTP_RESPONSE_CONFLICT,              "Conflict" },                       //!< request could not be completed due to a conflict with current state of the resource
    { PROTOHTTP_RESPONSE_GONE,                  "Gone" },                           //!< requested resource is no longer available and no forwarding address is known
    { PROTOHTTP_RESPONSE_LENGTHREQUIRED,        "Length Required" },                //!< a Content-Length header was not specified and is required
    { PROTOHTTP_RESPONSE_PRECONFAILED,          "Precondition Failed" },            //!< precondition given in request-header field(s) failed
    { PROTOHTTP_RESPONSE_REQENTITYTOOLARGE,     "Request Entity Too Large" },       //!< request entity is larger than the server is able or willing to process
    { PROTOHTTP_RESPONSE_REQURITOOLONG,         "Request-URI Too Long" },           //!< Request-URI is longer than the server is willing to interpret
    { PROTOHTTP_RESPONSE_UNSUPPORTEDMEDIA,      "Unsupported Media Type" },         //!< request entity is in unsupported format
    { PROTOHTTP_RESPONSE_REQUESTRANGE,          "Requested Range Not Satisfiable" },//!< invalid range in Range request header
    { PROTOHTTP_RESPONSE_EXPECTATIONFAILED,     "Expectation Failed" },             //!< expectation in Expect request-header field could not be met by server
    // 5xx - server error response
    { PROTOHTTP_RESPONSE_INTERNALSERVERERROR,   "Internal Server Error" },          //!< an unexpected condition prevented the server from fulfilling the request
    { PROTOHTTP_RESPONSE_NOTIMPLEMENTED,        "Not Implemented" },                //!< the server does not support the functionality required to fulfill the request
    { PROTOHTTP_RESPONSE_BADGATEWAY,            "Bad Gateway" },                    //!< invalid response from gateway server
    { PROTOHTTP_RESPONSE_SERVICEUNAVAILABLE,    "Service Unavailable" },            //!< unable to handle request due to a temporary overloading or maintainance
    { PROTOHTTP_RESPONSE_GATEWAYTIMEOUT,        "Gateway Timeout" },                //!< gateway or DNS server timeout
    { PROTOHTTP_RESPONSE_HTTPVERSUNSUPPORTED,   "HTTP Version Not Supported" },     //!< the server does not support the HTTP protocol version that was used in the request
    { PROTOHTTP_RESPONSE_PENDING,               "Unknown" },                        //!< unknown response code
};

//! map of filename extensions to content-types
static const char *_ProtoHttpServ_strContentTypes[][2] =
{
    { ".htm",  "text/html"  },
    { ".html", "text/html"  },
    { ".css",  "text/css"   },
    { ".xml",  "text/xml"   },
    { ".jpg",  "image/jpeg" },
    { ".gif",  "image/gif"  },
    { ".png",  "image/png"  },
    { ".mp3",  "audio/mpeg" }
};


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoHttpServGetCurrentTime

    \Description
       Gets and parses the current time into components

    \Input *uYear   - pointer to uint32_t var to store 'year' in
    \Input *uMonth  - pointer to uint8_t var to store 'month' in
    \Input *uDay    - pointer to uint8_t var to store 'day' in
    \Input *uHour   - pointer to uint8_t var to store 'hour' in
    \Input *uMin    - pointer to uint8_t var to store 'min' in
    \Input *uSec    - pointer to uint8_t var to store 'sec' in
    \Input *uMillis - pointer to uint32_t var to store 'milliseconds' in

    \Version 10/30/2013 (jbrookes) Borrowed from eafn logger
*/
/********************************************************************************F*/
static void _ProtoHttpServGetCurrentTime(uint32_t *uYear, uint8_t *uMonth, uint8_t *uDay, uint8_t *uHour, uint8_t *uMin, uint8_t *uSec, uint32_t *uMillis)
{
#if DIRTYCODE_PC
    SYSTEMTIME SystemTime;
    GetLocalTime(&SystemTime);
    *uYear = SystemTime.wYear;
    *uMonth = SystemTime.wMonth;
    *uDay = SystemTime.wDay;
    *uHour = SystemTime.wHour;
    *uMin = SystemTime.wMinute;
    *uSec = SystemTime.wSecond;
    *uMillis = SystemTime.wMilliseconds;
#else // all non-pc
#if DIRTYCODE_LINUX
    struct timeval tv;
    struct tm *pTime;
    gettimeofday(&tv, NULL);
    pTime = gmtime((time_t *)&tv.tv_sec);
    *uMillis = tv.tv_usec / 1000;
#else
    //$$TODO: plat-time doesn't have anything to get millis
    struct tm TmTime, *pTime;
    pTime = ds_secstotime(&TmTime, ds_timeinsecs());
    *uMillis = 0;
#endif
    *uYear = 1900 + pTime->tm_year;
    *uMonth = pTime->tm_mon + 1;
    *uDay = pTime->tm_mday;
    *uHour = pTime->tm_hour;
    *uMin = pTime->tm_min;
    *uSec = pTime->tm_sec;
#endif // !DIRTYCODE_PC
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServLogPrintf

    \Description
        Log printing for HttpServ server (compiles in all builds, unlike debug output).

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread log entry is for (may be NULL)
    \Input *pFormat     - printf format string
    \Input ...          - variable argument list

    \Version 08/03/2007 (jbrookes) Borrowed from DirtyCast
*/
/********************************************************************************F*/
static void _ProtoHttpServLogPrintf(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, const char *pFormat, ...)
{
    uint32_t uYear, uMillis;
    uint8_t uMonth, uDay, uHour, uMin, uSec;
    char strText[2048];
    int32_t iOffset;
    va_list Args;

    // format prefix to output buffer
    _ProtoHttpServGetCurrentTime(&uYear, &uMonth, &uDay, &uHour, &uMin, &uSec, &uMillis);
    if (pHttpThread != NULL)
    {
        iOffset = ds_snzprintf(strText, sizeof(strText), "%02u/%02u/%02u %02u:%02u:%02u.%03u | httpserv: [%08d] [%16a] ",
            uYear, uMonth, uDay, uHour, uMin, uSec, uMillis, pHttpThread->uHttpThreadId, SockaddrInGetAddr(&pHttpThread->RequestAddr));
    }
    else
    {
        iOffset = ds_snzprintf(strText, sizeof(strText), "%02u/%02u/%02u %02u:%02u:%02u.%03u | httpserv: ",
            uYear, uMonth, uDay, uHour, uMin, uSec, uMillis, strText);
    }

    // format output
    va_start(Args, pFormat);
    ds_vsnprintf(strText+iOffset, sizeof(strText)-iOffset, pFormat, Args);
    va_end(Args);

    // forward to callback, or print if no callback installed
    if (pHttpServ->pLogCb != NULL)
    {
        pHttpServ->pLogCb(strText, pHttpServ->pUserData);
    }
    else
    {
        NetPrintf(("%s", strText));
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServGetResponseText

    \Description
        Return response text for specified response code

    \Input eResponseCode    - code to get text for

    \Output
        const char *        - response text

    \Version 09/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoHttpServGetResponseText(ProtoHttpResponseE eResponseCode)
{
    int32_t iResponse;
    for (iResponse = 0; _ProtoHttpServ_Responses[iResponse].eResponse != PROTOHTTP_RESPONSE_PENDING; iResponse += 1)
    {
        if (_ProtoHttpServ_Responses[iResponse].eResponse == eResponseCode)
        {
            break;
        }
    }
    return(_ProtoHttpServ_Responses[iResponse].pResponseText);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServSSLCreate

    \Description
        Create ProtoSSL listen object

    \Input *pHttpServ   - module state
    \Input uPort        - port to bind
    \Input bReuseAddr   - TRUE to set SO_REUSEADDR, else FALSE

    \Output
        ProtoSSLRefT *  - socket ref, or NULL

    \Version 10/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static ProtoSSLRefT *_ProtoHttpServSSLCreate(ProtoHttpServRefT *pHttpServ, uint16_t uPort, uint8_t bReuseAddr)
{
    struct sockaddr BindAddr;
    ProtoSSLRefT *pProtoSSL;
    int32_t iResult;

    // create the protossl ref
    if ((pProtoSSL = ProtoSSLCreate()) == NULL)
    {
        return(NULL);
    }

    // enable reuseaddr
    if (bReuseAddr)
    {
        ProtoSSLControl(pProtoSSL, 'radr', 1, 0, NULL);
    }

    // bind ssl to specified port
    SockaddrInit(&BindAddr, AF_INET);
    SockaddrInSetPort(&BindAddr, uPort);
    if ((iResult = ProtoSSLBind(pProtoSSL, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
    {
        _ProtoHttpServLogPrintf(pHttpServ, NULL, "error %d binding to port\n", iResult);
        ProtoSSLDestroy(pProtoSSL);
        return(NULL);
    }

    // return ref to caller
    return(pProtoSSL);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServThreadAlloc

    \Description
        Allocate and initialize an HttpThread object

    \Input *pHttpServ       - module state
    \Input *pProtoSSL       - ProtoSSL ref for this thread
    \Input *pRequestAddr    - address connection request was made from

    \Output
        ProtoHttpServThreadT *  - newly allocated HttpThread object, or NULL on failure

    \Version 03/21/2014 (jbrookes)
*/
/********************************************************************************F*/
static ProtoHttpServThreadT *_ProtoHttpServThreadAlloc(ProtoHttpServRefT *pHttpServ, ProtoSSLRefT *pProtoSSL, struct sockaddr *pRequestAddr)
{
    ProtoHttpServThreadT *pHttpThread;
    // allocate and init thread memory
    if ((pHttpThread = DirtyMemAlloc(sizeof(*pHttpThread), HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "unable to alloc http thread\n");
        return(NULL);
    }
    memset(pHttpThread, 0, sizeof(*pHttpThread));
    // allocate http thread streaming buffer
    if ((pHttpThread->pBuffer = DirtyMemAlloc(HTTPSERV_BUFSIZE_DEFAULT, HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "unable to alloc http thread buffer\n");
        return(NULL);
    }
    // init thread members
    pHttpThread->pProtoSSL = pProtoSSL;
    pHttpThread->uHttpThreadId = pHttpServ->uCurrThreadId++;
    ds_memcpy_s(&pHttpThread->RequestAddr, sizeof(pHttpThread->RequestAddr), pRequestAddr, sizeof(*pRequestAddr));
    pHttpThread->iBufMax = HTTPSERV_BUFSIZE_DEFAULT;
    // return ref to caller
    return(pHttpThread);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServThreadFree

    \Description
        Free an HttpThread object

    \Input *pHttpServ       - module state
    \Input *pHttpThread     - HttpThread object to free

    \Version 03/21/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServThreadFree(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    NetPrintf(("protohttpserv: destroying http thread %d\n", pHttpThread->uHttpThreadId));
    if (pHttpThread->pProtoSSL != NULL)
    {
        ProtoSSLDestroy(pHttpThread->pProtoSSL);
    }
    if (pHttpThread->pBuffer != NULL)
    {
        DirtyMemFree(pHttpThread->pBuffer, HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData);
    }
    DirtyMemFree(pHttpThread, HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServThreadReallocBuf

    \Description
        Reallocate buffer for HttpThread object

    \Input *pHttpServ       - module state
    \Input *pHttpThread     - HttpThread object to realloc buffer for
    \Input iBufSize         - size to reallocate to

    \Version 03/21/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServThreadReallocBuf(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, int32_t iBufSize)
{
    char *pBuffer;
    if ((pBuffer = DirtyMemAlloc(iBufSize, HTTPMGR_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData)) == NULL)
    {
        return(-1);
    }
    ds_memcpy(pBuffer, pHttpThread->pBuffer+pHttpThread->iBufOff, pHttpThread->iBufLen-pHttpThread->iBufOff);
    DirtyMemFree(pHttpThread->pBuffer, HTTPMGR_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData);
    pHttpThread->pBuffer = pBuffer;
    pHttpThread->iBufLen -= pHttpThread->iBufOff;
    pHttpThread->iBufOff = 0;
    pHttpThread->iBufMax = iBufSize;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServDisconnect

    \Description
        Disconnect from a client

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)
    \Input bGraceful    - TRUE=graceful disconnect, FALSE=hard close

    \Version 10/11/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServDisconnect(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, uint8_t bGraceful)
{
    if (bGraceful)
    {
        ProtoSSLDisconnect(pHttpThread->pProtoSSL);
        pHttpThread->iDisconnectTimer = NetTick();
    }
    else
    {
        pHttpThread->bConnected = FALSE;
        pHttpThread->iDisconnectTimer = NetTick() - (HTTPSERV_DISCTIME+1);
    }
    pHttpThread->bDisconnecting = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServResetState

    \Description
        Reset state after a transaction is completed

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Version 10/26/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServResetState(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    memset(&pHttpThread->RequestInfo, 0, sizeof(pHttpThread->RequestInfo));
    pHttpThread->bReceivedHeader = FALSE;
    pHttpThread->bSentBody = FALSE;
    pHttpThread->iContentLength = 0;
    pHttpThread->iContentRead = 0;
    pHttpThread->iContentSent = 0;
    pHttpThread->iContentRecv = 0;
    pHttpThread->iBufLen = 0;
    pHttpThread->iBufOff = 0;
    pHttpThread->iChkLen = 0;
    pHttpThread->iChkOvr = 0;
    pHttpThread->uModifiedSince = 0;
    pHttpThread->uUnmodifiedSince = 0;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServParseHeader

    \Description
        Parse a received header

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServParseHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    const char *pHdr, *pEnd;
    char strName[128], strValue[4*1024];
    int32_t iResult, iType;

    // process request type
    for (iType = 0; iType < PROTOHTTP_NUMREQUESTTYPES; iType += 1)
    {
        if (!ds_strnicmp(_ProtoHttpServ_strRequestNames[iType], pHttpThread->strHeader, (int32_t)strlen(_ProtoHttpServ_strRequestNames[iType])))
        {
            NetPrintf(("protohttpserv: [%02x] processing %s request\n", pHttpThread->uHttpThreadId, _ProtoHttpServ_strRequestNames[iType]));
            pHttpThread->eRequestType = (ProtoHttpRequestTypeE)iType;
            break;
        }
    }
    if (iType == PROTOHTTP_NUMREQUESTTYPES)
    {
        // not a valid request type
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "invalid request type\n");
        pHttpThread->iResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-1);
    }

    // find url
    if ((pHdr = strchr(pHttpThread->strHeader, '/')) == NULL)
    {
        // not a valid header
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "could not find url\n");
        pHttpThread->iResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-2);
    }
    // find end of url
    if ((pEnd = strchr(pHdr, ' ')) == NULL)
    {
        // not a valid header
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "could not find url end\n");
        pHttpThread->iResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-3);
    }
    ds_strsubzcpy(pHttpThread->strUrl, sizeof(pHttpThread->strUrl), pHdr, (int32_t)(pEnd-pHdr));
    NetPrintf(("protohttpserv: [%02x] url=%s\n", pHttpThread->uHttpThreadId, pHttpThread->strUrl));

    // detect if it is a 1.0 response; if so we default to closing the connection
    if (!ds_strnicmp(pEnd+1, "HTTP/1.0", 8))
    {
        pHttpThread->bHttp1_0 = TRUE;
        pHttpThread->bConnectionClose = TRUE;
    }

    // skip to first header
    if ((pHdr = strstr(pEnd, "\r\n")) == NULL)
    {
        // not a valid header
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "could not find headers\n");
        pHttpThread->iResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-4);
    }

    // parse header
    for (iResult = 0; iResult >= 0; )
    {
        // get next header name and value from header buffer
        if ((iResult = ProtoHttpGetNextHeader(NULL, pHdr, strName, sizeof(strName), strValue, sizeof(strValue), &pEnd)) == 0)
        {
            // process header
            if (!ds_stricmp(strName, "connection"))
            {
                if (!ds_stricmp(strValue, "close"))
                {
                    pHttpThread->bConnectionClose = TRUE;
                }
                else if (!ds_stricmp(strValue, "keep-alive"))
                {
                    pHttpThread->bConnectionKeepAlive = TRUE;
                }
            }
            else if (!ds_stricmp(strName, "content-length") && !pHttpThread->bChunked)
            {
                pHttpThread->iContentLength = ds_strtoll(strValue, NULL, 10);
            }
            else if (!ds_stricmp(strName, "if-modified-since"))
            {
                pHttpThread->uModifiedSince = (uint32_t)ds_strtotime(strValue);
            }
            else if (!ds_stricmp(strName, "if-unmodified-since"))
            {
                pHttpThread->uUnmodifiedSince = (uint32_t)ds_strtotime(strValue);
            }
            else if (!ds_stricmp(strName, "transfer-encoding"))
            {
                if ((pHttpThread->bChunked = !ds_stricmp(strValue, "chunked")) == TRUE)
                {
                    pHttpThread->iContentLength = -1;
                }
            }
            else if (!ds_stricmp(strName, "user-agent"))
            {
                ds_strnzcpy(pHttpThread->strUserAgent, strValue, sizeof(pHttpThread->strUserAgent));
            }
            else
            {
                NetPrintf(("protohttpserv: [%02x] unprocessed header '%s'='%s'\n", pHttpThread->uHttpThreadId, strName, strValue));
            }
        }

        // move to next header
        pHdr = pEnd;
    }

    _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "processing %s %s user-agent=%s\n", _ProtoHttpServ_strRequestNames[iType], pHttpThread->strUrl, pHttpThread->strUserAgent);

    pHttpThread->bParsedHeader = TRUE;
    pHttpThread->bProcessedRequest = FALSE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServGetContentType

    \Description
        Get content-type based on target url

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        const char *    - content type

    \Version 07/09/2013 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoHttpServGetContentType(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    const char *pContentType = _ProtoHttpServ_strContentTypes[0][1];
    int32_t iType;

    for (iType = 0; iType < (signed)(sizeof(_ProtoHttpServ_strContentTypes)/sizeof(_ProtoHttpServ_strContentTypes[0])); iType += 1)
    {
        if (ds_stristr(pHttpThread->strUrl, _ProtoHttpServ_strContentTypes[iType][0]))
        {
            pContentType = _ProtoHttpServ_strContentTypes[iType][1];
            break;
        }
    }

    NetPrintf(("protohttpserv: [%02x] setting content-type to %s\n", pHttpThread->uHttpThreadId, pContentType));
    return(pContentType);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServFormatHeader

    \Description
        Format response header

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServFormatHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    struct tm TmTime;
    char strTime[64];
    int32_t iBufLen, iBufMax = pHttpThread->iBufMax;

    // format a basic response header
    iBufLen = ds_snzprintf(pHttpThread->pBuffer, iBufMax, "HTTP/1.1 %d %s\r\n", pHttpThread->iResponseCode,
        _ProtoHttpServGetResponseText((ProtoHttpResponseE)pHttpThread->iResponseCode));

    // date header; specify current GMT time
    ds_secstotime(&TmTime, ds_timeinsecs());
    iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Date: %s\r\n",
        ds_timetostr(&TmTime, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTime, sizeof(strTime)));

    // redirection?
    if (pHttpThread->strLocation[0] != '\0')
    {
        iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Location: %s\r\n", pHttpThread->strLocation);
    }
    if (pHttpThread->iContentLength > 0)
    {
        const char *pContentType = _ProtoHttpServGetContentType(pHttpServ, pHttpThread);

        // set content type
        iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Content-type: %s\r\n", pContentType);

        // set last modified time
        ds_secstotime(&TmTime, pHttpThread->RequestInfo.uModTime);
        iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Last-Modified: %s\r\n",
            ds_timetostr(&TmTime, TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTime, sizeof(strTime)));

        // set content length or transfer-encoding
        if (pHttpThread->RequestInfo.iChunkLength == 0)
        {
            iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Content-length: %qd\r\n", pHttpThread->iContentLength);
        }
        else
        {
            iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Transfer-Encoding: Chunked\r\n");
        }
    }
    iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Server: %s/ProtoHttpServ %d.%d/DS %d.%d.%d.%d.%d (" DIRTYCODE_PLATNAME ")\r\n",
        pHttpServ->strServerName, (HTTPSERV_VERSION>>8)&0xff, HTTPSERV_VERSION&0xff, DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON,
        DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH);
    if (pHttpThread->bConnectionClose)
    {
        iBufLen += ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "Connection: Close\r\n");
    }

    // format footer, update thread data
    pHttpThread->iBufLen = ds_snzprintf(pHttpThread->pBuffer+iBufLen, iBufMax-iBufLen, "\r\n") + iBufLen;
    pHttpThread->iBufOff = 0;
    pHttpThread->bFormattedHeader = TRUE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServProcessGet

    \Description
        Process GET/HEAD request

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServProcessGet(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult;

    // default to no data
    pHttpThread->iContentLength = 0;

    // make user callback to handle GET/HEAD
    if ((iResult = pHttpServ->pRequestCb(pHttpThread->eRequestType, pHttpThread->strUrl, pHttpThread->strHeader, &pHttpThread->RequestInfo, pHttpServ->pUserData)) < 0)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "error %d processing request\n", iResult);
        pHttpThread->iResponseCode = pHttpThread->RequestInfo.iResponseCode;
        return;
    }

    // check modified since
    if ((pHttpThread->uModifiedSince != 0) && ((int32_t)(pHttpThread->RequestInfo.uModTime-pHttpThread->uModifiedSince) <= 0))
    {
        NetPrintf(("protohttpserv: [%02x] file not modified (%d-%d=%d)\n", pHttpThread->uHttpThreadId, pHttpThread->uModifiedSince,
            pHttpThread->RequestInfo.uModTime, (int32_t)(pHttpThread->RequestInfo.uModTime-pHttpThread->uModifiedSince)));
        pHttpThread->iResponseCode = PROTOHTTP_RESPONSE_NOTMODIFIED;
        return;
    }

    // check unmodified since
    if ((pHttpThread->uUnmodifiedSince != 0) && ((int32_t)(pHttpThread->RequestInfo.uModTime-pHttpThread->uUnmodifiedSince) > 0))
    {
        NetPrintf(("protohttpserv: [%02x] file modified since (%d-%d=%d)\n", pHttpThread->uHttpThreadId, pHttpThread->uUnmodifiedSince,
            pHttpThread->RequestInfo.uModTime, (int32_t)(pHttpThread->RequestInfo.uModTime-pHttpThread->uUnmodifiedSince)));
        pHttpThread->iResponseCode = PROTOHTTP_RESPONSE_PRECONFAILED;
        return;
    }

    // set content-length
    pHttpThread->iContentLength = pHttpThread->RequestInfo.iDataSize;
    pHttpThread->iResponseCode = (pHttpThread->RequestInfo.iResponseCode != 0) ? pHttpThread->RequestInfo.iResponseCode : PROTOHTTP_RESPONSE_OK;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServProcessPut

    \Description
        Process PUT/POST request

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServProcessPut(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult;

    // get data size for callback
    pHttpThread->RequestInfo.iDataSize = pHttpThread->iContentLength;

    // if no content length and not chunked, force no keep-alive
    if ((pHttpThread->iContentLength <= 0) && !pHttpThread->bChunked && !pHttpThread->bConnectionClose)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "put/post with no content-length, not chunked encoding, and keep-alive not supported; connection will be closed\n");
        pHttpThread->bConnectionClose = TRUE;
    }

    // make user callback
    if ((iResult = pHttpServ->pRequestCb(pHttpThread->eRequestType, pHttpThread->strUrl, pHttpThread->strHeader, &pHttpThread->RequestInfo, pHttpServ->pUserData)) < 0)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "error %d processing request\n", iResult);
        pHttpThread->iResponseCode = pHttpThread->RequestInfo.iResponseCode;
        return;
    }

    // we've processed the request
    pHttpThread->iContentRecv = 0;
    pHttpThread->bReceivedBody = 0;

    // always close connection after upload $$todo should not always close the connection
    pHttpThread->bConnectionClose = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateListen

    \Description
        Update HttpServ while listening for a new connection

    \Input *pHttpServ   - module state

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateListen(ProtoHttpServRefT *pHttpServ)
{
    ProtoSSLRefT *pProtoSSL;
    ProtoHttpServThreadT *pHttpThread;
    struct sockaddr RequestAddr;
    int32_t iAddrLen = sizeof(RequestAddr);

    // don't try Accept() if poll hint says nothing to read
#if 0 //$$ TODO - fix this to work on linux
    if (ProtoSSLStat(pHttpServ->pListenSSL, 'read', NULL, 0) == 0)
    {
        return(0);
    }
#endif
    // accept incoming connection
    if ((pProtoSSL = ProtoSSLAccept(pHttpServ->pListenSSL, pHttpServ->iSecure, &RequestAddr, &iAddrLen)) == NULL)
    {
        return(0);
    }
    // allocate an http thread
    if ((pHttpThread = _ProtoHttpServThreadAlloc(pHttpServ, pProtoSSL, &RequestAddr)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "unable to alloc http thread\n");
        ProtoSSLDestroy(pProtoSSL);
        return(0);
    }
    // insert it in the list
    pHttpThread->pNext = pHttpServ->pThreadList;
    pHttpServ->pThreadList = pHttpThread;

    // log connecting request
    _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "connecting to %a:%d (%s)\n", SockaddrInGetAddr(&pHttpThread->RequestAddr),
        SockaddrInGetPort(&pHttpThread->RequestAddr), pHttpServ->iSecure ? "secure" : "insecure");

    // set server certificate and private key, if specified
    if (pHttpServ->iSecure)
    {
        ProtoSSLControl(pHttpThread->pProtoSSL, 'scrt', pHttpServ->iServerCertLen, 0, pHttpServ->pServerCert);
        ProtoSSLControl(pHttpThread->pProtoSSL, 'skey', pHttpServ->iServerKeyLen, 0, pHttpServ->pServerKey);
        ProtoSSLControl(pHttpThread->pProtoSSL, 'ccrt', pHttpServ->iClientCertLvl, 0, NULL);
        if (pHttpServ->uSslVersion != 0)
        {
            ProtoSSLControl(pHttpThread->pProtoSSL, 'vers', pHttpServ->uSslVersion, 0, NULL);
        }
        if (pHttpServ->uSslVersionMin != 0)
        {
            ProtoSSLControl(pHttpThread->pProtoSSL, 'vmin', pHttpServ->uSslVersionMin, 0, NULL);
        }
        ProtoSSLControl(pHttpThread->pProtoSSL, 'ciph', pHttpServ->uSslCiphers, 0, NULL);
    }
    pHttpThread->bConnected = FALSE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServSocketClose

    \Description
        Handle socket close on connect/send/recv

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)
    \Input iResult      - socket operation result
    \Input *pOperation  - operation type (conn, recv, send)

    \Version 10/31/2013 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServSocketClose(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread, int32_t iResult, const char *pOperation)
{
    ProtoSSLAlertDescT AlertDesc;
    int32_t iAlert;

    if ((iAlert = ProtoSSLStat(pHttpThread->pProtoSSL, 'alrt', &AlertDesc, sizeof(AlertDesc))) != 0)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "%s ssl alert %s (sslerr=%d, hResult=0x%08x); closing connection\n", (iAlert == 1) ? "recv" : "sent",
            AlertDesc.pAlertDesc, ProtoSSLStat(pHttpThread->pProtoSSL, 'fail', NULL, 0), ProtoSSLStat(pHttpThread->pProtoSSL, 'hres', NULL, 0));
    }
    else
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "%s returned %d (sockerr=%d, sslerr=%d, hResult=0x%08x); closing connection\n", pOperation, iResult,
            ProtoSSLStat(pHttpThread->pProtoSSL, 'serr', NULL, 0), ProtoSSLStat(pHttpThread->pProtoSSL, 'fail', NULL, 0), 
            ProtoSSLStat(pHttpThread->pProtoSSL, 'hres', NULL, 0));
    }

    _ProtoHttpServDisconnect(pHttpServ, pHttpThread, 0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateConnect

    \Description
        Update HttpServ while establishing a new connection

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateConnect(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iStat;

    if ((iStat = ProtoSSLStat(pHttpThread->pProtoSSL, 'stat', NULL, 0)) > 0)
    {
        if (pHttpServ->iSecure)
        {
            char strTlsVersion[16], strCipherSuite[32], strResumed[] = " (resumed)";
            ProtoSSLStat(pHttpThread->pProtoSSL, 'vers', strTlsVersion, sizeof(strTlsVersion));
            ProtoSSLStat(pHttpThread->pProtoSSL, 'ciph', strCipherSuite, sizeof(strCipherSuite));
            _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "connected to %a:%d using %s and %s%s\n", SockaddrInGetAddr(&pHttpThread->RequestAddr), SockaddrInGetPort(&pHttpThread->RequestAddr),
                strTlsVersion, strCipherSuite, ProtoSSLStat(pHttpThread->pProtoSSL, 'resu', NULL, 0) ? strResumed : "");
        }
        else
        {
            _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "connected to %a:%d (insecure)\n", SockaddrInGetAddr(&pHttpThread->RequestAddr), SockaddrInGetPort(&pHttpThread->RequestAddr));
        }
        pHttpThread->bConnected = TRUE;
        pHttpThread->iIdleTimer = NetTick();
        return(1);
    }
    else if (iStat < 0)
    {
        _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iStat, "conn");
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateRecvHeader

    \Description
        Update HttpServ while receiving header

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateRecvHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult;

    if ((iResult = ProtoSSLRecv(pHttpThread->pProtoSSL, pHttpThread->pBuffer+pHttpThread->iBufLen, pHttpThread->iBufMax-pHttpThread->iBufLen)) > 0)
    {
        char *pHdrEnd;
        int32_t iHdrLen;

        NetPrintf(("protohttpserv: [%02x] recv %d bytes\n", pHttpThread->uHttpThreadId, iResult));

        // update received data size
        pHttpThread->iBufLen += iResult;

        // reset idle timeout
        pHttpThread->iIdleTimer = NetTick();

        // check for header termination
        if ((pHdrEnd = strstr(pHttpThread->pBuffer, "\r\n\r\n")) != NULL)
        {
            // we want to keep the final header EOL
            pHdrEnd += 2;

            // copy header to buffer
            iHdrLen = (int32_t)(pHdrEnd - pHttpThread->pBuffer);
            ds_strsubzcpy(pHttpThread->strHeader, sizeof(pHttpThread->strHeader), pHttpThread->pBuffer, iHdrLen);
            NetPrintf(("protohttpserv: [%02x] received %d byte header\n", pHttpThread->uHttpThreadId, iHdrLen));
            NetPrintWrap(pHttpThread->strHeader, 132);

            // skip header/body seperator
            pHdrEnd += 2;
            iHdrLen += 2;

            // remove header from buffer
            memmove(pHttpThread->pBuffer, pHdrEnd, pHttpThread->iBufLen - iHdrLen);
            pHttpThread->iBufLen -= iHdrLen;

            // we've received the header
            pHttpThread->bReceivedHeader = TRUE;

            // reset for next state
            pHttpThread->bConnectionClose = FALSE;
            pHttpThread->bParsedHeader = FALSE;
            pHttpThread->bFormattedHeader = FALSE;
            pHttpThread->bSentHeader = FALSE;
        }

        // return whether we have parsed the header or not
        iResult = pHttpThread->bReceivedHeader ? 1 : 0;
    }
    else if (iResult < 0)
    {
        _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iResult, "recv");
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServProcessData

    \Description
        Process received data

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 03/20/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServProcessData(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iDataSize;
    if ((iDataSize = pHttpServ->pWriteCb(pHttpThread->pBuffer+pHttpThread->iBufOff, pHttpThread->iBufLen-pHttpThread->iBufOff, pHttpThread->RequestInfo.pRequestData, pHttpServ->pUserData)) > 0)
    {
        pHttpThread->iContentRecv += iDataSize;
        pHttpThread->iBufOff += iDataSize;

        NetPrintf(("protohttpserv: [%02x] wrote %d bytes to file (%qd total)\n", pHttpThread->uHttpThreadId, iDataSize, pHttpThread->iContentRecv));

        // if we've written all of the data reset buffer pointers
        if (pHttpThread->iBufOff == pHttpThread->iBufLen)
        {
            pHttpThread->iBufOff = 0;
            pHttpThread->iBufLen = 0;
        }
    }
    return(iDataSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServProcessChunkData

    \Description
        Process received chunk data

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 03/20/2014 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServProcessChunkData(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iChkLeft, iDataSize;

    // do we need a new chunk header?
    if (pHttpThread->iChkLen == 0)
    {
        char *s = pHttpThread->pBuffer+pHttpThread->iBufOff, *s2;
        char *t = pHttpThread->pBuffer+pHttpThread->iBufLen-1;

        // make sure we have a complete chunk header
        for (s2=s; (s2 < t) && ((s2[0] != '\r') || (s2[1] != '\n')); s2++)
            ;
        if (s2 >= t)
        {
            return(0);
        }

        // read chunk header; handle end of input
        if ((pHttpThread->iChkLen = (int32_t)strtol(s, NULL, 16)) == 0)
        {
            NetPrintf(("protohttpserv: [%02x] processing end chunk\n", pHttpThread->uHttpThreadId));
            pHttpThread->iBufOff = 0;
            pHttpThread->iBufLen = 0;
            pHttpThread->iContentLength = pHttpThread->iContentRecv;
            return(0);
        }
        NetPrintf(("protohttpserv: [%02x] processing %d byte chunk\n", pHttpThread->uHttpThreadId, pHttpThread->iChkLen));

        // reset chunk read counter
        pHttpThread->iChkRcv = 0;

        // consume chunk header and \r\n trailer
        pHttpThread->iBufOff += s2-s+2;
    }

    // if chunk size is bigger than our buffer, realloc our buffer larger to accomodate, so we can buffer an entire chunk in our buffer
    if (pHttpThread->iChkLen > pHttpThread->iBufMax)
    {
        NetPrintf(("protohttpserv: [%02x] increasing buffer to handle %d byte chunk\n", pHttpThread->uHttpThreadId, pHttpThread->iChkLen));
        if (_ProtoHttpServThreadReallocBuf(pHttpServ, pHttpThread, pHttpThread->iChkLen+2) < 0) // chunk plus trailer
        {
            return(-1);
        }
    }

    // get how much of the chunk we have left to read
    iChkLeft = pHttpThread->iChkLen - pHttpThread->iChkRcv;

    // get data size
    if ((iDataSize = pHttpThread->iBufLen - pHttpThread->iBufOff) < iChkLeft)
    {
        // compact buffer to make room for more data
        memmove(pHttpThread->pBuffer, pHttpThread->pBuffer+pHttpThread->iBufOff, iDataSize);
        pHttpThread->iBufLen -= pHttpThread->iBufOff;
        pHttpThread->iBufOff = 0;
        return(0);
    }
    // clamp data size to whatever we have left to read
    iDataSize = iChkLeft;

    // write the data
    if ((iDataSize = pHttpServ->pWriteCb(pHttpThread->pBuffer+pHttpThread->iBufOff, iDataSize, pHttpThread->RequestInfo.pRequestData, pHttpServ->pUserData)) > 0)
    {
        NetPrintf(("protohttpserv: [%02x] wrote %d bytes to file (%qd total)\n", pHttpThread->uHttpThreadId, iDataSize, pHttpThread->iContentRecv+iDataSize));
        //NetPrintMem(pHttpThread->pBuffer+pHttpThread->iBufOff, iDataSize, "protohttp-write");

        pHttpThread->iContentRecv += iDataSize;
        pHttpThread->iBufOff += iDataSize;
        pHttpThread->iChkRcv += iDataSize;

        // did we consume all of the chunk data?
        if (pHttpThread->iChkRcv == pHttpThread->iChkLen)
        {
            // consume chunk trailer, reset chunk length
            pHttpThread->iBufOff += 2;
            pHttpThread->iChkLen = 0;
        }

        // if we've written all of the data reset buffer pointers
        if (pHttpThread->iBufOff == pHttpThread->iBufLen)
        {
            pHttpThread->iBufOff = 0;
            pHttpThread->iBufLen = 0;
        }
    }
    return(iDataSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateRecvBody

    \Description
        Update HttpServ while receiving body

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateRecvBody(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iDataSize, iResult;

    while ((pHttpThread->iContentRecv < pHttpThread->iContentLength) || (pHttpThread->iContentLength == -1))
    {
        // need new data?
        while (pHttpThread->iBufLen < (signed)pHttpThread->iBufMax)
        {
            iResult = ProtoSSLRecv(pHttpThread->pProtoSSL, pHttpThread->pBuffer + pHttpThread->iBufLen, pHttpThread->iBufMax - pHttpThread->iBufLen);
            if (iResult > 0)
            {
                pHttpThread->iBufLen += iResult;
                NetPrintf(("protohttpserv: [%02x] recv %d bytes\n", pHttpThread->uHttpThreadId, iResult));
                // reset idle timeout
                pHttpThread->iIdleTimer = NetTick();
            }
            else if ((iResult == SOCKERR_CLOSED) && (pHttpThread->iContentLength == -1))
            {
                NetPrintf(("protohttpserv: [%02x] keep-alive connection closed by remote host\n", pHttpThread->uHttpThreadId));
                pHttpThread->iContentLength = pHttpThread->iContentRecv + pHttpThread->iBufLen;
                break;
            }
            else if (iResult < 0)
            {
                _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iResult, "recv");
                return(-2);
            }
            else
            {
                break;
            }
        }

        // if we have data, write it out
        if (pHttpThread->iBufLen > 0)
        {
            if (!pHttpThread->bChunked)
            {
                // write out as much data as we can
                iDataSize = _ProtoHttpServProcessData(pHttpServ, pHttpThread);
            }
            else
            {
                // write out as many chunks as we have
                while((iDataSize = _ProtoHttpServProcessChunkData(pHttpServ, pHttpThread)) > 0)
                    ;
            }

            if (iDataSize < 0)
            {
                NetPrintf(("protohttpserv: [%02x] error %d writing to file\n", pHttpThread->uHttpThreadId, iDataSize));
                pHttpThread->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
                return(-1);
            }
        }
        else
        {
            break;
        }
    }

    // check for upload completion
    if (pHttpThread->iContentRecv == pHttpThread->iContentLength)
    {
        // done receiving response
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "received upload of %qd bytes\n", pHttpThread->iContentRecv);

        // trigger callback to indicate transaction is complete
        pHttpServ->pWriteCb(NULL, 0, pHttpThread->RequestInfo.pRequestData, pHttpServ->pUserData);
        // we're done, so clear reference to request data, if any
        pHttpThread->RequestInfo.pRequestData = NULL;

        // done with response
        pHttpThread->iContentLength = 0;
        pHttpThread->bReceivedBody = TRUE;
        pHttpThread->iResponseCode = PROTOHTTP_RESPONSE_CREATED;
        return(1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateSendHeader

    \Description
        Update HttpServ while sending header

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateSendHeader(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult;

    // send data to requester
    if ((iResult = ProtoSSLSend(pHttpThread->pProtoSSL, pHttpThread->pBuffer + pHttpThread->iBufOff, pHttpThread->iBufLen - pHttpThread->iBufOff)) > 0)
    {
        pHttpThread->iBufOff += iResult;

        // reset idle timeout
        pHttpThread->iIdleTimer = NetTick();

        // are we done?
        if (pHttpThread->iBufOff == pHttpThread->iBufLen)
        {
            NetPrintf(("protohttpserv: [%02x] sent %d byte header\n", pHttpThread->uHttpThreadId, pHttpThread->iBufOff));
            if (pHttpThread->iContentLength == 0)
            {
                _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "sent %d response (no body)\n", pHttpThread->iResponseCode);
            }
            NetPrintWrap(pHttpThread->pBuffer, 132);
            pHttpThread->bSentHeader = TRUE;
            pHttpThread->iBufOff = 0;
            pHttpThread->iBufLen = 0;
            iResult = 1;
        }
        else
        {
            iResult = 0;
        }
    }
    else if (iResult < 0)
    {
        _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iResult, "send");
        pHttpThread->iBufOff = pHttpThread->iBufLen;
        return(-1);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateSendBody

    \Description
        Update HttpServ while sending response

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpServUpdateSendBody(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    int32_t iResult, iReadOff, iReadMax;
    uint8_t bError;

    // reserve space for chunk header
    if (pHttpThread->RequestInfo.iChunkLength != 0)
    {
        iReadOff = 10;
        iReadMax = pHttpThread->RequestInfo.iChunkLength;
    }
    else
    {
        iReadOff = 0;
        iReadMax = pHttpThread->iBufMax;
    }

    // while there is content to be sent
    for (bError = FALSE; pHttpThread->iContentSent < pHttpThread->iContentLength; )
    {
        // need new data?
        if (pHttpThread->iBufOff == pHttpThread->iBufLen)
        {
            NetPrintf(("protohttpserv: [%02x] sent=%qd len=%qd\n", pHttpThread->uHttpThreadId, pHttpThread->iContentSent, pHttpThread->iContentLength));
            // reset chunk overhead tracker
            pHttpThread->iChkOvr = 0;

            // read the data
            if ((pHttpThread->iBufLen = pHttpServ->pReadCb(pHttpThread->pBuffer+iReadOff, iReadMax, pHttpThread->RequestInfo.pRequestData, pHttpServ->pUserData)) > 0)
            {
                pHttpThread->iContentRead += pHttpThread->iBufLen;
                pHttpThread->iBufOff = 0;
                NetPrintf(("protohttpserv: [%02x] read %d bytes from file (%qd total)\n", pHttpThread->uHttpThreadId, pHttpThread->iBufLen, pHttpThread->iContentRead));

                // if chunked, fill in chunk header based on amount of data read
                if (pHttpThread->RequestInfo.iChunkLength != 0)
                {
                    char strHeader[11];

                    // format chunk header before data
                    ds_snzprintf(strHeader, sizeof(strHeader), "%08x\r\n", pHttpThread->iBufLen);
                    ds_memcpy(pHttpThread->pBuffer, strHeader, iReadOff);
                    pHttpThread->iBufLen += iReadOff;
                    pHttpThread->iChkOvr += iReadOff;

                    // add chunk trailer after data
                    pHttpThread->pBuffer[pHttpThread->iBufLen+0] = '\r';
                    pHttpThread->pBuffer[pHttpThread->iBufLen+1] = '\n';
                    pHttpThread->iBufLen += 2;
                    pHttpThread->iChkOvr += 2;

                    // if this is the last of the data, add terminating chunk
                    if (pHttpThread->iContentRead == pHttpThread->iContentLength)
                    {
                        ds_snzprintf(strHeader, sizeof(strHeader), "%08x\r\n", 0);
                        ds_memcpy(pHttpThread->pBuffer+pHttpThread->iBufLen, strHeader, iReadOff);
                        pHttpThread->iBufLen += iReadOff;
                        pHttpThread->iChkOvr += iReadOff;
                    }
                }
            }
            else
            {
                _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "error %d reading from file\n", pHttpThread->iBufLen);
                bError = TRUE;
                break;
            }
        }

        // do we have buffered data to send?
        if (pHttpThread->iBufOff < pHttpThread->iBufLen)
        {
            int32_t iSendLen = pHttpThread->iBufLen - pHttpThread->iBufOff;
            #if HTTPSERV_CHUNKWAITTEST
            if (iSendLen > 1) iSendLen -= 1;
            #endif
            iResult = ProtoSSLSend(pHttpThread->pProtoSSL, pHttpThread->pBuffer + pHttpThread->iBufOff, iSendLen);
            if (iResult > 0)
            {
                pHttpThread->iBufOff += iResult;
                pHttpThread->iContentSent += iResult;
                if (pHttpThread->iChkOvr > 0)
                {
                    pHttpThread->iContentSent -= pHttpThread->iChkOvr;
                    pHttpThread->iChkOvr = 0;
                }
                NetPrintf(("protohttpserv: [%02x] sent %d bytes (%qd total)\n", pHttpThread->uHttpThreadId, iResult, pHttpThread->iContentSent));
                // reset idle timeout
                pHttpThread->iIdleTimer = NetTick();
                #if HTTPSERV_CHUNKWAITTEST
                NetConnSleep(1000);
                #endif
            }
            else if (iResult < 0)
            {
                _ProtoHttpServSocketClose(pHttpServ, pHttpThread, iResult, "send");
                bError = TRUE;
                break;
            }
            else
            {
                break;
            }
        }
    }

    // check for send completion
    if ((pHttpThread->iContentSent == pHttpThread->iContentLength) || (bError == TRUE))
    {
        // done sending response
        if (pHttpThread->iContentLength != 0)
        {
            _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "sent %d response (%qd bytes)\n", pHttpThread->iResponseCode, pHttpThread->iContentSent);
        }

        // trigger callback to indicate transaction is complete
        pHttpServ->pReadCb(NULL, 0, pHttpThread->RequestInfo.pRequestData, pHttpServ->pUserData);

        // done with response
        pHttpThread->bSentBody = TRUE;
        return(1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpServUpdateThread

    \Description
        Update an HttpServ thread

    \Input *pHttpServ   - module state
    \Input *pHttpThread - http thread (connection-specific)

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpServUpdateThread(ProtoHttpServRefT *pHttpServ, ProtoHttpServThreadT *pHttpThread)
{
    uint32_t uCurTick = NetTick();

    // if no thread, or we are disconnected, nothing to update
    if (pHttpThread->pProtoSSL == NULL)
    {
        return;
    }

    // update ProtoSSL object
    ProtoSSLUpdate(pHttpThread->pProtoSSL);

    // poll for connection completion
    if ((pHttpThread->bConnected == FALSE) && (_ProtoHttpServUpdateConnect(pHttpServ, pHttpThread) <= 0))
    {
        return;
    }

    // check for timing out a keep-alive connection
    if (NetTickDiff(uCurTick, pHttpThread->iIdleTimer) > pHttpServ->iIdleTimeout)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "closing connection (idle timeout)\n");
        _ProtoHttpServDisconnect(pHttpServ, pHttpThread, FALSE);
        return;
    }

    // receive the header
    if ((pHttpThread->bReceivedHeader == FALSE) && (_ProtoHttpServUpdateRecvHeader(pHttpServ, pHttpThread) <= 0))
    {
        return;
    }

    // if disconnecting, early out (this is intentionally after recv)
    if (pHttpThread->bDisconnecting)
    {
        return;
    }

    // parse headers
    if ((pHttpThread->bParsedHeader == FALSE) && (_ProtoHttpServParseHeader(pHttpServ, pHttpThread) < 0))
    {
        return;
    }

    // process the request
    if (pHttpThread->bProcessedRequest == FALSE)
    {
        // if a PUT or POST
        if ((pHttpThread->eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (pHttpThread->eRequestType == PROTOHTTP_REQUESTTYPE_POST))
        {
            _ProtoHttpServProcessPut(pHttpServ, pHttpThread);
        }
        // if a GET or HEAD
        if ((pHttpThread->eRequestType == PROTOHTTP_REQUESTTYPE_GET) || (pHttpThread->eRequestType == PROTOHTTP_REQUESTTYPE_HEAD))
        {
            _ProtoHttpServProcessGet(pHttpServ, pHttpThread);
        }
        //$$TODO: handle other request types

        // mark that we've processed the request
        pHttpThread->bProcessedRequest = TRUE;
    }

    // if a PUT or POST
    if ((pHttpThread->eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (pHttpThread->eRequestType == PROTOHTTP_REQUESTTYPE_POST))
    {
        if ((pHttpThread->bReceivedBody == FALSE) && (_ProtoHttpServUpdateRecvBody(pHttpServ, pHttpThread) <= 0))
        {
            return;
        }
    }

    // format response header
    if ((pHttpThread->bFormattedHeader == FALSE) && (_ProtoHttpServFormatHeader(pHttpServ, pHttpThread) < 0))
    {
        return;
    }

    // send response header
    if ((pHttpThread->bSentHeader == FALSE) && (_ProtoHttpServUpdateSendHeader(pHttpServ, pHttpThread) <= 0))
    {
        return;
    }

    // process body data, if any
    if ((pHttpThread->bSentBody == FALSE) && (_ProtoHttpServUpdateSendBody(pHttpServ, pHttpThread) <= 0))
    {
        return;
    }

    // make sure we've sent all of the data
    if ((pHttpThread->pProtoSSL != NULL) && (ProtoSSLStat(pHttpThread->pProtoSSL, 'send', NULL, 0) > 0))
    {
        return;
    }

    // if no keep-alive initiate disconnection
    if (pHttpThread->bConnectionClose == TRUE)
    {
        _ProtoHttpServLogPrintf(pHttpServ, pHttpThread, "closing connection\n");
        _ProtoHttpServDisconnect(pHttpServ, pHttpThread, TRUE);
    }

    // reset transaction state
    _ProtoHttpServResetState(pHttpServ, pHttpThread);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoHttpServCreate

    \Description
        Create an HttpServ instance

    \Input iPort            - port to listen on
    \Input iSecure          - TRUE if secure, else FALSE
    \Input *pName           - name of server, used in Server: header

    \Output
        ProtoHttpServRefT * - pointer to state, or null on failure

    \Version 12/11/2013 (jbrookes)
*/
/********************************************************************************F*/
ProtoHttpServRefT *ProtoHttpServCreate(int32_t iPort, int32_t iSecure, const char *pName)
{
    ProtoHttpServRefT *pHttpServ;
    int32_t iMemGroup, iResult = 0;
    void *pMemGroupUserData;

    // query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate module state
    if ((pHttpServ = DirtyMemAlloc(sizeof(*pHttpServ), HTTPSERV_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpserv: [%p] unable to allocate module state\n", pHttpServ));
        return(NULL);
    }
    memset(pHttpServ, 0, sizeof(*pHttpServ));
    // save memgroup (will be used in ProtoHttpDestroy)
    pHttpServ->iMemGroup = iMemGroup;
    pHttpServ->pMemGroupUserData = pMemGroupUserData;

    // create a protossl listen ref
    if ((pHttpServ->pListenSSL = _ProtoHttpServSSLCreate(pHttpServ, iPort, TRUE)) == NULL)
    {
        _ProtoHttpServLogPrintf(pHttpServ, NULL, "could not create ssl ref on port %d\n", iPort);
        ProtoHttpServDestroy(pHttpServ);
        return(NULL);
    }
    // start listening
    if ((iResult = ProtoSSLListen(pHttpServ->pListenSSL, 2)) != SOCKERR_NONE)
    {
        _ProtoHttpServLogPrintf(pHttpServ, NULL, "error listening on diagnostic socket\n");
        ProtoHttpServDestroy(pHttpServ);
        return(NULL);
    }

    // save settings
    pHttpServ->iSecure = iSecure;
    ds_strnzcpy(pHttpServ->strServerName, pName, sizeof(pHttpServ->strServerName));

    // initialize default values
    pHttpServ->uCurrThreadId = 1;
    pHttpServ->uSslCiphers = PROTOSSL_CIPHER_ALL;
    pHttpServ->iIdleTimeout = HTTPSERV_IDLETIMEOUT_DEFAULT;

    // log success
    _ProtoHttpServLogPrintf(pHttpServ, NULL, "listening on port %d (%s)\n", iPort, iSecure ? "secure" : "insecure");

    // return new ref to caller
    return(pHttpServ);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServDestroy

    \Description
        Destroy an HttpServ instance

    \Input *pHttpServ       - module state to destroy

    \Version 12/11/2013 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpServDestroy(ProtoHttpServRefT *pHttpServ)
{
    if (pHttpServ->pListenSSL != NULL)
    {
        ProtoSSLDestroy(pHttpServ->pListenSSL);
    }
    DirtyMemFree(pHttpServ, HTTPSERV_MEMID, pHttpServ->iMemGroup, pHttpServ->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServCallback

    \Description
        Set required callback functions for HttpServ to use.

    \Input *pHttpServ   - module state
    \Input *pRequestCb  - request handler callback
    \Input *pReadCb     - read handler callback
    \Input *pWriteCb    - write handler callback
    \Input *pLogCb      - logging function to use (optional)
    \Input *pUserData   - user data for callbacks

    \Version 12/11/2013 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpServCallback(ProtoHttpServRefT *pHttpServ, ProtoHttpServRequestCbT *pRequestCb, ProtoHttpServReadCbT *pReadCb, ProtoHttpServWriteCbT *pWriteCb, ProtoHttpServLogCbT *pLogCb, void *pUserData)
{
    pHttpServ->pRequestCb = pRequestCb;
    pHttpServ->pReadCb = pReadCb;
    pHttpServ->pWriteCb = pWriteCb;
    pHttpServ->pLogCb = pLogCb;
    pHttpServ->pUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServControl

    \Description
        Set behavior of module, based on selector input

    \Input *pHttpServ   - reference pointer
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input iValue2      - selector specific
    \Input *pValue      - selector specific

    \Output
        int32_t         - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'ccrt'      Set client certificate (pValue=cert, iValue=len)
            'ciph'      Set supported cipher suites (iValue=PROTOSSL_CIPHER_*; default=PROTOSSL_CIPHER_ALL)
            'scrt'      Set certificate (pValue=cert, iValue=len)
            'skey'      Set private key (pValue=key, iValue=len)
            'time'      Set idle timeout in milliseconds (iValue=timeout; default=5 minutes)
            'vers'      Set server-supported maximum SSL version (iValue=version; default=0x302, TLS1.1)
            'vmin'      Set server-required minimum SSL version (iValue=version; default=0x300, SSLv3)
        \endverbatim

    \Version 12/12/2013 (jbrookes)
*/
/*******************************************************************************F*/
int32_t ProtoHttpServControl(ProtoHttpServRefT *pHttpServ, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iSelect == 'ccrt')
    {
        pHttpServ->iClientCertLvl = iValue;
        return(0);
    }
    if (iSelect == 'ciph')
    {
        pHttpServ->uSslCiphers = iValue;
        return(0);
    }
    if (iSelect == 'scrt')
    {
        pHttpServ->pServerCert = (char *)pValue;
        pHttpServ->iServerCertLen = iValue;
        return(0);
    }
    if (iSelect == 'skey')
    {
        pHttpServ->pServerKey = (char *)pValue;
        pHttpServ->iServerKeyLen = iValue;
        return(0);
    }
    if (iSelect == 'time')
    {
        pHttpServ->iIdleTimeout = iValue;
        return(0);
    }
    if (iSelect == 'vers')
    {
        pHttpServ->uSslVersion = iValue;
        return(0);
    }
    if (iSelect == 'vmin')
    {
        pHttpServ->uSslVersionMin = iValue;
        return(0);
    }
    // unsupported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServStatus

    \Description
        Return status of module, based on selector input

    \Input *pHttpServ   - module state
    \Input iSelect      - info selector (see Notes)
    \Input *pBuffer     - [out] storage for selector-specific output
    \Input iBufSize     - size of buffer

    \Output
        int32_t         - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
    \endverbatim

    \Version 12/12/2013 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpServStatus(ProtoHttpServRefT *pHttpServ, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    // unsupported selector
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpServUpdate

    \Description
        Update HttpServ module

    \Input *pHttpServ   - module state

    \Version 10/30/2013 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpServUpdate(ProtoHttpServRefT *pHttpServ)
{
    ProtoHttpServThreadT *pHttpThread, *pHttpThreadNext, **ppHttpThread;

    // if we don't have a listen object don't do anything at all
    if (pHttpServ->pListenSSL == NULL)
    {
        return;
    }

    // listen for a response
    _ProtoHttpServUpdateListen(pHttpServ);

    // update http threads
    for (pHttpThread = pHttpServ->pThreadList; pHttpThread != NULL; pHttpThread = pHttpThread->pNext)
    {
        _ProtoHttpServUpdateThread(pHttpServ, pHttpThread);
    }

    // clean up expired threads
    for (ppHttpThread = &pHttpServ->pThreadList; *ppHttpThread != NULL; )
    {
        pHttpThread = *ppHttpThread;
        if (pHttpThread->bDisconnecting && (NetTickDiff(NetTick(), pHttpThread->iDisconnectTimer) > HTTPSERV_DISCTIME))
        {
            pHttpThreadNext = pHttpThread->pNext;
            _ProtoHttpServThreadFree(pHttpServ, pHttpThread);
            *ppHttpThread = pHttpThreadNext;
        }
        else
        {
            ppHttpThread = &(*ppHttpThread)->pNext;
        }
    }
}

