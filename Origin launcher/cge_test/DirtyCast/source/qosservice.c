/*H********************************************************************************/
/*!
    \File qosservice.c

    \Description
        This module handles communication with the QOS coordinator from the QOS server.

    \Copyright
        Copyright (c) Electronic Arts 2015. ALL RIGHTS RESERVED.

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/crypt/cryptrand.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/proto/protohttp2.h"
#include "DirtySDK/util/base64.h"
#include "libsample/zfile.h"

#include "tokenapi.h"
#include "dirtycast.h"
#include "qosservice.h"
#include "DirtySDK/misc/qoscommon.h"

/*** Defines **********************************************************************/
#define QOSSERVICE_HOST_DEFAULT         ("http://127.0.0.1:4001")
#define QOSSERVICE_HTTPDEBUG_OFFSET     (2)
#define QOSSERVICE_MEMID                ('qsss')
#define QOSSERVICE_HEARTBEAT_TIME       (20*1000)   //!< heartbeat to the qos coordinator every (x)ms, to let it know we are still available
#define QOSSERVICE_HEARTBEAT_TIMEOUT    (15*1000)   //!< if a heartbeat takes longer than this abandon it and let the next heartbeat start
#define QOSSERVICE_NEW_SECURE_KEY_RATE  (4)         //!< rotate secure keys every (y) heart beats
#define QOSSERVICE_NUM_SECURE_KEYS      (2)         //!< we have 2 secure keys active at a time, the current and one previous
#define QOSSERVICE_SECURE_KEY_USER_TIME (10*1000)   //!< how long minimum of a window do we want to have the client able to use the secure key
#define QOSSERVICE_HEARTBEAT_RETRY_TIME (5*1000)    //!< how long to wait before heartbeating again if we know the heartbeat failed
#define QOSSERVICE_TOKEN_RETRY_TIME     (15*1000)   //!< if a token fetch failure occurs, try again after x time.
#define QOSSERVICE_COORDINATOR_TIMEOUT  (((QOSSERVICE_HEARTBEAT_TIME * QOSSERVICE_NEW_SECURE_KEY_RATE * QOSSERVICE_NUM_SECURE_KEYS) - QOSSERVICE_HEARTBEAT_TIME) - QOSSERVICE_SECURE_KEY_USER_TIME) 
//!< if the coordinator hasn't heard from us in this long, it should abandon us, note that the coordinator does its own calculation currently 2.5 x QOSSERVICE_HEARTBEAT_TIME

/*** Type Definitions *************************************************************/

struct QosServiceRefT
{
    //! allocation information
    int32_t iMemGroup;
    void *pMemGroupUserData;

    uint8_t aCurrSecureKey[QOS_COMMON_SECURE_KEY_LENGTH];  //!< used to calculate hmac to validate probes
    uint8_t aPrevSecureKey[QOS_COMMON_SECURE_KEY_LENGTH];  //!< if the current hmac failed it may be because we rotated keys, we will try the previous hmac too
    uint8_t aOverrideSecureKey[QOS_COMMON_SECURE_KEY_LENGTH];  //!< value to use as a secure key, if using 'osky'
    uint8_t bUseOverrideSecureKey;          //!< if set the QoS server will send this as the secure key, every time

    ProtoHttp2RefT *pHttp;                  //!< for making requests to the coordinator, heartbeats/registration
    int32_t iHttpStreamId;                  //!< stream identifier generated on post
    uint8_t aReceiveBuff[QOS_COMMON_MAX_RPC_BODY_SIZE];  //!< receive buffer for http communication
    char strQosCoordinatorAddr[256];        //!< Address of the QOS coordinator service address
    char *pCertData;                        //!< loaded certificate data
    char *pKeyData;                         //!< loaded key data
    int32_t iKeySize;
    int32_t iCertSize;

    TokenApiRefT *pTokenApi;                //!< used for getting tokens
    TokenApiAccessDataT TokenData;          //!< cached token data

