/*H*************************************************************************************/
/*!
    \File dirtynetwin.c

    \Description
        Provide a wrapper that translates the Winsock network interface
        into DirtySock calls. In the case of Winsock, little translation
        is needed since it is based off BSD sockets (as is DirtySock).

    \Copyright
        Copyright (c) Electronic Arts 1999-2013.

    \Version 1.0 01/02/2002 (gschaefer) First Version
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
#include <mstcpip.h>   // for tcp keep alive definitions

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtyvers.h"

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
    uint8_t bAsyncRecv;         //!< TRUE if async recv is enabled
    int8_t iVerbose;            //!< debug level

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
    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list
    int32_t iMaxPacket;                 //!< maximum packet size

    // module memory group
    int32_t iMemGroup;                  //!< module mem group id
    void *pMemGroupUserData;            //!< user data associated with mem group

    int32_t iVersion;                   //!< winsock version
    uint32_t uAdapterAddress;           //!< local interface used for SocketBind() operations if non-zero
    int32_t iVerbose;                   //!< debug level
    uint32_t uPacketLoss;               //!< packet loss simulation (debug only)

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
    sock->iVerbose = 1;

    // set to non blocking
    ioctlsocket(s, FIONBIO, (u_long *)&uTrue);
    // if udp, allow broadcast
    if (type == SOCK_DGRAM)
    {
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char *)&uTrue, sizeof(uTrue));
    }
    // if raw, set hdrincl
    if (type == SOCK_RAW)
    {
        setsockopt(s, IPPROTO_IP, IP_HDRINCL, (char *)&uTrue, sizeof(uTrue));
    }

    // set family/proto info
    sock->family = af;
    sock->type = type;
    sock->proto = proto;
    sock->bAsyncRecv = ((type == SOCK_DGRAM) || (type == SOCK_RAW)) ? TRUE : FALSE;

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
    uint8_t bSockInList = FALSE;
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

    /* Put into killed list:
       Usage of a kill list allows for postponing two things
        * destruction of recvcrit
        * release of socket data structure memory
      This ensures that recvcrit is not freed while in-use by a running thread.
      Such a scenario can occur when the receive callback invoked by _SocketRecvThread()
      (while recvcrit is entered) calls _SocketClose() */
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
        #if SOCKET_ASYNCRECVTHREAD
        NetCritKill(&sock->recvcrit);
        #endif

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
    NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: receive thread started (thid=%d)\n", GetCurrentThreadId()));

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
            // only handle non-virtual sockets with asyncrecv true
            if ((pSocket->bVirtual == FALSE) && (pSocket->socket != INVALID_SOCKET) && (pSocket->bAsyncRecv == TRUE))
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
    NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: receive thread exit\n"));
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

    \Input iThreadPrio        - priority to start threads with
    \Input iThreadStackSize   - stack size to start threads with (in bytes)
    \Input iThreadCpuAffinity - cpu affinity to start threads with

    \Output
        int32_t               - negative=error, zero=success

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketCreate(int32_t iThreadPrio, int32_t iThreadStackSize, int32_t iThreadCpuAffinity)
{
    SocketStateT *pState = _Socket_pState;
    WSADATA data;
    int32_t iResult;
    #if SOCKET_ASYNCRECVTHREAD
    int32_t pid;
    #endif
    int32_t iMemGroup;
    void *pMemGroupUserData;
#if SOCKET_ASYNCRECVTHREAD
    uintptr_t tempHandle = 1;
#endif

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetwin: DirtySDK v%d.%d.%d.%d.%d\n", DIRTYSDK_VERSION_YEAR, DIRTYSDK_VERSION_SEASON, DIRTYSDK_VERSION_MAJOR, DIRTYSDK_VERSION_MINOR, DIRTYSDK_VERSION_PATCH));

    // alloc and init state ref
    if ((pState = DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetwin: unable to allocate module state\n"));
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
    memset(&data, 0, sizeof(data));
    iResult = WSAStartup(MAKEWORD(2,2), &data);
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
    if ((pState->hRecvThread = CreateThread(NULL, iThreadStackSize, (LPTHREAD_START_ROUTINE)&_SocketRecvThread, NULL, 0, (LPDWORD)&pid)) != NULL)
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
        NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    NetPrintf(("dirtynetwin: shutting down\n"));

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
                NetPrintfVerbose((sock->iVerbose, 0, "dirtynetwin: making socket bound to port %d virtual\n", uPort));
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
    if ((iResult == SOCKERR_NONE) && (sock->bAsyncRecv == TRUE))
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

    \Input *pSocket - socket reference
    \Input *pName   - pointer to name of socket to connect to
    \Input iNameLen - length of name

    \Output
        int32_t     - standard network error code (SOCKERR_xxx)

    \Notes
        Only has real meaning for stream protocols. For a datagram protocol, this
        just sets the default remote host.

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketConnect(SocketT *pSocket, struct sockaddr *pName, int32_t iNameLen)
{
    struct sockaddr SockAddr;
    int32_t iResult;

    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connecting to %a:%d\n", SockaddrInGetAddr(pName), SockaddrInGetPort(pName)));

    // mark as not open
    pSocket->opened = 0;

    /* execute an explicit bind - this allows us to specify a non-zero local address
       or port (see SocketBind()).  a SOCKERR_INVALID result here means the socket has
       already been bound, so we ignore that particular error */
    SockaddrInit(&SockAddr, AF_INET);
    if (((iResult = SocketBind(pSocket, &SockAddr, sizeof(SockAddr))) < 0) && (iResult != SOCKERR_INVALID))
    {
        pSocket->iLastError = iResult;
        return(pSocket->iLastError);
    }

    // execute the connect
    iResult = _XlatError(connect(pSocket->socket, pName, iNameLen));

    // save connect address
    memcpy(&pSocket->remote, pName, sizeof(pSocket->remote));

    #if SOCKET_ASYNCRECVTHREAD
    // notify read thread that socket is ready to be read from
    if ((iResult == SOCKERR_NONE) && (pSocket->bAsyncRecv == TRUE))
    {
        WSASetEvent(_Socket_pState->iEvent);
    }
    #endif

    // return result to caller
    pSocket->iLastError = iResult;
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketListen

    \Description
        Start listening for an incoming connection on the socket.  The socket must already
        be bound and a stream oriented connection must be in use.

    \Input *pSocket - socket reference to bound socket (see SocketBind())
    \Input backlog  - number of pending connections allowed

    \Output
        int32_t         - standard network error code (SOCKERR_xxx)

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
int32_t SocketListen(SocketT *pSocket, int32_t backlog)
{
    // do the listen
    pSocket->iLastError = _XlatError(listen(pSocket->socket, backlog));
    return(pSocket->iLastError);
}

/*F*************************************************************************************/
/*!
    \Function    SocketAccept

    \Description
        Accept an incoming connection attempt on a socket.

    \Input *pSocket - socket reference to socket in listening state (see SocketListen())
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
SocketT *SocketAccept(SocketT *pSocket, struct sockaddr *addr, int32_t *addrlen)
{
    SocketT *pOpen = NULL;
    SOCKET iIncoming;

    pSocket->iLastError = SOCKERR_INVALID;

    // see if already connected
    if (pSocket->socket == INVALID_SOCKET)
    {
        return(NULL);
    }

    // make sure turn parm is valid
    if ((addr != NULL) && (*addrlen < sizeof(struct sockaddr)))
    {
        return(NULL);
    }

    // perform inet accept
    if (pSocket->family == AF_INET)
    {
        iIncoming = accept(pSocket->socket, addr, addrlen);
        if (iIncoming != INVALID_SOCKET)
        {
            pOpen = _SocketOpen(iIncoming, pSocket->family, pSocket->type, pSocket->proto);
            pSocket->iLastError = SOCKERR_NONE;
        }
        else
        {
            // use a negative error code to force _XlatError to internally call WSAGetLastError() and translate the obtained result
            pSocket->iLastError = _XlatError(-99);

            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                NetPrintf(("dirtynetwin: accept() failed err=%d\n", WSAGetLastError()));
            }
        }
    }

    // return the socket
    return(pOpen);
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
    int32_t iResult;

    // if installed, give socket callback right of first refusal
    if (pState->pSendCallback != NULL)
    {
        if ((iResult = pState->pSendCallback(pSocket, pSocket->type, (const uint8_t *)buf, len, to, pState->pSendCallref)) > 0)
        {
            return(iResult);
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
        iResult = send(pSocket->socket, buf, len, 0);
        to = &pSocket->remote;
    }
    else
    {
        iResult = sendto(pSocket->socket, buf, len, 0, to, tolen);
    }

    // return bytes sent
    pSocket->iLastError = _XlatError(iResult);
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
    #if SOCKET_ASYNCRECVTHREAD
    SocketStateT *pState = _Socket_pState;
    #endif
    int32_t iRecv = 0, iRecvErr;

    #if SOCKET_ASYNCRECVTHREAD
    // sockets marked for async recv had actual receive operation take place in the thread
    if (pSock->bAsyncRecv)
    {
        // acquire socket receive critical section
        NetCritEnter(&pSock->recvcrit);

        // get most recent socket error (moved up since _SocketRead might modify recverr)
        iRecvErr = pSock->recverr;
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
                // got data, no error
                iRecvErr = pSock->recverr = 0;
            }

            // mark data as consumed
            pSock->recvstat = 0;

            /*
            If a caller were to poll SocketRecvfrom() directly on a UDP socket, they would
            get a single queued packet in the first call and the second would return nothing,
            even if data is waiting, due to the fact that we queue only a single packet in
            the recv thread. This introduces a "bubble" in receiving data that drastically
            limits throughput. Normal usage ("normal" meaning "commudp") does not see this
            problem because the async recv thread forwards data immediately to the installed
            CommUDP event handler which queues the packet there. To remove the performance
            issue when using SocketRecvfrom() directly (no callback registered), as soon
            as recv data is consumed by the client, we try to fill the receive buffer with
            new data from the socket to maximize chances of data being available to the
            customer next time he calls SocketRecvfrom().
            */
            if ((pSock->recvinp == 0) && (pSock->callback == NULL))
            {
                _SocketRead(pSock);

                // IO pending, wake up receive thread to wait on this socket's event
                if ((pSock->recvinp != 0) && (pSock->recverr == WSA_IO_PENDING))
                {
                    WSASetEvent(pState->iEvent);
                }
            }
        }

        // wake up receive thread?
        if (pSock->recverr != WSA_IO_PENDING)
        {
            pSock->recverr = 0;
            WSASetEvent(pState->iEvent);
        }

        // release socket receive critical section
        NetCritLeave(&pSock->recvcrit);

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
        struct timeval tv;

        // if not a connected socket, return TRUE
        if (pSocket->type != SOCK_STREAM)
        {
            return(1);
        }

        // if not connected, use select to determine connect
        if (pSocket->opened == 0)
        {
            fd_set fdwrite;
            fd_set fdexcept;
            // setup write/exception lists so we can select against the socket
            FD_ZERO(&fdwrite);
            FD_ZERO(&fdexcept);
            FD_SET(pSocket->socket, &fdwrite);
            FD_SET(pSocket->socket, &fdexcept);
            tv.tv_sec = tv.tv_usec = 0;
            if (select(1, NULL, &fdwrite, &fdexcept, &tv) > 0)
            {
                // if we got an exception, that means connect failed
                if (fdexcept.fd_count > 0)
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connection failed\n"));
                    pSocket->opened = -1;
                }
                // if socket is writable, that means connect succeeded
                else if (fdwrite.fd_count > 0)
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connection open\n"));
                    pSocket->opened = 1;
                }
            }
        }

        // if previously connected, make sure connect still valid
        if (pSocket->opened > 0)
        {
            fd_set fdread;
            fd_set fdexcept;
            FD_ZERO(&fdread);
            FD_ZERO(&fdexcept);
            FD_SET(pSocket->socket, &fdread);
            FD_SET(pSocket->socket, &fdexcept);
            tv.tv_sec = tv.tv_usec = 0;
            if (select(1, &fdread, NULL, &fdexcept, &tv) > 0)
            {
                // if we got an exception, that means connect failed (usually closed by remote peer)
                if (fdexcept.fd_count > 0)
                {
                    NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connection failure\n"));
                    pSocket->opened = -1;
                }
                else if (fdread.fd_count > 0)
                {
                    u_long uAvailBytes = 1; // u_long is required by ioctlsocket
                    // if socket is readable but there's no data available, connect was closed
                    // uAvailBytes might be less than actual bytes, so it can only be used for zero-test
                    if ((ioctlsocket(pSocket->socket, FIONREAD, &uAvailBytes) != 0) || (uAvailBytes == 0))
                    {
                        pSocket->iLastError = SOCKERR_CLOSED;
                        NetPrintfVerbose((pSocket->iVerbose, 0, "dirtynetwin: connection closed (wsaerr=%d)\n", (uAvailBytes==0) ? WSAECONNRESET : WSAGetLastError()));
                        pSocket->opened = -1;
                    }
                }
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
            'arcv' - set async receive enable/disable (default enabled for DGRAM/RAW, disabled for TCP)
            'conn' - handle connect message
            'disc' - handle disconnect message
            'keep' - enable/disablet TCP keep-alive (STREAM sockets only)
            'ladr' - set local address for subsequent SocketBind() operations
            'loss' - simulate packet loss (debug only)
            'maxp' - set max udp packet size
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'ndly' - set TCP_NODELAY state for given stream socket (iData1=zero or one)
            'poll' - execute blocking wait on given socket list (pData2) or all sockets (pData2=NULL), 64 max sockets
            'push' - push data into given socket (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'rbuf' - set socket recv buffer size
            'sbuf' - set socket send buffer size
            'sdcb' - set send callback (pData2=callback, pData3=callref)
            'spam' - set debug level (default 1)
            'vadd' - add a port to virtual port list
            'vdel' - del a port from virtual port list
        \endverbatim

    \Version 10/04/1999 (gschaefer)
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
        WSASetEvent(pState->iEvent);
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
    // configure TCP keep-alive
    if (iOption == 'keep')
    {
        if (pSocket->type == SOCK_STREAM)
        {
            if (pSocket->socket != INVALID_SOCKET)
            {
                struct tcp_keepalive tcpKeepAlive;
                int32_t iResult;
                DWORD dwBytesReturned;

                // initialize tcpkeep alive structure
                tcpKeepAlive.onoff = iData1;              // on/off
                if (tcpKeepAlive.onoff)
                {
                    tcpKeepAlive.keepalivetime = *(uint32_t*)pData2;     // timeout in ms
                    tcpKeepAlive.keepaliveinterval = *(uint32_t*)pData3; // interval in ms
                }
                else
                {
                    tcpKeepAlive.keepalivetime = 0;     // timeout in ms
                    tcpKeepAlive.keepaliveinterval = 0; // interval in ms
                }

                if ((iResult = WSAIoctl(pSocket->socket, SIO_KEEPALIVE_VALS, (void *)&tcpKeepAlive, sizeof(tcpKeepAlive), NULL, 0, &dwBytesReturned, NULL, NULL)) == 0)
                {
                    pSocket->iLastError = SOCKERR_NONE;
                    NetPrintf(("dirtynetwin: successfully %s the TCP keep-alive for low-level socket %d [timeout=%dms|interval=%dms]\n",
                        iData1?"enabled":"disabled", pSocket->socket, tcpKeepAlive.keepalivetime, tcpKeepAlive.keepaliveinterval));
                }
                else
                {
                    pSocket->iLastError = _XlatError(iResult);
                    NetPrintf(("dirtynetwin: failed to %s TCP keep-alive for low-level socket %d (err = %d)\n",
                        iData1?"enable":"disable", pSocket->socket, WSAGetLastError()));
                }
            }
            else
            {
                NetPrintf(("dirtynetwin: 'keep' selector used with an invalid socket\n"));
                return(-1);
            }
        }
        else
        {
            NetPrintf(("dirtynetwin: 'keep' selector can only be used on a SOCK_STREAM socket\n"));
            return(-1);
        }

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

            // don't allow data that is too large (for the buffer) to be pushed
            if (iData1 > (signed)sizeof(pSocket->recvdata))
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
        }
        else
        {
            NetPrintf(("dirtynetwin: warning - call to SocketControl('push') ignored because pSocket is NULL\n"));
            return(-1);
        }
        return(0);
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
    // set debug level
    if (iOption == 'spam')
    {
        // per-socket debug level
        if (pSocket != NULL)
        {
            pSocket->iVerbose = iData1;
        }
        else // module level debug level
        {
            pState->iVerbose = iData1;
        }
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

/*F*************************************************************************************/
/*!
    \Function    _SocketLookupDone

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
    // return current status
    return(host->done);
}

/*F*************************************************************************************/
/*!
    \Function    _SocketLookupFree

    \Description
        Release resources used by gethostbyname.

    \Input *host    - pointer to host lookup record

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static void _SocketLookupFree(HostentT *host)
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
    \Function    _SocketLookupThread

    \Description
        Socket lookup thread

    \Input *host    - pointer to host lookup record

    \Output
        int32_t     - zero

    \Version 10/04/1999 (gschaefer)
*/
/************************************************************************************F*/
static int32_t _SocketLookupThread(HostentT *host)
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
        NetPrintfVerbose((pState->iVerbose, 0, "dirtynetwin: %s=%a\n", host->name, host->addr));
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
    int32_t iAddr;
    HostentT *pHost;
    DWORD pid;
    HANDLE thread;

    // dont allow negative timeouts
    if (timeout < 0)
    {
        return(NULL);
    }

    // create new structure
    pHost = DirtyMemAlloc(sizeof(*pHost), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    memset(pHost, 0, sizeof(*pHost));

    // setup callbacks
    pHost->Done = &_SocketLookupDone;
    pHost->Free = &_SocketLookupFree;

    // check for dot notation
    if ((iAddr = SocketInTextGetAddr(text)) != 0)
    {
        // save the data
        pHost->addr = iAddr;
        pHost->done = 1;
        // mark as "thread complete" so _SocketLookupFree will release memory
        pHost->thread = 1;
        // return completed record
        return(pHost);
    }

    // copy over the target address
    ds_strnzcpy(pHost->name, text, sizeof(pHost->name));

    // create dns lookup thread
    thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&_SocketLookupThread, (LPVOID)pHost, 0, &pid);
    if (thread != NULL)
    {
        CloseHandle(thread);
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
                    NetPrintf(("dirtynetwin: WSAIoctl(SIO_ROUTING_INTERFACE_QUERY) returned %d\n", err));
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
