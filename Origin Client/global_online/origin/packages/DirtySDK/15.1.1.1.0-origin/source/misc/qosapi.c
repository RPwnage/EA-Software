/*H********************************************************************************/
/*!
    \File qosapi.c

    \Description
        This module implements the client API for the quality of service server and 
        peer-to-peer communication.

    \Copyright
        Copyright (c) 2014 Electronic Arts Inc.

    \Version 1.0 04/07/2008 (cadam) First Version
    \Version 2.0 11/04/2014 (cvienneau) Refactored
*/
/********************************************************************************H*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/util/tagfield.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protoname.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/xml/xmlparse.h"

#include "DirtySDK/misc/qosapi.h"

/*** Defines **********************************************************************/

//! enable verbose output
#define QOSAPI_DEBUG                (DIRTYCODE_DEBUG && FALSE)

//!< qosapi default listen port
#if !DIRTYCODE_XBOXONE
#define QOSAPI_DEFAULT_LISTENPORT   (7673)
#else
#define QOSAPI_DEFAULT_LISTENPORT   (0)
#endif

//!< qosapi number of interfaces to use for a firewall request
#define QOSAPI_NUM_INTERFACES       (2)

//!< qosapi maximum packet length
#define QOSAPI_MAXPACKET            (SOCKET_MAXUDPRECV)

//!< qosapi maximum probes per request
#define QOSAPI_MAXPROBES            (64)

//!< qosapi minimum probes per request
#define QOSAPI_MINPROBES            (1)

//!< qosapi minimum timeout period
#define QOSAPI_MINIMUM_TIMEOUT      (5000 + 2000)  // extra 2sec for https, todo re-evaluate if that additional timeout is really necessary

//!< qosapi default timeout period
#define QOSAPI_DEFAULT_TIMEOUT      (QOSAPI_MINIMUM_TIMEOUT)

//!< qosapi minimum time before re-issuing
#define QOSAPI_MINIMUM_REISSUE_RATE (200)

//!< qosapi time till re-issue ping requests
#define QOSAPI_DEFAULT_REISSUE_RATE (1000)

//!< qosapi timeout for socket idle callback (0 to disable the idle callback)
#define QOSAPI_SOCKET_IDLE_RATE     (0)

//!< qosapi blaze http
#define QOSAPI_HTTP_VERSION         (0x0001)

//!< http receive buffer size
#define QOSAPI_HTTP_BUFFER_SIZE     (4096)

//!< buffer size to format URL strings into
#define QOSAPI_HTTP_MAX_URL_LENGTH  (256)

//!< packet size of incoming latency probes
#define QOSAPI_LATENCY_MIN_RESPONSE_SIZE  (30)

//!< packet size of outgoing latency probes
#define QOSAPI_LATENCY_REQUEST_SIZE  (20)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct QosApiHttpHandlerT                   //!< variables necessary for http communication needed by many request types
{
    char aRecvBuf[QOSAPI_HTTP_BUFFER_SIZE];         //!< buffer to store the http response data in
    char strTargetAddress[128];                     //!< the http address to issue config requests to
    ProtoHttpRefT *pProtoHttp;                      //!< http module, used for service requests to the qos server
    uint8_t bHttpPending;                           //!< is there a http get request waiting for a response?
    uint8_t pad[3];
} QosApiHttpHandlerT;

typedef struct QosApiRequestLatencyT                //!< Used to gather latency metrics, for REQUEST_TYPE_LATENCY
{
    //config
    uint32_t uProbeTotal;                           //!< total number of latency probes to have a round trip
    //status
    uint32_t uProbesSent;                           //!< number of probes the request sent
    uint32_t uProbesRecv;                           //!< number of valid RTT values from probes
    uint32_t aProbeRTTs[QOSAPI_MAXPROBES];          //!< array of probe round trip times
    uint32_t uAddr;                                 //!< address of service responding to request
    struct sockaddr ExternalAddr;                   //!< my address from the prospective of the service responding to the request
} QosApiRequestLatencyT;

typedef struct QosApiRequestBandwidthT              //!< Used to gather bandwidth metrics, for REQUEST_TYPE_BANDWIDTH
{
    //config
    uint32_t uReqId;                                //!< request id the server expects
    int32_t iReqSecret;                             //!< secret that the server expects
    uint32_t uProbeSize;                            //!< service bandwidth probe size
    uint32_t uProbeTotal;                           //!< total number of bandwidth probes to send (lost probes are not re-sent)
    //status
    uint32_t uProbesSent;                           //!< number of bandwidth probes sent so far
    uint32_t uProbesRecv;                           //!< number of bandwidth probes received
    uint32_t uRecvTime;                             //!< time when the first bandwidth probe was received
    uint32_t uUpBitsPerSec;                         //!< bandwidth to the target
    uint32_t uDownBitsPerSec;                       //!< bandwidth from the target
} QosApiRequestBandwidthT;

typedef struct QosApiRequestFirewallT               //!< Used to determine firewall type, for REQUEST_TYPE_FIREWALL
{
    //config
    uint32_t uReqId;                                //!< request id the server expects
    int32_t iReqSecret;                             //!< secret that the server expects
    uint32_t uNumInterfaces;                        //!< number of interfaces returned by the service
    uint32_t aAddrs[QOSAPI_NUM_INTERFACES];         //!< ip addresses of the service
    uint16_t aPorts[QOSAPI_NUM_INTERFACES];         //!< ports of the service
    //status
    uint32_t uProbesSent;                           //!< number of probes the request sent
} QosApiRequestFirewallT;

typedef enum QosApiRequestTypeE
{
    REQUEST_TYPE_LATENCY = 1,                       //!< ping for latency to server
    REQUEST_TYPE_BANDWIDTH = 2,                     //!< measure throughput to server
    REQUEST_TYPE_FIREWALL = 3                       //!< calculate firewall/NAT type with server
} QosApiRequestTypeE;   

typedef enum QosApiRequestStateE
{
    REQUEST_STATE_INIT = 0,                         //!< prepare configuration, usually http request to the server
    REQUEST_STATE_SEND,                             //!< send a series of probes to the target
    REQUEST_STATE_UPDATE,                           //!< wait for probes/response from target
    REQUEST_STATE_COMPLETE,                         //!< succeeded or failed, callback the user and clean up
} QosApiRequestStateE;                                        

//! QoS request
typedef struct QosApiRequestT
{
    struct QosApiRequestT *pNext;                   //!< link to the next record
    QosApiHttpHandlerT *pHttpHandler;               //!< allocated if the request requires http communication 

    uint32_t uTargetProbeAddr;                      //!< addr we want to send the qos probes to
    uint16_t uTargetProbePort;                      //!< port we want to send the qos probes to

    uint32_t uTimeRequested;                        //!< tick the request was made at
    uint32_t uTimeIssued;                           //!< the time the request was issued at
    uint32_t uTimeout;                              //!< timeout for this request in milliseconds

    uint32_t uStatus;                               //!< request status
    uint32_t hResult;                               //!< current error code
    uint32_t uRequestId;                            //!< id incremented client side to distinguish different requests
    
    QosApiRequestStateE eState;                     //!< the current state the request is in
    QosApiRequestTypeE eType;                       //!< the request will perform one of the supported request types
    union                                           //!< based on eType one of the union data will be valid
    {
        QosApiRequestLatencyT   Latency;
        QosApiRequestBandwidthT Bandwidth;
        QosApiRequestFirewallT  Firewall;
    } data;   
} QosApiRequestT;

//! current module state
struct QosApiRefT
{
    // module memory group
    int32_t iMemGroup;                              //!< module mem group id
    void *pMemGroupUserData;                        //!< user data associated with mem group

    QosApiCallbackT *pCallback;                     //!< callback function to call to when an status change occurs
    void *pUserData;                                //!< user specified data to pass to the callback

    SocketT *pSocket;                               //!< socket pointer
    QosApiRequestT *pRequestQueue;                  //!< linked list of requests pending
    QosApiNatTypeE eQosApiNatType;                  //!< current NAT type

    uint32_t uCurrentId;                            //!< current request id, incremented every time a new request is made
    uint16_t uServicePort;                          //!< port the http interface on the service provider is bound to
    int16_t iSpamValue;                             //!< the current logging level
    
    uint32_t uCurrentTimeout;                       //!< current timeout for requests in milliseconds
    uint32_t uCurrentReissueRate;                   //!< time till request probes are re-issued when no response arrives

    NetCritT ThreadCrit;                            //!< critical section, we lock receive thread, update, and public apis to ensure safety with one accessors to the module at a time, since everything access the pRequestQueue
    #if SOCKET_ASYNCRECVTHREAD
    int32_t iDeferredRecv;                          //!< data available for reading, _QosApiUpdate should take care of it
    #endif

    uint16_t uListenPort;                           //!< port to listen on
    uint16_t uCurrentListenPort;                    //!< port we are currently listening on
};

/*** Function Prototypes **********************************************************/

static int32_t _QosApiRecvCB(SocketT *pSocket, int32_t iFlags, void *pData);
void _QosApiUpdateServiceRequestProgress(QosApiRefT *pQosApi, QosApiRequestT *pRequest);

/*** Variables ********************************************************************/
#if DIRTYCODE_LOGGING
const char *_strQosApiRequestState[] =              //!< keep in sync with QosApiRequestStateE
{
    "REQUEST_STATE_INIT    ",                         
    "REQUEST_STATE_SEND    ",                         
    "REQUEST_STATE_UPDATE  ",                       
    "REQUEST_STATE_COMPLETE"                    
};

const char *_strQosApiRequestType[] =              //!< keep in sync with QosApiRequestTypeE
{
    "REQUEST_TYPE_UNKNOWN",                        //!< to fill the 0th slot since REQUEST_TYPE_LATENCY = 1
    "REQUEST_TYPE_LATENCY",
    "REQUEST_TYPE_BANDWIDTH",
    "REQUEST_TYPE_FIREWALL"
};
#endif

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _QosApiGetStatusString

    \Description
        Translates a status into a printable string

    \Input uStatus      - flags indicating past state progress QOSAPI_STATFL_*

    \Output 
        const char *    - static string indicating single status flag

    \Version 09/19/2014 (cvienneau)
*/
/********************************************************************************F*/
#if DIRTYCODE_LOGGING
static const char * _QosApiGetStatusString(uint32_t uStatus)
{
    const char * strResult = "QOSAPI_STATFL_UNKNOWN";

    if (uStatus == QOSAPI_STATFL_NONE)
    {
        strResult = "QOSAPI_STATFL_NONE";
    }
    else if (uStatus == QOSAPI_STATFL_COMPLETE)
    {
        strResult = "QOSAPI_STATFL_COMPLETE";
    }
    else if (uStatus == QOSAPI_STATFL_PART_COMPLETE)
    {
        strResult = "QOSAPI_STATFL_PART_COMPLETE";
    }
    else if (uStatus == QOSAPI_STATFL_DATA_RECEIVED)
    {
        strResult = "QOSAPI_STATFL_DATA_RECEIVED";
    }
    else if (uStatus == QOSAPI_STATFL_DISABLED)
    {
        strResult = "QOSAPI_STATFL_DISABLED";
    }
    else if (uStatus == QOSAPI_STATFL_CONTACTED)
    {
        strResult = "QOSAPI_STATFL_CONTACTED";
    }
    else if (uStatus == QOSAPI_STATFL_TIMEDOUT)
    {
        strResult = "QOSAPI_STATFL_TIMEDOUT";
    }
    else if (uStatus == QOSAPI_STATFL_FAILED)
    {
        strResult = "QOSAPI_STATFL_FAILED";
    }
    return(strResult);
}
#endif

