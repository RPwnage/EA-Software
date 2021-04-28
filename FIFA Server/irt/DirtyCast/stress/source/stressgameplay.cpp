/*H********************************************************************************/
/*!
    \File stressgameplay.cpp

    \Description
        Dirtycast stress client game play code

    \Copyright
        Copyright (c) 2009 Electronic Arts Inc.

    \Version 07/08/2009 (mclouatre) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/
#include <EABase/eabase.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DirtySDK/dirtyvers.h"
#include "DirtySDK/dirtysock.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"

#include "dirtycast.h"
#include "stress.h"
#include "stressgameplay.h"
#include "stressgameplaymetrics.h"

/*** Defines **********************************************************************/

#define STRESSGAME_MEMID ('gsgm')
#define STRESSGAMEPLAY_DEBUGPOLL (FALSE)


/*** Type Definitions ************************************************************/

/*** Variables ********************************************************************/

bool StressGamePlayC::m_bGameFailed = false;

SocketSendCallbackEntryT _Stress_aSendCbEntries[SOCKET_MAXSENDCALLBACKS];

static uint32_t _Stress_aExclusionList[32] = { 0 };

static uint8_t _Stress_bCallbackInstalled = FALSE;


/*** Private Functions ************************************************************/


/*F*************************************************************************************/
/*!
    \Function _StressGamePlayExcludeAddrCheck

    \Description
        Check to see if an address is in the exclusion list

    \Input uAddress     - address to check

    \Output
        uint32_t        - 1=in list, 0=not in list

    \Version 05/20/2010 (jbrookes)
*/
/**************************************************************************************F*/
static int32_t _StressGamePlayExcludeAddrCheck(uint32_t uAddress)
{
    uint32_t uList;
    for (uList = 0; uList < sizeof(_Stress_aExclusionList)/sizeof(_Stress_aExclusionList[0]); uList += 1)
    {
        if (_Stress_aExclusionList[uList] == uAddress)
        {
            return(1);
        }
    }
    return(0);
}

/*F*************************************************************************************/
/*!
    \Function _StressSendCallback

    \Description
        Global send callback used to snuff p2p traffic.

    \Input *pSocket     - pointer to socket being sent on
    \Input iType        - type of socket
    \Input *pData       - pointer to data being sent
    \Input iDataSize    - size of data being sent
    \Input *pTo         - send destination
    \Input *pCallref    - user callback data


    \Version 05/20/2010 (jbrookes)
*/
/**************************************************************************************F*/
static int32_t _StressSendCallback(SocketT *pSocket, int32_t iType, const uint8_t *pData, int32_t iDataSize, const struct sockaddr *pTo, void *pCallref)
{
    // only handle dgram sockets
    if ((iType == SOCK_DGRAM) && _StressGamePlayExcludeAddrCheck(SockaddrInGetAddr(pTo)))
    {
        // swallow packet intended for peer
        NetPrintf(("stress: eating packet send request (%d bytes on port %d)\n", iDataSize, SockaddrInGetPort(pTo)));
        return(iDataSize);
    }

    // forward to previously-installed send callback, if any
    return(SocketSendCallbackInvoke(&_Stress_aSendCbEntries[0], pSocket, iType, (const char *)pData, iDataSize, pTo));
}


/*F*************************************************************************************/
/*!
    \Function _StressInstallSendCallback

    \Description
        Install a global send callback at the socket layer.  We use this send callback
        to "snuff" data sent directly to a peer in p2p modes, to simulate a peer connection
        failure and force fallback through the rebroadcaster.

    \Input
        None.

    \Version 05/20/2010 (jbrookes)
*/
/**************************************************************************************F*/
static void _StressInstallSendCallback(void)
{
    if (!_Stress_bCallbackInstalled)
    {
        int32_t iCallbackIndex;

        for (iCallbackIndex = 0; iCallbackIndex < SOCKET_MAXSENDCALLBACKS; iCallbackIndex++)
        {
            // get callback entry
            NetConnStatus('sdcf', iCallbackIndex, &_Stress_aSendCbEntries[iCallbackIndex].pSendCallback, sizeof(_Stress_aSendCbEntries[iCallbackIndex].pSendCallback));
            NetConnStatus('sdcu', iCallbackIndex, &_Stress_aSendCbEntries[iCallbackIndex].pSendCallref, sizeof(_Stress_aSendCbEntries[iCallbackIndex].pSendCallref));

            if (_Stress_aSendCbEntries[iCallbackIndex].pSendCallback != NULL)
            {
                NetPrintf(("stressgameplay: replacing send callback (prevfunc=%p, prevdata=%p)\n", _Stress_aSendCbEntries[iCallbackIndex].pSendCallback, _Stress_aSendCbEntries[iCallbackIndex].pSendCallref));
                NetConnControl('sdcb', FALSE, 0, (void*)_Stress_aSendCbEntries[iCallbackIndex].pSendCallback, _Stress_aSendCbEntries[iCallbackIndex].pSendCallref);
            }
        }

        // install our send callback
        NetConnControl('sdcb', TRUE, 0, (void *)_StressSendCallback, NULL);

        // remember we are installed
        _Stress_bCallbackInstalled = TRUE;
    }
}

/*F*************************************************************************************/
/*!
    \Function _StressUninstallSendCallback

    \Description
        Uninstall a global send callback at the socket layer.

    \Notes
        When need to ensure that the send callback is properly cleaned up when we are
        finished. Otherwise, we might end up accessing invalid memory for send callbacks
        that get removed.

    \Version 12/14/2018 (eesponda)
*/
/**************************************************************************************F*/
static void _StressUninstallSendCallback(void)
{
    int32_t iCallbackIndex;

    if (!_Stress_bCallbackInstalled)
    {
        return;
    }

    // uninstall our send callback
    NetConnControl('sdcb', FALSE, 0, (void *)_StressSendCallback, NULL);

    // reinstall previous callbacks
    for (iCallbackIndex = 0; iCallbackIndex < SOCKET_MAXSENDCALLBACKS; iCallbackIndex++)
    {
        if (_Stress_aSendCbEntries[iCallbackIndex].pSendCallback != NULL)
        {
            NetPrintf(("stressgameplay: reinstalling send callback (prevfunc=%p, prevdata=%p)\n", _Stress_aSendCbEntries[iCallbackIndex].pSendCallback, _Stress_aSendCbEntries[iCallbackIndex].pSendCallref));
            NetConnControl('sdcb', TRUE, 0, (void*)_Stress_aSendCbEntries[iCallbackIndex].pSendCallback, _Stress_aSendCbEntries[iCallbackIndex].pSendCallref);
        }
    }

    // clear the callback entries
    ds_memclr(_Stress_aSendCbEntries, sizeof(_Stress_aSendCbEntries));

    // we have successfully uninstalled callback
    _Stress_bCallbackInstalled = FALSE;
}

