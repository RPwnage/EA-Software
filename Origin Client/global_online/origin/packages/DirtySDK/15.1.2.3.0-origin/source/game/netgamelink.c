/*H*************************************************************************************************/
/*!

    \File    netgamelink.c

    \Description
        This module provides a packet layer peer-peer interface which utilizes
        a lower layer "comm" module. This module is used either by netgamedist
        (if it is being used) or can be accessed directly if custom online game
        logic is being used. This module also calculates latency (ping) and
        maintains statistics about the connection (number of bytes send/received).
        The direct application interface is currently weak but will be improved.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        12/19/00 (GWS) First Version

*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>             /* memset/ds_memcpy */

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/comm/commall.h"

/*** Defines ***************************************************************************/

#define KEEP_ALIVE_TIME 500

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! netgamelink internal state
struct NetGameLinkRefT
{
    //! port used to communicate with server
    CommRef *m_port;
    //! flag to indicate if we own port (if we need to destroy when done)
    int32_t m_portowner;

    //! module memory group
    int32_t m_memgroup;
    void * m_memgrpusrdata;

    //! tick at which last packet was sent to server (in ms)
    uint32_t m_lastsend;
    //! tick at which last sync packet was received from server (in ms)
    uint32_t m_lastrecv;
    //! tick at which last sync packet contributed to latency average calculation
    uint32_t m_lastavg;
    //! last echo requested by server (used for rtt calc)
    uint32_t m_lastecho;
    //! last time a sync packet was sent
    uint32_t m_lastsync;
    //! last time the history slot changed
    uint32_t m_lasthist;

    //! the time weighted rtt average
    int32_t m_rttavg;
    //! the time weighted rtt deviation
    int32_t m_rttdev;

    //! external status monitoring
    NetGameLinkStatT m_stats;

    //! stats - input bytes
    int32_t m_inpbyte;
    //! stats - input packets
    int32_t m_inppack;
    //! stats - input raw bytes
    int32_t m_inprawb;
    //! stats - input raw packets
    int32_t m_inprawp;
    //! stats - UDP send overhead
    int32_t m_overhead;
    //! stats - UDP receive overhead
    int32_t m_rcvoverhead;
    //! stats - recv raw bytes
    int32_t m_rcvrawb;
    //!stats - recv packet bytes
    int32_t m_rcvrawp;
    //! stats - recv packets
    int32_t m_rcvpack;
    //! stats - recv bytes
    int32_t m_rcvbytes;

    // track packets sent/received for sending to peer in sync packets

    //! stats - raw packets sent
    int32_t m_packsent;
    //! stats - raw packets received
    int32_t m_packrcvd;
    //! stats - remote->local packets lost
    int32_t m_lpacklost;
    //! stats - naks sent
    int32_t m_naksent;

    //! point to receive buffer
    char *m_buffer;
    //! length of data in receive buffer
    int32_t m_buflen;
    //! size of receive buffer
    int32_t m_bufmax;

    //! protect resources
    NetCritT crit;
    //! count of missed accessed
    int32_t m_process;

    //! data for m_callproc
    void *m_calldata;
    //! callback for incoming packet notification
    void (*m_callproc)(void *m_calldata, int32_t kind);
    //! callback pending
    int32_t m_callpend;

    //! count calls to NetGameLinkRecv() to limit callback access
    int32_t m_recvprocct;

    //! send enable/disable
    int32_t m_sendenabled;

    //! sync enable/disable
    int32_t m_syncenabled;

    //! list of active streams
    NetGameLinkStreamT *m_stream;

    int32_t iStrmMaxPktSize;

    //! used for tracking QoS packets
    uint32_t uQosSendInterval;
    uint32_t uQosLastSendTime;
    uint32_t uQosStartTime;
    uint16_t uQosPacketsToSend;
    uint16_t uQosPacketsToRecv;
    uint16_t uQosPacketSize;
    int32_t iQosCumulLate;
    int32_t iQosLateCount;
    int32_t iQosAvgLate;

    //! verbosity
    int32_t iVerbosity;
};

typedef struct GameStreamB
{
    int32_t kind;           //!< block type
    uint32_t sync;          //!< sync sequence number
    int32_t sent;           //!< amount sent so far
    int32_t size;           //!< total block size
    int32_t iSubchan;       //!< subchannel index/
} GameStreamB;


/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

// Public variables


/*** Private Functions *****************************************************************/



/*F*************************************************************************************/
/*!
    \Function     _NetGameLinkUpdateLocalPacketLoss

    \Description
        Calculate local packet loss based on a difference of remote packets sent and
        local packets received.

    \Input uLPackRcvd       - count of packets received locally
    \Input uRPackSent       - count of packets sent by remote peer (cumulative psnt
                              values of all received sync packets)
    \Input uOldLPackLost    - old count of packets lost

    \Output
        uint32_t            - updated count of packets lost

    \Notes
        \verbatim
        This function returns the number of packets lost at this end of the connection
        since the beginning. It is obtained by subtracting uRPackSent (the number of
        packets sent by the remote end - cumulative psnt values of all received sync
        packets) from uLPackRcvd (the number of packets received locally).

        The following two scenarios indicate that a sync packet itself suffered packet
        loss, and it relied on the packet resending mechanisms of commudp to finally
        make it to us:
            1 - uLPackRcvd is larger than uRPackSent (subtracting them would return a
                negative value)
            2 - uLPackRcvd is smaller than uRPackSent, but the last saved count of
                packets lost is larger than the new one.

        For both scenarios, we skip updating the packets lost count to avoid a
        possibly negative or under-valued packet loss calculation, and we deal with
        that iteration as if no packet loss was detected.

        The assumption here is that the next sync packet that makes it to us without
        being "resent" will end up updating the packet lost counters with a "coherent"
        value as its "psnt" field will include all packets that were used for packet
        retransmission.  At the end, this is guaranteeing the coherency of the packets
        lost count (non negative value and only increasing over time) by delaying its
        update.
        \endverbatim

    \Version 09/02/14 (mclouatre)
*/
/*************************************************************************************F*/
static uint32_t _NetGameLinkUpdateLocalPacketLoss(uint32_t uLPackRcvd, uint32_t uRPackSent, uint32_t uOldLPackLost)
{
    uint32_t uUpdatedLPackLost = uOldLPackLost;

    // only proceed if packet loss is not negative
    if (uRPackSent > uLPackRcvd)
    {
        // only proceed if packet loss is larger than last calculated value
        if ((uRPackSent - uLPackRcvd) > uOldLPackLost)
        {
            uUpdatedLPackLost = uRPackSent - uLPackRcvd;
        }
     }

    return(uUpdatedLPackLost);
}

