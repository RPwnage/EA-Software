/*H********************************************************************************/
/*!
    \File dirtylspxenon.c

    \Description
        Xenon XLSP client implementation.

    \Copyright
        Copyright (c) 2006-2010 Electronic Arts Inc.

    \Version 03/13/2006 (jbrookes) First Version
    \Version 11/17/2009 (jbrookes) Added dynamic query size, user callback, XNet connection.
    \Version 04/21/2010 (jbrookes) Added connection expiration timeout.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <xtl.h>
#include <winsockx.h>

#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/dirtysock/xenon/dirtylspxenon.h"

/*** Defines **********************************************************************/

//! minimum number of records that will be allocated for query response
#define DIRTYLSP_MINQUERYCOUNT      (20)

//! number of cache records we keep internally
#define DIRTYLSP_CACHESIZE          (16)

//! amount of time that will elapse before a connection will be closed, in milliseconds
#define DIRTYLSP_EXPIRETIME_DEFAULT (5*60*1000)

/*** Type Definitions *************************************************************/

//! a single cache entry, representing an lsp name/address pair
typedef struct DirtyLSPCacheEntryT
{
    char strName[DIRTYLSP_USERDATA_MAXSIZE];    //!< lsp name
    uint32_t uAddress;                          //!< secure address
    uint32_t uConnTimer;                        //!< connection expiration timer
} DirtyLSPCacheEntryT;

//! dirtylsp address entry cache
typedef struct DirtyLSPCacheT
{
    int32_t iRefCount;              //!< cache ref count
    uint32_t uConnTimeout;          //!< cache entry expiration time

    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    DirtyLSPCacheCbT *pCacheCb;     //!< cache callback
    void *pUserRef;                 //!< user ref for cache callback

    NetCritT CacheCrit;             //!< critical section for accessing cache entries
    DirtyLSPCacheEntryT aCache[DIRTYLSP_CACHESIZE];  //!< cache entry array
} DirtyLSPCacheT;

// module state
struct DirtyLSPRefT
{
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    DirtyLSPNotifyCbT *pNotifyCb;   //!< optional notification callback
    void *pUserRef;                 //!< user ref for callback (may be null)

    uint32_t uMaxQueryCount;        //!< maximum number of XLSPs that will be returned in a query
    DirtyLSPQueryRecT *pQueryList;  //!< dynamic list of lsp query records
    NetCritT QueryCrit;             //!< mutex for accesses to query list
};

/*** Variables ********************************************************************/

