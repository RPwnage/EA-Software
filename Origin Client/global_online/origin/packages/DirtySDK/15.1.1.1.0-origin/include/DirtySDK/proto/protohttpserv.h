/*H********************************************************************************/
/*!
    \File protohttpserv.h

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

#ifndef _protohttpserv_h
#define _protohttpserv_h

/*!
\Moduledef ProtoHttpServ ProtoHttpServ
\Modulemember Proto
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/proto/protohttputil.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! request info to pass to/from request callback
typedef struct ProtoHttpServRequestInfoT
{
    int64_t iDataSize;
    uint32_t uModTime;
    int32_t iResponseCode;
    //content type
    int32_t iChunkLength;
    void *pRequestData;
} ProtoHttpServRequestInfoT;

//! request callback type
typedef int32_t (ProtoHttpServRequestCbT)(ProtoHttpRequestTypeE eRequestType, const char *pUrl, const char *pHeader, ProtoHttpServRequestInfoT *pRequestInfo, void *pUserData);

//! data read callback type
typedef int32_t (ProtoHttpServReadCbT)(char *pBuffer, int32_t iBufSize, void *pRequestData, void *pUserData);

//! data write callback type
typedef int32_t (ProtoHttpServWriteCbT)(const char *pBuffer, int32_t iBufSize, void *pRequestData, void *pUserData);

//! logging function type
typedef int32_t (ProtoHttpServLogCbT)(const char *pText, void *pUserData);

//! opaque module ref
typedef struct ProtoHttpServRefT ProtoHttpServRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//! create an httpserv module
DIRTYCODE_API ProtoHttpServRefT *ProtoHttpServCreate(int32_t iPort, int32_t iSecure, const char *pName);

//! destroy an httpserv module
DIRTYCODE_API void ProtoHttpServDestroy(ProtoHttpServRefT *pHttpServ);

//! set httpserv callback function handlers (required)
DIRTYCODE_API void ProtoHttpServCallback(ProtoHttpServRefT *pHttpServ, ProtoHttpServRequestCbT *pRequestCb, ProtoHttpServReadCbT *pReadCb, ProtoHttpServWriteCbT *pWriteCb, ProtoHttpServLogCbT *pLogCb, void *pUserData);

//! control function; functionality based on input selector
DIRTYCODE_API int32_t ProtoHttpServControl(ProtoHttpServRefT *pHttpServ, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue);

//! return module status based on input selector
DIRTYCODE_API int32_t ProtoHttpServStatus(ProtoHttpServRefT *pHttpServ, int32_t iSelect, void *pBuffer, int32_t iBufSize);

//! update the module
DIRTYCODE_API void ProtoHttpServUpdate(ProtoHttpServRefT *pHttpServ);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protohttpserv_h


