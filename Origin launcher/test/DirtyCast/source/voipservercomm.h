/*H********************************************************************************/
/*!
    \File voipservercomm.h

    \Description
        This module contains a set of utility functions around working with the
        comm modules that are used on the voipserver

    \Copyright
        Copyright (c) Electronic Arts 2015. ALL RIGHTS RESERVED.

    \Version 09/17/2015 (eesponda)
*/
/********************************************************************************H*/

#ifndef _voipservercomm_h
#define _voipservercomm_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

/*** Defines **********************************************************************/

#define VOIPSERVER_PACKET_OVERHEAD (28) //!< basic IP+UDP header size

/*** Macros ***********************************************************************/

// adds the overhead to the size of the packet
#define VOIPSERVER_EstimateNetworkPacketSize(uPacketSize) ((uPacketSize) + (VOIPSERVER_PACKET_OVERHEAD))

/*** Type Definitions *************************************************************/

//! forward declarations
typedef struct SocketT SocketT;

//! The callback fired when data is received on the opened socket
typedef int32_t (SocketRecvCb)(SocketT *pSocket, int32_t iFlags, void *pUserData);

//! commudp packet header
typedef struct CommUDPPacketHeadT
{
    uint8_t aSeq[4];            //!< sequence number/connection packet kind
    uint8_t aAck[4];            //!< acknowledgement/connection identifier
    uint8_t uClientId[4];       //!< client identifier (only in INIT or POKE packets)
} CommUDPPacketHeadT;

//! commudp meta1 packet header (comes after packet header, if meta type is set to 1)
typedef struct CommUDPPacketMeta1T
{
    uint32_t uSourceClientId;   //!< source client identifier
    uint32_t uTargetClientId;   //!< target client identifier
} CommUDPPacketMeta1T;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// decode a uint32_t from uint8_t[4]
uint32_t VoipServerDecodeU32(const uint8_t *pValue);

// pull latency information from packet (if available).
int32_t VoipServerExtractNetGameLinkLate(const CommUDPPacketHeadT *pPacket, uint32_t uPacketLen, uint32_t *pLatency);

// update netgamelink sync.echo with voipserver timestamp (if present in the packet).
void VoipServerTouchNetGameLinkEcho(CommUDPPacketHeadT *pPacket, uint32_t uPacketLen);

// pull meta1 from packet
int32_t VoipServerExtractMeta1Data(const CommUDPPacketHeadT *pPacket, int32_t iPacketSize, CommUDPPacketMeta1T *pMeta1Data);

// validate packet
CommUDPPacketHeadT *VoipServerValidateCommUDPPacket(const char *pPacketData, int32_t iPacketSize);

// opens a socket
SocketT *VoipServerSocketOpen(int32_t iType, uint32_t uLocalBindAddr, uint16_t uPort, uint32_t bReuseAddr, SocketRecvCb *pRecvCb, void *pUserData);

#ifdef __cplusplus
}
#endif

#endif // _voipservercomm_h