/*F*************************************************************************************/
/*!
    \Function _StressOpenCsv

    \Description
        Opens our csv for writing and will attempt to write a header the first time

    \Input *pFilename   - filename (without extension)

    \Output
        ZFileT          - identifier for file or ZFILE_INVALID on failure

    \Version 04/18/2019 (eesponda)
*/
/**************************************************************************************F*/
static ZFileT _StressOpenCsv(const char *pFilename)
{
    ZFileT iZFile;
    char strBuffer[256];
    bool bWriteHeader = false;
    ZFileStatT FileStat;

    ds_snzprintf(strBuffer, sizeof(strBuffer), "%s.csv", pFilename);
    // check if the file exists to know if we need to write our header
    if (ZFileStat(strBuffer, &FileStat) != ZFILE_ERROR_NONE)
    {
        bWriteHeader = true;
    }
    // open the file for writing adding the header if necessary
    if ((iZFile = ZFileOpen(strBuffer, ZFILE_OPENFLAG_APPEND | ZFILE_OPENFLAG_WRONLY)) == ZFILE_INVALID)
    {
        StressPrintf("stressgameplay: could not open csv to write stats\n");
        return(iZFile);
    }
    if (bWriteHeader)
    {
        int32_t iOffset = ds_snzprintf(strBuffer, sizeof(strBuffer), "key,persona,game,seqn,timestamp,epoch,late,outbps,outpps,inbps,inpps,lnaksent,lpacklost,lpacksaved,rnaksent,rpacklost,lslen,rslen,plat,proc,minproc,maxproc,isent,ircvd,drop,ique,oque\r\n");
        ZFileWrite(iZFile, strBuffer, iOffset);
    }
    return(iZFile);
}

/*F*************************************************************************************/
/*!
    \Function _StressWriteCsv

    \Description
        Writes our stats to the csv file

    \Input iZFileId     - file identifier
    \Input uGameId      - game identifier
    \Input iClientIndex - index into clientlist (doesn't include server)
    \Input *pPersona    - persona for the local player
    \Input *pLinkStat   - link level stats
    \Input *pDistStat   - dist level stats (can be NULL if dist is disabled)

    \Version 04/18/2019 (eesponda)
*/
/**************************************************************************************F*/
static void _StressWriteCsv(ZFileT iZFile, uint64_t uGameId, int32_t iClientIndex, const char *pPersona, const GameLinkStatT *pLinkStat, const GameDistStatT *pDistStat)
{
    char strWriteBuf[512], strDateBuf[32];
    int32_t iOffset;
    struct tm CurTime;
    uint64_t uEpoch;

    // if file isn't open don't do anything
    if (iZFile == ZFILE_INVALID)
    {
        return;
    }

    // write the record, added empty row for key and seqn as we fill them in during processing
    ds_timetostr(ds_secstotime(&CurTime, uEpoch = ds_timeinsecs()), TIMETOSTRING_CONVERSION_ISO_8601, FALSE, strDateBuf, sizeof(strDateBuf));
    iOffset = ds_snzprintf(strWriteBuf, sizeof(strWriteBuf), ",%s,%llu,,%s,%llu,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%d,%u,%u,%u,%u,%u,%u,%d,%d\r\n", pPersona,
        uGameId, strDateBuf, uEpoch, pLinkStat->Stats.late, pLinkStat->Stats.outbps, pLinkStat->Stats.outpps, pLinkStat->Stats.inbps, pLinkStat->Stats.inpps,
        pLinkStat->Stats.lnaksent - pLinkStat->iLNakSent, pLinkStat->Stats.lpacklost - pLinkStat->iLPackLost, pLinkStat->Stats.lpacksaved - pLinkStat->iLPackSaved,
        pLinkStat->Stats.rnaksent - pLinkStat->iRNakSent, pLinkStat->Stats.rpacklost - pLinkStat->iRPackLost, pLinkStat->iSlen, 
        (pDistStat != NULL) ? pDistStat->Stats[iClientIndex].slen : 0,
        (pDistStat != NULL) ? pDistStat->iPlat : 0, 
        (pDistStat != NULL) ? (pDistStat->uTotalRtt / pDistStat->uNumInputs) : 0, 
        (pDistStat != NULL) ? pDistStat->uMinRtt : 0,
        (pDistStat != NULL) ? pDistStat->uMaxRtt : 0,
        (pDistStat != NULL) ? (pDistStat->uSendIndex - pDistStat->uLastSendIndex) :0,
        (pDistStat != NULL) ? (pDistStat->uRecvIndex - pDistStat->uLastRecvIndex) : 0, 
        (pDistStat != NULL) ? pDistStat->uDroppedInputs : 0, 
        (pDistStat != NULL) ? pDistStat->iInputQueue : 0, 
        (pDistStat != NULL) ? pDistStat->iOutputQueue : 0);
    ZFileWrite(iZFile, strWriteBuf, iOffset);
}

/*F*************************************************************************************/
/*!
    \Function _StressPrepareMetrics

    \Description
        Writes our stats to oiv2

    \Input uGameId      - game identifier
    \Input iClientIndex - index into clientlist (doesn't include server)
    \Input *pLinkStat   - link level stats
    \Input *pDistStat   - dist level stats (can be NULL if dist is disabled)

    \Version 07/12/2019 (cvienneau)
*/
/**************************************************************************************F*/
static void _StressPrepareMetrics(StressGameplayMetricsRefT *pStressGameplayMetrics, uint64_t uGameId, int32_t iClientIndex, const GameLinkStatT *pLinkStat, const GameDistStatT *pDistStat)
{
    if (pStressGameplayMetrics != NULL)
    {
        StressGameplayMetricDataT data;

        // gauges
        data.Gauge.uLatencyMs = pLinkStat->Stats.late;      //!< end to end latency in ms

        data.Gauge.uSentBps = pLinkStat->Stats.outbps;
        data.Gauge.uReceivedBps = pLinkStat->Stats.inbps;
        data.Gauge.uSentPps = pLinkStat->Stats.outpps;
        data.Gauge.uReceivedPps = pLinkStat->Stats.inpps;
        data.Gauge.uSendQueueLength = pLinkStat->iSlen; // from NetGameLinkStatus('slen') selector
        data.Gauge.uLatencyPackets = (pDistStat != NULL) ? pDistStat->iPlat : 0;      //!< end to end latency in packets from NetGameDistStatus('plat')
        data.Gauge.uReceiveQueueLength = (pDistStat != NULL) ? pDistStat->Stats[iClientIndex].slen : 0;
        data.Gauge.uProcessingTimeMaxMs = (pDistStat != NULL) ? pDistStat->uMaxRtt : 0;
        data.Gauge.uProcessingTimeMinMs = (pDistStat != NULL) ? pDistStat->uMinRtt : 0;
        data.Gauge.uProcessingTimeAvgMs = (pDistStat != NULL) ? (pDistStat->uTotalRtt / pDistStat->uNumInputs) : 0;
        data.Gauge.uSentInputs = (pDistStat != NULL) ? pDistStat->uSendIndex - pDistStat->uLastSendIndex : 0;
        data.Gauge.uReceivedInputs = (pDistStat != NULL) ? pDistStat->uRecvIndex - pDistStat->uLastRecvIndex : 0;
        data.Gauge.uInputsSentQueued = (pDistStat != NULL) ? pDistStat->iOutputQueue : 0;
        data.Gauge.uInputsReceivedQueued = (pDistStat != NULL) ? pDistStat->iInputQueue : 0;

        // counters
        data.Count.uSentBytes = pLinkStat->Stats.sent;
        data.Count.uReceivedBytes = pLinkStat->Stats.rcvd;
        data.Count.uSentPackets = pLinkStat->Stats.lpacksent_now;
        data.Count.uReceivedPackets = pLinkStat->Stats.lpackrcvd;
        data.Count.uSentNak = pLinkStat->Stats.lnaksent;
        data.Count.uReceivedNak = pLinkStat->Stats.rnaksent;
        data.Count.uReceivedPacketsLost = pLinkStat->Stats.lpacklost;
        data.Count.uReceivedPacketsSaved = pLinkStat->Stats.lpacksaved;
        data.Count.uSentPacketsLost = pLinkStat->Stats.rpacklost;
        data.Count.uDroppedInputs = (pDistStat != NULL) ? pDistStat->uDroppedInputs : 0;

        // give the values to the metrics module
        StressGameplayMetricData(pStressGameplayMetrics, &data);
    }
}

