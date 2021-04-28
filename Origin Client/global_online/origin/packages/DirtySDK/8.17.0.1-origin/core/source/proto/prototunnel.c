/*H********************************************************************************/
/*!
    \File prototunnel.c

    \Description
        ProtoTunnel creates and manages a Virtual DirtySock Tunnel (VDST)
        connection.  The tunnel transparently bundles data sent from multiple ports
        to a specific remote host into a single send, optionally encrypting packets
        based on the tunnel mappings set up by the caller.  Only data sent over a
        UDP socket may be tunneled.

    \Notes
    \verbatim
        Encryption Key

            ProtoTunnel assumes that the encryption key used for non-Xbox platforms
            is provided by the caller creating the tunnel.  It does not provide any
            mechanism for secure key exchange; this is left to the caller.  A
            good random encryption key of at least sixteen bytes is strongly
            recommended to provide adequate security.

        Packet format

            ProtoTunnel bundles multiple packets bound for different ports on the
            same destination address into a single packet.  The packet contains
            a two-byte encryption header (see below), followed by a list of packet
            headers, followed by encrypted packet data (if any), followed by
            unencrypted packet data:

                 0              8            15
                -------------------------------
                |      Encryption Header      |     Tunnel encryption header
                -------------------------------
                |   Packet size        | Port |     Packet #1 header
                -------------------------------
                |   Packet size        | Port |     Packet #2 header
                -------------------------------
                                .
                                .
                                .
                -------------------------------
                |   Packet size        | Port |     Packet #n header
                -------------------------------
                |                             |
                |    Encrypted Packet Data    |     Encrypted Packets
                |                             |
                -------------------------------
                |                             |
                |   Unencrypted Packet Data   |     Unencrypted Packets
                |                             |
                -------------------------------
                        
            Encryption Header
        
            The leading two bytes of the tunnel packet are used for encryption.  On
            Xbox platforms, these two bytes indicate the amount of data in the packet
            that is encrypted.  The Xbox network stack takes care of the everything
            else for us.
        
            On other platforms, the two bytes are a 16bit stream index used to
            synchronize the stream cipher in the event of lost packets.  The 16bit
            index is shifted up by two bits to give a total window size of 256k.
            This requires the stream cipher to be iterated over the amount
            of data that is encrypted, and then advanced by the number of bytes
            required to round up to the next multiple of four bytes.  In the event
            tunnel packets are lost, the receiving side will advance the stream
            cipher by the amount of data lost to remain synchronized with the
            sender.  Any packets received with an offset into the most recent 1/4
            of the window are assumed to be out of order packets and are discarded.

            Packet Headers

            Packet headers consist of a twelve-bit size field and a four-bit port
            index.  This means the maximum bundled packet size is 4k and the maximum
            number of port mappings in a tunnel is 16.  ProtoTunnel restricts the actual
            maximum number of port mappings to eight.  ProtoTunnel also reserves the last
            port to use for embedded control data (see ProtoTunnel Control Info below),
            so this leaves seven user-allocatable ports.  It also means that tunnels
            are required to be created with the same port mappings on each side of
            the tunnel, otherwise the bundled tunnel packets will not be forwarded
            to the correct ports.  The tunnel demultiplexer iterates through the
            packet headers until the aggregate size of packet data plus packet
            headers plus encryption header equals the size of the received packet.
            On non-Xbox platforms, packet headers are decrypted one at a time until
            all of the headers are successfully decrypted.  If there are any
            discrepencies (e.g. invalid port mappings or aggregate sizes don't
            match) the packet is considered to be invalid and is discarded.

            Encrypted Packet Data

            Encrypted packets, if any, immediately follow the final header.  All
            encrypted packet data is decrypted together in one pass.

            Unencrypted Packet Data

            Unencrypted packets, if any, immediately follow encrypted packets, or
            packet headers if there are no encrypted packets.

        ProtoTunnel Control Info

            ProtoTunnel reserves port index seven to indicate embedded control data
            is included in the ProtoTunnel packet.  The data included with this
            port mapping is used internal to ProtoTunnel and is not forwarded.
    \endverbatim

    \Todo
        Handle situation where A has a tunnel to B and C, where B and C are
        both behind the same NAT and A is outside.  In this case, A will see
        traffic from B and C coming from the same address and there is no way
        to match up the data to the correct tunnel.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 12/02/2005 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#if defined(_XBOX)
#include <xtl.h>
#endif

#include <string.h>

#include "platform.h"
#include "dirtynet.h"
#include "dirtylib.h"
#include "dirtymem.h"
#include "cryptarc4.h"

#include "prototunnel.h"

/*** Defines **********************************************************************/

#define PROTOTUNNEL_PKTHDRSIZE      (2)
#define PROTOTUNNEL_MAXPACKETS      (8)
#define PROTOTUNNEL_MAXHDRSIZE      (PROTOTUNNEL_PKTHDRSIZE * PROTOTUNNEL_MAXPACKETS)
#define PROTOTUNNEL_PACKETBUFFER    (SOCKET_MAXUDPRECV)
#define PROTOTUNNEL_MAXPACKET       (PROTOTUNNEL_PACKETBUFFER - PROTOTUNNEL_PKTHDRSIZE - 2)

#define PROTOTUNNEL_CRYPTBITS       (2) // number of bits added to extend window
#define PROTOTUNNEL_CRYPTALGN       (1 << PROTOTUNNEL_CRYPTBITS)
#define PROTOTUNNEL_CRYPTMASK       (PROTOTUNNEL_CRYPTALGN-1)

#define PROTOTUNNEL_RECVDISCARD     (65536/4) // window extending back from present to detect and discard out of order packets

#define PROTOTUNNEL_CONTROL_PORTIDX (PROTOTUNNEL_MAXPORTS-1)

#if (DIRTYCODE_PLATFORM == DIRTYCODE_XENON)
 #define PROTOTUNNEL_XBOX_PLATFORM  (TRUE)
 #define PROTOTUNNEL_IPPROTO        (IPPROTO_VDP)
#else
 #define PROTOTUNNEL_XBOX_PLATFORM  (FALSE)
 #define PROTOTUNNEL_IPPROTO        (IPPROTO_IP)
#endif

#define PROTOTUNNEL_BASEADDRESS     (0x01000000) // base address in virtual address space

#define PROTOTUNNEL_MAXKEYS         (PROTOTUNNEL_MAXPORTS)

typedef enum ProtoTunnelControlE
{
    PROTOTUNNEL_CTRL_INIT
} ProtoTunnelControlE;

/*** Type Definitions *************************************************************/

typedef struct ProtoTunnelControlT
{
    uint8_t uPacketType;        //!< PROTOTUNNEL_CTRL_*
    uint8_t aClientId[4];       //!< source clientId
    uint8_t uActive;            //!< is connection active?
} ProtoTunnelControlT;

typedef struct ProtoTunnelT
{
    ProtoTunnelInfoT    Info;               //!< mapping info
    uint32_t            uVirtualAddr;       //!< virtual address, used for NAT support
    uint32_t            uLocalClientId;     //!< tunnel-specific local clientId (can override ProtoTunnelT if non-zero)
    int16_t             iBuffSize;          //!< current next open position in packet data buffer
    int16_t             iDataSize;          //!< current amount of data queued
    int8_t              iNumPackets;        //!< number of packets queued
    int8_t              _pad[3];
    NetCritT            PacketCrit;         //!< packet buffer critical section

    uint16_t            uSendOffset;        //!< stream send offset
    uint16_t            uRecvOffset;        //!< stream recv offset
    CryptArc4T          CryptSendState;     //!< crypto state for encrypting data
    CryptArc4T          CryptRecvState;     //!< crypto state for decrypting data

    #if DIRTYCODE_LOGGING
    uint32_t            uLastStatUpdate;    //!< last time stat update was printed
    ProtoTunnelStatT    LastSendStat;       //!< previous send stat
    #endif

    ProtoTunnelStatT    SendStat;           //!< cumulative send statistics
    ProtoTunnelStatT    RecvStat;           //!< cumulative recv statistics

    char                aKeyList[PROTOTUNNEL_MAXKEYS][128];        //!< crypto key(s)
    uint8_t             uActive;            //!< "active" if we've received data from remote peer
    uint8_t             uRefCount;          //!< tunnel ref count
    uint8_t             uSendKey;           //!< index of key we are using to send with
    uint8_t             uRecvKey;           //!< index of key we are using to recv with
    uint8_t             bSendCtrlInfo;      //!< whether to send control info or not
    uint8_t             aForceCrypt[PROTOTUNNEL_MAXPACKETS];    //!< force crypt override (per sub-packet)
    uint8_t             aPacketData[PROTOTUNNEL_PACKETBUFFER+PROTOTUNNEL_MAXHDRSIZE+2];  //!< packet aggregator
} ProtoTunnelT;

struct ProtoTunnelRefT
{
    // module memory group
    int32_t         iMemGroup;              //!< module mem group id
    void            *pMemGroupUserData;     //!< user data associated with mem group

    SocketT         *pSocket;               //!< physical socket tunnel uses
    uint32_t        uTunnelPort;            //!< port socket tunnel is bound to
    uint32_t        uLocalClientId;         //!< local client identifier
    
    int32_t         iMaxTunnels;            //!< maximum number of tunnels
    int32_t         iVerbosity;             //!< verbosity level

    ProtoTunnelModeE eMode;                 //!< prototunnel mode
    uint32_t        uVirtualAddr;           //!< next free virtual address

    uint32_t        uFlushRate;             //!< rate at which packets are flushed by update
    uint32_t        uLastFlush;             //!< time last flush was performed by update

    #if DIRTYCODE_DEBUG
    int32_t         iPacketDrop;            //!< drop the next n packets
    #endif

    uint32_t        uNumRecvCalls;          //!< number of receive calls
    uint32_t        uNumPktsRecvd;          //!< number of packets received
    uint32_t        uNumSubPktsRecvd;       //!< number of sub-packets received

    ProtoTunnelCallbackT *pCallback;        //!< prototunnel event callback
    void            *pUserData;             //!< callback user data

    RawRecvCallbackT *pRawRecvCallback;     //!< raw inbound data filtering function
    void            *pRawRecvUserData;      //!< callback user data

    NetCritT        TunnelsCritS;           //!< "send thread" critical section
    NetCritT        TunnelsCritR;           //!< "recv thread" critical section
    ProtoTunnelT    Tunnels[1];             //!< variable-length tunnel array - must be last
};

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

int32_t ProtoTunnelValidatePacket(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, uint8_t *pOutputData, const uint8_t *pPacketData, int32_t iPacketSize, const char *pKey);


