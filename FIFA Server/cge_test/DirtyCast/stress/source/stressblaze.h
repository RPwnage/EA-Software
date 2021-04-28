#ifndef _stressblaze_h
#define _stressblaze_h

#include "DirtySDK/dirtysock.h" // for types
#include "BlazeSDK/blazeerrors.h" // for types
#include "BlazeSDK/job.h"
#include "BlazeSDK/gamemanager/matchmakingsession.h"
#include "stressconfig.h"

namespace Blaze
{
    class BlazeHub;
    
    namespace BlazeNetworkAdapter
    {
        class ConnApiAdapter;
    }

    namespace GameManager
    {
        class Game;
    }
}

class StressBlazeStateListenerC;
class StressBlazeGameListenerC;
class GameManagerListenerC;
struct StressGameT;

typedef uint8_t (StressCallbackT)(void *pUserData, int32_t iEvent, StressGameT *pGame);

struct StressConfigC;

class StressBlazeC
{
    friend class StressBlazeStateListenerC;
    friend class StressBlazeGameListenerC;
    friend class GameManagerListenerC;

public:

    enum StressBlazeStateE
    {
        kDisconnected,
        kConnecting,
        kWaitingForQoS,
        kConnected,
        kCreatedGame,
        kWaitingForPlayers,
        kGameCloseToAllJoinsPending,
        kWaitingForJoinCompletions,
        kGameStartPending,
        kGameStarted,
        kHostingGame,
        kFailed
    };

    static StressBlazeC *Get();
    void StressBlazeUpdate();
    uint32_t GetPlayerCount();
    void SetConfig(StressConfigC *config);
    void SetCallback(StressCallbackT* pCallback, void* pCallbackData);
    uint32_t GetDebugLevel(void);
    StressBlazeStateE GetState();
    static const char * GetStateText(StressBlazeStateE eState);

    void LoginToBlaze();
    void Matchmake(bool bCreate);
    void SetFailedState();

    void SetGame(Blaze::GameManager::Game *game);
    Blaze::GameManager::Game *GetGame();
    Blaze::BlazeNetworkAdapter::ConnApiAdapter *GetConnApiAdapter();

private:
    StressBlazeC();

    void SetState(StressBlazeStateE eState);

    void CloseGameToAllJoinsCb(Blaze::BlazeError error, Blaze::GameManager::Game * game);
    void StartMatchmakingScenarioCb(Blaze::BlazeError err, Blaze::JobId jobId, Blaze::GameManager::MatchmakingScenario *matchmakingScenario, const char8_t *errDetails);

    Blaze::ClientPlatformType GetClientPlatformType();

    StressBlazeStateListenerC *GetStateListener();
    StressBlazeGameListenerC *GetGameListener();
    GameManagerListenerC *GetGameManagerListener();

    Blaze::BlazeHub *m_pBlazeHub;
    Blaze::BlazeNetworkAdapter::ConnApiAdapter *m_pConnApiAdapter;
    StressBlazeStateE m_eState;
    StressConfigC *m_pConfig;
    StressCallbackT* m_pCallback;
    void* m_pCallbackData;
    StressGameT m_Game;
    int32_t m_iWaitForPlayersTime;
    int32_t m_iWaitForJoinCompletion;
    int32_t m_iGameStartTime;
    int32_t m_iWaitForNextGameTime;
    int32_t m_iGameCount;
    uint32_t m_uNbOfPlayersToJoin;
    uint32_t m_uPlayerJoinedCount;
    int32_t m_iLastConnectionChangeTime;
    bool m_bCreatingGame;
    Blaze::GameManager::Game *m_BlazeGame;
};

#endif

