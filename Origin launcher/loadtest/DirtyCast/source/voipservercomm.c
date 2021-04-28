/*H********************************************************************************/
/*!
    \File voipservercomm.c

    \Description
        This module contains a set of utility functions around working with the
        comm modules that are used on the voipserver

    \Copyright
        Copyright (c) 2015 Electronic Arts Inc.

    \Version 09/17/2015 (eesponda)
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/comm/commudputil.h"

#include "dirtycast.h"
#include "voipservercomm.h"

/*** Defines **********************************************************************/

/*** Private functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function VoipServerDecodeU32

    \Description
        Read a u32 in network order.

    \Input *pValue  - pointer to data to extract

    \Output
        uint32_t    - extracted 32bit value

    \Version 05/16/2006 (jbrookes)
*/
/********************************************************************************F*/
uint32_t VoipServerDecodeU32(const uint8_t *pValue)
{
    uint32_t uValue = pValue[0] << 24;
    uValue |= pValue[1] << 16;
    uValue |= pValue[2] << 8;
    uValue |= pValue[3];
    return(uValue);
}

/*F********************************************************************************/
/*!
    \Function VoipServerExtractNetGameLinkLate

    \Description
        Extract latency information (if available from the given packet).

    \Input *pPacketData - pointer to packet head
    \Input uPacketLen   - size of received data
    \Input *pLatency    - [out] calculated latency value

    \Output
        int32_t         - >0=success; 0=no latency info found; <0=failure

    \Version 09/24/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipServerExtractNetGameLinkLate(const CommUDPPacketHeadT *pPacket, uint32_t uPacketLen, uint32_t *pLatency)
{
    uint32_t uSeq = VoipServerDecodeU32(pPacket->aSeq);
    const uint32_t uMetaType = CommUDPUtilGetMetaTypeFromSeq(uSeq);
    const uint8_t uMetaSize = CommUDPUtilGetMetaSize(uMetaType);
    const uint8_t *pPacketData = (const uint8_t *)pPacket;
    NetGamePacketSyncT *pSync;
    int32_t iBytesToProcess = uPacketLen;
    int32_t iMainSubPktLen;

    // mask out metatype from sequence (i.e. clear bits 24, 25, 26, 27)
    uSeq &= ~(0xf << COMMUDP_SEQ_META_SHIFT);

    // if control packet or keep-alive (zero-length payload), ignore it
    if ((uSeq < COMMUDP_RAW_PACKET_UNREL) || (uPacketLen <= (sizeof(CommUDPPacketHeadT) + uMetaSize)))
    {
        return(0); // no latency info found
    }

    // if a commudp multipacket, attempt extracting latency from the main subpacket only
    if (uSeq > COMMUDP_SEQ_MULTI_INC)
    {
        // extract from seq field, the count of extra subpackets appended after the first subpacket (located in bits 28, 29, 30, 31)
        int32_t iExtraSubPktCount = CommUDPUtilGetExtraSubPktCountFromSeq(uSeq);

        // process all the subpackets backward (from end of payload to beginning of payload) - excluding the main subpacket
        for (; iExtraSubPktCount > 0; --iExtraSubPktCount)
        {
            int32_t iSubPktLen;

            // find subpacket size at end of the subpacket, and remove from iBytesToProcess the number of
            // bytes (1 or 2) consumed to store that size.
            iBytesToProcess -= CommUDPUtilDecodeSubpacketSize(pPacketData + iBytesToProcess - 1, &iSubPktLen);

            // forward validate that advancing to next subpacket will work
            if (iBytesToProcess - iSubPktLen < 0)
            {
                NetPrintf(("voipservercomm: can't walk the multipacket because formatting is detected to be wrong (%d vs %d)\n", iBytesToProcess, iSubPktLen));
                return(-1);
            }

            // move to next subpacket
            iBytesToProcess -= iSubPktLen;
        }

        // we now know the size of the main subpacket 
        iMainSubPktLen = iBytesToProcess;
    }
    else
    {
        // the size of the main subpacket is same as the total size of the packet
        iMainSubPktLen = uPacketLen;
    }

    // check if main subpacket contains a sync msg (netgamelink always appends the packet 'kind' after the packet payload)
    if ((pPacketData[iMainSubPktLen-1] & GAME_PACKET_SYNC) == 0)
    {
        return(0); // no latency info found
    }

    // validate sync packet size is what we are expecting (sync packet size must come last)
    if (pPacketData[iMainSubPktLen-2] != sizeof(*pSync))
    {
        NetPrintf(("voipservercomm: can't retrieve latency info because formatting is detected to be wrong (%d vs %d)\n", pPacketData[iMainSubPktLen - 2], sizeof(*pSync)));
        return(-2);
    }

    // find the sync packet
    pSync = (NetGamePacketSyncT *)(pPacketData + iMainSubPktLen - 1 - sizeof(*pSync));

    // extract latency data
    (*pLatency) = SocketNtohs(pSync->late);

    return(1);
}

/*F********************************************************************************/
/*!
    \Function VoipServerTouchNetGameLinkEcho

    \Description
        Update netgamelink sync.echo with voipserver timestamp (if present in the packet).

    \Input *pPacketData - pointer to packet head
    \Input uPacketLen   - size of received data

    \Version 08/31/2017 (mclouatre)
*/
/********************************************************************************F*/
void VoipServerTouchNetGameLinkEcho(CommUDPPacketHeadT *pPacket, uint32_t uPacketLen)
{
    uint32_t uSeq = VoipServerDecodeU32(pPacket->aSeq);
    const uint32_t uMetaType = CommUDPUtilGetMetaTypeFromSeq(uSeq);
    const uint8_t uMetaSize = CommUDPUtilGetMetaSize(uMetaType);
    const uint8_t *pPacketData = (const uint8_t *)pPacket;
    NetGamePacketSyncT *pSync;
    int32_t iExtraSubPktCount;
    int32_t iBytesToProcess = uPacketLen;

    // mask out metatype from sequence (i.e. clear bits 24, 25, 26, 27)
    uSeq &= ~(0xf << COMMUDP_SEQ_META_SHIFT);

    // if control packet or keep-alive (zero-length payload), ignore it
    if ((uSeq < COMMUDP_RAW_PACKET_UNREL) || (uPacketLen <= (sizeof(CommUDPPacketHeadT) + uMetaSize)))
    {
        return;
    }

    // extract from seq field, the count of extra subpackets appended after the first subpacket (located in bits 28, 29, 30, 31)
    iExtraSubPktCount = CommUDPUtilGetExtraSubPktCountFromSeq(uSeq);

    // process all the subpackets backward (from end of payload to beginning of payload) - including main subpacket
    for (; iExtraSubPktCount >= 0; --iExtraSubPktCount)
    {
        int32_t iSubPktLen;
        
        if (iExtraSubPktCount > 0)
        {
            // find subpacket size at end of the subpacket, and remove from iBytesToProcess the number of
            // bytes (1 or 2) consumed to store that size.
            iBytesToProcess -= CommUDPUtilDecodeSubpacketSize(pPacketData + iBytesToProcess - 1, &iSubPktLen);
        }
        else
        {
            // the main subpacket does not have its own size encoded and packed following it
            iSubPktLen = iBytesToProcess;
        }

        // forward validate that advancing to next subpacket will work
        if ((iBytesToProcess - iSubPktLen) < 0)
        {
            NetPrintf(("voipservercomm: can't complete updating sync.echo in multipacket because formatting is detected to be wrong (%d vs %d)\n", iBytesToProcess, iSubPktLen));
            return;
        }

        // check if subpacket contains a sync msg (netgamelink always appends the packet 'kind' after the packet payload)
        if (pPacketData[iBytesToProcess-1] & GAME_PACKET_SYNC)
        {
            // sync packet size must come last
            int32_t iSyncSize = *(pPacketData + iBytesToProcess - 2);

            // validate sync size is consistent with what we are expecting 
            if (iSyncSize == (signed)sizeof(*pSync))
            {
                // locate beginning of sync
                pSync = (NetGamePacketSyncT *)(pPacketData + iBytesToProcess - 1 - sizeof(*pSync));

                // override gameserver's timestamp with voipserver's timestamp
                pSync->echo = SocketHtonl(NetTick());
            }
            else
            {
                NetPrintf(("voipservercomm: can't update sync.echo in packet because sync packet size is unexpected (%d vs %d)\n", iSyncSize, sizeof(*pSync)));
            }
        }

        // move to next subpacket
        iBytesToProcess -= iSubPktLen;
    }

    return;
}

