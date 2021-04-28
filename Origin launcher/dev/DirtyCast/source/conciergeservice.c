/*H********************************************************************************/
/*!
    \File conciergeservice.c

    \Description
        This module handles communication with the Connection Concierge Service (CCS)
        The CCS maintains the registry of all running instances of DirtyCast servers

    \Copyright
        Copyright (c) Electronic Arts 2015. ALL RIGHTS RESERVED.

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/util/jsonformat.h"
#include "DirtySDK/util/jsonparse.h"
#include "libsample/zfile.h"

#include "tokenapi.h"
#include "dirtycast.h"
#include "conciergeservice.h"

/*** Defines **********************************************************************/

#define CONCIERGESERVICE_MEMID              ('ccsv')
#define CONCIERGESERVICE_HEARTBEAT_TIME     (30*1000)
#define CONCIERGESERVICE_FASTHEARTBEAT_TIME (5*1000)
#define CONCIERGESERVICE_TOKEN_RETRY_TIME   (30*1000)
#define CONCIERGESERVICE_OFFLINE_DETECTION  (2*CONCIERGESERVICE_HEARTBEAT_TIME)
#define CONCIERGESERVICE_HOST_DEFAULT       ("http://10.10.29.91:18094")
#define CONCIERGESERVICE_HTTPDEBUG_OFFSET   (2)

/*** Type Definitions *************************************************************/

//! request information
typedef struct RegistrationRequestT
{
    uint32_t uCapacity;

    /* Once acquired from CCS with a successful initial registration, this value
       will never subsequently change for the lifetime of the module. It cannot go
       back to 0 even if ccs status changes from 'online' to 'offline'. While in
       'offline' state we keep attempting heartbeating the CCS using that same
       identifier. */
    uint32_t uHostingServerId;

    /* The following fields can't changed after they are initialized from a
       call to ConciergeServiceRegister(). Usage of that function is only allowed 
       once in the lifetime of the module */
    char strPingSite[64];
    char strPool[64];
    char strApiBase[256];
    char strVersions[64];

    char strBuffer[4096];
    int32_t iBufSize;
    int32_t iBytesSent;
} RegistrationRequestT;

struct ConciergeServiceRefT
{
    //! allocation information
    int32_t iMemGroup;
    void *pMemGroupUserData;

    ProtoHttpRefT *pHttp;           //!< for making requests to the CCS, heartbeats/registration
    char *pCertData;                //!< loaded certificate data
    char *pKeyData;                 //!< loaded key data

    TokenApiRefT *pTokenApi;        //!< used for getting tokens
    TokenApiAccessDataT TokenData;  //!< cached token dat
    uint32_t uTokenRetryTick;       //!< next time at which token acquisition can be attempted

    enum {
        CCS_STATUS_OFFLINE = 0,     //!< CCS has probably removed us from the set of available hosting servers
        CCS_STATUS_ONLINE           //!< CCS knows about us
    } eCcsStatus;

    RegistrationRequestT Request;   //!< registration information
    uint32_t uRequestTime;          //!< tick for when a new request should be made
    char strCcsAddr[256];           //!< CCS address
    uint32_t uErrorTick;            //!< when we hit the error

    char strCertPath[4096];         //!< path to cert for reloading
    char strKeyPath[4096];          //!< path to key for reloading
    int32_t iCertWatchFd;           //!< file descriptor for the cert file watch
    int32_t iKeyWatchFd;            //!< file descriptor for the key file watch

