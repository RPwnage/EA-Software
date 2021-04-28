/*H********************************************************************************/
/*!
    \File    gameserverblaze.cpp

    \Description
        This module implements the interactions with the BlazeSDK and manages our Blaze
        game, blazeserver connection

    \Copyright
        Copyright (c) Electronic Arts 2017. ALL RIGHTS RESERVED.
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include "DirtySDK/platform.h"

#if defined(DIRTYCODE_LINUX)
#include <sys/time.h>
#endif

#include "BlazeSDK/version.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/blazestateprovider.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/player.h"
#include "BlazeSDK/gamemanager/playerjoindatahelper.h"
#include "BlazeSDK/loginmanager/loginmanager.h"

#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "DirtySDK/game/netgameerr.h"

#include "libsample/zfile.h"
#include "libsample/zmem.h"

#include "EAAssert/eaassert.h"

#include "dirtycast.h"
#include "gameserverconfig.h"
#include "gameservernetworkadapter.h"
#include "gameserverpkt.h"
#include "gameserverblaze.h"
#include "gameserverdist.h"
#include "subspaceapi.h"

/*** Defines **********************************************************************/

// default connection default to the server
#define DEFAULT_CONNECT_DELAY       (60000)

// memory id used for allocations
#define GAMESERVERBLAZE_MEMID       ('gsbb')
// memid for eastl
#define EASTL_MEMID                 ('east')

// default verbosity level for gameserverblaze debug logging
#define GAMESERVERBLAZE_DEBUGLVL    (2)
// number of reset attempts before we are stuck
#define MAX_RESET_ATTEMPT           (5)

// custom errors for logging and metrics
#define GAMESERVERBLAZE_ERROR_STUCK         ((unsigned)-2)
#define GAMESERVERBLAZE_ERROR_ZOMBIE        ((unsigned)-3)

// define noexcept on platforms that require it
#if defined(DIRTYCODE_LINUX) || (defined(DIRTYCODE_PC) && (_MSC_VER >= 1900))
    #define NOEXCEPT noexcept
#else
    #define NOEXCEPT
#endif

/*** Type Definitions *************************************************************/

// our version of the core allocator
struct GameServerBlazeAllocatorT: public EA::Allocator::ICoreAllocator
{
    GameServerBlazeAllocatorT();

    void *Alloc(size_t uSize, const char *pName, unsigned int uFlags) override;
    void *Alloc(size_t uSize, const char *pName, unsigned int uFlags, unsigned int uAlign, unsigned int uAlignOffset = 0) override;
    void Free(void *pBlock, size_t uSize = 0) override;

    int32_t m_iMemgroup;
    uint8_t m_bMemgroupInited;
};

// implementation of the BlazeStateEventHandler and LoginManagerListener
class GameServerStateEventHandlerC : public Blaze::BlazeStateEventHandler, public Blaze::LoginManager::LoginManagerListener
{
public:
    // From BlazeStateEventHandler
    virtual void onConnected() override;
    virtual void onDisconnected(Blaze::BlazeError iErrorCode) override;
    virtual void onAuthenticated(uint32_t uUserIndex) override;
    virtual void onDeAuthenticated(uint32_t uUserIndex) override;

    // From LoginManagerListener
    virtual void onLoginFailure(Blaze::BlazeError iErrorCode, const char8_t *pPortalUrl) override;
    virtual void onSdkError(Blaze::BlazeError iErrorCode) override;
};

// implementation of the Blaze::GameManager::GameManagerAPIListener
class GameManagerListenerC: public Blaze::GameManager::GameManagerAPIListener
{
public:
    virtual void onGameCreated(Blaze::GameManager::Game *pCreatedGame) override;
    virtual void onGameDestructing(Blaze::GameManager::GameManagerAPI *pGameManagerApi, Blaze::GameManager::Game *pDyingGame, Blaze::GameManager::GameDestructionReason eReason) override;
    virtual void onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult eMatchmakingResult, const Blaze::GameManager::MatchmakingScenario *pMatchmakingScenario, Blaze::GameManager::Game *pGame) override;
    virtual void onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario *pMatchmakingScenario, const Blaze::GameManager::MatchmakingAsyncStatusList *pMatchmakingAsyncStatusList) override;
    virtual void onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList *pGameBrowserList, const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *pRemovedGames, const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *pUpdatedGames) override;
    virtual void onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList *pGameBrowserList) override;
    virtual void onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *pGameBrowserList) override;
    virtual void onUserGroupJoinGame(Blaze::GameManager::Game *pGame, Blaze::GameManager::Player *pPlayer, Blaze::GameManager::UserGroupId GroupId) override;
    virtual void onTopologyHostInjectionStarted(Blaze::GameManager::Game *pGame) override;
    virtual void onTopologyHostInjected(Blaze::GameManager::Game *pGame) override;

    #if BLAZE_SDK_VERSION < BLAZE_SDK_MAKE_VERSION(15, 1, 1, 8, 0)
    virtual void onMatchmakingSessionFinished(Blaze::GameManager::MatchmakingResult matchmakingResult, const Blaze::GameManager::MatchmakingSession* matchmakingSession, Blaze::GameManager::Game *game) override {}
    virtual void onMatchmakingAsyncStatus(const Blaze::GameManager::MatchmakingSession* matchmakingSession, const Blaze::GameManager::MatchmakingAsyncStatusList* matchmakingAsyncStatusList) override {}
    #endif
};

// implementation of the Blaze::GameManager::GameListener
class GameServerBlazeGameListenerC: public Blaze::GameManager::GameListener
{
public:
    GameServerBlazeGameListenerC();

    void GameCreated(Blaze::BlazeError iError, Blaze::JobId JobId, Blaze::GameManager::Game *pGame, const char *pFailedEntryCriteria);
    void GameDestroyed(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame);
    void GameReset(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame);
    void HostEjected(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame, Blaze::JobId Id);
    void GameEnded(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame);
    void InitGameComplete(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame);
    void PlayerKicked(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame, const Blaze::GameManager::Player *);

    void Idle();
    void Clear();
    bool IsStuck();
    void GameDestroyed();

    virtual void onPreGame(Blaze::GameManager::Game *pGame) override;
    virtual void onGameStarted(Blaze::GameManager::Game *pGame) override;
    virtual void onGameEnded(Blaze::GameManager::Game *pGame) override;
    virtual void onGameReset(Blaze::GameManager::Game *pGame) override;
    virtual void onReturnDServerToPool(Blaze::GameManager::Game *pGame) override;
    virtual void onGameAttributeUpdated(Blaze::GameManager::Game *pGame, const Blaze::Collections::AttributeMap *pChangedAttributeMap) override;
    virtual void onGameSettingUpdated(Blaze::GameManager::Game *pGame) override;
    virtual void onPlayerCapacityUpdated(Blaze::GameManager::Game *pGame) override;
    virtual void onPlayerJoining(Blaze::GameManager::Player *pPlayer) override;
    virtual void onPlayerJoiningQueue(Blaze::GameManager::Player *pPlayer) override;
    virtual void onPlayerJoinComplete(Blaze::GameManager::Player *pPlayer) override;
    virtual void onQueueChanged(Blaze::GameManager::Game *pGame) override;
    virtual void onPlayerRemoved(Blaze::GameManager::Game *pGame, const Blaze::GameManager::Player *pRemovedPlayer, Blaze::GameManager::PlayerRemovedReason ePlayerRemovedReason, Blaze::GameManager::PlayerRemovedTitleContext uTitleContext) override;
    virtual void onAdminListChanged(Blaze::GameManager::Game *pGame, const Blaze::GameManager::Player *pAdminPlayer, Blaze::GameManager::UpdateAdminListOperation  eOperation, Blaze::GameManager::PlayerId Pid) override;
    virtual void onPlayerJoinedFromQueue(Blaze::GameManager::Player *pPlayer) override;
    virtual void onPlayerJoinedFromReservation(Blaze::GameManager::Player *pPlayer) override;
    virtual void onPlayerAttributeUpdated(Blaze::GameManager::Player *pPlayer, const Blaze::Collections::AttributeMap *pChangedAttributeMap) override;
    virtual void onPlayerTeamUpdated(Blaze::GameManager::Player *pPlayer, Blaze::GameManager::TeamIndex previousTeamIndex) override;
    virtual void onGameTeamIdUpdated(Blaze::GameManager::Game *pGame, Blaze::GameManager::TeamId uOldTeamId, Blaze::GameManager::TeamId uNewTeamId) override;
    virtual void onPlayerCustomDataUpdated(Blaze::GameManager::Player *pPlayer) override;
    virtual void onPlayerStateChanged(Blaze::GameManager::Player *pPlayer, Blaze::GameManager::PlayerState ePreviousPlayerState) override;
    virtual void onHostMigrationStarted(Blaze::GameManager::Game *pGame) override;
    virtual void onHostMigrationEnded(Blaze::GameManager::Game *pGame) override;
    virtual void onNetworkCreated(Blaze::GameManager::Game *pGame) override;
    virtual void onMessageReceived(Blaze::GameManager::Game *pGame, const void *pBuf, size_t uBufSize, Blaze::BlazeId SenderId, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset MessageFlags) override {}
    virtual void onUnresponsive(Blaze::GameManager::Game *pGame, Blaze::GameManager::GameState ePreviousGameState) override;
    virtual void onBackFromUnresponsive(Blaze::GameManager::Game *pGame) override;

private:
    bool m_bNeedsReset;
    bool m_bNeedsIdleReset;
    Blaze::GameManager::Game *m_pIdleGame;
    uint32_t m_uLastIdle;
    int32_t m_iIdleResetCount;
};

// module state
struct GameServerBlazeT
{
    Blaze::BlazeHub *pBlazeHub;         //!< pointer to blazehub, used for interacting with the blazesdk
    Blaze::GameManager::Game *pGame;    //!< pointer to blaze game, used for game operations
    GameServerConfigT *pConfig;         //!< gameserver configuration
    GameServerBlazeStateE eState;       //!< state of the module in terms of connection and game
    GameServerBlazeCallbackT *pCallback;//!< callback pointer
    void *pCallbackData;                //!< data passed along with callback
    GameServerGameT Game;               //!< game information
    GameServerGameT LastGame;           //!< cached information about last game
    uint32_t uLastConnectionErrorKind;  //!< cached connection error kind
    uint32_t uLastConnectionErrorCode;  //!< cached connection error code
    uint32_t uLastConnectionChangeTime; //!< cached last connection state change time
    int32_t iConnectDelay;              //!< delay to connect to the server
    uint16_t uLastRemovedReason;        //!< cached reason for removing a player
    int16_t iLastKickReason;            //!< cached reason for kicking a player
    uint16_t uLastGameState;            //!< cached game state
    uint8_t bInjected;                  //!< were we injected into a game
    uint8_t bZombie;                    //!< are we trying to achieve zombie state?
    char strAccessToken[8192];          //!< access token used for logging into the server
    char strStatusUrl[256];             //!< url that points to our diagnostics page
    GameServerLinkCreateParamT LinkParams;//!< parameters to create gameserverlink in the network adapter
    int32_t iMemGroup;                  //!< memory allocation group
    void *pMemGroupUserData;            //!< memory allocation userdata

    GameServerBlazeNetworkAdapterC *pAdapter;           //!< network adapter
    GameServerBlazeGameListenerC *pGameListener;        //!< game listener
    GameServerStateEventHandlerC *pStateEventHandler;   //!< blaze state event handler
    GameManagerListenerC *pGameManagerListener;         //!< gamemanagerapi listener

    // subspace related
    SubspaceApiRefT *pSubspaceApi;      //!< ref for configuring subspace proxies
    uint32_t uLastSubspaceProvisionTick;//!< NetTick of the last time we provisioned with subspace
    uint32_t uLocalServerIp;            //!< cached copy of the voipservers front addr
    uint16_t uLocalServerPort;          //!< cached copy of the voipservers api port
    uint8_t bDoSubspaceProvision;       //!< now is a chance to re-provision the subspace ip/port
    uint8_t bSubspaceStartedGameDestroy;//!< have we started the async process of destroying the game, in the subspace provision procedure
    uint8_t bDoSubspaceStop;            //!< we should free our subspace resources
    uint8_t _pad[3];
};

/*** Variables ********************************************************************/

// global module state
static GameServerBlazeT *_pGameServerBlaze = NULL;

// filter for requests
static const DirtyCastFilterEntryT _strFilterReq[] =
{
    { DIRTYCAST_DBGLVL_RECURRING, "UtilComponent::ping", 0 },
    { DIRTYCAST_DBGLVL_RECURRING, "Fire2Connection::PING", 0 },
    { 0, NULL, 0 }
};
// filter for responses
static const DirtyCastFilterEntryT _strFilterResp[] =
{
    { DIRTYCAST_DBGLVL_RECURRING, "UtilComponent::ping", 0 },
    { DIRTYCAST_DBGLVL_RECURRING, "Fire2Connection::PING_REPLY", 0 },
    { 0, NULL, 0 }
};

// filter for request/response bodies
static const DirtyCastFilterEntryT _strFilterBody[] =
{
    { DIRTYCAST_DBGLVL_RECURRING, "  PingResponse = {",  0 },
    { 0, NULL, 0 }
};

// filter for misc prints
static const DirtyCastFilterEntryT _strFilterText[] =
{
    { DIRTYCAST_DBGLVL_RECURRING, "handleReceivedPing: Received a PING from the server, sending a PING_REPLY", 0 },
    { DIRTYCAST_DBGLVL_RECURRING, "checkPing: RPC count 0. Send ping.", 0 },
    { 0, NULL, 0 }
};

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _GetDebugLevel

    \Description
        Gets the debug level for logging

    \Input iError   - error code we base our debug level on (if specified)

    \Output
        uint32_t    - the debug level for logging

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static uint32_t _GetDebugLevel(Blaze::BlazeError iError = Blaze::ERR_OK)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    if (iError != Blaze::ERR_OK)
    {
        return(DIRTYCAST_DBGLVL_HIGHEST);
    }

    if (pBlazeServer->pConfig != NULL)
    {
        return(pBlazeServer->pConfig->uDebugLevel);
    }

    return(0);
}

