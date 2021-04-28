/*H*************************************************************************************/
/*!
    \File dirtynetxboxone.c

    \Description
        Provide a wrapper that translates the Winsock network interface
        into DirtySock calls. In the case of Winsock, little translation
        is needed since it is based off BSD sockets (as is DirtySock).

    \Copyright
        Copyright (c) Electronic Arts 1999-2013.

    \Version 1.0 03/22/2013 (jbrookes) Branched from dirtynetwin
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1           // avoid extra stuff
#endif

#pragma warning(push,0)
#include <windows.h>
#pragma warning(pop)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtyerr.h"
#include "DirtySDK/dirtyvers.h"

#include "dirtynetpriv.h"

/*** Defines ***************************************************************************/

#define SOCKET_MAXEVENTS        (64)

/*** Macros ****************************************************************************/

//$$TODO - IPv6 stuff needs to be cleaned up (move to dirtynet.h?)
#define SockaddrIn6GetAddr4(addr)     (((((((uint8_t)((addr)->sin6_addr.s6_addr[12])<<8)|(uint8_t)((addr)->sin6_addr.s6_addr[13]))<<8)|(uint8_t)((addr)->sin6_addr.s6_addr[14]))<<8)|(uint8_t)((addr)->sin6_addr.s6_addr[15]))

/*** Type Definitions ******************************************************************/

//! dirtysock connection socket structure
struct SocketT
{
    SocketT *pNext;             //!< link to next active
    SocketT *pKill;             //!< link to next killed socket

    SOCKET uSocket;             //!< winsock socket ref

    int32_t iFamily;            //!< protocol family
    int32_t iType;              //!< protocol type
    int32_t iProto;             //!< protocol ident

    int8_t iOpened;             //!< negative=error, zero=not open (connecting), positive=open
    uint8_t uImported;          //!< whether socket was imported or not
    uint8_t bVirtual;           //!< if true, socket is virtual
    uint8_t uShutdown;          //!< shutdown flag
    int32_t iLastError;         //!< last socket error

    struct sockaddr LocalAddr;  //!< local address
    struct sockaddr RemoteAddr; //!< remote address

    uint16_t uVirtualPort;      //!< virtual port, if set
    uint8_t bAsyncRecv;         //!< TRUE if async recv is enabled
    uint8_t bSendCbs;           //!< TRUE if send cbs are enabled, false otherwise

    SocketRateT SendRate;       //!< send rate estimation data
    SocketRateT RecvRate;       //!< recv rate estimation data

    uint32_t uCallMask;         //!< valid callback events
    uint32_t uCallLast;         //!< last callback tick
    uint32_t uCallIdle;         //!< ticks between idle calls
    void *pCallRef;             //!< reference calback value
    int32_t (*pCallback)(SocketT *pSocket, int32_t iFlags, void *pRef);

    #if SOCKET_ASYNCRECVTHREAD
    WSAOVERLAPPED Overlapped;   //!< overlapped i/o structure
    NetCritT RecvCrit;          //!< receive critical section
    int32_t iRecvErr;           //!< last error that occurred
    int32_t iAddrLen;           //!< storage for async address length write by WSARecv
    uint32_t uRecvFlag;         //!< flags from recv operation
    int32_t iRecvStat;          //!< how many queued receive bytes
    uint8_t bRecvInp;           //!< if true, a receive operation is in progress
    uint8_t _pad2[3];
    #endif

    struct sockaddr RecvAddr;   //!< receive address
    struct sockaddr_in6 RecvAddr6;   //!< receive address (ipv6)

    SocketPacketQueueT *pRecvQueue;
    SocketPacketQueueEntryT *pRecvPacket;
};

typedef struct SocketAddrMapEntryT
{
    int32_t iRefCount;
    int32_t iVirtualAddress;
    struct sockaddr_in6 SockAddr6;
} SocketAddrMapEntryT;

typedef struct SocketAddrMapT
{
    int32_t iNumEntries;
    int32_t iNextVirtAddr;
    SocketAddrMapEntryT *pMapEntries; //!< variable-length array
} SocketAddrMapT;

//! local state
typedef struct SocketStateT
{
    SocketT *pSockList;                 //!< master socket list
    SocketT *pSockKill;                 //!< list of killed sockets
    HostentT *pHostList;                //!< list of ongoing name resolution operations

    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list
    int32_t iMaxPacket;                 //!< maximum packet size

    // module memory group
    int32_t iMemGroup;                  //!< module mem group id
    void *pMemGroupUserData;            //!< user data associated with mem group

    int32_t iVersion;                   //!< winsock version

    #if SOCKET_ASYNCRECVTHREAD
    volatile HANDLE hRecvThread;        //!< receive thread handle
    volatile int32_t iRecvLife;         //!< receive thread alive indicator
    WSAEVENT hEvent;                    //!< event used to wake up receive thread
    NetCritT EventCrit;                 //!< event critical section (used when killing events)
    #endif

    SocketAddrMapT AddrMap;             //!< address map for translating ipv6 addresses to ipv4 virtual addresses and back

    SocketHostnameCacheT *pHostnameCache; //!< hostname cache

    SocketSendCallbackEntryT aSendCbEntries[SOCKET_MAXSENDCALLBACKS]; //!< collection of send callbacks

    uint32_t uLocalAddr;                //!< local ipv4 address
    uint32_t uRandBindPort;
    int8_t iVerbose;                    //!< debug output verbosity
    uint8_t _pad[3];
} SocketStateT;

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! module state ref
static SocketStateT *_Socket_pState = NULL;

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************/
/*!
    \Function _XlatError0

    \Description
        Translate a winsock error to dirtysock

    \Input iErr     - return value from winsock call
    \Input iWSAErr  - winsock error (from WSAGetLastError())

    \Output
        int32_t     - dirtysock error

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _XlatError0(int32_t iErr, int32_t iWSAErr)
{
    if (iErr < 0)
    {
        iErr = iWSAErr;
        if ((iErr == WSAEWOULDBLOCK) || (iErr == WSA_IO_PENDING))
            iErr = SOCKERR_NONE;
        else if ((iErr == WSAENETUNREACH) || (iErr == WSAEHOSTUNREACH))
            iErr = SOCKERR_UNREACH;
        else if (iErr == WSAENOTCONN)
            iErr = SOCKERR_NOTCONN;
        else if (iErr == WSAECONNREFUSED)
            iErr = SOCKERR_REFUSED;
        else if (iErr == WSAEINVAL)
            iErr = SOCKERR_INVALID;
        else if (iErr == WSAECONNRESET)
            iErr = SOCKERR_CONNRESET;
        else
        {
            NetPrintf(("dirtynetxboxone: error %s\n", DirtyErrGetName(iErr)));
            iErr = SOCKERR_OTHER;
        }
    }
    return(iErr);
}

/*F*************************************************************************************/
/*!
    \Function _XlatError

    \Description
        Translate the most recent winsock error to dirtysock

    \Input iErr     - return value from winsock call

    \Output
        int32_t     - dirtysock error

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _XlatError(int32_t iErr)
{
    return(_XlatError0(iErr, WSAGetLastError()));
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapAlloc

    \Description
        Alloc (or re-alloc) address map entry list

    \Input *pState      - module state
    \Input iNumEntries  - new entry list size

    \Output
        int32_t         - negative=error, zero=success

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t _SocketAddrMapAlloc(SocketStateT *pState, int32_t iNumEntries)
{
    int32_t iOldEntryListSize = 0, iNewEntryListSize = iNumEntries*sizeof(SocketAddrMapEntryT);
    SocketAddrMapEntryT *pNewEntries;

    // allocate new map memory
    if ((pNewEntries = (SocketAddrMapEntryT *)DirtyMemAlloc(iNewEntryListSize, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        return(-1);
    }
    // clear new map memory
    memset(pNewEntries, 0, iNewEntryListSize);
    // copy previous map data
    if (pState->AddrMap.pMapEntries != NULL)
    {
        iOldEntryListSize = pState->AddrMap.iNumEntries*sizeof(SocketAddrMapEntryT);
        ds_memcpy(pNewEntries, pState->AddrMap.pMapEntries, iOldEntryListSize);
        DirtyMemFree(pState->AddrMap.pMapEntries, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
    // update state
    pState->AddrMap.iNumEntries = iNumEntries;
    pState->AddrMap.pMapEntries = pNewEntries;
    // return success
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapGet

    \Description
        Find a map entry, based on specified IPv6 address

    \Input *pState      - module state
    \Input *pAddr       - address to get entry for
    \Input iAddrSize    - size of address

    \Output
        SockAddrMapEntryT * - pointer to map entry or NULL if not found

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static SocketAddrMapEntryT *_SocketAddrMapGet(SocketStateT *pState, const struct sockaddr *pAddr, int32_t iAddrSize)
{
    const struct sockaddr_in6 *pAddr6 = (const struct sockaddr_in6 *)pAddr;
    SocketAddrMapEntryT *pMapEntry;
    int32_t iMapEntry;

    for (iMapEntry = 0; iMapEntry < pState->AddrMap.iNumEntries; iMapEntry += 1)
    {
        pMapEntry = &pState->AddrMap.pMapEntries[iMapEntry];
        if ((pAddr->sa_family == AF_INET) && (pMapEntry->iVirtualAddress == SockaddrInGetAddr(pAddr)))
        {
            return(pMapEntry);
        }
        if ((pAddr->sa_family == AF_INET6) && (!memcmp(&pAddr6->sin6_addr, &pMapEntry->SockAddr6.sin6_addr, sizeof(pAddr6->sin6_addr))))
        {
            return(pMapEntry);
        }
    }
    return(NULL);
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapSet

    \Description
        Initialize a map entry

    \Input *pState      - module state
    \Input *pMapEntry   - entry to set
    \Input *pAddr6      - IPv6 address to set
    \Input iAddrSize    - size of address

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketAddrMapSet(SocketStateT *pState, SocketAddrMapEntryT *pMapEntry, const struct sockaddr_in6 *pAddr6, int32_t iAddrSize)
{
    pMapEntry->iRefCount = 1;
    pMapEntry->iVirtualAddress = pState->AddrMap.iNextVirtAddr;
    pState->AddrMap.iNextVirtAddr = (pState->AddrMap.iNextVirtAddr + 1) & 0x00ffffff;
    ds_memcpy(&pMapEntry->SockAddr6, pAddr6, iAddrSize);
    NetPrintfVerbose((pState->iVerbose, 1, "dirtynetxboxone: [%d] add map %A to %a\n", pMapEntry - pState->AddrMap.pMapEntries, pAddr6, pMapEntry->iVirtualAddress));
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapDel

    \Description
        Dereference (and clear, if no more references) a map entry

    \Input *pState      - module state
    \Input *pMapEntry   - entry to del

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketAddrMapDel(SocketStateT *pState, SocketAddrMapEntryT *pMapEntry)
{
    NetPrintfVerbose((pState->iVerbose, 1, "dirtynetxboxone: [%d] del map %A to %a (decremented refcount to %d)\n",
        pMapEntry - pState->AddrMap.pMapEntries, &pMapEntry->SockAddr6, pMapEntry->iVirtualAddress, pMapEntry->iRefCount-1));
    if (--pMapEntry->iRefCount == 0)
    {
        memset(pMapEntry, 0, sizeof(*pMapEntry));
    }
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapAdd

    \Description
        Add an IPv6 address to the address mapping table

    \Input *pState      - module state
    \Input *pAddr6      - address to add to the mapping table
    \Input iAddrSize    - size of address

    \Output
        int32_t         - negative=error, else virtual IPv4 address for newly mapped IPv6 address

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t _SocketAddrMapAdd(SocketStateT *pState, const struct sockaddr_in6 *pAddr6, int32_t iAddrSize)
{
    int32_t iMapEntry;

    // find an empty slot
    for (iMapEntry = 0; iMapEntry < pState->AddrMap.iNumEntries; iMapEntry += 1)
    {
        if (pState->AddrMap.pMapEntries[iMapEntry].iVirtualAddress == 0)
        {
            _SocketAddrMapSet(pState, &pState->AddrMap.pMapEntries[iMapEntry], pAddr6, iAddrSize);
            return(pState->AddrMap.pMapEntries[iMapEntry].iVirtualAddress);
        }
    }

    // if no empty slot, realloc the array
    if ((_SocketAddrMapAlloc(pState, pState->AddrMap.iNumEntries+8)) < 0)
    {
        return(-1);
    }
    // try the add again
    return(_SocketAddrMapAdd(pState, pAddr6, iAddrSize));
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrMapAddress

    \Description
        Map an IPv6 address and return a virtual IPv4 address that can be used to
        reference it.

    \Input *pState      - module state
    \Input *pAddr       - address to add to the mapping table
    \Input iAddrSize    - size of address

    \Output
        int32_t         - negative=error, else virtual IPv4 address for newly mapped IPv6 address

    \Version 04/17/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketAddrMapAddress(SocketStateT *pState, const struct sockaddr *pAddr, int32_t iAddrSize)
{
    SocketAddrMapEntryT *pMapEntry;

    // we only map ipv6 addresses
    if ((pAddr->sa_family != AF_INET6) || (iAddrSize < sizeof(struct sockaddr_in6)))
    {
        NetPrintf(("dirtynetxboxone: can't map address of type %d size=%d\n", pAddr->sa_family, iAddrSize));
        return(-1);
    }
    iAddrSize = sizeof(struct sockaddr_in6);
    // see if this address is already mapped
    if ((pMapEntry = _SocketAddrMapGet(pState, (const struct sockaddr *)pAddr, iAddrSize)) != NULL)
    {
        pMapEntry->iRefCount += 1;
        NetPrintfVerbose((pState->iVerbose, 1, "dirtynetxboxone: [%d] map %A to %a (incremented refcount to %d)\n", pMapEntry - pState->AddrMap.pMapEntries, &pMapEntry->SockAddr6, pMapEntry->iVirtualAddress, pMapEntry->iRefCount));
        return(pMapEntry->iVirtualAddress);
    }
    // add it to the map
    return(_SocketAddrMapAdd(pState, (const struct sockaddr_in6 *)pAddr, iAddrSize));
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrUnmapAddress

    \Description
        Removes an addres mapping from the mapping table

    \Input *pState      - module state
    \Input *pAddr       - address to remove from the mapping table
    \Input iAddrSize    - size of address

    \Output
        int32_t         - negative=error, else virtual IPv4 address for newly mapped IPv6 address

    \Version 04/18/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketAddrUnmapAddress(SocketStateT *pState, const struct sockaddr *pAddr, int32_t iAddrSize)
{
    SocketAddrMapEntryT *pMapEntry;

    // we only map ipv6 addresses
    if ((pAddr->sa_family != AF_INET6) || (iAddrSize < sizeof(struct sockaddr_in6)))
    {
        NetPrintf(("dirtynetxboxone: can't unmap address of type %d size=%d\n", pAddr->sa_family, iAddrSize));
        return(-1);
    }
    iAddrSize = sizeof(struct sockaddr_in6);
    // get the map entry for this address
    if ((pMapEntry = _SocketAddrMapGet(pState, (const struct sockaddr *)pAddr, iAddrSize)) == NULL)
    {
        NetPrintf(("dirtynetxboxone: address unmap operation on an address not in the table\n"));
        return(-2);
    }
    // unmap it
    _SocketAddrMapDel(pState, pMapEntry);
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _SocketAddrTranslate

    \Description
        Translate the following:
        - IPv4 real address to IPV4-mapped IPv6 address
        - IPv4 virtual address to IPv6 address
        - IPv6 address to IPv4 address

    \Input *pState      - module state
    \Input *pResult     - [out] storage for translated result
    \Input *pSource     - source address
    \Input *pNameLen    - [out] storage for result length

    \Output
        struct sockaddr * - pointer to resulting IPv6 address

    \Version 04/15/2013 (jbrookes)
*/
/************************************************************************************F*/
static struct sockaddr *_SocketAddrTranslate(SocketStateT *pState, struct sockaddr *pResult, const struct sockaddr *pSource, int32_t *pNameLen)
{
    SocketAddrMapEntryT *pMapEntry;
    struct sockaddr *pReturn = NULL;

