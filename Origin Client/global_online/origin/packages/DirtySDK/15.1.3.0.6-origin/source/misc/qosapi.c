/*H********************************************************************************/
/*!
    \File qosapi.c

    \Description
        This module implements the client API for the quality of service.

    \Copyright
        Copyright (c) 2008, 2014, 2016 Electronic Arts Inc.

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
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protoname.h"
#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/xml/xmlparse.h"

#include "DirtySDK/misc/qosapi.h"

/*** Defines **********************************************************************/

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
#define QOSAPI_MINIMUM_TIMEOUT      (7000)

//!< qosapi default timeout period
#define QOSAPI_DEFAULT_TIMEOUT      (QOSAPI_MINIMUM_TIMEOUT)

//!< qosapi minimum time before re-issuing
#define QOSAPI_MINIMUM_REISSUE_RATE (200)

//!< qosapi time till re-issue ping requests
#define QOSAPI_DEFAULT_REISSUE_RATE (1000)

//!< qosapi time till re-issue ping requests
#define QOSAPI_DEFAULT_REISSUE_EXTRA_PROBES     (2)

//!< qosapi time between probe sends
#define QOSAPI_DEFAULT_TIME_GAP_BETWEEN_PROBES  (25)

//!< max time to wait for all the latency tests to start at the same time, after this time the tests will continue without synchronizing
#define QOSAPI_DEFAULT_INIT_SYNC_TIME           (2500)

//!< passed to SocketControl 'rbuf'
#define QOSAPI_DEFAULT_RECEIVE_BUFF_SIZE        (16*1024)

//!< passed to SocketControl 'sbuf'
#define QOSAPI_DEFAULT_SEND_BUFF_SIZE           (16*1024)

/*! passed to SocketControl 'pque'
    By default non-virtual sockets have a 1-deep packet queue. When _QosApiRecvCB() fails
    to enter pQosApi->ThreadCrit we need space to buffer the packet. If extracting packets 
    from the socket receive buffer is delayed, then rtt time calculated from those probe replies 
    ends up being incorrectly higher than it should be. 
       
    Testing has shown that with 6 ping sites providing ~equal ping times a packet queue of 2 is sometimes required.
    If pQosApi->ThreadCrit fails to enter 50% of the time, approximately a 'pque' of 6 is required.
    With pQosApi->ThreadCrit failing 100% of the time, an 11 deep 'pque' successfully allows the updating via NetConnIdle to handle it.*/
#define QOSAPI_DEFAULT_PACKET_QUEUE_SIZE        (12)

//!< qosapi timeout for socket idle callback (0 to disable the idle callback)
#define QOSAPI_SOCKET_IDLE_RATE     (0)

//!< qosapi blaze http
#define QOSAPI_HTTP_VERSION         (0x0001)

//!< http receive buffer size
#define QOSAPI_HTTP_BUFFER_SIZE     (4096)

//!< buffer size to format URL strings into
#define QOSAPI_HTTP_MAX_URL_LENGTH  (256)

//!< easily switch from https to http for testing
#define QOSAPI_DEFAULT_USE_HTTPS    (TRUE)

//!< packet size of incoming latency probes
#define QOSAPI_LATENCY_MIN_RESPONSE_SIZE  (30)

//!< packet size of outgoing latency probes
#define QOSAPI_LATENCY_REQUEST_SIZE  (20)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct QosApiHttpHandlerT                   //!< variables necessary for http communication needed by many request types
{
    char aRecvBuf[QOSAPI_HTTP_BUFFER_SIZE];             //!< buffer to store the http response data in
    char strUrlBasePath[QOSAPI_HTTP_MAX_URL_LENGTH];    //!< the http address to issue config requests to
    ProtoHttpRefT *pProtoHttp;                          //!< http module, used for service requests to the QOS server
    uint8_t bHttpPending;                               //!< is there a http get request waiting for a response?
} QosApiHttpHandlerT;

