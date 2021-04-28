/*H********************************************************************************/
/*!
    \File protohttpxenon.h

    \Description
        Implements the ProtoHttp API, allowing use of either the generic ProtoHttp
        code used on other platforms or the XHTTP API that is Xenon-specific.
        Determination of which API to use can be made at run-time on a per-instance
        basis.  The default API is the generic ProtoHttp implementation; the 'xhtp'
        control selector is used to switch between implementations.

    \Notes
       The goal of protohttpxenon is to accomplish the following set of requirements:
         - Wrap the Xenon-specific XHTTP API in the ProtoHttp interface.
         - Continue to allow use of the generic ProtoHttp interface.
         - Have XHTTP/ProtoHttp use selectable at run-time on a per-instance basis.
         - Not require modification of the generic ProtoHttp code to accomplish this.

       To accomplish this goal, a somewhat complicated solution was implemented.  The
       protohttpxenon.c file includes protohttpxenon.h (and protohttp) twice. The first
       pass defines an alternate namespace for the 'generic' ProtoHttp functions, by
       using preprocessor defines to map ProtoHttpSomething to __ProtoHttpSomething.
       When protohttp is subsequently included, the ProtoHttp public interface is
       remapped to an underscore-prefixed version.

       The second pass undefines these mappings and re-includes protohttp.h.  This
       second pass restores the public API as owned by the protohttpxenon implementation
       of the public API.

       The end result is that calls to ProtoHttp-namespace functions are handled
       by protohttpxenon, but protohttpxenon is able to access the generic API by
       using the underscore-prefixed names.  It is therefore able to select at
       run-time, on a per-instance basis, which version of the API it is going
       to use, without needing to modify the generic code.

    \Copyright
        Copyright (c) Electronic Arts 2012.

    \Version 1.0 01/23/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifdef DIRTYSDK_XHTTP_ENABLED

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#ifndef _protohttpxenon_h
#define _protohttpxenon_h

// first pass: define alternate namespace for public/generic ProtoHttp API
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
#define ProtoHttpRequestCb(_pState, _pUrl, _pData, _iDataSize, _eRequestType, _pWriteCb, _pWriteCbUserData) __ProtoHttpRequestCb(_pState, _pUrl, _pData, _iDataSize, _eRequestType, _pWriteCb, _pWriteCbUserData)
#define ProtoHttpAbort(_pState) __ProtoHttpAbort(_pState)
#define ProtoHttpGetLocationHeader(_pState, _pInpBuf, _pBuffer, _iBufSize, _pHdrEnd) __ProtoHttpGetLocationHeader(_pState, _pInpBuf, _pBuffer, _iBufSize, _pHdrEnd)
#define ProtoHttpSetBaseUrl(_pState, _pUrl) __ProtoHttpSetBaseUrl(_pState, _pUrl)
#define ProtoHttpControl(_pState, _iSelect, _iValue, _iValue2, _pValue) __ProtoHttpControl(_pState, _iSelect, _iValue, _iValue2, _pValue)
#define ProtoHttpStatus(_pState, _iSelect, _pBuffer, _iBufSize) __ProtoHttpStatus(_pState, _iSelect, _pBuffer, _iBufSize)
#define ProtoHttpCheckKeepAlive(_pState, _pUrl) __ProtoHttpCheckKeepAlive(_pState, _pUrl)
#define ProtoHttpUpdate(_pState) __ProtoHttpUpdate(_pState)

#else

// second pass: undefine alternate namespace for native protohttpxenon implementation
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
#undef ProtoHttpRequestCb
#undef ProtoHttpAbort
#undef ProtoHttpSetBaseUrl
#undef ProtoHttpGetLocationHeader
#undef ProtoHttpControl
#undef ProtoHttpStatus
#undef ProtoHttpCheckKeepAlive
#undef ProtoHttpUpdate

#endif

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#endif // DIRTYSDK_XHTTP_ENABLED