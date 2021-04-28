/*H********************************************************************************/
/*!
    \File qosapi.c

    \Description
        This module implements the client API for the quality of service server and peer-to-peer communication.

    \Todo
        Add version information to the HttpRequest
        Encode the URL using ProtoHttpEncodeParam instead of snzprintf

    \Copyright
        Copyright (c) 2008 Electronic Arts Inc.

    \Version 1.0 04/07/2008 (cadam) First Version
*/
/********************************************************************************H*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "dirtyerr.h"
#include "lobbytagfield.h"
#include "netconn.h"
#include "protoname.h"
#include "protohttp.h"
#include "xmlparse.h"

#include "qosapi.h"

/*** Defines **********************************************************************/

//! enable verbose output
#define QOSAPI_DEBUG                (DIRTYCODE_DEBUG && FALSE)

//!< qosapi default bits per second
#define QOSAPI_DEFAULT_BITSPERSEC   (16384)

//!< qosapi minimum bits per second
#define QOSAPI_MINIMUM_BITSPERSEC   (4096)

//!< qosapi default listen port
#define QOSAPI_DEFAULT_LISTENPORT   (7673)

//!< qosapi number of firewall probe sets to send
#define QOSAPI_NUM_PROBESETS        (5)

//!< qosapi number of interfaces to use for a firewall request
#define QOSAPI_NUM_INTERFACES       (2)

//!< qosapi maximum packet length
#define QOSAPI_MAXPACKET            (2048)

//!< qosapi maximum probes per request
#define QOSAPI_MAXPROBES            (64)

//!< qosapi minimum probes per request
#define QOSAPI_MINPROBES            (1)

//!< qosapi timeout period
#define QOSAPI_MINIMUM_TIMEOUT      (5000)

//!< qosapi minimum time before re-issuing
#define QOSAPI_MINIMUM_REISSUE_RATE (200)

//!< qosapi time till re-issue ping requests
#define QOSAPI_DEFAULT_REISSUE_RATE (1000)

//!< qosapi timeout for socket idle callback (0 to disable the idle callback)
#define QOSAPI_SOCKET_IDLE_RATE     (0)

//!< qosapi blaze http version
#define QOSAPI_HTTP_VERSION         (0x0001)

//!< qosapi blaze qos types
#define QOSAPI_BLAZE_LATENCY        (1)
#define QOSAPI_BLAZE_BANDWIDTH      (2)
#define QOSAPI_BLAZE_FIREWALL       (3)
#define QOSAPI_BLAZE_FIRETYPE       (4)

/*** Type Definitions *************************************************************/

//! QoS service information
typedef struct QosApiServiceReqT
{
    ProtoHttpRefT *pProtoHttp;                              //!< http module, used for service requests to the blaze server
    HostentT *pTarget;                                      //!< hostent to resolve
    uint16_t uProbePort;                                    //!< port we want to send the qos probes to
    char strServiceUrl[128];                                //!< the service address to issue the requests to
    uint8_t aRecvBuf[4096];                                    //!< buffer to store the http response data in
    uint32_t uHttpPending;                                  //!< is there a http get request waiting for a response?

    enum
    {
        SS_PROTONAME = 0,
        SS_LATENCY,
        SS_BANDWIDTH_HTTP,
        SS_BANDWIDTH_START,
        SS_BANDWIDTH_SENT,
        SS_FIREWALL_HTTP,
        SS_FIREWALL_START,
        SS_FIREWALL_SENT
    } eServiceState;                                        //!< the current service request state

    uint32_t uBandwidth;                                    //!< service bandwidth probe size
    uint32_t uBandwidthSent;                                //!< number of bandwidth probes sent so far
    uint32_t uBandwidthTotal;                               //!< total number of bandwidth probes to send
    uint32_t uBandwidthRecv;                                //!< number of bandwidth probes received
    uint32_t uBwdRecvTime;                                  //!< time when the first bandwidth probe was received

    uint32_t uServiceReqId;                                 //!< request id the service expects
    int32_t iServiceReqSecret;                              //!< secret that the service expects

    uint32_t uNumInterfaces;                                //!< number of interfaces returned by the service
    uint32_t aAddrs[QOSAPI_NUM_INTERFACES];                 //!< ip addresses of the service
    uint16_t aPorts[QOSAPI_NUM_INTERFACES];                 //!< ports of the service

    uint32_t uProbeSetsSent;                                //!< number of firewall probe sets sent
    uint32_t uFirewallReqId;                                //!< request id the service expects
    int32_t iFirewallReqSecret;                             //!< secret that the service expects
} QosApiServiceReqT;

//! QoS request
typedef struct QosApiRequestT
{
    struct QosApiRequestT *pNext;                           //!< link to the next record

    QosInfoT *pQosInfo;                                     //!< pointer to the QosInfoT receive data
    uint32_t uStatus;                                       //!< request status
    uint32_t uFlags;                                        //!< request flags
    int32_t iValidRTTs;                                     //!< number of valid RTT values from probes
    uint32_t uTimeDiff;                                     //!< average difference between the client and target times
    uint32_t aProbeRTTs[QOSAPI_MAXPROBES];                  //!< array of probe roundtrip times

    uint32_t uTargetAddr;                                   //!< address of the target machine
    uint16_t uTargetPort;                                   //!< udp port of the target

    QosApiServiceReqT *pService;                            //!< service request information

    enum
    {
        TP_CLIENT = 0,                                      //!< client request
        TP_SERVICE                                          //!< service request
    } eRequestType;                                         //!< request type

    int32_t iIssued;                                        //!< has the request been issued
    uint32_t uTimeIssued;                                   //!< the time the request was issued at
    uint32_t uNumProbes;                                    //!< number of probes the request sends
    uint32_t uNumProbesReissued;                            //!< number of probes the request re-sent
    uint32_t uBitsPerSec;                                   //!< bit rate specified for the request
    uint32_t uTimeout;                                      //!< timeout for this request in milliseconds
} QosApiRequestT;

//! current module state
struct QosApiRefT
{
    // module memory group
    int32_t iMemGroup;                                      //!< module mem group id
    void *pMemGroupUserData;                                //!< user data associated with mem group

    uint32_t uCurrentId;                                    //!< current request id
    uint32_t uUserIdx;                                      //!< user index

    uint16_t uServicePort;                                  //!< port the http interface on the service provider is bound to
    int16_t iSpamValue;                                     //!< the current spam value for the http module

    QosApiCallbackT *pCallback;                             //!< callback function to jump to when a status change occurs
    void *pUserData;                                        //!< user specified data to pass to the callback

    uint8_t aResponse[QOSAPI_RESPONSE_MAXSIZE];             //!< title specific data to send when responding to QoS probes
    uint32_t uResponseSize;                                 //!< size of the title specific data

    SocketT *pSocket;                                       //!< socket pointer
    NetCritT ThreadCrit;                                    //!< critical section
    #if SOCKET_ASYNCRECVTHREAD
    int32_t iDeferredRecv;                                  //!< data available for reading, _QosApiUpdate should take care of it
    #endif

    uint32_t uListenSet;                                    //!< has listen already been called?
    uint16_t uListenPort;                                   //!< port to listen on
    uint16_t uCurrentListenPort;                            //!< port we are currently listening on
    QosListenStatusT QosListenStatus;                       //!< current QoS listen status

    uint32_t uCurrentTimeout;                               //!< current timeout for requests in milliseconds
    uint32_t uCurrentReissueRate;                           //!< time till requests are re-issued when no response arrives

    QosApiRequestT *pRequestQueue;                          //!< linked list of requests pending

    QosApiNatTypeE eQosApiNatType;                          //!< current nat type
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _QosApiCreateRequest

    \Description
        Create and allocate the required space for a request.

    \Input *pQosApi     - pointer to module state
    \Input uRequestType - request type

