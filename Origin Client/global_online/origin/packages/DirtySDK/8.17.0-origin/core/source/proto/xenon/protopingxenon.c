/*H********************************************************************************/
/*!
    \File protopingxenon.c

    \Description
        This module implements a DirtySock ping routine.

    \Notes
        Because of the unique network architecture of the Xbox (and the fact that
        a true ICMP ping is disallowed by the network stack), the network latency
        is instead measured using the Xbox QoS (Quality of Service) metrics.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 04/12/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <xonline.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "netconn.h"
#include "protoping.h"

/*** Defines **********************************************************************/

//! enable verbose output
#define PROTOPING_DEBUG     (DIRTYCODE_DEBUG && FALSE)

//! max number of supported Microsoft services - must be updated any time NetConn changes
#define NETCONN_MAXSERVICES (4)

/*** Type Definitions *************************************************************/

//! complete ping request packet
typedef struct ProtoPingRequestT
{
    struct ProtoPingRequestT *pNext;    //!< link to next record
    int32_t iPing;                      //!< ping result
    uint32_t uSeqn;                     //!< ping sequence number
    uint32_t uNumRetries;               //!< number of retries before success (will be non-zero only on Xbox)

    XNQOS *pXnQos;                      //!< pointer to receive QoS info

    enum
    {
        ST_INIT = 0,                    //!< initialized
        ST_SESS,                        //!< getting user session info (ST_CLIENT only)
        ST_PING,                        //!< getting ping info
        ST_DONE,                        //!< acquired ping info
        ST_FAIL                         //!< failed to acquire ping info
    } eRequestState;                    //!< request state

    enum
    {
        TP_CLIENT = 0,                  //!< ping of client
        TP_SERVER                       //!< ping of server (in this case, xbox live service)
    } eRequestType;                     //!< request type

    uint32_t uServiceId;                                    //!< service id to ping  (server only)
    XSESSION_INFO SessionInfo;                              //!< session info to ping (client only)
    XSESSION_SEARCHRESULT_HEADER *pSessionLookupResults;    //!< session lookup resuls (client-only)
    XOVERLAPPED SessionLookupMonitor;                       //!< structure used to monitor the session lookup (client-only)

    int32_t iDataLen;                       //!< length of user data
    char    aData[PROTOPING_MAXUSERDATA];   //!< user data
} ProtoPingRequestT;


//! module state
struct ProtoPingRefT
{
    ProtoPingRequestT *pQueue;              //!< linked list of outstanding ping requests
    uint32_t          uSeqn;                //!< master sequence number
    
    uint32_t          uUserIdx;             //!< index of the current player who will be looking up sessions.
    XNKID             ListenSession;        //!< session currently responding to QoS pings.
    uint32_t          uListenSessionSet;    //!< boolean flag to determine if listen session is set.
    uint32_t          uNumProbes;           //!< number of probes to send for each QoS lookup
    
    // module memory group
    int32_t           iMemGroup;            //!< module mem group id
    void              *pMemGroupUserData;   //!< user data associated with mem group
};

/*** Variables ********************************************************************/

// Private variables

// Public variables


