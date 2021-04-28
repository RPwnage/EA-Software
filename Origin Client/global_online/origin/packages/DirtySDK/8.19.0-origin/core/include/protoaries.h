/*H********************************************************************************/
/*!
    \File protoaries.h

    \Description
        This module supports the Aries transfer protocol (a simple length/type
        prefixed TCP message type).  It is a cross platform component and is used
        on all platforms save for the PS2-Inet platform, which has a custom
        implementation of the same protocol.  ProtoAries is used for communication
        with the Lobby and Buddy servers.

    \Copyright
        Copyright (c) Electronic Arts 2000-2005.  ALL RIGHTS RESERVED.

    \Version 02/28/1999 (gschaefer) First Version
    \Version 03/18/2002 (gschaefer) Renamed from netsess and cleanup
    \Version 07/10/2003 (gschaefer) Added listen support
    \Version 02/02/2004 (sbevan)    Added SSL support
*/
/********************************************************************************H*/

#ifndef _protoaries_h
#define _protoaries_h

/*!
\Module Proto
*/
//@{

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

#define ARIES_KIND_STAT ((signed)0xffffffff)
#define ARIES_CODE_CONN ((signed)0xffffffff)
#define ARIES_CODE_DISC ((signed)0xfefefefe)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

enum
{
    PROTOARIES_SECRET_SIZE = 32,
    PROTOARIES_TICKET_SIZE = PROTOARIES_SECRET_SIZE+sizeof(int32_t)
};

//! aries module state
typedef enum ProtoAriesStateE
{
    PROTOARIES_STATE_IDLE,
    PROTOARIES_STATE_LIST,
    PROTOARIES_STATE_CONN,
    PROTOARIES_STATE_ONLN,
    PROTOARIES_STATE_DISC
} ProtoAriesStateE;

//! aries security types
typedef enum ProtoAriesSecureE
{
    PROTOARIES_SECURE_NONE,     //!< no security
    PROTOARIES_SECURE_SSL,      //!< encryption via SSL
    PROTOARIES_SECURE_TICKET    //!< encryption using key server ticket
} ProtoAriesSecureE;

//! aries message header
typedef struct ProtoAriesMsgHdrT
{
    uint8_t kind[4];      //!< packet kind
    uint8_t code[4];      //!< packet status code
    uint8_t size[4];      //!< packet size
} ProtoAriesMsgHdrT;

//! module state reference
typedef struct ProtoAriesRefT ProtoAriesRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create new virtual network session
ProtoAriesRefT *ProtoAriesCreate(int32_t iMaxBuf);

// destroy existing session
void ProtoAriesDestroy(ProtoAriesRefT *pProtoAries);

// return session status info
int32_t ProtoAriesStatus(ProtoAriesRefT *pProtoAries, int32_t iSelect, void *pBuf, int32_t iBufLen);

// initiate connection to central server (TCP based)
int32_t ProtoAriesConnect(ProtoAriesRefT *pProtoAries, const char *pServer, uint32_t uAddr, int32_t iPort);

// terminate virtual session with server
int32_t ProtoAriesUnconnect(ProtoAriesRefT *pProtoAries);

// initiate connection to central server (TCP based)
int32_t ProtoAriesListen(ProtoAriesRefT *pProtoAries, uint32_t uAddr, int32_t iPort);

// terminate listen request
int32_t ProtoAriesUnlisten(ProtoAriesRefT *pProtoAries);

// give module time to perform periodic processing
void ProtoAriesUpdate(ProtoAriesRefT *pProtoAries);

// send a packet (request) to the server
int32_t ProtoAriesSend(ProtoAriesRefT *pProtoAries, int32_t iKind, int32_t iCode, const char *pBody, int32_t iSize);

// see if an incoming packet is waiting (but do not remove from queue)
int32_t ProtoAriesPeek(ProtoAriesRefT *pProtoAries, int32_t *pKind, int32_t *pCode, char **ppBody);

// get a packet from the server (will copy packet to private buffer)
int32_t ProtoAriesRecv(ProtoAriesRefT *pProtoAries, int32_t *pKind, int32_t *pCode, char *pBody, int32_t iSize);

// Define whether the Aries connection should be secure or insecure.
void ProtoAriesSecure(ProtoAriesRefT *pProtoAries, ProtoAriesSecureE eSecure);

// Set up the session key.
void ProtoAriesSetKey(ProtoAriesRefT *pProtoAries, const void *pKey, uint32_t uKeySize);

// return timebase for easy client calculations
uint32_t ProtoAriesTick(ProtoAriesRefT *pProtoAries);

#ifdef __cplusplus
}
#endif

//@}

#endif // _protoaries_h

