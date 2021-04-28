/*H********************************************************************************/
/*!
    \File protohttp.c

    \Description
        This module implements an HTTP client that can perform basic transactions
        (get/put) with an HTTP server.  It conforms to but does not fully implement
        the 1.1 HTTP spec (http://www.w3.org/Protocols/rfc2616/rfc2616.html), and
        allows for secure HTTP transactions as well as insecure transactions.

    \Copyright
        Copyright (c) Electronic Arts 2000-2004. ALL RIGHTS RESERVED.

    \Version 0.5 02/21/2000 (gschaefer) First Version
    \Version 1.0 12/07/2000 (gschaefer) Added PS2/Dirtysock support
    \Version 1.1 03/03/2004 (sbevan)    Rewrote to use ProtoSSL, added limited Post support.
    \Version 1.2 11/18/2004 (jbrookes)  Refactored, updated to HTTP 1.1, added full Post support.
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "dirtyvers.h"
#include "protossl.h"

#ifdef DIRTYCODE_XENON
#include "protohttpxenon.h"
#endif

#include "protohttp.h"

/*** Defines **********************************************************************/

//! default ProtoHttp timeout
#define PROTOHTTP_TIMEOUT       (30*1000)

//! default maximum allowed redirections
#define PROTOHTTP_MAXREDIRECT   (3)

//! size of "last-received" header cache
#define PROTOHTTP_HDRCACHESIZE  (1024)

//! protohttp revision number (maj.min)
#define PROTOHTTP_VERSION       (0x0103)          // update this for major bug fixes or protocol additions/changes

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! http module state
struct ProtoHttpRefT
{
    ProtoSSLRefT *pSsl;     //!< ssl module

    ProtoHttpCustomHeaderCbT *pCustomHeaderCb;      //!< callback for modifying request header
    ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb;    //!< callback for viewing recv header on recepit
    void *pCallbackRef;     //!< user ref for callback

    // module memory group
    int32_t iMemGroup;      //!< module mem group id
    void *pMemGroupUserData;//!< user data associated with mem group

    ProtoHttpRequestTypeE eRequestType;  //!< request type of current request
    int32_t iPort;          //!< server port
    int32_t iBasePort;      //!< base port (used for partial urls)
    int32_t iSecure;        //!< secure connection
    int32_t iBaseSecure;    //!< base security setting (used for partial urls)

    enum
    {
        ST_IDLE,            //!< idle
        ST_CONN,            //!< connecting
        ST_SEND,            //!< sending buffered data
        ST_RESP,            //!< waiting for initial response (also sending any data not buffered if POST or PUT)
        ST_HEAD,            //!< getting header
        ST_BODY,            //!< getting body
        ST_DONE,            //!< transaction success
        ST_FAIL             //!< transaction failed
    } eState;               //!< current state

    int32_t iSslFail;       //!< ssl failure code, if any
    int32_t iHdrCode;       //!< result code
    int32_t iHdrDate;       //!< last modified date

    int32_t iPostSize;      //!< amount of data being sent in a POST or PUT operation
    int32_t iHeadSize;      //!< size of head data
    int32_t iBodySize;      //!< size of body data
    int32_t iBodyRcvd;      //!< size of body data received by caller
    int32_t iRecvSize;      //!< amount of data received by ProtoHttpRecvAll
    int32_t iRecvRslt;      //!< last receive result

    char *pInpBuf;          //!< input buffer
    int32_t iInpMax;        //!< maximum buffer size
    int32_t iInpOff;        //!< offset into buffer
    int32_t iInpLen;        //!< total length
    int32_t iInpCnt;        //!< ongoing count
    int32_t iInpOvr;        //!< input overflow amount
    int32_t iChkLen;        //!< chunk length (if chunked encoding)
    int32_t iHdrLen;        //!< length of header(s) queued for sending
    int32_t iHdrOff;        //!< temp offset used when receiving header

    int32_t iNumRedirect;   //!< number of redirections processed
    int32_t iMaxRedirect;   //!< maximum number of redirections allowed

    uint32_t uTimeout;      //!< protocol timeout
    uint32_t uTimer;        //!< timeout timer
    int32_t iKeepAlive;     //!< indicate if we should try to use keep-alive
    int32_t iKeepAliveDflt; //!< keep-alive default (keep-alive will be reset to this value; can be overridden by user)

    char *pAppendHdr;       //!< append header buffer pointer
    int32_t iAppendLen;     //!< size of append header buffer

    char strHdr[PROTOHTTP_HDRCACHESIZE]; //!< storage for most recently received HTTP header
    char strRequestHdr[PROTOHTTP_HDRCACHESIZE]; //!< storage for most recent HTTP request header
    char strHost[256];      //!< server name
    char strBaseHost[256];  //!< base server name (used for partial urls)

    uint8_t bTimeout;       //!< boolean indicating whether a timeout occurred or not
    uint8_t bChunked;       //!< if TRUE, transfer is chunked
    uint8_t bHeadOnly;      //!< if TRUE, only get header
    uint8_t bCloseHdr;      //!< server wants close after this
    uint8_t bClosed;        //!< connection has been closed
    uint8_t bConnOpen;      //!< connection is open
    uint8_t iVerbose;       //!< debug output verbosity
    uint8_t bVerifyHdr;     //!< perform header type verification
    uint8_t bHttp1_0;       //!< TRUE if HTTP/1.0, else FALSE
    uint8_t bCompactRecv;   //!< compact receive buffer
    uint8_t bInfoHdr;       //!< TRUE if a new informational header has been cached; else FALSE
    uint8_t bNewConnection; //!< TRUE if a new connection should be used, else FALSE (if using keep-alive)
    uint8_t bPipelining;    //!< TRUE if pipelining is enabled, else FALSE
    uint8_t bPipeGetNext;   //!< TRUE if we should proceed to next pipelined result, else FALSE
    int8_t iPipedRequests;  //!< number of pipelined requests
    uint8_t bPipedRequestsLost; //!< TRUE if pipelined requests were lost due to a premature close

};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

// update this when PROTOHTTP_NUMREQUESTTYPES changes
static const char _ProtoHttp_strRequestNames[PROTOHTTP_NUMREQUESTTYPES][16] =
{
    "HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS"
};


// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoHttpParseUrl

    \Description
        Parse information from given URL.

    \Input *pUrl        - pointer to input URL to parse
    \Input *pKind       - [out] storage for parsed URL kind (eg "http", "https")
    \Input iKindSize    - size of pKind buffer
    \Input *pHost       - [out] storage for parsed URL host
    \Input iHostSize    - size of pHost buffer
    \Input *pPort       - [out] storage for parsed port
    \Input *pSecure     - [out] storage for parsed security (0 or 1)
    \Input *pPortSpecified - [out] storage for whether port was specified

    \Output
        const char *    - pointer to URL arguments

    \Version 11/10/2004 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