typedef struct QosApiRequestLatencyT                //!< Used to gather latency metrics, for REQUEST_TYPE_LATENCY
{
    //config
    uint32_t uTargetProbeCount;                     //!< total number of latency probes to have a round trip
    //status
    uint32_t uInitSyncTime;                         //!< the max time we wait after completing our initialization, used to be sure we don't wait too long for others
    uint32_t uProbesToSend;                         //!< the number of probes we need to send including retries and such
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

typedef enum QosApiRequestStateE
{
    REQUEST_STATE_INIT = 0,                         //!< prepare configuration, usually http request to the server
    REQUEST_STATE_INIT_SYNC,                        //!< attempt to start all latency tests at the same time, wait for other latency tests to finish their init before proceeding to the send state
    REQUEST_STATE_SEND,                             //!< send a series of probes to the target
    REQUEST_STATE_UPDATE,                           //!< wait for probes/response from target
    REQUEST_STATE_COMPLETE,                         //!< succeeded or failed, callback the user and clean up
} QosApiRequestStateE;                                        

//! QoS request
typedef struct QosApiRequestT
{
    struct QosApiRequestT *pNext;                   //!< link to the next record
    QosApiHttpHandlerT *pHttpHandler;               //!< allocated if the request requires http communication 

    uint32_t uTargetProbeAddr;                      //!< addr we want to send the QOS probes to
    uint16_t uTargetProbePort;                      //!< port we want to send the QOS probes to

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
    uint32_t uReceiveBuffSize;                      //!< passed to SocketControl 'rbuf'
    uint32_t uSendBuffSize;                         //!< passed to SocketControl 'sbuf'
    uint32_t uPacketQueueSize;                      //!< passed to SocketControl 'pque'

    QosApiRequestT *pRequestQueue;                  //!< linked list of requests pending
    QosApiNatTypeE eQosApiNatType;                  //!< current NAT type

    char strTargetAddressOverride[QOSAPI_HTTP_MAX_URL_LENGTH];  //!< the http address to issue config requests to, overrides serverAddress param in all QosApiServiceRequest
    int32_t iServicePortOverride;                   //!< port the http interface on the service provider is bound to, is only used if QosApiControl('sprt') is used and overrides port param in all QosApiServiceRequest (back to old behavior)
    uint32_t uCurrentId;                            //!< current request id, incremented every time a new request is made
    uint32_t uRequestTimeout;                       //!< current timeout for requests in milliseconds
    uint32_t uReissueRate;                          //!< time till request probes are re-issued when no response arrives
    uint32_t uReissueExtraProbeCount;               //!< in the case of finding packet loss, we request the lost packets again with a few extras, to reduce the chance that those are lost too
    uint32_t uTimeGapBetweenProbes;                 //!< the amount of time we should wait between each latency probe
    uint32_t uInitSyncTimeout;                      //!< the amount of time we might wait to synchronize all latency requests together
    int32_t iLatencyProbeCountOverride;             //!< the number of latency probes to send, overrides what the server sends as configuration
    int32_t iBandwidthProbeCountOverride;           //!< the number of bandwidth probes to send, overrides what the server sends as configuration
    int32_t iBandwidthProbeSizeOverride;            //!< the size of bandwidth probes to send, overrides what the server sends as configuration
    int16_t iSpamValue;                             //!< the current logging level
    uint8_t bSendAllLatencyProbesAtOnce;            //!< use old logic, such that all latency probes are sent in a tight loop (bypasses uTimeGapBetweenProbes)
    uint8_t bUseHttps;                              //!< true if communication is done via https otherwise http

    NetCritT ThreadCrit;                            //!< critical section, we lock receive thread, update, and public apis to ensure safety with one accessors to the module at a time, since everything access the pRequestQueue
    int32_t iDeferredRecv;                          //!< data available for reading, _QosApiUpdate should take care of it

    uint16_t uListenPort;                           //!< port we want to listen on, we receive probes from the server here
    uint16_t uCurrentListenPort;                    //!< port we are currently listening on (we may not get the port we wanted to listen on)
    
};

/*** Function Prototypes **********************************************************/

static int32_t _QosApiRecvCB(SocketT *pSocket, int32_t iFlags, void *pData);
void _QosApiUpdateServiceRequestProgress(QosApiRefT *pQosApi, QosApiRequestT *pRequest);

/*** Variables ********************************************************************/
#if DIRTYCODE_LOGGING
const char *_strQosApiRequestState[] =              //!< keep in sync with QosApiRequestStateE
{
    "REQUEST_STATE_INIT     ",                         
    "REQUEST_STATE_INIT_SYNC",                         
    "REQUEST_STATE_SEND     ",                         
    "REQUEST_STATE_UPDATE   ",                       
    "REQUEST_STATE_COMPLETE "                    
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
    
    \Input *pQosApi     - module state
    \Input *pRequest    - pointer to the request
    \Input eState       - new state to transition to, REQUEST_STATE_*
    \Input uStatus      - flags indicating past state progress QOSAPI_STATFL_*
    \Input uModule      - module producing the hResult code, DIRTYAPI_*
    \Input iCode        - status/error code to use in hResult
    \Input iLogLevel    - the log level this state transition should be logged at, equivalent to NetPrintfVerbose((pQosApi->iSpamValue, iLogLevel, ...

    \Version 09/19/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiStateTransition(QosApiRefT *pQosApi, QosApiRequestT *pRequest, QosApiRequestStateE eState, uint32_t uStatus, uint32_t uModule, int32_t iCode, int32_t iLogLevel)
{
    pRequest->hResult = DirtyErrGetHResult(uModule, iCode, uStatus != QOSAPI_STATFL_COMPLETE);
    pRequest->uStatus |= uStatus;
    
    NetPrintfVerbose((pQosApi->iSpamValue, iLogLevel, "qosapi: request[%02d] transitioning from %s-> %s (hResult: 0x%x status: %s)\n", pRequest->uRequestId, _strQosApiRequestState[pRequest->eState], _strQosApiRequestState[eState], pRequest->hResult, _QosApiGetStatusString(uStatus)));
    pRequest->eState = eState;
}

/*F********************************************************************************/
/*!
    \Function _QosApiSocketOpen

    \Description
        Open QosApi socket

    \Input *pQosApi     - pointer to module state

    \Output 
        QosApiRequestT * - pointer to the new request

    \Version 09/13/2013 (jbrookes)
*/
/********************************************************************************F*/
static SocketT *_QosApiSocketOpen(QosApiRefT *pQosApi)
{
    struct sockaddr LocalAddr;
    int32_t iResult;
    SocketT *pSocket;

    // set the port to the default value if 0
    if (pQosApi->uListenPort == 0)
    {
        pQosApi->uListenPort = QOSAPI_DEFAULT_LISTENPORT;
    }

    // create the socket
    if ((pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0)) == NULL)
    {
        NetPrintf(("qosapi: could not allocate socket\n"));
        return(NULL);
    }

    // make sure socket receive buffer is large enough to queue up
    // multiple probe responses (worst case: 10 probes * 1200 bytes)
    SocketControl(pSocket, 'rbuf', pQosApi->uReceiveBuffSize, NULL, NULL);
    SocketControl(pSocket, 'sbuf', pQosApi->uSendBuffSize, NULL, NULL);
    SocketControl(pSocket, 'pque', pQosApi->uPacketQueueSize, NULL, NULL);

    SockaddrInit(&LocalAddr, AF_INET);
    SockaddrInSetPort(&LocalAddr, pQosApi->uListenPort);

    // bind the socket
    if ((iResult = SocketBind(pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
    {
        NetPrintf(("qosapi: error %d binding socket to port %d, trying random\n", iResult, pQosApi->uListenPort));
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
    NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: bound socket to port %u\n", pQosApi->uCurrentListenPort));

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
    if(eRequestType == QOSAPI_REQUEST_FIREWALL)
    {
        bUseHttp = FALSE;
    }
#endif 

    // alloc top level request
    if ((pQosApiRequest = (QosApiRequestT*)DirtyMemAlloc(sizeof(*pQosApiRequest), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapi: error allocating request.\n"));
        return(NULL);
    }
    ds_memclr(pQosApiRequest, sizeof(*pQosApiRequest));
    pQosApiRequest->hResult = DirtyErrGetHResult(DIRTYAPI_QOS, QOSAPI_STATUS_INIT, TRUE);

    if (bUseHttp)
    {
        // allocate the http handler (wrapping protohttp)
        if ((pQosApiRequest->pHttpHandler = (QosApiHttpHandlerT*)DirtyMemAlloc(sizeof(*pQosApiRequest->pHttpHandler), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("qosapi: error allocating http handler.\n"));
            _QosApiFreeRequest(pQosApi, pQosApiRequest);
            return(NULL);
        }
        ds_memclr(pQosApiRequest->pHttpHandler, sizeof(*pQosApiRequest->pHttpHandler));

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

    NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] created, %s.\n", pQosApiRequest->uRequestId, _strQosApiRequestType[pQosApiRequest->eType]));
    
#ifdef DIRTYCODE_XBOXONE
    //on xbox one we can get the NAT type from the OS so the request is already complete
    if(pQosApiRequest->eType == QOSAPI_REQUEST_FIREWALL)
    {
        _QosApiStateTransition(pQosApi, pQosApiRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_SUCCESS, 0);
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

        //apply overrides to the received config
        //todo number of probes should have come from the server too (why not)
        if (pQosApi->iLatencyProbeCountOverride != -1)
        {
            pRequest->data.Latency.uProbesToSend = pRequest->data.Latency.uTargetProbeCount = pQosApi->iLatencyProbeCountOverride;
        }

        // validate basic config needed for both latency and BW
        if ((pRequest->uTargetProbeAddr == 0) || (pRequest->uTargetProbePort == 0))
        {
            NetPrintf(("qosapi: request[%02d] _QosApiParseHttpLatencyInitResponse config parse error required data missing, addr=%a, port=%d\n", pRequest->uRequestId, pRequest->uTargetProbeAddr, pRequest->uTargetProbePort));
            return(-3);
        }
        pRequest->data.Latency.uInitSyncTime = NetTick();
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_INIT_SYNC, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
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
        if (pRequest->eType == QOSAPI_REQUEST_BANDWIDTH)
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

            //apply overrides to the received config
            if (pQosApi->iBandwidthProbeCountOverride != -1)
            {
                pRequest->data.Bandwidth.uProbeTotal = pQosApi->iBandwidthProbeCountOverride;
            }
            if (pQosApi->iBandwidthProbeSizeOverride != -1)
            {
                pRequest->data.Bandwidth.uProbeSize = pQosApi->iBandwidthProbeSizeOverride;
            }

        }

        // validate specific BW config
        if ((pRequest->eType == QOSAPI_REQUEST_BANDWIDTH) && 
            ( (pRequest->data.Bandwidth.uReqId == 0) || (pRequest->data.Bandwidth.uProbeTotal < 2) || (pRequest->data.Bandwidth.uProbeSize == 0) ))
        {
            NetPrintf(("qosapi: request[%02d] _QosApiParseHttpBandwidthInitResponse config parse error required data missing, requestId=%d, probecount=%d, probesize=%d\n", pRequest->uRequestId, pRequest->data.Bandwidth.uReqId, pRequest->data.Bandwidth.uProbeTotal, pRequest->data.Bandwidth.uProbeSize));
            return(-4);
        }
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
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
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
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
            pQosApi->eQosApiNatType = (QosApiNatTypeE)XmlContentGetInteger(pXml2, 0);
            _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_SUCCESS, 0);
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

    NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] <- http response\n%s\n", pRequest->uRequestId, pRequest->pHttpHandler->aRecvBuf));

    if (pRequest->eState == REQUEST_STATE_INIT)
    {
        if (pRequest->eType == QOSAPI_REQUEST_LATENCY)
        {
            iRet = _QosApiParseHttpLatencyInitResponse(pQosApi, pRequest);
        }
        else if (pRequest->eType == QOSAPI_REQUEST_BANDWIDTH)
        {
            iRet = _QosApiParseHttpBandwidthInitResponse(pQosApi, pRequest);
        }
        else if (pRequest->eType == QOSAPI_REQUEST_FIREWALL)
        {
            iRet = _QosApiParseHttpFirewallInitResponse(pQosApi, pRequest);
        }
    }
    else if (pRequest->eState == REQUEST_STATE_UPDATE)
    {
        if (pRequest->eType == QOSAPI_REQUEST_FIREWALL)
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
    \Function _QosApiLatencyRequestWaitForInit

    \Description
        Transition to the send state if no other latency request are in the init state.
        We prefer to start all latency tests at the same time because if the users connection 
        suffers from periodic lag spikes, this will produce a more "fair" result.  Otherwise what 
        might be the best ping site can suffer from a random occurrence, and a less ideal ping 
        site which was lucky enough to avoid the lag spike will come out as the best ping site.
        Now all ping sites should avoid or suffer from the same lab spike since they are occurring
        at the same time.

    \Input *pQosApi  - pointer to module state
    \Input *pRequest - pointer to the request

    \Version 08/11/2016 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiLatencyRequestWaitForInit(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    QosApiRequestT * pRequestToCheck;

    if (NetTickDiff(NetTick(), pRequest->data.Latency.uInitSyncTime) > (int32_t)pQosApi->uInitSyncTimeout) //too much time passed
    {
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] giving up on waiting for other latency init, %dms have already passed.\n", pRequest->uRequestId, pQosApi->uInitSyncTimeout));
    }
    else
    {
        for (pRequestToCheck = pQosApi->pRequestQueue; pRequestToCheck != NULL; pRequestToCheck = pRequestToCheck->pNext)
        {
            if ((pRequestToCheck->eType == QOSAPI_REQUEST_LATENCY) && (pRequestToCheck->eState == REQUEST_STATE_INIT))
            {
                return;//one of the latency requests is still waiting for http/init to complete, lets continue to wait so that we can all measure latency at the same time
            }
        }
    }
    _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
}

/*F********************************************************************************/
/*!
    \Function _QosApiLatencyRequestSendProbes

    \Description
        Sends a latency probe out to the request's target.

    \Input *pQosApi  - pointer to module state
    \Input *pRequest - pointer to the request

    \Version 11/07/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiLatencyRequestSendProbes(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t aPacketData[QOSAPI_LATENCY_REQUEST_SIZE];
    struct sockaddr SendAddr;
    uint32_t uSendCounter;
    int32_t iResult = 0;
    uint32_t uProbeCount = 1;        //send probes one at a time
    
    //revert back to the old logic where there would be no gap in between probes to a given ping site
    if (pQosApi->bSendAllLatencyProbesAtOnce)
    {
        uProbeCount = pRequest->data.Latency.uProbesToSend - pRequest->data.Latency.uProbesSent;
    }

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
        ds_memclr(aPacketData, sizeof(aPacketData));
        aPacketData[0] = SocketHtonl(pRequest->uRequestId);
        aPacketData[1] = SocketHtonl(pRequest->eType);
        //aPacketData[2] = 0
        //aPacketData[3] = 0
        aPacketData[4] = SocketHtonl(NetTick());

        iResult = SocketSendto(pQosApi->pSocket, (const char *)aPacketData, QOSAPI_LATENCY_REQUEST_SIZE, 0, &SendAddr, sizeof(SendAddr));
        NetPrintfVerbose((pQosApi->iSpamValue, 2, "qosapi: request[%02d] sent latency probe %d to %A (result=%d)\n", pRequest->uRequestId, pRequest->data.Latency.uProbesSent, &SendAddr, iResult));

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
        NetPrintfVerbose((pQosApi->iSpamValue, 1, "qosapi: request[%02d] sent %d latency probes to address %A\n", pRequest->uRequestId, uProbeCount, &SendAddr));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 1);
        pRequest->uTimeIssued = NetTick();
    }
    else if (iResult == 0)  // need to try again
    {
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] needs to try again to send latency probe to %A\n", pRequest->uRequestId, &SendAddr));
        // transition anyways, let the re-issue logic handle the missing probes
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
        pRequest->uTimeIssued = NetTick();
    }    
    else if (iResult < 0)   // an error
    {
        NetPrintf(("qosapi: request[%02d] failed to send latency probe %d to address %A (err=%d)\n", pRequest->uRequestId, pRequest->data.Latency.uProbesSent, &SendAddr, iResult));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult, 0);
    }
    else                    // unexpected result
    {
        NetPrintf(("qosapi: request[%02d] unexpected result for latency probe %d to address %A (err=%d)\n", pRequest->uRequestId, pRequest->data.Latency.uProbesSent, &SendAddr, iResult));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult, 0);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiBandwidthRequestSendProbes

    \Description
        Sends a series of bandwidth probe out to the request's target.

    \Input *pQosApi   - pointer to module state
    \Input *pRequest  - pointer to the request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiBandwidthRequestSendProbes(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
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
        ds_memclr(aPacketData, sizeof(aPacketData));
        aPacketData[0] = SocketHtonl(pRequest->uRequestId);
        aPacketData[1] = SocketHtonl(pRequest->data.Bandwidth.uReqId);
        aPacketData[2] = SocketHtonl(pRequest->data.Bandwidth.iReqSecret);
        aPacketData[3] = SocketHtonl(pRequest->data.Bandwidth.uProbesSent);
        aPacketData[4] = SocketHtonl(pRequest->data.Bandwidth.uProbeTotal);

        iResult = SocketSendto(pQosApi->pSocket, (const char *)aPacketData, pRequest->data.Bandwidth.uProbeSize, 0, &SendAddr, sizeof(SendAddr));
        NetPrintfVerbose((pQosApi->iSpamValue, 2, "qosapi: request[%02d] sent bandwidth probe %d to %A (result=%d)\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbesSent, &SendAddr, iResult));

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
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] sent %d bandwidth probes to address %A\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbeTotal, &SendAddr));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
        pRequest->uTimeIssued = NetTick();
    }
    else if (iResult == 0)  // need to try again
    {
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] needs to try again to send %d more bandwidth probes to %A\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbeTotal - pRequest->data.Bandwidth.uProbesSent, &SendAddr));
        // if we just stay in the current state we will try again
    }    
    else if (iResult < 0)   // an error
    {
        NetPrintf(("qosapi: request[%02d] failed to send bandwidth probe %d to address %A (err=%d)\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbesSent, &SendAddr, iResult));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult, 0);
    }
    else                    // unexpected result
    {
        NetPrintf(("qosapi: request[%02d] unexpected result for bandwidth probe %d to address %A (err=%d)\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbesSent, &SendAddr, iResult));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult, 0);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiFirewallRequestSendProbes

    \Description
        Sends two firewall probes to the service addresses.

    \Input *pQosApi   - pointer to module state
    \Input *pRequest  - request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiFirewallRequestSendProbes(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
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
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] sent firewall probe %d to %A (result=%d)\n", pRequest->uRequestId, pRequest->data.Firewall.uProbesSent, &SendAddr, iResult));

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
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
        pRequest->uTimeIssued = NetTick();
    }
    else if (iResult == 0)  // need to try again
    {
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] needs to try again to send more firewall probes to %A\n", pRequest->uRequestId, &SendAddr));
        // transition anyways, let the re-issue logic handle the missing probes
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_UPDATE, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
        pRequest->uTimeIssued = NetTick();
    }    
    else if (iResult < 0)   // an error
    {
        NetPrintf(("qosapi: request[%02d] failed to send firewall probe to address %A (err=%d)\n", pRequest->uRequestId, &SendAddr, iResult));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult, 0);
    }
    else                    // unexpected result
    {
        NetPrintf(("qosapi: request[%02d] unexpected result for firewall probe %d to address %A (err=%d)\n", pRequest->uRequestId, &SendAddr, iResult));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_SOCKET, iResult, 0);
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
    // int32_t iDataLength          // currently server never returns data so 0, but a client->client QOS could come back with some data (which we no long support)
    // char[] data                  // up to 256 bytes of data (will always be none)

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

    NetPrintfVerbose((pQosApi->iSpamValue, 2, "qosapi: request[%02d] received probe %d\n", pRequest->uRequestId, pRequest->data.Latency.uProbesRecv));

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
        Move to next steps of the QOS process if necessary
    
    \Input *pQosApi     - module state
    \Input *pRequest    - probe resulting from this request

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiProcessLatencyStateTransition(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    //determine next state transitions
    if (!(pRequest->uStatus & QOSAPI_STATFL_CONTACTED)) //first probe QOSAPI_STATFL_CONTACTED
    {
        _QosApiStateTransition(pQosApi, pRequest, pRequest->eState, QOSAPI_STATFL_CONTACTED, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
    }
    else if ((pRequest->data.Latency.uProbesRecv >= (pRequest->data.Latency.uTargetProbeCount / 2))  // half the probes QOSAPI_STATFL_PART_COMPLETE
    && !(pRequest->uStatus & QOSAPI_STATFL_PART_COMPLETE))
    {
        _QosApiStateTransition(pQosApi, pRequest, pRequest->eState, QOSAPI_STATFL_PART_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
    }
    else if ((pRequest->data.Latency.uProbesRecv >= pRequest->data.Latency.uTargetProbeCount))   // all the probes QOSAPI_STATFL_COMPLETE
    {
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_SUCCESS, 0);
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

    NetPrintfVerbose((pQosApi->iSpamValue, 2, "qosapi: request[%02d] received probe %d\n", pRequest->uRequestId, pRequest->data.Bandwidth.uProbesRecv));

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function _QosApiProcessBandwidthStateTransition

    \Description
        Move to next steps of the QOS process if necessary
    
    \Input *pQosApi     - module state
    \Input *pRequest    - probe resulting from this request
    \Input *pPacketData - pointer to packet bytes

    \Version 10/31/2014 (cvienneau)
*/
/********************************************************************************F*/
static void _QosApiProcessBandwidthStateTransition(QosApiRefT *pQosApi, QosApiRequestT *pRequest, char *pPacketData)
{
    uint32_t uProbeSequenceNumber;

    //note its possible to receive bandwidth probes while we're in the send or update state
    uProbeSequenceNumber = SocketNtohl(*(int32_t *)&pPacketData[12]);
    if (!(pRequest->uStatus & QOSAPI_STATFL_CONTACTED)) //first probe QOSAPI_STATFL_CONTACTED
    {
        _QosApiStateTransition(pQosApi, pRequest, pRequest->eState, QOSAPI_STATFL_CONTACTED, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
    }
    if ((uProbeSequenceNumber >= (pRequest->data.Bandwidth.uProbeTotal / 2))  // half the probes QOSAPI_STATFL_PART_COMPLETE
    && !(pRequest->uStatus & QOSAPI_STATFL_PART_COMPLETE))
    {
        _QosApiStateTransition(pQosApi, pRequest, pRequest->eState, QOSAPI_STATFL_PART_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
    }
    // finish if we received the final bandwidth probe (or we spent too much time waiting, and timed out)
    else if (uProbeSequenceNumber >= pRequest->data.Bandwidth.uProbeTotal)
    {
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_COMPLETE, DIRTYAPI_QOS, QOSAPI_STATUS_SUCCESS, 0);
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

    //validate that the packet queue is healthy, it can be difficult to detect so we have special logging to check
#if DIRTYCODE_LOGGING
    int32_t iQueueHighWater = SocketInfo(pQosApi->pSocket, 'pmax', 0, NULL, 0);
    int32_t iQueueSize = SocketInfo(pQosApi->pSocket, 'psiz', 0, NULL, 0);
    int32_t iPacketDrop = SocketInfo(pQosApi->pSocket, 'pdrp', 0, NULL, 0);

    //if we are getting close to using all of the packet queue complain always
    if (iQueueHighWater >= (iQueueSize -2))
    {
       NetPrintf(("qosapi: warning %d/%d packet queue high watermark, lost packets %d.\n", iQueueHighWater, iQueueSize, iPacketDrop));
    }
    else
    {
        NetPrintfVerbose((pQosApi->iSpamValue, 2, "qosapi: %d/%d packet queue high watermark, lost packets %d.\n", iQueueHighWater, iQueueSize, iPacketDrop));
    }
#endif

    // get request matching id
    if ((pRequest = _QosApiGetRequest(pQosApi, uRequestId)) == NULL)
    {
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] could not be found, ignoring.\n", uRequestId));
        return;
    }

    // make sure it's not failed or timed out
    if (pRequest->eState == REQUEST_STATE_COMPLETE)
    {
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] response received but ignored, since the request is already completed.\n", pRequest->uRequestId));
        return;
    }

    // get recv time stamp from sockaddr
    uSockTick = SockaddrInGetMisc(pRecvAddr);
    
    if (pRequest->eType == QOSAPI_REQUEST_LATENCY)
    {
        if (_QosApiValidateLatencyProbe(pQosApi, pRequest, pPacketData, iPacketLen))
        {
            //determine the external address and address the probe came back from
            _QosApiProcessLatencyAddress(pRequest, pPacketData, pRecvAddr);

            //calculate rtt
           _QosApiProcessLatencyRTT(pRequest, pPacketData, uSockTick);

           //now that we've processed the full probe, determine if the state needs to change
           _QosApiProcessLatencyStateTransition(pQosApi, pRequest);
        }

    }
    else if (pRequest->eType == QOSAPI_REQUEST_BANDWIDTH)
    {
        if (_QosApiValidateBandwidthProbe(pQosApi, pRequest, pPacketData, iPacketLen))
        {
            _QosApiProcessBandwidthProbe(pRequest, pPacketData, uSockTick);

            _QosApiProcessBandwidthStateTransition(pQosApi, pRequest, pPacketData);
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
        _QosApiRecvResponse(pQosApi, aPacketData, iRecvLen, &RecvAddr, uRequestId);
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

    NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] sending http request %s.\n", pRequest->uRequestId, strUrl));

    // issue the request
    if ((iResult = ProtoHttpGet(pRequest->pHttpHandler->pProtoHttp, strUrl, FALSE)) >= 0)
    {
        pRequest->pHttpHandler->bHttpPending = TRUE;
    }
    else
    {
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_PROTO_HTTP, iResult, 0);
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
        if (pQosApi->iDeferredRecv)
        {
            /*
            reset the flag before we start to process the data,
            this could be safer since it's not protected by any mutex
            */
            pQosApi->iDeferredRecv = 0;
            _QosApiRecv(pQosApi->pSocket, pQosApi);
        }
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
                    //parsing code would have logged specific failure information
                    _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_QOS, QOSAPI_STATUS_HTTP_PARSE_RESPONSE_FAILED, 0);
                }
            }
            else
            { 
                NetPrintf(("qosapi: request[%02d] error http status %d\n", pRequest->uRequestId, iHttpStatusCode));
                _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_PROTO_HTTP, iHttpStatusCode, 0);
            }
        }
        else if ((iResult < 0) && (iResult != PROTOHTTP_RECVWAIT))
        {
            int32_t hResult = ProtoHttpStatus(pRequest->pHttpHandler->pProtoHttp, 'hres', NULL, 0);
            if (hResult >= 0)  //in this case there was no socket or ssl error
            {
                NetPrintf(("qosapi: request[%02d] error ProtoHttpRecvAll failed iResult=%d\n", pRequest->uRequestId, iResult));
                _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_FAILED, DIRTYAPI_PROTO_HTTP, iResult, 0); 
            }
            else
            {
                // normally use _QosApiUpdateRequestStatus but since we already have the hResult we do it manually here, report the socket/ssl hRestult
                NetPrintf(("qosapi: request[%02d] error ProtoHttpRecvAll failed hResult=%p\n", pRequest->uRequestId, hResult));
                pRequest->hResult = hResult;
                pRequest->uStatus |= QOSAPI_STATFL_FAILED;
                pRequest->eState = REQUEST_STATE_COMPLETE;
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
        // https://qos-prod-bio-dub-common-common.gos.ea.com:17504/qos/qos?vers=1&qtyp=1

        ds_snzprintf(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "%s/qos/qos", pRequest->pHttpHandler->strUrlBasePath);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "?vers=", QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&qtyp=", pRequest->eType);

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
    // there are two reasons why we might transition back to REQUEST_STATE_SEND
    // 1 the common case is that enough time has lapsed in between probes
    if ((pRequest->data.Latency.uProbesSent < pRequest->data.Latency.uProbesToSend) &&      //we haven't sent all probes yet
        (NetTickDiff(NetTick(), pRequest->uTimeIssued) > (int32_t)pQosApi->uTimeGapBetweenProbes) //too much time passed
        )
    {
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 1);
    }
    // 2 too much time may have laps before receiving responses in which case we want to send more probes than initially planned
    if ((pRequest->data.Latency.uProbesRecv < pRequest->data.Latency.uTargetProbeCount) &&      //we haven't received the number of responses we want, and
        (NetTickDiff(NetTick(), pRequest->uTimeIssued) > (int32_t)pQosApi->uReissueRate) //too much time passed
        )
    {
        int32_t iAdditionalProbes = (pRequest->data.Latency.uTargetProbeCount - pRequest->data.Latency.uProbesRecv) + pQosApi->uReissueExtraProbeCount;
        pRequest->data.Latency.uProbesToSend += iAdditionalProbes;
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] re-sending %d new probes.\n", pRequest->uRequestId, iAdditionalProbes));
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
    ds_memclr(pQosInfo, sizeof(QosInfoT));
    
    if (pRequest->eType == QOSAPI_REQUEST_LATENCY)
    {
        _QosApiGetLatencyQosInfo(pRequest, pQosInfo);
    }
    else if (pRequest->eType == QOSAPI_REQUEST_BANDWIDTH)
    {
        _QosApiGetBandwidthQosInfo(pRequest, pQosInfo);
    }
    else if (pRequest->eType == QOSAPI_REQUEST_FIREWALL)
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

    ds_memclr(&CBInfo, sizeof(QosApiCBInfoT));
    _QosApiGetQosInfo(pRequest, &QosInfo);

    CBInfo.uOldStatus = 0;  //todo is old status useful?
    CBInfo.uNewStatus = pRequest->uStatus;
    CBInfo.pQosInfo = &QosInfo;

    NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] latency test completed...\n uWhenRequested: %u\n uAddr: %a\n ExternalAddr: %a:%d\n iProbesSent: %d\n iProbesRecv: %d\n iMinRTT: %d\n iMedRTT: %d\n uFlags: %d\n hResult: 0x%x\n", 
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
    else if (pRequest->eState == REQUEST_STATE_INIT_SYNC)
    {
        _QosApiLatencyRequestWaitForInit(pQosApi, pRequest);
    }
    else if (pRequest->eState == REQUEST_STATE_SEND)
    {
        _QosApiLatencyRequestSendProbes(pQosApi, pRequest);
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
        // https://qos-prod-rs-ord-common-common.gos.ea.com:17504/qos/qos?vers=1&qtyp=2

        ds_snzprintf(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "%s/qos/qos", pRequest->pHttpHandler->strUrlBasePath);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "?vers=", QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "&qtyp=", pRequest->eType);

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

    ds_memclr(&CBInfo, sizeof(QosApiCBInfoT));
    _QosApiGetQosInfo(pRequest, &QosInfo);

    CBInfo.uOldStatus = 0;  //todo is old status useful?
    CBInfo.uNewStatus = pRequest->uStatus;
    CBInfo.pQosInfo = &QosInfo;

    NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] bandwidth test completed...\n uWhenRequested: %u\n iProbesSent: %d\n iProbesRecv: %d\n uUpBitsPerSec: %d\n uDwnBitsPerSec: %d\n uFlags: %d\n hResult: 0x%x\n", 
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
        _QosApiBandwidthRequestSendProbes(pQosApi, pRequest);
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

        ds_snzprintf(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "%s/qos/firewall", pRequest->pHttpHandler->strUrlBasePath);
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

        ds_snzprintf(strUrl, QOSAPI_HTTP_MAX_URL_LENGTH, "%s/qos/firetype", pRequest->pHttpHandler->strUrlBasePath);
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
    if (NetTickDiff(NetTick(), pRequest->uTimeIssued) > (int32_t)pQosApi->uReissueRate)
    {
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_SEND, QOSAPI_STATFL_NONE, DIRTYAPI_QOS, QOSAPI_STATUS_PENDING, 0);
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] re-issued; sending %d new probes.\n", pRequest->uRequestId, QOSAPI_NUM_INTERFACES));
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

    ds_memclr(&CBInfo, sizeof(QosApiCBInfoT));
    _QosApiGetQosInfo(pRequest, &QosInfo);

    CBInfo.uOldStatus = 0;  //todo, is old status useful?
    CBInfo.uNewStatus = pRequest->uStatus;
    CBInfo.pQosInfo = &QosInfo;
    CBInfo.eQosApiNatType = QosApiGetNatType(pQosApi);

    NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] firewall test completed...\n uWhenRequested: %u\n iProbesSent: %d\n eQosApiNatType: %d\n uFlags: %d\n hResult: 0x%x\n", 
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
        _QosApiFirewallRequestSendProbes(pQosApi, pRequest);
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
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: request[%02d] timed out (%d ms)\n", pRequest->uRequestId, NetTickDiff(NetTick(), pRequest->uTimeRequested)));
        _QosApiStateTransition(pQosApi, pRequest, REQUEST_STATE_COMPLETE, QOSAPI_STATFL_TIMEDOUT, DIRTYAPI_QOS, QOSAPI_STATUS_OPERATION_TIMEOUT, 0);
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
    if (pRequest->eType == QOSAPI_REQUEST_LATENCY)
    {
        bRequestCompleted = _QosApiUpdateLatencyRequest(pQosApi, pRequest);
    }
    else if (pRequest->eType == QOSAPI_REQUEST_BANDWIDTH)
    {
        bRequestCompleted = _QosApiUpdateBandwidthRequest(pQosApi, pRequest);
    }
    else if (pRequest->eType == QOSAPI_REQUEST_FIREWALL)
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

    \Output 
        QosApiRequestT* - pointer to request ref

    \Version 10/29/2014 (cvienneau)
*/
/********************************************************************************F*/
static QosApiRequestT *_QosApiRequestCommon(QosApiRefT *pQosApi, QosApiRequestTypeE eType)
{
    QosApiRequestT *pRequest;

    //todo get rid of this after server refactor, a person shouldn't always need to use the override if the config comes from the server, but it doesn't yet 
    //all this other probe specific setup should be moved to the respective _QosApiParseHttpLatencyInitResponse/_QosApiParseHttpBandwidthInitResponse/_QosApiParseHttpFirewallInitResponse
    //to use the actual probe counts for those request types
    uint32_t uNumProbes = 10;
    if (pQosApi->iLatencyProbeCountOverride == -1)
    {
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: using default latency probe count of 10.\n"));
        pQosApi->iLatencyProbeCountOverride = 10; 
    }
    uNumProbes = pQosApi->iLatencyProbeCountOverride;
    //end of code to be removed

    // if the module's socket hasn't yet been allocated
    if ((pQosApi->pSocket == NULL) && ((pQosApi->pSocket = _QosApiSocketOpen(pQosApi)) == NULL))
    {
        NetPrintf(("qosapi: error opening listening socket.\n"));
        return(NULL);
    }
    
    // set the number of probes to the minimum/maximum if required  
    if (uNumProbes < QOSAPI_MINPROBES)
    {
        uNumProbes = QOSAPI_MINPROBES;
    }
    else if (uNumProbes > QOSAPI_MAXPROBES)
    {
        uNumProbes = QOSAPI_MAXPROBES;
    }

    // allocate the request
    if ((pRequest = _QosApiCreateRequest(pQosApi, eType)) == 0)
    {
        NetPrintf(("qosapi: unable to create the request\n"));
        return(NULL);
    }

    // configure the request
    if (eType == QOSAPI_REQUEST_LATENCY)
    {
        // save the number of probes to send (other request types get this directly from the QOS server)
        pRequest->data.Latency.uProbesToSend = pRequest->data.Latency.uTargetProbeCount = uNumProbes;
    }

    // set the request timeout
    pRequest->uTimeout = pQosApi->uRequestTimeout;

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
    \Input uListenPort  - set the port incoming probes will go to, pass 0 to use defaults

    \Output 
        QosApiRefT * - state pointer

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
QosApiRefT *QosApiCreate(QosApiCallbackT *pCallback, void *pUserData, uint16_t uListenPort)
{
    QosApiRefT *pQosApi;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pQosApi = (QosApiRefT*)DirtyMemAlloc(sizeof(*pQosApi), QOSAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapi: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pQosApi, sizeof(*pQosApi));
    pQosApi->iMemGroup = iMemGroup;
    pQosApi->pMemGroupUserData = pMemGroupUserData;
    pQosApi->eQosApiNatType = QOSAPI_NAT_PENDING;
    pQosApi->iSpamValue = 1;
    pQosApi->uListenPort = uListenPort;

    // set the current request id to 2
    pQosApi->uCurrentId = 2;

    // set user callback
    pQosApi->pCallback = (pCallback == NULL) ? _QosApiDefaultCallback : pCallback;
    pQosApi->pUserData = pUserData;

    //socket buffer sizes
    pQosApi->uReceiveBuffSize = QOSAPI_DEFAULT_RECEIVE_BUFF_SIZE;
    pQosApi->uSendBuffSize = QOSAPI_DEFAULT_SEND_BUFF_SIZE;
    pQosApi->uPacketQueueSize = QOSAPI_DEFAULT_PACKET_QUEUE_SIZE;

    // set the current timeout
    pQosApi->uRequestTimeout = QOSAPI_DEFAULT_TIMEOUT;

    // set the current rate for re-issuing requests
    pQosApi->uReissueRate = QOSAPI_DEFAULT_REISSUE_RATE;

    // how many extra probes to send along in the case of discovering packet loss
    pQosApi->uReissueExtraProbeCount = QOSAPI_DEFAULT_REISSUE_EXTRA_PROBES;

    // set the current time spacing between latency probes
    pQosApi->uTimeGapBetweenProbes = QOSAPI_DEFAULT_TIME_GAP_BETWEEN_PROBES;

    // set the time we might wait for starting all latency requests at the same time
    pQosApi->uInitSyncTimeout = QOSAPI_DEFAULT_INIT_SYNC_TIME;

    //turn all overrides off by default
    pQosApi->bUseHttps = QOSAPI_DEFAULT_USE_HTTPS;
    pQosApi->iServicePortOverride = -1;
    pQosApi->iLatencyProbeCountOverride = -1;  
    pQosApi->iBandwidthProbeCountOverride = -1;
    pQosApi->iBandwidthProbeSizeOverride = -1; 

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
    \Input *pServerAddress  - address of the server to send http config query to
    \Input uServerPort      - port to match pServerAddress
    \Input eRequestType     - Type of request being made

    \Output 
        uint32_t        - request id, 0 if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
uint32_t QosApiServiceRequest(QosApiRefT *pQosApi, const char *pServerAddress, uint16_t uServerPort, QosApiRequestTypeE eRequestType)
{
    uint32_t uRet = 0;
    NetCritEnter(&pQosApi->ThreadCrit);

    //todo do we want to change the public api so it instead takes a eType
    if ((eRequestType == QOSAPI_REQUEST_LATENCY) || (eRequestType == QOSAPI_REQUEST_BANDWIDTH) || (eRequestType == QOSAPI_REQUEST_FIREWALL))
    {
        QosApiRequestT *pRequest = _QosApiRequestCommon(pQosApi, eRequestType);

        if (pRequest != NULL)
        {
            uRet = pRequest->uRequestId;
            
            // if the request uses an http handler (most of them do) set the target server info
            if (pRequest->pHttpHandler != NULL)         
            {
                const char * strHttpMode = "https";
                const char * strAddress = pServerAddress;
                uint16_t uPort = uServerPort;

                if (pQosApi->bUseHttps != TRUE)
                {
                    strHttpMode = "http";
                }

                if (pQosApi->strTargetAddressOverride[0] != '\0')
                {
                    strAddress = pQosApi->strTargetAddressOverride;
                }

                //set the port we are targeting for http config communication
                if (pQosApi->iServicePortOverride != -1)
                {
                    uPort = pQosApi->iServicePortOverride;
                }

                ds_snzprintf(pRequest->pHttpHandler->strUrlBasePath, QOSAPI_HTTP_MAX_URL_LENGTH, "%s://%s:%u", strHttpMode, strAddress, uPort);
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
        NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: the specified request does not exist; no requests cancelled\n"));
        iRet = -1;
    }
    
    NetCritLeave(&pQosApi->ThreadCrit);
    return(iRet);
}

/*F********************************************************************************/
/*!
    \Function QosApiControl

    \Description
        QosApi control function.  Different selectors control different behaviors.

    \Input *pQosApi - module state
    \Input iControl - control selector
    \Input iValue   - selector specific data
    \Input pValue   - selector specific data

    \Output 
        int32_t - control specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'bpco' - if used overrides config from server, this will be the number of bandwidth probes used for tests
            'bpso' - if used overrides config from server, this will be the size of bandwidth probes used for tests
            'cbfp' - set callback function pointer - pValue=pCallback
            'isyt' - sets the maximum number of ms to wait to initially synchronize all latency requests
            'lpco' - if used overrides config from server, this will be the number of latency probes used for tests
            'lprt' - set the port to use for the QoS listen port (must be set before calling listen/request), same as the value passed to QosApiCreate
            'pque' - passed to SocketControl 'pque' if not 0, otherwise the numpackets will be passed to SocketControl 'pque'
            'rbuf' - passed to SocketControl 'rbuf'
            'repc' - sets the number of extra probes sent (in addition to those which received no response) on a re-issue
            'rira' - sets the new re-issue rate of latency requests in ms.
            'sado' - set the override address to send the http requests to when issuing a service request, overrides parameter passed to QosApiServiceRequest
            'salp' - if TRUE use old logic of sending all latency probes at once (default FALSE)
            'sbuf' - passed to SocketControl 'sbuf'
            'spam' - set the verbosity of the module, default 1 (0 errors, 1 debug info, 2 extended debug info, 3 per probe info)
            'sprt' - set the override port to send the http requests to when issuing a service request, overrides parameter passed to QosApiServiceRequest
            'time' - set the current timeout for new requests (milliseconds)
            'tgbp' - sets the minimum number of ms between sending latency probes to the same site.
            'ussl' - if TRUE use https else http for setup communication (defaults TRUE)

        \endverbatim

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiControl(QosApiRefT *pQosApi, int32_t iControl, int32_t iValue, void *pValue)
{
    int32_t iRet = 0;
    NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: [%p] QosApiControl('%C'), (%d)\n", pQosApi, iControl, iValue));
    
    NetCritEnter(&pQosApi->ThreadCrit);
    if (iControl == 'bpco')
    {
        pQosApi->iBandwidthProbeCountOverride = iValue;
    }
    else if (iControl == 'bpso')
    {
        pQosApi->iBandwidthProbeSizeOverride = iValue;
    } 
    else if (iControl == 'cbfp')
    {
        // set callback function pointer
        pQosApi->pCallback = ((pValue != NULL) ? (QosApiCallbackT *)pValue : _QosApiDefaultCallback);
    }
    else if (iControl == 'isyt')
    {
        pQosApi->uInitSyncTimeout = iValue;
    }
    else if (iControl == 'lpco')
    {
        pQosApi->iLatencyProbeCountOverride = iValue;
    } 
    else if (iControl == 'lprt')
    {
        pQosApi->uListenPort = iValue;
    }
    else if (iControl == 'pque')
    {
        pQosApi->uPacketQueueSize = iValue;
    }  
    else if (iControl == 'rbuf')
    {
        pQosApi->uReceiveBuffSize = iValue;
    }
    else if (iControl == 'repc')
    {
        pQosApi->uReissueExtraProbeCount = iValue;
    }
    else if (iControl == 'rira')
    {
        pQosApi->uReissueRate = (iValue < QOSAPI_MINIMUM_REISSUE_RATE) ? QOSAPI_MINIMUM_REISSUE_RATE : iValue;
    }
    else if (iControl == 'sado')
    {
        ds_strnzcpy(pQosApi->strTargetAddressOverride, (const char *)pValue, sizeof(pQosApi->strTargetAddressOverride));
    } 
    else if (iControl == 'salp')
    {
        pQosApi->bSendAllLatencyProbesAtOnce = iValue;
    } 
    else if (iControl == 'sbuf')
    {
        pQosApi->uSendBuffSize = iValue;
    }
    else if (iControl == 'spam')
    {
        pQosApi->iSpamValue = iValue;
    }
    else if (iControl == 'sprt')
    {
        pQosApi->iServicePortOverride = iValue;
    }
    else if (iControl == 'time')
    {
        pQosApi->uRequestTimeout = (iValue < QOSAPI_MINIMUM_TIMEOUT) ? QOSAPI_MINIMUM_TIMEOUT : iValue;
    }
    else if (iControl == 'tgbp')
    {
        pQosApi->uTimeGapBetweenProbes = iValue;
    }    
    else if (iControl == 'ussl')
    {
        pQosApi->bUseHttps = iValue;
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
            'bpco' - get the override value for number of bandwidth probes used in tests (-1 if not used)
            'bpso' - get the override value for size of bandwidth probes used for tests (-1 if not used)
            'clpt' - get the current listen port of the QoS socket
            'extn' - get the external address information of the client for the specified request
            'isyt' - get the maximum number of ms to wait to initially synchronize all latency requests
            'lpco' - get the override used for number of latency probes used for tests (-1 if not used)
            'lprt' - get the port to use for the QoS listen port
            'pque' - get the value passed to SocketControl 'pque' (0 if not used)
            'rbuf' - get the value passed to SocketControl 'rbuf'
            'repc' - get the number of extra probes sent (in addition to those which received no response) on a re-issue
            'requ' - get the information for the specified request
            'rira' - get the new re-issue rate of latency requests in ms.
            'sado' - get the override address to send the http requests to when issuing a service request, overrides parameter passed to QosApiServiceRequest
            'salp' - get if send all latency probes at once is set 
            'sbuf' - get the value passed to SocketControl 'sbuf'
            'spam' - get the verbosity of the module
            'sprt' - get the override port to send the http requests to when issuing a service request, overrides parameter passed to QosApiServiceRequest
            'time' - get the current timeout value for QoS requests
            'tgbp' - get the minimum number of ms between sending latency probes to the same site.
            'ussl' - if TRUE use https else http for setup communication

        \endverbatim

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiStatus(QosApiRefT *pQosApi, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize)
{
    int32_t iRet = 0;
    NetPrintfVerbose((pQosApi->iSpamValue, 1, "qosapi: [%p] QosApiStatus('%C'), (%d)\n", pQosApi, iSelect, iData));

    NetCritEnter(&pQosApi->ThreadCrit);

    if (iSelect == 'bpco')
    {
        iRet = pQosApi->iBandwidthProbeCountOverride;
    }
    else if (iSelect == 'bpso')
    {
        iRet = pQosApi->iBandwidthProbeSizeOverride;
    } 
    if (iSelect == 'clpt')
    {
        iRet = pQosApi->uCurrentListenPort;
    }
    else if ((iSelect == 'extn') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(struct sockaddr)))
    {
        QosApiRequestT *pQosApiRequest;
        if ((pQosApiRequest = _QosApiGetRequest(pQosApi, (unsigned)iData)) != NULL)
        {
            if (pQosApiRequest->eType == QOSAPI_REQUEST_LATENCY)
            {
                ds_memcpy(pBuf, &pQosApiRequest->data.Latency.ExternalAddr, sizeof(struct sockaddr));
            }
            else
            {
                NetPrintfVerbose((pQosApi->iSpamValue, 0, "qosapi: 'extn', you may only get the ExternalAddr from a latency request.\n"));
                iRet = -3;
            }
        }
        else
        {
            iRet = -2;
        }
    }
    else if (iSelect == 'isyt')
    {
        iRet = pQosApi->uInitSyncTimeout;
    }
    else if (iSelect == 'lpco')
    {
        iRet = pQosApi->iLatencyProbeCountOverride;
    } 
    else if (iSelect == 'lprt')
    {
        iRet = pQosApi->uListenPort;
    }
    else if (iSelect == 'pque')
    {
        iRet = pQosApi->uPacketQueueSize;
    }  
    else if (iSelect == 'rbuf')
    {
        iRet = pQosApi->uReceiveBuffSize;
    }
    else if (iSelect == 'repc')
    {
        iRet = pQosApi->uReissueExtraProbeCount;
    }
    else if ((iSelect == 'requ') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(QosInfoT)))
    {
        QosApiRequestT *pQosApiRequest;
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
    else if (iSelect == 'rira')
    {
        iRet = pQosApi->uReissueRate;
    }
    else if (iSelect == 'sado')
    {
        ds_strnzcpy(pBuf, pQosApi->strTargetAddressOverride, iBufSize);
        iRet = 1;
    } 
    else if (iSelect == 'salp')
    {
        iRet = pQosApi->bSendAllLatencyProbesAtOnce;
    } 
    else if (iSelect == 'sbuf')
    {
        iRet = pQosApi->uSendBuffSize;
    }
    else if (iSelect == 'spam')
    {
        iRet = pQosApi->iSpamValue;
    }
    else if (iSelect == 'sprt')
    {
        iRet = pQosApi->iServicePortOverride;
    }
    else if (iSelect == 'time')
    {
        iRet = pQosApi->uRequestTimeout;
    }
    else if (iSelect == 'tgbp')
    {
        iRet = pQosApi->uTimeGapBetweenProbes;
    }    
    else if (iSelect == 'ussl')
    {
        iRet = pQosApi->bUseHttps;
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

