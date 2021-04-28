/*H*************************************************************************************************/
/*!

    \File    commser.c

    \Description
        Serial comm driver

    \Notes
        This is a simple serial transport class which incorporates
        the notions of virtual connections and error free transfer. The
        protocol is optimized for use in a real-time environment where
        latency is more important than bandwidth. Overhead is low with
        the protocol adding only 8 bytes per packet on top of that
        required by Ser itself.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 1999-2003.  ALL RIGHTS RESERVED.

    \Version    1.0        02/15/99 (GWS) First Version
    \Version    1.1        02/25/99 (GWS) Alpha release
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <windows.h>

#include "dirtysock.h"
#include "dirtymem.h"
#include "commall.h"
#include "commser.h"

/*** Defines ***************************************************************************/

//! define error free protocol packet types
enum
{
    RAW_PACKET_INIT = 1,        //!< initiate a connection
    RAW_PACKET_CONN,            //!< confirm a connection
    RAW_PACKET_DISC,            //!< terminate a connection
    RAW_PACKET_NAK,             //!< force resend of lost data
    RAW_PACKET_DATA = 100       //!< initial data packet sequence number
};

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! raw protocol packet format
typedef struct
{
    //! packet header which is not sent/received (this data is used internally)
    struct
    {
        int32_t len;                //!< variable data len (-1=none) (used int32_t for alignment)
        uint32_t when;      //!< tick when a packet was received
    } head;
    //! packet body which is sent/received
    struct
    {
        uint32_t seq;       //!< packet type or seqeunce number
        uint32_t ack;       //!< acknolwedgement of last packet
        char data[2048];        //!< user data
    } body;
} RawSerPacketT;

//! private module storage
typedef struct CommSerRef
{
    //! common header
    CommRef common;
    //! linked list of all instances
    CommSerRef *link;
    //! comm handle
    HANDLE handle;
    //! overlap variables
    OVERLAPPED rdover;
    OVERLAPPED wtover;
    OVERLAPPED wrover;
    DWORD rdlen;
    DWORD wtlen;
    DWORD wrlen;
    //! owning thread id
    DWORD notify;

    //! port state
    volatile enum
    {
        DEAD, IDLE, CONN, LIST, OPEN, HOLD, STOP, CLOSE, RESET, QUIT
    } state;

    //! identifier to keep from getting spoofed
    uint32_t connident;

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

    //! private packet buffer (for assembly)
    unsigned char rawbuf[2048];
    //! index into buffer
    int32_t rawlen;

    //! private send structures
    unsigned char quebuf[2][2048];
    int32_t quemax;
    int32_t quelen;
    int32_t queref;
    
    //! tick at which last packet was sent
    uint32_t sendtick;
    //! tick at which last packet was received
    uint32_t recvtick;

    //! semaphore to synchronize thread access
    CRITICAL_SECTION crit;
    //! callback in progress blocker
    volatile int32_t callback;
    //! pending event indicator
    int32_t gotevent;
    //! callback routine pointer
    void (*callproc)(CommRef *ref, int32_t event);
} CommSerRef;

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/