    if (pSource->sa_family == AF_INET)
    {
        struct sockaddr_in6 *pResult6 = (struct sockaddr_in6 *)pResult;
        uint32_t uAddr = SockaddrInGetAddr(pSource);
        // if this is a physical address, create an IPv4-mapped IPv6 address
        if ((uAddr == 0) || ((uAddr >> 24) != 0))
        {
            IN_ADDR Inv4Addr;
            memset(pResult, 0, sizeof(*pResult));
            Inv4Addr = *(IN_ADDR *)INETADDR_ADDRESS(pSource);
            IN6ADDR_SETV4MAPPED(pResult6, &Inv4Addr, INETADDR_SCOPE_ID(pSource), INETADDR_PORT(pSource));
            *pNameLen = sizeof(*pResult6);
            pReturn = pResult;
        }
        // get IPv6 address from virtual IPv
        else if ((pMapEntry = _SocketAddrMapGet(pState, pSource, *pNameLen)) != NULL)
        {
            ds_memcpy_s(pResult6, sizeof(*pResult6), &pMapEntry->SockAddr6, sizeof(pMapEntry->SockAddr6));
            pResult6->sin6_port = INETADDR_PORT(pSource);
            *pNameLen = sizeof(*pResult6);
            pReturn = pResult;
        }
    }
    else if (pSource->sa_family == AF_INET6)
    {
        struct sockaddr_in6 *pSource6 = (struct sockaddr_in6 *)pSource;
        // translate IPv6 to virtual IPv4 address
        if ((pMapEntry = _SocketAddrMapGet(pState, pSource, *pNameLen)) != NULL)
        {
            struct sockaddr *pResult4 = (struct sockaddr *)pResult;
            SockaddrInit(pResult4, AF_INET);
            SockaddrInSetAddr(pResult4, pMapEntry->iVirtualAddress);
            SockaddrInSetPort(pResult4, SocketHtons(pSource6->sin6_port));
            *pNameLen = sizeof(*pResult4);
            pReturn = pResult;
        }
        // do we have an IPv4-mapped IPv6 address?
        else if (!memcmp(pSource6->sin6_addr.u.Byte, in6addr_v4mappedprefix.u.Byte, 12))
        {
            struct sockaddr *pResult4 = (struct sockaddr *)pResult;
            uint32_t uAddress = SockaddrIn6GetAddr4(pSource6);
            SockaddrInit(pResult4, AF_INET);
            SockaddrInSetAddr(pResult4, uAddress);
            SockaddrInSetPort(pResult4, SocketHtons(pSource6->sin6_port));
            *pNameLen = sizeof(*pResult4);
            pReturn = pResult;
        }
    }
    if (pReturn != NULL)
    {
        NetPrintfVerbose((pState->iVerbose, 1, "dirtynetxboxone: map translate %A->%A\n", pSource, pReturn));
    }
    else
    {
        NetPrintf(("dirtynetxboxone: map translate %A failed\n", pSource));
    }
    return(pReturn);
}

