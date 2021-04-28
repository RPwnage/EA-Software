/*H*************************************************************************************************/
/*!

    \File    netgamedist.c

    \Description
        This file provides some upper layer protocol abstractions such as controller packet
        buffering and exchange logic.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2000-2002.  ALL RIGHTS RESERVED.

    \Version    1.0        12/20/00 (GWS) Based on split of GmClient.c
    \Version    1.1        12/31/01 (GWS) Cleaned up and made really platform independent
    \Version    1.2        12/03/09 (mclouatre) Added configurable run-time verbosity
*/
/*************************************************************************************************H*/


/*** Include files *********************************************************************/

#include <string.h>
#include <stdio.h>

#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamedist.h"

/*** Defines ***************************************************************************/

#define NETGAMEDIST_VERBOSITY (2)

#define TIMING_DEBUG                    (0)
#define PING_DEBUG                      (0)
#define INPUTCHECK_LOGGING_DELAY        (15)    // 15 msec
#define GMDIST_META_ARRAY_SIZE          (32)    // how many past versions of sparse multipacket to keep

// PACKET_WINDOW can be overriden at build time with the nant global property called dirtysdk-distpktwindow-size
#ifndef PACKET_WINDOW
#define PACKET_WINDOW                   (64)
#endif

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

//! Describe an entry in the flat buffer.
typedef struct GameBufferLookupB
{
    uint32_t pos; //indexes into m_out.m_ctrlio or m_inp.m_ctrlio
    uint16_t len;
    uint16_t lensize;
} GameBufferLookupB;

//! Flat buffer structure. Implements a wrapping queue of packets.
typedef struct GameBufferDataT
{
    //! incoming and outgoing controller packets
    unsigned char *m_ctrlio;
    //! length of the io buffer
    uint32_t m_buflen;
    // input/output lookup, addresses into m_ctrlio
    GameBufferLookupB m_lkup[PACKET_WINDOW];
    //! index of first packet to send/process in m_ctrlio
    int32_t m_beg;
    //! index of last packet to send/process in m_ctrlio
    int32_t m_end;
} GameBufferDataT;

//! Describes one version of multipacket.
typedef struct NetGameDistMetaInfoT
{
    //! which entries are used
    uint32_t uMask;
    //! number of players used
    uint8_t uPlayerCount;
    //! version number
    uint8_t uVer;
} NetGameDistMetaInfoT;

//! netgamedist internal state
struct NetGameDistRefT
{
    //! module memory group
    int32_t m_memgroup;
    void *m_memgrpusrdata;

    //! output buffer for packets and lookup table
    GameBufferDataT m_out;
    //! input buffer for packets and lookup table
    GameBufferDataT m_inp;

    //! offset between the input and output queues
    int32_t m_iooffset;

    //! local sequence number
    uint32_t m_localseq;
    //! global sequence number
    uint32_t m_globalseq;

    //! external status monitoring
    NetGameLinkStatT m_stats;

    //! current exchange rate
    int32_t m_inprate;
    //! input exchange window
    int32_t m_inpwind;
    //! clamp the min window size
    int32_t m_inpmini;
    //! clamp the max window size
    int32_t m_inpmaxi;

    //! when to recalc window
    uint32_t m_inpcalc;
    //! when packet was last send
    uint32_t m_inpnext;

    //! netgamelink ref
    void *linkref;
    //! netgamelink stat func
    NetGameDistStatProc *statproc;
    //! netgame send function
    NetGameDistSendProc *sendproc;
    //! netgame recv function
    NetGameDistRecvProc *recvproc;

    NetGameDistDropProc *dropproc;
    NetGameDistPeekProc *peekproc;

    NetGameDistLinkCtrlProc *linkctrlproc;

    //! the index of the local player
    uint32_t m_ourindex;
    //! the total number of players
    uint32_t m_totplrs;

    //! whether we are sending/receiving multi packets
    uint32_t m_recvmulti;
    uint32_t m_sendmulti;

    //! a max packet for use by input
    NetGameMaxPacketT m_maxpkt;

    //! a max packet for use by input
    char m_multibuf[NETGAME_DATAPKT_MAXSIZE];

    //! the number of writes to a position in the input queue, (dropproc)
    int32_t m_packetid[PACKET_WINDOW];

    //! latest stats received by each pClient
    NetGameDistStatT m_recvstats[GMDIST_MAX_CLIENTS];

    //! Error condition. Set during calls like update, if an error occurs.
    int32_t m_errcond;
    int32_t iCount;
    int32_t iLastSentDelta;
    uint32_t m_uSkippedInputCheckLogCount;
    uint32_t m_uLastInputCheckLogTick;
    int32_t iStrmMaxPktSize;

    char m_errortext[GMDIST_ERROR_SIZE];

    //! whether we must update the flow control flags to the game server
    uint8_t m_updateflowctrl;

    //! the range of meta info we must update the
    uint8_t m_updatemetainfobeg;
    uint8_t m_updatemetainfoend;

    //! whether we are ready to send or not
    uint8_t m_rdysend;

    //! whether we are ready to receive or not
    uint8_t m_rdyrecv;

    //! whether remote is ready to send or not
    uint8_t m_rdysendremote;

    //! whether remote is ready to receive or not
    uint8_t m_rdyrecvremote;

    uint32_t m_localCRC;
    uint8_t m_localCRCvalid;

    uint32_t m_remoteCRC;
    uint8_t m_remoteCRCvalid;

    //! debug output verbosity
    uint8_t iVerbose;

    //! boolean indicating whether we want to surface CRC cahllenges from the GS
    uint8_t m_ucrcchallenges;

    //! boolean indicating whether we received meta information on the packet layout
    uint8_t m_gotMetaInfo;

    //! boolean indicating whether we are sending sparse multi-packets
    uint8_t m_uSparse;

    //! Meta information about sparse multi-packets to send
    NetGameDistMetaInfoT m_metaInfoToSend[GMDIST_META_ARRAY_SIZE];

    //! Received meta information about sparse multi-packets (wraps around)
    NetGameDistMetaInfoT m_metaInfo[GMDIST_META_ARRAY_SIZE];

    //! The version meta information from the last peeked or queried packet
    uint32_t uLastQueriedVersion;
};

/*** Function Prototypes ***************************************************************/

/*** Variables *************************************************************************/

// Private variables

#if DIRTYCODE_LOGGING
// The following is meant to be indexed with the corresponding constants defined in netgamedist.h.
// Array contents need to be tailored if constants are removed, modified or added.
static char _strNetGameDistDataTypes[6][32] =
    {"INVALID",
     "GMDIST_DATA_NONE",
     "GMDIST_DATA_INPUT",
     "GMDIST_DATA_INPUT_DROPPABLE",
     "GMDIST_DATA_DISCONNECT",
     "GMDIST_DATA_NODATA"};
#endif

// Public variables


/*** Private Functions *****************************************************************/


#if PING_DEBUG
/*F*************************************************************************************************/
/*!
    \Function    _PingHistory

    \Description
        Display the uPing history [DEBUG only]

    \Input *pStats      - to be completed

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
static void _PingHistory(const NetGameLinkStatT *pStats)
{
    int32_t iPing;
    int32_t iIndex;
    char strMin[64];
    char strMax[64];
    char strAvg[64];
    char strCnt[64];
    const GameLinkHistT *pHist;
    static uint32_t iPrev = 0;

    // see if its time
    if (pStats->pingslot == iPrev)
    {
        return;
    }
    iPrev = pStats->pingslot;

    iPing = 0;

    for (iIndex = 0; iIndex < PING_HISTORY; ++iIndex)
    {
        pHist = pStats->pinghist + ((pStats->pingslot - iIndex) & (PING_HISTORY-1));
        strMin[iIndex] = '0'+pHist->min/50;
        strMax[iIndex] = '0'+pHist->max/50;
        strAvg[iIndex] = '0'+pHist->avg/50;
        strCnt[iIndex] = '0'+pHist->cnt;

        iPing += pHist->avg;
    }
    strMin[iIndex] = 0;
    strMax[iIndex] = 0;
    strAvg[iIndex] = 0;
    strCnt[iIndex] = 0;

    iPing /= PING_HISTORY;

    NetPrintf(("history(%d/%d): ping=%d, late=%d, calc=%d\n", pStats->pingslot, pStats->pingtick, pStats->ping, pStats->late, iPing));
    NetPrintf((" %s\n", strMin));
    NetPrintf((" %s\n", strMax));
    NetPrintf((" %s\n", strAvg));
    NetPrintf((" %s\n", strCnt));
}
#endif

/*F*************************************************************************************************/
/*!
    \Function    _Modulo

    \Description
        return positive modulo.

    \Input x        - value to divide
    \Input y        - divider

    \Output
        int32_t     - positive remainder of x/y

    \Version    1.0        03/05/07 (jrainy) First Version
*/
/*************************************************************************************************F*/
static int32_t _Modulo(int32_t x, int32_t y)
{
    return(((x % y) + y) % y);
}

