/*H********************************************************************************/
/*!
    \File httpserv.c

    \Description
        Implements basic http server

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 09/11/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/proto/protohttputil.h"

#include "zlib.h"
#include "zmem.h"
#include "zfile.h"

#include "testermodules.h"

/*** Defines **********************************************************************/

#define HTTPSERV_RATE       (1)
#define HTTPSERV_LISTENPORT (9000)

/*** Function Prototypes **********************************************************/

/*** Type Definitions *************************************************************/

typedef struct HttpServRespT
{
    ProtoHttpResponseE eResponse;
    const char *pResponseText;
}HttpServRespT;

typedef struct HttpServT      // module state storage
{
    SocketT *pListenSocket;
    SocketT *pResponseSocket;
    struct sockaddr RecvAddr;

    ProtoHttpRequestTypeE eRequestType;
    uint32_t uRequestAddr;
    int32_t iResponseCode;

    int32_t iBufLen;
    int32_t iBufOff;
    int32_t iConnectTimer;

    ZFileT  iFileId;
    int64_t iFileLen;
    int64_t iContentLength;
    int64_t iContentSent;
    int64_t iContentRecv;

    char strServerName[128];
    char strFileDir[512];
    char strUrl[1*1024];
    char strHeader[4*1024];
    char strBuffer[16*1024]; // temp

    uint8_t bConnected;
    uint8_t bReceivedHeader;
    uint8_t bParsedHeader;
    uint8_t bProcessedRequest;
    uint8_t bReceivedBody;
    uint8_t bFormattedHeader;
    uint8_t bSentHeader;
    uint8_t bSentResponse;
    uint8_t bConnectionClose;
    uint8_t _pad[3];
} HttpServT;

/*** Variables ********************************************************************/

static HttpServT _HttpServ_Ref;
static uint8_t   _HttpServ_bInitialized = FALSE;

static const char *_HttpServ_strRequestNames[PROTOHTTP_NUMREQUESTTYPES] =
{
    "HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS"
};

static const HttpServRespT _HttpServ_Responses[] =
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


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _HttpServGetResponseText

    \Description
        Return response text for specified response code

    \Input eResponseCode    - code to get text for

    \Output
        const char *        - response text

    \Version 09/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_HttpServGetResponseText(ProtoHttpResponseE eResponseCode)
{
    int32_t iResponse;
    for (iResponse = 0; _HttpServ_Responses[iResponse].eResponse != PROTOHTTP_RESPONSE_PENDING; iResponse += 1)
    {
        if (_HttpServ_Responses[iResponse].eResponse == eResponseCode)
        {
            break;
        }
    }
    return(_HttpServ_Responses[iResponse].pResponseText);
}

