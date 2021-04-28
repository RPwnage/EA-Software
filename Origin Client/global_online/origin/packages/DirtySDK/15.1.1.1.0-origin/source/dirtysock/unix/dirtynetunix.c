/*H********************************************************************************/
/*!
    \File dirtynetunix.c

    \Description
        Provides a wrapper that translates the Unix network interface to the
        DirtySock portable networking interface.

    \Copyright
        Copyright (c) 2010 Electronic Arts Inc.

    \Version 04/05/2010 (jbrookes) First version; a vanilla port to Linux from PS3
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
 #include <ifaddrs.h>
 #include <arpa/inet.h>
 #include <net/if.h>
 #include <net/if_dl.h>
#else
 #include <signal.h>
 #include <net/if.h>             // struct ifreq
 #include <sys/ioctl.h>          // ioctl
#endif

#if defined(DIRTYCODE_APPLEIOS)
 #include <CoreFoundation/CFData.h>
 #include <CoreFoundation/CFStream.h>
 #include <CFNetwork/CFHTTPStream.h>
 #include <CFNetwork/CFHost.h>
#endif

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtyerr.h"

#include "dirtynetpriv.h"       // private include for dirtynet common functions

/*** Defines **********************************************************************/

#define INVALID_SOCKET      (-1)

#define SOCKET_MAXPOLL      (32)

#define SOCKET_VERBOSE      (DIRTYCODE_DEBUG && FALSE)

/*** Type Definitions *************************************************************/

//! private socketlookup structure containing extra data
typedef struct SocketLookupPrivT
{
    HostentT    Host;      //!< must come first!
    pthread_t   iThreadId; //!< 64-bit thread id (so we can't use Host.thread)
} SocketLookupPrivT;

//! dirtysock connection socket structure
struct SocketT
{
    SocketT *next;              //!< link to next active
    SocketT *kill;              //!< link to next killed socket

    int32_t family;             //!< protocol family
    int32_t type;               //!< protocol type
    int32_t proto;              //!< protocol ident

    int8_t  opened;             //!< negative=error, zero=not open (connecting), positive=open
    uint8_t bImported;          //!< whether socket was imported or not
    uint8_t bVirtual;           //!< if true, socket is virtual
    uint8_t hasdata;            //!< zero if no data, else has data ready to be read
    uint8_t bInCallback;        //!< in a socket callback
    uint8_t brokenflag;         //!< 0 or 1=might not be broken, >1=broken socket
    uint8_t bAsyncRecv;         //!< if true, async recv is enabled
    uint8_t bSendCbs;           //!< TRUE if send cbs are enabled, false otherwise

    int32_t socket;             //!< unix socket ref
    int32_t iLastError;         //!< last socket error

    struct sockaddr local;      //!< local address
    struct sockaddr remote;     //!< remote address

    uint16_t virtualport;       //!< virtual port, if set
    uint16_t pollidx;           //!< index in blocking poll() operation

    SocketRateT SendRate;       //!< send rate estimation data
    SocketRateT RecvRate;       //!< recv rate estimation data

    int32_t callmask;           //!< valid callback events
    uint32_t calllast;          //!< last callback tick
    uint32_t callidle;          //!< ticks between idle calls
    void *callref;              //!< reference calback value
    int32_t (*callback)(SocketT *sock, int32_t flags, void *ref);

    #if SOCKET_ASYNCRECVTHREAD
    NetCritT recvcrit;          //!< receive critical section
    int32_t recverr;            //!< last error that occurred
    uint32_t recvflag;          //!< flags from recv operation
    uint8_t recvinp;            //!< if true, a receive operation is in progress
    uint8_t _pad2[3];
    #endif

    int32_t rbufsize;           //!< read buffer size (bytes)
    int32_t sbufsize;           //!< send buffer size (bytes)

    struct sockaddr recvaddr;   //!< receive address
    int32_t recvstat;           //!< how many queued receive bytes

    SocketPacketQueueT *pRecvQueue;
    SocketPacketQueueEntryT *pRecvPacket;
};

//! standard ipv4 packet header (see RFC791)
typedef struct HeaderIpv4
{
    uint8_t verslen;      //!< version and length fields (4 bits each)
    uint8_t service;      //!< type of service field
    uint8_t length[2];    //!< total packet length (header+data)
    uint8_t ident[2];     //!< packet sequence number
    uint8_t frag[2];      //!< fragmentation information
    uint8_t time;         //!< time to live (remaining hop count)
    uint8_t proto;        //!< transport protocol number
    uint8_t check[2];     //!< header checksum
    uint8_t srcaddr[4];   //!< source ip address
    uint8_t dstaddr[4];   //!< dest ip address
} HeaderIpv4;

//! local state
typedef struct SocketStateT
{
    SocketT  *pSockList;                //!< master socket list
    SocketT  *pSockKill;                //!< list of killed sockets
    HostentT *pHostList;                //!< list of ongoing name resolution operations

    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list

    // module memory group
    int32_t  iMemGroup;                 //!< module mem group id
    void     *pMemGroupUserData;        //!< user data associated with mem group

    uint32_t uConnStatus;               //!< current connection status
    uint32_t uLocalAddr;                //!< local internet address for active interface
    int32_t  iMaxPacket;                //!< maximum packet size

    uint8_t  aMacAddr[6];               //!< MAC address for active interface
    uint8_t  bSingleThreaded;           //!< TRUE if in single-threaded mode
    uint8_t  _pad;

    #if SOCKET_ASYNCRECVTHREAD
    pthread_t iRecvThread;
    volatile int32_t iRecvLife;
    #endif

    SocketHostnameCacheT *pHostnameCache; //!< hostname cache

    SocketSendCallbackEntryT aSendCbEntries[SOCKET_MAXSENDCALLBACKS]; //!< collection of send callbacks
} SocketStateT;

/*** Variables ********************************************************************/

//! module state ref
static SocketStateT *_Socket_pState = NULL;

#if DIRTYSOCK_ERRORNAMES
//! getaddrinfo() error result table
static const DirtyErrT _GAI_ErrList[] =
{
    DIRTYSOCK_ErrorName(EAI_BADFLAGS),      //  -1; Invalid value for 'ai_flags' field.
    DIRTYSOCK_ErrorName(EAI_NONAME),        //  -2; NAME or SERVICE is unknown.
    DIRTYSOCK_ErrorName(EAI_AGAIN),         //  -3; Temporary failure in name resolution.
    DIRTYSOCK_ErrorName(EAI_FAIL),          //  -4; Non-recoverable failure in name res.
    #if defined(__USE_GNU) || DIRTYCODE_ANDROID
    DIRTYSOCK_ErrorName(EAI_NODATA),        //  -5: No address associated with NAME.
    #endif
    DIRTYSOCK_ErrorName(EAI_FAMILY),        //  -6; 'ai_family' not supported.
    DIRTYSOCK_ErrorName(EAI_SOCKTYPE),      //  -7; 'ai_socktype' not supported.
    DIRTYSOCK_ErrorName(EAI_SERVICE),       //  -8; SERVICE not supported for 'ai_socktype'.
    #if defined(__USE_GNU) || DIRTYCODE_ANDROID
    DIRTYSOCK_ErrorName(EAI_ADDRFAMILY),    //  -9; Address family for NAME not supported.
    #endif
    DIRTYSOCK_ErrorName(EAI_MEMORY),        // -10; Memory allocation failure.
    DIRTYSOCK_ErrorName(EAI_SYSTEM),        // -11; System error returned in `errno'.
    DIRTYSOCK_ErrorName(EAI_OVERFLOW),      // -12; Argument buffer overflow.
    #if DIRTYCODE_LINUX && defined(__USE_GNU)
    DIRTYSOCK_ErrorName(EAI_INPROGRESS),    // -100; Processing request in progress.
    DIRTYSOCK_ErrorName(EAI_CANCELED),      // -101; Request canceled.
    DIRTYSOCK_ErrorName(EAI_NOTCANCELED),   // -102; Request not canceled.
    DIRTYSOCK_ErrorName(EAI_ALLDONE),       // -103; All requests done.
    DIRTYSOCK_ErrorName(EAI_INTR),          // -104; Interrupted by a signal.
    DIRTYSOCK_ErrorName(EAI_IDN_ENCODE),    // -105; IDN encoding failed.
    #endif
    DIRTYSOCK_ListEnd()
};
#endif

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function    _SocketTranslateError

    \Description
        Translate a BSD error to dirtysock

    \Input iErr     - BSD error code (e.g EAGAIN)

    \Output
        int32_t     - dirtysock error (SOCKERR_*)

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketTranslateError(int32_t iErr)
{
    if (iErr < 0)
    {
        iErr = errno;
        if ((iErr == EWOULDBLOCK) || (iErr == EINPROGRESS))
            iErr = SOCKERR_NONE;
        else if (iErr == EHOSTUNREACH)
            iErr = SOCKERR_UNREACH;
        else if (iErr == ENOTCONN)
            iErr = SOCKERR_NOTCONN;
        else if (iErr == ECONNREFUSED)
            iErr = SOCKERR_REFUSED;
        else if (iErr == ECONNRESET)
            iErr = SOCKERR_CONNRESET;
        else if ((iErr == EBADF) || (iErr == EPIPE))
            iErr = SOCKERR_BADPIPE;
        else
            iErr = SOCKERR_OTHER;
    }
    return(iErr);
}

#if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_ANDROID)
/*F********************************************************************************/
/*!
    \Function    _SocketDisableSigpipe

    \Description
        Disable SIGPIPE signal.

    \Version 12/08/2003 (sbevan)
*/
/********************************************************************************F*/
static void _SocketDisableSigpipe(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, 0);
}
#endif

/*F********************************************************************************/
/*!
    \Function    _SocketCreateSocket

    \Description
        Create a system level socket.

    \Input iAddrFamily  - address family (AF_INET)
    \Input iType        - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input iProto       - protocol type for SOCK_RAW (unused by others)

    \Output
        int32_t         - socket handle

    \Version 16/02/2012 (szhu)
*/
/********************************************************************************F*/
static int32_t _SocketCreateSocket(int32_t iAddrFamily, int32_t iType, int32_t iProto)
{
    int32_t iSocket;
    // create socket
    if ((iSocket = socket(iAddrFamily, iType, iProto)) >= 0)
    {
        uint32_t uTrue = 1;
        // if dgram, allow broadcast
        if (iType == SOCK_DGRAM)
        {
            setsockopt(iSocket, SOL_SOCKET, SO_BROADCAST, &uTrue, sizeof(uTrue));
        }
        // if a raw socket, set header include
        if (iType == SOCK_RAW)
        {
            setsockopt(iSocket, IPPROTO_IP, IP_HDRINCL, &uTrue, sizeof(uTrue));
        }
        // set nonblocking operation
        if (fcntl(iSocket, F_SETFL, O_NONBLOCK) < 0)
        {
            NetPrintf(("dirtynetunix: error trying to make socket non-blocking (err=%d)\n", errno));
        }

        #if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
        // ignore SIGPIPE; we ignore this signal globally, but that doesn't work on iOS/MacX
        if (setsockopt(iSocket, SOL_SOCKET, SO_NOSIGPIPE, &uTrue, sizeof(uTrue)) < 0)
        {
            NetPrintf(("dirtynetunix: unable to set NOSIGPIPE on socket (err=%s)\n", DirtyErrGetName(errno)));
        }
        #endif
    }
    else
    {
        NetPrintf(("dirtynetunix: socket() failed (err=%s)\n", DirtyErrGetName(errno)));
    }
    return(iSocket);
}

