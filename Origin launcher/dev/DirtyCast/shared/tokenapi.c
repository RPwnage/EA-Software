/*H********************************************************************************/
/*!
    \File tokenapi.c

    \Description
        This module wraps the Nucleus OAuth calls used for S2S communcation

    \Copyright
        Copyright (c) Electronic Arts 2015. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>

#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/protossl.h"
#include "DirtySDK/util/jsonparse.h"
#include "libsample/zfile.h"
#include "hasher.h"
#include "dirtycast.h"
#include "tokenapi.h"

/*** Defines **********************************************************************/

#define TOKENAPI_MEMID                  ('tokn')
#define TOKENAPI_JSON_MAX_SIZE          (512)
#define TOKENAPI_DEFAULT_MAX_SIZE       (64)

#define TOKENAPI_DEFAULT_CERT               "certs/default.crt"
#define TOKENAPI_DEFAULT_KEY                "certs/default.key"
#define TOKENAPI_CONNECT_S2SHOST_PRODUCTION "https://accounts2s.ea.com"

//! general request options
#define TOKENAPI_KEEPALIVE_TIME                     (60*1000*5)

//! specific to acquisition data
#define TOKENAPI_ACQUISITION_RETRYTIME              (5*1000)

//! token expiration leeway window
#define TOKENAPI_EXPIRATION_LEEWAY                  (30*1000)

//! specific to info query data
#define TOKENAPI_INFOQUERY_MAX_CACHE    (100)
#define TOKENAPI_INFOQUERY_MAX_REQUESTS (8)

//! per the spec the alert for certificate expired is 45; see: https://www.iana.org/assignments/tls-parameters/tls-parameters.xhtml#tls-parameters-6
#define TOKENAPI_SSL_CERT_EXPIRED (45)

/*** Type Definitions *************************************************************/

//! doubly linked list node for holding our previous cached access tokens
typedef struct TokenInfoQueryCacheNodeT
{
    char strAccessToken[TOKENAPI_DATA_MAX_SIZE];
    const TokenApiInfoDataT *pData;
    struct TokenInfoQueryCacheNodeT *pPrev;
    struct TokenInfoQueryCacheNodeT *pNext;
} TokenInfoQueryCacheNodeT;

/* our cache presentation for calls to /connect/tokeninfo
 *
 * we use a LRU (Least Recently Used) cache which requires
 * a hashtable and doubly linked list
 *
 * the doubly linked list keeps track of our data and is
 * sorted by most to least used.
 * new items are added to the front
 * old items are removed from the back
 * accessed items are moved from where ever they are token
 * the front
 *
 * the hashtable gives us a fast lookup from key (access token)
 * to node in the list. we use this as a way to get the position
 * quickly
 */
typedef struct TokenInfoQueryCacheT
{
    //! allocation information
    int32_t iMemGroup;
    void *pMemUserData;

    //! front/back to our doubly linked list
    TokenInfoQueryCacheNodeT *pFront;
    TokenInfoQueryCacheNodeT *pBack;

    HasherRef *pNodes; //!< access_token -> TokenApiCacheNodeT
} TokenInfoQueryCacheT;

//! singly linked list node for holding callbacks (information requests)
typedef struct TokenInfoQueryCbNodeT
{
    TokenApiInfoQueryCbT *pCallback;        //!< pointer to the user callback
    void *pUserData;                        //!< user data to send to the callback
    struct TokenInfoQueryCbNodeT *pNext;    //!< next entry in list
} TokenInfoQueryCbNodeT;

typedef struct TokenInfoQueryRequestT
{
    ProtoHttpRefT *pHttp;                   //!< protohttp module ref used to make requests
    TokenInfoQueryCbNodeT *pCbList;         //!< list of callbacks we respond to when request is complete
    char strAccessToken[TOKENAPI_DATA_MAX_SIZE]; //!< key for each request so we can group callbacks together
    uint8_t bActive;                        //!< if the request is active
    uint8_t _pad[3];
} TokenInfoQueryRequestT;

typedef struct TokenInfoQueryDataT
{
    TokenInfoQueryRequestT aRequests[TOKENAPI_INFOQUERY_MAX_REQUESTS];  //!< array of request data that is used to track our requests
    TokenInfoQueryCacheT Cache;                                         //!< cached token information
    int32_t iNumRequests;                                               //!< count of active requests, easy way to see if any are currently
                                                                        //!< active without querying each individually
} TokenInfoQueryDataT;

typedef struct TokenAcquisitionRequestT
{
    ProtoHttpRefT *pHttp;               //!< used for making the http requests
    char strData[256];                  //!< data to send
    int32_t iBytesSent;                 //!< number of bytes sent in the request
    int32_t iRequestSize;               //!< size of the request
    TokenApiAcquisitionCbT *pCallback;  //!< pointer to the user callback
    void *pUserData;                    //!< user data to send to the callback
} TokenAcquisitionRequestT;

typedef struct TokenAcquisitionDataT
{
    TokenAcquisitionRequestT Request;   //!< information specific to the request
    uint32_t uLastRequestTime;          //!< last time we requested an access token
    uint8_t bRequestActive;             //!< is the request active?
    uint8_t _pad[3];
} TokenAcquisitionDataT;

struct TokenApiRefT
{
    TokenAcquisitionDataT AcquisitionData;              //!< data pertaining to acquiring access tokens
    TokenInfoQueryDataT InfoQueryData;                  //!< data pertaining to querying token information

    #if DIRTYSDK_VERSION >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 6)
    ProtoSSLCertInfoT CertInfo;                         //!< certificate info
    #endif

    char *pCertData;                                    //!< data loaded from cert file
    char *pKeyData;                                     //!< data loaded from key file
    int32_t iCertSize;                                  //!< data size for loaded file
    int32_t iKeySize;                                   //!< data size for loaded file

    char strCertPath[4096];                             //!< path to cert for reloading
    char strKeyPath[4096];                              //!< path to key for reloading
    int32_t iCertWatchFd;                               //!< file descriptor for the cert file watch
    int32_t iKeyWatchFd;                                //!< file descriptor for the key file watch

    int32_t iMemGroup;                                  //!< used for allocation
    void *pMemUserData;                                 //!< user for allocation

