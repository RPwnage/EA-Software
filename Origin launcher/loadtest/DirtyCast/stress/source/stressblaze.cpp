#include <EABase/eabase.h>

#include "EAAssert/eaassert.h"
#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/debug.h"
#include "BlazeSDK/version.h"
#include "BlazeSDK/blazenetworkadapter/connapiadapter.h"
#include "BlazeSDK/blazeerrors.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/player.h"
#include "coreallocator/icoreallocator.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/version.h"
#include "BlazeSDK/gamemanager/playerjoindatahelper.h"

#include "stress.h"
#include "stressblaze.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/voip/voip.h"
#include "dirtycast.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "stressconfig.h"



#define STRESSBLAZE_MEMID ('gsbb')
#define WAIT_JOIN_TIMEOUT 10000

#define STRESSBLAZE_FILTERDEBUG (FALSE)  // if enabled, print %% FILTER %% in front of lines that would be filtered instead of actually filtering them

class StressBlazeAllocatorC: public EA::Allocator::ICoreAllocator
{
public:
    static StressBlazeAllocatorC* Get();

    void *Alloc(size_t size, const char *name, unsigned int flags);
    void *Alloc(size_t size, const char *name, unsigned int flags, unsigned int align, unsigned int alignOffset = 0);
    void Free(void *block, size_t size=0);

private:
    StressBlazeAllocatorC();

    int32_t mMemgroup;
    uint8_t mMemgroupInited;
};

class StressBlazeStateListenerC: public Blaze::BlazeStateEventHandler
{
public:
    StressBlazeStateListenerC() : mLoginAttempt(0) { }

    void onConnected() override;
    void onDisconnected(Blaze::BlazeError errorCode) override;
    void onAuthenticated(uint32_t userIndex) override;
    void onDeAuthenticated(uint32_t userIndex) override;

private:
    void ExpressLoginCb(const Blaze::Authentication::LoginResponse *response, Blaze::BlazeError blazeError, Blaze::JobId JobId);
#ifdef USE_STRESS_LOGIN
    void StressLoginCb(const Blaze::Authentication::LoginResponse *response, Blaze::BlazeError blazeError, Blaze::JobId JobId);
#endif

    int32_t mLoginAttempt;
};

class StressBlazeGameListenerC: public Blaze::GameManager::GameListener
{
public:
    StressBlazeGameListenerC() {}
    void GameCreated(Blaze::BlazeError err, Blaze::JobId jobId, Blaze::GameManager::Game* game);
    void GameReset(Blaze::BlazeError err, Blaze::GameManager::Game* game);
    void GameEnded(Blaze::BlazeError err, Blaze::GameManager::Game* game);
    void InitGameComplete(Blaze::BlazeError err, Blaze::GameManager::Game* game);
    void PlayerKicked(Blaze::BlazeError err, Blaze::GameManager::Game* game, const Blaze::GameManager::Player *);

    virtual void onPreGame(Blaze::GameManager::Game *game) override;
    virtual void onGameStarted(Blaze::GameManager::Game *game) override;
    virtual void onGameEnded(Blaze::GameManager::Game *game) override;
    virtual void onGameReset(Blaze::GameManager::Game *game) override;
    virtual void onReturnDServerToPool(Blaze::GameManager::Game *game) override;
    virtual void onGameAttributeUpdated(Blaze::GameManager::Game *game, const Blaze::Collections::AttributeMap *changedAttributeMap) override;
    virtual void onGameSettingUpdated(Blaze::GameManager::Game *game) override;
    virtual void onPlayerCapacityUpdated(Blaze::GameManager::Game *game) override;
    virtual void onPlayerJoining(Blaze::GameManager::Player *player) override;
    virtual void onPlayerJoiningQueue(Blaze::GameManager::Player *player) override;
    virtual void onPlayerJoinComplete(Blaze::GameManager::Player *player) override;
    virtual void onQueueChanged(Blaze::GameManager::Game *game) override;
    virtual void onPlayerRemoved(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *removedPlayer, Blaze::GameManager::PlayerRemovedReason playerRemovedReason, Blaze::GameManager::PlayerRemovedTitleContext titleContext) override;
    virtual void onPlayerAttributeUpdated(Blaze::GameManager::Player *player, const Blaze::Collections::AttributeMap *changedAttributeMap) override;
    virtual void onPlayerJoinedFromQueue(Blaze::GameManager::Player *player) override;
    virtual void onAdminListChanged(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *adminPlayer, Blaze::GameManager::UpdateAdminListOperation  operation, Blaze::GameManager::PlayerId) override;
    virtual void onPlayerTeamUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::TeamIndex previousTeamIndex) override;
    virtual void onGameTeamIdUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::TeamId oldTeamId, Blaze::GameManager::TeamId newTeamId) override;
    virtual void onPlayerCustomDataUpdated(Blaze::GameManager::Player *player) override;
    virtual void onHostMigrationStarted(Blaze::GameManager::Game *game) override;
    virtual void onHostMigrationEnded(Blaze::GameManager::Game *game) override;
    virtual void onNetworkCreated(Blaze::GameManager::Game *game) override;
    virtual void onMessageReceived(Blaze::GameManager::Game *game, const void *buf, size_t bufSize, Blaze::BlazeId senderId, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags) override;
    virtual void onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState) override;
    virtual void onBackFromUnresponsive(Blaze::GameManager::Game *game) override;
};

class GameManagerListenerC: public Blaze::GameManager::GameManagerAPIListener
{
public:
    virtual void onGameCreated(Blaze::GameManager::Game *createdGame) override; 
    virtual void onGameDestructing(Blaze::GameManager::GameManagerAPI *gameManagerAPI, Blaze::GameManager::Game *dyingGame, Blaze::GameManager::GameDestructionReason reason) override;
    virtual void onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult matchmakingResult, const Blaze::GameManager::MatchmakingScenario* matchmakingScenario, Blaze::GameManager::Game *game) override;
    virtual void onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario* matchmakingScenario, const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList) override;
    virtual void onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList *gameBrowserList, const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *removedGames, const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *updatedGames) override;
    virtual void onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *gameBrowserList) override;
    virtual void onUserGroupJoinGame(Blaze::GameManager::Game *game, Blaze::GameManager::Player* player, Blaze::GameManager::UserGroupId) override;
    virtual void onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList* pList) override;

    #if BLAZE_SDK_VERSION < BLAZE_SDK_MAKE_VERSION(15, 1, 1, 8, 0)
    virtual void onMatchmakingSessionFinished(Blaze::GameManager::MatchmakingResult matchmakingResult, const Blaze::GameManager::MatchmakingSession* matchmakingSession, Blaze::GameManager::Game *game) override {}
    virtual void onMatchmakingAsyncStatus(const Blaze::GameManager::MatchmakingSession* matchmakingSession, const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList) override {}
    #endif
};