    uint16_t uDebugLevel;           //!< debug level
    uint8_t bActive;                //!< is there an active http request?
    uint8_t bTokenValid;            //!< is the token valid?
};

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ConciergeServiceProcessCertReload

    \Description
        Process the file watcher for the cert file. If we detect a file change
        it will load the new file and update the needed refs.

    \Input *pConciergeService    - module state

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
static void _ConciergeServiceProcessCertReload(ConciergeServiceRefT *pConciergeService)
{
    char *pNewCertData;
    int32_t iNewCertSize;

    // check if the file has changed, if not nothing else to do
    const uint8_t bCertChanged = (pConciergeService->iCertWatchFd > 0) ? DirtyCastWatchFileProcess(pConciergeService->iCertWatchFd) : FALSE;
    if (bCertChanged == FALSE)
    {
        return;
    }

    // reload the file and set the data
    if ((pNewCertData = ZFileLoad(pConciergeService->strCertPath, &iNewCertSize, FALSE)) != NULL)
    {
        DirtyMemFree(pConciergeService->pCertData, CONCIERGESERVICE_MEMID, pConciergeService->iMemGroup, pConciergeService->pMemGroupUserData);
        pConciergeService->pCertData = pNewCertData;
    }
    else
    {
        DirtyCastLogPrintf("conciergeservice: [%p] failed to reload the cert file after getting a file change notification\n", pConciergeService);
    }

    // update the data in all the http (ssl) refs
    ProtoHttpControl(pConciergeService->pHttp, 'scrt', iNewCertSize, 0, pConciergeService->pCertData);
    DirtyCastLogPrintfVerbose(pConciergeService->uDebugLevel, 0, "conciergeservice: [%p] successfully loaded new cert file\n", pConciergeService);
}

/*F********************************************************************************/
/*!
    \Function _ConciergeServiceProcessKeyReload

    \Description
        Process the file watcher for the key file. If we detect a file change
        it will load the new file and update the needed refs.

    \Input *pConciergeService    - module state

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
static void _ConciergeServiceProcessKeyReload(ConciergeServiceRefT *pConciergeService)
{
    char *pNewKeyData;
    int32_t iNewKeySize;

    // check if the file has changed, if not nothing else to do
    const uint8_t bKeyChanged = (pConciergeService->iKeyWatchFd > 0) ? DirtyCastWatchFileProcess(pConciergeService->iKeyWatchFd) : FALSE;
    if (bKeyChanged == FALSE)
    {
        return;
    }

    // reload the file and set the data
    if ((pNewKeyData = ZFileLoad(pConciergeService->strKeyPath, &iNewKeySize, FALSE)) != NULL)
    {
        DirtyMemFree(pConciergeService->pKeyData, CONCIERGESERVICE_MEMID, pConciergeService->iMemGroup, pConciergeService->pMemGroupUserData);
        pConciergeService->pKeyData = pNewKeyData;
    }
    else
    {
        DirtyCastLogPrintf("conciergeservice: [%p] failed to reload the key file after getting a file change notification\n", pConciergeService);
    }

    // update the data in all the http (ssl) refs
    ProtoHttpControl(pConciergeService->pHttp, 'skey', iNewKeySize, 0, pConciergeService->pKeyData);
    DirtyCastLogPrintfVerbose(pConciergeService->uDebugLevel, 0, "conciergeservice: [%p] successfully loaded new key file\n", pConciergeService);
}

/*F********************************************************************************/
/*!
    \Function _ConciergeServiceProcessError

    \Description
        Does any processing needed when an error occured

    \Input *pConciergeService   - module state
    \Input iResult              - error code
    \Input *pErrorText          - error details (for logging)

    \Version 10/05/2018 (eesponda)
*/
/********************************************************************************F*/
static void _ConciergeServiceProcessError(ConciergeServiceRefT *pConciergeService, int32_t iResult, const char *pErrorText)
{
    uint32_t uCurTick = NetTick();

    // save the time when the error first occured
    if (pConciergeService->uErrorTick == 0)
    {
        pConciergeService->uErrorTick = uCurTick;

        // if NetTick() returned 0 then alter it because 0 means 'not in error state'
        if (pConciergeService->uErrorTick == 0)
        {
            pConciergeService->uErrorTick = 1;
        }
    }

    /* If we are not in error because of a token acquisition issue, then the error is with the 
       dirtycast-to-CCS http request. In that case, if the hosting server id is valid then we now
       we were heartbeating and not performing initial registration. In that case, we need to attempt
       to recover much faster due to the potential of us being removed from the directory by the CCS.
       If we get a result code 5xx type error, then also attempt to recover much faster (non connection error).
       4xx problems are our fault and retrying faster might not solve the issue. */
    if ((strcmp(pErrorText, "token acquisition") != 0) &&
        (pConciergeService->Request.uHostingServerId != 0) && 
        ((iResult < 0) || (PROTOHTTP_GetResponseClass(iResult) == PROTOHTTP_RESPONSE_SERVERERROR)))
    {
        pConciergeService->uRequestTime = uCurTick + CONCIERGESERVICE_FASTHEARTBEAT_TIME;
    }

    DirtyCastLogPrintf("conciergeservice: %s failed with error=%d\n", pErrorText, iResult);

    // mark the http request as complete (do this unconditionally even if error is for token acquisition)
    pConciergeService->bActive = FALSE;
}