/*F*************************************************************************************/
/*!
    \Function _StressHandlePairedInput

    \Description
        Updates stats when our input gets paired by the server and returned to us

    \Input *pDistStat   - stats to update
    \Input *pInput      - input that was paired

    \Version 04/18/2019 (eesponda)
*/
/**************************************************************************************F*/
static void _StressHandlePairedInput(GameDistStatT *pDistStat, GameInputT *pInput)
{
    uint32_t uDiff;

    // calculate how long it took for our package to get back
    pInput->uIndex = SocketNtohl(pInput->uIndex);
    uint32_t uRtt = NetTickDiff(NetTick(), pDistStat->aFrameTicks[pInput->uIndex % DIRTYCAST_CalculateArraySize(pDistStat->aFrameTicks)]);

    // add to totals for avg rtt calculation
    pDistStat->uTotalRtt += uRtt;
    pDistStat->uNumInputs += 1;

    // adjust min rtt
    if (pDistStat->uMinRtt == 0)
    {
        pDistStat->uMinRtt = uRtt;
    }
    else if (uRtt < pDistStat->uMinRtt)
    {
        pDistStat->uMinRtt = uRtt;
    }

    // adjust max rtt
    if (uRtt > pDistStat->uMaxRtt)
    {
        pDistStat->uMaxRtt = uRtt;
    }

    // check for dropped inputs
    if ((uDiff = pInput->uIndex - pDistStat->uNextSeq) > 0)
    {
        NetPrintf(("stressgameplay: dropped input detected, expected %u got %u (drop = %u)\n", pDistStat->uNextSeq, pInput->uIndex, uDiff));
        pDistStat->uDroppedInputs += uDiff;
    }
    pDistStat->uNextSeq = pInput->uIndex + 1;
}

