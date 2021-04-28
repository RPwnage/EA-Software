/*H*************************************************************************************************/
/*!

    \File    dirtynet.c

    \Description
        Platform-independent network related routines.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    1.0        01/02/02 (GWS) First Version
    \Version    1.1        01/27/03 (JLB) Split from dirtynetwin.c

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#include "dirtynetpriv.h"

/*** Defines ***************************************************************************/

//! 30s hostname cache timeout
#define DIRTYNET_HOSTNAMECACHELIFETIME  (30*1000)

//! verbose logging of dirtynet packet queue operations
#define DIRTYNET_PACKETQUEUEDEBUG       (DIRTYCODE_LOGGING && FALSE)

//! verbose logging of dirtynet rate estimation
#define DIRTYNET_RATEDEBUG              (DIRTYCODE_LOGGING && FALSE)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! socket hostname cache entry
typedef struct SocketHostnameCacheEntryT
{
    char strDnsName[256];
    uint32_t uAddress;
    uint32_t uTimer;
} SocketHostnameCacheEntryT;

//! socket hostname cache
struct SocketHostnameCacheT
{
    int32_t iMaxEntries;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    SocketHostnameCacheEntryT CacheEntries[1];  //!< variable-length cache entry list
};

//! socket packet queue
struct SocketPacketQueueT
{
    int32_t iMemGroup;
    void *pMemGroupUserData;

    int8_t iNumPackets;         //!< number of packets in the queue
    int8_t iMaxPackets;         //!< max queue size
    int8_t iPacketHead;         //!< current packet queue head
    int8_t iPacketTail;         //!< current packet queue tail

    uint32_t uLatency;          //!< simulated packet latency target, in milliseconds
    uint32_t uDeviation;        //!< simulated packet deviation target, in milliseconds
    uint32_t uPacketLoss;       //!< packet loss percentage, 16.16 fractional integer

    uint32_t uLatencyTime;      //!< current amount of latency in the packet queue
    int32_t iDeviationTime;     //!< current deviation

    SocketPacketQueueEntryT aPacketQueue[1]; //!< variable-length queue entry list
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables


// Public variables


/*** Private Functions *****************************************************************/

/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    SockaddrCompare

    \Description
        Compare two sockaddr structs and see if address is the same. This is different
        from simple binary compare because only relevent fields are checked.

    \Input *addr1   - address #1
    \Input *addr2   - address to compare with address #1

    \Output
        int32_t         - zero=no error, negative=error