/*F********************************************************************************/
/*!
    \Function _ConciergeServiceHeartbeat

    \Description
        Sends a heartbeat to the concierge service

    \Input *pConciergeService   - module state
    \Input uNewCapacity         - our server's capacity

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************F*/
static void _ConciergeServiceHeartbeat(ConciergeServiceRefT *pConciergeService, uint32_t uNewCapacity)
{
    int32_t iBytesSent;
    char *pBuffer;
    char strUri[512];
    static uint32_t uRequestId = 0;
    RegistrationRequestT *pRequest = &pConciergeService->Request;

    // update capacity
    pRequest->uCapacity = uNewCapacity;

    // json formatting
    JsonInit(pRequest->strBuffer, sizeof(pRequest->strBuffer), 0);
    JsonAddInt(pRequest->strBuffer, "request_id", uRequestId);
    JsonAddInt(pRequest->strBuffer, "capacity", pRequest->uCapacity);
    JsonAddStr(pRequest->strBuffer, "prototunnel_versions", pRequest->strVersions);
    JsonAddInt(pRequest->strBuffer, "hosting_server_id", pRequest->uHostingServerId);
    JsonAddInt(pRequest->strBuffer, "interval", CONCIERGESERVICE_HEARTBEAT_TIME);
    pBuffer = JsonFinish(pRequest->strBuffer);
    pRequest->iBufSize = (int32_t)strlen(pBuffer);

    if (pConciergeService->uDebugLevel > 2)
    {
        NetPrintf(("conciergeservice: request payload %d bytes\n", (int32_t)strlen(pBuffer)));
        NetPrintWrap(pBuffer, 132);
    }

    // format uri
    ds_snzprintf(strUri, sizeof(strUri), "%s/cc/v1/hosting_server/%s/%s", pConciergeService->strCcsAddr, pRequest->strPingSite, pRequest->strPool);
    ProtoHttpUrlEncodeStrParm(strUri, sizeof(strUri), "?url=", pRequest->strApiBase);

    // make request
    if ((iBytesSent = ProtoHttpPost(pConciergeService->pHttp, strUri, pBuffer, pRequest->iBufSize, FALSE)) >= 0)
    {
        pConciergeService->bActive = TRUE;
        pConciergeService->uRequestTime = NetTick() + CONCIERGESERVICE_HEARTBEAT_TIME;
        pRequest->iBytesSent = iBytesSent;
    }
    else
    {
        _ConciergeServiceProcessError(pConciergeService, iBytesSent, "http post");
    }

    // update request id
    uRequestId += 1;
}