/*F*************************************************************************************/
/*!
    \Function _SocketOpen

    \Description
        Allocates a SocketT.  If s is INVALID_SOCKET, a WinSock socket ref is created,
        otherwise s is used.

    \Input uSocket  - socket to use, or INVALID_SOCKET
    \Input iFamily  - address family
    \Input iType    - type (SOCK_DGRAM, SOCK_STREAM, ...)
    \Input iProto   - protocol

    \Output
        SocketT *   - pointer to new socket, or NULL

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static SocketT *_SocketOpen(SOCKET uSocket, int32_t iFamily, int32_t iType, int32_t iProto)
{
    SocketStateT *pState = _Socket_pState;
    uint32_t uTrue = 1, uFalse = 0;
    SocketT *pSocket;

    // allocate memory
    if ((pSocket = (SocketT *)DirtyMemAlloc(sizeof(*pSocket), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetxboxone: unable to allocate memory for socket\n"));
        return(NULL);
    }
    memset(pSocket, 0, sizeof(*pSocket));

    // open a winsock socket (force to AF_INET6 if we are opening it)
    if ((uSocket == INVALID_SOCKET) && ((uSocket = WSASocket(iFamily = AF_INET6, iType, iProto, NULL, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET))
    {
        NetPrintf(("dirtynetxboxone: error %d creating socket\n", WSAGetLastError()));
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // create one-deep packet queue
    if ((pSocket->pRecvQueue = SocketPacketQueueCreate(1, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetxboxone: failed to create socket queue for socket\n"));
        closesocket(uSocket);
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // save the socket
    pSocket->uSocket = uSocket;
    pSocket->iLastError = SOCKERR_NONE;

    // set to non blocking
    ioctlsocket(uSocket, FIONBIO, (u_long *)&uTrue);
    // disable IPV6 only
    setsockopt(uSocket, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&uFalse, sizeof(uFalse));
    // if udp, allow broadcast
    if (iType == SOCK_DGRAM)
    {
        setsockopt(uSocket, SOL_SOCKET, SO_BROADCAST, (char *)&uTrue, sizeof(uTrue));
    }
    // if raw, set hdrincl
    if (iType == SOCK_RAW)
    {
        setsockopt(uSocket, IPPROTO_IP, IP_HDRINCL, (char *)&uTrue, sizeof(uTrue));
    }

    // set family/proto info
    pSocket->iFamily = iFamily;
    pSocket->iType = iType;
    pSocket->iProto = iProto;
    pSocket->bAsyncRecv = ((iType == SOCK_DGRAM) || (iType == SOCK_RAW)) ? TRUE : FALSE;
    pSocket->bSendCbs = TRUE;

    #if SOCKET_ASYNCRECVTHREAD
    // create overlapped i/o event object
    memset(&pSocket->Overlapped, 0, sizeof(pSocket->Overlapped));
    pSocket->Overlapped.hEvent = WSACreateEvent();

    // inititalize receive critical section
    NetCritInit(&pSocket->RecvCrit, "recvthread");
    #endif

    // install into list
    NetCritEnter(NULL);
    pSocket->pNext = pState->pSockList;
    pState->pSockList = pSocket;
    NetCritLeave(NULL);

    // return the socket
    return(pSocket);
}

/*F*************************************************************************************/
/*!
    \Function _SocketClose

    \Description
        Disposes of a SocketT, including removal from the global socket list and
        disposal of the SocketT allocated memory.  Does NOT dispose of the winsock
        socket ref.

    \Input *pSocket     - socket to close
    \Input bShutdown    - if TRUE, shutdown and close socket ref

    \Output
        int32_t         - negative=failure, zero=success

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketClose(SocketT *pSocket, uint32_t bShutdown)
{
    SocketStateT *pState = _Socket_pState;
    uint8_t bSockInList = FALSE;
    SocketT **ppSocket;

    // for access to socket list
    NetCritEnter(NULL);

    // remove sock from linked list
    for (ppSocket = &pState->pSockList; *ppSocket != NULL; ppSocket = &(*ppSocket)->pNext)
    {
        if (*ppSocket == pSocket)
        {
            *ppSocket = pSocket->pNext;
            bSockInList = TRUE;
            break;
        }
    }

    // release before NetIdleDone
    NetCritLeave(NULL);

    // make sure the socket is in the socket list (and therefore valid)
    if (bSockInList == FALSE)
    {
        NetPrintf(("dirtynetxboxone: warning, trying to close socket 0x%08x that is not in the socket list\n", (uintptr_t)pSocket));
        return(-1);
    }

    // finish any idle call
    NetIdleDone();

    #if SOCKET_ASYNCRECVTHREAD
    // acquire global critical section
    NetCritEnter(NULL);

    // wake up out of WaitForMultipleEvents()
    WSASetEvent(pState->hEvent);

    // acquire event critical section
    NetCritEnter(&pState->EventCrit);

    // close event
    WSACloseEvent(pSocket->Overlapped.hEvent);
    pSocket->Overlapped.hEvent = WSA_INVALID_EVENT;

    // release event critical section
    NetCritLeave(&pState->EventCrit);

    // release global critical section
    NetCritLeave(NULL);
    #endif

    // destroy packet queue
    if (pSocket->pRecvQueue != NULL)
    {
        SocketPacketQueueDestroy(pSocket->pRecvQueue);
    }

    // mark as closed
    if ((bShutdown == TRUE) && (pSocket->uSocket != INVALID_SOCKET))
    {
        // close winsock socket
        shutdown(pSocket->uSocket, 2);
        closesocket(pSocket->uSocket);
    }
    pSocket->uSocket = INVALID_SOCKET;
    pSocket->iOpened = 0;

    /* Put into killed list:
       Usage of a kill list allows for postponing two things
        * destruction of RecvCrit
        * release of socket data structure memory
      This ensures that RecvCrit is not freed while in-use by a running thread.
      Such a scenario can occur when the receive callback invoked by _SocketRecvThread()
      (while RecvCrit is entered) calls _SocketClose() */
    NetCritEnter(NULL);
    pSocket->pKill = pState->pSockKill;
    pState->pSockKill = pSocket;
    NetCritLeave(NULL);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _SocketIdle

    \Description
        Call idle processing code to give connections time.

    \Input *_pState  - module state

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketIdle(void *_pState)
{
    SocketStateT *pState = (SocketStateT *)_pState;
    SocketT *pSocket;
    uint32_t uTick = NetTick();

    // for access to socket list and kill list
    NetCritEnter(NULL);

    // walk socket list and perform any callbacks
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
    {
        // see if we should do callback
        if ((pSocket->uCallIdle != 0) && (pSocket->pCallback != NULL) && (pSocket->uCallLast != (unsigned)-1) && (NetTickDiff(uTick, pSocket->uCallLast) > pSocket->uCallIdle))
        {
            pSocket->uCallLast = (unsigned)-1;
            pSocket->pCallback(pSocket, 0, pSocket->pCallRef);
            pSocket->uCallLast = uTick = NetTick();
        }
    }

    // delete any killed sockets
    while ((pSocket = pState->pSockKill) != NULL)
    {
        pState->pSockKill = pSocket->pKill;

        // release the socket's receive critical section
        #if SOCKET_ASYNCRECVTHREAD
        NetCritKill(&pSocket->RecvCrit);
        #endif

        // free the socket memory
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    // process hostname list, delete completed lookup requests
    SocketHostnameListProcess(&pState->pHostList, pState->iMemGroup, pState->pMemGroupUserData);

    // release access to socket list and kill list
    NetCritLeave(NULL);
}

#if SOCKET_ASYNCRECVTHREAD
/*F*************************************************************************************/
/*!
    \Function _SocketRecvData

    \Description
        Called when data is received by _SocketRecvThread().  If there is a callback
        registered than the socket is passed to that callback so the data may be
        consumed.

    \Input *pState  - module state
    \Input *pSocket - pointer to socket that has new data

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvData(SocketStateT *pState, SocketT *pSocket)
{
    // translate IPv6->IPv4, save receive timestamp
    if (pSocket->iType != SOCK_STREAM)
    {
        int32_t iNameLen = sizeof(pSocket->RecvAddr);
        _SocketAddrTranslate(pState, &pSocket->RecvAddr, (struct sockaddr *)&pSocket->RecvAddr6, &iNameLen);
        SockaddrInSetMisc(&pSocket->RecvAddr, NetTick());
    }

    // set up packet queue entry (already has data)
    pSocket->pRecvPacket->iPacketSize = pSocket->iRecvStat;
    ds_memcpy_s(&pSocket->pRecvPacket->PacketAddr, sizeof(pSocket->pRecvPacket->PacketAddr), &pSocket->RecvAddr, sizeof(pSocket->RecvAddr));

    // see if we should issue callback
    if ((pSocket->uCallLast != (unsigned)-1) && (pSocket->pCallback != NULL) && (pSocket->uCallMask & CALLB_RECV))
    {
        pSocket->uCallLast = (unsigned)-1;
        (pSocket->pCallback)(pSocket, 0, pSocket->pCallRef);
        pSocket->uCallLast = NetTick();
    }
}

/*F*************************************************************************************/
/*!
    \Function _SocketRead

    \Description
        Issue an overlapped read call on the given socket.

    \Input *pState  - module state
    \Input *pSocket - pointer to socket to read from

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketRead(SocketStateT *pState, SocketT *pSocket)
{
    int32_t iResult, iRecvStat = 0;
    WSABUF RecvBuf;

    // if we already have data or a read is already in progress, don't issue another
    if ((pSocket->iRecvStat != 0) || (pSocket->bRecvInp == TRUE) || (pSocket->bVirtual == TRUE))
    {
        return(0);
    }

    // get a packet queue entry to receive into
    pSocket->pRecvPacket = SocketPacketQueueAlloc(pSocket->pRecvQueue);

    // set up for recv call
    RecvBuf.buf = (CHAR *)pSocket->pRecvPacket->aPacketData;
    RecvBuf.len = sizeof(pSocket->pRecvPacket->aPacketData);
    pSocket->uRecvFlag = 0;

    // try and receive some data
    if (pSocket->iType == SOCK_DGRAM)
    {
        pSocket->iAddrLen = sizeof(pSocket->RecvAddr6);
        iResult = WSARecvFrom(pSocket->uSocket, &RecvBuf, 1, (LPDWORD)&pSocket->iRecvStat, (LPDWORD)&pSocket->uRecvFlag, (struct sockaddr *)&pSocket->RecvAddr6, (LPINT)&pSocket->iAddrLen, &pSocket->Overlapped, NULL);
    }
    else
    {
        iResult = WSARecv(pSocket->uSocket, &RecvBuf, 1, (LPDWORD)&pSocket->iRecvStat, (LPDWORD)&pSocket->uRecvFlag, &pSocket->Overlapped, NULL);
    }

    // error?
    if (iResult != 0)
    {
        // save error result and winsock error
        pSocket->iRecvStat = iResult;
        if ((pSocket->iRecvErr = WSAGetLastError()) == WSA_IO_PENDING)
        {
            // remember a receive operation is pending
            pSocket->bRecvInp = TRUE;
        }
        else
        {
            NetPrintf(("dirtynetxboxone: [%p] error %d trying to receive from socket\n", pSocket->uSocket, pSocket->iRecvErr));
        }
    }
    else if ((iRecvStat = pSocket->iRecvStat) > 0)
    {
        // if the read completed immediately, forward data to socket callback
        _SocketRecvData(pState, pSocket);
    }

    return(iRecvStat);
}

/*F*************************************************************************************/
/*!
    \Function _SocketRecvThread

    \Description
        Wait for incoming data and deliver it immediately to the registered socket callback,
        if any.

    \Input  uUnused - unused

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvThread(uint32_t uUnused)
{
    SocketStateT *pState = _Socket_pState;
    WSAEVENT aEventList[SOCKET_MAXEVENTS];
    int32_t iNumEvents, iResult;
    SocketT *pSocket;

    // show we are alive
    NetPrintf(("dirtynetxboxone: receive thread started (thid=%d)\n", GetCurrentThreadId()));

    // clear event list
    memset(aEventList, 0, sizeof(aEventList));

    // loop until done
    while (pState->hRecvThread != 0)
    {
        // acquire global critical section for access to g_socklist
        NetCritEnter(NULL);

        // add global event to list
        iNumEvents = 0;
        aEventList[iNumEvents++] = pState->hEvent;

        // walk the socket list and do two things:
        //    1- if the socket is ready for reading, perform the read operation
        //    2- if the buffer in which inbound data is saved is empty, initiate a new low-level read operation for that socket
        for (pSocket = pState->pSockList; (pSocket != NULL) && (iNumEvents < SOCKET_MAXEVENTS); pSocket = pSocket->pNext)
        {
            // only handle non-virtual sockets with asyncrecv true
            if ((pSocket->bVirtual == FALSE) && (pSocket->uSocket != INVALID_SOCKET) && (pSocket->bAsyncRecv == TRUE))
            {
                // acquire socket critical section
                NetCritEnter(&pSocket->RecvCrit);

                // if no data is queued, check for overlapped read completion
                if (pSocket->iRecvStat <= 0)
                {
                    // is a read in progress on this socket?
                    if (pSocket->bRecvInp == TRUE)
                    {
                        // check for overlapped read completion
                        if (WSAGetOverlappedResult(pSocket->uSocket, &pSocket->Overlapped, (LPDWORD)&pSocket->iRecvStat, FALSE, (LPDWORD)&pSocket->uRecvFlag) == TRUE)
                        {
                            // mark that receive operation is no longer in progress
                            pSocket->bRecvInp = FALSE;

                            // we've got data waiting on this socket
                            _SocketRecvData(pState, pSocket);
                        }
                        else if ((iResult = WSAGetLastError()) != WSA_IO_INCOMPLETE)
                        {
                            #if DIRTYCODE_LOGGING
                            if (iResult != WSAECONNRESET)
                            {
                                NetPrintf(("dirtynetxboxone: WSAGetOverlappedResult error %d\n", iResult));
                            }
                            #endif

                            // since receive operation failed, mark as not in progress
                            pSocket->bRecvInp = FALSE;
                        }
                    }

                    /*
                    Before proceeding, make sure that the user callback invoked in _SockRecvData() did not close the socket with SocketClose().
                    We know that SocketClose() function resets the value of pSocket->uSocket to INVALID_SOCKET. Also, we know that it
                    does not destroy pSocket but it queues it in the global kill list. Since this list cannot be processed before the code
                    below as it also runs in the context of the global critical section, the following code is thread-safe.
                    */
                    if (pSocket->uSocket != INVALID_SOCKET)
                    {
                        // try and read some data
                        _SocketRead(pState, pSocket);

                        // Before proceeding, make sure that user callback invoked in _SockRead() did not close the socket.
                        // Same rational as previous comment.
                        if (pSocket->uSocket != INVALID_SOCKET)
                        {
                            // add event to event list array
                            aEventList[iNumEvents++] = pSocket->Overlapped.hEvent;
                        }
                    }
                }

                // release socket critical section
                NetCritLeave(&pSocket->RecvCrit);
            }
        }

        // protect against events being deleted
        NetCritEnter(&pState->EventCrit);

        // release global critical section
        NetCritLeave(NULL);

        // wait for an event to trigger
        iResult = WSAWaitForMultipleEvents(iNumEvents, aEventList, FALSE, WSA_INFINITE, FALSE) - WSA_WAIT_EVENT_0;

        // reset the signaled event
        if ((iResult >= 0) && (iResult < iNumEvents))
        {
            WSAResetEvent(aEventList[iResult]);
        }

        // leave event protected section
        NetCritLeave(&pState->EventCrit);
    }

    // indicate we are done
    NetPrintf(("dirtynetxboxone: receive thread exit\n"));
    pState->iRecvLife = 0;
}
#endif // SOCKET_ASYNCRECVTHREAD


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function SocketCreate

    \Description
        Create new instance of socket interface module.  Initializes all global
        resources and makes module ready for use.

    \Input iThreadPrio        - priority to start threads with
    \Input iThreadStackSize   - stack size to start threads with (in bytes)
    \Input iThreadCpuAffinity - cpu affinity to start threads with

    \Output
        int32_t               - negative=error, zero=success

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    SocketStateT *pState = _Socket_pState;
    WSADATA WSAData;
    int32_t iResult;
    #if SOCKET_ASYNCRECVTHREAD
    int32_t iPid;
    #endif
    int32_t iMemGroup;
    void *pMemGroupUserData;
