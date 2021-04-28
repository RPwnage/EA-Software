/*H********************************************************************************/
/*!
    \File protohttpxenon.h

    \Description
        TODO

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Version 1.0 01/23/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifdef DIRTYSDK_XHTTP_ENABLED

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

/* TODO - write a description for this */

#ifndef _protohttpxenon_h
#define _protohttpxenon_h

#define ProtoHttpRefT __ProtoHttpRefT
#define ProtoHttpCustomHeaderCbT __ProtoHttpCustomHeaderCbT
#define ProtoHttpReceiveHeaderCbT __ProtoHttpReceiveHeaderCbT
#define ProtoHttpCreate(_iRcvBuf) __ProtoHttpCreate(_iRcvBuf)
#define ProtoHttpCallback(_pState, _pCustomHeaderCb, _pReceiveHeaderCb, _pUserRef) __ProtoHttpCallback(_pState, _pCustomHeaderCb, _pReceiveHeaderCb, _pUserRef)
#define ProtoHttpDestroy(_pState)  __ProtoHttpDestroy(_pState)
#define ProtoHttpGet(_pState, _pUrl, _bHeadOnly) __ProtoHttpGet(_pState, _pUrl, _bHeadOnly)
#define ProtoHttpRecv(_pState, _pBuffer, _iBufMin, _iBufMax) __ProtoHttpRecv(_pState, _pBuffer, _iBufMin, _iBufMax)
#define ProtoHttpRecvAll(_pState, _pBuffer, _iBufSize)  __ProtoHttpRecvAll(_pState, _pBuffer, _iBufSize)
#define ProtoHttpPost(_pState, _pUrl, _pData, _iDataSize, _bDoPut) __ProtoHttpPost(_pState, _pUrl, _pData, _iDataSize, _bDoPut)
#define ProtoHttpSend(_pState, _pData, _iDataSize) __ProtoHttpSend(_pState, _pData, _iDataSize)
#define ProtoHttpDelete(_pState, _pUrl) __ProtoHttpDelete(_pState, _pUrl)
#define ProtoHttpOptions(_pState, _pUrl) __ProtoHttpOptions(_pState, _pUrl)
#define ProtoHttpRequest(_pState, _pUrl, _pData, _iDataSize, _eRequestType) __ProtoHttpRequest(_pState, _pUrl, _pData, _iDataSize, _eRequestType)
#define ProtoHttpAbort(_pState) __ProtoHttpAbort(_pState)
#define ProtoHttpGetHeaderValue(_pState, _pHdrBuf, _pName, _pBuffer, _iBufSize, _pHdrEnd) __ProtoHttpGetHeaderValue(_pState, _pHdrBuf, _pName, _pBuffer, _iBufSize, _pHdrEnd)
#define ProtoHttpGetNextHeader(_pState, _pHdrBuf, _pName, _iNameSize, _pValue, _iValSize, _pHdrEnd) __ProtoHttpGetNextHeader(_pState, _pHdrBuf, _pName, _iNameSize, _pValue, _iValSize, _pHdrEnd)
#define ProtoHttpSetBaseUrl(_pState, _pUrl) __ProtoHttpSetBaseUrl(_pState, _pUrl)
#define ProtoHttpControl(_pState, _iSelect, _iValue, _iValue2, _pValue) __ProtoHttpControl(_pState, _iSelect, _iValue, _iValue2, _pValue)
#define ProtoHttpStatus(_pState, _iSelect, _pBuffer, _iBufSize) __ProtoHttpStatus(_pState, _iSelect, _pBuffer, _iBufSize)
#define ProtoHttpCheckKeepAlive(_pState, _pUrl) __ProtoHttpCheckKeepAlive(_pState, _pUrl)
#define ProtoHttpUpdate(_pState) __ProtoHttpUpdate(_pState)

