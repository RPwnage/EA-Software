/*H*************************************************************************************/
/*!
    \File dirtynetwin.c

    \Description
        Provide a wrapper that translates the Winsock network interface
        into DirtySock calls. In the case of Winsock, little translation
        is needed since it is based off BSD sockets (as is DirtySock).

    \Notes
        Because of type name conflicts, the code is broken into two files.

    \Copyright
        Copyright (c) Electronic Arts 1999-2004.  ALL RIGHTS RESERVED.

    \Version 1.0 01/02/2002 (gschaefer) First Version
*/
/*************************************************************************************H*/


/*** Include files *********************************************************************/

#ifndef _XBOX
 #define WIN32_LEAN_AND_MEAN 1           // avoid extra stuff
 #include <windows.h>
 #include <winsock2.h>
 #include <ws2tcpip.h>
#else
 #include <xtl.h>
 #include <winsockx.h>
#endif

#include "dirtylib.h"
#include "dirtymem.h"
#include "dirtyvers.h"
#include "dirtynet.h"

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
 #include "dirtylspxenon.h"
#endif

/*** Defines ***************************************************************************/

#define SOCKET_MAXEVENTS        (64)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! dirtysock connection socket structure
struct SocketT
{
    SocketT *next;              //!< link to next active
    SocketT *kill;              //!< link to next killed socket

    int32_t family;             //!< protocol family
    int32_t type;               //!< protocol type
    int32_t proto;              //!< protocol ident

    int8_t opened;              //!< negative=error, zero=not open (connecting), positive=open
    uint8_t imported;           //!< whether socket was imported or not
    uint8_t bVirtual;           //!< if true, socket is virtual
    uint8_t shutdown;           //!< shutdown flag
    uint32_t socket;            //!< winsock socket ref
    int32_t iLastError;         //!< last socket error

    struct sockaddr local;      //!< local address
    struct sockaddr remote;     //!< remote address

    uint16_t virtualport;       //!< virtual port, if set
    uint8_t bInsecure;          //!< socket is set for insecure access (Xenon only)
    uint8_t _pad;

    int32_t callmask;           //!< valid callback events
    uint32_t calllast;          //!< last callback tick
    uint32_t callidle;          //!< ticks between idle calls
    void *callref;              //!< reference calback value
    int32_t (*callback)(SocketT *sock, int32_t flags, void *ref);

    #if SOCKET_ASYNCRECVTHREAD
    WSAOVERLAPPED overlapped;   //!< overlapped i/o structure
    NetCritT recvcrit;          //!< receive critical section
    int32_t recverr;            //!< last error that occurred
    int32_t addrlen;            //!< storage for async address length write by WSARecv
    uint32_t recvflag;          //!< flags from recv operation
    unsigned char recvinp;      //!< if true, a receive operation is in progress
    #endif

    struct sockaddr recvaddr;   //!< receive address
    int32_t recvstat;           //!< how many queued receive bytes
    char recvdata[SOCKET_MAXUDPRECV]; //!< receive buffer
};

//! standard ipv4 packet header (see RFC791)
typedef struct HeaderIpv4
{
    unsigned char verslen;      //!< version and length fields (4 bits each)
    unsigned char service;      //!< type of service field
    unsigned char length[2];    //!< total packet length (header+data)
    unsigned char ident[2];     //!< packet sequence number
    unsigned char frag[2];      //!< fragmentation information
    unsigned char time;         //!< time to live (remaining hop count)
    unsigned char proto;        //!< transport protocol number
    unsigned char check[2];     //!< header checksum
    unsigned char srcaddr[4];   //!< source ip address
    unsigned char dstaddr[4];   //!< dest ip address
} HeaderIpv4;

//! local state
typedef struct SocketStateT
{
    SocketT *pSockList;                 //!< master socket list
    SocketT *pSockKill;                 //!< list of killed sockets
    SocketAddrMapT aSockAddrMap[32];    //!< address remap list
    SocketNameMapT aSockNameMap[32];    //!< name remap list
    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list
    int32_t iDnsRemap;                  //!< address to dns remap (or zero if no remapping)
    int32_t iMaxPacket;                 //!< maximum packet size

    // module memory group
    int32_t iMemGroup;                  //!< module mem group id
    void *pMemGroupUserData;            //!< user data associated with mem group

    int32_t iVersion;                   //!< winsock version
    uint32_t uAdapterAddress;           //!< local interface used for SocketBind() operations if non-zero

    uint32_t uPacketLoss;               //!< packet loss simulation (debug only)

    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    DirtyLSPRefT *pDirtyLSP;            //!< lsp query module state
    int32_t iFakeAddr;                  //!< next unused "fake" address
    #endif

    #if SOCKET_ASYNCRECVTHREAD
    volatile HANDLE hRecvThread;        //!< receive thread handle
    volatile int32_t iRecvLife;         //!< receive thread alive indicator
    WSAEVENT iEvent;                    //!< event used to wake up receive thread
    NetCritT EventCrit;                 //!< event critical section (used when killing events)
    #endif

    SocketSendCallbackT *pSendCallback; //!< global send callback
    void                *pSendCallref;  //!< user callback data
} SocketStateT;

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! module state ref
static SocketStateT *_Socket_pState = NULL;

// Public variables


/*** Private Functions *****************************************************************/

#if DIRTYCODE_LOGGING
/*F*************************************************************************************/
/*!
    \Function _DisplayAddrMapEntry

    \Description
        Format debug text for given addr map entry.

    \Input *pAddrMap    - address map entry

    \Output
        const char *    - output text

    \Version 04/29/2008 (jbrookes)
*/
/************************************************************************************F*/
static const char *_DisplayAddrMapEntry(SocketAddrMapT *pAddrMap)
{
    static char strOutput[128];

    // special-case for a blank entry
    if (pAddrMap->uRemap == 0)
    {
        return("[no entry]");
    }

    // format output and return to caller
    snzprintf(strOutput, sizeof(strOutput), "[%a/%a:%d] -> [%a:%d]", pAddrMap->uAddr,
        pAddrMap->uMask, pAddrMap->uSrcPort, pAddrMap->uRemap, pAddrMap->uDstPort);
    return(strOutput);
}
#endif

#if DIRTYCODE_LOGGING
/*F*************************************************************************************/
/*!
    \Function _DisplayNameMapEntry

    \Description
        Format debug text for given name map entry.

    \Input *pNameMap    - name map entry

    \Output
        const char *    - output text

    \Version 04/29/2008 (jbrookes)
*/
/************************************************************************************F*/
static const char *_DisplayNameMapEntry(SocketNameMapT *pNameMap)
{
    char strDstAddr[64] = "", strAddrBuf[20];
    static char strOutput[256];

    // resolve address into strings
    if (pNameMap->uRemap != 0)
    {
        strnzcpy(strDstAddr, SocketInAddrGetText(pNameMap->uRemap, strAddrBuf, sizeof(strAddrBuf)), sizeof(strDstAddr));
    }
    if (pNameMap->strRemap[0] != '\0')
    {
        if (strDstAddr[0] != '\0')
        {
            strnzcat(strDstAddr, "/", sizeof(strDstAddr));
        }
        strnzcat(strDstAddr, pNameMap->strRemap, sizeof(strDstAddr));
    }
    if (strDstAddr[0] == '\0')
    {
        // special-case for a blank entry
        return("[no entry]");
    }

    // format output and return to caller
    snzprintf(strOutput, sizeof(strOutput), "[%s:%d] -> [%s:%d]", pNameMap->strDnsName,
        pNameMap->uSrcPort, strDstAddr, pNameMap->uDstPort);
    return(strOutput);
}
#endif

/*F*************************************************************************************/
/*!
    \Function _MapNameMatch

    \Description
        Determine if the given name matches with the given match string

    \Input *pName   - given name
    \Input *pMatch  - match name

    \Output
        uint32_t    - TRUE if a match, else FALSE

    \Notes
        This function allows a single wildcard character (*) as either the first or
        last character in the match string.  Any other wildcards or wildcard locations,
        or a combination of the two wildcard locations, are not supported.

    \Version 03/14/2006 (jbrookes)
*/
/************************************************************************************F*/
static uint32_t _MapNameMatch(const char *pName, const char *pMatch)
{
    const char *pNameEnd, *pTmpName, *pMatchEnd, *pTmpMatch;
    uint32_t uResult;

    // early rejection for empty match string
    if (*pMatch == '\0')
    {
        return(0);
    }

    // point to last character of match string
    pMatchEnd = pMatch + strlen(pMatch)-1;

    // check to see if first character of match string is wildcard
    if (*pMatch == '*')
    {
        // point to last character of name
        pNameEnd = pName + strlen(pName)-1;

        // scan names backwards
        for (pTmpName = pNameEnd, pTmpMatch = pMatchEnd; (pTmpName != pName) && (pTmpMatch != pMatch); pTmpName -= 1, pTmpMatch -= 1)
        {
            if (*pTmpName != *pTmpMatch)
            {
                break;
            }
        }

        // wildcard?
        if (*pTmpMatch == '*')
        {
            pTmpName += 1;
            pTmpMatch += 1;
        }

        uResult = !strcmp(pTmpName, pTmpMatch);
    }
    else if (*pMatchEnd == '*') // is last character wildcarded?
    {
        // compare name and match forwards
        for (pTmpName = pName, pTmpMatch = pMatch; (*pTmpName != '\0') && (*pTmpMatch != '\0'); pTmpName += 1, pTmpMatch += 1)
        {
            if (*pTmpName != *pTmpMatch)
            {
                break;
            }
        }

        // return match result
        uResult = *pTmpMatch == '*' ? 1 : 0;
    }
    else // normal match
    {
        uResult = !strcmp(pName, pMatch);
    }

    return(uResult);
}

/*F*************************************************************************************/
/*!
    \Function _MapName

    \Description
        Remap an incoming dns name to a new name based on mapping table

    \Input *pState          - module state
    \Input *pName           - name

    \Output
        SocketNameMapT *    - pointer to map entry, or NULL if no mapping required

    \Version 03/14/2006 (jbrookes)
*/
/************************************************************************************F*/
static const SocketNameMapT *_MapName(SocketStateT *pState, const char *pName)
{
    const SocketNameMapT *pMap;
    int32_t iMap;

    // see if its mapped
    for (iMap = 0, pMap = NULL; iMap < sizeof(pState->aSockNameMap)/sizeof(pState->aSockNameMap[0]); iMap += 1)
    {
        if (_MapNameMatch(pName, pState->aSockNameMap[iMap].strDnsName))
        {
            #if DIRTYCODE_LOGGING
            char strAddrBuf[20];
            #endif
            pMap = &pState->aSockNameMap[iMap];
            NetPrintf(("dirtynetwin: name remap %s to %s\n", pName,
                (pMap->uRemap != 0) ? SocketInAddrGetText(pMap->uRemap, strAddrBuf, sizeof(strAddrBuf)) : pMap->strRemap));
            break;
        }
    }
    return(pMap);
}