/*F********************************************************************************/
/*!
    \Function _ConciergeServiceCheckRequest

    \Description
        Updates and checks our ProtoHttp request that is in progress

    \Input *pConciergeService   - module state

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************F*/
static void _ConciergeServiceCheckRequest(ConciergeServiceRefT *pConciergeService)
{
    int32_t iResult;
    char strPostBody[256];
    uint16_t aJsonParse[512];
    const char *pParserCurrent = NULL;
    RegistrationRequestT *pRequest = &pConciergeService->Request; 

    // send data if needed
    if (pRequest->iBytesSent < pRequest->iBufSize)
    {
        if ((iResult = ProtoHttpSend(pConciergeService->pHttp, pRequest->strBuffer+pRequest->iBytesSent, pRequest->iBufSize-pRequest->iBytesSent)) >= 0)
        {
            pRequest->iBytesSent += iResult;
        }
        else
        {
            _ConciergeServiceProcessError(pConciergeService, iResult, "http send");
            return;
        }
    }

    // check status
    if ((iResult = ProtoHttpStatus(pConciergeService->pHttp, 'done', NULL, 0)) == 0)
    {
        // request still in progress;
        return;
    }

    // request finished
    pConciergeService->bActive = FALSE;

    // handle the response
    if (iResult < 0)
    {
        _ConciergeServiceProcessError(pConciergeService, iResult, "http connection");
        return;
    }

    iResult = ProtoHttpStatus(pConciergeService->pHttp, 'code', NULL, 0);
    if (PROTOHTTP_GetResponseClass(iResult) != PROTOHTTP_RESPONSE_SUCCESSFUL)
    {
        _ConciergeServiceProcessError(pConciergeService, iResult, "http response");
        return;
    }

    // extract the hosting_server_id in the response if present,
    // this will be used for the prototunnel local id
    if ((iResult = ProtoHttpStatus(pConciergeService->pHttp, 'body', NULL, 0)) <= 0)
    {
        DirtyCastLogPrintf("conciergeservice: registration warning, expected data from concierge service was not provided.\n");
        return;
    }
    if (iResult + 1 > (int32_t)sizeof(strPostBody))
    {
        DirtyCastLogPrintf("conciergeservice: registration response body too large\n");
        return;
    }

    ds_memclr(strPostBody, sizeof(strPostBody));
    ds_memclr(aJsonParse, sizeof(aJsonParse));
    if ((iResult = ProtoHttpRecvAll(pConciergeService->pHttp, strPostBody, sizeof(strPostBody))) < 0)
    {
        _ConciergeServiceProcessError(pConciergeService, iResult, "http recv");
        return;
    }

    // log the response
    if (pConciergeService->uDebugLevel > 2)
    {
        NetPrintf(("conciergeservice: response payload %d bytes\n", iResult));
        NetPrintWrap(strPostBody, 132);
    }

    if (JsonParse(aJsonParse, sizeof(aJsonParse)/sizeof(aJsonParse[0]), strPostBody, iResult) == 0)
    {
        DirtyCastLogPrintf("conciergeservice: registration warning, parsing table too small to parse json.\n");
        return;
    }

    if ((pParserCurrent = JsonFind(aJsonParse, "hosting_server_id")) != NULL)
    {
        pConciergeService->Request.uHostingServerId = (uint32_t)JsonGetInteger(pParserCurrent, 0);
    }
    else
    {
        DirtyCastLogPrintf("conciergeservice: registration warning, expected 'hosting_server_id' from concierge service but none was provided.\n");
    }

    // if we recovered from an error then log it so we can figure out how long we were
    // in error for
    if (pConciergeService->uErrorTick > 0)
    {
        DirtyCastLogPrintf("conciergeservice: CCS heartbeating recovered from error (time=%ds)\n",
            NetTickDiff(NetTick(), pConciergeService->uErrorTick) / 1000);
        pConciergeService->uErrorTick = 0;
    }

    #if DIRTYCODE_LOGGING
    if (pConciergeService->eCcsStatus == CCS_STATUS_OFFLINE)
    {
        DirtyCastLogPrintf("conciergeservice: CCS status changed to ONLINE\n");
    }
    #endif

    pConciergeService->eCcsStatus = CCS_STATUS_ONLINE;
}

