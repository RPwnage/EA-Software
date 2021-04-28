/*H********************************************************************************/
/*!
    \File qosapixenon.c

    \Description
        This module implements the client API for the quality of service server and peer-to-peer communication.

    \Copyright
        Copyright (c) 2008 Electronic Arts Inc.

    \Version 1.0 04/07/2008 (cadam) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xtl.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/xenon/dirtysessionmanagerxenon.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/proto/protoname.h"

#include "DirtySDK/misc/qosapi.h"

/*** Defines **********************************************************************/

//! enable verbose output
#define QOSAPI_DEBUG                (DIRTYCODE_DEBUG && FALSE)

//!< qosapi default bits per second
#define QOSAPI_DEFAULT_BITSPERSEC   (16384)

//!< qosapi minimum bits per second
#define QOSAPI_MINIMUM_BITSPERSEC   (4096)

//!< qosapi timeout period
#define QOSAPI_MINIMUM_TIMEOUT      (5000)

/*** Type Definitions *************************************************************/

//! QoS request
typedef struct QosApiRequestT
{
    struct QosApiRequestT *pNext;                           //!< link to the next record

    QosInfoT *pQosInfo;                                     //!< pointer to the QosInfoT receive data
    XNQOS *pXnQos;                                          //!< pointer to the XNQOS receive data

    enum
    {
        TP_CLIENT = 0,                                      //!< client request
        TP_SERVICE                                          //!< service request
    } eRequestType;                                         //!< request type

    int32_t iIssued;                                        //!< has the request been issued
    uint32_t uNumProbes;                                    //!< number of probes the request sends
    uint32_t uBitsPerSec;                                   //!< bit rate specified for the request
    uint32_t uTimeout;                                      //!< timeout for this request in milliseconds

    HostentT *pHost;                                        //!< service address to ping (server only)
    uint32_t uServiceId;                                    //!< service id to ping  (server only)

    XSESSION_INFO SessionInfo;                              //!< session info to ping (client only)
    XSESSION_SEARCHRESULT_HEADER *pSessionLookupResults;    //!< session lookup results (client only)
    XOVERLAPPED SessionLookupMonitor;                       //!< structure used to monitor the session lookup (client-only)
} QosApiRequestT;

//! current module state
struct QosApiRefT
{
    // module memory group
    int32_t iMemGroup;                                      //!< module mem group id
    void *pMemGroupUserData;                                //!< user data associated with mem group
    
    uint32_t uCurrentId;                                    //!< current request id
    uint32_t uUserIdx;                                      //!< user index

    QosApiCallbackT *pCallback;                             //!< callback function to jump to when a status change occurs
    void *pUserData;                                        //!< user specified data to pass to the callback

    uint8_t *pResponse;                                     //!< title specific data to send when responding to QoS probes
    uint32_t uResponseSize;                                 //!< size of the title specific data

    DirtySessionManagerRefT *pSessionManager;               //!< session info for listen
    char strSession[128];                                   //!< session string

    uint32_t uListenSet;                                    //!< has listen already been called?
    QosListenStatusT QosListenStatus;                       //!< current QoS listen status

    uint32_t uCurrentTimeout;                               //!< current timeout for requests in milliseconds

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

    \Input *pQosApi             - pointer to module state

