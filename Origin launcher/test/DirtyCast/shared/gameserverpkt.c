/*H********************************************************************************/
/*!
    \File gameserverpkt.c

    \Description
        This module contains helper functions for encoding/decoding packets
        sent between the GameServer and VoipServer.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 04/03/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <string.h>
#include <stdlib.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "LegacyDirtySDK/util/tagfield.h"

#include "dirtycast.h"
#include "gameserverpkt.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

#if GAMESERVER_PACKETHISTORY
static GameServerPacketT _GameServerPkt_aSendHistory[GAMESERVER_PACKETHISTORY];
static GameServerPacketT _GameServerPkt_aRecvHistory[GAMESERVER_PACKETHISTORY];
static int32_t           _GameServerPkt_aSendSize[GAMESERVER_PACKETHISTORY];
static int32_t           _GameServerPkt_aRecvSize[GAMESERVER_PACKETHISTORY];
static int32_t           _GameServerPkt_iSendIdx = 0;
static int32_t           _GameServerPkt_iRecvIdx = 0;
#endif

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _GameServerUpdateGameStatsSum

    \Description
        Sum current stats into accumulation buffer.

    \Input *pSum    - buffer to accumulate total into
    \Input *pCur    - pointer to current stats

    \Version 04/12/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerUpdateGameStatsSum(GameServerStatsT *pSum, GameServerStatsT *pCur)
{
    pSum->GameStat.uByteRecv += pCur->GameStat.uByteRecv;
    pSum->GameStat.uByteSent += pCur->GameStat.uByteSent;
    pSum->GameStat.uPktsRecv += pCur->GameStat.uPktsRecv;
    pSum->GameStat.uPktsSent += pCur->GameStat.uPktsSent;
    pSum->VoipStat.uByteRecv += pCur->VoipStat.uByteRecv;
    pSum->VoipStat.uByteSent += pCur->VoipStat.uByteSent;
    pSum->VoipStat.uPktsRecv += pCur->VoipStat.uPktsRecv;
    pSum->VoipStat.uPktsSent += pCur->VoipStat.uPktsSent;
}

/*F********************************************************************************/
/*!
    \Function _GameServerUpdateGameStatsMax

    \Description
        Update max bandwidth stats.

    \Input *pMax    - buffer to store max into
    \Input *pCur    - pointer to current stats

    \Version 04/12/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _GameServerUpdateGameStatsMax(GameServerStatsT *pMax, GameServerStatsT *pCur)
{
    if (pMax->GameStat.uByteRecv < pCur->GameStat.uByteRecv)
    {
        pMax->GameStat.uByteRecv = pCur->GameStat.uByteRecv;
        pMax->GameStat.uPktsRecv = pCur->GameStat.uPktsRecv;
    }
    if (pMax->GameStat.uByteSent < pCur->GameStat.uByteSent)
    {
        pMax->GameStat.uByteSent = pCur->GameStat.uByteSent;
        pMax->GameStat.uPktsSent = pCur->GameStat.uPktsSent;
    }
    if (pMax->VoipStat.uByteRecv < pCur->VoipStat.uByteRecv)
    {
        pMax->VoipStat.uByteRecv = pCur->VoipStat.uByteRecv;
        pMax->VoipStat.uPktsRecv = pCur->VoipStat.uPktsRecv;
    }
    if (pMax->VoipStat.uByteSent < pCur->VoipStat.uByteSent)
    {
        pMax->VoipStat.uByteSent = pCur->VoipStat.uByteSent;
        pMax->VoipStat.uPktsSent = pCur->VoipStat.uPktsSent;
    }
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function GameServerPacketTranslate

    \Description
        Translate packet fields from network to host order.

    \Input *pOutPacket  - [out] storage for result
    \Input *pInpPacket  - source packet
    \Input uKind        - packet kind
    \Input *pXlateL     - uint32_t xlate func
    \Input *pXlateS     - uint16_t xlate func

    \Output
        uint32_t         - size of type-specific subpacket

    \Version 03/02/2007 (jbrookes)
*/
/********************************************************************************F*/
uint32_t GameServerPacketTranslate(GameServerPktT *pOutPacket, const GameServerPktT *pInpPacket, uint32_t uKind, uint32_t (*pXlateL)(uint32_t uValue), uint16_t (*pXlateS)(uint16_t uValue))
{
    int32_t iSubpacketSize;
    if (uKind == 'conf')
    {
        pOutPacket->Config.uFrontAddr = pXlateL(pInpPacket->Config.uFrontAddr);
        pOutPacket->Config.uTunnelPort = pXlateS(pInpPacket->Config.uTunnelPort);
        pOutPacket->Config.uDiagPort = pXlateS(pInpPacket->Config.uDiagPort);
        iSubpacketSize = sizeof(pOutPacket->Config);
    }
    else if (uKind == 'data')
    {
        pOutPacket->GameData.iClientId = pXlateL(pInpPacket->GameData.iClientId);
        pOutPacket->GameData.uTimestamp = pXlateL(pInpPacket->GameData.uTimestamp);
        iSubpacketSize = sizeof(pOutPacket->GameData);
    }
    else if (uKind == 'nusr')
    {
        pOutPacket->NewUser.iClientId = pXlateL(pInpPacket->NewUser.iClientId);
        pOutPacket->NewUser.iGameServerId = pXlateL(pInpPacket->NewUser.iGameServerId);
        pOutPacket->NewUser.iClientIdx = pXlateL(pInpPacket->NewUser.iClientIdx);
        iSubpacketSize = sizeof(pOutPacket->NewUser);
    }
    else if (uKind == 'dusr')
    {
        pOutPacket->DelUser.iClientId = pXlateL(pInpPacket->DelUser.iClientId);
        pOutPacket->DelUser.uGameState = pXlateL(pInpPacket->DelUser.uGameState);
        pOutPacket->DelUser.RemovalReason.uRemovalReason = pXlateL(pInpPacket->DelUser.RemovalReason.uRemovalReason);
        iSubpacketSize = sizeof(pOutPacket->DelUser);
    }
    else if (uKind == 'stat')
    {
        uint32_t uCount;
        pOutPacket->GameStatInfo.E2eLate.uMinLate = pXlateS(pInpPacket->GameStatInfo.E2eLate.uMinLate);
        pOutPacket->GameStatInfo.E2eLate.uMaxLate = pXlateS(pInpPacket->GameStatInfo.E2eLate.uMaxLate);
        pOutPacket->GameStatInfo.E2eLate.uSumLate = pXlateL(pInpPacket->GameStatInfo.E2eLate.uSumLate);
        pOutPacket->GameStatInfo.E2eLate.uNumStat = pXlateL(pInpPacket->GameStatInfo.E2eLate.uNumStat);
        for (uCount = 0; uCount < GAMESERVER_MAXLATEBINS; uCount += 1)
        {
            pOutPacket->GameStatInfo.LateBin.aBins[uCount] = pXlateS(pInpPacket->GameStatInfo.LateBin.aBins[uCount]);
        }
        pOutPacket->GameStatInfo.InbLate.uMinLate = pXlateS(pInpPacket->GameStatInfo.InbLate.uMinLate);
        pOutPacket->GameStatInfo.InbLate.uMaxLate = pXlateS(pInpPacket->GameStatInfo.InbLate.uMaxLate);
        pOutPacket->GameStatInfo.InbLate.uSumLate = pXlateL(pInpPacket->GameStatInfo.InbLate.uSumLate);
        pOutPacket->GameStatInfo.InbLate.uNumStat = pXlateL(pInpPacket->GameStatInfo.InbLate.uNumStat);
        pOutPacket->GameStatInfo.OtbLate.uMinLate = pXlateS(pInpPacket->GameStatInfo.OtbLate.uMinLate);
        pOutPacket->GameStatInfo.OtbLate.uMaxLate = pXlateS(pInpPacket->GameStatInfo.OtbLate.uMaxLate);
        pOutPacket->GameStatInfo.OtbLate.uSumLate = pXlateL(pInpPacket->GameStatInfo.OtbLate.uSumLate);
        pOutPacket->GameStatInfo.OtbLate.uNumStat = pXlateL(pInpPacket->GameStatInfo.OtbLate.uNumStat);
        pOutPacket->GameStatInfo.uRlmtChangeCount = pXlateL(pInpPacket->GameStatInfo.uRlmtChangeCount);
        pOutPacket->GameStatInfo.PackLostSent.uLpackLost = pXlateL(pInpPacket->GameStatInfo.PackLostSent.uLpackLost);
        pOutPacket->GameStatInfo.PackLostSent.uRpackLost = pXlateL(pInpPacket->GameStatInfo.PackLostSent.uRpackLost);
        pOutPacket->GameStatInfo.PackLostSent.uLpackSent = pXlateL(pInpPacket->GameStatInfo.PackLostSent.uLpackSent);
        pOutPacket->GameStatInfo.PackLostSent.uRpackSent = pXlateL(pInpPacket->GameStatInfo.PackLostSent.uRpackSent);

        iSubpacketSize = sizeof(pOutPacket->GameStatInfo);
    }
    else if (uKind == 'ngam')
    {
        pOutPacket->NewGame.iIdent = pXlateL(pInpPacket->NewGame.iIdent);
        iSubpacketSize = sizeof(pOutPacket->NewGame);
    }
    else if (uKind == 'dgam')
    {
        pOutPacket->DelGame.uGameEndState = pXlateL(pInpPacket->DelGame.uGameEndState);
        pOutPacket->DelGame.uRlmtChangeCount = pXlateL(pInpPacket->DelGame.uRlmtChangeCount);
        iSubpacketSize = sizeof(pOutPacket->DelGame);
    }
    else if (uKind == 'ping')
    {
        pOutPacket->PingData.eState = pXlateL(pInpPacket->PingData.eState);
        iSubpacketSize = sizeof(pOutPacket->PingData);
    }
    else if (uKind == 'stat')
    {
        pOutPacket->GameStats.GameStat.uPktsRecv = pXlateS(pInpPacket->GameStats.GameStat.uPktsRecv);
        pOutPacket->GameStats.GameStat.uPktsSent = pXlateS(pInpPacket->GameStats.GameStat.uPktsSent);
        pOutPacket->GameStats.GameStat.uByteRecv = pXlateL(pInpPacket->GameStats.GameStat.uByteRecv);
        pOutPacket->GameStats.GameStat.uByteSent = pXlateL(pInpPacket->GameStats.GameStat.uByteSent);
        pOutPacket->GameStats.VoipStat.uPktsRecv = pXlateS(pInpPacket->GameStats.VoipStat.uPktsRecv);
        pOutPacket->GameStats.VoipStat.uPktsSent = pXlateS(pInpPacket->GameStats.VoipStat.uPktsSent);
        pOutPacket->GameStats.VoipStat.uByteRecv = pXlateL(pInpPacket->GameStats.VoipStat.uByteRecv);
        pOutPacket->GameStats.VoipStat.uByteSent = pXlateL(pInpPacket->GameStats.VoipStat.uByteSent);
        iSubpacketSize = sizeof(pOutPacket->GameStats);
    }
    else if (uKind == 'tokn')
    {
        iSubpacketSize = sizeof(pOutPacket->Token);
    }
    else
    {
        iSubpacketSize = 0;
    }
    return(iSubpacketSize);
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketEncode

    \Description
        Helper function to prepare a GameServerPacket for sending.

    \Input *pPacket     - [out] storage for packet to send
    \Input uKind        - kind of packet to send
    \Input uCode        - code of packet to send
    \Input *pData       - data to send (NULL means no data)
    \Input iDataSize    - size of data to send

    \Output
        uint32_t        - size of packet to send

    \Version 03/02/2007 (jbrookes)
*/
/********************************************************************************F*/
uint32_t GameServerPacketEncode(GameServerPacketT *pPacket, uint32_t uKind, uint32_t uCode, void *pData, int32_t iDataSize)
{
    uint32_t uSize;

    // format packet header
    pPacket->Header.uKind = SocketHtonl(uKind);
    pPacket->Header.uCode = SocketHtonl(uCode);

    // copy packet data
    if (iDataSize > (signed)sizeof(pPacket->Packet))
    {
        DirtyCastLogPrintf("gameserverpacket: warning -- data for packet of type '%c%c%c%c' is too large; truncating\n",
            (uKind>>24)&0xff, (uKind>>16)&0xff, (uKind>>8)&0xff, uKind&0xff);
        iDataSize = sizeof(pPacket->Packet);
    }
    if (pData != NULL)
    {
        ds_memcpy(pPacket->Packet.aData, pData, iDataSize);
    }
    uSize = sizeof(pPacket->Header) + iDataSize;

    // translate packet fields from host to network order and accumulate subpacket size
    GameServerPacketHton(pPacket, pPacket, uKind);

    // store packet size
    pPacket->Header.uSize = SocketHtonl(uSize);

    // return packet size to caller
    return(uSize);
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketSend

    \Description
        Send a packet to a remote host

    \Input *pBufferedSocket - module state
    \Input *pPacket         - packet to send
    \Input iBufLen          - length of data to send

    \Output
        int32_t             - negative=failure, else success

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t GameServerPacketSend(BufferedSocketT *pBufferedSocket, const GameServerPacketT *pPacket)
{
    // read length from packet header
    int32_t iPacketLen = SocketNtohl(pPacket->Header.uSize), iResult;

    // make sure size is valid
    if ((iPacketLen < 0) || (iPacketLen > (signed)sizeof(*pPacket)))
    {
        DirtyCastLogPrintf("gameserverpkt: error -- send packet size is invalid (%d bytes, max=%d bytes)\n", iPacketLen, (signed)sizeof(*pPacket));
        return(-1);
    }

    // send packet
    if ((iResult = BufferedSocketSend(pBufferedSocket, (const char *)pPacket, iPacketLen)) != iPacketLen)
    {
        DirtyCastLogPrintf("gameserverpkt: BufferedSocketSend() of %d byte packet returned %d\n", iPacketLen, iResult);
        return(-1);
    }

    // save to packet history
    #if GAMESERVER_PACKETHISTORY
    ds_memclr(&_GameServerPkt_aSendHistory[_GameServerPkt_iSendIdx], sizeof(_GameServerPkt_aSendHistory[0]));
    ds_memcpy(&_GameServerPkt_aSendHistory[_GameServerPkt_iSendIdx], pPacket, iPacketLen);
    _GameServerPkt_aSendSize[_GameServerPkt_iSendIdx] = iPacketLen;
    _GameServerPkt_iSendIdx = (_GameServerPkt_iSendIdx + 1) % GAMESERVER_PACKETHISTORY;
    #endif
    return(0);
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketRecv

    \Description
        Receive a packet from a buffered socket

    \Input *pBufferedSocket - module state
    \Input *pPacket         - packet buffer to receive into
    \Input *pPacketSize     - size of packet

    \Output
        int32_t             - negative=failure, else success

    \Version 04/23/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t GameServerPacketRecv(BufferedSocketT *pBufferedSocket, GameServerPacketT *pPacket, uint32_t *pPacketSize)
{
    int32_t iRecvLen;

    // do we have a full packet header yet?
    if (*pPacketSize < sizeof(pPacket->Header))
    {
        // receive packet header
        if ((iRecvLen = BufferedSocketRecv(pBufferedSocket, ((char *)pPacket)+*pPacketSize, sizeof(pPacket->Header) - *pPacketSize)) <= 0)
        {
            if (iRecvLen < 0)
            {
                DirtyCastLogPrintf("gameserverpkt: error %d receiving data\n", iRecvLen);
            }
            return(iRecvLen);
        }
        // accumulate received amount
        *pPacketSize += iRecvLen;
        // if we've got the packet header, convert it from network to host order
        if (*pPacketSize == sizeof(pPacket->Header))
        {
            // convert header from network to host order
            pPacket->Header.uKind = SocketNtohl(pPacket->Header.uKind);
            pPacket->Header.uCode = SocketNtohl(pPacket->Header.uCode);
            pPacket->Header.uSize = SocketNtohl(pPacket->Header.uSize);

            // make sure size is valid
            if (pPacket->Header.uSize > sizeof(*pPacket))
            {
                DirtyCastLogPrintf("gameserverpkt: error -- received packet is too large (%d bytes, max=%d bytes)\n", pPacket->Header.uSize, (signed)sizeof(*pPacket));
                return(-1);
            }
        }
        else
        {
            return(0);
        }
    }

    // try and receive packet data
    if (*pPacketSize < pPacket->Header.uSize)
    {
        // receive packet data
        if ((iRecvLen = BufferedSocketRecv(pBufferedSocket, ((char *)pPacket)+*pPacketSize, pPacket->Header.uSize - *pPacketSize)) <= 0)
        {
            if (iRecvLen < 0)
            {
                DirtyCastLogPrintf("gameserverpkt: error %d receiving data\n", iRecvLen);
            }
            return(iRecvLen);
        }

        // add in received data
        *pPacketSize += iRecvLen;
    }

    // are we done?
    if (*pPacketSize == pPacket->Header.uSize)
    {
        #if GAMESERVER_PACKETHISTORY
        // save to packet history
        ds_memcpy(&_GameServerPkt_aRecvHistory[_GameServerPkt_iRecvIdx], pPacket, sizeof(_GameServerPkt_aRecvHistory[0]));
        _GameServerPkt_aRecvSize[_GameServerPkt_iRecvIdx] = *pPacketSize;
        _GameServerPkt_iRecvIdx = (_GameServerPkt_iRecvIdx + 1) % GAMESERVER_PACKETHISTORY;
        #endif

        return(1);
    }
    else
    {
        return(0);
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketUpdateLateStats

    \Description
        Update latency statistics.

    \Input *pLateOut    - [out] buffer to update with new latency info
    \Input *pLateIn     - source buffer (or NULL to init out)

    \Output
        None.

    \Version 06/29/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerPacketUpdateLateStats(GameServerLateT *pLateOut, const GameServerLateT *pLateIn)
{
    if (pLateIn != NULL)
    {
        if (pLateIn->uNumStat != 0)
        {
            // update min and max
            if ((pLateOut->uMinLate > pLateIn->uMinLate) || (pLateOut->uMinLate == 0))
            {
                pLateOut->uMinLate = pLateIn->uMinLate;
            }
            if (pLateOut->uMaxLate < pLateIn->uMaxLate)
            {
                pLateOut->uMaxLate = pLateIn->uMaxLate;
            }
            // accumulate into average
            pLateOut->uSumLate += pLateIn->uSumLate;
            // update stat count
            pLateOut->uNumStat += pLateIn->uNumStat;
        }
    }
    else
    {
        // init latency stats
        ds_memclr(pLateOut, sizeof(*pLateOut));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketUpdateLateStatsSingle

    \Description
        Update latency statistics with a single latency value

    \Input *pLateOut    - [out] buffer to update with new latency info
    \Input uLateIn      - input latency value.

    \Version 05/12/2010 (jbrookes)
*/
/********************************************************************************F*/
void GameServerPacketUpdateLateStatsSingle(GameServerLateT *pLateOut, uint32_t uLateIn)
{
    GameServerLateT LateStat;
    LateStat.uMinLate = uLateIn;
    LateStat.uMaxLate = uLateIn;
    LateStat.uSumLate = uLateIn;
    LateStat.uNumStat = 1;
    GameServerPacketUpdateLateStats(pLateOut, &LateStat);
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketLogLateStats

    \Description
        Write latency statistics to log output.

    \Input *pLateStat   - latency stats to log
    \Input *pPrefix     - text to prefix output with
    \Input *pSuffix     - text to append to output

    \Note
        A linefeed character is not included, so as to allow concatentation, so if
        a linefeed is required, it must be included in the suffix text.

    \Version 04/06/2009 (jbrookes)
*/
/********************************************************************************F*/
void GameServerPacketLogLateStats(const GameServerLateT *pLateStat, const char *pPrefix, const char *pSuffix)
{
    uint32_t uAvgLate = (pLateStat->uNumStat != 0) ? pLateStat->uSumLate/pLateStat->uNumStat : 0;
    DirtyCastLogPrintf("%s lmin: %3dms lmax: %3dms lavg: %3dms (%4d samples)%s", pPrefix,
        pLateStat->uMinLate, pLateStat->uMaxLate, uAvgLate, pLateStat->uNumStat,
        pSuffix);
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketUpdatePackLostSentStats

    \Description
        Update packet lost statistics.

    \Input *pPrevPackLostSent         - [out] buffer to update with new packet loss sent info
    \Input *pDeltaSumPackLostSentOut  - [out] buffer to update with new delta packet loss sent info
    \Input *pCurrentPackLostSent      - source buffer (or NULL to init out)

    \Output
        None.

    \Version 08/16/2019 (tcho)
*/
/********************************************************************************F*/
void GameServerPacketUpdatePackLostSentStats(GameServerPacketLostSentT *pPrevPackLostSent, GameServerPacketLostSentT *pDeltaSumPackLostSentOut, const GameServerPacketLostSentT *pCurrentPackLostSent)
{
    if (pCurrentPackLostSent != NULL)
    {
        // ignore inital values after a reset
        if (pPrevPackLostSent->uLpackSent != 0)
        {
            // do not update if rollover detection or when values are reset on the GS
            if ((pPrevPackLostSent->uLpackSent <= pCurrentPackLostSent->uLpackSent) && (pPrevPackLostSent->uRpackLost <= pCurrentPackLostSent->uRpackLost))
            {
                pDeltaSumPackLostSentOut->uLpackSent += (pCurrentPackLostSent->uLpackSent - pPrevPackLostSent->uLpackSent);
                pDeltaSumPackLostSentOut->uRpackLost += (pCurrentPackLostSent->uRpackLost - pPrevPackLostSent->uRpackLost);
            }
        }
        
        // ignore inital values after a reset
        if (pPrevPackLostSent->uRpackSent != 0)
        {
            // do not update if rollover detection or when values are reset on the GS
            if ((pPrevPackLostSent->uRpackSent <= pCurrentPackLostSent->uRpackSent) && (pPrevPackLostSent->uLpackLost <= pCurrentPackLostSent->uLpackLost))
            {
                pDeltaSumPackLostSentOut->uRpackSent += (pCurrentPackLostSent->uRpackSent - pPrevPackLostSent->uRpackSent);
                pDeltaSumPackLostSentOut->uLpackLost += (pCurrentPackLostSent->uLpackLost - pPrevPackLostSent->uLpackLost);
            }
        }

        pPrevPackLostSent->uLpackSent = pCurrentPackLostSent->uLpackSent;
        pPrevPackLostSent->uRpackSent = pCurrentPackLostSent->uRpackSent;
        pPrevPackLostSent->uRpackLost = pCurrentPackLostSent->uRpackLost;
        pPrevPackLostSent->uLpackLost = pCurrentPackLostSent->uLpackLost;
    }
    else
    {
        // clear out the stats
        ds_memclr(pPrevPackLostSent, sizeof(*pPrevPackLostSent));
        ds_memclr(pDeltaSumPackLostSentOut, sizeof(*pDeltaSumPackLostSentOut));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketUpdatePackLostSentStatSingle

    \Description
        Sum up packet lost/sent statistics  with a single set of values

    \Input *pPackLostSentOut         - [out] buffer to update with new packet loss sent info
    \Input *pPackLostSentIn          - source buffer (or NULL to init out)

    \Version 08/16/2019 (tcho)
*/
/********************************************************************************F*/
void GameServerPacketUpdatePackLostSentStatSingle(GameServerPacketLostSentT *pPackLostSentOut, const GameServerPacketLostSentT *pPackLostSentIn)
{
    if (pPackLostSentIn != NULL)
    {
        pPackLostSentOut->uLpackSent += pPackLostSentIn->uLpackSent;
        pPackLostSentOut->uRpackSent += pPackLostSentIn->uRpackSent;
        pPackLostSentOut->uRpackLost += pPackLostSentIn->uRpackLost;
        pPackLostSentOut->uLpackLost += pPackLostSentIn->uLpackLost;
    }
    else
    {
        ds_memclr(pPackLostSentOut, sizeof(GameServerPacketLostSentT));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketUpdateLateBin

    \Description
        Update latency bin info with a single latency value

    \Input *pLateBinCfg     - latency bin config
    \Input *pLateBinOut     - [out] buffer to update with new latency bin info
    \Input uLateIn          - input latency value.

    \Note
        The latency bin config is assumed to be terminated with UINT_MAX to
        simplify the code

    \Version 06/14/2011 (jbrookes)
*/
/********************************************************************************F*/
void GameServerPacketUpdateLateBin(const GameServerLateBinT *pLateBinCfg, GameServerLateBinT *pLateBinOut, uint32_t uLateIn)
{
    uint32_t uCount;
    for (uCount = 0; uCount < GAMESERVER_MAXLATEBINS; uCount += 1)
    {
        if (uLateIn <= pLateBinCfg->aBins[uCount])
        {
            pLateBinOut->aBins[uCount] += 1;
            break;
        }
    }
 }

/*F********************************************************************************/
/*!
    \Function GameServerPacketUpdateLateStats

    \Description
        Update latency bin statistics.

    \Input *pLateBinOut - [out] buffer to update with new latency info
    \Input *pLateBinIn  - source buffer (or NULL to init out)

    \Output
        None.

    \Version 06/15/2011 (jbrookes)
*/
/********************************************************************************F*/
void GameServerPacketUpdateLateBinStats(GameServerLateBinT *pLateBinOut, const GameServerLateBinT *pLateBinIn)
{
    if (pLateBinIn != NULL)
    {
        uint32_t uCount;
        for (uCount = 0; uCount < GAMESERVER_MAXLATEBINS; uCount += 1)
        {
            pLateBinOut->aBins[uCount] += pLateBinIn->aBins[uCount];
        }
    }
    else
    {
        // init latency stats
        ds_memclr(pLateBinOut, sizeof(*pLateBinOut));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerPacketReadLateBinConfig

    \Description
        Read latency bin config from config file

    \Input *pLateBinOut     - [out] buffer to update with config info

    \Note
        The latency bucket config is assumed to be terminated with UINT_MAX to
        simplify the code

    \Version 06/14/2011 (jbrookes)
*/
/********************************************************************************F*/
void GameServerPacketReadLateBinConfig(GameServerLateBinT *pLateBinCfg, const char *pCommandTags, const char *pConfigTags, const char *pTagName)
{
    const char *pBinCfg;
    uint32_t uCount = 0;

    if ((pBinCfg = DirtyCastConfigFind(pCommandTags, pConfigTags, pTagName)) != NULL)
    {
        char strBinVal[6];
        for ( ; uCount < (GAMESERVER_MAXLATEBINS-1); uCount += 1)
        {
            if (TagFieldGetDelim(pBinCfg, strBinVal, sizeof(strBinVal), "", uCount, ',') == 0)
            {
                break;
            }
            pLateBinCfg->aBins[uCount] = atoi(strBinVal);
        }
    }
    else
    {
        DirtyCastLogPrintf("gameserverpkt: no late bin config found\n");
    }

    // terminte bin list
    pLateBinCfg->aBins[uCount] = 0xffff;
 }

/*F********************************************************************************/
/*!
    \Function GameServerPacketDebug

    \Description
        Debug output of packet contents.

    \Input *pPacket     - packet to display contents of
    \Input iPacketSize  - total size of packet
    \Input uDebugLevel  - debug level to control verbosity
    \Input *pStr1       - header text for packet log
    \Input *pStr2       - header text for memdump

    \Version 04/08/2009 (jbrookes)
*/
/********************************************************************************F*/
void GameServerPacketDebug(const GameServerPacketT *pPacket, int32_t iPacketSize, uint32_t uDebugLevel, const char *pStr1, const char *pStr2)
{
    // calculate payload size
    int32_t iPayloadSize = iPacketSize - sizeof(pPacket->Header);

    NetPrintfVerbose((uDebugLevel, 2, "%s '%c%c%c%c' packet (%d bytes)\n", pStr1,
        (pPacket->Header.uKind >> 24) & 0xff, (pPacket->Header.uKind >> 16) & 0xff,
        (pPacket->Header.uKind >>  8) & 0xff, (pPacket->Header.uKind & 0xff),
         iPacketSize));
    if ((iPayloadSize > 0) && (uDebugLevel > 3))
    {
        if ((iPayloadSize > 64) && (uDebugLevel < 5))
        {
            iPayloadSize = 64;
        }
        NetPrintMem(&pPacket->Packet.aData, iPayloadSize, pStr2);
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerUpdateGameStats

    \Description
        Update sum and max stats

    \Input *pGameStats      - current stat data
    \Input *pGameStatsCur   - storage for current stat data
    \Input *pGameStatsSum   - stat accumulation buffer
    \Input *pGameStatsMax   - max stat buffer
    \Input bDivide1K        - should we divide stat numbers by 1024

    \Version 03/16/2007 (jbrookes)
*/
/********************************************************************************F*/
void GameServerUpdateGameStats(GameServerStatsT *pGameStats, GameServerStatsT *pGameStatCur, GameServerStatsT *pGameStatSum, GameServerStatsT *pGameStatMax, uint8_t bDivide1K)
{
    // save current stats
    ds_memcpy(pGameStatCur, pGameStats, sizeof(*pGameStatCur));
    // update max stats
    _GameServerUpdateGameStatsMax(pGameStatMax, pGameStats);

    // divide bandwidth numbers by 1024 so we don't overflow our 32bit integers when under heavy load
    if (bDivide1K == TRUE)
    {
        pGameStats->GameStat.uByteRecv /= 1024;
        pGameStats->GameStat.uByteSent /= 1024;
        pGameStats->VoipStat.uByteRecv /= 1024;
        pGameStats->VoipStat.uByteSent /= 1024;
    }

    // update sum stats
    _GameServerUpdateGameStatsSum(pGameStatSum, pGameStats);
    // clear stat structure
    ds_memclr(pGameStats, sizeof(*pGameStats));
}
