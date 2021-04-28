/*! ************************************************************************************************/
/*!
    \file gamemanagerapi.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/blazehub.h"
#include "BlazeSDK/loginmanager/loginmanager.h"
#include "BlazeSDK/usermanager/usermanager.h"
#include "BlazeSDK/component/gamereportingcomponent.h"
#include "BlazeSDK/connectionmanager/connectionmanager.h"
#include "BlazeSDK/blazesdkdefs.h"

#include "BlazeSDK/gamemanager/playerjoindatahelper.h"
#include "BlazeSDK/gamemanager/gamemanagerapi.h"
#include "BlazeSDK/gamemanager/gamemanagerapijobs.h"
#include "BlazeSDK/gamemanager/player.h"
#include "BlazeSDK/gamemanager/playerjoindatahelper.h"
#include "BlazeSDK/gamemanager/gameendpoint.h"
#include "BlazeSDK/gamemanager/primarysessionforuserhelper.h"
#include "BlazeSDK/shared/gamemanager/externalsessionbinarydatashared.h"

#include "DirtySDK/dirtysock/dirtynames.h"

#include "EASTL/algorithm.h"

#ifdef USE_TELEMETRY_SDK
#include "BlazeSDK/util/telemetryapi.h"
#endif

#if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
#include <np/np_common.h>//for defines needed with:
#include <np/np_webapi2_push_event.h>//for SCE_NP_WEBAPI2_PUSH_EVENT_UUID_LENGTH in updateGamePresenceForLocalUser()
#endif

namespace Blaze
{
namespace GameManager
{
    const uint32_t INVALID_USER_INDEX = static_cast<uint32_t> (-1); // Is there a better/centralized place to move it?

    /*! ****************************************************************************/
    /*! \brief Static factory method to create the GM API instance on the given BlazeHub 
    ********************************************************************************/
    void GameManagerAPI::createAPI(BlazeHub &hub, const GameManagerApiParams &params, 
            EA::Allocator::ICoreAllocator* gameManagerAllocator/* = nullptr*/, EA::Allocator::ICoreAllocator* gameBrowserAllocator /* = nullptr*/)
    {
        BlazeAssert(params.mNetworkAdapter != nullptr);

        if (hub.getGameManagerAPI() != nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Warning: Ignoring attempt to recreate GameManagerApi.\n");
            return;
        }
        
        // init specific allocators (if supplied)
        if (Blaze::Allocator::getAllocator(MEM_GROUP_GAMEMANAGER) == nullptr)
            Blaze::Allocator::setAllocator(MEM_GROUP_GAMEMANAGER, gameManagerAllocator!=nullptr? gameManagerAllocator : Blaze::Allocator::getAllocator());
        if (Blaze::Allocator::getAllocator(MEM_GROUP_GAMEBROWSER) == nullptr)
            Blaze::Allocator::setAllocator(MEM_GROUP_GAMEBROWSER, gameBrowserAllocator!=nullptr? gameBrowserAllocator : Blaze::Allocator::getAllocator(MEM_GROUP_GAMEMANAGER));
        
        // create dependent components
        GameManagerComponent::createComponent(&hub);
        GameReporting::GameReportingComponent::createComponent(&hub);

        GameManagerAPI *gameManagerApi = BLAZE_NEW(MEM_GROUP_GAMEMANAGER, "GameManagerAPI::createAPI") GameManagerAPI(hub, params, MEM_GROUP_GAMEMANAGER);
        hub.createAPI(GAMEMANAGER_API, gameManagerApi);
    }

    /*! ************************************************************************************************/
    /*! \brief ctor for GM Api param struct.
    ***************************************************************************************************/
    GameManagerAPI::GameManagerApiParams::GameManagerApiParams(BlazeNetworkAdapter::NetworkMeshAdapter *networkAdapter, 
                    uint32_t maxMatchmakingScenarios/*=0*/,
                    uint32_t maxGameBrowserLists /*=0*/,
                    uint32_t maxGameBrowserGamesInList /*=0*/,
                    uint32_t maxGameManagerGames/*=0*/,
                    const char8_t* gameProtocolVersion/*=GAME_PROTOCOL_DEFAULT_VERSION*/,
                    MemoryGroupId memGroupId,
                    bool preventMultipleGameMembership/* = true*/
            )
    : mNetworkAdapter(networkAdapter),
        mMaxGameBrowserLists(maxGameBrowserLists),
        mMaxGameBrowserGamesInList(maxGameBrowserGamesInList), mMaxGameManagerGames(maxGameManagerGames),
        mGameProtocolVersionString(nullptr, memGroupId), mPreventMultipleGameMembership(preventMultipleGameMembership),
        mMaxMatchmakingScenarios(maxMatchmakingScenarios)
    {
        mGameProtocolVersionString.set(gameProtocolVersion);
    }

    /*! ************************************************************************************************/
    /*! \brief log the GM Api Params
    ***************************************************************************************************/
    void GameManagerAPI::GameManagerApiParams::logParameters() const
    {
        BLAZE_SDK_DEBUGF("[GMGR] GameManagerAPI Parameters:\n");        
        BLAZE_SDK_DEBUGF("              NetworkAdapter Version: \"%s\"\n", mNetworkAdapter->getVersionInfo());
        BLAZE_SDK_DEBUGF("                GameProtocolVersionString: \"%s\"\n", mGameProtocolVersionString.c_str());
        BLAZE_SDK_DEBUGF("\n");
        BLAZE_SDK_DEBUGF("           MemoryPool Reservations := (initial memory reservations - 0 means unbounded)\n");
        BLAZE_SDK_DEBUGF("                          Max Games: %u\n", mMaxGameManagerGames);
        BLAZE_SDK_DEBUGF("          Max Matchmaking Scenarios: %u\n", mMaxMatchmakingScenarios);
        BLAZE_SDK_DEBUGF("              Max GameBrowser Lists: %u\n", mMaxGameBrowserLists);
        BLAZE_SDK_DEBUGF("    Max GameBrowserGames (per list): %u\n", mMaxGameBrowserGamesInList);
        BLAZE_SDK_DEBUGF("    Preventing Multiple Game Membership: %s\n", mPreventMultipleGameMembership ? "true" : "false");
        BLAZE_SDK_DEBUG("\n");
    }

    /*! ****************************************************************************/
    /*! \brief GameManagerAPI 
    
         \param[in] blazeHub pointer to Blaze Hub
         \param[in] params   init params 
         \param[in] memGroupId       mem group to be used by this class to allocate memory
    ********************************************************************************/
    GameManagerAPI::GameManagerAPI(BlazeHub &blazeHub, const GameManagerApiParams &params, MemoryGroupId memGroupId) :
        SingletonAPI(blazeHub),
        mApiParams(params),
        mUserToGameToJobVector(memGroupId, blazeHub.getNumUsers(), MEM_NAME(memGroupId, "GameManagerAPI::mUserToGameToJobMap")),
        mGameMap(memGroupId, MEM_NAME(memGroupId, "GameManagerAPI::mGameMap")),
        mGameMemoryPool(memGroupId),
        mMatchmakingScenarioMemoryPool(memGroupId),
        mGameBrowserMemoryPool(memGroupId),
        mGameToJobMemoryPool(memGroupId),
        mMatchmakingScenarioList(memGroupId, MEM_NAME(memGroupId, "GameManagerAPI::mMatchmakingScenarioList")),
        mGameBrowserListByClientIdMap(memGroupId, MEM_NAME(memGroupId, "GameManagerAPI::mGameBrowserListByClientIdMap")),
        mGameBrowserListByServerIdMap(memGroupId, MEM_NAME(memGroupId, "GameManagerAPI::mGameBrowserListByServerIdMap")),
        mNextGameBrowserListId(1),
        mNetworkAdapter(mApiParams.mNetworkAdapter),
        mMemGroup(memGroupId)
        ,mActivePresenceGameId(INVALID_GAME_ID)
#if defined(ARSON_BLAZE)
        ,mLastPresenceSetupError(ERR_OK)
