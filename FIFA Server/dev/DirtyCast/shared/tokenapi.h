/*H********************************************************************************/
/*!
    \File tokenapi.h

    \Description
        This module wraps the Nucleus OAuth calls used for S2S communcation

    \Copyright
        Copyright (c) Electronic Arts 2015. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

#ifndef _tokenapi_h
#define _tokenapi_h

/*!
\Moduledef TokenApi TokenApi
\Modulemember shared
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"
 
/*** Defines **********************************************************************/

#define TOKENAPI_DATA_MAX_SIZE (8192)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct TokenApiAccessDataT
{
    char strAccessToken[8192];                      //!< access token used for logins
    int32_t iExpiresIn;                             //!< expiration info
} TokenApiAccessDataT;

typedef struct TokenApiInfoDataT
{
    char strClientId[TOKENAPI_DATA_MAX_SIZE];       //!< clientid that was used request the token
    char strScopes[TOKENAPI_DATA_MAX_SIZE];         //!< permissions for the user token
    int32_t iExpiresIn;                             //!< expiration info
} TokenApiInfoDataT;

// opaque module ref
typedef struct TokenApiRefT TokenApiRefT;

// request callback
typedef void (TokenApiAcquisitionCbT)(const TokenApiAccessDataT *pData, void *pUserData);
typedef void (TokenApiInfoQueryCbT)(const char *pAccessToken, const TokenApiInfoDataT *pData, void *pUserData);

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// allocate module state and prepare for use
TokenApiRefT *TokenApiCreate(const char *pCertFile, const char *pKeyFile);

// destroy the module and release its state
void TokenApiDestroy(TokenApiRefT *pRef);

// return module status based on input selector
int32_t TokenApiStatus(TokenApiRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize);

// control function; functionality based on input selector
int32_t TokenApiControl(TokenApiRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// give time to module to do its thing (should be called periodically to allow module to perform work)
void TokenApiUpdate(TokenApiRefT *pRef);

// kick off a new request for access token
int32_t TokenApiAcquisitionAsync(TokenApiRefT *pRef, TokenApiAcquisitionCbT *pCallback, void *pUserData);

// try to get the information from the cache
const TokenApiInfoDataT *TokenApiInfoQuery(TokenApiRefT *pRef, const char *pAccessToken);

// kick of a request for getting token info
int32_t TokenApiInfoQueryAsync(TokenApiRefT *pRef, const char *pAccessToken, TokenApiInfoQueryCbT *pCallback, void *pUserData);

#ifdef __cplusplus
}
#endif

//@}

#endif // _tokenapi_h
