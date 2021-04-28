/*H*************************************************************************************************/
/*!

    \File protopingmanager.c

    \Description
        PingManager is a high-level interface to ProtoPing.  It provides metering of ping
        requests and LRU caching, so as to enable high-level applications to issue high volumes
        of pings without worrying about overburdening the system, or creating ping storms.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2003.  ALL RIGHTS RESERVED.

    \Version    1.0        08/20/03 (JLB) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>
#include <stdio.h>
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/proto/protoping.h"
#include "DirtySDK/proto/protopingmanager.h"
#include "DirtySDK/misc/qosapi.h"

/*** Defines ***************************************************************************/

#define PINGMGR_DEBUG               (DIRTYCODE_DEBUG && FALSE)

#define PINGMGR_PINGFLAG_NEWPING    (0x8000)                //!< ping request needs to be issued
#define PINGMGR_PINGFLAG_PINGING    (0x4000)                //!< ping request is in progress
#define PINGMGR_PINGFLAG_TIMEOUT    (0x2000)                //!< ping request has timed out
#define PINGMGR_PINGFLAG_SUCCESS    (0x0000)                //!< ping request was responded to
#define PINGMGR_PINGFLAG_MASK       (0xF000)                //!< all pingflags must fit in this mask
#define PINGMGR_MAXPINGVAL          (0x0FFF)                //!< maximum allowable ping value

#define PINGMGR_TICKMUL             (1)                     //!< one second (PINGMGR_TIMERTICKS) per tick
#define PINGMGR_TIMERTICKS          (PINGMGR_TICKMUL*1000)  //!< resolution of timer (one second)
#define PINGMGR_TIMEOUT             (4*PINGMGR_TICKMUL)     //!< timeout in TICKMUL units (four seconds)
#define PINGMGR_STALEEXPIRETIME     (60*PINGMGR_TICKMUL)    //!< if stale record hasn't been touched in this amount of time, expire it

#define PINGMGR_TICKSPERPING        (100)                   //!< number of ticks that must pass between pings

#define PINGMGR_MAXSERVERPINGS      (8)                     //!< maximum number of server pings that can be tracked simultaneously

/*** Macros ****************************************************************************/

//! set a PINGFLAG
#define PINGMGR_SetFlag(_pingvar, _newflag) \
{                                           \
    (_pingvar) &= ~PINGMGR_PINGFLAG_MASK;   \
    (_pingvar) |= (_newflag);               \
}

//! calculate time difference of time1-time2 with wrapping
#define PINGMGR_CalcTimeDiff(_time1, _time2)    (((unsigned)(_time1) - (unsigned)(_time2)) & 0xff)

/*** Type Definitions ******************************************************************/

//! a ping cache ref
typedef struct PingRecT
{
    DirtyAddrT Addr;                //!< opaque address type
    uint16_t uPing;                 //!< ping data (can be a ping value + one of the PINGMGR_PINGFLAG_ flags)
    unsigned char uCreatedTime;     //!< creation time (used for expiration due to age)
    unsigned char uTouchedTime;     //!< last time touched (used for expiration due to overflow)
    PingManagerCallbackT *pCallback;//!< user-specified callback
    void *pCallbackData;            //!< user-specified callback data
} PingRecT;

//! a server ping ref
typedef struct ServerPingRecT
{
    PingManagerCallbackT *pServCb;
    void *pServCbData;
    uint32_t uSeqn;
    uint32_t uAddr;
} ServerPingRecT;

//! ping manager private data
struct PingManagerRefT
{
    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group
    
    uint32_t uMaxRecords;           //!< max cache size
    uint32_t uNumRecords;           //!< number of records in cache
    unsigned char uTime;            //!< current time
    uint32_t uTickCount;            //!< current tick count
    uint32_t uLastPingTick;         //!< tick count when previous ping was issued

    ServerPingRecT ServerPings[PINGMGR_MAXSERVERPINGS]; //!< server ping callback tracking

    char strServerAddress[64];      //!< string used to convert ip into dotted string format for qosapi
    QosApiRefT *pQosRef;            //!< our QosApi ref

