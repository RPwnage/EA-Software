/*H********************************************************************************/
/*!
    \File voiptunnel.c

    \Description
        This module implements the main logic for the VoipTunnel server.

        Description forthcoming.

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/netconn.h"

#include "DirtySDK/voip/voipdef.h"
#include "DirtySDK/voip/voiptunnel.h"
#include "voippriv.h"
#include "voippacket.h"

/*** Defines **********************************************************************/

//! define this as TRUE to enable lookup table test code
#define VOIPTUNNEL_LOOKUP_TEST                      (FALSE)

//! voiptunnel broadcast flags
#define VOIPTUNNEL_GAMEFLAG_BROADCAST_ALL           0
#define VOIPTUNNEL_GAMEFLAG_BROADCAST_ALLOTHERS     1
#define VOIPTUNNEL_GAMEFLAG_SEND_SINGLE             2
#define VOIPTUNNEL_GAMEFLAG_SEND_MULTI              4
#define VOIPTUNNEL_GAMEFLAG_MASK                    0xf

#define VOIPTUNNEL_GAMEFLAG_VDP                     16

/*** Type Definitions *************************************************************/

//! VoIP packet data that this module cares about (header + first 4 bytes)
typedef struct VoipTunnelPacketT
{
    VoipPacketHeadT Head;       //!< packet header
    uint8_t aRemoteClientId[4]; //!< present in all packets except for mic packets, which have the send mask here instead
} VoipTunnelPacketT;

//! client lookup table element
typedef struct VoipTunnelLookupElemT
{
    uint32_t uClientId;
    uint32_t uClientIdx;
} VoipTunnelLookupElemT;

//! module state
struct VoipTunnelRefT
{
    // module memory group
    int32_t iMemGroup;              //!< module mem group id
    void *pMemGroupUserData;        //!< user data associated with mem group

    SocketT *pVoipSocket;           //!< virtual socket for receiving tunneled voip data
    VoipTunnelCallbackT *pCallback; //!< optional voiptunnel event callback
    void *pUserData;                //!< user data for voiptunnel callback
    uint32_t uLocalClientId;        //!< local client id (used for tunnel connectivity)
    uint16_t uVoipPort;             //!< virtual voip port
    uint16_t uVoiceRecvTimeout;     //!< number of milliseconds before clearing RECVVOICE flag
    uint8_t uDebugLevel;            //!< debug level
    uint8_t bPortSniff;             //!< TRUE if port sniffing enabled, else FALSE (default FALSE)
    uint8_t _pad[2];
    int32_t iNumClients;            //!< number of tunnel clients
    int32_t iNumTalkingClients;     //!< number of "talking" clients (i.e. client with VOIPTUNNEL_CLIENTFLAG_RECVVOICE flag set)
    int32_t iMaxClients;            //!< maximum number tunnel clients
    int32_t iMaxVoiceBroadcasters;  //!< maximum number of players can be talking at once in a single game
    VoipTunnelLookupElemT *pLookupTable; //!< fast-lookup table
    int32_t iNumGames;              //!< current number of games
    int32_t iMaxGames;              //!< maximum number of supported gameservers
    uint32_t uVoiceDataDropMetric;  //!< the number of voice packets that were not rebroadcast globally due to max broadcasters constraint
    uint32_t uVoiceMaxTalkersMetric;//!< the number of time the max broadcasters event was reached
    VoipTunnelGameT *pGameList;     //!< list of games
    VoipTunnelClientT ClientList[1]; //!< variable-length tunnel client list -- MUST COME LAST IN THE STRUCTURE
};

/*** Variables ********************************************************************/