#define ProtoHttpRequestTypeE __ProtoHttpRequestTypeE
#define PROTOHTTP_REQUESTTYPE_HEAD __PROTOHTTP_REQUESTTYPE_HEAD
#define PROTOHTTP_REQUESTTYPE_GET     __PROTOHTTP_REQUESTTYPE_GET
#define PROTOHTTP_REQUESTTYPE_POST    __PROTOHTTP_REQUESTTYPE_POST
#define PROTOHTTP_REQUESTTYPE_PUT     __PROTOHTTP_REQUESTTYPE_PUT
#define PROTOHTTP_REQUESTTYPE_DELETE  __PROTOHTTP_REQUESTTYPE_DELETE
#define PROTOHTTP_REQUESTTYPE_OPTIONS __PROTOHTTP_REQUESTTYPE_OPTIONS
#define PROTOHTTP_NUMREQUESTTYPES     __PROTOHTTP_NUMREQUESTTYPES

#define ProtoHttpResponseE __ProtoHttpResponseTypeE
#define PROTOHTTP_RESPONSE_INFORMATIONAL __PROTOHTTP_RESPONSE_INFORMATIONAL
#define PROTOHTTP_RESPONSE_CONTINUE __PROTOHTTP_RESPONSE_CONTINUE
#define PROTOHTTP_RESPONSE_SWITCHPROTO __PROTOHTTP_RESPONSE_SWITCHPROTO
#define PROTOHTTP_RESPONSE_SUCCESSFUL __PROTOHTTP_RESPONSE_SUCCESSFUL
#define PROTOHTTP_RESPONSE_OK __PROTOHTTP_RESPONSE_OK
#define PROTOHTTP_RESPONSE_CREATED __PROTOHTTP_RESPONSE_CREATED
#define PROTOHTTP_RESPONSE_ACCEPTED __PROTOHTTP_RESPONSE_ACCEPTED
#define PROTOHTTP_RESPONSE_NONAUTH __PROTOHTTP_RESPONSE_NONAUTH
#define PROTOHTTP_RESPONSE_NOCONTENT __PROTOHTTP_RESPONSE_NOCONTENT
#define PROTOHTTP_RESPONSE_RESETCONTENT __PROTOHTTP_RESPONSE_RESETCONTENT
#define PROTOHTTP_RESPONSE_PARTIALCONTENT __PROTOHTTP_RESPONSE_PARTIALCONTENT
#define PROTOHTTP_RESPONSE_REDIRECTION __PROTOHTTP_RESPONSE_REDIRECTION
#define PROTOHTTP_RESPONSE_MULTIPLECHOICES __PROTOHTTP_RESPONSE_MULTIPLECHOICES
#define PROTOHTTP_RESPONSE_MOVEDPERMANENTLY __PROTOHTTP_RESPONSE_MOVEDPERMANENTLY
#define PROTOHTTP_RESPONSE_FOUND __PROTOHTTP_RESPONSE_FOUND
#define PROTOHTTP_RESPONSE_SEEOTHER __PROTOHTTP_RESPONSE_SEEOTHER
#define PROTOHTTP_RESPONSE_NOTMODIFIED __PROTOHTTP_RESPONSE_NOTMODIFIED
#define PROTOHTTP_RESPONSE_USEPROXY __PROTOHTTP_RESPONSE_USEPROXY
#define PROTOHTTP_RESPONSE_RESERVED __PROTOHTTP_RESPONSE_RESERVED
#define PROTOHTTP_RESPONSE_TEMPREDIRECT __PROTOHTTP_RESPONSE_TEMPREDIRECT
#define PROTOHTTP_RESPONSE_CLIENTERROR __PROTOHTTP_RESPONSE_CLIENTERROR
#define PROTOHTTP_RESPONSE_BADREQUEST __PROTOHTTP_RESPONSE_BADREQUEST
#define PROTOHTTP_RESPONSE_UNAUTHORIZED __PROTOHTTP_RESPONSE_UNAUTHORIZED
#define PROTOHTTP_RESPONSE_PAYMENTREQUIRED __PROTOHTTP_RESPONSE_PAYMENTREQUIRED
#define PROTOHTTP_RESPONSE_FORBIDDEN __PROTOHTTP_RESPONSE_FORBIDDEN
#define PROTOHTTP_RESPONSE_NOTFOUND __PROTOHTTP_RESPONSE_NOTFOUND
#define PROTOHTTP_RESPONSE_METHODNOTALLOWED __PROTOHTTP_RESPONSE_METHODNOTALLOWED
#define PROTOHTTP_RESPONSE_NOTACCEPTABLE __PROTOHTTP_RESPONSE_NOTACCEPTABLE
#define PROTOHTTP_RESPONSE_PROXYAUTHREQ __PROTOHTTP_RESPONSE_PROXYAUTHREQ
#define PROTOHTTP_RESPONSE_REQUESTTIMEOUT __PROTOHTTP_RESPONSE_REQUESTTIMEOUT
#define PROTOHTTP_RESPONSE_CONFLICT __PROTOHTTP_RESPONSE_CONFLICT
#define PROTOHTTP_RESPONSE_GONE __PROTOHTTP_RESPONSE_GONE
#define PROTOHTTP_RESPONSE_LENGTHREQUIRED __PROTOHTTP_RESPONSE_LENGTHREQUIRED
#define PROTOHTTP_RESPONSE_PRECONFAILED __PROTOHTTP_RESPONSE_PRECONFAILED
#define PROTOHTTP_RESPONSE_REQENTITYTOOLARGE __PROTOHTTP_RESPONSE_REQENTITYTOOLARGE
#define PROTOHTTP_RESPONSE_REQURITOOLONG __PROTOHTTP_RESPONSE_REQURITOOLONG
#define PROTOHTTP_RESPONSE_UNSUPPORTEDMEDIA __PROTOHTTP_RESPONSE_UNSUPPORTEDMEDIA
#define PROTOHTTP_RESPONSE_REQUESTRANGE __PROTOHTTP_RESPONSE_REQUESTRANGE
#define PROTOHTTP_RESPONSE_EXPECTATIONFAILED __PROTOHTTP_RESPONSE_EXPECTATIONFAILED
#define PROTOHTTP_RESPONSE_SERVERERROR __PROTOHTTP_RESPONSE_SERVERERROR
#define PROTOHTTP_RESPONSE_INTERNALSERVERERROR __PROTOHTTP_RESPONSE_INTERNALSERVERERROR
#define PROTOHTTP_RESPONSE_NOTIMPLEMENTED __PROTOHTTP_RESPONSE_NOTIMPLEMENTED
#define PROTOHTTP_RESPONSE_BADGATEWAY __PROTOHTTP_RESPONSE_BADGATEWAY
#define PROTOHTTP_RESPONSE_SERVICEUNAVAILABLE __PROTOHTTP_RESPONSE_SERVICEUNAVAILABLE
#define PROTOHTTP_RESPONSE_GATEWAYTIMEOUT __PROTOHTTP_RESPONSE_GATEWAYTIMEOUT
#define PROTOHTTP_RESPONSE_HTTPVERSUNSUPPORTED __PROTOHTTP_RESPONSE_HTTPVERSUNSUPPORTED