#if SOCKET_ASYNCRECVTHREAD
    uintptr_t uTempHandle = 1;
#endif

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintf(("dirtynetxboxone: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetxboxone: DirtySDK v%d.%d.%d.%d.%d\n", DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH));

    // alloc and init state ref
    if ((pState = (SocketStateT *)DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetxboxone: unable to allocate module state\n"));
        return(-2);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iMaxPacket = SOCKET_MAXUDPRECV;
    pState->iVerbose = 1;

    // save global module ref
    _Socket_pState = pState;

    // startup network libs
    NetLibCreate(iThreadPrio, iThreadStackSize, iThreadCpuAffinity);

    // add our idle handler
    NetIdleAdd(&_SocketIdle, pState);

    // start winsock
    memset(&WSAData, 0, sizeof(WSAData));
    iResult = WSAStartup(MAKEWORD(2,2), &WSAData);
    if (iResult != 0)
    {
        NetPrintf(("dirtynetxboxone: error %d loading winsock library\n", iResult));
        SocketDestroy((uint32_t)(-1));
        return(-3);
    }

    // save the available version
    pState->iVersion = (LOBYTE(WSAData.wVersion)<<8)|(HIBYTE(WSAData.wVersion)<<0);

    // create hostname cache
    if ((pState->pHostnameCache = SocketHostnameCacheCreate(iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetxboxone: unable to create hostname cache\n"));
        SocketDestroy((uint32_t)(-1));
        return(-4);
    }

    #if SOCKET_ASYNCRECVTHREAD
    // create a global event to notify recv thread of newly available sockets
    pState->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (pState->hEvent == NULL)
    {
        NetPrintf(("dirtynetxboxone: error %d creating state event\n", GetLastError()));
        SocketDestroy(0);
        return(-5);
    }

    // initialize event critical section
    NetCritInit(&pState->EventCrit, "recv-event-crit");

    // hRecvThread is also the "quit flag", set to non-zero before create to avoid bogus exit
    pState->hRecvThread = (HANDLE)uTempHandle;

    /* if recvthread has been created but has no chance to run (upon failure or shutdown
       immediately), there will be a problem.  by setting iRecvLife to non-zero before
       thread creation we ensure SocketDestroy work properly. */
    pState->iRecvLife = 1; // see _SocketRecvThread for more details

    // start up socket receive thread
    if ((pState->hRecvThread = CreateThread(NULL, iThreadStackSize, (LPTHREAD_START_ROUTINE)&_SocketRecvThread, NULL, 0, (LPDWORD)&iPid)) != NULL)
    {
        // crank priority since we deal with real-time events
        if (SetThreadPriority(pState->hRecvThread, iThreadPrio) == FALSE)
        {
            NetPrintf(("dirtynetxboxone: error %d adjusting thread priority\n", GetLastError()));
        }

        // we dont need thread handle any more
        CloseHandle(pState->hRecvThread);
    }
    else
    {
        pState->iRecvLife = 0; // no recvthread was created, reset to 0
        NetPrintf(("dirtynetxboxone: error %d creating socket receive thread\n", GetLastError()));
        SocketDestroy(0);
        return(-6);
    }
    #endif

    // init ipv6 map virtual address
    pState->AddrMap.iNextVirtAddr = NetTick() & 0x00ffffff;

    // return success
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function SocketDestroy

    \Description
        Release resources and destroy module.

    \Input uShutdownFlags   - shutdown flags

    \Output
        int32_t             - negative=error, zero=success

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketDestroy(uint32_t uShutdownFlags)
{
    SocketStateT *pState = _Socket_pState;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtynetxboxone: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    NetPrintf(("dirtynetxboxone: shutting down\n"));

    // kill idle callbacks
    NetIdleDel(&_SocketIdle, pState);

    // let any idle event finish
    NetIdleDone();

    // destroy hostname cache
    if (pState->pHostnameCache != NULL)
    {
        SocketHostnameCacheDestroy(pState->pHostnameCache);
    }

    #if SOCKET_ASYNCRECVTHREAD
    // tell receive thread to quit and wake it up
    pState->hRecvThread = 0;
    if (pState->hEvent != NULL)
    {
        WSASetEvent(pState->hEvent);
    }

    // wait for thread to terminate
    while (pState->iRecvLife > 0)
    {
        Sleep(1);
    }
    #endif

    // cleanup map list if allocated
    if (pState->AddrMap.pMapEntries != NULL)
    {
        DirtyMemFree(pState->AddrMap.pMapEntries, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    // close all sockets
    while (pState->pSockList != NULL)
    {
        SocketClose(pState->pSockList);
    }

    // clear the kill list
    _SocketIdle(pState);

    #if SOCKET_ASYNCRECVTHREAD
    // we rely on the Handle having been created to know if the critical section was created.
    if (pState->hEvent != NULL)
    {
        // destroy event critical section
        NetCritKill(&pState->EventCrit);
        // delete global event
        CloseHandle(pState->hEvent);
    }
    #endif

    // free the memory and clear global module ref
    _Socket_pState = NULL;
    DirtyMemFree(pState, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // shut down network libs
    NetLibDestroy(0);

    // shutdown winsock, unless told otherwise
    if (uShutdownFlags == 0)
    {
        WSACleanup();
    }

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function SocketOpen

    \Description
        Create a new transfer endpoint. A socket endpoint is required for any
        data transfer operation.

    \Input af       - address family (AF_INET)
    \Input type     - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input proto    - protocol type for SOCK_RAW (unused by others)

    \Output
        SocketT     - socket reference

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
SocketT *SocketOpen(int32_t af, int32_t type, int32_t proto)
{
    return(_SocketOpen(INVALID_SOCKET, af, type, proto));
}

/*F*************************************************************************************/
/*!
    \Function SocketClose

    \Description
        Close a socket. Performs a graceful shutdown of connection oriented protocols.

    \Input *pSocket    - socket reference

    \Output
        int32_t     - zero

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketClose(SocketT *pSocket)
{
    return(_SocketClose(pSocket, TRUE));
}

/*F*************************************************************************************/
/*!
    \Function SocketImport

    \Description
        Import a socket.  The given socket ref may be a SocketT, in which case a
        SocketT pointer to the ref is returned, or it can be an actual Sony socket ref,
        in which case a SocketT is created for the Sony socket ref.

    \Input uSockRef     - socket reference

    \Output
        SocketT *       - pointer to imported socket, or NULL

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
SocketT *SocketImport(intptr_t uSockRef)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iProto, iProtoSize;
    SocketT *pSocket;

    // see if this socket is already in our socket list
    NetCritEnter(NULL);
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
    {
        if (pSocket == (SocketT *)uSockRef)
        {
            break;
        }
    }
    NetCritLeave(NULL);

    // if socket is in socket list, just return it
    if (pSocket != NULL)
    {
        return(pSocket);
    }

    //$$ TODO - this assumes AF_INET

    // get info from socket ref
    iProtoSize = sizeof(iProto);
    if (getsockopt((SOCKET)uSockRef, 0, SO_TYPE, (char *)&iProto, &iProtoSize) != SOCKET_ERROR)
    {
        // create the socket (note: winsock socket types directly map to dirtysock socket types)
        pSocket = _SocketOpen(uSockRef, AF_INET, iProto, 0);

        // update local and remote addresses
        SocketInfo(pSocket, 'bind', 0, &pSocket->LocalAddr, sizeof(pSocket->LocalAddr));
        SocketInfo(pSocket, 'peer', 0, &pSocket->RemoteAddr, sizeof(pSocket->RemoteAddr));

        // mark it as imported
        pSocket->uImported = 1;
    }

    return(pSocket);
}

/*F*************************************************************************************/
/*!
    \Function SocketRelease

    \Description
        Release an imported socket.

    \Input *pSocket   - pointer to socket

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
void SocketRelease(SocketT *pSocket)
{
    // if it wasn't imported, nothing to do
    if (pSocket->uImported == FALSE)
    {
        return;
    }

    // dispose of SocketT, but leave the sockref alone
    _SocketClose(pSocket, FALSE);
}

/*F*************************************************************************************/
/*!
    \Function SocketShutdown

    \Description
        Perform partial/complete shutdown of socket indicating that either sending
        and/or receiving is complete.

    \Input *pSocket - socket reference
    \Input iHow     - SOCK_NOSEND and/or SOCK_NORECV

    \Output
        int32_t     - zero

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketShutdown(SocketT *pSocket, int32_t iHow)
{
    // no shutdown for an invalid socket
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        return(0);
    }

    // translate how
    if (iHow == SOCK_NOSEND)
    {
        iHow = SD_SEND;
    }
    else if (iHow == SOCK_NORECV)
    {
        iHow = SD_RECEIVE;
    }
    else if (iHow == (SOCK_NOSEND|SOCK_NORECV))
    {
        iHow = SD_BOTH;
    }

    pSocket->uShutdown |= iHow;
    shutdown(pSocket->uSocket, iHow);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function SocketBind

    \Description
        Bind a local address/port to a socket.

    \Input *pSocket - socket reference
    \Input *pName   - local address/port
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        If either address or port is zero, then they are filled in automatically.

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketBind(SocketT *pSocket, const struct sockaddr *pName, int32_t iNameLen)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr_in6 SockAddr6;
    int32_t iResult;

    // is the bind port a virtual port?
    if (pSocket->iType == SOCK_DGRAM)
    {
        int32_t iPort;
        uint16_t uPort;

        if ((uPort = SockaddrInGetPort(pName)) != 0)
        {
            // find virtual port in list
            for (iPort = 0; (iPort < SOCKET_MAXVIRTUALPORTS) && (pState->aVirtualPorts[iPort] != uPort); iPort++)
                ;
            if (iPort < SOCKET_MAXVIRTUALPORTS)
            {
                // acquire socket critical section
                #if SOCKET_ASYNCRECVTHREAD
                NetCritEnter(&pSocket->RecvCrit);
                #endif
                // close winsock socket
                NetPrintf(("dirtynetxboxone: [%p] making socket bound to port %d virtual\n", pSocket, uPort));
                if (pSocket->uSocket != INVALID_SOCKET)
                {
                    shutdown(pSocket->uSocket, SOCK_NOSEND);
                    closesocket(pSocket->uSocket);
                    pSocket->uSocket = INVALID_SOCKET;
                }
                /* increase socket queue size; this protects virtual sockets from having data pushed into
                   them and overwriting previous data that hasn't been read yet */
                pSocket->pRecvQueue = SocketPacketQueueResize(pSocket->pRecvQueue, 4, pState->iMemGroup, pState->pMemGroupUserData);
                // mark socket as virtual
                pSocket->uVirtualPort = uPort;
                pSocket->bVirtual = TRUE;
                // release socket critical section
                #if SOCKET_ASYNCRECVTHREAD
                NetCritLeave(&pSocket->RecvCrit);
                #endif
                return(0);
            }
        }
    }


    // set up IPv6 address (do not use an IPv4-mapped IPv6 here as it will disallow IPv6 address use)
    memset(&SockAddr6, 0, sizeof(SockAddr6));
    SockAddr6.sin6_family = AF_INET6;
    SockAddr6.sin6_port = SocketHtons(SockaddrInGetPort(pName));
    pName = (const struct sockaddr *)&SockAddr6;
    iNameLen = sizeof(SockAddr6);

    // execute the bind
    iResult = _XlatError(bind(pSocket->uSocket, pName, iNameLen));

    #if SOCKET_ASYNCRECVTHREAD
    // notify read thread that socket is ready to be read from
    if ((iResult == SOCKERR_NONE) && (pSocket->bAsyncRecv == TRUE))
    {
        WSASetEvent(pState->hEvent);
    }
    #endif

    // return result to caller
    pSocket->iLastError = iResult;
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketConnect

    \Description
        Initiate a connection attempt to a remote host.

    \Input *pSocket - socket reference
    \Input *pName   - pointer to name of socket to connect to
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        Only has real meaning for stream protocols. For a datagram protocol, this
        just sets the default remote host.

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketConnect(SocketT *pSocket, struct sockaddr *pName, int32_t iNameLen)
{
    struct sockaddr_in6 SockAddr6;
    int32_t iResult;

    NetPrintf(("dirtynetxboxone: connecting to %a:%d\n", SockaddrInGetAddr(pName), SockaddrInGetPort(pName)));

    // mark as not open
    pSocket->iOpened = 0;

    // save connect address
    ds_memcpy_s(&pSocket->RemoteAddr, sizeof(pSocket->RemoteAddr), pName, sizeof(*pName));

    // translate to IPv6 if required
    pName = _SocketAddrTranslate(_Socket_pState, (struct sockaddr *) &SockAddr6, pName, &iNameLen);

    // execute the connect
    iResult = _XlatError(connect(pSocket->uSocket, pName, iNameLen));

    #if SOCKET_ASYNCRECVTHREAD
    // notify read thread that socket is ready to be read from
    if ((iResult == SOCKERR_NONE) && (pSocket->bAsyncRecv == TRUE))
    {
        WSASetEvent(_Socket_pState->hEvent);
    }
    #endif

    // return result to caller
    pSocket->iLastError = iResult;
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketListen

    \Description
        Start listening for an incoming connection on the socket.  The socket must already
        be bound and a stream oriented connection must be in use.

    \Input *pSocket - socket reference to bound socket (see SocketBind())
    \Input iBackLog - number of pending connections allowed

    \Output
        int32_t         - standard network error code (SOCKERR_xxx)

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketListen(SocketT *pSocket, int32_t iBackLog)
{
    // do the listen
    pSocket->iLastError = _XlatError(listen(pSocket->uSocket, iBackLog));
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketAccept

    \Description
        Accept an incoming connection attempt on a socket.

    \Input *pSocket - socket reference to socket in listening state (see SocketListen())
    \Input *pAddr   - pointer to storage for address of the connecting entity, or NULL
    \Input *pAddrLen - pointer to storage for length of address, or NULL

    \Output
        SocketT *   - the accepted socket, or NULL if not available

    \Notes
        The integer pointed to by addrlen should on input contain the number of characters
        in the buffer addr.  On exit it will contain the number of characters in the
        output address.

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
SocketT *SocketAccept(SocketT *pSocket, struct sockaddr *pAddr, int32_t *pAddrLen)
{
    SocketT *pOpen = NULL;
    SOCKET iIncoming;

    pSocket->iLastError = SOCKERR_INVALID;

    // see if already connected
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        return(NULL);
    }

    // make sure turn parm is valid
    if ((pAddr != NULL) && (*pAddrLen < sizeof(struct sockaddr)))
    {
        return(NULL);
    }

    // perform inet6 accept
    if (pSocket->iFamily == AF_INET6)
    {
        struct sockaddr_in6 SockAddr6;
        int32_t iAddrLen, iIpv4Addr;

        memset(&SockAddr6, 0, sizeof(SockAddr6));
        SockAddr6.sin6_family = AF_INET6;
        SockAddr6.sin6_port   = SocketHtons(SockaddrInGetPort(pAddr));
        SockAddr6.sin6_addr   = in6addr_any;
        iAddrLen = sizeof(SockAddr6);

        iIncoming = accept(pSocket->uSocket, (struct sockaddr *)&SockAddr6, &iAddrLen);
        if (iIncoming != INVALID_SOCKET)
        {
            pOpen = _SocketOpen(iIncoming, pSocket->iFamily, pSocket->iType, pSocket->iProto);
            pSocket->iLastError = SOCKERR_NONE;
            // translate ipv6 to ipv4 virtual address
            iIpv4Addr = SocketControl(NULL, '+ip6', sizeof(SockAddr6), &SockAddr6, NULL);
            // save translated connecting info for caller
            _SocketAddrTranslate(_Socket_pState, pAddr, (struct sockaddr *) &SockAddr6, pAddrLen);
        }
        else
        {
            // use a negative error code to force _XlatError to internally call WSAGetLastError() and translate the obtained result
            pSocket->iLastError = _XlatError(-99);

            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                NetPrintf(("dirtynetxboxone: accept() failed err=%d\n", WSAGetLastError()));
            }
        }
    }

    // return the socket
    return(pOpen);
}

/*F*************************************************************************************/
/*!
    \Function SocketSendto

    \Description
        Send data to a remote host. The destination address is supplied along with
        the data. Should only be used with datagram sockets as stream sockets always
        send to the connected peer.

    \Input *pSocket - socket reference
    \Input *pBuf    - the data to be sent
    \Input iLen     - size of data
    \Input iFlags   - unused
    \Input *pTo     - the address to send to (NULL=use connection address)
    \Input iToLen   - length of address

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketSendto(SocketT *pSocket, const char *pBuf, int32_t iLen, int32_t iFlags, const struct sockaddr *pTo, int32_t iToLen)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iResult;

    if (pSocket->bSendCbs)
    {
        // if installed, give socket callback right of first refusal
        if ((iResult = SocketSendCallbackInvoke(&pState->aSendCbEntries[0], pSocket, pSocket->iType, pBuf, iLen, pTo)) > 0)
        {
            return(iResult);
        }
    }

    // make sure socket ref is valid
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        #if DIRTYCODE_LOGGING
        uint32_t uAddr = 0, uPort = 0;
        if (pTo)
        {
            uAddr = SockaddrInGetAddr(pTo);
            uPort = SockaddrInGetPort(pTo);
        }
        NetPrintf(("dirtynetxboxone: attempting to send to %a:%d on invalid socket\n", uAddr, uPort));
        #endif
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // handle optional data rate throttling
    if ((iLen = SocketRateThrottle(&pSocket->SendRate, pSocket->iType, iLen, "send")) == 0)
    {
        return(0);
    }

    // use appropriate version
    if (pTo == NULL)
    {
        iResult = send(pSocket->uSocket, pBuf, iLen, 0);
        pTo = &pSocket->RemoteAddr;
    }
    else
    {
        struct sockaddr_in6 SockAddr6;
        struct sockaddr *pTo6;
        iToLen = sizeof(SockAddr6);
        pTo6 = _SocketAddrTranslate(pState, (struct sockaddr *)&SockAddr6, pTo, &iToLen);
        iResult = sendto(pSocket->uSocket, pBuf, iLen, 0, pTo6, iToLen);
    }

    // update data rate estimation
    SocketRateUpdate(&pSocket->SendRate, iResult, "send");

    // return bytes sent
    pSocket->iLastError = _XlatError(iResult);
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketRecvfrom

    \Description
        Receive data from a remote host. If socket is a connected stream, then data can
        only come from that source. A datagram socket can receive from any remote host.

    \Input *pSock       - socket reference
    \Input *pBuf        - buffer to receive data
    \Input iLen         - length of recv buffer
    \Input iFlags       - unused
    \Input *pFrom       - address data was received from (NULL=ignore)
    \Input *pFromLen    - length of address

    \Output
        int32_t         - positive=data bytes received, else standard error code

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketRecvfrom(SocketT *pSock, const char *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pFrom, int32_t *pFromLen)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iRecv = 0, iRecvErr = 0;

    // handle rate throttling, if enabled
    if ((iLen = SocketRateThrottle(&pSock->RecvRate, pSock->iType, iLen, "recv")) == 0)
    {
        return(0);
    }

    #if SOCKET_ASYNCRECVTHREAD
    // sockets marked for async recv had actual receive operation take place in the thread
    if (pSock->bAsyncRecv)
    {
        // do not try to get a packet if no buffer
        if (iLen <= 0)
        {
            return(0);
        }

        // acquire socket receive critical section
        NetCritEnter(&pSock->RecvCrit);

        // get a packet
        iRecv = SocketPacketQueueRem(pSock->pRecvQueue, (uint8_t *)pBuf, iLen, &pSock->RecvAddr);

        if ((iRecv > 0) || (pSock->iRecvStat != 0))
        {
            if (pFrom != NULL)
            {
                ds_memcpy_s(pFrom, sizeof(*pFrom), &pSock->RecvAddr, sizeof(pSock->RecvAddr));
                *pFromLen = sizeof(*pFrom);
            }

            if (iRecv > 0)
            {
                // got data, no error
                iRecvErr = pSock->iRecvErr = 0;
            }

            // mark data as consumed
            pSock->iRecvStat = 0;

            /* If a caller were to poll SocketRecvfrom() directly on a UDP socket, they would
               get a single queued packet in the first call and the second would return nothing,
               even if data is waiting, due to the fact that we queue only a single packet in
               the recv thread. This introduces a "bubble" in receiving data that drastically
               limits throughput. Normal usage ("normal" meaning "commudp") does not see this
               problem because the async recv thread forwards data immediately to the installed
               CommUDP event handler which queues the packet there. To remove the performance
               issue when using SocketRecvfrom() directly (no callback registered), as soon
               as recv data is consumed by the client, we try to fill the receive buffer with
               new data from the socket to maximize chances of data being available to the
               customer next time he calls SocketRecvfrom(). */
            if ((pSock->bRecvInp == FALSE) && (pSock->pCallback == NULL))
            {
                _SocketRead(pState, pSock);

                // IO pending, wake up receive thread to wait on this socket's event
                if ((pSock->bRecvInp != FALSE) && (pSock->iRecvErr == WSA_IO_PENDING))
                {
                    WSASetEvent(pState->hEvent);
                }
            }
        }

        // wake up receive thread?
        if (pSock->iRecvErr != WSA_IO_PENDING)
        {
            pSock->iRecvErr = 0;
            WSASetEvent(pState->hEvent);
        }

        // release socket receive critical section
        NetCritLeave(&pSock->RecvCrit);

        // if we got no input
        if (iRecv == 0)
        {
            // translate the error
            pSock->iLastError = (iRecvErr == 0) ? SOCKERR_NONE : _XlatError0(SOCKET_ERROR, iRecvErr);
            return(pSock->iLastError);
        }
    }
    else
    #else
    // check for pushed data
    if ((iRecv = SocketPacketQueueRem(pSock->pRecvQueue, (uint8_t *)pBuf, iLen, &pSock->RecvAddr)) > 0)
    {
        if (pFrom != NULL)
        {
            ds_memcpy_s(pFrom, sizeof(*pFrom), &pSock->RecvAddr, sizeof(pSock->RecvAddr));
        }
        if (iLen > iRecv)
        {
            iLen = iRecv;
        }
        return(iLen);
    }
    #endif
    {
        // make sure socket ref is valid
        if (pSock->uSocket == INVALID_SOCKET)
        {
            pSock->iLastError = SOCKERR_INVALID;
            return(pSock->iLastError);
        }

        // socket not handled by async recv thread, so read directly here
        if (pFrom != NULL)
        {
            struct sockaddr_in6 SockAddr6;
            *pFromLen = sizeof(SockAddr6);
            if ((iRecv = recvfrom(pSock->uSocket, pBuf, iLen, 0, pFrom, pFromLen)) > 0)
            {
                _SocketAddrTranslate(pState, pFrom, (struct sockaddr *)&SockAddr6, pFromLen);
                SockaddrInSetMisc(pFrom, NetTick());
            }
        }
        else
        {
            iRecv = recv(pSock->uSocket, pBuf, iLen, 0);
            pFrom = &pSock->RemoteAddr;
        }

        // get most recent socket error
        if ((iRecvErr = WSAGetLastError()) == WSAEMSGSIZE)
        {
            /* if there was a message truncation, simply return the truncated size.  this matches what we
               do with the async recv thread version and also the linux behavior of recvfrom() */
            iRecv = iLen;
        }
    }

    // do error conversion
    iRecv = (iRecv == 0) ? SOCKERR_CLOSED : _XlatError0(iRecv, iRecvErr);

    // update data rate estimation
    SocketRateUpdate(&pSock->RecvRate, iRecv, "recv");

    // return the error code
    pSock->iLastError = iRecv;
    return(pSock->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function SocketInfo

    \Description
        Return information about an existing socket.

    \Input *pSocket - socket reference
    \Input iInfo    - selector for desired information
    \Input iData    - selector specific
    \Input *pBuf    - [out] return buffer
    \Input iLen     - buffer length

    \Output
        int32_t     - size of returned data or error code (negative value)

    \Notes
        iInfo can be one of the following:

        \verbatim
            'addr' - return local address
            'conn' - who we are connecting to
            'bind' - return bind data (if pSocket == NULL, get socket bound to given port)
            'bndu' - return bind data (only with pSocket=NULL, get SOCK_DGRAM socket bound to given port)
            'maxp' - return configured max packet size
            'maxr' - return configured max recv rate (bytes/sec; zero=uncapped)
            'maxs' - return configured max send rate (bytes/sec; zero=uncapped)
            'peer' - peer info (only valid if connected)
            'ratr' - return current recv rate estimation (bytes/sec)
            'rats' - return current send rate estimation (bytes/sec)
            'sdcf' - get installed send callback function pointer (iData specifies index in array)
            'sdcu' - get installed send callback userdata pointer (iData specifies index in array)
            'serr' - last socket error
            'sock' - return windows socket associated with the specified DirtySock socket
            'stat' - TRUE if connected, else FALSE
            'virt' - TRUE if socket is virtual, else FALSE
        \endverbatim

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketInfo(SocketT *pSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    SocketStateT *pState = _Socket_pState;

    // always zero results by default
    if (pBuf != NULL)
    {
        memset(pBuf, 0, iLen);
    }

    // handle global socket options
    if (pSocket == NULL)
    {
        if (iInfo == 'addr')
        {
            // force new acquisition of address?
            if (iData == 1)
            {
                pState->uLocalAddr = 0;
            }
            return(SocketGetLocalAddr());
        }
        // get socket bound to given port
        if ( (iInfo == 'bind') || (iInfo == 'bndu') )
        {
            struct sockaddr BindAddr;
            int32_t iFound = -1;

            // for access to socket list
            NetCritEnter(NULL);

            // walk socket list and find matching socket
            for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->pNext)
            {
                // if iInfo is 'bndu', only consider sockets of type SOCK_DGRAM
                // note: 'bndu' stands for "bind udp"
                if ( (iInfo == 'bind') || ((iInfo == 'bndu') && (pSocket->iType == SOCK_DGRAM)) )
                {
                    // get socket info
                    SocketInfo(pSocket, 'bind', 0, &BindAddr, sizeof(BindAddr));
                    if (SockaddrInGetPort(&BindAddr) == iData)
                    {
                        *(SocketT **)pBuf = pSocket;
                        iFound = 0;
                        break;
                    }
                }
            }

            // for access to g_socklist and g_sockkill
            NetCritLeave(NULL);
            return(iFound);
        }
        // return max packet size
        if (iInfo == 'maxp')
        {
            return(pState->iMaxPacket);
        }

        // get send callback function pointer (iData specifies index in array)
        if (iInfo == 'sdcf')
        {
            if ((pBuf != NULL) && (iLen == sizeof(pState->aSendCbEntries[iData].pSendCallback)))
            {
                ds_memcpy(pBuf, &pState->aSendCbEntries[iData].pSendCallback, sizeof(pState->aSendCbEntries[iData].pSendCallback));
                return(0);
            }

            NetPrintf(("dirtynetxboxone: 'sdcf' selector used with invalid paramaters\n"));
            return(-1);
        }
        // get send callback user data pointer (iData specifies index in array)
        if (iInfo == 'sdcu')
        {
            if ((pBuf != NULL) && (iLen == sizeof(pState->aSendCbEntries[iData].pSendCallref)))
            {
                ds_memcpy(pBuf, &pState->aSendCbEntries[iData].pSendCallref, sizeof(pState->aSendCbEntries[iData].pSendCallref));
                return(0);
            }

            NetPrintf(("dirtynetxboxone: 'sdcu' selector used with invalid paramaters\n"));
            return(-1);
        }


        NetPrintf(("dirtynetxboxone: unhandled global SocketInfo() selector '%C'\n", iInfo));

        return(-1);
    }

    // return local bind data
    if (iInfo == 'bind')
    {
        int32_t iResult = 0;
        if (pSocket->bVirtual == TRUE)
        {
            SockaddrInit((struct sockaddr *)pBuf, AF_INET);
            SockaddrInSetPort((struct sockaddr *)pBuf, pSocket->uVirtualPort);
        }
        else
        {
            struct sockaddr_in6 SockAddr6;
            iLen = sizeof(SockAddr6);
            if ((iResult = getsockname(pSocket->uSocket, (struct sockaddr *)&SockAddr6, &iLen)) == 0)
            {
                SockaddrInit((struct sockaddr *)pBuf, AF_INET);
                SockaddrInSetPort((struct sockaddr *)pBuf, SocketHtons(SockAddr6.sin6_port));
                // do not support addr at this time
            }
            iResult = _XlatError(iResult);
        }

        return(iResult);
    }

    // return configured max recv rate
    if (iInfo == 'maxr')
    {
        return(pSocket->RecvRate.uMaxRate);
    }

    // return configured max send rate
    if (iInfo == 'maxs')
    {
        return(pSocket->SendRate.uMaxRate);
    }

    // return socket protocol
    if (iInfo == 'prot')
    {
        return(pSocket->iProto);
    }

    // return whether the socket is virtual or not
    if (iInfo == 'virt')
    {
        return(pSocket->bVirtual);
    }

    // make sure we are connected
    if (pSocket->uSocket == INVALID_SOCKET)
    {
        pSocket->iLastError = SOCKERR_OTHER;
        return(pSocket->iLastError);
    }

    // return peer info (only valid if connected)
    if ((iInfo == 'conn') || (iInfo == 'peer'))
    {
        getpeername(pSocket->uSocket, (struct sockaddr *)pBuf, &iLen);
        //$$TODO this needs to be IPv6, translate to virtual address for output
        return(0);
    }

    // return current recv rate estimation
    if (iInfo == 'ratr')
    {
        return(pSocket->RecvRate.uCurRate);
    }

    // return current send rate estimation
    if (iInfo == 'rats')
    {
        return(pSocket->SendRate.uCurRate);
    }

    // return last socket error
    if (iInfo == 'serr')
    {
        return(pSocket->iLastError);
    }

    // return windows socket identifier
    if (iInfo == 'sock')
    {
        if (pBuf != NULL)
        {
            if (iLen == (int32_t)sizeof(pSocket->uSocket))
            {
                memcpy(pBuf, &pSocket->uSocket, sizeof(pSocket->uSocket));
            }
        }
        return((int32_t)pSocket->uSocket);
    }

    // return socket status
    if (iInfo == 'stat')
    {
        fd_set FdRead, FdWrite, FdExcept;
        struct timeval TimeVal;

        // if not a connected socket, return TRUE
        if (pSocket->iType != SOCK_STREAM)
        {
            return(1);
        }

        // if not connected, use select to determine connect
        if (pSocket->iOpened == 0)
        {
            // setup write/exception lists so we can select against the socket
            FD_ZERO(&FdWrite);
            FD_ZERO(&FdExcept);
            FD_SET(pSocket->uSocket, &FdWrite);
            FD_SET(pSocket->uSocket, &FdExcept);
            TimeVal.tv_sec = TimeVal.tv_usec = 0;
            if (select(1, NULL, &FdWrite, &FdExcept, &TimeVal) > 0)
            {
                // if we got an exception, that means connect failed
                if (FdExcept.fd_count > 0)
                {
                    NetPrintf(("dirtynetxboxone: connection failed\n"));
                    pSocket->iOpened = -1;
                }
                // if socket is writable, that means connect succeeded
                else if (FdWrite.fd_count > 0)
                {
                    NetPrintf(("dirtynetxboxone: connection open\n"));
                    pSocket->iOpened = 1;
                }
            }
        }

        // if previously connected, make sure connect still valid
        if (pSocket->iOpened > 0)
        {
            FD_ZERO(&FdRead);
            FD_ZERO(&FdExcept);
            FD_SET(pSocket->uSocket, &FdRead);
            FD_SET(pSocket->uSocket, &FdExcept);
            TimeVal.tv_sec = TimeVal.tv_usec = 0;
            if (select(1, &FdRead, NULL, &FdExcept, &TimeVal) > 0)
            {
                // if we got an exception, that means connect failed (usually closed by remote peer)
                if (FdExcept.fd_count > 0)
                {
                    NetPrintf(("dirtynetxboxone: connection failure\n"));
                    pSocket->iOpened = -1;
                }
                else if (FdRead.fd_count > 0)
                {
                    u_long uAvailBytes = 1; // u_long is required by ioctlsocket
                    // if socket is readable but there's no data available, connect was closed
                    // uAvailBytes might be less than actual bytes, so it can only be used for zero-test
                    if ((ioctlsocket(pSocket->uSocket, FIONREAD, &uAvailBytes) != 0) || (uAvailBytes == 0))
                    {
                        pSocket->iLastError = SOCKERR_CLOSED;
                        NetPrintf(("dirtynetxboxone: connection closed (wsaerr=%d)\n", (uAvailBytes==0) ? WSAECONNRESET : WSAGetLastError()));
                        pSocket->iOpened = -1;
                    }
                }
            }
        }

        // return connect status
        return(pSocket->iOpened);
    }

    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function SocketCallback

    \Description
        Register a callback routine for notification of socket events.  Also includes
        timeout support.

    \Input *pSocket - socket reference
    \Input iMask    - valid callback events (CALLB_NONE, CALLB_SEND, CALLB_RECV)
    \Input iIdle    - if nonzero, specifies the number of ticks between idle calls
    \Input *pRef    - user data to be passed to proc
    \Input *pProc   - user callback

    \Output
        int32_t     - zero

    \Notes
        A callback will reset the idle timer, so when specifying a callback and an
        idle processing time, the idle processing time represents the maximum elapsed
        time between calls.

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
#pragma optimize("", off) // make sure this block of code is not reordered
int32_t SocketCallback(SocketT *pSocket, int32_t iMask, int32_t iIdle, void *pRef, int32_t (*pProc)(SocketT *pSocket, int32_t iFlags, void *pRef))
{
    pSocket->uCallIdle = iIdle;
    pSocket->uCallMask = iMask;
    pSocket->pCallRef = pRef;
    pSocket->pCallback = pProc;
    return(0);
}
#pragma optimize("", on)

/*F*************************************************************************************/
/*!
    \Function SocketControl

    \Description
        Process a control message (type specific operation)

    \Input *pSocket - socket to control, or NULL for module-level option
    \Input iOption  - the option to pass
    \Input iData1   - message specific parm
    \Input *pData2  - message specific parm
    \Input *pData3  - message specific parm

    \Output
        int32_t     - message specific result (-1=unsupported message, -2=no such module)

    \Notes
        iOption can be one of the following:

        \verbatim
            'arcv' - set async receive enable/disable (default enabled for DGRAM/RAW, disabled for TCP)
            'conn' - handle connect message
            'disc' - handle disconnect message
            '+ip6' - add an IPv6 address into the mapping table and return a virtual IPv4 address to reference it
            '-ip6' - del an IPv6 address from the mapping table
            'maxp' - set max udp packet size
            'maxr' - set max recv rate (bytes/sec; zero=uncapped)
            'maxs' - set max send rate (bytes/sec; zero=uncapped)
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'ndly' - set TCP_NODELAY state for given stream socket (iData1=zero or one)
            'pdev' - set simulated packet deviation
            'plat' - set simulated packet latency
            'plos' - set simulated packet loss
            'poll' - execute blocking wait on given socket list (pData2) or all sockets (pData2=NULL), 64 max sockets
            'push' - push data into given socket (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'rbuf' - set socket recv buffer size
            'sbuf' - set socket send buffer size
            'scbk' - enable/disable "send callbacks usage" on specified socket (defaults to enable)
            'sdcb' - set/unset send callback (iData1=TRUE for set - FALSE for unset, pData2=callback, pData3=callref)
            'vadd' - add a port to virtual port list
            'vdel' - del a port from virtual port list
        \endverbatim

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketControl(SocketT *pSocket, int32_t iOption, int32_t iData1, void *pData2, void *pData3)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iResult;

    // set async recv enable
    #if SOCKET_ASYNCRECVTHREAD
    if (iOption == 'arcv')
    {
        // set socket async recv flag
        pSocket->bAsyncRecv = iData1 ? TRUE : FALSE;
        // wake up recvthread to update socket polling
        WSASetEvent(pState->hEvent);
        return(0);
    }
    #endif // SOCKET_ASYNCRECVTHREAD
    // handle connect message
    if (iOption == 'conn')
    {
        return(0);
    }
    // handle disconnect message
    if (iOption == 'disc')
    {
        return(0);
    }
    // set an ipv6 address into the mapping table
    if (iOption == '+ip6')
    {
        return(_SocketAddrMapAddress(pState, (const struct sockaddr *)pData2, iData1));
    }
    // del an ipv6 address from the mapping table
    if (iOption == '-ip6')
    {
        return(_SocketAddrUnmapAddress(pState, (const struct sockaddr *)pData2, iData1));
    }
    // set max udp packet size
    if (iOption == 'maxp')
    {
        NetPrintf(("dirtynetxboxone: setting max udp packet size to %d\n", iData1));
        pState->iMaxPacket = iData1;
        return(0);
    }
    // set max recv rate
    if (iOption == 'maxr')
    {
        NetPrintf(("dirtynetxboxone: setting max recv rate to %d bytes/sec\n", iData1));
        pSocket->RecvRate.uMaxRate = iData1;
        return(0);
    }
    // set max send rate
    if (iOption == 'maxs')
    {
        NetPrintf(("dirtynetxboxone: setting max send rate to %d bytes/sec\n", iData1));
        pSocket->SendRate.uMaxRate = iData1;
        return(0);
    }
    // if a stream socket, set nonblocking/blocking mode
    if ((iOption == 'nbio') && (pSocket != NULL) && (pSocket->iType == SOCK_STREAM))
    {
        uint32_t uNbio = (uint32_t)iData1;
        iResult = ioctlsocket(pSocket->uSocket, FIONBIO, (u_long *)&uNbio);
        pSocket->iLastError = _XlatError(iResult);
        NetPrintf(("dirtynetxboxone: setting socket:0x%x to %s mode %s (LastError=%d).\n", pSocket, iData1 ? "nonblocking" : "blocking", iResult ? "failed" : "succeeded", pSocket->iLastError));
        return(pSocket->iLastError);
    }
    // if a stream socket, set TCP_NODELAY state
    if ((iOption == 'ndly') && (pSocket != NULL) && (pSocket->iType == SOCK_STREAM))
    {
        iResult = setsockopt(pSocket->uSocket, IPPROTO_TCP, TCP_NODELAY, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);
        return(pSocket->iLastError);
    }
    // set simulated packet loss or packet latency
    #if SOCKET_ASYNCRECVTHREAD
    if ((iOption == 'pdev') || (iOption == 'plat') || (iOption == 'plos'))
    {
        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);
        // forward selector to packet queue
        iResult = SocketPacketQueueControl(pSocket->pRecvQueue, iOption, iData1);
        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);
        return(iResult);
    }
    #endif
    // block waiting on input from socket list
    if (iOption == 'poll')
    {
        fd_set FdRead, FdExcept;
        struct timeval TimeVal;
        SocketT *pSocket, **ppSocket;
        int32_t iSocket;

        FD_ZERO(&FdRead);
        FD_ZERO(&FdExcept);
        TimeVal.tv_sec = iData1/1000;
        TimeVal.tv_usec = (iData1%1000) * 1000;

        // if socket list specified, use it
        if (pData2 != NULL)
        {
            // add sockets to select list (FD_SETSIZE is 64)
            for (ppSocket = (SocketT **)pData2, iSocket = 0; (*ppSocket != NULL) && (iSocket < FD_SETSIZE); ppSocket++, iSocket++)
            {
                FD_SET((*ppSocket)->uSocket, &FdRead);
                FD_SET((*ppSocket)->uSocket, &FdExcept);
            }
        }
        else
        {
            // get exclusive access to socket list
            NetCritEnter(NULL);
            // walk socket list and add all sockets
            for (pSocket = pState->pSockList, iSocket = 0; (pSocket != NULL) && (iSocket < FD_SETSIZE); pSocket = pSocket->pNext, iSocket++)
            {
                if (pSocket->uSocket != INVALID_SOCKET)
                {
                    FD_SET(pSocket->uSocket, &FdRead);
                    FD_SET(pSocket->uSocket, &FdExcept);
                }
            }
            // for access to g_socklist and g_sockkill
            NetCritLeave(NULL);
        }

        // wait for input on the socket list for up to iData1 milliseconds
        return(select(iSocket, &FdRead, NULL, &FdExcept, &TimeVal));
    }
    // change packet queue size
    #if SOCKET_ASYNCRECVTHREAD
    if (iOption == 'pque')
    {
        // acquire socket receive critical section
        NetCritEnter(&pSocket->RecvCrit);
        // resize the queue
        pSocket->pRecvQueue = SocketPacketQueueResize(pSocket->pRecvQueue, iData1, pState->iMemGroup, pState->pMemGroupUserData);
        // release socket receive critical section
        NetCritLeave(&pSocket->RecvCrit);
    }
    #endif
    // push data into receive buffer
    if (iOption == 'push')
    {
        if (pSocket != NULL)
        {
            // acquire socket critical section
            #if SOCKET_ASYNCRECVTHREAD
            NetCritEnter(&pSocket->RecvCrit);
            #endif

            // don't allow data that is too large (for the buffer) to be pushed
            if (iData1 > SOCKET_MAXUDPRECV)
            {
                NetPrintf(("dirtynetxboxone: request to push %d bytes of data discarded (max=%d)\n", iData1, SOCKET_MAXUDPRECV));
                #if SOCKET_ASYNCRECVTHREAD
                NetCritLeave(&pSocket->RecvCrit);
                #endif
                return(-1);
            }

            // add packet to queue
            SocketPacketQueueAdd(pSocket->pRecvQueue, (uint8_t *)pData2, iData1, (struct sockaddr *)pData3);

            // release socket critical section
            #if SOCKET_ASYNCRECVTHREAD
            NetCritLeave(&pSocket->RecvCrit);
            #endif

            // see if we should issue callback
            if ((pSocket->pCallback != NULL) && (pSocket->uCallMask & CALLB_RECV))
            {
                pSocket->pCallback(pSocket, 0, pSocket->pCallRef);
            }
        }
        else
        {
            NetPrintf(("dirtynetxboxone: warning - call to SocketControl('push') ignored because pSocket is NULL\n"));
            return(-1);
        }
        return(0);
    }
    // set REUSEADDR
    if (iOption == 'radr')
    {
        iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);
        return(pSocket->iLastError);
    }
    // set socket receive buffer size
    if ((iOption == 'rbuf') || (iOption == 'sbuf'))
    {
        int32_t iOldSize, iNewSize, iOptLen=4;
        int32_t iSockOpt = (iOption == 'rbuf') ? SO_RCVBUF : SO_SNDBUF;

        // get current buffer size
        getsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (char *)&iOldSize, &iOptLen);

        // set new size
        iResult = setsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);

        // get new size
        getsockopt(pSocket->uSocket, SOL_SOCKET, iSockOpt, (char *)&iNewSize, &iOptLen);
        NetPrintf(("dirtynetxboxone: setsockopt(%s) changed buffer size from %d to %d\n", (iOption == 'rbuf') ? "SO_RCVBUF" : "SO_SNDBUF",
            iOldSize, iNewSize));

        return(pSocket->iLastError);
    }
    // enable/disable "send callbacks usage" on specified socket (defaults to enable)
    if (iOption == 'scbk')
    {
        if (pSocket->bSendCbs != (iData1?TRUE:FALSE))
        {
            NetPrintf(("dirtynetxboxone: send callbacks usage changed from %s to %s for socket ref %p\n", (pSocket->bSendCbs?"ON":"OFF"), (iData1?"ON":"OFF"), pSocket));
            pSocket->bSendCbs = (iData1?TRUE:FALSE);
        }
        return(0);
    }
    // set/unset send callback (iData1=TRUE for set - FALSE for unset, pData2=callback, pData3=callref)
    if (iOption == 'sdcb')
    {
        SocketSendCallbackEntryT sendCbEntry;
        sendCbEntry.pSendCallback = (SocketSendCallbackT *)pData2;
        sendCbEntry.pSendCallref = pData3;

        if (iData1)
        {
            return(SocketSendCallbackAdd(&pState->aSendCbEntries[0], &sendCbEntry));
        }
        else
        {
            return(SocketSendCallbackRem(&pState->aSendCbEntries[0], &sendCbEntry));
        }
    }
    // set debug spam level
    if (iOption == 'spam')
    {
        NetPrintf(("dirtynetxboxone: debug verbosity level changed from %d to %d\n", pState->iVerbose, iData1));
        pState->iVerbose = iData1;
        return(0);
    }
    // mark a port as virtual
    if (iOption == 'vadd')
    {
        int32_t iPort;

        // find a slot to add virtual port
        for (iPort = 0; pState->aVirtualPorts[iPort] != 0; iPort++)
            ;
        if (iPort < SOCKET_MAXVIRTUALPORTS)
        {
            pState->aVirtualPorts[iPort] = (uint16_t)iData1;
            return(0);
        }
    }
    // remove port from virtual port list
    if (iOption == 'vdel')
    {
        int32_t iPort;

        // find virtual port in list
        for (iPort = 0; (iPort < SOCKET_MAXVIRTUALPORTS) && (pState->aVirtualPorts[iPort] != (uint16_t)iData1); iPort++)
            ;
        if (iPort < SOCKET_MAXVIRTUALPORTS)
        {
            pState->aVirtualPorts[iPort] = 0;
            return(0);
        }
    }
    // unhandled
    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function SocketGetLocalAddr

    \Description
        Returns the "external" local address (ie, the address as a machine "out on
        the Internet" would see as the local machine's address).

    \Output
        uint32_t        - local address

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
uint32_t SocketGetLocalAddr(void)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr InetAddr, HostAddr;

    if ((pState->uLocalAddr == 0) || (pState->uLocalAddr == 0x7f000001))
    {
        // create a remote internet address
        memset(&InetAddr, 0, sizeof(InetAddr));
        InetAddr.sa_family = AF_INET;
        SockaddrInSetPort(&InetAddr, 79);
        SockaddrInSetAddr(&InetAddr, 0xc0a80101);

        // ask socket to give us local address that can connect to it
        memset(&HostAddr, 0, sizeof(HostAddr));
        SocketHost(&HostAddr, sizeof(HostAddr), &InetAddr, sizeof(InetAddr));

        pState->uLocalAddr = SockaddrInGetAddr(&HostAddr);
    }
    return(pState->uLocalAddr);
}

/*F*************************************************************************************/
/*!
    \Function _SocketLookupDone

    \Description
        Callback to determine if gethostbyname is complete.

    \Input *pHost   - pointer to host lookup record

    \Output
        int32_t     - zero=in progess, neg=done w/error, pos=done w/success

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketLookupDone(HostentT *pHost)
{
    return(pHost->done);
}

/*F*************************************************************************************/
/*!
    \Function _SocketLookupFree

    \Description
        Release resources used by gethostbyname.

    \Input *pHost   - pointer to host lookup record

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketLookupFree(HostentT *pHost)
{
    // release resource
    pHost->refcount -= 1;
}

/*F*************************************************************************************/
/*!
    \Function _SocketLookupThread

    \Description
        Socket lookup thread

    \Input *pHost   - pointer to host lookup record

    \Output
        int32_t     - zero

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketLookupThread(HostentT *pHost)
{
    SocketStateT *pState = _Socket_pState;
    struct hostent *pWinHost;
    uint8_t *pIPAddr;

    // perform the blocking dns query
    pWinHost = gethostbyname(pHost->name);
    if (pWinHost != NULL)
    {
        // extract ip address
        pIPAddr = (unsigned char *)pWinHost->h_addr_list[0];
        pHost->addr = (pIPAddr[0]<<24)|(pIPAddr[1]<<16)|(pIPAddr[2]<<8)|(pIPAddr[3]<<0);
        NetPrintf(("dirtynetxboxone: %s=%a\n", pHost->name, pHost->addr));
        // mark success
        pHost->done = 1;
        // add hostname to cache
        SocketHostnameCacheAdd(pState->pHostnameCache, pHost->name, pHost->addr, pState->iVerbose);
    }
    else
    {
        // unsuccessful
        pHost->done = -1;
    }

    // release thread-allocated refcount on hostname resource
    pHost->refcount -= 1;

    // return thread success
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function SocketLookup

    \Description
        Lookup a host by name and return the corresponding Internet address. Uses
        a callback/polling system since the socket library does not allow blocking.

    \Input *pText   - pointer to null terminated address string
    \Input iTimeout - number of milliseconds to wait for completion

    \Output
        HostentT *  - hostent struct that includes callback vectors

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
HostentT *SocketLookup(const char *pText, int32_t iTimeout)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iAddr;
    HostentT *pHost, *pHostRef;
    DWORD dwPid;
    HANDLE hThread;

    NetPrintf(("dirtynetxboxone: looking up address for host '%s'\n", pText));

    // dont allow negative timeouts
    if (iTimeout < 0)
    {
        return(NULL);
    }

    // create new structure
    pHost = DirtyMemAlloc(sizeof(*pHost), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(pHost, 0, sizeof(*pHost));

    // setup callbacks
    pHost->Done = &_SocketLookupDone;
    pHost->Free = &_SocketLookupFree;
    // copy over the target address
    ds_strnzcpy(pHost->name, pText, sizeof(pHost->name));

    // look for refcounted lookup
    if ((pHostRef = SocketHostnameAddRef(&pState->pHostList, pHost, TRUE)) != NULL)
    {
        DirtyMemFree(pHost, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(pHostRef);
    }

    // check for dot notation
    if (((iAddr = SocketInTextGetAddr(pText)) != 0) || ((iAddr = SocketHostnameCacheGet(pState->pHostnameCache, pText, pState->iVerbose)) != 0))
    {
        // save the data
        pHost->addr = iAddr;
        pHost->done = 1;
        // mark as "thread complete" so _SocketLookupFree will release memory
        pHost->thread = 1;
        // return completed record
        return(pHost);
    }

    /* add an extra refcount for the thread; this ensures the host structure survives until the thread
       is done with it.  this must be done before thread creation. */
    pHost->refcount += 1;

    // create dns lookup thread
    if ((hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_SocketLookupThread, (LPVOID)pHost, 0, &dwPid)) != NULL)
    {
        CloseHandle(hThread);
    }
    else
    {
        NetPrintf(("dirtynetwin: CreateThread() failed\n"));
        pHost->done = -1;
        // remove refcount we just added
        pHost->refcount -= 1;
    }

    // return the host reference
    return(pHost);
}

/*F*************************************************************************************/
/*!
    \Function SocketHost

    \Description
        Return the host address that would be used in order to communicate with the given
        destination address.

    \Input *pHost   - local sockaddr struct
    \Input iHostLen - length of structure (sizeof(host))
    \Input *pDest   - remote sockaddr struct
    \Input iDestLen  - length of structure (sizeof(dest))

    \Output
        int32_t     - zero=success, negative=error

    \Version 03/22/2013 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketHost(struct sockaddr *pHost, int32_t iHostLen, const struct sockaddr *pDest, int32_t iDestLen)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr SockAddr;
    struct hostent *pHostent;
    char strHostName[256];
    int32_t iErr;
    SOCKET iSock;

    // must be same kind of addresses
    if (iHostLen != iDestLen)
    {
        return(-1);
    }

    // do family specific lookup
    if (pDest->sa_family == AF_INET)
    {
        // make them match initially
        ds_memcpy(pHost, pDest, iHostLen);

        // zero the address portion
        pHost->sa_data[2] = pHost->sa_data[3] = pHost->sa_data[4] = pHost->sa_data[5] = 0;

        // create a temp socket (must be datagram)
        iSock = socket(AF_INET, SOCK_DGRAM, 0);
        if (iSock != INVALID_SOCKET)
        {
            // use routing check if winsock 2.0
            if (pState->iVersion >= 0x200)
            {
                // get interface that would be used for this dest
                memset(&SockAddr, 0, sizeof(SockAddr));
                if (WSAIoctl(iSock, SIO_ROUTING_INTERFACE_QUERY, (void *)pDest, iDestLen, &SockAddr, sizeof(SockAddr), (LPDWORD)&iHostLen, NULL, NULL) < 0)
                {
                    iErr = WSAGetLastError();
                    NetPrintf(("dirtynetxboxone: WSAIoctl(SIO_ROUTING_INTERFACE_QUERY) returned %d\n", iErr));
                }
                else
                {
                    // copy over the result
                    ds_memcpy(pHost->sa_data+2, SockAddr.sa_data+2, 4);
                }

                // convert loopback to requested address
                if (SockaddrInGetAddr(pHost) == INADDR_LOOPBACK)
                {
                    ds_memcpy(pHost->sa_data+2, pDest->sa_data+2, 4);
                }
            }

            // if no result, try connecting to socket then reading
            // (this only works on some non-microsoft stacks)
            if (SockaddrInGetAddr(pHost) == 0)
            {
                // connect to remote address
                if (connect(iSock, pDest, iDestLen) == 0)
                {
                    // query the host address
                    if (getsockname(iSock, &SockAddr, &iHostLen) == 0)
                    {
                        // just copy over the address portion
                        ds_memcpy(pHost->sa_data+2, SockAddr.sa_data+2, 4);
                    }
                }
            }

            // if still no result, use classic gethosthame/gethostbyname
            // (works for microsoft, not for non-microsoft stacks)
            if (SockaddrInGetAddr(pHost) == 0)
            {
                // get machine name
                gethostname(strHostName, sizeof(strHostName));
                // lookup ip info
                pHostent = gethostbyname(strHostName);
                if (pHostent != NULL)
                {
                    ds_memcpy(pHost->sa_data+2, pHostent->h_addr_list[0], 4);
                }
            }

            // close the socket
            closesocket(iSock);
        }
        return(0);
    }

    // unsupported family
    memset(pHost, 0, iHostLen);
    return(-3);
}