/*F********************************************************************************/
/*!
    \Function _ProtoTunnelIndexFromId

    \Description
        Return tunnel index of tunnel with specified Id, or -1 if there is no
        tunnel that matches.

    \Input *pProtoTunnel    - module state
    \Input uTunnelId        - tunnel ident

    \Output
        int32_t             - tunnel index, or -1 if not found

    \Version 02/20/2009 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelIndexFromId(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId)
{
    int32_t iTunnel;

    // find tunnel id
    for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel += 1)
    {
        // found it?
        if (pProtoTunnel->Tunnels[iTunnel].uVirtualAddr == uTunnelId)
        {
            return(iTunnel);
        }
    }
    // not found
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelStreamAdvance

    \Description
        Advance stream offset, and advance crypt state if we need to for padding
        purposes.

    \Input *pCryptState - pointer to crypt state to advance
    \Input *pOffset     - pointer to current offset (16bit in crypt data units)
    \Input uOffset      - amount to advance by (in bytes)

    \Version 12/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoTunnelStreamAdvance(CryptArc4T *pCryptState, uint16_t *pOffset, uint32_t uOffset)
{
    uint32_t uCryptAlign;

    // convert from number of bytes to number of crypt data units
    *pOffset += (uint16_t)(uOffset>>PROTOTUNNEL_CRYPTBITS);

    // if we have 'left-over' bytes, advance the crypt state and add a data unit
    if ((uCryptAlign = (uOffset & PROTOTUNNEL_CRYPTMASK)) != 0)
    {
        uCryptAlign = PROTOTUNNEL_CRYPTALGN - uCryptAlign;
        CryptArc4Advance(pCryptState, uCryptAlign);
        *pOffset += 1;
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelVirtualToPhysical

    \Description
        Convert a virtual address into its corresponding physical address.

    \Input *pProtoTunnel    - pointer to module state
    \Input uVirtualAddr     - virtual address
    \Input *pBuf            - [out] storage for sockaddr (optional)
    \Input iBufSize         - size of buffer

    \Output
        int32_t             - physical address, or zero if no match

    \Version 03/31/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelVirtualToPhysical(ProtoTunnelRefT *pProtoTunnel, uint32_t uVirtualAddr, char *pBuf, int32_t iBufSize)
{
    ProtoTunnelT *pTunnel;
    uint32_t uRemoteAddr;
    int32_t iTunnel;

    // acquire exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // find tunnel virtual address is bound to
    for (iTunnel = 0, uRemoteAddr = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];
        if (pTunnel->uVirtualAddr == uVirtualAddr)
        {
            struct sockaddr SockAddr;
            if ((pBuf != NULL) && (iBufSize >= (signed)sizeof(SockAddr)))
            {
                SockaddrInit(&SockAddr, AF_INET);
                SockaddrInSetAddr(&SockAddr, pTunnel->Info.uRemoteAddr);
                SockaddrInSetPort(&SockAddr, pTunnel->Info.uRemotePort);
                memcpy(pBuf, &SockAddr, sizeof(SockAddr));
            }
            uRemoteAddr = pTunnel->Info.uRemoteAddr;
            break;
        }
    }

    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);

    // return addr to caller
    return(uRemoteAddr);
}
 
/*F********************************************************************************/
/*!
    \Function _ProtoTunnelBufferCollect

    \Description
        Collect buffered data in final packet form.

    \Input *pTunnel         - tunnel to send data on
    \Input iHeadSize        - size of packet headers
    \Input *pPacketData     - [out] buffer for finalized packet
    \Input **ppHeaderDst    - pointer to current header output in finalized packet
    \Input **ppPacketDst    - pointer to current packet data output in finalized packet
    \Input uPortFlag        - flag indicating whether we are collecting encrypted or
                              unencrypted packet data
    \Input *pLimit          - end of buffer, used for debug overflow check

    \Output
        int32_t             - size of packet data added to output buffer

    \Version 12/08/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelBufferCollect(ProtoTunnelT *pTunnel, int32_t iHeadSize, uint8_t *pPacketData, uint8_t **ppHeaderDst, uint8_t **ppPacketDst, uint32_t uPortFlag, uint8_t *pLimit)
{
    int32_t iDataSize, iPacket, iPacketSize;
    uint8_t *pHeaderSrc, *pPacketSrc;
    uint32_t bEnabled;

    // point to packet headers and data
    pHeaderSrc = pTunnel->aPacketData + 2;
    pPacketSrc = pTunnel->aPacketData + PROTOTUNNEL_MAXHDRSIZE + 2;
    iDataSize = 0;

    // collect packets
    for (iPacket = 0; iPacket < pTunnel->iNumPackets; iPacket++)
    {
        // get packet size
        iPacketSize = (pHeaderSrc[0] << 4) | (pHeaderSrc[1] >> 4);

        // if packet encryption matches that specified by the caller, copy it and its header
        bEnabled = (((pTunnel->Info.aPortFlags[pHeaderSrc[1] & 0xf] & (unsigned)PROTOTUNNEL_PORTFLAG_ENCRYPTED) == uPortFlag));
        if (pTunnel->aForceCrypt[iPacket] != 0)
        {
            bEnabled = !bEnabled;
        }
        if (bEnabled)
        {
            if ((((*ppPacketDst) + iPacketSize) - pLimit) > 0)
            {
                NetPrintf(("prototunnel: collect buffer overflow by %d bytes\n", (((*ppPacketDst) + iPacketSize) - pLimit)));
            }

            memcpy(*ppPacketDst, pPacketSrc, iPacketSize);
            (*ppHeaderDst)[0] = pHeaderSrc[0];
            (*ppHeaderDst)[1] = pHeaderSrc[1];
            iDataSize += iPacketSize;
            *ppPacketDst += iPacketSize;
            *ppHeaderDst += PROTOTUNNEL_PKTHDRSIZE;
        }

        // increment to next packet header and packet data
        pHeaderSrc += PROTOTUNNEL_PKTHDRSIZE;
        pPacketSrc += iPacketSize;
    }

    // return size to caller
    return(iDataSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelBufferSend

    \Description
        Send buffered data

    \Input *pProtoTunnel    - pointer to module state
    \Input *pTunnel         - tunnel to send data on

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static void _ProtoTunnelBufferSend(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel)
{
    uint8_t aPacketData[PROTOTUNNEL_PACKETBUFFER];
    int32_t iCryptSize, iDataSize, iHeadSize, iResult;
    uint8_t *pHeaderDst, *pPacketDst;
    struct sockaddr SendAddr;
    #if DIRTYCODE_LOGGING
    int32_t iNumPackets;
    #endif
    
    // get sole access to packet buffer
    NetCritEnter(&pTunnel->PacketCrit);

    // no data to send or socket to send it on?
    if ((pProtoTunnel->pSocket == NULL) || (pTunnel->iDataSize <= 0))
    {
        NetCritLeave(&pTunnel->PacketCrit);
        return;
    }

    // create send addr
    SockaddrInit(&SendAddr, AF_INET);
    SockaddrInSetAddr(&SendAddr, pTunnel->Info.uRemoteAddr);
    SockaddrInSetPort(&SendAddr, pTunnel->Info.uRemotePort);

    // format packet in local buffer
    memset(aPacketData, 0, sizeof(aPacketData));
    
    // set up for copy
    iHeadSize = ((pTunnel->iNumPackets + pTunnel->bSendCtrlInfo) * PROTOTUNNEL_PKTHDRSIZE) + 2;
    pHeaderDst = aPacketData + 2;
    pPacketDst = aPacketData + iHeadSize;

    // add init control packet, if desired
    if (pTunnel->bSendCtrlInfo)
    {
        ProtoTunnelControlT ControlPacket;
        uint32_t uLocalClientId = (pTunnel->uLocalClientId != 0) ? pTunnel->uLocalClientId : pProtoTunnel->uLocalClientId;

        // format control packet
        memset(&ControlPacket, 0, sizeof(ControlPacket));
        ControlPacket.uPacketType = PROTOTUNNEL_CTRL_INIT;
        ControlPacket.aClientId[0] = (uint8_t)(uLocalClientId >> 24);
        ControlPacket.aClientId[1] = (uint8_t)(uLocalClientId >> 16);
        ControlPacket.aClientId[2] = (uint8_t)(uLocalClientId >> 8);
        ControlPacket.aClientId[3] = (uint8_t)(uLocalClientId);
        ControlPacket.uActive = pTunnel->uActive;

        // copy it into the collect buffer
        memcpy(pPacketDst, &ControlPacket, sizeof(ControlPacket));
        
        // set the header
        pHeaderDst[0] = sizeof(ControlPacket)>>4;
        pHeaderDst[1] = sizeof(ControlPacket)<<4 | PROTOTUNNEL_CONTROL_PORTIDX;
        
        // adjust pointers & offsets
        pHeaderDst += 2;
        pPacketDst += sizeof(ControlPacket);
        iCryptSize = sizeof(ControlPacket);
    }
    else
    {
        iCryptSize = 0;
    }

    // collect encrypted packets
    iCryptSize += _ProtoTunnelBufferCollect(pTunnel, iHeadSize, aPacketData, &pHeaderDst, &pPacketDst, PROTOTUNNEL_PORTFLAG_ENCRYPTED, aPacketData + PROTOTUNNEL_PACKETBUFFER);

    // collect unencrypted packets
    iDataSize = _ProtoTunnelBufferCollect(pTunnel, iHeadSize, aPacketData, &pHeaderDst, &pPacketDst, 0, aPacketData + PROTOTUNNEL_PACKETBUFFER) + iCryptSize;

    // calculate total encrypted size (encrypted headers + encrypted data, but skip the first two bytes)
    iCryptSize += iHeadSize - 2;
    // total size of packet
    iDataSize += iHeadSize;

    if (pProtoTunnel->iVerbosity > 3)
    {
        NetPrintMem(aPacketData, (iDataSize < 64) ? iDataSize : 64, "prototunnel-send-nocrypt");
    }

    // encrypt headers + encrypted data (if any)
    if (pProtoTunnel->eMode == PROTOTUNNEL_MODE_GENERIC)
    {
        aPacketData[0] = (uint8_t)(pTunnel->uSendOffset >> 8);
        aPacketData[1] = (uint8_t)(pTunnel->uSendOffset >> 0);
        CryptArc4Apply(&pTunnel->CryptSendState, aPacketData + 2, iCryptSize);
        _ProtoTunnelStreamAdvance(&pTunnel->CryptSendState, &pTunnel->uSendOffset, iCryptSize);
    }
    else // PROTOTUNNEL_MODE_XENON
    {
        iCryptSize = SocketHtons(iCryptSize);
        aPacketData[0] = (uint8_t)(iCryptSize >> 8);
        aPacketData[1] = (uint8_t)(iCryptSize >> 0);
    }
    
    // accumulate stats
    pTunnel->SendStat.uNumBytes += iDataSize;
    pTunnel->SendStat.uNumSubpacketBytes += iDataSize - iHeadSize;
    pTunnel->SendStat.uNumPackets += 1;
    pTunnel->SendStat.uNumSubpackets += pTunnel->iNumPackets + pTunnel->bSendCtrlInfo;

    #if DIRTYCODE_LOGGING
    iNumPackets = pTunnel->iNumPackets + pTunnel->bSendCtrlInfo;
    #endif

    // mark data as sent
    pTunnel->iBuffSize = 0;
    pTunnel->iDataSize = 0;
    pTunnel->iNumPackets = 0;
    memset(pTunnel->aForceCrypt, 0, sizeof(pTunnel->aForceCrypt));

    // release packet buffer critical section
    NetCritLeave(&pTunnel->PacketCrit);

    #if DIRTYCODE_LOGGING
    if (NetTickDiff(NetTick(), pTunnel->uLastStatUpdate) > 5000)
    {
        ProtoTunnelStatT DiffStat;
        DiffStat.uNumBytes = pTunnel->SendStat.uNumBytes - pTunnel->LastSendStat.uNumBytes;
        DiffStat.uNumSubpacketBytes = pTunnel->SendStat.uNumSubpacketBytes - pTunnel->LastSendStat.uNumSubpacketBytes;
        DiffStat.uNumPackets = pTunnel->SendStat.uNumPackets - pTunnel->LastSendStat.uNumPackets;
        DiffStat.uNumSubpackets = pTunnel->SendStat.uNumSubpackets - pTunnel->LastSendStat.uNumSubpackets;
        memcpy(&pTunnel->LastSendStat, &pTunnel->SendStat, sizeof(pTunnel->LastSendStat));
        NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: pkts: %d eff: %.2f sent: %d eff: %.2f\n",
            DiffStat.uNumPackets, (float)DiffStat.uNumSubpackets / (float)DiffStat.uNumPackets,
            DiffStat.uNumBytes, (float)DiffStat.uNumSubpacketBytes / (float)DiffStat.uNumBytes));
        pTunnel->uLastStatUpdate = NetTick();
    }
    if (pProtoTunnel->iVerbosity > 3)
    {
        NetPrintMem(aPacketData, (iDataSize < 64) ? iDataSize : 64, "prototunnel-send");
    }
    #endif

    // send the data
    if ((iResult = SocketSendto(pProtoTunnel->pSocket, (char *)aPacketData, iDataSize, 0, &SendAddr, sizeof(SendAddr))) < 0)
    {
        NetPrintf(("prototunnel: [send] error %d sending buffered packet to %a:%d\n", iResult,
            SockaddrInGetAddr(&SendAddr), SockaddrInGetPort(&SendAddr)));
    }
    else
    {
        NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: [send] sent %d bytes [%d packets] to %a:%d\n", iDataSize, iNumPackets,
            SockaddrInGetAddr(&SendAddr), SockaddrInGetPort(&SendAddr)));
    }
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelBufferData

    \Description
        Buffer a send

    \Input *pProtoTunnel    - pointer to module state
    \Input *pTunnel         - tunnel to buffer data on
    \Input *pData           - data to be sent
    \Input uSize            - size of data to send
    \Input uPortIdx         - index of port in tunnel map
    \Input bForceCrypt      - TRUE to force encryption of packet, else FALSE

    \Output
        int32_t             - amount of data buffered

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelBufferData(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, const uint8_t *pData, uint32_t uSize, uint32_t uPortIdx, uint8_t bForceCrypt)
{
    uint8_t *pBuffer;

    // make sure data is within size limits
    if ((uSize == 0) || (uSize > PROTOTUNNEL_MAXPACKET))
    {
        NetPrintf(("prototunnel: packet of size %d will not be tunneled\n", uSize));
        NetPrintMem(pData, 64, "prototunnel-nobuf");
        return(0);
    }

    // acquire packet buffer critical section
    NetCritEnter(&pTunnel->PacketCrit);

    // if buffer is full, or we've already buffered max packets, flush buffer
    if ((((unsigned)pTunnel->iDataSize + uSize + PROTOTUNNEL_PKTHDRSIZE) > PROTOTUNNEL_PACKETBUFFER) ||
         (pTunnel->iNumPackets == PROTOTUNNEL_MAXPACKETS))
    {
        // flush current packet
        _ProtoTunnelBufferSend(pProtoTunnel, pTunnel);
    }

    // if buffer is empty, reserve space for encryption header and max packet headers
    if (pTunnel->iBuffSize == 0)
    {
        pTunnel->iBuffSize = 2 + PROTOTUNNEL_MAXHDRSIZE;
        pTunnel->iDataSize = 2;
        memset(pTunnel->aPacketData, 0, pTunnel->iBuffSize);
    }

    // store packet header
    pBuffer = pTunnel->aPacketData + (pTunnel->iNumPackets * PROTOTUNNEL_PKTHDRSIZE) + 2;
    pBuffer[0] = (uint8_t)(uSize >> 4);
    pBuffer[1] = (uint8_t)((uSize << 4) | uPortIdx);

    // store packet data
    pBuffer = pTunnel->aPacketData + pTunnel->iBuffSize;
    memcpy(pBuffer, pData, uSize);
    pTunnel->iBuffSize += uSize;
    pTunnel->iDataSize += uSize + PROTOTUNNEL_PKTHDRSIZE;
    pTunnel->aForceCrypt[(uint32_t)pTunnel->iNumPackets] = bForceCrypt;

    // update overall count
    pTunnel->iNumPackets += 1;

    if (pTunnel->Info.aPortFlags[uPortIdx] & PROTOTUNNEL_PORTFLAG_AUTOFLUSH)
    {
        _ProtoTunnelBufferSend(pProtoTunnel, pTunnel);
    }


    // release packet buffer critical section
    NetCritLeave(&pTunnel->PacketCrit);

    // return buffered size to caller
    return(uSize);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelSendCallback

    \Description
        Global send callback handler.

    \Input *pSocket     - socket to send on
    \Input iType        - socket type
    \Input *pData       - data to be sent
    \Input iDataSize    - size of data to send
    \Input *pTo         - destination address
    \Input *pCallref    - prototunnel ref

    \Output
        int32_t         - zero=send not handled, else handled

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelSendCallback(SocketT *pSocket, int32_t iType, const uint8_t *pData, int32_t iDataSize, const struct sockaddr *pTo, void *pCallref)
{
    ProtoTunnelRefT *pProtoTunnel = (ProtoTunnelRefT *)pCallref;
    ProtoTunnelT *pTunnel = NULL;
    uint32_t uAddr, uPort;
    int32_t iPort=0, iResult=0, iTunnel;
    uint32_t bFound, bFoundAddr, bForceCrypt=0;
    struct sockaddr SockAddr;

    // only handle dgram sockets, and don't handle our own socket
    if ((iType != SOCK_DGRAM) || (pProtoTunnel->pSocket == pSocket))
    {
        return(0);
    }

    // skip xbox encryption header
    #if PROTOTUNNEL_XBOX_PLATFORM
    if (SocketInfo(pSocket, 'prot', 0, NULL, 0) == IPPROTO_VDP)
    {
        // extract encrypted size
        int32_t iCryptSize = (pData[0] << 4) | pData[1];

        // skip the header    
        pData += 2;
        iDataSize -= 2;

        // check to see if entire packet is supposed to be encrypted
        if (iCryptSize == iDataSize)
        {
            // packet is fully encrypted so force encryption
            NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: crypt override of packet with size %d\n", iCryptSize));
            bForceCrypt = 1;
        }
    }
    #endif

    // if a destination address is not specified, get it from the socket
    if (pTo == NULL)
    {
        SocketInfo(pSocket, 'peer', 0, &SockAddr, sizeof(SockAddr));
        pTo = &SockAddr;
    }

    // get destination address and port
    uAddr = SockaddrInGetAddr(pTo);
    uPort = SockaddrInGetPort(pTo);

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);

    // see if we have a match
    for (iTunnel = 0, bFound = FALSE, bFoundAddr = FALSE; (iTunnel < pProtoTunnel->iMaxTunnels) && (bFound == FALSE); iTunnel++)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];

        // does virtual address match?
        if (pTunnel->uVirtualAddr != uAddr)
        {
            continue;
        }
        // remember if we have a tunnel to this address
        bFoundAddr = TRUE;
        // see if we have a port match or are at the end of our port list
        for (iPort = 0; iPort < PROTOTUNNEL_MAXPORTS; iPort++)
        {
            uint32_t uPort2 = pTunnel->Info.aRemotePortList[iPort];
            if (uPort2 == uPort)
            {
                bFound = TRUE;
                break;
            }
        }
    }

    // found a match?
    if (bFound == TRUE)
    {
        // if we have a match, queue data
        iResult = _ProtoTunnelBufferData(pProtoTunnel, pTunnel, pData, iDataSize, iPort, bForceCrypt);
    }
    else if (bFoundAddr)
    {
        // if we have a tunnel to this address, but no port match, print a warning diagnostic
        NetPrintf(("prototunnel: [send] no match for data sent to %a:%d\n", uAddr, uPort));
    }

    // release exclusive access to tunnel list and return result
    NetCritLeave(&pProtoTunnel->TunnelsCritS);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelDecryptAndValidatePacket

    \Description
        Decrypt the tunnel packet

    \Input *pTunnel     - tunnel
    \Input *pPacketData - pointer to tunnel packet
    \Input iRecvLen     - size of tunnel packet

    \Output
        int32_t         - negative=error, else success

    \Version 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelDecryptAndValidatePacket(ProtoTunnelT *pTunnel, uint8_t *pPacketData, int32_t iRecvLen)
{
    int32_t iEncryptedSize, iNumPackets, iPacketOffset, iPacketSize;
    uint16_t aPacketHeader[PROTOTUNNEL_MAXHDRSIZE+2];
    uint8_t *pPacketStart = pPacketData;
    uint32_t uRecvOffset, uRecvOffsetState;
    CryptArc4T CryptRecvState;

    // make a copy of packet header before doing anything else, in case we need to restore it after a decrypt failure
    memcpy(aPacketHeader, pPacketData, sizeof(aPacketHeader));

    /* get stream offset from packet header - this tells us
       where we need to be to be able to decrypt the packet */
    uRecvOffset = (pPacketData[0] << 8) | pPacketData[1];
    
    // work on a local copy of the crypt state and stream offset, in case we need to throw this packet out
    memcpy(&CryptRecvState, &pTunnel->CryptRecvState, sizeof(CryptRecvState));
    uRecvOffsetState = pTunnel->uRecvOffset;
    
    // lost data?  sync the cipher
    if (uRecvOffset != uRecvOffsetState)
    {
        // calc how many data units we've missed
        int16_t iRecvOffset = (signed)uRecvOffset;
        int16_t iTunlOffset = (signed)pTunnel->uRecvOffset;
        int16_t iRecvDiff = iRecvOffset - iTunlOffset;
        if (iRecvDiff < 0)
        {
            // assume this is an out-of-order packet and toss it
            NetPrintf(("prototunnel: received out of order packet (off=%d); discarding\n", iRecvDiff));
            return(0);
        }

        // advance the cipher to resync
        NetPrintf(("prototunnel: stream skip %d bytes\n", iRecvDiff<<PROTOTUNNEL_CRYPTBITS));
        CryptArc4Advance(&CryptRecvState, iRecvDiff<<PROTOTUNNEL_CRYPTBITS);
        uRecvOffsetState += (uint32_t)iRecvDiff;
    }

    // reset offset to count bytes added from this packet
    uRecvOffset = 0;
    
    // skip stream offset    
    pPacketData += 2;
    iPacketOffset = 2;

    // iterate through packet headers    
    for (iNumPackets = 0, iEncryptedSize = 0; iPacketOffset < iRecvLen; iNumPackets++)
    {
        // decrypt the packet header
        CryptArc4Apply(&CryptRecvState, pPacketData, PROTOTUNNEL_PKTHDRSIZE);
        uRecvOffset += PROTOTUNNEL_PKTHDRSIZE;

        // extract size and port info
        iPacketSize = (pPacketData[0] << 4) | (pPacketData[1] >> 4);
        iPacketOffset += iPacketSize + PROTOTUNNEL_PKTHDRSIZE;
        if (pTunnel->Info.aPortFlags[pPacketData[1] & 0xf] & PROTOTUNNEL_PORTFLAG_ENCRYPTED)
        {
            iEncryptedSize += iPacketSize;
        }
        
        // increment to next packet header
        pPacketData += PROTOTUNNEL_PKTHDRSIZE;
    }

    // make sure size matches
    if (iPacketOffset != iRecvLen)
    {
        NetPrintf(("prototunnel: [recv] mismatched size in received packet (%d received, %d decoded)\n", iRecvLen, iPacketOffset));
        // restore original packet header
        memcpy(pPacketStart, aPacketHeader, sizeof(aPacketHeader));
        return(-1);
    }

    // decrypt encrypted packet data
    if (iEncryptedSize > 0)
    {
        CryptArc4Apply(&CryptRecvState, pPacketData, iEncryptedSize);
        uRecvOffset += iEncryptedSize;
    }

    // update crypt state and stream offset with working temp copies
    memcpy(&pTunnel->CryptRecvState, &CryptRecvState, sizeof(pTunnel->CryptRecvState));
    pTunnel->uRecvOffset = (uint16_t)uRecvOffsetState;
    // advance stream by received encrypted packet data size, plus padding if necessary
    _ProtoTunnelStreamAdvance(&pTunnel->CryptRecvState, &pTunnel->uRecvOffset, uRecvOffset);

    // return number of packets decoded    
    return(iNumPackets);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelValidatePacket

    \Description
        Validate packet

    \Input *pPacketData - pointer to tunnel packet
    \Input iRecvLen     - size of tunnel packet

    \Output
        int32_t         - negative=error, else success

    \Version 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelValidatePacket(const uint8_t *pPacketData, int32_t iRecvLen)
{
    int32_t iPacketSize;
    int32_t iNumPackets;

    // skip vdp packet header
    pPacketData += 2;
    iRecvLen -= 2;

    // iterate through packet headers    
    for (iNumPackets = 0, iPacketSize = 0; iPacketSize < iRecvLen; iNumPackets++)
    {
        // extract size info
        iPacketSize += ((pPacketData[0] << 4) | (pPacketData[1] >> 4)) + PROTOTUNNEL_PKTHDRSIZE;
        
        // increment to next packet header
        pPacketData += PROTOTUNNEL_PKTHDRSIZE;
    }

    // make sure size matches
    if (iPacketSize != iRecvLen)
    {
        NetPrintf(("prototunnel: [recv] mismatched size in received packet (%d received, %d decoded)\n", iRecvLen, iPacketSize));
        iNumPackets = -1;
    }

    // return number of packets decoded
    return(iNumPackets);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelGetControlPacket

    \Description
        Dereference control packet in prototunnel packet buffer

    \Input *pPacketData             - source data
    \Input iPacketSize              - size of source data
    \Input iNumPackets              - number of subpackets in packet buffer

    \Output
        const ProtoTunnelControlT * - pointer to control packet, or NULL if none is present

    \Version 06/27/2006 (jbrookes)
*/
/********************************************************************************F*/
static const ProtoTunnelControlT *_ProtoTunnelGetControlPacket(const uint8_t *pPacketData, int32_t iPacketSize, int32_t iNumPackets)
{
    uint32_t uPktHead, uPktSize, uPortIdx;
    int32_t iPacket, iPacketOffset;
    const uint8_t *pPacketHead;

    // search through packets for a control packet (note - this code assumes only one control packet is included!)    
    for (iPacket = 0, iPacketOffset = (iNumPackets*2)+2, pPacketHead = pPacketData + 2; iPacket < iNumPackets; iPacket++)
    {
        // extract size and port info
        uPktHead = (pPacketHead[0] << 8) | pPacketHead[1];
        uPktSize = uPktHead >> 4;
        uPortIdx = uPktHead & 0xf;
    
        // if it is a control packet, decode it
        if (uPortIdx == PROTOTUNNEL_CONTROL_PORTIDX)
        {
            return((ProtoTunnelControlT *)(pPacketData+iPacketOffset));
        }

        // iterate to next packet
        pPacketHead += PROTOTUNNEL_PKTHDRSIZE;
        iPacketOffset += (signed)uPktSize;
    }
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelMatchTunnel

    \Description
        Attempt to decrypt incoming data against a tunnel, and check for clientId match

    \Input *pProtoTunnel    - module state
    \Input *pTunnel         - tunnel to check data against
    \Input *pPacketData     - source data
    \Input iPacketSize      - size of source data

    \Output
        int32_t             - zero=no match, else match

    \Version 06/26/2006 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelMatchTunnel(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, const uint8_t *pPacketData, int32_t iPacketSize)
{
    uint8_t aPacketData[PROTOTUNNEL_PACKETBUFFER];
    const ProtoTunnelControlT *pControl;
    int32_t iNumPackets=0;
    int32_t iRecvKey;

    // try any non-empty keys in this tunnel
    for (iRecvKey = 0; iRecvKey < PROTOTUNNEL_MAXKEYS; iRecvKey++)
    {
        // skip empty keys
        if (pTunnel->aKeyList[iRecvKey][0] == '\0')
        {
            continue;
        }

        // try and validate packet against this key
        if ((iNumPackets = ProtoTunnelValidatePacket(pProtoTunnel, pTunnel, aPacketData, pPacketData, iPacketSize, pTunnel->aKeyList[iRecvKey])) > 0)
        {
            break;
        }
    }

    // did we successfully decode the packet?
    if (iNumPackets <= 0)
    {
        return(0);
    }

    // if we're rematching an active tunnel, we're done
    if (pTunnel->uActive != 0)
    {
        NetPrintf(("prototunnel: successful rematch to key %d (%s)\n", iRecvKey, pTunnel->aKeyList[iRecvKey]));
        CryptArc4Init(&pTunnel->CryptRecvState, (unsigned char *)pTunnel->aKeyList[iRecvKey], (int32_t)strlen(pTunnel->aKeyList[iRecvKey]), 1);
        pTunnel->uRecvKey = (uint8_t)iRecvKey;
        pTunnel->uRecvOffset = 0;
        return(iNumPackets);
    }

    // try and find a control packet
    if ((pControl = _ProtoTunnelGetControlPacket(aPacketData, iPacketSize, iNumPackets)) != NULL)
    {
        uint32_t uClientId;

        // extract the client identifier
        uClientId = pControl->aClientId[0] << 24;
        uClientId |= pControl->aClientId[1] << 16;
        uClientId |= pControl->aClientId[2] << 8;
        uClientId |= pControl->aClientId[3];

        // if we found the matching tunnel, we're done
        if (uClientId == pTunnel->Info.uRemoteClientId)
        {
            #if DIRTYCODE_LOGGING
            if (iRecvKey != 0)
            {
                NetPrintf(("prototunnel: matched key with index=%d\n", iRecvKey));
            }
            #endif
            CryptArc4Init(&pTunnel->CryptRecvState, (unsigned char *)pTunnel->aKeyList[iRecvKey], (int32_t)strlen(pTunnel->aKeyList[iRecvKey]), 1);
            pTunnel->uRecvKey = (uint8_t)iRecvKey;
            pTunnel->uRecvOffset = 0;
            return(1);
        }
        else
        {
            NetPrintf(("prototunnel: packet clientId 0x%08x does not match tunnel clientId 0x%08x\n",
                uClientId, pTunnel->Info.uRemoteClientId));
            if (pProtoTunnel->iVerbosity > 3)
            {
                NetPrintMem(aPacketData, (iPacketSize < 64) ? iPacketSize : 64, "prototunnel-recv-decrypt");
            }
        }
    }
    else
    {
        NetPrintf(("prototunnel: no control data included in packet on inactive tunnel\n"));
    }

    // no match
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelRecvData

    \Description
        Match incoming data with a tunnel, and if source address is mapped,
        decode the number of subpackets and decrypt packet data.

    \Input *pProtoTunnel    - module state
    \Input *pRemotePortList - [out] storage for remote port list of matching tunnel
    \Input iPortListSize    - size of port list
    \Input *pPacketData     - source data
    \Input iRecvLen         - size of source data
    \Input *pRecvAddr       - source address

    \Output
        int32_t             - -1=decode error, -2=no tunnel match, else number of decoded packets

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelRecvData(ProtoTunnelRefT *pProtoTunnel, uint16_t *pRemotePortList, int32_t iPortListSize, uint8_t *pPacketData, int32_t iRecvLen, struct sockaddr *pRecvAddr)
{
    uint32_t uRecvAddr, uRecvPort;
    int32_t iNumPackets, iTunnel;
    ProtoTunnelT *pTunnel;

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // find tunnel with matching remote address and port
    for (iTunnel = 0, uRecvAddr = SockaddrInGetAddr(pRecvAddr), uRecvPort = SockaddrInGetPort(pRecvAddr); iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];
        if ((pTunnel->Info.uRemoteAddr == uRecvAddr) && (pTunnel->Info.uRemotePort == uRecvPort))
        {
            if (pTunnel->uActive == 0)
            {
                // decrypt packet and verify clientId matches
                if (_ProtoTunnelMatchTunnel(pProtoTunnel, pTunnel, pPacketData, iRecvLen) != 0)
                {
                    // found tunnel to match incoming data with
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    // if we didn't find a tunnel try and find a matching unallocated tunnel
    if (iTunnel == pProtoTunnel->iMaxTunnels)
    {
        // find tunnel with matching remote address and port
        for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
        {
            pTunnel = &pProtoTunnel->Tunnels[iTunnel];
            
            // only consider allocated inactive tunnels for matching
            if ((pProtoTunnel->Tunnels[iTunnel].uVirtualAddr != 0) && (pTunnel->uActive == 0))
            {
                // decrypt packet and verify clientId matches
                if (_ProtoTunnelMatchTunnel(pProtoTunnel, pTunnel, pPacketData, iRecvLen) != 0)
                {
                    // found tunnel to match incoming data with
                    break;
                }
            }
        }
    }

    // did we find a tunnel?
    if (iTunnel < pProtoTunnel->iMaxTunnels)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];
        
        // if the tunnel isn't active yet, bind to it and mark it as active
        if (pTunnel->uActive == 0)
        {
            if ((pTunnel->Info.uRemoteAddr != uRecvAddr) || (pTunnel->Info.uRemotePort != uRecvPort))
            {
                NetPrintf(("prototunnel: updating remote address from %a:%d to %a:%d\n", pTunnel->Info.uRemoteAddr, pTunnel->Info.uRemotePort, uRecvAddr, uRecvPort));
            }

            // update tunnel to addr/port combo
            pTunnel->Info.uRemoteAddr = uRecvAddr;
            pTunnel->Info.uRemotePort = uRecvPort;

            // mark that we're now active
            pTunnel->uActive = 1;

            NetPrintf(("prototunnel: matched incoming data from %a:%d to tunnelId 0x%08x / clientId 0x%08x\n", uRecvAddr, uRecvPort,
               pTunnel->uVirtualAddr, pTunnel->Info.uRemoteClientId));
        }

        // make a copy of port list for later use
        memcpy(pRemotePortList, pTunnel->Info.aRemotePortList, iPortListSize);
        
        // display recv info
        NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: [recv] received %d bytes on tunnel 0x%08x from %a:%d\n",
            iRecvLen, pTunnel->uVirtualAddr, uRecvAddr, uRecvPort));
        
        // decode the packet to local buffer
        if (pProtoTunnel->eMode == PROTOTUNNEL_MODE_GENERIC)
        {
            iNumPackets = _ProtoTunnelDecryptAndValidatePacket(pTunnel, pPacketData, iRecvLen);
        }
        else
        {
            iNumPackets = _ProtoTunnelValidatePacket(pPacketData, iRecvLen);
        }
        if (iNumPackets > 0)
        {
            const ProtoTunnelControlT *pControl;

            // accumulate receive stats
            pTunnel->RecvStat.uNumBytes += iRecvLen;
            pTunnel->RecvStat.uNumSubpacketBytes += iRecvLen - (iNumPackets * PROTOTUNNEL_PKTHDRSIZE) - 2;
            pTunnel->RecvStat.uNumPackets += 1;
            pTunnel->RecvStat.uNumSubpackets += iNumPackets;
            pProtoTunnel->uNumSubPktsRecvd += (unsigned)iNumPackets;

            // process any control packets
            if ((pControl = _ProtoTunnelGetControlPacket(pPacketData, iRecvLen, iNumPackets)) != NULL)
            {
                if (pControl->uActive == 1)
                {
                    NetPrintf(("prototunnel: connection is active; disabling control packets\n"));
                    pTunnel->bSendCtrlInfo = FALSE;
                }
                else if (!pTunnel->bSendCtrlInfo)
                {
                    /*
                    This path is typically exercised when one side of the connection ends up refcounting a tunnel and the other side
                    ends up destroying/recreating the tunnel because both consoles get Blaze messages with different timing. Under such
                    conditions, the prototunnel handshaking (sending of ctrl messages) needs to occur again.
                    */
                    NetPrintf(("prototunnel: peer no longer sees connection as active; resume sending control packets\n"));
                    pTunnel->bSendCtrlInfo = TRUE;
                }
            }
            else if ((pTunnel->bSendCtrlInfo) && (pTunnel->uActive == 1))
            {
                NetPrintf(("prototunnel: connection is active; disabling control packets\n"));
                pTunnel->bSendCtrlInfo = FALSE;
            }
            
            // rewrite address to virtual address
            SockaddrInSetAddr(pRecvAddr, pTunnel->uVirtualAddr);
        }
        else if (iNumPackets < 0)
        {
            // error decoding, so try to match a different key on this tunnel
            NetPrintf(("prototunnel: attempting to rematch tunnel key\n"));
            _ProtoTunnelMatchTunnel(pProtoTunnel, pTunnel, pPacketData, iRecvLen);
        }
    }
    else
    {
        iNumPackets = -2;
    }

    // release exclusive access
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    
    // return number of packets decoded
    return(iNumPackets);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelRecv

    \Description
        Callback to handle idle and recv callbacks on a tunnel socket.

    \Input *pProtoTunnel    - module ref
    \Input pPacketData      - packet data
    \Input iRecvLen         - receive length
    \Input *pRecvAddr       - sender's address

    \Output
        int32_t             - zero

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelRecv(ProtoTunnelRefT *pProtoTunnel, uint8_t *pPacketData, int32_t iRecvLen, struct sockaddr *pRecvAddr)
{
    int32_t iPacket, iNumPackets;
    uint16_t aRemotePortList[PROTOTUNNEL_MAXPORTS];
    uint8_t *pPacketHead;

    // find matching tunnel and decode packets
    if ((iNumPackets = _ProtoTunnelRecvData(pProtoTunnel, aRemotePortList, sizeof(aRemotePortList), pPacketData, iRecvLen, pRecvAddr)) == -1)
    {
        // try again - this will try and match a new crypto key in the case of a decryption error
        iNumPackets = _ProtoTunnelRecvData(pProtoTunnel, aRemotePortList, sizeof(aRemotePortList), pPacketData, iRecvLen, pRecvAddr);
    }

    // no matching tunnel?
    if (iNumPackets == -2)
    {
        NetPrintf(("prototunnel: [recv] received %d byte packet on tunnel socket from unmapped source %a:%d\n", iRecvLen,
            SockaddrInGetAddr(pRecvAddr), SockaddrInGetPort(pRecvAddr)));
        
        // if event callback is set, call it
        if (pProtoTunnel->pCallback != NULL)
        {
            pProtoTunnel->pCallback(pProtoTunnel, PROTOTUNNEL_EVENT_RECVNOMATCH, (char *)pPacketData, iRecvLen, pRecvAddr, pProtoTunnel->pUserData);
        }
    }

    // if no packets
    if (iNumPackets <= 0)
    {
        NetPrintf(("prototunnel: [recv] received unhandled %d bytes from %a:%d\n", iRecvLen,
            SockaddrInGetAddr(pRecvAddr), SockaddrInGetPort(pRecvAddr)));
            
        // display recv info
        if (pProtoTunnel->iVerbosity > 3)
        {
            NetPrintMem(pPacketData, (iRecvLen < 64) ? iRecvLen : 64, "prototunnel-recv-nomatch");
        }
        return(0);
    }

    // display decrypted packet data
    #if DIRTYCODE_LOGGING
    if (pProtoTunnel->iVerbosity > 3)
    {
        NetPrintMem(pPacketData, (iRecvLen < 64) ? iRecvLen : 64, "prototunnel-recv");
    }
    #endif

    // demultiplex aggregate packet data and push into the appropriate sockets
    for (iPacket = 0, pPacketHead = pPacketData + 2, pPacketData = pPacketHead + (iNumPackets * PROTOTUNNEL_PKTHDRSIZE); iPacket < iNumPackets; iPacket++)
    {
        // extract size and port info
        uint32_t uPktHead = (pPacketHead[0] << 8) | pPacketHead[1];
        uint32_t uPktSize = uPktHead >> 4;
        uint32_t uPortIdx = uPktHead & 0xf;
        uint32_t uPort = aRemotePortList[uPortIdx];

        // if it is a control packet, skip it
        if (uPortIdx != PROTOTUNNEL_CONTROL_PORTIDX)
        {
            SocketT *pSocket;

            // find SOCK_DGRAM socket bound to this port
            if (SocketInfo(NULL, 'bndu', uPort, &pSocket, sizeof(pSocket)) == 0)
            {
                // rewrite port to match what socket will be expecting
                SockaddrInSetPort(pRecvAddr, uPort);

                // display tunneled receive
                NetPrintfVerbose((pProtoTunnel->iVerbosity, 2, "prototunnel: [recv] %db->%d\n", uPktSize, uPort));

                /* for Xbox platforms, we need to add two bytes for the VDP header if the socket
                   is an IPPROTO_VDP socket.  we make the assumption here that the receiving side
                   doesn't need to know the value of the encryption header and it is acceptable
                   to have junk data present there. */
                #if PROTOTUNNEL_XBOX_PLATFORM
                if (SocketInfo(pSocket, 'prot', 0, NULL, 0) == IPPROTO_VDP)
                {
                    pPacketData -= 2;
                    uPktSize += 2;
                }
                #endif

                // forward data on to caller
                SocketControl(pSocket, 'push', uPktSize, pPacketData, pRecvAddr);
            }
            else
            {
                NetPrintf(("prototunnel: [recv] warning, got data for port %d with no socket bound to that port\n", uPort));
            }
        }
        else
        {
            NetPrintf(("prototunnel: [recv] received control packet\n"));
        }

        // skip to next packet
        pPacketHead += PROTOTUNNEL_PKTHDRSIZE;
        pPacketData += uPktSize;
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _ProtoTunnelRecvCallback

    \Description
        Callback to handle idle and recv callbacks on a tunnel socket.

    \Input *pSocket - socket ref
    \Input iFlags   - unused
    \Input *_pRef   - tunnel map ref

    \Output
        int32_t     - zero

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelRecvCallback(SocketT *pSocket, int32_t iFlags, void *_pRef)
{
    ProtoTunnelRefT *pProtoTunnel = (ProtoTunnelRefT *)_pRef;
    struct sockaddr RecvAddr;
    int32_t iRecvAddrLen = sizeof(RecvAddr), iRecv, iRecvLen;
    uint8_t aPacketData[PROTOTUNNEL_PACKETBUFFER];

    // got any input?
    for (iRecv = 0; iRecv < 16; iRecv += 1)
    {
        if ((iRecvLen = SocketRecvfrom(pSocket, (char *)aPacketData, sizeof(aPacketData), 0, &RecvAddr, &iRecvAddrLen)) <= 0)
        {
            break;
        }

        // drop packet for easy testing of lost packet recovery
        #if DIRTYCODE_DEBUG
        if (pProtoTunnel->iPacketDrop > 0)
        {
            pProtoTunnel->iPacketDrop -= 1;
            NetPrintf(("prototunnel: dropping received packet\n"));
            continue;
        }
        #endif

        // forward to any registered raw inbound data filter
        if (pProtoTunnel->pRawRecvCallback)
        {
            if (pProtoTunnel->pRawRecvCallback(pSocket, aPacketData, iRecvLen, &RecvAddr, sizeof(RecvAddr), pProtoTunnel->pRawRecvUserData) > 0)
            {
                // inbound raw data was swallowed by the filter, this packet is to be ignored by ProtoTunnel
                continue;
            }
        }

        _ProtoTunnelRecv(pProtoTunnel, aPacketData, iRecvLen, &RecvAddr);
    }


    // update stats
    pProtoTunnel->uNumRecvCalls += iRecv > 0;
    pProtoTunnel->uNumPktsRecvd += (unsigned)iRecv;

    return(0);
}


/*F********************************************************************************/
/*!
    \Function _ProtoTunnelSocketOpen

    \Description
        Open a tunnel socket

    \Input *pProtoTunnel    - pointer to module state
    \Input iPort            - port tunnel will go over

    \Output
        int32_t             - negative=error, else success

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _ProtoTunnelSocketOpen(ProtoTunnelRefT *pProtoTunnel, int32_t iPort)
{
    struct sockaddr BindAddr;
    int32_t iResult;
    
    // open the socket
    if ((pProtoTunnel->pSocket = SocketOpen(AF_INET, SOCK_DGRAM, PROTOTUNNEL_IPPROTO)) == NULL)
    {
        NetPrintf(("prototunnel: unable to open socket\n"));
        return(-1);
    }

    // bind socket to specified port
    SockaddrInit(&BindAddr, AF_INET);
    SockaddrInSetPort(&BindAddr, iPort);
    if ((iResult = SocketBind(pProtoTunnel->pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
    {
        NetPrintf(("prototunnel: error %d binding to port %d, trying random\n", iResult, iPort));
        SockaddrInSetPort(&BindAddr, 0);
        if ((iResult = SocketBind(pProtoTunnel->pSocket, &BindAddr, sizeof(BindAddr))) != SOCKERR_NONE)
        {
            NetPrintf(("prototunnel: error %d binding to port\n", iResult));
            SocketClose(pProtoTunnel->pSocket);
            pProtoTunnel->pSocket = NULL;
            return(-1);
        }
    }

    // retrieve bound port
    SocketInfo(pProtoTunnel->pSocket, 'bind', 0, &BindAddr, sizeof(BindAddr));
    iPort = SockaddrInGetPort(&BindAddr);
    NetPrintf(("prototunnel: bound socket to port %d\n", iPort));

    // save local port
    pProtoTunnel->uTunnelPort = iPort;

    // set up for socket callback events
    #if SOCKET_ASYNCRECVTHREAD
    SocketCallback(pProtoTunnel->pSocket, CALLB_RECV, 100, pProtoTunnel, &_ProtoTunnelRecvCallback);
    #endif

    return(0);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function ProtoTunnelCreate

    \Description
        Create the ProtoTunnel module.

    \Input iMaxTunnels      - maximum number of tunnels module can allocate
    \Input iTunnelPort      - local port for socket all tunnels will use

    \Output
        ProtoTunnelRefT *   - pointer to new module, or NULL

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
ProtoTunnelRefT *ProtoTunnelCreate(int32_t iMaxTunnels, int32_t iTunnelPort)
{
    ProtoTunnelRefT *pProtoTunnel;
    int32_t iRefSize = sizeof(*pProtoTunnel) + ((iMaxTunnels-1) * sizeof(ProtoTunnelT));
    int32_t iMemGroup;
    void *pMemGroupUserData;
    
    // Query current mem group data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    
    // allocate and init module state
    if ((pProtoTunnel = DirtyMemAlloc(iRefSize, PROTOTUNNEL_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        NetPrintf(("prototunnel: could not allocate module state\n"));
        return(NULL);
    }
    memset(pProtoTunnel, 0, iRefSize);
    pProtoTunnel->iMemGroup = iMemGroup;
    pProtoTunnel->pMemGroupUserData = pMemGroupUserData;
    pProtoTunnel->iMaxTunnels = iMaxTunnels;
    pProtoTunnel->uTunnelPort = iTunnelPort;
    pProtoTunnel->uVirtualAddr = PROTOTUNNEL_BASEADDRESS;

    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    pProtoTunnel->eMode = PROTOTUNNEL_MODE_XENON;
    #else
    pProtoTunnel->eMode = PROTOTUNNEL_MODE_GENERIC;
    #endif

    // create the tunnel socket
    if (_ProtoTunnelSocketOpen(pProtoTunnel, iTunnelPort) < 0)
    {
        DirtyMemFree(pProtoTunnel, PROTOTUNNEL_MEMID, pProtoTunnel->iMemGroup, pProtoTunnel->pMemGroupUserData);
        return(NULL);
    }

    // initialize critical sections
    NetCritInit(&pProtoTunnel->TunnelsCritS, "prototunnel-global-send");
    NetCritInit(&pProtoTunnel->TunnelsCritR, "prototunnel-global-recv");

    // restrict max udp packet size
    SocketControl(NULL, 'maxp', PROTOTUNNEL_MAXPACKET, NULL, NULL);
    
    // hook into global socket send hook
    SocketControl(NULL, 'sdcb', 0, (void *)_ProtoTunnelSendCallback, pProtoTunnel);

    // initialize timers
    pProtoTunnel->uLastFlush = NetTick();

    // return ref to caller
    return(pProtoTunnel);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelDestroy

    \Description
        Destroy the ProtoTunnel module.

    \Input *pProtoTunnel    - pointer to module state

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoTunnelDestroy(ProtoTunnelRefT *pProtoTunnel)
{
    // clear global socket send hook
    SocketControl(NULL, 'sdcb', 0, NULL, NULL);

    // close tunnel socket
    SocketClose(pProtoTunnel->pSocket);

    // dispose of critical sections
    NetCritKill(&pProtoTunnel->TunnelsCritR);
    NetCritKill(&pProtoTunnel->TunnelsCritS);

    // dispose of module memory
    DirtyMemFree(pProtoTunnel, PROTOTUNNEL_MEMID, pProtoTunnel->iMemGroup, pProtoTunnel->pMemGroupUserData);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelCallback

    \Description
        Set event callback

    \Input *pProtoTunnel    - pointer to module state
    \Input *pCallback       - callback pointer
    \Input *pUserData       - callback data

    \Version 03/24/2006 (jbrookes)
*/
/********************************************************************************F*/
void ProtoTunnelCallback(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelCallbackT *pCallback, void *pUserData)
{
    pProtoTunnel->pCallback = pCallback;
    pProtoTunnel->pUserData = pUserData;
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelAlloc

    \Description
        Allocate a tunnel.

    \Input *pProtoTunnel    - pointer to module state
    \Input *pInfo           - tunnel info
    \Input *pKey            - encryption key for tunnel

    \Output
        int32_t             - negative=error, else allocated tunnel id

    \Version 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelAlloc(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelInfoT *pInfo, const char *pKey)
{
    ProtoTunnelT *pTunnel;
    int32_t iTunnel;

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // see if we already have a tunnel with this clientId
    for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        pTunnel = &pProtoTunnel->Tunnels[iTunnel];
        if (pTunnel->Info.uRemoteClientId == pInfo->uRemoteClientId)
        {
            int32_t iKey, iKeySlot, iResult = (signed)pTunnel->uVirtualAddr;

            // refcount the tunnel
            NetPrintf(("prototunnel: refcounting tunnel with id=0x%08x key=%s clientId=0x%08x remote address=%a\n", pTunnel->uVirtualAddr, pKey, pInfo->uRemoteClientId, pInfo->uRemoteAddr));
            pTunnel->uRefCount += 1;
            
            // append key to key list
            for (iKey = 0, iKeySlot = -1; iKey < PROTOTUNNEL_MAXKEYS; iKey++)
            {
                #if DIRTYCODE_LOGGING
                if (!strcmp(pKey, pTunnel->aKeyList[iKey]))
                {
                    NetPrintf(("prototunnel: warning - duplicate key %s on alloc\n", pKey));
                }
                #endif
                if ((iKeySlot == -1) && (pTunnel->aKeyList[iKey][0] == '\0'))
                {
                    iKeySlot = iKey;
                }
            }
            if (iKeySlot != -1)
            {
                strnzcpy(pTunnel->aKeyList[iKeySlot], pKey, sizeof(pTunnel->aKeyList[iKeySlot]));
            }
            else
            {
                NetPrintf(("prototunnel: error - key overflow on alloc\n"));
                iResult = -1;
            }

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
            if (pTunnel->Info.uRemoteAddrFallbackCount < PROTOTUNNEL_MAXADDR)
            {
                pTunnel->Info.uRemoteAddrFallback[pTunnel->Info.uRemoteAddrFallbackCount++] = pInfo->uRemoteAddr;
            }
#endif

            NetCritLeave(&pProtoTunnel->TunnelsCritR);
            NetCritLeave(&pProtoTunnel->TunnelsCritS);

            return(iResult);
        }
    }

    for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
    {
        if (pProtoTunnel->Tunnels[iTunnel].uVirtualAddr == 0)
        {
            break;
        }
    }

    // make sure we found room
    if (iTunnel == pProtoTunnel->iMaxTunnels)
    {
        NetPrintf(("prototunnel: could not allocate a new tunnel\n"));
        
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);
        
        return(-1);
    }

    pTunnel = &pProtoTunnel->Tunnels[iTunnel];

    memset(pTunnel, 0, sizeof(*pTunnel));
    memcpy(&pTunnel->Info, pInfo, sizeof(pTunnel->Info));
    NetCritInit(&pTunnel->PacketCrit, "prototunnel-tunnel");
    CryptArc4Init(&pTunnel->CryptSendState, (unsigned char *)pKey, (int32_t)strlen(pKey), 1);
    strnzcpy(pTunnel->aKeyList[0], pKey, sizeof(pTunnel->aKeyList[0]));
    pTunnel->uRefCount = 1;

    // port index seven is reserved by ProtoTunnel and always encrypted
    pTunnel->Info.aPortFlags[PROTOTUNNEL_CONTROL_PORTIDX] = PROTOTUNNEL_PORTFLAG_ENCRYPTED;

    // initialize to send connect info
    pTunnel->bSendCtrlInfo = TRUE;
    
    // assign a virtual address/id
    pTunnel->uVirtualAddr = pProtoTunnel->uVirtualAddr++;

    // if unspecified, set remote port
    if (pTunnel->Info.uRemotePort == 0)
    {
        pTunnel->Info.uRemotePort = pProtoTunnel->uTunnelPort;
    }

    #if DIRTYCODE_LOGGING
    pTunnel->uLastStatUpdate = NetTick();
    #endif

    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);

    // debug spam
    #if DIRTYCODE_LOGGING
    {
        int32_t iPort;
        NetPrintf(("prototunnel: creating map to remote client %a:%d (id=0x%08x)\n", pTunnel->Info.uRemoteAddr, pTunnel->Info.uRemotePort, pTunnel->Info.uRemoteClientId));
        for (iPort = 0; (iPort < PROTOTUNNEL_MAXPORTS) && (pTunnel->Info.aRemotePortList[iPort] != 0); iPort++)
        {
            NetPrintf(("prototunnel: [%d] %d\n", iPort, pTunnel->Info.aRemotePortList[iPort]));
        }
    }
    #endif
    
    // return addr/id to caller
    NetPrintf(("prototunnel: allocated tunnel with id 0x%08x key=%s\n", pTunnel->uVirtualAddr, pTunnel->aKeyList[0]));
    return(pTunnel->uVirtualAddr);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelFree

    \Description
        Free a tunnel.

    \Input *pProtoTunnel    - pointer to module state
    \Input uTunnelId        - id of tunnel to free
    \Input *pKey            - key tunnel was allocated with

    \Notes
        pKey is only required when tunnel refcounting is used.  Otherwise, pKey
        may be specified as NULL.

    \Version 12/07/2005 (jbrookes)
*/
/********************************************************************************F*/
uint32_t ProtoTunnelFree(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, const char *pKey)
{
    return(ProtoTunnelFree2(pProtoTunnel, uTunnelId, pKey, 0));
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelFree2

    \Description
        Free a tunnel. Same as ProtoTunnelFree but takes an IP address. Used on 360
        to allow ProtoTunnel to maintain a list of secure IP addresses for sending.
        When refcounting down a tunnel, the current secure IP address will be 
        replaced by another valid one, reaching the same peer.

    \Input *pProtoTunnel    - pointer to module state
    \Input uTunnelId        - id of tunnel to free
    \Input *pKey            - key tunnel was allocated with
    \Input uAddr            - address of peer that is being freed

    \Notes
        pKey is only required when tunnel refcounting is used.  Otherwise, pKey
        may be specified as NULL.

    \Version 03/15/2010 (jrainy)
*/
/********************************************************************************F*/
uint32_t ProtoTunnelFree2(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, const char *pKey, uint32_t uAddr)
{
    ProtoTunnelT *pTunnel;
    int32_t iTunnel;
    uint32_t uRet = (uint32_t)-1;
    #if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    int32_t iAddr=0;
    #endif
    #if DIRTYCODE_LOGGING
    uint32_t bFound = FALSE;
    #endif

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // find tunnel id
    for (iTunnel = 0, pTunnel = &pProtoTunnel->Tunnels[0]; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++, pTunnel++)
    {
        // found it?
        if (pTunnel->uVirtualAddr == uTunnelId)
        {
            #if DIRTYCODE_LOGGING
            bFound = TRUE;
            #endif

            // deallocate the tunnel
            if (pTunnel->uRefCount == 1)
            {
                NetPrintf(("prototunnel: freeing tunnel 0x%08x\n", uTunnelId));

                // flush buffer before destroy
                _ProtoTunnelBufferSend(pProtoTunnel, pTunnel);
            
                // dispose of critical section
                NetCritKill(&pTunnel->PacketCrit);

                memset(pTunnel, 0, sizeof(*pTunnel));
 
                uRet = 0;
            }
            else
            {
                int32_t iKey;

                NetPrintf(("prototunnel: decrementing refcount of tunnel 0x%08x (clientId=0x%08x)\n", uTunnelId, pTunnel->Info.uRemoteClientId));

                // remove key from key list
                for (iKey = 0; iKey < PROTOTUNNEL_MAXKEYS; iKey++)
                {
                    if (!strcmp(pKey, pTunnel->aKeyList[iKey]))
                    {
                        memset(pTunnel->aKeyList[iKey], 0, sizeof(pTunnel->aKeyList[iKey]));
                        break;
                    }
                }
                
                // did we blow away our send key?
                if (pTunnel->uSendKey == (uint8_t)iKey)
                {
                    NetPrintf(("prototunnel: free of current send key; picking another\n"));
                    for (iKey = 0; iKey < PROTOTUNNEL_MAXKEYS; iKey++)
                    {
                        if (pTunnel->aKeyList[iKey][0] != '\0')
                        {
                            NetPrintf(("prototunnel: picking key %d (%s) offset=%d\n", iKey, pTunnel->aKeyList[iKey], pTunnel->uSendOffset));
                            CryptArc4Init(&pTunnel->CryptSendState, (unsigned char *)pTunnel->aKeyList[iKey], (int32_t)strlen(pTunnel->aKeyList[iKey]), 1);
                            CryptArc4Advance(&pTunnel->CryptSendState, pTunnel->uSendOffset<<PROTOTUNNEL_CRYPTBITS);
                            pTunnel->uSendKey = (uint8_t)iKey;
                            break;
                        }
                    }
                }
                else if (iKey == PROTOTUNNEL_MAXKEYS)
                {
                    NetPrintf(("prototunnel: could not find key %s in key list on free\n", pKey));
                }

                pTunnel->uRefCount -= 1;
                uRet = pTunnel->uRefCount;

                NetPrintf(("prototunnel: refcounting down tunnel with id=0x%08x key=%s clientId=0x%08x remote address=%a\n", pTunnel->uVirtualAddr, pKey, pTunnel->Info.uRemoteClientId, pTunnel->Info.uRemoteAddr));

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
                for(iAddr = 0; iAddr < pTunnel->Info.uRemoteAddrFallbackCount; iAddr++)
                {
                    if (pTunnel->Info.uRemoteAddrFallback[iAddr] == uAddr)
                    {
                        break;
                    }
                }

                if (iAddr != pTunnel->Info.uRemoteAddrFallbackCount)
                {
                    // assign over the freed address the last from the fallback list
                    pTunnel->Info.uRemoteAddrFallback[iAddr] = pTunnel->Info.uRemoteAddrFallback[pTunnel->Info.uRemoteAddrFallbackCount - 1];
                    pTunnel->Info.uRemoteAddrFallbackCount--;
                }
                // if the freed address is not in the fallback list, it has to be the current address
                else if (uAddr == pTunnel->Info.uRemoteAddr)
                {
                    if (pTunnel->Info.uRemoteAddrFallbackCount)
                    {
                        // don't merge those two lines. SocketInAddrGetText return a single buffer, can't have two at the same time.
                        NetPrintf(("prototunnel: secure address replacement from %a to %a\n", pTunnel->Info.uRemoteAddr, pTunnel->Info.uRemoteAddrFallback[pTunnel->Info.uRemoteAddrFallbackCount - 1]));

                        pTunnel->Info.uRemoteAddr = pTunnel->Info.uRemoteAddrFallback[pTunnel->Info.uRemoteAddrFallbackCount - 1];
                        pTunnel->Info.uRemoteAddrFallbackCount--;
                    }
                    else
                    {
                        NetPrintf(("prototunnel: freeing tunnel using last secure address\n"));
                    }
                }
                else
                {
                    NetPrintf(("prototunnel: Warning: ProtoTunnelFree address is not current, nor in fallback list\n"));
                }
#endif
            }
            
            // done
            break;
        }
    }

    #if DIRTYCODE_LOGGING
    if (bFound == FALSE)
    {
        NetPrintf(("prototunnel: could not find tunnel with id 0x%08x to free\n", uTunnelId));
    }
    #endif

    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);
    return(uRet);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelUpdatePortList

    \Description
        Updates the port list for the given tunnel.  Port and port flag info is
        copied over from the specified info structure if the port value is non-
        zero.

    \Input *pProtoTunnel    - pointer to module state
    \Input uTunnelId        - id of tunnel to update port mapping for
    \Input *pInfo           - structure containing updated port info

    \Output
        int32_t             - zero=success, else could not find tunnel with given id

    \Version 06/12/2008 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelUpdatePortList(ProtoTunnelRefT *pProtoTunnel, uint32_t uTunnelId, ProtoTunnelInfoT *pInfo)
{
    ProtoTunnelT *pTunnel;
    int32_t iTunnel, iResult = -1;
    int32_t iPort;

    // get exclusive access to tunnel list
    NetCritEnter(&pProtoTunnel->TunnelsCritS);
    NetCritEnter(&pProtoTunnel->TunnelsCritR);

    // find tunnel id
    for (iTunnel = 0, pTunnel = &pProtoTunnel->Tunnels[0]; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++, pTunnel++)
    {

        // found it?
        if (pTunnel->uVirtualAddr == uTunnelId)
        {

            // copy portlist items that should be updated
            for (iPort = 0; iPort < PROTOTUNNEL_MAXPORTS; iPort += 1)
            {

                if (pInfo->aRemotePortList[iPort] != 0)
                {
                    NetPrintf(("prototunnel: updating port mapping %d for tunnel=0x%08x from (%d,%d) to (%d,%d)\n", 
                        iPort, uTunnelId,
                        pTunnel->Info.aRemotePortList[iPort], pTunnel->Info.aPortFlags[iPort],
                        pInfo->aRemotePortList[iPort], pInfo->aPortFlags[iPort]));
                    pTunnel->Info.aRemotePortList[iPort] = pInfo->aRemotePortList[iPort];
                    pTunnel->Info.aPortFlags[iPort] = pInfo->aPortFlags[iPort];
                }
            }
            iResult = 0;
            break;
        }
    }
    
    // release exclusive access to tunnel list
    NetCritLeave(&pProtoTunnel->TunnelsCritR);
    NetCritLeave(&pProtoTunnel->TunnelsCritS);
    
    // return result code to caller
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelValidatePacket

    \Description
        Validate key against packet, a tunnel has not been allocated yet

    \Input *pProtoTunnel- pointer to module state
    \Input *pTunnel     - pointer to tunnel
    \Input *pOutputData - [out] pointer to buffer to store decrypted data (may be NULL)
    \Input *pPacketData - pointer to tunnel packet
    \Input iPacketSize  - size of tunnel packet
    \Input *pKey        - encryption key for tunnel

    \Output
        int32_t         - negative=error, else success

    \Version 06/06/2006 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelValidatePacket(ProtoTunnelRefT *pProtoTunnel, ProtoTunnelT *pTunnel, uint8_t *pOutputData, const uint8_t *pPacketData, int32_t iPacketSize, const char *pKey)
{
    uint8_t aPacketData[PROTOTUNNEL_PACKETBUFFER], *_pPacketData;
    int32_t iEncryptedSize, iNumPackets, iPacketOffset, iSubPacketSize;
    CryptArc4T CryptRecvState;
    uint32_t uRecvOffset;
    
    // if not generic mode, no decrypt required
    if (pProtoTunnel->eMode != PROTOTUNNEL_MODE_GENERIC)
    {
        // copy to output buffer?
        if (pOutputData != NULL)
        {
            memcpy(pOutputData, pPacketData, iPacketSize);
        }
        // validate packet
        return(_ProtoTunnelValidatePacket(pPacketData, iPacketSize));
    }

    // copy packet data to temp buffer
    memcpy(aPacketData, pPacketData, iPacketSize);
    _pPacketData = aPacketData;

    /* get stream offset from packet header - this tells us
    where we need to be to be able to decrypt the packet */
    uRecvOffset = (_pPacketData[0] << 8) | _pPacketData[1];

    // initialize the crypt receive state
    CryptArc4Init(&CryptRecvState, (unsigned char *)pKey, (int32_t)strlen(pKey), 1);

    // lost data?  sync the cipher
    if (uRecvOffset != 0)
    {
        // calc how many data units we've missed
        int32_t iRecvDiff = (signed)(uRecvOffset - 0);

        // advance the cipher to resync
        NetPrintf(("prototunnel: stream skip %d bytes\n", iRecvDiff<<PROTOTUNNEL_CRYPTBITS));
        CryptArc4Advance(&CryptRecvState, iRecvDiff<<PROTOTUNNEL_CRYPTBITS);
    }

    // reset offset to count bytes added from this packet
    uRecvOffset = 0;

    // skip stream offset
    _pPacketData += 2;
    iPacketOffset = 2;

    // iterate through packet headers
    for (iNumPackets = 0, iEncryptedSize = 0; iPacketOffset < iPacketSize; iNumPackets++)
    {
        // decrypt the packet header
        CryptArc4Apply(&CryptRecvState, _pPacketData, PROTOTUNNEL_PKTHDRSIZE);
        uRecvOffset += PROTOTUNNEL_PKTHDRSIZE;

        // extract size and port info
        iSubPacketSize = (_pPacketData[0] << 4) | (_pPacketData[1] >> 4);
        iPacketOffset += iSubPacketSize + PROTOTUNNEL_PKTHDRSIZE;

        if (pTunnel->Info.aPortFlags[_pPacketData[1] & 0xf] & PROTOTUNNEL_PORTFLAG_ENCRYPTED)
        {
            iEncryptedSize += iSubPacketSize;
        }

        // increment to next packet header
        _pPacketData += PROTOTUNNEL_PKTHDRSIZE;
    }

    // make sure size matches
    if (iPacketOffset != iPacketSize)
    {
        NetPrintf(("prototunnel: [recv] mismatched size in received packet (%d received, %d decoded)\n", iPacketSize, iPacketOffset));
        return(-1);
    }

    // copy to output buffer?
    if (pOutputData != NULL)
    {
        // decrypt encrypted packet data
        if (iEncryptedSize > 0)
        {
            CryptArc4Apply(&CryptRecvState, _pPacketData, iEncryptedSize);
        }
        memcpy(pOutputData, aPacketData, iPacketSize);
    }

    // return number of packets decoded
    return(iNumPackets);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelStatus

    \Description
        Get module status

    \Input *pProtoTunnel    - pointer to module state
    \Input iSelect          - status selector
    \Input iValue           - selector specific
    \Input *pBuf            - [out] - selector specific
    \Input iBufSize         - size of output buffer

    \Output
        int32_t             - selector specific

    \Notes
        iControl can be one of the following:

        \verbatim
            'lprt' - return local socket port
            'rcvs' - copy receive statistics to pBuf
            'rcal' - return number of recv calls
            'rprt' - remote game port for the specificed tunnel id
            'rsub' - return number of sub-packets received
            'rtot' - return number of packets received
            'snds' - copy send statistics to pBuf
            'sock' - copy socket ref to pBuf
            'vtop' - convert virtual address to physical address (pBuf == sockaddr, optional)
        \endverbatim

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelStatus(ProtoTunnelRefT *pProtoTunnel, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    if (iSelect == 'lprt')
    {
        return(pProtoTunnel->uTunnelPort);
    }
    if (iSelect == 'rcal')
    {
        return(pProtoTunnel->uNumRecvCalls);
    }
    if (iSelect == 'rprt')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(uint16_t)))
        {
            int32_t iTunnel = _ProtoTunnelIndexFromId(pProtoTunnel, (uint32_t)iValue);
            if (iTunnel != -1)
            {
                *(uint16_t *)pBuf = pProtoTunnel->Tunnels[iTunnel].Info.uRemotePort;
                return(0);
            }
            else
            {
                NetPrintf(("prototunnel: 'rprt' status selector called with unknown tunnelid=0x%08x\n", iValue));
                return(-1);
            }
        }
    }
    if (iSelect == 'rsub')
    {
        return(pProtoTunnel->uNumSubPktsRecvd);
    }
    if (iSelect == 'rtot')
    {
        return(pProtoTunnel->uNumPktsRecvd);
    }
    if (iSelect == 'rcvs')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(ProtoTunnelStatT)) && (iValue < pProtoTunnel->iMaxTunnels))
        {
            memcpy(pBuf, &pProtoTunnel->Tunnels[iValue].RecvStat, sizeof(ProtoTunnelStatT));
            return(0);
        }
    }
    if (iSelect == 'snds')
    {
        if ((pBuf != NULL) && (iBufSize == sizeof(ProtoTunnelStatT)) && (iValue < pProtoTunnel->iMaxTunnels))
        {
            memcpy(pBuf, &pProtoTunnel->Tunnels[iValue].SendStat, sizeof(ProtoTunnelStatT));
            return(0);
        }
    }
    if (iSelect == 'sock')
    {
        memcpy(pBuf, &pProtoTunnel->pSocket, iBufSize);
        return(0);
    }
    if (iSelect == 'vtop')
    {
        return(_ProtoTunnelVirtualToPhysical(pProtoTunnel, iValue, pBuf, iBufSize));
    }
    // selector not supported
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function ProtoTunnelControl

    \Description
        Control the module

    \Input *pProtoTunnel    - pointer to module state
    \Input iControl         - control selector
    \Input iValue           - selector specific
    \Input iValue2          - selector specific
    \Input *pValue          - selector specific

    \Output
        int32_t             - selector result

    \Notes
        iControl can be one of the following:

        \verbatim
            'bind' - recreate tunnel socket bound to specified port
            'clid' - set local clientId for all tunnels
            'drop' - drop next iValue packets (debug only; used to test crypt recovery)
            'flsh' - flush the specified tunnelId
            'mode' - set mode (PROTOTUNNEL_MODE_*) prototunnel is operating under
            'rate' - set flush rate in milliseconds; defaults to 16ms
            'rprt' - set specified tunnel's remote port
            'rrcb' - set raw receive callback
            'rrud' - set raw receive user data
            'spam' - set verbosity level (debug only)
            'tcid' - set per-tunnel local clientId override for specific tunnel
        \endverbatim

        Unrecognized selectors are passed through to SocketControl()

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ProtoTunnelControl(ProtoTunnelRefT *pProtoTunnel, int32_t iControl, int32_t iValue, int32_t iValue2, const void *pValue)
{
    if (iControl == 'bind')
    {
        NetPrintf(("prototunnel: recreating tunnel socket bound to port %d\n", iValue));

        // acquire exclusive access to tunnel list
        NetCritEnter(&pProtoTunnel->TunnelsCritS);
        NetCritEnter(&pProtoTunnel->TunnelsCritR);

        // close tunnel socket
        if (pProtoTunnel->pSocket != NULL)
        {
            SocketClose(pProtoTunnel->pSocket);
            pProtoTunnel->pSocket = NULL;
        }
        
        // recreate tunnel socket bound to new port
        _ProtoTunnelSocketOpen(pProtoTunnel, iValue);
        
        // release exclusive access to tunnel list
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);
        return(0);
    }
    if (iControl == 'clid')
    {
        NetPrintf(("prototunnel: setting local clientId=0x%08x\n", iValue));
        pProtoTunnel->uLocalClientId = iValue;
        return(0);
    }
    #if DIRTYCODE_DEBUG
    if (iControl == 'drop')
    {
        pProtoTunnel->iPacketDrop = iValue;
        return(0);
    }
    #endif
    if ((iControl == 'flsh') || (iControl == 'rprt'))
    {
        ProtoTunnelT *pTunnel;
        int32_t iTunnel;
        
        // acquire exclusive access to tunnel list
        NetCritEnter(&pProtoTunnel->TunnelsCritS);
        NetCritEnter(&pProtoTunnel->TunnelsCritR);

        // flush specified tunnel
        for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
        {
            pTunnel = &pProtoTunnel->Tunnels[iTunnel];
            if (pTunnel->uVirtualAddr == (unsigned)iValue)
            {
                if (iControl == 'flsh')
                {
                    NetPrintf(("prototunnel: explicitly flushing tunnel 0x%08x\n", pTunnel->uVirtualAddr));
                    _ProtoTunnelBufferSend(pProtoTunnel, pTunnel);
                }
                if (iControl == 'rprt')
                {
                    NetPrintf(("prototunnel: updating remote port for tunnel 0x%08x to %d\n", pTunnel->uVirtualAddr, iValue2));
                    pTunnel->Info.uRemotePort = (unsigned)iValue2;
                }
                break;
            }
        }

        // release exclusive access to tunnel list
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);
        
        // if we didn't find the tunnel
        if (iTunnel == pProtoTunnel->iMaxTunnels)
        {
            NetPrintf(("prototunnel: unable to find tunnel 0x%08x for '%c%c%c%c' operation\n", iValue,
                (uint8_t)(iControl>>24), (uint8_t)(iControl>>16), (uint8_t)(iControl>>8), (uint8_t)iControl));
            return(-1);
        }
        return(0);
    }
    if (iControl == 'mode')
    {
        pProtoTunnel->eMode = iValue;
        return(0);
    }
    if (iControl == 'rate')
    {
        pProtoTunnel->uFlushRate = iValue;
        return(0);
    }
    if (iControl == 'rrcb')
    {
        NetPrintf(("prototunnel: 'rrcb' selector used to change raw recv callback from %p to %p\n", pProtoTunnel->pRawRecvCallback, pValue));
        pProtoTunnel->pRawRecvCallback = (RawRecvCallbackT *)pValue;
        return(0);
    }
    if (iControl == 'rrud')
    {
        NetPrintf(("prototunnel: 'rrud' selector used to change raw recv callback user data from %p to %p\n", pProtoTunnel->pRawRecvUserData, pValue));
        pProtoTunnel->pRawRecvUserData = (void *)pValue;
        return(0);
    }
    if (iControl == 'spam')
    {
        pProtoTunnel->iVerbosity = iValue;
        return(0);
    }
    if (iControl == 'tcid')
    {
        int32_t iTunnel = _ProtoTunnelIndexFromId(pProtoTunnel, (uint32_t)iValue);
        if (iTunnel != -1)
        {
            pProtoTunnel->Tunnels[iTunnel].uLocalClientId = (uint32_t)iValue2;
            return(0);
        }
        else
        {
            NetPrintf(("prototunnel: 'tcid' control selector called with unknown tunnelid=0x%08x\n", iValue));
            return(-1);
        }
    }
    // pass-through to SocketControl()
    return(SocketControl(NULL, iControl, iValue, NULL, NULL));
}