/*F********************************************************************************/
/*!
    \Function    _SocketOpen

    \Description
        Create a new transfer endpoint. A socket endpoint is required for any
        data transfer operation.  If iSocket != -1 then used existing socket.

    \Input iSocket      - Socket descriptor to use or -1 to create new
    \Input iAddrFamily  - address family (AF_INET)
    \Input iType        - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input iProto       - protocol type for SOCK_RAW (unused by others)
    \Input iOpened      - 0=not open (connecting), 1=open

    \Output
        SocketT *       - socket reference

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static SocketT *_SocketOpen(int32_t iSocket, int32_t iAddrFamily, int32_t iType, int32_t iProto, int32_t iOpened)
{
    SocketStateT *pState = _Socket_pState;
    SocketT *pSocket;

    // allocate memory
    if ((pSocket = DirtyMemAlloc(sizeof(*pSocket), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetunix: unable to allocate memory for socket\n"));
        return(NULL);
    }
    memset(pSocket, 0, sizeof(*pSocket));

    // open a socket (force to AF_INET if we are opening it)
    if ((iSocket == -1) && ((iSocket = _SocketCreateSocket(iAddrFamily = AF_INET, iType, iProto)) < 0))
    {
        NetPrintf(("dirtynetunix: error %s creating socket\n", DirtyErrGetName(iSocket)));
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // create one-deep packet queue
    if ((pSocket->pRecvQueue = SocketPacketQueueCreate(1, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetunix: failed to create socket queue for socket\n"));
        close(iSocket);
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(NULL);
    }

    // set family/proto info
    pSocket->family = iAddrFamily;
    pSocket->type = iType;
    pSocket->proto = iProto;
    pSocket->socket = iSocket;
    pSocket->opened = iOpened;
    pSocket->iLastError = SOCKERR_NONE;
    pSocket->bAsyncRecv = ((iType == SOCK_DGRAM) || (iType == SOCK_RAW)) ? TRUE : FALSE;
    pSocket->bSendCbs = TRUE;

    // inititalize critical section
    #if SOCKET_ASYNCRECVTHREAD
    NetCritInit(&pSocket->recvcrit, "inet-recv");
    #endif

    // install into list
    NetCritEnter(NULL);
    pSocket->next = pState->pSockList;
    pState->pSockList = pSocket;
    NetCritLeave(NULL);

    // return the socket
    return(pSocket);
}

/*F********************************************************************************/
/*!
    \Function    _SocketReopen

    \Description
        Recreate a socket endpoint.

    \Input *pSocket     - socket ref

    \Output
        SocketT *       - socket ref on success, NULL for error

    \Notes
        This function should not be called from the async/idle thread,
        so socket recreation cannot happen in SocketRecvfrom.

    \Version 16/02/2012 (szhu)
*/
/********************************************************************************F*/
static SocketT *_SocketReopen(SocketT *pSocket)
{
    // we need recvcrit to prevent the socket from being used by the async/idle thread
    #if SOCKET_ASYNCRECVTHREAD
    // for non-virtual non-tcp sockets only
    if (!pSocket->bVirtual && ((pSocket->type == SOCK_DGRAM) || (pSocket->type == SOCK_RAW)))
    {
        SocketT *pResult = NULL;
        // acquire socket receive critical section
        NetCritEnter(&pSocket->recvcrit);
        NetPrintf(("dirtynetunix: trying to recreate the socket handle for %x->%d\n", pSocket, pSocket->socket));

        // close existing socket handle
        if (pSocket->socket >= 0)
        {
            close(pSocket->socket);
            pSocket->socket = INVALID_SOCKET;
        }

        // create socket
        if ((pSocket->socket = _SocketCreateSocket(pSocket->family, pSocket->type, pSocket->proto)) >= 0)
        {
            // set socket buffer size
            if (pSocket->rbufsize > 0)
            {
                SocketControl(pSocket, 'rbuf', pSocket->rbufsize, NULL, 0);
            }
            if (pSocket->sbufsize > 0)
            {
                SocketControl(pSocket, 'sbuf', pSocket->sbufsize, NULL, 0);
            }
            // bind if previous socket was bound to a specific port
            if (SockaddrInGetPort(&pSocket->local) != 0)
            {
                int32_t iResult;
                // always set reuseaddr flag for socket recreation
                SocketControl(pSocket, 'radr', 1, NULL, 0);
                // we don't call SocketBind() here (to avoid virtual socket translation)
                if ((iResult = bind(pSocket->socket, &pSocket->local, sizeof(pSocket->local))) < 0)
                {
                    pSocket->iLastError = _SocketTranslateError(iResult);
                    NetPrintf(("dirtynetunix: bind() to %a:%d failed (err=%s)\n",
                               SockaddrInGetAddr(&pSocket->local),
                               SockaddrInGetPort(&pSocket->local),
                               DirtyErrGetName(errno)));
                }
            }
            // connect if previous socket was connected to a specific remote
            if (SockaddrInGetPort(&pSocket->remote) != 0)
            {
                struct sockaddr SockAddr;
                // make a copy of remote
                ds_memcpy_s(&SockAddr, sizeof(SockAddr), &pSocket->remote, sizeof(pSocket->remote));
                SocketConnect(pSocket, &SockAddr, sizeof(SockAddr));
            }
            // success
            pSocket->brokenflag = 0;
            pResult = pSocket;
            NetPrintf(("dirtynetunix: socket recreation (%x->%d) succeeded.\n", pSocket, pSocket->socket));
        }
        else
        {
            pSocket->iLastError = _SocketTranslateError(pSocket->socket);
            NetPrintf(("dirtynetunix: socket recreation (%x) failed.\n", pSocket));
        }

        // release socket receive critical section
        NetCritLeave(&pSocket->recvcrit);
        return(pResult);
    }
    #endif
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _SocketClose

    \Description
        Disposes of a SocketT, including disposal of the SocketT allocated memory.  Does
        NOT dispose of the unix socket ref.

    \Input *pSocket - socket to close

    \Output
        int32_t     - negative=error, else zero

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketClose(SocketT *pSocket)
{
    SocketStateT *pState = _Socket_pState;
    uint32_t bSockInList;
    SocketT **ppSocket;

    // remove sock from linked list
    NetCritEnter(NULL);
    for (ppSocket = &pState->pSockList, bSockInList = FALSE; *ppSocket != NULL; ppSocket = &(*ppSocket)->next)
    {
        if (*ppSocket == pSocket)
        {
            *ppSocket = pSocket->next;
            bSockInList = TRUE;
            break;
        }
    }
    NetCritLeave(NULL);

    // make sure the socket is in the socket list (and therefore valid)
    if (bSockInList == FALSE)
    {
        NetPrintf(("dirtynetunix: warning, trying to close socket 0x%08x that is not in the socket list\n", (intptr_t)pSocket));
        return(-1);
    }

    // finish any idle call
    NetIdleDone();

    // mark as closed
    pSocket->socket = INVALID_SOCKET;
    pSocket->opened = FALSE;

    // kill critical section
    #if SOCKET_ASYNCRECVTHREAD
    NetCritKill(&pSocket->recvcrit);
    #endif

    // destroy packet queue
    if (pSocket->pRecvQueue != NULL)
    {
        SocketPacketQueueDestroy(pSocket->pRecvQueue);
    }

    // put into killed list
    NetCritEnter(NULL);
    pSocket->kill = pState->pSockKill;
    pState->pSockKill = pSocket;
    NetCritLeave(NULL);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    _SocketIdle

    \Description
        Call idle processing code to give connections time.

    \Input *pData   - pointer to socket state

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketIdle(void *pData)
{
    SocketStateT *pState = (SocketStateT *)pData;
    SocketT *pSocket;
    uint32_t uTick;

    // for access to g_socklist and g_sockkill
    NetCritEnter(NULL);

    // get current tick
    uTick = NetTick();

    // walk socket list and perform any callbacks
    for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->next)
    {
        // see if we should do callback
        if ((pSocket->callidle != 0) &&
            (pSocket->callback != NULL) &&
            (!pSocket->bInCallback) &&
            (NetTickDiff(uTick, pSocket->calllast) > (signed)pSocket->callidle))
        {
            pSocket->bInCallback = TRUE;
            (pSocket->callback)(pSocket, 0, pSocket->callref);
            pSocket->bInCallback = FALSE;
            pSocket->calllast = uTick = NetTick();
        }
    }

    // delete any killed sockets
    while ((pSocket = pState->pSockKill) != NULL)
    {
        pState->pSockKill = pSocket->kill;
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    // process hostname list, delete completed lookup requests
    SocketHostnameListProcess(&pState->pHostList, pState->iMemGroup, pState->pMemGroupUserData);

    // for access to g_socklist and g_sockkill
    NetCritLeave(NULL);
}

/*F********************************************************************************/
/*!
    \Function    _SocketPoll

    \Description
        Execute a blocking poll on input of all sockets.

    \Input pState       - pointer to module state
    \Input uPollMsec    - maximum number of milliseconds to block in poll

    \Version 04/02/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketPoll(SocketStateT *pState, uint32_t uPollMsec)
{
    struct pollfd aPollFds[1024];
    int32_t iPoll, iMaxPoll, iResult;
    SocketT *pSocket;

    // for access to socket list
    NetCritEnter(NULL);

    // walk socket list and find matching socket
    for (pSocket = pState->pSockList, iPoll = 0, iMaxPoll = 1024; (pSocket != NULL) && (iPoll < iMaxPoll); pSocket = pSocket->next)
    {
        // skip invalid sockets or sockets we have an error on
        if ((pSocket->socket == INVALID_SOCKET) || (pSocket->hasdata & 0x80))
        {
            continue;
        }

        // add socket to poll array
        aPollFds[iPoll].fd = pSocket->socket;
        aPollFds[iPoll].events = POLLIN;
        aPollFds[iPoll].revents = 0;

        // remember poll index
        pSocket->pollidx = iPoll++;
    }

    // release critical section
    NetCritLeave(NULL);

    // execute the poll
    iResult = poll(aPollFds, iPoll, uPollMsec);

    // if any sockets have pending data, figure out which ones
    if (iResult > 0)
    {
        uint32_t uCurTick;

        // re-acquire critical section
        NetCritEnter(NULL);

        // update sockets if there is data to be read or not
        for (pSocket = pState->pSockList, uCurTick = NetTick(); pSocket != NULL; pSocket = pSocket->next)
        {
            pSocket->hasdata += aPollFds[pSocket->pollidx].revents & POLLIN ? 1 : 0;
            if ((pSocket->socket != INVALID_SOCKET)
                && (pSocket->hasdata)
                && (!pSocket->bInCallback)
                && (pSocket->callback != 0)
                && (pSocket->callmask & CALLB_RECV))
            {
                pSocket->bInCallback = TRUE;
                pSocket->callback(pSocket, 0, pSocket->callref);
                pSocket->bInCallback = FALSE;
                pSocket->calllast = uCurTick;
            }

            // if we have a socket error, remove from future poll events
            if (aPollFds[pSocket->pollidx].revents & (POLLERR|POLLHUP))
            {
                pSocket->hasdata |= 0x80;
            }
        }

        // release the critical section
        NetCritLeave(NULL);
    }
    else if (iResult < 0)
    {
        NetPrintf(("dirtynetunix: poll() failed (err=%s)\n", DirtyErrGetName(errno)));
    }

    // return number of file descriptors with pending data
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _SocketLookupDone

    \Description
        Callback to determine if gethostbyname is complete.

    \Input *pHost   - pointer to host lookup record

    \Output
        int32_t     - zero=in progess, neg=done w/error, pos=done w/success

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketLookupDone(HostentT *pHost)
{
    // return current status
    return(pHost->done);
}

/*F********************************************************************************/
/*!
    \Function _SocketLookupFree

    \Description
        Release resources used by SocketLookup()

    \Input *pHost   - pointer to host lookup record

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketLookupFree(HostentT *pHost)
{
    // release resource
    pHost->refcount -= 1;
}

/*F********************************************************************************/
/*!
    \Function    _SocketLookupThread

    \Description
        Socket lookup thread

    \Input *_pRef       - thread argument (hostent record)

    \Output
        void *          - NULL

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void *_SocketLookupThread(void *_pRef)
{
    SocketStateT *pState = _Socket_pState;
    HostentT *pHost = (HostentT *)_pRef;
    int32_t iMemGroup, iResult;
    void *pMemGroupUserData;
    struct addrinfo Hints, *pList = NULL;

    // if state is null the module has been shut down
    if (pState == NULL)
    {
        return(NULL);
    }
    iMemGroup = pState->iMemGroup;
    pMemGroupUserData = pState->pMemGroupUserData;

    #if DIRTYCODE_LOGGING
    pthread_t iThreadId = pthread_self();
    #endif

    // start lookup
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = AF_INET;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;
    NetPrintf(("dirtynetunix: lookup thread start; name=%s (thid=%d)\n", pHost->name, iThreadId));

    if ((iResult = getaddrinfo(pHost->name, NULL, &Hints, &pList)) == 0)
    {
        // extract ip address
        pHost->addr = SocketNtohl(((struct sockaddr_in *)pList->ai_addr)->sin_addr.s_addr);

        // mark success
        NetPrintf(("dirtynetunix: %s=%a\n", pHost->name, pHost->addr));
        pHost->done = 1;

        // add hostname to cache
        SocketHostnameCacheAdd(pState->pHostnameCache, pHost->name, pHost->addr, 1 /*pState->iVerbose*/);

       // release memory
       freeaddrinfo(pList);
    }
    else
    {
        // unsuccessful
        NetPrintf(("dirtynetunix: getaddrinfo() failed err=%s\n", DirtyErrGetNameList(iResult, _GAI_ErrList)));
        pHost->done = -1;
    }

    NetPrintf(("dirtynetunix: lookup thread exit; name=%s (thid=%d)\n", pHost->name, iThreadId));

    // release thread-allocated refcount on hostname resource
    pHost->refcount -= 1;
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function    _SocketRecvfrom

    \Description
        Receive data from a remote host on a datagram socket.

    \Input *pSocket     - socket reference
    \Input *pBuf        - buffer to receive data
    \Input iLen         - length of recv buffer
    \Input *pFrom       - address data was received from (NULL=ignore)
    \Input *pFromLen    - length of address

    \Output
        int32_t         - positive=data bytes received, else error

    \Version 09/10/2004 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, struct sockaddr *pFrom, int32_t *pFromLen)
{
    int32_t iResult;

    if (pFrom != NULL)
    {
        // do the receive
        if ((iResult = (int32_t)recvfrom(pSocket->socket, pBuf, iLen, 0, pFrom, (socklen_t *)pFromLen)) > 0)
        {
            // save recv timestamp
            SockaddrInSetMisc(pFrom, NetTick());
        }
    }
    else
    {
        iResult = (int32_t)recv(pSocket->socket, pBuf, iLen, 0);
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    _SocketRecvData

    \Description
        Called when data is received by _SocketRecvThread(). If there is a callback
        registered than the socket is passed to that callback so the data may be
        consumed.

    \Input *pSocket - pointer to socket that has new data

    \Version 10/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketRecvData(SocketT *pSocket)
{
    // set up packet queue entry (already has data)
    pSocket->pRecvPacket->iPacketSize = pSocket->recvstat;
    ds_memcpy_s(&pSocket->pRecvPacket->PacketAddr, sizeof(pSocket->pRecvPacket->PacketAddr), &pSocket->recvaddr, sizeof(pSocket->recvaddr));

    // see if we should issue callback
    if ((pSocket->calllast != (unsigned)-1) && (pSocket->callback != NULL) && (pSocket->callmask & CALLB_RECV))
    {
        pSocket->calllast = (unsigned)-1;
        (pSocket->callback)(pSocket, 0, pSocket->callref);
        pSocket->calllast = NetTick();
    }
}

/*F********************************************************************************/
/*!
    \Function    _SocketRead

    \Description
        Attempt to read data from the given socket.

    \Input *pState  - pointer to module state
    \Input *pSocket - pointer to socket to read from

    \Output
        int32_t     - receive result

    \Version 10/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketRead(SocketStateT *pState, SocketT *pSocket)
{
    // if we already have data or this is a virtual socket
    if ((pSocket->recvstat != 0)  || (pSocket->bVirtual == TRUE))
    {
        return;
    }

    // get a packet queue entry to receive into
    pSocket->pRecvPacket = SocketPacketQueueAlloc(pSocket->pRecvQueue);

    // try and receive some data
    if ((pSocket->type == SOCK_DGRAM) || (pSocket->type == SOCK_RAW))
    {
        int32_t iFromLen = sizeof(pSocket->recvaddr);
        pSocket->recvstat = _SocketRecvfrom(pSocket, (char *)pSocket->pRecvPacket->aPacketData, sizeof(pSocket->pRecvPacket->aPacketData), &pSocket->recvaddr, &iFromLen);
    }
    else
    {
        pSocket->recvstat = _SocketRecvfrom(pSocket, (char *)pSocket->pRecvPacket->aPacketData, sizeof(pSocket->pRecvPacket->aPacketData), NULL, 0);
    }

    // if the read completed successfully, forward data to socket callback
    if (pSocket->recvstat > 0)
    {
        _SocketRecvData(pSocket);
    }
}

#if SOCKET_ASYNCRECVTHREAD
/*F********************************************************************************/
/*!
    \Function    _SocketRecvThread

    \Description
        Wait for incoming data and deliver it immediately to the socket callback,
        if registered.

    \Input  *pArg    - pointer to Socket module state

    \Version 10/21/2004 (jbrookes)
*/
/********************************************************************************F*/
static void *_SocketRecvThread(void *pArg)
{
    typedef struct PollListT
    {
        SocketT *aSockets[SOCKET_MAXPOLL];
        struct pollfd aPollFds[SOCKET_MAXPOLL];
        int32_t iCount;
    } PollListT;

    PollListT pollList, previousPollList;
    SocketT *pSocket;
    int32_t iListIndex, iResult;
    SocketStateT *pState = (SocketStateT *)pArg;

    // show we are alive
    NetPrintf(("dirtynetunix: recv thread running (thid=%d)\n", pState->iRecvThread));
    pState->iRecvLife = 1;

    // reset contents of pollList
    memset(&pollList, 0, sizeof(pollList));

    // loop until done
    while(pState->iRecvLife == 1)
    {
        // reset contents of previousPollList
        memset(&previousPollList, 0, sizeof(previousPollList));

        // make a copy of the poll list used for the last poll() call
        for (iListIndex = 0; iListIndex < pollList.iCount; iListIndex++)
        {
            // copy entry from pollList to previousPollList
            previousPollList.aSockets[iListIndex] = pollList.aSockets[iListIndex];
            previousPollList.aPollFds[iListIndex] = pollList.aPollFds[iListIndex];
        }
        previousPollList.iCount = pollList.iCount;

        // reset contents of pollList in preparation for the next poll() call
        memset(&pollList, 0, sizeof(pollList));

        // acquire global critical section for access to socket list
        NetCritEnter(NULL);

        // walk the socket list and do two things:
        //    1- if the socket is ready for reading, perform the read operation
        //    2- if the buffer in which inbound data is saved is empty, initiate a new low-level read operation for that socket
        for (pSocket = pState->pSockList; (pSocket != NULL) && (pollList.iCount < SOCKET_MAXPOLL); pSocket = pSocket->next)
        {
            // only handle non-virtual sockets with asyncrecv enabled
            if ((pSocket->bVirtual == FALSE) && (pSocket->socket != INVALID_SOCKET) && (pSocket->bAsyncRecv == TRUE))
            {
                // acquire socket critical section
                NetCritEnter(&pSocket->recvcrit);

                // was this socket in the poll list of the previous poll() call
                for (iListIndex = 0; iListIndex < previousPollList.iCount; iListIndex++)
                {
                    if (previousPollList.aSockets[iListIndex] == pSocket)
                    {
                        // socket was in previous poll list!
                        // now check if poll() notified that this socket is ready for reading
                        if (previousPollList.aPollFds[iListIndex].revents & POLLIN)
                        {
                            /*
                            Note:
                            The poll() doc states that some error codes returned by the function
                            may only apply to one of the sockets in the poll list. For this reason,
                            we check the polling result for all entries in the list regardless
                            of the return value of poll().
                            */

                            // ready for reading, so go ahead and read
                            _SocketRead(pState, previousPollList.aSockets[iListIndex]);
                        }
                        /*
                            POLLNVAL: The file descriptor is not open. we need to exclude this socket
                            handle from being added to the poll list, otherwise the poll() will keep
                            returning 1 at once.

                            Due to the race-condition (the main thread might have recreated the socket
                            before we reach here), 'brokenflag' is accumulated rather simply set to TRUE, and
                            the socket will be excluded from the poll list only if it's marked as broken
                            twice or more ('brokenflag' is reset to 0 when recreating the socket, so actually
                            the max possible value of 'brokenflag' is 2).
                        */
                        else if (previousPollList.aPollFds[iListIndex].revents & POLLNVAL)
                        {
                            NetPrintf(("dirtynetunix: marking socket (%x->%d) as broken upon POLLNVAL\n", pSocket, pSocket->socket));
                            pSocket->brokenflag++;
                        }
                        break;
                    }
                }

                // if no data is queued, add this socket to the poll list to be used by the next poll() call
                if ((pSocket->recvstat <= 0) && (pSocket->socket != INVALID_SOCKET) && (pSocket->brokenflag <= 1))
                {
                    // add socket to poll list
                    pollList.aSockets[pollList.iCount] = pSocket;
                    pollList.aPollFds[pollList.iCount].fd = pSocket->socket;
                    pollList.aPollFds[pollList.iCount].events = POLLIN;
                    pollList.iCount += 1;
                }

                // release socket critical section
                NetCritLeave(&pSocket->recvcrit);
            }
        }

        // release global critical section
        NetCritLeave(NULL);

        // any sockets?
        if (pollList.iCount > 0)
        {
            // poll for data (wait up to 50ms)
            iResult = poll(pollList.aPollFds, pollList.iCount, 50);

            if (iResult < 0)
            {
                NetPrintf(("dirtynetunix: poll() failed (err=%s)\n", DirtyErrGetName(errno)));

                // stall for 50ms because experiment shows that next call to poll() may not block
                // internally if a socket is alreay in error.
                usleep(50*1000);
            }
        }
        else
        {
            // no sockets, so stall for 50ms
            usleep(50*1000);
        }
    }

    // indicate we are done
    NetPrintf(("dirtynetunix: receive thread exit\n"));
    pState->iRecvLife = 0;
    return(NULL);
}
#endif // SOCKET_ASYNCRECVTHREAD



/*F********************************************************************************/
/*!
    \Function    _SocketGetMacAddress

    \Description
        Attempt to retreive MAC address of the system.

    \Input *pState  - pointer to module state

    \Output
        uint8_t     - TRUE if MAC address found, FALSE otherwise

    \Notes
        Usage of getifaddrs() is preferred over usage of ioctl() with a socket to save
        the socket creation step. However, not all platforms support the AF_LINK address
        family. In those cases, usage of ioctl() can't be avoided.

    \Version 05/12/2004 (mclouatre)
*/
/********************************************************************************F*/
static uint8_t _SocketGetMacAddress(SocketStateT *pState)
{
    int32_t iResult;
    uint8_t bFound =  TRUE;

#if defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_APPLEOSX)
    struct ifaddrs* pIntfList = NULL;
    struct ifaddrs* pIntf = NULL;

    // retrieve the current interfaces - returns 0 on success
    iResult = getifaddrs(&pIntfList);
    if (iResult == 0)
    {
        // loop through linked list of interfaces
        pIntf = pIntfList;
        while(pIntf != NULL)
        {
            // "en0" is the name of the wifi adapter on the iPhone
            if ((pIntf->ifa_addr->sa_family == AF_LINK) && strcmp(pIntf->ifa_name, "en0") == 0)
            {
                struct sockaddr_dl* pDataLinkSockAddr = (struct sockaddr_dl *)(pIntf->ifa_addr);

                if (pDataLinkSockAddr && pDataLinkSockAddr->sdl_alen == 6)
                {
                    ds_memcpy(pState->aMacAddr, LLADDR(pDataLinkSockAddr), 6);

                    NetPrintf(("dirtynetunix: mac address - %X:%X:%X:%X:%X:%X\n",
                              (uint32_t)pState->aMacAddr[0], (uint32_t)pState->aMacAddr[1], (uint32_t)pState->aMacAddr[2],
                              (uint32_t)pState->aMacAddr[3], (uint32_t)pState->aMacAddr[4], (uint32_t)pState->aMacAddr[5]));

                    break;
                }
            }

            pIntf = pIntf->ifa_next;
        }

        if (pIntf == NULL)
        {
            bFound = FALSE;  // signal that a MAC address could not be found
        }

        // free interface list returned by getifaddrs()
        freeifaddrs(pIntfList);
    }
    else
    {
        NetPrintf(("dirtynetunix: getifaddrs() returned nonzero status: %d\n", iResult));
        bFound = FALSE;  // signal that a MAC address could not be found
    }
#else
    struct ifreq req;
    int32_t fd;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
    {
        strncpy(req.ifr_name, "eth0", IFNAMSIZ);
        if ((iResult = ioctl(fd, SIOCGIFHWADDR, &req)) >= 0)
        {
            ds_memcpy(pState->aMacAddr, req.ifr_hwaddr.sa_data, 6);
        }
        else
        {
            NetPrintf(("dirtynetunix: (eth0) failed to query MAC address - SIOCGIFHWADDR ioctl failure %d\n", errno));

            strncpy(req.ifr_name, "wlan0", IFNAMSIZ);
            if ((iResult = ioctl(fd, SIOCGIFHWADDR, &req)) >= 0)
            {
                ds_memcpy(pState->aMacAddr, req.ifr_hwaddr.sa_data, 6);
            }
            else
            {
                NetPrintf(("dirtynetunix: (wlan0) failed to query MAC address - SIOCGIFHWADD ioctl failure %d\n", errno));
                bFound = FALSE;   // signal that a MAC address could not be found
            }
        }
        close(fd);
    }
    else
    {
        NetPrintf(("dirtynetunix: can't open socket %d for MAC address query with ioctl(SIOCGIFHWADDR)\n", errno));
        bFound = FALSE;   // signal that a MAC address could not be found
    }
#endif

    return(bFound);
}

/*** Public Functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function    SocketCreate

    \Description
        Create new instance of socket interface module.  Initializes all global
        resources and makes module ready for use.

    \Input iThreadPrio        - priority to start threads with
    \Input iThreadStackSize   - stack size to start threads with (in bytes)
    \Input iThreadCpuAffinity - cpu affinity to start threads with

    \Output
        int32_t               - negative=error, zero=success

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    #if SOCKET_ASYNCRECVTHREAD
    pthread_attr_t attr;
    int32_t iResult;
    #endif

    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintf(("dirtynetunix: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetunix: DirtySDK v%d.%d.%d.%d.%d\n", DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH));

    // alloc and init state ref
    if ((pState = DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetunix: unable to allocate module state\n"));
        return(-2);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iMaxPacket = SOCKET_MAXUDPRECV;

    if (iThreadPrio < 0)
    {
        pState->bSingleThreaded = TRUE;
    }

    // disable SIGPIPE (Linux-based system only)
    #if defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_APPLEIOS) || defined(DIRTYCODE_ANDROID)
    _SocketDisableSigpipe();
    #endif

    // startup network libs
    NetLibCreate(iThreadPrio, iThreadStackSize, iThreadCpuAffinity);

    // add our idle handler
    if (!pState->bSingleThreaded)
    {
        NetIdleAdd(&_SocketIdle, pState);
    }

    // create hostname cache
    if ((pState->pHostnameCache = SocketHostnameCacheCreate(iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetunix: unable to create hostname cache\n"));
        SocketDestroy((uint32_t)(-1));
        return(-3);
    }

    // create high-priority receive thread
    #if SOCKET_ASYNCRECVTHREAD
    if (!pState->bSingleThreaded)
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if ((iResult = pthread_create(&pState->iRecvThread, &attr, _SocketRecvThread, pState)) != 0)
        {
            NetPrintf(("dirtynetunix: unable to create recv thread (err=%s)\n", DirtyErrGetName(iResult)));
        }
        // wait for receive thread startup
        while (pState->iRecvLife == 0)
        {
            usleep(100);
        }
    }
    #endif

    // save state
    _Socket_pState = pState;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    SocketDestroy

    \Description
        Release resources and destroy module.

    \Input uShutdownFlags   - shutdown flags

    \Output
        int32_t             - negative=error, zero=success

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketDestroy(uint32_t uShutdownFlags)
{
    SocketStateT *pState = _Socket_pState;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtynetunix: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    NetPrintf(("dirtynetunix: shutting down\n"));

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
    if (!pState->bSingleThreaded)
    {
        // tell receive thread to quit
        pState->iRecvLife = 2;
        // wait for thread to terminate
        while (pState->iRecvLife > 0)
        {
            usleep(1*1000);
        }
    }
    #endif

    // close any remaining sockets
    while (pState->pSockList != NULL)
    {
        SocketClose(pState->pSockList);
    }

    // clear the kill list
    _SocketIdle(pState);

    // shut down network libs
    NetLibDestroy(0);

    // dispose of state
    DirtyMemFree(pState, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    _Socket_pState = NULL;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function    SocketOpen

    \Description
        Create a new transfer endpoint. A socket endpoint is required for any
        data transfer operation.

    \Input iAddrFamily  - address family (AF_INET)
    \Input iType        - socket type (SOCK_STREAM, SOCK_DGRAM, SOCK_RAW, ...)
    \Input iProto       - protocol type for SOCK_RAW (unused by others)

    \Output
        SocketT *       - socket reference

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
SocketT *SocketOpen(int32_t iAddrFamily, int32_t iType, int32_t iProto)
{
    return(_SocketOpen(-1, iAddrFamily, iType, iProto, 0));
}

/*F********************************************************************************/
/*!
    \Function    SocketClose

    \Description
        Close a socket. Performs a graceful shutdown of connection oriented protocols.

    \Input *pSocket     - socket reference

    \Output
        int32_t         - negative=error, else zero

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketClose(SocketT *pSocket)
{
    int32_t iSocket = pSocket->socket;

    // stop sending
    SocketShutdown(pSocket, SOCK_NOSEND);

    // dispose of SocketT
    if (_SocketClose(pSocket) < 0)
    {
        return(-1);
    }

    // close unix socket if allocated
    if (iSocket >= 0)
    {
        // close socket
        if (close(iSocket) < 0)
        {
            NetPrintf(("dirtynetunix: close() failed (err=%s)\n", DirtyErrGetName(errno)));
        }
    }

    // success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function SocketImport

    \Description
        Import a socket.  The given socket ref may be a SocketT, in which case a
        SocketT pointer to the ref is returned, or it can be an actual unix socket ref,
        in which case a SocketT is created for the unix socket ref.

    \Input uSockRef - socket reference

    \Output
        SocketT *   - pointer to imported socket, or NULL

    \Version 01/14/2005 (jbrookes)
*/
/********************************************************************************F*/
SocketT *SocketImport(intptr_t uSockRef)
{
    SocketStateT *pState = _Socket_pState;
    socklen_t iProtoSize;
    int32_t iProto;
    SocketT *pSock;

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
    iProtoSize = sizeof(iProto);
    if (getsockopt((int32_t)uSockRef, SOL_SOCKET, SO_TYPE, &iProto, &iProtoSize) == 0)
    {
        // create the socket
        pSock = _SocketOpen((int32_t)uSockRef, AF_INET, iProto, 0, 0);

        // update local and remote addresses
        SocketInfo(pSock, 'bind', 0, &pSock->local, sizeof(pSock->local));
        SocketInfo(pSock, 'peer', 0, &pSock->remote, sizeof(pSock->remote));

        // mark it as imported
        pSock->bImported = TRUE;
    }
    else
    {
        NetPrintf(("dirtynetunix: getsockopt(SO_TYPE) failed (err=%s)\n", DirtyErrGetName(errno)));
    }

    return(pSock);
}

/*F********************************************************************************/
/*!
    \Function SocketRelease

    \Description
        Release an imported socket.

    \Input *pSocket - pointer to socket

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
void SocketRelease(SocketT *pSocket)
{
    // if it wasn't imported, nothing to do
    if (pSocket->bImported == FALSE)
    {
        return;
    }

    // dispose of SocketT, but leave the sockref alone
    _SocketClose(pSocket);
}

/*F********************************************************************************/
/*!
    \Function    SocketShutdown

    \Description
        Perform partial/complete shutdown of socket indicating that either sending
        and/or receiving is complete.

    \Input *pSocket - socket reference
    \Input iHow     - SOCK_NOSEND and/or SOCK_NORECV

    \Output
        int32_t     - negative=error, else zero

    \Version 09/10/2004 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketShutdown(SocketT *pSocket, int32_t iHow)
{
    int32_t iResult=0;

    // only shutdown a connected socket
    if (pSocket->type != SOCK_STREAM)
    {
        pSocket->iLastError = SOCKERR_NONE;
        return(pSocket->iLastError);
    }

    // make sure socket ref is valid
    if (pSocket->socket == INVALID_SOCKET)
    {
        pSocket->iLastError = SOCKERR_NONE;
        return(pSocket->iLastError);
    }

    // translate how
    if (iHow == SOCK_NOSEND)
    {
        iHow = SHUT_WR;
    }
    else if (iHow == SOCK_NORECV)
    {
        iHow = SHUT_RD;
    }
    else if (iHow == (SOCK_NOSEND|SOCK_NORECV))
    {
        iHow = SHUT_RDWR;
    }

    // do the shutdown
    if (shutdown(pSocket->socket, iHow) < 0)
    {
        iResult = errno;
        NetPrintf(("dirtynetunix: shutdown() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    pSocket->iLastError = _SocketTranslateError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketBind

    \Description
        Bind a local address/port to a socket.

    \Input *pSocket - socket reference
    \Input *pName   - local address/port
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        If either address or port is zero, then they are filled in automatically.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketBind(SocketT *pSocket, const struct sockaddr *pName, int32_t iNameLen)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iResult;

    // make sure socket is valid
    if (pSocket->socket < 0)
    {
        NetPrintf(("dirtynetunix: attempt to bind invalid socket\n"));
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // save local address
    ds_memcpy_s(&pSocket->local, sizeof(pSocket->local), pName, sizeof(*pName));

    // is the bind port a virtual port?
    if (pSocket->type == SOCK_DGRAM)
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
                NetCritEnter(&pSocket->recvcrit);
                #endif
                // close winsock socket
                NetPrintf(("dirtynetunix: [%p] making socket bound to port %d virtual\n", pSocket, uPort));
                if (pSocket->socket != INVALID_SOCKET)
                {
                    shutdown(pSocket->socket, SOCK_NOSEND);
                    close(pSocket->socket);
                    pSocket->socket = INVALID_SOCKET;
                }
                /* increase socket queue size; this protects virtual sockets from having data pushed into
                   them and overwriting previous data that hasn't been read yet */
                pSocket->pRecvQueue = SocketPacketQueueResize(pSocket->pRecvQueue, 4, pState->iMemGroup, pState->pMemGroupUserData);
                // mark socket as virtual
                pSocket->virtualport = uPort;
                pSocket->bVirtual = TRUE;
                // release socket critical section
                #if SOCKET_ASYNCRECVTHREAD
                NetCritLeave(&pSocket->recvcrit);
                #endif
                return(0);
            }
        }
    }

    // do the bind
    if ((iResult = bind(pSocket->socket, pName, iNameLen)) < 0)
    {
        NetPrintf(("dirtynetunix: bind() to port %d failed (err=%s)\n", SockaddrInGetPort(pName), DirtyErrGetName(errno)));
    }
    else if (SockaddrInGetPort(&pSocket->local) == 0)
    {
        iNameLen = sizeof(pSocket->local);
        iResult = getsockname(pSocket->socket, &pSocket->local, (socklen_t *)&iNameLen);
        NetPrintf(("dirtynetunix: bind(port=0) succeeded, local address=%a:%d.\n",
                   SockaddrInGetAddr(&pSocket->local),
                   SockaddrInGetPort(&pSocket->local)));
    }

    pSocket->iLastError = _SocketTranslateError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketConnect

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

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketConnect(SocketT *pSocket, struct sockaddr *pName, int32_t iNameLen)
{
    int32_t iResult;

    NetPrintf(("dirtynetunix: connecting to %a:%d\n", SockaddrInGetAddr(pName), SockaddrInGetPort(pName)));

    // do the connect
    pSocket->opened = 0;
    if (((iResult = connect(pSocket->socket, pName, iNameLen)) < 0) && (errno != EINPROGRESS))
    {
        NetPrintf(("dirtynetunix: connect() failed (err=%s)\n", DirtyErrGetName(errno)));
    }
    // if connect succeeded (usually for udp sockets) or attempting to establish a non-blocking connection
    // save correct address
    else
    {
        ds_memcpy_s(&pSocket->remote, sizeof(pSocket->remote), pName, sizeof(*pName));
    }

    pSocket->iLastError = _SocketTranslateError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketListen

    \Description
        Start listening for an incoming connection on the socket.  The socket must already
        be bound and a stream oriented connection must be in use.

    \Input *pSocket - socket reference to bound socket (see SocketBind())
    \Input iBacklog - number of pending connections allowed

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketListen(SocketT *pSocket, int32_t iBacklog)
{
    int32_t iResult;

    // do the listen
    if ((iResult = listen(pSocket->socket, iBacklog)) < 0)
    {
        NetPrintf(("dirtynetunix: listen() failed (err=%s)\n", DirtyErrGetName(errno)));
    }

    pSocket->iLastError = _SocketTranslateError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketAccept

    \Description
        Accept an incoming connection attempt on a socket.

    \Input *pSocket     - socket reference to socket in listening state (see SocketListen())
    \Input *pAddr       - pointer to storage for address of the connecting entity, or NULL
    \Input *pAddrLen    - pointer to storage for length of address, or NULL

    \Output
        SocketT *       - the accepted socket, or NULL if not available

    \Notes
        The integer pointed to by addrlen should on input contain the number of characters
        in the buffer addr.  On exit it will contain the number of characters in the
        output address.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
SocketT *SocketAccept(SocketT *pSocket, struct sockaddr *pAddr, int32_t *pAddrLen)
{
    SocketT *pOpen = NULL;
    int32_t iResult;

    pSocket->iLastError = SOCKERR_INVALID;

    // make sure we have an INET socket
    if ((pSocket->socket == INVALID_SOCKET) || (pSocket->family != AF_INET))
    {
        return(NULL);
    }

    // make sure turn parm is valid
    if ((pAddr != NULL) && (*pAddrLen < (signed)sizeof(struct sockaddr)))
    {
        return(NULL);
    }

    // perform inet accept
    if (pAddrLen)
    {
        *pAddrLen = sizeof(*pAddr);
    }

    iResult = accept(pSocket->socket, pAddr, (socklen_t *)pAddrLen);
    if (iResult > 0)
    {
        // Allocate socket structure and install in list
        pOpen = _SocketOpen(iResult, pSocket->family, pSocket->type, pSocket->proto, 1);
        pSocket->iLastError = SOCKERR_NONE;

        #if defined(DIRTYCODE_ANDROID) || defined(DIRTYCODE_LINUX)
        /*
        http://linux.die.net/man/2/accept:
        On Linux, the new socket returned by accept() does not inherit file status flags
        such as O_NONBLOCK and O_ASYNC from the listening socket. This behaviour differs
        from the canonical BSD sockets implementation.
        */
        if (fcntl(pSocket->socket, F_GETFL, O_NONBLOCK))
        {
            // set nonblocking operation
            if (fcntl(iResult, F_SETFL, O_NONBLOCK) < 0)
            {
                NetPrintf(("dirtynetunix: error trying to make socket non-blocking (err=%d)\n", errno));
            }
        }
        #endif
    }
    else
    {
        pSocket->iLastError = _SocketTranslateError(iResult);

        if (errno != EWOULDBLOCK)
        {
            NetPrintf(("dirtynetunix: accept() failed (err=%s)\n", DirtyErrGetName(errno)));
        }
    }

    // return the socket
    return(pOpen);
}

/*F********************************************************************************/
/*!
    \Function    SocketSendto

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

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketSendto(SocketT *pSocket, const char *pBuf, int32_t iLen, int32_t iFlags, const struct sockaddr *pTo, int32_t iToLen)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iResult = -1;

    if (pSocket->bSendCbs)
    {
        // if installed, give socket callback right of first refusal
        if ((iResult = SocketSendCallbackInvoke(&pState->aSendCbEntries[0], pSocket, pSocket->type, pBuf, iLen, pTo)) > 0)
        {
            return(iResult);
        }
    }

    // make sure socket ref is valid
    if (pSocket->socket < 0)
    {
        #if DIRTYCODE_LOGGING
        uint32_t uAddr = 0, uPort = 0;
        if (pTo)
        {
            uAddr = SockaddrInGetAddr(pTo);
            uPort = SockaddrInGetPort(pTo);
        }
        NetPrintf(("dirtynetunix: attempting to send to %a:%d on invalid socket\n", uAddr, uPort));
        #endif
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // handle optional data rate throttling
    if ((iLen = SocketRateThrottle(&pSocket->SendRate, pSocket->type, iLen, "send")) == 0)
    {
        return(0);
    }

    // use appropriate version
    if (pTo == NULL)
    {
        if ((iResult = (int32_t)send(pSocket->socket, pBuf, iLen, 0)) < 0)
        {
            if (errno != EWOULDBLOCK)
                NetPrintf(("dirtynetunix: send() failed (err=%s)\n", DirtyErrGetName(errno)));
        }
    }
    else
    {
        // do the send
        #if SOCKET_VERBOSE
        NetPrintf(("dirtynetunix: sending %d bytes to %a:%d\n", iLen, SockaddrInGetAddr(pTo), SockaddrInGetPort(pTo)));
        #endif
        if ((iResult = (int32_t)sendto(pSocket->socket, pBuf, iLen, 0, pTo, iToLen)) < 0)
        {
            NetPrintf(("dirtynetunix: sendto(%a:%d) failed (err=%s)\n", SockaddrInGetAddr(pTo), SockaddrInGetPort(pTo), DirtyErrGetName(errno)));
        }
    }
    // translate error
    pSocket->iLastError = iResult = _SocketTranslateError(iResult);
    if (iResult == SOCKERR_BADPIPE)
    {
        // recreate socket (iLastError will be set in _SocketReopen)
        if (_SocketReopen(pSocket))
        {
            // success, re-send (should either work or error out)
            return(SocketSendto(pSocket, pBuf, iLen, iFlags, pTo, iToLen));
        }
        // failed to recreate the socket, error
    }

    // update data rate estimation
    SocketRateUpdate(&pSocket->SendRate, iResult, "send");
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    SocketRecvfrom

    \Description
        Receive data from a remote host. If socket is a connected stream, then data can
        only come from that source. A datagram socket can receive from any remote host.

    \Input *pSocket     - socket reference
    \Input *pBuf        - buffer to receive data
    \Input iLen         - length of recv buffer
    \Input iFlags       - unused
    \Input *pFrom       - address data was received from (NULL=ignore)
    \Input *pFromLen    - length of address

    \Output
        int32_t         - positive=data bytes received, else standard error code

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pFrom, int32_t *pFromLen)
{
    SocketStateT *pState = _Socket_pState;
    int32_t iRecv = -1;

    // clear "hasdata" hint
    pSocket->hasdata = 0;

    // handle rate throttling, if enabled
    if ((iLen = SocketRateThrottle(&pSocket->RecvRate, pSocket->type, iLen, "recv")) == 0)
    {
        return(0);
    }

    // receiving on an asyncrecv socket?
    if (pSocket->bAsyncRecv == TRUE)
    {
        // do not try to get a packet if no buffer
        if (iLen <= 0)
        {
            return(0);
        }

        // acquire socket receive critical section
        #if SOCKET_ASYNCRECVTHREAD
        NetCritEnter(&pSocket->recvcrit);
        #endif

        // get a packet
        iRecv = SocketPacketQueueRem(pSocket->pRecvQueue, (uint8_t *)pBuf, iLen, &pSocket->recvaddr);

        // see if we have any data in socket receive buffer (could be async, could be pushed)
        if ((iRecv > 0) || (pSocket->recvstat != 0))
        {
            // copy sender
            if (pFrom != NULL)
            {
                ds_memcpy_s(pFrom, sizeof(*pFrom), &pSocket->recvaddr, sizeof(pSocket->recvaddr));
                *pFromLen = sizeof(*pFrom);
            }

            // mark data as consumed
            pSocket->recvstat = 0;

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
            if (pSocket->callback == NULL)
            {
                _SocketRead(_Socket_pState, pSocket);
            }

        }
        else if ((pState->bSingleThreaded) && (pSocket->socket != INVALID_SOCKET))
        {
            // do direct recv call
            if (((iRecv = _SocketRecvfrom(pSocket, pBuf, iLen, pFrom, pFromLen)) < 0) && (errno != EAGAIN))
            {
                NetPrintf(("dirtynetunix: _SocketRecvfrom() failed (err=%s)\n", DirtyErrGetName(errno)));
            }
        }

        // release socket receive critical section
        #if SOCKET_ASYNCRECVTHREAD
        NetCritLeave(&pSocket->recvcrit);
        #endif

        // do udp error conversion
        iRecv = (iRecv == 0) ? SOCKERR_NONE : _SocketTranslateError(iRecv);
    }
    else // non-async recvthread socket
    {
        // make sure socket ref is valid
        if (pSocket->socket == INVALID_SOCKET)
        {
            pSocket->iLastError = SOCKERR_INVALID;
            return(pSocket->iLastError);
        }
        // do direct recv call
        if (((iRecv = _SocketRecvfrom(pSocket, pBuf, iLen, pFrom, pFromLen)) < 0) && (errno != EAGAIN))
        {
            NetPrintf(("dirtynetunix: _SocketRecvfrom() failed on a SOCK_STREAM socket (err=%s)\n", DirtyErrGetName(errno)));
        }
        // do tcp error conversion
        iRecv = (iRecv == 0) ? SOCKERR_CLOSED : _SocketTranslateError(iRecv);
    }

    // update data rate estimation
    SocketRateUpdate(&pSocket->RecvRate, iRecv, "recv");

    // return the error code
    pSocket->iLastError = iRecv;
    return(iRecv);
}

/*F********************************************************************************/
/*!
    \Function    SocketInfo

    \Description
        Return information about an existing socket.

    \Input *pSocket - socket reference
    \Input iInfo    - selector for desired information
    \Input iData    - selector specific
    \Input *pBuf    - return buffer
    \Input iLen     - buffer length

    \Output
        int32_t     - selector-specific

    \Notes
        iInfo can be one of the following:

        \verbatim
            'addr' - returns interface address; iData=destination address for routing
            'bind' - return bind data (if pSocket == NULL, get socket bound to given port)
            'bndu' - return bind data (only with pSocket=NULL, get SOCK_DGRAM socket bound to given port)
            'conn' - connection status
            'eth0' - returns 0 if we have a valid ip for eth0 device. negative if error or no address found (Android Only)
            'ethr'/'macx' - local ethernet address (returned in pBuf), 0=success, negative=error
            'maxp' - return configured max packet size
            'maxr' - return configured max recv rate (bytes/sec; zero=uncapped)
            'maxs' - return configured max send rate (bytes/sec; zero=uncapped)
            'peer' - peer info (only valid if connected)
            'ratr' - return current recv rate estimation (bytes/sec)
            'rats' - return current send rate estimation (bytes/sec)
            'read' - return if socket has data available for reading
            'sdcf' - get installed send callback function pointer (iData specifies index in array)
            'sdcu' - get installed send callback userdata pointer (iData specifies index in array)
            'serr' - last socket error
            'sock' - return socket associated with the specified DirtySock socket
            'stat' - socket status
            'virt' - TRUE if socket is virtual, else FALSE
            'wan0' - returns true if we have a valid ip for wlan0 device. Negative if error or no address found (Android Only)
        \endverbatim

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
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
            #if defined(DIRTYCODE_APPLEIOS) //$$TODO -- evaluate
            if (pState->uLocalAddr == 0)
            {
                // get local address here, or possibly at network startup
                struct ifaddrs* interfaces = NULL;

                int error = getifaddrs(&interfaces);
                if (error == 0)
                {
                    struct ifaddrs *currentAddress;
                    for (currentAddress = interfaces; currentAddress; currentAddress = currentAddress->ifa_next)
                    {
                        // only consider live inet interfaces and return first valid non-loopback address
                        if ((currentAddress->ifa_addr->sa_family == AF_INET) && ((currentAddress->ifa_flags & (IFF_LOOPBACK | IFF_UP)) == IFF_UP))
                        {
                            struct sockaddr_in* HostAddr = (struct sockaddr_in*)currentAddress->ifa_addr;
                            pState->uLocalAddr = ntohl(HostAddr->sin_addr.s_addr);
                            break;
                        }
                    }

                    freeifaddrs(interfaces);
                }
            }
            return(pState->uLocalAddr);
            #else
            struct sockaddr HostAddr, DestAddr;
            SockaddrInit(&DestAddr, AF_INET);
            SockaddrInSetAddr(&DestAddr, (uint32_t)iData);
            if (SocketHost(&HostAddr, sizeof(HostAddr), &DestAddr, sizeof(DestAddr)) != -1)
            {
                return(SockaddrInGetAddr(&HostAddr));
            }
            else
            {
                return(-1);
            }
            #endif
        }
        // get socket bound to given port
        if ((iInfo == 'bind') || (iInfo == 'bndu'))
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
                if ((iInfo == 'bind') || ((iInfo == 'bndu') && (pSocket->type == SOCK_DGRAM)))
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

        if (iInfo == 'conn')
        {
            return(pState->uConnStatus);
        }

        #ifdef DIRTYCODE_ANDROID
        if (iInfo == 'eth0' || iInfo == 'wan0')
        {
            int32_t iRet = -1;
            int32_t fd = socket(AF_INET, SOCK_DGRAM, 0);
            struct ifreq ifr;

            if (fd == -1)
            {
                NetPrintf(("dirtynetunix: SocketInfo('eth0/wan0') cannot create socket descriptor.\n"));
                iRet = -2;
            }
            else
            {
                if (iInfo == 'eth0')
                {
                    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
                }
                else
                {
                    strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ);
                }

                if (ioctl(fd, SIOCGIFADDR, &ifr) == -1)
                {
                    close(fd);
                    NetPrintf(("dirtynetunix: SocketInfo('%c') cannot find an IP address for device %s. Errno %s\n", iInfo, ifr.ifr_name, strerror(errno)));
                    iRet = -3;
                }
                else
                {
                    close(fd);
                    struct sockaddr_in* ipaddr = (struct sockaddr_in*)&ifr.ifr_addr;
                    if (ipaddr->sin_addr.s_addr != 0)
                    {
                        NetPrintf(("dirtynetunix: SocketInfo('%c') network address found for %s.\n", iInfo, ifr.ifr_name));
                        iRet = 0;
                    }
                    else
                    {
                        NetPrintf(("dirtynetunix: SocketInfo('%c') cannot find an IP address for device %s.\n", iInfo, ifr.ifr_name));
                        iRet = -4;
                    }
                }
            }

            return(iRet);
        }
        #endif

        // get MAC address
        if ((iInfo == 'ethr') || (iInfo == 'macx'))
        {
            uint8_t aZeros[6] = { 0, 0, 0, 0, 0, 0 };
            uint8_t bFound = TRUE;

            // early exit if user-provided buffer not correct
            if ((pBuf == NULL) && (iLen < (signed)sizeof(pState->aMacAddr)))
            {
                return(-1);
            }

            // try to get mac address if we don't already have it
            if (!memcmp(pState->aMacAddr, aZeros, sizeof(pState->aMacAddr)))
            {
                bFound = _SocketGetMacAddress(pState);
            }

            if (bFound)
            {
                // copy MAC address in user-provided buffer and signal success
                ds_memcpy(pBuf, &pState->aMacAddr, sizeof(pState->aMacAddr));
                return(0);
            }

            // signal failure - no MAC address found
            return(-1);
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

            NetPrintf(("dirtynetunix: 'sdcf' selector used with invalid paramaters\n"));
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

            NetPrintf(("dirtynetunix: 'sdcu' selector used with invalid paramaters\n"));
            return(-1);
        }

        NetPrintf(("dirtynetunix: unhandled global SocketInfo() selector '%C'\n", iInfo));

        return(-1);
    }

    // return local bind data
    if (iInfo == 'bind')
    {
        if (iLen >= (signed)sizeof(pSocket->local))
        {
            if (pSocket->bVirtual == TRUE)
            {
                SockaddrInit((struct sockaddr *)pBuf, AF_INET);
                SockaddrInSetPort((struct sockaddr *)pBuf, pSocket->virtualport);
            }
            else
            {
                getsockname(pSocket->socket, pBuf, (socklen_t *)&iLen);
            }
            return(0);
        }
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

    // return whether the socket is virtual or not
    if (iInfo == 'virt')
    {
        return(pSocket->bVirtual);
    }

    // make sure there is a valid socket ref
    if (pSocket->socket == INVALID_SOCKET)
    {
        return(-2);
    }

    // return local peer data
    if ((iInfo == 'conn') || (iInfo == 'peer'))
    {
        if (iLen >= (signed)sizeof(pSocket->local))
        {
            getpeername(pSocket->socket, pBuf, (socklen_t *)&iLen);
        }
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

    // return if socket has data
    if (iInfo == 'read')
    {
        return(pSocket->hasdata);
    }

    // return last socket error
    if (iInfo == 'serr')
    {
        return(pSocket->iLastError);
    }

    // return unix socket ref
    if (iInfo == 'sock')
    {
        return(pSocket->socket);
    }

    // return socket status
    if (iInfo == 'stat')
    {
        struct pollfd PollFd;

        // if not a connected socket, return TRUE
        if (pSocket->type != SOCK_STREAM)
        {
            return(1);
        }

        // if not connected, use poll to determine connect
        if (pSocket->opened == 0)
        {
            memset(&PollFd, 0, sizeof(PollFd));
            PollFd.fd = pSocket->socket;
            PollFd.events = POLLOUT;
            if (poll(&PollFd, 1, 0) != 0)
            {
                /*
                    Experimentation shows that on connect failed:
                    Android: (only) POLLERR is returned.
                    iOS5: (only) POLLHUP is returned.
                    Linux: both POLLERR and POLLHUP are returned (POLLERR|POLLHUP).

                    To make the code work on all platforms, we test if any of POLLERR and POLLHUP was set.
                */
                if ((PollFd.revents & POLLERR) || (PollFd.revents & POLLHUP))
                {
                    NetPrintf(("dirtynetunix: read exception on connect\n"));
                    pSocket->opened = -1;
                }
                // if socket is writable, that means connect succeeded
                else if (PollFd.revents & POLLOUT)
                {
                    NetPrintf(("dirtynetunix: connection open\n"));
                    pSocket->opened = 1;
                }
            }
        }

        // if previously connected, make sure connect still valid
        if (pSocket->opened > 0)
        {
            memset(&PollFd, 0, sizeof(PollFd));
            PollFd.fd = pSocket->socket;
            PollFd.events = POLLIN;
            if (poll(&PollFd, 1, 0) != 0)
            {
                // if we got an exception, that means connect failed (usually closed by remote peer)
                if ((PollFd.revents & POLLERR) || (PollFd.revents & POLLHUP))
                {
                    NetPrintf(("dirtynetunix: connection failure\n"));
                    pSocket->opened = -1;
                }
                else if (PollFd.revents & POLLIN)
                {
                    int32_t iAvailBytes = 1;
                    // get number of bytes for read (might be less than actual bytes, so it can only be used for zero-test)
                    if (ioctl(pSocket->socket, FIONREAD, &iAvailBytes) != 0)
                    {
                        NetPrintf(("dirtynetunix: ioctl(FIONREAD) failed (err=%s).\n", DirtyErrGetName(errno)));
                    }
                    // if socket is readable but there's no data available for read, connect was closed
                    else if (iAvailBytes == 0)
                    {
                        pSocket->iLastError = SOCKERR_CLOSED;
                        NetPrintf(("dirtynetunix: connection closed\n"));
                        pSocket->opened = -1;
                    }
                }
            }
        }

        // return connect status
        return(pSocket->opened);
    }

    // unhandled option?
    NetPrintf(("dirtynetunix: unhandled SocketInfo() option '%C'\n", iInfo));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    SocketCallback

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

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketCallback(SocketT *pSocket, int32_t iMask, int32_t iIdle, void *pRef, int32_t (*pProc)(SocketT *pSock, int32_t iFlags, void *pRef))
{
    pSocket->callidle = iIdle;
    pSocket->callmask = iMask;
    pSocket->callref = pRef;
    pSocket->callback = pProc;
    pSocket->calllast = NetTick() - iIdle;
    return(0);
}

