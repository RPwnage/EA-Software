/*H********************************************************************************/
/*!
    \File restapi.c

    \Description
       Helps serve a restful api using ProtoHttpServ

    \Copyright
        Copyright (c) 2015-2017 Electronic Arts Inc.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_PC)
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "dirtycast.h"
#include "restapi.h"

/*** Defines **********************************************************************/

#define RESTAPI_MEMID          ('capi')

/*** Type Definitions *************************************************************/

//! resource data as we use it internally
typedef struct RestApiResourceT
{
    RestApiResourceDataT ResourceData;
    void *pUserData;
} RestApiResourceT;

//! module state
struct RestApiRefT
{
    ProtoHttpServRefT *pProtoHttpServ;      //!< our internal http/1.1 server
    uint8_t uDebugLevel;                    //!< debug level
    uint8_t bContentBin;                    //!< is content type binary?
    uint8_t _pad[2];

    //! allocation information
    int32_t iMemGroup;
    void *pMemGroupUserdata;

    //! certificate data for secure mode
    char *pCert;
    char *pKey;
    int32_t iCertLen;
    int32_t iKeyLen;

    char strContentType[256];               //!< supported content type
    char strAuthScheme[64];                 //!< scheme we use for authenication
    int32_t iMaxBodySize;                   //!< max body size for inbound/outbound buffers
    int32_t iNumResources;                  //!< number of RestApiResourceT
    RestApiResourceT *pResources;           //!< dynamic sized array of resources
};

/*** Varibles *********************************************************************/

//! http request names
static const char *_RestApi_strMethodNames[PROTOHTTP_NUMREQUESTTYPES] =
{
    "HEAD", "GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH", "CONNECT"
};