/*F********************************************************************************/
/*!

    \Function ProtoTunnelUpdate

    \Description
        Update the module

    \Input *pProtoTunnel    - pointer to module state

    \Version 12/02/2005 (jbrookes)
*/
/********************************************************************************F*/
void ProtoTunnelUpdate(ProtoTunnelRefT *pProtoTunnel)
{
    uint32_t uTick = NetTick();

    // for clients without async recv thread support, poll
    #if !SOCKET_ASYNCRECVTHREAD
    _ProtoTunnelRecvCallback(pProtoTunnel->pSocket, 0, pProtoTunnel);
    #endif

    // time to flush?
    if (NetTickDiff(uTick, pProtoTunnel->uLastFlush) >= (signed)pProtoTunnel->uFlushRate)
    {
        int32_t iTunnel;

        // acquire exclusive access to tunnel list
        NetCritEnter(&pProtoTunnel->TunnelsCritS);
        NetCritEnter(&pProtoTunnel->TunnelsCritR);

        // flush all tunnels
        for (iTunnel = 0; iTunnel < pProtoTunnel->iMaxTunnels; iTunnel++)
        {
            if (pProtoTunnel->Tunnels[iTunnel].uVirtualAddr)
            {
                _ProtoTunnelBufferSend(pProtoTunnel, &pProtoTunnel->Tunnels[iTunnel]);
            }
        }

        // release exclusive access to tunnel list
        NetCritLeave(&pProtoTunnel->TunnelsCritR);
        NetCritLeave(&pProtoTunnel->TunnelsCritS);

        // update last flush
        pProtoTunnel->uLastFlush = uTick;
    }
}

/*F*************************************************************************************/
/*!
    \Function    ProtoTunnelRawSendto

    \Description
        Send data to a remote host over the prototunnel socket. The destination address
        is supplied along with the data. Important: remote host shall not be expecting
        tunneled data because that function bypasses all the tunneling logic on the sending
        side.

    \Input *pProtoTunnel    - pointer to module state
    \Input *pBuf            - the data to be sent
    \Input iLen             - size of data
    \Input *pTo             - the address to send to
    \Input iToLen           - length of address

    \Output
        int32_t             - number of bytes sent or standard network error code (SOCKERR_xxx)

    \Version 07/12/2011 (mclouatre)
*/
/************************************************************************************F*/
int32_t ProtoTunnelRawSendto(ProtoTunnelRefT *pProtoTunnel, const char *pBuf, int32_t iLen, const struct sockaddr *pTo, int32_t iToLen)
{
    if (pProtoTunnel->pSocket)
    {
        return(SocketSendto(pProtoTunnel->pSocket, pBuf, iLen, 0, pTo, iToLen));
    }
    else
    {
        return(SOCKERR_INVALID);
    }
}
