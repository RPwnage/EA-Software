/*H********************************************************************************/
/*!
    \File netresource.c

    \Description
        Resource downloader/caching utility

    \Notes
        The network resource manager provides game titles with a simple
        method to download arbitrary resources (binary objects of any
        underlying type) via a simple type/identifier combination. Each game
        will have its own unique type/identifier mapping to avoid conflicts
        with other titles.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "DirtySDK/proto/protohttp.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/web/netresource.h"

/*** Defines **********************************************************************/

// set this to TRUE if more output is desired
#define NETRSRC_VERBOSE (DIRTYCODE_DEBUG && FALSE)

#define NETRSRC_MAXURLLENGTH        (128)       //!< maximum length of a URL
#define NETRSRC_HTTPSPOOLSIZE       (4096)      //!< spool size for http engine
#define NETRSRC_RECVBUFFERSIZE      (1024)      //!< internal buffer for data from httprecv

/*** Type Definitions *************************************************************/

typedef struct NetResourceControlT NetResourceControlT;
typedef struct NetResourceCacheUsedListT NetResourceCacheUsedListT;

//! NetResource state structure
struct NetResourceT
{
    ProtoHttpRefT               *pHttp;             //!< http module used for web transactions

    // module memory group
    int32_t                     iMemGroup;          //!< module mem group id
    void                        *pMemGroupUserData; //!< user data associated with mem group

    uint32_t                    uXferState;         //!< internal state used to track current state
    NetResourceControlT         *pXferCBList;       //!< list of xfer control block structures
    NetResourceControlT         *pCurrentXferCB;    //!< the current transfer block we're processing
    int32_t                     iRecvLength;        //!< amount of data received into pRecvBuf
    char                        *pCache;            //!< pointer to user-supplied memory cache
    int32_t                     iCacheLen;          //!< length of user-supplied memory cache
    int32_t                     iFreeCacheLen;      //!< amount of contiguous free cache memory after a compact is complete
    NetResourceCacheUsedListT   *pUsedList;         //!< pointer to usedlist structures
    int32_t                     iTotalSeqNum;       //!< global sequence number
    char                        pRecvBuffer[NETRSRC_RECVBUFFERSIZE];    //!< internal recv buffer
    char                        strHomeURL[NETRSRC_MAXURLLENGTH];       //!< URL of home server
};

//! NetResourceCacheUsedListT - cache memory management of used chunks in cache
struct NetResourceCacheUsedListT
{
    NetResourceCacheUsedListT   *pNext;         //!< pointer to next used list entry
    int32_t                     iOffset;        //!< offset into cache where used memory starts
    int32_t                     iLength;        //!< length of available memory at that location
    uint32_t                    uLastTick;      //!< last time the data was requested
    uint32_t                    uNumAccesses;   //!< number of times this data chunk was accessed
    uint32_t                    uType;          //!< type for this resource
    uint32_t                    uIdent;         //!< identity for this resource
    int16_t                     iPriority;      //!< priority of keeping this chunk in memory
    uint16_t                    uPad1;          //!< pad
};

// NetResourceControlT - control block for a netresource xfer
struct NetResourceControlT
{
    int32_t                     iSeqNum;        //!< sequence number of the transaction
    int32_t                     iStatus;        //!< status of the transaction
    NetResourceXferT            *pUserXfer;     //!< user-managed xfer structure pointer
    uint32_t                    uType;          //!< type of data to be downloaded - set by calling NetResourceFetch/NetResourceFetchString
    uint32_t                    uIdent;         //!< identity of data to be downloaded - set by calling NetResourceFetch/NetResourceFetchString
    int32_t                     iLength;        //!< total object size
    int32_t                     iTimeout;       //!< timeout limit (read-only - set by a parameter passed in to FETCH by the game)
    int16_t                     iPriority;      //!< priority in the cache
    uint16_t                    uCacheStatus;   //!< used for signifying loading/saving to the cache
    void                        *pCache;        //!< pointer to cache memory where incoming/outgoing data is stored
    int32_t                     iCacheLen;      //!< length of data in cache memory
    NetResourceControlT         *pPrev;         //!< pointer to prev xfer struct (read-only - used to maintain the linked list)
    NetResourceControlT         *pNext;         //!< pointer to next xfer struct (read-only - used to maintain the linked list)
    NetResourceCallbackT        *pCallback;     //!< callback to use for status changes (read-only - set by a parameter passed in to FETCH by the game)
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _NetResourceCallUserCallback

    \Description
        Call the user callback, copying in the necessary info to the xfer struct

    \Input *pState   - module state
    \Input *pXfer    - Xfer control structure
    \Input *pDataBuf - data buffer pointer to pass through
    \Input iDataLen  - length of data at databuf

