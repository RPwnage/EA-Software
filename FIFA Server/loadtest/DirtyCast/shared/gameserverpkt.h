/*H********************************************************************************/
/*!
    \File gameserverpkt.h

    \Description
        Defines the VoipServer<->GameServer packet interface

    \Notes
        GameServer->VoipServer packet types
            'ngam'  - new game
            'dgam'  - delete game
            'nusr'  - new user
            'dusr'  - delete user
            'clst'  - set client send list

        VoipServer<->GameServer packet types
            'data'
            'ping'
            'tokn'

    \Copyright
        Copyright (c) 2006 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _gameserverpkt_h
#define _gameserverpkt_h

/*** Include files ****************************************************************/

#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/proto/prototunnel.h"
#include "DirtySDK/voip/voiptunnel.h"
#include "bufferedsocket.h"

/*** Defines **********************************************************************/

//! maximum len of product id string
#define GAMESERVER_MAX_PRODUCTID_LENGTH            (48)

//! maximum len of hostname
#define GAMESERVER_MAX_HOSTNAME_LENGTH             (256)

//! maximum len of deploy info 
#define GAMESERVER_MAX_DEPLOYINFO_LENGTH           (128)
#define GAMESERVER_MAX_DEPLOYINFO2_LENGTH          (128)

//! gameserver gamestate flags
enum
{
    GAMESERVER_GAMESTATE_PREGAME = 0,
    GAMESERVER_GAMESTATE_INGAME,
    GAMESERVER_GAMESTATE_POSTGAME,
    GAMESERVER_GAMESTATE_MIGRATING,
    GAMESERVER_GAMESTATE_OTHER,
    GAMESERVER_NUMGAMESTATES
};

//! state shared between vs and gs in ping packets
typedef enum LobbyStateE
{
    kOffline = 0,
    kAvailable,
    kActive,
    kZombie
} LobbyStateE;

//! game packet history (for debugging, 0 to disable)
#define GAMESERVER_PACKETHISTORY                    (0)

//! max number of latency bin ranges (includes terminator)
#define GAMESERVER_MAXLATEBINS                      (8)

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

//! game header structure that prefixes voiptunnel<->gameserver packet data
typedef struct GameServerPktHdrT
{
    uint32_t uKind;
    uint32_t uCode;
    uint32_t uSize;
} GameServerPktHdrT;

//! game stats structure for shared statistics data
typedef struct GameServerProtoStatT
{
    uint16_t uPktsRecv;     //!< number of packets received since last update
    uint16_t uPktsSent;     //!< number of packets sent since last update
    uint32_t uByteRecv;     //!< number of bytes received since last update
    uint32_t uByteSent;     //!< number of bytes sent since last update
} GameServerProtoStatT;

//! game latency stats
typedef struct GameServerLateT
{
    uint16_t uMinLate;
    uint16_t uMaxLate;
    uint32_t uSumLate;
    uint32_t uNumStat;
} GameServerLateT;

//! packet loss/sent stats
typedef struct GameServerPacketLostSentT
{
    uint32_t uLpackSent;
    uint32_t uLpackLost;
    uint32_t uRpackSent;
    uint32_t uRpackLost;
}GameServerPacketLostSentT;

//! game latency bucket stats (count of users falling into fixed latency ranges, or 'buckets')
typedef struct GameServerLateBinT
{
    uint32_t aBins[GAMESERVER_MAXLATEBINS];
} GameServerLateBinT;

//! system latency stats
typedef struct GameServerStatInfoT
{
    GameServerLateT E2eLate;                     //!< client end-to-end latency
    GameServerLateT InbLate;                     //!< inbound packet latency
    GameServerLateT OtbLate;                     //!< outbound packet latency
    GameServerLateBinT LateBin;                  //!< latency bin info
    GameServerPacketLostSentT PackLostSent;      //!< track total / delta packet loss and sent
    uint32_t uRlmtChangeCount;                   //!< number of times rlmt selector was used since last update
} GameServerStatInfoT;