/*F*************************************************************************************************/
/*!
    \Function    _SetDataPtrTry

    \Description
        returns the position in the input or output buffer for addition of a packet of length uLength.

    \Input *pRef    - reference pointer
    \Input *pBuffer - buffer pointer
    \Input uLength  - length of the next packet to be stored

    \Output
        uint8_t     - pos if a space was found. -1 if the buffer cannot accomodate uLength

    \Version 02/08/07 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t _SetDataPtrTry(NetGameDistRefT *pRef, GameBufferDataT *pBuffer, uint16_t uLength)
{
    uint32_t indexA,indexB;
    uint32_t posA,posB;

    indexA = _Modulo(pBuffer->m_end - 1, PACKET_WINDOW);
    posA = pBuffer->m_lkup[indexA].pos + pBuffer->m_lkup[indexA].len;

    indexB = pBuffer->m_beg;
    posB = pBuffer->m_lkup[indexB].pos;

    posA = ((posA+3)&~3); // aligns posA to the next 4-byte boundary

    if ((posA >= posB) || (pBuffer->m_beg == pBuffer->m_end))
    {
        // if we can't fit at the end, let's retry from the beginning
        if ((posA + uLength) > pBuffer->m_buflen)
        {
            posA = 0;
            // fall trough to the next 'if'
        }
        else
        {
            return(posA);
        }
    }
    if (posB >= posA)
    {
        if (((posA + uLength) >= posB) && (pBuffer->m_beg != pBuffer->m_end))
        {
            ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "overflow in _SetDataPtrTry. indexA was %d, indexB was %d, posA was %d, posB was %d, ", indexA, indexB, pBuffer->m_lkup[indexA].pos + pBuffer->m_lkup[indexA].len, posB);

            NetPrintf(("netgamedist: [0x%08x] input/output buffer full.\n", pRef));
            return(-1);
        }
        else
        {
            return(posA);
        }
    }

    ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "_SetDataPtrTry reached unreachable code.");

    // unreachable code. If we get here something went terribly wrong.
    return(-1);
}

/*F*************************************************************************************************/
/*!
    \Function    _SetDataPtr

    \Description
        Prepares the input or output buffer for addition of a packet of length uLength.

    \Input *pRef    - reference pointer
    \Input *pBuffer - buffer pointer
    \Input uLength  - length of the next packet to be stored

    \Output
        uint8_t     - TRUE if a space was found. FALSE if the buffer cannot accomodate uLength

    \Version 02/08/07 (jrainy)
*/
/*************************************************************************************************F*/
static uint8_t _SetDataPtr(NetGameDistRefT *pRef, GameBufferDataT *pBuffer, uint16_t uLength)
{
    int32_t pos = _SetDataPtrTry(pRef, pBuffer, uLength);

    if (pos != -1)
    {
        pBuffer->m_lkup[pBuffer->m_end].pos = pos;
        return(TRUE);
    }

    return(FALSE);
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistCheckWindow

    \Description
        Handle the window stretching. If we have too many unacknowledged packets, we start sending less often

    \Input *pRef    - The NetGameDist ref
    \Input iRemain  - Remaining frame time (before window stretching)
    \Input uTick    - Current tick

    \Output
        none

    \Version    1.0        12/21/09 (jrainy) First Version
*/
/*************************************************************************************************F*/
static void _NetGameDistCheckWindow(NetGameDistRefT *pRef, int32_t iRemain, uint32_t uTick)
{
    int32_t iQueue;

    if (!pRef->m_recvmulti)
    {
        // stretch cycle if we are over window
        iQueue = _Modulo(pRef->m_out.m_end - _Modulo(pRef->m_inp.m_beg + pRef->m_iooffset, PACKET_WINDOW), PACKET_WINDOW);
        if ((iQueue > pRef->m_inpwind) && (iRemain < pRef->m_inprate/2))
        {
            #if TIMING_DEBUG
            // dont show single cycle adjustments
            if (uTick+pRef->m_inprate/2-pRef->m_inpnext != 1)
            {
                NetPrintf(("netgamedist: [0x%08x] stretching cycle (que=%d, win=%d, tick=%d, add=%d)\n",
                     pRef, iQueue, pRef->m_inpwind, uTick, (uTick+pRef->m_inprate/2)-pRef->m_inpnext);
            }
            #endif
            // stretch the next send
            pRef->m_inpnext = uTick+pRef->m_inprate/2;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistUpdateSendTime

    \Description
        Update the next send time after sending each packet. In p2p mode, clamp the next send to
        no earlier than half a frame after *now* and no later than two frames after *now*

    \Input *pRef    - The NetGameDist ref

    \Output
        none

    \Version    1.0        12/21/09 (jrainy) First Version
*/
/*************************************************************************************************F*/
static void _NetGameDistUpdateSendTime(NetGameDistRefT *pRef)
{
    uint32_t uTick;
    int32_t iNext;
    // get current time

    uTick = NetTick();
    // record the send time so we can schedule next packet
    pRef->m_inpnext += pRef->m_inprate;

    if (!pRef->m_recvmulti)
    {
        // figure time till next send
        iNext = pRef->m_inpnext - uTick;
        // clamp the time range to half/double rate
        if (iNext < pRef->m_inprate / 2)
        {
            #if TIMING_DEBUG
            NetPrintf(("send clamping to half (was %d)\n", next);
            #endif
            pRef->m_inpnext = uTick + pRef->m_inprate/2;
        }
        if (iNext > pRef->m_inprate * 2)
        {
            #if TIMING_DEBUG
            NetPrintf(("send clamping to double (was %d)\n", next);
            #endif
            pRef->m_inpnext = uTick + pRef->m_inprate*2;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistSendInput

    \Description
        Provide local input data

    \Input *pRef        - reference pointer
    \Input *pBuffer     - controller data
    \Input iLength      - data length
    \Input iLengthSize  - size of length buffer

    \Output
        int32_t         - negative=error (including GMDIST_OVERFLOW=overflow), positive=packet successfully sent or saved to NetGameDist send buffer

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
static int32_t _NetGameDistSendInput(NetGameDistRefT *pRef, void *pBuffer, int32_t iLength, int32_t iLengthSize)
{
    int32_t iNext,result,iBeg;
    unsigned char *pData;

    // add packet to queue
    if (pBuffer != NULL)
    {
        // verify length is valid
        if (iLength < 0)
        {
            ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "netgamedist: _NetGameDistSendInput with iLength %d.", iLength);
            NetPrintf(("netgamedist: invalid buffer length passed to _NetGameDistSendInput()!\n"));
            return(GMDIST_INVALID);
        }

        // see if room to buffer packet
        iNext = _Modulo(pRef->m_out.m_end+1, PACKET_WINDOW);
        // the - 1 steals a spot but prevents:
        // input local prechanging iooffset triggering this
        iBeg = _Modulo(pRef->m_inp.m_beg + pRef->m_iooffset - 1, PACKET_WINDOW);
        if (!pRef->m_sendmulti && (iNext == iBeg))
        {
            ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "netgamedist: _NetGameDistSendInput with iNext %d and iBeg %d.", iNext, iBeg);
            return(GMDIST_OVERFLOW);
        }

        if (iNext == pRef->m_out.m_beg)
        {
            ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "netgamedist: _NetGameDistSendInput with iNext %d and m_out.m_beg %d.", iNext, pRef->m_out.m_beg);
            return(GMDIST_OVERFLOW);
        }

        // point to buffer
        if (!_SetDataPtr(pRef, &pRef->m_out, iLength))
        {
            return(GMDIST_OVERFLOW);
        }
        pData = pRef->m_out.m_ctrlio+pRef->m_out.m_lkup[pRef->m_out.m_end].pos;

        // copy data into buffer
        pRef->m_out.m_lkup[pRef->m_out.m_end].len = iLength;
        pRef->m_out.m_lkup[pRef->m_out.m_end].lensize = iLengthSize;
        ds_memcpy(pData, pBuffer, iLength);

        // incorporate into buffer
        pRef->m_out.m_end = iNext;

        // moved from inside the 'while' below
        // this will update the send time on send attempt, not on actual send.
        // i.e. "queue packets at regular interval", not "queue packets so that
        // they send at regular interval", which is not really doable anyway.
        _NetGameDistUpdateSendTime(pRef);
    }

    // try and send packet
    // prevent sending if we have pending metainfo to send as it will affect the format
    while ((pRef->m_out.m_beg != pRef->m_out.m_end) && (pRef->m_updatemetainfobeg == pRef->m_updatemetainfoend))
    {
        // point to buffer
        pData = pRef->m_out.m_ctrlio+pRef->m_out.m_lkup[pRef->m_out.m_beg].pos;

        // setup a data input packet
        if (pRef->m_sendmulti)
        {
            if (pRef->m_out.m_lkup[pRef->m_out.m_beg].lensize == 2)
            {
                pRef->m_maxpkt.head.kind = GAME_PACKET_INPUT_MULTI_FAT;
            }
            else
            {
                pRef->m_maxpkt.head.kind = GAME_PACKET_INPUT_MULTI;
            }
        }
        else
        {
            pRef->m_maxpkt.head.kind = GAME_PACKET_INPUT;
        }

        pRef->m_maxpkt.head.len = pRef->m_out.m_lkup[pRef->m_out.m_beg].len;
        ds_memcpy(pRef->m_maxpkt.body.data, pData, pRef->m_maxpkt.head.len);

        // try and send
        result = pRef->sendproc(pRef->linkref, (NetGamePacketT*)&pRef->m_maxpkt, 1);
        if (result > 0)
        {
            // all is well -- remove from buffer
            pRef->m_out.m_beg = _Modulo(pRef->m_out.m_beg+1, PACKET_WINDOW);
        }
        else if (result < 0)
        {
            ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "netgamedist: GMDIST_SENDPROC_FAILED result is %d.", result);

            pRef->m_errcond = GMDIST_SENDPROC_FAILED;
            NetPrintf(("netgamedist: sendproc failed in _NetGameDistSendInput()!\n"));
            return(GMDIST_SENDPROC_FAILED);
        }
        else
        {
            // lower-level transport is out of send buffer space at the moment… exit for now and try again later.
            break;
        }
    }

    // packet was either buffered and/or sent
    return(1);
}

/*F*************************************************************************************************/
/*!
    \Function    _ProcessPacketType

    \Description
        Whenever a packet of a given type is received, this function is called
        to do any specific process.

    \Input *pRef        - reference pointer
    \Input uPacketType  - the packet type

    \Output
        none

    \Version    1.0        02/22/07 (jrainy) First Version
*/
/*************************************************************************************************F*/
static void _ProcessPacketType(NetGameDistRefT *pRef, uint8_t uPacketType)
{
    if ((uPacketType == GAME_PACKET_INPUT_MULTI) || (uPacketType == GAME_PACKET_INPUT_MULTI_FAT))
    {
        pRef->m_recvmulti = TRUE;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistSendFlowUpdate

    \Description
        Sends a packet to the server enabling or disabling flow control

    \Input *pRef        - reference pointer

    \Output
        none

    \Version    1.0        09/29/09 (jrainy) First Version
*/
/*************************************************************************************************F*/
static void _NetGameDistSendFlowUpdate(NetGameDistRefT *pRef)
{
    int32_t result;

    pRef->m_maxpkt.head.kind = GAME_PACKET_INPUT_FLOW;
    pRef->m_maxpkt.head.len = 7;

    pRef->m_maxpkt.body.data[0] = pRef->m_rdysend;
    pRef->m_maxpkt.body.data[1] = pRef->m_rdyrecv;

    pRef->m_maxpkt.body.data[2] = pRef->m_localCRCvalid;
    pRef->m_maxpkt.body.data[3] = (pRef->m_localCRC >> 24);
    pRef->m_maxpkt.body.data[4] = (pRef->m_localCRC >> 16);
    pRef->m_maxpkt.body.data[5] = (pRef->m_localCRC >> 8);
    pRef->m_maxpkt.body.data[6] = pRef->m_localCRC;

    pRef->m_localCRCvalid = FALSE;

    result = pRef->sendproc(pRef->linkref, (NetGamePacketT*)&pRef->m_maxpkt, 1);

    if (result < 0)
    {
        NetPrintf(("netgamedist: [0x%08x] Flow update failed (error=%d)!\n", pRef, result));
        pRef->m_updateflowctrl = FALSE;
    }
    else if (result > 0)
    {
        pRef->m_updateflowctrl = FALSE;
    }
    else
    {
        NetPrintf(("netgamedist: [0x%08x] Flow update deferred (error=overflow)!\n", pRef));
        // nothing to do here, the caller - NetGameDistUpdate - will try again later.
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistSendMetaInfo

    \Description
        Sends a packet to the client describing the format of upcoming packets

    \Input *pRef        - reference pointer

    \Output
        none

    \Version    1.0        09/29/09 (jrainy) First Version
*/
/*************************************************************************************************F*/
static void _NetGameDistSendMetaInfo(NetGameDistRefT *pRef)
{
    int32_t result;
    NetGameDistMetaInfoT *pMetaInfo;

    while(pRef->m_updatemetainfobeg != pRef->m_updatemetainfoend)
    {
        pMetaInfo = &pRef->m_metaInfoToSend[pRef->m_updatemetainfobeg];

        pRef->m_maxpkt.head.kind = GAME_PACKET_INPUT_META;
        pRef->m_maxpkt.head.len = 5;

        pRef->m_maxpkt.body.data[0] = pMetaInfo->uVer;
        pRef->m_maxpkt.body.data[1] = pMetaInfo->uMask >> 24;
        pRef->m_maxpkt.body.data[2] = pMetaInfo->uMask >> 16;
        pRef->m_maxpkt.body.data[3] = pMetaInfo->uMask >> 8;
        pRef->m_maxpkt.body.data[4] = pMetaInfo->uMask;

        result = pRef->sendproc(pRef->linkref, (NetGamePacketT*)&pRef->m_maxpkt, 1);

        if (result < 0)
        {
            NetPrintf(("netgamedist: [0x%08x] Meta info update failed (error=%d)!\n", pRef, result));
            pRef->m_updatemetainfobeg = _Modulo(pRef->m_updatemetainfobeg + 1, GMDIST_META_ARRAY_SIZE);
        }
        else if (result > 0)
        {
            NetPrintf(("netgamedist: [0x%08x] Meta info update sent %d 0x%08x !\n", pRef, pMetaInfo->uVer, pMetaInfo->uMask));
            pRef->m_updatemetainfobeg = _Modulo(pRef->m_updatemetainfobeg + 1, GMDIST_META_ARRAY_SIZE);
        }
        else
        {
            NetPrintf(("netgamedist: [0x%08x] Meta info update deferred (error=overflow)!\n", pRef));
            // nothing to do here, the caller - NetGameDistUpdate - will try again later.

            break;
        }
    }
}

/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistCountBits

    \Description
        Count the number of bits set in a given uint32_t variable

    \Input *pRef        - reference pointer
    \Input uMask        - mask

    \Output
        uint32_t        - the number of bits set

    \Version 09/26/09 (jrainy)
*/
/*************************************************************************************************F*/
static uint32_t _NetGameDistCountBits(NetGameDistRefT *pRef, uint32_t uMask)
{
    uint32_t uCount = 0;

    while(uMask)
    {
        uCount += (uMask & 1);
        uMask /= 2;
    };

    return(uCount);
}

#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function    _NetGameDistInputCheckLog

    \Description
        Logs the values returned by NetGameDistInputCheck.

    \Input *pRef        - reference pointer
    \Input *pSend       - the pointer to pSend the client passed to InputCheck
    \Input *pRecv       - the pointer to pRecv the client passed to InputCheck

    \Output
        none

    \Version    1.0        12/21/09 (jrainy) First Version
*/
/*************************************************************************************************F*/
static void _NetGameDistInputCheckLog(NetGameDistRefT *pRef, int32_t *pSend, int32_t *pRecv)
{
    uint32_t uCurrentTick = NetTick();
    uint32_t uDelay;

    // Calculate delay since last log
    uDelay = NetTickDiff(uCurrentTick, pRef->m_uLastInputCheckLogTick);

    // Check if condition to display the trace is met.
    // Condition is: meaningful send/recv info ready OR skip timeout expire
    if ( (pSend && (*pSend==0)) || (pRecv && (*pRecv!=0)) ||     // check meaningful send/recv
         (uDelay >= INPUTCHECK_LOGGING_DELAY) )                    // check skip timeout
    {
        if (pSend && pRecv)
        {
            NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] exiting NetGameDistInputCheck() tick=%d, *pSend=%d, *pRecv=%d logskipped_count=%d\n",
                pRef, uCurrentTick, *pSend, *pRecv, pRef->m_uSkippedInputCheckLogCount));
        }
        else if (pSend && !pRecv)
        {
            NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] exiting NetGameDistInputCheck() tick=%d, *pSend=%d, pRecv=NULL logskipped_count=%d\n",
                pRef, uCurrentTick, *pSend, pRef->m_uSkippedInputCheckLogCount));
        }
        else if (!pSend && pRecv)
        {
            NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] exiting NetGameDistInputCheck() tick=%d, pSend=NULL, *pRecv=%d logskipped_count=%d\n",
                pRef, uCurrentTick, *pRecv, pRef->m_uSkippedInputCheckLogCount));
        }
        else
        {
            NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] exiting NetGameDistInputCheck() tick=%d, pSend=NULL, pRecv=NULL logskipped_count=%d\n",
                pRef, uCurrentTick, pRef->m_uSkippedInputCheckLogCount));
        }

        // Re-initialize variables used to skip some logs
        pRef->m_uSkippedInputCheckLogCount = 0;
        pRef->m_uLastInputCheckLogTick = uCurrentTick;
    }
    else
    {
        pRef->m_uSkippedInputCheckLogCount++;
    }
}
#endif

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputPeekRecv

    \Description
        Behaves just as a receive function, taking data from netgamelink. However, we peek first and
        if an overflow is detected, we do not take the packet from netgamelink. In this case, we
        return 0, as if doing was ready to be received

    \Input *pRef     - reference pointer
    \Input *buf      - buffer to receive into
    \Input len       - available length in buf

    \Output
        int32_t      - recvproc return value, or forced to 0 if we'd be in overflow

    \Notes

    \Version    1.0        08/15/11 (jrainy)
