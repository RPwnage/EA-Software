#ifndef _stressconfig_h
#define _stressconfig_h

#include "servermetrics.h"

#define STRESS_EVENT_GAME                   (1)
#define STRESS_EVENT_PLAY                   (2)
#define STRESS_EVENT_DELETE                 (3)
#define STRESS_EVENT_END                    (4)
#define STRESS_EVENT_CREATE                 (5)

#define STRESS_MAX_PERSONA_LENGTH       (32)
#define STRESS_MAX_PLAYERS              (32)
#define STRESS_MACHINE_ADDRESS_LENGTH   (80)
#define STRESS_MAX_STREAM_MSG_LENGTH    (4096)

typedef enum StressGameCountActionE
{
    kNone,
    kExit,
    kDisableMetrics
} StressGameCountActionE;

typedef struct StressPlayerT
{
    char strPers[STRESS_MAX_PERSONA_LENGTH];
    char strMachineAddr[STRESS_MACHINE_ADDRESS_LENGTH];
    uint32_t uAddr;
    int32_t iIdent;
} StressPlayerT;

typedef struct StressGameT
{
    StressPlayerT aPlayers[STRESS_MAX_PLAYERS];
    int32_t iIdent;
    int32_t iCount;
    uint32_t uWhen;
    int32_t iMinSize;
    int32_t iMaxSize;
    char strName[STRESS_MAX_PERSONA_LENGTH];
    char strUUID[64];
} StressGameT;

typedef struct StressConfigC
{
    char strService[50];
    char strEnvironment[5];
    char strGameStream[5];
    char strUsername[48];
    char strPassword[48];
    char strPersona[48];
    char strRdirName[64];       //!< redirector name override (optional)
    char strGameProtocolVersion[64]; //!< allow match making with this protocol string only, override (optional), default match any "match_any_protocol"
    char strMatchmakingScenario[64]; //!< specify with matchmaking scenario to use
    uint16_t uStressIndex;      //!< filled in dynamically from command-line param
    uint16_t uGamePort;         //!< offset from uStressIndex
    uint16_t uPacketsPerSec;    //!< packets to send per second
    uint16_t uPacketSize;       //!< packet size
    uint16_t uMinPlayers;       //!< minimum number of players to wait before before starting a game
    uint16_t uMaxPlayers;       //!< maximum number of players per game
    int32_t iWaitForPlayers;    //!< time to wait for players to join before starting the game
    int32_t iGameLength;        //!< length of time a game should run
    int32_t iWaitBetweenGames;  //!< time to wait after a game before matchmaking again
    int32_t iMaxGameCount;      //!< the maximum number of games to play, 0 for unlimited
    StressGameCountActionE eMaxGameCountAction; //!< what to do when we reach our max game count
    uint32_t uGameStream;       //!< stream to use for the game
    uint32_t uStreamMessageSize; //!< NetGameLinkStream data size to send
    uint32_t uLateRate;         //!< latency printing rate in seconds
    int32_t iDummyVoipSubPacketSize;//!< size of artificially injected test voip sub-packets
    int32_t iDummyVoipSendPeriod; //!< time delta between each injected test voip sub-packets
    uint8_t uDebugLevel;        //!< debug level
    bool bUseDist;              //!< do we use dist (or link)
    bool bDebugPoll;            //!< enable output tracking polling behavior
    bool bUseSecureBlazeConn;   //!< use an encrypted SSL connection when connecting to the blaze server instead of an insecure connection
    bool bUseStressLogin;       //!< use stress login instead of express login (requires BlazeSDK alterations)
    uint8_t uGameMode;          //!< game mode (0=p2p 1=peer link 2=peer dist)
    bool bWriteCsv;             //!< write stats to csv to be graphed

    // metrics related
    char strLocalHostName[256];
    char strMetricsAggregatorHost[256];
    char strDeployInfo[128];
    char strDeployInfo2[128];
    uint32_t uMetricsPushingRate;
    ServerMetricsFormatE eMetricsFormat;
    uint16_t uMetricsAggregatorPort;
    uint8_t uMetricsEnabled;    //!< defaults on, but can be turned off via config or at runtime

    //!< signal flag state
    uint32_t uSignalFlags;
} StressConfigC;

#endif