//! game stats structure
typedef struct GameServerStatsT
{
    GameServerProtoStatT GameStat;
    GameServerProtoStatT VoipStat;
    GameServerLateT      OtbLate;   //!< outbound latency
} GameServerStatsT;

//! game/voip server config info
typedef struct GameServerConfigInfoT
{
    uint32_t uFrontAddr;        //!< voipserver front-end address
    uint16_t uTunnelPort;       //!< voipserver-configured tunnel port
    uint16_t uDiagPort;         //!< gameserver/voipserver diagnostic port
    char strDeployInfo[GAMESERVER_MAX_DEPLOYINFO_LENGTH]; //!< deploy info
    char strDeployInfo2[GAMESERVER_MAX_DEPLOYINFO2_LENGTH]; //!< deploy info 2
    char strHostName[GAMESERVER_MAX_HOSTNAME_LENGTH];     //!< gameserver/voipserver hostname
    char strProductId[GAMESERVER_MAX_PRODUCTID_LENGTH];   //!< blaze service name that this gameserver is associated with
} GameServerConfigInfoT;

//! client removal reason
typedef struct GameServerClientRemovalReasonT
{
    uint32_t uRemovalReason;        //!< numeric code indicating removal cause
    char strRemovalReason[64];      //!< textual version of removal code
} GameServerClientRemovalReasonT;

//! game/voip server ping data
typedef struct PingDataT
{
    LobbyStateE eState;             //!< state shared between vs and gs in ping data
} PingDataT;

//! generic gameserver packet
typedef union GameServerPktT
{
    //! 'clst' [gameserver->voipserver] set client list
    struct
    {
        int32_t iClientId;      //!< client to set client list for
        int32_t iNumClients;    //!< number of clients in client id list
        uint32_t aClientIds[VOIPTUNNEL_MAXGROUPSIZE]; //!< client id list
    } SetList;
    //! 'conf' [voipserver<->gameserver] notify gameserver of voipserver configuration
    GameServerConfigInfoT Config;
    //! 'data' [gameserver<->voipserver] game data packet
    struct
    {
        int32_t iClientId;      //!< source client id
        int32_t iDstClientId;   //!< clientid to send to (if sendsingle)
        uint32_t uTimestamp;    //!< originating timestamp
        uint16_t uFlag;         //!< flags (this field is deprecated / unused)
        uint8_t aData[SOCKET_MAXUDPRECV];       //!< game data (must come last)
    } GameData;
    //! 'ngam' [gameserver->voipserver] create game notification
    struct
    {
        int32_t iIdent;             //!< game ident
        char    strGameUUID[64];    //!< game uuid
    } NewGame;
    //! 'dgam' [gameserver->voipserver] remove game notifications
    struct
    {
        uint32_t uGameEndState;
        uint32_t uRlmtChangeCount;  //!< count of redundancy limit changes (across all clients during that game)
    } DelGame;
    //! 'nusr' [gameserver->voipserver] add client to client list
    struct
    {
        int32_t iClientId;      //!< id of client to add
        int32_t iClientIdx;     //!< index of client to add
        int32_t iGameServerId;  //!< id of gameserver
        char strTunnelKey[PROTOTUNNEL_MAXKEYLEN]; //!< client's portion of the tunnel key
    } NewUser;
    //! 'dusr' [gameserver->voipserver] remove client from client list
    struct
    {
        int32_t iClientId;      //!< id of client to remove
        uint32_t uGameState;    //!< game state at time of removal (GAMESERVER_GAMESTATE_*)
        GameServerClientRemovalReasonT RemovalReason;
    } DelUser;
    //! 'ping' [voipserver<->gameserver] alive+health check
    PingDataT PingData;
    //! 'stat' [voipserver->gameserver] send latest stat dump for gameserver consumption
    GameServerStatsT GameStats;
    //! 'stat' [gameserver->voipserver] send latest stat info
    GameServerStatInfoT GameStatInfo;
    //! 'tokn' [voipserver<->gameserver] access token for blaze logins
    struct
    {
        char strAccessToken[8192];  //!< token we use for login
    } Token;

    uint8_t aData[1];
} GameServerPktT;