    ProtoPingRefT *pPingRef;        //!< protoping ref
    PingRecT PingCache[1];          //!< ping cache

};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! PingManager ref (only one allowed)
static PingManagerRefT  *_PingManager_pRef = NULL;

//! PingManager refcount, if "creating" more than one instance
static int32_t          _PingManager_iRefCount = 0;

// Public variables


/*** Private Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    _PingManagerFindRecordByAddress

    \Description
        Find the record in the cache that holds the given address.

    \Input  *pPingManager   - ping manager ref
    \Input  *pAddr          - address of cache entry to find

    \Output
        int32_t             - index of matching cache entry, or -1 if no match

    \Version 08/21/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _PingManagerFindRecordByAddress(PingManagerRefT *pPingManager, DirtyAddrT *pAddr)
{
    uint32_t uRecord;

    for (uRecord = 0; uRecord < pPingManager->uNumRecords; uRecord++)
    {
        if (DirtyAddrCompare(&pPingManager->PingCache[uRecord].Addr, pAddr))
        {
            return((int32_t)uRecord);
        }
    }

    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    _PingManagerFindRecordLRU

    \Description
        Find the least recently used record.

    \Input  *pPingManager   - ping manager ref

    \Output
        int32_t             - index of oldest cache entry

    \Version 09/02/03 (JLB)
*/
/*************************************************************************************************F*/
static uint32_t _PingManagerFindRecordLRU(PingManagerRefT *pPingManager)
{
    uint32_t uRecord, uOldest, uOldestTime, uTemp;

    for (uRecord = 0, uOldest = 0, uOldestTime = 0; uRecord < pPingManager->uNumRecords; uRecord++)
    {
        // calculate age of 8bit wrapping timer value
        uTemp = PINGMGR_CalcTimeDiff(pPingManager->uTime, pPingManager->PingCache[uRecord].uTouchedTime);
        if (uOldestTime < uTemp)
        {
            uOldestTime = uTemp;
            uOldest = uRecord;
        }
    }

    return(uOldest);
}

/*F*************************************************************************************************/
/*!
    \Function    _PingManagerInvalidateRecord

    \Description
        Invalidate an individual cache entry.

    \Input  *pPingManager   - ping manager ref
    \Input  *pRecord        - record to invalidate

    \Version 03/19/04 (JLB)
*/
/*************************************************************************************************F*/
static void _PingManagerInvalidateRecord(PingManagerRefT *pPingManager, PingRecT *pRecord)
{
    // invalidate record
    pRecord->uPing = PINGMGR_PINGFLAG_NEWPING;
    pRecord->uCreatedTime = pRecord->uTouchedTime = pPingManager->uTime-1;
}