    \Output QosApiRequestT * - pointer to the new request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static QosApiRequestT *_QosApiCreateRequest(QosApiRefT *pQosApi, uint32_t uRequestType)
{
    QosApiRequestT *pQosApiRequest, **ppQosApiRequest;

    // alloc and init ref
    if ((pQosApiRequest = DirtyMemAlloc(sizeof(*pQosApiRequest), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapi: error allocating request\n"));
        return(NULL);
    }
    memset(pQosApiRequest, 0, sizeof(*pQosApiRequest));

    // alloc and init QosInfo
    if ((pQosApiRequest->pQosInfo = DirtyMemAlloc(sizeof(*pQosApiRequest->pQosInfo), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapi: error allocating QosInfo\n"));
        DirtyMemFree(pQosApiRequest, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
        return(NULL);
    }
    memset(pQosApiRequest->pQosInfo, 0, sizeof(*pQosApiRequest->pQosInfo));

    if (uRequestType == TP_SERVICE)
    {
        // alloc and init ref
        if ((pQosApiRequest->pService = DirtyMemAlloc(sizeof(*pQosApiRequest->pService), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("qosapi: error allocating service information\n"));
            DirtyMemFree(pQosApiRequest->pQosInfo, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            DirtyMemFree(pQosApiRequest, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            return(NULL);
        }
        memset(pQosApiRequest->pService, 0, sizeof(*pQosApiRequest->pService));

        // allocate the protohttp module
        if ((pQosApiRequest->pService->pProtoHttp = ProtoHttpCreate(4096)) == NULL)
        {
            NetPrintf(("qosapi: could not create the protohttp module\n"));
            DirtyMemFree(pQosApiRequest->pService, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            DirtyMemFree(pQosApiRequest->pQosInfo, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            DirtyMemFree(pQosApiRequest, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            return(NULL);
        }

        // set to the current spam mode
        ProtoHttpControl(pQosApiRequest->pService->pProtoHttp, 'spam', pQosApi->iSpamValue, 0, NULL);

        // set to a keep-alive connection
        ProtoHttpControl(pQosApiRequest->pService->pProtoHttp, 'keep', 1, 0, NULL);
    }

    // set the request type
    pQosApiRequest->eRequestType = uRequestType;

    // make sure we own resources
    NetCritEnter(&pQosApi->ThreadCrit);

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
    pQosApiRequest->pQosInfo->uRequestId = pQosApi->uCurrentId;

    // release resources
    NetCritLeave(&pQosApi->ThreadCrit);

    // increment the current request id
    pQosApi->uCurrentId++;

    // return ref
    return(pQosApiRequest);
}

/*F********************************************************************************/
/*!
    \Function _QosApiDestroyRequest

    \Description
        Remove request from queue and free any memory associated with the request.

    \Input *pQosApi   - pointer to module state
    \Input uRequestId - id of the request to destroy

    \Output int32_t - 0 if successful, negative otherwise

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
        if ((*ppQosApiRequest)->pQosInfo->uRequestId == uRequestId)
        {
            // make sure we own resources
            NetCritEnter(&pQosApi->ThreadCrit);

            // set the request
            pQosApiRequest = *ppQosApiRequest;

            // dequeue
            *ppQosApiRequest = (*ppQosApiRequest)->pNext;

            if (pQosApiRequest->eRequestType == TP_SERVICE)
            {
                // destroy the ProtoHttp module
                ProtoHttpDestroy(pQosApiRequest->pService->pProtoHttp);

                // free the hostent
                pQosApiRequest->pService->pTarget->Free(pQosApiRequest->pService->pTarget);

                // destroy the service information
                DirtyMemFree(pQosApiRequest->pService, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            }

            // dispose of request memory and quit loop
            DirtyMemFree(pQosApiRequest->pQosInfo, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            DirtyMemFree(pQosApiRequest, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);

            // release resources
            NetCritLeave(&pQosApi->ThreadCrit);

            return(0);
        }
    }

    // specified request not found
    return(-1);
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
        if (pRequest->pQosInfo->uRequestId == uRequestId)
        {
            return(pRequest);
        }
    }
    // did not find the request
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _QosApiReissueServiceRequest

    \Description
        Determine if a Service Request should be re-attempted, if so modify the
        request so that it is.

    \Input *pQosApi     - pointer to module state
    \Input *pRequest    - request

    \Output
        int32_t         - positive if re-issued, negative otherwise

    \Version 07/16/2009 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _QosApiReissueServiceRequest(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t iResult = -1;

    // make sure we own resources
    NetCritEnter(&pQosApi->ThreadCrit);

    //handling for latency probes
    if (
        (pRequest->iIssued == 1) &&                                 //the request has already been issued and
        ((pRequest->pService->eServiceState == SS_LATENCY) ||
        (pRequest->eRequestType == TP_CLIENT)) &&                   //the request in question is a latency test and
        (pRequest->pQosInfo->iProbesRecv < (int32_t)pRequest->uNumProbes) &&  //we haven't received all responses and
        (!(pRequest->pQosInfo->uFlags & QOSAPI_STATFL_FAILED)) &&   //the request is not in Error
        (!(pRequest->pQosInfo->uFlags & QOSAPI_STATFL_TIMEDOUT)) && //the request hasn't already timed out and
        (NetTickDiff(NetTick(), pRequest->uTimeIssued) > (int32_t)pQosApi->uCurrentReissueRate) //too much time passed
        )
    {
        pRequest->iIssued = 0;
        pRequest->uTimeIssued = 0;
        pRequest->uNumProbesReissued += pRequest->uNumProbes - pRequest->pQosInfo->iProbesRecv;
        pRequest->pQosInfo->iProbesSent -= (pRequest->uNumProbes - pRequest->pQosInfo->iProbesRecv);
        NetPrintf(("qosapi: re-issued latency requestid %d; sending %d new probes.\n", pRequest->pQosInfo->uRequestId, (pRequest->uNumProbes - pRequest->pQosInfo->iProbesRecv)));
        iResult = 1;
    }

    //handling for firewall probes
    if (
        (pRequest->pService->eServiceState == SS_FIREWALL_SENT) &&  //firewall probes have been sent
        (!(pRequest->pQosInfo->uFlags & QOSAPI_STATFL_COMPLETE)) &&   //the request is not finished
        (!(pRequest->pQosInfo->uFlags & QOSAPI_STATFL_FAILED)) &&   //the request is not in Error
        (!(pRequest->pQosInfo->uFlags & QOSAPI_STATFL_TIMEDOUT)) && //the request hasn't already timed out and
        (NetTickDiff(NetTick(), pRequest->uTimeIssued) > (int32_t)pQosApi->uCurrentReissueRate) //too much time passed
        )
    {
        pRequest->uTimeIssued = 0;
        pRequest->pService->uProbeSetsSent = 0;
        pRequest->pService->eServiceState = SS_FIREWALL_START;
        NetPrintf(("qosapi: re-issued firewall requestid %d; sending %d new probes.\n", pRequest->pQosInfo->uRequestId, QOSAPI_NUM_PROBESETS));
        iResult = 1;
    }

    // release resources
    NetCritLeave(&pQosApi->ThreadCrit);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _QosApiFormatServiceUrl

    \Description
        Format string with service url.

    \Input *pQosApi     - pointer to module state
    \Input *pRequest    - request
    \Input *pBuffer     - pointer to the request
    \Input iBufSize     - size of the buffer
    \Input iType        - type of http request

    \Version 08/07/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiFormatServiceUrl(QosApiRefT *pQosApi, QosApiRequestT *pRequest, char *pBuffer, int32_t iBufSize, int32_t iType)
{
    if (iType == QOSAPI_BLAZE_FIREWALL)
    {
        snzprintf(pBuffer, iBufSize, "http://%s:%u/qos/firewall?vers=%d", pRequest->pService->strServiceUrl, pQosApi->uServicePort, QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(pBuffer, iBufSize, "&nint=", QOSAPI_NUM_INTERFACES);
    }
    else if (iType == QOSAPI_BLAZE_FIRETYPE)
    {
        uint32_t uLocalIp = NetConnStatus('addr', 0, NULL, 0);

        snzprintf(pBuffer, iBufSize, "http://%s:%u/qos/firetype?vers=%d", pRequest->pService->strServiceUrl, pQosApi->uServicePort, QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(pBuffer, iBufSize, "&rqid=", pRequest->pService->uFirewallReqId);
        ProtoHttpUrlEncodeIntParm(pBuffer, iBufSize, "&rqsc=", pRequest->pService->iFirewallReqSecret);
        ProtoHttpUrlEncodeIntParm(pBuffer, iBufSize, "&inip=", uLocalIp);
        ProtoHttpUrlEncodeIntParm(pBuffer, iBufSize, "&inpt=", pQosApi->uCurrentListenPort);
    }
    else
    {
        snzprintf(pBuffer, iBufSize, "http://%s:%u/qos/qos?vers=%d", pRequest->pService->strServiceUrl, pQosApi->uServicePort, QOSAPI_HTTP_VERSION);
        ProtoHttpUrlEncodeIntParm(pBuffer, iBufSize, "&qtyp=", iType);
        ProtoHttpUrlEncodeIntParm(pBuffer, iBufSize, "&prpt=", pRequest->pService->uProbePort);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiParseHttpResponse

    \Description
        Parses the blaze http responses.

    \Input *pQosApi  - pointer to module state
    \Input *pRequest - pointer to the request

    \Output int32_t - negative on error, 0 on success

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static int32_t _QosApiParseHttpResponse(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    const unsigned char *pXml, *pXml2;

    NetPrintf(("qosapi: httprecvall -> %s\n", pRequest->pService->aRecvBuf));

    if ((pXml = XmlFind(pRequest->pService->aRecvBuf, (unsigned char *)"firewall")) != NULL)
    {
        uint32_t uInterface;
        const unsigned char *pXml3;

        pRequest->pService->uProbeSetsSent = 0;
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".numinterfaces")) != NULL)
        {
            pRequest->pService->uNumInterfaces = XmlContentGetInteger(pXml2, 0);
        }

        if (((pXml2 = XmlFind(pXml, (unsigned char *)".ips.ips")) != NULL) && ((pXml3 = XmlFind(pXml, (unsigned char *)".ports.ports")) != NULL))
        {
            // store the first interface information
            pRequest->pService->aAddrs[0] = XmlContentGetInteger(pXml2, 0);
            pRequest->pService->aPorts[0] = (uint16_t)XmlContentGetInteger(pXml3, 0);

            // get the rest of the interfaces required
            for (uInterface = 1; uInterface < pRequest->pService->uNumInterfaces; uInterface++)
            {
                pXml2 = XmlNext(pXml2);
                pXml3 = XmlNext(pXml3);

                // if we get null early then error
                if ((pXml2 == NULL) || (pXml3 == NULL))
                {
                    return(-2);
                }

                pRequest->pService->aAddrs[uInterface] = XmlContentGetInteger(pXml2, 0);
                pRequest->pService->aPorts[uInterface] = (uint16_t)XmlContentGetInteger(pXml3, 0);
            }
        }
        else
        {
            // could not find the interface information; error
            return(-2);
        }
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".requestid")) != NULL)
        {
            pRequest->pService->uFirewallReqId = XmlContentGetInteger(pXml2, 0);
        }
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".reqsecret")) != NULL)
        {
            pRequest->pService->iFirewallReqSecret = XmlContentGetInteger(pXml2, 0);
        }

        if (pRequest->pService->uFirewallReqId == 0)
        {
            return(-1);
        }

        return(0);
    }
    else if ((pXml = XmlFind(pRequest->pService->aRecvBuf, (unsigned char *)"firetype")) != NULL)
    {
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".firetype")) != NULL)
        {
            pQosApi->eQosApiNatType = XmlContentGetInteger(pXml2, 0);
        }
        pRequest->pService->uProbeSetsSent = QOSAPI_NUM_PROBESETS;

        if (pQosApi->eQosApiNatType != QOSAPI_NAT_PENDING)
        {
            QosApiCBInfoT CBInfo;
            memset(&CBInfo, 0, sizeof(CBInfo));
            CBInfo.eQosApiNatType = pQosApi->eQosApiNatType;
            NetPrintf(("qosapi: eQosApiNatType=%d\n", CBInfo.eQosApiNatType));
            pQosApi->pCallback(pQosApi, &CBInfo, QOSAPI_CBTYPE_NAT, pQosApi->pUserData);
        }

        return(0);
    }
    else if ((pXml = XmlFind(pRequest->pService->aRecvBuf, (unsigned char *)"qos")) != NULL)
    {
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".numprobes")) != NULL)
        {
            pRequest->pService->uBandwidthTotal = XmlContentGetInteger(pXml2, 0);
        }
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".probesize")) != NULL)
        {
            pRequest->pService->uBandwidth = XmlContentGetInteger(pXml2, 0);
        }
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".qosport")) != NULL)
        {
            pRequest->uTargetPort = XmlContentGetInteger(pXml2, 0);
        }
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".requestid")) != NULL)
        {
            pRequest->pService->uServiceReqId = XmlContentGetInteger(pXml2, 0);
        }
        if ((pXml2 = XmlFind(pXml, (unsigned char *)".reqsecret")) != NULL)
        {
            pRequest->pService->iServiceReqSecret = XmlContentGetInteger(pXml2, 0);
        }

        if (((pRequest->pService->eServiceState == SS_BANDWIDTH_HTTP) && ((pRequest->pService->uBandwidth == 0) || (pRequest->pService->uBandwidthTotal < 2))) ||
            (pRequest->uTargetPort == 0) || (pRequest->pService->uServiceReqId == 0))
        {
            return(-1);
        }

        return(0);
    }

    return(-2);
}

/*F********************************************************************************/
/*!
    \Function _QosApiSendLatencyProbe

    \Description
        Sends a latency probe out to the request's target.

    \Input *pQosApi  - pointer to module state
    \Input *pRequest - pointer to the request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiSendLatencyProbe(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t iResult;
    int32_t aPacketData[QOSAPI_MAXPACKET/sizeof(int32_t)];
    struct sockaddr SendAddr;

    // setup the target address
    SockaddrInit(&SendAddr, AF_INET);
    SockaddrInSetAddr(&SendAddr, pRequest->uTargetAddr);
    SockaddrInSetPort(&SendAddr, pRequest->uTargetPort);

    memset(aPacketData, 0, sizeof(aPacketData));
    aPacketData[0] = SocketHtonl(pRequest->pQosInfo->uRequestId);
    if (pRequest->eRequestType == TP_CLIENT)
    {
        aPacketData[1] = 0;
        aPacketData[2] = 0;
    }
    else
    {
        aPacketData[1] = SocketHtonl(pRequest->pService->uServiceReqId);
        aPacketData[2] = SocketHtonl(pRequest->pService->iServiceReqSecret);
    }
    aPacketData[3] = SocketHtonl(pRequest->uFlags);
    aPacketData[4] = SocketHtonl(NetTick());

    iResult = SocketSendto(pQosApi->pSocket, (const char *)aPacketData, 20, 0, &SendAddr, sizeof(SendAddr));

    // make sure we own resources
    NetCritEnter(&pQosApi->ThreadCrit);

    pRequest->pQosInfo->iProbesSent++;

    if (iResult != 20)
    {
        pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_FAILED;
        pRequest->iIssued = 1;
        pRequest->uTimeIssued = 0;
        pRequest->pQosInfo->iProbesSent += pRequest->uNumProbesReissued;
    }

    if (pRequest->pQosInfo->iProbesSent == (int32_t)pRequest->uNumProbes)
    {
        pRequest->iIssued = 1;
        pRequest->uTimeIssued = NetTick();
    }

    // release resources
    NetCritLeave(&pQosApi->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function _QosApiSendBandwidthProbe

    \Description
        Sends a bandwidth probe out to the request's target.

    \Input *pQosApi   - pointer to module state
    \Input *pRequest  - pointer to the request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiSendBandwidthProbe(QosApiRefT *pQosApi, QosApiRequestT *pRequest)
{
    int32_t iResult;
    int32_t aPacketData[QOSAPI_MAXPACKET/sizeof(int32_t)];
    struct sockaddr SendAddr;

    // setup the target address
    SockaddrInit(&SendAddr, AF_INET);
    SockaddrInSetAddr(&SendAddr, pRequest->uTargetAddr);
    SockaddrInSetPort(&SendAddr, pRequest->uTargetPort);

    memset(aPacketData, 0, sizeof(aPacketData));
    aPacketData[0] = SocketHtonl(pRequest->pQosInfo->uRequestId);
    aPacketData[1] = SocketHtonl(pRequest->pService->uServiceReqId);
    aPacketData[2] = SocketHtonl(pRequest->pService->iServiceReqSecret);
    aPacketData[3] = SocketHtonl(pRequest->pService->uBandwidthSent);
    aPacketData[4] = SocketHtonl(pRequest->pService->uBandwidthTotal);

    iResult = SocketSendto(pQosApi->pSocket, (const char *)aPacketData, pRequest->pService->uBandwidth, 0, &SendAddr, sizeof(SendAddr));

    if (iResult != (int32_t)pRequest->pService->uBandwidth)
    {
        NetPrintf(("qosapi: failed to send bandwidth probe %d (err=%d)\n", pRequest->pService->uBandwidthSent, iResult));
    }

    pRequest->pService->uBandwidthSent++;
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
    int32_t iResult;

    // init the socket address
    SockaddrInit(&SendAddr, AF_INET);

    // set the payload
    aPacketData[0] = SocketHtonl(pRequest->pService->uFirewallReqId);
    aPacketData[1] = SocketHtonl(pRequest->pService->iFirewallReqSecret);

    // send probes to each of the interfaces
    for (uInterface = 0; uInterface < pRequest->pService->uNumInterfaces; uInterface++)
    {
        // send probe
        SockaddrInSetAddr(&SendAddr, pRequest->pService->aAddrs[uInterface]);
        SockaddrInSetPort(&SendAddr, pRequest->pService->aPorts[uInterface]);

        NetPrintf(("qosapi: sending probe to %a:%u\n", pRequest->pService->aAddrs[uInterface], pRequest->pService->aPorts[uInterface]));
        if ((iResult = SocketSendto(pQosApi->pSocket, (const char *)aPacketData, sizeof(aPacketData), 0, &SendAddr, sizeof(SendAddr))) != sizeof(aPacketData))
        {
            NetPrintf(("qosapi: failed to send firewall probe %d to address %d\n", pRequest->pService->uProbeSetsSent, uInterface));
        }
    }

    if (++pRequest->pService->uProbeSetsSent == QOSAPI_NUM_PROBESETS)
    {
        pRequest->pService->eServiceState = SS_FIREWALL_SENT;
        pRequest->uTimeIssued = NetTick();
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
    \Function _QosApiRecvRequest

    \Description
        Process a receive (listen) request.

    \Input *pQosApi     - module state
    \Input *pPacketData - received packet data
    \Input *pRecvAddr   - source of packet data

    \Version 08/15/2011 (jbrookes) refactored from _QosApiRecv()
*/
/********************************************************************************F*/
static void _QosApiRecvRequest(QosApiRefT *pQosApi, char *pPacketData, struct sockaddr *pRecvAddr)
{
    // incoming request, respond if listening
    if (!pQosApi->uListenSet)
    {
        return;
    }

    if (pQosApi->uResponseSize > 0)
    {
        pQosApi->QosListenStatus.uDataRequestsRecv += 1;
    }
    pQosApi->QosListenStatus.uProbeRequestsRecv += 1;

    *(int32_t *)&pPacketData[20] = SocketHtonl(SockaddrInGetAddr(pRecvAddr));
    *(int16_t *)&pPacketData[24] = SocketHtons(SockaddrInGetPort(pRecvAddr));
    *(int32_t *)&pPacketData[26] = SocketHtonl(pQosApi->uResponseSize);
    if (pQosApi->uResponseSize > 0)
    {
        memcpy(pPacketData+30, pQosApi->aResponse, pQosApi->uResponseSize);
    }

    SocketSendto(pQosApi->pSocket, pPacketData, 30 + pQosApi->uResponseSize, 0, pRecvAddr, sizeof(*pRecvAddr));

    if (pQosApi->uResponseSize > 0)
    {
        pQosApi->QosListenStatus.uDataRepliesSent++;
    }
    pQosApi->QosListenStatus.uProbeRepliesSent++;
}

/*F********************************************************************************/
/*!
    \Function _QosApiRecvResponse

    \Description
        Process a receive response.

    \Input *pQosApi     - module state
    \Input *pPacketData - received packet data
    \Input *pRecvAddr   - source of packet data
    \Input uRequestId   - response request id

    \Version 08/15/2011 (jbrookes) refactored from _QosApiRecv()
*/
/********************************************************************************F*/
static void _QosApiRecvResponse(QosApiRefT *pQosApi, char *pPacketData, struct sockaddr *pRecvAddr, uint32_t uRequestId)
{
    QosApiRequestT *pRequest = pQosApi->pRequestQueue;
    int32_t iDataLen, iPosition, iServiceReqSecret;
    uint32_t uSockTick, uServiceReqId, uFlags;

    // get request matching id
    if ((pRequest = _QosApiGetRequest(pQosApi, uRequestId)) == NULL)
    {
        NetPrintf(("qosapi: invalid qos packet received; ignoring\n"));
        if (pQosApi->uListenSet)
        {
            pQosApi->QosListenStatus.uRequestsDiscarded += 1;
        }
        return;
    }

    // make sure it's not failed or timed out
    if ((pRequest->pQosInfo->uFlags & QOSAPI_STATFL_FAILED) || (pRequest->pQosInfo->uFlags & QOSAPI_STATFL_TIMEDOUT))
    {
        NetPrintf(("qosapi: response received but ignored since the request failed or timed out\n"));
        return;
    }

    // get recv timestamp from sockaddr
    uSockTick = SockaddrInGetMisc(pRecvAddr);

    // extract data from packet
    uServiceReqId = SocketNtohl(*(int32_t *)&pPacketData[4]);
    iServiceReqSecret = SocketNtohl(*(int32_t *)&pPacketData[8]);
    uFlags = SocketNtohl(*(int32_t *)&pPacketData[12]);

    if (uServiceReqId < 2)
    {
        uint32_t uPacketRTT, uSentTick;

        // set up external address
        SockaddrInit(&pRequest->pQosInfo->ExternalAddr, AF_INET);
        SockaddrInSetAddr(&pRequest->pQosInfo->ExternalAddr, SocketNtohl(*(int32_t *)&pPacketData[20]));
        SockaddrInSetPort(&pRequest->pQosInfo->ExternalAddr, SocketNtohs(*(int16_t *)&pPacketData[24]));

        // calculate rtt
        uSentTick = SocketNtohl(*(int32_t *)&pPacketData[16]);
        uPacketRTT = uSockTick - uSentTick;

        // insert the RTT in the correct location in the list (insertion sort since the list is small)
        for (iPosition = pRequest->iValidRTTs++; (iPosition > 0) && (pRequest->aProbeRTTs[iPosition-1] > uPacketRTT); iPosition -= 1)
        {
            pRequest->aProbeRTTs[iPosition] = pRequest->aProbeRTTs[iPosition-1];
        }
        pRequest->aProbeRTTs[iPosition] = uPacketRTT;

        // set the receive address
        pRequest->pQosInfo->uAddr = SockaddrInGetAddr(pRecvAddr);

        // set the minimum and medium RTT values
        pRequest->pQosInfo->iMinRTT = pRequest->aProbeRTTs[0];
        pRequest->pQosInfo->iMedRTT = pRequest->aProbeRTTs[pRequest->iValidRTTs/2];

        // check for title specific data
        iDataLen = SocketNtohl(*(int32_t *)&pPacketData[26]);
        if ((pRequest->pQosInfo->iDataLen == 0) && ((iDataLen > 0) && (iDataLen <= QOSAPI_RESPONSE_MAXSIZE)))
        {
            memcpy(pRequest->pQosInfo->aData, pPacketData+30, iDataLen);
            pRequest->pQosInfo->iDataLen = iDataLen;

            // set the data received flag
            pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_DATA_RECEIVED;
        }

        pRequest->pQosInfo->iProbesRecv += 1;
        pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_CONTACTED;

        // set the flag values correctly
        if (uFlags & QOSAPI_REQUESTFL_LATENCY)
        {
            if (pRequest->pQosInfo->iProbesRecv == (int32_t)(pRequest->uNumProbes / 4))
            {
                pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_PART_COMPLETE;
            }
            if (pRequest->pQosInfo->iProbesRecv == (int32_t)pRequest->uNumProbes)
            {
                pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_COMPLETE;
                pRequest->pQosInfo->iProbesSent += pRequest->uNumProbesReissued;
            }
        }
        else if (pRequest->pQosInfo->iProbesRecv == (int32_t)pRequest->uNumProbes)
        {
            pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_PART_COMPLETE;
            pRequest->pQosInfo->iProbesSent += pRequest->uNumProbesReissued;
            // continue to bandwidth test
            pRequest->pService->eServiceState = SS_BANDWIDTH_HTTP;
            // un-issue the request since its the same request to be used.
            pRequest->iIssued = 0;
        }
    }
    else if ((pRequest->pService->uServiceReqId == uServiceReqId) && (pRequest->pService->iServiceReqSecret == iServiceReqSecret))
    {
        if (pRequest->pService->uBandwidthRecv++ == 0)
        {
            pRequest->pService->uBwdRecvTime = uSockTick;
            pRequest->pQosInfo->uUpBitsPerSec = SocketNtohl(*(int32_t *)&pPacketData[20]);
        }
        else
        {
            pRequest->pQosInfo->uDwnBitsPerSec = (8 * 1000 * pRequest->pService->uBandwidth * pRequest->pService->uBandwidthRecv);
            if (uSockTick != pRequest->pService->uBwdRecvTime)
            {
                pRequest->pQosInfo->uDwnBitsPerSec /= (uSockTick - pRequest->pService->uBwdRecvTime);
            }
        }

        // if we received the final bandwidth probe we're done
        if (SocketNtohl(*(int32_t *)&pPacketData[12]) == pRequest->pService->uBandwidthTotal)
        {
            if (pQosApi->eQosApiNatType != QOSAPI_NAT_PENDING)
            {
                pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_COMPLETE;
            }
            else
            {
                pRequest->pService->eServiceState = SS_FIREWALL_HTTP;
                pRequest->iIssued = 0;
            }
        }
    }
    else
    {
        // ignored since the service request id's don't match up
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiRecv

    \Description
        Process QosApi socket data.

    \Input *pSocket - pointer to module socket
    \Input *pData   - pointer to module state

    \Output int32_t - zero

    \Notes
        The call to this function should be guarded by pQosApi->ThreadCrit when in async-recv mode.

    \Version 08/08/2011 (szhu)
*/
/********************************************************************************F*/
static int32_t _QosApiRecv(SocketT *pSocket, void *pData)
{
    QosApiRefT *pQosApi = (QosApiRefT *)pData;
    struct sockaddr RecvAddr;
    char aPacketData[QOSAPI_MAXPACKET];
    int32_t iAddrLen = sizeof(struct sockaddr), iRecvLen;

    while ((iRecvLen = SocketRecvfrom(pQosApi->pSocket, aPacketData, sizeof(aPacketData), 0, &RecvAddr, &iAddrLen)) > 0)
    {
        uint32_t uRequestId = SocketNtohl(*(int32_t *)&aPacketData);

        // check for invalid qos packet
        if (uRequestId == 0)
        {
            NetPrintf(("qosapi: invalid qos packet received; ignoring\n"));
            if (pQosApi->uListenSet)
            {
                pQosApi->QosListenStatus.uRequestsDiscarded++;
            }
            continue;
        }

        // process packet
        if (iRecvLen == 20)
        {
            _QosApiRecvRequest(pQosApi, aPacketData, &RecvAddr);
        }
        else
        {
            _QosApiRecvResponse(pQosApi, aPacketData, &RecvAddr, uRequestId);
        }
    }

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

    \Output int32_t - zero

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
    QosApiCBInfoT CBInfo;
    int32_t iResult;

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

            NetCritEnter(&pQosApi->ThreadCrit);
            _QosApiRecv(pQosApi->pSocket, pQosApi);
            NetCritLeave(&pQosApi->ThreadCrit);
        }
        #else
        _QosApiRecv(pQosApi->pSocket, pQosApi);
        #endif
    }

    for (pRequest = pQosApi->pRequestQueue; pRequest != NULL; pRequest = pRequest->pNext)
    {
        if (pRequest->eRequestType == TP_SERVICE)
        {
            // see if this Service Request is stale and need reissuing
            _QosApiReissueServiceRequest(pQosApi, pRequest);
        }

        /* pRequest->iIssued can be modified in _QosApiRecv*: pRequest->iIssued = 0;
        but (so far) it's safe to read it and compare its value with 0 without mutex-ing
        TODO: refactor??? */
        if (pRequest->iIssued == 0)
        {
            if (pRequest->eRequestType == TP_CLIENT)
            {
                _QosApiSendLatencyProbe(pQosApi, pRequest);
            }
            else
            {
                if (pRequest->pService->uHttpPending)
                {
                    // make sure we own resources
                    NetCritEnter(&pQosApi->ThreadCrit);

                    ProtoHttpUpdate(pRequest->pService->pProtoHttp);

                    if ((iResult = ProtoHttpRecvAll(pRequest->pService->pProtoHttp, (char *)pRequest->pService->aRecvBuf, sizeof(pRequest->pService->aRecvBuf))) >= 0)
                    {
                        pRequest->pService->uHttpPending = FALSE;

                        // parse the http response
                        if (_QosApiParseHttpResponse(pQosApi, pRequest) < 0)
                        {
                            pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_FAILED;
                            pRequest->iIssued = 1;
                        }
                        else if (pRequest->pService->eServiceState == SS_PROTONAME)
                        {
                            pRequest->pService->eServiceState = SS_LATENCY;
                        }
                        else if (pRequest->pService->eServiceState == SS_BANDWIDTH_HTTP)
                        {
                            pRequest->pService->eServiceState = SS_BANDWIDTH_START;
                        }
                        else if (pRequest->pService->eServiceState == SS_FIREWALL_HTTP)
                        {
                            pRequest->pService->eServiceState = SS_FIREWALL_START;
                        }
                        else if ((pRequest->pService->eServiceState == SS_FIREWALL_START) || (pRequest->pService->eServiceState == SS_FIREWALL_SENT))
                        {
                            pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_COMPLETE;
                            pRequest->iIssued = 1;
                        }
                    }
                    else if ((iResult < 0) && (iResult != PROTOHTTP_RECVWAIT))
                    {
                        pRequest->pService->uHttpPending = FALSE;
                        pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_FAILED;
                        pRequest->iIssued = 1;
                    }
                    else if (pRequest->pService->eServiceState == SS_FIREWALL_START)
                    {
                        _QosApiSendFirewallProbeSet(pQosApi, pRequest);
                    }

                    // release resources
                    NetCritLeave(&pQosApi->ThreadCrit);
                }
                else
                {
                    if (pRequest->pService->eServiceState == SS_PROTONAME)
                    {
                        int32_t iAddress = pRequest->pService->pTarget->Done(pRequest->pService->pTarget);

                        if (iAddress == -1)
                        {
                            pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_FAILED;
                            pRequest->iIssued = 1;
                        }
                        else if (iAddress != 0)
                        {
                            char strUrl[256];
                            char strAddrText[20];

                            pRequest->uTargetAddr = pRequest->pService->pTarget->addr;

                            // save the service address
                            strnzcpy(pRequest->pService->strServiceUrl, SocketInAddrGetText(pRequest->uTargetAddr, strAddrText, sizeof(strAddrText)), sizeof(pRequest->pService->strServiceUrl));

                            // format the URL
                            _QosApiFormatServiceUrl(pQosApi, pRequest, strUrl, sizeof(strUrl), QOSAPI_BLAZE_LATENCY);

                            // issue the request
                            ProtoHttpGet(pRequest->pService->pProtoHttp, strUrl, FALSE);
                            pRequest->pService->uHttpPending = TRUE;
                        }
                    }
                    else if (pRequest->pService->eServiceState == SS_LATENCY)
                    {
                        _QosApiSendLatencyProbe(pQosApi, pRequest);
                    }
                    else if (pRequest->pService->eServiceState == SS_BANDWIDTH_HTTP)
                    {
                        char strUrl[256];

                        // make sure we own resources
                        NetCritEnter(&pQosApi->ThreadCrit);

                        // format the URL
                        _QosApiFormatServiceUrl(pQosApi, pRequest, strUrl, sizeof(strUrl), QOSAPI_BLAZE_BANDWIDTH);

                        // issue the request
                        ProtoHttpGet(pRequest->pService->pProtoHttp, strUrl, FALSE);
                        pRequest->pService->uHttpPending = TRUE;

                        // release resources
                        NetCritLeave(&pQosApi->ThreadCrit);
                    }
                    else if (pRequest->pService->eServiceState == SS_BANDWIDTH_START)
                    {
                        uint32_t uBandwidth;

                        // make sure we own resources
                        NetCritEnter(&pQosApi->ThreadCrit);

                        // send bandwidth probes
                        for (uBandwidth = 0; uBandwidth < pRequest->pService->uBandwidthTotal; uBandwidth += 1)
                        {
                            _QosApiSendBandwidthProbe(pQosApi, pRequest);
                        }

                        pRequest->pService->eServiceState = SS_BANDWIDTH_SENT;
                        pRequest->iIssued = 1;

                        // release resources
                        NetCritLeave(&pQosApi->ThreadCrit);
                    }
                    else if (pRequest->pService->eServiceState == SS_FIREWALL_HTTP)
                    {
                        char strUrl[256];

                        // make sure we own resources
                        NetCritEnter(&pQosApi->ThreadCrit);

                        // format the URL
                        _QosApiFormatServiceUrl(pQosApi, pRequest, strUrl, sizeof(strUrl), QOSAPI_BLAZE_FIREWALL);

                        // issue the request
                        ProtoHttpGet(pRequest->pService->pProtoHttp, strUrl, FALSE);
                        pRequest->pService->uHttpPending = TRUE;

                        // release resources
                        NetCritLeave(&pQosApi->ThreadCrit);
                    }
                    else if (pRequest->pService->eServiceState == SS_FIREWALL_START)
                    {
                        char strUrl[256];

                        // make sure we own resources
                        NetCritEnter(&pQosApi->ThreadCrit);

                        // format the URL
                        _QosApiFormatServiceUrl(pQosApi, pRequest, strUrl, sizeof(strUrl), QOSAPI_BLAZE_FIRETYPE);

                        // issue the request
                        ProtoHttpGet(pRequest->pService->pProtoHttp, strUrl, FALSE);
                        pRequest->pService->uHttpPending = TRUE;

                        // release resources
                        NetCritLeave(&pQosApi->ThreadCrit);
                    }
                }
            }
        }

        // make sure we own resources
        NetCritEnter(&pQosApi->ThreadCrit);

        if ((!((pRequest->pQosInfo->uFlags & QOSAPI_STATFL_FAILED) ||
               (pRequest->pQosInfo->uFlags & QOSAPI_STATFL_TIMEDOUT) ||
               (pRequest->pQosInfo->uFlags & QOSAPI_STATFL_COMPLETE))) &&
                (NetTickDiff(NetTick(), pRequest->pQosInfo->uWhenRequested) > (int32_t)pRequest->uTimeout))
        {
            pRequest->pQosInfo->uFlags |= QOSAPI_STATFL_TIMEDOUT;
            pRequest->iIssued = 1;
            pRequest->uTimeIssued = 0;
            pRequest->pQosInfo->iProbesSent += pRequest->uNumProbesReissued;
        }

        if (pRequest->uStatus != pRequest->pQosInfo->uFlags)
        {
            QosInfoT QosInfo;

            memcpy(&QosInfo, pRequest->pQosInfo, sizeof(QosInfo));

            CBInfo.uOldStatus = pRequest->uStatus;
            CBInfo.uNewStatus = pRequest->pQosInfo->uFlags;
            CBInfo.eQosApiNatType = pQosApi->eQosApiNatType;
            pRequest->uStatus = pRequest->pQosInfo->uFlags;

            CBInfo.pQosInfo = &QosInfo;

            // release resources
            NetCritLeave(&pQosApi->ThreadCrit);

            // display request result info
            NetPrintf(("qosapi: [%d] received response flags=0x%08x\n", CBInfo.pQosInfo->uRequestId, CBInfo.pQosInfo->uFlags));
            if ((CBInfo.pQosInfo->uFlags & (QOSAPI_STATFL_COMPLETE|QOSAPI_STATFL_TIMEDOUT|QOSAPI_STATFL_FAILED)) != 0)
            {
                NetPrintf(("qosapi: when=%u\n", CBInfo.pQosInfo->uWhenRequested));
                if (!(CBInfo.pQosInfo->uFlags & QOSAPI_STATFL_FAILED))
                {
                    NetPrintf(("qosapi: sent/recv=(%d/%d)\n", CBInfo.pQosInfo->iProbesSent, CBInfo.pQosInfo->iProbesRecv));
                    NetPrintf(("qosapi: rtt (min/med)=(%d,%d)\n", CBInfo.pQosInfo->iMinRTT, CBInfo.pQosInfo->iMedRTT));
                    NetPrintf(("qosapi: bps (up/down)=(%d,%d)\n", CBInfo.pQosInfo->uUpBitsPerSec, CBInfo.pQosInfo->uDwnBitsPerSec));
                    NetPrintf(("qosapi: external address=%a:%d\n", SockaddrInGetAddr(&CBInfo.pQosInfo->ExternalAddr),
                        SockaddrInGetPort(&CBInfo.pQosInfo->ExternalAddr)));
                    if (CBInfo.pQosInfo->iDataLen > 0)
                    {
                        NetPrintf(("qosapi: datalen=%d\n", CBInfo.pQosInfo->iDataLen));
                        NetPrintMem(CBInfo.pQosInfo->aData, CBInfo.pQosInfo->iDataLen, "qosdata");
                    }
                }
            }

            pQosApi->pCallback(pQosApi, &CBInfo, QOSAPI_CBTYPE_STATUS, pQosApi->pUserData);
        }
        else
        {
            // release resources
            NetCritLeave(&pQosApi->ThreadCrit);
        }
    }
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

    \Output QosApiRefT * - state pointer

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

    // set the current request id to 2
    pQosApi->uCurrentId = 2;

    // set user callback
    pQosApi->pCallback = (pCallback == NULL) ? _QosApiDefaultCallback : pCallback;
    pQosApi->pUserData = pUserData;

    // set the service port
    pQosApi->uServicePort = iServicePort;

    // set the current timeout
    pQosApi->uCurrentTimeout = QOSAPI_MINIMUM_TIMEOUT;

    // set the current rate for re-issuing requests
    pQosApi->uCurrentReissueRate = QOSAPI_DEFAULT_REISSUE_RATE;

    // install idle callback
    NetConnIdleAdd(_QosApiUpdate, pQosApi);

    // return module state to caller
    return(pQosApi);
}

/*F********************************************************************************/
/*!
    \Function QosApiListen

    \Description
        Start listening for Quality of service probes.

    \Input *pQosApi         - module state
    \Input *pResponse       - title specific data to send with any QoS probe responses
    \Input uResponseSize    - size of the title specific data
    \Input uBitsPerSecond   - bits per second to send responses out
    \Input uFlags           - QOSAPI_LISTENFL_*

    \Output 0 if successful, negative if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiListen(QosApiRefT *pQosApi, uint8_t *pResponse, uint32_t uResponseSize, uint32_t uBitsPerSecond, uint32_t uFlags)
{
    int32_t iResult;

    if (uFlags == 0)
    {
        uFlags = QOSAPI_LISTENFL_ENABLE | QOSAPI_LISTENFL_SET_DATA | QOSAPI_LISTENFL_SET_BPS;
    }

    if (uFlags & QOSAPI_LISTENFL_ENABLE)
    {
        // if this is the first listen call create the socket and bind
        if (pQosApi->pSocket == NULL)
        {
            struct sockaddr LocalAddr;

            // create the socket
            if ((pQosApi->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0)) == NULL)
            {
                NetPrintf(("qosapi: could not allocate socket\n"));
                return(-1);
            }

            // set the port to the default value if 0
            if (pQosApi->uListenPort == 0)
            {
                pQosApi->uListenPort = QOSAPI_DEFAULT_LISTENPORT;
            }

            SockaddrInit(&LocalAddr, AF_INET);
            SockaddrInSetPort(&LocalAddr, pQosApi->uListenPort);

            // bind the socket
            if ((iResult = SocketBind(pQosApi->pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
            {
                NetPrintf(("qosapi: error %d binding socket to port %d, trying random\n", iResult, pQosApi->uListenPort));
                SockaddrInSetPort(&LocalAddr, 0);
                if ((iResult = SocketBind(pQosApi->pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
                {
                    NetPrintf(("qosapi: error %d binding socket to listen\n", iResult));
                    SocketClose(pQosApi->pSocket);
                    pQosApi->pSocket = NULL;
                    return(0);
                }
            }

            // set the current listen port
            SocketInfo(pQosApi->pSocket, 'bind', 0, &LocalAddr, sizeof(LocalAddr));
            pQosApi->uCurrentListenPort = SockaddrInGetPort(&LocalAddr);
            NetPrintf(("qosapi: bound socket to port %u\n", pQosApi->uCurrentListenPort));

            // need to sync access to data
            NetCritInit(&pQosApi->ThreadCrit, "qosapi");

            // set the callback
            SocketCallback(pQosApi->pSocket, CALLB_RECV, QOSAPI_SOCKET_IDLE_RATE, pQosApi, &_QosApiRecvCB);
        }

        pQosApi->uListenSet = 1;

        NetPrintf(("qosapi: listen enabled\n"));
    }

    if (uFlags & QOSAPI_LISTENFL_DISABLE)
    {
        pQosApi->uListenSet = 0;

        NetPrintf(("qosapi: listen disabled\n"));
    }

    // set aResponse if required
    if (uFlags & QOSAPI_LISTENFL_SET_DATA)
    {
        // make sure we own resources
        NetCritEnter(&pQosApi->ThreadCrit);
        memset(pQosApi->aResponse, 0, sizeof(pQosApi->aResponse));
        pQosApi->uResponseSize = 0;
        if (pResponse != NULL)
        {
            if (uResponseSize > QOSAPI_RESPONSE_MAXSIZE)
            {
                uResponseSize = QOSAPI_RESPONSE_MAXSIZE;
            }
            memcpy(pQosApi->aResponse, pResponse, uResponseSize);
            pQosApi->uResponseSize = uResponseSize;
        }
        // release resources
        NetCritLeave(&pQosApi->ThreadCrit);

        NetPrintMem(pQosApi->aResponse, uResponseSize, "qosapi: listen data set to");
    }

    // set bit rate if required
    if (uFlags & QOSAPI_LISTENFL_SET_BPS)
    {
        NetPrintf(("qosapi: QOSAPI_LISTENFL_SET_BPS unsupported\n"));
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function QosApiRequest

    \Description
        Send a QoS request to the specified host.

    \Input *pQosApi    - module state
    \Input *pAddr      - DirtyAddr specifying the host to send the request to
    \Input uNumProbes  - number of probes to send out for the request
    \Input uBitsPerSec - bits per second to send the request at
    \Input uFlags      - QOSAPI_REQUESTFL_*

    \Output uint32_t - request id, 0 if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
uint32_t QosApiRequest(QosApiRefT *pQosApi, DirtyAddrT *pAddr, uint32_t uNumProbes, uint32_t uBitsPerSec, uint32_t uFlags)
{
    QosApiRequestT *pRequest;
    int32_t iResult;

    // if the module's socket hasn't yet been allocated
    if (pQosApi->pSocket == NULL)
    {
        struct sockaddr LocalAddr;

        // create the socket
        if ((pQosApi->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0)) == NULL)
        {
            NetPrintf(("qosapi: could not allocate socket\n"));
            return(0);
        }

        // set the port to the default value if 0
        if (pQosApi->uListenPort == 0)
        {
            pQosApi->uListenPort = QOSAPI_DEFAULT_LISTENPORT;
        }

        SockaddrInit(&LocalAddr, AF_INET);
        SockaddrInSetPort(&LocalAddr, pQosApi->uListenPort);

        // bind the socket
        if ((iResult = SocketBind(pQosApi->pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
        {
            NetPrintf(("qosapi: error %d binding socket to port %u, trying random\n", iResult, pQosApi->uListenPort));
            SockaddrInSetPort(&LocalAddr, 0);
            if ((iResult = SocketBind(pQosApi->pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
            {
                NetPrintf(("qosapi: error %d binding socket to listen\n", iResult));
                SocketClose(pQosApi->pSocket);
                pQosApi->pSocket = NULL;
                return(0);
            }
        }

        // set the current listen port
        SocketInfo(pQosApi->pSocket, 'bind', 0, &LocalAddr, sizeof(LocalAddr));
        pQosApi->uCurrentListenPort = SockaddrInGetPort(&LocalAddr);
        NetPrintf(("qosapi: bound socket to port %u\n", pQosApi->uCurrentListenPort));

        // need to sync access to data
        NetCritInit(&pQosApi->ThreadCrit, "qosapi");

        // set the callback
        SocketCallback(pQosApi->pSocket, CALLB_RECV, QOSAPI_SOCKET_IDLE_RATE, pQosApi, &_QosApiRecvCB);
    }

    // allocate the request
    if ((pRequest = _QosApiCreateRequest(pQosApi, TP_CLIENT)) == 0)
    {
        NetPrintf(("qosapi: unable to create the request\n"));
        return(0);
    }
    // only service requests should have bandwidth measurements so force it to latency only
    pRequest->uFlags = uFlags | QOSAPI_REQUESTFL_LATENCY;

    // set the number of probes to the minimum/maximum if required
    if (uNumProbes < QOSAPI_MINPROBES)
    {
        uNumProbes = QOSAPI_MINPROBES;
    }
    if (uNumProbes > QOSAPI_MAXPROBES)
    {
        uNumProbes = QOSAPI_MAXPROBES;
    }

    // set uBitsPerSec to the default value if 0 was passed in
    if (uBitsPerSec == 0)
    {
        uBitsPerSec = QOSAPI_DEFAULT_BITSPERSEC;
    }

    // convert opaque address type to 32bit address type
    DirtyAddrToHostAddr(&pRequest->uTargetAddr, sizeof(pRequest->uTargetAddr), pAddr);

    // set the request port to the listen port
    pRequest->uTargetPort = pQosApi->uListenPort;

    // save the number of probes to send and the bit rate the request should use
    pRequest->uNumProbes = uNumProbes;
    pRequest->uBitsPerSec = uBitsPerSec;

    // set the request timeout
    pRequest->uTimeout = pQosApi->uCurrentTimeout;

    // set the request time
    pRequest->pQosInfo->uWhenRequested = NetTick();

    return(pRequest->pQosInfo->uRequestId);
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
    \Input uBitsPerSec      - bits per second to send the request at
    \Input uFlags           - QOSAPI_REQUESTFL_*

    \Output uint32_t        - request id, 0 if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
uint32_t QosApiServiceRequest(QosApiRefT *pQosApi, char *pServerAddress, uint16_t uProbePort, uint32_t uServiceId, uint32_t uNumProbes, uint32_t uBitsPerSec, uint32_t uFlags)
{
    QosApiRequestT *pRequest;
    int32_t iResult;

    // if the module's socket hasn't yet been allocated
    if (pQosApi->pSocket == NULL)
    {
        struct sockaddr LocalAddr;

        // create the socket
        if ((pQosApi->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, 0)) == NULL)
        {
            NetPrintf(("qosapi: could not allocate socket\n"));
            return(0);
        }

        // set the port to the default value if 0
        if (pQosApi->uListenPort == 0)
        {
            pQosApi->uListenPort = QOSAPI_DEFAULT_LISTENPORT;
        }

        SockaddrInit(&LocalAddr, AF_INET);
        SockaddrInSetPort(&LocalAddr, pQosApi->uListenPort);

        // bind the socket
        if ((iResult = SocketBind(pQosApi->pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
        {
            NetPrintf(("qosapi: error %d binding socket to port %d, trying random\n", iResult, pQosApi->uListenPort));
            SockaddrInSetPort(&LocalAddr, 0);
            if ((iResult = SocketBind(pQosApi->pSocket, &LocalAddr, sizeof(LocalAddr))) != SOCKERR_NONE)
            {
                NetPrintf(("qosapi: error %d binding socket to listen\n", iResult));
                SocketClose(pQosApi->pSocket);
                pQosApi->pSocket = NULL;
                return(0);
            }
        }

        // set the current listen port
        SocketInfo(pQosApi->pSocket, 'bind', 0, &LocalAddr, sizeof(LocalAddr));
        pQosApi->uCurrentListenPort = SockaddrInGetPort(&LocalAddr);
        NetPrintf(("qosapi: bound socket to port %u\n", pQosApi->uCurrentListenPort));

        // need to sync access to data
        NetCritInit(&pQosApi->ThreadCrit, "qosapi");

        // set the callback
        SocketCallback(pQosApi->pSocket, CALLB_RECV, QOSAPI_SOCKET_IDLE_RATE, pQosApi, &_QosApiRecvCB);
    }

    // allocate the request
    if ((pRequest = _QosApiCreateRequest(pQosApi, TP_SERVICE)) == 0)
    {
        NetPrintf(("qosapi: unable to create the request\n"));
        return(0);
    }
    pRequest->uFlags = uFlags;

    // set the number of probes to the minimum/maximum if required
    if (uNumProbes < QOSAPI_MINPROBES)
    {
        uNumProbes = QOSAPI_MINPROBES;
    }
    if (uNumProbes > QOSAPI_MAXPROBES)
    {
        uNumProbes = QOSAPI_MAXPROBES;
    }

    // set uBitsPerSec to the default value if 0 was passed in
    if (uBitsPerSec == 0)
    {
        uBitsPerSec = QOSAPI_DEFAULT_BITSPERSEC;
    }

    // save the number of probes sent and the bit rate of the request
    pRequest->uNumProbes = uNumProbes;
    pRequest->uBitsPerSec = uBitsPerSec;

    // set the target address of the request
    pRequest->pService->pTarget = ProtoNameAsync(pServerAddress, 5000);
    pRequest->pService->uProbePort = uProbePort;

    // set the service state
    pRequest->pService->eServiceState = SS_PROTONAME;

    // set the request timeout
    pRequest->uTimeout = pQosApi->uCurrentTimeout;

    // set the request time
    pRequest->pQosInfo->uWhenRequested = NetTick();

    return(pRequest->pQosInfo->uRequestId);
}

/*F********************************************************************************/
/*!
    \Function QosApiCancelRequest

    \Description
        Cancel the specified QoS request.

    \Input *pQosApi   - module state
    \Input uRequestId - request to cancel

    \Output 0 if successful, negative if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiCancelRequest(QosApiRefT *pQosApi, uint32_t uRequestId)
{
    if (_QosApiDestroyRequest(pQosApi, uRequestId) != 0)
    {
        NetPrintf(("qosapi: the specified request does not exist; no requests cancelled\n"));
        return(-1);
    }

    return(0);
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

    \Output int32_t - control specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'cbfp' - set callback function pointer - pValue=pCallback
            'lena' - Enable listening for QoS requests
            'ldis' - Disable listening for QoS requests
            'lprt' - Set the port to use for the QoS listen port (must be set before calling listen/request)
            'sdat' - Set response data when responding to a latency request
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
    if (iControl == 'cbfp')
    {
        // set callback function pointer
        pQosApi->pCallback = ((pValue != NULL) ? (QosApiCallbackT *)pValue : _QosApiDefaultCallback);
        return(0);
    }
    if (iControl == 'lena')
    {
        if (pQosApi->pSocket != NULL)
        {
            pQosApi->uListenSet = 1;
            return(0);
        }

        return(-2);
    }
    if (iControl == 'ldis')
    {
        pQosApi->uListenSet = 0;
        return(0);
    }
    if (iControl == 'lprt')
    {
        pQosApi->uListenPort = iValue;
        return(0);
    }
    if (iControl == 'sdat')
    {
        // make sure we own resources
        NetCritEnter(&pQosApi->ThreadCrit);
        memset(pQosApi->aResponse, 0, sizeof(pQosApi->aResponse));
        pQosApi->uResponseSize = 0;
        if (pValue != NULL)
        {
            if (iValue > QOSAPI_RESPONSE_MAXSIZE)
            {
                iValue = QOSAPI_RESPONSE_MAXSIZE;
            }
            memcpy(pQosApi->aResponse, pValue, iValue);
            pQosApi->uResponseSize = (uint32_t)iValue;
        }
        // release resources
        NetCritLeave(&pQosApi->ThreadCrit);

        return(0);
    }
    if (iControl == 'sbps')
    {
        return(0);
    }
    if (iControl == 'spam')
    {
        pQosApi->iSpamValue = iValue;
        return(0);
    }
    if (iControl == 'sprt')
    {
        pQosApi->uServicePort = iValue;
        return(0);
    }
    if (iControl == 'time')
    {
        pQosApi->uCurrentTimeout = (iValue < QOSAPI_MINIMUM_TIMEOUT) ? QOSAPI_MINIMUM_TIMEOUT : iValue;
        return(0);
    }
    if (iControl == 'rira')
    {
        pQosApi->uCurrentReissueRate = (iValue < QOSAPI_MINIMUM_REISSUE_RATE) ? QOSAPI_MINIMUM_REISSUE_RATE : iValue;
        return(0);
    }
    if (iControl == 'uidx')
    {
        pQosApi->uUserIdx = iValue;
        return(0);
    }
    return(-1);
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

    \Output int32_t - selector specific

    \Notes
        iSelect can be one of the following:

        \verbatim
            'sess' - Get the session information
            'list' - Get the current listen status for this client
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

    if (iSelect == 'sess')
    {
        return(0);
    }
    if ((iSelect == 'list') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(QosListenStatusT)))
    {
        if ((pQosApiRequest = _QosApiGetRequest(pQosApi, (unsigned)iData)) != NULL)
        {
            // safe copy of listen status
            NetCritEnter(&pQosApi->ThreadCrit);
            memcpy(pBuf, &pQosApi->QosListenStatus, sizeof(QosListenStatusT));
            NetCritLeave(&pQosApi->ThreadCrit);
            return(0);
        }
        return(-2);
    }
    if ((iSelect == 'requ') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(QosInfoT)))
    {
        if ((pQosApiRequest = _QosApiGetRequest(pQosApi, (unsigned)iData)) != NULL)
        {
            // safe copy of qos info
            NetCritEnter(&pQosApi->ThreadCrit);
            memcpy(pBuf, pQosApiRequest->pQosInfo, sizeof(QosInfoT));
            NetCritLeave(&pQosApi->ThreadCrit);
            return(0);
        }
        return(-2);
    }
    if ((iSelect == 'extn') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(struct sockaddr)))
    {
        if ((pQosApiRequest = _QosApiGetRequest(pQosApi, (unsigned)iData)) != NULL)
        {
            // safe copy of external address
            NetCritEnter(&pQosApi->ThreadCrit);
            memcpy(pBuf, &pQosApiRequest->pQosInfo->ExternalAddr, sizeof(struct sockaddr));
            NetCritLeave(&pQosApi->ThreadCrit);
            return(0);
        }
        return(-2);
    }
    if (iSelect == 'clpt')
    {
        return(pQosApi->uCurrentListenPort);
    }
    if (iSelect == 'time')
    {
        return(pQosApi->uCurrentTimeout);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function QosApiGetNatType

    \Description
        Returns the current NAT information of the client.

    \Input *pQosApi - module state

    \Output QosApiNatTypeE - nat type.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
QosApiNatTypeE QosApiGetNatType(QosApiRefT *pQosApi)
{
    return(pQosApi->eQosApiNatType);
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
    int32_t iCritKill = 0;

    NetConnIdleDel(_QosApiUpdate, pQosApi);

    if (pQosApi->pSocket != NULL)
    {
        SocketClose(pQosApi->pSocket);
        pQosApi->pSocket = NULL;
        iCritKill = 1;
    }

    while (pQosApi->pRequestQueue != NULL)
    {
        _QosApiDestroyRequest(pQosApi, pQosApi->pRequestQueue->pQosInfo->uRequestId);
    }

    if (iCritKill)
    {
        NetCritKill(&pQosApi->ThreadCrit);
    }

    // release module memory
    DirtyMemFree(pQosApi, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
}