ICOREALLOCATOR_INTERFACE_API EA::Allocator::ICoreAllocator *EA::Allocator::ICoreAllocator::GetDefaultAllocator()
{
   return StressBlazeAllocatorC::Get();
}

StressBlazeAllocatorC::StressBlazeAllocatorC()
:mMemgroup(0), mMemgroupInited(FALSE)
{

}

StressBlazeAllocatorC* StressBlazeAllocatorC::Get()
{
    static StressBlazeAllocatorC g_StressBlazeAllocator;

    return(&g_StressBlazeAllocator);
}

void *StressBlazeAllocatorC::Alloc(size_t size, const char *name, unsigned int flags)
{
    if (!mMemgroupInited)
    {
        mMemgroupInited = TRUE;
        DirtyMemGroupQuery(&mMemgroup, NULL);
    }
    
    // allocate state for ref
    return(DirtyMemAlloc((int32_t)size, STRESSBLAZE_MEMID, mMemgroup, NULL));
}

void *StressBlazeAllocatorC::Alloc(size_t size, const char *name, unsigned int flags, unsigned int align, unsigned int alignOffset)
{
    return(Alloc(size, name, flags));
}

void StressBlazeAllocatorC::Free(void *block, size_t size)
{
    DirtyMemFree(block, STRESSBLAZE_MEMID, mMemgroup, NULL);
}

void StressBlazeStateListenerC::ExpressLoginCb(const Blaze::Authentication::LoginResponse *response, Blaze::BlazeError blazeError, Blaze::JobId JobId)
{
    if (blazeError == Blaze::AUTH_ERR_INVALID_USER)
    {
        StressPrintf("stressblaze: login failed with invalid user, make sure the account exists\n");
    }
}

#ifdef USE_STRESS_LOGIN
void StressBlazeStateListenerC::StressLoginCb(const Blaze::Authentication::LoginResponse *response, Blaze::BlazeError blazeError, Blaze::JobId JobId)
{
    if (blazeError == 0)
    {
        StressPrintf("stressblaze: StressLogin Time %d\n", NetTick());
    }
    else
    {
        StressPrintf("stressblaze: StressLogin error 0x%08x\n", blazeError);
    }
}
#endif

void StressBlazeStateListenerC::onConnected()
{
    // we are connected!
    StressPrintf("stressblaze: connected to blaze server\n");

    if (++mLoginAttempt <= 2)
    {
        if (StressBlazeC::Get()->m_pConfig->bUseStressLogin)
        {
#ifdef USE_STRESS_LOGIN
            Blaze::Authentication::StressLoginRequest loginRequest;
            loginRequest.setEmail(StressBlazeC::Get()->m_pConfig->strUsername);
            loginRequest.setPersonaName(StressBlazeC::Get()->m_pConfig->strPersona);
            #if BLAZE_SDK_VERSION >= BLAZE_SDK_MAKE_VERSION(15, 1, 1, 8, 0)
            loginRequest.setAccountId((uint64_t)StressBlazeC::Get()->m_pConfig->uStressIndex);
            #else
            loginRequest.setNucleusId((uint64_t)StressBlazeC::Get()->m_pConfig->uStressIndex);
            #endif
            loginRequest.setClientType(Blaze::CLIENT_TYPE_GAMEPLAY_USER);

            Blaze::Authentication::AuthenticationComponent *auth = StressBlazeC::Get()->m_pBlazeHub->getComponentManager(0)->getAuthenticationComponent();
            auth->stressLogin(loginRequest, Blaze::Authentication::AuthenticationComponent::StressLoginCb(this, &StressBlazeStateListenerC::StressLoginCb));
#else
            StressPrintf("stressblaze: not setup for stress login, please set 'client_export = true' for stressLogin in authenticationcomponent.rpc and rebuild the solution with '-D:use_stress_login=true'\n");
#endif
        }
        else
        {
            Blaze::Authentication::ExpressLoginRequest loginRequest;
            loginRequest.setEmail(StressBlazeC::Get()->m_pConfig->strUsername);
            loginRequest.setPassword(StressBlazeC::Get()->m_pConfig->strPassword);
            loginRequest.setPersonaName(StressBlazeC::Get()->m_pConfig->strPersona);
            loginRequest.setClientType(Blaze::CLIENT_TYPE_GAMEPLAY_USER);
        
            Blaze::Authentication::AuthenticationComponent *auth = StressBlazeC::Get()->m_pBlazeHub->getComponentManager(0)->getAuthenticationComponent();
            auth->expressLogin(loginRequest, Blaze::Authentication::AuthenticationComponent::ExpressLoginCb(this, &StressBlazeStateListenerC::ExpressLoginCb));
        }
    }
}

void StressBlazeStateListenerC::onDisconnected(Blaze::BlazeError errorCode)
{
    StressPrintf("stressblaze: disconnected from blaze server\n");

    StressBlazeC::Get()->SetState(StressBlazeC::kDisconnected);
}

void StressBlazeStateListenerC::onAuthenticated(uint32_t userIndex)
{
    StressPrintf("stressblaze: BlazeStateProvider::onAuthenticated called for player %u\n", userIndex);

    //We are connected and logged in! Now we wait for QoS to complete
    StressBlazeC::Get()->SetState(StressBlazeC::kWaitingForQoS);
}

void StressBlazeStateListenerC::onDeAuthenticated(uint32_t userIndex)
{
    StressPrintf("stressblaze: BlazeStateProvider::onDeAuthenticated called for player %u\n", userIndex);

    // set login attempts back to zero to allow for logins
    mLoginAttempt = 0;
}

typedef struct StressFilterEntryT
{
    uint32_t    uDebugLevel;        // min debug level to set for text to not be filtered
    const char *pFilterText;        // text to check for filtering
} StressFilterEntryT;

static StressFilterEntryT _strFilterReq[] =
{
    { 3, "UtilComponent::ping" },
    { 0, NULL                  }
};

static StressFilterEntryT _strFilterResp[] =
{
    { 3, "UtilComponent::ping" },
    { 0, NULL                  }
};

static StressFilterEntryT _strFilterBody[] =
{
    { 3, "  PingResponse = {", },
    { 0, NULL                  }
};