*/
/*************************************************************************************************F*/
static int32_t NetGameDistInputPeekRecv(NetGameDistRefT *pRef, NetGamePacketT *buf, int32_t len)
{
    NetGamePacketT* pPeekPacket = NULL;

    if (pRef->peekproc)
    {
        uint32_t uDistMask = (1 << GAME_PACKET_INPUT) |
                             (1 << GAME_PACKET_INPUT_MULTI) |
                             (1 << GAME_PACKET_INPUT_MULTI_FAT);

        (*pRef->peekproc)(pRef->linkref, &pPeekPacket, uDistMask);

        // ok, we have a mean to see what is coming, and we got something.
        if (pPeekPacket)
        {
            // let's check if we have space for it.
            // for completeness, we may check buffer overflow on slots and dropproc, but this seems overkill
            if (_SetDataPtrTry(pRef, &pRef->m_inp, pPeekPacket->head.len + 1) < 0)
            {
                NetPrintf(("netgamedist: [0x%08x] not receiving from link layer a packet of type %d and length %d because our buffer is full\n", pRef, pPeekPacket->head.kind, pPeekPacket->head.len));
                // act as if the recvproc had nothing.
                return(0);
            }
        }
    }

    return((*pRef->recvproc)(pRef->linkref, buf, len, TRUE));
}
/*** Public Functions ******************************************************************/

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistCreate

    \Description
        Create the game pClient

    \Input *pLinkRef        - netgamelink module ref
    \Input *pStatProc       - netgamelink stat callback
    \Input *pSendProc       - game send func
    \Input *pRecvProc       - game recv func
    \Input *uInBufferSize   - input buffer size, plan around PACKET_WINDOW*n*packets
    \Input *uOutBufferSize  - output buffer size, plan around PACKET_WINDOW*packets

    \Output
        NetGameDistRefT * - pointer to module state

    \Version    1.0        12/20/00 (GWS) First Version
    \Version    1.1        02/08/07 (jrainy) modified from to include size params.