static const char *_ProtoHttpParseUrl(const char *pUrl, char *pKind, int32_t iKindSize, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure, uint8_t *pPortSpecified)
{
    const char *s;
    int32_t i, iPort;

    // skip any leading white-space
    while ((*pUrl != 0) && (*pUrl <= ' '))
    {
        pUrl++;
    }

    // see if there is a protocol reference
    *pKind = '\0';
    for (s = pUrl; isalpha(*s); s++)
    {
    }
    if ((*s == ':') && ((s-pUrl) < iKindSize))
    {
        // copy over the protocol kind
        for (i = 0; pUrl[i] != ':'; i++)
        {
            pKind[i] = pUrl[i];
        }
        pUrl += i+1;
        pKind[i] = '\0';
    }

    // determine if secure connection
    *pSecure = (ds_stricmp(pKind, "https") == 0);

    // skip past any white space
    while ((*pUrl != 0) && (*pUrl <= ' '))
    {
        pUrl++;
    }

    // skip optional //
    if ((pUrl[0] == '/') && (pUrl[1] == '/'))
    {
        pUrl += 2;
    }

    // extract the server name
    for (i = 0; i < (iHostSize-1); i++)
    {
        // make sure the data is valid
        if ((*pUrl <= ' ') || (*pUrl == '/') || (*pUrl == '?') || (*pUrl == ':'))
        {
            break;
        }
        pHost[i] = *pUrl++;
    }
    pHost[i] = '\0';

    // extract the port if included
    iPort = 0;
    if (*pUrl == ':')
    {
        for (pUrl++, iPort = 0; (*pUrl >= '0') && (*pUrl <= '9'); pUrl++)
        {
            iPort = (iPort * 10) + (*pUrl & 15);
        }
    }
    // if port is unspecified, set it here to a standard port
    if (iPort == 0)
    {
        iPort = *pSecure ? 443 : 80;
        *pPortSpecified = FALSE;
    }
    else
    {
        *pPortSpecified = TRUE;
    }
    *pPort = iPort;

    // skip past white space to arguments
    while ((*pUrl != 0) && (*pUrl <= ' '))
    {
        pUrl++;
    }

    // return pointer to arguments
    return(pUrl);
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
        NetPrintf(("protohttp: host not present; setting to %s\n", pState->strBaseHost));
        strnzcpy(pHost, pState->strBaseHost, iHostSize);
        bChanged = TRUE;
    }
    if ((bPortSpecified == FALSE) && (pState->iBasePort != 0))
    {
        NetPrintf(("protohttp: port not present; setting to %d\n", pState->iBasePort));
        *pPort = pState->iBasePort;
        bChanged = TRUE;
    }
    if (*pKind == '\0')
    {
        NetPrintf(("protohttp: kind (protocol) not present; setting to %d\n", pState->iBaseSecure));
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
    if (pState->bClosed)
    {
        // already issued disconnect, don't need to do it again
        return;
    }

    NetPrintf(("protohttp: [0x%08x] closing connection: %s\n", (uintptr_t)pState, pReason));
    ProtoSSLDisconnect(pState->pSsl);
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
    memset(pState->strHdr, 0, sizeof(pState->strHdr));
    memset(pState->strRequestHdr, 0, sizeof(pState->strRequestHdr));
    pState->eState = ST_IDLE;
    pState->iSslFail = 0;
    pState->iHdrCode = -1;
    pState->iHdrDate = 0;
    pState->iHeadSize = 0;
    pState->iBodySize = pState->iBodyRcvd = 0;
    pState->iRecvSize = 0;
    pState->iInpOff = 0;
    pState->iInpLen = 0;
    pState->iInpOvr = 0;
    pState->iChkLen = 0;
    pState->bTimeout = FALSE;
    pState->bChunked = FALSE;
    pState->bClosed = FALSE;
    pState->bHeadOnly = FALSE;
    pState->bPipeGetNext = FALSE;
    pState->bPipedRequestsLost = FALSE;
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
            NetPrintf(("protohttp: could not allocate %d byte buffer for append header\n", iAppendBufLen));
            pState->iAppendLen = 0;
            return(-1);
        }
    }

    // copy append header
    strnzcpy(pState->pAppendHdr, pAppendHdr, iAppendStrLen+1);

    // if append header is not \r\n terminated, do it here
    if (pState->pAppendHdr[iAppendStrLen-2] != '\r' || pState->pAppendHdr[iAppendStrLen-1] != '\n')
    {
        strnzcat(pState->pAppendHdr, "\r\n", pState->iAppendLen);
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
    \Input *pRequest    - pointer to request type ("GET", "HEAD", "POST", "PUT")
    \Input *pData       - pointer to included data, or NULL if none
    \Input iDataLen     - size of included data; zero if none, negative if streaming put/post

    \Output
        int32_t         - zero=success, else error

    \Version 11/11/2004 (jbrookes) Split/combined from ProtoHttpGet() and ProtoHttpPost()
*/
/********************************************************************************F*/
//$$ [DEPRECATE-V9] Use snzprintf with C99 overflow semantics for this function
#undef snzprintf
#define snzprintf ds_snzprintf2
//$$ [DEPRECATE-V9] Use snzprintf with C99 overflow semantics for this function
static int32_t _ProtoHttpFormatRequestHeader(ProtoHttpRefT *pState, const char *pUrl, const char *pHost, int32_t iPort, const char *pRequest, const char *pData, int32_t iDataLen)
{
    int32_t iInpMax, iOffset = 0;
    char *pInpBuf;

    // if no url specified use the minimum
    if (*pUrl == '\0')
    {
        pUrl = "/";
    }

    // set up for header formatting
    pInpBuf = pState->pInpBuf + pState->iInpLen;
    iInpMax = pState->iInpMax - pState->iInpLen;
    if (pState->iInpLen != 0)
    {
        pState->iPipedRequests += 1;
    }

    // format request header
    iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "%s %s HTTP/1.1\r\n", pRequest, pUrl);
    if ((pState->iSecure && (iPort == 443)) || (iPort == 80))
    {
        iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Host: %s\r\n", pHost);
    }
    else
    {
        iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Host: %s:%d\r\n", pHost, iPort);
    }
    if (iDataLen != -1)
    {
        iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Content-Length: %d\r\n", iDataLen);
    }
    else
    {
        iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Transfer-Encoding: chunked\r\n");
    }
    if (pState->iKeepAlive == 0)
    {
        iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Connection: Close\r\n");
    }
    if ((pState->pAppendHdr == NULL) || !stristr(pState->pAppendHdr, "User-Agent:"))
    {
        iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "User-Agent: ProtoHttp %d.%d/DS %d.%d.%d.%d (" DIRTYCODE_PLATNAME ")\r\n",
            (PROTOHTTP_VERSION>>8)&0xff, PROTOHTTP_VERSION&0xff,
            (DIRTYVERS>>24)&0xff, (DIRTYVERS>>16)&0xff,
            (DIRTYVERS>>8)&0xff, DIRTYVERS&0xff);
    }
    if ((pState->pAppendHdr == NULL) || (pState->pAppendHdr[0] == '\0'))
    {
        iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "Accept: */*\r\n");
    }
    else
    {
        iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "%s", pState->pAppendHdr);
    }

    // call custom header format callback, if specified
    if (pState->pCustomHeaderCb != NULL)
    {
        if ((iOffset = pState->pCustomHeaderCb(pState, pInpBuf, iInpMax, pData, iDataLen, pState->pCallbackRef)) < 0)
        {
            NetPrintf(("protohttp: custom header callback error %d\n", iOffset));
            return(iOffset);
        }
        if (iOffset == 0)
        {
            iOffset = (int32_t)strlen(pInpBuf);
        }
    }

    // append header terminator and return header length
    iOffset += snzprintf(pInpBuf+iOffset, iInpMax-iOffset, "\r\n");

    // make sure we were able to complete the header
    if (iOffset > iInpMax)
    {
        NetPrintf(("protohttp: not enough buffer to format request header (have %d, need %d)\n", iInpMax, iOffset));
        pState->iInpOvr = iOffset;
        return(PROTOHTTP_MINBUFF);
    }

    // save a copy of the header
    strnzcpy(pState->strRequestHdr, pInpBuf, sizeof(pState->strRequestHdr));

    // update buffer size
    pState->iInpLen += iOffset;

    // save updated header size
    pState->iHdrLen = pState->iInpLen;
    return(0);
}
//$$ [DEPRECATE-V9] Use snzprintf with C99 overflow semantics for this function
#undef snzprintf
#define snzprintf ds_snzprintf
//$$ [DEPRECATE-V9] Use snzprintf with C99 overflow semantics for this function

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
static int32_t _ProtoHttpFormatRequest(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int32_t iDataLen, ProtoHttpRequestTypeE eRequestType)
{
    char strHost[sizeof(pState->strHost)], strKind[8];
    int32_t iPort, iResult, iSecure;
    int32_t eState = pState->eState;
    uint8_t bPortSpecified;

    NetPrintf(("protohttp: [0x%08x] %s %s\n", (uintptr_t)pState, _ProtoHttp_strRequestNames[eRequestType], pUrl));
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

    // parse the url for kind, host, and port
    pUrl = _ProtoHttpParseUrl(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // fill in any missing info (relative url) if available
    if (_ProtoHttpApplyBaseUrl(pState, strKind, strHost, sizeof(strHost), &iPort, &iSecure, bPortSpecified) != 0)
    {
        NetPrintf(("protohttp: [0x%08x] %s %s://%s:%d%s\n", (uintptr_t)pState, _ProtoHttp_strRequestNames[eRequestType],
            iSecure ? "https" : "http", strHost, iPort, pUrl));
    }

    // determine if host, port, or security settings have changed since the previous request
    if ((iSecure != pState->iSecure) || (ds_stricmp(strHost, pState->strHost) != 0) || (iPort != pState->iPort))
    {
        NetPrintf(("protohttp: [0x%08x] requesting new connection -- url change to %s\n", (uintptr_t)pState, strHost));

        // reset keep-alive
        pState->iKeepAlive = pState->iKeepAliveDflt;

        // save new server/port/security state
        strnzcpy(pState->strHost, strHost, sizeof(pState->strHost));
        pState->iPort = iPort;
        pState->iSecure = iSecure;

        // make sure we use a new connection
        pState->bNewConnection = TRUE;
    }

    // check to see if previous connection (if any) is still active
    if ((pState->bNewConnection == FALSE) && (ProtoSSLStat(pState->pSsl, 'stat', NULL, 0) < 0))
    {
        NetPrintf(("protohttp: [0x%08x] requesting new connection -- previous connection was closed\n", (uintptr_t)pState));
        pState->bNewConnection = TRUE;
    }

    // check to make sure we are in a known valid state
    if ((pState->bNewConnection == FALSE) && (eState != ST_IDLE) && (eState != ST_DONE))
    {
        NetPrintf(("protohttp: [0x%08x] requesting new connection -- current state of %d does not allow connection reuse\n", (uintptr_t)pState, eState));
        pState->bNewConnection = TRUE;
    }

    // format the request header
    if ((iResult = _ProtoHttpFormatRequestHeader(pState, pUrl, strHost, iPort, _ProtoHttp_strRequestNames[eRequestType], pData, iDataLen)) < 0)
    {
        return(iResult);
    }

    // append data to header?
    if ((pData != NULL) && (iDataLen != 0))
    {
        // see how much data will fit into the buffer
        if (iDataLen > (pState->iInpMax - pState->iInpLen))
        {
            iDataLen = (pState->iInpMax - pState->iInpLen);
        }

        // copy data into buffer (must happen after _ProtoHttpFormatRequestHeader())
        memcpy(pState->pInpBuf + pState->iInpLen, pData, iDataLen);
        pState->iInpLen += iDataLen;
    }

    // set headonly status
    pState->bHeadOnly = (eRequestType == PROTOHTTP_REQUESTTYPE_HEAD) ? TRUE : FALSE;
    return(iDataLen);
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
    char cTest;

    /* if we still want to reuse the current connection, try and receive on it and
       make sure it is in a valid state (not an error state and no data to be read) */
    if (pState->bNewConnection == FALSE)
    {
        if ((iResult = ProtoSSLRecv(pState->pSsl, &cTest, sizeof(cTest))) > 0)
        {
            NetPrintf(("protohttp: [0x%08x] requesting new connection -- receive on previous connection returned data (0x%02x)\n", (uintptr_t)pState, cTest));
            pState->bNewConnection = TRUE;
        }
        else if (iResult < 0)
        {
            NetPrintf(("protohttp: [0x%08x] requesting new connection -- received %d error response from previous connection\n", (uintptr_t)pState, iResult));
            pState->bNewConnection = TRUE;
        }
    }

    // set connection timeout
    pState->uTimer = NetTick() + pState->uTimeout;

    // see if we need a new connection
    if (pState->bNewConnection == TRUE)
    {
        // close the existing connection, if not already closed
        _ProtoHttpClose(pState, "new connection");

        // start connect
        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [0x%08x] connect start (tick=%u)\n", (uintptr_t)pState, NetTick()));
        ProtoSSLConnect(pState->pSsl, pState->iSecure, pState->strHost, 0, pState->iPort);
        pState->eState = ST_CONN;
        pState->bClosed = FALSE;
    }
    else
    {
        // advance state
        NetPrintf(("protohttp: [0x%08x] reusing previous connection (keep-alive)\n", (uintptr_t)pState));
        pState->eState = ST_SEND;
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
    \Function _ProtoHttpRetrySendRequest

    \Description
        If the connection was a keep-alive connection and the request method was
        GET or HEAD, we re-issue the request one time on a fresh connection.

    \Input *pState      - reference pointer

    \Output
        uint32_t        - zero=did not reissue request, else we did

    \Version 07/14/2009 (jbrookes) Split from ProtoHttpUpdate()
*/
/********************************************************************************F*/
static uint32_t _ProtoHttpRetrySendRequest(ProtoHttpRefT *pState)
{
    // if this was a keepalive connection and we can retry
    if (pState->bNewConnection || ((pState->eRequestType != PROTOHTTP_REQUESTTYPE_GET) && (pState->eRequestType != PROTOHTTP_REQUESTTYPE_HEAD)))
    {
        return(0);
    }

    // retry the connection
    NetPrintf(("protohttp: [0x%08x] request failure on keep-alive connection; retrying\n", (uintptr_t)pState));
    _ProtoHttpClose(pState, "retry");

    // rewind buffer pointers to resend header
    pState->iInpLen = pState->iHdrLen;
    pState->iInpOff = 0;

    /* set keep-alive so we don't try another reconnect attempt, but we do
       request keep-alive on any further requests if this one succeeds */
    pState->iKeepAlive = 1;

    // reconnect
    ProtoSSLConnect(pState->pSsl, pState->iSecure, pState->strHost, 0, pState->iPort);
    pState->eState = ST_CONN;
    pState->bClosed = FALSE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpFindHeaderFieldValue

    \Description
        Finds the header field value for the designated header field.  The input
        header text to find should not include formatting (leading \n or trailing
        colon).

    \Input *pInpBuf     - input buffer
    \Input *pHeaderText - header field to find

    \Output
        char *          - pointer to header value text or null if not found

    \Version 03/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static const char *_ProtoHttpFindHeaderFieldValue(const char *pInpBuf, const char *pHeaderText)
{
    char strSearchText[64];
    const char *pFoundText;

    snzprintf(strSearchText, sizeof(strSearchText), "\n%s:", pHeaderText);
    if ((pFoundText = stristr(pInpBuf, strSearchText)) != NULL)
    {
        // skip header field name
        pFoundText += strlen(strSearchText);

        // skip any leading white-space
        while ((*pFoundText != '\0') && (*pFoundText <= ' '))
        {
            pFoundText += 1;
        }
    }

    return(pFoundText);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpGetHeaderFieldName

    \Description
        Extract header value from the designated header field.  Passing pOutBuf=null
        and iOutLen=0 results in returning the size required to store the field,
        null-inclusive.

    \Input *pInpBuf - pointer to header text
    \Input *pOutBuf - [out] output for header field name
    \Input iOutLen  - size of output buffer

    \Output
        int32_t     - negative=error, positive=success

    \Version 03/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpGetHeaderFieldName(const char *pInpBuf, char *pOutBuf, int32_t iOutLen)
{
    int32_t iOutSize;

    for (iOutSize = 0; (iOutSize < iOutLen) && (pInpBuf[iOutSize] != ':') && (pInpBuf[iOutSize] != '\0'); iOutSize += 1)
    {
        pOutBuf[iOutSize] = pInpBuf[iOutSize];
    }
    if (iOutSize == iOutLen)
    {
        return(-1);
    }
    pOutBuf[iOutSize] = '\0';
    return(iOutSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpGetHeaderFieldValue

    \Description
        Extract header value from the designated header field.  Passing pOutBuf=null
        and iOutLen=0 results in returning the size required to store the field,
        null-inclusive.

    \Input *pInpBuf - pointer to header text
    \Input *pOutBuf - [out] output for header field value, null for size query
    \Input iOutLen  - size of output buffer or zero for size query
    \Input **ppHdrEnd- [out] pointer past end of parsed header (optional)

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 03/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpGetHeaderFieldValue(const char *pInpBuf, char *pOutBuf, int32_t iOutLen, const char **ppHdrEnd)
{
    int32_t iValLen;

    // no input, nothing to look for
    if (pInpBuf == NULL)
    {
        return(-1);
    }

    // calc size and copy to buffer, if specified
    for (iValLen = 0; *pInpBuf != '\0' ; pInpBuf += 1, iValLen += 1)
    {
        // check for EOL (CRLF)
        if ((pInpBuf[0] == '\r') && (pInpBuf[1] == '\n'))
        {
            // check next character -- if whitespace we have a line continuation
            if ((pInpBuf[2] == ' ') || (pInpBuf[2] == '\t'))
            {
                // skip CRLF and LWS
                for (pInpBuf += 3; (*pInpBuf == ' ') || (*pInpBuf == '\t'); pInpBuf += 1)
                    ;
            }
            else // done
            {
                break;
            }
        }

        // copy to output buffer
        if (pOutBuf != NULL)
        {
            pOutBuf[iValLen] = *pInpBuf;
            if ((iValLen+1) >= iOutLen)
            {
                *pOutBuf = '\0';
                return(-1);
            }
        }
    }
    // set header end pointer, if requested
    if (ppHdrEnd != NULL)
    {
        *ppHdrEnd = pInpBuf;
    }
    // if a copy request, null-terminate and return success
    if (pOutBuf != NULL)
    {
        pOutBuf[iValLen] = '\0';
        return(0);
    }
    // if a size inquiry, return value length (null-inclusive)
    return(iValLen+1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpGetLocationHeader

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
static int32_t _ProtoHttpGetLocationHeader(ProtoHttpRefT *pState, const char *pInpBuf, char *pBuffer, int32_t iBufSize, const char **pHdrEnd)
{
    const char *pLocHdr;
    int32_t iLocLen, iLocPreLen=0;

    // get a pointer to header
    if ((pLocHdr = _ProtoHttpFindHeaderFieldValue(pState->pInpBuf, "location")) == NULL)
    {
        return(-1);
    }

    /* according to RFC location headers should be absolute, but some webservers respond with relative
       URL's.  we assume any url that does not start with "http://" or "https://" is a relative url,
       and if we find one, we assume the request keeps the same security, port, and host as the previous
       request */
    if (ds_strnicmp(pLocHdr, "http://", 7) && ds_strnicmp(pLocHdr, "https://", 8))
    {
        char strTemp[288]; // space for max DNS name (253 chars) plus max http url prefix

        // format http url prefix
        snzprintf(strTemp, sizeof(strTemp), "%s://%s:%d", pState->iSecure ? "https" : "http", pState->strHost, pState->iPort);

        // make sure relative URL includes '/' as the first character, required when we format the redirection url
        if (*pLocHdr != '/')
        {
            strnzcat(strTemp, "/", sizeof(strTemp));
        }

        // calculate url prefix length
        iLocPreLen = (int32_t)strlen(strTemp);

        // copy url prefix text if a buffer is specified
        if (pBuffer != NULL)
        {
            strnzcpy(pBuffer, strTemp, iBufSize);
            pBuffer = (void *)((uint8_t *)pBuffer + iLocPreLen);
            iBufSize -= iLocPreLen;
        }
    }

    // extract location header and return size
    iLocLen = _ProtoHttpGetHeaderFieldValue(pLocHdr, pBuffer, iBufSize, pHdrEnd);
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

    NetPrintf(("protohttp: [0x%08x] resizing input buffer from %d to %d bytes\n", (uintptr_t)pState, pState->iInpMax, iBufMax));
    if ((pInpBuf = DirtyMemAlloc(iBufMax, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp: [0x%08x] could not resize input buffer\n", (uintptr_t)pState));
        return(-1);
    }

    // calculate size of data to copy from old buffer to new
    if ((iCopySize = pState->iInpLen - pState->iInpOff) > iBufMax)
    {
        NetPrintf(("protohttp: [0x%08x] warning; resize of input buffer is losing %d bytes of data\n", iCopySize - iBufMax));
        iCopySize = iBufMax;
    }
    // copy valid contents of input buffer, if any, to new buffer
    memcpy(pInpBuf, pState->pInpBuf+pState->iInpOff, iCopySize);

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
    \Function _ProtoHttpParseHeaderCode

    \Description
        Parse header response code from HTTP header.

    \Input *pHdrBuf     - pointer to first line of HTTP header

    \Output
        int32_t         - parsed header code

    \Version 01/12/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpParseHeaderCode(const char *pHdrBuf)
{
    int32_t iHdrCode;

    // skip to result code
    while ((*pHdrBuf != '\r') && (*pHdrBuf > ' '))
    {
        pHdrBuf += 1;
    }
    while ((*pHdrBuf != '\r') && (*pHdrBuf <= ' '))
    {
        pHdrBuf += 1;
    }
    // grab the result code
    for (iHdrCode = 0; (*pHdrBuf >= '0') && (*pHdrBuf <= '9'); pHdrBuf += 1)
    {
        iHdrCode = (iHdrCode * 10) + (*pHdrBuf & 15);
    }
    return(iHdrCode);
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
    char *s = pState->pInpBuf;
    char *t = pState->pInpBuf+pState->iInpLen-3;
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
    pState->iHeadSize = s+4-pState->pInpBuf;
    // terminate header for easy parsing
    s[2] = s[3] = 0;

    // make sure the header is valid
    if (pState->bVerifyHdr)
    {
        if (strncmp(pState->pInpBuf, "HTTP", 4))
        {
            // header is invalid
            NetPrintf(("protohttp: [0x%08x] invalid result type\n", (uintptr_t)pState));
            pState->eState = ST_FAIL;
            return(-2);
        }
    }

    // detect if it is a 1.0 response
    pState->bHttp1_0 = !strncmp(pState->pInpBuf, "HTTP/1.0", 8);

    #if DIRTYCODE_LOGGING
    if (pState->iVerbose > 0)
    {
        NetPrintf(("protohttp: [0x%08x] received response header (%d bytes):\n", (uintptr_t)pState, pState->iHeadSize));
        NetPrintWrap(pState->pInpBuf, 80);
    }
    #endif

    // parse header code
    pState->iHdrCode = _ProtoHttpParseHeaderCode(pState->pInpBuf);

    // parse content-length field
    if ((_ProtoHttpGetHeaderFieldValue(_ProtoHttpFindHeaderFieldValue(pState->pInpBuf, "content-length"), strTemp, sizeof(strTemp), NULL)) != -1)
    {
        pState->iBodySize = strtol(strTemp, NULL, 10);
        pState->bChunked = FALSE;
    }
    else
    {
        pState->iBodySize = -1;
    }

    // parse last-modified header
    if ((_ProtoHttpGetHeaderFieldValue(_ProtoHttpFindHeaderFieldValue(pState->pInpBuf, "last-modified"), strTemp, sizeof(strTemp), NULL)) != -1)
    {
        pState->iHdrDate = strtotime(strTemp);
    }
    else
    {
        pState->iHdrDate = 0;
    }

    // parse transfer-encoding header
    if ((_ProtoHttpGetHeaderFieldValue(_ProtoHttpFindHeaderFieldValue(pState->pInpBuf, "transfer-encoding"), strTemp, sizeof(strTemp), NULL)) != -1)
    {
        pState->bChunked = !ds_stricmp(strTemp, "chunked");
    }

    // parse connection header
    if (pState->bCloseHdr == FALSE)
    {
        _ProtoHttpGetHeaderFieldValue(_ProtoHttpFindHeaderFieldValue(pState->pInpBuf, "connection"), strTemp, sizeof(strTemp), NULL);
        pState->bCloseHdr = !ds_stricmp(strTemp, "close");
        // if server is closing the connection and we are expecting subsequent piped results, we should not expect to get them
        if (pState->bCloseHdr && (pState->iPipedRequests > 0))
        {
            NetPrintf(("protohttp: [0x%08x] lost %d piped requests due to server connection-close request\n", (uintptr_t)pState, pState->iPipedRequests));
            pState->iPipedRequests = 0;
            pState->bPipedRequestsLost = TRUE;
        }
    }

    // note if it is an informational header
    pState->bInfoHdr = PROTOHTTP_GetResponseClass(pState->iHdrCode) == PROTOHTTP_RESPONSE_INFORMATIONAL;

    // copy header to header cache
    strnzcpy(pState->strHdr, pState->pInpBuf, sizeof(pState->strHdr));

    // trigger recv header user callback, if specified
    if (pState->pReceiveHeaderCb != NULL)
    {
        pState->pReceiveHeaderCb(pState, pState->pInpBuf, (uint32_t)strlen(pState->pInpBuf), pState->pCallbackRef);
    }

    // remove header from input buffer
    pState->iInpOff = pState->iHeadSize;
    pState->iInpCnt = pState->iInpLen - pState->iHeadSize;

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
    NetPrintf(("protohttp: [0x%08x] ignoring %d response from server\n", (uintptr_t)pState, pState->iHdrCode));

    pState->iInpLen = 0;
    pState->iInpOff = 0;
    pState->eState = ST_HEAD;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpProcessRedirect

    \Description
        Handle redirection header (response code=3xx)

    \Input *pState      - reference pointer

    \Notes
        A maximum of PROTOHTTP_MAXREDIRECT redirections is allowed.  Any further
        redirection attempts will result in a failure state.  A redirection
        Location url is limited based on the size of the http receive buffer.

        Auto-redirection is implemented as specified by RFC:
        (http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10.3);
        if auto-redirection is not performed, processing ends and it is the
        responsibility of the application to recognize the 3xx result code
        and handle it accordingly.

    \Version 11/12/2004 (jbrookes) First Version.
*/
/********************************************************************************F*/
static void _ProtoHttpProcessRedirect(ProtoHttpRefT *pState)
{
    int32_t iResult, iUrlLen;
    char *pUrlBuf;

    // do not auto-redirect multiplechoices or notmodified responses
    if ((pState->iHdrCode == PROTOHTTP_RESPONSE_MULTIPLECHOICES) || (pState->iHdrCode == PROTOHTTP_RESPONSE_NOTMODIFIED))
    {
        return;
    }
    // do not auto-redirect responses that are not head or get requests, and are not a SEEOTHER response
    if ((pState->eRequestType != PROTOHTTP_REQUESTTYPE_GET) && (pState->eRequestType != PROTOHTTP_REQUESTTYPE_HEAD))
    {
        /* As per HTTP 1.1 RFC, 303 SEEOTHER POST requests may be auto-redirected to a GET requset.  302 FOUND responses
           to a POST request are not supposed to be auto-redirected; however, there is a note in the RFC indicating that
           this is a common client behavior, and it is additionally a common server behavior to use 302 even when automatic
           redirection is intended, as some clients do not support 303 SEEOTHER.  Therefore, we perform auto-redirection
           on 302 FOUND responses to POST requests with a GET request for compatibility with servers that choose this
           behavior */
        if ((pState->iHdrCode == PROTOHTTP_RESPONSE_FOUND) || (pState->iHdrCode == PROTOHTTP_RESPONSE_SEEOTHER))
        {
            pState->eRequestType = PROTOHTTP_REQUESTTYPE_GET;
        }
        else
        {
            return;
        }
    }

    // increment redirect count, and bail if too many
    if (++pState->iNumRedirect > pState->iMaxRedirect)
    {
        NetPrintf(("protohttp: [0x%08x] maximum number of redirections (%d) exceeded\n", (uintptr_t)pState, pState->iMaxRedirect));
        pState->eState = ST_FAIL;
        return;
    }

    // get size of location header
    if ((iUrlLen = _ProtoHttpGetLocationHeader(pState, pState->pInpBuf, NULL, 0, NULL)) <= 0)
    {
        NetPrintf(("protohttp: [0x%08x] no location included in redirect header\n", (uintptr_t)pState));
        pState->eState = ST_FAIL;
        return;
    }

    // get url at the end of input buffer
    pUrlBuf = pState->pInpBuf + pState->iInpMax - iUrlLen;
    if (_ProtoHttpGetLocationHeader(pState, pState->pInpBuf, pUrlBuf, iUrlLen, NULL) != 0)
    {
        NetPrintf(("protohttp: [0x%08x] failed to get location header url", (uintptr_t)pState));
        pState->eState = ST_FAIL;
        return;
    }

    // close connection?
    if (pState->bCloseHdr)
    {
        _ProtoHttpClose(pState, "server request");
    }

    // clear piped result count
    pState->iPipedRequests = 0;
    pState->bPipedRequestsLost = FALSE;

    // format redirection request
    if ((iResult = _ProtoHttpFormatRequest(pState, pUrlBuf, NULL, 0, pState->eRequestType)) < 0)
    {
        NetPrintf(("protohttp: redirect header format request failed\n"));
        pState->eState = ST_FAIL;
        return;
    }
    // send redirection request
    _ProtoHttpSendRequest(pState);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpChunkProcess

    \Description
        Process output if chunked encoding.

    \Input *pState      - reference pointer
    \Input iBufMax      - maximum number of bytes to return (buffer size)

    \Output
        int32_t         - number of bytes available

    \Notes
        Does not support anything but chunked encoding.  Does not support optional
        end-transfer header (anything past the terminating 0 chunk is discarded).

    \Version 04/05/2005 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpChunkProcess(ProtoHttpRefT *pState, int32_t iBufMax)
{
    int32_t iChkSize, iLen;

    // if no new data, bail
    if (pState->iInpLen == pState->iInpOff)
    {
        return(0);
    }

    // see if we are starting a new chunk
    if (pState->iChkLen == 0)
    {
        char *s = pState->pInpBuf+pState->iInpOff, *s2;
        char *t = pState->pInpBuf+pState->iInpLen-1;

        // make sure we have a complete chunk header
        for (s2=s; (s2 < t) && ((s2[0] != '\r') || (s2[1] != '\n')); s2++)
            ;
        if (s2 == t)
        {
            if (pState->iInpLen == pState->iInpMax)
            {
                // tell ProtoHttpRecv() to compact recv buffer next time around
                pState->bCompactRecv = TRUE;
            }
            return(0);
        }

        // get the chunk length
        if ((pState->iChkLen = strtol(s, NULL, 16)) == 0)
        {
            // terminating chunk - clear the buffer and set state to done
            NetPrintf(("protohttp: [0x%08x] parsed end chunk\n", (uintptr_t)pState));
            pState->iInpOff += s2-s+4; // remove chunk header plus terminating crlf
            pState->iBodySize = pState->iBodyRcvd;
            pState->eState = ST_DONE;

            // return no data
            return(0);
        }
        else
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [0x%08x] parsed chunk size=%d\n", (uintptr_t)pState, pState->iChkLen));
        }

        // remove header from input
        pState->iInpOff += s2-s+2;
    }

    // calculate length
    if ((iLen = pState->iInpLen - pState->iInpOff) > iBufMax)
    {
        iLen = iBufMax;
    }

    // have we got all the data?
    if (iLen >= pState->iChkLen)
    {
        // got chunk and trailer, so consume it
        if (iLen >= (pState->iChkLen+2))
        {
            // reset chunk length
            iChkSize = pState->iChkLen;
            pState->iChkLen = 0;
        }
        else // consume some data and/or compact recv buffer to make room for the trailer
        {
            iChkSize = iLen/2;
            pState->iChkLen -= iChkSize;
            // tell ProtoHttpRecv() to compact recv buffer next time around
            pState->bCompactRecv = TRUE;
        }
    }
    else
    {
        iChkSize = iLen;
        pState->iChkLen -= iChkSize;
    }

    return(iChkSize);
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
    int32_t iResult;

    // try and send some data
    if ((iResult = ProtoSSLSend(pState->pSsl, pStrBuf, iSize)) > 0)
    {
        #if DIRTYCODE_LOGGING
        if (pState->iVerbose > 1)
        {
            NetPrintf(("protohttp: [0x%08x] sent %d bytes\n", (uintptr_t)pState, iResult));
        }
        if (pState->iVerbose > 2)
        {
            NetPrintMem(pStrBuf, iResult, "http-send");
        }
        #endif

        // sent data, so update timeout
        pState->uTimer = NetTick() + pState->uTimeout;
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttp: [0x%08x] error %d sending %d bytes\n", (uintptr_t)pState, iResult, iSize));
        pState->eState = ST_FAIL;
    }
    return(iResult);
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
        iSentSize = iResult;
    }
    else if (iResult < 0)
    {
        NetPrintf(("protohttp: [0x%08x] failed to send request (err=%d)\n", (uintptr_t)pState, iResult));
        pState->iInpLen = 0;
        pState->eState = ST_FAIL;
        iSentSize = -1;
    }
    return(iSentSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSendChunk

    \Description
        Send the specified data using chunked transfer encoding.

    \Input  *pState     - reference pointer
    \Input  *pStrBuf    - pointer to buffer to send from
    \Input  iSize       - amount of data to try and send (zero for stream completion)

    \Output
        int32_t         - negative=error, else number of bytes of user data sent

    \Notes
        Unlike _ProtoHttpSend(), which simply passes the data to ProtoSSL and returns
        the amount of data that was accepted, this function buffers one or more chunks
        of data, up to the buffer limit, and the data is sent by ProtoHttpUpdate().
        This guarantees that a chunk will be sent correctly even if a partial send
        occurs.  In addition, space is reserved for an end chunk to be buffered so the
        ProtoHttpSend() call indicating the stream is complete does not ever need to
        be retried.

    \Version 05/07/2008 (jbrookes) First Version.
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSendChunk(ProtoHttpRefT *pState, const char *pStrBuf, int32_t iSize)
{
    char *pInpBuf = pState->pInpBuf + pState->iInpLen;
    int32_t iInpMax = pState->iInpMax - pState->iInpLen;
    int32_t iSendSize, iSentSize, iCompSize;
    int32_t iResult;

    // calculate amount of space remaining in buffer
    iInpMax = pState->iInpMax - pState->iInpLen;

    // make sure we have enough room for a max chunk header, chunk data, and possible end chunk
    if ((iSendSize = iSize) > 0)
    {
        if (iSendSize > (iInpMax - (6+2+2 + 1+2+2)))
        {
            iSendSize = (iInpMax - (6+2+2 + 1+2+2));
        }
        if (iSendSize <= 0)
        {
            // compact send buffer
            if ((iCompSize = _ProtoHttpCompactBuffer(pState)) > 0)
            {
                // retry send
                NetPrintf(("protohttp: [0x%08x] compacted send buffer (%d bytes)\n", (uintptr_t)pState, iCompSize));
                return(_ProtoHttpSendChunk(pState, pStrBuf, iSize));
            }
            else
            {
                // no room to buffer for sending
                return(0);
            }
        }
    }
    else
    {
        pState->iPostSize = 0;
    }

    // format chunk into buffer
    iSentSize = snzprintf(pInpBuf, iInpMax, "%x\r\n", iSendSize);
    if (iSendSize > 0)
    {
        memcpy(pInpBuf+iSentSize, pStrBuf, iSendSize);
        iSentSize += iSendSize;
    }
    iSentSize += snzprintf(pInpBuf+iSentSize, iInpMax, "\r\n");

    // add chunk to length
    pState->iInpLen += iSentSize;

    // try to send the buffered data
    if ((iResult = _ProtoHttpSendBuff(pState)) < 0)
    {
        iSendSize = -1;
    }
    // return buffered size to caller
    return(iSendSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRecv

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
static int32_t _ProtoHttpRecv(ProtoHttpRefT *pState, char *pStrBuf, int32_t iSize)
{
    // if we have no buffer space, don't try to receive
    if (iSize == 0)
    {
        return(0);
    }

    // try and receive some data
    if ((pState->iRecvRslt = ProtoSSLRecv(pState->pSsl, pStrBuf, iSize)) > 0)
    {
        #if DIRTYCODE_LOGGING
        if (pState->iVerbose > 1)
        {
            NetPrintf(("protohttp: [0x%08x] recv %d bytes\n", (uintptr_t)pState, pState->iRecvRslt));
        }
        if (pState->iVerbose > 2)
        {
            NetPrintMem(pStrBuf, pState->iRecvRslt, "http-recv");
        }
        #endif

        // received data, so update timeout
        pState->uTimer = NetTick() + pState->uTimeout;
    }
    return(pState->iRecvRslt);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpRecvFirstHeaderLine

    \Description
        Try and receive the first line of the response header.  We receive into
        the header cache buffer directly so we can avoid conflicting with possible
        use of the input buffer sending streaming data.  This allows us to receive
        while we are sending, which is useful in detecting situations where the
        server responsds with an error in the middle of a streaming post transaction.

    \Input *pState          - reference pointer

    \Output
        int32_t             - negative=error, zero=pending, else success

    \Version 01/13/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRecvFirstHeaderLine(ProtoHttpRefT *pState)
{
    int32_t iResult;
    for (iResult = 1; (iResult == 1) && (pState->iHdrOff < 64);  ) // hard-coded max limit in case this is not http
    {
        if ((iResult = _ProtoHttpRecv(pState, pState->strHdr+pState->iHdrOff, 1)) == 1)
        {
            pState->iHdrOff += 1;
            if ((pState->strHdr[pState->iHdrOff-2] == '\r') && (pState->strHdr[pState->iHdrOff-1] == '\n'))
            {
                // we've received the first line of the response header... get response code
                int32_t iHdrCode = _ProtoHttpParseHeaderCode(pState->strHdr);
                //int32_t iHdrClass = PROTOHTTP_GetResponseClass(iHdrCode);

                /* if this is a streaming post, check the response.  we do this because a server might
                   abort a streaming upload prematurely if the file size is too large and this allows
                   the client to stop sending gracefully; typically the server will issue a forceful
                   reset if the client keeps sending data after being sent notification */
                if ((pState->iPostSize == -1) && (iHdrCode != PROTOHTTP_RESPONSE_CONTINUE))
                {
                    NetPrintf(("protohttp [0x%08x] got unexpected response %d during streaming post; aborting send\n", (uintptr_t)pState, iHdrCode));
                    pState->iPostSize = 0;
                }
            }
        }
    }
    return(iResult);
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

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // clamp the buffer size
    if (iBufSize < 4096)
    {
        iBufSize = 4096;
    }

    // allocate the resources
    if ((pState = DirtyMemAlloc(sizeof(*pState), PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp: [0x%08x] unable to allocate module state\n", (uintptr_t)pState));
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));
    // save memgroup (will be used in ProtoHttpDestroy)
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // allocate ssl module
    if ((pState->pSsl = ProtoSSLCreate()) == NULL)
    {
        NetPrintf(("protohttp: [0x%08x] unable to allocate ssl module\n", (uintptr_t)pState));
        ProtoHttpDestroy(pState);
        return(NULL);
    }
    if ((pState->pInpBuf = DirtyMemAlloc(iBufSize, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttp: [0x%08x] unable to allocate protohttp buffer\n", (uintptr_t)pState));
        ProtoHttpDestroy(pState);
        return(NULL);
    }

    // save parms & set defaults
    pState->eState = ST_IDLE;
    pState->iInpMax = iBufSize;
    pState->uTimeout = PROTOHTTP_TIMEOUT;
    pState->bVerifyHdr = TRUE;
    pState->iVerbose = 1;
    pState->iMaxRedirect = PROTOHTTP_MAXREDIRECT;

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

    \Output
        None.

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
    if (pState->pSsl != NULL)
    {
        ProtoSSLDestroy(pState->pSsl);
    }
    if (pState->pInpBuf != NULL)
    {
        DirtyMemFree(pState->pInpBuf, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
    if (pState->pAppendHdr != NULL)
    {
        DirtyMemFree(pState->pAppendHdr, PROTOHTTP_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
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

    // format the request
    if (pUrl != NULL)
    {
        if ((iResult = _ProtoHttpFormatRequest(pState, pUrl, NULL, 0, bHeadOnly ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_GET)) < 0)
        {
            return(iResult);
        }
    }
    // issue the request
    if (!pState->bPipelining || (pUrl == NULL))
    {
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
        ProtoHttpUpdate(pState);
    }

    // figure out how much data is available
    if (pState->bChunked == TRUE)
    {
        iLen = _ProtoHttpChunkProcess(pState, iBufMax);
    }
    else if ((iLen = (pState->iInpLen - pState->iInpOff)) > iBufMax)
    {
        iLen = iBufMax;
    }

    // check for end of data
    if ((iLen == 0) && (pState->eState == ST_DONE))
    {
        return(PROTOHTTP_RECVDONE);
    }

    // special check for responses with trailing piped data
    if (pState->iPipedRequests > 0)
    {
        // check for end of this transaction
        if (pState->iBodyRcvd == pState->iBodySize)
        {
            return(PROTOHTTP_RECVDONE);
        }
        // clamp available data to body size
        if ((pState->iBodySize != -1) && (iLen > (pState->iBodySize - pState->iBodyRcvd)))
        {
            iLen = pState->iBodySize - pState->iBodyRcvd;
        }
    }

    // see if there is enough to return
    if ((iLen >= iBufMin) || (pState->iInpCnt == pState->iBodySize))
    {
        // return data to caller
        if (pBuffer != NULL)
        {
            memcpy(pBuffer, pState->pInpBuf+pState->iInpOff, iLen);

            #if DIRTYCODE_LOGGING
            if (pState->iVerbose > 2)
            {
                NetPrintf(("protohttp: [0x%08x] read %d bytes\n", (uintptr_t)pState, iLen));
            }
            if (pState->iVerbose > 3)
            {
                NetPrintMem(pBuffer, iLen, "http-read");
            }
            #endif
        }
        pState->iInpOff += iLen;
        pState->iBodyRcvd += iLen;

        // if we're at the end of a chunk, skip trailing crlf
        if ((pState->bChunked == TRUE) && (pState->iChkLen == 0))
        {
            pState->iInpOff += 2;
        }

        // return bytes read
        return(iLen);
    }

    // nothing available
    return(0);
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
        NetPrintf(("protohttp: [0x%08x] error %d receiving response\n", (uintptr_t)pState, iRecvResult));
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
int32_t ProtoHttpPost(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int32_t iDataSize, uint32_t bDoPut)
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

    // send the data
    if (pState->iPostSize < 0)
    {
        return(_ProtoHttpSendChunk(pState, pData, iDataSize));
    }
    else
    {
        return(_ProtoHttpSend(pState, pData, iDataSize));
    }
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
    \Function ProtoHttpRequest

    \Description
        Initiate an HTTP transfer. Pass in a URL and the module starts a transfer
        from the appropriate server.

    \Input *pState      - reference pointer
    \Input *pUrl        - the url to retrieve
    \Input *pData       - user data to send to server (PUT and POST only)
    \Input iDataSize    - size of user data to send to server (PUT and POST only)
    \Input eRequestType - request type to make

    \Output
        int32_t         - negative=failure, zero=success, >0=number of data bytes sent (PUT and POST only)

    \Version 06/01/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpRequest(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int32_t iDataSize, ProtoHttpRequestTypeE eRequestType)
{
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
        NetPrintf(("protohttp: [0x%08x] unrecognized request type %d\n", (uintptr_t)pState, eRequestType));
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
    // terminate current connection, if any
    _ProtoHttpClose(pState, "abort");

    // reset state
    _ProtoHttpReset(pState);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGetHeaderValue

    \Description
        Extract header value for the header field specified by pName from pHdrBuf.
        Passing pOutBuf=null and iOutLen=0 results in returning the size required
        to store the field, null-inclusive.

    \Input *pState  - module state
    \Input *pHdrBuf - header text
    \Input *pName   - header field name (case-insensitive)
    \Input *pBuffer - [out] pointer to buffer to store extracted header value (null for size query)
    \Input iBufSize - size of output buffer (zero for size query)
    \Input **pHdrEnd- [out] pointer past end of parsed header (optional)

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 03/24/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetHeaderValue(ProtoHttpRefT *pState, const char *pHdrBuf, const char *pName, char *pBuffer, int32_t iBufSize, const char **pHdrEnd)
{
    int32_t iResult;

    // find header text in input buffer
    if ((pHdrBuf = _ProtoHttpFindHeaderFieldValue(pHdrBuf, pName)) == NULL)
    {
        return(-1);
    }

    // check for location header request which is specially handled
    if ((pState != NULL) && (!ds_stricmp(pName, "location")))
    {
        iResult = _ProtoHttpGetLocationHeader(pState, pHdrBuf, pBuffer, iBufSize, pHdrEnd);
    }
    else // extract the header
    {
        iResult = _ProtoHttpGetHeaderFieldValue(pHdrBuf, pBuffer, iBufSize, pHdrEnd);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpGetNextHeader

    \Description
        Get the next header name/value pair from the header buffer.  pHdrBuf should
        initially be pointing to the start of the header text buffer ("HTTP...") and
        this function can then be iteratively called to extract the individual header
        fields from the header one by one.  pHdrBuf could also start as the result
        of a call to ProtoHttpGetHeaderValue() if desired.

    \Input *pState  - module state
    \Input *pHdrBuf - header text
    \Input *pName   - [out] header field name
    \Input iNameSize- size of header field name buffer
    \Input *pValue  - [out] pointer to buffer to store extracted header value (null for size query)
    \Input iValSize - size of output buffer (zero for size query)
    \Input **pHdrEnd- [out] pointer past end of parsed header

    \Output
        int32_t     - negative=not found or not enough space, zero=success, positive=size query result

    \Version 03/24/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetNextHeader(ProtoHttpRefT *pState, const char *pHdrBuf, char *pName, int32_t iNameSize, char *pValue, int32_t iValSize, const char **pHdrEnd)
{
    int32_t iNameLen, iResult;

    // is this a first call?
    if (!strncmp(pHdrBuf, "HTTP", 4))
    {
        // skip http header
        for ( ; *pHdrBuf != '\r' && *pHdrBuf != '\0'; pHdrBuf += 1)
            ;
        if (*pHdrBuf == '\0')
        {
            return(-1);
        }
    }

    // skip crlf from previous line
    pHdrBuf += 2;

    // get header name
    if ((iNameLen = _ProtoHttpGetHeaderFieldName(pHdrBuf, pName, iNameSize)) <= 0)
    {
        return(-1);
    }
    // skip header name and lws
    for (pHdrBuf += iNameLen + 1; (*pHdrBuf != '\0') && (*pHdrBuf <= ' '); pHdrBuf += 1)
        ;

    // get header value
    iResult = _ProtoHttpGetHeaderFieldValue(pHdrBuf, pValue, iValSize, pHdrEnd);
    return(iResult);
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
    _ProtoHttpParseUrl(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // set base info
    NetPrintf(("protohttp: [0x%08x] setting base url to %s://%s:%d\n", (uintptr_t)pState, iSecure ? "https" : "http", strHost, iPort));
    strnzcpy(pState->strBaseHost, strHost, sizeof(pState->strBaseHost));
    pState->iBasePort = iPort;
    pState->iBaseSecure = iSecure;
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
            'pipe'      Sets pipelining; zero=disabled, else enabled
            'pnxt'      Call to proceed to next piped result
            'rmax'      Sets maximum number of redirections (default=3)
            'spam'      Sets debug output verbosity (0...n)
            'time'      Sets ProtoHttp client timeout in milliseconds (default=30s)
        \endverbatim

        Unhandled selectors are passed on to ProtoSSL.

    \Version 11/12/2004 (jbrookes) First Version
*/
/*******************************************************************************F*/
int32_t ProtoHttpControl(ProtoHttpRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
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
        NetPrintf(("protohttp: [0x%08x] header type verification %s\n", (uintptr_t)pState, iValue ? "enabled" : "disabled"));
        pState->bVerifyHdr = iValue;
    }
    if (iSelect == 'ires')
    {
        return(_ProtoHttpResizeBuffer(pState, iValue));
    }
    if (iSelect == 'keep')
    {
        NetPrintf(("protohttp: [0x%08x] setting keep-alive to %d\n", (uintptr_t)pState, iValue));
        pState->iKeepAlive = pState->iKeepAliveDflt = iValue;
        return(0);
    }
    if (iSelect == 'pipe')
    {
        NetPrintf(("protohttp: [0x%08x] pipelining %s\n", (uintptr_t)pState, iValue ? "enabled" : "disabled"));
        pState->bPipelining = iValue ? TRUE : FALSE;
        return(0);
    }
    if (iSelect == 'pnxt')
    {
        NetPrintf(("protohttp: [0x%08x] proceeding to next pipeline result\n", (uintptr_t)pState));
        pState->bPipeGetNext = TRUE;
        return(0);
    }
    if (iSelect == 'rmax')
    {
        pState->iMaxRedirect = iValue;
        return(0);
    }
    if (iSelect == 'spam')
    {
        pState->iVerbose = iValue;
        return(0);
    }
    if (iSelect == 'time')
    {
        NetPrintf(("protohttp: [0x%08x] setting timeout to %d ms\n", (uintptr_t)pState, iValue));
        pState->uTimeout = (unsigned)iValue;
        return(0);
    }
    // unhandled -- pass on to ProtoSSL
    return(ProtoSSLControl(pState->pSsl, iSelect, iValue, iValue2, pValue));
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
        'body'      negative=failed or pending, else size of body
        'code'      negative=no response, else server response code (ProtoHttpResponseE)
        'data'      negative=failed, zero=pending, positive=amnt of data ready
        'date'      last-modified date, if available
        'done'      negative=failed, zero=pending, positive=done
        'essl'      returns protossl error state
        'head'      negative=failed or pending, else size of header
        'host'      current host copied to output buffer
        'htxt'      current received http header text copied to output buffer
        'info'      copies most recent info header received, if any, to output buffer (one time only)
        'imax'      size of input buffer
        'iovr'      returns input buffer overflow size (valid on PROTOHTTP_MINBUFF result code)
        'plst'      return whether any piped requests were lost due to a premature close
        'port'      current port
        'rtxt'      most recent http request header text copied to output buffer
        'time'      TRUE if the client timed out the connection, else FALSE
    \endverbatim

    \Version 04/12/2000 (gschaefer) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpStatus(ProtoHttpRefT *pState, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    // return protossl address
    if ((iSelect == 'addr') && (pState->pSsl != NULL))
    {
        return(ProtoSSLStat(pState->pSsl, iSelect, pBuffer, iBufSize));
    }

    // return protossl error state (we cache this since we reset the state when we disconnect on error)
    if (iSelect == 'essl')
    {
        return(pState->iSslFail);
    }

    // return cert info
    if ((iSelect == 'cert') && (pState->pSsl != NULL))
    {
        return(ProtoSSLStat(pState->pSsl, iSelect, pBuffer, iBufSize));
    }

    // return if a CA fetch request is in progress
    if ((iSelect == 'cfip') && (pState->pSsl != NULL))
    {
        return(ProtoSSLStat(pState->pSsl, iSelect, NULL, 0));
    }

    // return current host
    if (iSelect == 'host')
    {
        strnzcpy(pBuffer, pState->strHost, iBufSize);
        return(0);
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

    // return piped requests lost status
    if (iSelect == 'plst')
    {
        return(pState->bPipedRequestsLost);
    }

    // return current port
    if (iSelect == 'port')
    {
        return(pState->iPort);
    }

    // return most recent http request header text
    if (iSelect == 'rtxt')
    {
        strnzcpy(pBuffer, pState->strRequestHdr, iBufSize);
        return(0);
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
                strnzcpy(pBuffer, pState->strHdr, iBufSize);
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
        return(pState->iHeadSize);
    if (iSelect == 'body')
        return(pState->iBodySize);
    if (iSelect == 'date')
        return(pState->iHdrDate);
    if (iSelect == 'htxt')
    {
        strnzcpy(pBuffer, pState->strHdr, iBufSize);
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
    char strHost[sizeof(pState->strHost)], strKind[6];
    int32_t iPort, iSecure;
    uint8_t bPortSpecified;

    // parse the url
    pUrl = _ProtoHttpParseUrl(pUrl, strKind, sizeof(strKind), strHost, sizeof(strHost), &iPort, &iSecure, &bPortSpecified);

    // refresh open status
    if (pState->bConnOpen && (ProtoSSLStat(pState->pSsl, 'stat', NULL, 0) <= 0))
    {
        NetPrintf(("protohttp: [0x%08x] check for keep-alive detected connection close\n", (uintptr_t)pState));
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

    // give time to comm module
    ProtoSSLUpdate(pState->pSsl);

    // check for timeout
    if ((pState->eState != ST_IDLE) && (pState->eState != ST_DONE) && (pState->eState != ST_FAIL))
    {
        if (NetTickDiff(NetTick(), pState->uTimer) >= 0)
        {
            NetPrintf(("protohttp: [0x%08x] server timeout (state=%d)\n", (uintptr_t)pState, pState->eState));
            pState->eState = ST_FAIL;
            pState->bTimeout = TRUE;
        }
    }

    // see if connection is complete
    if (pState->eState == ST_CONN)
    {
        iResult = ProtoSSLStat(pState->pSsl, 'stat', NULL, 0);
        if (iResult > 0)
        {
            pState->uTimer = NetTick() + pState->uTimeout;
            pState->eState = ST_SEND;
            pState->bConnOpen = TRUE;
        }
        if (iResult < 0)
        {
            NetPrintf(("protohttp: [0x%08x] ST_CONN got ST_FAIL (err=%d)\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            pState->iSslFail = ProtoSSLStat(pState->pSsl, 'fail', NULL, 0);
        }
    }

    // send buffered header+data to webserver
    if (pState->eState == ST_SEND)
    {
        if (((iResult = _ProtoHttpSendBuff(pState)) > 0) && (pState->iInpLen == 0))
        {
            #if DIRTYCODE_LOGGING
            // if we just sent a header, display header to debug output
            if (pState->iVerbose > 0)
            {
                int32_t iRequestType;
                for (iRequestType = 0; iRequestType < PROTOHTTP_NUMREQUESTTYPES; iRequestType += 1)
                {
                    if (!strncmp(pState->pInpBuf, _ProtoHttp_strRequestNames[iRequestType], strlen(_ProtoHttp_strRequestNames[iRequestType])))
                    {
                        char *pHdrEnd = pState->pInpBuf + pState->iHdrLen;
                        char cHdrChr = *pHdrEnd;
                        *pHdrEnd = '\0';
                        NetPrintf(("protohttp: [0x%08x] sent request:\n", (uintptr_t)pState));
                        NetPrintfVerbose((pState->iVerbose, 1, "protohttp: [0x%08x] tick=%u\n", (uintptr_t)pState, NetTick()));
                        NetPrintWrap(pState->pInpBuf, 80);
                        *pHdrEnd = cHdrChr;
                        break;
                    }
                }
            }
            #endif
            pState->iInpOff = 0;
            pState->iHdrOff = 0;
            pState->eState = ST_RESP;
        }
    }

    // wait for initial response
    if (pState->eState == ST_RESP)
    {
        // try for the first line of http response
        if ((iResult = _ProtoHttpRecvFirstHeaderLine(pState)) > 0)
        {
            // copy first line of header to input buffer and proceed
            strnzcpy(pState->pInpBuf, pState->strHdr, pState->iHdrOff+1);
            pState->iInpLen = pState->iHdrOff;
            pState->eState = ST_HEAD;
        }
        else if ((iResult < 0) && !_ProtoHttpRetrySendRequest(pState))
        {
            NetPrintf(("protohttp: [0x%08x] ST_HEAD byte0 got ST_FAIL (err=%d)\n", (uintptr_t)pState, iResult));
            pState->iInpLen = 0;
            pState->eState = ST_FAIL;
        }
    }

    // try to receive header data
    if (pState->eState == ST_HEAD)
    {
        // append data to buffer
        if ((iResult = _ProtoHttpRecv(pState, pState->pInpBuf+pState->iInpLen, pState->iInpMax - pState->iInpLen)) > 0)
        {
            pState->iInpLen += iResult;
        }
        // deal with network error (defer handling closed state to next block, to allow pipelined receives of already buffered transactions)
        if ((iResult < 0) && ((iResult != SOCKERR_CLOSED) || (pState->iInpLen <= 4)))
        {
            NetPrintf(("protohttp: [0x%08x] ST_HEAD append got ST_FAIL (err=%d, len=%d)\n", (uintptr_t)pState, iResult, pState->iInpLen));
            pState->eState = ST_FAIL;
        }
    }

    // see if header is complete
    if ((pState->eState == ST_HEAD) && (pState->iInpLen > 4))
    {
        // try parsing header
        if (_ProtoHttpParseHeader(pState) < 0)
        {
            // was there a prior SOCKERR_CLOSED error?
            if (pState->iRecvRslt < 0)
            {
                NetPrintf(("protohttp: [0x%08x] ST_HEAD append got ST_FAIL (err=%d, len=%d)\n", (uintptr_t)pState, pState->iRecvRslt, pState->iInpLen));
                pState->eState = ST_FAIL;
            }
            // if the buffer is full, we don't have enough room to receive the header
            if (pState->iInpLen == pState->iInpMax)
            {
                if (pState->iInpOvr == 0)
                {
                    NetPrintf(("protohttp: [0x%08x] input buffer too small for response header\n", (uintptr_t)pState));
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
        else if (PROTOHTTP_GetResponseClass(pState->iHdrCode) == PROTOHTTP_RESPONSE_REDIRECTION)
        {
            _ProtoHttpProcessRedirect(pState);
        }
    }

    // parse incoming body data
    while (pState->eState == ST_BODY)
    {
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
            iResult = 0;
        }
        else
        {
            // try to add to buffer
            iResult = _ProtoHttpRecv(pState, pState->pInpBuf+pState->iInpLen, iResult);
        }
        if (iResult == 0)
        {
            break;
        }

        // check for connection close
        if ((iResult == SOCKERR_CLOSED) && ((pState->iBodySize == -1) || (pState->iBodySize == pState->iInpCnt)))
        {
            if (!pState->bChunked)
            {
                pState->iBodySize = pState->iInpCnt;
            }
            pState->bCloseHdr = TRUE;
            pState->eState = ST_DONE;
            break;
        }

        // otherwise an error is fatal
        if (iResult < 0)
        {
            NetPrintf(("protohttp: [0x%08x] ST_FAIL (err=%d)\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            break;
        }

        // add to buffer
        pState->iInpLen += iResult;
        pState->iInpCnt += iResult;

        // check for end of body
        if ((pState->iBodySize >= 0) && (pState->iInpCnt >= pState->iBodySize))
        {
            pState->eState = ST_DONE;
            break;
        }
    }

    // force disconnect in failure state
    if (pState->eState == ST_FAIL)
    {
        _ProtoHttpClose(pState, "error");
    }

    // handle completion
    if (pState->eState == ST_DONE)
    {
        if (pState->bPipelining && (pState->iPipedRequests > 0))
        {
            // are we ready to proceed?
            if ((pState->iBodyRcvd == pState->iBodySize) && pState->bPipeGetNext)
            {
                NetPrintf(("protohttp: [0x%08x] handling piped request\n", (uintptr_t)pState));
                _ProtoHttpCompactBuffer(pState);
                pState->eState = ST_HEAD;
                pState->iHeadSize = pState->iBodySize = pState->iBodyRcvd = 0;
                pState->iPipedRequests -= 1;
                pState->bPipeGetNext = FALSE;
            }
        }
        else if (pState->bCloseHdr)
        {
            _ProtoHttpClose(pState, "server request");
        }
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlEncodeIntParm

    \Description
        Url-encode an integer parameter.

    \Input *pBuffer - [out] pointer to buffer to append parameter to
    \Input iLength  - length of buffer
    \Input *pParm   - pointer to parameter (not encoded)
    \Input iValue   - integer to encode

    \Output
        int32_t     - negative=failure, zero=success

    \Version 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpUrlEncodeIntParm(char *pBuffer, int32_t iLength, const char *pParm, int32_t iValue)
{
    char strValue[32];
    char *pData;

    // format the value
    snzprintf(strValue, sizeof(strValue), "%d", iValue);
    pData = strValue;

    // save room for null terminator
    iLength -= 1;

    // locate the append point
    for (; (*pBuffer != 0) && (iLength > 0); iLength--)
    {
        pBuffer++;
    }

    // add in the parameter (non encoded)
    for (; (*pParm != 0) && (iLength > 0); iLength--)
    {
        *pBuffer++ = *pParm++;
    }

    // add in the number
    for (; (*pData != 0) && (iLength > 0); iLength--)
    {
        *pBuffer++ = *pData++;
    }

    // null-terminate string
    *pBuffer = '\0';

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlEncodeStrParm

    \Description
        Url-encode a string parameter.

    \Input *pBuffer - [out] pointer to buffer to append parameter to
    \Input iLength  - length of buffer
    \Input *pParm   - pointer to url parameter (not encoded)
    \Input *pData   - string to encode

    \Output
        int32_t     - negative=failure, zero=success

    \Version 11/30/2004 (jbrookes) First Version
*/
/********************************************************************************F*/
int32_t ProtoHttpUrlEncodeStrParm(char *pBuffer, int32_t iLength, const char *pParm, const char *pData)
{
    // table of "safe" characters
    // 0=hex encode, non-zero=direct encode, @-O=valid hex digit (&15 to get value)
    static char _strSafe[256] =
        "0000000000000000" "0000000000000000"   // 0x00 - 0x1f
        "0000000000000000" "@ABCDEFGHI000000"   // 0x20 - 0x3f (allow digits)
        "0JKLMNO111111111" "1111111111100000"   // 0x40 - 0x5f (allow uppercase)
        "0JKLMNO111111111" "1111111111100000"   // 0x60 - 0x7f (allow lowercase)
        "0000000000000000" "0000000000000000"   // 0x80 - 0x9f
        "0000000000000000" "0000000000000000"   // 0xa0 - 0xbf
        "0000000000000000" "0000000000000000"   // 0xc0 - 0xdf
        "0000000000000000" "0000000000000000";  // 0xe0 - 0xff

    // hex translation table
    static char _strHex[16] = "0123456789ABCDEF";

    // save room for null terminator
    iLength -= 1;

    // locate the append point
    for (; (*pBuffer != 0) && (iLength > 0); iLength--)
    {
        pBuffer++;
    }

    // add in the parameter (non encoded)
    for (; (*pParm != 0) && (iLength > 0); iLength--)
    {
        *pBuffer++ = *pParm++;
    }

    // add in the encoded data
    for (; (*pData != 0) && (iLength > 2); )
    {
        // see how we need to encode this
        if (_strSafe[*(uint8_t *)pData] == '0')
        {
            uint8_t ch = *pData++;
            *pBuffer++ = '%';
            *pBuffer++ = _strHex[ch>>4];
            *pBuffer++ = _strHex[ch&15];
            iLength -= 3;
        }
        else
        {
            *pBuffer++ = *pData++;
            iLength--;
        }
    }

    // make sure to add the extra data if a hex encode isn't necessary for the last bytes of pData
    for (; (*pData != 0) && (iLength > 0) && (_strSafe[*(uint8_t *)pData] != '0'); iLength--)
    {
        *pBuffer++ = *pData++;
    }

    // null-terminate string
    *pBuffer = '\0';

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUrlParse

    \Description
        Parses a Url into component parts.

    \Input *pUrl        - Url to parse
    \Input *pKind       - [out] http request kind
    \Input iKindSize    - size of kind output buffer
    \Input *pHost       - [out] host name
    \Input iHostSize    - size of host output buffer
    \Input *pPort       - [out] request port
    \Input *pSecure     - [out] request security

    \Output
        const char *    - pointer past end of parsed Url

    \Version 07/01/2009 (jbrookes) First Version
*/
/********************************************************************************F*/
const char *ProtoHttpUrlParse(const char *pUrl, char *pKind, int32_t iKindSize, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure)
{
    uint8_t bPortSpecified;
    // parse the url into component parts
    return(_ProtoHttpParseUrl(pUrl, pKind, iKindSize, pHost, iHostSize, pPort, pSecure, &bPortSpecified));
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
    return(ProtoSSLSetCACert(pCACert, iCertSize));
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
    return(ProtoSSLSetCACert2(pCACert, iCertSize));
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
    return(ProtoSSLValidateAllCA());
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
    ProtoSSLClrCACerts();
}