/*F********************************************************************************/
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
        int32_t     - message specific result (-1=unsupported message)

    \Notes
        iOption can be one of the following:

        \verbatim
            'arcv' - set async receive enable/disable (default enabled for DGRAM/RAW, disabled for TCP)
            'conn' - init network stack
            'disc' - bring down network stack
            'idle' - perform any network connection related processing
            'maxp' - set max udp packet size
            'maxr' - set max recv rate (bytes/sec; zero=uncapped)
            'maxs' - set max send rate (bytes/sec; zero=uncapped)
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'ndly' - set TCP_NODELAY state for given stream socket (iData1=zero or one)
            'pdev' - set simulated packet deviation
            'plat' - set simulated packet latency
            'plos' - set simulated packet loss
            'poll' - block waiting on input from socket list (iData1=ms to block)
            'push' - push data into given socket receive buffer (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'radr' - set SO_REUSEADDR On the specified socket
            'rbuf' - set socket recv buffer size
            'sbuf' - set socket send buffer size
            'scbk' - enable/disable "send callbacks usage" on specified socket (defaults to enable)
            'sdcb' - set/unset send callback (iData1=TRUE for set - FALSE for unset, pData2=callback, pData3=callref)
            'vadd' - add a port to virtual port list
            'vdel' - del a port from virtual port list
        \endverbatim

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
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
        return(0);
    }
    #endif // SOCKET_ASYNCRECVTHREAD
    // init network stack and bring up interface
    if (iOption == 'conn')
    {
        #if defined(DIRTYCODE_APPLEIOS) //$$TODO - evaluate
        CFStringRef URLString = CFStringCreateWithCString(kCFAllocatorDefault, "http://gos.ea.com/util/test.jsp", kCFStringEncodingASCII);
        CFStringRef getString = CFStringCreateWithCString(kCFAllocatorDefault, "GET", kCFStringEncodingASCII);
        CFURLRef baseURL = NULL;
        CFURLRef url = CFURLCreateWithString(kCFAllocatorDefault, URLString, baseURL);
        CFHTTPMessageRef request = CFHTTPMessageCreateRequest(NULL, getString, url, kCFHTTPVersion1_1);
        CFMutableDataRef data = CFDataCreateMutable(NULL, 0);
        CFReadStreamRef readStream = CFReadStreamCreateForHTTPRequest(NULL, request);

        if (CFReadStreamOpen(readStream))
        {
            char done = FALSE;
            do
            {
                const int BUFSIZE = 4096;
                unsigned char buf[BUFSIZE];
                int bytesRead = (int)CFReadStreamRead(readStream, buf, BUFSIZE);
                if (bytesRead > 0)
                {
                    CFDataAppendBytes(data, buf, bytesRead);
                }
                else if (bytesRead == 0)
                {
                    done = TRUE;
                }
                else
                {
                    done = TRUE;
                }
            } while (!done);
        }

        CFReadStreamClose(readStream);
        CFRelease(readStream);
        readStream = nil;

        CFRelease(url);
        url = NULL;

        CFRelease(data);
        data = NULL;

        CFRelease(URLString);
        URLString = NULL;

        CFRelease(getString);
        getString = NULL;
        #endif

        pState->uConnStatus = '+onl';
        return(0);
    }
    // bring down interface
    if (iOption == 'disc')
    {
        NetPrintf(("dirtynetunix: disconnecting from network\n"));
        pState->uConnStatus = '-off';
        return(0);
    }
    // handle any idle processing required
    if (iOption == 'idle')
    {
        // in single-threaded mode, we have to give life to the network idle process
        if (pState->bSingleThreaded)
        {
            _SocketIdle(pState);
        }
        return(0);
    }
    // set max udp packet size
    if (iOption == 'maxp')
    {
        NetPrintf(("dirtynetunix: setting max udp packet size to %d\n", iData1));
        pState->iMaxPacket = iData1;
        return(0);
    }
    // set max recv rate
    if (iOption == 'maxr')
    {
        NetPrintf(("dirtynetunix: setting max recv rate to %d bytes/sec\n", iData1));
        pSocket->RecvRate.uMaxRate = iData1;
        return(0);
    }
    // set max send rate
    if (iOption == 'maxs')
    {
        NetPrintf(("dirtynetunix: setting max send rate to %d bytes/sec\n", iData1));
        pSocket->SendRate.uMaxRate = iData1;
        return(0);
    }
    // if a stream socket, set nonblocking/blocking mode
    if ((iOption == 'nbio') && (pSocket != NULL) && (pSocket->type == SOCK_STREAM))
    {
        int32_t iVal = fcntl(pSocket->socket, F_GETFL, O_NONBLOCK);
        iVal = iData1 ? (iVal | O_NONBLOCK) : (iVal & ~O_NONBLOCK);
        iResult = fcntl(pSocket->socket, F_SETFL, iVal);
        pSocket->iLastError = _SocketTranslateError(iResult);
        NetPrintf(("dirtynetunix: setting socket:0x%x to %s mode %s (LastError=%d).\n", pSocket, iData1 ? "nonblocking" : "blocking", iResult ? "failed" : "succeeded", pSocket->iLastError));
        return(pSocket->iLastError);
    }
    // if a stream socket, set TCP_NODELAY state
    if ((iOption == 'ndly') && (pSocket != NULL) && (pSocket->type == SOCK_STREAM))
    {
        iResult = setsockopt(pSocket->socket, IPPROTO_TCP, TCP_NODELAY, &iData1, sizeof(iData1));
        pSocket->iLastError = _SocketTranslateError(iResult);
        return(pSocket->iLastError);
    }
    // set simulated packet loss or packet latency
    #if SOCKET_ASYNCRECVTHREAD
    if ((iOption == 'pdev') || (iOption == 'plat') || (iOption == 'plos'))
    {
        // acquire socket receive critical section
        NetCritEnter(&pSocket->recvcrit);
        // forward selector to packet queue
        iResult = SocketPacketQueueControl(pSocket->pRecvQueue, iOption, iData1);
        // release socket receive critical section
        NetCritLeave(&pSocket->recvcrit);
        return(iResult);
    }
    #endif
    // block waiting on input from socket list
    if (iOption == 'poll')
    {
        return(pState->bSingleThreaded ? _SocketPoll(pState, (unsigned)iData1) : -1);
    }
    // change packet queue size
    #if SOCKET_ASYNCRECVTHREAD
    if (iOption == 'pque')
    {
        // acquire socket receive critical section
        NetCritEnter(&pSocket->recvcrit);
        // resize the queue
        pSocket->pRecvQueue = SocketPacketQueueResize(pSocket->pRecvQueue, iData1, pState->iMemGroup, pState->pMemGroupUserData);
        // release socket receive critical section
        NetCritLeave(&pSocket->recvcrit);
    }
    #endif
    // push data into receive buffer
    if (iOption == 'push')
    {
        if (pSocket != NULL)
        {
            // acquire socket critical section
            #if SOCKET_ASYNCRECVTHREAD
            NetCritEnter(&pSocket->recvcrit);
            #endif

            // don't allow data that is too large (for the buffer) to be pushed
            if (iData1 > SOCKET_MAXUDPRECV)
            {
                NetPrintf(("dirtynetunix: request to push %d bytes of data discarded (max=%d)\n", iData1, SOCKET_MAXUDPRECV));
                #if SOCKET_ASYNCRECVTHREAD
                NetCritLeave(&pSocket->recvcrit);
                #endif
                return(-1);
            }

            // add packet to queue
            SocketPacketQueueAdd(pSocket->pRecvQueue, (uint8_t *)pData2, iData1, (struct sockaddr *)pData3);
            // remember we have data
            pSocket->hasdata = 1;

            // release socket critical section
            #if SOCKET_ASYNCRECVTHREAD
            NetCritLeave(&pSocket->recvcrit);
            #endif

            // see if we should issue callback
            if ((pSocket->callback != NULL) && (pSocket->callmask & CALLB_RECV))
            {
                pSocket->callback(pSocket, 0, pSocket->callref);
            }
        }
        else
        {
            NetPrintf(("dirtynetunix: warning - call to SocketControl('push') ignored (pSocket=%p)\n", pSocket));
            return(-1);
        }
        return(0);
    }
    // set SO_REUSEADDR
    if (iOption == 'radr')
    {
        iResult = setsockopt(pSocket->socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&iData1, sizeof(iData1));
        pSocket->iLastError = _SocketTranslateError(iResult);
        return(pSocket->iLastError);
    }
    // set socket receive buffer size
    if ((iOption == 'rbuf') || (iOption == 'sbuf'))
    {
        int32_t iOldSize, iNewSize;
        int32_t iSockOpt = (iOption == 'rbuf') ? SO_RCVBUF : SO_SNDBUF;
        socklen_t uOptLen = 4;

        // get current buffer size
        getsockopt(pSocket->socket, SOL_SOCKET, iSockOpt, (char *)&iOldSize, &uOptLen);

        // set new size
        iResult = setsockopt(pSocket->socket, SOL_SOCKET, iSockOpt, (const char *)&iData1, sizeof(iData1));
        if ((pSocket->iLastError = _SocketTranslateError(iResult)) == SOCKERR_NONE)
        {
            // save new buffer size
            if (iOption == 'rbuf')
            {
                pSocket->rbufsize = iData1;
            }
            else
            {
                pSocket->sbufsize = iData1;
            }
        }

        // get new size
        getsockopt(pSocket->socket, SOL_SOCKET, iSockOpt, (char *)&iNewSize, &uOptLen);
        #if defined(DIRTYCODE_LINUX)
        /* as per SO_RCVBUF/SO_SNDBUF documentation: "The kernel doubles the value (to allow space for bookkeeping
           overhead) when it is set using setsockopt(), and this doubled value is returned by getsockopt()."  To
           account for this we halve the getsockopt() value so it matches what we requested. */
        iNewSize /= 2;
        #endif
        NetPrintf(("dirtynetunix: setsockopt(%s) changed buffer size from %d to %d\n", (iOption == 'rbuf') ? "SO_RCVBUF" : "SO_SNDBUF",
            iOldSize, iNewSize));

        return(pSocket->iLastError);
    }
    // enable/disable "send callbacks usage" on specified socket (defaults to enable)
    if (iOption == 'scbk')
    {
        if (pSocket->bSendCbs != (iData1?TRUE:FALSE))
        {
            NetPrintf(("dirtynetunix: send callbacks usage changed from %s to %s for socket ref %p\n", (pSocket->bSendCbs?"ON":"OFF"), (iData1?"ON":"OFF"), pSocket));
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
    NetPrintf(("dirtynetunix: unhandled control option '%C'\n", iOption));
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function    SocketGetLocalAddr

    \Description
        Returns the "external" local address (ie, the address as a machine "out on
        the Internet" would see as the local machine's address).

    \Output
        uint32_t        - local address

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
uint32_t SocketGetLocalAddr(void)
{
    SocketStateT *pState = _Socket_pState;
    if (pState->uLocalAddr == 0)
    {
        pState->uLocalAddr = SocketInfo(NULL, 'addr', 0, NULL, 0);
    }
    return(pState->uLocalAddr);
}

/*F********************************************************************************/
/*!
    \Function    SocketLookup

    \Description
        Lookup a host by name and return the corresponding Internet address. Uses
        a callback/polling system since the socket library does not allow blocking.

    \Input *pText   - pointer to null terminated address string
    \Input iTimeout - number of milliseconds to wait for completion

    \Output
        HostentT *  - hostent struct that includes callback vectors

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
HostentT *SocketLookup(const char *pText, int32_t iTimeout)
{
    SocketStateT *pState = _Socket_pState;
    SocketLookupPrivT *pPriv;
    int32_t iAddr, iResult;
    HostentT *pHost, *pHostRef;
    pthread_attr_t attr;

    NetPrintf(("dirtynetunix: looking up address for host '%s'\n", pText));

    // dont allow negative timeouts
    if (iTimeout < 0)
    {
        return(NULL);
    }

    // create new structure
    pPriv = DirtyMemAlloc(sizeof(*pPriv), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(pPriv, 0, sizeof(*pPriv));
    pHost = &pPriv->Host;

    // setup callbacks
    pHost->Done = &_SocketLookupDone;
    pHost->Free = &_SocketLookupFree;
    // copy over the target address
    ds_strnzcpy(pHost->name, pText, sizeof(pHost->name));

    // look for refcounted lookup
    if ((pHostRef = SocketHostnameAddRef(&pState->pHostList, &pPriv->Host, TRUE)) != NULL)
    {
        DirtyMemFree(pPriv, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        return(pHostRef);
    }

    // check for dot notation, then check hostname cache
    if (((iAddr = SocketInTextGetAddr(pText)) != 0) || ((iAddr = SocketHostnameCacheGet(pState->pHostnameCache, pText, 1 /*pState->iVerbose*/)) != 0))
    {
        // we've got a dot-notation address
        pHost->addr = iAddr;
        pHost->done = 1;
        // return completed record
        return(pHost);
    }

    /* add an extra refcount for the thread; this ensures the host structure survives until the thread
       is done with it.  this must be done before thread creation. */
    pHost->refcount += 1;

    // create dns lookup thread
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if ((iResult = pthread_create(&pPriv->iThreadId, &attr, _SocketLookupThread, pPriv)) != 0)
    {
        NetPrintf(("dirtynetunix: pthread_create() failed (err=%s)\n", DirtyErrGetName(iResult)));
        pPriv->Host.done = -1;
        // remove refcount we just added
        pHost->refcount -= 1;
    }

    // return the host reference
    return(pHost);
}