static int32_t _FilterFunction(const char8_t *pText, uint32_t uDebugLevel, uint32_t *pFilteredLevel)
{
    static bool _bFilteringBody = false;
    static uint32_t _uFilterDebugLevel = 0;
    int32_t iCheckStr;

    // check against requests filter if it is a request
    if (!strncmp(pText, "-> req:", 7))
    {
        for (iCheckStr = 0; _strFilterReq[iCheckStr].pFilterText != NULL; iCheckStr += 1)
        {
            if ((uDebugLevel < _strFilterReq[iCheckStr].uDebugLevel) && strstr(pText, _strFilterReq[iCheckStr].pFilterText))
            {
                *pFilteredLevel = _strFilterReq[iCheckStr].uDebugLevel;
                return(1);
            }
        }
    }
    // check against response filter if it is a response
    else if (!strncmp(pText, "<- resp:", 8))
    {
        for (iCheckStr = 0; _strFilterResp[iCheckStr].pFilterText != NULL; iCheckStr += 1)
        {
            if ((uDebugLevel < _strFilterResp[iCheckStr].uDebugLevel) && strstr(pText, _strFilterResp[iCheckStr].pFilterText))
            {
                *pFilteredLevel = _strFilterResp[iCheckStr].uDebugLevel;
                return(1);
            }
        }
    }
    // see if we have a body filter to apply
    else if (_bFilteringBody == false)
    {
        for (iCheckStr = 0; _strFilterBody[iCheckStr].pFilterText != NULL; iCheckStr += 1)
        {
            if ((uDebugLevel < _strFilterBody[iCheckStr].uDebugLevel) && !strncmp(pText, _strFilterBody[iCheckStr].pFilterText, strlen(_strFilterBody[iCheckStr].pFilterText)))
            {
                _bFilteringBody = true;
                *pFilteredLevel = _uFilterDebugLevel = _strFilterBody[iCheckStr].uDebugLevel;
                return(1);
            }
        }
    }
    // apply body filter, swallowing everything up to and including the closing brace
    else
    {
        if (!strncmp(pText, "  }", 3))
        {
            _bFilteringBody = false;
        }
        *pFilteredLevel = _uFilterDebugLevel;
        return(1);
    }

    // not filtered
    return(0);
}

static void _LoggingFunction(const char8_t *pText, void *pData)
{
    StressBlazeC *pStressBlaze = (StressBlazeC *)pData;
    uint32_t uDebugLevel, uFilterLevel = (unsigned)-1;

    // snuff all blaze output at debuglevel zero
    if ((uDebugLevel = pStressBlaze->GetDebugLevel()) == 0)
    {
        return;
    }

    // check to see if this line should be filtered
    if (_FilterFunction(pText, uDebugLevel, &uFilterLevel) != 0)
    {
#if !STRESSBLAZE_FILTERDEBUG   // show what would be filtered instead of filtering
        return;
#else
        StressPrintf("%% FILTERED (%d/%d) %% ", uDebugLevel, uFilterLevel);
#endif
    }

    // if not filtered, print it out
    StressPrintf("%s", pText);
}

void GameManagerListenerC::onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList *pList)
{
    StressPrintf("stressblaze: onGameBrowserListUpdateTimeout\n");
}

void GameManagerListenerC::onGameCreated(Blaze::GameManager::Game *createdGame)
{
    StressBlazeC* blazeClient = StressBlazeC::Get();

    StressPrintf("stressblaze: onGameCreated\n");

    // if voip is enabled, configure the dummy settings
    if (VoipGetRef() != NULL)
    {
        VoipControl(VoipGetRef(), 'psiz', blazeClient->m_pConfig->iDummyVoipSubPacketSize, NULL);
        VoipControl(VoipGetRef(), 'prat', blazeClient->m_pConfig->iDummyVoipSendPeriod, NULL);
    }
}

void GameManagerListenerC::onGameDestructing(Blaze::GameManager::GameManagerAPI *gameManagerApi, Blaze::GameManager::Game *dyingGame, Blaze::GameManager::GameDestructionReason reason)
{
    StressPrintf("stressblaze: onGameDestructing\n");

    StressBlazeC* blazeClient = StressBlazeC::Get();

    // if we are destructing because we disconnected, don't go back into the connected state
    if (blazeClient->m_eState != StressBlazeC::kDisconnected)
    {
        blazeClient->m_iWaitForNextGameTime = NetTick();
        blazeClient->SetState(StressBlazeC::kConnected);
    }
    blazeClient->m_iGameStartTime = 0;

    blazeClient->m_pCallback(blazeClient->m_pCallbackData, STRESS_EVENT_DELETE, &blazeClient->m_Game);
}


void GameManagerListenerC::onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult matchmakingResult, const Blaze::GameManager::MatchmakingScenario* matchmakingScenario, Blaze::GameManager::Game *game)
{
    StressBlazeC* blazeClient = StressBlazeC::Get();

    switch(matchmakingResult)
    {
        case Blaze::GameManager::SUCCESS_CREATED_GAME:
        case Blaze::GameManager::SUCCESS_JOINED_EXISTING_GAME:
        case Blaze::GameManager::SUCCESS_JOINED_NEW_GAME:
        {
            blazeClient->SetGame(game);
            break;
        }

        case Blaze::GameManager::SESSION_TIMED_OUT:
        {
            if (!blazeClient->m_bCreatingGame)
            {
                // if we weren't creating a game last time, do so now
                blazeClient->Matchmake(true);
                blazeClient->m_iWaitForNextGameTime = 0;
            }
            else
            {
                // we only want to wait 4 seconds when there's a timeout so that clients that failed previously should be able to match up sooner
                blazeClient->m_iWaitForNextGameTime = NetTick() - blazeClient->m_pConfig->iWaitBetweenGames + 4000;
            }

            break;
        }
        case Blaze::GameManager::SESSION_CANCELED:
        case Blaze::GameManager::SESSION_TERMINATED:
        case Blaze::GameManager::SESSION_QOS_VALIDATION_FAILED:
        case Blaze::GameManager::SESSION_ERROR_GAME_SETUP_FAILED:
        {
            break;
        }
        default:
        {
            StressPrintf("stressblaze: onMatchmakingScenarioFinished - unknown matchmaking result\n");
            break;
        }
    }
}

void GameManagerListenerC::onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario* matchmakingScenario, const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList)
{
    StressPrintf("stressblaze: onMatchmakingScenarioAsyncStatus\n");
}

void GameManagerListenerC::onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList *gameBrowserList, const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *removedGames, const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *updatedGames)
{
    StressPrintf("stressblaze: onGameBrowserListUpdated\n");
}

void GameManagerListenerC::onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *gameBrowserList)
{
    StressPrintf("stressblaze: onGameBrowserListDestroy\n");
}

void GameManagerListenerC::onUserGroupJoinGame(Blaze::GameManager::Game *game, Blaze::GameManager::Player* player, Blaze::GameManager::UserGroupId)
{
    StressPrintf("stressblaze: onUserGroupJoinGame\n");
}

void StressBlazeGameListenerC::GameCreated(Blaze::BlazeError err, Blaze::JobId jobId, Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: GameCreated\n");
    StressPrintf("stressblaze: Blaze::BlazeError err is %d\n", err);
}

