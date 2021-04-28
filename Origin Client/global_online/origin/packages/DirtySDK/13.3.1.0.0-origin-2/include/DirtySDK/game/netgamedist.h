/*H*************************************************************************************/
/*!
    \File netgamedist.h

    \Description
        The GameDist class provides the main control interface between the
        individual game controllers and the distributed game-input engine.
        The GameInput and GameComm classes provided the interfaces used by
        this class for real-world communication.

    \Copyright
        Copyright (c) Electronic Arts 1998-2007.

    \Version 1.0 12/15/1998 (gschaefer) First Version
    \Version 1.1 12/26/1999 (gschaefer) Revised to add external stats interface
    \Version 1.2 11/18/2002 (jbrookes) Moved to NetGame hierarchy
*/
/*************************************************************************************H*/

#ifndef _netgamedist_h
#define _netgamedist_h

/*!
    \Moduledef NetGame NetGame

    \Description
        The NetGame module provides game-level peer networking.

        <b>Overview</b>

        <b>Module Dependency Graph</b>
    
            <img alt="" src="netgame.png">

*/
//@{

/*** Include files *********************************************************************/

#include "DirtySDK/game/netgameerr.h"
#include "DirtySDK/game/netgamepkt.h"

/*** Defines ***************************************************************************/

#define GMDIST_DEFAULT_BUFFERSIZE_IN        (16384) //<! default queue size for outbound packets
#define GMDIST_DEFAULT_BUFFERSIZE_OUT       (8192)  //<! default queue size for inbound packets

#define GMDIST_MAX_CLIENTS                  (32)    //<! maximum number of clients
#define GMDIST_ERROR_SIZE                   (2048)  //<! max string size for NetGameDistGetErrorText

#define GMDIST_DATA_NONE                    (1)     //<! packets type:  Nothing 
#define GMDIST_DATA_INPUT                   (2)     //<!    Input from some player
#define GMDIST_DATA_INPUT_DROPPABLE         (3)     //<!    Droppable input from some player
#define GMDIST_DATA_DISCONNECT              (4)     //<!    Disconnected player
#define GMDIST_DATA_NODATA                  (5)     //<!    Sparse multipacket with blank for this player 

// This value end up being packed in a nibble. The fourth bit is reserved:
#define GMDIST_DATA_CRC_REQUEST             (0x08)  //<!    CRC check challenge from the GS


/*** Macros ****************************************************************************/

/*** Type Definitions ******************************************************************/

// opaque module ref
typedef struct NetGameDistRefT NetGameDistRefT;

//! dist stat proc
typedef int32_t (NetGameDistStatProc)(void *pLinkRef, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

//! dist send proc
typedef int32_t (NetGameDistSendProc)(void *pLinkRef, NetGamePacketT *pPkt, int32_t iLen);

//! dist recv proc
typedef int32_t (NetGameDistRecvProc)(void *pLinkRef, NetGamePacketT *pPkt, int32_t iLen, uint8_t bDist);

//! dist peek proc
typedef int32_t (NetGameDistPeekProc)(void *pLinkRef, NetGamePacketT **pPkt, uint32_t uMask);

//! dist drop proc
typedef int8_t (NetGameDistDropProc)(void *pLinkRef, void *pExisting, void *pIncoming, uint8_t uTypeExisting, uint8_t uTypeIncoming);

typedef int32_t (NetGameDistLinkCtrlProc)(void *ref, int32_t iKind, int32_t iData);

typedef struct NetGameDistStatT
{
    uint32_t ping;          //!< peer to peer ping
    uint32_t late;          //!< overall latency of game (show this to user)
    uint32_t bps;           //!< bytes per second (user data)
    uint32_t pps;           //!< packets per second (user packets)
} NetGameDistStatT;

/*** Variables *************************************************************************/

/*** Functions *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// construct the game client
NetGameDistRefT *NetGameDistCreate(void *pLinkRef, NetGameDistStatProc *pStatProc, NetGameDistSendProc *pSendProc, NetGameDistRecvProc *pRecvProc, uint32_t uInBufferSize, uint32_t uOutBufferSize );

// destruct the game client
void NetGameDistDestroy(NetGameDistRefT *pRef);

// status function
int32_t NetGameDistStatus(NetGameDistRefT *pRef, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// for game-server usage, make NetGameDist send multi-packet 
void NetGameDistSetServer(NetGameDistRefT *pRef, uint8_t uActAsServer);

// If using the "multi" versions, sets the index of the local user and the total number of players
void NetGameDistMultiSetup(NetGameDistRefT *pRef, int32_t iOurIndex, int32_t iTotPlrs );

// set whether to enable meta-information sending, the mask and the version number
void NetGameDistMetaSetup(NetGameDistRefT *pRef, uint8_t uSparse, uint32_t uMask, uint32_t uVersion);

// peek input from peer
int32_t NetGameDistInputPeek(NetGameDistRefT *pRef, uint8_t *pType, void *pPeer, int32_t *pPlen);

// return completed input from client
int32_t NetGameDistInputQuery(NetGameDistRefT *pRef, void *pOurs, int32_t *pOlen, void *pPeer, int32_t *pPlen);

// return completed input from multiple clients
int32_t NetGameDistInputQueryMulti(NetGameDistRefT *pRef, uint8_t *pDataTypes, void **pInputs, int32_t *pLen);

// provide local input data
int32_t NetGameDistInputLocal(NetGameDistRefT *pRef, void *pBuffer, int32_t iLen);

// provide local input data (n-consoles). Client usage is to provide one-long arrays. Type can be GMDIST_DATA_INPUT, GMDIST_DATA_INPUT_DROPPABLE
int32_t NetGameDistInputLocalMulti(NetGameDistRefT *pRef, uint8_t* uTypes, void **pBuffer, int32_t *pLen, int32_t iDelta);

// check input status (see how long till next)
void NetGameDistInputCheck(NetGameDistRefT *pRef, int32_t *pSend, int32_t *pRecv);

// set new input exchange rate
void NetGameDistInputRate(NetGameDistRefT *pRef, int32_t iRate);

// flush the input queue (note that this must be done with
void NetGameDistInputClear(NetGameDistRefT *pRef);

// set NetGameDist operation parameters
int32_t NetGameDistControl(NetGameDistRefT *pRef, int32_t iSelect, int32_t iValue, void *pValue);

// dispatch to appropriate updaters
uint32_t NetGameDistUpdate(NetGameDistRefT *pRef);

// override the send,recv,stats procs or set the drop proc
void NetGameDistSetProc(NetGameDistRefT *pRef, int32_t iKind, void *pProc);

// For the game server to provides stats, the array must be sized according to what was passed to NetGameDistMultiSetup
void NetGameDistSendStats(NetGameDistRefT *pRef, NetGameDistStatT *pStats);

// return non-zero if there ever was an overflow
int32_t NetGameDistGetError(NetGameDistRefT *pRef);

// copies into the passed buffer the last error text.
void NetGameDistGetErrorText(NetGameDistRefT *pRef, char *pErrorBuffer, int32_t iBufSize);

// reset the error count
void NetGameDistResetError(NetGameDistRefT *pRef);

#ifdef __cplusplus
}
#endif

//@}

#endif // _netgamedist_h







