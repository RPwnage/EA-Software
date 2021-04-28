/*H********************************************************************************/
/*!
    \File dirtynetps3.c

    \Description
        Provides a wrapper that translates the Sony network interface to the
        DirtySock portable networking interface.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 10/05/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>

// basic type defintions
#include <system_types.h>

// thread/timer
#include <ppu_thread.h>
#include <timer.h>

// network includes
#include <sdk_version.h> // included so we know where to find network headers

#include <sys/poll.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <netex/ifctl.h>
#include <netex/net.h>
#include <netex/libnetctl.h>
#include <netex/errno.h>
#include <netex/sockinfo.h>
#include <sysutil/sysutil_common.h>

#include "dirtysock.h"
#include "dirtyvers.h"
#include "dirtymem.h"
#include "dirtyerr.h"

/*** Defines **********************************************************************/

#define INVALID_SOCKET      (-1)

#define SOCKET_MAXPOLL      (32)

#define SOCKET_VERBOSE      (DIRTYCODE_DEBUG && FALSE)

/*** Type Definitions *************************************************************/

//! private socketlookup structure containing extra data
typedef struct SocketLookupPrivT
{
    HostentT            Host;      //!< must come first!
    sys_ppu_thread_t    iThreadId; //!< 64-bit thread id (so we can't use Host.thread)
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

    uint8_t virtual;            //!< if true, socket is virtual
    uint8_t unused1;

    int32_t socket;             //!< ps3 socket ref
    int32_t iLastError;         //!< last socket error

    struct sockaddr local;      //!< local address
    struct sockaddr remote;     //!< remote address

    uint16_t virtualport;       //!< virtual port, if set
    uint16_t _pad;

    int32_t callmask;           //!< valid callback events
    uint32_t calllast;          //!< last callback tick
    uint32_t callidle;          //!< ticks between idle calls
    void *callref;              //!< reference calback value
    int32_t (*callback)(SocketT *sock, int32_t flags, void *ref);

    #if SOCKET_ASYNCRECVTHREAD
    NetCritT recvcrit;          //!< receive critical section
    int32_t recverr;            //!< last error that occurred
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
    SocketT  *pSockList;                //!< master socket list
    SocketT  *pSockKill;                //!< list of killed sockets

    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list

    // module memory group
    int32_t  iMemGroup;                 //!< module mem group id
    void     *pMemGroupUserData;        //!< user data associated with mem group

    void     *pMemPool;                 //!< sys_net memory pool pointer

    NetCritT callbackCrit;              //!< critical section used to ensure thread safety in handler registered with libnet utility

    int32_t  iSid;                      //!< sony state id
    int32_t  iHandlerId;                //!< id of connection handler
    int32_t  iMaxPacket;                //!< maximum packet size

    /*
    The following three variables are to be accessed safely within callbackCrit. They are
    all used in the DirtyNetPS3 code and in the callback registered with the libnet utility.
    So they need to be protected with a critical section to guarantee thread safety.
     */
    uint32_t uConnStatus;               //!< current connection status
    uint32_t uLocalAddr;                //!< local internet address for active interface
    uint8_t  aMacAddr[6];               //!< mac address for active interface
    uint8_t  bNetCtlAlreadyInitialized; //!< TRUE if cellNetCtl was already initialized at connect time, else FALSE
    uint8_t  _pad;

    uint32_t uPacketLoss;               //!< packet loss simulation (debug only)

    #if SOCKET_ASYNCRECVTHREAD
    volatile sys_ppu_thread_t iRecvThread;
    volatile int32_t iRecvLife;
    #endif

    SocketSendCallbackT *pSendCallback; //!< global send callback
    void                *pSendCallref;  //!< user callback data
} SocketStateT;

/*** Variables ********************************************************************/

//! module state ref
static SocketStateT *_Socket_pState = NULL;

//! mempool size for sys_net
static int32_t      _Socket_iMemPoolSize = 128 * 1024;

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function    _XlatError

    \Description
        Translate a BSD error to dirtysock

    \Input iErr     - BSD error code (e.g EAGAIN)

    \Output
        int32_t         - dirtysock error

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _XlatError(int32_t iErr)
{
    if (iErr < 0)
    {
        iErr = sys_net_errno;
        if ((iErr == SYS_NET_EWOULDBLOCK) || (iErr == SYS_NET_EINPROGRESS))
            iErr = SOCKERR_NONE;
        else if (iErr == SYS_NET_EHOSTUNREACH)
            iErr = SOCKERR_UNREACH;
        else if (iErr == SYS_NET_ENOTCONN)
            iErr = SOCKERR_NOTCONN;
        else if (iErr == SYS_NET_ECONNREFUSED)
            iErr = SOCKERR_REFUSED;
        else if (iErr == SYS_NET_ECONNRESET)
            iErr = SOCKERR_CONNRESET;
        else
            iErr = SOCKERR_OTHER;
    }
    return(iErr);
}