/*F********************************************************************************/
/*!
    \Function _MoveToNextState

    \Description
        Centralizes state transition logic for gameserverblaze 

    \Input *pBlazeServer    - module state
    \Input eNewState        - new state to go to

    \Version 09/11/2020 (mclouatre)
*/
/********************************************************************************F*/
static void _MoveToNextState(GameServerBlazeT* pBlazeServer, GameServerBlazeStateE eNewState)
{
    static const char* strGameServerBlazeState[] =
    {
        "kDisconnected     ",
        "kConnecting       ",
        "kConnected        ",
        "kCreatedGame      ",
        "kWaitingForPlayers",
        "kGameStarted      "
    };

    DirtyCastLogPrintfVerbose(pBlazeServer->pConfig->uDebugLevel, 1, "gameserverblaze: state transition %s --> %s\n", strGameServerBlazeState[pBlazeServer->eState], strGameServerBlazeState[eNewState]);

    /* Whenever 'connected' is lost or 'connecting' is entered, we want to restart the timer used
       to detect if a connection to blaze should be re-attempted. */
    if ( ((pBlazeServer->eState >= kConnected) && (eNewState < kConnected)) ||
         ((pBlazeServer->eState != kConnecting) && (eNewState == kConnecting)) )
    {
        pBlazeServer->uLastConnectionChangeTime = NetTick();
    }

    pBlazeServer->eState = eNewState;
}

/*F********************************************************************************/
/*!
    \Function _ClearBlazeGame

    \Description
        Clears the blaze game is not already cleared

    \Input *pBlazeServer    - module state

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static void _ClearBlazeGame(GameServerBlazeT *pBlazeServer)
{
    GameServerGameT ClearGame;
    ds_memclr(&ClearGame, sizeof(ClearGame));

    if (memcmp(&pBlazeServer->Game, &ClearGame, sizeof(pBlazeServer->Game)) != 0)
    {
        DirtyCastLogPrintfVerbose(pBlazeServer->pConfig->uDebugLevel, 0, "gameserverblaze: clearing game state\n");
        ds_memclr(&pBlazeServer->Game, sizeof(pBlazeServer->Game));
    }
}

/*F********************************************************************************/
/*!
    \Function _AddPlayerToGame

    \Description
        Adds a player to the game

    \Input *pBlazeServer    - module state
    \Input *pName           - the player name
    \Input iSlotId          - the slot the player is put into
    \Input iId              - the player identifier

    \Output
        int32_t             - the refcount for this player

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _AddPlayerToGame(GameServerBlazeT *pBlazeServer, const char *pName, int32_t iSlotId, int32_t iId)
{
    int32_t iClient, iPlayerCount;
    GameServerClientT *pClient = &pBlazeServer->Game.aClients[iSlotId - 1];
    GameServerPlayerT *pPlayer = &pClient->aPlayers[pClient->iRefCount];

    if (pClient->iRefCount == 0)
    {
        pClient->iIdent = iId;
        pBlazeServer->Game.iCount += 1;
    }

    // find an empty player slot in the client
    ds_strnzcpy(pPlayer->strPers, pName, sizeof(pPlayer->strPers));

    // increase the ref count
    pClient->iRefCount += 1;

    // count the total number of players
    for (iClient = 0, iPlayerCount = 0; iClient < pBlazeServer->Game.iCount; iClient += 1)
    {
        iPlayerCount += pBlazeServer->Game.aClients[iClient].iRefCount;
    }

    DirtyCastLogPrintf("gameserverblaze: game has %d client(s) - %d user(s)\n", pBlazeServer->Game.iCount, iPlayerCount);
    for (iClient = 0; iClient < pBlazeServer->Game.iCount; iClient += 1)
    {
        int32_t iPlayer, iLogOffset = 0;
        char strLogBuf[256];
        GameServerClientT *pTemp = &pBlazeServer->Game.aClients[iClient];

        iLogOffset += ds_snzprintf(strLogBuf+iLogOffset, sizeof(strLogBuf)-iLogOffset, "[%2d] client=0x%08x refcount=%d user(s)=>", iClient, pTemp->iIdent, pTemp->iRefCount);

        for (iPlayer = 0; iPlayer < pClient->iRefCount; iPlayer += 1)
        {
            iLogOffset += ds_snzprintf(strLogBuf+iLogOffset, sizeof(strLogBuf)-iLogOffset, "'%s' ", pTemp->aPlayers[iPlayer].strPers);
        }

        DirtyCastLogPrintf("%s\n", strLogBuf);
    }

    return(pClient->iRefCount);
}

/*F********************************************************************************/
/*!
    \Function _RemovePlayerFromGame

    \Description
        Remove a player from the game

    \Input *pBlazeServer    - module state
    \Input iClientId        - player identifier
    \Input *pUserName       - player's name

    \Output
        int32_t             - the refcount for this player

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _RemovePlayerFromGame(GameServerBlazeT *pBlazeServer, int32_t iClientId, const char *pUserName)
{
    int iSlot;
    int iPlayer;
    int32_t iRefCount = 0;
    bool bUserFound = false;

    for (iSlot = 0; iSlot < GAMESERVER_MAX_CLIENTS; iSlot++)
    {
        if (pBlazeServer->Game.aClients[iSlot].iIdent == iClientId)
        {
            pBlazeServer->Game.aClients[iSlot].iRefCount--;

            if (pBlazeServer->Game.aClients[iSlot].iRefCount == 0)
            {
                ds_memclr(&pBlazeServer->Game.aClients[iSlot], sizeof(pBlazeServer->Game.aClients[0]));
                pBlazeServer->Game.iCount -= 1;
                bUserFound = true; // ignore the name because we are removing the entire client
            }
            else if (pBlazeServer->Game.aClients[iSlot].iRefCount < 0)
            {
                DirtyCastLogPrintf("gameserverblaze: critical error - negative ref count (%d) for client 0x%08x (slot=#%d pers=%s)\n",
                    pBlazeServer->Game.aClients[iSlot].iRefCount, pBlazeServer->Game.aClients[iSlot].iIdent, iSlot, pUserName);
            }
            else // the client's ref count is > 0, remove only the player from the client's list
            {
                for (iPlayer = 0; iPlayer < GAMESERVER_MAX_PLAYERS; iPlayer++)
                {
                    if (ds_strnicmp(pUserName, pBlazeServer->Game.aClients[iSlot].aPlayers[iPlayer].strPers, GAMESERVER_MAX_PERSONA_LENGTH) == 0)
                    {
                        ds_memclr(pBlazeServer->Game.aClients[iSlot].aPlayers[iPlayer].strPers, sizeof(pBlazeServer->Game.aClients[iSlot].aPlayers[iPlayer].strPers));
                        bUserFound = true;
                        break;
                    }
                }
            }
            iRefCount = pBlazeServer->Game.aClients[iSlot].iRefCount;
            break;
        }
    }

    if (!bUserFound)
    {
        DirtyCastLogPrintf("gameserverblaze: user %s is not associated with client 0x%08x.\n", pUserName, iClientId);
    }

    if (iSlot < GAMESERVER_MAX_CLIENTS)
    {
        DirtyCastLogPrintfVerbose(pBlazeServer->pConfig->uDebugLevel, 0, "gameserverblaze: %d clients in game\n", pBlazeServer->Game.iCount);
        for (iSlot = 0; iSlot < GAMESERVER_MAX_CLIENTS; iSlot++)
        {
            if (pBlazeServer->Game.aClients[iSlot].iIdent != 0)
            {
                for (iPlayer = 0; iPlayer < GAMESERVER_MAX_PLAYERS; iPlayer++)
                {
                    if (pBlazeServer->Game.aClients[iSlot].aPlayers[iPlayer].strPers[0] != '\0')
                    {
                        DirtyCastLogPrintfVerbose(pBlazeServer->pConfig->uDebugLevel, 0, "[%2d] client=0x%08x user=%s refcount=%d\n", iSlot, pBlazeServer->Game.aClients[iSlot].iIdent, pBlazeServer->Game.aClients[iSlot].aPlayers[iPlayer].strPers, pBlazeServer->Game.aClients[iSlot].iRefCount);
                    }
                }
            }
        }
    }
    else
    {
        DirtyCastLogPrintf("gameserverblaze: could not find player %s with client=0x%08x to remove from game\n", pUserName, iClientId);
    }

    return(iRefCount);
}

/*F********************************************************************************/
/*!
    \Function _ResetGame

    \Description
        Handle the resetting of the game

    \Input *pBlazeServer    - module state
    \Input *pGame           - game we are resetting
    \Input uGameEvent       - event that caused the reset

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static void _ResetGame(GameServerBlazeT *pBlazeServer, Blaze::GameManager::Game *pGame, uint32_t uGameEvent)
{
    _ClearBlazeGame(pBlazeServer);
    pBlazeServer->Game.iIdent = pGame->getId();
    pBlazeServer->Game.uWhen = time(NULL);
    ds_snzprintf(pBlazeServer->Game.strName, sizeof(pBlazeServer->Game.strName), "game%llu", pGame->getId());
    ds_strnzcpy(pBlazeServer->Game.strUUID, pGame->getUUID(), sizeof(pBlazeServer->Game.strUUID));
    ds_strnzcpy(pBlazeServer->Game.strGameMode, pGame->getGameMode(), sizeof(pBlazeServer->Game.strGameMode));

    if (pBlazeServer->pCallback(pBlazeServer->pCallbackData, uGameEvent, &pBlazeServer->Game) != 0)
    {
        ds_memcpy(&pBlazeServer->LastGame, &pBlazeServer->Game, sizeof(pBlazeServer->LastGame));
    }

    _MoveToNextState(pBlazeServer, kWaitingForPlayers);
}

/*F********************************************************************************/
/*!
    \Function _SubspaceGetUniqueId

    \Description
        Generate a unique id to be used with subspace
    
    \Input *strServerUniqueID   - memory to write the unique id to
    \Input uSize                - sizeof strServerUniqueID

    \Version 07/21/2020 (cvienneau)
*/
/********************************************************************************F*/
static void _SubspaceGetUniqueId(char *strServerUniqueID, uint32_t uSize)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    // the GameServerUUID will consist of the voipservers uFrontAddr:uApiPort and the GameServers uDiagnosticPort (of which there should only be 1 per host)
    ds_snzprintf(strServerUniqueID, uSize, "%a:%d_%d", pBlazeServer->uLocalServerIp, pBlazeServer->uLocalServerPort, pBlazeServer->pConfig->uDiagnosticPort);
}

/*F********************************************************************************/
/*!
    \Function _SubspaceRequestUpdate

    \Description
        Provisions a new subpsace address / port

    \Output
        int32_t        - > 0 if done, 0 in progress, negative on error

    \Version 06/30/2020 (cvienneau)
*/
/********************************************************************************F*/
static int32_t _SubspaceRequestUpdate(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    uint32_t uOldAddr, uOldPort;

    // get any pending resutls we have from the subspace api
    SubspaceApiResponseT Response = SubspaceApiResponse(pBlazeServer->pSubspaceApi);

    // first deal with any provision requests
    if (pBlazeServer->bDoSubspaceProvision == TRUE)
    {
        int8_t bSuccessfulProvisionRet = 1;
        // if we are doing a stop immediatly after this start, don't tell the caller that we are done, they should wait till stop finishes too
        if (pBlazeServer->bDoSubspaceStop)
        {
            bSuccessfulProvisionRet = 0;
        }

        if (pBlazeServer->pGame != NULL)
        {
            DirtyCastLogPrintf("gameserverblaze: subspace provision aborted, a game currently exists.\n");
            pBlazeServer->bDoSubspaceProvision = FALSE;
            return(-1);
        }

        // if we are doing nothing or already working on provision, then process provision
        if ((Response.eRequestType == SUBSPACE_REQUEST_TYPE_NONE) || (Response.eRequestType == SUBSPACE_REQUEST_TYPE_PROVISION))
        {
            // if the we are currently idle, make our request
            if (Response.eSubspaceState == SUBSPACE_STATE_IDLE)
            {
                // if we have provisioned recently, just use the values we got last time
                if ((pBlazeServer->uLastSubspaceProvisionTick != 0) &&
                    (NetTickDiff(NetTick(), pBlazeServer->uLastSubspaceProvisionTick) < (int32_t)pBlazeServer->pConfig->uSubspaceProvisionRateMax))
                {
                    int32_t uSSAddr = SubspaceApiStatus(pBlazeServer->pSubspaceApi, 'ssad', 0, NULL, 0);
                    int32_t uSSPort = SubspaceApiStatus(pBlazeServer->pSubspaceApi, 'sspo', 0, NULL, 0);
                    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_CONNINFO, "gameserverblaze: subspace provision skipped, it has already completed recently.\n");

                    if ((uSSAddr != 0) && (uSSPort != 0))
                    {
                        uOldAddr = pBlazeServer->LinkParams.uAddr;
                        uOldPort = pBlazeServer->LinkParams.uPort;
                        pBlazeServer->LinkParams.uAddr = uSSAddr;
                        pBlazeServer->LinkParams.uPort = uSSPort;

                        DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_CONNINFO, "gameserverblaze: using previous subspace address info, changing from %a:%d -> %a:%d.\n",
                            uOldAddr, uOldPort, pBlazeServer->LinkParams.uAddr, pBlazeServer->LinkParams.uPort);
                    }
                    else
                    {
                        DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_CONNINFO, "gameserverblaze: keeping current settings %a:%d.\n",
                            pBlazeServer->LinkParams.uAddr, pBlazeServer->LinkParams.uPort);
                    }

                    pBlazeServer->bDoSubspaceProvision = FALSE;
                    return(bSuccessfulProvisionRet);
                }
                else
                {
                    // start things off
                    char strServerUniqueID[128];
                    _SubspaceGetUniqueId(strServerUniqueID, sizeof(strServerUniqueID));
                    SubspaceApiRequest(pBlazeServer->pSubspaceApi, SUBSPACE_REQUEST_TYPE_PROVISION, pBlazeServer->uLocalServerIp, pBlazeServer->uLocalServerPort, strServerUniqueID);
                    return(0);
                }
            }
            else if (Response.eSubspaceState == SUBSPACE_STATE_IN_PROGRESS)
            {
                // wait
                return(0);
            }
            else if (Response.eSubspaceState == SUBSPACE_STATE_DONE)
            {
                // complete success
                pBlazeServer->uLastSubspaceProvisionTick = NetTick();
                uOldAddr = pBlazeServer->LinkParams.uAddr;
                uOldPort = pBlazeServer->LinkParams.uPort;
                pBlazeServer->LinkParams.uAddr = Response.uResultAddr;
                pBlazeServer->LinkParams.uPort = Response.uResultPort;

                DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_CONNINFO, "gameserverblaze: subspace changed address info from %a:%d -> %a:%d.\n",
                    uOldAddr, uOldPort, pBlazeServer->LinkParams.uAddr, pBlazeServer->LinkParams.uPort);
                pBlazeServer->bDoSubspaceProvision = FALSE;
                return(bSuccessfulProvisionRet);
            }
            else if (Response.eSubspaceState == SUBSPACE_STATE_ERROR)
            {
                DirtyCastLogPrintf("gameserverblaze: subspaceapi in error state.\n");
            }
            else
            {
                DirtyCastLogPrintf("gameserverblaze: unknown subspaceapi state %d.\n", Response.eSubspaceState);
            }
        }
        else
        {
            DirtyCastLogPrintf("gameserverblaze: unexpected subspace request type %d\n", Response.eRequestType);
        }

        // complete in failure case, just use our non-subspace settings but act as if we provisioned it from subspace
        pBlazeServer->uLastSubspaceProvisionTick = NetTick();
        uOldAddr = pBlazeServer->LinkParams.uAddr;
        uOldPort = pBlazeServer->LinkParams.uPort;
        pBlazeServer->LinkParams.uAddr = pBlazeServer->uLocalServerIp;
        pBlazeServer->LinkParams.uPort = pBlazeServer->uLocalServerPort;

        DirtyCastLogPrintf("gameserverblaze: subspace error, changed address info back from %a:%d -> %a:%d.\n",
            uOldAddr, uOldPort, pBlazeServer->LinkParams.uAddr, pBlazeServer->LinkParams.uPort);
        pBlazeServer->bDoSubspaceProvision = FALSE;
        return(-2);
    }
    else if (pBlazeServer->bDoSubspaceStop)
    {
        // if we are doing nothing or already working on a stop, then process stop
        if ((Response.eRequestType == SUBSPACE_REQUEST_TYPE_NONE) || (Response.eRequestType == SUBSPACE_REQUEST_TYPE_STOP))
        {
            // if the we are currently idle, make our request
            if (Response.eSubspaceState == SUBSPACE_STATE_IDLE)
            {
                // start things off
                char strServerUniqueID[128];
                _SubspaceGetUniqueId(strServerUniqueID, sizeof(strServerUniqueID));
                SubspaceApiRequest(pBlazeServer->pSubspaceApi, SUBSPACE_REQUEST_TYPE_STOP, pBlazeServer->uLocalServerIp, pBlazeServer->uLocalServerPort, strServerUniqueID);
                return(0);
            }
            else if (Response.eSubspaceState == SUBSPACE_STATE_IN_PROGRESS)
            {
                // wait
                return(0);
            }
            else if (Response.eSubspaceState == SUBSPACE_STATE_DONE)
            {
                // complete
                DirtyCastLogPrintf("gameserverblaze: subspaceapi stop completed.\n");
                pBlazeServer->bDoSubspaceStop = FALSE;
                return(1);
            }
            else if (Response.eSubspaceState == SUBSPACE_STATE_ERROR)
            {
                DirtyCastLogPrintf("gameserverblaze: subspaceapi in error state.\n");
            }
            else
            {
                DirtyCastLogPrintf("gameserverblaze: unknown subspaceapi state %d.\n", Response.eSubspaceState);
            }
        }
        else
        {
            DirtyCastLogPrintf("gameserverblaze: unexpected subspace request type %d\n", Response.eRequestType);
        }
        // complete in failure case
        pBlazeServer->bDoSubspaceStop = FALSE;
        return(-3);
    }
    // nothing to do
    return(-4);
}

