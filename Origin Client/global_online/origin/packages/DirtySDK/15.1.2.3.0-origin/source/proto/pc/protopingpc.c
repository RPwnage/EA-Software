/*H********************************************************************************/
/*!
    \File protopingpc.c

    \Description
        This module implements a DirtySock ping routine using the Windows API
        ICMPSendEcho2.  It can both generate ping requests and record their
        responses.  Use of the ICMPSendEcho2 API means an application does not
        require administrative privileges to operate, however it does limit the
        module to using ICMP only, as opposed to the generic version using RAW
        sockets.

    \Copyright
        Copyright (c) Electronic Arts 2014.

    \Version 07/14/2014 (jbrookes) First Version, heavily based on contributions from BFBC2
*/
/********************************************************************************H*/


/*** Include files ****************************************************************/

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <icmpapi.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/misc/qosapi.h"

#include "DirtySDK/proto/protoping.h"

/*** Defines **********************************************************************/

#define PROTOPING_VERBOSE           (DIRTYCODE_DEBUG && FALSE)

#define PROTOPING_FLAG_RECEIVED     (1)         //!< indicates a response to request has been received
#define PROTOPING_FLAG_SERVER       (2)         //!< indicates request was a server ping

#define PROTOPING_DEFAULT_TTL       (64)        //!< default ttl is 64 hops
#define PROTOPING_DEFAULT_TIMEOUT   (5 * 1000)  //!< default timeout is five seconds
#define PROTOPING_DEFAULT_MAXPINGS  (32)        //!< maximum number of pings that can be queued

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! user data sent in ping
typedef struct EchoDataT
{
    char userdata[PROTOPING_MAXUSERDATA];    //!< user data
} EchoDataT;

typedef struct ProtoPingIcmpResponseT
{
    ICMP_ECHO_REPLY echoReply;
    EchoDataT data;
    char icmpError[8];
    char strBuffer[32];     //!< extra buffer required for correct functioning under Windows XP
} ProtoPingIcmpResponseT;

//! packet queue entry
typedef struct ProtoPingQueueT
{
    uint32_t uAddr;     //!< address pinged/responding address
    uint32_t uTime;     //!< timeout, in milliseconds
    uint32_t uTick;     //!< time received
    void    *pData;     //!< pointer to user data, or NULL if none
#if DIRTYCODE_LOGGING
    const char *pType;  //!< type name
#endif
    uint16_t uSize;     //!< size of user data, or zero if none
    uint16_t uSeqn;     //!< ping sequence number
    uint16_t uPing;     //!< ping time, or zero if timeout
    uint8_t  uType;     //!< ICMP type of response
    uint8_t  uFlag;     //!< flags
} ProtoPingQueueT;

//! module state
struct ProtoPingRefT
{
    HANDLE hIcmp;
    NetCritT ThreadCrit;        //!< critical section

    // module memory group
    int32_t iMemGroup;          //!< module mem group id
    void *pMemGroupUserData;    //!< user data associated with mem group

    ePingType eType;            //!< current ping type
    QosApiRefT *pQosApi;        //!< qosapi ref

    uint32_t uLocalAddr;        //!< local address
    int32_t iSeqn;              //!< current sequence number
    int32_t iRefCount;          //!< instance count
    int32_t iQueueSize;         //!< size of request queue
    int32_t iQueueTail;         //!< end of queue
    uint8_t bVerbose;           //!< verbose or not?
    uint8_t _pad[3];
    HANDLE *pEvents;
    ProtoPingIcmpResponseT *pResponses;
    ProtoPingIcmpResponseT **ppResponses;
    ProtoPingQueueT RequestQueue[1];  //!< variably-sized request queue (must come last)
};

/*** Function Prototypes **********************************************************/

/*** Variables ********************************************************************/

// Private variables

