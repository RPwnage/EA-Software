/*H********************************************************************************/
/*!
    \File    protohttp.h

    \Description
        This module implements an HTTP client that can perform basic transactions
        (get/put) with an HTTP server.  It conforms to but does not fully implement
        the 1.1 HTTP spec (http://www.w3.org/Protocols/rfc2616/rfc2616.html), and
        allows for secure HTTP transactions as well as insecure transactions.

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2000-2007. ALL RIGHTS RESERVED.

    \Version 0.5 02/21/2000 (gschaefer) First Version
    \Version 1.0 12/07/2000 (gschaefer) Added PS2/Dirtysock support
    \Version 1.1 03/03/2004 (sbevan)    ProtoSSL/https rewrite, added limited Post support.
    \Version 1.2 11/18/2004 (jbrookes)  Refactored, updated to HTTP 1.1, added full Post support.
*/
/********************************************************************************H*/

#ifndef _protohttp_h
#define _protohttp_h

/*!
\Module Proto
*/
//@{

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// defines for ProtoHttpPost's bDoPut parameter
#define PROTOHTTP_POST      (0)     //!< execute a POST when calling ProtoHttpPost
#define PROTOHTTP_PUT       (1)     //!< execute a PUT when calling ProtoHttpPost

// define for ProtoHttpPost's iDataSize parameter
#define PROTOHTTP_STREAM_BEGIN (-1) //!< start streaming upload (size unknown)

// define for ProtoHttpSend's iDataSize parameter
#define PROTOHTTP_STREAM_END   (0)  //!< streaming upload operation is complete

// defines for ProtoHttpGet's bHeadOnly parameter
#define PROTOHTTP_HEADBODY  (0)     //!< get head and body when calling ProtoHttpGet
#define PROTOHTTP_HEADONLY  (1)     //!< only get head, not body, when calling ProtoHttpGet

// defines for ProtoHttpRecv's return result
#define PROTOHTTP_RECVDONE  (-1)    //!< receive operation complete, and all data has been read
#define PROTOHTTP_RECVFAIL  (-2)    //!< receive operation failed
#define PROTOHTTP_RECVWAIT  (-3)    //!< waiting for body data
#define PROTOHTTP_RECVHEAD  (-4)    //!< in headonly mode and header has been received
#define PROTOHTTP_RECVBUFF  (-5)    //!< recvall did not have enough space in the provided buffer

// generic protohttp errors (do not overlap with RECV* codes)
#define PROTOHTTP_MINBUFF   (-6)    //!< not enough input buffer space for operation

/*** Macros ***********************************************************************/

//! get class of response code
#define PROTOHTTP_GetResponseClass(_eError)                 (((_eError)/100) * 100)

/*** Type Definitions *************************************************************/

//! request types
typedef enum ProtoHttpRequestTypeE
{
    PROTOHTTP_REQUESTTYPE_HEAD = 0,     //!< HEAD request - does not return any data
    PROTOHTTP_REQUESTTYPE_GET,          //!< GET request  - get data from a server
    PROTOHTTP_REQUESTTYPE_POST,         //!< POST request - post data to a server
    PROTOHTTP_REQUESTTYPE_PUT,          //!< PUT request - put data on a server
    PROTOHTTP_REQUESTTYPE_DELETE,       //!< DELETE request - delete resource from a server
    PROTOHTTP_REQUESTTYPE_OPTIONS,      //!< OPTIONS request - get server options for specified resource
    
    PROTOHTTP_NUMREQUESTTYPES
} ProtoHttpRequestTypeE;

/*! HTTP response codes - see http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html#sec10
    for more detailed information. These response codes may be retrieved from ProtoHttp
    by calling ProtoHttpStatus('code').  Note that response codes are not available until
    a response has been received from the server and parsed by ProtoHttp.  If a response
    code is not yet available, -1 will be returned. */
