/*H********************************************************************************/
/*!
    \File gameserverlink.h

    \Description
        Provides optional link connections for gameservers running in <blah> mode.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 02/01/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _gameserverlink_h
#define _gameserverlink_h

/*** Include files ****************************************************************/

#include "DirtySDK/game/netgamepkt.h"
#include "DirtySDK/game/netgameutil.h"
#include "DirtySDK/game/netgamelink.h"
#include "DirtySDK/game/netgamedist.h"
#include "DirtySDK/proto/prototunnel.h"
#include "gameserverpkt.h"

/*** Defines **********************************************************************/

// gameserverlink multi-delete actions (use for GameServerLinkDelClient() iClientId param)
#define GSLINK_DELALLCLIENTS    (-1)    //!< delete all clients from client list
#define GSLINK_DELDSCCLIENTS    (-2)    //!< delete disconnected clients from client list

#define GSLINK_DEBUGBUFFERLENGTH    (2048)
/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! connection status
typedef enum GameServLinkStatusE
{
    GSLINK_STATUS_FREE,         //!< unallocated

    GSLINK_STATUS_CONN,         //!< connecting to link
    GSLINK_STATUS_ACTV,         //!< connection established
    GSLINK_STATUS_CLSE,         //!< connection closing
    GSLINK_STATUS_DISC,         //!< disconnected (link destroyed)

    GSLINK_NUMSTATUSTYPES       //!< max number of status types
} GameServLinkStatusE;

//! client events
typedef enum GameServLinkClientEventE
{
    GSLINK_CLIENTEVENT_ADD,
    GSLINK_CLIENTEVENT_CONN,
    GSLINK_CLIENTEVENT_DSC,
    GSLINK_CLIENTEVENT_DEL
} GameServLinkClientEventE;

//! opaque module state
typedef struct GameServerLinkT GameServerLinkT;

//! client info
typedef struct GameServerLinkClientT
{
    int32_t iClientId;                  //!< id of the client, used for connection establishment and identification
    int32_t iGameServerId;              //!< our (gameserver) local id used to establish connections
    uint32_t uAddr;
    uint16_t uVirt;
    uint16_t uLate;
    uint16_t uDisconnectReason;
    uint32_t uConnStart;                //!< connection attempt start time before connection and establishment time after connection
    uint32_t uConnTime;                 //!< amount of time connection establishment took to complete
    uint32_t uRLmt;                     //!< used with GameServerLinkLogClientLatencies()
    uint32_t uRNakSent;                 //!< used with GameServerLinkLogClientLatencies()
    uint32_t uRPackSent;                //!< used with GameServerLinkLogClientLatencies()
    uint32_t uRPackRcvd;                //!< used with GameServerLinkLogClientLatencies()
    uint32_t uRPackRcvd2;               //!< used with _GameServerLinkL2RPktLossDetection()
    uint32_t uLPackRcvd;                //!< used with GameServerLinkLogClientLatencies()
    uint32_t uRPackLost;                //!< used with GameServerLinkLogClientLatencies()
    uint32_t uRPackLost2;               //!< used with _GameServerLinkL2RPktLossDetection()
    uint32_t uLPackLost;                //!< used with GameServerLinkLogClientLatencies()
    int32_t iStallTime;
    int32_t iLastPktLoss;
    GameServerPacketLostSentT PacketLostSentStat;
    GameServLinkStatusE eStatus;
    NetGameUtilRefT *pGameUtil;
    NetGameLinkRefT *pGameLink;
    NetGameLinkStreamT *pGameLinkStream;
    SocketT *pSocket;
    uint32_t uRlmtChangeCount;          //!< count of redundancy limit changes
    uint8_t bRlmtEnabled;               //!< TRUE if redundancy limit is not currently set to default, FALSE otherwise
    uint8_t bRlmtChanged;               //!< TRUE if redundancy limit enabled variable has changed since last update
    uint8_t _pad[2];
    char strTunnelKey[PROTOTUNNEL_MAXKEYLEN];   //!< tunnel key associated with this client
} GameServerLinkClientT;