/*F********************************************************************************/
/*!
    \Function _QosApiStateTransition

    \Description
        Update the requests state, status and current error code.

    \Input *pRequest    - pointer to the request
    \Input eState       - new state to transition to, REQUEST_STATE_*
    \Input uStatus      - flags indicating past state progress QOSAPI_STATFL_*
    \Input uModule      - module producing the hResult code, DIRTYAPI_*
    \Input iCode        - status/error code to use in hResult

    \Version 09/19/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiStateTransition(QosApiRequestT *pRequest, QosApiRequestStateE eState, uint32_t uStatus, uint32_t uModule, int32_t iCode)
{
    pRequest->hResult = DirtyErrGetHResult(uModule, iCode, uStatus != QOSAPI_STATFL_COMPLETE);
    pRequest->uStatus |= uStatus;
    
    NetPrintf(("qosapi: request[%02d] transitioning from %s-> %s (hResult: 0x%x status: %s)\n", pRequest->uRequestId, _strQosApiRequestState[pRequest->eState], _strQosApiRequestState[eState], pRequest->hResult, _QosApiGetStatusString(uStatus)));
    pRequest->eState = eState;
}

/*F********************************************************************************/
/*!
    \Function _QosApiSocketOpen

    \Description
        Open QosApi socket

    \Input *pQosApi     - pointer to module state
    \Input uListenPort  - port to bind socket to

    \Output 
        QosApiRequestT * - pointer to the new request

    \Version 09/13/2013 (jbrookes)
*/
/********************************************************************************F*/
static SocketT *_QosApiSocketOpen(QosApiRefT *pQosApi, uint32_t uListenPort)
{
    struct sockaddr LocalAddr;
    int32_t iResult;
    SocketT *pSocket;

    // create the socket
    if ((pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0)) == NULL)
    {
        NetPrintf(("qosapi: could not allocate socket\n"));
        return(NULL);
    }

    // make sure socket receive buffer is large enough to queue up
    // multiple probe responses (worst case: 10 probes * 1200 bytes)
    SocketControl(pSocket, 'rbuf', 16*1024, NULL, NULL);
    SocketControl(pSocket, 'sbuf', 16*1024, NULL, NULL);

    SockaddrInit(&LocalAddr, AF_INET);
    SockaddrInSetPort(&LocalAddr, uListenPort);

    // bind the socket
    if ((iResult = SocketBind(pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
    {
        NetPrintf(("qosapi: error %d binding socket to port %d, trying random\n", iResult, uListenPort));
        SockaddrInSetPort(&LocalAddr, 0);
        if ((iResult = SocketBind(pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
        {
            NetPrintf(("qosapi: error %d binding socket to listen\n", iResult));
            SocketClose(pSocket);
            return(NULL);
        }
    }

    // set the current listen port
    SocketInfo(pSocket, 'bind', 0, &LocalAddr, sizeof(LocalAddr));
    pQosApi->uCurrentListenPort = SockaddrInGetPort(&LocalAddr);
    NetPrintf(("qosapi: bound socket to port %u\n", pQosApi->uCurrentListenPort));

    // set the callback
    SocketCallback(pSocket, CALLB_RECV, QOSAPI_SOCKET_IDLE_RATE, pQosApi, &_QosApiRecvCB);
    // return to caller
    return(pSocket);
}

/*F********************************************************************************/
/*!
    \Function _QosApiFreeRequest

    \Description
        Free any memory associated with the request.

    \Input *pQosApi   - pointer to module state
    \Input *pRequest  - pointer to request object

    \Version 10/27/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiFreeRequest(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    if (pRequest != NULL)
    {
        if (pRequest->pHttpHandler != NULL)
        {
            if (pRequest->pHttpHandler->pProtoHttp != NULL)
            {
                ProtoHttpDestroy(pRequest->pHttpHandler->pProtoHttp);
            }
            DirtyMemFree(pRequest->pHttpHandler, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
        }
        DirtyMemFree(pRequest, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiDestroyRequest

    \Description
        Remove request from queue and free any memory associated with the request.

    \Input *pQosApi   - pointer to module state
    \Input uRequestId - id of the request to destroy

    \Output 
        int32_t - 0 if successful, negative otherwise

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static int32_t _QosApiDestroyRequest(QosApiRefT *pQosApi, uint32_t uRequestId)
{
    QosApiRequestT *pQosApiRequest, **ppQosApiRequest;

    // find request in queue
    for (ppQosApiRequest = &pQosApi->pRequestQueue; *ppQosApiRequest != NULL; ppQosApiRequest = &(*ppQosApiRequest)->pNext)
    {
        // found the request to destroy?
        if ((*ppQosApiRequest)->uRequestId == uRequestId)
        {
            // set the request
            pQosApiRequest = *ppQosApiRequest;

            // dequeue
            *ppQosApiRequest = (*ppQosApiRequest)->pNext;

            // free memory
            _QosApiFreeRequest(pQosApi, pQosApiRequest);

            return(0);
        }
    }

    // specified request not found
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _QosApiCreateRequest

    \Description
        Create and allocate the required space for a request.

    \Input *pQosApi     - pointer to module state
    \Input eRequestType - request type

    \Output 
        QosApiRequestT * - pointer to the new request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static QosApiRequestT *_QosApiCreateRequest(QosApiRefT *pQosApi, QosApiRequestTypeE eRequestType)
{
    QosApiRequestT *pQosApiRequest, **ppQosApiRequest;
    uint8_t bUseHttp = TRUE;

#ifdef DIRTYCODE_XBOXONE
    if(eRequestType == REQUEST_TYPE_FIREWALL)
    {
        bUseHttp = FALSE;
    }
#endif 

    // alloc top level request
    if ((pQosApiRequest = DirtyMemAlloc(sizeof(*pQosApiRequest), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapi: error allocating request.\n"));
        return(NULL);
    }
    memset(pQosApiRequest, 0, sizeof(*pQosApiRequest));
    pQosApiRequest->hResult = DirtyErrGetHResult(DIRTYAPI_QOS, QOSAPI_STATUS_INIT, TRUE);

    if (bUseHttp)
    {
        // allocate the http handler (wrapping protohttp)
        if ((pQosApiRequest->pHttpHandler = DirtyMemAlloc(sizeof(*pQosApiRequest->pHttpHandler), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("qosapi: error allocating http handler.\n"));
            _QosApiFreeRequest(pQosApi, pQosApiRequest);
            return(NULL);
        }
        memset(pQosApiRequest->pHttpHandler, 0, sizeof(*pQosApiRequest->pHttpHandler));

        // allocate the protohttp module
        DirtyMemGroupEnter(pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
        if ((pQosApiRequest->pHttpHandler->pProtoHttp = ProtoHttpCreate(QOSAPI_HTTP_BUFFER_SIZE)) == NULL)
        {
            DirtyMemGroupLeave();
            NetPrintf(("qosapi: could not create the protohttp module.\n"));
            _QosApiFreeRequest(pQosApi, pQosApiRequest);
            return(NULL);

        }
        DirtyMemGroupLeave();

        // set to the current spam mode
        ProtoHttpControl(pQosApiRequest->pHttpHandler->pProtoHttp, 'spam', pQosApi->iSpamValue, 0, NULL);

        // set to a keep-alive connection
        ProtoHttpControl(pQosApiRequest->pHttpHandler->pProtoHttp, 'keep', 1, 0, NULL);
    }

    // set the request type
    pQosApiRequest->eType = eRequestType;

    // find end of queue and append
    for (ppQosApiRequest = &pQosApi->pRequestQueue; *ppQosApiRequest != NULL; ppQosApiRequest = &(*ppQosApiRequest)->pNext)
    {
    }
    *ppQosApiRequest = pQosApiRequest;

    if (pQosApi->uCurrentId == 65536)
    {
        pQosApi->uCurrentId = 2;
    }

    // set the request id
    pQosApiRequest->uRequestId = pQosApi->uCurrentId;

    // increment the current request id
    pQosApi->uCurrentId++;

    NetPrintf(("qosapi: request[%02d] created, %s.\n", pQosApiRequest->uRequestId, _strQosApiRequestType[pQosApiRequest->eType]));
    
#ifdef DIRTYCODE_XBOXONE
    //on xbox one we can get the NAT type from the OS so the request is already complete
    if(pQosApiRequest->eType == REQUEST_TYPE_FIREWALL)
    {
        _QosApiStateTransition(pQosApiRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_SUCCESS);
    }
#endif

    // return ref
    return(pQosApiRequest);
}

/*F********************************************************************************/
/*!
    \Function _QosApiGetRequest

    \Description
        Get a request by request ID.

    \Input *pQosApi         - pointer to module state
    \Input uRequestId       - id of the request to get

    \Output
        QosApiRequestT *    -   pointer to request or NULL if not found

    \Version 08/16/2011 (jbrookes)
*/
/********************************************************************************F*/
static QosApiRequestT *_QosApiGetRequest(QosApiRefT *pQosApi, uint32_t uRequestId)
{
    QosApiRequestT *pRequest;

    // find request in queue
    for (pRequest = pQosApi->pRequestQueue; pRequest != NULL; pRequest = pRequest->pNext)
    {
        // found the request?
        if (pRequest->uRequestId == uRequestId)
        {
            return(pRequest);
        }
    }
    // did not find the request
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _QosApiParseHttpLatencyInitResponse

    \Description
        Parses http configuration data for a latency request.

    \Input *pQosApi         - pointer to module state
    \Input *pRequest        - request who's owns this http response

    \Output
        int32_t             - negative on error

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosApiParseHttpLatencyInitResponse(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    const char *pXml, *pXml2;

    // response will look something like:
    //<?xml version="1.0" encoding="UTF-8"?>
    //<qos>
    //    <numprobes>0</numprobes>
    //    <qosip>3236792599</qosip> //new
    //    <qosport>17499</qosport>
    //    <probesize>0</probesize>
    //    <requestid>1</requestid>
    //    <reqsecret>0</reqsecret>
    //</qos>

    if ((pXml = XmlFind(pRequest->pHttpHandler->aRecvBuf, "qos")) != NULL)
    {
        //basic config used by both Latency and Bandwidth
        if ((pXml2 = XmlFind(pXml, ".qosport")) != NULL)
        {
            pRequest->uTargetProbePort = XmlContentGetInteger(pXml2, 0);
        }
        if ((pXml2 = XmlFind(pXml, ".qosip")) != NULL)
        {
            pRequest->uTargetProbeAddr = XmlContentGetInteger(pXml2, 0);
        }
        // validate basic config needed for both latency and BW
        if ((pRequest->uTargetProbeAddr == 0) || (pRequest->uTargetProbePort == 0))
        {
            NetPrintf(("qosapi: request[%02d] _QosApiParseHttpLatencyInitResponse config parse error required data missing, addr=%a, port=%d\n", pRequest->uRequestId, pRequest->uTargetProbeAddr, pRequest->uTargetProbePort));
            return(-3);
        }
        _QosApiStateTransition(pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        return(0);
    }

    NetPrintf(("qosapi: request[%02d] _QosApiParseHttpLatencyInitResponse parse error, unknown message type.\n", pRequest->uRequestId));
    return(-2);
}

/*F********************************************************************************/
/*!
    \Function _QosApiParseHttpBandwidthInitResponse

    \Description
        Parses http configuration data for a bandwidth request.

    \Input *pQosApi         - pointer to module state
    \Input *pRequest        - request who's owns this http response

    \Output
        int32_t             - negative on error

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosApiParseHttpBandwidthInitResponse(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    const char *pXml, *pXml2;

    // response will look something like this:
    //<?xml version="1.0" encoding="UTF-8"?>
    //<qos>
    //    <numprobes>10</numprobes>
    //    <qosip>3236792599</qosip> //new
    //    <qosport>17499</qosport>
    //    <probesize>1200</probesize>
    //    <requestid>2</requestid>
    //    <reqsecret>1505</reqsecret>
    //</qos>

    // response from the server telling us bandwidth test configuration info
    if ((pXml = XmlFind(pRequest->pHttpHandler->aRecvBuf, "qos")) != NULL)
    {
        //basic config used by both Latency and Bandwidth
        if ((pXml2 = XmlFind(pXml, ".qosport")) != NULL)
        {
            pRequest->uTargetProbePort = XmlContentGetInteger(pXml2, 0);
        }
        if ((pXml2 = XmlFind(pXml, ".qosip")) != NULL)
        {
            pRequest->uTargetProbeAddr = XmlContentGetInteger(pXml2, 0);
        }
        // validate basic config needed for both latency and BW
        if ((pRequest->uTargetProbeAddr == 0) || (pRequest->uTargetProbePort == 0))
        {
            NetPrintf(("qosapi: request[%02d] _QosApiParseHttpBandwidthInitResponse config parse error required data missing, addr=%a, port=%d\n", pRequest->uRequestId, pRequest->uTargetProbeAddr, pRequest->uTargetProbePort));
            return(-3);
        }

        // Bandwidth specific config
        if (pRequest->eType == REQUEST_TYPE_BANDWIDTH)
        {
            if ((pXml2 = XmlFind(pXml, ".numprobes")) != NULL)
            {
                pRequest->data.Bandwidth.uProbeTotal = XmlContentGetInteger(pXml2, 0);
            }
            if ((pXml2 = XmlFind(pXml, ".probesize")) != NULL)
            {
                pRequest->data.Bandwidth.uProbeSize = XmlContentGetInteger(pXml2, 0);
            }        
            if ((pXml2 = XmlFind(pXml, ".requestid")) != NULL)
            {
                pRequest->data.Bandwidth.uReqId = XmlContentGetInteger(pXml2, 0);
            }
            if ((pXml2 = XmlFind(pXml, ".reqsecret")) != NULL)
            {
                pRequest->data.Bandwidth.iReqSecret = XmlContentGetInteger(pXml2, 0);
            }

        }

        // validate specific BW config
        if ((pRequest->eType == REQUEST_TYPE_BANDWIDTH) && 
            ( (pRequest->data.Bandwidth.uReqId == 0) || (pRequest->data.Bandwidth.uProbeTotal < 2) || (pRequest->data.Bandwidth.uProbeSize == 0) ))
        {
            NetPrintf(("qosapi: request[%02d] _QosApiParseHttpBandwidthInitResponse config parse error required data missing, requestId=%d, probecount=%d, probesize=%d\n", pRequest->uRequestId, pRequest->data.Bandwidth.uReqId, pRequest->data.Bandwidth.uProbeTotal, pRequest->data.Bandwidth.uProbeSize));
            return(-4);
        }
        _QosApiStateTransition(pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        return(0);
    }

    NetPrintf(("qosapi: request[%02d] _QosApiParseHttpBandwidthInitResponse parse error, unknown message type.\n", pRequest->uRequestId));
    return(-2);
}

/*F********************************************************************************/
/*!
    \Function _QosApiParseHttpFirewallInitResponse

    \Description
        Parses http configuration data for a firewall request.

    \Input *pQosApi         - pointer to module state
    \Input *pRequest        - request who's owns this http response

    \Output
        int32_t             - negative on error

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosApiParseHttpFirewallInitResponse(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    const char *pXml, *pXml2;

    // response will look something like this:
    //<?xml version="1.0" encoding="UTF-8"?>
    //<firewall>
    //    <ips>
    //        <ips>3236792599</ips>
    //        <ips>3236792601</ips>
    //    </ips>
    //    <numinterfaces>2</numinterfaces>
    //    <ports>
    //        <ports>17500</ports>
    //        <ports>17501</ports>
    //    </ports>
    //    <requestid>2</requestid>
    //    <reqsecret>1222</reqsecret>
    //</firewall>

    if ((pXml = XmlFind(pRequest->pHttpHandler->aRecvBuf, "firewall")) != NULL)
    {
        uint32_t uInterface;
        const char *pXml3;

        if ((pXml2 = XmlFind(pXml, ".numinterfaces")) != NULL)
        {
            pRequest->data.Firewall.uNumInterfaces = XmlContentGetInteger(pXml2, 0);
        }

        if (((pXml2 = XmlFind(pXml, ".ips.ips")) != NULL) && ((pXml3 = XmlFind(pXml, ".ports.ports")) != NULL))
        {
            // store the first interface information
            pRequest->uTargetProbeAddr = pRequest->data.Firewall.aAddrs[0] = XmlContentGetInteger(pXml2, 0);
            pRequest->uTargetProbePort = pRequest->data.Firewall.aPorts[0] = (uint16_t)XmlContentGetInteger(pXml3, 0);

            // get the rest of the interfaces required
            for (uInterface = 1; uInterface < pRequest->data.Firewall.uNumInterfaces; uInterface++)
            {
                pXml2 = XmlNext(pXml2);
                pXml3 = XmlNext(pXml3);

                // if we get null early then error
                if ((pXml2 == NULL) || (pXml3 == NULL))
                {
                    NetPrintf(("qosapi: request[%02d] _QosApiParseHttpFirewallInitResponse config parse error, not enough ip/port information to match numinterfaces.\n", pRequest->uRequestId));
                    return(-3);
                }

                pRequest->data.Firewall.aAddrs[uInterface] = XmlContentGetInteger(pXml2, 0);
                pRequest->data.Firewall.aPorts[uInterface] = (uint16_t)XmlContentGetInteger(pXml3, 0);
            }
        }
        else
        {
            NetPrintf(("qosapi: request[%02d] _QosApiParseHttpFirewallInitResponse config parse error, no ip/port information.\n", pRequest->uRequestId));
            return(-4);
        }
        if ((pXml2 = XmlFind(pXml, ".requestid")) != NULL)
        {
            pRequest->data.Firewall.uReqId = XmlContentGetInteger(pXml2, 0);
        }
        if ((pXml2 = XmlFind(pXml, ".reqsecret")) != NULL)
        {
            pRequest->data.Firewall.iReqSecret = XmlContentGetInteger(pXml2, 0);
        }

        if (pRequest->data.Firewall.uReqId == 0)
        {
            NetPrintf(("qosapi: request[%02d] _QosApiParseHttpFirewallInitResponse config parse error, requestid unset.\n", pRequest->uRequestId));
            return(-5);
        }
        _QosApiStateTransition(pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        return(0);
    }
    NetPrintf(("qosapi: request[%02d] _QosApiParseHttpFirewallInitResponse parse error, unknown message type.\n", pRequest->uRequestId));
    return(-2);
}

/*F********************************************************************************/
/*!
    \Function _QosApiParseHttpFirewallUpdateResponse

    \Description
        Parses http firewall request final result

    \Input *pQosApi         - pointer to module state
    \Input *pRequest        - request who's owns this http response

    \Output
        int32_t             - negative on error

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosApiParseHttpFirewallUpdateResponse(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    const char *pXml, *pXml2;

    // response will look something like:
    //<?xml version="1.0" encoding="UTF-8"?>
    //<firetype>
    //    <firetype>4</firetype>
    //</firetype>

    if ((pXml = XmlFind(pRequest->pHttpHandler->aRecvBuf, "firetype")) != NULL)
    {
        if ((pXml2 = XmlFind(pXml, ".firetype")) != NULL)
        {
            pQosApi->eQosApiNatType = XmlContentGetInteger(pXml2, 0);
            _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_SUCCESS);
            return(0);
        }
        else
        {
            NetPrintf(("qosapi: request[%02d] _QosApiParseHttpFirewallUpdateResponse parse error, firetype not set.\n", pRequest->uRequestId));
            return(-3);
        }
    }
    NetPrintf(("qosapi: request[%02d] _QosApiParseHttpFirewallUpdateResponse parse error, unknown message type.\n", pRequest->uRequestId));
    return(-2);
}

/*F********************************************************************************/
/*!
    \Function _QosApiParseHttpResponse

    \Description
        Parses the blaze http responses.

    \Input *pQosApi  - pointer to module state
    \Input *pRequest - pointer to the request

    \Output 
        int32_t      - negative on error, 0 on success

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static int32_t _QosApiParseHttpResponse(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t iRet = -1;

    NetPrintf(("qosapi: request[%02d] <- http response\n%s\n", pRequest->uRequestId, pRequest->pHttpHandler->aRecvBuf));

    if (pRequest->eState == REQUEST_STATE_INIT)
    {
        if (pRequest->eType == REQUEST_TYPE_LATENCY)
        {
            iRet = _QosApiParseHttpLatencyInitResponse(pQosApi, pRequest);
        }
        else if (pRequest->eType == REQUEST_TYPE_BANDWIDTH)
        {
            iRet = _QosApiParseHttpBandwidthInitResponse(pQosApi, pRequest);
        }
        else if (pRequest->eType == REQUEST_TYPE_FIREWALL)
        {
            iRet = _QosApiParseHttpFirewallInitResponse(pQosApi, pRequest);
        }
    }
    else if (pRequest->eState == REQUEST_STATE_UPDATE)
    {
        if (pRequest->eType == REQUEST_TYPE_FIREWALL)
        {
            iRet = _QosApiParseHttpFirewallUpdateResponse(pQosApi, pRequest);
        }
    }
    if (iRet < 0)
    {
        NetPrintf(("qosapi: request[%02d] error unable to handle http response\n", pRequest->uRequestId));
    }
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function _QosApiSendLatencyProbes

    \Description
        Sends a latency probe out to the request's target.

    \Input *pQosApi  - pointer to module state
    \Input *pRequest - pointer to the request

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiSendLatencyProbes(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t aPacketData[QOSAPI_LATENCY_REQUEST_SIZE];
    struct sockaddr SendAddr;
    uint32_t uSendCounter;
    int32_t iResult = 0;
    uint32_t uProbeCount = pRequest->data.Latency.uProbeTotal - pRequest->data.Latency.uProbesRecv;  //send a new probe for every response we haven't received

    // an outgoing latency probe looks like this:
    // int32_t requestID            // increments client side, starting at 2
    // int32_t serviceRequestID     // 1 for client->server latency, 0 for client->client latency
    // int32_t serviceSecret        // always 0 for latency
    // int32_t flags                // always 0 for latency
    // int32_t NetTick()            // time value used to calculate round trip

    // setup the target address
    SockaddrInit(&SendAddr, AF_INET);
    SockaddrInSetAddr(&SendAddr, pRequest->uTargetProbeAddr);
    SockaddrInSetPort(&SendAddr, pRequest->uTargetProbePort);
    
    // send latency probes
    for (uSendCounter = 0; uSendCounter < uProbeCount; uSendCounter++)
    {
        memset(aPacketData, 0, sizeof(aPacketData));
        aPacketData[0] = SocketHtonl(pRequest->uRequestId);
        aPacketData[1] = SocketHtonl(pRequest->eType);
        //aPacketData[2] = 0
        //aPacketData[3] = 0
        aPacketData[4] = SocketHtonl(NetTick());

        iResult = SocketSendto(pQosApi->pSocket, (const char *)aPacketData, QOSAPI_LATENCY_REQUEST_SIZE, 0, &SendAddr, sizeof(SendAddr));
        NetPrintfVerbose((pQosApi->iSpamValue, 1, "qosapi: request[%02d] sent latency probe %d to %A (result=%d)\n", pRequest->uRequestId, pRequest->data.Latency.uProbesSent, &SendAddr, iResult));

        if (iResult == QOSAPI_LATENCY_REQUEST_SIZE)
        {
            pRequest->data.Latency.uProbesSent++;
        }
        else
        {
            break;          //some failure deal with it below
        }
    }

    if (iResult == QOSAPI_LATENCY_REQUEST_SIZE)    //check to see that we were successful in our last send
    {
        //the last Latency probe has been sent, wait to receive response probes
        NetPrintf(("qosapi: request[%02d] sent %d probes to address %A\n", pRequest->uRequestId, uProbeCount, &SendAddr));
        _QosApiStateTransition(pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        pRequest->uTimeIssued = NetTick();
    }
    else if (iResult == 0)  // need to try again
    {
        NetPrintf(("qosapi: request[%02d] needs to try again to send %d more probes to %A\n", pRequest->uRequestId, pRequest->data.Latency.uProbeTotal - pRequest->data.Latency.uProbesSent, &SendAddr));
        // transition anyways, let the re-issue logic handle the missing probes
        _QosApiStateTransition(pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        pRequest->uTimeIssued = NetTick();
    }    
    else if (iResult < 0)   // an error
    {
        NetPrintf(("qosapi: request[%02d] failed to send probe %d to address %A (err=%d)\n", pRequest->uRequestId, pRequest->data.Latency.uProbesSent, &SendAddr, iResult));
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult);
    }
    else                    // unexpected result
    {
        NetPrintf(("qosapi: request[%02d] unexpected result for probe %d to address %A (err=%d)\n", pRequest->uRequestId, pRequest->data.Latency.uProbesSent, &SendAddr, iResult));
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiSendBandwidthProbes

    \Description
        Sends a series of bandwidth probe out to the request's target.

    \Input *pQosApi   - pointer to module state
    \Input *pRequest  - pointer to the request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiSendBandwidthProbes(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t aPacketData[QOSAPI_MAXPACKET/sizeof(int32_t)];
    struct sockaddr SendAddr;
    uint32_t uSendCounter;
    int32_t iResult = 0;
    uint32_t uProbeCount = pRequest->data.Bandwidth.uProbeTotal - pRequest->data.Bandwidth.uProbesSent;  //send the rest/all of the probes

    // an outgoing bandwidth probe looks like this:
    // int32_t requestID            // increments client side, starting at 2
    // int32_t serviceRequestID     // id from server
    // int32_t serviceSecret        // secret from server
    // int32_t uProbeSent           // count index of this probe
    // int32_t uProbeTotal          // total number of probes that will be sent
    // char[]  data                 // data to fill up the packet size to pRequest->data.Bandwidth.uProbeSize (zeroed out)

    // setup the target address
    SockaddrInit(&SendAddr, AF_INET);
    SockaddrInSetAddr(&SendAddr, pRequest->uTargetProbeAddr);
    SockaddrInSetPort(&SendAddr, pRequest->uTargetProbePort);

    // send bandwidth probes
    for (uSendCounter = 0; uSendCounter < uProbeCount; uSendCounter++)
    {
        memset(aPacketData, 0, sizeof(aPacketData));
        aPacketData[0] = SocketHtonl(pRequest->uRequestId);
        aPacketData[1] = SocketHtonl(pRequest->data.Bandwidth.uReqId);
        aPacketData[2] = SocketHtonl(pRequest->data.Bandwidth.iReqSecret);
        aPacketData[3] = SocketHtonl(pRequest->data.Bandwidth.uProbesSent);
        aPacketData[4] = SocketHtonl(pRequest->data.Bandwidth.uProbeTotal);

        iResult = SocketSendto(pQosApi->pSocket, (const char *)aPacketData, pRequest->data.Bandwidth.uProbeSize, 0, &SendAddr, sizeof(SendAddr));
        NetPrintfVerbose((pQosApi->iSpamValue, 1, "qosapi: request[%02d] sent bandwidth probe %d to %A (result=%d)\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbesSent, &SendAddr, iResult));

        if (iResult == (int32_t)pRequest->data.Bandwidth.uProbeSize)
        {
            pRequest->data.Bandwidth.uProbesSent++;
        }
        else
        {
            break;          //some failure deal with it below
        }
    }

    if (iResult == (int32_t)pRequest->data.Bandwidth.uProbeSize)   //check to see that we were successful in our last send
    {
        //the last BW probe has been sent, wait to receive response probes
        NetPrintf(("qosapi: request[%02d] sent %d probes to address %A\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbeTotal, &SendAddr));
        _QosApiStateTransition(pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        pRequest->uTimeIssued = NetTick();
    }
    else if (iResult == 0)  // need to try again
    {
        NetPrintf(("qosapi: request[%02d] needs to try again to send %d more probes to %A\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbeTotal - pRequest->data.Bandwidth.uProbesSent, &SendAddr));
        // if we just stay in the current state we will try again
    }    
    else if (iResult < 0)   // an error
    {
        NetPrintf(("qosapi: request[%02d] failed to send probe %d to address %A (err=%d)\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbesSent, &SendAddr, iResult));
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult);
    }
    else                    // unexpected result
    {
        NetPrintf(("qosapi: request[%02d] unexpected result for probe %d to address %A (err=%d)\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbesSent, &SendAddr, iResult));
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiSendFirewallProbeSet

    \Description
        Sends two firewall probes to the service addresses.

    \Input *pQosApi   - pointer to module state
    \Input *pRequest  - request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiSendFirewallProbeSet(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t aPacketData[2];
    struct sockaddr SendAddr;
    uint32_t uInterface;
    int32_t iResult = 0;

    // an outgoing firewall probe looks like this (there are no incoming firewall probes):
    // int32_t serviceRequestID     // id from server
    // int32_t serviceSecret        // secret from server

    // init the socket address
    SockaddrInit(&SendAddr, AF_INET);

    // set the payload
    aPacketData[0] = SocketHtonl(pRequest->data.Firewall.uReqId);
    aPacketData[1] = SocketHtonl(pRequest->data.Firewall.iReqSecret);

    // send probes to each of the interfaces
    for (uInterface = 0; uInterface < pRequest->data.Firewall.uNumInterfaces; uInterface++)
    {
        // send probe
        SockaddrInSetAddr(&SendAddr, pRequest->data.Firewall.aAddrs[uInterface]);
        SockaddrInSetPort(&SendAddr, pRequest->data.Firewall.aPorts[uInterface]);

        iResult = SocketSendto(pQosApi->pSocket, (const char *)aPacketData, sizeof(aPacketData), 0, &SendAddr, sizeof(SendAddr));
        NetPrintf(("qosapi: request[%02d] sent fire wall probe %d to %A (result=%d)\n", pRequest->uRequestId, pRequest->data.Firewall.uProbesSent, &SendAddr, iResult));

        if (iResult == sizeof(aPacketData))
        {
            pRequest->data.Firewall.uProbesSent++;
        }
        else
        {
            break;          //some failure deal with it below
        }
    }

    if (iResult == sizeof(aPacketData))
    {
        _QosApiStateTransition(pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        pRequest->uTimeIssued = NetTick();
    }
    else if (iResult == 0)  // need to try again
    {
        NetPrintf(("qosapi: request[%02d] needs to try again to send more probes to %A\n", pRequest->uRequestId, &SendAddr));
        // transition anyways, let the re-issue logic handle the missing probes
        _QosApiStateTransition(pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        pRequest->uTimeIssued = NetTick();
    }    
    else if (iResult < 0)   // an error
    {
        NetPrintf(("qosapi: request[%02d] failed to send probe to address %A (err=%d)\n", pRequest->uRequestId, &SendAddr, iResult));
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult);
    }
    else                    // unexpected result
    {
        NetPrintf(("qosapi: request[%02d] unexpected result for probe %d to address %A (err=%d)\n", pRequest->uRequestId, &SendAddr, iResult));
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiDefaultCallback

    \Description
        Default QosApi user callback.

    \Input *pQosApi   - pointer to module state
    \Input *pCBInfo   - callback info
    \Input eCBType    - callback type
    \Input *pUserData - callback user data

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiDefaultCallback(QosApiRefT *pQosApi, QosApiCBInfoT *pCBInfo, QosApiCBTypeE eCBType, void *pUserData)
{
}

/*F********************************************************************************/
/*!
    \Function _QosApiValidateLatencyProbe

    \Description
        Check the latency probe to be sure we want to continue processing it.
    
    \Input *pQosApi     - module state
    \Input *pRequest    - probe resulting from this request
    \Input *pPacketData - received packet data
    \Input iPacketLen   - packet length

    \Output 
        uint8_t     - true if valid

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t _QosApiValidateLatencyProbe(QosApiRefT *pQosApi, QosApiRequestT *pRequest, char* pPacketData, int32_t iPacketLen)
{
    //todo perhaps parse the whole latency packet into a struct to be used in the later functions

    // an incoming latency probe looks like this
    // uint32_t requestID           // (same as outgoing) increments client side, starting at 2 
    // uint32_t serviceRequestID    // (same as outgoing) 1 for client->server latency, 0 for client->client latency 
    // int32_t serviceSecret        // (same as outgoing) always 0 for latency 
    // uint32_t flags               // (same as outgoing) always 0 for latency
    // int32_t NetTick()            // (same as outgoing) time value used to calculate round trip
    // int32_t addr                 // your external address
    // int16_t addr                 // your external address port
    // int32_t iDataLength          // currently server never returns data so 0, but a client->client qos could come back with some data
    // char[] data                  // up to QOSAPI_RESPONSE_MAXSIZE (256) bytes of data

    uint32_t uServiceReqId = SocketNtohl(*(int32_t *)&pPacketData[4]);
    
    if (iPacketLen < QOSAPI_LATENCY_MIN_RESPONSE_SIZE)
    {
        NetPrintf(("qosapi: request[%02d] received latency response packet is not the expected length=%d", pRequest->uRequestId, iPacketLen));
        return(FALSE);
    }

    if ((uint32_t)pRequest->eType != uServiceReqId)
    {
        NetPrintf(("qosapi: request[%02d] received latency response packet does not contain the expected request type.", pRequest->uRequestId, pRequest->eType));
        return(FALSE);
    }

    if (pRequest->data.Latency.uProbesRecv >= QOSAPI_MAXPROBES)
    {
        NetPrintf(("qosapi: request[%02d] ignoring probe, received QOSAPI_MAXPROBES already.\n", pRequest->uRequestId));
        return(FALSE);
    }

    NetPrintfVerbose((pQosApi->iSpamValue, 1, "qosapi: request[%02d], received probe %d\n", pRequest->uRequestId, pRequest->data.Latency.uProbesRecv));

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _QosApiProcessLatencyAddress

    \Description
        Read address information from the probe

    \Input *pRequest    - probe resulting from this request
    \Input *pPacketData - received packet data
    \Input *pRecvAddr   - source of packet data

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiProcessLatencyAddress(QosApiRequestT *pRequest, char *pPacketData, struct sockaddr *pRecvAddr)
{
    // set the receive address
    pRequest->data.Latency.uAddr = SockaddrInGetAddr(pRecvAddr);

    // set up external address
    SockaddrInit(&pRequest->data.Latency.ExternalAddr, AF_INET);
    SockaddrInSetAddr(&pRequest->data.Latency.ExternalAddr, SocketNtohl(*(int32_t *)&pPacketData[20]));
    SockaddrInSetPort(&pRequest->data.Latency.ExternalAddr, SocketNtohs(*(int16_t *)&pPacketData[24]));
}

/*F********************************************************************************/
/*!
    \Function _QosApiProcessLatencyRTT

    \Description
        Use probe information to calculate the RTT

    \Input *pRequest    - probe resulting from this request
    \Input *pPacketData - received packet data
    \Input uWhenReceived- time the packet was received

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiProcessLatencyRTT(QosApiRequestT *pRequest, char *pPacketData, uint32_t uWhenReceived)
{
    uint32_t uPacketRTT, uSentTick;
    int32_t iPosition;

    // calculate rtt
    uSentTick = SocketNtohl(*(int32_t *)&pPacketData[16]);
    uPacketRTT = uWhenReceived - uSentTick;

    // insert the RTT in the correct location in the list (insertion sort since the list is small)
    for (iPosition = pRequest->data.Latency.uProbesRecv++; (iPosition > 0) && (pRequest->data.Latency.aProbeRTTs[iPosition-1] > uPacketRTT); iPosition -= 1)
    {
        pRequest->data.Latency.aProbeRTTs[iPosition] = pRequest->data.Latency.aProbeRTTs[iPosition-1];
    }
    pRequest->data.Latency.aProbeRTTs[iPosition] = uPacketRTT;
}

/*F********************************************************************************/
/*!
    \Function _QosApiProcessLatencyStateTransition

    \Description
        Move to next steps of the qos process if necessary

    \Input *pRequest    - probe resulting from this request

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiProcessLatencyStateTransition(QosApiRequestT *pRequest)
{
    //determine next state transitions
    if (!(pRequest->uStatus & QOSAPI_STATFL_CONTACTED)) //first probe QOSAPI_STATFL_CONTACTED
    {
        _QosApiStateTransition(pRequest, pRequest->eState, QOSAPI_STATFL_CONTACTED, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
    }
    else if ((pRequest->data.Latency.uProbesRecv >= (pRequest->data.Latency.uProbeTotal / 2))  // half the probes QOSAPI_STATFL_PART_COMPLETE
    && !(pRequest->uStatus & QOSAPI_STATFL_PART_COMPLETE))
    {
        _QosApiStateTransition(pRequest, pRequest->eState, QOSAPI_STATFL_PART_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
    }
    else if ((pRequest->data.Latency.uProbesRecv >= pRequest->data.Latency.uProbeTotal))   // all the probes QOSAPI_STATFL_COMPLETE
    {
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_SUCCESS);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiValidateBandwidthProbe

    \Description
        Check the bandwidth probe to be sure we want to continue processing it.

    \Input *pQosApi     - module state
    \Input *pRequest    - probe resulting from this request
    \Input *pPacketData - received packet data
    \Input iPacketLen   - packet length
    
    \Output 
        uint8_t         - true if valid

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static uint8_t _QosApiValidateBandwidthProbe(QosApiRefT *pQosApi, QosApiRequestT *pRequest, char* pPacketData, int32_t iPacketLen)
{
    //todo perhaps parse the whole bandwidth packet into a struct to be used in the later functions

    // an incoming bandwidth probe looks like this (exactly the same as the outgoing):
    // int32_t requestID            // increments client side, starting at 2
    // int32_t serviceRequestID     // id from server
    // int32_t serviceSecret        // secret from server
    // int32_t uProbeSent           // count index of this probe
    // int32_t uProbeTotal          // total number of probes that will be sent
    // char[]  data                 // data to fill up the packet size to pRequest->data.Bandwidth.uProbeSize (zeroed out)

    uint32_t uServiceReqId = SocketNtohl(*(int32_t *)&pPacketData[4]);
    int32_t iServiceReqSecret = SocketNtohl(*(int32_t *)&pPacketData[8]);

    if ((int32_t)pRequest->data.Bandwidth.uProbeSize != iPacketLen)
    {
        NetPrintf(("qosapi: received bandwidth response packet is not the expected length=%d\n", iPacketLen));
        return(FALSE);
    }

    if (pRequest->data.Bandwidth.uReqId != uServiceReqId)
    {
        NetPrintf(("qosapi: received bandwidth response packet does not contain the expected request id %d, instead contained %d.\n", pRequest->data.Bandwidth.uReqId, uServiceReqId));
        return(FALSE);
    }

    if (pRequest->data.Bandwidth.iReqSecret != iServiceReqSecret)
    {
        NetPrintf(("qosapi: received bandwidth response packet does not contain the expected secret %d, instead contained %d.\n", pRequest->data.Bandwidth.iReqSecret, iServiceReqSecret));
        return(FALSE);
    }

    NetPrintfVerbose((pQosApi->iSpamValue, 1, "qosapi: request[%02d], received probe %d\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbesRecv));

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _QosApiProcessBandwidthStateTransition

    \Description
        Move to next steps of the qos process if necessary

    \Input *pRequest    - probe resulting from this request
    \Input *pPacketData - pointer to packet bytes

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiProcessBandwidthStateTransition(QosApiRequestT *pRequest, char *pPacketData)
{
    uint32_t uProbeSequenceNumber;

    //note its possible to receive bandwidth probes while we're in the send or update state
    uProbeSequenceNumber = SocketNtohl(*(int32_t *)&pPacketData[12]);
    if (!(pRequest->uStatus & QOSAPI_STATFL_CONTACTED)) //first probe QOSAPI_STATFL_CONTACTED
    {
        _QosApiStateTransition(pRequest, pRequest->eState, QOSAPI_STATFL_CONTACTED, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
    }
    if ((uProbeSequenceNumber >= (pRequest->data.Bandwidth.uProbeTotal / 2))  // half the probes QOSAPI_STATFL_PART_COMPLETE
    && !(pRequest->uStatus & QOSAPI_STATFL_PART_COMPLETE))
    {
        _QosApiStateTransition(pRequest, pRequest->eState, QOSAPI_STATFL_PART_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
    }
    // finish if we received the final bandwidth probe (or we spent too much time waiting, and timed out)
    else if (uProbeSequenceNumber >= pRequest->data.Bandwidth.uProbeTotal)
    {
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_SUCCESS);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiProcessBandwidthProbe

    \Description
        Calculate bandwidth

    \Input *pRequest        - probe resulting from this request
    \Input *pPacketData     - received packet data
    \Input uWhenReceived    - tick the packet was received.

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiProcessBandwidthProbe(QosApiRequestT *pRequest, char *pPacketData, uint32_t uWhenReceived)
{
    //get up bandwidth from server, the server actually puts it in every packet but we only need to do this once, the server only sends responses after it has received all the probes or times out
    if (pRequest->data.Bandwidth.uProbesRecv++ == 0)
    {
        pRequest->data.Bandwidth.uRecvTime = uWhenReceived;
        pRequest->data.Bandwidth.uUpBitsPerSec = SocketNtohl(*(int32_t *)&pPacketData[20]);
    }
    //calculate down bandwidth
    else
    {
        //calculate the total time of the bw phase so far
        uint32_t ticks = (uWhenReceived - pRequest->data.Bandwidth.uRecvTime);
        
        //calculate total data so far as data per tick, (bits per byte) * (ms per sec) * (bytes per probe) * (number of probes so far)
        pRequest->data.Bandwidth.uDownBitsPerSec = (8 * 1000 * pRequest->data.Bandwidth.uProbeSize * pRequest->data.Bandwidth.uProbesRecv); 
        
        //apply time to the amount of data
        if (ticks != 0)
        {            
            pRequest->data.Bandwidth.uDownBitsPerSec /= ticks;
        }
        
        //todo the first packet is excluded from the bw calculation, is that ok?
        //todo there is no bw penalty for lost packets, if the packet isn't received we don't calculate bw
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiRecvResponse

    \Description
        Process a receive response.

    \Input *pQosApi     - module state
    \Input *pPacketData - received packet data
    \Input iPacketLen   - packet length
    \Input *pRecvAddr   - source of packet data
    \Input uRequestId   - response request id

    \Version 08/15/2011 (jbrookes) re-factored from _QosApiRecv()
*/
/********************************************************************************F*/
static void _QosApiRecvResponse(QosApiRefT *pQosApi, char *pPacketData, int32_t iPacketLen, struct sockaddr *pRecvAddr, uint32_t uRequestId)
{
    QosApiRequestT *pRequest;
    uint32_t uSockTick;

    // get request matching id
    if ((pRequest = _QosApiGetRequest(pQosApi, uRequestId)) == NULL)
    {
        NetPrintf(("qosapi: request[%02d] could not be found, ignoring.\n", uRequestId));
        return;
    }

    // make sure it's not failed or timed out
    if (pRequest->eState == REQUEST_STATE_COMPLETE)
    {
        NetPrintf(("qosapi: request[%02d] response received but ignored, since the request is already completed.\n", pRequest->uRequestId));
        return;
    }

    // get recv time stamp from sockaddr
    uSockTick = SockaddrInGetMisc(pRecvAddr);
    
    if (pRequest->eType == REQUEST_TYPE_LATENCY)
    {
        if (_QosApiValidateLatencyProbe(pQosApi, pRequest, pPacketData, iPacketLen))
        {
            //determine the external address and address the probe came back from
            _QosApiProcessLatencyAddress(pRequest, pPacketData, pRecvAddr);

            //calculate rtt
           _QosApiProcessLatencyRTT(pRequest, pPacketData, uSockTick);

           //now that we've processed the full probe, determine if the state needs to change
           _QosApiProcessLatencyStateTransition(pRequest);
        }

    }
    else if (pRequest->eType == REQUEST_TYPE_BANDWIDTH)
    {
        if (_QosApiValidateBandwidthProbe(pQosApi, pRequest, pPacketData, iPacketLen))
        {
            _QosApiProcessBandwidthProbe(pRequest, pPacketData, uSockTick);

            _QosApiProcessBandwidthStateTransition(pRequest, pPacketData);
        }
    }
    else
    {
        NetPrintf(("qosapi: request[%02d] error received response for unhandled request type %d.\n", pRequest->uRequestId, pRequest->eType));
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiRecv

    \Description
        Process QosApi socket data.

    \Input *pSocket - pointer to module socket
    \Input *pData   - pointer to module state

    \Output 
        int32_t - zero

    \Version 08/08/2011 (szhu)
*/
/********************************************************************************F*/
static int32_t _QosApiRecv(SocketT *pSocket, void *pData)
{
    QosApiRefT *pQosApi = (QosApiRefT *)pData;
    struct sockaddr RecvAddr;
    char aPacketData[QOSAPI_MAXPACKET];
    int32_t iAddrLen = sizeof(struct sockaddr), iRecvLen;

    NetCritEnter(&pQosApi->ThreadCrit);

    while ((iRecvLen = SocketRecvfrom(pSocket, aPacketData, sizeof(aPacketData), 0, &RecvAddr, &iAddrLen)) > 0)
    {
        uint32_t uRequestId;

        //be sure the packet has enough data for the request id
        if (iRecvLen < (int32_t)sizeof(uint32_t))   
        {
            NetPrintf(("qosapi: received packet is too short to process; received len=%d\n", iRecvLen));
            continue;
        }

        uRequestId = SocketNtohl(*(int32_t *)&aPacketData);

        // check for invalid qos packet
        if (uRequestId == 0)
        {
            NetPrintf(("qosapi: invalid qos packet received; ignoring\n"));
            continue;
        }

        // process packet
        if (iRecvLen != QOSAPI_LATENCY_REQUEST_SIZE)
        {
            _QosApiRecvResponse(pQosApi, aPacketData, iRecvLen, &RecvAddr, uRequestId);
        }
    }
    NetCritLeave(&pQosApi->ThreadCrit);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _QosApiRecvCB

    \Description
        QosApi socket callback.

    \Input *pSocket - pointer to module socket
    \Input iFlags   - ignored
    \Input *pData   - pointer to module state

    \Output 
        int32_t     - zero

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static int32_t _QosApiRecvCB(SocketT *pSocket, int32_t iFlags, void *pData)
{
    #if SOCKET_ASYNCRECVTHREAD
    QosApiRefT *pQosApi = (QosApiRefT *)pData;
    // make sure we own resources
    if (NetCritTry(&pQosApi->ThreadCrit))
    {
        _QosApiRecv(pSocket, pData);
        // release resources
        NetCritLeave(&pQosApi->ThreadCrit);
    }
    else
    {
        pQosApi->iDeferredRecv = 1;
    }
    #endif
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _QosApiHttpGet

    \Description
        Setup and start httpget operation, will automatically transition to fail
        state in an error.

    \Input *pQosApi     - module state
    \Input *pRequest    - request to initiate
    \Input *strUrl      - url to get

    \Output 
        int32_t     - negative on error

    \Version 09/19/2014 (cvienneau) refactored from _QosApiUpdate()
*/
/********************************************************************************F*/
static int32_t _QosApiHttpGet(QosApiRefT *pQosApi, QosApiRequestT *pRequest, char * strUrl)
{
    int32_t iResult;

    NetPrintf(("qosapi: request[%02d] sending http request %s.\n", pRequest->uRequestId, strUrl));

    // issue the request
    if ((iResult = ProtoHttpGet(pRequest->pHttpHandler->pProtoHttp, strUrl, FALSE)) >= 0)
    {
        pRequest->pHttpHandler->bHttpPending = TRUE;
    }
    else
    {
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_PROTO_HTTP, iResult);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _QosApiUpdateUdpProbes

    \Description
        Receive any incoming probes

    \Input *pQosApi     - module state

    \Version 09/19/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiUpdateUdpProbes(QosApiRefT *pQosApi)
{
        /*
    check if we need to poll for data. we need to do this if:
        1. this platform does not implement an async recv thread
        2. there's a deferred recv call issued by async recv thread
    */
    if (pQosApi->pSocket != NULL)
    {
        #if SOCKET_ASYNCRECVTHREAD
        if (pQosApi->iDeferredRecv)
        {
            /*
            reset the flag before we start to process the data,
            this could be safer since it's not protected by any mutex
            */
            pQosApi->iDeferredRecv = 0;
            _QosApiRecv(pQosApi->pSocket, pQosApi);
        }
        #else
        _QosApiRecv(pQosApi->pSocket, pQosApi);
        #endif
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiUpdateHttp

    \Description
        Receive any incoming http responses

    \Input *pQosApi     - module state
    \Input *pRequest    - request to check for pending http actions

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiUpdateHttp(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t iResult;

    if (pRequest->pHttpHandler && pRequest->pHttpHandler->bHttpPending)
    {
        ProtoHttpUpdate(pRequest->pHttpHandler->pProtoHttp);

        if ((iResult = ProtoHttpRecvAll(pRequest->pHttpHandler->pProtoHttp, (char *)pRequest->pHttpHandler->aRecvBuf, sizeof(pRequest->pHttpHandler->aRecvBuf))) >= 0)
        {
            int32_t iHttpStatusCode = ProtoHttpStatus(pRequest->pHttpHandler->pProtoHttp, 'code', NULL, 0);
            pRequest->pHttpHandler->bHttpPending = FALSE;

            if (iHttpStatusCode ==  200)
            {
                // parse the http response
                if (_QosApiParseHttpResponse(pQosApi, pRequest) < 0)
                {
                    _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_QOS, QOSAPI_STATUS_HTTP_PARSE_RESPONSE_FAILED);
                }
            }
            else
            {
                _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_PROTO_HTTP, iHttpStatusCode);
            }
        }
        else if ((iResult < 0) && (iResult != PROTOHTTP_RECVWAIT))
        {
            int32_t hResult = ProtoHttpStatus(pRequest->pHttpHandler->pProtoHttp, 'hres', NULL, 0);
            if (hResult >= 0)  //in this case there was no socket or ssl error
            {
                _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_PROTO_HTTP, iResult); 
            }
            else
            {
                // normally use _QosApiUpdateRequestStatus but since we already have the hResult we do it manually here, report the socket/ssl hRestult
                pRequest->uStatus |= QOSAPI_STATFL_FAILED;
                pRequest->hResult = hResult;
            }
            pRequest->pHttpHandler->bHttpPending = FALSE;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiLatencyRequestInit

    \Description
        Assemble and send a http latency init request.

    \Input *pQosApi     - module state
    \Input *pRequest    - request generating the message

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiLatencyRequestInit(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    // query the server for latency test configuration, once we have a response we will transition into the send state
    if (pRequest->pHttpHandler->bHttpPending == FALSE)
    {
        char strUrl[QOSAPI_HTTP_MAX_URL_LENGTH];

        // request will look something like:
        // https://qos-prod-bio-dub-common-common.gos.ea.com:17504/qos/qos?vers=1&qtyp=1&prpt=3659

        ds_snzprintf(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "https://%s:%u/qos/qos", pRequest->pHttpHandler->strTargetAddress, pQosApi->uServicePort);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "?vers=", QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&qtyp=", pRequest->eType);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&prpt=", pRequest->uTargetProbePort);

        _QosApiHttpGet(pQosApi, pRequest, strUrl);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiLatencyRequestUpdate

    \Description
        Check to see if any latency probes should be resent, since insufficient 
        response was received.

    \Input *pQosApi     - module state
    \Input *pRequest    - request being processes

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiLatencyRequestUpdate(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    if ((pRequest->data.Latency.uProbesRecv < pRequest->data.Latency.uProbeTotal) &&      //we haven't received all responses and
        (NetTickDiff(NetTick(), pRequest->uTimeIssued) > (int32_t)pQosApi->uCurrentReissueRate) //too much time passed
        )
    {
        _QosApiStateTransition(pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        NetPrintf(("qosapi: request[%02d] re-issued; sending %d new probes.\n", pRequest->uRequestId, (pRequest->data.Latency.uProbeTotal - pRequest->data.Latency.uProbesRecv)));
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiGetLatencyQosInfo

    \Description
        Copy relevant fields from request into a QosInfoT for public api use.

    \Input *pRequest    - source request
    \Input *pQosInfo    - target QosInfoT struct

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiGetLatencyQosInfo(QosApiRequestT *pRequest, QosInfoT *pQosInfo)
{
    ds_memcpy(&pQosInfo->ExternalAddr, &pRequest->data.Latency.ExternalAddr, sizeof(struct sockaddr));
    pQosInfo->hResult = pRequest->hResult;
    pQosInfo->iMedRTT = pRequest->data.Latency.aProbeRTTs[pRequest->data.Latency.uProbesRecv/2];
    pQosInfo->iMinRTT = pRequest->data.Latency.aProbeRTTs[0];
    pQosInfo->iProbesRecv = pRequest->data.Latency.uProbesRecv;
    pQosInfo->iProbesSent = pRequest->data.Latency.uProbesSent;
    pQosInfo->uAddr = pRequest->data.Latency.uAddr;
    pQosInfo->uFlags = pRequest->uStatus;
    pQosInfo->uRequestId = pRequest->uRequestId;
    pQosInfo->uWhenRequested = pRequest->uTimeRequested;
}

/*F********************************************************************************/
/*!
    \Function _QosApiGetBandwidthQosInfo

    \Description
        Copy relevant fields from request into a QosInfoT for public api use.

    \Input *pRequest    - source request
    \Input *pQosInfo    - target QosInfoT struct

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiGetBandwidthQosInfo(QosApiRequestT *pRequest, QosInfoT *pQosInfo)
{
    pQosInfo->hResult = pRequest->hResult;
    pQosInfo->iProbesRecv = pRequest->data.Bandwidth.uProbesRecv;
    pQosInfo->iProbesSent = pRequest->data.Bandwidth.uProbesSent;
    pQosInfo->uDwnBitsPerSec = pRequest->data.Bandwidth.uDownBitsPerSec;
    pQosInfo->uFlags = pRequest->uStatus;
    pQosInfo->uRequestId = pRequest->uRequestId;
    pQosInfo->uUpBitsPerSec = pRequest->data.Bandwidth.uUpBitsPerSec;
    pQosInfo->uWhenRequested = pRequest->uTimeRequested;
}

/*F********************************************************************************/
/*!
    \Function _QosApiGetFirewallQosInfo

    \Description
        Copy relevant fields from request into a QosInfoT for public api use.

    \Input *pRequest    - source request
    \Input *pQosInfo    - target QosInfoT struct

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiGetFirewallQosInfo(QosApiRequestT *pRequest, QosInfoT *pQosInfo)
{
    pQosInfo->hResult = pRequest->hResult;
    pQosInfo->iProbesSent = pRequest->data.Firewall.uProbesSent;
    pQosInfo->uFlags = pRequest->uStatus;
    pQosInfo->uRequestId = pRequest->uRequestId;
    pQosInfo->uWhenRequested = pRequest->uTimeRequested;
}

/*F********************************************************************************/
/*!
    \Function _QosApiGetQosInfo

    \Description
        Copy relevant fields from request into a QosInfoT for public api use.

    \Input *pRequest    - source request
    \Input *pQosInfo    - target QosInfoT struct

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiGetQosInfo(QosApiRequestT *pRequest, QosInfoT *pQosInfo)
{
    memset(pQosInfo, 0, sizeof(QosInfoT));
    
    if (pRequest->eType == REQUEST_TYPE_LATENCY)
    {
        _QosApiGetLatencyQosInfo(pRequest, pQosInfo);
    }
    else if (pRequest->eType == REQUEST_TYPE_BANDWIDTH)
    {
        _QosApiGetBandwidthQosInfo(pRequest, pQosInfo);
    }
    else if (pRequest->eType == REQUEST_TYPE_FIREWALL)
    {
        _QosApiGetFirewallQosInfo(pRequest, pQosInfo);
    }
    else
    {
        NetPrintf(("qosapi: request[%02d] error, unable to _QosApiGetQosInfo for unknown type.\n", pRequest->uRequestId));
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiLatencyRequestComplete

    \Description
        Finish the request; call back the user, and delete the request.

    \Input *pQosApi     - module ref
    \Input *pRequest    - request being completed

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiLatencyRequestComplete(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    QosInfoT QosInfo;
    QosApiCBInfoT CBInfo;
    QosApiCBTypeE CBType = QOSAPI_CBTYPE_LATENCY;

    memset(&CBInfo, 0, sizeof(QosApiCBInfoT));
    _QosApiGetQosInfo(pRequest, &QosInfo);

    CBInfo.uOldStatus = 0;  //todo is old status useful?
    CBInfo.uNewStatus = pRequest->uStatus;
    CBInfo.pQosInfo = &QosInfo;

    NetPrintf(("qosapi: request[%02d] latency test completed...\n uWhenRequested: %u\n uAddr: %a\n ExternalAddr: %a:%d\n iProbesSent: %d\n iProbesRecv: %d\n iMinRTT: %d\n iMedRTT: %d\n uFlags: %d\n hResult: 0x%x\n", 
        CBInfo.pQosInfo->uRequestId, 
        CBInfo.pQosInfo->uWhenRequested,
        CBInfo.pQosInfo->uAddr,
        SockaddrInGetAddr(&CBInfo.pQosInfo->ExternalAddr),
        SockaddrInGetPort(&CBInfo.pQosInfo->ExternalAddr),
        CBInfo.pQosInfo->iProbesSent,
        CBInfo.pQosInfo->iProbesRecv,
        CBInfo.pQosInfo->iMinRTT,
        CBInfo.pQosInfo->iMedRTT,
        CBInfo.pQosInfo->uFlags,
        CBInfo.pQosInfo->hResult));

    pQosApi->pCallback(pQosApi, &CBInfo, CBType, pQosApi->pUserData);
    _QosApiDestroyRequest(pQosApi, pRequest->uRequestId);
}

/*F********************************************************************************/
/*!
    \Function _QosApiUpdateLatencyRequest

    \Description
        Process the request for any given state.

    \Input *pQosApi     - module ref
    \Input *pRequest    - request being completed

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static uint8_t _QosApiUpdateLatencyRequest(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    if (pRequest->eState == REQUEST_STATE_INIT)
    {
        _QosApiLatencyRequestInit(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_SEND)
    {
        _QosApiSendLatencyProbes(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_UPDATE)
    {
        _QosApiLatencyRequestUpdate(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_COMPLETE)
    {
        _QosApiLatencyRequestComplete(pQosApi, pRequest);
        return(TRUE);
    }
    else
    {
        NetPrintf(("qosapi: request[%02d] error request in unexpected state.\n", pRequest->uRequestId));
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _QosApiBandwidthRequestInit

    \Description
        Assemble and send a http bandwidth init request.

    \Input *pQosApi     - module state
    \Input *pRequest    - request generating the message

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiBandwidthRequestInit(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    // query the server for bandwidth test configuration, once we have a response we will transition into the send state
    if (pRequest->pHttpHandler->bHttpPending == FALSE)
    {
        char strUrl[QOSAPI_HTTP_MAX_URL_LENGTH];

        // request will look something like:
        // https://qos-prod-rs-ord-common-common.gos.ea.com:17504/qos/qos?vers=1&qtyp=2&prpt=3659

        ds_snzprintf(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "https://%s:%u/qos/qos", pRequest->pHttpHandler->strTargetAddress, pQosApi->uServicePort);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "?vers=", QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&qtyp=", pRequest->eType);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&prpt=", pRequest->uTargetProbePort);

        _QosApiHttpGet(pQosApi, pRequest, strUrl);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiBandwidthRequestComplete

    \Description
        Finish the request; call back the user, and delete the request.

    \Input *pQosApi     - module ref
    \Input *pRequest    - request being completed

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiBandwidthRequestComplete(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    QosInfoT QosInfo;
    QosApiCBInfoT CBInfo;
    QosApiCBTypeE CBType = QOSAPI_CBTYPE_BANDWIDTH;

    memset(&CBInfo, 0, sizeof(QosApiCBInfoT));
    _QosApiGetQosInfo(pRequest, &QosInfo);

    CBInfo.uOldStatus = 0;  //todo is old status useful?
    CBInfo.uNewStatus = pRequest->uStatus;
    CBInfo.pQosInfo = &QosInfo;

    NetPrintf(("qosapi: request[%02d] bandwidth test completed...\n uWhenRequested: %u\n iProbesSent: %d\n iProbesRecv: %d\n uUpBitsPerSec: %d\n uDwnBitsPerSec: %d\n uFlags: %d\n hResult: 0x%x\n", 
        CBInfo.pQosInfo->uRequestId, 
        CBInfo.pQosInfo->uWhenRequested,
        CBInfo.pQosInfo->iProbesSent,
        CBInfo.pQosInfo->iProbesRecv,
        CBInfo.pQosInfo->uUpBitsPerSec,
        CBInfo.pQosInfo->uDwnBitsPerSec,
        CBInfo.pQosInfo->uFlags,
        CBInfo.pQosInfo->hResult));

    pQosApi->pCallback(pQosApi, &CBInfo, CBType, pQosApi->pUserData);
    _QosApiDestroyRequest(pQosApi, pRequest->uRequestId);
}

/*F********************************************************************************/
/*!
    \Function _QosApiUpdateBandwidthRequest

    \Description
        Process the request for any given state.

    \Input *pQosApi     - module ref
    \Input *pRequest    - request being completed
    
    \Output 
        int8_t          - TRUE if the request has been destroyed.

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static uint8_t _QosApiUpdateBandwidthRequest(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    if (pRequest->eState == REQUEST_STATE_INIT)
    {
        _QosApiBandwidthRequestInit(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_SEND)
    {
        _QosApiSendBandwidthProbes(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_UPDATE)
    {
        //bandwidth has nothing todo while updating (though like all requests it might time out)
    }
    else if (pRequest->eState == REQUEST_STATE_COMPLETE)
    {
        _QosApiBandwidthRequestComplete(pQosApi, pRequest);
        return(TRUE);
    }
    else
    {
        NetPrintf(("qosapi: request[%02d] error request in unexpected state.\n", pRequest->uRequestId));
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _QosApiFirewallRequestInit

    \Description
        Assemble and send a http firewall init request.

    \Input *pQosApi     - module state
    \Input *pRequest    - request generating the message

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiFirewallRequestInit(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    // query the server for latency test configuration, once we have a response we will transition into the send state
    if (pRequest->pHttpHandler->bHttpPending == FALSE)
    {
        char strUrl[QOSAPI_HTTP_MAX_URL_LENGTH];
        
        // request will look something like this:
        // https://qos-prod-rs-ord-common-common.gos.ea.com:17504/qos/firewall?vers=1&nint=2

        ds_snzprintf(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "https://%s:%u/qos/firewall", pRequest->pHttpHandler->strTargetAddress, pQosApi->uServicePort);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "?vers=", QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&nint=", QOSAPI_NUM_INTERFACES);

        _QosApiHttpGet(pQosApi, pRequest, strUrl);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiFirewallRequestTypeQuery

    \Description
        Assemble and send a http firewall type request, udp probes should be in flight.

    \Input *pQosApi     - module state
    \Input *pRequest    - request generating the message

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiFirewallRequestTypeQuery(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    // query the server for firewall type, once we have a response we will transition into the completed state
    if (pRequest->pHttpHandler->bHttpPending == FALSE)
    {
        char strUrl[QOSAPI_HTTP_MAX_URL_LENGTH];
        uint32_t uLocalIp = NetConnStatus('addr', 0, NULL, 0);

        ds_snzprintf(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "https://%s:%u/qos/firetype", pRequest->pHttpHandler->strTargetAddress, pQosApi->uServicePort);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "?vers=", QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&rqid=", pRequest->data.Firewall.uReqId);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&rqsc=", pRequest->data.Firewall.iReqSecret);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&inip=", uLocalIp);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&inpt=", pQosApi->uCurrentListenPort);

        _QosApiHttpGet(pQosApi, pRequest, strUrl);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiFirewallRequestUpdate

    \Description
        Check to see if any firewall probes should be resent, since insufficient 
        response was received.

    \Input *pQosApi     - module state
    \Input *pRequest    - request being processes

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiFirewallRequestUpdate(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    //handling for firewall probes
    if (NetTickDiff(NetTick(), pRequest->uTimeIssued) > (int32_t)pQosApi->uCurrentReissueRate)
    {
        _QosApiStateTransition(pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING);
        NetPrintf(("qosapi: request[%02d] re-issued; sending %d new probes.\n", pRequest->uRequestId, QOSAPI_NUM_INTERFACES));
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiFirewallRequestComplete

    \Description
        Finish the request; call back the user, and delete the request.

    \Input *pQosApi     - module ref
    \Input *pRequest    - request being completed

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiFirewallRequestComplete(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    QosInfoT QosInfo;
    QosApiCBInfoT CBInfo;
    QosApiCBTypeE CBType = QOSAPI_CBTYPE_NAT;

    memset(&CBInfo, 0, sizeof(QosApiCBInfoT));
    _QosApiGetQosInfo(pRequest, &QosInfo);

    CBInfo.uOldStatus = 0;  //todo, is old status useful?
    CBInfo.uNewStatus = pRequest->uStatus;
    CBInfo.pQosInfo = &QosInfo;
    CBInfo.eQosApiNatType = QosApiGetNatType(pQosApi);

    NetPrintf(("qosapi: request[%02d] firewall test completed...\n uWhenRequested: %u\n iProbesSent: %d\n eQosApiNatType: %d\n uFlags: %d\n hResult: 0x%x\n", 
        CBInfo.pQosInfo->uRequestId, 
        CBInfo.pQosInfo->uWhenRequested,
        CBInfo.pQosInfo->iProbesSent,
        CBInfo.eQosApiNatType,
        CBInfo.pQosInfo->uFlags,
        CBInfo.pQosInfo->hResult));

    pQosApi->pCallback(pQosApi, &CBInfo, CBType, pQosApi->pUserData);
    _QosApiDestroyRequest(pQosApi, pRequest->uRequestId);
}

/*F********************************************************************************/
/*!
    \Function _QosApiUpdateFirewallRequest

    \Description
        Process the request for any given state.

    \Input *pQosApi     - module ref
    \Input *pRequest    - request being completed
    
    \Output 
        int8_t          - TRUE if the request has been destroyed.

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static uint8_t _QosApiUpdateFirewallRequest(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    if (pRequest->eState == REQUEST_STATE_INIT)
    {
        _QosApiFirewallRequestInit(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_SEND)
    {
        _QosApiFirewallRequestTypeQuery(pQosApi, pRequest); //this http request will stay outstanding until the server response with our firewall type
        _QosApiSendFirewallProbeSet(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_UPDATE)
    {
        _QosApiFirewallRequestUpdate(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_COMPLETE)
    {
        _QosApiFirewallRequestComplete(pQosApi, pRequest);
        return(TRUE);
    }
    else
    {
        NetPrintf(("qosapi: request[%02d] error request in unexpected state.\n", pRequest->uRequestId));
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _QosApiCheckForTimeout

    \Description
        Check to see if the given request has timed out, if so transition to complete state.

    \Input *pQosApi     - module ref
    \Input *pRequest    - request being completed

    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static void _QosApiCheckForTimeout(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    if (NetTickDiff(NetTick(), pRequest->uTimeRequested) > (int32_t)pRequest->uTimeout)
    {
        NetPrintf(("qosapi: request[%02d] timed out (%d ms)\n", pRequest->uRequestId, NetTickDiff(NetTick(), pRequest->uTimeRequested)));
        _QosApiStateTransition(pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_TIMEDOUT, DIRTYAPI_QOS, QOSAPI_STATUS_OPERATION_TIMEOUT);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiUpdateRequest

    \Description
        Process the request for any given state.

    \Input *pQosApi     - module ref
    \Input *pRequest    - request being completed
    
    \Output 
        int8_t          - TRUE if the request has been destroyed.
        
    \Version 11/07/2014 (cvienneau) 
*/
/********************************************************************************F*/
static uint8_t _QosApiUpdateRequest(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    uint8_t bRequestCompleted = FALSE;
    if (pRequest->eType == REQUEST_TYPE_LATENCY)
    {
        bRequestCompleted = _QosApiUpdateLatencyRequest(pQosApi, pRequest);
    }
    else if (pRequest->eType == REQUEST_TYPE_BANDWIDTH)
    {
        bRequestCompleted = _QosApiUpdateBandwidthRequest(pQosApi, pRequest);
    }
    else if (pRequest->eType == REQUEST_TYPE_FIREWALL)
    {
        bRequestCompleted = _QosApiUpdateFirewallRequest(pQosApi, pRequest);
    }
    else
    {
        NetPrintf(("qosapi: request[%02d] error unexpected request type.\n", pRequest->uRequestId));
    }

    if (bRequestCompleted == FALSE)
    {
        // see if this request has taken more than its allotted time
        _QosApiCheckForTimeout(pQosApi, pRequest);
    }
    else
    {
        return(TRUE);
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _QosApiUpdate

    \Description
        Default QosApi update function.

    \Input *pData - point to module state
    \Input uTick  - current millisecond tick

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiUpdate(void *pData, uint32_t uTick)
{
    QosApiRefT *pQosApi = (QosApiRefT *)pData;
    QosApiRequestT *pRequest;

    NetCritEnter(&pQosApi->ThreadCrit);

    // receive data for any incoming probes
    _QosApiUpdateUdpProbes(pQosApi);    //maybe we can choose to only do this during the update state of latency/BW

    for (pRequest = pQosApi->pRequestQueue; pRequest != NULL; pRequest = pRequest->pNext)
    {
        // if there is any pending http action process it now
        _QosApiUpdateHttp(pQosApi, pRequest);

        // update based off recent communication udp or http
        if(_QosApiUpdateRequest(pQosApi, pRequest) == TRUE) 
        {
            break;//on TRUE a request has been destroyed the list is no longer valid
        }
    }

    NetCritLeave(&pQosApi->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function _QosApiRequestCommon

    \Description
        Build the portion of the request that is common between client and other types.

    \Input *pQosApi     - module state
    \Input eType        - request type
    \Input uNumProbes   - number of probes to send out for the request

    \Output 
        QosApiRequestT* - pointer to request ref

    \Version 10/29/2014 (cvienneau)
*/
/********************************************************************************F*/
static QosApiRequestT *_QosApiRequestCommon(QosApiRefT *pQosApi, QosApiRequestTypeE eType, uint32_t uNumProbes)
{
    QosApiRequestT *pRequest;

    // set the port to the default value if 0
    if (pQosApi->uListenPort == 0)
    {
        pQosApi->uListenPort = QOSAPI_DEFAULT_LISTENPORT;
    }

    // if the module's socket hasn't yet been allocated
    if ((pQosApi->pSocket == NULL) && ((pQosApi->pSocket = _QosApiSocketOpen(pQosApi, pQosApi->uListenPort)) == NULL))
    {
        NetPrintf(("qosapi: error opening listening socket.\n"));
        return(NULL);
    }

    // allocate the request
    if ((pRequest = _QosApiCreateRequest(pQosApi, eType)) == 0)
    {
        NetPrintf(("qosapi: unable to create the request\n"));
        return(NULL);
    }

    // configure the request
    if (eType == REQUEST_TYPE_LATENCY)
    {
        // set the number of probes to the minimum/maximum if required  
        if (uNumProbes < QOSAPI_MINPROBES)
        {
            uNumProbes = QOSAPI_MINPROBES;
        }
        if (uNumProbes > QOSAPI_MAXPROBES)
        {
            uNumProbes = QOSAPI_MAXPROBES;
        }

        // save the number of probes to send
        pRequest->data.Latency.uProbeTotal = uNumProbes;
    }

    // set the request timeout
    pRequest->uTimeout = pQosApi->uCurrentTimeout;

    // set the request time
    pRequest->uTimeRequested = NetTick();

    return(pRequest);
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function QosApiCreate

    \Description
        Create the QosApi module.

    \Input *pCallback   - callback function to use when a status change is detected
    \Input *pUserData   - use data to be set for the callback
    \Input iServicePort - set the blaze qos service http port

    \Output 
        QosApiRefT * - state pointer

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
QosApiRefT *QosApiCreate(QosApiCallbackT *pCallback, void *pUserData, int32_t iServicePort)
{
    QosApiRefT *pQosApi;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pQosApi = DirtyMemAlloc(sizeof(*pQosApi), QOSAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapi: could not allocate module state\n"));
        return(NULL);
    }
    memset(pQosApi, 0, sizeof(*pQosApi));
    pQosApi->iMemGroup = iMemGroup;
    pQosApi->pMemGroupUserData = pMemGroupUserData;
    pQosApi->eQosApiNatType = QOSAPI_NAT_PENDING;
    pQosApi->iSpamValue = 1;

    // set the current request id to 2
    pQosApi->uCurrentId = 2;

    // set user callback
    pQosApi->pCallback = (pCallback == NULL) ? _QosApiDefaultCallback : pCallback;
    pQosApi->pUserData = pUserData;

    // set the service port
    pQosApi->uServicePort = iServicePort;

    // set the current timeout
    pQosApi->uCurrentTimeout = QOSAPI_DEFAULT_TIMEOUT;

    // set the current rate for re-issuing requests
    pQosApi->uCurrentReissueRate = QOSAPI_DEFAULT_REISSUE_RATE;

    // need to sync access to data
    NetCritInit(&pQosApi->ThreadCrit, "qosapi");

    // install idle callback
    NetConnIdleAdd(_QosApiUpdate, pQosApi);

    // return module state to caller
    return(pQosApi);
}

/*F********************************************************************************/
/*!
    \Function QosApiServiceRequest

    \Description
        Send a QoS request to the QoS service provider.

    \Input *pQosApi         - module state
    \Input *pServerAddress  - address of the server to send the request to
    \Input uProbePort       - port to send the qos probes to [NON-XENON only]
    \Input uServiceId       - service id to request on [XENON only]
    \Input uNumProbes       - number of probes to send out for the request
    \Input uBitsPerSec      - unused
    \Input uRequestType     - QOSAPI_REQUESTFL_*

    \Output 
        uint32_t        - request id, 0 if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
uint32_t QosApiServiceRequest(QosApiRefT *pQosApi, char *pServerAddress, uint16_t uProbePort, uint32_t uServiceId, uint32_t uNumProbes, uint32_t uBitsPerSec, uint32_t uRequestType)
{
    uint32_t uRet = 0;
    NetCritEnter(&pQosApi->ThreadCrit);
    
    //todo do we want to change the public api so it instead takes a eType
    if ((uRequestType == REQUEST_TYPE_LATENCY) || (uRequestType == REQUEST_TYPE_BANDWIDTH) || (uRequestType == REQUEST_TYPE_FIREWALL))
    {
        QosApiRequestTypeE eType = (QosApiRequestTypeE)uRequestType;
        QosApiRequestT *pRequest = _QosApiRequestCommon(pQosApi, eType, uNumProbes);

        if (pRequest != NULL)
        {
            uRet = pRequest->uRequestId;
            if (pRequest->pHttpHandler != NULL)        // if the request uses an http handler (most of them do) set the target address of the request
            {
                ds_strnzcpy(pRequest->pHttpHandler->strTargetAddress, pServerAddress, sizeof(pRequest->pHttpHandler->strTargetAddress));
                pRequest->uTargetProbePort = uProbePort;
            }
        }
    }
    else
    {
        NetPrintf(("qosapi: QosApiServiceRequest unexpected request type.\n"));
    }

    NetCritLeave(&pQosApi->ThreadCrit);
    return(uRet);
}

/*F********************************************************************************/
/*!
    \Function QosApiCancelRequest

    \Description
        Cancel the specified QoS request.

    \Input *pQosApi   - module state
    \Input uRequestId - request to cancel

    \Output 
        int32_t - 0 if successful, negative if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiCancelRequest(QosApiRefT *pQosApi, uint32_t uRequestId)
{
    int32_t iRet = 0;
    NetCritEnter(&pQosApi->ThreadCrit);
    
    if (_QosApiDestroyRequest(pQosApi, uRequestId) != 0)
    {
        NetPrintf(("qosapi: the specified request does not exist; no requests cancelled\n"));
        iRet = -1;
    }
    
    NetCritLeave(&pQosApi->ThreadCrit);
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function QosApiControl

    \Description
        QosApi control function.  Different selectors control different behaviours.

    \Input *pQosApi - module state
    \Input iControl - control selector
    \Input iValue   - selector specific data
    \Input pValue   - selector specific data

    \Output 
        int32_t - control specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'cbfp' - set callback function pointer - pValue=pCallback
            'lprt' - Set the port to use for the QoS listen port (must be set before calling listen/request)
            'sbps' - Set the maximum bits per second that QoS responses should use
            'spam' - Set the verbosity of the http module (not used on xenon)
            'sprt' - Set the port to send the http requests to when issuing a blaze service request (not used on xenon)
            'time' - Set the current timeout for new requests (milliseconds)
            'uidx' - Sets the user index
            'rira' - Sets the new re-issue rate of requests in ms.
        \endverbatim

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiControl(QosApiRefT *pQosApi, int32_t iControl, int32_t iValue, void *pValue)
{
    int32_t iRet = 0;
    NetCritEnter(&pQosApi->ThreadCrit);

    if (iControl == 'cbfp')
    {
        // set callback function pointer
        pQosApi->pCallback = ((pValue != NULL) ? (QosApiCallbackT *)pValue : _QosApiDefaultCallback);
    }
    else if (iControl == 'lprt')
    {
        pQosApi->uListenPort = iValue;
    }
    else if (iControl == 'sbps')
    {
    }
    else if (iControl == 'spam')
    {
        pQosApi->iSpamValue = iValue;
    }
    else if (iControl == 'sprt')
    {
        pQosApi->uServicePort = iValue;
    }
    else if (iControl == 'time')
    {
        pQosApi->uCurrentTimeout = (iValue < QOSAPI_MINIMUM_TIMEOUT) ? QOSAPI_MINIMUM_TIMEOUT : iValue;
    }
    else if (iControl == 'rira')
    {
        pQosApi->uCurrentReissueRate = (iValue < QOSAPI_MINIMUM_REISSUE_RATE) ? QOSAPI_MINIMUM_REISSUE_RATE : iValue;
    }
    else if (iControl == 'uidx')
    {
    }
    else
    {
        iRet = -1;
    }

    NetCritLeave(&pQosApi->ThreadCrit);
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function QosApiStatus

    \Description
        Return quality of service information.

    \Input *pQosApi - module state
    \Input iSelect  - output selector
    \Input iData    - selector specific
    \Input *pBuf    - [out] pointer to output buffer
    \Input iBufSize - size of output buffer

    \Output 
        int32_t - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'sess' - Get the session information
            'requ' - Gets the information for the specified request
            'extn' - Gets the external address information of the client for the specified request
            'clpt' - Gets the current listen port of the QoS socket
            'time' - Gets the current timeout value for QoS requests
        \endverbatim

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiStatus(QosApiRefT *pQosApi, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize)
{
    QosApiRequestT *pQosApiRequest;
    int32_t iRet = 0;

    NetCritEnter(&pQosApi->ThreadCrit);

    if (iSelect == 'sess')
    {
    }
    else if ((iSelect == 'requ') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(QosInfoT)))
    {
        if ((pQosApiRequest = _QosApiGetRequest(pQosApi, (unsigned)iData)) != NULL)
        {
            QosInfoT info;
            _QosApiGetQosInfo(pQosApiRequest, &info);
            // safe copy of qos info
            ds_memcpy(pBuf, &info, sizeof(QosInfoT));
        }
        else
        {
            iRet = -2;
        }
    }
    else if ((iSelect == 'extn') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(struct sockaddr)))
    {
        if ((pQosApiRequest = _QosApiGetRequest(pQosApi, (unsigned)iData)) != NULL)
        {
            if (pQosApiRequest->eType == REQUEST_TYPE_LATENCY)
            {
                ds_memcpy(pBuf, &pQosApiRequest->data.Latency.ExternalAddr, sizeof(struct sockaddr));
            }
            else
            {
                NetPrintf(("qosapi: 'extn', you may only get the ExternalAddr from a latency request.\n"));
                iRet = -3;
            }
        }
        else
        {
            iRet = -2;
        }
    }
    else if (iSelect == 'clpt')
    {
        iRet = pQosApi->uCurrentListenPort;
    }
    else if (iSelect == 'time')
    {
        iRet = pQosApi->uCurrentTimeout;
    }
    else
    {
        iRet = -1;
    }

    NetCritLeave(&pQosApi->ThreadCrit);
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function QosApiGetNatType

    \Description
        Returns the current NAT information of the client.

    \Input *pQosApi - module state

    \Output 
        QosApiNatTypeE - nat type.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
QosApiNatTypeE QosApiGetNatType(QosApiRefT *pQosApi)
{
#ifdef DIRTYCODE_XBOXONE

    int32_t iResult = 0;
    int32_t iNatType;

    iResult = NetConnStatus('natt', 0, &iNatType, sizeof(iNatType));
    if (iResult < 0)
    {
        NetPrintf(("qosapi: QosApiGetNatType() error retrieving NAT type\n"));
        return(QOSAPI_NAT_UNKNOWN);
    }

    if (iNatType == 0)
    {
        return(QOSAPI_NAT_OPEN);
    }
    else if (iNatType == 1)
    {
        return(QOSAPI_NAT_MODERATE);
    }
    else if (iNatType == 2)
    {
        return(QOSAPI_NAT_STRICT);
    }

    NetPrintf(("qosapi: QosApiGetNatType() unknown error\n"));
    return(QOSAPI_NAT_UNKNOWN);

#else

    return(pQosApi->eQosApiNatType);

#endif
}

/*F********************************************************************************/
/*!
    \Function QosApiDestroy

    \Description
        Destroy the QosApi module.

    \Input *pQosApi - module state

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
void QosApiDestroy(QosApiRefT *pQosApi)
{
    NetCritEnter(&pQosApi->ThreadCrit);
    NetConnIdleDel(_QosApiUpdate, pQosApi);

    if (pQosApi->pSocket != NULL)
    {
        SocketClose(pQosApi->pSocket);
        pQosApi->pSocket = NULL;
    }

    while (pQosApi->pRequestQueue != NULL)
    {
        _QosApiDestroyRequest(pQosApi, pQosApi->pRequestQueue->uRequestId);
    }

    NetCritLeave(&pQosApi->ThreadCrit);
    NetCritKill(&pQosApi->ThreadCrit);

    // release module memory
    DirtyMemFree(pQosApi, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
}