    \Output QosApiRequestT *    - pointer to the new request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static QosApiRequestT *_QosApiCreateRequest(QosApiRefT *pQosApi)
{
    QosApiRequestT *pQosApiRequest, **ppQosApiRequest;

    // alloc and init ref
    if ((pQosApiRequest = DirtyMemAlloc(sizeof(*pQosApiRequest), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapixenon: error allocating request\n"));
        return(NULL);
    }
    memset(pQosApiRequest, 0, sizeof(*pQosApiRequest));

    // alloc and init QosInfo
    if ((pQosApiRequest->pQosInfo = DirtyMemAlloc(sizeof(*pQosApiRequest->pQosInfo), QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapixenon: error allocating QosInfo\n"));
        DirtyMemFree(pQosApiRequest, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
        return(NULL);
    }    
    memset(pQosApiRequest->pQosInfo, 0, sizeof(*pQosApiRequest->pQosInfo));

    // find end of queue and append
    for (ppQosApiRequest = &pQosApi->pRequestQueue; *ppQosApiRequest != NULL; ppQosApiRequest = &(*ppQosApiRequest)->pNext)
    {
    }
    *ppQosApiRequest = pQosApiRequest;

    if (pQosApi->uCurrentId == 0)
    {
        pQosApi->uCurrentId++;
    }

    // set the request id
    pQosApiRequest->pQosInfo->uRequestId = pQosApi->uCurrentId;

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

    \Input *pQosApi     - pointer to module state
    \Input uRequestId   - id of the request to destroy

    \Output int32_t     - 0 if successful, negative otherwise

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
            // set the request
            pQosApiRequest = *ppQosApiRequest;

            // free XNQOS structure
            if ((*ppQosApiRequest)->pXnQos != NULL)
            {
                XNetQosRelease((*ppQosApiRequest)->pXnQos);
            }

            // free the hostent if one has been allocated
            if ((*ppQosApiRequest)->pHost != NULL)
            {
                (*ppQosApiRequest)->pHost->Free((*ppQosApiRequest)->pHost);
                (*ppQosApiRequest)->pHost = NULL;
            }

            // dequeue
            *ppQosApiRequest = (*ppQosApiRequest)->pNext;

            // dispose of request memory and quit loop
            DirtyMemFree(pQosApiRequest->pQosInfo, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            DirtyMemFree(pQosApiRequest, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
            return(0);
        }
    }

    // specified request not found
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _QosApiConvertXNetQosInfoFlags

    \Description
        Converts the XNQOSINFO flag values to their equivalent QOSAPI_STATFL_* value.

    \Input *pXnQos      - pointer to a request's XNQOS structure

    \Output uint32_t    - qosapi status flags of the request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static uint32_t _QosApiConvertXNetQosInfoFlags(XNQOS *pXnQos)
{
    uint32_t uFlags = 0;

    if (pXnQos->axnqosinfo[0].bFlags & XNET_XNQOSINFO_COMPLETE)
    {
        uFlags |= QOSAPI_STATFL_COMPLETE;
    }
    if (pXnQos->axnqosinfo[0].bFlags & XNET_XNQOSINFO_PARTIAL_COMPLETE)
    {
        uFlags |= QOSAPI_STATFL_PART_COMPLETE;
    }
    if (pXnQos->axnqosinfo[0].bFlags & XNET_XNQOSINFO_DATA_RECEIVED)
    {
        uFlags |= QOSAPI_STATFL_DATA_RECEIVED;
    }
    if (pXnQos->axnqosinfo[0].bFlags & XNET_XNQOSINFO_TARGET_DISABLED)
    {
        uFlags |= QOSAPI_STATFL_DISABLED;
    }
    if (pXnQos->axnqosinfo[0].bFlags & XNET_XNQOSINFO_TARGET_CONTACTED)
    {
        uFlags |= QOSAPI_STATFL_CONTACTED;
    }

    return(uFlags);
}

/*F********************************************************************************/
/*!
    \Function _QosApiQueryXOnlineNatType

    \Description
        Calls XOnlineGetNatType and returns the equivalent QOSAPI_NAT_TYPE value.

    \Output QosApiNatTypeE - NAT type of the client

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static QosApiNatTypeE _QosApiQueryXOnlineNatType(void)
{
    XONLINE_NAT_TYPE xOnlineNatType = XOnlineGetNatType();

    switch(xOnlineNatType)
    {
        case XONLINE_NAT_OPEN:
            return(QOSAPI_NAT_OPEN);
        case XONLINE_NAT_MODERATE:
            return(QOSAPI_NAT_MODERATE);
        case XONLINE_NAT_STRICT:
            return(QOSAPI_NAT_STRICT);
        default:
            return(QOSAPI_NAT_PENDING);
    }
}

/*F********************************************************************************/
/*!
    \Function _QosApiConvertXNetQosInfoFlags

    \Description
        Converts the XNQOSINFO flag values to their equivalent QOSAPI_STATFL_* value.

    \Input *pQosApiRequest  - request

    \Output uint32_t        - qosapi status flags of the request

    \Version 04/11/2008 (cadam)
*/
/********************************************************************************F*/
static void _QosApiUpdateQosInfo(QosApiRequestT *pQosApiRequest)
{
    pQosApiRequest->pQosInfo->iProbesSent = pQosApiRequest->pXnQos->axnqosinfo[0].cProbesXmit;
    pQosApiRequest->pQosInfo->iProbesRecv = pQosApiRequest->pXnQos->axnqosinfo[0].cProbesXmit;
    pQosApiRequest->pQosInfo->iMinRTT = pQosApiRequest->pXnQos->axnqosinfo[0].wRttMinInMsecs;
    pQosApiRequest->pQosInfo->iMedRTT = pQosApiRequest->pXnQos->axnqosinfo[0].wRttMedInMsecs;
    pQosApiRequest->pQosInfo->uUpBitsPerSec = pQosApiRequest->pXnQos->axnqosinfo[0].dwUpBitsPerSec;
    pQosApiRequest->pQosInfo->uDwnBitsPerSec = pQosApiRequest->pXnQos->axnqosinfo[0].dwDnBitsPerSec;

    // set the title specific data if there is some and it hasn't already been set
    if ((pQosApiRequest->pQosInfo->iDataLen == 0) && (pQosApiRequest->pXnQos->axnqosinfo[0].cbData > 0))
    {
        pQosApiRequest->pQosInfo->iDataLen = pQosApiRequest->pXnQos->axnqosinfo[0].cbData;
        memcpy(pQosApiRequest->pQosInfo->aData, pQosApiRequest->pXnQos->axnqosinfo[0].pbData, pQosApiRequest->pQosInfo->iDataLen);
        pQosApiRequest->pQosInfo->aData[pQosApiRequest->pQosInfo->iDataLen] = '\0';
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
    NetPrintf(("eCBType: %d\n", eCBType));

    if (eCBType == QOSAPI_CBTYPE_NAT)
    {
        NetPrintf(("eQosApiNatType: %d\n", pCBInfo->eQosApiNatType));
    }
    else
    {
        NetPrintf(("uRequestId: %d\n", pCBInfo->pQosInfo->uRequestId));
        NetPrintf(("uWhenRequested: %d\n", pCBInfo->pQosInfo->uWhenRequested));
        NetPrintf(("iProbesSent: %d\n", pCBInfo->pQosInfo->iProbesSent));
        NetPrintf(("iProbesRecv: %d\n", pCBInfo->pQosInfo->iProbesRecv));
        NetPrintf(("aData: %s\n", pCBInfo->pQosInfo->aData));
        NetPrintf(("iDataLen: %d\n", pCBInfo->pQosInfo->iDataLen));
        NetPrintf(("iMinRTT: %d\n", pCBInfo->pQosInfo->iMinRTT));
        NetPrintf(("iMedRTT: %d\n", pCBInfo->pQosInfo->iMedRTT));
        NetPrintf(("uUpBitsPerSec: %d\n", pCBInfo->pQosInfo->uUpBitsPerSec));
        NetPrintf(("uDwnBitsPerSec: %d\n", pCBInfo->pQosInfo->uDwnBitsPerSec));
        NetPrintf(("uFlags: %d\n", pCBInfo->pQosInfo->uFlags));
    }
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
    QosApiRequestT **ppRequest = &pQosApi->pRequestQueue;
    QosApiCBInfoT CBInfo;
    uint32_t uFlags = 0;

    if (pQosApi->pSessionManager != NULL)
    {
        DirtySessionManagerUpdate(pQosApi->pSessionManager);
    }

    if (pQosApi->eQosApiNatType == QOSAPI_NAT_PENDING)
    {
        pQosApi->eQosApiNatType = _QosApiQueryXOnlineNatType();

        if (pQosApi->eQosApiNatType != QOSAPI_NAT_PENDING)
        {
            memset(&CBInfo, 0, sizeof(CBInfo));
            CBInfo.eQosApiNatType = pQosApi->eQosApiNatType;
            pQosApi->pCallback(pQosApi, &CBInfo, QOSAPI_CBTYPE_NAT, pQosApi->pUserData);
        }
    }

    while (*ppRequest != NULL)
    {
        // set the uFlags value to the current value of the request flags
        uFlags = (*ppRequest)->pQosInfo->uFlags;

        if ((*ppRequest)->iIssued == 0)
        {
            if ((*ppRequest)->eRequestType == TP_CLIENT)
            {
                DWORD dwResult = XGetOverlappedExtendedError(&(*ppRequest)->SessionLookupMonitor);

                if (dwResult == ERROR_SUCCESS)
                {
                    if ((*ppRequest)->pSessionLookupResults->dwSearchResults == 1)
                    {
                        XNADDR *pXnAddr = &(*ppRequest)->SessionInfo.hostAddress;
                        XNKID *pXnKid = &(*ppRequest)->SessionInfo.sessionID;
                        XNKEY *pXnKey = &(*ppRequest)->SessionInfo.keyExchangeKey;

                        memcpy(&(*ppRequest)->SessionInfo, &(*ppRequest)->pSessionLookupResults->pResults[0].info,
                               sizeof((*ppRequest)->SessionInfo));

                        // issue the request
                        if (XNetQosLookup(1, &pXnAddr, &pXnKid, &pXnKey, 0, NULL, NULL, (*ppRequest)->uNumProbes,
                                          (*ppRequest)->uBitsPerSec, 0, NULL, &(*ppRequest)->pXnQos) != 0)
                        {
                            NetPrintf(("qosapixenon: request %d failed\n", (*ppRequest)->pQosInfo->uRequestId));
                            uFlags = QOSAPI_STATFL_FAILED;
                        }
                        else
                        {
                            NetPrintf(("qosapixenon: request %d sent %d probes\n", (*ppRequest)->pQosInfo->uRequestId, (*ppRequest)->uNumProbes));
                        }
                    }
                    else
                    {
                        NetPrintf(("qosapixenon: request %d failed; %d search results received instead of 1\n",
                                   (*ppRequest)->pQosInfo->uRequestId, (*ppRequest)->pSessionLookupResults->dwSearchResults));
                        uFlags = QOSAPI_STATFL_FAILED;
                    }
                    (*ppRequest)->iIssued = 1;
                }
                else if (dwResult != ERROR_IO_INCOMPLETE)
                {
                    NetPrintf(("qosapixenon: request %d failed with error %s while getting the user session\n",
                               (*ppRequest)->pQosInfo->uRequestId, DirtyErrGetName(dwResult)));
                    uFlags = QOSAPI_STATFL_FAILED;
                    (*ppRequest)->iIssued = 1;
                }
            }
            else
            {
                int32_t iHost = (*ppRequest)->pHost->Done((*ppRequest)->pHost);

                if (iHost == -1)
                {
                    NetPrintf(("qosapixenon: service request %d failed; unknown host\n", (*ppRequest)->pQosInfo->uRequestId));
                    uFlags = QOSAPI_STATFL_FAILED;
                    (*ppRequest)->iIssued = 1;
                }
                else if (iHost != 0)
                {
                    IN_ADDR InAddr;
                    struct sockaddr SockaddrTo, SockaddrFm;

                    SockaddrInit(&SockaddrTo, AF_INET);
                    SockaddrInSetAddr(&SockaddrTo, (*ppRequest)->pHost->addr);

                    if (NetConnControl('madr', 0, 0, &SockaddrTo, &SockaddrFm) == 0)
                    {
                        InAddr.s_addr = SockaddrInGetAddr(&SockaddrFm);
                        
                        // if service is unspecified get it from remap operation result
                        if ((*ppRequest)->uServiceId == 0)
                        {
                            (*ppRequest)->uServiceId = SockaddrInGetMisc(&SockaddrFm);
                        }

                        // issue the request
                        if (XNetQosLookup(0, NULL, NULL, NULL, 1, &InAddr, (DWORD *)&(*ppRequest)->uServiceId, (*ppRequest)->uNumProbes, (*ppRequest)->uBitsPerSec, 0, NULL, &(*ppRequest)->pXnQos) != 0)
                        {
                            NetPrintf(("qosapixenon: service request %d failed to issue the request\n", (*ppRequest)->pQosInfo->uRequestId));
                            uFlags = QOSAPI_STATFL_FAILED;
                        }
                    }
                    else
                    {
                        NetPrintf(("qosapixenon: service request %d failed to remap to the secure address\n", (*ppRequest)->pQosInfo->uRequestId));
                        uFlags = QOSAPI_STATFL_FAILED;
                    }
                    (*ppRequest)->iIssued = 1;
                }
            }
        }

        if ((*ppRequest)->iIssued == 1)
        {
            if (!((uFlags & QOSAPI_STATFL_FAILED) || (uFlags & QOSAPI_STATFL_TIMEDOUT)))
            {
                uFlags = _QosApiConvertXNetQosInfoFlags((*ppRequest)->pXnQos);
            }

            if ((!((uFlags & QOSAPI_STATFL_FAILED) || (uFlags & QOSAPI_STATFL_TIMEDOUT) ||
                   (uFlags & QOSAPI_STATFL_COMPLETE))) &&
                ((NetTick() - (*ppRequest)->pQosInfo->uWhenRequested) > (*ppRequest)->uTimeout))
            {
                uFlags |= QOSAPI_STATFL_TIMEDOUT;
            }

            if ((*ppRequest)->pQosInfo->uFlags != uFlags)
            {
                CBInfo.uOldStatus = (*ppRequest)->pQosInfo->uFlags;
                (*ppRequest)->pQosInfo->uFlags |= uFlags;
                CBInfo.uNewStatus = (*ppRequest)->pQosInfo->uFlags;
                if (!(((*ppRequest)->pQosInfo->uFlags & QOSAPI_STATFL_FAILED) || ((*ppRequest)->pQosInfo->uFlags & QOSAPI_STATFL_TIMEDOUT)))
                {
                    _QosApiUpdateQosInfo(*ppRequest);
                }
                CBInfo.pQosInfo = (*ppRequest)->pQosInfo;
                CBInfo.eQosApiNatType = pQosApi->eQosApiNatType;
                pQosApi->pCallback(pQosApi, &CBInfo, QOSAPI_CBTYPE_STATUS, pQosApi->pUserData);
            }
        }

        ppRequest = &(*ppRequest)->pNext;
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function QosApiCreate

    \Description
        Create the QosApi module.

    \Input *pCallback    - callback function to use when a status change is detected
    \Input *pUserData    - use data to be set for the callback
    \Input iServicePort  - set the blaze qos service http port

    \Output QosApiRefT * - state pointer

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
QosApiRefT *QosApiCreate(QosApiCallbackT *pCallback, void *pUserData, int32_t iServicePort)
{
    DirtyAddrT aSelfAddr[NETCONN_MAXLOCALUSERS];             //local address, using array to identify slot
    QosApiRefT *pQosApi;
    int32_t iMemGroup;
    int32_t iUserIndex;
    void *pMemGroupUserData;
    XNADDR XnAddr;
    XUID Xuid;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pQosApi = DirtyMemAlloc(sizeof(*pQosApi), QOSAPI_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapixenon: could not allocate module state\n"));
        return(NULL);
    }
    memset(pQosApi, 0, sizeof(*pQosApi));
    pQosApi->iMemGroup = iMemGroup;
    pQosApi->pMemGroupUserData = pMemGroupUserData;
    pQosApi->eQosApiNatType = QOSAPI_NAT_PENDING;

    // get a logged in user index
    iUserIndex = NetConnQuery(NULL, NULL, 0);
    // get xuid and xnaddr
    NetConnStatus('xuid', iUserIndex, &Xuid, sizeof(Xuid));
    NetConnStatus('xadd', 0, &XnAddr, sizeof(XnAddr));
    // set maddr
    memset(aSelfAddr, 0, sizeof(aSelfAddr));
    DirtyAddrSetInfoXenon(&aSelfAddr[iUserIndex], &Xuid, &XnAddr);

    // create the session ref - local is the host
    if ((pQosApi->pSessionManager = DirtySessionManagerCreate(NULL, NULL)) == NULL)
    {
        NetPrintf(("qosapixenon: could not create dirtysessionmanager ref\n"));
        DirtyMemFree(pQosApi, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
        return(NULL);
    }

    // set the session flags
    DirtySessionManagerControl(pQosApi->pSessionManager, 'sflg', XSESSION_CREATE_USES_MATCHMAKING |
                                                                 XSESSION_CREATE_USES_PEER_NETWORK |
                                                                 XSESSION_CREATE_JOIN_IN_PROGRESS_DISABLED,
   
                                                                 0, NULL);

    // create the session
    if (DirtySessionManagerCreateSess(pQosApi->pSessionManager, FALSE, 0, NULL, aSelfAddr) != 0)
    {
        NetPrintf(("qosapixenon: could not create the session\n"));
        DirtySessionManagerDestroy(pQosApi->pSessionManager);
        DirtyMemFree(pQosApi, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
        return(NULL);
    }

    // set user callback
    pQosApi->pCallback = (pCallback == NULL) ? _QosApiDefaultCallback : pCallback;
    pQosApi->pUserData = pUserData;

    // set the current timeout
    pQosApi->uCurrentTimeout = QOSAPI_MINIMUM_TIMEOUT;

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

    \Output
        int32_t             - 0 if successful, negative if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiListen(QosApiRefT *pQosApi, uint8_t *pResponse, uint32_t uResponseSize, uint32_t uBitsPerSecond, uint32_t uFlags)
{
    int32_t iResult;
    uint32_t uListenFlags = uFlags;
    XSESSION_INFO SessionInfo;

    // set the session string
    if (DirtySessionManagerStatus(pQosApi->pSessionManager, 'sess', pQosApi->strSession, sizeof(pQosApi->strSession)) < 0)
    {
        NetPrintf(("qosapixenon: could not get the session information\n"));
        return(-1);
    }

    // get the session information
    DirtySessionManagerDecodeSession(&SessionInfo, pQosApi->strSession);

    // if uListenFlags is 0 set to the default ENABLE | SET_DATA | SET_BITSPERSEC flag
    if (uListenFlags == 0)
    {
        uListenFlags = XNET_QOS_LISTEN_ENABLE | XNET_QOS_LISTEN_SET_DATA | XNET_QOS_LISTEN_SET_BITSPERSEC;
    }

    // listen with default values; values can be changed use the QosApiControl function with the correct iControl value
    iResult = XNetQosListen(&SessionInfo.sessionID, pResponse, uResponseSize, uBitsPerSecond, uListenFlags);
    if (iResult != 0)
    {
        NetPrintf(("qosapixenon: XNetQosListen failed with %d\n", iResult));
        return(-2);
    }
    pQosApi->uListenSet = 1;

    return(0);
}

/*F********************************************************************************/
/*!
    \Function QosApiRequest

    \Description
        Send a QoS request to the specified host.

    \Input *pQosApi     - module state
    \Input *pAddr       - DirtyAddr specifying the host to send the request to
    \Input uNumProbes   - number of probes to send out for the request
    \Input uBitsPerSec  - bits per second to send the request at
    \Input uFlags       - QOSAPI_REQUESTFL_*

    \Output
        uint32_t        - request id, 0 if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
uint32_t QosApiRequest(QosApiRefT *pQosApi, DirtyAddrT *pAddr, uint32_t uNumProbes, uint32_t uBitsPerSec, uint32_t uFlags)
{
    QosApiRequestT *pRequest;
    DWORD dwStatus, dwBufSize = 0;

    // allocate the request
    if ((pRequest = _QosApiCreateRequest(pQosApi)) == 0)
    {
        NetPrintf(("qosapixenon: unable to create the request\n"));
        return(0);
    }
    pRequest->eRequestType = TP_CLIENT;

    // set uBitsPerSec to the default value if 0 was passed in
    if (uBitsPerSec == 0)
    {
        uBitsPerSec = QOSAPI_DEFAULT_BITSPERSEC;
    }

    // set the session id
    memcpy(&pRequest->SessionInfo.sessionID, pAddr->strMachineAddr, sizeof(pRequest->SessionInfo.sessionID));

    // get the session information
    dwStatus = XSessionSearchByID(pRequest->SessionInfo.sessionID, pQosApi->uUserIdx, &dwBufSize, NULL, NULL);

    // check to make sure the first call failed due to the buffer
    if (dwStatus != ERROR_INSUFFICIENT_BUFFER)
    {
        NetPrintf(("qosapixenon: session search failed\n"));
        _QosApiDestroyRequest(pQosApi, pRequest->pQosInfo->uRequestId);
        return(0);
    }

    if ((pRequest->pSessionLookupResults = DirtyMemAlloc(dwBufSize, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("qosapixenon: unable to create the session lookup result buffer\n"));
        _QosApiDestroyRequest(pQosApi, pRequest->pQosInfo->uRequestId);
        return(0);
    }

    dwStatus = XSessionSearchByID(pRequest->SessionInfo.sessionID, pQosApi->uUserIdx, &dwBufSize, pRequest->pSessionLookupResults,
                                  &pRequest->SessionLookupMonitor);

    if (dwStatus != ERROR_IO_PENDING)
    {
        NetPrintf(("qosapixenon: session search failed\n"));
        _QosApiDestroyRequest(pQosApi, pRequest->pQosInfo->uRequestId);
        return(0);
    }

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
uint32_t QosApiServiceRequest(QosApiRefT *pQosApi, char *pServerAddress, uint16_t uProbePort, uint32_t uServiceId,
                               uint32_t uNumProbes, uint32_t uBitsPerSec, uint32_t uFlags)
{
    QosApiRequestT *pRequest;

    // make sure we're in secure mode before me make any QoS requests to the security gateway
    if (NetConnStatus('secu', 0, NULL, 0) == FALSE)
    {
        NetPrintf(("qosapixenon: service request only usable when in secure mode\n"));
        return(0);
    }

    // allocate the request
    if ((pRequest = _QosApiCreateRequest(pQosApi)) == 0)
    {
        NetPrintf(("qosapixenon: service request was unable to create the request\n"));
        return(0);
    }
    pRequest->eRequestType = TP_SERVICE;

    // set uBitsPerSec to the default value if 0 was passed in
    if (uBitsPerSec == 0)
    {
        uBitsPerSec = QOSAPI_DEFAULT_BITSPERSEC;
    }

    // lookup the host
    pRequest->pHost = ProtoNameAsync(pServerAddress, 5000);
    pRequest->uServiceId = uServiceId;

    // save the number of probes sent and the bit rate of the request
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
    \Function QosApiCancelRequest

    \Description
        Cancel the specified QoS request.

    \Input *pQosApi     - module state
    \Input uRequestId   - request to cancel

    \Output
        int32_t         - 0 if successful, negative if failure.

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiCancelRequest(QosApiRefT *pQosApi, uint32_t uRequestId)
{
    if (_QosApiDestroyRequest(pQosApi, uRequestId) != 0)
    {
        NetPrintf(("qosapixenon: the specified request does not exist; no requests cancelled\n"));
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
                'lena' - Enable listening for QoS requests
                'ldis' - Disable listening for QoS requests
                'lprt' - Set the port to use for the QoS listen port (must be set before calling listen/request)
                'sdat' - Set response data when responding to a latency request
                'sbps' - Set the maximum bits per second that QoS responses should use
                'spam' - Set the verbosity of the http module (not used on xenon)
                'sprt' - Set the port to send the http requests to when issuing a blaze service request (not used on xenon)
                'time' - Set the current timeout for new requests (milliseconds)
                'uidx' - Sets the user index
        \endverbatim

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiControl(QosApiRefT *pQosApi, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'lena')
    {
        int32_t iResult;
        XSESSION_INFO SessionInfo;

        // get the session information
        DirtySessionManagerDecodeSession(&SessionInfo, pQosApi->strSession);

        // enable listen
        iResult = XNetQosListen(&SessionInfo.sessionID, NULL, 0, 0, XNET_QOS_LISTEN_ENABLE);
        if (iResult != 0)
        {
            NetPrintf(("qosapixenon: XNetQosListen failed with %d\n", iResult));
            return(-2);
        }
        return(0);
    }
    if (iControl == 'ldis')
    {
        int32_t iResult;
        XSESSION_INFO SessionInfo;

        // get the session information
        DirtySessionManagerDecodeSession(&SessionInfo, pQosApi->strSession);

        // disable listen
        iResult = XNetQosListen(&SessionInfo.sessionID, NULL, 0, 0, XNET_QOS_LISTEN_DISABLE);
        if (iResult != 0)
        {
            NetPrintf(("qosapixenon: XNetQosListen failed with %d\n", iResult));
            return(-2);
        }
        return(0);
    }
    if (iControl == 'lprt')
    {
        return(0);
    }
    if (iControl == 'sdat')
    {
        int32_t iResult;
        XSESSION_INFO SessionInfo;

        // get the session information
        DirtySessionManagerDecodeSession(&SessionInfo, pQosApi->strSession);

        // set the response data
        iResult = XNetQosListen(&SessionInfo.sessionID, pValue, iValue, 0, XNET_QOS_LISTEN_SET_DATA);
        if (iResult != 0)
        {
            NetPrintf(("qosapixenon: XNetQosListen failed with %d\n", iResult));
            return(-2);
        }
        return(0);
    }
    if (iControl == 'sbps')
    {
        int32_t iResult;
        XSESSION_INFO SessionInfo;

        // get the session information
        DirtySessionManagerDecodeSession(&SessionInfo, pQosApi->strSession);

        // set the bits per second
        iResult = XNetQosListen(&SessionInfo.sessionID, NULL, 0, iValue, XNET_QOS_LISTEN_SET_BITSPERSEC);
        if (iResult != 0)
        {
            NetPrintf(("qosapixenon: XNetQosListen failed with %d\n", iResult));
            return(-2);
        }
        return(0);
    }
    if (iControl == 'spam')
    {
        return(0);
    }
    if (iControl == 'sprt')
    {
        return(0);
    }
    if (iControl == 'time')
    {
        if (iValue < QOSAPI_MINIMUM_TIMEOUT)
        {
            pQosApi->uCurrentTimeout = QOSAPI_MINIMUM_TIMEOUT;
        }
        else
        {
            pQosApi->uCurrentTimeout = iValue;
        }
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
                'extn' - Gets the external address information of the client for the specified request (not used on xenon)
                'clpt' - Gets the current listen port of the QoS socket (not used on xenon)
        \endverbatim

    \Version 04/07/2008 (cadam)
*/
/********************************************************************************F*/
int32_t QosApiStatus(QosApiRefT *pQosApi, int32_t iSelect, int32_t iData, void *pBuf, int32_t iBufSize)
{
    if ((iSelect == 'sess') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pQosApi->strSession)))
    {
        memcpy(pBuf, &pQosApi->strSession, sizeof(pQosApi->strSession));
        return(0);
    }
    if ((iSelect == 'list') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(QosListenStatusT)))
    {
        memcpy(pBuf, &pQosApi->QosListenStatus, sizeof(QosListenStatusT));
        return(0);
    }
    if ((iSelect == 'requ') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(QosInfoT)))
    {
        QosApiRequestT **ppQosApiRequest;

        // find request in queue
        for (ppQosApiRequest = &pQosApi->pRequestQueue; *ppQosApiRequest != NULL; ppQosApiRequest = &(*ppQosApiRequest)->pNext)
        {
            // found the request to return the status for?
            if ((*ppQosApiRequest)->pQosInfo->uRequestId == (unsigned)iData)
            {
                memcpy(pBuf, (*ppQosApiRequest)->pQosInfo, sizeof(QosInfoT));
                return(0);
            }
        }
        return(-2);
    }
    if (iSelect == 'extn')
    {
        return(0);
    }
    if (iSelect == 'clpt')
    {
        return(0);
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

    \Version 1.0 04/07/2008 (cadam)
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
    NetConnIdleDel(_QosApiUpdate, pQosApi);

    while (pQosApi->pRequestQueue != NULL)
    {
        _QosApiDestroyRequest(pQosApi, pQosApi->pRequestQueue->pQosInfo->uRequestId);
    }

    if (pQosApi->uListenSet)
    {
        XSESSION_INFO SessionInfo;
        DirtySessionManagerDecodeSession(&SessionInfo, pQosApi->strSession);
        XNetQosListen(&SessionInfo.sessionID, NULL, 0, 0, XNET_QOS_LISTEN_RELEASE);
    }

    if (pQosApi->pSessionManager != NULL)
    {
        DirtySessionManagerDestroy(pQosApi->pSessionManager);
        pQosApi->pSessionManager = NULL;
    }

    DirtyMemFree(pQosApi, QOSAPI_MEMID, pQosApi->iMemGroup, pQosApi->pMemGroupUserData);
}