/*F********************************************************************************/
/*!
    \Function _FilterFunction

    \Description
        Filters our logging

    \Input *pText           - the text we are filtering
    \Input uDebugLevel      - the debug level of the logging
    \Input *pFilteredLevel  - the debug level of the filtered text

    \Output
        int32_t             - 1=text filtered, 0=text not filtered

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static int32_t _FilterFunction(const char *pText, uint32_t uDebugLevel, uint32_t *pFilteredLevel)
{
    static bool _bFilteringBody = false;
    static uint32_t _uFilterDebugLevel = 0;
    int32_t iCheckStr;

    // check against requests filter if it is a request
    if (!strncmp(pText, "-> req:", 7))
    {
        for (iCheckStr = 0; _strFilterReq[iCheckStr].pFilterText != NULL; iCheckStr += 1)
        {
            if ((uDebugLevel <= _strFilterReq[iCheckStr].uDebugLevel) && strstr(pText, _strFilterReq[iCheckStr].pFilterText))
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
            if ((uDebugLevel <= _strFilterResp[iCheckStr].uDebugLevel) && strstr(pText, _strFilterResp[iCheckStr].pFilterText))
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
            if ((uDebugLevel <= _strFilterBody[iCheckStr].uDebugLevel) && !strncmp(pText, _strFilterBody[iCheckStr].pFilterText, strlen(_strFilterBody[iCheckStr].pFilterText)))
            {
                _bFilteringBody = true;
                *pFilteredLevel = _uFilterDebugLevel = _strFilterBody[iCheckStr].uDebugLevel;
                return(1);
            }
        }

        for (iCheckStr = 0; _strFilterText[iCheckStr].pFilterText != NULL; iCheckStr += 1)
        {
            if ((uDebugLevel <= _strFilterText[iCheckStr].uDebugLevel) && strstr(pText, _strFilterText[iCheckStr].pFilterText))
            {
                *pFilteredLevel = _strFilterText[iCheckStr].uDebugLevel;
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

/*F********************************************************************************/
/*!
    \Function _LoggingFunction

    \Description
        The BlazeSDK logging callback

    \Input *pText           - the text we are logging
    \Input *pData           - unused

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static void _LoggingFunction(const char8_t *pText, void *pData)
{
    uint32_t uDebugLevel, uFilterLevel = (unsigned)-1;

    // snuff blaze output if below debuglevel
    if ((uDebugLevel = _GetDebugLevel()) <= DIRTYCAST_DBGLVL_BLAZE)
    {
        return;
    }

    // check to see if this line should be filtered
    if (_FilterFunction(pText, uDebugLevel, &uFilterLevel) != 0)
    {
#if !DIRTYCAST_FILTERDEBUG   // show what would be filtered instead of filtering
        return;
#else
        DirtyCastLogPrintf("%% FILTERED (%d/%d) %% ", uDebugLevel, uFilterLevel);
#endif
    }

    // if not filtered, print it out
    DirtyCastLogPrintf("%s", pText);
}

/*F********************************************************************************/
/*!
    \Function _EAAssertFailure

    \Description
        Callback when an assert fails

    \Input *pExpr       - the expression the failed
    \Input *pFilename   - the filename where the failure occured
    \Input iLine        - the line number where the failure occured
    \Input *pFunction   - the function name where the failure occured
    \Input *pMsg        - the failure message
    \Input Args         - additional args to the message

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static bool _EAAssertFailure(const char *pExpr, const char *pFilename, int32_t iLine, const char *pFunction, const char *pMsg, va_list Args)
{
    char strFmtMsg[1024];

    ds_vsnzprintf(strFmtMsg, sizeof(strFmtMsg), pMsg, Args);
    DirtyCastLogPrintf("%s(%d) : assert failed: '%s' in function: %s\n, message: %s", pFilename, iLine, pExpr, pFunction, strFmtMsg);

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
        DirtyCastLogPrintf("%s(%u): blaze assert failed: '%s'\n", pFile, (unsigned)uLine, pAssertLiteral);
    }
}

/*F********************************************************************************/
/*!
    \Function _OnRedirectorMessages

    \Description
        Callback when we get messages back from the redirector

    \Input iResult  - result of the operation
    \Input *pMsgs   - the messages we receive from the redirector

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static void _OnRedirectorMessages(Blaze::BlazeError iResult, const Blaze::Redirector::DisplayMessageList *pMsgs)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(iResult), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: OnRedirectorMessages begin\n");

    for (const Blaze::EntityName &Message : *pMsgs)
    {
        DirtyCastLogPrintfVerbose(_GetDebugLevel(iResult), DIRTYCAST_DBGLVL_BLAZE, "%s\n", Message.c_str());
    }

    DirtyCastLogPrintfVerbose(_GetDebugLevel(iResult), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: OnRedirectorMessages end\n");
}

/*F********************************************************************************/
/*!
    \Function _GetKickReasonString

    \Description
        Converts the kick reason to a string

    \Input iReason  - the kick reason we are converting

    \Output
        const char *- the kick reason as a string, "UNKNOWN" if no match

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
static const char *_GetKickReasonString(int16_t iReason)
{
    const char *pReasonStr;

    // get kick reason ($$temp -- these strings should probably be available from DirtySDK)
    switch (iReason)
    {
    case GMDIST_ORPHAN:                         pReasonStr = "GMDIST_ORPHAN"; break;                            // (-1)  error - orphan
    case GMDIST_OVERFLOW:                       pReasonStr = "GMDIST_OVERFLOW"; break;                          // (-2)  error - overflow
    case GMDIST_INVALID:                        pReasonStr = "GMDIST_INVALID"; break;                           // (-3)  error - invalid
    case GMDIST_BADSETUP:                       pReasonStr = "GMDIST_BADSETUP"; break;                          // (-4)  error - badly setup number of players and/or index
    case GMDIST_SENDPROC_FAILED:                pReasonStr = "GMDIST_SENDPROC_FAILED"; break;                   // (-5)  error condition - SendProc failed
    case GMDIST_QUEUE_FULL:                     pReasonStr = "GMDIST_QUEUE_FULL"; break;                        // (-6)  error condition - too many queued packets
    case GMDIST_QUEUE_MEMORY:                   pReasonStr = "GMDIST_QUEUE_MEMORY"; break;                      // (-7)  error condition - queue has insufficient space left
    case GMDIST_STREAM_FAILURE:                 pReasonStr = "GMDIST_STREAM_FAILURE"; break;                    // (-8)  error - stream failed
    case GMDIST_NO_STREAM:                      pReasonStr = "GMDIST_NO_STREAM"; break;                         // (-9)  error - stream failed (no ref)
    case GMDIST_DELETED:                        pReasonStr = "GMDIST_DELETED"; break;                           // (-10) error - deleted
    case GMDIST_PEEK_ERROR:                     pReasonStr = "GMDIST_PEEK_ERROR"; break;                        // (-11) error peek
    case GMDIST_INPUTLOCAL_FAILED:              pReasonStr = "GMDIST_INPUTLOCAL_FAILED"; break;                 // (-12) error inputlocal
    case GMDIST_INPUTQUERY_FAILED:              pReasonStr = "GMDIST_INPUTQUERY_FAILED"; break;                 // (-13) error inputquery
    case GMDIST_DISCONNECTED:                   pReasonStr = "GMDIST_DISCONNECTED"; break;                      // (-14) error disconnected
    case GMDIST_INPUTLOCAL_FAILED_INVALID:      pReasonStr = "GMDIST_INPUTLOCAL_FAILED_INVALID"; break;         // (-15) error inputlocal with invalid.
    case GMDIST_INPUTLOCAL_FAILED_MULTI:        pReasonStr = "GMDIST_INPUTLOCAL_FAILED_MULTI"; break;           // (-16) error inputlocal with invalid.
    case GMDIST_INPUTLOCAL_FAILED_WINDOW:       pReasonStr = "GMDIST_INPUTLOCAL_FAILED_WINDOW"; break;          // (-17) error inputlocal with invalid.
    case GMDIST_OVERFLOW_MULTI:                 pReasonStr = "GMDIST_OVERFLOW_MULTI"; break;                    // (-18) error - overflow
    case GMDIST_OVERFLOW_WINDOW:                pReasonStr = "GMDIST_OVERFLOW_WINDOW"; break;                   // (-19) error - overflow
    case GMDIST_DESYNCED:                       pReasonStr = "GMDIST_DESYNCED"; break;                          // (-20) error - desynced
    case GMDIST_DESYNCED_ALL_PLAYERS:           pReasonStr = "GMDIST_DESYNCED_ALL_PLAYERS"; break;              // (-21) error - desynced all players

    case GMLINK_DIRTYCAST_DELETED:              pReasonStr = "GMLINK_DIRTCAST_DELETED"; break;                  // (-64)   //<! error - GameServer had the link deleted
    case GMLINK_DIRTYCAST_CONNECTION_TIMEDOUT:  pReasonStr = "GMLINK_DIRTYCAST_CONNECTION_TIMEDOUT"; break;     // (-65)   //<! error - GameServer connection timed out
    case GMLINK_DIRTYCAST_CONNECTION_FAILURE:   pReasonStr = "GMLINK_DIRTYCAST_CONNECTION_FAILURE"; break;      // (-66)   //<! error - GameServer initial connection failed
    case GMLINK_DIRTYCAST_PEER_DISCONNECT:      pReasonStr = "GMLINK_DIRTYCAST_PEER_DISCONNECT"; break;         // (-67)   //<! error - GameServer peer diconnected

    default:                                    pReasonStr = "UNKNOWN"; break;
    }

    return(pReasonStr);
}

#ifndef EASTL_DEBUG_BREAK
// implementation of EASTL_DEBUG_BREAK when not implemented by EASTL
void EASTL_DEBUG_BREAK()
{
}
#endif

/*F********************************************************************************/
/*!
    \Function GameServerBlazeAllocatorT::GameServerBlazeAllocatorT

    \Description
        Constructs the GameServerBlazeAllocatorT struct

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
GameServerBlazeAllocatorT::GameServerBlazeAllocatorT()
    : m_iMemgroup(0)
    , m_bMemgroupInited(FALSE)
{
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeAllocatorT::Alloc

    \Description
        Allocates a block of memory

    \Input uSize    - size of the block
    \Input *pName   - name of the allocation
    \Input uFlags   - allocation flags

    \Output
        void *      - the block of memory allocated or NULL if failure

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void *GameServerBlazeAllocatorT::Alloc(size_t uSize, const char *pName, unsigned int uFlags)
{
    if (!m_bMemgroupInited)
    {
        m_bMemgroupInited = TRUE;
        DirtyMemGroupQuery(&m_iMemgroup, NULL);
    }

    // allocate state for ref
    return(DirtyMemAlloc((int32_t)uSize, GAMESERVERBLAZE_MEMID, m_iMemgroup, NULL));
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeAllocatorT::Alloc

    \Description
        Allocates a block of memory

    \Input uSize        - size of the block
    \Input *pName       - name of the allocation
    \Input uFlags       - allocation flags
    \Input uAlign       - unused
    \Input uAlignOffset - unused

    \Output
        void *      - the block of memory allocated or NULL if failure

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void *GameServerBlazeAllocatorT::Alloc(size_t uSize, const char *pName, unsigned int uFlags, unsigned int uAlign, unsigned int uAlignOffset)
{
    return(Alloc(uSize, pName, uFlags));
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeAllocatorT::Free

    \Description
        Free a block of memory

    \Input *pBlock  - block of memory
    \Input uSize    - size of the block

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeAllocatorT::Free(void *pBlock, size_t uSize)
{
    DirtyMemFree(pBlock, GAMESERVERBLAZE_MEMID, m_iMemgroup, NULL);
}

/*F********************************************************************************/
/*!
    \Function EA::Allocator::ICoreAllocator::GetDefaultAllocator()

    \Description
        Export the needed functions used by the memory system

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
ICOREALLOCATOR_INTERFACE_API EA::Allocator::ICoreAllocator *EA::Allocator::ICoreAllocator::GetDefaultAllocator()
{
    static GameServerBlazeAllocatorT g_Allocator;
    return(&g_Allocator);
}

/*F********************************************************************************/
/*!
    \Function operator new[]

    \Description
        Created to link eastl calls to DirtyMemAlloc

    \Input uSize         - size of memory to allocate
    \Input *pName        - unused
    \Input iFlags        - unused
    \Input uDebugFlags   - unused
    \Input *pFile        - unused
    \Input iLine         - unused

    \Output
        void *          - pointer to memory, or NULL
*/
/********************************************************************************F*/
void *operator new[](size_t uSize, const char *pName, int iFlags, unsigned uDebugFlags, const char *pFile, int iLine)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    return(DirtyMemAlloc((int32_t)uSize, EASTL_MEMID, iMemGroup, pMemGroupUserData));
}

