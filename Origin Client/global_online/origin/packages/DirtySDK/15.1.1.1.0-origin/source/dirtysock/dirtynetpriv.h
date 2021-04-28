/*H*************************************************************************************/
/*!
    \File    dirtynetpriv.h

    \Description
        Private include for platform independent interface to network layers.

    \Copyright
        Copyright (c) Electronic Arts 2002-2014

    \Version 1.0 08/07/2014 (jbrookes) First version, split from dirtynet.h
*/
/*************************************************************************************H*/

#ifndef _dirtynetpriv_h
#define _dirtynetpriv_h

/*!
\Moduledef DirtyNetPriv DirtyNetPriv
\Modulemember DirtySock
*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtynet.h"

/*** Defines ***************************************************************************/

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! socket hostname cache
typedef struct SocketHostnameCacheT SocketHostnameCacheT;

//! socket packet queue
typedef struct SocketPacketQueueT SocketPacketQueueT;

//! socket packet queue entry
typedef struct SocketPacketQueueEntryT
{
    int32_t iPacketSize;                        //!< packet size
    struct sockaddr PacketAddr;                 //!< packet source
    uint32_t uPacketTick;                       //!< tick packet was added to the queue
    uint8_t aPacketData[SOCKET_MAXUDPRECV];     //!< packet data
} SocketPacketQueueEntryT;

//! socket rate estimation
typedef struct SocketRateT
{
    uint32_t uMaxRate;          //!< maximum transfer rate in bytes/sec (zero=no limit)
    uint32_t uCurRate;          //!< current transfer rate in bytes/sec
    uint32_t uNextRate;         //!< estimated rate at next update
    uint32_t uLastTick;         //!< last update tick
    uint32_t uLastRateTick;     //!< tick count at last rate update
    uint32_t aTickHist[16];     //!< tick history (when update was recorded)
    uint32_t aDataHist[16];     //!< data history (how much data was sent during update)
    uint8_t  aCallHist[16];     //!< call history (how many times we were updated))
    uint8_t  uDataIndex;        //!< current update index
    uint8_t  _pad[3];
} SocketRateT;


/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*
    HostName Cache functions
*/

// create hostname cache
SocketHostnameCacheT *SocketHostnameCacheCreate(int32_t iMemGroup, void *pMemGroupUserData);

// destroy hostname cache
void SocketHostnameCacheDestroy(SocketHostnameCacheT *pCache);

// add entry to hostname cache
void SocketHostnameCacheAdd(SocketHostnameCacheT *pCache, const char *pStrHost, uint32_t uAddress, int32_t iVerbose);

// get entry from hostname cache
uint32_t SocketHostnameCacheGet(SocketHostnameCacheT *pCache, const char *pStrHost, int32_t iVerbose);

// check for refcounted (in-progress) hostname lookup then addref, or force using a new entry if bUseRef=false
HostentT *SocketHostnameAddRef(HostentT **ppHostList, HostentT *pHost, uint8_t bUseRef);

// process hostname list, delete completed lookups
void SocketHostnameListProcess(HostentT **ppHostList, int32_t iMemGroup, void *pMemGroupUserData);

/*
    Packet Queue functions
*/

// create packet queue
SocketPacketQueueT *SocketPacketQueueCreate(int32_t iMaxPackets, int32_t iMemGroup, void *pMemGroupUserData);

// destroy packet queue
void SocketPacketQueueDestroy(SocketPacketQueueT *pPacketQueue);

// resize a packet queue
SocketPacketQueueT *SocketPacketQueueResize(SocketPacketQueueT *pPacketQueue, int32_t iMaxPackets, int32_t iMemGroup, void *pMemGroupUserData);

// packet queue control function
int32_t SocketPacketQueueControl(SocketPacketQueueT *pPacketQueue, int32_t iControl, int32_t iValue);

// add to packet queue
int32_t SocketPacketQueueAdd(SocketPacketQueueT *pPacketQueue, const uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr);

// alloc packet queue entry
SocketPacketQueueEntryT *SocketPacketQueueAlloc(SocketPacketQueueT *pPacketQueue);

// remove from packet queue
int32_t SocketPacketQueueRem(SocketPacketQueueT *pPacketQueue, uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr);

/*
    Rate functions
*/

// update socket data rate calculations
void SocketRateUpdate(SocketRateT *pRate, int32_t iData, const char *pOpName);

// throttle based on socket data rate calcuations and configured max rate
int32_t SocketRateThrottle(SocketRateT *pRate, int32_t iSockType, int32_t iData, const char *pOpName);

/*
    Send Callback functions
*/

// register a new socket send callback
int32_t SocketSendCallbackAdd(SocketSendCallbackEntryT aCbList[], SocketSendCallbackEntryT *pCbEntry);

// removes a socket send callback previously registered
int32_t SocketSendCallbackRem(SocketSendCallbackEntryT aCbList[], SocketSendCallbackEntryT *pCbEntry);


#ifdef __cplusplus
}
#endif

//@}

#endif // _dirtynetpriv_h


