/*H*************************************************************************************************/
/*!

    \File    commtcp.c

    \Description
        This is a simple packet based TCP transport class. It is optimized
        for use in real-time environments (expending additional bandwidth for
        the sake of better latency). It uses the same packet based interface
        as the other Comm modules. Packet overhead is very low at only 2
        extra bytes per packet beyond what TCP adds.

    \Notes
        Unfortunately, because of delayed ack handling in many TCP implementations,
        performance of this module is far below that of the UDP version. Also, it
        is much more sensitive to data loss. While this is kept around for historical
        reasons, it should not be used for new applications.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2002.  ALL RIGHTS RESERVED.

    \Version    0.5        02/19/99 (GWS) First Version
    \Version    1.0        02/25/99 (GWS) Alpha release
    \Version    1.1        07/27/99 (GWS) Initial release
    \Version    2.0        10/28/99 (GWS) Revised to use winsock 1.1/2.0
    \Version    2.1        12/28/99 (GWS) Removed winsock 1.1 support
    \Version    2.2        07/10/00 (GWS) Reverted to winsock 1.1/2.0 version
    \Version    3.0        01/06/03 (GWS) Converted to dirtysock

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <string.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "commall.h"
#include "commtcp.h"

/*** Defines ***************************************************************************/

#define COMMTCP_VERBOSE (COMM_PRINT && FALSE)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! raw protocol packet format
typedef struct
{
    //! packet header which is not sent/received
    //! (this data is used internally)
    struct
    {
        uint32_t when;     //!< tick when a packet was received
    } head;
    //! packet body which is sent/received
    struct
    {
        int16_t len;              //!< variable data len (-1=none) (used int32_t for alignment)
        char data[2048];        //!< user data
    } body;
} RawTCPPacketT;


//! private module storage
struct CommTCPRef
{
    //! common header
    CommRef common;
    //! linked list of all instances
    CommTCPRef *link;
    //! comm socket
    SocketT *socket;
    //! connect address
    struct sockaddr peeraddr;

    //! port state
    enum
    {
        DEAD, IDLE, CONN, LIST, OPEN, CLOSE
    } state;

    //! identifier to keep from getting spoofed
    uint32_t connident;

    //! receive offset into current record
    int32_t rcvoff;
    //! width of receive records (same as width of send)
    int32_t rcvwid;
    //! length of receive buffer (multiple of rcvwid)
    int32_t rcvlen;
    //! fifo input offset
    int32_t rcvinp;
    //! fifo output offset
    int32_t rcvout;
    //! pointer to buffer storage
    char *rcvbuf;
    //! next packet expected (sequence number)
    uint32_t rcvseq;

    //! send offset into current record
    int32_t sndoff;
    //! width of send record (same as width of receive)
    int32_t sndwid;
    //! length of send buffer (multiple of sndwid)
    int32_t sndlen;
    //! fifo input offset
    int32_t sndinp;
    //! fifo output offset
    int32_t sndout;
    //! pointer to buffer storage
    char *sndbuf;
    //! next packet to send (sequence number)
    uint32_t sndseq;

    //! tick at which last packet was sent
    uint32_t sendtick;
    //! tick at which last packet was received
    uint32_t recvtick;

    //! allow for dns lookup
    uint32_t dnsid;
    char dnsquery[256];
    char *dnsbuf;
    int32_t dnslen;
    char dnsdiv;

    //! semaphore to synchronize thread access
    NetCritT crit;
    //! callback synchronization
    volatile int32_t callback;
    //! indcate pending event
    int32_t gotevent;
    //! callback routine pointer
    void (*callproc)(CommRef *ref, int32_t event);
};

/*** Function Prototypes ***************************************************************/

static int32_t _CommTCPEvent(SocketT *sock, int32_t flags, void *_ref);

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    _CommTCPSetAddrInfo

    \Description
        Sets peer/host addr/port info in common ref.

    \Input *ref     - reference pointer

    \Output
        None.

    \Version    1.0        04/16/04 (JLB) First Version
