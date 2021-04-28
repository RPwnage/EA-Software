/*H*******************************************************************/
/*!
    \File protoudp.h

    \Description
        This module enables sending/receiving UDP packets.

    \Notes
        This was initially created for use by the TickerApi.

    \Copyright
        Copyright (c) Electronic Arts 2003. ALL RIGHTS RESERVED.

    \Version 1.0 12/11/2003 (sbevan) First Version
*/
/*******************************************************************H*/

#ifndef _protoudp_h
#define _protoudp_h

/*!
\Module Proto
*/
//@{

/*** Include files ***************************************************/

/*** Defines *********************************************************/

/*** Macros **********************************************************/

//! wrapper for ProtoUdpRecvFrom
#define ProtoUdpRecv(_pUdp, _pData, _uSize)     ProtoUdpRecvFrom(_pUdp, _pData, _uSize, NULL);

/*** Type Definitions ************************************************/

typedef struct ProtoUdpT ProtoUdpT;

/*** Variables *******************************************************/

/*** Functions *******************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Create a udp connection.
ProtoUdpT *ProtoUdpCreate(int32_t iMaxPacketSize, int32_t iMaxPackets);

// Bind the local port (and implicitly address) of the socket.
int32_t ProtoUdpBind(ProtoUdpT *pUdp, int32_t iLocalPort);

// Bind the remote address of the socket.
int32_t ProtoUdpConnect(ProtoUdpT *pUdp, struct sockaddr *pRemoteAddress);

// Destroy the instance.  
void ProtoUdpDestroy(ProtoUdpT *pUdp);

// Undo the effect of any previous ProtoUdpConnect.
void ProtoUdpDisconnect(ProtoUdpT* pUdp);

// Undo the effect of any previous ProtoUdpConnect.
void ProtoUdpGetLocalAddr(ProtoUdpT *pUdp, struct sockaddr_in *pLocalAddr);

// Receive a packet
int32_t ProtoUdpRecvFrom(ProtoUdpT *pUdp, char *pData, uint32_t uSize, struct sockaddr *pRemoteAddr);

// Send a packet to the previously bound address.
int32_t ProtoUdpSend(ProtoUdpT *pUdp, const char *pData, int32_t iSize);

// Send a packet to the given address.
int32_t ProtoUdpSendTo(ProtoUdpT *pUdp, const char *pData, int32_t iSize, struct sockaddr *pRemoteAddr);

// Perform any needed background processing.
void ProtoUdpUpdate(ProtoUdpT *pUdp);

#ifdef __cplusplus
}
#endif

//@}

#endif
