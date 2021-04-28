/*H********************************************************************************/
/*!
    \File protohttpxenon.c

    \Description
        TODO

    \Copyright
        Copyright (c) Electronic Arts 2012

    \Version 1.0 12/06/2011 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifdef DIRTYSDK_XHTTP_ENABLED

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xhttp.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "dirtyvers.h"
#include "dirtyerr.h"
#include "dirtyauthxenon.h"

// this 1st include of protohttpxenon/protohttp defines the platform-independent version 'namespaced' with underscores
#include "protohttpxenon.h"
#include "protohttp.h"

// this 2nd include of protohttpxenon/protohttp defines the platform-dependent version with the platform-independent namespace
#include "protohttpxenon.h"
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
typedef struct ProtoHttpRefT
{
    __ProtoHttpRefT *pProtoHttp;

    HINTERNET hHttpConnect, hHttpSession, hHttpRequest;
    XHTTP_STATUS_CALLBACK hHttpCallback;

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
        ST_AUTH,            //!< waiting for auth request to complete
        ST_CONN,            //!< connecting state
        ST_SEND,            //!< sending buffered data
        ST_RESP,            //!< waiting for initial response (also sending any data not buffered if POST or PUT)
        ST_HEAD,            //!< getting header
        ST_BODY,            //!< getting body
        ST_DONE,            //!< transaction success
        ST_FAIL             //!< transaction failed
    } eState;               //!< current state

    int32_t iHdrCode;       //!< result code
    int32_t iHdrDate;       //!< last modified date

    int32_t iPostSize;      //!< amount of data being sent in a POST or PUT operation
    int32_t iHeadSize;      //!< size of head data
    int32_t iBodySize;      //!< size of body data
    int32_t iBodyRcvd;      //!< size of body data received by caller
    int32_t iRecvSize;      //!< amount of data received by ProtoHttpRecvAll
    int32_t iRecvRslt;      //!< last recv result
    int32_t iSendRslt;      //!< last send result
    int32_t iSentSize;      //!< total amount of data sent

    char *pInpBuf;          //!< input buffer
    int32_t iInpMax;        //!< maximum buffer size
    int32_t iInpOff;        //!< offset into buffer
    int32_t iInpLen;        //!< total length
    int32_t iInpCnt;        //!< ongoing count
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
    char strKind[16];       //!< request kind (http/https)
    char strHost[256];      //!< server name
    char strBaseHost[256];  //!< base server name (used for partial urls)
    char strXhttpRecvBuf[1024];
    char strUserAgent[64];

    uint8_t bTimeout;       //!< boolean indicating whether a timeout occurred or not
    uint8_t bCloseHdr;      //!< server wants close after this
    uint8_t bClosed;        //!< connection has been closed
    uint8_t bConnOpen;      //!< connection is open
    uint8_t iVerbose;       //!< debug output verbosity
    uint8_t bHttp1_0;       //!< TRUE if HTTP/1.0, else FALSE
    uint8_t bCompactRecv;   //!< compact receive buffer
    uint8_t bInfoHdr;       //!< TRUE if a new informational header has been cached; else FALSE
    uint8_t bRecvInProgress; //!< async read is in progress
    uint8_t bSendInProgress; //!< async send is in progress
    uint8_t bRecvComplete;  //!< async read has returned zero, meaning the transfer is complete
    uint8_t bSendComplete;  //!< async send has returned zero, meaning the transfer is complete
    uint8_t bHeadAvail;     //!< TRUE when header is available, else FALSE

} ProtoHttpRefT;

/*** Function Prototypes **********************************************************/