*/
/*************************************************************************************************F*/
static void _CommTCPSetAddrInfo(CommTCPRef *ref)
{
    struct sockaddr SockAddr;
    
    // save peer addr/port info in common ref
    SocketInfo(ref->socket, 'peer', 0, &SockAddr, sizeof(SockAddr));
    ref->common.peerip = SockaddrInGetAddr(&SockAddr);
    ref->common.hostport = SockaddrInGetPort(&SockAddr);

    // save host addr/port info in common ref    
    SocketInfo(ref->socket, 'bind', 0, &SockAddr, sizeof(SockAddr));
    ref->common.hostip = SockaddrInGetAddr(&SockAddr);
    ref->common.hostport = SockaddrInGetPort(&SockAddr);
    
    // debug output
    NetPrintf(("commtcp: peer=0x%08x:%d, host=0x%08x:%d\n",
        ref->common.peerip,
        ref->common.peerport,
        ref->common.hostip,
        ref->common.hostport));
}

/*F*************************************************************************************************/
/*!
    \Function    _CommTCPSetSocket

    \Description
        Sets socket in ref and socketref in common portion of ref.

    \Input *pRef    - reference pointer
    \Input *pSocket - socket to set

    \Output
        None.

    \Version    1.0        08/24/04 (JLB) First Version
*/
/*************************************************************************************************F*/
static void _CommTCPSetSocket(CommTCPRef *pRef, SocketT *pSocket)
{
    pRef->socket = pSocket;
    pRef->common.sockptr = pSocket;
}