/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _VoipTunnelEventCallback

    \Description
        Call user event callback, if it is available.

    \Input *pVoipTunnel - voiptunnel ref
    \Input eEvent       - event triggering callback
    \Input iGameIdx     - index of game in voiptunnel game list
    \Input *pClient     - pointer to client associated with event
    \Input iDataSize    - size of packet data associated with event
    \Input uTick        - time tick to associate with the event

    \Version 03/01/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTunnelEventCallback(VoipTunnelRefT *pVoipTunnel, VoipTunnelEventE eEvent, int32_t iGameIdx, VoipTunnelClientT *pClient, int32_t iDataSize, uint32_t uTick)
{
    if (pVoipTunnel->pCallback != NULL)
    {
        VoipTunnelEventDataT EventData;
        EventData.eEvent = eEvent;
        EventData.pClient = pClient;
        EventData.iGameIdx = iGameIdx;
        EventData.iDataSize = iDataSize;
        EventData.uTick = uTick;
        pVoipTunnel->pCallback(pVoipTunnel, &EventData, pVoipTunnel->pUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelClientListSend
 
    \Description
        Broadcast input data to all other clients in our session.

    \Input *pVoipTunnel - voiptunnel ref
    \Input *pSrcClient  - pointer to client that sent the data
    \Input uDstClientId - clientId to send to, if SEND_SINGLE
    \Input iGameIdx     - index of game in voiptunnel game list
    \Input *pPacketData - pointer to packet data
    \Input iPacketSize  - size of packet data
    \Input *pAddr       - address data came from
    \Input uSendFlag    - send flags

    \Version 03/31/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTunnelClientListSend(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pSrcClient, uint32_t uDstClientId, int32_t iGameIdx, const char *pPacketData, int32_t iPacketSize, struct sockaddr *pAddr, uint32_t uSendFlag)
{
    VoipTunnelClientT *pClient;
    int32_t iClientIdx, iResult;

    // if vdp, restore the header
    if (uSendFlag & VOIPTUNNEL_GAMEFLAG_VDP)
    {
        pPacketData -= 2;
        iPacketSize += 2;
    }

    // unicast or broadcast?
    if (!(uSendFlag & VOIPTUNNEL_GAMEFLAG_SEND_SINGLE))
    {
        VoipTunnelGameMappingT *pGameMapping;
        VoipTunnelGameT *pGame = &pVoipTunnel->pGameList[iGameIdx];

        // forward data data to other clients in session
        for (iClientIdx = 0; iClientIdx < VOIPTUNNEL_MAXGROUPSIZE; iClientIdx++)
        {
            // make sure the client is valid
            if (pGame->aClientList[iClientIdx] == 0)
            {
                continue;
            }

            // ref client
            if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, pGame->aClientList[iClientIdx])) == NULL)
            {
                continue;
            }

            // don't send data to source client
            if ((uSendFlag & VOIPTUNNEL_GAMEFLAG_BROADCAST_ALLOTHERS) && (pClient == pSrcClient))
            {
                continue;
            }

            pGameMapping = VoipTunnelClientMatchGameIdx(pSrcClient, iGameIdx);

            // if traffic voip send mask not set for this client, skip them
            if ((pGameMapping->uTrafficVoipSendMask & (1 << (unsigned)iClientIdx)) == 0)
            {
                NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: client traffic voip send mask mute of voice from clientId=0x%08x to clientId=0x%08x\n", pSrcClient->uClientId, pClient->uClientId));
                continue;
            }

            // if topology voip send mask not set for this client, skip them
            if ((pGameMapping->uTopologyVoipSendMask & (1 << (unsigned)iClientIdx)) == 0)
            {
                NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: client topology voip send mask mute of voice from clientId=0x%08x to clientId=0x%08x\n", pSrcClient->uClientId, pClient->uClientId));
                continue;
            }

            // rewrite address to send to this client
            SockaddrInSetAddr(pAddr, pClient->uRemoteAddr);
            // if port sniffing is enabled, rewrite port as well
            if (pVoipTunnel->bPortSniff)
            {
                // don't send if we don't have a port to send to
                if (pClient->uRemoteVoipPort == 0)
                {
                    continue;
                }
                SockaddrInSetPort(pAddr, pClient->uRemoteVoipPort);
            }

            if ((iResult = SocketSendto(pVoipTunnel->pVoipSocket, pPacketData, iPacketSize, 0, pAddr, sizeof(*pAddr))) == iPacketSize)
            {
                // call user callback
                _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_SENDVOICE, iGameIdx, pClient, iPacketSize, NetTick());
            }
            else
            {
                NetPrintf(("voiptunnel: send of %d byte voice packet from clientId=0x%08x to clientId=0x%08x failed (err=%d)\n", iPacketSize, pSrcClient->uClientId, pClient->uClientId, iResult));
            }
        }
    }
    else
    {
        if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uDstClientId)) != NULL)
        {
            // rewrite address to send to this client
            SockaddrInSetAddr(pAddr, pClient->uRemoteAddr);
            // if port sniffing is enabled, rewrite port as well
            if (pVoipTunnel->bPortSniff)
            {
                // don't send if we don't have a port to send to
                if (pClient->uRemoteVoipPort == 0)
                {
                    return;
                }
                SockaddrInSetPort(pAddr, pClient->uRemoteVoipPort);
            }

            if ((iResult = SocketSendto(pVoipTunnel->pVoipSocket, pPacketData, iPacketSize, 0, pAddr, sizeof(*pAddr))) == iPacketSize)
            {
                // call user callback
                _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_SENDVOICE, iGameIdx, pClient, iPacketSize, NetTick());
            }
            else
            {
                NetPrintf(("voiptunnel: send of %d byte voice packet from clientId=0x%08x to clientId=0x%08x failed (err=%d)\n", iPacketSize, pSrcClient->uClientId, pClient->uClientId, iResult));
            }
        }            
        else
        {
            NetPrintf(("voiptunnel: unable to ref client with id=0x%08x for unicast send\n", uDstClientId));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelValidatePacketType

    \Description
        Returns whether the data points to a valid packet type or not.

    \Input *pPacketData - pointer to type to evaluate

    \Version 08/24/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTunnelValidatePacketType(const char *pPacketData)
{
    return(!memcmp(pPacketData, "CO", 2) || !memcmp(pPacketData, "DSC", 3) || !memcmp(pPacketData, "PNG", 3) || !memcmp(pPacketData, "MIC", 3));
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelValidatePacket

    \Description
        Locate packet data and validate packet

    \Input *pVoipTunnel - voip tunnel
    \Input *pPacketData - pointer to packet head
    \Input iPacketSize  - size of received data
    \Input pVdpHeader   - vdp header

    \Output
        const VoipTunnelPacketT *pPacket  - pointer to packet

    \Version 08/24/2006 (jbrookes)
*/
/********************************************************************************F*/
static VoipTunnelPacketT *_VoipTunnelValidatePacket(VoipTunnelRefT *pVoipTunnel, char *pPacketData, int32_t iPacketSize, uint32_t *pVdpHeader)
{
    // try and determine if there is a VDP header or not
    if (_VoipTunnelValidatePacketType(pPacketData+2))
    {
        // VDP header... so adjust the pointer to account for it
        NetPrintfVerbose((pVoipTunnel->uDebugLevel, 2, "voiptunnel: vdp header detected\n"));
        pPacketData += 2;
        iPacketSize -= 2;
        *pVdpHeader = TRUE;
    }
    else if (_VoipTunnelValidatePacketType(pPacketData))
    {
        NetPrintfVerbose((pVoipTunnel->uDebugLevel, 2, "voiptunnel: no vdp header\n"));
        *pVdpHeader = FALSE;
    }
    else
    {
        NetPrintf(("voiptunnel: unknown VoIP packet type; discarding\n"));
        return(NULL);
    }

    // validate size
    if (iPacketSize < (signed)sizeof(VoipTunnelPacketT))
    {
        NetPrintf(("voiptunnel: voip packet size is too small; discarding\n"));
        return(NULL);
    }
    
    return((VoipTunnelPacketT *)pPacketData);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelGetClientId
 
    \Description
        Read client ID from voice packet

    \Input *pPacket     - pointer to packet
    \Input iPacketSize  - size of received data

    \Output
        int32_t         - clientId, or zero
 
    \Version 05/16/2006 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _VoipTunnelGetClientId(const VoipTunnelPacketT *pPacket, int32_t iPacketSize)
{
    uint32_t uClientId;
    
    // extract the client identifier
    uClientId = pPacket->Head.aClientId[0] << 24;
    uClientId |= pPacket->Head.aClientId[1] << 16;
    uClientId |= pPacket->Head.aClientId[2] << 8;
    uClientId |= pPacket->Head.aClientId[3];
    
    // return to caller
    return(uClientId);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelGetGameId

    \Description
        Read game ID from voice packet

    \Input *pPacket     - pointer to packet
    \Input iPacketSize  - size of received data

    \Output
        uint32_t        - gameId

    \Version 25/11/2020 (mclouatre)
*/
/********************************************************************************F*/
static uint32_t _VoipTunnelGetGameId(const VoipTunnelPacketT *pPacket, int32_t iPacketSize)
{
    uint32_t uGameId;

    // extract the voip session identifier
    uGameId = pPacket->Head.aSessionId[0] << 24;
    uGameId |= pPacket->Head.aSessionId[1] << 16;
    uGameId |= pPacket->Head.aSessionId[2] << 8;
    uGameId |= pPacket->Head.aSessionId[3];

    return(uGameId);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelLookupSort
 
    \Description
        qsort callback used to sort fast lookup array.

    \Input *_pElem0     - pointer to first element to compare
    \Input *_pElem1     - pointer to second element to compare

    \Output
        int32_t         - sort value (one or minus one)
 
    \Version 04/03/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTunnelLookupSort(const void *_pElem0, const void *_pElem1)
{
    const VoipTunnelLookupElemT *pElem0 = (const VoipTunnelLookupElemT *)_pElem0;
    const VoipTunnelLookupElemT *pElem1 = (const VoipTunnelLookupElemT *)_pElem1;

    return((pElem0->uClientId > pElem1->uClientId) ? 1 : -1);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelLookupClient
 
    \Description
        Lookup a client given the clientId using binary search on sorted array.

    \Input *pVoipTunnel     - module state
    \Input uClientId        - client identifier

    \Output
        VoipTunnelClientT * - pointer to found client, or NULL if not found
 
    \Version 04/03/2007 (jbrookes)
*/
/********************************************************************************F*/
static VoipTunnelClientT *_VoipTunnelLookupClient(VoipTunnelRefT *pVoipTunnel, uint32_t uClientId)
{
    int32_t iCheck, iLow, iHigh;
    uint32_t uCheckId;
    
    // execute binary search on sorted lookup table
    for (iLow = 0, iHigh = pVoipTunnel->iNumClients-1; iLow <= iHigh; )
    {
        iCheck = iLow + ((iHigh - iLow) / 2);
        if ((uCheckId = pVoipTunnel->pLookupTable[iCheck].uClientId) > uClientId)
        {
            iHigh = iCheck - 1;
        }
        else if (uCheckId < uClientId)
        {
            iLow = iCheck + 1;
        }
        else
        {
            #if VOIPTUNNEL_LOOKUP_TEST
            NetPrintf(("voiptunnel: lookup found client id=0x%08x\n", uClientId));    
            #endif
            return(&pVoipTunnel->ClientList[pVoipTunnel->pLookupTable[iCheck].uClientIdx]);
        }
    }

    // not found
    NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: lookup could not find client id=0x%08x\n", uClientId));    
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelLookupBuild

    \Description
        Build a lookup table sorted by clientId for fast lookups.

    \Input *pVoipTunnel     - module state

    \Version 04/03/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTunnelLookupBuild(VoipTunnelRefT *pVoipTunnel)
{
    int32_t iClient;
    
    // only build it if the table has been allocated
    if (pVoipTunnel->pLookupTable == NULL)
    {
        return;
    }
    
    // build unsorted table
    for (iClient = 0; iClient < pVoipTunnel->iNumClients; iClient++)
    {
        pVoipTunnel->pLookupTable[iClient].uClientId = pVoipTunnel->ClientList[iClient].uClientId;
        pVoipTunnel->pLookupTable[iClient].uClientIdx = (unsigned)iClient;
    }
    
    // sort it by clientId
    qsort(pVoipTunnel->pLookupTable, iClient, sizeof(pVoipTunnel->pLookupTable[0]), _VoipTunnelLookupSort);
}

#if VOIPTUNNEL_LOOKUP_TEST
/*F********************************************************************************/
/*!
    \Function _VoipTunnelLookupTest

    \Description
        Test code to run lookup code through a few simple test cases.

    \Input *pVoipTunnel     - module state

    \Version 04/03/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTunnelLookupTest(VoipTunnelRefT *pVoipTunnel)
{
    VoipTunnelLookupElemT _LookupTable[32];
    
    // set up temp table for testing
    pVoipTunnel->pLookupTable = _LookupTable;
    
    // test no elements
    pVoipTunnel->iNumClients = 0;
    _VoipTunnelLookupBuild(pVoipTunnel);
    _VoipTunnelLookupClient(pVoipTunnel, 0);
 
    // test one element
    pVoipTunnel->iNumClients = 1;
    pVoipTunnel->ClientList[0].uClientId = 0xdeadbeef;
    _VoipTunnelLookupBuild(pVoipTunnel);
    _VoipTunnelLookupClient(pVoipTunnel, 0xdeadbeef);

    // test two elements
    pVoipTunnel->iNumClients = 2;
    pVoipTunnel->ClientList[1].uClientId = 0xcacabeef;
    _VoipTunnelLookupBuild(pVoipTunnel);
    _VoipTunnelLookupClient(pVoipTunnel, 0xdeadbeef);
    _VoipTunnelLookupClient(pVoipTunnel, 0xcacabeef);

    // test three elements
    pVoipTunnel->iNumClients = 3;
    pVoipTunnel->ClientList[2].uClientId = 0xf000beef;
    _VoipTunnelLookupBuild(pVoipTunnel);
    _VoipTunnelLookupClient(pVoipTunnel, 0xdeadbeef);
    _VoipTunnelLookupClient(pVoipTunnel, 0xcacabeef);
    _VoipTunnelLookupClient(pVoipTunnel, 0xf000beef);
    
    // test many elements
    pVoipTunnel->iNumClients = 8;
    pVoipTunnel->ClientList[3].uClientId = 0xf000b00f;
    pVoipTunnel->ClientList[4].uClientId = 0xf000baaf;
    pVoipTunnel->ClientList[5].uClientId = 0xeaa0beef;
    pVoipTunnel->ClientList[6].uClientId = 0x0000beef;
    pVoipTunnel->ClientList[7].uClientId = 0x1000beef;
    _VoipTunnelLookupBuild(pVoipTunnel);
    _VoipTunnelLookupClient(pVoipTunnel, 0xdeadbeef);
    _VoipTunnelLookupClient(pVoipTunnel, 0xcacabeef);
    _VoipTunnelLookupClient(pVoipTunnel, 0xf000beef);
    _VoipTunnelLookupClient(pVoipTunnel, 0xf000b00f);
    _VoipTunnelLookupClient(pVoipTunnel, 0xf000baaf);
    _VoipTunnelLookupClient(pVoipTunnel, 0xeaa0beef);
    _VoipTunnelLookupClient(pVoipTunnel, 0x0000beef);
    _VoipTunnelLookupClient(pVoipTunnel, 0x1000beef);

    // reset    
    pVoipTunnel->iNumClients = 0;
    pVoipTunnel->pLookupTable = NULL;
    ds_memclr(pVoipTunnel->ClientList, sizeof(pVoipTunnel->ClientList));
}
#endif

/*F********************************************************************************/
/*!
    \Function _VoipTunnelReleaseVoipBroadcastingSlotForTargetClient

    \Description
        Releases the broadcasting slot that the client possesses as an originator
        when sending voip to the target client (consumer).

    \Input *pVoipTunnel         - module state
    \Input *pSourceClient       - source client
    \Input iGameIdx             - index of game in voiptunnel game list
    \Input iTargetClientIndex   - client index to remove

    \Output
        int32_t                 - updated sendmask for the source client.

    \Notes
        voice squelching: There are only 4 broadcasting slots per consumer client.
        Only 4 originator clients at a time can send voip to a specific
        consumer client.

        $$TODO MCLOUATRE DECEMBER 2020 - should this take into account ALL games
        that the client is involved in?

    \Version 02/01/2019 (amakoukji)
*/
/********************************************************************************F*/
static uint32_t _VoipTunnelReleaseVoipBroadcastingSlotForTargetClient(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pSourceClient, int32_t iGameIdx, int32_t iTargetClientIndex)
{
    VoipTunnelGameMappingT *pGameMapping = VoipTunnelClientMatchGameIdx(pSourceClient, iGameIdx);

    // if this is a client we're talking to
    if ((pGameMapping->uTrafficVoipSendMask) & (1 << iTargetClientIndex))
    {
        VoipTunnelClientT *pTargetClient = VoipTunnelClientListMatchId(pVoipTunnel, pGameMapping->aClientIds[iTargetClientIndex]);

        NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: 0x%08x is not talking to 0x%08x anymore.\n", pSourceClient->uClientId, pTargetClient ? pTargetClient->uClientId : 0xFFFFFFFF));

        pGameMapping->uTrafficVoipSendMask &= ~(1 << iTargetClientIndex);
        if (pTargetClient)
        {
            VoipTunnelGameMappingT *pTargetGameMapping = VoipTunnelClientMatchGameIdx(pTargetClient, iGameIdx);

            pTargetGameMapping->iNumTalkers--;
            if (pTargetClient->uFlags & VOIPTUNNEL_CLIENTFLAG_MAX_VOICES_REACHED)
            {
                pTargetClient->uFlags &= ~VOIPTUNNEL_CLIENTFLAG_MAX_VOICES_REACHED;
                _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_AVLBVOICE, iGameIdx, pTargetClient, 0, NetTick());
                NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: 0x%08x is accepting new talkers again.\n", pTargetClient->uClientId));
            }
        }
    }

    return(pGameMapping->uTrafficVoipSendMask);
}


/*F********************************************************************************/
/*!
    \Function _VoipTunnelAcquireVoipBroadcastingSlot

    \Description
        Allocates sendmask slots to the target clients if some are available.

    \Input *pVoipTunnel     - module state
    \Input *pClient         - client ref
    \Input iGameIdx         - index of game in voiptunnel game list
    \Input uTargetSendMask  - the clients we want to send to

    \Output
        int32_t             - acquired sendmask for the required client.

    \Notes
        voice squelching: There are only 4 broadcasting slots per consumer client.
        Only 4 originator clients at a time can send voip to a specific
        consumer client.

        $$TODO MCLOUATRE DECEMBER 2020 - should this take into account ALL games
        that the client is involved in?

    \Version 01/15/2010 (cvienneau)
*/
/********************************************************************************F*/
static uint32_t _VoipTunnelAcquireVoipBroadcastingSlot(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient, int32_t iGameIdx, uint32_t uTargetSendMask)
{
    int32_t iIndex;
    VoipTunnelGameT *pGame = &pVoipTunnel->pGameList[iGameIdx];
    VoipTunnelGameMappingT *pGameMapping = VoipTunnelClientMatchGameIdx(pClient, iGameIdx);
    uint32_t uAcquiredSendMask = pGameMapping->uTrafficVoipSendMask;
    uint32_t uToReleaseMask = 0;
    uint32_t uToAddMask = 0;

    if (uAcquiredSendMask == uTargetSendMask)
    {
        return(uAcquiredSendMask);
    }

    // if there's one or more targets we no longer need, release
    uToReleaseMask = (uAcquiredSendMask & (~uTargetSendMask));
    if (uToReleaseMask != 0)
    {
        for (iIndex = 0; iIndex < VOIPTUNNEL_MAXGROUPSIZE; iIndex++)
        {
            // if this is a client we'd like to release but haven't yet
            if (uToReleaseMask & (1 << iIndex))
            {
                uAcquiredSendMask = _VoipTunnelReleaseVoipBroadcastingSlotForTargetClient(pVoipTunnel, pClient, iGameIdx, iIndex);
            }
        }
    }

    // if there's one or more target we have not yet acquired, try to acquire
    uToAddMask = ((~uAcquiredSendMask) & uTargetSendMask);
    if (uToAddMask != 0)
    {
        for (iIndex = 0; iIndex < VOIPTUNNEL_MAXGROUPSIZE; iIndex++)
        {
            // if this is a client we'd like to talk to, but don't already
            if (uToAddMask & (1 << iIndex))
            {
                VoipTunnelClientT *pTargetClient = VoipTunnelClientListMatchId(pVoipTunnel, pGameMapping->aClientIds[iIndex]);

                if (pTargetClient)
                {
                    VoipTunnelGameMappingT *pTargetGameMapping = VoipTunnelClientMatchGameIdx(pTargetClient, iGameIdx);

                    if (pTargetGameMapping->iNumTalkers < pVoipTunnel->iMaxVoiceBroadcasters)
                    {
                        pTargetGameMapping->iNumTalkers++;

                        uAcquiredSendMask |= (1 << iIndex);
                        NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: 0x%08x is now talking to 0x%08x\n", pClient->uClientId, pTargetClient->uClientId));
                    }
                    else
                    {
                        // we've just reached the maximum number of clients allowed to send, send an event and set the flag
                        if (!(pTargetClient->uFlags & VOIPTUNNEL_CLIENTFLAG_MAX_VOICES_REACHED))
                        {
                            pTargetClient->uFlags |= VOIPTUNNEL_CLIENTFLAG_MAX_VOICES_REACHED;
                            pVoipTunnel->uVoiceMaxTalkersMetric++;
                            _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_MAXDVOICE, iGameIdx, pTargetClient, 0, NetTick());

                            NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: 0x%08x is not taking new talkers  (will not get voice from 0x%08x)\n", pTargetClient->uClientId, pClient->uClientId));
                        }

                        pVoipTunnel->uVoiceDataDropMetric++;
                        pGame->VoipSessionInfo.uVoiceDataDropMetric++;
                    }
                }
            }
        }
    }

    return(uAcquiredSendMask);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelReleaseAllVoipBroadcastingSlots

    \Description
        Releases all broadcasting slots that the client possesses as an originator.

    \Input *pVoipTunnel     - module state
    \Input *pClient         - client ref
    \Input iGameIdx         - index of game in voiptunnel game list

    \Notes
        voice squelching: There are only 4 broadcasting slot per consumer clients.
        Only 4 originator clients at a time can send voip to a specific
        consumer client.

        $$TODO MCLOUATRE DECEMBER 2020 - should this take into account ALL games
        that the client is involved in?

    \Version 01/15/2010 (cvienneau)
*/
/********************************************************************************F*/
static void _VoipTunnelReleaseAllVoipBroadcastingSlots(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient, int32_t iGameIdx)
{
    int32_t iClientIndex;
    VoipTunnelGameMappingT *pGameMapping = VoipTunnelClientMatchGameId(pVoipTunnel, pClient, pVoipTunnel->pGameList[iGameIdx].uGameId);

    for (iClientIndex = 0; iClientIndex < VOIPTUNNEL_MAXGROUPSIZE; iClientIndex++)
    {
        // if this is a client we're talking to
        if ((pGameMapping->uTrafficVoipSendMask) & (1 << iClientIndex))
        {
            _VoipTunnelReleaseVoipBroadcastingSlotForTargetClient(pVoipTunnel, pClient, iGameIdx, iClientIndex);
        }
    }
}
 \
/*F********************************************************************************/
/*!
    \Function _VoipTunnelRouteVoipPacket
 
    \Description
        Routes incoming VoIP packet to appropriate destination(s).

    \Input *pVoipTunnel - voip tunnel ref
    \Input *pClient     - source client
    \Input iGameIdx     - index of game in voiptunnel game list
    \Input *pPacket     - voip packet
    \Input iPacketSize  - size of packet
    \Input *pRecvAddr   - source address
    \Input uCurTick     - current tick
    \Input bVdpHeader   - TRUE if there was a VDP header, else FALSE

    \Version 03/27/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTunnelRouteVoipPacket(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient, int32_t iGameIdx, const VoipTunnelPacketT *pPacket, int32_t iPacketSize, struct sockaddr *pRecvAddr, uint32_t uCurTick, uint32_t bVdpHeader)
{
    uint32_t uSendFlag = (bVdpHeader) ? VOIPTUNNEL_GAMEFLAG_VDP : 0;

    // see if we have a packet type that is unicast
    if (!memcmp(pPacket->Head.aType, "CO", 2) || !memcmp(pPacket->Head.aType, "PNG", 3) || !memcmp(pPacket->Head.aType, "DSC", 3))
    {
        uint32_t uRemoteClientId;

        // extract voip packet version from connection packet if we haven't done so already
        if ((pPacket->Head.aType[0] == 'C') && (pClient->uPacketVersion == 0))
        {
            pClient->uPacketVersion = pPacket->Head.aType[2];
            NetPrintf(("voiptunnel: detected voip packet format '%c' from clientId=0x%08x\n", (char)pClient->uPacketVersion, pClient->uClientId));
        }
        
        // extract the remote client identifier
        uRemoteClientId = pPacket->aRemoteClientId[0] << 24;
        uRemoteClientId |= pPacket->aRemoteClientId[1] << 16;
        uRemoteClientId |= pPacket->aRemoteClientId[2] << 8;
        uRemoteClientId |= pPacket->aRemoteClientId[3];

        NetPrintfVerbose((pVoipTunnel->uDebugLevel, pPacket->Head.aType[0] == 'P' ? 2 : 1, "voiptunnel: forwarding %d byte unicast packet of type %c%c%c from clientId=0x%08x to clientId=0x%08x\n",
            iPacketSize, pPacket->Head.aType[0], pPacket->Head.aType[1], pPacket->Head.aType[2],
            pClient->uClientId, uRemoteClientId));
        
        // forward to other clients in our group
        _VoipTunnelClientListSend(pVoipTunnel, pClient, uRemoteClientId, iGameIdx, (const char *)pPacket, iPacketSize, pRecvAddr, uSendFlag|VOIPTUNNEL_GAMEFLAG_SEND_SINGLE);
    }
    else if (!memcmp(pPacket->Head.aType, "MIC", 3))
    {
        uint32_t uSendMask = (uint32_t)-1;
        VoipTunnelGameMappingT *pGameMapping;

        // update recv voice timestamp and mark that we are receiving voip mic data from this client
        pClient->uLastRecvVoice = uCurTick;
        if ((pClient->uFlags & VOIPTUNNEL_CLIENTFLAG_RECVVOICE) == 0)
        {
            pClient->uFlags |= VOIPTUNNEL_CLIENTFLAG_RECVVOICE;
            pVoipTunnel->iNumTalkingClients++;
            pVoipTunnel->pGameList[iGameIdx].VoipSessionInfo.iNumTalkingClients++;
        }


        // extract send mask if packet version supports it
        if (pClient->uPacketVersion >= (unsigned)'c')
        {
            uSendMask = pPacket->aRemoteClientId[0] << 24;
            uSendMask |= pPacket->aRemoteClientId[1] << 16;
            uSendMask |= pPacket->aRemoteClientId[2] << 8;
            uSendMask |= pPacket->aRemoteClientId[3];
        }

        pGameMapping = VoipTunnelClientMatchGameIdx(pClient, iGameIdx);

        // update the client we have permission to send to
        pGameMapping->uTrafficVoipSendMask = _VoipTunnelAcquireVoipBroadcastingSlot(pVoipTunnel, pClient, iGameIdx, uSendMask);

        if (pGameMapping->uTrafficVoipSendMask != 0)
        {
            NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: broadcasting voip packet %d from clientId=0x%08x\n", pPacket->aRemoteClientId[0], pClient->uClientId));

            // broadcast to all other clients in our group
            _VoipTunnelClientListSend(pVoipTunnel, pClient, 0, iGameIdx, (const char *)pPacket, iPacketSize, pRecvAddr, uSendFlag|VOIPTUNNEL_GAMEFLAG_BROADCAST_ALLOTHERS); 
        }
        else
        {
            NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: failed to acquire voip broadcasting slot for voip packet %d from clientId=0x%08x\n", pPacket->aRemoteClientId[0], pClient->uClientId));
        }
    }
    else
    {
        NetPrintf(("voiptunnel: unknown VoIP packet type; ignoring\n"));
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelVoipRecvCallback
 
    \Description
        Callback handler for data received on the virtual Voip socket.

    \Input *pSocket     - pointer to socket
    \Input iFlags       - unused
    \Input *_pRef       - voiptunnel ref

    \Output
        int32_t         - zero

    \Version 03/27/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTunnelVoipRecvCallback(SocketT *pSocket, int32_t iFlags, void *_pRef)
{
    VoipTunnelRefT *pVoipTunnel = (VoipTunnelRefT *)_pRef;
    int32_t iAddrLen = sizeof(struct sockaddr), iRecvLen, iGameIdx;
    char aPacketData[SOCKET_MAXUDPRECV];
    const VoipTunnelPacketT *pPacket;
    VoipTunnelClientT *pClient;
    struct sockaddr RecvAddr;
    uint32_t uClientId, uGameId, bVdpHeader = 0;
    uint32_t uCurTick = NetTick();

    // got any input?
    if ((iRecvLen = SocketRecvfrom(pSocket, aPacketData, sizeof(aPacketData), 0, &RecvAddr, &iAddrLen)) <= 0)
    {
        return(0);
    }

    // validate the packet
    if ((pPacket = _VoipTunnelValidatePacket(pVoipTunnel, aPacketData, iRecvLen, &bVdpHeader)) == NULL)
    {
        return(0);
    }

    // extract client identifier from voip packet header
    uClientId = _VoipTunnelGetClientId(pPacket, iRecvLen);

    // extract game identifier from voip packet header (session id field)
    uGameId = _VoipTunnelGetGameId(pPacket, iRecvLen);

    // get matching game info
    if (VoipTunnelGameListMatchId(pVoipTunnel, uGameId, &iGameIdx) == NULL)
    {
        NetPrintf(("voiptunnel: ignoring '%c%c%c' voip packet from %a:%d because clientId=0x%08x does not belong to gameId=0x%08x\n",
            pPacket->Head.aType[0], pPacket->Head.aType[1], pPacket->Head.aType[2],
            SockaddrInGetAddr(&RecvAddr), SockaddrInGetPort(&RecvAddr), uClientId, uGameId));
        return(0);
    }

    // if we don't have a client with this id yet, bail
    if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId)) == NULL)
    {
        NetPrintf(("voiptunnel: ignoring '%c%c%c' voip packet from %a:%d with unregistered id=0x%08x\n",
            pPacket->Head.aType[0], pPacket->Head.aType[1], pPacket->Head.aType[2],
            SockaddrInGetAddr(&RecvAddr), SockaddrInGetPort(&RecvAddr), uClientId));
        return(0);
    }

    // if the address isn't set yet, set it now
    if (pClient->uRemoteAddr != (unsigned)SockaddrInGetAddr(&RecvAddr))
    {
        // get source client address
        pClient->uRemoteAddr = SockaddrInGetAddr(&RecvAddr);
        NetPrintf(("voiptunnel: matching clientId=0x%08x to addr %a\n", pClient->uClientId, pClient->uRemoteAddr));
        _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_MATCHADDR, iGameIdx, pClient, 0, uCurTick);
    }
    // if the voip port isn't set yet, set it now
    if (pClient->uRemoteVoipPort != SockaddrInGetPort(&RecvAddr))
    {
        // get source client port
        pClient->uRemoteVoipPort = SockaddrInGetPort(&RecvAddr);
        NetPrintf(("voiptunnel: matching clientId=0x%08x to port %d\n", pClient->uClientId, pClient->uRemoteVoipPort));
        _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_MATCHPORT, iGameIdx, pClient, 0, uCurTick);
    }

    /* call user callback and pass the kernel-applied tick as an input when available
       unix: SockaddrInGetMisc(&RecvAddr) returns kernel-applied received timestamp
       windows: SockaddrInGetMisc(&RecvAddr) returns dirtynet-applied received timestamp */
    _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_RECVVOICE, iGameIdx, pClient, iRecvLen, SockaddrInGetMisc(&RecvAddr));

    // forward voip data to others in our group according to packet type
    _VoipTunnelRouteVoipPacket(pVoipTunnel, pClient, iGameIdx, pPacket, iRecvLen, &RecvAddr, uCurTick, bVdpHeader);

    // update source client timestamp
    pClient->uLastUpdate = uCurTick;
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelPrintGameClientList
 
    \Description
        Prints the clients in the game

    \Input *pVoipTunnel     - voiptunnel ref
    \Input iGameIdx         - index of game print clients from

    \Version 01/28/2016 (eesponda)
*/
/********************************************************************************F*/
static void _VoipTunnelPrintGameClientList(VoipTunnelRefT *pVoipTunnel, int32_t iGameIdx)
{
    int32_t iClient;
    const VoipTunnelGameT *pGame = &pVoipTunnel->pGameList[iGameIdx];

    NetPrintf(("voiptunnel: game %d clients:\n", iGameIdx));
    for (iClient = 0; iClient < VOIPTUNNEL_MAXGROUPSIZE; iClient++)
    {
        if (pGame->aClientList[iClient] != 0)
        {
            NetPrintf(("voiptunnel: [%d] 0x%08x\n", iClient, pGame->aClientList[iClient]));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelPrintClientGameList

    \Description
        Prints the games the specified client belongs to

    \Input *pVoipTunnel     - voiptunnel ref
    \Input *pClient         - client

    \Version 12/15/2020 (mclouatre)
*/
/********************************************************************************F*/
static void _VoipTunnelPrintClientGameList(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient)
{
    char strGameList[128];
    int32_t iOffset, iGameMappingIdx;
    uint32_t bNone = TRUE;

    for (iGameMappingIdx = 0, iOffset = 0; (iGameMappingIdx < VOIPTUNNEL_MAX_GAMES_PER_CLIENT) && ((sizeof(strGameList) - iOffset) > 0); iGameMappingIdx++)
    {
        if (pClient->aGameMappings[iGameMappingIdx].iGameIdx != -1)
        {
            iOffset += ds_snzprintf(&strGameList[0] + iOffset, sizeof(strGameList) - iOffset, "[id=0x%08x/idx=%04d] ",
                pVoipTunnel->pGameList[pClient->aGameMappings[iGameMappingIdx].iGameIdx].uGameId,
                pClient->aGameMappings[iGameMappingIdx].iGameIdx);
            bNone = FALSE;
        }
    }

    if (bNone)
    {
        NetPrintf(("voiptunnel: clientId=0x%08x belongs to game(s): - none -\n", pClient->uClientId));
    }
    else
    {
        NetPrintf(("voiptunnel: clientId=0x%08x belongs to game(s): %s\n", pClient->uClientId, strGameList));
    }
}


/*F********************************************************************************/
/*!
    \Function _VoipTunnelClientGameDel
 
    \Description
        Remove client from the game list

    \Input *pVoipTunnel     - voiptunnel ref
    \Input *pClient         - client to remove
    \Input iGameIdx         - index of game to remove client from

    \Output
        int32_t             - negative=error, else success
 
    \Version 06/10/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _VoipTunnelClientGameDel(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient, int32_t iGameIdx)
{
    // remove entry from game list
    VoipTunnelGameT *pGame = &pVoipTunnel->pGameList[iGameIdx];
    int32_t iGameClient;
    
    // scan array to find client id
    for (iGameClient = 0; iGameClient < VOIPTUNNEL_MAXGROUPSIZE; iGameClient++)
    {
        // found client?
        if (pGame->aClientList[iGameClient] == pClient->uClientId)
        {
            VoipTunnelGameMappingT *pGameMapping;

            NetPrintf(("voiptunnel: removing clientId=0x%08x from game %d\n", pClient->uClientId, iGameIdx));

            // if the user was talking as they left, free up that broadcasting slot
            _VoipTunnelReleaseAllVoipBroadcastingSlots(pVoipTunnel, pClient, iGameIdx);

            // expire recvvoice flag?
            if (pClient->uFlags & VOIPTUNNEL_CLIENTFLAG_RECVVOICE)
            {
                pClient->uFlags &= ~VOIPTUNNEL_CLIENTFLAG_RECVVOICE;
                pVoipTunnel->iNumTalkingClients--;
                pVoipTunnel->pGameList[iGameIdx].VoipSessionInfo.iNumTalkingClients--;

                // catch a potential error if this number goes below 0.
                // this occured in GOS-31044 so we place this safeguard here 
                // if this occurs the metric can no longer be relied upon to be 
                // accurate but should still be close to the appropriate value, 
                // rather than negative, which will be problematic for the dashboard.
                if (pVoipTunnel->iNumTalkingClients < 0)
                {
                    NetPrintf(("voiptunnel: [%p] error tracking number of talking clients, game %d\n", pVoipTunnel, iGameIdx));
                    pVoipTunnel->iNumTalkingClients = 0;
                }
            }

            // clear entry from game list
            pGame->aClientList[iGameClient] = 0;

            // decrement client count
            pGame->iNumClients -= 1;
            // debug echo of game client list after delete
            if (pVoipTunnel->uDebugLevel > 0)
            {
                _VoipTunnelPrintGameClientList(pVoipTunnel, iGameIdx);
            }
            // update send masks for other clients in this game
            for (iGameClient = 0; iGameClient < VOIPTUNNEL_MAXGROUPSIZE; iGameClient++)
            {
                VoipTunnelClientT *pOpponentClient;

                // make sure the client is valid
                if (pGame->aClientList[iGameClient] == 0)
                {
                    continue;
                }

                // ref the client
                if ((pOpponentClient = VoipTunnelClientListMatchId(pVoipTunnel, pGame->aClientList[iGameClient])) != NULL)
                {
                    // refresh send mask
                    VoipTunnelClientRefreshSendMask(pVoipTunnel, pOpponentClient, iGameIdx);
                }
            }

            // invalidate the corresponding client-to-game mapping
            pGameMapping = VoipTunnelClientMatchGameIdx(pClient, iGameIdx);
            pGameMapping->iGameIdx = -1;

            return(0);
        }
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelSocketOpen
 
    \Description
        Open a DirtySock socket.

    \Input *pVoipTunnel - module state
    \Input uPort        - port to bind
    \Input *pCallback   - socket callback function

    \Output
        SocketT *       - socket ref, or NULL
 
    \Version 03/27/2006 (jbrookes)
*/
/********************************************************************************F*/
static SocketT *_VoipTunnelSocketOpen(VoipTunnelRefT *pVoipTunnel, uint16_t uPort, int32_t (*pCallback)(SocketT *pSocket, int32_t iFlags, void *pRef))
{
    struct sockaddr BindAddr;
    SocketT *pSocket;
    int32_t iResult;

    // open the socket
    if ((pSocket = SocketOpen(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == NULL)
    {
        NetPrintf(("voiptunnel: unable to open socket\n"));
        return(NULL);
    }

    // bind socket to specified port
    SockaddrInit(&BindAddr, AF_INET);
    SockaddrInSetPort(&BindAddr, uPort);
    if ((iResult = SocketBind(pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
    {
        NetPrintf(("voiptunnel: error %d binding to port\n", iResult));
        SocketClose(pSocket);
        return(NULL);
    }

    // set up for socket callback events
    SocketCallback(pSocket, CALLB_RECV, 100, pVoipTunnel, pCallback);

    // return ref to caller
    return(pSocket);
}

/*F********************************************************************************/
/*!
    \Function _VoipTunnelClientListUpdate

    \Description
        Perform update processing on client list

    \Input *pVoipTunnel - module state

    \Version 09/18/2006 (jbrookes)
*/
/********************************************************************************F*/
static void _VoipTunnelClientListUpdate(VoipTunnelRefT *pVoipTunnel)
{
    uint32_t uCurTick;
    int32_t iClientIdx, iGameMappingIdx;

    // loop through clientlist
    for (iClientIdx = 0, uCurTick = NetTick(); iClientIdx < pVoipTunnel->iNumClients; iClientIdx++)
    {
        // loop through all games this client belongs to
        for (iGameMappingIdx = 0; iGameMappingIdx < VOIPTUNNEL_MAX_GAMES_PER_CLIENT; iGameMappingIdx++)
        {
            VoipTunnelGameT *pGame;
            VoipTunnelClientT *pClient = &pVoipTunnel->ClientList[iClientIdx];

            // make sure the game mapping is valid
            if (pClient->aGameMappings[iGameMappingIdx].iGameIdx == -1)
            {
                continue;
            }

            pGame = &pVoipTunnel->pGameList[pClient->aGameMappings[iGameMappingIdx].iGameIdx];

            // expire recvvoice flag?
            if (pClient->uFlags & VOIPTUNNEL_CLIENTFLAG_RECVVOICE)
            {
                if (NetTickDiff(uCurTick, pClient->uLastRecvVoice) > (int32_t)pVoipTunnel->uVoiceRecvTimeout)
                {
                    NetPrintfVerbose((pVoipTunnel->uDebugLevel, 1, "voiptunnel: stopped receiving voice from clientId=0x%08x\n", pClient->uClientId));
                    pClient->uFlags &= ~VOIPTUNNEL_CLIENTFLAG_RECVVOICE;
                    pVoipTunnel->iNumTalkingClients--;
                    pGame->VoipSessionInfo.iNumTalkingClients--;
                    _VoipTunnelReleaseAllVoipBroadcastingSlots(pVoipTunnel, pClient, pClient->aGameMappings[iGameMappingIdx].iGameIdx);

                    // catch a potential error if this number goes below 0.
                    // this occured in GOS-31044 so we place this safeguard here 
                    // if this occurs the metric can no longer be relied upon to be 
                    // accurate but should still be close to the appropriate value, 
                    // rather than negative, which will be problematic for the dashboard.
                    if (pVoipTunnel->iNumTalkingClients < 0)
                    {
                        NetPrintf(("voiptunnel: [%p] error tracking number of talking clients\n", pVoipTunnel));
                        pVoipTunnel->iNumTalkingClients = 0;
                    }
                }
            }

            /* check for connection death (have previously received data, but have not received
               data for at least the timeout period.  timeout is client timeout plus five seconds
               so that clients that are being removed from the game don't produce this warning */
            if ((pClient->uRemoteVoipPort != 0) && ((signed)(uCurTick - pClient->uLastUpdate) > 15 * 1000))
            {
                NetPrintf(("voiptunnel: clientId=0x%08x voice connection has gone dead\n", pClient->uClientId));
                /* clear voip point so 1) we don't send to them 2) we don't warn again and 3) in
                   the unlikely event data starts flowing again somehow, we get notified by a new
                   match event */
                pClient->uRemoteVoipPort = 0;
                _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_DEADVOICE, pClient->aGameMappings[iGameMappingIdx].iGameIdx, pClient, 0, uCurTick);
            }
        }
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function VoipTunnelCreate

    \Description
        Create the VoipTunnel module.

    \Input uVoipPort        - local virtual voip port
    \Input iMaxClients      - max number of clients allowed
    \Input iMaxGames        - max number of games allowed

    \Output
        VoipTunnelRefT *    - new module state pointer, or null

    \Version 04/06/2006 (jbrookes)
*/
/********************************************************************************F*/
VoipTunnelRefT *VoipTunnelCreate(uint32_t uVoipPort, int32_t iMaxClients, int32_t iMaxGames)
{
    VoipTunnelRefT *pVoipTunnel;
    int32_t iMemSize = sizeof(*pVoipTunnel) + (sizeof(pVoipTunnel->ClientList[0]) * (iMaxClients - 1));
    int32_t iGameListSize;
    int32_t iMemGroup;
    void *pMemGroupUserData;

    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // allocate and init module state    
    if ((pVoipTunnel = DirtyMemAlloc(iMemSize, VOIPTUNNEL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voiptunnel: could not allocate module state\n"));
        return(NULL);
    }
    ds_memclr(pVoipTunnel, iMemSize);
    pVoipTunnel->iMemGroup = iMemGroup;
    pVoipTunnel->pMemGroupUserData = pMemGroupUserData;
    pVoipTunnel->uVoipPort = uVoipPort;
    pVoipTunnel->iMaxClients = iMaxClients;
    pVoipTunnel->iMaxGames = iMaxGames;
    pVoipTunnel->iMaxVoiceBroadcasters = VOIPTUNNEL_MAX_BROADCASTING_VOICES_DEFAULT;
    pVoipTunnel->uVoiceRecvTimeout = VOIPTUNNEL_RECVVOICE_TIMEOUT_DEFAULT;

    // test lookup table
    #if VOIPTUNNEL_LOOKUP_TEST
    _VoipTunnelLookupTest(pVoipTunnel);
    #endif

    // allocate game list
    iGameListSize = sizeof(*pVoipTunnel->pGameList) * iMaxGames;
    if ((pVoipTunnel->pGameList = DirtyMemAlloc(iGameListSize, VOIPTUNNEL_MEMID, pVoipTunnel->iMemGroup, pVoipTunnel->pMemGroupUserData)) == NULL)
    {
        NetPrintf(("voiptunnel: unable to allocate game list\n"));
        // destroy anything that was started
        VoipTunnelDestroy(pVoipTunnel);
        return(NULL);
    }
    ds_memset(pVoipTunnel->pGameList, -1, iGameListSize);

    // allocate fast lookup table if warranted
    if (iMaxClients >= 32)
    {
        if ((pVoipTunnel->pLookupTable = DirtyMemAlloc(sizeof(*pVoipTunnel->pLookupTable) * iMaxClients, VOIPTUNNEL_MEMID, pVoipTunnel->iMemGroup, pVoipTunnel->pMemGroupUserData)) == NULL)
        {
            NetPrintf(("voiptunnel: unable to allocate fast lookup table\n"));
        }
    }
    
    // create voip socket
    if ((pVoipTunnel->pVoipSocket = _VoipTunnelSocketOpen(pVoipTunnel, uVoipPort, _VoipTunnelVoipRecvCallback)) == NULL)
    {
        // destroy anything that was started
        VoipTunnelDestroy(pVoipTunnel);
        return(NULL);
    }

    // return ref to caller
    return(pVoipTunnel);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelCallback

    \Description
        Set optional voiptunnel event callback

    \Input *pVoipTunnel - voiptunnel ref
    \Input *pCallback   - callback pointer
    \Input *pUserData   - callback user data

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void VoipTunnelCallback(VoipTunnelRefT *pVoipTunnel, VoipTunnelCallbackT *pCallback, void *pUserData)
{
    pVoipTunnel->pCallback = pCallback;
    pVoipTunnel->pUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelDestroy

    \Description
        Shutdown this VoipTunnel.

    \Input *pVoipTunnel - voiptunnel ref

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void VoipTunnelDestroy(VoipTunnelRefT *pVoipTunnel)
{
    // destroy lookup table
    if (pVoipTunnel->pLookupTable != NULL)
    {
        DirtyMemFree(pVoipTunnel->pLookupTable, VOIPTUNNEL_MEMID, pVoipTunnel->iMemGroup, pVoipTunnel->pMemGroupUserData);
    }
    // destroy game list
    if (pVoipTunnel->pGameList != NULL)
    {
        DirtyMemFree(pVoipTunnel->pGameList, VOIPTUNNEL_MEMID, pVoipTunnel->iMemGroup, pVoipTunnel->pMemGroupUserData);
    }
    // destroy virtual voip socket
    if (pVoipTunnel->pVoipSocket != NULL)
    {
        SocketClose(pVoipTunnel->pVoipSocket);
    }
    // dispose of module memory
    DirtyMemFree(pVoipTunnel, VOIPTUNNEL_MEMID, pVoipTunnel->iMemGroup, pVoipTunnel->pMemGroupUserData);
}


/*F********************************************************************************/
/*!
    \Function VoipTunnelClientListAdd

    \Description
        Add a new client to the client list

    \Input *pVoipTunnel     - voiptunnel ref, at the first empty spot past a given index
    \Input uClientId        - client id
    \Input iGameIdx         - index of game in voiptunnel game list
    \Input *pTunnelKey      - tunnel key needed to create tunnel to this client
    \Input iClientListIdx   - index to add at in the client list
    \Input **ppNewClient    - [out] new client pointer (optional; can be NULL)

    \Output
        int32_t             - negative=error, else success

    \Version 12/03/2020 (mclouatre)
*/
/********************************************************************************F*/
int32_t VoipTunnelClientListAdd(VoipTunnelRefT *pVoipTunnel, uint32_t uClientId, int32_t iGameIdx, const char *pTunnelKey, int32_t iClientListIdx, VoipTunnelClientT **ppNewClient)
{
    VoipTunnelClientT *pClient = NULL;
    VoipTunnelGameT *pGame;
    int32_t iGameClientIdx;
    int32_t iGameMappingIdx = 0;
    uint32_t bNewClient = FALSE;

    // make sure there's room in the list
    if (pVoipTunnel->iNumClients >= pVoipTunnel->iMaxClients)
    {
        NetPrintf(("voiptunnel: adding clientId=0x%08x to global clientlist - max client limit exceeded (%d)\n", 
            uClientId, pVoipTunnel->iMaxClients));
        return(-1);
    }

    // make sure game index is valid
    if (iGameIdx >= pVoipTunnel->iMaxGames)
    {
        NetPrintf(("voiptunnel: adding clientId=0x%08x to global clientlist - invalid game index %d\n", 
            uClientId, iGameIdx));
        return(-2);
    }

    // make sure game has been initialized and there's room in the game
    pGame = &pVoipTunnel->pGameList[iGameIdx];
    if (pGame->iNumClients < 0)
    {
        NetPrintf(("voiptunnel: adding clientId=0x%08x to global clientlist - game %d not properly initialized\n", 
            uClientId, iGameIdx));
        return(-3);
    }
    if (pGame->iNumClients >= VOIPTUNNEL_MAXGROUPSIZE)
    {
        NetPrintf(("voiptunnel: adding clientId=0x%08x to global clientlist - max game size (%d ) exceeded for game %d\n", 
            uClientId, VOIPTUNNEL_MAXGROUPSIZE, iGameIdx));
        return(-4);
    }

    // make sure client does not already appear in the game list
    for (iGameClientIdx = 0; iGameClientIdx < VOIPTUNNEL_MAXGROUPSIZE; iGameClientIdx++)
    {
        if (uClientId == pGame->aClientList[iGameClientIdx])
        {
            NetPrintf(("voiptunnel: adding clientId=0x%08x to global clientlist - client is already in game index %d at client index %d\n", 
                uClientId, iGameIdx, iGameClientIdx));
            return(-5);
        }
    }
 
    // validate client list index
    if ((iClientListIdx >= VOIPTUNNEL_MAXGROUPSIZE) || (pGame->aClientList[iClientListIdx] != 0))
    {
        NetPrintf(("voiptunnel: could not add clientId=0x%08x to game list at index %d for game %d\n",
            uClientId, iClientListIdx, iGameIdx));
        return(-6);
    }

    // check for clients that are already tracked by the voiptunnel
    if ((pClient = VoipTunnelClientListMatchId(pVoipTunnel, uClientId)) == NULL)
    {
        // allocate new entry
        pClient = &pVoipTunnel->ClientList[pVoipTunnel->iNumClients];
        ds_memclr(pClient, sizeof(*pClient));
        bNewClient = TRUE;

        // initialize game mappings
        for (iGameMappingIdx = 0; iGameMappingIdx < VOIPTUNNEL_MAX_GAMES_PER_CLIENT; iGameMappingIdx++)
        {
            pClient->aGameMappings[iGameMappingIdx].iGameIdx = -1;
        }
    }

    // find a free spot to store the client-to-game mapping 
    for (iGameMappingIdx = 0; iGameMappingIdx < VOIPTUNNEL_MAX_GAMES_PER_CLIENT; iGameMappingIdx++)
    {
        if (pClient->aGameMappings[iGameMappingIdx].iGameIdx == -1)
        {
            break;
        }
    }
    if (iGameMappingIdx != VOIPTUNNEL_MAX_GAMES_PER_CLIENT)
    {
        if (bNewClient)
        {
            pClient->uClientId = uClientId;
            pVoipTunnel->iNumClients += 1;

            NetPrintf(("voiptunnel: successfully added clientId=0x%08x to global clientlist (client count = %d)\n", uClientId, pVoipTunnel->iNumClients));

            _VoipTunnelLookupBuild(pVoipTunnel);
        }
        else
        {
            NetPrintf(("voiptunnel: clientId=0x%08x now belongs to more than one game\n", uClientId, pVoipTunnel->iNumClients));
        }

        // add client-to-game mapping in spot identified ealier
        pClient->aGameMappings[iGameMappingIdx].iGameIdx = iGameIdx;
        pClient->aGameMappings[iGameMappingIdx].uTrafficVoipSendMask = 0;     // at every packet received, this will be recomputed
        pClient->aGameMappings[iGameMappingIdx].iNumTalkers = 0;
        ds_strnzcpy(pClient->aGameMappings[iGameMappingIdx].strTunnelKey, pTunnelKey, sizeof(pClient->aGameMappings[iGameMappingIdx].strTunnelKey));

        // add client to game's client list
        pGame->aClientList[iClientListIdx] = pClient->uClientId;
        pGame->iNumClients += 1;
    }
    else
    {
        NetPrintf(("voiptunnel: adding clientId=0x%08x to global clientlist - max game mappings per client reached (%d)\n",
            uClientId, VOIPTUNNEL_MAX_GAMES_PER_CLIENT));
        return(-7);
    }

    // debug echo of game client list after add
    if (pVoipTunnel->uDebugLevel > 0)
    {
        // print game's client list
        _VoipTunnelPrintGameClientList(pVoipTunnel, pClient->aGameMappings[iGameMappingIdx].iGameIdx);

        // print games to which the client belongs
        _VoipTunnelPrintClientGameList(pVoipTunnel, pClient);
    }

    // notify application of add
    _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_ADDCLIENT, iGameIdx, pClient, 0, NetTick());

    // write back new client pointer
    if (ppNewClient != NULL)
    {
        *ppNewClient = pClient;
    }

    // return success
    return(0);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientListDel

    \Description
        Remove client from the client list given the context of a game

    \Input *pVoipTunnel     - voiptunnel ref
    \Input *pClient         - client to remove
    \Input iGameIdx         - index of game in voiptunnel game list (game we are removing from)

    \Version 04/10/2006 (jbrookes)
*/
/********************************************************************************F*/
void VoipTunnelClientListDel(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient, int32_t iGameIdx)
{
    int32_t iClientIdx = (int32_t)(pClient - pVoipTunnel->ClientList);

    // make sure client index is valid
    if ((iClientIdx < 0) || (iClientIdx >= pVoipTunnel->iMaxClients))
    {
        NetPrintf(("voiptunnel: deleting clientId=0x%08x from global client list - invalid client index %d\n",
            pClient->uClientId, iClientIdx));
        return;
    }

    // make sure game index is valid
    if (iGameIdx < 0)
    {
        NetPrintf(("voiptunnel: deleting clientId=0x%08x from global client list - invalid game idx %d\n",
            pClient->uClientId, iGameIdx));
        return;
    }

    // notify application of upcoming deletion to allow for deletion of any associate resources
    _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_DELCLIENT, iGameIdx, pClient, 0, NetTick());

    // remove entry from game
    if (_VoipTunnelClientGameDel(pVoipTunnel, pClient, iGameIdx) >= 0)
    {
        NetPrintf(("voiptunnel: successfully deleted clientId=0x%08x from game idx %d\n",
            pClient->uClientId, iGameIdx));
    }
    else
    {
        NetPrintf(("voiptunnel: deleting clientId=0x%08x from game idx %d - could not find client\n",
            pClient->uClientId, iGameIdx));
    }

    if (VoipTunnelIsClientInGame(pClient) == FALSE)
    {
#if DIRTYCODE_LOGGING
        uint32_t uSavedClientId = pClient->uClientId;
#endif

        // remove entry from client list
        if (iClientIdx != (pVoipTunnel->iNumClients - 1))
        {
            int32_t iNumMoveClients = (pVoipTunnel->iNumClients - 1) - iClientIdx;

            // move the clients to remove the gap
            memmove(pClient, pClient + 1, iNumMoveClients * sizeof(VoipTunnelClientT));
        }

        // decrement count
        pVoipTunnel->iNumClients -= 1;

        // rebuild lookup table
        _VoipTunnelLookupBuild(pVoipTunnel);

        // make sure to print this 'before' invalidating pClient
        NetPrintf(("voiptunnel: successfully removed clientId=0x%08x from global clientlist (client count = %d)\n", uSavedClientId, pVoipTunnel->iNumClients));
    }

    // print games the client is still in
    _VoipTunnelPrintClientGameList(pVoipTunnel, pClient);
}


/*F********************************************************************************/
/*!
    \Function VoipTunnelGameListAdd
 
    \Description
        Add a new game.

    \Input *pVoipTunnel - voiptunnel ref
    \Input iGameIdx     - index of game whose clients are to be removed

    \Output
        int32_t         - negative=error, else success
 
    \Version 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTunnelGameListAdd(VoipTunnelRefT *pVoipTunnel, int32_t iGameIdx)
{
    return(VoipTunnelGameListAdd2(pVoipTunnel, iGameIdx, 0));
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelGameListAdd2
 
    \Description
        Add a new game.

    \Input *pVoipTunnel - voiptunnel ref
    \Input iGameIdx     - index of game whose clients are to be removed
    \Input uGameId      - unique game identifier

    \Output
        int32_t         - negative=error, else success

    \Version 12/01/2015 (eesponda)
*/
/********************************************************************************F*/
int32_t VoipTunnelGameListAdd2(VoipTunnelRefT *pVoipTunnel, int32_t iGameIdx, uint32_t uGameId)
{
    if (pVoipTunnel->iNumGames >= pVoipTunnel->iMaxGames)
    {
        NetPrintf(("voiptunnel: game list overflow -- could not add game\n"));
        return(-1);
    }
    if (pVoipTunnel->pGameList[iGameIdx].iNumClients != -1)
    {
        NetPrintf(("voiptunnel: refusing game %d add request; game is already active\n", iGameIdx));
        return(-2);
    }
    pVoipTunnel->iNumGames++;
    ds_memclr(&pVoipTunnel->pGameList[iGameIdx], sizeof(pVoipTunnel->pGameList[0]));
    pVoipTunnel->pGameList[iGameIdx].uGameId = uGameId;

    // notify application of add
    _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_ADDGAME, iGameIdx, NULL, 0, NetTick());

    return(0);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelGameListDel

    \Description
        Remove game at specified index in game list.

    \Input *pVoipTunnel - voiptunnel ref
    \Input iGameIdx     - index of game in voiptunnel game list (game to be removed)

    \Output
        int32_t         - number of clients deleted

    \Version 04/27/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTunnelGameListDel(VoipTunnelRefT *pVoipTunnel, int32_t iGameIdx)
{
    int32_t iGameClientListIdx;
    int32_t iClientsDeleted = 0;

    if (iGameIdx < pVoipTunnel->iMaxGames)
    {
        VoipTunnelGameT *pGame = VoipTunnelGameListMatchIndex(pVoipTunnel, iGameIdx);

        // iterate through game client list and delete all clients
        for (iGameClientListIdx = 0; iGameClientListIdx < VOIPTUNNEL_MAXGROUPSIZE; iGameClientListIdx++)
        {
            VoipTunnelClientT *pClient = NULL;

            if (pGame->aClientList[iGameClientListIdx] != 0)
            {
                pClient = _VoipTunnelLookupClient(pVoipTunnel, pGame->aClientList[iGameClientListIdx]);
            }

            if (pClient != NULL)
            {
                VoipTunnelClientListDel(pVoipTunnel, pClient, iGameIdx);
            }

            // increment deleted count
            iClientsDeleted += 1;
        }

        // notify application of upcoming deletion to allow for deletion of any associated resources
        _VoipTunnelEventCallback(pVoipTunnel, VOIPTUNNEL_EVENT_DELGAME, iGameIdx, NULL, 0, NetTick());

        // wipe game and mark as unallocated
        ds_memclr(&pVoipTunnel->pGameList[iGameIdx], sizeof(pVoipTunnel->pGameList[0]));
        pVoipTunnel->pGameList[iGameIdx].iNumClients = -1;
        pVoipTunnel->iNumGames--;
    }
    else
    {
        NetPrintf(("voiptunnel: deleting gameIdx %d - game index out of bounds\n", iGameIdx));
    }

    // return number of clients deleted
    return(iClientsDeleted);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientListMatchAddr
 
    \Description
        Return client matching given address.

    \Input *pVoipTunnel     - voiptunnel ref
    \Input uRemoteAddr      - address incoming packet originated from

    \Output
        VoipTunnelClientT * - pointer to new client, or NULL
 
    \Version 03/27/2006 (jbrookes)
*/
/********************************************************************************F*/
VoipTunnelClientT *VoipTunnelClientListMatchAddr(VoipTunnelRefT *pVoipTunnel, uint32_t uRemoteAddr)
{
    VoipTunnelClientT *pClient;
    int32_t iClientId;
    
    // iterate through client list and look for a match
    for (iClientId = 0, pClient = &pVoipTunnel->ClientList[iClientId]; iClientId < pVoipTunnel->iNumClients; iClientId++, pClient++)
    {
        // is there a match?
        if (pClient->uRemoteAddr == uRemoteAddr)
        {
            return(pClient);
        }
    }

    // no match     
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientListMatchId
 
    \Description
        Return client matching given address.

    \Input *pVoipTunnel     - voiptunnel ref
    \Input uClientId        - unique client identifier

    \Output
        VoipTunnelClientT * - pointer to matching client, or NULL
 
    \Version 03/27/2006 (jbrookes)
*/
/********************************************************************************F*/
VoipTunnelClientT *VoipTunnelClientListMatchId(VoipTunnelRefT *pVoipTunnel, uint32_t uClientId)
{
    VoipTunnelClientT *pClient;
    int32_t iClient;

    // ignore invalid clientId
    if (uClientId == 0)
    {
        return(NULL);
    }    
    
    // use lookup?
    if (pVoipTunnel->pLookupTable != NULL)
    {
        pClient = _VoipTunnelLookupClient(pVoipTunnel, uClientId);
    }
    else
    {
        // iterate through client list and look for a match
        for (iClient = 0, pClient = NULL; iClient < pVoipTunnel->iNumClients; iClient++)
        {
            // is there a match?
            if (pVoipTunnel->ClientList[iClient].uClientId == uClientId)
            {
                pClient = &pVoipTunnel->ClientList[iClient];
                break;
            }
        }
    }

    // return client to caller, or NULL if no match
    return(pClient);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientListMatchIndex
 
    \Description
        Return client at the given index.

    \Input *pVoipTunnel     - voiptunnel ref
    \Input uClientIndex     - index of client in client list

    \Output
        VoipTunnelClientT * - pointer to client, or NULL
 
    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
VoipTunnelClientT *VoipTunnelClientListMatchIndex(VoipTunnelRefT *pVoipTunnel, uint32_t uClientIndex)
{
    VoipTunnelClientT *pClient = NULL;
    if (uClientIndex < (unsigned)pVoipTunnel->iNumClients)
    {
        pClient = &pVoipTunnel->ClientList[uClientIndex];
    }
    return(pClient);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientListMatchSockaddr

    \Description
        Return client matching the address and port in the specified sockaddr

    \Input *pVoipTunnel     - voiptunnel ref
    \Input *pSockaddr       - sockaddr containing addr and port to match

    \Output
        VoipTunnelClientT * - pointer to matching client, or NULL

    \Version 03/13/2007 (jbrookes)
*/
/********************************************************************************F*/
VoipTunnelClientT *VoipTunnelClientListMatchSockaddr(VoipTunnelRefT *pVoipTunnel, struct sockaddr *pSockaddr)
{
    VoipTunnelClientT *pClient;
    int32_t iClientId;
    uint32_t uAddr = SockaddrInGetAddr(pSockaddr);
    uint16_t uPort = SockaddrInGetPort(pSockaddr);
    
    // iterate through client list and look for a match
    for (iClientId = 0, pClient = &pVoipTunnel->ClientList[iClientId]; iClientId < pVoipTunnel->iNumClients; iClientId++, pClient++)
    {
        // is there a match?
        if ((pClient->uRemoteAddr == uAddr) && (pClient->uRemoteGamePort == uPort))
        {
            return(pClient);
        }
    }

    // no match     
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientListMatchFunc
 
    \Description
        Return client matching criteria of matching function

    \Input *pVoipTunnel     - voiptunnel ref
    \Input *pMatchFunc      - function to call to indicate if we have a match or not
    \Input *pUserData       - user data to pass to match function

    \Output
        VoipTunnelClientT * - pointer to found client, or NULL
 
    \Version 04/10/2007 (jbrookes)
*/
/********************************************************************************F*/
VoipTunnelClientT *VoipTunnelClientListMatchFunc(VoipTunnelRefT *pVoipTunnel, VoipTunnelMatchFuncT *pMatchFunc, void *pUserData)
{
    VoipTunnelClientT *pClient;
    int32_t iClientId;
    
    // iterate through client list and look for a match
    for (iClientId = 0, pClient = &pVoipTunnel->ClientList[iClientId]; iClientId < pVoipTunnel->iNumClients; iClientId++, pClient++)
    {
        // is there a match?
        if (pMatchFunc(pClient, pUserData) == 0)
        {
            return(pClient);
        }
    }

    // no match     
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelGameListMatchIndex
 
    \Description
        Return game at the given index.

    \Input *pVoipTunnel     - voiptunnel ref
    \Input iGameIdx         - index of game in game list

    \Output
        VoipTunnelGameT * - pointer to game, or NULL

    \Version 12/01/2015 (eesponda)
*/
/********************************************************************************F*/
VoipTunnelGameT *VoipTunnelGameListMatchIndex(VoipTunnelRefT *pVoipTunnel, int32_t iGameIdx)
{
    VoipTunnelGameT *pGame = NULL;
    if (iGameIdx < pVoipTunnel->iMaxGames)
    {
        pGame = &pVoipTunnel->pGameList[iGameIdx];
    }
    return(pGame);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelGameListMatchId
 
    \Description
        Return game matching given game identifier.

    \Input *pVoipTunnel     - voiptunnel ref
    \Input uGameId          - unique game identifier
    \Input *pGameIdx        - [out] index the game was at

    \Output
        VoipTunnelGameT * - pointer to matching game, or NULL

    \Version 12/01/2015 (eesponda)
*/
/********************************************************************************F*/
VoipTunnelGameT *VoipTunnelGameListMatchId(VoipTunnelRefT *pVoipTunnel, uint32_t uGameId, int32_t *pGameIdx)
{
    VoipTunnelGameT *pGame;
    int32_t iGameIdx;

    // ignore invalid game id
    if (uGameId == 0)
    {
        return(NULL);
    }
    // initialize if valid
    if (pGameIdx != NULL)
    {
        *pGameIdx = -1;
    }

    // iterate through game list and look for a match
    for (iGameIdx = 0, pGame = NULL; iGameIdx < pVoipTunnel->iMaxGames; iGameIdx += 1)
    {
        if (pVoipTunnel->pGameList[iGameIdx].uGameId == uGameId)
        {
            pGame = &pVoipTunnel->pGameList[iGameIdx];
            if (pGameIdx != NULL)
            {
                *pGameIdx = iGameIdx;
            }

            break;
        }
    }

    return(pGame);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientRefreshSendMask

    \Description
        Let api know client's send list has been updated

    \Input *pVoipTunnel     - voiptunnel ref
    \Input *pClient         - client whose send list has been updated
    \Input iGameIdx         - index of game in voiptunnel game list

    \Version 06/10/2007 (jbrookes)
*/
/********************************************************************************F*/
void VoipTunnelClientRefreshSendMask(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient, int32_t iGameIdx)
{
    VoipTunnelGameT *pGame = &pVoipTunnel->pGameList[iGameIdx];
    uint32_t bInList, uGameClientId, uTopologyVoipSendMask;
    int32_t iGameClientIdx, iSendClientIdx;
    VoipTunnelGameMappingT *pGameMapping = VoipTunnelClientMatchGameIdx(pClient, iGameIdx);

    // iterate through game list and rebuild client's send mask
    for (iGameClientIdx = 0, uTopologyVoipSendMask = 0; iGameClientIdx < VOIPTUNNEL_MAXGROUPSIZE; iGameClientIdx++)
    {
        // skip empty or inactive entries
        if (pGame->aClientList[iGameClientIdx] == 0)
        {
            continue;
        }
        // iterate through client's send list
        for (bInList = FALSE, iSendClientIdx = 0, uGameClientId = pGame->aClientList[iGameClientIdx]; iSendClientIdx < VOIPTUNNEL_MAXGROUPSIZE; iSendClientIdx++)
        {
            if (uGameClientId == (unsigned)pGameMapping->aClientIds[iSendClientIdx])
            {
                bInList = TRUE;
                break;
            }
        }
        // update game send mask
        uTopologyVoipSendMask |= bInList << iGameClientIdx;
    }
    // write back updated mask
    NetPrintf(("voiptunnel: setting clientId=0x%08x topology voip send mask to 0x%08x for game idx %d\n", pClient->uClientId, uTopologyVoipSendMask, iGameIdx));
    pGameMapping->uTopologyVoipSendMask = uTopologyVoipSendMask;
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientMatchGameId

    \Description
        Returns a valid 'client-to-game mapping' if the specified client is involved in
        the game identified by the game id, otherwise return NULL

    \Input *pVoipTunnel         - voiptunnel ref
    \Input *pClient             - client
    \Input uGameId              - unique game identifier

    \Output
        VoipTunnelGameMappingT* - NULL if not found

    \Version 12/06/2020 (mclouatre)
*/
/********************************************************************************F*/
VoipTunnelGameMappingT *VoipTunnelClientMatchGameId(VoipTunnelRefT *pVoipTunnel, VoipTunnelClientT *pClient, uint32_t uGameId)
{
    int32_t iGameMappingIdx;

    for (iGameMappingIdx = 0; iGameMappingIdx < VOIPTUNNEL_MAX_GAMES_PER_CLIENT; iGameMappingIdx++)
    {
        if (pClient->aGameMappings[iGameMappingIdx].iGameIdx != -1)
        {
            if (uGameId == pVoipTunnel->pGameList[pClient->aGameMappings[iGameMappingIdx].iGameIdx].uGameId)
            {
                return(&pClient->aGameMappings[iGameMappingIdx]);
            }
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelClientMatchGameIdx

    \Description
        Returns a valid 'client-to-game mapping' if the specified client is involved in
        the game identified by the voiptunnel game index, otherwise return NULL

    \Input *pClient             - client
    \Input iGameIdx             - index of game in voiptunnel game list

    \Output
        VoipTunnelGameMappingT* - NULL if not found

    \Version 12/06/2006 (mclouatre)
*/
/********************************************************************************F*/
VoipTunnelGameMappingT *VoipTunnelClientMatchGameIdx(VoipTunnelClientT *pClient, int32_t iGameIdx)
{
    int32_t iGameMappingIdx;

    for (iGameMappingIdx = 0; iGameMappingIdx < VOIPTUNNEL_MAX_GAMES_PER_CLIENT; iGameMappingIdx++)
    {
        if (iGameIdx == pClient->aGameMappings[iGameMappingIdx].iGameIdx)
        {
            return(&pClient->aGameMappings[iGameMappingIdx]);
        }
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelIsClientInGame

    \Description
        Returns TRUE if client still has a valid 'client-to-game mapping', returns
        FALSE otherwise.

    \Input *pClient             - client

    \Output
        uint32_t                - TRUE or FALSE

    \Version 12/15/2020 (mclouatre)
*/
/********************************************************************************F*/
uint32_t VoipTunnelIsClientInGame(VoipTunnelClientT *pClient)
{
    int32_t iGameMappingIdx;

    for (iGameMappingIdx = 0; iGameMappingIdx < VOIPTUNNEL_MAX_GAMES_PER_CLIENT; iGameMappingIdx++)
    {
        // is the game mapping used?
        if (pClient->aGameMappings[iGameMappingIdx].iGameIdx != -1)
        {
            return(TRUE);
        }
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelStatus

    \Description
        Get module status

    \Input *pVoipTunnel - pointer to module state
    \Input iSelect      - status selector
    \Input iValue       - selector specific
    \Input *pBuf        - [out] - selector specific
    \Input iBufSize     - size of output buffer
    
    \Output
        int32_t         - selector specific

    \Notes
        iControl can be one of the following:
    
        \verbatim
            'game' - copies game info for game index=iValue into buffer; returns zero on success, negative on failure
            'vddm' - retuns the number of voip packets that were not rebroadcast due to limits on max number of senders per game
            'vmtm' - returns the voip max talker metric (number of times the limit on talkers was reached)
            'mgam' - returns the max number of games the voiptunnel supports
            'musr' - returns the max number of clients the voiptunnel supports
            'ngam' - returns the total game count
            'nusr' - if iValue==-1, returns total user count, else returns count of users w/ gameserver==iValue
            'sock' - returns voip socket ref (copied into buffer), if available
            'talk' - returns the current number of active clients that are tagged as "talking"
        \endverbatim

    \Version 04/10/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTunnelStatus(VoipTunnelRefT *pVoipTunnel, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if ((iSelect == 'game') && (pBuf != NULL) && (iBufSize == sizeof(VoipTunnelGameT)) && (iValue < pVoipTunnel->iMaxGames))
    {
        ds_memcpy(pBuf, &pVoipTunnel->pGameList[iValue], iBufSize);
        return(0);
    }
    if (iSelect == 'mgam')
    {
        return(pVoipTunnel->iMaxGames);
    }
    if (iSelect == 'musr')
    {
        return(pVoipTunnel->iMaxClients);
    }
    if (iSelect == 'ngam')
    {
        return(pVoipTunnel->iNumGames);
    }
    if (iSelect == 'nusr')
    {
        int32_t iNumClients;
        if (iValue == -1)
        {
            iNumClients = pVoipTunnel->iNumClients;
        }
        else if (iValue < pVoipTunnel->iMaxGames)
        {
            iNumClients = pVoipTunnel->pGameList[iValue].iNumClients;
        }
        else
        {
            NetPrintf(("voiptunnel: invalid game index (%d) passed to VoipTunnelStatus('nusr') selector\n", iValue));
            iNumClients = -1;
        }
        return(iNumClients);
    }
    if (iSelect == 'vddm')
    {
        return(pVoipTunnel->uVoiceDataDropMetric);
    }    
    if (iSelect == 'vmtm')
    {
        return(pVoipTunnel->uVoiceMaxTalkersMetric);
    }
    if ((iSelect == 'sock') && (pBuf != NULL) && (iBufSize >= (signed)sizeof(pVoipTunnel->pVoipSocket)))
    {
        ds_memcpy(pBuf, &pVoipTunnel->pVoipSocket, sizeof(pVoipTunnel->pVoipSocket));
        return(0);
    }
    if (iSelect == 'talk')
    {
        int32_t iNumTalkingClients = -1;  // default to error
        if (iValue == -1)
        {
            iNumTalkingClients = pVoipTunnel->iNumTalkingClients;
        }
        else if (iValue < pVoipTunnel->iMaxGames)
        {
            iNumTalkingClients = pVoipTunnel->pGameList[iValue].VoipSessionInfo.iNumTalkingClients;
        }
        else
        {
            NetPrintf(("voiptunnel: invalid game index (%d) passed to VoipTunnelStatus('talk')\n", iValue));
        }
        return(iNumTalkingClients);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelControl

    \Description
        Control the module

    \Input *pVoipTunnel     - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'bvma' - set the maximum number of simultaneous voice broadcasters per game
            'rtim' - set voice receive timeout value (iValue = timeout in ms)
            'spam' - set debug level (iValue = level)
            'xnat' - set port sniffing enable/disable (iValue = TRUE/FALSE, default FALSE)
        \endverbatim

    \Version 04/09/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t VoipTunnelControl(VoipTunnelRefT *pVoipTunnel, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue)
{
    if (iControl == 'bvma')
    {
        NetPrintf(("voiptunnel: maximum number of simultaneous voice broadcasters changed from %d to %d\n", pVoipTunnel->iMaxVoiceBroadcasters, iValue));
        pVoipTunnel->iMaxVoiceBroadcasters = iValue;
        return(0);
    }
    if (iControl == 'rtim')
    {
        pVoipTunnel->uVoiceRecvTimeout = iValue;
        return(0);
    }
    if (iControl == 'spam')
    {
        pVoipTunnel->uDebugLevel = iValue;
        return(0);
    }
    if (iControl == 'xnat')
    {
        NetPrintf(("voiptunnel: port sniffing %s\n", iValue ? "enabled" : "disabled"));
        pVoipTunnel->bPortSniff = iValue;
        return(0);
    }
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function VoipTunnelUpdate

    \Description
        Update the module

    \Input *pVoipTunnel - voiptunnel ref

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void VoipTunnelUpdate(VoipTunnelRefT *pVoipTunnel)
{   
    // update clientlist
    _VoipTunnelClientListUpdate(pVoipTunnel);
}