/*F********************************************************************************/
/*!
    \Function _ConciergeServiceTokenCb

    \Description
        Callback that is fired when the access token request is complete

    \Input *pData       - the response information
    \Input *pUserData   - contains our module ref

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************F*/
static void _ConciergeServiceTokenCb(const TokenApiAccessDataT *pData, void *pUserData)
{
    ConciergeServiceRefT *pConciergeService = (ConciergeServiceRefT *)pUserData;

    if (pData != NULL)
    {
        char strHeader[8*1024];
        int32_t iHdrLen = 0;

        // copy the new data
        ds_memcpy(&pConciergeService->TokenData, pData, sizeof(pConciergeService->TokenData));

        // format the header data for the protohttp ref as it only needs to be set when it changes
        iHdrLen += ds_snzprintf(strHeader+iHdrLen, sizeof(strHeader)-iHdrLen, "Authorization: NEXUS_S2S %s\r\n", pConciergeService->TokenData.strAccessToken);
        iHdrLen += ds_snzprintf(strHeader+iHdrLen, sizeof(strHeader)-iHdrLen, "Content-Type: application/json\r\n");
        ProtoHttpControl(pConciergeService->pHttp, 'apnd', 0, 0, NULL);
        ProtoHttpControl(pConciergeService->pHttp, 'apnd', 0, 0, (void *)strHeader);

        pConciergeService->bTokenValid = TRUE;
        DirtyCastLogPrintf("conciergeservice: successful token acquisition\n");
    }
    else
    {
        _ConciergeServiceProcessError(pConciergeService, -1, "token acquisition");
    }
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ConciergeServiceCreate

    \Description
        Allocate module state and prepare for use

    \Input *pTokenApi       - ref to tokenapi module used to get access tokens
    \Input *pCertFile       - file to use for certificate (NULL if not used)
    \Input *pKeyFile        - file to use for certificate key (NULL if not used)

    \Output
        ConciergeServiceRefT*   - pointer to module state, or NULL

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************F*/
ConciergeServiceRefT *ConciergeServiceCreate(TokenApiRefT *pTokenApi, const char *pCertFile, const char *pKeyFile)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    ProtoHttpRefT *pHttp;
    ConciergeServiceRefT *pConciergeService;
    int32_t iCertSize = 0, iKeySize = 0;

    // query memory group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // create the ref for making requests to the CCS
    if ((pHttp = ProtoHttpCreate(4096)) == NULL)
    {
        return(NULL);
    }
    ProtoHttpControl(pHttp, 'keep', TRUE, 0, NULL);
    ProtoHttpControl(pHttp, 'rput', TRUE, 0, NULL);
    ProtoHttpControl(pHttp, 'spam', 0, 0, NULL);

    // create the module state
    if ((pConciergeService = (ConciergeServiceRefT *)DirtyMemAlloc(sizeof(*pConciergeService), CONCIERGESERVICE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        ProtoHttpDestroy(pHttp);
        return(NULL);
    }
    ds_memclr(pConciergeService, sizeof(*pConciergeService));
    pConciergeService->iMemGroup = iMemGroup;
    pConciergeService->pMemGroupUserData = pMemGroupUserData;
    pConciergeService->pHttp = pHttp;
    pConciergeService->pTokenApi = pTokenApi;
    pConciergeService->uRequestTime = NetTick();
    ds_strnzcpy(pConciergeService->strCcsAddr, CONCIERGESERVICE_HOST_DEFAULT, sizeof(pConciergeService->strCcsAddr));

    if ((pCertFile != NULL) && (pCertFile[0] != '\0'))
    {
        // load certificate; save the path and create a watch to handle reloading
        if ((pConciergeService->pCertData = ZFileLoad(pCertFile, &iCertSize, FALSE)) == NULL)
        {
            ConciergeServiceDestroy(pConciergeService);
            return(NULL);
        }
        ds_strnzcpy(pConciergeService->strCertPath, pCertFile, sizeof(pConciergeService->strCertPath));
        pConciergeService->iCertWatchFd = DirtyCastWatchFileInit(pCertFile);
        ProtoHttpControl(pHttp, 'scrt', iCertSize, 0, pConciergeService->pCertData);
    }
    if ((pKeyFile != NULL) && (pKeyFile[0] != '\0'))
    {
        // load key; save the path and create a watch to handle reloading
        if ((pConciergeService->pKeyData = ZFileLoad(pKeyFile, &iKeySize, FALSE)) == NULL)
        {
            ConciergeServiceDestroy(pConciergeService);
            return(NULL);
        }
        ds_strnzcpy(pConciergeService->strKeyPath, pKeyFile, sizeof(pConciergeService->strKeyPath));
        pConciergeService->iKeyWatchFd = DirtyCastWatchFileInit(pKeyFile);
        ProtoHttpControl(pHttp, 'skey', iKeySize, 0, pConciergeService->pKeyData);
    }

    return(pConciergeService);
}

/*F********************************************************************************/
/*!
    \Function ConciergeServiceUpdate

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pConciergeService   - module state
    \Input uCapacity            - current server capacity

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************F*/
void ConciergeServiceUpdate(ConciergeServiceRefT *pConciergeService, uint32_t uCapacity)
{
    RegistrationRequestT *pRequest = &pConciergeService->Request;
    uint32_t uCurTick = NetTick();

    // check the file watches
    _ConciergeServiceProcessCertReload(pConciergeService);
    _ConciergeServiceProcessKeyReload(pConciergeService);

    // update even if we are not active we want to update the socket (cases where server closes socket)
    ProtoHttpUpdate(pConciergeService->pHttp);

    if (pConciergeService->bActive == TRUE)
    {
        _ConciergeServiceCheckRequest(pConciergeService);
        return;
    }

    /* if ConciergeServiceRegister() has not yet been called, then it is too
       early to fire a new token request or try to send a heartbeat to CCS */
    if (pRequest->strPingSite[0] == '\0')
    {
        return;
    }

    // don't do anything else if token acquistion already in progress
    if (TokenApiStatus(pConciergeService->pTokenApi, 'done', NULL, 0) == FALSE)
    {
        return;
    }

    if ((pConciergeService->uErrorTick != 0) &&
        NetTickDiff(uCurTick, pConciergeService->uErrorTick) > CONCIERGESERVICE_OFFLINE_DETECTION)
    {
        #if DIRTYCODE_LOGGING
        if (pConciergeService->eCcsStatus == CCS_STATUS_ONLINE)
        {
            DirtyCastLogPrintf("conciergeservice: CCS status changed to OFFLINE\n");
        }
        #endif

        pConciergeService->eCcsStatus = CCS_STATUS_OFFLINE;
    }

    // if token is invalid or expired, then request one
    if ((pConciergeService->bTokenValid == FALSE) || (NetTickDiff(uCurTick, pConciergeService->TokenData.iExpiresIn) > 0))
    {
        if (pConciergeService->bTokenValid == TRUE)
        {
            DirtyCastLogPrintfVerbose(pConciergeService->uDebugLevel, 1, "conciergeservice: cached token marked as invalid\n");
            pConciergeService->bTokenValid = FALSE;
            ds_memclr(&pConciergeService->TokenData, sizeof(pConciergeService->TokenData));
        }

        // delay initialization of token acquisition if we have not waited long enough since last attempt
        if (NetTickDiff(uCurTick, pConciergeService->uTokenRetryTick) > 0)
        {
            DirtyCastLogPrintfVerbose(pConciergeService->uDebugLevel, 1, "conciergeservice: initiating token acquisition\n");

            pConciergeService->uTokenRetryTick = NetTick() + CONCIERGESERVICE_TOKEN_RETRY_TIME;
            TokenApiAcquisitionAsync(pConciergeService->pTokenApi, _ConciergeServiceTokenCb, pConciergeService);
        }
        return;
    }

    // kick off a new request
    if (NetTickDiff(uCurTick, pConciergeService->uRequestTime) > 0)
    {
        _ConciergeServiceHeartbeat(pConciergeService, uCapacity);
    }
}

/*F********************************************************************************/
/*!
    \Function ConciergeServiceRegister

    \Description
        Sets our registration data to later use for heartbeats

    \Input *pConciergeService   - module state
    \Input *pPingSite           - the data center we are hosted within
    \Input *pPool               - which pool of dirtycast servers are we int32_t
    \Input *pApiBase            - the url where hosted connections can be created
    \Input *pVersions           - versions supported by the server (comma delimited)

    \Output
        int32_t                 - zero=success, -1=already registered

    \Notes
        The module is designed in such a way that this function is expected to be called
        only once. Any subsequent call will keep returning -1 and result in a no-op for
        the lifetime of the module.

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t ConciergeServiceRegister(ConciergeServiceRefT *pConciergeService, const char *pPingSite, const char *pPool, const char *pApiBase, const char *pVersions)
{
    RegistrationRequestT *pRequest = &pConciergeService->Request;

    if (pRequest->strPingSite[0] != '\0')
    {
        // already registered, nothing left to do
        return(-1);
    }

    DirtyCastLogPrintf("conciergeservice: setting up CCS registration using [pingsite = %s, pool = %s, url = %s, versions = %s]\n", pPingSite, pPool, pApiBase, pVersions);

    // setup data
    ds_strnzcpy(pRequest->strPingSite, pPingSite, sizeof(pRequest->strPingSite));
    ds_strnzcpy(pRequest->strPool, pPool, sizeof(pRequest->strPool));
    ds_strnzcpy(pRequest->strApiBase, pApiBase, sizeof(pRequest->strApiBase));
    ds_strnzcpy(pRequest->strVersions, pVersions, sizeof(pRequest->strVersions));

    // make sure token acquisition is attempted next time the update function is pumped
    pConciergeService->uTokenRetryTick = NetTick();

    return(0);
}

/*F********************************************************************************/
/*!
    \Function ConciergeServiceControl

    \Description
        ConciergeService control function. Different selectors control different behaviors.

    \Input *pConciergeService    - module state
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
        'ccsa'      sets the CCS address
        'hrbt'      sets the request time to zero to allow the next update to heartbeat
        'spam'      sets the debug level
    \endverbatim

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t ConciergeServiceControl(ConciergeServiceRefT *pConciergeService, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'ccsa')
    {
        if (pValue != NULL && ds_strnlen((char*)pValue, (int32_t)sizeof(pConciergeService->strCcsAddr)) > 0)
        {
            ds_strnzcpy(pConciergeService->strCcsAddr, (char*)pValue,(int32_t)sizeof(pConciergeService->strCcsAddr));
            DirtyCastLogPrintf("conciergeservice: configured to register with %s\n", pConciergeService->strCcsAddr);
            return(0);
        }
        return(-1);
    }
    if (iControl == 'hrbt')
    {
        pConciergeService->uRequestTime = NetTick();
        return(0);
    }
    if (iControl == 'spam')
    {
        int32_t iHttpDebug;
        pConciergeService->uDebugLevel = (uint16_t)iValue;

        // we offset to make sure we don't get very spammy logs
        if ((iHttpDebug = iValue - CONCIERGESERVICE_HTTPDEBUG_OFFSET) > 0)
        {
            DirtyCastLogPrintf("conciergeservice: enabling protohttp debug logging\n");
            ProtoHttpControl(pConciergeService->pHttp, iControl, iHttpDebug, iValue2, pValue);
        }

        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ConciergeServiceStatus

    \Description
        ConciergeService status function. Different selectors control different behaviors.

    \Input *pConciergeService    - module state
    \Input iSelect  - control selector (see notes)
    \Input iValue   - selector specific
    \Input *pBuf    - selector specific
    \Input iBufSize - size of pBuf

    \Output
        int32_t - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'clid'      returns the server's client id. 0 is invalid and means it has not been received yet
        'done'      returns if the module has finished all its time critical operations
        'offl'      returns whether we are offline (unknown to CCS) or online (known to CCS)
        'regi'      returns whether ConciergeServiceRegister() has been used or not
    \endverbatim

    \Version 01/12/2016 (amakoukji)
*/
/********************************************************************************F*/
int32_t ConciergeServiceStatus(ConciergeServiceRefT *pConciergeService, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'clid')
    {
        return((int32_t)pConciergeService->Request.uHostingServerId);
    }
    if (iSelect == 'done')
    {
        return(ProtoHttpStatus(pConciergeService->pHttp, 'rsao', NULL, 0) == FALSE);
    }
    if (iSelect == 'offl')
    {
        return(pConciergeService->eCcsStatus == CCS_STATUS_OFFLINE);
    }
    if (iSelect == 'regi')
    {
        return(pConciergeService->Request.strPingSite[0] != '\0');
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ConciergeServiceDestroy

    \Description
        Destroy the module and release its state

    \Input *pConciergeService    - module state

    \Version 10/23/2015 (eesponda)
*/
/********************************************************************************F*/
void ConciergeServiceDestroy(ConciergeServiceRefT *pConciergeService)
{
    // close the watches
    if (pConciergeService->iCertWatchFd > 0)
    {
        DirtyCastWatchFileClose(pConciergeService->iCertWatchFd);
    }
    if (pConciergeService->iKeyWatchFd > 0)
    {
        DirtyCastWatchFileClose(pConciergeService->iKeyWatchFd);
    }

    ProtoHttpDestroy(pConciergeService->pHttp);
    pConciergeService->pHttp = NULL;

    if (pConciergeService->pKeyData != NULL)
    {
        DirtyMemFree(pConciergeService->pKeyData, CONCIERGESERVICE_MEMID, pConciergeService->iMemGroup, pConciergeService->pMemGroupUserData);
        pConciergeService->pKeyData = NULL;
    }

    if (pConciergeService->pCertData != NULL)
    {
        DirtyMemFree(pConciergeService->pCertData, CONCIERGESERVICE_MEMID, pConciergeService->iMemGroup, pConciergeService->pMemGroupUserData);
        pConciergeService->pCertData = NULL;
    }

    pConciergeService->pTokenApi = NULL;
    DirtyMemFree(pConciergeService, CONCIERGESERVICE_MEMID, pConciergeService->iMemGroup, pConciergeService->pMemGroupUserData);
}