/*F*************************************************************************************/
/*!
    \Function _MapAddress

    \Description
        Remap an incoming IP address to a new address based on mapping table

    \Input *pSocket         - socket this mapping operation is for (can be null)
    \Input *pTemp           - temp sockaddr struct (will get populated if remap needed)
    \Input *pAddr           - target sockaddr for connect / data send (already filled out)

    \Output
        struct sockaddr *   - pointer to pTemp (if mapping needed) or pAddr (if not)

    \Version 1.0 03/24/04 (gschaefer) First Version
*/
/************************************************************************************F*/
static const struct sockaddr *_MapAddress(SocketT *pSocket, struct sockaddr *pTemp, const struct sockaddr *pAddr)
{
    SocketStateT *pState = _Socket_pState;
    const SocketAddrMapT *pMap = pState->aSockAddrMap;
    uint32_t uAddr, uSrcPort, uDstPort;
    const SocketNameMapT *pNameMap;
    char strAddrText[16];
    int32_t iMap;

    // skip remap if socket is marked for insecure use
    if ((pSocket != NULL) && (pSocket->bInsecure == TRUE))
    {
        NetPrintf(("dirtynetwin: skipping address remap as socket is marked for insecure use\n"));
        return(pAddr);
    }

    // get the raw address
    uAddr = SockaddrInGetAddr(pAddr);
    uSrcPort = SockaddrInGetPort(pAddr);

    // see if there is a name map for this address
    SockaddrInGetAddrText((struct sockaddr *)pAddr, strAddrText, sizeof(strAddrText));
    if ((pNameMap = _MapName(pState, strAddrText)) != NULL)
    {
        if (pNameMap->uRemap != 0)
        {
            uAddr = pNameMap->uRemap;
            /* make copy of entry and update address to mapped - note this
               is only required if there is not a subsequent addr map */
            memcpy(pTemp, pAddr, sizeof(*pTemp));
            SockaddrInSetAddr(pTemp, uAddr);
            pAddr = pTemp;
        }
        else
        {
            NetPrintf(("dirtynetwin: warning -- could not map address %s using name map, as remap address is not yet specified\n", strAddrText));
        }
    }

    // see if its mapped
    for (iMap = 0; iMap < sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]); iMap += 1)
    {
        pMap = &pState->aSockAddrMap[iMap];

        // skip empty entries
        if (pMap->uRemap == 0)
        {
            continue;
        }
        // check addr match
        if (pMap->uAddr != (uAddr & pMap->uMask)) // && ((uAddr >> 24) != 0))
        {
            continue;
        }
        // check port match
        if ((pMap->uSrcPort != 0) && (uSrcPort != 0) && (pMap->uSrcPort != uSrcPort))
        {
            continue;
        }

        // figure out dst port
        uDstPort = (pMap->uDstPort != 0) ? pMap->uDstPort : uSrcPort;

        NetPrintf(("dirtynetwin: addr remap %a:%d to %a:%d\n", uAddr, uSrcPort, pMap->uRemap, uDstPort));

        // make copy of entry and update address to mapped
        memcpy(pTemp, pAddr, sizeof(*pTemp));
        SockaddrInSetAddr(pTemp, pMap->uRemap);
        SockaddrInSetPort(pTemp, uDstPort);
        SockaddrInSetMisc(pTemp, pMap->uServiceId);
        pAddr = pTemp;
        break;
    }
    return(pAddr);
}

/*F*************************************************************************************/
/*!
    \Function _RemoveLSPMapping

    \Description
        Remove specified LSP mapping from mapping table.

    \Input *pState  - module state
    \Input uAddress - address to remove from mapping tables

    \Version 11/30/2009 (jbrookes)
*/
/************************************************************************************F*/
#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
static void _RemoveLSPMapping(SocketStateT *pState, uint32_t uAddress)
{
    int32_t iAddrMap, iNameMap;
    uint32_t uFakeAddr;

    // scrub secure address from addr map table
    for (iAddrMap = 0, uFakeAddr = 0; iAddrMap < sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]); iAddrMap += 1)
    {
        SocketAddrMapT *pAddrMap = &pState->aSockAddrMap[iAddrMap];
        if (pAddrMap->uRemap == uAddress)
        {
            // save fake address we generated for this connection
            uFakeAddr = pAddrMap->uAddr;

            // scrub entry
            NetPrintf(("dirtynetwin: scrubbing addr map entry %s\n", _DisplayAddrMapEntry(pAddrMap)));
            memset(pAddrMap, 0, sizeof(*pAddrMap));
        }
    }

    // if we found an address, scrube fake address from name map table
    if (uFakeAddr != 0)
    {
        for (iNameMap = 0; iNameMap < sizeof(pState->aSockNameMap)/sizeof(pState->aSockNameMap[0]); iNameMap += 1)
        {
            SocketNameMapT *pNameMap = &pState->aSockNameMap[iNameMap];
            if (pNameMap->uRemap == uFakeAddr)
            {
                // scrub entry
                NetPrintf(("dirtynetwin: scrubbing name map entry %s\n", _DisplayNameMapEntry(pNameMap)));
                pNameMap->uRemap = 0;
            }
        }
    }
}
#endif

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
/*F*************************************************************************************/
/*!
    \Function _SocketLSPCacheCallback

    \Description
        Handle DirtyLSP cache events

    \Input uAddress     - address for cache event
    \Input eCacheEvent  - kind of cache event
    \Input *pUserRef    - socket module state

    \Version 04/16/2010 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketLSPCacheCallback(uint32_t uAddress, DirtyLSPCacheEventE eCacheEvent, void *pUserRef)
{
    SocketStateT *pState = (SocketStateT *)pUserRef;

    // if DirtyLSP has expired an address, purge it from the addr/name map tables
    if (eCacheEvent == DIRTYLSP_CACHEEVENT_EXPIRED)
    {
        _RemoveLSPMapping(pState, uAddress);
    }
}
#endif

/*F*************************************************************************************/
/*!
    \Function    _XlatError0

    \Description
        Translate a winsock error to dirtysock

    \Input err      - return value from winsock call
    \Input wsaerr   - winsock error (from WSAGetLastError())

    \Output
        int32_t     - dirtysock error

    \Version 09/09/2004 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _XlatError0(int32_t err, int32_t wsaerr)
{
    if (err < 0)
    {
        err = wsaerr;
        if ((err == WSAEWOULDBLOCK) || (err == WSA_IO_PENDING))
            err = SOCKERR_NONE;
        else if ((err == WSAENETUNREACH) || (err == WSAEHOSTUNREACH))
            err = SOCKERR_UNREACH;
        else if (err == WSAENOTCONN)
            err = SOCKERR_NOTCONN;
        else if (err == WSAECONNREFUSED)
            err = SOCKERR_REFUSED;
        else if (err == WSAEINVAL)
            err = SOCKERR_INVALID;
        else if (err == WSAECONNRESET)
            err = SOCKERR_CONNRESET;
        else
            err = SOCKERR_OTHER;
    }
    return(err);
}

/*F*************************************************************************************/
/*!
    \Function    _XlatError

    \Description
        Translate the most recent winsock error to dirtysock

    \Input err      - return value from winsock call

    \Output
        int32_t     - dirtysock error

    \Version 01/02/2002 (gschaefer)
*/
/************************************************************************************F*/
static int32_t _XlatError(int32_t err)
{
    return(_XlatError0(err, WSAGetLastError()));
}

