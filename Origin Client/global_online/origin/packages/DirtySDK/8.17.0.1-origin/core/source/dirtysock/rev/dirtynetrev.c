/*H********************************************************************************/
/*!
    \File dirtynetrev.c

    \Description
        Provides a wrapper that translates the Revolution sockets interface to the
        DirtySock portable networking interface.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 11/27/2006 (jbrookes) First Version
    \Version 11/25/2008 (mclouatre) Improved error handling for SOInit() and SOStartup()
    \Version 01/15/2010 (mclouatre) Added support for async recv thread
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <dwc.h>
#include <revolution/os.h>
#include <revolution/soex.h>

#include <stdio.h>
#include <string.h>

// undefine some stuff DWC defines that we want to define ourselves
#undef AF_INET
#undef IPPROTO_TCP
#undef IPPROTO_UDP
#undef SOCK_DGRAM
#undef SOCK_STREAM
#undef sockaddr
#undef sockaddr_in
#undef in_addr

#include "dirtysock.h"
#include "dirtyvers.h"
#include "dirtymem.h"
#include "dirtyerr.h"

/*** Defines **********************************************************************/

#define INVALID_SOCKET      (-1)
#define SOCKET_MAXPOLL      (32)
#define SOCKET_VERBOSE      (DIRTYCODE_DEBUG && FALSE)

#define SOCKET_MAXPING      (16)    // max number of queued ping requests
#define SOCKET_PINGTIMEOUT  (1)     // seconds before a ping request is timed out

/*** Type Definitions *************************************************************/

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
    uint8_t unused;

    int32_t socket;             //!< rev socket ref
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

    uint32_t temp;
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

//! standard icmp packet header
typedef struct HeaderIcmp
{
    unsigned char type;         //!< packet type
    unsigned char code;         //!< sub-type
    unsigned char check[2];     //!< checksum
    unsigned char idn[2];       //!< identifier
    unsigned char seq[2];       //!< sequence number
} HeaderIcmp;

//! standard ipv4+icmp packet header
typedef struct EchoHeaderT
{
    HeaderIpv4 ipv4;            //!< ip header
    HeaderIcmp icmp;            //!< icmp header
} EchoHeaderT;

//! socket ping struct
typedef struct SocketPingReqT
{
    uint32_t uPingAddr;
    uint16_t uPingActive;
    uint16_t uPingResult;
    uint16_t uSeqn;
    uint16_t uIdentToken;
} SocketPingReqT;

//! local state
typedef struct SocketStateT
{
    SocketT  *pSockList;                //!< master socket list
    SocketT  *pSockKill;                //!< list of killed sockets

    uint16_t aVirtualPorts[SOCKET_MAXVIRTUALPORTS]; //!< virtual port list

    // module memory group
    int32_t  iMemGroup;                 //!< module mem group id
    void     *pMemGroupUserData;        //!< user data associated with mem group

    int32_t  iThreadPrio;               //!< thread priority module was started at
    int32_t  iMaxPacket;                 //!< maximum packet size

    uint32_t uConnStatus;               //!< current connection status
    uint32_t uLocalAddr;                //!< local internet address for active interface
    uint8_t  aMacAddr[6];               //!< mac address for active interface
    uint8_t  bSoLibAlreadyInitialized;  //!< TRUE if WII socket lib was already initialized with SOInit() at connect time, else FALSE
    uint8_t  bSoLibAlreadyStarted;      //!< TRUE if WII socket lib was already started with SOStartup() at startup time, else FALSE

    uint32_t uGameId;                   //!< DWC game identifier
    char     strAuthCode[8];            //!< DWC auth code
    char     strAuthEnv[8];             //!< auth environment ("dev" or "prod")

    int32_t  iDWCErrCode;               //!< most recent DWC error code
    DWCErrorType eDWCErrType;           //!< most recent DWC error type

    int32_t  iLinkStatus;               //!< current link status reported with NCDGetLinkStatus()

    DWCSvlResult SvlResult;             //!< Svl result (includes hostname and token)

    uint32_t uPacketLoss;               //!< packet loss simulation (debug only)

    #if SOCKET_ASYNCRECVTHREAD
    OSThread AsyncRecvThread;
    #endif
    OSThread StartThread;
    OSThread LookThread;
    OSThread PingThread;

    NetCritT LookCrit;                  //!< lookup critical section
    NetCritT PingCrit;                  //!< ping critical section
    HostentT *pHost;                    //!< lookup list head

    SocketPingReqT aPingRequests[SOCKET_MAXPING];
    volatile int32_t iPingHead;
    volatile int32_t iPingTail;

    #if SOCKET_ASYNCRECVTHREAD
    volatile int32_t iAsyncRecvDone;    //!< set to one to kill async recv thread
    #endif
    volatile int32_t iLookDone;         //!< set to one to kill lookup thread

    SocketSendCallbackT *pSendCallback; //!< global send callback
    void                *pSendCallref;  //!< user callback data

    uint8_t  bDWCInitialized;           //!< tracks whether DWC was initialized or not
    volatile uint8_t  bSocStarted;      //!< tracks whether SOC lib was started or not
} SocketStateT;

/*** Variables ********************************************************************/

//! module state ref
static SocketStateT *_Socket_pState = NULL;

#if SOCKET_ASYNCRECVTHREAD
//! socket async recv thread stack
#define SOCKET_ASYNCRECVSTACKSIZE  (8192)
static uint64_t _Socket_aAsyncRecvStack[SOCKET_ASYNCRECVSTACKSIZE/sizeof(uint64_t)];
#define SOCKET_ASYNCRECVSTACKSIZE_WORD  (sizeof(_Socket_aAsyncRecvStack)/sizeof(_Socket_aAsyncRecvStack[0]))
#endif

//! socket lookup thread stack
#define SOCKET_LOOKUPSTACKSIZE  (8192)
static uint64_t _Socket_aLookupStack[SOCKET_LOOKUPSTACKSIZE/sizeof(uint64_t)];
#define SOCKET_LOOKUPSTACKSIZE_WORD  (sizeof(_Socket_aLookupStack)/sizeof(_Socket_aLookupStack[0]))

//! socket ping thread stack
#define SOCKET_PINGSTACKSIZE  (8192)
static uint64_t _Socket_aPingStack[SOCKET_PINGSTACKSIZE/sizeof(uint64_t)];
#define SOCKET_PINGSTACKSIZE_WORD  (sizeof(_Socket_aPingStack)/sizeof(_Socket_aPingStack[0]))

//! socket startup thread stack
#define SOCKET_STARTUPSTACKSIZE  (8192)
static uint64_t _Socket_aStartupStack[SOCKET_STARTUPSTACKSIZE/sizeof(uint64_t)];
#define SOCKET_STARTUPSTACKSIZE_WORD  (sizeof(_Socket_aStartupStack)/sizeof(_Socket_aStartupStack[0]))

/* There is a problem with the current Nintendo library where SOGetPeerName writes
   to a block of memory after that block has already been freed.  To work around
   this problem, we set a flag so that the memory manager can assign a special
   block of memory for that function, which in turn means that it doesn't stomp
   over anything else. */
static uint8_t _Socket_bGetPeerNameAllocHack = 0;
static uint8_t *_Socket_pGetPeerNameAllocHackMem = NULL;


/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function    _XlatError

    \Description
        Translate a BSD error to dirtysock

    \Input iErr     - BSD error code (e.g EAGAIN)

    \Output
        int32_t     - dirtysock error

    \Version 08/28/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _XlatError(int32_t iErr)
{
    if (iErr < 0)
    {
        /* if we get an SO_ENETRESET error, that indicates that the network
           connection has been reset, so we change the global connection
           status accordingly */
        if (iErr == SO_ENETRESET)
        {
            SocketStateT *pState = _Socket_pState;
            if ((pState != NULL) && ((pState->uConnStatus >> 24) != '-'))
            {
                NetPrintf(("dirtynetrev: detected socket network reset event; updating connection status\n"));
                pState->uConnStatus = '-rst';
            }
        }

        if ((iErr == SO_EWOULDBLOCK) || (iErr == SO_EINPROGRESS))
            iErr = SOCKERR_NONE;
        else if (iErr == SO_EHOSTUNREACH)
            iErr = SOCKERR_UNREACH;
        else if (iErr == SO_ENOTCONN)
            iErr = SOCKERR_NOTCONN;
        else if (iErr == SO_ECONNREFUSED)
            iErr = SOCKERR_REFUSED;
        else if (iErr == SO_ECONNRESET)
            iErr = SOCKERR_CONNRESET;
        else
            iErr = SOCKERR_OTHER;
    }
    return(iErr);
}

