/*H*************************************************************************************************/
/*!

    \File    commudp.c

    \Description
        This is a reliable UDP transport class optimized for use in a
        controller passing game applications. When the actual data
        bandwidth is low (as is typical with controller passing), it
        sends redundant data in order to quickly recover from any lost
        packets. Overhead is low adding only 8 bytes per packet in
        addition to UDP/IP overhead. This module support mutual
        connects in order to allow connections through firewalls.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    0.1        02/09/99 (GWS) First Version
    \Version    0.2        02/14/99 (GWS) Revised and enhanced
    \Version    0.5        02/14/99 (GWS) Alpha release
    \Version    1.0        07/30/99 (GWS) Final release
    \Version    2.0        10/27/99 (GWS) Revised to use winsock 1.1/2.0
    \Version    2.1        12/04/99 (GWS) Removed winsock 1.1 support
    \Version    2.2        01/12/00 (GWS) Fixed receive tick bug
    \Version    2.3        06/12/00 (GWS) Added fastack for low-latency nets
    \Version    2.4        12/04/00 (GWS) Added firewall penetrator
    \Version    3.0        12/04/00 (GWS) Reported to dirtysock
    \Version    3.1        11/20/02 (JLB) Added Send() flags parameter
    \Version    3.2        02/18/03 (JLB) Fixes for multiple connection support
    \Version    3.3        05/06/03 (GWS) Allowed poke to come from any IP
    \Version    3.4        09/02/03 (JLB) Added unreliable packet type
    \Version    4.0        09/12/03 (JLB) Per-send optional unreliable transport
    \Version    5.0        07/07/09 (jrainy) Putting meta-data bits over the high bits of the sequence number
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <stdio.h>
#include <string.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/comm/commall.h"
#include "DirtySDK/comm/commudp.h"

/*** Defines ***************************************************************************/

#undef COMM_PRINT
#define COMM_PRINT      (0)
#define ESC_CAUSES_LOSS (DIRTYCODE_DEBUG && defined(DIRTYCODE_PC) && 0)

#if ESC_CAUSES_LOSS
#include <windows.h>
#endif

#define BUSY_KEEPALIVE  (100)
#define IDLE_KEEPALIVE  (2500)
#define PENETRATE_RATE  (1000)
#define UNACK_LIMIT     (2048)

//! define protocol packet types
enum {
    RAW_PACKET_INIT = 1,        // initiate a connection
    RAW_PACKET_CONN,            // confirm a connection
    RAW_PACKET_DISC,            // terminate a connection
    RAW_PACKET_NAK,             // force resend of lost data
    RAW_PACKET_POKE,            // try and poke through firewall

    RAW_PACKET_UNREL = 128,     // unreliable packet send (must be power of two)
                                // 128-255 reserved for unreliable packet sequence
    RAW_PACKET_DATA = 256,      // initial data packet sequence number (must be power of two)

    /*  Width of the sequence window, can be anything provided 
        RAW_PACKET_DATA + RAW_PACKET_DATA_WINDOW
        doesn't overlap the meta-data bits.
    */  
    RAW_PACKET_DATA_WINDOW = (1 << 24) - 256
};

#define RAW_METATYPE1_SIZE  (8)
//! max additional space needed by a commudp meta type
#define COMMUDP_MAX_METALEN (8)

#define SEQ_MASK (0x00ffffff)
#define SEQ_MULTI_SHIFT (28)
#define SEQ_META_SHIFT  (28 - 4)
#define SEQ_MULTI_INC (1 << SEQ_MULTI_SHIFT)

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! raw protocol packet format
typedef struct
{
    //! packet header which is not sent/received (this data is used internally)
    struct {
        int32_t len;                        //!< variable data len (-1=none) (used int32_t for alignment)
        uint32_t when;                      //!< tick when a packet was received
        uint32_t meta;                      //!< four-bit metadata field extracted from seq field
    } head;
    //! packet body which is sent/received
    struct {
        uint32_t seq;                       //!< packet type or sequence number
        uint32_t ack;                       //!< acknowledgement of last packet
        uint8_t  data[SOCKET_MAXUDPRECV-8]; //!< user data
    } body;
} RawUDPPacketT;

//! raw protocol packet header (no data) -- used for stack local formatting of packets without data
typedef struct
{
    //! packet header which is not sent/received (this data is used internally)
    struct {
        int32_t len;            //!< variable data len (-1=none) (used int32_t for alignment)
        uint32_t when;          //!< tick when a packet was received
        uint32_t meta;          //!< four-bit metadata field extracted from seq field
    } head;
    //! packet body which is sent/received
    struct {
        uint32_t seq;           //!< packet type or seqeunce number
        uint32_t ack;           //!< acknowledgement of last packet
        uint32_t cid;           //!< client id (only included in INIT and CONN packets)
        uint32_t data[16];      //!< space for possible metadata included in control packets
    } body;
} RawUDPPacketHeadT;

//! private module storage
struct CommUDPRef
{
    //! common header is first
    CommRef common;

    //! max amount of unacknowledged data that can be sent in one go (default 2k)
    uint32_t unacklimit; 

    //! max amount of data that can be sent redundantly (default 64)
    uint32_t redundantlimit;

    //! linked list of all instances
    CommUDPRef *link;
    //! comm socket
    SocketT *socket;
    //! peer address
    struct sockaddr peeraddr;

    //! port state
    enum {
        DEAD,       //!< dead
        IDLE,       //!< idle
        CONN,       //!< conn
        LIST,       //!< list
        OPEN,       //!< open
        CLOSE       //!< close
    } state;

    //! identifier to keep from getting spoofed
    uint32_t connident;
    
    //! type of metachunk to include in stream (zero=none)
    uint32_t metatype;
    
    //! unique client identifier (used for game server identification)
    uint32_t clientident;
    //! remote client identifier
    uint32_t rclientident;

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
    //! next unreliable packet expected
    uint32_t urcvseq;
    //! last packet we acknowledged
    uint32_t rcvack;
    //! number of unacknowledged received bytes
    int32_t rcvuna;

    //! width of send record (same as width of receive)
    int32_t sndwid;
    //! length of send buffer (multiple of sndwid)
    int32_t sndlen;
    //! fifo input offset
    int32_t sndinp;
    //! fifo output offset
    int32_t sndout;
    //! current output point within fifo
    int32_t sndnxt;
    //! pointer to buffer storage
    char *sndbuf;
    //! next packet to send (sequence number)
    uint32_t sndseq;
    //! unreliable packet sequence number
    uint32_t usndseq;
    //! last send result
    uint32_t snderr;

    //! tick at which last packet was sent
    uint32_t sendtick;
    //! tick at which last packet was received
    uint32_t recvtick;
    //! tick at which last idle callback made
    uint32_t idletick;

