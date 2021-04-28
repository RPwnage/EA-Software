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

#include "DirtySDK/proto/protohttputil.h"

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

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct ProtoHttpRefT ProtoHttpRefT;

//! write callback info
#ifndef _PROTOHTTP_WRITECBINFO_DEFINED
typedef struct ProtoHttpWriteCbInfoT
{
    ProtoHttpRequestTypeE eRequestType;
    ProtoHttpResponseE eRequestResponse;
} ProtoHttpWriteCbInfoT;

//! write data callback
typedef int32_t (ProtoHttpWriteCbT)(ProtoHttpRefT *pState, const ProtoHttpWriteCbInfoT *pCbInfo, const char *pData, int32_t iDataSize, void *pUserData);

#define _PROTOHTTP_WRITECBINFO_DEFINED
#endif

//! callback that may be used to customize request header before sending
typedef int32_t (ProtoHttpCustomHeaderCbT)(ProtoHttpRefT *pState, char *pHeader, uint32_t uHeaderSize, const char *pData, int64_t iDataLen, void *pUserRef);

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
int32_t ProtoHttpPost(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize, uint32_t bDoPut);

// send the actual url data
int32_t ProtoHttpSend(ProtoHttpRefT *pState, const char *pData, int32_t iDataSize);

// request deletion of a server-based resource
int32_t ProtoHttpDelete(ProtoHttpRefT *pState, const char *pUrl);

// get server options for specified resource
int32_t ProtoHttpOptions(ProtoHttpRefT *pState, const char *pUrl);

// make an HTTP request
#define ProtoHttpRequest(_pState, _pUrl, _pData, _iDataSize, _eRequestType) ProtoHttpRequestCb(_pState, _pUrl, _pData, _iDataSize, _eRequestType, NULL, NULL)

// make an HTTP request with write callback
int32_t ProtoHttpRequestCb(ProtoHttpRefT *pState, const char *pUrl, const char *pData, int64_t iDataSize, ProtoHttpRequestTypeE eRequestType, ProtoHttpWriteCbT *pWriteCb, void *pWriteCbUserData);

// abort current transaction
void ProtoHttpAbort(ProtoHttpRefT *pState);

// set base url (used in relative url support)
void ProtoHttpSetBaseUrl(ProtoHttpRefT *pState, const char *pUrl);

// get location header (requires state and special processing for relative urls)
int32_t ProtoHttpGetLocationHeader(ProtoHttpRefT *pState, const char *pInpBuf, char *pBuffer, int32_t iBufSize, const char **pHdrEnd);

// control function; functionality based on input selector
int32_t ProtoHttpControl(ProtoHttpRefT *pState, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

// return module status based on input selector
int32_t ProtoHttpStatus(ProtoHttpRefT *pState, int32_t iSelect, void *pBuffer, int32_t iBufSize);

// check whether a request for the given Url would be a keep-alive transaction
int32_t ProtoHttpCheckKeepAlive(ProtoHttpRefT *pState, const char *pUrl);

// give time to module to do its thing (should be called periodically to allow module to perform work)
void ProtoHttpUpdate(ProtoHttpRefT *pState);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protohttp_h