#if DIRTYCODE_LOGGING
static DirtyErrT _ProtoPing_ErrList[] =
{
    DIRTYSOCK_ErrorName(IP_SUCCESS),                // 0 - The status was success.
    DIRTYSOCK_ErrorName(IP_BUF_TOO_SMALL),          // 11001 - The reply buffer was too small.
    DIRTYSOCK_ErrorName(IP_DEST_NET_UNREACHABLE),   // 11002 - The destination network was unreachable.
    DIRTYSOCK_ErrorName(IP_DEST_HOST_UNREACHABLE),  // 11003 - The destination host was unreachable.
    DIRTYSOCK_ErrorName(IP_DEST_PROT_UNREACHABLE),  // 11004 - The destination protocol was unreachable.
    DIRTYSOCK_ErrorName(IP_DEST_PORT_UNREACHABLE),  // 11005 - The destination port was unreachable.
    DIRTYSOCK_ErrorName(IP_NO_RESOURCES),           // 11006 - Insufficient IP resources were available.
    DIRTYSOCK_ErrorName(IP_BAD_OPTION),             // 11007 - A bad IP option was specified.
    DIRTYSOCK_ErrorName(IP_HW_ERROR),               // 11008 - A hardware error occurred.
    DIRTYSOCK_ErrorName(IP_PACKET_TOO_BIG),         // 11009 - The packet was too big.
    DIRTYSOCK_ErrorName(IP_REQ_TIMED_OUT),          // 11010 - The request timed out.
    DIRTYSOCK_ErrorName(IP_BAD_REQ),                // 11011 - A bad request.
    DIRTYSOCK_ErrorName(IP_BAD_ROUTE),              // 11012 - A bad route.
    DIRTYSOCK_ErrorName(IP_TTL_EXPIRED_TRANSIT),    // 11013 - The time to live (TTL) expired in transit.
    DIRTYSOCK_ErrorName(IP_TTL_EXPIRED_REASSEM),    // 11014 - The time to live expired during fragment reassembly.
    DIRTYSOCK_ErrorName(IP_PARAM_PROBLEM),          // 11015 - A parameter problem.
    DIRTYSOCK_ErrorName(IP_SOURCE_QUENCH),          // 11016 - Datagrams are arriving too fast to be processed and datagrams may have been discarded.
    DIRTYSOCK_ErrorName(IP_OPTION_TOO_BIG),         // 11017 - An IP option was too big.
    DIRTYSOCK_ErrorName(IP_BAD_DESTINATION),        // 11018 - A bad destination.
    DIRTYSOCK_ErrorName(IP_GENERAL_FAILURE),        // 11050 - A general failure. This error can be returned for some malformed ICMP packets.
    // NULL terminate
    DIRTYSOCK_ListEnd()
};
#endif

static ProtoPingRefT *_ProtoPing_pRef = NULL;

// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoPingUpdate

    \Description
        Callback to poll for and process ICMP response

    \Input *pProtoPing  - module state

    \Version 07/14/2014 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoPingUpdate(ProtoPingRefT *pProtoPing)
{
    ProtoPingQueueT *pRequest;
    const ProtoPingIcmpResponseT *pResponse;
    int32_t iWaitCount, iPingRequest, iPingResponse, iUserDataSize;
    DWORD dwWaitResult;

    for (iPingRequest = 0; iPingRequest < pProtoPing->iQueueTail; iPingRequest += MAXIMUM_WAIT_OBJECTS)
    {
        iWaitCount = DS_MIN(pProtoPing->iQueueTail - iPingRequest, MAXIMUM_WAIT_OBJECTS);
        dwWaitResult = WaitForMultipleObjects(iWaitCount, pProtoPing->pEvents + iPingRequest, FALSE, 0);
        if (dwWaitResult < (WAIT_OBJECT_0 + iWaitCount))
        {
            iPingResponse = iPingRequest + (dwWaitResult - WAIT_OBJECT_0);
            pRequest = &pProtoPing->RequestQueue[iPingResponse];
            pResponse = pProtoPing->ppResponses[iPingResponse];
            ResetEvent(pProtoPing->pEvents[iPingResponse]);

            // try to parse the result
            if (pRequest->uFlag & PROTOPING_FLAG_RECEIVED)
            {
                NetPrintf(("protoping: got redundant response for sequence %d\n"));
                continue;
            }

            if ((pResponse->echoReply.Status != IP_SUCCESS) && (pResponse->echoReply.Status != IP_TTL_EXPIRED_TRANSIT))
            {
                if (pResponse->echoReply.Status != IP_REQ_TIMED_OUT)
                {
                    NetPrintf(("protoping: failed ping to %a (%s)\n", SocketNtohl(pResponse->echoReply.Address),
                        DirtyErrGetNameList(pResponse->echoReply.Status, _ProtoPing_ErrList)));
                }
                pRequest->uType = ICMP_ECHOREPLY;
                pRequest->uPing = 0xffff; // all failed requests are marked as timing out
                pRequest->uFlag |= PROTOPING_FLAG_RECEIVED;
                continue;
            }

            pRequest->uType = ICMP_ECHOREPLY;
            pRequest->uAddr = SocketNtohl(pResponse->echoReply.Address);
            pRequest->uFlag |= PROTOPING_FLAG_RECEIVED;
            pRequest->uPing = (uint16_t)pResponse->echoReply.RoundTripTime;
            if (pRequest->uPing == 0)
            {
                pRequest->uPing = 1;
            }

            iUserDataSize = (int32_t)(uint32_t)pResponse->echoReply.DataSize;
            iUserDataSize -= sizeof(pResponse->data);
            if (iUserDataSize > 0)
            {
                if (iUserDataSize > sizeof(pResponse->data.userdata))
                {
                    iUserDataSize = sizeof(pResponse->data.userdata);
                }

                NetPrintf(("protoping: got user data from %a, %d bytes\n", pRequest->uAddr, iUserDataSize));
                if ((pRequest->pData = (char *)DirtyMemAlloc(iUserDataSize, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData)) != NULL)
                {
                    pRequest->uSize = (uint16_t)iUserDataSize;
                    ds_memcpy(pRequest->pData, pResponse->data.userdata, pRequest->uSize);
                }
                else
                {
                    NetPrintf(("protoping: unable to allocate memory for ping response data\n"));
                }
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingQoSCallback

    \Description
        Callback to process QoS status changes

    \Input *pQosApi   - QosApiRefT pointer
    \Input *pCBInfo   - pointer to qosapi info
    \Input eCBType    - callback type
    \Input *pUserData - ProtoPingRefT pointer

    \Version 06/09/2009 (cadam)
*/
/********************************************************************************F*/
static void _ProtoPingQosCallback(QosApiRefT *pQosApi, QosApiCBInfoT *pCBInfo, QosApiCBTypeE eCBType, void *pUserData)
{
    int32_t iQueue;
    ProtoPingRefT *pProtoPing = (ProtoPingRefT *)pUserData;
    ProtoPingQueueT *pRequest;

    // make sure we own resources
    NetCritEnter(&pProtoPing->ThreadCrit);

    // find request in request queue
    for (iQueue = 0; iQueue < pProtoPing->iQueueTail; iQueue++)
    {
        pRequest = &pProtoPing->RequestQueue[iQueue];
        if (pRequest->uSeqn == pCBInfo->pQosInfo->uRequestId)
        {
            if (pRequest->uFlag & QOSAPI_STATFL_COMPLETE)
            {
                NetPrintf(("protoping: got redundant response for sequence %d\n"));
                break;
            }
            pRequest->uAddr = pCBInfo->pQosInfo->uAddr;
            pRequest->uPing = pCBInfo->pQosInfo->iMinRTT;
            if (pRequest->uPing == 0)
            {
                pRequest->uPing = 1;
            }
            if (pCBInfo->pQosInfo->uFlags & QOSAPI_STATFL_FAILED)
            {
                pRequest->uType = ICMP_DESTUNREACH;
            }
            else
            {
                pRequest->uType = ICMP_ECHOREPLY;
            }
            pRequest->uFlag |= pCBInfo->pQosInfo->uFlags;
            break;
        }
    }

    // unclaimed?
    #if DIRTYCODE_LOGGING
    if (iQueue == pProtoPing->iQueueTail)
    {
        NetPrintf(("protoping: received unclaimed probe from %a\n", pCBInfo->pQosInfo->uAddr));
    }
    else if (pProtoPing->bVerbose)
    {
        NetPrintf(("protoping: received probe from %a\n", pCBInfo->pQosInfo->uAddr));
    }
    #endif

    // release resources
    NetCritLeave(&pProtoPing->ThreadCrit);
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingRequest

    \Description
        Send a ping request,

    \Input *pProtoPing  - reference pointer
    \Input uAddress     - target address
    \Input *pData       - user data
    \Input iDataLen     - length of data
    \Input iTimeout     - timeout in ms, or zero for default
    \Input iTtl         - time to live, or zero for default
    \Input bServer      - TRUE if a server ping, else false

    \Output
        int32_t         - sequence number of packet (zero=failed)

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
static int32_t _ProtoPingRequest(ProtoPingRefT *pProtoPing, uint32_t uAddress, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl, uint32_t bServer)
{
    DWORD dwResult;
    EchoDataT RequestData = {0};
    IP_OPTION_INFORMATION ipOptInf;
    ProtoPingQueueT *pRequest;

    #if PROTOPING_VERBOSE
    NetPrintf(("protoping: pinging address %a\n", uAddress));
    #endif

    // add request queue entry
    if (pProtoPing->iQueueTail >= pProtoPing->iQueueSize)
    {
        NetPrintf(("protoping: request queue overflow\n"));
        return(-1);
    }

    if (pProtoPing->eType == PROTOPING_TYPE_ICMP)
    {
        // easy string handling
        if (iDataLen < 0)
        {
            iDataLen = (int32_t)strlen((const char *)pData)+1;
        }

        // handle defaults
        if ((iTtl <= 0) || (iTtl > 255))
        {
            iTtl = PROTOPING_DEFAULT_TTL;
        }
        if (iTimeout <= 0)
        {
            iTimeout = PROTOPING_DEFAULT_TIMEOUT;
        }

        // advance the sequence number and wrap at 16bits
        if (++pProtoPing->iSeqn > 65535)
        {
            pProtoPing->iSeqn = 1;
        }

        pRequest = &pProtoPing->RequestQueue[pProtoPing->iQueueTail];
        memset(pRequest, 0, sizeof(ProtoPingQueueT));

        // clamp the data size
        if (iDataLen > (int32_t)sizeof(RequestData.userdata))
        {
            NetPrintf(("protoping: clamping data (length=%d) to %d bytes\n", iDataLen, sizeof(RequestData.userdata)));
            iDataLen = sizeof(RequestData.userdata);
        }
        // copy over the data
        ds_memcpy_s(RequestData.userdata, sizeof(RequestData.userdata), pData, iDataLen);
        iDataLen += sizeof(RequestData) - sizeof(RequestData.userdata);

        // set option information
        memset(&ipOptInf, 0, sizeof(ipOptInf));
        ipOptInf.Ttl = iTtl;

        dwResult = IcmpSendEcho2(pProtoPing->hIcmp, pProtoPing->pEvents[pProtoPing->iQueueTail], NULL, NULL, htonl(uAddress), &RequestData, iDataLen, &ipOptInf,
            pProtoPing->ppResponses[pProtoPing->iQueueTail], sizeof(ProtoPingIcmpResponseT), (DWORD)iTimeout);
        if ((dwResult == 0) && (GetLastError() == ERROR_IO_PENDING))
        {
            pProtoPing->iQueueTail += 1;
            pRequest->uAddr = uAddress;
            pRequest->uTime = (uint32_t)iTimeout;
            pRequest->uSeqn = (uint16_t)pProtoPing->iSeqn;
            pRequest->uFlag = bServer ? PROTOPING_FLAG_SERVER : 0;
            pRequest->uTick = NetTick();
        }
        else
        {
            NetPrintf(("protoping: error (%d) sending icmp ping\n", GetLastError()));
            return(-1);
        }
    }
    else
    {
        if (iTimeout <= 0)
        {
            iTimeout = QosApiStatus(pProtoPing->pQosApi, 'time', 0, NULL, 0);
        }
        else
        {
            QosApiControl(pProtoPing->pQosApi, 'time', iTimeout, NULL);
            iTimeout = QosApiStatus(pProtoPing->pQosApi, 'time', 0, NULL, 0);
        }

        if (bServer)
        {
            char strAddrText[20];
            pProtoPing->iSeqn = QosApiServiceRequest(pProtoPing->pQosApi, SocketInAddrGetText(uAddress, strAddrText, sizeof(strAddrText)), 0, QOSAPI_REQUEST_LATENCY);
        }
        else
        {
            NetPrintf(("protoping: error client pings are unsported\n"));
            return(-1);
        }

        if (pProtoPing->iSeqn == 0)
        {
            NetPrintf(("protoping: error sending udp ping\n"));
            return(-1);
        }

        pRequest = &pProtoPing->RequestQueue[pProtoPing->iQueueTail++];
        memset(pRequest, 0, sizeof(*pRequest));
        pRequest->uAddr = uAddress;
        pRequest->uTime = (uint32_t)iTimeout;
        pRequest->uSeqn = (uint16_t)pProtoPing->iSeqn;
        pRequest->uFlag = PROTOPING_FLAG_SERVER;
        pRequest->uTick = NetTick();
    }

    // return sequence number
    return(pProtoPing->iSeqn);
}


/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoPingCreate

    \Description
        Create new ping module

    \Input iMaxPings    - maximum number of ping requests that can be issued at one time

    \Output
        ProtoPingRefT * - reference pointer (needed for all other calls)

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
ProtoPingRefT *ProtoPingCreate(int32_t iMaxPings)
{
    ProtoPingRefT *pProtoPing;
    int32_t iRequest, iResponse;
    int32_t iStateSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // if already created, just ref it
    if (_ProtoPing_pRef != NULL)
    {
        _ProtoPing_pRef->iRefCount += 1;
        return(_ProtoPing_pRef);
    }

    // iMaxPings of zero means use the default
    if (iMaxPings == 0)
    {
        iMaxPings = PROTOPING_DEFAULT_MAXPINGS;
    }

    // calculate size of ref
    iStateSize = sizeof(*pProtoPing) + ((iMaxPings-1) * sizeof(ProtoPingQueueT));

    // allocate and init module state
    if ((pProtoPing = DirtyMemAlloc(iStateSize, PROTOPING_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protoping: could not allocate module state\n"));
        return(NULL);
    }
    memset(pProtoPing, 0, iStateSize);
    pProtoPing->iMemGroup = iMemGroup;
    pProtoPing->pMemGroupUserData = pMemGroupUserData;

    if ((pProtoPing->hIcmp = IcmpCreateFile()) == INVALID_HANDLE_VALUE)
    {
        NetPrintf(("protoping: IcmpCreatefile returned error: %ld\n", GetLastError()));
        ProtoPingDestroy(pProtoPing);
        return(NULL);
    }

    // allocate response buffer
    if ((pProtoPing->pResponses = DirtyMemAlloc(iMaxPings*sizeof(*pProtoPing->pResponses), PROTOPING_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protoping: could not allocate response buffer\n"));
        ProtoPingDestroy(pProtoPing);
        return(NULL);
    }

    // allocate and init response handle buffer
    if ((pProtoPing->ppResponses = DirtyMemAlloc(iMaxPings*sizeof(*pProtoPing->ppResponses), PROTOPING_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protoping: could not allocate response handle buffer\n"));
        ProtoPingDestroy(pProtoPing);
        return(NULL);
    }
    for (iResponse = 0; iResponse < iMaxPings; iResponse += 1)
    {
        pProtoPing->ppResponses[iResponse] = &pProtoPing->pResponses[iResponse];
    }

    // allocate and init event buffer
    if ((pProtoPing->pEvents = DirtyMemAlloc(iMaxPings*sizeof(HANDLE), PROTOPING_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protoping: could not allocate event buffer\n"));
        ProtoPingDestroy(pProtoPing);
        return(NULL);
    }
    // allocate events
    for (iRequest = 0; iRequest < iMaxPings; iRequest += 1)
    {
        if ((pProtoPing->pEvents[iRequest] = CreateEvent(NULL, TRUE, FALSE, NULL)) == INVALID_HANDLE_VALUE)
        {
            NetPrintf(("protoping: CreateEvent() returned error: %ld; max simultaneous pings will be limited to %d\n", GetLastError(), iRequest));
            iMaxPings = iRequest;
            break;
        }
    }
    // fill remaining request event handles as invalid
    for ( ; iRequest < iMaxPings; iRequest += 1)
    {
        pProtoPing->pEvents[iRequest] = INVALID_HANDLE_VALUE;
    }

    // get local address
    pProtoPing->uLocalAddr = SocketGetLocalAddr();

    // save queue size
    pProtoPing->iQueueSize = iMaxPings;
    // need to sync access to data
    NetCritInit(&pProtoPing->ThreadCrit, "protoping");

    // save ref and return state pointer
    pProtoPing->iRefCount = 1;
    _ProtoPing_pRef = pProtoPing;
    return(pProtoPing);
}

/*F********************************************************************************/
/*!
    \Function ProtoPingDestroy

    \Description
        Destroy the module

    \Input *pProtoPing    - reference pointer

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
void ProtoPingDestroy(ProtoPingRefT *pProtoPing)
{
    ProtoPingQueueT *pQueue;
    int32_t iQueue;

    // if we aren't the last, just decrement the refcount and return
    if (--pProtoPing->iRefCount > 0)
    {
        return;
    }

    // close event handles
    for (iQueue = 0; iQueue < pProtoPing->iQueueSize; iQueue++)
    {
        if (pProtoPing->pEvents[iQueue] != INVALID_HANDLE_VALUE)
        {
            CloseHandle(pProtoPing->pEvents[iQueue]);
        }
    }
    // release event handle buffer
    if (pProtoPing->pEvents != NULL)
    {
        DirtyMemFree(pProtoPing->pEvents, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
    }
    // release response handle buffer
    if (pProtoPing->ppResponses != NULL)
    {
        DirtyMemFree(pProtoPing->ppResponses, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
    }
    // release response buffer
    if (pProtoPing->pResponses != NULL)
    {
        DirtyMemFree(pProtoPing->pResponses, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
    }
    // close icmp file handle
    if (pProtoPing->hIcmp != INVALID_HANDLE_VALUE)
    {
        IcmpCloseHandle(pProtoPing->hIcmp);
    }

    // free any unclaimed response data
    for (iQueue = 0; iQueue < pProtoPing->iQueueTail; iQueue++)
    {
        pQueue = &pProtoPing->RequestQueue[iQueue];
        if (pQueue->pData != NULL)
        {
            DirtyMemFree(pQueue->pData, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
        }
    }

    // free the qos ref if there is one
    if (pProtoPing->pQosApi != NULL)
    {
        QosApiDestroy(pProtoPing->pQosApi);
    }

    // done with semaphore
    NetCritKill(&pProtoPing->ThreadCrit);

    // done with state
    DirtyMemFree(pProtoPing, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);

    // clear ref
    _ProtoPing_pRef = NULL;
}

/*F********************************************************************************/
/*!
    \Function ProtoPingControl

    \Description
        Configure ProtoPing module

    \Input *pProtoPing  - module state ref
    \Input iControl     - config selector
    \Input iValue       - selector specific
    \Input *pValue      - selector specific

    \Output
        int32_t         - selector specific

    \Notes
        iControl can be:

        \verbatim
            'spam' - set verbose debug output
            'type' - set the ping type to use
        \endverbatim

    \Version 11/18/2002 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingControl(ProtoPingRefT *pProtoPing, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'spam')
    {
        pProtoPing->bVerbose = iValue;
        if (pProtoPing->pQosApi != NULL)
        {
            return(QosApiControl(pProtoPing->pQosApi, iControl, iValue, pValue));
        }
    }
    if (iControl == 'type')
    {
        pProtoPing->eType = iValue;
        if (pProtoPing->eType == PROTOPING_TYPE_QOS)
        {
            if (pProtoPing->pQosApi == NULL)
            {
                DirtyMemGroupEnter(pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
                pProtoPing->pQosApi = QosApiCreate(_ProtoPingQosCallback, pProtoPing, 0);
                DirtyMemGroupLeave();
                // fail if the ref was not created
                if (pProtoPing->pQosApi == NULL)
                {
                    pProtoPing->eType = PROTOPING_TYPE_ICMP;
                    NetPrintf(("protoping: error, qosapicreate failed\n"));
                    return(-1);
                }
                else
                {
                    //set the server port to hit
                    QosApiControl(pProtoPing->pQosApi, 'sprt', *(int32_t *)pValue, NULL);
                    //configure all latency request to use only 1 probe
                    QosApiControl(pProtoPing->pQosApi, 'lpco', 1, NULL);        
                }
            }
            else
            {
                NetPrintf(("protoping: warning, qosapi ref already exists\n"));
            }
        }
        return(0);
    }
    if (pProtoPing->eType == PROTOPING_TYPE_QOS)
    {
        // pass-through to QosApiControl
        return(QosApiControl(pProtoPing->pQosApi, iControl, iValue, pValue));
    }
    else
    {
        return(-1);
    }
}

/*F********************************************************************************/
/*!
    \Function ProtoPingRequest

    \Description
        Send a ping request uSockAddrg opaque address form.

    \Input *pRef        - reference pointer
    \Input *pAddr       - address to ping
    \Input *pData       - pointer to user data
    \Input iDataLen     - iLength of user data
    \Input iTimeout     - timeout in ms, or zero for default
    \Input iTtl         - time to live, or zero for default

    \Output
        int32_t     - sequence number of packet (zero=failed)

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingRequest(ProtoPingRefT *pRef, DirtyAddrT *pAddr, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl)
{
    uint32_t uAddress;

    // convert opaque address type to 32bit address type
    DirtyAddrToHostAddr(&uAddress, sizeof(uAddress), pAddr);

    // make the request
    return(_ProtoPingRequest(pRef, uAddress, pData, iDataLen, iTimeout, iTtl, FALSE));
}

/*F********************************************************************************/
/*!
    \Function ProtoPingRequestServer

    \Description
        Ping the given server address.

    \Input *pRef            - reference pointer
    \Input uServerAddress   - address of server to ping
    \Input *pData           - pointer to user data
    \Input iDataLen         - iLength of user data
    \Input iTimeout         - timeout in ms, or zero for default
    \Input iTtl             - time to live, or zero for default

    \Output
        int32_t             - sequence number of packet (zero=failed)

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingRequestServer(ProtoPingRefT *pRef, uint32_t uServerAddress, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl)
{
    return(_ProtoPingRequest(pRef, uServerAddress, pData, iDataLen, iTimeout, iTtl, TRUE));
}

/*F********************************************************************************/
/*!
    \Function ProtoPingResponse

    \Description
        Get ping response from queue

    \Input *pProtoPing  - reference pointer
    \Input *pBuffer     - space for user data
    \Input *pBufLen     - size of buffer on entry, actual data size on exit
    \Input *pResponse   - pointer to ping response structure

    \Output
        int32_t         - zero if nothing pending, else ping (negative=timeout)

    \Version 04/03/2002 (gschaefer)
*/
/********************************************************************************F*/
int32_t ProtoPingResponse(ProtoPingRefT *pProtoPing, unsigned char *pBuffer, int32_t *pBufLen, ProtoPingResponseT *pResponse)
{
    ProtoPingQueueT *pRequest;
    int32_t iPing, iQueue;
    uint32_t uCurTime;
    uint8_t bTimeout;
    HANDLE hEvent;
    ProtoPingIcmpResponseT *pIcmpResponse;

    // update icmp rseponse polling
    _ProtoPingUpdate(pProtoPing);

    // if empty queue, bail
    if (pProtoPing->iQueueTail == 0)
    {
        return(0);
    }

    // make sure we own resources
    NetCritEnter(&pProtoPing->ThreadCrit);

    // scan request queue for a response
    for (iPing=0, iQueue=0, uCurTime=NetTick(); iQueue < pProtoPing->iQueueTail; iQueue++)
    {
        pRequest = &pProtoPing->RequestQueue[iQueue];
        bTimeout = (uCurTime - pRequest->uTick) > pRequest->uTime;
        if (!bTimeout)
        {
            bTimeout = (pRequest->uFlag & PROTOPING_FLAG_RECEIVED) != 0 && (pRequest->uPing == 0xffff);
        }
        if ((pProtoPing->eType == PROTOPING_TYPE_QOS) && (pRequest->uFlag & QOSAPI_STATFL_FAILED))
        {
            // timeout early if the request failed
            bTimeout = TRUE;
        }
        if ((pRequest->uFlag & PROTOPING_FLAG_RECEIVED) || (bTimeout == TRUE))
        {
            // save ping for return to caller
            iPing = (!bTimeout) ? (signed)pRequest->uPing : -1;

            // fill in response struct
            if (pResponse != NULL)
            {
                memset(pResponse, 0, sizeof(*pResponse));
                DirtyAddrFromHostAddr(&pResponse->Addr, &pRequest->uAddr);
                pResponse->uAddr = pRequest->uAddr;
                pResponse->iPing = iPing;
                pResponse->uType = pRequest->uType;
                pResponse->uSeqn = pRequest->uSeqn;
                pResponse->bServer = (pRequest->uFlag & PROTOPING_FLAG_SERVER) ? TRUE : FALSE;
            }

            // copy response data, if any, to caller
            if (pRequest->pData != NULL)
            {
                if (pBufLen != NULL)
                {
                    // fill in buffer iLength
                    if (*pBufLen > pRequest->uSize)
                    {
                        *pBufLen = pRequest->uSize;
                    }

                    // fill in buffer
                    if (pBuffer != NULL)
                    {
                        ds_memcpy(pBuffer, pRequest->pData, *pBufLen);
                    }
                }

                // dispose of user data buffer
                DirtyMemFree(pRequest->pData, PROTOPING_MEMID, pProtoPing->iMemGroup, pProtoPing->pMemGroupUserData);
            }

            // remove from queue
            ds_memcpy(&pProtoPing->RequestQueue[iQueue], &pProtoPing->RequestQueue[pProtoPing->iQueueTail - 1], sizeof(*pRequest));
            // swap event handles
            hEvent = pProtoPing->pEvents[iQueue];
            pProtoPing->pEvents[iQueue] = pProtoPing->pEvents[pProtoPing->iQueueTail - 1];
            pProtoPing->pEvents[pProtoPing->iQueueTail - 1] = hEvent;
            // swap response pointers
            pIcmpResponse = pProtoPing->ppResponses[iQueue];
            pProtoPing->ppResponses[iQueue] = pProtoPing->ppResponses[pProtoPing->iQueueTail - 1];
            pProtoPing->ppResponses[pProtoPing->iQueueTail - 1] = pIcmpResponse;
            pProtoPing->iQueueTail -= 1;
            break;
        }
    }

    // release resources
    NetCritLeave(&pProtoPing->ThreadCrit);

    // return ping
    return(iPing);
}


