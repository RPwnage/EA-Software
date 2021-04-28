/*H********************************************************************************/
/*!
    \File netresource.h

    \Description
        Resource downloader/caching utility

    \Notes
        As several GDDs now require the use of dynamic downloaded images,
        some unified method of downloading these resources is required.
        The network resource manager would provide game titles with a simple
        method to download arbitrary resources (binary objects of any
        underlying type) via a simple type/identifier combination. Each game
        will have its own unique type/identifier mapping to avoid conflicts
        with other titles.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************H*/

#ifndef _netresource_h
#define _netresource_h

/*!
\Moduledef NetResource NetResource
\Modulemember Misc
*/
//@{

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define NETRSRC_STAT_IDLE               (0)   //!< not doing anything
#define NETRSRC_STAT_START              (1)   //!< start of transfer
#define NETRSRC_STAT_SIZE               (2)   //!< size of object known (allocate buffer)
#define NETRSRC_STAT_DATA               (3)   //!< append data to buffer
#define NETRSRC_STAT_DONE               (4)   //!< transfer is complete (hand off data)
#define NETRSRC_STAT_ERROR              (5)   //!< transfer failed
#define NETRSRC_STAT_CANCEL             (6)   //!< transfer was canceled (release buffer)
#define NETRSRC_STAT_TIMEOUT            (7)   //!< transfer timed out
#define NETRSRC_STAT_MISSING            (8)   //!< resource is not on the server
#define NETRSRC_STAT_FINISH             (9)   //!< finished – release resources

#define NETRSRC_PRIO_DONTCACHE         (-1)   //!< don't cache the incoming data whatsoever (one-time use data to download, like rosters etc.)
#define NETRSRC_PRIO_DEFAULT            (0)   //!< default - don't do anything special to keep or discard data in the cache
#define NETRSRC_PRIO_KEEPIFPOSSIBLE     (1)   //!< keep the data in the cache if at all possible (commonly used data)

#define NETRSRC_ERROR_NONE              (0)   //!< no error
#define NETRSRC_ERROR_BADNAME          (-1)   //!< bad resource string name passed to fetch function

#define NETRSRC_CACHESTATUS_LOADING  (0x01)   //! loading from the cache
#define NETRSRC_CACHESTATUS_SAVING   (0x02)   //! saving to the cache

/*** Macros ***********************************************************************/

#define NETRSRC_PRINTFTYPEIDENT(uType, uIdent)  ((uType) >> 24) & 0xFF, \
    ((uType) >> 16) & 0xFF, \
    ((uType) >> 8) & 0xFF, \
    (uType) & 0xFF, (uIdent)

/*** Type Definitions *************************************************************/

// NetResource state structure
typedef struct NetResourceT NetResourceT;

// NetResource Transfer Descriptor Structure
typedef struct NetResourceXferT NetResourceXferT;

// Callback prototype used for download resources
typedef void (NetResourceCallbackT)(NetResourceT *pState, NetResourceXferT *pXfer, void *pDataBuf, int32_t iDataLen);

//! NetResource Transfer Descriptor Structure
struct NetResourceXferT
{
    //! public data monitored by the game
    int32_t     iSeqNum;        //!< sequence number for the transaction
    uint32_t    uType;          //!< type of the resource
    uint32_t    uIdent;         //!< ident of the resource
    int32_t     iStatus;        //!< NETRSRC_STAT_* (game team monitors this to determine what state we are in)
    int32_t     iLength;        //!< total object size (set by NetResource when the SIZE command returns and used by the game team to allocate a chunk of memory for pBuffer)
    int16_t     iPriority;      //!< priority in the cache
    uint16_t    uCacheStatus;   //!< cache status (SAVING or LOADING)

    //! convenience data: user-allocated and user-managed data (not used by NetResource at all)
    void        *pBuffer;       //!< buffer pointer for game - the game will allocate memory for this variable after SIZE status when size of incoming data is known.
    uint32_t    uAmtReceived;   //!< used by game - for keeping track of how much data has been received
    int32_t     iRefVal1;       //!< custom user data  - The following values are for game-team use
    int32_t     iRefVal2;       //!< custom user data  - (possibly used for window ID so the user-defined callback knows what to do
    void        *pRefPtr1;      //!< custom user data  - with the resource when it is asynchronously received)
    void        *pRefPtr2;      //!< custom user data
};

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Create network resource manager instance.
DIRTYCODE_API NetResourceT *NetResourceCreate(const char *pHome);

// Destroy the network resource manager instance.
DIRTYCODE_API void NetResourceDestroy(NetResourceT *pState);

// Set the external cache buffer
DIRTYCODE_API int32_t NetResourceCache(NetResourceT *pState, void *pCacheBuf, const int32_t iCacheLen);

// Check to see if the cache contains the given resource
DIRTYCODE_API int32_t NetResourceCacheCheck(NetResourceT *pState, const char *pResource, uint32_t uType, uint32_t uIdent);

// Start a network transfer from a number based resource
DIRTYCODE_API int32_t NetResourceFetch(NetResourceT *pState, NetResourceXferT *pXfer,
                     const uint32_t uType, const uint32_t uIdent,
                     NetResourceCallbackT *pCallback, const int32_t iTimeout,
                     const int32_t iPriority);

// Start a network transfer from a string based resource, return sequence number
DIRTYCODE_API int32_t NetResourceFetchString(NetResourceT *pState, NetResourceXferT *pXfer,
                           const char *pResource,
                           NetResourceCallbackT *pCallback, const int32_t iTimeout,
                           const int32_t iPriority);

// Cancel an in-progress network transfer using the sequence number from Fetch
DIRTYCODE_API void NetResourceCancel(NetResourceT *pState, int32_t iSeqNum);

#if DIRTYCODE_LOGGING
// useful debugging routine for printing the contents of the cache
DIRTYCODE_API void NetResourceCachePrint(NetResourceT *pState);
#endif

#ifdef __cplusplus
};
#endif

//@}

#endif // _netresource_h