    uint8_t uDebugLevel;                                //!< debug level used for ProtoHttp
    uint8_t bJwtEnabled;                                //!< enable requesting jwt tokens
    uint8_t _pad[2];

    char strNucleusAddr[256];                           //!< Nucleus server address, overrides defaults
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _TokenApiCacheFind

    \Description
        Look for the cached information given the access token

    \Input *pCache  - cache state
    \Input *pKey    - the key to lookup the data with

    \Notes
        If the information is found, it is moved to the front of the linked list
        so that less used data is removed first

    \Output
        TokenApiInfoDataT *       - cached data or NULL
*/
/********************************************************************************F*/
static const TokenApiInfoDataT *_TokenApiCacheFind(TokenInfoQueryCacheT *pCache, const char *pKey)
{
    TokenInfoQueryCacheNodeT *pFind;
    if ((pFind = (TokenInfoQueryCacheNodeT *)HashStrFind(pCache->pNodes, pKey)) == NULL)
    {
        return(NULL);
    }

    // if pFind->pPrev is NULL we are at the front so nothing to do
    if (pFind->pPrev != NULL)
    {
        // otherwise set your prev->next to your next
        pFind->pPrev->pNext = pFind->pNext;

        // if pFind->pNext is NULL we are at the back
        // so set a new back to your prev
        if (pFind->pNext == NULL)
        {
            pCache->pBack = pFind->pPrev;
        }
        else
        {
            // otherwise set your next->prev to your prev
            pFind->pNext->pPrev = pFind->pPrev;
        }

        // connect the new prev and next pointers
        pFind->pPrev = NULL;
        pFind->pNext = pCache->pFront;

        // push it to the front
        pCache->pFront->pPrev = pFind;
        pCache->pFront = pFind;
    }

    return(pFind->pData);
}

/*F********************************************************************************/
/*!
    \Function _TokenApiCacheInsert

    \Description
        Insert a new item info the cache

    \Input *pCache      - cache state
    \Input *pAccessToken- the access token this cache entry is for
    \Input *pData       - data we will be caching

    \Notes
        If our cache is full, it deletes items from the back to make space
*/
/********************************************************************************F*/
static void _TokenApiCacheInsert(TokenInfoQueryCacheT *pCache, const char *pAccessToken, const TokenApiInfoDataT *pData)
{
    TokenInfoQueryCacheNodeT *pEntry;

    // make sure the item doesn't exist
    if (_TokenApiCacheFind(pCache, pAccessToken) != NULL)
    {
        return;
    }

    // let see if we have space, if we don't remove an item from the back
    if (HasherCount(pCache->pNodes) >= TOKENAPI_INFOQUERY_MAX_CACHE)
    {
        TokenInfoQueryCacheNodeT *pNode = pCache->pBack;

        pNode->pPrev->pNext = NULL;
        pCache->pBack = pNode->pPrev;

        // stop tracking the key
        HashStrDel(pCache->pNodes, pNode->strAccessToken);

        // clear the memory
        DirtyMemFree((void*)pNode->pData, TOKENAPI_MEMID, pCache->iMemGroup, pCache->pMemUserData);
        DirtyMemFree(pNode, TOKENAPI_MEMID, pCache->iMemGroup, pCache->pMemUserData);
    }

    // try to allocate an entry for the cache
    if ((pEntry = (TokenInfoQueryCacheNodeT *)DirtyMemAlloc(sizeof(*pEntry), TOKENAPI_MEMID, pCache->iMemGroup, pCache->pMemUserData)) == NULL)
    {
        return;
    }
    ds_memclr(pEntry, sizeof(*pEntry));

    // allocate space for the cached data
    if ((pEntry->pData = (TokenApiInfoDataT*)DirtyMemAlloc(sizeof(*pData), TOKENAPI_MEMID, pCache->iMemGroup, pCache->pMemUserData)) == NULL)
    {
        return;
    }
    ds_memcpy((void*)pEntry->pData, pData, sizeof(*pData));
    ds_strnzcpy(pEntry->strAccessToken, pAccessToken, sizeof(pEntry->strAccessToken));

    // push to the front of the list
    if (pCache->pBack != NULL)
    {
        pEntry->pNext = pCache->pFront;
        pCache->pFront->pPrev = pEntry;
    }
    else
    {
        pCache->pBack = pEntry;
    }
    pCache->pFront = pEntry;

    // insert the key and iterator to the front of the list in the map
    HashStrAdd(pCache->pNodes, pEntry->strAccessToken, pEntry);
}

/*F********************************************************************************/
/*!
    \Function _TokenApiCacheClear

    \Description
        Clears the cache

    \Input *pCache  - cache state
*/
/********************************************************************************F*/
static void _TokenApiCacheClear(TokenInfoQueryCacheT *pCache)
{
    TokenInfoQueryCacheNodeT *pNode;
    while ((pNode = pCache->pFront) != NULL)
    {
        pCache->pFront = pNode->pNext;

        DirtyMemFree((void*)pNode->pData, TOKENAPI_MEMID, pCache->iMemGroup, pCache->pMemUserData);
        DirtyMemFree(pNode, TOKENAPI_MEMID, pCache->iMemGroup, pCache->pMemUserData);
    }
    pCache->pBack = NULL;

    HasherClear(pCache->pNodes);
}

/*F********************************************************************************/
/*!
    \Function _TokenApiSetupCert

    \Description
        Loads the certificate and key file needed to make the S2S request

    \Input *pRef            - module state
    \Input *pCertFilename   - filename (path) for the certificate
    \Input *pKeyFilename    - filename (path) for the key file

    \Output
        int32_t             - TRUE=success, FALSE=failure
*/
/********************************************************************************F*/
static int32_t _TokenApiSetupCert(TokenApiRefT *pRef, const char *pCertFilename, const char *pKeyFilename)
{
    // attempt to load the cert, saving the path on success
    if ((pRef->pCertData = ZFileLoad(pCertFilename, &pRef->iCertSize, FALSE)) == NULL)
    {
        DirtyCastLogPrintf("tokenapi: error -- could not open file for cert: %s\n", pCertFilename);
        return(FALSE);
    }
    ds_strnzcpy(pRef->strCertPath, pCertFilename, sizeof(pRef->strCertPath));

    // attempt to load the key, saving the path on success
    if ((pRef->pKeyData = ZFileLoad(pKeyFilename, &pRef->iKeySize, FALSE)) == NULL)
    {
        DirtyCastLogPrintf("tokenapi: error -- could not open file for key: %s\n", pKeyFilename);
        return(FALSE);
    }
    ds_strnzcpy(pRef->strKeyPath, pKeyFilename, sizeof(pRef->strKeyPath));

    // add watches, the error checking will happen at a different level
    pRef->iCertWatchFd = DirtyCastWatchFileInit(pCertFilename);
    pRef->iKeyWatchFd = DirtyCastWatchFileInit(pCertFilename);
    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _TokenApiAcquisitionJsonParse

    \Description
        Parse the json response and fill our response with the data

    \Input *pBuffer     - raw response data
    \Input iSize        - size of the data
    \Input *pResponse   - [out] data we are writing to

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiAcquisitionJsonParse(char *pBuffer, int32_t iSize, TokenApiAccessDataT *pResponse)
{
    uint16_t aJsonParse[TOKENAPI_JSON_MAX_SIZE];
    const char *pCurrent = "";

    // zero out the parsing table
    ds_memclr(aJsonParse, sizeof(aJsonParse));

    if (JsonParse(aJsonParse, TOKENAPI_JSON_MAX_SIZE, pBuffer, iSize) == 0)
    {
        DirtyCastLogPrintf("tokenapi: error -- json parse table size too small\n");
        return;
    }

    if ((pCurrent = JsonFind(aJsonParse, "access_token")) != NULL)
    {
        JsonGetString(pCurrent, pResponse->strAccessToken, sizeof(pResponse->strAccessToken), "");
    }

    if ((pCurrent = JsonFind(aJsonParse, "expires_in")) != NULL)
    {
        /* take the provided expiration time and subtract some time from it to give us an opportunity to request a new
           token. the value was chosen so that is above what our documented high end time it would take to make a token
           request, while at the same time given us some resistance to failed requests on renewal. */
        int32_t iExpiresIn = (JsonGetInteger(pCurrent, 60) * 1000) - TOKENAPI_EXPIRATION_LEEWAY;

        pResponse->iExpiresIn = NetTick() + iExpiresIn;
    }
}

/*F********************************************************************************/
/*!
    \Function _TokenApiInfoQueryJsonParse

    \Description
        Parse the json response and fill our response with the data

    \Input *pRef        - module state
    \Input *pBuffer     - raw response data
    \Input iSize        - size of the data
    \Input *pAccessToken- the access token this response we are parsing is for
    \Input *pResponse   - [out] data we are writing to

    \Note   Adds the response data to the cache

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiInfoQueryJsonParse(TokenApiRefT *pRef, char *pBuffer, int32_t iSize, const char *pAccessToken, TokenApiInfoDataT *pResponse)
{
    uint16_t aJsonParse[TOKENAPI_JSON_MAX_SIZE];
    const char *pCurrent = "";

    // zero out the parsing table
    ds_memclr(aJsonParse, sizeof(aJsonParse));

    if (JsonParse(aJsonParse, TOKENAPI_JSON_MAX_SIZE, pBuffer, iSize) == 0)
    {
        DirtyCastLogPrintf("tokenapi: error -- json parse table size too small\n");
        return;
    }

    if ((pCurrent = JsonFind(aJsonParse, "expires_in")) != NULL)
    {
        pResponse->iExpiresIn = NetTick() + (JsonGetInteger(pCurrent, 60)  *1000);
    }

    if ((pCurrent = JsonFind(aJsonParse, "client_id")) != NULL)
    {
        JsonGetString(pCurrent, pResponse->strClientId, sizeof(pResponse->strClientId), "");
    }

    if ((pCurrent = JsonFind(aJsonParse, "scope")) != NULL)
    {
        JsonGetString(pCurrent, pResponse->strScopes, sizeof(pResponse->strScopes), "");
    }

    // add it to our cache
    _TokenApiCacheInsert(&pRef->InfoQueryData.Cache, pAccessToken, pResponse);
}

/*F********************************************************************************/
/*!
    \Function _TokenApiInfoQueryFireCallback

    \Description
        Fire all our info query callbacks we have store in the list and clean
        up the memory

    \Input *pRef        - module state
    \Input *pRequest    - request information
    \Input *pResponse   - data we respond the callback with

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiInfoQueryFireCallback(TokenApiRefT *pRef, TokenInfoQueryRequestT *pRequest, TokenApiInfoDataT *pResponse)
{
    TokenInfoQueryCbNodeT *pFront;

    // if the user registered callbacks handle them here
    while ((pFront = pRequest->pCbList) != NULL)
    {
        pFront->pCallback(pRequest->strAccessToken, pResponse, pFront->pUserData);

        pRequest->pCbList = pFront->pNext;
        DirtyMemFree(pFront, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData);
    }

    // the callback(s) has(have) been fired we can now set the request as no longer active
    pRequest->bActive = FALSE;
    pRef->InfoQueryData.iNumRequests -= 1;
}

/*F********************************************************************************/
/*!
    \Function _TokenApiCheckAcquisitionRequest

    \Description
        Update the protohttp ref and process our pending acquisition request

    \Input *pRef    - module state

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiCheckAcquisitionRequest(TokenApiRefT *pRef)
{
    int32_t iResult = 0, iSize = 0, iHttpCode = 0;
    char *pBuffer = NULL;
    TokenAcquisitionRequestT *pRequest = &pRef->AcquisitionData.Request;

    // always update protohttp
    ProtoHttpUpdate(pRequest->pHttp);

    // if request is not active we are done
    if (pRef->AcquisitionData.bRequestActive == FALSE)
    {
        return;
    }

    // send data if needed
    if (pRequest->iBytesSent < pRequest->iRequestSize)
    {
        if ((iResult = ProtoHttpSend(pRequest->pHttp, pRequest->strData+pRequest->iBytesSent, pRequest->iRequestSize-pRequest->iBytesSent)) >= 0)
        {
            pRequest->iBytesSent += iResult;
        }
        else
        {
            DirtyCastLogPrintf("tokenapi: error -- request failed (send)\n");
            pRequest->pCallback(NULL, pRequest->pUserData);
            pRef->AcquisitionData.bRequestActive = FALSE;
            return;
        }
    }

    // if the request is not complete we are done
    if ((iResult = ProtoHttpStatus(pRequest->pHttp, 'done', NULL, 0)) == 0)
    {
        return;
    }
    pRef->AcquisitionData.bRequestActive = FALSE;

    // Process the response if valid
    // If an error occurs, try to fire a callback to the user
    if (iResult < 0)
    {
        ProtoSSLAlertDescT Alert;

        // if we sent or received an alert at the ssl layer which ended up in tearing down the connection log it out
        if ((iResult = ProtoHttpStatus(pRequest->pHttp, 'alrt', &Alert, sizeof(Alert))) > 0)
        {
            char strError[256];

            // per the spec alert 45 is "certificate_expired"
            if (Alert.iAlertType == TOKENAPI_SSL_CERT_EXPIRED)
            {
                ds_snzprintf(strError, sizeof(strError), "check for new certificate on our ess vault namespace, refer to readme.txt in runs/certs (%s)", Alert.pAlertDesc);
            }
            else
            {
                ds_snzprintf(strError, sizeof(strError), "alert %s (%s)", iResult == 1 ? "recv" : "sent", Alert.pAlertDesc);
            }
            DirtyCastLogPrintf("tokenapi: error -- %s\n", strError);
        }
        // otherwise this was a connection level issue and just log it as such
        else
        {
            DirtyCastLogPrintf("tokenapi: error -- request failed (hResult=0x%08x)\n", ProtoHttpStatus(pRequest->pHttp, 'hres', NULL, 0));
        }

        pRequest->pCallback(NULL, pRequest->pUserData);
        return;
    }

    // Get the response for processing and debugging purposes (on error)
    iSize = ProtoHttpStatus(pRequest->pHttp, 'data', NULL, 0) + 1;
    if ((pBuffer = (char*)DirtyMemAlloc(iSize, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData)) != NULL)
    {
        ProtoHttpRecvAll(pRequest->pHttp, pBuffer, iSize);
        if (pRef->uDebugLevel > 2)
        {
            NetPrintf(("tokenapi: response\n"));
            NetPrintWrap(pBuffer, 160);
        }

        iHttpCode = ProtoHttpStatus(pRequest->pHttp, 'code', NULL, 0);
        if (iHttpCode == PROTOHTTP_RESPONSE_OK)
        {
            TokenApiAccessDataT Response;
            ds_memclr(&Response, sizeof(Response));

            DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: [%p][http: %p] request completed successfully\n", pRequest, pRequest->pHttp);
            _TokenApiAcquisitionJsonParse(pBuffer, iSize, &Response);
            pRequest->pCallback(&Response, pRequest->pUserData);
        }
        else
        {
            DirtyCastLogPrintf("tokenapi: error -- request failed with httpcode [%d]\n", iHttpCode);
            pRequest->pCallback(NULL, pRequest->pUserData);
        }

        DirtyMemFree(pBuffer, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData);
    }
    else
    {
        DirtyCastLogPrintf("tokenapi: error -- unable to allocate buffer to receive data\n");
        pRequest->pCallback(NULL, pRequest->pUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _TokenApiCheckInfoQueryRequests

    \Description
        Update the protohttp ref and process our pending info query requests

    \Input *pRef    - module state

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiCheckInfoQueryRequests(TokenApiRefT *pRef)
{
    uint32_t uIndex;
    for (uIndex = 0; uIndex < TOKENAPI_INFOQUERY_MAX_REQUESTS; uIndex += 1)
    {
        int32_t iResult = 0, iSize = 0, iHttpCode = 0;
        char *pBuffer = NULL;
        TokenInfoQueryRequestT *pRequest = &pRef->InfoQueryData.aRequests[uIndex];

        // always update protohttp
        ProtoHttpUpdate(pRequest->pHttp);

        // if the request is not active continue
        if (pRequest->bActive == FALSE)
        {
            continue;
        }

        // if the request is not complete continue
        if ((iResult = ProtoHttpStatus(pRequest->pHttp, 'done', NULL, 0)) == 0)
        {
            continue;
        }

        // catch the response code
        iHttpCode = ProtoHttpStatus(pRequest->pHttp, 'code', NULL, 0);

        // Process the response if valid
        // If an error occurs, try to fire a callback to the user
        if ((iResult == -1) || (PROTOHTTP_GetResponseClass(iHttpCode) == PROTOHTTP_RESPONSE_SERVERERROR))
        {
            DirtyCastLogPrintf("tokenapi: error -- request failed\n");
            _TokenApiInfoQueryFireCallback(pRef, pRequest, NULL);
            continue;
        }

        // Get the response for processing and debugging purposes (on error)
        iSize = ProtoHttpStatus(pRequest->pHttp, 'data', NULL, 0) + 1;
        if ((pBuffer = (char*)DirtyMemAlloc(iSize, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData)) != NULL)
        {
            TokenApiInfoDataT Response;
            ds_memclr(&Response, sizeof(Response));

            ProtoHttpRecvAll(pRequest->pHttp, pBuffer, iSize);
            NetPrintfVerbose((pRef->uDebugLevel, 2, "tokenapi: response %s\n", pBuffer));

            if (PROTOHTTP_GetResponseClass(iHttpCode) == PROTOHTTP_RESPONSE_SUCCESSFUL)
            {
                int32_t iCurrentTick = NetTick();
                _TokenApiInfoQueryJsonParse(pRef, pBuffer, iSize, pRequest->strAccessToken, &Response);
                DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: [%p][http: %p] request completed (info query) successfully | tick: 0x%08x | expires in: %d ms | token: %s\n",
                    pRequest, pRequest->pHttp, iCurrentTick, NetTickDiff(Response.iExpiresIn, iCurrentTick), pRequest->strAccessToken);
            }
            // if any other type of error log it
            else
            {
                DirtyCastLogPrintf("tokenapi: error -- request failed with httpcode [%d]\n", iHttpCode);
            }

            _TokenApiInfoQueryFireCallback(pRef, pRequest, &Response);
            DirtyMemFree(pBuffer, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData);
        }
        else
        {
            DirtyCastLogPrintf("tokenapi: error -- unable to allocate buffer to receive data\n");
            _TokenApiInfoQueryFireCallback(pRef, pRequest, NULL);
        }
    }
}

#if DIRTYSDK_VERSION >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 6)
/*F********************************************************************************/
/*!
    \Function _TokenApiUpdateCertificateInfo

    \Description
        Updates the cached certificate information after the cert is loaded

    \Input *pRef    - module state
    \Input *pHttp   - http module state we use to query the information

    \Version 11/02/2020 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiUpdateCertificateInfo(TokenApiRefT *pRef, ProtoHttpRefT *pHttp)
{
    struct tm TmTime;
    char strTime[32], strTime2[32];

    // query the certificate information from protossl via protohttp
    ProtoHttpStatus(pHttp, 'scrt', &pRef->CertInfo, sizeof(pRef->CertInfo));

    // log the information
    DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: cert subject (C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s)\n",
        pRef->CertInfo.Subject.strCountry, pRef->CertInfo.Subject.strState, pRef->CertInfo.Subject.strCity,
        pRef->CertInfo.Subject.strOrg, pRef->CertInfo.Subject.strUnit, pRef->CertInfo.Subject.strCommon);
    DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: cert issuer (C=%s, ST=%s, L=%s, O=%s, OU=%s, CN=%s)\n",
        pRef->CertInfo.Issuer.strCountry, pRef->CertInfo.Issuer.strState, pRef->CertInfo.Issuer.strCity,
        pRef->CertInfo.Issuer.strOrg, pRef->CertInfo.Issuer.strUnit, pRef->CertInfo.Issuer.strCommon);
    DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: cert validity (from=%s, until=%s)\n",
        ds_timetostr(ds_secstotime(&TmTime, pRef->CertInfo.uGoodFrom), TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTime, sizeof(strTime)),
        ds_timetostr(ds_secstotime(&TmTime, pRef->CertInfo.uGoodTill), TIMETOSTRING_CONVERSION_RFC_0822, FALSE, strTime2, sizeof(strTime2)));
}
#endif

/*F********************************************************************************/
/*!
    \Function _TokenApiInitAcquisitionData

    \Description
        Init the state directly involved in making token acquisition requests

    \Input *pRef    - module state

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiInitAcquisitionData(TokenApiRefT *pRef)
{
    ProtoHttpRefT *pHttp;

    // create a ref for acquisition
    if ((pHttp = ProtoHttpCreate(4096)) == NULL)
    {
        DirtyCastLogPrintf("tokenapi: error -- could not create http ref\n");
    }
    ProtoHttpControl(pHttp, 'keep', TRUE, 0, NULL);
    ProtoHttpControl(pHttp, 'skep', TRUE, TOKENAPI_KEEPALIVE_TIME, NULL);
    ProtoHttpControl(pHttp, 'rput', TRUE, 0, NULL);
    ProtoHttpControl(pHttp, 'scrt', pRef->iCertSize, 0, pRef->pCertData);
    ProtoHttpControl(pHttp, 'skey', pRef->iKeySize, 0, pRef->pKeyData);

    #if DIRTYSDK_VERSION >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 6)
    // update the certificate information
    _TokenApiUpdateCertificateInfo(pRef, pHttp);
    #endif

    pRef->AcquisitionData.Request.pHttp = pHttp;
}

/*F********************************************************************************/
/*!
    \Function _TokenApiDestroyAcquisitionData

    \Description
        Cleanup the state directly involved in making token acquisition requests

    \Input *pRef    - module state

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiDestroyAcquisitionData(TokenApiRefT *pRef)
{
    if (pRef->AcquisitionData.Request.pHttp != NULL)
    {
        ProtoHttpDestroy(pRef->AcquisitionData.Request.pHttp);
        pRef->AcquisitionData.Request.pHttp = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function _TokenApiInitInfoQueryData

    \Description
        Init the state directly involved in making info query requests

    \Input *pRef    - module state

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiInitInfoQueryData(TokenApiRefT *pRef)
{
    ProtoHttpRefT *pHttp;
    int32_t iRequest;

    // create a ref per request (information requests)
    for (iRequest = 0; iRequest < TOKENAPI_INFOQUERY_MAX_REQUESTS; iRequest += 1)
    {
        if ((pHttp = ProtoHttpCreate(16*1024)) == NULL)
        {
            DirtyCastLogPrintf("tokenapi: error -- could not create http ref\n");
        }
        ProtoHttpControl(pHttp, 'keep', TRUE, 0, NULL);
        ProtoHttpControl(pHttp, 'skep', TRUE, TOKENAPI_KEEPALIVE_TIME, NULL);
        ProtoHttpControl(pHttp, 'scrt', pRef->iCertSize, 0, pRef->pCertData);
        ProtoHttpControl(pHttp, 'skey', pRef->iKeySize, 0, pRef->pKeyData);
        ProtoHttpControl(pHttp, 'time', (5*1000), 0, NULL);

        pRef->InfoQueryData.aRequests[iRequest].pHttp = pHttp;
    }

    // initialize our cache
    pRef->InfoQueryData.Cache.pNodes = HasherCreate(TOKENAPI_INFOQUERY_MAX_CACHE, TOKENAPI_INFOQUERY_MAX_CACHE);
    pRef->InfoQueryData.Cache.iMemGroup = pRef->iMemGroup;
    pRef->InfoQueryData.Cache.pMemUserData = pRef->pMemUserData;
}

/*F********************************************************************************/
/*!
    \Function _TokenApiDestroyInfoQueryData

    \Description
        Cleanup the state directly involved in making info query requests

    \Input *pRef    - module state

    \Version 03/09/2015 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiDestroyInfoQueryData(TokenApiRefT *pRef)
{
    int32_t iRequest;

    // clean up information requests
    for (iRequest = 0; iRequest < TOKENAPI_INFOQUERY_MAX_REQUESTS; iRequest += 1)
    {
        TokenInfoQueryRequestT *pRequest = &pRef->InfoQueryData.aRequests[iRequest];
        if (pRequest->pHttp != NULL)
        {
            ProtoHttpDestroy(pRequest->pHttp);
            pRequest->pHttp = NULL;
        }

        // clean up the callbacks
        _TokenApiInfoQueryFireCallback(pRef, pRequest, NULL);
    }

    _TokenApiCacheClear(&pRef->InfoQueryData.Cache);
    HasherDestroy(pRef->InfoQueryData.Cache.pNodes);
}

/*F********************************************************************************/
/*!
    \Function _TokenApiProcessCertReload

    \Description
        Process the file watcher for the cert file. If we detect a file change
        it will load the new file and update the needed refs.

    \Input *pRef    - module state

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiProcessCertReload(TokenApiRefT *pRef)
{
    char *pNewCertData;
    int32_t iNewCertSize, iRequest;

    // check if the file has changed, if not nothing else to do
    const uint8_t bCertChanged = (pRef->iCertWatchFd > 0) ? DirtyCastWatchFileProcess(pRef->iCertWatchFd) : FALSE;
    if (bCertChanged == FALSE)
    {
        return;
    }

    // reload the file and set the data
    if ((pNewCertData = ZFileLoad(pRef->strCertPath, &iNewCertSize, FALSE)) != NULL)
    {
        DirtyMemFree(pRef->pCertData, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData);
        pRef->pCertData = pNewCertData;
        pRef->iCertSize = iNewCertSize;
    }
    else
    {
        DirtyCastLogPrintf("tokenapi: [%p] failed to reload the cert file after getting a file change notification\n", pRef);
    }

    // update the data in all the http (ssl) refs
    ProtoHttpControl(pRef->AcquisitionData.Request.pHttp, 'scrt', pRef->iCertSize, 0, pRef->pCertData);
    for (iRequest = 0; iRequest < TOKENAPI_INFOQUERY_MAX_REQUESTS; iRequest += 1)
    {
        ProtoHttpControl(pRef->InfoQueryData.aRequests[iRequest].pHttp, 'scrt', pRef->iCertSize, 0, pRef->pCertData);
    }
    DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: [%p] successfully loaded new cert file\n", pRef);

    #if DIRTYSDK_VERSION >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 6)
    // update the certificate information after the cert was reloaded
    _TokenApiUpdateCertificateInfo(pRef, pRef->AcquisitionData.Request.pHttp);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _TokenApiProcessKeyReload

    \Description
        Process the file watcher for the key file. If we detect a file change
        it will load the new file and update the needed refs.

    \Input *pRef    - module state

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
static void _TokenApiProcessKeyReload(TokenApiRefT *pRef)
{
    char *pNewKeyData;
    int32_t iNewKeySize, iRequest;

    // check if the file has changed, if not nothing else to do
    const uint8_t bKeyChanged = (pRef->iKeyWatchFd > 0) ? DirtyCastWatchFileProcess(pRef->iKeyWatchFd) : FALSE;
    if (bKeyChanged == FALSE)
    {
        return;
    }

    // reload the file and set the data
    if ((pNewKeyData = ZFileLoad(pRef->strKeyPath, &iNewKeySize, FALSE)) != NULL)
    {
        DirtyMemFree(pRef->pKeyData, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData);
        pRef->pKeyData = pNewKeyData;
        pRef->iKeySize = iNewKeySize;
    }
    else
    {
        DirtyCastLogPrintf("tokenapi: [%p] failed to reload the key file after getting a file change notification\n", pRef);
    }

    // update the data in all the http (ssl) refs
    ProtoHttpControl(pRef->AcquisitionData.Request.pHttp, 'skey', pRef->iKeySize, 0, pRef->pKeyData);
    for (iRequest = 0; iRequest < TOKENAPI_INFOQUERY_MAX_REQUESTS; iRequest += 1)
    {
        ProtoHttpControl(pRef->InfoQueryData.aRequests[iRequest].pHttp, 'skey', pRef->iKeySize, 0, pRef->pKeyData);
    }
    DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: [%p] successfully loaded new key file\n", pRef);
}

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function TokenApiCreate

    \Description
        Allocate module state and prepare for use
        Will configure with defaults, use TokenApiControl to override

    \Input *pCertFile  - certificate filename
    \Input *pKeyFile   - key filename

    \Output
        TokenApiRefT *   - pointer to module state, or NULL
*/
/********************************************************************************F*/
TokenApiRefT *TokenApiCreate(const char *pCertFile, const char *pKeyFile)
{
    TokenApiRefT *pRef = NULL;
    int32_t iMemGroup = 0;
    void *pMemUserData = NULL;
    uint8_t bCertLoaded = FALSE;

    DirtyMemGroupQuery(&iMemGroup, &pMemUserData);

    if ((pRef = (TokenApiRefT *)DirtyMemAlloc(sizeof(TokenApiRefT), TOKENAPI_MEMID, iMemGroup, pMemUserData)) == NULL)
    {
        DirtyCastLogPrintf("tokenapi: error -- could not allocate module state\n");
        return(NULL);
    }
    ds_memclr(pRef, sizeof(TokenApiRefT));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemUserData = pMemUserData;
    pRef->uDebugLevel = 1;

    // setup the certs
    if ((pCertFile != NULL) && (pCertFile[0] != '\0') && (pKeyFile != NULL) && (pKeyFile[0] != '\0'))
    {
        // Try to load the certificate & key, if that fails then try the defaults
        // This is to retain some the default behaviour from the old system
        // The system requires the use of a game/platform/year specific cert/key but if that
        // doesn't exist we want to try to fallback
        bCertLoaded = (_TokenApiSetupCert(pRef, pCertFile, pKeyFile) == TRUE) || (_TokenApiSetupCert(pRef, TOKENAPI_DEFAULT_CERT, TOKENAPI_DEFAULT_KEY) == TRUE);
    }

    // log appropriately when there has not been any cert loaded
    if (bCertLoaded == FALSE)
    {
        DirtyCastLogPrintf("tokenapi: couldn't load certificates, this is required for communication with nucleus. please refer to the readme.txt in run/certs for instructions on onboarding.\n");
    }

    // init the acquisiton specific data
    _TokenApiInitAcquisitionData(pRef);
    // init the information query specific data
    _TokenApiInitInfoQueryData(pRef);

    return(pRef);
}

/*F********************************************************************************/
/*!
    \Function TokenApiDestroy

    \Description
        Destroy the module and release its state

    \Input *pRef    - module state

    \Output
        None
*/
/********************************************************************************F*/
void TokenApiDestroy(TokenApiRefT *pRef)
{
    // clean up information requests
    _TokenApiDestroyInfoQueryData(pRef);
    // clean up acquisition request
    _TokenApiDestroyAcquisitionData(pRef);

    // close the watches
    if (pRef->iCertWatchFd > 0)
    {
        DirtyCastWatchFileClose(pRef->iCertWatchFd);
    }
    if (pRef->iKeyWatchFd > 0)
    {
        DirtyCastWatchFileClose(pRef->iKeyWatchFd);
    }

    DirtyMemFree(pRef->pCertData, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData);
    DirtyMemFree(pRef->pKeyData, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData);
    pRef->pCertData = pRef->pKeyData = NULL;

    DirtyMemFree(pRef, TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData);
}

/*F********************************************************************************/
/*!
    \Function TokenApiStatus

    \Description
        Return status of token request. Status type depends on selector

    \Input *pRef        - module state
    \Input iSelect      - info selector (see notes)
    \Input *pBuf        - [out] storage for selector-specific output
    \Input iBufSize     - size of buffer

    \Output
        int32_t - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'cexp'      returns the certificate expiration date via pBuf (uint64_t)
        'done'      zero=complete otherwise in-progress
    \endverbatim
*/
/********************************************************************************F*/
int32_t TokenApiStatus(TokenApiRefT *pRef, int32_t iSelect, void *pBuf, int32_t iBufSize)
{
    #if DIRTYSDK_VERSION >= DIRTYSDK_VERSION_MAKE(15, 1, 6, 0, 6)
    if ((iSelect == 'cexp') && (pBuf != NULL) && (iBufSize == sizeof(pRef->CertInfo.uGoodTill)))
    {
        ds_memcpy(pBuf, &pRef->CertInfo.uGoodTill, iBufSize);
        return(0);
    }
    #endif
    if (iSelect == 'done')
    {
        return((pRef->AcquisitionData.bRequestActive == FALSE) && (pRef->InfoQueryData.iNumRequests == 0));
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function TokenApiControl

    \Description
        TokenApi control function. Different selectors control different behaviors.

    \Input *pRef    - module state
    \Input iControl - control selector (see notes)
    \Input iValue   - selector specific
    \Input iValue2  - selector specific
    \Input *pValue  - selector specific

    \Output
        int32_t - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'jwts'      sets if we should acquire JWT tokens
        'nuca'      specify the Nucleus server address, overrides the defaults
        'spam'      debug level passed to ProtoHttp
    \endverbatim
*/
/********************************************************************************F*/
int32_t TokenApiControl(TokenApiRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'jwts')
    {
        DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: [%p] jwt setting changed to %s\n", pRef, iValue ? "TRUE" : "FALSE"); 
        pRef->bJwtEnabled = iValue;
        return(0);
    }
    if (iControl == 'nuca')
    {
        ds_strnzcpy(pRef->strNucleusAddr, (char*)pValue, (int32_t)sizeof(pRef->strNucleusAddr));
        return(0);
    }

    if (iControl == 'spam')
    {
        int32_t iRequest;
        for (iRequest = 0; iRequest < TOKENAPI_INFOQUERY_MAX_REQUESTS; iRequest += 1)
        {
            ProtoHttpControl(pRef->InfoQueryData.aRequests[iRequest].pHttp, 'spam', iValue, 0, NULL);
        }
        ProtoHttpControl(pRef->AcquisitionData.Request.pHttp, 'spam', iValue, 0, NULL);
        pRef->uDebugLevel = iValue;

        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function TokenApiUpdate

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pRef - module state
*/
/********************************************************************************F*/
void TokenApiUpdate(TokenApiRefT *pRef)
{
    // check the file watches
    _TokenApiProcessCertReload(pRef);
    _TokenApiProcessKeyReload(pRef);

    // Check requests
    _TokenApiCheckAcquisitionRequest(pRef);
    _TokenApiCheckInfoQueryRequests(pRef);
}

/*F********************************************************************************/
/*!
    \Function TokenApiAcquisitionAsync

    \Description
        If a request is not already in progress it will start a new
        request to acquire a token

    \Input *pRef        - module state
    \Input *pCallback   - callback to respond with
    \Input *pUserData   - data that will be provided with the callback

    \Output
        int32_t         - 1=success, 0=too soon, negative=error

    \Notes
        This function is rate limited to prevent us from spamming Nucleus when
        failure occurs.
*/
/********************************************************************************F*/
int32_t TokenApiAcquisitionAsync(TokenApiRefT *pRef, TokenApiAcquisitionCbT *pCallback, void *pUserData)
{
    const uint32_t uCurTick = NetTick();
    TokenAcquisitionRequestT *pRequest = &pRef->AcquisitionData.Request;
    int32_t iRetCode = 1;  // default to success

    if (pCallback == NULL || pUserData == NULL)
    {
        DirtyCastLogPrintf("tokenapi: error -- callback required for acquisition request\n");
        return(-1);
    }
    pRequest->pCallback = pCallback;
    pRequest->pUserData = pUserData;

    if ((uint32_t)NetTickDiff(uCurTick, pRef->AcquisitionData.uLastRequestTime) > TOKENAPI_ACQUISITION_RETRYTIME)
    {
        char strUri[TOKENAPI_DEFAULT_MAX_SIZE];
        int32_t iResult;

        pRef->AcquisitionData.uLastRequestTime = uCurTick;

        DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: [%p][http: %p] request started\n", pRequest, pRequest->pHttp);

        ProtoHttpControl(pRequest->pHttp, 'apnd', 0, 0, (void*)"Content-Type: application/x-www-form-urlencoded\r\nenable-client-cert-auth: true");
        ProtoHttpControl(pRequest->pHttp, 'rmax', 0, 0, NULL);

        // format the uri
        ds_snzprintf(strUri, sizeof(strUri), "%s/connect/token",
            pRef->strNucleusAddr[0] == '\0' ? TOKENAPI_CONNECT_S2SHOST_PRODUCTION : pRef->strNucleusAddr);

        // format the data
        pRequest->strData[0] = '\0';
        ProtoHttpUrlEncodeStrParm(pRequest->strData, sizeof(pRequest->strData), "grant_type=", "client_credentials");
        if (pRef->bJwtEnabled == TRUE)
        {
            ProtoHttpUrlEncodeStrParm(pRequest->strData, sizeof(pRequest->strData), "&token_format=", "JWS");
        }
        pRequest->iRequestSize = (int32_t)strlen(pRequest->strData);

        if ((iResult = ProtoHttpPost(pRequest->pHttp, strUri, pRequest->strData, pRequest->iRequestSize, FALSE)) >= 0)
        {
            pRef->AcquisitionData.bRequestActive = TRUE;
            pRequest->iBytesSent = iResult;
        }
        else
        {
            iRetCode = iResult;  //  if error, return ProtoHttpPost() error code
        }
    }
    else
    {
        // too soon
        iRetCode = 0;
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function TokenApiInfoQuery

    \Description
        Try to pull the information from the cache

    \Input *pRef            - module state
    \Input *pAccessToken    - the token we need information on

    \Output
        const TokenApiInfoDataT * - cached item or NULL
*/
/********************************************************************************F*/
const TokenApiInfoDataT *TokenApiInfoQuery(TokenApiRefT *pRef, const char *pAccessToken)
{
    return(_TokenApiCacheFind(&pRef->InfoQueryData.Cache, pAccessToken));
}

/*F********************************************************************************/
/*!
    \Function TokenApiInfoQueryAsync

    \Description
        Attempts to get the token information based on the passed in access token

    \Input *pRef            - module state
    \Input *pAccessToken    - the token we need information on
    \Input *pCallback       - callback to respond with
    \Input *pUserData       - data that will be provided with the callback

    \Output
        int32_t             - zero=request fired, >zero=cache entry found, negative=error

    \Notes
        If a request is in flight for the access token already we save the callback
        to be fired when we receive a response.

        If a request is in flight for a different access token or no requests are
        in flight we create new request that is processed individually from other requests
*/
/********************************************************************************F*/
int32_t TokenApiInfoQueryAsync(TokenApiRefT *pRef, const char *pAccessToken, TokenApiInfoQueryCbT *pCallback, void *pUserData)
{
    int32_t iResult, iRequest, iFreeSlotIdx = -1;
    char strUri[TOKENAPI_DEFAULT_MAX_SIZE+TOKENAPI_DATA_MAX_SIZE]; // enough space for the host + access token
    TokenInfoQueryRequestT *pRequest;
    TokenInfoQueryCbNodeT *pFront;

    // make sure the callback data is valid
    if (pCallback == NULL || pUserData == NULL)
    {
        DirtyCastLogPrintf("tokenapi: error -- callback required for info query request\n");
        return(-1);
    }

    // double check to make sure it isn't in the cache
    if (_TokenApiCacheFind(&pRef->InfoQueryData.Cache, pAccessToken) != NULL)
    {
        return(1);
    }

    // look for either a pending request that matches or a new free slot
    for (iRequest = 0; iRequest < TOKENAPI_INFOQUERY_MAX_REQUESTS; iRequest += 1)
    {
        pRequest = &pRef->InfoQueryData.aRequests[iRequest];
        if (pRequest->bActive == FALSE)
        {
            iFreeSlotIdx = iRequest;
            continue;
        }

        // check to see if the request matches
        if (strncmp(pRequest->strAccessToken,
                    pAccessToken,
                    sizeof(pRequest->strAccessToken)) == 0)
        {
            break;
        }
    }
    // if we did not find a duplicate request and found a free slot use that instead
    if ((iRequest == TOKENAPI_INFOQUERY_MAX_REQUESTS) && (iFreeSlotIdx != -1))
    {
        pRequest = &pRef->InfoQueryData.aRequests[iFreeSlotIdx];
    }
    // otherwise if we cannot make the request so return error
    else if ((iRequest == TOKENAPI_INFOQUERY_MAX_REQUESTS) && (iFreeSlotIdx == -1))
    {
        DirtyCastLogPrintf("tokenapi: error -- cannot add any more requests\n");
        return(-1);
    }

    /* if the request is not active or it is active and the request is the same then save the callback
       so it can be fired when the request completes */

    pFront = pRequest->pCbList;
    if ((pRequest->pCbList = (TokenInfoQueryCbNodeT *)DirtyMemAlloc(sizeof(*pRequest->pCbList), TOKENAPI_MEMID, pRef->iMemGroup, pRef->pMemUserData)) == NULL)
    {
        DirtyCastLogPrintf("tokenapi: error -- failed to allocate callback node for info query request\n");
        return(-1);
    }
    pRequest->pCbList->pCallback = pCallback;
    pRequest->pCbList->pUserData = pUserData;
    pRequest->pCbList->pNext = pFront;

    // if we found a request that matches then nothing left to do
    if (iRequest < TOKENAPI_INFOQUERY_MAX_REQUESTS)
    {
        return(0);
    }

    // save off the access token
    ds_strnzcpy(pRequest->strAccessToken, pAccessToken, sizeof(pRequest->strAccessToken));

    DirtyCastLogPrintfVerbose(pRef->uDebugLevel, 0, "tokenapi: [%p][http: %p] request started (info query)\n", pRequest, pRequest->pHttp);

    ds_snzprintf(strUri, sizeof(strUri), "%s/connect/tokeninfo",
        pRef->strNucleusAddr[0] == '\0' ? TOKENAPI_CONNECT_S2SHOST_PRODUCTION : pRef->strNucleusAddr);
    ProtoHttpUrlEncodeStrParm(strUri, sizeof(strUri), "?access_token=", pAccessToken);

    if ((iResult = ProtoHttpGet(pRequest->pHttp, strUri, FALSE)) == 0)
    {
        pRequest->bActive = TRUE;
        pRef->InfoQueryData.iNumRequests += 1;
    }
    return(iResult);
}