/*F*************************************************************************************************/
/*!
    \Function    _CommTCPResetTransfer

    \Description
        Reset the transfer state

    \Input *ref    - reference pointer

    \Output
        None.

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static void _CommTCPResetTransfer(CommTCPRef *ref)
{
    // reset the send queue
    ref->sndinp = 0;
    ref->sndout = 0;
    ref->sndoff = -1;

    // reset the receive queue
    ref->rcvinp = 0;
    ref->rcvout = 0;
    ref->rcvoff = 0;

    // make sendtick really old (in protocol terms)
    ref->sendtick = NetTick()-5000;
    return;
}

/*F*************************************************************************************************/
/*!
    \Function    _CommTCPClose

    \Description
        Close the connection

    \Input *ref     - reference pointer

    \Output
        int32_t         - negative=error, zero=ok

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t _CommTCPClose(CommTCPRef *ref)
{
    // see if we are even connected
    if (ref->state != OPEN)
    {
        return(0);
    }

    // set to disconnect state
    ref->connident = 0;
    ref->state = CLOSE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommTCPEventExclusive

    \Description
        Take care of data processing

    \Input *ref     - module ref
    \Input tick     - current tick

    \Output
        int32_t         - number of packets received

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t _CommTCPEventExclusive(CommTCPRef *ref, uint32_t tick)
{
    int32_t len;
    int32_t count = 0;
    RawTCPPacketT *packet;
    char *buffer;

    // see if we need a connect
    if (ref->state == CONN)
    {
        if (SocketInfo(ref->socket, 'stat', 0, NULL, 0) > 0)
        {
            // mark as open
            ref->state = OPEN;
            // set peer/host addr/port info
            _CommTCPSetAddrInfo(ref);

            #if COMMTCP_VERBOSE
            NetPrintf(("commtcp: connection established\n"));
            #endif
        }
    }
    // see if we need an accept
    if (ref->state == LIST)
    {
        struct sockaddr addr;
        int32_t addrlen = sizeof(addr);
        SocketT *sock2 = NULL;

        // accept the connection
        sock2 = SocketAccept(ref->socket, &addr, &addrlen);
        if (sock2 != NULL)
        {
            SocketClose(ref->socket);
            // save new socket
            _CommTCPSetSocket(ref, sock2);
            // setup for callbacks
            SocketCallback(ref->socket, CALLB_RECV, 100, ref, &_CommTCPEvent);
            // mark as open
            ref->state = OPEN;
            // set peer/host addr/port info
            _CommTCPSetAddrInfo(ref);

            #if COMMTCP_VERBOSE
            NetPrintf(("commtcp: connection established\n"));
            #endif
        }
    }

    // grab all the pending data
    while (ref->state == OPEN)
    {
        // see if any room in input buffer
        if ((ref->rcvinp+ref->rcvwid)%ref->rcvlen == ref->rcvout)
        {
            break;
        }
        // get pointer to input buffer
        packet = (RawTCPPacketT *) &ref->rcvbuf[ref->rcvinp];
        buffer = (char *) &packet->body;
        buffer += ref->rcvoff;

        // figure size of packet framing
        len = sizeof(packet->body)-sizeof(packet->body.data);
        // if framing complete, add in length of packet (else not known)
        if (ref->rcvoff >= len)
        {
            len += packet->body.len;
        }
        // subtract out number of bytes already received
        len -= ref->rcvoff;

        // attempt to complete the packet
        len = SocketRecv(ref->socket, buffer, len, 0);
        // stop when we run out of data
        if (len == 0)
        {
            break;
        }
        // see if we got an error
        if (len < 0)
        {
            ref->state = CLOSE;
            break;
        }

        // update the receive tick
        ref->recvtick = NetTick();
        // timestamp at first byte
        if (ref->rcvoff == 0)
            packet->head.when = ref->recvtick;
        // add data to buffer
        count += len;
        ref->rcvoff += len;
        // see if packet is complete
        len = sizeof(packet->body)-sizeof(packet->body.data);
        if (ref->rcvoff < len)
        {
            continue;
        }
        len += packet->body.len;
        if (ref->rcvoff < len)
        {
            continue;
        }

        // reset the data offset
        ref->rcvoff = 0;
        // dont save if its just a keepalive
        if (packet->body.len == 0)
        {
            continue;
        }

        // stop receive access
        ref->callback += 1;
        // add the packet to the incoming data buffer
        ref->rcvinp = (ref->rcvinp+ref->rcvwid) % ref->rcvlen;
        if (ref->common.RecvCallback != NULL)
        {
            ref->common.RecvCallback((CommRef *)ref, packet->body.data, packet->body.len, packet->head.when);
        }
        // done with callback
        ref->callback -= 1;
        // mark as having event
        ref->gotevent |= 1;
    }

    // see if we should send a keepalive
    if ((ref->state == OPEN) && (ref->sndoff < 0) && (ref->sndinp == ref->sndout))
    {
        // see if time has elapsed
        if (NetTick() > ref->sendtick+50)
        {
            // queue up a keepalive packet
            packet = (RawTCPPacketT *) &ref->sndbuf[ref->sndinp];
            packet->head.when = NetTick();
            packet->body.len = 0;
            ref->sndinp = (ref->sndinp+ref->sndwid)%ref->sndlen;
        }
    }

    // send all queued data
    while (ref->state == OPEN)
    {
        // see if we need to grab a new packet
        if (ref->sndoff < 0)
        {
            // see if there is a packet to grab
            if (ref->sndinp == ref->sndout)
            {
                break;
            }
            // reset the offset
            ref->sndoff = 0;
            // perform callback if requested
            if (ref->common.SendCallback != NULL)
            {
                packet = (RawTCPPacketT *) &ref->sndbuf[ref->sndout];
                ref->common.SendCallback((CommRef *)ref, packet->body.data, packet->body.len, 0);
            }
        }
        // calc the data pointer
        packet = (RawTCPPacketT *) &ref->sndbuf[ref->sndout];
        buffer = (char *) &packet->body;
        buffer += ref->sndoff;
        // figure out amount to send
        len = sizeof(packet->body)-sizeof(packet->body.data);   // packet framing
        len += packet->body.len;                                // variable data
        len -= ref->sndoff;                                 // already sent
        // send some data
        len = SocketSend(ref->socket, buffer, len, 0);
        // stop if we did not send anything
        if (len <= 0)
        {
            break;
        }
        // update the send tick
        ref->sendtick = NetTick();
        // update the buffer offset
        ref->sndoff += len;
        // see if packet was completely sent
        if ((unsigned)ref->sndoff == packet->body.len+sizeof(packet->body)-sizeof(packet->body.data))
        {
            ref->sndout = (ref->sndout+ref->sndwid)%ref->sndlen;
            ref->sndoff = -1;
        }
    }

    // do callback if needed
    if ((ref->callback == 0) && (ref->gotevent != 0))
    {
        // limit callback access
        ref->callback += 1;
        // notify user
        if (ref->callproc != NULL)
        {
            ref->callproc((CommRef *)ref, ref->gotevent);
        }
        // limit callback access
        ref->callback -= 1;
        // done with event
        ref->gotevent = 0;
    }

    // return byte count
    return(count);
}


/*F*************************************************************************************************/
/*!
    \Function    _CommTCPEvent

    \Description
        Main event function which provides exclusive access

    \Input *sock    - socket ref
    \Input flags    - flags (ignored)
    \Input *_ref    - CommTCP module ref

    \Output
        int32_t     -

    \Version    1.0        01/06/03 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t _CommTCPEvent(SocketT *sock, int32_t flags, void *_ref)
{
    CommTCPRef *ref = _ref;

    // see if we have exclusive access
    if (NetCritTry(&ref->crit))
    {
        uint32_t tick = NetTick();

        // process data
        _CommTCPEventExclusive(ref, tick);

        // free access
        NetCritLeave(&ref->crit);
    }
    return(0);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    CommTCPConstruct

    \Description
        Construct the class

    \Input maxwid   - max record width
    \Input maxinp   - input packet buffer size
    \Input maxout   - output packet buffer size

    \Output
        CommTCPRef  - reference pointer

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
CommTCPRef *CommTCPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout)
{
    CommTCPRef *ref;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate class storage
    ref = DirtyMemAlloc(sizeof(*ref), COMMTCP_MEMID, iMemGroup, pMemGroupUserData);
    if (ref == NULL)
    {
        return(NULL);
    }
    memset(ref, 0, sizeof(*ref));
    ref->common.memgroup = iMemGroup;
    ref->common.memgrpusrdata = pMemGroupUserData;

    // initialize the callback routines
    ref->common.Construct = (CommAllConstructT *)CommTCPConstruct;
    ref->common.Destroy = (CommAllDestroyT *)CommTCPDestroy;
    ref->common.Resolve = (CommAllResolveT *)CommTCPResolve;
    ref->common.Unresolve = (CommAllUnresolveT *)CommTCPUnresolve;
    ref->common.Listen = (CommAllListenT *)CommTCPListen;
    ref->common.Unlisten = (CommAllUnlistenT *)CommTCPUnlisten;
    ref->common.Connect = (CommAllConnectT *)CommTCPConnect;
    ref->common.Unconnect = (CommAllUnconnectT *)CommTCPUnconnect;
    ref->common.Callback = (CommAllCallbackT *)CommTCPCallback;
    ref->common.Status = (CommAllStatusT *)CommTCPStatus;
    ref->common.Tick = (CommAllTickT *)CommTCPTick;
    ref->common.Send = (CommAllSendT *)CommTCPSend;
    ref->common.Peek = (CommAllPeekT *)CommTCPPeek;
    ref->common.Recv = (CommAllRecvT *)CommTCPRecv;

    // remember max packet width
    ref->common.maxwid = maxwid;
    ref->common.maxinp = maxinp;
    ref->common.maxout = maxout;

    // reset access to shared resources
    NetCritInit(&ref->crit, "commtcp");

    // allocate the buffers
    ref->rcvwid = sizeof(RawTCPPacketT)-sizeof(((RawTCPPacketT *)0)->body.data)+maxwid;
    ref->rcvwid = (ref->rcvwid+3) & 0x7ffc;
    ref->rcvlen = ref->rcvwid * maxinp;
    ref->rcvbuf = DirtyMemAlloc(ref->rcvlen, COMMTCP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    ref->sndwid = sizeof(RawTCPPacketT)-sizeof(((RawTCPPacketT *)0)->body.data)+maxwid;
    ref->sndwid = (ref->sndwid+3) & 0x7ffc;
    ref->sndlen = ref->sndwid * maxout;
    ref->sndbuf = DirtyMemAlloc(ref->sndlen, COMMTCP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);

    // reset the socket
    _CommTCPSetSocket(ref, NULL);

    // reset the state
    ref->state = IDLE;
    ref->connident = 0;

    // return the reference
    return(ref);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPDestroy

    \Description
        Destruct the class

    \Input *ref     - reference pointer

    \Output
        None.

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
void CommTCPDestroy(CommTCPRef *ref)
{
    // if port is open, close it
    if (ref->state == OPEN)
    {
        _CommTCPClose(ref);
    }

    // kill the socket
    if (ref->socket != NULL)
    {
        SocketClose(ref->socket);
        _CommTCPSetSocket(ref, NULL);
    }

    // get rid of sockets critical section
    NetCritKill(&ref->crit);

    // release resources
    DirtyMemFree(ref->rcvbuf, COMMTCP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    DirtyMemFree(ref->sndbuf, COMMTCP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    DirtyMemFree(ref, COMMTCP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPCallback

    \Description
        Set upper layer callback

    \Input *ref         - reference pointer
    \Input *callback    - socket generating callback

    \Output
        None.

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
void CommTCPCallback(CommTCPRef *ref, void (*callback)(CommRef *ref, int32_t event))
{
    ref->callproc = callback;
    ref->gotevent |= 2;
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPResolve

    \Description
        Resolve an address

    \Input *ref     - endpoint
    \Input *addr    - resolve address
    \Input *buf     - target buffer
    \Input len      - target length (min 64 bytes)
    \Input div      - divider char

    \Output
        int32_t         - <0=error, 0=complete (COMM_NOERROR), >0=in progress (COMM_PENDING)

    \Notes
        Target list is always double null terminated allowing null
        to be used as the divider character if desired. when COMM_PENDING
        is returned, target buffer is set to "~" until completion

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPResolve(CommTCPRef *ref, const char *addr, char *buf, int32_t len, char div)
{
    NetPrintf(("commtcp: resolve functionality not supported\n"));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPUnresolve

    \Description
        Stop the resolver

    \Input *ref     - endpoint ref

    \Output
        None.

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
void CommTCPUnresolve(CommTCPRef *ref)
{
    return;
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPUnlisten

    \Description
        Stop listening.

    \Input *ref     - reference pointer

    \Output
        int32_t         - negative=error, zero=ok

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPUnlisten(CommTCPRef *ref)
{
    // never close listening socket (it is shared)
    if (ref->state == LIST)
    {
        _CommTCPSetSocket(ref, NULL);
    }

    // get rid of socket if presernt
    if (ref->socket != NULL)
    {
        SocketClose(ref->socket);
        _CommTCPSetSocket(ref, NULL);
    }

    // return to idle mode
    ref->state = IDLE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPUnconnect

    \Description
        Terminate a connection

    \Input *ref     - reference pointer

    \Output
        int32_t         - negative=error, zero=ok

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPUnconnect(CommTCPRef *ref)
{
    // never close listening socket (it is shared)
    if (ref->state == LIST)
    {
        _CommTCPSetSocket(ref, NULL);
    }

    // get rid of socket if presernt
    if (ref->socket != NULL)
    {
        SocketClose(ref->socket);
        _CommTCPSetSocket(ref, NULL);
    }

    // return to idle mode
    ref->state = IDLE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPStatus

    \Description
        Return current stream status

    \Input *ref     - reference pointer

    \Output
        int32_t         - CONNECTING, OFFLINE, ONLINE or FAILURE

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPStatus(CommTCPRef *ref)
{
    // return state
    if ((ref->state == CONN) || (ref->state == LIST))
    {
        return(COMM_CONNECTING);
    }
    if ((ref->state == IDLE) || (ref->state == CLOSE))
    {
        return(COMM_OFFLINE);
    }
    if (ref->state == OPEN)
    {
        return(COMM_ONLINE);
    }
    return(COMM_FAILURE);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPTick

    \Description
        Return current tick

    \Input *ref         - reference pointer

    \Output
        uint32_t    - elapsed milliseconds

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
uint32_t CommTCPTick(CommTCPRef *ref)
{
    return(NetTick());
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPSend

    \Description
        Send a packet

    \Input *ref     - reference pointer
    \Input *buffer  - pointer to data
    \Input length   - length of data
    \Input flags    - COMM_FLAGS_*

    \Output
        int32_t         - negative=error, zero=buffer full (temp fail), positive=queue position (ok)

    \Notes
        Zero length packets may not be sent (they are used for buffer query)

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPSend(CommTCPRef *ref, const void *buffer, int32_t length, uint32_t flags)
{
    int32_t pos;
    RawTCPPacketT *packet;
    
    // make sure port is open
    if (ref->state != OPEN)
    {
        return(COMM_BADSTATE);
    }

    // see if output buffer full
    if ((ref->sndinp+ref->sndwid)%ref->sndlen == ref->sndout)
    {
        NetPrintf(("TCP: output buffer full\n"));
        return(0);
    }

    // return error for oversized packets
    if (length > (signed)(ref->sndwid-(sizeof(RawTCPPacketT)-sizeof((RawTCPPacketT *)0)->body.data)))
    {
        NetPrintf(("CommTCP: Oversized packet send (%d bytes)\n", length));
        return(COMM_MINBUFFER);
    }

    // zero sized packet cannot be sent (they are used for acks)
    // instead, treat them as successful which means the queue
    // position is returned
    if (length == 0)
    {
        pos = (((ref->sndinp+ref->sndlen)-ref->sndout)%ref->sndlen)/ref->sndwid;
        return(pos+1);
    }

    #if COMMTCP_VERBOSE
    NetPrintf(("commtcp: sending %d bytes\n",length));
    #endif

    // add packet to output buffer
    packet = (RawTCPPacketT *) &(ref->sndbuf[ref->sndinp]);
    packet->body.len = (int16_t)length;

    // copy the packet to the buffer
    memcpy(packet->body.data, buffer, length);
    // set the send time
    packet->head.when = NetTick();

    // add the packet to the queue
    ref->sndinp = (ref->sndinp+ref->sndwid) % ref->sndlen;

    // try and send immediately
    _CommTCPEvent(ref->socket, 0, ref);

    // return buffer depth
    pos = (((ref->sndinp+ref->sndlen)-ref->sndout)%ref->sndlen)/ref->sndwid;
    return((pos > 0) ? pos : 1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPPeek

    \Description
        Peek at waiting packet

    \Input *ref     - reference pointer
    \Input *target  - target buffer
    \Input length   - buffer length
    \Input *when    - tick received at

    \Output
        int32_t         - negative=nothing pending, else packet length

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPPeek(CommTCPRef *ref, void *target, int32_t length, uint32_t *when)
{
    RawTCPPacketT *packet;

    // see if a packet is available
    if (ref->rcvout == ref->rcvinp)
    {
        return(COMM_NODATA);
    }

    // point to the packet
    packet = (RawTCPPacketT *) &(ref->rcvbuf[ref->rcvout]);

    // copy over the data portion
    memcpy(target, packet->body.data, (packet->body.len < length ? packet->body.len : length));
    // get the timestamp
    if (when != NULL)
    {
        *when = packet->head.when;
    }
    
    // return packet data length
    return(packet->body.len);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPRecv

    \Description
        Receive a packet from the buffer

    \Input *ref     - reference pointer
    \Input *target  - target buffer
    \Input length   - buffer length
    \Input *when    - tick received at

    \Output
        int32_t         - negative=error, else packet length

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPRecv(CommTCPRef *ref, void *target, int32_t length, uint32_t *when)
{
    // use peek to remove the data
    int32_t len = CommTCPPeek(ref, target, length, when);
    if (len >= 0)
    {
        ref->rcvout = (ref->rcvout+ref->rcvwid)%ref->rcvlen;
    }
    // all done
    return(len);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPListen

    \Description
        Listen for a connection

    \Input *ref     - reference pointer
    \Input *addr    - port to listen on (only :port portion used)

    \Output
        int32_t         - negative=error, zero=ok

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPListen(CommTCPRef *ref, const char *addr)
{
    int32_t err;
    struct sockaddr bindaddr;
    SocketT *sock;

    // make sure in valid state
    if ((ref->state != IDLE) || (ref->socket != NULL))
    {
        return(COMM_BADSTATE);
    }

    // setup bind point
    SockaddrInit(&bindaddr, AF_INET);

    // parse the address (only allow connects from this host)
    // extract the port number (this is the listen target)
    if ((SockaddrInParse(&bindaddr, addr) & 2) == 0)
    {
        return(COMM_BADADDRESS);
    }
                          
    // reset to unresolved
    _CommTCPResetTransfer(ref);

    // create socket in case its needed
    sock = SocketOpen(AF_INET, SOCK_STREAM, 0);
    _CommTCPSetSocket(ref, sock);
    if (ref->socket == NULL)
    {
        return(COMM_NORESOURCE);
    }

    // bind the address to the socket
    err = SocketBind(ref->socket, &bindaddr, sizeof(bindaddr));
    if (err < 0)
    {
        SocketClose(ref->socket);
        _CommTCPSetSocket(ref, NULL);
        return(COMM_UNEXPECTED);
    }

    // start the listen
    SocketListen(ref->socket, 5);

    // setup for callbacks
    SocketCallback(ref->socket, CALLB_RECV, 100, ref, &_CommTCPEvent);

    #if COMMTCP_VERBOSE
    NetPrintf(("commtcp: listening for connect %s", addr));
    #endif

    // put into listen mode
    ref->state = LIST;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommTCPConnect

    \Description
        Initiate a connection to a peer

    \Input *ref     - reference pointer
    \Input *addr    - address in ip-address:port form

    \Output
        int32_t         - negative=error, zero=ok

    \Notes
        Does not currently perform dns translation

    \Version    1.0        02/19/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommTCPConnect(CommTCPRef *ref, const char *addr)
{
    SocketT *sock;
    
    // make sure in valid state
    if ((ref->state != IDLE) || (ref->socket != NULL))
    {
        return(COMM_BADSTATE);
    }

    // setup target info
    SockaddrInit(&ref->peeraddr, AF_INET);

    // parse address and port from addr string
    if (SockaddrInParse(&ref->peeraddr, addr) != 3)
    {
        return(COMM_BADADDRESS);
    }

    #if COMMTCP_VERBOSE
    NetPrintf(("commtcp: connecting to %s\n",addr));
    #endif

    // reset to unresolved
    _CommTCPResetTransfer(ref);

    // create the actual socket
    sock = SocketOpen(AF_INET, SOCK_STREAM, 0);
    _CommTCPSetSocket(ref, sock);
    if (ref->socket == NULL)
    {
        return(COMM_NORESOURCE);
    }

    // start the connect
    SocketConnect(ref->socket, &ref->peeraddr, sizeof(ref->peeraddr));

    // setup for callbacks
    SocketCallback(ref->socket, CALLB_RECV, 100, ref, &_CommTCPEvent);

    // change the state
    ref->state = CONN;
    return(0);
}