/*F********************************************************************************/
/*!
    \Function    SocketHost

    \Description
        Return the host address that would be used in order to communicate with
        the given destination address.

    \Input *pHost   - [out] local sockaddr struct
    \Input iHostlen - length of structure (sizeof(host))
    \Input *pDest   - remote sockaddr struct
    \Input iDestlen - length of structure (sizeof(dest))

    \Output
        int32_t     - zero=success, negative=error

    \Version 12/12/2003 (sbevan)
*/
/********************************************************************************F*/
int32_t SocketHost(struct sockaddr *pHost, int32_t iHostlen, const struct sockaddr *pDest, int32_t iDestlen)
{
#if defined(DIRTYCODE_APPLEIOS)
    SocketStateT *pState = _Socket_pState;

    // must be same kind of addresses
    if (iHostlen != iDestlen)
    {
        return(-1);
    }

    // do family specific lookup
    if (pDest->sa_family == AF_INET)
    {
        // special case destination of zero or loopback to return self
        if ((SockaddrInGetAddr(pDest) == 0) || (SockaddrInGetAddr(pDest) == 0x7f000000))
        {
            ds_memcpy(pHost, pDest, iHostlen);
            return(0);
        }
        else
        {
            memset(pHost, 0, iHostlen);
            pHost->sa_family = AF_INET;
            SockaddrInSetAddr(pHost, pState->uLocalAddr);
            return(0);
        }
    }

    // unsupported family
    memset(pHost, 0, iHostlen);
    return(-3);
#elif defined(DIRTYCODE_LINUX) || defined(DIRTYCODE_ANDROID)
    struct sockaddr_in HostAddr;
    struct sockaddr_in DestAddr;
    uint32_t uSource = 0, uTarget;
    int32_t iSocket;

    // get target address
    uTarget = SockaddrInGetAddr(pDest);

    // create a temp socket (must be datagram)
    iSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (iSocket != INVALID_SOCKET)
    {
        int32_t iIndex;
        int32_t iCount;
        struct ifreq EndpRec[16];
        struct ifconf EndpList;
        uint32_t uAddr;
        uint32_t uMask;

        // request list of interfaces
        memset(&EndpList, 0, sizeof(EndpList));
        EndpList.ifc_req = EndpRec;
        EndpList.ifc_len = sizeof(EndpRec);
        if (ioctl(iSocket, SIOCGIFCONF, &EndpList) >= 0)
        {
            // figure out number and walk the list
            iCount = EndpList.ifc_len / sizeof(EndpRec[0]);
            for (iIndex = 0; iIndex < iCount; ++iIndex)
            {
                // extract the individual fields
                ds_memcpy(&HostAddr, &EndpRec[iIndex].ifr_addr, sizeof(HostAddr));
                uAddr = ntohl(HostAddr.sin_addr.s_addr);
                ioctl(iSocket, SIOCGIFNETMASK, &EndpRec[iIndex]);
                ds_memcpy(&DestAddr, &EndpRec[iIndex].ifr_broadaddr, sizeof(DestAddr));
                uMask = ntohl(DestAddr.sin_addr.s_addr);
                ioctl(iSocket, SIOCGIFFLAGS, &EndpRec[iIndex]);

                NetPrintf(("dirtynetunix: checking interface name=%s, fam=%d, flags=%04x, addr=%08x, mask=%08x\n",
                    EndpRec[iIndex].ifr_name, HostAddr.sin_family,
                    EndpRec[iIndex].ifr_flags, uAddr, uMask));

                // only consider live inet interfaces
                if ((HostAddr.sin_family == AF_INET) && ((EndpRec[iIndex].ifr_flags & (IFF_LOOPBACK+IFF_UP)) == (IFF_UP)))
                {
                    // if target is within address range, must be hit
                    if ((uAddr & uMask) == (uTarget & uMask))
                    {
                        uSource = uAddr;
                        break;
                    }
                    // if in a private address space and nothing else found
                    if (((uAddr & 0xff000000) == 0x0a000000) || ((uAddr & 0xffff0000) == 0xc0a80000))
                    {
                        if (uSource == 0)
                        {
                            uSource = uAddr;
                        }
                    }
                    // always take a public address
                    else
                    {
                        uSource = uAddr;
                    }
                }
            }
        }
        // close the socket
        close(iSocket);
    }

    // populate dest addr
    SockaddrInit(pHost, AF_INET);
    SockaddrInSetAddr(pHost, uSource);

    // return result
    return((uSource != 0) ? 0 : -1);
#else
    return(-1);
#endif
}