/*F********************************************************************************/
/*!
    \Function _HttpServSocketOpen

    \Description
        Open a DirtySock socket for http server

    \Input uPort        - port to bind

    \Output
        SocketT *       - socket ref, or NULL

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static SocketT *_HttpServSocketOpen(uint16_t uPort)
{
    struct sockaddr BindAddr;
    SocketT *pSocket;
    int32_t iResult;

    // open the socket
    if ((pSocket = SocketOpen(AF_INET, SOCK_STREAM, IPPROTO_IP)) == NULL)
    {
        return(NULL);
    }

    // bind socket to specified port
    SockaddrInit(&BindAddr, AF_INET);
    SockaddrInSetPort(&BindAddr, uPort);
    if ((iResult = SocketBind(pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
    {
        ZPrintf("httpserv: error %d binding to port\n", iResult);
        SocketClose(pSocket);
        return(NULL);
    }

    // return ref to caller
    return(pSocket);
}

/*F********************************************************************************/
/*!
    \Function _HttpServParseHeader

    \Description
        Update HttpServ while receiving header

    \Input *pHttpServ   - module state.

    \Output
        int32_t         - positive=success, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServParseHeader(HttpServT *pHttpServ)
{
    const char *pHdr, *pEnd;
    char strName[128], strValue[1024];
    int32_t iResult, iType;

    // process request type
    for (iType = 0; iType < PROTOHTTP_NUMREQUESTTYPES; iType += 1)
    {
        if (!ds_strnicmp(_HttpServ_strRequestNames[iType], pHttpServ->strHeader, (int32_t)strlen(_HttpServ_strRequestNames[iType])))
        {
            ZPrintf("httpserv: processing %s request\n", _HttpServ_strRequestNames[iType]);
            pHttpServ->eRequestType = (ProtoHttpRequestTypeE)iType;
            break;
        }
    }
    if (iType == PROTOHTTP_NUMREQUESTTYPES)
    {
        // not a valid request type
        ZPrintf("httpserv: invalid request type\n");
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-1);
    }

    // find url
    if ((pHdr = strchr(pHttpServ->strHeader, '/')) == NULL)
    {
        // not a valid header
        ZPrintf("httpserv: could not find url\n");
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-2);
    }
    // find end of url
    if ((pEnd = strchr(pHdr, ' ')) == NULL)
    {
        // not a valid header
        ZPrintf("httpserv: could not find url end\n");
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-3);
    }
    ds_strsubzcpy(pHttpServ->strUrl, sizeof(pHttpServ->strUrl), pHdr, pEnd-pHdr+1);
    NetPrintf(("httpserv: url=%s\n", pHttpServ->strUrl));

    // skip to first header
    if ((pHdr = strstr(pEnd, "\r\n")) == NULL)
    {
        // not a valid header
        ZPrintf("httpserv: could not find headers\n");
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_BADREQUEST;
        return(-4);
    }

    // parse header
    for (iResult = 0; iResult >= 0; )
    {
        // get next header name and value from header buffer
        if ((iResult = ProtoHttpGetNextHeader(NULL, pHdr, strName, sizeof(strName), strValue, sizeof(strValue), &pEnd)) == 0)
        {
            // process header
            if (!ds_stricmp(strName, "connection") && (!ds_stricmp(strValue, "close")))
            {
                pHttpServ->bConnectionClose = TRUE;
            }
            if (!ds_stricmp(strName, "content-length"))
            {
                pHttpServ->iContentLength = ds_strtoll(strValue, NULL, 10);
            }
            else
            {
                NetPrintf(("httpserv: unprocessed header '%s'='%s'\n", strName, strValue));
            }
        }

        // move to next header
        pHdr = pEnd;
    }

    pHttpServ->bParsedHeader = TRUE;
    pHttpServ->bProcessedRequest = FALSE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _HttpServFormatHeader

    \Description
        Update HttpServ while receiving header

    \Input *pHttpServ   - module state.

    \Output
        int32_t         - positive=success, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServFormatHeader(HttpServT *pHttpServ)
{
    // format a basic response header
    pHttpServ->iBufLen = ds_snzprintf(pHttpServ->strBuffer, sizeof(pHttpServ->strBuffer), "HTTP/1.1 %d %s\r\n", pHttpServ->iResponseCode,
        _HttpServGetResponseText((ProtoHttpResponseE)pHttpServ->iResponseCode));
    if (pHttpServ->bConnectionClose)
    {
        pHttpServ->iBufLen += ds_snzprintf(pHttpServ->strBuffer+pHttpServ->iBufLen, sizeof(pHttpServ->strBuffer), "Connection: Close\r\n");
    }
    pHttpServ->iBufLen += ds_snzprintf(pHttpServ->strBuffer+pHttpServ->iBufLen, sizeof(pHttpServ->strBuffer), "Content-length: %qd\r\n", pHttpServ->iContentLength);
    pHttpServ->iBufLen += ds_snzprintf(pHttpServ->strBuffer+pHttpServ->iBufLen, sizeof(pHttpServ->strBuffer), "Content-type: text/html\r\n");
    pHttpServ->iBufLen += ds_snzprintf(pHttpServ->strBuffer+pHttpServ->iBufLen, sizeof(pHttpServ->strBuffer), "\r\n");
    pHttpServ->iBufOff = 0;
    pHttpServ->bFormattedHeader = TRUE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _HttpServProcessGet

    \Description
        Process GET/HEAD request

    \Input *pHttpServ   - module state.

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpServProcessGet(HttpServT *pHttpServ)
{
    char strFilePath[1024];

    // default to no data
    pHttpServ->iContentLength = 0;

    // create filepath
    ds_snzprintf(strFilePath, sizeof(strFilePath), "%s\\%s", pHttpServ->strFileDir, pHttpServ->strUrl + 1);

    // see if url refers to a file
    if ((pHttpServ->iFileId = ZFileOpen(strFilePath, ZFILE_OPENFLAG_RDONLY|ZFILE_OPENFLAG_BINARY)) == ZFILE_INVALID)
    {
        // no file
        ZPrintf("httpserv: could not open file '%s' for reading\n", strFilePath);
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_NOTFOUND;
        pHttpServ->bProcessedRequest = TRUE;
        return;
    }

    // get the file size
    if ((pHttpServ->iFileLen = ZFileSize(pHttpServ->iFileId)) < 0)
    {
        ZPrintf("httpserv: error %d getting file size\n", pHttpServ->iFileLen);
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        pHttpServ->bProcessedRequest = TRUE;
        return;
    }

    // we don't support chunked encoding, so force connection closed
    if (pHttpServ->eRequestType == PROTOHTTP_REQUESTTYPE_GET)
    {
        pHttpServ->bConnectionClose = TRUE;
    }

    // set content-length
    pHttpServ->iContentLength = pHttpServ->iFileLen;
    pHttpServ->bProcessedRequest = TRUE;
    pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_OK; 
}

/*F********************************************************************************/
/*!
    \Function _HttpServProcessPut

    \Description
        Process PUT/POST request

    \Input *pHttpServ   - module state.

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpServProcessPut(HttpServT *pHttpServ)
{
    char strFilePath[1024];

    // we require content-length up front
    if (pHttpServ->iContentLength == 0)
    {
        ZPrintf("httpserv: put/post with no content-length not supported\n");
        pHttpServ->bProcessedRequest = TRUE;
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_NOTIMPLEMENTED;
        return;
    }

    // create filepath
    ds_snzprintf(strFilePath, sizeof(strFilePath), "%s\\%s", pHttpServ->strFileDir, pHttpServ->strUrl + 1);

    // try to open file
    if ((pHttpServ->iFileId = ZFileOpen(strFilePath, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_BINARY)) == ZFILE_INVALID)
    {
        ZPrintf("httpserv: could not open file '%s' for writing\n", strFilePath);
        pHttpServ->bProcessedRequest = TRUE;
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return;
    }

    // we've processed the request
    pHttpServ->iContentRecv = 0;
    pHttpServ->bReceivedBody = 0;
    pHttpServ->bProcessedRequest = TRUE;
}

/*F********************************************************************************/
/*!
    \Function _HttpServUpdateListen

    \Description
        Update HttpServ while listening for a new connection

    \Input *pHttpServ   - module state.

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServUpdateListen(HttpServT *pHttpServ)
{
    int32_t iAddrLen = sizeof(pHttpServ->RecvAddr);
    // don't try Accept() if poll hint says nothing to read
    if (SocketInfo(pHttpServ->pListenSocket, 'read', 0, NULL, 0) == 0)
    {
        return(0);
    }
    // listen for a new status inquiry
    if ((pHttpServ->pResponseSocket = SocketAccept(pHttpServ->pListenSocket, &pHttpServ->RecvAddr, &iAddrLen)) == NULL)
    {
        return(0);
    }
    pHttpServ->bConnected = FALSE;
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _HttpServUpdateConnect

    \Description
        Update HttpServ while establishing a new connection

    \Input *pHttpServ   - module state.

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServUpdateConnect(HttpServT *pHttpServ)
{
    int32_t iStat;
    if ((iStat = SocketInfo(pHttpServ->pResponseSocket, 'stat', 0, NULL, 0)) > 0)
    {
        // get requester address
        pHttpServ->uRequestAddr = SockaddrInGetAddr(&pHttpServ->RecvAddr);

        ZPrintf("httpserv: connected to %a:%d\n",
            SockaddrInGetAddr(&pHttpServ->RecvAddr),
            SockaddrInGetPort(&pHttpServ->RecvAddr));

        pHttpServ->bConnected = TRUE;
        pHttpServ->bReceivedHeader = FALSE;
        pHttpServ->bSentResponse = FALSE;
        pHttpServ->iContentSent = 0;
        pHttpServ->iContentLength = 0;
        pHttpServ->iContentRecv = 0;
        pHttpServ->iBufLen = 0;
        pHttpServ->iBufOff = 0;
        pHttpServ->iConnectTimer = NetTick();
        return(1);
    }
    else if (iStat < 0)
    {
        ZPrintf("httpserv: error connecting to requester\n");
        SocketClose(pHttpServ->pResponseSocket);
        pHttpServ->pResponseSocket = NULL;
        return(-1);
    }
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpServUpdateRecvHeader

    \Description
        Update HttpServ while receiving header

    \Input *pHttpServ   - module state.

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServUpdateRecvHeader(HttpServT *pHttpServ)
{
    int32_t iResult;

    if ((iResult = SocketRecv(pHttpServ->pResponseSocket, pHttpServ->strBuffer+pHttpServ->iBufLen, sizeof(pHttpServ->strBuffer)-pHttpServ->iBufLen, 0)) > 0)
    {
        char *pHdrEnd;
        int32_t iHdrLen;

        // update received data size
        pHttpServ->iBufLen += iResult;

        // check for header termination
        if ((pHdrEnd = strstr(pHttpServ->strBuffer, "\r\n\r\n")) != NULL)
        {
            // we want to keep the final header EOL
            pHdrEnd += 2;

            // copy header to buffer
            iHdrLen = pHdrEnd - pHttpServ->strBuffer;
            ds_strsubzcpy(pHttpServ->strHeader, sizeof(pHttpServ->strHeader), pHttpServ->strBuffer, iHdrLen);
            ZPrintf("httpserv: receved %d byte header\n", iHdrLen);
            NetPrintWrap(pHttpServ->strHeader, 132);

            // skip header/body seperator
            pHdrEnd += 2;
            iHdrLen += 2;

            // remove header from buffer
            memmove(pHttpServ->strBuffer, pHdrEnd, pHttpServ->iBufLen - iHdrLen);
            pHttpServ->iBufLen -= iHdrLen;

            // we've received the header
            pHttpServ->bReceivedHeader = TRUE;

            // reset for next state
            pHttpServ->bConnectionClose = FALSE;
            pHttpServ->bParsedHeader = FALSE;
            pHttpServ->bFormattedHeader = FALSE;
            pHttpServ->bSentHeader = FALSE;
        }
    }
    else if (iResult < 0)
    {
        ZPrintf("httpserv: SocketRecv() returned error %d; closing connection\n", iResult);
        SocketClose(pHttpServ->pResponseSocket);
        pHttpServ->pResponseSocket = NULL;
        pHttpServ->bConnected = FALSE;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpServUpdateRecvBody

    \Description
        Update HttpServ while receiving body

    \Input *pHttpServ   - module state.

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServUpdateRecvBody(HttpServT *pHttpServ)
{
    int32_t iResult;

    if (pHttpServ->iFileId == ZFILE_INVALID)
    {
        pHttpServ->bReceivedBody = TRUE;
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(1);
    }

    while (pHttpServ->iContentRecv < pHttpServ->iContentLength)
    {
        // need new data?
        while (pHttpServ->iBufLen < (signed)sizeof(pHttpServ->strBuffer))
        {
            iResult = SocketRecv(pHttpServ->pResponseSocket, pHttpServ->strBuffer + pHttpServ->iBufLen, sizeof(pHttpServ->strBuffer) - pHttpServ->iBufLen, 0);
            if (iResult > 0)
            {
                pHttpServ->iBufLen += iResult;
                ZPrintf("httpserv: recv %d bytes\n", iResult);
            }
            else if (iResult < 0)
            {
                ZPrintf("httpserv: SocketRecv() failed; error %d\n", iResult);
                return(-2);
            }
            else
            {
                break;
            }
        }

        // if we have a full buffer, or we have the last bit of data, write it
        if ((pHttpServ->iBufLen == sizeof(pHttpServ->strBuffer)) || ((pHttpServ->iBufLen - pHttpServ->iBufOff + pHttpServ->iContentRecv) == pHttpServ->iContentLength))
        {
            int32_t iDataSize = pHttpServ->iBufLen - pHttpServ->iBufOff;
            if ((iDataSize = ZFileWrite(pHttpServ->iFileId, pHttpServ->strBuffer, iDataSize)) > 0)
            {
                pHttpServ->iContentRecv += iDataSize;
                pHttpServ->iBufOff += iDataSize;

                ZPrintf("httpserv: wrote %d bytes to file (%qd total)\n", iDataSize, pHttpServ->iContentRecv);

                // if we've written all of the data reset buffer pointers
                if (pHttpServ->iBufOff == pHttpServ->iBufLen)
                {
                    pHttpServ->iBufOff = 0;
                    pHttpServ->iBufLen = 0;
                }
            }
            else
            {
                ZPrintf("httpserv: error %d reading from file\n", pHttpServ->iBufLen);
                pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
                return(-1);
            }
        }
        else
        {
            break;
        }
    }

    // check for upload completion
    if (pHttpServ->iContentRecv == pHttpServ->iContentLength)
    {
        // done receiving response
        ZPrintf("httpserv: received upload of %qd bytes\n", pHttpServ->iContentRecv);

        // close the file
        ZFileClose(pHttpServ->iFileId);
        pHttpServ->iFileId = ZFILE_INVALID;

        // done with response
        pHttpServ->iContentLength = 0;
        pHttpServ->bReceivedBody = TRUE;
        pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_CREATED;
        return(1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpServUpdateSendHeader

    \Description
        Update HttpServ while sending header

    \Input *pHttpServ   - module state.

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServUpdateSendHeader(HttpServT *pHttpServ)
{
    int32_t iResult;

    // send data to requester
    if ((iResult = SocketSend(pHttpServ->pResponseSocket, pHttpServ->strBuffer + pHttpServ->iBufOff, pHttpServ->iBufLen - pHttpServ->iBufOff, 0)) > 0)
    {
        pHttpServ->iBufOff += iResult;

        // are we done?
        if (pHttpServ->iBufOff == pHttpServ->iBufLen)
        {
            ZPrintf("httpserv: sent %d byte header\n", pHttpServ->iBufOff);
            NetPrintWrap(pHttpServ->strBuffer, 132);
            pHttpServ->bSentHeader = TRUE;
            pHttpServ->iBufOff = 0;
            pHttpServ->iBufLen = 0;
            iResult = 1;
        }
        else
        {
            iResult = 0;
        }
    }
    else if (iResult < 0)
    {
        ZPrintf("httpserv: SocketSend() failed; error %d\n", iResult);
        pHttpServ->iBufOff = pHttpServ->iBufLen;
        return(-1);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _HttpServUpdateSendResponse

    \Description
        Update HttpServ while sending response

    \Input *pHttpServ   - module state.

    \Output
        int32_t         - positive=success, zero=in progress, negative=failure

    \Version 09/12/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _HttpServUpdateSendResponse(HttpServT *pHttpServ)
{
    int32_t iResult;

    if (pHttpServ->iFileId == ZFILE_INVALID)
    {
        pHttpServ->bSentResponse = TRUE;
        return(1);
    }

    while (pHttpServ->iContentSent < pHttpServ->iContentLength)
    {
        // need new data?
        if (pHttpServ->iBufOff == pHttpServ->iBufLen)
        {
            if ((pHttpServ->iBufLen = ZFileRead(pHttpServ->iFileId, pHttpServ->strBuffer, sizeof(pHttpServ->strBuffer))) > 0)
            {
                ZPrintf("httpserv: read %d bytes from file\n", pHttpServ->iBufLen);
                pHttpServ->iBufOff = 0;
            }
            else
            {
                ZPrintf("httpserv: error %d reading from file\n", pHttpServ->iBufLen);
                pHttpServ->iResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
                return(-1);
            }
        }

        // do we have buffered data to send?
        if (pHttpServ->iBufOff < pHttpServ->iBufLen)
        {
            iResult = SocketSend(pHttpServ->pResponseSocket, pHttpServ->strBuffer + pHttpServ->iBufOff, pHttpServ->iBufLen - pHttpServ->iBufOff, 0);
            if (iResult > 0)
            {
                pHttpServ->iContentSent += iResult;
                pHttpServ->iBufOff += iResult;
                ZPrintf("httpserv: sent %d bytes (%qd total)\n", iResult, pHttpServ->iContentSent);
            }
            else if (iResult < 0)
            {
                ZPrintf("httpserv: SocketSend() failed; error %d\n", iResult);
                return(-2);
            }
            else
            {
                break;
            }
        }
    }

    // check for send completion
    if (pHttpServ->iContentSent == pHttpServ->iContentLength)
    {
        // done sending response
        ZPrintf("httpserv: sent %qd bytes\n", pHttpServ->iContentSent);

        // close the file
        if (pHttpServ->iFileId != ZFILE_INVALID)
        {
            ZFileClose(pHttpServ->iFileId);
            pHttpServ->iFileId = ZFILE_INVALID;
        }

        // done with response
        pHttpServ->bSentResponse = TRUE;
        return(1);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _HttpServUpdate

    \Description
        Update HttpServ module

    \Input *pHttpServ   - module state.

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _HttpServUpdate(HttpServT *pHttpServ)
{
    // if we don't have a listen socket don't do anything at all
    if (pHttpServ->pListenSocket == NULL)
    {
        return;
    }

    // if we're not already responding, listen for a response
    if ((pHttpServ->pResponseSocket == NULL) && (_HttpServUpdateListen(pHttpServ) <= 0))
    {
        return;
    }

    // poll for connection completion
    if ((pHttpServ->bConnected == FALSE) && (_HttpServUpdateConnect(pHttpServ) <= 0))
    {
        return;
    }

    // receive the header
    if ((pHttpServ->bReceivedHeader == FALSE) && (_HttpServUpdateRecvHeader(pHttpServ) <= 0))
    {
        return;
    }

    // parse headers
    if ((pHttpServ->bParsedHeader == FALSE) && (_HttpServParseHeader(pHttpServ) < 0))
    {
        return;
    }

    // process the request
    if (pHttpServ->bProcessedRequest == FALSE)
    {
        // if a PUT or POST
        if ((pHttpServ->eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (pHttpServ->eRequestType == PROTOHTTP_REQUESTTYPE_POST))
        {
            _HttpServProcessPut(pHttpServ);
        }

        // if a GET or HEAD
        if ((pHttpServ->eRequestType == PROTOHTTP_REQUESTTYPE_GET) || (pHttpServ->eRequestType == PROTOHTTP_REQUESTTYPE_HEAD))
        {
            _HttpServProcessGet(pHttpServ);
        }
        if (pHttpServ->bProcessedRequest == FALSE)
        {
            return;
        }
    }

    // if a PUT or POST
    if ((pHttpServ->eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (pHttpServ->eRequestType == PROTOHTTP_REQUESTTYPE_POST))
    {
        if ((pHttpServ->bReceivedBody == FALSE) && (_HttpServUpdateRecvBody(pHttpServ) <= 0))
        {
            return;
        }
    }

    // format response header
    if ((pHttpServ->bFormattedHeader == FALSE) && (_HttpServFormatHeader(pHttpServ) < 0))
    {
        return;
    }

    // send response header
    if ((pHttpServ->bSentHeader == FALSE) && (_HttpServUpdateSendHeader(pHttpServ) <= 0))
    {
        return;
    }

    // process response data, if any
    if ((pHttpServ->bSentResponse == FALSE) && (_HttpServUpdateSendResponse(pHttpServ) <= 0))
    {
        return;
    }

    // if no keep-alive kill the connection
    if (pHttpServ->bConnectionClose == TRUE)
    {
        ZPrintf("httpserv: closing connection\n");
        SocketClose(pHttpServ->pResponseSocket);
        pHttpServ->pResponseSocket = NULL;
        pHttpServ->bConnected = FALSE;
    }
    else
    {
        pHttpServ->bReceivedHeader = FALSE;
        pHttpServ->iBufLen = 0;
        pHttpServ->iBufOff = 0;
        pHttpServ->iConnectTimer = NetTick();
    }
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpServIdleCB

    \Description
        Callback to process while idle

    \Input *argz    -
    \Input argc     -
    \Input *argv[]  -

    \Output int32_t -

    \Version 09/26/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _CmdHttpServIdleCB(ZContext *argz, int32_t argc, char *argv[])
{
    HttpServT *pRef = &_HttpServ_Ref;

    // shut down?
    if (argc == 0)
    {
        return(0);
    }

    // process
    _HttpServUpdate(pRef);

    // keep on idling
    return(ZCallback(&_CmdHttpServIdleCB, HTTPSERV_RATE));
}

/*F********************************************************************************/
/*!
    \Function _CmdHttpServUsage

    \Description
        Display usage information.

    \Input argc         - argument count
    \Input *argv[]      - argument list

    \Version 09/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _CmdHttpServUsage(int argc, char *argv[])
{
    if (argc == 2)
    {
        ZPrintf("   basic http server\n");
        ZPrintf("   usage: %s [filedir|listen]", argv[0]);
    }
    else if (argc == 3)
    {
        if (!strcmp(argv[2], "filedir"))
        {
            ZPrintf("   usage: %s filedir <directory>\n", argv[0]);
        }
        else if (!strcmp(argv[2], "listen"))
        {
            ZPrintf("   usage: %s listen [listenport]\n", argv[0]);
        }
    }
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function CmdHttpServ

    \Description
        Simple HTTP server

    \Input *argz    - context
    \Input argc     - command count
    \Input *argv[]  - command arguments

    \Output int32_t -

    \Version 09/11/2012 (jbrookes)
*/
/********************************************************************************F*/
int32_t CmdHttpServ(ZContext *argz, int32_t argc, char *argv[])
{
    HttpServT *pHttpServ = &_HttpServ_Ref;
    int32_t iResult = 0;

    if ((argc < 2) || !ds_stricmp(argv[1], "help"))
    {
        _CmdHttpServUsage(argc, argv);
        return(iResult);
    }

    // if not initialized yet, do so now
    if (_HttpServ_bInitialized == FALSE)
    {
        memset(pHttpServ, 0, sizeof(*pHttpServ));
        ds_strnzcpy(pHttpServ->strServerName, "Tester2 Http Server", sizeof(pHttpServ->strServerName));
        ds_strnzcpy(pHttpServ->strFileDir, "c:\\temp\\", sizeof(pHttpServ->strFileDir));
        pHttpServ->iBufLen = sizeof(pHttpServ->strBuffer);
        pHttpServ->iFileId = ZFILE_INVALID;
        _HttpServ_bInitialized = TRUE;
    }

    // check for filedir set
    if ((argc == 3) && !ds_stricmp(argv[1], "filedir"))
    {
        ds_strnzcpy(pHttpServ->strFileDir, argv[1], sizeof(pHttpServ->strFileDir));
        return(iResult);
    }

    // check for listen
    if ((argc >= 2) && !ds_stricmp(argv[1], "listen"))
    {
        int32_t iPort = HTTPSERV_LISTENPORT;
        if (argc == 3)
        {
            iPort = strtol(argv[2], NULL, 10);
            if ((iPort < 1) || (iPort > 65535))
            {
                ZPrintf("%s: invalid port %d specified in listen request\n", argv[0], iPort);
                return(-1);
            }
        }

        // open a socket
        if ((pHttpServ->pListenSocket = _HttpServSocketOpen(iPort)) == NULL)
        {
            ZPrintf("%s: could not open socket on port %d\n", argv[0], iPort);
            return(-2);
        }
        // start listening
        if ((iResult = SocketListen(pHttpServ->pListenSocket, 2)) != SOCKERR_NONE)
        {
            ZPrintf("%s: error listening on diagnostic socket\n", argv[0]);
            SocketClose(pHttpServ->pListenSocket);
            pHttpServ->pListenSocket = NULL;
            return(iResult);
        }

        // install recurring update
        iResult = ZCallback(_CmdHttpServIdleCB, HTTPSERV_RATE);
    }

    return(iResult);
}


