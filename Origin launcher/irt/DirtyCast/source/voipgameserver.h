/*H********************************************************************************/
/*!
    \File voipgameserver.h

    \Description
        This module implements the main logic for the VoipTunnel server
        when acting as a dedicated server for Blaze

    \Copyright
        Copyright (c) 2006-2015 Electronic Arts Inc.

    \Version 03/24/2006 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _voipgameserver_h
#define _voipgameserver_h

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

#include "dirtycast.h"
#include "gameserverpkt.h"

/*** Defines **********************************************************************/

//! max game history
#define VOIPSERVER_GAMEHISTORYCNT       5

//!< gameserver stat update sent every fifteen seconds
#define VOIPSERVER_GS_STATRATE          (15*1000)

//! max number of client removal reasons that will be tracked
#define VOIPSERVER_MAXCLIENTREMOVALINFO         (100)

/*** Type Definitions *************************************************************/

//! forward declarations
typedef struct VoipServerRefT VoipServerRefT;
typedef struct VoipGameServerRefT VoipGameServerRefT;

//! game history stats
typedef struct GameServerStatHistoryT
{
    time_t uGameStart;              //!< start time of game
    time_t uGameEnd;                //!< end time of game
    GameServerStatsT GameStatsSum;  //!< accumulated stats
    GameServerStatsT GameStatsMax;  //!< max bandwidth numbers for a single stat interval
    uint32_t uRlmtChangeCount;      //!< count of redundancy limit changes (across all clients during that game)
} GameServerStatHistoryT;

//! gameserver connection state
typedef struct GameServerT
{
    time_t uConnectTime;            //!< when connected to voipserver
    BufferedSocketT *pBufferedSocket;//!< voipserver<->gameserver socket
    struct sockaddr Address;        //!< gameserver address
    LobbyStateE eLobbyState;        //!< lobby connection state
    uint32_t uLastPing;             //!< last ping to gameserver
    uint32_t uLastSend;             //!< last send to gameserver
    uint32_t uLastRecv;             //!< last recv from gameserver
    uint32_t uLastStat;             //!< last stat update sent to gameserver
    int32_t iLastSendResult;        //!< last send result
    GameServerPacketT PacketData;   //!< packet bufer
    int32_t iPacketSize;            //!< size of packet data
    GameServerStatsT GameStats;     //!< active stat buffer
    GameServerStatsT GameStatsCur;  //!< most recent stats
    GameServerStatsT GameStatsSum;  //!< accumulated stats
    GameServerStatsT GameStatsMax;  //!< max bandwidth numbers for a single stat interval
    GameServerStatHistoryT GameStatHistory[VOIPSERVER_GAMEHISTORYCNT]; //!< history of most recent games
    char strGameUUID[64];                   //!< game uuid
    uint32_t uNumLateStats;                 //!< number of accumulated late stats
    GameServerStatInfoT StatInfo;           //!< statistics
    GameServerPacketLostSentT PackLostSent; //!< prev total packet loss sent
    GameServerLateT OtbLate;                //!< one-second outbound latency accumulator
    time_t uStuckDetectionStartTime;        //!< time (in seconds) used to detect if a game has been stuck (i.e. with 0 player for the duration of VOIPGAMESERVER_STUCK_DETECTION_DELAY)
    uint32_t uLastActvClientsCnt;           //!< last known count of active clients in the game (used for stuck game detection)
    uint32_t uTotalGames;                   //!< total number of games played
    uint32_t uTotalRlmtChangeCount;         //!< total number of rlmt changes across all games played for that game server
    uint32_t uAccRlmtChangeCount;           //!< accumulation of rlmt changes for that game server over the statistic sampling period
    uint16_t uDiagnosticPort;               //!< gameserver diagnostic port (logged in voipserver diagnostic report)
    uint8_t bConnected;                     //!< true if connected, else false
    uint8_t bGameActive;                    //!< true if a game is active, else false
    uint8_t bGameStuck;                     //!< true if gameserver is "stuck"
    uint8_t _pad;
} GameServerT;

//! client removal tracker
typedef struct GameServerClientRemovalInfoT
{
    GameServerClientRemovalReasonT RemovalReason;
    uint32_t aRemovalStateCt[GAMESERVER_NUMGAMESTATES];
} GameServerClientRemovalInfoT;

//! metrics for gameserver module
typedef struct VoipGameServerStatT
{
    GameServerPacketLostSentT DeltaPackLostSent; //!< track packet lost sent delta

    uint32_t iGamesAvailable;     //!< available games
    uint32_t iGamesOffline;       //!< offline games
    uint32_t iGamesActive;        //!< active games according to the gameservers
    uint32_t iGamesZombie;        //!< games that have been drained for scale down
    uint32_t iGamesStuck;         //!< number of games in a 'stuck' state (no users or >24 hours elapsed time)
    uint32_t iGamesFailedToRecv;  //!< consecutive games that never received any packets
    uint32_t uGameServDiscCount;  //!< number of gameservers disconnected in this metrics interval
    time_t uLastGameServDiscTime; //!< time of most recent GameServer disconnect

    uint32_t uAccRlmtChangeCount;   //!< accumulation of rlmt changes for that game server over the statistic sampling period

    // game end state tracker
    uint32_t aGameEndStateCt[GAMESERVER_NUMGAMESTATES];

    uint32_t aLastGameEndStateCt[GAMESERVER_NUMGAMESTATES]; //!< game end state counts from last sampling period used to calculate reporting deltas

    // client removal info tracker
    GameServerClientRemovalInfoT aClientRemovalInfo[VOIPSERVER_MAXCLIENTREMOVALINFO];

    GameServerClientRemovalInfoT aLastClientRemovalInfo[VOIPSERVER_MAXCLIENTREMOVALINFO]; //!< removal info from last sampling period used to calculate deltas
} VoipGameServerStatT;

/*** Functions ********************************************************************/

// create the module
VoipGameServerRefT *VoipGameServerInitialize(VoipServerRefT *pVoipServer, const char *pCommandTags, const char *pConfigTags);

// update the module
uint8_t VoipGameServerProcess(VoipGameServerRefT *pRef, uint32_t *pSignalFlags, uint32_t uCurTick);

// get base module
VoipServerRefT *VoipGameServerGetBase(VoipGameServerRefT *pRef);

// get gameserver based on index
GameServerT *VoipGameServerMatchIndex(VoipGameServerRefT *pRef, int32_t iGameServer);

// status function
int32_t VoipGameServerStatus(VoipGameServerRefT *pRef, int32_t iStatus, int32_t iValue, void *pBuf, int32_t iBufSize);

// control function
int32_t VoipGameServerControl(VoipGameServerRefT *pRef, int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue);

// drain the module
uint32_t VoipGameServerDrain(VoipGameServerRefT *pRef);

// destroy the module
void VoipGameServerShutdown(VoipGameServerRefT *pRef);

#endif // _voipgameserver_h