/*F********************************************************************************/
/*!
    \Function VoipServerExtractMeta1Data

    \Description
        Extract meta1 chunk from packet.

    \Input *pPacket     - pointer to packet head
    \Input iPacketSize  - size of received data
    \Input *pMeta1Data  - [out] meta1data that is extracted

    \Output
        int32_t         - zero=no routing info,>0=success,<0=failure

    \Version 02/12/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipServerExtractMeta1Data(const CommUDPPacketHeadT *pPacket, int32_t iPacketSize, CommUDPPacketMeta1T *pMeta1Data)
{
    int32_t iMetaOffset;
    const uint8_t *pPacketData = (const uint8_t *)pPacket;
    uint32_t uSeq = VoipServerDecodeU32(pPacket->aSeq);
    const uint32_t uMetaType = CommUDPUtilGetMetaTypeFromSeq(uSeq);

    // mask out metatype from sequence (i.e. clear bits 24, 25, 26, 27)
    uSeq &= ~(0xf << COMMUDP_SEQ_META_SHIFT);

    // find offset past comm header (uClientId only exists in INIT/POKE packets)
    iMetaOffset = ((uSeq == COMMUDP_RAW_PACKET_INIT) || (uSeq == COMMUDP_RAW_PACKET_POKE)) ?
        sizeof(*pPacket) :
        (sizeof(*pPacket) - sizeof(pPacket->uClientId));

    // is extended routing info present?
    if (uMetaType != 1)
    {
        return(0);
    }

    // make sure we have room
    if (iPacketSize < (iMetaOffset + COMMUDP_RAW_METATYPE1_SIZE))
    {
        return(-1);
    }

    // copy it out
    pMeta1Data->uSourceClientId  = ((uint32_t)(pPacketData[iMetaOffset+0]))<<24;
    pMeta1Data->uSourceClientId |= ((uint32_t)(pPacketData[iMetaOffset+1]))<<16;
    pMeta1Data->uSourceClientId |= ((uint32_t)(pPacketData[iMetaOffset+2]))<<8;
    pMeta1Data->uSourceClientId |= ((uint32_t)(pPacketData[iMetaOffset+3]));

    pMeta1Data->uTargetClientId  = ((uint32_t)pPacketData[iMetaOffset+4])<<24;
    pMeta1Data->uTargetClientId |= ((uint32_t)pPacketData[iMetaOffset+5])<<16;
    pMeta1Data->uTargetClientId |= ((uint32_t)pPacketData[iMetaOffset+6])<<8;
    pMeta1Data->uTargetClientId |= ((uint32_t)pPacketData[iMetaOffset+7]);

    return(1);
}

/*F********************************************************************************/
/*!
    \Function _VoipServerValidateCommUDPPacket

    \Description
        Locate packet data and validate packet

    \Input *pPacketData - pointer to packet head
    \Input iPacketSize  - size of received data

    \Output
        CommUDPPacketHeadT * - pointer to packet head=success, NULL=failure

    \Version 02/12/2007 (jbrookes)
*/
/********************************************************************************F*/
CommUDPPacketHeadT *VoipServerValidateCommUDPPacket(const char *pPacketData, int32_t iPacketSize)
{
    uint32_t uMetaSize;
    CommUDPPacketHeadT *pPacketHead = (CommUDPPacketHeadT *)pPacketData;
    uint32_t uSeq = VoipServerDecodeU32(pPacketHead->aSeq);
    uint32_t uMetaType = CommUDPUtilGetMetaTypeFromSeq(uSeq);

    // mask out metatype from sequence (i.e. clear bits 24, 25, 26, 27)
    uSeq &= ~(0xf << COMMUDP_SEQ_META_SHIFT);

    // only consider INIT or POKE packets
    if ((uSeq != COMMUDP_RAW_PACKET_INIT) && (uSeq != COMMUDP_RAW_PACKET_POKE))
    {
        NetPrintf(("voipservercomm: ignoring commudp packet type %d for match\n", uSeq));
        return(NULL);
    }

    // infer meta size based on type
    uMetaSize = CommUDPUtilGetMetaSize(uMetaType);

    // minimum size check for commudp INIT/POKE packet size
    if ((uint32_t)iPacketSize < (sizeof(CommUDPPacketHeadT) + uMetaSize))
    {
        NetPrintf(("voipservercomm: ignoring commudp packet of size %d for match (expected at least %d bytes)\n", iPacketSize, sizeof(CommUDPPacketHeadT) + uMetaSize));
        NetPrintf(("voipservercomm: uMetaType=%d. uConnType=%d\n", uMetaType, uSeq));
        return(NULL);
    }

    // return packet data
    return(pPacketHead);
}