/*F********************************************************************************/
/*!
    \Function    _SockAddrToSceAddr

    \Description
        Translate DirtySock socket address to Sony socket address.

    \Input *pSceAddr    - [out] pointer to Sce address to set up
    \Input *pSockAddr   - pointer to source DirtySock address

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SockAddrToSceAddr(struct sockaddr *pSceAddr, const struct sockaddr *pSockAddr)
{
    memcpy(pSceAddr, pSockAddr, sizeof(*pSceAddr));
    pSceAddr->sa_len = sizeof(*pSceAddr);
    pSceAddr->sa_family = (unsigned char)pSockAddr->sa_family;
}

/*F********************************************************************************/
/*!
    \Function _SocketNetCtlHandler

    \Description
        Finds the active interface, and gets its IP address and MAC address for
        later use.

    \Input iPrevState   - previous state
    \Input iNewState    - new state
    \Input iEvent       - event
    \Input iErrorCode   - error code
    \Input *pArg        - arguments

    \Notes
        This callback is now thread-safe. It uses a critical section to access
        variables that are shared with other functions of dirtynetps3. Thread-safety
        is required because we cannot guarantee that cellSysutilCheckCallback()
        is going to be called from the same thread as DirtySDK... and that function
        is the one that internally invokes the callbacks we register with the PS3 
        system.

    \Version 06/22/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketNetCtlHandler(int32_t iPrevState, int32_t iNewState, int32_t iEvent, int32_t iErrorCode, void *pArg)
{
    SocketStateT *pState = (SocketStateT *)pArg;
    int32_t iResult;

    #if DIRTYCODE_LOGGING
    static const char *_StateNames[] =
    {
        "Disconnected",
        "Connecting",
        "IPObtaining",
        "IPObtained"
    };
    static const char *_EventNames[] =
    {
        "CONNECT_REQ",
        "ESTABLISH",
        "GET_IP",
        "DISCONNECT_REQ",
        "ERROR",
        "LINK_DISCONNECTED",
        "AUTO_RETRY"
    };

    NetPrintf(("dirtynetps3: prev=%s new=%s event=%s err=%d\n", _StateNames[iPrevState],
        _StateNames[iNewState], _EventNames[iEvent], iErrorCode));
    #endif

    // the following variables can only be accessed in the context of the callbackCrit
    // critical section: uLocalAddr, aMacAddr, uConnStatus
    NetCritEnter(&pState->callbackCrit);

    if (iEvent == CELL_NET_CTL_EVENT_ERROR)
    {
        NetPrintf(("dirtynetps3: got error event: err=%s\n", DirtyErrGetName(iErrorCode)));
        pState->uConnStatus = '-err';
    }
    else if (iEvent == CELL_NET_CTL_EVENT_GET_IP)
    {
        union CellNetCtlInfo CtlInfo;

        // get local address
        if ((iResult = cellNetCtlGetInfo(CELL_NET_CTL_INFO_IP_ADDRESS, &CtlInfo)) >= 0)
        {
            pState->uLocalAddr = SocketInTextGetAddr(CtlInfo.ip_address);
            NetPrintf(("dirtynetps3: obtained IP address %a\n", pState->uLocalAddr));
        }
        else
        {
            NetPrintf(("dirtynetps3: cellNetCtlGetInfo(_IP_ADDRESS) failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        // get ethernet address
        if ((iResult = cellNetCtlGetInfo(CELL_NET_CTL_INFO_ETHER_ADDR, &CtlInfo)) >= 0)
        {
            memcpy(pState->aMacAddr, CtlInfo.ether_addr.data, sizeof(pState->aMacAddr));
            NetPrintf(("dirtynetps3: obtained MAC address $%02x%02x%02x%02x%02x%02x\n",
                pState->aMacAddr[0], pState->aMacAddr[1], pState->aMacAddr[2],
                pState->aMacAddr[3], pState->aMacAddr[4], pState->aMacAddr[5]));
        }
        else
        {
            NetPrintf(("dirtynetps3: cellNetCtlGetInfo(_IP_ADDRESS) failed (err=%s)\n", DirtyErrGetName(iResult)));
        }

        // mark us as online
        pState->uConnStatus = '+onl';
    }
    else if (((iEvent == CELL_NET_CTL_EVENT_DISCONNECT_REQ) && (iNewState == CELL_NET_CTL_STATE_Disconnected)) ||
             ((iEvent == CELL_NET_CTL_EVENT_LINK_DISCONNECTED) && (iPrevState == CELL_NET_CTL_STATE_IPObtained)))
     {
        NetPrintf(("dirtynetps3: got disconnect event at %d\n", NetTick()));
        pState->uConnStatus = '-dsc';
        pState->uLocalAddr = 0;
    }

    NetCritLeave(&pState->callbackCrit);
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
        NetPrintf(("dirtynetps3: unable to allocate memory for socket\n"));
        return(NULL);
    }
    memset(pSocket, 0, sizeof(*pSocket));

    if (iSocket == -1)
    {
        uint32_t uTrue = 1;

        // create socket
        if ((iSocket = socket(AF_INET, iType, iProto)) >= 0)
        {
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
            setsockopt(iSocket, SOL_SOCKET, SO_NBIO, &uTrue, sizeof(uTrue));
        }
        else
        {
            NetPrintf(("dirtynetps3: socket() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
        }
    }

    // set family/proto info
    pSocket->family = iAddrFamily;
    pSocket->type = iType;
    pSocket->proto = iProto;
    pSocket->socket = iSocket;
    pSocket->opened = iOpened;
    pSocket->iLastError = SOCKERR_NONE;

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
    \Function _SocketClose

    \Description
        Disposes of a SocketT, including disposal of the SocketT allocated memory.  Does
        NOT dispose of the Sony socket ref.

    \Input *pSocket - socket to close

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
        NetPrintf(("dirtynetps3: warning, trying to close socket 0x%08x that is not in the socket list\n", (intptr_t)pSocket));
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
            (pSocket->calllast != (unsigned)-1) &&
            ((uTick - pSocket->calllast) > pSocket->callidle))
        {
            pSocket->calllast = (unsigned)-1;
            (pSocket->callback)(pSocket, 0, pSocket->callref);
            pSocket->calllast = uTick = NetTick();
        }
    }

    // delete any killed sockets
    while ((pSocket = pState->pSockKill) != NULL)
    {
        pState->pSockKill = pSocket->kill;
        DirtyMemFree(pSocket, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }

    // for access to g_socklist and g_sockkill
    NetCritLeave(NULL);
}

/*F********************************************************************************/
/*!
    \Function    SocketLookupDone

    \Description
        Callback to determine if gethostbyname is complete.

    \Input *pHost    - pointer to host lookup record

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
    \Function    SocketLookupFree

    \Description
        Release resources used by SocketLookup()

    \Input *pHost    - pointer to host lookup record

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketLookupFree(HostentT *pHost)
{
    SocketStateT *pState = _Socket_pState;
    SocketLookupPrivT *pPriv = (SocketLookupPrivT *)pHost;

    if (pPriv->iThreadId != 0)
    {
        // if the resolve is still in progress, terminate it
        if (sys_net_abort_resolver(pPriv->iThreadId, SYS_NET_ABORT_STRICT_CHECK) == 0)
        {
            NetPrintf(("dirtynetps3: aborted dns lookup name=%s\n", pHost->name));
        }

        // signal the thread to die
        pHost->sema = 0;
    }
    else
    {
        // no thread -- take care of free
        DirtyMemFree(pHost, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function    _SocketLookupThread

    \Description
        Socket lookup thread

    \Input _pRef    - module state

    \Output
        int32_t         - zero

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketLookupThread(uint64_t _pRef)
{
    SocketStateT *pState = _Socket_pState;
    HostentT *pHost = (HostentT *)(uintptr_t)_pRef;
    struct hostent *pSceHost;
    unsigned char *pAddr;
    int32_t iMemGroup = pState->iMemGroup;
    void *pMemGroupUserData = pState->pMemGroupUserData;

    #if DIRTYCODE_LOGGING
    sys_ppu_thread_t uThreadId;
    sys_ppu_thread_get_id(&uThreadId);
    #endif

    #if DIRTYSOCK_ERRORNAMES
    static const DirtyErrT _LookupErrorNames[] =
    {
        DIRTYSOCK_ErrorName(NETDB_INTERNAL),    // -1
        DIRTYSOCK_ErrorName(NETDB_SUCCESS),     // 0
        DIRTYSOCK_ErrorName(HOST_NOT_FOUND),    // 1
        DIRTYSOCK_ErrorName(TRY_AGAIN),         // 2
        DIRTYSOCK_ErrorName(NO_RECOVERY),       // 3
        DIRTYSOCK_ErrorName(NO_DATA),           // 4 (also NO_ADDRESS)
        // NULL terminate
        DIRTYSOCK_ListEnd()
    };
    #endif

    // start lookup
    NetPrintf(("dirtynetps3: lookup thread start; name=%s (thid=%d)\n", pHost->name, uThreadId));
    if ((pSceHost = gethostbyname(pHost->name)) != NULL)
    {
        if ((pSceHost->h_addr_list != NULL) && (pSceHost->h_addr_list[0] != NULL))
        {
            // extract ip address
            pAddr = (unsigned char *)pSceHost->h_addr_list[0];
            pHost->addr = (pAddr[0]<<24)|(pAddr[1]<<16)|(pAddr[2]<<8)|pAddr[3];

            // mark success
            NetPrintf(("dirtynetps3: %s=%a\n", pHost->name, pHost->addr));
            pHost->done = 1;
        }
        else
        {
            // empty hostent
            NetPrintf(("dirtynetps3: gethostbyname() failed; returned a hostent with an empty ip address list\n"));
            pHost->done = -1;
        }
    }
    else
    {
        // unsuccessful
        NetPrintf(("dirtynetps3: gethostbyname() failed err=%s\n", DirtyErrGetNameList(sys_net_h_errno, _LookupErrorNames)));
        pHost->done = -1;
    }

    // wait until _SocketLookupFree() is called
    while((volatile int32_t)pHost->sema > 0)
    {
        sys_timer_usleep(1000);
    }

    NetPrintf(("dirtynetps3: lookup thread exit; name=%s (thid=%d)\n", pHost->name, uThreadId));

    // dispose of resources
    DirtyMemFree(pHost, SOCKET_MEMID, iMemGroup, pMemGroupUserData);

    // terminate ourselves
    sys_net_free_thread_context(0, SYS_NET_THREAD_SELF);
    sys_ppu_thread_exit(0);
}

/*F********************************************************************************/
/*!
    \Function    _SocketRecvfrom

    \Description
        Receive data from a remote host on a datagram socket.

    \Input *pSocket - socket reference
    \Input *pBuf    - buffer to receive data
    \Input iLen     - length of recv buffer
    \Input *pFrom   - address data was received from (NULL=ignore)
    \Input *pFromLen- length of address

    \Output
        int32_t         - positive=data bytes received, else error

    \Version 09/10/04 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, struct sockaddr *pFrom, int32_t *pFromLen)
{
    int32_t iResult;

    if (pFrom != NULL)
    {
        struct sockaddr SockAddr;
        int32_t iFromLen = sizeof(SockAddr);

        // do the receive
        if ((iResult = recvfrom(pSocket->socket, pBuf, iLen, 0, &SockAddr, (socklen_t *)&iFromLen)) > 0)
        {
            // save who we were from
            memcpy(pFrom, &SockAddr, sizeof(*pFrom));
            pFrom->sa_family = SockAddr.sa_family;
            SockaddrInSetMisc(pFrom, NetTick());
             *pFromLen = sizeof(*pFrom);
        }
    }
    else
    {
        iResult = recv(pSocket->socket, pBuf, iLen, 0);
    }

    return(iResult);
}

#if SOCKET_ASYNCRECVTHREAD
/*F*************************************************************************************/
/*!
    \Function    _SocketRecvData

    \Description
        Called when data is received by _SocketRecvThread(). If there is a callback
        registered than the socket is passed to that callback so the data may be
        consumed.

    \Input *pSocket - pointer to socket that has new data

    \Version 10/21/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvData(SocketT *pSocket)
{
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
        Attempt to read data from the given socket.

    \Input *pState  - pointer to module state
    \Input *pSocket - pointer to socket to read from

    \Version 10/21/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRead(SocketStateT *pState, SocketT *pSocket)
{
    // if we already have data or this is a virtual socket
    if ((pSocket->recvstat != 0)  || (pSocket->virtual == TRUE))
    {
        return;
    }

    // try and receive some data
    if ((pSocket->type == SOCK_DGRAM) || (pSocket->type == SOCK_RAW))
    {
        int32_t iFromLen = sizeof(pSocket->recvaddr);
        pSocket->recvstat = _SocketRecvfrom(pSocket, pSocket->recvdata, sizeof(pSocket->recvdata), &pSocket->recvaddr, &iFromLen);
    }
    else
    {
        NetPrintf(("dirtynetps3: trying to read from unsupported socket type %d\n", pSocket->type));
        return;
    }

    // if the read completed successfully, forward data to socket callback
    if (pSocket->recvstat > 0)
    {
        _SocketRecvData(pSocket);
    }
}

/*F*************************************************************************************/
/*!
    \Function    _SocketRecvThread

    \Description
        Wait for incoming data and deliver it immediately to the socket callback,
        if registered.

    \Input  pArg    - pointer to Socket module state

    \Version 10/21/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvThread(uint64_t pArg)
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
    SocketStateT *pState = (SocketStateT *)(uintptr_t)pArg;

    // show we are alive
    NetPrintf(("dirtynetps3: receive thread started (thid=%d)\n", pState->iRecvThread));
    pState->iRecvLife = 1;

    // reset contents of pollList
    memset(&pollList, 0, sizeof(pollList));

    // loop until done
    while(pState->iRecvThread != 0)
    {
        // reset contents of previousPollList
        memset(&previousPollList, 0, sizeof(previousPollList));

        // make a copy of the poll list used for the last socketpoll() call
        for (iListIndex = 0; iListIndex < pollList.iCount; iListIndex++)
        {
            // copy entry from pollList to previousPollList
            previousPollList.aSockets[iListIndex] = pollList.aSockets[iListIndex];
            previousPollList.aPollFds[iListIndex] = pollList.aPollFds[iListIndex];
        }
        previousPollList.iCount = pollList.iCount;

        // reset contents of pollList in preparation for the next socketpoll() call
        memset(&pollList, 0, sizeof(pollList));

        // acquire global critical section for access to socket list
        NetCritEnter(NULL);

        // walk the socket list and do two things:
        //    1- if the socket is ready for reading, perform the read operation
        //    2- if the buffer in which inbound data is saved is empty, initiate a new low-level read operation for that socket
        for (pSocket = pState->pSockList; (pSocket != NULL) && (pollList.iCount < SOCKET_MAXPOLL); pSocket = pSocket->next)
        {
            // only handle non-virtual SOCK_DGRAM and non-virtual SOCK_RAW
            if ((pSocket->virtual == FALSE) && (pSocket->socket != INVALID_SOCKET) &&
                ((pSocket->type == SOCK_DGRAM) || (pSocket->type == SOCK_RAW)))
            {
                // acquire socket critical section
                NetCritEnter(&pSocket->recvcrit);

                // was this socket in the poll list of the previous socketpoll() call
                for (iListIndex = 0; iListIndex < previousPollList.iCount; iListIndex++)
                {
                    if (previousPollList.aSockets[iListIndex] == pSocket)
                    {
                        // socket was in previous poll list!
                        // now check if socketpoll() notified that this socket is ready for reading
                        if (previousPollList.aPollFds[iListIndex].revents & POLLIN)
                        {
                            /*
                            Note:
                            The socketpoll() doc states that some error codes returned by the function
                            may only apply to one of the sockets in the poll list. For this reason,
                            we check the polling result for all entries in the list regardless
                            of the return value of socketpoll().
                            */

                            // ready for reading, so go ahead and read
                            _SocketRead(pState, previousPollList.aSockets[iListIndex]);
                        }
                        break;
                    }
                }

                // if no data is queued, add this socket to the poll list to be used by the next socketpoll() call
                if ((pSocket->recvstat <= 0) && (pSocket->socket != INVALID_SOCKET))
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
            iResult = socketpoll(pollList.aPollFds, pollList.iCount, 50);

            if (iResult < 0)
            {
                NetPrintf(("dirtynetps3: socketpoll() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));

                // stall for 50ms because experiment shows that next call to socketpoll() may not block
                // internally if a socket is alreay in error.
                sys_timer_usleep(50*1000);
            }
        }
        else
        {
            // no sockets, so stall for 50ms
            sys_timer_usleep(50*1000);
        }
    }

    // indicate we are done
    NetPrintf(("dirtynetps3: receive thread exit\n"));
    pState->iRecvLife = 0;

    // terminate and exit
    sys_net_free_thread_context(0, SYS_NET_THREAD_SELF);
    sys_ppu_thread_exit(0);
}
#endif // SOCKET_ASYNCRECVTHREAD