/*F********************************************************************************/
/*!
    \Function operator new[]

    \Description
        Created to link eastl calls to DirtyMemAlloc

    \Input uSize             - size of memory to allocate
    \Input uAlignment        - unused
    \Input uAlignmentOffset  - unused
    \Input *pName            - unused
    \Input iFlags            - unused
    \Input uDebugFlags       - unused
    \Input *pFile            - unused
    \Input iLine             - unused

    \Output
        void *              - pointer to memory, or NULL
*/
/********************************************************************************F*/
void *operator new[](size_t uSize, size_t uAlignment, size_t uAlignmentOffset, const char* pName, int iFlags, unsigned uDebugFlags, const char *pFile, int iLine)
{
    int32_t iMemGroup;
    void *pMemGroupUserData;
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
    return(DirtyMemAlloc((int32_t)uSize, EASTL_MEMID, iMemGroup, pMemGroupUserData));
}

#if !DIRTYCAST_SANITIZE
/*F********************************************************************************/
/*!
    \Function operator delete

    \Description
        Created to link eastl calls to DirtyMemFree

    \Input *pMem            - memory pointer to free
    \Input *pName           - unused
    \Input iFlags           - unused
    \Input uDebugFlags      - unused
    \Input *pFile           - unused
    \Input iLine            - unused
*/
/********************************************************************************F*/
void operator delete(void *pMem, const char *pName, int iFlags, unsigned uDebugFlags, const char *pFile, int iLine)
{
    if (pMem != NULL)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
        DirtyMemFree(pMem, EASTL_MEMID, iMemGroup, pMemGroupUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function operator delete

    \Description
        Created to link eastl calls to DirtyMemFree

    \Input *pMem            - memory pointer to free
*/
/********************************************************************************F*/
void operator delete(void *pMem) NOEXCEPT
{
    if (pMem != NULL)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
        DirtyMemFree(pMem, EASTL_MEMID, iMemGroup, pMemGroupUserData);
    }
}

/*F********************************************************************************/
/*!
    \Function operator delete

    \Description
        Created to link eastl calls to DirtyMemFree

    \Input *pMem            - memory pointer to free
*/
/********************************************************************************F*/
void operator delete[](void *pMem) NOEXCEPT
{
    if (pMem != NULL)
    {
        int32_t iMemGroup;
        void *pMemGroupUserData;
        DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);
        DirtyMemFree(pMem, EASTL_MEMID, iMemGroup, pMemGroupUserData);
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function GameServerStateEventHandlerC::onConnected

    \Description
        Callback fired when we connect to the server

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerStateEventHandlerC::onConnected()
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    Blaze::BlazeHub *pBlazeHub = pBlazeServer->pBlazeHub;
    GameServerConfigT *pConfig = pBlazeServer->pConfig;

    // we are connected!
    DirtyCastLogPrintf("gameserverblaze: connected to blaze server\n");

    DirtyCastLogPrintf("gameserverblaze: DoS2STrustLogin, starting login.\n");
    pBlazeHub->getLoginManager(0)->startTrustedLoginProcess(pBlazeServer->strAccessToken, pConfig->strServer, "DirtyCast");
}

/*F********************************************************************************/
/*!
    \Function GameServerStateEventHandlerC::onDisconnected

    \Description
        Callback fired when we disconnect from the server

    \Input iErrorCode   - the error code associated with the disconnect

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerStateEventHandlerC::onDisconnected(Blaze::BlazeError iErrorCode)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    DirtyCastLogPrintf("gameserverblaze: onDisconnected error 0x%08x (%s); disconnecting\n", iErrorCode, GameServerBlazeGetErrorCodeName(iErrorCode));

    pBlazeServer->pCallback(pBlazeServer->pCallbackData, GAMESERVER_EVENT_DELETE, &pBlazeServer->Game);
    _MoveToNextState(pBlazeServer, kDisconnected);

    pBlazeServer->uLastConnectionErrorKind = 'UNKN';
    pBlazeServer->uLastConnectionErrorCode = iErrorCode;

    // Clear the token since we are disconnecting and want to pull a fresh one
    // from the voipserver
    ds_memclr(pBlazeServer->strAccessToken, sizeof(pBlazeServer->strAccessToken));

    _ClearBlazeGame(pBlazeServer);
    ds_memcpy(&pBlazeServer->LastGame, &pBlazeServer->Game, sizeof(pBlazeServer->LastGame));
}

/*F********************************************************************************/
/*!
    \Function GameServerStateEventHandlerC::onAuthenticated

    \Description
        Callback fired when we authenticate with the server

    \Input uUserIndex   - index of the user that authenticated

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerStateEventHandlerC::onAuthenticated(uint32_t uUserIndex)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    Blaze::UserManager::LocalUser *pLocalUser = pBlazeServer->pBlazeHub->getUserManager()->getLocalUser(uUserIndex);

    // we are connected and logged in!
    DirtyCastLogPrintf("gameserverblaze: authenticated with blaze server (id=%lld, conngroup=%lld)\n", pLocalUser->getId(), pLocalUser->getConnectionGroupObjectId().id);
    _MoveToNextState(pBlazeServer, kConnected);
    pBlazeServer->uLastConnectionErrorKind = 0;
    pBlazeServer->uLastConnectionErrorCode = 0;
}

/*F********************************************************************************/
/*!
    \Function GameServerStateEventHandlerC::onDeAuthenticated

    \Description
        Callback fired when we deauthenticate with the server

    \Input uUserIndex   - index of the user that deauthenticated

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerStateEventHandlerC::onDeAuthenticated(uint32_t uUserIndex)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    DirtyCastLogPrintf("gameserverblaze: de-authenticated from blaze server\n");
    pBlazeServer->pGameListener->Clear();
}

/*F********************************************************************************/
/*!
    \Function GameServerStateEventHandlerC::onLoginFailure

    \Description
        Callback fired when we failed to login to the server

    \Input iErrorCode   - the error code associated with the login failure
    \Input *pPortalUrl  - unused

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerStateEventHandlerC::onLoginFailure(Blaze::BlazeError iErrorCode, const char8_t *pPortalUrl)
{
    Blaze::BlazeHub *pBlazeHub = _pGameServerBlaze->pBlazeHub;

    DirtyCastLogPrintf("gameserverblaze: onLoginFailure %s\n", GameServerBlazeGetErrorCodeName(iErrorCode));

    if (iErrorCode != Blaze::ERR_OK)
    {
        // Authentication can fail for reasons such as ERR_TIMEOUT.
        // Our state here will be AUTHENTICATE_TRANSITION.
        // Easiest thing to do is to start over with the Blaze connection.

        // issue disconnect to reset us to a known state
        pBlazeHub->getConnectionManager()->disconnect();
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerStateEventHandlerC::onSdkError

    \Description
        Callback fired when wwe get a fatal sdk error

    \Input iErrorCode   - the error code associated with the login failure

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerStateEventHandlerC::onSdkError(Blaze::BlazeError iErrorCode)
{
    Blaze::BlazeHub *pBlazeHub = _pGameServerBlaze->pBlazeHub;

    DirtyCastLogPrintf("gameserverblaze: onSdkError %s\n", GameServerBlazeGetErrorCodeName(iErrorCode));

    if (iErrorCode != Blaze::ERR_OK)
    {
        // Authentication can fail for reasons such as ERR_TIMEOUT.
        // Our state here will be AUTHENTICATE_TRANSITION.
        // Easiest thing to do is to start over with the Blaze connection.

        // issue disconnect to reset us to a known state
        pBlazeHub->getConnectionManager()->disconnect();
    }
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onGameCreated

    \Description
        Callback fired when the game is created

    \Input *pCreatedGame    - pointer to the game that was created

    \Version 09/23/2020 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onGameCreated(Blaze::GameManager::Game *pCreatedGame)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    // setup the game
    pCreatedGame->addListener(pBlazeServer->pGameListener);
    _MoveToNextState(pBlazeServer, kCreatedGame);
    pBlazeServer->pGame = pCreatedGame;

    DirtyCastLogPrintf("gameserverblaze: game created (%llu)\n", pCreatedGame->getId());
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onGameDestructing

    \Description
        Callback fired when the game is being destroyed

    \Input *pGameManagerApi - pointer to the gamemanager
    \Input *pDyingGame      - pointer to the game that is dying
    \Input eReason          - the reason for the destruction

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onGameDestructing(Blaze::GameManager::GameManagerAPI *pGameManagerApi, Blaze::GameManager::Game *pDyingGame, Blaze::GameManager::GameDestructionReason eReason)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    DirtyCastLogPrintf("gameserverblaze: onGameDestructing: %s/%d\n", GameDestructionReasonToString(eReason), eReason);

    pBlazeServer->pGame = NULL;

    // if we are provisioning don't mess with game destruction/creation, we will be re-creating the game to provision anyways
    if (pBlazeServer->bDoSubspaceProvision == FALSE)
    {
        // if we ejected ourselves then create our game again
        if (eReason == Blaze::GameManager::HOST_EJECTION)
        {
            GameServerBlazeCreateGame();
        }
        // if we are being injected then wait for the event
        else if (eReason != Blaze::GameManager::HOST_INJECTION)
        {
            pBlazeServer->pGameListener->GameDestroyed();
        }
    }
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onMatchmakingScenarioFinished

    \Description
        Callback fired when scenario matchmaking is complete

        \Input eMatchmakingResult   - the result of the matchmaking
        \Input *pMatchmakingSession - pointer the matchmaking session
        \Input *pGame               - the game that was resulted from matchmaking

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onMatchmakingScenarioFinished(Blaze::GameManager::MatchmakingResult eMatchmakingResult, const Blaze::GameManager::MatchmakingScenario *pMatchmakingScenario, Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onMatchmakingScenarioFinished\n");
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onMatchmakingScenarioAsyncStatus

    \Description
        Callback fired when get an matchmaking status update for scenario matchmaking

    \Input *pMatchmakingSession         - pointer the matchmaking session
    \Input *pMatchmakingAsyncStatusList - the matchmaking status data

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onMatchmakingScenarioAsyncStatus(const Blaze::GameManager::MatchmakingScenario *eMatchmakingScenario, const Blaze::GameManager::MatchmakingAsyncStatusList *pMatchmakingAsyncStatusList)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onMatchmakingScenarioAsyncStatus\n");
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onGameBrowserListUpdated

    \Description
        Callback fired when the gamebrowser list is updated

    \Input *pGameBrowserList    - the gamebrowser list
    \Input *pRemovedGames       - removed games from the list
    \Input *pUpdatedGames       - updated games from the list

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onGameBrowserListUpdated(Blaze::GameManager::GameBrowserList *pGameBrowserList, const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *pRemovedGames, const Blaze::GameManager::GameBrowserList::GameBrowserGameVector *pUpdatedGames)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameBrowserListUpdated\n");
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onGameBrowserListUpdateTimeout

    \Description
        Callback fired when the gamebrowser list update timedout

    \Input *pGameBrowserList    - the gamebrowser list

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onGameBrowserListUpdateTimeout(Blaze::GameManager::GameBrowserList *pGameBrowserList)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameBrowserListUpdateTimeout\n");
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onGameBrowserListDestroy

    \Description
        Callback fired when the gamebrowser list is destroyed

    \Input *pGameBrowserList    - the gamebrowser list

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onGameBrowserListDestroy(Blaze::GameManager::GameBrowserList *pGameBrowserList)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameBrowserListDestroy\n");
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onUserGroupJoinGame

    \Description
        Callback fired when the group joins a game

    \Input *pGame   - the game that was joined
    \Input *pPlayer - the player information
    \Input GroupId  - the group identifier

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onUserGroupJoinGame(Blaze::GameManager::Game *pGame, Blaze::GameManager::Player *pPlayer, Blaze::GameManager::UserGroupId GroupId)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onUserGroupJoinGame\n");
}

/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onTopologyHostInjecting

    \Description
        Callback fired when we are being injected but not yet complete

    \Input *pGame   - the game that we were injected into

    \Version 03/19/2019 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onTopologyHostInjectionStarted(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onTopologyHostInjecting\n");
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    // set the game to be used
    pGame->addListener(pBlazeServer->pGameListener);
    pBlazeServer->pGame = pGame;
    _ResetGame(pBlazeServer, pGame, GAMESERVER_EVENT_CREATE);

    // add the players that already exist to the client list
    for (uint16_t uPlayerIndex = 0; uPlayerIndex < pGame->getPlayerCount(); uPlayerIndex += 1)
    {
        Blaze::GameManager::Player *pPlayer = pGame->getPlayerByIndex(uPlayerIndex);
        pBlazeServer->pGameListener->onPlayerJoining(pPlayer);
    }
}
/*F********************************************************************************/
/*!
    \Function GameManagerListenerC::onTopologyHostInjected

    \Description
        Callback fired when we were injected into a game as a host

    \Input *pGame   - the game that we were injected into

    \Version 03/19/2019 (eesponda)
*/
/********************************************************************************F*/
void GameManagerListenerC::onTopologyHostInjected(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onTopologyHostInjected\n");
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    // finish setup for game to be used
    pBlazeServer->bInjected = TRUE;
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::GameServerBlazeGameListenerC

    \Description
        Constructs the GameServerBlazeListenerC class

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
GameServerBlazeGameListenerC::GameServerBlazeGameListenerC()
{
    Clear();
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::GameCreated

    \Description
        Callback fired from createGameFromTemplate RPC completion

    \Input iError                   - the result of the game creation
    \Input JobId                    - the identifier associated with the RPC
    \Input *pGame                   - the game created
    \Input *pFailedEntryCriteria    - why we failed to entry the game

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::GameCreated(Blaze::BlazeError iError, Blaze::JobId JobId, Blaze::GameManager::Game *pGame, const char *pFailedEntryCriteria)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    if (iError != Blaze::ERR_OK)
    {
        pBlazeServer->pGame = NULL;
        pBlazeServer->uLastConnectionErrorKind = 'CREA';
        pBlazeServer->uLastConnectionErrorCode = iError;
        pBlazeServer->pCallback(pBlazeServer->pCallbackData, GAMESERVER_EVENT_GAME_CREATION_FAILURE, NULL);
        DirtyCastLogPrintf("gameserverblaze: game creation failed with error 0x%08x (err=%s, criteria=%s)\n", iError, GameServerBlazeGetErrorCodeName(iError), pFailedEntryCriteria);
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::GameDestroyed

    \Description
        Callback fired from destroyGame RPC completion

    \Input iError   - the result of the game destruction
    \Input *pGame   - the game destroyed

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::GameDestroyed(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    if (iError != Blaze::ERR_OK)
    {
        DirtyCastLogPrintf("gameserverblaze: WARNING: GameDestroyed failed. returned %d\n", iError);
    }

    // clear our tracking of the blaze game
    _ClearBlazeGame(pBlazeServer);

    // clear our pointer to the blaze game object
    pBlazeServer->pGame = NULL;
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::GameReset

    \Description
        Callback fired from resetGame RPC completion

    \Input iError   - the result of the game reset
    \Input *pGame   - the game reset

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::GameReset(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    DirtyCastLogPrintf("gameserverblaze: GameReset (err=%d/%s)\n", iError, pBlazeServer->pBlazeHub->getErrorName(iError));

    if (iError == Blaze::ERR_OK)
    {
        // game has been returned to the pool, clean up its state
        m_bNeedsReset = false;
        _ResetGame(pBlazeServer, pGame, GAMESERVER_EVENT_DELETE);
    }
    else
    {
        DirtyCastLogPrintf("gameserverblaze: WARNING: returnDedicatedServerToPool failed. Trying on idle\n");
        m_bNeedsIdleReset = true;
        m_uLastIdle = NetTick();
        m_pIdleGame = pGame;
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::HostEjected

    \Description
        Callback fired from ejectHost

    \Input iError   - the result of the ejection
    \Input *pGame   - the game we are leaving
    \Input Id       - identifier of the RPC call

    \Version 03/18/2019 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::HostEjected(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame, Blaze::JobId Id)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    _ResetGame(pBlazeServer, pGame, GAMESERVER_EVENT_DELETE);
    pBlazeServer->bInjected = FALSE;

    // if ejection failed for whatever reason, just set the game as destroyed which will go through re-login process
    if (iError != Blaze::ERR_OK)
    {
        pBlazeServer->pGame = NULL;
        pBlazeServer->pGameListener->GameDestroyed();
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::GameEnded

    \Description
        Callback fired from endGame RPC completion

    \Input iError   - the result of the game end
    \Input *pGame   - the game ended

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::GameEnded(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(iError), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: GameEnded\n");
    DirtyCastLogPrintfVerbose(_GetDebugLevel(iError), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: Blaze::BlazeError err is %d\n", iError);

    if (iError != Blaze::ERR_OK)
    {
        DirtyCastLogPrintf("gameserverblaze: Simulating OnGameEnded, as an attempt to recover\n");
        onGameEnded(pGame);
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::InitGameComplete

    \Description
        Callback fired from initGameComplete RPC completion

    \Input iError   - the result of the init game completion
    \Input *pGame   - the game with init completion

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::InitGameComplete(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(iError), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: InitGameComplete\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::PlayerKicked

    \Description
        Callback fired from kickPlayer RPC completion

    \Input iError   - the result of the kickPlayer
    \Input *pGame   - the game that the player was kicked from
    \Input *pPlayer - the player that was kicked from the game

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::PlayerKicked(Blaze::BlazeError iError, Blaze::GameManager::Game *pGame, const Blaze::GameManager::Player *pPlayer)
{
    // Not logging this one even if in error.
    // Given the frequency of player kick and race conditions in disconnection, there could be too many errors logged
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: PlayerKicked\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::Idle

    \Description
        Handles idle processing for the game listener

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::Idle()
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    // nothing left to do if idle doesn't need reset
    if (!m_bNeedsIdleReset)
    {
        return;
    }

    if (NetTickDiff(NetTick(), m_uLastIdle) > pBlazeServer->iConnectDelay)
    {
        m_bNeedsIdleReset = false;

        if (m_pIdleGame != NULL)
        {
            m_pIdleGame->returnDedicatedServerToPool(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::GameReset));
        }
        m_iIdleResetCount++;
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::Clear

    \Description
        Clears the member data for this class

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::Clear()
{
    DirtyCastLogPrintf("gameserverblaze: game cleared\n");

    m_bNeedsReset = false;
    m_bNeedsIdleReset = false;
    m_pIdleGame = NULL;
    m_uLastIdle = 0;
    m_iIdleResetCount = 0;
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::Clear

    \Description
        Checks if our cached idle game is stick

    \Output
        bool    - true if stuck, false otherwise

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
bool GameServerBlazeGameListenerC::IsStuck()
{
    if (m_bNeedsIdleReset && (m_iIdleResetCount > MAX_RESET_ATTEMPT))
    {
        return(true);
    }
    return(false);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::GameDestroyed

    \Description
        Called when the game is being destroyed to allow us to reset our data

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::GameDestroyed()
{
    m_bNeedsIdleReset = true;
    m_iIdleResetCount = MAX_RESET_ATTEMPT + 1;
    m_uLastIdle = NetTick();
    m_pIdleGame = NULL;
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPreGame

    \Description
        Callback fired when a game hits the PRE_GAME state

    \Input *pGame   - the game that entered PRE_GAME

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPreGame(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPreGame\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onGameStarted

    \Description
        Callback fired when a game hits the IN_GAME state

    \Input *pGame   - the game that entered IN_GAME

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onGameStarted(Blaze::GameManager::Game *pGame)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameStarted\n");

    _MoveToNextState(pBlazeServer, kGameStarted);
    pBlazeServer->pCallback(pBlazeServer->pCallbackData, GAMESERVER_EVENT_PLAY, &pBlazeServer->Game);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onGameEnded

    \Description
        Callback fired when a game hits the POST_GAME state

    \Input *pGame   - the game that entered POST_GAME

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onGameEnded(Blaze::GameManager::Game *pGame)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameEnded\n");

    pBlazeServer->pCallback(pBlazeServer->pCallbackData, GAMESERVER_EVENT_END, &pBlazeServer->Game);

    if (m_bNeedsReset)
    {
        DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: returning to pool\n");

        pGame->returnDedicatedServerToPool(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::GameReset));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onGameReset

    \Description
        Callback fired when a game is reset

    \Input *pGame   - the game that was reset

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onGameReset(Blaze::GameManager::Game *pGame)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameReset\n");

    m_bNeedsReset = false;
    m_bNeedsIdleReset = false;
    m_iIdleResetCount = 0;

    // reset the game
    _ResetGame(pBlazeServer, pGame, GAMESERVER_EVENT_CREATE);

    // finish up game initialization
    pGame->initGameComplete(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::InitGameComplete));
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onReturnDServerToPool

    \Description
        Callback fired when our server is returned to pool to await reset

    \Input *pGame   - the game that was moved back to the pool

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onReturnDServerToPool(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onReturnDServerToPool\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onGameAttributeUpdated

    \Description
        Callback fired when the game attributes for the game are updated

    \Input *pGame                   - the game that had its attributes updated
    \Input *pChangedAttributeMap    - the attributes that were changed

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onGameAttributeUpdated(Blaze::GameManager::Game *pGame, const Blaze::Collections::AttributeMap *pChangedAttributeMap)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameAttributeUpdated\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onGameSettingUpdated

    \Description
        Callback fired when the game settings are updated

    \Input *pGame - the game that had its settings updated

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onGameSettingUpdated(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameSettingUpdated\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerCapacityUpdated

    \Description
        Callback fired when the player capacity for the game is changed

    \Input *pGame - the game that had its capacity updated

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerCapacityUpdated(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerCapacityUpdated\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerJoining

    \Description
        Callback fired when a new player is joining the game

    \Input *pPlayer - the new player that is joining

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerJoining(Blaze::GameManager::Player *pPlayer)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    int32_t iId = 0;
    int32_t iRefCount;
    const char *pName = pPlayer->getName();

    if (pPlayer->isReserved())
    {
        DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerJoining for RESERVED player '%s' ignored\n", pPlayer->getName());
        return;
    }

    iId = static_cast<int32_t>(pPlayer->getMeshEndpoint()->getRemoteLowLevelConnectivityId());

    if ((iRefCount = _AddPlayerToGame(pBlazeServer, pName, pPlayer->getSlotId(), iId)) > 1)
    {
        DirtyCastLogPrintf("gameserverblaze: adding user %s to game for client 0x%08x at slot #%d (client refcount incremented to %d)\n", pPlayer->getName(), iId, pPlayer->getSlotId(), iRefCount);

        // early exit because client entry was refcounted only
        return;
    }
    else
    {
        DirtyCastLogPrintf("gameserverblaze: adding user %s to game for client 0x%08x at slot #%d (client refcount incremented to %d)\n", pPlayer->getName(), iId, pPlayer->getSlotId(), iRefCount);
    }

    if (pBlazeServer->pCallback(pBlazeServer->pCallbackData, GAMESERVER_EVENT_GAME, &pBlazeServer->Game))
    {
        ds_memcpy(&pBlazeServer->LastGame, &pBlazeServer->Game, sizeof(pBlazeServer->LastGame));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerJoinedFromReservation

    \Description
        Callback fired when a new player is joining the game from a reservation

    \Input *pPlayer - the new player that is joining

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerJoinedFromReservation(Blaze::GameManager::Player *pPlayer)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    GameServerClientT *pClient = &pBlazeServer->Game.aClients[pPlayer->getSlotId() - 1];

    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerJoinedFromReservation\n");

    if (pClient->bReserved)
    {
        DirtyCastLogPrintf("gameserverblaze: moving user %s at slot #%d from a RESERVED state after claiming a disconnect reservation\n", pPlayer->getName(),
            pPlayer->getSlotId());
        pClient->bReserved = FALSE;
    }
    else
    {
        onPlayerJoining(pPlayer);
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerJoiningQueue

    \Description
        Callback fired when a new player is joining the game queue

    \Input *pPlayer - the new player that is joining the queue

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerJoiningQueue(Blaze::GameManager::Player *pPlayer)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerJoiningQueue\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerJoinedFromQueue

    \Description
        Callback fired when a new player is joining the game from the queue

    \Input *pPlayer - the new player that is joining

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerJoinedFromQueue(Blaze::GameManager::Player *pPlayer)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerJoinedFromQueue\n");
    onPlayerJoining(pPlayer);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerJoinComplete

    \Description
        Callback fired when the player join is complete

    \Input *pPlayer - the new player that is joined

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerJoinComplete(Blaze::GameManager::Player *pPlayer)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerJoinComplete\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onQueueChanged

    \Description
        Callback fired when the queue has changed

    \Input *pGame   - the game which had its queue changed

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onQueueChanged(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onQueueChanged\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerRemoved

    \Description
        Callback fired when a player is removed from the game

    \Input *pGame               - the game which had a player removed
    \Input *pRemovedPlayer      - the player who was removed
    \Input ePlayerRemovedReason - the reason the player was removed
    \Input uTitleContext        - the context for which the player was removed

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerRemoved(Blaze::GameManager::Game *pGame, const Blaze::GameManager::Player *pRemovedPlayer, Blaze::GameManager::PlayerRemovedReason ePlayerRemovedReason, Blaze::GameManager::PlayerRemovedTitleContext uTitleContext)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    int32_t iRefCount;

    // save removal reason and log the removal
    pBlazeServer->uLastRemovedReason = (uint16_t)ePlayerRemovedReason;
    pBlazeServer->iLastKickReason = (int16_t)uTitleContext;
    switch (pGame->getGameState())
    {
    case Blaze::GameManager::PRE_GAME:
        pBlazeServer->uLastGameState = GAMESERVER_GAMESTATE_PREGAME;
        break;
    case Blaze::GameManager::IN_GAME:
        pBlazeServer->uLastGameState = GAMESERVER_GAMESTATE_INGAME;
        break;
    case Blaze::GameManager::POST_GAME:
        pBlazeServer->uLastGameState = GAMESERVER_GAMESTATE_POSTGAME;
        break;
    case Blaze::GameManager::MIGRATING:
        // ignore transitions to MIGRATING when in POSTGAME state; for our metrics purposes we'd rather track these leaves as POSTGAME
        if (pBlazeServer->uLastGameState != GAMESERVER_GAMESTATE_POSTGAME)
        {
            pBlazeServer->uLastGameState = GAMESERVER_GAMESTATE_MIGRATING;
        }
        break;
    default:
        pBlazeServer->uLastGameState = GAMESERVER_GAMESTATE_OTHER;
        break;
    }

    if ((iRefCount = _RemovePlayerFromGame(pBlazeServer, static_cast<int32_t>(pRemovedPlayer->getMeshEndpoint()->getRemoteLowLevelConnectivityId()), pRemovedPlayer->getName())) > 0)
    {
        DirtyCastLogPrintf("gameserverblaze: removing user %s from game for client 0x%08x at slot #%d (client refcount decremented to %d)\n",
            pRemovedPlayer->getName(),
            static_cast<int32_t>(pRemovedPlayer->getMeshEndpoint()->getRemoteLowLevelConnectivityId()),
            pRemovedPlayer->getSlotId(),
            iRefCount);

        // early exit because refcount has not yet reached 0
        return;
    }
    else
    {
        DirtyCastLogPrintf("gameserverblaze: removing user %s from game for client 0x%08x at slot #%d (client refcount decremented to %d), (count=%d), (state=%s); %s/%d\n",
            pRemovedPlayer->getName(),
            static_cast<int32_t>(pRemovedPlayer->getMeshEndpoint()->getRemoteLowLevelConnectivityId()),
            pRemovedPlayer->getSlotId(),
            iRefCount,
            pGame->getActivePlayerCount(),
            Blaze::GameManager::GameStateToString(pGame->getGameState()),
            Blaze::GameManager::PlayerRemovedReasonToString(ePlayerRemovedReason), (int16_t)uTitleContext);
    }

    if (pBlazeServer->pCallback(pBlazeServer->pCallbackData, GAMESERVER_EVENT_GAME, &pBlazeServer->Game))
    {
        ds_memcpy(&pBlazeServer->LastGame, &pBlazeServer->Game, sizeof(pBlazeServer->LastGame));
    }

    // if we still have users connected, we have nothing left to do
    if (pGame->getActivePlayerCount() > 0)
    {
        return;
    }

    // if the game was not host injected follow the normal flow
    if (!pBlazeServer->bInjected)
    {
        // If the game has ended and we are using subspace, we instead destroy the game since we need to re-provision an ip/port.
        // We don't provision in PRE_GAME since:
        // - usually not much time has passed (no need to provision), the uSubspaceProvisionRateMin will handle if a long duration
        // - a race condition may result in destroying the game as people are leaving/joining
        if ((pBlazeServer->pSubspaceApi != NULL) && (pGame->getGameState() != Blaze::GameManager::PRE_GAME))
        {
            DirtyCastLogPrintf("gameserverblaze: destroying game for subspace provision since all players left.\n");
            pBlazeServer->bDoSubspaceProvision = TRUE;
        }
        else if ((pGame->getGameState() == Blaze::GameManager::PRE_GAME) || (pGame->getGameState() == Blaze::GameManager::POST_GAME) || (pGame->getGameState() == Blaze::GameManager::INITIALIZING))
        {
            DirtyCastLogPrintf("gameserverblaze: empty game, returning to pool\n");
            pGame->returnDedicatedServerToPool(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::GameReset));
        }
        else if (pGame->getGameState() == Blaze::GameManager::RESETABLE)
        {
            DirtyCastLogPrintf("gameserverblaze: resettable game, deleting local game\n");
            _ResetGame(pBlazeServer, pGame, GAMESERVER_EVENT_DELETE);
        }
        else if (pGame->getGameState() == Blaze::GameManager::UNRESPONSIVE)
        {
            // we can be here if we actually became responsive again but just got the (queued) notification of the
            // state change to UNRESPONSIVE. The state change out of UNRESPONSIVE should come soon, wait for it.
            m_bNeedsReset = true;
            DirtyCastLogPrintf("gameserverblaze: empty game, recovering from unresponsive state, awaiting onBackFromUnresponsive before returning to pool\n");
        }
        else
        {
            m_bNeedsReset = true;
            if (pGame->getGameState() == Blaze::GameManager::IN_GAME)
            {
                DirtyCastLogPrintf("gameserverblaze: empty game, ending it before returning to pool\n");

                pGame->endGame(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::GameEnded));
            }
            else
            {
                DirtyCastLogPrintf("gameserverblaze: WARNING ! Unexpected state %d at last player removal.\n", pGame->getGameState());
                DirtyCastLogPrintf("gameserverblaze: Will attempt reset at next known transition.\n");
            }
        }
    }
    /* otherwise, we just eject ourselves regardless of the game state. when the game is ejected the game state goes to
       inactive virtual. if we change the game state in an attempt to eject the host this would cause the game to potentially
       go into an unexpected state that the game clients don't expect. another host will get injected if either player
       claims their reservation */
    else
    {
        DirtyCastLogPrintf("gameserverblaze: empty game, ejecting ourselves from the game\n");
        pGame->ejectHost(Blaze::GameManager::Game::EjectHostJobCb(this, &GameServerBlazeGameListenerC::HostEjected));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onAdminListChanged

    \Description
        Callback fired when the admin list has changed

    \Input *pGame           - the game which had its admin list changed
    \Input *pAdminPlayer    - the subject of the change
    \Input eOperation       - the operation that was performed on the list
    \Input PlayerId         - the player id associated with the change

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onAdminListChanged(Blaze::GameManager::Game *pGame, const Blaze::GameManager::Player *pAdminPlayer, Blaze::GameManager::UpdateAdminListOperation eOperation, Blaze::GameManager::PlayerId PlayerId)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onAdminListChanged\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerAttributeUpdated

    \Description
        Callback fired when player attributes are changed

    \Input *pPlayer                 - the player whose attributes are changed
    \Input *pChangedAttributeMap    - the changed attributes for the player

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerAttributeUpdated(Blaze::GameManager::Player *pPlayer, const Blaze::Collections::AttributeMap *pChangedAttributeMap)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerAttributeUpdated\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerTeamUpdated

    \Description
        Callback fired when the player team has changed

    \Input *pPlayer             - the player whose team has changed
    \Input uPreviousTeamIndex   - the previous team index for the player

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerTeamUpdated(Blaze::GameManager::Player *pPlayer, Blaze::GameManager::TeamIndex uPreviousTeamIndex)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerTeamUpdated\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onGameTeamIdUpdated

    \Description
        Callback  fired when the game team id was updated

    \Input *pGame       - the game whose team id was updated
    \Input uOldTeamId   - the old team id for the game
    \Input uNewTeamId   - the new team id for the game

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onGameTeamIdUpdated(Blaze::GameManager::Game *pGame, Blaze::GameManager::TeamId uOldTeamId, Blaze::GameManager::TeamId uNewTeamId)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onGameTeamIdUpdated\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerCustomDataUpdated

    \Description
        Callback fired when the player custom data is updated

    \Input *pPlayer - the player whose custom data was updated

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerCustomDataUpdated(Blaze::GameManager::Player *pPlayer)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerCustomDataUpdated\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onPlayerStateChanged

    \Description
        Callback fired when the state of a player is changed

    \Input *pPlayer             - the player whose state has changed
    \Input ePreviousPlayerState - what the previous state was

    \Version 09/16/2019 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onPlayerStateChanged(Blaze::GameManager::Player *pPlayer, Blaze::GameManager::PlayerState ePreviousPlayerState)
{
    using namespace Blaze::GameManager;
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    GameServerClientT *pClient = &pBlazeServer->Game.aClients[pPlayer->getSlotId() - 1];
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onPlayerStateChanged\n");
    Game *pGame = pPlayer->getGame();

    /* this is the only way to detect that a player received a disconnect reservation. we set the flag here to handle the claiming of the reservation
       correctly (not adding the same player twice). */
    bool bDisconnectReservation = (pPlayer->isReserved() && ((ePreviousPlayerState == ACTIVE_CONNECTED) || (ePreviousPlayerState == ACTIVE_CONNECTING) ||
                                  (ePreviousPlayerState == ACTIVE_MIGRATING) || (ePreviousPlayerState == ACTIVE_KICK_PENDING)));

    // we are only concerned about disconnect reservations, ignore all other types of state changes
    if (!bDisconnectReservation)
    {
        return;
    }

    // if there are still players connected then mark this client as reserved in case they claim it later
    if (pGame->getActivePlayerCount() > 0)
    {
        DirtyCastLogPrintf("gameserverblaze: moving user %s at slot #%d to a RESERVED state with a disconnect reservation\n", pPlayer->getName(),
            pPlayer->getSlotId());
        pClient->bReserved = TRUE;
    }
    // otherwise, if there are no players left then eject ourselves, another host will get injected if a player claims their disconnect reservation
    else
    {
        DirtyCastLogPrintfVerbose(_GetDebugLevel(), 0, "gameserverblaze: ejecting ourselves from the game as no actively connected users remain\n");
        pGame->ejectHost(Game::EjectHostJobCb(this, &GameServerBlazeGameListenerC::HostEjected));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onHostMigrationStarted

    \Description
        Callback fired when host migration begins

    \Input *pGame   - the game whose host is migrated

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onHostMigrationStarted(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onHostMigrationStarted\n");
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onHostMigrationEnded

    \Description
        Callback fired when host migration ended

    \Input *pGame   - the game whose host was migrated

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onHostMigrationEnded(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onHostMigrationEnded\n");

    if (m_bNeedsReset)
    {
        if (pGame->getGameState() == Blaze::GameManager::IN_GAME)
        {
            DirtyCastLogPrintf("gameserverblaze: empty game, done migrating, ending it before returning to pool\n");

            pGame->endGame(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::GameEnded));
        }
        else
        {
            DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: returning to pool\n");

            pGame->returnDedicatedServerToPool(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::GameReset));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onUnresponsive

    \Description
        Callback fired when the game is marked as unresponsive

    \Input *pGame               - the game who is unresponsive
    \Input ePreviousGameState   - the previous game state we were in

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onUnresponsive(Blaze::GameManager::Game *pGame, Blaze::GameManager::GameState ePreviousGameState)
{
    DirtyCastLogPrintf("gameserverblaze: onUnresponsive. Prior game state was %s. Awaiting reconnection..\n", Blaze::GameManager::GameStateToString(ePreviousGameState));
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onBackFromUnresponsive

    \Description
        Callback fired when the game has recovered from being unresponsive

    \Input *pGame   - the game who is now responsive

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onBackFromUnresponsive(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintf("gameserverblaze: onBackFromUnresponsive. Restored state is %s. Current player count is %u (%u active). %s\n", Blaze::GameManager::GameStateToString(pGame->getGameState()), pGame->getPlayerCount(), pGame->getActivePlayerCount(), (m_bNeedsReset ? "Returning game to pool. " : ""));

    if (m_bNeedsReset)
    {
        if (pGame->getGameState() == Blaze::GameManager::IN_GAME)
        {
            DirtyCastLogPrintf("gameserverblaze: empty game, ending it before returning to pool\n");

            pGame->endGame(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::GameEnded));
        }
        else
        {
            DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: returning to pool\n");

            pGame->returnDedicatedServerToPool(Blaze::GameManager::Game::ChangeGameStateCb(this, &GameServerBlazeGameListenerC::GameReset));
        }
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGameListenerC::onNetworkCreated

    \Description
        Callback fired when the network is created for the game

    \Input *pGame   - the game whose network was created

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeGameListenerC::onNetworkCreated(Blaze::GameManager::Game *pGame)
{
    DirtyCastLogPrintfVerbose(_GetDebugLevel(), DIRTYCAST_DBGLVL_BLAZE, "gameserverblaze: onNetworkCreated\n");
}

/*** Public Functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function GameServerBlazeCreate

    \Description
        Sets up the global gameserverblaze state

    \Input *pConfig         - the gameserver configuration
    \Input *pCommandTags    - command tag buffer
    \Input *pConfigTags     - config tag buffer

    \Output
        int32_t             - zero=success, negative=failure

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t GameServerBlazeCreate(GameServerConfigT *pConfig, const char *pCommandTags, const char *pConfigTags)
{
    GameServerBlazeT *pBlazeServer;
    int32_t iMemGroup, iMsec;
    void *pMemGroupUserData;
    struct tm Now;

    // seed the rand number generator
    ds_plattimetotimems(&Now, &iMsec);
    srand(Now.tm_sec + iMsec);

    // query memgroup data
    DirtyMemGroupQuery(&iMemGroup, &pMemGroupUserData);

    // init the module state data
    if ((pBlazeServer = _pGameServerBlaze = (GameServerBlazeT *)DirtyMemAlloc(sizeof(*_pGameServerBlaze), GAMESERVERBLAZE_MEMID, iMemGroup, pMemGroupUserData)) == NULL)
    {
        DirtyCastLogPrintf("gameserverblaze: unable to allocate module state\n");
        return(-1);
    }
    ds_memclr(pBlazeServer, sizeof(*pBlazeServer));
    pBlazeServer->iMemGroup = iMemGroup;
    pBlazeServer->pMemGroupUserData = pMemGroupUserData;
    pBlazeServer->uLastConnectionErrorCode = '----';
    pBlazeServer->uLastConnectionErrorKind = '----';
    pBlazeServer->uLastConnectionChangeTime = NetTick();
    pBlazeServer->pConfig = pConfig;
    ds_strnzcpy(pBlazeServer->LinkParams.aCommandTags, pCommandTags, sizeof(pBlazeServer->LinkParams.aCommandTags));
    ds_strnzcpy(pBlazeServer->LinkParams.aConfigsTags, pConfigTags, sizeof(pBlazeServer->LinkParams.aConfigsTags));

    // create module to communicate with the subspace sidekick app
    if (pConfig->uSubspaceSidekickPort != 0)
    {
        if ((pBlazeServer->pSubspaceApi = SubspaceApiCreate(pConfig->strSubspaceSidekickAddr, pConfig->uSubspaceSidekickPort)) != NULL)
        {
            // kick off our first provision
            pBlazeServer->bDoSubspaceProvision = TRUE;
        }
        else
        {
            DirtyCastLogPrintf("gameserverblaze: unable to create module subspaceapi.\n");
            return(-2);
        }
    }

    // allocate internal blaze modules
    if ((pBlazeServer->pAdapter = CORE_NEW(EA::Allocator::ICoreAllocator::GetDefaultAllocator(), "GameServerBlazeNetworkAdapterC", EA::Allocator::MEM_PERM) GameServerBlazeNetworkAdapterC()) == NULL)
    {
        DirtyCastLogPrintf("gameserverblaze: unable to allocate network adapter\n");
        return(-3);
    }
    if ((pBlazeServer->pGameListener = CORE_NEW(EA::Allocator::ICoreAllocator::GetDefaultAllocator(), "GameServerBlazeGameListenerC", EA::Allocator::MEM_PERM) GameServerBlazeGameListenerC()) == NULL)
    {
        DirtyCastLogPrintf("gameserverblaze: unable to allocate game listener\n");
        return(-4);
    }
    if ((pBlazeServer->pStateEventHandler = CORE_NEW(EA::Allocator::ICoreAllocator::GetDefaultAllocator(), "GameServerStateEventHandlerC", EA::Allocator::MEM_PERM) GameServerStateEventHandlerC()) == NULL)
    {
        DirtyCastLogPrintf("gameserverblaze: unable to allocate state event handler\n");
        return(-5);
    }
    if ((pBlazeServer->pGameManagerListener = CORE_NEW(EA::Allocator::ICoreAllocator::GetDefaultAllocator(), "GameManagerListenerC", EA::Allocator::MEM_PERM) GameManagerListenerC()) == NULL)
    {
        DirtyCastLogPrintf("gameserverblaze: unable to allocate gamemanager listener\n");
        return(-6);
    }

    // override assert callback (we do not want our servers intentially crashing themselves)
    EA::Assert::SetFailureCallback(&_EAAssertFailure);
    Blaze::Debug::SetAssertHook(&_BlazeAssertFunction, NULL);
    // disable log BlazeSDK timestamp
    Blaze::Debug::disableBlazeLogTimeStamp();

    // set connect delays
    pBlazeServer->iConnectDelay = (rand() % DEFAULT_CONNECT_DELAY) + DEFAULT_CONNECT_DELAY;
    DirtyCastLogPrintf("gameserverblaze: using a connect delay of %d ms\n", pBlazeServer->iConnectDelay);

    return(0);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeRelease

    \Description
        Cleans up the global gameserverblaze state

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeRelease(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    // if module state is null, nothing left to do
    if (pBlazeServer == NULL)
    {
        return;
    }

    // disconnect to cleanup
    GameServerBlazeDisconnect();

    if (pBlazeServer->pSubspaceApi != NULL)
    {
        // todo make this non-blocking
        pBlazeServer->bDoSubspaceStop = TRUE;
        while (pBlazeServer->bDoSubspaceStop == TRUE)
        {
            // service subspace api
            SubspaceApiUpdate(pBlazeServer->pSubspaceApi);
            
            // attempt to release subspace resources (will modify pBlazeServer->bDoSubspaceStop)
            _SubspaceRequestUpdate();

            // give a bit of time before the next update _SubspaceRequestUpdate
            NetConnSleep(10);
        }
        SubspaceApiDestroy(pBlazeServer->pSubspaceApi);
        pBlazeServer->pSubspaceApi = NULL;
    }

    CORE_DELETE(EA::Allocator::ICoreAllocator::GetDefaultAllocator(), pBlazeServer->pGameManagerListener);
    pBlazeServer->pGameManagerListener = NULL;

    CORE_DELETE(EA::Allocator::ICoreAllocator::GetDefaultAllocator(), pBlazeServer->pStateEventHandler);
    pBlazeServer->pStateEventHandler = NULL;

    CORE_DELETE(EA::Allocator::ICoreAllocator::GetDefaultAllocator(), pBlazeServer->pGameListener);
    pBlazeServer->pGameListener = NULL;

    CORE_DELETE(EA::Allocator::ICoreAllocator::GetDefaultAllocator(), pBlazeServer->pAdapter);
    pBlazeServer->pAdapter = NULL;

    DirtyMemFree(pBlazeServer, GAMESERVERBLAZE_MEMID, pBlazeServer->iMemGroup, pBlazeServer->pMemGroupUserData);
    _pGameServerBlaze = pBlazeServer = NULL;
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeUpdate

    \Description
        Gives the module some processing time

    \Output
        uint8_t - TRUE=disconnected, FALSE=online

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
uint8_t GameServerBlazeUpdate(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    uint8_t bDisconnected = FALSE;
    uint8_t uDisconnectedState, uTimeIsOver, uTimeIsWrapped;
    uint32_t uNow;

    GameServerBlazeStateE eOldState = pBlazeServer->eState;

    pBlazeServer->pBlazeHub->idle();
    pBlazeServer->pGameListener->Idle();

    uDisconnectedState = (pBlazeServer->eState < kConnected);
    uNow = NetTick();
    uTimeIsOver = (NetTickDiff(uNow, pBlazeServer->uLastConnectionChangeTime) > pBlazeServer->iConnectDelay);
    uTimeIsWrapped = (NetTickDiff(uNow, pBlazeServer->uLastConnectionChangeTime) < 0);

    if (pBlazeServer->pSubspaceApi != NULL)
    {
        SubspaceApiUpdate(pBlazeServer->pSubspaceApi);
        if ((pBlazeServer->bDoSubspaceProvision == FALSE) &&    // we aren't already doing this
            (NetTickDiff(uNow, pBlazeServer->uLastSubspaceProvisionTick) > (int32_t)pBlazeServer->pConfig->uSubspaceProvisionRateMin) &&   // too much time has passed
            ((pBlazeServer->pGame == NULL) || pBlazeServer->pGame->getPlayerCount() == 0))  // a game isn't currently active
        {
            pBlazeServer->bDoSubspaceProvision = TRUE;
            DirtyCastLogPrintf("gameserverblaze: destroying game for subspace provision, since too much time has passed.\n");
        }
    }

    // if we want to go to zombie state and we currently aren't in use, disconnect and go idle
    if (pBlazeServer->bZombie)
    {
        if (((pBlazeServer->pGame == NULL) || pBlazeServer->pGame->getPlayerCount() == 0) && (pBlazeServer->eState != kDisconnected))
        {
            // we are supposed to disconnect and stay disconnected
            pBlazeServer->pBlazeHub->getConnectionManager()->disconnect();
            bDisconnected = TRUE;
            pBlazeServer->uLastConnectionErrorKind = 'ZOMB';
            pBlazeServer->uLastConnectionErrorCode = GAMESERVERBLAZE_ERROR_ZOMBIE;
        }
        // nothing to do, we are idle
    }
    // we must re-provision with subspace; destroy the game, provision, create a new game 
    else if (pBlazeServer->bDoSubspaceProvision)
    {
        if ((pBlazeServer->pGame != NULL) && (pBlazeServer->pGame->getPlayerCount() > 0))
        {
            DirtyCastLogPrintf("gameserverblaze: error, can't subspace provision, players are in game.\n");
            return(FALSE);
        }

        if (pBlazeServer->pGame != NULL)
        {
            if (pBlazeServer->bSubspaceStartedGameDestroy == FALSE)
            {
                // if there is a game we need to destroy it so we can update the ip/port info
                _ResetGame(pBlazeServer, pBlazeServer->pGame, GAMESERVER_EVENT_DELETE);
                GameServerBlazeDestroyGame();
                pBlazeServer->bSubspaceStartedGameDestroy = TRUE;
            }
        }
        else if (_SubspaceRequestUpdate() != 0)
        {
            // we are done, create the new game to advertise our new ip/port to blaze
            GameServerBlazeCreateGame();
            pBlazeServer->bSubspaceStartedGameDestroy = FALSE;
        }
    }
    /* --> the GameServerBlazeStatus('accs', 0, NULL, 0) is there to avoid attempting to reconnect to blaze while the
           gameserver layer is in the middle of re-acquiring a token from the voipserver process. Without this check,
           there is a possibility for the very recently acquired token (refreshed by the gameserver layer 
           using GameServerBlazeStatus('accs') upon reception of the voipserver response) to be invalidated by the below
           calls to pBlazeHub->getConnectionManager()->disconnect() and eventually resulting in a blaze login failure.
       --> the uTimeIsWrapped check is there for potential cases where we get stuck, more than 25 days after the last
           connection change, for some unspecified reason like game state hangs (HM, etc...)
           in this case, we can immediately reconnect. */
    else if ( GameServerBlazeStatus('accs', 0, NULL, 0) && 
              ((uDisconnectedState && uTimeIsOver) ||
              (pBlazeServer->pGameListener->IsStuck() && (uTimeIsOver || uTimeIsWrapped)))
            )
    {
        DirtyCastLogPrintf("gameserverblaze: attempting to reconnect to Blaze (tick diff : %d, iConnectDelay : %d, IsStuck() : %d, uTimeIsWrapped : %d)\n",
            NetTickDiff(uNow, pBlazeServer->uLastConnectionChangeTime), pBlazeServer->iConnectDelay, pBlazeServer->pGameListener->IsStuck(), uTimeIsWrapped);

        bDisconnected = TRUE;
        pBlazeServer->uLastConnectionErrorCode = GAMESERVERBLAZE_ERROR_STUCK;

        // if we're reconnecting because we're stuck, throw a game creation failure to reset state
        if (pBlazeServer->pGameListener->IsStuck())
        {
            pBlazeServer->uLastConnectionErrorKind = 'CREA';
            pBlazeServer->pCallback(pBlazeServer->pCallbackData, GAMESERVER_EVENT_GAME_CREATION_FAILURE, NULL);
        }
        else
        {
            pBlazeServer->uLastConnectionErrorKind = 'POOL';
        }

        pBlazeServer->uLastConnectionChangeTime = NetTick();

        /* clear the token to force the gameserver layer to request a new token from the voipsever and, upon reception of that token, 
           reexecute the GameServerBlazeDisconnect()/GameServerBlazeLogin() sequence */
        ds_memclr(pBlazeServer->strAccessToken, sizeof(pBlazeServer->strAccessToken));
    }
    if ((eOldState != kDisconnected) && (pBlazeServer->eState == kDisconnected))
    {
        bDisconnected = TRUE;
    }
    return(bDisconnected);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeStatus

    \Description
        Gets status information

    \Input iSelect  - info selector (see Notes)
    \Input iValue   - selector specific
    \Input *pBuf    - storage for selector specific output
    \Input iBufSize - size of buffer

    \Output
        int32_t     - selector specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'accs'      returns TRUE if the access token is valid, FALSE otherwise
        'bhub'      returns TRUE if the blazehub is valid
        'lgam'      returns the cache from the last game via pBuf
        'lcde'      returns the last connection error code
        'lknd'      returns the last connection error kind
        'nusr'      returns the number of clients in the current game
        'stat'      returns the current state of the module (GameServerBlazeStateE)
        'sspr'      returns TRUE if we are currently trying to provision with subspace
        'ssst'      returns subspace status; 0 off, 1 provisioned, 2 not-provisioned
        'zomb'      returns if the game server is attmpting to reach zombie state (disconnected from blaze and idle)
    \endverbatim

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t GameServerBlazeStatus(int32_t iSelect, int32_t iValue, void *pBuf, int32_t iBufSize)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    // returns if access token is valid
    if (iSelect == 'accs')
    {
        return(pBlazeServer->strAccessToken[0] != '\0');
    }
    // returns if blaze hub is valid
    if (iSelect == 'bhub')
    {
        return(pBlazeServer->pBlazeHub != NULL);
    }
    // returns last game
    if (iSelect == 'lgam')
    {
        if (iBufSize >= (int32_t)sizeof(pBlazeServer->LastGame))
        {
            ds_memcpy(pBuf, &pBlazeServer->LastGame, iBufSize);
            return(0);
        }
    }
    // returns last connection error code
    if (iSelect == 'lcde')
    {
        return(pBlazeServer->uLastConnectionErrorCode);
    }
    // returns last connection error kind
    if (iSelect == 'lknd')
    {
        return(pBlazeServer->uLastConnectionErrorKind);
    }
    // returns number of clients in the game
    if (iSelect == 'nusr')
    {
        return(pBlazeServer->Game.iCount);
    }
    // returns the module status
    if (iSelect == 'stat')
    {
        return(pBlazeServer->eState);
    }
    // returns TRUE if we are currently trying to provision with subspace
    if (iSelect == 'sspr')
    {
        return(pBlazeServer->bDoSubspaceProvision);
    }
    // returns subspace status, 0 off, 1 provisioned, 2 not-provisioned
    if (iSelect == 'ssst')
    {
        int32_t iStatus = 2;    // 2 not-provisioned (default)

        // if we aren't using the sidekick then subspace is off
        if (pBlazeServer->pConfig->uSubspaceSidekickPort == 0)
        {
            iStatus = 0;        // 0 off
        }
        // see if our address values have been altered, if they have its because we have provisioned
        else if ((pBlazeServer->LinkParams.uAddr != pBlazeServer->uLocalServerIp) || (pBlazeServer->LinkParams.uPort != pBlazeServer->uLocalServerPort))
        {
            iStatus = 1;        // 1 provisioned
        }

        return(iStatus);
    }
    // returns if we are going to, or in, the zombie state
    if (iSelect == 'zomb')
    {
        return(pBlazeServer->bZombie);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeControl

    \Description
        Control function. Different selectors control different behaviors

    \Input iControl - control selector (see Notes)
    \Input iValue   - control specific
    \Input iValue2  - control specific
    \Input *pValue  - control specific

    \Output
        int32_t     - control specific

    \Notes
        Selectors are:

    \verbatim
        SELECTOR    RETURN RESULT
        'accs'      sets the access token via pValue
        'addr'      sets the server address via iValue
        'port'      sets the server port via iValue
        'surl'      sets the status url via pValue
        'zomb'      sets the game server to reach zombie state (disconnected from blaze and idle)
    \endverbatim

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
int32_t GameServerBlazeControl(int32_t iControl, int32_t iValue, int32_t iValue2, void *pValue)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    // set the access token
    if (iControl == 'accs')
    {
        ds_snzprintf(pBlazeServer->strAccessToken, sizeof(pBlazeServer->strAccessToken), "NEXUS_S2S %s", (const char *)pValue);
        return(0);
    }
    // set the address for the server
    if (iControl == 'addr')
    {
        pBlazeServer->LinkParams.uAddr = (uint32_t)iValue;

        // the fist time this is called we can cache the uFrontAddr, but if we are using subspace we may end up changing LinkParams.uAddr often
        if (pBlazeServer->uLocalServerIp == 0)
        {
            pBlazeServer->uLocalServerIp = (uint32_t)iValue;
        }
        return(0);
    }
    // set the port for the server
    if (iControl == 'port')
    {
        pBlazeServer->LinkParams.uPort = (uint16_t)iValue;

        // the fist time this is called we can cache the uTunnelPort, but if we are using subspace we may end up changing LinkParams.uPort often
        if (pBlazeServer->uLocalServerPort == 0)
        {
            pBlazeServer->uLocalServerPort = (uint16_t)iValue;
        }
        return(0);
    }
    // set the status url
    if (iControl == 'surl')
    {
        ds_strnzcpy(pBlazeServer->strStatusUrl, (const char *)pValue, sizeof(pBlazeServer->strStatusUrl));
        return(0);
    }
    // set that we intend to go to the zombie state
    if (iControl == 'zomb')
    {
        pBlazeServer->bZombie = (uint8_t)iValue;
        return(0);
    }
    // unhandled
    return(-1);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeCallback

    \Description
        Sets the various callbacks fired by gameserverblaze

    \Input *pCallback   - the gameserverblaze callback
    \Input *pSend       - the gameserverlink send callback
    \Input *pDisc       - the gameserverlink disconnect callback
    \Input *pEvent      - the gameserverlink events callback
    \Input *pCallbackData- the userdata sent with the callback

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeCallback(GameServerBlazeCallbackT *pCallback, GameServerLinkSendT *pSend, GameServerLinkDiscT *pDisc, GameServerLinkClientEventT *pEvent, void *pCallbackData)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    pBlazeServer->pCallback = pCallback;
    pBlazeServer->LinkParams.pUserData = pBlazeServer->pCallbackData = pCallbackData;
    pBlazeServer->LinkParams.pSendFunc = pSend;
    pBlazeServer->LinkParams.pDisFunc = pDisc;
    pBlazeServer->LinkParams.pEventFunc = pEvent;
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeLogin

    \Description
        Sets the login process to the blazeserver

    \Output
        uint8_t - TRUE=success, FALSE=failure

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
uint8_t GameServerBlazeLogin(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    const GameServerConfigT *pConfig = pBlazeServer->pConfig;
    Blaze::BlazeHub::InitParameters InitParams;
    char *pCert = NULL, *pKey = NULL;

    ds_strnzcpy(InitParams.ServiceName, pConfig->strServer, sizeof(InitParams.ServiceName));
    ds_strnzcpy(InitParams.AdditionalNetConnParams, "", sizeof(InitParams.AdditionalNetConnParams));
    ds_strnzcpy(InitParams.ClientName, "Dirtycast Server", sizeof(InitParams.ClientName));
    ds_strnzcpy(InitParams.ClientVersion, "7.0.0", sizeof(InitParams.ClientVersion));
    ds_strnzcpy(InitParams.ClientSkuId, "SKU", sizeof(InitParams.ClientSkuId));
    if (pConfig->strRdirName[0] != '\0')
    {
        DirtyCastLogPrintf("gameserverblaze: setting redirector override to %s\n", pConfig->strRdirName);
        ds_strnzcpy(InitParams.Override.RedirectorAddress, pConfig->strRdirName, sizeof(InitParams.Override.RedirectorAddress));
    }

    InitParams.Secure                   = pConfig->bSecure == TRUE;
    InitParams.UserCount                = 1;
    InitParams.OutgoingBufferSize       = 65536;
    InitParams.MaxIncomingPacketSize    = 262144;
    InitParams.Locale                   = 'enUS';
    InitParams.Client                   = Blaze::CLIENT_TYPE_DEDICATED_SERVER;
    InitParams.EnableQos                = false;

    DirtyCastLogPrintf("gameserverblaze: m_pConfig->strEnvironment is %s\n", pConfig->strEnvironment);

    if (ds_stristr(pConfig->strEnvironment, "prod"))
    {
        DirtyCastLogPrintf("gameserverblaze: using the prod env\n");
        InitParams.Environment = Blaze::ENVIRONMENT_PROD;
    }
    else if (ds_stristr(pConfig->strEnvironment, "test"))
    {
        DirtyCastLogPrintf("gameserverblaze: using the stest env\n");
        InitParams.Environment = Blaze::ENVIRONMENT_STEST;
    }
    else if (ds_stristr(pConfig->strEnvironment, "cert"))
    {
        DirtyCastLogPrintf("gameserverblaze: using the scert env\n");
        InitParams.Environment = Blaze::ENVIRONMENT_SCERT;
    }
    else if (ds_stristr(pConfig->strEnvironment, "dev"))
    {
        DirtyCastLogPrintf("gameserverblaze: using the sdev env\n");
        InitParams.Environment = Blaze::ENVIRONMENT_SDEV;
    }
    else
    {
        DirtyCastLogPrintf("gameserverblaze: environment unset, defaulting to prod env\n");
        InitParams.Environment = Blaze::ENVIRONMENT_PROD;
    }

    // if cert has been provided load it and pass it to blaze
    if ((pConfig->strCertFile[0] != '\0') && (pConfig->strKeyFile[0] != '\0'))
    {
        pCert = ZFileLoad(pConfig->strCertFile, NULL, FALSE);
        pKey = ZFileLoad(pConfig->strKeyFile, NULL, FALSE);
        DirtyCastLogPrintfVerbose(pConfig->uDebugLevel, 1, "gameserverblaze: cert status=%s, key status=%s\n",
            (pCert != NULL) ? "loaded" : "failed", (pKey != NULL) ? "loaded" : "failed");
    }

    // mTLS was added in Shrike.1, previously we did not pass in a cert/key
    #if BLAZE_SDK_VERSION > BLAZE_SDK_MAKE_VERSION(15, 1, 1, 8, 0)
    Blaze::BlazeError iError = Blaze::BlazeHub::initialize(&pBlazeServer->pBlazeHub, InitParams, EA::Allocator::ICoreAllocator::GetDefaultAllocator(), _LoggingFunction, pBlazeServer, pCert, pKey);
    #else
    Blaze::BlazeError iError = Blaze::BlazeHub::initialize(&pBlazeServer->pBlazeHub, InitParams, EA::Allocator::ICoreAllocator::GetDefaultAllocator(), _LoggingFunction, pBlazeServer);
    #endif

    // clean up the memory
    ZMemFree(pCert);
    ZMemFree(pKey);

    if((iError != Blaze::ERR_OK) || (pBlazeServer->pBlazeHub == NULL))
    {
        // this is fatal.
        DirtyCastLogPrintf("gameserverblaze: BlazeHub::initialize failed with error %d, BlazeHub is 0x%08x\n", iError, pBlazeServer->pBlazeHub);
        return(FALSE);
    }
    else
    {
        // We want our BlazeAppStateListener object to be notified of states changes within Blaze.
        pBlazeServer->pBlazeHub->getLoginManager(0)->addListener(pBlazeServer->pStateEventHandler);
        pBlazeServer->pBlazeHub->addUserStateEventHandler(pBlazeServer->pStateEventHandler);
        pBlazeServer->pBlazeHub->getConnectionManager()->connect(&_OnRedirectorMessages);
        _MoveToNextState(pBlazeServer, kConnecting);

        pBlazeServer->pAdapter->SetConfig(pConfig, &pBlazeServer->LinkParams);
        Blaze::GameManager::GameManagerAPI::GameManagerApiParams Params(pBlazeServer->pAdapter);

        // since we have just set the ip information, we need to give subspace another chance to provision if needed
        if (pBlazeServer->pSubspaceApi != NULL)
        {
            DirtyCastLogPrintf("gameserverblaze: subspace will provision, since the VS has given us new ip information.\n");
            pBlazeServer->bDoSubspaceProvision = TRUE;
        }

        if (strcmp(pConfig->strGameProtocolVersion, "") == 0)
        {
            Params.mGameProtocolVersionString = Blaze::GameManager::GAME_PROTOCOL_VERSION_MATCH_ANY;
        }
        else
        {
            Params.mGameProtocolVersionString = pConfig->strGameProtocolVersion;
        }

        Blaze::GameManager::GameManagerAPI::createAPI(*pBlazeServer->pBlazeHub, Params);
        pBlazeServer->pBlazeHub->getGameManagerAPI()->addListener(pBlazeServer->pGameManagerListener);
    }

    return(TRUE);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeDisconnect

    \Description
        Disconnects from blaze and cleans up the hub

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeDisconnect(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    if (pBlazeServer->pBlazeHub == NULL)
    {
        return;
    }

    // remove the listeners
    pBlazeServer->pBlazeHub->getGameManagerAPI()->removeListener(pBlazeServer->pGameManagerListener);
    pBlazeServer->pBlazeHub->getConnectionManager()->disconnect();
    pBlazeServer->pBlazeHub->removeUserStateEventHandler(pBlazeServer->pStateEventHandler);
    pBlazeServer->pBlazeHub->getLoginManager(0)->removeListener(pBlazeServer->pStateEventHandler);

    // shutdown the blazehub and clear out the pointer
    Blaze::BlazeHub::shutdown(pBlazeServer->pBlazeHub);
    pBlazeServer->pBlazeHub = NULL;
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeCreateGame

    \Description
        Starts the creation of the game

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeCreateGame(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    GameServerConfigT *pConfig = pBlazeServer->pConfig;
    uint8_t bUsingSubspace = FALSE, uCustomAttr;

    // now we have all the pieces, lets put the CreateGameParameters together and create the game
    Blaze::GameManager::TemplateAttributes Attributes;

    // indidcate if this server is using subspace
    if (pBlazeServer->pSubspaceApi != NULL)
    {
        pBlazeServer->pAdapter->SetConfig(pConfig, &pBlazeServer->LinkParams); // so a new addr/port can be applied
        bUsingSubspace = (GameServerBlazeStatus('ssst', 0, NULL, 0) == 1);
    }
    if (EA::TDF::GenericTdfType* pSubspace = Attributes.allocate_element())
    {
        pSubspace->set(bUsingSubspace ? "true" : "false");
        Attributes["SUBSPACE"] = pSubspace;
    }

    // set up game to support max player count
    if (EA::TDF::GenericTdfType *pMaxPlayers = Attributes.allocate_element())
    {
        pMaxPlayers->set(pConfig->uMaxClients + pConfig->uMaxSpectators);
        Attributes["PLAYER_CAPACITY"] = pMaxPlayers;
    }
    if (EA::TDF::GenericTdfType *pMaxClients = Attributes.allocate_element())
    {
        pMaxClients->set(pConfig->uMaxClients);
        Attributes["PARTICIPANT_CAPACITY"] = pMaxClients;
    }
    if (EA::TDF::GenericTdfType *pMaxSpectators = Attributes.allocate_element())
    {
        pMaxSpectators->set(pConfig->uMaxSpectators);
        Attributes["SPECTATOR_CAPACITY"] = pMaxSpectators;
    }

    if (EA::TDF::GenericTdfType *pPingSite = Attributes.allocate_element())
    {
        pPingSite->set(pConfig->strPingSite);
        Attributes["PING_SITE_ALIAS"] = pPingSite;
    }

    if (EA::TDF::GenericTdfType *pGameStatusUrl = Attributes.allocate_element())
    {
        pGameStatusUrl->set(pBlazeServer->strStatusUrl);
        Attributes["GAME_STATUS_URL"] = pGameStatusUrl;
    }

    if (EA::TDF::GenericTdfType *pFixedRate = Attributes.allocate_element())
    {
        pFixedRate->set(pConfig->uFixedUpdateRate);
        Attributes["FIXED_RATE"] = pFixedRate;
    }

    // add any custom attributes the gameserver was configured with
    for (uCustomAttr = 0; uCustomAttr < pConfig->uNumAttributes; uCustomAttr += 1)
    {
        if (EA::TDF::GenericTdfType *pCustomAttr = Attributes.allocate_element())
        {
            pCustomAttr->set(pConfig->aCustomAttributes[uCustomAttr].strValue);
            Attributes[pConfig->aCustomAttributes[uCustomAttr].strKey] = pCustomAttr;
        }
    }

    pBlazeServer->pBlazeHub->getGameManagerAPI()->createGameFromTemplate("dirtyCastGame", Attributes, Blaze::GameManager::PlayerJoinDataHelper(), Blaze::GameManager::GameManagerAPI::JoinGameCb(pBlazeServer->pGameListener, &GameServerBlazeGameListenerC::GameCreated));
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeDestroyGame

    \Description
        Starts the destruction of the game

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeDestroyGame(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    if (pBlazeServer->pGame != NULL)
    {
        pBlazeServer->pGame->destroyGame(Blaze::GameManager::HOST_LEAVING, Blaze::GameManager::Game::DestroyGameCb(pBlazeServer->pGameListener, &GameServerBlazeGameListenerC::GameDestroyed));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeKickClient

    \Description
        Attempts to kick a client from the game (if possible)

    \Input iClientId    - the client we are kicking
    \Input uReason      - the reason we are kicking

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
void GameServerBlazeKickClient(int32_t iClientId, uint16_t uReason)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    uint16_t uIndex;

    // if game is null nothing left to do
    if (pBlazeServer->pGame == NULL)
    {
        return;
    }

    // loop through the members of the game to find the client
    for (uIndex = 0; uIndex < pBlazeServer->pGame->getActivePlayerCount(); uIndex += 1)
    {
        const Blaze::GameManager::Player *pPlayer = pBlazeServer->pGame->getActivePlayerByIndex(uIndex);
        const Blaze::MeshEndpoint *pMeshEndpoint = pPlayer->getMeshEndpoint();

        // check if the player matches the client
        if (pMeshEndpoint->getRemoteLowLevelConnectivityId() != static_cast<uint32_t>(iClientId))
        {
            continue;
        }

        // we only need to kick a single user from the game because Blaze's behavior is to kick all local players when any other local player is kicked
        DirtyCastLogPrintf("gameserverblaze: kicking client 0x%08x with reason %d (%s)\n", iClientId, (int16_t)uReason, _GetKickReasonString(uReason));
        pBlazeServer->pGame->kickPlayer(pPlayer, Blaze::GameManager::Game::KickPlayerCb(pBlazeServer->pGameListener, &GameServerBlazeGameListenerC::PlayerKicked), false, uReason, _GetKickReasonString(uReason), Blaze::GameManager::Game::KICK_UNRESPONSIVE_PEER);
        break;
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGetLastRemovalReason

    \Description
        Retrieves the last removal reason and game state

    \Input *pBuffer     - [out] the last removal / last kick reason as a string
    \Input iBufSize     - size of the buffer
    \Input *pGameState  - [out] the last game state

    \Output
        uint32_t        - the last removal reason and last kick reason encoded in a uint32_t

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
uint32_t GameServerBlazeGetLastRemovalReason(char *pBuffer, int32_t iBufSize, uint32_t *pGameState)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    if (pBuffer != NULL)
    {
        if (pBlazeServer->uLastRemovedReason != Blaze::GameManager::PLAYER_KICKED)
        {
            ds_strnzcpy(pBuffer, Blaze::GameManager::PlayerRemovedReasonToString((Blaze::GameManager::PlayerRemovedReason)pBlazeServer->uLastRemovedReason), iBufSize);
        }
        else
        {
            ds_snzprintf(pBuffer, iBufSize, "PLAYER_KICKED/%s", _GetKickReasonString(pBlazeServer->iLastKickReason));
        }
    }
    if (pGameState != NULL)
    {
        *pGameState = pBlazeServer->uLastGameState;
    }
    return(((uint32_t)pBlazeServer->uLastRemovedReason << 16) | (((uint32_t)pBlazeServer->iLastKickReason) & 0xffff));
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGetErrorCodeName

    \Description
        Converts the error code to a string

    \Input uErrorCode   - the error code we are converting

    \Output
        const char *    - the error code as a string

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
const char *GameServerBlazeGetErrorCodeName(uint32_t uErrorCode)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;

    switch(uErrorCode)
    {
        // special DirtyCast errors -- these should probably be registered as a custom component instead
        case GAMESERVERBLAZE_ERROR_STUCK: return("GAMESERVERBLAZE_ERROR_STUCK");
        // "normal" processing - pass the error to Blaze to get the name back
        default: return(pBlazeServer->pBlazeHub->getErrorName((Blaze::BlazeError)uErrorCode));
    }
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGetGameServerLink

    \Description
        Retrieves the gameserverlink module ref used by the game

    \Output
        GameServerLinkT *   - gameserverlink module pointer or NULL if not available

    \Version 03/31/2017 (eesponda)
*/
/********************************************************************************F*/
GameServerLinkT *GameServerBlazeGetGameServerLink(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    return(pBlazeServer->pAdapter != NULL ? pBlazeServer->pAdapter->GetGameServerLink() : NULL);
}

/*F********************************************************************************/
/*!
    \Function GameServerBlazeGetGameServerDist

    \Description
        Retrieves the gameserverdist module ref used by the game

    \Output
        GameServerDistT *   - gameserverdist module pointer or NULL if not available

    \Version 07/31/2019 (tcho)
*/
/********************************************************************************F*/
GameServerDistT *GameServerBlazeGetGameServerDist(void)
{
    GameServerBlazeT *pBlazeServer = _pGameServerBlaze;
    return(pBlazeServer->pAdapter != NULL ? pBlazeServer->pAdapter->GetGameServerDist() : NULL);
}
