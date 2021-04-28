/*H********************************************************************************/
/*!
    \File porttestpkt.c

    \Description
        This module provides definitions of the port test packet structures and
        functions to marshal these packets into and out of buffers for
        transmission/reception.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 03/04/2005 (doneill) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "platform.h"

// if building on the server
#ifdef EAFN_SERVER
 #ifdef WIN32
  #include <windows.h>
 #else
  #include <netinet/in.h>
 #endif
 #define SocketHtonl(_val) htonl(_val) 
 #define SocketNtohl(_val) ntohl(_val)
#else  // building on the client
 #include "dirtylib.h"
 #include "dirtynet.h"
#endif

#include "porttestpkt.h"

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
     \Function DirtyPortRequestEncode
 
     \Description
          Encode the given request packet into the provided buffer for transmission
          to the server.
  
     \Input pPkt    - Packet to encode
     \Input pData   - Buffer to encode into
     \Input iLen    - Length of pData
 
     \Output
        int32_t     - 0 on error; encoded packet length on success
 
     \Version 03/04/2005 (doneill)
*/
/********************************************************************************F*/
int32_t DirtyPortRequestEncode(DirtyPortRequestPktT *pPkt, uint8_t *pData, int32_t iLen)
{
    int32_t iIdx;
    uint8_t *pEnc;

    if (iLen < (int32_t)sizeof(*pPkt))
    {
        return(0);
    }
    pEnc = pData;

    // ensure int32_t values are in network byte order
    *((int32_t *)pEnc) = SocketHtonl(pPkt->iCount);
    pEnc += sizeof(int32_t);

    *((int32_t *)pEnc) = SocketHtonl(pPkt->iPort);
    pEnc += sizeof(int32_t);

    *((int32_t *)pEnc) = SocketHtonl(pPkt->iSeqn);
    pEnc += sizeof(int32_t);

    // encode strings; pad with null characters
    for(iIdx = 0; (iIdx < (int32_t)sizeof(pPkt->strTitle)) && (pPkt->strTitle[iIdx] != '\0'); iIdx++)
    {
        *pEnc++ = pPkt->strTitle[iIdx];
    }
    for(; iIdx < (int32_t)sizeof(pPkt->strTitle); iIdx++)
    {
        *pEnc++ = '\0';
    }

    for(iIdx = 0; (iIdx < (int32_t)sizeof(pPkt->strUser)) && (pPkt->strUser[iIdx] != '\0'); iIdx++)
    {
        *pEnc++ = pPkt->strUser[iIdx];
    }
    for(; iIdx < (int32_t)sizeof(pPkt->strUser); iIdx++)
    {
        *pEnc++ = '\0';
    }
    
    return(pEnc - pData);
}

/*F********************************************************************************/
/*!
     \Function DirtyPortRequestDecode
 
     \Description
          Decode the given buffer into the provided request packet structure.
  
     \Input pPkt    - Packet to decode into
     \Input pData   - Buffer to decode from
     \Input iLen    - Length of pData
 
     \Output
        int32_t     - 0 on error; decoded packet length on success
 
     \Version 03/04/2005 (doneill)
*/
/********************************************************************************F*/
int32_t DirtyPortRequestDecode(DirtyPortRequestPktT *pPkt, unsigned char *pData, int32_t iLen)
{
    int32_t iIdx;
    uint8_t *pDec;

    if (iLen < (int32_t)sizeof(*pPkt))
    {
        return(0);
    }
    pDec = pData;

    // ensure int32_t values are in host byte order
    pPkt->iCount = SocketNtohl(*(int32_t *)pDec);
    pDec += sizeof(int32_t);

    pPkt->iPort = SocketNtohl(*(int32_t *)pDec);
    pDec += sizeof(int32_t);

    pPkt->iSeqn = SocketNtohl(*(int32_t *)pDec);
    pDec += sizeof(int32_t);

    // decode string parameters
    for (iIdx = 0; iIdx < (int32_t)sizeof(pPkt->strTitle); iIdx++)
    {
        pPkt->strTitle[iIdx] = *pDec++;
    }

    for (iIdx = 0; iIdx < (int32_t)sizeof(pPkt->strUser); iIdx++)
    {
        pPkt->strUser[iIdx] = *pDec++;
    }

    return(pDec - pData);
}