typedef enum ProtoHttpResponseE
{
    // 1xx - informational reponse
    PROTOHTTP_RESPONSE_INFORMATIONAL        = 100,  //!< generic name for a 1xx class response
    PROTOHTTP_RESPONSE_CONTINUE             = 100,  //!< continue with request, generally ignored
    PROTOHTTP_RESPONSE_SWITCHPROTO,                 //!< 101 - OK response to client switch protocol request

    // 2xx - success response
    PROTOHTTP_RESPONSE_SUCCESSFUL           = 200,  //!< generic name for a 2xx class response
    PROTOHTTP_RESPONSE_OK                   = 200,  //!< client's request was successfully received, understood, and accepted
    PROTOHTTP_RESPONSE_CREATED,                     //!< new resource has been created
    PROTOHTTP_RESPONSE_ACCEPTED,                    //!< request accepted but not complete
    PROTOHTTP_RESPONSE_NONAUTH,                     //!< non-authoritative info (ok)
    PROTOHTTP_RESPONSE_NOCONTENT,                   //!< request fulfilled, no message body
    PROTOHTTP_RESPONSE_RESETCONTENT,                //!< request success, reset document view
    PROTOHTTP_RESPONSE_PARTIALCONTENT,              //!< server has fulfilled partial GET request
                                            
    // 3xx - redirection response
    PROTOHTTP_RESPONSE_REDIRECTION          = 300,  //!< generic name for a 3xx class response
    PROTOHTTP_RESPONSE_MULTIPLECHOICES      = 300,  //!< requested resource corresponds to a set of representations
    PROTOHTTP_RESPONSE_MOVEDPERMANENTLY,            //!< requested resource has been moved permanently to new URI
    PROTOHTTP_RESPONSE_FOUND,                       //!< requested resources has been moved temporarily to a new URI
    PROTOHTTP_RESPONSE_SEEOTHER,                    //!< response can be found under a different URI
    PROTOHTTP_RESPONSE_NOTMODIFIED,                 //!< response to conditional get when document has not changed
    PROTOHTTP_RESPONSE_USEPROXY,                    //!< requested resource must be accessed through proxy
    PROTOHTTP_RESPONSE_RESERVED,                    //!< old response code - reserved
    PROTOHTTP_RESPONSE_TEMPREDIRECT,                //!< requested resource resides temporarily under a different URI
                                            
    // 4xx - client error response
    PROTOHTTP_RESPONSE_CLIENTERROR          = 400,  //!< generic name for a 4xx class response
    PROTOHTTP_RESPONSE_BADREQUEST           = 400,  //!< request could not be understood by server due to malformed syntax
    PROTOHTTP_RESPONSE_UNAUTHORIZED,                //!< request requires user authorization
    PROTOHTTP_RESPONSE_PAYMENTREQUIRED,             //!< reserved for future user
    PROTOHTTP_RESPONSE_FORBIDDEN,                   //!< request understood, but server will not fulfill it
    PROTOHTTP_RESPONSE_NOTFOUND,                    //!< Request-URI not found
    PROTOHTTP_RESPONSE_METHODNOTALLOWED,            //!< method specified in the Request-Line is not allowed
    PROTOHTTP_RESPONSE_NOTACCEPTABLE,               //!< resource incapable of generating content acceptable according to accept headers in request
    PROTOHTTP_RESPONSE_PROXYAUTHREQ,                //!< client must first authenticate with proxy
    PROTOHTTP_RESPONSE_REQUESTTIMEOUT,              //!< client did not produce response within server timeout
    PROTOHTTP_RESPONSE_CONFLICT,                    //!< request could not be completed due to a conflict with current state of the resource
    PROTOHTTP_RESPONSE_GONE,                        //!< requested resource is no longer available and no forwarding address is known
    PROTOHTTP_RESPONSE_LENGTHREQUIRED,              //!< a Content-Length header was not specified and is required
    PROTOHTTP_RESPONSE_PRECONFAILED,                //!< precondition given in request-header field(s) failed
    PROTOHTTP_RESPONSE_REQENTITYTOOLARGE,           //!< request entity is larger than the server is able or willing to process
    PROTOHTTP_RESPONSE_REQURITOOLONG,               //!< Request-URI is longer than the server is willing to interpret
    PROTOHTTP_RESPONSE_UNSUPPORTEDMEDIA,            //!< request entity is in unsupported format
    PROTOHTTP_RESPONSE_REQUESTRANGE,                //!< invalid range in Range request header
    PROTOHTTP_RESPONSE_EXPECTATIONFAILED,           //!< expectation in Expect request-header field could not be met by server
                                            
    // 5xx - server error response
    PROTOHTTP_RESPONSE_SERVERERROR          = 500,  //!< generic name for a 5xx class response
    PROTOHTTP_RESPONSE_INTERNALSERVERERROR  = 500,  //!< an unexpected condition prevented the server from fulfilling the request
    PROTOHTTP_RESPONSE_NOTIMPLEMENTED,              //!< the server does not support the functionality required to fulfill the request
    PROTOHTTP_RESPONSE_BADGATEWAY,                  //!< invalid response from gateway server
    PROTOHTTP_RESPONSE_SERVICEUNAVAILABLE,          //!< unable to handle request due to a temporary overloading or maintainance
    PROTOHTTP_RESPONSE_GATEWAYTIMEOUT,              //!< gateway or DNS server timeout
    PROTOHTTP_RESPONSE_HTTPVERSUNSUPPORTED,         //!< the server does not support the HTTP protocol version that was used in the request
    
    PROTOHTTP_RESPONSE_ERROR                = (unsigned)-1
} ProtoHttpResponseE;                       