//! client list
typedef struct GameServerLinkClientListT
{
    int32_t iNumClients;                //!< current number of clients
    int32_t iMaxClients;                //!< max number of clients
    GameServerLinkClientT Clients[1];   //!< variable-length array of clients
} GameServerLinkClientListT;

//! send function prototype
typedef void (GameServerLinkSendT)(GameServerPacketT *pPacket, void *pUserData);

//! disconnection function prototype (used to notify gameserver.cpp of a client peer disconnect)
typedef void (GameServerLinkDiscT)(GameServerLinkClientT *pClient, void *pUserData);

//! client update function prototype
typedef int32_t (GameServerLinkUpdateClientT)(GameServerLinkClientListT *pClientList, int32_t iClient, void *pUpdateData, char* pLogBuffer);

//! clientlist update function prototype
typedef void    (GameServerLinkClientEventT)(GameServerLinkClientListT *pClientList, GameServLinkClientEventE eEvent, int32_t iClient, void *pUpdateData);

//! link create parameters
typedef struct GameServerLinkCreateParamT
{
    uint32_t uPort;
    uint32_t uAddr;
    char aCommandTags[4096];
    char aConfigsTags[4096];
    GameServerLinkSendT *pSendFunc;
    GameServerLinkDiscT *pDisFunc;
    GameServerLinkClientEventT *pEventFunc;
    void *pUserData;
} GameServerLinkCreateParamT;


/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create the module
GameServerLinkT *GameServerLinkCreate(const char *pCommandTags, const char *pConfigTags, int32_t iMaxClients, int32_t iVerbosity, uint32_t uRedundancyThreshold,
                                      uint32_t uRedundancyLimit, int32_t iRedundancyTimeout, GameServerLinkSendT *pSendFunc, GameServerLinkDiscT *pDiscFunc, void *pUserData);

// set update callbacks
void GameServerLinkCallback(GameServerLinkT *pLink, GameServerLinkClientEventT *pEventFunc, GameServerLinkUpdateClientT *pUpdateFunc, void *pUpdateData);

// destroy the module
void GameServerLinkDestroy(GameServerLinkT *pLink);

// add a client
int32_t GameServerLinkAddClient(GameServerLinkT *pLink, int32_t iClientIndex, int32_t iClientId, int32_t iGameServerId, uint32_t uClientAddr, const char *pTunnelKey);

// delete a client
int32_t GameServerLinkDelClient(GameServerLinkT *pLink, int32_t iClientId);

// receive a packet
int32_t GameServerLinkRecv(GameServerLinkT *pLink, GameServerPacketT *pPacket);

// update the module (called when new data is available from voipserver)
GameServerStatInfoT *GameServerLinkUpdate(GameServerLinkT *pLink);

// update the module (called at a fixed rate)
void GameServerLinkUpdateFixed(GameServerLinkT *pLink);

// handle start game event
void GameServerLinkStartGame(GameServerLinkT *pLink);

// handle stop game event
void GameServerLinkStopGame(GameServerLinkT *pLink);

// verbose logging of client latency info
void GameServerLinkLogClientLatencies(GameServerLinkT *pLink);

// check if client link redundancy needs to be modified
void GameServerLinkUpdateRedundancy(GameServerLinkT *pLink);

// format gameserverlink stats for diagnostic request
int32_t GameServerLinkDiagnosticStatus(GameServerLinkT *pLink, char *pBuffer, int32_t iBufSize, time_t uCurTime);

// status function
int32_t GameServerLinkStatus(GameServerLinkT *pLink, int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize);

// get the game server link client list
GameServerLinkClientListT* GameServerLinkClientList(GameServerLinkT* pLink);

#ifdef __cplusplus
}
#endif

#endif // _gameserverlink_h