//! dirtylsp cache -- this is a singleton
static DirtyLSPCacheT *_DirtyLSP_pCache = NULL;

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _DirtyLSPConnectionOpen

    \Description
        Open a secure connection.

    \Input *pQueryRec   - pointer to query record with connection info

    \Version 11/30/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPConnectionOpen(DirtyLSPQueryRecT *pQueryRec)
{
    DWORD dwResult;
    if ((dwResult = XNetConnect(pQueryRec->InAddr)) == 0)
    {
        pQueryRec->eQueryState = DIRTYLSP_QUERYSTATE_CONN;
    }
    else
    {
        NetPrintf(("dirtylspxenon: [%08x] XNetConnect() failed (err=%s)\n", (uintptr_t)pQueryRec, DirtyErrGetName(dwResult)));
        pQueryRec->eQueryState = DIRTYLSP_QUERYSTATE_FAIL;
        pQueryRec->uQueryError = (uint32_t)dwResult;
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPConnectionClose

    \Description
        Close a secure server connection.

    \Input uAddress     - secure address to close

    \Version 11/25/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPConnectionClose(uint32_t uAddress)
{
    IN_ADDR InAddr;
    DWORD dwResult;

    InAddr.s_addr = SocketHtonl(uAddress);
    if ((dwResult = XNetUnregisterInAddr(InAddr)) == 0)
    {
        NetPrintf(("dirtylspxenon: XNetUnregisterInAddr(%a) succeeded\n", uAddress));
    }
    else
    {
        NetPrintf(("dirtylspxenon: XNetUnregisterInAddr(%a) failed (err=%s)\n", uAddress, DirtyErrGetName(dwResult)));
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheEntryResetTimer

    \Description
        Reset specified cache entry timer.

    \Input *pDirtyCache         - pointer to global cache
    \Input *pCacheEntry         - pointer to cache entry to reset

    \Version 04/15/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPCacheEntryResetTimer(DirtyLSPCacheT *pDirtyCache, DirtyLSPCacheEntryT *pCacheEntry)
{
    pCacheEntry->uConnTimer = NetTick() + pDirtyCache->uConnTimeout;
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheCreate

    \Description
        Create global cache, or refcount it if it already exists

    \Input *pDirtyCache         - pointer to global cache, or NULL if it does not yet exist
    \Input iMemGroup            - mem group id
    \Input *pMemGroupUserData   - user-provided data pointer

    \Output
        DirtyLSPCacheT *        - global cache ref

    \Version 02/23/2010 (jbrookes)
*/
/********************************************************************************F*/
static DirtyLSPCacheT *_DirtyLSPCacheCreate(DirtyLSPCacheT *pDirtyCache, int32_t iMemGroup, void *pMemGroupUserData)
{
    if (pDirtyCache == NULL)
    {
        NetPrintf(("dirtylspxenon: allocating cache array\n"));
        if ((pDirtyCache = DirtyMemAlloc(sizeof(*pDirtyCache), DIRTYLSP_MEMID, iMemGroup, pMemGroupUserData)) != NULL)
        {
            memset(pDirtyCache, 0, sizeof(*pDirtyCache));
            pDirtyCache->iRefCount = 1;
            pDirtyCache->iMemGroup = iMemGroup;
            pDirtyCache->pMemGroupUserData = pMemGroupUserData;
            pDirtyCache->uConnTimeout = DIRTYLSP_EXPIRETIME_DEFAULT;
            NetCritInit(&pDirtyCache->CacheCrit, "dirtylsp-cache");
        }
    }
    else
    {
        pDirtyCache->iRefCount += 1;
        NetPrintf(("dirtylspxenon: refcounted cache array (%d references)\n", pDirtyCache->iRefCount));
    }

    return(pDirtyCache);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheCallback

    \Description
        Set cache notification callback and/or user data pointer

    \Input *pDirtyCache - pointer to global cache
    \Input *pCacheCb    - pointer to callback (NULL=don't set)
    \Input *pUserRef    - pointer to user data (NULL=don't set)

    \Version 04/08/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPCacheCallback(DirtyLSPCacheT *pDirtyCache, DirtyLSPCacheCbT *pCacheCb, void *pUserRef)
{
    // acquire cache critical section
    NetCritEnter(&pDirtyCache->CacheCrit);

    if (pCacheCb != NULL)
    {
        NetPrintf(("dirtylspxenon: setting cache notification callback = %p\n", pCacheCb));
        #if DIRTYCODE_LOGGING
        if ((pDirtyCache->pCacheCb != NULL) && (pDirtyCache->pCacheCb != pCacheCb))
        {
            NetPrintf(("dirtylspxenon: warning; overwriting previous cache notification callback %p\n", pDirtyCache->pCacheCb));
        }
        #endif
        pDirtyCache->pCacheCb = pCacheCb;
    }
    if (pUserRef != NULL)
    {
        NetPrintf(("dirtylspxenon: setting cache notification user ref = %p\n", pUserRef));
        #if DIRTYCODE_LOGGING
        if ((pDirtyCache->pUserRef != NULL) && (pDirtyCache->pUserRef != pUserRef))
        {
            NetPrintf(("dirtylspxenon: warning; overwriting previous cache notification user ref %p\n", pDirtyCache->pUserRef));
        }
        #endif
        pDirtyCache->pUserRef = pUserRef;
    }

    // release cache critical section
    NetCritLeave(&pDirtyCache->CacheCrit);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheDel

    \Description
        Delete the cache entry.

    \Input *pDirtyCache - pointer to global cache
    \Input *pCacheEntry - pointer to entry to delete

    \Version 04/08/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPCacheDel(DirtyLSPCacheT *pDirtyCache, DirtyLSPCacheEntryT *pCacheEntry)
{
    NetPrintf(("dirtylspxenon: closing connection %s/%a (tick=0x%08x)\n", pCacheEntry->strName, pCacheEntry->uAddress, NetTick()));
    _DirtyLSPConnectionClose(pCacheEntry->uAddress);

    // call system cache callback if installed
    if (pDirtyCache->pCacheCb != NULL)
    {
        pDirtyCache->pCacheCb(pCacheEntry->uAddress, DIRTYLSP_CACHEEVENT_EXPIRED, pDirtyCache->pUserRef);
    }

    pCacheEntry->strName[0] = '\0';
    pCacheEntry->uAddress = 0;
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheReset

    \Description
        Resets the cache.

    \Input *pDirtyCache - pointer to global cache

    \Version 04/08/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPCacheReset(DirtyLSPCacheT *pDirtyCache)
{
    DirtyLSPCacheEntryT *pCacheEntry;
    int32_t iCacheEntry;

    NetPrintf(("dirtylspxenon: clearing global cache\n"));

    // acquire cache critical section
    NetCritEnter(&pDirtyCache->CacheCrit);

    // iterate through cache looking for entries to expire
    for (iCacheEntry = 0, pCacheEntry = pDirtyCache->aCache; iCacheEntry < DIRTYLSP_CACHESIZE; iCacheEntry += 1, pCacheEntry += 1)
    {
        // check for entries to expire
        if (pCacheEntry->uAddress != 0)
        {
            // delete the cache entry
            _DirtyLSPCacheDel(pDirtyCache, pCacheEntry);
        }
    }

    // reset the cache
    memset(pDirtyCache->aCache, 0, sizeof(pDirtyCache->aCache));

    // release cache critical section
    NetCritLeave(&pDirtyCache->CacheCrit);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheDestroy

    \Description
        Decrement ref count for global cache, and destroy it if the refcount reaches zero.

    \Input *pDirtyCache         - pointer to global cache

    \Output
        DirtyLSPCacheT *        - global cache ref, or NULL if the cache was destroyed

    \Version 02/23/2010 (jbrookes)
*/
/********************************************************************************F*/
static DirtyLSPCacheT *_DirtyLSPCacheDestroy(DirtyLSPCacheT *pDirtyCache)
{
    if (pDirtyCache->iRefCount == 1)
    {
        NetPrintf(("dirtylspxenon: destroying cache array\n"));
        _DirtyLSPCacheReset(pDirtyCache);
        NetCritKill(&pDirtyCache->CacheCrit);
        DirtyMemFree(pDirtyCache, DIRTYLSP_MEMID, pDirtyCache->iMemGroup, pDirtyCache->pMemGroupUserData);
        pDirtyCache = NULL;
    }
    else
    {
        pDirtyCache->iRefCount -= 1;
        NetPrintf(("dirtylspxenon: decremented cache array refcount (%d references)\n", pDirtyCache->iRefCount));
    }
    return(pDirtyCache);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheGet

    \Description
        Get cache entry for given name or address.

    \Input *pDirtyCache         - pointer to global cache
    \Input *pStrName            - LSP name to check for, or NULL to check by address
    \Input uAddress             - address to check for, if pStrName=NULL

    \Output
        DirtyLSPCacheEntryT *   - matching cache entry or NULL if not found

    \Notes
        It is expected that the caller of this function uses the cache critical
        section appropriately to guard access to the cache.

    \Version 11/25/2009 (jbrookes)
*/
/********************************************************************************F*/
static DirtyLSPCacheEntryT *_DirtyLSPCacheGet(DirtyLSPCacheT *pDirtyCache, const char *pStrName, uint32_t uAddress)
{
    DirtyLSPCacheEntryT *pCacheEntry, *pCacheRef;
    int32_t iCacheEntry;

    // ignore get request for null address
    if ((pStrName == NULL) && (uAddress == 0))
    {
        return(NULL);
    }

    // scan cache for matching entry
    for (iCacheEntry = 0, pCacheEntry = NULL; (iCacheEntry < DIRTYLSP_CACHESIZE) && (pCacheEntry == NULL); iCacheEntry += 1)
    {
        // ref cache entry
        pCacheRef = &pDirtyCache->aCache[iCacheEntry];

        // if we've been given a name, do a name-based check
        if (pStrName != NULL)
        {
            if (!strcmp(pCacheRef->strName, pStrName))
            {
                pCacheEntry = pCacheRef;
            }
        }
        // otherwise, assume we've been given an address and check that
        else if (pCacheRef->uAddress == uAddress)
        {
            pCacheEntry = pCacheRef;
        }
    }

    // return result to caller
    return(pCacheEntry);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheGetAddr

    \Description
        Get cached address for given name.

    \Input *pDirtyCache         - pointer to global cache
    \Input *pStrName            - LSP name to check for, or NULL to check by address

    \Output
        uint32_t                - address for cache entry, or zero if not in the cache

    \Version 02/23/2010 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _DirtyLSPCacheGetAddr(DirtyLSPCacheT *pDirtyCache, const char *pStrName)
{
    DirtyLSPCacheEntryT *pCacheEntry;
    uint32_t uAddress;

    // acquire cache critical section
    NetCritEnter(&pDirtyCache->CacheCrit);

    // check cache for an entry with the given name
    if ((pCacheEntry = _DirtyLSPCacheGet(pDirtyCache, pStrName, 0)) != NULL)
    {
        NetPrintf(("dirtylspxenon: found cache entry %s/%a\n", pCacheEntry->strName, pCacheEntry->uAddress));
        uAddress = pCacheEntry->uAddress;
    }
    else
    {
        uAddress = 0;
    }

    // release cache critical section
    NetCritLeave(&pDirtyCache->CacheCrit);

    // return result to caller
    return(uAddress);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheAdd

    \Description
        Add an entry to the LSP cache, if it is not already in the cache.

    \Input *pDirtyCache - pointer to global cache
    \Input *pQueryRec   - pointer to query record

    \Version 11/25/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPCacheAdd(DirtyLSPCacheT *pDirtyCache, DirtyLSPQueryRecT *pQueryRec)
{
    DirtyLSPCacheEntryT *pCacheEntry;

    // acquire cache critical section
    NetCritEnter(&pDirtyCache->CacheCrit);

    // if record isn't already present add it to the cache
    if ((pCacheEntry = _DirtyLSPCacheGet(pDirtyCache, pQueryRec->strName, 0)) == NULL)
    {
        int32_t iCacheEntry;

        for (iCacheEntry = 0, pCacheEntry = pDirtyCache->aCache; iCacheEntry < DIRTYLSP_CACHESIZE; iCacheEntry += 1, pCacheEntry += 1)
        {
            #if DIRTYCODE_LOGGING
            if (pCacheEntry->uAddress == pQueryRec->uAddress)
            {
                NetPrintf(("dirtylspxenon: warning; found duplicate at index %d of uAddress=%a when adding query to cache\n", iCacheEntry, pCacheEntry->uAddress));
            }
            #endif

            // empty slot?
            if (pCacheEntry->uAddress == 0)
            {
                NetPrintf(("dirtylspxenon: adding %s/%a to cache slot %d\n", pQueryRec->strName, pQueryRec->uAddress, iCacheEntry));
                ds_strnzcpy(pCacheEntry->strName, pQueryRec->strName, sizeof(pCacheEntry->strName));
                pCacheEntry->uAddress = pQueryRec->uAddress;
                _DirtyLSPCacheEntryResetTimer(pDirtyCache, pCacheEntry);
                break;
            }
        }
        if (iCacheEntry == DIRTYLSP_CACHESIZE)
        {
            NetPrintf(("dirtylspxenon: could not add %s/%a to cache\n", pQueryRec->strName, pQueryRec->uAddress));
        }
    }
    else
    {
        _DirtyLSPCacheEntryResetTimer(pDirtyCache, pCacheEntry);
    }

    // release cache critical section
    NetCritLeave(&pDirtyCache->CacheCrit);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheUpdate

    \Description
        Update DirtyLSP cache.

    \Input *pDirtyCache - pointer to global cache

    \Version 11/25/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPCacheUpdate(DirtyLSPCacheT *pDirtyCache)
{
    DirtyLSPCacheEntryT *pCacheEntry;
    int32_t iCacheEntry, iTickDiff;

    // acquire cache critical section
    NetCritEnter(&pDirtyCache->CacheCrit);

    // iterate through cache looking for entries to expire
    for (iCacheEntry = 0, pCacheEntry = pDirtyCache->aCache; iCacheEntry < DIRTYLSP_CACHESIZE; iCacheEntry += 1, pCacheEntry += 1)
    {
        // check for entries to expire
        if ((pCacheEntry->uAddress != 0) && ((iTickDiff = NetTickDiff(NetTick(), pCacheEntry->uConnTimer)) > 0))
        {
            // delete the cache entry
            _DirtyLSPCacheDel(pDirtyCache, pCacheEntry);
        }
    }

    // release cache critical section
    NetCritLeave(&pDirtyCache->CacheCrit);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheTimeout

    \Description
        Set cache timeout.

    \Input *pDirtyCache - pointer to global cache
    \Input uConnTimeout - cache connection timeout, in milliseconds

    \Version 04/19/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPCacheTimeout(DirtyLSPCacheT *pDirtyCache, uint32_t uConnTimeout)
{
    int32_t iCacheEntry;

    NetPrintf(("dirtylspxenon: setting cache timeout to %dms\n", uConnTimeout));

    // acquire cache critical section
    NetCritEnter(&pDirtyCache->CacheCrit);

    // set timeout
    pDirtyCache->uConnTimeout = uConnTimeout;

    // update all cache entries with new timeout value
    for (iCacheEntry = 0; iCacheEntry < DIRTYLSP_CACHESIZE; iCacheEntry += 1)
    {
        // touch cache entry
        _DirtyLSPCacheEntryResetTimer(pDirtyCache, &pDirtyCache->aCache[iCacheEntry]);
    }

    // release cache critical section
    NetCritLeave(&pDirtyCache->CacheCrit);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPCacheTouch

    \Description
        'Touch' a single cache entry, resetting the expiration timeout.

    \Input *pDirtyCache - pointer to global cache
    \Input uAddress     - cache address to touch

    \Version 04/15/2010 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPCacheTouch(DirtyLSPCacheT *pDirtyCache, uint32_t uAddress)
{
    DirtyLSPCacheEntryT *pCacheEntry;

    // acquire cache critical section
    NetCritEnter(&pDirtyCache->CacheCrit);

    // get cache entry for given address
    if ((pCacheEntry = _DirtyLSPCacheGet(pDirtyCache, NULL, uAddress)) != NULL)
    {
        _DirtyLSPCacheEntryResetTimer(pDirtyCache, pCacheEntry);
    }

    // release cache critical section
    NetCritLeave(&pDirtyCache->CacheCrit);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPQueryCallback

    \Description
        Default query callback, used if caller does not set their own callback.

    \Input *pDirtyLSP   - pointer to module state
    \Input *pQueryRec   - pointer to query record
    \Input *pUserRef    - user data pointer

    \Version 11/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPQueryCallback(DirtyLSPRefT *pDirtyLSP, const DirtyLSPQueryRecT *pQueryRec, void *pUserRef)
{
    #if DIRTYCODE_LOGGING
    const char *_strStates[] =
    {
        "DIRTYLSP_QUERYSTATE_FAIL",
        "DIRTYLSP_QUERYSTATE_RSLV",
        "DIRTYLSP_QUERYSTATE_INIT",
        "DIRTYLSP_QUERYSTATE_CONN",
        "DIRTYLSP_QUERYSTATE_DONE"
    };

    NetPrintf(("dirtylspxenon: [%08x] query transition to state %s\n", (uintptr_t)pQueryRec, _strStates[(uint32_t)pQueryRec->eQueryState+1]));
    #endif
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPQueryAlloc

    \Description
        Allocate a query record

    \Input *pQueryStr       - query string (may be empty for global query)
    \Input iBufferSize      - size of query buffer to allocate for query
    \Input iMaxQueryResults - maximum number of query results that are being allocated

    \Output
        DirtyLSPQueryRecT * - query record or NULL on failure

    \Version 11/25/2009 (jbrookes)
*/
/********************************************************************************F*/
static DirtyLSPQueryRecT *_DirtyLSPQueryAlloc(const char *pQueryStr, int32_t iBufferSize, int32_t iMaxQueryResults)
{
    DirtyLSPQueryRecT *pQueryRec;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    int32_t iQuerySize;

    // query records use netconn's memgroup and memgroup pointer as the memory can exceed the lifespan of the DirtyLSP module
    iMemGroup = NetConnStatus('mgrp', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    iQuerySize = sizeof(*pQueryRec) + iBufferSize;
    if ((pQueryRec = DirtyMemAlloc(iQuerySize, DIRTYLSP_MEMID, iMemGroup, pMemGroupUserData)) != NULL)
    {
        NetPrintf(("dirtylspxenon: [%08x] allocated %d bytes for query record (%d max results)\n", (uintptr_t)pQueryRec, iQuerySize, iMaxQueryResults));
        memset(pQueryRec, 0, iQuerySize);
        ds_strnzcpy(pQueryRec->strName, pQueryStr, sizeof(pQueryRec->strName));
    }
    else
    {
        NetPrintf(("dirtylspxenon: failed to allocate %d byte query record\n", iQuerySize));
    }

    return(pQueryRec);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPQueryFree

    \Description
        Free an allocated query.

    \Input *pQueryRec   - pointer to query record to free

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPQueryFree(DirtyLSPQueryRecT *pQueryRec)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // query records use netconn's memgroup and memgroup pointer as the memory can exceed the lifespan of the DirtyLSP module
    iMemGroup = NetConnStatus('mgrp', 0, &pMemGroupUserData, sizeof(pMemGroupUserData));

    // close lsp enumerator, if allocated
    if (pQueryRec->hEnumerator != 0)
    {
        CloseHandle(pQueryRec->hEnumerator);
    }
    // release query memory
    DirtyMemFree(pQueryRec, DIRTYLSP_MEMID, iMemGroup, pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPQueryStart

    \Description
        Start an LSP query.

    \Input *pDirtyLSP       - pointer to module state
    \Input *pQueryStr       - query string (may be empty for global query)
    \Input uMaxResponses    - maximum number of responses to allow

    \Output
        DirtyLSPQueryRecT * - query record or NULL on failure

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
static DirtyLSPQueryRecT *_DirtyLSPQueryStart(DirtyLSPRefT *pDirtyLSP, const char *pQueryStr, uint32_t uMaxResponses)
{
    DirtyLSPQueryRecT *pQueryRec;
    DWORD dwBufferSize, dwResult;
    HANDLE hEnumerator;

    // create the enumerator
    NetPrintf(("dirtylspxenon: enumerating '%s'\n", pQueryStr));
    if ((dwResult = XTitleServerCreateEnumerator(pQueryStr, uMaxResponses, &dwBufferSize, &hEnumerator)) != ERROR_SUCCESS)
    {
        NetPrintf(("dirtylspxenon: could not create LSP enumerator (err=%s)\n", DirtyErrGetName(dwResult)));
        return(NULL);
    }

    // allocate query record
    if ((pQueryRec = _DirtyLSPQueryAlloc(pQueryStr, dwBufferSize, uMaxResponses)) == NULL)
    {
        CloseHandle(hEnumerator);
        return(NULL);
    }
    pQueryRec->hEnumerator = hEnumerator;

    // enumerate title servers
    if ((dwResult = XEnumerate(pQueryRec->hEnumerator, pQueryRec->aServerInfo, dwBufferSize, 0, &pQueryRec->XOverlapped)) != ERROR_IO_PENDING)
    {
        NetPrintf(("dirtylspxenon: could not enumerate LSP servers (err=%s)\n", DirtyErrGetName(dwResult)));
        _DirtyLSPQueryFree(pQueryRec);
        return(NULL);
    }

    // return query record to caller
    return(pQueryRec);
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPQueryUpdateRslv

    \Description
        Poll XLSP resolve query for completion

    \Input *pQueryRec   - pointer to query record

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPQueryUpdateRslv(DirtyLSPQueryRecT *pQueryRec)
{
    DWORD dwResult, dwNumResults;
    DirtyLSPQueryStateE eQueryState = DIRTYLSP_QUERYSTATE_FAIL;
    uint32_t uAddress = 0;

    // check for completion
    if (XHasOverlappedIoCompleted(&pQueryRec->XOverlapped) == FALSE)
    {
        return;
    }

    // check completion result
    if ((dwResult = XGetOverlappedResult(&pQueryRec->XOverlapped, &dwNumResults, TRUE)) == ERROR_SUCCESS)
    {
        DWORD dwServer=0;

        // display results
        NetPrintf(("dirtylspxenon: [%08x] got %d title service results\n", (uintptr_t)pQueryRec, dwNumResults));
        for (dwResult = 0; dwResult < dwNumResults; dwResult++)
        {
            NetPrintf(("dirtylspxenon: [%08x]   [%d] %a\n", (uintptr_t)pQueryRec, dwResult, pQueryRec->aServerInfo[dwResult].inaServer.s_addr));
        }

        // choose a TSADDR.  in case there are more than one, randomize for load balancing.
        if (dwNumResults > 1)
        {
            XNetRandom((BYTE *)&dwServer, sizeof(dwServer));
            dwServer %= dwNumResults;
            NetPrintf(("dirtylspxenon: [%08x] choosing server %d\n", (uintptr_t)pQueryRec, dwServer));
        }

        // convert to secure address
        if ((dwResult = XNetServerToInAddr(pQueryRec->aServerInfo[dwServer].inaServer, NetConnStatus('xlsp', 0, NULL, 0), &pQueryRec->InAddr)) == S_OK)
        {
            // save address
            uAddress = SocketNtohl(pQueryRec->InAddr.s_addr);
            eQueryState = DIRTYLSP_QUERYSTATE_INIT;
            NetPrintf(("dirtylspxenon: [%08x] resolved secure address %a for name %s\n", (uintptr_t)pQueryRec,
                uAddress, pQueryRec->strName));
        }
        else
        {
            NetPrintf(("dirtylspxenon: [%08x] XNetServerToInAddr failed (err=%s)\n", (uintptr_t)pQueryRec, DirtyErrGetName(dwResult)));
            pQueryRec->uQueryError = dwResult;
        }
    }
    else
    {
        pQueryRec->uQueryError = XGetOverlappedExtendedError(&pQueryRec->XOverlapped);
        if (pQueryRec->uQueryError == (unsigned)HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES))
        {
            NetPrintf(("dirtylspxenon: [%08x] there are no XLSPs in this environment advertising the UserData string '%s' for your TitleID;  please check your XLSP configuration and the TitleID embedded in your executable\n",
                (uintptr_t)pQueryRec, pQueryRec->strName));
        }
        else
        {
            NetPrintf(("dirtylspxenon: [%08x] XGetOverlappedResult failed (err=%s)\n", (uintptr_t)pQueryRec, DirtyErrGetName(pQueryRec->uQueryError)));
        }
    }

    // update state and address with completion result
    pQueryRec->eQueryState = eQueryState;
    pQueryRec->uAddress = uAddress;
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPQueryUpdateConn

    \Description
        Poll XLSP connection status for completion

    \Input *pQueryRec   - pointer to query record

    \Version 11/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPQueryUpdateConn(DirtyLSPQueryRecT *pQueryRec)
{
    // get connection status
    DWORD dwStatus = XNetGetConnectStatus(pQueryRec->InAddr);
    if (dwStatus == XNET_CONNECT_STATUS_CONNECTED)
    {
        NetPrintf(("dirtylspxenon: [%08x] connected to secure address %a (%s)\n", (uintptr_t)pQueryRec,
            pQueryRec->uAddress, pQueryRec->strName));
        pQueryRec->eQueryState = DIRTYLSP_QUERYSTATE_DONE;

        // add to cache
        _DirtyLSPCacheAdd(_DirtyLSP_pCache, pQueryRec);
    }
    else if (dwStatus == XNET_CONNECT_STATUS_LOST)
    {
        NetPrintf(("dirtylspxenon: [%08x] could not connect to secure address %a (%s)\n", (uintptr_t)pQueryRec,
            pQueryRec->uAddress, pQueryRec->strName));
        pQueryRec->eQueryState = DIRTYLSP_QUERYSTATE_FAIL;
    }
    else if (dwStatus != XNET_CONNECT_STATUS_PENDING)
    {
        NetPrintf(("dirtylspxenon: [%08x] unknown result %d from XNetGetConnectStatus()\n", (uintptr_t)pQueryRec, dwStatus));
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPQueryUpdate

    \Description
        Poll query for completion

    \Input *pDirtyLSP   - pointer to module state
    \Input *pQueryRec   - pointer to query record

    \Version 11/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPQueryUpdate(DirtyLSPRefT *pDirtyLSP, DirtyLSPQueryRecT *pQueryRec)
{
    DirtyLSPQueryStateE eQueryState = pQueryRec->eQueryState;

    // update xlsp resolve process
    if (pQueryRec->eQueryState == DIRTYLSP_QUERYSTATE_RSLV)
    {
        _DirtyLSPQueryUpdateRslv(pQueryRec);
    }

    // initiate connection to xlsp
    if (pQueryRec->eQueryState == DIRTYLSP_QUERYSTATE_INIT)
    {
        _DirtyLSPConnectionOpen(pQueryRec);
    }

    // update xlsp connection process
    if (pQueryRec->eQueryState == DIRTYLSP_QUERYSTATE_CONN)
    {
        _DirtyLSPQueryUpdateConn(pQueryRec);
    }

    // check to see if we need to clean up a ref freed while in RSLV state
    if ((eQueryState == DIRTYLSP_QUERYSTATE_RSLV) && (pQueryRec->eQueryState != DIRTYLSP_QUERYSTATE_RSLV) &&
        (pQueryRec->bCleanup == TRUE))
    {
        NetPrintf(("dirtylspxenon: [%08x] cleaning up query that was freed while resolve was pending\n", (uintptr_t)pQueryRec));
        DirtyLSPQueryFree(pDirtyLSP, pQueryRec);
        return;
    }

    // if state changed, trigger callback
    if (eQueryState != pQueryRec->eQueryState)
    {
        pDirtyLSP->pNotifyCb(pDirtyLSP, pQueryRec, pDirtyLSP->pUserRef);
    }
    /* the notify callback is allowed to free the query, so it should
       not be accessed after this point */
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPQueryUpdateAll

    \Description
        Iterate through all queries and update them

    \Input *pDirtyLSP   - pointer to module state

    \Version 11/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPQueryUpdateAll(DirtyLSPRefT *pDirtyLSP)
{
    DirtyLSPQueryRecT *pQueryRec, *pQueryRecNext;

    // iterate through query linked list
    for (pQueryRec = pDirtyLSP->pQueryList; pQueryRec != NULL; pQueryRec = pQueryRecNext)
    {
        // cache next pointer so record may be freed in the user notify callback
        pQueryRecNext = pQueryRec->pNext;

        // update the query
        _DirtyLSPQueryUpdate(pDirtyLSP, pQueryRec);
    }
}

/*F********************************************************************************/
/*!
    \Function _DirtyLSPIdle

    \Description
        NetConn idle function to update the DirtyLSP module.

    \Input *pData   - pointer to module state
    \Input uTick    - current tick count

    \Notes
        This function is installed as a NetConn Idle function.  NetConnIdle()
        must be regularly polled for this function to be called.

    \Version 11/17/2009 (jbrookes)
*/
/********************************************************************************F*/
static void _DirtyLSPIdle(void *pData, uint32_t uTick)
{
    DirtyLSPRefT *pDirtyLSP = (DirtyLSPRefT *)pData;

    // acquire dirtylsp critical section
    NetCritEnter(&pDirtyLSP->QueryCrit);

    // update any ongoing queries
    _DirtyLSPQueryUpdateAll(pDirtyLSP);

    // check for connections to be closed
    _DirtyLSPCacheUpdate(_DirtyLSP_pCache);

    // release critical section
    NetCritLeave(&pDirtyLSP->QueryCrit);
}

/*F*************************************************************************************************/
/*!
    \Function _DirtyLSPQueryFreeCallback

    \Description
        Free an allocated query that was in the RSLV state when the DirtyLSPXenon module that owned
        the query was destroyed.  Because the memory allocated for an ongoing overlapped operation
        cannot be freed, responsibility for disposal of the query is handed off to NetConn, which
        calls this function until the object can be freed.

    \Input *pQueryMem   - pointer to query record

    \Output
        int32_t         - zero=success; -1=try again; other negative=error

    \Version 03/10/2010 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _DirtyLSPQueryFreeCallback(void *pQueryMem)
{
    DirtyLSPQueryRecT *pQueryRec = (DirtyLSPQueryRecT *)pQueryMem;

    // update the query if we are in the resolve state
    if (pQueryRec->eQueryState == DIRTYLSP_QUERYSTATE_RSLV)
    {
        _DirtyLSPQueryUpdateRslv(pQueryRec);
    }
    if (pQueryRec->eQueryState != DIRTYLSP_QUERYSTATE_RSLV)
    {
        NetPrintf(("dirtylspxenon: [%08x] query freed via netconn cleanup callback\n", (uintptr_t)pQueryRec));
        _DirtyLSPQueryFree(pQueryRec);
        return(0);
    }
    return(-1);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyLSPCreate

    \Description
        Create the DirtyLSP module state.

    \Input uMaxQueryCount   - max number of XLSPs that will be returned in a single query

    \Output
        DirtyLSPRefT *      - pointer to module state, or NULL

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
DirtyLSPRefT *DirtyLSPCreate(uint32_t uMaxQueryCount)
{
    DirtyLSPRefT *pDirtyLSP;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // validate query count
    if (uMaxQueryCount < DIRTYLSP_MINQUERYCOUNT)
    {
        NetPrintf(("dirtylspxenon: clamping query count to %d\n", DIRTYLSP_MINQUERYCOUNT));
        uMaxQueryCount = DIRTYLSP_MINQUERYCOUNT;
    }
    if (uMaxQueryCount > XTITLE_SERVER_MAX_LSP_INFO)
    {
        NetPrintf(("dirtylspxenon: clamping query count to %d\n", XTITLE_SERVER_MAX_LSP_INFO));
        uMaxQueryCount = XTITLE_SERVER_MAX_LSP_INFO;
    }

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pDirtyLSP = DirtyMemAlloc(sizeof(*pDirtyLSP), DIRTYLSP_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtylspxenon: could not allocate module state\n"));
        return(NULL);
    }
    memset(pDirtyLSP, 0, sizeof(*pDirtyLSP));
    pDirtyLSP->iMemGroup = iMemGroup;
    pDirtyLSP->pMemGroupUserData = pMemGroupUserData;
    pDirtyLSP->uMaxQueryCount = uMaxQueryCount;

    // set default callback
    pDirtyLSP->pNotifyCb = _DirtyLSPQueryCallback;

    // init query critical section
    NetCritInit(&pDirtyLSP->QueryCrit, "dirtylsp");

    // add NetConn idle handler
    NetConnIdleAdd(_DirtyLSPIdle, pDirtyLSP);

    // create (or refcount, if already created) global cache
    if ((_DirtyLSP_pCache = _DirtyLSPCacheCreate(_DirtyLSP_pCache, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtylspxenon: could not allocate cache\n"));
        DirtyLSPDestroy(pDirtyLSP);
        pDirtyLSP = NULL;
    }

    // return ref to caller
    return(pDirtyLSP);
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPCallback

    \Description
        Set optional notification callback.

    \Input *pDirtyLSP   - pointer to module state
    \Input *pNotifyCb   - pointer to notification callback
    \Input *pUserRef    - user-supplied callback ref (may be NULL)

    \Version 11/17/2009 (jbrookes)
*/
/********************************************************************************F*/
void DirtyLSPCallback(DirtyLSPRefT *pDirtyLSP, DirtyLSPNotifyCbT *pNotifyCb, void *pUserRef)
{
    pDirtyLSP->pNotifyCb = pNotifyCb;
    pDirtyLSP->pUserRef = pUserRef;
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPDestroy

    \Description
        Destroy the DirtyLSP module.

    \Input *pDirtyLSP   - pointer to module state

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
void DirtyLSPDestroy(DirtyLSPRefT *pDirtyLSP)
{
    DirtyLSPQueryRecT *pQueryRec, *pNextQueryRec;

    // del NetConn idle handler
    NetConnIdleDel(_DirtyLSPIdle, pDirtyLSP);

    // decrement refcount for (and destroy if refcounted to zero) global cache
    _DirtyLSP_pCache = _DirtyLSPCacheDestroy(_DirtyLSP_pCache);

    // remove any remaining query records from query list
    NetCritEnter(&pDirtyLSP->QueryCrit);
    for (pQueryRec = pDirtyLSP->pQueryList; pQueryRec != NULL; pQueryRec = pNextQueryRec)
    {
        pNextQueryRec = pQueryRec->pNext;

        // if no resolve pending, we can free the query
        if (pQueryRec->eQueryState != DIRTYLSP_QUERYSTATE_RSLV)
        {
            _DirtyLSPQueryFree(pQueryRec);
        }
        else
        {
            // resolve is in progress, so transfer query destruction responsibility to NetConn
            NetPrintf(("dirtylspxenon: [%08x] query free on module shutdown deferred to netconn due to pending resolve\n", (uintptr_t)pQueryRec));
            NetConnControl('recu', 0, 0, (void *)_DirtyLSPQueryFreeCallback, pQueryRec);
        }
    }
    NetCritLeave(&pDirtyLSP->QueryCrit);

    // kill query critical section
    NetCritKill(&pDirtyLSP->QueryCrit);

    // release module memory
    DirtyMemFree(pDirtyLSP, DIRTYLSP_MEMID, pDirtyLSP->iMemGroup, pDirtyLSP->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPQueryStart

    \Description
        Start an LSP query.

    \Input *pDirtyLSP       - pointer to module state
    \Input *pQueryStr       - query string (may be empty for global query; not recommended)

    \Output
        DirtyLSPQueryRecT * - query record or NULL on failure

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
DirtyLSPQueryRecT *DirtyLSPQueryStart(DirtyLSPRefT *pDirtyLSP, const char *pQueryStr)
{
    return(DirtyLSPQueryStart2(pDirtyLSP, pQueryStr, pDirtyLSP->uMaxQueryCount));
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPQueryStart2

    \Description
        Start an LSP query.

    \Input *pDirtyLSP       - pointer to module state
    \Input *pQueryStr       - query string (may be empty for global query; not recommended)
    \Input uMaxResponses    - maximum number of responses to allow

    \Output
        DirtyLSPQueryRecT * - query record or NULL on failure

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
DirtyLSPQueryRecT *DirtyLSPQueryStart2(DirtyLSPRefT *pDirtyLSP, const char *pQueryStr, uint32_t uMaxResponses)
{
    DirtyLSPQueryRecT *pQueryRec;
    uint32_t uAddress;

    // check to see if we already have this LSP resolved
    if ((uAddress = _DirtyLSPCacheGetAddr(_DirtyLSP_pCache, pQueryStr)) != 0)
    {
        /* if this LSP has already been resolved, allocate a query record and start
           it at the connect phase.  we do this because there are cases where the
           security association is still valid but the the connection is not, and
           calling XNetConnect() will restore the connection.  if the connection is
           already valid, the connection request will immediately succeed. */
        if ((pQueryRec = _DirtyLSPQueryAlloc(pQueryStr, 0, 0)) != NULL)
        {
            pQueryRec->uAddress = uAddress;
            pQueryRec->InAddr.s_addr = SocketHtonl(uAddress);
            pQueryRec->eQueryState = DIRTYLSP_QUERYSTATE_INIT;
        }
    }
    else
    {
        pQueryRec = _DirtyLSPQueryStart(pDirtyLSP, pQueryStr, uMaxResponses);
    }

    // enqueue query record
    if (pQueryRec != NULL)
    {
        // add query record to query list
        NetCritEnter(&pDirtyLSP->QueryCrit);
        pQueryRec->pNext = pDirtyLSP->pQueryList;
        pDirtyLSP->pQueryList = pQueryRec;
        NetCritLeave(&pDirtyLSP->QueryCrit);
    }

    // return query record to caller
    return(pQueryRec);
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPQueryFree

    \Description
        Free an allocated query.

    \Input *pDirtyLSP   - pointer to module state
    \Input *pQueryRec   - pointer to query record to free

    \Notes
        This function may be called from a user callback.

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
void DirtyLSPQueryFree(DirtyLSPRefT *pDirtyLSP, DirtyLSPQueryRecT *pQueryRec)
{
    DirtyLSPQueryRecT **ppQueryRec;

    // defer free if we are in a resolve state
    if (pQueryRec->eQueryState == DIRTYLSP_QUERYSTATE_RSLV)
    {
        NetPrintf(("dirtylspxenon: [%08x] deferring query free as it is in RSLV state\n", (uintptr_t)pQueryRec));
        pQueryRec->bCleanup = TRUE;
        return;
    }

    // remove query record from query list
    NetCritEnter(&pDirtyLSP->QueryCrit);
    for (ppQueryRec = &pDirtyLSP->pQueryList; *ppQueryRec != NULL; ppQueryRec = &(*ppQueryRec)->pNext)
    {
        if (*ppQueryRec == pQueryRec)
        {
            *ppQueryRec = pQueryRec->pNext;
            break;
        }
    }
    NetCritLeave(&pDirtyLSP->QueryCrit);

    // free query
    _DirtyLSPQueryFree(pQueryRec);
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPQueryDone

    \Description
        Poll query for completion

    \Input *pDirtyLSP   - pointer to module state
    \Input *pQueryRec   - pointer to query record

    \Output
        int32_t         - negative=error, zero=pending, positive=complete

    \Version 03/13/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyLSPQueryDone(DirtyLSPRefT *pDirtyLSP, DirtyLSPQueryRecT *pQueryRec)
{
    // translate query state to result code
    if (pQueryRec->eQueryState == DIRTYLSP_QUERYSTATE_DONE)
    {
        return(1);
    }
    else if (pQueryRec->eQueryState == DIRTYLSP_QUERYSTATE_FAIL)
    {
        return(-1);
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPConnectionUpdate

    \Description
        Update specified connection.  Must be polled to prevent the security
        association from timing out.

    \Input uAddress     - secure address of connection to update

    \Version 04/15/2010 (jbrookes)
*/
/********************************************************************************F*/
void DirtyLSPConnectionUpdate(uint32_t uAddress)
{
    if (_DirtyLSP_pCache)
    {
        _DirtyLSPCacheTouch(_DirtyLSP_pCache, uAddress);
    }
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPConnectionClose [DEPRECATED]

    \Description
        Close connection associated with given secure address, if it is an address
        that is managed by DirtyLSP.

    \Input *pDirtyLSP   - pointer to module state
    \Input uAddress     - secure address of connection to close

    \Output
        int32_t         - zero=connection closed, negative=not mapped, else refcount

    \Version 11/17/2009 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyLSPConnectionClose(DirtyLSPRefT *pDirtyLSP, uint32_t uAddress)
{
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPControl

    \Description
        DirtyLSP control function.  Different selectors control different behaviors.

    \Input *pDirtyLSP   - reference pointer
    \Input iSelect      - control selector
    \Input iValue       - selector specific
    \Input iValue2      - selector specific
    \Input *pValue      - selector specific

    \Output
        int32_t         - selector specific

    \Notes
        Selectors are:

        \verbatim
            SELECTOR    DESCRIPTION
            'cbfp'      Sets cache notification callback (internal use only)
            'cbup'      Sets cache notification user ref (internal use only)
            'xtim'      Sets cache timeout
            'xclr'      Clears global cache
        \endverbatim

    \Version 02/24/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyLSPControl(DirtyLSPRefT *pDirtyLSP, int32_t iSelect, int32_t iValue, int32_t iValue2, void *pValue)
{
    DirtyLSPCacheT *pDirtyCache = _DirtyLSP_pCache;

    // set cache notification callback
    if ((iSelect == 'cbfp') && (pDirtyCache != NULL))
    {
        _DirtyLSPCacheCallback(pDirtyCache, (DirtyLSPCacheCbT *)pValue, NULL);
        return(0);
    }

    // set cache notification user data pointer
    if ((iSelect == 'cbup') && (pDirtyCache != NULL))
    {
        _DirtyLSPCacheCallback(pDirtyCache, NULL, pValue);
        return(0);
    }

    // set cache timeout
    if ((iSelect == 'xtim') && (pDirtyCache != NULL))
    {
        _DirtyLSPCacheTimeout(pDirtyCache, iValue);
        return(0);
    }

    // handle cache clear selector
    if ((iSelect == 'xclr') && (pDirtyCache != NULL))
    {
        _DirtyLSPCacheReset(pDirtyCache);
        return(0);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function DirtyLSPStatus

    \Description
        Return status of DirtyLSP module.  Status type depends on selector.

    \Input *pDirtyLSP   - reference pointer
    \Input iSelect      - info selector (see Notes)
    \Input *pBuffer     - [out] storage for selector-specific output
    \Input iBufSize     - size of buffer

    \Output
        int32_t         - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
    \endverbatim

    \Version 02/24/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyLSPStatus(DirtyLSPRefT *pDirtyLSP, int32_t iSelect, void *pBuffer, int32_t iBufSize)
{
    return(-1);
}