#else
#undef _protohttpxenon_h
#undef _protohttp_h

#undef ProtoHttpRefT
#undef ProtoHttpCustomHeaderCbT
#undef ProtoHttpReceiveHeaderCbT
#undef ProtoHttpCreate
#undef ProtoHttpCallback
#undef ProtoHttpDestroy
#undef ProtoHttpGet
#undef ProtoHttpRecv
#undef ProtoHttpRecvAll
#undef ProtoHttpPost
#undef ProtoHttpSend
#undef ProtoHttpDelete
#undef ProtoHttpOptions
#undef ProtoHttpRequest
#undef ProtoHttpAbort
#undef ProtoHttpGetHeaderValue
#undef ProtoHttpGetNextHeader
#undef ProtoHttpSetBaseUrl
#undef ProtoHttpControl
#undef ProtoHttpStatus
#undef ProtoHttpCheckKeepAlive
#undef ProtoHttpUpdate

#undef ProtoHttpRequestTypeE
#undef PROTOHTTP_REQUESTTYPE_HEAD
#undef PROTOHTTP_REQUESTTYPE_GET
#undef PROTOHTTP_REQUESTTYPE_POST
#undef PROTOHTTP_REQUESTTYPE_PUT
#undef PROTOHTTP_REQUESTTYPE_DELETE
#undef PROTOHTTP_REQUESTTYPE_OPTIONS
#undef PROTOHTTP_NUMREQUESTTYPES