/*F*************************************************************************************/
/*!
    \Function     _NetGameLinkGetSync

    \Description
        Extract sync packet from buffer, and return a pointer to the sync packet location in the buffer

    \Input *pPacket     - pointer to packet structure
    \Input *pPacketData - pointer to packet data (used instead of packet buffer ref to allow split use)
    \Input *pSync       - [out] output buffer for extracted sync packet

    \Output
        NetGamePacketSyncT * - pointer to start of sync packet in buffer, or NULL if invalid

    \Version 09/09/11 (jbrookes)
*/
/*************************************************************************************F*/
static NetGamePacketSyncT *_NetGameLinkGetSync(NetGamePacketT *pPacket, uint8_t *pPacketData, NetGamePacketSyncT *pSync)
{
    uint32_t uSyncSize=0;
    // validate sync length byte is available
    if (pPacket->head.len < 1)
    {
        NetPrintf(("netgamelink: received a sync packet with no data\n"));
        return(NULL);
    }
    // validate sync size
    if ((uSyncSize = (uint32_t)pPacketData[pPacket->head.len-1]) != sizeof(*pSync))
    {
        NetPrintf(("netgamelink: received a sync with an invalid size (got=%d, expected=%d)\n", uSyncSize, sizeof(*pSync)));
        return(NULL);
    }
    // validate packet is large enough to hold sync
    if (pPacket->head.len < uSyncSize)
    {
        NetPrintf(("netgamelink: received a sync too large for packet (len=%d)\n", pPacket->head.len));
        return(NULL);
    }
    // locate sync at end of packet & subtract from packet length, copy to output
    pPacket->head.len -= uSyncSize;
    ds_memcpy(pSync, pPacketData+pPacket->head.len, uSyncSize);
    // return sync packet pointer to caller
    return((NetGamePacketSyncT *)(pPacketData+pPacket->head.len));
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkSendPacket

    \Description
        Send a packet to the server

    \Input *ref     - reference pointer
    \Input *packet  - pointer to completed packet
    \Input currtick - current tick

    \Output
        int32_t     - bad error, zero=unable to send now, positive=sent

    \Notes
        Adds timestamp, rtt and duplex information to packet

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkSendPacket(NetGameLinkRefT *ref, NetGamePacketT *packet, uint32_t currtick)
{
    int32_t iResult;
    int32_t iSynced = 0;
    NetGamePacketSyncT sync;
    uint32_t uPacketFlags, uPacketType;
    uint32_t uPackSent=0, uPackRcvd=0, uLPktLost=0, uNakSent=0;

    // sending enabled?
    if (ref->m_sendenabled == FALSE)
    {
        NetPrintf(("netgamelink: warning -- trying to send packet over a sending-disabled link (%d bytes)\n", packet->head.len));
        return(1);
    }

    // return error for oversized packets
    if ((packet->head.len + NETGAME_DATAPKT_MAXTAIL) > ref->m_port->maxwid)
    {
        NetPrintf(("netgamelink: oversized packet send (%d bytes)\n", packet->head.len));
        return(-1);
    }

    // see if we should add sync info
    if (((packet->head.kind & GAME_PACKET_SYNC) || (currtick-ref->m_lastsync > KEEP_ALIVE_TIME/2)) && (ref->m_syncenabled == TRUE))
    {
        // build the sync packet
        memset(&sync, 0, sizeof(sync));

        sync.size = sizeof(sync);
        sync.echo = SocketHtonl(currtick);
        sync.repl = SocketHtonl(ref->m_lastecho+(currtick-ref->m_lastrecv));
        sync.late = SocketHtons((int16_t)((ref->m_rttavg+ref->m_rttdev+1)/2));

        uLPktLost = _NetGameLinkUpdateLocalPacketLoss(ref->m_stats.lpackrcvd, ref->m_stats.rpacksent, (unsigned)ref->m_lpacklost);
        uPackSent = ref->m_port->packsent;
        uNakSent = ref->m_port->naksent;
        uPackRcvd = ref->m_port->packrcvd;

        sync.plost = (uint8_t)(uLPktLost - ref->m_lpacklost);
        sync.psnt = (uint8_t)(uPackSent - ref->m_packsent);
        sync.nsnt = (uint8_t)(uNakSent - ref->m_naksent);
        sync.prcvd = (uint8_t)(uPackRcvd - ref->m_packrcvd);

        // if we have not received a sync packet from the remote peer, our repl field is not valid, so we tell the remote peer to ignore it
        if (ref->m_stats.rpacksent == 0)
        {
            sync.flags |= NETGAME_SYNCFLAG_REPLINVALID;
        }

        // piggyback on existing packet
        ds_memcpy(packet->body.data+packet->head.len, &sync, sizeof(sync));
        packet->head.len += sizeof(sync);
        packet->head.kind |= GAME_PACKET_SYNC;
        iSynced = 1;
    }

    // put type as last byte
    packet->body.data[packet->head.len] = packet->head.kind;
    // determine packet flags
    uPacketType = packet->head.kind & ~GAME_PACKET_SYNC;
    if (((uPacketType <= GAME_PACKET_ONE_BEFORE_FIRST) || (uPacketType >= GAME_PACKET_ONE_PAST_LAST)) && (packet->head.kind != GAME_PACKET_SYNC))
    {
        NetPrintf(("netgamelink: warning -- send unrecognized packet kind %d\n", packet->head.kind));
    }
    if (uPacketType == GAME_PACKET_USER_UNRELIABLE)
    {
        uPacketFlags = COMM_FLAGS_UNRELIABLE;
    }
    else if (uPacketType == GAME_PACKET_USER_BROADCAST)
    {
        uPacketFlags = COMM_FLAGS_UNRELIABLE|COMM_FLAGS_BROADCAST;
    }
    else
    {
        uPacketFlags = COMM_FLAGS_RELIABLE;
    }

    // send the packet
    iResult = ref->m_port->Send(ref->m_port, packet->body.data, packet->head.len + 1, uPacketFlags);

    // if we added sync info, remove it now
    if (iSynced)
    {
        packet->head.len -= sizeof(sync);
        packet->head.kind ^= GAME_PACKET_SYNC;
    }

    // make sure send succeeded
    if (iResult > 0)
    {
        // record the send time
        ref->m_lastsend = currtick;
        // if it was a sync packet, update sync info
        if (iSynced)
        {
            ref->m_lastsync = currtick;
            ref->m_packsent = uPackSent;
            ref->m_naksent = uNakSent;
            ref->m_packrcvd = uPackRcvd;
            ref->m_lpacklost = uLPktLost;
        }

        // update the stats
        ref->m_inpbyte += packet->head.len;
        ref->m_inppack += 1;
        ref->m_stats.sent += packet->head.len;
        ref->m_stats.sentlast = ref->m_lastsend;

        // see if we should turn off send light
        if ((ref->m_stats.sentshow != 0) && (ref->m_lastsend-ref->m_stats.sentshow > 100))
        {
            ref->m_stats.sentshow = 0;
            ref->m_stats.senthide = ref->m_lastsend;
        }

        // see if we should turn on send light
        if ((ref->m_stats.senthide != 0) && (ref->m_lastsend-ref->m_stats.senthide > 100))
        {
            ref->m_stats.senthide = 0;
            ref->m_stats.sentshow = ref->m_lastsend;
        }
    }
    else
    {
//        NetPrintf(("GmLink: send failed! (kind=%d, len=%d, synced=%d)\n",
//            packet->head.kind, packet->head.len, iSynced));
    }

    // return the result code
    return(iResult);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkRecvPacket0

    \Description
        Process incoming data packet

    \Input *ref     - reference pointer
    \Input currtick - current tick

    \Output
        int32_t     - zero=no packet processed, positive=packet processed

    \Version 12/19/00 (GWS)

*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkRecvPacket0(NetGameLinkRefT *ref, uint32_t currtick)
{
    int32_t size;
    int16_t delta;
    uint32_t when, kind;
    NetGamePacketT *packet;
    NetGameLinkHistT *history;

    // calculate buffer space free, making sure to include packet header overhead (not sent, but queued)
    if ((size = ref->m_bufmax-ref->m_buflen-sizeof(packet->head)) <= 0)
    {
        return(0);
    }
    else if (size > ref->m_port->maxwid)
    {
        size = ref->m_port->maxwid;
    }

    // setup packet buffer
    packet = (NetGamePacketT *)(ref->m_buffer+ref->m_buflen);

    // see if packet is available
    size = ref->m_port->Recv(ref->m_port, &packet->body, size, &when);
    if ((size <= 0) || (size > ref->m_port->maxwid))
    {
        #if DIRTYCODE_LOGGING
        if (size != COMM_NODATA)
        {
            NetPrintf(("netgamelink: Recv() returned %d\n", size));
        }
        #endif
        return(0);
    }
    packet->head.len = size;
    packet->head.size = 0;
    packet->head.when = when;

    ref->m_stats.tick = currtick;
    ref->m_stats.tickseqn += 1;

    // update the stats
    ref->m_stats.rcvdlast = currtick;
    ref->m_stats.rcvd += packet->head.len;
    ref->m_rcvbytes += packet->head.len;
    ref->m_rcvpack += 1;

    // see if we should turn off receive light
    if ((ref->m_stats.rcvdshow != 0) && (currtick-ref->m_stats.rcvdshow > 100))
    {
        ref->m_stats.rcvdshow = 0;
        ref->m_stats.rcvdhide = currtick;
    }

    // see if we should turn on receive light
    if ((ref->m_stats.rcvdhide != 0) && (currtick-ref->m_stats.rcvdhide > 100))
    {
        ref->m_stats.rcvdhide = 0;
        ref->m_stats.rcvdshow = currtick;
    }

    // extract the kind field and fix the length
    packet->head.len -= 1;
    packet->head.kind = packet->body.data[packet->head.len];

    // get packet kind
    kind = packet->head.kind & ~GAME_PACKET_SYNC;

    // warn if kind is invalid
    if (((kind <= GAME_PACKET_ONE_BEFORE_FIRST) || (kind >= GAME_PACKET_ONE_PAST_LAST)) && (packet->head.kind != GAME_PACKET_SYNC))
    {
        NetPrintf(("netgamelink: warning -- recv unrecognized packet kind %d\n", packet->head.kind));
        return(1);
    }

    // see if this packet contains timing info
    if (packet->head.kind & GAME_PACKET_SYNC)
    {
        // get sync packet info
        NetGamePacketSyncT sync;
        if (_NetGameLinkGetSync(packet, packet->body.data, &sync) == NULL)
        {
            return(1);
        }

        // remove sync bit
        packet->head.kind ^= GAME_PACKET_SYNC;

        // update the latency stat
        ref->m_stats.late = SocketNtohs(sync.late);

        // calculate instantaneous rtt
        if ((sync.flags & NETGAME_SYNCFLAG_REPLINVALID) == 0)
        {
            /* delta = recvtime - sendtime + 1
               The +1 is needed because the peer may think the packet stayed in
               its queue for 1ms while we may think it was returned within the
               same millisecond. This happens because the clocks are not precisely
               synchronized. Adding 1 avoids the issue without compromising precision. */
            delta = (int16_t)(when - SocketNtohl(sync.repl) + 1);
        }
        else
        {
            // remote peer has indicated their repl field is invalid, so we ignore sync latency info
            delta = -1;
        }
        if ((delta >= 0) && (delta <= 2500))
        {
            // figure out elapsed time since last packet
            int32_t elapsed = when-ref->m_lastavg;
            if (elapsed < 10)
            {
                elapsed = 10;
            }
            ref->m_lastavg = when;

            // perform rtt calc using weighted time average
            if (elapsed < RTT_SAMPLE_PERIOD)
            {
                // figure out weight of existing data
                int32_t weight = RTT_SAMPLE_PERIOD-elapsed;
                // figure deviation first since it uses average
                int32_t deviate = delta - ref->m_rttavg;
                if (deviate < 0)
                    deviate = -deviate;
                // calc weighted deviation
                ref->m_rttdev = (weight*ref->m_rttdev+elapsed*deviate)/RTT_SAMPLE_PERIOD;
                // calc weighted average
                ref->m_rttavg = (weight*ref->m_rttavg+elapsed*delta)/RTT_SAMPLE_PERIOD;
            }
            else
            {
                // if more than our scale has elapsed, use this data
                ref->m_rttavg = delta;
                ref->m_rttdev = 0;
            }

            // save copy of ping in stats table
            ref->m_stats.ping = ref->m_rttavg+ref->m_rttdev;
            ref->m_stats.pingavg = ref->m_rttavg;
            ref->m_stats.pingdev = ref->m_rttdev;

            // see if this is a new recod
            if (when-ref->m_lasthist >= PING_LENGTH)
            {
                // remember the update time
                ref->m_lasthist = when;
                // advance to next ping slot
                ref->m_stats.pingslot = (ref->m_stats.pingslot + 1) % PING_HISTORY;
                history = ref->m_stats.pinghist + ref->m_stats.pingslot;

                // save the information
                history->min = delta;
                history->max = delta;
                history->avg = delta;
                history->cnt = 1;
            }
            else
            {
                // update the information
                history = ref->m_stats.pinghist + ref->m_stats.pingslot;
                if (delta < history->min)
                    history->min = delta;
                if (delta > history->max)
                    history->max = delta;
                history->avg = ((history->avg * history->cnt) + delta) / (history->cnt+1);
                history->cnt += 1;
            }

            // save this update time
            ref->m_stats.pingtick = when;
        }
        else if ((sync.flags & NETGAME_SYNCFLAG_REPLINVALID) == 0)
        {
            NetPrintf(("netgamelink: sync delta %d is out of range (when=0x%08x,sync.repl=0x%08x)\n", delta, when, SocketNtohl(sync.repl) ));
        }

        // extract timing information
        ref->m_lastrecv = when;
        ref->m_lastecho = SocketNtohl(sync.echo);

        // update remote peer's packets sent/recv stat tracker
        if (ref->m_stats.rpacksent == 0)
        {
            /*
               bias initial update by one to account for the commdup packet conveying this NetGameLinkSyncT packet not being in the count (but it should really be)
               no need to do so afterwards because the count will include the previous packet for which the count is already biased
            */
            ref->m_stats.rpacksent += 1;
        }
        ref->m_stats.rpacksent += sync.psnt;
        ref->m_stats.rnaksent += sync.nsnt;
        ref->m_stats.rpackrcvd += sync.prcvd;
        ref->m_stats.rpacklost += sync.plost;

        // update packets received, packets saved, packets sent and packets lost; this is done here to keep in sync with rpacksent and rpackrcvd
        ref->m_stats.lpackrcvd = ref->m_port->packrcvd;
        ref->m_stats.lpacksaved = ref->m_port->packsaved;
        ref->m_stats.lpacksent = ref->m_port->packsent;
        ref->m_stats.lpacklost = _NetGameLinkUpdateLocalPacketLoss(ref->m_stats.lpackrcvd, ref->m_stats.rpacksent, ref->m_stats.lpacklost);
    }

    // save packet if something remains (was not just sync)
    if (packet->head.kind != 0)
    {
        packet->head.size = (sizeof(packet->head)+packet->head.len+3) & 0x7ffc;
        ref->m_buflen += packet->head.size;
    }
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkRecvPacket

    \Description
        Call _NetGameLinkRecvPacket0 if we haven't already

    \Input *ref     - reference pointer
    \Input currtick - current tick

    \Output
        int32_t     - zero=no packet processed, positive=packet processed

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkRecvPacket(NetGameLinkRefT *ref, uint32_t currtick)
{
    int32_t retval;

    // limit call depth to prevent a callback from calling us more than once
    if (ref->m_recvprocct > 0)
    {
        #if DIRTYCODE_LOGGING && 0
        NetPrintf(("netgamelink: m_recvprocct=%d, unable to call _NetGameLinkRecvPacket0\n", ref->m_recvprocct));
        #endif
        return(0);
    }

    ref->m_recvprocct = 1;
    retval = _NetGameLinkRecvPacket0(ref, currtick);
    ref->m_recvprocct = 0;

    return(retval);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkProcess

    \Description
        Main send/receive processing loop

    \Input *ref     - reference pointer
    \Input currtick - current tick

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static void _NetGameLinkProcess(NetGameLinkRefT *ref, uint32_t currtick)
{
    uint32_t range;

    // grab any pending packets
    while (_NetGameLinkRecvPacket(ref, currtick) > 0)
    {
        ref->m_callpend += 1;
    }

    // mark as processing complete
    ref->m_process = 0;

    // handle stats update (once per second)
    ref->m_stats.tick = currtick;
    ref->m_stats.tickseqn += 1;
    range = currtick - ref->m_stats.stattick;
    if (range >= 1000)
    {
        // calc bytes per second, raw bytes per second, and network bytes per second
        ref->m_stats.outbps = (ref->m_inpbyte*1000)/range;
        ref->m_stats.outrps = (ref->m_port->datasent-ref->m_inprawb)*1000/range;
        ref->m_stats.outnps = ((ref->m_port->datasent-ref->m_inprawb)+(ref->m_port->overhead-ref->m_overhead))*1000/range;
        ref->m_stats.inrps = ((ref->m_port->datarcvd-ref->m_rcvrawb)*1000)/range;
        ref->m_stats.inbps = (ref->m_rcvbytes* 1000)/range;
        ref->m_stats.innps = ((ref->m_port->datarcvd-ref->m_rcvrawb)+(ref->m_port->rcvoverhead-ref->m_rcvoverhead))*1000/range;
        // calculate packets per second and raw packets per second
        ref->m_stats.outpps = ((ref->m_inppack*1000)+500)/range;
        ref->m_stats.outrpps = ((ref->m_port->packsent-ref->m_inprawp)*1000+500)/range;
        ref->m_stats.inpps = ((ref->m_rcvpack*1000)+500)/range;
        ref->m_stats.inrpps = ((ref->m_port->packrcvd - ref->m_rcvrawp)*1000+500)/range;
        // reset tracking variables
        ref->m_inpbyte = ref->m_inppack = 0;
        ref->m_rcvbytes = ref->m_rcvpack = 0;
        ref->m_inprawb = ref->m_port->datasent;
        ref->m_inprawp = ref->m_port->packsent;
        ref->m_rcvrawp = ref->m_port->packrcvd;
        ref->m_rcvrawb = ref->m_port->datarcvd;
        ref->m_overhead = ref->m_port->overhead;
        ref->m_rcvoverhead = ref->m_port->rcvoverhead;

        // remember stat update time
        ref->m_stats.stattick = currtick;
    }

    // see if we should turn off send light
    if ((ref->m_stats.sentshow != 0) && (currtick-ref->m_stats.sentshow > 100))
    {
        ref->m_stats.sentshow = 0;
        ref->m_stats.senthide = currtick;
    }

    // see if we should turn on send light
    if ((ref->m_stats.senthide != 0) && (currtick-ref->m_stats.senthide > 100) && (currtick-ref->m_stats.sentlast < 50))
    {
        ref->m_stats.senthide = 0;
        ref->m_stats.sentshow = currtick;
    }

    // see if we should turn off receive light
    if ((ref->m_stats.rcvdshow != 0) && (currtick-ref->m_stats.rcvdshow > 100))
    {
        ref->m_stats.rcvdshow = 0;
        ref->m_stats.rcvdhide = currtick;
    }

    // see if we should turn on receive light
    if ((ref->m_stats.rcvdhide != 0) && (currtick-ref->m_stats.rcvdhide > 100) && (currtick-ref->m_stats.rcvdlast < 50))
    {
        ref->m_stats.rcvdhide = 0;
        ref->m_stats.rcvdshow = currtick;
    }

    // send keepalive/sync if needed
    if ((currtick-ref->m_lastsend > KEEP_ALIVE_TIME) && (ref->m_syncenabled == TRUE))
    {
        // make sure we do not overflow output queue
        int32_t queue = ref->m_port->Send(ref->m_port, NULL, 0, COMM_FLAGS_RELIABLE);
        // the exact buffer limit is unimportant, but it needs something
        // to avoid overrun during a semi-persistant failure
        if ((queue > 0) && (queue < (ref->m_port->maxout/4)))
        {
            // send sync packet
            NetGamePacketT spacket;
            spacket.head.kind = GAME_PACKET_SYNC;
            spacket.head.len = 0;
            _NetGameLinkSendPacket(ref, &spacket, currtick);
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkPrintStats

    \Description
        Prints the current NetGameLink stats.

    \Input *pRef     - reference pointer

    \Version 09/03/14 (mcorcoran)
*/
/*************************************************************************************************F*/
static void _NetGameLinkPrintStats(NetGameLinkRefT *pRef)
{
    NetGameLinkStatT stats;
    NetGameLinkStatus(pRef, 'stat', 0, &stats, sizeof(stats));

    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: QoS Results ------------------------------------------------------------------------------\n"));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: latency over QoS period                                                          %d\n", pRef->iQosAvgLate));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: latency over last sampling period                                                %d (sampl. prd = %d ms)\n", stats.late, RTT_SAMPLE_PERIOD));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: when connection established                                                      %d\n", stats.conn));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: when data most recently sent                                                     %d\n", stats.sentlast));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: when data most recently received                                                 %d\n", stats.rcvdlast));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: ping deviation                                                                   %d\n", stats.pingdev));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: ping average                                                                     %d\n", stats.pingavg));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of bytes sent                                                             %d\n", stats.sent));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of bytes received                                                         %d\n", stats.rcvd));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets sent to peer since start (at time = last inbound sync pkt)     %d\n", stats.lpacksent));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets sent to peer since start (at time = now)                       %d\n", stats.lpacksent_now));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets received from peer since start                                 %d\n", stats.lpackrcvd));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets sent by peer (to us) since start                               %d\n", stats.rpacksent));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of packets received by peer (from us) since start                         %d\n", stats.rpackrcvd));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: local->remote packets lost: number of packets (from us) lost by peer since start %d\n", stats.rpacklost));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: packets saved: number of packets recovered by via redundancy mechanisms          %d\n", stats.lpacksaved));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: number of NAKs sent by peer (to us) since start                                  %d\n", stats.rnaksent));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: packets per sec sent (user packets)                                              %d\n", stats.outpps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: raw packets per sec sent (packets sent to network)                               %d\n", stats.outrpps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: bytes per sec sent (user data)                                                   %d\n", stats.outbps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: raw bytes per sec sent (data sent to network)                                    %d\n", stats.outrps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: network bytes per sec sent (inrps + estimated UDP/Eth frame overhead)            %d\n", stats.outnps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: packets per sec received (user packets)                                          %d\n", stats.inpps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: raw packets per sec received (packets sent to network)                           %d\n", stats.inrpps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: bytes per sec received (user data)                                               %d\n", stats.inbps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: raw bytes per sec received (data sent to network)                                %d\n", stats.inrps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: network bytes per sec received (inrps + estimated UDP/Eth frame overhead)        %d\n", stats.innps));
    NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: ------------------------------------------------------------------------------------------\n"));
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkNotify

    \Description
        Main notification from lower layer

    \Input *what    - generic comm pointer
    \Input event    - event type

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
static void _NetGameLinkNotify(CommRef *what, int32_t event)
{
    NetGameLinkRefT *ref = (NetGameLinkRefT *)what->refptr;

    // make sure we are exclusive
    if (NetCritTry(&ref->crit))
    {
        // do the processing
        _NetGameLinkProcess(ref, NetTick());

        // do callback if needed
        if ((ref->m_buflen > 0) && (ref->m_callproc != NULL) && (ref->m_callpend > 0))
        {
            ref->m_callpend = 0;
            (ref->m_callproc)(ref->m_calldata, 1);
        }

        // free access
        NetCritLeave(&ref->crit);
    }
    else
    {
        // count the miss
        ref->m_process += 1;
        if (ref->m_process > 0)
        {
//            NetPrintf(("netgamelink: missed %d events\n", ref->m_process));
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function _NetGameLinkSendCallback

    \Description
        Handle notification of send (or re-send) of data from comm layer.

    \Input *pComm       - pointer to comm ref
    \Input *pPacket     - packet data that is being sent
    \Input iPacketLen   - packet length
    \Input uCurrTick    - current timestamp

    \Version 09/09/11 (jbrookes)
*/
/*************************************************************************************************F*/
static void _NetGameLinkSendCallback(CommRef *pComm, void *pPacket, int32_t iPacketLen, uint32_t uCurrTick)
{
    uint8_t *pPacketData = (uint8_t *)pPacket;
    NetGamePacketSyncT sync, *pSync;
    NetGamePacketHeadT head;
    uint32_t uTickDiff;

    // does this packet include a sync packet?
    if ((pPacketData[iPacketLen-1] & GAME_PACKET_SYNC) == 0)
    {
        return;
    }

    // extract sync packet
    head.kind = pPacketData[iPacketLen-1];
    head.len = iPacketLen-1;
    if ((pSync = _NetGameLinkGetSync((NetGamePacketT *)&head, pPacketData, &sync)) != NULL)
    {
        // compare currtick with current tick
        uTickDiff = NetTickDiff(uCurrTick, SocketNtohl(sync.echo));

        // refresh echo and repl and write back sync packet
        sync.echo = SocketHtonl(uCurrTick);
        sync.repl = SocketHtonl((SocketNtohl(sync.repl) + uTickDiff));
        ds_memcpy(pSync, &sync, sizeof(sync));

        /* Note:
           Unlike echo and repl, other sync packet fields (psnt, plost, nsnt, prcvd,...) are intentionally not
           refreshed here because they convey update of counts between two sync packets (they are delta values).
           Any update in the values of these counters not included in this sync packet (regardless of this
           packet being resent or not) will be safely covered by the next sync packet. */
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkPollStream

    \Description
        Poll to see if we should send stream data

    \Input *pRef    - reference pointer

    \Output
        int32_t     - send count

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkPollStream(NetGameLinkRefT *pRef)
{
    int32_t iCount = 0, iSize, result;
    NetGameMaxPacketT Packet;
    GameStreamB Block;
    NetGameLinkStreamT *pStream;

    // see if any stream has pending data
    for (pStream = pRef->m_stream; pStream != NULL; pStream = pStream->pNext)
    {
        // see if anything to send
        while (pStream->iOutProg < pStream->iOutSize)
        {
            int32_t iLimit;

            // get data block
            ds_memcpy(&Block, pStream->pOutData+pStream->iOutProg, sizeof(Block));
            // setup data packet
            Packet.head.kind = GAME_PACKET_LINK_STRM;
            // set up packet 'size' field, which is size | subchannel
            iSize = (Block.size & ~0xff000000) | ((Block.iSubchan & 0xff) << 24);
            // store stream header fields in network order
            Packet.body.strm.ident = SocketHtonl(pStream->iIdent);
            Packet.body.strm.kind = SocketHtonl(Block.kind);
            Packet.body.strm.size = SocketHtonl(iSize);
            iSize = Block.size-Block.sent;

            // don't overflow our buffer
            iLimit = (signed)sizeof(Packet.body.strm.data);
            if (iSize > iLimit)
            {
                iSize = iLimit;
            }

            // don't overflow the underlying layer max packet size
            iLimit = pRef->iStrmMaxPktSize;
            if (iSize > iLimit)
            {
                iSize = iLimit;
            }

            ds_memcpy(Packet.body.strm.data, pStream->pOutData+pStream->iOutProg+sizeof(Block)+Block.sent, iSize);
            Packet.head.len = (uint16_t)(sizeof(Packet.body.strm)-sizeof(Packet.body.strm.data)+iSize);
            // try and queue/send the packet
            result = NetGameLinkSend(pRef, (NetGamePacketT *)&Packet, 1);
            if (result < 0)
            {
                NetPrintf(("netgamelink: [0x%08x] stream send failed for stream 0x%08x!\n", pRef, pStream));
                break;
            }
            else if (result == 0)
            {
                break;
            }
            // advance the sent count
            Block.sent += iSize;
            if (Block.sent != Block.size)
            {
                // save back updated block for next time (could just update .sent, but this is easier due to alignment)
                ds_memcpy(pStream->pOutData+pStream->iOutProg, &Block, sizeof(Block));
            }
            else
            {
                pStream->iOutProg += sizeof(Block)+Block.size;
                // see if we should shift the buffer (more than 75% full and shift would gain >33%)
                if ((pStream->iOutProg < pStream->iOutSize) &&
                    (pStream->iOutSize*4 > pStream->iOutMaxm*3) &&
                    (pStream->iOutProg*3 > pStream->iOutMaxm*1))
                {
                    // do the shift
                    memmove(pStream->pOutData, pStream->pOutData+pStream->iOutProg, pStream->iOutSize-pStream->iOutProg);
                    pStream->iOutSize -= pStream->iOutProg;
                    pStream->iOutProg = 0;
                }
            }
            // count the send
            ++iCount;
        }
    }

    // return send count
    return(iCount);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkSendStream

    \Description
        Let user queue up a buffer to send

    \Input *pStream - stream to send on
    \Input iSubChan - subchannel stream is to be received on
    \Input iKind    - kind of data (used to determine if sync)
    \Input *pBuffer - pointer to data to send
    \Input iLength  - length of data to send

    \Output
        int32_t     - negative=error, zero=busy (send again later), positive=sent

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkSendStream(NetGameLinkStreamT *pStream, int32_t iSubChan, int32_t iKind, void *pBuffer, int32_t iLength)
{
    GameStreamB Block;

    // if this is an orphaned stream, return an error
    if (pStream->pClient == NULL)
    {
        return(-1); //todo
    }

    // allow negative length to mean strlen(data)+1
    if (iLength < 0)
    {
        for (iLength = 0; ((char *)pBuffer)[iLength] != '\0'; ++iLength)
            ;
        ++iLength;
    }

    // make sure send buffer does not exceed total input buffer
    // (do this AFTER negative length check)
    if (iLength >= pStream->iInpMaxm)
    {
        return(-1);
    }

    // see if we can reset buffer pointers
    if (pStream->iOutProg >= pStream->iOutSize)
    {
        pStream->iOutProg = pStream->iOutSize = 0;
    }

    // make sure send buffer does not exceed current output buffer
    if (iLength+sizeof(GameStreamB) >= (unsigned)pStream->iOutMaxm-pStream->iOutSize)
    {
        return(0);
    }

    // if this is a sync, make sure we have space in sync buffer
    if ((iKind > '~   ') && (iLength+sizeof(GameStreamB) >= (unsigned)pStream->iSynMaxm-pStream->iSynSize))
    {
        return(0);
    }

    // see if this is a length query
    if (pBuffer == NULL)
    {
        // calc main buffer space
        iLength = pStream->iOutMaxm-pStream->iOutSize;
        // if this is sync, limit based on sync buffer
        if ((iKind > '~   ') && (iLength > pStream->iSynMaxm-pStream->iSynSize))
        {
            iLength = pStream->iSynMaxm-pStream->iSynSize;
        }
        // subtract packet header size
        iLength -= sizeof(GameStreamB);
        if (iLength < 0)
        {
            iLength = 0;
        }
        // return available size
        return(iLength);
    }

    // setup the header block
    Block.kind = iKind;
    Block.sync = 0;
    Block.sent = 0;
    Block.size = iLength;
    Block.iSubchan = iSubChan;

    // if sync, copy into sync buffer
    if (iKind > '~   ')
    {
        // set sync sequence number
        Block.sync = 0;
        // copy into sync byffer
        ds_memcpy(pStream->pSynData+pStream->iSynSize, &Block, sizeof(Block));
        ds_memcpy(pStream->pSynData+pStream->iSynSize+sizeof(Block), pBuffer, iLength);
        pStream->iSynSize += sizeof(Block)+iLength;
    }

    // do a memmove just in case data source is already in this buffer
    // (this is an optimization used for database access)
    memmove(pStream->pOutData+pStream->iOutSize+sizeof(Block), pBuffer, iLength);
    ds_memcpy(pStream->pOutData+pStream->iOutSize, &Block, sizeof(Block));
    pStream->iOutSize += sizeof(Block)+iLength;

    if (pStream->iOutSize > pStream->iHighWaterUsed)
    {
        pStream->iHighWaterUsed = pStream->iOutSize;
    }
    if ((pStream->iOutSize - pStream->iOutProg) > pStream->iHighWaterNeeded)
    {
        pStream->iHighWaterNeeded = pStream->iOutSize - pStream->iOutProg;
    }

    // attempt immediate send
    _NetGameLinkPollStream(pStream->pClient);

    // return send status
    return(iLength);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkRecvStream

    \Description
        Process a received stream packet

    \Input *pRef    - reference pointer
    \Input *pPacket - packet to process

    \Output
        int32_t     - -1=no match, -2=overflow error, 0=valid packet received

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _NetGameLinkRecvStream(NetGameLinkRefT *pRef, NetGamePacketT *pPacket)
{
    int32_t iRes = -1, iSize;
    NetGameLinkStreamT *pStream;
    NetGameLinkStreamInpT *pInp;
    int32_t iSubChannel;
    #if DIRTYCODE_LOGGING
    char strIdent[5];
    strIdent[0] = (char)(pPacket->body.strm.ident >> 24);
    strIdent[1] = (char)(pPacket->body.strm.ident >> 16);
    strIdent[2] = (char)(pPacket->body.strm.ident >> 8);
    strIdent[3] = (char)(pPacket->body.strm.ident);
    strIdent[4] = '\0';
    #endif

    // see if this packet matches any of the streams
    for (pStream = pRef->m_stream; pStream != NULL; pStream = pStream->pNext)
    {
        // see if this is the matching stream
        if (pStream->iIdent == pPacket->body.strm.ident)
        {
            // extract size/subchannel from 'size' member
            iSize = pPacket->body.strm.size & ~0xff000000;
            iSubChannel = (unsigned)pPacket->body.strm.size >> 24;
            if ((iSubChannel < 0) || (iSubChannel >= (pStream->iSubchan+1)))
            {
                NetPrintf(("netgamelink: [0x%08x] warning, received packet on stream '%s' with invalid packet subchannel %d\n", pRef, strIdent, iSubChannel));
                return(-1);
            }
            // ref channel
            pInp = pStream->pInp + iSubChannel;
            // default to overflow error
            iRes = GMDIST_OVERFLOW;
            // validate the packet size
            if (iSize <= pStream->iInpMaxm)
            {
                /* auto-clear packet if anything looks inconsistant -- note that this is expected behavior
                   for the first packet fragment in a sequence of packet fragments */
                if ((pPacket->body.strm.kind != pInp->iInpKind) || (iSize != pInp->iInpSize))
                {
                    // reset progress
                    pInp->iInpProg = 0;
                    // save basic packet header
                    pInp->iInpKind = pPacket->body.strm.kind;
                    pInp->iInpSize = iSize;
                }
                // figure out size of data in packet
                iSize = pPacket->head.len-(sizeof(pPacket->body.strm)-sizeof(pPacket->body.strm.data));
                if (iSize > pInp->iInpSize-pInp->iInpProg)
                {
                    NetPrintf(("netgamelink: [0x%08x] clamped stream packet size from %d to %d on stream '%s'\n", pRef, iSize, pInp->iInpSize-pInp->iInpProg, strIdent));
                    iSize = pInp->iInpSize-pInp->iInpProg;
                }
                // append to existing packet
                ds_memcpy(pInp->pInpData+pInp->iInpProg, pPacket->body.strm.data, iSize);
                pInp->iInpProg += iSize;
                // see if packet is complete
                if (pInp->iInpProg == pInp->iInpSize)
                {
                    // deliver packet
                    if (pStream->Recv != NULL)
                    {
                        pStream->Recv(pStream, iSubChannel, pInp->iInpKind, pInp->pInpData, pInp->iInpSize);
                    }
                    else
                    {
                        NetPrintf(("netgamelink: [0x%08x] no registered stream recv handler on stream '%s'\n", pRef, strIdent));
                    }
                    // clear from buffer
                    pInp->iInpProg = 0;
                }
                // packet was valid
                iRes = 0;
            }
            else
            {
                NetPrintf(("netgamelink: [0x%08x] invalid stream packet size (%d >= %d) on stream '%s'\n",
                    pRef, pPacket->body.strm.size, pStream->iInpMaxm, strIdent));
            }
            return(iRes);
        }
    }

    // didn't find the stream
    NetPrintf(("netgamelink: [0x%08x] could not find stream for stream packet with iIdent '%s'\n", pRef, strIdent));

    // return result
    return(iRes);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkUpdateQos

    \Description
        Send and receive QoS packets.

    \Input *pRef    - reference pointer

    \Version 09/03/14 (mcorcoran)
*/
/*************************************************************************************************F*/
static void _NetGameLinkUpdateQos(NetGameLinkRefT *pRef)
{
    NetGameMaxPacketT QosPacket;
    uint32_t now = NetTick();

    // save QoS start time
    if (pRef->uQosStartTime == 0)
    {
        pRef->uQosStartTime = now;
    }

    // init QoS send time
    if ((pRef->uQosPacketsToSend > 0) && (pRef->uQosLastSendTime == 0))
    {
        /* We are now expecting QoS packet from the other end of the link.
           Make this value non-zero to satisfy the while() condition below. */
        pRef->uQosPacketsToRecv = 1;
        pRef->uQosLastSendTime = now - (pRef->uQosSendInterval+1);
    }

    // check if it's time to send a QoS packet.
    if ((pRef->uQosPacketsToSend > 0) && (NetTickDiff(now, pRef->uQosLastSendTime) > (signed)pRef->uQosSendInterval))
    {
        // set qos last send time (reserve zero for uninitialized)
        pRef->uQosLastSendTime = (now != 0) ? now : now-1;
        pRef->uQosPacketsToSend -= 1;

        // create and send the packet
        QosPacket.head.kind = GAME_PACKET_USER;
        QosPacket.head.len = pRef->uQosPacketSize;

        memset(QosPacket.body.data, 0, QosPacket.head.len);

        // the other end of the link needs to know how many more packets to expect.
        QosPacket.body.data[0] = (uint8_t)(pRef->uQosPacketsToSend >> 8);
        QosPacket.body.data[1] = (uint8_t)(pRef->uQosPacketsToSend >> 0);

        NetGameLinkSend(pRef, (NetGamePacketT*)&QosPacket, 1);
        NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: [0x%08x] sent QoS packet, size(%d), remaining packets to send (%d)\n", pRef, QosPacket.head.len, pRef->uQosPacketsToSend));
    }

    while ((pRef->uQosPacketsToRecv > 0) && NetGameLinkRecv(pRef, (NetGamePacketT*)&QosPacket, 1, FALSE))
    {
        // the other end of the link tells us how many more packets to expect (wait for before transitioning to active)
        pRef->uQosPacketsToRecv =
            (QosPacket.body.data[0] << 8) |
            (QosPacket.body.data[1] << 0);

        NetPrintfVerbose((pRef->iVerbosity, 3, "netgamelink: [0x%08x] received QoS packet, size(%d), remaining packets to recv (%d)\n", pRef, QosPacket.head.len, pRef->uQosPacketsToRecv));
    }

    // have we finished sending and receiving everything yet?
    if ((pRef->uQosPacketsToSend == 0) && (pRef->uQosPacketsToRecv == 0))
    {
        // we're done, print available stats
        _NetGameLinkPrintStats(pRef);
    }

    // only start averaging latency when 1 sample period is complete
    if (NetTickDiff(now, pRef->uQosStartTime) > RTT_SAMPLE_PERIOD)
    {
        pRef->iQosCumulLate += pRef->m_stats.late;
        pRef->iQosLateCount++;
        pRef->iQosAvgLate = pRef->iQosCumulLate / pRef->iQosLateCount;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameLinkDrainRecvStream

    \Description
        Need for streams handling.  Received all data on the stream.

    \Input *pRef    - reference pointer

    \Version 09/03/14 (mcorcoran)
*/
/*************************************************************************************************F*/
static void _NetGameLinkDrainRecvStream(NetGameLinkRefT *pRef)
{
    NetGameMaxPacketT packet;

    while (NetGameLinkRecv2(pRef, (NetGamePacketT*)&packet, 1, 1 << GAME_PACKET_LINK_STRM))
    {
        // convert stream header from network to host order
        packet.body.strm.ident = SocketNtohl(packet.body.strm.ident);
        packet.body.strm.kind = SocketNtohl(packet.body.strm.kind);
        packet.body.strm.size = SocketNtohl(packet.body.strm.size);

        // process the packet
        _NetGameLinkRecvStream(pRef, (NetGamePacketT *)&packet);
    }
}

/*** Public Functions ******************************************************************/


/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkCreate

    \Description
        Construct the game client

    \Input *pCommRef        - the connection from NetGameUtilComplete()
    \Input owner            - if TRUE, NetGameLink will assume ownership of the port (ie, delete it when done)
    \Input buflen           - length of input buffer

    \Output
        NetGameLinkRefT *   - pointer to new NetGameLinkRef

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
NetGameLinkRefT *NetGameLinkCreate(void *pCommRef, int32_t owner, int32_t buflen)
{
    int32_t index;
    uint32_t tick;
    CommRef *port = pCommRef;
    NetGameLinkRefT *ref;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((ref = DirtyMemAlloc(sizeof(*ref), NETGAMELINK_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netgamelink: unable to allocate module state\n"));
        return(NULL);
    }
    memset(ref, 0, sizeof(*ref));
    ref->m_memgroup = iMemGroup;
    ref->m_memgrpusrdata = pMemGroupUserData;

    // assign port info
    ref->m_port = port;
    ref->m_portowner = owner;

    // allocate input buffer
    if (buflen < 4096)
    {
        buflen = 4096;
    }
    ref->m_buffer = DirtyMemAlloc(buflen, NETGAMELINK_MEMID, ref->m_memgroup, ref->m_memgrpusrdata);
    ref->m_bufmax = buflen;

    // reset the timing info
    ref->m_rttavg = 0;
    ref->m_rttdev = 0;

    // setup connection time for reference
    tick = NetTick();
    ref->m_stats.conn = tick;
    ref->m_stats.senthide = tick;
    ref->m_stats.rcvdhide = tick;
    ref->m_stats.rcvdlast = tick;
    ref->m_stats.sentlast = tick;
    ref->m_stats.stattick = tick;
    ref->m_stats.isconn = FALSE;
    ref->m_stats.isopen = FALSE;

    // fill ping history with bogus starting data
    for (index = 0; index < PING_HISTORY; ++index)
    {
        ref->m_stats.pinghist[index].min = PING_DEFAULT;
        ref->m_stats.pinghist[index].max = PING_DEFAULT;
        ref->m_stats.pinghist[index].avg = PING_DEFAULT;
        ref->m_stats.pinghist[index].cnt = 1;
    }

    // setup critical section
    NetCritInit(&ref->crit, "netgamelink");

    // setup for callbacks
    port->refptr = ref;
    port->Callback(port, _NetGameLinkNotify);
    port->SendCallback = _NetGameLinkSendCallback;

    // default sending and syncs to enabled
    ref->m_sendenabled = TRUE;
    ref->m_syncenabled = TRUE;

    //todo have this grow with mwid
    ref->iStrmMaxPktSize = NETGAME_STRMPKT_DEFSIZE;

    // other QoS items memset() to zero above
    ref->uQosSendInterval = NETGAME_QOS_INTERVAL_MIN;
    ref->uQosPacketSize = NETGAME_DATAPKT_MAXSIZE;

    ref->iVerbosity = 1;

    return(ref);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkDestroy

    \Description
        Destruct the game client

    \Input *ref     - reference pointer

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameLinkDestroy(NetGameLinkRefT *ref)
{
    // dont need callback
    ref->m_port->Callback(ref->m_port, NULL);

    // we own the port -- get rid of it
    if (ref->m_portowner)
        ref->m_port->Destroy(ref->m_port);

    // free receive buffer
    DirtyMemFree(ref->m_buffer, NETGAMELINK_MEMID, ref->m_memgroup, ref->m_memgrpusrdata);

    // free critical section
    NetCritKill(&ref->crit);

    // free our memory
    DirtyMemFree(ref, NETGAMELINK_MEMID, ref->m_memgroup, ref->m_memgrpusrdata);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkCallback

    \Description
        Register a callback function

    \Input *ref         - reference pointer
    \Input *calldata    - caller reference data
    \Input *callproc    - callback function

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameLinkCallback(NetGameLinkRefT *ref, void *calldata, void (*callproc)(void *calldata, int32_t kind))
{
    ref->m_calldata = calldata;
    ref->m_callproc = callproc;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkStatus

    \Description
        Return current link status

    \Input *pRef        - reference pointer
    \Input iSelect      - selector
    \Input iValue       - input value
    \Input pBuf         - output buffer
    \Input iBufSize     - output buffer size

    \Output
        int32_t         - selector specific return value

    \Notes
        iSelect can be one of the following:

        \verbatim
            'qlat' - average latency calculated during the initial qos phase
            'sinf' - returns SocketInfo() with iValue being the status selector (iData param is unsupported)
            'stat' - Fills out a NetGameLinkStatT with QoS info. pBuf=&NetGameLinkStatT, iBufSize=sizeof(NetGameLinkStatT)
        \endverbatim

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkStatus(NetGameLinkRefT *pRef, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'qlat')
    {
        return(pRef->iQosAvgLate);
    }
    if ((iSelect == 'sinf') && (pRef->m_port != NULL) && (pRef->m_port->sockptr != NULL))
    {
        return(SocketInfo(pRef->m_port->sockptr, iValue, 0, pBuf, iBufSize));
    }
    if (iSelect == 'stat')
    {
        volatile uint32_t seqn;
        uint32_t portstat;

        do
        {
            // remember pre-update ticks
            seqn = pRef->m_stats.tickseqn;

            // update tick is same as movement tick
            pRef->m_stats.tick = NetTick();

        // if things changed during out assignment, do it again
        } while (pRef->m_stats.tickseqn != seqn);

        // update port status
        portstat = pRef->m_port->Status(pRef->m_port);
        pRef->m_stats.isopen = ((portstat == COMM_CONNECTING) || (portstat == COMM_ONLINE)) ? TRUE : FALSE;
        pRef->m_stats.isopen = pRef->m_stats.isopen && (pRef->uQosPacketsToSend == 0) && (pRef->uQosPacketsToRecv == 0);

        // update packet sent counter
        pRef->m_stats.lpacksent_now = pRef->m_port->packsent;

        // make sure user-provided buffer is large enough to receive a pointer
        if ((pBuf != NULL) && (iBufSize >= (int32_t)sizeof(NetGameLinkStatT)))
        {
            ds_memcpy(pBuf, &pRef->m_stats, sizeof(NetGameLinkStatT));
            return(0);
        }
        else
        {
            // unhandled
            return(-1);
        }
    }
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkSend

    \Description
        Handle incoming packet stream from upper layer

    \Input *ref     - reference pointer
    \Input *pkt     - packet list (1 or more)
    \Input len      - size packet list (1=one packet)

    \Output
        int32_t     - negative=bad error, zero=unable to send now (overflow), positive=bytes sent

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkSend(NetGameLinkRefT *ref, NetGamePacketT *pkt, int32_t len)
{
    int32_t cnt = 0, err;
    uint32_t currtick = NetTick();

    // get exclusive access
    NetCritEnter(&ref->crit);

    // see if we need to handle missed event
    if (ref->m_process > 0)
    {
        NetPrintf(("netgamelink: processing missed event\n"));
        _NetGameLinkProcess(ref, currtick);
    }

    // walk the packet list
    while (len > 0)
    {
        if ((err = _NetGameLinkSendPacket(ref, pkt, currtick)) <= 0)
        {
            // don't spam on overflow
            if (err != 0)
            {
                NetPrintf(("netgamelink: error %d sending packet\n", err));
            }
            // should _NetGameLinkSendPacket fail at first send attempt, return err to caller.
            if (cnt == 0)
            {
                cnt = err;
            }
            break;
        }

        // calc packet size for them
        pkt->head.size = (sizeof(pkt->head)+pkt->head.len+sizeof(NetGamePacketSyncT)+3)&0x7ffc;
        // count the packet size
        cnt += pkt->head.size;
        // stop if this was single packet
        if (len == 1)
        {
            break;
        }

        // skip to next packet
        len -= pkt->head.size;
        pkt = (NetGamePacketT *)(((char *)pkt)+pkt->head.size);
    }

    // release exclusive access
    NetCritLeave(&ref->crit);

    // return bytes sent
    return(cnt);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkPeek2

    \Description
        Peek into the buffer, for the first packet type matching mask

    \Input *ref     - reference pointer
    \Input **pkt    - (optional) storage for pointer to current packet data
    \Input mask     - which packet types we want. bitmask, addressed by GAME_PACKET values

    \Output
        int32_t     - buffer size

    \Version 08/08/11 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkPeek2(NetGameLinkRefT *ref, NetGamePacketT **pkt, uint32_t mask)
{
    int32_t iBufLen = ref->m_buflen;
    int32_t idx, iKind;
    NetGamePacketT *pkt0;
    int32_t pktsize = 0;

    // I do not understand the reason for && (iBufLen > 0).
    // Kept to maintain current behaviour
    // TODO: investigate
    if ((pkt != NULL) && (iBufLen > 0))
    {
        // while going through our buffer of received packets
        for (idx = 0;idx < ref->m_buflen;)
        {
            // extract size of current head packet
            pkt0 = (NetGamePacketT *)(ref->m_buffer + idx);
            pktsize = pkt0->head.size;

            // if the current packet matches what we want

            iKind = (((NetGamePacketT *)(ref->m_buffer + idx))->head.kind) & ~GAME_PACKET_SYNC;

            if (mask & (1 << iKind))
            {
                *pkt = pkt0;
                break;
            }
            else
            {
                // skip over the non-matching packets.
                idx += pktsize;
            }
        }
    }
    else if (pkt != NULL)
    {
        *pkt = NULL;
    }


    // return buffer size
    return(iBufLen);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkPeek

    \Description
        Peek into the buffer

    \Input *ref     - reference pointer
    \Input **pkt    - (optional) storage for pointer to current packet data

    \Output
        int32_t     - buffer size

    \Version 12/19/00 (GWS)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkPeek(NetGameLinkRefT *ref, NetGamePacketT **pkt)
{
    return(NetGameLinkPeek2(ref, pkt, (unsigned)~0));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkRecv2

    \Description
        Outgoing packet stream to upper layer

    \Input *ref     - reference pointer
    \Input *buf     - storage for packet list (1 or more)
    \Input len      - size packet list (1=one packet)
    \Input mask     - which packet types we wanted. bitmask, addressed by GAME_PACKET values

    \Output
        int32_t     - 0=no data available, else number of packets received

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkRecv2(NetGameLinkRefT *ref, NetGamePacketT *buf, int32_t len, uint32_t mask)
{
    NetGamePacketT *pkt;
    uint32_t currtick = NetTick();
    int32_t idx, iKind;
    int32_t lenread = 0;
    int32_t pktsize = 0;

    // disable callback
    NetCritEnter(&ref->crit);

    // if no data in queue, check with lower layer
    if (ref->m_buflen == 0)
    {
        while (_NetGameLinkRecvPacket(ref, currtick) > 0)
            ;
    }

    // see if there is anything to process
    if (ref->m_buflen == 0)
    {
        len = 0;
    }
    else
    {
        // while going through our buffer of received packets
        for (idx = 0;idx < ref->m_buflen;)
        {
            // extract size of current head packet
            pkt = (NetGamePacketT *)(ref->m_buffer + idx);
            pktsize = pkt->head.size;

            // don't access pkt past here, as the memmove will overwrite it

            // if the current packet matches what we want

            iKind = (((NetGamePacketT *)(ref->m_buffer + idx))->head.kind) & ~GAME_PACKET_SYNC;

            if (mask & (1 << iKind))
            {
                // if we can fit it in
                if ((pktsize <= len) || (len == 1))
                {
                    // copy the packet to return buffer
                    ds_memcpy(buf, ref->m_buffer + idx, pktsize);
                    len -= pktsize;
                    buf = (NetGamePacketT *)(((char*)buf) + pktsize);
                    lenread += pktsize;

                    // remove from our list of received packets
                    memmove(ref->m_buffer + idx, ref->m_buffer + idx + pktsize, ref->m_buflen - idx - pktsize);
                    ref->m_buflen -= pktsize;

                    // if len was 1, it's now negative, we got our packet, bail out
                    if (len < 0)
                    {
                        break;
                    }
                }
                else
                {
                    // bail out, we got all we could
                    break;
                }
            }
            else
            {
                // skip over the non-matching packets.
                idx += pktsize;
            }
        }
    }

    // enable access
    NetCritLeave(&ref->crit);

    // return length
    return(lenread);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkRecv

    \Description
        Outgoing packet stream to upper layer

    \Input *ref     - reference pointer
    \Input *buf     - storage for packet list (1 or more)
    \Input len      - size packet list (1=one packet)
    \Input bDist    - whether dist packets are to be received. Use FALSE.

    \Output
        int32_t     - 0=no data available, else number of packets received

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkRecv(NetGameLinkRefT *ref, NetGamePacketT *buf, int32_t len, uint8_t bDist)
{
    uint32_t uDistMask =    (1 << GAME_PACKET_INPUT) |
                            (1 << GAME_PACKET_INPUT_MULTI) |
                            (1 << GAME_PACKET_INPUT_MULTI_FAT) |
                            (1 << GAME_PACKET_STATS) |
                            (1 << GAME_PACKET_INPUT_FLOW) |
                            (1 << GAME_PACKET_INPUT_META);

    return(NetGameLinkRecv2(ref, buf, len, (bDist ? uDistMask : ~uDistMask) & ~(1 << GAME_PACKET_LINK_STRM)));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkControl

    \Description
        NetGameLink control function.  Different selectors control different behaviors.

    \Input *ref     - reference pointer
    \Input iSelect  - status selector (send, sdpt)
    \Input iValue   - selector-specific value
    \Input pValue   - selector-specific pointer

    \Output
        int32_t         - selector specific, or negative if failure

    \Notes
        selector inputs:

        \verbatim
            rlmt: redundant packet size limit
            send: iValue==TRUE to enable sending, iData==FALSE to disable
            slen: none
            spam: iValue== new verbosity level
            sque: none
            sync: iValue==FALSE to disable sync packets, TRUE to enable (default)
        \endverbatim

        selector outputs:

        \verbatim
            rlim: 0
            send: TRUE
            slen: current length of send queue
            spam: TRUE if iValue within range 0-5, FALSE otherwise
            sque: TRUE if send queue is empty, else FALSE
            sync: zero
        \endverbatim

        Unhandled selectors are passed through to the underlying comm module

    \Version 07/07/03 (jbrookes)
*/
/*************************************************************************************************F*/
int32_t NetGameLinkControl(NetGameLinkRefT *ref, int32_t iSelect, int32_t iValue, void *pValue)
{
    // $$todo read mwid field -- this should be a NetGameLinkStatus() selector in V8
    if (iSelect == 'mwid')
    {
        return(ref->m_port->maxwid);
    }
    // set redundant packet size limit
    if (iSelect == 'rlmt')
    {
        NetPrintf(("netgamelink: [0x%08x] updating redundant packet size limit for underlying comm (%d)\n", ref, iValue));
        return(ref->m_port->Control(ref->m_port, iSelect, iValue, pValue));
    }
    // set send/recv value
    if (iSelect == 'send')
    {
        ref->m_sendenabled = iValue;
        return(1);
    }
    if (iSelect == 'spam')
    {
        if (iValue >= 0 && iValue <= 5)
        {
            ref->iVerbosity = iValue;
            return(1);
        }
        return(0);
    }
    // queue checks
    if ((iSelect == 'sque') || (iSelect == 'slen'))
    {
        // get output queue position
        int32_t iQueue = ref->m_port->Send(ref->m_port, NULL, 0, COMM_FLAGS_RELIABLE);

        if (iSelect == 'sque')
        {
            // if output queue position is one send queue is empty (assume error=empty)
            iQueue = (iQueue > 0) ? (iQueue == 1) : TRUE;
        }
        return(iQueue);
    }
    // enable/disable sync packets
    if (iSelect == 'sync')
    {
        ref->m_syncenabled = iValue;
        return(0);
    }
    // set QoS duration and interval
    if (iSelect == 'sqos')
    {
        int32_t iValue2 = *(int32_t*)pValue;

        if (ref->uQosLastSendTime != 0)
        {
            NetPrintf(("netgamelink: [0x%08x] Cannot change QoS duration or interval while QoS is in progress or is already finished.", ref));
            return(-1);
        }

        if ((iValue < NETGAME_QOS_DURATION_MIN) || (iValue > NETGAME_QOS_DURATION_MAX))
        {
            NetPrintf(("netgamelink: [0x%08x] Invalid QoS duration period provided (%d). QoS duration must be >= %d and <= %s ms\n", ref, iValue, NETGAME_QOS_DURATION_MIN, NETGAME_QOS_DURATION_MAX));
            iValue = (iValue < NETGAME_QOS_DURATION_MIN ? NETGAME_QOS_DURATION_MIN : iValue);
            iValue = (iValue > NETGAME_QOS_DURATION_MAX ? NETGAME_QOS_DURATION_MAX : iValue);
        }
        if ((iValue != 0) && ((iValue2 < NETGAME_QOS_INTERVAL_MIN) || (iValue2 > iValue)))
        {
            NetPrintf(("netgamelink: [0x%08x] Invalid QoS interval provided (%d). QoS interval must be >= %d and <= QoS duration period\n", ref, iValue2, NETGAME_QOS_INTERVAL_MIN));
            iValue2 = (iValue2 < NETGAME_QOS_INTERVAL_MIN ? NETGAME_QOS_INTERVAL_MIN : iValue2);
            iValue2 = (iValue2 > iValue ? iValue : iValue2);
        }
        if (iValue == 0)
        {
            NetPrintf(("netgamelink: [0x%08x] QoS for NetGameLink connections is disabled\n", ref));
        }
        NetPrintf(("netgamelink: [0x%08x] QoS duration period set to %d ms\n", ref, iValue));
        NetPrintf(("netgamelink: [0x%08x] QoS interval set to %d ms\n", ref, iValue2));

        ref->uQosSendInterval = iValue2;
        ref->uQosPacketsToSend = (iValue == 0 ? 0 : iValue / iValue2);
        return(0);
    }
    // set individual QoS packet size
    if (iSelect == 'lqos')
    {
        if ((iValue < NETGAME_QOS_PACKETSIZE_MIN) || (iValue > NETGAME_DATAPKT_MAXSIZE))
        {
            NetPrintf(("netgamelink: [0x%08x] Invalid QoS packet size specified (%d). QoS packets must be >= %d and <= %d\n", ref, iValue, NETGAME_QOS_PACKETSIZE_MIN, NETGAME_DATAPKT_MAXSIZE));
            iValue = (iValue < NETGAME_QOS_PACKETSIZE_MIN ? NETGAME_QOS_PACKETSIZE_MIN : iValue);
            iValue = (iValue > NETGAME_DATAPKT_MAXSIZE ? NETGAME_DATAPKT_MAXSIZE : iValue);
        }
        NetPrintf(("netgamelink: [0x%08x] QoS packet size set to %d\n", ref, iValue));
        ref->uQosPacketSize = iValue;
        return(0);
    }
    // unhandled; pass through to comm module if available
    return((ref->m_port != NULL) ? ref->m_port->Control(ref->m_port, iSelect, iValue, pValue) : -1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkUpdate

    \Description
        Provides NetGameLink with time, at regular interval. Need for streams handling.
        Also handles QoS stage when during link startup.

    \Input *pRef    - reference pointer

    \Output
        uint32_t    - zero

    \Version 01/11/10 (jrainy)
*/
/*************************************************************************************************F*/
uint32_t NetGameLinkUpdate(NetGameLinkRefT *pRef)
{
    // Are we currently in the QoS phase?
    if ((pRef->uQosPacketsToSend > 0) || (pRef->uQosPacketsToRecv > 0))
    {
        // Send and receive QoS packets
        _NetGameLinkUpdateQos(pRef);
    }
    else
    {
        // Handle normal link data
        _NetGameLinkDrainRecvStream(pRef);

        _NetGameLinkPollStream(pRef);
    }

    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameLinkCreateStream

    \Description
        Allocate a stream

    \Input *pRef            - module state reference
    \Input iSubChan         - number of subchannels (zero for a normal stream)
    \Input iIdent           - unique stream iIdent
    \Input iInpLen          - size of input buffer
    \Input iOutLen          - size of output buffer
    \Input iSynLen          - size of sync buffer

    \Output
        NetGameLinkStreamT * - new stream, or NULL if the iIdent was not unique

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
NetGameLinkStreamT *NetGameLinkCreateStream(NetGameLinkRefT *pRef, int32_t iSubChan, int32_t iIdent, int32_t iInpLen, int32_t iOutLen, int32_t iSynLen)
{
    NetGameLinkStreamT *pStream;
    int32_t iInpSize;
    char *pInpData;

    // make sure the pipe identifier is unique
    for (pStream = pRef->m_stream; pStream != NULL; pStream = pStream->pNext)
    {
        // dont create a duplicate stream
        if (pStream->iIdent == iIdent)
        {
            NetPrintf(("netgamelink: [0x%08x] error -- attempting to create duplicate stream '%c%c%c%c'\n",
                pRef, (uint8_t)(iIdent>>24), (uint8_t)(iIdent>>16), (uint8_t)(iIdent>>8), (uint8_t)iIdent));
            return(NULL);
        }
    }

    // allocate a pipe record
    if ((pStream = (NetGameLinkStreamT *) DirtyMemAlloc(sizeof(*pStream), NETGAMELINK_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata)) == NULL)
    {
        NetPrintf(("netgamelink: [0x%08x] unable to allocate stream\n", pRef));
        return(NULL);
    }
    memset(pStream, 0, sizeof(*pStream));

    // setup the data fields
    pStream->pClient = pRef;
    pStream->iIdent = iIdent;
    pStream->iSubchan = iSubChan;
    pStream->Send = _NetGameLinkSendStream;
    pStream->Recv = NULL;

    // allocate input buffer plus input buffer tracking structure(s)
    pStream->iInpMaxm = iInpLen;
    iInpSize = sizeof(*pStream->pInp) * (pStream->iSubchan + 1);
    pStream->pInp = DirtyMemAlloc(iInpSize + (iInpLen * (pStream->iSubchan + 1)), NETGAMELINK_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata);
    memset(pStream->pInp, 0, iInpSize);
    for (iSubChan = 0, pInpData = (char *)pStream->pInp + iInpSize; iSubChan < pStream->iSubchan+1; iSubChan++)
    {
        pStream->pInp[iSubChan].pInpData = pInpData;
        pInpData += iInpLen;
    }

    // allocate output buffer
    if (iOutLen < iInpLen)
    {
        iOutLen = iInpLen;
    }
    pStream->iOutMaxm = iOutLen;
    pStream->pOutData = DirtyMemAlloc(iOutLen, NETGAMELINK_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata);

    // allocate sync buffer
    pStream->iSynMaxm = iSynLen;
    if (iSynLen > 0)
    {
        pStream->pSynData = DirtyMemAlloc(iSynLen, NETGAMELINK_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata);
    }

    // put into list
    pStream->pNext = pRef->m_stream;
    pRef->m_stream = pStream;
    return(pStream);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistDestroyStream

    \Description
        Destroy a stream

    \Input *pRef    - reference pointer
    \Input *pStream - pointer to stream to destroy

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameLinkDestroyStream(NetGameLinkRefT *pRef, NetGameLinkStreamT *pStream)
{
    NetGameLinkStreamT **link;

    // make sure stream is valid
    if (pStream != NULL)
    {
        // if stream is active, remove the link
        if (pStream->pClient != NULL)
        {
            // locate the stream in the list for removal
            for (link = &pStream->pClient->m_stream; *link != pStream; link = &((*link)->pNext))
            {
                // see if at end of list
                if (*link == NULL)
                {
                    return;
                }
            }
            // remove stream from list
            *link = pStream->pNext;
        }

        // now the tricky part-- make sure nobody is still processing this stream

        // release the resources
        if (pStream->pSynData != NULL)
        {
            DirtyMemFree(pStream->pSynData, NETGAMELINK_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata);
        }
        DirtyMemFree(pStream->pInp, NETGAMELINK_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata);
        DirtyMemFree(pStream->pOutData, NETGAMELINK_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata);
        DirtyMemFree(pStream, NETGAMELINK_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata);
    }
}