/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ProtoPingCreateRequest

    \Description
        Allocate and initialize a ping request structure and append it to the
        request queue.

    \Input *pRef            - pointer to module state
    \Input eRequestType     - request type (TP_*)
    \Input *pData           - pointer to user data (may be NULL)
    \Input iDataLen         - length of user data (may be zero)

    \Output
        ProtoPingRequestT *  - pointer to new request structure

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
static ProtoPingRequestT *_ProtoPingCreateRequest(ProtoPingRefT *pRef, int32_t eRequestType, const unsigned char *pData, int32_t iDataLen)
{
    ProtoPingRequestT *pRequest, **ppRequest;

    // alloc and init ref
    if ((pRequest = DirtyMemAlloc(sizeof(*pRequest), PROTOPING_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("protopingxenon: error allocating request\n"));
        return(NULL);
    }
    memset(pRequest, 0, sizeof(*pRequest));
    pRequest->eRequestType = eRequestType;
    pRequest->uSeqn = ++pRef->uSeqn;

    // easy string handling
    if (iDataLen < 0)
    {
        iDataLen = strlen((char *)pData)+1;
    }

    // clamp data
    if (iDataLen > sizeof(pRequest->aData))
    {
        NetPrintf(("protopingxenon: clamping data (length=%d) to %d bytes\n", iDataLen, sizeof(pRequest->aData)));
        iDataLen = sizeof(pRequest->aData);
    }

    // copy data
    if (iDataLen != 0)
    {
        memcpy(pRequest->aData, pData, iDataLen);
    }
    pRequest->iDataLen = iDataLen;

    // find end of queue and append
    for (ppRequest = &pRef->pQueue; *ppRequest != NULL; ppRequest = &(*ppRequest)->pNext)
    {
    }
    *ppRequest = pRequest;

    // return ref
    return(pRequest);
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingDestroyRequest

    \Description
        Remove request from queue and free any memory associated with the request.

    \Input *pRef        - pointer to module state
    \Input *pDestroy    - pointer to request to destroy

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoPingDestroyRequest(ProtoPingRefT *pRef, ProtoPingRequestT *pDestroy)
{
    ProtoPingRequestT **ppRequest;

    // find request in queue
    for (ppRequest = &pRef->pQueue; *ppRequest != NULL; ppRequest = &(*ppRequest)->pNext)
    {
        // found the request to destroy?
        if (*ppRequest == pDestroy)
        {
            // free XNQOS structure
            if ((*ppRequest)->pXnQos != NULL)
            {
                XNetQosRelease((*ppRequest)->pXnQos);
            }

            // if we were looking up a session, kill the overlapped operation and any associated memory
            if ((*ppRequest)->eRequestState == ST_SESS)
            {
                XCancelOverlapped(&(*ppRequest)->SessionLookupMonitor);
                if ((*ppRequest)->pSessionLookupResults != NULL)
                {
                    DirtyMemFree((*ppRequest)->pSessionLookupResults, PROTOPING_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
                }
            }

            // dequeue
            *ppRequest = (*ppRequest)->pNext;

            // dispose of request memory and quit loop
            DirtyMemFree(pDestroy, PROTOPING_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
            break;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingUpdateClientPing

    \Description
        Update a client ping request.

    \Input *pRef        - pointer to module state
    \Input *pRequest    - pointer to request to update

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoPingUpdateClientPing(ProtoPingRefT *pRef, ProtoPingRequestT *pRequest)
{   
    if (pRequest->eRequestState == ST_INIT)
    {
        // in the init state, we want to first look up the session info of the host.
        // the first call returns the size of the buffer we need to create
        DWORD dwBufSize = 0;
        DWORD dwStatus = XSessionSearchByID(pRequest->SessionInfo.sessionID, pRef->uUserIdx, 
            &dwBufSize, NULL, NULL);

        if (dwStatus == ERROR_INSUFFICIENT_BUFFER)
        {
            // now call again with the correctly sized buffer 
            if ((pRequest->pSessionLookupResults = DirtyMemAlloc(dwBufSize, PROTOPING_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData)) == NULL)
            {
                NetPrintf(("protopingxenon: error allocating session result buffer\n"));
                return;
            }

            dwStatus = XSessionSearchByID(pRequest->SessionInfo.sessionID, pRef->uUserIdx,
                &dwBufSize, pRequest->pSessionLookupResults, &pRequest->SessionLookupMonitor);

            if (dwStatus == ERROR_IO_PENDING)
            {
                pRequest->eRequestState = ST_SESS;  
            }
            else
            {
                NetPrintf(("protopingxenon: error identifying session\n"));
                pRequest->eRequestState = ST_FAIL;
                return;
            }
        }
        else
        {
            // the first call failed for some reason
            NetPrintf(("protopingxenon: error identifying session\n"));
            pRequest->eRequestState = ST_FAIL;
            return;
        }
    }               

    if (pRequest->eRequestState == ST_SESS)
    {
        DWORD dwResult = XGetOverlappedResult(&pRequest->SessionLookupMonitor, NULL, FALSE);

        if (dwResult == ERROR_SUCCESS)
        {
            // lookup is finished, store the results and start a ping                        
            if (pRequest->pSessionLookupResults->dwSearchResults == 1)
            {
                XNADDR *pXnAddr = &(pRequest->SessionInfo.hostAddress);
                XNKID  *pXnKid = &pRequest->SessionInfo.sessionID;
                XNKEY  *pXnKey = &pRequest->SessionInfo.keyExchangeKey;
                int32_t iResult;

                // store the results
                memcpy(&pRequest->SessionInfo, &pRequest->pSessionLookupResults->pResults[0].info, 
                    sizeof(pRequest->SessionInfo));

                // initiate a QOS query
                iResult = XNetQosLookup(
                    1, &pXnAddr, &pXnKid, &pXnKey, 
                    0, NULL, NULL, pRef->uNumProbes, 0, 0, NULL,
                    &pRequest->pXnQos);
                
                // transation to next state
                pRequest->eRequestState = (iResult == 0) ? ST_PING : ST_FAIL;
            }
            else
            {
                pRequest->eRequestState = ST_FAIL;
            }
        }
        else if (dwResult != ERROR_IO_INCOMPLETE) // if we arent' still waiting, we failed
        {
            // an error occured in the user session lookup, so report as timed out
            NetPrintf(("protopingxenon: error %d getting user session, aborting\n", dwResult));
            pRequest->iPing = 0;
            pRequest->eRequestState = ST_FAIL;
        }
        
        // if we're done, free the search results
        if (pRequest->eRequestState != ST_SESS)
        {
            DirtyMemFree(pRequest->pSessionLookupResults, PROTOPING_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingUpdateServerPing

    \Description
        Update a server ping request.

    \Input *pRef        - pointer to module state
    \Input *pRequest    - pointer to request to update

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoPingUpdateServerPing(ProtoPingRefT *pRef, ProtoPingRequestT *pRequest)
{
    if (pRequest->eRequestState == ST_INIT)
    {
        IN_ADDR InAddr;
        uint32_t uServiceID;
        int32_t iResult;

        // set up required server info
        if (pRequest->uServiceId == 0)
        {
            // special case -- ping default gateway
            InAddr.s_addr = ntohl(NetConnStatus('host', 0, NULL, 0));
            uServiceID = NetConnStatus('xlsp', 0, NULL, 0);
        }
        else
        {
            // remap to secure address
            struct sockaddr SockaddrTo, SockaddrFm;
            SockaddrInit(&SockaddrTo, AF_INET);
            SockaddrInSetAddr(&SockaddrTo, pRequest->uServiceId);
            if (NetConnControl('madr', 0, 0, &SockaddrTo, &SockaddrFm) == 0)
            {
                InAddr.s_addr = SockaddrInGetAddr(&SockaddrFm);
                uServiceID = NetConnStatus('xlsp', 0, NULL, 0);
            }
            else
            {
                NetPrintf(("protopingxenon: failed to resolve secure address 0x%08x\n", pRequest->uServiceId));
                pRequest->eRequestState = ST_FAIL;
                return;
            }
        }

        #if PROTOPING_DEBUG
        NetPrintf(("protopingxenon: [%d] pinging sg=0x%08x, serviceID=0x%08x, tick=%d\n",
            pRequest->uSeqn, SocketInAddrGetText(InAddr.s_addr), uServiceID, NetTick()));
        #endif

        // initiate a QOS query
        if ((iResult = XNetQosLookup(0, NULL, NULL, NULL, 1, &InAddr, (DWORD *)&uServiceID, pRef->uNumProbes, 0, 0, NULL, &pRequest->pXnQos)) != 0)
        {
            NetPrintf(("protopingxenon: [%d] QoS returned error 0x%08x\n", pRequest->uSeqn, iResult));
        }
        
        // transation to next state
        pRequest->eRequestState = (iResult == 0) ? ST_PING : ST_FAIL;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoPingUpdate

    \Description
        Update queued ping requests.

    \Input *pRef            - pointer to module state

    \Output
        ProtoPingRequestT * - pointer to completed ping request, or NULL if there are none.

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
static ProtoPingRequestT *_ProtoPingUpdate(ProtoPingRefT *pRef)
{
    ProtoPingRequestT *pRequest;

    // process all queued requests
    for (pRequest = pRef->pQueue; pRequest != NULL; pRequest = pRequest->pNext)
    {
        // make sure we're online
        if (NetConnStatus('onln', 0, NULL, 0) != TRUE)
        {
            NetPrintf(("protopingxenon: warning, attempting to ping while not online\n"));
            pRequest->eRequestState = ST_FAIL;
        }

        // if the QOS query has not been issued yet
        if (pRequest->eRequestState < ST_PING)
        {
            // update client ping request
            if (pRequest->eRequestType == TP_CLIENT)
            {
                _ProtoPingUpdateClientPing(pRef, pRequest);
            }
        
            // update server ping request
            if (pRequest->eRequestType == TP_SERVER)
            {
                _ProtoPingUpdateServerPing(pRef, pRequest);
            }
        }

        // see if the request is complete
        if (pRequest->eRequestState == ST_PING)
        {
            if (pRequest->pXnQos->axnqosinfo[0].bFlags & XNET_XNQOSINFO_COMPLETE)
            {
                // got a result, so save ping
                if ((pRequest->iPing = pRequest->pXnQos->axnqosinfo[0].wRttMedInMsecs) == 0)
                {
                    // clamp min ping at one (zero is defined as a timeout by ProtoPing)
                    pRequest->iPing = 1;
                }
                else if (pRequest->iPing == 0xffff)
                {
                    // timeout
                    pRequest->iPing = 0;
                }
                pRequest->eRequestState = ST_DONE;

                NetPrintf(("protopingxenon: [%d] ping=%d tick=%d\n", pRequest->uSeqn, pRequest->iPing, NetTick()));

                #if PROTOPING_DEBUG
                NetPrintf(("protopingxenon: [%d] nprobes=%d nresponses=%d\n", pRequest->uSeqn,
                    pRequest->pXnQos->axnqosinfo[0].cProbesXmit, 
                    pRequest->pXnQos->axnqosinfo[0].cProbesRecv)); 
                #endif

                // store number of retries (number of probes sent minus number of probes received)
                pRequest->uNumRetries = pRequest->pXnQos->axnqosinfo[0].cProbesXmit - pRequest->pXnQos->axnqosinfo[0].cProbesRecv;
            }
        }

        // if we are done or failed, return
        if ((pRequest->eRequestState == ST_DONE) || (pRequest->eRequestState == ST_FAIL))
        {
            if (pRequest->eRequestState == ST_FAIL)
            {
                NetPrintf(("protopingxenon: request %d failed\n", pRequest->uSeqn));
            }

            return(pRequest);
        }
    }

    // no request completed yet
    return(NULL);
}


/*** Public Functions ******************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoPingCreate

    \Description
        Create new ping module.

    \Input iMaxPings    - unused

    \Output
        ProtoPingRefT * - module state ref

    \Notes
        XNetQosListen() is presumed to have been called from outside of this module
        to enable QoS responses.

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
ProtoPingRefT *ProtoPingCreate(int32_t iMaxPings)
{
    ProtoPingRefT *pRef = NULL;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and clear the module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), PROTOPING_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->iMemGroup = iMemGroup;
    pRef->pMemGroupUserData = pMemGroupUserData;
    pRef->uNumProbes = 1;

    // return state pointer
    return(pRef);
}

/*F********************************************************************************/
/*!
    \Function ProtoPingDestroy

    \Description
        Destroy the module.

    \Input *pRef    - module state ref

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
void ProtoPingDestroy(ProtoPingRefT *pRef)
{
    // make sure they have a module
    if (pRef == NULL)
    {
        NetPrintf(("protopingxenon: trying to destroy a NULL ref\n"));
    }

    // free any remaining queued responses
    while (pRef->pQueue != NULL)
    {
        _ProtoPingDestroyRequest(pRef, pRef->pQueue);
    }

    // if we had a session we were listening to, release the memory
    if (pRef->uListenSessionSet != 0)
    {
        XNetQosListen(&pRef->ListenSession, NULL, 0, 0, XNET_QOS_LISTEN_RELEASE);
    }

    // done with state
    DirtyMemFree(pRef, PROTOPING_MEMID, pRef->iMemGroup, pRef->pMemGroupUserData);
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
        int32_t     - selector specific

    \Notes
        iControl can be:

        \verbatim
            'nprb' - sets the number of probes to send out with each QoS lookup.
            'sess' - listens for QoS pings on the session id (XNKID) passed to pValue
            'uidx' - sets the index of the active 360 user to iValue (for session lookups)
        \endverbatim

     \Version 01/10/2007 (rbutterfoss)
*/
/********************************************************************************F*/
int32_t ProtoPingControl(ProtoPingRefT *pProtoPing, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'nprb')
    {
        // set the number of probes sent
        pProtoPing->uNumProbes = iValue;
        return(0);
    }
    if (iControl == 'sess')
    {
        // see if we already have something set in the session
        if (pProtoPing->uListenSessionSet != 0)
        {
            // if we do, toss the old one
            XNetQosListen(&pProtoPing->ListenSession, NULL, 0, 0, XNET_QOS_LISTEN_RELEASE);
            memset(&pProtoPing->ListenSession, 0, sizeof(pProtoPing->ListenSession));
            pProtoPing->uListenSessionSet = 0;
        }
        if (pValue != NULL)
        {
            // set the new one and store it for later
            XNKID *sessionId = (XNKID *) pValue;
            XNetQosListen(sessionId, NULL, 0, 0, XNET_QOS_LISTEN_ENABLE);
            memcpy(&pProtoPing->ListenSession, sessionId, sizeof(pProtoPing->ListenSession));
            pProtoPing->uListenSessionSet = 1;
        }
        return(0);
    }
    if (iControl == 'uidx')
    {
        pProtoPing->uUserIdx = iValue;
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoPingRequest

    \Description
        Issue a ping request.

    \Input *pRef    - module state ref
    \Input pAddr    - pointer to address to ping
    \Input *pData   - user data pointer
    \Input iDataLen - size of user data
    \Input iTimeout - unused
    \Input iTtl     - unused

    \Output
        int32_t     - positive=sequence number, zero=failure

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingRequest(ProtoPingRefT *pRef, DirtyAddrT *pAddr, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl)
{
    // allocate client ping request
    ProtoPingRequestT *pRequest;
    if ((pRequest = _ProtoPingCreateRequest(pRef, TP_CLIENT, pData, iDataLen)) == NULL)
    {
        return(0);
    }

    #if PROTOPING_DEBUG
    NetPrintf(("protopingxenon: pinging user %s\n", pAddr->strMachineAddr));
    #endif

    // set target address
    memcpy(&pRequest->SessionInfo.sessionID, pAddr->strMachineAddr, sizeof(pRequest->SessionInfo.sessionID));

    // return sequence number
    return(pRequest->uSeqn);
}

/*F********************************************************************************/
/*!
    \Function ProtoPingRequestServer

    \Description
        Issue a ping request against server.

    \Input *pRef            - module state ref
    \Input uServerAddress   - service id of server to ping - 0 for default gateway
    \Input *pData           - user data pointer
    \Input iDataLen         - size of user data
    \Input iTimeout         - unused
    \Input iTtl             - unused

    \Output
        int32_t             - positive=sequence number, zero=failure

    \Notes
        This function is only supported when running with security enabled.

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingRequestServer(ProtoPingRefT *pRef, uint32_t uServerAddress, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl)
{
    ProtoPingRequestT *pRequest;

    // if we're not in secure mode, we are not connected to a security gateway and cannot QoS ("ping") it.
    if (NetConnStatus('secu', 0, NULL, 0) == FALSE)
    {
        return(0);
    }

    // allocate server ping request
    if ((pRequest = _ProtoPingCreateRequest(pRef, TP_SERVER, pData, iDataLen)) == NULL)
    {
        return(0);
    }
    
    // fill out the service id
    pRequest->uServiceId = uServerAddress;

    #if PROTOPING_DEBUG
    NetPrintf(("protopingxenon: pinging server\n"));
    #endif

    // return sequence number
    return(pRequest->uSeqn);
}

/*F********************************************************************************/
/*!
    \Function ProtoPingResponse

    \Description
        Get ping response from queue.

    \Input *pRef        - module state ref
    \Input *pBuffer     - space for user data
    \Input *pBufLen     - size of buffer on entry, actual data size on exit
    \Input *pResponse   - pointer to ping response structure

    \Output
        int32_t         - ping value, or zero if no ping values are available

    \Version 04/12/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoPingResponse(ProtoPingRefT *pRef, unsigned char *pBuffer, int32_t *pBufLen, ProtoPingResponseT *pResponse)
{
    ProtoPingRequestT *pRequest;
    int32_t iPing;

    // update outstanding requests
    if ((pRequest = _ProtoPingUpdate(pRef)) == NULL)
    {
        // no results ready yet
        return(0);
    }
    
    // get ping value and convert a zero timeout response to -1
    iPing = (pRequest->iPing > 0) ? pRequest->iPing : -1;

    // set up response structure
    if (pResponse != NULL)
    {
        memset(pResponse, 0, sizeof(*pResponse));
        pResponse->iPing = iPing;
        pResponse->uSeqn = pRequest->uSeqn;
        pResponse->bServer = (pRequest->eRequestType == TP_SERVER) ? TRUE : FALSE;
        pResponse->uNumRetries = pRequest->uNumRetries;
        memcpy(&pResponse->Addr.strMachineAddr, &pRequest->SessionInfo.sessionID, sizeof(pRequest->SessionInfo.sessionID));
    }

    // copy data to caller
    if (pBufLen != NULL)
    {
        if (*pBufLen > pRequest->iDataLen)
        {
            *pBufLen = pRequest->iDataLen;
        }
        if (*pBufLen != 0)
        {
            memcpy(pBuffer, pRequest->aData, *pBufLen);
        }
    }

    // destroy request
    _ProtoPingDestroyRequest(pRef, pRequest);

    // return ping
    return(iPing);
}


