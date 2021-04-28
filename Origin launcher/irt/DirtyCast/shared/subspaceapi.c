/*H********************************************************************************/
/*!
    \File subspaceapi.c

    \Description
        Module for interacting with the subspace sidekick application.  The subspace
        network is a means of improving latency by using improved, private routes, 
        that are not typically available over the internet.  The subspace sidekick 
        application is used to setup NAT to gain access to the subspace network.

    \Copyright
        Copyright (c) 2020 Electronic Arts Inc.

    \Version 04/24/2020 (cvienneau) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <stdio.h>
#include <string.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/util/jsonparse.h"

#include "dirtycast.h"
#include "subspaceapi.h"

/*** Defines **********************************************************************/

//! module memid
#define SUBSPACEAPI_MEMID           ('ssap')
#define SUBSPACEAPI_DEBUG_LEVEL     (1)
#define SUBSPACEAPI_PROVISION_URL   ("provision")
#define SUBSPACEAPI_STOP_URL        ("stop")
#define SUBSPACEAPI_QUERY_URL       ("query")   // todo implement "query" api which may be used by OTP
#define SUBSPACEAPI_MAX_JSON_SIZE   (2048)
#define SUBSPACEAPI_REQUEST_TIMEOUT (10000)

/*** Type Definitions *************************************************************/

// module state
struct SubspaceApiRefT
{
    //! module memory group
    int32_t iMemGroup;
    void *pMemGroupUserData;

    //! state machine
    SubspaceApiRequestTypeE eRequestType;
    SubspaceApiStateE eState;

    //! for making calls to sidekick app
    ProtoHttpRefT *pHttp;   
    char aReceiveBuff[2048];  //!< receive buffer for http communication
    char aSendBuff[1024];     //!< send buffer for http communication
    int32_t iBytesSent;       //!< amount of data sent for the current request
    int32_t iBytesToSend;     //!< amount of data that should be sent for the current request

    //! info about the subspace sidekick we will communicate to
    char strSubspaceSidekickAddr[256];
    uint16_t uSubspaceSidekickPort;

    //! results from provision
    uint32_t uSubspaceLocalIp;
    uint16_t uSubspaceLocalPort;

    uint32_t uDebugLevel;
};

typedef struct SubspaceProvisionResponseT
{
    uint32_t uIp;
    uint16_t uPort;
    uint8_t _pad[2];
} SubspaceProvisionResponseT;

typedef struct SubspaceStopResponseT
{
    uint32_t uIp;
    uint16_t uPort;
    uint8_t _pad[2];
} SubspaceStopResponseT;