//! opaque module ref
typedef struct ProtoHttpRefT ProtoHttpRefT;

//! callback that may be used to customize request header before sending
typedef int32_t (ProtoHttpCustomHeaderCbT)(ProtoHttpRefT *pState, char *pHeader, uint32_t uHeaderSize, const char *pData, uint32_t uDataLen, void *pUserRef);

//! callback that may be used to implement custom header parsing on header receipt
typedef int32_t (ProtoHttpReceiveHeaderCbT)(ProtoHttpRefT *pState, const char *pHeader, uint32_t uHeaderSize, void *pUserRef);


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate module state and prepare for use
ProtoHttpRefT *ProtoHttpCreate(int32_t iRcvBuf);

// set custom header callback
void ProtoHttpCallback(ProtoHttpRefT *pState, ProtoHttpCustomHeaderCbT *pCustomHeaderCb, ProtoHttpReceiveHeaderCbT *pReceiveHeaderCb, void *pUserRef);

// destroy the module and release its state
void ProtoHttpDestroy(ProtoHttpRefT *pState);

// initiate an HTTP transfer
int32_t ProtoHttpGet(ProtoHttpRefT *pState, const char *pUrl, uint32_t bHeadOnly);

// return the actual url data
int32_t ProtoHttpRecv(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufMin, int32_t iBufMax);

// receive all of the response data
int32_t ProtoHttpRecvAll(ProtoHttpRefT *pState, char *pBuffer, int32_t iBufSize);

// initiate transfer of data to to the server via an HTTP POST command
int32_t ProtoHttpPost(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int32_t iDataSize, uint32_t bDoPut);

// send the actual url data
int32_t ProtoHttpSend(ProtoHttpRefT *pState, const char *pData, int32_t iDataSize);

// request deletion of a server-based resource
int32_t ProtoHttpDelete(ProtoHttpRefT *pState, const char *pUrl);

// get server options for specified resource
int32_t ProtoHttpOptions(ProtoHttpRefT *pState, const char *pUrl);

// make an HTTP request
int32_t ProtoHttpRequest(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int32_t iDataSize, ProtoHttpRequestTypeE eRequestType);

// abort current transaction
void ProtoHttpAbort(ProtoHttpRefT *pState); 

// extract a header value from the specified header text
int32_t ProtoHttpGetHeaderValue(ProtoHttpRefT *pState, const char *pHdrBuf, const char *pName, char *pBuffer, int32_t iBufSize, const char **pHdrEnd);

// get next header name and value from header buffer
int32_t ProtoHttpGetNextHeader(ProtoHttpRefT *pState, const char *pHdrBuf, char *pName, int32_t iNameSize, char *pValue, int32_t iValSize, const char **pHdrEnd);

// set base url (used in relative url support)
void ProtoHttpSetBaseUrl(ProtoHttpRefT *pState, const char *pUrl);

// control function; functionality based on input selector
int32_t ProtoHttpControl(ProtoHttpRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// return module status based on input selector
int32_t ProtoHttpStatus(ProtoHttpRefT *pState, int32_t iSelect, void *pBuffer, int32_t iBufSize);

// check whether a request for the given Url would be a keep-alive transaction
int32_t ProtoHttpCheckKeepAlive(ProtoHttpRefT *pState, const char *pUrl);

// give time to module to do its thing (should be called periodically to allow module to perform work)
void ProtoHttpUpdate(ProtoHttpRefT *pState);

// url-encode a integer parameter
int32_t ProtoHttpUrlEncodeIntParm(char *pBuffer, int32_t iLength, const char *pParm, int32_t iValue);

// url-encode a string parameter
int32_t ProtoHttpUrlEncodeStrParm(char *pBuffer, int32_t iLength, const char *pParm, const char *pData);

// parse a url into component parts
const char *ProtoHttpUrlParse(const char *pUrl, char *pKind, int32_t iKindSize, char *pHost, int32_t iHostSize, int32_t *pPort, int32_t *pSecure);

// add an X.509 CA certificate that will be recognized in future transactions
int32_t ProtoHttpSetCACert(const uint8_t *pCACert, int32_t iCertSize);

// same as ProtoHttpSetCACert(), but .pem certificates multiply bundled are parsed bottom to top
int32_t ProtoHttpSetCACert2(const uint8_t *pCACert, int32_t iCertSize);

// validate all CAs that have not already been validated
int32_t ProtoHttpValidateAllCA(void);

// clear all CA certs
void ProtoHttpClrCACerts(void);


#ifdef __cplusplus
}
#endif

//@}        

#endif // _protohttp_h