/*F********************************************************************************/
/*!
     \Function DirtyPortResponseEncode
 
     \Description
          Encode the given response packet into the provided buffer for transmission
          to the client.
  
     \Input pPkt    - Packet to encode
     \Input pData   - Buffer to encode into
     \Input iLen    - Length of pData
 
     \Output
        int32_t     - 0 on error; encoded packet length on success
 
     \Version 03/04/2005 (doneill)
*/
/********************************************************************************F*/
int32_t DirtyPortResponseEncode(DirtyPortResponsePktT *pPkt, uint8_t *pData, int32_t iLen)
{
    int32_t iIdx;
    uint8_t *pEnc;

    if (iLen < (int32_t)sizeof(*pPkt))
    {
        return(0);
    }
    pEnc = pData;

    // ensure int32_t values are in network byte order
    *((int32_t *)pEnc) = SocketHtonl(pPkt->iIndex);
    pEnc += sizeof(int32_t);

    *((int32_t *)pEnc) = SocketHtonl(pPkt->iCount);
    pEnc += sizeof(int32_t);

    *((int32_t *)pEnc) = SocketHtonl(pPkt->iSeqn);
    pEnc += sizeof(int32_t);

    // encode strings; pad with null characters
    for (iIdx = 0; (iIdx < (int32_t)sizeof(pPkt->strTitle)) && (pPkt->strTitle[iIdx] != '\0'); iIdx++)
    {
        *pEnc++ = pPkt->strTitle[iIdx];
    }
    for (; iIdx < (int32_t)sizeof(pPkt->strTitle); iIdx++)
    {
        *pEnc++ = '\0';
    }

    for (iIdx = 0; (iIdx < (int32_t)sizeof(pPkt->strUser)) && (pPkt->strUser[iIdx] != '\0'); iIdx++)
    {
        *pEnc++ = pPkt->strUser[iIdx];
    }
    for (; iIdx < (int32_t)sizeof(pPkt->strUser); iIdx++)
    {
        *pEnc++ = '\0';
    }

    return(pEnc - pData);
}

/*F********************************************************************************/
/*!
     \Function DirtyPortResponseDecode
 
     \Description
          Decode the given buffer into the provided response packet structure.
  
     \Input pPkt    - Packet to decode into
     \Input pData   - Buffer to decode from
     \Input iLen    - Length of pData
 
     \Output
        int32_t     - 0 on error; decoded packet length on success
 
     \Version 03/04/2005 (doneill)
*/
/********************************************************************************F*/
int32_t DirtyPortResponseDecode(DirtyPortResponsePktT *pPkt, uint8_t *pData, int32_t iLen)
{
    int32_t iIdx;
    uint8_t *pDec;

    if (iLen < (int32_t)sizeof(*pPkt))
    {
        return(0);
    }
    pDec = pData;

    // ensure int32_t values are in host byte order
    pPkt->iIndex = SocketNtohl(*(int32_t *)pDec);
    pDec += sizeof(int32_t);

    pPkt->iCount = SocketNtohl(*(int32_t *)pDec);
    pDec += sizeof(int32_t);

    pPkt->iSeqn = SocketNtohl(*(int32_t *)pDec);
    pDec += sizeof(int32_t);

    // decode string parameters
    for (iIdx = 0; iIdx < (int32_t)sizeof(pPkt->strTitle); iIdx++)
    {
        pPkt->strTitle[iIdx] = *pDec++;
    }

    for (iIdx = 0; iIdx < (int32_t)sizeof(pPkt->strUser); iIdx++)
    {
        pPkt->strUser[iIdx] = *pDec++;
    }

    return(pDec - pData);
}


