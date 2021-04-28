/*H********************************************************************************/
/*!
    \File voippacket.h

    \Description
        VoIP data packet definitions.

    \Copyright
        Copyright (c) Electronic Arts 2004. ALL RIGHTS RESERVED.

    \Version 1.0 03/17/2004 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voippacket_h
#define _voippacket_h

/*** Include files ****************************************************************/

#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
#include <xhv2.h>
#endif

/*** Defines **********************************************************************/

//! maximum voice sub-packets in a single voip packet (can't go beyond 16 if size of pPacket->aDataMap[] is not adjusted)
#define VOIP_MAXSUBPKTS_PER_PKT             (16)

//! maximum voice sub-packet payload size
#define VOIP_MAXPKTSIZE                     (SOCKET_MAXUDPRECV - sizeof(VoipPacketHeadT) - sizeof(VoipMicrInfoT))

//! VoIP Flags bit field included in VoIP packets
#define VOIP_PACKET_STATUS_FLAG_HEADSETOK   (1)


/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! VoIP data packet header
typedef struct VoipPacketHeadT
{
#if DIRTYCODE_PLATFORM == DIRTYCODE_XENON
    uint16_t iGameLen;      //!< VDP header
#endif
    uint8_t aType[3];       //!< type/version info
    uint8_t _pad;
    uint8_t aClientId[4];   //!< source clientId
    uint8_t aFlags[2];      //!< field to store system state ie. headset status.
} VoipPacketHeadT;

//! VoIP connection packet
typedef struct VoipConnPacketT
{
    VoipPacketHeadT Head;
    uint8_t         aRemoteClientId[4];
    uint8_t         aSessionId[4];
    uint8_t         bConnected;
    int8_t          iPrimaryLocalUser;
    int8_t          iPrimaryRemoteUser;
    uint8_t         pad;
    VoipUserT       LocalUsers[VOIP_MAXLOCALUSERS];
    VoipUserT       RemoteUsers[VOIP_MAXLOCALUSERS];
} VoipConnPacketT;

//! VoIP ping packet
typedef struct VoipPingPacketT
{
    VoipPacketHeadT Head;
    uint8_t         aRemoteClientId[4];
    uint8_t         aChannelId[4];                  //!< packet channelId
} VoipPingPacketT;

//! VoIP disc packet
typedef struct VoipDiscPacketT
{
    VoipPacketHeadT Head;
    uint8_t         aRemoteClientId[4];
    uint8_t         aSessionId[4];
} VoipDiscPacketT;

//! VoIP micr packet info
typedef struct VoipMicrInfoT
{
    uint8_t         aSendMask[4];                   //!< lsb->msb, one bit per user, used for voip server
    uint8_t         aChannelId[4];                  //!< packet channelId
    uint8_t         aDataMap[4];                    //!< lsb->msb, two bits per sub-packet
    uint8_t         uSeqn;                          //!< voice packet sequence number
    uint8_t         uNumSubPackets;                 //!< current number of sub-packets packed in this packet
    uint8_t         uSubPacketSize;                 //!< size of sub-packets (same fixed-size for all sub-packets)
} VoipMicrInfoT;

//! VoIP data packet
typedef struct VoipMicrPacketT
{
    VoipPacketHeadT Head;                           //!< packet header
    VoipMicrInfoT   MicrInfo;                       //!< micr packet info
    uint8_t         aData[VOIP_MAXPKTSIZE];         //!< voice packet data (this is where sub-packets are bundled)
} VoipMicrPacketT;

//! VoipPacketBufferT: a buffer that can(does) contain any type of voip packet
typedef union VoipPacketBufferT
{
    VoipPacketHeadT VoipPacketHead;
    VoipConnPacketT VoipConnPacket;
    VoipPingPacketT VoipPingPacket;
    VoipDiscPacketT VoipDiscPacket;
    VoipMicrPacketT VoipMicrPacket;
} VoipPacketBufferT;


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#endif // _voippacket_h