*/
/*************************************************************************************************F*/
NetGameDistRefT *NetGameDistCreate(void *pLinkRef, NetGameDistStatProc *pStatProc, NetGameDistSendProc *pSendProc, NetGameDistRecvProc *pRecvProc, uint32_t uInBufferSize, uint32_t uOutBufferSize )
{
    NetGameDistRefT *pRef;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state
    if ((pRef = DirtyMemAlloc(sizeof(*pRef), NETGAMEDIST_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("netgamedist: [0x%08x] unable to allocate module state\n", pRef));
        return(NULL);
    }
    memset(pRef, 0, sizeof(*pRef));
    pRef->m_memgroup = iMemGroup;
    pRef->m_memgrpusrdata = pMemGroupUserData;
    pRef->iLastSentDelta = -1;

    // Allocate the input and output buffers
    pRef->m_inp.m_ctrlio = DirtyMemAlloc( uInBufferSize, NETGAMEDIST_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata );
    pRef->m_out.m_ctrlio = DirtyMemAlloc( uOutBufferSize, NETGAMEDIST_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata );

    memset(pRef->m_inp.m_ctrlio, 0, uInBufferSize);
    memset(pRef->m_out.m_ctrlio, 0, uOutBufferSize);

    pRef->m_inp.m_buflen = uInBufferSize;
    pRef->m_out.m_buflen = uOutBufferSize;

    // save the link layer callbacks
    pRef->linkref = pLinkRef;
    pRef->statproc = pStatProc;
    pRef->sendproc = pSendProc;
    pRef->recvproc = pRecvProc;

    // set default controller exchange rate
    pRef->m_inprate = 50;
    // set the defaults
    pRef->m_inpmini = 1;
    pRef->m_inpmaxi = 10;

    // Defaults to two players so that the non-multi version behaves as it used to.
    // In this case, it doesn't matter whether the pClient NetGameLinkDist thinks it's
    // player 0 or 1.
    pRef->m_totplrs = 2;
    pRef->m_ourindex = 0;

    // Set default verbosity level
    pRef->iVerbose = 1;

    // init check tick counter
    pRef->m_uLastInputCheckLogTick = NetTick();

    NetPrintf(("netgamedist: module created with PACKET_WINDOW = %d\n", PACKET_WINDOW));

    return(pRef);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistDestroy

    \Description
        Destroy the game pClient

    \Input *pRef     - reference pointer

    \Version    1.0        12/20/00 (GWS) First Version
*/
/*************************************************************************************************F*/
void NetGameDistDestroy(NetGameDistRefT *pRef)
{
    DirtyMemFree( pRef->m_inp.m_ctrlio, NETGAMEDIST_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata );
    DirtyMemFree( pRef->m_out.m_ctrlio, NETGAMEDIST_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata );
    pRef->m_inp.m_ctrlio = NULL;
    pRef->m_out.m_ctrlio = NULL;

    // free our memory
    DirtyMemFree(pRef, NETGAMEDIST_MEMID, pRef->m_memgroup, pRef->m_memgrpusrdata);

}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistStatus

    \Description
        Get pointer to status counters

    \Input *pRef                     - reference pointer
    \Input iSelect                   - selector
    \Input iValue                    - input value
    \Input pBuf                      - output buffer
    \Input iBufSize                  - output buffer size

    \Output
        int32_t                      - selector specific return value

    \Notes
        This is read-only data which can be read at any time.  This reference becomes
        invalid when the module is destroyed.

    \Version    1.0        12/20/00 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t NetGameDistStatus(NetGameDistRefT *pRef, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'mult')
    {
        // Make user-provide buffer is large enough to receive a pointer
        if ((pBuf != NULL) && (iBufSize >= (int32_t)(sizeof(NetGameDistStatT) * pRef->m_totplrs)))
        {
            ds_memcpy(pBuf, pRef->m_recvstats, sizeof(NetGameDistStatT) * pRef->m_totplrs);
            return(pRef->m_totplrs);
        }
        else
        {
            // unhandled
            return(-1);
        }
    }
    if (iSelect == 'qver')
    {
        return(pRef->uLastQueriedVersion);
    }
    if (iSelect == 'stat')
    {
        (pRef->statproc)(pRef->linkref, iSelect, iValue, &pRef->m_stats, sizeof(NetGameLinkStatT));

        // Make user-provide buffer is large enough to receive a pointer
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
    \Function    NetGameDistSetServer

    \Description
        Get pointer to status counters

    \Input *pRef            - reference pointer
    \Input uActAsServer     - boolean, whether NetGameDist should send multi-packets

    \Output
        none

    \Notes

    \Version    1.0        02/26/07 (jrainy) First Version
*/
/*************************************************************************************************F*/
void NetGameDistSetServer(NetGameDistRefT *pRef, uint8_t uActAsServer)
{
    // the trigraph is there to make sure we don't store non-{0,1} value in.
    pRef->m_sendmulti = uActAsServer ? TRUE : FALSE;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistMultiSetup

    \Description
        Tells NetGameDist how many players there are in the game, and which one we are.
        Will only affect the order packets are received from NetGameDistInputQueryMulti.

    \Input *pRef            - netgamelink module ref
    \Input iOurIndex        - Which player we are
    \Input iTotPlrs         - Total number of players

    \Output

    \Version    1.0        02/08/07 (jrainy) (GWS) First Version
*/
/*************************************************************************************************F*/
void NetGameDistMultiSetup(NetGameDistRefT *pRef, int32_t iOurIndex, int32_t iTotPlrs )
{
    NetPrintf(("netgamedist: [0x%08x] NetGameDistMultiSetup index: %d, total players: %d\n", pRef, iOurIndex, iTotPlrs));

    pRef->m_totplrs = iTotPlrs;
    pRef->m_ourindex = iOurIndex;
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistMetaSetup

    \Description
        Tells NetGameDist whether to enable meta-information sending, and
        sets the mask & the version number for the meta-information.

    \Input *pRef            - netgamelink module ref
    \Input uSparse          - enable or disable meta-information sending (disabling after enabling is currently not-supported on the client)
    \Input uMask            - set the mask for the meta-information
    \Input uVersion         - set the version number for the meta-information.

    \Output

    \Version    1.0        08/29/11 (szhu)
*/
/*************************************************************************************************F*/
void NetGameDistMetaSetup(NetGameDistRefT *pRef, uint8_t uSparse, uint32_t uMask, uint32_t uVersion)
{
    NetPrintf(("netgamedist: [%p] NetGameDistMetaSetup Enabled: %s, Mask: 0x%x, Version: %d\n",
               pRef, uSparse?"TRUE":"FALSE", uMask, uVersion));

    pRef->m_uSparse = uSparse;
    pRef->m_metaInfoToSend[pRef->m_updatemetainfoend].uMask = uMask;
    pRef->m_metaInfoToSend[pRef->m_updatemetainfoend].uPlayerCount = _NetGameDistCountBits(pRef, uMask);
    pRef->m_metaInfoToSend[pRef->m_updatemetainfoend].uVer = _Modulo(uVersion, GMDIST_META_ARRAY_SIZE);
    pRef->m_updatemetainfoend = _Modulo(pRef->m_updatemetainfoend + 1, GMDIST_META_ARRAY_SIZE);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputPeek

    \Description
        See if a completed packet is ready

    \Input *pRef    - reference pointer
    \Input *pType   - will be filled with data type
    \Input *pPeer   - buffer sent by peer
    \Input *pPlen   - length of data in peer buffer. Must pass in the length of pPeer

    \Output
        int32_t     - zero=no data pending, negative=error, positive=data returned

    \Version    1.0        02/05/2007 (JLB) First Version
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputPeek(NetGameDistRefT *pRef, uint8_t *pType, void *pPeer, int32_t *pPlen)
{
    uint8_t *pPsrc;
    int32_t length;
    uint8_t inputKind;
    int16_t iLengthSize = 1;
    uint8_t offsetLengths;
    uint8_t offsetBuffer;
    uint8_t offsetTypes;
    uint8_t offsetVersion;
    uint16_t uPlayerCount = pRef->m_totplrs;
    uint8_t uVer = 0;


    // if no remote data, do an update
    if (pRef->m_inp.m_beg == pRef->m_inp.m_end)
    {
        NetGameDistUpdate(pRef);
    }

    // no packets from peer?
    if (pRef->m_inp.m_beg == pRef->m_inp.m_end)
    {
        return(0);
    }

    // point to data buffer
    pPsrc = pRef->m_inp.m_ctrlio+pRef->m_inp.m_lkup[pRef->m_inp.m_beg].pos;

    inputKind = pPsrc[pRef->m_inp.m_lkup[pRef->m_inp.m_beg].len - 1];

    if (inputKind == GAME_PACKET_INPUT_MULTI_FAT)
    {
        iLengthSize = 2;
    }

    // find the location to lookup the version
    offsetVersion = pRef->m_recvmulti ? 1 : 0;

    // extract the version number of the packet, and player count, if the server ever gave us metadata
    if (pRef->m_gotMetaInfo && pRef->m_recvmulti)
    {
        uVer = _Modulo(pPsrc[offsetVersion], GMDIST_META_ARRAY_SIZE);
        uPlayerCount = pRef->m_metaInfo[uVer].uPlayerCount;
        pRef->uLastQueriedVersion = uVer;
    }

    // always pack packets as if there was at least 2 players (to avoid negatively-sized fields)
    if (pRef->m_recvmulti && (pRef->m_totplrs == 1))
    {
        uPlayerCount = 2;
    }

    // compute offsets to various parts
    offsetTypes = offsetVersion + (pRef->m_gotMetaInfo ? 1 : 0);
    offsetLengths = offsetTypes + (pRef->m_recvmulti ? (uPlayerCount / 2) : 1);
    offsetBuffer = offsetLengths + (pRef->m_recvmulti ? (iLengthSize * (uPlayerCount - 2)) : 0);

    // copy over the data, -1 to skip the packet kind tacked at the end
    length = pRef->m_inp.m_lkup[pRef->m_inp.m_beg].len - offsetBuffer - 1;
    if (pPeer != NULL)
    {
        //check the passed-in length to prevent overflow
        if (length > *pPlen)
        {
            ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "NetGameDistInputPeek error. length is %d, *pLen is %d.", length, *pPlen);

            // fail if the buffer is insufficient
            *pPlen = length;
            return GMDIST_OVERFLOW;
        }

        ds_memcpy(pPeer, pPsrc + offsetBuffer, length);
    }

    *pPlen = length;

    if (pType != NULL)
    {
        *pType = *(pPsrc + (pRef->m_recvmulti ? 1 : 0));
    }

    #if DIRTYCODE_LOGGING
    NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] peeked type %d len %d\n", pRef, pType?*pType:0, pPlen?*pPlen:0));
    #endif

    // return success
    return(pRef->m_packetid[pRef->m_inp.m_beg]);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputQuery

    \Description
        See if a completed packet is ready

    \Input *pRef    - reference pointer
    \Input *pOurs   - buffer previously sent with NetGameDistInputLocal
    \Input *pOlen   - length of data in ours buffer
    \Input *pPeer   - buffer sent by peer
    \Input *pPlen    - length of data in peer buffer

    \Output
        int32_t     - zero=no data pending, negative=error, positive=data returned

    \Version    1.0        12/20/00 (GWS) First Version
    \Version    1.1        02/08/07 (jrainy) cahnged to use the new multi version
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputQuery(NetGameDistRefT *pRef, void *pOurs, int32_t *pOlen, void *pPeer, int32_t *pPlen)
{
    void* inputs[2];
    int32_t lengths[2];
    uint8_t types[2];
    int32_t ret;

    if ( pRef->m_totplrs != 2 )
    {
        return(GMDIST_BADSETUP);
    }

    inputs[pRef->m_ourindex] = pOurs;
    inputs[1-pRef->m_ourindex] = pPeer;

    ret = NetGameDistInputQueryMulti( pRef, types, inputs, lengths);

    *pOlen = lengths[pRef->m_ourindex];
    *pPlen = lengths[1-pRef->m_ourindex];

    return(ret);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputQueryMulti

    \Description
        See if a completed packet is ready

    \Input *pRef        - reference pointer
    \Input *pDataTypes  - Array of data types (GMDIST_DATA_NONE, GMDIST_DATA_INPUT, GMDIST_DATA_INPUT_DROPPABLE, GMDIST_DATA_DISCONNECT)
    \Input **pInputs    - Array of pointers to inputs to receive
    \Input *pLen        - Array of lengths from the data received

    \Output
        int32_t         - zero=no data pending, negative=error, positive=data returned

    \Version    1.0        12/20/00 (GWS) First Version
    \Version    1.1        02/08/07 (jrainy) From the old non-multi version.
    \Version    1.2        12/03/09 (mclouatre) Added configurable run-time verbosity
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputQueryMulti(NetGameDistRefT *pRef, uint8_t *pDataTypes, void **pInputs, int32_t *pLen)
{
    unsigned char *pOsrc;
    uint32_t i,count,pos,recvLen;
    unsigned char *recvBuff;
    uint8_t *lengths;
    uint8_t *typePos;
    uint8_t delta;
    uint8_t offsetLengths;
    uint8_t offsetBuffer;
    uint8_t offsetTypes;
    uint8_t offsetVersion;
    uint32_t ourinp;
    uint16_t uPlayerCount = pRef->m_totplrs;
    uint8_t crcRequest = FALSE;
    uint8_t uVer = 0;

    // if just waiting on remote data, do an update
    if ((pRef->m_inp.m_beg == pRef->m_inp.m_end) && (_Modulo(pRef->m_inp.m_beg + pRef->m_iooffset, PACKET_WINDOW) != pRef->m_out.m_end))
    {
        NetGameDistUpdate(pRef);
    }

    // check for completed packets
    if ((pRef->m_inp.m_beg == pRef->m_inp.m_end) || (!pRef->m_sendmulti && !pRef->m_recvmulti && (_Modulo(pRef->m_inp.m_beg + pRef->m_iooffset, PACKET_WINDOW) == pRef->m_out.m_end)))
    {
        #if DIRTYCODE_LOGGING
        NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] exiting NetGameDistInputQueryMulti() retval=0 tick=%d len=n/a type=n/a\n", pRef, NetTick()));
        #endif

        return(0);
    }

    if (pInputs)
    {
        //access the first byte after the received data
        uint8_t* pPacketKind = pRef->m_inp.m_ctrlio + pRef->m_inp.m_lkup[pRef->m_inp.m_beg].pos + pRef->m_inp.m_lkup[pRef->m_inp.m_beg].len - 1;
        int16_t iLengthSize = 1;

        if (*pPacketKind == GAME_PACKET_INPUT_MULTI_FAT)
        {
            iLengthSize = 2;
        }

        // Go through the received packet and fill all the input data except ours, then copy our own from outlkup

        count = 0;
        pos = 0;
        recvLen = pRef->m_inp.m_lkup[pRef->m_inp.m_beg].len - 1;
        recvBuff = pRef->m_inp.m_ctrlio+pRef->m_inp.m_lkup[pRef->m_inp.m_beg].pos;
        delta = pRef->m_recvmulti ? recvBuff[0] : 1;

        offsetVersion = pRef->m_recvmulti ? 1 : 0;

        // extract the version number of the packet, and player count, if the server ever gave us metadata
        if (pRef->m_gotMetaInfo && pRef->m_recvmulti)
        {
            uVer = _Modulo(((char *)recvBuff)[offsetVersion], GMDIST_META_ARRAY_SIZE);
            uPlayerCount = pRef->m_metaInfo[uVer].uPlayerCount;
            pRef->uLastQueriedVersion = uVer;
        }

        // always pack packets as if there was at least 2 players (to avoid negatively-sized fields)
        if (pRef->m_recvmulti && (uPlayerCount == 1))
        {
            uPlayerCount = 2;
        }

        // compute offsets to various parts
        offsetTypes = offsetVersion + (pRef->m_gotMetaInfo ? 1 : 0);
        offsetLengths = offsetTypes + (pRef->m_recvmulti ? (uPlayerCount / 2) : 1);
        offsetBuffer = offsetLengths + (pRef->m_recvmulti ? (iLengthSize * (uPlayerCount - 2)) : 0);

        lengths = recvBuff + offsetLengths;
        typePos = recvBuff + offsetTypes;
        recvBuff += offsetBuffer;

        for (i = 0; i<pRef->m_totplrs; i++)
        {
            // if this is not our own input and we are either
            //      sending for everyone
            //      or this is a player we are including in the current sparse multi-packet
            // then, add it in
            if (i != pRef->m_ourindex)
            {
                if (!pRef->m_gotMetaInfo || (pRef->m_metaInfo[uVer].uMask & (1 << i)))
                {
                    // compute the length of packet for player i.
                    // last length is known (ours) second to last is implicit (total - known ones)
                    if (count == (uPlayerCount - 2u))
                    {
                        pLen[i] = recvLen - pos - offsetBuffer;
                    }
                    else
                    {
                        if (iLengthSize == 2)
                        {
                            pLen[i] = (lengths[count * iLengthSize] * 256) + lengths[count * iLengthSize + 1];
                        }
                        else
                        {
                            pLen[i] = lengths[count];
                        }
                    }

                    //prevent invalid sizes if bogus data is received
                    if (pLen[i] < 0)
                    {
                        pLen[i] = 0;
                    }

                    // clamp the total read length to the received buffer, to prevent overflow.
                    if (pLen[i] + pos + offsetBuffer > recvLen)
                    {
                        NetPrintfVerbose((pRef->iVerbose, 0, "netgamedist: Warning ! Buffer overrun. Packet trimmed.\n"));
                        pLen[i] = recvLen - pos - offsetBuffer;
                    }

                    //compute the type of packet for player i. (nibbles)
                    if (count % 2)
                    {
                        pDataTypes[i] = *typePos >> 4;
                        typePos++;
                    }
                    else
                    {
                        pDataTypes[i] = *typePos & 0x0f;
                    }

                    //copy the packet for player i.
                    if (((pDataTypes[i] & ~GMDIST_DATA_CRC_REQUEST) == GMDIST_DATA_INPUT) ||
                        ((pDataTypes[i] & ~GMDIST_DATA_CRC_REQUEST) == GMDIST_DATA_INPUT_DROPPABLE))
                    {
                        ds_memcpy(pInputs[i], recvBuff + pos, pLen[i]);
                        pos += pLen[i];
                    }
                    else
                    {
                        pLen[i] = 0;
                    }

                    //if crc-checking is disabled locally, clear the "crc request" bit
                    if (!pRef->m_ucrcchallenges)
                    {
                        pDataTypes[i] &= ~GMDIST_DATA_CRC_REQUEST;
                    }

                    //set a local variable if any of the input has a crc request in.
                    //this is to set the "crc request" bit in local input too, to ease our customers job.
                    if (pDataTypes[i] & GMDIST_DATA_CRC_REQUEST)
                    {
                        crcRequest = TRUE;
                    }

                    count++;
                }
                else
                {
                    pDataTypes[i] = GMDIST_DATA_NODATA;
                    pLen[i] = 0;
                }
            }
        }

        if (delta)
        {
            // point to data buffers. Our local storage has just 1-byte type field
            pRef->m_iooffset += (delta - 1);
            ourinp = _Modulo(pRef->m_inp.m_beg + pRef->m_iooffset, PACKET_WINDOW);
            pOsrc = pRef->m_out.m_ctrlio + pRef->m_out.m_lkup[ourinp].pos + 1;

            // copy over the data
            pLen[pRef->m_ourindex] = pRef->m_out.m_lkup[ourinp].len - 1;
            ds_memcpy(pInputs[pRef->m_ourindex], pOsrc, pLen[pRef->m_ourindex]);

            pDataTypes[pRef->m_ourindex] = *(pRef->m_out.m_ctrlio + pRef->m_out.m_lkup[ourinp].pos);
        }
        else
        {
            pRef->m_iooffset = _Modulo(pRef->m_iooffset - 1, PACKET_WINDOW);
            pDataTypes[pRef->m_ourindex] = GMDIST_DATA_NONE;
            pLen[pRef->m_ourindex] = 0;
        }

        if (crcRequest)
        {
            pDataTypes[pRef->m_ourindex] |= GMDIST_DATA_CRC_REQUEST;
        }
    }

    // return item from buffer
    pRef->m_inp.m_beg = _Modulo(pRef->m_inp.m_beg + 1, PACKET_WINDOW);
    // update global sequence number
    pRef->m_globalseq += 1;

    #if DIRTYCODE_LOGGING
    NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] exiting NetGameDistInputQueryMulti() retval=%d tick=%d\n", pRef, pRef->m_globalseq, NetTick()));
    NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "  ==== Dumping player-specific data returned in [OUT] params\n"));
    if (pInputs)
    {
        for (i = 0; i<pRef->m_totplrs; i++)
        {
            uint8_t dataType = (pDataTypes[i] & ~GMDIST_DATA_CRC_REQUEST);
            NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "    == Player %d --> len=%d type=%s\n", i, pLen[i],
                _strNetGameDistDataTypes[((dataType >= GMDIST_DATA_NONE && dataType <= GMDIST_DATA_NODATA) ? dataType : 0)]));
        }
    }
    NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "  ==== End of player-specific data dump\n"));
    #endif

    // return the sequence number
    return(pRef->m_globalseq);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputLocal

    \Description
        Provide local input data

    \Input *pRef    - reference pointer
    \Input *pBuffer - controller data
    \Input iLength  - data length

    \Output
        int32_t     - negative=error (including GMDIST_OVERFLOW=overflow), positive=successfully sent or queued

    \Version    1.0        12/20/00 (GWS) First Version
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputLocal(NetGameDistRefT *pRef, void *pBuffer, int32_t iLength)
{
    if (!pBuffer)
    {
        return(_NetGameDistSendInput(pRef, NULL, iLength, 0));
    }
    pRef->m_multibuf[0] = GMDIST_DATA_INPUT;
    ds_memcpy(pRef->m_multibuf + 1, pBuffer, iLength);
    return(_NetGameDistSendInput(pRef, pRef->m_multibuf, iLength + 1, 1));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputLocalMulti

    \Description
        Provide local input data for all players, including ours, which will be discarded

    \Input *pRef        - reference pointer
    \Input *uTypes      - array of data types (GMDIST_DATA_INPUT_DROPPABLE, GMDIST_DATA_INPUT)
    \Input *pBuffer     - array of controller data
    \Input *iLengths    - array of data length
    \Input *iDelta      - game team should pass in 1, other values only usable on server

    \Output
        int32_t         - negative=error (including GMDIST_OVERFLOW*=overflow), positive=successfully sent or queued

    \Version    1.0        02/08/07 (jrainy) First Version
    \Version    1.1        12/03/09 (mclouatre) Added configurable run-time verbosity
*/
/*************************************************************************************************F*/
int32_t NetGameDistInputLocalMulti(NetGameDistRefT *pRef, uint8_t* uTypes, void **pBuffer, int32_t* iLengths, int32_t iDelta)
{
    uint32_t index,pos,count;
    uint8_t offsetLengths;
    uint8_t offsetBuffer;
    uint8_t offsetTypes;
    uint8_t offsetVersion;
    uint8_t *lengths;
    uint8_t *typePos;
    uint8_t uOurType, uType;
    uint32_t sendsize;
    int16_t iLengthSize = 1;
    uint16_t uPlayerCount = pRef->m_sendmulti ? pRef->m_totplrs : 1;
    uint16_t uLastPlayer = uPlayerCount;

    // check if any input forces us to send fat multi-packets.
    for (index = 0; index < uPlayerCount; index++)
    {
        if (iLengths[index] > 0xFF)
        {
            iLengthSize = 2;
        }
    }

    // if we are sending sparse multipacket, adjust the player count accordingly
    if (pRef->m_uSparse && pRef->m_sendmulti)
    {
        uPlayerCount = pRef->m_metaInfoToSend[_Modulo(pRef->m_updatemetainfoend - 1, GMDIST_META_ARRAY_SIZE)].uPlayerCount;
    }

    // always pack multipacket as if there was at least 2 players (simplify the logic)
    if (pRef->m_sendmulti && (uPlayerCount == 1))
    {
        uLastPlayer = 2;
        uPlayerCount = 2;
    }

    // compute the offset to various parts of the multipacket
    offsetVersion = pRef->m_sendmulti ? 1 : 0;
    offsetTypes = offsetVersion + ((pRef->m_uSparse && pRef->m_sendmulti) ? 1 : 0);
    offsetLengths = offsetTypes + (pRef->m_sendmulti ? (uPlayerCount / 2) : 1);
    offsetBuffer = offsetLengths + (pRef->m_sendmulti ? (iLengthSize * (uPlayerCount - 2)) : 0);

    if (pRef->m_sendmulti)
    {
        // compute the total packet size
        sendsize = offsetBuffer;
        for (index = 0; index < pRef->m_totplrs; index++)
        {
            if (index != pRef->m_ourindex)
            {
                sendsize += iLengths[index];
            }
        }

        // bail out if the packet would overflow
        if (sendsize >= NETGAME_DATAPKT_MAXSIZE)
        {
            ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "Multipacket bigger than NETGAME_DATAPKT_MAXSIZE (%d). Discarding and reporting overflow.\n", NETGAME_DATAPKT_MAXSIZE);
            NetPrintf(("netgamedist: [0x%08x] Multipacket bigger than NETGAME_DATAPKT_MAXSIZE (%d). Discarding and reporting overflow.\n", pRef, NETGAME_DATAPKT_MAXSIZE));
            return(GMDIST_OVERFLOW_MULTI);
        }

        // mark the version, if we are sending meta information
        if (pRef->m_uSparse)
        {
            pRef->m_multibuf[offsetVersion] = _Modulo(pRef->m_metaInfoToSend[_Modulo(pRef->m_updatemetainfoend - 1, GMDIST_META_ARRAY_SIZE)].uVer, GMDIST_META_ARRAY_SIZE);
        }

        uOurType = (uTypes[pRef->m_ourindex] & ~GMDIST_DATA_CRC_REQUEST);
        if (uOurType == GMDIST_DATA_NONE)
        {
            pRef->m_multibuf[0] = 0;
        }
        else
        {
            if (((iDelta - 1) - pRef->iLastSentDelta) >= PACKET_WINDOW)
            {
                ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "GMDIST_OVERFLOW_WINDOW (pRef->m_packetid[pRef->m_inp.m_beg]-1) is %d, pRef->iLastSentDelta is %d.\n", (pRef->m_packetid[pRef->m_inp.m_beg]-1), pRef->iLastSentDelta);
                return(GMDIST_OVERFLOW_WINDOW);
            }

            pRef->m_multibuf[0] = (iDelta-1) - pRef->iLastSentDelta;

            pRef->iLastSentDelta = (iDelta-1);
        }
        pRef->m_packetid[pRef->m_inp.m_beg] = 0;

        if (uOurType == GMDIST_DATA_NONE)
        {
            pRef->m_iooffset = _Modulo(pRef->m_iooffset + 1, PACKET_WINDOW);
        }
    }

    lengths = (unsigned char *)pRef->m_multibuf + offsetLengths;
    typePos = (unsigned char *)pRef->m_multibuf + offsetTypes;

    pos = offsetBuffer;
    count = 0;

    for (index = 0; index < uLastPlayer; index++)
    {
        uType = (uTypes[index] & ~GMDIST_DATA_CRC_REQUEST);

        if (((uType == GMDIST_DATA_NONE) || (uType == GMDIST_DATA_DISCONNECT)) && !pRef->m_sendmulti)
        {
            #if DIRTYCODE_LOGGING
            NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY,"netgamedist: [0x%08x] Exiting NetGameDistInputLocalMulti() because of invalid data type.\n", pRef));
            #endif
            return(GMDIST_INVALID);
        }

        //if 1) this is to someone else than us or if we are sending single packet
        // and 2) we're either: not sending sparse packet or this is a player that is present in the current mask
        if (((index != pRef->m_ourindex) || (!pRef->m_sendmulti)) &&
            ((!pRef->m_uSparse) || (pRef->m_metaInfoToSend[_Modulo(pRef->m_updatemetainfoend - 1, GMDIST_META_ARRAY_SIZE)].uMask & (1 << index))))
        {
            //put the data in
            ds_memcpy(pRef->m_multibuf + pos, pBuffer[index], iLengths[index]);
            pos += iLengths[index];

            if (pRef->m_sendmulti)
            {
                if (count < (uPlayerCount - 2u))
                {
                    //we assign the length here for normal packets
                    if (iLengthSize == 1)
                    {
                        lengths[count] = iLengths[index];
                    }
                    else
                    {
                        //and for fat multipackets
                        lengths[count * 2] = iLengths[index] / 256;
                        lengths[count * 2 + 1] = iLengths[index] % 256;
                    }
                }
            }

            if (count % 2)
            {
                *typePos = (*typePos & 0x0f) | ((uTypes[index] << 4) & 0xf0);
                typePos++;
            }
            else
            {
                *typePos = (uTypes[index] & 0x0f);
            }
            count++;
        }
    }

    if (!NetGameDistControl(pRef, '?cws', pos, NULL))
    {
        #if DIRTYCODE_LOGGING
        NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] Exiting NetGameDistInputLocalMulti() with return value GMDIST_OVERFLOW.\n", pRef));
        #endif
        return(GMDIST_OVERFLOW);
    }

    #if DIRTYCODE_LOGGING
    {
        uint8_t dataType = (uTypes[0] & ~GMDIST_DATA_CRC_REQUEST);
        NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] NetGameDistInputLocalMulti() called with tick=%d len=%d type=%s\n",
            pRef, NetTick(), iLengths[0], _strNetGameDistDataTypes[((dataType >= GMDIST_DATA_NONE && dataType <= GMDIST_DATA_NODATA) ? dataType : 0)]));

    }
    #endif

    // pass the size of length used to the send input function
    return(_NetGameDistSendInput(pRef, pRef->m_multibuf, pos, iLengthSize));
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputCheck

    \Description
        Check input status (see how long till next)

    \Input *pRef    - reference pointer
    \Input *pSend   - (optional) stores time until next packet should be sent
    \Input *pRecv   - (optional) stores whether data is available

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameDistInputCheck(NetGameDistRefT *pRef, int32_t *pSend, int32_t *pRecv)
{
    int32_t iRemain;
    uint32_t uTick;
    int32_t iInData;
    int32_t iOutData;

    // call the status proc
    (pRef->statproc)(pRef->linkref, 'stat', 0, &pRef->m_stats, sizeof(NetGameLinkStatT));

    // get current time
    uTick = NetTick();

    // make sure next send time is initialized
    if (pRef->m_inpnext == 0)
    {
        pRef->m_inpnext = uTick;
    }

    // see if its time to recalc network parms
    if (uTick > pRef->m_inpcalc)
    {
        // set the number of unacknowledged packets permitted
        // (this number must be large enough to cover the network latency
        pRef->m_inpwind = (pRef->m_stats.late+pRef->m_inprate)/pRef->m_inprate;

        if (pRef->m_recvmulti)
        {
            pRef->m_inpwind *= 2;
        }

        if (pRef->m_inpwind < pRef->m_inpmini)
        {
            pRef->m_inpwind = pRef->m_inpmini;
        }
        if (pRef->m_inpwind > pRef->m_inpmaxi)
        {
            pRef->m_inpwind = pRef->m_inpmaxi;
        }
        if (pRef->m_inpwind > 500/pRef->m_inprate)
        {
            pRef->m_inpwind = 500/pRef->m_inprate;
        }
        // determine time till next update
        pRef->m_inpcalc = uTick + pRef->m_inprate;
    }

    // figure out time remaining until next send
    iRemain = pRef->m_inpnext - uTick;

    // clamp to valid rate
    if (iRemain < 0)
    {
        iRemain = 0;
    }
    if (iRemain > pRef->m_inprate*2)
    {
        iRemain = pRef->m_inprate*2;
    }

    _NetGameDistCheckWindow(pRef, iRemain, uTick);

    // figure out time till next send
    iRemain = pRef->m_inpnext - uTick;
    // clamp to valid rate
    if (iRemain < 0)
    {
        iRemain = 0;
    }

    // return time until next packet should be sent
    if (pSend != NULL)
    {
        // make sure rate is set
        if (pRef->m_inprate == 0)
        {
            // data is not initialized -- just return a delay
            *pSend = 50;
        }
        else if ((pRef->m_out.m_beg != pRef->m_out.m_end) || (!pRef->m_rdyrecvremote))
        {
            // dont send while something in output queue
            *pSend = 10;
        }
        else
        {
            *pSend = iRemain;
        }
    }

    // indicate if a packet is waiting
    if (pRecv != NULL)
    {
        // if just waiting on remote data, do an update
        if ((pRef->m_inp.m_beg == pRef->m_inp.m_end) && (_Modulo(pRef->m_inp.m_beg + pRef->m_iooffset, PACKET_WINDOW) != pRef->m_out.m_end))
        {
            NetGameDistUpdate(pRef);
        }
        // indicate if data is available
        iInData = _Modulo(pRef->m_inp.m_end - pRef->m_inp.m_beg, PACKET_WINDOW);
        iOutData = _Modulo(pRef->m_out.m_end - _Modulo(pRef->m_inp.m_beg + pRef->m_iooffset, PACKET_WINDOW), PACKET_WINDOW);

        *pRecv = (iInData < iOutData ? iInData : iOutData);
        if (*pRecv < 0)
        {
            NetPrintf(("netgamedist: [0x%08x] error, *pRecv < 0\n", pRef));
        }
        else if ((*pRecv > 0) && !((pRef->m_inp.m_beg != pRef->m_inp.m_end) && (_Modulo(pRef->m_inp.m_beg + pRef->m_iooffset, PACKET_WINDOW) != pRef->m_out.m_end)))
        {
            NetPrintf(("netgamedist: [0x%08x] error, *pRecv > 0\n", pRef));
        }
    }

    #if DIRTYCODE_LOGGING
    _NetGameDistInputCheckLog(pRef, pSend, pRecv);
    #endif
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputRate

    \Description
        Set the input rate

    \Input *pRef    - reference pointer
    \Input iRate    - new input rate

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameDistInputRate(NetGameDistRefT *pRef, int32_t iRate)
{
    // save rate if changed
    if (iRate > 0)
    {
        pRef->m_inprate = iRate;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistInputClear

    \Description
        Flush the input queue.

    \Input *pRef     - reference pointer

    \Notes
        This must be done with independent synchronization before and after to avoid issues.

    \Version 12/20/00 (GWS)
*/
/*************************************************************************************************F*/
void NetGameDistInputClear(NetGameDistRefT *pRef)
{
    // reset buffer pointers
    pRef->m_out.m_end = 0;
    pRef->m_out.m_beg = 0;
    pRef->m_inp.m_end = 0;
    pRef->m_inp.m_beg = 0;

    // reset sequence numbers
    pRef->m_localseq = 0;
    pRef->m_globalseq = 0;

    // reset the iooffset between the two queues.
    pRef->m_iooffset = 0;

    // reset the send time
    pRef->m_inpnext = NetTick();
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistControl

    \Description
        Set NetGameDist operation parameters

    \Input *pRef     - reference pointer
    \Input iSelect  - item to tweak
    \Input iValue    - tweak value
    \Input pValue    - pointer tweak value

    \Output
        int32_t         - selector specific

    \Notes
        select can be one of the following:
        \n
        \n ---- basic setup and control ----
        \n    'clri' - clear the local queue of inputs
        \n    'mini' - min window size clamp
        \n    'maxi' - max window size clamp
        \n    'spam' - sets debug output verbosity (0...n)
        \n
        \n --- advanced setup
        \n    'crcs' - enable/disable the reception of CRC challenges
        \n    'lcrc' - to provide the current CRC as request by NetGameDist
        \n    'lrcv' - local recv flow control. Set whether we are ready to receive or not.
        \n    'lsnd' - local send flow control. Set whether we are ready to send or not.
        \n
        \n ---- provide information ----
        \n    'late' - overall latency of game
        \n    'plat' - get the end-to-end latency in number of packets
        \n    'rate' - current exchange rate
        \n    'rcrc' - get the remote CRC. if iValue==0, returns whether we have a value. Otherwise return the value itself.
        \n    'rrcv' - remote recv flow control. Tells whether remote is ready to receive or not.
        \n    'rsnd' - remote send flow control. Tells whether remote is ready to send or not.
        \n    'wind' - input exchange window
        \n    '?cmp' - offset of first packet to process in input buffer
        \n    '?cws' - can we send. FALSE if sending would overflow the send queue, iValue is length
        \n    '?inp' - offset of last packet to process in input buffer
        \n    '?out' - offset of last packet to send in output buffer
        \n    '?snd' - offset of first packet to send in output buffer
        \n
        \n  The count of [sndoff..outoff] is the number of packets ready to send.  The min of
            [cmpoff..inpoff] and [cmpoff..outoff] is the number of packets ready to process (because
            you need a packet from each peer in order to perform processing).

    \Version    1.1        11/18/08 (jrainy) refactored from NetGameDistInputTweak
*/
/*************************************************************************************************F*/
int32_t NetGameDistControl(NetGameDistRefT *pRef, int32_t iSelect, int32_t iValue, void *pValue)
{
    if (iSelect == 'clri')
    {
        NetGameDistInputClear(pRef);
        return(0);
    }
    if (iSelect == 'mini')
    {
        if (iValue >= 0)
        {
            pRef->m_inpmini = iValue;
        }
        return(pRef->m_inpmini);
    }
    if (iSelect == 'maxi')
    {
        if (iValue >= 0)
        {
            pRef->m_inpmaxi = iValue;
        }
        return(pRef->m_inpmaxi);
    }
    if (iSelect == 'spam')
    {
        pRef->iVerbose = iValue;
        return(0);
    }
    if (iSelect == 'crcs')
    {
        pRef->m_ucrcchallenges = iValue;
        return(0);
    }
    if (iSelect == 'lcrc')
    {
        pRef->m_updateflowctrl = TRUE;
        pRef->m_localCRC = iValue;
        pRef->m_localCRCvalid = TRUE;
        return(0);
    }
    if (iSelect == 'lrcv')
    {
        if (iValue != pRef->m_rdyrecv)
        {
            pRef->m_updateflowctrl = TRUE;
            pRef->m_rdyrecv = iValue;
        }
        return(pRef->m_rdyrecv);
    }
    if (iSelect == 'lsnd')
    {
        if (iValue != pRef->m_rdysend)
        {
            pRef->m_updateflowctrl = TRUE;
            pRef->m_rdysend = iValue;
        }
        return(pRef->m_rdysend);
    }
    if (iSelect == 'late')
    {
        return(pRef->m_stats.late);
    }
    if (iSelect == 'plat')
    {
        return(_Modulo(pRef->m_out.m_end - _Modulo(pRef->m_inp.m_beg + pRef->m_iooffset, PACKET_WINDOW), PACKET_WINDOW));
    }
    if (iSelect == 'rate')
    {
        return(pRef->m_inprate);
    }
    if (iSelect == 'rcrc')
    {
        if (iValue)
        {
            int32_t ret = pRef->m_remoteCRC;
            pRef->m_remoteCRC = 0;
            pRef->m_remoteCRCvalid = FALSE;
            return(ret);
        }
        else
        {
            return(pRef->m_remoteCRCvalid);
        }
    }
    if (iSelect == 'rrcv')
    {
        return(pRef->m_rdyrecvremote);
    }
    if (iSelect == 'rsnd')
    {
        return(pRef->m_rdysendremote);
    }
    if (iSelect == 'wind')
    {
        return(pRef->m_inpwind);
    }
    if (iSelect == '?cmp')
    {
        return(pRef->m_inp.m_beg);
    }
    if (iSelect == '?cws')
    {
        int32_t iNext;
        if (iValue < 0)
        {
            return(FALSE);
        }

        iNext = _Modulo(pRef->m_out.m_end+1, PACKET_WINDOW);

        if (!pRef->m_sendmulti && (iNext == _Modulo(pRef->m_inp.m_beg + pRef->m_iooffset - 1, PACKET_WINDOW)))
        {
            ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "'?cws' failure.");
            return(FALSE);
        }
        if (_SetDataPtrTry(pRef, &pRef->m_out, iValue) == -1)
        {
            return(FALSE);
        }

        return(TRUE);
    }
    if (iSelect == '?inp')
    {
        return(pRef->m_inp.m_end);
    }
    if (iSelect == '?out')
    {
        return(pRef->m_out.m_end);
    }
    if (iSelect == '?snd')
    {
        return(pRef->m_out.m_beg);
    }
    // no action
    return(0);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistUpdate

    \Description
        Perform periodic tasks.

    \Input *pRef         - reference pointer

    \Output
        uint32_t    - current sequence number

    \Notes
        Application must call this every 100ms or so.

    \Version    1.0        12/20/00 (GWS) First Version
*/
/*************************************************************************************************F*/
uint32_t NetGameDistUpdate(NetGameDistRefT *pRef)
{
    NetGamePacketT* pPacket = (NetGamePacketT*)&pRef->m_maxpkt;
    unsigned char *pData;
    int32_t next,current;
    uint32_t index,pos;
    unsigned char *pExisting;
    unsigned char *pIncoming;

    // update the input rate if needed
    NetGameDistInputRate(pRef, 0);

    // send any pending game packets
    NetGameDistInputLocal(pRef, NULL, 0);

    // grab incoming packets
    while (( _Modulo(pRef->m_inp.m_end + 1, PACKET_WINDOW) != pRef->m_inp.m_beg ) && (NetGameDistInputPeekRecv(pRef, pPacket, 1) > 0))
    {
        // dispatch the packet according to type
        if ((pPacket->head.kind == GAME_PACKET_INPUT) ||
            (pPacket->head.kind == GAME_PACKET_INPUT_MULTI) ||
            (pPacket->head.kind == GAME_PACKET_INPUT_MULTI_FAT))
        {
            pRef->iCount++;

            next = _Modulo(pRef->m_inp.m_end + 1, PACKET_WINDOW);
            if (pRef->m_inp.m_beg != pRef->m_inp.m_end && !pRef->m_recvmulti && pRef->m_sendmulti && pRef->dropproc)
            {
                current = _Modulo(pRef->m_inp.m_end - 1, PACKET_WINDOW);
                pExisting = (pRef->m_inp.m_ctrlio + pRef->m_inp.m_lkup[current].pos);
                pIncoming = pPacket->body.data;

                if (pRef->dropproc(pRef, pExisting + 1, pIncoming + 1, pExisting[0], pIncoming[0]))
                {
                    next = pRef->m_inp.m_end;
                    pRef->m_inp.m_end = _Modulo(pRef->m_inp.m_end - 1, PACKET_WINDOW);
                }
            }

            _ProcessPacketType(pRef, pPacket->head.kind);
            // check for overflow
            if (next == pRef->m_inp.m_beg)
            {
                // ignore the packet
                NetPrintf(("NetGameDistUpdate: buffer overflow (queue full)!\n"));
                ds_snzprintf(pRef->m_errortext, sizeof(pRef->m_errortext), "NetGameDistUpdate, buffer overflow (queue full).");

                pRef->m_errcond = GMDIST_QUEUE_FULL;
                return((unsigned)(-1));
            }
            else
            {
                // point to data buffer

                // we currently write one byte past the buffer after reserving one extra byte
                // the len set in the data structure includes this extra byte past the packet data
                if (_SetDataPtr(pRef, &pRef->m_inp, pPacket->head.len + 1))
                {
                    pData = pRef->m_inp.m_ctrlio+pRef->m_inp.m_lkup[pRef->m_inp.m_end].pos;
                    pRef->m_inp.m_lkup[pRef->m_inp.m_end].len = pPacket->head.len + 1;
                    // copy into buffer
                    ds_memcpy(pData, pPacket->body.data, pPacket->head.len);
                    pData[pPacket->head.len] = pPacket->head.kind;

                    pRef->m_packetid[pRef->m_inp.m_end] = pRef->iCount;

                    // add item to buffer
                    pRef->m_inp.m_end = next;
                    // display number of packets in buffer
                    #if TIMING_DEBUG
                    NetPrint("NetGameDistUpdate: inpbuf=%d packets\n",
                        _Modulo(pRef->m_inp.m_end - pRef->m_inp.m_beg, PACKET_WINDOW));
                    #endif
                }
                else
                {
                    NetPrintf(("NetGameDistUpdate: buffer overflow (memory)!\n"));
                    pRef->m_errcond = GMDIST_QUEUE_MEMORY;
                    return((unsigned)(-1));
                }
            }
        }
        else if (pPacket->head.kind == GAME_PACKET_STATS)
        {
            pos = 0;
            for (index = 0; index < pRef->m_totplrs; index++)
            {
                ds_memcpy(&pRef->m_recvstats[index], pPacket->body.data + pos, sizeof(*pRef->m_recvstats) );
                pos += sizeof(*pRef->m_recvstats);

                pRef->m_recvstats[index].ping = SocketNtohl(pRef->m_recvstats[index].ping);
            }
        }
        else if (pPacket->head.kind == GAME_PACKET_INPUT_FLOW)
        {
            pRef->m_rdysendremote = pPacket->body.data[0];
            pRef->m_rdyrecvremote = pPacket->body.data[1];

            if (pPacket->head.len >= 7)
            {
                if (pRef->m_maxpkt.body.data[2])
                {
                    pRef->m_remoteCRCvalid = TRUE;
                    pRef->m_remoteCRC = (pRef->m_maxpkt.body.data[6] |
                                        (pRef->m_maxpkt.body.data[5] << 8) |
                                        (pRef->m_maxpkt.body.data[4] << 16) |
                                        (pRef->m_maxpkt.body.data[3] << 24));
                }
                #if DIRTYCODE_LOGGING
                NetPrintfVerbose((pRef->iVerbose, NETGAMEDIST_VERBOSITY, "netgamedist: [0x%08x] m_remoteCRCvalid is %d, m_remoteCRC is %d\n", pRef, pRef->m_remoteCRCvalid, pRef->m_remoteCRC));
                #endif
            }

            NetPrintf(("netgamedist: [0x%08x] Got GAME_PACKET_INPUT_FLOW (send %d recv %d)\n", pRef, pRef->m_rdysendremote, pRef->m_rdyrecvremote));
        }
        else if (pPacket->head.kind == GAME_PACKET_INPUT_META)
        {
            // process meta-information about sparse multi-packets
            uint8_t uVer;
            uint32_t uMask;

            pRef->m_gotMetaInfo = TRUE;

            uVer = _Modulo(pPacket->body.data[0], GMDIST_META_ARRAY_SIZE);
            uMask = ((pPacket->body.data[1] << 24) +
                     (pPacket->body.data[2] << 16) +
                     (pPacket->body.data[3] << 8) +
                     (pPacket->body.data[4]));

            NetPrintf(("netgamedist: Got GAME_PACKET_INPUT_META (version %d mask 0x%08x)\n", uVer, uMask));

            pRef->m_metaInfo[uVer].uMask = uMask;
            pRef->m_metaInfo[uVer].uPlayerCount = _NetGameDistCountBits(pRef, uMask);
            pRef->m_metaInfo[uVer].uVer = uVer;
        }
    }

    // update link status
    NetGameDistStatus(pRef, 'stat', 0, NULL, 0);

    // show the history
    #if PING_DEBUG
    _PingHistory(&pRef->m_stats);
    #endif

    // send meta-information as needed
    if (pRef->m_updatemetainfobeg != pRef->m_updatemetainfoend)
    {
        _NetGameDistSendMetaInfo(pRef);
    }

    // send flow control updates as needed
    if (pRef->m_updateflowctrl)
    {
        _NetGameDistSendFlowUpdate(pRef);
    }

    // return current sequence number
    return(pRef->m_globalseq);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistSetProc

    \Description
        Sets or override the various procedure pointers.

    \Input *pRef        - reference pointer
    \Input kind         - kind
    \Input *proc        - proc

    \Output
        kind            - proc to override. ( 'send','recv','stat' or 'drop' )

    \Notes
        'drop' should only be used on game servers

    \Version    1.0        02/27/07 (jrainy) First Version
*/
/*************************************************************************************************F*/
void NetGameDistSetProc(NetGameDistRefT *pRef, int32_t kind, void* proc)
{
    switch(kind)
    {
    case 'drop':
        pRef->dropproc = (NetGameDistDropProc *)proc;
        break;
    case 'peek':
        pRef->peekproc = (NetGameDistPeekProc *)proc;
        break;
    case 'recv':
        pRef->recvproc = (NetGameDistRecvProc *)proc;
        break;
    case 'send':
        pRef->sendproc = (NetGameDistSendProc *)proc;
        break;
    case 'stat':
        pRef->statproc = (NetGameDistStatProc *)proc;
        break;
    case 'link':
        pRef->linkctrlproc = (NetGameDistLinkCtrlProc *)proc;
        break;
    }
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistSendStats

    \Description
        Send stats. Meant to be used by the game server to send stats regarding all clients

    \Input *pRef         - reference pointer
    \Input *pStats       - m_totplrs-sized array of stats

    \Output

    \Notes

    \Version    1.0        03/14/07 (jrainy) First Version
*/
/*************************************************************************************************F*/
void NetGameDistSendStats(NetGameDistRefT *pRef, NetGameDistStatT *pStats)
{
    uint32_t index;
    int32_t pos;

    pRef->m_maxpkt.head.kind = GAME_PACKET_STATS;
    pRef->m_maxpkt.head.len = (uint16_t)sizeof(NetGameDistStatT) * pRef->m_totplrs;

    pos = 0;
    for (index = 0; index<pRef->m_totplrs; index++)
    {
        ds_memcpy(pRef->m_maxpkt.body.data + pos, &pStats[index], sizeof(NetGameDistStatT));
        pos += sizeof(NetGameDistStatT);
    }

    pRef->sendproc(pRef->linkref, (NetGamePacketT*)&pRef->m_maxpkt, 1);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistGetError

    \Description
        return non-zero if there ever was an overflow

    \Input *pRef         - reference pointer

    \Output
            int32_t      - 0 if there was no error, positive >0 otherwise.

    \Notes

    \Version    1.0        03/23/07 (jrainy) First Version
*/
/*************************************************************************************************F*/
int32_t NetGameDistGetError(NetGameDistRefT *pRef)
{
    return(pRef->m_errcond);
}

/*F*************************************************************************************************/
/*!
    \Function    NetGameDistGetError

    \Description
            copies into the passed buffer the last error text.

    \Input *pRef         - reference pointer
    \Input *pErrorBuffer - buffer to write into
    \Input iBufSize      - buf size

    \Output
            none

    \Notes

    \Version    1.0        08/15/08 (jrainy) First Version
*/
/*************************************************************************************************F*/
void NetGameDistGetErrorText(NetGameDistRefT *pRef, char *pErrorBuffer, int32_t iBufSize)
{
    int32_t iMinSize = sizeof(pRef->m_errortext);

    if (iBufSize < iMinSize)
    {
        iMinSize = iBufSize;
    }

    strncpy(pErrorBuffer, pRef->m_errortext, iMinSize);
    pErrorBuffer[iMinSize - 1] = 0;
}

// reset the error count
/*F*************************************************************************************************/
/*!
    \Function    NetGameDistResetError

    \Description
        reset the overflow error condition to 0.

    \Input *pRef         - reference pointer

    \Output

    \Notes

    \Version    1.0        03/23/07 (jrainy) First Version
*/
/*************************************************************************************************F*/
void NetGameDistResetError(NetGameDistRefT *pRef)
{
    pRef->m_errcond = 0;
}