/*F*************************************************************************************/
/*!
    \Function    _SocketOpen

    \Description
        Allocates a SocketT.  If s is INVALID_SOCKET, a WinSock socket ref is created,
        otherwise s is used.

    \Input s        - socket to use, or INVALID_SOCKET
    \Input af       - address family
    \Input type     - type (SOCK_DGRAM, SOCK_STREAM, ...)
    \Input proto    - protocol


    \Output
        SocketT     - pointer to new socket, or NULL

    \Version 03/03/2005 (jbrookes)
*/
/************************************************************************************F*/
static SocketT *_SocketOpen(SOCKET s, int32_t af, int32_t type, int32_t proto)
{
    SocketStateT *pState = _Socket_pState;
    uint32_t uTrue = 1;
    SocketT *sock;

    // open a winsock socket
    if (s == INVALID_SOCKET)
    {
        if ((s = socket(af, type, proto)) == INVALID_SOCKET)
        {
            NetPrintf(("dirtynetwin: error %d creating socket\n", WSAGetLastError()));
            return(NULL);
        }
    }

    // allocate memory
    sock = DirtyMemAlloc(sizeof(*sock), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(sock, 0, sizeof(*sock));
    // save the socket
    sock->socket = s;
    sock->iLastError = SOCKERR_NONE;

    // set to non blocking
    ioctlsocket(s, FIONBIO, (u_long *)&uTrue);
    // if udp, allow broadcast
    if (type == SOCK_DGRAM)
    {
        #if (DIRTYCODE_PLATFORM == DIRTYCODE_XENON)
        // broadcast not supported for VDP
        if (proto != IPPROTO_VDP)
        #endif
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&uTrue, sizeof(uTrue));
    }
    // if raw, set hdrincl
    #if DIRTYCODE_PLATFORM != DIRTYCODE_XENON
    if (type == SOCK_RAW)
    {
        setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *)&uTrue, sizeof(uTrue));
    }
    #endif

    // set family/proto info
    sock->family = af;
    sock->type = type;
    sock->proto = proto;

    #if SOCKET_ASYNCRECVTHREAD
    // create overlapped i/o event object
    memset(&sock->overlapped, 0, sizeof(sock->overlapped));
    sock->overlapped.hEvent = WSACreateEvent();

    // inititalize receive critical section
    NetCritInit(&sock->recvcrit, "recvthread");
    #endif

    // install into list
    NetCritEnter(NULL);
    sock->next = pState->pSockList;
    pState->pSockList = sock;
    NetCritLeave(NULL);

    // return the socket
    return(sock);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketClose

    \Description
        Disposes of a SocketT, including removal from the global socket list and
        disposal of the SocketT allocated memory.  Does NOT dispose of the winsock
        socket ref.

    \Input *sock        - socket to close
    \Input bShutdown    - if TRUE, shutdown and close socket ref

    \Output
        int32_t         - negative=failure, zero=success

    \Version 01/14/2005 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketClose(SocketT *sock, uint32_t bShutdown)
{
    SocketStateT *pState = _Socket_pState;
    unsigned char bSockInList = FALSE;
    SocketT **psock;

    // for access to g_socklist and g_sockkill
    NetCritEnter(NULL);

    // remove sock from linked list
    for (psock = &pState->pSockList; *psock != NULL; psock = &(*psock)->next)
    {
        if (*psock == sock)
        {
            *psock = sock->next;
            bSockInList = TRUE;
            break;
        }
    }

    // release before NetIdleDone
    NetCritLeave(NULL);

    // make sure the socket is in the socket list (and therefore valid)
    if (bSockInList == FALSE)
    {
        NetPrintf(("dirtynetwin: warning, trying to close socket 0x%08x that is not in the socket list\n", (uintptr_t)sock));
        return(-1);
    }

    // finish any idle call
    NetIdleDone();

    #if SOCKET_ASYNCRECVTHREAD
    // acquire global critical section
    NetCritEnter(NULL);

    // wake up out of WaitForMultipleEvents()
    WSASetEvent(pState->iEvent);

    // acquire event critical section
    NetCritEnter(&pState->EventCrit);

    // close event
    WSACloseEvent(sock->overlapped.hEvent);
    sock->overlapped.hEvent = WSA_INVALID_EVENT;

    // release event critical section
    NetCritLeave(&pState->EventCrit);

    // release global critical section
    NetCritLeave(NULL);
    #endif

    // mark as closed
    if ((bShutdown == TRUE) && (sock->socket != INVALID_SOCKET))
    {
        // close winsock socket
        shutdown(sock->socket, 2);
        closesocket(sock->socket);
    }
    sock->socket = INVALID_SOCKET;
    sock->opened = 0;

    // put into killed list
    /*
    Usage of a kill list allows for postponing two things
        * destruction of recvcrit
        * release of socket data structure memory
    This ensures that recvcrit is not freed while in-use by a running thread.
    Such a scenario can occur when the receive callback invoked by _SocketRecvThread()
    (while recvcrit is entered) calls _SocketClose()
    */
    NetCritEnter(NULL);
    sock->kill = pState->pSockKill;
    pState->pSockKill = sock;
    NetCritLeave(NULL);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketIdle

    \Description
        Call idle processing code to give connections time.

    \Input _pState  - module state

    \Version 10/15/1999 (gschaefer)
*/
/************************************************************************************F*/
static void _SocketIdle(void *_pState)
{
    SocketStateT *pState = (SocketStateT *)_pState;
    SocketT *sock;
    uint32_t tick = NetTick();

    // for access to g_socklist and g_sockkill
    NetCritEnter(NULL);

    // walk socket list and perform any callbacks
    for (sock = pState->pSockList; sock != NULL; sock = sock->next)
    {
        // see if we should do callback
        if ((sock->callidle != 0) && (sock->callback != NULL) && (sock->calllast != (unsigned)-1) && (tick-sock->calllast > sock->callidle))
        {
            sock->calllast = (unsigned)-1;
            (sock->callback)(sock, 0, sock->callref);
            sock->calllast = tick = NetTick();
        }
    }

    // delete any killed sockets
    while ((sock = pState->pSockKill) != NULL)
    {
        pState->pSockKill = sock->kill;

        // release the socket's receive critical section
        NetCritKill(&sock->recvcrit);

        // free the socket memory
        DirtyMemFree(sock, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    // for access to g_socklist and g_sockkill
    NetCritLeave(NULL);
}

#if SOCKET_ASYNCRECVTHREAD
/*F*************************************************************************************/
/*!
    \Function    _SocketRecvData

    \Description
        Called when data is received by _SocketRecvThread().  If there is a callback
        registered than the socket is passed to that callback so the data may be
        consumed.

    \Input *pSocket - pointer to socket that has new data

    \Version 08/30/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvData(SocketT *pSocket)
{
    // save receive timestamp
    if (pSocket->type != SOCK_STREAM)
    {
        SockaddrInSetMisc(&pSocket->recvaddr, NetTick());
    }

    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    // update xlsp connection
    DirtyLSPConnectionUpdate(SockaddrInGetAddr(&pSocket->recvaddr));
    #endif

    // see if we should issue callback
    if ((pSocket->calllast != (unsigned)-1) && (pSocket->callback != NULL) && (pSocket->callmask & CALLB_RECV))
    {
        pSocket->calllast = (unsigned)-1;
        (pSocket->callback)(pSocket, 0, pSocket->callref);
        pSocket->calllast = NetTick();
    }
}

/*F*************************************************************************************/
/*!
    \Function    _SocketRead

    \Description
        Issue an overlapped read call on the given socket.

    \Input *pSocket - pointer to socket to read from

    \Version 08/30/2004 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketRead(SocketT *pSocket)
{
    int32_t iResult, iRecvStat = 0;
    WSABUF recvbuf;

    // if we already have data or a read is already in progress, don't issue another
    if ((pSocket->recvstat != 0) || (pSocket->recvinp == 1) || (pSocket->bVirtual == TRUE))
    {
        return(0);
    }

    // set up for recv call
    recvbuf.buf = pSocket->recvdata;
    recvbuf.len = sizeof(pSocket->recvdata);
    pSocket->recvflag = 0;

    // try and receive some data
    if (pSocket->type == SOCK_DGRAM)
    {
        pSocket->addrlen = sizeof(pSocket->recvaddr);
        iResult = WSARecvFrom(pSocket->socket, &recvbuf, 1, (LPDWORD)&pSocket->recvstat, (LPDWORD)&pSocket->recvflag, &pSocket->recvaddr, (LPINT)&pSocket->addrlen, &pSocket->overlapped, NULL);
    }
    else // pSocket->type == SOCK_RAW
    {
        iResult = WSARecv(pSocket->socket, &recvbuf, 1, (LPDWORD)&pSocket->recvstat, (LPDWORD)&pSocket->recvflag, &pSocket->overlapped, NULL);
    }

    // error?
    if (iResult != 0)
    {
        // save error result and winsock error
        pSocket->recvstat = iResult;
        if ((pSocket->recverr = WSAGetLastError()) == WSA_IO_PENDING)
        {
            // remember a receive operation is pending
            pSocket->recvinp = 1;
        }
        else
        {
            NetPrintf(("dirtynetwin: error %d trying to receive from socket %d\n", pSocket->recverr, pSocket->socket));
        }
    }
    else if ((iRecvStat = pSocket->recvstat) > 0)
    {
        // if the read completed immediately, forward data to socket callback
        _SocketRecvData(pSocket);
    }

    return(iRecvStat);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketRecvThread

    \Description
        Wait for incoming data and deliver it immediately to the registered socket callback,
        if any.

    \Input  uUnused - unused

    \Version 08/30/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvThread(uint32_t uUnused)
{
    SocketStateT *pState = _Socket_pState;
    WSAEVENT aEventList[SOCKET_MAXEVENTS];
    int32_t iNumEvents, iResult;
    SocketT *pSocket;

    // show we are alive
    NetPrintf(("dirtynetwin: receive thread started (thid=%d)\n", GetCurrentThreadId()));

    // clear event list
    memset(aEventList, 0, sizeof(aEventList));

    // loop until done
    while (pState->hRecvThread != 0)
    {
        // acquire global critical section for access to g_socklist
        NetCritEnter(NULL);

        // add global event to list
        iNumEvents = 0;
        aEventList[iNumEvents++] = pState->iEvent;

        // walk the socket list and do two things:
        //    1- if the socket is ready for reading, perform the read operation
        //    2- if the buffer in which inbound data is saved is empty, initiate a new low-level read operation for that socket
        for (pSocket = pState->pSockList; (pSocket != NULL) && (iNumEvents < SOCKET_MAXEVENTS); pSocket = pSocket->next)
        {
            // only handle non-virtual SOCK_DGRAM and non-virtual SOCK_RAW
            if ((pSocket->bVirtual == FALSE) && (pSocket->socket != INVALID_SOCKET) &&
                ((pSocket->type == SOCK_DGRAM) || (pSocket->type == SOCK_RAW)))
            {
                // acquire socket critical section
                NetCritEnter(&pSocket->recvcrit);

                // if no data is queued, check for overlapped read completion
                if (pSocket->recvstat <= 0)
                {
                    // is a read in progress on this socket?
                    if (pSocket->recvinp == 1)
                    {
                        // check for overlapped read completion
                        if (WSAGetOverlappedResult(pSocket->socket, &pSocket->overlapped, (LPDWORD)&pSocket->recvstat, FALSE, (LPDWORD)&pSocket->recvflag) == TRUE)
                        {
                            // mark that receive operation is no longer in progress
                            pSocket->recvinp = 0;

                            // we've got data waiting on this socket
                            _SocketRecvData(pSocket);
                        }
                        else if ((iResult = WSAGetLastError()) != WSA_IO_INCOMPLETE)
                        {
                            #if DIRTYCODE_LOGGING
                            if (iResult != WSAECONNRESET)
                            {
                                NetPrintf(("dirtynetwin: WSAGetOverlappedResult error %d\n", iResult));
                            }
                            #endif

                            // since receive operation failed, mark as not in progress
                            pSocket->recvinp = 0;
                        }
                    }

                    /*
                    Before proceeding, make sure that the user callback invoked in _SockRecvData() did not close the socket with SocketClose().
                    We know that SocketClose() function resets the value of pSocket->socket to INVALID_SOCKET. Also, we know that it
                    does not destroy pSocket but it queues it in the global kill list. Since this list cannot be processed before the code
                    below as it also runs in the context of the global critical section, the following code is thread-safe.
                    */
                    if (pSocket->socket != INVALID_SOCKET)
                    {
                        // try and read some data
                        _SocketRead(pSocket);

                        // Before proceeding, make sure that user callback invoked in _SockRead() did not close the socket.
                        // Same rational as previous comment.
                        if (pSocket->socket != INVALID_SOCKET)
                        {
                            // add event to event list array
                            aEventList[iNumEvents++] = pSocket->overlapped.hEvent;
                        }
                    }
                }

                // release socket critical section
                NetCritLeave(&pSocket->recvcrit);
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
    NetPrintf(("dirtynetwin: receive thread exit\n"));
    pState->iRecvLife = 0;
}
#endif // SOCKET_ASYNCRECVTHREAD


/*** Public Functions ******************************************************************/


/*F*************************************************************************************/
/*!
    \Function    SocketCreate

    \Description
        Create new instance of socket interface module.  Initializes all global
        resources and makes module ready for use.

    \Input  iThreadPrio - thread priority to start NetLib thread with

    \Output
        int32_t         - negative=error, zero=success

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketCreate(int32_t iThreadPrio)
{
    SocketStateT *pState = _Socket_pState;
    WSADATA data;
    int32_t iResult;
    #if SOCKET_ASYNCRECVTHREAD
    int32_t pid;
    #endif
    int32_t iMemGroup;
    void *pMemGroupUserData;
#if defined(_WIN64)
    uint64_t tempHandle = 1;
#else
    uint32_t tempHandle = 1;
#endif

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintf(("dirtynetwin: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetwin: DirtySock SDK v%d.%d.%d.%d\n",
        (DIRTYVERS>>24)&0xff, (DIRTYVERS>>16)&0xff,
        (DIRTYVERS>> 8)&0xff, (DIRTYVERS>> 0)&0xff));

    // alloc and init state ref
    if ((pState = DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetpc: unable to allocate module state\n"));
        return(-2);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iMaxPacket = SOCKET_MAXUDPRECV;

    // save global module ref
    _Socket_pState = pState;

    // startup network libs
    NetLibCreate(iThreadPrio);

    // add our idle handler
    NetIdleAdd(&_SocketIdle, pState);

    // start winsock
    memset(&data, 0, sizeof(data));
    #if DIRTYCODE_PLATFORM != DIRTYCODE_XENON
    // startup winsock (would like 2.0 if available)
    iResult = WSAStartup(MAKEWORD(2, 0), &data);
    #else
    /* Standard WinSock startup. The Xbox allows all versions of Winsock
       up through 2.2 (i.e. 1.0, 1.1, 2.0, 2.1, and 2.2), although it
       technically supports only and exactly what is specified in the
       Xbox network documentation, not necessarily the full Winsock
       functional specification. */
    iResult = WSAStartup(MAKEWORD(2,2), &data);
    #endif

    if (iResult != 0)
    {
        NetPrintf(("dirtynetwin: error %d loading winsock library\n", iResult));
        SocketDestroy((uint32_t)(-1));
        return(-3);
    }

    // save the available version
    pState->iVersion = (LOBYTE(data.wVersion)<<8)|(HIBYTE(data.wVersion)<<0);

    #if SOCKET_ASYNCRECVTHREAD
    // create a global event to notify recv thread of newly available sockets
    pState->iEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == pState->iEvent)
    {
        NetPrintf(("dirtynetwin: error %d creating state event\n", GetLastError()));
        SocketDestroy(0);
        return(-4);
    }

    // initialize event critical section
    NetCritInit(&pState->EventCrit, "recv-event-crit");

    // single g_recvthread is also the "quit flag", set to non-zero
    // before create to avoid bogus exit
    pState->hRecvThread = (HANDLE)tempHandle;

    // if recvthread has been created but has no chance to run (upon failure or shutdown immediatelly), there will be a problem.
    // by setting iRecvLife to non-zero before thread creation that make sure SocketDestroy work properly.
    pState->iRecvLife = 1; // see _SocketRecvThread for more details

    // start up socket receive thread
    if ((pState->hRecvThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_SocketRecvThread, NULL, 0, (LPDWORD)&pid)) != NULL)
    {
        // crank priority since we deal with real-time events
        if (SetThreadPriority(pState->hRecvThread, iThreadPrio) == FALSE)
        {
            NetPrintf(("dirtynetwin: error %d adjusting thread priority\n", GetLastError()));
        }

        // we dont need thread handle any more
        CloseHandle(pState->hRecvThread);
    }
    else
    {
        pState->iRecvLife = 0; // no recvthread was created, reset to 0
        NetPrintf(("dirtynetwin: error %d creating socket receive thread\n", GetLastError()));
        SocketDestroy(0);
        return(-5);
    }
    #endif

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    SocketDestroy

    \Description
        Release resources and destroy module.

    \Input uShutdownFlags   - shutdown flags

    \Output
        int32_t             - negative=error, zero=success

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketDestroy(uint32_t uShutdownFlags)
{
    SocketStateT *pState = _Socket_pState;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtynetpc: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    NetPrintf(("dirtynetpc: shutting down\n"));

    // kill idle callbacks
    NetIdleDel(&_SocketIdle, pState);

    // let any idle event finish
    NetIdleDone();

    #if SOCKET_ASYNCRECVTHREAD
    // tell receive thread to quit and wake it up
    pState->hRecvThread = 0;
    if (pState->iEvent != NULL)
    {
        WSASetEvent(pState->iEvent);
    }

    // wait for thread to terminate
    while (pState->iRecvLife > 0)
    {
        Sleep(1);
    }
    #endif

    // close all sockets
    while (pState->pSockList != NULL)
    {
        SocketClose(pState->pSockList);
    }

    // clear the kill list
    _SocketIdle(pState);

    #if SOCKET_ASYNCRECVTHREAD
    // we rely on the Handle having been created to know if the critical section was created.
    if (pState->iEvent != NULL)
    {
        // destroy event critical section
        NetCritKill(&pState->EventCrit);
        // delete global event
        CloseHandle(pState->iEvent);
    }
    #endif

    // free the memory and clear global module ref
    _Socket_pState = NULL;
    DirtyMemFree(pState, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

    // shut down network libs
    NetLibDestroy();

    // shutdown winsock, unless told otherwise
    if (uShutdownFlags == 0)
    {
        WSACleanup();
    }

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    SocketOpen

    \Description
        Create a new transfer endpoint. A socket endpoint is required for any
        data transfer operation.

    \Input af       - address family (AF_INET)
    \Input type     - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input proto    - protocol type for SOCK_RAW (unused by others)

    \Output
        SocketT     - socket reference

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
SocketT *SocketOpen(int32_t af, int32_t type, int32_t proto)
{
    return(_SocketOpen(INVALID_SOCKET, af, type, proto));
}

/*F*************************************************************************************/
/*!
    \Function    SocketClose

    \Description
        Close a socket. Performs a graceful shutdown of connection oriented protocols.

    \Input *sock    - socket reference

    \Output
        int32_t     - zero

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketClose(SocketT *sock)
{
    return(_SocketClose(sock, TRUE));
}

/*F*************************************************************************************/
/*!
    \Function    SocketImport

    \Description
        Import a socket.  The given socket ref may be a SocketT, in which case a
        SocketT pointer to the ref is returned, or it can be an actual Sony socket ref,
        in which case a SocketT is created for the Sony socket ref.

    \Input uSockRef     - socket reference

    \Output
        SocketT *       - pointer to imported socket, or NULL

    \Version 01/14/2005 (jbrookes)
*/
/************************************************************************************F*/
SocketT *SocketImport(intptr_t uSockRef)
{
    SocketStateT *pState = _Socket_pState;
    SocketT *pSock;
    int32_t proto, protosize;

    // see if this socket is already in our socket list
    NetCritEnter(NULL);
    for (pSock = pState->pSockList; pSock != NULL; pSock = pSock->next)
    {
        if (pSock == (SocketT *)uSockRef)
        {
            break;
        }
    }
    NetCritLeave(NULL);

    // if socket is in socket list, just return it
    if (pSock != NULL)
    {
        return(pSock);
    }

    // get info from socket ref
    protosize = sizeof(proto);
    if (getsockopt((SOCKET)uSockRef, 0, SO_TYPE, (char *)&proto, &protosize) != SOCKET_ERROR)
    {
        // create the socket (note: winsock socket types directly map to dirtysock socket types)
        pSock = _SocketOpen(uSockRef, AF_INET, proto, 0);

        // update local and remote addresses
        SocketInfo(pSock, 'bind', 0, &pSock->local, sizeof(pSock->local));
        SocketInfo(pSock, 'peer', 0, &pSock->remote, sizeof(pSock->remote));

        // mark it as imported
        pSock->imported = 1;
    }

    return(pSock);
}

/*F*************************************************************************************/
/*!
    \Function SocketRelease

    \Description
        Release an imported socket.

    \Input *pSock   - pointer to socket

    \Version 01/14/2005 (jbrookes)
*/
/************************************************************************************F*/
void SocketRelease(SocketT *pSock)
{
    // if it wasn't imported, nothing to do
    if (pSock->imported == FALSE)
    {
        return;
    }

    // dispose of SocketT, but leave the sockref alone
    _SocketClose(pSock, FALSE);
}

/*F*************************************************************************************/
/*!
    \Function    SocketShutdown

    \Description
        Perform partial/complete shutdown of socket indicating that either sending
        and/or receiving is complete.

    \Input *pSocket - socket reference
    \Input iHow     - SOCK_NOSEND and/or SOCK_NORECV

    \Output
        int32_t     - zero

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketShutdown(SocketT *pSocket, int32_t iHow)
{
    // no shutdown for an invalid socket
    if (pSocket->socket == INVALID_SOCKET)
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

    pSocket->shutdown |= iHow;
    shutdown(pSocket->socket, iHow);

    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    SocketBind

    \Description
        Bind a local address/port to a socket.

    \Input *sock    - socket reference
    \Input *name    - local address/port
    \Input namelen  - length of name

    \Output
        int32_t         - standard network error code (SOCKERR_xxx)

    \Notes
        If either address or port is zero, then they are filled in automatically.

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketBind(SocketT *sock, const struct sockaddr *name, int32_t namelen)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr BindAddr;
    int32_t iResult;

    // bind to specific address?
    if ((SockaddrInGetAddr(name) == 0) && (pState->uAdapterAddress != 0))
    {
        memcpy(&BindAddr, name, sizeof(BindAddr));
        SockaddrInSetAddr(&BindAddr, pState->uAdapterAddress);
        name = &BindAddr;
    }

    // is the bind port a virtual port?
    if (sock->type == SOCK_DGRAM)
    {
        int32_t iPort;
        uint16_t uPort;

        if ((uPort = SockaddrInGetPort(name)) != 0)
        {
            // find virtual port in list
            for (iPort = 0; (iPort < SOCKET_MAXVIRTUALPORTS) && (pState->aVirtualPorts[iPort] != uPort); iPort++)
                ;
            if (iPort < SOCKET_MAXVIRTUALPORTS)
            {
                // close winsock socket
                NetPrintf(("dirtynetwin: making socket bound to port %d virtual\n", uPort));
                if (sock->socket != INVALID_SOCKET)
                {
                    shutdown(sock->socket, SOCK_NOSEND);
                    closesocket(sock->socket);
                    sock->socket = INVALID_SOCKET;
                }
                sock->virtualport = uPort;
                sock->bVirtual = TRUE;
                return(0);
            }
        }
    }

    // execute the bind
    iResult = _XlatError(bind(sock->socket, name, namelen));

    #if SOCKET_ASYNCRECVTHREAD
    // notify read thread that socket is ready to be read from
    if ((iResult == SOCKERR_NONE) && ((sock->type == SOCK_DGRAM) || (sock->type == SOCK_RAW)))
    {
        WSASetEvent(pState->iEvent);
    }
    #endif

    // return result to caller
    sock->iLastError = iResult;
    return(sock->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketConnect

    \Description
        Initiate a connection attempt to a remote host.

    \Input *sock    - socket reference
    \Input *name    - pointer to name of socket to connect to
    \Input namelen  - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        Only has real meaning for stream protocols. For a datagram protocol, this
        just sets the default remote host.

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketConnect(SocketT *sock, struct sockaddr *name, int32_t namelen)
{
    SocketStateT *pState = _Socket_pState;
    const struct sockaddr *pRemap;
    struct sockaddr temp;
    int32_t iResult;

    // mark as not open
    sock->opened = 0;

    SockaddrInit(&temp, AF_INET);

    /* execute an explicit bind - this allows us to specify a non-zero local address
       or port (see SocketBind()).  a SOCKERR_INVALID result here means the socket has
       already been bound, so we ignore that particular error */
    if (((iResult = SocketBind(sock, &temp, sizeof(temp))) < 0) && (iResult != SOCKERR_INVALID))
    {
        sock->iLastError = iResult;
        return(sock->iLastError);
    }

    // map the address
    pRemap = _MapAddress(sock, &temp, name);

    // execute the connect
    iResult = _XlatError(connect(sock->socket, pRemap, namelen));

    // save connect address
    memcpy(&sock->remote, pRemap, sizeof(sock->remote));

    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    DirtyLSPConnectionUpdate(SockaddrInGetAddr(&sock->remote));
    #endif

    #if SOCKET_ASYNCRECVTHREAD
    // notify read thread that socket is ready to be read from
    if ((iResult == SOCKERR_NONE) && ((sock->type == SOCK_DGRAM) || (sock->type == SOCK_RAW)))
    {
        WSASetEvent(pState->iEvent);
    }
    #endif

    // return result to caller
    sock->iLastError = iResult;
    return(sock->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketListen

    \Description
        Start listening for an incoming connection on the socket.  The socket must already
        be bound and a stream oriented connection must be in use.

    \Input *sock    - socket reference to bound socket (see SocketBind())
    \Input backlog  - number of pending connections allowed

    \Output
        int32_t         - standard network error code (SOCKERR_xxx)

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketListen(SocketT *sock, int32_t backlog)
{
    // do the listen
    sock->iLastError = _XlatError(listen(sock->socket, backlog));
    return(sock->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketAccept

    \Description
        Accept an incoming connection attempt on a socket.

    \Input *sock    - socket reference to socket in listening state (see SocketListen())
    \Input *addr    - pointer to storage for address of the connecting entity, or NULL
    \Input *addrlen - pointer to storage for length of address, or NULL

    \Output
        SocketT *   - the accepted socket, or NULL if not available

    \Notes
        The integer pointed to by addrlen should on input contain the number of characters
        in the buffer addr.  On exit it will contain the number of characters in the
        output address.

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
SocketT *SocketAccept(SocketT *sock, struct sockaddr *addr, int32_t *addrlen)
{
    SocketT *open = NULL;
    SOCKET incoming;

    // see if already connected
    if (sock->socket == INVALID_SOCKET)
    {
        return(NULL);
    }

    // make sure turn parm is valid
    if ((addr != NULL) && (*addrlen < sizeof(struct sockaddr)))
    {
        return(NULL);
    }

    // perform inet accept
    if (sock->family == AF_INET)
    {
        incoming = accept(sock->socket, addr, addrlen);
        if (incoming != INVALID_SOCKET)
        {
            open = _SocketOpen(incoming, sock->family, sock->type, sock->proto);
        }
    }

    // return the socket
    return(open);
}

/*F*************************************************************************************/
/*!
    \Function    SocketSendto

    \Description
        Send data to a remote host. The destination address is supplied along with
        the data. Should only be used with datagram sockets as stream sockets always
        send to the connected peer.

    \Input *pSocket - socket reference
    \Input *buf     - the data to be sent
    \Input len      - size of data
    \Input flags    - unused
    \Input *to      - the address to send to (NULL=use connection address)
    \Input tolen    - length of address

    \Output
        int32_t         - standard network error code (SOCKERR_xxx)

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketSendto(SocketT *pSocket, const char *buf, int32_t len, int32_t flags, const struct sockaddr *to, int32_t tolen)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr temp;
    int32_t retn;

    // if installed, give socket callback right of first refusal
    if (pState->pSendCallback != NULL)
    {
        if ((retn = pState->pSendCallback(pSocket, pSocket->type, (const uint8_t *)buf, len, to, pState->pSendCallref)) > 0)
        {
            return(retn);
        }
    }

    // make sure socket ref is valid
    if (pSocket->socket == INVALID_SOCKET)
    {
        NetPrintf(("dirtynetwin: attempting to send on invalid socket\n"));
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // use appropriate version
    if (to == NULL)
    {
        retn = send(pSocket->socket, buf, len, 0);
        to = &pSocket->remote;
    }
    else
    {
        to = _MapAddress(pSocket, &temp, to);
        retn = sendto(pSocket->socket, buf, len, 0, to, tolen);
    }

    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    DirtyLSPConnectionUpdate(SockaddrInGetAddr(to));
    #endif

    // return bytes sent
    pSocket->iLastError = _XlatError(retn);
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketRecvfrom

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

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketRecvfrom(SocketT *pSock, char *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pFrom, int32_t *pFromLen)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iRecv = 0, iRecvErr;

    #if SOCKET_ASYNCRECVTHREAD
    // if DGRAM or RAW, receive thread does the receiving
    if ((pSock->type == SOCK_DGRAM) || (pSock->type == SOCK_RAW))
    {
        // acquire socket receive critical section
        NetCritEnter(&pSock->recvcrit);

        // get the data
        if (((iRecv = pSock->recvstat) != 0) && (iLen > 0))
        {
            if (pFrom != NULL)
            {
                memcpy(pFrom, &pSock->recvaddr, sizeof(*pFrom));
                *pFromLen = sizeof(*pFrom);
            }

            if (iRecv > 0)
            {
                if (iRecv > iLen)
                {
                    iRecv = iLen;
                }
                memcpy(pBuf, pSock->recvdata, iRecv);
                pSock->recverr = 0;
            }

            // mark data as consumed
            pSock->recvstat = 0;
        }

        // wake up receive thread?
        if (pSock->recverr != WSA_IO_PENDING)
        {
            pSock->recverr = 0;
            WSASetEvent(pState->iEvent);
        }

        // get most recent socket error
        iRecvErr = pSock->recverr;

        // release socket receive critical section
        NetCritLeave(&pSock->recvcrit);
    }
    else
    #else
    // first check for pushed data
    if (pSock->recvstat != 0)
    {
        if (pFrom != NULL)
        {
            memcpy(pFrom, &pSock->recvaddr, sizeof(*pFrom));
        }
        if (iLen > pSock->recvstat)
        {
            iLen = pSock->recvstat;
        }
        memcpy(pBuf, pSock->recvdata, iLen);
        pSock->recvstat = 0;
        return(iLen);
    }
    #endif
    {
        // make sure socket ref is valid
        if (pSock->socket == INVALID_SOCKET)
        {
            pSock->iLastError = SOCKERR_INVALID;
            return(pSock->iLastError);
        }

        // socket not handled by async recv thread, so read directly here
        if (pFrom != NULL)
        {
            if ((iRecv = recvfrom(pSock->socket, pBuf, iLen, 0, pFrom, pFromLen)) > 0)
            {
                SockaddrInSetMisc(pFrom, NetTick());
            }
        }
        else
        {
            iRecv = recv(pSock->socket, pBuf, iLen, 0);
            pFrom = &pSock->remote;
        }

        // update xlsp connection
        #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
        if (iRecv > 0)
        {
            DirtyLSPConnectionUpdate(SockaddrInGetAddr(pFrom));
        }
        #endif

        // get most recent socket error
        iRecvErr = WSAGetLastError();
    }

    // do error conversion
    iRecv = (iRecv == 0) ? SOCKERR_CLOSED : _XlatError0(iRecv, iRecvErr);

    // simulate packet loss
    #if DIRTYCODE_DEBUG
    if ((_Socket_pState->uPacketLoss != 0) && (pSock->type == SOCK_DGRAM) && (iRecv > 0) && (SocketSimulatePacketLoss(_Socket_pState->uPacketLoss) != 0))
    {
        pSock->iLastError = SOCKERR_NONE;
        return(pSock->iLastError);
    }
    #endif

    // return the error code
    pSock->iLastError = iRecv;
    return(pSock->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketInfo

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
            'ladr' - get local address previously specified by user for subsequent SocketBind() operations
            'maxp' - return max packet size
            'peer' - peer info (only valid if connected)
            'sdcf' - get installed global send callback function pointer
            'sdcu' - get installed global send callback userdata pointer
            'serr' - last socket error
            'sock' - return windows socket associated with the specified DirtySock socket
            'stat' - TRUE if connected, else FALSE
            'virt' - TRUE if socket is virtual, else FALSE
            'xapr' - display addr map (DEBUG ONLY)
            'xmap' - copy address remap table to output buffer
            'xnam' - copy name remap table to output buffer
            'xnpr' - display name map (DEBUG ONLY)
        \endverbatim

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketInfo(SocketT *pSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    SocketStateT *pState = _Socket_pState;

    // always zero results by default
    if ( (pBuf != NULL) && (iInfo != 'ladr') )
    {
        memset(pBuf, 0, iLen);
    }

    // handle global socket options
    if (pSocket == NULL)
    {
        if (iInfo == 'addr')
        {
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
            for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->next)
            {
                // if iInfo is 'bndu', only consider sockets of type SOCK_DGRAM
                // note: 'bndu' stands for "bind udp"
                if ( (iInfo == 'bind') || ((iInfo == 'bndu') && (pSocket->type == SOCK_DGRAM)) )
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
        // get local address previously specified by user for subsequent SocketBind() operations
        if (iInfo == 'ladr')
        {
            // If 'ladr' had not been set previously, address field of output sockaddr buffer
            // will just be filled with 0.
            SockaddrInSetAddr((struct sockaddr *)pBuf, pState->uAdapterAddress);
            return(0);
        }
        // return max packet size
        if (iInfo == 'maxp')
        {
            return(pState->iMaxPacket);
        }

        // get global send callback function pointer
        if (iInfo == 'sdcf')
        {
            if ((pBuf != NULL) && (iLen == sizeof(pState->pSendCallback)))
            {
                memcpy(pBuf, &pState->pSendCallback, sizeof(pState->pSendCallback));
                return(0);
            }
        }
        // get global send callback user data pointer
        if (iInfo == 'sdcu')
        {
            if ((pBuf != NULL) && (iLen == sizeof(pState->pSendCallref)))
            {
                memcpy(pBuf, &pState->pSendCallref, sizeof(pState->pSendCallref));
                return(0);
            }
        }

        // copy address remap table to output buffer
        if ((iInfo == 'xmap') && (pBuf != NULL) && (iLen > 0))
        {
            if (iLen > sizeof(pState->aSockAddrMap))
            {
                iLen = sizeof(pState->aSockAddrMap);
            }
            memcpy(pBuf, pState->aSockAddrMap, iLen);
            return(0);
        }
        // copy name remap table to output buffer
        if ((iInfo == 'xnam') && (pBuf != NULL) && (iLen > 0))
        {
            if (iLen > sizeof(pState->aSockNameMap))
            {
                iLen = sizeof(pState->aSockNameMap);
            }
            memcpy(pBuf, pState->aSockNameMap, iLen);
            return(0);
        }

        // debug-only selectors to dump addr/name tables, respectively
        #if DIRTYCODE_LOGGING
        if (iInfo == 'xapr')
        {
            int32_t iMapEntry;
            NetPrintf(("dirtynetwin: addr mapping table\n"));
            for (iMapEntry = 0; iMapEntry < sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]); iMapEntry++)
            {
                NetPrintf(("dirtynetwin: [%2d] %s\n", iMapEntry, _DisplayAddrMapEntry(&pState->aSockAddrMap[iMapEntry])));
            }
            return(0);
        }
        if (iInfo == 'xnpr')
        {
            int32_t iMapEntry;
            NetPrintf(("dirtynetwin: name mapping table\n"));
            for (iMapEntry = 0; iMapEntry < sizeof(pState->aSockNameMap)/sizeof(pState->aSockNameMap[0]); iMapEntry++)
            {
                NetPrintf(("dirtynetwin: [%2d] %s\n", iMapEntry, _DisplayNameMapEntry(&pState->aSockNameMap[iMapEntry])));
            }
            return(0);
        }
        #endif

        // not handled
        return(-1);
    }

    // return local bind data
    if (iInfo == 'bind')
    {
        if (pSocket->bVirtual == TRUE)
        {
            SockaddrInit((struct sockaddr *)pBuf, AF_INET);
            SockaddrInSetPort((struct sockaddr *)pBuf, pSocket->virtualport);
        }
        else
        {
            getsockname(pSocket->socket, pBuf, &iLen);

            // for Xenon, getsockname() does not fill in the local address, so we have to do that ourselves
            #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
            if ((pSocket->type == SOCK_STREAM) && (SockaddrInGetPort((struct sockaddr *)pBuf) != 0))
            {
                XNADDR XnAddr;
                XNetGetTitleXnAddr(&XnAddr);
                SockaddrInSetAddr((struct sockaddr *)pBuf, SocketNtohl(XnAddr.ina.s_addr));
            }
            #endif
        }

        return(0);
    }

    // return socket protocol
    if (iInfo == 'prot')
    {
        return(pSocket->proto);
    }

    // return whether the socket is virtual or not
    if (iInfo == 'virt')
    {
        return(pSocket->bVirtual);
    }

    // make sure we are connected
    if (pSocket->socket == INVALID_SOCKET)
    {
        pSocket->iLastError = SOCKERR_OTHER;
        return(pSocket->iLastError);
    }

    // return peer info (only valid if connected)
    if ((iInfo == 'conn') || (iInfo == 'peer'))
    {
        getpeername(pSocket->socket, pBuf, &iLen);
        return(0);
    }

    // return last socket error
    if (iInfo == 'serr')
    {
        return(pSocket->iLastError);
    }

    // return windows socket identifier
    if (iInfo == 'sock')
    {
        return(pSocket->socket);
    }

    // return socket status
    if (iInfo == 'stat')
    {
        int32_t iErr;
        fd_set fdwrite;
        fd_set fdexcept;
        struct timeval tv;
        struct sockaddr peeraddr;

        // if not connected, use select to determine connect
        if (pSocket->opened == 0)
        {
            // setup write/exception lists so we can select against the socket
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcept);
            FD_SET(pSocket->socket, &fdwrite);
            FD_SET(pSocket->socket, &fdexcept);
            tv.tv_sec = tv.tv_usec = 0;
            if (select(1, NULL, &fdwrite, &fdexcept, &tv) != 0)
            {
                // if we got an exception, that means connect failed
                if (fdexcept.fd_count > 0)
                {
                    NetPrintf(("dirtynetwin: connection failed\n"));
                    pSocket->opened = -1;
                }
                // if socket is writable, that means connect succeeded
                else if (fdwrite.fd_count > 0)
                {
                    NetPrintf(("dirtynetwin: connection open\n"));
                    pSocket->opened = 1;
                }
            }
        }

        // if previously connected, make sure connect still valid
        if (pSocket->opened > 0)
        {
            iLen = sizeof(peeraddr);
            iErr = _XlatError(getpeername(pSocket->socket, &peeraddr, &iLen));
            pSocket->iLastError = iErr;
            if (iErr == SOCKERR_NOTCONN)
            {
                NetPrintf(("dirtynetwin: connection closed\n"));
                pSocket->opened = -1;
            }
        }

        // return connect status
        return(pSocket->opened);
    }

    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function    SocketCallback

    \Description
        Register a callback routine for notification of socket events.  Also includes
        timeout support.

    \Input *sock    - socket reference
    \Input mask     - valid callback events (CALLB_NONE, CALLB_SEND, CALLB_RECV)
    \Input idle     - if nonzero, specifies the number of ticks between idle calls
    \Input *ref     - user data to be passed to proc
    \Input *proc    - user callback

    \Output
        int32_t     - zero

    \Notes
        A callback will reset the idle timer, so when specifying a callback and an
        idle processing time, the idle processing time represents the maximum elapsed
        time between calls.

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
#pragma optimize("", off) // make sure this block of code is not reordered
int32_t SocketCallback(SocketT *sock, int32_t mask, int32_t idle, void *ref, int32_t (*proc)(SocketT *sock, int32_t flags, void *ref))
{
    sock->callidle = idle;
    sock->callmask = mask;
    sock->callref = ref;
    sock->callback = proc;
    return(0);
}
#pragma optimize("", on)

/*F*************************************************************************************/
/*!
    \Function    SocketControl

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
            'conn' - handle connect message
            'disc' - handle disconnect message
            'ladr' - set local address for subsequent SocketBind() operations
            'loss' - simulate packet loss (debug only)
            'madr' - perform address remap (pData2=source sockaddr, pData3=remapped sockaddr) (Xbox/360 only)
            'maxp' - set max udp packet size
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'ndly' - set TCP_NODELAY state for given stream socket (iData1=zero or one)
            'poll' - execute blocking wait on given socket list (pData2) or all sockets (pData2=NULL), 64 max sockets
            'push' - push data into given socket (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'rbuf' - set socket recv buffer size
            'sbuf' - set socket send buffer size
            'sdcb' - set send callback (pData2=callback, pData3=callref)
            'vadd' - add a port to virtual port list
            'vdel' - del a port from virtual port list
            'xdns' - set dns remap address (Xbox/360 only)
            'xmap' - set address remap table  (iData1=number of entries, pData2=source data)
            'xnam' - set name remap table (iData1=number of entries, pData2=source data)
            'xtim' - set xlsp cache timeout in milliseconds (XENON only)
            '+xam' - add a single addr map entry (pData2=addr map entry to add)
            '+xnm' - add a single name map entry (pData2=name map entry to add)
            '-xam' - del a single addr map entry (iData1=index of addr map entry to del OR pData2=addr map entry to del)
            '-xnm' - del a single name map entry (iData1=index of name map entry to del OR pData2=name map entry to del)

        \endverbatim

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketControl(SocketT *pSocket, int32_t iOption, int32_t iData1, void *pData2, void *pData3)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iMapEntry;
    int32_t iResult;

    // handle connect message
    if (iOption == 'conn')
    {
        // clear the addr and name map tables
        memset(pState->aSockNameMap, 0, sizeof(pState->aSockNameMap));
        memset(pState->aSockAddrMap, 0, sizeof(pState->aSockAddrMap));

        #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
        // create LSP query module
        if ((pState->pDirtyLSP = DirtyLSPCreate(100)) != NULL)
        {
            // set up XLSP cache callback
            DirtyLSPControl(pState->pDirtyLSP, 'cbfp', 0, 0, (void *)_SocketLSPCacheCallback);
            DirtyLSPControl(pState->pDirtyLSP, 'cbup', 0, 0, pState);
        }
        else
        {
            NetPrintf(("dirtynetwin: could not create DirtyLSP\n"));
        }
        // set up initial "fake" address
        pState->iFakeAddr = 0x01000000;
        #endif

        return(0);
    }
    // handle disconnect message
    if (iOption == 'disc')
    {
        #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
        // clear xlsp global mapping table
        DirtyLSPControl(pState->pDirtyLSP, 'xclr', 0, 0, NULL);

        // destroy LSP query module
        if (pState->pDirtyLSP != NULL)
        {
            DirtyLSPDestroy(pState->pDirtyLSP);
            pState->pDirtyLSP = NULL;
        }
        #endif
        return(0);
    }
    // set local address? (used to select between multiple network interfaces)
    if (iOption == 'ladr')
    {
        pState->uAdapterAddress = (unsigned)iData1;
        return(0);
    }
    // set up packet loss simulation
    #if DIRTYCODE_DEBUG
    if (iOption == 'loss')
    {
        pState->uPacketLoss = (unsigned)iData1;
        NetPrintf(("dirtynetwin: packet loss simulation %s (param=0x%08x)\n", pState->uPacketLoss ? "enabled" : "disabled", pState->uPacketLoss));
        return(0);
    }
    #endif
    // perform address remap
    if (iOption == 'madr')
    {
        struct sockaddr TempAddr;
        const struct sockaddr *pAddr;
        pAddr = _MapAddress(NULL, &TempAddr, pData2);
        memcpy(pData3, pAddr, sizeof(*pAddr));
        return(0);
    }
    // set max udp packet size
    if (iOption == 'maxp')
    {
        NetPrintf(("dirtynetwin: setting max udp packet size to %d\n", iData1));
        pState->iMaxPacket = iData1;
        return(0);
    }
    // if a stream socket, set nonblocking/blocking mode
    if ((iOption == 'nbio') && (pSocket != NULL) && (pSocket->type == SOCK_STREAM))
    {
        uint32_t uNbio = (uint32_t)iData1;
        iResult = ioctlsocket(pSocket->socket, FIONBIO, (u_long *)&uNbio);
        pSocket->iLastError = _XlatError(iResult);
        NetPrintf(("dirtynetwin: setting socket:0x%x to %s mode %s (LastError=%d).\n", pSocket, iData1 ? "nonblocking" : "blocking", iResult ? "failed" : "succeeded", pSocket->iLastError));
        return(pSocket->iLastError);
    }
    // if a stream socket, set TCP_NODELAY state
    if ((iOption == 'ndly') && (pSocket != NULL) && (pSocket->type == SOCK_STREAM))
    {
        iResult = setsockopt(pSocket->socket, IPPROTO_TCP, TCP_NODELAY, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);
        return(pSocket->iLastError);
    }
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
                FD_SET((*ppSocket)->socket, &FdRead);
                FD_SET((*ppSocket)->socket, &FdExcept);
            }
        }
        else
        {
            // get exclusive access to socket list
            NetCritEnter(NULL);
            // walk socket list and add all sockets
            for (pSocket = pState->pSockList, iSocket = 0; (pSocket != NULL) && (iSocket < FD_SETSIZE); pSocket = pSocket->next, iSocket++)
            {
                if (pSocket->socket != INVALID_SOCKET)
                {
                    FD_SET(pSocket->socket, &FdRead);
                    FD_SET(pSocket->socket, &FdExcept);
                }
            }
            // for access to g_socklist and g_sockkill
            NetCritLeave(NULL);
        }

        // wait for input on the socket list for up to iData1 milliseconds
        return(select(iSocket, &FdRead, NULL, &FdExcept, &TimeVal));
    }
    // push data into receive buffer
    if (iOption == 'push')
    {
        if (pSocket != NULL)
        {
            // acquire socket critical section
            #if SOCKET_ASYNCRECVTHREAD
            NetCritEnter(&pSocket->recvcrit);
            #endif

            // don't allow data to be pushed that is too large for the buffer
            if (iData1 > sizeof(pSocket->recvdata))
            {
                NetPrintf(("dirtynetwin: request to push %d bytes of data discarded (max=%d)\n", iData1, sizeof(pSocket->recvdata)));
                #if SOCKET_ASYNCRECVTHREAD
                NetCritLeave(&pSocket->recvcrit);
                #endif
                return(-1);
            }

            // warn if there is already data present
            if (pSocket->recvstat > 0)
            {
                NetPrintf(("dirtynetwin: warning - overwriting packet data with SocketControl('push')\n"));
            }

            // save the size and copy the data
            pSocket->recvstat = iData1;
            memcpy(pSocket->recvdata, pData2, pSocket->recvstat);

            // save the address
            memcpy(&pSocket->recvaddr, pData3, sizeof(pSocket->recvaddr));
            // save receive timestamp
            SockaddrInSetMisc(&pSocket->recvaddr, NetTick());

            // release socket critical section
            #if SOCKET_ASYNCRECVTHREAD
            NetCritLeave(&pSocket->recvcrit);
            #endif

            // see if we should issue callback
            if ((pSocket->callback != NULL) && (pSocket->callmask & CALLB_RECV))
            {
                pSocket->callback(pSocket, 0, pSocket->callref);
            }
            return(0);
        }
        else
        {
            NetPrintf(("dirtynetwin: warning - call to SocketControl('push') ignored because pSocket is NULL\n"));
        }
    }
    // set REUSEADDR
    if (iOption == 'radr')
    {
        iResult = setsockopt(pSocket->socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);
        return(pSocket->iLastError);
    }
    // set socket receive buffer size
    if ((iOption == 'rbuf') || (iOption == 'sbuf'))
    {
        int32_t iOldSize, iNewSize, iOptLen=4;
        int32_t iSockOpt = (iOption == 'rbuf') ? SO_RCVBUF : SO_SNDBUF;

        // get current buffer size
        getsockopt(pSocket->socket, SOL_SOCKET, iSockOpt, (char *)&iOldSize, &iOptLen);

        // set new size
        iResult = setsockopt(pSocket->socket, SOL_SOCKET, iSockOpt, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);

        // get new size
        getsockopt(pSocket->socket, SOL_SOCKET, iSockOpt, (char *)&iNewSize, &iOptLen);
        NetPrintf(("dirtynetwin: setsockopt(%s) changed buffer size from %d to %d\n", (iOption == 'rbuf') ? "SO_RCVBUF" : "SO_SNDBUF",
            iOldSize, iNewSize));

        return(pSocket->iLastError);
    }
    // set global send callback
    if (iOption == 'sdcb')
    {
        pState->pSendCallback = (SocketSendCallbackT *)pData2;
        pState->pSendCallref = pData3;
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
    // set dns override
    if (iOption == 'xdns')
    {
        pState->iDnsRemap = iData1;
        return(0);
    }
    // set secure override
    if (iOption == 'xins')
    {
        NetPrintf(("dirtynetwin: secure bypass %s\n", iData1 ? "enabled" : "disabled"));
        setsockopt(pSocket->socket, SOL_SOCKET, 0x5801 /*SO_INSECURE*/, (const char *)&iData1, sizeof(iData1));
        pSocket->bInsecure = iData1 ? TRUE : FALSE;
        return(0);
    }
    // set an address mapping table
    if (iOption == 'xmap')
    {
        iData1 *= sizeof(pState->aSockAddrMap[0]);
        if (iData1 > sizeof(pState->aSockAddrMap))
        {
            NetPrintf(("dirtynetwin: clamping addr map to %d entries\n", sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0])));
            iData1 = sizeof(pState->aSockAddrMap);
        }
        memcpy(pState->aSockAddrMap, pData2, iData1);
        return(0);
    }
    // set a name mapping table
    if (iOption == 'xnam')
    {
        iData1 *= sizeof(pState->aSockNameMap[0]);
        if (iData1 > sizeof(pState->aSockNameMap))
        {
            NetPrintf(("dirtynetwin: clamping name map to %d entries\n", sizeof(pState->aSockNameMap)/sizeof(pState->aSockNameMap[0])));
            iData1 = sizeof(pState->aSockNameMap);
        }
        memcpy(pState->aSockNameMap, pData2, iData1);
        return(0);
    }
    // add an addr map entry
    if (iOption == '+xam')
    {
        for (iMapEntry = 0; iMapEntry < sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]); iMapEntry++)
        {
            if (pState->aSockAddrMap[iMapEntry].uRemap == 0)
            {
                memcpy(&pState->aSockAddrMap[iMapEntry], pData2, sizeof(pState->aSockAddrMap[0]));
                NetPrintf(("dirtynetwin: add addr map entry [%d] %s\n", iMapEntry, _DisplayAddrMapEntry(&pState->aSockAddrMap[iMapEntry])));
                return(iMapEntry);
            }
        }
        NetPrintf(("dirtynetwin: unable to add addr map entry %s; table full\n", _DisplayAddrMapEntry((SocketAddrMapT *)pData2)));
        return(-1);
    }
    // set xlsp cache timeout, in milliseconds
    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    if ((iOption == 'xtim') && (pState->pDirtyLSP != NULL))
    {
        DirtyLSPControl(pState->pDirtyLSP, iOption, iData1, 0, NULL);
        return(0);
    }
    #endif
    // add a name map entry
    if (iOption == '+xnm')
    {
        for (iMapEntry = 0; iMapEntry < sizeof(pState->aSockNameMap)/sizeof(pState->aSockNameMap[0]); iMapEntry++)
        {
            if (pState->aSockNameMap[iMapEntry].strDnsName[0] == '\0')
            {
                memcpy(&pState->aSockNameMap[iMapEntry], pData2, sizeof(pState->aSockNameMap[0]));
                NetPrintf(("dirtynetwin: add name map entry [%d] %s\n", iMapEntry, _DisplayNameMapEntry(&pState->aSockNameMap[iMapEntry])));
                return(iMapEntry);
            }
        }
        NetPrintf(("dirtynetwin: unable to add name map entry %s; table full\n", _DisplayNameMapEntry((SocketNameMapT *)pData2)));
        return(-1);
    }
    // del an addr map entry
    if (iOption == '-xam')
    {
        int32_t iDelEntry = -1;
        // if specified, find entry to delete
        if (pData2 != NULL)
        {
            for (iMapEntry = 0; iMapEntry < sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]); iMapEntry++)
            {
                if (!memcmp(&pState->aSockAddrMap[iMapEntry], pData2, sizeof(pState->aSockAddrMap[0])))
                {
                    iDelEntry = iMapEntry;
                    break;
                }
            }
            if (iMapEntry == sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]))
            {
                NetPrintf(("dirtynetwin: could not find addr map entry %s to delete\n", _DisplayAddrMapEntry((SocketAddrMapT *)pData2)));
            }
        }
        else if ((iData1 >= 0) && (iData1 < sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0])))
        {
            iDelEntry = iData1;
        }
        else
        {
            NetPrintf(("dirtynetwin: could not delete addr map entry with index %d; invalid range\n", iData1));
        }
        // remove entry?
        if (iDelEntry != -1)
        {
            NetPrintf(("dirtynetwin: del addr map entry [%d] %s\n", iDelEntry, _DisplayAddrMapEntry(&pState->aSockAddrMap[iDelEntry])));
            memset(&pState->aSockAddrMap[iDelEntry], 0, sizeof(pState->aSockAddrMap[0]));
            return(iDelEntry);
        }
    }
    // del a name map entry
    if (iOption == '-xnm')
    {
        int32_t iDelEntry = -1;
        // if specified, find entry to delete
        if (pData2 != NULL)
        {
            for (iMapEntry = 0; iMapEntry < sizeof(pState->aSockNameMap)/sizeof(pState->aSockNameMap[0]); iMapEntry++)
            {
                if (!memcmp(&pState->aSockNameMap[iMapEntry], pData2, sizeof(pState->aSockNameMap[0])))
                {
                    iDelEntry = iMapEntry;
                    break;
                }
            }
            if (iMapEntry == sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]))
            {
                NetPrintf(("dirtynetwin: could not find name map entry %s to delete\n", _DisplayNameMapEntry((SocketNameMapT *)pData2)));
            }
        }
        else if ((iData1 >= 0) && (iData1 < sizeof(pState->aSockNameMap)/sizeof(pState->aSockNameMap[0])))
        {
            iDelEntry = iData1;
        }
        else
        {
            NetPrintf(("dirtynetwin: could not delete name map entry with index %d; invalid range\n", iData1));
        }
        // remove entry?
        if (iDelEntry != -1)
        {
            NetPrintf(("dirtynetwin: del name map entry [%d] %s\n", iDelEntry, _DisplayNameMapEntry(&pState->aSockNameMap[iDelEntry])));
            memset(&pState->aSockNameMap[iDelEntry], 0, sizeof(pState->aSockNameMap[0]));
            return(iDelEntry);
        }
    }
    // unhandled
    return(-1);
}