/*F*************************************************************************************************/
/*!
    \Function _CommSerResetTransfer
    
    \Description
        Reset the transfer state
        
    \Input *ref    - module ref
    
    \Output
        None.
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static void _CommSerResetTransfer(CommSerRef *ref)
{
    // reset the send queue
    ref->sndinp = 0;
    ref->sndout = 0;
    ref->sndnxt = 0;
    // reset the sequence number
    ref->sndseq = RAW_PACKET_DATA+1;

    // reset the receive queue
    ref->rcvinp = 0;
    ref->rcvout = 0;
    // reset the packet sequence number
    ref->rcvseq = RAW_PACKET_DATA+1;

    // make sendtick really old (in protocol terms)
    ref->sendtick = GetTickCount()-5000;
    return;
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerCheck
    
    \Description
        Perform checksum on data block
    
    \Input *buf         - buffer pointer
    \Input len          - data length
    
    \Output
        uint16_t  - checksum value (16-bit)
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static uint16_t _CommSerCheck(const unsigned char *buf, int32_t len)
{
    uint16_t checksum = 0;

    // quick calc on data
    while (len > 0) {
        checksum = (checksum * 13) + *buf++;
        --len;
    }

    // return the value
    return(checksum);
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerRead
    
    \Description
        Read a packet from the peer
    
    \Input *ref     - module ref
    \Input *packet  - packet pointer
    
    \Output
        int32_t         - >=0 = got a packet
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t _CommSerRead(CommSerRef *ref, RawSerPacketT *packet)
{
    int32_t need;
    int32_t len = ref->rawlen, pktlen;
    unsigned char *buf = ref->rawbuf;
    unsigned char *ptr;

    // force loop
    for (need = 0;;) {
        // see if need to add data
        if (need || (len < 8)) {
            // setup overlap structure
            ref->rdover.Internal = 0;
            ref->rdover.InternalHigh = 0;
            ref->rdover.Offset = 0;
            ref->rdover.OffsetHigh = 0;
            // read the pending data
            if (!ReadFile(ref->handle, buf+len, sizeof(ref->rawbuf)-len, &ref->rdlen, &ref->rdover) &&
                (GetLastError() == ERROR_IO_PENDING))
                GetOverlappedResult(ref->handle, &ref->rdover, &ref->rdlen, TRUE);
            // see if we really got something
            if (ref->rdlen > 0) {
                // add to the buffer
                len += ref->rdlen;
                need = 0;
                continue;
            }

            // see if a current comm query is still running
            if (WaitForSingleObject(ref->wtover.hEvent, 0) == WAIT_TIMEOUT) {
                // update the buffer pointer
                ref->rawlen = len;
                return(0);
            }

            // setup overlap structure
            ref->rdover.Internal = 0;
            ref->rdover.InternalHigh = 0;
            ref->rdover.Offset = 0;
            ref->rdover.OffsetHigh = 0;
            // wait for an end of packet marker to arrive
            if ((WaitCommEvent(ref->handle, &ref->wtlen, &ref->wtover) == 0) &&
                (GetLastError() == ERROR_IO_PENDING)) {
                // update the buffer pointer
                ref->rawlen = len;
                return(0);
            }

            // since we did not get an io_pending, assume got new data
            need = 1;
            continue;
        }

        // make sure header is ok
        if ((buf[0] != 'G') || (buf[1] != 'S') || (buf[2] != (buf[3]^255))) {
#if COMM_PRINT
            if (buf[0] != 0)
                OutputDebugStringA("bogus packet\n");
#endif
            // skip this header
            ++buf;
            --len;
            // locate next potential header
            for (; len > 0; ++buf, --len) {
                if (buf[0] != 'G')
                    continue;
                if ((len > 1) && (buf[1] != 'S'))
                    continue;
                if ((len > 3) && (buf[2] != (buf[3]^255)))
                    continue;
                // found some kind of match
                break;
            }
            // shift back the buffer
            if (len > 0)
                memcpy(ref->rawbuf, buf, len);
            buf = ref->rawbuf;
            continue;
        }

        pktlen = sizeof(packet->body)-sizeof(packet->body.data)+buf[2];

        // see if packet is complete
        if (len < 8+pktlen) {
            need = 1;
            continue;
        }

        // evaluate the packet trailer
        ptr = buf+4+pktlen;
        if ((ptr[2] != '\r') || (ptr[3] != '\n') ||
            (_CommSerCheck(buf+4, pktlen) != ((ptr[0]<<0)|(ptr[1]<<8)))) {
            // force new header check
            buf[0] = 0;
            need = 0;
#if COMM_PRINT
            OutputDebugStringA("bogus packet\n");
#endif
            continue;
        }

        // extract data from framing
        memcpy(&packet->body, buf+4, pktlen);
        packet->head.len = pktlen-(sizeof(packet->body)-sizeof(packet->body.data));
        packet->head.when = GetTickCount();

        // shift back any remaining data
        ref->rawlen = len-(8+pktlen);
        if (ref->rawlen != 0)
            memcpy(ref->rawbuf, ptr+4, ref->rawlen);
        return(1);
    }
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerWrite
    
    \Description
        Send a packet to the peer
    
    \Input *ref     - module ref
    \Input *packet  - packet pointer
    
    \Output
        int32_t         - negative=error, zero=ok
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t _CommSerWrite(CommSerRef *ref, RawSerPacketT *packet)
{
    DWORD len;
    uint16_t checksum;
    unsigned char *frame;

    // see if we have a packet to queue
    if (packet != NULL) {
        // figure full packet length (nak/ack fields + variable data)
        len = sizeof(packet->body)-sizeof(packet->body.data)+packet->head.len;

        // see if we can fit data in buffer
        if (ref->quemax-ref->quelen < (signed)len+8)
            return(-1);

        // point to the data target
        frame = &ref->quebuf[ref->queref][ref->quelen];

        // put in start marker
        frame[0] = 'G';
        frame[1] = 'S';

        // encode the user data size
        frame[2] = (unsigned char) packet->head.len;
        frame[3] = (unsigned char) (packet->head.len ^ 255);

        // copy data
        memcpy(frame+4, &packet->body, len);

        // put in checksum
        checksum = _CommSerCheck(frame+4, len);
        frame[4+len+0] = (unsigned char) (checksum >> 0);
        frame[4+len+1] = (unsigned char) (checksum >> 8);

        // add in the trailing marker
        frame[4+len+2] = '\r';
        frame[4+len+3] = '\n';

        // if we added first item to buffer, break any comm wait
        if (ref->quelen == 0)
            SetCommMask(ref->handle, EV_RXFLAG);
        // calc new buffer size
        ref->quelen += len+8;
        // record the attempt time
        ref->sendtick = GetTickCount();
    }

    // see if there is data to send
    if (ref->quelen == 0)
        return(0);

    // see if the previous send has finished
    if (WaitForSingleObject(ref->wrover.hEvent, 0) == WAIT_TIMEOUT)
        return(0);

    // start the new send
    WriteFile(ref->handle, &ref->quebuf[ref->queref], ref->quelen,
        &ref->wrlen, &ref->wrover);

    // add new data to buffer we just finished with
    ref->queref ^= 1;
    ref->quelen = 0;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerClose
    
    \Description
        Close the connection
    
    \Input *ref     - module ref
    
    \Output
        int32_t         - negative=error, zero=ok
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t _CommSerClose(CommSerRef *ref)
{
    RawSerPacketT packet;

    // see if we are even connected
    if ((ref->state == IDLE) || (ref->state == CLOSE))
        return(0);

    // send a disconnect message
    packet.head.len = 0;
    packet.body.seq = RAW_PACKET_DISC;
    packet.body.ack = ref->connident;
    _CommSerWrite(ref, &packet);

    // set to disconnect state
    ref->connident = 0;
    ref->state = CLOSE;
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerProcessSetup
    
    \Description
        Process a setup/teardown request
    
    \Input *ref     - module ref
    \Input *packet  - requesting packet
    \Input notify   - main thread id
    
    \Output
        None
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static void _CommSerProcessSetup(CommSerRef *ref, RawSerPacketT *packet, DWORD notify)
{
    // all setup packets are zero length and carry the ident value
    if ((packet->head.len != 0) || (packet->body.ack != ref->connident))
        return;
    
    // response to connection query
    if (packet->body.seq == RAW_PACKET_INIT) {
        // send a response
        packet->body.seq = RAW_PACKET_CONN;
        _CommSerWrite(ref, packet);
        return;
    }

    // response to a connect confirmation
    if (packet->body.seq == RAW_PACKET_CONN) {
        // change to open if not already there
        if (ref->state == CONN)
            ref->state = OPEN;
        return;
    }

    // response to disconnect message
    if (packet->body.seq == RAW_PACKET_DISC) {
        // close the connection
        if (ref->state == OPEN)
            ref->state = CLOSE;
    }

    // should not get here
    return;
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerProcessInit

    \Description
        Initiate a connection

    \Input *ref     - module ref

    \Output
        None

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static void _CommSerProcessInit(CommSerRef *ref)
{
    RawSerPacketT packet;

    // generate a unique connection identifier
    while (ref->connident == 0) {
#if defined(_WIN64)
        ref->connident =  (uint32_t)(((uint64_t)ref) ^ GetTickCount());
#else
        ref->connident = (((DWORD)ref) ^ GetTickCount());
#endif
    }

    // send a connection initiate request
    packet.head.len = 0;
    packet.body.seq = RAW_PACKET_INIT;
    packet.body.ack = ref->connident;
    _CommSerWrite(ref, &packet);
    return;
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerProcessAlive
    
    \Description
        Send a keepalive packet
    
    \Input *ref     - module ref
    
    \Output
        None
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static void _CommSerProcessAlive(CommSerRef *ref)
{
    RawSerPacketT packet;

    // set our packet number
    packet.body.seq = ref->sndseq;
    // acknowledge packet prior to one we are waiting for
    packet.body.ack = ref->rcvseq-1;
    // no data means keepalive
    packet.head.len = 0;
    // send it
    _CommSerWrite(ref, &packet);
    return;
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerProcessOutput
    
    \Description
        Send data packet(s)
    
    \Input *ref     - module ref
    
    \Output
        None
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static void _CommSerProcessOutput(CommSerRef *ref)
{
    int32_t index;
    int32_t limit = 2048;

    // figure unacked data length
    for (index = ref->sndout; index != ref->sndnxt;
        index = (index+ref->sndwid)%ref->sndlen) {
        RawSerPacketT *buffer = (RawSerPacketT *) &ref->sndbuf[index];
        // count down the limit
        limit -= buffer->head.len;
    }
    
    // try and send some packets
    while (ref->sndnxt != ref->sndinp) {
        RawSerPacketT *buffer = (RawSerPacketT *) &ref->sndbuf[ref->sndnxt];
        // see if next packet fits bandwidth limitations
        if ((ref->sndnxt != ref->sndout) && (buffer->head.len > limit))
            return;
        // count the bandwidth for this packet
        limit -= buffer->head.len;
        // update the ack value (one less than one we are waiting for)
        buffer->body.ack = ref->rcvseq-1;
        // use the callback if present
        if (ref->common.SendCallback != NULL)
            ref->common.SendCallback((CommRef *)ref, buffer->body.data, buffer->head.len, 0);
        // go ahead and send the packet
        if (_CommSerWrite(ref, buffer) < 0)
            break;
        // advance the buffer
        ref->sndnxt = (ref->sndnxt+ref->sndwid) % ref->sndlen;
    }
    return;
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerProcessFlow
    
    \Description
        Perform flow control based on ack/nak packets
    
    \Input *ref     - module ref
    \Input *packet  - incoming packet
    
    \Output
        None
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static void _CommSerProcessFlow(CommSerRef *ref, RawSerPacketT *packet)
{
    // grab the ack point and nak flag
    int32_t nak = (packet->body.seq == RAW_PACKET_NAK);
    uint32_t ack = (nak ? packet->body.ack-1 : packet->body.ack);

    // advance ack point
    while (ref->sndout != ref->sndinp) {
        RawSerPacketT *buffer = (RawSerPacketT *) &ref->sndbuf[ref->sndout];
        // see if this packet has been acked
        if (ack < buffer->body.seq)
            break;
        // if about to send this packet, skip to next
        if (ref->sndnxt == ref->sndout)
            ref->sndnxt = (ref->sndnxt+ref->sndwid) % ref->sndlen;
        // remove the packet from the queue
        ref->sndout = (ref->sndout+ref->sndwid) % ref->sndlen;
    }

    // reset send point for nak
    if (nak) {
        // reset send point
        ref->sndnxt = ref->sndout;
        // immediate restart
        _CommSerProcessOutput(ref);
    }

    // all done
    return;
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerProcessFlow
    
    \Description
        Process incoming data packet
    
    \Input *ref     - module ref
    \Input *packet  - incoming packet
    \Input notify   - main thread id
    
    \Output
        None
            
    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static void _CommSerProcessInput(CommSerRef *ref, RawSerPacketT *packet, DWORD notify)
{
    RawSerPacketT *buffer;

    // see if room in buffer for packet
    if ((ref->rcvinp+ref->rcvwid)%ref->rcvlen == ref->rcvout) {
        // no room in buffer -- just drop packet
        // could nak, but would generate lots of
        // network activity with no result
        return;
    }

    // ignore old packets
    if (packet->body.seq < ref->rcvseq)
        return;

    // immediate nak for missing packets
    if (packet->body.seq > ref->rcvseq) {
        // send a nak packet
        packet->body.seq = RAW_PACKET_NAK;
        packet->body.ack = ref->rcvseq;
        packet->head.len = 0;
        _CommSerWrite(ref, packet);
        return;
    }

    // no further processing for empty (ack) packets
    if (packet->head.len == 0)
        return;

    // add the packet to the buffer
    buffer = (RawSerPacketT *) &ref->rcvbuf[ref->rcvinp];
    memcpy(buffer, packet, ref->rcvwid);

    // limit access to receive until callback complete
    ref->callback += 1;
    ref->rcvinp = (ref->rcvinp+ref->rcvwid) % ref->rcvlen;
    ref->rcvseq += 1;
    // do receive callback
    if (ref->common.RecvCallback != NULL)
        ref->common.RecvCallback((CommRef *)ref, buffer->body.data, buffer->head.len, buffer->head.when);
    // make receive available again
    ref->callback -= 1;
    // mark as having an event
    ref->gotevent |= 1;
    return;
}

/*F*************************************************************************************************/
/*!
    \Function _CommSerThread

    \Description
        Main worker thread

    \Input ref     - reference pointer

    \Output
        int32_t     - zero

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
static int32_t _CommSerThread(CommSerRef *ref)
{
    RawSerPacketT packet;
    uint32_t tick;

    // a simple blocking receive loop with timeout
    while (ref->state != QUIT) {
        // handle reset state
        if (ref->state == RESET)
            ref->state = IDLE;
        // just wait if idle or dead
        if ((ref->state == IDLE) || (ref->state == DEAD) ||
            (ref->state == CLOSE) || (ref->state == HOLD)) {
            // is closed with output pending, process the output
            if ((ref->state == CLOSE) && (ref->quelen > 0)) {
                // take care of pending writes
                EnterCriticalSection(&(ref->crit));
                _CommSerWrite(ref, NULL);
                LeaveCriticalSection(&(ref->crit));
                Sleep(25);
                continue;
            }
            // just give up some time
            Sleep(50);
            continue;
        }

        // see if we need to block
        if (WaitForSingleObject(ref->wtover.hEvent, 0) == WAIT_TIMEOUT) {
            int32_t hcount = 0;
            HANDLE hlist[2];
            // add the waitcomm event
            hlist[hcount++] = ref->wtover.hEvent;
            // also add output event if needed
            if ((ref->quelen > 0) && (WaitForSingleObject(ref->wrover.hEvent, 0) == WAIT_TIMEOUT))
                hlist[hcount++] = ref->wrover.hEvent;
            // do the wait
            WaitForMultipleObjects(hcount, hlist, FALSE, 100);
        }

        // set to no packet waiting
        packet.head.len = -1;
        // check for packet
        _CommSerRead(ref, &packet);
        // get current tick
        tick = GetTickCount();

        // take control of resource
        EnterCriticalSection(&(ref->crit));

        // take care of pending writes
        _CommSerWrite(ref, NULL);

        // check for connection timeout
        if ((ref->state == OPEN) && (NetTickDiff(tick, ref->recvtick) > 45*1000)) {
            // disconnect due to timeout
            ref->state = STOP;
        }

        // check for connection shutdown
        if (ref->state == STOP) {
            // close the port
            _CommSerClose(ref);
        }

        // see if any output for this port
        if ((ref->state == OPEN) && (ref->sndnxt != ref->sndinp)) {
            _CommSerProcessOutput(ref);
            tick = GetTickCount();
        }
        // see if we are trying to connect
        if ((ref->state == CONN) && (tick - ref->sendtick > 1000)) {
            // send an initiate
            _CommSerProcessInit(ref);
            tick = GetTickCount();
        }
        // see if we need a keepalive
        if ((ref->state == OPEN) && (ref->sndnxt == ref->sndinp) &&
            (tick - ref->sendtick > 1000)) {
            // send a keepalive
            _CommSerProcessAlive(ref);
            tick = GetTickCount();
        }

        // see if packet belongs to someone
        if ((packet.head.len >= 0) && 
            ((ref->state == OPEN) || (ref->state == CONN))) {
            // process an incoming packet
            if (packet.body.seq == RAW_PACKET_NAK) {
                // resend the missing data
                _CommSerProcessFlow(ref, &packet);
            } else if ((packet.body.seq == RAW_PACKET_INIT) ||
                (packet.body.seq == RAW_PACKET_CONN) ||
                (packet.body.seq == RAW_PACKET_DISC)) {
                // handle connection setup/teardown
                _CommSerProcessSetup(ref, &packet, ref->notify);
            } else {
                // process the ack info
                _CommSerProcessFlow(ref, &packet);
                // save the data
                _CommSerProcessInput(ref, &packet, ref->notify);
            }
            // mark packet as processed
            packet.head.len = -1;
            ref->recvtick = GetTickCount();
        }

        // see if this is a listen request
        if ((ref->state == LIST) && (packet.head.len == 0) &&
            (packet.body.ack != 0) && (packet.body.seq == RAW_PACKET_INIT)) {
            // save the connection identifier
            ref->connident = packet.body.ack;
            ref->state = OPEN;
            // process the packet
            _CommSerProcessSetup(ref, &packet, ref->notify);
            // mark packet as processed
            packet.head.len = -1;
            ref->recvtick = GetTickCount();
        }

        // done with this ref
        LeaveCriticalSection(&(ref->crit));

        // see if a pending event
        if (ref->gotevent != 0) {
            // notify user
            if (ref->callproc != NULL)
                ref->callproc((CommRef *)ref, ref->gotevent);
            ref->gotevent = 0;
        }
    }

    // done with client
    ref->state = IDLE;
    return(0);
}


/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    CommSerConstruct
    
    \Description
        Construct the class
    
    \Input maxwid       - max record width
    \Input maxinp       - input packet buffer size
    \Input maxout       - output packet buffer size
    
    \Output
        CommSerRef *    - pointer to class, or NULL

    \Notes
        Creates worker thread on construction.

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
CommSerRef *CommSerConstruct(int32_t maxwid, int32_t maxinp, int32_t maxout)
{
    DWORD pid;
    HANDLE thread;
    CommSerRef *ref;
    int32_t iMemGroup;
    void *pMemGroupUserData;
#if defined(_WIN64)
    int64_t tempHandle = -1;
#else
    int32_t tempHandle = -1;
#endif

    // Query mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate class storage
    ref = DirtyMemAlloc(sizeof(*ref), COMMSER_MEMID, iMemGroup, pMemGroupUserData);
    if (ref == NULL)
        return(NULL);
    memset(ref, 0, sizeof(*ref));
    ref->common.memgroup = iMemGroup;
    ref->common.memgrpusrdata = pMemGroupUserData;

    // initialize the callback routines
    ref->common.Construct = (CommAllConstructT *)CommSerConstruct;
    ref->common.Destroy = (CommAllDestroyT *)CommSerDestroy;
    ref->common.Resolve = (CommAllResolveT *)CommSerResolve;
    ref->common.Unresolve = (CommAllUnresolveT *)CommSerUnresolve;
    ref->common.Listen = (CommAllListenT *)CommSerListen;
    ref->common.Unlisten = (CommAllUnlistenT *)CommSerUnlisten;
    ref->common.Connect = (CommAllConnectT *)CommSerConnect;
    ref->common.Unconnect = (CommAllUnconnectT *)CommSerUnconnect;
    ref->common.Callback = (CommAllCallbackT *)CommSerCallback;
    ref->common.Status = (CommAllStatusT *)CommSerStatus;
    ref->common.Tick = (CommAllTickT *)CommSerTick;
    ref->common.Send = (CommAllSendT *)CommSerSend;
    ref->common.Peek = (CommAllPeekT *)CommSerPeek;
    ref->common.Recv = (CommAllRecvT *)CommSerRecv;

    // remember max packet width
    ref->common.maxwid = maxwid;
    ref->common.maxinp = maxinp;
    ref->common.maxout = maxout;

    // reset access to shared resources
    InitializeCriticalSection(&ref->crit);

    // allocate the buffers
    ref->rcvwid = sizeof(RawSerPacketT)-sizeof(((RawSerPacketT *)0)->body.data)+maxwid;
    ref->rcvlen = ref->rcvwid * maxinp;
    ref->rcvbuf = (char *)DirtyMemAlloc(ref->rcvlen, COMMSER_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    ref->sndwid = sizeof(RawSerPacketT)-sizeof(((RawSerPacketT *)0)->body.data)+maxwid;
    ref->sndlen = ref->sndwid * maxout;
    ref->sndbuf = (char *)DirtyMemAlloc(ref->sndlen, COMMSER_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);

    // reset packet buffer
    ref->rawlen = 0;
    ref->queref = 0;
    ref->quelen = 0;
    ref->quemax = sizeof(ref->quebuf[0]);

    // reset the state
    ref->state = IDLE;
    ref->connident = 0;
    ref->notify = GetCurrentThreadId();

    // setup resources
    ref->handle = (HANDLE)tempHandle;
    ref->rdover.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    ref->wtover.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    ref->wrover.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);

    // start the worker thread
    thread = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)&_CommSerThread, (LPVOID)ref, 0, &pid);
    if (thread == NULL)
        return(NULL);
    // we deal with real-time events
    SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST);
    CloseHandle(thread);

    // return endpoint reference
    return(ref);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerDestroy
    
    \Description
        Destruct the class
    
    \Input *ref     - module ref
    
    \Output
        None.

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
void CommSerDestroy(CommSerRef *ref)
{
#if defined(_WIN64)
    int64_t tempHandle = -1;
#else
    int32_t tempHandle = -1;
#endif

    // if port is open, close it
    if (ref->state == OPEN)
    {
        ref->state = STOP;
        // wait for thread to close port
        while (ref->state == STOP)
        {
            Sleep(0);
        }
    }

    // signal thread to quit
    ref->state = QUIT;
    while (ref->state != IDLE)
    {
        Sleep(0);
    }

    // free the port if in use
    if (ref->handle != (HANDLE)tempHandle)
    {
        // kill any pending waitcommstate
        SetCommMask(ref->handle, EV_RXFLAG);
        // kill all pending reads/writes
        PurgeComm(ref->handle, PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
        // close the handle
        CloseHandle(ref->handle);
    }

    // done with event objects
    CloseHandle(ref->rdover.hEvent);
    CloseHandle(ref->wtover.hEvent);
    CloseHandle(ref->wrover.hEvent);

    // release resources
    DirtyMemFree(ref->rcvbuf, COMMSER_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    DirtyMemFree(ref->sndbuf, COMMSER_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
    DirtyMemFree(ref, COMMSER_MEMID, ref->common.memgroup, ref->common.memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerCallback

    \Description
        Set upper layer callback

    \Input *ref         - reference pointer
    \Input *callback    - socket generating callback

    \Output
        None.

    \Version    1.0        03/10/03 (JLB) Copied from CommSRPCallback()
*/
/*************************************************************************************************F*/
void CommSerCallback(CommSerRef *ref, void (*callback)(CommRef *ref, int32_t event))
{
    //ref->callproc = callback;
    //ref->gotevent |= 2;
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerResolve

    \Description
        Resolve an address

    \Input *ref         - module ref
    \Input *addr        - resolve address
    \Input *buf         - target buffer
    \Input len          - target length (min 64 bytes)
    \Input div          - divider char

    \Output
        int32_t             - <0=error, 0=complete (COMM_NOERROR), >0=in progress (COMM_PENDING)

    \Notes
        Target list is always double null terminated allowing null
        to be used as the divider character if desired. when COMM_PENDING
        is returned, target buffer is set to "~" until completion.

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerResolve(CommSerRef *ref, const char *addr, char *buf, int32_t len, char div)
{
    int32_t index;
    HANDLE hand;
    char *list[] = { "COM1", "COM2", "COM3", "COM4", NULL };
    char *org = buf;
#if defined(_WIN64)
    int64_t tempHandle = -1;
#else
    int32_t tempHandle = -1;
#endif


    // default to error
    buf[0] = '*';
    buf[1] = 0;
    buf[2] = 0;

    // handle null request special
    if ((addr == NULL) || (addr[0] == 0))
    {
        return(COMM_BADPARM);
    }

    // basic buffer size check
    if ((len < 64) || ((unsigned)len < strlen(addr)+2))
    {
        return(COMM_MINBUFFER);
    }

    // the only thing we resolve is localhost
    if (strcmp(addr, "localhost") != 0)
    {
        // pass everything else back as-is
        wsprintf(buf, "%s%c", addr, 0);
        return(1);
    }

    // figure out valid comm ports
    for (index = 0; list[index] != NULL; ++index)
    {
        // try and open the device
        hand = CreateFile(list[index], GENERIC_READ|GENERIC_WRITE, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL);
        if (hand == (HANDLE)tempHandle)
        {
            continue;
        }
        // done with port
        CloseHandle(hand);
        // add to the list
        if (buf != org)
        {
            *buf++ = div;
        }
        buf += wsprintf(buf, "%s", list[index]);
    }

    // terminate the list
    *buf++ = 0;
    *buf++ = 0;
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerUnresolve

    \Description
        Stop the resolver

    \Input *ref         - reference pointer

    \Output
        None.

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
void CommSerUnresolve(CommSerRef *ref)
{
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerRecover

    \Description
        Recover a broken connection

    \Input *ref         - reference pointer
    \Input *addr        - port to open on

    \Output
        int32_t             - negative=error, zero=ok

    \Notes
        Not an advertised function; used internally and by tapi driver

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerRecover(CommSerRef *ref, const char *addr)
{
    char device[32];
    COMMTIMEOUTS timeouts = { (unsigned)-1, 0, 0, 0, 0 };
#if defined(_WIN64)
    int64_t tempHandle = -1;
    int64_t tapi = 0;
#else
    int32_t tempHandle = -1;
    int32_t tapi = 0;
#endif
    union
    {
        COMMCONFIG val;
        char data[4096];
    } config;
    DCB *dcb = &config.val.dcb;

    // put into hold mode
    if ((addr == NULL) && (ref->state == OPEN))
    {
        ref->state = HOLD;
        // kill any pending waitcommstate
        SetCommMask(ref->handle, EV_RXFLAG);
        // kill all pending reads/writes
        PurgeComm(ref->handle, PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
        // close the handle
        CloseHandle(ref->handle);
        ref->handle = (HANDLE)tempHandle;
        return(COMM_NOERROR);
    }

    // make sure in valid state
    if ((ref->state != IDLE) && (ref->state != HOLD))
    {
        return(COMM_INPROGRESS);
    }

    // reset to unresolved
    if (ref->state == IDLE)
    {
        _CommSerResetTransfer(ref);
    }

    // extract the device from the address
    strnzcpy(device, addr, sizeof(device));
    if (strchr(device, ':'))
    {
        (strchr(device, ':'))[0] = 0;
        addr = strchr(addr, ':')+1;
    }
    else
    {
        addr = "";
    }

    // see if this is a tapi call
    if (strncmp(device, "TAPI", 4) == 0)
    {
        char *s = device+4;
        for (tapi = 0; (*s >= '0') && (*s <= '9'); ++s)
        {
            tapi = (tapi * 10) + (*s & 15);
        }
        ref->handle = (HANDLE)tapi;
    }
    else
    {
        // just use the address as-is
        ref->handle = CreateFile(device, GENERIC_READ|GENERIC_WRITE, 0, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED, NULL);
        if (ref->handle == (HANDLE)tempHandle)
        {
            return(COMM_NORESOURCE);
        }
    }

    // setup the buffering
    SetupComm(ref->handle, 8192, 4096);

    // setup for no blocking during reads
    SetCommTimeouts(ref->handle, &timeouts);

    // set the flag character
    SetCommMask(ref->handle, EV_RXFLAG);

    // config the port -- must use SetCommConfig for TAPI and SetCommState for Serial
    // (for reasons unknown, using SetCommConfig for serial under win95 locks the machine)
    if (tapi)
    {
        int32_t len = sizeof(config);
        GetCommConfig(ref->handle, &config.val, (LPDWORD)&len);
        // setup attributes needed for serial processing
        dcb->EvtChar = '\n';            // the event character
        dcb->fBinary = TRUE;            // need binary mode (all win95 supports)
        dcb->fNull = FALSE;             // dont strip nulls
        dcb->fOutX = FALSE;             // dont want xon/xoff flow control
        dcb->fInX = FALSE;              // dont want xon/xoff flow control
        dcb->fAbortOnError = FALSE;     // hmmm.. dont know what this does
        // update the config
        SetCommConfig(ref->handle, &config.val, len);
    }
    else
    {
        // get current config
        dcb->DCBlength = sizeof(*dcb);
        GetCommState(ref->handle, dcb);
        // setup attributes needed for processing
        dcb->EvtChar = '\n';            // the event character
        dcb->fBinary = TRUE;            // need binary mode (all win95 supports)
        dcb->fNull = FALSE;             // dont strip nulls
        dcb->fOutX = FALSE;             // dont want xon/xoff flow control
        dcb->fInX = FALSE;              // dont want xon/xoff flow control
        dcb->fAbortOnError = FALSE;     // hmmm.. dont know what this does
        // setup common serial settings
        dcb->ByteSize = 8;              // 8n1
        dcb->fParity = TRUE;            // 8n1
        dcb->StopBits = ONESTOPBIT;     // 8n1
        dcb->fDsrSensitivity = FALSE;           // ignore dsr state
        dcb->fOutxDsrFlow = FALSE;              // dont monitor DSR as flow control pin
        dcb->fDtrControl = DTR_CONTROL_ENABLE;  // keep DTR high while port open
        // parse user options and set corresponding values
        while ((addr != NULL) && (*addr != 0))
        {
            // skip commas
            if (*addr == ',')
                ++addr;
            // check for +rts (regulate RTS for output flow control)
            if (strncmp(addr, "+RTS", 4) == 0)
                dcb->fRtsControl = RTS_CONTROL_HANDSHAKE;
            // check for -rts (just keep RTS high while port open)
            if (strncmp(addr, "-RTS", 4) == 0)
                dcb->fRtsControl = RTS_CONTROL_ENABLE;
            // check for +cts (monitor CTS for output flow control)
            if (strncmp(addr, "+CTS", 4) == 0)
                dcb->fOutxCtsFlow = TRUE;
            // check for -cts (ignore CTS)
            if (strncmp(addr, "-CTS", 4) == 0)
                dcb->fOutxCtsFlow = FALSE;
            // handle baud rate selection
            if ((*addr >= '0') && (*addr <= '9'))
            {
                dcb->BaudRate = 0;
                while ((*addr >= '0') && (*addr <= '9'))
                {
                    dcb->BaudRate = (dcb->BaudRate * 10) + (*addr++ & 15);
                }
            }
            // skip to comma
            while ((*addr != ',') && (*addr != 0))
            {
                ++addr;
            }
        }
        // set updated parms
        SetCommState(ref->handle, dcb);
    }

    // clear any pending data
    PurgeComm(ref->handle, PURGE_TXCLEAR|PURGE_RXCLEAR);

    // signal events by default
    SetEvent(ref->rdover.hEvent);
    SetEvent(ref->wtover.hEvent);
    SetEvent(ref->wrover.hEvent);

    // transition the mode
    ref->state = (ref->state == HOLD ? OPEN : HOLD);
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerListen

    \Description
        Listen for a connection

    \Input *ref         - reference pointer
    \Input *addr        - port to open on (only :port portion used)

    \Output
        int32_t             - negative=error, zero=ok

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerListen(CommSerRef *ref, const char *addr)
{
    int32_t err;

    // screen for null addresses
    if ((addr == NULL) || (ref->state != IDLE))
    {
        return(COMM_BADSTATE);
    }

    // connect and listen are same except initial state
    EnterCriticalSection(&(ref->crit));
    // setup the listen
    err = CommSerRecover(ref, addr);
    // convert from list to conn
    if (ref->state == HOLD)
    {
        ref->state = LIST;
    }
    // done with critical section (dont want thread to start
    // doing listen things since we are chaning to CONN)
    LeaveCriticalSection(&(ref->crit));
    return(err);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerUnlisten

    \Description
        Stop listening

    \Input *ref         - reference pointer

    \Output
        int32_t             - negative=error, zero=ok

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerUnlisten(CommSerRef *ref)
{
#if defined(_WIN64)
    int64_t tempHandle = -1;
#else
    int32_t tempHandle = -1;
#endif

    // if already idle, then nothing to do
    if (ref->state == IDLE)
    {
        return(0);
    }

    // if port is open, close it
    if (ref->state == OPEN)
    {
        ref->state = STOP;
        // wait for thread to close port
        while (ref->state == STOP)
        {
            Sleep(0);
        }
    }

    // change to idle state
    ref->state = RESET;
    while (ref->state != IDLE)
    {
        Sleep(0);
    }

    // release resources
    if (ref->handle != (HANDLE)tempHandle)
    {
        // kill any pending waitcommstate
        SetCommMask(ref->handle, EV_RXFLAG);
        // kill all pending reads/writes
        PurgeComm(ref->handle, PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
        // close the handle
        CloseHandle(ref->handle);
        ref->handle = (HANDLE)tempHandle;
    }
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerUnlisten

    \Description
        Initiate a connection to a peer

    \Input *ref         - reference pointer
    \Input *addr        - address in ip-address:port form

    \Output
        int32_t             - negative=error, zero=ok

    \Notes
        Does not currently perform dns translation

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerConnect(CommSerRef *ref, const char *addr)
{
    int32_t err;

    // screen for null addresses
    if ((addr == NULL) || (ref->state != IDLE))
    {
        return(COMM_BADSTATE);
    }

    // connect and listen are same except initial state
    EnterCriticalSection(&(ref->crit));
    // setup the listen
    err = CommSerRecover(ref, addr);
    // convert from list to conn
    if (ref->state == HOLD)
    {
        ref->state = CONN;
    }
    // done with critical section (dont want thread to start
    // doing listen things since we are chaning to CONN)
    LeaveCriticalSection(&(ref->crit));
    return(err);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerUnconnect

    \Description
        Terminate a connection

    \Input *ref         - reference pointer

    \Output
        int32_t             - negative=error, zero=ok

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerUnconnect(CommSerRef *ref)
{
    return(CommSerUnlisten(ref));
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerStatus

    \Description
        Return current stream status

    \Input *ref         - reference pointer

    \Output
        int32_t             - COMM_CONNECTING, COMM_OFFLINE, COMM_ONLINE or COMM_FAILURE

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerStatus(CommSerRef *ref)
{
    // return state
    if ((ref->state == CONN) || (ref->state == LIST))
        return(COMM_CONNECTING);
    if ((ref->state == IDLE) || (ref->state == CLOSE))
        return(COMM_OFFLINE);
    if ((ref->state == OPEN) || (ref->state == HOLD))
        return(COMM_ONLINE);
    return(COMM_FAILURE);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerTick

    \Description
        Return current tick

    \Input *ref         - reference pointer

    \Output
        uint32_t    - elapsed milliseconds

    \Version    1.0        03/10/03 (JLB) Copied from CommSRPTick
*/
/*************************************************************************************************F*/
uint32_t CommSerTick(CommSerRef *ref)
{
    return(NetTick());
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerSend

    \Description
        Send a packet

    \Input *ref         - reference pointer
    \Input buffer       - pointer to data
    \Input length       - length of data

    \Output
        int32_t             - negative=error, zero=buffer full (temp fail), positive=queue position (ok)

    \Notes
        Zero length packets may not be sent (they are used for buffer query)

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerSend(CommSerRef *ref, const void *buffer, int32_t length)
{
    int32_t pos;
    RawSerPacketT *packet;
    
    // make sure port is open
    if ((ref->state != OPEN) && (ref->state != HOLD))
    {
        return(COMM_BADSTATE);
    }

    // see if output buffer full
    if ((ref->sndinp+ref->sndwid)%ref->sndlen == ref->sndout)
    {
        return(0);
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
    packet = (RawSerPacketT *) &(ref->sndbuf[ref->sndinp]);
    packet->head.len = length;

    // return error for oversized packets
    if (packet->head.len+sizeof(packet) > ref->sndwid+sizeof(packet->body.data))
    {
        return(COMM_MINBUFFER);
    }

    // copy the packet to the buffer
    memcpy(packet->body.data, buffer, length);
    // set the data fields
    packet->body.seq = ref->sndseq++;
    packet->body.ack = ref->rcvseq-1;
    // set the send time
    packet->head.when = GetTickCount();

    // add the packet to the queue
    ref->sndinp = (ref->sndinp+ref->sndwid) % ref->sndlen;

    // send packet immediately
    EnterCriticalSection(&(ref->crit));
    _CommSerProcessOutput(ref);
    LeaveCriticalSection(&(ref->crit));

    // return buffer depth
    pos = (((ref->sndinp+ref->sndlen)-ref->sndout)%ref->sndlen)/ref->sndwid;
    return(pos > 0 ? pos : 1);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerPeek

    \Description
        Peek at waiting packet

    \Input *ref         - reference pointer
    \Input target       - target buffer
    \Input length       - buffer length
    \Input *when        - tick received at

    \Output
        int32_t             - negative=nothing pending, else packet length

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerPeek(CommSerRef *ref, void *target, int32_t length, uint32_t *when)
{
    RawSerPacketT *packet;
    
    // see if a packet is available
    if (ref->rcvout == ref->rcvinp)
    {
        return(COMM_NODATA);
    }

    // block access during callback usage
    while (ref->callback)
    {
        Sleep(0);
    }

    // point to the packet
    packet = (RawSerPacketT *) &(ref->rcvbuf[ref->rcvout]);

    // copy over the data portion
    memcpy(target, packet->body.data, (packet->head.len < length ? packet->head.len : length));
    // get the timestamp
    if (when != NULL)
    {
        *when = packet->head.when;
    }
    
    // return packet data length
    return(packet->head.len);
}

/*F*************************************************************************************************/
/*!
    \Function    CommSerRecv

    \Description
        Receive a packet from the buffer

    \Input *ref         - reference pointer
    \Input target       - target buffer
    \Input length       - buffer length
    \Input *when        - tick received at

    \Output
        int32_t             - negative=error, else packet length

    \Version    1.0        02/15/99 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t CommSerRecv(CommSerRef *ref, void *target, int32_t length, uint32_t *when)
{
    // use peek to remove the data
    int32_t len = CommSerPeek(ref, target, length, when);
    if (len >= 0)
    {
        ref->rcvout = (ref->rcvout+ref->rcvwid)%ref->rcvlen;
    }
    // all done
    return(len);
}