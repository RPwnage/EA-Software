/*H*************************************************************************************/
/*!

    \File    protopingmanager.h

    \Description
        ProtoPingManager is a high-level interface to ProtoPing.  It provides both
        metering and and LRU caching of ping requests, so as to enable high-level
        applications to issue high volume ping requests without worrying about over-
        burdening the EE<->IOP interface or creating network-unfriendly ping storms.

    \Copyright
        Copyright (c) Electronic Arts 2003-2007.

    \Version 1.0 08/20/2003 (jbrookes) First Version
    \Version 1.1 02/06/2007 (jbrookes) Moved from Lobby to Proto
*/
/*************************************************************************************H*/


#ifndef _protopingmanager_h
#define _protopingmanager_h

/*!
\Module Proto
*/
//@{

/*** Include files *********************************************************************/

/*** Defines ***************************************************************************/

//! this address is currently being pinged
#define PINGMGR_PINGSTATUS_PINGING  (-1)

//! ping of this address timed out
#define PINGMGR_PINGSTATUS_TIMEOUT  (-2)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! ping manager ref
typedef struct PingManagerRefT PingManagerRefT;

//! ping manager callback type
typedef void (PingManagerCallbackT)(DirtyAddrT *pAddr, uint32_t uPing, void *pUserData);

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
PingManagerRefT *PingManagerCreate(uint32_t uMaxRecords);

// destroy the module
void PingManagerDestroy(PingManagerRefT *pPingManager);

// periodic update routine
void PingManagerUpdate(PingManagerRefT *pPingManager);

// ping an address
int32_t  PingManagerPingAddress(PingManagerRefT *pPingManager, DirtyAddrT *pAddr, PingManagerCallbackT *pCallback, void *pCallbackData);

// ping server
int32_t  PingManagerPingServer(PingManagerRefT *pPingManager, uint32_t uServerAddress, PingManagerCallbackT *pCallback, void *pCallbackData);

// ping server (on xenon, does not overwrite uServerAddress with zero)
int32_t  PingManagerPingServer2(PingManagerRefT *pPingManager, uint32_t uServerAddress, PingManagerCallbackT *pCallback, void *pCallbackData);

// invalidate a single address in the address cache
int32_t  PingManagerInvalidateAddress(PingManagerRefT *pPingManager, DirtyAddrT *pAddr);

// invalidate all addresses in the address cache (USE SPARINGLY IF AT ALL)
void PingManagerInvalidateCache(PingManagerRefT *pPingManager);

// cancel an outstanding ping request callback
void PingManagerCancelRequest(PingManagerRefT *pPingManager, DirtyAddrT *pAddr);

// cancel an outstanding server ping request callback
void PingManagerCancelServerRequest(PingManagerRefT *pPingManager, uint32_t uSeqn);

#ifdef __cplusplus
}
#endif

//@}

#endif // _pingmanager_h