/*** Variables ********************************************************************/


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _SubspaceResponseProvision

    \Description
        Decode buffer into a  SubspaceProvisionResponseT, which
        tells the provision registration status.

    \Input *pSidekickResponse   - [out] structure to write values to
    \Input *pBuffer             - buffer we are reading from
    \Input uBuffSize            - size of buffer

    \Output
        int32_t                 - negative on err, 0 on success

    \Version 04/28/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _SubspaceResponseProvision(SubspaceProvisionResponseT *pSidekickResponse, const char *pBuffer, uint32_t uBuffSize)
{
    uint16_t aJsonParse[SUBSPACEAPI_MAX_JSON_SIZE];
    const char *pCurrent = "";

    // zero out the parsing table
    ds_memclr(aJsonParse, sizeof(aJsonParse));
    ds_memclr(pSidekickResponse, sizeof(SubspaceProvisionResponseT));

    if (JsonParse(aJsonParse, SUBSPACEAPI_MAX_JSON_SIZE, pBuffer, uBuffSize) == 0)
    {
        DirtyCastLogPrintf("subspaceapi: error -- json parse table size too small\n");
        return(-1);
    }

    if ((pCurrent = JsonFind(aJsonParse, "subspace_port")) != NULL)
    {
        pSidekickResponse->uPort = JsonGetInteger(pCurrent, 0);
    }
    else
    {
        return(-2);
    }

    if ((pCurrent = JsonFind(aJsonParse, "subspace_ipv4")) != NULL)
    {
        char strIp[32];
        struct sockaddr TempAddr;
        SockaddrInit(&TempAddr, AF_INET);
        JsonGetString(pCurrent, strIp, sizeof(strIp), "");
        SockaddrInSetAddrText(&TempAddr, strIp);
        pSidekickResponse->uIp = SockaddrInGetAddr(&TempAddr);
    }
    else
    {
        return(-3);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _SubspaceResponseStop

    \Description
        Decode buffer into a  SubspaceStopResponseT, which
        tells the stop status.

    \Input *pSidekickResponse   - [out] structure to write values to
    \Input *pBuffer             - buffer we are reading from
    \Input uBuffSize            - size of buffer

    \Output
    int32_t                     - negative on err, 0 on success

    \Version 04/28/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _SubspaceResponseStop(SubspaceStopResponseT *pSidekickResponse, const char *pBuffer, uint32_t uBuffSize)
{
    uint16_t aJsonParse[SUBSPACEAPI_MAX_JSON_SIZE];
    const char *pCurrent = "";

    // zero out the parsing table
    ds_memclr(aJsonParse, sizeof(aJsonParse));
    ds_memclr(pSidekickResponse, sizeof(SubspaceStopResponseT));

    if (JsonParse(aJsonParse, SUBSPACEAPI_MAX_JSON_SIZE, pBuffer, uBuffSize) == 0)
    {
        DirtyCastLogPrintf("subspaceapi: error -- json parse table size too small\n");
        return(-1);
    }

    if ((pCurrent = JsonFind(aJsonParse, "server_port")) != NULL)
    {
        pSidekickResponse->uPort = JsonGetInteger(pCurrent, 0);
    }
    else
    {
        return(-2);
    }

    if ((pCurrent = JsonFind(aJsonParse, "server_ip")) != NULL)
    {
        char strIp[32];
        struct sockaddr TempAddr;
        SockaddrInit(&TempAddr, AF_INET);
        JsonGetString(pCurrent, strIp, sizeof(strIp), "");
        SockaddrInSetAddrText(&TempAddr, strIp);
        pSidekickResponse->uIp = SockaddrInGetAddr(&TempAddr);
    }
    else
    {
        return(-3);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _SubspaceResponse

    \Description
        Decode and act on the current request.

    \Input *pSubspaceApi        - module state
    \Input *pBuffer             - buffer we are reading from
    \Input uBuffSize            - size of buffer

    \Output
        int32_t                 - negative on err, 0 on success

    \Version 04/28/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _SubspaceResponse(SubspaceApiRefT *pSubspaceApi, const char *pBuffer, uint32_t uBuffSize)
{
    if (pSubspaceApi->eRequestType == SUBSPACE_REQUEST_TYPE_PROVISION)
    {
        SubspaceProvisionResponseT ProvisionResponse;
        if (_SubspaceResponseProvision(&ProvisionResponse, pBuffer, uBuffSize) >= 0)
        {
            // validate we are happy with the response
            if ((ProvisionResponse.uPort == 0) || (ProvisionResponse.uIp == 0))
            {
                DirtyCastLogPrintf("subspaceapi: provision response failed validation\n");
                return(-1);
            }

            // set the new values use by subspace
            pSubspaceApi->uSubspaceLocalIp = ProvisionResponse.uIp;
            pSubspaceApi->uSubspaceLocalPort = ProvisionResponse.uPort;
            return(0);
        }
    }
    else if (pSubspaceApi->eRequestType == SUBSPACE_REQUEST_TYPE_STOP)
    {
        SubspaceStopResponseT StopResponse;
        if (_SubspaceResponseStop(&StopResponse, pBuffer, uBuffSize) >= 0)
        {
            // validate we are happy with the response
            if ((StopResponse.uPort == 0) || (StopResponse.uIp == 0))
            {
                DirtyCastLogPrintf("subspaceapi: stop response failed validation\n");
                return(-2);
            }

            // clear the values use by subspace
            pSubspaceApi->uSubspaceLocalIp = 0;
            pSubspaceApi->uSubspaceLocalPort = 0;
            return(0);
        }
    }
    DirtyCastLogPrintf("subspaceapi: error response unhandled response type.\n");

    return(-3);
}

/*F********************************************************************************/
/*!
    \Function _SubspaceApiCheckRequest

    \Description
        Updates and checks our ProtoHttp request that is in progress

    \Input *pSubspaceApi     - module state

    \Version 04/27/2020 (cvienneau)
*/
/********************************************************************************F*/
static void _SubspaceApiRequestCheck(SubspaceApiRefT *pSubspaceApi)
{
    int32_t iResult;

    if ((iResult = ProtoHttpStatus(pSubspaceApi->pHttp, 'done', NULL, 0)) > 0)
    {
        if ((iResult = ProtoHttpRecvAll(pSubspaceApi->pHttp, pSubspaceApi->aReceiveBuff, sizeof(pSubspaceApi->aReceiveBuff))) >= 0)
        {
            int32_t iHttpStatusCode = ProtoHttpStatus(pSubspaceApi->pHttp, 'code', NULL, 0);

            if (PROTOHTTP_GetResponseClass(iHttpStatusCode) == PROTOHTTP_RESPONSE_SUCCESSFUL)
            {
                // log the response
                DirtyCastLogPrintfVerbose(pSubspaceApi->uDebugLevel, 0, "subspaceapi: SubspaceResponse:\n%s", pSubspaceApi->aReceiveBuff);

                // parse/act on the response
                if (_SubspaceResponse(pSubspaceApi, pSubspaceApi->aReceiveBuff, iResult) >= 0)
                {
                    // request finished successfully
                    pSubspaceApi->eState = SUBSPACE_STATE_DONE;
                }
                else
                {
                    // decode error occurred
                    DirtyCastLogPrintf("subspaceapi: failed to decode response:\n");
                    NetPrintf(("%s", pSubspaceApi->aReceiveBuff));
                    pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
                }
            }
            else
            {
                // http error occurred
                DirtyCastLogPrintf("subspaceapi: request failed, http status=%d, iResult=%d\n", iHttpStatusCode, iResult);
                NetPrintf(("%s\n", pSubspaceApi->aReceiveBuff));
                pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
            }
        }
        else if (iResult == PROTOHTTP_RECVBUFF)
        {
            // not enough space to receive into, we don't grow the buffer so this is critical
            DirtyCastLogPrintf("subspaceapi: Insufficient buffer space to receive.\n");
            pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
        }
        else if (iResult != PROTOHTTP_RECVWAIT)
        {
            int32_t hResult = ProtoHttpStatus(pSubspaceApi->pHttp, 'hres', NULL, 0);
            DirtyCastLogPrintf("subspaceapi: error ProtoHttpRecvAll %s iResult=%d, hResult=%d, errno=%d.\n", ProtoHttpStatus(pSubspaceApi->pHttp, 'time', NULL, 0) ? "timeout" : "failed", iResult, hResult, DirtyCastGetLastError());
            pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
        }
    }
    else if (iResult < 0)
    {
        int32_t hResult = ProtoHttpStatus(pSubspaceApi->pHttp, 'hres', NULL, 0);
        DirtyCastLogPrintf("subspaceapi: error 'done' %s iResult=%d, hResult=%d, errno=%d.\n", ProtoHttpStatus(pSubspaceApi->pHttp, 'time', NULL, 0) ? "timeout" : "failed", iResult, hResult, DirtyCastGetLastError());
        pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
    }
}

/*F********************************************************************************/
/*!
    \Function _SubspaceApiBeginProvision

    \Description
        Use the provision method to get a Subspace IP and port for
        proxying optimized traffic to and from your game server

    \Input *pSubspaceApi     - module state
    \Input uServerAddr       - address of this server
    \Input uServerPort       - port of this server
    \Input strServerUniqueID - unique identifier for the server, if ip/port is not sufficient

    \Output
        int32_t              - negative on err, 0 on success

    \Version 04/27/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _SubspaceApiBeginProvision(SubspaceApiRefT *pSubspaceApi, uint32_t uServerIp, uint16_t uServerPort, char* strServerUniqueID)
{
    char strUri[128];

    if (pSubspaceApi->eState != SUBSPACE_STATE_IDLE)
    {
        DirtyCastLogPrintf("subspaceapi: _SubspaceApiBeginProvision error, can not make a request while there is a pending request.");
        pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
        return(-1);
    }

    ds_snzprintf(pSubspaceApi->aSendBuff, sizeof(pSubspaceApi->aSendBuff), "{ \"server_port\": %d, \"server_ip\": \"%a\", \"external_id\": \"%s\" }",
        uServerPort, uServerIp, strServerUniqueID);
    DirtyCastLogPrintfVerbose(pSubspaceApi->uDebugLevel, 0, "subspaceapi: _SubspaceApiBeginProvision:\n%s\n", pSubspaceApi->aSendBuff);

    // format URL
    ds_snzprintf(strUri, sizeof(strUri), "http://%s:%d/%s", pSubspaceApi->strSubspaceSidekickAddr, pSubspaceApi->uSubspaceSidekickPort, SUBSPACEAPI_PROVISION_URL);

    // make request
    pSubspaceApi->iBytesToSend = (int32_t)strlen(pSubspaceApi->aSendBuff);
    if ((pSubspaceApi->iBytesSent = ProtoHttpPost(pSubspaceApi->pHttp, strUri, pSubspaceApi->aSendBuff, pSubspaceApi->iBytesToSend, FALSE)) >= 0)
    {
        pSubspaceApi->eRequestType = SUBSPACE_REQUEST_TYPE_PROVISION;
        pSubspaceApi->eState = SUBSPACE_STATE_IN_PROGRESS;
        return(0);
    }
    else
    {
        DirtyCastLogPrintf("subspaceapi: _SubspaceApiBeginProvision, ProtoHttpPost() failed %d\n", pSubspaceApi->iBytesSent);
        pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
        return(-2);
    }
}

/*F********************************************************************************/
/*!
    \Function _SubspaceApiBeginStop

    \Description
        Use the stop method to terminate all of the Subspace IPs and Ports provisioned by this
        Sidekick. We supply the optional server_ip and server_port parameters to only stop
        proxying the supplied server_port .

    \Input *pSubspaceApi     - module state
    \Input uServerAddr       - address of this server
    \Input uServerPort       - port of this server
    \Input strServerUniqueID - unique identifier for the server, if ip/port is not sufficient

    \Output
        int32_t              - negative on err, 0 on success

    \Version 04/27/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _SubspaceApiBeginStop(SubspaceApiRefT* pSubspaceApi, uint32_t uServerIp, uint16_t uServerPort, char* strServerUniqueID)
{
    char strUri[128];

    if (pSubspaceApi->eState != SUBSPACE_STATE_IDLE)
    {
        DirtyCastLogPrintf("subspaceapi: _SubspaceApiBeginStop error, can not make a request while there is a pending request.");
        pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
        return(-1);
    }

    ds_snzprintf(pSubspaceApi->aSendBuff, sizeof(pSubspaceApi->aSendBuff), "{ \"server_port\": %d, \"server_ip\": \"%a\", \"external_id\": \"%s\" }",
        uServerPort, uServerIp, strServerUniqueID);
    DirtyCastLogPrintfVerbose(pSubspaceApi->uDebugLevel, 0, "subspaceapi: _SubspaceApiBeginStop:\n%s\n", pSubspaceApi->aSendBuff);

    // format URL
    ds_snzprintf(strUri, sizeof(strUri), "http://%s:%d/%s", pSubspaceApi->strSubspaceSidekickAddr, pSubspaceApi->uSubspaceSidekickPort, SUBSPACEAPI_STOP_URL);

    // make request
    pSubspaceApi->iBytesToSend = (int32_t)strlen(pSubspaceApi->aSendBuff);
    if ((pSubspaceApi->iBytesSent = ProtoHttpPost(pSubspaceApi->pHttp, strUri, pSubspaceApi->aSendBuff, pSubspaceApi->iBytesToSend, FALSE)) >= 0)
    {
        pSubspaceApi->eRequestType = SUBSPACE_REQUEST_TYPE_STOP;
        pSubspaceApi->eState = SUBSPACE_STATE_IN_PROGRESS;
        return(0);
    }
    else
    {
        DirtyCastLogPrintf("subspaceapi: _SubspaceApiBeginStop, ProtoHttpPost() failed %d\n", pSubspaceApi->iBytesSent);
        pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
        return(-2);
    }
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function SubspaceApiCreate

    \Description
        Create SubspaceApi module.

    \Input *pSubspaceSidekickAddr     - address of the sidekick app (probably localhost) 
    \Input uSubspaceSidekickPort      - port of the sidekick app (probably 5005) 

    \Output
        SubspaceApiRefT *             - module state, or NULL if create failed

    \Version 04/24/2020 (cvienneau)
*/
/********************************************************************************F*/
SubspaceApiRefT *SubspaceApiCreate(const char *pSubspaceSidekickAddr, uint16_t uSubspaceSidekickPort)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    SubspaceApiRefT *pSubspaceApi;

    // query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // create module state
    if ((pSubspaceApi = (SubspaceApiRefT *)DirtyMemAlloc(sizeof(*pSubspaceApi), SUBSPACEAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("subspaceapi: could not allocate module state\n");
        return(NULL);
    }
    ds_memclr(pSubspaceApi, sizeof(*pSubspaceApi));
    pSubspaceApi->uDebugLevel = SUBSPACEAPI_DEBUG_LEVEL;
    pSubspaceApi->iMemGroup = iMemGroup;
    pSubspaceApi->pMemGroupUserData = pMemGroupUserData;
    
    // fill in configs
    ds_strnzcpy(pSubspaceApi->strSubspaceSidekickAddr, pSubspaceSidekickAddr, sizeof(pSubspaceApi->strSubspaceSidekickAddr));
    pSubspaceApi->uSubspaceSidekickPort = uSubspaceSidekickPort;

    // create the http ref for making requests to the sidekick app
    if ((pSubspaceApi->pHttp = ProtoHttpCreate(0)) == NULL)
    {
        SubspaceApiDestroy(pSubspaceApi);
        return(NULL);
    }

    // set default debug level
    ProtoHttpControl(pSubspaceApi->pHttp, 'spam', pSubspaceApi->uDebugLevel, 0, NULL);
    
    // format the header data for the protohttp ref as it only needs to be set when it changes
    ProtoHttpControl(pSubspaceApi->pHttp, 'apnd', 0, 0, (void*)"Content-Type: application/json\r\nAccept: application/json\r\n");

    // set timeout
    ProtoHttpControl(pSubspaceApi->pHttp, 'time', SUBSPACEAPI_REQUEST_TIMEOUT, 0, NULL);

    // return module state to caller
    return(pSubspaceApi);
}

/*F********************************************************************************/
/*!
    \Function SubspaceApiUpdate

    \Description
        Update SubspaceApi module

    \Input *SubspaceApiRefT  - module state.

    \Version 04/24/2020 (cvienneau)
*/
/********************************************************************************F*/
void SubspaceApiUpdate(SubspaceApiRefT *pSubspaceApi)
{
    ProtoHttpUpdate(pSubspaceApi->pHttp);

    if (pSubspaceApi->eState == SUBSPACE_STATE_IN_PROGRESS)
    {
        if (pSubspaceApi->iBytesToSend == pSubspaceApi->iBytesSent)
        {
            _SubspaceApiRequestCheck(pSubspaceApi);
        }
        else
        {
            int32_t iResult = ProtoHttpSend(pSubspaceApi->pHttp, pSubspaceApi->aSendBuff + pSubspaceApi->iBytesSent, pSubspaceApi->iBytesToSend - pSubspaceApi->iBytesSent);
            if (iResult >= 0)
            {
                pSubspaceApi->iBytesSent += iResult;
            }
            else
            {
                DirtyCastLogPrintf("subspaceapi: SubspaceApiUpdate, ProtoHttpSend() failed %d\n", iResult);
                pSubspaceApi->eState = SUBSPACE_STATE_ERROR;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function SubspaceApiDestroy

    \Description
        Destroy SubspaceApi module.

    \Input *pSubspaceApi  - module state.

    \Version 04/24/2020 (cvienneau)
*/
/********************************************************************************F*/
void SubspaceApiDestroy(SubspaceApiRefT *pSubspaceApi)
{
    if (pSubspaceApi->pHttp != NULL)
    {
        ProtoHttpDestroy(pSubspaceApi->pHttp);
        pSubspaceApi->pHttp = NULL;
    }

    DirtyMemFree(pSubspaceApi, SUBSPACEAPI_MEMID, pSubspaceApi->iMemGroup, pSubspaceApi->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function SubspaceApiRequest

    \Description
        Start a request to the subspace system, either provisionig or stopping a 
        previous provision.

    \Input *pSubspaceApi     - module state
    \Input eRequestType      - either SUBSPACE_REQUEST_TYPE_PROVISION or SUBSPACE_REQUEST_TYPE_STOP
    \Input uServerAddr       - address of this server
    \Input uServerPort       - port of this server
    \Input strServerUniqueID - unique identifier for the server, if ip/port is not sufficient

    \Output
        int32_t              - negative on err, 0 on success

    \Version 04/27/2020 (cvienneau)
*/
/********************************************************************************F*/
int32_t SubspaceApiRequest(SubspaceApiRefT* pSubspaceApi, SubspaceApiRequestTypeE eRequestType, uint32_t uServerIp, uint16_t uServerPort, char* strServerUniqueID)
{
    if (eRequestType == SUBSPACE_REQUEST_TYPE_PROVISION)
    {
        return(_SubspaceApiBeginProvision(pSubspaceApi, uServerIp, uServerPort, strServerUniqueID));
    }
    else if (eRequestType == SUBSPACE_REQUEST_TYPE_STOP)
    {
        return(_SubspaceApiBeginStop(pSubspaceApi, uServerIp, uServerPort, strServerUniqueID));
    }
    return(-3);
}

/*F********************************************************************************/
/*!
    \Function SubspaceApiResponse

    \Description
        Get status and results of current request.

    \Input *pSubspaceApi     - module state

    \Output
        SubspaceApiResponseT - struct containing request/reponse information

    \Version 07/23/2020 (cvienneau)
*/
/********************************************************************************F*/
SubspaceApiResponseT SubspaceApiResponse(SubspaceApiRefT* pSubspaceApi)
{
    SubspaceApiResponseT Result;

    Result.eRequestType = pSubspaceApi->eRequestType;
    Result.eSubspaceState = pSubspaceApi->eState;

    if (pSubspaceApi->eState == SUBSPACE_STATE_DONE)
    {
        Result.uResultAddr = pSubspaceApi->uSubspaceLocalIp;
        Result.uResultPort = pSubspaceApi->uSubspaceLocalPort;
    }
    else
    {
        Result.uResultAddr = 0;
        Result.uResultPort = 0;
    }

    if ((pSubspaceApi->eState == SUBSPACE_STATE_DONE) || (pSubspaceApi->eState == SUBSPACE_STATE_ERROR))
    {
        pSubspaceApi->eState = SUBSPACE_STATE_IDLE;
        pSubspaceApi->eRequestType = SUBSPACE_REQUEST_TYPE_NONE;
    }

    return(Result);
}

/*F********************************************************************************/
/*!
    \Function SubspaceApiControl

    \Description
        Pass control selectors to SubspaceApi

    \Input *pSubspaceApi    - module state
    \Input iControl         - status selector
    \Input iValue           - control value
    \Input iValue2          - control value
    \Input *pValue          - control value

    \Output
        int32_t             - size of returned data or error code (negative value)

    \Notes
        iControl can be one of the following:

        \verbatim
            'spam' - set the debug level of the module
        \endverbatim

    \Version 04/24/2020 (cvienneau)
*/
/********************************************************************************F*/
int32_t SubspaceApiControl(SubspaceApiRefT *pSubspaceApi, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    // set the debug level of the module
    if (iControl == 'spam')
    {
        pSubspaceApi->uDebugLevel = iValue;
        ProtoHttpControl(pSubspaceApi->pHttp, 'spam', pSubspaceApi->uDebugLevel, 0, NULL);
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function SubspaceApiStatus

    \Description
        SubspaceApi status function. Different selectors control different behaviors.

    \Input *pSubspaceApi    - module state
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
        'rtyp'      returns the request type the api is working on as a SubspaceApiRequestTypeE
        'ssad'      returns the addr provisioned from subspace as uint32_t
        'sspo'      returns the port provisioned from subspace as uint16_t
        'stat'      returns the current status of the state machine as SubspaceApiStateE

    \endverbatim

    \Version 04/29/2020 (cvienneau)
*/
/********************************************************************************F*/
int32_t SubspaceApiStatus(SubspaceApiRefT *pSubspaceApi, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'rtyp')
    {
        return(pSubspaceApi->eRequestType);
    }
    else if (iSelect == 'ssad')
    {
        return(pSubspaceApi->uSubspaceLocalIp);
    }
    else if (iSelect == 'sspo')
    {
        return(pSubspaceApi->uSubspaceLocalPort);
    }
    else if (iSelect == 'stat')
    {
        return(pSubspaceApi->eState);
    }
    return(-1);
}