/*F********************************************************************************/
/*!
    \Function    _SockAddrToSoAddr

    \Description
        Translate DirtySock socket address to Nintendo socket address.

    \Input *pSoAddr     - [out] pointer to Nintendo address to set up
    \Input *pSockAddr   - pointer to source DirtySock address

    \Version 06/21/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SockAddrToSoAddr(SOSockAddr *pSoAddr, const struct sockaddr *pSockAddr)
{
    memcpy(pSoAddr, pSockAddr, sizeof(*pSoAddr));
    pSoAddr->len = sizeof(*pSoAddr);
    pSoAddr->family = (unsigned char)pSockAddr->sa_family;
}

/*F********************************************************************************/
/*!
    \Function _SocketAlloc

    \Description
        Handle memory allocation requests from network stack

    \Input uName    - SO_MEM_*
    \Input iSize    - size of block to allocate

    \Output
        void *      - pointer to newly allocated memory block, or NULL

    \Version 08/28/2006 (jbrookes)
*/
/********************************************************************************F*/
static void *_SocketAlloc(uint32_t uName, int32_t iSize)
{
    if ((_Socket_bGetPeerNameAllocHack) && (iSize == 64))
    {
        #if DIRTYCODE_DEBUG
        memset(_Socket_pGetPeerNameAllocHackMem, 0xab, iSize);
        #endif
        return(_Socket_pGetPeerNameAllocHackMem);
    }

    return(DirtyMemAlloc(iSize, NINSOC_MEMID, 'soc0' + uName, NULL));
}

/*F********************************************************************************/
/*!
    \Function _SocketFree

    \Description
        Handle memory free requests from network stack

    \Input uName    - SO_MEM_*
    \Input *pMem    - pointer to memory block to free
    \Input iSize    - size of allocated memory block

    \Version 08/28/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketFree(uint32_t uName, void *pMem, int32_t iSize)
{
    if (pMem == _Socket_pGetPeerNameAllocHackMem)
    {
        #if DIRTYCODE_DEBUG
        memset(_Socket_pGetPeerNameAllocHackMem, 0xfe, iSize);
        #endif
        return;
    }
    DirtyMemFree(pMem, NINSOC_MEMID, 'soc0' + uName, NULL);
}

/*F********************************************************************************/
/*!
    \Function _SocketDWCAlloc

    \Description
        Handle memory allocation requests from DWC

    \Input uName    - SOC_MEM_*
    \Input uSize    - size of block to allocate
    \Input iAlign   - alignment

    \Output
        void *      - pointer to newly allocated memory block, or NULL

    \Version 08/28/2006 (jbrookes)
*/
/********************************************************************************F*/
static void *_SocketDWCAlloc(DWCAllocType uName, uint32_t uSize, int32_t iAlign)
{
    return(DirtyMemAlloc(uSize, NINDWC_MEMID, 'dwc0' + uName, NULL));
}