    \Version 10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t SockaddrCompare(struct sockaddr *addr1, struct sockaddr *addr2)
{
    int32_t len = sizeof(*addr1)-sizeof(addr1->sa_family);

    // make sure address family matches
    if (addr1->sa_family != addr2->sa_family)
        return(addr1->sa_family-addr2->sa_family);

    // do type specific comparison
    if (addr1->sa_family == AF_INET)
        len = 2+4;

    // fall back to binary compare
    return(memcmp(addr1->sa_data, addr2->sa_data, len));
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInSetAddrText

    \Description
        Set Internet address component of sockaddr struct from textual address (a.b.c.d).

    \Input *addr    - sockaddr structure
    \Input *s       - textual address

    \Output
        int32_t     - zero=no error, negative=error

    \Version 10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
int32_t SockaddrInSetAddrText(struct sockaddr *addr, const char *s)
{
    int32_t i;
    unsigned char *ipaddr = (unsigned char *)(addr->sa_data+2);

    for (i = 0; i < 4; ++i, ++s) {
        ipaddr[i] = 0;
        while ((*s >= '0') && (*s <= '9'))
            ipaddr[i] = (ipaddr[i]*10) + (*s++ & 15);
        if ((i < 3) && (*s != '.')) {
            ipaddr[0] = ipaddr[1] = 0; ipaddr[2] = ipaddr[3] = 0;
            return(-1);
        }
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInGetAddrText

    \Description
        Return Internet address component of sockaddr struct in textual form (a.b.c.d).

    \Input *addr    - sockaddr struct
    \Input *str     - address buffer
    \Input len      - address length

    \Output
        char *      - returns str on success, NULL on failure

    \Version    10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
char *SockaddrInGetAddrText(struct sockaddr *addr, char *str, int32_t len)
{
    int32_t i;
    char *s = str;

    // make sure buffer has room
    if (len <= 0)
        return(NULL);
    if (len < 16) {
        *s = 0;
        return(NULL);
    }

    // convert to text form
    for (i = 2; i < 6; ++i) {
        int32_t val = (unsigned char)addr->sa_data[i];
        if (val > 99) {
            *s++ = (char)('0'+(val/100));
            val %= 100;
            *s++ = (char)('0'+(val/10));
            val %= 10;
        }
        if (val > 9) {
            *s++ = (char)('0'+(val/10));
            val %= 10;
        }
        *s++ = (char)('0'+val);
        if (i < 5) {
            *s++ = '.';
        }
    }

    *s = 0;
    return(str);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketHtons

    \Description
        Convert int16_t from host to network byte order

    \Input addr     - value to convert

    \Output
        uint16_t    - converted value

    \Version    1.0        10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint16_t SocketHtons(uint16_t addr)
{
    unsigned char netw[2];

    ds_memcpy((char *)netw, (char *)&addr, sizeof(addr));

    return((netw[0]<<8)|(netw[1]<<0));
}

/*F*************************************************************************************************/
/*!
    \Function    SocketHtonl

    \Description
        Convert int32_t from host to network byte order.

    \Input addr     - value to convert

    \Output
        uint32_t    - converted value

    \Version 10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint32_t SocketHtonl(uint32_t addr)
{
    unsigned char netw[4];

    ds_memcpy((char *)netw, (char *)&addr, sizeof(addr));

    return((((((netw[0]<<8)|netw[1])<<8)|netw[2])<<8)|netw[3]);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketNtohs

    \Description
        Convert int16_t from network to host byte order.

    \Input addr   - value to convert

    \Output
        uint16_t  - converted value

    \Version 10/0/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint16_t SocketNtohs(uint16_t addr)
{
    unsigned char netw[2];

    netw[1] = (unsigned char)addr;
    addr >>= 8;
    netw[0] = (unsigned char)addr;

    ds_memcpy((char *)&addr, (char *)netw, sizeof(addr));

    return(addr);
}

/*F*************************************************************************************************/
/*!
    \Function    SocketNtohl

    \Description
        Convert int32_t from network to host byte order.

    \Input addr     - value to convert

    \Output
        uint32_t    - converted value

    \Version    1.0        10/04/99 (GWS) First Version

*/
/*************************************************************************************************F*/
uint32_t SocketNtohl(uint32_t addr)
{
    unsigned char netw[4];

    netw[3] = (unsigned char)addr;
    addr >>= 8;
    netw[2] = (unsigned char)addr;
    addr >>= 8;
    netw[1] = (unsigned char)addr;
    addr >>= 8;
    netw[0] = (unsigned char)addr;

    ds_memcpy((char *)&addr, (char *)netw, sizeof(addr));

    return(addr);
}

/*F*************************************************************************************************/
/*!
    \Function SocketInAddrGetText

    \Description
        Convert 32-bit internet address into textual form.

    \Input addr     - address
    \Input *str     - [out] address buffer
    \Input len      - address length

    \Output
        char *      - returns str on success, NULL on failure

    \Version 06/17/2009 (jbrookes)
*/
/*************************************************************************************************F*/
char *SocketInAddrGetText(uint32_t addr, char *str, int32_t len)
{
    struct sockaddr sa;
    SockaddrInSetAddr(&sa,addr);
    return(SockaddrInGetAddrText(&sa, str, len));
}

/*F*************************************************************************************************/
/*!
    \Function    SocketInTextGetAddr

    \Description
        Convert textual internet address into 32-bit integer form

    \Input *pAddrText   - textual address

    \Output
        int32_t         - integer form

    \Version 11/23/02 (JLB) First Version

*/
/*************************************************************************************************F*/
int32_t SocketInTextGetAddr(const char *pAddrText)
{
    struct sockaddr SockAddr;
    int32_t iAddr = 0;

    if (SockaddrInSetAddrText(&SockAddr, pAddrText) == 0)
    {
        iAddr = SockaddrInGetAddr(&SockAddr);
    }
    return(iAddr);
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInParse

    \Description
        Convert textual internet address:port into sockaddr structure

        If the textual internet address:port is followed by a second :port, the second port
        is optionally parsed into pPort2, if not NULL.

    \Input *sa      - sockaddr to fill in
    \Input *pParse  - textual address

    \Output
        int32_t         - flags:
            0=parsed nothing
            1=parsed addr
            2=parsed port
            3=parsed addr+port

    \Version 11/23/02 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t SockaddrInParse(struct sockaddr *sa, const char *pParse)
{
    int32_t iReturn = 0, iPort = 0;
    uint32_t uAddr = 0;

    // init the address
    SockaddrInit(sa, AF_INET);

    // parse addr:port
    iReturn = SockaddrInParse2(&uAddr, &iPort, NULL, pParse);

    // set addr:port in sockaddr
    SockaddrInSetAddr(sa, uAddr);
    SockaddrInSetPort(sa, iPort);

    // return parse info
    return(iReturn);
}

/*F*************************************************************************************************/
/*!
    \Function    SockaddrInParse2

    \Description
        Convert textual internet address:port into sockaddr structure

        If the textual internet address:port is followed by a second :port, the second port
        is optionally parsed into pPort2, if not NULL.

    \Input *pAddr   - address to fill in
    \Input *pPort   - port to fill in
    \Input *pPort2  - second port to fill in
    \Input *pParse  - textual address

    \Output
        int32_t         - flags:
            0=parsed nothing
            1=parsed addr
            2=parsed port
            3=parsed addr+port
            4=parsed port2

    \Version 11/23/02 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t SockaddrInParse2(uint32_t *pAddr, int32_t *pPort, int32_t *pPort2, const char *pParse)
{
    int32_t iReturn = 0;
    uint32_t uVal;

    // skip embedded white-space
    while ((*pParse > 0) && (*pParse <= ' '))
    {
        ++pParse;
    }

    // parse the address (no dns for listen)
    for (uVal = 0; ((*pParse >= '0') && (*pParse <= '9')) || (*pParse == '.'); ++pParse)
    {
        // either add or shift
        if (*pParse != '.')
        {
            uVal = (uVal - (uVal & 255)) + ((uVal & 255) * 10) + (*pParse & 15);
        }
        else
        {
            uVal <<= 8;
        }
    }
    if ((*pAddr = uVal) != 0)
    {
        iReturn |= 1;
    }

    // skip non-port info
    while ((*pParse != ':') && (*pParse != 0))
    {
        ++pParse;
    }

    // parse the port
    uVal = 0;
    if (*pParse == ':')
    {
        for (++pParse; (*pParse >= '0') && (*pParse <= '9'); ++pParse)
        {
            uVal = (uVal * 10) + (*pParse & 15);
        }
        iReturn |= 2;
    }
    *pPort = (int32_t)uVal;

    // parse port2 (optional)
    if (pPort2 != NULL)
    {
        uVal = 0;
        if (*pParse == ':')
        {
            for (++pParse; (*pParse >= '0') && (*pParse <= '9'); ++pParse)
            {
                uVal = (uVal * 10) + (*pParse & 15);
            }
            iReturn |= 4;
        }
        *pPort2 = (int32_t)uVal;
    }

    // return the address
    return(iReturn);
}

/*
    HostName Cache functions
*/

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheCreate

    \Description
        Create short-term hostname (DNS) cache

    \Input iMemGroup            - memgroup to alloc/free with
    \Input *pMemGroupUserData   - memgroup user data to alloc/free with

    \Output
        SocketHostnameCacheT *  - hostname cache or NULL on failure

    \Version 10/09/2013 (jbrookes)
*/
/********************************************************************************F*/
SocketHostnameCacheT *SocketHostnameCacheCreate(int32_t iMemGroup, void *pMemGroupUserData)
{
    const int32_t iMaxEntries = 16;
    int32_t iCacheSize = sizeof(SocketHostnameCacheT) + (iMaxEntries * sizeof(SocketHostnameCacheEntryT));
    SocketHostnameCacheT *pCache;

    // alloc and init cache
    if ((pCache = DirtyMemAlloc(iCacheSize, SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynet: could not alloc %d bytes for hostname cache\n", iCacheSize));
        return(NULL);
    }
    memset(pCache, 0, iCacheSize);

    // set base info
    pCache->iMaxEntries = iMaxEntries;
    pCache->iMemGroup = iMemGroup;
    pCache->pMemGroupUserData = pMemGroupUserData;

    // return to caller
    return(pCache);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheDestroy

    \Description
        Destroy short-term hostname (DNS) cache

    \Input *pCache      - hostname cache

    \Version 10/09/2013 (jbrookes)
*/
/********************************************************************************F*/
void SocketHostnameCacheDestroy(SocketHostnameCacheT *pCache)
{
    DirtyMemFree(pCache, SOCKET_MEMID, pCache->iMemGroup, pCache->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheAdd

    \Description
        Add hostname and address to hostname cache

    \Input *pCache      - hostname cache
    \Input *pStrHost    - hostname to add
    \Input uAddress     - address of hostname
    \Input iVerbose     - debug level

    \Version 10/09/2013 (jbrookes)
*/
/********************************************************************************F*/
void SocketHostnameCacheAdd(SocketHostnameCacheT *pCache, const char *pStrHost, uint32_t uAddress, int32_t iVerbose)
{
    SocketHostnameCacheEntryT *pCacheEntry;
    int32_t iCacheIdx;

    // if we're already in the cache, bail
    if (SocketHostnameCacheGet(pCache, pStrHost, 0) != 0)
    {
        return;
    }

    // scan cache for an open entry
    NetCritEnter(NULL);
    for (iCacheIdx = 0; iCacheIdx < pCache->iMaxEntries; iCacheIdx += 1)
    {
        pCacheEntry = &pCache->CacheEntries[iCacheIdx];
        if (pCacheEntry->uAddress == 0)
        {
            NetPrintfVerbose((iVerbose, 1, "dirtynet: adding hostname cache entry %s/%a\n", pStrHost, uAddress));
            ds_strnzcpy(pCacheEntry->strDnsName, pStrHost, sizeof(pCacheEntry->strDnsName));
            pCacheEntry->uAddress = uAddress;
            pCacheEntry->uTimer = NetTick();
            break;
        }
    }
    NetCritLeave(NULL);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameCacheGet

    \Description
        Get address for hostname from cache, if available.  Also expires cache
        entry if applicable.

    \Input *pCache      - hostname cache
    \Input *pStrHost    - hostname to add
    \Input iVerbose     - debug level

    \Output
        uint32_t        - address for hostname, or zero if not in cache

    \Version 10/09/2013 (jbrookes)
*/
/********************************************************************************F*/
uint32_t SocketHostnameCacheGet(SocketHostnameCacheT *pCache, const char *pStrHost, int32_t iVerbose)
{
    SocketHostnameCacheEntryT *pCacheEntry;
    uint32_t uCurTick, uAddress;
    int32_t iCacheIdx;

    // scan cache for dns entry
    NetCritEnter(NULL);
    for (iCacheIdx = 0, uAddress = 0, uCurTick = NetTick(); iCacheIdx < pCache->iMaxEntries; iCacheIdx += 1)
    {
        pCacheEntry = &pCache->CacheEntries[iCacheIdx];
        // first check all entries for expiration
        if (NetTickDiff(uCurTick, pCacheEntry->uTimer) > DIRTYNET_HOSTNAMECACHELIFETIME)
        {
            NetPrintfVerbose((iVerbose, 1, "dirtynet: expiring hostname cache entry %s/%a\n", pCacheEntry->strDnsName, pCacheEntry->uAddress));
            memset(pCacheEntry, 0, sizeof(*pCacheEntry));
            continue;
        }
        // check for entry we want
        if (!strcmp(pCacheEntry->strDnsName, pStrHost))
        {
            NetPrintfVerbose((iVerbose, 0, "dirtynet: %s=%a [cache]\n", pCacheEntry->strDnsName, pCacheEntry->uAddress));
            uAddress = pCache->CacheEntries[iCacheIdx].uAddress;
            break;
        }
    }
    NetCritLeave(NULL);
    return(uAddress);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameAddRef

    \Description
        Check for in-progress DNS requests we can piggyback on, instead of issuing
        a new request.

    \Input **ppHostList - list of active lookups
    \Input *pHost       - current lookup
    \Input bUseRef      - force using a new entry if bUseRef=FALSE (should normally be TRUE)

    \Output
        HostentT *      - Pre-existing DNS request we have refcounted, or NULL

    \Version 01/16/2014 (jbrookes)
*/
/********************************************************************************F*/
HostentT *SocketHostnameAddRef(HostentT **ppHostList, HostentT *pHost, uint8_t bUseRef)
{
    HostentT *pHost2 = NULL;

    // look for refcounted lookup
    NetCritEnter(NULL);
    if (bUseRef == TRUE)
    {
        for (pHost2 = *ppHostList; pHost2 != NULL; pHost2 = pHost2->pNext)
        {
            if (!strcmp(pHost2->name, pHost->name))
            {
                break;
            }
        }
    }

    // new lookup, so add to list
    if (pHost2 == NULL)
    {
        pHost->refcount = 1;
        pHost->pNext = *ppHostList;
        *ppHostList = pHost;
        pHost = NULL;
    }
    else // found an in-progress lookup, so piggyback on it
    {
        pHost = pHost2;
        pHost->refcount += 1;
    }

    NetCritLeave(NULL);
    return(pHost);
}

/*F********************************************************************************/
/*!
    \Function SocketHostnameListProcess

    \Description
        Process list of in-progress DNS requests, disposing of those that are
        completed and no longer referenced.

    \Input **ppHostList - list of active lookups
    \Input iMemGroup    - memgroup hostname lookup records are allocated with
    \Input *pMemGroupUserData - memgroup userdata hostname lookup records are allocated with

    \Notes
        This function is called from the SocketIdle thread, which is already guarded
        by the global critical section.  It is therefore assumed that it does not
        need to be explicitly guarded here.

    \Version 01/16/2014 (jbrookes)
*/
/********************************************************************************F*/
void SocketHostnameListProcess(HostentT **ppHostList, int32_t iMemGroup, void *pMemGroupUserData)
{
    HostentT **ppHost;
    for (ppHost = ppHostList; *ppHost != NULL;)
    {
        if ((*ppHost)->refcount == 0)
        {
            HostentT *pHost = *ppHost;
            *ppHost = (*ppHost)->pNext;
            DirtyMemFree(pHost, SOCKET_MEMID, iMemGroup, pMemGroupUserData);
        }
        else
        {
            ppHost = &(*ppHost)->pNext;
        }
    }
}

/*
    Packet Queue functions
*/

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueCreate

    \Description
        Create a packet queue

    \Input iMaxPackets          - size of queue, in packets (max 127)
    \Input iMemGroup            - memgroup to alloc/free with
    \Input *pMemGroupUserData   - memgroup user data to alloc/free with

    \Output
        SocketPacketQueueT *    - packet queue, or NULL on failure

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
SocketPacketQueueT *SocketPacketQueueCreate(int32_t iMaxPackets, int32_t iMemGroup, void *pMemGroupUserData)
{
    SocketPacketQueueT *pPacketQueue;
    int32_t iQueueSize;

    // enforce min/max queue sizes
    iMaxPackets = DS_CLAMP(iMaxPackets, 1, 127);

    // calculate memory required for queue
    iQueueSize = sizeof(*pPacketQueue) + ((iMaxPackets-1) * sizeof(pPacketQueue->aPacketQueue[0]));

    // alloc and init queue
    if ((pPacketQueue = DirtyMemAlloc(iQueueSize, SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynet: could not alloc %d bytes for packet queue\n", iQueueSize));
        return(NULL);
    }
    memset(pPacketQueue, 0, iQueueSize);

    // set base info
    pPacketQueue->iNumPackets = 0;
    pPacketQueue->iMaxPackets = iMaxPackets;
    pPacketQueue->iMemGroup = iMemGroup;
    pPacketQueue->pMemGroupUserData = pMemGroupUserData;

    // latency/packet loss simulation setup
    pPacketQueue->uLatencyTime = NetTick();

    //$$temp - testing
    //pPacketQueue->uLatency = 100;
    //pPacketQueue->uDeviation = 5;
    //pPacketQueue->uPacketLoss = 5*65536;

    // return queue to caller
    return(pPacketQueue);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueDestroy

    \Description
        Destroy packet queue

    \Input *pPacketQueue    - packet queue to destroy

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
void SocketPacketQueueDestroy(SocketPacketQueueT *pPacketQueue)
{
    DirtyMemFree(pPacketQueue, SOCKET_MEMID, pPacketQueue->iMemGroup, pPacketQueue->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueResize

    \Description
        Resize a packet queue, if the max packet size is different

    \Input *pPacketQueue        - packet queue to resize (may be null)
    \Input iMaxPackets          - new size of queue, in packets (max 127)
    \Input iMemGroup            - memgroup to alloc/free with
    \Input *pMemGroupUserData   - memgroup user data to alloc/free with

    \Output
        SocketPacketQueueT * - pointer to resized packet queue

    \Notes
        If the new max queue size is less than the number of packets in the current
        queue, packets will be overwritten in the usual manner (older discarded in
        favor of newer).

    \Version 05/30/2014 (jbrookes)
*/
/********************************************************************************F*/
SocketPacketQueueT *SocketPacketQueueResize(SocketPacketQueueT *pPacketQueue, int32_t iMaxPackets, int32_t iMemGroup, void *pMemGroupUserData)
{
    uint8_t aPacketData[SOCKET_MAXUDPRECV];
    SocketPacketQueueT *pNewPacketQueue;
    struct sockaddr PacketAddr;
    int32_t iPacketSize;

    // enforce min/max queue sizes
    iMaxPackets = DS_CLAMP(iMaxPackets, 1, 127);

    // if we have a queue and it's already the right size, return it
    if ((pPacketQueue != NULL) && (pPacketQueue->iMaxPackets == iMaxPackets))
    {
        return(pPacketQueue);
    }

    // create new queue
    NetPrintf(("dirtynet: [%p] re-creating socket packet queue with %d max packets\n", pPacketQueue, iMaxPackets));
    if ((pNewPacketQueue = SocketPacketQueueCreate(iMaxPackets, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynet: could not allocate new packet queue\n"));
        return(pPacketQueue);
    }

    // copy old data (if any) over, and destroy the old packet queue
    if (pPacketQueue != NULL)
    {
        while ((iPacketSize = SocketPacketQueueRem(pPacketQueue, aPacketData, sizeof(aPacketData), &PacketAddr)) > 0)
        {
            SocketPacketQueueAdd(pNewPacketQueue, aPacketData, iPacketSize, &PacketAddr);
        }
        SocketPacketQueueDestroy(pPacketQueue);
    }

    // return resized queue to caller
    return(pNewPacketQueue);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueAdd

    \Description
        Add a packet to packet queue

    \Input *pPacketQueue    - packet queue to add to
    \Input *pPacketData     - packet data to add to queue
    \Input iPacketSize      - size of packet data
    \Input *pPacketAddr     - remote address associated with packet

    \Output
        int32_t             - zero=success, negative=failure

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueAdd(SocketPacketQueueT *pPacketQueue, const uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr)
{
    SocketPacketQueueEntryT *pQueueEntry;
    // reject if packet data is too large
    if (iPacketSize > SOCKET_MAXUDPRECV)
    {
        NetPrintf(("dirtynet: [%p] packet too large to add to queue\n", pPacketQueue));
        return(-1);
    }
    // if queue is full, overwrite oldest member
    if (pPacketQueue->iNumPackets == pPacketQueue->iMaxPackets)
    {
        NetPrintf(("dirtynet: [%p] add to full queue; oldest entry will be overwritten\n", pPacketQueue));
        pPacketQueue->iPacketHead = (pPacketQueue->iPacketHead + 1) % pPacketQueue->iMaxPackets;
    }
    else
    {
        pPacketQueue->iNumPackets += 1;
    }
    // set packet entry
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] [%d] packet queue entry added (%d entries)\n", pPacketQueue, pPacketQueue->iPacketTail, pPacketQueue->iNumPackets));
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketTail];
    ds_memcpy_s(pQueueEntry->aPacketData, sizeof(pQueueEntry->aPacketData), pPacketData, iPacketSize);
    ds_memcpy(&pQueueEntry->PacketAddr, pPacketAddr, sizeof(pQueueEntry->PacketAddr));
    pQueueEntry->iPacketSize = iPacketSize;
    pQueueEntry->uPacketTick = NetTick();
    // add to queue
    pPacketQueue->iPacketTail = (pPacketQueue->iPacketTail + 1) % pPacketQueue->iMaxPackets;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] head=%d tail=%d\n", pPacketQueue, pPacketQueue->iPacketHead, pPacketQueue->iPacketTail));
    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueAlloc

    \Description
        Alloc a packet queue entry.  This is used when receiving data directly into
        the packet queue data buffer.

    \Input *pPacketQueue    - packet queue to alloc entry from

    \Output
        SocketPacketQueueEntryT * - packet queue entry

    \Version 02/24/2014 (jbrookes)
*/
/********************************************************************************F*/
SocketPacketQueueEntryT *SocketPacketQueueAlloc(SocketPacketQueueT *pPacketQueue)
{
    SocketPacketQueueEntryT *pQueueEntry;
    // if queue is full, alloc over oldest member
    if (pPacketQueue->iNumPackets == pPacketQueue->iMaxPackets)
    {
        // print a warning if we're altering a slot that is in use
        if (pPacketQueue->aPacketQueue[pPacketQueue->iPacketTail].iPacketSize != -1)
        {
            NetPrintf(("dirtynet: [%p] alloc to full queue; oldest entry will be overwritten\n", pPacketQueue));
        }
        pPacketQueue->iPacketHead = (pPacketQueue->iPacketHead + 1) % pPacketQueue->iMaxPackets;
    }
    else
    {
        pPacketQueue->iNumPackets += 1;
    }
    // allocate queue entry
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketTail];
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] [%d] packet queue entry alloc (%d entries)\n", pPacketQueue, pPacketQueue->iPacketTail, pPacketQueue->iNumPackets));
    // mark packet as allocated
    pQueueEntry->iPacketSize = -1;
    // add to queue
    pPacketQueue->iPacketTail = (pPacketQueue->iPacketTail + 1) % pPacketQueue->iMaxPackets;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] head=%d tail=%d\n", pPacketQueue, pPacketQueue->iPacketHead, pPacketQueue->iPacketTail));
    return(pQueueEntry);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueRem

    \Description
        Remove a packet from packet queue

    \Input *pPacketQueue    - packet queue to add to
    \Input *pPacketData     - [out] storage for packet data
    \Input iPacketSize      - size of packet output data buffer
    \Input *pPacketAddr     - [out] storage for packet addr

    \Output
        int32_t             - positive=size of packet, zero=no packet, negative=failure

    \Version 02/21/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueRem(SocketPacketQueueT *pPacketQueue, uint8_t *pPacketData, int32_t iPacketSize, struct sockaddr *pPacketAddr)
{
    SocketPacketQueueEntryT *pQueueEntry;
    uint32_t uCurTick = NetTick();

    // nothing to do if queue is empty
    if (pPacketQueue->iNumPackets == 0)
    {
        return(0);
    }
    // get head packet
    pQueueEntry = &pPacketQueue->aPacketQueue[pPacketQueue->iPacketHead];
    // nothing to do if packet was allocated and has not been filled
    if (pQueueEntry->iPacketSize < 0)
    {
        return(0);
    }

    // apply simulated latency, if enabled
    if (pPacketQueue->uLatency != 0)
    {
        // compare to current latency
        if (NetTickDiff(uCurTick, pQueueEntry->uPacketTick) < (signed)pPacketQueue->uLatency + pPacketQueue->iDeviationTime)
        {
            NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] latency=%d, deviation=%d\n", pPacketQueue, NetTickDiff(uCurTick, pPacketQueue->uLatencyTime), pPacketQueue->iDeviationTime));
            return(0);
        }

        // update packet receive timestamp
        SockaddrInSetMisc(&pQueueEntry->PacketAddr, uCurTick);

        // recalculate deviation
        pPacketQueue->iDeviationTime = (signed)NetRand(pPacketQueue->uDeviation*2) - (signed)pPacketQueue->uDeviation;
    }

    // get packet size to copy (will truncate if output buffer is too small)
    if (iPacketSize > pQueueEntry->iPacketSize)
    {
        iPacketSize = pQueueEntry->iPacketSize;
    }
    // copy out packet data and source
    ds_memcpy(pPacketData, pQueueEntry->aPacketData, iPacketSize);
    ds_memcpy(pPacketAddr, &pQueueEntry->PacketAddr, sizeof(pQueueEntry->PacketAddr));
    // remove packet from queue
    pPacketQueue->iNumPackets -= 1;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] [%d] packet queue entry removed from head in %dms (%d entries)\n", pPacketQueue, pPacketQueue->iPacketHead,
        NetTickDiff(NetTick(), pQueueEntry->uPacketTick), pPacketQueue->iNumPackets));
    pPacketQueue->iPacketHead = (pPacketQueue->iPacketHead + 1) % pPacketQueue->iMaxPackets;
    NetPrintfVerbose((DIRTYNET_PACKETQUEUEDEBUG, 0, "dirtynet: [%p] head=%d tail=%d\n", pPacketQueue, pPacketQueue->iPacketHead, pPacketQueue->iPacketTail));

    // simulate packet loss
    if (pPacketQueue->uPacketLoss != 0)
    {
        uint32_t uRand = NetRand(100*65536);
        if (uRand < pPacketQueue->uPacketLoss)
        {
            NetPrintf(("dirtynet: [%p] lost packet (rand=%d, comp=%d)!\n", pPacketQueue, uRand, pPacketQueue->uPacketLoss));
            return(0);
        }
    }

    // return success
    return(pQueueEntry->iPacketSize);
}

/*F********************************************************************************/
/*!
    \Function SocketPacketQueueControl

    \Description
        Control socket packet queue options

    \Input *pPacketQueue    - packet queue control funciton; different selectors control different behaviors
    \Input iControl         - control selector
    \Input iValue           - selector specific

    \Output
        int32_t             - selector result

    \Version 10/07/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketPacketQueueControl(SocketPacketQueueT *pPacketQueue, int32_t iControl, int32_t iValue)
{
    if (iControl == 'pdev')
    {
        NetPrintf(("dirtynet: setting simulated packet deviation=%dms\n", iValue));
        pPacketQueue->uDeviation = iValue;
        return(0);
    }
    if (iControl == 'plat')
    {
        NetPrintf(("dirtynet: setting simulated packet latency=%dms\n", iValue));
        pPacketQueue->uLatency = iValue;
        return(0);
    }
    if (iControl == 'plos')
    {
        NetPrintf(("dirtynet: setting simulated packet loss to %d.%d\n", iValue >> 16, iValue & 0xffff));
        pPacketQueue->uPacketLoss = iValue;
        return(0);
    }
    return(-1);
}

/*
    Rate functions
*/

/*F********************************************************************************/
/*!
    \Function SocketRateUpdate

    \Description
        Update socket data rate estimation

    \Input *pRate           - state used to store rate estimation data
    \Input iData            - amount of data being sent/recv
    \Input *pOpName         - indicates send or recv (for debug use only)

    \Notes
        Rate estimation is based on a rolling 16-deep history of ~100ms samples
        of data rate (the actual period may vary slightly based on update rate
        and tick resolution) covering a total of ~1600-1700ms.  We also keep track
        of the rate we are called at (excluding multiple calls within the same
        tick) so we can estimate how much data we need to have sent by the next
        time we are called.  This is important because we will always be shooting
        lower than our cap if we don't consider this factor, and the amount we
        are shooting lower by increases the slower the update rate.

    \Version 08/20/2014 (jbrookes)
*/
/********************************************************************************F*/
void SocketRateUpdate(SocketRateT *pRate, int32_t iData, const char *pOpName)
{
    const uint32_t uMaxIndex = (unsigned)(sizeof(pRate->aDataHist)/sizeof(pRate->aDataHist[0]));
    uint32_t uCurTick = NetTick(), uOldTick;
    uint32_t uIndex, uCallRate, uCallSum, uDataSum;
    int32_t iTickDiff;

    // reserve tick=0 as uninitialized
    if (uCurTick == 0)
    {
        uCurTick = 1;
    }
    // initialize update tick counters
    if (pRate->uLastTick == 0)
    {
        pRate->uLastTick = NetTickDiff(uCurTick, 2);
        // initialize rate tick counter to current-100 so as to force immediate history update
        pRate->uLastRateTick = NetTickDiff(uCurTick, 100);
        // start off at max rate
        pRate->uCurRate = pRate->uNextRate = pRate->uMaxRate;
    }
    // exclude error results
    if (iData < 0)
    {
        return;
    }

    // update the data
    pRate->aDataHist[pRate->uDataIndex] += iData;
    /* update the call count, only only if time has elapsed since our last call, since we
       want the true update rate) */
    if (NetTickDiff(uCurTick, pRate->uLastTick) > 1)
    {
        pRate->aCallHist[pRate->uDataIndex] += 1;
    }
    // update last update tick
    pRate->uLastTick = uCurTick;
    /* update timestamp, but only if it hasn't been updated already.  we do this so we can
       correctly update the rate calculation continuously with the current sample */
    if (pRate->aTickHist[pRate->uDataIndex] == 0)
    {
        pRate->aTickHist[pRate->uDataIndex] = uCurTick;
    }

    // get oldest tick & sum recorded data & callcounts
    for (uIndex = (pRate->uDataIndex + 1) % uMaxIndex, uOldTick = 0, uCallSum = 0, uDataSum = 0; ; uIndex = (uIndex + 1) % uMaxIndex)
    {
        // skip uninitialized tick values
        if ((uOldTick == 0) && (pRate->aTickHist[uIndex] != 0))
        {
            uOldTick = pRate->aTickHist[uIndex];
        }
        // update call sum
        uCallSum += pRate->aCallHist[uIndex];
        // update data sum
        uDataSum += pRate->aDataHist[uIndex];
        // quit when we've hit every entry
        if (uIndex == pRate->uDataIndex)
        {
            break;
        }
    }

    // update rate estimation
    if ((iTickDiff = NetTickDiff(uCurTick, uOldTick)) > 0)
    {
        // calculate call rate
        uCallRate = (uCallSum > 0) ? iTickDiff/uCallSum : 0;
        // update current rate estimation
        pRate->uCurRate = (uDataSum * 1000) / iTickDiff;
        // update next rate estimation (fudge slightly by 2x as it gives us better tracking to our desired rate)
        pRate->uNextRate = (uDataSum * 1000) / (iTickDiff+(uCallRate*2));

        #if DIRTYNET_RATEDEBUG
        if (pRate->uCurRate != 0)
        {
            NetPrintf(("dirtynet: [%p] rate=%4.2fkb nextrate=%4.2f callrate=%dms tickdiff=%d tick=%d\n", pRate, ((float)pRate->uCurRate)/1024.0f, ((float)pRate->uNextRate)/1024.0f, uCallRate, iTickDiff, uCurTick));
        }
        #endif
    }

    // move to next slot in history every 100ms
    if (NetTickDiff(uCurTick, pRate->uLastRateTick) >= 100)
    {
        pRate->uDataIndex = (pRate->uDataIndex + 1) % uMaxIndex;
        pRate->aDataHist[pRate->uDataIndex] = 0;
        pRate->aTickHist[pRate->uDataIndex] = 0;
        pRate->aCallHist[pRate->uDataIndex] = 0;
        pRate->uLastRateTick = uCurTick;

        #if DIRTYNET_RATEDEBUG
        if (pRate->uCurRate != 0)
        {
            NetPrintf(("dirtynet: [%p] %s=%5d rate=%4.2fkb/s indx=%2d tick=%08x diff=%d\n", pRate, pOpName, uDataSum, ((float)pRate->uCurRate)/1024.0f, pRate->uDataIndex, uCurTick, iTickDiff));
        }
        #endif
    }
}

/*F********************************************************************************/
/*!
    \Function SocketRateThrottle

    \Description
        Throttles data size to send or recv based on calculated data rate and
        configured max rate.

    \Input *pRate           - state used to calculate rate
    \Input iSockType        - socket type (SOCK_DGRAM, SOCK_STREAM, etc)
    \Input iData            - amount of data being sent/recv
    \Input *pOpName         - indicates send or recv (for debug use only)

    \Output
        int32_t             - amount of data to send/recv

    \Version 08/20/2014 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketRateThrottle(SocketRateT *pRate, int32_t iSockType, int32_t iData, const char *pOpName)
{
    int32_t iRateDiff;

    // max rate of zero means rate throttling is disabled
    if ((pRate->uMaxRate == 0) || (iSockType != SOCK_STREAM))
    {
        return(iData);
    }

    // if we're exceeding the max rate, update rate estimation and return no data
    if ((iRateDiff = pRate->uMaxRate - pRate->uNextRate) <= 0)
    {
        NetPrintfVerbose((DIRTYNET_RATEDEBUG, 0, "dirtynet: [%p] exceeding max rate; clamping to zero\n", pRate));
        iData = 0;
    }
    else if (iData > iRateDiff) // return a limited amount of data so as not to exceed max rate
    {
        // clamp in multiples of the typical TCP maximum segment size, so as not to generate fragmented packets
        iData = (iRateDiff / 1460) * 1460;
        NetPrintfVerbose((DIRTYNET_RATEDEBUG, 0, "dirtynet: [%p] exceeding max rate; clamping to %d bytes from %d bytes\n", pRate, iRateDiff, pRate->uMaxRate - pRate->uNextRate));
    }

    // if we are returning no data, update the rate as the caller won't
    if (iData == 0)
    {
        SocketRateUpdate(pRate, 0, pOpName);
    }

    return(iData);
}

/*
    Send Callback functions
*/

/*F********************************************************************************/
/*!
    \Function SocketSendCallbackAdd

    \Description
        Register a new socket send callback

    \Input aCbEntries   - collection of callbacks to add to
    \Input *pCbEntry    - entry to be added

    \Output
        int32_t         - zero=success; negative=failure

    \Version 07/18/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketSendCallbackAdd(SocketSendCallbackEntryT aCbEntries[], SocketSendCallbackEntryT *pCbEntry)
{
    int32_t iRetCode = -1; // default to failure
    int32_t iEntryIndex;

    for(iEntryIndex = 0; iEntryIndex < SOCKET_MAXSENDCALLBACKS; iEntryIndex++)
    {
        if (aCbEntries[iEntryIndex].pSendCallback == NULL)
        {
            aCbEntries[iEntryIndex].pSendCallback = pCbEntry->pSendCallback;
            aCbEntries[iEntryIndex].pSendCallref = pCbEntry->pSendCallref;

            NetPrintf(("dirtynet: adding send callback (%p, %p)\n", pCbEntry->pSendCallback, pCbEntry->pSendCallref));

            iRetCode = 0;  // success

            break;
        }
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function SocketSendCallbackRem

    \Description
        Unregister a socket send callback that was already registered.

    \Input aCbEntries   - collection of callbacks to remove from
    \Input *pCbEntry    - entry to be removed

    \Output
        int32_t         - zero=success; negative=failure

    \Version 07/18/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketSendCallbackRem(SocketSendCallbackEntryT aCbEntries[], SocketSendCallbackEntryT *pCbEntry)
{
    int32_t iRetCode = -1; // default to failure
    int32_t iEntryIndex;

    for(iEntryIndex = 0; iEntryIndex < SOCKET_MAXSENDCALLBACKS; iEntryIndex++)
    {
        if (aCbEntries[iEntryIndex].pSendCallback == pCbEntry->pSendCallback && aCbEntries[iEntryIndex].pSendCallref == pCbEntry->pSendCallref)
        {
            NetPrintf(("dirtynet: removing send callback (%p, %p)\n", aCbEntries[iEntryIndex].pSendCallback,  aCbEntries[iEntryIndex].pSendCallref));

            aCbEntries[iEntryIndex].pSendCallback = NULL;
            aCbEntries[iEntryIndex].pSendCallref = NULL;

            iRetCode = 0;  // success

            break;
        }
    }

    return(iRetCode);
}

/*F********************************************************************************/
/*!
    \Function SocketSendCallbackInvoke

    \Description
        Invoke all send callbacks in specified collection. Only one of the callback
        is supposed to return "handled"... warn if not the case.

    \Input aCbEntries   - collection of callbacks to be invoked
    \Input pSocket      - socket reference
    \Input iType        - socket type
    \Input *pBuf        - the data to be sent
    \Input iLen         - size of data
    \Input *pTo         - the address to send to (NULL=use connection address)

    \Output
        int32_t         - 0 = send not handled; >0 = send successfully handled (bytes sent); <0 = send handled but failed (SOCKERR_XXX)

    \Version 07/18/2014 (mclouatre)
*/
/********************************************************************************F*/
int32_t SocketSendCallbackInvoke(SocketSendCallbackEntryT aCbEntries[], SocketT *pSocket, int32_t iType, const char *pBuf, int32_t iLen, const struct sockaddr *pTo)
{
    int32_t iRetCode = 0;  // default to send not handled
    int32_t iResult;
    int32_t iEntryIndex;

    for (iEntryIndex = 0; iEntryIndex < SOCKET_MAXSENDCALLBACKS; iEntryIndex++)
    {
        // we expect zero or one send callback to handle the send, more than one indicates an invalid condition
        if (aCbEntries[iEntryIndex].pSendCallback != NULL)
        {
            if((iResult = aCbEntries[iEntryIndex].pSendCallback(pSocket, iType, (const uint8_t *)pBuf, iLen, pTo, aCbEntries[iEntryIndex].pSendCallref)) != 0)
            {
                if (iRetCode == 0)
                {
                    iRetCode = iResult;
                }
                else
                {
                    NetPrintf(("dirtynet: critical error - send handled by more than one callback  (iEntryIndex = %d, iResult = %d)\n", iEntryIndex, iResult));
                }
            }
        }
    }

    return(iRetCode);
}