/*** Private functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _RestApiMatch

    \Description
        Match our resource and url based on a regular expression

    \Input *pUrl        - the data we are matching our regex on
    \Input *pResource   - regex we are using to match

    \Output
        int32_t         - zero=success, non-zero=error

    \Notes
        To allow the system to provide a better way to register handlers
        we added this function to support matching on a regular expression

    \Version 03/11/2016 (eesponda)
*/
/********************************************************************************F*/
static int32_t _RestApiMatch(const char *pUrl, const char *pResource)
{
    int32_t iResult;
    regex_t Re;

    if (regcomp(&Re, pResource, REG_EXTENDED | REG_NOSUB) != 0)
    {
        DirtyCastLogPrintf("restapi: failed to create regex parser\n");
        return(-1);
    }

    iResult = regexec(&Re, pUrl, 0, NULL, 0);
    regfree(&Re);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _RestApiCheckResource

    \Description
        Finds the index of the resource handling data

    \Input *pApi        - module state
    \Input *pRequest    - the request data using to figuring out which resource we need
    \Input *pResponse   - the response data (for updating return code)

    \Output
        int32_t         - positive=success, negative=error
*/
/********************************************************************************F*/
static int32_t _RestApiCheckResource(RestApiRefT *pApi, const ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse)
{
    int32_t iIndex;
    for (iIndex = 0; iIndex < pApi->iNumResources; iIndex += 1)
    {
        RestApiResourceDataT *pResource = &pApi->pResources[iIndex].ResourceData;

        // check if we support the resource path
        if (_RestApiMatch(pRequest->strUrl, pResource->strPath) != 0)
        {
            continue;
        }

        // check if a handler is registered for this method
        if (pResource->aHandlers[pRequest->eRequestType] != NULL)
        {
            return(iIndex);
        }

        DirtyCastLogPrintf("restapi: [%p] method %s not supported for resource %s\n", pRequest, _RestApi_strMethodNames[pRequest->eRequestType], pRequest->strUrl);

        pResponse->eResponseCode = PROTOHTTP_RESPONSE_METHODNOTALLOWED;

        // build a list of allowed methods to send back
        pResponse->iHeaderLen += ds_snzprintf(pResponse->strHeader+pResponse->iHeaderLen, sizeof(pResponse->strHeader)-pResponse->iHeaderLen, "Allow: ");
        for (iIndex = 0; iIndex < PROTOHTTP_NUMREQUESTTYPES; iIndex += 1)
        {
            if (pResource->aHandlers[iIndex] == NULL)
            {
                continue;
            }
            pResponse->iHeaderLen += ds_snzprintf(pResponse->strHeader+pResponse->iHeaderLen, sizeof(pResponse->strHeader)-pResponse->iHeaderLen, "%s, ",
                _RestApi_strMethodNames[iIndex]);
        }
        // trim ', '
        pResponse->iHeaderLen -= 2;
        pResponse->strHeader[pResponse->iHeaderLen] = '\0';

        return(-1);
    }

    DirtyCastLogPrintf("restapi: [%p] registration not found for resource %s\n", pRequest, pRequest->strUrl);

    pResponse->eResponseCode = PROTOHTTP_RESPONSE_NOTIMPLEMENTED;
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _RestApiHeaderCb

    \Description
        Checks the request header data to see if we want to move forward with
        handling the request

    \Input *pRequest    - data about the request
    \Input *pResponse   - data on how we want to respond
    \Input *pUserData   - user data

    \Output
        int32_t         - zero=success, negative=error
*/
/********************************************************************************F*/
static int32_t _RestApiHeaderCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    int32_t iResult;
    char strName[128], strValue[8192], strAccessToken[8192];
    char *pResponseBody;
    RestApiRequestDataT *pRequestData;
    const char *pHdr = pRequest->strHeader;
    RestApiRefT *pApi = (RestApiRefT *)pUserData;
    int32_t iMemSize = sizeof(*pRequestData) - sizeof(char) + pApi->iMaxBodySize;

    // check to make sure the method / resource is supported
    if ((iResult = _RestApiCheckResource(pApi, pRequest, pResponse)) < 0)
    {
        return(-1);
    }

    // make sure we have a body for put or post
    if ((pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_PUT) || (pRequest->eRequestType == PROTOHTTP_REQUESTTYPE_POST))
    {
        if (pRequest->iContentLength < 0)
        {
            DirtyCastLogPrintf("restapi: [%p] chunked encoding not supported\n", pRequest);

            /* normally we would put a more descriptive error for 400 but this
               is a case I don't expect to ever happen */
            pResponse->eResponseCode = PROTOHTTP_RESPONSE_CLIENTERROR;  // (400 error)
            return(-1);
        }
        else if (pRequest->iContentLength == 0)
        {
            DirtyCastLogPrintf("restapi: [%p] PUT/POST request without a body\n", pRequest);

            /* normally we would put a more descriptive error for 400 but this
               is a case I don't expect to ever happen */
            pResponse->eResponseCode = PROTOHTTP_RESPONSE_CLIENTERROR;  // (400 error)
            return(-1);
        }
    }

    /* to insure that we are not left open to attacks we validate the size of the payload and check to
       make sure it is within reason */
    if (pRequest->iContentLength > pApi->iMaxBodySize)
    {
        DirtyCastLogPrintf("restapi: [%p] request content length larger than maximum configured limit (size=%d, max=%d)\n", pRequest, pRequest->iContentLength, pApi->iMaxBodySize);

        pResponse->eResponseCode = PROTOHTTP_RESPONSE_REQENTITYTOOLARGE; // (413 error)
        return(-1);
    }

    // make sure we are using the correct format for the payload, if it empty any content type is accepted
    if (pRequest->iContentLength > 0)
    {
        if ((pApi->strContentType[0] != '\0') && (ds_stristr(pRequest->strContentType, pApi->strContentType) == NULL))
        {
            DirtyCastLogPrintf("restapi: [%p] request uses unsupported content type %s\n", pRequest, pRequest->strContentType);

            pResponse->eResponseCode = PROTOHTTP_RESPONSE_UNSUPPORTEDMEDIA; // (415 error)
            return(-1);
        }
    }

    // just in case it isn't found
    ds_memclr(strAccessToken, sizeof(strAccessToken));

    // currently the only additional we process is in the case authorization is required
    if (pApi->strAuthScheme[0] != '\0')
    {
        const char *pStart;

        while ((iResult = ProtoHttpGetNextHeader(NULL, pHdr, strName, sizeof(strName), strValue, sizeof(strValue), &pHdr)) == 0)
        {
            if (ds_stricmp(strName, "authorization") == 0)
            {
                ds_strnzcpy(strAccessToken, strValue, sizeof(strAccessToken));
            }
        }
        if ((strAccessToken[0] == '\0') || ((pStart = ds_stristr(strAccessToken, pApi->strAuthScheme)) == NULL))
        {
            DirtyCastLogPrintf("restapi: [%p] request has incorrect scheme or missing access token\n", pRequest);

            pResponse->iHeaderLen += ds_snzprintf(pResponse->strHeader, sizeof(pResponse->strHeader)-pResponse->iHeaderLen, "WWW-Authenticate: %s\r\n",
                pApi->strAuthScheme);
            pResponse->eResponseCode = PROTOHTTP_RESPONSE_UNAUTHORIZED;
            return(-1);
        }

        // skip the authorization scheme
        for (pStart += strlen(pApi->strAuthScheme); *pStart == ' '; pStart += 1)
            ;
        memmove(strAccessToken, pStart, strlen(pStart)+1);
    }

    // allocate request data storage
    if ((pRequestData = (RestApiRequestDataT *)DirtyMemAlloc(iMemSize, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("restapi: [%p] failed to allocate space for request data\n", pRequest);

        pResponse->eResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-1);
    }
    ds_memclr(pRequestData, iMemSize);
    ds_strnzcpy(pRequestData->strAccessToken, strAccessToken, sizeof(pRequestData->strAccessToken));

    // allocate response data storage
    if ((pResponseBody = (char *)DirtyMemAlloc(pApi->iMaxBodySize, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata)) == NULL)
    {
        DirtyCastLogPrintf("restapi: [%p] failed to allocate space for response body\n", pResponse);

        // clear the previously allocated buffer
        DirtyMemFree(pRequestData, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        pResponse->eResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-1);
    }
    ds_memclr(pResponseBody, pApi->iMaxBodySize);

    // save pointers
    pRequest->pData = pRequestData;
    pResponse->pData = pResponseBody;

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _RestApiRequestCb

    \Description
        Callback to handle the request

    \Input *pRequest    - data about the request
    \Input *pResponse   - data on how we want to respond
    \Input *pUserData   - user data

    \Output
        int32_t         - zero=success, positive=in-progress, negative=error
*/
/********************************************************************************F*/
static int32_t _RestApiRequestCb(ProtoHttpServRequestT *pRequest, ProtoHttpServResponseT *pResponse, void *pUserData)
{
    int32_t iResult;
    ProtoHttpServRequestCbT *pCallback;
    void* pCbUserData;
    RestApiRefT *pApi = (RestApiRefT *)pUserData;

    // check to make our data is valid
    if (((pRequest->iContentLength > 0) && (pRequest->pData == NULL)) || (pResponse->pData == NULL))
    {
        DirtyCastLogPrintf("restapi: [%p] expected data but no data allocated\n", pRequest);

        pResponse->eResponseCode = PROTOHTTP_RESPONSE_INTERNALSERVERERROR;
        return(-1);
    }

    // get the index
    if ((iResult = _RestApiCheckResource(pApi, pRequest, pResponse)) < 0)
    {
        return(-1);
    }
    pCallback = pApi->pResources[iResult].ResourceData.aHandlers[pRequest->eRequestType];
    pCbUserData = pApi->pResources[iResult].pUserData;

    // fire the callback
    if ((iResult = pCallback(pRequest, pResponse, pCbUserData)) > 0)
    {
        return(iResult);
    }

    // print the response
    if ((pResponse->iContentLength > 0) && (pApi->uDebugLevel > 1))
    {
        NetPrintf(("restapi: [%p] sending response of %d bytes\n", pRequest, (int32_t)pResponse->iContentLength));
        if (pApi->bContentBin == FALSE)
        {
            NetPrintWrap((char *)pResponse->pData, 132);
        }
    }

    // if an error occured or nothing to send, clear the memory
    if (pResponse->iContentLength <= 0)
    {
        DirtyMemFree(pRequest->pData, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        DirtyMemFree(pResponse->pData, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        pRequest->pData = pResponse->pData = NULL;
    }
    // otherwise just clear the request
    else
    {
        // if we have a response then set the content type
        ds_strnzcpy(pResponse->strContentType, pApi->strContentType, sizeof(pResponse->strContentType));

        DirtyMemFree(pRequest->pData, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        pRequest->pData = NULL;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _RestApiReceiveCb

    \Description
        Callback to read request data

    \Input *pResponse   - data about the request
    \Input *pBuffer     - input buffer to read request data from
    \Input iBufSize     - the size of the input buffer
    \Input *pUserData   - user data

    \Output
        int32_t         - positive=amount of data read, negative=error
*/
/********************************************************************************F*/
static int32_t _RestApiReceiveCb(ProtoHttpServRequestT *pRequest, const char *pBuffer, int32_t iBufSize, void *pUserData)
{
    RestApiRefT *pApi = (RestApiRefT *)pUserData;
    RestApiRequestDataT *pRequestData = (RestApiRequestDataT *)pRequest->pData;

    // if the buffer is NULL or buf size is zero or less, do the cleanup
    if ((pBuffer == NULL) || (iBufSize <= 0))
    {
        // print the request body
        if ((pRequest->iContentLength > 0) && (pApi->uDebugLevel > 1))
        {
            NetPrintf(("restapi: [%p] received request of %d bytes\n", pRequest, (int32_t)pRequest->iContentLength));
            if (pApi->bContentBin == FALSE)
            {
                NetPrintWrap(pRequestData->pBody, 132);
            }
        }

        return(0);
    }

    // if the data was not allocated, something went wrong
    if (pRequest->pData == NULL)
    {
        DirtyCastLogPrintf("restapi: [%p] data incoming but no space allocated to write to\n", pRequest);
        return(-1);
    }

    // concat to the end
    iBufSize = DS_MIN(iBufSize, pRequest->iContentLength-pRequest->iContentRecv);
    ds_memcpy(pRequestData->pBody+pRequest->iContentRecv, pBuffer, iBufSize);
    return(iBufSize);
}

/*F********************************************************************************/
/*!
    \Function _RestApiSendCb

    \Description
        Callback to send response data

    \Input *pResponse   - data about the response
    \Input *pBuffer     - output buffer to write response to
    \Input iBufSize     - the size of the output buffer
    \Input *pUserData   - user data

    \Output
        int32_t         - positive=amount of data written, negative=error
*/
/********************************************************************************F*/
static int32_t _RestApiSendCb(ProtoHttpServResponseT *pResponse, char *pBuffer, int32_t iBufSize, void *pUserData)
{
    RestApiRefT *pApi = (RestApiRefT *)pUserData;
    char *pBody = (char *)pResponse->pData;

    // if the buffer is NULL or buf size is zero or less, do the cleanup
    if ((pBuffer == NULL) || (iBufSize <= 0))
    {
        DirtyMemFree(pResponse->pData, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        pResponse->pData = NULL;
        return(0);
    }

    // check that the buffer is valid
    if (pBody == NULL)
    {
        DirtyCastLogPrintf("restapi: [%p] data outgoing but no data allocated to read from\n", pResponse);
        return(-1);
    }

    // copy the response into the buffer and increment the position
    iBufSize = DS_MIN(iBufSize, pResponse->iContentLength-pResponse->iContentRead);
    ds_memcpy(pBuffer, pBody+pResponse->iContentRead, iBufSize);
    return(iBufSize);
}

/*F********************************************************************************/
/*!
    \Function _RestApiLogCb

    \Description
        Logging function for ProtoHttpServ

    \Input *pText       - text to log
    \Input *pUserData   - user data
*/
/********************************************************************************F*/
static void _RestApiLogCb(const char *pText, void *pUserData)
{
    DirtyCastLogPrintf("%s", pText);
}

/*F********************************************************************************/
/*!
    \Function _RestApiLoadCert

    \Description
        Loads a certificate file and returns the buffer with the data

    \Input *pRef            - module state
    \Input *strFilename     - name of the certificate/key being loaded
    \Input *pSize           - size of the file
    \Input *strBegin        - string that denotes beginning of certificate data
    \Input *strEnd          - string that denotes end of certificate data

    \Output
        char*        - pBuffer if loaded, NULL otherwise
*/
/********************************************************************************F*/
static char* _RestApiLoadCert(RestApiRefT *pRef, const char *strFilename, int32_t *pSize, const char *strBegin, const char *strEnd)
{
    // Used for file work
    FILE* pFd = NULL;
    uint32_t uFilesize = 0;

    // Start and End data for the certificate
    char* pStart = NULL;
    char* pEnd = NULL;

    // Output
    char* pBuffer = NULL;

    *pSize = 0;

    if ((pFd = fopen(strFilename, "rb")) == NULL)
    {
        // Cannot open file;
        return(NULL);
    }

    // get the file size:
    fseek(pFd, 0, SEEK_END);
    uFilesize = ftell(pFd);
    rewind(pFd);

    if ((pBuffer = (char*)DirtyMemAlloc(uFilesize + 1, RESTAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserdata)) == NULL)
    {
        // Allocation failed;
        return(NULL);
    }

    // Clear the buffer
    ds_memclr(pBuffer, uFilesize + 1);

    if (fread(pBuffer, 1, uFilesize, pFd) != uFilesize)
    {
        DirtyMemFree(pBuffer, RESTAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserdata);
        return(NULL);
    }

    if ((pStart = ds_stristr(pBuffer, strBegin)) == NULL)
    {
        DirtyMemFree(pBuffer, RESTAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserdata);
        return(NULL);
    }

    // skip the certificate data
    for (pStart += strlen(strBegin); (*pStart == '\r') || (*pStart == '\n') || (*pStart == ' '); pStart += 1)
        ;

    // remove begin signature from buffer
    memmove(pBuffer, pStart, strlen(pStart) + 1);

    if ((pEnd = ds_stristr(pBuffer, strEnd)) == NULL)
    {
        DirtyMemFree(pBuffer, RESTAPI_MEMID, pRef->iMemGroup, pRef->pMemGroupUserdata);
        return(NULL);
    }

    for (pEnd -= 1; *pEnd == '\r' || *pEnd == '\n' || *pEnd == ' '; pEnd -= 1)
        ;

    pEnd[1] = '\0';

    // calculate length
    *pSize = pEnd - pBuffer + 1;

    return(pBuffer);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function RestApiCreate

    \Description
        Creates the REST API module

    \Input uPort        - The port to listen for requests on
    \Input *pName       - Name to identify the server
    \Input iNumResources- How many resources this module needs to handled
    \Input iMaxBodySize - max inbound/outbound body size
    \Input bSecure      - http/https?

    \Output
        RestApiRefT *  - module ref or NULL
*/
/********************************************************************************F*/
RestApiRefT *RestApiCreate(uint32_t uPort, const char *pName, int32_t iNumResources, int32_t iMaxBodySize, uint8_t bSecure, uint32_t uFlags)
{
    RestApiRefT *pApi;
    ProtoHttpServRefT *pProtoHttpServ;
    RestApiResourceT *pResources;
    int32_t iMemGroup;
    void *pMemGroupUserdata;

    if (iMaxBodySize <= 0)
    {
        DirtyCastLogPrintf("restapi: invalid body size\n");
        return(NULL);
    }

    // query allocation info
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserdata);

    // create protohttpserv instance
    if ((pProtoHttpServ = ProtoHttpServCreate2(uPort, bSecure, pName, uFlags)) == NULL)
    {
        DirtyCastLogPrintf("restapi: unable to create protohttpserv module\n");
        return(NULL);
    }

    // allocate resources
    if ((pResources = (RestApiResourceT *)DirtyMemAlloc(sizeof(RestApiResourceT) * iNumResources, RESTAPI_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        ProtoHttpServDestroy(pProtoHttpServ);
        return(NULL);
    }
    ds_memclr(pResources, sizeof(RestApiResourceT) * iNumResources);

    // allocate the module state
    if ((pApi = (RestApiRefT *)DirtyMemAlloc(sizeof(RestApiRefT), RESTAPI_MEMID, iMemGroup, pMemGroupUserdata)) == NULL)
    {
        ProtoHttpServDestroy(pProtoHttpServ);
        DirtyMemFree(pResources, RESTAPI_MEMID, iMemGroup, pMemGroupUserdata);
        return(NULL);
    }
    ds_memclr(pApi, sizeof(RestApiRefT));
    pApi->pProtoHttpServ = pProtoHttpServ;
    pApi->iMemGroup = iMemGroup;
    pApi->pMemGroupUserdata = pMemGroupUserdata;
    pApi->iMaxBodySize = iMaxBodySize;
    pApi->iNumResources = iNumResources;
    pApi->pResources = pResources;

    // configure callbacks
    ProtoHttpServCallback(pProtoHttpServ, _RestApiRequestCb, _RestApiReceiveCb, _RestApiSendCb, _RestApiHeaderCb, _RestApiLogCb, pApi);

    return(pApi);
}

/*F********************************************************************************/
/*!
    \Function RestApiUpdate

    \Description
        Updates the module

    \Input *pApi    - instance of module
*/
/********************************************************************************F*/
void RestApiUpdate(RestApiRefT *pApi)
{
    ProtoHttpServUpdate(pApi->pProtoHttpServ);
}

/*F********************************************************************************/
/*!
    \Function RestApiAddResource

    \Description
        Adds a resource to be handled

    \Input *pApi            - instance of module
    \Input *pResourceData   - The data for the resource we are adding
    \Input *pUserData       - User specific data to pass to the callbacks

    \Output
        int32_t -   index of resource, negative=error
*/
/********************************************************************************F*/
int32_t RestApiAddResource(RestApiRefT *pApi, const RestApiResourceDataT *pResourceData, void *pUserData)
{
    int32_t iIndex;
    for (iIndex = 0; iIndex < pApi->iNumResources; iIndex += 1)
    {
        RestApiResourceDataT *pResource = &pApi->pResources[iIndex].ResourceData;
        if (pResource->strPath[0] == '\0')
        {
            ds_memcpy(pResource, pResourceData, sizeof(RestApiResourceDataT));
            pApi->pResources[iIndex].pUserData = pUserData;
            return(iIndex);
        }
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function RestApiControl

    \Description
        Control function, different selectors control different behaviours

    \Input *pApi            - instance of module
    \Input iControl         - control selector (see notes)
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t -   selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'asch'      sets the scheme we use for auth, ignored if we do not require auth
        'cont'      sets the content type (from pValue) and if it is binary (from iValue)
        'mbdy'      sets the max body size
        'scrt'      loads the cert file passed via pValue and passes it to ProtoHttpServControl
        'skey'      loads the key file passed via pValue and passed it to ProtoHttpServControl
        'spam'      sets the debug level (via iValue)
    \endverbatim

        If unhandled, it will fall-through into ProtoHttpServControl
*/
/********************************************************************************F*/
int32_t RestApiControl(RestApiRefT *pApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'asch')
    {
        ds_strnzcpy(pApi->strAuthScheme, (const char *)pValue, sizeof(pApi->strAuthScheme));
        return(0);
    }
    if (iControl == 'cont')
    {
        ds_strnzcpy(pApi->strContentType, (const char *)pValue, sizeof(pApi->strContentType));
        pApi->bContentBin = (uint8_t)iValue;
        return(0);
    }
    if (iControl == 'mbdy')
    {
        DirtyCastLogPrintfVerbose(pApi->uDebugLevel, 0, "restapi: max body size set to %d\n", iValue);
        pApi->iMaxBodySize = iValue;
        return(0);
    }
    if (iControl == 'scrt')
    {
        const char *pPath = (const char *)pValue;
        // make sure we have a valid path
        if ((pPath == NULL) || (pPath[0] == '\0'))
        {
            return(-1);
        }

        if (pApi->pCert != NULL)
        {
            DirtyMemFree(pApi->pCert, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        }

        if ((pApi->pCert = _RestApiLoadCert(pApi, pPath, &pApi->iCertLen, "-----BEGIN CERTIFICATE-----", "-----END CERTIFICATE-----")) == NULL)
        {
            DirtyCastLogPrintf("restapi: failed to load certificate\n");
            return(-1);
        }

        return(ProtoHttpServControl(pApi->pProtoHttpServ, 'scrt', pApi->iCertLen, 0, pApi->pCert));
    }
    if (iControl == 'skey')
    {
        const char *pPath = (const char *)pValue;
        // make sure we have a valid path
        if ((pPath == NULL) || (pPath[0] == '\0'))
        {
            return(-1);
        }

        if (pApi->pKey != NULL)
        {
            DirtyMemFree(pApi->pKey, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        }

        if ((pApi->pKey = _RestApiLoadCert(pApi, (const char *)pValue, &pApi->iKeyLen, "-----BEGIN RSA PRIVATE KEY-----", "-----END RSA PRIVATE KEY-----")) == NULL)
        {
            DirtyCastLogPrintf("restapi: failed to load key\n");
            return(-1);
        }

        return(ProtoHttpServControl(pApi->pProtoHttpServ, 'skey', pApi->iKeyLen, 0, pApi->pKey));
    }

    // these controls fall through to protohttpserv
    if (iControl == 'spam')
    {
        pApi->uDebugLevel = (uint8_t)iValue;

        // we always want to atleast log at level 1 for protohttpserv to get the information about requests/responses
        return(ProtoHttpServControl(pApi->pProtoHttpServ, 'spam', DS_MAX(iValue, 1), iValue2, pValue));
    }
    return(ProtoHttpServControl(pApi->pProtoHttpServ, iControl, iValue, iValue2, pValue));
}

/*F********************************************************************************/
/*!
    \Function RestApiGetRequestData

    \Description
        Converts the void * in the request data to its actual type to attempt
        to prevent type safety issues

    \Input *pRequest            - the request which contains the data we convert

    \Output
        RestApiRequestDataT *   - the request data the caller works with

    \Version 04/04/2018 (eesponda)
*/
/********************************************************************************F*/
RestApiRequestDataT *RestApiGetRequestData(const ProtoHttpServRequestT *pRequest)
{
    return((RestApiRequestDataT *)pRequest->pData);
}

/*F********************************************************************************/
/*!
    \Function RestApiGetResponseData

    \Description
        Converts the void * in the response data to its actual type to attempt
        to prevent type safety issues

    \Input *pResponse            - the response which contains the data we convert

    \Output
        RestApiResponseDataT *   - the response data the caller works with

    \Version 04/04/2018 (eesponda)
*/
/********************************************************************************F*/
RestApiResponseDataT *RestApiGetResponseData(const ProtoHttpServResponseT *pResponse)
{
    return((RestApiResponseDataT *)pResponse->pData);
}

/*F********************************************************************************/
/*!
    \Function RestApiDestroy

    \Description
        Cleans up the module data

    \Input *pApi    - instance of module
*/
/********************************************************************************F*/
void RestApiDestroy(RestApiRefT *pApi)
{
    ProtoHttpServDestroy(pApi->pProtoHttpServ);
    pApi->pProtoHttpServ = NULL;

    if (pApi->pCert != NULL)
    {
        DirtyMemFree(pApi->pCert, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        pApi->pCert = NULL;
    }
    if (pApi->pKey != NULL)
    {
        DirtyMemFree(pApi->pKey, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
        pApi->pKey = NULL;
    }

    // free resources
    DirtyMemFree(pApi->pResources, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);

    // cleanup the module state
    DirtyMemFree(pApi, RESTAPI_MEMID, pApi->iMemGroup, pApi->pMemGroupUserdata);
}