/*F*************************************************************************************/
/*!
    \Function StressGamePlayC::GamePlayFailure

    \Description
        Flag a game play failure

    \Input
        None.

    \Version 07/09/2009 (mclouatre) initial version
*/
/**************************************************************************************F*/
void StressGamePlayC::GamePlayFailure()
{
    StressPrintf("stressgameplay: game play interrupted because of critical failure.\n");
    m_bGameFailed = true;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::GameStatsUpdate

    \Description
        Update stats on receive, print a summary once a second.

    \Input iGameConn    - index of game connection
    \Input iPacketLen   - packet length
    \Input uCurTick     - current millisecond tick count

    \Version 08/07/2009 (mclouatre) Initial version - same pattern as function written by JBrookes for VoipTunnel
*/
/********************************************************************************F*/
void StressGamePlayC::GameStatsUpdate(NetGameLinkRefT *pGameLink, GameLinkStatT *pLinkStat, int32_t iGameConn, int32_t iPacketLen, uint32_t uCurTick)
{
    int32_t iIndex;

    // don't update if there is no ref
    if (pGameLink == NULL)
    {
        return;
    }

    // init stat tracker?
    if (pLinkStat->iStatTick == 0)
    {
        pLinkStat->iStatTick = (signed)uCurTick;
    }

    // update current recv size and total recv size
    pLinkStat->iRecvSize += iPacketLen;
    pLinkStat->iTotalSize += iPacketLen;

    // calculate the current input/output queue lengths if using dist
    if (m_pNetGameDist != NULL)
    {
        int32_t iInputQueue, iOutputQueue;

        if ((iInputQueue = GMDIST_Modulo(NetGameDistStatus(m_pNetGameDist, '?inp', 0, NULL, 0) - NetGameDistStatus(m_pNetGameDist, '?cmp', 0, NULL, 0), NetGameDistStatus(m_pNetGameDist, 'pwin', 0, NULL, 0))) > m_DistStat.iInputQueue)
        {
            m_DistStat.iInputQueue = iInputQueue;
        }
        if ((iOutputQueue = GMDIST_Modulo(NetGameDistStatus(m_pNetGameDist, '?out', 0, NULL, 0) - NetGameDistStatus(m_pNetGameDist, '?snd', 0, NULL, 0), NetGameDistStatus(m_pNetGameDist, 'pwin', 0, NULL, 0))) > m_DistStat.iOutputQueue)
        {
            m_DistStat.iOutputQueue = iOutputQueue;
        }
    }

    // periodically output throughput
    if (NetTickDiff(uCurTick, pLinkStat->iStatTick) > (m_iLateRate * 1000))
    {
        NetGameLinkStatus(pGameLink, 'stat', 0, &pLinkStat->Stats, sizeof(NetGameLinkStatT));
        pLinkStat->iSlen = NetGameLinkStatus(pGameLink, 'slen', 0, NULL, 0);

        if (m_pNetGameDist != NULL)
        {
            NetGameDistStatus(m_pNetGameDist, 'mult', 0, &m_DistStat.Stats, sizeof(m_DistStat.Stats));
            m_DistStat.iPlat = NetGameDistStatus(m_pNetGameDist, 'plat', 0, NULL, 0);
        }

        StressPrintf("stressgameplay: [%02d] %2.2f k/s (%4dk) late=%3d pps/rpps=%d/%d bps/rps/nps=%d/%d/%d over=%d/%d pct=%d%%/%d%%\n", iGameConn,
            (float)pLinkStat->iRecvSize/(1024.0f*m_iLateRate), pLinkStat->iTotalSize/1024, pLinkStat->Stats.late,
            pLinkStat->Stats.outpps, pLinkStat->Stats.outrpps,
            pLinkStat->Stats.outbps, pLinkStat->Stats.outrps, pLinkStat->Stats.outnps,
            pLinkStat->Stats.outrps - pLinkStat->Stats.outbps, pLinkStat->Stats.outnps - pLinkStat->Stats.outbps,
            pLinkStat->Stats.outbps ? ((pLinkStat->Stats.outrps - pLinkStat->Stats.outbps) * 100) / pLinkStat->Stats.outbps : 0,
            pLinkStat->Stats.outbps ? ((pLinkStat->Stats.outnps - pLinkStat->Stats.outbps) * 100) / pLinkStat->Stats.outbps : 0);

        // write the stats to csv
        _StressWriteCsv(m_iZFile, m_pGame->getId(), m_iClientId, m_strPersona, pLinkStat, ((m_pNetGameDist != NULL) ? &m_DistStat : NULL));

        // provide the metrics to the metrics reporting system
        _StressPrepareMetrics(m_pStressGameplayMetrics, m_pGame->getId(), m_iClientId, pLinkStat, ((m_pNetGameDist != NULL) ? &m_DistStat : NULL));

        // save current for next calculation
        pLinkStat->iLNakSent = pLinkStat->Stats.lnaksent;
        pLinkStat->iLPackLost = pLinkStat->Stats.lpacklost;
        pLinkStat->iLPackSaved = pLinkStat->Stats.lpacksaved;
        pLinkStat->iRNakSent = pLinkStat->Stats.rnaksent;
        pLinkStat->iRPackLost = pLinkStat->Stats.rpacklost;
        pLinkStat->iRecvSize = 0;
        pLinkStat->iStatTick = (signed)uCurTick;
        if (m_pNetGameDist != NULL)
        {
            m_DistStat.uLastSendIndex = m_DistStat.uSendIndex;
            m_DistStat.uLastRecvIndex = m_DistStat.uRecvIndex;
            m_DistStat.uTotalRtt = 0;
            m_DistStat.uMinRtt = 0;
            m_DistStat.uMaxRtt = 0;
            m_DistStat.uNumInputs = 0;
            m_DistStat.uDroppedInputs = 0;
            m_DistStat.iInputQueue = 0;
            m_DistStat.iOutputQueue = 0;

            for (iIndex = 0; iIndex < m_iNumberOfPlayers; iIndex++)
            {
                NetGameDistStatT *pDistStat = &m_DistStat.Stats[iIndex];

                StressPrintf("stressgameplay: (%d) slen %d, late %d, bps %d, pps %d, plost %d, naksent %d\n", iIndex,
                    pDistStat->slen, pDistStat->late, pDistStat->bps, pDistStat->pps,
                    pDistStat->plost, pDistStat->naksent);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::LinkSend

    \Description
        Try and send data to player identified by iGameConn

    \Input iGameConn    - index of game connection
    \Input uCurTick     - current millisecond tick count

    \Output
        int32_t         - TRUE if we sent, else FALSE

    \Version 08/07/2009 (mclouatre) Initial version - same pattern as function written by JBrookes for VoipTunnel
*/
/********************************************************************************F*/
int32_t StressGamePlayC::LinkSend(NetGameLinkRefT *pGameLink, GameLinkStatT *pLinkStat, int32_t iGameConn, uint32_t uCurTick)
{
    NetGameMaxPacketT MaxPacket;
    int32_t iResult = 0, iSendResult;

    // set up a packet to send
    ds_memclr(&MaxPacket, sizeof(MaxPacket));
    MaxPacket.head.kind = GAME_PACKET_USER;
    MaxPacket.head.len = m_uPacketSize;
    ds_snzprintf((char *)MaxPacket.body.data, sizeof(MaxPacket.body.data), "Game Test %d Byte Packet %d", m_uPacketSize, pLinkStat->iSendSeqn);
    
    // send packet
    if ((iSendResult = NetGameLinkSend(pGameLink, (NetGamePacketT *)&MaxPacket, 1)) > 0)
    {
        // update send sequence and last send time
        pLinkStat->iSendSeqn += 1;

        iResult = 1;
    }
    else if (iSendResult < 0)
    {
        StressPrintf("stressgameplay: unable to send (error=%d) -- backing off\n", iSendResult);
    }
    else
    {
        StressPrintf("stressgameplay: unable to send (error=overflow) -- backing off\n");
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::LinkRecv

    \Description
        Try and receive data from a player.

    \Input iGameConn    - index of game connection
    \Input uCurTick     - current millisecond tick count

    \Version 08/07/2009 (mclouatre) Initial version - same patttern as function written by JBrookes for VoipTunnel
*/
/********************************************************************************F*/
void StressGamePlayC::LinkRecv(NetGameLinkRefT *pGameLink, GameLinkStatT *pLinkStat, int32_t iGameConn, uint32_t uCurTick)
{
    NetGameMaxPacketT MaxPacket;
    int32_t iSeqn, iSize;

    // check for receive
    while (NetGameLinkRecv(pGameLink, (NetGamePacketT *)&MaxPacket, 1, FALSE) > 0)
    {
        if (sscanf((const char *)MaxPacket.body.data, "%*s %*s %d %*s %*s %d", &iSize, &iSeqn) > 0)
        {
            pLinkStat->iRecvSeqn = iSeqn;
            if (iSize != MaxPacket.head.len)
            {
                StressPrintf("stressgameplay: warning -- packet was %d bytes but received as %d bytes\n", iSize, MaxPacket.head.len);
            }
        }
        else
        {
            StressPrintf("stressgameplay: failed to get sequence\n");
        }

        // update stats
        GameStatsUpdate(pGameLink, pLinkStat, iGameConn, MaxPacket.head.len, uCurTick);
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::DistSend

    \Description
        Send simulated controller input.

    \Input iGameConn    - index of game connection
    \Input uCurTick     - current millisecond tick count

    \Version 08/07/2009 (mclouatre) Initial version - same patttern as function written by JRainy for VoipTunnel
*/
/********************************************************************************F*/
void StressGamePlayC::DistSend(int32_t iGameConn, uint32_t uCurTick)
{
    GameInputT Input;
    void *pInputAddress = &Input;
    int32_t iLen, iResult, iSend;
    char *pBuffer;
    uint8_t uType;

    // check to make sure it is time to send, to make sure the game has started and the flow control updates have been received
    NetGameDistInputCheck(m_pNetGameDist, &iSend, NULL);
    if (iSend != 0)
    {
        StressPrintf("stressgameplay: netgamedist said it is not time send, remain = %d\n", iSend);
        return;
    }

    ds_memclr(&Input, sizeof(Input));
    Input.uIndex = SocketHtonl(m_DistStat.uSendIndex);

    // with the index (seq) reserved in the packet, fill in the remaining bytes with random data
    for (pBuffer = Input.aBuffer, iLen = (signed)sizeof(Input.uIndex); iLen < (signed)m_uPacketSize; iLen += 1)
    {
        *pBuffer++ = (rand() % 64) + ' ';
    }

    // make 1 out of 10 packets undroppable
    uType = ((m_DistStat.uSendIndex % 10) != 0) ? GMDIST_DATA_INPUT_DROPPABLE : GMDIST_DATA_INPUT;

    // Send newly constructed packet to OTP server for dist processing
    if ((iResult = NetGameDistInputLocalMulti(m_pNetGameDist, &uType, &pInputAddress, &iLen, 1)) > 0)
    {
        NetPrintfVerbose((m_iVerbosity, 1, "stressgameplay: sending packet %d at %d length=%d\n", Input.uIndex, uCurTick, m_uPacketSize));

        m_DistStat.aFrameTicks[m_DistStat.uSendIndex % DIRTYCAST_CalculateArraySize(m_DistStat.aFrameTicks)] = NetTick();
        m_DistStat.uSendIndex++;
    }
    else
    {
        StressPrintf("stressgameplay: failure sending dist packet %d at %d. With failure %d\n", Input.uIndex, uCurTick, iResult);
        GamePlayFailure();
    }
}


/*F********************************************************************************/
/*!
    \Function StressGamePlayC::DistRecv

    \Description
        Receive simulated controller input.

    \Input iGameConn    - index of game connection
    \Input uCurTick     - current millisecond tick count

    \Version 08/07/2009 (mclouatre) Initial version - same patttern as function written by JRainy for VoipTunnel
*/
/********************************************************************************F*/
void StressGamePlayC::DistRecv(int32_t iGameConn, uint32_t uCurTick)
{
    GameInputT aInputs[STRESS_MAX_PLAYERS];
    int32_t aSizes[STRESS_MAX_PLAYERS];
    int32_t iIndex, iRet, iSize = 0;
    void *aInputPtrs[STRESS_MAX_PLAYERS];
    uint8_t aDataTypes[STRESS_MAX_PLAYERS];

    for (iIndex = 0; iIndex < STRESS_MAX_PLAYERS; iIndex++)
    {
        aInputPtrs[iIndex] = &aInputs[iIndex];
    }

    while ((iRet = NetGameDistInputQueryMulti(m_pNetGameDist, aDataTypes, aInputPtrs, aSizes)) > 0)
    {
        #if DIRTYCODE_LOGGING
        static int32_t previous=0;
        int32_t now = NetTick();
        NetPrintfVerbose((m_iVerbosity, 1, "stressgameplay: dist packet received at %d (%d) ---- dumping player-specific data received\n", now, now-previous));
        previous = now;
        #endif

        // if we paired one of our inputs successfully, report on metrics on it
        if ((aDataTypes[m_iClientId] == GMDIST_DATA_INPUT) || (aDataTypes[m_iClientId] == GMDIST_DATA_INPUT_DROPPABLE))
        {
            _StressHandlePairedInput(&m_DistStat, &aInputs[m_iClientId]);
        }

        for (iIndex = 0; iIndex < STRESS_MAX_PLAYERS; iIndex++)
        {
            if (aDataTypes[iIndex] == GMDIST_DATA_NONE)
            {
                NetPrintfVerbose((m_iVerbosity, 1, "    (player %d) --> NONE\n", iIndex));
            }
            else if (aDataTypes[iIndex] == GMDIST_DATA_DISCONNECT)
            {
                NetPrintfVerbose((m_iVerbosity, 1, "    (player %d) --> DISC\n", iIndex));
            }
            else if (aDataTypes[iIndex] == GMDIST_DATA_NODATA)
            {
                NetPrintfVerbose((m_iVerbosity, 1, "    (player %d) --> NODATA\n", iIndex));
            }
            else if ((aDataTypes[iIndex] == GMDIST_DATA_INPUT) || (aDataTypes[iIndex] == GMDIST_DATA_INPUT_DROPPABLE))
            {
                NetPrintfVerbose((m_iVerbosity, 1, "    (player %d) --> frame=%d size=%d type=%d\n", iIndex, SocketNtohl(aInputs[iIndex].uIndex), aSizes[iIndex], aDataTypes[iIndex]));
                iSize += aSizes[iIndex];
            }
        }

        m_DistStat.uRecvIndex++;

        // update stats
        GameStatsUpdate(m_pNetGameLink[0], &m_LinkStat[0], iGameConn, iSize, uCurTick);
    }

    if ((iRet < 0) || (NetGameDistGetError(m_pNetGameDist)))
    {
        StressPrintf("stressgameplay: DistRecv() failure\n");
        GamePlayFailure();
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetupNetGameDist

    \Description
        Create dist ref and dist stream for use by client.

    \Output
        int32_t             - negative=error, else success

    \Version 09/07/2009 (mclouatre)  Initial version
*/
/********************************************************************************F*/
int32_t StressGamePlayC::SetupNetGameDist()
{
    if (m_pNetGameLink[0] == NULL)
    {
        StressPrintf("stressgameplay: can't set up NetGameDist with NULL NetGameLink ref\n");
        return(-1);
    }
    StressPrintf("stressgameplay: setup game's NetGameDist\n");

    ConnApiRefT* pConnApi = m_pConnApiAdapter->getConnApiRefT(m_pGame);

    // create the dist ref
    m_pNetGameDist = NetGameDistCreate(m_pNetGameLink[0],
        (NetGameDistStatProc *)NetGameLinkStatus,
        (NetGameDistSendProc *)NetGameLinkSend,
        (NetGameDistRecvProc *)NetGameLinkRecv,
        GMDIST_DEFAULT_BUFFERSIZE_IN * 10,
        GMDIST_DEFAULT_BUFFERSIZE_OUT * 20 );

    if (m_pNetGameDist == NULL)
    {
        StressPrintf("stressgameplay: unable to create dist ref\n");
        return(-2);
    }

    // propagate verbosity lelel to NetGameDist
    NetGameDistControl(m_pNetGameDist, 'spam', m_iVerbosity, NULL);

    NetGameDistInputRate(m_pNetGameDist, m_iUsecsPerFrame/1000);

    // get local client index in client list and set up for multidist
    m_iClientId = ConnApiStatus(pConnApi, 'self', NULL, 0) - 1;
    NetGameDistMultiSetup(m_pNetGameDist, m_iClientId, STRESS_MAX_PLAYERS);

    StressPrintf("stressgameplay: setup complete\n");

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::TeardownNetGameDist

    \Description
        Destroy dist ref and dist stream for use by client.

    \Version 09/12/2009 (jrainy)  Initial version
*/
/********************************************************************************F*/
void StressGamePlayC::TeardownNetGameDist()
{
    StressPrintf("stressgameplay: teardown game's NetGameDist\n");

    // destroy dist, since it relies on the underlying link
    if (m_pNetGameDist != NULL)
    {
        NetGameDistDestroy(m_pNetGameDist);
        m_pNetGameDist = NULL;
    }

    // just null link ref - connapi has already destroyed it for us
    m_pNetGameLink[0] = NULL;
}

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function StressGamePlayC constructor

    \Description
        Construct a new StressGamePlay instance.

    \Input *pConnApiAdapter    - reference to ConnApiAdapter

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
StressGamePlayC::StressGamePlayC(Blaze::BlazeNetworkAdapter::ConnApiAdapter *pConnApiAdapter, StressGameplayMetricsRefT *pMetricsSystem) :
    m_bUseDist(FALSE),
    m_bPeer2peer(FALSE),
    m_bGameStarted(FALSE),
    m_uPacketSize(0),
    m_uPacketsPerSec(0),
    m_iUsecsPerFrame(0),
    m_iVerbosity(0),
    m_iNumberOfPlayers(0),
    m_iClientId(0),
    m_pConnApiAdapter(pConnApiAdapter),
    m_pGame(NULL),
    m_pNetGameDist(NULL),
    m_uStreamMessageSize(0),
    m_pStressGameplayMetrics(pMetricsSystem),
    m_uGameStreamId(0),
    m_uLastFrameRecv((uint32_t)-1),
    m_iZFile(ZFILE_INVALID),
    m_bWriteCsv(false)
{
    ds_memclr(m_pNetGameLink, sizeof(m_pNetGameLink));
    ds_memclr(m_LinkStat, sizeof(m_LinkStat));
    ds_memclr(&m_DistStat, sizeof(m_DistStat));
    ds_memclr(m_strPersona, sizeof(m_strPersona));
    ds_memclr(m_pNetGameLinkStream, sizeof(m_pNetGameLinkStream));

    srand(time(NULL));
    m_bGameFailed = false;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC destructor

    \Description
        Destroy a StressGamePlay instance.

    \Input None.

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
StressGamePlayC::~StressGamePlayC()
{
    if (m_bUseDist)
    {
        TeardownNetGameDist();
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetGameStarted

    \Description
        Used to inform StressGamePlay object that the game is started

    \Version 07/08/2009 (cadam)
*/
/********************************************************************************F*/
void StressGamePlayC::SetGameStarted()
{
    char *pBuffer;
    int32_t iBufferSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    m_bGameStarted = TRUE;
    
    // open the csv file for writing if this feature is enabled
    if (m_bWriteCsv)
    {
        m_iZFile = _StressOpenCsv(m_strPersona);
    }
 
    if (m_uStreamMessageSize > 0)
    {
        iBufferSize = ( m_uStreamMessageSize < STRESS_MAX_STREAM_MSG_LENGTH ) ? m_uStreamMessageSize : STRESS_MAX_STREAM_MSG_LENGTH;

        DirtyMemGroupQuery(&iMemGroup, NULL);
        pBuffer = (char*)DirtyMemAlloc(iBufferSize, STRESSGAME_MEMID, iMemGroup, &pMemGroupUserData);
        
        for (int32_t iIndex = 0; iIndex < iBufferSize; iIndex++)
        {
            pBuffer[iIndex] = (rand() % 64) + ' ';
        }

        StressPrintf("stressgameplay: sending netgamelink stream messages\n");
        for (int32_t iIndex = 0; iIndex < STRESS_MAX_PLAYERS; iIndex++)
        {
            if (m_pNetGameLinkStream[iIndex] != NULL)
            {
                m_pNetGameLinkStream[iIndex]->Send(m_pNetGameLinkStream[iIndex], 0, 'test', (void*)pBuffer, iBufferSize);
            }
        }
        DirtyMemFree(pBuffer, STRESSGAME_MEMID, iMemGroup, pMemGroupUserData);
    }

    // tell the server we are ready to send / receive
    if (m_pNetGameDist != NULL)
    {
        NetGameDistControl(m_pNetGameDist, 'lsnd', TRUE, NULL);
        NetGameDistControl(m_pNetGameDist, 'lrcv', TRUE, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetGameEnded

    \Description
        Used to inform StressGamePlay object that the game is ended

    \Version 01/29/2013 (jbrookes)
*/
/********************************************************************************F*/
void StressGamePlayC::SetGameEnded()
{
    m_bGameStarted = FALSE;

    // close the file if opened
    if (m_iZFile != ZFILE_INVALID)
    {
        ZFileClose(m_iZFile);
        m_iZFile = ZFILE_INVALID;
    }

    // tell the server we are not ready to send / receive
    if (m_pNetGameDist != NULL)
    {
        NetGameDistControl(m_pNetGameDist, 'lsnd', FALSE, NULL);
        NetGameDistControl(m_pNetGameDist, 'lrcv', FALSE, NULL);
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetPersona

    \Description
        Set the local client's persona name

    \Input *pPersona    - the name we are setting to

    \Version 04/05/2019 (eesponda)

*/
/********************************************************************************F*/
void StressGamePlayC::SetPersona(const char *pPersona)
{
    ds_strnzcpy(m_strPersona, pPersona, sizeof(m_strPersona));
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetPeer2Peer

    \Description
        Set the peer2peer flag

    \Input *bPeer2Peer    - true or false

    \Version 17/07/2019 (dvladu)

*/
/********************************************************************************F*/
void StressGamePlayC::SetPeer2Peer(bool bPeer2Peer)
{
    m_bPeer2peer = bPeer2Peer;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetStreamMessageSize

    \Description
        Set the NetGameLinkStream message size

    \Input *uSize    -  bytes to send

    \Version 17/07/2019 (dvladu)

*/
/********************************************************************************F*/
void StressGamePlayC::SetStreamMessageSize(uint32_t uSize)
{
    m_uStreamMessageSize = uSize;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetWriteCsv

    \Description
        Enable/disable the write stat csv feature

    \Input bWriteCsv    - value we set to enable/disable the feature

    \Version 01/29/2013 (jbrookes)
*/
/********************************************************************************F*/
void StressGamePlayC::SetWriteCsv(bool bWriteCsv)
{
    m_bWriteCsv = bWriteCsv;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetVerbosity

    \Description
        Used to set verbosity level for StressGamePlay object

    \Input iVerbosityLevel  - verbosity level

    \Version 07/29/2009 (mclouatre)
*/
/********************************************************************************F*/
void StressGamePlayC::SetVerbosity(int32_t iVerbosityLevel)
{
    m_iVerbosity = iVerbosityLevel;
}

void StressGamePlayC::SetDebugPoll(bool bEnabled)
{
    m_bDebugPoll = bEnabled;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetLateRate

    \Description
        Used to set latency printing rate for StressGamePlay object

    \Input iLateRate            - latency printing rate

    \Version 07/28/2010 (jbrookes)
*/
/********************************************************************************F*/
void StressGamePlayC::SetLateRate(int32_t iLateRate)
{
    m_iLateRate = iLateRate;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetGame

    \Description
        Used to inform StressGamePlay object which Blaze game to use.

    \Input *pGame   - pointer to Blaze game object

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
void StressGamePlayC::SetGame(Blaze::GameManager::Game *pGame)
{
    ConnApiRefT *pConnApi = m_pConnApiAdapter->getConnApiRefT(pGame);

    if (pGame)
    {
        if (pConnApi != NULL)
        {
            ConnApiAddCallback(pConnApi, StressGamePlayC::ConnApiCb, this);

            m_pGame = pGame;

            // there is a race condition between CONNAPI_STATUS_ACTV and MatchMakingFinished
            // if there is a link available already we need to use it (CONNAPI_STATUS_ACTV already occurred)
            ConnApiClientT GameServerClient;
            if (ConnApiStatus(pConnApi, 'gsrv', &GameServerClient, sizeof(GameServerClient)) == TRUE)
            {
                if (GameServerClient.pGameLinkRef != NULL)
                {
                    NetPrintf(("stressgameplay: game's NetGameLink already available!"));
                    m_pNetGameLink[0] = GameServerClient.pGameLinkRef;

                    m_pNetGameLinkStream[0] = NetGameLinkCreateStream(m_pNetGameLink[0], STRESS_MAX_PLAYERS, (signed)m_uGameStreamId, STRESS_MAX_STREAM_MSG_LENGTH, STRESS_MAX_STREAM_MSG_LENGTH, STRESS_MAX_STREAM_MSG_LENGTH);
                    if (m_pNetGameLinkStream[0] != NULL)
                    {
                        m_pNetGameLinkStream[0]->Recv = &StreamRecvHook;
                        m_pNetGameLinkStream[0]->pRefPtr = this;
                    }
                }
            }
        }
        else
        {
            StressPrintf("stressgameplay: could not get connapi ref\n");
        }
    }
    else
    {
        if (pConnApi != NULL)
        {
            ConnApiRemoveCallback(pConnApi, StressGamePlayC::ConnApiCb, this);
        }
        m_pGame = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::GetDist

    \Description
        Used to inform StressGamePlay object which Blaze game to use.

    \Input None.

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
bool StressGamePlayC::GetDist()
{
    return(m_bUseDist);
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetDist

    \Description
        Used to inform StressGamePlay object which game mode to use.

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
void StressGamePlayC::SetDist()
{
    m_bUseDist = TRUE;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetPacketSize

    \Description
        Used to inform StressGamePlay object of the packet size to be used

    \Input uPacketSize  - packet size

    \Output None.

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
void StressGamePlayC::SetPacketSize(uint32_t uPacketSize)
{
    m_uPacketSize = uPacketSize;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetPacketsPerSec

    \Description
        Used to inform StressGamePlay object of the packet rate to be used

    \Input uPacketsPerSec   - packets per second

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
void StressGamePlayC::SetPacketsPerSec(uint32_t uPacketsPerSec)
{
    m_uPacketsPerSec = uPacketsPerSec;
    if (uPacketsPerSec != 0)
    {
        m_iUsecsPerFrame = 1000000/uPacketsPerSec;
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetNumberOfPlayers

    \Description
        Used to inform StressGamePlay object of the number of players in the game

    \Input iNumberOfPlayers - number of players

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
void StressGamePlayC::SetNumberOfPlayers(int32_t iNumberOfPlayers)
{
    m_iNumberOfPlayers = iNumberOfPlayers;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::SetNumberOfPlayers

    \Description
        Used to inform StressGamePlay object of the number of players in the game

    \Input iNumberOfPlayers - number of players

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
void StressGamePlayC::SetNetGameDistStreamId(uint32_t uGameStreamId)
{
    m_uGameStreamId = uGameStreamId;
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::ConnApiCb

    \Description
        ConnApi callback function registeredby StressGamePlayC

    \Input pConnApi - pointer to ConnApi that invokes the callback
    \Input pCbInfo  - info structure conveying the data for the event being notified
    \Input pUserData    - user-date passed to ConnApi when regisering the callback

    \Version 07/08/2009 (mclouatre)
*/
/********************************************************************************F*/
void StressGamePlayC::ConnApiCb(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData)
{
    StressGamePlayC *pStressGamePlayObj = (StressGamePlayC *) pUserData;

    // watch for DEST callback, so we can delete our refs
    if (pCbInfo->eType == CONNAPI_CBTYPE_DESTEVENT)
    {
        NetPrintf(("stressgameplay: CONNAPI_CBTYPE_DESTEVENT event! invalidate dist and link references if applicable\n"));

        // uninstall the send callback now that we are disconnecting
        _StressUninstallSendCallback();

        /* Currently NetGameDist implementation is not working in peer2peer mode*/
        if (!pStressGamePlayObj->m_bPeer2peer)
        {
            pStressGamePlayObj->TeardownNetGameDist();
        }
        else
        {
            if (pStressGamePlayObj->m_pNetGameLink[pCbInfo->iClientIndex] != NULL)
            {
                if (pStressGamePlayObj->m_pNetGameLinkStream[pCbInfo->iClientIndex] != NULL)
                {
                    NetGameLinkDestroyStream(pStressGamePlayObj->m_pNetGameLink[pCbInfo->iClientIndex], pStressGamePlayObj->m_pNetGameLinkStream[pCbInfo->iClientIndex]);
                    pStressGamePlayObj->m_pNetGameLinkStream[pCbInfo->iClientIndex] = NULL;
                }

                NetPrintf(("stressgameplay: cleaning up link for user %d\n", pCbInfo->iClientIndex));
                pStressGamePlayObj->m_pNetGameLink[pCbInfo->iClientIndex] = NULL;
            }
            else
            {
                NetPrintf(("stressgameplay: got DESTEVENT for client %d with no link!\n", pCbInfo->iClientIndex));
            }
        }
    }
    else if (pCbInfo->eType == CONNAPI_CBTYPE_GAMEEVENT)
    {
        // watch for ACTV callback, to cache link ref
        if (pCbInfo->eNewStatus == CONNAPI_STATUS_ACTV)
        {
            ConnApiClientT GameServerClient;

            // install the send callback now that we are connected
            _StressInstallSendCallback();

            /* Peer2Peer mode */
            if (pStressGamePlayObj->m_bPeer2peer == true && pCbInfo->pClient->pGameLinkRef != NULL )
            {
                StressPrintf("stressgameplay: adding netgamelink for client id %d\n", pCbInfo->iClientIndex);
                pStressGamePlayObj->m_pNetGameLink[pCbInfo->iClientIndex] = pCbInfo->pClient->pGameLinkRef;
                
                StressPrintf("stressgameplay: creating netgamelink for client id %d\n", pCbInfo->iClientIndex);
                pStressGamePlayObj->m_pNetGameLinkStream[pCbInfo->iClientIndex] = NetGameLinkCreateStream(pStressGamePlayObj->m_pNetGameLink[pCbInfo->iClientIndex], STRESS_MAX_PLAYERS, (signed)pStressGamePlayObj->m_uGameStreamId, STRESS_MAX_STREAM_MSG_LENGTH, STRESS_MAX_STREAM_MSG_LENGTH, STRESS_MAX_STREAM_MSG_LENGTH);
                if (pStressGamePlayObj->m_pNetGameLinkStream[pCbInfo->iClientIndex] != NULL)
                {
                    pStressGamePlayObj->m_pNetGameLinkStream[pCbInfo->iClientIndex]->Recv = &StreamRecvHook;
                    pStressGamePlayObj->m_pNetGameLinkStream[pCbInfo->iClientIndex]->pRefPtr = pStressGamePlayObj;
                }
            }
            /* server hosted mode */
            else if (ConnApiStatus(pConnApi, 'gsrv', &GameServerClient, sizeof(GameServerClient)) == TRUE)
            {
                char strAddrText[20];
                StressPrintf("stressgameplay: connected to game %d/%s:%d\n", GameServerClient.ClientInfo.uId,
                    SocketInAddrGetText(GameServerClient.ClientInfo.uAddr, strAddrText, sizeof(strAddrText)), GameServerClient.ClientInfo.uTunnelPort);
                pStressGamePlayObj->m_pNetGameLink[0] = GameServerClient.pGameLinkRef;
                
                pStressGamePlayObj->m_pNetGameLinkStream[0] = NetGameLinkCreateStream(pStressGamePlayObj->m_pNetGameLink[0], STRESS_MAX_PLAYERS, (signed)pStressGamePlayObj->m_uGameStreamId, STRESS_MAX_STREAM_MSG_LENGTH, STRESS_MAX_STREAM_MSG_LENGTH, STRESS_MAX_STREAM_MSG_LENGTH);
                if (pStressGamePlayObj->m_pNetGameLinkStream[0] != NULL)
                {
                    pStressGamePlayObj->m_pNetGameLinkStream[0]->Recv = &StreamRecvHook;
                    pStressGamePlayObj->m_pNetGameLinkStream[0]->pRefPtr = pStressGamePlayObj;
                }
            }

            /* Currently NetGameDist implementation is not working in peer2peer mode*/
            if (pStressGamePlayObj->GetDist() && pStressGamePlayObj->m_bPeer2peer == false)
            {
                pStressGamePlayObj->SetupNetGameDist();
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::Poll

    \Description
        Execute blocking poll, for an amount of time up to the rate we are sending at.

    \Input uCurTick     - current time

    \Output
        int32_t         - true if we have reached our send rate, else false

    \Version 08/11/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t StressGamePlayC::Poll(uint64_t &uCurTick)
{
    static uint64_t _uNextSendTick = 0;
    int32_t iMustSend, iRemain;

    if (_uNextSendTick == 0)
    {
        _uNextSendTick = uCurTick;
    }

    iRemain = NetTickDiff(_uNextSendTick, uCurTick);

    if (iRemain < (-10 * m_iUsecsPerFrame))
    {
        StressPrintf("stressgameplay: at %llu, too far behind, let's sync up\n", uCurTick);
        iMustSend = 1;
        _uNextSendTick = uCurTick + (uint32_t)m_iUsecsPerFrame;
    }
    else if (iRemain <= 0)
    {
        iMustSend = 1;
        _uNextSendTick += (uint32_t)m_iUsecsPerFrame;

        if (m_bDebugPoll)
        {
            StressPrintf("stressgameplay: at %llu, send=%dms\n", uCurTick, iRemain/1000);
        }
    }
    else
    {
        iMustSend = 0;

        if (m_bDebugPoll)
        {
            StressPrintf("stressgameplay: at %llu, poll=%dms\n", uCurTick, iRemain/1000);
        }

        #if DIRTYVERS >= DIRTYSDK_VERSION_MAKE(15, 1, 5, 0, 2)
        NetConnControl('poln', iRemain, 0, NULL, NULL);
        #else
        NetConnControl('poll', iRemain/1000, 0, NULL, NULL);
        #endif
    }

    return(iMustSend);
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::Send

    \Description
        Send, using different methods as appropriate based on mode.

    \Input uCurTick     - current time

    \Version 08/11/2010 (jbrookes) Split from Update()

    \Input uCurTick     - current time
*/
/********************************************************************************F*/
void StressGamePlayC::Send(uint32_t uCurTick)
{
    // only send if game is active/started
    if (!m_bGameStarted)
    {
        return;
    }

    // send based on mode
    if (m_bUseDist)
    {
        // if we have a dist ref, use it
        if (m_pNetGameDist != NULL)
        {
            DistSend(0, uCurTick);
        }
    }
    else if (!m_bPeer2peer)
    {
        // if we have a link ref, use that
        if (m_pNetGameLink[0] != NULL)
        {
            LinkSend(m_pNetGameLink[0], &m_LinkStat[0], 0, uCurTick);
        }
    }
    else
    {
        // in peer-web mode, iterate through all clients
        int32_t iIndex;
        for (iIndex = 0; iIndex < STRESS_MAX_PLAYERS; iIndex += 1)
        {
            if (m_pNetGameLink[iIndex] != NULL)
            {
                LinkSend(m_pNetGameLink[iIndex], &m_LinkStat[iIndex], iIndex, uCurTick);
                // to explicit tunnel flush to model NFS send pattern
                //$$ note -- this requires a hack to NetConn to allow calling NetConnIdle without rate limiting
                //NetConnIdle();
            }
        }
    }
    // explicitly flush packets (flushing here allows multiple packets to be bundled into one)
    NetConnIdle();
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::Recv

    \Description
        Try and receive, using different methods as appropriate based on mode.

    \Input uCurTick     - current time

    \Version 08/11/2010 (jbrookes) Split from Update()
*/
/********************************************************************************F*/
void StressGamePlayC::Recv(uint32_t uCurTick)
{
    // recv based on mode
    if (m_bUseDist)
    {
        // if we have a dist ref, use it
        if (m_pNetGameDist != NULL)
        {
            DistRecv(0, uCurTick);
        }
        else
        {
            int32_t iTemp = m_iLateRate;
            m_iLateRate = 1;
            GameStatsUpdate(m_pNetGameLink[0], &m_LinkStat[0], 0, 0, uCurTick);
            m_iLateRate = iTemp;
        }
    }
    else if (!m_bPeer2peer)
    {
        // if we have a link ref, use that
        if (m_pNetGameLink[0] != NULL)
        {
            LinkRecv(m_pNetGameLink[0], &m_LinkStat[0], 0, uCurTick);
        }
    }
    else
    {
        // in peer-web mode, iterate through all clients
        int32_t iIndex;
        for (iIndex = 0; iIndex < STRESS_MAX_PLAYERS; iIndex += 1)
        {
            if (m_pNetGameLink[iIndex] != NULL)
            {
                LinkRecv(m_pNetGameLink[iIndex], &m_LinkStat[iIndex], iIndex, uCurTick);
            }
        }
    }
}

/*F********************************************************************************/
/*!
    \Function StressGamePlayC::Update

    \Description
        Game play update function.

    \Input uCurTick     - current time

    \Output
        bool: true to continue process, false otherwise

    \Version 08/07/2009 (mclouatre)
*/
/********************************************************************************F*/
bool StressGamePlayC::Update(uint32_t uCurTick)
{
    int32_t iPollResult;
    uint64_t uCurTickUsec = NetTickUsec();

    if (m_bGameFailed)
    {
        return(false);
    }
   
    for (int32_t iIndex = 0; iIndex < STRESS_MAX_PLAYERS; iIndex++)
    {
        if (m_pNetGameLink[iIndex] != NULL)
        {
            NetGameLinkUpdate(m_pNetGameLink[iIndex]);
        }
    }

    // execute blocking poll
    iPollResult = Poll(uCurTickUsec);

    // try to receive
    Recv(uCurTickUsec/1000);

    // if it's not time to send yet, return
    if (!iPollResult)
    {
        return(true);
    }

    #if STRESSGAMEPLAY_DEBUGPOLL
    StressPrintf("stressgameplay: sending\n");
    #endif

    Send(uCurTickUsec/1000);
    return(true);
}

void StressGamePlayC::StreamRecvHook(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen)
{    
    StressPrintf("stressgameplay: NetGameLinkStream received %d bytes\n", iLen);    
}