/*F********************************************************************************/
/*!
    \Function _SocketDWCFree

    \Description
        Handle memory free requests from network stack

    \Input uName    - SOC_MEM_*
    \Input *pMem    - pointer to memory block to free
    \Input uSize    - size of allocated memory block

    \Version 08/28/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketDWCFree(DWCAllocType uName, void *pMem, uint32_t uSize)
{
    DirtyMemFree(pMem, NINDWC_MEMID, 'dwc0' + uName, NULL);
}

/*F********************************************************************************/
/*!
    \Function _SocketDWCGetError

    \Description
        Get most recent error code and type from DWC.

    \Input *pState  - module state

    \Version 01/19/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketDWCGetError(SocketStateT *pState)
{
    DWCErrorType eErrorType;
    DWCError eError;
    int32_t iErrorCode;
    #if DIRTYCODE_LOGGING
    static char *_DWCErrorTypeNames[] =
    {
        "DWC_ETYPE_NO_ERROR",
        "DWC_ETYPE_LIGHT",          // with a game-specific display only, no error displayed is required
        "DWC_ETYPE_SHOW_ERROR",     // only display error code
        "DWC_ETYPE_SHUTDOWN_FM",    // it is necessary to call DWC_ShutdownFriendsMatch() to terminate FriendsMatch Library.
        "DWC_ETYPE_SHUTDOWN_GHTTP", // it is necessary to call DWC_ShutdownGHTTP() to terminate the GHTTP Library.
        "DWC_ETYPE_SHUTDOWN_ND",    // it is necessary to call DWC_NdCleanupAsync() to terminate DWC_Nd Library.
        "DWC_ETYPE_DISCONNECT",     // if DWC_ShutdownFriendsMatch() is called during FriendsMatch processing, it's necessary to have already disconnected communications using DWC_CleanupInet().
        "DWC_ETYPE_FATAL",          // since this is equivalent to a Fatal Error it's necessary to prompt the user to turn power OFF.
    };
    #endif

    eError = DWC_GetLastErrorEx(&iErrorCode, &eErrorType);

    pState->iDWCErrCode = iErrorCode;
    pState->eDWCErrType = eErrorType;

    NetPrintf(("dirtynetrev: DWC error %s (code=%d type=%s)\n",
        DirtyErrGetName(eError), iErrorCode, _DWCErrorTypeNames[eErrorType]));
}

/*F********************************************************************************/
/*!
    \Function    _SocketGetPeerName

    \Description
        Conveinence wrapper for SOGetPeerName

    \Input *pSocket     - socket to get peer name from
    \Input *pSockAddr   - output for peer name (may be NULL)

    \Output
        int32_t         - result code from SOGetPeerName

    \Version 02/15/2008 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketGetPeerName(SocketT *pSocket, struct sockaddr *pSockAddr)
{
    SOSockAddrIn SoAddr;
    int32_t iResult;

    // get peer name
    memset(&SoAddr, 0, sizeof(SoAddr));
    SoAddr.len = sizeof(SoAddr);

    _Socket_bGetPeerNameAllocHack = TRUE;
    iResult = SOGetPeerName(pSocket->socket, &SoAddr);
    _Socket_bGetPeerNameAllocHack = FALSE;

    // convert from native to dirtysock format
    if (pSockAddr != NULL)
    {
        memcpy(pSockAddr, &SoAddr, sizeof(*pSockAddr));
        pSockAddr->sa_family = SoAddr.family;
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function    _SocketConnUpdate

    \Description
        Update during connection process.

    \Input *pState      - pointer to module state

    \Output
        uint32_t        - current connection status

    \Version 02/27/2007 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _SocketConnUpdate(SocketStateT *pState)
{
    if (pState->uConnStatus == '~con')
    {
        // get host address
        if ((pState->uLocalAddr = SOGetHostID()) != 0)
        {
            DWCAUTHSERVER iAuthServer;
            int32_t iOptLen, iResult;

            // get interface MAC address
            iOptLen = sizeof(pState->aMacAddr);
            if ((iResult = SOGetInterfaceOpt(NULL, SO_SOL_CONFIG, SO_CONFIG_MAC_ADDRESS, pState->aMacAddr, &iOptLen)) != 0)
            {
                NetPrintf(("dirtynetrev: SOGetInterfaceOpt(MAC_ADDRESS) failed (err=%s)\n", DirtyErrGetName(iResult)));
            }
            NetPrintf(("dirtynetrev: acquired host addr=%a mac=$%02x%02x%02x%02x%02x%02x\n", pState->uLocalAddr,
                pState->aMacAddr[0], pState->aMacAddr[1], pState->aMacAddr[2],
                pState->aMacAddr[3], pState->aMacAddr[4], pState->aMacAddr[5]));

            // select DWC server environment
            iAuthServer = !strcmp(pState->strAuthEnv, "prod") ? DWC_AUTHSERVER_RELEASE : DWC_AUTHSERVER_DEBUG;
            NetPrintf(("dirtynetrev: setting NAS environment to %s\n", iAuthServer == DWC_AUTHSERVER_DEBUG ? "debug" : "release"));

            // initialize DWC library
            NetPrintf(("dirtynetrev: initializing DWC with authserver=%d and gameid=%d\n", iAuthServer, pState->uGameId));
            DWC_Init(iAuthServer, "", pState->uGameId, _SocketDWCAlloc, _SocketDWCFree);
            pState->bDWCInitialized = TRUE;

            // specify DWC debug display level
            #if DIRTYCODE_LOGGING
            DWC_SetReportLevel(DWC_REPORTFLAG_ALL);
            #endif

            // start login process
            if (DWC_NasLoginAsync() == TRUE)
            {
                pState->uConnStatus = '~log';
            }
            else
            {
                NetPrintf(("dirtynetrev: DWC_NasLoginAsync() failed\n"));
                _SocketDWCGetError(pState);
                pState->uConnStatus = '-log';
            }
        }
    }
    if (pState->uConnStatus == '~log')
    {
        #if DIRTYCODE_LOGGING
        static char *_DWCLoginStateNames[] =
        {
            "DWC_NASLOGIN_STATE_DIRTY",
            "DWC_NASLOGIN_STATE_IDLE",
            "DWC_NASLOGIN_STATE_HTTP",
            "DWC_NASLOGIN_STATE_SUCCESS",
            "DWC_NASLOGIN_STATE_ERROR",
            "DWC_NASLOGIN_STATE_CANCELED",
        };
        static DWCNasLoginState _eLastLoginState = 0;
        #endif
        DWCNasLoginState eLoginState;

        // update login process
        eLoginState = DWC_NasLoginProcess();
        #if DIRTYCODE_LOGGING
        if (eLoginState != _eLastLoginState)
        {
            NetPrintf(("dirtynetrev: %s->%s\n", _DWCLoginStateNames[_eLastLoginState], _DWCLoginStateNames[eLoginState]));
            _eLastLoginState = eLoginState;
        }
        #endif

        // connected?
        if (eLoginState == DWC_NASLOGIN_STATE_SUCCESS)
        {
            NetPrintf(("dirtynetrev: login complete\n"));
            if (DWC_SvlInit() == TRUE)
            {
                if (DWC_SvlGetTokenAsync(pState->strAuthCode, &pState->SvlResult) == TRUE)
                {
                    pState->uConnStatus = '~ath';
                }
                else
                {
                    NetPrintf(("dirtynetrev: DWC_SvlGetTokenAsync() failed\n"));
                    pState->uConnStatus = '-ath';
                }
            }
            else
            {
                NetPrintf(("dirtynetrev: DWC_SvlInit() failed\n"));
                pState->uConnStatus = '-ath';
            }
        }
        else if (eLoginState == DWC_NASLOGIN_STATE_ERROR)
        {
            NetPrintf(("dirtynetrev: NAS Login failed\n"));
            _SocketDWCGetError(pState);
            pState->uConnStatus = '-log';
        }
        else if (eLoginState == DWC_NASLOGIN_STATE_CANCELED)
        {
            NetPrintf(("dirtynetrev: login canceled\n"));
            pState->uConnStatus = '-lcn';
        }
    }
    if (pState->uConnStatus == '~ath')
    {
        #if DIRTYCODE_LOGGING
        static char *_DWCSvlStateNames[] =
        {
            "DWC_SVL_STATE_DIRTY",
            "DWC_SVL_STATE_IDLE",
            "DWC_SVL_STATE_HTTP",
            "DWC_SVL_STATE_SUCCESS",
            "DWC_SVL_STATE_ERROR",
            "DWC_SVL_STATE_CANCELED",
        };
        static DWCSvlState _eLastSvlState = 0;
        #endif
        DWCSvlState eSvlState;

        // update login process
        eSvlState = DWC_SvlProcess();
        #if DIRTYCODE_LOGGING
        if (eSvlState != _eLastSvlState)
        {
            NetPrintf(("dirtynetrev: %s->%s\n", _DWCSvlStateNames[_eLastSvlState], _DWCSvlStateNames[eSvlState]));
            _eLastSvlState = eSvlState;
        }
        #endif

        // connected?
        if (eSvlState == DWC_SVL_STATE_SUCCESS)
        {
            NetPrintf(("dirtynetrev: login complete; rslt=%d host=%s token=%s\n", pState->SvlResult.status, pState->SvlResult.svlhost, pState->SvlResult.svltoken));
            pState->uConnStatus = '+onl';
        }
        else if (eSvlState == DWC_SVL_STATE_ERROR)
        {
            NetPrintf(("dirtynetrev: Svl authentication failed\n"));
            _SocketDWCGetError(pState);
            pState->uConnStatus = '-ath';
        }
        else if (eSvlState == DWC_SVL_STATE_CANCELED)
        {
            NetPrintf(("dirtynetrev: Svl authentication canceled\n"));
            pState->uConnStatus = '-acn';
        }
    }

    return(pState->uConnStatus);
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
    uint32_t uTrue = 1;

    // allocate memory
    if ((pSocket = DirtyMemAlloc(sizeof(*pSocket), SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetrev: unable to allocate memory for socket\n"));
        return(NULL);
    }
    memset(pSocket, 0, sizeof(*pSocket));

    if (iSocket == -1)
    {
        // is the socket a raw ICMP socket?
        if ((iType == SOCK_RAW) && (iProto == IPPROTO_ICMP))
        {
            // don't create socket ref, let the ping thread handle that
        }
        else if ((iSocket = SOSocket(SO_AF_INET, iType, iProto)) >= 0)
        {
            // if udp, allow broadcast
            #if 0
            if (iType == SOCK_DGRAM)
            {
                // $$ jlb $$ - no SO_BROADCAST option
            }
            #endif
        }
        else
        {
            NetPrintf(("dirtynetrev: SOSocket() failed (err=%s)\n", DirtyErrGetName(iSocket)));
        }
    }

    if (iSocket != -1)
    {
        // set nonblocking operation
        uint32_t uVal = SOFcntl(iSocket, SO_F_GETFL, 0);
        SOFcntl(iSocket, SO_F_SETFL, uVal | SO_O_NONBLOCK);
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
        NetPrintf(("dirtynetrev: warning, trying to close socket 0x%08x that is not in the socket list\n", (intptr_t)pSocket));
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
    \Function    _SocketStartupThread

    \Description
        Socket startup thread, since SOStartup() is blocking

    \Input *pRef    - socket module state

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketStartupThread(void *pRef)
{
    SocketStateT *pState = pRef;
    int32_t iResult;

    // start interface
    NetPrintf(("dirtynetrev: starting socket library\n"));

    iResult = SOStartup();
    if (iResult == 0)
    {
        // call to SOStartup() succeeded
        NetPrintf(("dirtynetrev: socket library startup complete\n"));
        pState->bSoLibAlreadyStarted = FALSE;
        pState->uConnStatus = '~con';
        pState->bSocStarted = TRUE;
    }
    else if (iResult == SO_EALREADY)
    {
        // call to SOStartup() failed - SO lib already started
        NetPrintf(("dirtynetrev: warning -- network already started; SOCleanup() will not be called on disconnect\n"));
        pState->bSoLibAlreadyStarted = TRUE;
        pState->uConnStatus = '~con';
        pState->bSocStarted = TRUE;
    }
    else
    {
        // call to SOStartup() failed - other errors
        pState->iDWCErrCode = NETGetStartupErrorCode(iResult);
        pState->eDWCErrType = -1;
        NetPrintf(("dirtynetrev: SOStartup() failed err=%s dwcerr=%d\n", DirtyErrGetName(iResult), pState->iDWCErrCode));
        pState->uConnStatus = '-sta';
    }

    // quit the thread
    OSExitThread(NULL);
}

/*F********************************************************************************/
/*!
    \Function    _SocketPingThread

    \Description
        Socket ping thread, since ICMPPing() is blocking

    \Input *pRef    - socket module state

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketPingThread(void *pRef)
{
    SocketStateT *pState = pRef;
    SocketPingReqT *pPingReq;
    int32_t iEntry, iEntries, iResult;
    int32_t iSocket = -1;

    // get thread id
    #if DIRTYCODE_LOGGING
    OSThread *pThread = OSGetCurrentThread();
    NetPrintf(("dirtynetrev: ping thread running (thread=0x%08x)\n", pThread));
    #endif

    // loop until done
    while (pState->iPingHead >= 0)
    {
        // if socket lib isn't started yet or socket creation failed, just sleep
        if ((pState->bSocStarted == FALSE) || (iSocket == -2))
        {
            OSSleepMilliseconds(5);
            continue;
        }

        // create an ICMP socket
        if (iSocket == -1)
        {
            if ((iSocket = ICMPSocket(SO_AF_INET)) >= 0)
            {
                NetPrintf(("dirtynetrev: ICMP socket created (id=%d)\n", iSocket));
            }
            else
            {
                NetPrintf(("dirtynetrev: ICMPSocket() failed (err=%s)\n", DirtyErrGetName(iSocket)));
                iSocket = -2;
                continue;
            }
        }

        // acquire ping list critical section
        NetCritEnter(&pState->PingCrit);

        // do we have any pending entries?
        for (iEntry = pState->iPingTail, iEntries = 0; iEntries < SOCKET_MAXPING; iEntry = (iEntry + 1) % SOCKET_MAXPING, iEntries++)
        {
            // check ping slot to see if we have a ping request that is not yet started
            if (pState->aPingRequests[iEntry].uPingActive == 1)
            {
                pPingReq = &pState->aPingRequests[iEntry];
                pPingReq->uPingActive = 2;
                break;
            }
        }

        // release ping list critical section
        NetCritLeave(&pState->PingCrit);

        // if we have a ping request...
        if (pPingReq != NULL)
        {
            SOSockAddr SoAddr;
            struct sockaddr SockAddr;
            uint32_t uStartTick;
            int32_t iResult;

            // create ping address
            SockaddrInit(&SockAddr, AF_INET);
            SockaddrInSetAddr(&SockAddr, pPingReq->uPingAddr);
            SockaddrInSetPort(&SockAddr, pPingReq->uIdentToken);

            // translate to Nintendo socket structure
            _SockAddrToSoAddr(&SoAddr, &SockAddr);

            // time blocking ping request
            uStartTick = NetTick();
            if ((iResult = ICMPPing(iSocket, NULL, 0, &SoAddr, OSSecondsToTicks(SOCKET_PINGTIMEOUT))) >= 0)
            {
                pPingReq->uPingResult = NetTick() - uStartTick;
            }
            else
            {
                NetPrintf(("dirtynetrev: ICMPPing() failed (err=%s)\n", DirtyErrGetName(iResult)));
                pPingReq->uPingResult = 0xffff;
            }
            pPingReq->uPingActive = 3;
            pPingReq = NULL;
        }
        else // wait a while and then check again
        {
            OSSleepMilliseconds(5);
        }
    }

    // close socket
    if ((iSocket >= 0) && (iResult = ICMPClose(iSocket)) < 0)
    {
        NetPrintf(("dirtynetrev: ICMPClose() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }

    // let SocketDestroy know it can proceed
    pState->iPingTail = -1;

    // quit the thread
    NetPrintf(("dirtynetrev: ping thread exits\n"));
    OSExitThread(NULL);
}

/*F********************************************************************************/
/*!
    \Function    SocketLookupDone

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
    SocketStateT *pState = _Socket_pState;
    HostentT **ppFind;

    // acquire lookup list critical section
    NetCritEnter(&pState->LookCrit);

    // find in list
    for (ppFind = &pState->pHost; (*ppFind != pHost) && (*ppFind != NULL); ppFind = (HostentT **)&(*ppFind)->thread)
        ;
    // found it?
    if (*ppFind == pHost)
    {
        // already marked... dispose of it
        if (pHost->sema == 1)
        {
            // remove it from the list
            *ppFind = (HostentT *)(*ppFind)->thread;
            // dispose of memory
            DirtyMemFree(pHost, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
        }
        else
        {
            // mark for subsequent disposal
            pHost->sema = 1;
        }
    }

    // release lookup list critical section
    NetCritLeave(&pState->LookCrit);
}

/*F********************************************************************************/
/*!
    \Function    _SocketLookupThread

    \Description
        Socket lookup thread.

    \Input *pRef    - socket module state

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _SocketLookupThread(void *pRef)
{
    SocketStateT *pState = pRef;
    SOHostEnt *pSocHost;
    HostentT *pHost;
    uint8_t *pAddr;

    // get thread id
    #if DIRTYCODE_LOGGING
    OSThread *pThread = OSGetCurrentThread();
    NetPrintf(("dirtynetrev: lookup thread running (thread=0x%08x)\n", pThread));
    #endif

    // loop until done
    while (pState->iLookDone == 0)
    {
        // acquire lookup list critical section
        NetCritEnter(&pState->LookCrit);

        // do we have any pending entries?
        for (pHost = pState->pHost; (pHost != NULL) && (pHost->done != 0); pHost = (HostentT *)pHost->thread)
            ;

        // release lookup list critical section
        NetCritLeave(&pState->LookCrit);

        // if we have a lookup entry...
        if (pHost != NULL)
        {
            // start lookup
            NetPrintf(("dirtynetrev: looking up name '%s'\n", pHost->name));
            if ((pSocHost = SOGetHostByName(pHost->name)) != NULL)
            {
                // extract ip address
                pAddr = pSocHost->addrList[0];
                pHost->addr = (pAddr[0]<<24)|(pAddr[1]<<16)|(pAddr[2]<<8)|pAddr[3];

                // mark success
                NetPrintf(("dirtynetrev: %s=%a\n", pHost->name, pHost->addr));
                pHost->done = 1;
            }
            else
            {
                // unsuccessful
                NetPrintf(("dirtynetrev: SOGetHostByName() failed (err=%s)\n", DirtyErrGetName(SOGetLastError())));
                pHost->done = -1;
            }

            // free memory if _SocketLookupFree() has already been called
            _SocketLookupFree(pHost);
        }
        else // wait a while and then check again
        {
            OSSleepMilliseconds(5);
        }
    }

    // let SocketDestroy know it can proceed
    pState->iLookDone = -1;

    NetPrintf(("dirtynetrev: lookup thread exits\n"));
    OSExitThread(NULL);
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

    \Version 09/10/04 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _SocketRecvfrom(SocketT *pSocket, char *pBuf, int32_t iLen, struct sockaddr *pFrom, int32_t *pFromLen)
{
    int32_t iResult;

    if (pFrom != NULL)
    {
        // do the receive
        SOSockAddrIn SockAddr;

        memset(&SockAddr, 0, sizeof(SockAddr));
        SockAddr.len = sizeof(SockAddr);
        if ((iResult = SORecvFrom(pSocket->socket, pBuf, iLen, 0, &SockAddr)) > 0)
        {
            // save who we were from
            memcpy(pFrom, &SockAddr, sizeof(*pFrom));
            pFrom->sa_family = SockAddr.family;
            SockaddrInSetMisc(pFrom, NetTick());
             *pFromLen = sizeof(*pFrom);
        }
    }
    else
    {
        iResult = SORecv(pSocket->socket, pBuf, iLen, 0);
    }

    return(iResult);
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
    if (pSocket->type == SOCK_DGRAM)
    {
        int32_t iFromLen = sizeof(pSocket->recvaddr);
        pSocket->recvstat = _SocketRecvfrom(pSocket, pSocket->recvdata, sizeof(pSocket->recvdata), &pSocket->recvaddr, &iFromLen);
    }
    else
    {
        NetPrintf(("dirtynetrev: trying to read from unsupported socket type %d\n", pSocket->type));
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

    \Input *pRef    - socket module state

    \Version 10/21/2004 (jbrookes)
*/
/************************************************************************************F*/
static void _SocketRecvThread(void *pRef)
{
    typedef struct PollListT
    {
        SocketT *aSockets[SOCKET_MAXPOLL];
        struct SOPollFD aPollFds[SOCKET_MAXPOLL];
        int32_t iCount;
    } PollListT;

    PollListT pollList, previousPollList;
    SocketT *pSocket;
    int32_t iListIndex, iResult;
    SocketStateT *pState = (SocketStateT *)pRef;

    // get thread id
    #if DIRTYCODE_LOGGING
    OSThread *pThread = OSGetCurrentThread();
    NetPrintf(("dirtynetrev: async recv thread running (thread=0x%08x)\n", pThread));
    #endif

    // reset contents of pollList
    memset(&pollList, 0, sizeof(pollList));

    // loop until done
    while(pState->iAsyncRecvDone == 0)
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
                        if (previousPollList.aPollFds[iListIndex].revents & SO_POLLIN)
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
                if (pSocket->recvstat <= 0)
                {
                    // add socket to poll list
                    pollList.aSockets[pollList.iCount] = pSocket;
                    pollList.aPollFds[pollList.iCount].fd = pSocket->socket;
                    pollList.aPollFds[pollList.iCount].events = SO_POLLIN;
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
            iResult = SOPoll(pollList.aPollFds, pollList.iCount, OSMillisecondsToTicks(50));

            if (iResult < 0)
            {
                NetPrintf(("dirtynetrev: SOPoll() failed (err=%s)\n", DirtyErrGetName(iResult)));

                // stall for 50ms because experiment shows that next call to SOPoll() may not block
                // internally if a socket is alreay in error.
                OSSleepMilliseconds(50);
            }
        }
        else
        {
            // no sockets, so stall for 50ms
            OSSleepMilliseconds(50);
        }
    }

    // let SocketDestroy know it can proceed
    pState->iAsyncRecvDone = -1;

    // terminate and exit
    NetPrintf(("dirtynetrev: async recv thread exits\n"));
    OSExitThread(NULL);
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
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // error if already started
    if (pState != NULL)
    {
        NetPrintf(("dirtynetrev: SocketCreate() called while module is already active\n"));
        return(-1);
    }

    // print version info
    NetPrintf(("dirtynetrev: DirtySock SDK v%d.%d.%d.%d\n",
        (DIRTYVERS>>24)&0xff, (DIRTYVERS>>16)&0xff,
        (DIRTYVERS>> 8)&0xff, (DIRTYVERS>> 0)&0xff));

    // alloc and init state ref
    if ((pState = DirtyMemAlloc(sizeof(*pState), SOCKET_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("dirtynetrev: unable to allocate module state\n"));
        return(-1);
    }
    memset(pState, 0, sizeof(*pState));
    pState->iMemGroup = iMemGroup;
    pState->pMemGroupUserData = pMemGroupUserData;
    pState->iThreadPrio = iThreadPrio;
    pState->iMaxPacket = SOCKET_MAXUDPRECV;

    // allocate hack buffer
    _Socket_pGetPeerNameAllocHackMem = DirtyMemAlloc(256, SOCKET_MEMID, iMemGroup, pMemGroupUserData);

    // startup network libs
    NetLibCreate(iThreadPrio);

    // add our idle handler
    NetIdleAdd(&_SocketIdle, pState);

    // create high-priority receive thread
    #if SOCKET_ASYNCRECVTHREAD
    OSCreateThread(&pState->AsyncRecvThread, _SocketRecvThread, pState, _Socket_aAsyncRecvStack+SOCKET_ASYNCRECVSTACKSIZE_WORD, SOCKET_ASYNCRECVSTACKSIZE, 10, OS_THREAD_ATTR_DETACH);
    OSResumeThread(&pState->AsyncRecvThread);
    #endif

    // create lookup thread critical section and high-priority socket lookup thread
    NetCritInit(&pState->LookCrit, "socket-lookup");
    OSCreateThread(&pState->LookThread, _SocketLookupThread, pState, _Socket_aLookupStack+SOCKET_LOOKUPSTACKSIZE_WORD, SOCKET_LOOKUPSTACKSIZE, iThreadPrio, OS_THREAD_ATTR_DETACH);
    OSResumeThread(&pState->LookThread);

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
        NetPrintf(("dirtynetrev: SocketDestroy() called while module is not active\n"));
        return(-1);
    }

    NetPrintf(("dirtynetrev: shutting down\n"));

    // tell lookup thread to quit and wait until it terminates
    for (pState->iLookDone = 1; pState->iLookDone == 1; OSSleepMilliseconds(1))
        ;

    // kill lookup thread critical section
    NetCritKill(&pState->LookCrit);

    // kill idle callbacks
    NetIdleDel(&_SocketIdle, pState);

    // let any idle event finish
    NetIdleDone();

    #if SOCKET_ASYNCRECVTHREAD
    // tell async thread to quit and wait until it terminates
    for (pState->iAsyncRecvDone = 1; pState->iAsyncRecvDone == 1; OSSleepMilliseconds(1))
        ;
    #endif

    // close all sockets
    while (pState->pSockList != NULL)
    {
        SocketClose(pState->pSockList);
    }

    // clear the kill list
    _SocketIdle(pState);

    // shut down network libs
    NetLibDestroy();

    // dispose of hack buffer
    DirtyMemFree(_Socket_pGetPeerNameAllocHackMem, SOCKET_MEMID, pState->iMemGroup, pState->pMemGroupUserData);
    _Socket_pGetPeerNameAllocHackMem = NULL;

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

    \Input *pSocket - socket reference

    \Output
        int32_t     - zero

    \Version 06/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketClose(SocketT *pSocket)
{
    int32_t iSocket = pSocket->socket, iResult;

    // stop sending
    SocketShutdown(pSocket, SOCK_NOSEND);

    // dispose of SocketT
    if (_SocketClose(pSocket) < 0)
    {
        return(-1);
    }

    // close soc socket if allocated
    if (iSocket >= 0)
    {
        // close socket
        if ((iResult = SOClose(iSocket)) < 0)
        {
            NetPrintf(("dirtynetrev: SOClose() failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }

    // return
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
    int32_t iProto, iResult;
    uint32_t uProtoSize;
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
    uProtoSize = sizeof(iProto);
    if ((iResult = SOGetSockOpt(uSockRef, SO_SOL_SOCKET, SO_SO_TYPE, &iProto, &uProtoSize)) == 0)
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
        NetPrintf(("dirtynetrev: SOL_GetSockOpt(SO_TYPE) failed (err=%s)\n", DirtyErrGetName(iResult)));
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
        int32_t     - zero

    \Version 09/10/04 (jbrookes)
*/
/********************************************************************************F*/
int32_t SocketShutdown(SocketT *pSocket, int32_t iHow)
{
    int32_t iResult;

    // only shutdown a connected socket
    if (pSocket->type != SOCK_STREAM)
    {
        pSocket->iLastError = SOCKERR_NONE;
        return(pSocket->iLastError);
    }

    // translate how
    if (iHow == SOCK_NOSEND)
    {
        iHow = SO_SHUT_WR;
    }
    else if (iHow == SOCK_NORECV)
    {
        iHow = SO_SHUT_RD;
    }
    else if (iHow == (SOCK_NOSEND|SOCK_NORECV))
    {
        iHow = SO_SHUT_RDWR;
    }

    // do the shutdown
    if (((iResult = SOShutdown(pSocket->socket, iHow)) < 0) && (iResult != SO_ENOTCONN))
    {
        NetPrintf(("dirtynetrev: SOShutdown() failed (err=%s)\n", DirtyErrGetName(iResult)));
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
    SOSockAddr SoAddr;
    int32_t iResult;

    // if a RAW/ICMP socket, ignore bind
    if ((pSocket->type == SOCK_RAW) && (pSocket->proto == IPPROTO_ICMP))
    {
        pSocket->iLastError = SOCKERR_NONE;
        return(pSocket->iLastError);
    }

    // make sure socket is valid
    if (pSocket->socket < 0)
    {
        NetPrintf(("dirtynetrev: attempt to bind invalid socket\n"));
        pSocket->iLastError = SOCKERR_INVALID;
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
                // close physical socket
                NetPrintf(("dirtynetrev: making socket bound to port %d virtual\n", uPort));
                if (pSocket->socket != INVALID_SOCKET)
                {
                    SOShutdown(pSocket->socket, SO_SHUT_RDWR);
                    SOClose(pSocket->socket);
                    pSocket->socket = INVALID_SOCKET;
                }
                pSocket->virtualport = uPort;
                pSocket->virtual = TRUE;
                return(0);
            }
        }
    }

    // set up native sockaddr
    _SockAddrToSoAddr(&SoAddr, pName);

    // $$ hack $$ work around lack of port zero support
    if (SockaddrInGetPort(pName) == 0)
    {
        static uint16_t __uPort = 5000;
        NetPrintf(("dirtynetrev: $$ hack $$ -- manually replacing request to bind port zero with port %d\n", __uPort));
        SockaddrInSetPort((struct sockaddr *)&SoAddr, __uPort++);
    }

    // do the bind
    if ((iResult = SOBind(pSocket->socket, &SoAddr)) < 0)
    {
        NetPrintf(("dirtynetrev: SOBind() failed (err=%s)\n", DirtyErrGetName(iResult)));
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
    SOSockAddr SoAddr;
    int32_t iResult;

    NetPrintf(("dirtynetrev: connecting to %a:%d\n", SockaddrInGetAddr(pName), SockaddrInGetPort(pName)));

    // format address
    _SockAddrToSoAddr(&SoAddr, pName);

    // do the connect
    pSocket->opened = 0;
    if (((iResult = SOConnect(pSocket->socket, &SoAddr)) < 0) && (iResult != SO_EINPROGRESS))
    {
        NetPrintf(("dirtynetrev: SOConnect() failed (err=%s)\n", DirtyErrGetName(iResult)));
    }
    else
    {
        // save remote address, since we can't get that from the socket on Revolution
        memcpy(&pSocket->remote, pName, sizeof(pSocket->remote));
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
    // do the listen
    int32_t iResult;
    if ((iResult = SOListen(pSocket->socket, iBacklog)) < 0)
    {
        NetPrintf(("dirtynetrev: listen() failed (err=%s)\n", DirtyErrGetName(iResult)));
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
    SocketT *pOpen = NULL;
    SOSockAddr SoAddr;
    int32_t iIncoming;

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

    // set up native sockaddr
    _SockAddrToSoAddr(&SoAddr, pAddr);

    // perform inet accept
    if ((iIncoming = SOAccept(pSocket->socket, &SoAddr)) >= 0)
    {
        // Allocate socket structure and install in list
        pOpen = _SocketOpen(iIncoming, pSocket->family, pSocket->type, pSocket->proto, 1);
    }
    else if (iIncoming != SO_EWOULDBLOCK)
    {
        NetPrintf(("dirtynetrev: accept() failed (err=%s)\n", DirtyErrGetName(iIncoming)));
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
        if ((iResult = pState->pSendCallback(pSocket, pSocket->type, pBuf, iLen, pTo, pState->pSendCallref)) > 0)
        {
            return(iResult);
        }
    }

    // raw sockets assume the data includes an IPV4 header
    if ((pSocket->type == SOCK_RAW) && (pSocket->proto == IPPROTO_ICMP))
    {
        SocketPingReqT *pPingReq;

        // validate input
        if (pTo == NULL)
        {
            NetPrintf(("dirtynetrev: cannot sendto raw socket without remote address\n"));
            pSocket->iLastError = SOCKERR_INVALID;
            return(pSocket->iLastError);
        }
        if ((pBuf == NULL) || (iLen < sizeof(EchoHeaderT)))
        {
            NetPrintf(("dirtynetrev: invalid echo header included in sendto data on raw socket\n"));
        }

        // acquire ping list critical section
        NetCritEnter(&pState->PingCrit);

        // do we have a slot free?
        pPingReq = &pState->aPingRequests[pState->iPingHead];
        if ((pPingReq->uPingActive == 0) && (pState->iPingHead != -1))
        {
            EchoHeaderT *pEchoHeader = (EchoHeaderT *)pBuf;

            // allocate ping entry
            pState->iPingHead = (pState->iPingHead + 1) % SOCKET_MAXPING;

            // init ping entry from remote address and EchoHeader sequence and token
            memset(pPingReq, 0, sizeof(pPingReq));
            pPingReq->uPingAddr = SockaddrInGetAddr(pTo);
            pPingReq->uSeqn = (pEchoHeader->icmp.seq[0] << 8) | pEchoHeader->icmp.seq[1];
            pPingReq->uIdentToken = (pEchoHeader->icmp.idn[0] << 8) | pEchoHeader->icmp.idn[1];
            pPingReq->uPingActive = 1;
            iResult = SOCKERR_NONE;
        }
        else
        {
            iResult = SOCKERR_NORSRC;
        }

        // release ping list critical section
        NetCritLeave(&pState->PingCrit);

        pSocket->iLastError = iResult;
        return(pSocket->iLastError);
    }

    // make sure socket ref is valid
    if (pSocket->socket < 0)
    {
        NetPrintf(("dirtynetrev: attempting to send on invalid socket\n"));
        pSocket->iLastError = SOCKERR_INVALID;
        return(pSocket->iLastError);
    }

    // use appropriate version
    if (pTo == NULL)
    {
        if (((iResult = SOSend(pSocket->socket, pBuf, iLen, 0)) < 0) && (iResult != SO_EAGAIN))
        {
            NetPrintf(("dirtynetrev: SOSend() failed (err=%s)\n", DirtyErrGetName(iResult)));
        }
    }
    else
    {
        SOSockAddr SoAddr;

        // set up Sce addr
        _SockAddrToSoAddr(&SoAddr, pTo);

        // do the send
        #if SOCKET_VERBOSE
        NetPrintf(("dirtynetrev: sending %d bytes to %a\n", iLen, SockaddrInGetAddr(pTo)));
        #endif
        if ((iResult = SOSendTo(pSocket->socket, pBuf, iLen, 0, &SoAddr)) < 0)
        {
            NetPrintf(("dirtynetrev: SOSendTo() failed (err=%s)\n", DirtyErrGetName(iResult)));
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

    // receiving on a dgram socket?
    if (pSocket->type == SOCK_DGRAM)
    {
        #if SOCKET_ASYNCRECVTHREAD
        // see if we have any data
        if (((iRecv = pSocket->recvstat) > 0) && (iLen > 0))
        {
            // acquire socket receive critical section
            NetCritEnter(&pSocket->recvcrit);

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

            // release socket receive critical section
            NetCritLeave(&pSocket->recvcrit);
        }
        else if (pSocket->recvstat < 0)
        {
            // clear error
            pSocket->recvstat = 0;
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
        if (((iRecv = _SocketRecvfrom(pSocket, pBuf, iLen, pFrom, pFromLen)) < 0) && (iRecv != SO_EAGAIN))
        {
            NetPrintf(("dirtynetrev: _SocketRecvfrom() failed (err=%s)\n", DirtyErrGetName(iRecv)));
        }
        #endif
    }
    else if ((pSocket->type == SOCK_RAW) && (pSocket->proto == IPPROTO_ICMP))
    {
        // ref ping request
        SocketPingReqT *pPingReq = &pState->aPingRequests[pState->iPingTail];

        // read ping tail and see if request is complete
        if (pPingReq->uPingActive == 3)
        {
            EchoHeaderT EchoHeader;

            // clear echo response header
            memset(&EchoHeader, 0, sizeof(EchoHeader));

            // fill in subset of echo response header fields -- we only fill in what ProtoPing needs
            EchoHeader.ipv4.srcaddr[0] = (uint8_t)(pPingReq->uPingAddr >> 24);
            EchoHeader.ipv4.srcaddr[1] = (uint8_t)(pPingReq->uPingAddr >> 16);
            EchoHeader.ipv4.srcaddr[2] = (uint8_t)(pPingReq->uPingAddr >> 8);
            EchoHeader.ipv4.srcaddr[3] = (uint8_t)(pPingReq->uPingAddr);
            EchoHeader.icmp.type = 0;  // ICMP_ECHOREPLY
            EchoHeader.icmp.seq[0] = (uint8_t)(pPingReq->uSeqn >> 8);
            EchoHeader.icmp.seq[1] = (uint8_t)(pPingReq->uSeqn);
            EchoHeader.icmp.idn[0] = (uint8_t)(pPingReq->uIdentToken >> 8);
            EchoHeader.icmp.idn[1] = (uint8_t)(pPingReq->uIdentToken);

            // set from address
            if (pFrom != NULL)
            {
                SockaddrInit(pFrom, AF_INET);
                SockaddrInSetAddr(pFrom, pPingReq->uPingAddr);
                SockaddrInSetPort(pFrom, 1);
                SockaddrInSetMisc(pFrom, pPingReq->uPingResult);
            }

            // remove ping entry from queue
            NetCritEnter(&pState->PingCrit);
            pState->iPingTail = (pState->iPingTail + 1) % SOCKET_MAXPING;
            pPingReq->uPingActive = 0;
            NetCritLeave(&pState->PingCrit);

            // copy echo response header to output
            if (iLen >= (signed)sizeof(EchoHeader))
            {
                memcpy(pBuf, &EchoHeader, sizeof(EchoHeader));
                iRecv = sizeof(EchoHeader);
            }
            else
            {
                iRecv = SO_ENOBUFS;
            }
        }
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
        if (((iRecv = _SocketRecvfrom(pSocket, pBuf, iLen, pFrom, pFromLen)) < 0) && (iRecv != SO_EAGAIN))
        {
            NetPrintf(("dirtynetrev: _SocketRecvfrom() failed (err=%s)\n", DirtyErrGetName(iRecv)));
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
        int32_t     - size of returned data or error code (negative value)

    \Notes
        iInfo can be one of the following:

        \verbatim
            'addr' - return local address
            'aths' - return athentication server type (1 for DWC_AUTHSERVER_RELEASE, 0 for DWC_AUTHSERVER_DEBUG)
            'bind' - return bind data (if pSocket == NULL, get socket bound to given port)
            'bndu' - return bind data (only with pSocket=NULL, get SOCK_DGRAM socket bound to given port)
            'conn' - return peer data (if pSocket == NULL, update connection/login processing and return status)
            'dwec' - return most recent DWC error code
            'dwet' - return most recent DWC error type
            'ethr' - return interface MAC address (returned in pBuf), 0=success, negative=error
            'macx' - return interface MAC address (returned in pBuf), 0=success, negative=error
            'maxp' - return max packet size
            'serr' - last socket error
            'stat' - return socket connection status (TCP only)
            'tick' - return ticket (authentication token value) acquired from Svl
            'virt' - TRUE if socket is virtual, else FALSE
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

    // socket module options
    if (pSocket == NULL)
    {
        if (iInfo == 'addr')
        {
            if (pState->uLocalAddr == 0)
            {
                pState->uLocalAddr = SOGetHostID();
            }
            return(pState->uLocalAddr);
        }
        if (iInfo == 'aths')
        {
            if (pState->bDWCInitialized)
            {
                if (!strcmp(pState->strAuthEnv, "prod"))
                {
                    return(1);
                }
                else
                {
                    return(0);
                }
            }
            else
            {
                NetPrintf(("dirtynetrev: - WARNING - processing of 'aths' selector in SocketInfo() failed because not currently connected to authentication server!\n"));
                return(-1);
            }
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
        // return connection status
        if (iInfo == 'conn')
        {
            return(_SocketConnUpdate(pState));
        }
        // get most recent DWC error code
        if (iInfo == 'dwec')
        {
            return(pState->iDWCErrCode);
        }
        // get most recent DWC error type
        if (iInfo == 'dwet')
        {
            return(pState->eDWCErrType);
        }
        // return local mac address
        if ((iInfo == 'ethr') || (iInfo == 'macx'))
        {
            if ((pBuf != NULL) && (iLen >= (signed)sizeof(pState->aMacAddr)))
            {
                memcpy(pBuf, &pState->aMacAddr, sizeof(pState->aMacAddr));
                return(0);
            }
            return(-1);
        }
        // return max packet size
        if (iInfo == 'maxp')
        {
            return(pState->iMaxPacket);
        }
        // return ticket info
        if ((iInfo == 'tick') && (pState->uConnStatus == '+onl') && (pBuf != NULL))
        {
            int32_t iTokenLen = strlen(pState->SvlResult.svltoken);
            if (iTokenLen < iLen)
            {
                strnzcpy(pBuf, pState->SvlResult.svltoken, iLen);
                return(iTokenLen);
            }
        }
        return(-1);
    }

    // return local bind data
    if (iInfo == 'bind')
    {
        if (iLen >= (signed)sizeof(pSocket->local))
        {
            struct sockaddr SockAddr;
            SOSockAddr SoAddr;

            if (pSocket->virtual == TRUE)
            {
                SockaddrInit((struct sockaddr *)pBuf, AF_INET);
                SockaddrInSetPort((struct sockaddr *)pBuf, pSocket->virtualport);
            }
            else
            {
                memset(&SoAddr, 0, sizeof(SoAddr));
                SoAddr.len = sizeof(SoAddr);
                SOGetSockName(pSocket->socket, &SoAddr);
                memcpy(&SockAddr, &SoAddr, sizeof(SockAddr));
                SockAddr.sa_family = SoAddr.family;
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
            _SocketGetPeerName(pSocket, pBuf);
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
        // if not a connected socket, return TRUE
        if (pSocket->type != SOCK_STREAM)
        {
            return(1);
        }

        // if not connected, use poll to determine connect
        if (pSocket->opened == 0)
        {
            #if 0
            {
                TCPStat Stat;
                TCPEntry Entries[8];
                int32_t iOptLen, iResult, iValue;
                iOptLen = sizeof(Stat);
                if ((iResult = SOGetInterfaceOpt(NULL, SO_SOL_CONFIG, SO_CONFIG_TCP_STATISTICS, &Stat, &iOptLen)) < 0)
                {
                    NetPrintf(("dirtynetrev: SOGetInterfaceOpt(CONFIG_TCP_STATISTICS) failed (err=%s)\n", DirtyErrGetName(iResult)));
                }
                iOptLen = sizeof(iValue);
                if ((iResult = SOGetInterfaceOpt(NULL, SO_SOL_CONFIG, SO_CONFIG_TCP_NUMBER, &iValue, &iOptLen)) < 0)
                {
                    NetPrintf(("dirtynetrev: SOGetInterfaceOpt(CONFIG_TCP_NUMBER) failed (err=%s)\n", DirtyErrGetName(iResult)));
                }
                if (iValue > 8) iValue = 8;
                iOptLen = sizeof(Entries[0])*iValue;
                memset(Entries, 0, sizeof(Entries));
                if ((iResult = SOGetInterfaceOpt(NULL, SO_SOL_CONFIG, SO_CONFIG_TCP_TABLE, Entries, &iOptLen)) < 0)
                {
                    NetPrintf(("dirtynetrev: SOGetInterfaceOpt(CONFIG_TCP_TABLE) failed (err=%s)\n", DirtyErrGetName(iResult)));
                }
                NetPrintf(("dirtynetrev: stat.activeOpens=%d stat.currEstab=%d stat.attemptFails=%d\n", Stat.activeOpens, Stat.currEstab, Stat.attemptFails));
                for (iResult = 0; iResult < iValue; iResult++)
                {
                    NetPrintf(("dirtynetrev: [%d] entry.state=%d, entry.remote=%a\n", iResult, Entries[iResult].state, SockaddrInGetAddr((struct sockaddr *)&Entries[iResult].remote)));
                    //$$ hack -- use entry values to determine if we are connected or not
                    if ((Entries[iResult].state == TCP_STATE_ESTABLISHED) && (SockaddrInGetAddr((struct sockaddr *)&Entries[iResult].remote) == SockaddrInGetAddr(&pSocket->remote)))
                    {
                        NetPrintf(("dirtynetrev: $$ hack $$ - connection open\n"));
                        pSocket->opened = 1;
                    }
                }
            }
            #endif
            #if 0
            {
                struct SOPollFD PollFd;
                int32_t iResult;
                memset(&PollFd, 0, sizeof(PollFd));
                PollFd.fd = pSocket->socket;
                PollFd.events = SO_POLLOUT|SO_POLLERR;

                if ((iResult = SOPoll(&PollFd, 1, 0)) > 0)
                {
                    // if we got an exception, that means connect failed
                    if (PollFd.revents & SO_POLLERR)
                    {
                        NetPrintf(("dirtynetrev: read exception on connect\n"));
                        pSocket->opened = -1;
                    }

                    // if socket is writable, that means connect succeeded
                    if (PollFd.revents & SO_POLLOUT)
                    {
                        NetPrintf(("dirtynetrev: connection open\n"));
                        pSocket->opened = 1;
                    }
                }
                //NetPrintf(("dirtynetrev: SOPoll()=%d\n", iResult));
            }
            #endif
            #if 1
            // $$ hack $$ - this is a temporary workaround for broken poll above
            {
                if (_SocketGetPeerName(pSocket, NULL) >= 0)
                {
                    NetPrintf(("dirtynetrev: connection open\n"));
                    pSocket->opened = 1;
                }
                else
                {
                    //NetPrintf(("dirtynetrev: waiting for connection\n"));
                }
            }
            #endif
        }

        // if previously connected, make sure connection is still valid
        if (pSocket->opened > 0)
        {
            if (_SocketGetPeerName(pSocket, NULL) < 0)
            {
                NetPrintf(("dirtynetrev: connection closed\n"));
                pSocket->opened = -1;
            }
        }

        // return connect status
        return(pSocket->opened > 0);
    }

    // unhandled option?
    NetPrintf(("dirtynetrev: unhandled SocketInfo() option '%c%c%c%c'\n",
        (uint8_t)(iInfo >> 24),
        (uint8_t)(iInfo >> 16),
        (uint8_t)(iInfo >>  8),
        (uint8_t)(iInfo >>  0)));
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
        int32_t     - zero

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
            'conn' - initiate connection to access point (pData3=auth environment or NULL)
            'disc' - bring down network stack
            'dwab' - initiate DWC connection abort operation
            'dwce' - clear most recent DWC error
            'init' - init socket library (iData1=game code, pData2=auth code)
            'loss' - simulate packet loss (debug only)
            'maxp' - set max udp packet size
            'nbio' - set nonblocking/blocking mode (TCP only, iData1=TRUE (nonblocking) or FALSE (blocking))
            'push' - push data into given socket receive buffer (iData1=size, pData2=data ptr, pData3=sockaddr ptr)
            'sdcb' - set send callback (pData2=callback, pData3=callref)
            'shut' - shut down socket library
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
        OSThread *pCurrThread = OSGetCurrentThread();
        int32_t iCurrThreadPriority = OSGetThreadPriority(pCurrThread);

        // create socket startup thread with 1 unit of priority higher than parent thread to avoid stalling within SOStartup() invoked from newly created thread
        pState->uConnStatus = '~sta';
        OSCreateThread(&pState->StartThread, _SocketStartupThread, pState, _Socket_aStartupStack+SOCKET_STARTUPSTACKSIZE_WORD, SOCKET_STARTUPSTACKSIZE, (iCurrThreadPriority - 1), OS_THREAD_ATTR_DETACH);
        OSResumeThread(&pState->StartThread);

        // create ping thread critical section and high-priority ping thread
        NetCritInit(&pState->PingCrit, "socket-ping");
        OSCreateThread(&pState->PingThread, _SocketPingThread, pState, _Socket_aPingStack+SOCKET_PINGSTACKSIZE_WORD, SOCKET_PINGSTACKSIZE, pState->iThreadPrio, OS_THREAD_ATTR_DETACH);
        OSResumeThread(&pState->PingThread);

        // save auth environment if specified
        if (pData3 != NULL)
        {
            strnzcpy(pState->strAuthEnv, pData3, sizeof(pState->strAuthEnv));
        }
        return(0);
    }
    // initiate dwc abort
    if (iOption == 'dwab')
    {
        if (pState->uConnStatus == '~log')
        {
            NetPrintf(("dirtynetrev: initiating DWC abort\n"));
            DWC_NasLoginAbort();
        }
        else if (pState->uConnStatus == '~ath')
        {
            NetPrintf(("dirtynetrev: initiating Svl abort\n"));
            DWC_SvlAbort();
        }
        else
        {
            NetPrintf(("dirtynetrev: cannot initiate abort when not in login or auth state\n"));
        }
        return(0);
    }
    // clear most recent DWC error
    if (iOption == 'dwce')
    {
        NetPrintf(("dirtynetrev: clearing most recent DWC error\n"));
        DWC_ClearError();
        pState->iDWCErrCode = 0;
        pState->eDWCErrType = DWC_ETYPE_NO_ERROR;
        return(0);
    }
    // bring down interface
    if (iOption == 'disc')
    {
        NetPrintf(("dirtynetrev: disconnecting\n"));

        // tell ping thread to quit and wait until it is done
        for (pState->iPingHead = -1; pState->iPingTail > 0; OSSleepMilliseconds(1))
            ;
        // kill ping thread critical section
        NetCritKill(&pState->PingCrit);

        // shut down dwc
        if (pState->bDWCInitialized)
        {
            DWC_Shutdown();
            pState->bDWCInitialized = FALSE;
        }

        // shut down socket lib
        if (pState->bSocStarted)
        {
            // shut down network lib
            if (pState->bSoLibAlreadyStarted)
            {
                NetPrintf(("dirtynetrev: bypassing SOCleanup() because SOStartup() was also bypassed in init code\n"));
            }
            else if ((iResult = SOCleanup()) < 0)
            {
                NetPrintf(("dirtynetrev: SOCleanup() failed err=%s\n", DirtyErrGetName(iResult)));
            }
            pState->bSocStarted = FALSE;
        }
        pState->uConnStatus = '-dsc';
        return(0);
    }
    // perform idle processing
    if (iOption == 'idle')
    {
        int32_t iLinkStatus, iResult;
        #if DIRTYCODE_LOGGING
        static char *_NCDLinkStatusNames[] =
        {
            "NCD_LINKSTATUS_UNDEFINED",
            "NCD_LINKSTATUS_WORKING",           // automatic state transition underway
            "NCD_LINKSTATUS_NONE",              // the link target device is not configured
            "NCD_LINKSTATUS_WIRED",             // link with the targetted wired device
            "NCD_LINKSTATUS_WIRELESS_DOWN",     // link not active for the targetted wireless device
            "NCD_LINKSTATUS_WIRELESS_UP",       // link established for the targetted wireless device
        };
        #endif
        // get link status
        if ((iResult = NCDGetLinkStatus()) >= 0)
        {
            iLinkStatus = iResult;
        }
        else
        {
            NetPrintf(("dirtynetrev: NCDGetLinkStatus() failed (err=%d)\n"));
            iLinkStatus = 0;
        }
        // compare to previous status
        if (pState->iLinkStatus != iLinkStatus)
        {
            NetPrintf(("dirtynetrev: %s->%s\n", _NCDLinkStatusNames[pState->iLinkStatus], _NCDLinkStatusNames[iLinkStatus]));
            // if wireless goes down, transition to disconnected state, but *not* when we are starting up (we let SOStartup() handle that)
            if ((pState->iLinkStatus == NCD_LINKSTATUS_WIRELESS_UP) && (iLinkStatus == NCD_LINKSTATUS_WIRELESS_DOWN) && (pState->uConnStatus != '~sta'))
            {
                NetPrintf(("dirtynetrev: going to disconnected status because wireless link is down\n"));
                pState->uConnStatus = '-dsc';
            }
            pState->iLinkStatus = iLinkStatus;
        }
        // return NCDGetLinkStatus() result
        return(iResult);
    }
    // init socket library
    if (iOption == 'init')
    {
        SOLibraryConfig Config;
        int32_t iResult;

        // start up socket lib (required before DWC_Init())
        memset(&Config, 0, sizeof(Config));
        Config.alloc = _SocketAlloc;
        Config.free = _SocketFree;

        // init SO library
        iResult = SOInit(&Config);
        if (iResult == 0)
        {
            // call to SOInit() succeeded
            pState->bSoLibAlreadyInitialized = FALSE;
        }
        else if (iResult == SO_EALREADY)
        {
            // call to SOInit() failed - SO lib already initialized
            NetPrintf(("dirtynetrev: warning -- network already initialized; SOFinish() will not be called on disconnect\n"));
            pState->bSoLibAlreadyInitialized = TRUE;
            iResult = 0; // Force caller to behave as if SOInit() had succeeded
        }
        else
        {
            // call to SOInit() failed - other errors
            NetPrintf(("dirtynetrev: SOInit() failed err=%s\n", DirtyErrGetName(iResult)));
        }

        // save game id
        pState->uGameId = iData1;
        // save auth code
        strnzcpy(pState->strAuthCode, pData2, sizeof(pState->strAuthCode));
        return(iResult);
    }
    // set up packet loss simulation
    #if DIRTYCODE_DEBUG
    if (iOption == 'loss')
    {
        pState->uPacketLoss = (unsigned)iData1;
        NetPrintf(("dirtynetrev: packet loss simulation %s (param=0x%08x)\n", pState->uPacketLoss ? "enabled" : "disabled", pState->uPacketLoss));
        return(0);
    }
    #endif
    // set max udp packet size
    if (iOption == 'maxp')
    {
        NetPrintf(("dirtynetrev: setting max udp packet size to %d\n", iData1));
        pState->iMaxPacket = iData1;
        return(0);
    }
    // if a stream socket, set nonblocking/blocking mode
    if ((iOption == 'nbio') && (pSocket != NULL) && (pSocket->type == SOCK_STREAM))
    {
        uint32_t uVal = SOFcntl(pSocket->socket, SO_F_GETFL, 0);
        uVal = iData1 ? (uVal | SO_O_NONBLOCK) : (uVal & ~SO_O_NONBLOCK);
        iResult = SOFcntl(pSocket->socket, SO_F_SETFL, uVal);
        pSocket->iLastError = _XlatError(iResult);
        NetPrintf(("dirtynetrev: setting socket:0x%x to %s mode %s (LastError=%d).\n", pSocket, iData1 ? "nonblocking" : "blocking", iResult ? "failed" : "succeeded", pSocket->iLastError));
        return(pSocket->iLastError);
    }
    #if SOCKET_ASYNCRECVTHREAD
    // push data into receive buffer
    if (iOption == 'push')
    {
        if (pSocket == NULL)
        {
            NetPrintf(("dirtynetrev: warning - call to SocketControl('push') ignored because pSocket is NULL\n"));
        }
        else
        {
            // acquire socket critical section
            NetCritEnter(&pSocket->recvcrit);

            if (pSocket->recvstat > 0)
            {
                NetPrintf(("dirtynetrev: warning - overwriting packet data with SocketControl('push')\n"));
            }

            // save the size and copy the data
            pSocket->recvstat = iData1;
            memcpy(pSocket->recvdata, pData2, pSocket->recvstat);

            // save the address
            memcpy(&pSocket->recvaddr, pData3, sizeof(pSocket->recvaddr));
            // save receive timestamp
            SockaddrInSetMisc(&pSocket->recvaddr, NetTick());

            // release socket critical section
            NetCritLeave(&pSocket->recvcrit);

            // see if we should issue callback
            if ((pSocket->callback != NULL) && (pSocket->callmask & CALLB_RECV))
            {
                pSocket->callback(pSocket, 0, pSocket->callref);
            }
            return(0);
        }
    }
    #endif
    // set global send callback
    if (iOption == 'sdcb')
    {
        pState->pSendCallback = (SocketSendCallbackT *)pData2;
        pState->pSendCallref = pData3;
        return(0);
    }
    // shut down socket library
    if (iOption == 'shut')
    {
        // Do not call SOFinish() if SOInit() was not used during init phase (iOption == 'init')
        if (pState->bSoLibAlreadyInitialized == FALSE)
        {
            // shut down network lib
            if ((iResult = SOFinish()) < 0)
            {
                NetPrintf(("dirtynetrev: SOFinish() failed err=%s\n", DirtyErrGetName(iResult)));
            }
        }
        return(iResult);
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
    return(SocketInfo(NULL, 'addr', 0, NULL, 0));
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
    HostentT *pHost, **ppAdd;
    int32_t iAddr;

    NetPrintf(("dirtynetrev: looking up address for host '%s'\n", pText));

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

    // check for dot notation
    if ((iAddr = SocketInTextGetAddr(pText)) != 0)
    {
        // we've got a dot-notation address
        pHost->addr = iAddr;
        pHost->done = 1;
        // mark as "thread complete" so SocketLookupFree() will release memory
        pHost->sema = 1;
    }

    // copy over the target address
    strnzcpy(pHost->name, pText, sizeof(pHost->name));

    // acquire lookup list critical section
    NetCritEnter(&pState->LookCrit);

    // add to tail of list
    for (ppAdd = &pState->pHost; *ppAdd != NULL; ppAdd = (HostentT *)&(*ppAdd)->thread)
        ;
    *ppAdd = pHost;

    // release lookup list critical section
    NetCritLeave(&pState->LookCrit);

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
        int32_t     - zero=success, negative=error

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
            SockaddrInSetAddr(host, SocketInfo(NULL, 'addr', 0, NULL, 0));
            return(0);
        }
    }

    // unsupported family
    memset(host, 0, hostlen);
    return(-3);
}