    \Version 03/15/2005 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceCallUserCallback(NetResourceT *pState, NetResourceControlT *pXfer, void *pDataBuf, int32_t iDataLen)
{
    // if the xfer structure is non-null fill in some stuff
    if ((pXfer != NULL) && (pXfer->pCallback != NULL))
    {
        pXfer->pUserXfer->iLength       = pXfer->iLength;
        pXfer->pUserXfer->iStatus       = pXfer->iStatus;
        pXfer->pUserXfer->iSeqNum       = pXfer->iSeqNum;
        pXfer->pUserXfer->uType         = pXfer->uType;
        pXfer->pUserXfer->uIdent        = pXfer->uIdent;
        pXfer->pUserXfer->uCacheStatus  = pXfer->uCacheStatus;
        pXfer->pUserXfer->iPriority     = pXfer->iPriority;

        // call the callback
        pXfer->pCallback(pState, pXfer->pUserXfer, pDataBuf, iDataLen);
    }
}

/*F********************************************************************************/
/*!
    \Function _NetResourcePrintXferList

    \Description
        Print the NetResourceXferT list to the debug console

    \Input *pState  - module state

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceXferListPrint(NetResourceT *pState)
{
#if NETRSRC_VERBOSE
    NetResourceControlT *pLoop;
    uint32_t uCount = 0;

    NetPrintf(("netresource: Printing NetResourceXferT List.\n"));
    for(pLoop = pState->pXferCBList; pLoop != NULL; pLoop = pLoop->pNext)
    {
        NetPrintf(("netresource: (%c%c%c%c-%08X) pPrev[%8X] pXfer[%8X] pNext[%8X] Item[%3u]\n",
            NETRSRC_PRINTFTYPEIDENT(pLoop->uType, pLoop->uIdent),
            pLoop->pPrev, pLoop, pLoop->pNext, uCount));
        uCount++;
    }
    if(pState->pCurrentXferCB)
    {
        pLoop = pState->pCurrentXferCB;
        NetPrintf(("netresource: (%c%c%c%c-%08X) *pCurrentXferCB [0x%8X]\n",
            NETRSRC_PRINTFTYPEIDENT(pLoop->uType, pLoop->uIdent), pState->pCurrentXferCB));
    }
#endif
}

/*F********************************************************************************/
/*!
    \Function _NetResourceDeleteControlBlock

    \Description
        Delete the specified item in the control block list

    \Input *pState   - module state
    \Input iSeqNum   - sequence number of the item to delete

    \Version 03/15/2005 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceDeleteControlBlock(NetResourceT *pState, int32_t iSeqNum)
{
    NetResourceControlT *pLoop;

    // first find the requested control block
    for(pLoop = pState->pXferCBList; pLoop != NULL; pLoop = pLoop->pNext)
    {
        if(pLoop->iSeqNum == iSeqNum)
        {
            break;
        }
    }

    // if we found a matching block, delete it and patch the list
    if (pLoop)
    {
        // patch the list -- check for it being the first entry on the list
        if(pLoop->pPrev == NULL)
        {
            // first item on the list - adjust the list pointer
            pState->pXferCBList = pLoop->pNext;
            // if there's a next entry, patch backwards as well
            if(pLoop->pNext != NULL)
            {
                pLoop->pNext->pPrev = NULL;
            }
        }
        // otherwise patch the list
        else
        {
            pLoop->pPrev->pNext = pLoop->pNext;
            // if there's a next entry, patch backwards as well
            if(pLoop->pNext != NULL)
            {
                pLoop->pNext->pPrev = pLoop->pPrev;
            }
        }

        // now free the memory
        DirtyMemFree(pLoop, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _NetResourceXferListClear

    \Description
        Remove all entries from the XferT List
        (do not delete as the game team manages their own XferT structs)

    \Input *pState   - module state

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceXferListClear(NetResourceT *pState)
{
    NetResourceControlT *pXferEntry, *pNextXferEntry;

    // dump the Xfer list
    pXferEntry = pState->pXferCBList;
    while(pXferEntry != NULL)
    {
        pNextXferEntry = pXferEntry->pNext;
        pXferEntry = pNextXferEntry;
    }

    // set the references to NULL
    pState->pXferCBList = NULL;
}

/*F********************************************************************************/
/*!
    \Function _NetResourceCacheClear

    \Description
        Delete all entries in the cache and used list which references it

    \Input *pState   - module state

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceCacheClear(NetResourceT *pState)
{
    NetResourceCacheUsedListT *pUsedEntry, *pNextUsedEntry;

    // dump the used list
    pUsedEntry = pState->pUsedList;
    while(pUsedEntry != NULL)
    {
        pNextUsedEntry = pUsedEntry->pNext;
        DirtyMemFree(pUsedEntry, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pUsedEntry = pNextUsedEntry;
    }

    // set the pointers to NULL just in case
    pState->pCache = NULL;
    pState->pUsedList = NULL;
}

/*F********************************************************************************/
/*!
    \Function _NetResourceCacheDeleteXfer

    \Description
        Delete a specific entry in the cache from both the cache list and the used list

    \Input *pState   - module state
    \Input uType     - the type of the object in the cache to delete
    \Input uIdent    - the ident of the object in the cache to delete

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceCacheDeleteXfer(NetResourceT *pState, uint32_t uType, uint32_t uIdent)
{
    NetResourceControlT *pXfer, *pPrevXfer;
    NetResourceCacheUsedListT *pUsed, *pPrevUsed;

    // first delete from cache
    pXfer = pState->pXferCBList;
    pPrevXfer = NULL;
    while(pXfer != NULL)
    {
        if((pXfer->uType == uType) && (pXfer->uIdent == uIdent))
        {
            // reconnect the list
            if(pPrevXfer == NULL)
            {
                pState->pXferCBList = pXfer->pNext;
                if(pState->pXferCBList != NULL)
                {
                    pState->pXferCBList->pPrev = NULL;
                }
            }
            else
            {
                pPrevXfer->pNext = pXfer->pNext;
            }

            // and remember to restore the free cache length
            pState->iFreeCacheLen += pXfer->iLength;

            // delete the item since we just removed it
            DirtyMemFree(pXfer, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

            // signify that we found the item and we're done deleting
            pXfer = NULL;
        }
        // only advance if the xfer is non-null
        if(pXfer)
        {
            pPrevXfer = pXfer;
            pXfer = pXfer->pNext;
        }
    }

    // now dump from used list
    pUsed = pState->pUsedList;
    pPrevUsed = NULL;
    while(pUsed != NULL)
    {
        if((pUsed->uType == uType) && (pUsed->uIdent == uIdent))
        {
            // reconnect the list
            if(pPrevUsed == NULL)
            {
                pState->pUsedList = pUsed->pNext;
            }
            else
            {
                pPrevUsed->pNext = pUsed->pNext;
            }
            // now dump the memory
            DirtyMemFree(pUsed, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            // signify that we found the item and we're done deleting
            pUsed = NULL;
        }
        // only advance if the Used is non-null
        if(pUsed)
        {
            pPrevUsed = pUsed;
            pUsed = pUsed->pNext;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetResourceCacheDeleteSomethingFromCache

    \Description
        Delete something in the cache according to deletion heuristic

    \Input *pState - module state
    \Input *pXfer  - xfer module state

    \Output  -1=error (did not delete anything) 1=success (deleted something)

    \Notes
        Build a list of entries with the least number of accesses
        and then dump the oldest entry in that list

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static int32_t _NetResourceCacheDeleteSomethingFromCache(NetResourceT *pState, NetResourceControlT *pXfer)
{
    NetResourceCacheUsedListT *pEntry, *pPrevEntry;
    uint32_t uLeastNumAccesses = (uint32_t)(-1);    // set to something really big
    uint32_t uOldestTick = NetTick();               // set to "just born"
    int32_t iTotalDefaultSpace = 0;
    uint32_t uDeleteKeepers;

    // check to see if we have a list already
    if(pState->pUsedList == NULL)
    {
        return(-1);
    }

    // see if we need to delete KEEPIFPOSSIBLE entries (no DEFAULT entries left)
    pEntry = pState->pUsedList;
    while(pEntry != NULL)
    {
        /* if we have even one default entry left, delete if first
           otherwise everything in the cache is fair game */
        if(pEntry->iPriority == NETRSRC_PRIO_DEFAULT)
        {
            iTotalDefaultSpace += pEntry->iLength;
        }
        pEntry = pEntry->pNext;
    }

    /* check for the case where we would delete all the DEFAULT priority chunks and
       there's not enough room still for */
    if((pXfer->iPriority == NETRSRC_PRIO_DEFAULT) && (iTotalDefaultSpace < pXfer->iLength))
    {
        /* even if we deleted all the default entries, we still won't have space
           so don't bother - just don't cache it */
        return(-1);
    }
    /* otherwise, at this point, we know that one of the following is correct:
       1.  The priority is DEFAULT and we need to kill DEFAULTS to store it
       2.  The priority is KEEP and we need to kill DEFAULTS to store it
       3.  The priority is KEEP and we need to kill KEEP to store it */

    /* see if we need to delete keep priority cache chunks
       this will ensure that we delete defaults first */
    if(iTotalDefaultSpace > 0)
    {
        uDeleteKeepers = 0;
    }
    else
    {
        uDeleteKeepers = 1;
    }

    // first get the least number of accesses
    pEntry = pState->pUsedList;
    while(pEntry != NULL)
    {
        // see if the priority of the current chunk is KEEP and we should be keeping
        if((pEntry->iPriority == NETRSRC_PRIO_KEEPIFPOSSIBLE) && (uDeleteKeepers == 0))
        {
            // don't count this chunk
        }
        else
        {
            if(pEntry->uNumAccesses < uLeastNumAccesses)
            {
                uLeastNumAccesses = pEntry->uNumAccesses;
            }
        }
        pEntry = pEntry->pNext;
    }

    // now determine what the oldest item is
    pEntry = pState->pUsedList;
    while(pEntry != NULL)
    {
        // see if the priority of the current chunk is KEEP and we should be keeping
        if((pEntry->iPriority == NETRSRC_PRIO_KEEPIFPOSSIBLE) && (uDeleteKeepers == 0))
        {
            // don't count this chunk
        }
        else
        {
            if((pEntry->uNumAccesses == uLeastNumAccesses) && (pEntry->uLastTick < uOldestTick))
            {
                uOldestTick = pEntry->uLastTick;
            }
        }
        pEntry = pEntry->pNext;
    }

    // now dump the least used, oldest item
    pEntry = pState->pUsedList;
    pPrevEntry = NULL;
    while(pEntry != NULL)
    {
        if((pEntry->uNumAccesses == uLeastNumAccesses) && (pEntry->uLastTick == uOldestTick))
        {
            // this is the item to delete
            if(pPrevEntry == NULL)
            {
                // we're at the start of the list
                pState->pUsedList = pEntry->pNext;
            }
            else
            {
                // we're in the middle of the list
                pPrevEntry->pNext = pEntry->pNext;
            }
            // add the amount of cache we are gaining back in
            pState->iFreeCacheLen += pEntry->iLength;
            // dump the current entry and quit looping
            DirtyMemFree(pEntry, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            pEntry = NULL;
        }

        // check for null before advancing
        if(pEntry != NULL)
        {
            pPrevEntry = pEntry;
            pEntry = pEntry->pNext;
        }
    }
    // we successfully deleted something - return success code
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _NetResourceCacheCompact

    \Description
        Defrag memory by making it all contiguous

    \Input *pState  - module state

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceCacheCompact(NetResourceT *pState)
{
    NetResourceCacheUsedListT *pMoveablePtr;
    NetResourceCacheUsedListT *pUsedPtr;
    int32_t iTotalContiguous;               // the offset into the cache of the amount of contiguous memory
    int32_t iTotalUsed;                     // amount of used memory
    uint32_t uLoop;                         // loop variable

    if (pState->pCache == NULL)
    {
        return;
    }
    if (pState->pUsedList == NULL)
    {
        return;
    }

    iTotalContiguous = 0;
    iTotalUsed = pState->iCacheLen - pState->iFreeCacheLen;

    while(iTotalContiguous != iTotalUsed)
    {
        // find the lowest offset chunk which is also >= than the current total contiguous
        pMoveablePtr = NULL;
        pUsedPtr = pState->pUsedList;

        // first see if we're bigger than the total contiguous memory
        while(pUsedPtr != NULL)
        {
            if(pUsedPtr->iOffset >= iTotalContiguous)
            {
                if(pMoveablePtr == NULL)
                {
                    // we don't have a movable pointer yet
                    pMoveablePtr = pUsedPtr;
                }
                else if(pUsedPtr->iOffset < pMoveablePtr->iOffset)
                {
                    /* the current used pointer is lower in memory than
                       the one we're saving, so save the new one instead */
                    pMoveablePtr = pUsedPtr;
                }
            }
            pUsedPtr = pUsedPtr->pNext;
        }

        // now see what we got
        if(pMoveablePtr == NULL)
        {
            // error occurred
            NetPrintf(("netresource: cache compacting utility failed.\n"));
            return;
        }

        // now move the chunk if needed
        if(pMoveablePtr->iOffset > iTotalContiguous)
        {
            uint32_t uMemoryDistance = pMoveablePtr->iOffset - iTotalContiguous;
            for (uLoop = 0; uLoop < (uint32_t)pMoveablePtr->iLength; uLoop++)
            {
                pState->pCache[iTotalContiguous + uLoop] = pState->pCache[iTotalContiguous + uMemoryDistance + uLoop];
            }
            // reset the offset in the memory chunk which got moved
            pMoveablePtr->iOffset = iTotalContiguous;
        }
        // and move the contiguous offset forward
        iTotalContiguous += pMoveablePtr->iLength;
    }

    // save off the amount of memory left
    pState->iFreeCacheLen = pState->iCacheLen - iTotalContiguous;
}

/*F********************************************************************************/
/*!
    \Function _NetResourceCacheAssign

    \Description
        According to the caching priority, setup cache for this transaction

    \Input *pState  - module state
    \Input *pXfer   - transfer block state

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceCacheAssign(NetResourceT *pState, NetResourceControlT *pXfer)
{
    NetResourceCacheUsedListT *pUsed;   // pointer to entry in used list
    uint32_t uDone = 0;                 // signifies that we found a place for the data

    // see if there's a cache enabled
    if(pState->pCache == NULL)
    {
        return;
    }

    // now see if this transaction is allowed to be cached
    if(pXfer->iPriority == NETRSRC_PRIO_DONTCACHE)
    {
        #if NETRSRC_VERBOSE
        NetPrintf(("netresource: (%c%c%c%c-%08X) not caching resource because flag set to NETRSRC_PRIO_DONTCACHE.\n",
            NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
        #endif
        return;
    }

    // otherwise, find a spot for it
    while(!uDone)
    {
        // compact the available memory
        _NetResourceCacheCompact(pState);

        if(pState->iFreeCacheLen >= pXfer->iLength)
        {
            // create a new entry
            pUsed = (NetResourceCacheUsedListT *)DirtyMemAlloc(sizeof(*pUsed), NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
            memset(pUsed,0,sizeof(NetResourceCacheUsedListT));
            pUsed->iLength = pXfer->iLength;
            pUsed->iOffset = pState->iCacheLen - pState->iFreeCacheLen;
            pUsed->uLastTick = NetTick();
            pUsed->uNumAccesses = 1;
            pUsed->iPriority = pXfer->iPriority;
            pUsed->uIdent = pXfer->uIdent;
            pUsed->uType = pXfer->uType;

            // subtract off the cache space we're now using
            pState->iFreeCacheLen -= pXfer->iLength;

            // signify that we are saving to the cache
            pXfer->uCacheStatus = NETRSRC_CACHESTATUS_SAVING;
            pXfer->pCache = pState->pCache + pUsed->iOffset;
            pXfer->iCacheLen = 0;   // this is the offset into the cache buffer

            // now add it to the list
            if(pState->pUsedList == NULL)
            {
                // empty used list - create a new list
                pState->pUsedList = pUsed;
            }
            else
            {
                NetResourceCacheUsedListT *pList = pState->pUsedList;
                // create a new used block at the end of the list
                pList = pState->pUsedList;
                while(pList->pNext != NULL)
                {
                    pList = pList->pNext;
                }
                pList->pNext = pUsed;
            }
            uDone = 1;
        }
        else
        {
            // there's not enough room - delete something and try again
            if(_NetResourceCacheDeleteSomethingFromCache(pState, pXfer) < 0)
            {
                // we failed to delete anything - so bail and don't cache this entry
                uDone = 1;
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetResourceCacheSave

    \Description
        Save incoming data to the cache, if enabled

    \Input *pState      - module state
    \Input *pXfer       - transfer block state
    \Input *pDataBuf    - incoming data
    \Input iDataLen     - incoming data length

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceCacheSave(NetResourceT *pState, NetResourceControlT *pXfer, void *pDataBuf, int32_t iDataLen)
{
    if(pXfer->uCacheStatus & NETRSRC_CACHESTATUS_SAVING)
    {
        ds_memcpy(((char *)pXfer->pCache) + pXfer->iCacheLen, (char *)pDataBuf, iDataLen);
        pXfer->iCacheLen += iDataLen;
    }
}

/*F********************************************************************************/
/*!
    \Function _NetResourceCacheCheck

    \Description
        Check to see if the cache contains the given resource
        Assign cache pointer if it does.

    \Input *pState          - module state
    \Input *pXfer           - transfer block state
    \Input bSetPriority     - if TRUE set priority from Xfer block

    \Output
        NetResourceCacheUsedListT * - cache entry pointer (hit) or NULL (miss)

    \Version 05/15/2006 (jbrookes)
*/
/********************************************************************************F*/
static NetResourceCacheUsedListT *_NetResourceCacheCheck(NetResourceT *pState, NetResourceControlT *pXfer, uint32_t bSetPriority)
{
    NetResourceCacheUsedListT *pUsed = NULL;

    // initialize the cache variables
    pXfer->pCache = NULL;
    pXfer->iCacheLen = 0;
    pXfer->uCacheStatus = 0;

    // see if the cache even exists
    if (pState->pCache == NULL)
    {
        return(NULL);
    }

    // a cache exists, so walk the used list to see if the entry is in the cache.
    for (pUsed = pState->pUsedList; pUsed != NULL; pUsed = pUsed->pNext)
    {
        // skip non-matching entries
        if ((pUsed->uType != pXfer->uType) || (pUsed->uIdent != pXfer->uIdent))
        {
            continue;
        }

        // assign the cache pointer
        pXfer->pCache = pState->pCache + pUsed->iOffset;
        pXfer->iCacheLen = pUsed->iLength;
        pXfer->uCacheStatus = NETRSRC_CACHESTATUS_LOADING;

        // copy the new priority into the cache entry
        if (bSetPriority)
        {
            pUsed->iPriority = pXfer->iPriority;
        }

        // done
        break;
    }

    // return result to caller
    return(pUsed);
}

/*F********************************************************************************/
/*!
    \Function _NetResourceHttpErrorCheck

    \Description
        Check ProtoHTTP for errors and set module state if an error occurs

    \Input *pState  - module state
    \Input *pXfer   - transfer block state

    \Output -1=error, 0=in progress, 1=successful

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static int32_t _NetResourceHttpErrorCheck(NetResourceT *pState, NetResourceControlT *pXfer)
{
    int32_t iTimeout = ProtoHttpStatus(pState->pHttp, 'time', NULL, 0);
    int32_t iDone = ProtoHttpStatus(pState->pHttp, 'done', NULL, 0);
    int32_t iCode = ProtoHttpStatus(pState->pHttp, 'code', NULL, 0);

    #if NETRSRC_VERBOSE
    NetPrintf(("netresource: HttpErrorCheck: time=[%d] done=[%d] code=[%d]\n", iTimeout, iDone, iCode));
    #endif

    // did something time out?
    if (iTimeout)
    {
        pXfer->iStatus = NETRSRC_STAT_TIMEOUT;
        return(-1);
    }
    // did we get an error downloading?
    else if(iDone < 0)
    {
        // check for special case MISSING
        if (iCode == PROTOHTTP_RESPONSE_NOTFOUND)
        {
            pXfer->iStatus = NETRSRC_STAT_MISSING;
        }
        // otherwise, something just plain broke
        else
        {
            pXfer->iStatus = NETRSRC_STAT_ERROR;
        }
        ProtoHttpAbort(pState->pHttp);
        return(-1);
    }
    // check for a bad response code from the transaction
    else if((iCode > 0) && (iCode != PROTOHTTP_RESPONSE_OK))
    {
        // check for special case MISSING
        if (iCode == PROTOHTTP_RESPONSE_NOTFOUND)
        {
            pXfer->iStatus = NETRSRC_STAT_MISSING;
        }
        // otherwise, something just plain broke
        else
        {
            pXfer->iStatus = NETRSRC_STAT_ERROR;
        }
        ProtoHttpAbort(pState->pHttp);
        return(-1);
    }
    // no errors
    else
    {
        // return if we're in progress or done
        return(iDone);
    }
}

/*F********************************************************************************/
/*!
    \Function _NetResourceGetFromString

    \Description
        Convert a resource string into a Type and Ident.

    \Input *pResource       - string for resource to download[in]
    \Input *pType           - type of resource to download[out]
    \Input *pIdent          - identity of resource to download[out]

    \Output
        int32_t             - negative=error, 1=successful

    \Version 05/12/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _NetResourceGetFromString(const char *pResource, uint32_t *pType, uint32_t *pIdent)
{
    uint32_t uType = 0;
    uint32_t uIdent = 0;
    const char strNetRes[] = "netres:";
    char *pIdentEnd = NULL;

    // chop "netres:" prefix if present
    if (strncmp(pResource, strNetRes, strlen(strNetRes)) == 0)
    {
        pResource += strlen(strNetRes);
    }

    // make sure there are enough characters to parse (four id chars and one number minimum)
    if (strlen(pResource) < 5)
    {
        return(NETRSRC_ERROR_BADNAME);
    }

    // extract the type
    uType = (unsigned char)*pResource++;
    uType <<= 8;
    uType |= (unsigned char)*pResource++;
    uType <<= 8;
    uType |= (unsigned char)*pResource++;
    uType <<= 8;
    uType |= (unsigned char)*pResource++;

    // skip dash if present
    if (*pResource == '-')
    {
        pResource += 1;
    }

    // parse the identifier
    uIdent = (uint32_t)strtoul(pResource, &pIdentEnd, 16);

    *pType = uType;
    *pIdent = uIdent;

    if (pIdentEnd != NULL && pIdentEnd[0] != 0)
    {
        // probablly error occurred
        NetPrintf(("netresource: warning -- Identity string (%s) ignored in _NetResourceGetFromString.\n", pIdentEnd));
    }
    return(1);
}

/*F********************************************************************************/
/*!
    \Function _NetResourceUpdate2

    \Description
        Does the state transitions for the Update call.
        Called by _NetResourceUpdate, NetResourceCancel, and NetResourceFetch.

    \Input *pState  - module reference
    \Input *pXfer   - Transfer control structure to do work on

    \Notes
        Call this function at least 4 times per second for good performance

    \Version 03/15/2005 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceUpdate2(NetResourceT *pState, NetResourceControlT *pXfer)
{
    char strFullURL[NETRSRC_MAXURLLENGTH];
    int32_t iStatus;

    // we've got data - see what to do
    switch(pXfer->iStatus)
    {
        case NETRSRC_STAT_IDLE:     // waiting for something to do
        {
            NetResourceCacheUsedListT *pUsed;

            #if NETRSRC_VERBOSE
            NetPrintf(("netresource: (%c%c%c%c-%08X) :: IDLE\n",
                NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
            #endif
            /* the transfer list is non-empty and we are in IDLE state begin processing by
               setting the state to START and kicking the ProtoHTTP module into gear */
            pXfer->iStatus = NETRSRC_STAT_START;

            // see if the resource is in the cache already
            if ((pUsed = _NetResourceCacheCheck(pState, pXfer, TRUE)) != NULL)
            {
                #if NETRSRC_VERBOSE
                NetPrintf(("netresource: (%c%c%c%c-%08X) Cache HIT - Copying from cache.\n",
                    NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                #endif
                // adjust the metrics of the used entry we're using
                pUsed->uNumAccesses++;
                pUsed->uLastTick = NetTick();
            }
            else
            {
                #if NETRSRC_VERBOSE
                NetPrintf(("netresource: (%c%c%c%c-%08X) Cache MISS - Beginning HTTP Get.\n",
                    NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                #endif

                // create the fully-qualified URL
                memset(strFullURL,0,sizeof(strFullURL));
                ds_snzprintf(strFullURL, sizeof(strFullURL), "%s/%c%c%c%c-%08X", pState->strHomeURL,
                    NETRSRC_PRINTFTYPEIDENT(pXfer->uType,pXfer->uIdent));

                // set the timeout
                ProtoHttpControl(pState->pHttp, 'time', pXfer->iTimeout, 0, NULL);

                // begin the transfer
                ProtoHttpGet(pState->pHttp, strFullURL, PROTOHTTP_HEADBODY);
            }

            // call the callback for the START STATE
            _NetResourceCallUserCallback(pState, pXfer, NULL, 0);
            break;
        }
        case NETRSRC_STAT_START:    // start of transfer
        {
            // if we're getting from cache, just transition to the next state
            if(pXfer->uCacheStatus & NETRSRC_CACHESTATUS_LOADING)
            {
                // get the length from the length of the cache entry
                pXfer->iLength = pXfer->iCacheLen;
                pXfer->iStatus = NETRSRC_STAT_SIZE;
                #if NETRSRC_VERBOSE
                NetPrintf(("netresource: (%c%c%c%c-%08X) :: START\n",
                    NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                #endif
            }
            else
            {
                // see if we've gotten the size yet
                iStatus = ProtoHttpStatus(pState->pHttp, 'body', NULL, 0);
                if(iStatus >= 0)
                {
                    // notify the game that we have the size now save off the size and set the new state
                    pXfer->iLength = iStatus;
                    pXfer->iStatus = NETRSRC_STAT_SIZE;
                    #if NETRSRC_VERBOSE
                    NetPrintf(("netresource: (%c%c%c%c-%08X) :: START\n",
                        NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                    #endif
                }
                else
                {
                    // check to see if we've generated an error
                    _NetResourceHttpErrorCheck(pState, pXfer);
                }
            }
            break;
        }
        case NETRSRC_STAT_SIZE:     // size of object known
        {
            if(pXfer->uCacheStatus & NETRSRC_CACHESTATUS_LOADING)
            {
                // call the callback for the SIZE state
                _NetResourceCallUserCallback(pState, pXfer, NULL, 0);

                #if NETRSRC_VERBOSE
                NetPrintf(("netresource: SIZE  resource (%c%c%c%c-%08X) will be loaded from cache.\n",
                    NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                #endif

                // if we're using a cache entry, there's not really anything to do here
                pXfer->iStatus = NETRSRC_STAT_DATA;
            }
            else
            {
                // check to see if we've generated an error
                if(_NetResourceHttpErrorCheck(pState, pXfer) >= 0)
                {
                    // call the callback for the SIZE state
                    _NetResourceCallUserCallback(pState, pXfer, NULL, 0);

                    // setup caching if enabled for this transaction
                    _NetResourceCacheAssign(pState, pXfer);

                    #if NETRSRC_VERBOSE
                    // some debugging print stuff
                    if(pXfer->uCacheStatus & NETRSRC_CACHESTATUS_SAVING)
                    {
                        NetPrintf(("netresource: SIZE  resource (%c%c%c%c-%08X) will be saved to cache.\n",
                            NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                    }
                    else
                    {
                        NetPrintf(("netresource: SIZE  resource (%c%c%c%c-%08X) will not be cache saved/loaded.\n",
                            NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                    }
                    #endif

                    // now begin receiving data next time around
                    pXfer->iStatus = NETRSRC_STAT_DATA;
                }
            }
            break;
        }
        case NETRSRC_STAT_DATA:     // append data to the buffer
        {
            if(pXfer->uCacheStatus & NETRSRC_CACHESTATUS_LOADING)
            {
                // send the entire thing at once
                _NetResourceCallUserCallback(pState, pXfer, pXfer->pCache, pXfer->iCacheLen);
                // done - transition the state
                #if NETRSRC_VERBOSE
                NetPrintf(("netresource: (%c%c%c%c-%08X) :: DATA\n",
                    NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                #endif
                pXfer->iStatus = NETRSRC_STAT_DONE;
            }
            else
            {
                // get some data if there is any
                while ((iStatus = ProtoHttpRecv(pState->pHttp, pState->pRecvBuffer, 1, sizeof(pState->pRecvBuffer))) > 0)
                {
                    // pass the data we just got back, just in case they want to copy it out
                    if(pXfer->uCacheStatus & NETRSRC_CACHESTATUS_SAVING)
                    {
                        // here's the data buffer pointer to use later on
                        char *pDataBuf = (char *)(pXfer->pCache) + pXfer->iCacheLen;
                        // save in the cache (if enabled)
                        _NetResourceCacheSave(pState, pXfer, pState->pRecvBuffer, iStatus);
                        // call the user callback
                        _NetResourceCallUserCallback(pState, pXfer, pDataBuf, iStatus);
                    }
                    else
                    {
                        _NetResourceCallUserCallback(pState, pXfer, pState->pRecvBuffer, iStatus);
                    }
                    // increment the amount of data we've received
                    pState->iRecvLength += iStatus;
                }
                // check to see if we've generated an error
                if(iStatus < 0)
                {
                    if(_NetResourceHttpErrorCheck(pState, pXfer) > 0)
                    {
                        // we're done downloading - set the state to DONE
                        #if NETRSRC_VERBOSE
                        NetPrintf(("netresource: (%c%c%c%c-%08X) :: DATA\n",
                            NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                        #endif
                        pXfer->iStatus = NETRSRC_STAT_DONE;
                    }
                }
                // otherwise see if we're done
                else if(ProtoHttpStatus(pState->pHttp, 'done', NULL, 0))
                {
                    // successfully got data with no errors -- see if we should increment the state
                    if (pState->iRecvLength == pXfer->iLength)
                    {
                        // we're done copying - set the state to DONE
                        #if NETRSRC_VERBOSE
                        NetPrintf(("netresource: (%c%c%c%c-%08X) :: DATA\n",
                            NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
                        #endif
                        pXfer->iStatus = NETRSRC_STAT_DONE;
                    }
                }
            }
            // done for now
            break;
        }
        case NETRSRC_STAT_DONE:     // transfer is complete
        {
            // call the callback with this state
            _NetResourceCallUserCallback(pState,pXfer,pXfer->pCache,pXfer->iCacheLen);
            // set the state to finish
            pXfer->iStatus = NETRSRC_STAT_FINISH;
            #if NETRSRC_VERBOSE
            NetPrintf(("netresource: (%c%c%c%c-%08X) :: DONE\n",
                NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
            #endif
            break;
        }
        case NETRSRC_STAT_ERROR:    // transfer failed
        {
            #if NETRSRC_VERBOSE
            NetPrintf(("netresource: (%c%c%c%c-%08X) :: ERROR\n",
                NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
            #endif
            // call the callback with this state
            _NetResourceCallUserCallback(pState,pXfer,NULL,0);
            pXfer->iStatus = NETRSRC_STAT_FINISH;
            break;
        }
        case NETRSRC_STAT_CANCEL:   // transfer was canceled
        {
            #if NETRSRC_VERBOSE
            NetPrintf(("netresource: (%c%c%c%c-%08X) :: CANCEL\n",
                NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
            #endif
            // call the callback with this state
            _NetResourceCallUserCallback(pState,pXfer,NULL,0);
            // abort the protohttp transfer
            if (pState->pCurrentXferCB == pXfer)
            {
                ProtoHttpAbort(pState->pHttp);
            }
            // if we were caching to it, wipe it out
            if(pXfer->uCacheStatus & NETRSRC_CACHESTATUS_SAVING)
            {
                // delete the xfer and control block
                _NetResourceCacheDeleteXfer(pState, pXfer->uType, pXfer->uIdent);
            }
            else
            {
                // delete the control block
                _NetResourceDeleteControlBlock(pState, pXfer->iSeqNum);
            }
            if (pState->pCurrentXferCB == pXfer)
            {
                 pState->pCurrentXferCB = NULL;
            }
            break;
        }
        case NETRSRC_STAT_TIMEOUT:  // transfer timed out
        {
            #if NETRSRC_VERBOSE
            NetPrintf(("netresource: (%c%c%c%c-%08X) :: TIMEOUT\n",
                NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
            #endif
            // call the callback with this state
            _NetResourceCallUserCallback(pState,pXfer,NULL,0);
            pXfer->iStatus = NETRSRC_STAT_FINISH;
            break;
        }
        case NETRSRC_STAT_MISSING:  // resource is not on the server
        {
            #if NETRSRC_VERBOSE
            NetPrintf(("netresource: (%c%c%c%c-%08X) :: MISSING\n",
                NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
            #endif
            // call the callback with this state
            _NetResourceCallUserCallback(pState,pXfer,NULL,0);
            pXfer->iStatus = NETRSRC_STAT_FINISH;
            break;
        }
        case NETRSRC_STAT_FINISH:   // finished - release resources
        {
            #if NETRSRC_VERBOSE
            NetPrintf(("netresource: (%c%c%c%c-%08X) :: FINISHED\n",
                NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent)));
            #endif
            // call the callback with this state
            _NetResourceCallUserCallback(pState,pXfer,NULL,0);
            // now be done with this transfer block
            _NetResourceDeleteControlBlock(pState, pXfer->iSeqNum);
            // and remember not to do anything with the pXfer block anymore
            pState->pCurrentXferCB = NULL;
            break;
        }
        default:                    // nothing to do - default case not handled
        {
            NetPrintf(("netresource: (%c%c%c%c-%08X) unknown state [%u] - not calling callback, going to FINISH state\n",
                NETRSRC_PRINTFTYPEIDENT(pXfer->uType, pXfer->uIdent), pXfer->iStatus));
            pXfer->iStatus = NETRSRC_STAT_FINISH;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _NetResourceUpdate

    \Description
        Let network resource manager perform background processing

    \Input *pData   - user-defined callback value (NetResourceT *ptr)
    \Input uTick    - incoming tick value

    \Notes
        Call this function at least 4 times per second for good performance

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
static void _NetResourceUpdate(void *pData, uint32_t uTick)
{
    NetResourceT *pState = (NetResourceT *)pData;
    NetResourceControlT *pXfer;

    // first, give life to http module
    ProtoHttpUpdate(pState->pHttp);

    // see if we've got anything to process
    if(pState->pXferCBList == NULL)
    {
        return;
    }

    // the list isn't empty, so assign a xfer structure
    if(pState->pCurrentXferCB == NULL)
    {
        _NetResourceXferListPrint(pState);

        // if we have a cache, try to load the resource from it first
        if(pState->pCache)
        {
            #if NETRSRC_VERBOSE
            NetPrintf(("netresource: checking cache for a cache hit\n"));
            #endif
            // walk the list and look for anything already in the cache
            for (pXfer = pState->pXferCBList; pXfer != NULL; pXfer = pXfer->pNext)
            {
                // see if we have a cache hit
                if (_NetResourceCacheCheck(pState, pXfer, TRUE) != NULL)
                {
                    pState->pCurrentXferCB = pXfer;
                    // we got something, so dump out of the for loop
                    break;
                }
            }
        }

        // see if we still need to get an xfer
        if(pState->pCurrentXferCB == NULL)
        {
            pState->pCurrentXferCB = pState->pXferCBList;
        }
    }

    // set the convenience pointer
    pXfer = pState->pCurrentXferCB;

    // if we're getting from the cache, do all of the states in one call
    if(pXfer->uCacheStatus & NETRSRC_CACHESTATUS_LOADING)
    {
        // call the update function until done
        while(pState->pCurrentXferCB)
        {
            _NetResourceUpdate2(pState, pXfer);
        }
    }
    else
    {
        _NetResourceUpdate2(pState, pXfer);
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetResourceCreate

    \Description
        Create network resource manager instance

    \Input *pHome - address of home server (normally downloaded from lobby server)

    \Output NetResourceT - pass this return value as the first parameter
                           to all other functions

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
NetResourceT *NetResourceCreate(const char *pHome)
{
    NetResourceT *pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    #if NETRSRC_VERBOSE
    NetPrintf(("netresource: create called with home of [%s]\n", pHome));
    #endif

    // create and initialize the net resource structure
    if ((pState = (NetResourceT *)DirtyMemAlloc(sizeof(*pState), NETRSRC_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netresource: unable to allocate module state\n"));
        return(NULL);
    }
    memset(pState,0,sizeof(NetResourceT));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;

    // allocate protohttp module
    pState->pHttp = ProtoHttpCreate(NETRSRC_HTTPSPOOLSIZE);
    if (pState->pHttp == NULL)
    {
        NetPrintf(("netresource: unable to create protohttp module\n"));
        DirtyMemFree(pState, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // copy the home string in
    ds_strnzcpy(pState->strHomeURL, pHome, sizeof(pState->strHomeURL));

    // install idle callback
    NetConnIdleAdd(_NetResourceUpdate, pState);
    return(pState);
}

/*F********************************************************************************/
/*!
    \Function NetResourceDestroy

    \Description
        Destroy the network resource manager instance.

    \Input *pState  - module state

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
void NetResourceDestroy(NetResourceT *pState)
{
    // remove idle callback
    NetConnIdleDel(_NetResourceUpdate, pState);

    // destroy ProtoHttp module
    ProtoHttpDestroy(pState->pHttp);

    // delete all pending transfers
    _NetResourceXferListClear(pState);

    // delete the cache and used list
    _NetResourceCacheClear(pState);

    // finally, dump the module state itself
    DirtyMemFree(pState, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function NetResourceCache

    \Description
        Sets the external cache buffer, which will be used for caching resources
        until a different buffer is selected.  Pass NULL to disable caching.

    \Input *pState    - module state
    \Input *pCacheBuf - pointer to the user-supplied (and alloc'ed) memory block
    \Input iCacheLen  - length of the user-supplied memory block

    \Output --- TBD ---

    \Notes
        This module does not allocate or free any memory.  These operations are
        left up to the caller.

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
int32_t NetResourceCache(NetResourceT *pState, void *pCacheBuf, const int32_t iCacheLen)
{
    NetResourceCacheUsedListT *pUsed;

    // wipe out the previous used list
    pUsed = pState->pUsedList;
    while((pUsed = pState->pUsedList) != NULL)
    {
        pUsed = pState->pUsedList->pNext;
        DirtyMemFree(pState->pUsedList, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        pState->pUsedList = pUsed;
    }
    // dump the old pointers we used
    pState->pUsedList = NULL;
    pState->pCache = NULL;
    pState->iCacheLen = 0;

    // only setup a cache if we got some cache to use
    if((pCacheBuf) && (iCacheLen > 0))
    {
        // assign the cache buffer
        pState->pCache = pCacheBuf;
        pState->iCacheLen = iCacheLen;
        pState->iFreeCacheLen = iCacheLen;
    }

    // no errors
    return(NETRSRC_ERROR_NONE);
}

/*F********************************************************************************/
/*!
    \Function NetResourceCacheCheck

    \Description
        Check to see if the cache contains the given resource.

    \Input *pState          - module state
    \Input *pResource       - resource name (NULL if using type and ident)
    \Input uType            - type of resource to check if pResource is NULL; else unused
    \Input uIdent           - identity of resource to check if pResource is NULL; else unused

    \Output
        int32_t             - 0=cache miss, 1=cache hit

    \Version 05/12/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t NetResourceCacheCheck(NetResourceT *pState, const char *pResource, uint32_t uType, uint32_t uIdent)
{
    NetResourceControlT ResControl;

    // if we've got a resource name, translate it to type/ident
    if (pResource != NULL)
    {
        int32_t iResult = _NetResourceGetFromString(pResource, &uType, &uIdent);
        if (iResult < 0)
        {
            return(iResult);
        }
    }

    // set up control structure used to check for resource in cache
    memset(&ResControl, 0, sizeof(ResControl));
    ResControl.uType = uType;
    ResControl.uIdent = uIdent;

    // check cache for resource and do not update priority
    return(_NetResourceCacheCheck(pState, &ResControl, FALSE) != NULL);
}

/*F********************************************************************************/
/*!
    \Function NetResourceFetch

    \Description
        Start a network transfer from a resource type and id value.

    \Input *pState          - module state
    \Input *pXfer           - transfer descriptor
    \Input uType            - type of resource to download
    \Input uIdent           - identity of resource to download
    \Input *pCallback       - function to call with update progress
    \Input iTimeout         - number of milliseconds allowed for transaction
    \Input iPriority        - NETRSRC_PRIO_* priority of item, if cache enabled
                                (send 0 if don't care)

    \Output int32_t - Sequence number for the transaction, changes for each update
                    Returns 0 if the request was for a cached entry and there's
                    no sequence number to remember.

    \Notes
        The type/ident fields are internally translated into URLs via a mapping
        from the server and the file downloaded from a web server. The
        user supplied callback function handles buffer management and appending
        the data into the buffer.  The timeout field lets the application specify
        a maximum time for the transfer.

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
int32_t NetResourceFetch(NetResourceT *pState, NetResourceXferT *pXfer, const uint32_t uType, const uint32_t uIdent,
    NetResourceCallbackT *pCallback, const int32_t iTimeout, const int32_t iPriority)
{
    NetResourceControlT *pList, *pNewEntry;

    // create a new control structure
    pNewEntry = (NetResourceControlT *)DirtyMemAlloc(sizeof(*pNewEntry), NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(pNewEntry, 0, sizeof(NetResourceControlT));

    // set the mandatory parameters
    // in the XferT structures from passed-in params
    pNewEntry->uType = uType;
    pNewEntry->uIdent = uIdent;
    pNewEntry->pCallback = pCallback;
    pNewEntry->iTimeout = iTimeout;
    pNewEntry->iPriority = iPriority;
    // set the status of the transfer to a default value
    pNewEntry->iStatus = NETRSRC_STAT_IDLE;
    // assign the user xfer block
    pNewEntry->pUserXfer = pXfer;
    // and give it a sequence number
    pNewEntry->iSeqNum = pState->iTotalSeqNum++;

    // see if the entry is in the cache already, and if so
    // don't bother adding it to the list - just run the
    // update function to completion for this resource
    if(_NetResourceCacheCheck(pState, pNewEntry, TRUE) != NULL)
    {
        // save off the current transfer block since we're pre-empting
        NetResourceControlT *pTempCurrent = pState->pCurrentXferCB;

        // assign the new pCurrentXferCB
        pState->pCurrentXferCB = pNewEntry;

        // call the update function until done
        while(pState->pCurrentXferCB)
        {
            _NetResourceUpdate2(pState, pNewEntry);
        }

        // and re-write the current transfer block
        pState->pCurrentXferCB = pTempCurrent;

        // dump the control block
        DirtyMemFree(pNewEntry, NETRSRC_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

        // return, signaling that there is no sequence number to remember
        return(0);
    }
    else
    {
        // add the transfer structure to the list
        // see if the list is empty
        if(pState->pXferCBList == NULL)
        {
            // list is empty - add the structure to the list
            pState->pXferCBList = pNewEntry;
            pNewEntry->pNext = NULL;
            pNewEntry->pPrev = NULL;
        }
        else
        {
            // find the end of the list
            pList = pState->pXferCBList;
            while(pList->pNext != NULL)
            {
                pList = pList->pNext;
            }
            // add onto the end of the list
            pList->pNext = pNewEntry;
            pNewEntry->pPrev = pList;
            pNewEntry->pNext = NULL;
        }

        // done - return sequence number
        return(pNewEntry->iSeqNum);
    }
}

/*F********************************************************************************/
/*!
    \Function NetResourceFetchString

    \Description
        Start a network transfer from a resource string.

    \Input *pState          - module state
    \Input *pXfer           - transfer descriptor
    \Input *pResource       - string for resource to download
    \Input *pCallback       - function to call with update progress
    \Input iTimeout         - number of milliseconds allowed for transaction
    \Input iPriority        - NETRSRC_PRIO_* priority of item, if cache enabled
                                (send 0 if don't care)

    \Output Result of NetReourceFetch - see return codes for that function

    \Notes
        This is an adapter for the NetResourceFetch which allows game code
        to request a resource in string format as opposed to type and ident.
        The string sent to this function should be in the format:
          "<4-character type>-<numeric ID value>"
        An optional prefix of "netres:" may be added if required, but will be
        ignored by NetResourceFetch.
        Example:  "netres:imag-001"

    \Version 02/22/2005 (jfrank)
*/
/********************************************************************************F*/
int32_t NetResourceFetchString(NetResourceT *pState, NetResourceXferT *pXfer, const char *pResource,
    NetResourceCallbackT *pCallback, const int32_t iTimeout,  const int32_t iPriority)
{
    uint32_t uType = 0;
    uint32_t uIdent = 0;
    int32_t iResult = _NetResourceGetFromString(pResource, &uType, &uIdent);

    if (iResult < 0)
    {
        return(iResult);
    }

    // now pass to normal type/ident function
    return(NetResourceFetch(pState, pXfer, uType, uIdent, pCallback, iTimeout, iPriority));
}

/*F********************************************************************************/
/*!
    \Function NetResourceCancel

    \Description
        Cancel an in-progress network transfer

    \Input *pState          - module state
    \Input iSeqNum          - Sequence number of the transaction to cancel

    \Notes
        If pXfer is NULL, cancel ALL pending transfers.
        Otherwise, run through the list of pending transfers
        and cancel only the requested tranfer.

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
void NetResourceCancel(NetResourceT *pState, int32_t iSeqNum)
{
    NetResourceControlT *pList;
    NetResourceControlT *pListNext;

    if (iSeqNum > 0)
    {
        // find the item in question
        for (pList = pState->pXferCBList; pList != NULL; pList = pList->pNext)
        {
            // see if we've found it
            if (pList->iSeqNum == iSeqNum)
            {
                break;
            }
        }
        // kill it if non-null
        if (pList != NULL)
        {
            // force the state to cancel - it should dump itself inside the state machine
            pList->iStatus = NETRSRC_STAT_CANCEL;
            pList->pCallback = NULL;
            NetPrintf(("netresource: (%c%c%c%c-%08X) Cancelling Transaction\n",
                NETRSRC_PRINTFTYPEIDENT(pList->uType, pList->uIdent)));
            // Pump NetResource to force cancellation code to be executed
            _NetResourceUpdate2(pState, pList); // Once for NETRSRC_STAT_CANCEL state processing
        }
    }
    else
    {
        NetPrintf(("netresource: canceling all pending transactions:\n"));
        pList = pState->pXferCBList;
        while (pList != NULL)
        {
            NetPrintf(("netresource: (%c%c%c%c-%08X) Cancelling Transaction\n",
                NETRSRC_PRINTFTYPEIDENT(pList->uType, pList->uIdent)));
            pList->iStatus = NETRSRC_STAT_CANCEL;
            pList->pCallback = NULL;

            pListNext = pList->pNext;
            // Pump NetResource to force cancellation code to be executed
            _NetResourceUpdate2(pState, pList); // Once for NETRSRC_STAT_CANCEL state processing
            pList = pListNext;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function NetResourceCachePrint

    \Description
        Print the used list to the debug console

    \Input *pState  - module state

    \Version 12/15/2004 (jfrank)
*/
/********************************************************************************F*/
#if DIRTYCODE_LOGGING
void NetResourceCachePrint(NetResourceT *pState)
{
    NetResourceCacheUsedListT *pUsed = pState->pUsedList;
    NetPrintf(("netresource: Used List:\n"));
    // see if there are any entries
    if(pUsed == NULL)
    {
        NetPrintf(("netresource: -- Used list empty.\n"));
    }
    // loop through the entries
    else
    {
        while(pUsed)
        {
            char strPriority[5];
            if(pUsed->iPriority == NETRSRC_PRIO_DEFAULT)
            {
                ds_snzprintf(strPriority, sizeof(strPriority), "DFLT");
            }
            else if(pUsed->iPriority == NETRSRC_PRIO_DONTCACHE)
            {
                ds_snzprintf(strPriority, sizeof(strPriority), "DONT");
            }
            else if(pUsed->iPriority == NETRSRC_PRIO_KEEPIFPOSSIBLE)
            {
                ds_snzprintf(strPriority, sizeof(strPriority), "KEEP");
            }
            else
            {
                ds_snzprintf(strPriority, sizeof(strPriority), "????");
            }

            NetPrintf(("netresource: -- Cache Entry: (%c%c%c%c-%08X) Priority[%s] NumAccesses[%5u] LastTick[%10u] Offset[%8u] Length[%8d]\n",
                NETRSRC_PRINTFTYPEIDENT(pUsed->uType,pUsed->uIdent), strPriority,
                pUsed->uNumAccesses, pUsed->uLastTick, pUsed->iOffset, pUsed->iLength));
            // increment to the next item
            pUsed = pUsed->pNext;
        }
    }
}
#endif