/*F*************************************************************************************/
/*!
    \Function    SocketGetLocalAddr

    \Description
        Returns the "external" local address (ie, the address as a machine "out on
        the Internet" would see as the local machine's address).

    \Output
        uint32_t        - local address

    \Version 07/28/2003 (jbrookes)
*/
/************************************************************************************F*/
uint32_t SocketGetLocalAddr(void)
{
    struct sockaddr inet, host;

    // create a remote internet address
    memset(&inet, 0, sizeof(inet));
    inet.sa_family = AF_INET;
    SockaddrInSetPort(&inet, 79);
    SockaddrInSetAddr(&inet, 0xc0a80101);

    // ask socket to give us local address that can connect to it
    memset(&host, 0, sizeof(host));
    SocketHost(&host, sizeof(host), &inet, sizeof(inet));

    return(SockaddrInGetAddr(&host));
}


// Windows-specific functions

#if DIRTYCODE_PLATFORM != DIRTYCODE_XENON
/*F*************************************************************************************/
/*!
    \Function    SocketLookupDone

    \Description
        Callback to determine if gethostbyname is complete.

    \Input *host    - pointer to host lookup record

    \Output
        int32_t     - zero=in progess, neg=done w/error, pos=done w/success

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static int32_t SocketLookupDone(HostentT *host)
{
    // return current status
    return(host->done);
}

/*F*************************************************************************************/
/*!
    \Function    SocketLookupFree

    \Description
        Release resources used by gethostbyname.

    \Input *host    - pointer to host lookup record

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static void SocketLookupFree(HostentT *host)
{
    SocketStateT *pState = _Socket_pState;

    // mark thread for death
    if (InterlockedExchange((volatile LONG *)&host->thread, 1) != 0)
    {
        // already marked-- we must free memory
        DirtyMemFree(host, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
}

/*F*************************************************************************************/
/*!
    \Function    SocketLookupThread

    \Description
        Socket lookup thread

    \Input *host    - pointer to host lookup record

    \Output
        int32_t     - zero

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static int32_t SocketLookupThread(HostentT *host)
{
    SocketStateT *pState = _Socket_pState;
    struct hostent *winhost;
    unsigned char *ipaddr;

    // perform the blocking dns query
    winhost = gethostbyname(host->name);
    if (winhost != NULL)
    {
        // extract ip address
        ipaddr = (unsigned char *)winhost->h_addr_list[0];
        host->addr = (ipaddr[0]<<24)|(ipaddr[1]<<16)|(ipaddr[2]<<8)|(ipaddr[3]<<0);
        // mark success
        host->done = 1;
    }
    else
    {
        // unsuccessful
        host->done = -1;
    }

    // indicate thread is done and see if we need to release memory
    if (InterlockedExchange((volatile LONG *)&host->thread, 1) != 0)
    {
        // already marked-- we must free memory
        DirtyMemFree(host, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    // return thread success
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function    SocketLookup

    \Description
        Lookup a host by name and return the corresponding Internet address. Uses
        a callback/polling system since the socket library does not allow blocking.

    \Input *text    - pointer to null terminated address string
    \Input timeout  - number of milliseconds to wait for completion

    \Output
        HostentT *  - hostent struct that includes callback vectors

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
HostentT *SocketLookup(const char *text, int32_t timeout)
{
    SocketStateT *pState = _Socket_pState;
    int32_t i, j;
    const char *s;
    HostentT *host;
    DWORD pid;
    HANDLE thread;

    // dont allow negative timeouts
    if (timeout < 0)
    {
        return(NULL);
    }

    // create new structure
    host = DirtyMemAlloc(sizeof(*host), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(host, 0, sizeof(*host));

    // setup callbacks
    host->Done = &SocketLookupDone;
    host->Free = &SocketLookupFree;

    // check for dot notation
    for (s = text; *s != 0; ++s)
    {
        if ((*s != '.') && ((*s < '0') || (*s > '9')))
        {
            break;
        }
    }
    if (*s == 0)
    {
        // translate into address
        s = text;
        for (i = 0; i < 4; ++i)
        {
            // grab one element
            for (j = 0; (*s >= '0') && (*s <= '9');)
            {
                j = (j * 10) + (*s++ & 15);
            }
            // make sure it was terminated correct
            if ((*s != ((i < 3) ? '.' : 0)) || (j > 255))
            {
                host->done = -2;
                host->addr = 0;
                break;
            }
            // save the data
            host->addr = (host->addr << 8) | j;
            host->done = 1;
            ++s;
        }
        // mark as "thread complete" so SocketLookupFree will release memory
        host->thread = 1;
        // return completed record
        return(host);
    }

    // copy over the target address
    strnzcpy(host->name, text, sizeof(host->name));

    // create dns lookup thread
    thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&SocketLookupThread, (LPVOID)host, 0, &pid);
    if (thread != NULL)
    {
        CloseHandle(thread);
    }

    // return the host reference
    return(host);
}

/*F*************************************************************************************/
/*!
    \Function    SocketHost

    \Description
        Return the host address that would be used in order to communicate with the given
        destination address.

    \Input *host    - local sockaddr struct
    \Input hostlen  - length of structure (sizeof(host))
    \Input *dest    - remote sockaddr struct
    \Input destlen  - length of structure (sizeof(dest))

    \Output
        int32_t     - zero=success, negative=error

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketHost(struct sockaddr *host, int32_t hostlen, const struct sockaddr *dest, int32_t destlen)
{
    SocketStateT *pState = _Socket_pState;
    struct sockaddr addr;
    struct hostent *ent;
    char hostname[256];
    int32_t err;
    SOCKET sock;

    // must be same kind of addresses
    if (hostlen != destlen)
    {
        return(-1);
    }

    // do family specific lookup
    if (dest->sa_family == AF_INET)
    {
        // make them match initially
        memcpy(host, dest, hostlen);

        // respect adapter override
        if (pState->uAdapterAddress != 0)
        {
            SockaddrInSetAddr(host, pState->uAdapterAddress);
            return(0);
        }

        // zero the address portion
        host->sa_data[2] = host->sa_data[3] = host->sa_data[4] = host->sa_data[5] = 0;

        // create a temp socket (must be datagram)
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock != INVALID_SOCKET)
        {
            // use routing check if winsock 2.0
            if (pState->iVersion >= 0x200)
            {
                // get interface that would be used for this dest
                memset(&addr, 0, sizeof(addr));
                if (WSAIoctl(sock, SIO_ROUTING_INTERFACE_QUERY, (void *)dest, destlen, &addr, sizeof(addr), (LPDWORD)&hostlen, NULL, NULL) < 0)
                {
                    err = WSAGetLastError();
                    NetPrintf(("sockethost: SIO_ROUTING_INTERFACE_QUERY returned %d\n", err));
                }
                else
                {
                    // copy over the result
                    memcpy(host->sa_data+2, addr.sa_data+2, 4);
                }

                // convert loopback to requested address
                if (SockaddrInGetAddr(host) == INADDR_LOOPBACK)
                {
                    memcpy(host->sa_data+2, dest->sa_data+2, 4);
                }
            }

            // if no result, try connecting to socket then reading
            // (this only works on some non-microsoft stacks)
            if (SockaddrInGetAddr(host) == 0)
            {
                // connect to remote address
                if (connect(sock, dest, destlen) == 0)
                {
                    // query the host address
                    if (getsockname(sock, &addr, &hostlen) == 0)
                    {
                        // just copy over the address portion
                        memcpy(host->sa_data+2, addr.sa_data+2, 4);
                    }
                }
            }

            // if still no result, use classic gethosthame/gethostbyname
            // (works for microsoft, not for non-microsoft stacks)
            if (SockaddrInGetAddr(host) == 0)
            {
                // get machine name
                gethostname(hostname, sizeof(hostname));
                // lookup ip info
                ent = gethostbyname(hostname);
                if (ent != NULL)
                {
                    memcpy(host->sa_data+2, ent->h_addr_list[0], 4);
                }
            }

            // close the socket
            closesocket(sock);
        }
        return(0);
    }

    // unsupported family
    memset(host, 0, hostlen);
    return(-3);
}

#endif // windows-specific functions


#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON // xbox 360 specific functions
/*F*************************************************************************************/
/*!
    \Function    SocketLookupDone

    \Description
        Callback to determine if gethostbyname is complete.

    \Input *host    - pointer to host lookup record

    \Output
        int32_t     - zero=in progess, neg=done w/error, pos=done w/success

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static int32_t _SocketLookupDone(HostentT *host)
{
    // check status
    XNDNS *pXndns = (XNDNS *)host->sema;
    unsigned char *pAddr;

    // if no thread, we completed immediately
    if (host->thread == 0)
    {
        return(host->done);
    }

    // wait for completion
    if (WaitForSingleObject((HANDLE)host->thread, 0) == WAIT_OBJECT_0)
    {
        if (pXndns->iStatus == 0)
        {
            host->done = TRUE;
            if (pXndns->cina > 0)
            {
                // extract ip address
                pAddr = (unsigned char *)&pXndns->aina[0].S_un.S_un_b;
                host->addr = (pAddr[0]<<24)|(pAddr[1]<<16)|(pAddr[2]<<8)|(pAddr[3]<<0);
                NetPrintf(("socketlookupdone: success, addr=%a\n", host->addr));
            }
        }
        else if (pXndns->iStatus != WSAEINPROGRESS)
        {
            NetPrintf(("socketlookupdone: failure, err=%d\n", pXndns->iStatus));
            host->done = -1;
        }
    }

    // return current status
    return(host->done);
}

/*F*************************************************************************************/
/*!
    \Function    SocketLookupFree

    \Description
        Release resources used by gethostbyname.

    \Input *host    - pointer to host lookup record

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static void _SocketLookupFree(HostentT *host)
{
    SocketStateT *pState = _Socket_pState;
    if (host->sema != 0)
    {
        XNetDnsRelease((XNDNS *)host->sema);
    }
    if (host->thread != 0)
    {
        WSACloseEvent((HANDLE)host->thread);
    }
    DirtyMemFree(host, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F*************************************************************************************/