#undef ProtoHttpResponseE
#undef PROTOHTTP_RESPONSE_INFORMATIONAL
#undef PROTOHTTP_RESPONSE_CONTINUE
#undef PROTOHTTP_RESPONSE_SWITCHPROTO
#undef PROTOHTTP_RESPONSE_SUCCESSFUL
#undef PROTOHTTP_RESPONSE_OK
#undef PROTOHTTP_RESPONSE_CREATED
#undef PROTOHTTP_RESPONSE_ACCEPTED
#undef PROTOHTTP_RESPONSE_NONAUTH
#undef PROTOHTTP_RESPONSE_NOCONTENT
#undef PROTOHTTP_RESPONSE_RESETCONTENT
#undef PROTOHTTP_RESPONSE_PARTIALCONTENT
#undef PROTOHTTP_RESPONSE_REDIRECTION
#undef PROTOHTTP_RESPONSE_MULTIPLECHOICES
#undef PROTOHTTP_RESPONSE_MOVEDPERMANENTLY
#undef PROTOHTTP_RESPONSE_FOUND
#undef PROTOHTTP_RESPONSE_SEEOTHER
#undef PROTOHTTP_RESPONSE_NOTMODIFIED
#undef PROTOHTTP_RESPONSE_USEPROXY
#undef PROTOHTTP_RESPONSE_RESERVED
#undef PROTOHTTP_RESPONSE_TEMPREDIRECT
#undef PROTOHTTP_RESPONSE_CLIENTERROR
#undef PROTOHTTP_RESPONSE_BADREQUEST
#undef PROTOHTTP_RESPONSE_UNAUTHORIZED
#undef PROTOHTTP_RESPONSE_PAYMENTREQUIRED
#undef PROTOHTTP_RESPONSE_FORBIDDEN
#undef PROTOHTTP_RESPONSE_NOTFOUND
#undef PROTOHTTP_RESPONSE_METHODNOTALLOWED
#undef PROTOHTTP_RESPONSE_NOTACCEPTABLE
#undef PROTOHTTP_RESPONSE_PROXYAUTHREQ
#undef PROTOHTTP_RESPONSE_REQUESTTIMEOUT
#undef PROTOHTTP_RESPONSE_CONFLICT
#undef PROTOHTTP_RESPONSE_GONE
#undef PROTOHTTP_RESPONSE_LENGTHREQUIRED
#undef PROTOHTTP_RESPONSE_PRECONFAILED
#undef PROTOHTTP_RESPONSE_REQENTITYTOOLARGE
#undef PROTOHTTP_RESPONSE_REQURITOOLONG
#undef PROTOHTTP_RESPONSE_UNSUPPORTEDMEDIA
#undef PROTOHTTP_RESPONSE_REQUESTRANGE
#undef PROTOHTTP_RESPONSE_EXPECTATIONFAILED
#undef PROTOHTTP_RESPONSE_SERVERERROR
#undef PROTOHTTP_RESPONSE_INTERNALSERVERERROR
#undef PROTOHTTP_RESPONSE_NOTIMPLEMENTED
#undef PROTOHTTP_RESPONSE_BADGATEWAY
#undef PROTOHTTP_RESPONSE_SERVICEUNAVAILABLE
#undef PROTOHTTP_RESPONSE_GATEWAYTIMEOUT
#undef PROTOHTTP_RESPONSE_HTTPVERSUNSUPPORTED

#endif

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#endif // DIRTYSDK_XHTTP_ENABLED