static int32_t _ProtoHttpGetHeader(ProtoHttpRefT *pState);

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
    \Function _ProtoHttpXHttpStatusCallback

    \Description
        Close an XHttp handle

    \Input hInternet    - handle to associate with status callback
    \Input dwContext    - protohttp module state
    \Input dwInternetStatus - callback status
    \Input *pvStatusInformation - unused
    \Input dwStatusInformationLength - unused

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpXHttpStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, void *pvStatusInformation, DWORD dwStatusInformationLength)
{
    ProtoHttpRefT *pState = (ProtoHttpRefT *)dwContext;

    #if DIRTYCODE_LOGGING
    static DirtyErrT _XHttpCallbackStatusList[] =
    {
        DIRTYSOCK_ErrorName(XHTTP_CALLBACK_STATUS_HANDLE_CLOSING),                            // 1
        DIRTYSOCK_ErrorName(XHTTP_CALLBACK_STATUS_REDIRECT),                               // 5
        DIRTYSOCK_ErrorName(XHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE),
        DIRTYSOCK_ErrorName(XHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE),
        DIRTYSOCK_ErrorName(XHTTP_CALLBACK_STATUS_READ_COMPLETE),
        DIRTYSOCK_ErrorName(XHTTP_CALLBACK_STATUS_WRITE_COMPLETE),
        DIRTYSOCK_ErrorName(XHTTP_CALLBACK_STATUS_REQUEST_ERROR),
        DIRTYSOCK_ErrorName(XHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE),
        DIRTYSOCK_ListEnd()
    };
    static DirtyErrT _XHttpCallbackResultList[] = 
    {
        DIRTYSOCK_ErrorName(API_RECEIVE_RESPONSE),
        DIRTYSOCK_ErrorName(API_QUERY_DATA_AVAILABLE),
        DIRTYSOCK_ErrorName(API_READ_DATA),
        DIRTYSOCK_ErrorName(API_WRITE_DATA),
        DIRTYSOCK_ErrorName(API_SEND_REQUEST),
        DIRTYSOCK_ListEnd()
    };
    #endif

    NetPrintfVerbose((pState->iVerbose, 1, "protohttpxenon: XHttpCallback(%s) len=%d\n", DirtyErrGetNameList(dwInternetStatus, _XHttpCallbackStatusList), dwStatusInformationLength));

    if (dwInternetStatus == XHTTP_CALLBACK_STATUS_HANDLE_CLOSING)
    {
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxenon: handle 0x%08x closed\n", (uintptr_t)pvStatusInformation));
    }
    if (dwInternetStatus == XHTTP_CALLBACK_STATUS_REDIRECT)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpxenon: redirecting to '%s'\n", (char *)pvStatusInformation));
        if (pState->pReceiveHeaderCb != NULL)
        {
            _ProtoHttpGetHeader(pState);
        }
    }
    if (dwInternetStatus == XHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpxenon: [0x%08x] send request complete\n", (uintptr_t)pState));
        // send is no longer in progress
        pState->bSendInProgress = FALSE;
    }
    if (dwInternetStatus == XHTTP_CALLBACK_STATUS_WRITE_COMPLETE)
    {
        DWORD dwWriteLen = *(DWORD *)pvStatusInformation;
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxenon: [0x%08x] write complete (len=%d)\n", (uintptr_t)pState, dwWriteLen));
        // remember send result
        pState->iSendRslt = dwWriteLen;
        // send is no longer in progress
        pState->bSendInProgress = FALSE;
        // update sent size
        pState->iSentSize += pState->iSendRslt;
    }
    if (dwInternetStatus == XHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpxenon: [0x%08x] headers available\n", (uintptr_t)pState));
        pState->bHeadAvail = TRUE;
        if (pState->eState != ST_HEAD)
        {
            NetPrintf(("protohttpxenon: [0x%08x] warning; got unexpected result %s when in state %d\n", (uintptr_t)pState,
                DirtyErrGetNameList(dwInternetStatus, _XHttpCallbackStatusList), pState->eState));
        }
    }
    if (dwInternetStatus == XHTTP_CALLBACK_STATUS_READ_COMPLETE)
    {
        if ((pState->iRecvRslt = dwStatusInformationLength) == 0)
        {
            // a zero result means the transaction is complete
            pState->bRecvComplete = TRUE;
        }
        // receive is no longer in progress
        pState->bRecvInProgress = FALSE;
    }
    if (dwInternetStatus == XHTTP_CALLBACK_STATUS_REQUEST_ERROR)
    {
        #if DIRTYCODE_LOGGING
        XHTTP_ASYNC_RESULT *pResult = (XHTTP_ASYNC_RESULT *)pvStatusInformation;
        NetPrintf(("protohttpxenon: [0x%08x] dwResult=%s, dwError=%s\n", (uintptr_t)pState, DirtyErrGetNameList(pResult->dwResult, _XHttpCallbackResultList), DirtyErrGetName(pResult->dwError)));
        #endif

        //NetPrintf(("protohttpxenon: [0x%08x] server timeout (state=%d)\n", (uintptr_t)pState, pState->eState));
        pState->eState = ST_FAIL;
        //pState->bTimeout = TRUE;

        //$$TODO - distinguish different errors appropriately
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpCloseHandle

    \Description
        Close an XHttp handle

    \Input *pState  - module state
    \Input *pHandle - pointer to handle to close
    \Input *pName   - name of handle (for debug output)

    \Version 01/26/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpCloseHandle(ProtoHttpRefT *pState, HINTERNET *pHandle, const char *pName)
{
    if ((*pHandle != NULL) && (XHttpCloseHandle(*pHandle) == FALSE))
    {
        NetPrintf(("protohttpxenon: [0x%08x] XHttpCloseHandle(%s) failed (err=%s)\n", (uintptr_t)pState, pName,
            DirtyErrGetName(GetLastError())));
    }
    *pHandle = NULL;
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpClose

    \Description
        Close connection to server, if open.

    \Input *pState      - module state
    \Input *pReason     - reason connection is being closed (for debug output)

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpClose(ProtoHttpRefT *pState, const char *pReason)
{
    if (pState->bClosed)
    {
        // already issued disconnect, don't need to do it again
        return;
    }

    NetPrintf(("protohttpxenon: [0x%08x] closing connection: %s\n", (uintptr_t)pState, pReason));
    pState->bCloseHdr = FALSE;
    pState->bConnOpen = FALSE;
    pState->bClosed = TRUE;

    _ProtoHttpCloseHandle(pState, &pState->hHttpRequest, "hHttpRequest");
    _ProtoHttpCloseHandle(pState, &pState->hHttpConnect, "hHttpConnect");
    _ProtoHttpCloseHandle(pState, &pState->hHttpSession, "hHttpSession");
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpReset

    \Description
        Reset state before a transaction request.

    \Input  *pState     - reference pointer

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpReset(ProtoHttpRefT *pState)
{
    memset(pState->strHdr, 0, sizeof(pState->strHdr));
    memset(pState->strRequestHdr, 0, sizeof(pState->strRequestHdr));
    pState->eState = ST_IDLE;
    pState->iHdrCode = -1;
    pState->iHdrDate = 0;
    pState->iHeadSize = 0;
    pState->iBodySize = pState->iBodyRcvd = 0;
    pState->iRecvSize = 0;
    pState->iSentSize = 0;
    pState->iInpOff = 0;
    pState->iInpLen = 0;
    pState->iInpCnt = 0;
    pState->bTimeout = FALSE;
    pState->bClosed = FALSE;
    pState->bRecvInProgress = FALSE;
    pState->bRecvComplete = FALSE;
    pState->bHeadAvail = FALSE;
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

    \Version 12/06/2011 (jbrookes)
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
    \Function _ProtoHttpRequest

    \Description
        Issue XHttp request

    \Input *pState      - module state

    \Output
        int32_t         - negative=failure, else success

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRequest(ProtoHttpRefT *pState)
{
    // parse out url components
    const char *pUrl = ProtoHttpUrlParse(pState->pInpBuf, pState->strKind, sizeof(pState->strKind), pState->strHost, sizeof(pState->strHost), &pState->iPort, &pState->iSecure);
    int32_t iAppendLen = (pState->pAppendHdr) ? strlen(pState->pAppendHdr) : 0;

    // if no url specified use the minimum
    if (*pUrl == '\0')
    {
        pUrl = "/";
    }

    // open an xhttp session
    if ((pState->hHttpSession = XHttpOpen(pState->strUserAgent, XHTTP_ACCESS_TYPE_NO_PROXY, XHTTP_NO_PROXY_NAME, XHTTP_NO_PROXY_BYPASS, XHTTP_FLAG_ASYNC)) == NULL)
    {
        NetPrintf(("protohttpxenon: could not open http session (err=%s)\n", DirtyErrGetName(GetLastError())));
        return(-1);
    }
    // connect to remote host
    if ((pState->hHttpConnect = XHttpConnect(pState->hHttpSession, pState->strHost, pState->iPort, pState->iSecure ? XHTTP_FLAG_SECURE : 0)) == NULL)
    {
        NetPrintf(("protohttpxenon: could not create https connection (err=%s)\n", DirtyErrGetName(GetLastError())));
        return(-2);
    }
    // open request
    NetPrintf(("protohttpxenon: %s %s\n", _ProtoHttp_strRequestNames[pState->eRequestType], pUrl));
    if ((pState->hHttpRequest = XHttpOpenRequest(pState->hHttpConnect, _ProtoHttp_strRequestNames[pState->eRequestType], pUrl, NULL, NULL/*XHTTP_NO_REFERER*/, NULL, 0)) == NULL)
    {
        NetPrintf(("protohttpxenon: could not open https request (err=%s)\n", DirtyErrGetName(GetLastError())));
        return(-3);
    }

    // set status callback
    XHttpSetStatusCallback(pState->hHttpRequest, _ProtoHttpXHttpStatusCallback, XHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, (DWORD_PTR)pState);

    // set max redirects
    if (!XHttpSetOption(pState->hHttpRequest, XHTTP_OPTION_MAX_HTTP_AUTOMATIC_REDIRECTS, &pState->iMaxRedirect, sizeof(pState->iMaxRedirect)))
    {
        NetPrintf(("protohttpxenon: error %s trying to set max redirection to %d\n", DirtyErrGetName(GetLastError()), pState->iMaxRedirect));
    }
    // set keep-alive disabled (enabled by default)
    if (pState->iKeepAlive == 0)
    {
        DWORD dwOption = XHTTP_DISABLE_KEEP_ALIVE;
        if (!XHttpSetOption(pState->hHttpRequest, XHTTP_OPTION_DISABLE_FEATURE, &dwOption, sizeof(dwOption)))
        {
            NetPrintf(("protohttpxenon: error %s trying to disable keep-alive\n", DirtyErrGetName(GetLastError())));
        }
    }

    // send the request
    if (iAppendLen > 0)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "protohttpxenon: sending with append header:\n"));
        NetPrintWrap(pState->pAppendHdr, 132);
    }
    if (!XHttpSendRequest(pState->hHttpRequest, pState->pAppendHdr, iAppendLen, NULL, 0, pState->iPostSize, (DWORD_PTR)pState))
    {
        NetPrintf(("protohttpxenon: could not send https request (err=%s)\n", DirtyErrGetName(GetLastError())));
        return(-4);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpParseHeader

    \Description
        Parse incoming HTTP header.  The HTTP header is presumed to be at the
        beginning of the input buffer.

    \Input *pState      - reference pointer
    \Input *pStrHdr     - header to parse
    \Input iHdrLen      - length of header text

    \Output
        int32_t         - negative=not ready or error, else success

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpParseHeader(ProtoHttpRefT *pState, char *pStrHdr, int32_t iHdrLen)
{
    DWORD dwBufferLength = sizeof(pState->iHdrCode);
    char strTemp[128];

    #if DIRTYCODE_LOGGING
    if (pState->iVerbose > 0)
    {
        NetPrintf(("protohttpxenon: [0x%08x] received response header (%d bytes):\n", (uintptr_t)pState, iHdrLen));
        NetPrintWrap(pStrHdr, 80);
    }
    #endif

    // save header size
    pState->iHeadSize = iHdrLen;

    // parse header code
    if (XHttpQueryHeaders(pState->hHttpRequest, XHTTP_QUERY_STATUS_CODE|XHTTP_QUERY_FLAG_NUMBER, XHTTP_HEADER_NAME_BY_INDEX, (DWORD *)&pState->iHdrCode, &dwBufferLength, XHTTP_NO_HEADER_INDEX) != TRUE)
    {
        NetPrintf(("protohttpxenon: could not get response code err=%s\n", DirtyErrGetName(GetLastError())));
    }

    // parse content-length field
    if ((__ProtoHttpGetHeaderValue(NULL, pStrHdr, "content-length", strTemp, sizeof(strTemp), NULL)) != -1)
    {
        pState->iBodySize = strtol(strTemp, NULL, 10);
    }
    else
    {
        pState->iBodySize = -1;
    }

    // parse last-modified header
    if ((__ProtoHttpGetHeaderValue(NULL, pStrHdr, "last-modified", strTemp, sizeof(strTemp), NULL)) != -1)
    {
        pState->iHdrDate = strtotime(strTemp);
    }
    else
    {
        pState->iHdrDate = 0;
    }

    // parse connection header
    if (pState->bCloseHdr == FALSE)
    {
        __ProtoHttpGetHeaderValue(NULL, pStrHdr, "connection", strTemp, sizeof(strTemp), NULL);
        pState->bCloseHdr = !ds_stricmp(strTemp, "close");
    }

    // note if it is an informational header
    pState->bInfoHdr = PROTOHTTP_GetResponseClass(pState->iHdrCode) == PROTOHTTP_RESPONSE_INFORMATIONAL;

    // trigger recv header user callback, if specified
    if (pState->pReceiveHeaderCb != NULL)
    {
        pState->pReceiveHeaderCb(pState, pStrHdr, iHdrLen, pState->pCallbackRef);
    }

    // header successfully parsed
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpGetHeader

    \Description
        Get header from Xhttp

    \Input *pState      - reference pointer

    \Output
        int32_t         - zero=failure, else success

    \Version 02/27/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpGetHeader(ProtoHttpRefT *pState)
{
    DWORD dwHeaderLen = sizeof(pState->strHdr);
    BOOL bResult;
    if ((bResult = XHttpQueryHeaders(pState->hHttpRequest, XHTTP_QUERY_RAW_HEADERS_CRLF, XHTTP_HEADER_NAME_BY_INDEX, pState->strHdr, &dwHeaderLen, NULL)) == TRUE)
    {
        // parse the header
        _ProtoHttpParseHeader(pState, pState->strHdr, dwHeaderLen);
    }
    else
    {
        NetPrintf(("protohttpxenon: [0x%08x] XHttpQueryHeaders() failed (err=%s)\n", (uintptr_t)pState,
            DirtyErrGetName(GetLastError())));
    }
    return((int32_t)bResult);
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
    \Function _ProtoHttpRecv

    \Description
        Try and receive some data.  With XHttp, a receive request is asynchronous
        with the result being given by callback.

    \Input *pState      - reference pointer
    \Input *pStrBuf     - [out] pointer to buffer to receive into
    \Input iSize        - amount of data to try and receive

    \Output
        int32_t         - negative=error, else success

    \Version 12/12/2011 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpRecv(ProtoHttpRefT *pState, char *pStrBuf, int32_t iSize)
{
    int32_t iReadSize, iRequestSize;

    // if we have no buffer space, or a receive is in progress, don't try to get data
    for (iReadSize = 0; (pState->bRecvInProgress == FALSE) && (pState->bRecvComplete == FALSE); )
    {
        // check for data
        if (pState->iRecvRslt > 0)
        {
            // copy data to output buffer
            memcpy(pStrBuf, pState->strXhttpRecvBuf, pState->iRecvRslt);
            iReadSize += pState->iRecvRslt;

            // update output buffer
            pStrBuf += pState->iRecvRslt;
            iSize -= pState->iRecvRslt;

            // clear read status
            pState->iRecvRslt = 0;
        }

        // if there's room, request more data
        if (iSize > 0)
        {
            NetPrintfVerbose((pState->iVerbose, 2, "protohttpxenon: [0x%08x] XHttpReadData[0x%08x,%d]\n", (uintptr_t)pState, (uintptr_t)pState->strXhttpRecvBuf, sizeof(pState->strXhttpRecvBuf)));
            /* mark that a receive is in progress, and issue the request.  note
               that the XHttpReadData() call can itself trigger the read data
               complete callback, not just XHttpDoWork(), so we set in progress
               prior to calling the function */
            iRequestSize = (iSize > sizeof(pState->strXhttpRecvBuf)) ? sizeof(pState->strXhttpRecvBuf) : iSize;
            pState->bRecvInProgress = TRUE;
            if (!XHttpReadData(pState->hHttpRequest, pState->strXhttpRecvBuf, iRequestSize, NULL))
            {
                NetPrintf(("protohttpxenon: [0x%08x] error %s trying to read data\n", (uintptr_t)pState,
                    DirtyErrGetName(GetLastError())));
                //$$todo - provide better error mapping to SOCKERR_*
                return(-1);
            }
        }
        else
        {
            break;
        }
    }

    // did we get anything?
    if (iReadSize > 0)
    {
        #if DIRTYCODE_LOGGING
        if (pState->iVerbose > 1)
        {
            NetPrintf(("protohttpxenon: [0x%08x] recv %d bytes\n", (uintptr_t)pState, iReadSize));
        }
        if (pState->iVerbose > 2)
        {
            NetPrintMem(pState->pInpBuf+pState->iInpLen, iReadSize, "http-recv");
        }
        #endif

        // received data, so update timeout
        pState->uTimer = NetTick() + pState->uTimeout;
    }

    return(iReadSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoHttpSendBuff

    \Description
        Send data queued in buffer.

    \Input  *pState     - reference pointer

    \Version 02/22/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoHttpSendBuff(ProtoHttpRefT *pState)
{
    int32_t iSentSize = 0, iSentLeft;

    // if we have no buffer space, or a receive is in progress, don't try to get data
    for (iSentSize = 0; (pState->bSendInProgress == FALSE) /*&& (pState->bRecvComplete == FALSE)*/; )
    {
        // check for data send completion
        if (pState->iSendRslt > 0)
        {
            #if DIRTYCODE_LOGGING
            NetPrintfVerbose((pState->iVerbose, 1, "protohttpxenon: [0x%08x] sent %d bytes\n", (uintptr_t)pState, pState->iSendRslt));
            if (pState->iVerbose > 2)
            {
                NetPrintMem(pState->pInpBuf+pState->iInpOff, pState->iSendRslt, "http-recv");
            }
            #endif

            // track sent size
            iSentSize += pState->iSendRslt;

            // if not all of the data was sent, compact the buffer
            if ((iSentLeft = pState->iInpLen - pState->iSendRslt) > 0)
            {
                memmove(pState->pInpBuf, pState->pInpBuf+pState->iSendRslt, iSentLeft);
            }
            pState->iInpLen -= pState->iSendRslt;

            // clear send status
            pState->iSendRslt = 0;

            // sent data, so update timeout
            pState->uTimer = NetTick() + pState->uTimeout;
        }

        // if there's more data, send it
        if (pState->iInpLen > 0)
        {
            NetPrintfVerbose((pState->iVerbose, 1, "protohttpxenon: [0x%08x] XHttpWriteData[0x%08x,%d]\n", (uintptr_t)pState, (uintptr_t)pState->pInpBuf, pState->iInpLen));
            /* mark that a send is in progress, and issue the request.  note
               that the XHttpWriteData() call can itself trigger the read data (TODO - VERIFY?)
               complete callback, not just XHttpDoWork(), so we set in progress
               prior to calling the function */
            pState->bSendInProgress = TRUE;
            if (!XHttpWriteData(pState->hHttpRequest, pState->pInpBuf, pState->iInpLen, NULL))
            {
                NetPrintf(("protohttp: [0x%08x] error %s tryping to send data\n", (uintptr_t)pState, DirtyErrGetName(GetLastError())));
                pState->iInpLen = 0;
                pState->eState = ST_FAIL;
                iSentSize = -1;
                break;
            }
        }
        else
        {
            break;
        }
    }
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

    \Version 02/22/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoHttpSend(ProtoHttpRefT *pState, const char *pStrBuf, int32_t iSize)
{
    int32_t iInpAvail, iSent = 0;

    // add data to buffer if there is room
    if ((iInpAvail = pState->iInpMax - pState->iInpLen) > 0)
    {
        if (iSize > iInpAvail)
        {
            iSize = iInpAvail;
        }
        NetPrintfVerbose((pState->iVerbose, 1, "protohttpxenon: buffering %d bytes\n", iSize));
        memcpy(pState->pInpBuf+pState->iInpLen, pStrBuf, iSize);
        pState->iInpLen += iSize;
        iSent = iSize;
    }

    // return data buffered
    return(iSent);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoHttpCreate

    \Description
        Allocate module state and prepare for use

    \Input iBufSize     - length of receive buffer

    \Output
        __ProtoHttpRefT * - pointer to module state, or NULL

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
ProtoHttpRefT *ProtoHttpCreate(int32_t iBufSize)
{
    ProtoHttpRefT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate the resources
    if ((pState = DirtyMemAlloc(sizeof(*pState), PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpxenon: [0x%08x] unable to allocate module state\n", (uintptr_t)pState));
        return(NULL);
    }
    memset(pState, 0, sizeof(*pState));

    // allocate input buffer
    if ((pState->pInpBuf = DirtyMemAlloc(iBufSize, PROTOHTTP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protohttpxenon: [0x%08x] unable to allocate input buffer\n", (uintptr_t)pState));
        ProtoHttpDestroy(pState);
        return(NULL);
    }

    // save parms & set defaults
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->eState = ST_IDLE;
    pState->iInpMax = iBufSize;
    pState->uTimeout = PROTOHTTP_TIMEOUT;
    pState->iVerbose = 1;
    pState->iMaxRedirect = PROTOHTTP_MAXREDIRECT;

    // format user-agent string
    snzprintf(pState->strUserAgent, sizeof(pState->strUserAgent), "ProtoHttp %d.%d/DS %d.%d.%d.%d (" DIRTYCODE_PLATNAME ")",
        (PROTOHTTP_VERSION>>8)&0xff, PROTOHTTP_VERSION&0xff,
        (DIRTYVERS>>24)&0xff, (DIRTYVERS>>16)&0xff,
        (DIRTYVERS>>8)&0xff, DIRTYVERS&0xff);

    // allocate ProtoHttp ref (we default to normal ProtoHttp)
    pState->pProtoHttp = __ProtoHttpCreate(iBufSize);
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpCallback(ProtoHttpRefT *pState, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pCallbackRef)
{
    if (pState->pProtoHttp != NULL)
    {
        __ProtoHttpCallback(pState->pProtoHttp, (__ProtoHttpCustomHeaderCbT *)pCustomHeaderCb, (__ProtoHttpReceiveHeaderCbT *)pReceiveHeaderCb, pCallbackRef);
    }
    else if (pCustomHeaderCb != NULL)
    {
        NetPrintf(("protohttpxenon: [0x%08x] custom header callbacks are not supported by XHttp; specification of callback %p will be ignored\n", pCustomHeaderCb));
    }
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpDestroy(ProtoHttpRefT *pState)
{
    if (pState->pProtoHttp != NULL)
    {
        __ProtoHttpDestroy(pState->pProtoHttp);
    }
    _ProtoHttpClose(pState, "destroy");
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGet(ProtoHttpRefT *pState, const char *pUrl, uint32_t bHeadOnly)
{
    return(ProtoHttpRequest(pState, pUrl, NULL, 0, bHeadOnly ? PROTOHTTP_REQUESTTYPE_HEAD : PROTOHTTP_REQUESTTYPE_GET));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpRecv(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufMin, int32_t iBufMax)
{
    int32_t iLen;

    if (pState->pProtoHttp != NULL)
    {
        return(__ProtoHttpRecv(pState->pProtoHttp, pBuffer, iBufMin, iBufMax));
    }

    // early out for failure result
    if (pState->eState == ST_FAIL)
    {
        return(PROTOHTTP_RECVFAIL);
    }
    // waiting for data
    if ((pState->eState != ST_BODY) && (pState->eState != ST_DONE))
    {
        return(PROTOHTTP_RECVWAIT);
    }
    // if they only wanted head, thats all they get
    if (pState->eRequestType == PROTOHTTP_REQUESTTYPE_HEAD)
    {
        return(PROTOHTTP_RECVHEAD);
    }

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
            memcpy(pBuffer, pState->pInpBuf+pState->iInpOff, iLen);

            #if DIRTYCODE_LOGGING
            if (pState->iVerbose > 2)
            {
                NetPrintf(("protohttpxenon: [0x%08x] read %d bytes\n", (uintptr_t)pState, iLen));
            }
            if (pState->iVerbose > 3)
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
    \Function ProtoHttpRecvAll

    \Description
        Return all of the url data.

    \Input *pState  - reference pointer
    \Input *pBuffer - buffer to store data in
    \Input iBufSize - size of buffer

    \Output
        int32_t     - PROTOHTTP_RECV*, or positive=bytes in response

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpRecvAll(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufSize)
{
    int32_t iRecvMax, iRecvResult;

    if (pState->pProtoHttp != NULL)
    {
        return(__ProtoHttpRecvAll(pState->pProtoHttp, pBuffer, iBufSize));
    }

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
        NetPrintf(("protohttpxenon: [0x%08x] error %d receiving response\n", (uintptr_t)pState, iRecvResult));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpPost(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int32_t iDataSize, uint32_t bDoPut)
{
    return(ProtoHttpRequest(pState, pUrl, pData, iDataSize, bDoPut ? PROTOHTTP_REQUESTTYPE_PUT : PROTOHTTP_REQUESTTYPE_POST));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpSend(ProtoHttpRefT *pState, const char *pData, int32_t iDataSize)
{
    // handle protohttp request
    if (pState->pProtoHttp != NULL)
    {
        return(__ProtoHttpSend(pState->pProtoHttp, pData, iDataSize));
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

    // send the data
    return(_ProtoHttpSend(pState, pData, iDataSize));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpDelete(ProtoHttpRefT *pState, const char *pUrl)
{
    return(ProtoHttpRequest(pState, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_DELETE));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpOptions(ProtoHttpRefT *pState, const char *pUrl)
{
    return(ProtoHttpRequest(pState, pUrl, NULL, 0, PROTOHTTP_REQUESTTYPE_OPTIONS));
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpRequestCb

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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpRequestCb(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int32_t iDataSize, ProtoHttpRequestTypeE eRequestType, ProtoHttpWriteCbT *pWriteCb, void *pWriteCbUserData)
{
    int32_t iResult;

    // check for 'normal' request
    if (pState->pProtoHttp != NULL)
    {
        return(__ProtoHttpRequestCb(pState->pProtoHttp, pUrl, pData, iDataSize, eRequestType, pWriteCb, pWriteCbUserData));
    }

    // reset state for new request
    if (pState->eState != ST_IDLE)
    {
        _ProtoHttpReset(pState);
    }

    // save url to input buffer for later issue
    strnzcpy(pState->pInpBuf, pUrl, pState->iInpMax);

    // save request type
    pState->eRequestType = eRequestType;

    // close previous request, if there was one
    _ProtoHttpCloseHandle(pState, &pState->hHttpRequest, "hHttpRequest");
    _ProtoHttpCloseHandle(pState, &pState->hHttpConnect, "hHttpConnect");
    _ProtoHttpCloseHandle(pState, &pState->hHttpSession, "hHttpSession");

    // parse the url to get secure state
    ProtoHttpUrlParse(pUrl, pState->strKind, sizeof(pState->strKind), pState->strHost, sizeof(pState->strHost), &pState->iPort, &pState->iSecure);

    // request token if in secure mode
    if (pState->iSecure)
    {
        // make token request
        if ((iResult = DirtyAuthGetToken(pState->strHost, FALSE)) == DIRTYAUTH_PENDING)
        {
            // transition to authenticating state, return success
            pState->eState = ST_AUTH;
        }
        else if (iResult == DIRTYAUTH_SUCCESS)
        {
            // token is cached and already available, so we proceed directly to connection stage
            pState->eState = ST_CONN;
        }
    }
    else
    {
        pState->eState = ST_CONN;
    }

    // set post size
    pState->iPostSize = ((eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (eRequestType == PROTOHTTP_REQUESTTYPE_POST)) ? iDataSize : 0;

    // set connection timeout
    pState->uTimer = NetTick() + pState->uTimeout;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpAbort

    \Description
        Abort current operation, if any.

    \Input *pState      - reference pointer

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpAbort(ProtoHttpRefT *pState)
{
    if (pState->pProtoHttp != NULL)
    {
        __ProtoHttpAbort(pState->pProtoHttp);
    }
    //$$TODO - can Xhttp abort?
    NetPrintf(("protohttpxenon: [0x%08x] ProtoHttpAbort() unhandled\n", (uintptr_t)pState));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetHeaderValue(ProtoHttpRefT *pState, const char *pHdrBuf, const char *pName, char *pBuffer, int32_t iBufSize, const char **pHdrEnd)
{
    return(__ProtoHttpGetHeaderValue(pState->pProtoHttp, pHdrBuf, pName, pBuffer, iBufSize, pHdrEnd));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpGetNextHeader(ProtoHttpRefT *pState, const char *pHdrBuf, char *pName, int32_t iNameSize, char *pValue, int32_t iValSize, const char **pHdrEnd)
{
    return(__ProtoHttpGetNextHeader(pState->pProtoHttp, pHdrBuf, pName, iNameSize, pValue, iValSize, pHdrEnd));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpSetBaseUrl(ProtoHttpRefT *pState, const char *pUrl)
{
    if (pState->pProtoHttp != NULL)
    {
        __ProtoHttpSetBaseUrl(pState->pProtoHttp, pUrl);
    }
    //$$todo - implement for Xhttp
    NetPrintf(("protohttpxenon: [0x%08x] ProtoHttpSetBaseUrl() unhandled\n", (uintptr_t)pState));
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
        Selectors are (Xhttp only):

        \verbatim
            SELECTOR    DESCRIPTION
            'xhtp'      Select XHTTP (TRUE) or ProtoHttp (FALSE) (default=FALSE)
        \endverbatim

        In default (non-Xhttp) mode, selectors are passed through to ProtoHttp

    \Version 12/06/2011 (jbrookes)
*/
/*******************************************************************************F*/
int32_t ProtoHttpControl(ProtoHttpRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    // enable/disable xhttp use (must come first)
    if (iSelect == 'xhtp')
    {
        if ((iValue != 0) && (pState->pProtoHttp != NULL))
        {
            __ProtoHttpDestroy(pState->pProtoHttp);
            pState->pProtoHttp = NULL;
        }
        else if ((iValue == 0) && (pState->pProtoHttp == NULL))
        {
            pState->pProtoHttp = __ProtoHttpCreate(pState->iInpMax);
        }
        NetPrintf(("protohttpxenon: xhttp %s\n", (pState->pProtoHttp == NULL) ? "enabled" : "disabled"));
        return(0);
    }
    // if xhttp is not enabled, forward selector to protohttp
    if (pState->pProtoHttp != NULL)
    {
        return(__ProtoHttpControl(pState->pProtoHttp, iSelect, iValue, iValue2, pValue));
    }

    // xhttp selectors
    if (iSelect == 'apnd')
    {
        return(_ProtoHttpSetAppendHeader(pState, (const char *)pValue));
    }
    if (iSelect == 'spam')
    {
        pState->iVerbose = iValue;
        return(0);
    }
    if (iSelect == 'time')
    {
        NetPrintf(("protohttpxenon: [0x%08x] setting timeout to %d ms\n", (uintptr_t)pState, iValue));
        pState->uTimeout = (unsigned)iValue;
        return(0);
    }
    // unhandled
    NetPrintf(("protohttpxenon: [0x%08x] ProtoHttpControl('%C') unhandled\n", (uintptr_t)pState, iSelect));
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
        Selectors are (Xhttp only):

        \verbatim
            SELECTOR    RETURN RESULT
        \endverbatim

       In default (non-Xhttp) mode, selectors are passed through to ProtoHttp

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpStatus(ProtoHttpRefT *pState, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    // handle http status requests
    if (pState->pProtoHttp != NULL)
    {
        return(__ProtoHttpStatus(pState->pProtoHttp, iSelect, pBuffer, iBufSize));
    }

    // handle xhttp status requests

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
    // unhandled
    NetPrintf(("protohttpxenon: [0x%08x] ProtoHttpStatus('%C') unhandled\n", (uintptr_t)pState, iSelect));
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

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoHttpCheckKeepAlive(ProtoHttpRefT *pState, const char *pUrl)
{
    if (pState->pProtoHttp != NULL)
    {
        return(__ProtoHttpCheckKeepAlive(pState->pProtoHttp, pUrl));
    }
    //$$ todo
    NetPrintf(("protohttpxenon: [0x%08x] ProtoHttpCheckKeepAlive() unhandled\n", (uintptr_t)pState));
    return(0);
}

/*F********************************************************************************/
/*!
    \Function ProtoHttpUpdate

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pState      - reference pointer

    \Version 12/06/2011 (jbrookes)
*/
/********************************************************************************F*/
void ProtoHttpUpdate(ProtoHttpRefT *pState)
{
    int32_t iResult;

    if (pState->pProtoHttp != NULL)
    {
        __ProtoHttpUpdate(pState->pProtoHttp);
        return;
    }

    // idle? do nothing
    if (pState->eState == ST_IDLE)
    {
        return;
    }

    // check for timeout
    if ((pState->eState > ST_IDLE) && (pState->eState < ST_DONE))
    {
        if (NetTickDiff(NetTick(), pState->uTimer) >= 0)
        {
            NetPrintf(("protohttpxenon: [0x%08x] server timeout (state=%d)\n", (uintptr_t)pState, pState->eState));
            pState->eState = ST_FAIL;
            pState->bTimeout = TRUE;
        }
    }

    // wait for connection completion
    if (pState->eState == ST_AUTH)
    {
        int32_t iResult;
        if ((iResult = DirtyAuthCheckToken(pState->strHost)) == DIRTYAUTH_SUCCESS)
        {
            pState->eState = ST_CONN;
        }
        else if (iResult == DIRTYAUTH_PENDING)
        {
            return;
        }
        else
        {
            pState->eState = ST_FAIL;
        }
    }

    // connecting state - issue request
    if (pState->eState == ST_CONN)
    {
        if (_ProtoHttpRequest(pState) >= 0)
        {
            pState->uTimer = NetTick() + pState->uTimeout;
            pState->eState = ST_SEND;
            pState->bConnOpen = TRUE;
            pState->bSendInProgress = TRUE;
        }
        else
        {
            pState->eState = ST_FAIL;
        }
    }

    // give time to XHttp
    if (pState->eState < ST_DONE)
    {
        XHttpDoWork(pState->hHttpSession, 1);
    }

    // waiting for callback to clear initial send status
    if ((pState->eState == ST_SEND) && (pState->bSendInProgress != TRUE))
    {
        if (XHttpReceiveResponse(pState->hHttpRequest, NULL) == TRUE)
        {
            NetPrintf(("protohttpxenon: [0x%08x] received response\n", (uintptr_t)pState));
            pState->eState = (pState->iPostSize > 0) ? ST_RESP : ST_HEAD;
        }
        else
        {
            NetPrintf(("protohttpxenon: [0x%08x] XHttpReceiveResponse() failed (err=%s)\n",
                (uintptr_t)pState, DirtyErrGetName(GetLastError())));
            pState->eState = ST_FAIL;
        }
    }

    // wait for data sending to be complete
    if (pState->eState == ST_RESP)
    {
        // update send processing
        _ProtoHttpSendBuff(pState);

        // are we done?
        if (pState->iSentSize == pState->iPostSize)
        {
            NetPrintf(("protohttpxenon: [0x%08x] POST data send complete\n", (uintptr_t)pState));
            pState->iInpLen = 0;
            pState->eState = ST_HEAD;
        }
    }

    // get header data
    if ((pState->eState == ST_HEAD) && (pState->bHeadAvail == TRUE))
    {
        // get the header
        if (_ProtoHttpGetHeader(pState) == 0)
        {
            pState->eState = ST_FAIL;
        }
        else if ((pState->iSecure == 1) && (pState->iHdrCode == PROTOHTTP_RESPONSE_FORBIDDEN))
        {
            // xhttp: if secure and response is 403, this indicates the token has expired
            NetPrintf(("protohttpxenon: token has expired; requesting new token\n"));

            // clear state that has been set so far
            pState->iSentSize = 0;
            pState->iHeadSize = 0;
            pState->iHdrCode = 0;
            pState->iBodySize = 0;
            pState->iHdrDate = 0;
            pState->bCloseHdr = 0;
            pState->bInfoHdr = 0;

            // clear state that has been set so far
            if ((iResult = DirtyAuthGetToken(pState->strHost, TRUE)) == DIRTYAUTH_PENDING)
            {
                // wait for authentication to complete
                pState->eState = ST_AUTH;
            }
            else if (iResult == DIRTYAUTH_SUCCESS)
            {
                // reconnect
                pState->eState = ST_CONN;
            }
            else
            {
                pState->eState = ST_FAIL;
            }
        }
        else if ((pState->eRequestType == PROTOHTTP_REQUESTTYPE_HEAD) || (pState->iHdrCode == PROTOHTTP_RESPONSE_NOCONTENT) || (pState->iHdrCode == PROTOHTTP_RESPONSE_NOTMODIFIED))
        {
            // only needed the header (or response did not include a body; see HTTP RFC 1.1 section 4.4) -- all done
            pState->eState = ST_DONE;
        }
        else if ((pState->iBodySize >= 0) && (pState->iInpCnt == pState->iBodySize))
        {
            // we got entire body with header -- all done
            pState->eState = ST_DONE;
        }
        else
        {
            // wait for rest of body
            pState->eState = ST_BODY;
        }
    }

    // check for data available, request new data
    for (iResult = 1; iResult != 0 && pState->eState == ST_BODY; )
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

        /* add to buffer (do this before check for connection close, because unlike with a socket,
            we can get a close notification and data */
        if (iResult > 0)
        {
            pState->iInpLen += iResult;
            pState->iInpCnt += iResult;
        }

        // check for connection close
        if ((pState->bRecvComplete == TRUE) && ((pState->iBodySize == -1) || (pState->iBodySize == pState->iInpCnt)))
        {
            if (pState->iBodySize == -1)
            {
                pState->iBodySize = pState->iInpCnt;
            }
            pState->bCloseHdr = TRUE;
            pState->eState = ST_DONE;
            break;
        }
        else if (iResult < 0)
        {
            NetPrintf(("protohttpxenon: [0x%08x] ST_FAIL (err=%d)\n", (uintptr_t)pState, iResult));
            pState->eState = ST_FAIL;
            break;
        }

        // check for end of body
        if ((pState->iBodySize >= 0) && (pState->iInpCnt >= pState->iBodySize))
        {
            pState->eState = ST_DONE;
            break;
        }

        // give XHttp a chance to do some stuff
        XHttpDoWork(pState->hHttpSession, 1);
    }

    // handle completion
    if (pState->eState == ST_DONE)
    {
        _ProtoHttpClose(pState, "server request");
    }
}
#else

unsigned char _ProtoHttpXenonDisabled = 1; // suppress linker warning

#endif // DIRTYSDK_XHTTP_ENABLED