/*F********************************************************************************/
/*!
    \Function VoipServerSocketOpen

    \Description
        Open a DirtySock socket.

    \Input iType            - SOCK_DGRAM or SOCK_STREAM
    \Input uLocalBindAddr   - local address to bind to, such as the front facing address "pConfig->uFrontAddr", 0 to not specify
    \Input uPort            - port to bind
    \Input bReuseAddr       - if TRUE, set SO_REUSEADDR
    \Input *pCallback       - socket callback
    \Input *pUserData       - user specific state

    \Output
        SocketT *       - socket ref, or NULL

    \Version 03/27/2006 (jbrookes)
*/
/********************************************************************************F*/
SocketT *VoipServerSocketOpen(int32_t iType, uint32_t uLocalBindAddr, uint16_t uPort, uint32_t bReuseAddr, SocketRecvCb *pRecvCb, void *pUserData)
{
    struct sockaddr BindAddr;
    SocketT *pSocket;
    int32_t iResult;

    // open the socket
    if ((pSocket = SocketOpen(AF_INET, iType, IPPROTO_IP)) == NULL)
    {
        DirtyCastLogPrintf("voipservercomm: unable to open socket\n");
        return(NULL);
    }

    // set SO_REUSEADDR
    if (bReuseAddr == TRUE)
    {
        SocketControl(pSocket, 'radr', TRUE, NULL, NULL);
    }

    SockaddrInit(&BindAddr, AF_INET);
    // bind socket to specific local address
    if (uLocalBindAddr != 0)
    {
        SockaddrInSetAddr(&BindAddr, uLocalBindAddr);
    }
    // bind socket to specific port
    SockaddrInSetPort(&BindAddr, uPort);
    
    if ((iResult = SocketBind(pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
    {
        DirtyCastLogPrintf("voipservercomm: error %d binding %A.\n", iResult, &BindAddr);
        SocketClose(pSocket);
        return(NULL);
    }

    if (pRecvCb != NULL)
    {
        // set up for socket callback events
        SocketCallback(pSocket, CALLB_RECV, 100, pUserData, pRecvCb);
    }

    // return ref to caller
    return(pSocket);
}
