#ifndef _gameserverconfig_h
#define _gameserverconfig_h

#include "DirtySDK/proto/prototunnel.h"
#include "servermetrics.h"
#include "gameserverpkt.h"

//! gameserver events
#define GAMESERVER_EVENT_GAME                   (1)
#define GAMESERVER_EVENT_PLAY                   (2)
#define GAMESERVER_EVENT_DELETE                 (3)
#define GAMESERVER_EVENT_END                    (4)
#define GAMESERVER_EVENT_CREATE                 (5)
#define GAMESERVER_EVENT_LOGIN_FAILURE          (6)
#define GAMESERVER_EVENT_GAME_CREATION_FAILURE  (7)
#define GAMESERVER_EVENT_DRAIN                  (8)

//! gameserver voip server connection state
typedef enum
{
    GAMESERVER_VOIPSERVER_INIT = 0,
    GAMESERVER_VOIPSERVER_CONNECTING,
    GAMESERVER_VOIPSERVER_CONNECTED,
    GAMESERVER_VOIPSERVER_ONLINE,
    GAMESERVER_VOIPSERVER_DISCONNECTED
} GameServerVoipStateE;

#define GAMESERVER_MAX_PERSONA_LENGTH       (32)
#define GAMESERVER_MAX_CLIENTS              (32)
#define GAMESERVER_MAX_PLAYERS              (16)

typedef struct GameServerPlayerT
{
    char strPers[GAMESERVER_MAX_PERSONA_LENGTH];
} GameServerPlayerT;

typedef struct GameServerClientT
{
    GameServerPlayerT aPlayers[GAMESERVER_MAX_PLAYERS];
    int32_t iIdent;
    int32_t iRefCount;      //!< a client really refers to a console, this refcount is required for MLU support
    uint8_t bReserved;      //!< has a disconnect reservation (we ignore other types of reservations until they join)
    uint8_t _pad[3];
} GameServerClientT;

typedef struct GameServerGameT
{
    GameServerClientT aClients[GAMESERVER_MAX_CLIENTS]; //!< list of clients in the game must be indexed using getSlotId() - 1 (not getConnectionSlotId())
    int32_t iIdent;
    int32_t iCount;
    uint32_t uWhen;
    char strName[GAMESERVER_MAX_PERSONA_LENGTH];
    char strUUID[64];
    char strGameMode[64];
} GameServerGameT;

typedef struct GameServerCustomAttrT
{
    char strKey[256];       //!< attribute key
    char strValue[256];     //!< attribute value
} GameServerCustomAttrT;

typedef struct GameServerConfigT
{
    //! metrics aggregator config
    char strMetricsAggregatorHost[256];     //!< metrics aggregator host
    uint32_t uMetricsAggregatorPort;        //!< metrics aggregator port
    uint32_t uMetricsPushingRate;           //!< reporting rate
    uint32_t uMetricsMaxAccumulationCycles; //!< maximum number of back-to-back reporting cycles that can push metrics values in accumulators
    uint32_t uMetricsFinalPushingRate;      //!< final reporting rate
    ServerMetricsFormatE eMetricsFormat;    //!< metrics format

    //! subspace config
    char strSubspaceSidekickAddr[256];  //!< subspace sidekick app host (probably localhost)
    uint32_t uSubspaceSidekickPort;     //!< subspace sidekick app port (0 is disabled, probably 5005)
    uint32_t uSubspaceProvisionRateMin; //!< if we haven't done a provision in x time, do one if no game is in session
    uint32_t uSubspaceProvisionRateMax; //!< skip provision if only a short time has passed since last time

    char strServer[GAMESERVER_MAX_PRODUCTID_LENGTH];      //!< blaze service name
    char strHostName[GAMESERVER_MAX_HOSTNAME_LENGTH];     //!< hostname from voipserver
    char strDeployInfo[GAMESERVER_MAX_DEPLOYINFO_LENGTH]; //!< deploy info from voipserver
    char strDeployInfo2[GAMESERVER_MAX_DEPLOYINFO2_LENGTH];//!< deploy info 2 from voipserver
    char strVoipServerAddr[16]; //!< address of voipserver
    char strEnvironment[16];    //!< the environment to use
    char strPingSite[64];       //!< ping site
    char strRdirName[64];       //!< redirector name override (optional)
    char strGameProtocolVersion[64];       //!< allow match making with this protocol string only, override (optional), default match any "match_any_protocol"
    char strCertFile[64];       //!< nucleus cert filename path
    char strKeyFile[64];        //!< nucleus key filename path
    uint32_t uVoipServTime;     //!< voipserver timeout (in milliseconds)
    int32_t iRedundancyPeriod;  //!< comm redundancy config - time period (in ms) over which local->remote packet loss is calculated for redundancy
    uint32_t uRedundancyThreshold; //!< unreasonable server-to-client packet loss threshold (in number of packets per 100000) - When packet loss exceeds this threshold, redundancylimit is applied.
    uint32_t uRedundancyLimit;  //!< max limit (in bytes) to be used when default limit results in unreasonable packet loss
    int32_t iRedundancyTimeout; //!< time limit (in sec) without exceeding server-to-client packet loss threshold before backing down to default redundancy config
    uint16_t uVoipServerPort;   //!< port to voip server
    uint16_t uDiagnosticPort;   //!< port gameserver listens on for diagnostic requests
    uint16_t uFixedUpdateRate;  //!< update rate for fixed update processing
    uint8_t  uMaxClients;       //!< maximum number of supported clients
    uint8_t  uMaxSpectators;    //!< maximum number of supported spectators
    uint8_t  uDebugLevel;       //!< debug level
    uint8_t  bSecure;           //!< TRUE if secure connect used to connect to the blazeserver
    uint8_t uNumAttributes;     //!< number of attributes
    GameServerCustomAttrT aCustomAttributes[256];   //!< array of attributes that we pass along to the create game
} GameServerConfigT;

#endif
