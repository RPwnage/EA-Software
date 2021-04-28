/*H********************************************************************************/
/*!
    \File stressgameplay.h

    \Description
        This module implements the main game play logic for the dirtycast stress client.

    \Copyright
        Copyright (c) 2009 Electronic Arts Inc.

    \Version 07/08/2009 (mclouatre) First Version
*/
/********************************************************************************H*/

#ifndef _stressgameplay_h
#define _stressgameplay_h

/*** Include files ****************************************************************/
#include "DirtySDK/game/connapi.h"
#include "libsample/zfile.h"
#include "stressconfig.h"

namespace Blaze
{
    namespace BlazeNetworkAdapter
    {
        class ConnApiAdapter;
    }

    namespace GameManager
    {
        class Game;
    }
}

/*** Defines **********************************************************************/

/*** Type Definitions *************************************************************/

//! forward declaration
typedef struct StressGameplayMetricsRefT StressGameplayMetricsRefT;

//! represents a game input
typedef struct GameInputT
{
    uint32_t        uIndex;         //!< sequence number of the input
    char            aBuffer[32];    //!< buffer filled with random data
} GameInputT;

typedef struct GameLinkStatT
{
    NetGameLinkStatT Stats;
    int32_t iNextSend;   //!< keep next-send time
    int32_t iSendSeqn;
    int32_t iRecvSeqn;
    int32_t iRecvSize;
    int32_t iTotalSize;
    int32_t iStatTick;

    int32_t iLNakSent;
    int32_t iLPackLost;
    int32_t iLPackSaved;
    int32_t iRNakSent;
    int32_t iRPackLost;
    int32_t iSlen;
} GameLinkStatT;

typedef struct GameDistStatT
{
    NetGameDistStatT Stats[STRESS_MAX_PLAYERS];  //!< latest stat from the server
    uint32_t uSendIndex;        //!< current number of inputs sent
    uint32_t uRecvIndex;        //!< current number of minput received
    uint32_t uNextSeq;          //!< next expected seq 
    uint32_t uLastSendIndex;    //!< last reported inputs sent
    uint32_t uLastRecvIndex;    //!< last reported minputs received
    uint32_t uTotalRtt;         //!< total round trip time for our inputs
    uint32_t uNumInputs;        //!< number of paired inputs over the sampling window
    uint32_t uMinRtt;           //!< minimum round trip time for our inputs
    uint32_t uMaxRtt;           //!< maximum round trip time for our inputs
    uint32_t uDroppedInputs;    //!< number of inputs that were dropped by the server
    int32_t iInputQueue;        //!< high water mark of our dist input queue
    int32_t iOutputQueue;       //!< high water mark of our dist output queue
    int32_t iPlat;              //!< end-to-end latency in inputs
    uint32_t aFrameTicks[64];   //!< send ticks for inputs
} GameDistStatT;

class StressGamePlayC
{
public:
    StressGamePlayC(Blaze::BlazeNetworkAdapter::ConnApiAdapter *pConnApiAdapter, StressGameplayMetricsRefT *pMetricsSystem);
    ~StressGamePlayC();

    void GameStatsUpdate(NetGameLinkRefT *pGameLink, GameLinkStatT *pLinkStat, int32_t iGameConn, int32_t iPacketLen, uint32_t uCurTick);
    int32_t SetupNetGameDist();
    void TeardownNetGameDist();
    int32_t LinkSend(NetGameLinkRefT *pGameLink, GameLinkStatT *pLinkStat, int32_t iGameConn, uint32_t uCurTick);   
    void LinkRecv(NetGameLinkRefT *pGameLink, GameLinkStatT *pLinkStat, int32_t iGameConn, uint32_t uCurTick);
    void DistSend(int32_t iGameConn, uint32_t uCurTick);
    void DistRecv(int32_t iGameConn, uint32_t uCurTick);

    void SetVerbosity(int32_t iVerbosityLevel);
    void SetDebugPoll(bool bEnabled);
    void SetLateRate(int32_t iLateRate);
    void SetPacketSize(uint32_t uPacketSize);
    void SetPacketsPerSec(uint32_t uPacketsPerSec);
    void SetNumberOfPlayers(int32_t iNumberOfPlayers);
    void SetNetGameDistStreamId(uint32_t uGameStreamId);
    void SetPersona(const char *pPersona);
    void SetWriteCsv(bool bWriteCsv);
    void SetGame(Blaze::GameManager::Game *pGame);
    void SetGameStarted();
    void SetGameEnded();
    bool GetDist();
    void SetDist();
    void SetPeer2Peer(bool bPeer2Peer);
    void SetStreamMessageSize(uint32_t size);

    bool Update(uint32_t uCurTick);

private:

    static void ConnApiCb(ConnApiRefT *pConnApi, ConnApiCbInfoT *pCbInfo, void *pUserData);
    static void PermanentLog(const char *pFormat, ...);
    static void GamePlayFailure();
    static void StreamRecvHook(NetGameLinkStreamT *pStream, int32_t iSubchan, int32_t iKind, void *pBuffer, int32_t iLen);

    int32_t Poll(uint64_t &uCurTick);
    void    Send(uint32_t uCurTick);
    void    Recv(uint32_t uCurTick);

    // game play
static   bool                                   m_bGameFailed;
    bool                                        m_bUseDist;
    bool                                        m_bPeer2peer;
    bool                                        m_bGameStarted;
    bool                                        m_bDebugPoll;
    bool                                        m_bCrcCheckDist;
    uint32_t                                    m_uPacketSize;
    uint32_t                                    m_uPacketsPerSec;
    int32_t                                     m_iUsecsPerFrame;
    int32_t                                     m_iVerbosity;
    int32_t                                     m_iLateRate;
    int32_t                                     m_iNumberOfPlayers;
    int32_t                                     m_iClientId;   // Local client index in ConnApi client list

    // ConnApiAdapter
    Blaze::BlazeNetworkAdapter::ConnApiAdapter  *m_pConnApiAdapter;
    Blaze::GameManager::Game                    *m_pGame;

    // NetGameLink
    NetGameLinkRefT                             *m_pNetGameLink[STRESS_MAX_PLAYERS];
    GameLinkStatT                               m_LinkStat[STRESS_MAX_PLAYERS];

    // NetGameDist
    NetGameDistRefT                             *m_pNetGameDist;
    GameDistStatT                               m_DistStat;

    // NetGameLinkStream
    NetGameLinkStreamT                          *m_pNetGameLinkStream[STRESS_MAX_PLAYERS];
    uint32_t                                    m_uStreamMessageSize;
    
    // Metrics
    StressGameplayMetricsRefT                   *m_pStressGameplayMetrics;

    uint32_t                                    m_uGameStreamId;
    uint32_t                                    m_uLastFrameRecv;
    ZFileT                                      m_iZFile;
    char                                        m_strPersona[64];
    bool                                        m_bWriteCsv;
};


#endif

