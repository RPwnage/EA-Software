/*H********************************************************************************/
/*!
    \File restapi.h

    \Description
        Helps serve a restful api using ProtoHttpServ

    \Copyright
        Copyright (c) 2015 Electronic Arts Inc.
*/
/********************************************************************************H*/

#ifndef _restapi_h
#define _restapi_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/proto/protohttpserv.h"
#include "DirtySDK/proto/protohttputil.h"

/*** Type Definitions *************************************************************/

//! opaque module ref
typedef struct RestApiRefT RestApiRefT;

//! request data
typedef struct RestApiRequestDataT
{
    char strAccessToken[8192];  //!< access token data
    void *pResource;            //!< data specific to the resource
    char pBody[1];              //!< dynamic based on how the api is configured max body size (must come last)
} RestApiRequestDataT;

//! response data
typedef struct RestApiResponseDataT
{
    char pBody[1];              //!< dynamic base on how the api is configured max body size (must come last)
} RestApiResponseDataT;

//! resources to configure
typedef struct RestApiResourceDataT
{
    char strPath[256];
    ProtoHttpServRequestCbT *aHandlers[PROTOHTTP_NUMREQUESTTYPES];
} RestApiResourceDataT;

//! complete function type
typedef void (RestApiCompleteCbT)(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData);

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
RestApiRefT *RestApiCreate(uint32_t uPort, const char *pName, int32_t iNumResources, int32_t iMaxBodySize, uint8_t bSecure, uint32_t uFlags);

// update the module
void RestApiUpdate(RestApiRefT *pApi);

// add path/resource to be handled
int32_t RestApiAddResource(RestApiRefT *pApi, const RestApiResourceDataT *pResourceData, void *pUserData);

// set restapi callback function handler
void RestApiCallback(RestApiRefT *pApi, RestApiCompleteCbT *pCompleteCb, void *pUserData);

// control function
int32_t RestApiControl(RestApiRefT *pApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// get request data from protohttpserv request
RestApiRequestDataT *RestApiGetRequestData(const ProtoHttpServRequestT *pRequest);

// get response data from protohttpserv response
RestApiResponseDataT *RestApiGetResponseData(const ProtoHttpServResponseT *pResponse);

// destroy the module
void RestApiDestroy(RestApiRefT *pApi);

#ifdef __cplusplus
}
#endif

#endif // _restapi_h