/*!
    \Function _SocketLSPQueryDone

    \Description
        Callback to determine if LSP query is complete

    \Input *pHost   - pointer to host lookup record

    \Output
        int32_t     - zero=in progess, neg=done w/error, pos=done w/success

    \Version 03/13/2006 (jbrookes)
*/
/************************************************************************************F*/
static int32_t _SocketLSPQueryDone(HostentT *pHost)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iAddrMap, iNameMap, iRealAddr;

    // query still pending
    if (pHost->done == 0)
    {
        // ref query record
        DirtyLSPQueryRecT *pQueryRec = (DirtyLSPQueryRecT *)pHost->thread;

        // is the query complete?
        if ((pHost->done = DirtyLSPQueryDone(pState->pDirtyLSP, pQueryRec)) > 0)
        {
            // save real secure address
            iRealAddr = pQueryRec->uAddress;

            // generate a new fake address to use to set up address remap
            pHost->addr = pState->iFakeAddr++;

            // find first empty slot in address map
            for (iAddrMap = 0; iAddrMap < sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]); iAddrMap += 1)
            {
                SocketAddrMapT *pAddrMap = &pState->aSockAddrMap[iAddrMap];
                if (pAddrMap->uAddr == 0)
                {
                    // add entries for all unique port mappings in name map
                    for (iNameMap = 0; iNameMap < sizeof(pState->aSockNameMap)/sizeof(pState->aSockNameMap[0]); iNameMap += 1)
                    {
                        SocketNameMapT *pNameMap = &pState->aSockNameMap[iNameMap];

                        if (!strcmp(pHost->name, pNameMap->strRemap))
                        {
                            // make sure there is room in the address map
                            if (iAddrMap == sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]))
                            {
                                break;
                            }

                            // save fake address in name map
                            pNameMap->uRemap = pHost->addr;

                            // set up address remap entry
                            pAddrMap->uAddr = pHost->addr;
                            pAddrMap->uMask = 0xffffffff;
                            pAddrMap->uRemap = iRealAddr;
                            pAddrMap->uServiceId = pNameMap->uServiceId;
                            pAddrMap->uSrcPort = pNameMap->uSrcPort;
                            pAddrMap->uDstPort = pNameMap->uDstPort;
                            pAddrMap += 1;
                            iAddrMap += 1;
                        }
                    }
                    // done
                    pHost->done = 1;
                    break;
                }
            }
            if (iAddrMap == sizeof(pState->aSockAddrMap)/sizeof(pState->aSockAddrMap[0]))
            {
                NetPrintf(("dirtynetwin: warning -- could not add lsp query result to address map\n"));
            }
        }
    }
    return(pHost->done);
}