#endif
    {
        // reserve & init memory pools for GameManagerAPI
        if (mApiParams.mMaxMatchmakingScenarios > 0)
        {
            mMatchmakingScenarioList.reserve(mApiParams.mMaxMatchmakingScenarios);
        }
        if (mApiParams.mMaxGameManagerGames > 0)
        {
            mGameMap.reserve(mApiParams.mMaxGameManagerGames);
        }
        if (mApiParams.mMaxGameBrowserLists > 0)
        {
            mGameBrowserListByClientIdMap.reserve(mApiParams.mMaxGameBrowserLists);
            mGameBrowserListByServerIdMap.reserve(mApiParams.mMaxGameBrowserLists);
        }

        mGameMemoryPool.reserve(mApiParams.mMaxGameManagerGames, "GMAPI::GamePool");
        mMatchmakingScenarioMemoryPool.reserve(mApiParams.mMaxMatchmakingScenarios, "GMAPI::ScenariosPool");
        mGameBrowserMemoryPool.reserve(mApiParams.mMaxGameBrowserLists, "GMAPI::GameBrowserPool");
        mGameToJobMemoryPool.reserve(mApiParams.mMaxGameManagerGames*blazeHub.getNumUsers(), "GMAPI::GameToJobPool");

        // Init game to job maps for each local user.
        for (uint16_t i = 0; i < blazeHub.getNumUsers(); ++i)
        {
            mUserToGameToJobVector[i] =  new (mGameToJobMemoryPool.alloc()) GameToJobMap( memGroupId, MEM_NAME(memGroupId, "GameManagerAPI::GameToJobMap"));
        }

        // cache off frequently looked up classes
        mUserManager = getBlazeHub()->getUserManager();
        // Only a single API is exposed to the implementing client.
        // However, under the hood we still maintain the separate components per index.
        // Our primary index has changed, and we must now call all RPC's from the new component index.
        mGameManagerComponent = getBlazeHub()->getComponentManager(mUserManager->getPrimaryLocalUserIndex())->getGameManagerComponent();

        mUserManager->addPrimaryUserListener(this);
    
        // Setup network adapter
        mNetworkAdapter->addListener(this);
        getBlazeHub()->addIdler(mNetworkAdapter, "ConnApiAdapter");
        getBlazeHub()->addUserGroupProvider(ENTITY_TYPE_GAME, this);
        getBlazeHub()->addUserGroupProvider(ENTITY_TYPE_GAME_GROUP, this);

        setupNotificationHandlers();
    }

    /*! ************************************************************************************************/
    /*! \brief destructor
    ***************************************************************************************************/
    GameManagerAPI::~GameManagerAPI()
    {
        clearNotificationHandlers();
        destroyLocalData(INVALID_USER_INDEX);     

        for (UserToGameToJobVector::iterator jobIter = mUserToGameToJobVector.begin(), jobEnd = mUserToGameToJobVector.end(); jobIter != jobEnd; ++jobIter)
        {
            mGameToJobMemoryPool.free(*jobIter);
        }
        mUserToGameToJobVector.clear();

        mUserManager->removePrimaryUserListener(this);

        // Stop listening to network adapter
        mNetworkAdapter->removeListener( this );
        getBlazeHub()->removeIdler(mNetworkAdapter);

        // user index is meaningless.
        mNetworkAdapter->destroy();

        getBlazeHub()->removeUserGroupProvider(ENTITY_TYPE_GAME, this);
        getBlazeHub()->removeUserGroupProvider(ENTITY_TYPE_GAME_GROUP, this);

        
        
        
        // move any pending jobs - do not cancel as that could be problematic if done with the API destructor
        // keep this as the last line of the destructor in order to clean up any more jobs that the destructor itself may create.
        getBlazeHub()->getScheduler()->removeByAssociatedObject(this);
        // Do NOT add code below this line.
    }

    const GameManagerCensusData* GameManagerAPI::getCensusData() const 
    { 
        CensusData::NotifyServerCensusDataItem* notifyServerCensusDataItem = getBlazeHub()->getCensusDataAPI()->getCensusData(GameManagerCensusData::TDF_ID);
        if (notifyServerCensusDataItem)
        {
            return static_cast<const GameManagerCensusData*>(notifyServerCensusDataItem->getTdf());
        }
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief log the GM's startup params
    ***************************************************************************************************/
    void GameManagerAPI::logStartupParameters() const
    {
        mApiParams.logParameters();
    }

    /*! ************************************************************************************************/
    /*! \brief impl of UserGroupProvider::getUserGroupById; return a specific game as a usergroup
        \param blazeObjectId that identifies a particular Game.
        \return Game object that corresponds to the bobjId.
    ***************************************************************************************************/
    Game* GameManagerAPI::getUserGroupById(const EA::TDF::ObjectId &blazeObjectId) const
    {
        // pre: GameId's are unique across game sessions and game groups
        if ((blazeObjectId.type == ENTITY_TYPE_GAME) || (blazeObjectId.type == ENTITY_TYPE_GAME_GROUP))
        {
            return getGameById(blazeObjectId.toObjectId<GameId>());
        }
        return nullptr;
    }


    void GameManagerAPI::onAuthenticated(uint32_t userIndex) 
    { 
#if ENABLE_BLAZE_SDK_LOGGING
        UserManager::LocalUser *localUser = mUserManager->getLocalUser(userIndex);
        BLAZE_SDK_DEBUGF("[GMGR] user(%" PRId64 ":%s) at user index(%u) AUTHENTICATED\n", localUser->getId(), localUser->getName(), userIndex);
#endif
        // init network adapter
        if (!mNetworkAdapter->isInitialized())
        {
            mNetworkAdapter->initialize(getBlazeHub());
        }

        mNetworkAdapter->registerVoipLocalUser(userIndex);

        int32_t preventMultipleGameMembership = 0;
        if (getBlazeHub()->getConnectionManager()->getServerConfigInt(BLAZESDK_CONFIG_KEY_GAMEMANAGER_PREVENT_MULTI_GAME_MEMBERSHIP, &preventMultipleGameMembership))
        {
            mApiParams.mPreventMultipleGameMembership = (preventMultipleGameMembership != 0);
        }
    }

    void GameManagerAPI::onDeAuthenticated(uint32_t userIndex) 
    { 
        BLAZE_SDK_DEBUGF("[GMGR] user index(%u) DEAUTHENTICATED\n", userIndex);

        bool networkAdapterInitialized = mNetworkAdapter->isInitialized();
        if (networkAdapterInitialized)
        {
            mNetworkAdapter->unregisterVoipLocalUser(userIndex);
        }

        if (!mUserManager->hasLocalUsers()) // No local user exists anymore, just clean up all the data
        {
            getBlazeHub()->getScheduler()->cancelByAssociatedObject(this);
            
            if (networkAdapterInitialized)
            {
                mNetworkAdapter->destroy();
            }
            
            destroyLocalData(INVALID_USER_INDEX);
        }
        else
        {
            destroyLocalData(userIndex);
        }
        
    }


    /*! ************************************************************************************************/
    /*! \brief set all async notification handlers on the GM component.
    ***************************************************************************************************/
    void GameManagerAPI::setupNotificationHandlers()
    {
        // Note: order doesn't matter (technically), but we should keep this in sync with the order in clearNotificationHandlers

        // MLU AUDIT: we may want to consider only registering for the primary user and switching notification
        // handlers when primary is removed.  However, it may make more sense to govern this from the server.
        // Once that is fixed there are some returns on checks that the player is not the primary user.

        // register for events from all local users.
        for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); ++userIndex)
        {
            GameManagerComponent* component = getBlazeHub()->getComponentManager(userIndex)->getGameManagerComponent();

            // player join/remove notifications
            component->setNotifyGameSetupHandler(GameManagerComponent::NotifyGameSetupCb( this, &GameManagerAPI::onNotifyGameSetup));
            component->setNotifyPlayerJoiningHandler(GameManagerComponent::NotifyPlayerJoiningCb(this, &GameManagerAPI::onNotifyPlayerJoining));
            component->setNotifyPlayerJoiningQueueHandler(GameManagerComponent::NotifyPlayerJoiningQueueCb(this, &GameManagerAPI::onNotifyPlayerJoiningQueue));
            component->setNotifyPlayerClaimingReservationHandler(GameManagerComponent::NotifyPlayerClaimingReservationCb(this, &GameManagerAPI::onNotifyPlayerClaimingReservation));
            component->setNotifyPlayerPromotedFromQueueHandler(GameManagerComponent::NotifyPlayerPromotedFromQueueCb(this, &GameManagerAPI::onNotifyPlayerPromotedFromQueue));
            component->setNotifyPlayerDemotedToQueueHandler(GameManagerComponent::NotifyPlayerPromotedFromQueueCb(this, &GameManagerAPI::onNotifyPlayerDemotedToQueue));
            component->setNotifyJoiningPlayerInitiateConnectionsHandler(GameManagerComponent::NotifyJoiningPlayerInitiateConnectionsCb(this, &GameManagerAPI::onNotifyJoiningPlayerInitiateConnections));
            component->setNotifyPlayerJoinCompletedHandler(GameManagerComponent::NotifyPlayerJoinCompletedCb(this, &GameManagerAPI::onNotifyPlayerJoinComplete));
            component->setNotifyPlayerRemovedHandler(GameManagerComponent::NotifyPlayerRemovedCb(this, &GameManagerAPI::onNotifyPlayerRemoved));

            // game state/settings notifications
            component->setNotifyGameResetHandler(GameManagerComponent::NotifyGameResetCb(this, &GameManagerAPI::onNotifyGameReset));
            component->setNotifyGameStateChangeHandler(GameManagerComponent::NotifyGameStateChangeCb(this, &GameManagerAPI::onNotifyGameStateChanged));
            component->setNotifyGameCapacityChangeHandler(GameManagerComponent::NotifyGameCapacityChangeCb(this, &GameManagerAPI::onNotifyGameCapacityChanged));
            component->setNotifyGameReportingIdChangeHandler(GameManagerComponent::NotifyGameReportingIdChangeCb(this, &GameManagerAPI::onNotifyGameReportingIdChanged));
            component->setNotifyAdminListChangeHandler(GameManagerComponent::NotifyAdminListChangeCb(this, &GameManagerAPI::onNotifyGameAdminListChange));
            component->setNotifyGameAttribChangeHandler( GameManagerComponent::NotifyGameAttribChangeCb(this, &GameManagerAPI::onNotifyGameAttributeChanged) );
            component->setNotifyDedicatedServerAttribChangeHandler(GameManagerComponent::NotifyDedicatedServerAttribChangeCb(this, &GameManagerAPI::onNotifyDedicatedServerAttributeChanged));
            component->setNotifyGameSettingsChangeHandler( GameManagerComponent::NotifyGameSettingsChangeCb(this, &GameManagerAPI::onNotifyGameSettingChanged) );
            component->setNotifyGameModRegisterChangedHandler( GameManagerComponent::NotifyGameModRegisterChangedCb(this, &GameManagerAPI::onNotifyGameModRegisterChanged) );
            component->setNotifyGameEntryCriteriaChangedHandler( GameManagerComponent::NotifyGameEntryCriteriaChangedCb(this, &GameManagerAPI::onNotifyGameEntryCriteriaChanged) );
            component->setNotifyGameTeamIdChangeHandler( GameManagerComponent::NotifyGameTeamIdChangeCb(this, &GameManagerAPI::onNotifyGameTeamIdChanged) );
            component->setNotifyGameRemovedHandler( GameManagerComponent::NotifyGameRemovedCb(this, &GameManagerAPI::onNotifyGameRemoved) );
            component->setNotifyGameNameChangeHandler( GameManagerComponent::NotifyGameNameChangeCb(this, &GameManagerAPI::onNotifyGameNameChanged) );
            component->setNotifyPresenceModeChangedHandler( GameManagerComponent::NotifyPresenceModeChangedCb(this, &GameManagerAPI::onNotifyPresenceModeChanged) );

            // Host Migration related notifications
            component->setNotifyPlatformHostInitializedHandler( GameManagerComponent::NotifyPlatformHostInitializedCb(this, &GameManagerAPI::onNotifyPlatformHostInitialized) );
            component->setNotifyHostMigrationStartHandler( GameManagerComponent::NotifyHostMigrationStartCb(this, &GameManagerAPI::onNotifyHostMigrationStart) );
            component->setNotifyHostMigrationFinishedHandler( GameManagerComponent::NotifyHostMigrationFinishedCb(this, &GameManagerAPI::onNotifyHostMigrationFinished) );

            // player state/settings notifications
            component->setNotifyPlayerCustomDataChangeHandler( GameManagerComponent::NotifyPlayerCustomDataChangeCb(this, &GameManagerAPI::onNotifyPlayerCustomDataChanged) );
            component->setNotifyGamePlayerStateChangeHandler( GameManagerComponent::NotifyGamePlayerStateChangeCb(this, &GameManagerAPI::onNotifyGamePlayerStateChanged));
            component->setNotifyPlayerAttribChangeHandler( GameManagerComponent::NotifyPlayerAttribChangeCb(this, &GameManagerAPI::onNotifyPlayerAttributeChanged) );
            component->setNotifyGamePlayerTeamRoleSlotChangeHandler( GameManagerComponent::NotifyGamePlayerTeamRoleSlotChangeCb(this, &GameManagerAPI::onNotifyGamePlayerTeamRoleSlotChange) );

            // Matchmaker/GameBrowser notifications
            component->setNotifyMatchmakingFailedHandler(GameManagerComponent::NotifyMatchmakingFailedCb(this, &GameManagerAPI::onNotifyMatchmakingFailed));
            component->setNotifyMatchmakingAsyncStatusHandler(GameManagerComponent::NotifyMatchmakingAsyncStatusCb(this, &GameManagerAPI::onNotifyMatchmakingAsyncStatus));
            component->setNotifyMatchmakingSessionConnectionValidatedHandler(GameManagerComponent::NotifyMatchmakingSessionConnectionValidatedCb(this, &GameManagerAPI::onNotifyMatchmakingSessionConnectionValidated));
            component->setNotifyMatchmakingPseudoSuccessHandler(GameManagerComponent::NotifyMatchmakingPseudoSuccessCb(this, &GameManagerAPI::onNotifyMatchmakingPseudoSuccess));
            component->setNotifyMatchmakingScenarioPseudoSuccessHandler(GameManagerComponent::NotifyMatchmakingScenarioPseudoSuccessCb(this, &GameManagerAPI::onNotifyMatchmakingScenarioPseudoSuccess));
            component->setNotifyGameListUpdateHandler(GameManagerComponent::NotifyGameListUpdateCb(this, &GameManagerAPI::onNotifyGameListUpdate));
            component->setNotifyProcessQueueHandler(GameManagerComponent::NotifyProcessQueueCb(this, &GameManagerAPI::onNotifyProcessQueue));
            component->setNotifyQueueChangedHandler(GameManagerComponent::NotifyQueueChangedCb(this, &GameManagerAPI::onNotifyQueueChanged));
            component->setNotifyMatchmakingReservedExternalPlayersHandler(GameManagerComponent::NotifyMatchmakingReservedExternalPlayersCb(this, &GameManagerAPI::onNotifyMatchmakingReservedExternalPlayers));
            
            // remote user action notifications
            component->setNotifyRemoteMatchmakingStartedHandler(GameManagerComponent::NotifyRemoteMatchmakingStartedCb(this, &GameManagerAPI::onNotifyRemoteMatchmakingStarted));
            component->setNotifyRemoteMatchmakingEndedHandler(GameManagerComponent::NotifyRemoteMatchmakingEndedCb(this, &GameManagerAPI::onNotifyRemoteMatchmakingEnded));
            component->setNotifyRemoteJoinFailedHandler(GameManagerComponent::NotifyRemoteJoinFailedCb(this, &GameManagerAPI::onRemoteJoinFailed));

            component->setNotifyHostedConnectivityAvailableHandler(GameManagerComponent::NotifyHostedConnectivityAvailableCb(this, &GameManagerAPI::onNotifyHostedConnectivityAvailable));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief clear all async notification handlers on the GM component.
    ***************************************************************************************************/
    void GameManagerAPI::clearNotificationHandlers()
    {
        // Note: order doesn't matter (technically), but we should keep this in sync with the order in setupNotificationHandlers

        // unregister for events from all local users.
        for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); ++userIndex)
        {
            GameManagerComponent* component = getBlazeHub()->getComponentManager(userIndex)->getGameManagerComponent();

            // player join/remove notifications
            component->clearNotifyGameSetupHandler();
            component->clearNotifyPlayerJoiningHandler();
            component->clearNotifyPlayerJoiningQueueHandler();
            component->clearNotifyPlayerClaimingReservationHandler();
            component->clearNotifyPlayerPromotedFromQueueHandler();
            component->clearNotifyPlayerDemotedToQueueHandler();
            component->clearNotifyJoiningPlayerInitiateConnectionsHandler();
            component->clearNotifyPlayerJoinCompletedHandler();
            component->clearNotifyPlayerRemovedHandler();

            // game state/settings notifications
            component->clearNotifyGameResetHandler();
            component->clearNotifyGameStateChangeHandler();
            component->clearNotifyGameCapacityChangeHandler();
            component->clearNotifyGameReportingIdChangeHandler();
            component->clearNotifyAdminListChangeHandler();
            component->clearNotifyGameAttribChangeHandler();
            component->clearNotifyGameSettingsChangeHandler();
            component->clearNotifyGameModRegisterChangedHandler();
            component->clearNotifyGameEntryCriteriaChangedHandler();
            component->clearNotifyGameTeamIdChangeHandler();
            component->clearNotifyGameRemovedHandler();
            component->clearNotifyGameNameChangeHandler();
            component->clearNotifyPresenceModeChangedHandler();

            // Host Migration related notifications
            component->clearNotifyPlatformHostInitializedHandler();
            component->clearNotifyHostMigrationStartHandler();
            component->clearNotifyHostMigrationFinishedHandler();

            // player state/settings notifications
            component->clearNotifyPlayerCustomDataChangeHandler();
            component->clearNotifyGamePlayerStateChangeHandler();
            component->clearNotifyPlayerAttribChangeHandler();
            component->clearNotifyGamePlayerTeamRoleSlotChangeHandler();

            // Matchmaker/GameBrowser notifications
            component->clearNotifyMatchmakingFailedHandler();
            component->clearNotifyMatchmakingAsyncStatusHandler();
            component->clearNotifyMatchmakingSessionConnectionValidatedHandler();
            component->clearNotifyMatchmakingPseudoSuccessHandler();
            component->clearNotifyMatchmakingScenarioPseudoSuccessHandler();
            component->clearNotifyGameListUpdateHandler();
            component->clearNotifyProcessQueueHandler();
            component->clearNotifyQueueChangedHandler();
            component->clearNotifyMatchmakingReservedExternalPlayersHandler();

            // remote user action notifications
            component->clearNotifyRemoteMatchmakingStartedHandler();
            component->clearNotifyRemoteMatchmakingEndedHandler();
            component->clearNotifyRemoteJoinFailedHandler();

            component->clearNotifyHostedConnectivityAvailableHandler();
        }
    }

    /*! ************************************************************************************************/
    /*! \brief destroy local data, usually after a logout or loss of network connection (games, matchmaking sessions, game browser lists, etc)
    \param[in] userIndex for a valid user. If INVALID_USER_INDEX, all the local data is destroyed.
    ***************************************************************************************************/
    void GameManagerAPI::destroyLocalData(uint32_t userIndex)
    {
        if (userIndex == INVALID_USER_INDEX)
        {
            // clean up any GameBrowser lists - do this before cleaning up any remaining games/players.
            GameBrowserListMap::iterator gameListMapIter = mGameBrowserListByClientIdMap.begin();
            GameBrowserListMap::iterator gameListMapEnd = mGameBrowserListByClientIdMap.end();
            for ( ; gameListMapIter != gameListMapEnd; ++gameListMapIter )
            {
                mDispatcher.dispatch("onGameBrowserListDestroy", &GameManagerAPIListener::onGameBrowserListDestroy, gameListMapIter->second);
                mGameBrowserMemoryPool.free(gameListMapIter->second);
            }
            mGameBrowserListByClientIdMap.clear();
            mGameBrowserListByServerIdMap.clear();

            // cleanup any remaining games
            while (!mGameMap.empty())
            {
                GameMap::iterator gameMapIter = mGameMap.begin();
                Game *game = gameMapIter->second;

#if defined(BLAZE_EXTERNAL_SESSIONS_SUPPORT)
                // Before deleting game, ensure cleanup and listeners notified. Dispatch now. Don't wait for the primary game update as it may never arrive
                if ((game->getId() == getActivePresenceGameId()) && (getBlazeHub() != nullptr))
                    onActivePresenceCleared(true, getBlazeHub()->getPrimaryLocalUserIndex());
#endif
                mGameMap.erase(game->getId());

                if (game->isSettingUpNetwork() || game->isNetworkCreated())
                {
                    // tear down network connection(s)
                    mNetworkAdapter->destroyNetworkMesh(game);
                }

                //  dispatch to listeners the game's local destruction.
                mDispatcher.dispatch("onGameDestructing", &GameManagerAPIListener::onGameDestructing, this, game, SYS_GAME_ENDING);

                mGameMemoryPool.free(game);
            }

            // clean up any matchmaking scenarios
            MatchmakingScenarioList::iterator scenarioIter = mMatchmakingScenarioList.begin();
            MatchmakingScenarioList::iterator scenarioEnd = mMatchmakingScenarioList.end();
            for ( ; scenarioIter != scenarioEnd; ++scenarioIter)
            {
                MatchmakingScenario *scenario = *scenarioIter;
                mDispatcher.dispatch("onMatchmakingScenarioFinished", &GameManagerAPIListener::onMatchmakingScenarioFinished, SESSION_CANCELED, const_cast<const MatchmakingScenario*>(scenario), static_cast<Game*>(nullptr));
                mMatchmakingScenarioMemoryPool.free( scenario );
            }
            mMatchmakingScenarioList.clear();
        }
        else
        {
            // Remove games in which no local users are left online.
            GameMap gameMapCopy = mGameMap;
            GameMap::iterator gameMapIter = gameMapCopy.begin();
            while (gameMapIter != gameMapCopy.end())
            {
                Game *game = gameMapIter->second;
                uint32_t otherLocalPlayersExist = false;
                for (uint16_t playerIndex = 0; playerIndex <  mBlazeHub->getNumUsers(); ++playerIndex)
                {
                    if (playerIndex != userIndex)
                    {
                        Player* pPlayer = game->getLocalPlayer(playerIndex);
                        if (pPlayer)
                        {   
                            otherLocalPlayersExist = true;
                            break;
                        }
                    }
                }

                if (!otherLocalPlayersExist)
                {
#if defined(BLAZE_EXTERNAL_SESSIONS_SUPPORT)
                    // Before deleting game, ensure cleanup and listeners notified. Dispatch now. Don't wait for the primary game update as it may never arrive
                    if ((game->getId() == getActivePresenceGameId()) && (getBlazeHub() != nullptr))
                        onActivePresenceCleared(true, getBlazeHub()->getPrimaryLocalUserIndex());
#endif
                    mGameMap.erase(game->getId());

                    if (game->isSettingUpNetwork() || game->isNetworkCreated())
                    {
                        // tear down network connection(s)
                        mNetworkAdapter->destroyNetworkMesh(game);
                    }
                
                    //  dispatch to listeners the game's local destruction.
                    mDispatcher.dispatch("onGameDestructing", &GameManagerAPIListener::onGameDestructing, this, game, SYS_GAME_ENDING);

                    mGameMemoryPool.free(game);

                }
                ++gameMapIter;
            }
        }
    }

    /**************************************************************************************************/
    
    void GameManagerAPI::linkGameToJob(uint32_t userIndex, GameId gameId, const GameManagerApiJob *job)
    {
        if (userIndex < mUserToGameToJobVector.size())
        {
            GameToJobMap* jobMap = mUserToGameToJobVector[userIndex];
            if (jobMap != nullptr)
            {
                jobMap->insert(eastl::make_pair(gameId, job->getId()));
            }
        }
    }

    void GameManagerAPI::unlinkGameFromJob(uint32_t userIndex, GameId gameId)
    {
        if (userIndex < mUserToGameToJobVector.size())
        {
            GameToJobMap* jobMap = mUserToGameToJobVector[userIndex];
            if (jobMap != nullptr)
            {
                jobMap->erase(gameId);
            }
        }
    }

    GameManagerApiJob* GameManagerAPI::getJob(uint32_t userIndex, GameId gameId)
    {
        if (userIndex < mUserToGameToJobVector.size())
        {
            GameToJobMap* jobMap = mUserToGameToJobVector[userIndex];
            GameToJobMap::const_iterator jobMapIt = jobMap->find(gameId);
            if (jobMapIt != jobMap->end())
            {        
                return static_cast<GameManagerApiJob*>(getBlazeHub()->getScheduler()->getJob(jobMapIt->second));
            }
        }

        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Helper to init a SlotCapacitiesVector from a SlotCapacities (array).
    
        \param[out] destVector the destination vector that's initialized
        \param[in] slotArray the source slot capacities array
    ***************************************************************************************************/
    void GameManagerAPI::setSlotCapacitiesVector(SlotCapacitiesVector &destVector, const SlotCapacities &slotArray)
    {
        for (SlotCapacitiesVector::size_type counter = SLOT_PUBLIC_PARTICIPANT; counter < MAX_SLOT_TYPE; ++counter)
        {
            destVector[counter] = slotArray[counter];
        }
    }

    JobId GameManagerAPI::scheduleCreateGameCb(const char8_t* jobName, const CreateGameCb &titleCb, BlazeError error)
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor(jobName, titleCb, error, jobId, (Game*)nullptr,
            this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
        return jobId;
    }

    JobId GameManagerAPI::scheduleJoinGameCb(const char8_t* jobName, const JoinGameCb &titleCb, BlazeError error)
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        JobId jobId = jobScheduler->reserveJobId();
        jobId = jobScheduler->scheduleFunctor(jobName, titleCb, error, jobId, (Game*)nullptr, "",
            this, 0, jobId); // assocObj, delayMs, reservedJobId
        Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
        return jobId;
    }

    // Helper function: 
    void GameManagerAPI::prepareCommonGameData(CommonGameRequestData& commonData, GameType gameType) const
    {
        commonData.setGameProtocolVersionString(mApiParams.mGameProtocolVersionString.c_str());
        commonData.setGameType(gameType);

        // Pull address information from network adapter.  If its not SG address this goes through
        // qosManager to get the latest address
        // GM_TODO: this address is now also sent up to the server in users extended data, this could be removed.
        if (mNetworkAdapter->isInitialized())
            mNetworkAdapter->getLocalAddress().copyInto(commonData.getPlayerNetworkAddress());
        else 
            BLAZE_SDK_DEBUGF("[GMGR] prepareCommonGameData: cannot populate player network address, local network adapter not initialzied.\n");
    }

    BlazeError GameManagerAPI::prepareCreateGameRequest(const Game::CreateGameParametersBase &params, const PlayerJoinDataHelper& playerData,
        CreateGameRequest& createGameRequest) const
    {
        // A specified user group will always trump the local connection group.
        if (playerData.getGroupId() != EA::TDF::OBJECT_ID_INVALID)
        {
            // User group can't create client server dedicated game
            if (params.mNetworkTopology == CLIENT_SERVER_DEDICATED)
            {
                // group can't create dedicated server game, need to use resetDedicatedServer instead;
                return GAMEMANAGER_ERR_INVALID_ACTION_FOR_GROUP;
            }
        }

        if (params.mVoipTopology == VOIP_DEDICATED_SERVER && ((params.mNetworkTopology == CLIENT_SERVER_PEER_HOSTED) || (params.mNetworkTopology == PEER_TO_PEER_FULL_MESH)))
        {
            // can't create game with unsupported topology (VoIP dedicated)
            return GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED;
        }

        BlazeError err = ERR_OK;

        playerData.copyInto(createGameRequest.getPlayerJoinData());
        prepareCommonGameData(createGameRequest.getCommonGameData(), params.mGameType);

        createGameRequest.getGameCreationData().setGameModRegister(params.mGameModRegister);
        createGameRequest.getGameCreationData().setGameName(params.mGameName);
        createGameRequest.getGameCreationData().setPresenceMode(params.mPresenceMode);
        createGameRequest.getGameCreationData().getGameSettings().setBits( params.mGameSettings.getBits() );
        createGameRequest.getGameCreationData().setNetworkTopology(params.mNetworkTopology);
        createGameRequest.getGameCreationData().setVoipNetwork(params.mVoipTopology);
        createGameRequest.getGameCreationData().getEntryCriteriaMap().insert(params.mEntryCriteriaMap.begin(), params.mEntryCriteriaMap.end());
        createGameRequest.setGamePingSiteAlias(params.mGamePingSiteAlias.c_str());
        createGameRequest.setGameStatusUrl(params.mGameStatusUrl.c_str());
        setSlotCapacitiesVector(createGameRequest.getSlotCapacities(), params.mPlayerCapacity);
        createGameRequest.getGameCreationData().setQueueCapacity(params.mQueueCapacity);
        createGameRequest.getGameCreationData().setMaxPlayerCapacity(params.mMaxPlayerCapacity);
        createGameRequest.getGameCreationData().setMinPlayerCapacity(params.mMinPlayerCapacity);
        createGameRequest.getGameCreationData().getGameAttribs().insert(params.mGameAttributes.begin(), params.mGameAttributes.end());
        createGameRequest.getGameCreationData().setPermissionSystemName(params.mPermissionSystemName);
        createGameRequest.getMeshAttribs().insert(params.mMeshAttributes.begin(), params.mMeshAttributes.end());
        createGameRequest.setGameReportName(params.mGameReportName);
        createGameRequest.setServerNotResetable(params.mServerNotResetable);
#if (defined(EA_PLATFORM_PS4) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        params.mExternalSessionStatus.copyInto(createGameRequest.getGameCreationData().getExternalSessionStatus());
#endif
#if defined(EA_PLATFORM_PS4) || defined(EA_PLATFORM_PS5) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        params.mExternalSessionCustomData.copyInto(createGameRequest.getGameCreationData().getExternalSessionCustomData());
#endif
#if defined(EA_PLATFORM_XBOXONE)|| defined(EA_PLATFORM_XBSX) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        createGameRequest.getGameCreationData().getExternalSessionIdentSetup().getXone().setTemplateName(params.mSessionTemplateName.c_str());
        if (!params.mExternalSessionCustomDataString.empty())
            createGameRequest.getGameCreationData().getExternalSessionCustomData().setData((uint8_t*)params.mExternalSessionCustomDataString.c_str(), params.mExternalSessionCustomDataString.length());
#endif
        validateGameSettings(params.mGameSettings);

        prepareTeamsForServerRequest(createGameRequest.getPlayerJoinData(), params.mTeamIds, createGameRequest.getGameCreationData().getTeamIds());

        // required defaults supplied on server if the following are unset by the client
        params.mRoleInformation.copyInto(createGameRequest.getGameCreationData().getRoleInformation());

        uint32_t userIndex = getBlazeHub()->getPrimaryLocalUserIndex();
        UserManager::LocalUser* localUser = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
        PlayerJoinData& playerJoinData = createGameRequest.getPlayerJoinData();

        // Virtual game creation doesn't add players, except if its reservations. No need to add caller if not reserving
        if ((localUser != nullptr) && (!params.mGameSettings.getVirtualized() || (createGameRequest.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION)))
        {
            PlayerJoinDataHelper::addPlayerJoinData(playerJoinData, localUser->getUser());
        }

        // set persisted id and secret to the request if the setting is true
        if (params.mGameSettings.getEnablePersistedGameId())
        {
            createGameRequest.setPersistedGameId(params.mPersistedGameId);
            createGameRequest.getPersistedGameIdSecret().setData(params.mPersistedGameIdSecret.getData(), params.mPersistedGameIdSecret.getCount());
        }

        // override network topology for game group
        if (params.mGameType == GAME_TYPE_GROUP)
        {
            if (params.mNetworkTopology != NETWORK_DISABLED)
            {
                 BLAZE_SDK_DEBUGF("[GMGR] Network topology (%s) is not supported in game group.\n", GameNetworkTopologyToString(params.mNetworkTopology));
                 return GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED;
            }

            if ((params.mVoipTopology != VOIP_DISABLED) && (params.mVoipTopology != VOIP_PEER_TO_PEER))
            {
                 BLAZE_SDK_DEBUGF("[GMGR] Voip topology (%s) is not supported in game group.\n", VoipTopologyToString(params.mVoipTopology));
                 return GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED;
            }
        }

        if (createGameRequest.getPlayerJoinData().getGroupId() == EA::TDF::OBJECT_ID_INVALID)
        {
            // send up our connection group id.
            if (localUser != nullptr)
            {
                createGameRequest.getPlayerJoinData().setGroupId(localUser->getConnectionGroupObjectId());
            }
        }

        // Validate that only a single role is specified for each player
        err = validatePlayerJoinDataForCreateOrJoinGame(createGameRequest.getPlayerJoinData());
        if (err == ERR_OK)
            err = validateTeamIndex(createGameRequest.getGameCreationData().getTeamIds(), createGameRequest.getPlayerJoinData());

        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief attempt to create a game with the supplied args
    ***************************************************************************************************/
    JobId GameManagerAPI::createGame(const Game::CreateGameParametersBase &params, 
                                     const PlayerJoinDataHelper& playerData, 
                                     const CreateGameCb &titleCreateGameCb)
    {
        // can't create a game if the network adapter isn't initialized
        BlazeAssert(mNetworkAdapter->isInitialized());
        if (!mNetworkAdapter->isInitialized())
        {
            return scheduleCreateGameCb("createGameCb", titleCreateGameCb, ERR_AUTHENTICATION_REQUIRED);
        }

        if (!getBlazeHub()->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
        {
            return scheduleCreateGameCb("createGameCb", titleCreateGameCb, SDK_ERR_QOS_PINGSITE_NOT_INITIALIZED);
        }

        // Prepare request
        CreateGameRequest createGameRequest;
        BlazeError err = prepareCreateGameRequest(params, playerData, createGameRequest);

        if (err != ERR_OK)
        {
            return scheduleCreateGameCb("createGameCb", titleCreateGameCb, err);
        }

        // Begins the CreateGameJob which manages several RPC tasks.
        CreateGameJob *createGameJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "CreateGameJob") CreateGameJob(this, titleCreateGameCb);
        createGameJob->setGameId(getBlazeHub()->getPrimaryLocalUserIndex(), INVALID_GAME_ID);

        // the create game job is created to provide a job rpcJobId SDK users can cancel a create game request with
        // if the job is canceled, it triggers the titleCb with SDK_ERR_RPC_CANCELED and a nullptr game pointer
        // gamemanagerAPI will then take care of destroying the game in the background.
        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("CreateGameJob", createGameJob, this);

        BLAZE_SDK_DEBUGF("[GMGR] Starting createGameJob(%u).\n", jobId.get());

        // Send off request to server; stuffing the title's cb functor into the rpc payload
        mGameManagerComponent->createGame( createGameRequest, MakeFunctor(this, &GameManagerAPI::internalCreateGameCb), jobId); 

        return jobId;
    }

    JobId GameManagerAPI::createGame(const Game::CreateGameParameters &params, 
                                     const CreateGameCb &titleCreateGameCb, 
                                     const UserGroup *userGroup /* = nullptr */,
                                     const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        // loadFromCreateGameParameters
        PlayerJoinDataHelper::loadFromRoleNameToPlayerMap(playerData.getPlayerJoinData(), params.mPlayerRoles);
        PlayerJoinDataHelper::loadFromRoleNameToUserIdentificationMap(playerData.getPlayerJoinData(), params.mUserIdentificationRoles, true);
        playerData.getPlayerJoinData().setDefaultSlotType(params.mJoiningSlotType);
        playerData.getPlayerJoinData().setGameEntryType(params.mJoiningEntryType);
        playerData.getPlayerJoinData().setDefaultTeamIndex(params.mJoiningTeamIndex);

        if (userGroup != nullptr)
        {
            playerData.getPlayerJoinData().setGroupId(userGroup->getBlazeObjectId());
        }
        if (reservedPlayerIdentifications != nullptr)
        {
            PlayerJoinDataHelper::loadFromUserIdentificationList(playerData.getPlayerJoinData(), *reservedPlayerIdentifications, nullptr, true);
        }

        return createGame(params, playerData, titleCreateGameCb);
    }

    /*! ************************************************************************************************/
    /*! \brief internal createGame callback; a DS host will create a local game here, all other topologies
            wait for a join game notification (see onNotifyGameSetup)
    ***************************************************************************************************/
    void GameManagerAPI::internalCreateGameCb(const CreateGameResponse *response, BlazeError error, JobId rpcJobId, JobId jobId)
    {
        // check if user canceled the createGameJob
        CreateGameJob *createGameJob = (CreateGameJob*)getBlazeHub()->getScheduler()->getJob(jobId);
        if (createGameJob == nullptr)
        {
            // job canceled by user, if the game was created, destroy it (silently)
            if (error == ERR_OK)
            {
                BLAZE_SDK_DEBUGF("[GMGR] Game creation succeeded for canceled createGameJob(%u); destroying game silently.\n", jobId.get());
                DestroyGameRequest destroyGameRequest;
                destroyGameRequest.setGameId( response->getGameId() );
                destroyGameRequest.setDestructionReason( HOST_LEAVING );
                mGameManagerComponent->destroyGame(destroyGameRequest);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[GMGR] Game creation failed for canceled createGameJob(%u) (no action taken).\n", jobId.get());
            }
            return;
        }
        
        // createGameJob still valid, handle blazeServer createGame response
        if (error != ERR_OK)
        {   
            // Note: the assumption here is that the game has not been created (normal server error).  
            // RPC timeouts however will fall through to here also, and since we know nothing of the game at this point
            // we are unable to silently destroy it, but we dispatch title callbacks.
            createGameJob->cancel(error);
            BLAZE_SDK_DEBUGF("[GMGR] createGameJob(%u): Game creation failed with %s.\n", 
                jobId.get(), getBlazeHub()->getErrorName(error, getBlazeHub()->getUserManager()->getPrimaryLocalUserIndex()));
            getBlazeHub()->getScheduler()->removeJob(createGameJob);
            return;
        }

        // blazeServer created game successfully
        BLAZE_SDK_DEBUGF("[GMGR] createGameJob(%u): Game(%" PRIu64 ") created.\n", jobId.get(), response->getGameId());

        // The primary local user is always the one that creates a new game:
        createGameJob->setGameId(getBlazeHub()->getPrimaryLocalUserIndex(), response->getGameId());
        createGameJob->setReservedExternalPlayerIdentifications(response->getJoinedReservedPlayerIdentifications());

        //  execution continues from onNotifyGameSetup

        //  NOTE: Shouldn't a DS host remove the createGameJob here based on this method's comments (above)?
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


    JobId GameManagerAPI::createGameFromTemplate(const TemplateName& templateName, 
        const TemplateAttributes& templateAttributes,
        const PlayerJoinDataHelper& playerData, 
        const JoinGameCb &titleCb)
    {
        // can't matchmake if the network adapter isn't initialized
        BlazeAssert(mNetworkAdapter->isInitialized());
        if(!mNetworkAdapter->isInitialized())
        {
            return scheduleJoinGameCb("createGameFromTemplateCb", titleCb, ERR_AUTHENTICATION_REQUIRED);
        }

        if (!getBlazeHub()->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
        {
            return scheduleJoinGameCb("createGameFromTemplateCb", titleCb, SDK_ERR_QOS_PINGSITE_NOT_INITIALIZED);
        }

        CreateGameTemplateRequest createGameTemplateRequest;
        prepareCommonGameData(createGameTemplateRequest.getCommonGameData(), GAME_TYPE_GAMESESSION);  // Game Type is overridden via the server config, so it doesn't matter here. 
        templateAttributes.copyInto(createGameTemplateRequest.getTemplateAttributes());
        createGameTemplateRequest.setTemplateName(templateName);

        playerData.copyInto(createGameTemplateRequest.getPlayerJoinData());

        // Add the main player: (if possible)
        uint32_t userIndex = getBlazeHub()->getPrimaryLocalUserIndex();
        UserManager::LocalUser* localUser = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
        if (localUser == nullptr)
        {
            return scheduleJoinGameCb("createGameFromTemplateCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND);
        }

        PlayerJoinDataHelper::addPlayerJoinData(createGameTemplateRequest.getPlayerJoinData(), localUser->getUser());    // Main caller/player is always added to the create game template requests, even if this will be an external owner, virtual game etc (what to do with the data determined by server side template)

        if (playerData.getGroupId() == EA::TDF::OBJECT_ID_INVALID)
        {
            // send up our connection group id.
            createGameTemplateRequest.getPlayerJoinData().setGroupId(localUser->getConnectionGroupObjectId());
        }

        BlazeError err = validatePlayerJoinDataForCreateOrJoinGame(createGameTemplateRequest.getPlayerJoinData());
        if (err != ERR_OK)
        {
            return scheduleJoinGameCb("createGameFromTemplateCb", titleCb, err);
        }

        // Begins the CreateGameJob which manages several RPC tasks.
        JoinGameJob *createGameJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "CreateGameTemplateJob") JoinGameJob(this, titleCb, getBlazeHub()->getPrimaryLocalUserIndex());
        createGameJob->setGameId(getBlazeHub()->getPrimaryLocalUserIndex(), INVALID_GAME_ID);

        // the create game job is created to provide a job rpcJobId SDK users can cancel a create game request with
        // if the job is canceled, it triggers the titleCb with SDK_ERR_RPC_CANCELED and a nullptr game pointer
        // gamemanagerAPI will then take care of destroying the game in the background.
        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("CreateGameTemplateJob", createGameJob, this);

        BLAZE_SDK_DEBUGF("[GMGR] Starting createGameTemplateJob(%u).\n", jobId.get());


        mGameManagerComponent->createGameTemplate(createGameTemplateRequest, MakeFunctor(this, &GameManagerAPI::internalCreateOrJoinGameCb), jobId);

        return jobId;

    }

    /*! ****************************************************************************/
    /*! \brief Find a resetable dedicated server, reset it using the supplied parameters and
            join the newly reset game.
    ********************************************************************************/
    JobId GameManagerAPI::resetDedicatedServer(const Game::ResetGameParametersBase &params,
                                               const PlayerJoinDataHelper& playerData,
                                               const JoinGameCb &titleCb)
    {
        // can't reset a dedicated server if the network adapter isn't initialized
        BlazeAssert(mNetworkAdapter->isInitialized());
        if (!mNetworkAdapter->isInitialized())
        {
            return scheduleJoinGameCb("resetDedicatedServerCb", titleCb, ERR_AUTHENTICATION_REQUIRED);
        }

        if (!getBlazeHub()->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
        {
            return scheduleJoinGameCb("resetDedicatedServerCb", titleCb, SDK_ERR_QOS_PINGSITE_NOT_INITIALIZED);
        }

        // Technically, this is not needed because we'll always set the Network Topology to a DedicatedServer topology (if the wrong topology is used) on the blaze server.
        if (params.mVoipTopology == VOIP_DEDICATED_SERVER && ((params.mNetworkTopology == CLIENT_SERVER_PEER_HOSTED) || (params.mNetworkTopology == PEER_TO_PEER_FULL_MESH)))
        {
            // can't create game with unsupported topology (VoIP dedicated)
            return scheduleJoinGameCb("resetDedicatedServerCb", titleCb, GAMEMANAGER_ERR_TOPOLOGY_NOT_SUPPORTED);
        }

        BLAZE_SDK_DEBUGF("[GMGR] Reset dedicated server %s settings %d\n", params.mGameName.c_str(), params.mGameSettings.getBits());

        // Prepare request
        CreateGameRequest resetGameRequest;
        playerData.copyInto(resetGameRequest.getPlayerJoinData());
        prepareCommonGameData(resetGameRequest.getCommonGameData(), GAME_TYPE_GAMESESSION);

        resetGameRequest.getGameCreationData().setGameModRegister(params.mGameModRegister);
        resetGameRequest.getGameCreationData().setGameName(params.mGameName);
        resetGameRequest.getGameCreationData().setPresenceMode(params.mPresenceMode);
        resetGameRequest.getGameCreationData().getGameSettings().setBits( params.mGameSettings.getBits() );
        resetGameRequest.getGameCreationData().setNetworkTopology(params.mNetworkTopology);
        resetGameRequest.getGameCreationData().setVoipNetwork(params.mVoipTopology);
        setSlotCapacitiesVector(resetGameRequest.getSlotCapacities(), params.mPlayerCapacity);
        resetGameRequest.getGameCreationData().setQueueCapacity(params.mQueueCapacity);
        resetGameRequest.getGameCreationData().getGameAttribs().insert(params.mGameAttributes.begin(), params.mGameAttributes.end());
        resetGameRequest.getGameCreationData().setPermissionSystemName(params.mPermissionSystemName);
        resetGameRequest.getGameCreationData().getEntryCriteriaMap().insert(params.mEntryCriteriaMap.begin(), params.mEntryCriteriaMap.end());
#if defined(EA_PLATFORM_PS4) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        params.mExternalSessionStatus.copyInto(resetGameRequest.getGameCreationData().getExternalSessionStatus());
        params.mExternalSessionCustomData.copyInto(resetGameRequest.getGameCreationData().getExternalSessionCustomData());
#endif
#if defined(EA_PLATFORM_XBOXONE)|| defined(EA_PLATFORM_XBSX) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        resetGameRequest.getGameCreationData().getExternalSessionIdentSetup().getXone().setTemplateName(params.mSessionTemplateName.c_str());
        if (!params.mExternalSessionCustomDataString.empty())
            resetGameRequest.getGameCreationData().getExternalSessionCustomData().setData((uint8_t*)params.mExternalSessionCustomDataString.c_str(), params.mExternalSessionCustomDataString.length());
#endif
        validateGameSettings(params.mGameSettings);

        prepareTeamsForServerRequest(resetGameRequest.getPlayerJoinData(), params.mTeamIds, resetGameRequest.getGameCreationData().getTeamIds());

        // required defaults supplied on server if the following are unset by the client
        params.mRoleInformation.copyInto(resetGameRequest.getGameCreationData().getRoleInformation());

        uint32_t userIndex = getBlazeHub()->getPrimaryLocalUserIndex();
        UserManager::LocalUser* localUser = getBlazeHub()->getUserManager()->getLocalUser(userIndex);

        // set persisted id and secret to the request if the setting is true
        if (params.mGameSettings.getEnablePersistedGameId())
        {
            resetGameRequest.setPersistedGameId(params.mPersistedGameId);
            resetGameRequest.getPersistedGameIdSecret().setData(params.mPersistedGameIdSecret.getData(), params.mPersistedGameIdSecret.getCount());
        }

        if (resetGameRequest.getPlayerJoinData().getGroupId() == EA::TDF::OBJECT_ID_INVALID)
        {
            // send up our connection group id.
            if (localUser != nullptr)
            {
                resetGameRequest.getPlayerJoinData().setGroupId(localUser->getConnectionGroupObjectId());
            }
        }


        BlazeError err = validateTeamIndex(resetGameRequest.getGameCreationData().getTeamIds(), resetGameRequest.getPlayerJoinData());
        if (err != ERR_OK)
        {
            return scheduleJoinGameCb("resetDedicatedServerCb", titleCb, err);
        }

        err = validatePlayerJoinDataForCreateOrJoinGame(resetGameRequest.getPlayerJoinData());
        if (err != ERR_OK)
        {
            return scheduleJoinGameCb("resetDedicatedServerCb", titleCb, err);
        }

        JoinGameJob *resetGameJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "ResetDedicatedServerJob") JoinGameJob(this, titleCb, getBlazeHub()->getPrimaryLocalUserIndex());
        resetGameJob->setGameId(getBlazeHub()->getPrimaryLocalUserIndex(), INVALID_GAME_ID);

        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("ResetDedicatedServerJob", resetGameJob, titleCb.getObject());
        
        // Send off request to server; stuffing the title's cb functor into the rpc payload
        mGameManagerComponent->resetDedicatedServer(resetGameRequest, 
                                                    MakeFunctor(this, &GameManagerAPI::internalResetDedicatedServerCb), jobId); 
        return jobId;
    }

    JobId GameManagerAPI::resetDedicatedServer(const Game::ResetGameParameters &params, 
                                              const JoinGameCb &titleCb, 
                                              const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, 
                                              const UserGroup *userGroup /* = nullptr*/,
                                              const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        // loadFromCreateGameParameters
        PlayerJoinDataHelper::loadFromRoleNameToPlayerMap(playerData.getPlayerJoinData(), params.mPlayerRoles);
        PlayerJoinDataHelper::loadFromRoleNameToUserIdentificationMap(playerData.getPlayerJoinData(), params.mUserIdentificationRoles, true);
        playerData.getPlayerJoinData().setDefaultSlotType(params.mJoiningSlotType);
        playerData.getPlayerJoinData().setGameEntryType(params.mJoiningEntryType);
        playerData.getPlayerJoinData().setDefaultTeamIndex(params.mJoiningTeamIndex);

        if (userGroup != nullptr)
        {
            playerData.getPlayerJoinData().setGroupId(userGroup->getBlazeObjectId());
        }
        if (reservedPlayerIdentifications != nullptr)
        {
            PlayerJoinDataHelper::loadFromUserIdentificationList(playerData.getPlayerJoinData(), *reservedPlayerIdentifications, nullptr, true);
        }

        return resetDedicatedServer(params, playerData, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief internal callback for a resetGame rpc, on success we wait for an async join game notification
             (see onNotifyGameSetup)
    ***************************************************************************************************/
    void GameManagerAPI::internalResetDedicatedServerCb(const JoinGameResponse *response, BlazeError errorCode, JobId rpcJobId, JobId jobId)
    {    
        internalJoinGameCb(response, nullptr, errorCode, rpcJobId, jobId);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


    /*! ****************************************************************************/
    /*! \brief Create a new game or join an existing game.
    ********************************************************************************/
    JobId GameManagerAPI::createOrJoinGame(const Game::CreateGameParametersBase &params, 
                                           const PlayerJoinDataHelper& playerData,
                                           const JoinGameCb &titleCb)
    {
        // can't create a game if the network adapter isn't initialized
        BlazeAssert(mNetworkAdapter->isInitialized());
        if(!mNetworkAdapter->isInitialized())
        {
            return scheduleJoinGameCb("createOrJoinGameCb", titleCb, ERR_AUTHENTICATION_REQUIRED);
        }

        if (!getBlazeHub()->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
        {
            return scheduleJoinGameCb("createOrJoinGameCb", titleCb, SDK_ERR_QOS_PINGSITE_NOT_INITIALIZED);
        }

        // Prepare request
        CreateGameRequest createGameRequest;
        BlazeError err = prepareCreateGameRequest(params, playerData, createGameRequest);

        if (err != ERR_OK)
        {
            return scheduleJoinGameCb("createOrJoinGameCb", titleCb, err);
        }

        JoinGameJob *joinGameJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "CreateOrJoinGameJob") JoinGameJob(this, titleCb, getBlazeHub()->getPrimaryLocalUserIndex());
        joinGameJob->setGameId(getBlazeHub()->getPrimaryLocalUserIndex(), INVALID_GAME_ID);
        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("CreateOrJoinGameJob", joinGameJob, titleCb.getObject());
        
        // Send off request to server; stuffing the title's cb functor into the rpc payload
        mGameManagerComponent->createOrJoinGame(createGameRequest, MakeFunctor(this, &GameManagerAPI::internalCreateOrJoinGameCb), jobId); 
        return jobId;
    }
    JobId GameManagerAPI::createOrJoinGame(const Game::CreateGameParameters &params, 
                                              const JoinGameCb &titleCb, 
                                              const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, 
                                              const UserGroup *userGroup /* = nullptr*/,
                                              const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        // loadFromCreateGameParameters
        PlayerJoinDataHelper::loadFromRoleNameToPlayerMap(playerData.getPlayerJoinData(), params.mPlayerRoles);
        PlayerJoinDataHelper::loadFromRoleNameToUserIdentificationMap(playerData.getPlayerJoinData(), params.mUserIdentificationRoles, true);
        playerData.getPlayerJoinData().setDefaultSlotType(params.mJoiningSlotType);
        playerData.getPlayerJoinData().setGameEntryType(params.mJoiningEntryType);
        playerData.getPlayerJoinData().setDefaultTeamIndex(params.mJoiningTeamIndex);

        if (userGroup != nullptr)
        {
            playerData.getPlayerJoinData().setGroupId(userGroup->getBlazeObjectId());
        }
        if (reservedPlayerIdentifications != nullptr)
        {
            PlayerJoinDataHelper::loadFromUserIdentificationList(playerData.getPlayerJoinData(), *reservedPlayerIdentifications, nullptr, true);
        }

        return createOrJoinGame(params, playerData, titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief internal callback for a createOrJoinGame rpc, on success we wait for an async join game notification
             (see onNotifyGameSetup)
    ***************************************************************************************************/
    void GameManagerAPI::internalCreateOrJoinGameCb(const JoinGameResponse *response, BlazeError errorCode, JobId rpcJobId, JobId jobId)
    {    
        internalJoinGameCb(response, nullptr, errorCode, rpcJobId, jobId);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


    void GameManagerAPI::prepareTeamsForServerRequest(PlayerJoinData& joiningPlayerData, const TeamIdVector& teamVector, TeamIdVector &requestTeamIdVector) const
    {
        if (!teamVector.empty())
        {
            teamVector.copyInto(requestTeamIdVector);
        }
        else
        {
            // a game must have at least one team, override passed in team-related params
            // push in 1 team, with unspecified ID, have local users join it
            requestTeamIdVector.push_back(ANY_TEAM_ID);
            if (joiningPlayerData.getDefaultTeamIndex() == UNSPECIFIED_TEAM_INDEX)
            {
                joiningPlayerData.setDefaultTeamIndex(0);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief impl of GameNetworkListener::networkMeshCreated; dispatched by adapter when a game's network
            is first created (as a result of a call to either NetworkMeshAdapter::hostGame or NetworkMeshAdapter::joinGame)
    ***************************************************************************************************/
    void GameManagerAPI::networkMeshCreated( const Mesh *mesh, uint32_t userIndex, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError gameNetworkError )
    {
        // GM_AUDIT: we should collapse this listener with the functor we pass into the adapter.
        //  Side Note: onNetworkCreated doesn't appear to be usable by the game team, 
        //  since they don't have access to a game pointer yet (so no way to add themself as a listener)

        BLAZE_SDK_DEBUGF("[GMGR] GNA: GameManagerAPI::networkMeshCreated(Error: %d). Waiting for GameManagerAPI::createGameNetworkCb().\n", gameNetworkError);
        Game *game = (Game*)mesh;
        if (gameNetworkError == BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK)
        {
            game->setNetworkCreated();

            // GM_AUDIT: we have collapsed this listener with the functor we pass into the adapter, 
            // now the below method should be merged with this listener method
            // Side Note: onNetworkCreated doesn't appear to be usable by the game team, 
            // since they don't have access to a game pointer yet (so no way to add themself as a listener)
            createdGameNetwork(gameNetworkError, mesh->getId(), userIndex);

            // We notify title that the game network has been created.
            // The rest we will take care by the call back
            game->mDispatcher.dispatch("onNetworkCreated", &GameListener::onNetworkCreated, game);

            game->startTelemetryReporting();
        }
        else
        {
            // Adapter had an error initializing the game network

            // separate RPC to indicate host injection failed.
            if ( (game->isTopologyHost()) && (game->getGameSettings().getVirtualized()) )
            {
                EjectHostRequest request;
                request.setGameId(game->getId());
                mGameManagerComponent->ejectHost(request, 
                    MakeFunctor(this, &GameManagerAPI::internalEjectHostCb), game->getId());
            }

            BLAZE_SDK_DEBUGF("[GMGR] GNA: GameManagerAPI::networkMeshCreated event reports an error: %d\n", gameNetworkError);
            
            GameManagerApiJob *gameManagerApiJob = getJob(getBlazeHub()->getPrimaryLocalUserIndex(), game->getId());
            if (gameManagerApiJob != nullptr)
            {
                //GM_AUDIT: this is a no-op, as far as I know (we've already dispatched the title cb when the adapter called createGameNetworkCb) 
                //   so the GameManagerApiJob will always be nullptr
                gameManagerApiJob->cancel(ERR_SYSTEM);
                getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);                
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief allocate/init a new Game and insert it into our gameMap.  Caller should check the local
            player's type (queued, active), and init the game network or dispatch appropriately.

        \param[in] replicatedGameData the initial game data to use when creating the game
        \param[in] gameRoster the game's roster
        \param[in] gameQueue the game's queue
        \param[in] gameSetupReason - setup reason
        \param[in] qosSettings - the QoS settings to use when QoS validation is required
        \param[in] performQosOnActivePlayers if true, active players already in the roster will be set to have a qos test done during connection
        \param[in] lockableForPreferredJoins - whether the game will become locked for preferred joins, when
            players are removed from it. The lock blocks finding or joining the game via game browser and matchmaking,
            until its cleared via a server configured time out, or when all active players have called
            joinGameByUserList or preferredJoinOptOut in response to the lock, or the game becomes full.
        \param[in] gameAttributeName - the name of the game attribute that stores the game mode
    ***************************************************************************************************/
    Game* GameManagerAPI::createLocalGame(const ReplicatedGameData &replicatedGameData, const ReplicatedGamePlayerList &gameRoster, 
        const ReplicatedGamePlayerList &gameQueue, const GameSetupReason& gameSetupReason, const TimeValue& telemetryInterval, 
        const QosSettings& qosSettings, bool performQosOnActivePlayers, bool lockableForPreferredJoins, const char8_t* gameAttributeName)
    {
        GameId gameId = replicatedGameData.getGameId();

        GameMap::iterator gameMapIter = mGameMap.find(gameId);
        if (gameMapIter == mGameMap.end()) 
        {
            // allocate new game & insert it into our game map
            if (mApiParams.mMaxGameManagerGames > 0)
            {
                //  cap on the max number of concurrent games managed.
                BlazeAssertMsg(mGameMap.size() < mApiParams.mMaxGameManagerGames, "Exceeded game count memory cap specified in GameManagerApiParams");
            }

            Game* newGame = new (mGameMemoryPool.alloc()) Game(*this, &replicatedGameData, gameRoster, gameQueue, gameSetupReason, telemetryInterval, qosSettings, performQosOnActivePlayers, lockableForPreferredJoins, gameAttributeName, mMemGroup );
            mGameMap.insert(eastl::make_pair(replicatedGameData.getGameId(), newGame));

            BLAZE_SDK_DEBUGF("[GMGR] Local game(%" PRIu64 ":%s) created; UUID: %s.\n",gameId, replicatedGameData.getGameName(), replicatedGameData.getUUID());
            mDispatcher.dispatch("onGameCreated", &GameManagerAPIListener::onGameCreated, newGame);
            return newGame;
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GMGR] ERR: Cannot create a new local game(%" PRIu64 "), a local game already exists with that id.\n", gameId);
            BlazeAssert(false);
            return gameMapIter->second;
        }
    }


    // Set flag to indicate we are attempting to join the game's network, and join it if ready.
    // If for instance, awaiting CCS connectivity, block starting the network until ready.
    void GameManagerAPI::preInitGameNetwork( Game *game, uint32_t userIndex )
    {
        BLAZE_SDK_DEBUGF("[GMGR] GameManagerAPI::preInitGameNetwork by userIndex(%u) for game(%" PRIu64 ")\n", userIndex, game->getId());

        // Flag to indicate that we are in the process of attempting to join the games network.
        game->mIsLocalPlayerPreInitNetwork = true;

        resolveGameMembership(game, userIndex);
    }

    void GameManagerAPI::resolveGameMembership( Game *newGame, uint32_t userIndex, bool usingSelfOwnedPresence )
    {
        GameId newGameId = newGame->getId();

        uint32_t index = 0;
        
        // if the new game is a game group, don't leave old games.
        if (!newGame->isGameTypeGroup())
        {
            // iterate over the contents of mGameMap
            // we need to decide if we should depart from any pre-existing games
            while(index < mGameMap.size())
            {
                // mGameMap is a vector_map, need to do this because we may potentially invalidate iterators as games are destroyed
                GameMap::iterator gameIter = mGameMap.begin();
                eastl::advance(gameIter, index);

                GameId oldGameId = gameIter->first;
                if (oldGameId != newGameId)
                {
                    Game* oldGame = gameIter->second;
                    if (oldGame != nullptr && !oldGame->isGameTypeGroup())
                    {
                        bool removeOldGame = oldGame->resolveGameMembership(newGame);
                        if (removeOldGame)
                        {

                            // have to destroy if we're host of a dedicated server topology
                            if (oldGame->isDedicatedServerHost())
                            {
                                BLAZE_SDK_DEBUGF("[GMGR] resolveGameMembership for game(%" PRIu64 ") returned true, destroying game.\n", oldGameId);

                                // we fire and forget the destroyGame/ejectHost, because we're not waiting to tear down the old game
                                if (oldGame->getGameSettings().getVirtualized())
                                {
                                    EjectHostRequest ejectHostRequest;
                                    ejectHostRequest.setGameId(oldGameId);

                                    mGameManagerComponent->ejectHost(ejectHostRequest);
                                }
                                else
                                {
                                    DestroyGameRequest destroyGameRequest;
                                    destroyGameRequest.setGameId(oldGameId);
                                    destroyGameRequest.setDestructionReason(HOST_LEAVING);

                                    mGameManagerComponent->destroyGame(destroyGameRequest); 
                                }

                                // now destroy the local game so we can proceed with setup, this dispatches onGameDestructing to listeners
                                destroyLocalGame(oldGame, HOST_LEAVING, true, true);

                                // don't advance the index, since we just erased the old item at this index
                                continue;
                            }
                            else if ((oldGame->getLocalPlayer() != nullptr) && (oldGame->getLocalPlayer()->isActive()))
                            {
                                // if we're active in the game, clean things up. Maintaining a reservation or queue slot is acceptable
                                // we fire and forget the removePlayer, because we're not waiting to tear down the old game
                                BlazeAssertMsg((mUserManager->getLocalUser(userIndex) != nullptr), "Unable to find local user");
                                BLAZE_SDK_DEBUGF("[GMGR] resolveGameMembership for game(%" PRIu64 ") returned true, leaving and destroying local game.\n", oldGameId);
                                RemovePlayerRequest removePlayerRequest;
                                removePlayerRequest.setGameId(oldGameId);
                                removePlayerRequest.setPlayerId(mUserManager->getLocalUser(userIndex)->getId());
                                removePlayerRequest.setPlayerRemovedReason(PLAYER_LEFT_SWITCHED_GAME_SESSION);
                                mGameManagerComponent->removePlayer(removePlayerRequest);

                                // now destroy the local game so we can proceed with setup, this dispatches onGameDestructing to listeners
                                bool wasActive = ((oldGame->getLocalPlayer() != nullptr) ? oldGame->getLocalPlayer()->isActive() : true);
                                destroyLocalGame(oldGame, LOCAL_PLAYER_LEAVING, oldGame->isTopologyHost(), wasActive);

                                // don't advance the index, since we just erased the old item at this index
                                continue;
                            }

                        }
                    }
                }

                ++index;
            }
        }


        initGameNetwork(newGame, userIndex);
    }

    void GameManagerAPI::sendFinalizeGameCreation(GameId gameId, uint32_t userIndex)
    {
        UpdateGameSessionRequest updateGameSessionRequest;
        updateGameSessionRequest.setGameId(gameId);
        auto* gmComponent = mGameManagerComponent;
        if (EA_LIKELY(getBlazeHub()->getComponentManager(userIndex) != nullptr))
            gmComponent = getBlazeHub()->getComponentManager(userIndex)->getGameManagerComponent();

        gmComponent->finalizeGameCreation(updateGameSessionRequest,
            MakeFunctor(this, &GameManagerAPI::internalFinalizeGameCreationCb), gameId, userIndex);
    }

    /*! ************************************************************************************************/
    /*! \brief init the game's network (via NetworkMeshAdapter).
        GameManagerAPI::createGameNetworkCb dispatched when the network's setup.
        \param[in] game The game to init the peer network on
    ***************************************************************************************************/
    void GameManagerAPI::initGameNetwork(Game *game, uint32_t userIndex)
    {
        // This check ensures that all required checks have been kicked off before we check that they are all done.
        if (!game->mIsLocalPlayerPreInitNetwork)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Game(%" PRIu64 ") delaying initNetworkMesh until local player has attempted to join the game's network.\n", game->getId());
            return;
        }

        if (!game->hasFullCCSConnectivity())
        {
            game->mDelayingInitGameNetworkForCCS = true;  // DelayingForCCSConnectivity

            BLAZE_SDK_DEBUGF("[GMGR] Game(%" PRIu64 ") delaying initNetworkMesh until all end points have hosted connectivity in a hostedOnly or crossplay game.\n", game->getId());
            return;
        }
        game->mDelayingInitGameNetworkForCCS = false;

        // set up the network 
        BlazeNetworkAdapter::NetworkMeshAdapter::Config config;

        // Configure the QoS settings to be used for the NetGameLinks
        config.QosDurationInMs = game->getQosDuration();
        config.QosIntervalInMs = game->getQosInterval();
        config.QosPacketSize = game->getQosPacketSize();
        config.CreatorUserIndex = userIndex;

        game->getNetworkMeshHelper().initNetworkMesh(game, config);

        // reset the joining flag
        game->mIsLocalPlayerPreInitNetwork = false;
        game->mIsLocalPlayerInitNetwork = true;
    }


    bool GameManagerAPI::setupPlayerJoinData(PlayerJoinDataHelper& playerData, uint32_t userIndex,
        SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, const UserGroup *userGroup /*= nullptr*/, 
        TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, GameEntryType entryType /*= GAME_ENTRY_TYPE_DIRECT*/,
        const Blaze::GameManager::PlayerIdList *additionalUsersList /*= nullptr*/, const RoleNameToPlayerMap *joiningRoles /*= nullptr*/, 
        const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/)
    {
        playerData.getPlayerJoinData().setDefaultSlotType(slotType);
        playerData.getPlayerJoinData().setDefaultTeamIndex(teamIndex);
        playerData.getPlayerJoinData().setGameEntryType(entryType);

        RoleName defaultRole = PLAYER_ROLE_NAME_DEFAULT;
        if (joiningRoles != nullptr && joiningRoles->size() == 1 && 
            joiningRoles->begin()->second->size() == 1 && 
            *joiningRoles->begin()->second->begin() == INVALID_BLAZE_ID)
        {
            defaultRole = joiningRoles->begin()->first;
        }

        // set player attribs, if any are specified
        UserManager::LocalUser* localUser = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
        if (localUser == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] invalid join game attempt: Local user at index (%d) is nullptr. Make sure that the user is logged in, before attempting a join.\n", userIndex);
            return false;
        }

        if ((userGroup != nullptr) && (additionalUsersList != nullptr))
        {
            BLAZE_SDK_DEBUGF("[GMGR] invalid join game attempt: cannot specify both a user group and additional users list.\n");
            return false;
        }

        // Set the requested roles for each specified player
        if (joiningRoles != nullptr)
        {
            PlayerJoinDataHelper::loadFromRoleNameToPlayerMap(playerData.getPlayerJoinData(), *joiningRoles);
            if (validatePlayerJoinDataForCreateOrJoinGame(playerData.getPlayerJoinData()) != ERR_OK)
            {
                BLAZE_SDK_DEBUGF("[GMGR] invalid join game attempt: player join data validation failed\n");
                return false;
            }
        }

        if (userGroup != nullptr)
        {
            playerData.getPlayerJoinData().setGroupId(userGroup->getBlazeObjectId());
        }
        else
        {
            if (additionalUsersList != nullptr)
            {
                // For any additional players, set the default role for them (if they don't already have any roles)
                PlayerJoinDataHelper::loadFromPlayerIdList(playerData.getPlayerJoinData(), *additionalUsersList, &defaultRole);
            }
        }

        if (reservedPlayerIdentifications != nullptr)
        {
            if (userIdentificationJoiningRoles != nullptr)
            {
                PlayerJoinDataHelper::loadFromRoleNameToUserIdentificationMap(playerData.getPlayerJoinData(), *userIdentificationJoiningRoles, true);
            }

            PlayerJoinDataHelper::loadFromUserIdentificationList(playerData.getPlayerJoinData(), *reservedPlayerIdentifications, &defaultRole, true);
        }

        // If we haven't specified any roles, add the default role
        const PerPlayerJoinData* ppJoinData = PlayerJoinDataHelper::getPlayerJoinData(playerData.getPlayerJoinData(), localUser->getUser()->getId());
        if (ppJoinData != nullptr && !ppJoinData->getRoles().empty())
            playerData.addPlayer(localUser->getUser(), nullptr, playerAttributeMap);
        else
            playerData.addPlayer(localUser->getUser(), &defaultRole, playerAttributeMap);

        return true;
    }

    /*! ****************************************************************************/
    /*! \brief Joins the given user's game.
    ********************************************************************************/
    Blaze::JobId GameManagerAPI::joinGameByUser(const UserManager::User *user, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Joining game by user(%" PRId64 ":%s)\n", user->getId(), user->getName());
        
        if (joinMethod >= SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME)
        {
            return scheduleJoinGameCb("joinGameByUserCb", callbackFunctor, GAMEMANAGER_ERR_INVALID_JOIN_METHOD);
        }

        UserIdentification userIdentification;
        userIdentification.setBlazeId(user->getId());
        user->getPlatformInfo().copyInto(userIdentification.getPlatformInfo());

        return joinGameInternal(getBlazeHub()->getPrimaryLocalUserIndex(), &userIdentification, 0, joinMethod, playerData, callbackFunctor,
            DEFAULT_JOINING_SLOT, 0, 0, false, gameType);
    }

    Blaze::JobId GameManagerAPI::joinGameByUser( const UserManager::User *user, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, 
        SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, const UserGroup *userGroup /*= nullptr*/, 
        TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, GameEntryType entryType /*= GAME_ENTRY_TYPE_DIRECT*/,
        const Blaze::GameManager::PlayerIdList *additionalUsersList /*= nullptr*/, const RoleNameToPlayerMap *joiningRoles /*= nullptr*/, 
        GameType gameType /*= GAME_TYPE_GAMESESSION*/,
        const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        if (!setupPlayerJoinData(playerData, getBlazeHub()->getPrimaryLocalUserIndex(), slotType, playerAttributeMap, userGroup, teamIndex, entryType, 
                                 additionalUsersList, joiningRoles, reservedPlayerIdentifications, userIdentificationJoiningRoles))
        {
            return scheduleJoinGameCb("joinGameByUserCb", callbackFunctor, ERR_SYSTEM);
        }

        return joinGameByUser(user, joinMethod, playerData, callbackFunctor, gameType);
    }

    /*! ****************************************************************************/
    /*! \brief Joins the given user's game by blazeId.
    ********************************************************************************/
    Blaze::JobId GameManagerAPI::joinGameByBlazeId(BlazeId id, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Joining game by blazeId(%" PRId64 ")\n", id);

        if (joinMethod >= SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME)
        {
            return scheduleJoinGameCb("joinGameByBlazeIdCb", callbackFunctor, GAMEMANAGER_ERR_INVALID_JOIN_METHOD);
        }

        UserIdentification userIdentification;
        userIdentification.setBlazeId(id);

        return joinGameInternal(getBlazeHub()->getPrimaryLocalUserIndex(), &userIdentification, 0, joinMethod, playerData, callbackFunctor,
            DEFAULT_JOINING_SLOT, 0, 0, false, gameType);
    }

    Blaze::JobId GameManagerAPI::joinGameByBlazeId( BlazeId id, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, 
        const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, const UserGroup *userGroup /*= nullptr*/, TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, 
        GameEntryType entryType /*= GAME_ENTRY_TYPE_DIRECT*/, const Blaze::GameManager::PlayerIdList *additionalUsersList /*= nullptr*/, 
        const RoleNameToPlayerMap *joiningRoles /*= nullptr*/, GameType gameType /*= GAME_TYPE_GAMESESSION*/,
        const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        if (!setupPlayerJoinData(playerData, getBlazeHub()->getPrimaryLocalUserIndex(), slotType, playerAttributeMap, userGroup, teamIndex, entryType, 
                                 additionalUsersList, joiningRoles, reservedPlayerIdentifications, userIdentificationJoiningRoles))
        {
            return scheduleJoinGameCb("joinGameByBlazeIdCb", callbackFunctor, ERR_SYSTEM);
        }

        return joinGameByBlazeId(id, joinMethod, playerData, callbackFunctor, gameType);
    }

    /*! ****************************************************************************/
    /*! \brief Joins the given user's game by name (persona/1st party)
    ********************************************************************************/
    Blaze::JobId GameManagerAPI::joinGameByUsername(const char8_t *username, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Joining game by username(%s)\n", username);
        
        if (joinMethod >= SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME)
        {
            return scheduleJoinGameCb("joinGameByUsernameCb", callbackFunctor, GAMEMANAGER_ERR_INVALID_JOIN_METHOD);
        }

        UserIdentification userIdentification;
        userIdentification.setName(username);

        return joinGameInternal(getBlazeHub()->getPrimaryLocalUserIndex(), &userIdentification, 0, joinMethod, playerData, callbackFunctor,
            DEFAULT_JOINING_SLOT, 0, 0, false, gameType);
    }
    Blaze::JobId GameManagerAPI::joinGameByUsername( const char8_t *username, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, 
        const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, const UserGroup *userGroup /*= nullptr*/, TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, 
        GameEntryType entryType /*= GAME_ENTRY_TYPE_DIRECT*/, const Blaze::GameManager::PlayerIdList *additionalUsersList /*= nullptr*/, 
        const RoleNameToPlayerMap *joiningRoles /*= nullptr*/, GameType gameType /*= GAME_TYPE_GAMESESSION*/,
        const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        if (!setupPlayerJoinData(playerData, getBlazeHub()->getPrimaryLocalUserIndex(), slotType, playerAttributeMap, userGroup, teamIndex, entryType, 
                                 additionalUsersList, joiningRoles, reservedPlayerIdentifications, userIdentificationJoiningRoles))
        {
            return scheduleJoinGameCb("joinGameByUsernameCb", callbackFunctor, ERR_SYSTEM);
        }

        return joinGameByUsername(username, joinMethod, playerData, callbackFunctor, gameType);
    }


    /*! ****************************************************************************/
    /*! \brief Joins a game by GameId
    ********************************************************************************/
    Blaze::JobId GameManagerAPI::joinGameById(GameId gameId, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Joining game(%" PRIu64 ")\n", gameId);

        // Extra check of JoinMethod. Note JOIN_BY_PLAYER is allowed here as you can be joining via a user's gamer card to a specific game.
        if (joinMethod >= SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME)
        {
            return scheduleJoinGameCb("joinGameByIdCb", callbackFunctor, GAMEMANAGER_ERR_INVALID_JOIN_METHOD);
        }

        // validate that we're not already in that game
        Game *localGame = (gameId != GameManager::INVALID_GAME_ID) ? getGameById( gameId ) : nullptr;
        if (EA_UNLIKELY(localGame != nullptr))
        {
            // we already have a local version of that game; check if I'm already a member of it
            uint32_t userIndex = getBlazeHub()->getUserManager()->getPrimaryLocalUserIndex();
            if (mUserManager->getLocalUser(userIndex) == nullptr)
            {
                return scheduleJoinGameCb("joinGameByIdCb", callbackFunctor, GAMEMANAGER_ERR_PLAYER_NOT_FOUND);
            }
            PlayerId localPlayerId = mUserManager->getLocalUser(userIndex)->getId();
            Player *localPlayer = localGame->getPlayerById(localPlayerId);
            // joinGame is also used to claim a reservation, so allow if player reserved.
            if (localPlayer != nullptr && !localPlayer->isReserved())
            {
                return scheduleJoinGameCb("joinGameByIdCb", callbackFunctor, GAMEMANAGER_ERR_ALREADY_GAME_MEMBER);
            }
        }

        return joinGameInternal(getBlazeHub()->getPrimaryLocalUserIndex(), nullptr, gameId, joinMethod, playerData, callbackFunctor,
            DEFAULT_JOINING_SLOT, 0, 0, false, GAME_TYPE_GAMESESSION);
    }
    Blaze::JobId GameManagerAPI::joinGameById( GameId gameId, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, 
        SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, const UserGroup *userGroup /*= nullptr*/, 
        TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, GameEntryType entryType /*= GAME_ENTRY_TYPE_DIRECT*/,
        const Blaze::GameManager::PlayerIdList *additionalUsersList /*= nullptr*/, const RoleNameToPlayerMap *joiningRoles /*= nullptr*/,
        const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        if (!setupPlayerJoinData(playerData, getBlazeHub()->getPrimaryLocalUserIndex(), slotType, playerAttributeMap, userGroup, teamIndex, entryType, 
                                 additionalUsersList, joiningRoles, reservedPlayerIdentifications, userIdentificationJoiningRoles))
        {
            return scheduleJoinGameCb("joinGameByIdCb", callbackFunctor, ERR_SYSTEM);
        }

        return joinGameById(gameId, joinMethod, playerData, callbackFunctor);
    }

    /*! ****************************************************************************/
    /*! \brief Joins a game by GameId, attempting to join a specific slot. For internal dev use only.
    ********************************************************************************/
    Blaze::JobId GameManagerAPI::joinGameByIdWithSlotId( GameId gameId, JoinMethod joinMethod, SlotId slotId,const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Joining game(%" PRIu64 ") with slot (%u)\n", gameId, slotId);

        // Extra check of join by player when joining by game id.  Join by player does not make sense here
        // as we are not specifying any player information.
        if (joinMethod == JOIN_BY_PLAYER || joinMethod >= SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME)
        {
            return scheduleJoinGameCb("joinGameByIdWithSlotIdCb", callbackFunctor, GAMEMANAGER_ERR_INVALID_JOIN_METHOD);
        }

        // validate that we're not already in that game
        Game *localGame = (gameId != GameManager::INVALID_GAME_ID) ? getGameById( gameId ) : nullptr;
        if (EA_UNLIKELY(localGame != nullptr))
        {
            // we already have a local version of that game; check if I'm already a member of it
            uint32_t userIndex = getBlazeHub()->getUserManager()->getPrimaryLocalUserIndex();
            if (mUserManager->getLocalUser(userIndex) == nullptr)
            {
                return scheduleJoinGameCb("joinGameByIdWithSlotIdCb", callbackFunctor, GAMEMANAGER_ERR_PLAYER_NOT_FOUND);
            }
            PlayerId localPlayerId = mUserManager->getLocalUser(userIndex)->getId();
            Player *localPlayer = localGame->getPlayerById(localPlayerId);
            // joinGame is also used to claim a reservation, so allow if player reserved.
            if (localPlayer != nullptr && !localPlayer->isReserved())
            {
                JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
                JobId jobId = jobScheduler->reserveJobId();
                jobId = jobScheduler->scheduleFunctor("joinGameByIdWithSlotIdCb", callbackFunctor, GAMEMANAGER_ERR_ALREADY_GAME_MEMBER, jobId, localGame, "", 
                    this, 0, jobId); // assocObj, delayMs, reservedJobId
                Job::addTitleCbAssociatedObject(jobScheduler, jobId, callbackFunctor);
                return jobId;
            }
        }

        return joinGameInternal(getBlazeHub()->getPrimaryLocalUserIndex(), nullptr, gameId, joinMethod, playerData, callbackFunctor,
            slotId, 0, 0, false, GAME_TYPE_GAMESESSION);
    }
    Blaze::JobId GameManagerAPI::joinGameByIdWithSlotId( GameId gameId, JoinMethod joinMethod, const JoinGameCb &callbackFunctor, SlotId slotId,
        SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, const UserGroup *userGroup /*= nullptr*/, 
        TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, GameEntryType entryType /*= GAME_ENTRY_TYPE_DIRECT*/,
        const Blaze::GameManager::PlayerIdList *additionalUsersList /*= nullptr*/, const RoleNameToPlayerMap *joiningRoles /*= nullptr*/, 
        const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        if (!setupPlayerJoinData(playerData, getBlazeHub()->getPrimaryLocalUserIndex(), slotType, playerAttributeMap, userGroup, teamIndex, entryType, 
                                 additionalUsersList, joiningRoles, reservedPlayerIdentifications, userIdentificationJoiningRoles))
        {
            return scheduleJoinGameCb("joinGameByIdWithSlotIdCb", callbackFunctor, ERR_SYSTEM);
    }

        return joinGameByIdWithSlotId(gameId, joinMethod, slotId, playerData, callbackFunctor);
    }

    Blaze::JobId GameManagerAPI::joinGameLocalUser( uint32_t userIndex, GameId gameId, const PlayerJoinDataHelper& playerData, const JoinGameCb &callbackFunctor, GameType gameType)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Joining game(%" PRIu64 ") for local user at index(%u)\n", gameId, userIndex);

        // User must be a local user, that is logged in.
        UserManager::LocalUser* localUser = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
        if (EA_UNLIKELY((userIndex >= getBlazeHub()->getNumUsers()) || (localUser == nullptr)))
        {
            return scheduleJoinGameCb("joinGameLocalUserCb", callbackFunctor, GAMEMANAGER_ERR_PLAYER_NOT_FOUND);
        }

        // Game must already be known locally.
        if (EA_UNLIKELY(getGameById(gameId) == nullptr))
        {
            return scheduleJoinGameCb("joinGameLocalUserCb", callbackFunctor, GAMEMANAGER_ERR_INVALID_GAME_ID);
        }

        RoleNameToPlayerMap joiningRoleMap;
        // validate that we're not already in that game
        Game *localGame = (gameId != GameManager::INVALID_GAME_ID) ? getGameById( gameId ) : nullptr;
        if (EA_UNLIKELY(localGame != nullptr))
        {
            // we already have a local version of that game; check if I'm already a member of it
            PlayerId localPlayerId = localUser->getId();
            Player *localPlayer = localGame->getPlayerById(localPlayerId);
            // joinGame is also used to claim a reservation, so allow if player reserved.
            if (localPlayer != nullptr && !localPlayer->isReserved())
            {
                JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
                JobId jobId = jobScheduler->reserveJobId();
                jobId = jobScheduler->scheduleFunctor("joinGameLocalUserCb", callbackFunctor, GAMEMANAGER_ERR_ALREADY_GAME_MEMBER, jobId, localGame, "",
                    this, 0, jobId); // assocObj, delayMs, reservedJobId
                Job::addTitleCbAssociatedObject(jobScheduler, jobId, callbackFunctor);
                return jobId;
            }

            // Check that the primary local player is part of the game: 
            Player *primaryLocalPlayer = localGame->getLocalPlayer();
            if (primaryLocalPlayer != localPlayer && (primaryLocalPlayer == nullptr || primaryLocalPlayer->isReserved()))
            {
                JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
                JobId jobId = jobScheduler->reserveJobId();
                jobId = jobScheduler->scheduleFunctor("joinGameLocalUserCb", callbackFunctor, GAMEMANAGER_ERR_MISSING_PRIMARY_LOCAL_PLAYER, jobId, localGame, "",
                    this, 0, jobId); // assocObj, delayMs, reservedJobId
                Job::addTitleCbAssociatedObject(jobScheduler, jobId, callbackFunctor);
                return jobId;
            }
        }

        // Side: we use a special internal JOIN_BY_FOLLOWLOCALUSER JoinMethod rather than JOIN_BY_PLAYER, this
        // allows restrictions on MLU players to be enforced by title independent of the game settings.

        // switch the component to be the specified index.
        mGameManagerComponent = getBlazeHub()->getComponentManager(userIndex)->getGameManagerComponent();
        bool localUserJoin = true;
        JobId jobId = joinGameInternal(userIndex, nullptr, gameId, SYS_JOIN_BY_FOLLOWLOCALUSER, playerData, callbackFunctor,
            DEFAULT_JOINING_SLOT, 0, 0, localUserJoin, gameType);

        // switch the component back out the primary index.
        mGameManagerComponent = getBlazeHub()->getComponentManager(getBlazeHub()->getPrimaryLocalUserIndex())->getGameManagerComponent();

        return jobId;
    }
    Blaze::JobId GameManagerAPI::joinGameLocalUser( uint32_t userIndex, GameId gameId, const JoinGameCb &callbackFunctor, SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, 
        const Collections::AttributeMap *playerAttributeMap /*= nullptr*/, TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, 
        GameEntryType entryType /*= GAME_ENTRY_TYPE_DIRECT*/, const RoleName roleName /*= PLAYER_ROLE_NAME_DEFAULT*/, 
        GameType gameType /*= GAME_TYPE_GAMESESSION*/,
        const Blaze::UserIdentificationList *reservedPlayerIdentifications /*= nullptr*/, const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/)
    {
        PlayerJoinDataHelper playerData;
        if (!setupPlayerJoinData(playerData, userIndex, slotType, playerAttributeMap, nullptr, teamIndex, entryType, 
                                 nullptr, nullptr, reservedPlayerIdentifications, userIdentificationJoiningRoles))
        {
            return scheduleJoinGameCb("joinGameLocalUserCb", callbackFunctor, ERR_SYSTEM);
        }

        return joinGameLocalUser(userIndex, gameId, playerData, callbackFunctor, gameType);
    }

    /*! ************************************************************************************************/
    /*! \brief internal helper for joining a game - all joinGame overloads call this method.  Also
        called when group members are notified to join a game.

            It sends up either a joinGame or joinGameByGroup RPC (depending if this is a multiuser join)
    ***************************************************************************************************/
    JobId GameManagerAPI::joinGameInternal(uint32_t userIndex, const UserIdentification* userIdentification, GameId gameId, JoinMethod joinMethod, 
            const PlayerJoinDataHelper& playerData, const JoinGameCb &titleCb, 
            SlotId slotId /*= DEFAULT_JOINING_SLOT*/,
            int32_t sessionStart /*= 0*/, int32_t sessionLength /*= 0*/, bool localUserJoin /*= false*/, GameType gameType /*= GAME_TYPE_GAMESESSION*/)
    {
        // can't join a game if the network adapter isn't initialized
        BlazeAssert(mNetworkAdapter->isInitialized());
        if (!mNetworkAdapter->isInitialized())
        {
            return scheduleJoinGameCb("joinGameInternalCb", titleCb, ERR_AUTHENTICATION_REQUIRED);
        }

        if (!getBlazeHub()->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
        {
            return scheduleJoinGameCb("joinGameInternalCb", titleCb, SDK_ERR_QOS_PINGSITE_NOT_INITIALIZED);
        }

        BlazeAssert(playerData.getSlotType() < MAX_SLOT_TYPE);

        // Set-up joinGame request
        JoinGameRequest joinGameRequest;
        prepareCommonGameData(joinGameRequest.getCommonGameData(), gameType);

        if (gameId != GameManager::INVALID_GAME_ID)
        {
            joinGameRequest.setGameId(gameId);
        }
        else
        {
            if (userIdentification != nullptr)
            {
                joinGameRequest.getUser().setBlazeId(userIdentification->getBlazeId());
                joinGameRequest.getUser().setName(userIdentification->getName());
                userIdentification->getPlatformInfo().copyInto(joinGameRequest.getUser().getPlatformInfo());
            }
            else
            {
                //no user identification provided, and invalid game id provided
                return scheduleJoinGameCb("joinGameInternalCb", titleCb, GAMEMANAGER_ERR_INVALID_GAME_ID);
            }
        }

        joinGameRequest.setJoinMethod(joinMethod);
        playerData.copyInto(joinGameRequest.getPlayerJoinData());

        // for DS dev
        joinGameRequest.setRequestedSlotId(slotId);

        if (joinGameRequest.getPlayerJoinData().getGroupId() == EA::TDF::OBJECT_ID_INVALID)
        {
            // Local user joins should only act on the particular player.
            if (!localUserJoin)
            {
                // send up our connection group id.
                uint32_t primaryUserIndex = getBlazeHub()->getPrimaryLocalUserIndex();
                UserManager::LocalUser* primaryLocalUser = getBlazeHub()->getUserManager()->getLocalUser(primaryUserIndex);
                if (primaryLocalUser != nullptr)
                {
                    joinGameRequest.getPlayerJoinData().setGroupId(primaryLocalUser->getConnectionGroupObjectId());
                }
            }
        }

        // the join game job is created to provide a job id SDK users can cancel a join game request with
        // if the job is canceled, it triggers the titleCb with SDK_ERR_RPC_CANCELED and a nullptr game pointer
        // gamemanagerAPI then handles the onGameJoined notification in the background and dispatches a leave
        // game RPC.
        JobId jobId = INVALID_JOB_ID;

        JoinGameJob *joinGameJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "JoinGameJob") JoinGameJob(this, titleCb, userIndex);
        joinGameJob->setGameId(getBlazeHub()->getPrimaryLocalUserIndex(), INVALID_GAME_ID);
        jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("JoinGameJob", joinGameJob, this);

        mGameManagerComponent->joinGame(joinGameRequest,
            MakeFunctor(this, &GameManagerAPI::internalJoinGameCb), jobId);

        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief internal RPC callback for joinGame; on success, we wait for a join game notification
            (see onNotifyGameSetup).
    ***************************************************************************************************/
    void GameManagerAPI::internalJoinGameCb(const JoinGameResponse *response, const Blaze::EntryCriteriaError *error, 
            BlazeError errorCode, JobId rpcJobId, JobId jobId)
    {        
        // check if user canceled the joinGameJob or MM Session
        JoinGameJob *joinGameJob = (JoinGameJob*)getBlazeHub()->getScheduler()->getJob(jobId);
        bool joinGameJobCanceled = (joinGameJob == nullptr);
        
        if ( joinGameJobCanceled)
        {
            // job/session canceled by user, if the join succeeded, leave the game (silently)
            uint32_t userIndex = getBlazeHub()->getUserManager()->getPrimaryLocalUserIndex();
            if ((errorCode == ERR_OK) && (mUserManager->getLocalUser(userIndex) != nullptr))
            {
                BLAZE_SDK_DEBUGF("[GMGR] Game join succeeded for canceled action; leaving game silently.\n");
                RemovePlayerRequest removePlayerRequest;
                removePlayerRequest.setGameId( response->getGameId() );
                removePlayerRequest.setPlayerId(mUserManager->getLocalUser(userIndex)->getId());
                removePlayerRequest.setPlayerRemovedReason(PLAYER_LEFT_CANCELLED_MATCHMAKING);
                mGameManagerComponent->removePlayer(removePlayerRequest);
                // GM_AUDIT: GOS-4488: if this was a PG game setup, we should probably do a PG leave game
            }
            else
            {
                BLAZE_SDK_DEBUGF("[GMGR] Game join failed for canceled join (no action taken).\n");
            }
           
            return;
        }

        // handle blazeServer joinGame response
        if (errorCode != ERR_OK || response->getJoinState() == NO_ONE_JOINED || response->getIsExternalCaller())    // This happens when creating virtual games without reservations
        {
            // Note: the assumption here is that the game has not been joined (normal server error).  
            // RPC timeouts however will fall through to here also, and since we know nothing of the game at this point
            // we are unable to silently leave it, but we need to dispatch title callback.
            BLAZE_SDK_DEBUGF("[GMGR] joinGameJob(%u): join failed; job finished.\n", jobId.get());
            //  need to explicitly pass back entry criteria in callback - so override the default cancel behavior.
            JoinGameCb titleCb;
            joinGameJob->getTitleCb(titleCb);
            if (errorCode == ERR_OK)
            {
                joinGameJob->setGameId(joinGameJob->getUserIndex(), response->getGameId());
            }
            const char8_t *joinCriteriaFailureMessage = (error != nullptr) ? error->getFailedCriteria() : "";
            titleCb(errorCode, jobId, nullptr, joinCriteriaFailureMessage);
            getBlazeHub()->getScheduler()->removeJob(jobId);
            return;
        }

        // joinGame rpc success, (job/setup not canceled or if there).
        joinGameJob->setGameId(joinGameJob->getUserIndex(), response->getGameId());
        joinGameJob->setReservedExternalPlayerIdentifications(response->getJoinedReservedPlayerIdentifications());

        // execution continues from onNotifyGameSetup
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ****************************************************************************/
    /*! \brief Joins the specified list of users to the game. Does not join caller.
    ********************************************************************************/
    Blaze::JobId GameManagerAPI::joinGameByUserList(GameId gameId, JoinMethod joinMethod, const PlayerJoinDataHelper& playerData, 
                                                    const JoinGameCb &callbackFunctor, GameType gameType /*= GAME_TYPE_GAMESESSION*/)
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
        const Game *game = getGameById(gameId);
        if (game == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Local user must be a member and have local game object for the game(id %" PRIu64 ") before calling join game by user list. Local game object was nullptr.\n", gameId);
            return scheduleJoinGameCb("joinGameByUserListCb", callbackFunctor, GAMEMANAGER_ERR_INVALID_GAME_ID);
        }

        if (playerData.getPlayerDataList().empty())
        {
            BLAZE_SDK_DEBUGF("[GMGR] invalid empty user list specified in join game(id %" PRIu64 "): you must add users to list.\n", gameId);
            return scheduleJoinGameCb("joinGameByUserListCb", callbackFunctor, GAMEMANAGER_ERR_PLAYER_NOT_FOUND);
        }

        UserIdentification tempLocalUserIdent;
        getBlazeHub()->getUserManager()->getPrimaryLocalUser()->getUser()->copyIntoUserIdentification(tempLocalUserIdent);
        for (PerPlayerJoinDataList::const_iterator itr = playerData.getPlayerDataList().begin(); itr != playerData.getPlayerDataList().end(); ++itr)
        {
            if (PlayerJoinDataHelper::areUserIdsEqual((*itr)->getUser(), tempLocalUserIdent))
            {
                BLAZE_SDK_DEBUGF("[GMGR] invalid arguments for join game(id %" PRIu64 "): you cannot include yourself in the reserved players list, for join by user list.\n", gameId);
                return scheduleJoinGameCb("joinGameByUserListCb", callbackFunctor, GAMEMANAGER_ERR_INVALID_PLAYER_PASSEDIN);
            }
        }

        BLAZE_SDK_DEBUGF("[GMGR] Joining game(id %" PRIu64 ") by reserved list of size(%" PRIsize ")\n", gameId, (size_t)playerData.getPlayerDataList().size());

        JoinGameByUserListRequest joinGameRequest;
        prepareCommonGameData(joinGameRequest.getCommonGameData(), gameType);

        playerData.copyInto(joinGameRequest.getPlayerJoinData());

        joinGameRequest.setGameId(gameId);
        joinGameRequest.getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_MAKE_RESERVATION);
        joinGameRequest.setJoinMethod(joinMethod);

        uint32_t userIndex = getBlazeHub()->getUserManager()->getPrimaryLocalUserIndex();

        JoinGameByUserListJob *joinGameJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "JoinGameByUserListJob") JoinGameByUserListJob(this, callbackFunctor, userIndex);
        JobId jobId = jobScheduler->scheduleJobNoTimeout("JoinGameByUserListJob", joinGameJob, this);

        mGameManagerComponent->joinGameByUserList(joinGameRequest, MakeFunctor(this, &GameManagerAPI::internalJoinGameByUserListCb), jobId);
        return jobId;
    }

    Blaze::JobId GameManagerAPI::joinGameByUserList(GameId gameId, JoinMethod joinMethod, const JoinGameCb &callbackFunctor,
        const Blaze::UserIdentificationList& reservedPlayerIdentifications,
        SlotType slotType /*= SLOT_PUBLIC_PARTICIPANT*/, TeamIndex teamIndex /*= UNSPECIFIED_TEAM_INDEX*/, GameType gameType /*= GAME_TYPE_GAMESESSION*/,
        const RoleNameToUserIdentificationMap *userIdentificationJoiningRoles /*= nullptr*/)
    {
        // This is a weird hack to allow you to specify a common RoleName for each member of the joining group:
        RoleName defaultRole = PLAYER_ROLE_NAME_DEFAULT;
        if (userIdentificationJoiningRoles != nullptr && userIdentificationJoiningRoles->size() == 1 && 
            userIdentificationJoiningRoles->begin()->second->size() == 1 && 
            PlayerJoinDataHelper::isUserIdDefault(*(*userIdentificationJoiningRoles->begin()->second->begin())))
        {
            defaultRole = userIdentificationJoiningRoles->begin()->first;
        }

        PlayerJoinDataHelper playerData;
        playerData.setSlotType(slotType);
        playerData.setTeamIndex(teamIndex);
        playerData.setGameEntryType(GAME_ENTRY_TYPE_MAKE_RESERVATION);

        // roles for external players
        if (userIdentificationJoiningRoles != nullptr)
        {
            PlayerJoinDataHelper::loadFromRoleNameToUserIdentificationMap(playerData.getPlayerJoinData(), *userIdentificationJoiningRoles, true);
        }
        PlayerJoinDataHelper::loadFromUserIdentificationList(playerData.getPlayerJoinData(), reservedPlayerIdentifications, &defaultRole);

        return joinGameByUserList(gameId, joinMethod, playerData, callbackFunctor, gameType);
    }

    void GameManagerAPI::internalJoinGameByUserListCb(const JoinGameResponse *response, const Blaze::EntryCriteriaError *criteriaError, 
        BlazeError errorCode, JobId rpcJobId, JobId jobId)
    {
        JoinGameByUserListJob *joinGameJob = (JoinGameByUserListJob*)getBlazeHub()->getScheduler()->getJob(jobId);
        bool joinGameJobCanceled = (joinGameJob == nullptr);
        if ( joinGameJobCanceled)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Game join by user list %s, while job was canceled. %s\n", ((errorCode == ERR_OK)? "succeeded":"failed"), ((errorCode == ERR_OK)? ". Warning: Users may still be joined to game" : ""));
            return;
        }

        // handle blazeServer joinGame response
        if (errorCode != ERR_OK || response->getJoinState() == NO_ONE_JOINED)    // NO_ONE_JOINED happens when creating MLU external players are joining
        {
            BLAZE_SDK_DEBUGF("[GMGR] joinGameByUserListJob(%u): join failed; job finished.\n", jobId.get());
            //  need to explicitly pass back entry criteria in callback - so override the default cancel behavior.
            JoinGameCb titleCb;
            joinGameJob->getTitleCb(titleCb);
            if (errorCode == ERR_OK)
            {
                joinGameJob->setGameId(joinGameJob->getUserIndex(), response->getGameId());
            }
            const char8_t *joinCriteriaFailureMessage = (criteriaError != nullptr) ? criteriaError->getFailedCriteria() : "";
            titleCb(errorCode, jobId, nullptr, joinCriteriaFailureMessage);
            getBlazeHub()->getScheduler()->removeJob(jobId);
            return;
        }

        // Join by user list immediately triggers title cb here. Caller was pre-joined, and there's no additional game setup to await.
        joinGameJob->setGameId(joinGameJob->getUserIndex(), response->getGameId());
        joinGameJob->setReservedExternalPlayerIdentifications(response->getJoinedReservedPlayerIdentifications());
        joinGameJob->execute();
        getBlazeHub()->getScheduler()->removeJob(joinGameJob);
    }

    /*! ************************************************************************************************/
    /*! \brief async notification that a local player has joined a game, and needs to start joining the
            game's network mesh.

        This can be triggered from a number of paths: explicit create/join/reset game or implicitly
            due to a matchmaking session or following a game group.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameSetup(const NotifyGameSetup *notifyGameSetup, uint32_t userIndex)
    {
        GameId gameId = notifyGameSetup->getGameData().getGameId();
        BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Local player(%" PRId64 ":%s) has setup game(%" PRIu64 ") with setup reason index(%u)\n", 
            userIndex, (mUserManager->getLocalUser(userIndex) != nullptr ? mUserManager->getLocalUser(userIndex)->getId() : INVALID_BLAZE_ID),
            (mUserManager->getLocalUser(userIndex) != nullptr ? mUserManager->getLocalUser(userIndex)->getName() : "unknown"), gameId,
            notifyGameSetup->getGameSetupReason().getActiveMemberIndex());

        //  retrieve job based on this object
        GameManagerApiJob *gameManagerApiJob = getJob(userIndex, gameId);
        if (gameManagerApiJob == nullptr)
        {
            // Job could be null for a number of reasons
            //  1) Matchmaking dispatches title callback after internal response by design.  Only need to ensure we
            //     still have a local session. If we don't the prc timed out.
            //  2) RPC timeout - we immediately dispatch title cb after a RPC timeout, as we don't actually know if the
            //     server received our message.
            //  3) The job was canceled.  This removes the job through the exposed call in the job scheduler.
            //  4) Server initiated join.  A game group member is not notified until after they are pulled into the game.
            //  5) Host injection triggered by the blaze server
            //  6) Join initiated by another user calling the join game by user list method.

            MatchmakingScenario *matchmakingResults = nullptr;
            if (notifyGameSetup->getGameSetupReason().getMatchmakingSetupContext())
            {
                MatchmakingScenarioId scenarioId = notifyGameSetup->getGameSetupReason().getMatchmakingSetupContext()->getScenarioId();
                matchmakingResults = getMatchmakingScenarioById( scenarioId );
            }

            bool ownedMatchmaking = (matchmakingResults != nullptr);
            bool hostInjected = ((notifyGameSetup->getGameSetupReason().getDatalessSetupContext() != nullptr) &&
                (notifyGameSetup->getGameSetupReason().getDatalessSetupContext()->getSetupContext() == HOST_INJECTION_SETUP_CONTEXT)) ;
 
            // These joins types by design setup a new context to be dispatched after the join process has completed.
            if ( (notifyGameSetup->getGameSetupReason().getIndirectJoinGameSetupContext() != nullptr) || 
                 (notifyGameSetup->getGameSetupReason().getIndirectMatchmakingSetupContext() != nullptr) ||
                  ownedMatchmaking ||
                  hostInjected
               )
            {
                // Add a new job for the rest of our join process, until we dispatch based off our setup context from the notification.
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] NotifyGameSetup found no associated job for game(%" PRIu64 "), creating dummy context.\n", 
                    userIndex, notifyGameSetup->getGameData().getGameId());
                GameSetupJob *gameSetupJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "GameSetupJob") GameSetupJob(this, notifyGameSetup->getGameSetupReason());
                getBlazeHub()->getScheduler()->scheduleJobNoTimeout("GameSetupJob", gameSetupJob, this);
                gameSetupJob->setGameId(userIndex, notifyGameSetup->getGameData().getGameId());
                gameManagerApiJob = gameSetupJob;
            }

            if (ownedMatchmaking)
            {
                // Store off the gameId onto the session so a cancel on the matchmaking session can lookup the game id
                // to remove the player from the game.
                matchmakingResults->setGameId(gameId);
            }
        }      
        else
        {
            // copy our game setup reason from the notification into our existing context (to keep track of a callback)
            gameManagerApiJob->setSetupReason(notifyGameSetup->getGameSetupReason());
        }

        // create game; With MLU, it is possible the game has already been allocated by another local player.
        Game *game = getGameById( notifyGameSetup->getGameData().getGameId() );
        if ( (game != nullptr) && (getBlazeHub()->getNumUsers() == 1) )
        {
            // If we get here we're not using MLU, but the game already existed.  This should never happen,
            // but can potentially happen if the client misses a notification from the server that the game has been destroyed.  
            // Destroy the game silently and use our new one from the notificaiton.
            game->destroyGame(SYS_CREATION_FAILED, Game::DestroyGameJobCb());
            game = nullptr;
        }
        if (game == nullptr)
        {
            game = createLocalGame(notifyGameSetup->getGameData(), notifyGameSetup->getGameRoster(), notifyGameSetup->getGameQueue(), 
                notifyGameSetup->getGameSetupReason(), notifyGameSetup->getQosTelemetryInterval(), notifyGameSetup->getQosSettings(), 
                notifyGameSetup->getPerformQosValidation(), notifyGameSetup->getIsLockableForPreferredJoins(), notifyGameSetup->getGameModeAttributeName());
        }

        // If we still have a null job, we were either canceled or the RPC timed out.
        // Our titleCb has already been dispatched so we just need to clean up our existence in this game.
        if (gameManagerApiJob == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] NotifyGameSetup with reason index(%u) for game(%" PRIu64 "), that has been canceled or timed out locally.\n",
                userIndex, notifyGameSetup->getGameSetupReason().getActiveMemberIndex(), gameId);

            if ( ((notifyGameSetup->getGameSetupReason().getDatalessSetupContext() != nullptr) && 
                  (notifyGameSetup->getGameSetupReason().getDatalessSetupContext()->getSetupContext() == CREATE_GAME_SETUP_CONTEXT)) ||
                 ((notifyGameSetup->getGameSetupReason().getMatchmakingSetupContext() != nullptr) &&
                  (notifyGameSetup->getGameSetupReason().getMatchmakingSetupContext()->getMatchmakingResult() == SUCCESS_CREATED_GAME))
               )
            {
                // Destroy it silently, we've already dispatched our titleCb
                game->destroyGame(HOST_LEAVING, Game::DestroyGameJobCb());
            }
            else
            {
                // Leave silently, we've already dispatched our titleCb
                game->leaveGame(Game::LeaveGameJobCb());
            }
            // Game object deleted when user has fully left/destroyed the game (async callbacks).
            return;
        }

        // If matchmaking session was canceled, we had no job after we dispatched the start matchmaking callback.(since there was no game yet)
        // Check the setup context here from our notification, to see if we attempted to cancel this matchmaking session. 
        if (gameManagerApiJob->isMatchmakingScenarioCanceled())
        {
            // The MM Session was canceled, so we override the MM session's result & dispatch
            // (in case the cancel RPC crossed the finished notification on the wire).
            // NOTE: we must get the MMSession from the context obj since a PG MM session will send down
            // a prejoin notification instead of a joinGame notification.
            MatchmakingScenarioId scenarioId = gameManagerApiJob->getSetupReason().getMatchmakingSetupContext()->getScenarioId();
            MatchmakingScenario *matchmakingResults = getMatchmakingScenarioById( scenarioId );

            if (matchmakingResults != nullptr)
            {
                matchmakingResults->setMatchmakingResult(SESSION_CANCELED);
                dispatchNotifyMatchmakingScenarioFinished(*matchmakingResults, nullptr); // destroys the MM session
                
                gameManagerApiJob->cancel(ERR_OK);          // this should clean up any other resources (like if in a game for example)
                getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                return;
            }
            else
            {
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] ERR NotifyGameSetup game %" PRIu64 ", matchmaking scenario %" PRIu64 " has been destroyed.\n",
                    userIndex, notifyGameSetup->getGameData().getGameId(), scenarioId);
                BlazeAssertMsg(false,  "[GMGR] ERR NotifyGameSetup - Matchmaking Session has been destroyed.");
            }
        }

        // check if the join failed in the notification
        //   this is only possible if the blazeServer is using dynamic dedicated servers,
        //   ex: when the dynamic dedicated server creation takes too long and times out.
        if (gameManagerApiJob->getSetupReason().getResetDedicatedServerSetupContext() != nullptr)
        {
            BlazeError resetError = (Blaze::BlazeError)(gameManagerApiJob->getSetupReason().getResetDedicatedServerSetupContext()->getJoinErr());
            if (resetError != ERR_OK)
            {
                // there was a problem creating a dynamically allocated dedicated server.
                gameManagerApiJob->cancel(resetError);
                getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                destroyLocalGame(game, SYS_CREATION_FAILED, false/*wasHost*/, false/*wasActive*/);
                return;
            }
        }

        // Validate and cancel if protocol version mismatches for pg non-owner members.
        if (!validateProtocolVersion(*game, *gameManagerApiJob))
        {
            // GM_AUDIT: Here PG's mis-matchers kick out after join (matching members of PG may get in).
            // We may later add GameProtocolVersionString to UED so all PG members can be checked up front; revisit if we want these extra checks here then.
            // Currently only rpc-callers' GameProtocolVersionString is accessible to check up front at originating rpc. (GOS-7893).
            return;
        }

        // Check if the local user needs to do QoS
        if ( (game->getLocalPlayer(userIndex) != nullptr) )
        {
            ((GameEndpoint*)game->getLocalMeshEndpoint())->setPerformQosDuringConnection(notifyGameSetup->getPerformQosValidation());
        }

        // don't wait for the connection validated notification if we don't actually perform connection validation.
        if (!notifyGameSetup->getPerformQosValidation() )
        {
            gameManagerApiJob->setConnectionValidated();
        }

        // Local player can be null when we are a dedicated server user, but we still need to init our network.
        if ( (game->getLocalPlayer(userIndex) != nullptr) && !(game->getLocalPlayer(userIndex)->isActive()) )
        {
            // we've joined, but we're not an active player (we're queued/reserved)
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] GameManagerAPI::onNotifyGameSetup(): Joining game(%" PRIu64 ":%s)'s as inactive.\n", 
                userIndex, game->getId(),game->getName());
            if (gameManagerApiJob->isMatchmakingConnectionValidated())
            {
                gameManagerApiJob->execute();
                getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
            }
        }
        else
        {
            if (game->isExternalOwner())
            {
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] [GameManagerAPI].onNotifyGameSetup: Adding game(%" PRIu64 ":%s), local user is external owner.\n",
                    userIndex, game->getId(), game->getName());
                gameManagerApiJob->execute();
                getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                return;
            }

            if ((notifyGameSetup->getGameSetupReason().getDatalessSetupContext() != nullptr) && 
                (notifyGameSetup->getGameSetupReason().getDatalessSetupContext()->getSetupContext() == HOST_INJECTION_SETUP_CONTEXT))
            {
                // let the dedicated server get the game pointer to register listeners right away.
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] GameManagerAPI::onNotifyGameSetup(): Notifying host injection has begun for game(%" PRIu64 ":%s).\n",
                    userIndex, game->getId(),game->getName());
                mDispatcher.dispatch("onTopologyHostInjectionStarted", &GameManagerAPIListener::onTopologyHostInjectionStarted, game);
            }

            if ((game->getTopologyHostId() == INVALID_BLAZE_ID) && !game->isHostless())
            {
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] GameManagerAPI::onNotifyGameSetup(): skipping preInitGameNetwork because game doesn't yet have a host.\n", userIndex);
                        return;
            }

            // init the game network, we're active (not queued)
            if (!game->isNetworkCreated() && !game->isSettingUpNetwork())
            {
                preInitGameNetwork(game, userIndex);
                // dispatches createGameNetworkCb on completion
                return;
            }
            else
            {
                // Network has already been created or is in the process of being created by another local user, connect to the network.
                bool existingConnection = game->initiatePlayerConnections(game->getLocalPlayer(userIndex), userIndex);
                    
                // if we just updated an existing connection, finish out the job
                if (existingConnection)
                {
                    gameManagerApiJob->execute();
                    getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief callback for either NetworkMeshAdapter::hostGame or NetworkMeshAdapter::joinGame
        Dispatched by adapter after the game network has been initialized

        Note: game network init can happen for a number of reasons:
        gameCreation, gameReset, gameJoin, or as a result of matchmaking (which can implicitly create or join a game)
    ***************************************************************************************************/
    void GameManagerAPI::createdGameNetwork(BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError gameNetworkError, GameId gameId, uint32_t userIndex)
    {
        // NOTE: this is common code for ALL paths that create a local game object:
        //   CreateGame, ResetDedicatedServer, JoinGame, StartMatchmaking.
        //   We may be a player in the game, or we could be a dedicated server host.

        // ALSO, the adapter caches off a functor to this callback and CAN/WILL dispatch it multiple times.

        BLAZE_SDK_DEBUGF("[GMGR] Network for game(%" PRIu64 ") created. ERR(%u)\n", gameId, gameNetworkError);

        Game *game = getGameById(gameId);
        if (game == nullptr)
        {
            BlazeAssert (false && "got createGameNetworkCb callback for invalid gameId!\n");
            return;
        }
        BlazeAssertMsg(!game->isMigrating(), "createGameNetworkCb but the context seems to be host migration.\n");

        GameManagerApiJob *gameManagerApiJob = getJob(userIndex, gameId);
        if (gameManagerApiJob == nullptr)
        {
            BlazeAssertMsg(false,  "GameManagerAPI::createGameNetworkCb called for game with nullptr gameManagerApiJob.\n");
            return;
        }

        
        // check if job (or MM Session) existed and has been canceled
        if (gameManagerApiJob->isMatchmakingScenarioCanceled())
        {
            BLAZE_SDK_DEBUGF("[GMGR] Leaving game(%" PRIu64 ") due to cancel job or MM session.\n", gameId);
            gameManagerApiJob->cancel(ERR_OK);          // this should clean up any other resources (like if in a game for example)
            getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
            return;
        }

        //  at this point, the job exists 

        // check if the network init'd correctly
        if (gameNetworkError != BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK)
        {
            //  cancel job
            gameManagerApiJob->cancel(ERR_SYSTEM);
            getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
            return;
        }

        game->mIsLocalPlayerInitNetwork = false;

        // success, network init'd, any players in the mesh when created have already been connected to.

        if (game->getGameState() == IN_GAME)
        {
            // start the game if we join while the game's in progress...
            getNetworkAdapter()->startGame(game);
        }

        // We need to call up to the blaze server and send ourselves as the player connecting to himself:
        if (!game->isDedicatedServerHost())
        {
            Player* joiningPlayer = game->getLocalPlayer(userIndex);
            if ( EA_LIKELY(joiningPlayer != nullptr) )
            {
                sendMeshEndpointsConnected(game->getId(), joiningPlayer->getConnectionGroupId(), PlayerNetConnectionFlags(), 0, 0);
            }
        }

        // if we are the toplogy host and we created the game, we finalize the game creation.
        // Resetting should never finalize the game.
        if (game->isTopologyHost() && gameManagerApiJob->getSetupReason().getResetDedicatedServerSetupContext() == nullptr )
        {
            // call finalizeGameCreation, execution continues at GameManagerAPI::internalFinalizeGameCreationCb
            sendFinalizeGameCreation(gameId, userIndex);
        }
        else
        {
            // we're not the topology host
            if (gameManagerApiJob->isMatchmakingConnectionValidated())
            {
#if defined(BLAZE_EXTERNAL_SESSIONS_SUPPORT)
                // for a host-less game's creator, determine the primary game here (not via finalize game creation)
                if (gameManagerApiJob->isCreatingHostlessGame())
                {
                    updateGamePresenceForLocalUser(userIndex, *game, UPDATE_PRESENCE_REASON_GAME_CREATE, gameManagerApiJob);
                    return;
                }
#endif
                gameManagerApiJob->execute();
                getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
            }
            else
            {
                gameManagerApiJob->setWaitingForConnectionValidation();
            }
        }

        // need to attempt to connect to deferred players now that network is set up
        // AUDIT: we should check to see if this needs to be called for dedicated servers.
        game->connectToDeferredJoiningPlayers();
    }

    /*! ************************************************************************************************/
    /*! \brief callback when the game's topology host "finalizes" the game creation after the peer
            network is created.
    ***************************************************************************************************/
    void GameManagerAPI::internalFinalizeGameCreationCb( BlazeError error, JobId rpcJobId, GameId gameId, uint32_t userIndex)
    {
        GameManagerApiJob *gameManagerApiJob = getJob(userIndex, gameId);
        if (gameManagerApiJob == nullptr)
            return;

        if (error != ERR_OK)
        {
            gameManagerApiJob->cancel(ERR_SYSTEM);
        }
        else
        {
#if defined(BLAZE_EXTERNAL_SESSIONS_SUPPORT)
            // determine the primary game upon completing the join.
            Game* game = getGameById(gameId);
            if (EA_LIKELY(game != nullptr))
            {
                if ( !gameManagerApiJob->isMatchmakingConnectionValidated() )
                    gameManagerApiJob->setWaitingForConnectionValidation();

                updateGamePresenceForLocalUser(userIndex, *game, UPDATE_PRESENCE_REASON_GAME_CREATE, gameManagerApiJob);
                // Side: we now wait for primary session updates before dispatching create game title cb
                return;
            }
#endif

            // if we're waiting for connection validation, don't yet execute the job
            if ( !gameManagerApiJob->isMatchmakingConnectionValidated() )
            {
                // waiting should have been set already before the game was finalized
                gameManagerApiJob->setWaitingForConnectionValidation();
                return;
            }

            gameManagerApiJob->execute();
        }

        getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief callback when the game state is advanced to a new state.
    ***************************************************************************************************/
    void GameManagerAPI::internalAdvanceGameStateCb(Blaze::BlazeError blazeError, Game *game)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPI] Advance game state finished for game(%" PRIu64 ") with error %s\n", 
            game->getId(), getBlazeHub()->getErrorName(blazeError));
    }

    /*! ************************************************************************************************/
    /*! \brief Internal callback for completion of host injection notice to the blaze server
    ***************************************************************************************************/
    void GameManagerAPI::internalEjectHostCb(BlazeError error, JobId rpcJobId, GameId gameId)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPI] Host injection finished for game(%" PRIu64 ")\n", gameId);
    }

    /*! ************************************************************************************************/
    /*! \brief dispatch the "correct" gameSetup/fail callback or listener notification (by accessing the game's gameSetupContext.
            NOTE: clears the gameSetupContext after dispatch (to avoid double dispatches)
    ***************************************************************************************************/
    void GameManagerAPI::dispatchGameSetupCallback(Game &game, BlazeError blazeError, const GameSetupReason& reason, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GMGR][UI:%u] dispatching game(%" PRIu64 ") setup call back reason %u and error %s\n", userIndex, game.getId(), reason.getActiveMemberIndex(), getBlazeHub()->getErrorName(blazeError));
        if (reason.getIndirectJoinGameSetupContext() != nullptr)
        {        
            if ((blazeError == ERR_OK) && (game.getLocalPlayer(userIndex) != nullptr))
            {
                mDispatcher.dispatch("onUserGroupJoinGame", &GameManagerAPIListener::onUserGroupJoinGame, &game, game.getLocalPlayer(userIndex), reason.getIndirectJoinGameSetupContext()->getUserGroupId());
            }
            else
            {
                mDispatcher.dispatch("onIndirectJoinFailed", &GameManagerAPIListener::onIndirectJoinFailed, &game, userIndex, blazeError);
            }
        }
        else if (reason.getIndirectMatchmakingSetupContext() != nullptr)
        {
            if ((blazeError == ERR_OK) && (game.getLocalPlayer(userIndex) != nullptr))
            {
                mDispatcher.dispatch("onUserGroupJoinGame", &GameManagerAPIListener::onUserGroupJoinGame, &game, game.getLocalPlayer(userIndex), reason.getIndirectMatchmakingSetupContext()->getUserGroupId());
            }
            else
            {
                mDispatcher.dispatch("onIndirectJoinFailed", &GameManagerAPIListener::onIndirectJoinFailed, &game, userIndex, blazeError);
            }
        }
        else if (reason.getMatchmakingSetupContext() != nullptr)
        {
            MatchmakingScenario* matchmakingResults = getMatchmakingScenarioById(reason.getMatchmakingSetupContext()->getScenarioId());
            if (matchmakingResults == nullptr)
            {
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] dispatchGameSetupCallback: Skip dispatching NotifyMatchmakingFinished as matchmaking session(%" PRIu64 ") is cancelled. game(%" PRIu64 ").\n",
                    userIndex, reason.getMatchmakingSetupContext()->getSessionId(), game.getId());
            }
            else
            {
                if (blazeError != ERR_OK)
                {
                    if (matchmakingResults->getGameId() == INVALID_GAME_ID)
                    {
                        BLAZE_SDK_DEBUGF("[GMGR] UI[%u] dispatchGameSetupCallback: Skip dispatching NotifyMatchmakingFinished as matchmaking session(%" PRIu64 ") is continuing after failed QoS validation of game(%" PRIu64 ").\n",
                            userIndex, reason.getMatchmakingSetupContext()->getSessionId(), game.getId());
                    }
                    else // this matchmaking session is still tied to the game
                    {
                        // game setup failed, nuke the local MM Session
                        if (matchmakingResults->isCanceled())
                        {
                            // if client has canceled the local session
                            matchmakingResults->setMatchmakingResult(SESSION_CANCELED);
                        }
                        else
                        {                        
                            matchmakingResults->setMatchmakingResult(SESSION_ERROR_GAME_SETUP_FAILED);
                        }
                        // GM AUDIT: shouldn't we clean up the game here?
                        dispatchNotifyMatchmakingScenarioFinished(*matchmakingResults, &game); // destroys the MM Session
                    }
                }
                else
                {
                    // game setup success, we've already validated the connection if we're here
                    matchmakingResults->finishMatchmaking(reason.getMatchmakingSetupContext()->getFitScore(),
                        reason.getMatchmakingSetupContext()->getMaxPossibleFitScore(),
                        reason.getMatchmakingSetupContext()->getMatchmakingResult(),
                        reason.getMatchmakingSetupContext()->getTimeToMatch());
                    dispatchNotifyMatchmakingScenarioFinished(*matchmakingResults, &game);
                }
            }
        }
        else if (reason.getDatalessSetupContext() != nullptr && reason.getDatalessSetupContext()->getSetupContext() == INDIRECT_JOIN_GAME_FROM_QUEUE_SETUP_CONTEXT)
        {
            // Note, we only dispatch from here if we are the local player joining the game network.
            // External players are dispatched directly when notified of the state change.
            if ((blazeError == ERR_OK) && (game.getLocalPlayer(userIndex) != nullptr))
            {
                game.mDispatcher.dispatch("onPlayerJoinedFromQueue", &GameListener::onPlayerJoinedFromQueue, game.getLocalPlayer(userIndex));
            }
            else
            {
                mDispatcher.dispatch("onIndirectJoinFailed", &GameManagerAPIListener::onIndirectJoinFailed, &game, userIndex, blazeError);
            }
        }
        else if (reason.getDatalessSetupContext() != nullptr && reason.getDatalessSetupContext()->getSetupContext() == INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT)
        {
            // Note, we only dispatch from here if we are the local player joining the game network.
            // External players are dispatched directly when notified of the state change.
            if ((blazeError == ERR_OK) && (game.getLocalPlayer(userIndex) != nullptr))
            {
                // If this wasn't the Primary user, then we've already handled this callback when Game::claimPlayerReservation was called. 
                if (game.isNotificationHandlingUserIndex(userIndex))
                {
                    game.mDispatcher.dispatch("onPlayerJoinedFromReservation", &GameListener::onPlayerJoinedFromReservation, game.getLocalPlayer(userIndex));
                }
            }
            else
            {
                mDispatcher.dispatch("onIndirectJoinFailed", &GameManagerAPIListener::onIndirectJoinFailed, &game, userIndex, blazeError);
            }
        }
        else if (reason.getDatalessSetupContext() != nullptr && reason.getDatalessSetupContext()->getSetupContext() == HOST_INJECTION_SETUP_CONTEXT)
        {
            // Ensure that we aren't canceled due to the game being destroyed before dispatching our injected
            // notification to the client.
            if (blazeError == ERR_OK)
            {
                mDispatcher.dispatch("onTopologyHostInjected", &GameManagerAPIListener::onTopologyHostInjected, &game);
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Unhandled gameSetupReason (%u) in dispatchGameSetupCallback\n", userIndex, reason.getActiveMemberIndex());
            BlazeAssertMsg(false,  "Error: unhandled gameSetupReason in GameManagerAPI::dispatchGameSetupCallback");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief dispatch the listener notification
    ***************************************************************************************************/
    void GameManagerAPI::dispatchOnReservedPlayerIdentifications(Blaze::GameManager::Game *game, const UserIdentificationList &externalIdents, BlazeError blazeError, uint32_t userIndex)
    {
        //side: onReservedPlayerIdentifications returns the game id instead of a local game object, in case your
        //external players got their reservations in the game, but you don't have a local game object created/setup here.
        //This could be due to game setup error and/or because you aren't joined into the game.
        if (blazeError != ERR_OK)
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Warning: player failed joining game, with error %s but may still have added reservations for (%" PRIsize ") players.\n",
                userIndex, getBlazeHub()->getErrorName(blazeError), (size_t)externalIdents.size());
        }
        mDispatcher.dispatch("onReservedPlayerIdentifications", &GameManagerAPIListener::onReservedPlayerIdentifications, game, userIndex, &externalIdents);
    }

    // Helper function to standardize the message output, and make it easier to add a breakpoint for all cases:
    void GameManagerAPI::notifyDropWarn(const char8_t* notifyName, uint32_t userIndex, const Game* game, GameId gameId) const
    {
        BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Warning: dropping async %s for %s game(%" PRIu64 ")\n", userIndex, notifyName, 
                                        (game == nullptr ? "unknown" : "non-primary local user in"), gameId);
    }

    /*! ************************************************************************************************/
    /*! \brief async notification that an external player is joining a game's roster you know about.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerJoining(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex)
    {
        Game *game = getGameById( notifyPlayerJoining->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            // A reserved player in the roster is treated as inactive.
            if (notifyPlayerJoining->getJoiningPlayer().getPlayerState() == RESERVED)
            {
                game->onNotifyInactivePlayerJoining(notifyPlayerJoining->getJoiningPlayer(), userIndex);
            }
            else
            {
                // create player(s) & direct NetworkMeshAdapter to initiate player connection(s)
                // network adapter will connect & update blaze with conn status...
                // Blaze eventually either sends async NotifyPlayerJoinCompleted (onNotifyPlayerJoinComplete),
                // Or NotifyPlayerRemoved (for timeout)  
                game->onNotifyPlayerJoining(&notifyPlayerJoining->getJoiningPlayer(), userIndex, notifyPlayerJoining->getValidateQosForJoiningPlayer());
            }
        }
        else
        {
            notifyDropWarn("NotifyPlayerJoining", userIndex, game, notifyPlayerJoining->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief async notification that another (external) player is joining a game's queue you know about

        \param[in] notifyPlayerJoining
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerJoiningQueue(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex)
    {
        Game *game = getGameById( notifyPlayerJoining->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            // Note: player could be RESERVED or just in the queue, but they are still treated as inactive.
            game->onNotifyInactivePlayerJoiningQueue(notifyPlayerJoining->getJoiningPlayer(), userIndex);
        }
        else
        {
            notifyDropWarn("NotifyPlayerJoiningQueue", userIndex, game, notifyPlayerJoining->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief Handles async notification that a player is claiming a reservation.
    
        \param[in] notifyPlayerJoining
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerClaimingReservation(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex)
    {
        Game *game = getGameById( notifyPlayerJoining->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyPlayerClaimingReservation(notifyPlayerJoining->getJoiningPlayer(), userIndex, notifyPlayerJoining->getValidateQosForJoiningPlayer());
        }
        else
        {
            notifyDropWarn("NotifyPlayerClaimingReservation", userIndex, game, notifyPlayerJoining->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief Handles async notification that an external player is being promoted from the queue
    
        \param[in] notifyPlayerJoining
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerPromotedFromQueue(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex)
    {
        Game *game = getGameById( notifyPlayerJoining->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyPlayerPromotedFromQueue(notifyPlayerJoining->getJoiningPlayer(), userIndex );
        }
        else
        {
            notifyDropWarn("NotifyPlayerPromotedFromQueue", userIndex, game, notifyPlayerJoining->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief Handles async notification that an external player is being promoted from the queue
    
        \param[in] notifyPlayerJoining
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerDemotedToQueue(const NotifyPlayerJoining *notifyPlayerJoining, uint32_t userIndex)
    {
        Game *game = getGameById( notifyPlayerJoining->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyPlayerDemotedToQueue(notifyPlayerJoining->getJoiningPlayer(), userIndex );
        }
        else
        {
            notifyDropWarn("NotifyPlayerDemotedToQueue", userIndex, game, notifyPlayerJoining->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/


    /*! ************************************************************************************************/
    /*! \brief async notification that the local player needs to initiate player connections.
        Notification should only be received by the player who actually needs to initiate connections.
    
        \param[in] notifyPlayerJoining joining player notification information.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyJoiningPlayerInitiateConnections(const NotifyGameSetup *notifyGameSetup, uint32_t userIndex)
    {
        Game *game = getGameById( notifyGameSetup->getGameData().getGameId() );
        if (game == nullptr)
        {
            // Local player doesn't have an instance of the game.  Create one here.
            // create game;
            game = createLocalGame(notifyGameSetup->getGameData(), notifyGameSetup->getGameRoster(), notifyGameSetup->getGameQueue(), 
                notifyGameSetup->getGameSetupReason(), notifyGameSetup->getQosTelemetryInterval(), notifyGameSetup->getQosSettings(), 
                notifyGameSetup->getPerformQosValidation(), notifyGameSetup->getIsLockableForPreferredJoins(), notifyGameSetup->getGameModeAttributeName());
        }
        else if((notifyGameSetup->getGameSetupReason().getDatalessSetupContext() != nullptr) && 
            (notifyGameSetup->getGameSetupReason().getDatalessSetupContext()->getSetupContext() == HOST_INJECTION_SETUP_CONTEXT))
        {
            // a host injection has occurred after joining the game, need to update the topology host information
            game->completeRemoteHostInjection(notifyGameSetup);
        }

        BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Local player(%" PRId64 ":%s)  initiating connections to game(%" PRIu64 ") with setup reason index(%u)\n", 
            userIndex, (mUserManager->getLocalUser(userIndex) != nullptr ? mUserManager->getLocalUser(userIndex)->getId() : INVALID_BLAZE_ID),
            (mUserManager->getLocalUser(userIndex) != nullptr ? mUserManager->getLocalUser(userIndex)->getName() : "unknown"), game->getId(),
            notifyGameSetup->getGameSetupReason().getActiveMemberIndex());

        // The local player already knows about the game, so they haven't logged out/in before
        // claiming the reservation.  We only need to update the local player.
        // If they player exists they are already in the game, queued, or we may have an error state
        Player *localPlayer = game->getLocalPlayer(userIndex);
        if (localPlayer == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] onNotifyJoiningPlayerInitiateConnections for unknown local player in game(%" PRIu64 ")!", userIndex, game->getId());
            return;
        }
        
        ((GameEndpoint*)game->getLocalMeshEndpoint())->setPerformQosDuringConnection(notifyGameSetup->getPerformQosValidation());

        uint16_t localQueueIndex = localPlayer->getQueueIndex();
        BLAZE_SDK_DEBUGF("[GMGR] UI[%u] onNotifyJoiningPlayerInitiateConnections: Updating local player(%" PRId64 ") in game(%" PRIu64 "), slot(%u), queue index(%u).\n", 
            userIndex, localPlayer->getId(), game->getId(), localPlayer->getSlotId(), localQueueIndex);

        // Note that the iterations below are looping through the notifications roster, not the local game objects roster.
        // So first check that the player is claiming a reservation while in the queue.
        if ( (localQueueIndex != INVALID_QUEUE_INDEX) && (notifyGameSetup->getGameSetupReason().getDatalessSetupContext() != nullptr) && 
            (notifyGameSetup->getGameSetupReason().getDatalessSetupContext()->getSetupContext() == INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT))
        {
            ReplicatedGamePlayerList::const_iterator iter = notifyGameSetup->getGameQueue().begin();
            ReplicatedGamePlayerList::const_iterator end = notifyGameSetup->getGameQueue().end();
            for (; iter != end; ++iter)
            {
                const ReplicatedGamePlayer& replicatedPlayer = **iter;
                if (replicatedPlayer.getPlayerId() == localPlayer->getId())
                {
                    // Update the player queue
                    game->onNotifyJoiningPlayerInitiateConnections(replicatedPlayer, notifyGameSetup->getGameSetupReason());
                }
            }
        }
        else
        {
            // If we're not claiming a reservation while in the queue, we fall here.
            // Which means we are either claiming a reservation in the roster, or we are being
            // promoted from the queue into the roster.
            ReplicatedGamePlayerList::const_iterator iter = notifyGameSetup->getGameRoster().begin();
            ReplicatedGamePlayerList::const_iterator end = notifyGameSetup->getGameRoster().end();
            for (; iter != end; ++iter)
            {
                const ReplicatedGamePlayer& replicatedPlayer = **iter;
                if (replicatedPlayer.getPlayerId() == localPlayer->getId())
                {
                    // Update the player roster
                    game->onNotifyJoiningPlayerInitiateConnections(replicatedPlayer, notifyGameSetup->getGameSetupReason());
                }
                else
                {
                    // Update non-local players (replicatedPlayer) now that the local player is going active
                    game->onNotifyLocalPlayerGoingActive(replicatedPlayer, notifyGameSetup->getPerformQosValidation());
                }
            }
        }

        // Notification has made the player non reserved, which means we want to attempt connections.
        if (localPlayer->getPlayerState() != RESERVED)
        {
            // Reserved players will have the join request context if they joined to claim the reservation, 
            // Players reserved through MM are async'ly told to claim their reservation.
            // Queued players are receiving this from an async notification however, 
            // so we need to provide fake a context.
            GameManagerApiJob *gameManagerApiJob = getJob(userIndex, game->getId());
            if (gameManagerApiJob == nullptr)
            {
                // Add a context for the listener dispatch for a player joining the game from an inactive state.
                // Note that we are forcing the setup context into the game post creation as the joingameCB was already
                // issued with the initial joining into the queue.
                //  create a new job for this game setup context.
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] NotifyJoiningPlayerInitiateConnections found no associated job for game(%" PRIu64 "), creating dummy context.\n", 
                    userIndex, notifyGameSetup->getGameData().getGameId());
                gameManagerApiJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "JoiningPlayersInitJob") GameSetupJob(this, notifyGameSetup->getGameSetupReason());
                getBlazeHub()->getScheduler()->scheduleJobNoTimeout("JoiningPlayersInitJob", gameManagerApiJob, this);
                gameManagerApiJob->setGameId(userIndex, game->getId());
            }
            else if (localPlayer->getPlayerState() == QUEUED)
            {
                // player has a job from initiating a claim reservation request, but still is in the queue, we need to dispatch the title CB
                // but we also aren't ready to set up a game network so we early out. We'll finish up when we get promoted from the queue.
                gameManagerApiJob->execute();
                getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                return;
            }

            if (!notifyGameSetup->getPerformQosValidation())
            {
                gameManagerApiJob->setConnectionValidated();
            }

            // If my player got reserved into the game before a host was injected, the local copy of game's host info may be missing here even after injection
            // occurred server side (as blaze server would only have sent the HOST_INJECTION_SETUP_CONTEXT notifications to active players). Add it if so.
            if (game->getGameSettings().getVirtualized() && (game->getTopologyHostId() == INVALID_BLAZE_ID) &&
                (notifyGameSetup->getGameData().getTopologyHostInfo().getPlayerId() != INVALID_BLAZE_ID))
            {
                game->storeHostConnectionInfoPostInjection(*notifyGameSetup);

                notifyGameSetup->getGameData().getAdminPlayerList().copyInto(game->mAdminList);
            }

            if ((game->getTopologyHostId() != INVALID_BLAZE_ID) || game->isHostless())
            {
                // With MLU, another local player could have already setup our network, or be in the middle of setting up the network.
                if (!game->isNetworkCreated() && !game->isSettingUpNetwork())
                {
                    // Start our network connections.
                    preInitGameNetwork(game, userIndex);
                }
                else
                {
                    bool existingConnection = game->initiatePlayerConnections(localPlayer, userIndex);

                    // if we just updated an existing connection, finish out the job
                    if (existingConnection)
                    {
                        gameManagerApiJob->execute();
                        getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                    }
                    else if (game->isSettingUpNetwork())
                    {
                        // This means someone else locally is setting up for us, and its not done.
                        gameManagerApiJob->execute();
                        getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
                    }
                }
            }
            else
            {
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] onNotifyJoiningPlayerInitiateConnections(): skipping preInitGameNetwork for player(%" PRId64 ") joining due to no game host.\n", 
                    userIndex, localPlayer->getId());

            }

        }

    }


    /*! ************************************************************************************************/
    /*! \brief async notification that a player has completely joined the game (fully connected to the game's
            peer network mesh).
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerJoinComplete(const NotifyPlayerJoinCompleted *notifyPlayerJoinCompleted, uint32_t userIndex)
    {
        Game *game = getGameById(notifyPlayerJoinCompleted->getGameId());
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyPlayerJoinComplete(notifyPlayerJoinCompleted, userIndex);
        }
        else
        {
            notifyDropWarn("NotifyPlayerJoinComplete", userIndex, game, notifyPlayerJoinCompleted->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief async notification that a player's state has changed in the game
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGamePlayerStateChanged(const NotifyGamePlayerStateChange *notification, uint32_t userIndex)
    {
        Game *game = getGameById(notification->getGameId());
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            // Tell the game about the notification
            game->onNotifyGamePlayerStateChanged(notification->getPlayerId(), notification->getPlayerState(), userIndex);
        }
        else
        {
            notifyDropWarn("NotifyGamePlayerStateChanged", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief async notification that a player's team or role has changed in the game
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGamePlayerTeamRoleSlotChange(const NotifyGamePlayerTeamRoleSlotChange *notification, uint32_t userIndex)
    {
        Game *game = getGameById(notification->getGameId());
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGamePlayerTeamRoleSlotChanged(notification->getPlayerId(), notification->getTeamIndex(), notification->getPlayerRole(), notification->getSlotType());
        }
        else
        {
            notifyDropWarn("NotifyGamePlayerTeamRoleSlotChange", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief async notification that a team in the game has had it's ID changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameTeamIdChanged(const NotifyGameTeamIdChange *notification, uint32_t userIndex)
    {
        Game *game = getGameById(notification->getGameId());
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameTeamIdChanged(notification->getTeamIndex(), notification->getNewTeamId());
        }
        else
        {
            notifyDropWarn("NotifyGameTeamIdChanged", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/
    /*! ************************************************************************************************/
    /*! \brief async notification that the game's player capacity has changed (game resized)
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameCapacityChanged(const NotifyGameCapacityChange *notification, uint32_t userIndex)
    {
        Game *game = getGameById(notification->getGameId());
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameCapacityChanged(notification->getSlotCapacities(), notification->getTeamRosters(), notification->getRoleInformation());
            if (notification->getLockedForPreferredJoins())
                mDispatcher.dispatch("onGameLockedForPreferredJoins", &GameManagerAPIListener::onGameLockedForPreferredJoins, game, INVALID_BLAZE_ID, INVALID_EXTERNAL_ID);
        }
        else
        {
            notifyDropWarn("NotifyGameCapacityChanged", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief async notification that a player has been removed from a game (voluntary leave, kicked, kicked&banned, etc)
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerRemoved(const NotifyPlayerRemoved *notification, uint32_t userIndex)
    {
        Game *game = getGameById( notification->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            ExternalId leavingPlayerExternalId = INVALID_EXTERNAL_ID;
            BlazeId leavingPlayerId = notification->getPlayerId();

            const Player* leavingPlayer = game->getPlayerById(leavingPlayerId);
            if (leavingPlayer != nullptr)
            {
                leavingPlayerExternalId = leavingPlayer->getUser()->getExternalId();
            }
            game->onPlayerRemoved( notification->getPlayerId(), notification->getPlayerRemovedReason(), notification->getPlayerRemovedTitleContext(), userIndex );
        
            // if local game still exists after removing the player, dispatch the locked for preferred joins notification as needed.
            game = getGameById(notification->getGameId());
            if ((game != nullptr) && notification->getLockedForPreferredJoins() &&
                (notification->getPlayerRemovedReason() != GAME_DESTROYED) && (notification->getPlayerRemovedReason() != GAME_ENDED))
            {
                mDispatcher.dispatch("onGameLockedForPreferredJoins", &GameManagerAPIListener::onGameLockedForPreferredJoins, game, leavingPlayerId, leavingPlayerExternalId);
            }
            
        }
        else
        {
            notifyDropWarn("NotifyPlayerRemoved", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief async notification that the game's platform host has been init'd (part of platform host migration)
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlatformHostInitialized(const NotifyPlatformHostInitialized *notification, uint32_t userIndex)
    {
        Game *game = getGameById( notification->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->initPlatformHostSlotId(notification->getPlatformHostSlotId(), notification->getPlatformHostId());
        }
        else
        {
            notifyDropWarn("NotifyPlatformHostInitialized", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief async notification that host migration has started
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyHostMigrationStart(const NotifyHostMigrationStart *notification, uint32_t userIndex)
    {
        Game *game = getGameById( notification->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] GameManagerAPI::onNotifyHostMigrationStart() game(%" PRIu64 ") player(%" PRId64 ") migrationType(%s)\n",
                userIndex, notification->getGameId(), notification->getNewHostId(), HostMigrationTypeToString(notification->getMigrationType()));

            game->onNotifyHostMigrationStart(notification->getNewHostId(), notification->getNewHostSlotId(), notification->getNewHostConnectionSlotId(),
                notification->getMigrationType(), userIndex);
        }
        else
        {
            notifyDropWarn("NotifyHostMigrationStart", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/
 
    void GameManagerAPI::networkAdapterMigrateHost(BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error, GameId gameId, HostMigrationType migrationType)
    {

        Game *game = getGameById(gameId);
        if (game == nullptr)
        {
            return;
        }

        BLAZE_SDK_DEBUGF("[GMGR] Network for game(%" PRIu64 ") being migrated. Hosting(%s) ERR(%u)\n", gameId, 
            game->isPlatformHost() ? "true" : "false", error);

        if (!game->isMigrating())
        {
            return;
        }
        if( error == BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK )
        {
            UpdateGameHostMigrationStatusRequest updateGameHostMigrationStatusRequest;
            updateGameHostMigrationStatusRequest.setGameId( game->getId() );
            updateGameHostMigrationStatusRequest.setHostMigrationType( migrationType );
            mGameManagerComponent->updateGameHostMigrationStatus(updateGameHostMigrationStatusRequest);
        }

        // GM_AUDIT: no error handling, but connApiAdapter always returns ERR_OK...
    }


    /*! ************************************************************************************************/
    /*! \brief async notification that migration has finished
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyHostMigrationFinished(const NotifyHostMigrationFinished *notification, uint32_t userIndex)
    {
        Game *game = getGameById( notification->getGameId() );        
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyHostMigrationFinish();
        }
        else
        {
            notifyDropWarn("NotifyHostMigrationFinished", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief async notification that the game's reporting id has changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameReportingIdChanged(const NotifyGameReportingIdChange *notification, uint32_t userIndex)
    {
        Game *game = getGameById( notification->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
#if defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
            updateGamePresenceForLocalUser(userIndex, *game, UPDATE_PRESENCE_REASON_GAME_NEWROUND, nullptr);
#endif

            game->setGameReportingId(notification->getGameReportingId());
        }
        else
        {
            notifyDropWarn("NotifyGameReportingIdChanged", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ****************************************************************************/
    /*! \brief Return one of the localUser's games by gameId
    ********************************************************************************/
    Game *GameManagerAPI::getGameById(GameId gameId) const
    {
        GameMap::const_iterator gameMapIter = mGameMap.find( gameId );
        if( gameMapIter != mGameMap.end() )
        {
            return gameMapIter->second;
        }

        return nullptr;
    }

    /*! ****************************************************************************/
    /*! \brief Return one of the localUser's games (by index).
    ********************************************************************************/
    Game *GameManagerAPI::getGameByIndex(uint32_t index) const
    {
        if (index < mGameMap.size())
        {
            GameMap::const_iterator gameMapIter = mGameMap.begin();
            eastl::advance(gameMapIter, index);
            return gameMapIter->second;
        }

        BlazeAssertMsg(false,  "index out of bounds (must be < getGameCount())...\n");
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief async notification that the game's state has changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameStateChanged(const NotifyGameStateChange *notifyGameStateChanged, uint32_t userIndex)
    {
        GameId gameId = notifyGameStateChanged->getGameId();
        Game *game = getGameById( gameId );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameStateChanged(notifyGameStateChanged->getNewGameState(), userIndex);
        }
        else
        {
            notifyDropWarn("NotifyGameStateChanged", userIndex, game, gameId);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief Helper: remove the supplied game from the gameMap, dispatch GameManagerAPIListener::onGameDestructing,
            then teardown the game's network and delete the game.
        \param[in] game the game to delete
        \param[in] wasLocalPlayerHost flag to show if game's local player is a host
        \param[in] wasLocalPlayerActive flag to indicate if the local player was active (non-queued, active on the network.)
        \param[in] dispatchGameManagerApiJob flag to indicate if game's setup job dispatches are required. If false, just remove the job without dispatching. Set false if caller already handles the dispatch's ops.
    ***************************************************************************************************/
    void GameManagerAPI::destroyLocalGame(Game *game, const GameDestructionReason destructionReason, bool wasLocalPlayerHost, bool wasLocalPlayerActive, bool dispatchGameManagerApiJob /*= true*/)
    {
        BlazeAssert(game != nullptr);
        BlazeAssert(mGameMap.find(game->getId()) != mGameMap.end());

        BLAZE_SDK_DEBUGF("[GMGR] Destroying local game representation for game(%" PRIu64 ":%s)\n", game->getId(), game->getName());

        // remove the game from the map, dispatch that it's being destroyed, then delete it.
        mGameMap.erase(game->getId());

#if defined(BLAZE_EXTERNAL_SESSIONS_SUPPORT)
        // before deleting game object or its players, dispatch any primary game updates for users in the game.
        // Done before actual game or player objects are freed as updatePrimarySessionForUser may access them.
        uint32_t numLocalUsers = getBlazeHub()->getNumUsers();
        for (uint32_t ui = 0; ui < numLocalUsers; ++ui)
        {
            updateGamePresenceForLocalUser(ui, *game, UPDATE_PRESENCE_REASON_GAME_LEAVE, nullptr, GAME_DESTROYED);
        }
        // Before deleting game, ensure cleanup and listeners notified. Dispatch now. Don't wait for the primary game update as it may never arrive
        if (game->getId() == getActivePresenceGameId())
            onActivePresenceCleared(true, getBlazeHub()->getPrimaryLocalUserIndex());
#endif

        //we dispatch if there is a job.  also cleans up the job associated with this game if it already hasn't been removed.
        GameManagerApiJob *gameManagerApiJob = getJob(getBlazeHub()->getPrimaryLocalUserIndex(), game->getId());
        if (gameManagerApiJob != nullptr)
        {
            if (gameManagerApiJob->getSetupReason().getMatchmakingSetupContext() != nullptr)
            {
                BLAZE_SDK_DEBUGF("[GMGR] Game (%" PRIu64 ":%s) destroyed with matchmaking context.\n", game->getId(), game->getName());               
                if (!gameManagerApiJob->isMatchmakingConnectionValidated() && (!game->isNetworkCreated() || gameManagerApiJob->isWaitingForConnectionValidation()))
                {
                    MatchmakingScenario* matchmakingResults = getMatchmakingScenarioById(gameManagerApiJob->getSetupReason().getMatchmakingSetupContext()->getScenarioId());
                    if ((matchmakingResults != nullptr) && !matchmakingResults->isCanceled())
                    {
                        // if this MM session is still going- validation was in-progress and incomplete, remove the game id so the MM session doesn't die locally.
                        BLAZE_SDK_DEBUGF("[GMGR] Game (%" PRIu64 ":%s) destroyed with matchmaking context while awaiting connection validation, clearing cached GameId so matchmaking session (%" PRIu64 ") can continue.\n", 
                            game->getId(), game->getName(), gameManagerApiJob->getSetupReason().getMatchmakingSetupContext()->getSessionId()); 
                        matchmakingResults->setGameId(INVALID_GAME_ID);
                    }
                }
            }

            if (destructionReason == HOST_EJECTION)
            {
                if (dispatchGameManagerApiJob)
                {
                    gameManagerApiJob->dispatch(GAMEMANAGER_ERR_NO_HOSTS_AVAILABLE_FOR_INJECTION, game);
                }
            }
            else
            {
                if (!game->isNetworkCreated())
                {
                    // Having a job means we're in the middle of setting up a game.  Even if we don't yet have the network,
                    // we call destroy to cleanup anything on the network side that may dependent on the existence
                    // of the game object, which is deleted below.
                    BLAZE_SDK_DEBUGF("[GMGR] Game (%" PRIu64 ":%s) destroyed with reason(%s) before network connections were established.\n", game->getId(), game->getName(), GameDestructionReasonToString(destructionReason));
                    mNetworkAdapter->destroyNetworkMesh(game);
                }
                if (dispatchGameManagerApiJob)
                {
                    gameManagerApiJob->dispatch(ERR_SYSTEM, game);        // do not cancel the job as cancellation may issue a destroy request, which is unncessary in this case.
                }
            }
            getBlazeHub()->getScheduler()->removeJob(gameManagerApiJob);
        }

        // ensure network tear down and signaling leaving game groups, i.e. undo anything set up between pre-init network and network mesh created stages.
        if ( game->isSettingUpNetwork() || game->isNetworkCreated())
        {
            // tear down network connection(s)
            mNetworkAdapter->destroyNetworkMesh(game);
        }
       
        mDispatcher.dispatch("onGameDestructing", &GameManagerAPIListener::onGameDestructing, this, game, destructionReason);

        mGameMemoryPool.free(game);
    }

    /*! ************************************************************************************************/
    /*! \brief NetworkMeshAdapterListener impl
    ***************************************************************************************************/
    void GameManagerAPI::networkMeshDestroyed( const Blaze::Mesh *mesh, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        Game *game = (Game*)mesh;

        BLAZE_SDK_DEBUGF("[GMGR] Adapter destroyed the game(%" PRIu64 ":%s) network\n", game->getId(), game->getName());

        game->stopTelemetryReporting();

        PlayerNetConnectionFlags playerConnectionFlags;

        playerConnectionFlags.setBits(game->getLeavingPlayerConnFlags());
    
        PlayerNetConnectionStatus playerNetConnectionStatus = game->isLocalPlayerMapEmpty() ? DISCONNECTED : DISCONNECTED_PLAYER_REMOVED;

        sendMeshEndpointsDisconnected( game->getId(), game->getTopologyHostConnectionGroupId(), playerNetConnectionStatus, playerConnectionFlags);

    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief NetworkMeshAdapterListener impl - we've connected to endpoint
        \param[in] endpoint - the endpoint we've connected from
        \param[in] connectionFlags - connection-specific flags
        \param[in] error - error code from the adapter
    ***************************************************************************************************/
    void GameManagerAPI::connectedToEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        if (mesh == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Adapter sent null mesh to connectedToEndpoint!\n");
            return;
        }
        
        if (error != BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Skipping connectedToEndpoint handling due to error - (%d)\n", error);
            return;
        }

        //set game-connection specific flags
        PlayerNetConnectionFlags playerConnectionFlags;
        playerConnectionFlags.setBits(connectionFlags.to_uint32());

        ConnectionGroupId targetId;
        Blaze::BlazeNetworkAdapter::MeshEndpointQos qosData;
        // cast required to get the Qos stats
        const Game* gameMesh = (const Game*)mesh;

        if ((gameMesh->getNetworkTopology() == CLIENT_SERVER_DEDICATED) && !gameMesh->isTopologyHost())
        {
            targetId = gameMesh->getTopologyHostConnectionGroupId();
            gameMesh->getQosStatisticsForTopologyHost(qosData, true);
        }
        else // not a dedicater server game, or this client *is* the dedicated server
        {
            targetId = targetConnGroupId;
            const MeshEndpoint *endpoint = gameMesh->getMeshEndpointByConnectionGroupId(targetId);
            if (endpoint != nullptr)
            {
                gameMesh->getQosStatisticsForEndpoint(endpoint, qosData, true);
            }
        }


        BLAZE_SDK_DEBUGF("[GMGR] Adapter %s to endpoint(%" PRIu64 ") game(%" PRIu64 ":%s)\n", 
            PlayerNetConnectionStatusToString(Blaze::GameManager::CONNECTED), targetId, 
            mesh->getId(), mesh->getName());
        sendMeshEndpointsConnected( mesh->getId(), targetId, playerConnectionFlags, qosData.LatencyMs, qosData.getPacketLossPercent() );
    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief NetworkMeshAdapterListener impl - we've disconnected from a peer, we asked for a disconnection
        \param[in] endpoint - the peer we've disconnected from
        \param[in] connectionFlags - connection-specific flags
        \param[in] error - error code from the adapter
    ***************************************************************************************************/
    void GameManagerAPI::disconnectedFromEndpoint( const Blaze::MeshEndpoint *endpoint, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        //set game-connection specific flags
        PlayerNetConnectionFlags playerConnectionFlags;
        playerConnectionFlags.setBits(connectionFlags.to_uint32());

        Game* game = (Game*)endpoint->getMesh();

        BLAZE_SDK_DEBUGF("[GMGR] Adapter DISCONNECTED from endpoint(%" PRIu64 ") in game(%" PRIu64 ":%s)\n", 
            endpoint->getConnectionGroupId(), game->getId(), game->getName());


        // The adapter tells us about disconnects, even if there was not a physical connection.
        // p2p networks update mesh for every player to every player.
        // c/s networks only update mesh for connections to and from the topology host.
        if (game->isPeerToPeer() || (endpoint->getConnectionGroupId() == game->getTopologyHostConnectionGroupId() || game->isTopologyHost()))
        {
            sendMeshEndpointsDisconnected( game->getId(), endpoint->getConnectionGroupId(), DISCONNECTED_PLAYER_REMOVED, playerConnectionFlags );
        }
    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief NetworkMeshAdapterListener impl - we've disconnected from a peer, we've lost an established connection to the player
        \param[in] endpoint - the peer we've disconnected from
        \param[in] connectionFlags - connection-specific flags
        \param[in] error - error code from the adapter
    ***************************************************************************************************/
    void GameManagerAPI::connectionToEndpointLost( const Blaze::Mesh *mesh,  Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        if (mesh == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] GameManagerAPI::connectionToEndpointLost: Warn: Adapter peer disconnect detected, but no mesh info provided.\n");
            return;
        }
            //set game-connection specific flags
        PlayerNetConnectionFlags playerConnectionFlags;
        playerConnectionFlags.setBits(connectionFlags.to_uint32());

        Game *game = (Game*)mesh;

        BLAZE_SDK_DEBUGF("[GMGR] Adapter LOST CONNECTION from endpoint(%" PRId64 ") in game(%" PRIu64 ":%s)\n", 
            targetConnGroupId, game->getId(), game->getName());

        sendMeshEndpointsConnectionLost( game->getId(), targetConnGroupId, playerConnectionFlags); 

#ifdef USE_TELEMETRY_SDK
        Blaze::Telemetry::TelemetryAPI* telemetryApi = getBlazeHub()->getTelemetryAPI(getBlazeHub()->getPrimaryLocalUserIndex());
        if ((telemetryApi != nullptr) && telemetryApi->isDisconnectTelemetryEnabled())
        {
            TelemetryApiEvent3T event;
            //telemetry error codes are guaranteed to be negative
            int32_t retVal = TelemetryApiInitEvent3(&event,'BLAZ','GAME','DISC');
            retVal += TelemetryApiEncAttributeString(&event, 'gnam', mesh->getName());
            retVal += TelemetryApiEncAttributeLong(&event, 'gmid', mesh->getId());
            retVal += TelemetryApiEncAttributeLong(&event, 'epid', targetConnGroupId);
            retVal += TelemetryApiEncAttributeString(&event, 'gpvs', getGameProtocolVersionString());
            retVal += TelemetryApiEncAttributeString(&event, 'nmae', BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterErrorToString(error));

            if (retVal == TC_OKAY)
            {
                telemetryApi->queueBlazeSDKTelemetry(event);
            }
            else
            {
                BLAZE_SDK_DEBUGF("[GMGR].connectionToEndpointLost() - unable to submit BlazeSDK telemetry\n");
            }
        }
#endif
    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

     /*! ************************************************************************************************/
    /*! \brief NetworkMeshAdapterListener impl - we've connected to a voip endpoint
        \param[in] endpoint - the endpoint we've connected from
        \param[in] connectionFlags - connection-specific flags
        \param[in] error - error code from the adapter
    ***************************************************************************************************/
    void GameManagerAPI::connectedToVoipEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        if (mesh == nullptr)
        {
            // Nothing to be done.
            BLAZE_SDK_DEBUGF("[GMGR] Adapter sent null mesh to connectedToVoipEndpoint!\n");
            return;
        }
        
        Game* game = (Game*) mesh;
        BLAZE_SDK_DEBUGF("[GMGR] Adapter CONNECTED to VoIP endpoint(%" PRIu64 ") in game(%" PRIu64 ":%s)\n", 
            targetConnGroupId, game->getId(), game->getName());           

        // have the game dispatch the listener
        game->gameVoipConnected(targetConnGroupId);

        //Send the mesh update for voip notifications only if the game connection between these two end points is not required. If it is, we track the connectivity via that notification. 
        if (game->shouldSendMeshConnectionUpdateForVoip(targetConnGroupId))
        {
            PlayerNetConnectionFlags playerConnectionFlags;
            playerConnectionFlags.setBits(connectionFlags.to_uint32());

            Blaze::BlazeNetworkAdapter::MeshEndpointQos qosData;
            const MeshEndpoint *endpoint = game->getMeshEndpointByConnectionGroupId(targetConnGroupId);
            if (endpoint != nullptr)
            {
                game->getQosStatisticsForEndpoint(endpoint, qosData, true);
            }
            sendMeshEndpointsConnected( mesh->getId(), targetConnGroupId, playerConnectionFlags, qosData.LatencyMs, qosData.getPacketLossPercent() );
        }
    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief NetworkMeshAdapterListener impl - we've disconnected from a voip endpoint, we asked for a disconnection
        \param[in] endpoint - the peer we've disconnected from
        \param[in] connectionFlags - connection-specific flags
        \param[in] error - error code from the adapter
    ***************************************************************************************************/
    void GameManagerAPI::disconnectedFromVoipEndpoint( const Blaze::Mesh *mesh, Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        if (mesh == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] GameManagerAPI::connectionToVoipEndpointLost: Warn: Adapter peer disconnect detected, but no mesh info provided.\n");
            return;
        }
        
        Game* game = (Game*) mesh;
        BLAZE_SDK_DEBUGF("[GMGR] Adapter Disconnected from VoIP endpoint(%" PRIu64 ") in game(%" PRIu64 ":%s)\n", 
            targetConnGroupId, game->getId(), game->getName());

        if (game->shouldSendMeshConnectionUpdateForVoip(targetConnGroupId))
         {
            PlayerNetConnectionFlags playerConnectionFlags;
            playerConnectionFlags.setBits(connectionFlags.to_uint32());

            sendMeshEndpointsDisconnected( game->getId(), targetConnGroupId, DISCONNECTED_PLAYER_REMOVED, playerConnectionFlags );
        }
    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief NetworkMeshAdapterListener impl - we've disconnected from a voi[ peer, we've lost an established connection to the player
        \param[in] endpoint - the peer we've disconnected from
        \param[in] connectionFlags - connection-specific flags
        \param[in] error - error code from the adapter
    ***************************************************************************************************/
    void GameManagerAPI::connectionToVoipEndpointLost( const Blaze::Mesh *mesh,  Blaze::ConnectionGroupId targetConnGroupId, const ConnectionFlagsBitset connectionFlags, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        if (mesh == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] GameManagerAPI::connectionToVoipEndpointLost: Warn: Adapter peer disconnect detected, but no mesh info provided.\n");
            return;
        }

        Game* game = (Game*) mesh;
        BLAZE_SDK_DEBUGF("[GMGR] Adapter LOST CONNECTION to VoIP endpoint(%" PRIu64 ") in game(%" PRIu64 ":%s)\n", 
            targetConnGroupId, game->getId(), game->getName());

        // have the game dispatch the listener
        game->gameVoipLostConnection(targetConnGroupId);
        if (game->shouldSendMeshConnectionUpdateForVoip(targetConnGroupId))
        {
            PlayerNetConnectionFlags playerConnectionFlags;
            playerConnectionFlags.setBits(connectionFlags.to_uint32());

            sendMeshEndpointsConnectionLost( game->getId(), targetConnGroupId, playerConnectionFlags); 
        }
    } /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/


    /*! ************************************************************************************************/
    /*! \brief send an MeshEndpointsConnected RPC to the blazeServer (fire 'n forget)

        \param[in] gameId the game we're updating
        \param[in] targetBlazeId the blazeId (aka playerId) of the endpoint we've connected to (peer or host)
        \param[in] connectionFlags the connection-specific flags
        \param[in] latencyMs the latency on the connection in MS
        \param[in] packetLoss the percentage of packets lost on the connection
    ***************************************************************************************************/
    void GameManagerAPI::sendMeshEndpointsConnected(GameId gameId, ConnectionGroupId targetConnGroupId, PlayerNetConnectionFlags connectionFlags, uint32_t latencyMs /* = 0 */, float packetLoss /* = 0.0f */) const
    {
        MeshEndpointsConnectedRequest meshEndpointsConnectedRequest;
        meshEndpointsConnectedRequest.setGameId( gameId );

        EA::TDF::ObjectId targetObjectId(ENTITY_TYPE_CONN_GROUP,(Blaze::EntityId)targetConnGroupId);
        
        meshEndpointsConnectedRequest.setTargetGroupId( targetObjectId );
        meshEndpointsConnectedRequest.getPlayerNetConnectionFlags().setBits( connectionFlags.getBits() );

        meshEndpointsConnectedRequest.getQosInfo().setLatencyMs(latencyMs);
        meshEndpointsConnectedRequest.getQosInfo().setPacketLoss(packetLoss);

        mGameManagerComponent->meshEndpointsConnected( meshEndpointsConnectedRequest );
    }

    /*! ************************************************************************************************/
    /*! \brief send an MeshEndpointsDisconnected RPC to the blazeServer (fire 'n forget)

        \param[in] gameId the game we're updating
        \param[in] targetBlazeId the blazeId (aka playerId) of the endpoint we've connected to (peer or host)
        \param[in] connectionStatus the status of the connection.
        \param[in] latencyMs the latency on the connection in MS
        \param[in] packetLoss the percentage of packets lost on the connection
    ***************************************************************************************************/
    void GameManagerAPI::sendMeshEndpointsDisconnected(GameId gameId, ConnectionGroupId targetConnGroupId, PlayerNetConnectionStatus connectionStatus, PlayerNetConnectionFlags connectionFlags) const
    {
        MeshEndpointsDisconnectedRequest meshEndpointsDisconnectedRequest;
        meshEndpointsDisconnectedRequest.setGameId( gameId );

        EA::TDF::ObjectId targetObjectId(ENTITY_TYPE_CONN_GROUP,(Blaze::EntityId)targetConnGroupId);
        
        meshEndpointsDisconnectedRequest.setTargetGroupId( targetObjectId );
        meshEndpointsDisconnectedRequest.setPlayerNetConnectionStatus( connectionStatus );
        meshEndpointsDisconnectedRequest.getPlayerNetConnectionFlags().setBits( connectionFlags.getBits() );

        mGameManagerComponent->meshEndpointsDisconnected( meshEndpointsDisconnectedRequest );
    }

    /*! ************************************************************************************************/
    /*! \brief send an MeshEndpointsDisconnected RPC to the blazeServer (fire 'n forget)

        \param[in] gameId the game we're updating
        \param[in] targetBlazeId the blazeId (aka playerId) of the endpoint we've connected to (peer or host)
        \param[in] connectionStatus the status of the connection.
        \param[in] latencyMs the latency on the connection in MS
        \param[in] packetLoss the percentage of packets lost on the connection
    ***************************************************************************************************/
    void GameManagerAPI::sendMeshEndpointsConnectionLost(GameId gameId, ConnectionGroupId targetConnGroupId, PlayerNetConnectionFlags connectionFlags) const
    {
        MeshEndpointsConnectionLostRequest meshEndpointsConnectionLostRequest;
        meshEndpointsConnectionLostRequest.setGameId( gameId );

        EA::TDF::ObjectId targetObjectId(ENTITY_TYPE_CONN_GROUP,(Blaze::EntityId)targetConnGroupId);
        meshEndpointsConnectionLostRequest.setTargetGroupId( targetObjectId );
        meshEndpointsConnectionLostRequest.getPlayerNetConnectionFlags().setBits( connectionFlags.getBits() );

        mGameManagerComponent->meshEndpointsConnectionLost( meshEndpointsConnectionLostRequest );
    }

    void GameManagerAPI::migratedTopologyHost( const Blaze::Mesh *mesh, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        if (error == BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK)
        {
            networkAdapterMigrateHost(error, mesh->getId(), TOPOLOGY_HOST_MIGRATION);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GMGR] GNA: GameManagerAPI::migratedTopologyHost event reports an error: %d\n", error);
        }
    }

    void GameManagerAPI::migratedPlatformHost( const Blaze::Mesh *mesh, BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error )
    {
        if (error == BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK)
        {
            networkAdapterMigrateHost(error, mesh->getId(), PLATFORM_HOST_MIGRATION);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GMGR] GNA: GameManagerAPI::migratedPlatformHost event reports an error: %d\n", error);
        }
    }

    /* TO BE DEPRECATED - do not use - not MLU ready - receiving game data should be implemented against DirtySDk's NetGameLink api directly */
    void GameManagerAPI::receivedMessageFromEndpoint( const Blaze::MeshEndpoint *endpoint, const void *buf, size_t bufSize, BlazeNetworkAdapter::NetworkMeshAdapter::MessageFlagsBitset messageFlags, Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::NetworkMeshAdapterError error)
    {
        const Mesh *mesh = endpoint->getMesh();

        if(error != Blaze::BlazeNetworkAdapter::NetworkMeshAdapter::ERR_OK)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Adapter received message from endpoint with error '%d'", error);
            return;
        }

        // the game by id, dispatch message notification
        Game* game = (Game*)mesh; 
        BLAZE_SDK_DEBUGF("[GMGR] Adapter received message from from endpoint(%" PRId64 ") in game(%" PRIu64 ":%s)\n", endpoint->getConnectionGroupId(), game->getId(), game->getName());
        game->mDispatcher.dispatch(&GameListener::onMessageReceived, game, buf, bufSize, (BlazeId)endpoint->getConnectionGroupId(), messageFlags);

    }  /*lint !e1961 - Avoid lint to check whether virtual member function could be made const*/

    JobId GameManagerAPI::startMatchmakingScenario(const ScenarioName& scenarioName, 
        const ScenarioAttributes& scenarioAttributes,
        const PlayerJoinDataHelper& playerData, 
        const GameManagerAPI::StartMatchmakingScenarioCb &titleCb, 
        bool addLocalUsersToRequest)
    {
        // can't matchmake if the network adapter isn't initialized
        BlazeAssert(mNetworkAdapter->isInitialized());
        if(!mNetworkAdapter->isInitialized())
        {
            JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("startMatchmakingScenarioCb", titleCb, ERR_AUTHENTICATION_REQUIRED, jobId, (MatchmakingScenario*)nullptr, "",
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
            return jobId;
        }

        if (!getBlazeHub()->getConnectionManager()->initialQosPingSiteLatencyRetrieved())
        {
            JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("startMatchmakingScenarioCb", titleCb, SDK_ERR_QOS_PINGSITE_NOT_INITIALIZED, jobId, (MatchmakingScenario*)nullptr, "",
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
            return jobId;
        }

        StartMatchmakingScenarioRequest startScenarioRequest;
        prepareCommonGameData(startScenarioRequest.getCommonGameData(), GAME_TYPE_GAMESESSION);
        scenarioAttributes.copyInto(startScenarioRequest.getScenarioAttributes());
        startScenarioRequest.setScenarioName(scenarioName);

        playerData.copyInto(startScenarioRequest.getPlayerJoinData());

        // Add the main player:
        uint32_t userIndex = getBlazeHub()->getPrimaryLocalUserIndex();
        UserManager::LocalUser* localUser = getBlazeHub()->getUserManager()->getLocalUser(userIndex);
        if (localUser == nullptr)
        {
            JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("startMatchmakingScenarioCb", titleCb, GAMEMANAGER_ERR_PLAYER_NOT_FOUND, jobId, (MatchmakingScenario*)nullptr, "",
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
            return jobId;
        }

        if (addLocalUsersToRequest)
        {
            PlayerJoinDataHelper::addPlayerJoinData(startScenarioRequest.getPlayerJoinData(), localUser->getUser());    // Main player is always added to the scenario requests

            if (playerData.getGroupId() == EA::TDF::OBJECT_ID_INVALID)
            {
                // send up our connection group id.
                startScenarioRequest.getPlayerJoinData().setGroupId(localUser->getConnectionGroupObjectId());
            }
        }
        else if (playerData.getGroupId() == EA::TDF::OBJECT_ID_INVALID && playerData.getPlayerDataList().empty())
        {
            // Helper check to avoid sending up an empty request:
            JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("startMatchmakingScenarioCb", titleCb, GAMEMANAGER_ERR_USER_NOT_FOUND, jobId, (MatchmakingScenario*)nullptr, "No users were provided in player data for matchmaking request.  addLocalUsersToRequest was disabled.",
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
            return jobId;
        }

        BLAZE_SDK_DEBUGF("[GMGR] Starting matchmaking scenario name(%s)\n", scenarioName.c_str());

        // create/schedule a job to track the matchmaking session (so we can cancel before we have a MatchmakingSession object)
        MatchmakingScenarioJob *matchmakingScenarioJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "MatchmakingScenarioJob") MatchmakingScenarioJob(this, titleCb);
        matchmakingScenarioJob->setGameId(getBlazeHub()->getPrimaryLocalUserIndex(), INVALID_GAME_ID);
        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("MatchmakingScenarioJob", matchmakingScenarioJob, this);

        mGameManagerComponent->startMatchmakingScenario(startScenarioRequest, MakeFunctor(this, &GameManagerAPI::internalStartMatchmakingScenarioCb), jobId);

        return jobId;
    }

    /*! ************************************************************************************************/
    /*!\brief Internal callback for startMatchmakingSession.
        \param[in] rpcJobId jobId of the actual StartMatchmaking RPC (ignored)
        \param[in] jobId jobId of the MatchmakingSessionJob we scheduled
    ***************************************************************************************************/
    void GameManagerAPI::internalStartMatchmakingScenarioCb( const StartMatchmakingScenarioResponse *startScenarioResponse,
        const MatchmakingCriteriaError *startMatchmakingError, BlazeError errorCode, JobId rpcJobId, JobId jobId)
    {
        JobScheduler *jobScheduler = getBlazeHub()->getScheduler();

        MatchmakingScenarioJob *matchmakingScenarioJob = (MatchmakingScenarioJob *)jobScheduler->getJob(jobId);
        if (matchmakingScenarioJob == nullptr && errorCode != ERR_OK)
        {            
            BLAZE_SDK_DEBUGF("[GMGR] StartMatchmakingScenario failed for canceled matchmakingScenarioJob(%u); no action needed.\n", jobId.get());
            return;
        }
        
        MatchmakingScenario* matchmakingScenario = nullptr;
        const char8_t* errMsg = "";

        if (errorCode == ERR_OK)
        {
            // success; create MM Session & wait for session finish
            if (mApiParams.mMaxMatchmakingScenarios > 0)
            {
                //  Cap on the number of concurrent matchmaking sessions.
                BlazeAssertMsg(mMatchmakingScenarioList.size() < mApiParams.mMaxMatchmakingScenarios, "Object count exceeded memory cap specified in GameManagerApiParams");
            }

            matchmakingScenario = new (mMatchmakingScenarioMemoryPool.alloc()) MatchmakingScenario(this, startScenarioResponse, mMemGroup);
            if (matchmakingScenarioJob == nullptr)
            {
                // Job Canceled by user, we still need to save the session to correlate the MM finish coming later
                // we can't delete the MM session and let a failed lookup indicate 'canceled' because that's indistinguishable from a PG member's MM join inside onNotifyGameSetup
                BLAZE_SDK_DEBUGF("[GMGR] StartMatchmakingScenario succeeded for canceled matchmakingScenarioJob(%u); MM Session(%" PRIu64 "); send a catch up cancel.\n", jobId.get(), startScenarioResponse->getScenarioId() );
                // mark it as cancel and send a cancel to the server
                matchmakingScenario->cancelScenario();
            }
            mMatchmakingScenarioList.push_back(matchmakingScenario);
            BLAZE_SDK_DEBUGF("[GMGR] StartMatchmakingScenario started for matchmakingScenarioJob(%u); Job %s, waiting for NotifyMatchmakingScenarioFinished.\n", jobId.get(), matchmakingScenario->isCanceled()?"canceled":"finished");
        }
        else
        {
            // get err message, if any.  Note: startMatchmakingError is nullptr if the RPC times out (no response from server == no server error obj)
            if (startMatchmakingError != nullptr)
            {
                errMsg = startMatchmakingError->getErrMessage();
            }
            BLAZE_SDK_DEBUGF("[GMGR] Matchmaking scenario failed to start with %s ERR(%d) %s\n", 
                getBlazeHub()->getErrorName(errorCode, getBlazeHub()->getUserManager()->getPrimaryLocalUserIndex()), 
                errorCode, errMsg);
        }

        if (matchmakingScenarioJob != nullptr)
        {
            // dispatch title's startMatchmakingScenario callback
            // We have a separate async call for when the session is actually done.
            StartMatchmakingScenarioCb titleCb;
            matchmakingScenarioJob->getTitleCb(titleCb);
            titleCb(errorCode, jobId, matchmakingScenario, errMsg);
            jobScheduler->removeJob(jobId);
        }
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


    /*! ************************************************************************************************/
    /*! \brief Return the scenario, or nullptr if the index is out of bounds
    ***************************************************************************************************/
    MatchmakingScenario* GameManagerAPI::getMatchmakingScenarioByIndex(uint32_t index) const
    {
        if (index < mMatchmakingScenarioList.size())
        {
            return mMatchmakingScenarioList[index];
        }
        else
        {
            // Error: index out of bounds
            BlazeAssertMsg(false,  "Error: MatchmakingScenarioIndex is out of bounds." );
            return nullptr;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Return the scenario with the supplied id, or nullptr if not found locally
    ***************************************************************************************************/
    MatchmakingScenario* GameManagerAPI::getMatchmakingScenarioById(MatchmakingScenarioId matchmakingScenarioId) const
    {
        // there should only be a small # of sessions active at once, so we just do a linear search
        MatchmakingScenario* matchmakingScenario;
        MatchmakingScenarioList::const_iterator iter = mMatchmakingScenarioList.begin();
        MatchmakingScenarioList::const_iterator end = mMatchmakingScenarioList.end();
        for( ; iter!=end; ++iter)
        {
            matchmakingScenario = *iter;
            if (matchmakingScenario->getScenarioId() == matchmakingScenarioId)
            {
                return matchmakingScenario;
            }
        }

        return nullptr; // session with supplied id not found
    }


    /*! ************************************************************************************************/
    /*! \brief Internal async handler for NotifyMatchmakingFinished messages.  Dispatches onNotifyMatchmakingFinished
            to registered GameManagerListeners.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyMatchmakingFailed(const NotifyMatchmakingFailed* notifyMatchmakingFailed, uint32_t userIndex)
    {
        // lookup the matchmaking session
        MatchmakingScenarioId matchmakingScenarioId = notifyMatchmakingFailed->getScenarioId();
        MatchmakingScenario* matchmakingResults = getMatchmakingScenarioById(matchmakingScenarioId);
        if ( matchmakingResults == nullptr )
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] dropping NotifyMatchmakingFinished for unknown or canceled (matchmakingScenarioId %" PRIu64 ")\n", userIndex, matchmakingScenarioId);
            return;
        }

        matchmakingResults->finishMatchmaking(0, notifyMatchmakingFailed->getMaxPossibleFitScore(), notifyMatchmakingFailed->getMatchmakingResult(), 0);

        if (matchmakingResults->isCanceled())
        {
            // if client has canceled the local session, we override the notification's result 
            //  (in case the cancel RPC crossed the finished notification on the wire).
            matchmakingResults->setMatchmakingResult(SESSION_CANCELED);
        }

        // we didn't match anything
        dispatchNotifyMatchmakingScenarioFinished(*matchmakingResults, nullptr);
    }

    void GameManagerAPI::onNotifyMatchmakingSessionConnectionValidated(const NotifyMatchmakingSessionConnectionValidated *notifyMatchmakingConnectionValidated, uint32_t userIndex)
    {
        // lookup the matchmaking session
        MatchmakingScenarioId matchmakingScenarioId = notifyMatchmakingConnectionValidated->getScenarioId();
        GameId gameId = notifyMatchmakingConnectionValidated->getGameId();

        MatchmakingScenario* matchmakingResults = getMatchmakingScenarioById(matchmakingScenarioId);
        GameManagerApiJob* gmJob = getJob(userIndex, gameId);

        if ( gmJob == nullptr )
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] dropping NotifyMatchmakingSessionConnectionValidated for unknown or removed game %" PRIu64 ", matchmakingScenarioId %" PRIu64 "\n", 
                userIndex, gameId, matchmakingScenarioId);

            // dispatchSessionFinished true means as far as Blaze Server is concerned the MM session is over. If on edge cases the game was left but dispatchSessionFinished
            // is true, and our local client still has its matchmaking session, ensure matchmaking session finished is dispatched.
            //
            // This only applies when qos validation is enabled and NotifyMatchmakingSessionConnectionValidated is guaranteed to arrive after the client has created
            // the associated local game (or already destroyed it). When qos validation is disabled, the blazeserver will send a NotifyMatchmakingSessionConnectionValidated
            // immediately after setting up the game, and this notification may arrive on the client before the NotifyGameSetup that triggers it to create a local game.
            if (notifyMatchmakingConnectionValidated->getQosValidationPerformed() && notifyMatchmakingConnectionValidated->getDispatchSessionFinished() && (matchmakingResults != nullptr))
            {
                // with MLU, gmJob nullptr may be because non-owner user dispatched already (not because it left). Ensure have no other local users before finishing.
                if ((mUserManager != nullptr) && !matchmakingScenarioHasLocalUser(*matchmakingResults, gameId))
                {
                    // No local user exists in the MM session anymore, just clean up all the data
                    matchmakingResults->setMatchmakingResult(SESSION_ERROR_GAME_SETUP_FAILED);
                    dispatchNotifyMatchmakingScenarioFinished(*matchmakingResults, nullptr); // destroys the MM session
                }
            }
            return;
        }

        Game* game = getGameById(gameId);

        // Matchmaking session was canceled.  We don't care what the results of the connection test were.
        if (matchmakingResults != nullptr && matchmakingResults->isCanceled())
        {
            matchmakingResults->setMatchmakingResult(SESSION_CANCELED);
            dispatchNotifyMatchmakingScenarioFinished(*matchmakingResults, nullptr); // destroys the MM session

            gmJob->cancel(ERR_OK);          // this should clean up any other resources (like if in a game for example)
            getBlazeHub()->getScheduler()->removeJob(gmJob);
            return;
        }

        // No connection validation required in our current context, no-op, our job will be finished out where appropriate.
        if (gmJob->isMatchmakingConnectionValidated())
        {
            return;
        }

        if (!notifyMatchmakingConnectionValidated->getDispatchSessionFinished())
        {

            if (matchmakingResults != nullptr)
            {
                // we're done verifying the connection, MM will continue, so clear this status
                matchmakingResults->setGameId(INVALID_GAME_ID);
            }

            if (game != nullptr)
            {
                // silently destroy the local game (blaze server will handle cleaning up the session server-side)

                // don't dispatch, a matchmaking finished will come down for that if needed.
                // Matchmaking continues, so we don't do anything with our job.
                const bool dispatchGameManagerJob = false;

                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] destroying local game object for matchmakingScenarioId %" PRIu64 " and gameId %" PRIu64 " due to connection validation failure.\n", 
                    userIndex, matchmakingScenarioId, gameId);
                if (game->getLocalPlayer() != nullptr)
                {
                    destroyLocalGame(game, SYS_GAME_FAILED_CONNECTION_VALIDATION, game->getLocalPlayer()->isHost(), game->getLocalPlayer()->isActive(), dispatchGameManagerJob);
                }
                else
                {
                    destroyLocalGame(game, SYS_GAME_FAILED_CONNECTION_VALIDATION, false/*wasHost*/, false/*wasActive*/, dispatchGameManagerJob);
                }
            }
        }
        else
        {
            // owner of a non-canceled MM session that is not allowed to continue.
            // by assumption of MM success, they should have a game, either as the owner of a MM session, or an indirect group member.
            if ( game != nullptr )
            { 
                gmJob->setConnectionValidated();
                // Connection has now been verified, as long as we've also connected, we're good to close this out.
                if ( game->isNetworkCreated() && gmJob->isWaitingForConnectionValidation() )
                {
                    // Side: No need to call updateGamePresenceForLocalUser() here. MM QoS flow separately triggers
                    // onPlayerJoinComplete for the user (including creators) which updates the primary game.

                    gmJob->execute();
                    getBlazeHub()->getScheduler()->removeJob(gmJob);
                }
                // else, we haven't connected yet, just let the job know that the connection was validated.
                    // job will be executed once the connection is established.

            }
            else
            {
                BLAZE_SDK_DEBUGF("[GMGR] UI[%u] ERR - NotifyMatchmakingSessionConnectionValidated for known job, but unkown matchmakingScenarioId %" PRIu64 " and gameId %" PRIu64 "\n", 
                    userIndex, matchmakingScenarioId, gameId);
                gmJob->cancel(ERR_SYSTEM);
                getBlazeHub()->getScheduler()->removeJob(gmJob);
            }
        }
        
    }

    void GameManagerAPI::dispatchNotifyMatchmakingScenarioFinished(MatchmakingScenario &matchmakingScenario, Game *matchedGame)
    {
        // notify listeners that a matchmaking session has finished
        mDispatcher.dispatch("onMatchmakingScenarioFinished", &GameManagerAPIListener::onMatchmakingScenarioFinished, matchmakingScenario.mResult,
            const_cast<const MatchmakingScenario*>(&matchmakingScenario), matchedGame);

        // delete the finished matchmaking scenario after the dispatch
        MatchmakingScenarioList::iterator iter = eastl::find(mMatchmakingScenarioList.begin(), mMatchmakingScenarioList.end(), &matchmakingScenario);
        if (iter != mMatchmakingScenarioList.end())
        {
            mMatchmakingScenarioList.erase(iter);
            mMatchmakingScenarioMemoryPool.free(&matchmakingScenario);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Internal async handler for NotifyMatchmakingScenarioStatusList messages.  Dispatches
         onNotifyMatchmakingScenarioStatusList to registered GameManagerListeners.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyMatchmakingAsyncStatus(const NotifyMatchmakingAsyncStatus* notifyMatchmakingAsyncStatus, uint32_t userIndex)
    {
        MatchmakingScenario *matchmakingResults = getMatchmakingScenarioById(notifyMatchmakingAsyncStatus->getMatchmakingScenarioId());
        if ( (matchmakingResults!= nullptr) && !matchmakingResults->isCanceled() )
        {
            matchmakingResults->setSessionAgeSeconds(notifyMatchmakingAsyncStatus->getMatchmakingSessionAge());
            matchmakingResults->setAssignedInstanceId(notifyMatchmakingAsyncStatus->getInstanceId());
            if (!notifyMatchmakingAsyncStatus->getPackerStatus().getQualityFactorScores().empty())
                matchmakingResults->updateFitScorePercent(notifyMatchmakingAsyncStatus->getPackerStatus().getOverallQualityFactorScore());
            // notify listeners that a matchmaking session a sync status is received
            mDispatcher.dispatch("onMatchmakingScenarioAsyncStatus", &GameManagerAPIListener::onMatchmakingScenarioAsyncStatus,
                (const MatchmakingScenario*)matchmakingResults, &notifyMatchmakingAsyncStatus->getMatchmakingAsyncStatusList());
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] dropping async matchmaking status notification for %s scenario (%" PRIu64 ")\n", 
                userIndex, matchmakingResults?(matchmakingResults->isCanceled()?"canceled":"unknown"):"null", notifyMatchmakingAsyncStatus->getMatchmakingScenarioId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/


    /*! ************************************************************************************************/
    /*! \brief Internal async handler for NotifyMatchmakingPseudoSuccess messages.  Dispatches onMatchmakingSessionFinished
            to registered GameManagerListeners.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyMatchmakingPseudoSuccess(const NotifyMatchmakingPseudoSuccess *notifyMatchmakingDebugged, uint32_t userIndex)
    {
        MatchmakingScenarioId matchmakingScenarioId = notifyMatchmakingDebugged->getScenarioId();
        MatchmakingScenario* matchmakingResults = getMatchmakingScenarioById(matchmakingScenarioId);
        if ( matchmakingResults == nullptr )
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] dropping NotifyMatchmakingFinished for unknown or canceled (matchmakingScenarioId %" PRIu64 ")\n", userIndex, matchmakingScenarioId);
            return;
        }

        matchmakingResults->addDebugInfo(notifyMatchmakingDebugged->getFindGameResults(), notifyMatchmakingDebugged->getCreateGameResults());
        if (notifyMatchmakingDebugged->getMatchmakingResult() == SUCCESS_PSEUDO_FIND_GAME)
        {
            if (notifyMatchmakingDebugged->getFindGameResults().getFoundGame().getGame().getGameId() != 0)
            {
                matchmakingResults->finishMatchmaking(notifyMatchmakingDebugged->getFindGameResults().getFoundGameFitScore(), notifyMatchmakingDebugged->getFindGameResults().getMaxFitScore(), 
                    notifyMatchmakingDebugged->getMatchmakingResult(), notifyMatchmakingDebugged->getFindGameResults().getFoundGameTimeToMatch());
            }
            else
            {
                matchmakingResults->finishMatchmaking(0, 0, notifyMatchmakingDebugged->getMatchmakingResult(), TimeValue::getTimeOfDay());
            }
        }
        else if (notifyMatchmakingDebugged->getMatchmakingResult() == SUCCESS_PSEUDO_CREATE_GAME)
        {
           matchmakingResults->finishMatchmaking(0, notifyMatchmakingDebugged->getCreateGameResults().getMaxFitScore(), notifyMatchmakingDebugged->getMatchmakingResult(), notifyMatchmakingDebugged->getFindGameResults().getFoundGameTimeToMatch());
        }

        // we didn't match anything
        dispatchNotifyMatchmakingScenarioFinished(*matchmakingResults, nullptr);
    }

     /*! ************************************************************************************************/
    /*! \brief Internal async handler for NotifyMatchmakingSessionPseudoSuccess messages.  Dispatches onMatchmakingScenarioFinished
            to registered GameManagerListeners.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyMatchmakingScenarioPseudoSuccess(const NotifyMatchmakingScenarioPseudoSuccess *notifyMatchmakingDebugged, uint32_t userIndex)
    {
        if (notifyMatchmakingDebugged->getPseudoSuccessList().empty())
        {
            
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] dropping NotifyMatchmakingScenarioPseudoSuccess for (matchmakingScenarioId %" PRIu64 ") with empty success list.\n", userIndex, notifyMatchmakingDebugged->getScenarioId());
            return;
        }

        // there's no support for mutiple subsessions, so we just pass the first result to the handler
        onNotifyMatchmakingPseudoSuccess(*(notifyMatchmakingDebugged->getPseudoSuccessList().begin()), userIndex);
    }

    /*! ************************************************************************************************/
    /*! \brief notification that 1+ game attribs have changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameAttributeChanged(const NotifyGameAttribChange *notifyGameAttributeChanged, uint32_t userIndex)
    {
        GameId gameId = notifyGameAttributeChanged->getGameId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameAttributeChanged(&notifyGameAttributeChanged->getGameAttribs());
        }
        else
        {
            notifyDropWarn("NotifyGameAttributeChanged", userIndex, game, gameId);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/


    /*! ************************************************************************************************/
    /*! \brief notification that 1+ dedicated server attribs have changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyDedicatedServerAttributeChanged(const NotifyDedicatedServerAttribChange *notifyDedicatedServerAttributeChanged, uint32_t userIndex)
    {
        GameId gameId = notifyDedicatedServerAttributeChanged->getGameId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyDedicatedServerAttributeChanged(&notifyDedicatedServerAttributeChanged->getDedicatedServerAttribs());
        }
        else
        {
            notifyDropWarn("NotifyDedicatedServerAttributeChanged", userIndex, game, gameId);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/


    /*! ************************************************************************************************/
    /*! \brief notification that 1+ player attribs have changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerAttributeChanged(const NotifyPlayerAttribChange *notifyPlayerAttributeChanged, uint32_t userIndex)
    {
        GameId gameId = notifyPlayerAttributeChanged->getGameId();
        PlayerId playerId = notifyPlayerAttributeChanged->getPlayerId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyPlayerAttributeChanged(playerId, &notifyPlayerAttributeChanged->getPlayerAttribs());
        }
        else
        {
            notifyDropWarn("NotifyPlayerAttributeChanged", userIndex, game, gameId);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief notification that a player's custom data blob has changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyPlayerCustomDataChanged(const NotifyPlayerCustomDataChange *notifyPlayerCustomDataChanged, uint32_t userIndex)
    {
        GameId gameId = notifyPlayerCustomDataChanged->getGameId();
        PlayerId playerId = notifyPlayerCustomDataChanged->getPlayerId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyPlayerCustomDataChanged(playerId, &notifyPlayerCustomDataChanged->getCustomData());
        }
        else
        {
            notifyDropWarn("NotifyPlayerCustomDataChanged", userIndex, game, gameId);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief notification that a game's gameSettings bitfield has changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameSettingChanged(const NotifyGameSettingsChange *notifyGameSettingsChanged, uint32_t userIndex)
    {
        GameId gameId = notifyGameSettingsChanged->getGameId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameSettingsChanged(&notifyGameSettingsChanged->getGameSettings());
        }
        else
        {
            notifyDropWarn("NotifyGameSettingsChanged", userIndex, game, gameId);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief notification that a game's gameSettings bitfield has changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameModRegisterChanged(const NotifyGameModRegisterChanged *notifyGameModRegisterChanged, uint32_t userIndex)
    {
        GameId gameId = notifyGameModRegisterChanged->getGameId();
        Game *game = getGameById(gameId);
        if (game != nullptr  && game->isNotificationHandlingUserIndex(userIndex) && game->getGameModRegister() != notifyGameModRegisterChanged->getGameModRegister())
        {
            game->onNotifyGameModRegisterChanged(notifyGameModRegisterChanged->getGameModRegister());
        }
        else
        {
            notifyDropWarn("NotifyGameModRegisterChanged", userIndex, game, gameId);
        }
    } /*lint !e1762 - Avoid lint to check whether the member function could be made const*/ 

    /*! ************************************************************************************************/
    /*! \brief notification that a game's entry criteria has changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameEntryCriteriaChanged(const NotifyGameEntryCriteriaChanged *notifyGameEntryCriteriaChanged, uint32_t userIndex)
    {
        GameId gameId = notifyGameEntryCriteriaChanged->getGameId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameEntryCriteriaChanged(notifyGameEntryCriteriaChanged->getEntryCriteriaMap(), notifyGameEntryCriteriaChanged->getRoleEntryCriteriaMap());
        }
        else
        {
            notifyDropWarn("NotifyGameEntryCriteriaChanged", userIndex, game, gameId);
        }
    } /*lint !e1762 - Avoid lint to check whether the member function could be made const*/ 


    /*! ************************************************************************************************/
    /*! \brief notification that a dedicated server game is being reset with new values
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameReset(const NotifyGameReset *notifyGameReset, uint32_t userIndex)
    {
        GameId gameId = notifyGameReset->getGameData().getGameId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameReset(&notifyGameReset->getGameData());
        }
        else
        {
            notifyDropWarn("NotifyGameReset", userIndex, game, gameId);
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief gets the matchmaking rules configured on the blazeServer.
    ***************************************************************************************************/
    JobId GameManagerAPI::getMatchmakingConfig( const GetMatchmakingConfigCb &titleCb )
    {
        return mGameManagerComponent->getMatchmakingConfig(MakeFunctor(this, &GameManagerAPI::internalGetMatchmakingConfigCb), titleCb);
    }

    /*! ************************************************************************************************/
    /*! \brief helper that exists only to re-order the args from the RPC callback to the title callback.
    ***************************************************************************************************/
    void GameManagerAPI::internalGetMatchmakingConfigCb( const GetMatchmakingConfigResponse *matchmakingConfigResponse, 
            BlazeError errorCode, JobId jobId, GetMatchmakingConfigCb titleCb)
    {
        titleCb(errorCode, jobId, matchmakingConfigResponse);
    }  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

    JobId GameManagerAPI::getGameBrowserAttributesConfig(const GetGameBrowserScenarioConfigCb &titleCb)
    {
        return mGameManagerComponent->getGameBrowserAttributesConfig(MakeFunctor(this, &GameManagerAPI::internalGetGameBrowserAttributesConfigCb), titleCb);
    }

    void GameManagerAPI::internalGetGameBrowserAttributesConfigCb(const GetGameBrowserScenariosAttributesResponse *gameBrowserAttributesResponse, BlazeError errorCode, JobId jobId, GetGameBrowserScenarioConfigCb titleCb)
    {
        titleCb(errorCode, jobId, gameBrowserAttributesResponse);
    }

    /*! ************************************************************************************************/
    /*! \brief Return the GameBrowserList at the given index.  Returns nullptr if index is out of bounds.
    ***************************************************************************************************/
    GameBrowserList* GameManagerAPI::getGameBrowserListByIndex(uint32_t index) const
    {
        if (index < mGameBrowserListByClientIdMap.size())
        {
        GameBrowserListMap::const_iterator listMapIter = mGameBrowserListByClientIdMap.begin();
            eastl::advance(listMapIter, index);
                return listMapIter->second;
            }
        else
        {
            BlazeAssertMsg(false,  "index is out of bounds\n");
            return nullptr;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Return the GameBrowserList with the given id.  Returns nullptr if list not found.
    ***************************************************************************************************/
    GameBrowserList* GameManagerAPI::getGameBrowserListByClientId(GameBrowserListId listId) const
    {
        GameBrowserListMap::const_iterator listMapIter = mGameBrowserListByClientIdMap.find(listId);
        if (listMapIter!= mGameBrowserListByClientIdMap.end())
        {
            return listMapIter->second;
        }
        
        return nullptr;
    }

    GameBrowserList* GameManagerAPI::getGameBrowserListByServerId(GameBrowserListId listId) const
    {
        GameBrowserListMap::const_iterator listMapIter = mGameBrowserListByServerIdMap.find(listId);
        if (listMapIter!= mGameBrowserListByServerIdMap.end())
        {
            return listMapIter->second;
        }
        
        return nullptr;
    }

    /*! ************************************************************************************************/
    /*! \brief Attempt to create a GameBrowserList using the supplied parameters;
            dispatches the supplied callback functor upon success/failure.
    ***************************************************************************************************/
    JobId GameManagerAPI::createGameBrowserList(const GameBrowserList::GameBrowserListParameters &parameters,
        const CreateGameBrowserListCb &titleCb)
    {
        PlayerJoinDataHelper playerJoinData;
        playerJoinData.setTeamId(parameters.mTeamId);

        // Probably need to redesign this slightly...
        RoleMap::const_iterator curIter = parameters.mRoleMap.begin();
        RoleMap::const_iterator endIter = parameters.mRoleMap.end();
        for (; curIter != endIter; ++curIter)
        {
            for (uint16_t i = 0; i < curIter->second; ++i)
            {
                playerJoinData.getPlayerDataList().pull_back()->setRole(curIter->first);
            }
        }

        return createGameBrowserList(parameters, playerJoinData, titleCb);
    }

    JobId GameManagerAPI::createGameBrowserList(const GameBrowserList::GameBrowserListParametersBase &parameters,
        const PlayerJoinDataHelper &playerJoinData, const CreateGameBrowserListCb &titleCb)
    {
        // validate list capacity (non-zero)
        if (EA_UNLIKELY( parameters.mListCapacity == 0 ))
        {
            JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("createGameBrowserListCb", titleCb, GAMEBROWSER_ERR_INVALID_CAPACITY, jobId,
                    (GameBrowserList*)nullptr, "Error: GameBrowserList capacity must be >= 1.",
                    this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
            return jobId;
        }

        // NB: client can't do meaningful validation for the criteria or config name

        // populate the RPC request from the params obj & send up the RPC
        GetGameListRequestPtr getListRequest = GetGameListRequestPtr(BLAZE_NEW(MEM_GROUP_GAMEMANAGER, "GetGameListRequest") GetGameListRequest());
        prepareCommonGameData(getListRequest->getCommonGameData(), GAME_TYPE_GAMESESSION);

        getListRequest->setListCapacity(parameters.mListCapacity);
        getListRequest->setListConfigName(parameters.mListConfigName.c_str());
        parameters.mListCriteria.copyInto(getListRequest->getListCriteria() );
        getListRequest->setIgnoreGameEntryCriteria(parameters.mIgnoreGameEntryCriteria);
        playerJoinData.copyInto(getListRequest->getPlayerJoinData());

        // set up createGameBrowserListJob
        CreateGameBrowserListJob *createGameBrowserListJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "CreateGameBrowserListJob") CreateGameBrowserListJob(this, titleCb, getListRequest);
        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("CreateGameBrowserListJob", createGameBrowserListJob, this);

        if (parameters.mListType == GameBrowserList::LIST_TYPE_SUBSCRIPTION)
        {
            // subscription list
            mGameManagerComponent->getGameListSubscription( *getListRequest,
                MakeFunctor(this, &GameManagerAPI::internalCreateGameListCb),
                jobId, parameters.mListType, parameters.mListCapacity);
        }
        else
        {
            // snapshot list
            mGameManagerComponent->getGameListSnapshot( *getListRequest,
                MakeFunctor(this, &GameManagerAPI::internalCreateGameListCb),
                jobId, parameters.mListType, parameters.mListCapacity);
        }

        return jobId;
    }

    void GameManagerAPI::internalCreateGameBrowserListByScenarioCb(const GetGameListResponse *getGameListResponse,
        const MatchmakingCriteriaError *criteriaError, BlazeError errorCode, JobId rpcJobId, JobId jobId)
    {
        GameBrowserScenarioListJob *job = (GameBrowserScenarioListJob *)getBlazeHub()->getScheduler()->getJob(jobId);

        if (job == nullptr)
        {
            // the local player canceled the call to create the list, so destroy the list if the server created one
            // NOTE: title cb already dispatched by tracking job
            if (errorCode == ERR_OK)
            {
                DestroyGameListRequest destroyRequest;
                destroyRequest.setListId(getGameListResponse->getListId());
                mGameManagerComponent->destroyGameList(destroyRequest); // NOTE: fire & forget, since we can't respond meaningfully to any errors 
            }
            return;
        }

        // job has not been canceled; handle RPC response
        GetGameListScenarioRequestPtr getListRequest = job->mGetGameListScenarioRequest; // take back ownership of the request object
        CreateGameBrowserListCb titleCb;
        job->getTitleCb(titleCb);
        getBlazeHub()->getScheduler()->removeJob(jobId);

        if (errorCode == ERR_OK)
        {
            // Ensure our list id is not already in the list
            if ((mGameBrowserListByServerIdMap.find(getGameListResponse->getListId()) != mGameBrowserListByServerIdMap.end()))
            {
                BlazeAssertMsg(false, "Error: GM client already contains a list with this listId.");
            }

            // create client side GameBrowserList obj & dispatch title's callback
            if (mApiParams.mMaxGameBrowserLists > 0)
            {
                //  cap on the max number of concurrent gamebrowser lists.
                BlazeAssertMsg(mGameBrowserListByClientIdMap.size() < mApiParams.mMaxGameBrowserLists, "Object count exceeded memory cap specified in GameManagerApiParams");
            }
            
            GameBrowserList *newList = new (mGameBrowserMemoryPool.alloc()) GameBrowserList(
                this, mNextGameBrowserListId++, getGameListResponse->getIsSnapshotList()? GameBrowserList::LIST_TYPE_SNAPSHOT : GameBrowserList::LIST_TYPE_SUBSCRIPTION, getGameListResponse->getListCapacity(), nullptr, EA::TDF::OBJECT_ID_INVALID, getGameListResponse, mMemGroup, getListRequest);
            mGameBrowserListByClientIdMap[newList->getListId()] = newList;
            mGameBrowserListByServerIdMap[newList->mServerListId] = newList;
            titleCb(errorCode, jobId, newList, "");

            getBlazeHub()->getScheduler()->scheduleMethod("verifyGameBrowserList", this, &GameManagerAPI::verifyGameBrowserList, newList->getListId(), this, (uint32_t)(newList->getMaxUpdateInterval().getMillis()));
        }
        else
        {
            // server returned error, dispatch title cb
            const char8_t *errMessage = "";
            if (criteriaError != nullptr)
            {
                errMessage = criteriaError->getErrMessage(); // criteriaError can be nullptr, for example if the RPC times out...
            }
            titleCb(errorCode, jobId, nullptr, errMessage);
        }
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    /*! ************************************************************************************************/
    /*! \brief internal sdk callback when attempting to create a GameBrowserList; parse server result
            and dispatch title's callback.
    
        Note: We need to pass along the listType & listCapacity because the server response doesn't include those.
    ***************************************************************************************************/
    void GameManagerAPI::internalCreateGameListCb(const GetGameListResponse *getGameListResponse,
        const MatchmakingCriteriaError *criteriaError, BlazeError errorCode, JobId rpcJobId, JobId jobId, 
        GameBrowserList::ListType listType, uint32_t listCapacity)
    {
        CreateGameBrowserListJob *job = (CreateGameBrowserListJob *) getBlazeHub()->getScheduler()->getJob(jobId);

        if (job == nullptr)
        {
            // the local player canceled the call to create the list, so destroy the list if the server created one
            // NOTE: title cb already dispatched by tracking job
            if (errorCode == ERR_OK)
            {
                DestroyGameListRequest destroyRequest;
                destroyRequest.setListId( getGameListResponse->getListId() );
                mGameManagerComponent->destroyGameList(destroyRequest); // NOTE: fire & forget, since we can't respond meaningfully to any errors 
            }
            return;
        }

        // job has not been canceled; handle RPC response
        GetGameListRequestPtr getListRequest = job->mGetGameListRequest; // take back ownership of the request object
        CreateGameBrowserListCb titleCb;
        job->getTitleCb(titleCb);
        getBlazeHub()->getScheduler()->removeJob(jobId);

        if (errorCode == ERR_OK)
        {
            // Ensure our list id is not already in the list
            if ((mGameBrowserListByServerIdMap.find(getGameListResponse->getListId()) != mGameBrowserListByServerIdMap.end()))
            {
                BlazeAssertMsg(false,  "Error: GM client already contains a list with this listId.");
            }

            // create client side GameBrowserList obj & dispatch title's callback
            if (mApiParams.mMaxGameBrowserLists > 0)
            {
                //  cap on the max number of concurrent gamebrowser lists.
                BlazeAssertMsg(mGameBrowserListByClientIdMap.size() < mApiParams.mMaxGameBrowserLists, "Object count exceeded memory cap specified in GameManagerApiParams");
            }

            GameBrowserList *newList = new (mGameBrowserMemoryPool.alloc()) GameBrowserList(
                this, mNextGameBrowserListId++, listType, listCapacity, getListRequest, EA::TDF::OBJECT_ID_INVALID, getGameListResponse, mMemGroup);
            mGameBrowserListByClientIdMap[newList->getListId()] = newList;
            mGameBrowserListByServerIdMap[newList->mServerListId] = newList;
            titleCb(errorCode, jobId, newList, "");

            getBlazeHub()->getScheduler()->scheduleMethod("verifyGameBrowserList", this, &GameManagerAPI::verifyGameBrowserList, newList->getListId(), this, (uint32_t)(newList->getMaxUpdateInterval().getMillis()));
        }
        else
        {
            // server returned error, dispatch title cb
            const char8_t *errMessage = "";
            if (criteriaError != nullptr)
            {
                errMessage = criteriaError->getErrMessage(); // criteriaError can be nullptr, for example if the RPC times out...
            }
            titleCb(errorCode, jobId, nullptr, errMessage);
        }
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

    JobId GameManagerAPI::subscribeUserSetGameList(const UserGroupId& userSetId, const CreateGameBrowserListCb &titleCb)
    {        
        Blaze::GameManager::GetUserSetGameListSubscriptionRequest request;
        request.setUserSetId(userSetId);

        // set up createGameBrowserListJob
        CreateGameBrowserListJob *createGameBrowserListJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "SubscribeUserSetJob") CreateGameBrowserListJob(this,titleCb,nullptr);
        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("SubscribeUserSetJob", createGameBrowserListJob, this);
        
        mGameManagerComponent->getUserSetGameListSubscription( request, MakeFunctor(this, &GameManagerAPI::internalGetUserSetGameListSubscriptionCb),userSetId,jobId);
        return jobId;
    }

    void GameManagerAPI::internalGetUserSetGameListSubscriptionCb(const GetGameListResponse *getGameListResponse,const MatchmakingCriteriaError *criteriaError, BlazeError errorCode, JobId rpcJobId, UserGroupId userSetId, JobId jobId)
    {
        CreateGameBrowserListJob *job = (CreateGameBrowserListJob *) getBlazeHub()->getScheduler()->getJob(jobId);
        if (job == nullptr)
        {
            // the local player canceled the call to create the list, so destroy the list if the server created one
            // NOTE: title cb already dispatched by tracking job
            if (errorCode == ERR_OK)
            {
                DestroyGameListRequest destroyRequest;
                destroyRequest.setListId( getGameListResponse->getListId() );
                mGameManagerComponent->destroyGameList(destroyRequest); // NOTE: fire & forget, since we can't respond meaningfully to any errors 
            }
            return;
        }

        // job has not been canceled; handle RPC response
        CreateGameBrowserListCb titleCb;
        job->getTitleCb(titleCb);
        getBlazeHub()->getScheduler()->removeJob(jobId);

        if (errorCode == ERR_OK)
        {
            // Ensure our list id is not already in the list
            if ((mGameBrowserListByServerIdMap.find(getGameListResponse->getListId()) != mGameBrowserListByServerIdMap.end()))
            {
                BlazeAssertMsg(false,  "Error: GM client already contains a list with this listId.");
            }

            // create client side GameBrowserList obj & dispatch title's callback
            if (mApiParams.mMaxGameBrowserLists > 0)
            {
                //  cap on the max number of concurrent gamebrowser lists.
                BlazeAssertMsg(mGameBrowserListByClientIdMap.size() < mApiParams.mMaxGameBrowserLists, "Object count exceeded memory cap specified in GameManagerApiParams");
            }

            GameBrowserList *newList = new (mGameBrowserMemoryPool.alloc()) GameBrowserList(
                this, mNextGameBrowserListId++, GameBrowserList::LIST_TYPE_SUBSCRIPTION, 100, nullptr, userSetId, getGameListResponse, mMemGroup);
            mGameBrowserListByClientIdMap[newList->getListId()] = newList;
            mGameBrowserListByServerIdMap[newList->mServerListId] = newList;
            titleCb(errorCode, jobId, newList, "");

            getBlazeHub()->getScheduler()->scheduleMethod("verifyGameBrowserList", this, &GameManagerAPI::verifyGameBrowserList, newList->getListId(), this, (uint32_t)(newList->getMaxUpdateInterval().getMillis()));
        }
        else
        {
            // server returned error, dispatch title cb
            const char8_t *errMessage = "";
            if (criteriaError != nullptr)
            {
                errMessage = criteriaError->getErrMessage(); // criteriaError can be nullptr, for example if the RPC times out...
            }
            titleCb(errorCode, jobId, nullptr, errMessage);
        }
    }  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/


    JobId GameManagerAPI::internalLookupGamesByUser(const UserManager::User *user, const GameBrowserListConfigName &listConfigName, const LookupGamesByUserCb &titleCb, bool disconnectedReservation)
    {
        if (user == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] User passed to lookupGamesByUser was nullptr.\n");
            JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("internalLookupGamesByUserCb", titleCb, ERR_SYSTEM, jobId, (const GameBrowserDataList*) nullptr,
                    this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
            return jobId;
        }

        if (user->getId() == INVALID_BLAZE_ID && !BlazeHub::hasExternalIdentifier(user->getPlatformInfo()) && (user->getName())[0] == '\0')
        {
            // no data to look up by
            BLAZE_SDK_DEBUGF("[GMGR] User passed to lookupGamesByUser had no BlazeId, PlatformInfo or name set.\n");
            JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("internalLookupGamesByUserCb", titleCb, ERR_SYSTEM, jobId, (const GameBrowserDataList*) nullptr,
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
            return jobId;
        }

        GetGameDataByUserRequest request;
        request.getUser().setBlazeId(user->getId());
        request.getUser().setName(user->getName());
        user->getPlatformInfo().copyInto(request.getUser().getPlatformInfo());

        request.setListConfigName(listConfigName.c_str());

        JobId jobId = mGameManagerComponent->getGameDataByUser(request, MakeFunctor(this, &GameManagerAPI::internalLookupGamesByUserCb), titleCb, disconnectedReservation, user);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief Lookup a user's games (games they are a member of) and return a snapshot of the game information.
            The subset of game data returned is filtered using a GameBrowserList configuration.
    ***************************************************************************************************/
    JobId GameManagerAPI::lookupGamesByUser(const UserManager::User *user, const GameBrowserListConfigName &listConfigName, const LookupGamesByUserCb &titleCb)
    {
        return internalLookupGamesByUser(user, listConfigName, titleCb, false);
    }

    /*! ************************************************************************************************/
    /*! \brief Lookup a user's games with reservations made for the user and 
            return a snapshot of the game information.
    ***************************************************************************************************/
    JobId GameManagerAPI::lookupReservationsByUser(const UserManager::User *user, const GameBrowserListConfigName &listConfigName, const LookupGamesByUserCb &titleCb)
    {
        return internalLookupGamesByUser(user, listConfigName, titleCb, true);
    }

    JobId GameManagerAPI::cancelGameReservation(const UserManager::User *user, GameId gameId, const CancelGameReservationCb &titleCb)
    {
        if (user == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] User passed to cancelGameReservation was nullptr.\n");
            JobScheduler *jobScheduler = getBlazeHub()->getScheduler();
            JobId jobId = jobScheduler->reserveJobId();
            jobId = jobScheduler->scheduleFunctor("internalCancelGameReservation", titleCb, ERR_SYSTEM, jobId,
                this, 0, jobId); // assocObj, delayMs, reservedJobId
            Job::addTitleCbAssociatedObject(jobScheduler, jobId, titleCb);
            return jobId;
        }


        BLAZE_SDK_DEBUGF("[GAME] Player(%" PRId64 ":%s) attempting to cancel reservation in game(%" PRIu64 ")\n",
            user->getId(), user->getName(), gameId);

        // Setup request
        RemovePlayerRequest removePlayerRequest;
        removePlayerRequest.setGameId(gameId);
        removePlayerRequest.setPlayerId(user->getId());
        removePlayerRequest.setPlayerRemovedReason(PLAYER_LEFT);
        JobId jobId = mGameManagerComponent->removePlayer(removePlayerRequest, titleCb);
        Job::addTitleCbAssociatedObject(getBlazeHub()->getScheduler(), jobId, titleCb);
        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief RPC callback when looking up a user's games; re-arranges the callback's argument order
    ***************************************************************************************************/
    void GameManagerAPI::internalLookupGamesByUserCb(const GameBrowserDataList *gameBrowserDataList, BlazeError errorCode, JobId rpcJobId, LookupGamesByUserCb titleCb, bool checkReservation, const UserManager::User *user)
    {
        // GM_AUDIT: the internal callback only exists to change the order of the args in the callback - we should just use the low level callback and remove
        //  the need for the internal callback.

        GameBrowserDataList *returnedGameBrowserDataList = const_cast<GameBrowserDataList*>(gameBrowserDataList);

        if (checkReservation && gameBrowserDataList != nullptr)
        {
            // only return games that have disconnect reservations for the requested user
            GameBrowserDataList::GameBrowserGameDataList::iterator gdItr = returnedGameBrowserDataList->getGameData().begin();
            while (gdItr != returnedGameBrowserDataList->getGameData().end())
            {
                GameBrowserGameData* gameData = *gdItr;
                GameBrowserGameData::GameBrowserPlayerDataList::const_iterator pdItr, pdEnd;
                bool removeGame = true;
                for (pdItr = gameData->getGameRoster().begin(), pdEnd = gameData->getGameRoster().end(); pdItr != pdEnd; ++pdItr)
                {
                    const GameBrowserPlayerData* playerData = *pdItr;
                    bool matchUserInfo = (playerData->getPlayerId() != INVALID_BLAZE_ID && playerData->getPlayerId() == user->getId()) ||
                        (playerData->getPlatformInfo().getExternalIds().getXblAccountId() != INVALID_XBL_ACCOUNT_ID && playerData->getPlatformInfo().getExternalIds().getXblAccountId() == user->getPlatformInfo().getExternalIds().getXblAccountId()) ||
                        (playerData->getPlatformInfo().getExternalIds().getPsnAccountId() != INVALID_PSN_ACCOUNT_ID && playerData->getPlatformInfo().getExternalIds().getPsnAccountId() == user->getPlatformInfo().getExternalIds().getPsnAccountId()) ||
                        (playerData->getPlatformInfo().getExternalIds().getSteamAccountId() != INVALID_STEAM_ACCOUNT_ID && playerData->getPlatformInfo().getExternalIds().getSteamAccountId() == user->getPlatformInfo().getExternalIds().getSteamAccountId()) ||
                        (playerData->getPlatformInfo().getExternalIds().getStadiaAccountId() != INVALID_STADIA_ACCOUNT_ID && playerData->getPlatformInfo().getExternalIds().getStadiaAccountId() == user->getPlatformInfo().getExternalIds().getStadiaAccountId()) ||
                        (!playerData->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString().empty() && playerData->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString() == user->getPlatformInfo().getExternalIds().getSwitchIdAsTdfString()) ||
                        (playerData->getPlatformInfo().getEaIds().getNucleusAccountId() != INVALID_ACCOUNT_ID && playerData->getPlatformInfo().getEaIds().getNucleusAccountId() == user->getPlatformInfo().getEaIds().getNucleusAccountId()) ||
                        ((playerData->getPlayerName())[0] != '\0' && DirtyUsernameCompare(playerData->getPlayerName(), user->getName()) == 0);
                    bool isReserved = (playerData->getPlayerState() == RESERVED);
                    if (matchUserInfo && isReserved)
                    {
                        BLAZE_SDK_DEBUGF("[GMGR] Reservation for user(%" PRId64 ":%s) is found in game id:(%" PRIu64 ")\n", user->getId(), user->getName(), gameData->getGameId());
                        removeGame = false;
                        break;
                    }
                }

                if (removeGame)
                {
                    gdItr = returnedGameBrowserDataList->getGameData().erase(gdItr);
                }
                else
                {
                    ++gdItr;
                }
            }
        }

        titleCb(errorCode, rpcJobId, returnedGameBrowserDataList); // note: gameBrowserDataList will be nullptr on error, as expected.
    }  /*lint !e1746 !e1762 - Avoid lint to check whether parameter or function could be made const reference or const*/

    /*! ************************************************************************************************/
    /*! \brief make the request to verify a game browser list

        \param[in] request info for the list to verify
    ***************************************************************************************************/
    void GameManagerAPI::verifyGameBrowserList(GameBrowserListId listId)
    {
        GameBrowserList* list = getGameBrowserListByClientId(listId);
        if (list == nullptr || list->isListFinishedUpdating())
        {
            BLAZE_SDK_DEBUGF("[GMGR] Game browser list %" PRIu64 " is %s, ending periodic verification checks.\n",
                listId, (list == nullptr ? "gone" : "finished"));
            return;
        }

        // Note that the server should send updates somewhat faster than the heartbeat interval, the interval the server has sent us
        // additionally builds in a grace period to allow for slight delays in arrival of updates
        uint32_t currentTime = NetTick();
        if (NetTickDiff(currentTime, list->getLastUpdateTime()) > list->getMaxUpdateInterval().getMillis())
        {
            // At this point assume the subscription is no longer valid on the server, we'll want to recreate it,
            // but first issue a destroy call to make sure it's really gone and clean up any lingering resources on the server
            BLAZE_SDK_DEBUGF("[GMGR] Warning, expected update for list %" PRIu64 " has not been received\n", listId);

            DestroyGameListRequest request;
            request.setListId(list->mServerListId);
            mGameManagerComponent->destroyGameList(request, MakeFunctor(this, &GameManagerAPI::verifyGameBrowserListCb), listId);
            return;
        }

        // At this point we've received a recent notification from the server, thus our list should be healthy, schedule our next check
        getBlazeHub()->getScheduler()->scheduleMethod("verifyGameBrowserList", this, &GameManagerAPI::verifyGameBrowserList, listId, this, (uint32_t)(list->getMaxUpdateInterval().getMillis()));
    }

    void GameManagerAPI::verifyGameBrowserListCb(BlazeError errorCode, JobId rpcJobId, GameBrowserListId listId)
    {
        // handle overload
        if (errorCode != ERR_OK && errorCode != GAMEBROWSER_ERR_INVALID_LIST_ID)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Warning, unexpected error %d while cleaning up the previous server list for client list %" PRIu64 "\n", errorCode, listId);
        }

        if (errorCode != ERR_OK && errorCode != GAMEBROWSER_ERR_INVALID_LIST_ID)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Warning, unexpected error %d while cleaning up the previous server list for client list %" PRIu64 "\n", errorCode, listId);
        }

        GameBrowserList* list = getGameBrowserListByClientId(listId);
        if (list == nullptr || list->isListFinishedUpdating())
        {
            // List may have been removed, or last update may have snuck in while we were busy attempting to clean up on the server
            return;
        }

        // For snapshot lists we will not recreate, let the client know we've timed out
        if (list->getListType() == GameBrowserList::LIST_TYPE_SNAPSHOT)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Destroyed snapshot list %" PRIu64 " after expected updates were not received from server\n", listId);
            list->getGameManagerAPI()->dispatchOnGameBrowserListUpdateTimeout(list);
            return;
        }

        // Now for subscription requests, kick off a re-subscription attempt
        if (list->mGetGameListScenarioRequest != nullptr)
        {
            mGameManagerComponent->getGameListByScenario(*(list->mGetGameListScenarioRequest), MakeFunctor(this, &GameManagerAPI::resubscribeGamebrowserListCb), listId);
        }
        else if (list->mGetGameListRequest != nullptr)
        {
            mGameManagerComponent->getGameListSubscription(*(list->mGetGameListRequest), MakeFunctor(this, &GameManagerAPI::resubscribeGamebrowserListCb), listId);
        }
        else
        {
            GetUserSetGameListSubscriptionRequest request;
            request.setUserSetId(list->mUserGroupId);
            mGameManagerComponent->getUserSetGameListSubscription(request, MakeFunctor(this, &GameManagerAPI::resubscribeGamebrowserListCb), listId);
        }
    }

    void GameManagerAPI::resubscribeGamebrowserListCb(const GetGameListResponse *getGameListResponse, const MatchmakingCriteriaError *criteriaError, BlazeError errorCode, JobId rpcJobId, GameBrowserListId listId)
    {
        GameBrowserListMap::const_iterator it = mGameBrowserListByClientIdMap.find(listId);
        if (it == mGameBrowserListByClientIdMap.end())
        {
            // Assume client destroyed the list while we were busy trying to resubscribe,
            // we thus have no need for the new subscription we just created, if our resubscribe failed
            // no need to do anything, but if we succeeded we now have to clean up the unnecessary subscription
            if (errorCode == ERR_OK)
            {
                DestroyGameListRequest destroyRequest;
                destroyRequest.setListId(getGameListResponse->getListId());
                mGameManagerComponent->destroyGameList(destroyRequest); // NOTE: fire & forget, since we can't respond meaningfully to any errors
            }

            return;
        }

        GameBrowserList* list = it->second;

        // In the more likely case that the list still does exist, if we failed to resubscribe we have to let the title know
        // so that their data does not simply go stale as we have no way of knowing in a failure case just how long it may take
        // to re-establish a subscription
        if (errorCode != ERR_OK)
        {
            BLAZE_SDK_DEBUGF("[GMGR] Warning, could not re-establish subscription for list %" PRIu64 " generating final update now\n", listId);
            list->sendFinalUpdate();
            return;
        }

        BLAZE_SDK_DEBUGF("[GMGR] List %" PRIu64 " has been resubscribed, previous server id %" PRIu64 " new server id %" PRIu64 "\n",
            listId, list->mServerListId, getGameListResponse->getListId());

        // Update our mappings to reflect the new server list id
        mGameBrowserListByServerIdMap.erase(list->mServerListId);
        mGameBrowserListByServerIdMap[getGameListResponse->getListId()] = list;
        list->resetSubscription(getGameListResponse->getListId());

        // Finally kick off the loop to verify our new subscription stays healthy
        getBlazeHub()->getScheduler()->scheduleMethod("verifyGameBrowserList", this, &GameManagerAPI::verifyGameBrowserList, list->getListId(), this, (uint32_t)(list->getMaxUpdateInterval().getMillis()));
    }

    /*! ************************************************************************************************/
    /*! \brief destroy the supplied game browser list, removing it from the game browser list map.
                Note: called by GameBrowserList::destroy
    
        \param[in] list the list to destroy
    ***************************************************************************************************/
    void GameManagerAPI::destroyGameBrowserList(GameBrowserList *list)
    {
        if (list == nullptr)
        {
            BlazeAssertMsg(false,  "calling destroyGameBrowserList with a nullptr list pointer.\n");
            return;
        }

        // if snapshot, and we know its already finished updating, its already destroyed. no need to send rpc to server.
        if ((list->getListType() != GameBrowserList::LIST_TYPE_SNAPSHOT) || !list->isListFinishedUpdating())
        {
            // send destroyGameList RPC
            DestroyGameListRequest destroyRequest;
            destroyRequest.setListId( list->mServerListId );
            mGameManagerComponent->destroyGameList(destroyRequest); // NOTE: fire & forget, since we can't respond meaningfully to any errors
        }

        mDispatcher.dispatch("onGameBrowserListDestroy", &GameManagerAPIListener::onGameBrowserListDestroy, list);

        // destroy local copy of the list
        mGameBrowserListByClientIdMap.erase(list->getListId());
        mGameBrowserListByServerIdMap.erase(list->mServerListId);
        mGameBrowserMemoryPool.free(list);
    }

    /*! ************************************************************************************************/
    /*! \brief Attempt to start gamebrowser scenarios using the supplied parameters;
    dispatches the supplied callback functor upon success/failure.
    ***************************************************************************************************/
    JobId GameManagerAPI::createGameBrowserListByScenario(const ScenarioName& gameBrowserScenarioName, const ScenarioAttributes& gameBrowserScenarioAttributes, const PlayerJoinDataHelper& playerData, const CreateGameBrowserListCb &titleCb)
    {
        // populate the RPC request from the params obj & send up the RPC
        GetGameListScenarioRequestPtr getListRequest = GetGameListScenarioRequestPtr(BLAZE_NEW(MEM_GROUP_GAMEMANAGER, "GetGameListScenarioRequest") GetGameListScenarioRequest());
        prepareCommonGameData(getListRequest->getCommonGameData(), GAME_TYPE_GAMESESSION);

        getListRequest->setGameBrowserScenarioName(gameBrowserScenarioName);
        gameBrowserScenarioAttributes.copyInto(getListRequest->getGameBrowserScenarioAttributes());

        // Populate playerData.
        playerData.copyInto(getListRequest->getPlayerJoinData());

        // set up createGameBrowserListJob
        GameBrowserScenarioListJob *getGameBrowserScenarioListJob = BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "GameBrowserScenarioListJob") GameBrowserScenarioListJob(this, titleCb, getListRequest);
        JobId jobId = getBlazeHub()->getScheduler()->scheduleJobNoTimeout("GameBrowserScenarioListJob", getGameBrowserScenarioListJob, this);

        mGameManagerComponent->getGameListByScenario(*getListRequest,
            MakeFunctor(this, &GameManagerAPI::internalCreateGameBrowserListByScenarioCb),
            jobId);

        return jobId;
    }

    /*! ************************************************************************************************/
    /*! \brief handle an async update to a GameBrowserList.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameListUpdate(const NotifyGameListUpdate *notifyGameListUpdate, uint32_t userIndex)
    {
        // lookup list
        GameBrowserList *gameList = getGameBrowserListByServerId( notifyGameListUpdate->getListId() );
        if (gameList != nullptr)
        {
            gameList->onNotifyGameListUpdate(notifyGameListUpdate);
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Warning: dropping async NotifyGameListUpdate for unknown gameList(%" PRIu64 ")\n", userIndex, notifyGameListUpdate->getListId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    /*! ************************************************************************************************/
    /*! \brief helper to dispatch a GB list update (from the inside the list class)
    ***************************************************************************************************/
    void GameManagerAPI::dispatchOnGameBrowserListUpdated(GameBrowserList *gameBrowserList, 
            const GameBrowserList::GameBrowserGameVector &removedGames,
            const GameBrowserList::GameBrowserGameVector &updatedGames) const
    {
        mDispatcher.dispatch("onGameBrowserListUpdated", &GameManagerAPIListener::onGameBrowserListUpdated, gameBrowserList, &removedGames, &updatedGames);
    }

    /*! ************************************************************************************************/
    /*! \brief helper to dispatch a GB list update timeout (from inside the list class)
    ***************************************************************************************************/
    void GameManagerAPI::dispatchOnGameBrowserListUpdateTimeout(GameBrowserList *gameBrowserList) const
    {
        mDispatcher.dispatch("onGameBrowserListUpdateTimeout", &GameManagerAPIListener::onGameBrowserListUpdateTimeout, gameBrowserList);
    }

    /*! ************************************************************************************************/
    /*! \brief async notification that a game has been removed on the blaze server
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameRemoved(const NotifyGameRemoved *notification, uint32_t userIndex)
    {
        Game *game = getGameById( notification->getGameId() );
        if (game != nullptr)
        {            
            if(game->isDedicatedServerHost())  
            {
                // dedicated server host isn't active
                destroyLocalGame( game, notification->getDestructionReason(), game->isTopologyHost(), false/*wasActive*/ );
            }
            // else the user was a player and will have the game destroyed on removal from game
        }
        else
        {
            notifyDropWarn("NotifyGameRemoved", userIndex, game, notification->getGameId());

            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Warning: dropping async NotifyGameRemoved for %s game(%" PRIu64 ")\n", userIndex, game == nullptr ? "unknown" : "non-primary local user in",  notification->getGameId());
        }

    }

    void GameManagerAPI::onNotifyProcessQueue(const NotifyProcessQueue *notification, uint32_t userIndex)
    {
        Game *game = getGameById(notification->getGameId());
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyProcessQueue();
        }
        else
        {
            notifyDropWarn("NotifyProcessQueue", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    void GameManagerAPI::onNotifyQueueChanged(const NotifyQueueChanged *notification, uint32_t userIndex)
    {
        Game *game = getGameById(notification->getGameId());
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyQueueChanged(notification->getPlayerIdList());
        }
        else
        {
            notifyDropWarn("NotifyQueueChanged", userIndex, game, notification->getGameId());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief notification that the game name have been changed
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameNameChanged(const NotifyGameNameChange *notification, uint32_t userIndex)
    {
        GameId gameId = notification->getGameId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameNameChanged(notification->getGameName());
        }
        else
        {
            notifyDropWarn("NotifyGameNameChanged", userIndex, game, notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    void GameManagerAPI::onNotifyPresenceModeChanged(const NotifyPresenceModeChanged *notification, uint32_t userIndex)
    {
        GameId gameId = notification->getGameId();
        Game *game = getGameById(gameId);
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyPresenceModeChanged(notification->getNewPresenceMode());
        }
        else
        {
            notifyDropWarn("NotifyPresenceModeChanged", userIndex, game, notification->getGameId());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief handle an async update to a Game's admin list.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyGameAdminListChange(const NotifyAdminListChange *notifyAdminListChange, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Info: call onNotifyGameAdminListChange for game(%" PRIu64 ")", userIndex, notifyAdminListChange->getGameId());
        Game *game = getGameById( notifyAdminListChange->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyGameAdminListChange(notifyAdminListChange->getAdminPlayerId(), notifyAdminListChange->getOperation(), notifyAdminListChange->getUpdaterPlayerId());
        }
        else
        {
            notifyDropWarn("NotifyGameAdminListChange", userIndex, game, notifyAdminListChange->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    Blaze::BlazeError GameManagerAPI::validateTeamIndex( const TeamIdVector &teamIdVector, const PlayerJoinData &playerJoinData ) const
    {
        // Teams are not used for Spectators, so we do not need a restriction here. 
        if (isSpectatorSlot(playerJoinData.getDefaultSlotType()))
        {
            return ERR_OK;
        }

        if (playerJoinData.getDefaultTeamIndex() != UNSPECIFIED_TEAM_INDEX && playerJoinData.getDefaultTeamIndex() >= teamIdVector.size())
        {
            BLAZE_SDK_DEBUGF("[GMGR] Default Team Index (%u) is outside of the provided team id vector.\n", playerJoinData.getDefaultTeamIndex());
            return GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
        }

        // Check that all players have a valid team index: 
        for (PerPlayerJoinDataList::const_iterator itr = playerJoinData.getPlayerDataList().begin(); itr != playerJoinData.getPlayerDataList().end(); ++itr)
        {
            if (isSpectatorSlot((*itr)->getSlotType()))
            {
                continue;
            }

            if ((*itr)->getTeamIndex() != UNSPECIFIED_TEAM_INDEX && (*itr)->getTeamIndex() >= teamIdVector.size())
            {
                BLAZE_SDK_DEBUGF("[GMGR] Player Team Index (%u) is outside of the provided team id vector.\n", (*itr)->getTeamIndex());
                return GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
            }
        }

        return ERR_OK;
    }

    BlazeError GameManagerAPI::validatePlayerJoinDataForCreateOrJoinGame(const PlayerJoinData& playerJoinData) const
    {
        for (const auto& ppJoinData : playerJoinData.getPlayerDataList())
        {
            if (!ppJoinData->getRoles().empty())
            {
                // If we are using the roles list, the legacy role name should be empty
                if (ppJoinData->getRole()[0] != '\0')
                {
                    BLAZE_SDK_DEBUGF("Role name is not empty: %s\n", ppJoinData->getRole());
                    return GAMEMANAGER_ERR_ROLE_CRITERIA_INVALID;
                }

                // For non-MM requests (create, join, reset ded. server, etc.),
                // role count for each player should be one
                if (ppJoinData->getRoles().size() > 1)
                {
                    BLAZE_SDK_DEBUGF("Role size: %" PRIsize "\n", (size_t)ppJoinData->getRoles().size());
                    return GAMEMANAGER_ERR_ROLE_CRITERIA_INVALID;
                }
            }
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief impl of GameManagerAPI::setGameProtocolVersionString; set the game protocol version string
        \param[in] versionString the new game protocol version
    ***************************************************************************************************/
    void GameManagerAPI::setGameProtocolVersionString(const char8_t *versionString)
    {
        mApiParams.mGameProtocolVersionString.set(versionString);
    }

    /*! ************************************************************************************************/
    /*! \brief impl of GameManagerAPI::getGameProtocolVersionString; return a game protocol version string
    \return mGameProtocolVersionString string.
    ***************************************************************************************************/
    const char8_t* GameManagerAPI::getGameProtocolVersionString() const
    {
        return mApiParams.mGameProtocolVersionString.c_str();
    }
    
    /*! ************************************************************************************************/
    /*! \brief Cancels game setup if client's GameProtocolVersionString mismatches game
        \param[in,out] jobToCancel - used to cancel if mismatch
        \return true iff client and game have same version, or either is match-any
    ***************************************************************************************************/
    bool GameManagerAPI::validateProtocolVersion(const Game &game, GameManagerApiJob &jobToCancel) const
    {
        const bool shouldValidate = 
            ((jobToCancel.getSetupReason().getIndirectMatchmakingSetupContext() != nullptr) &&
            jobToCancel.getSetupReason().getIndirectMatchmakingSetupContext()->getRequiresClientVersionCheck()) ||
            ((jobToCancel.getSetupReason().getIndirectJoinGameSetupContext() != nullptr) &&
            jobToCancel.getSetupReason().getIndirectJoinGameSetupContext()->getRequiresClientVersionCheck());

        // error if version strings mismatch
        if (shouldValidate &&
            (blaze_strcmp(mApiParams.mGameProtocolVersionString.c_str(), game.getGameProtocolVersionString()) != 0) &&
            ((blaze_strcmp(GAME_PROTOCOL_VERSION_MATCH_ANY, game.getGameProtocolVersionString()) != 0) &&
            (blaze_strcmp(mApiParams.mGameProtocolVersionString.c_str(), GAME_PROTOCOL_VERSION_MATCH_ANY) != 0)))
        {
            BLAZE_SDK_DEBUGF("[GMGR] Warning: player exiting/canceling local game(%" PRIu64 ":%s) setup due to mismatching GameProtocolVersionString(player: %s, game: %s)\n",game.getId(),game.getName(), mApiParams.mGameProtocolVersionString.c_str(), game.getGameProtocolVersionString());
            jobToCancel.cancel(Blaze::GAMEMANAGER_ERR_GAME_PROTOCOL_VERSION_MISMATCH);
            getBlazeHub()->getScheduler()->removeJob(&jobToCancel);
            return false;
        }
        return true;
    }

    void GameManagerAPI::onPrimaryLocalUserChanged(uint32_t primaryUserIndex)
    {
        // Only a single API is exposed to the implementing client.
        // However, under the hood we still maintain the separate components per index.
        // Our primary index has changed, and we must now call all RPC's from the new component index.
        mGameManagerComponent = getBlazeHub()->getComponentManager(primaryUserIndex)->getGameManagerComponent();
    }
    
    /*! ************************************************************************************************/
    /*! \brief Internal async handler for NotifyMatchmakingReservedExternalPlayers messages.  Dispatches
         onNotifyMatchmakingReservedExternalPlayers to registered GameManagerListeners.
    ***************************************************************************************************/
    void GameManagerAPI::onNotifyMatchmakingReservedExternalPlayers(const NotifyMatchmakingReservedExternalPlayers *notification, uint32_t userIndex)
    {   
        Game *game = getGameById(notification->getGameId());
        if (game != nullptr)
        {
            GameManagerApiJob *gameSetupJob = getJob(userIndex, notification->getGameId());
            if (gameSetupJob != nullptr)
            {
                // still have a game setup job. for consistency, ensure onReservedPlayerIdentifications dispatch is after job completes.
                gameSetupJob->setReservedExternalPlayerIdentifications(notification->getJoinedReservedPlayerIdentifications());
            }
            else
            {
                dispatchOnReservedPlayerIdentifications(game, notification->getJoinedReservedPlayerIdentifications(), Blaze::ERR_OK, userIndex);
            }
        }
        else
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] Warning: dropping async NotifyMatchmakingReservedExternalPlayers for %s game(%" PRIu64 ")\n", userIndex, game == nullptr ? "unknown" : "non-primary local user in",  notification->getGameId());
        }
    }  /*lint !e1762 - Avoid lint to check whether the member function could be made const*/

    // event handlers for when some other user takes action on your behalf, such as entering mm or joining a game
    void GameManagerAPI::onNotifyRemoteMatchmakingStarted(const NotifyRemoteMatchmakingStarted *notification, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Pulled into matchmaking session(%" PRIu64 "), scenario(%" PRIu64 ") initiated by player id(%" PRIu64 ").\n", 
            notification->getMMSessionId(), notification->getMMScenarioId(),
            notification->getRemoteUserInfo().getInitiatorId());

        mDispatcher.dispatch("onRemoteMatchmakingStarted", &GameManagerAPIListener::onRemoteMatchmakingStarted, notification, userIndex);
    }

    void GameManagerAPI::onNotifyRemoteMatchmakingEnded(const NotifyRemoteMatchmakingEnded *notification, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Matchmaking session(%" PRIu64 "), scenario(%" PRIu64 ") initiated by player id(%" PRIu64 ") ended with result '%s'.\n",
            notification->getMMSessionId(), notification->getMMScenarioId(),
            notification->getRemoteUserInfo().getInitiatorId(), MatchmakingResultToString(notification->getMatchmakingResult()));
        mDispatcher.dispatch("onRemoteMatchmakingEnded", &GameManagerAPIListener::onRemoteMatchmakingEnded, notification, userIndex);
    }

    void GameManagerAPI::onRemoteJoinFailed(const NotifyRemoteJoinFailed *notification, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GMGR] Failed to join game(%" PRIu64 "), with join initiated by player id(%" PRIu64 ") due to error '%s'.\n", notification->getGameId(), 
            notification->getRemoteUserInfo().getInitiatorId(), getBlazeHub()->getErrorName(notification->getJoinError()));
        mDispatcher.dispatch("onRemoteJoinFailed", &GameManagerAPIListener::onRemoteJoinFailed, notification, userIndex);
    }

    void GameManagerAPI::onNotifyHostedConnectivityAvailable(const NotifyHostedConnectivityAvailable* notification, uint32_t userIndex)
    {
        Game *game = getGameById( notification->getGameId() );
        if (game != nullptr && game->isNotificationHandlingUserIndex(userIndex))
        {
            game->onNotifyHostedConnectivityAvailable(*notification, userIndex);
        }
        else
        {
            notifyDropWarn("NotifyHostedConnectivityAvailable", userIndex, game, notification->getGameId());
        }
    }

#if defined(EA_PLATFORM_PS4)
    /*! ****************************************************************************/
    /*! \brief Deprecated - just compare externalId instead
    ********************************************************************************/
    bool GameManagerAPI::areSonyIdsEqual(const PrimarySonyId& id1, const PrimarySonyId& id2) const
    {
        //special handling since SceNpOnlineId's .data field is not always terminated within its char array, but instead in the next .term character field
        return (ds_strnicmp(id1.data, id2.data, SCE_NP_ONLINEID_MAX_LENGTH + 1) == 0);
    }
#endif


    /*! ************************************************************************************************/
    /*! \brief Update user's game presence. Decide which game (if any) should be designated as the local user's primary game
        (displayed as joinable in first party UX etc), and set or clear the user's primary game as needed.

        \param[in] userIndex the user's player object who joined or is leaving a game.
        \param[in] changedGame the game the player joined or is leaving. Pre: if joined, game already in mGameMap.
        \param[in] job if not nullptr, the game job to dispatch, after the update completes
    ***************************************************************************************************/
    void GameManagerAPI::updateGamePresenceForLocalUser(uint32_t userIndex, const Game& changedGame, UpdateExternalSessionPresenceForUserReason change, GameManagerApiJob* job, PlayerRemovedReason removeReason /*= SYS_PLAYER_REMOVE_REASON_INVALID*/)
    {
        // double check the user is in fact local. Dedicated servers never become the primary game.
        // Also, guests by spec never update the primary game and must await non-guests to do this.
        const Player* localPlayer = changedGame.getLocalPlayer(userIndex);
        if ((localPlayer == nullptr) || changedGame.isDedicatedServerHost() || EA_UNLIKELY(getBlazeHub() == nullptr || localPlayer->getUser() == nullptr) ||
            localPlayer->getUser()->isGuestUser())
        {
            dispatchSetupCallbacksOnGamePresenceUpdated(((job != nullptr) ? job->getId() : INVALID_JOB_ID), INVALID_GAME_ID, INVALID_BLAZE_ID);
            return;
        }
        BlazeId blazeId = localPlayer->getId();
        
        UpdateExternalSessionPresenceForUserRequest request;
        localPlayer->getUser()->copyIntoUserIdentification(request.getUserIdentification());
        request.setChange(change);
        populateGameActivity(request.getChangedGame(), changedGame, localPlayer, "");
        request.setOldPrimaryGameId(getActivePresenceGameId());
        request.setRemoveReason(removeReason);
#if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
        char8_t strPushCtx[SCE_NP_WEBAPI2_PUSH_EVENT_UUID_LENGTH + 1];
        memset(strPushCtx, 0, SCE_NP_WEBAPI2_PUSH_EVENT_UUID_LENGTH + 1);
        NetConnStatus('pscx', 0, strPushCtx, sizeof(strPushCtx));
        request.setPsnPushContextId(strPushCtx);
#elif defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        request.setPsnPushContextId(eastl::string(eastl::string::CtorSprintf(), "mock%" PRIi64 ":%p", TimeValue::getTimeOfDay().getMicroSeconds(), &changedGame).c_str());
#endif

        // send list of user's current games to pick the primary game from.
        for (GameMap::const_iterator itr = mGameMap.begin(), end = mGameMap.end(); itr != end; ++itr)
        {
            // skip left game if its still in mGameMap. check local user is in game also. 
            const Player* p = itr->second->getPlayerById(blazeId);//pre: already checked local above
            if (((change == UPDATE_PRESENCE_REASON_GAME_LEAVE) && (changedGame.getId() == itr->second->getId())) || (p == nullptr))
                continue;
            populateGameActivity(*request.getCurrentGames().pull_back(), *itr->second, p, "");
        }

        BLAZE_SDK_DEBUGF("[GMGR] Determining which of %" PRIsize " games (if any) to set as primary game for user(%" PRIu64 ") on %s of game(%" PRIu64 ", gameType=%s).\n",
            request.getCurrentGames().size(), blazeId, UpdateExternalSessionPresenceForUserReasonToString(change), changedGame.getId(), GameTypeToString(changedGame.getGameType()));

        // use the MLU user's game manager component
        GameManagerComponent* gmComp = ((getBlazeHub()->getComponentManager(userIndex) == nullptr)? nullptr : getBlazeHub()->getComponentManager(userIndex)->getGameManagerComponent());
        if (EA_UNLIKELY(gmComp == nullptr))
        {
            BLAZE_SDK_DEBUGF("[GMGR] Internal error: target local user (index=%u) game manager component missing. Attempting to use primary local user (index=%u) to do update of primary game for the target user(%" PRIu64 ").\n", userIndex, getBlazeHub()->getPrimaryLocalUserIndex(), blazeId);
            gmComp = mGameManagerComponent;
        }

        // Pre: user is logged into one SDK client, so the client is the central authority on the games the user is in at any time.
        // The server has the logic for deciding which game (if any) should be designated as the primary game. Send the games and data to server.
        GameId joinCompleteGameId = ((change == UPDATE_PRESENCE_REASON_GAME_JOIN) ? changedGame.getId() : INVALID_GAME_ID);
        gmComp->updatePrimaryExternalSessionForUser(request, MakeFunctor(this, &GameManagerAPI::internalUpdateGamePresenceForUserCb), ((job != nullptr) ? job->getId() : INVALID_JOB_ID), joinCompleteGameId, blazeId);
    }

    void GameManagerAPI::internalUpdateGamePresenceForUserCb(const UpdateExternalSessionPresenceForUserResponse *response,
        const UpdateExternalSessionPresenceForUserErrorInfo *errInfo, BlazeError error, JobId rpcJobId, JobId jobId, GameId joinCompleteGameId, BlazeId blazeId)
    {
    #if defined(ARSON_BLAZE)
        mLastPresenceSetupError = error;
    #endif
        // 1st update local ExternalSessionIdentification as needed. note this may or not be the primary now.
        const UpdatedExternalSessionForUserResult* updated = nullptr;
        if (response != nullptr && !response->getUpdated().getActivity().empty())
        {
            updated = &response->getUpdated();
        }
        else if (errInfo != nullptr && !errInfo->getUpdated().getActivity().empty())
        {
            updated = &errInfo->getUpdated();
        }
        if (updated != nullptr)
        {
            Game* game = getGameById(updated->getGameId());
            if (game != nullptr)
                updated->getSession().copyInto(game->mExternalSessionIdentification);
        }

       
        // 2nd handle dispatching client notifications
        if ((response == nullptr) || (error != ERR_OK))
        {
            // note: we won't treat failure as catastrophic, just log it, and dispatch the join completing callbacks to title etc
            BLAZE_SDK_DEBUGF("[GMGR] update of primary game for user(%" PRIu64 ") result: %s(%d).\n", blazeId, getBlazeHub()->getErrorName(error), error);
            ExternalSessionIdentification emptySessId;//empty ensures cleanup
            const char8_t* pushContextId = (errInfo ? errInfo->getCurrentPrimary().getPsnPushContextId() : nullptr);
            onActivePresenceUpdated(((errInfo != nullptr) ? errInfo->getCurrentPrimary().getGameId() : INVALID_GAME_ID), (errInfo ? errInfo->getCurrentPrimary().getSession() : emptySessId), pushContextId, error, getLocalUserIndex(blazeId));
            dispatchSetupCallbacksOnGamePresenceUpdated(jobId, joinCompleteGameId, blazeId);
            return;
        }
        if (response->getBlazeId() == INVALID_BLAZE_ID)
        {
            dispatchSetupCallbacksOnGamePresenceUpdated(jobId, joinCompleteGameId, blazeId);
            return;//disabled
        }
        BLAZE_SDK_DEBUGF("[GMGR] update of primary game for user(%" PRIu64 ") result: %s is set as the current primary game.\n",
            blazeId, ((response->getCurrentPrimary().getGameId() == INVALID_GAME_ID) ? "NO game" : eastl::string().sprintf("game(%" PRIu64 ")", response->getCurrentPrimary().getGameId()).c_str()));

        // For usability, before dispatching create/join completion callbacks below, update local game's NpSessionId
        onActivePresenceUpdated(response->getCurrentPrimary().getGameId(), response->getCurrentPrimary().getSession(), response->getCurrentPrimary().getPsnPushContextId(), error, getLocalUserIndex(blazeId));

        dispatchSetupCallbacksOnGamePresenceUpdated(jobId, joinCompleteGameId, blazeId);
    }

    /*! ************************************************************************************************/
    /*! \brief dispatch pending create/join completion callbacks to title
        \param[in] joinCompleteGameId If valid, dispatch onPlayerJoinComplete for the player
    ***************************************************************************************************/
    void GameManagerAPI::dispatchSetupCallbacksOnGamePresenceUpdated(const JobId& jobId, GameId joinCompleteGameId, BlazeId blazeId)
    {
        GameManagerApiJob* job = (jobId != INVALID_JOB_ID ? (GameManagerApiJob*)getBlazeHub()->getScheduler()->getJob(jobId) : nullptr);
        if ((job != nullptr) && job->isMatchmakingConnectionValidated())
        {
            job->execute();
            getBlazeHub()->getScheduler()->removeJob(job);
        }
        Game* joinedGame = (joinCompleteGameId != INVALID_GAME_ID ? getGameById(joinCompleteGameId) : nullptr);
        if ((joinedGame != nullptr) && (joinedGame->getPlayerById(blazeId) != nullptr))
        {
            joinedGame->mDispatcher.dispatch(&GameListener::onPlayerJoinComplete, joinedGame->getPlayerById(blazeId));
        }
    }

    /*! ************************************************************************************************/
    /*! \brief when the game advertised as the local user's primary activity changed, notify DirtySDK and local listeners.
        \param[in] presenceGameId The new advertised game's id, or INVALID_GAME_ID if user has none advertised
        \param[in] presenceSession The new advertised game's ext session. Empty if user has none advertised
    ***************************************************************************************************/
    void GameManagerAPI::onActivePresenceUpdated(GameId presenceGameId, const ExternalSessionIdentification& presenceSession, const char8_t* psnPushContextId,
        BlazeError error, uint32_t userIndex)
    {
        // Only the primary local user is supported for notifications below. Can skip if old game id is up to date
        if ((getBlazeHub()->getPrimaryLocalUserIndex() != userIndex) || (getActivePresenceGameId() == presenceGameId))
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] %slocal user's primary game for presence does not need to be updated. Skipping dispatches.\n", userIndex, (getBlazeHub()->getPrimaryLocalUserIndex() == userIndex ? "primary " : ""));
            return;
        }
        // update and notify about clearing old game 1st
        onActivePresenceCleared(false, userIndex);
        
        // check if need to notify about a new one
        Game* game = (presenceGameId != INVALID_GAME_ID ? getGameById(presenceGameId) : nullptr);
        if (game == nullptr)
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] local user's primary game for presence has%s no updates needed.\n", userIndex, (presenceGameId != INVALID_GAME_ID ? "been already left, " : ""));
            return;
        }
        BLAZE_SDK_DEBUGF("[GMGR] UI[%u] local user's primary game for presence set to game(%" PRIu64 ", external session(%s %s %s)), with server return code(%i).\n", userIndex, presenceGameId, presenceSession.getXone().getSessionName(), presenceSession.getPs4().getNpSessionId(), presenceSession.getPs5().getPlayerSession().getPlayerSessionId(), error);
        mActivePresenceGameId = presenceGameId;

        // platforms with only a single primary 1st party session may only have one added now. Ensure updated
    #if defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        game->mExternalSessionIdentification.getPs5().getPlayerSession().setPlayerSessionId(presenceSession.getPs5().getPlayerSession().getPlayerSessionId());
        if (psnPushContextId != nullptr)
        {
            game->mPsnPushContextId.set(psnPushContextId);
        }
    #endif
    #if (defined(EA_PLATFORM_PS4) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)) && !defined(EA_PLATFORM_PS4_CROSSGEN)
        game->mExternalSessionIdentification.getPs4().setNpSessionId(presenceSession.getPs4().getNpSessionId());

        // activate DirtySDK session
        getNetworkAdapter()->setActiveSession(game);
        game->mDispatcher.dispatch("onNpSessionIdAvailable", &GameListener::onNpSessionIdAvailable, game, presenceSession.getPs4().getNpSessionId());//deprecated
    #endif
        game->mDispatcher.dispatch("onActivePresenceUpdated", &GameListener::onActivePresenceUpdated, game, (error == ERR_OK ? PRESENCE_ACTIVE : PRESENCE_INACTIVE), userIndex);
    }


    void GameManagerAPI::onActivePresenceCleared(bool isLocalGameDying, uint32_t userIndex)
    {
        // Only the primary local user is supported for notifications below
        if ((getBlazeHub()->getPrimaryLocalUserIndex() != userIndex) || (getActivePresenceGameId() == INVALID_GAME_ID))
        {
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] %slocal user's primary game for presence does not need to be cleared. Skipping dispatches.\n", userIndex, (getBlazeHub()->getPrimaryLocalUserIndex() == userIndex ? "primary " : ""));
            return;
        }
        BLAZE_SDK_DEBUGF("[GMGR] UI[%u] local user's primary game for presence is no longer game(%" PRIu64 ").\n", userIndex, getActivePresenceGameId());

        // ensure cleanup DirtySDK session
        getNetworkAdapter()->clearActiveSession();

        Game* oldGame = getGameById(getActivePresenceGameId());
        mActivePresenceGameId = INVALID_GAME_ID;
        if ((oldGame == nullptr) || isLocalGameDying)
        {
            // No need to dispatch GameListener below if game already dying. Also prevents crashes in title code if it destroyed their GameListener, but forgot to call removeListener here
            BLAZE_SDK_DEBUGF("[GMGR] UI[%u] %s old primary game for presence.\n", userIndex, (isLocalGameDying ? "Skipping GameListener notifications for dying local" : "Warning: dropping clear of unknown"));
            return;
        }

        // On PS4 and PS5, user is only in its primary game's NP/Player session (see DevNet ticket 58807). To avoid misuse (and match old behavior), clear local NP/Player session ids on non-primary games
    #if defined(EA_PLATFORM_PS4) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        oldGame->mExternalSessionIdentification.getPs4().setNpSessionId("");
    #endif
    #if defined(EA_PLATFORM_PS5) || defined(EA_PLATFORM_PS4_CROSSGEN) || defined(BLAZE_ENABLE_MOCK_EXTERNAL_SESSIONS)
        oldGame->mExternalSessionIdentification.getPs5().getPlayerSession().setPlayerSessionId("");
        if (!oldGame->mPsnPushContextId.empty())
        {
            #if defined(EA_PLATFORM_PS5) || (defined(EA_PLATFORM_PS4_CROSSGEN) && defined(EA_PLATFORM_PS4))
            NetConnStatus('pscd', 0, (void*)oldGame->mPsnPushContextId.c_str(), oldGame->mPsnPushContextId.length());
            #endif
            oldGame->mPsnPushContextId.set("");
        }
    #endif
        oldGame->mDispatcher.dispatch("onActivePresenceUpdated", &GameListener::onActivePresenceUpdated, oldGame, PRESENCE_INACTIVE, userIndex);
    }



    /*! ************************************************************************************************/
    /*! \brief validates game settings
    ***************************************************************************************************/
    void GameManagerAPI::validateGameSettings(const GameSettings& gameSettings) const
    {
#ifdef EA_PLATFORM_XBOXONE
        // Xbox One XR-124 states all games joinable via the UX should be open to invites
        if (gameSettings.getOpenToJoinByPlayer() && !gameSettings.getOpenToInvites())
        {
            BLAZE_SDK_DEBUG("[GMGR] Warning: setting openToJoinByPlayer but clearing openToInvites, possible cert violating settings on Xbox One. Blaze may automatically disable game's join-ability via first party UX to avoid cert violation.\n");
        }
        // Xbox One XR-064 expects games un-joinable via the UX to be closed to browse typically (can depend on browser implementation)
        if (!gameSettings.getOpenToJoinByPlayer() && gameSettings.getOpenToBrowsing())
        {
            BLAZE_SDK_DEBUG("[GMGR] Warning: clearing openToJoinByPlayer but setting openToBrowsing, possible cert violating settings on Xbox One.\n");
        }
#else
        // friend join restrictions are only checked for Xbox currently. Warn that on other platforms, it allows bypassing openToJoinByPlayer when using JOIN_BY_PLAYER.
        if (gameSettings.getFriendsBypassClosedToJoinByPlayer())
        {
            BLAZE_SDK_DEBUG("[GMGR] Warning: setting friendsBypassClosedToJoinByPlayer for the current platform will allow all joins by player to bypass closed to join by player.\n");
        }
#endif
    }

    /*! ************************************************************************************************/
    /*! \brief return whether there are any local users with an open job for the matchmaking session.
    ***************************************************************************************************/
    bool GameManagerAPI::matchmakingScenarioHasLocalUser(const MatchmakingScenario& mmSession, GameId gameId) const
    {
        for (uint32_t userIndex = 0; userIndex < getBlazeHub()->getNumUsers(); ++userIndex)
        {
            const GameManagerApiJob* gmJob = const_cast<GameManagerAPI&>(*this).getJob(userIndex, gameId);
            if (gmJob == nullptr)
                continue;

            if (((gmJob->getSetupReason().getIndirectMatchmakingSetupContext() != nullptr) && (gmJob->getSetupReason().getIndirectMatchmakingSetupContext()->getScenarioId() == mmSession.getScenarioId())) ||
                ((gmJob->getSetupReason().getMatchmakingSetupContext() != nullptr) && (gmJob->getSetupReason().getMatchmakingSetupContext()->getScenarioId() == mmSession.getScenarioId())))
            {
                return true;
            }
        }
        return false;
    }

    uint32_t GameManagerAPI::getLocalUserIndex(BlazeId blazeId) const
    {
        const UserManager::LocalUser* user = (getUserManager() != nullptr ? getUserManager()->getLocalUserById(blazeId) : nullptr);
        return (user == nullptr) ? INVALID_USER_INDEX : user->getUserIndex();
    }

    /*! ************************************************************************************************/
    /*! \brief return whether there are local game groups.
        \param[in] ignoreGameGroupId if this is for a game group, ignore counting this.
    ***************************************************************************************************/
    bool GameManagerAPI::hasLocalGameGroups(GameId ignoreGameGroupId) const
    { 
        GameMap::const_iterator iter = mGameMap.begin();
        for (; iter != mGameMap.end(); ++iter)
        {
            Game* game = iter->second;
            if ((game->getGameType() == GAME_TYPE_GROUP) && (game->getId() != ignoreGameGroupId))
            {
                return true;
            }
        }
        return false;
    }

    void GameManagerAPIListener::onTopologyHostInjectionStarted(Game *game)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default handler for host injection notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());
    }

    void GameManagerAPIListener::onTopologyHostInjected(Game *game)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default handler for host injection notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", 
            game->getId());
    }

    void GameManagerAPIListener::onIndirectJoinFailed(Game *game, uint32_t userIndex, Blaze::BlazeError blazeError)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default handler for indirect join fail notification for game(%" PRIu64 "), userIndex(%u), error(%d).  Override this listener method to provide a specific implementation.\n",
            game->getId(), userIndex, blazeError);
    }

    void GameManagerAPIListener::onReservedPlayerIdentifications(Blaze::GameManager::Game *game, uint32_t userIndex, const Blaze::UserIdentificationList *reservedPlayerIdentifications)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default handler for reserved external player identities notification for game(%" PRIu64 "), userIndex(%u), number reserved(%" PRIsize ").  Override this listener method to provide a specific implementation.\n",
            (game != nullptr? game->getId() : GameManager::INVALID_GAME_ID), userIndex, (reservedPlayerIdentifications != nullptr ? (size_t)reservedPlayerIdentifications->size() : 0));
    }

    void GameManagerAPIListener::onGameLockedForPreferredJoins(Blaze::GameManager::Game *game, Blaze::BlazeId playerId, Blaze::ExternalId playerExternalId)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default handler for locked for preferred joins notification for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n",
            (game != nullptr? game->getId() : GameManager::INVALID_GAME_ID));

        // By default we opt the local player out of preferred joins for the game. 
        // Implementers should similarly call preferredJoinOptOut to opt out, or call joinGameByUserList to try adding members.
        if (game != nullptr)
        {            
            Blaze::GameManager::Game::PreferredJoinOptOutJobCb cbFunctor(this, &GameManagerAPIListener::defaultPreferredJoinOptOutTitleCb);
            game->preferredJoinOptOut(cbFunctor);
        }
    }

    void GameManagerAPIListener::defaultPreferredJoinOptOutTitleCb(Blaze::BlazeError error, Blaze::GameManager::Game *game, JobId id)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default callback for preferred join opt out for game(%" PRIu64 "), error code(%d).\n", ((game != nullptr)? game->getId() : GameManager::INVALID_GAME_ID), error);
    }

    void GameManagerAPIListener::onGameCreated(Blaze::GameManager::Game *createdGame)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default callback for game created for game(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", createdGame->getId());
    }

    void GameManagerAPIListener::onRemoteMatchmakingStarted(const Blaze::GameManager::NotifyRemoteMatchmakingStarted *matchmakingStartedNotification, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default callback for remote matchmaking started for sessionId(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", matchmakingStartedNotification->getMMSessionId());
    }

    void GameManagerAPIListener::onRemoteMatchmakingEnded(const Blaze::GameManager::NotifyRemoteMatchmakingEnded *matchmakingEndedNotification, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default callback for remote matchmaking ended for sessionId(%" PRIu64 ").  Override this listener method to provide a specific implementation.\n", matchmakingEndedNotification->getMMSessionId());
    }

    void GameManagerAPIListener::onRemoteJoinFailed(const Blaze::GameManager::NotifyRemoteJoinFailed *remoteJoinFailedNotification, uint32_t userIndex)
    {
        BLAZE_SDK_DEBUGF("[GameManagerAPIListener] Default callback for remote join to game(%" PRIu64 ") failed.  Override this listener method to provide a specific implementation.\n", remoteJoinFailedNotification->getGameId());
    }

} // GameManager
} //Blaze