/*F********************************************************************************/
/*!
    \Function    SocketCreate

    \Description
        Create new instance of socket interface module.  Initializes all global
        resources and makes module ready for use.

    \Input  iThreadPrio - thread priority to start socket thread with

    \Output
        int32_t         - negative=error, zero=success

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketCreate(int32_t iThreadPrio)
{
    SocketStateT *pState = _Socket_pState;
    sys_net_initialize_parameter_t InitParam;
    int32_t iResult;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintf(("dirtynetps3: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetps3: DirtySock SDK v%d.%d.%d.%d\n",
        (DIRTYVERS>>24)&0xff, (DIRTYVERS>>16)&0xff,
        (DIRTYVERS>> 8)&0xff, (DIRTYVERS>> 0)&0xff));

    // alloc and init state ref
    if ((pState = DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetps3: unable to allocate module state\n"));
        return(-2);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iMaxPacket = SOCKET_MAXUDPRECV;

    // startup network libs
    NetLibCreate(iThreadPrio);

    // add our idle handler
    NetIdleAdd(&_SocketIdle, pState);

    // create high-priority receive thread
    #if SOCKET_ASYNCRECVTHREAD
    sys_ppu_thread_create((sys_ppu_thread_t *)&pState->iRecvThread, _SocketRecvThread, (uintptr_t)pState, iThreadPrio, 16*1024, 0, "SocketRecv");
    if (pState->iRecvThread <= 0)
    {
        NetPrintf(("dirtynetps3: unable to create recv thread (err=%s)\n", DirtyErrGetName((uint32_t)pState->iRecvThread)));
    }
    #endif

    // allocate memory pool for sys_net init
    if ((pState->pMemPool = DirtyMemAlloc(_Socket_iMemPoolSize, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetps3: unable to allocate %d byte memory pool for sys_net\n", _Socket_iMemPoolSize));
        DirtyMemFree(pState, SOCKET_MEMID, iMemGroup, pMemGroupUserData);
        return(-3);
    }

    // init network stack
    InitParam.memory = (void *)pState->pMemPool;
    InitParam.memory_size = _Socket_iMemPoolSize;
    InitParam.flags = 0;
    if ((iResult = sys_net_initialize_network_ex(&InitParam)) != 0)
    {
        NetPrintf(("dirtynetps3: sys_net_initialize_network_ex() failed (err=%s)\n", DirtyErrGetName(iResult)));
        DirtyMemFree(pState, SOCKET_MEMID, iMemGroup, pMemGroupUserData);
        return(-4);
    }

    // init critical section used to ensure thread safety in handler registered with libnet utility
    NetCritInit(&pState->callbackCrit, "dirtynetps3 callback crit");

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
    int32_t iResult;

    // error if not active
    if (pState == NULL)
    {
        NetPrintf(("dirtynetps3: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    /* If NET_SHUTDOWN_THREADSTARVE is passed in, instead of shutting down we simply
       lower the priority of the recv thread to starve the thread and prevent it from
       executing.  This functionality is expected to be used in an XMB exit situation
       where the main code can be interrupted at any time, in any state, and forced
       to exit.  In this situation, simply preventing the threads from executing
       further allows the game to safely exit. */
    if (uShutdownFlags & NET_SHUTDOWN_THREADSTARVE)
    {
        #if SOCKET_ASYNCRECVTHREAD
        sys_ppu_thread_set_priority(pState->iRecvThread, 3071);
        #endif
        NetLibDestroy2(uShutdownFlags);
        return(0);
    }

    NetPrintf(("dirtynetps3: shutting down\n"));

    // kill idle callbacks
    NetIdleDel(&_SocketIdle, pState);

    // let any idle event finish
    NetIdleDone();

    #if SOCKET_ASYNCRECVTHREAD
    // tell receive thread to quit
    pState->iRecvThread = 0;

    // wait for thread to terminate
    while (pState->iRecvLife > 0)
    {
        sys_timer_usleep(1*1000);
    }
    #endif

    // close all sockets
    while (pState->pSockList != NULL)
    {
        SocketClose(pState->pSockList);
    }

    // clear the kill list
    _SocketIdle(pState);

    // destroy critical section used to ensure thread safety in handler registered with libnet utility
    NetCritKill(&pState->callbackCrit);

    // shut down network libs
    NetLibDestroy2(uShutdownFlags);

    // shut down sony networking (must come after thread exit)
    if ((iResult = sys_net_finalize_network()) < 0)
    {
        NetPrintf(("dirtynetps3: sys_net_finalize_network() failed err=%s\n", DirtyErrGetName(iResult)));
    }

    // dispose of sys_net memory pool
    DirtyMemFree(pState->pMemPool, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);

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
        SocketT *   - socket reference

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

    \Input *pSocket - socket reference

    \Output
        int32_t         - zero

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

    // close sce socket if allocated
    if (iSocket >= 0)
    {
        // close socket
        if (socketclose(iSocket) < 0)
        {
            NetPrintf(("dirtynetps3: socketclose() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
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
        SocketT pointer to the ref is returned, or it can be an actual Sony socket ref,
        in which case a SocketT is created for the Sony socket ref.

    \Input uSockRef - socket reference

    \Output
        SocketT *   - pointer to imported socket, or NULL

    \Version 01/14/2005 (jbrookes)
*/
/********************************************************************************F*/
SocketT *SocketImport(intptr_t uSockRef)
{
    SocketStateT *pState = _Socket_pState;
    uint32_t uProtoSize;
    SocketT *pSock;
    int32_t iProto;

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
    uProtoSize = sizeof(iProto);
    if (getsockopt(uSockRef, SOL_SOCKET, SO_TYPE, &iProto, &uProtoSize) == 0)
    {
        // create the socket
        pSock = _SocketOpen(uSockRef, AF_INET, iProto, 0, 0);

        // update local and remote addresses
        SocketInfo(pSock, 'bind', 0, &pSock->local, sizeof(pSock->local));
        SocketInfo(pSock, 'peer', 0, &pSock->remote, sizeof(pSock->remote));

        // mark it as imported
        pSock->bImported = TRUE;
    }
    else
    {
        NetPrintf(("dirtynetps3: getsockopt(SO_TYPE) failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
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
        int32_t         - zero

    \Version 09/10/04 (jbrookes)
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
        iResult = sys_net_errno;
        NetPrintf(("dirtynetps3: shutdown() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    pSocket->iLastError = _XlatError(iResult);
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
    struct sockaddr SockAddr;
    int32_t iResult;

    // make sure socket is valid
    if (pSocket->socket < 0)
    {
        NetPrintf(("dirtynetps3: attempt to bind invalid socket\n"));
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // make sure we're not binding port 3658/UDP, which is used by Sony for STUN
    if ((pSocket->type == SOCK_DGRAM) && (SockaddrInGetPort(pName) == 3658))
    {
        NetPrintf(("dirtynetps3: warning -- bind of port 3658/UDP not allowed due to conflict with Sony STUN service\n"));
        pSocket->iLastError = SOCKERR_NONE;
        return(pSocket->iLastError);
    }

    // save local address
    memcpy(&pSocket->local, pName, sizeof(pSocket->local));

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
                // close winsock socket
                NetPrintf(("dirtynetps3: making socket bound to port %d virtual\n", uPort));
                if (pSocket->socket != INVALID_SOCKET)
                {
                    shutdown(pSocket->socket, SOCK_NOSEND);
                    socketclose(pSocket->socket);
                    pSocket->socket = INVALID_SOCKET;
                }
                pSocket->virtualport = uPort;
                pSocket->virtual = TRUE;
                return(0);
            }
        }
    }

    // set up sce sockaddr
    _SockAddrToSceAddr(&SockAddr, pName);

    // do the bind
    if ((iResult = bind(pSocket->socket, &SockAddr, sizeof(SockAddr))) < 0)
    {
        NetPrintf(("dirtynetps3: bind() to port %d failed (err=%s)\n", SockaddrInGetPort(pName), DirtyErrGetName(sys_net_errno)));
    }

    pSocket->iLastError = _XlatError(iResult);
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
        int32_t         - standard network error code (SOCKERR_xxx)

    \Notes
        Only has real meaning for stream protocols. For a datagram protocol, this
        just sets the default remote host.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketConnect(SocketT *pSocket, struct sockaddr *pName, int32_t iNameLen)
{
    struct sockaddr SockAddr;
    int32_t iResult;

    NetPrintf(("dirtynetps3: connecting to %a:%d\n", SockaddrInGetAddr(pName), SockaddrInGetPort(pName)));

    // format address
    _SockAddrToSceAddr(&SockAddr, pName);

    // do the connect
    pSocket->opened = 0;
    iNameLen = sizeof(SockAddr);
    if (((iResult = connect(pSocket->socket, &SockAddr, iNameLen)) < 0) && (sys_net_errno != SYS_NET_EINPROGRESS))
    {
        NetPrintf(("dirtynetps3: connect() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
    }

    pSocket->iLastError = _XlatError(iResult);
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
        int32_t         - standard network error code (SOCKERR_xxx)

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketListen(SocketT *pSocket, int32_t iBacklog)
{
    int32_t iResult;

    // do the listen
    if ((iResult = listen(pSocket->socket, iBacklog)) < 0)
    {
        NetPrintf(("dirtynetps3: listen() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
    }

    pSocket->iLastError = _XlatError(iResult);
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
    int32_t iAddrLen, iResult;
    struct sockaddr SockAddr;
    SocketT *pOpen = NULL;

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

    // set up sce sockaddr
    _SockAddrToSceAddr(&SockAddr, pAddr);

    // perform inet accept
    iAddrLen = sizeof(SockAddr);
    iResult = accept(pSocket->socket, &SockAddr, (socklen_t *)&iAddrLen);
    if (iResult > 0)
    {
        // allocate socket structure and install in list
        pOpen = _SocketOpen(iResult, pSocket->family, pSocket->type, pSocket->proto, 1);
        pSocket->iLastError = SOCKERR_NONE;
    }
    else
    {
         pSocket->iLastError = _XlatError(iResult);

        if (sys_net_errno != SYS_NET_EWOULDBLOCK)
        {
            NetPrintf(("dirtynetps3: accept() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
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

    // if installed, give socket callback right of first refusal
    if (pState->pSendCallback != NULL)
    {
        if ((iResult = pState->pSendCallback(pSocket, pSocket->type, (const uint8_t *)pBuf, iLen, pTo, pState->pSendCallref)) > 0)
        {
            return(iResult);
        }
    }

    // make sure socket ref is valid
    if (pSocket->socket < 0)
    {
        NetPrintf(("dirtynetps3: attempting to send on invalid socket\n"));
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // use appropriate version
    if (pTo == NULL)
    {
        if ((iResult = send(pSocket->socket, pBuf, iLen, 0)) < 0)
        {
            NetPrintf(("dirtynetps3: send() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
        }
    }
    else
    {
        struct sockaddr SockAddr;

        // set up Sce addr
        _SockAddrToSceAddr(&SockAddr, pTo);

        // do the send
        #if SOCKET_VERBOSE
        NetPrintf(("dirtynetps3: sending %d bytes to %a\n", iLen, SockaddrInGetAddr(pTo)));
        #endif
        if ((iResult = sendto(pSocket->socket, pBuf, iLen, 0, &SockAddr, sizeof(SockAddr))) < 0)
        {
            NetPrintf(("dirtynetps3: sendto() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
        }
    }

    // return bytes sent
    pSocket->iLastError = _XlatError(iResult);
    return(pSocket->iLastError);
}

/*F********************************************************************************/
/*!
    \Function    SocketRecvfrom

    \Description
        Receive data from a remote host. If socket is a connected stream, then data can
        only come from that source. A datagram socket can receive from any remote host.

    \Input *pSocket - socket reference
    \Input *pBuf    - buffer to receive data
    \Input iLen     - length of recv buffer
    \Input iFlags   - unused
    \Input *pFrom   - address data was received from (NULL=ignore)
    \Input *pFromLen- length of address

    \Output
        int32_t         - positive=data bytes received, else standard error code

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, int32_t iFlags, struct sockaddr *pFrom, int32_t *pFromLen)
{
    int32_t iRecv = -1;

    // receiving on a dgram socket?
    if ((pSocket->type == SOCK_DGRAM) || (pSocket->type == SOCK_RAW))
    {
        #if SOCKET_ASYNCRECVTHREAD
        // see if we have any data
        if (((iRecv = pSocket->recvstat) > 0) && (iLen > 0))
        {
            // acquire socket receive critical section
            NetCritEnter(&pSocket->recvcrit);

            /*  re-read recvstat now that we have acquired critical section.  we do this
                because SocketControl('push') could have updated the recvstat variable
                to a new value before we acquired the critical section */
            iRecv = pSocket->recvstat;

            // copy sender
            if (pFrom != NULL)
            {
                memcpy(pFrom, &pSocket->recvaddr, sizeof(*pFrom));
                *pFromLen = sizeof(*pFrom);
            }

            // make sure we fit in the buffer
            if (iRecv > iLen)
            {
                iRecv = iLen;
            }
            memcpy(pBuf, pSocket->recvdata, iRecv);

            // mark data as consumed
            pSocket->recvstat = 0;

            // issue a new read request (only if no callback installed for this socket)
            if (pSocket->callback == NULL)
            {
                _SocketRead(_Socket_pState, pSocket);
            }

            // release socket receive critical section
            NetCritLeave(&pSocket->recvcrit);
        }
        else if (pSocket->recvstat <= 0)
        {
            // acquire socket receive critical section
            NetCritEnter(&pSocket->recvcrit);
            // clear error
            if (pSocket->recvstat < 0)
            {
                pSocket->recvstat = 0;
            }
            // release socket receive critical section
            NetCritLeave(&pSocket->recvcrit);

            pSocket->iLastError = SOCKERR_NONE;
            return(pSocket->iLastError);
        }
        #else
        // first check for pushed data
        if (pSocket->recvstat != 0)
        {
            if (pFrom != NULL)
            {
                memcpy(pFrom, &pSocket->recvaddr, sizeof(*pFrom));
            }
            if (iLen > pSocket->recvstat)
            {
                iLen = pSocket->recvstat;
            }
            memcpy(pBuf, pSocket->recvdata, iLen);
            pSocket->recvstat = 0;
            return(iLen);
        }
        // make sure socket ref is valid
        if (pSocket->socket == INVALID_SOCKET)
        {
            pSocket->iLastError = SOCKERR_INVALID;
            return(pSocket->iLastError);
        }
        // do direct recv call
        if (((iRecv = _SocketRecvfrom(pSocket, pBuf, iLen, pFrom, pFromLen)) < 0) && (sys_net_errno != SYS_NET_EAGAIN))
        {
            NetPrintf(("dirtynetps3: _SocketRecvfrom() failed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
        }
        #endif
    }
    else if (pSocket->type == SOCK_STREAM)
    {
        // make sure socket ref is valid
        if (pSocket->socket == INVALID_SOCKET)
        {
            pSocket->iLastError = SOCKERR_INVALID;
            return(pSocket->iLastError);
        }
        // do direct recv call
        if (((iRecv = _SocketRecvfrom(pSocket, pBuf, iLen, pFrom, pFromLen)) < 0) && (sys_net_errno != SYS_NET_EAGAIN))
        {
            NetPrintf(("dirtynetps3: _SocketRecvfrom() failed on a SOCK_STREAM socket (err=%s)\n", DirtyErrGetName(sys_net_errno)));
        }
    }

    // do error conversion
    iRecv = (iRecv == 0) ? SOCKERR_CLOSED : _XlatError(iRecv);

    // simulate packet loss
    #if DIRTYCODE_DEBUG
    if ((_Socket_pState->uPacketLoss != 0) && (pSocket->type == SOCK_DGRAM) && (iRecv > 0) && (SocketSimulatePacketLoss(_Socket_pState->uPacketLoss) != 0))
    {
        pSocket->iLastError = SOCKERR_NONE;
        return(pSocket->iLastError);
    }
    #endif

    // return the error code
    pSocket->iLastError = iRecv;
    return(pSocket->iLastError);
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
            'actv' - returns whether module is active or not
            'addr' - returns local address
            'audt' - prints verbose socket info (debug only)
            'bind' - return bind data (if pSocket == NULL, get socket bound to given port)
            'bndu' - return bind data (only with pSocket=NULL, get SOCK_DGRAM socket bound to given port)
            'conn' - connection status
            'ethr' - local ethernet address (returned in pBuf), 0=success, negative=error
            'macx' - mac address of client (returned in pBuf),  0=success, negative=error
            'maxp' - return max packet size
            'peer' - peer info (only valid if connected)
            'plug' - plug status
            'serr' - last socket error
            'stat' - socket status
            'virt' - TRUE if socket is virtual, else FALSE
        \endverbatim

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketInfo(SocketT *pSocket, int32_t iInfo, int32_t iData, void *pBuf, int32_t iLen)
{
    SocketStateT *pState = _Socket_pState;
    union CellNetCtlInfo Info;
    int32_t iResult;

    // always zero results by default
    if (pBuf != NULL)
    {
        memset(pBuf, 0, iLen);
    }

    // socket module options
    if (pSocket == NULL)
    {
        union CellNetCtlInfo CtlInfo;

        if (iInfo == 'addr')
        {
            uint32_t uLocalAddr;

            // uLocalAddr can only be accessed in the context of the callbackCrit
            NetCritEnter(&pState->callbackCrit);
            uLocalAddr = pState->uLocalAddr;

            if (uLocalAddr == 0)
            {
                if ((iResult = cellNetCtlGetInfo(CELL_NET_CTL_INFO_IP_ADDRESS, &Info)) == 0)
                {
                    uLocalAddr = pState->uLocalAddr = SocketInTextGetAddr(Info.ip_address);
                }
                else
                {
                    NetPrintf(("dirtynetps3: cellNetCtlGetInfo(IP_ADDRESS) failed (err=%s)\n", DirtyErrGetName(iResult)));
                }
            }

            NetCritLeave(&pState->callbackCrit);

            return(uLocalAddr);
        }
        // display verbose socket info
        #if DIRTYCODE_LOGGING
        if (iInfo == 'audt')
        {
            // display global statistics
            NetPrintf(("dirtynetps3: --------------------- audit status ---------------------\n"));
            sys_net_show_ifconfig();

            // walk socket list and display audit information for all sockets
            NetCritEnter(NULL);
            for (pSocket = pState->pSockList; pSocket != NULL; pSocket = pSocket->next)
            {
                SocketInfo(pSocket, iInfo, iData, pBuf, iLen);
            }
            NetCritLeave(NULL);
            NetPrintf(("dirtynetps3: --------------------------------------------------------\n"));
        }
        #endif
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
        if (iInfo == 'conn')
        {
            uint32_t uConnStatus;

            // uConnStatus can only be accessed in the context of the callbackCrit
            NetCritEnter(&pState->callbackCrit);
            uConnStatus = pState->uConnStatus;
            NetCritLeave(&pState->callbackCrit);

            return(uConnStatus);
        }
        if ((iInfo == 'ethr') || (iInfo == 'macx'))
        {
            uint8_t aZeros[6] = { 0, 0, 0, 0, 0, 0 };
            int32_t iRetCode = -1;

            // aMacAddr can only be accessed in the context of the callbackCrit
            NetCritEnter(&pState->callbackCrit);

            // try to get mac address if we don't already have it
            if (!memcmp(pState->aMacAddr, aZeros, sizeof(pState->aMacAddr)))
            {
                // get ethernet address
                if ((iResult = cellNetCtlGetInfo(CELL_NET_CTL_INFO_ETHER_ADDR, &CtlInfo)) >= 0)
                {
                    memcpy(pState->aMacAddr, CtlInfo.ether_addr.data, sizeof(pState->aMacAddr));
                    NetPrintf(("dirtynetps3: obtained MAC address $%02x%02x%02x%02x%02x%02x\n",
                        pState->aMacAddr[0], pState->aMacAddr[1], pState->aMacAddr[2],
                        pState->aMacAddr[3], pState->aMacAddr[4], pState->aMacAddr[5]));
                }
                else
                {
                    NetPrintf(("dirtynetps3: cellNetCtlGetInfo(ETHER_ADDR) failed (err=%s)\n", DirtyErrGetName(iResult)));
                }
            }

            // return MAC address
            if ((pBuf != NULL) && (iLen >= (signed)sizeof(pState->aMacAddr)))
            {
                memcpy(pBuf, &pState->aMacAddr, sizeof(pState->aMacAddr));
                iRetCode = 0;
            }

            NetCritLeave(&pState->callbackCrit);

            return(iRetCode);
        }
        // return max packet size
        if (iInfo == 'maxp')
        {
            return(pState->iMaxPacket);
        }
        if (iInfo == 'plug')
        {
            if ((iResult = cellNetCtlGetInfo(CELL_NET_CTL_INFO_LINK, &CtlInfo)) == 0)
            {
                return(CtlInfo.link);
            }
            else
            {
                NetPrintf(("dirtynetps3: cellNetCtlGetInfo(LINK) failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
        }
        return(-1);
    }

    // display verbose socket info for this socket
    #if DIRTYCODE_LOGGING
    if (iInfo == 'audt')
    {
        sys_net_sockinfo_ex_t SockInfo;
        if ((iResult = sys_net_get_sockinfo_ex(pSocket->socket, &SockInfo, 1, 0)) >= 0)
        {
            NetPrintf(("dirtynetps3: id=%d pr=%d rq=%d sq=%d la=%a:%d ra=%a:%d st=%d\n",
                SockInfo.s,
                SockInfo.proto,
                SockInfo.recv_queue_length,
                SockInfo.send_queue_length,
                SockInfo.local_adr.s_addr,
                SockInfo.local_port,
                SockInfo.remote_adr.s_addr,
                SockInfo.remote_port,
                SockInfo.state));
        }
        else
        {
            NetPrintf(("dirtynetps3: sys_net_get_sockinfo_ex() failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
        return(0);
    }
    #endif

    // return local bind data
    if (iInfo == 'bind')
    {
        if (iLen >= (signed)sizeof(pSocket->local))
        {
            struct sockaddr SceAddr, SockAddr;
            uint32_t iAddrLen = sizeof(SceAddr);

            if (pSocket->virtual == TRUE)
            {
                SockaddrInit((struct sockaddr *)pBuf, AF_INET);
                SockaddrInSetPort((struct sockaddr *)pBuf, pSocket->virtualport);
            }
            else
            {
                getsockname(pSocket->socket, &SceAddr, &iAddrLen);
                memcpy(&SockAddr, &SceAddr, sizeof(SockAddr));
                SockAddr.sa_family = SceAddr.sa_family;
                memcpy(pBuf, &SockAddr, iLen);
            }
            return(0);
        }
    }

    // return whether the socket is virtual or not
    if (iInfo == 'virt')
    {
        return(pSocket->virtual);
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
            struct sockaddr SceAddr, SockAddr;
            uint32_t iAddrLen = sizeof(SceAddr);

            getpeername(pSocket->socket, &SceAddr, &iAddrLen);
            memcpy(&SockAddr, &SceAddr, sizeof(SockAddr));
            SockAddr.sa_family = SceAddr.sa_family;
            memcpy(pBuf, &SockAddr, iLen);
        }
        return(0);
    }

    // return last socket error
    if (iInfo == 'serr')
    {
        return(pSocket->iLastError);
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

        // if not connected, use select to determine connect
        if (pSocket->opened == 0)
        {
            memset(&PollFd, 0, sizeof(PollFd));
            PollFd.fd = pSocket->socket;
            PollFd.events = POLLOUT;
            if (socketpoll(&PollFd, 1, 0) != 0)
            {
                /*
                if we got an exception, that means connect failed.
                ps3sdk (370.001-SNC360.1) says POLLHUP will not be returned currently, 
                but we still test it here for sure.
                */
                if ((PollFd.revents & POLLERR) || (PollFd.revents & POLLHUP))
                {
                    NetPrintf(("dirtynetps3: read exception on connect\n"));
                    pSocket->opened = -1;
                }
                // if socket is writable, that means connect succeeded
                else if (PollFd.revents & POLLOUT)
                {
                    NetPrintf(("dirtynetps3: connection open\n"));
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
            if (socketpoll(&PollFd, 1, 0) != 0)
            {
                // if we got an exception, that means connect failed (usually closed by remote peer)
                if ((PollFd.revents & POLLERR) || (PollFd.revents & POLLHUP))
                {
                    NetPrintf(("dirtynetps3: connection failure\n"));
                    pSocket->opened = -1;
                }
                else if (PollFd.revents & POLLIN)
                {
                    // if socket is readable and recv(MSG_PEEK) returns 0, that means connect closed
                    if (recv(pSocket->socket, (char*)&iData, 1, MSG_PEEK) <= 0)
                    {
                        pSocket->iLastError = SOCKERR_CLOSED;
                        NetPrintf(("dirtynetps3: connection closed (err=%s)\n", DirtyErrGetName(sys_net_errno)));
                        pSocket->opened = -1;
                    }
                }
            }
        }

        // return connect status
        return(pSocket->opened);
    }

    // unhandled option?
    NetPrintf(("dirtynetps3: unhandled SocketInfo() option '%c%c%c%c'\n",
        (unsigned char)(iInfo >> 24),
        (unsigned char)(iInfo >> 16),
        (unsigned char)(iInfo >>  8),
        (unsigned char)(iInfo >>  0)));
    return(-1);
}

/*F********************************************************************************/
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
        int32_t         - zero

    \Notes
        A callback will reset the idle timer, so when specifying a callback and an
        idle processing time, the idle processing time represents the maximum elapsed
        time between calls.

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketCallback(SocketT *sock, int32_t mask, int32_t idle, void *ref, int32_t (*proc)(SocketT *sock, int32_t flags, void *ref))
{
    sock->callidle = idle;
    sock->callmask = mask;
    sock->callref = ref;
    sock->callback = proc;
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
            'conn' - init network stack
            'disc' - bring down network stack
            'loss' - simulate packet loss (debug only)
            'maxp' - set max udp packet size
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'ndly' - set TCP_NODELAY state for given stream socket (iData1=zero or one)
            'pool' - set sys_net mempool size (default=128k)
            'push' - push data into given socket receive buffer (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'rbuf' - set socket recv buffer size
            'sbuf' - set socket send buffer size
            'sdcb' - set send callback (pData2=callback, pData3=callref)
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

    // init network stack and bring up interface
    if (iOption == 'conn')
    {
        int32_t iState;

        if ((iResult = cellNetCtlInit()) < 0)
        {
            if (iResult == (signed)CELL_NET_CTL_ERROR_NOT_TERMINATED)
            {
                NetPrintf(("dirtynetps3: warning -- cellNetCtl already initialized; cellNetCtlTerm() will not be called on disconnect\n"));
                pState->bNetCtlAlreadyInitialized = TRUE;
            }
            else
            {
                NetPrintf(("dirtynetps3: cellNetCtlInit() failed err=%s\n", DirtyErrGetName(iResult)));
                return(-1);
            }
        }
        else
        {
            pState->bNetCtlAlreadyInitialized = FALSE;
        }

        if ((iResult = cellNetCtlAddHandler(_SocketNetCtlHandler, pState, &pState->iHandlerId)) < 0)
        {
            NetPrintf(("dirtynetps3: cellNetCtlAddHandler() failed err=%s\n", DirtyErrGetName(iResult)));
            if (pState->bNetCtlAlreadyInitialized == FALSE)
            {
                cellNetCtlTerm();
            }
            return(-1);
        }

        // uConnStatus can only be accessed in the context of the callbackCrit
        NetCritEnter(&pState->callbackCrit);
        pState->uConnStatus = '~con';
        NetCritLeave(&pState->callbackCrit);

        // to compensate for any event we may have missed because of late registration of the handler,
        // we poll system connection status once and fake a connection status transition if appropriate
        cellNetCtlGetState(&iState);
        switch (iState)
        {
            case CELL_NET_CTL_STATE_Disconnected:
                break;
            case CELL_NET_CTL_STATE_Connecting:
                _SocketNetCtlHandler(CELL_NET_CTL_STATE_Disconnected, iState, CELL_NET_CTL_EVENT_CONNECT_REQ, 0, pState);
                break;
            case CELL_NET_CTL_STATE_IPObtaining:
                _SocketNetCtlHandler(CELL_NET_CTL_STATE_Disconnected, iState, CELL_NET_CTL_EVENT_ESTABLISH, 0, pState);
                break;
            case CELL_NET_CTL_STATE_IPObtained:
                _SocketNetCtlHandler(CELL_NET_CTL_STATE_Disconnected, iState, CELL_NET_CTL_EVENT_GET_IP, 0, pState);
                break;
            default:
                NetPrintf(("dirtynetps3: cellNetCtlGetState() returned unknown network connection state (%d)\n", iState));
                break;
        }

        return(0);
    }
    // bring down interface
    if (iOption == 'disc')
    {
        NetPrintf(("dirtynetps3: disconnecting from network\n"));
        if ((iResult = cellNetCtlDelHandler(pState->iHandlerId)) < 0)
        {
            NetPrintf(("dirtynetps3: cellNetCtlDelHandler() failed err=%s\n", DirtyErrGetName(iResult)));
        }
        if (pState->bNetCtlAlreadyInitialized == FALSE)
        {
            cellNetCtlTerm();
        }
        return(0);
    }
    // set up packet loss simulation
    #if DIRTYCODE_DEBUG
    if (iOption == 'loss')
    {
        pState->uPacketLoss = (unsigned)iData1;
        NetPrintf(("dirtynetps3: packet loss simulation %s (param=0x%08x)\n", pState->uPacketLoss ? "enabled" : "disabled", pState->uPacketLoss));
        return(0);
    }
    #endif
    // set max udp packet size
    if (iOption == 'maxp')
    {
        NetPrintf(("dirtynetps3: setting max udp packet size to %d\n", iData1));
        pState->iMaxPacket = iData1;
        return(0);
    }
    // if a stream socket, set nonblocking/blocking mode
    if ((iOption == 'nbio') && (pSocket != NULL) && (pSocket->type == SOCK_STREAM))
    {
        iResult = setsockopt(pSocket->socket, SOL_SOCKET, SO_NBIO, &iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);
        NetPrintf(("dirtynetps3: setting socket:0x%x to %s mode %s (LastError=%d).\n", pSocket, iData1 ? "nonblocking" : "blocking", iResult ? "failed" : "succeeded", pSocket->iLastError));
        return(pSocket->iLastError);
    }
    // if a stream socket, set TCP_NODELAY state
    if ((iOption == 'ndly') && (pSocket != NULL) && (pSocket->type == SOCK_STREAM))
    {
        iResult = setsockopt(pSocket->socket, IPPROTO_TCP, TCP_NODELAY, &iData1, sizeof(iData1));
        pSocket->iLastError = _XlatError(iResult);
        return(pSocket->iLastError);
    }
    if (iOption == 'pool')
    {
        NetPrintf(("dirtynetps3: setting netpool size=%d bytes\n", iData1));
        _Socket_iMemPoolSize = iData1;
        return(0);
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
                NetPrintf(("dirtynetps3: request to push %d bytes of data discarded (max=%d)\n", iData1, sizeof(pSocket->recvdata)));
                #if SOCKET_ASYNCRECVTHREAD
                NetCritLeave(&pSocket->recvcrit);
                #endif
                return(-1);
            }

            if (pSocket->recvstat > 0)
            {
                NetPrintf(("dirtynetps3: warning - overwriting packet data with SocketControl('push')\n"));
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
            NetPrintf(("dirtynetps3: warning - call to SocketControl('push') ignored because pSocket is NULL\n"));
            return(-1);
        }
        return(0);
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
        pSocket->iLastError = _XlatError(iResult);

        // get new size
        getsockopt(pSocket->socket, SOL_SOCKET, iSockOpt, (char *)&iNewSize, &uOptLen);
        NetPrintf(("dirtynetps3: setsockopt(%s) changed buffer size from %d to %d\n", (iOption == 'rbuf') ? "SO_RCVBUF" : "SO_SNDBUF",
            iOldSize, iNewSize));

        return(pSocket->iLastError);
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
    // set global send callback
    if (iOption == 'sdcb')
    {
        pState->pSendCallback = (SocketSendCallbackT *)pData2;
        pState->pSendCallref = pData3;
        return(0);
    }
    // unhandled
    NetPrintf(("dirtynetps3: unhandled control option '%c%c%c%c'\n",
        (uint8_t)(iOption>>24), (uint8_t)(iOption>>16), (uint8_t)(iOption>>8), (uint8_t)iOption));
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
    uint32_t uLocalAddr;

    // uLocalAddr can only be accessed in the context of the callbackCrit
    NetCritEnter(&pState->callbackCrit);
    uLocalAddr = pState->uLocalAddr;
    NetCritLeave(&pState->callbackCrit);

    return(uLocalAddr);
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
    HostentT *pHost;

    NetPrintf(("dirtynetps3: looking up address for host '%s'\n", pText));

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

    // check for dot notation
    if ((iAddr = SocketInTextGetAddr(pText)) != 0)
    {
        // we've got a dot-notation address
        pHost->addr = iAddr;
        pHost->done = 1;
        // return completed record
        return(pHost);
    }

    // copy over the target address
    strnzcpy(pHost->name, pText, sizeof(pHost->name));
    pHost->sema = 1;

    // create dns lookup thread
    if ((iResult = sys_ppu_thread_create(&pPriv->iThreadId, _SocketLookupThread, (uintptr_t)pPriv, 10, 0x4000, 0, "SocketLookup")) < 0)
    {
        NetPrintf(("dirtynetps3: sys_ppu_thread_create() failed (err=%s)\n", DirtyErrGetName(iResult)));
        pPriv->Host.addr = 0;
        pPriv->Host.done = -1;
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

    \Input *host    - local sockaddr struct
    \Input hostlen  - length of structure (sizeof(host))
    \Input *dest    - remote sockaddr struct
    \Input destlen  - length of structure (sizeof(dest))

    \Output
        int32_t         - zero=success, negative=error

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketHost(struct sockaddr *host, int32_t hostlen, const struct sockaddr *dest, int32_t destlen)
{
    SocketStateT *pState = _Socket_pState;

    // must be same kind of addresses
    if (hostlen != destlen)
    {
        return(-1);
    }

    // do family specific lookup
    if (dest->sa_family == AF_INET)
    {
        // special case destination of zero or loopback to return self
        if ((SockaddrInGetAddr(dest) == 0) || (SockaddrInGetAddr(dest) == 0x7f000000))
        {
            memcpy(host, dest, hostlen);
            return(0);
        }
        else
        {
            memset(host, 0, hostlen);
            host->sa_family = AF_INET;

            // uLocalAddr can only be accessed in the context of the callbackCrit
            NetCritEnter(&pState->callbackCrit);
            SockaddrInSetAddr(host, pState->uLocalAddr);
            NetCritLeave(&pState->callbackCrit);
            return(0);
        }
    }

    // unsupported family
    memset(host, 0, hostlen);
    return(-3);
}