/*F*************************************************************************************/
/*!
    \Function _SocketLSPQueryFree

    \Description
        Release resources used by gethostbyname.

    \Input *pHost   - pointer to host lookup record

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static void _SocketLSPQueryFree(HostentT *pHost)
{
    SocketStateT *pState = _Socket_pState;
    DirtyLSPQueryRecT *pQueryRec = (DirtyLSPQueryRecT *)pHost->thread;
    if (pQueryRec != NULL)
    {
        DirtyLSPQueryFree(pState->pDirtyLSP, pQueryRec);
    }
    DirtyMemFree(pHost, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
}

/*F*************************************************************************************/
/*!
    \Function    SocketLookup

    \Description
        Lookup a host by name and return the corresponding Internet address. Uses
        a callback/polling system since the socket library does not allow blocking.

    \Input *text    - pointer to null terminated address string
    \Input timeout  - number of milliseconds to wait for completion

    \Output
        HostentT *  - hostent struct that includes callback vectors

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
HostentT *SocketLookup(const char *text, int32_t timeout)
{
    return(SocketLookup2(text, timeout, 0));
}

/*F*************************************************************************************/
/*!
    \Function    SocketLookup2

    \Description
        Lookup a host by name and return the corresponding Internet address. Uses
        a callback/polling system since the socket library does not allow blocking.

    \Input *text    - pointer to null terminated address string
    \Input timeout  - number of milliseconds to wait for completion
    \Input flags    - SOCKLOOK_FLAGS_*

    \Output
        HostentT *  - hostent struct that includes callback vectors

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
HostentT *SocketLookup2(const char *text, int32_t timeout, uint32_t flags)
{
    SocketStateT *pState = _Socket_pState;
    const SocketNameMapT *pSockNameMap;
    int32_t iErr, iAddr;
    HostentT *pHost;

    // dont allow negative timeouts
    if (timeout < 0)
    {
        return(NULL);
    }

    // create new structure
    pHost = DirtyMemAlloc(sizeof(*pHost), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(pHost, 0, sizeof(*pHost));

    // setup callbacks
    pHost->Done = _SocketLookupDone;
    pHost->Free = _SocketLookupFree;

    // copy name to host record
    NetPrintf(("dirtynetwin: looking up address for '%s'\n", text));

    // secure mode?
    if ((pState->iDnsRemap != 0) && ((flags & SOCKLOOK_FLAGS_ALLOWXDNS) == 0))
    {
        // try name remap
        if ((pSockNameMap = _MapName(pState, text)) != NULL)
        {
            // do we have an address?
            if (pSockNameMap->uRemap == 0)
            {
                DirtyLSPQueryRecT *pQueryRec;

                // start LSP query
                strnzcpy(pHost->name, pSockNameMap->strRemap, sizeof(pHost->name));
                if ((pQueryRec = DirtyLSPQueryStart(pState->pDirtyLSP, pHost->name)) == NULL)
                {
                    NetPrintf(("dirtynetwin: error initiating service query\n"));
                    return(NULL);
                }
                // redirect callbacks to Query versions
                pHost->Done = &_SocketLSPQueryDone;
                pHost->Free = &_SocketLSPQueryFree;
                // save query record ref
                pHost->thread = (intptr_t)pQueryRec;
            }
            else // already have an address, so use it
            {
                pHost->addr = pSockNameMap->uRemap;
                pHost->done = 1;
            }
        }
        // check for dot notation
        else if ((iAddr = SocketInTextGetAddr(text)) != 0)
        {
            // save the data and return completed record
            pHost->addr = iAddr;
            pHost->done = 1;
            return(pHost);
        }
        else // no name remap for this name, so default to our dns remap address
        {
            NetPrintf(("dirtynetwin: dns remap %s to %a\n", text, pState->iDnsRemap));
            pHost->addr = pState->iDnsRemap;
            pHost->done = 1;
        }
    }
    else // not in secure mode, so use real DNS
    {
        // check for dot notation
        if ((iAddr = SocketInTextGetAddr(text)) != 0)
        {
            // save the data and return completed record
            pHost->addr = iAddr;
            pHost->done = 1;
            return(pHost);
        }

        // create WASEvent
        pHost->thread = (int32_t)WSACreateEvent();

        // initiate DNS lookup
        pHost->sema = 0;
        strnzcpy(pHost->name, text, sizeof(pHost->name));
        if ((iErr = XNetDnsLookup(pHost->name, (HANDLE)pHost->thread, (XNDNS **)&pHost->sema)) != 0)
        {
            NetPrintf(("dirtynetwin: error %d initiating DNS lookup\n", iErr));
            WSACloseEvent((HANDLE)pHost->thread);
            pHost->thread = 0;
            pHost->addr = 0;
            pHost->done = -1;
        }
    }

    // return the host reference
    return(pHost);
}

/*F*************************************************************************************/
/*!
    \Function    SocketHost

    \Description
        Return the host address that would be used in order to communicate with the given
        destination address.

    \Input *host    - local sockaddr struct
    \Input hostlen  - length of structure (sizeof(host))
    \Input *dest    - remote sockaddr struct
    \Input destlen  - length of structure (sizeof(dest))

    \Output
        int32_t     - zero=success, negative=error

    \Version 01/15/2004 (jbrookes)
*/
/************************************************************************************F*/
int32_t SocketHost(struct sockaddr *host, int32_t hostlen, const struct sockaddr *dest, int32_t destlen)
{
    DWORD dwResult;
    XNADDR XNAddr;
    int32_t iAddr = 0;

    // must be same kind of addresses
    if (hostlen != destlen)
    {
        NetPrintf(("sockethost: len mismatch\n"));
        return(-1);
    }

    // do family specific lookup
    if (dest->sa_family == AF_INET)
    {
        // make them match initially
        memcpy(host, dest, hostlen);

        // zero address portion by default
        host->sa_data[2] = host->sa_data[3] = host->sa_data[4] = host->sa_data[5] = 0;

        // get address
        if ((dwResult = XNetGetTitleXnAddr(&XNAddr)) & (XNET_GET_XNADDR_GATEWAY|XNET_GET_XNADDR_DNS))
        {
            unsigned char *pAddr;

            // extract ip address
            pAddr = (unsigned char *)&XNAddr.ina.S_un.S_un_b;
            iAddr = (pAddr[0]<<24)|(pAddr[1]<<16)|(pAddr[2]<<8)|(pAddr[3]<<0);

            // set it into the host structure, return success
            SockaddrInSetAddr(host, iAddr);
            return(0);
        }

        // unable to get address
        return(-2);
    }

    // unsupported family
    memset(host, 0, hostlen);
    return(-3);
}
#endif