    QosCommonServerToCoordinatorRequestT Request;   //!< registration information
    uint32_t uRequestTime;                  //!< tick for when a new request should be made
    uint32_t uCurrentLoad;                  //!< load statistic to be shared with the coordinator
    uint32_t uMinServiceRequestID;          //!< the minimum acceptable service request id, as specified by the coordinator
    uint32_t uHeartBeatCount;               //!< count of the number of hearbeats we've done, used with QOSSERVICE_NEW_SECURE_KEY_RATE
    uint8_t bRegistrationStarted;           //!< have we started the registration process
    uint8_t bActiveHeartbeat;               //!< is there an active request?
    uint8_t bRegistrationSuccess;           //!< are we successfully registered with the coordinator
    uint16_t uDebugLevel;                   //!< debug level
    uint32_t uErrorTick;                    //!< when we hit the error

    char strCertPath[4096];                 //!< path to cert for reloading
    char strKeyPath[4096];                  //!< path to key for reloading
    int32_t iCertWatchFd;                   //!< file descriptor for the cert file watch
    int32_t iKeyWatchFd;                    //!< file descriptor for the key file watch
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _QosServiceProcessCertReload

    \Description
        Process the file watcher for the cert file. If we detect a file change
        it will load the new file and update the needed refs.

    \Input *pQosService - module state

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
static void _QosServiceProcessCertReload(QosServiceRefT *pQosService)
{
    char *pNewCertData;
    int32_t iNewCertSize;

    // check if the file has changed, if not nothing else to do
    const uint8_t bCertChanged = (pQosService->iCertWatchFd > 0) ? DirtyCastWatchFileProcess(pQosService->iCertWatchFd) : FALSE;
    if (bCertChanged == FALSE)
    {
        return;
    }

    // reload the file and set the data
    if ((pNewCertData = ZFileLoad(pQosService->strCertPath, &iNewCertSize, FALSE)) != NULL)
    {
        DirtyMemFree(pQosService->pCertData, QOSSERVICE_MEMID, pQosService->iMemGroup, pQosService->pMemGroupUserData);
        pQosService->pCertData = pNewCertData;
        pQosService->iCertSize = iNewCertSize;
    }
    else
    {
        DirtyCastLogPrintf("qosservice: [%p] failed to reload the cert file after getting a file change notification\n", pQosService);
    }

    // update the data in all the http (ssl) refs
    ProtoHttp2Control(pQosService->pHttp, 0, 'scrt', pQosService->iCertSize, 0, pQosService->pCertData);
    DirtyCastLogPrintfVerbose(pQosService->uDebugLevel, 0, "qosservice: [%p] successfully loaded new cert file\n", pQosService);
}

/*F********************************************************************************/
/*!
    \Function _ConciergeServiceProcessKeyReload

    \Description
        Process the file watcher for the key file. If we detect a file change
        it will load the new file and update the needed refs.

    \Input *pQosService - module state

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
static void _QosServiceProcessKeyReload(QosServiceRefT *pQosService)
{
    char *pNewKeyData;
    int32_t iNewKeySize;

    // check if the file has changed, if not nothing else to do
    const uint8_t bKeyChanged = (pQosService->iKeyWatchFd > 0) ? DirtyCastWatchFileProcess(pQosService->iKeyWatchFd) : FALSE;
    if (bKeyChanged == FALSE)
    {
        return;
    }

    // reload the file and set the data
    if ((pNewKeyData = ZFileLoad(pQosService->strKeyPath, &iNewKeySize, FALSE)) != NULL)
    {
        DirtyMemFree(pQosService->pKeyData, QOSSERVICE_MEMID, pQosService->iMemGroup, pQosService->pMemGroupUserData);
        pQosService->pKeyData = pNewKeyData;
        pQosService->iKeySize = iNewKeySize;
    }
    else
    {
        DirtyCastLogPrintf("qosservice: [%p] failed to reload the key file after getting a file change notification\n", pQosService);
    }

    // update the data in all the http (ssl) refs
    ProtoHttp2Control(pQosService->pHttp, 0, 'skey', pQosService->iKeySize, 0, pQosService->pKeyData);
    DirtyCastLogPrintfVerbose(pQosService->uDebugLevel, 0, "qosservice: [%p] successfully loaded new key file\n", pQosService);
}

/*F********************************************************************************/
/*!
    \Function _QosServiceCreateProtoHttp

    \Description
        Create and configure a ProtoHttp2RefT used to communicate to the coordinator

    \Input *pQosService     - module state

    \Version 12/14/2018 (cvienneau)
*/
/********************************************************************************F*/
static ProtoHttp2RefT * _QosServiceCreateProtoHttp(QosServiceRefT *pQosService)
{
    char strHeader[1024];
    ProtoHttp2RefT *pHttp;
    int32_t iHdrLen = 0;

    if (pQosService->pHttp != NULL)
    {
        DirtyCastLogPrintf("qosservice: warning, re-creating protohttp.\n");
        ProtoHttp2Destroy(pQosService->pHttp);
        pQosService->pHttp = NULL;
    }

    // create the ref for making requests to the qos coordinator
    if ((pHttp = ProtoHttp2Create(0)) == NULL)
    {
        return(NULL);
    }
    QosServiceControl(pQosService, 'spam', pQosService->uDebugLevel, 0, NULL); // this sets the debug level of protohttp too
    ProtoHttp2Control(pHttp, 0, 'scrt', pQosService->iCertSize, 0, pQosService->pCertData);
    ProtoHttp2Control(pHttp, 0, 'skey', pQosService->iKeySize, 0, pQosService->pKeyData);

    // format the header data for the protohttp ref as it only needs to be set when it changes
    iHdrLen += ds_snzprintf(strHeader + iHdrLen, sizeof(strHeader) - iHdrLen, "Authorization: NEXUS_S2S %s\r\n", pQosService->TokenData.strAccessToken);
    iHdrLen += ds_snzprintf(strHeader + iHdrLen, sizeof(strHeader) - iHdrLen, "te: trailers\r\ncontent-type: application/grpc\r\n");
    ProtoHttp2Control(pHttp, pQosService->iHttpStreamId, 'apnd', 0, 0, NULL);
    ProtoHttp2Control(pHttp, pQosService->iHttpStreamId, 'apnd', 0, 0, (void *)strHeader);

    pQosService->pHttp = pHttp;
    return(pHttp);
}

/*F********************************************************************************/
/*!
    \Function _QosServiceRotateKey

    \Description
        Generate a new secure key (to be passed to the QosCoordinator), and remember
        the old key for potential use on in flight requests.

    \Input *pQosService     - module state

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServiceRotateKey(QosServiceRefT *pQosService)
{
    
    if ((pQosService->uHeartBeatCount == 0) ||                                      // init the secure key the first time no matter what
        (((pQosService->uHeartBeatCount % QOSSERVICE_NEW_SECURE_KEY_RATE) == 0) &&  // we only actually rotate the key every so many heartbeats
        (pQosService->bRegistrationSuccess)))                                       // no use rotating keys if we can't tell anyone
    {
        char strPrevKey[64];
        char strCurrKey[64];

        //save the old key
        ds_memcpy(pQosService->aPrevSecureKey, pQosService->aCurrSecureKey, sizeof(pQosService->aPrevSecureKey));

        if (pQosService->bUseOverrideSecureKey == FALSE)
        {
            //generate new key
            CryptRandGet(pQosService->aCurrSecureKey, sizeof(pQosService->aCurrSecureKey));
        }
        else
        {
            //use whatever is in the override
            ds_memcpy(pQosService->aCurrSecureKey, pQosService->aOverrideSecureKey, sizeof(pQosService->aCurrSecureKey));
        }

        //print the active keys
        Base64Encode2((const char *)pQosService->aPrevSecureKey, sizeof(pQosService->aPrevSecureKey), strPrevKey, sizeof(strPrevKey));
        Base64Encode2((const char *)pQosService->aCurrSecureKey, sizeof(pQosService->aCurrSecureKey), strCurrKey, sizeof(strCurrKey));
        DirtyCastLogPrintf("qosservice: strCurrKey=%s, strPrevKey=%s.\n", strCurrKey, strPrevKey);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosServiceSetErrorTime

    \Description
    Sets the time the first error occurred.

    \Input *pQosService   - module state

    \Output
    char *       - TRUE if a new error time was set, false otherwise

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t _QosServiceSetErrorTime(QosServiceRefT *pQosService)
{
    uint8_t bRet = FALSE;
    //first time in this error state
    if (pQosService->uErrorTick == 0)
    {
        pQosService->uErrorTick = NetTick();
        bRet = TRUE;
    }

    //if there is an active stream, clean it up, this happens often when getting an http error code
    if (pQosService->iHttpStreamId != 0)
    {
        ProtoHttp2StreamFree(pQosService->pHttp, pQosService->iHttpStreamId);
        pQosService->iHttpStreamId = 0;
    }

    // get a new protohttp, to clean up as much state as we can
    _QosServiceCreateProtoHttp(pQosService);

    //if we have gone into or are remaining in an error state, lets say our request aren't active anymore so we retry
    pQosService->bActiveHeartbeat = FALSE;
    pQosService->bRegistrationSuccess = FALSE;
    return(bRet);
}

/*F********************************************************************************/
/*!
    \Function _QosServiceHeartbeat

    \Description
        Sends a heartbeat to the QOS coordinator

    \Input *pQosService     - module state

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServiceHeartbeat(QosServiceRefT *pQosService)
{
    uint8_t aEncodedMsgBuff[QOS_COMMON_MAX_RPC_BODY_SIZE], *pEncodedMsg;
    uint32_t uEncodedMsgLength = 0;
    int32_t iResult;
    char strUri[QOS_COMMON_MAX_URL_LENGTH];
    QosCommonServerToCoordinatorRequestT *pRequest = &pQosService->Request;

    //ready a new secure key
    _QosServiceRotateKey(pQosService);
    ds_memcpy(pRequest->aSecureKey, pQosService->aCurrSecureKey, sizeof(pRequest->aSecureKey));
    pRequest->uLastLoadPerSec = pQosService->uCurrentLoad;
    pRequest->uUpdateInterval = QOSSERVICE_HEARTBEAT_TIME;  //the coordinator uses the heartbeat time to do a calculation like QOSSERVICE_COORDINATOR_TIMEOUT

    pEncodedMsg = QosCommonServerToCoordinatorRequestEncode(pRequest, aEncodedMsgBuff, sizeof(aEncodedMsgBuff), &uEncodedMsgLength);

    if (pQosService->uDebugLevel > 2)
    {
        NetPrintMem(pEncodedMsg, uEncodedMsgLength, "ServerToCoordinatorRequest");
    }

    // format URL
    ds_snzprintf(strUri, sizeof(strUri), "%s%s", pQosService->strQosCoordinatorAddr, QOS_COMMON_SERVER_URL);

    // if there is an active stream, clean it up (this should never happen)
    if (pQosService->iHttpStreamId != 0)
    {
        DirtyCastLogPrintf("qosservice: cleaning up unexpected open stream, %d\n", pQosService->iHttpStreamId);
        ProtoHttp2StreamFree(pQosService->pHttp, pQosService->iHttpStreamId);
        pQosService->iHttpStreamId = 0;
    }

    // make request
    if ((iResult = ProtoHttp2Request(pQosService->pHttp, strUri, pEncodedMsg, uEncodedMsgLength, PROTOHTTP_REQUESTTYPE_POST, &pQosService->iHttpStreamId)) > 0)
    {
        pQosService->bActiveHeartbeat = TRUE;
        pQosService->uHeartBeatCount++;
        if (!pRequest->bShuttingDown)
        {
            // schedule a heart beat at the normal frequency
            pQosService->uRequestTime = NetTick() + QOSSERVICE_HEARTBEAT_TIME;
        }
        else
        {
            // disable heartbeats, this will be the last one the server gets from us
            pQosService->uRequestTime = 0;
        }
        DirtyCastLogPrintf("qosservice: _QosServiceHeartbeat, in progress iResult=%d, next at %u.\n", iResult, pQosService->uRequestTime);

    }
    else
    {
        DirtyCastLogPrintf("qosservice: _QosServiceHeartbeat, ProtoHttp2Request() failed %d\n", iResult);
        pQosService->uRequestTime = NetTick() + QOSSERVICE_HEARTBEAT_RETRY_TIME;
        _QosServiceSetErrorTime(pQosService);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosServiceCheckRequest

    \Description
        Updates and checks our ProtoHttp request that is in progress

    \Input *pQosService   - module state

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServiceCheckRequest(QosServiceRefT *pQosService)
{
    int32_t iResult;
    QosCommonCoordinatorToServerResponseT registrationResponse;

    if ((iResult = ProtoHttp2RecvAll(pQosService->pHttp, pQosService->iHttpStreamId, pQosService->aReceiveBuff, sizeof(pQosService->aReceiveBuff))) >= 0)
    {
        int32_t iHttpStatusCode = ProtoHttp2Status(pQosService->pHttp, pQosService->iHttpStreamId, 'code', NULL, 0);
        if (iHttpStatusCode == PROTOHTTP_RESPONSE_SUCCESSFUL)
        {
            // log the response
            if (pQosService->uDebugLevel > 2)
            {
                NetPrintMem(pQosService->aReceiveBuff, iResult, "CoordinatorToServerResponse");
            }            
                
            // parse the http response
            if (QosCommonCoordinatorToServerResponseDecode(&registrationResponse, pQosService->aReceiveBuff, iResult) >= 0)
            {
                // request finished
                DirtyCastLogPrintf("qosservice: response payload %d bytes; registration message: %s\n", iResult, registrationResponse.strRegistrationMessage);
                ProtoHttp2StreamFree(pQosService->pHttp, pQosService->iHttpStreamId);
                pQosService->iHttpStreamId = 0;
                pQosService->bActiveHeartbeat = FALSE;   
                pQosService->bRegistrationSuccess = FALSE; // we will check this next

                // check to see if we successfully registered
                if ((ds_strnicmp("UPDATED", registrationResponse.strRegistrationMessage, 7) == 0) ||
                    (ds_strnicmp("ADDED", registrationResponse.strRegistrationMessage, 5) == 0))
                {
                    pQosService->bRegistrationSuccess = TRUE;
                    #if DIRTYSDK_VERSION >= 1501040002
                        pQosService->uMinServiceRequestID = registrationResponse.uMinServiceRequestID;
                    #else
                        pQosService->uMinServiceRequestID = 0;
                    #endif
                    // if we recovered from an error then log it so we can figure out how long we were in error for
                    if (pQosService->uErrorTick > 0)
                    {
                        DirtyCastLogPrintf("qosservice: registration recovered from error (time=%ds)\n",
                            NetTickDiff(NetTick(), pQosService->uErrorTick) / 1000);
                        pQosService->uErrorTick = 0;
                    }
                }
                else
                {
                    // unexpected response
                    DirtyCastLogPrintf("qosservice: coordinator did not provide expected response.\n");
                    NetPrintMem(pQosService->aReceiveBuff, iResult, "ReceiveBuff");
                    _QosServiceSetErrorTime(pQosService);
                }
            }
            else
            {
                // decode error occurred
                DirtyCastLogPrintf("qosservice: failed to decode response.\n");
                NetPrintMem(pQosService->aReceiveBuff, iResult, "ReceiveBuff");
                _QosServiceSetErrorTime(pQosService);
            }
        }
        else
        {
            // http error occurred
            DirtyCastLogPrintf("qosservice: registration failed, http status=%d, iResult=%d\n", iHttpStatusCode, iResult);
            NetPrintMem(pQosService->aReceiveBuff, iResult, "ReceiveBuff");
            _QosServiceSetErrorTime(pQosService);
        }
    }
    else if (iResult == PROTOHTTP2_RECVWAIT)
    {
        //DirtyCastLogPrintf("qosservice: Waiting for complete http response to arrive.\n");
    }
    else if (iResult == PROTOHTTP2_RECVBUFF)
    {
        // not enough space to receive into, we don't grow the buffer so this is critical
        DirtyCastLogPrintf("qosservice: Insufficient buffer space to receive.\n");
        _QosServiceSetErrorTime(pQosService);
    }
    else if (iResult < 0)
    {
        int32_t hResult = ProtoHttp2Status(pQosService->pHttp, pQosService->iHttpStreamId, 'hres', NULL, 0);
        DirtyCastLogPrintf("qosservice: error ProtoHttp2RecvAll failed iResult=%d, hResult=%d, errno=%d.\n", iResult, hResult, DirtyCastGetLastError());
        _QosServiceSetErrorTime(pQosService);
    }
    
    // the current heartbeat has been going on too long, a new heartbeat should be started soon
    if ((pQosService->uRequestTime != 0) && (NetTickDiff(NetTick(), pQosService->uRequestTime - (uint32_t)(QOSSERVICE_HEARTBEAT_TIME - QOSSERVICE_HEARTBEAT_TIMEOUT)) > 0))
    {
        DirtyCastLogPrintf("qosservice: error the current heartbeat has taken too long, entering error state.\n");
        _QosServiceSetErrorTime(pQosService);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosServiceTokenCb

    \Description
        Callback that is fired when the access token request is complete

    \Input *pData       - the response information
    \Input *pUserData   - contains our module ref

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************F*/
static void _QosServiceTokenCb(const TokenApiAccessDataT *pData, void *pUserData)
{
    QosServiceRefT *pQosService = (QosServiceRefT *)pUserData;

    if (pData != NULL)
    {
        char strHeader[8*1024];
        int32_t iHdrLen = 0;

        // copy the new data
        ds_memcpy(&pQosService->TokenData, pData, sizeof(pQosService->TokenData));

        // format the header data for the protohttp ref as it only needs to be set when it changes
        iHdrLen += ds_snzprintf(strHeader+iHdrLen, sizeof(strHeader)-iHdrLen, "Authorization: NEXUS_S2S %s\r\n", pQosService->TokenData.strAccessToken);
        iHdrLen += ds_snzprintf(strHeader+iHdrLen, sizeof(strHeader)-iHdrLen, "te: trailers\r\ncontent-type: application/grpc\r\n");
        ProtoHttp2Control(pQosService->pHttp, pQosService->iHttpStreamId, 'apnd', 0, 0, NULL);
        ProtoHttp2Control(pQosService->pHttp, pQosService->iHttpStreamId, 'apnd', 0, 0, (void *)strHeader);
    }
    else
    {
        // failure, reset the data
        ds_memclr(&pQosService->TokenData, sizeof(pQosService->TokenData));
        pQosService->TokenData.iExpiresIn = NetTick() + QOSSERVICE_TOKEN_RETRY_TIME;  // wait a bit, don't fetch again right away, but ideally before the next heartbeat attempt
    }
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function QosServiceCreate

    \Description
        Allocate module state and prepare for use

    \Input *pTokenApi       - ref to tokenapi module used to get access tokens
    \Input *pCertFile       - file to use for certificate (NULL if not used)
    \Input *pKeyFile        - file to use for certificate key (NULL if not used)

    \Output
        QosServiceRefT*   - pointer to module state, or NULL

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************F*/
QosServiceRefT *QosServiceCreate(TokenApiRefT *pTokenApi, const char *pCertFile, const char *pKeyFile)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    QosServiceRefT *pQosService;

    // query memory group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // create the module state
    if ((pQosService = (QosServiceRefT *)DirtyMemAlloc(sizeof(*pQosService), QOSSERVICE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }

    ds_memclr(pQosService, sizeof(*pQosService));
    pQosService->iMemGroup = iMemGroup;
    pQosService->pMemGroupUserData = pMemGroupUserData;
    pQosService->pTokenApi = pTokenApi;
    pQosService->uRequestTime = NetTick();
    ds_strnzcpy(pQosService->strQosCoordinatorAddr, QOSSERVICE_HOST_DEFAULT, sizeof(pQosService->strQosCoordinatorAddr));

    if ((pCertFile != NULL) && (pCertFile[0] != '\0'))
    {
        // load certificate; save the path and create a watch to handle reloading
        if ((pQosService->pCertData = ZFileLoad(pCertFile, &pQosService->iCertSize, FALSE)) == NULL)
        {
            QosServiceDestroy(pQosService);
            return(NULL);
        }
        ds_strnzcpy(pQosService->strCertPath, pCertFile, sizeof(pQosService->strCertPath));
        pQosService->iCertWatchFd = DirtyCastWatchFileInit(pCertFile);
    }
    if ((pKeyFile != NULL) && (pKeyFile[0] != '\0'))
    {
        // load key; save the path and create a watch to handle reloading
        if ((pQosService->pKeyData = ZFileLoad(pKeyFile, &pQosService->iKeySize, FALSE)) == NULL)
        {
            QosServiceDestroy(pQosService);
            return(NULL);
        }
        ds_strnzcpy(pQosService->strKeyPath, pKeyFile, sizeof(pQosService->strKeyPath));
        pQosService->iKeyWatchFd = DirtyCastWatchFileInit(pKeyFile);
    }

    // create the ref for making requests to the qos coordinator
    if (_QosServiceCreateProtoHttp(pQosService) == NULL)
    {
        QosServiceDestroy(pQosService);
        return(NULL);
    }

    return(pQosService);
}

/*F********************************************************************************/
/*!
    \Function QosServiceUpdate

    \Description
        Give time to module to do its thing (should be called periodically to
        allow module to perform work)

    \Input *pQosService   - module state

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************F*/
void QosServiceUpdate(QosServiceRefT *pQosService)
{
    // check the file watches
    _QosServiceProcessCertReload(pQosService);
    _QosServiceProcessKeyReload(pQosService);

    // update even if we are not active we want to update the socket (cases where server closes socket)
    ProtoHttp2Update(pQosService->pHttp);

    if (pQosService->bActiveHeartbeat == TRUE)
    {
        _QosServiceCheckRequest(pQosService);
        return;
    }

    // don't do anything else if we are in process of requesting to nucleus
    if (TokenApiStatus(pQosService->pTokenApi, 'done', NULL, 0) == FALSE)
    {
        return;
    }

    /* if we are not registered there is nothing left to do

       in the case that register has not been called we do not want to fire a new token
       request or try to send a heartbeat to QOS coordinator

       in the case that register has been called at this point we confirm that no
       token requests are currently active by the evaluate this check so we can
       continue without any additional checks */
    if (pQosService->bRegistrationStarted == FALSE)
    {
        return;
    }

    // check if our token will expire soon
    if (NetTickDiff(NetTick(), pQosService->TokenData.iExpiresIn) > 0)
    {
        TokenApiAcquisitionAsync(pQosService->pTokenApi, _QosServiceTokenCb, pQosService);
        return;
    }

    // kick off a new request
    if ((pQosService->uRequestTime != 0) && (NetTickDiff(NetTick(), pQosService->uRequestTime) > 0))
    {
        _QosServiceHeartbeat(pQosService);
    }
}

/*F********************************************************************************/
/*!
    \Function QosServiceRegister

    \Description
        Sets our registration data to later use for heartbeats

    \Input *pQosService         - module state
    \Input *registrationInfo    - the data to be used in the hearbeat

    \Output
        int32_t - zero=success,negative=failure
*/
/********************************************************************************F*/
int32_t QosServiceRegister(QosServiceRefT *pQosService, const QosCommonServerToCoordinatorRequestT *pRegistrationInfo)
{
    QosCommonServerToCoordinatorRequestT *pRequest = &pQosService->Request;

    if (pQosService->bRegistrationStarted == TRUE)
    {
        // already started, nothing left to do
        return(-1);
    }

    DirtyCastLogPrintf("qosservice: registering to the QOS coordinator with [ site = %s, url = %s:%d]\n", 
        pRegistrationInfo->strSiteName, pRegistrationInfo->strAddr, pRegistrationInfo->uPort);
    ds_memcpy(pRequest, pRegistrationInfo, sizeof(QosCommonServerToCoordinatorRequestT));
    pQosService->bRegistrationStarted = TRUE;

    // first get a token to make a request
    if (TokenApiAcquisitionAsync(pQosService->pTokenApi, _QosServiceTokenCb, pQosService) > 0)
    {
        return(0);
    }

    // error;
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function QosServiceControl

    \Description
        QosService control function. Different selectors control different behaviors.

    \Input *pQosService    - module state
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
        'osky'      sets the override secure key
        'qcad'      sets the QOS Coordinator address
        'hrbt'      sets the request time to zero to allow the next update to heartbeat
        'load'      sets the current load to be shared with the coordinator on its next heartbeat
        'shut'      lets the coordinator know that this server will be shutting down
        'spam'      sets the debug level
    \endverbatim

    \Version 10/23/2016 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosServiceControl(QosServiceRefT *pQosService, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    if (iControl == 'osky')
    {
        if (pValue != NULL)
        {
            ds_memcpy(pQosService->aOverrideSecureKey, pValue, sizeof(pQosService->aOverrideSecureKey));
            pQosService->bUseOverrideSecureKey = TRUE;
            return(0);
        }
        return(-1);
    }
    if (iControl == 'qcad')
    {
        if (pValue != NULL && ds_strnlen((char*)pValue, (int32_t)sizeof(pQosService->strQosCoordinatorAddr)) > 0)
        {
            ds_strnzcpy(pQosService->strQosCoordinatorAddr, (char*)pValue,(int32_t)sizeof(pQosService->strQosCoordinatorAddr));
            DirtyCastLogPrintf("qosservice: configured to register with %s\n", pQosService->strQosCoordinatorAddr);
            return(0);
        }
        return(-1);
    }
    if (iControl == 'hrbt')
    {
        pQosService->uRequestTime = NetTick();
        return(0);
    }
    if (iControl == 'load')
    {
        pQosService->uCurrentLoad = iValue;
    }
    if (iControl == 'shut')
    {
        pQosService->Request.bShuttingDown = TRUE;
        pQosService->uRequestTime = NetTick();
        return(0);
    }    
    if (iControl == 'spam')
    {
        int32_t iHttpDebug;
        pQosService->uDebugLevel = (uint16_t)iValue;

        // we offset to make sure we don't get very spammy logs
        if ((iHttpDebug = iValue - QOSSERVICE_HTTPDEBUG_OFFSET) > 0)
        {
            if (pQosService->pHttp != NULL)
            {
                DirtyCastLogPrintf("qosservice: enabling protohttp debug logging.\n");
                ProtoHttp2Control(pQosService->pHttp, pQosService->iHttpStreamId, iControl, iHttpDebug, iValue2, pValue);
            }
            else
            {
                DirtyCastLogPrintf("qosservice: skipped enabling protohttp debug logging, because pQosService->pHttp == NULL.\n");
            }
        }

        return(0);
    }

    return(-1);
}

/*F********************************************************************************/
/*!
    \Function QosServiceStatus

    \Description
        QosService status function. Different selectors control different behaviors.

    \Input *pQosService    - module state
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
        'done'      returns if the module has finished all its time critical operations
        'msid'      returns the mininum service id the qos server should accept
        'rcoo'      returns 1 if registered with a coordinator, else 0
        'secc'      returns the current secure key
        'secp'      returns the previous secure key
    \endverbatim

    \Version 01/12/2017 (cvienneau)
*/
/********************************************************************************F*/
int32_t QosServiceStatus(QosServiceRefT *pQosService, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'done')
    {
        return(ProtoHttp2Status(pQosService->pHttp, pQosService->iHttpStreamId, 'rsao', NULL, 0) == FALSE);
    }
    else if (iSelect == 'msid')
    {
        return(pQosService->uMinServiceRequestID);
    }
    else if (iSelect == 'rcoo')
    {
        return(pQosService->bRegistrationSuccess);
    }
    else if (iSelect == 'secc')
    {
        ds_memcpy_s(pBuf, iBufSize, pQosService->aCurrSecureKey, sizeof(pQosService->aCurrSecureKey));
        return(0);
    }
    else if (iSelect == 'secp')
    {
        ds_memcpy_s(pBuf, iBufSize, pQosService->aPrevSecureKey, sizeof(pQosService->aPrevSecureKey));
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function QosServiceDestroy

    \Description
        Destroy the module and release its state

    \Input *pQosService    - module state

    \Version 10/23/2015 (cvienneau)
*/
/********************************************************************************F*/
void QosServiceDestroy(QosServiceRefT *pQosService)
{
    // close the watches
    if (pQosService->iCertWatchFd > 0)
    {
        DirtyCastWatchFileClose(pQosService->iCertWatchFd);
    }
    if (pQosService->iKeyWatchFd > 0)
    {
        DirtyCastWatchFileClose(pQosService->iKeyWatchFd);
    }

    if (pQosService->pHttp != NULL)
    {
        ProtoHttp2Destroy(pQosService->pHttp);
        pQosService->pHttp = NULL;
    }

    if (pQosService->pKeyData != NULL)
    {
        DirtyMemFree(pQosService->pKeyData, QOSSERVICE_MEMID, pQosService->iMemGroup, pQosService->pMemGroupUserData);
        pQosService->pKeyData = NULL;
    }

    if (pQosService->pCertData != NULL)
    {
        DirtyMemFree(pQosService->pCertData, QOSSERVICE_MEMID, pQosService->iMemGroup, pQosService->pMemGroupUserData);
        pQosService->pCertData = NULL;
    }

    pQosService->pTokenApi = NULL;

    DirtyMemFree(pQosService, QOSSERVICE_MEMID, pQosService->iMemGroup, pQosService->pMemGroupUserData);
}