typedef struct GameServerPacketT
{
    GameServerPktHdrT Header;
    GameServerPktT    Packet;
} GameServerPacketT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

// network->host translate game packet fields
#define GameServerPacketNtoh(_pOutPacket, _pInpPacket, _uKind) GameServerPacketTranslate(&(_pOutPacket)->Packet, &(_pInpPacket)->Packet, _uKind, SocketNtohl, SocketNtohs)

// host->network translate game packet fields
#define GameServerPacketHton(_pOutPacket, _pInpPacket, _uKind) GameServerPacketTranslate(&(_pOutPacket)->Packet, &(_pInpPacket)->Packet, _uKind, SocketHtonl, SocketHtons)

#ifdef __cplusplus
extern "C" {
#endif

// host<->network translate a packet for sending (do not use directly; use macros instead)
uint32_t GameServerPacketTranslate(GameServerPktT *pOutPacket, const GameServerPktT *pInpPacket, uint32_t uKind, uint32_t (*pXlateL)(uint32_t uValue), uint16_t (*pXlateS)(uint16_t uValue));

// encode a packet for network transmission
uint32_t GameServerPacketEncode(GameServerPacketT *pPacket, uint32_t uKind, uint32_t uCode, void *pData, int32_t iDataSize);

// send packet to a remote host
int32_t GameServerPacketSend(BufferedSocketT *pBufferedSocket, const GameServerPacketT *pPacket);

// recv packet from a remote host
int32_t GameServerPacketRecv(BufferedSocketT *pBufferedSocket, GameServerPacketT *pPacket, uint32_t *pPacketSize);

// update latency statistics
void GameServerPacketUpdateLateStats(GameServerLateT *pLateOut, const GameServerLateT *pLateIn);

// update latency statistics with a single latency value
void GameServerPacketUpdateLateStatsSingle(GameServerLateT *pLateOut, uint32_t uLateIn);

// log output of latency statistics
void GameServerPacketLogLateStats(const GameServerLateT *pLateStat, const char *pPrefix, const char *pSuffix);

// update latency bin info with a single latency value
void GameServerPacketUpdateLateBin(const GameServerLateBinT *pLateBinCfg, GameServerLateBinT *pLateBinOut, uint32_t uLateIn);

// update latency bin statistics
void GameServerPacketUpdateLateBinStats(GameServerLateBinT *pLateBinOut, const GameServerLateBinT *pLateBinIn);

// sum up packet lost/sent stat samples from clients
void GameServerPacketUpdatePackLostSentStatSingle(GameServerPacketLostSentT *pPackLostSentOut, const GameServerPacketLostSentT *pPackLostSentIn);

// update packet lost/sent delta statistics
void GameServerPacketUpdatePackLostSentStats(GameServerPacketLostSentT *pPackLostSentOut, GameServerPacketLostSentT *pDeltaSumPackLostSentOut, const GameServerPacketLostSentT *pPackLostSentIn);

// read latency bin config from config file or command-line
void GameServerPacketReadLateBinConfig(GameServerLateBinT *pLateBinCfg, const char *pCommandTags, const char *pConfigTags, const char *pTagName);

// log packet
void GameServerPacketDebug(const GameServerPacketT *pPacket, int32_t iPacketSize, uint32_t uDebugLevel, const char *pStr1, const char *pStr2);

// update current/max/sum stats and reset, allow diving by 1024 to prevent overflowing
void GameServerUpdateGameStats(GameServerStatsT *pGameStats, GameServerStatsT *pGameStatCur, GameServerStatsT *pGameStatSum, GameServerStatsT *pGameStatMax, uint8_t bDivide1K);

#ifdef __cplusplus
};
#endif

#endif // _gameserverpkt_h

