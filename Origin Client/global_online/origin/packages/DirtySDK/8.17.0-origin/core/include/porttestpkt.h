/*H********************************************************************************/
/*!
    \File porttestpkt.h

    \Description
        This module provides definitions of the port test packet structures and
        functions to marshal these packets into and out of buffers for
        transmission/reception.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/04/2005 (doneill) First Version
*/
/********************************************************************************H*/

#ifndef _porttestpkt_h
#define _porttestpkt_h

/*** Macros ***********************************************************************/

#define DirtyPortRequestSize() (sizeof(DirtyPortRequestPktT))
#define DirtyPortResponseSize() (sizeof(DirtyPortResponsePktT))

/*** Type Definitions *************************************************************/

// Port test request packet (client->server)
typedef struct DirtyPortRequestPktT
{
    int32_t iCount;         // Number of response packets requested
    int32_t iPort;          // Port for responses to be sent back to
    int32_t iSeqn;          // Sequence number of the request
    char strTitle[32];  // Name of the game issuing the request
    char strUser[32];   // Name of the user issuing the request
} DirtyPortRequestPktT;

// Port test response packet (server->client)
typedef struct DirtyPortResponsePktT
{
    int32_t iIndex;         // Packet number from 0-iCount
    int32_t iCount;         // Number of responses sent
    int32_t iSeqn;          // Sequence number matching request packet
    char strTitle[32];  // Name of the game issuing the request
    char strUser[32];   // Name of the user issuing the request
} DirtyPortResponsePktT;

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// Encode a request packet into the provided buffer
int32_t DirtyPortRequestEncode(DirtyPortRequestPktT *pPkt, uint8_t *pData, int32_t iLen);

// Decode a request packet from the given buffer
int32_t DirtyPortRequestDecode(DirtyPortRequestPktT *pPkt, uint8_t *pData, int32_t iLen);

// Encode a response packet into the provided buffer
int32_t DirtyPortResponseEncode(DirtyPortResponsePktT *pPkt, uint8_t *pData, int32_t iLen);

// Decode a response packet from the given buffer
int32_t DirtyPortResponseDecode(DirtyPortResponsePktT *pPkt, uint8_t *pData, int32_t iLen);

#ifdef __cplusplus
};
#endif

#endif // _porttestpkt_h