    //! control access during callbacks
    volatile int32_t callback;
    //! indicate there is an event pending
    uint32_t gotevent;
    //! callback routine pointer
    void (*callproc)(void *ref, int32_t event);
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

//! linked list of port objects
static CommUDPRef   *g_link = NULL;

//! semaphore to synchronize thread access
static NetCritT     g_crit;

//! missed event marker
static int32_t      g_missed;

//! variable indicates call to _CommUDPEvent() in progress
static int32_t      g_inevent;

#if DIRTYCODE_LOGGING
static const char  *g_strConnNames[] = { "RAW_PACKET_INVALID", "RAW_PACKET_INIT", "RAW_PACKET_CONN", "RAW_PACKET_DISC", "RAW_PACKET_NAK", "RAW_PACKET_POKE" };
#endif

// Public variables


/*** Private Functions *****************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSeqnDelta

    \Description
        Compute the sequence number off of uPos by iDelta places
        Can NOT be used for unreliable sequence offsetting

    \Input uPos     - starting position
    \Input iDelta   - offset

    \Output
        uint32_t    - resulting position

    \Version 07/07/09 (jrainy)
*/
/*************************************************************************************************F*/
static uint32_t _CommUDPSeqnDelta(uint32_t uPos, int32_t iDelta)
{
    return(((uPos + iDelta + RAW_PACKET_DATA_WINDOW - RAW_PACKET_DATA) % RAW_PACKET_DATA_WINDOW) + RAW_PACKET_DATA);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSeqnDelta

    \Description
        Compute the difference in position between two sequence numbers
        Can NOT be used to compute unreliable sequence differences

    \Input uPos1    - source position
    \Input uPos2    - target position

    \Output
        int32_t     - signed difference in position

    \Version 07/07/09 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _CommUDPSeqnDiff(uint32_t uPos1, uint32_t uPos2)
{
    return(((uPos1 - uPos2) + (3 * RAW_PACKET_DATA_WINDOW / 2)) % RAW_PACKET_DATA_WINDOW) - (RAW_PACKET_DATA_WINDOW / 2);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSetAddrInfo

    \Description
        Sets peer/host addr/port info in common ref.

    \Input *ref     - reference pointer
    \Input *sin     - address pointer

    \Version 04/16/04 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPSetAddrInfo(CommUDPRef *ref, struct sockaddr *sin)
{
    // save peer addr/port info in common ref
    ref->common.peerip = SockaddrInGetAddr(sin);
    ref->common.peerport = SockaddrInGetPort(sin);
    NetPrintf(("commudp: peer=%a:%d\n", ref->common.peerip, ref->common.peerport));
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSetSocket

    \Description
        Sets socket in ref and socketref in common portion of ref.

    \Input *pRef    - reference pointer
    \Input *pSocket - socket to set

    \Version 08/24/04 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPSetSocket(CommUDPRef *pRef, SocketT *pSocket)
{
    pRef->socket = pSocket;
    pRef->common.sockptr = pSocket;
    if (pSocket != NULL)
    {
        struct sockaddr SockAddr;
        // save host addr/port info in common ref    
        SocketInfo(pRef->socket, 'bind', 0, &SockAddr, sizeof(SockAddr));
        pRef->common.hostip = SocketGetLocalAddr();
        pRef->common.hostport = SockaddrInGetPort(&SockAddr);
        NetPrintf(("commudp: host=%a:%d\n", pRef->common.hostip, pRef->common.hostport));
    }
    else
    {
        pRef->common.hostip = 0;
        pRef->common.hostport = 0;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPSetConnID

    \Description
        Sets connident to the 32bit hash of the specified connection identifier string, if any.

    \Input *ref         - reference pointer
    \Input *pStrConn    - pointer to user-specified connection string

    \Version 06/16/04 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPSetConnID(CommUDPRef *ref, const char *pStrConn)
{
    const char *pConnID = strchr(pStrConn, '#');
    if (pConnID != NULL)
    {
        ref->connident = NetHash(pConnID+1);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPResetTransfer

    \Description
        Reset the transfer state

    \Input *ref     - reference pointer

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static void _CommUDPResetTransfer(CommUDPRef *ref)
{
    // reset the send queue
    ref->sndinp = 0;
    ref->sndout = 0;
    ref->sndnxt = 0;
    // reset the sequence number
    ref->sndseq = RAW_PACKET_DATA;
    ref->usndseq = RAW_PACKET_UNREL;
    // reset the receive queue
    ref->rcvinp = 0;
    ref->rcvout = 0;
    // reset the packet sequence number
    ref->rcvseq = RAW_PACKET_DATA;
    ref->urcvseq = RAW_PACKET_UNREL;

    // no unack data
    ref->rcvuna = 0;

    // make sendtick really old (in protocol terms)
    ref->sendtick = NetTick()-5000;
    // recvtick must be older than tick because the first
    // packet that arrives may come in moments before this
    // initialization takes place and without this adjustment
    // code can compute an elapsed receive time of 0xffffffff]
    ref->recvtick = NetTick()-5000;
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPOverhead

    \Description
        Computes the bandwidth overhead associated with a packet of length packetLength

    \Input *ref         - module reference
    \Input pktlen       - length of the packet we are about to send

    \Output
        int32_t         - the associated overhead. 28 on most platforms, but higher on xbox360.

    \Version 01/08/07 (JRainy)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPOverhead(CommUDPRef *ref, int32_t pktlen)
{
    // start with basic IP+UDP header size
    int32_t overhead = 28;
    
    #if defined(DIRTYCODE_XENON)
    /* from the XDK documentation :
    28 is
     IP header Standard IP header 20, NAT UDP For NAT traversal 8     
    20 is
     XFlags Packet information 1, XSP Security parameter index 3, SourcePort UDP source port 0-2
     DestPort UDP dest port 0-2, XSeqNum Packet sequence number 2, XHash Authentication hash 10
    The rest is to compute the padding length :
     Padding Pad to 8-byte boundary 0-7
     this one appears right after the payload, which is already 8-bytes-aligned */
    if ((ref->common.hostport == 1000) && (ref->common.peerport == 1000))
    {
        // no extra overhead
    }
    else if ((ref->common.hostport > 1000) && (ref->common.hostport < 1255) &&
             (ref->common.peerport > 1000) && (ref->common.peerport < 1255))
    {
        overhead += 2;
    }
    else
    {
        overhead += 4;
    }
    overhead += 16 + ((8 - (pktlen % 8)) % 8);
    #endif

    return(overhead);
}
/*F*************************************************************************************************/
/*!
    \Function    _CommUDPWrite

    \Description
        Send a packet to the peer

    \Input *ref         - reference pointer
    \Input *packet      - packet pointer
    \Input *peeraddr    - address of peer to send to
    \Input currtick     - current tick

    \Output
        int32_t         - negative=error, zero=ok

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPWrite(CommUDPRef *ref, RawUDPPacketT *packet, struct sockaddr *peeraddr, uint32_t currtick)
{
    int32_t err;
    int32_t len;

    #if ESC_CAUSES_LOSS
    // lose packets when escape is pressed
    if (GetAsyncKeyState(VK_ESCAPE) < 0)
    {
        NetPrintf(("commudp: dropping packet to simulate packet loss\n"));
        ref->sendtick = currtick;
        return(0);
    }
    #endif

    // figure full packet length (nak/ack fields + variable data)
    len = sizeof(packet->body)-sizeof(packet->body.data)+packet->head.len;

    // fill in metatype info    
    if (ref->metatype == 1)
    {
        int32_t iMetaOffset = ((packet->body.seq == RAW_PACKET_INIT) || (packet->body.seq == RAW_PACKET_POKE)) ? 4 : 0;
        // make room for metadata
        memmove(&packet->body.data[RAW_METATYPE1_SIZE+iMetaOffset], &packet->body.data[0+iMetaOffset], len-iMetaOffset);
        len += RAW_METATYPE1_SIZE;
        // metatype1 src clientident
        packet->body.data[0+iMetaOffset] = (uint8_t)(ref->clientident >> 24);
        packet->body.data[1+iMetaOffset] = (uint8_t)(ref->clientident >> 16);
        packet->body.data[2+iMetaOffset] = (uint8_t)(ref->clientident >> 8);
        packet->body.data[3+iMetaOffset] = (uint8_t)(ref->clientident);
        // metatype1 dst clientident
        packet->body.data[4+iMetaOffset] = (uint8_t)(ref->rclientident >> 24);
        packet->body.data[5+iMetaOffset] = (uint8_t)(ref->rclientident >> 16);
        packet->body.data[6+iMetaOffset] = (uint8_t)(ref->rclientident >> 8);
        packet->body.data[7+iMetaOffset] = (uint8_t)(ref->rclientident);
        // set metatype header
        packet->body.seq |= (ref->metatype & 0xf) << SEQ_META_SHIFT;
    }

    #if COMM_PRINT > 1
    NetPrintf(("commudp: [0x%08x] [0x%08x] send %d bytes to %a:%d\n", packet->body.seq, packet->body.ack, len, SockaddrInGetAddr(peeraddr), SockaddrInGetPort(peeraddr)));
    #endif
    #if COMM_PRINT > 2
    NetPrintMem(&packet->body, len, "cudp-send");
    #endif

    // translate seq and ack to network order for send
    packet->body.seq = SocketHtonl(packet->body.seq);
    packet->body.ack = SocketHtonl(packet->body.ack);

    // store send time in misc field of sockaddr
    SockaddrInSetMisc(peeraddr, currtick);

    // send the packet
    err = SocketSendto(ref->socket, (char *)&packet->body, len, 0, peeraddr, sizeof(*peeraddr));

    // translate seq and ack back to host order
    packet->body.seq = SocketNtohl(packet->body.seq);
    packet->body.ack = SocketNtohl(packet->body.ack);

    // check for success
    if (err == len)
    {
        // update last send time
        ref->sendtick = currtick;
        ref->common.datasent += len;
        ref->common.packsent += 1;
        ref->common.overhead += _CommUDPOverhead(ref, len);

        // is the packet reliable?
        if ((packet->body.seq & SEQ_MASK) >= RAW_PACKET_DATA)
        {
            // we assume any reliable send includes an up to date ack value
            // which means that we can reset the unacked data count to zero
            ref->rcvuna = 0;
        }
    }
    else
    {
        NetPrintf(("commudp: SocketSendto() returned %d\n", err));
        ref->snderr = err;
        err = -1;
    }

    return(err);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPClose

    \Description
        Close the connection

    \Input *ref     - reference pointer
    \Input currtick - current tick

    \Output
        int32_t     - negative=error, zero=ok

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPClose(CommUDPRef *ref, uint32_t currtick)
{
    RawUDPPacketHeadT packet;

    // if we're open, shut down connection
    if (ref->state == OPEN)
    {
        // see if any output data pending
        if (ref->sndnxt != ref->sndinp)
        {
            NetPrintf(("commudp: unsent data pending\n"));
        }
        else if (ref->sndout != ref->sndinp)
        {
            NetPrintf(("commudp: unacked data pending\n"));
        }

        // send a disconnect message
        packet.head.len = 0;
        packet.body.seq = RAW_PACKET_DISC;
        packet.body.ack = ref->connident;
        _CommUDPWrite(ref, (RawUDPPacketT *)&packet, &ref->peeraddr, currtick);
    }

    // set to disconnect state
    NetPrintf(("commudp: closed connection\n"));
    ref->connident = 0;
    ref->state = CLOSE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessSetup

    \Description
        Process a setup/teardown request

    \Input *ref     - reference pointer
    \Input *packet  - requesting packet
    \Input *sin     - address
    \Input currtick - current tick

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static void _CommUDPProcessSetup(CommUDPRef *ref, const RawUDPPacketT *packet, struct sockaddr *sin, uint32_t currtick)
{
    // make sure the session identifier matches
    if (packet->body.ack != ref->connident)
    {
        NetPrintf(("commudp: warning - connident mismatch (expected=0x%08x got=0x%08x)\n", ref->connident, packet->body.ack));
        // an init packet with a different session identifier
        // indicates that the old session has closed
        if (packet->body.seq == RAW_PACKET_INIT)
        {
            ref->state = CLOSE;
        }
        return;
    }
    
    // update valid receive time -- must put into past to avoid race condition
    ref->recvtick = currtick-1000;

    // response to connection/poke query
    if ((packet->body.seq == RAW_PACKET_INIT) || (packet->body.seq == RAW_PACKET_POKE))
    {
        RawUDPPacketHeadT connpacket;
        
        // set host/peer addr/port info
        _CommUDPSetAddrInfo(ref, sin);

        // send a response
        NetPrintf(("commudp: sending CONN packet to %a:%d connident=0x%08x\n", SockaddrInGetAddr(&ref->peeraddr),
            SockaddrInGetPort(&ref->peeraddr), ref->connident));
        connpacket.head.len = 0;
        connpacket.body.seq = RAW_PACKET_CONN;
        connpacket.body.ack = ref->connident;
        _CommUDPWrite(ref, (RawUDPPacketT *)&connpacket, &ref->peeraddr, currtick);
        return;
    }

    // response to a connect confirmation
    if (packet->body.seq == RAW_PACKET_CONN)
    {
        // change to open if not already there
        if (ref->state == CONN)
        {
            // set host/peer addr/port info
            _CommUDPSetAddrInfo(ref, sin);
            ref->state = OPEN;
        }
        return;
    }

    // response to disconnect message
    if (packet->body.seq == RAW_PACKET_DISC)
    {
        // close the connection
        if (ref->state == OPEN)
        {
            ref->state = CLOSE;
        }
        NetPrintf(("commudp: received DISC packet\n"));
    }

    // should not get here
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessInit

    \Description
        Initiate a connection

    \Input *ref     - reference pointer
    \Input currtick - current tick

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static void _CommUDPProcessInit(CommUDPRef *ref, uint32_t currtick)
{
    RawUDPPacketHeadT packet;

    NetPrintf(("commudp: sending INIT packet to %a:%d connident=0x%08x clientident=0x%08x\n", SockaddrInGetAddr(&ref->peeraddr),
        SockaddrInGetPort(&ref->peeraddr), ref->connident, ref->clientident));
        
    // format init packet
    packet.head.len = sizeof(packet.body.cid);
    packet.body.seq = RAW_PACKET_INIT;
    packet.body.ack = ref->connident;
    packet.body.cid = SocketHtonl(ref->clientident);

    // send init to peer
    _CommUDPWrite(ref, (RawUDPPacketT *)&packet, &ref->peeraddr, currtick);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessOutput

    \Description
        Send data packet(s)

    \Input *ref     - reference pointer
    \Input currtick - current tick

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static void _CommUDPProcessOutput(CommUDPRef *ref, uint32_t currtick)
{
    int32_t index, count;
    int32_t tlimit = ref->unacklimit;
    int32_t plimit = ref->unacklimit/4;
    static RawUDPPacketT multi;
    RawUDPPacketT *buffer;
    static uint32_t mlimit = 0x20000000;
    int32_t metalen = (ref->metatype == 1) ? RAW_METATYPE1_SIZE : 0;

    // figure unacked data length
    for (index = ref->sndout; index != ref->sndnxt; index = (index+ref->sndwid)%ref->sndlen) {
        buffer = (RawUDPPacketT *) &ref->sndbuf[index];
        // count down the limit
        tlimit -= buffer->head.len;
    }

    // allow a minimum send rate (256 bytes per 250 ms)
    if ((tlimit < 256) && (currtick-ref->sendtick > 250))
        tlimit = 256;

    // send as much data as limit allows
    while (tlimit > 0) {

        // limit size of forward packet consolidation
        plimit = SocketInfo(NULL, 'maxp', 0, NULL, 0) - metalen - sizeof(multi.body) + sizeof(multi.body.data);
        
        if (plimit > tlimit)
            plimit = tlimit;

        // attempt forward consolation of packets
        for (count = 0; (count < 8) && (plimit > 0) && (ref->sndnxt != ref->sndinp); ++count) {
            buffer = (RawUDPPacketT *) &ref->sndbuf[ref->sndnxt];
            plimit -= (buffer->head.len+1);
            // if not the first packet, then we must be careful about size
            if ((count > 0) && (plimit <= 0))
                break;
            ref->sndnxt = (ref->sndnxt+ref->sndwid) % ref->sndlen;
            // if packet is > 250 chars, it must be final packet (first in multisend)
            if (buffer->head.len > 250) {
                ++count;
                break;
            }
        }
        if (count == 0)
            return;

        // setup main packet
        index = (ref->sndnxt+ref->sndlen-ref->sndwid)%ref->sndlen;
        buffer = (RawUDPPacketT *) &ref->sndbuf[index];
        // if they want a callback, do it now
        if (ref->common.SendCallback != NULL)
            ref->common.SendCallback((CommRef *)ref, buffer->body.data, buffer->head.len, currtick);
        memcpy(&multi, buffer, sizeof(buffer->head)+8+buffer->head.len);
        count -= 1;

        // add in required preceding packets
        for (; count > 0; --count) {
            // move to preceding packet
            index = (index+ref->sndlen-ref->sndwid)%ref->sndlen;
            // point to potential piggyback packet
            buffer = (RawUDPPacketT *) &ref->sndbuf[index];
            // if they want a callback, do it now
            if (ref->common.SendCallback != NULL)
                ref->common.SendCallback((CommRef *)ref, buffer->body.data, buffer->head.len, currtick);
            // combine the packets
            multi.body.seq += SEQ_MULTI_INC;
            memcpy(multi.body.data+multi.head.len, buffer->body.data, buffer->head.len);
            multi.head.len += buffer->head.len;
            multi.body.data[multi.head.len] = (unsigned char)buffer->head.len;
            multi.head.len += 1;
        }

        // add in optional redundant packets
        while ((index != ref->sndout) && (multi.body.seq <= mlimit)) {
            // move to preceding packet
            index = (index+ref->sndlen-ref->sndwid)%ref->sndlen;
            // point to potential piggyback packet
            buffer = (RawUDPPacketT *) &ref->sndbuf[index];
            // if packet is > 250 chars, we cannot multi-send it
            if (buffer->head.len > 250)
                break;
            // see if combined length would be a problem
            if ((multi.head.len + buffer->head.len + 1) > (signed)ref->redundantlimit)
                break;
            // if they want a callback, do it now
            if (ref->common.SendCallback != NULL)
                ref->common.SendCallback((CommRef *)ref, buffer->body.data, buffer->head.len, currtick);
            // combine the packets
            multi.body.seq += SEQ_MULTI_INC;
            memcpy(multi.body.data+multi.head.len, buffer->body.data, buffer->head.len);
            multi.head.len += buffer->head.len;
            multi.body.data[multi.head.len] = (unsigned char)buffer->head.len;
            multi.head.len += 1;
        }

        // adjust the max redundancy
        if (index == ref->sndout) {
            mlimit = 0x20000000;
        } else {
            mlimit = (mlimit < 0x80000000 ? mlimit*2 : 0xf0000000);
        }

        // update the ack value (one less than one we are waiting for)
        ref->rcvack = ref->rcvseq;
        multi.body.ack = _CommUDPSeqnDelta(ref->rcvseq, -1);
        // go ahead and send the packet
        if (_CommUDPWrite(ref, &multi, &ref->peeraddr, currtick) < 0)
        {
            return;
        }
        // count the bandwidth for this packet
        tlimit -= multi.head.len;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessFlow

    \Description
        Perform flow control based on ack/nak packets

    \Input *ref     - reference pointer
    \Input *packet  - incoming packet header
    \Input currtick - current tick

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static void _CommUDPProcessFlow(CommUDPRef *ref, RawUDPPacketHeadT *packet, uint32_t currtick)
{
    int32_t nak;
    uint32_t ack;

    // grab the ack point and nak flag
    nak = (packet->body.seq == RAW_PACKET_NAK);

    if (packet->body.ack < RAW_PACKET_DATA)
    {
        ack = (nak ? packet->body.ack-1 : packet->body.ack);
    }
    else
    {
        ack = (nak ? _CommUDPSeqnDelta(packet->body.ack, -1) : packet->body.ack);
    }

    // advance ack point
    while (ref->sndout != ref->sndinp)
    {
        RawUDPPacketT *buffer = (RawUDPPacketT *) &ref->sndbuf[ref->sndout];
        // see if this packet has been acked
        if (((ack & SEQ_MASK) < RAW_PACKET_DATA) ||
            (_CommUDPSeqnDiff(ack, buffer->body.seq) < 0))
        {
            break;
        }

        // if about to send this packet, skip to next
        if (ref->sndnxt == ref->sndout)
            ref->sndnxt = (ref->sndnxt+ref->sndwid) % ref->sndlen;
        // remove the packet from the queue
        ref->sndout = (ref->sndout+ref->sndwid) % ref->sndlen;
    }

    // reset send point for nak
    if (nak)
    {
        #if COMM_PRINT
        NetPrintf(("commudp: got NAK for packet %d\n", ack+1));
        #endif
        // reset send point
        ref->sndnxt = ref->sndout;
        // immediate restart
        _CommUDPProcessOutput(ref, currtick);
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessInput

    \Description
        Process incoming data packet

    \Input *ref     - module state
    \Input *packet  - incoming packet header
    \Input *data    - pointer to packet data
    \Input currtick - current network tick

    \Output
        int32_t     - -1=nak, 0=old, 1=new, 2=buffer full

    \Version 12/04/2000 (gschaefer)
*/
/*************************************************************************************************F*/
static int32_t _CommUDPProcessInput(CommUDPRef *ref, RawUDPPacketHeadT *packet, uint8_t *data, uint32_t currtick)
{
    RawUDPPacketT *buffer;

    // see if room in buffer for packet
    if ((ref->rcvinp+ref->rcvwid)%ref->rcvlen == ref->rcvout)
    {
        // no room in buffer -- just drop packet
        // could nak, but would generate lots of
        // network activity with no result
        NetPrintf(("commudp: input buffer overflow\n"));
        return(2);
    }

    // handle unreliable receive
    if (((packet->body.seq & SEQ_MASK) >= RAW_PACKET_UNREL) && ((packet->body.seq & SEQ_MASK) < RAW_PACKET_DATA))
    {
        // calculate the number of free packets in rcvbuf
        int32_t queuepos = ((ref->rcvinp+ref->rcvlen)-ref->rcvout)%ref->rcvlen;
        int32_t pktsfree = (ref->rcvlen - queuepos)/ref->rcvwid;

        // calculate delta between received sequence and expected sequence, accounting for wrapping
        int32_t delta = (packet->body.seq - ref->urcvseq) & (RAW_PACKET_UNREL - 1);

        // update lost packet count
        if ((packet->body.seq > ref->urcvseq) || (delta < RAW_PACKET_UNREL/4))
        {
            ref->common.packlost += delta;
        }

        // calculate new sequence number
        if ((ref->urcvseq = (packet->body.seq + 1)) >= RAW_PACKET_DATA)
        {
            ref->urcvseq = RAW_PACKET_UNREL + (ref->urcvseq - RAW_PACKET_DATA);
        }

        // see if there is room to buffer (leave room for reliable packets)
        if (pktsfree <= 4)
        {
            return(2);
        }
    }
    else
    {
        // ignore old packets
        if (_CommUDPSeqnDiff(packet->body.seq, ref->rcvseq) < 0)
        {
            return(0);
        }

        // immediate nak for missing packets
        if (_CommUDPSeqnDiff(packet->body.seq, ref->rcvseq) > 0)
        {
            // update lost packet count
            ref->common.packlost += _CommUDPSeqnDiff(packet->body.seq, ref->rcvseq);
            
            // send a nak packet
            #if COMM_PRINT
            NetPrintf(("commudp: sending a NAK of packet %d (tick=%u)\n", ref->rcvseq, NetTick()));
            #endif
            packet->body.seq = RAW_PACKET_NAK;
            packet->body.ack = ref->rcvseq;
            packet->head.len = 0;
            _CommUDPWrite(ref, (RawUDPPacketT *)packet, &ref->peeraddr, currtick);
            return(-1);
        }

        // no further processing for empty (ack) packets
        if (packet->head.len == 0)
        {
            return(0);
        }
    }

    // add the packet to the buffer
    buffer = (RawUDPPacketT *) &ref->rcvbuf[ref->rcvinp];
    // copy the packet
    memcpy(buffer, packet, sizeof(*packet));
    memcpy(buffer->body.data, data, packet->head.len);

    // limit receive access for callbacks
    ref->callback += 1;
    // add item to receive buffer
    ref->rcvinp = (ref->rcvinp+ref->rcvwid) % ref->rcvlen;

    // reliable specific processing
    if ((packet->body.seq & SEQ_MASK) >= RAW_PACKET_DATA)
    {
        ref->rcvseq = _CommUDPSeqnDelta(ref->rcvseq, 1);
        // add to unacknowledged byte count
        ref->rcvuna += buffer->head.len;
    }

    // indicate we got an event
    ref->gotevent |= 1;
    // let the callback process it
    if (ref->common.RecvCallback != NULL)
        ref->common.RecvCallback((CommRef *)ref, buffer->body.data, buffer->head.len, buffer->head.when);
    // release access to receive
    ref->callback -= 1;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessPoke

    \Description
        Penetrate firewall with poke packet

    \Input *ref     - reference pointer
    \Input currtick - current tick

    \Version 07/07/00 (GWS)

*/
/*************************************************************************************************F*/
static void _CommUDPProcessPoke(CommUDPRef *ref, uint32_t currtick)
{
    RawUDPPacketHeadT packet;

    NetPrintf(("commudp: sending POKE packet to %a:%d connident=0x%08x clientident=0x%08x\n", SockaddrInGetAddr(&ref->peeraddr),
        SockaddrInGetPort(&ref->peeraddr), ref->connident, ref->clientident));
        
    // length of body is just size of client identifier
    packet.head.len = sizeof(packet.body.cid);
    // poke packet to poke through firewall
    packet.body.seq = RAW_PACKET_POKE;
    // set connident
    packet.body.ack = ref->connident;
    // set clientident
    packet.body.cid = SocketHtonl(ref->clientident);
    
    // send it
    _CommUDPWrite(ref, (RawUDPPacketT *)&packet, &ref->peeraddr, currtick);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPProcessAlive

    \Description
        Send a keepalive packet

    \Input *ref     - reference pointer
    \Input currtick - current tick

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static void _CommUDPProcessAlive(CommUDPRef *ref, uint32_t currtick)
{
    RawUDPPacketHeadT packet;

    // see if we should resend most recent packet
    if ((ref->sndout != ref->sndinp) && (ref->sndnxt == ref->sndinp)) {
        // this shound result in a multi-send
        ref->sndnxt = (ref->sndnxt+ref->sndlen-ref->sndwid)%ref->sndlen;
        _CommUDPProcessOutput(ref, currtick);
        return;
    }

    // set our packet number
    packet.body.seq = ref->sndseq;
    // acknowledge packet prior to one we are waiting for
    ref->rcvack = ref->rcvseq;
    packet.body.ack = _CommUDPSeqnDelta(ref->rcvseq, -1);

    // no data means keepalive
    packet.head.len = 0;
    // send it
    #if COMM_PRINT
    NetPrintf(("commudp: sending keep-alive\n"));
    #endif
    _CommUDPWrite(ref, (RawUDPPacketT *)&packet, &ref->peeraddr, currtick);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPFlush

    \Description
        Flush output buffer

    \Input *ref     - reference pointer
    \Input limit    - timeout in milliseconds
    \Input currtick - current tick

    \Output
        uint32_t    - number of packets in buffer

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static uint32_t _CommUDPFlush(CommUDPRef *ref, uint32_t limit, uint32_t currtick)
{
    int32_t iNumPackets;
    int32_t index;
    RawUDPPacketT *buffer;

    iNumPackets = (((ref->sndinp+ref->sndlen)-ref->sndout)%ref->sndlen)/ref->sndwid;
    NetPrintf(("commudp: flushing %d packets in send queue\n", iNumPackets));

    for (index = ref->sndout; index != ref->sndnxt; index = (index+ref->sndwid)%ref->sndlen) 
    {
        buffer = (RawUDPPacketT *) &ref->sndbuf[index];
        _CommUDPWrite(ref, buffer, &ref->peeraddr, currtick);
    }

    return(iNumPackets);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPThreadData

    \Description
        Take care of data processing

    \Input currtick - current tick

    \Output
        int32_t     - number of packets received

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPThreadData(uint32_t currtick)
{
    int32_t len;
    int32_t count = 0;
    CommUDPRef *ref;
    CommUDPRef *pListen = NULL;
    struct sockaddr sin;
    static RawUDPPacketT packet;
    SocketT *pSocket;
    uint32_t connidentmatch=0;
    uint32_t lclientident=0, rclientident=0;

    // clear socket address packet
    // (else unused bytes cause mismatch during ident)
    memset(&sin, 0, sizeof(sin));

    // scan sockets for incoming packet
    packet.head.len = -1;
    pSocket = NULL;
    for (ref = g_link; ref != NULL; ref = ref->link) {
        // make sure we need to scan this one
        if ((ref->socket != NULL) && (ref->socket != pSocket)) {
            // see if data is pending
            pSocket = ref->socket;
            len = sizeof(sin);
            len = SocketRecvfrom(pSocket, (char *)&packet.body, sizeof(packet.body), 0, &sin, &len);
            if (len > 0) {
                // fill in the header
                packet.head.len = len-(sizeof(packet.body)-sizeof(packet.body.data));
                packet.head.when = SockaddrInGetMisc(&sin);
                // translate seq and ack to host order
                packet.body.seq = SocketNtohl(packet.body.seq);
                packet.body.ack = SocketNtohl(packet.body.ack);

                // extract and clear meta bits
                packet.head.meta = (packet.body.seq >> SEQ_META_SHIFT) & 0xf;
                packet.body.seq &= ~(0x0f << SEQ_META_SHIFT);

                #if COMM_PRINT > 1
                NetPrintf(("commudp: [0x%08x] [0x%08x] recv %d bytes from %a:%d (head.len=%d, meta=%d) at tick=%d\n", packet.body.seq, packet.body.ack, len, SockaddrInGetAddr(&sin), SockaddrInGetPort(&sin), packet.head.len, packet.head.meta, packet.head.when));
                #endif
                #if COMM_PRINT > 2
                NetPrintMem(&packet.body, len, "cudp-recv");
                #endif
                
                if (packet.body.seq == RAW_PACKET_CONN || (packet.body.seq == RAW_PACKET_INIT) || (packet.body.seq == RAW_PACKET_POKE))
                    NetPrintf(("commudp: got %s connident=0x%08x len=%d\n", g_strConnNames[packet.body.seq], packet.body.ack, packet.head.len));
                // count the packet
                ++count;
                break;
            }
        }
    }

    // if we have meta chunk use that for matching with our connection ref
    if ((packet.head.len >= RAW_METATYPE1_SIZE) && (packet.head.meta == 1))
    {
        int32_t iMetaOffset = ((packet.body.seq == RAW_PACKET_INIT) || (packet.body.seq == RAW_PACKET_POKE)) ? 4 : 0;
        
        // read meta type one chunk data
        rclientident = ((uint32_t)packet.body.data[0+iMetaOffset])<<24 | ((uint32_t)packet.body.data[1+iMetaOffset])<<16 | ((uint32_t)packet.body.data[2+iMetaOffset])<<8 | (uint32_t)packet.body.data[3+iMetaOffset];
        lclientident = ((uint32_t)packet.body.data[4+iMetaOffset])<<24 | ((uint32_t)packet.body.data[5+iMetaOffset])<<16 | ((uint32_t)packet.body.data[6+iMetaOffset])<<8 | (uint32_t)packet.body.data[7+iMetaOffset];
        
        // remove metadata from packet
        packet.head.len -= RAW_METATYPE1_SIZE;
        if ((packet.head.len-iMetaOffset) > 0)
        {
            memmove(&packet.body.data[0+iMetaOffset], &packet.body.data[RAW_METATYPE1_SIZE+iMetaOffset], packet.head.len-iMetaOffset);
        }
        #if COMM_PRINT > 2
        NetPrintf(("commudp: processed metadata chunk\n"));
        #endif
    }

    // walk port list and handle processing
    for (ref = g_link; ref != NULL; ref = ref->link)
    {
        // get latest tick
        currtick = NetTick();

        // identify port to handle connection request
        if ((pListen == NULL) && (pSocket == ref->socket) && (ref->state == LIST) && (packet.head.len >= 0) && 
            ((packet.body.seq == RAW_PACKET_INIT) || (packet.body.seq == RAW_PACKET_CONN)) && (ref->connident == packet.body.ack))
        {
            pListen = ref;
        }

        if (packet.head.len >= 0)
        {
            if (packet.head.meta == 1)
            {
                connidentmatch = (rclientident == ref->rclientident) && (lclientident == ref->clientident);
                #if COMM_PRINT > 2
                NetPrintf(("commudp: [0x%08x] matching with metadata: src=0x%08x(0x%08x) dst=0x%08x(0x%08x) match=%d\n", (uintptr_t)ref,
                    rclientident, ref->rclientident, lclientident, ref->clientident, connidentmatch));
                #endif
            }
            else
            {
                // if this is an INIT or POKE, make sure the connident matches
                connidentmatch = ((packet.body.seq == RAW_PACKET_INIT) || (packet.body.seq == RAW_PACKET_POKE)) ? (packet.body.ack == ref->connident) : 1;
            }
        }
        
        // see if packet belongs to someone
        if ((packet.head.len >= 0) && (ref->state != LIST) && (ref->state != CLOSE) && (pSocket == ref->socket) &&
            (SockaddrCompare(&ref->peeraddr, &sin) == 0) && (connidentmatch))
        {
            // do stats
            ref->common.datarcvd += packet.head.len;
            ref->common.packrcvd += 1;

            // transition to open state if we're getting data from peer
            if ((ref->state == CONN) && ((packet.body.seq == RAW_PACKET_UNREL) || (packet.body.seq == RAW_PACKET_DATA)))
            {
                NetPrintf(("commudp: transitioning to OPEN state due to received data from peer\n"));
                ref->state = OPEN;
            }

            // process an incoming packet
            if ((packet.body.seq == RAW_PACKET_INIT) ||
                (packet.body.seq == RAW_PACKET_POKE) ||
                (packet.body.seq == RAW_PACKET_CONN) ||
                (packet.body.seq == RAW_PACKET_DISC)) {
                // handle connection setup/teardown
                _CommUDPProcessSetup(ref, &packet, &sin, currtick);
            } else if (ref->state != OPEN) {
                // ignore the packet
                continue;
            } else if (packet.body.seq == RAW_PACKET_NAK) {
                // remember we got something
                ref->recvtick = packet.head.when;
                // resend the missing data
                _CommUDPProcessFlow(ref, (RawUDPPacketHeadT *)&packet, currtick);
            } else if (packet.body.seq > SEQ_MULTI_INC) {
                RawUDPPacketHeadT multi;
                uint32_t oldseq = ref->rcvseq;
                int32_t count2 = packet.body.seq >> SEQ_MULTI_SHIFT;
                multi.head.when = packet.head.when;
                multi.body.seq = _CommUDPSeqnDelta((packet.body.seq & SEQ_MASK), -count2);
                multi.body.ack = packet.body.ack;
                // remember we got something
                ref->recvtick = packet.head.when;
                // process all the packets
                for (; count2 >= 0; --count2) {
                    if (count2 > 0) {
                        packet.head.len -= 1;
                        multi.head.len = packet.body.data[packet.head.len];
                    } else {
                        multi.head.len = packet.head.len;
                    }
                    packet.head.len -= multi.head.len;

                    if (packet.head.len < 0)
                    {
                        // we just received corrupt data, bail out
                        break;
                    }

                    // process the ack info
                    _CommUDPProcessFlow(ref, &multi, currtick);
                    // process the input data and check for missing data (nak)
                    // (if nak, stop processing so we only send one nak packet)
                    if (_CommUDPProcessInput(ref, &multi, packet.body.data+packet.head.len, currtick) < 0)
                        count2 = 0;
                    // see if packets were saved
                    if ((count2 > 0) && (oldseq != ref->rcvseq))
                    {
                        #if COMM_PRINT
                        NetPrintf(("commudp: redundant packet %d prevented loss of packet %d\n", packet.body.seq & SEQ_MASK, oldseq));
                        #endif
                        oldseq = ref->rcvseq;
                    }
                    // advance the packet sequence number

                    // if we sent a nak, multi.body.seq got wiped and count2 set to 0, so don't increment it.
                    if (multi.body.seq >= RAW_PACKET_UNREL)
                    {
                        multi.body.seq = _CommUDPSeqnDelta(multi.body.seq, 1);
                    }
                }
            } else {
                // remember we got something
                ref->recvtick = packet.head.when;
                // process the ack info
                _CommUDPProcessFlow(ref, (RawUDPPacketHeadT *)&packet, currtick);
                // save the data
                _CommUDPProcessInput(ref, (RawUDPPacketHeadT *)&packet, packet.body.data, currtick);
            }
            // mark packet as processed
            packet.head.len = -1;
        }

        // see if we are trying to connect
        if ((ref->state == CONN) && (currtick-ref->sendtick > 1000))
            _CommUDPProcessInit(ref, currtick);

        // see if any output for this port
        if ((ref->state == OPEN) && (ref->sndnxt != ref->sndinp))
            _CommUDPProcessOutput(ref, currtick);

        // check for connection timeout
        if ((ref->state == OPEN) && (NetTickDiff(currtick, ref->recvtick) > 120*1000) && (NetTickDiff(currtick, ref->sendtick) < 2000)) {
            NetPrintf(("commudp: closing connection due to timeout\n"));
            NetPrintf(("commudp: tick=%d, rtick=%d, stick=%d\n", currtick, ref->recvtick, ref->sendtick));
            _CommUDPClose(ref, currtick);
        }

        // check for connection closure
        #if (defined(DIRTYCODE_XENON))
        if ((ref->snderr == SOCKERR_UNREACH) && ((ref->state == OPEN) || (ref->state == CONN) || (ref->state == LIST))) {
            NetPrintf(("commudp: closing connection as host is unreachable\n"));
            NetPrintf(("commudp: tick=%d, rtick=%d, stick=%d\n", currtick, ref->recvtick, ref->sendtick));
            _CommUDPClose(ref, currtick);
            ref->snderr = 0;
        }
        #endif

        // see if we should run the penetrator
        if ((ref->state == LIST) && (ref->peeraddr.sa_family == AF_INET) && 
            (currtick > ref->sendtick+PENETRATE_RATE)) {
            // penetrate the firewall
            _CommUDPProcessPoke(ref, currtick);
        }

        // see if callback needs an idle tick
        if ((ref->gotevent == 0) && (currtick > ref->idletick+250)) {
            ref->idletick = currtick;
            ref->gotevent |= 4;
        }

        // do callback if needed
        if ((ref->callback == 0) && (ref->gotevent != 0)) {
            // limit callback access
            ref->callback += 1;
            // callback the handler
            if (ref->callproc != NULL)
            {
                ref->callproc((CommRef *)ref, ref->gotevent);
            }
            else if (ref->state == OPEN)
            {
                #if COMM_PRINT
                NetPrintf(("commudp: no upper layer callback\n"));
                #endif
            }
            // limit callback access
            ref->callback -= 1;
            // reset event count
            ref->gotevent = 0;
        }

        // see if we need a keepalive
        // due this after callback since callback often generates a
        // packet send that eliminates the need for the idle packet
        if ((ref->state == OPEN) && (ref->sndnxt == ref->sndinp)) {
            int32_t timeout = NetTickDiff(currtick, ref->sendtick);
            if (((timeout > BUSY_KEEPALIVE) && (ref->rcvack != ref->rcvseq)) ||
                ((timeout > BUSY_KEEPALIVE) && (ref->sndinp != ref->sndout)) ||
//                ((timeout > (BUSY_KEEPALIVE*3)/4) && (ref->rcvuna > 0)) ||
                 (timeout > IDLE_KEEPALIVE) ||
                 (ref->rcvuna >= UNACK_LIMIT)) {
                // force reset of unack count just in case
                ref->rcvuna = 0;
                // send a keepalive
                _CommUDPProcessAlive(ref, currtick);
            }
        }
    }

    // check for unclaimed connection packet (port mangling)
    if ((packet.head.len >= 0) && (sin.sa_family == AF_INET) &&
        ((packet.body.seq == RAW_PACKET_POKE) ||
         (packet.body.seq == RAW_PACKET_INIT) ||
         (packet.body.seq == RAW_PACKET_CONN)))
    {
        uint32_t bIgnored = FALSE;
        NetPrintf(("commudp: received %s packet from %a:%d\n", g_strConnNames[packet.body.seq], SockaddrInGetAddr(&sin), SockaddrInGetPort(&sin)));
        
        // look for matching reference
        for (ref = g_link; ref != NULL; ref = ref->link)
        {
            if (((ref->state == CONN) || (ref->state == LIST)) && (ref->peeraddr.sa_family == AF_INET))
            {
                if (ref->connident == packet.body.ack)
                {
                    // see if they are trying to connect to poke source but port is mangled
                    if ((pSocket == ref->socket) && (SockaddrInGetAddr(&ref->peeraddr) == SockaddrInGetAddr(&sin)) && (SockaddrInGetPort(&ref->peeraddr) != SockaddrInGetPort(&sin)))
                    {
                        // assume poke source is masq, change port number to correspond
                        NetPrintf(("commudp: changing peer to %a:%d (was expecting %a:%d)\n", SockaddrInGetAddr(&sin), SockaddrInGetPort(&sin),
                            SockaddrInGetAddr(&ref->peeraddr), SockaddrInGetPort(&ref->peeraddr)));
                        memcpy(&ref->peeraddr, &sin, sizeof(ref->peeraddr));
                    }
                    break;
                }
                else // remember we ignored a packet with non-matching connident
                {
                    bIgnored = TRUE;
                }
            }
        }
        if ((ref == NULL) && (bIgnored == TRUE))
        {
            NetPrintf(("commudp: ignoring %s packet with connident 0x%08x\n", g_strConnNames[packet.body.seq], packet.body.ack));
        }
    }

    // see if this was an initial connection request
    if ((pListen != NULL) && (packet.head.len >= 0) && ((packet.body.seq == RAW_PACKET_INIT) || (packet.body.seq == RAW_PACKET_CONN)))
    {
        int32_t iPeerAddr = SockaddrInGetAddr(&pListen->peeraddr);
        ref = pListen;
        
        if ((ref->connident == packet.body.ack) && ((iPeerAddr == 0) || (iPeerAddr == SockaddrInGetAddr(&sin))))
        {
            NetPrintf(("commudp: transitioning to CONN state after receiving %s packet\n", g_strConnNames[packet.body.seq]));
            // change to connecting state
            ref->state = CONN;
            // if we were listening without a peer address, set it now
            if (iPeerAddr == 0)
            {
                memcpy(&ref->peeraddr, &sin, sizeof(ref->peeraddr));
            }
            // process init packet
            _CommUDPProcessSetup(ref, &packet, &sin, currtick);
        }
    }

    return(count);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPEvent

    \Description
        Main event function

    \Input *sock    - to be completed
    \Input flags    - to be completed
    \Input *_ref    - to be completed

    \Output
        int32_t     - to be completed

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPEvent(SocketT *sock, int32_t flags, void *_ref)
{
    // remember we're in an event call
    g_inevent = 1;

    // see if we have exclusive access
    if (NetCritTry(&g_crit))
    {
        uint32_t currtick = NetTick();
        // clear event counter before calling _CommUDPProcessThreadData to prevent possible unwanted recursion
        g_missed = 0;
        // process data as long as we get something
        while (_CommUDPThreadData(currtick) > 0)
            ;
        // free access
        NetCritLeave(&g_crit);
    }
    else
    {
        g_missed += 1;
        #if COMM_PRINT
        NetPrintf(("commudp: missed %d events\n", g_missed));
        #endif
    }

    // leaving event call
    g_inevent = 0;

    // done for now
    return(0);
}


/*F*************************************************************************************************/
/*!
    \Function    _CommUDPEnlistRef

    \Description
        Add the given ref to the global linked list of references.

    \Input *ref     - ref to add

    \Notes
        This also handles initialization of the global critical section global
        missed event counter.

    \Version 02/18/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPEnlistRef(CommUDPRef *ref)
{
    // if first ref, init global critical section
    if (g_link == NULL)
    {
        NetCritInit(&g_crit, "commudp-global");
        g_missed = 0;
        g_inevent = 0;
    }

    // add to linked list of ports
    NetCritEnter(&g_crit);
    ref->link = g_link;
    g_link = ref;
    NetCritLeave(&g_crit);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPDelistRef

    \Description
        Remove the given ref from the global linked list of references.    

    \Input *ref     - ref to remove

    \Notes
        This also handles destruction of the global critical section.

    \Version 02/18/03 (JLB)
*/
/*************************************************************************************************F*/
static void _CommUDPDelistRef(CommUDPRef *ref)
{
    CommUDPRef **link;

    // remove from linked list of ports
    NetCritEnter(&g_crit);
    for (link = &(g_link); *link != ref; link = &((*link)->link))
        ;
    *link = ref->link;
    NetCritLeave(&g_crit);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    CommUDPConstruct

    \Description
        Construct the class

    \Input maxwid   - max record width
    \Input maxinp   - input packet buffer size
    \Input maxout   - output packet buffer size

    \Output
        CommUDPRef  - construct pointer

    \Notes
        Initialized winsock for first class. also creates linked
        list of all current instances of the class and worker thread
        to do most udp stuff.

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
CommUDPRef *CommUDPConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout)
{
    CommUDPRef *ref;
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate class storage
    ref = DirtyMemAlloc(sizeof(*ref), COMMUDP_MEMID, iMemGroup, pMemGroupUserData);
    if (ref == NULL)
        return(NULL);
    memset(ref, 0, sizeof(*ref));
    ref->common.memgroup = iMemGroup;
    ref->common.memgrpusrdata = pMemGroupUserData;

    // initialize the callback routines
    ref->common.Construct = (CommAllConstructT *)CommUDPConstruct;
    ref->common.Destroy = (CommAllDestroyT *)CommUDPDestroy;
    ref->common.Resolve = (CommAllResolveT *)CommUDPResolve;
    ref->common.Unresolve = (CommAllUnresolveT *)CommUDPUnresolve;
    ref->common.Listen = (CommAllListenT *)CommUDPListen;
    ref->common.Unlisten = (CommAllUnlistenT *)CommUDPUnlisten;
    ref->common.Connect = (CommAllConnectT *)CommUDPConnect;
    ref->common.Unconnect = (CommAllUnconnectT *)CommUDPUnconnect;
    ref->common.Callback = (CommAllCallbackT *)CommUDPCallback;
    ref->common.Control = (CommAllControlT *)CommUDPControl;
    ref->common.Status = (CommAllStatusT *)CommUDPStatus;
    ref->common.Tick = (CommAllTickT *)CommUDPTick;
    ref->common.Send = (CommAllSendT *)CommUDPSend;
    ref->common.Peek = (CommAllPeekT *)CommUDPPeek;
    ref->common.Recv = (CommAllRecvT *)CommUDPRecv;

    // remember max packet width
    ref->common.maxwid = maxwid;
    ref->common.maxinp = maxinp;
    ref->common.maxout = maxout;

    // allocate the buffers
    ref->rcvwid = sizeof(RawUDPPacketT)-sizeof(((RawUDPPacketT *)0)->body.data)+maxwid+COMMUDP_MAX_METALEN;
    ref->rcvwid = (ref->rcvwid+3) & 0x7ffc;
    ref->rcvlen = ref->rcvwid * maxinp;
    ref->rcvbuf = (char *)DirtyMemAlloc(ref->rcvlen, COMMUDP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    ref->sndwid = sizeof(RawUDPPacketT)-sizeof(((RawUDPPacketT *)0)->body.data)+maxwid+COMMUDP_MAX_METALEN;
    ref->sndwid = (ref->sndwid+3) & 0x7ffc;
    ref->sndlen = ref->sndwid * maxout;
    ref->sndbuf = (char *)DirtyMemAlloc(ref->sndlen, COMMUDP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);

    // reset the socket
    _CommUDPSetSocket(ref, NULL);
    
    // reset peer address
    memset(&ref->peeraddr, 0, sizeof(ref->peeraddr));

    // reset the state
    ref->state = IDLE;
    ref->connident = 0;
    ref->unacklimit = UNACK_LIMIT;
    ref->redundantlimit = 64;

    // add to port list
    _CommUDPEnlistRef(ref);

    return(ref);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPDestroy

    \Description
        Destruct the class

    \Input *ref     - reference pointer

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
void CommUDPDestroy(CommUDPRef *ref)
{
    CommUDPRef *find;
    uint32_t currtick = NetTick();

    // flush final data
    _CommUDPFlush(ref, 200, currtick);

    // remove from port list
    _CommUDPDelistRef(ref);

    // if port is open, close it
    if (ref->state == OPEN)
        _CommUDPClose(ref, currtick);

    // kill the socket
    if (ref->socket != NULL) {
        // see if socket shared
        for (find = g_link; find != NULL; find = find->link) {
            if (find->socket == ref->socket)
                break;
        }
        // if we are only user of this socket
        if (find == NULL) {
            SocketClose(ref->socket);
            _CommUDPSetSocket(ref, NULL);
        }
    }

	// if last ref, destroy global critical section
    if (g_link == NULL)
    {
        NetCritKill(&g_crit);
    }

    // release resources
    DirtyMemFree(ref->rcvbuf, COMMUDP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    DirtyMemFree(ref->sndbuf, COMMUDP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    DirtyMemFree(ref, COMMUDP_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPCallback

    \Description
        Set upper layer callback

    \Input *ref         - reference pointer
    \Input *callback    - socket generating callback

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
void CommUDPCallback(CommUDPRef *ref, void (*callback)(void *ref, int32_t event))
{
    NetCritEnter(&g_crit);
    ref->callproc = callback;
    ref->gotevent |= 2;
    NetCritLeave(&g_crit);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPResolve

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
        is returned, target buffer is set to "~" until completion.

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPResolve(CommUDPRef *ref, const char *addr, char *buf, int32_t len, char div)
{
    NetPrintf(("commudp: resolve functionality not supported\n"));
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUnresolve

    \Description
        Stop the resolver

    \Input *ref     - reference pointer

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
void CommUDPUnresolve(CommUDPRef *ref)
{
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPListen0

    \Description
        Listen for a connection (private version)

    \Input *ref         - reference pointer
    \Input *sock        - fallback socket
    \Input *bindaddr    - local port

    \Output
        int32_t             - negative=error, zero=ok

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _CommUDPListen0(CommUDPRef *ref, SocketT *sock, struct sockaddr *bindaddr)
{
    int32_t err;
    CommUDPRef *find;
    struct sockaddr glueaddr;

    // make sure in valid state
    if (ref->state != IDLE)
    {
        SocketClose(sock);
        return(COMM_BADSTATE);
    }

    // reset to unresolved
    _CommUDPSetSocket(ref, NULL);
    _CommUDPResetTransfer(ref);

    // setup bind points
    memset(&glueaddr, 0, sizeof(glueaddr));
    memset(&ref->peeraddr, 0, sizeof(ref->peeraddr));

#if defined(DIRTYCODE_PC)
    /*
    the following line will fill in bindaddr with the IP addr previously set with
    SocketControl(... 'ladr' ...). If that selector was never used, then the IP
    address field of bindaddr will just be filled with 0.
    note: only required for multi-homed system (PC)
    */
    SocketInfo(NULL, 'ladr', 0, bindaddr, sizeof(bindaddr));
#endif

    // see if there is an existing socket bound to this port
    for (find = g_link; find != NULL; find = find->link)
    {
        // dont check ourselves or unbound sockets
        if ((find == ref) || (find->socket == NULL))
            continue;

        // see where this endpoint is bound
        if (SocketInfo(find->socket, 'bind', 0, &glueaddr, sizeof(glueaddr)) < 0)
            continue;

        /*
        see if the socket can be reused
            if the endpoint is virtual: we only compare the ports
            if the endpoint is not virtual:
                if bindaddr (specifying what we want to bind to) has ipaddr=0, we only compare the ports
                otherwise we compare the full sockaddr structs (i.e. family, port, addr)
        */
        if (SockaddrInGetPort(bindaddr) == SockaddrInGetPort(&glueaddr))
        {
            if ((SocketInfo(find->socket, 'virt', 0, NULL, 0) == TRUE) ||
                (SockaddrInGetAddr(bindaddr) == 0) ||
                (SockaddrCompare(bindaddr, &glueaddr) == 0))
            {
                // share the socket
                _CommUDPSetSocket(ref, find->socket);

                // dont need supplied socket
                SocketClose(sock);
                break;
            }
        }
    }

    // create socket if no existing one
    if (find == NULL)
    {
        // bind the address to the socket
        err = SocketBind(sock, bindaddr, sizeof(*bindaddr));
        if (err < 0)
        {
            NetPrintf(("commudp: bind to %d failed with %d\n", SockaddrInGetPort(bindaddr), err));
            SockaddrInSetPort(bindaddr, 0);
            if ((err = SocketBind(sock, bindaddr, sizeof(*bindaddr))) < 0)
            {
                NetPrintf(("commudp: bind to 0 failed with result %d\n", err));
                ref->state = DEAD;
                SocketClose(sock);
                return((err == SOCKERR_INVALID) ? COMM_PORTBOUND : COMM_UNEXPECTED);
            }
        }
        // use the supplied socket
        _CommUDPSetSocket(ref, sock);

        // setup for socket events
        SocketCallback(sock, CALLB_RECV, 100, NULL, &_CommUDPEvent);
    }

    // put into listen mode
    ref->state = LIST;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUnlisten

    \Description
        Stop listening

    \Input *ref     - reference pointer

    \Output
        int32_t     - negative=error, zero=ok

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t CommUDPUnlisten(CommUDPRef *ref)
{
    uint32_t currtick = NetTick();

    // flush final data
    _CommUDPFlush(ref, 200, currtick);

    // never close listening socket (it is shared)
    if (ref->state == LIST)
    {
        _CommUDPSetSocket(ref, NULL);
    }

    // get rid of socket if presernt
    if (ref->socket != NULL)
    {
        // attempt to close socket
        _CommUDPClose(ref, currtick);
        // done with socket
        SocketClose(ref->socket);
        _CommUDPSetSocket(ref, NULL);
    }

    // return to idle mode
    ref->state = IDLE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    _CommUDPConnect0

    \Description
        Initiate a connection to a peer (private version)

    \Input *ref         - reference pointer
    \Input *sock        - socket
    \Input *peeraddr    - peer address

    \Output
        int32_t         - negative=error, zero=ok

    \Notes
        Does not currently perform dns translation

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _CommUDPConnect0(CommUDPRef *ref, SocketT *sock, struct sockaddr *peeraddr)
{
    // make sure in valid state
    if (ref->state != IDLE) {
        SocketClose(sock);
        return(COMM_BADSTATE);
    }

    // reset to unresolved
    _CommUDPSetSocket(ref, NULL);
    _CommUDPResetTransfer(ref);

    // setup target info
    memcpy(&ref->peeraddr, peeraddr, sizeof(*peeraddr));

    // save the socket
    _CommUDPSetSocket(ref, sock);

    // setup for callbacks
    SocketCallback(sock, CALLB_RECV, 100, NULL, &_CommUDPEvent);

    // change the state
    ref->state = CONN;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPUnconnect

    \Description
        Terminate a connection

    \Input *ref     - reference pointer

    \Output
        int32_t     - negative=error, zero=ok


    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPUnconnect(CommUDPRef *ref)
{
    uint32_t currtick = NetTick();

    // flush final data
    _CommUDPFlush(ref, 200, currtick);

    // never close listening socket (it is shared)
    if (ref->state == LIST)
    {
        _CommUDPSetSocket(ref, NULL);
    }

    // get rid of socket if presernt
    if (ref->socket != NULL)
    {
        // attempt to close socket
        _CommUDPClose(ref, currtick);
        // done with socket
        SocketClose(ref->socket);
        _CommUDPSetSocket(ref, NULL);
    }

    // return to idle mode
    ref->state = IDLE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPStatus

    \Description
        Return current stream status

    \Input *ref     - reference pointer

    \Output
        int32_t     - CONNECTING, OFFLINE, ONLINE or FAILURE

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPStatus(CommUDPRef *ref)
{
    // return state
    if ((ref->state == CONN) || (ref->state == LIST))
        return(COMM_CONNECTING);
    if ((ref->state == IDLE) || (ref->state == CLOSE))
        return(COMM_OFFLINE);
    if (ref->state == OPEN)
        return(COMM_ONLINE);
    return(COMM_FAILURE);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPControl

    \Description
        Set connection behavior.

    \Input *pRef     - reference pointer
    \Input iControl - control selector
    \Input iValue   - selector specfic
    \Input *pValue  - selector specific

    \Output
        int32_t     - negative=error, else selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            'clid' - client identifier
            'meta' - set metatype (default=0)
            'rcid' - remote client identifier
            'rlmt' - redundant packet size limit (64 bytes default)
            'ulmt' - unacknowledged packet limit (2k default)
        \endverbatim

    \Version 02/20/2007 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t CommUDPControl(CommUDPRef *pRef, int32_t iControl, int32_t iValue, void *pValue)
{
    if (iControl == 'clid')
    {
        pRef->clientident = iValue;
        return(0);
    }
    if (iControl == 'meta')
    {
        NetPrintf(("commudp: setting metatype=%d\n", iValue));
        pRef->metatype = iValue;
        return(0);
    }
    if (iControl == 'rcid')
    {
        pRef->rclientident = iValue;
        return(0);
    }
    if (iControl == 'rlmt')
    {
        const int32_t iMaxLimit = sizeof(((RawUDPPacketT *)0)->body.data);
        if (iValue >= iMaxLimit)
        {
            iValue = iMaxLimit;
        }
        pRef->redundantlimit = iValue;
        NetPrintf(("commudp: setting rlimit to %d bytes\n", pRef->redundantlimit));
        return(0);
    }
    if (iControl == 'ulmt')
    {
        pRef->unacklimit = iValue;
        NetPrintf(("commudp: setting ulimit to %d bytes\n", pRef->unacklimit));
        return(0);
    }
    // unhandled
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPTick

    \Description
        Return current tick

    \Input *ref     - reference pointer

    \Output
        uint32_t    - elaped milliseconds

    \Version 12/04/00 (GWS)
*/
/*************************************************************************************************F*/
uint32_t CommUDPTick(CommUDPRef *ref)
{
    return(NetTick());
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPSend

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

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPSend(CommUDPRef *ref, const void *buffer, int32_t length, uint32_t flags)
{
    RawUDPPacketT *packet;
    uint32_t currtick = NetTick();
    int32_t pos, metalen;

    // make sure port is open
    if (ref->state != OPEN)
    {
        return(COMM_BADSTATE);
    }

    // see if output buffer full
    if ((ref->sndinp+ref->sndwid)%ref->sndlen == ref->sndout)
    {
        #if COMM_PRINT
        NetPrintf(("commudp: send overflow (connident=0x%08x)\n", ref->connident));
        #endif
        return(0);
    }

    // if metachunk enabled, adjust size 
    metalen = (ref->metatype == 1) ? RAW_METATYPE1_SIZE : 0;

    // return error for oversized packets
    if ((length+metalen) > (signed)(ref->sndwid-(sizeof(RawUDPPacketT)-sizeof(((RawUDPPacketT *)0)->body.data))))
    {
        NetPrintf(("commudp: oversized packet send (%d bytes)\n", length));
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

    // add packet to output buffer
    packet = (RawUDPPacketT *) &(ref->sndbuf[ref->sndinp]);
    packet->head.len = length;

    // copy the packet to the buffer
    memcpy(packet->body.data, buffer, length);
    // set the send time
    packet->head.when = currtick;

    // handle unreliable send
    if (flags & COMM_FLAGS_UNRELIABLE)
    {
        int32_t err = -1;
        NetCritEnter(&g_crit);
        
        // set up and send an unreliable packet
        packet->body.seq = ref->usndseq;
        packet->body.ack = _CommUDPSeqnDelta(ref->rcvseq, -1);
        if (flags & COMM_FLAGS_BROADCAST)
        {
            struct sockaddr peeraddr;
            memcpy(&peeraddr, &ref->peeraddr, sizeof(peeraddr));
            SockaddrInSetAddr(&peeraddr, 0xffffffff);
            err = _CommUDPWrite(ref, packet, &peeraddr, currtick);
        }
        else
        {
            err = _CommUDPWrite(ref, packet, &ref->peeraddr, currtick);
        }

        // calculate new sequence number
        if (++ref->usndseq >= RAW_PACKET_DATA)
        {
            ref->usndseq = RAW_PACKET_UNREL;
        }
        
        NetCritLeave(&g_crit);
        // send failed, return buffer-full(0) or error
        if (err < 0)
        {
            return(ref->snderr == SOCKERR_NONE ? 0 : err);
        }
        return(1);
    }

    // set the data fields
    packet->body.seq = ref->sndseq;
    ref->sndseq = _CommUDPSeqnDelta(ref->sndseq, 1);

    packet->body.ack = _CommUDPSeqnDelta(ref->rcvseq, -1);

    // add the packet to the queue
    ref->sndinp = (ref->sndinp+ref->sndwid) % ref->sndlen;
    pos = (((ref->sndinp+ref->sndlen)-ref->sndout)%ref->sndlen)/ref->sndwid;

    // try to send packet immediately if buffer is at least half empty
    if (pos < ref->common.maxout/2)
    {
        NetCritEnter(&g_crit);
        _CommUDPProcessOutput(ref, currtick);

        // process incoming if we missed event
        if (g_missed != 0)
        {
            // clear event counter before calling _CommUDPProcessThreadData to prevent possible unwanted recursion
            g_missed = 0;
            // process data as long as we get something
            while (_CommUDPThreadData(currtick) > 0)
                ;
            #if COMM_PRINT
            NetPrintf(("commudp: processing %d after send\n", g_missed));
            #endif
        }
        NetCritLeave(&g_crit);
    }

    // return buffer depth
    return(pos > 0 ? pos : 1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPPeek

    \Description
        Peek at waiting packet

    \Input *ref     - reference pointer
    \Input *target  - target buffer
    \Input length   - buffer length
    \Input *when    - tick received at

    \Output
        int32_t         - negative=nothing pending, else packet length

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPPeek(CommUDPRef *ref, void *target, int32_t length, uint32_t *when)
{
    RawUDPPacketT *packet;

    /* if this platform does not implement an async recv thread, poll for data,
       but only if we're not already being called from inside _CommUDPEvent() */
    #if !SOCKET_ASYNCRECVTHREAD
    if ((ref->rcvout == ref->rcvinp) && (g_inevent == 0))
    {
        _CommUDPEvent(NULL, 0, ref);
    }
    #endif

    // see if a packet is available
    if (ref->rcvout == ref->rcvinp)
        return(COMM_NODATA);

    // point to the packet
    packet = (RawUDPPacketT *) &(ref->rcvbuf[ref->rcvout]);

    // copy data?
    if (length > 0)
    {
        // make sure enough space is available
        if (length < packet->head.len)
            return(COMM_MINBUFFER);

        // copy over the data portion
        memcpy(target, packet->body.data, packet->head.len);
    }
        
    // get the timestamp
    if (when != NULL)
        *when = packet->head.when;
    
    // return packet data length
    return(packet->head.len);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPRecv

    \Description
        Receive a packet from the buffer

    \Input *ref     - reference pointer
    \Input *target  - target buffer
    \Input length   - buffer length
    \Input *when    - tick received at

    \Output
        int32_t     - negative=error, else packet length

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPRecv(CommUDPRef *ref, void *target, int32_t length, uint32_t *when)
{
    // use peek to remove the data
    int32_t len = CommUDPPeek(ref, target, length, when);
    if (len >= 0)
        ref->rcvout = (ref->rcvout+ref->rcvwid)%ref->rcvlen;
    // all done
    return(len);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPListen

    \Description
        Listen for a connection

    \Input *ref     - reference pointer
    \Input *addr    - port to listen on (only :port portion used)

    \Output
        int32_t     - negative=error, zero=ok

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPListen(CommUDPRef *ref, const char *addr)
{
    int32_t err, iListenPort, iConnPort;
    uint32_t poke;
    SocketT *sock;
    struct sockaddr bindaddr;

    // setup bind points
    SockaddrInit(&bindaddr, AF_INET);

    // parse at least port
    if ((SockaddrInParse2(&poke, &iListenPort, &iConnPort, addr) & 0x2) != 0x2)
    {
        return(COMM_BADADDRESS);
    }
    SockaddrInSetPort(&bindaddr, iListenPort);

    // create socket in case its needed
    sock = SocketOpen(AF_INET, SOCK_DGRAM, 0);
    if (sock == NULL)
    {
        return(COMM_NORESOURCE);
    }

    // let common code finish up
    err = _CommUDPListen0(ref, sock, &bindaddr);

    // set connection identifier
    _CommUDPSetConnID(ref, addr);

    NetPrintf(("commudp: listen err=%d, bind=%d, connident=0x%08x\n", err, iListenPort, ref->connident));

    // see if we should setup peer address
    if ((err == 0) && (poke != 0))
    {
        if (iConnPort == 0)
        {
            iConnPort = iListenPort+1;
        }

        NetPrintf(("commudp: poke address=%08x:%d\n", poke, iConnPort));
        SockaddrInit(&ref->peeraddr, AF_INET);
        SockaddrInSetAddr(&ref->peeraddr, poke);
        SockaddrInSetPort(&ref->peeraddr, iConnPort);
    }

    // clear any previous receive errors
    ref->snderr = 0;
    return(err);
}

/*F*************************************************************************************************/
/*!
    \Function    CommUDPConnect

    \Description
        Initiate a connection to a peer

    \Input *ref     - reference pointer
    \Input *addr    - address in ip-address:port form

    \Output
        int32_t     - negative=error, zero=ok

    \Notes
        Does not currently perform dns translation

    \Version 12/04/00 (GWS)

*/
/*************************************************************************************************F*/
int32_t CommUDPConnect(CommUDPRef *ref, const char *addr)
{
    int32_t err, iConnPort, iListenPort;
    uint32_t uAddr;
    CommUDPRef *find;
    SocketT *sock;
    struct sockaddr bindaddr, glueaddr, peeraddr;

    // setup target info
    SockaddrInit(&peeraddr, AF_INET);
    SockaddrInit(&bindaddr, AF_INET);

    // parse addr - make sure we have at least an address and port
    err = SockaddrInParse2(&uAddr, &iListenPort, &iConnPort, addr);
    if ((err & 3) != 3)
    {
        return(COMM_BADADDRESS);
    }

    // if we don't have an alternate connect port, connect=listen
    if (iConnPort == 0)
    {
        iConnPort = iListenPort++;
    }

    // set listen port
    SockaddrInSetPort(&bindaddr, iListenPort);

    // set connection identifier
    _CommUDPSetConnID(ref, addr);
    NetPrintf(("commudp: connect addr=%08x, bind=%d, peer=%d connident=0x%08x\n",
        uAddr, iListenPort, iConnPort, ref->connident));

#if defined(DIRTYCODE_PC)
    /*
    the following line will fill in bindaddr with the IP addr previously set with
    SocketControl(... 'ladr' ...). If that selector was never used, then the IP
    address field of bindaddr will just be filled with 0.
    note: only required for multi-homed system (PC)
    */
    SocketInfo(NULL, 'ladr', 0, &bindaddr, sizeof(bindaddr));
#endif

    // see if there is an existing socket bound to this port
    for (find = g_link, sock = NULL; find != NULL; find = find->link)
    {
        // dont check ourselves or unbound sockets
        if ((find == ref) || (find->socket == NULL))
            continue;

        // see where this endpoint is bound
        if (SocketInfo(find->socket, 'bind', 0, &glueaddr, sizeof(glueaddr)) < 0)
            continue;

        /*
        see if the socket can be reused
            if the endpoint is virtual: we only compare the ports
            if the endpoint is not virtual:
                if bindaddr (specifying what we want to bind to) has ipaddr=0, we only compare the ports
                otherwise we compare the full sockaddr structs (i.e. family, port, addr)
        */
        if (SockaddrInGetPort(&bindaddr) == SockaddrInGetPort(&glueaddr))
        {
            if ((SocketInfo(find->socket, 'virt', 0, NULL, 0) == TRUE) ||
                (SockaddrInGetAddr(&bindaddr) == 0) ||
                (SockaddrCompare(&bindaddr, &glueaddr) == 0))
            {
                // share the socket
                sock = find->socket;
                break;
            }
        }
    }

    // create the actual socket
    if (sock == NULL)
    {
        sock = SocketOpen(AF_INET, SOCK_DGRAM, 0);
        if (sock == NULL)
        {
            return(COMM_NORESOURCE);
        }
        
        // bind socket
        if ((err = SocketBind(sock, &bindaddr, sizeof(bindaddr))) < 0)
        {
            NetPrintf(("commudp: bind to %d failed with %d\n", iListenPort, err));
            SockaddrInSetPort(&bindaddr, 0);
            if ((err = SocketBind(sock, &bindaddr, sizeof(bindaddr))) < 0)
            {
                NetPrintf(("commudp: bind to 0 failed with result %d\n", err));
                SocketClose(sock);
                return(COMM_UNEXPECTED);
            }
            else
            {
                SocketInfo(sock, 'bind', 0, &bindaddr, sizeof(bindaddr));
                NetPrintf(("commudp: bound socket to port %d\n", SockaddrInGetPort(&bindaddr)));
            }
        }
    }

    // set connect sockaddr
    SockaddrInSetAddr(&peeraddr, uAddr);
    SockaddrInSetPort(&peeraddr, iConnPort);

    // clear any previous receive errors
    ref->snderr = 0;

    // pass to common handler
    return(_CommUDPConnect0(ref, sock, &peeraddr));
}