/*F*************************************************************************************************/
/*!
    \Function    _PingManagerUpdateTimer

    \Description
        Update the ping manager timer

    \Input  *pPingManager   - ping manager ref
    \Input  uCurTick        - current tick

    \Version 09/03/03 (JLB)
*/
/*************************************************************************************************F*/
static void _PingManagerUpdateTimer(PingManagerRefT *pPingManager, uint32_t uCurTick)
{
    uint32_t uTimeElapsed, uTicksElapsed;

    // calc elapsed time in PINGMGR_TIMERTICKS units
    uTicksElapsed = uCurTick - pPingManager->uTickCount;
    uTimeElapsed = uTicksElapsed/PINGMGR_TIMERTICKS;

    // has at least one PINGMGR_TIMERTICKS unit elapsed since the last update?
    if (uTimeElapsed > 0)
    {
        // update timer and tickcount
        pPingManager->uTime += uTimeElapsed;
        pPingManager->uTickCount += uTimeElapsed * PINGMGR_TIMERTICKS;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _PingManagerGetPing

    \Description
        Take internal ping data and return a ping value for external consumption.

    \Input  uPingData       - ping data

    \Output
        int32_t             - ping value, or PINGMGR_PINGSTATUS_TIMEOUT, or PINGMGR_PINGSTATUS_PINGING

    \Version 09/04/03 (JLB)
*/
/*************************************************************************************************F*/
static int32_t _PingManagerGetPing(uint32_t uPingData)
{
    uint32_t uPingValue;

    if (uPingData & PINGMGR_PINGFLAG_TIMEOUT)
    {
        return(PINGMGR_PINGSTATUS_TIMEOUT);
    }
    else if ((uPingValue = (uPingData & ~PINGMGR_PINGFLAG_MASK)) != 0)
    {
        return(uPingValue);
    }
    else
    {
        return(PINGMGR_PINGSTATUS_PINGING);
    }
}

#if defined(DIRTYCODE_PC)
/*F*************************************************************************************************/
/*!
    \Function    _PingManagerQosCallback

    \Description
        Handles the qosapi callbacks and dispatch them back to the user of PingManager 

    \Input *pQosApi     - QosApi ref
    \Input *pCBInfo     - the info regarding this callback
    \Input eCBType      - the callback type
    \Input *pUserData   - our PingManager erf, passed back to us

    \Version 11/01/08 (jrainy)
*/
/*************************************************************************************************F*/
static void _PingManagerQosCallback(QosApiRefT *pQosApi, QosApiCBInfoT *pCBInfo, QosApiCBTypeE eCBType, void *pUserData)
{
    PingManagerRefT *pPingManager = (PingManagerRefT *)pUserData;
    ServerPingRecT *pServerPing;
    int32_t iServerPing;

    for (iServerPing = 0, pServerPing = NULL; iServerPing < PINGMGR_MAXSERVERPINGS; iServerPing++)
    {
        if (pPingManager->ServerPings[iServerPing].uSeqn == pCBInfo->pQosInfo->uRequestId)
        {
            pServerPing = &pPingManager->ServerPings[iServerPing];
            break;
        }
    }

    if (pServerPing)
    {
        NetPrintf(("pingmanager: _PingManagerQosCallback: flags %d, address %a\n", pCBInfo->uNewStatus, pServerPing->uAddr));
        pServerPing->pServCb((DirtyAddrT*)&pServerPing->uAddr, pCBInfo->pQosInfo->iMinRTT, pServerPing->pServCbData);
        pServerPing->uSeqn = 0;
    }
}
#endif

/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    PingManagerCreate

    \Description
        Create PingManager module.

    \Input uMaxRecords      - size of LRU ping history cache.

    \Output
        PingManagerRefT *   - created PingManager ref, or NULL if failure.  

    \Notes
        Only one copy is instantiated.  All other Create calls increment a
        reference counter and return a copy to the first version.  This means
        that the first Create call sets the cache size.

    \Version 08/21/03 (JLB)
*/
/*************************************************************************************************F*/
PingManagerRefT *PingManagerCreate(uint32_t uMaxRecords)
{
    PingManagerRefT *pPingManager;
    uint32_t uSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // already created?  if so, just ref it
    if (_PingManager_pRef != NULL)
    {
        _PingManager_iRefCount++;
        return(_PingManager_pRef);
    }
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // ensure minimum cache size
    if (uMaxRecords < 16)
    {
        NetPrintf(("pingmanager: warning -- create call specifies a max record size that is too small\n"));
        uMaxRecords = 16;
    }

    // calculate size of ping manager
    uSize = sizeof(PingManagerRefT) + ((uMaxRecords-1) * sizeof(PingRecT));

    // allocate module state
    if ((pPingManager = DirtyMemAlloc(uSize, PINGMGR_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        return(NULL);
    }

    // initialize module state
    memset(pPingManager, 0, uSize);
    pPingManager->iMemGroup = iMemGroup;
    pPingManager->pMemGroupUserData = pMemGroupUserData;

    // save max record count
    pPingManager->uMaxRecords = uMaxRecords;

    // init tick count
    pPingManager->uTickCount = pPingManager->uLastPingTick = NetTick();


    // instantiate ping module & set timeout
    if ((pPingManager->pPingRef = ProtoPingCreate(0)) == NULL)
    {
#if defined(DIRTYCODE_PC)
        pPingManager->pQosRef = QosApiCreate(_PingManagerQosCallback, pPingManager, 17552);

        if (pPingManager->pQosRef == NULL)
        {
            DirtyMemFree(pPingManager, PINGMGR_MEMID, pPingManager->iMemGroup, pPingManager->pMemGroupUserData);
            return(NULL);
        }
#else
        DirtyMemFree(pPingManager, PINGMGR_MEMID, pPingManager->iMemGroup, pPingManager->pMemGroupUserData);
        return(NULL);
#endif
    }
    ProtoPingControl(pPingManager->pPingRef, 'time', PINGMGR_TIMEOUT, NULL);

    // set global ref and refcount
    _PingManager_pRef = pPingManager;
    _PingManager_iRefCount = 1;

    // return module ref to caller
    return(pPingManager);
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerDestroy

    \Description
        Shut down PingManager module.

    \Input  *pPingManager   - ping manager ref

    \Notes
        As only one copy of the Ping Manager is instantiated, this call only destroys
        the Ping Manager if the internal reference count is zero.

    \Version 08/21/03 (JLB)
*/
/*************************************************************************************************F*/
void PingManagerDestroy(PingManagerRefT *pPingManager)
{
    // decrement refcount
    if (--_PingManager_iRefCount > 0)
    {
        return;
    }

#if defined(DIRTYCODE_PC)
    if (pPingManager->pQosRef)
    {
        QosApiDestroy(pPingManager->pQosRef);
        pPingManager->pQosRef = NULL;
    }
#endif

    // destroy ping manager ref
    ProtoPingDestroy(pPingManager->pPingRef);
    DirtyMemFree(pPingManager, PINGMGR_MEMID, pPingManager->iMemGroup, pPingManager->pMemGroupUserData);

    // reset global ref and refcount
    _PingManager_pRef = NULL;
    _PingManager_iRefCount = 0;
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerUpdate

    \Description
        Give life to PingManager.

    \Input  *pPingManager   - ping manager ref

    \Version 08/21/03 (JLB)
*/
/*************************************************************************************************F*/
void PingManagerUpdate(PingManagerRefT *pPingManager)
{
    uint32_t uRecord, uPingCount, uCurTick, uMaxPingIssue;
    ProtoPingResponseT PingResp;
    PingRecT *pPingCache, *pRecord;
    int32_t iIndex;

    // ref ping cache
    pPingCache = pPingManager->PingCache;

    // get current tick count
    uCurTick = NetTick();

    // update timer
    _PingManagerUpdateTimer(pPingManager, uCurTick);

    // calculate the max number of pings we can issue this update
    uMaxPingIssue = (uCurTick - pPingManager->uLastPingTick) / PINGMGR_TICKSPERPING;

    // re-ping stale records, issue new requests, and timeout requests
    for (uRecord = 0, uPingCount = 0; uRecord < pPingManager->uNumRecords; uRecord++)
    {
        // ref record
        pRecord = &pPingCache[uRecord];

        // do we have a stale record?
        if (pRecord->uCreatedTime == pPingManager->uTime)
        {
            // if it hasn't been touched for PINGMGR_STALEEXPIRETIME, expire it
            if ((PINGMGR_CalcTimeDiff(pPingManager->uTime, pRecord->uTouchedTime) >= PINGMGR_STALEEXPIRETIME))
            {
                #if PINGMGR_DEBUG
                NetPrintf(("pingmanager: expiring address %s due to age\n", pRecord->Addr.strMachineAddr));
                #endif
                // expire this record by copying last record over it
                ds_memcpy(pRecord, &pPingCache[pPingManager->uNumRecords-1], sizeof(PingRecT));
                pPingManager->uNumRecords--;
            }
            else // has been touched since creation, so re-ping
            {
                #if PINGMGR_DEBUG
                NetPrintf(("pingmanager: invalidating address %s due to age\n", pRecord->Addr.strMachineAddr));
                #endif
                _PingManagerInvalidateRecord(pPingManager, pRecord);
            }
        }

        if( pPingManager->pPingRef != NULL )
        {
            // issue a new request?
            if ((pRecord->uPing & PINGMGR_PINGFLAG_NEWPING) && (uPingCount < uMaxPingIssue))
            {
                #if PINGMGR_DEBUG
                NetPrintf(("pingmanager: pinging address %s\n", pRecord->Addr.strMachineAddr));
                #endif

                ProtoPingRequest(pPingManager->pPingRef, &pRecord->Addr, NULL, 0, 0, 0);
                PINGMGR_SetFlag(pRecord->uPing, PINGMGR_PINGFLAG_PINGING);
                pRecord->uCreatedTime = pRecord->uTouchedTime = pPingManager->uTime-1;
                pPingManager->uLastPingTick = uCurTick;
                uPingCount++;
            }
        }
    }

    if( pPingManager->pPingRef != NULL )
    {
        // handle ping responses
        for ( ; ProtoPingResponse(pPingManager->pPingRef, NULL, NULL, &PingResp) != 0; )
        {
            // if it was a server ping, trigger the callback immediately (no cacheing)
            if (PingResp.bServer == TRUE)
            {
                // find the server ping record
                ServerPingRecT *pServerPing;
                int32_t iServerPing;
                for (iServerPing = 0; iServerPing < PINGMGR_MAXSERVERPINGS; iServerPing++)
                {
                    pServerPing = &pPingManager->ServerPings[iServerPing];
                    if (pServerPing->uSeqn == PingResp.uSeqn)
                    {
                        // issue callback if present
                        #if PINGMGR_DEBUG
                        NetPrintf(("pingmanager: server ping entry=%d\n", iServerPing));
                        #endif
                        if (pServerPing->pServCb != NULL)
                        {
                            pServerPing->pServCb(&PingResp.Addr, PingResp.iPing, pServerPing->pServCbData);
                        }
                        // deallocate entry
                        pServerPing->uSeqn = 0;
                        break;
                    }
                }
                continue;
            }

            // find matching ping record
            if ((iIndex = _PingManagerFindRecordByAddress(pPingManager, &PingResp.Addr)) != -1)
            {
                pRecord = &pPingCache[iIndex];
                
                // handle ping timeouts
                if (PingResp.iPing > 0)
                {
                    // update ping
                    pRecord->uPing = (PingResp.iPing < PINGMGR_MAXPINGVAL) ? (uint16_t)PingResp.iPing : PINGMGR_MAXPINGVAL;
                    #if PINGMGR_DEBUG
                    NetPrintf(("pingmanager: %dms ping response from address %s\n", pRecord->uPing, PingResp.Addr.strMachineAddr));
                    #endif

                    // issue callback if present
                    if (pRecord->pCallback != NULL)
                    {
                        // call callback, and clear so it isn't called again
                        pRecord->pCallback(&PingResp.Addr, PingResp.iPing, pRecord->pCallbackData);
                        pRecord->pCallback = NULL;
                    }
                }
                else
                {
                    #if PINGMGR_DEBUG
                    NetPrintf(("pingmanager: ping of address %s timed out\n", pRecord->Addr.strMachineAddr));
                    #endif
                    pRecord->uPing = PINGMGR_PINGFLAG_TIMEOUT;
                
                    // issue callback if present
                    if (pRecord->pCallback != NULL)
                    {
                        // call callback, and clear so it isn't called again
                        pRecord->pCallback(&pRecord->Addr, 0, pRecord->pCallbackData);
                        pRecord->pCallback = NULL;
                    }
                }
            }
            // no match
            else
            {
                NetPrintf(("pingmanager: warning - got a ping response for address %a not in the cache\n", PingResp.uAddr));
                pRecord = NULL;
            }
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerPingAddress

    \Description
        Ping an address.

    \Input  *pPingManager   - ping manager ref
    \Input  *pAddr          - address to ping
    \Input  *pCallback      - callback to call on ping response
    \Input  *pCallbackData  - user data for callback

    \Output
        int32_t             - ping value for given address, can also be PINGMGR_INVALID or PINGMGR_PINGING 

    \Version 08/21/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t PingManagerPingAddress(PingManagerRefT *pPingManager, DirtyAddrT *pAddr, PingManagerCallbackT *pCallback, void *pCallbackData)
{
    PingRecT *pRecord;
    int32_t iIndex;

    // search for record - do we already have it?
    if ((iIndex = _PingManagerFindRecordByAddress(pPingManager, pAddr)) != -1)
    {
        // ref record
        pRecord = &pPingManager->PingCache[iIndex];

        // update last touched timestamp
        pRecord->uTouchedTime = pPingManager->uTime-1;
    }
    else // if no record, allocate one from cache
    {
        // cache full?
        if (pPingManager->uNumRecords == pPingManager->uMaxRecords)
        {
            // find LRU record to replace
            iIndex = _PingManagerFindRecordLRU(pPingManager);

            // overwrite the record
            pRecord = &pPingManager->PingCache[iIndex];

            #if PINGMGR_DEBUG
            NetPrintf(("pingmanager: Invalidating address %s due to overflow\n", pRecord->Addr.strMachineAddr));
            #endif
        }
        else
        {
            // append record to end of cache
            pRecord = &pPingManager->PingCache[pPingManager->uNumRecords++];
        }

        #if PINGMGR_DEBUG
        NetPrintf(("pingmanager: Adding addr %s to ping cache (%d/%d)\n",
            pAddr->strMachineAddr, pPingManager->uNumRecords, pPingManager->uMaxRecords));
        #endif

        // init record
        ds_memcpy(&pRecord->Addr, pAddr, sizeof(pRecord->Addr));
        pRecord->pCallback = pCallback;
        pRecord->pCallbackData = pCallbackData;
        _PingManagerInvalidateRecord(pPingManager, pRecord);
    }

    // return current ping value to caller
    return(_PingManagerGetPing(pRecord->uPing));
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerPingServer

    \Description
        Ping server.  This is a direct pass-through to ProtoPingRequestServer.

    \Input  *pPingManager   - ping manager ref
    \Input  uServerAddress  - server address
    \Input  *pCallback      - callback to call on ping response
    \Input  *pCallbackData  - user data for callback

    \Output
        int32_t             - response code from ProtoPingRequestServer 

    \Version 04/11/04 (JLB)
*/
/*************************************************************************************************F*/
int32_t PingManagerPingServer(PingManagerRefT *pPingManager, uint32_t uServerAddress, PingManagerCallbackT *pCallback, void *pCallbackData)
{
    #if defined(DIRTYCODE_XENON)
    // force ping of security gateway
    uServerAddress = 0;
    #endif
    return(PingManagerPingServer2(pPingManager, uServerAddress, pCallback, pCallbackData));
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerPingServer2

    \Description
        Ping server.  This is a direct pass-through to ProtoPingRequestServer.

    \Input  *pPingManager   - ping manager ref
    \Input  uServerAddress  - server address
    \Input  *pCallback      - callback to call on ping response
    \Input  *pCallbackData  - user data for callback

    \Output
        int32_t           - response code from ProtoPingRequestServer 

    \Version 04/11/04 (JLB)
*/
/*************************************************************************************************F*/
int32_t PingManagerPingServer2(PingManagerRefT *pPingManager, uint32_t uServerAddress, PingManagerCallbackT *pCallback, void *pCallbackData)
{
    ServerPingRecT *pServerPing;
    int32_t iServerPing;

    // find an empty server ping record
    for (iServerPing = 0, pServerPing = NULL; iServerPing < PINGMGR_MAXSERVERPINGS; iServerPing++)
    {
        if (pPingManager->ServerPings[iServerPing].uSeqn == 0)
        {
            #if PINGMGR_DEBUG
            NetPrintf(("pingmanager: allocating server ping entry %d\n", iServerPing));
            #endif
            pServerPing = &pPingManager->ServerPings[iServerPing];
            break;
        }
    }
    if (pServerPing == NULL)
    {
        NetPrintf(("pingmanager: server ping record overflow\n"));
        return(-1);
    }

    pServerPing->uAddr = uServerAddress;

#if defined(DIRTYCODE_PC)
    if (pPingManager->pQosRef)
    {
        ds_snzprintf(pPingManager->strServerAddress, sizeof(pPingManager->strServerAddress), "%d.%d.%d.%d", ((uServerAddress >> 24) & 255), ((uServerAddress >> 16) & 255), ((uServerAddress >> 8) & 255), (uServerAddress & 255));

        NetPrintf(("pingmanager: Using QoS to ping address %s\n", pPingManager->strServerAddress ));

        pServerPing->pServCb = pCallback;
        pServerPing->pServCbData = pCallbackData;
        pServerPing->uSeqn = QosApiServiceRequest(pPingManager->pQosRef, pPingManager->strServerAddress, 0, 0, 10, 0, QOSAPI_REQUESTFL_LATENCY);
        return(pServerPing->uSeqn);
    }
#endif

    pServerPing->pServCb = pCallback;
    pServerPing->pServCbData = pCallbackData;
    pServerPing->uSeqn = ProtoPingRequestServer(pPingManager->pPingRef, uServerAddress, NULL, 0, 0, 0);
    return(pServerPing->uSeqn);
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerInvalidateAddress

    \Description
        Invalidate an address in the cache.  This forces PingManager to issue a new ping the next time
        PingManagerPingAddress() is called with this address.

    \Input  *pPingManager   - ping manager ref
    \Input  *pAddr          - address to invalidate

    \Output
        uint32_t            - TRUE if address was found in cache, else FALSE

    \Version 09/04/03 (JLB)
*/
/*************************************************************************************************F*/
int32_t PingManagerInvalidateAddress(PingManagerRefT *pPingManager, DirtyAddrT *pAddr)
{
    int32_t iIndex;

    // find record
    if ((iIndex = _PingManagerFindRecordByAddress(pPingManager, pAddr)) != -1)
    {
        // invalidate record
        _PingManagerInvalidateRecord(pPingManager, &pPingManager->PingCache[iIndex]);
        memset(&pPingManager->PingCache[iIndex].Addr, 0, sizeof(pPingManager->PingCache[iIndex].Addr));
        return(TRUE);
    }
    else
    {
        return(FALSE);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerInvalidateCache

    \Description
        Invalidate all addresses in the cache.

        WARNING:
            This function should be used very sparingly if at all, as indescriminate
            use invalidates the purpose of the address cache, and will result in a
            very network-unfriendly title.  To get a fresh ping for a single user use
            PingManagerInvalidateAddress() on that user's address, and then re-ping that
            user with PingManagerPingAddress().

    \Input  *pPingManager   - ping manager ref

    \Version 03/19/04 (JLB)
*/
/*************************************************************************************************F*/
void PingManagerInvalidateCache(PingManagerRefT *pPingManager)
{
    uint32_t uRecord;

    // go through all cache records
    for (uRecord = 0; uRecord < pPingManager->uNumRecords; uRecord++)
    {
        // invalidate record
        _PingManagerInvalidateRecord(pPingManager, &pPingManager->PingCache[uRecord]);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerCancelRequest

    \Description
        Cancel an outstanding ping request callback.

    \Input  *pPingManager   - ping manager ref
    \Input  *pAddr          - address to cancel request for

    \Version 07/12/04 (JLB)
*/
/*************************************************************************************************F*/
void PingManagerCancelRequest(PingManagerRefT *pPingManager, DirtyAddrT *pAddr)
{
    uint32_t uRecord;
    PingRecT *pRecord;

    for (uRecord = 0; uRecord < pPingManager->uNumRecords; uRecord++)
    {
        pRecord = &pPingManager->PingCache[uRecord];

        // cancel request?
        if ((pAddr == NULL) || (DirtyAddrCompare(&pRecord->Addr, pAddr)))
        {
            pRecord->pCallback = NULL;
            pRecord->pCallbackData = NULL;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    PingManagerCancelServerRequest

    \Description
        Cancel an outstanding server ping request callback.

    \Input  *pPingManager   - ping manager ref
    \Input  uSeqn           - sequence for request to cancel, or zero to cancell all requests

    \Version 06/12/07 (JLB)
*/
/*************************************************************************************************F*/
void PingManagerCancelServerRequest(PingManagerRefT *pPingManager, uint32_t uSeqn)
{
    int32_t iServerPing;

    // find the request
    for (iServerPing = 0; iServerPing < PINGMGR_MAXSERVERPINGS; iServerPing++)
    {
        if ((pPingManager->ServerPings[iServerPing].uSeqn == uSeqn) || (uSeqn == 0))
        {
            memset(&pPingManager->ServerPings[iServerPing], 0, sizeof(pPingManager->ServerPings[0]));
        }
    }
    // could not find request to cancel?
    if ((uSeqn != 0) && (iServerPing == PINGMGR_MAXSERVERPINGS))
    {
        NetPrintf(("pingmanager: could not find server request with seqn=%d to cancel\n", uSeqn));
    }
}
