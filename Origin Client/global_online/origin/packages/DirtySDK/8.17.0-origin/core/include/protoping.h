/*H*************************************************************************************************/
/*!

    \File    protoping.h

    \Description
        This module implements a Dirtysock ping routine. It can both generate
        ping requests and record their responses. Because ICMP access (ping)
        is o/s specific, different code is used for each environment. This code
        was derived from the Winsock version.

    \Notes
        None.

    \Copyright
        Copyright (c) Tiburon Entertainment / Electronic Arts 2002.  ALL RIGHTS RESERVED.

    \Version    1.0        04/03/2002 (GWS) First Version
    \Version    1.1        11/18/2002 (JLB) Names changed to include "Proto"

*/
/*************************************************************************************************H*/

#ifndef _protoping_h
#define _protoping_h

/*!
\Module Proto
*/
//@{

/*** Include files *********************************************************************/

#include "dirtyaddr.h"

/*** Defines ***************************************************************************/

/*
    Summary of Message Types (RFC792)
    0  Echo Reply
    3  Destination Unreachable
    4  Source Quench
    5  Redirect
    8  Echo
    11  Time Exceeded
    12  Parameter Problem
    13  Timestamp
    14  Timestamp Reply
    15  Information Request
    16  Information Reply
*/

// ICMP packet types we handle
#define ICMP_ECHOREPLY          (0)     //! ICMP echo reply
#define ICMP_DESTUNREACH        (3)     //! ICMP destination unreachable
#define ICMP_ECHOREQ            (8)     //! ICMP echo request
#define ICMP_TIMEEXCEEDED       (11)    //! ICMP time exceeded

#define PROTOPING_MAXUSERDATA   (256)   //!< maximum user data size

/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

typedef enum PingType
{
    PROTOPING_TYPE_ICMP = 0,
    PROTOPING_TYPE_QOS,

    PROTOPING_NUMTYPES
} ePingType;

// opaque module state reference
typedef struct ProtoPingRefT ProtoPingRefT;

//! ping response structure
typedef struct ProtoPingResponseT
{
    DirtyAddrT  Addr;           //!< address of host that replied (opaque form)
    uint32_t    uAddr;          //!< address of host that replied (old form)
    int16_t     iPing;          //!< ping response time, or -1 for timeout
    uint16_t    uSeqn;          //!< sequence number
    uint16_t    uNumRetries;    //!< number of retries before success (will be non-zero only on Xbox)
    uint8_t     uType;          //!< ICMP type of response
    uint8_t     bServer;        //!< TRUE if this was a server ping, else FALSE
} ProtoPingResponseT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create new module
ProtoPingRefT *ProtoPingCreate(int32_t iMaxPings);

// destroy the module
void ProtoPingDestroy(ProtoPingRefT *pProtoPing);

// change a ping state value
int32_t ProtoPingControl(ProtoPingRefT *pProtoPing, int32_t iControl, int32_t iValue, void *pValue);

// send a ping request
int32_t ProtoPingRequest(ProtoPingRefT *pProtoPing, DirtyAddrT *pAddr, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl);

// ping server
int32_t ProtoPingRequestServer(ProtoPingRefT *pProtoPing, uint32_t uServerAddress, const unsigned char *pData, int32_t iDataLen, int32_t iTimeout, int32_t iTtl);

// get latest ping response
int32_t ProtoPingResponse(ProtoPingRefT *pProtoPing, unsigned char *pBuffer, int32_t *pBufLen, ProtoPingResponseT *pResponse);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protoping_h