void StressBlazeGameListenerC::GameReset(Blaze::BlazeError err, Blaze::GameManager::Game* game)
{
    StressPrintf("stressblaze: GameReset\n");
    StressPrintf("stressblaze: Blaze::BlazeError err is %d\n", err);
}

void StressBlazeGameListenerC::GameEnded(Blaze::BlazeError err, Blaze::GameManager::Game* game)
{
    StressPrintf("stressblaze: GameEnded\n");
    StressPrintf("stressblaze: Blaze::BlazeError err is %d\n", err);
}

void StressBlazeGameListenerC::InitGameComplete(Blaze::BlazeError err, Blaze::GameManager::Game* game)
{
    StressPrintf("stressblaze: InitGameComplete\n");
    StressPrintf("stressblaze: Blaze::BlazeError err is %d\n", err);
}

void StressBlazeGameListenerC::PlayerKicked(Blaze::BlazeError err, Blaze::GameManager::Game* game, const Blaze::GameManager::Player *)
{
    StressPrintf("stressblaze: PlayerKicked\n");
    StressPrintf("stressblaze: Blaze::BlazeError err is %d\n", err);
}

void StressBlazeGameListenerC::onPreGame(Blaze::GameManager::Game *game)
{
    StressBlazeC* blazeClient = StressBlazeC::Get();

    StressPrintf("stressblaze: onPreGame\n");

    // in a dedicated server game start game gets fired elsewhere and in voip testing mode we stay in preGame
    if ((blazeClient->m_pConfig->uGameMode != 1) && (blazeClient->m_pConfig->uGameMode != 2))
    {
        // start the game right away since we have already confirmed all the users are in the game
        if (game->getLocalPlayer()->isAdmin())
        {
            game->startGame(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
        }
    }
    else
    {
        StressPrintf("stressblaze: startGame transition skipped/post-poned from preGame based on game mode configuration\n");
    }
}

void StressBlazeGameListenerC::onGameStarted(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onGameStarted, I %s the game admin.\n", game->getLocalPlayer()->isAdmin() ? "am" : "am not");

    StressBlazeC* blazeClient = StressBlazeC::Get();

    if (blazeClient->m_eState == StressBlazeC::kGameStartPending)
    {
        // we're the host
        blazeClient->SetState(StressBlazeC::kHostingGame);
    }
    else
    {
        blazeClient->SetState(StressBlazeC::kGameStarted);
    }

    blazeClient->m_Game.iCount = game->getPlayerCount();

    blazeClient->m_iGameStartTime = NetTick();

    blazeClient->m_pCallback(blazeClient->m_pCallbackData, STRESS_EVENT_PLAY, &blazeClient->m_Game);
}

void StressBlazeGameListenerC::onGameEnded(Blaze::GameManager::Game *game)
{
    StressPrintf(("stressblaze: onGameEnded\n"));
    StressBlazeC* blazeClient = StressBlazeC::Get();

    blazeClient->m_Game.iCount = 0; // the game just ended the player count is now 0
    blazeClient->m_iGameStartTime = 0;

    blazeClient->m_BlazeGame->leaveGame(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
    blazeClient->m_pCallback(blazeClient->m_pCallbackData, STRESS_EVENT_END, &blazeClient->m_Game);
}

void StressBlazeGameListenerC::onGameReset(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onGameReset\n");
}

void StressBlazeGameListenerC::onReturnDServerToPool(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onReturnDServerToPool\n");
}

void StressBlazeGameListenerC::onGameAttributeUpdated(Blaze::GameManager::Game *game, const Blaze::Collections::AttributeMap *changedAttributeMap)
{
    StressPrintf("stressblaze: onGameAttributeUpdated\n");
}

void StressBlazeGameListenerC::onPlayerTeamUpdated(Blaze::GameManager::Player *player, Blaze::GameManager::TeamIndex previousTeamIndex)
{
    StressPrintf("gameserverblaze: onPlayerTeamUpdated\n");
}

void StressBlazeGameListenerC::onGameSettingUpdated(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onGameSettingUpdated\n");
}

void StressBlazeGameListenerC::onPlayerCapacityUpdated(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onPlayerCapacityUpdated\n");
}

void StressBlazeGameListenerC::onPlayerJoining(Blaze::GameManager::Player *player)
{
    StressPrintf("stressblaze: onPlayerJoining\n");
}

void StressBlazeGameListenerC::onPlayerJoiningQueue(Blaze::GameManager::Player *player)
{
    StressPrintf("stressblaze: onPlayerJoiningQueue\n");
}

void StressBlazeGameListenerC::onPlayerJoinedFromQueue(Blaze::GameManager::Player *player)
{
    StressPrintf("stressblaze: onPlayerJoinedFromQueue\n");
}

void StressBlazeGameListenerC::onPlayerJoinComplete(Blaze::GameManager::Player *player)
{
    StressPrintf("stressblaze: onPlayerJoinComplete [%02d] %s\n", player->getSlotId(), player->getName());

    if (player->isLocal() && player->isAdmin())
    {
        StressBlazeC::Get()->SetState(StressBlazeC::kWaitingForPlayers);
        StressPrintf("stressblaze: game host now waiting for all other players to join.\n");
        StressBlazeC::Get()->m_uPlayerJoinedCount++;
        StressPrintf("stressblaze: game host is now aware of %d joined players out of total %u\n", StressBlazeC::Get()->m_uPlayerJoinedCount, StressBlazeC::Get()->m_uNbOfPlayersToJoin);
    }
}

void StressBlazeGameListenerC::onQueueChanged(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onQueueChanged\n");
}

void StressBlazeGameListenerC::onPlayerRemoved(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *removedPlayer, Blaze::GameManager::PlayerRemovedReason playerRemovedReason, Blaze::GameManager::PlayerRemovedTitleContext titleContext)
{
    StressPrintf("stressblaze: onPlayerRemoved [%02d] %s (reason=%s)\n", removedPlayer->getSlotId(), removedPlayer->getName(), Blaze::GameManager::PlayerRemovedReasonToString(playerRemovedReason));
}


void StressBlazeGameListenerC::onAdminListChanged(Blaze::GameManager::Game *game, const Blaze::GameManager::Player *adminPlayer, Blaze::GameManager::UpdateAdminListOperation  operation, Blaze::GameManager::PlayerId)
{
    StressPrintf("stressblaze: onAdminListChanged\n");
}

void StressBlazeGameListenerC::onPlayerAttributeUpdated(Blaze::GameManager::Player *player, const Blaze::Collections::AttributeMap *changedAttributeMap)
{
    StressPrintf("stressblaze: onPlayerAttributeUpdated\n");
}

void StressBlazeGameListenerC::onGameTeamIdUpdated(Blaze::GameManager::Game *game, Blaze::GameManager::TeamId oldTeamId, Blaze::GameManager::TeamId newTeamId)
{
    StressPrintf("stressblaze: onGameTeamIdUpdated\n");
}

void StressBlazeGameListenerC::onPlayerCustomDataUpdated(Blaze::GameManager::Player *player)
{
    StressPrintf("stressblaze: onPlayerCustomDataUpdated\n");
}


void StressBlazeGameListenerC::onHostMigrationStarted(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onHostMigrationStarted\n");
}


void StressBlazeGameListenerC::onHostMigrationEnded(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onHostMigrationEnded, I %s the game admin.\n", game->getLocalPlayer()->isAdmin() ? "am" : "am not");

    if (game->getLocalPlayer()->isAdmin())
    {
        StressBlazeC* blazeClient = StressBlazeC::Get();

        if (blazeClient->m_eState == StressBlazeC::kGameStarted)
        {
            blazeClient->SetState(StressBlazeC::kHostingGame);
        }
        else if (blazeClient->m_eState == StressBlazeC::kCreatedGame)
        {
            blazeClient->SetState(StressBlazeC::kWaitingForPlayers);
        }
    }
}

void StressBlazeGameListenerC::onUnresponsive(Blaze::GameManager::Game *game, Blaze::GameManager::GameState previousGameState)
{
    StressPrintf("stressblaze: onUnresponsive\n");
}
void StressBlazeGameListenerC::onBackFromUnresponsive(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onBackFromUnresponsive\n");
}

void StressBlazeGameListenerC::onNetworkCreated(Blaze::GameManager::Game *game)
{
    StressPrintf("stressblaze: onNetworkCreated\n");
}

void StressBlazeGameListenerC::onMessageReceived(Blaze::GameManager::Game *game, const void *buf, size_t bufSize, Blaze::BlazeId senderId, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags)
{
    StressPrintf("stressblaze: onMessageReceived\n");
}

static bool _EAAssertFailure(const char *pExpr, const char *pFilename, int32_t iLine, const char *pFunction, const char *pMsg, va_list Args)
{
    char strFmtMsg[1024];

    ds_vsnzprintf(strFmtMsg, sizeof(strFmtMsg), pMsg, Args);
    StressPrintf("%s(%d) : assert failed: '%s' in function: %s\n, message: %s", pFilename, iLine, pExpr, pFunction, strFmtMsg);

    return(false);
}

/*F********************************************************************************/
/*!
    \Function _BlazeAssertFunction

    \Description
        The assert function handler simply logs an assertion failure message

    \Input bAssertValue     - the value to assert on
    \Input *pAssertLiteral  - the string-ified assertion value
    \Input *pFile           - the file the assertion was thrown from
    \Input iLine            - the line the assertion was thrown from
    \Input *pData           - optional data pointer 

    \Version 09/20/2017 (eesponda)
*/
/********************************************************************************F*/
static void _BlazeAssertFunction(bool bAssertValue, const char *pAssertLiteral, const char *pFile, size_t uLine, void *pData)
{
    if (!bAssertValue)
    {
        StressPrintf("%s(%u): blaze assert failed: '%s'\n", pFile, (unsigned)uLine, pAssertLiteral);
    }
}

StressBlazeC::StressBlazeC()
: m_pBlazeHub(NULL),
  m_pConnApiAdapter(NULL),
  m_eState(kDisconnected),
  m_pConfig(NULL),
  m_uNbOfPlayersToJoin(0),
  m_uPlayerJoinedCount(0),
  m_bCreatingGame(false)
{
    m_Game.iCount = 0;
    m_iLastConnectionChangeTime = NetTick();
    // override assert callback (we do not want our stress clients intentionally crashing themselves)
    EA::Assert::SetFailureCallback(&_EAAssertFailure);
    Blaze::Debug::SetAssertHook(&_BlazeAssertFunction, NULL);
    // disable Blaze timestamp logging
    Blaze::Debug::disableBlazeLogTimeStamp();
}

static void _OnRedirectorMessages(Blaze::BlazeError err, const Blaze::Redirector::DisplayMessageList* msgs)
{
    Blaze::Redirector::DisplayMessageList::const_iterator it;

    NetPrintf(("stressblaze: OnRedirectorMessages begin\n"));

    for(it = msgs->begin(); it != msgs->end(); it++)
    {
        StressPrintf("%s\n", it->c_str());
    }

    NetPrintf(("stressblaze: OnRedirectorMessages end\n"));
}


void StressBlazeC::CloseGameToAllJoinsCb(Blaze::BlazeError error, Blaze::GameManager::Game * game)
{
    if (error == Blaze::ERR_OK)
    {
        StressBlazeC::Get()->m_uNbOfPlayersToJoin =  game->getActivePlayerCount();
        StressPrintf("stressblaze: CloseGameToAllJoins m_uNbOfPlayersToJoin=%u, m_uPlayerJoinedCount=%u, res=%s\n", StressBlazeC::Get()->m_uNbOfPlayersToJoin, StressBlazeC::Get()->m_uPlayerJoinedCount, (StressBlazeC::Get()->m_uNbOfPlayersToJoin == (StressBlazeC::Get()->m_uPlayerJoinedCount)) ? "true" : "false");
        if (StressBlazeC::Get()->m_uNbOfPlayersToJoin == (StressBlazeC::Get()->m_uPlayerJoinedCount)) 
        {
            StressPrintf("stressblaze: CloseGameToAllJoins procedure succeeded - starting game right away as all %d registered players already completed joining\n", m_uNbOfPlayersToJoin);
            StressBlazeC::Get()->SetState(StressBlazeC::kGameStartPending);

            // if we are in a dedicated server game, it has already transitioned to preGame automatically by the host.  when we reach this point we just need to start the game.
            if (StressBlazeC::Get()->m_pConfig->uGameMode == 1)
            {
                game->startGame(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
            }
            // for the other modes we will transition the game into preGame as we are the host/admin
            else
            {
                game->initGameComplete(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
            }
        }
        else
        {
            StressPrintf("stressblaze: CloseGameToAllJoins procedure succeeded - Game will start when all %d registered players complete joining\n", m_uNbOfPlayersToJoin);
            StressBlazeC::Get()->SetState(StressBlazeC::kWaitingForJoinCompletions);
            StressBlazeC::Get()->m_iWaitForJoinCompletion = NetTick();
        }
    }
    else
    {
        StressPrintf("gameserverblaze: WARNING ! CloseGameToAllJoins procedure failed.\n");

        if (game->getLocalPlayer()->isAdmin())
        {
            game->endGame(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
        }
    }
}

void StressBlazeC::StressBlazeUpdate()
{
    m_pBlazeHub->idle();

    StressBlazeC* blazeClient = StressBlazeC::Get();

    if ((blazeClient->m_eState == StressBlazeC::kDisconnected) &&
        (NetTickDiff(NetTick(), blazeClient->m_iLastConnectionChangeTime) > 30000))
    {
        StressPrintf("stressblaze: attempting to reconnect to Blaze\n");

        m_pBlazeHub->getConnectionManager()->connect(&_OnRedirectorMessages);
        blazeClient->m_iLastConnectionChangeTime = NetTick();
    }

    if (blazeClient->m_eState == StressBlazeC::kFailed)
    {
        StressPrintf("stressblaze: leaving failed game\n");
        blazeClient->m_iGameStartTime = 0;
        blazeClient->SetState(StressBlazeC::kConnected);
        blazeClient->m_uNbOfPlayersToJoin = 0;
        blazeClient->m_uPlayerJoinedCount = 0;
        blazeClient->m_BlazeGame->leaveGame(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
    }
    
    if ((blazeClient->m_eState == StressBlazeC::kWaitingForQoS) && (m_pBlazeHub->getConnectionManager()->initialQosPingSiteLatencyRetrieved()))
    {
        StressPrintf("stressblaze: qos complete; moving to matchmaking state\n");
        blazeClient->SetState(StressBlazeC::kConnected);
        blazeClient->m_iWaitForNextGameTime = NetTick() - blazeClient->m_pConfig->iWaitBetweenGames;
    }

    if ((blazeClient->m_iWaitForNextGameTime != 0) && (NetTickDiff(NetTick(), blazeClient->m_iWaitForNextGameTime) >= blazeClient->m_pConfig->iWaitBetweenGames))
    {
        if ((blazeClient->m_pConfig->iMaxGameCount == 0) || (blazeClient->m_iGameCount < blazeClient->m_pConfig->iMaxGameCount))
        {
            StressPrintf("stressblaze: delay between games reached, attempting to matchmake, game(%d)\n", blazeClient->m_iGameCount);
            blazeClient->m_iWaitForNextGameTime = 0;
            if ((blazeClient->m_pConfig->uStressIndex % blazeClient->m_pConfig->uMaxPlayers) == 0)
            {
                StressPrintf("stressblaze: electing to be host; creating game\n");
                blazeClient->Matchmake(true);
            }
            else
            {
                StressPrintf("stressblaze: electing to be client; looking for game\n");
                blazeClient->Matchmake(false);
            }
            blazeClient->m_iGameCount++;
        }
        else
        {
            StressPrintf("stressblaze: max games (%d) reached disabling future games.\n", blazeClient->m_iGameCount);
            blazeClient->m_iWaitForNextGameTime = 0;
            if (StressBlazeC::Get()->m_pConfig->eMaxGameCountAction == kDisableMetrics)
            {
                StressBlazeC::Get()->m_pConfig->uMetricsEnabled = FALSE; // just disable metrics, so we don't show up on the dashbaords anymore
            }
            else if (StressBlazeC::Get()->m_pConfig->eMaxGameCountAction == kExit)
            {
                StressBlazeC::Get()->m_pConfig->uSignalFlags = STRESS_SHUTDOWN_IFEMPTY; // shutdown, but once all the processes shutdown the pod will restart
            }
        }
    }

    if (blazeClient->m_eState == StressBlazeC::kWaitingForPlayers)
    {
        
        if (blazeClient->m_BlazeGame->getActivePlayerCount() < blazeClient->m_pConfig->uMinPlayers)
        {
            blazeClient->m_iWaitForPlayersTime = 0;
        }
        else
        {
            if (blazeClient->m_BlazeGame->getActivePlayerCount() == blazeClient->m_pConfig->uMaxPlayers)
            {
                StressPrintf("stressblaze: max player reached, initiating CloseGameToAllJoins procedure\n");
                blazeClient->m_iWaitForPlayersTime = 0;
                blazeClient->SetState(StressBlazeC::kGameCloseToAllJoinsPending);
                Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &StressBlazeC::CloseGameToAllJoinsCb);
                blazeClient->m_BlazeGame->closeGameToAllJoins(cb);
            }
            else if (blazeClient->m_iWaitForPlayersTime == 0)
            {
                blazeClient->m_iWaitForPlayersTime = NetTick();
            }
            else if (NetTickDiff(NetTick(), blazeClient->m_iWaitForPlayersTime) > blazeClient->m_pConfig->iWaitForPlayers)
            {
                StressPrintf("stressblaze: game joining wait period expired, initiating CloseGameToAllJoins procedure\n");
                blazeClient->m_iWaitForPlayersTime = 0;
                blazeClient->SetState(StressBlazeC::kGameCloseToAllJoinsPending);
                Blaze::GameManager::Game::ChangeGameSettingsCb cb(this, &StressBlazeC::CloseGameToAllJoinsCb);
                blazeClient->m_BlazeGame->closeGameToAllJoins(cb);
            }
        }
    }

    if ((blazeClient->m_eState == StressBlazeC::kWaitingForJoinCompletions) && (NetTickDiff(NetTick(), blazeClient->m_iWaitForJoinCompletion) > WAIT_JOIN_TIMEOUT))
    {
        StressBlazeC::Get()->SetState(StressBlazeC::kGameStartPending);

        // if we are in a dedicated server game, it has already transitioned to preGame automatically by the host.  when we reach this point we just need to start the game.
        if (StressBlazeC::Get()->m_pConfig->uGameMode == 1)
        {
            StressBlazeC::Get()->m_BlazeGame->startGame(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
        }
        // for the other modes we will transition the game into preGame as we are the host/admin
        else
        {
            StressBlazeC::Get()->m_BlazeGame->initGameComplete(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
        }
    }

    // host end game that has gone on long enough
    if ((blazeClient->m_iGameStartTime != 0) && (blazeClient->m_eState == StressBlazeC::kHostingGame) && (NetTickDiff(NetTick(), blazeClient->m_iGameStartTime) > blazeClient->m_pConfig->iGameLength))
    {
        StressPrintf("stressblaze: ending game due to desired game length being reached\n");
        blazeClient->m_iGameStartTime = 0;
        blazeClient->SetState(StressBlazeC::kConnected);
        blazeClient->m_uNbOfPlayersToJoin = 0;
        blazeClient->m_uPlayerJoinedCount = 0;
        blazeClient->m_BlazeGame->endGame(Blaze::GameManager::Game::ChangeGameStateCb(NULL));
    }

    // everyone leave game that has gone on too long, 100 seconds longer than planned
    if ((blazeClient->m_iGameStartTime != 0)&& (NetTickDiff(NetTick(), blazeClient->m_iGameStartTime) > (blazeClient->m_pConfig->iGameLength + (100 * 1000))))
    {
        StressPrintf("stressblaze: setting game to failed state, because it has gone on longer than intended.\n");
        SetFailedState();
    }
}

void StressBlazeC::LoginToBlaze()
{
    Blaze::BlazeHub::InitParameters initParams;

    ds_strnzcpy(initParams.ServiceName, m_pConfig->strService, sizeof(initParams.ServiceName));
    ds_strnzcpy(initParams.AdditionalNetConnParams, "", sizeof(initParams.AdditionalNetConnParams));
    ds_strnzcpy(initParams.ClientName, "Dirtycast Stress Client", sizeof(initParams.ClientName));
    ds_strnzcpy(initParams.ClientVersion, "15.1.1.2.0", sizeof(initParams.ClientVersion));
    ds_strnzcpy(initParams.ClientSkuId, "SKU", sizeof(initParams.ClientSkuId));
    if (m_pConfig->strRdirName[0] != '\0')
    {
        StressPrintf("stressblaze: overriding redirector name to %s\n", m_pConfig->strRdirName);
        ds_strnzcpy(initParams.Override.RedirectorAddress, m_pConfig->strRdirName, sizeof(initParams.Override.RedirectorAddress));
    }

    initParams.Secure                   = m_pConfig->bUseSecureBlazeConn;
    initParams.UserCount                = 1;
    initParams.MaxCachedUserCount       = 0;
    initParams.MaxCachedUserLookups     = 0;
    initParams.OutgoingBufferSize       = 262144;
    initParams.MaxIncomingPacketSize    = 262144;
    initParams.Locale                   = 'enUS';
    initParams.Client                   = Blaze::CLIENT_TYPE_GAMEPLAY_USER;
    initParams.GamePort                 = m_pConfig->uGamePort;

    StressPrintf("stressblaze: gameport=%d\n",initParams.GamePort);
    StressPrintf("stressblaze: m_pConfig->strEnvironment is %s\n", m_pConfig->strEnvironment);

    if (ds_stristr(m_pConfig->strEnvironment, "prod"))
    {
        StressPrintf("stressblaze: using the prod env\n");
        initParams.Environment = Blaze::ENVIRONMENT_PROD;
    }
    else if (ds_stristr(m_pConfig->strEnvironment, "test"))
    {
        StressPrintf("stressblaze: using the stest env\n");
        initParams.Environment = Blaze::ENVIRONMENT_STEST;
    }
    else if (ds_stristr(m_pConfig->strEnvironment, "cert"))
    {
        StressPrintf("stressblaze: using the scert env\n");
        initParams.Environment = Blaze::ENVIRONMENT_SCERT;
    }
    else
    {
        StressPrintf("stressblaze: using the sdev env\n");
        initParams.Environment = Blaze::ENVIRONMENT_SDEV;
    }

    //Finally we can Initialize Blaze   first NULL is a pointer to something
    Blaze::BlazeError err = Blaze::BlazeHub::initialize(&m_pBlazeHub, initParams, StressBlazeAllocatorC::Get(), _LoggingFunction, this);

    if( err != Blaze::ERR_OK)
    {
        StressPrintf("stressblaze: initialization failed\n");
        // TODO: Does this leak the allocator or any of the params above?
    }
    else
    {
        StressPrintf("stressblaze: initialization succeeded\n");

        SetState(kConnecting);

        m_pBlazeHub->addUserStateEventHandler(GetStateListener());

        m_pBlazeHub->getConnectionManager()->setClientPlatformTypeOverride(GetClientPlatformType());
        m_pBlazeHub->getConnectionManager()->connect(&_OnRedirectorMessages);

        Blaze::BlazeNetworkAdapter::ConnApiAdapterConfig config;
        config.mEnableDemangler = FALSE;
        config.mPacketSize = NETGAME_DATAPKT_MAXSIZE;
        if (m_pConfig->uGameMode == 2)
        { 
            config.mEnableVoip = true;
            config.mMaxNumTunnels = VOIP_MAXCONNECT;
        }
        else
        {
            config.mEnableVoip = false;
        }
        #if BLAZE_SDK_VERSION < BLAZE_SDK_MAKE_VERSION(15, 1, 1, 8, 0)
        config.mMaxNumEndpoints = 30; 
        #endif
        config.mMaxNumVoipPeers = VOIP_MAXCONNECT;
        m_pConnApiAdapter = new Blaze::BlazeNetworkAdapter::ConnApiAdapter(config);

        Blaze::GameManager::GameManagerAPI::GameManagerApiParams gameManagerParams(m_pConnApiAdapter);
        if (strcmp(m_pConfig->strGameProtocolVersion, "") == 0)
        {
            gameManagerParams.mGameProtocolVersionString = Blaze::GameManager::GAME_PROTOCOL_VERSION_MATCH_ANY;
        }
        else
        {
            gameManagerParams.mGameProtocolVersionString = m_pConfig->strGameProtocolVersion;
        }

        Blaze::GameManager::GameManagerAPI::createAPI(*m_pBlazeHub, gameManagerParams);

        m_pBlazeHub->getGameManagerAPI()->addListener(GetGameManagerListener());
    }
}

void StressBlazeC::StartMatchmakingScenarioCb(Blaze::BlazeError err, Blaze::JobId jobId, Blaze::GameManager::MatchmakingScenario *matchmakingScenario, const char8_t *errDetails)
{
    StressPrintf("stressblaze: onMatchmakingSessionStarted (err=0x%08x/%s)\n", err, m_pBlazeHub->getErrorName(err));
}

void StressBlazeC::Matchmake(bool bCreate)
{ 
    StressPrintf("stressblaze: matchmaking\n");

    // Setup the matchmaking parameters
    Blaze::GameManager::ScenarioAttributes Attributes;

    m_bCreatingGame = bCreate;

    if (EA::TDF::GenericTdfType *pMaxPlayers = Attributes.allocate_element())
    {
        pMaxPlayers->set(StressBlazeC::Get()->m_pConfig->uMaxPlayers);
        Attributes["PLAYER_CAPACITY"] = pMaxPlayers;
    }
    if (EA::TDF::GenericTdfType *pMinPlayers = Attributes.allocate_element())
    {
        pMinPlayers->set(StressBlazeC::Get()->m_pConfig->uMinPlayers);
        Attributes["MIN_PLAYER_COUNT"] = pMinPlayers;
    }
    if (EA::TDF::GenericTdfType *pDesiredPlayers = Attributes.allocate_element())
    {
        pDesiredPlayers->set(StressBlazeC::Get()->m_pConfig->uMaxPlayers);
        Attributes["DESIRED_PLAYER_COUNT"] = pDesiredPlayers;
    }

    if (EA::TDF::GenericTdfType *pGameName = Attributes.allocate_element())
    {
        pGameName->set(StressBlazeC::Get()->m_pConfig->strPersona);
        Attributes["GAME_NAME"] = pGameName;
    }

    if (EA::TDF::GenericTdfType *pNetworkTopology = Attributes.allocate_element())
    {
        switch (StressBlazeC::Get()->m_pConfig->uGameMode)
        {
        case 1:
            pNetworkTopology->set(Blaze::CLIENT_SERVER_DEDICATED);
            break;
        case 2:
            pNetworkTopology->set(Blaze::NETWORK_DISABLED);
            break;
        case 3:
            pNetworkTopology->set(Blaze::PEER_TO_PEER_FULL_MESH);
            break;
        case 4:
            pNetworkTopology->set(Blaze::CLIENT_SERVER_PEER_HOSTED);
            break;
        default:
            pNetworkTopology->set(Blaze::CLIENT_SERVER_DEDICATED);
            break;
        }
        Attributes["NETWORK_TOPOLOGY"] = pNetworkTopology;
    }

    if (EA::TDF::GenericTdfType *pVoipNetwork = Attributes.allocate_element())
    {
        switch (StressBlazeC::Get()->m_pConfig->uGameMode)
        {
        case 2:
            pVoipNetwork->set(Blaze::VOIP_PEER_TO_PEER);
            break;
        default:
            pVoipNetwork->set(Blaze::VOIP_DISABLED);
            break;
        }
        Attributes["VOIP_NETWORK"] = pVoipNetwork;
    }

    StressPrintf("stressblaze: initiating matchmaking using '%s' scenario\n", StressBlazeC::Get()->m_pConfig->strMatchmakingScenario);

    // Start the matchmaking session
    m_pBlazeHub->getGameManagerAPI()->startMatchmakingScenario(StressBlazeC::Get()->m_pConfig->strMatchmakingScenario, Attributes, Blaze::GameManager::PlayerJoinDataHelper(), Blaze::GameManager::GameManagerAPI::StartMatchmakingScenarioCb(this, &StressBlazeC::StartMatchmakingScenarioCb));
}

void StressBlazeC::SetGame(Blaze::GameManager::Game *game)
{
    SetState(StressBlazeC::kCreatedGame);
    m_BlazeGame = game;
    m_pCallback(m_pCallbackData, STRESS_EVENT_GAME, &m_Game);
    game->addListener(GetGameListener());
}

Blaze::GameManager::Game *StressBlazeC::GetGame()
{
    return(m_BlazeGame);
}

Blaze::BlazeNetworkAdapter::ConnApiAdapter *StressBlazeC::GetConnApiAdapter()
{
    return(m_pConnApiAdapter);
}

StressBlazeC *StressBlazeC::Get()
{
    static StressBlazeC g_StressBlaze;

    return(&g_StressBlaze);
}

StressBlazeStateListenerC *StressBlazeC::GetStateListener()
{
    static StressBlazeStateListenerC g_StressBlazeStateListener;

    return(&g_StressBlazeStateListener);
}

StressBlazeGameListenerC *StressBlazeC::GetGameListener()
{
    static StressBlazeGameListenerC g_StressBlazeGameListener;

    return(&g_StressBlazeGameListener);
}

GameManagerListenerC *StressBlazeC::GetGameManagerListener()
{
    static GameManagerListenerC g_GameManagerListener;

    return(&g_GameManagerListener);
}

void StressBlazeC::SetConfig(StressConfigC *config)
{
    m_pConfig = config;
}

void StressBlazeC::SetCallback(StressCallbackT* pCallback, void* pCallbackData)
{
    m_pCallback = pCallback;
    m_pCallbackData = pCallbackData;

}

uint32_t StressBlazeC::GetPlayerCount() 
{ 
    return(m_Game.iCount);
}

uint32_t StressBlazeC::GetDebugLevel(void)
{
    if (m_pConfig)
    {
        return(m_pConfig->uDebugLevel);
    }

    return(0);
}

StressBlazeC::StressBlazeStateE StressBlazeC::GetState()
{
    return(m_eState);
}


void StressBlazeC::SetFailedState()
{
    SetState(StressBlazeC::kFailed);
}

const char * StressBlazeC::GetStateText(StressBlazeStateE eState)
{
    static const char *_strStressBlazeStateText[] =
    {
        "kDisconnected",
        "kConnecting",
        "kWaitingForQoS",
        "kConnected",
        "kCreatedGame",
        "kWaitingForPlayers",
        "kGameCloseToAllJoinsPending",
        "kWaitingForJoinCompletions",
        "kGameStartPending",
        "kGameStarted",
        "kHostingGame",
        "kFailed"
    };

    return(_strStressBlazeStateText[eState]);
}


void StressBlazeC::SetState(StressBlazeStateE eState)
{
    StressPrintf("stressblaze: changing state %s -> %s\n", GetStateText(m_eState), GetStateText(eState));
    m_eState = eState;
}


/*F********************************************************************************/
/*!
    \Function StressBlazeC::GetClientPlatformType

    \Description
        Extracts a client platform type (ps4, xone, etc) from a blaze service name.

    \Output
        Blaze::ClientPlatformType - Extracted client platform type 

    \Version 03/21/2020 (mclouatre)

    \Notes
        Assumptions about the format of the blaze service name
         --> dashes are used as field delimiter
         --> the client platform type is always located after the second dash
         --> the client platform type may be followed by a dash or by the NULL-termination char
            examples: battlefield-1-pc-trial, fifa-2020-ps4, madden-2019-xone-api, dirtycast-testbed-ps4
*/
/********************************************************************************F*/
Blaze::ClientPlatformType StressBlazeC::GetClientPlatformType()
{
    eastl::string strBlazeServiceName(m_pConfig->strService);

    // find position of second dash in blaze service name (it is not possible there is no such character)
    eastl_size_t firstDashPosition = strBlazeServiceName.find_first_of("-", 0);
    eastl_size_t secondDashPosition = strBlazeServiceName.find_first_of("-", firstDashPosition + 1);

    // find position of third dash in blaze service name (it is possible there is no such character)
    eastl_size_t thirdDashPosition = strBlazeServiceName.find_first_of("-", secondDashPosition + 1);

    // if third dash not found, then use the position of the NULL-termination char
    if (thirdDashPosition == eastl::string::npos)
    {
        thirdDashPosition = strBlazeServiceName.length();
    }

    // extract client platform type as a substring located between second dash and third dash of the blaze service name
    eastl::string strClientPlatformType = strBlazeServiceName.substr(secondDashPosition + 1, (thirdDashPosition - secondDashPosition - 1));

    // convert client platform type string to the matching blaze-specific value
    Blaze::ClientPlatformType blazeClientPlatfromType = Blaze::INVALID;
    Blaze::ParseClientPlatformType(strClientPlatformType.c_str(), blazeClientPlatfromType);

    return(blazeClientPlatfromType);
}
