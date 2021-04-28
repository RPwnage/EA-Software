/*! ************************************************************************************************/
/*!
    \file gamesessionmaster.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "framework/util/uuid.h"
#include "framework/util/random.h"
#include "framework/connection/selector.h"
#include "framework/event/eventmanager.h"
#include "framework/connection/session.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/outboundhttpconnectionmanager.h"
#include "framework/redis/redislockmanager.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/gamenetworkbehavior.h"
#include "gamemanager/playerinfo.h"
#include "gamemanager/playerroster.h"
#include "gamemanager/externalsessions/externalsessionscommoninfo.h"
#include "gamemanager/gamemanagermetrics.h"
#include "gamemanager/gamemanagervalidationutils.h"
#include "gamemanager/gamemanagerhelperutils.h"
#include "gamemanager/gamemanagermasterimpl.h"

#include "gamemanager/tdf/gamemanagerevents_server.h"
#include "gamemanager/tdf/gamemanager.h"

#ifdef TARGET_gamereporting
#include "component/gamereporting/rpc/gamereportingslave.h"
#include "gamereporting/tdf/gamereporting_server.h"
#endif

#include "EAStdC/EAHashCRC.h"
#include "EASTL/sort.h"
#include "EASTL/algorithm.h"

#include "framework/tdf/riverposter.h" // for mp_match_info status code string consts

namespace Blaze
{
namespace GameManager
{

    extern const char8_t PUBLIC_GAME_FIELD[];
    extern const char8_t PRIVATE_GAME_FIELD[];

    void intrusive_ptr_add_ref(GameSessionMaster* ptr)
    {
        if (ptr->mRefCount == 1)
        {
            // pin the sliver from migrating because we 'may' be writing the game object back
            bool result = ptr->mOwnedSliverRef->getPrioritySliverAccess(ptr->mOwnedSliverAccessRef);
            EA_ASSERT_MSG(result, "Game Session failed to obtain owned sliver access!");
        }
        ++ptr->mRefCount;
    }

    void intrusive_ptr_release(GameSessionMaster* ptr)
    {
        if (ptr->mRefCount > 0)
        {
            --ptr->mRefCount;
            if (ptr->mRefCount == 0)
                delete ptr;
            else if (ptr->mRefCount == 1)
            {
                if (ptr->mAutoCommit)
                {
                    const GameId gameId = ptr->getGameId();
                    const GameSessionMaster* game = gGameManagerMaster->getReadOnlyGameSession(gameId);
                    if (game == ptr)
                    {
                        // This means we are returning the game back to the container, trigger a writeback to storage
                        gGameManagerMaster->getGameStorageTable().upsertField(gameId, PRIVATE_GAME_FIELD, *ptr->mGameDataMaster);
                        gGameManagerMaster->getGameStorageTable().upsertField(gameId, PUBLIC_GAME_FIELD, *ptr->mGameData);
                        ptr->mPlayerRoster.upsertPlayerFields();
                    }
                    ptr->mAutoCommit = false;
                }
                // unpin the sliver and enable it to migrate once more (once the pending upserts have been flushed)
                ptr->mOwnedSliverAccessRef.release();
            }
        }
    }

    GameSessionMaster::EndpointSlotBitset& GameSessionMaster::asEndpointSlotBitset(GameDataMaster::EndpointSlotList& slotList)
    {
        EA_ASSERT(sizeof(EndpointSlotBitset) == slotList.size()*sizeof(GameDataMaster::EndpointSlotList::value_type));
        // this cast is safe because EndpointSlotBitset is internally a simple array of uint64_t
        return *reinterpret_cast<EndpointSlotBitset*>(slotList.begin());
    }

    const GameSessionMaster::EndpointSlotBitset& GameSessionMaster::asEndpointSlotBitset(const GameDataMaster::EndpointSlotList& slotList)
    {
        EA_ASSERT(sizeof(EndpointSlotBitset) == slotList.size()*sizeof(GameDataMaster::EndpointSlotList::value_type));
        // this cast is safe because EndpointSlotBitset is internally a simple array of uint64_t
        return *reinterpret_cast<const EndpointSlotBitset*>(slotList.begin());
    }

    bool GameSessionMaster::isSlotOccupied(SlotId slot) const 
    {
        // test(slot) == 0 means slot is occupied
        return !asEndpointSlotBitset(mGameDataMaster->getOpenSlotIdList()).test(slot);
    }

    /*! ************************************************************************************************/
    /*! \brief create an uninitialized Game object.  You must call initialize before using the game.
    ***************************************************************************************************/
    GameSessionMaster::GameSessionMaster(GameId gameId, OwnedSliverRef& ownedSliverRef)
        : GameSession(gameId),
        mAutoCommit(false),
        mRefCount(0),
        mOwnedSliverRef(ownedSliverRef),
        mGameDataMaster(BLAZE_NEW GameDataMaster),
        mGameNetwork(nullptr), 
        mMatchEndTime(0),
        mLastGameUpdateEventHash(EA::StdC::kCRC32InitialValue)
    {
        mPlayerRoster.setOwningGameSession(*this);
        mPlayerConnectionMesh.setGameSessionMaster(*this);
        mMetricsHandler.setGameSessionMaster(*this);
        getGameData().setCreateTime(TimeValue::getTimeOfDay());
    }

    // This ctor is used to re-create the GameSessionMaster when rebuilding replicated data from Redis
    GameSessionMaster::GameSessionMaster(ReplicatedGameDataServer& gameSession, GameDataMaster& gameDataMaster, GameNetworkBehavior& gameNetwork, OwnedSliverRef& ownedSliverRef)
        : GameSession(gameSession),
        mAutoCommit(false),
        mRefCount(0),
        mOwnedSliverRef(ownedSliverRef),
        mGameDataMaster(&gameDataMaster),
        mGameNetwork(&gameNetwork), 
        mMatchEndTime(0),
        mLastGameUpdateEventHash(EA::StdC::kCRC32InitialValue)
    {
        mPlayerRoster.setOwningGameSession(*this);
        mPlayerConnectionMesh.setGameSessionMaster(*this);
        mMetricsHandler.setGameSessionMaster(*this);
        refreshDynamicPlatformList();
    }

    //! \brief destructor
    GameSessionMaster::~GameSessionMaster()
    {
    }

    void GameSessionMaster::cleanUpMatchmakingQosDataMap()
    {
        const PlayerNetConnectionStatus status = DISCONNECTED;
        PlayerIdList tempPlayerIdList;
        GameDataMaster::MatchmakingSessionMap::const_iterator mapIter = mGameDataMaster->getMatchmakingSessionMap().begin();
        for (; mapIter != mGameDataMaster->getMatchmakingSessionMap().end(); ++mapIter)
        {
            tempPlayerIdList.push_back(mapIter->first);
        }

        PlayerIdList::const_iterator playerIter = tempPlayerIdList.begin();
        for (; playerIter != tempPlayerIdList.end(); ++playerIter)
        {
            // this will clear out the list one item at a time and notify MM that the users didn't connect
            PlayerInfoMaster* playerInfo = getPlayerRoster()->getPlayer(*playerIter);
            ConnectionGroupId connectionGroupId = playerInfo != nullptr ? playerInfo->getConnectionGroupId() : INVALID_CONNECTION_GROUP_ID;
            updateMatchmakingQosData(*playerIter, &status, connectionGroupId);
        }
    }


    /*! ************************************************************************************************/
    /*! \brief init this game's data from the supplied createGameRequest, inserts the game
            into the master's replicated game map and initializes the game's roster.

        \param[in] createGameRequest the createGameRequest to init this game's common data from.
        \param[in] hostSession the user session who activates the game creation
        \param[out] joinedExternalUserInfos adds platform host's external user info to this list
        \return ERR_OK on success, or a variety of validation failure codes.
    ***************************************************************************************************/
    CreateGameMasterError::Error GameSessionMaster::initialize( const CreateGameMasterRequest& masterRequest, const UserSessionSetupReasonMap &setupReasons,
        const PersistedGameId& persistedId, const PersistedGameIdSecret& persistedIdSecret, JoinExternalSessionParameters &params)
    {
        const CreateGameRequest& createGameRequest = masterRequest.getCreateRequest();

        bool isVirtualized = createGameRequest.getGameCreationData().getGameSettings().getVirtualized();
        bool isVirtualWithReservation = (isVirtualized && createGameRequest.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION);
        
        getGameData().setPersistedGameId(persistedId);
        getGameData().getPersistedGameIdSecret().setData(persistedIdSecret.getData(), persistedIdSecret.getCount());
        BlazeRpcError blazeRpcError = ERR_OK;

        // Accounts for pg members and reserved slots.
        SlotTypeSizeMap slotTypeSizeMap;
        ValidationUtils::validatePlayersInCreateRequest(masterRequest, blazeRpcError, slotTypeSizeMap, isVirtualWithReservation);
        if (EA_UNLIKELY(blazeRpcError != ERR_OK))
        {
            return CreateGameMasterError::commandErrorFromBlazeError( blazeRpcError );
        }

        // Check that the requested slot type has a valid spot in the game for all joining immediately.
        blazeRpcError = ValidationUtils::validateGameCapacity(slotTypeSizeMap, createGameRequest);
        if (EA_UNLIKELY(blazeRpcError != ERR_OK))
        {
            return CreateGameMasterError::commandErrorFromBlazeError( blazeRpcError );
        }

        // Check teams, note that we do not account for reserved slots in the team capacity checks.
        // I'm allowed to reserve slots for players that may join another team, this is also done by Matchmaking.
        blazeRpcError = ValidationUtils::validateTeamCapacity(slotTypeSizeMap, createGameRequest, isVirtualWithReservation);
        if (EA_UNLIKELY(blazeRpcError != ERR_OK))
        {
            return CreateGameMasterError::commandErrorFromBlazeError( blazeRpcError );
        }

        // Verify that the role capacities are large enough for the group.
        blazeRpcError = ValidationUtils::validatePlayerRolesForCreateGame(masterRequest);
        if (EA_UNLIKELY(blazeRpcError != Blaze::ERR_OK))
        {
            return CreateGameMasterError::commandErrorFromBlazeError( blazeRpcError );
        }

        // set immutable replicated data (protocol version & game network behavior & topology)

        // Immutable: game protocol version & network topology
        getGameData().setNetworkTopology(createGameRequest.getGameCreationData().getNetworkTopology());
        mGameDataMaster->setInitialGameProtocolVersionString(createGameRequest.getCommonGameData().getGameProtocolVersionString());

        // Immutable: the gameNetwork topology behavior obj
        mGameNetwork = gGameManagerMaster->getGameNetworkBehavior( getGameNetworkTopology() );
        if (EA_UNLIKELY(mGameNetwork == nullptr))
        {
            ERR_LOG("[GameSessionMaster] Initializing game(" << getGameId() << ") with an unknown network topology enumeration value("
                    << getGameNetworkTopology() << ").");
            return CreateGameMasterError::ERR_SYSTEM;
        }

        // init creation-specific values, then common values shared between init & reset game
        getGameData().setGameState(NEW_STATE);
        getGameData().setGameName(createGameRequest.getGameCreationData().getGameName());
        getGameData().setGameReportName(createGameRequest.getGameCreationData().getGameReportName());
        getGameData().setGameStatusUrl(createGameRequest.getGameStatusUrl());
        getGameData().setGameEventAddress(createGameRequest.getGameEventAddress());
        getGameData().setGameStartEventUri(createGameRequest.getGameStartEventUri());
        getGameData().setGameEndEventUri(createGameRequest.getGameEndEventUri());
        createGameRequest.getTournamentIdentification().copyInto(getGameData().getTournamentIdentification());
        getGameData().setGameType(createGameRequest.getCommonGameData().getGameType());
        
        // we have to init the host info on the game here (even though we call initializeGameHost later),
        //  since we need the host info to be correct for our initial replication to the slave (and we don't
        //  call initializeGameHost until after the game's been inserted into the replication map)
        const UserSessionInfo* hostSessionInfo = getHostSessionInfo(masterRequest.getUsersInfo());
        bool isHostless = (isVirtualized || createGameRequest.getGameCreationData().getIsExternalOwner()
            || ((getGameNetworkTopology() == NETWORK_DISABLED) && !gGameManagerMaster->getConfig().getGameSession().getAssignTopologyHostForNetworklessGameSessions()));

        if (hostSessionInfo == nullptr && !isHostless)
        {
            ERR_LOG("[GameSessionMaster] Missing host for game(" << getGameId() << ") that cannot be hostless. Creation failed. ");
            return CreateGameMasterError::ERR_SYSTEM;
        }

        if ((createGameRequest.getCommonGameData().getPlayerNetworkAddress().getActiveMember() == NetworkAddress::MEMBER_UNSET) &&
            !isHostless && !isPseudoGame())
        {
            ERR_LOG("[GameSessionMaster] Initializing game for host(" << hostSessionInfo->getUserInfo().getId() << ") and game(" << getGameId() 
                    << ") failed, player network address was not set.");
            return CreateGameMasterError::ERR_SYSTEM;
        }

        if (!isHostless && !isPseudoGame()) 
        {
            if (createGameRequest.getCommonGameData().getPlayerNetworkAddress().getActiveMember() != NetworkAddress::MEMBER_UNSET)
            {
                createGameRequest.getCommonGameData().getPlayerNetworkAddress().copyInto(*getGameData().getTopologyHostNetworkAddressList().pull_back());
            }

            if (isDedicatedHostedTopology(getGameNetworkTopology()))
            {
                getGameData().getTopologyHostNetworkAddressList().copyInto(getGameData().getDedicatedServerHostNetworkAddressList());
            }

            getGameData().getTopologyHostInfo().setUserSessionId(hostSessionInfo->getSessionId());
            getGameData().getTopologyHostInfo().setPlayerId(hostSessionInfo->getUserInfo().getId());
            getGameData().getTopologyHostInfo().setConnectionGroupId(hostSessionInfo->getConnectionGroupId());

            if (isDedicatedHostedTopology(getGameNetworkTopology()) && !isHostless)
            {
                getGameData().getTopologyHostInfo().copyInto(getGameData().getDedicatedServerHostInfo());
            }

        }
        // similarly set external owner before any potential hasExternalOwner() calls
        if (createGameRequest.getGameCreationData().getIsExternalOwner())
        {
            getGameData().getExternalOwnerInfo().setUserSessionId(masterRequest.getExternalOwnerInfo().getSessionId());
            getGameData().getExternalOwnerInfo().setPlayerId(masterRequest.getExternalOwnerInfo().getUserInfo().getId());
            getGameData().getExternalOwnerInfo().setConnectionGroupId(masterRequest.getExternalOwnerInfo().getConnectionGroupId());
            masterRequest.getExternalOwnerInfo().copyInto(mGameDataMaster->getCreatorHostInfo());//just for debugging
            blazeRpcError = ValidationUtils::validateExternalOwnerParamsInReq(createGameRequest, masterRequest.getExternalOwnerInfo());
            if (blazeRpcError != ERR_OK)
            {
                return CreateGameMasterError::commandErrorFromBlazeError(blazeRpcError);
            }
        }
       
       
        getGameData().setServerNotResetable(createGameRequest.getServerNotResetable());

        const char8_t* pingSiteAlias = createGameRequest.getGamePingSiteAlias();
        // if the game specified a pingSiteAlias (data center), validate it (warning only), override if necessary
        if (!gUserSessionManager->getQosConfig().isKnownPingSiteAlias(pingSiteAlias))
        {
            bool setPingSiteFromHost = false;
            if (!isHostless)
            {
                const char8_t* hostSessionBestPingSiteAlias = hostSessionInfo->getBestPingSiteAlias();
                // for non-dedicated server game, override with the host session's pingSiteAlias if it's valid
                if (!isDedicatedHostedTopology(getGameNetworkTopology()) && gUserSessionManager->getQosConfig().isKnownPingSiteAlias(hostSessionBestPingSiteAlias))
                {
                    pingSiteAlias = hostSessionBestPingSiteAlias;
                    setPingSiteFromHost = true;
                }
            }

            if (!setPingSiteFromHost)
            {
                if (isVirtualized)
                {
                    ERR_LOG("[GameSessionMaster] Initializing virtualized game(" << getGameId() << ") with an invalid pingSiteAlias:\"" << pingSiteAlias << "\".  Returning GAMEMANAGER_ERR_INVALID_PING_SITE_ALIAS.");
                    return CreateGameMasterError::GAMEMANAGER_ERR_INVALID_PING_SITE_ALIAS;
                }
                else
                {
                    WARN_LOG("[GameSessionMaster] Initializing a game session(" << getGameId() << ") where the best pingSiteAlias of host was invalid:\"" << pingSiteAlias << "\".");
                }
            }
        }

        // Sanity check to avoid setting a ping site to some uninitialized value:
        if (!gUserSessionManager->getQosConfig().isAcceptablePingSiteAliasName(pingSiteAlias))
        {
            pingSiteAlias = INVALID_PINGSITE;
        }

        getGameData().setPingSiteAlias(pingSiteAlias);

        blazeRpcError = initGameDataCommon(masterRequest);
        if (EA_UNLIKELY(blazeRpcError != ERR_OK))
        {
            WARN_LOG("[GameSessionMaster] Initializing game(" << getGameId() << ") with invalid replicated game data, error("
                     << (ErrorHelp::getErrorName(blazeRpcError)) << ")");
            return CreateGameMasterError::commandErrorFromBlazeError( blazeRpcError );
        }

        //need to set this here so the init replicated update will come in w/ the topology host.
        if (!isHostless)
        {
            // cache off the creator for dirtycast game sessions
            hostSessionInfo->copyInto(mGameDataMaster->getCreatorHostInfo());
            setHostInfo(&mGameDataMaster->getCreatorHostInfo());
        }
        else if (!isVirtualized)
        {
            // everybody is reserved into a NETWORK_DISABLED session, don't set host info until someone claims it
            if (hostSessionInfo)
                hostSessionInfo->copyInto(mGameDataMaster->getCreatorHostInfo());
            setHostInfo(nullptr);
        }

        // Find the host sessions setup reason
        GameSetupReason *hostSetupReason = nullptr;
        if (hostSessionInfo)
        {
            UserSessionSetupReasonMap::const_iterator iter = setupReasons.find(hostSessionInfo->getSessionId());
            if (iter == setupReasons.end())
            {
                // host user session not in the map.
                ERR_LOG("[GameSessionMaster] Create game request did not have an entry in the GameSetupReasonMap for the host user session("
                    << hostSessionInfo->getSessionId() << ")");
                return CreateGameMasterError::ERR_SYSTEM;
            }
            hostSetupReason = iter->second.get();
        }

        // set up QoS validation list
        if (masterRequest.getFinalizingMatchmakingSessionId() != INVALID_MATCHMAKING_SESSION_ID)
        {
            addMatchmakingQosData(masterRequest.getUsersInfo(), masterRequest.getFinalizingMatchmakingSessionId());
        }

        // create and process host player information, can't do that before insert the data
        // into game map and initialize playerRoster since we need to use playerRoster when
        // initializeGameHost.
        if (isVirtualized)
        {
            // Virtual games initialize without a host. and transition directly into the VIRTUAL state.
            blazeRpcError = initializeVirtualGameHost(createGameRequest);
        }
        else if (createGameRequest.getGameCreationData().getIsExternalOwner())
        {
            blazeRpcError = initializeExternalOwner(createGameRequest);
        }
        else
        {
            blazeRpcError = initializeGameHost(masterRequest, *hostSetupReason);
        }

        if (EA_UNLIKELY(blazeRpcError != ERR_OK))
        {
            WARN_LOG("[GameSessionMaster] Failed to create host player(" << (hostSessionInfo ? hostSessionInfo->getUserInfo().getId() : INVALID_BLAZE_ID) << ":"
                     << (hostSessionInfo ? hostSessionInfo->getUserInfo().getPersonaName() : "") << ") for game(" << getGameId() << "), error("
                     << (ErrorHelp::getErrorName(blazeRpcError)) << ").");
            return CreateGameMasterError::commandErrorFromBlazeError( blazeRpcError );
        }

        mGameDataMaster->setGameStatusUrl(createGameRequest.getGameStatusUrl());
        mGameDataMaster->setGameEventAddress(createGameRequest.getGameEventAddress());
        mGameDataMaster->setGameStartEventUri(createGameRequest.getGameStartEventUri());
        mGameDataMaster->setGameEndEventUri(createGameRequest.getGameEndEventUri());
        // For backward compatibility with clients which may not specify an encoding, default to JSON:
        mGameDataMaster->setGameEventContentType((createGameRequest.getGameEventContentType()[0] == '\0' ? JSON_CONTENTTYPE : createGameRequest.getGameEventContentType()));
        createGameRequest.getTournamentIdentification().copyInto(mGameDataMaster->getTournamentIdentification());

        // successfully initialized. add created game to metrics
        mMetricsHandler.onGameCreated();

        // replicate initialized game to slaves, for client/server hosts to get the game information (since they do not join).
        if (isDedicatedHostedTopology(getGameNetworkTopology()) ||
            createGameRequest.getGameCreationData().getIsExternalOwner())
        {
            if (isDedicatedHostedTopology(getGameNetworkTopology()))
            {
                NotifyGameSetup notifyJoinGame;
                GameSetupReason dummySetupReason;
                if (hostSetupReason == nullptr)
                    hostSetupReason = &dummySetupReason;
                // For dedicated server hosts, initNofityGameSetup is called with platform 'common' to ensure that the NotifyGameSetup notification sent to the dedicated server
                // has complete information about the game (i.e. presenceMode is not overridden)
                initNotifyGameSetup(notifyJoinGame, *hostSetupReason, !mGameDataMaster->getMatchmakingSessionMap().empty(), getDedicatedServerHostSessionId(), getDedicatedServerHostInfo().getConnectionGroupId(), common);
                gGameManagerMaster->sendNotifyGameSetupToUserSessionById(getDedicatedServerHostSessionId(), &notifyJoinGame);
            }

            if (createGameRequest.getGameCreationData().getIsExternalOwner())
            {
                NotifyGameSetup notifyJoinGame;
                GameSetupReason extOwnerSetupReason;//unused
                initNotifyGameSetup(notifyJoinGame, extOwnerSetupReason, false, getExternalOwnerSessionId(), getExternalOwnerInfo().getConnectionGroupId());
                gGameManagerMaster->sendNotifyGameSetupToUserSessionById(getExternalOwnerSessionId(), &notifyJoinGame);
            }
        }
        else
        {
            // add external id to response list so slave can add player to external session
            bool makingReservations = createGameRequest.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION;
            if (hostSessionInfo && (gUserSessionManager->getSessionExists(hostSessionInfo->getSessionId()) || makingReservations))
            {
                updateExternalSessionJoinParams(params, *hostSessionInfo, makingReservations, *this);
            }
        }

        if (isDedicatedHostedTopology(getGameNetworkTopology()) && !isVirtualized && hostSessionInfo != nullptr)
        {
            // need to associate the topology host of a dedicated server game's session with the game for notification in case of master going down
            BlazeRpcError err = ERR_OK;
            err = gUserSessionManager->insertBlazeObjectIdForSession(hostSessionInfo->getSessionId(), getBlazeObjectId());
            if (err != ERR_OK)
            {
                WARN_LOG("[GameSessionMaster].initialize: " << (ErrorHelp::getErrorName(err)) << " error adding host session " << hostSessionInfo->getSessionId() << " to game " << getGameId());
            }
        }

        // submit game session created event
        if (!isPseudoGame())
        {
            CreatedGameSession gameSessionEvent;
            gameSessionEvent.setGameId(getGameId());
            gameSessionEvent.setGameReportingId(getGameReportingId());
            gameSessionEvent.setGameName(createGameRequest.getGameCreationData().getGameName());
            gameSessionEvent.setCreationReason(GameManager::GameUpdateReasonToString(GameManager::GAME_INITIALIZED));
            gameSessionEvent.setNetworkTopology(GameNetworkTopologyToString(getGameNetworkTopology()));
            gameSessionEvent.getTopologyHostInfo().setPlayerId(getTopologyHostInfo().getPlayerId());
            gameSessionEvent.getTopologyHostInfo().setSlotId(getTopologyHostInfo().getSlotId());
            gameSessionEvent.getPlatformHostInfo().setPlayerId(getPlatformHostInfo().getPlayerId());
            gameSessionEvent.getPlatformHostInfo().setSlotId(getPlatformHostInfo().getSlotId());
            getTopologyHostNetworkAddressList().copyInto(gameSessionEvent.getHostAddressList());
            gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_CREATEDGAMESESSIONEVENT), gameSessionEvent, true);
        }

        return CreateGameMasterError::ERR_OK;
    }

    void GameSessionMaster::setGameReportingId(GameReportingId gameReportingId)
    {
        getGameData().setGameReportingId(gameReportingId);
    }


    void GameSessionMaster::clearPersistedGameIdAndSecret()
    {
        getGameData().setPersistedGameId("");
        getGameData().getPersistedGameIdSecret().setData(nullptr, 0);
    }

    /*! ************************************************************************************************/
    /*! \brief init a subset of GameData from a createGameRequest (used by create & reset game).

        Does NOT trigger a replicated game data update (since the map item hasn't been inserted
            into the map for createGame).

        \param[in] createGameRequest the createGameRequest to init this game's common data from.
        \param[in] creatingNewGame flag to show if the function is called by createGame session, default is true
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::initGameDataCommon(const CreateGameMasterRequest& masterRequest, bool creatingNewGame /* = true*/)
    {
        const CreateGameRequest& createGameRequest = masterRequest.getCreateRequest();

        GameSettings oldSettings = getGameData().getGameSettings();
        getGameData().getGameSettings() = createGameRequest.getGameCreationData().getGameSettings();

        // Crossplay virtual games need to set the platform(s) to use:
        eastl::string oldPlatform = getPlatformForMetrics();

        // Track the original platforms the dedicated server used, so we can reset to them when everyone leaves:
        if (creatingNewGame && isDedicatedHostedTopology(getGameData().getNetworkTopology()))
        {
            createGameRequest.getClientPlatformListOverride().copyInto(getGameData().getDedicatedServerSupportedPlatformList());
            if (getGameData().getDedicatedServerSupportedPlatformList().empty())
            {
                for (auto curPlat : gController->getHostedPlatforms())
                    getGameData().getDedicatedServerSupportedPlatformList().insertAsSet(curPlat);
            }

            getGameData().setIsCrossplayEnabled(false);
        }
        
        if (getGameData().getDedicatedServerSupportedPlatformList().empty())
        {
            createGameRequest.getClientPlatformListOverride().copyInto(getGameData().getBasePlatformList());
        }
        else
        {
            // Only set the Game's platforms to a subset of the allowed platforms on the server:
            getGameData().getBasePlatformList().clear();
            for (auto curPlat : getGameData().getDedicatedServerSupportedPlatformList())
            {
                if (createGameRequest.getClientPlatformListOverride().findAsSet(curPlat) != createGameRequest.getClientPlatformListOverride().end())
                    getGameData().getBasePlatformList().insertAsSet(curPlat);
            }
        }

        if (getGameData().getBasePlatformList().empty())
            getGameData().getDedicatedServerSupportedPlatformList().copyInto(getGameData().getBasePlatformList());

        getGameData().setIsCrossplayEnabled(masterRequest.getIsCrossplayEnabled());

        //Set PINIsCrossplayGame flag to false by default when game is created/reset
        getGameData().setPINIsCrossplayGame(false);

        refreshDynamicPlatformList();

        // to avoid potential title side game server bugs, disallow changing queue capacity updatability flag, post creation
        if (!creatingNewGame && (getGameData().getGameSettings().getUpdateQueueCapacityOnReset() != oldSettings.getUpdateQueueCapacityOnReset()))
        {
            WARN_LOG("[GameSessionMaster].initGameDataCommon: Cannot change value for GameSetting UpdateQueueCapacityOnReset in game(" <<
                getGameId() << ") from " << (oldSettings.getUpdateQueueCapacityOnReset() ? "true" : "false") << ". Setting unchanged.");

            getGameData().getGameSettings().setUpdateQueueCapacityOnReset(oldSettings.getUpdateQueueCapacityOnReset());
        }
        // If we're reseting a server, we need to track game settings changes, and update the metrics as needed: 
        if (!creatingNewGame)
        {
            mMetricsHandler.onGameSettingsChanged(oldSettings, getGameData().getGameSettings());
        }


        // set the game's queueCapacity as needed
        if (creatingNewGame || getGameData().getGameSettings().getUpdateQueueCapacityOnReset())
        {
            getGameData().setQueueCapacity(createGameRequest.getGameCreationData().getQueueCapacity());
            uint16_t maxQueueCapacity = gGameManagerMaster->getQueueCapacityMax();
            if (getQueueCapacity() > maxQueueCapacity)
            {
                getGameData().setQueueCapacity(maxQueueCapacity);
            }
        }

        getGameData().setPlayerReservationTimeout(createGameRequest.getGameCreationData().getPlayerReservationTimeout());
        getGameData().setDisconnectReservationTimeout(createGameRequest.getGameCreationData().getDisconnectReservationTimeout());
        getGameData().setPermissionSystemName(createGameRequest.getGameCreationData().getPermissionSystemName());

        if (getGameData().getPermissionSystemName()[0] != '\0' && (&getGamePermissionSystem() == &gGameManagerMaster->getGamePermissionSystem("")))
        {
            WARN_LOG("[GameSessionMaster] Unknown Permission system (" << getGameData().getPermissionSystemName() << ") used for game.");
        }

        setDataSourceNameList(createGameRequest.getGameCreationData().getDataSourceNameList());
        
        // If we're reseting a server, we need to track game mode changes, and update the metrics as needed: 
        eastl::string oldGameMode = (getGameMode() == nullptr) ? "" : getGameMode();
        createGameRequest.getGameCreationData().getGameAttribs().copyInto(getGameData().getGameAttribs());

        // validate mode exists
        if ((getGameMode() == nullptr))
        {
            // if the mode is missing, fail game creation
            // exception - games going into the resettable pool do not need to have a game mode attribute, as that's set on reset (or already set on the target session for host injection)
            if (creatingNewGame && isDedicatedHostedTopology(getGameData().getNetworkTopology()) && !getGameSettings().getVirtualized() && !getGameData().getServerNotResetable())
            {
                getGameData().getGameAttribs()[gGameManagerMaster->getConfig().getGameSession().getGameModeAttributeName()] = "";
            }
            else if (getGameType() == GAME_TYPE_GAMESESSION)
            {
                WARN_LOG("[GameSessionMaster] " << (creatingNewGame ? "Create" : "Reset") << " request for (" << getGameId() << ") was missing the game mode attribute '" 
                    << gGameManagerMaster->getConfig().getGameSession().getGameModeAttributeName() << "'.");
                // fail the creation
                return GAMEMANAGER_ERR_GAME_MODE_ATTRIBUTE_MISSING;
            }
        }

        if (!creatingNewGame)
        {
            eastl::string newPlatform = getPlatformForMetrics();
            if (oldPlatform != newPlatform)
            {
                gGameManagerMaster->removeGameFromCensusCache(*this);
                gGameManagerMaster->addGameToCensusCache(*this);
            }
        }

              
        getGameData().setGameProtocolVersionString(createGameRequest.getCommonGameData().getGameProtocolVersionString());

        getGameData().setPresenceMode(createGameRequest.getGameCreationData().getPresenceMode());
        createGameRequest.getGameCreationData().getPresenceDisabledList().copyInto(getGameData().getPresenceDisabledList());

        // This is still set, for use with backward compatible clients not using server driven external sessions:
        getGameData().setOwnsFirstPartyPresence(createGameRequest.getGameCreationData().getPresenceMode() != PRESENCE_MODE_NONE 
                                             && createGameRequest.getGameCreationData().getPresenceMode() != PRESENCE_MODE_NONE_2);

        // Hash off the game protocol version string for fast MM comparison.
        eastl::hash<const char8_t*> stringHash;
        getGameData().setGameProtocolVersionHash(stringHash(createGameRequest.getCommonGameData().getGameProtocolVersionString()));
        getGameData().setCreateGameTemplateName(masterRequest.getCreateGameTemplateName());

        if (creatingNewGame)
        {
            // settings that are ONLY set when creating a new game
            // (these values cannot be changed when resetting a dedicated server)
            getGameData().setVoipNetwork(createGameRequest.getGameCreationData().getVoipNetwork());
            getGameData().setMaxPlayerCapacity(createGameRequest.getGameCreationData().getMaxPlayerCapacity());
            getGameData().setMinPlayerCapacity(createGameRequest.getGameCreationData().getMinPlayerCapacity());
            createGameRequest.getMeshAttribs().copyInto(getGameData().getMeshAttribs());
            createGameRequest.getDedicatedServerAttribs().copyInto(getGameData().getDedicatedServerAttribs());

            // external session: set scid
            // external session template name, external session name and correlation id may update on reset, so they're not set here.
            if (getBasePlatformList().findAsSet(xone) != getBasePlatformList().end())
            {
                const ExternalSessionUtil* util = getExternalSessionUtilManager().getUtil(xone);
                if (util != nullptr)
                    getGameData().setScid(util->getConfig().getScid());
            }
            if (getBasePlatformList().findAsSet(xbsx) != getBasePlatformList().end())
            {
                const ExternalSessionUtil* util = getExternalSessionUtilManager().getUtil(xbsx);
                if (util != nullptr)
                    getGameData().setScid(util->getConfig().getScid());
            }

            if (!isDedicatedHostedTopology(getGameData().getNetworkTopology())) 
            {
                const int32_t CCS_HOST_MAX_CAPACITY = 32; // CCS hosting server (Dirtycast) can not handle client index > 32
                if (getGameData().getMaxPlayerCapacity() <= CCS_HOST_MAX_CAPACITY)
                {
                    CCSMode ccsMode = CCS_MODE_INVALID;
                    const char8_t* ccsPool = "";
                    ValidationUtils::getCCSModeAndPoolToUse(gGameManagerMaster->getConfig(), getGameMode(), isCrossplayEnabled(), ccsMode, ccsPool);

                    getGameData().setCCSMode(ccsMode);
                    getGameData().setCCSPool(ccsPool);
                }
                else
                {
                    WARN_LOG("[GameSessionMaster].initGameDataCommon: game(" << getGameId() << ") is not eligible for ccs assistance as player capacity(" << getGameData().getMaxPlayerCapacity() << ") is more than"
                        << "hosting server capacity(" << CCS_HOST_MAX_CAPACITY << ").");
                }
            }

            // retain the initial game template name, to use for restoring the
            // template name if/when this game is returned back to the pool
            mGameDataMaster->setInitialGameTemplateName(masterRequest.getCreateGameTemplateName());

            // we can safely mask client IPs if there's either no P2P connectivity (no game / voip networking or all via dedicated server, OR we're using hosted-only CCS, OR CCS failover in a crossplay game.)
            mGameDataMaster->setEnableNetworkAddressProtection(gGameManagerMaster->getEnableNetworkAddressProtection() &&
                ( ( ((getGameNetworkTopology() == CLIENT_SERVER_DEDICATED) || (getGameNetworkTopology() == NETWORK_DISABLED)) && ((getVoipNetwork() == VOIP_DISABLED) || (getVoipNetwork() == VOIP_DEDICATED_SERVER)) )
                    || (getCCSMode() == CCS_MODE_HOSTEDONLY) || (isCrossplayEnabled() && (getCCSMode() == CCS_MODE_HOSTEDFALLBACK)) ) );
        }

        // external session: set the correlation id, external session template name, external session name
        initExternalSessionData(masterRequest.getExternalSessionData(), &masterRequest.getCreateRequest().getGameCreationData(), &masterRequest.getCreateRequest().getTournamentSessionData());

        // copy the admin list in request over
        // which will wipe out the existing admin player in current list
        createGameRequest.getAdminPlayerList().copyInto(getGameData().getAdminPlayerList());

        if (getGamePermissionSystem().mEnableAutomaticAdminSelection)
        {
            // add the host back to the admin list, don't use addAdminPlayer since
            // the function will trigger update for ReplicatedGameData
            if (getTopologyHostInfo().getPlayerId() != INVALID_BLAZE_ID && !isAdminPlayer(getTopologyHostInfo().getPlayerId()))
                getGameData().getAdminPlayerList().push_back(getTopologyHostInfo().getPlayerId());
        }

        // set/validate game's slotPlayerCapacity
        BlazeRpcError err = validateSlotCapacities(createGameRequest.getSlotCapacities());
        if (err != ERR_OK)
        {
            return err;
        }

        const uint16_t oldTotalParticipantCapacity = creatingNewGame ? 0 : getTotalParticipantCapacity();
        const uint16_t oldTotalSpectatorCapacity = creatingNewGame ? 0 : getTotalSpectatorCapacity();
        // update the metrics after we set up the game attributes and cache off the old capacities
        createGameRequest.getSlotCapacities().copyInto(getGameData().getSlotCapacities());

        // validate game's teamCapacities
        err = validateTeams(createGameRequest.getGameCreationData().getTeamIds(), getTotalParticipantCapacity());
        if (err != ERR_OK)
        {
            return err;
        }

        getGameData().setGameModRegister(createGameRequest.getGameCreationData().getGameModRegister());

        // important to update teams before calling getTeamCapacity()
        createGameRequest.getGameCreationData().getTeamIds().copyInto(getGameData().getTeamIds());
        createGameRequest.getGameCreationData().getRoleInformation().copyInto(getGameData().getRoleInformation());


        // validate game's RoleInformation
        err = validateRoleInformation(createGameRequest.getGameCreationData().getRoleInformation(), getTeamCapacity());
        if (err != ERR_OK)
        {
            return err;
        }

        // Virtual games have slot metrics tracked upon host injection
        if (!getGameSettings().getVirtualized())
        {
            // update metric for new total capacity
            mMetricsHandler.calcPlayerSlotUpdatedMetrics(oldTotalParticipantCapacity, getTotalParticipantCapacity(), oldTotalSpectatorCapacity, getTotalSpectatorCapacity());
        }


        // set/parse the game entry criteria
        createGameRequest.getGameCreationData().getEntryCriteriaMap().copyInto(getGameData().getEntryCriteriaMap());

        if (!creatingNewGame)
        {
            clearCriteriaExpressions();
        }

        int32_t errorCount = createCriteriaExpressions();
        if (errorCount > 0)
        {
            TRACE_LOG("[GameSessionMaster] Failed to parse game entry criteria for game(" << getGameId() << ").");
            return GAMEMANAGER_ERR_INVALID_GAME_ENTRY_CRITERIA;
        }

        err = updateRoleEntryCriteriaEvaluators();
        if (err != ERR_OK)
        {
            TRACE_LOG("[GameSessionMaster] Failed to parse role-specific game entry criteria for game(" << getGameId() << ").");
            return err;
        }

        //set up bitset of available slots
        if (  EA_UNLIKELY(( (hasDedicatedServerHost()) && (SLOT_ID_MAX_COUNT < (getMaxPlayerCapacity() + 1)) ) ||
            (SLOT_ID_MAX_COUNT < getMaxPlayerCapacity()) ))
        {
            TRACE_LOG("[GameSessionMaster] MaxPlayerCapacity too large to store in bitset. Please increase SLOT_ID_MAX_COUNT in gamemanager.tdf");
            return GAMEMANAGER_ERR_MAX_PLAYER_CAPACITY_TOO_LARGE;
        }

        // re-initialize slot id lists to their maximum capacities
        mGameDataMaster->getOpenSlotIdList().clear();
        mGameDataMaster->getOpenSlotIdList().resize(SLOT_ID_MAX_ARRAY_SIZE, 0);
        asEndpointSlotBitset(mGameDataMaster->getOpenSlotIdList()).flip(); // mark all slots as open (ie: set to 1)
        mGameDataMaster->getConnectionSlotIdList().clear();
        mGameDataMaster->getConnectionSlotIdList().resize(SLOT_ID_MAX_ARRAY_SIZE, 0);
        asEndpointSlotBitset(mGameDataMaster->getConnectionSlotIdList()).flip(); // mark all slots as open (ie: set to 1)

        // Generate the UUID for the game object.
        UUID uuid;
        UUIDUtil::generateUUID( uuid );
        getGameData().setUUID( uuid );

		// WARNING:  PlayerJoinData GroupId is NOT VALID per-user (for create game matchmaking, only a single group id is set for the PJD).  
		//           The value must come from the UserJoinInfo in the cgMasterRequest
		// It's unclear if this should include all players' groups, but for safety we'll do that: 
		for (auto& userInfo : masterRequest.getUsersInfo())
		{
			UserGroupId groupId = userInfo->getUserGroupId(); // createGameRequest.getPlayerJoinData().getGroupId();
			if (groupId != EA::TDF::OBJECT_ID_INVALID)
			{
				// this guards against an invalid game group id from being inserted into the group list on creation of a dynamic dedicated server
				if (getGameSettings().getEnforceSingleGroupJoin() && isGroupAllowedToJoinSingleGroupGame(groupId))
				{
					addGroupIdToAllowedSingleGroupJoinGroupIds(groupId);
				}
			}
		}
		return ERR_OK;
    }

    // This function will be used to update the GEC that is specified by an Admin
    BlazeRpcError GameSessionMaster::setGameEntryCriteria(const SetGameEntryCriteriaRequest& request)
    {
        // Cache the previous values
        EntryCriteriaMap oldEntries;
        getGameData().getEntryCriteriaMap().copyInto(oldEntries);
        RoleInformation oldRoleInfo;
        getGameData().getRoleInformation().copyInto(oldRoleInfo);

        bool revertChanges = false;
        BlazeRpcError setEntryCriteriaError = ERR_OK;
        // set/parse the game entry criteria
        request.getEntryCriteriaMap().copyInto(getGameData().getEntryCriteriaMap());
        clearCriteriaExpressions();
        int32_t errorCount = createCriteriaExpressions();
        if (errorCount > 0)
        {
            revertChanges = true;
            TRACE_LOG("[GameSessionMaster::setGameEntryCriteria] Failed to parse game entry criteria for game(" << getGameId() << ").");
            setEntryCriteriaError = GAMEMANAGER_ERR_INVALID_GAME_ENTRY_CRITERIA;
        }
        else
        {
            // copy new entry criteria to the replicated game session
            RoleEntryCriteriaMap::const_iterator roleEntryCriteriaMapEnd = request.getRoleEntryCriteriaMap().end();
            RoleCriteriaMap::iterator roleCriteriaIter = getGameData().getRoleInformation().getRoleCriteriaMap().begin();
            RoleCriteriaMap::iterator roleCriteriaEnd = getGameData().getRoleInformation().getRoleCriteriaMap().end();
            for(; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
            {
                RoleEntryCriteriaMap::const_iterator roleEntryCriteriaIter = request.getRoleEntryCriteriaMap().find(roleCriteriaIter->first);
                if (roleEntryCriteriaIter == roleEntryCriteriaMapEnd)
                {
                    // clear entry criteria
                    roleCriteriaIter->second->getRoleEntryCriteriaMap().clear();
                }
                else
                {
                    // criteria was set
                    roleEntryCriteriaIter->second->copyInto(roleCriteriaIter->second->getRoleEntryCriteriaMap());
                }
            }

            // update takes care of clearing the old ExpressionMaps
            setEntryCriteriaError = updateRoleEntryCriteriaEvaluators();
            if (setEntryCriteriaError != ERR_OK)
            {
                TRACE_LOG("[GameSessionMaster::setGameEntryCriteria] Failed to parse role-specific game entry criteria for game(" << getGameId() << ").");
                revertChanges = true;
            }
        }

        if (revertChanges)
        {
            // now try the role-specific entry criteria
            oldEntries.copyInto(getGameData().getEntryCriteriaMap());
            clearCriteriaExpressions();
            errorCount = createCriteriaExpressions();
            if (errorCount > 0)
            {
                // This really shouldn't happen ever, the entries should remain valid while the game does.
                ERR_LOG("[GameSessionMaster::setGameEntryCriteria] Reverting to previous game entry criteria failed for game(" << getGameId() << ").");
            }

            oldRoleInfo.copyInto(getGameData().getRoleInformation());
            updateRoleEntryCriteriaEvaluators();
        }

        // now try the role-specific

        return setEntryCriteriaError;
    }

     /*! ************************************************************************************************/
    /*! \brief init virtual game's placeholder topology host user info
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameSessionMaster::initializeVirtualGameHost( const CreateGameRequest& request )
    {
        if (!request.getGameCreationData().getGameSettings().getVirtualized() || !hasDedicatedServerHost())
        {
            return GAMEMANAGER_ERR_INVALID_GAME_SETTINGS;
        }

        virtualizeDedicatedGame();

        return Blaze::ERR_OK;

    }

    /*! ************************************************************************************************/
    /*! \brief Virtualizes the dedicated game.
    ***************************************************************************************************/
    void GameSessionMaster::virtualizeDedicatedGame()
    {
        // add host information for the created game.
        setHostInfo(nullptr);

        // mark the host slot as 'occupied'
        markSlotOccupied(ON_GAME_CREATE_HOST_SLOT_ID);
        // we don't have a conn group id to associate
        markConnectionSlotOccupied(ON_GAME_CREATE_HOST_SLOT_ID);

        setGameState(INACTIVE_VIRTUAL);
    }

    /*! ************************************************************************************************/
    /*! \brief init game's (topology) host user info (and create/join a host player, if needed)
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameSessionMaster::initializeGameHost( const CreateGameMasterRequest& masterRequest, const GameSetupReason &playerJoinContext )
    {
        UserJoinInfoPtr hostJoinInfo = nullptr;
        const UserSessionInfo* hostSessionInfo = getHostSessionInfo(masterRequest.getUsersInfo(), &hostJoinInfo);
        if (!isPseudoGame() && !hostSessionInfo)
        {
            WARN_LOG("[GameSessionMaster] Host player is missing for game(" << getGameId() << ")");
            return ERR_SYSTEM;
        }

        const UserSessionId hostUserSessionId = hostSessionInfo->getSessionId();
        const PlayerId hostPlayerId = hostSessionInfo->getUserInfo().getId();
        const char8_t *hostPersonaName = hostSessionInfo->getUserInfo().getPersonaName();
        const CreateGameRequest& request = masterRequest.getCreateRequest();

        bool isHostless = ((getGameNetworkTopology() == NETWORK_DISABLED) 
                                && !gGameManagerMaster->getConfig().getGameSession().getAssignTopologyHostForNetworklessGameSessions());

        if (!isPseudoGame() && !gUserSessionManager->getSessionExists(hostUserSessionId))
        {
            WARN_LOG("[GameSessionMaster] Host player (" << hostPlayerId << ":"
                << hostPersonaName << ") for game(" << getGameId() << "), didn't have a session");
            return ERR_SYSTEM;
        }

        // create player info for the host, for dedicated server, host is not a player
        // so we don't need to create playerInfo and add the player to roster
        if (!isDedicatedHostedTopology(getGameNetworkTopology()))
        {
            // first get the host's role
            const PerPlayerJoinData* hostData = lookupPerPlayerJoinDataConst(request.getPlayerJoinData(), hostSessionInfo->getUserInfo());
            if (hostData == nullptr)
            {
                WARN_LOG("[GameSessionMaster] Failed to create host player(" << hostPlayerId << ":"
                    << hostPersonaName << ") for game(" << getGameId() << "), no player role specified, error("
                    << (ErrorHelp::getErrorName(GAMEMANAGER_ERR_ROLE_NOT_ALLOWED)) << ")");
                return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
            }

            RoleName hostRole = hostData->getRole();
            if (hostRole.empty())
            {
                hostRole = request.getPlayerJoinData().getDefaultRole();
            }

            const NetworkAddress *networkAddress = nullptr;
            if (request.getCommonGameData().getPlayerNetworkAddress().getActiveMember() != NetworkAddress::MEMBER_UNSET)
            {
                // Note that we blindly copy the first element here.  Ideally this gets moved to pulling the address from UserExtended data.
                networkAddress = &request.getCommonGameData().getPlayerNetworkAddress();
            }
            else if (!isHostless)
            {
                ERR_LOG("[GameSessionMaster] Initializing game for host(" << hostPlayerId << ":" 
                    << hostPersonaName << ") and game(" << getGameId() 
                    << ") failed, player network address was not set.");
                return Blaze::ERR_SYSTEM;
            }

            SlotType slotType = getSlotType(request.getPlayerJoinData(),  hostData->getSlotType());
            BlazeRpcError err = validateJoinGameSlotType(slotType, slotType);
            if (EA_UNLIKELY(err != ERR_OK))
            {
                WARN_LOG("[GameSessionMaster] Failed to find a valid slot type for host player(" << hostPlayerId << ":"
                        << hostPersonaName << ") for game(" << getGameId() << "), error("
                        << (ErrorHelp::getErrorName(err)) << ")");
                return err;
            }

            PlayerInfoMasterPtr newPlayer = nullptr;
            PlayerState newPlayerState = (request.getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION) ? RESERVED : ACTIVE_CONNECTED;

            err = mPlayerRoster.createNewPlayer(hostPlayerId, newPlayerState,
                slotType,
                getTeamIndex(request.getPlayerJoinData(), hostData->getTeamIndex()),
                hostRole,
				(hostJoinInfo != nullptr) ? hostJoinInfo->getUserGroupId() : request.getPlayerJoinData().getGroupId(),  // Always gather GroupData from UserJoinInfo, not PlayerJoinData (to support CG MM games)
                *hostSessionInfo, 
                GameManager::getEncryptedBlazeId(request.getPlayerJoinData(), hostSessionInfo->getUserInfo()),
                newPlayer);
            if (EA_UNLIKELY(err != ERR_OK))
            {
                // RESERVED players are no longer allowed (even temporarily) as host of the game.
                // This is because they are now replicated all the way to the client.
                WARN_LOG("[GameSessionMaster] Failed to create host player(" << hostPlayerId << ":"
                        << hostPersonaName << ") for game(" << getGameId() << "), error("
                        << (ErrorHelp::getErrorName(err)) << ")");
                return err;
            }

            if (blaze_stricmp(newPlayer->getRoleName(), PLAYER_ROLE_NAME_ANY) == 0)
            {
                err = findOpenRole(*newPlayer, &request.getPlayerJoinData(), hostData->getRoles(), hostRole);
                if (err != ERR_OK)
                {
                    return JoinGameMasterError::commandErrorFromBlazeError(err);
                }
                newPlayer->setRoleName(hostRole);
            }

            newPlayer->setJoinMethod(SYS_JOIN_BY_CREATE);
            if (networkAddress != nullptr)
            {
                newPlayer->setNetworkAddress((*networkAddress));  // clone host's network addresses into host player
            }
            newPlayer->setPlayerAttributes(hostData->getPlayerAttributes()); // clone host's player attributes into host player

            if (hostJoinInfo != nullptr)
            {
                newPlayer->setScenarioInfo(hostJoinInfo->getScenarioInfo());
            }

            // Note, below order should not be changed, we have to change the game state first
            // and then add admin player and player. In this way, we can make sure the game
            // state on client side is INITIALIZING instead of NEW_STATE when client side
            // createGame called back is triggered.
            if (!isPseudoGame() && (!isHostless || !request.getGameCreationData().getSkipInitializing()))
            {
                getGameData().setGameState(INITIALIZING);
            }
            else if (getGameType() == GAME_TYPE_GAMESESSION)
            {
                getGameData().setGameState(PRE_GAME);  // This should probably be configured.
            }
            else if (getGameType() == GAME_TYPE_GROUP)
            {
                getGameData().setGameState(GAME_GROUP_INITIALIZED);  // This should probably be configured.
            }

            if (getGamePermissionSystem().mEnableAutomaticAdminSelection)
            {
                const PlayerId noPlayer = 0;
                addAdminPlayer(hostPlayerId, noPlayer); // GM_AUDIT: addAdminPlayer triggers game admin list replication
            }

            sendGameStateUpdate();

            // add the host as player.
            if (newPlayerState == RESERVED)
            {
                err = addReservedPlayer(newPlayer.get(), playerJoinContext);
            }
            else
            {
                err = addActivePlayer(newPlayer.get(), playerJoinContext);
            }

            if (err != ERR_OK)
            {
                WARN_LOG("[GameSessionMaster] Failed to initialize host player(" << newPlayer->getPlayerId() << ") into game("
                         << getGameId() << "), error " << (ErrorHelp::getErrorName(err)));

                return err;
            }

            // Send the notification that the player is fuly connected:
            if (newPlayer->isConnected())
            {
                sendPlayerJoinCompleted(*newPlayer);
            }

            // submit join game session event
            if (!isPseudoGame())
            {
                bool isPlatformHost = ( hasDedicatedServerHost() && (getPlatformHostInfo().getPlayerId() == newPlayer->getPlayerId()));
                PlayerJoinedGameSession gameSessionEvent;
                gameSessionEvent.setPlayerId(newPlayer->getPlayerId());
                gameSessionEvent.setGameId(getGameId());
                gameSessionEvent.setGameReportingId(getGameReportingId());
                gameSessionEvent.setGameName(getGameName());
                gameSessionEvent.setPersonaName(newPlayer->getPlayerName());
                gameSessionEvent.setPersonaNamespace(newPlayer->getPlayerNameSpace());
                gameSessionEvent.setClientPlatform(newPlayer->getPlatformInfo().getClientPlatform());
                gameSessionEvent.setPlayerId(newPlayer->getClientPlatform());
                gameSessionEvent.setIsPlatformHost(isPlatformHost);
                gameSessionEvent.setRoleName(hostRole);
                gameSessionEvent.setSlotType(slotType);
                getGameAttribs().copyInto(gameSessionEvent.getGameAttribs());
                gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_PLAYERJOINEDGAMESESSIONEVENT), gameSessionEvent, true);
            }
            return Blaze::ERR_OK;
        }
        else
        {
            initializeDedicatedGameHost(*hostSessionInfo);
            return Blaze::ERR_OK;
        }
    }


    /*! ************************************************************************************************/
    /*! \brief initializes a dedicated server host.
    ***************************************************************************************************/
    void GameSessionMaster::initializeDedicatedGameHost(const UserSessionInfo& hostInfo)
    {
        // when a Dedicated Server game is created, the host (admin) does not actually joins the game
        // so we don't call gameSessionMaster->addPlayer here, however, we should still update the network
        // behavior of the host. So later on, if the host actually joined the game, we don't need to do this anymore

        // only add the endpoint for client server dedicated.
        mPlayerConnectionMesh.addEndpoint(hostInfo.getConnectionGroupId(), hostInfo.getUserInfo().getId(), hostInfo.getUserInfo().getPlatformInfo().getClientPlatform(), CREATED, false);

        if (getGameSettings().getVirtualized())
        {
            getGameData().setGameState(PRE_GAME);
        }
        else if (getGameData().getServerNotResetable())
        {
            // Non resettable dedicated servers go straight to initializing. dynamic dedicated servers are created to host a specific game, so they start in initializing
            // the players will join the game once the DS host has called init complete.
            getGameData().setGameState(INITIALIZING);
        }
        else
        {
            // regular dedicated servers are created in the resettable state and wait to be reset
            getGameData().setGameState(RESETABLE);
        }

        mMetricsHandler.onDedicatedGameHostInitialized(getGameSettings());

        if (isPseudoGame())
        {
            getGameData().setGameState(PRE_GAME);
        }

        if (getGamePermissionSystem().mEnableAutomaticAdminSelection)
        {
            const PlayerId noPlayer = 0;
            addAdminPlayer(hostInfo.getUserInfo().getId(), noPlayer);
        }

        sendGameStateUpdate();

        //Add game to dedicated server user's session game map
        gGameManagerMaster->addUserSessionGameInfo(hostInfo.getSessionId(), *this);
    }

    /*! ************************************************************************************************/
    /*! \brief initializes game with its external owner
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameSessionMaster::initializeExternalOwner(const CreateGameRequest& request)
    {
        TRACE_LOG("[GameSessionMaster].initializeExternalOwner: owner(" << getExternalOwnerBlazeId() << "), sessionId(" << getExternalOwnerSessionId() << ") for game(" << getGameId() << ")");  ASSERT_COND_LOG((getExternalOwnerBlazeId() != INVALID_BLAZE_ID && getExternalOwnerSessionId() != INVALID_USER_SESSION_ID), "external owner info missing.");

        if (request.getGameCreationData().getSkipInitializing())
        {
            getGameData().setGameState(isGameGroup() ? GAME_GROUP_INITIALIZED : PRE_GAME);
        }
        else
        {
            getGameData().setGameState(INITIALIZING);
        }
        
        if (getGamePermissionSystem().mEnableAutomaticAdminSelection)
        {
            const PlayerId noPlayer = INVALID_BLAZE_ID;
            addAdminPlayer(getExternalOwnerBlazeId(), noPlayer);//pre: owner id initialized
        }

        sendGameStateUpdate();

        gGameManagerMaster->addUserSessionGameInfo(getExternalOwnerSessionId(), *this);
        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief set this game's host information from the supplied hostSession.  Updates the hostPlayerId,
            hostSessionId, hostSlotId, and the game's network QOS data.

        \param[in] id the topology host's session id.  Note: we can't pass in a player, since dedicated server hosts aren't players
        \param[in] connGroupId dedicated server topology host's connection group. 0 omits setting
    ***************************************************************************************************/
    void GameSessionMaster::setHostInfo(const UserSessionInfo* hostInfo)
    {
        UserSessionId id = INVALID_USER_SESSION_ID;
        BlazeId blazeId = INVALID_BLAZE_ID;
        ConnectionGroupId connGroupId = 0;

        if (hostInfo != nullptr)
        {
            id = hostInfo->getSessionId();
            blazeId = hostInfo->getUserInfo().getId();
            connGroupId = hostInfo->getConnectionGroupId();
        }
        else
        {
            UserSessionInfo emptyHostInfo;
            emptyHostInfo.copyInto(mGameData->getTopologyHostUserInfo());
        }


        if ( isDedicatedHostedTopology(getGameNetworkTopology()) )
        {
            if ( (getGameState() == NEW_STATE) || (getGameState() == INITIALIZING) || (getGameState() == INACTIVE_VIRTUAL) )
            {
                // game creation
                if(hostInfo)
                {
                    hostInfo->copyInto(mGameData->getTopologyHostUserInfo());
                }

                // init topology host info (he's an endpoint, but not a player in the game)

                getGameData().getDedicatedServerHostInfo().setUserSessionId(id);
                getGameData().getDedicatedServerHostInfo().setPlayerId(blazeId);
                getGameData().getDedicatedServerHostInfo().setSlotId(ON_GAME_CREATE_HOST_SLOT_ID);
                getGameData().getDedicatedServerHostInfo().setConnectionSlotId(ON_GAME_CREATE_HOST_SLOT_ID);
                getGameData().getDedicatedServerHostInfo().setConnectionGroupId(connGroupId);

                getGameData().getDedicatedServerHostInfo().copyInto(getGameData().getTopologyHostInfo());

                // If the hostInfo was nullptr, this is going to be a virtual game. Don't actually occupy a slot.
                if (hostInfo != nullptr)
                {
                    // mark the slots occupied for the host.
                    markSlotOccupied(getDedicatedServerHostInfo().getSlotId());
                    // Assumes dedicated host does not have any MLU members.
                    markConnectionSlotOccupied(getDedicatedServerHostInfo().getConnectionSlotId(), getDedicatedServerHostInfo().getConnectionGroupId());
                }


                // In the case of a remote host injection, we'll have already selected a platform host, so need to ensure we don't clobber it with
                // the dedicated server host's data
                if ((mPlayerRoster.getRosterPlayerCount() == 0) || (mPlayerRoster.getPlayer(getGameData().getPlatformHostInfo().getPlayerId()) == nullptr))
                {
                    // the platform host is initially set to the dedicated server
                    //   This is a sentinel value, since the (pc) dedicated server can't act as a platform host
                    //   (the platform host is responsible for creating a 360 presence session).  We update
                    //   the platform host as soon as the first real player joins (a 360 client)
                    getGameData().getDedicatedServerHostInfo().copyInto(getGameData().getPlatformHostInfo());
                }

                TRACE_LOG("[GameSessionMaster] Dedicated server user(" << getDedicatedServerHostBlazeId() << ") in slot "
                          << getDedicatedServerHostInfo().getSlotId() << " and connectionSlot " << getDedicatedServerHostInfo().getConnectionSlotId() << " set as host of game(" << getGameId() << ") in state " << GameStateToString(getGameState()));

                // update the game's network QOS info given the new host
                if (hostInfo != nullptr)
                    updateNetworkQosData(hostInfo->getQosData());
            }
            else if (getGameState() == MIGRATING)
            {
                // set the Platform Host during a platform host migration.  For the dedicated server case
                // the topology host doesn't get migrated as it is fixed.
                getGameData().getPlatformHostInfo().setPlayerId(blazeId);
                getGameData().getPlatformHostInfo().setUserSessionId(id);
                PlayerInfoMaster* platHostPlayer = mPlayerRoster.getPlayer(getPlatformHostInfo().getPlayerId());
                getGameData().getPlatformHostInfo().setSlotId(platHostPlayer->getSlotId());
                getGameData().getPlatformHostInfo().setConnectionSlotId(platHostPlayer->getConnectionSlotId());
                getGameData().getPlatformHostInfo().setConnectionGroupId(platHostPlayer->getConnectionGroupId());
                TRACE_LOG("[GameSessionMaster] Player(" << getPlatformHostInfo().getPlayerId() << ") in slot " << getPlatformHostInfo().getSlotId()
                          << " set as platform host of game(" << getGameId() << ")")
            }
        }
        else
        {
            if(hostInfo)
            {
                hostInfo->copyInto(mGameData->getTopologyHostUserInfo());
            }

            // In the non dedicated server case topology host is the same as the platform host.
            getGameData().getTopologyHostInfo().setUserSessionId(id);
            getGameData().getTopologyHostInfo().setPlayerId(blazeId);
            getGameData().getTopologyHostInfo().setConnectionGroupId(connGroupId);
            getGameData().getPlatformHostInfo().setUserSessionId(id);
            getGameData().getPlatformHostInfo().setPlayerId(blazeId);
            getGameData().getPlatformHostInfo().setConnectionGroupId(connGroupId);


            //not a dedicated server game
            if ( (getGameState() == NEW_STATE) || (getGameState() == INITIALIZING) || !hostInfo)
            {
                getGameData().getTopologyHostInfo().setSlotId(ON_GAME_CREATE_HOST_SLOT_ID);
                getGameData().getTopologyHostInfo().setConnectionSlotId(ON_GAME_CREATE_HOST_SLOT_ID);
            }
            else
            {
                // setting host after host migration
                getGameData().getTopologyHostInfo().setSlotId(mPlayerRoster.getPlayer(getTopologyHostInfo().getPlayerId())->getSlotId());
                getGameData().getTopologyHostInfo().setConnectionSlotId(mPlayerRoster.getPlayer(getTopologyHostInfo().getPlayerId())->getConnectionSlotId());
            }

            getGameData().getPlatformHostInfo().setSlotId( getTopologyHostInfo().getSlotId() );
            getGameData().getPlatformHostInfo().setConnectionSlotId( getTopologyHostInfo().getConnectionSlotId() );

            TRACE_LOG("[GameSessionMaster] Player(" << getTopologyHostInfo().getPlayerId() << ":connGroupId=" << getTopologyHostInfo().getConnectionGroupId() << ") in slot " << getTopologyHostInfo().getSlotId() 
                      << " set as host of game(" << getGameId() << ")");

            // update the game's network QOS info given the new host
            if (hostInfo != nullptr)
                updateNetworkQosData(hostInfo->getQosData());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief update the game's network QOS data (bandwidth & NAT type)
    ***************************************************************************************************/
    void GameSessionMaster::updateNetworkQosData(const Util::NetworkQosData& hostQosData)
    {
        // update the bandwidth from the host session's qos data
        getGameData().getNetworkQosData().setUpstreamBitsPerSecond( hostQosData.getUpstreamBitsPerSecond() );
        getGameData().getNetworkQosData().setDownstreamBitsPerSecond( hostQosData.getDownstreamBitsPerSecond() );

        // update the game's NAT type depending on the game's network topology
        if (!isNetworkFullMesh())
        {
            // not p2p full: the game's NAT type is equal to the host's NAT type
            getGameData().getNetworkQosData().setNatType( hostQosData.getNatType() );
        }
        else
        {
            // p2p full: the game's NAT type is equal to the most restrictive player's NAT type
            getGameData().getNetworkQosData().setNatType( getMostRestrictivePlayerNatType() );
        }

    }

    /*! ************************************************************************************************/
    /*! \brief update the game's NAT type (inside networkQosInfo) to the most restrictive player's NAT type.
            Only valid for p2p full mesh games; will send replication update if the NAT type has changed.
    ***************************************************************************************************/
    void GameSessionMaster::updateGameNatType()
    {
        // GM_AUDIT - inefficient: caller knows who's the joining/leaving player
        //   for joins, we can just compare game.Nat vs join.Nat
        //   for leaves, we can ignore anyone leaving who's less restrictive than the game...

        if (!isNetworkFullMesh())
        {
            FAIL_LOG("[GameSessionMaster::updateGameNatType()]: called on non-full mesh game. Gameid = " << getGameId())
            return;
        }

        Util::NatType newNatType = getMostRestrictivePlayerNatType();
        if (newNatType != getNetworkQosData().getNatType())
        {
            getGameData().getNetworkQosData().setNatType(newNatType);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief iterate over the game's active players, and return the most restrictive NAT type found.
            Only usable for non-dedicated server topologies (ie: host must be a game player)

        \return the most restrictive NAT type found from the game's active players
    ***************************************************************************************************/
   Util::NatType GameSessionMaster::getMostRestrictivePlayerNatType() const
    {

        if (getGameNetworkTopology() == CLIENT_SERVER_DEDICATED)
        {
            FAIL_LOG("[GameSessionMaster::getMostRestrictivePlayerNatType]: Cannot be called on dedicated server game, gameid = " << getGameId())
            return Util::NAT_TYPE_UNKNOWN;
        }

        Util::NatType mostRestrictiveNatType = Util::NAT_TYPE_OPEN;
        // we care about players here, though we generally defer to participants over spectators when deciding who to kick because of a connection problem
        // a spectator could potentially become game host, so we have to check everyone
        const PlayerRoster::PlayerInfoList& activePlayerList = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        PlayerRoster::PlayerInfoList::const_iterator playerIter = activePlayerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator playerEnd = activePlayerList.end();
        for ( ; ((playerIter != playerEnd) && (mostRestrictiveNatType < Util::NAT_TYPE_UNKNOWN)); ++playerIter )
        {
            Util::NatType currentNatType = (*playerIter)->getQosData().getNatType();
            if ( currentNatType > mostRestrictiveNatType )
            {
                mostRestrictiveNatType = currentNatType;
            }
        }

        return mostRestrictiveNatType;
    }

    const GamePermissionSystemInternal& GameSessionMaster::getGamePermissionSystem() const
    {
        return gGameManagerMaster->getGamePermissionSystem(getGameData().getPermissionSystemName());
    }

    bool GameSessionMaster::hasPermission(PlayerId playerId, GameAction::Actions action) const
    {
        const GamePermissionSystemInternal& permissionSystem = getGamePermissionSystem();

        // The command is being done by a super user, so just accept it as valid:
        // (Ex.  UserSession::SuperUserPermissionAutoPtr autoPtr(true); )
        if (UserSession::hasSuperUserPrivilege())
            return true;

        // HOST Permissions:
        bool isHost = false;
        if (getTopologyHostInfo().getPlayerId() == playerId)
        {
            isHost = true;
            const GamePermissionSystemInternal::GameActionSet& actionSet = permissionSystem.mHost;
            if (actionSet.find(action) != actionSet.end())
                return true;
        }
        
        // ADMIN Permissions:
        bool isAdmin = false;
        if (isAdminPlayer(playerId))
        {
            isAdmin = true;
            const GamePermissionSystemInternal::GameActionSet& actionSet = permissionSystem.mAdmin;
            if (actionSet.find(action) != actionSet.end())
                return true;
        }

        bool isParticipant = false;
        bool isSpectator = false;
        const PlayerInfoMaster* playerInfo = getPlayerRoster()->getPlayer(playerId);
        if (playerInfo != nullptr)
        {
            // PARTICIPANT Permissions:
            if (playerInfo->isParticipant())
            {
                isParticipant = true;
                const GamePermissionSystemInternal::GameActionSet& actionSet = permissionSystem.mParticipant;
                if (actionSet.find(action) != actionSet.end())
                    return true;
            }

            // SPECTATOR Permissions:
            if (playerInfo->isSpectator())
            {
                isSpectator = true;
                const GamePermissionSystemInternal::GameActionSet& actionSet = permissionSystem.mSpectator;
                if (actionSet.find(action) != actionSet.end())
                    return true;
            }
        }

        WARN_LOG("PlayerId " << playerId << " is missing game permission " << GameAction::ActionsToString(action) << ".  Player is type (" 
                             << (isHost ? " HOST " : "") << (isAdmin ? " ADMIN " : "") <<  (isParticipant ? " PARTICIPANT " : "") << (isSpectator ? " SPECTATOR " : "") <<").");

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief return true if the supplied playerId has game admin rights.
            Note: doesn't have to be a game member (ie: ded server host)
    ***************************************************************************************************/
    bool GameSessionMaster::isAdminPlayer(PlayerId playerId) const
    {
        const PlayerIdList& adminList = getAdminPlayerList();
        for (PlayerIdList::const_iterator adminIdIter = adminList.begin(), adminIdEnd = adminList.end(); adminIdIter != adminIdEnd; ++adminIdIter)
        {
            if (playerId == *adminIdIter)
            {
                return true;
            }
        }

        return false;
    }


   /*! ************************************************************************************************/
   /*! \brief add the player to the admin player list and update the replicated game data map
   ***************************************************************************************************/
    void GameSessionMaster::addAdminPlayer(PlayerId playerId, PlayerId sourcePlayerId)
   {
       if (!isAdminPlayer(playerId))
       {
           getGameData().getAdminPlayerList().push_back(playerId);
           updateAdminPlayerList(playerId, GM_ADMIN_ADDED, sourcePlayerId);
       }
   }

   /*! ************************************************************************************************/
   /*! \brief remove the player from admin player list. If the operation necessitates choosing a new admin player,
        avoid any other player from the the playerId's connection group if skipPlayerConnGroup is true.
   ***************************************************************************************************/
    void GameSessionMaster::removeAdminPlayer(PlayerId playerId, PlayerId sourcePlayerId, ConnectionGroupId connectionGroupToSkip)
    {
        PlayerIdList &adminList = getGameData().getAdminPlayerList();

        for (PlayerIdList::iterator iter = adminList.begin(), end = adminList.end() ; iter != end; ++iter)
        {
            PlayerId adminId = *iter;
            if (adminId == playerId)
            {
                adminList.erase(iter);
                updateAdminPlayerList(playerId, GM_ADMIN_REMOVED, sourcePlayerId);

                if (getGamePermissionSystem().mEnableAutomaticAdminSelection)
                {
                    // We need at least 1 player admin (apart from the dedicated server host who is always an admin). So we choose a new one if applicable. 
                    bool promotePlayerToAdmin = hasDedicatedServerHost() ? (adminList.size() == 1) : adminList.empty();
                    if (promotePlayerToAdmin)
                    {
                        PlayerInfoMaster* newAdminPlayer = mPlayerRoster.pickPossHostByTime(playerId, connectionGroupToSkip);
                        if (newAdminPlayer != nullptr)
                        {
                            addAdminPlayer(newAdminPlayer->getPlayerId(), sourcePlayerId);
                        }
                        else
                        {
                            TRACE_LOG("[GameSessionMaster] Unable to assign a new admin for game(" << getGameId() << ")");
                        }
                    }
                }
                return;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief add the targetPlayerId as the new admin and remove sourcePlayerId as an admin.
    ***************************************************************************************************/
    void GameSessionMaster::migrateAdminPlayer(PlayerId targetPlayerId, PlayerId sourcePlayerId)
    {
        PlayerIdList &adminList = getGameData().getAdminPlayerList();

        for (PlayerIdList::iterator iter = adminList.begin(), end = adminList.end() ; iter!=end; ++iter )
        {
            PlayerId adminId = *iter;
            if (adminId == sourcePlayerId)
            {
                adminList.erase(iter);
                break;
            }
        }

        if (!isAdminPlayer(targetPlayerId))
        {
            adminList.push_back(targetPlayerId);
            updateAdminPlayerList(targetPlayerId, GM_ADMIN_MIGRATED, sourcePlayerId);
        }
    }

   /*! ************************************************************************************************/
   /*! \brief update game replicated data when admin list is updated

        \param[in] playerId - player added to or removed admin list
        \param[in] operation - a enum to indicated the player is added or removed from admin list
   ***************************************************************************************************/
   void GameSessionMaster::updateAdminPlayerList(PlayerId playerId, UpdateAdminListOperation operation, PlayerId updaterPlayerId)
   {
       NotifyAdminListChange adminListNotification;
       adminListNotification.setGameId(getGameId());
       adminListNotification.setAdminPlayerId(playerId);
       adminListNotification.setUpdaterPlayerId(updaterPlayerId);
       adminListNotification.setOperation(operation);
       GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
       gGameManagerMaster->sendNotifyAdminListChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &adminListNotification);
   }

    /*! **********************************************************************************************/
    /*! \brief Set the game as responsive or not

        If a game is unresponsive, all state transitions are disallowed (other than becoming responsive
        again in the pre-unresponsive state).

        \param[in] responsive   Whether to set the game as responsive or not
    **************************************************************************************************/
    void GameSessionMaster::setResponsive(bool responsive)
    {
        GameState newGameState;

        if (responsive)
        {
            if (getGameState() != UNRESPONSIVE)
            {
                TRACE_LOG("[GameSessionMaster].setResponsive: game(" << getGameId() << ") already responsive -- current state is "
                    << GameStateToString(getGameState()));
                return;
            }

            WARN_LOG("[GameSessionMaster].setResponsive: setting game(" << getGameId() << ") responsive -- previous state before becoming unresponsive was "
                << GameStateToString(mGameDataMaster->getPreUnresponsiveState()));
            newGameState = mGameDataMaster->getPreUnresponsiveState();
            
        }
        else
        {
            if (getGameState() == UNRESPONSIVE)
            {
                TRACE_LOG("[GameSessionMaster].setResponsive: game(" << getGameId() << ") already not responsive -- previous state before becoming unresponsive was "
                    << GameStateToString(mGameDataMaster->getPreUnresponsiveState()));
                return;
            }
            if (getGameState() == INACTIVE_VIRTUAL)
            {
                TRACE_LOG("[GameSessionMaster].setResponsive: NOOP as game(" << getGameId() << ") state is " << GameStateToString(getGameState()) << " -- previous state before becoming unresponsive was "
                    << GameStateToString(mGameDataMaster->getPreUnresponsiveState()));
                return;
            }

            mGameDataMaster->setPreUnresponsiveState(getGameState());
            WARN_LOG("[GameSessionMaster].setResponsive: setting game(" << getGameId() << ") unresponsive -- current state is "
                << GameStateToString(mGameDataMaster->getPreUnresponsiveState()));
            newGameState = UNRESPONSIVE;
        }

        mMetricsHandler.onGameResponsivenessChanged(responsive);
        processSetReplicatedGameState(newGameState);
    }

    /*! ************************************************************************************************/
    /*! \brief sets the game state for this game. A game state transition check is made to make sure that
            that its valid.
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::setGameState(const GameState newGameState, bool canTransitionFromConnectionVerification/* = false*/)
    {
        if (newGameState == getGameState())
        {
            // no op, already in that state
            return Blaze::ERR_OK;
        }
        if (newGameState == RESETABLE && isServerNotResetable())
        {
            WARN_LOG("[GameSessionMaster] Invalid state change to RESETABLE, server does not allow transition.");
            return Blaze::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
        }

        if ((CONNECTION_VERIFICATION == getGameState()) && ((RESETABLE != newGameState) && !canTransitionFromConnectionVerification))
        {
            WARN_LOG("[GameSessionMaster] Server does not allow transition from CONNECTION_VERIFICATION state outside of QoS validation or dedicatd server reset.");
            return Blaze::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
        }

        if (!gGameManagerMaster->getGameStateHelper()->isStateChangeAllowed(getGameState(), newGameState))
        {
            TRACE_LOG("[GameSessionMaster] Invalid state transition(" << getGameState() << " -> " << newGameState << ") for game(" << getGameId() << ")");
            return Blaze::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
        }

        processSetReplicatedGameState(newGameState);
        return Blaze::ERR_OK;
    }

    /*! **********************************************************************************************/
    /*! \brief helper to set the game state

        The replicated game state is set and updates/notifications are sent.

        \param[in] newGameState   New game state
    **************************************************************************************************/
    void GameSessionMaster::processSetReplicatedGameState(const GameState newGameState)
    {
        //save off original data for external session update afterwards
        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        // submit the GameSessionStateChanged event.
        // IMPORTANT: This needs to occur before the mReplicatedGameSession.setGameState() in order to get the old game state
        // IMPORTANT: This event is submitted via the EventManager::submitBinaryEvent() which does not direct the event to
        //            the outgoing XML event stream.  But rather, only to the binary event log file.
        GameSessionStateChanged stateChanged;
        stateChanged.setGameId(getGameId());
        stateChanged.setGameReportingId(getGameReportingId());
        stateChanged.setGameName(getGameName());
        stateChanged.setOldGameState(getGameState()); // game state being transitioned from
        stateChanged.setNewGameState(newGameState);   // game state being transitioned to
        gEventManager->submitBinaryEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_GAMESESSIONSTATECHANGEDEVENT), stateChanged);

        getGameData().setGameState(newGameState);

        sendGameStateUpdate();

        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());
    }

    /*! ************************************************************************************************/
    /*! \brief set the game settings for the game.  the settings are blown away, not merged.

        \param[in] settings - new setting for the game
    ***************************************************************************************************/
    SetGameSettingsMasterError::Error GameSessionMaster::setGameSettings(const GameSettings* setting)
    {
        BlazeRpcError err = gGameManagerMaster->getGameStateHelper()->isActionAllowed(this, GAME_SETTING_UPDATE);
        if (err != ERR_OK)
        {
            WARN_LOG("[GameSessionMaster] Invalid action, setting update action for game(" << getGameId() << ") not allowed.");
            return SetGameSettingsMasterError::commandErrorFromBlazeError(err);
        }

        //save off original data for external session update afterwards
        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        GameSettings oldSettings = getGameSettings();
        bool origDynamicRepGameSetting = getGameSettings().getDynamicReputationRequirement();

        //need to block changes to allowAnyReputation if dynamic rep requirement enabled, cache old value
        bool origAllowAnyRepGameSetting = getGameData().getGameSettings().getAllowAnyReputation();
        //need to block changes to virtualized setting, cache old value
        bool origVirtualizedGameSetting = getGameData().getGameSettings().getVirtualized();
        

        bool origEnablePersistedGameIdGameSetting = getGameData().getGameSettings().getEnablePersistedGameId();

        getGameData().getGameSettings().setBits( setting->getBits() );
        if (setting->getVirtualized() != origVirtualizedGameSetting)
        {
            WARN_LOG("[GameSessionMaster] Cannot change value for GameSetting virtualized in game(" << getGameId() << ") from " << (origVirtualizedGameSetting ? "true" : "false") << ".");
            getGameData().getGameSettings().setVirtualized(origVirtualizedGameSetting);
        }

        if (setting->getEnablePersistedGameId() != origEnablePersistedGameIdGameSetting)
        {
            //flag enablePersistedGameId should not be set if game session has no persisted game id
            if (!hasPersistedGameId())
            {
                getGameData().getGameSettings().clearEnablePersistedGameId();
            }
        }

        if (getGameSettings().getDynamicReputationRequirement())
        {
            if (setting->getAllowAnyReputation() != origAllowAnyRepGameSetting)
            {
                WARN_LOG("[GameSessionMaster] Cannot change value for GameSetting allowAnyReputation in game(" << getGameId() << ") from " << (origAllowAnyRepGameSetting ? "true" : "false") << "to " << (setting->getAllowAnyReputation()? "true" : "false")  << ", when dynamic reputation requirement is enabled.");
            }
            // if dynamic reputation requirement *newly* enabled, auto init allowAnyReputation. Or else if caller tried changing allowAnyReputation, change it back
            if (!origDynamicRepGameSetting || (setting->getAllowAnyReputation() != origAllowAnyRepGameSetting))
            {
                getGameData().getGameSettings().setAllowAnyReputation(!areAllMembersGoodReputation());
            }
        }

        // to avoid potential title side game server bugs, disallow changing queue capacity updatability flag, post creation
        if (getGameData().getGameSettings().getUpdateQueueCapacityOnReset() != oldSettings.getUpdateQueueCapacityOnReset())
        {
            WARN_LOG("[GameSessionMaster].setGameSettings: Cannot change value for GameSetting UpdateQueueCapacityOnReset in game(" <<
                getGameId() << ") from " << (oldSettings.getUpdateQueueCapacityOnReset() ? "true" : "false") << ". Setting unchanged.");

            getGameData().getGameSettings().setUpdateQueueCapacityOnReset(oldSettings.getUpdateQueueCapacityOnReset());
        }

        mMetricsHandler.onGameSettingsChanged(oldSettings, getGameSettings());

        TRACE_LOG("[GameSessionMaster] Settings for game(" << getGameId() << ") updated, new settings("<< SbFormats::HexLower(setting->getBits()) <<")");

        sendGameSettingsUpdate();

        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());

        // Locked as Busy Timeout exists so that we don't have servers stuck in a busy 'zombie' state.  After (30s) the game switches back to non-busy.
        if (getGameSettings().getLockedAsBusy() && gGameManagerMaster->getConfig().getGameSession().getLockedAsBusyTimeout() > 0)
        {
            // add timer
            if (mGameDataMaster->getLockedAsBusyTimeout() != 0)
            {
                gGameManagerMaster->getLockedAsBusyTimerSet().cancelTimer(getGameId());
                mGameDataMaster->setLockedAsBusyTimeout(0);
            }

            TimeValue timeout = TimeValue::getTimeOfDay() + gGameManagerMaster->getConfig().getGameSession().getLockedAsBusyTimeout();
            if (gGameManagerMaster->getLockedAsBusyTimerSet().scheduleTimer(getGameId(), timeout))
                mGameDataMaster->setLockedAsBusyTimeout(timeout);
        }

        return SetGameSettingsMasterError::ERR_OK;
    }


    void GameSessionMaster::clearLockedAsBusy(GameId gameId)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(gameId);
        if (gameSession != nullptr)
            gameSession->clearLockedAsBusy();
    }

    void GameSessionMaster::clearLockedAsBusy()
    {
        if (mGameDataMaster->getLockedAsBusyTimeout() != 0)
        {
            gGameManagerMaster->getLockedAsBusyTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setLockedAsBusyTimeout(0);
        }

        if (!getGameData().getGameSettings().getLockedAsBusy())
            return;

        TRACE_LOG("[GameSessionMaster] Clearing Locked as Busy setting for game(" << getGameId() << ").");

        GameSettings oldSettings = getGameSettings();
        getGameData().getGameSettings().clearLockedAsBusy();
        mMetricsHandler.onGameSettingsChanged(oldSettings, getGameSettings());

        sendGameSettingsUpdate();
    }



    void GameSessionMaster::sendGameSettingsUpdate()
    {
        NotifyGameSettingsChange nSettings;
        nSettings.setGameId(getGameId());
        nSettings.getGameSettings().setBits(getGameSettings().getBits());
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyGameSettingsChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nSettings );
    }

    void GameSessionMaster::sendPlayerJoinCompleted(const PlayerInfoMaster& playerInfo)
    {
        // send join complete notification for the player.
        NotifyPlayerJoinCompleted joined;
        joined.setGameId(playerInfo.getGameId());
        joined.setPlayerId(playerInfo.getPlayerId());
        joined.setJoinedGameTimestamp(playerInfo.getPlayerData()->getJoinedGameTimestamp());
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyPlayerJoinCompletedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &joined);
    }

    /*! ************************************************************************************************/
    /*! \brief retrieve game attribute value for the configured mode attribute

        \return game attribute value for the given attribute, nullptr if the attribute is not found
    ***************************************************************************************************/
    const char8_t* GameSessionMaster::getGameMode() const
    {
        return getGameAttrib(gGameManagerMaster->getConfig().getGameSession().getGameModeAttributeName());
    }

    const char8_t* GameSessionMaster::getPINGameModeType() const
    {
        const char8_t* modeType = getGameAttrib(gGameManagerMaster->getConfig().getGameSession().getPINGameModeTypeAttributeName());
        return (modeType != nullptr) ? modeType : RiverPoster::PIN_MODE_TYPE;
    }

    const char8_t* GameSessionMaster::getPINGameType() const
    {
        const char8_t* gameType = getGameAttrib(gGameManagerMaster->getConfig().getGameSession().getPINGameTypeAttributeName());
        return (gameType != nullptr) ? gameType : RiverPoster::PIN_GAME_TYPE;
    }

    const char8_t* GameSessionMaster::getPINGameMap() const
    {
        const char8_t* gameAttrib = getGameAttrib(gGameManagerMaster->getConfig().getGameSession().getPINGameMapAttributeName());
        if (gameAttrib == nullptr)
            gameAttrib = "";

        return gameAttrib;
    }

    /*! ************************************************************************************************/
    /*! \brief updates the game's gameAttributeMap (merged the updated attribs into the existing map)
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::setGameAttributes(const Collections::AttributeMap *updatedGameAttributes, uint32_t &updatedAttributeCount)
    {
        updatedAttributeCount = 0;

        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        // Note: this is really more of an attribute merge - we only modify or add attributes (none are removed)
        Collections::AttributeMap &gameAttribMap = getGameData().getGameAttribs();
        Collections::AttributeMap::const_iterator updatedAttribIter = updatedGameAttributes->begin();
        Collections::AttributeMap::const_iterator updatedAttribEnd = updatedGameAttributes->end();
        eastl::string attrStr;
        for ( ; updatedAttribIter != updatedAttribEnd; ++updatedAttribIter )
        {
            if (gameAttribMap.find(updatedAttribIter->first) == gameAttribMap.end())
            {
                // new attribute, this is an update
                ++updatedAttributeCount;
            }
            else if (blaze_strcmp( gameAttribMap[updatedAttribIter->first].c_str(), updatedAttribIter->second.c_str()) != 0)
            {
                // changed attribute
                ++updatedAttributeCount;
                // was this the game mode?
                if (blaze_stricmp(updatedAttribIter->first.c_str(), gGameManagerMaster->getConfig().getGameSession().getGameModeAttributeName()) == 0)
                {
                    // the game mode was changed (game mode will never be 'added' as Blaze inserts it if missing during create/reset)
                    eastl::string platforms = getPlatformForMetrics();
                    mMetricsHandler.onGameModeOrPlatformChanged(getGameMode(), updatedAttribIter->second.c_str(), platforms.c_str(), platforms.c_str());
                }
            }

            gameAttribMap[updatedAttribIter->first] = updatedAttribIter->second;
            if (IS_LOGGING_ENABLED(Logging::TRACE))
            {
                attrStr.append_sprintf("%s=%s ", updatedAttribIter->first.c_str(), updatedAttribIter->second.c_str());
            }
        }

        TRACE_LOG("[GameSessionMaster] Attributes updated for game(" << getGameId() << "), new attributes ( " << attrStr.c_str() << ")");
        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());
        return Blaze::ERR_OK;
    }

    
    /*! ************************************************************************************************/
    /*! \brief updates the game's gameAttributeMap (merged the updated attribs into the existing map)
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::setDedicatedServerAttributes(const Collections::AttributeMap *updatedDSAttributes, uint32_t &updatedAttributeCount)
    {
        updatedAttributeCount = 0;
        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        // Note: this is really more of an attribute merge - we only modify or add attributes (none are removed)
        Collections::AttributeMap &gameAttribMap = getGameData().getDedicatedServerAttribs();
        Collections::AttributeMap::const_iterator updatedAttribIter = updatedDSAttributes->begin();
        Collections::AttributeMap::const_iterator updatedAttribEnd = updatedDSAttributes->end();
        eastl::string attrStr;
        for (; updatedAttribIter != updatedAttribEnd; ++updatedAttribIter)
        {
            ++updatedAttributeCount;
            gameAttribMap[updatedAttribIter->first] = updatedAttribIter->second;
            if (IS_LOGGING_ENABLED(Logging::TRACE))
            {
                attrStr.append_sprintf("%s=%s ", updatedAttribIter->first.c_str(), updatedAttribIter->second.c_str());
            }
        }

        TRACE_LOG("[GameSessionMaster] Dedicated Server Attributes updated for game(" << getGameId() << "), new DS attribute ( " << attrStr.c_str() << ")");
        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());
        return Blaze::ERR_OK;
    }

    
    /*! ************************************************************************************************/
    /*! \brief update the status of two endpoints that connected.

        \param[in] request  - MeshEndpointsConnectedMasterRequest
    ***************************************************************************************************/
    MeshEndpointsConnectedMasterError::Error GameSessionMaster::meshEndpointsConnected(const UpdateMeshConnectionMasterRequest& request)
    {
        if (mGameNetwork == nullptr)
        {
            ERR_LOG("[GameSessionMaster].meshEndpointsConnected: unexpected error - the gameNetwork topology behavior pointer(mGameNetwork) is not initialized.");
            EA_ASSERT(mGameNetwork != nullptr);
            return MeshEndpointsConnectedMasterError::ERR_SYSTEM;
        }
        
        return MeshEndpointsConnectedMasterError::commandErrorFromBlazeError(mGameNetwork->updateClientMeshConnection(request, this));
    }

    /*! ************************************************************************************************/
    /*! \brief update the status of two endpoints that disconnected.

        \param[in] request  - MeshEndpointsDisconnectedMasterRequest
    ***************************************************************************************************/
    MeshEndpointsDisconnectedMasterError::Error GameSessionMaster::meshEndpointsDisconnected(const UpdateMeshConnectionMasterRequest& request)
    {
        if (mGameNetwork == nullptr)
        {
            ERR_LOG("[GameSessionMaster].meshEndpointsDisconnected: unexpected error - the gameNetwork topology behavior pointer(mGameNetwork) is not initialized.");
            EA_ASSERT(mGameNetwork != nullptr);
            return MeshEndpointsDisconnectedMasterError::ERR_SYSTEM;
        }
        
        return MeshEndpointsDisconnectedMasterError::commandErrorFromBlazeError(mGameNetwork->updateClientMeshConnection(request, this));

    }

    /*! ************************************************************************************************/
    /*! \brief update the status of two endpoints that lost connection.

        \param[in] request  - MeshEndpointsConnectionLostMasterRequest
    ***************************************************************************************************/
    MeshEndpointsConnectionLostMasterError::Error GameSessionMaster::meshEndpointsConnectionLost(const UpdateMeshConnectionMasterRequest& request)
    {
        if (mGameNetwork == nullptr)
        {
            ERR_LOG("[GameSessionMaster].meshEndpointsConnectionLost: unexpected error - the gameNetwork topology behavior pointer(mGameNetwork) is not initialized.");
            EA_ASSERT(mGameNetwork != nullptr);
            return MeshEndpointsConnectionLostMasterError::ERR_SYSTEM;
        }
        
        if (doesLeverageCCS())
        {
            // We only want to involve ccs if the connection could never be established. We don't want to act on the connection lost once it was established.
            if (!mPlayerConnectionMesh.areEndpointsConnected(request.getSourceConnectionGroupId(), request.getTargetConnectionGroupId()))
            {
                // If ccs trigger was not received yet, this is the first time we are seeing this. So we go ahead and submit a request to use ccs.
                // Otherwise, we are seeing this in response to the connection to the hosting server and we update the GameNetwork.  
                if (!mPlayerConnectionMesh.getCCSTriggerWasReceived(request.getSourceConnectionGroupId(), request.getTargetConnectionGroupId()))
                {
                    TRACE_LOG("[GameSessionMaster].meshEndpointsConnectionLost: game("<< getGameId() <<"). CCS trigger received for the source,target pair: ("<<request.getSourceConnectionGroupId()<<","<<request.getTargetConnectionGroupId()<<")");
                    mPlayerConnectionMesh.setCCSTriggerWasReceived(request.getSourceConnectionGroupId(), request.getTargetConnectionGroupId(), CC_HOSTEDFALLBACK_MODE);

                    CCSRequestPair requestPair; 
                    requestPair.setConsoleFirstConnGrpId(request.getSourceConnectionGroupId());
                    requestPair.setConsoleSecondConnGrpId(request.getTargetConnectionGroupId());

                    addPairToPendingCCSRequests(requestPair);

                    return MeshEndpointsConnectionLostMasterError::ERR_OK;
                }
            }

            GameDataMaster::ClientConnectionDetails* connDetails = mPlayerConnectionMesh.getConnectionDetails(request.getSourceConnectionGroupId(), request.getTargetConnectionGroupId());
            if (connDetails != nullptr && connDetails->getCCSInfo().getCCSRequestInProgress())
            {
                TRACE_LOG("[GameSessionMaster].meshEndpointsConnectionLost: game("<< getGameId() <<"). Skip ConnectionLost processing because the CCS request is in progress for the source,target pair: ("<<request.getSourceConnectionGroupId()<<","<<request.getTargetConnectionGroupId()<<")");
                return MeshEndpointsConnectionLostMasterError::ERR_OK;
            }
        }

        return MeshEndpointsConnectionLostMasterError::commandErrorFromBlazeError(mGameNetwork->updateClientMeshConnection(request, this));
    }

    /*! ************************************************************************************************/
    /*! \brief validates a setPlayerCapacity request and applies it to the game session.
    \param[in] request - the request
    \param[out] newTeamRosters - the membership of the teams in the game after processing the request.
    \param[out] errorInfo - details on errors related to the player roster
    \return SetPlayerCapacityMasterError::Error
    ***************************************************************************************************/
    SetPlayerCapacityMasterError::Error GameSessionMaster::processSetPlayerCapacityMaster( const SetPlayerCapacityRequest &request, TeamDetailsList& newTeamRosters, SwapPlayersErrorInfo &errorInfo )
    {
        BlazeRpcError err = gGameManagerMaster->getGameStateHelper()->isActionAllowed(this, GAME_CAPACITY_RESIZE);
        if (err != ERR_OK)
        {
            TRACE_LOG("[GameSessionMaster] Invalid action, capacity resize not allowed for game(" << getGameId() << ")");
            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Cannot change game capacity in game state '%s', try again later.", GameStateToString(getGameState()));
            errorInfo.setErrMessage(msg);
            return SetPlayerCapacityMasterError::commandErrorFromBlazeError(err);
        }

        // do a teams rebuild if team info was specified
        bool needsTeamRebuild = !request.getTeamDetailsList().empty();

        // validate the new settings against the game.
        const SlotCapacitiesVector& newSlotCapacities = request.getSlotCapacities();

        err = validateSlotCapacities(newSlotCapacities);
        if (err != ERR_OK)
        {
            // total the slot capacities for error purposes
            uint16_t requestedTotalSlotCapacity = 0;
            for (uint32_t i = 0; i < MAX_SLOT_TYPE; ++i)
            {
                SlotType slot = (SlotType)i;
                requestedTotalSlotCapacity += newSlotCapacities[slot];
            }

            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            switch (err)
            {
                case GAMEMANAGER_ERR_PLAYER_CAPACITY_IS_ZERO:
                    {
                        blaze_snzprintf(msg, sizeof(msg), "Cannot reduce game capacity to 0.");
                        break;
                    }
                case GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_SMALL:
                    {
                        blaze_snzprintf(msg, sizeof(msg), "Cannot reduce game capacity to '%" PRIu16 "', minimum for this game session is '%" PRIu16 "'.", requestedTotalSlotCapacity, getMinPlayerCapacity());
                        break;
                    }
                case GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_LARGE:
                    {
                        blaze_snzprintf(msg, sizeof(msg), "Cannot increase game capacity to '%" PRIu16 "', maximum for this game session is '%" PRIu16 "'.", requestedTotalSlotCapacity, getMaxPlayerCapacity());
                        break;
                    }
                default:
                    {
                        blaze_snzprintf(msg, sizeof(msg), "Cannot change game capacity due to error '%s'.", ErrorHelp::getErrorName(err));
                    }
            }

            errorInfo.setErrMessage(msg);
            return SetPlayerCapacityMasterError::commandErrorFromBlazeError(err);
        }

        //individual slot capacity cannot be smaller than the current number of players in that slot type
        // also total new slot size
        uint16_t newTotalSlotCapacity = 0;
        uint16_t newTotalParticipantSlotCapacity = newSlotCapacities[SLOT_PUBLIC_PARTICIPANT] + newSlotCapacities[SLOT_PRIVATE_PARTICIPANT];
        for (uint32_t i = 0; i < MAX_SLOT_TYPE; ++i)
        {
            SlotType slot = (SlotType)i;
            newTotalSlotCapacity = newTotalSlotCapacity + newSlotCapacities[slot];
            if (mPlayerRoster.getPlayerCount(slot) > newSlotCapacities[slot])
            {
                TRACE_LOG("[GameSessionMaster] cannot set SlotCapacity[" <<  SlotTypeToString(slot) << "] to " << newSlotCapacities[slot] << " for game("
                          << getGameId() << "); game already contains " << mPlayerRoster.getPlayerCount(slot) << " players in that slot type.");

                errorInfo.setSlotType(slot);
                char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Failed to reduce capacity of slot type '%s' to '%" PRIu16 "', there are '%" PRIu16 "' players occupying those slots.",
                    SlotTypeToString(slot), newSlotCapacities[slot], mPlayerRoster.getPlayerCount(slot));
                errorInfo.setErrMessage(msg);

                return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_SMALL;
            }
        }
        uint16_t teamCount = getTeamCount();

        // if no teams or membership have been specified in the request, use the current team count
        uint16_t newTeamCount = request.getTeamDetailsList().empty() ? getTeamCount() : request.getTeamDetailsList().size();

        // newTeamCount should never be 0, since that would imply that the game currently has 0 teams.
        if ((newTeamCount != 0) && ((newTotalParticipantSlotCapacity % newTeamCount) != 0))
        {
            TRACE_LOG("[GameSessionMaster] cannot set total player SlotCapacity to (" << newTotalParticipantSlotCapacity << ") for game("
                << getGameId() << "); with " << newTeamCount << " teams, player capacity was not evenly divisible.");
            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Failed validation of new team capacity. '%" PRIu16 "' teams cannot evenly divide '%" PRIu16 "' participant slots.",
                newTeamCount, newTotalParticipantSlotCapacity);
            errorInfo.setErrMessage(msg);
            return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_PLAYER_CAPACITY_NOT_EVENLY_DIVISIBLE_BY_TEAMS;
        }


        uint16_t newTeamCapacity = (newTeamCount != 0) ? (newTotalParticipantSlotCapacity / newTeamCount) : 0;

        // we need to make a copy of the role information because if the request's wasn't present, we supply defaults
        RoleInformation newRoleInformation;

        if (!request.getRoleInformation().getRoleCriteriaMap().empty())
        {
            request.getRoleInformation().copyInto(newRoleInformation);
        }
        else
        {
            // we require the role criteria, if none was supplied, put in some defaults
            setUpDefaultRoleInformation(newRoleInformation, newTeamCapacity);
        }

        err = validateRoleInformation(newRoleInformation, newTeamCapacity);
        if (err != ERR_OK)
        {
            return SetPlayerCapacityMasterError::commandErrorFromBlazeError(err);
        }


        // time to check teams
        // if the number of teams has changed, rebuild teams
        if ((teamCount != newTeamCount))
        {
            // need to re-arrange players
            TRACE_LOG("[GameSessionMaster] Team count for game(" << getGameId()
                << "); changed from " << teamCount << " to " << newTeamCount << ", need to rebalance players.");

            needsTeamRebuild = true;
        }

        if (teamCount != 0)
        {

            // validate individual teams and team sizes vs game's roster
            for(uint16_t i = 0; i < teamCount; ++i)
            {
                // already validated that the game has enough room in total for all players, just checking if any teams have more players than capacity
                if (mPlayerRoster.getTeamSize(i) > newTeamCapacity)
                {
                    TRACE_LOG("[GameSessionMaster] TeamCapacity for team (index " << i << ") to " << newTeamCapacity << " for game(" << getGameId()
                              << "); game already contains " << mPlayerRoster.getTeamSize(i) << " players in that team, need to rebalance players.");
                    needsTeamRebuild = true;
                }
            }
        }

        if (newTeamCount > 1)
        {
            // validate that explicitly placed players aren't overloading any teams
            // we don't need to check if there's only one team, because we've already validated the overall game capacity at that point
            eastl::set<TeamId> teamIdSet;
            TeamDetailsList::const_iterator newTeamIter = request.getTeamDetailsList().begin();
            TeamDetailsList::const_iterator newTeamEnd = request.getTeamDetailsList().end();
            for(uint16_t i = 0; newTeamIter != newTeamEnd; ++i, ++newTeamIter)
            {
                TeamId newTeamId = (*newTeamIter)->getTeamId();
                uint16_t newTeamSize = (*newTeamIter)->getTeamRoster().size();
                if (newTeamSize > newTeamCapacity)
                {
                    TRACE_LOG("[GameSessionMaster] Cannot assign [" <<  newTeamSize << "] to team at index (" << i << ") for game("
                        << getGameId() << "); with team capacity" << newTeamCapacity << " too many players.");

                    errorInfo.setTeamIndex(i);
                    char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                    blaze_snzprintf(msg, sizeof(msg), "Team at index '%" PRIu16 "' is over capacity. Failed assignment of '%" PRIu16 "' players to '%" PRIu16 "' spots.",
                        i, newTeamSize, newTeamCapacity);
                    errorInfo.setErrMessage(msg);

                    return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_TEAM_FULL;
                }
                // if the duplicate team flag isn't set, verify no duplicates (except ANY_TEAM_ID)
                if (newTeamId != ANY_TEAM_ID && !getGameSettings().getAllowSameTeamId() &&
                    (teamIdSet.find(newTeamId) != teamIdSet.end()))
                {
                    TRACE_LOG("[GameSessionMaster] cannot insert TeamId (" << newTeamId << ") for game("
                        << getGameId() << "); when the GameSettings::allowSameTeamId flag is false.");

                    errorInfo.setTeamIndex(i);
                    char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                    blaze_snzprintf(msg, sizeof(msg), "Failed validation of TeamId assignments. Team at index '%" PRIu16 "' duplicates TeamId of '%" PRIu16 "' already in the game, and GameSettings::getAllowSameTeamId() is false.",
                        i, newTeamId);
                    errorInfo.setErrMessage(msg);

                    return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_DUPLICATE_TEAM_CAPACITY;
                }
                teamIdSet.insert(newTeamId);
            }
        }

        EntryCriteriaName failedCriteria;
        RoleCriteriaMap::const_iterator itr = newRoleInformation.getRoleCriteriaMap().begin();
        RoleCriteriaMap::const_iterator itrEnd = newRoleInformation.getRoleCriteriaMap().end();
        for (; itr != itrEnd; ++itr)
        {
            if (!EntryCriteriaEvaluator::validateEntryCriteria(itr->second->getRoleEntryCriteriaMap(),  failedCriteria))
            {
                // Failed:
                TRACE_LOG("[GameSessionMaster].processSetPlayerCapacityMaster(): Role entry criteria (" << failedCriteria.c_str() << ") was invalid.");

                errorInfo.setRoleName(itr->first);
                char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Failed validation of Roles. Role '%s' defined invalid entry criteria '%s'.",
                    itr->first.c_str(), failedCriteria.c_str());
                errorInfo.setErrMessage(msg);

                return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_ROLE_CRITERIA_INVALID;
            }
        }

        // Rather than recreate the entry criteria multiple times, we just store the criteria generated by validateEntryCriteria
        ExpressionMap newMultiroleRules;

        // Check that new rules are valid:
        if (!MultiRoleEntryCriteriaEvaluator::validateEntryCriteria(newRoleInformation, failedCriteria, &newMultiroleRules))
        {
            // Failed:
            TRACE_LOG("[GameSessionMaster].processSetPlayerCapacityMaster(): Multirole entry criteria (" << failedCriteria.c_str() << ") was invalid.");

            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Failed validation of Roles. Multi-role criteria with name '%s' was invalid.",
                failedCriteria.c_str());
            errorInfo.setErrMessage(msg);

            return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_INVALID;
        }

        if (needsTeamRebuild)
        {
            if (request.getTeamDetailsList().empty())
            {
                // need to add the team ids to a temp list because teams weren't changed, but have shrunk
                // an empty TeamDetailsList will wipe the existing teams from the game!
                TeamDetailsList tempTeamDetailsList;
                tempTeamDetailsList.reserve(getTeamCount());
                for(TeamIndex teamIndex = 0; teamIndex < (TeamIndex)getTeamCount(); ++teamIndex)
                {
                    tempTeamDetailsList.pull_back()->setTeamId(getTeamIds()[teamIndex]);
                }
                generateRebuiltTeams(tempTeamDetailsList, newTeamRosters, newTeamCapacity);
            }
            else
            {
                // use the team details list from the request
                generateRebuiltTeams(request.getTeamDetailsList(), newTeamRosters, newTeamCapacity);
            }
        }

        // Test Role entry criteria:
        {
            MultiRoleEntryCriteriaEvaluator evaluator;
            evaluator.setStaticRoleInfoSource(newRoleInformation);
            evaluator.setEntryCriteriaExpressions(newMultiroleRules);

            RoleSizeMap tempSizeMap;    // size map used with newTeamCapacity

            // Check teams against the role rules
            TeamIndex curIndex = 0;
            TeamIndex maxIndex = needsTeamRebuild ? newTeamRosters.size() : getTeamCount();
            for (; curIndex < maxIndex; ++curIndex)
            {
                if (needsTeamRebuild)
                {
                    tempSizeMap.clear();

                    // Build a RoleSizeMap for the current team:
                    const TeamMemberInfoList & infoList = newTeamRosters[curIndex]->getTeamRoster();
                    TeamMemberInfoList::const_iterator curIter = infoList.begin();
                    TeamMemberInfoList::const_iterator endIter = infoList.end();

                    for (; curIter != endIter; ++curIter)
                    {
                        ++tempSizeMap[(*curIter)->getPlayerRole()];
                    }
                }

                // Check that every player's role exists in the new update:
                RoleSizeMap rosterRoleSizeMap;
                RoleSizeMap* roleSizeMap = &tempSizeMap;
                if (!needsTeamRebuild)
                {
                    getPlayerRoster()->getRoleSizeMap(rosterRoleSizeMap, curIndex);
                    if (rosterRoleSizeMap.empty())
                        continue;
                    roleSizeMap = &rosterRoleSizeMap;
                }

                RoleMap::const_iterator curIter = roleSizeMap->begin();
                RoleMap::const_iterator endIter = roleSizeMap->end();
                for (; curIter != endIter; ++curIter)
                {
                    RoleCriteriaMap::const_iterator iterNewMap = newRoleInformation.getRoleCriteriaMap().find(curIter->first);
                    if ((curIter->second > 0) && (iterNewMap == newRoleInformation.getRoleCriteriaMap().end()))
                    {
                        // Role no longer exists, fail.
                        TRACE_LOG("[GameSessionMaster].processSetPlayerCapacityMaster(): Role " << curIter->first.c_str() << " does not exist in new role information.");

                        errorInfo.setRoleName(curIter->first);
                        errorInfo.setTeamIndex(curIndex);
                        char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                        blaze_snzprintf(msg, sizeof(msg), "'%" PRIu16 "' players on team at index '%" PRIu16 "' failed assignment to Role '%s', which is not valid for this game session.",
                            curIter->second, curIndex, curIter->first.c_str());

                        errorInfo.setErrMessage(msg);

                        return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
                    }

                    // If the role capacity is exceeded by the current roles used:
                    if ((iterNewMap != newRoleInformation.getRoleCriteriaMap().end()) && (curIter->second > iterNewMap->second->getRoleCapacity()))
                    {
                        TRACE_LOG("[GameSessionMaster].processSetPlayerCapacityMaster(): Existing role " << curIter->first.c_str() << " capacity is exceeded "
                                  "with new role information (Current: " << curIter->second << " > New Limit: " << iterNewMap->second->getRoleCapacity() << "." );

                        errorInfo.setRoleName(curIter->first);
                        errorInfo.setTeamIndex(curIndex);
                        char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                        blaze_snzprintf(msg, sizeof(msg), "Role '%s' is full on team at index '%" PRIu16 "'. Failed assignment of '%" PRIu16 "' players to '%" PRIu16 "' spots.",
                            curIter->first.c_str(), curIndex, curIter->second, iterNewMap->second->getRoleCapacity());

                        errorInfo.setErrMessage(msg);

                        return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_ROLE_FULL;
                    }
                }

                if (!evaluator.evaluateEntryCriteria(*roleSizeMap, failedCriteria))
                {
                    TRACE_LOG("[GameSessionMaster].processSetPlayerCapacityMaster(): Multirole entry criteria (" << failedCriteria.c_str() << ") failed for team index " << curIndex << ".");

                    errorInfo.setTeamIndex(curIndex);
                    char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                    blaze_snzprintf(msg, sizeof(msg), "Team at index '%" PRIu16 "', failed validation due to multirole criteria '%s'.",
                        curIndex, failedCriteria.c_str());
                    errorInfo.setErrMessage(msg);

                    return SetPlayerCapacityMasterError::GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_FAILED;
                }
            }
        }


        // All tests have completed, and now we can update the actual values used in the game:
        const uint16_t oldTotalParticipantCapacity = getTotalParticipantCapacity();
        const uint16_t oldTotalSpectatorCapacity = getTotalSpectatorCapacity();
        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        newSlotCapacities.copyInto(getGameData().getSlotCapacities());

        // Update the role information and criteria used by the game:
        newRoleInformation.copyInto(getGameData().getRoleInformation());
        updateRoleEntryCriteriaEvaluators();

        if (needsTeamRebuild)
        {
            finalizeRebuiltTeams(request.getTeamDetailsList(), newTeamRosters, newTeamCapacity);
        }

        mMetricsHandler.onPlayerCapacityUpdated(oldTotalParticipantCapacity, getTotalParticipantCapacity(), oldTotalSpectatorCapacity, getTotalSpectatorCapacity());
        
        // if fullness changed, update external session props. Handles roles fullness changes here also.
        // side: dequeues may also update external session, but done after, separate from here for flow-consistency.
        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());

        return SetPlayerCapacityMasterError::ERR_OK;
    }

    void GameSessionMaster::processQueueAfterSetPlayerCapacity()
    {
        // if we're enlarging the game, pump the queue
        if (getTotalPlayerCapacity() > mPlayerRoster.getRosterPlayerCount())
        {
            // pump the queue for both types of managed queues.
            processPlayerQueue();
        }
    }

    /*! ************************************************************************************************/
    /*! \brief  Helper to pump the player queue depending on server configuration.  Should only be called
        in places where all types of queue management need to pump the queue
    ***************************************************************************************************/
    void GameSessionMaster::processPlayerQueue()
    {
        if (gGameManagerMaster->getGameSessionQueueMethod() == GAME_QUEUE_ADMIN_MANAGED)
        {
            processPlayerAdminManagedQueue();
        }
        else if (gGameManagerMaster->getGameSessionQueueMethod() == GAME_QUEUE_BLAZE_MANAGED)
        {
            processPlayerServerManagedQueue();
        }
    }

    bool GameSessionMaster::shouldPumpServerManagedQueue() const
    {
        if (gGameManagerMaster->getGameSessionQueueMethod() == GAME_QUEUE_ADMIN_MANAGED)
        {
            return false;
        }

        // any new slots available?
        uint16_t rosterPlayers = mPlayerRoster.getRosterPlayerCount();
        uint16_t newSlots = getTotalPlayerCapacity() - rosterPlayers;
        uint16_t reservedPlayers = rosterPlayers - (uint16_t)mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PLAYERS).size();

        // Can not join when the game is in progress; even from the queue.
        if (!getGameSettings().getJoinInProgressSupported() && isGameInProgress())
        {
            TRACE_LOG("[GameSessionMaster] Not allowing queued players to join game in progress. " << newSlots << " slots available. (" << reservedPlayers << " reserved)");
            return false;
        }

        // Early out if nobody in the queue or the game is still full
        if (!mPlayerRoster.hasQueuedPlayers() || (newSlots == 0 && (!getGameSettings().getAutoDemoteReservedPlayers() || reservedPlayers == 0)))
        {
            TRACE_LOG("[GameSessionMaster] No reason to pump the player queue at this time. " << newSlots << " slots available. (" << reservedPlayers << " reserved)");
            return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief pump the game's player queue, possibly causing queued players to join the game.
            Note: caller's responsibility to only call this when needed (should check that there are available slots)
    ***************************************************************************************************/
    void GameSessionMaster::processPlayerServerManagedQueue()
    {
        if (gGameManagerMaster->getGameSessionQueueMethod() != GAME_QUEUE_ADMIN_MANAGED)
        {
            decrementQueuePumpsPending();
            // marks game so new joiners can't jump the queue
            updatePromotabilityOfQueuedPlayers();
        }

        if (!shouldPumpServerManagedQueue())
            return;

        uint16_t rosterPlayers = mPlayerRoster.getRosterPlayerCount();
        uint16_t newSlots = getTotalPlayerCapacity() - rosterPlayers;
        uint16_t reservedPlayers = rosterPlayers - (uint16_t)mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PLAYERS).size();

        PlayerRoster::PlayerInfoList qPlayerList = mPlayerRoster.getPlayers(PlayerRoster::QUEUED_PLAYERS);

        if (getGameSettings().getAutoDemoteReservedPlayers())
        {
            // Demote non-reserved players first:
            PlayerRoster::PlayerInfoList tempPlayerList = qPlayerList;
            qPlayerList.clear();

            PlayerRoster::PlayerInfoList::const_iterator it = tempPlayerList.begin();
            for (; it != tempPlayerList.end(); ++it)
            {
                if (!(*it)->isReserved())
                    qPlayerList.push_back(*it);
            }
            for (it = tempPlayerList.begin(); it != tempPlayerList.end(); ++it)
            {
                if ((*it)->isReserved())
                    qPlayerList.push_back(*it);
            }
        }

        TRACE_LOG("[GameSessionMaster] Server pumping player queue. " << newSlots << " slots available.");

        PlayerRoster::PlayerInfoList::const_iterator it = qPlayerList.begin();
        while (it != qPlayerList.end() && (newSlots > 0 || (getGameSettings().getAutoDemoteReservedPlayers() && reservedPlayers > 0)))
        {
            PlayerInfoMaster *qPlayer = static_cast<PlayerInfoMaster*>(*it);
            ++it; // increment the iterator since we may delete them below.

            SlotType slotType = qPlayer->getSlotType();
            // dump next queued player into game if there are additional public slots
            // or if they are requesting the type of slot that was opened
            bool desiredSlotFull = isSlotTypeFull(slotType);
            // make sure we're checking the correct desired slot type
            bool publicSlotFull = qPlayer->isParticipant() ? isSlotTypeFull(SLOT_PUBLIC_PARTICIPANT) : isSlotTypeFull(SLOT_PUBLIC_SPECTATOR);
            if (!desiredSlotFull || !publicSlotFull)
            {
                --newSlots;

                if (addPlayerFromQueue(*qPlayer) != ERR_OK)
                {
                    // The add failed, leave the slot open for the next player as this one has been removed.
                    ++newSlots;
                }
            }
            else if (getGameSettings().getAutoDemoteReservedPlayers())
            {
                // If the player is reserved, see if there's anyone that we can demote so that this player can take their place: 
                PlayerId playerToDemote = INVALID_BLAZE_ID;
                if (canQueuedPlayerSwapWithReservedInGamePlayer(qPlayer, &playerToDemote))
                {
                    PlayerInfoMaster* playerInfo = getPlayerRoster()->getPlayer(playerToDemote);
                    BlazeRpcError err = GameSessionMaster::demotePlayerToQueue(*playerInfo, false);  // Skip the capacity check, since we're promoting a player immediately.
                    if (err != ERR_OK)
                    {
                        WARN_LOG("[GameSessionMaster]::processPlayerServerManagedQueue - Failed to demote a player.  This is unexpected.");
                    }
                    else
                    {
                        --reservedPlayers;
                        if (addPlayerFromQueue(*qPlayer) != ERR_OK)
                        {
                            // This shouldn't happen. We should already have verified that the player would fit when we called canQueuedPlayerSwapWithReservedInGamePlayer.
                            WARN_LOG("[GameSessionMaster]::processPlayerServerManagedQueue - Failed to add queued player after another player was demoted.  This is unexpected.");

                            // Try to undo the demotion, if that fails we're in a bad spot.
                            playerInfo = getPlayerRoster()->getPlayer(playerToDemote);
                            err = GameSessionMaster::addPlayerFromQueue(*playerInfo);
                            if (err != ERR_OK)
                            {
                                ERR_LOG("[GameSessionMaster]::processPlayerServerManagedQueue - Failed to add queued player, or repromote player.  The queue is now in a bad state. (Count may exceed max).");
                            }


                            // The add failed, leave the slot open for the next player as this one has been removed.
                            ++reservedPlayers;
                        }
                    }
                }
            }
        }

        // All players that are available to be promoted have already been added via addPlayerFromQueue.
        mGameDataMaster->setIsAnyQueueMemberPromotable(false);
    }

    void GameSessionMaster::processPlayerAdminManagedQueue()
    {
        if (gGameManagerMaster->getGameSessionQueueMethod() == GAME_QUEUE_BLAZE_MANAGED)
        {
            return;
        }

        if (!isQueueEnabled() || !mPlayerRoster.hasQueuedPlayers())
        {
            //queue is empty, no-op
            return;
        }

        // increment counter, to be decremented on admin managed dequeue
        incrementQueuePumpsPending();
        // marks game so new joiners can't jump the queue
        updatePromotabilityOfQueuedPlayers();


        // Notify admin/host sessions that the queue can be pumped.
        NotifyProcessQueue notification;
        notification.setGameId(getGameId());

        // Send individual messages for each removal so the client can handle them sequentially and process
        // all open slots.
        if (hasDedicatedServerHost())
        {
            // Always send to dedicated user
            gGameManagerMaster->sendNotifyProcessQueueToUserSessionById(getDedicatedServerHostSessionId(), &notification);
        }

        for (PlayerIdList::const_iterator iter = getAdminPlayerList().begin(), end = getAdminPlayerList().end(); iter != end; ++iter)
        {
            PlayerId adminPlayerId = *iter;
            PlayerInfo *admin = getPlayerRoster()->getPlayer(adminPlayerId);
            if (admin != nullptr)
            {
                gGameManagerMaster->sendNotifyProcessQueueToUserSessionById(admin->getPlayerSessionId(), &notification);
            }
        }
    }

    // This function updates the unrestricted platform set used by the Game.
    // The unrestricted platform set is used to determine which platforms can join into the Game.
    // The set has to be updated dynamically, since we may have cases like [pc,xone,ps4] where pc can play with either console, but the consoles can't play together.
    void GameSessionMaster::refreshDynamicPlatformList()
    {
        eastl::string oldPlatform = getPlatformForMetrics();

        ClientPlatformTypeList curPlatforms;
        // we use PlayerRoster::ALL_PLAYERS because we can't invalidate a queued player's ability to be promoted from the queue (or violate their platform restrictions.)
        PlayerRoster::PlayerInfoList playerInfoList = getPlayerRoster()->getPlayers(PlayerRoster::ALL_PLAYERS);
        
        // Only update the list if someone is in the Game:
        if (playerInfoList.empty())
        {
            // clear this out, and we'll copy over the base platform list below
            getGameData().getCurrentlyAcceptedPlatformList().clear();
        }

        // Get the base override list (if it exists):
        if (!getGameData().getBasePlatformList().empty())
        {
            for (auto curPlat : getGameData().getBasePlatformList())
                curPlatforms.insertAsSet(curPlat);
        }

        // If crossplay is disabled, skip the dynamic update:
        if (curPlatforms.size() != 1)
        {
            for (auto player : playerInfoList)
            {
                // Otherwise, remove all platforms that are restricted (based on the current players):
                const ClientPlatformSet& curSet = gUserSessionManager->getUnrestrictedPlatforms(player->getPlatformInfo().getClientPlatform());
                if (curPlatforms.empty())
                {
                    // This is the code that lets the dedicated servers support all hosted platforms in the back-compat case:
                    for (auto curPlat : gController->getHostedPlatforms())
                        curPlatforms.insertAsSet(curPlat);
                }
                else
                {
                    for (ClientPlatformTypeList::iterator curPlatIter = curPlatforms.begin(); curPlatIter != curPlatforms.end();)
                    {
                        if (curSet.find(*curPlatIter) == curSet.end())
                        {
                            TRACE_LOG("[GameSessionMaster] removing previously allowed platform (" << ClientPlatformTypeToString(*curPlatIter) <<
                                ") from the game(" << getGameId() << ") due to global user sessions restrictions of in-game players.");
                            curPlatIter = curPlatforms.eraseAsSet(*curPlatIter);
                        }
                        else
                            ++curPlatIter;
                    }
                }
            }
        }

        //refresh is_crossplay_game whenever a new player joins
        refreshPINIsCrossplayGameFlag();

        // New restrictions are only set if old ones were missing, or the crossplay status is the same:
        bool newRestrictionCpDisable = (curPlatforms.size() == 1);
   //     bool oldRestrictionCpDisable = (getGameData().getClientPlatformList().size() == 1);

   
   //     bool oldRestrictionCpDisable = (getGameData().getCurrentlyAcceptedPlatformList().size() == 1);

        curPlatforms.copyInto(getGameData().getCurrentlyAcceptedPlatformList());

        // If we only support a single platform, we also update the overrides to ensure that the Game won't ever allow Crossplay. 
        if (newRestrictionCpDisable)
            curPlatforms.copyInto(getGameData().getBasePlatformList());

        eastl::string newPlatform = getPlatformForMetrics();

        mMetricsHandler.onGameModeOrPlatformChanged(getGameMode(), getGameMode(), oldPlatform.c_str(), newPlatform.c_str());
        if (oldPlatform != newPlatform)
        {
            gGameManagerMaster->removeGameFromCensusCache(*this);
            gGameManagerMaster->addGameToCensusCache(*this);
        }
    }


    struct UserGroupTargetPlayerEquals
    {
        UserGroupTargetPlayerEquals(UserGroupId userGroupId) : mUserGroupId(userGroupId) {}
        bool operator()(const GameDataMaster::UserGroupTargetPlayer* info) const { return mUserGroupId == info->getUserGroupId(); }
        UserGroupId mUserGroupId;
    };

    struct UserGroupQueuedPlayersCountEquals
    {
        UserGroupQueuedPlayersCountEquals(UserGroupId userGroupId) : mUserGroupId(userGroupId) {}
        bool operator()(const GameDataMaster::UserGroupQueuedPlayersCount* info) const { return mUserGroupId == info->getUserGroupId(); }
        UserGroupId mUserGroupId;
    };  


    /*! ************************************************************************************************/
    /*! \brief adds the given queued player to the game.

    \returns ERR_OK if the player was added to the game from the queue, false if not.
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameSessionMaster::addPlayerFromQueue( PlayerInfoMaster& queuedPlayer, TeamIndex overrideTeamIndex /*= UNSPECIFIED_TEAM_INDEX*/, const char8_t *overrideRoleName /*= nullptr*/ )
    {
        if (mPlayerRoster.getQueuePlayer(queuedPlayer.getPlayerId()) == nullptr)
        {
            TRACE_LOG("[GameSessionMaster] Invalid attempt to add non-queued player(" <<
                queuedPlayer.getPlayerId() << ") to game(" << getGameId() << ")");
            return GAMEMANAGER_ERR_PLAYER_NOT_IN_QUEUE;
        }

        if (getGameState() == MIGRATING)
        {
            TRACE_LOG("[GameSessionMaster] Unable to add player(" << queuedPlayer.getPlayerId() <<
                ") from the queue to game(" << getGameId() << ") while the game is migrating.");
            return GAMEMANAGER_ERR_DEQUEUE_WHILE_MIGRATING;
        }

        // Can not join when the game is in progress; even from the queue.
        if (!getGameSettings().getJoinInProgressSupported() && isGameInProgress())
        {
            TRACE_LOG("[GameSessionMaster] Not allowing queued player(" << queuedPlayer.getPlayerId() <<
                ") to join game(" << getGameId() << ") while it is in progress.");
            return GAMEMANAGER_ERR_DEQUEUE_WHILE_IN_PROGRESS;
        }

        // Make sure our slot type matches what is open.
        SlotType actualSlotType = SLOT_PUBLIC_PARTICIPANT;
        
        // Make sure our team id fits into the game.
        bool joiningTeamSet = false;
        TeamIndex joiningTeamIndex = queuedPlayer.getTeamIndex();
        if (overrideTeamIndex != UNSPECIFIED_TEAM_INDEX)
        {
            if (overrideTeamIndex != joiningTeamIndex &&
                !hasPermission(UserSession::getCurrentUserBlazeId(), GameAction::SET_OTHER_PLAYER_TEAM_INDEX))
            {
                TRACE_LOG("[GameSessionMaster].addPlayerFromQueue(): User attempting change queued player's team index does not have SET_OTHER_PLAYER_TEAM_INDEX permission.");
                return GAMEMANAGER_ERR_PERMISSION_DENIED;
            }

            joiningTeamIndex = overrideTeamIndex;
            joiningTeamSet = true;
        }
        else if (joiningTeamIndex == TARGET_USER_TEAM_INDEX)
        {
            // We attempt to determine the joingingTeamIndex in the logic below.  We default to UNSPECIFIED_TEAM_INDEX here
            // in case we can't find a targeted player that has a valid TeamIndex.
            joiningTeamIndex = UNSPECIFIED_TEAM_INDEX;

            // This loop deals with the case when p1 is targeting p2, who is in-turn targeting p3, etc. etc., where the chain finally
            // ends with pN who is either not in the queue or has a known TeamIndex.  The loop is limited to a depth of 10 iterations
            // in order to avoid an infinite loop which could be caused by p1 who targets p2 who targets p1 (or similar).
            PlayerInfoMaster* targetPlayerInfoMaster = mPlayerRoster.getPlayer(queuedPlayer.getTargetPlayerId());
            for (int32_t counter = 0; (counter < 10) && (targetPlayerInfoMaster != nullptr); ++counter)
            {
                if (!targetPlayerInfoMaster->isInQueue() ||
                    ((targetPlayerInfoMaster->getTeamIndex() != TARGET_USER_TEAM_INDEX) && (targetPlayerInfoMaster->getTeamIndex() != UNSPECIFIED_TEAM_INDEX)))
                {
                    joiningTeamIndex = targetPlayerInfoMaster->getTeamIndex();
                    joiningTeamSet = true;
                    break;
                }

                // Find whoever this guy is targeting, if anyone.
                targetPlayerInfoMaster = mPlayerRoster.getPlayer(targetPlayerInfoMaster->getTargetPlayerId());
            }
        }

        // If the player's team is not being overridden and he did not join the game targeting a specific player, we make another attempt to put him on a particular
        // team in case of a group join. We do this in the queue processing here because if the target player switched the team while others were in the queue, we want others to use his new team.
        // We also want the first suitable player who can join the game have a chance of becoming the target player rather than just the leader.
        const UserGroupId queuedPlayerGroupId = queuedPlayer.getPlayerDataMaster()->getOriginalUserGroupId();
        const PlayerId queuedPlayerId = queuedPlayer.getPlayerId();
        bool provisionalFollowThisPlayer = false; 
        if (!joiningTeamSet && getGameType() == GAME_TYPE_GAMESESSION) 
        {
            if (queuedPlayerGroupId != EA::TDF::OBJECT_ID_INVALID) // If the player joined as part of a group
            {
                bool foundExistingGroup = false;
                PlayerInfoMaster* targetPlayerInfoMaster = getTargetPlayer(queuedPlayerGroupId, foundExistingGroup);
                if (targetPlayerInfoMaster != nullptr)
                {
                    joiningTeamIndex = targetPlayerInfoMaster->getTeamIndex();
                    joiningTeamSet = true;
                }
                else
                {
                    if (foundExistingGroup)
                    {
                        // If we found a group but the target player is null, target player has left the game. In that case, we let the players go to any available spot. 
                        // This is as designed. Do not erase the groupId from the list yet as that would otherwise make the next player to pop up as the target player.
                        joiningTeamIndex = UNSPECIFIED_TEAM_INDEX;
                        joiningTeamSet = true;
                    }
                    else
                    {
                        // If we did not find any existing group (for example, entire group might have queued and as a result, we don't have a target player yet), we make this player, who is first to 
                        // pop out as a player rest can follow pending he can join the game successfully.
                        provisionalFollowThisPlayer = true;
                    }
                }
            }
        }

        RoleName joiningRoleName = overrideRoleName ? overrideRoleName : queuedPlayer.getRoleName();
        if (joiningRoleName != queuedPlayer.getRoleName() &&
            !hasPermission(UserSession::getCurrentUserBlazeId(), GameAction::SET_OTHER_PLAYER_ROLE))
        {
            TRACE_LOG("[GameSessionMaster].addPlayerFromQueue(): User attempting change queued player's role does not have SET_OTHER_PLAYER_ROLE permission.");
            return GAMEMANAGER_ERR_PERMISSION_DENIED;
        }


        // Update slot type and team information with what is available.
        JoinGameMasterError::Error joinCapacityError = validateJoinGameCapacities(joiningRoleName, queuedPlayer.getSlotType(), joiningTeamIndex, queuedPlayer.getPlayerId(), actualSlotType, joiningTeamIndex);

        // Possible that a private slot has opened, but we are trying to join public.
        if (joinCapacityError != JoinGameMasterError::ERR_OK)
        {
            // Skip this player for now since slots are full.  Don't remove them, as we may be able to add them later.
            TRACE_LOG("[GameSessionMaster] Failed to add queued player(" << queuedPlayer.getPlayerId() << ") into game("
                      << getGameId() << "), error(" << (ErrorHelp::getErrorName((BlazeRpcError)joinCapacityError)) << ")");
            return static_cast<BlazeRpcError>(joinCapacityError);
        }

        // Check the invariant that the queue index saved at time of promotion must always be a valid value
        EA_ASSERT(queuedPlayer.getSlotId() < mPlayerRoster.getQueueCount());
        // store off the last queued index of the player
        queuedPlayer.setQueuedIndexWhenPromoted(queuedPlayer.getSlotId());
        // We set the queued players slot id to the default, so one will be assigned for them when they are
        // added to the game below.
        queuedPlayer.getPlayerData()->setSlotId(DEFAULT_JOINING_SLOT);

        // save off original data for external session update afterwards
        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        queuedPlayer.getPlayerData()->setSlotType(actualSlotType);
        queuedPlayer.getPlayerData()->setTeamIndex(joiningTeamIndex);
        queuedPlayer.getPlayerData()->setRoleName(joiningRoleName);

        TRACE_LOG("[GameSessionMaster] Adding player(" << queuedPlayer.getPlayerId() << ":" << queuedPlayer.getPlayerName() << ":conngroup=" << queuedPlayer.getConnectionGroupId() << "), in state("
                  << PlayerStateToString(queuedPlayer.getPlayerState()) << ") from queue into game(" << getGameId() << ") roster.");

        // We're outside any client requested activity.
        GameSetupReason playerJoinContext;
        playerJoinContext.switchActiveMember(GameSetupReason::MEMBER_DATALESSSETUPCONTEXT);
        playerJoinContext.getDatalessSetupContext()->setSetupContext(INDIRECT_JOIN_GAME_FROM_QUEUE_SETUP_CONTEXT);

        BlazeRpcError addErr = ERR_OK;

        bool wasQueued = true;
        PlayerId playerId = queuedPlayer.getPlayerId();
        // If the player only had a reservation in the queue we promote them, but they stay reserved.
        if (queuedPlayer.isReserved())
        {
            addErr = addReservedPlayer(&queuedPlayer, playerJoinContext, wasQueued);
        }
        else
        {
            addErr = addActivePlayer(&queuedPlayer, playerJoinContext, false, wasQueued);
        }

        if (addErr != ERR_OK)
        {
            // remove the player from the game (and thus the queue)
            PlayerRemovalContext playerRemovalContext(PLAYER_JOIN_FROM_QUEUE_FAILED, 0, nullptr);
            RemovePlayerMasterError::Error err = removePlayer(&queuedPlayer, playerRemovalContext);
            ERR_LOG("[GameSessionMaster]  Failed to add queued player(" << queuedPlayer.getPlayerId() << ") into game(" << getGameId() << "), add error("
                    << (ErrorHelp::getErrorName(addErr)) << "), remove error(" << (ErrorHelp::getErrorName((BlazeRpcError)err)) << ")");
            return addErr;
        }
        else
        {
            // Note that queuedPlayer has been invalidated.
            PlayerInfoMaster* promotedPlayer = mPlayerRoster.getRosterPlayer(playerId);
            if (promotedPlayer != nullptr)
            {
                submitEventDequeuePlayer(*promotedPlayer);

                // we did not add dequeuing players to the pending preferred joins set, at the pt the slot opened up.
                // Now that its actually dequeued, add it, if we're locked for preferred joins.
                if (isLockedForPreferredJoins() && (!promotedPlayer->isReserved()))
                    insertToPendingPreferredJoinSet(playerId);
            }

            // if fullness changed, update external session props.
            updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());
            if (provisionalFollowThisPlayer)
            {
                addTargetPlayerIdForGroup(queuedPlayerGroupId, queuedPlayerId);
            }
            // We skip the explicit removal call in failure case as removePlayer does that internally.
            eraseUserGroupIdFromLists(queuedPlayerGroupId, true);
        }

        return ERR_OK;
    }


    Blaze::BlazeRpcError GameSessionMaster::demotePlayerToQueue(PlayerInfoMaster& queuedPlayer, bool checkForFullQueue)
    {
        if (mPlayerRoster.getQueuePlayer(queuedPlayer.getPlayerId()) != nullptr)
        {
            TRACE_LOG("[GameSessionMaster] Invalid attempt to demote queued player(" <<
                queuedPlayer.getPlayerId() << ") to queue in game(" << getGameId() << ")");
            return GAMEMANAGER_ERR_PLAYER_NOT_IN_QUEUE;
        }

        if (getGameState() == MIGRATING)
        {
            TRACE_LOG("[GameSessionMaster] Unable to demote player(" << queuedPlayer.getPlayerId() <<
                ") to the queue from game(" << getGameId() << ") while the game is migrating.");
            return GAMEMANAGER_ERR_DEQUEUE_WHILE_MIGRATING;
        }

        // Can not join when the game is in progress; even from the queue.
        if (!getGameSettings().getJoinInProgressSupported() && isGameInProgress())
        {
            TRACE_LOG("[GameSessionMaster] Not allowing player(" << queuedPlayer.getPlayerId() <<
                ") to demote to queue in game(" << getGameId() << ") while it is in progress.");
            return GAMEMANAGER_ERR_DEQUEUE_WHILE_IN_PROGRESS;
        }

        if (!isQueueEnabled() || (checkForFullQueue && isQueueFull()))
        {
            TRACE_LOG("[GameSessionMaster] Not allowing player(" << queuedPlayer.getPlayerId() <<
                ") to demote to queue in game(" << getGameId() << ") because the queue if full.");
            return GAMEMANAGER_ERR_QUEUE_FULL;
        }

        PlayerInfoMaster* newPlayer = &queuedPlayer;
        if (!mPlayerRoster.demotePlayerToQueue(newPlayer))
        {
            TRACE_LOG("[GameSessionMaster] Unable to demote player to queue in game(" << getGameId() << ").");
            return ERR_SYSTEM;            
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief remove all game members and prepare the game for destruction (note: doesn't actually delete the game).
    ***************************************************************************************************/
    void GameSessionMaster::destroyGame(const GameDestructionReason& destructionReason)
    {
        TRACE_LOG("[GameSessionMaster] Destroying game(" << getGameId() << ") for " << GameDestructionReasonToString(destructionReason) << ".");
        
        // send last snapshot before clearing/reseting game's data
        updateFinalExternalSessionProperties();

        mMetricsHandler.onGameDestroyed(true, destructionReason);

        //cancel outstanding timer
        if (mGameDataMaster->getMigrationTimeout() != 0)
        {
            gGameManagerMaster->getMigrationTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setMigrationTimeout(0);
        }

        cleanUpMatchmakingQosDataMap();

        // Delay the ccs http request for 'delete all' by setting 'immediate' flag to false. During the game destruction, the individual consoles issue their own 'delete'
        // requests followed by this 'delete all' request at the blaze server instance level. However, as the requests are issued by different http instances in the pool,
        // packets on the internet take different route. As a result, the 'delete all' request may reach the CCS server before 'delete specific'. So we introduce a delay 
        // during game destruction for 'delete all' request. 
        cleanupCCS(false);

        PlayerRemovedReason playerRemovedReason = GAME_DESTROYED;
        if (destructionReason == SYS_GAME_FAILED_CONNECTION_VALIDATION)
            playerRemovedReason = PLAYER_CONN_POOR_QUALITY;
        removeAllPlayers(playerRemovedReason);

        for (auto& pinConnEvents : gGameManagerMaster->getMetrics().mConnectionStatsEventListByConnGroup[getGameId()])
        {
            GameManagerMasterMetrics::ConnectionGroupIdPair connPair = pinConnEvents.first;
            gGameManagerMaster->sendPINConnStatsEvent(this, connPair.first, pinConnEvents.second);
        }

        // clean up maps for connection metrics
        gGameManagerMaster->removeGameFromConnectionMetrics(getGameId());

        if (hasDedicatedServerHost())
        {
            // remove this game from the dedicated server host's sessionGameInfo
            gGameManagerMaster->removeUserSessionGameInfo(getDedicatedServerHostSessionId(), *this);
        }
        if (hasExternalOwner())
        {
            gGameManagerMaster->removeUserSessionGameInfo(getExternalOwnerSessionId(), *this);
        }

        

        // All dedicated server games can make an injection job, so better safe than sorry to remove the job in all cases:
        // clean up host injection job if one exists.
        gGameManagerMaster->removeHostInjectionJob(getGameId());

        if(isDedicatedHostedTopology(getGameNetworkTopology()) && getGameState() != INACTIVE_VIRTUAL)
        {
            // disassociate the host's session with the game
            BlazeRpcError err = ERR_OK;
            err = gUserSessionManager->removeBlazeObjectIdForSession(getDedicatedServerHostSessionId(), getBlazeObjectId());
            if (err != ERR_OK)
            {
                WARN_LOG("[GameSessionMaster].dtor: " << (ErrorHelp::getErrorName(err)) << " error removing host session " << mGameDataMaster->getCreatorHostInfo().getSessionId() << " from game " << getGameId());
            }
        }

        // remove locked for preferred joins timer
        clearLockForPreferredJoins(false);

        // Clear busy timeout:
        if (mGameDataMaster->getLockedAsBusyTimeout() != 0)
        {
            gGameManagerMaster->getLockedAsBusyTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setLockedAsBusyTimeout(0);
        }

        gGameManagerMaster->eraseGameData(getGameId());

        // submit game session destroyed event
        DestroyedGameSession gameSessionEvent;
        gameSessionEvent.setGameId(getGameId());
        gameSessionEvent.setGameReportingId(getGameReportingId());
        gameSessionEvent.setGameName(getGameName());
        gameSessionEvent.setDestructionReason(GameDestructionReasonToString(destructionReason));
        gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_DESTROYEDGAMESESSIONEVENT), gameSessionEvent, true);
    }


    /*! ************************************************************************************************/
    /*! \brief remove all game members and return the dedicated server to the "resetable" state.
    ***************************************************************************************************/
    ReturnDedicatedServerToPoolMasterError::Error GameSessionMaster::returnDedicatedServerToPool()
    {


        // set game state.  If we already are resettable, skip this as we could just be cleaning up other aspects of the game
        if (getGameState() == UNRESPONSIVE)
        {
            if (mGameDataMaster->getPreUnresponsiveState() != RESETABLE)
            {
                if (isServerNotResetable())
                {
                    ERR_LOG("[GameSessionMaster] unresponsive game(" << getGameId() << ") is not allowed to be RESETABLE");
                    return ReturnDedicatedServerToPoolMasterError::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
                }
                if (!gGameManagerMaster->getGameStateHelper()->isStateChangeAllowed(mGameDataMaster->getPreUnresponsiveState(), RESETABLE))
                {
                    ERR_LOG("[GameSessionMaster] unresponsive game(" << getGameId() << ") does not support state transition from "
                        << GameStateToString(mGameDataMaster->getPreUnresponsiveState()) << " to RESETABLE");
                    return ReturnDedicatedServerToPoolMasterError::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
                }
                mGameDataMaster->setPreUnresponsiveState(RESETABLE);
            }
        }
        else if (getGameState() != RESETABLE)
        {
            BlazeRpcError err = setGameState(RESETABLE);
            if (EA_UNLIKELY(err != ERR_OK))
            {
                ERR_LOG("[GameSessionMaster] Failed to set state for game(" << getGameId() << "), the transition is not supported, error("
                        << (ErrorHelp::getErrorName(err)) << ")");
                return ReturnDedicatedServerToPoolMasterError::GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
            }
        }

        gameFinished();

        GameReportingId gameReportId = gGameManagerMaster->getNextGameReportingId();
        if (gameReportId == GameManager::INVALID_GAME_REPORTING_ID)
        {
            WARN_LOG("[GameSessionMaster] Game(" << getGameId() << ") couldn't generate a GameReportingId when returning to the pool.");
        }

        //Nuke this game out of the census cache
        gGameManagerMaster->removeGameFromCensusCache(*this);

        // send last snapshot before clearing/reseting game's data
        updateFinalExternalSessionProperties();

        // reset for next match
        mGameDataMaster->setGameFinished(false);

        cleanUpMatchmakingQosDataMap();


        //need to clean up platform restrictions (set it back to the hosted platform list for the blaze cluster) upon returning to pool
        // by updating the base platform list, removeAllPlayers will fix up the main platform list
        for (auto curPlat : getGameData().getDedicatedServerSupportedPlatformList())
            getGameData().getBasePlatformList().insertAsSet(curPlat);

        // If no players are left, then removeAllPlayers wouldn't call refreshDynamicPlatformList
        if (mPlayerRoster.getPlayerCount(PlayerRoster::ALL_PLAYERS) == 0)
            refreshDynamicPlatformList();

        // remove all active players - note: this bypasses the GameManagerMasterImpl::removePlayer helper, since
        //  we don't want to trigger n-1 host migrations as we destroy the game.
        removeAllPlayers(GAME_ENDED);

        // No players must remain in the roster!
        const uint16_t playerCount = mPlayerRoster.getPlayerCount(PlayerRoster::ALL_PLAYERS);
        if (playerCount > 0)
        {
            // Bad news, this should not happen
            WARN_LOG("[GameSessionMaster] Game(" << getGameId() << ") failed to remove " << playerCount << " players when returning game to pool.");
        }

        for (auto& pinConnEvents : gGameManagerMaster->getMetrics().mConnectionStatsEventListByConnGroup[getGameId()])
        {
            GameManagerMasterMetrics::ConnectionGroupIdPair connPair = pinConnEvents.first;
            gGameManagerMaster->sendPINConnStatsEvent(this, connPair.first, pinConnEvents.second);
        }

        // clean up maps for connection metrics
        gGameManagerMaster->removeGameFromConnectionMetrics(getGameId());

        // we need to reset/clear the game's platform host info since this is a DS game
        //  when the game is reset, it will init a new platform host from a joining player (if needed)
        getGameData().getDedicatedServerHostInfo().copyInto( getGameData().getPlatformHostInfo() );

        // update game state and set a new game reporting id
        setGameReportingId(gameReportId);

        // return game protocol version of server to its default
        getGameData().setGameProtocolVersionString(mGameDataMaster->getInitialGameProtocolVersionString());
        // Hash off the game protocol version string for fast MM comparison.
        eastl::hash<const char8_t*> stringHash;
        getGameData().setGameProtocolVersionHash(stringHash(mGameDataMaster->getInitialGameProtocolVersionString()));

        // reset template name back to the initial value
        getGameData().setCreateGameTemplateName(mGameDataMaster->getInitialGameTemplateName());

        // check the configuration value of "clearBannedListWithReset".
        // if it is true, blaze server will clear the banned list value on resetting a dedicated server.
        if (gGameManagerMaster->getClearBannedListWithReset())
        {
            clearBannedList();
        }


        // clear any game groups from previous games
        mGameData->getSingleGroupMatchIds().clear();

        // This data will replicate to slave when gamereporting id changes.
        // Though it wont replicate to clients (as there are none)

        // clear any team/role information
        getGameData().getTeamIds().clear();
        getGameData().getRoleInformation().getMultiRoleCriteria().clear();
        getGameData().getRoleInformation().getRoleCriteriaMap().clear();

        // need to have valid team/role info in case of direct reset
        getGameData().getTeamIds().push_back(ANY_TEAM_ID);
        uint16_t perTeamCapacity = getTotalParticipantCapacity();
        setUpDefaultRoleInformation(getGameData().getRoleInformation(), perTeamCapacity);

        // clean up persisted id if one exists
        gGameManagerMaster->removePersistedGameIdMapping(*this);

        NotifyGameReportingIdChange nGameReportingIdChange;
        nGameReportingIdChange.setGameId(getGameId());
        nGameReportingIdChange.setGameReportingId(getGameReportingId());
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyGameReportingIdChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nGameReportingIdChange);


        mMetricsHandler.onDedicatedGameHostReturnedToPool();
        
        clearExternalSessionData();

        return ReturnDedicatedServerToPoolMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief returns true if a joinMethod should ignore the game's gameEntryCriteria check.
    ***************************************************************************************************/
    bool GameSessionMaster::ignoreEntryCriteria( JoinMethod joinMethod, SlotType slotType ) const
    {
        if (isSpectatorSlot(slotType))
        {
            // spectators skip GEC
            return true;
        }

        // the game may allow invited players to skip over the entry criteria check.
        if ( getIgnoreEntryCriteriaWithInvite() && (joinMethod == JOIN_BY_INVITES) )
        {
            return true;
        }

        // we always skip the entry criteria for certain system-level joins (typically following a game group leader)
        return ( (joinMethod == SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME) ||
                 (joinMethod == SYS_JOIN_BY_RESETDEDICATEDSERVER) ||
                 (joinMethod == SYS_JOIN_BY_FOLLOWLEADER_RESETDEDICATEDSERVER) );
    }

    /*! ************************************************************************************************/
    /*! \brief returns true if a joinMethod should ignore the game's banlist check.
    ***************************************************************************************************/
    bool GameSessionMaster::ignoreBanList(JoinMethod joinMethod) const
    {
        // the game may allow invited players to skip over the ban list check.
        // Note: JOIN_BY_PLAYER not included here as with entry criteria, to avoid stalkers
        return (getIgnoreEntryCriteriaWithInvite() && (joinMethod == JOIN_BY_INVITES) );
    }

    /*! ************************************************************************************************/
    /*! \brief returns true if an admin invited this player to the game and the game only allows admin invited
        player to bypass entry checks.
    ***************************************************************************************************/
    bool GameSessionMaster::ignoreEntryCheckAdminInvited(const JoinGameRequest &joinGameRequest) const
    {
        // Attempt to get the inviter of the player.  Must be online and admin in game
        return ( getGameSettings().getAdminInvitesOnlyIgnoreEntryChecks() &&
            (joinGameRequest.getJoinMethod() == JOIN_BY_INVITES) && hasPermission(joinGameRequest.getUser().getBlazeId(), GameAction::SKIP_ENTRY_CHECKS_BY_INVITE));
    }

    void GameSessionMaster::initExternalSessionData(const ExternalSessionMasterRequestData& newData, const GameCreationData* createData /*= nullptr*/, const TournamentSessionData* tournamentInfo /*= nullptr*/, const ExternalSessionActivityTypeInfo* forType /*= nullptr*/)
    {
        TRACE_LOG("[GameSessionMaster] Setting game(" << getGameId() << ") external session identification setup to (" << (forType ? toLogStr(newData.getExternalSessionIdentSetupMaster(), *forType) : toLogStr(newData.getExternalSessionIdentSetupMaster())) << "), current participant count is " << mPlayerRoster.getParticipantCount() << ", current game state is " << GameStateToString(getGameState()) << ", base externalSessionIdent " << (createData ? toLogStr(createData->getExternalSessionIdentSetup()) : toLogStr(getExternalSessionIdentification())));
        if (forType)
        {
            setExtSessIdent(getExternalSessionIdentification(), newData.getExternalSessionIdentSetupMaster(), *forType);
        }
        else
        {
            newData.getExternalSessionIdentSetupMaster().copyInto(getExternalSessionIdentification());
        }
        // side: NP session id set later when external session is created
        if (createData != nullptr)
        {
            createData->getExternalSessionCustomData().copyInto(getGameDataMaster()->getExternalSessionCustomData());
            createData->getExternalSessionStatus().copyInto(getGameDataMaster()->getExternalSessionStatus());
        }
        if (tournamentInfo != nullptr)
        {
            tournamentInfo->copyInto(getGameDataMaster()->getTournamentSessionData());
        }
        newToOldReplicatedExternalSessionData(getGameData());// for back compat
    }

    void GameSessionMaster::clearExternalSessionData()
    {
        getExternalSessionIdentification().getXone().setCorrelationId("");
        getExternalSessionIdentification().getXone().setTemplateName("");
        getExternalSessionIdentification().getXone().setSessionName("");
        getGameDataMaster()->getTournamentSessionData().setTournamentDefinition("");
        getGameDataMaster()->getTournamentSessionData().setScheduledStartTime("");
        getGameDataMaster()->getTournamentSessionData().setArbitrationTimeout(0);
        getGameDataMaster()->getTournamentSessionData().setForfeitTimeout(0);

        getExternalSessionIdentification().getPs4().setNpSessionId("");
        getExternalSessionIdentification().getPs5().getMatch().setMatchId("");
        getExternalSessionIdentification().getPs5().getMatch().setActivityObjectId("");
        getExternalSessionIdentification().getPs5().getPlayerSession().setPlayerSessionId("");
        getGameDataMaster()->getExternalSessionCustomData().setData(nullptr, 0);
        getGameDataMaster()->getExternalSessionStatus().setUnlocalizedValue("");
        getGameDataMaster()->getExternalSessionStatus().getLangMap().clear();
        getGameDataMaster()->getTrackedExternalSessionMembers().clear();
        newToOldReplicatedExternalSessionData(getGameData());// for back compat
    }

    /*! ************************************************************************************************/
    /*! \brief clear ids of the 1st party session as needed, after last of the platform's members leaves
        \param[in] type If platform has alternate types of external sessions, specify it.
    ***************************************************************************************************/
    void GameSessionMaster::clearExternalSessionIds(ClientPlatformType platform, ExternalSessionActivityType type /*= EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY*/)
    {
        switch (platform)
        {
            case xone:
            case xbsx:
                getExternalSessionIdentification().getXone().setCorrelationId("");
                getExternalSessionIdentification().getXone().setSessionName("");
                break;
            case ps4:
                getExternalSessionIdentification().getPs4().setNpSessionId("");
                break;
            case ps5:
                if (type == EXTERNAL_SESSION_ACTIVITY_TYPE_PRIMARY)
                    getExternalSessionIdentification().getPs5().getPlayerSession().setPlayerSessionId("");
                // Skip PS5 Matches as they can persist even when empty.
                break;
            // following platforms don't have external sessions
            case pc:
            case nx:
            case steam:
            case stadia:
            default:
                break;
        }

        newToOldReplicatedExternalSessionData(getGameData());// for back compat
    }

    /*! ************************************************************************************************/
    /*! \brief join a player into a game

        \param[in] request - join game request
        \param[out] error - an error tdf filled out with details of the entry criteria that prevented the user from joining the game
        \param[in] we use this replication reason when inserting the player into the replicated roster map
    ***************************************************************************************************/
    JoinGameMasterError::Error GameSessionMaster::joinGame(const JoinGameMasterRequest &joinGameRequest, JoinGameMasterResponse &joinGameResponse,
        EntryCriteriaError &entryCriteriaError, const GameSetupReason &playerJoinContext)
    {
        // If locked as busy, we may allow caller to suspend the join attempt till later.
        // Pre: this check is done before any checks against significant things that can change while locked.
        if (getGameSettings().getLockedAsBusy())
        {
            return JoinGameMasterError::GAMEMANAGER_ERR_GAME_BUSY;
        }

        BlazeId joiningBlazeId = joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getId();
        const PlatformInfo& joiningPlatformInfo = joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo();

        // Crossplay Platform restriction check:  
        // GOS-32214 - May need to do this check earlier if we want to avoid edge cases where a [pc, xone] group tries to join a [pc, ps4] game when xone/ps4 crossplay is disabled. 
        // or if a mixed group of cross-play opted -in and -out players try to join a crossplay enabled game.
        
        //+ The usage of TRACE log level below is intentional to avoid ERR spam in logs. All of these are normal operations in our game management/mm flow.
        bool userAllowedCrossplay = UserSessionManager::isCrossplayEnabled(joinGameRequest.getUserJoinInfo().getUser().getUserInfo());
        if (isCrossplayEnabled() && !userAllowedCrossplay)
        {
            TRACE_LOG("[GameSessionMaster].joinGame - user ("<< joiningBlazeId <<") prevented from joining Game (" << getGameId() << ") because user has not opted in to crossplay.");
            return JoinGameMasterError::GAMEMANAGER_ERR_CROSSPLAY_DISABLED_USER;
        }

        // Base Platform List comes from the game set up
        if (!getBasePlatformList().empty() && getBasePlatformList().findAsSet(joiningPlatformInfo.getClientPlatform()) == getBasePlatformList().end())
        {
            TRACE_LOG("[GameSessionMaster].joinGame - user (" << joiningBlazeId << ") prevented from joining Game (" << getGameId() << ") because player platform ("
                << ClientPlatformTypeToString(joiningPlatformInfo.getClientPlatform()) << ") is not an acceptable platform for this Game. Allowed platforms: "
                << getBasePlatformList());
            return JoinGameMasterError::GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
        }
        
        // Dynamic platform list comes by combining the global restrictions in usersessions.cfg and platforms of the current players in the game.  
        if ((!getCurrentlyAcceptedPlatformList().empty() && getCurrentlyAcceptedPlatformList().findAsSet(joiningPlatformInfo.getClientPlatform()) == getCurrentlyAcceptedPlatformList().end()))
        {
            TRACE_LOG("[GameSessionMaster].joinGame - user (" << joiningBlazeId << ") prevented from joining Game (" << getGameId() << ") because player platform ("
                << ClientPlatformTypeToString(joiningPlatformInfo.getClientPlatform()) << ") is currently not an acceptable platform for this Game. Currently Allowed platforms: "
                << getCurrentlyAcceptedPlatformList());
            return JoinGameMasterError::GAMEMANAGER_ERR_CROSSPLAY_DISABLED;
        }
        //-

        bool usingCorrelationIds = false;
        if (getCurrentlyAcceptedPlatformList().findAsSet(xone) != getCurrentlyAcceptedPlatformList().end())
        {
            const ExternalSessionUtil* util = getExternalSessionUtilManager().getUtil(xone);
            if (util != nullptr)
                usingCorrelationIds = (util->getConfig().getScid()[0] != '\0');
        }
        if (!usingCorrelationIds && (getCurrentlyAcceptedPlatformList().findAsSet(xbsx) != getCurrentlyAcceptedPlatformList().end()))
        {
            const ExternalSessionUtil* util = getExternalSessionUtilManager().getUtil(xbsx);
            if (util != nullptr)
                usingCorrelationIds = (util != nullptr) && (util->getConfig().getScid()[0] != '\0');
        }
        if ((usingCorrelationIds && (getExternalSessionIdentification().getXone().getCorrelationId()[0] == '\0')) ||
            (getExternalSessionIdentification().getXone().getSessionName()[0] == '\0'))
        {
            // Virtual or direct-reseted games may not have set up an external session or correlation id by time we host-inject/join.
            // Add the correlation id and, its corresponding external session name once available. Note: If we don't have a correlation id (but use them)
            // by flow, GM hasn't joined anyone to any external session for this game, so it should be fine to switch external session name.
            ExternalSessionActivityTypeInfo forType;
            forType.setPlatform(xone);//in case cross-play
            initExternalSessionData(joinGameRequest.getExternalSessionData(), nullptr, nullptr, &forType);
        }

        // an existing player is ok, if it's just the playerReservation
        // if the player already has a reservation, we can claim it
        PlayerInfoMasterPtr joiningPlayer = mPlayerRoster.getPlayer(joiningBlazeId, joiningPlatformInfo);
        if (joiningPlayer != nullptr)
        {
            JoinGameMasterError::Error existingPlayerError = validateExistingPlayerJoinGame(joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType(), *joiningPlayer);
            // player is already in the game, return the proper error code if we don't support it (claiming a reservation).
            if (existingPlayerError != JoinGameMasterError::ERR_OK)
            {
                return existingPlayerError;
            }
        }

        // Special case for local user joins: 
        // Need to make sure that the user is joining a game with a player that's already connected:
        if (joinGameRequest.getJoinRequest().getJoinMethod() == SYS_JOIN_BY_FOLLOWLOCALUSER &&
            joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType() != GAME_ENTRY_TYPE_MAKE_RESERVATION)
        {
            PlayerRoster::PlayerInfoList playerList = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PLAYERS);
            bool foundLocalPlayer = false;
            for (auto& curPlayer : playerList)
            {
                PlayerInfoMaster *currentMember = static_cast<PlayerInfoMaster*>(curPlayer);
                if (currentMember->getPlayerId() != joiningBlazeId &&
                    currentMember->getConnectionGroupId() == joinGameRequest.getUserJoinInfo().getUser().getConnectionGroupId())
                {
                    foundLocalPlayer = true;
                    break;
                }
            }

            if (!foundLocalPlayer)
                return JoinGameMasterError::GAMEMANAGER_ERR_MISSING_PRIMARY_LOCAL_PLAYER;
        }

        // These are validations across all joins including making & claiming a reservation.
        JoinGameMasterError::Error validationErr = validatePlayerJoin(joinGameRequest);
        if (validationErr != JoinGameMasterError::ERR_OK)
        {
            return validationErr;
        }
        validationErr = validatePlayerJoinUserSessionExistence(joinGameRequest);
        if (validationErr != JoinGameMasterError::ERR_OK)
        {
            return validationErr;
        }

        // save off original data for external session update afterwards
        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        // How we handle an entry criteria error depends on how we are joining
        JoinGameMasterError::Error criteriaErr = JoinGameMasterError::ERR_OK;


        // Before Attempting to Join, check if the external user is now logged in, and if so, update the user info held in the request (and switch to a reservation):
        bool externalUserLoggedIn = false;
        if (!PlayerInfo::getSessionExists(joinGameRequest.getUserJoinInfo().getUser()))
        {
            const PlatformInfo& platformInfo = joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo();
            UserSessionPtr userSession = gUserSessionManager->getSession(gUserSessionManager->getUserSessionIdByPlatformInfo(platformInfo));
            if (userSession != nullptr)
            {
                // Player exists, need to update the UserJoinInfo with the player's data: 
                ValidationUtils::setJoinUserInfoFromSessionExistence(const_cast<JoinGameMasterRequest&>(joinGameRequest).getUserJoinInfo().getUser(), userSession->getId(),
                    userSession->getBlazeId(), userSession->getPlatformInfo(), userSession->getAccountLocale(), userSession->getAccountCountry(), userSession->getServiceName(), userSession->getProductName());
                externalUserLoggedIn = true;
            }
        }


        // If we're already reserved in the game, we should just claim the reservation.
        GameEntryType origEntryType = joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType();
        DatalessContext origSetupContext = playerJoinContext.getDatalessSetupContext() ? playerJoinContext.getDatalessSetupContext()->getSetupContext() : CREATE_GAME_SETUP_CONTEXT;

        if ((joiningPlayer != nullptr && joiningPlayer->isReserved()) && origEntryType == GAME_ENTRY_TYPE_DIRECT)
        {
            // Const_cast here to set the game entry type, which will be used by setupJoiningPlayer
            const_cast<JoinGameMasterRequest&>(joinGameRequest).getJoinRequest().getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_CLAIM_RESERVATION);
            // The dataless setup context also needs to be updated: (This is sent to the client)
            if (playerJoinContext.getDatalessSetupContext() != nullptr)
                const_cast<GameSetupReason&>(playerJoinContext).getDatalessSetupContext()->setSetupContext(INDIRECT_JOIN_GAME_FROM_RESERVATION_CONTEXT);
        }
        else if (externalUserLoggedIn && origEntryType != GAME_ENTRY_TYPE_MAKE_RESERVATION)
        {
            // Const_cast here to set the game entry type, which will be used by setupJoiningPlayer
            const_cast<JoinGameMasterRequest&>(joinGameRequest).getJoinRequest().getPlayerJoinData().setGameEntryType(GAME_ENTRY_TYPE_MAKE_RESERVATION);
        }

        switch (joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType())
        {
        case GAME_ENTRY_TYPE_DIRECT:
            {
                // validation for new player
                JoinGameMasterError::Error newPlayerErr = validateNewPlayerJoin(joinGameRequest);
                if (newPlayerErr != JoinGameMasterError::ERR_OK)
                {
                    return newPlayerErr;
                }

                // Direct joiners must pass entry criteria right away.
                // This may cause issues for external users, since even though they're going to make a reservation, they might fail this check. 
                criteriaErr = validateGameEntryCriteria(joinGameRequest, entryCriteriaError);
                if (criteriaErr != JoinGameMasterError::ERR_OK)
                {
                    return criteriaErr;
                }

                // we're joining as a new player (there's no existing reservation)
                //      we'll either error, join the game, or join the queue
                JoinGameMasterError::Error joinError = joinGameAsNewPlayer(joinGameRequest, joinGameResponse, playerJoinContext, joiningPlayer);
                if (joinError != JoinGameMasterError::ERR_OK)
                {
                    return joinError;
                }
            }
            break;

        case GAME_ENTRY_TYPE_MAKE_RESERVATION:
            {
                // validation for new player
                JoinGameMasterError::Error newPlayerErr = validateNewPlayerJoin(joinGameRequest);
                if (newPlayerErr != JoinGameMasterError::ERR_OK)
                {
                    // If we changed the entry type, revert the values
                    if (origEntryType != joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType())
                    {
                        const_cast<JoinGameMasterRequest&>(joinGameRequest).getJoinRequest().getPlayerJoinData().setGameEntryType(origEntryType);
                        if (playerJoinContext.getDatalessSetupContext() != nullptr)
                            const_cast<GameSetupReason&>(playerJoinContext).getDatalessSetupContext()->setSetupContext(origSetupContext);
                    }
                    return newPlayerErr;
                }

                // Not an error, player can pass criteria when claiming the reservation.
                criteriaErr = validateGameEntryCriteria(joinGameRequest, entryCriteriaError);

                // we're joining as a new player (there's no existing reservation)
                //      we'll either error, join the game, or join the queue
                JoinGameMasterError::Error joinError = joinGameAsNewPlayer(joinGameRequest, joinGameResponse, playerJoinContext, joiningPlayer);
                if (joinError != JoinGameMasterError::ERR_OK)
                {
                    // If we changed the entry type, revert the values
                    if (origEntryType != joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType())
                    {
                        const_cast<JoinGameMasterRequest&>(joinGameRequest).getJoinRequest().getPlayerJoinData().setGameEntryType(origEntryType);
                        if (playerJoinContext.getDatalessSetupContext() != nullptr)
                            const_cast<GameSetupReason&>(playerJoinContext).getDatalessSetupContext()->setSetupContext(origSetupContext);
                    }
                    return joinError;
                }

                // store this off on the player to note that they still need to check entry criteria.
                // If currently don't have a blaze user session, also re-check entry criteria at claim.
                if ((criteriaErr != JoinGameMasterError::ERR_OK) || !PlayerInfo::getSessionExists(joiningPlayer->getUserInfo()))
                {
                    joiningPlayer->setNeedsEntryCriteriaCheck();
                }
            }
            break;

        case GAME_ENTRY_TYPE_CLAIM_RESERVATION:
            {
                // validation for claiming reservation player
                bool failedEntryCriteria = false;
                JoinGameMasterError::Error err = validateClaimReservationJoin(joiningPlayer.get(), joiningBlazeId);
                if (err == JoinGameMasterError::ERR_OK)
                {
                    // When claiming, if we already passed entry criteria when making the reservation, we can proceed.
                    err = validateGameEntryCriteria(joinGameRequest, entryCriteriaError, joiningPlayer->getRoleName(), joiningPlayer->getSlotType());
                    if (err != JoinGameMasterError::ERR_OK && joiningPlayer->getNeedsEntryCriteriaCheck())
                    {
                        failedEntryCriteria = true;
                    }
                    else
                    {
                        // Reset err, in case we failed criteria check, but we don't need the check.
                        err = JoinGameMasterError::ERR_OK;

                        // The players claim request has beat their onUserSessionExistence.  In this case, we need to update the
                        // replicated map so it is keyed of our actual BlazeId, instead of the temporary negative one
                        // we gave them while they didn't have an EA account.
                        if (joiningPlayer->hasExternalPlayerId() && (joiningPlayer->getPlayerId() != joiningBlazeId))
                        {
                            PlayerInfoMaster* replacedJoiningPlayer = mPlayerRoster.replaceExternalPlayer(joinGameRequest.getUserJoinInfo().getUser().getSessionId(), joiningBlazeId,
                                joiningPlatformInfo, joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getAccountLocale(),
                                joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getAccountCountry(),
                                joinGameRequest.getUserJoinInfo().getUser().getServiceName(), joinGameRequest.getUserJoinInfo().getUser().getProductName());
                            if (replacedJoiningPlayer == nullptr)
                            {
                                err = JoinGameMasterError::ERR_SYSTEM;
                            }
                            else
                            {
                                joiningPlayer = replacedJoiningPlayer;
                            }
                        }

                        // Update the game entry type as a rejoin might have a different game entry type
                        joiningPlayer->setGameEntryType(joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType());

                        // Finally, actually claim the reservation:
                        if (err == JoinGameMasterError::ERR_OK)
                        {
                            err = joinGameClaimReservation(joinGameRequest, playerJoinContext, *joiningPlayer);
                        }
                    }
                }

                if (err != JoinGameMasterError::ERR_OK)
                {
                    mMetricsHandler.onPlayerReservationClaimFailed(joiningPlayer.get(), failedEntryCriteria);
                    // If we changed the entry type, revert the values
                    if (origEntryType != joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType())
                    {
                        const_cast<JoinGameMasterRequest&>(joinGameRequest).getJoinRequest().getPlayerJoinData().setGameEntryType(origEntryType);
                        if (playerJoinContext.getDatalessSetupContext() != nullptr)
                            const_cast<GameSetupReason&>(playerJoinContext).getDatalessSetupContext()->setSetupContext(origSetupContext);
                    }
                    return err;
                }
            }
            break;

        default:
            ERR_LOG("[GameSessionMaster] Unknown game entry type while validating join action for player(" << joiningBlazeId
                    << ") into game(" << getGameId() << ")");
            EA_FAIL();
            break;
        }

        if (IS_LOGGING_ENABLED(Logging::TRACE))
        {
            eastl::string attrStr;
            if (joiningPlayer == nullptr)
                return JoinGameMasterError::ERR_SYSTEM;

            Collections::AttributeMap::const_iterator i = joiningPlayer->getPlayerAttribs().begin();
            Collections::AttributeMap::const_iterator end = joiningPlayer->getPlayerAttribs().end();
            for(; i != end; i++)
            {
                attrStr.append_sprintf("%s=%s ", i->first.c_str(), i->second.c_str());
            }
            TRACE_LOG("[GameSessionMaster] Player(" << joiningBlazeId << ":" << joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getPersonaName() << ":connGroupId=" << joinGameRequest.getUserJoinInfo().getUser().getConnectionGroupId() << ") with attributes( " << attrStr.c_str() << ") joined game(" << getGameId() << ").");
        }

        // add group to allowed group list if needed
        if (getGameSettings().getEnforceSingleGroupJoin())
        {
			UserGroupId groupId = joinGameRequest.getUserJoinInfo().getUserGroupId();
            if (groupId != EA::TDF::OBJECT_ID_INVALID)
            {
                addGroupIdToAllowedSingleGroupJoinGroupIds(groupId);
            }
        }

        // if dynamic reputation requirement enabled, adjust allowAnyReputation as needed.
        updateAllowAnyReputationOnJoin(joinGameRequest.getUserJoinInfo().getUser());

        submitJoinGameEvent(joinGameRequest, joinGameResponse.getJoinResponse(), *joiningPlayer);

        if (hasDedicatedServerHost())
        {
             gGameManagerMaster->logDedicatedServerEvent("player joined", *this);
             gGameManagerMaster->logUserInfoForDedicatedServerEvent("player joined", joiningBlazeId, joinGameRequest.getUserJoinInfo().getUser().getLatencyMap(), *this);
        }

        // We need to return the actual list of external id, so slave caller can know which ones need to be added to the external session.
        // This is because several ops such as joinGameByGroup, or, any join, create or reset using the 'reservedExternalPlayers'
        // parameter, could silently fail joining *some* of the optional users. To keep the pattern consistent, all slave cmds
        // whose master call comes through here (i.e. join-related), will use this ExternalUserJoinInfos list to update the external session after this master call returns.
        if (joiningPlayer != nullptr) //side: Blaze (15.x+) puts queued players into ext session but won't set primarysession/presence until dequeue (see GOS-28245, GOS-28244)
        {
            //GM_AUDIT: if copying external tokens here and sending back to slave impacts perf, try looking cached token up from the create/join request at slave, where its originating from, instead.
            updateExternalSessionJoinParams(joinGameResponse.getExternalSessionParams(), joiningPlayer->getUserInfo(),
                (joiningPlayer->isReserved()), *this);
        }
        // if fullness changed, update external session props. Note: check even for claiming a reservation as role fullness may change.
        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());


        // Revert back any temp changes we made to the entry type:
        if (origEntryType != joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType())
        {
            const_cast<JoinGameMasterRequest&>(joinGameRequest).getJoinRequest().getPlayerJoinData().setGameEntryType(origEntryType);
            if (playerJoinContext.getDatalessSetupContext() != nullptr)
                const_cast<GameSetupReason&>(playerJoinContext).getDatalessSetupContext()->setSetupContext(origSetupContext);
        }

        return JoinGameMasterError::ERR_OK;
    }

    void GameSessionMaster::submitJoinGameEvent(const JoinGameMasterRequest &joinRequest, const JoinGameResponse &joinResponse, const PlayerInfoMaster &joiningPlayer)
    {
        if (isPseudoGame())
        {
            return;
        }

        if (joinRequest.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION)
        {
            // submit a make reservation event
            // while you can make a reservation in the queue or in the game, we only
            // send a reserved event until they actually claim the reservation.  At that
            // time, we send the proper event below.
            PlayerReservedGameSession gameSessionEvent;
            gameSessionEvent.setPlayerId(joinRequest.getUserJoinInfo().getUser().getUserInfo().getId());
            gameSessionEvent.setGameId(getGameId());
            gameSessionEvent.setGameReportingId(getGameReportingId());
            gameSessionEvent.setGameName(getGameName());
            gameSessionEvent.setPersonaName(joinRequest.getUserJoinInfo().getUser().getUserInfo().getPersonaName());
            gameSessionEvent.setJoinState(joinResponse.getJoinState());
            mPlayerRoster.populateQueueEvent(gameSessionEvent.getQueuedPlayers());
            gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_PLAYERRESERVEDGAMESESSIONEVENT), gameSessionEvent, true);
        }
        else
        {
            if (joinResponse.getJoinState() == JOINED_GAME)
            {
                // submit join game session event, either joining direct, or claiming reservation.
                bool isPlatformHost = ( hasDedicatedServerHost() && (getPlatformHostInfo().getPlayerId() == joinRequest.getUserJoinInfo().getUser().getUserInfo().getId()));
                PlayerJoinedGameSession gameSessionEvent;
                gameSessionEvent.setPlayerId(joinRequest.getUserJoinInfo().getUser().getUserInfo().getId());
                gameSessionEvent.setGameId(getGameId());
                gameSessionEvent.setGameReportingId(getGameReportingId());
                gameSessionEvent.setGameName(getGameName());
                gameSessionEvent.setPersonaName(joinRequest.getUserJoinInfo().getUser().getUserInfo().getPersonaName());
                gameSessionEvent.setClientPlatform(joinRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo().getClientPlatform());
                gameSessionEvent.setIsPlatformHost(isPlatformHost);
                gameSessionEvent.setRoleName(joiningPlayer.getRoleName());
                gameSessionEvent.setSlotType(joiningPlayer.getSlotType());
                getGameAttribs().copyInto(gameSessionEvent.getGameAttribs());
                gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_PLAYERJOINEDGAMESESSIONEVENT), gameSessionEvent, true);
            }
            else
            {
                // submit joined queue event.
                PlayerQueuedGameSession gameSessionEvent;
                gameSessionEvent.setPlayerId(joinRequest.getUserJoinInfo().getUser().getUserInfo().getId());
                gameSessionEvent.setGameId(getGameId());
                gameSessionEvent.setGameReportingId(getGameReportingId());
                gameSessionEvent.setGameName(getGameName());
                gameSessionEvent.setPersonaName(joinRequest.getUserJoinInfo().getUser().getUserInfo().getPersonaName());
                mPlayerRoster.populateQueueEvent(gameSessionEvent.getQueuedPlayers());
                gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_PLAYERQUEUEDGAMESESSIONEVENT), gameSessionEvent, true);
            }
        }
    }

    /*!************************************************************************************************/
    /*! \brief validates a player's joinGame request & determines if the player is claiming an existing player reservation.

        \param[in] joinGameRequest
        \param[out] entryCriteriaError - populated with a GameEntryCriteria failure message (if we fail the entry criteria check)
    ***************************************************************************************************/
    JoinGameMasterError::Error GameSessionMaster::validatePlayerJoin(const JoinGameMasterRequest &joinGameMasterRequest) const
    {
        const JoinGameRequest& joinGameRequest = joinGameMasterRequest.getJoinRequest();

        if (!isGameProtocolVersionStringCompatible(gGameManagerMaster->getConfig().getGameSession().getEvaluateGameProtocolVersionString(), joinGameRequest.getCommonGameData().getGameProtocolVersionString(), getGameProtocolVersionString()))
        {
            TRACE_LOG("[GameSessionMaster].validatePlayerJoin() : Game protocol version mismatch; game protocol version \""
                << joinGameRequest.getCommonGameData().getGameProtocolVersionString() << "\" attemted to join game " << getGameId()
                << " with game protocol version \"" << getGameProtocolVersionString() << "\"");
            return JoinGameMasterError::GAMEMANAGER_ERR_GAME_PROTOCOL_VERSION_MISMATCH;
        }

        // note reserved list players were validated earlier instead based on caller's joinMethod.
        bool doRepCheck = (joinGameRequest.getJoinMethod() != SYS_JOIN_BY_FOLLOWLEADER_RESERVEDEXTERNALPLAYER);

        if (doRepCheck && !validateJoinReputation(joinGameMasterRequest.getUserJoinInfo().getUser(), joinGameRequest.getPlayerJoinData().getGameEntryType(), joinGameRequest.getJoinMethod()))
        {
            return JoinGameMasterError::GAMEMANAGER_ERR_FAILED_REPUTATION_CHECK;
        }

        JoinGameMasterError::Error actionErr = validateJoinAction(joinGameMasterRequest);
        if (actionErr != JoinGameMasterError::ERR_OK)
        {
            return actionErr;
        }

        // prevent potential user/client issues, ensure external host never a player
        if (hasExternalOwner() && (joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo().getId() == getExternalOwnerBlazeId()))
        {
            ERR_LOG("[GameSessionMaster] cannot join an external owner(" << getExternalOwnerBlazeId() << ") as a player in its game(" << getGameId() << ")");
            return JoinGameMasterError::ERR_SYSTEM;
        }

        // Restrict entry checks to only be bypassed when invited by an admin player
        // Check the joining player against the ban list.
        if (!ignoreEntryCheckAdminInvited(joinGameMasterRequest.getJoinRequest()) && !ignoreBanList(joinGameRequest.getJoinMethod()))
        {
            if ( isAccountBanned(joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId()) )
            {
                return JoinGameMasterError::GAMEMANAGER_ERR_PLAYER_BANNED;
            }
        }

        if ((joinGameRequest.getRequestedSlotId() != DEFAULT_JOINING_SLOT) && isSlotOccupied(joinGameRequest.getRequestedSlotId()))
        {
            // requested slot was occupied, join fails
            WARN_LOG("[GameSessionMaster] Failed to join game(" << getGameId() << ") when trying to add player(" << joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo().getId()
                     << "), slot (" << joinGameRequest.getRequestedSlotId() << ") was already in use.");
            return JoinGameMasterError::GAMEMANAGER_ERR_SLOT_OCCUPIED;
        }

        // Validate the join request against this games teams settings, if the user is joining as a player, not a spectator
        SlotType joiningSlotType = getSlotType(joinGameMasterRequest.getJoinRequest().getPlayerJoinData(), joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo());
        if (isParticipantSlot(joiningSlotType))
        {
            JoinGameMasterError::Error teamsErr = validatePlayerJoinTeamGame(joinGameMasterRequest);
            if (teamsErr != JoinGameMasterError::ERR_OK)
            {
                return teamsErr;
            }
        }

        // check against joining a game in progress when not allowed
        if (!getGameSettings().getJoinInProgressSupported() && isGameInProgress())
        {
            return JoinGameMasterError::GAMEMANAGER_ERR_GAME_IN_PROGRESS;
        }

        return JoinGameMasterError::ERR_OK;
    }

    bool GameSessionMaster::validateJoinReputation(const UserSessionInfo& joiningUserInfo, GameEntryType entryType, JoinMethod joinMethod) const
    {
        if (gUserSessionManager->isReputationDisabled())
            return true;

        // players already successfully reserved into games will not re-check reputation on claim.
        if (entryType == GAME_ENTRY_TYPE_CLAIM_RESERVATION)
            return true;

        // check reputation. If dynamic reputation requirement enabled, join by invite bypasses this.
        if (!getGameSettings().getAllowAnyReputation() && joiningUserInfo.getHasBadReputation() &&
            !(getGameSettings().getDynamicReputationRequirement() && (joinMethod == JOIN_BY_INVITES)))
        {
            // we've already refreshed reputation before any request got to the master
            TRACE_LOG("[GameSessionMaster].validateJoinReputation() : Failed to join player(" << joiningUserInfo.getUserInfo().getId() <<
                ") using joinMethod " << JoinMethodToString(joinMethod) << " to game(" << getGameId() << "), failed reputation check.");

            return false;
        }
        return true;
    }

    JoinGameMasterError::Error GameSessionMaster::validateGameEntryCriteria(const JoinGameMasterRequest& joinRequest, EntryCriteriaError &entryCriteriaError,
                                                                            const char8_t* joiningRole, SlotType joiningSlotType) const
    {
        BlazeId joiningBlazeId = joinRequest.getUserJoinInfo().getUser().getUserInfo().getId();

        if (joiningSlotType == INVALID_SLOT_TYPE)
        {
            joiningSlotType = getSlotType(joinRequest.getJoinRequest().getPlayerJoinData(), joinRequest.getUserJoinInfo().getUser().getUserInfo());
        }

        // Restrict entry checks to only be bypassed when invited by an admin player
            // check the joining player against the game's game entry criteria
        if (!ignoreEntryCheckAdminInvited(joinRequest.getJoinRequest()) && !ignoreEntryCriteria(joinRequest.getJoinRequest().getJoinMethod(), joiningSlotType))
        {
            EntryCriteriaName failedEntryCriteria;
            if (!evaluateEntryCriteria(joiningBlazeId, joinRequest.getUserJoinInfo().getUser().getDataMap(), failedEntryCriteria))
            {
                TRACE_LOG("[GameSessionMaster] Player(" << joiningBlazeId << ") failed game entry criteria for game("
                          << getGameId() << "), failedEntryCriteria(" << failedEntryCriteria.c_str() << ")");
                    entryCriteriaError.setFailedCriteria(failedEntryCriteria.c_str());
                    return JoinGameMasterError::GAMEMANAGER_ERR_GAME_ENTRY_CRITERIA_FAILED;
            }

            // passed base entry criteria, check role-specific criteria
            if (joiningRole == nullptr)
            {
                joiningRole = lookupPlayerRoleName(joinRequest.getJoinRequest().getPlayerJoinData(), joiningBlazeId);
            }

            if (joiningRole && (blaze_stricmp(joiningRole, GameManager::PLAYER_ROLE_NAME_ANY) != 0))
            {
                RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.find(joiningRole);
                if (roleCriteriaIter != mRoleEntryCriteriaEvaluators.end())
                {
                    if (!roleCriteriaIter->second->evaluateEntryCriteria(joiningBlazeId, joinRequest.getUserJoinInfo().getUser().getDataMap(), failedEntryCriteria))
                    {
                        TRACE_LOG("[GameSessionMaster] Player(" << joiningBlazeId << ") failed game entry criteria for role '"
                            << joiningRole << "' in game(" << getGameId() << "), failedEntryCriteria(" << failedEntryCriteria.c_str() << ")");
                        entryCriteriaError.setFailedCriteria(failedEntryCriteria.c_str());
                        return JoinGameMasterError::GAMEMANAGER_ERR_ROLE_CRITERIA_FAILED;
                    }
                }
            }
            else
            {
                // need to test if this user passes *any* role entry criteria
                bool passedRoleEntryCriteria = false;
                RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.begin();
                RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaEnd = mRoleEntryCriteriaEvaluators.end();
                for (; roleCriteriaIter != roleCriteriaEnd; ++roleCriteriaIter)
                {
                    if (roleCriteriaIter->second->evaluateEntryCriteria(joiningBlazeId, joinRequest.getUserJoinInfo().getUser().getDataMap(), failedEntryCriteria))
                    {
                        passedRoleEntryCriteria = true;
                        break;
                    }
                }

                if (!passedRoleEntryCriteria)
                {
                    TRACE_LOG("[GameSessionMaster] Player(" << joiningBlazeId << ") failed game entry criteria for role '"
                        << joiningRole << "' in game(" << getGameId() << ").");
                    entryCriteriaError.setFailedCriteria(failedEntryCriteria.c_str());
                    return JoinGameMasterError::GAMEMANAGER_ERR_ROLE_CRITERIA_FAILED;
                }
            }
        }

        return JoinGameMasterError::ERR_OK;
    }

    JoinGameMasterError::Error GameSessionMaster::validateJoinAction(const JoinGameMasterRequest& joinGameMasterRequest) const
    {
        BlazeId joiningBlazeId = joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo().getId();

        // is this the host joining the game (or a regular player).
        ActionType joinActionType = PLAYER_JOIN_GAME;
        if ( joiningBlazeId == getTopologyHostInfo().getPlayerId() )
        {
            joinActionType = HOST_JOIN_GAME;

            if (getGameNetworkTopology() == CLIENT_SERVER_DEDICATED)
            {
                // the host of a dedicated server game cannot join the game as a player
                return JoinGameMasterError::GAMEMANAGER_ERR_DEDICATED_SERVER_HOST_CANNOT_JOIN;
            }

            // Topology host can't make a reservation.
            if (joinGameMasterRequest.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION)
            {
                TRACE_LOG("[GameSessionMaster] Topology host player(" << joiningBlazeId << ") is not allowed to join game("
                          << getGameId() << ") by making a reservation.");
                return JoinGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ENTRY_TYPE;
            }
        }
        else
        {
            switch(joinGameMasterRequest.getJoinRequest().getPlayerJoinData().getGameEntryType())
            {
            case GAME_ENTRY_TYPE_CLAIM_RESERVATION:
                joinActionType = PLAYER_CLAIM_RESERVATION;
                break;

            case GAME_ENTRY_TYPE_DIRECT:
                {
                    // We will attempt to claim a reservation, if we're already reserved in the game:
                    PlayerInfoMaster* joiningPlayer = mPlayerRoster.getPlayer(joiningBlazeId, joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo());
                    if (joiningPlayer && joiningPlayer->isReserved())
                        joinActionType = PLAYER_CLAIM_RESERVATION;
                    else
                        joinActionType = PLAYER_JOIN_GAME;
                }
                break;

            case GAME_ENTRY_TYPE_MAKE_RESERVATION:
                joinActionType = PLAYER_MAKE_RESERVATION;
                break;

            default:
                {
                    ERR_LOG("[GameSessionMaster] Unknown game entry type while validating join action for player(" << joiningBlazeId
                            << ") into game(" << getGameId() << ")");
                    EA_FAIL();
                    return JoinGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ENTRY_TYPE;
                }
            }
        }

        // is this 'type' of join allowed in the current game state?
        bool migratingFromUnresponsive = ((getGameState() == MIGRATING) && (mGameDataMaster->getPreHostMigrationState() == UNRESPONSIVE));
        BlazeRpcError err = gGameManagerMaster->getGameStateHelper()->isActionAllowed(this, joinActionType);
        if (err != ERR_OK || migratingFromUnresponsive)
        {
            TRACE_LOG("[GameSessionMaster] Invalid action(" << joinActionType << "), unable to join game(" << getGameId() << ")");
            
            if (isBusyGameState(getGameState()))
            {
                return JoinGameMasterError::GAMEMANAGER_ERR_GAME_BUSY;
            }
            else if (migratingFromUnresponsive)
            { 
                return JoinGameMasterError::GAMEMANAGER_ERR_UNRESPONSIVE_GAME_STATE;
            }
            else
            {
                return JoinGameMasterError::commandErrorFromBlazeError(err);
            }
        }

        return JoinGameMasterError::ERR_OK;
    }

    JoinGameMasterError::Error GameSessionMaster::validatePlayerJoinUserSessionExistence(const JoinGameMasterRequest& joinGameMasterRequest) const
    {
        if (isPseudoGame())
        {
            return JoinGameMasterError::ERR_OK;
        }

        // guard vs log outs *right* as they're about to try joining as active
        bool isValidGameEntryType = true;
        switch(joinGameMasterRequest.getJoinRequest().getPlayerJoinData().getGameEntryType())
        {
        case GAME_ENTRY_TYPE_CLAIM_RESERVATION:
            isValidGameEntryType = PlayerInfo::getSessionExists(joinGameMasterRequest.getUserJoinInfo().getUser());
            break;
        case GAME_ENTRY_TYPE_DIRECT:
            isValidGameEntryType = PlayerInfo::getSessionExists(joinGameMasterRequest.getUserJoinInfo().getUser());
            break;
        case GAME_ENTRY_TYPE_MAKE_RESERVATION:
            break;
        default:
            ERR_LOG("[GameSessionMaster] Unknown game entry type while validating user session existence for player(" << joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo().getId() << ") into game(" << getGameId() << ")");
            return JoinGameMasterError::GAMEMANAGER_ERR_INVALID_GAME_ENTRY_TYPE;
        }
        if (!isValidGameEntryType)
        {
            TRACE_LOG("[GameSessionMaster] player(" << joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo().getId() << ") currently has no user session. Cannot join game("
                << getGameId() << "), with game entry type " << GameEntryTypeToString(joinGameMasterRequest.getJoinRequest().getPlayerJoinData().getGameEntryType()) << ".");
            return JoinGameMasterError::GAMEMANAGER_ERR_USER_NOT_FOUND;
        }
        return JoinGameMasterError::ERR_OK;
    }

    /*!************************************************************************************************/
    /*! \brief finds a role that can be assigned as the 'any role' for the player
        \param[in] player - Player info
        \param[in] groupJoinData - Group join request's player join data. Used to count those in request that explicitly
            specified roles, to ensure there'd be enough space for them in the roles, before picking as the player's open role
        \param[in] desiredRoles - List of desired role names 
        \param[in/out] foundRoleName - Role name found for the player
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::findOpenRole(const PlayerInfoMaster& player, const PlayerJoinData* groupJoinData, const RoleNameList& desiredRoles, RoleName &foundRoleName) const
    {
        const RoleCriteriaMap &roleCriteriaMap = getRoleInformation().getRoleCriteriaMap();
        RoleNameList tempDesiredRoles;
        desiredRoles.copyInto(tempDesiredRoles); // Need a copy so we can shuffle it randomly

        if (!tempDesiredRoles.empty())
        {
            // Randomize our desired roles so that we don't inadvertently prefer some roles over others
            // There's a case to be made here for ordering the roles based on which roles are least desired amongst the group
            // such that we're more likely to choose roles that won't compete with other players
            // This can be looked as a future enhancement should a random selection prove unsatisfactory
            eastl::random_shuffle(tempDesiredRoles.begin(), tempDesiredRoles.end(), Random::getRandomNumber);

            for (const auto& desiredRole : tempDesiredRoles)
            {
                RoleCriteriaMap::const_iterator roleCriteriaItr = roleCriteriaMap.find(desiredRole);
                if (roleCriteriaItr != roleCriteriaMap.end())
                {
                    if (validateOpenRole(player, roleCriteriaItr, groupJoinData, foundRoleName))
                        return ERR_OK;
                }
            }
        }
        else
        {
            RoleCriteriaMap::const_iterator roleCriteriaItr = roleCriteriaMap.begin();
            RoleCriteriaMap::const_iterator roleCriteriaEnd = roleCriteriaMap.end();
            for (; roleCriteriaItr != roleCriteriaEnd; ++roleCriteriaItr)
            {
                if (validateOpenRole(player, roleCriteriaItr, groupJoinData, foundRoleName))
                    return ERR_OK;
            }
        }

        return GAMEMANAGER_ERR_ROLE_FULL;
    }

    bool GameSessionMaster::validateOpenRole(const PlayerInfoMaster& player, const RoleCriteriaMap::const_iterator& roleCriteriaItr, const PlayerJoinData* groupJoinData,
                                             RoleName &foundRoleName) const
    {
        const TeamIndex joiningTeamIndex = player.getTeamIndex();
        const PlayerRosterMaster *playerRoster = getPlayerRoster();
        const RoleName &currentRoleName = roleCriteriaItr->first;
        const RoleCriteria *roleCriteria = roleCriteriaItr->second;
        const uint16_t roleCapacity = roleCriteria->getRoleCapacity();

        uint16_t numInRole = playerRoster->getRoleSize(joiningTeamIndex, currentRoleName);

        // ensure enough room for possible remaining joining group members, before picking this role as an any role
        if (roleCapacity > (numInRole + countJoinersRequiringRole(joiningTeamIndex, currentRoleName, groupJoinData)))
        {
            // make sure that this role selection doesn't violate multirole criteria
            RoleSizeMap playerRoleRequirement;
            playerRoleRequirement[currentRoleName.c_str()] = 1;
            EntryCriteriaName failedEntryCriteria;
            if (!mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(getPlayerRoster(), playerRoleRequirement, joiningTeamIndex, failedEntryCriteria))
            {
                SPAM_LOG("[GameSessionMaster].validateOpenRole: Open role " << currentRoleName << " failed multirole entry criteria to enter game(" << getGameId()
                    << "), failedEntryCriteria(" << failedEntryCriteria << "), trying next role.");
            }
            else
            {
                // role entry criteria need to be tested
                RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.find(currentRoleName);
                if (roleCriteriaIter != mRoleEntryCriteriaEvaluators.end())
                {
                    if (!roleCriteriaIter->second->evaluateEntryCriteria(player.getPlayerId(), player.getExtendedDataMap(), failedEntryCriteria))
                    {
                        SPAM_LOG("[GameSessionMaster].validateOpenRole: Player(" << player.getPlayerId() << ") failed game entry criteria for role '"
                            << currentRoleName << "' in game(" << getGameId() << "), failedEntryCriteria(" << failedEntryCriteria << ")");
                    }
                }

                foundRoleName = currentRoleName;
                TRACE_LOG("[GameSessionMaster].validateOpenRole: Role(" << foundRoleName << ") found for player(" << player.getPlayerId() << "), in game(" << getGameId() << ").");
                return true;
            }
        }

        TRACE_LOG("[GameSessionMaster].validateOpenRole: Role(" << currentRoleName << ") is not available for player(" << player.getPlayerId() << ").");
        return false;
    }

    uint16_t GameSessionMaster::countJoinersRequiringRole(TeamIndex teamIndex, const RoleName &roleName, const PlayerJoinData* groupJoinData) const
    {
        uint16_t count = 0;
        if (groupJoinData != nullptr)
        {
            for (const auto& itr : groupJoinData->getPlayerDataList())
            {
                // for simplicity, no need to check whether user already joined
                if (isParticipantSlot(getSlotType(*groupJoinData, itr->getSlotType())) && (itr->getTeamIndex() == teamIndex))
                {
                    auto nextRole = lookupPlayerRoleName(*groupJoinData, itr->getUser().getBlazeId());
                    if ((nextRole != nullptr) && (blaze_stricmp(nextRole, roleName.c_str()) == 0))
                    {
                        ++count;
                    }
                }
            }
        }
        return count;
    }

    /*! ************************************************************************************************/
    /*! \brief Validations new players.  This does not apply to players who have already made a reservation.

        \param[in] joinGameRequest - the joining players request
        \return an error code.
    ***************************************************************************************************/
    JoinGameMasterError::Error GameSessionMaster::validateNewPlayerJoin(const JoinGameMasterRequest &joinGameMasterRequest) const
    {
        const JoinGameRequest &joinGameRequest = joinGameMasterRequest.getJoinRequest();

        // is the game open to joins from the player's joinMethod?
        if (!isSupportedJoinMethod(joinGameRequest.getJoinMethod(), joinGameMasterRequest))
        {
            return JoinGameMasterError::GAMEMANAGER_ERR_JOIN_METHOD_NOT_SUPPORTED;
        }

        // does the game allow single players to join (or only a game group)?
        // Note: group members go through pre-join first, and are allowed in due to the reservation
        if (getGameSettings().getEnforceSingleGroupJoin())
        {
            bool isJoiningAllowedGroup = false;
			UserGroupId groupId = joinGameMasterRequest.getUserJoinInfo().getUserGroupId();
            if (groupId != EA::TDF::OBJECT_ID_INVALID)
            {
                isJoiningAllowedGroup = isGroupAllowedToJoinSingleGroupGame(groupId);
            }
            // if the "single" player wasn't invited (and they don't have a reservation or system reason for joining the game),
            //   stop their join
            // GM_AUDIT: - brittle, use mode flag or something
            if ( (joinGameRequest.getJoinMethod() != JOIN_BY_INVITES) && (joinGameRequest.getJoinMethod() < SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME) && !isJoiningAllowedGroup)
            {
                return JoinGameMasterError::GAMEMANAGER_ERR_ENFORCING_SINGLE_GROUP_JOINS;
            }
        }

        return JoinGameMasterError::ERR_OK;
    }

    JoinGameMasterError::Error GameSessionMaster::validateClaimReservationJoin(const PlayerInfoMaster* joiningPlayer, BlazeId joiningBlazeId) const
    {
        // Can't claim a reservation you don't have.
        if (joiningPlayer == nullptr || !joiningPlayer->isReserved())
        {
            TRACE_LOG("[GameSessionMaster] No reservation found for player(" << joiningBlazeId << ") in game(" << getGameId() << ") to claim.");
            return JoinGameMasterError::GAMEMANAGER_ERR_NO_RESERVATION_FOUND;
        }

        return JoinGameMasterError::ERR_OK;
    }

    JoinGameMasterError::Error GameSessionMaster::validatePlayerJoinTeamGame(const JoinGameMasterRequest &joinGameMasterRequest) const
    {
        TeamIndex joiningTeamIndex = getTeamIndex(joinGameMasterRequest.getJoinRequest().getPlayerJoinData(), joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo());

        // Validate that specific team id/index values are allowed for this game if requested.

        // Requested a specific index that is not valid for this game.
        if (joiningTeamIndex != UNSPECIFIED_TEAM_INDEX && joiningTeamIndex != TARGET_USER_TEAM_INDEX)
        {
            if (joiningTeamIndex >= getTeamCount())
            {
                // tried to join team with an out of range team index
                TRACE_LOG("[GameSessionMaster] Player(" << joinGameMasterRequest.getUserJoinInfo().getUser().getUserInfo().getId() << ") unable to join Game(" << getGameId()
                          << ") because team index(" << joiningTeamIndex << ") is not a valid; outside of team count(" << getTeamCount() << ").");
                return JoinGameMasterError::GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
            }
        }

        return JoinGameMasterError::ERR_OK;
    }


    /*! ************************************************************************************************/
    /*! \brief return the JoinGame Error if the joining player is an existing game member (error code depends
                on the player's state (already game member vs already in queue)
    ***************************************************************************************************/
    JoinGameMasterError::Error GameSessionMaster::validateExistingPlayerJoinGame(const GameEntryType& gameEntryType, const PlayerInfoMaster &existingPlayer) const
    {
        switch (existingPlayer.getPlayerState())
        {
        case RESERVED:
            {
                // Only trigger the error when a player tries to reserve multiple times
                if (gameEntryType != GAME_ENTRY_TYPE_MAKE_RESERVATION)
                {
                    return JoinGameMasterError::ERR_OK;
                }
                else
                {
                    // Multiple reservations is an error because this may not be the connection that originally made the reservation,
                    // and as such, the BlazeSDK would not have received an notifyGameSetup event, and would not be connected to the mesh.
                    return JoinGameMasterError::GAMEMANAGER_ERR_RESERVATION_ALREADY_EXISTS;
                }
            }
        case ACTIVE_CONNECTING:
        case ACTIVE_MIGRATING:
        case ACTIVE_CONNECTED:
        case ACTIVE_KICK_PENDING:
            {
                return JoinGameMasterError::GAMEMANAGER_ERR_ALREADY_GAME_MEMBER;
            }
        case QUEUED:
            {
                return JoinGameMasterError::GAMEMANAGER_ERR_ALREADY_IN_QUEUE;
            }
        default:
            {
                ERR_LOG("[GameSessionMaster].validateExistingPlayerJoinGame - unsupported player state = " << existingPlayer.getPlayerState()
                        << ", player id = " << existingPlayer.getPlayerId());
                EA_FAIL();
                return JoinGameMasterError::ERR_SYSTEM;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief attempt to join the game as a new player (ie: someone w/o an existing player reservation).
                Performs final validation before joining player into game (or the game's queue, if full).

        \param[in] joinGameRequest
        \param[in] playerReplicationReason - the replication reason tdf to use when inserting the player into the map.  Can be nullptr.
        \param[out] joiningPlayer - pointer reference that returns the newly allocated/joined player.  nullptr on failure.
    ***************************************************************************************************/
    JoinGameMasterError::Error GameSessionMaster::joinGameAsNewPlayer(const JoinGameMasterRequest &joinGameRequest, JoinGameMasterResponse &joinGameResponse,
            const GameSetupReason &playerJoinContext, PlayerInfoMasterPtr& joiningPlayer)
    {
        EA_ASSERT(joiningPlayer == nullptr);

        BlazeId joiningBlazeId = joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getId();

        // We calculate the actual joining slot type in cases where private is full, you can still join public
        SlotType actualSlotType = getSlotType(joinGameRequest.getJoinRequest().getPlayerJoinData(), joinGameRequest.getUserJoinInfo().getUser().getUserInfo());

        // We calculate your joining team index for you if not properly defined.
        TeamIndex joiningTeamIndex = getTeamIndex(joinGameRequest.getJoinRequest().getPlayerJoinData(), joinGameRequest.getUserJoinInfo().getUser().getUserInfo());

        // Target team user wants to join - based on specified player.
        bool targetInQueue = false;
        PlayerId targetPlayerId = joinGameRequest.getJoinRequest().getUser().getBlazeId();
        if ((joiningTeamIndex == TARGET_USER_TEAM_INDEX) && (targetPlayerId != INVALID_BLAZE_ID))
        {
            PlayerInfo* playerInfo = getPlayerRoster()->getPlayer(targetPlayerId);
            if (playerInfo != nullptr)
            {
                joiningTeamIndex = playerInfo->getTeamIndex();
                targetInQueue = playerInfo->isInQueue();
            }
            else   // We specifically use an else here (rather than switching to UNSPECIFIED_TEAM_INDEX) so that the player will attempt to enter the QUEUE if the target player is in it. 
            {
                joiningTeamIndex = UNSPECIFIED_TEAM_INDEX;
            }
        }

		const UserGroupId playerGroupId = joinGameRequest.getUserJoinInfo().getUserGroupId();

        // Only do this 'join the host's team' logic if a team was not specified for the joining user:
        if (joiningTeamIndex == UNSPECIFIED_TEAM_INDEX)
        {
            bool foundExistingGroup = false;
            (void) foundExistingGroup;
            PlayerInfoMaster* targetPlayerInfoMaster = getTargetPlayer(playerGroupId, foundExistingGroup);
            if (targetPlayerInfoMaster != nullptr)
            {
                joiningTeamIndex = targetPlayerInfoMaster->getTeamIndex();
            }
        }

        JoinGameMasterError::Error joinCapacityError;
        // The game is not full, but there are people waiting to be dequeued in the queue.
        // We don't allow new players trying to join to take those slots, or be put on specific teams now.
        // The team information will be different once they have a chance to join, so we will
        // mitigate it then.
        if ( isQueueEnabled() && !isQueueBypassPossible(actualSlotType))
        {
            // we check this in validateJoinGameCapacities for other players, but we need to do a bounds check here
            if (actualSlotType >= MAX_SLOT_TYPE)
            {
                // out of bounds gets defaulted to public slot
                WARN_LOG("[GameSessionMaster].joinGameAsNewPlayer: Player(" << joiningBlazeId << ") trying to queue into game(" << getGameId() << ") specified an out of bounds SlotType value (" << actualSlotType << ") defaulting to (" << SlotTypeToString(SLOT_PUBLIC_PARTICIPANT) << ").");
                actualSlotType = SLOT_PUBLIC_PARTICIPANT;
            }

            joinCapacityError = (JoinGameMasterError::Error) (getSlotFullErrorCode(actualSlotType));
            if (playerJoinContext.getMatchmakingSetupContext() != nullptr || playerJoinContext.getIndirectMatchmakingSetupContext() != nullptr)
            {
                // enforce matchmaking not being able to join the queue.
                TRACE_LOG("[GameSessionMaster].joinGameAsNewPlayer: Player(" << joiningBlazeId << ") unable to join game(" << getGameId() << ") because there is a queue and they are entering via matchmaking.");
                return joinCapacityError;
            }
        }
        else
        {
            if (isParticipantSlot(actualSlotType))
            {
                const char8_t* joiningRole = lookupPlayerRoleName(joinGameRequest.getJoinRequest().getPlayerJoinData(), joinGameRequest.getUserJoinInfo().getUser());
                if (!joiningRole)
                {
                    TRACE_LOG("[GameSessionMaster].joinGameAsNewPlayer: Player(" << joiningBlazeId << ") unable to join game(" << getGameId()
                        << ") for no player role specified, error("
                        << (ErrorHelp::getErrorName(GAMEMANAGER_ERR_ROLE_NOT_ALLOWED)) << ")");
                    return JoinGameMasterError::GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
                }

                joinCapacityError = validateJoinGameCapacities(joiningRole, actualSlotType, joiningTeamIndex, joiningBlazeId, actualSlotType, joiningTeamIndex, targetInQueue);
            }
            else // spectator, skip team checks
            {
                joinCapacityError = validateJoinGameSlotType(actualSlotType, actualSlotType);
                // always force the team index to be unset for spectators
                joiningTeamIndex = UNSPECIFIED_TEAM_INDEX;
            }

            if (joinCapacityError != JoinGameMasterError::ERR_OK)
            {
                //if the game was full, first process some logic to determine if the join should fail, or the player can be queued.
                if (isQueueableJoinError(joinCapacityError) && 
                    ((isParticipantSlot(actualSlotType) && getTotalParticipantCapacity() > 0) || (isSpectatorSlot(actualSlotType) && getTotalSpectatorCapacity() > 0)) )
                {
                    if (!isQueueEnabled())
                    {
                        TRACE_LOG("[GameSessionMaster].joinGameAsNewPlayer: Player(" << joiningBlazeId << ") unable to join gameId " << getGameId()
                                  << " as " << SlotTypeToString(actualSlotType)
                                  << " because the game is full and doesn't support queuing.");
                        return joinCapacityError;
                    }
                    else if (playerJoinContext.getMatchmakingSetupContext() != nullptr || playerJoinContext.getIndirectMatchmakingSetupContext() != nullptr)
                    {
                        // enforce matchmaking not being able to join the queue.
                        TRACE_LOG("[GameSessionMaster] Player(" << joiningBlazeId << ") unable to join game(" << getGameId() << ") because the game is full and they are entering via matchmaking.");
                        return joinCapacityError;
                    }
                    // else we fall through and the player will join the queue
                }
                else
                {
                    // Always return the error for non full related errors
                    return joinCapacityError;
                }
            }
        }

        // Joining a virtual game chooses a dedicated host to host the game.
        if ((getGameState() == INACTIVE_VIRTUAL) && (joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType() != GAME_ENTRY_TYPE_MAKE_RESERVATION))
        {
            if (!gUserSessionManager->getSessionExists(joinGameRequest.getUserJoinInfo().getUser().getSessionId()))
            {
                eastl::string platformInfoStr;
                TRACE_LOG("[GameSessionMaster].joinGameAsNewPlayer: The player(blazeId=" << joiningBlazeId << ", platformInfo=(" << platformInfoToString(joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo(), platformInfoStr) <<
                    ")) does not have a user session and cannot join game(" << getGameId() << ") because it the game is in virtual state and requires player to have user session in order to be injected as host.");
                return JoinGameMasterError::GAMEMANAGER_ERR_NO_HOSTS_AVAILABLE_FOR_INJECTION;
            }

            TRACE_LOG("[GameSessionMaster].joinGameAsNewPlayer: Attempting host injection for game(" << getGameId() << ").");
            // add a remote host injection job, let join continue without topology host
            gGameManagerMaster->addHostInjectionJob(*this, joinGameRequest);
        }

        // create a new player, joining either the game or the game queue
        if (PlayerInfo::getSessionExists(joinGameRequest.getUserJoinInfo().getUser()) || isPseudoGame())
        {
            joiningPlayer = BLAZE_NEW PlayerInfoMaster(joinGameRequest, *this);
        }
        else
        {
            // an external player. create place holder player object
            const char8_t* rolename = lookupPlayerRoleName(joinGameRequest.getJoinRequest().getPlayerJoinData(), joinGameRequest.getUserJoinInfo().getUser());
            BlazeRpcError err = mPlayerRoster.createExternalPlayer(actualSlotType, joiningTeamIndex, rolename,
				joinGameRequest.getUserJoinInfo().getUserGroupId(), joinGameRequest.getUserJoinInfo().getUser(),
                GameManager::getEncryptedBlazeId(joinGameRequest.getJoinRequest().getPlayerJoinData(), joinGameRequest.getUserJoinInfo().getUser().getUserInfo()),
                joiningPlayer);
            if (err != ERR_OK)
            {
                return JoinGameMasterError::commandErrorFromBlazeError(err);
            }
            joiningPlayer->setScenarioInfo(joinGameRequest.getUserJoinInfo().getScenarioInfo());
        }

        if (blaze_stricmp(joiningPlayer->getRoleName(), GameManager::PLAYER_ROLE_NAME_ANY) == 0)
        {
            const PerPlayerJoinData* ppJoinData = lookupPerPlayerJoinDataConst(joinGameRequest.getJoinRequest().getPlayerJoinData(),
                joinGameRequest.getUserJoinInfo().getUser().getUserInfo());
            if (ppJoinData != nullptr)
            {
                RoleName newRoleName;
                BlazeRpcError findRoleError = findOpenRole(*joiningPlayer, &joinGameRequest.getJoinRequest().getPlayerJoinData(), ppJoinData->getRoles(), newRoleName);
                if (findRoleError != ERR_OK)
                {
                    joinCapacityError = JoinGameMasterError::commandErrorFromBlazeError(findRoleError);
                }

                joiningPlayer->setRoleName(newRoleName);
            }
            else
            {
                // This should never happen
                eastl::string platformInfoStr;
                ERR_LOG("[GameSessionMaster].joinGameAsNewPlayer: Unable to find playerJoinData for external user with platformInfo (" << 
                    platformInfoToString(joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo(), platformInfoStr) << ") in game (" << getGameId() << ").");
                return JoinGameMasterError::GAMEMANAGER_ERR_USER_NOT_FOUND;
            }
        }


        joiningPlayer->getPlayerData()->setSlotType(actualSlotType);
        // need to set the team index in case player tried to join with TeamId.
        joiningPlayer->getPlayerData()->setTeamIndex(joiningTeamIndex);
        // need to set SlotId in case player tried to join with SlotId
        joiningPlayer->getPlayerData()->setSlotId(joinGameRequest.getJoinRequest().getRequestedSlotId());

        joinGameResponse.getJoinResponse().setJoinState(JOINED_GAME);

        // The game was full, but queuing is enabled (checked above)
        if (isQueueableJoinError(joinCapacityError))
        {
            // Check if the original team index was targeted:  (The joiningTeamIndex will have already been updated)
            if (getTeamIndex(joinGameRequest.getJoinRequest().getPlayerJoinData(), joinGameRequest.getUserJoinInfo().getUser().getUserInfo()) == TARGET_USER_TEAM_INDEX)
            {
                // User attempted to join target user's team.
                joiningPlayer->getPlayerData()->setTeamIndex(TARGET_USER_TEAM_INDEX);
                joiningPlayer->getPlayerData()->setTargetPlayerId(joinGameRequest.getJoinRequest().getUser().getBlazeId());
            }

            joinGameResponse.getJoinResponse().setJoinState(IN_QUEUE);

            if (joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION)
            {
                joiningPlayer->setPlayerState(RESERVED);
            }
            else
            {
                joiningPlayer->setPlayerState(QUEUED);
            }
            BlazeRpcError joinQueueError = addQueuedPlayer(*joiningPlayer, playerJoinContext);
            if (joinQueueError != ERR_OK)
            {
                joiningPlayer = nullptr;
                return JoinGameMasterError::commandErrorFromBlazeError(joinQueueError);
            }
        }
        else if (joinGameRequest.getJoinRequest().getPlayerJoinData().getGameEntryType() == GAME_ENTRY_TYPE_MAKE_RESERVATION)
        {

            BlazeRpcError blazeError = addReservedPlayer(joiningPlayer.get(), playerJoinContext);
            if (blazeError != ERR_OK)
            {
                WARN_LOG("[GameSessionMaster].joinGameAsNewPlayer: Player(" << joiningPlayer->getPlayerId() << ") failed to make reservation in game("
                         << getGameId() << "), error " << (ErrorHelp::getErrorName(blazeError)));
                joiningPlayer = nullptr;
                return JoinGameMasterError::commandErrorFromBlazeError(blazeError);
            }
        }
        else
        {

            // game is not full, try adding myself to the roster
            BlazeRpcError blazeRpcError = addActivePlayer(joiningPlayer.get(), playerJoinContext);
            if (blazeRpcError != ERR_OK)
            {
                WARN_LOG("[GameSessionMaster].joinGameAsNewPlayer: Failed to join game(" << getGameId() << ") when trying to add player(" << joiningPlayer->getPlayerId()
                         << "), error " << (ErrorHelp::getErrorName(blazeRpcError)));

                joiningPlayer = nullptr;
                return JoinGameMasterError::commandErrorFromBlazeError(blazeRpcError);
            }
        }

        // Now we know that the player join is successful.
        // If the new player is part of a group, is the first to get the game and will not get queued we associate the group id with this player so that the members who get queued can target
        // the group of this player. The players who did not get queued already end up on the same team because validateGroupJoin changes their target team.
        // Note that this player is NOT necessarily the leader of the group.
        if (getGameType() == GAME_TYPE_GAMESESSION)
        {
            if (playerGroupId != EA::TDF::OBJECT_ID_INVALID)
            {
                if (isQueueableJoinError(joinCapacityError))
                {
                    addQueuedPlayerCountForGroup(playerGroupId);
                }
                else
                {
                    // Only add the player if we have a group of more than 1 players otherwise the player has already joined the game.
                    if (mGameDataMaster->getGroupJoinInProgress() && joinGameRequest.getJoinRequest().getPlayerJoinData().getPlayerDataList().size() > 1)
                    {
                        addTargetPlayerIdForGroup(playerGroupId,joiningBlazeId);
                    }
                }
            }
        }            
         
        // if we locked game for preferred joins (for a prior leave), and now filled the slots, clear the lock.
        if (isGameAndQueueFull())
            clearLockForPreferredJoins();

        // Send the completed join message for players that are already Connected (local player #2+)
        if (joiningPlayer->isConnected() && getGameNetworkTopology() != NETWORK_DISABLED)  // Network disabled users already were notified via addActivePlayer
        {
            sendPlayerJoinCompleted(*joiningPlayer);
        }

        return JoinGameMasterError::ERR_OK;
    }

    JoinGameMasterError::Error GameSessionMaster::validateJoinGameCapacities(const RoleName& requiredRole, const SlotType requestedSlotType, TeamIndex requestedTeamIndex, const BlazeId joiningPlayerId, 
                                                                             SlotType& suggestedSlotType, TeamIndex& outUnspecifiedTeamIndex, bool targetPlayerInQueue) const
    {
        if (isParticipantSlot(requestedSlotType))
        {
            // only validate team capacities for players, not spectators
            TeamIndexRoleSizeMap joiningRoleMap;
            joiningRoleMap[requestedTeamIndex][requiredRole] = 1;
            // First check for any teams related errors
            JoinGameMasterError::Error validateTeamInfoError = validateJoinTeamGameTeamCapacities(joiningRoleMap, joiningPlayerId, 1, outUnspecifiedTeamIndex, targetPlayerInQueue);
            if (validateTeamInfoError != JoinGameMasterError::ERR_OK)
            {
                return validateTeamInfoError;
            }
        }

        // Now check for our slot cap capacities.
        return validateJoinGameSlotType(requestedSlotType, suggestedSlotType);
    }

    /*! ************************************************************************************************/
    /*! \brief attempt to join the game as an existing player (ie: someone with a reservation)
    Performs final validation before joining player into game (or the game's queue, if full).

    \param[in] joinGameRequest
    \param[in] joiningUserSession
    \param[in] playerReplicationReason - the replication reason tdf to use when inserting the player into the map.  Can be nullptr.
    \param[in/out] joiningPlayer - the joining player, modified by the join process
    ***************************************************************************************************/
    JoinGameMasterError::Error GameSessionMaster::joinGameClaimReservation(const JoinGameMasterRequest &joinGameRequest,
        const GameSetupReason &playerJoinContext, PlayerInfoMaster &joiningPlayer)
    {
        BlazeId joiningBlazeId = joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getId();

        bool previousTeamIndexUnspecified = (joiningPlayer.getTeamIndex() == UNSPECIFIED_TEAM_INDEX);
        ConnectionGroupId previousConnGroupId = joiningPlayer.getConnectionGroupId();
        SlotId connectionSlotId = joiningPlayer.getConnectionSlotId();

        // claim an existing player reservation
        joiningPlayer.setupJoiningPlayer(joinGameRequest);

        // conn group id can change on claim (after setupJoiningPlayer above), ensure its conn slot id tracking updated.
        if ((previousConnGroupId != INVALID_CONNECTION_GROUP_ID) &&
            (previousConnGroupId != joiningPlayer.getConnectionGroupId()) && !mPlayerRoster.hasPlayersOnConnection(previousConnGroupId))
        {
            markConnectionSlotOpen(connectionSlotId, previousConnGroupId);
            // new conn group's conn slot id is assigned later at addActivePlayer()
        }

        // spectator should already be set to UNSPECIFIED_TEAM_INDEX, but we make sure here
        TeamIndex joiningTeamIndex = joiningPlayer.isSpectator()? UNSPECIFIED_TEAM_INDEX : joiningPlayer.getTeamIndex();
        TeamIndex joiningTeamIndexFromRequest = getTeamIndex(joinGameRequest.getJoinRequest().getPlayerJoinData(), joinGameRequest.getUserJoinInfo().getUser().getUserInfo());

        // Ensure that a reserved player is taking the slot that they reserved if the request is coming from the client (clients have only one team for all players).
        // Avoid the check for matchmaking context as the team index for each player may be different than the global team index retrieved via PlayerJoinData.
        if (!joiningPlayer.getJoinedViaMatchmaking())
        {
            if ((joiningTeamIndexFromRequest != UNSPECIFIED_TEAM_INDEX) && (joiningPlayer.getTeamIndex() != UNSPECIFIED_TEAM_INDEX) &&
                (joiningTeamIndexFromRequest != joiningPlayer.getTeamIndex()))
            {
                WARN_LOG("[GameSessionMaster] Player(" << joiningBlazeId << ") claiming reservation in game(" << getGameId()
                    << ") requested team(" << joiningTeamIndexFromRequest << ") which does not match reservation team("
                    << joiningPlayer.getTeamIndex() << ").  This likely indicates a bug in Blaze.");
                // We allow the player to fall through and claim the reservation ignoring the team
                // they specified in the join request.  We warn here to let teams know this is not desired behavior.
            }
        }

        // If we specify the team index then verify that it is valid.
        // During CreateGame, reserved players who are not part of the creator's game group,
        // will have INVALID_TEAM_ID, UNSPECIFIED_TEAM_INDEX for their team information.
        // We pick their team here. (Note this step only applies to matchmaker CG. Regular non-MM
        // GM joiners would have had their joiningTeamIndex set when making the reservation).
        // Skip this check if the player is currently queued, as a team may not have been set,
        // and we expect that all the teams will still be full.
        // Also skip this check if the user is a spectator, as a spectator has no team
        bool wasQueued = (getPlayerRoster()->getQueuePlayer(joiningPlayer.getPlayerId()) != nullptr);
        if (!joiningPlayer.isSpectator())
        {
            if (joiningTeamIndex == UNSPECIFIED_TEAM_INDEX)
            {
                // matchmaker sent the finalized team in this join request, so we can use it here.
                if (joiningTeamIndexFromRequest == UNSPECIFIED_TEAM_INDEX)
                {
                    TRACE_LOG("[GameSessionMaster].joinGameClaimReservation MM did not send a specific team index for the join game("
                        << getGameId() << ") for player(" << joiningPlayer.getPlayerId() << "). One will be determined by validateJoinTeamGameTeamCapacities.");
                }
                joiningTeamIndex = joiningTeamIndexFromRequest;

                uint16_t joiningPlayerCount = 1;
                TeamIndexRoleSizeMap teamRoleRequirements;
                gGameManagerMaster->buildTeamIndexRoleSizeMapFromJoinRequest(joinGameRequest.getJoinRequest(), teamRoleRequirements, joiningPlayerCount);
            
                TeamIndex unspecifiedTeamIndex = UNSPECIFIED_TEAM_INDEX;
                JoinGameMasterError::Error validateTeamInfoError = validateJoinTeamGameTeamCapacities(teamRoleRequirements, joiningBlazeId, joiningPlayerCount, unspecifiedTeamIndex);
                if (validateTeamInfoError != JoinGameMasterError::ERR_OK)
                {
                    if (!wasQueued)
                    {
                        return validateTeamInfoError;
                    }
                }
                else if (joiningTeamIndex == UNSPECIFIED_TEAM_INDEX)
                {
                    // Set the TeamIndex with the values chosen from the arbitrated team values.
                    joiningPlayer.getPlayerData()->setTeamIndex(unspecifiedTeamIndex);
                }
            }

            if (blaze_stricmp(joiningPlayer.getPlayerData()->getRoleName(), GameManager::PLAYER_ROLE_NAME_ANY) == 0)
            {
                const PerPlayerJoinData* ppJoinData = lookupPerPlayerJoinDataConst(joinGameRequest.getJoinRequest().getPlayerJoinData(), joiningPlayer.getUserInfo().getUserInfo());
                if (ppJoinData != nullptr)
                {
                    RoleName newRoleName;
                    BlazeRpcError findRoleError = findOpenRole(joiningPlayer, &joinGameRequest.getJoinRequest().getPlayerJoinData(), ppJoinData->getRoles(), newRoleName);
                    if (findRoleError != ERR_OK)
                    {
                        if (!wasQueued)
                        {
                            WARN_LOG("[GameSessionMaster].joinGameClaimReservation: Failed to join game(" << getGameId() << ") when trying to add player(" << joiningPlayer.getPlayerId()
                                << "), error " << (ErrorHelp::getErrorName(findRoleError)) << " when picking a role.");
                            return JoinGameMasterError::commandErrorFromBlazeError(findRoleError);
                        }
                    }
                    else
                    {
                        joiningPlayer.getPlayerData()->setRoleName(newRoleName.c_str());
                    }
                }
                else
                {
                    // This should never happen
                    ERR_LOG("[GameSessionMaster].joinGameClaimReservation: Unable to find playerJoinData for user (" << joiningPlayer.getPlayerId() << ") in game (" << getGameId() << ").");
                    return JoinGameMasterError::GAMEMANAGER_ERR_USER_NOT_FOUND;
                }
            }
        }

        // Joining a virtual game chooses a dedicated host to host the game.
        if (getGameState() == INACTIVE_VIRTUAL)
        {
            if (!gUserSessionManager->getSessionExists(joinGameRequest.getUserJoinInfo().getUser().getSessionId()))
            {
                eastl::string platformInfoStr;
                TRACE_LOG("[GameSessionMaster].joinGameClaimReservation: The player(blazeId=" << joiningBlazeId << ", platformInfo=(" << platformInfoToString(joinGameRequest.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo(), platformInfoStr) <<
                    ")) does not have a user session and cannot join game(" << getGameId() << ") because it the game is in virtual state and requires player to have user session in order to be injected as host.");
                return JoinGameMasterError::GAMEMANAGER_ERR_NO_HOSTS_AVAILABLE_FOR_INJECTION;
            }

            TRACE_LOG("[GameSessionMaster].joinGameClaimReservation: Attempting host injection for game(" << getGameId() << ").");
            // add a remote host injection job, let join continue without topology host
            gGameManagerMaster->addHostInjectionJob(*this, joinGameRequest);
        }

        // add the reserved player as an actual player
        BlazeRpcError blazeRpcError = addActivePlayer(&joiningPlayer, playerJoinContext, previousTeamIndexUnspecified);
        if (blazeRpcError != ERR_OK)
        {
            WARN_LOG("[GameSessionMaster] Failed to join game(" << getGameId() << ") when trying to add player(" << joiningPlayer.getPlayerId()
                     << "), error " << (ErrorHelp::getErrorName(blazeRpcError)));
            return JoinGameMasterError::commandErrorFromBlazeError(blazeRpcError);
        }

        if (wasQueued)
        {
            if (shouldPumpServerManagedQueue())
            {
                processPlayerServerManagedQueue();
            }
        }

        return JoinGameMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief check that there's room for me to join, and see if I need to change my slotType.
    ***************************************************************************************************/
    JoinGameMasterError::Error GameSessionMaster::validateJoinGameSlotType(const SlotType requestedSlotType, SlotType& suggestedSlotType) const
    {
        suggestedSlotType = requestedSlotType;
        if ( isSlotTypeFull(requestedSlotType) )
        {
            if (isParticipantSlot(requestedSlotType))
            {
                if ( (requestedSlotType == SLOT_PUBLIC_PARTICIPANT) || isSlotTypeFull(SLOT_PUBLIC_PARTICIPANT) )
                {
                    // game is full - neither my requested nor a public slot is available
                    return (JoinGameMasterError::Error) (getSlotFullErrorCode(requestedSlotType));
                }
                else
                {
                    // our requested slot wasn't available, but a public one is -- take it instead
                    suggestedSlotType = SLOT_PUBLIC_PARTICIPANT;
                }
            }
            else // player is spectator
            {
                if ( (requestedSlotType == SLOT_PUBLIC_SPECTATOR) || isSlotTypeFull(SLOT_PUBLIC_SPECTATOR) )
                {
                    // game is full - neither my requested nor a public spectator slot is available
                    return (JoinGameMasterError::Error) (getSlotFullErrorCode(requestedSlotType));
                }
                else
                {
                    // our requested slot wasn't available, but a public one is -- take it instead
                    suggestedSlotType = SLOT_PUBLIC_SPECTATOR;
                }
            }
        }

        return JoinGameMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief if there's room in the queue, add them to this game's player queue.

        \param[in] newPlayer the player to try to add to the game's player queue
        \param[in] playerplayerReplicationReason - the replication reason to use when inserting the player into the roster (can be nullptr)
        \return ERR_OK on success
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::addQueuedPlayer(PlayerInfoMaster& newPlayer, const GameSetupReason &playerJoinContext)
    {
        if (isQueueFull())
        {
            return GAMEMANAGER_ERR_QUEUE_FULL;
        }

        if (!mPlayerRoster.addNewPlayerToQueue(newPlayer, playerJoinContext))
        {
            return GAMEMANAGER_ERR_ALREADY_IN_QUEUE;
        }

        newPlayer.getPlayerData()->setJoinedViaMatchmaking((playerJoinContext.getMatchmakingSetupContext() != nullptr) || (playerJoinContext.getIndirectMatchmakingSetupContext() != nullptr));

        // add the game to the player's session game info map,
        //   since we're queued, no need to modify the activeGameCount
        if (PlayerInfo::getSessionExists(newPlayer.getUserInfo()))
            gGameManagerMaster->addUserSessionGameInfo(newPlayer.getPlayerSessionId(), *this);

        TRACE_LOG("[GameSessionMaster] Player(" << newPlayer.getPlayerId() << ":" << newPlayer.getPlayerName()
                  << ") added to queue for game(" << getGameId() << ")");

        if (shouldPumpServerManagedQueue())
        {
            processPlayerServerManagedQueue();
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Adds a reserved player to the games roster (not the queue).  Use addQueuedPlayer and set the player
        state to RESERVED to add a reserved player to the queue.  This reserved player will not establish network connections.

        \param[in] newPlayer - the player to add
        \param[in] playerJoinContext - the context in which the player is joining.
        \return ERR_OK on success
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::addReservedPlayer(PlayerInfoMaster* newPlayer, const GameSetupReason& playerJoinContext, bool wasQueued /*= false */)
    {
        // A reserved player takes up a slot if they are reserved in a game slot.
        assignPlayerToFirstOpenSlot(*newPlayer);
        // reserved players do not occupy a connection slot, since they (potentially) have not yet connected
        // They will be given one when they claim their reservation.
        newPlayer->getPlayerData()->setConnectionSlotId(DEFAULT_JOINING_SLOT);
        newPlayer->getPlayerData()->setJoinedViaMatchmaking((playerJoinContext.getMatchmakingSetupContext() != nullptr) || (playerJoinContext.getIndirectMatchmakingSetupContext() != nullptr));

        if (wasQueued)
        {
            if (!mPlayerRoster.promoteQueuedPlayer(newPlayer, playerJoinContext))
            {
                return ERR_SYSTEM;
            }
        }
        else
        {
            if (!mPlayerRoster.addNewReservedPlayer(*newPlayer, playerJoinContext))
            {
                return GAMEMANAGER_ERR_ALREADY_GAME_MEMBER;
            }
        }

        mMetricsHandler.onPlayerAdded(*newPlayer, false);

        return ERR_OK;
    }


    /*! ************************************************************************************************/
    /*! \brief create player for each member in user group including the leader, and join them to the game.

        \return JoinGameByGroupMasterError
    ***************************************************************************************************/
    JoinGameByGroupMasterError::Error GameSessionMaster::joinGameByGroup(const JoinGameByGroupMasterRequest& request, JoinGameMasterResponse& response,
                                                                  Blaze::EntryCriteriaError& error,
                                                                  const UserSessionSetupReasonMap &setupReasons)
    {
        UserJoinInfoPtr hostJoinInfo = nullptr;
        const UserSessionInfo* leaderInfo = getHostSessionInfo(request.getUsersInfo(), &hostJoinInfo);
        if (leaderInfo == nullptr)
        {
            WARN_LOG("[GameSessionMaster] Attempt to join game(" << request.getJoinRequest().getGameId()
                << ") without host/leader of the joining group.");
            return JoinGameByGroupMasterError::ERR_SYSTEM;
        }

        UserSessionId leaderSessionId = leaderInfo->getSessionId();
        BlazeId leaderBlazeId = leaderInfo->getUserInfo().getId();

        // Join the leader to the game first. Without success here we bail on everyone else.
        UserSessionSetupReasonMap::const_iterator reasonIter = setupReasons.find(leaderSessionId);
        if (reasonIter == setupReasons.end())
        {
            WARN_LOG("[GameSessionMaster] Attempt to join game(" << request.getJoinRequest().getGameId()
                     << ") by group with no setup reason for leader session(" << leaderSessionId << ").");
            return JoinGameByGroupMasterError::ERR_SYSTEM;
        }
        GameSetupReason& reason = *reasonIter->second;

        JoinGameMasterRequest joinMasterRequest;
        joinMasterRequest.setJoinRequest(const_cast<JoinGameRequest&>(request.getJoinRequest()));
        joinMasterRequest.setUserJoinInfo(*hostJoinInfo);
        joinMasterRequest.setExternalSessionData(const_cast<ExternalSessionMasterRequestData&>(request.getExternalSessionData()));

        // GM_AUDIT: we don't validatePlayerJoin until the joinGame call.  This is where we validate most
        // of our arguments in the join request.  validateGroupJoin does not check those.
        BlazeRpcError err = validateGroupJoinSetTeams(const_cast<JoinGameByGroupMasterRequest&>(request), leaderBlazeId);
        if (err != JoinGameByGroupMasterError::ERR_OK)
        {
            return JoinGameByGroupMasterError::commandErrorFromBlazeError(err);
        }

        mGameDataMaster->setGroupJoinInProgress(true);
        err = (BlazeRpcError)joinGame(joinMasterRequest, response, error, reason);

        JoinState leaderState = response.getJoinResponse().getJoinState();

        // This requires both error blocks in the RPC to be the same.
        if (err != ERR_OK)
        {
            mGameDataMaster->setGroupJoinInProgress(false);
            return JoinGameByGroupMasterError::commandErrorFromBlazeError(err);
        }

        BlazeRpcError errorValue = (BlazeRpcError)joinGameByGroupMembersOnly(request, response, error, setupReasons);
        mGameDataMaster->setGroupJoinInProgress(false);

        JoinState groupState = response.getJoinResponse().getJoinState();

        // At this point, we know that the leader is either JOINED or IN_QUEUE (err otherwise),
        // and since the groupState is only set if we had group members who attempted to join, the state can only change to PARTIALLY_JOINED.
        if (groupState != leaderState || errorValue != ERR_OK)
            response.getJoinResponse().setJoinState(GROUP_PARTIALLY_JOINED);

        // If no player in the group got queued, there is nobody to follow and we get rid of the group entry in the map.
        if (getGameType() == GAME_TYPE_GAMESESSION)
        {
			const UserGroupId playerGroupId = joinMasterRequest.getUserJoinInfo().getUserGroupId();
            GameDataMaster::UserGroupQueuedPlayersCountList::iterator iter = eastl::find_if(mGameDataMaster->getUserGroupQueuedPlayersCountList().begin(), mGameDataMaster->getUserGroupQueuedPlayersCountList().end(), UserGroupQueuedPlayersCountEquals(playerGroupId));
            if (iter == mGameDataMaster->getUserGroupQueuedPlayersCountList().end())
            {
                eraseUserGroupIdFromLists(playerGroupId, false);
            }
        }
        // we return the leader's error state.
        return JoinGameByGroupMasterError::commandErrorFromBlazeError(err);
    }

    /*! ************************************************************************************************/
    /*! \brief Validate that a group can join this game.

        \param[in] request - the join request sent by a member of the group.
        \param[in] leaderBlazeId - the BlazeId of the member who sent the join request.
        \return
    ***************************************************************************************************/
    JoinGameByGroupMasterError::Error GameSessionMaster::validateGroupJoinSetTeams(JoinGameByGroupMasterRequest& request, BlazeId leaderBlazeId) const
    {
        if (!isGameProtocolVersionStringCompatible(gGameManagerMaster->getConfig().getGameSession().getEvaluateGameProtocolVersionString(), request.getJoinRequest().getCommonGameData().getGameProtocolVersionString(), getGameProtocolVersionString()))
        {
            TRACE_LOG("[GameSessionMaster].validateGroupJoinSetTeams() : Game protocol version mismatch; game protocol version \""
                << request.getJoinRequest().getCommonGameData().getGameProtocolVersionString() << "\" attempted to join game " << getGameId()
                << " with game protocol version \"" << getGameProtocolVersionString() << "\"");
            return JoinGameByGroupMasterError::GAMEMANAGER_ERR_GAME_PROTOCOL_VERSION_MISMATCH;
        }

        const UserSessionInfo* leaderInfo = getHostSessionInfo(request.getUsersInfo());
        if (leaderInfo == nullptr)
        {
            ERR_LOG("[GameSessionMaster].validateGroupJoinSetTeams() : Missing host player from join request.");
            return JoinGameByGroupMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }

        // Check the entry type, and adjust if we're going to claim a reservation:
        GameEntryType entryType = request.getJoinRequest().getPlayerJoinData().getGameEntryType();
        PlayerInfoMaster* joiningPlayer = mPlayerRoster.getPlayer(leaderBlazeId, leaderInfo->getUserInfo().getPlatformInfo());
        if (joiningPlayer && joiningPlayer->isReserved() && entryType == GAME_ENTRY_TYPE_DIRECT)
            entryType = GAME_ENTRY_TYPE_CLAIM_RESERVATION;

        if (!validateJoinReputation(*leaderInfo, entryType, request.getJoinRequest().getJoinMethod()))
        {
            return JoinGameByGroupMasterError::GAMEMANAGER_ERR_FAILED_REPUTATION_CHECK;
        }

        // Check the slot types of the joining players, and make sure the team is unspecified for Spectators:
        bool isSomeoneJoiningAsPlayer = false;
        if (request.getJoinRequest().getPlayerJoinData().getPlayerDataList().empty())
        {
            SlotType slotType = request.getJoinRequest().getPlayerJoinData().getDefaultSlotType();
            if (isParticipantSlot(slotType))
            {
                isSomeoneJoiningAsPlayer = true;
            }
        }
        else
        {
            PerPlayerJoinDataList::const_iterator playerIter = request.getJoinRequest().getPlayerJoinData().getPlayerDataList().begin();
            PerPlayerJoinDataList::const_iterator playerEnd = request.getJoinRequest().getPlayerJoinData().getPlayerDataList().end();
            for (; playerIter != playerEnd; ++playerIter)
            {
                SlotType slotType = getSlotType(request.getJoinRequest().getPlayerJoinData(), (*playerIter)->getSlotType());
                if (isParticipantSlot(slotType))
                {
                    isSomeoneJoiningAsPlayer = true;
                }
                else
                {
                    const_cast<PerPlayerJoinData&>(**playerIter).setTeamIndex(UNSPECIFIED_TEAM_INDEX);
                }
            }
        }
        if (!isSomeoneJoiningAsPlayer)
        {
            const_cast<JoinGameByGroupMasterRequest&>(request).getJoinRequest().getPlayerJoinData().setDefaultTeamIndex(UNSPECIFIED_TEAM_INDEX);
        }


        // Check each joining player and see what slot type is used:

        SlotTypeSizeMap nonOptionalSlotTypeSizeMap;
        for (UserJoinInfoList::const_iterator itr = request.getUsersInfo().begin(),
            end = request.getUsersInfo().end(); itr != end; ++itr)
        {
            if (!(*itr)->getIsOptional())
            {
                SlotType joiningSlotType = GameManager::getSlotType(request.getJoinRequest().getPlayerJoinData(), (*itr)->getUser().getUserInfo());
                nonOptionalSlotTypeSizeMap[joiningSlotType] += 1; 
            }
        }

        uint16_t openQueueSlots = 0;

        if (isQueueEnabled())
        {
            if (isQueueFull() && (entryType != GAME_ENTRY_TYPE_CLAIM_RESERVATION))
            {
                // We only return queue full if no one will make it.  Only the leader needs to get in,
                // everyone else can get individual queue full notices on their joins
                return JoinGameByGroupMasterError::GAMEMANAGER_ERR_QUEUE_FULL;
            }

            // we'll need this below if the queue is empty, but the game doesn't have room for all members of the group.
            openQueueSlots = getQueueCapacity() - mPlayerRoster.getQueueCount();

            SlotTypeSizeMap::const_iterator curSlotTypeSize = nonOptionalSlotTypeSizeMap.begin();
            SlotTypeSizeMap::const_iterator endSlotTypeSize = nonOptionalSlotTypeSizeMap.end();
            for (; curSlotTypeSize != endSlotTypeSize; ++curSlotTypeSize)
            {
                SlotType slotType = curSlotTypeSize->first;
                uint16_t nonOptionalUserCount = curSlotTypeSize->second;

                // The game is not full, but there are people waiting to be dequeued in the queue.
                // Since spectators and players may be queued, we ensure the queue is free of users seeking similar slot types, if there are,
                // we don't allow new players trying to join to take those slots, or be put on specific teams now.
                // The team information will be different once they have a chance to join, so we will
                // mitigate it then.  We return OK here and the subsequent joinGame attempts will queue the players.
                // This doesn't account for any players in this list that are already members of this game session.
            
                // Technically, we just want to check if the is a spot for the host's slot type. We don't care about the rest. 
                if (!isQueueBypassPossible(slotType) && (openQueueSlots >= nonOptionalUserCount))
                {
                    return JoinGameByGroupMasterError::ERR_OK;
                }
            }
        }

        // Validate there is enough room for the entire group on a team.
        // Count the number of existing players to determine how many slots we
        // will be taking up.
        SlotTypeSizeMap numSlotsRequiredMap;
        uint32_t numReservedNeedTeamRequired = 0;

        bool targetPlayerInQueue = false;
        TeamIndex targetTeamIndex = UNSPECIFIED_TEAM_INDEX;
        TeamIndexRoleSizeMap teamRoleSlotsRequired;
        if (isSomeoneJoiningAsPlayer)
        {
            // Non-specator users: 
            uint16_t nonOptionalUserCount = nonOptionalSlotTypeSizeMap[SLOT_PUBLIC_PARTICIPANT] + nonOptionalSlotTypeSizeMap[SLOT_PRIVATE_PARTICIPANT];
            gGameManagerMaster->buildTeamIndexRoleSizeMapFromJoinRequest(request.getJoinRequest(), teamRoleSlotsRequired, nonOptionalUserCount);

            // Update the TARGET_USER_TEAM_INDEX role slots: 
            TeamIndexRoleSizeMap::iterator targetTeam = teamRoleSlotsRequired.find(TARGET_USER_TEAM_INDEX);
            if (targetTeam != teamRoleSlotsRequired.end())
            {
                PlayerInfo* playerInfo = getPlayerRoster()->getPlayer(request.getJoinRequest().getUser().getBlazeId());
                if (playerInfo != nullptr)
                {
                    targetTeamIndex = playerInfo->getTeamIndex();
                    targetPlayerInQueue = playerInfo->isInQueue();
                }

                RoleSizeMap::iterator roleIter = teamRoleSlotsRequired[TARGET_USER_TEAM_INDEX].begin();
                RoleSizeMap::iterator roleEnd = teamRoleSlotsRequired[TARGET_USER_TEAM_INDEX].end();
                for (; roleIter != roleEnd; ++roleIter)
                {
                    teamRoleSlotsRequired[targetTeamIndex][roleIter->first] += roleIter->second;
                }
                teamRoleSlotsRequired.erase(TARGET_USER_TEAM_INDEX);   // If the target is in-queue targeting someone else, this will just clear the roleSlots requirements.
            }
        }

        eastl::vector<PlayerInfoPtr> resvClaimPlayers;
        for (UserJoinInfoList::const_iterator itr = request.getUsersInfo().begin(),
            end = request.getUsersInfo().end(); itr != end; ++itr)
        {
            if ((*itr)->getIsOptional())
                continue;

            const UserSessionInfo& userInfo = (*itr)->getUser();

            if (gUserSessionManager->getSessionExists(userInfo.getSessionId()))
            {
                PlayerInfoMaster* existingPlayer = mPlayerRoster.getPlayer(userInfo.getUserInfo().getId());
               
                SlotType joiningSlotType = GameManager::getSlotType(request.getJoinRequest().getPlayerJoinData(), userInfo.getUserInfo());
                bool isJoiningAsPlayer = isParticipantSlot(joiningSlotType); 

                // If there is no existing player but the user is still logged in
                // then we will need a slot for it in this game.
                // Reserved players do not need to be accounted for unless they need a team to be
                // decided for them. (Came in through create game MM not in the creator's game group).
                if (existingPlayer == nullptr)
                {
                    numSlotsRequiredMap[joiningSlotType] += 1;
                }
                else if ((existingPlayer->isReserved()) &&
                         (existingPlayer->getTeamIndex() == UNSPECIFIED_TEAM_INDEX) && isJoiningAsPlayer)
                {
                    // Player was reserved through create game MM but has not been assigned a team yet.
                    // They will be assigned to the team specified in the join request, but that doesn't
                    // happen until we get into joinGameAsReservedPlayer
                    ++numReservedNeedTeamRequired;
                }
                else if (isJoiningAsPlayer)
                {
                    // For players that already have a reservation in the game, we add them to this vector. 
                    // The players in the vector will be temporarily removed when doing the capacity checks (since they may change roles/teams)
                    resvClaimPlayers.push_back(mPlayerRoster.getPlayerInfoPtr(userInfo.getUserInfo().getId()));

                    // Ensure that the join request specifies the same team for the player as a WARN will be generated when claiming the reservation
                    // if the reserved team and claim reservation team don't match
                    for (auto& playerData : request.getJoinRequest().getPlayerJoinData().getPlayerDataList())
                    {
                        if (playerData->getUser().getBlazeId() == userInfo.getUserInfo().getId())
                        {
                            if (playerData->getTeamIndex() != existingPlayer->getTeamIndex())
                            {
                                if (playerData->getTeamIndex() != UNSPECIFIED_TEAM_INDEX)
                                {
                                    WARN_LOG("[GameSessionMaster].validateGroupJoinSetTeams: Player(" << playerData->getUser().getBlazeId() << ") claiming reservation in game("
                                        << getGameId() << ") requested team(" << playerData->getTeamIndex() << ") which does not match reservation team("
                                        << existingPlayer->getTeamIndex() << ").  Player will be added to the reserved team.");
                                }
                                playerData->setTeamIndex(existingPlayer->getTeamIndex());
                            }
                            break;
                        }
                    }
                }

                // GM_AUDIT: if the conn group is all or nothing, shouldn't we check entry criteria at this stage as well?
                // MLU checks here at group validation stage, to ensure if one cannot get in then none do.
                // Spectators do have to do the ban list check
                if ((request.getJoinRequest().getPlayerJoinData().getGroupId().type == ENTITY_TYPE_CONN_GROUP) || (request.getJoinRequest().getPlayerJoinData().getGroupId().type != ENTITY_TYPE_LOCAL_USER_GROUP))
                {
                    // Restrict entry checks to only be bypassed when invited by an admin player
                    // Check the joining player against the ban list.
                    if (!ignoreEntryCheckAdminInvited(request.getJoinRequest()) &&
                        !ignoreBanList(request.getJoinRequest().getJoinMethod()) &&
                        isAccountBanned(userInfo.getUserInfo().getPlatformInfo().getEaIds().getNucleusAccountId()))
                    {
                        return JoinGameByGroupMasterError::GAMEMANAGER_ERR_PLAYER_BANNED;
                    }
                }
            }
        }

        // Check that all the joining slots will fit: 
        SlotTypeSizeMap::const_iterator curNumSlotsRequired = numSlotsRequiredMap.begin();
        SlotTypeSizeMap::const_iterator endNumSlotsRequired = numSlotsRequiredMap.end();
        for (; curNumSlotsRequired != endNumSlotsRequired; ++curNumSlotsRequired)
        {
            SlotType slotType = curNumSlotsRequired->first;
            uint16_t numSlotsRequired = curNumSlotsRequired->second;

            // public slots are always available
            uint16_t availableSlots = 0;
            if (isParticipantSlot(slotType))
            {
                availableSlots = getSlotTypeCapacity(SLOT_PUBLIC_PARTICIPANT) - getPlayerRoster()->getPlayerCount(SLOT_PUBLIC_PARTICIPANT);
                // Add in private slots if we are requesting a private slot
                if (slotType == SLOT_PRIVATE_PARTICIPANT)
                {
                    availableSlots += getSlotTypeCapacity(SLOT_PRIVATE_PARTICIPANT) - getPlayerRoster()->getPlayerCount(SLOT_PRIVATE_PARTICIPANT);
                }
            }
            else
            {
                // this is a spectator join
                availableSlots = getSlotTypeCapacity(SLOT_PUBLIC_SPECTATOR) - getPlayerRoster()->getPlayerCount(SLOT_PUBLIC_SPECTATOR);
                // Add in private slots if we are requesting a private slot
                if (slotType == SLOT_PRIVATE_SPECTATOR)
                {
                    availableSlots += getSlotTypeCapacity(SLOT_PRIVATE_SPECTATOR) - getPlayerRoster()->getPlayerCount(SLOT_PRIVATE_SPECTATOR);
                }
            }

            // Check if there is enough room for our PG.
            if (numSlotsRequired > availableSlots)
            {
                if (!isQueueEnabled())
                {
                    TRACE_LOG("[GameSessionMaster].validateGroupJoin player " << leaderBlazeId << " not enough slots for entire group in game "
                        << getGameId() << " slots required " << numSlotsRequired << ", game slots available " << availableSlots << ".");
                    return (JoinGameByGroupMasterError::Error) (getSlotFullErrorCode(slotType));
                }
                else if ( numSlotsRequired > (uint16_t)(availableSlots + openQueueSlots))
                {
                    TRACE_LOG("[GameSessionMaster].validateGroupJoin player " << leaderBlazeId << " not enough slots for entire group in game "
                        << getGameId() << " slots required " << numSlotsRequired << ", game slots available " << availableSlots << ", queue slots available " << openQueueSlots << ".");
                    return JoinGameByGroupMasterError::GAMEMANAGER_ERR_QUEUE_FULL;
                }
            }
        }

        TeamIndex unspecifiedTeamIndex = UNSPECIFIED_TEAM_INDEX;
        if (isSomeoneJoiningAsPlayer)
        {
            // Temporarily remove the reserved players that are now claiming a slot (before doing the role/team checks):
            // (We don't need to do this for the host check because that's just a super edge case that is already poorly handled; all group members end up in queue.)
            for (eastl::vector<PlayerInfoPtr>::iterator curPlayer = resvClaimPlayers.begin(); curPlayer != resvClaimPlayers.end(); ++curPlayer)
                const_cast<GameSessionMaster*>(this)->mPlayerRoster.erasePlayer((*curPlayer)->getPlayerId());

            // Note that this also accounts for the number of reserved players still needing a team.
            uint16_t numSlotsRequired = numSlotsRequiredMap[SLOT_PUBLIC_PARTICIPANT] + numSlotsRequiredMap[SLOT_PRIVATE_PARTICIPANT];
            JoinGameMasterError::Error validateTeamInfoError = validateJoinTeamGameTeamCapacities(teamRoleSlotsRequired, leaderBlazeId, (uint16_t)(numSlotsRequired + numReservedNeedTeamRequired), unspecifiedTeamIndex, targetPlayerInQueue);

            // Re-add the players we temporarily removed:  
            for (eastl::vector<PlayerInfoPtr>::iterator curPlayer = resvClaimPlayers.begin(); curPlayer != resvClaimPlayers.end(); ++curPlayer)
                const_cast<GameSessionMaster*>(this)->mPlayerRoster.insertPlayer(**curPlayer);

            if (validateTeamInfoError != JoinGameMasterError::ERR_OK)
            {
                if (isQueueableJoinError(validateTeamInfoError))
                {
                    if (!isQueueEnabled())
                    {
                        // Queuing is not enabled, return immediately.
                        return JoinGameByGroupMasterError::commandErrorFromBlazeError((BlazeRpcError)validateTeamInfoError);
                    }
                    else
                    {
                        TeamIndexRoleSizeMap leaderRoleRequirement;
                        const PerPlayerJoinData* playerData = lookupPerPlayerJoinDataConst(request.getJoinRequest().getPlayerJoinData(), leaderInfo->getUserInfo());
                        if (playerData != nullptr)
                        {
                            TeamIndex joiningLeaderTeamIndex = getTeamIndex(request.getJoinRequest().getPlayerJoinData(), playerData->getTeamIndex());
                            joiningLeaderTeamIndex = (joiningLeaderTeamIndex == UNSPECIFIED_TEAM_INDEX || joiningLeaderTeamIndex == TARGET_USER_TEAM_INDEX) ? targetTeamIndex : joiningLeaderTeamIndex;

                            const char8_t *leaderRoleName = lookupPlayerRoleName(request.getJoinRequest().getPlayerJoinData(), leaderBlazeId);
                            if (EA_LIKELY(leaderRoleName != nullptr))
                            {
                                leaderRoleRequirement[joiningLeaderTeamIndex][leaderRoleName] = 1;
                            }
                            else
                            {
                                ERR_LOG("[GameSessionMaster].validateGroupJoin leading player " << leaderBlazeId << " had no role "
                                    << " for game " << getGameId() << ".");
                                // no leader role
                                return JoinGameByGroupMasterError::GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
                            }
                            // Some portion of the group will have to be queued.  Our design calls for however
                            // many players possible (starting with group leader) to join the game, everyone else
                            // will enter the queue, or get queue full individually.  Anyone in the queue needs
                            // to know what team the leader joined, so we determine that here.
                            validateJoinTeamGameTeamCapacities(leaderRoleRequirement, leaderBlazeId, 1, unspecifiedTeamIndex, targetPlayerInQueue);
                            // we drop the validation error on just the leader, we wont learn anything we don't already
                            // know from the overall group error.
                        }
                        else
                        {
                            int playersWithUnspecifiedTeam = 0;
                            if (request.getJoinRequest().getPlayerJoinData().getDefaultTeamIndex() == UNSPECIFIED_TEAM_INDEX)
                            {
                                for (auto& playerIter : request.getJoinRequest().getPlayerJoinData().getPlayerDataList())
                                {
                                    if (playerIter->getTeamIndex() == UNSPECIFIED_TEAM_INDEX && isParticipantSlot(playerIter->getSlotType()))
                                    {
                                        ++playersWithUnspecifiedTeam;
                                    }
                                }
                            }
                            if (playersWithUnspecifiedTeam != 0)
                            {
                                WARN_LOG("[GameSessionMaster].validateGroupJoin leading player is missing, so " << playersWithUnspecifiedTeam << " players with an UNSPECIFIED_TEAM are staying that way, for game " << getGameId() << ".");
                            }
                        }
                    }
                }
                else
                {
                    // A non team full validation error.  Something has gone wrong.
                    return JoinGameByGroupMasterError::commandErrorFromBlazeError((BlazeRpcError)validateTeamInfoError);
                }
            }
        }

        // Update the Unspecified TeamIndex of all the player join data: 
        // (Other places that call validateJoinTeamGameTeamCapacities typically only update the team held by the default, since they're just since elements)
        setUnspecifiedTeamIndex(request.getJoinRequest().getPlayerJoinData(), unspecifiedTeamIndex);

        return JoinGameByGroupMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief create player for each member in user group excluding the leader, and join them to the game.

        \return JoinGameByGroupMasterError
    ***************************************************************************************************/
    JoinGameByGroupMasterError::Error GameSessionMaster::joinGameByGroupMembersOnly(const JoinGameByGroupMasterRequest& request, JoinGameMasterResponse& joinGameMasterResponse,
                                                                                   Blaze::EntryCriteriaError& error,
                                                                                   const UserSessionSetupReasonMap &setupReasons)
    {
        JoinGameResponse& memberJoinResponse = joinGameMasterResponse.getJoinResponse();
        uint16_t nonOptionalPlayersAttempted = 0;
        uint16_t optionalPlayersAttempted = 0;
        // Join all game group members to the game.
        JoinGameByGroupMasterError::Error result;
        result = joinGameByGroupMembersOnlyInternal(request.getUsersInfo(), false, request, joinGameMasterResponse, error, setupReasons, nonOptionalPlayersAttempted);
        if (result != JoinGameByGroupMasterError::ERR_OK)
        {
            return result;
        }

        JoinState usersState = memberJoinResponse.getJoinState();

        // Join optional reserved players to the game.
        result = joinGameByGroupMembersOnlyInternal(request.getUsersInfo(), true, request, joinGameMasterResponse, error, setupReasons, optionalPlayersAttempted);
        JoinState optUsersState = memberJoinResponse.getJoinState();

        // If there were both optional players and non-optional players, and there was a join state mismatch set as partially joined:
        // otherwise the join state will reflect the last set value from joinGameByGroupMembersOnlyInternal()
        if ((nonOptionalPlayersAttempted != 0) && (optionalPlayersAttempted != 0))
        {
            if (usersState != optUsersState)
                memberJoinResponse.setJoinState(GROUP_PARTIALLY_JOINED);
        }
        
        

        return result;
    }

    JoinGameByGroupMasterError::Error GameSessionMaster::joinGameByGroupMembersOnlyInternal(const UserJoinInfoList& sessionIdList, bool isOptionalReservedList,
                                                                                   const JoinGameByGroupMasterRequest& masterRequest, JoinGameMasterResponse &joinGameMasterResponse,
                                                                                   Blaze::EntryCriteriaError& error,
                                                                                   const UserSessionSetupReasonMap &setupReasons, uint16_t &outNumPlayersAttemptingJoin)
    {
        JoinGameResponse& memberJoinResponse = joinGameMasterResponse.getJoinResponse();
        BlazeId leaderId = INVALID_BLAZE_ID;
        if (masterRequest.getJoinLeader())
        {
            const UserSessionInfo* hostSessionInfo = getHostSessionInfo(masterRequest.getUsersInfo());
            leaderId = hostSessionInfo ? hostSessionInfo->getUserInfo().getId() : INVALID_BLAZE_ID;
        }
        if (leaderId == INVALID_BLAZE_ID)
            leaderId = UserSession::getCurrentUserBlazeId();

        // Count the number of players that will actually try to join now:
        outNumPlayersAttemptingJoin = 0;
        for (UserJoinInfoList::const_iterator iter = sessionIdList.begin(), end = sessionIdList.end(); iter != end; ++iter)
        {
            if (isOptionalReservedList != (*iter)->getIsOptional() || (*iter)->getIsHost())
                continue;

            ++outNumPlayersAttemptingJoin;
        }

        // Early out if noone is joining (don't change the join state)
        if (outNumPlayersAttemptingJoin == 0)
        {
            return JoinGameByGroupMasterError::ERR_OK;
        }

        bool setFirstJoinState = false;
        JoinState firstJoinState = JOINED_GAME;

        // modifiable request for our members.
        JoinGameRequest memberJoinRequest;
        masterRequest.getJoinRequest().copyInto(memberJoinRequest);
        if (isOptionalReservedList)
        {
            memberJoinRequest.setJoinMethod(SYS_JOIN_BY_FOLLOWLEADER_RESERVEDEXTERNALPLAYER);           // This isn't really an accurate desc. of what's going on.
        }

        // build this out once to send to any logged-in users who won't be making it into the game
        NotifyRemoteJoinFailed remoteJoinFailedNotification;
        remoteJoinFailedNotification.setGameId(getGameId());
        remoteJoinFailedNotification.setJoinError(ERR_SYSTEM); // this will be overridden on a per-user basis
        remoteJoinFailedNotification.getRemoteUserInfo().setAssociatedUserGroupId(masterRequest.getJoinRequest().getPlayerJoinData().getGroupId());
        remoteJoinFailedNotification.getRemoteUserInfo().setInitiatorId(leaderId);


        GameSetupReason reservedMemberReason;
        reservedMemberReason.switchActiveMember(GameSetupReason::MEMBER_INDIRECTJOINGAMESETUPCONTEXT);
        reservedMemberReason.getIndirectJoinGameSetupContext()->setRequiresClientVersionCheck(gGameManagerMaster->getConfig().getGameSession().getEvaluateGameProtocolVersionString());

        for (UserJoinInfoList::const_iterator iter = sessionIdList.begin(), end = sessionIdList.end(); iter != end; ++iter)
        {
            // We run this function twice, once for the required users (who all have to join) and once for the optional users (who may fail to join)
            // Here we skip the users that aren't part of the current optional/non-optional list iteration.
            if (isOptionalReservedList != (*iter)->getIsOptional() || (*iter)->getIsHost())
            {
                continue;
            }

            const UserSessionInfo& user = (*iter)->getUser();

            // set the GameSetupReason
            GameSetupReason *memberReason = &reservedMemberReason;
            if (!isOptionalReservedList)
            {
                UserSessionSetupReasonMap::const_iterator memberReasonIter = setupReasons.find(user.getSessionId());
                if (memberReasonIter == setupReasons.end())
                {
                    WARN_LOG("[GameSessionMaster] Error joining group members -- Attempt to join game(" << masterRequest.getJoinRequest().getGameId()
                         << ") by group with no setup reason for session(" << user.getUserInfo().getId() << ")");
                    continue;
                }
                memberReason = memberReasonIter->second.get();
            }

            // modify join request per member
            updateMemberJoinRequestFromMemberInfo(memberJoinRequest, **iter, *memberReason, masterRequest.getJoinRequest(), isOptionalReservedList);

            // validate each reserved list player's reputation using caller's (not own) joinMethod. we do this
            // to ensure we only allow them to bypass dynamic reputation if caller specified join by invites.
            if (isOptionalReservedList && !validateJoinReputation(user, memberJoinRequest.getPlayerJoinData().getGameEntryType(), masterRequest.getJoinRequest().getJoinMethod()))
            {
                // this user won't be reserved into the game, send down a notification that the join failed
                remoteJoinFailedNotification.setUserSessionId(user.getSessionId());
                remoteJoinFailedNotification.setJoinError(GAMEMANAGER_ERR_FAILED_REPUTATION_CHECK);
                gGameManagerMaster->sendNotifyRemoteJoinFailedToUserSessionById(user.getSessionId(), &remoteJoinFailedNotification);

                continue;
            }

            PerPlayerJoinData* roleRosterForOptionalPlayer = nullptr;
            if (isOptionalReservedList && gUserSessionManager->getSessionExists(user.getSessionId()))
            {
                // If here, client specified this user via the reserved external players list, but Blaze found a user session for it,
                // so now its being added as a non-external player. Track its role in the non-external role map for its join game.
                roleRosterForOptionalPlayer = externalRoleRosterEntryToNonExternalRoster(user.getUserInfo(), memberJoinRequest.getPlayerJoinData());
            }

            // Note: Members who fail to join is not considered a failure.
            JoinGameMasterRequest memberJoinMasterRequest;
            memberJoinMasterRequest.setJoinRequest(memberJoinRequest);
            memberJoinMasterRequest.setUserJoinInfo(const_cast<UserJoinInfo&>(**iter));
            masterRequest.getExternalSessionData().copyInto(memberJoinMasterRequest.getExternalSessionData());

            // Note: Members who fail to join is not considered a failure.
            JoinGameMasterError::Error memberErr = joinGame(memberJoinMasterRequest, joinGameMasterResponse, error, *memberReason);

            // Update the join state:
            if (memberErr != JoinGameMasterError::ERR_OK)
                memberJoinResponse.setJoinState(NO_ONE_JOINED);

            JoinState curJoinState = memberJoinResponse.getJoinState();
            if (!setFirstJoinState)
            {
                firstJoinState = curJoinState;
                setFirstJoinState = true;
            }
            if (firstJoinState != curJoinState)
                memberJoinResponse.setJoinState(GROUP_PARTIALLY_JOINED);

            // Verify the member joined:
            if (memberErr != JoinGameMasterError::ERR_OK)
            {
                TRACE_LOG("[GameSessionMaster] Failed joining group members -- User id(" << user.getUserInfo().getId() << ") failed to join game with error("
                    << (ErrorHelp::getErrorName((BlazeRpcError)memberErr)) << ")");

                // this user won't be reserved into the game, send down a notification that the join failed
                remoteJoinFailedNotification.setUserSessionId(user.getSessionId());
                remoteJoinFailedNotification.setJoinError(memberErr);
                gGameManagerMaster->sendNotifyRemoteJoinFailedToUserSessionById(user.getSessionId(), &remoteJoinFailedNotification);

                if (isOptionalReservedList)
                {
                    // Un-track the optional user from the role map, since its not joining,
                    // This avoids incorrect role size validation fails, for the next user checking the role map.
                    if (roleRosterForOptionalPlayer != nullptr)
                        roleRosterForOptionalPlayer->setIsOptionalPlayer(true);

                    // For optional players, when game gets full stop trying to join
                    if ((memberErr == JoinGameMasterError::GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL) || (memberErr == JoinGameMasterError::GAMEMANAGER_ERR_SPECTATOR_SLOTS_FULL))
                    {
                        TRACE_LOG("[GameSessionMaster] Game full: Remaining members of optional reserved external players will not be added");
                        // we need to try to dispatch to all the optional players, so before bailing, finish up our iteration to notify them all
                        // advance first, we already notified the current player
                        ++iter;
                        for (; iter != end; ++iter)
                        {
                            const UserSessionInfo& optionalReservedUser = (*iter)->getUser();

                            // Skip non-optional users && the leader if they specified them here.
                            if (isOptionalReservedList != (*iter)->getIsOptional() || (*iter)->getIsHost())
                            {
                                continue;
                            }
                            // this user won't be reserved into the game, send down a notification that the join failed
                            remoteJoinFailedNotification.setUserSessionId(optionalReservedUser.getSessionId());
                            remoteJoinFailedNotification.setJoinError(memberErr);
                            gGameManagerMaster->sendNotifyRemoteJoinFailedToUserSessionById(optionalReservedUser.getSessionId(), &remoteJoinFailedNotification);
                        }
                        break;
                    }
                }
            }
            else
            {
                if (isOptionalReservedList)
                {
                    // add the successfully reserved optional player to the results
                    UserInfo::filloutUserIdentification(user.getUserInfo(), *memberJoinResponse.getJoinedReservedPlayerIdentifications().pull_back());
                }
            }
        }

        // If everyone joins, we just use the last state, which may be either JOINED_GAME or IN_QUEUE.
        if (!setFirstJoinState)
            memberJoinResponse.setJoinState(NO_ONE_JOINED);

        return JoinGameByGroupMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief some values in the group join request are defaults, that should be overridden per member, for instance, multi-MM
        session group joins may have different game entry types for different MM sessions. Update the per member request.
        \param[out] memberJoinRequest per member join request
        \param[in] groupRequest original group request
    ***************************************************************************************************/
    void GameSessionMaster::updateMemberJoinRequestFromMemberInfo(JoinGameRequest& memberJoinRequest, const UserJoinInfo& memberJoinInfo,
        const GameSetupReason& memberReason, const JoinGameRequest& groupRequest, bool isOptionalList) const
    {
        const TeamIndex teamIndex = getTeamIndex(groupRequest.getPlayerJoinData(), memberJoinInfo.getUser().getUserInfo());
        memberJoinRequest.getPlayerJoinData().setDefaultTeamIndex(teamIndex);

        memberJoinRequest.getPlayerJoinData().setGameEntryType(isOptionalList? GAME_ENTRY_TYPE_MAKE_RESERVATION :
            getUserGameEntryTypeFromData(memberJoinInfo.getUser().getSessionId(), groupRequest.getPlayerJoinData(), memberReason));
    }

    GameEntryType GameSessionMaster::getUserGameEntryTypeFromData(UserSessionId userSessionId, const PlayerJoinData& joinData, const GameSetupReason& memberReason) const
    {
        GameEntryType gameEntryType = joinData.getGameEntryType();//caller's value or default
        // side: game entry type is in MM setup reasons for client use and back-compatibility.
        if (memberReason.getMatchmakingSetupContext() != nullptr)
            gameEntryType = memberReason.getMatchmakingSetupContext()->getGameEntryType();
        if (memberReason.getIndirectMatchmakingSetupContext() != nullptr)
            gameEntryType = memberReason.getIndirectMatchmakingSetupContext()->getGameEntryType();
        return gameEntryType;
    }

    void GameSessionMaster::addGroupIdToAllowedSingleGroupJoinGroupIds(const UserGroupId& groupId)
    {
        // Only valid for single group games
        if (!getGameSettings().getEnforceSingleGroupJoin())
        {
            WARN_LOG("[GameSessionMaster] Attempting add groupId(" << groupId.toString().c_str()
                     << ") via SingleGroupJoin when SingleGroupJoin is not enforced on the game(" << getGameId() << ") ");
            return;
        }
        // Only add if we are allowed to add
        if (!isGroupAllowedToJoinSingleGroupGame(groupId))
        {
            WARN_LOG("[GameSessionMaster] Attempting add groupId(" << groupId.toString().c_str() << ") via SingleGroupJoin to game("
                     << getGameId() << "), which already has enough groups(" << mGameData->getSingleGroupMatchIds().size() << ").");
            return;
        }
        bool found = false;
        for (SingleGroupMatchIdList::iterator i = mGameData->getSingleGroupMatchIds().begin(), e = mGameData->getSingleGroupMatchIds().end(); i != e; ++i)
            if (*i == groupId)
                found = true;
        if (!found)
        {
            mGameData->getSingleGroupMatchIds().push_back(groupId);
            TRACE_LOG("[GameSessionMaster] Adding Group (" << groupId.toString().c_str() << ") to allowed groups for game(" 
                << getGameData().getGameId() << "), Current Number of Allowed Groups (" << mGameData->getSingleGroupMatchIds().size() << ").");
        }
    }

    bool GameSessionMaster::checkDisconnectReservationCondition(const PlayerRemovalContext& removeContext) const
    {
        PlayerRemovedReason reason = removeContext.mPlayerRemovedReason;
        return ((reason == PLAYER_CONN_LOST) || (reason == BLAZESERVER_CONN_LOST) ||
            (reason == PLAYER_LEFT_MAKE_RESERVATION) || (reason == GROUP_LEFT_MAKE_RESERVATION) ||
            (reason == HOST_EJECTED) ||
            (reason == PLAYER_JOIN_TIMEOUT && removeContext.mPlayerRemovedTitleContext == GAME_ENTRY_TYPE_CLAIM_RESERVATION) ||
            (reason == MIGRATION_FAILED && mGameDataMaster->getHostMigrationType() == PLATFORM_HOST_MIGRATION) ||
            (reason == PLAYER_KICKED_CONN_UNRESPONSIVE));//Note this is a server-side only version of the fix to handle game server disconnect. A possible enhancement that isnt back compatible likely adds a makeReservation flag to kickPlayer() BlazeSDK interface.

        // Current Unacceptable Reasons:
            // PLAYER_CONN_POOR_QUALITY, ///< The player's connection to another player (or a required peer) was too poor. This will happen as the result of MM connection validation.
            // GAME_DESTROYED, ///< The game was destroyed, disconnecting all players.
            // GAME_ENDED, ///< The game ended, disconnecting all players. (typically sent when a dedicated server game ends).
            // PLAYER_LEFT, ///< The player left the game voluntarily.
            // GROUP_LEFT, ///< The player is leaving the game voluntarily because his userGroup (game group) left.
            // PLAYER_KICKED, ///< The player was kicked out of the game by a game admin.
            // PLAYER_KICKED_WITH_BAN, ///< The player was kicked out of the game by a game admin (and has been banned from this game session).
            // PLAYER_KICKED_CONN_UNRESPONSIVE_WITH_BAN, ///< The player was kicked out of the game by a game admin, due to the loss of connectivity with the peer (and has been banned from this game session).
            // PLAYER_KICKED_POOR_CONNECTION, ///< The player was kicked out of the game by a game admin, due to poor connectivity with the peer.
            // PLAYER_KICKED_POOR_CONNECTION_WITH_BAN, ///< The player was kicked out of the game by a game admin, due to poor connectivity with the peer (and has been banned from this game session).
            // PLAYER_JOIN_FROM_QUEUE_FAILED, ///< The player was unable to join the game from the queue.
            // PLAYER_RESERVATION_TIMEOUT, ///< The player failed to claim their player reservation before the timeout expired.
            // HOST_INJECTION_FAILED, ///< Host injection failed to complete.
            // PLAYER_JOIN_EXTERNAL_SESSION_FAILED, ///< Failed to join player to the external game session.
            // RESERVATION_TRANSFER_TO_NEW_USER, ///< The old placeholder player object's game reservation is now being transferred to the player's 'real' player object, after it logged in with a Blaze user account.
            // DISCONNECT_RESERVATION_TIMEOUT, ///< The player failed to claim their disconnect reservation before the timeout expired.
            // SYS_PLAYER_REMOVE_REASON_INVALID, ///< SYSTEM INTERNAL USE ONLY. A default invalid value for the player removed reason
            // PLAYER_LEFT_CANCELLED_MATCHMAKING, ///< The player cancelled matchmaking while the join game request was in-flight.
            // PLAYER_LEFT_SWITCHED_GAME_SESSION ///< The player left the game due to moving to a new game session.
    }

    /*!************************************************************************************************/
    /*! \brief removes a player from the game. A player entry is removed from following data structure:
        1) Player entry from game's roster replication map
        2) Game entry for the player from data structure held by component( see GameManagerMasterImpl's PlayerToGameMap typedef)

        \param[in] playerInfo - PlayerInfoMaster of the player being removed.
        \param[in] kickerBlazeId  - blaze id of player who trigger the remove player action
        \param[in] reasonCode - reason why the player is removed
        \return RemovePlayerMasterError
    ***************************************************************************************************/
    RemovePlayerMasterError::Error GameSessionMaster::processRemovePlayer(const RemovePlayerMasterRequest* request, const BlazeId kickerBlazeId,
                                                LeaveGroupExternalSessionParameters *externalSessionParams /*= nullptr*/)
    {
        PlayerInfoMaster* playerInfo = mPlayerRoster.getPlayer(request->getPlayerId());

        if (isRemovalReasonABan(request->getPlayerRemovedReason()))
        {
            // if we're banning the player, update their account id
            //  (we could be kick/banning a prejoining player or a reservation who hasn't init'd their account id yet)
            playerInfo->setAccountId(request->getAccountId());
        }

        if ((kickerBlazeId != playerInfo->getPlayerId()) && (request->getPlayerRemovedReason() != GROUP_LEFT)
            && (request->getPlayerRemovedReason() != MIGRATION_FAILED)
            && !gGameManagerMaster->checkPermission(*this, GameAction::KICK_PLAYERS, kickerBlazeId))
        {
            // player kicked (when it's a group leaving, the session id does not need to be game host)
            TRACE_LOG("[GameSessionMaster] the session player(" << kickerBlazeId << ") does not have privilege to kick player("
                        << playerInfo->getPlayerId() << ") from game(" << getGameId() << ") ");
            return RemovePlayerMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }
        // else player leaves

        // remove player from roster
        PlayerRemovalContext playerRemovalContext(request->getPlayerRemovedReason(), request->getPlayerRemovedTitleContext(), request->getTitleContextString());
        RemovePlayerMasterError::Error err= removePlayer(playerInfo, playerRemovalContext, externalSessionParams);

        return err;
    }

    /*! ************************************************************************************************/
    /*! \brief helper to ban a account account from the game.  If the user is a game member, they're kicked out of the game.
    ***************************************************************************************************/
    BanPlayerMasterError::Error GameSessionMaster::banUserFromGame(BlazeId bannedBlazeId, AccountId bannedAccountId, PlayerRemovedTitleContext titleContext, Blaze::GameManager::BanPlayerMasterResponse &resp)
    {
        // Add banned user to the ban list
        banAccount(bannedAccountId);

        // kick banned user out of the game if they're a member
        PlayerInfoMaster *player = mPlayerRoster.getPlayer(bannedBlazeId);
        if (player != nullptr)
        {
            PlayerId banningPlayerId = UserSession::getCurrentUserBlazeId();

            RemovePlayerMasterRequest removeRequest;
            removeRequest.setGameId(player->getGameId());
            removeRequest.setPlayerId(player->getPlayerId());
            removeRequest.setPlayerRemovedReason(PLAYER_KICKED_WITH_BAN);
            removeRequest.setPlayerRemovedTitleContext(titleContext);
            removeRequest.setAccountId(player->getAccountId());

            RemovePlayerMasterError::Error removeError;
            gGameManagerMaster->removePlayer(&removeRequest, banningPlayerId, removeError, &resp.getExternalSessionParams());
            if (removeError != RemovePlayerMasterError::ERR_OK)
            {
                TRACE_LOG("[GameSessionMaster] Unable to remove player(" << bannedBlazeId << ") from game(" << getGameId() << ").");
                return BanPlayerMasterError::GAMEMANAGER_ERR_REMOVE_PLAYER_FAILED;
            }
        }

        return BanPlayerMasterError::ERR_OK;
    }

    RemovePlayerFromBannedListMasterError::Error GameSessionMaster::removePlayerFromBannedList(AccountId removedAccountId)
    {
        GameDataMaster::BannedAccountMap::iterator bannedAccountIter = mGameDataMaster->getBannedAccountMap().find(removedAccountId);
        if (bannedAccountIter != mGameDataMaster->getBannedAccountMap().end())
        {
            bool needToUpdateOldestBannedAccount = (removedAccountId == mGameDataMaster->getOldestBannedAccount());

            mGameDataMaster->getBannedAccountMap().erase(bannedAccountIter);

            if (needToUpdateOldestBannedAccount)
            {
                // need to find the next oldest account
                updateOldestBannedAccount();
            }

            return RemovePlayerFromBannedListMasterError::ERR_OK;
        }

        return RemovePlayerFromBannedListMasterError::GAMEMANAGER_ERR_BANNED_PLAYER_NOT_FOUND;
    }

    void GameSessionMaster::clearBannedList()
    {
        mGameDataMaster->getBannedAccountMap().clear();
        mGameDataMaster->setOldestBannedAccount(INVALID_ACCOUNT_ID);
    }

    void GameSessionMaster::getBannedList(AccountIdList &accountIds)
    {
        accountIds.reserve(mGameDataMaster->getBannedAccountMap().size());

        GameDataMaster::BannedAccountMap::const_iterator bannedAccountIter = mGameDataMaster->getBannedAccountMap().begin();
        GameDataMaster::BannedAccountMap::const_iterator bannedAccountEnd = mGameDataMaster->getBannedAccountMap().end();
        for ( ; bannedAccountIter!=bannedAccountEnd; ++bannedAccountIter)
        {
            accountIds.push_back(bannedAccountIter->first);
        }
    }

    ReplayGameMasterError::Error GameSessionMaster::processReplayGameMaster(const Blaze::GameManager::ReplayGameRequest &request, BlazeId callingPlayerId)
    {
        // only admin player can change the state
        if (!hasPermission(callingPlayerId, GameAction::REPLAY_GAME))
        {
            TRACE_LOG("[GameSessionMaster::processReplayGameMaster] Non admin player(" << callingPlayerId << ") can't set set the game state from POST_GAME to PRE_GAME");
            return ReplayGameMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        //get the next game reporting id and proceed to the pregame state
        BlazeRpcError blazeRpcError = setGameState(PRE_GAME);
        if (blazeRpcError == ERR_OK)
        {
            // send last snapshot before clearing GameFinished and last round's data. (& before new PSN Match)
            updateFinalExternalSessionProperties();
            mGameDataMaster->setGameFinished(false);
            getExternalSessionUtilManager().prepForReplay(*this);

            // set PINIsCrossplayGame flag to false if replaying game (will update when game starts)
            getGameData().setPINIsCrossplayGame(false);

            GameReportingId gameReportId = gGameManagerMaster->getNextGameReportingId();
            if (gameReportId == GameManager::INVALID_GAME_REPORTING_ID)
            {
                return ReplayGameMasterError::ERR_SYSTEM;
            }

            setGameReportingId(gameReportId);

            NotifyGameReportingIdChange nGameReportingIdChange;
            nGameReportingIdChange.setGameId(getGameId());
            nGameReportingIdChange.setGameReportingId(getGameReportingId());
            GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
            gGameManagerMaster->sendNotifyGameReportingIdChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nGameReportingIdChange);
        }

        // When the game is replayed there is the possibility that players left during join in progress.
        // Since the game is no longer in progress queued players now have the potential to join the game.
        // Only process server managed queues here as admin managed get the removal notification upon player
        // removal.
        processPlayerServerManagedQueue();

        return ReplayGameMasterError::commandErrorFromBlazeError(blazeRpcError);
    }
    

    /*! ************************************************************************************************/
    /*! \brief set ps4 np session id for the game

        \param[in] request - UpdateGameSessionRequest contains the np session id
        \param[in] callingPlayerId - the BlazeId of the user who issued the RPC.
    ***************************************************************************************************/
    UpdateGameSessionMasterError::Error GameSessionMaster::updateGameSession( const Blaze::GameManager::UpdateGameSessionRequest &request, BlazeId callingPlayerId, bool sendUpdateNotificationForEmptyNpSession /*= false*/ )
    {
        // This method is now only for backward compatible clients, when server driven external sessions is disabled
        WARN_LOG("[GameSessionMaster].updateGameSession: not updating game(" << request.getGameId() << "). This flow is deprecated/unused when Blaze Server driven external sessions is enabled. No op.");
        return UpdateGameSessionMasterError::ERR_SYSTEM;
    }

    /*! ************************************************************************************************/
    /*! \brief update game's 1st party presence session status; if any members of the game group can't support enabling presence, it is disabled

    \return true if the presence status of the game is changing
    ***************************************************************************************************/
    bool GameSessionMaster::changePresenceState()
    {
        if (!isGameGroup() || ((getPresenceMode() == PRESENCE_MODE_NONE || getPresenceMode() == PRESENCE_MODE_NONE_2) && !getGameData().getOwnsFirstPartyPresence()))
        {
            // only game groups can change presence status, and if presence isn't enabled, this is a non-op
            return false;
        }

        bool previouslyOwnedPresence = getGameData().getOwnsFirstPartyPresence();
        bool gameHasPresence = doesGameSessionControlPresence();
        getGameData().setOwnsFirstPartyPresence(gameHasPresence);
        bool changedPresenceOwnership = (previouslyOwnedPresence != gameHasPresence);

        if (changedPresenceOwnership)
        {
            GameSessionUpdatedNotification updateNotification;
            updateNotification.setGameId(getGameId());
            updateNotification.setOwnsFirstPartyPresence(gameHasPresence);
            GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
            gGameManagerMaster->sendNotifyGamePresenceChangedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &updateNotification );
        }

        return changedPresenceOwnership;
    }

    bool GameSessionMaster::doesGameSessionControlPresence() const
    {
        if (getPresenceMode() == PRESENCE_MODE_NONE || getPresenceMode() == PRESENCE_MODE_NONE_2)
        {
            // can't control presence if there's no presence enabled.
            return false;
        }

        if (!isGameGroup())
        {
            // game sessions always control their own presence
            return true;
        }


        //  returns whether the game controls its own presence state on the clients machines
        //  basically all game members must use this game's presence for this to return true
        //  if any member does not use the game's presence status, this will return false.
        //  conforms with a limitation of Xbox 360 presence management
        const PlayerRoster::PlayerInfoList& playerList = getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        PlayerRoster::PlayerInfoList::const_iterator playerIter = playerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator end = playerList.end();

        for (; playerIter != end; ++playerIter)
        {
            PlayerInfoMaster *playerInfoMaster = static_cast<PlayerInfoMaster*>(*playerIter);
            if (!playerInfoMaster->isUsingGamePresence())
            {
                return false;
            }
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief update game's host migration status

    \param[in] request - UpdateGameHostRequest contains the game id and migration type
    \param[in] callingPlayerId - the BlazeId of the user who issued the RPC.
    ***************************************************************************************************/
    UpdateGameHostMigrationStatusMasterError::Error GameSessionMaster::updateGameHostMigrationStatus( const Blaze::GameManager::UpdateGameHostMigrationStatusRequest &request, BlazeId callingPlayerId )
    {
        if (callingPlayerId != getPlatformHostInfo().getPlayerId())
        {
            //early out if the migration update not coming from the platform host
            return UpdateGameHostMigrationStatusMasterError::ERR_OK;
        }

        if (getGameState() == MIGRATING)
        {
            mGameDataMaster->setNumMigrationsReceived(mGameDataMaster->getNumMigrationsReceived()+1);
            // Early out if we have not received both of our requested migration notifications.
            if (mGameDataMaster->getHostMigrationType() == TOPOLOGY_PLATFORM_HOST_MIGRATION && mGameDataMaster->getNumMigrationsReceived() < TOPOLOGY_PLATFORM_HOST_MIGRATION)
            {
                TRACE_LOG("[GameSessionMaster] Game(" << getGameId() << ") has " << mGameDataMaster->getNumMigrationsReceived()
                          << " migrations received, host migration not yet completed for migrationType("
                          << HostMigrationTypeToString(mGameDataMaster->getHostMigrationType()) << ")");
                return UpdateGameHostMigrationStatusMasterError::ERR_OK;
            }
            if (!getPlayerRoster()->hasActiveMigratingPlayers())
            {
                TRACE_LOG("[GameSessionMaster] Game(" << getGameId() << ") has no further migrating players, host migration of type ("
                          << mGameDataMaster->getHostMigrationType() << ") finished.");
                hostMigrationCompleted();
            }
        }

        return UpdateGameHostMigrationStatusMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief finalizeGameCreationMaster is a wrapper around updateGameSession (with additional validation
            to ensure a game's creation is only finalized once).
    ***************************************************************************************************/
    FinalizeGameCreationMasterError::Error GameSessionMaster::finalizeGameCreation(
            const UpdateGameSessionRequest &request, BlazeId callingPlayerId)
    {
        // note: finalizeGameCreation is sent up by the sdk itself, so the title has no control over it
        //   that's why we send back err_system if there's a problem.

        if (callingPlayerId != getTopologyHostInfo().getPlayerId())
        {
            ERR_LOG("[GameSessionMaster] finalizeGameCreation called by player(" << callingPlayerId <<
                ") who is not the topology host player(" << getTopologyHostInfo().getPlayerId() << ").");
            return FinalizeGameCreationMasterError::ERR_SYSTEM;
        }

        if (!mGameDataMaster->getIsGameCreationFinalized())
        {
            BlazeRpcError blazeRpcError = ERR_OK;
            if (blazeRpcError == ERR_OK)
            {
                mGameDataMaster->setIsGameCreationFinalized(true);
            }
            return FinalizeGameCreationMasterError::commandErrorFromBlazeError( blazeRpcError );
        }

        ERR_LOG("[GameSessionMaster] finalizeGameCreation called multiple times for gameId(" << getGameId() <<
            ") by player(" << callingPlayerId << ").");
        return FinalizeGameCreationMasterError::ERR_SYSTEM;
    }


    /*! ************************************************************************************************/
    /*! \brief set the attributes for specified player; requires admin rights to set someone else's attribs

        \param[in] request - SetPlayerAttributesRequest
        \param[in] callingPlayerId - the BlazeId of the user who issued the RPC.
    ***************************************************************************************************/
    SetPlayerAttributesMasterError::Error GameSessionMaster::processSetPlayerAttributesMaster(const SetPlayerAttributesRequest &request, BlazeId callingPlayerId)
    {
        // validate player existence
        PlayerInfoMaster *player = mPlayerRoster.getPlayer(request.getPlayerId());
        if (player == nullptr)
        {
            WARN_LOG("[GameSessionMaster] Player(" << request.getPlayerId() << ") is not found in game(" << getGameId() << ")");
            return SetPlayerAttributesMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }

        if (callingPlayerId == request.getPlayerId()) 
        {
            if (!hasPermission(callingPlayerId, GameAction::SET_PLAYER_ATTRIBUTES))
            {
                TRACE_LOG("[GameSessionMaster] You do not have permission to set your player attributes");
                return SetPlayerAttributesMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
            }
        }
        else if (!hasPermission(callingPlayerId, GameAction::SET_OTHER_PLAYER_ATTRIBUTES))
        {
            // we require admin rights if we're trying to set another player's attribs
            TRACE_LOG("[GameSessionMaster] Non admin player can't set other player attributes");
            return SetPlayerAttributesMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        NotifyPlayerAttribChange nPlayerAttrib;

        //update the game player information.
        player->setPlayerAttributes(request.getPlayerAttributes(), &nPlayerAttrib.getPlayerAttribs());
        if (nPlayerAttrib.getPlayerAttribs().empty())
        {
            TRACE_LOG("[GameSessionMaster].processSetPlayerAttributesMaster: no player attributes actually changed.");
            return SetPlayerAttributesMasterError::ERR_OK;
        }

        nPlayerAttrib.setGameId(getGameId());
        nPlayerAttrib.setPlayerId(player->getPlayerId());
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyPlayerAttribChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nPlayerAttrib );

        return SetPlayerAttributesMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief set the custom data for specified player

        \param[in] request - SetPlayerCustomDataRequest
        \return SetPlayerCustomDataMasterError::Error
    ***************************************************************************************************/
    SetPlayerCustomDataMasterError::Error GameSessionMaster::processSetPlayerCustomDataMaster(const SetPlayerCustomDataRequest &request, BlazeId callingPlayerId)
    {
        PlayerInfoMaster *player = mPlayerRoster.getPlayer(request.getPlayerId());
        if (player == nullptr)
        {
            WARN_LOG("[GameSessionMaster] Player(" << request.getPlayerId() << ") is not found in game(" << getGameId() << ")");
            return SetPlayerCustomDataMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }

        if (callingPlayerId == request.getPlayerId()) 
        {
            if (!hasPermission(callingPlayerId, GameAction::SET_PLAYER_CUSTOM_DATA))
            {
                TRACE_LOG("[GameSessionMaster] You do not have permission to set your player's custom data");
                return SetPlayerCustomDataMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
            }
        }
        else if (!hasPermission(callingPlayerId, GameAction::SET_OTHER_PLAYER_CUSTOM_DATA))
        {
            // only admin player can change other players custom data
            TRACE_LOG("[GameSessionMaster] Non admin player can't set other player custom data");
            return SetPlayerCustomDataMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        player->setCustomData(&request.getCustomData());

        NotifyPlayerCustomDataChange nCustomDataChange;
        nCustomDataChange.setGameId(getGameId());
        nCustomDataChange.setPlayerId(player->getPlayerId());
        nCustomDataChange.getCustomData().setData(player->getCustomData()->getData(), player->getCustomData()->getSize());
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyPlayerCustomDataChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nCustomDataChange);

        return SetPlayerCustomDataMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief set the team data for specified player

    \param[in] request - ChangeTeamIdRequest
    \param[in] callingPlayerId - the BlazeId of the player making the request
    \return ChangeGameTeamIdMasterError::Error
    ***************************************************************************************************/
    ChangeGameTeamIdMasterError::Error GameSessionMaster::processChangeGameTeamIdMaster(const ChangeTeamIdRequest &request, BlazeId callingPlayerId)
    {
        // only admin player can change team ids
        if (!hasPermission(callingPlayerId, GameAction::UPDATE_GAME_TEAM_IDS))
        {
            TRACE_LOG("[GameSessionMaster] Non admin player can't set team ids for game");
            return ChangeGameTeamIdMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }

        return changeGameTeamIdMaster(request.getTeamIndex(), request.getNewTeamId());
    }

    ChangeGameTeamIdMasterError::Error GameSessionMaster::changeGameTeamIdMaster(TeamIndex teamIndex, TeamId newTeamId)
    {
        if ( (teamIndex >= getTeamCount()) || (newTeamId == INVALID_TEAM_ID) )
        {
            TRACE_LOG("[GameSessionMaster] The cannot change TeamId to or from INVALID_TEAM_ID");
            return ChangeGameTeamIdMasterError::GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
        }

        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        // find the old team
        TeamId oldTeamId = getTeamIdByIndex(teamIndex);

        if (oldTeamId == INVALID_TEAM_ID)
        {
            // didn't find team
            return ChangeGameTeamIdMasterError::GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
        }


        if (oldTeamId == newTeamId)
        {
            // no-op
            TRACE_LOG("[GameSessionMaster] TeamIds were identical");
            return ChangeGameTeamIdMasterError::ERR_OK;
        }


        // need to ensure new team doesn't exist in game already if unique teams are required
        if ((newTeamId != ANY_TEAM_ID) && !(getGameData().getGameSettings().getAllowSameTeamId()) && isTeamPresentInGame(newTeamId))
        {
            TRACE_LOG("[GameSessionMaster] Cannot add the same team to a game twice");
            return ChangeGameTeamIdMasterError::GAMEMANAGER_ERR_DUPLICATE_TEAM_CAPACITY;
        }


        // send notifications for game and player team changes
        getGameData().getTeamIds()[teamIndex] = newTeamId;

        // for potential PS5 Match team name update
        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());

        // Players will get their updated team ids from the game update notification, individual players don't know their team ID, just their index.

        NotifyGameTeamIdChange teamIdChangeNotification;
        teamIdChangeNotification.setGameId(getGameId());
        teamIdChangeNotification.setOldTeamId(oldTeamId);
        teamIdChangeNotification.setTeamIndex(teamIndex);
        teamIdChangeNotification.setNewTeamId(newTeamId);
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyGameTeamIdChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &teamIdChangeNotification);

        return ChangeGameTeamIdMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief swap teams and roles for any players in the game

    \param[in] requestConst - SwapPlayersRequest
    \param[in] callingPlayerId - the BlazeId of the player making the request
    \param[out] errorInfo - details on errors related to the player roster
    \return SwapPlayersMasterError::Error
    ***************************************************************************************************/
    SwapPlayersMasterError::Error GameSessionMaster::processSwapPlayersMaster(const SwapPlayersRequest &requestConst, BlazeId callingPlayerId, SwapPlayersErrorInfo &errorInfo)
    {
        // Non const version so we can change slot values directly.
        SwapPlayersRequest & request = const_cast<SwapPlayersRequest &>(requestConst);

        {
            bool setMyRole = false;
            bool setMyTeamIndex = false;
            bool setMySlotType = false;
            bool setOtherRole = false;
            bool setOtherTeamIndex = false;
            bool setOtherSlotType = false;
            SwapPlayerDataList::iterator itor = request.getSwapPlayers().begin();
            SwapPlayerDataList::iterator end = request.getSwapPlayers().end();
            for (; itor != end; ++itor)
            {
                PlayerInfoMaster *player = mPlayerRoster.getPlayer((*itor)->getPlayerId());
                if (player == nullptr)
                {
                    WARN_LOG("[GameSessionMaster] Can't find the player (" << (*itor)->getPlayerId() << ").");
                    continue;
                }

                PlayerId swapPlayerId = (*itor)->getPlayerId();
                TeamIndex swapTeamIndex = (*itor)->getTeamIndex();
                const RoleName &swapRoleName = (*itor)->getRoleName();
                SlotType swapSlotType = (*itor)->getSlotType();

                // Do a quick check to see what we're swapping: 
                if (swapPlayerId != callingPlayerId)
                {
                    if (swapRoleName != player->getRoleName())
                        setOtherRole = true;
                    if (swapTeamIndex != player->getTeamIndex())
                        setOtherTeamIndex = true;
                    if (swapSlotType != player->getSlotType())
                        setOtherSlotType = true;
                }
                else
                {
                    if (swapRoleName != player->getRoleName())
                        setMyRole = true;
                    if (swapTeamIndex != player->getTeamIndex())
                        setMyTeamIndex = true;
                    if (swapSlotType != player->getSlotType())
                        setMySlotType = true;
                }
            }

            // Admins are the only ones allowed to swap >1 person (single players can swap themselves)
            if ((setMyRole          && !hasPermission(callingPlayerId, GameAction::SET_PLAYER_ROLE)) ||
                (setMySlotType      && !hasPermission(callingPlayerId, GameAction::SET_PLAYER_SLOT_TYPE)) ||
                (setMyTeamIndex     && !hasPermission(callingPlayerId, GameAction::SET_PLAYER_TEAM_INDEX)) ||
                (setOtherRole       && !hasPermission(callingPlayerId, GameAction::SET_OTHER_PLAYER_ROLE)) ||
                (setOtherSlotType   && !hasPermission(callingPlayerId, GameAction::SET_OTHER_PLAYER_SLOT_TYPE)) ||
                (setOtherTeamIndex  && !hasPermission(callingPlayerId, GameAction::SET_OTHER_PLAYER_TEAM_INDEX)) )
            {            
                TRACE_LOG("[GameSessionMaster] Missing required permission in swap players code.");

                errorInfo.setPlayerId(callingPlayerId);
                char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Player '%" PRIi64 "' is missing one of the following required permissions: %s %s %s %s %s %s", callingPlayerId, 
                                                    (setMyRole          ? "SET_PLAYER_ROLE" : ""), 
                                                    (setMySlotType      ? "SET_PLAYER_SLOT_TYPE" : ""), 
                                                    (setMyTeamIndex     ? "SET_PLAYER_TEAM_INDEX" : ""), 
                                                    (setOtherRole       ? "SET_OTHER_PLAYER_ROLE" : ""), 
                                                    (setOtherSlotType   ? "SET_OTHER_PLAYER_SLOT_TYPE" : ""), 
                                                    (setOtherTeamIndex  ? "SET_OTHER_PLAYER_TEAM_INDEX" : "") );
                errorInfo.setErrMessage(msg);

                return SwapPlayersMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
            }
        }

        // In the case of a single player case, make sure the player is valid
        if (request.getSwapPlayers().size() == 1 && mPlayerRoster.getPlayer(request.getSwapPlayers()[0]->getPlayerId()) == nullptr)
        {
            WARN_LOG("[GameSessionMaster] Player(" << request.getSwapPlayers()[0]->getPlayerId() << ") is not found in game(" << getGameId() << ")");

            errorInfo.setPlayerId(request.getSwapPlayers()[0]->getPlayerId());
            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            blaze_snzprintf(msg, sizeof(msg), "Player '%" PRIi64 "' is not a member of the game.", request.getSwapPlayers()[0]->getPlayerId());
            errorInfo.setErrMessage(msg);
            errorInfo.setPlayerId(request.getSwapPlayers()[0]->getPlayerId());

            return SwapPlayersMasterError::GAMEMANAGER_ERR_PLAYER_NOT_FOUND;
        }


        SlotCapacitiesVector expectedSlotCapacity;
        mPlayerRoster.getSlotPlayerCounts(expectedSlotCapacity);

        // Check if the team is valid or full. expectedTeamSizeMap is a expected team size map after the swap operation is completed.
        uint16_t teamCount = getTeamCount();
        eastl::map<TeamIndex, uint16_t> expectedTeamSizeMap;
        eastl::map<TeamIndex, RoleSizeMap> expectedRoleSizeMap;
        SwapPlayerDataList::iterator itor = request.getSwapPlayers().begin();
        SwapPlayerDataList::iterator end = request.getSwapPlayers().end();
        for (; itor != end; ++itor)
        {
            PlayerInfoMaster *player = mPlayerRoster.getPlayer((*itor)->getPlayerId());
            if (player == nullptr)
            {
                WARN_LOG("[GameSessionMaster] Can't find the player (" << (*itor)->getPlayerId() << ").");
                continue;
            }

            PlayerId swapPlayerId = (*itor)->getPlayerId();
            TeamIndex swapTeamIndex = (*itor)->getTeamIndex();
            const RoleName &swapRoleName = (*itor)->getRoleName();
            SlotType swapSlotType = (*itor)->getSlotType();

            if (!isParticipantSlot(swapSlotType) && !isSpectatorSlot(swapSlotType))
            {
                TRACE_LOG("[GameSessionMaster] Slot Type (" << swapSlotType << ") is not valid in game(" << getGameId() << "), continuing to use the player current slot type.");
                // Continue using the original slot:
                (*itor)->setSlotType(player->getSlotType());
                swapSlotType = (*itor)->getSlotType();
            }

            if (isParticipantSlot(swapSlotType))
            {
                // queued players are allowed to swap to UNSPECIFIED_TEAM_INDEX, because they're not really on a team until they get promoted
                if ( (swapTeamIndex >= teamCount) && !(player->isInQueue() && (swapTeamIndex == UNSPECIFIED_TEAM_INDEX)))
                {
                    TRACE_LOG("[GameSessionMaster] TeamIndex (" << swapTeamIndex << ") is not valid in game(" << getGameId() << "); greater than team count(" << teamCount << ").");

                    errorInfo.setPlayerId(swapPlayerId);
                    errorInfo.setTeamIndex(swapTeamIndex);
                    errorInfo.setRoleName(swapRoleName.c_str());
                    errorInfo.setSlotType(swapSlotType);
                    char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                    blaze_snzprintf(msg, sizeof(msg), "Player '%" PRIi64 "' failed assignment to invalid team at index '%" PRIu16 "', there are only '%" PRIu16 "' teams in this game session.",
                        swapPlayerId, swapTeamIndex, teamCount);
                    errorInfo.setErrMessage(msg);

                    return SwapPlayersMasterError::GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
                }
                if (getRoleCapacity(swapRoleName) == 0)
                {
                    // role not present in game or 0 capacity
                    TRACE_LOG("[GameSessionMaster] RoleName" << swapRoleName << ") is not valid in game(" << getGameId() << ").");

                    errorInfo.setPlayerId(swapPlayerId);
                    errorInfo.setTeamIndex(swapTeamIndex);
                    errorInfo.setRoleName(swapRoleName.c_str());
                    errorInfo.setSlotType(swapSlotType);
                    char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                    blaze_snzprintf(msg, sizeof(msg), "Player '%" PRIi64 "' failed assignment to Role '%s', which is not valid for this game session.",
                        swapPlayerId, swapRoleName.c_str());
                    errorInfo.setErrMessage(msg);

                    return SwapPlayersMasterError::GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
                }
            }

            TeamIndex currentTeamIndex = player->getTeamIndex();
            const RoleName &currentRoleName = player->getRoleName();
            SlotType currentSlotType = player->getSlotType();

            // only need to test capacities for roster players.
            if (!player->isInQueue())
            {
                // Remove from current slot/team/role count:
                --expectedSlotCapacity[currentSlotType];
                if (isParticipantSlot(currentSlotType))
                {
                    eastl::map<TeamIndex, uint16_t>::iterator currentTeamSizeIter = expectedTeamSizeMap.find(currentTeamIndex);
                    if (currentTeamSizeIter == expectedTeamSizeMap.end())
                    {
                        // tree implementation of insert returns pair, where first element is the iterator
                        currentTeamSizeIter = expectedTeamSizeMap.insert(currentTeamIndex).first;
                        currentTeamSizeIter->second = mPlayerRoster.getTeamSize(currentTeamIndex);
                    }
                    currentTeamSizeIter->second--;

                    eastl::map<TeamIndex, RoleSizeMap>::iterator currentRoleSizeIter = expectedRoleSizeMap.find(currentTeamIndex);
                    // don't need to nullptr-check return value of getRoleSizeMap() calls below because we've already validated the index is in-bounds
                    if (currentRoleSizeIter == expectedRoleSizeMap.end())
                    {
                        // tree implementation of insert returns pair, where first element is the iterator
                        currentRoleSizeIter = expectedRoleSizeMap.insert(currentTeamIndex).first;
                        mPlayerRoster.getRoleSizeMap(currentRoleSizeIter->second, currentTeamIndex);
                    }
                    currentRoleSizeIter->second[currentRoleName]--;
                }

                // Add to new slot/team/role count:
                ++expectedSlotCapacity[swapSlotType];
                if (isParticipantSlot(swapSlotType))
                {
                    eastl::map<TeamIndex, uint16_t>::iterator swapTeamSizeIter = expectedTeamSizeMap.find(swapTeamIndex);
                    if (swapTeamSizeIter == expectedTeamSizeMap.end())
                    {
                        swapTeamSizeIter = expectedTeamSizeMap.insert(swapTeamIndex).first;
                        swapTeamSizeIter->second = mPlayerRoster.getTeamSize(swapTeamIndex);
                    }
                    swapTeamSizeIter->second++;

                    eastl::map<TeamIndex, RoleSizeMap>::iterator swapRoleSizeIter = expectedRoleSizeMap.find(swapTeamIndex);
                    if (swapRoleSizeIter == expectedRoleSizeMap.end())
                    {
                        swapRoleSizeIter = expectedRoleSizeMap.insert(swapTeamIndex).first;
                        mPlayerRoster.getRoleSizeMap(swapRoleSizeIter->second, swapTeamIndex);
                    }
                    swapRoleSizeIter->second[swapRoleName]++;

                    if (currentRoleName != swapRoleName)
                    {
                        // make sure we pass the role criteria (if we're not an admin)
                        RoleEntryCriteriaEvaluatorMap::const_iterator roleCriteriaIter = mRoleEntryCriteriaEvaluators.find(swapRoleName);
                        if (!hasPermission(callingPlayerId, GameAction::SWAP_PLAYERS_SKIP_ROLE_ENTRY_CHECK) && roleCriteriaIter != mRoleEntryCriteriaEvaluators.end())
                        {
                            EntryCriteriaName failedEntryCriteria;
                            if (!roleCriteriaIter->second->evaluateEntryCriteria(swapPlayerId, player->getExtendedDataMap(), failedEntryCriteria))
                            {
                                TRACE_LOG("[GameSessionMaster] Player(" << swapPlayerId << ") failed game entry criteria for role '"
                                    << swapRoleName.c_str() << "' in game(" << getGameId() << "), failedEntryCriteria(" << failedEntryCriteria.c_str() << ")");

                                errorInfo.setPlayerId(swapPlayerId);
                                errorInfo.setTeamIndex(swapTeamIndex);
                                errorInfo.setRoleName(swapRoleName.c_str());
                                errorInfo.setSlotType(swapSlotType);
                                char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                                blaze_snzprintf(msg, sizeof(msg), "Player '%" PRIi64 "' failed assignment to Role '%s', due to role entry criteria '%s'.",
                                    swapPlayerId, swapRoleName.c_str(), failedEntryCriteria.c_str());
                                errorInfo.setErrMessage(msg);

                                return SwapPlayersMasterError::GAMEMANAGER_ERR_ROLE_CRITERIA_FAILED;
                            }
                        }
                    }

                    // When becoming a player, that's the only time when we need to recheck the game entry criteria:
                    if (!hasPermission(callingPlayerId, GameAction::SWAP_PLAYERS_SKIP_SPECTATOR_ENTRY_CHECK) && isSpectatorSlot(currentSlotType))
                    {
                        EntryCriteriaName failedEntryCriteria;
                        if (!evaluateEntryCriteria(swapPlayerId, player->getExtendedDataMap(), failedEntryCriteria))
                        {
                            WARN_LOG("[GameSessionMaster] Failed game entry criteria. Spectator cannot convert to a player.");

                            errorInfo.setPlayerId(swapPlayerId);
                            errorInfo.setTeamIndex(swapTeamIndex);
                            errorInfo.setRoleName(swapRoleName.c_str());
                            errorInfo.setSlotType(swapSlotType);
                            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                            blaze_snzprintf(msg, sizeof(msg), "Spectator '%" PRIi64 "' failed assignment to '%s', due to game entry criteria '%s'.",
                                swapPlayerId, SlotTypeToString(swapSlotType), failedEntryCriteria.c_str());
                            errorInfo.setErrMessage(msg);

                            return SwapPlayersMasterError::GAMEMANAGER_ERR_GAME_ENTRY_CRITERIA_FAILED;
                        }
                    }
                }
            }
        }

        // Public/Private slot checks:
        int16_t numGoingPublicParticipant = 0;
        if (expectedSlotCapacity[SLOT_PRIVATE_PARTICIPANT] > getSlotTypeCapacity(SLOT_PRIVATE_PARTICIPANT))
        {
            numGoingPublicParticipant = (expectedSlotCapacity[SLOT_PRIVATE_PARTICIPANT] - getSlotTypeCapacity(SLOT_PRIVATE_PARTICIPANT));
        }

        int16_t numGoingPublicSpectator = 0;
        if (expectedSlotCapacity[SLOT_PRIVATE_SPECTATOR] > getSlotTypeCapacity(SLOT_PRIVATE_SPECTATOR))
        {
            numGoingPublicSpectator = (expectedSlotCapacity[SLOT_PRIVATE_SPECTATOR] - getSlotTypeCapacity(SLOT_PRIVATE_SPECTATOR));
        }



        int16_t publicParticipantCount = expectedSlotCapacity[SLOT_PUBLIC_PARTICIPANT] + numGoingPublicParticipant;
        int16_t publicSpectatorCount = expectedSlotCapacity[SLOT_PUBLIC_SPECTATOR] + numGoingPublicSpectator;
        // public slots can't be shifted, so we need to check them last:
        if (publicParticipantCount > getSlotTypeCapacity(SLOT_PUBLIC_PARTICIPANT) ||
            publicSpectatorCount > getSlotTypeCapacity(SLOT_PUBLIC_SPECTATOR))
        {
            TRACE_LOG("[GameSessionMaster] Slots are full in game(" << getGameId() << ").");

            char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
            SlotType fullSlotType = SLOT_PUBLIC_PARTICIPANT;
            uint16_t slotTypeMembers = 0;
            if (publicParticipantCount > getSlotTypeCapacity(SLOT_PUBLIC_PARTICIPANT))
            {
               if ((getSlotTypeCapacity(SLOT_PRIVATE_PARTICIPANT) != 0) && (getSlotTypeCapacity(SLOT_PUBLIC_PARTICIPANT) == 0))
               {
                   fullSlotType = SLOT_PRIVATE_PARTICIPANT;
                   slotTypeMembers = expectedSlotCapacity[SLOT_PRIVATE_PARTICIPANT];
               }
               else
               {
                   fullSlotType = SLOT_PUBLIC_PARTICIPANT;
                   slotTypeMembers = publicParticipantCount;
               }
            }
            else // if (publicParticipantCount > getSlotTypeCapacity(SLOT_PUBLIC_SPECTATOR))
            {
                if ((getSlotTypeCapacity(SLOT_PRIVATE_SPECTATOR) != 0) && (getSlotTypeCapacity(SLOT_PUBLIC_SPECTATOR) == 0))
                {
                    fullSlotType = SLOT_PRIVATE_SPECTATOR;
                    slotTypeMembers = expectedSlotCapacity[SLOT_PRIVATE_SPECTATOR];
                }
                else
                {
                    fullSlotType = SLOT_PUBLIC_SPECTATOR;
                    slotTypeMembers = publicSpectatorCount;
                }
            }

            errorInfo.setSlotType(fullSlotType);
            blaze_snzprintf(msg, sizeof(msg), "Capacity of '%s' slots was exceeded. Failed assignment of '%" PRIu16 "' players to '%" PRIu16 "' slots.",
                SlotTypeToString(fullSlotType), slotTypeMembers, getSlotTypeCapacity(fullSlotType));
            errorInfo.setErrMessage(msg);

            return (SwapPlayersMasterError::Error)getSlotFullErrorCode(fullSlotType);
        }

        // Update the slots:
        for(itor = request.getSwapPlayers().begin(); itor != end; ++itor)
        {
            if ((*itor)->getSlotType() == SLOT_PRIVATE_PARTICIPANT && numGoingPublicParticipant > 0)
            {
                --numGoingPublicParticipant;
                (*itor)->setSlotType(SLOT_PUBLIC_PARTICIPANT);
            }
            else if ((*itor)->getSlotType() == SLOT_PRIVATE_SPECTATOR && numGoingPublicSpectator > 0)
            {
                --numGoingPublicSpectator;
                (*itor)->setSlotType(SLOT_PUBLIC_SPECTATOR);
            }
        }





        uint16_t teamCapacity = getTeamCapacity();
        for (eastl::map<TeamIndex, uint16_t>::iterator expectedTeamItor = expectedTeamSizeMap.begin(); expectedTeamItor != expectedTeamSizeMap.end(); ++expectedTeamItor)
        {
            if (expectedTeamItor->second > teamCapacity)
            {
                TRACE_LOG("[GameSessionMaster] Team (" << expectedTeamItor->first << ") in game(" << getGameId() << ") is full.");

                errorInfo.setTeamIndex(expectedTeamItor->first);
                char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Team at index '%" PRIu16 "' is over capacity. Failed assignment of '%" PRIu16 "' players to '%" PRIu16 "' spots.",
                     expectedTeamItor->first, expectedTeamItor->second, teamCapacity);
                errorInfo.setErrMessage(msg);

                return SwapPlayersMasterError::GAMEMANAGER_ERR_TEAM_FULL;
            }
        }

        // validate role sizes versus capacity
        for (eastl::map<TeamIndex, RoleSizeMap>::iterator expectedRoleTeamItor = expectedRoleSizeMap.begin(); expectedRoleTeamItor != expectedRoleSizeMap.end(); ++expectedRoleTeamItor)
        {
            for (RoleSizeMap::iterator expectedRoleItor = expectedRoleTeamItor->second.begin(); expectedRoleItor != expectedRoleTeamItor->second.end(); ++expectedRoleItor)
            {
                if (expectedRoleItor->second > getRoleCapacity(expectedRoleItor->first))
                {
                    TRACE_LOG("[GameSessionMaster] Role '" << expectedRoleItor->first.c_str() << "' in Team (" << expectedRoleTeamItor->first
                        << ") for game(" << getGameId() << ") is full.");

                    errorInfo.setTeamIndex(expectedRoleTeamItor->first);
                    errorInfo.setRoleName(expectedRoleItor->first.c_str());

                    char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                    blaze_snzprintf(msg, sizeof(msg), "Role '%s' is full on team at index '%" PRIu16 "'. Failed assignment of '%" PRIu16 "' players to '%" PRIu16 "' spots.",
                        expectedRoleItor->first.c_str(), expectedRoleTeamItor->first, expectedRoleItor->second, getRoleCapacity(expectedRoleItor->first));
                    errorInfo.setErrMessage(msg);

                    return SwapPlayersMasterError::GAMEMANAGER_ERR_ROLE_FULL;
                }
            }

            // Check that the mutirole criteria allows the new team role setup:
            EntryCriteriaName failedName;
            if (!mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(expectedRoleTeamItor->second, failedName))
            {
                TRACE_LOG("[GameSessionMaster]  game(" << getGameId() << ") failed the multirole criteria (" << failedName.c_str() << ") before swapping players.");

                errorInfo.setTeamIndex(expectedRoleTeamItor->first);

                char8_t msg[SwapPlayersErrorInfo::MAX_ERRMESSAGE_LEN];
                blaze_snzprintf(msg, sizeof(msg), "Team at index '%" PRIu16 "', failed validation due to multirole criteria '%s'.",
                    expectedRoleTeamItor->first, failedName.c_str());
                errorInfo.setErrMessage(msg);

                return SwapPlayersMasterError::GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_FAILED;
            }
        }

        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        //swap the players' team
        for(itor = request.getSwapPlayers().begin(); itor != end; ++itor)
        {
            const SwapPlayerData* swapPlayerTeamData = *itor;
            PlayerInfoMaster *player = mPlayerRoster.getPlayer(swapPlayerTeamData->getPlayerId());
            //skip if the player left the game
            if (player == nullptr)
                continue;

            //update if the player's team index is changed or their role is changed
            if ((player->getTeamIndex() != swapPlayerTeamData->getTeamIndex()) ||
                (blaze_stricmp(player->getRoleName(), swapPlayerTeamData->getRoleName()) != 0) ||
                (swapPlayerTeamData->getSlotType() != player->getSlotType()))
            {
                // ignore any errors here (since they shouldn't occur after the earlier checks)
                mPlayerRoster.updatePlayerTeamRoleAndSlot(player, swapPlayerTeamData->getTeamIndex(), swapPlayerTeamData->getRoleName(), swapPlayerTeamData->getSlotType());
            }
        }

        // if fullness changed, update external session props.
        // side: dequeues may also update external session, but done after, separate from here for flow-consistency.
        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());

        processPlayerQueue();

        return SwapPlayersMasterError::ERR_OK;
    }

     /*! ************************************************************************************************/
    /*! \brief generates a rebuilt team composition for a game session based on an input map of factions to players.
    non-specified players are rebalanced based on player count.
    Players in the teamMembership map, but no longer in the game are silently ignored.
    This does not actually update the teams, but generates the map that will be used.

    \param[in] teamMembership - map of the new factions to include in the game session to player lists for those factions
    \param[in\out] newTeamRosters - the team rosters after rebuilding the teams
    \param[in] teamCapacity - the capacity of the teams
    ***************************************************************************************************/
    void GameSessionMaster::generateRebuiltTeams( const TeamDetailsList &teamMembership, TeamDetailsList& newTeamRosters, uint16_t teamCapacity )
    {
        // first validate the potential teams
        uint16_t teamCount = teamMembership.size();
        TeamIndex smallestTeamIndex = 0;
        uint16_t smallestTeamSize = UINT16_MAX;

        eastl::map<TeamIndex, uint16_t> expectedTeamSizeMap; // track the size of each team when constructing new teams
        eastl::set<PlayerId> assignedPlayerSet;

        TeamDetailsList::const_iterator teamMembershipIter = teamMembership.begin();
        TeamDetailsList::const_iterator teamMembershipEnd = teamMembership.end();
        for(uint16_t i = 0; teamMembershipIter != teamMembershipEnd; ++i, ++teamMembershipIter)
        {
            uint16_t teamSize = 0;
            TeamDetails *teamDetails = newTeamRosters.pull_back();
            teamDetails->getTeamRoster().reserve(teamCapacity);
            teamDetails->setTeamId((*teamMembershipIter)->getTeamId());

            TeamMemberInfoList &teamMembers = teamDetails->getTeamRoster();

            // this loop isn't needed for the current sprint deliverable, but with the upcoming roles feature,
            // each player will need to be examined individually
            TeamMemberInfoList::const_iterator playerIter = (*teamMembershipIter)->getTeamRoster().begin();
            TeamMemberInfoList::const_iterator playerEnd = (*teamMembershipIter)->getTeamRoster().end();
            while(playerIter != playerEnd)
            {
                PlayerId playerId = (*playerIter)->getPlayerId();
                RoleName roleName = (*playerIter)->getPlayerRole();
                PlayerInfoMaster *player = mPlayerRoster.getPlayer(playerId);
                ++playerIter;
                //skip if the player left the game
                if (player == nullptr)
                    continue;
                // otherwise, add him to the team roster, increase the player count for the team

                TeamMemberInfo *memberInfo = teamMembers.pull_back();
                memberInfo->setPlayerId(playerId);
                memberInfo->setPlayerRole(roleName);
                teamSize++;
                // track players who have been assigned to a team
                assignedPlayerSet.insert(playerId);
            }

            expectedTeamSizeMap[i] = teamSize;
            if (teamSize < smallestTeamSize)
            {
                smallestTeamIndex = i;
                smallestTeamSize = teamSize;
            }
        }

        // assign remaining players to teams, we use ROSTER_PARTICIPANTS, not ROSTER_PLAYERS because spectators don't have a team
        const PlayerRoster::PlayerInfoList &rosterPlayerList = mPlayerRoster.getPlayers(PlayerRoster::ROSTER_PARTICIPANTS);
        PlayerRoster::PlayerInfoList::const_iterator rosterPlayerIter = rosterPlayerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator rosterPlayerEnd = rosterPlayerList.end();
        for (; rosterPlayerIter != rosterPlayerEnd; ++rosterPlayerIter)
        {
            PlayerId playerId = (*rosterPlayerIter)->getPlayerId();
            TeamIndex teamIndex = (*rosterPlayerIter)->getTeamIndex();
            RoleName roleName = (*rosterPlayerIter)->getRoleName();
            if (assignedPlayerSet.find(playerId) == assignedPlayerSet.end())
            {
                // this user may need a new team if his team was removed, or is now full
                if ((teamIndex >= teamCount) || (expectedTeamSizeMap[teamIndex] >= teamCapacity))
                {
                    //get new smallest team, only need to recalculate this if a player needs to be moved
                    for(uint16_t i = 0; i < (uint16_t)teamMembership.size(); i++)
                    {
                        if (expectedTeamSizeMap[i] < expectedTeamSizeMap[smallestTeamIndex])
                        {
                            smallestTeamIndex = i;
                        }
                    }

                    // player's current team is full
                    expectedTeamSizeMap[smallestTeamIndex]++;
                    TeamMemberInfo *newMemberInfo = newTeamRosters[smallestTeamIndex]->getTeamRoster().pull_back();
                    newMemberInfo->setPlayerId(playerId);
                    newMemberInfo->setPlayerRole(roleName);
                }
                else
                {
                    // player can remain on his current team
                    expectedTeamSizeMap[teamIndex]++;
                    TeamMemberInfo *newMemberInfo = newTeamRosters[teamIndex]->getTeamRoster().pull_back();
                    newMemberInfo->setPlayerId(playerId);
                    newMemberInfo->setPlayerRole(roleName);
                }

            }
        }
    }

     /*! ************************************************************************************************/
    /*! \brief finalizes the rebuilt team composition for a game session based on an input map of factions to players.
    non-specified players are rebalanced based on player count.
    Players in the teamMembership map, but no longer in the game are silently ignored.
    This takes the given team membership and updates all of the team data in-game.

    \param[in] teamMembership - map of the new factions to include in the game session to player lists for those factions
    \param[in\out] newTeamRosters - the team rosters after rebuilding the teams
    \param[in] teamCapacity - the capacity of the teams
    ***************************************************************************************************/
    void GameSessionMaster::finalizeRebuiltTeams( const TeamDetailsList &teamMembership, TeamDetailsList& newTeamRosters, uint16_t teamCapacity )
    {
        uint16_t teamCount = teamMembership.size();

        // now change the teams in the game
        TeamIdVector &teamIdVector = getGameData().getTeamIds();
        teamIdVector.clear();
        teamIdVector.reserve(teamCount);

        TeamDetailsList::const_iterator teamMembershipIter = teamMembership.begin();
        TeamDetailsList::const_iterator teamMembershipEnd = teamMembership.end();
        while(teamMembershipIter != teamMembershipEnd)
        {
            teamIdVector.push_back((*teamMembershipIter)->getTeamId());
            ++teamMembershipIter;
        }

        // finally, move players around
        //swap the players' team
        TeamDetailsList::const_iterator newRosterIter = newTeamRosters.begin();
        TeamDetailsList::const_iterator newRosterEnd = newTeamRosters.end();
        for(uint16_t i = 0; newRosterIter != newRosterEnd; ++i, ++newRosterIter)
        {
            TeamMemberInfoList::const_iterator teamMemberIter = (*newRosterIter)->getTeamRoster().begin();
            TeamMemberInfoList::const_iterator teamMemberEnd = (*newRosterIter)->getTeamRoster().end();
            while(teamMemberIter != teamMemberEnd)
            {
                PlayerInfoMaster *player = mPlayerRoster.getPlayer((*teamMemberIter)->getPlayerId());
                ++teamMemberIter;
                //skip if the player left the game
                if (player == nullptr)
                {
                    // this shouldn't happen, because we've verified the player's presence above and haven't blocked
                    WARN_LOG("[GameSessionMaster] Can't find the player (" << (*teamMemberIter)->getPlayerId() << ") in game(" << getGameId() << ") during TeamIndex update phase, this shouldn't happen.");
                    continue;
                }
                //skip if the player's team index is unchanged.
                if (player->getTeamIndex() == i)
                    continue;
                mPlayerRoster.updatePlayerTeamIndexForCapacityChange(player, i);
            }

        }

    }

    /*! ************************************************************************************************/
    /*! \brief adds the player to the game's roster and replicates the added player to the slaves.
        Also initializes the player in the mesh as establishing conneciton.

        \param[in] player - PlayerInfoMaster of the added player.
        \param[in] playerReplicationReason - the replication reason to use when inserting the player into the roster (can be nullptr)
        \param[in] wasQueued - whether player is being promoted from queue.
        \return JoinGameMasterError::Error associated with adding the player returns ERR_OK if successfully
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameSessionMaster::addActivePlayer(PlayerInfoMaster* player, const GameSetupReason &playerJoinContext, bool previousTeamIndexUnspecified /*= false*/, bool wasQueued /*= false*/)
    {
        if (player->isActive())
        {
            // Only update JoinedViaMatchmaking when the player becomes active (not claiming a reservation or was queued).
            player->getPlayerData()->setJoinedViaMatchmaking((playerJoinContext.getMatchmakingSetupContext() != nullptr) || (playerJoinContext.getIndirectMatchmakingSetupContext() != nullptr));
        }

        // If the player is reserved and in the queue, we are only claiming a queued reservation.
        // no need to consider them active.
        const bool isClaimQueueReservation = (player->isReserved() && player->isInQueue());
        if ( !isClaimQueueReservation )
        {
            // GM_TODO: if add player returns error, we don't clear the slot
            if (!player->isReserved())
            {
                // reserved players already have a slot.
                assignPlayerToFirstOpenSlot(*player);
            }
            // But reserved players still need a connection slot.
            assignConnectionToFirstOpenSlot(*player);

            mPlayerConnectionMesh.addEndpoint(player->getConnectionGroupId(), player->getPlayerId(), player->getClientPlatform(), player->getConnectionJoinType());
        }

        // see if the joining player needs to assume the role of platform host
        //   (ie: is this the first player joining a dedicated server game)
        // the dedicated server game only *needs* a platform host if we're not using S2S external session managment, check the config option and the status of externalSessionUtil.
        bool gameNeedsPlatformHost = ( ((isDedicatedHostedTopology(getGameNetworkTopology()) && (getPlatformHostInfo().getSlotId() == getDedicatedServerHostInfo().getSlotId())) 
                                        && (gGameManagerMaster->getConfig().getGameSession().getAssignPlatformHostForDedicatedServerGames()))
                                      || ((getGameNetworkTopology() == NETWORK_DISABLED) && (getPlatformHostInfo().getPlayerId() == INVALID_BLAZE_ID)) );

       

        if ( gameNeedsPlatformHost && player->hasHostPermission())
        {
            // Setup the platform host for the first user.
            getGameData().getPlatformHostInfo().setPlayerId(player->getPlayerId());
            getGameData().getPlatformHostInfo().setUserSessionId(player->getUserInfo().getSessionId());
            getGameData().getPlatformHostInfo().setSlotId(player->getSlotId());
            getGameData().getPlatformHostInfo().setConnectionGroupId(player->getConnectionGroupId());
            getGameData().getPlatformHostInfo().setConnectionSlotId(player->getConnectionSlotId());
            TRACE_LOG("[GameSessionMaster] player(" << getPlatformHostInfo().getPlayerId() << ") in slot " << getPlatformHostInfo().getSlotId() 
                << " and connection slot " << getPlatformHostInfo().getConnectionSlotId() << " set as platform host of game(" << getGameId() << ").");

            NotifyPlatformHostInitialized notifyPlatformHostInitialized;
            notifyPlatformHostInitialized.setGameId(getGameId());
            notifyPlatformHostInitialized.setPlatformHostSlotId(getPlatformHostInfo().getSlotId());
            notifyPlatformHostInitialized.setPlatformHostId(getPlatformHostInfo().getPlayerId());
            GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
            gGameManagerMaster->sendNotifyPlatformHostInitializedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &notifyPlatformHostInitialized);
        }
        


        bool claimedReservation = false;
        if (wasQueued)
        {
            // player entering game from game's queue
            if (!mPlayerRoster.promoteQueuedPlayer(player, playerJoinContext))
            {
                return ERR_SYSTEM;
            }
        }
        // Reserved players can be in both the roster and in the queue.
        else if (player->isReserved())
        {
            // Topology host is always connected (even if previously reserved)
            PlayerState newState;

            if (getTopologyHostSessionId() == player->getPlayerSessionId() || getPlayerRoster()->hasConnectedPlayersOnConnection(player->getConnectionGroupId()))
            {
                newState = ACTIVE_CONNECTED;
            }
            else
            {
                newState = ACTIVE_CONNECTING;
            }
            

            // player entering game from a reservation
            if (!mPlayerRoster.promoteReservedPlayer(player, playerJoinContext, newState, previousTeamIndexUnspecified))
            {
                return ERR_SYSTEM;
            }
            claimedReservation = true;
        }
        else
        {
            // player entering game directly, wasn't in queue or reserved
            if (!mPlayerRoster.addNewPlayer(player, playerJoinContext))
            {
                return ERR_SYSTEM;
            }
        }
        
        mMetricsHandler.onPlayerAdded(*player, claimedReservation);

        for (CensusEntrySet::const_iterator i = mCensusEntrySet.begin(), e = mCensusEntrySet.end(); i != e; ++i)
        {
            // game is already tracking census entries, that means the newly added active player needs to track them as well
            CensusEntry& entry = **i;
            player->addToCensusEntry(entry);
        }

        // update the player's list of associated games
        if ((player->getPlayerState() == QUEUED) && (player->getOriginalPlayerSessionId() != player->getPlayerSessionId()))
        {
            // Queued reservations track their original making (HTTP/gRPC) user session (to allow cleanup/remove of reservation from game if the session goes away (queue has no reservation timeouts)).
            // Untrack the original user session now since since we're replacing with a new user session.
            gGameManagerMaster->removeUserSessionGameInfo(player->getOriginalPlayerSessionId(), *this, false);
        }

        if (!isPseudoGame())
        {
             gGameManagerMaster->addUserSessionGameInfo(player->getPlayerSessionId(), *this, (player->getPlayerState() != QUEUED), &(player->getPlayerData()->getUserInfo()));
        }

        TRACE_LOG("[GameSessionMaster].addActivePlayer Player(" << player->getPlayerId() << ") in state(" << PlayerStateToString(player->getPlayerState()) << ") added to game(" << getGameId() << ") in slot(" << player->getSlotId() << ") with teamIndex(" << player->getTeamIndex() << ").");

        // p2p full mesh games must update the game's NAT type when players join/leave,
        //   since the game's NAT type is that of the most restrictive player
        if (isNetworkFullMesh())
        {
            updateGameNatType();
        }

        //  remove player from DnfList if it was added from a prior leave
        GameDataMaster::DnfPlayerList::iterator itDnfPlayerEnd = mGameDataMaster->getDnfPlayerList().end();
        GameDataMaster::DnfPlayerList::iterator itDnfPlayer = mGameDataMaster->getDnfPlayerList().begin();
        for (; itDnfPlayer != itDnfPlayerEnd; ++itDnfPlayer)
        {
            if ((*itDnfPlayer)->getPlayerId() == player->getPlayerId())
            {
                mGameDataMaster->getDnfPlayerList().erase(itDnfPlayer);
                break;
            }
        }

        // Note that reserved players do not pass through here and thus do not have user extended data
        // associating them to the game.
        BlazeRpcError err = ERR_OK;
        err = gUserSessionManager->insertBlazeObjectIdForSession(player->getPlayerSessionId(), getBlazeObjectId());
        if (err != ERR_OK)
        {
            WARN_LOG("[GameSessionMaster].addActivePlayer: " << (ErrorHelp::getErrorName(err)) << " error adding player session " << player->getPlayerSessionId() << " to game " << getGameId());
        }

        mGameNetwork->onActivePlayerJoined(this, player);
        
        return ERR_OK;
    }


    /*! ************************************************************************************************/
    /*! \brief reserves a slot for a disconnected player in the game.

        \param[in] player - playerInfo of the player being removed.
    ***************************************************************************************************/
    RemovePlayerMasterError::Error GameSessionMaster::reserveForDisconnectedPlayer(PlayerInfoMaster* player, PlayerRemovedReason reason)
    {
        player->getPlayerData()->getPlayerSettings().setHasDisconnectReservation();
        ConnectionGroupId connGroupId = player->getConnectionGroupId();
        SlotId connSlotId = player->getConnectionSlotId();

        // start disconnect reservation timer
        player->startReservationTimer(0, true);

        bool wasActive = player->isActive();  // Check for a non-queued (or reserved) state, so that we decrement metrics correctly. 
        changePlayerState(player, RESERVED);

        // remove player from the network mesh
        if ( !mPlayerConnectionMesh.removeEndpoint(player->getConnectionGroupId(), player->getPlayerId(), reason))
        {
            ERR_LOG("[GameSessionMaster] Remove player(" << player->getPlayerId() << ") failed on network player information removal.");
        }

        // save player's attributes for game report if the game is in progress.
        if (isGameInProgress() && (mGameDataMaster->getDnfPlayerList().size() < gGameManagerMaster->getDnfListMax()))
        {
            GameDataMaster::DnfPlayer *dnfPlayer = mGameDataMaster->getDnfPlayerList().pull_back();
            dnfPlayer->setPlayerId(player->getPlayerId());
            player->getPlayerAttribs().copyInto(dnfPlayer->getAttributes());
            dnfPlayer->setAccountLocale(player->getAccountLocale());
            dnfPlayer->setRemovedReason(reason);
            dnfPlayer->setUserSessionId(player->getPlayerSessionId());
            dnfPlayer->setEncryptedBlazeId(player->getPlayerData()->getEncryptedBlazeId());
            dnfPlayer->setTeamIndex(player->getTeamIndex());
        }

        // now free up the connection slot, if this was the only player in the connection group.
        if (!mPlayerRoster.hasPlayersOnConnection(connGroupId))
        {
            markConnectionSlotOpen(connSlotId, connGroupId);
            player->getPlayerData()->setConnectionSlotId(DEFAULT_JOINING_SLOT);
        }

        mMetricsHandler.onPlayerReservationAdded(*player, wasActive);

        return RemovePlayerMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief removes a player from the game. send the player left game event.

        \param[in] player - playerInfo of the player being removed.
        \param[in] reason - reason why the player being removed.
        \param[out] removedExternalUserInfos - if non nullptr, appends the removed user's external info for caller to update game's external session.
    ***************************************************************************************************/
    RemovePlayerMasterError::Error GameSessionMaster::removePlayer(PlayerInfoMaster* player, const PlayerRemovalContext &removalContext, LeaveGroupExternalSessionParameters *externalSessionParams /*= nullptr*/, bool isRemovingAllPlayers /*= false*/)
    {
        player->removeFromCensusEntries();  // player is going away, remove it from all census entries

        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        // extsess updates here maybe used to cleanup server disconnect cases, when connected leave flow can't:
        ExternalSessionUpdateEventContext extUpdateContext;
        if (removalContext.mPlayerRemovedReason == GameManager::BLAZESERVER_CONN_LOST)
        {
            auto extPlayerUpdate = extUpdateContext.getMembershipChangedContext().pull_back();
            extPlayerUpdate->setRemoveReason(removalContext.mPlayerRemovedReason);
            extPlayerUpdate->setPlayerState(player->getPlayerState());
            UserInfo::filloutUserIdentification(player->getUserInfo().getUserInfo(), extPlayerUpdate->getUser().getUserIdentification());
        }

        // disassociate 1st party session as needed. Do this immediately to prevent joins to a dying 1st party session. NOOPs for non-applicable platforms:
        for (auto it : origUpdateInfo.getPresenceModeByPlatform()) //check all plats in case user in off-platform extsessions
        {
            untrackExtSessionMembership(player->getPlayerId(), it.first, nullptr);
        }

        // cache external session ids in case we need them later (and they've been cleared out)
        if ((externalSessionParams != nullptr) && !isExtSessIdentSet(externalSessionParams->getSessionIdentification()))
        {
            getExternalSessionIdentification().copyInto(externalSessionParams->getSessionIdentification());
        }


        // remove this game from the player's sessionGameInfo
        gGameManagerMaster->removeUserSessionGameInfo(player->getPlayerSessionId(), *this, player->isActive(), &(player->getPlayerData()->getUserInfo()));

        // remove the game from the session's extended user data gameId list
        if (player->getSessionExists())
        {
            //  Remove EA::TDF::ObjectId from UserSession's list.
            BlazeRpcError err = ERR_OK;
            err = gUserSessionManager->removeBlazeObjectIdForSession(player->getPlayerSessionId(), getBlazeObjectId());
            if (err != ERR_OK)
            {
                WARN_LOG("[GameSessionMaster].removePlayer: " << (ErrorHelp::getErrorName(err)) << " error removing player session " << player->getPlayerSessionId() << " from game " << getGameId());
            }
        }

        // make disconnect reservation if it is enabled in server and in game setting and the removed reason satisfies the condition
        // We specifically check for HOST_EJECTED, since we want to maintain any DC reservations between ejections.  
        // NOTE:  We do not maintain normal reservations after a host eject - this is only done to support disconnect reservations. 
        if (!isRemovingAllPlayers || removalContext.mPlayerRemovedReason == HOST_EJECTED)
        {
            if (getGameSettings().getDisconnectReservation() && gGameManagerMaster->getDisconnectReservationTimeout() != 0 && checkDisconnectReservationCondition(removalContext))
            {
                // Spectator/queued players who join a match probably don't need reservations created when they leave the match.
                if (!gGameManagerMaster->getEnableSpectatorDisconnectReservations() && player->isSpectator())
                {
                    TRACE_LOG("[GameSessionMaster].removePlayer: Not making disconnect reservation for spectator player (" << player->getPlayerId() << ").");
                }
                else if (!gGameManagerMaster->getEnableQueuedDisconnectReservations() && player->isInQueue())
                {
                    TRACE_LOG("[GameSessionMaster].removePlayer: Not making disconnect reservation for in-queue player (" << player->getPlayerId() << ").");
                }
                else
                {
                    if (player->getPlayerDataMaster()->getReservationTimeout() == 0 && !player->isReserved())
                    {
                        // remove player from the admin list (no-op if not an admin), pick new admin player if necessary
                        removeAdminPlayer(player->getPlayerId(), INVALID_BLAZE_ID, removalKicksConngroup(removalContext.mPlayerRemovedReason) ? player->getConnectionGroupId() : INVALID_CONNECTION_GROUP_ID);
                        player->cancelJoinGameTimer();
                        return reserveForDisconnectedPlayer(player, removalContext.mPlayerRemovedReason);
                    }
                    else
                    {
                        TRACE_LOG("[GameSessionMaster].removePlayer: " << "Not making disconnect reservation as"
                            << ((player->getPlayerDataMaster()->getReservationTimeout() == 0) ? "" : " Disconnect reservation timer already active")
                            << (player->isReserved() ? " Player already reserved" : "")
                        );
                        return RemovePlayerMasterError::ERR_OK;
                    }
                }
            }
        }

        if (!isRemovingAllPlayers)
        { 
            // remove player from the admin list (no-op if not an admin), pick new admin player if necessary
            removeAdminPlayer(player->getPlayerId(), INVALID_BLAZE_ID, removalKicksConngroup(removalContext.mPlayerRemovedReason) ? player->getConnectionGroupId() : INVALID_CONNECTION_GROUP_ID);
        }

        // do we need to ban the player's account too
        if (isRemovalReasonABan(removalContext.mPlayerRemovedReason))
        {
            banAccount(player->getAccountId());
        }

        // if dynamic reputation requirement enabled, adjust allowAnyReputation as needed.
        // for efficiency no need to update if we're removing all players from game.
        if (!isRemovingAllPlayers)
        {
            updateAllowAnyReputationOnRemove(player->getUserInfo());
        }

        mMetricsHandler.onPlayerRemoved(*player, &removalContext.mPlayerRemovedReason, true);

        // build & submit mp_match_info event for player removal
        // we do this before removing the player, because we want to capture the state of the game as it was
        // _prior_ to player removal.
        // skip generating the event for a player being removed due to QoS validation failure- they haven't had a match_join event generated yet
        if (!isPlayerInMatchmakingQosDataMap(player->getPlayerId()))
        {
            gGameManagerMaster->buildAndSendMatchLeftEvent(*this, *player, removalContext);
        }

        // Send the ConnStats Event for every connection that involved this player:
        for (auto& pinConnEvents : gGameManagerMaster->getMetrics().mConnectionStatsEventListByConnGroup[getGameId()])
        {
            GameManagerMasterMetrics::ConnectionGroupIdPair connPair = pinConnEvents.first;
            if (connPair.first == player->getConnectionGroupId() || connPair.second == player->getConnectionGroupId())
                gGameManagerMaster->sendPINConnStatsEvent(this, connPair.first, pinConnEvents.second);
        }

        // send player leave event
        submitEventPlayerLeftGame(*player, *this, removalContext);

        // Queued players can be directly deleted now.
        if (player->isInQueue())
        {
            eraseUserGroupIdFromLists(player->getOriginalUserGroupId(), true);
            
            mPlayerRoster.deletePlayer(player, removalContext);

            if (!isRemovingAllPlayers)
            {
                updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo(), extUpdateContext);
            }
         
            return RemovePlayerMasterError::ERR_OK;
        }

        //free up slot
        markSlotOpen(player->getSlotId());
        TRACE_LOG("[GameSessionMaster] Player(" << player->getPlayerId() << ":connGroupId=" << player->getConnectionGroupId() << ") removed from game(" << getGameId() << "," 
                  << Blaze::GameNetworkTopologyToString(getGameNetworkTopology())
                  << ") for " << PlayerRemovedReasonToString(removalContext.mPlayerRemovedReason) << " and slot "
                  << player->getSlotId() << " opened. Client Context code (" << removalContext.mPlayerRemovedTitleContext
                  << "), Client Context String (" << removalContext.mTitleContextString << ")");

        //cache playerId since player object will be deleted by mPlayerRoster.deletePlayer
        PlayerId leavingPlayerId = player->getPlayerId();
        bool playerIsReserved = player->isReserved();

        // Reserved players are not part of the network mesh, or count against admin lists.
        if (!playerIsReserved)
        {
            // remove player from the network mesh
            if ( !mPlayerConnectionMesh.removeEndpoint(player->getConnectionGroupId(), player->getPlayerId(), removalContext.mPlayerRemovedReason))
            {
                ERR_LOG("[GameSessionMaster] Remove player(" << player->getPlayerId() << ") failed on network player information removal.");
            }


            // save player's attributes for game report if the game is in progress.
            if (isGameInProgress() && (mGameDataMaster->getDnfPlayerList().size() < gGameManagerMaster->getDnfListMax()))
            {
                GameDataMaster::DnfPlayer *dnfPlayer = mGameDataMaster->getDnfPlayerList().pull_back();
                dnfPlayer->setPlayerId(leavingPlayerId);
                player->getPlayerAttribs().copyInto(dnfPlayer->getAttributes());
                dnfPlayer->setAccountLocale(player->getAccountLocale());
                dnfPlayer->setAccountCountry(player->getAccountCountry());
                dnfPlayer->setRemovedReason(removalContext.mPlayerRemovedReason);
                dnfPlayer->setUserSessionId(player->getPlayerSessionId());
                dnfPlayer->setEncryptedBlazeId(player->getPlayerData()->getEncryptedBlazeId());
                dnfPlayer->setTeamIndex(player->getTeamIndex());
                UserInfo::filloutUserIdentification(player->getUserInfo().getUserInfo(), dnfPlayer->getUserIdentification());
            }
        }

        // remove this player from the matchmaking qos data
        // this is done after removing the endpoint from the mesh
        // so that the mesh can properly determine if the removed
        // player was in the MM qos data map
        if ((removalContext.mPlayerRemovedReason == PLAYER_LEFT) || (removalContext.mPlayerRemovedReason == GROUP_LEFT) ||
            removalContext.mPlayerRemovedReason == PLAYER_LEFT_CANCELLED_MATCHMAKING || removalContext.mPlayerRemovedReason == PLAYER_LEFT_SWITCHED_GAME_SESSION)
        {
            // if this was voluntary leave (or due to pg non-leader self-leaving on protocol mismatch)
            // delete it from qos validation to avoid failing qos for other MM session members.
            PlayerIdList mmQosList;
            mmQosList.push_back(player->getPlayerId());
            removeMatchmakingQosData(mmQosList);
        }
        else if((player->getJoinMethod() == JOIN_BY_MATCHMAKING) || player->getJoinedViaMatchmaking())
        {
            const PlayerNetConnectionStatus status = DISCONNECTED;
            updateMatchmakingQosData(player->getPlayerId(), &status, player->getConnectionGroupId());
        }

        SlotId connSlotId = player->getConnectionSlotId();
        ConnectionGroupId connGroupId = player->getConnectionGroupId();

        // if originally full, and game is not being removed, we should lock for preferred joins when the player leaves.
        bool shouldLockForPreferredJoins = ((removalContext.mPlayerRemovedReason != GAME_DESTROYED) && (removalContext.mPlayerRemovedReason != GAME_ENDED) &&
            gGameManagerMaster->getLockGamesForPreferredJoins() && isGameAndQueueFull());
        if (shouldLockForPreferredJoins)
        {
            TRACE_LOG("[GameSessionMaster].removePlayer: locking for preferred joins if not full on remove of player " << leavingPlayerId << ", for reason '" << PlayerRemovedReasonToString(removalContext.mPlayerRemovedReason) << "', for game id " << getGameId());
            lockForPreferredJoins();
        }

        // We return the list of actual removed external ids, so slave caller can know which ones actually need to be removed from the game's external session
        // (some ops like ban player and leave game by group, may specify extra users not in the game, which don't need to be removed from the external session).
        // Note: This code is only reached for players in the game roster, queued players are handled above and are already removed from the external session.
        if (externalSessionParams != nullptr)
            updateExternalSessionLeaveParams(*externalSessionParams, player->getUserInfo(), getReplicatedGameSession());


        // delete the player (NOTE: player is nullptr after this call)
        mPlayerRoster.deletePlayer(player, removalContext, shouldLockForPreferredJoins, isRemovingAllPlayers);

        // If the last player on this connection, open the connections slot
        // to be valid this has to occur after PlayerRosterMaster::delete player such that the removed player
        // is already untracked
        if (!mPlayerRoster.hasPlayersOnConnection(connGroupId))
        {
            markConnectionSlotOpen(connSlotId, connGroupId);
        }

        if (removalContext.mPlayerRemovedReason != GAME_DESTROYED
            && removalContext.mPlayerRemovedReason != GAME_ENDED)
        {
            // if the game's not being destroyed, see if we need to fixup the game after removing the player
            //  (promote a new admin, end host migration, pump the game's player queue, etc)

            if (!playerIsReserved)
            {
                mGameNetwork->onActivePlayerRemoved(this);

                if (isHostMigrationDone())
                {
                    //FIXME: the active migrating check is not quite working in platform host migration
                    TRACE_LOG("[GameSessionMaster::removePlayer] as there is no more active migrating players");
                    hostMigrationCompleted();
                }

                // p2p full mesh games must update the game's NAT type when players join/leave,
                //   since the game's NAT type is that of the most restrictive player
                if (isNetworkFullMesh())
                {
                    updateGameNatType();
                    // GM_AUDIT: inefficient - pumping the queue will update the Nat type too - should consolidate
                }
            }

            // if fullness changed, update external session props. do this after the removal but before dequeues.
            // side: dequeues may also update external session, but done after, separate from here for flow-consistency.
            if (!isRemovingAllPlayers)
            {
                updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo(), extUpdateContext);
            }

            // Always attempt to pump the admin managed queue.  This will send a notification
            // to all admins in the game to add a new player from the queue.  We don't
            // worry about timing of host migration here.
            processPlayerAdminManagedQueue();

            // we track this in case we don't complete server's pumping the queue in the host migration case below,
            // to prevent the preferred joins lock from lifting until deques complete.
            if (shouldPumpServerManagedQueue())
            {
                incrementQueuePumpsPending();
                // marks game so new joiners can't jump the queue
                updatePromotabilityOfQueuedPlayers();
            }

            // If the player removed was the host, the game will either be destroyed or migrated
            // Either way we don't need to tell queued players to join as their connection would fail.
            // We will pump the queue again after host migration has completed.
            mPlayerRoster.updateQueuedPlayersDataOnPlayerRemove(leavingPlayerId);
            bool topologyHostLeft = (getTopologyHostInfo().getPlayerId() == leavingPlayerId);
            bool platformHostLeft = (getPlatformHostInfo().getPlayerId() == leavingPlayerId);
            if (!topologyHostLeft && !platformHostLeft)
            {
                // pump the game's player queue
                processPlayerServerManagedQueue();
            }

            // This player left, so ensure we're not waiting on it for any preferred join reply. Note: we do this after the above
            // incrementQueuePumpsPending() to prevent prematurely lift the lock here (on empty PreferredJoinSet) until dequeues complete.
            eraseFromPendingPreferredJoinSet(leavingPlayerId);
        }

        return RemovePlayerMasterError::ERR_OK;
    }

    
    /*! ************************************************************************************************/
    /*! \brief remove all players from a game
        \param[in]reason - reason why players are removed
    ***************************************************************************************************/
    void GameSessionMaster::removeAllPlayers(const PlayerRemovedReason& reason)
    {
        // TODO: We may want to add a new flag to the game to avoid adding new sessions into it while removing all players?
        PlayerRoster::PlayerInfoList playerList = mPlayerRoster.getPlayers(PlayerRoster::ALL_PLAYERS);

        PlayerRemovalContext playerRemovalContext(reason, 0, nullptr);
        LeaveGroupExternalSessionParameters externalSessionParams;

        // removePlayer invalidates the iterator to playerList so just remove the first player every time.
        for (PlayerRoster::PlayerInfoList::iterator i = playerList.begin(), e = playerList.end(); i != e; ++i)
        {
            PlayerInfoMaster* player = static_cast<PlayerInfoMaster*>(*i);
            PlayerId playerId = (player!=nullptr)?player->getPlayerId():0;

            // pass in true here to avoid leaving members from external session individually. We will group leave them below.
            RemovePlayerMasterError::Error err = removePlayer(player, playerRemovalContext, &externalSessionParams, true);
            TRACE_LOG("[GameSessionMaster].removeAllPlayers Removed player(" << playerId << ") from game("
                << getGameId() << ") with error: " << (ErrorHelp::getErrorName((BlazeRpcError) err)));
        }

        // group leave external session. Pre: no need to check remove reason here as all paths through here need ensuring external session emptied.
        // note: MPSD call below will guarantee all get left (even if some members get left elsewhere before below fiber fires).
        if (!externalSessionParams.getExternalUserLeaveInfos().empty())
        {
            // note we can pass the user infos directly here as the fiber cb immediately takes ownership before blocking
            Fiber::CreateParams params;
            params.namedContext = "GameManagerMasterImpl::handleLeaveExternalSession";

            // We create a copy of external session params for the fiber pool schedule call, which later will be owned by GameManagerMasterImpl::handleLeaveExternalSession()
            LeaveGroupExternalSessionParametersPtr leaveGroupExternalSessonParams = BLAZE_NEW LeaveGroupExternalSessionParameters(externalSessionParams);

            gGameManagerMaster->getExternalSessionJobQueue().queueFiberJob(gGameManagerMaster, &GameManagerMasterImpl::handleLeaveExternalSession, leaveGroupExternalSessonParams, "GameManagerMasterImpl::handleLeaveExternalSession");
        }
    }

    void GameSessionMaster::eraseUserGroupIdFromLists(const UserGroupId& queuedPlayerGroupId, bool onQueuedPlayerRemoval)
    {
        if (getGameType() == GAME_TYPE_GAMESESSION) 
        {
            if (queuedPlayerGroupId != EA::TDF::OBJECT_ID_INVALID) 
            {
                bool eraseGroupId = true;
                if (onQueuedPlayerRemoval)
                {
                    GameDataMaster::UserGroupQueuedPlayersCountList::iterator iter = eastl::find_if(mGameDataMaster->getUserGroupQueuedPlayersCountList().begin(), mGameDataMaster->getUserGroupQueuedPlayersCountList().end(), UserGroupQueuedPlayersCountEquals(queuedPlayerGroupId));
                    if (iter != mGameDataMaster->getUserGroupQueuedPlayersCountList().end())
                    {
                        (*iter)->setQueuedPlayersCount(((*iter)->getQueuedPlayersCount()) - 1);
                        // Only erase the entry in both the lists if no more queued players for this group. 
                        if ((*iter)->getQueuedPlayersCount() != 0)
                        {
                            eraseGroupId = false;
                        }
                    }
                }

                if (eraseGroupId)
                {
                    GameDataMaster::UserGroupQueuedPlayersCountList::iterator iter = eastl::find_if(mGameDataMaster->getUserGroupQueuedPlayersCountList().begin(), mGameDataMaster->getUserGroupQueuedPlayersCountList().end(), UserGroupQueuedPlayersCountEquals(queuedPlayerGroupId));
                    if (iter != mGameDataMaster->getUserGroupQueuedPlayersCountList().end())
                    {
                        mGameDataMaster->getUserGroupQueuedPlayersCountList().erase(iter);
                    }
                    GameDataMaster::UserGroupTargetPlayerList::iterator iterTP = eastl::find_if(mGameDataMaster->getUserGroupTargetPlayerList().begin(), mGameDataMaster->getUserGroupTargetPlayerList().end(), UserGroupTargetPlayerEquals(queuedPlayerGroupId));
                    if (iterTP != mGameDataMaster->getUserGroupTargetPlayerList().end())
                    {
                        mGameDataMaster->getUserGroupTargetPlayerList().erase(iterTP);
                    }
                }
            }
        }
    }

    PlayerInfoMaster* GameSessionMaster::getTargetPlayer(const UserGroupId& groupId, bool& groupFound) const
    {
        if (getGameType() == GAME_TYPE_GAMESESSION)
        {
            if (groupId != EA::TDF::OBJECT_ID_INVALID)
            {
                GameDataMaster::UserGroupTargetPlayerList::iterator iter = eastl::find_if(mGameDataMaster->getUserGroupTargetPlayerList().begin(), mGameDataMaster->getUserGroupTargetPlayerList().end(), UserGroupTargetPlayerEquals(groupId));
                if (iter != mGameDataMaster->getUserGroupTargetPlayerList().end())
                {
                    groupFound = true;
                    return mPlayerRoster.getPlayer((*iter)->getTargetPlayerId());
                }
            }
        }

        groupFound = false;
        return nullptr;
    }

    void GameSessionMaster::addTargetPlayerIdForGroup(const UserGroupId& groupId, PlayerId playerId)
    {
        GameDataMaster::UserGroupTargetPlayerList::iterator iter = eastl::find_if(mGameDataMaster->getUserGroupTargetPlayerList().begin(), mGameDataMaster->getUserGroupTargetPlayerList().end(), UserGroupTargetPlayerEquals(groupId));
        if (iter == mGameDataMaster->getUserGroupTargetPlayerList().end())
        {
            GameDataMaster::UserGroupTargetPlayer *userGroupTargetPlayer = mGameDataMaster->getUserGroupTargetPlayerList().pull_back();
            userGroupTargetPlayer->setUserGroupId(groupId);
            userGroupTargetPlayer->setTargetPlayerId(playerId);
        }
    }

    void GameSessionMaster::addQueuedPlayerCountForGroup(const UserGroupId& groupId)
    {
        GameDataMaster::UserGroupQueuedPlayersCountList::iterator iter = eastl::find_if(mGameDataMaster->getUserGroupQueuedPlayersCountList().begin(), mGameDataMaster->getUserGroupQueuedPlayersCountList().end(), UserGroupQueuedPlayersCountEquals(groupId));
        if (iter == mGameDataMaster->getUserGroupQueuedPlayersCountList().end())
        {
            GameDataMaster::UserGroupQueuedPlayersCount *userGroupQueuedPlayerCount = mGameDataMaster->getUserGroupQueuedPlayersCountList().pull_back();
            userGroupQueuedPlayerCount->setUserGroupId(groupId);
            userGroupQueuedPlayerCount->setQueuedPlayersCount(1);
        }
        else
        {
            (*iter)->setQueuedPlayersCount(((*iter)->getQueuedPlayersCount()) + 1);
        }
    }

    /*! ************************************************************************************************/
    /*! \brief free hosted connections that belong to this game
    ***************************************************************************************************/
    void GameSessionMaster::cleanupCCS(bool immediate)
    {
        TRACE_LOG("[GameSessionMaster].cleanupCCS: game("<< getGameId()<<"). clean up CCS as it is no longer required.");
        if (mGameDataMaster->getCCSRequestIssueTime() != 0)
        {
            gGameManagerMaster->getCCSRequestIssueTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setCCSRequestIssueTime(0);
        }
        
        if (mGameDataMaster->getCCSExtendLeaseTime() != 0)
        {
            gGameManagerMaster->getCCSExtendLeaseTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setCCSExtendLeaseTime(0);
        }

        mGameDataMaster->getPendingCCSRequests().clear();

        const char8_t* connSetId = mGameDataMaster->getConnectionSetId();
        if (connSetId != nullptr && connSetId[0] !='\0')
        {
            mPlayerConnectionMesh.freeHostedConnectivityMesh(immediate);
        }
        mGameDataMaster->setNumHostedConnections(0);
        mGameDataMaster->setConnectionSetId("");
    }

    

    /*! ************************************************************************************************/
    /*! \brief add the CCSRequestPair to the pending requests and fire off the timer.
    ***************************************************************************************************/
    void GameSessionMaster::addPairToPendingCCSRequests(const CCSRequestPair& requestPair)
    {
        GameDataMaster::CCSRequests::iterator iter = eastl::find_if(mGameDataMaster->getPendingCCSRequests().begin(), mGameDataMaster->getPendingCCSRequests().end(), CCSRequestPairEqual(requestPair));
        if (iter == mGameDataMaster->getPendingCCSRequests().end())
        {
            TRACE_LOG("[GameSessionMaster].addPairToPendingCCSRequests: game("<< getGameId() <<"). added pending CCS request for the source,target pair(" << requestPair.getConsoleFirstConnGrpId() <<","<< requestPair.getConsoleSecondConnGrpId()<< "). Waiting for the trigger from target. ");
            requestPair.copyInto(*(mGameDataMaster->getPendingCCSRequests().pull_back()));
        }
        else
        {
            TRACE_LOG("[GameSessionMaster].addPairToPendingCCSRequests: game("<< getGameId() <<"). CCS trigger from target now available. The pair(" << requestPair.getConsoleSecondConnGrpId() <<","<< requestPair.getConsoleFirstConnGrpId() << ") now eligible for issuing assistance from CCS.");
        }

        scheduleCCSRequestTimer();
    }

    /*! ************************************************************************************************/
    /*! \brief If the timer is not already active, schedule it.
    ***************************************************************************************************/
    void GameSessionMaster::scheduleCCSRequestTimer()
    {
        if (mGameDataMaster->getCCSRequestIssueTime() == 0)
        {
            TimeValue timeToFire = TimeValue::getTimeOfDay() + gGameManagerMaster->getCCSRequestIssueDelay();
            if (gGameManagerMaster->getCCSRequestIssueTimerSet().scheduleTimer(getGameId(), timeToFire))
            {
                mGameDataMaster->setCCSRequestIssueTime(timeToFire);
            }
            else
            {
                ERR_LOG("[GameSessionMaster].scheduleCCSRequestTimer: game("<< getGameId() <<"). failed to schedule CCS request.");
            }
        }
    }

    MeshEndpointsConnectionLostMasterError::Error GameSessionMaster::updatePlayersConnLostStatus(GameId gameId, const CCSRequestPair* requestPair)
    {
        UpdateMeshConnectionMasterRequest req;
        req.setGameId(gameId);
        req.setPlayerNetConnectionStatus(Blaze::GameManager::DISCONNECTED);

        req.setSourceConnectionGroupId(requestPair->getConsoleFirstConnGrpId());
        req.setTargetConnectionGroupId(requestPair->getConsoleSecondConnGrpId());
        if (meshEndpointsConnectionLost(req) == MeshEndpointsConnectionLostMasterError::Error::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE)
        {
            return MeshEndpointsConnectionLostMasterError::Error::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE;
        }

        req.setSourceConnectionGroupId(requestPair->getConsoleSecondConnGrpId());
        req.setTargetConnectionGroupId(requestPair->getConsoleFirstConnGrpId());
        if (meshEndpointsConnectionLost(req) == MeshEndpointsConnectionLostMasterError::Error::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE)
        {
            return MeshEndpointsConnectionLostMasterError::Error::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE;
        }
        return MeshEndpointsConnectionLostMasterError::Error::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Called a set time after the need for a ccs hosting server is triggered. Issues the request to the slave for 
        obtaining the hosting server.
    ***************************************************************************************************/
    void GameSessionMaster::ccsRequestIssueTimerFired(GameId gameId)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(gameId);
        if (gameSession == nullptr)
            return;

        TRACE_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). ccs request timer fired. Issue any eligible requests.");

        gameSession->mGameDataMaster->setCCSRequestIssueTime(0);

        const char8_t* connSetId = gameSession->mGameDataMaster->getConnectionSetId();
        bool haveValidConnSetId = connSetId && connSetId[0] != '\0';
        uint32_t numInFlightRequests = gameSession->mGameDataMaster->getNumInflightCCSRequests();
        if (numInFlightRequests > 0 && !haveValidConnSetId)
        {
            TRACE_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). reschedule timer for later as waiting for a valid connection set id. Number of requests in flight:" << numInFlightRequests);
            gameSession->scheduleCCSRequestTimer();
            return;
        }
        
        Blaze::GameManager::CCSAllocateRequest ccsRequest;
        GameDataMaster::CCSRequests::iterator iter = gameSession->mGameDataMaster->getPendingCCSRequests().begin();
        while (iter != gameSession->mGameDataMaster->getPendingCCSRequests().end())
        {
            CCSRequestPair* requestPair = (*iter);
            if ((gameSession->mPlayerConnectionMesh.getConnectionDetails(requestPair->getConsoleFirstConnGrpId(), requestPair->getConsoleSecondConnGrpId()) == nullptr)
                || (gameSession->mPlayerConnectionMesh.getConnectionDetails(requestPair->getConsoleSecondConnGrpId(), requestPair->getConsoleFirstConnGrpId()) == nullptr))
            {
                TRACE_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). Erase pending hosted connectivity request for pair ("<<requestPair->getConsoleFirstConnGrpId()<<","<<requestPair->getConsoleSecondConnGrpId()
                    <<") as at least one of them has been removed from the game.");
                iter = gameSession->mGameDataMaster->getPendingCCSRequests().erase(iter);
            }
            else if (gameSession->mPlayerConnectionMesh.getHostedConnectivityInfo(requestPair->getConsoleFirstConnGrpId(),requestPair->getConsoleSecondConnGrpId()) != nullptr)
            {
                ERR_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). Erase pending hosted connectivity request for pair ("<<requestPair->getConsoleFirstConnGrpId()<<","<<requestPair->getConsoleSecondConnGrpId()
                    <<") as they already have it. This should not happen.");
                iter = gameSession->mGameDataMaster->getPendingCCSRequests().erase(iter);
            }
            else
            {
                bool receivedFirst = gameSession->mPlayerConnectionMesh.getCCSTriggerWasReceived(requestPair->getConsoleFirstConnGrpId(), requestPair->getConsoleSecondConnGrpId());
                bool receivedSec = gameSession->mPlayerConnectionMesh.getCCSTriggerWasReceived(requestPair->getConsoleSecondConnGrpId(), requestPair->getConsoleFirstConnGrpId());
                if (receivedFirst && receivedSec)
                {
                    requestPair->copyInto(*(ccsRequest.getCCSRequestPairs().pull_back()));
                    iter = gameSession->mGameDataMaster->getPendingCCSRequests().erase(iter);
                }
                else
                {
                    if (receivedFirst || receivedSec)
                    {
                        if (requestPair->getRequestPairCreationTime() == 0)
                        {
                            requestPair->setRequestPairCreationTime(TimeValue::getTimeOfDay().getMicroSeconds());
                        }
                        else if (TimeValue::getTimeOfDay().getMicroSeconds() - requestPair->getRequestPairCreationTime().getMicroSeconds() >= gGameManagerMaster->getCCSBidirectionalConnLostUpdateTimeout())
                        {
                            TRACE_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). Timeout when waiting for pair (" << requestPair->getConsoleFirstConnGrpId() 
                                << "," << requestPair->getConsoleSecondConnGrpId() << " to arrive. CCS trigger from " 
                                << ((receivedFirst) ? requestPair->getConsoleSecondConnGrpId() : requestPair->getConsoleFirstConnGrpId()) << " was not received");

                            if (gameSession->updatePlayersConnLostStatus(gameId, requestPair) == MeshEndpointsConnectionLostMasterError::Error::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE)
                                return;
                        }
                    }
                    ++iter;
                }
            }
        }

        if (!ccsRequest.getCCSRequestPairs().empty()) 
        {
            ccsRequest.setGameId(gameId);

            CCSUtil ccsUtil;
            BlazeRpcError error = ccsUtil.buildGetHostedConnectionRequest(*gameSession, ccsRequest);
            if (error == Blaze::ERR_OK)
            {
                Blaze::GameManager::GameManagerSlave* gmgrSlave = static_cast<Blaze::GameManager::GameManagerSlave*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, false, true, &error));
                if (gmgrSlave != nullptr)
                {
                    TRACE_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). Found waiting pairs. Issue CCS request to the slave.");
                    gameSession->mGameDataMaster->setNumInflightCCSRequests(gameSession->mGameDataMaster->getNumInflightCCSRequests() + 1);
                    error = gmgrSlave->requestConnectivityViaCCS(ccsRequest); 
                    if (error != Blaze::ERR_OK) 
                    {
                        // The slave rpc failed. We are not going to get ccsConnectivityResultsAvailable so we adjust our in flight request count. We intentionally incremented the count earlier rather than checking the error 
                        // code first in order to avoid any potential race conditions(even though possibility is infinitely small)
                        gameSession->mGameDataMaster->setNumInflightCCSRequests(gameSession->mGameDataMaster->getNumInflightCCSRequests() - 1);
                        ERR_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). requestConnectivityViaCCS on slave failed. Error("<<ErrorHelp::getErrorName(error)<<"). Request: "<< ccsRequest);
                    }
                    TRACE_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). number of ccs requests in flight "<<gameSession->mGameDataMaster->getNumInflightCCSRequests());
                }
            }
            else
            {
                ERR_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << ").  failed to build hosted connection request. Error("<<ErrorHelp::getErrorName(error)<<")");
            }
        }

        if (!gameSession->mGameDataMaster->getPendingCCSRequests().empty())
        {            
            TRACE_LOG("[GameSessionMaster].ccsRequestIssueTimerFired: game(" << gameId << "). reschedule timer for later as we still have " << gameSession->mGameDataMaster->getPendingCCSRequests().size() << " pending requests that are not eligible to be fullfilled at this time.");
            gameSession->scheduleCCSRequestTimer();
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Cancel the current lease timer and set it to fire off after (leastTimeMinutes).
    (leastTimeMinutes) should be the value returned from DirtyCast, but we'll only use an arbitrary fraction of that time
    as the renewal interval.  This difference serves as a grace period to mitigate race conditions due to network delays.
    ***************************************************************************************************/
    void GameSessionMaster::scheduleCCSExtendLeaseTimer(uint16_t leaseTimeMinutes)
    {
        gGameManagerMaster->getCCSExtendLeaseTimerSet().cancelTimer(getGameId());

        uint16_t renewalIntervalMinutes = leaseTimeMinutes * 2 / 3;

        if (renewalIntervalMinutes < 1)
        {
            WARN_LOG("[GameSessionMaster].scheduleCCSExtendLeaseTimer: game(" << getGameId() << "). Renewal interval is too small (based on lease time of " << leaseTimeMinutes << "m). Bumping it to 1 minute.");
            renewalIntervalMinutes = 1;
        }

        TimeValue fireTimeInterval;
        fireTimeInterval.setSeconds(renewalIntervalMinutes * 60);
        TimeValue timeToFire = TimeValue::getTimeOfDay() + fireTimeInterval;
        if (gGameManagerMaster->getCCSExtendLeaseTimerSet().scheduleTimer(getGameId(), timeToFire))
        {
            mGameDataMaster->setCCSExtendLeaseTime(timeToFire);
        }
        else
        {
            ERR_LOG("[GameSessionMaster].scheduleCCSExtendLeaseTimer: game("<< getGameId() <<"). failed to schedule CCS lease extension.");
        }
    }
    
    /*! ************************************************************************************************/
    /*! \brief Time to extend our lease if the game session still needs it. If not, terminate the lease. 
    ***************************************************************************************************/
    void GameSessionMaster::ccsExtendLeaseTimerFired(GameId gameId)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(gameId);
        if (gameSession == nullptr)
            return;
        
        TRACE_LOG("[GameSessionMaster].ccsExtendLeaseTimerFired: game(" << gameId << "). ccs extend lease timer fired.");
        if (gameSession->mGameDataMaster->getNumHostedConnections() > 0 || !gameSession->mGameDataMaster->getPendingCCSRequests().empty())
        {
            TRACE_LOG("[GameSessionMaster].ccsExtendLeaseTimerFired: game(" << gameId << "). extending lease as we have (" << gameSession->mGameDataMaster->getNumHostedConnections() <<") hosted connection(s)"
                << " and (" << gameSession->mGameDataMaster->getPendingCCSRequests().size() <<") pending allocation requests in the game.");
            
            Blaze::GameManager::CCSLeaseExtensionRequest ccsRequest;
            ccsRequest.setGameId(gameId);

            CCSUtil ccsUtil;
            BlazeRpcError error = ccsUtil.buildLeaseExtensionRequest(*gameSession, ccsRequest);
            if (error == Blaze::ERR_OK)
            {
                Blaze::GameManager::GameManagerSlave* gmgrSlave = static_cast<Blaze::GameManager::GameManagerSlave*>(gController->getComponent(Blaze::GameManager::GameManagerSlave::COMPONENT_ID, false, true, &error));
                if (gmgrSlave != nullptr)
                {
                    error = gmgrSlave->requestLeaseExtensionViaCCS(ccsRequest); 
                    if (error == Blaze::ERR_OK) 
                    {
                        return;
                    }
                    else
                    {
                        ERR_LOG("[GameSessionMaster].ccsExtendLeaseTimerFired: game(" << gameId << "). requestLeaseExtensionViaCCS on slave failed. Error("<<ErrorHelp::getErrorName(error)<<"). Request: "<< ccsRequest);
                    }
                }
                else
                {
                    ERR_LOG("[GameSessionMaster].ccsExtendLeaseTimerFired: game(" << gameId << "). Could not get a slave component.");
                }
            }
            else
            {
                ERR_LOG("[GameSessionMaster].ccsExtendLeaseTimerFired: game(" << gameId << ").  failed to build lease extension request. Error("<<ErrorHelp::getErrorName(error)<<")");
            }
        }

        // In case we no longer require CCS assistance or failed to extend the lease, clean up the state on our end.
        gameSession->cleanupCCS(); 
    }
    /*! ************************************************************************************************/
    /*! \brief processes the results of ccs connectivity request and issues the notifications as appropriate.
    ***************************************************************************************************/
    CcsConnectivityResultsAvailableError::Error GameSessionMaster::ccsConnectivityResultsAvailable(const CCSAllocateRequestMaster& request)
    {
        if (request.getSuccess())
        {
            TRACE_LOG("[GameSessionMaster].ccsConnectivityResultsAvailable: game(" << getGameId() << "). ccs connectivity request succeeded at ccs service level for request id(" << request.getHostedConnectionResponse().getRequestId() << "). " << request);
        }
        else
        {
            ERR_LOG("[GameSessionMaster].ccsConnectivityResultsAvailable: game(" << getGameId() << "). ccs connectivity request failed at ccs service level for request id(" << request.getErr().getRequestId() << "). " << request);
        }
        
        mGameDataMaster->setNumInflightCCSRequests(mGameDataMaster->getNumInflightCCSRequests() - 1);
        
        for (Blaze::GameManager::CCSRequestPairs::const_iterator it = request.getCCSRequestPairs().begin(), itEnd = request.getCCSRequestPairs().end(); it != itEnd; ++it) 
        {
            GameDataMaster::ClientConnectionDetails* console1Details = mPlayerConnectionMesh.getConnectionDetails((*it)->getConsoleFirstConnGrpId(), (*it)->getConsoleSecondConnGrpId());
            GameDataMaster::ClientConnectionDetails* console2Details = mPlayerConnectionMesh.getConnectionDetails((*it)->getConsoleSecondConnGrpId(), (*it)->getConsoleFirstConnGrpId());
            if (console1Details != nullptr && console2Details != nullptr)
            {
                console1Details->getCCSInfo().setCCSRequestInProgress(false);
                console2Details->getCCSInfo().setCCSRequestInProgress(false);
            }
        }

        if (request.getSuccess())
        {
            const Blaze::CCS::GetHostedConnectionResponse& hostedConnResponse = request.getHostedConnectionResponse(); 
            if (request.getCCSRequestPairs().size() == hostedConnResponse.getAllocatedResources().size())
            {
                if (hostedConnResponse.getConnectionSetId() && hostedConnResponse.getConnectionSetId()[0] != '\0')
                {
                    mGameDataMaster->setConnectionSetId(hostedConnResponse.getConnectionSetId());
                }

                Blaze::CCS::AllocatedResources::const_iterator it, itEnd;
                Blaze::GameManager::CCSRequestPairs::const_iterator it2;
                for (it = hostedConnResponse.getAllocatedResources().begin(), itEnd = hostedConnResponse.getAllocatedResources().end(), 
                    it2 = request.getCCSRequestPairs().begin(); 
                    it != itEnd; ++it, ++it2)
                {
                    const CCS::ConnIdMap* connectivityIds = *it;
                    if (connectivityIds != nullptr)
                    {
                        // We increment the count beforehand as in case we end up freeing the connectivity, we'll be decrementing the count.
                        mGameDataMaster->setNumHostedConnections(mGameDataMaster->getNumHostedConnections() + 1);

                        CCS::ConnIdMap::const_iterator gameConsoleId1Iter = connectivityIds->find("game_console_id_1");
                        CCS::ConnIdMap::const_iterator gameConsoleId2Iter = connectivityIds->find("game_console_id_2");
                        if (gameConsoleId1Iter == connectivityIds->end() || gameConsoleId2Iter == connectivityIds->end())
                        {
                            ERR_LOG("[GameSessionMaster].ccsConnectivityResultsAvailable: Missing connectivityIds for game_console_id_1 or game_console_id_2.  Unable to update.");
                            continue;
                        }

                        uint32_t gameConsoleId1 = gameConsoleId1Iter->second;
                        uint32_t gameConsoleId2 = gameConsoleId2Iter->second;

                        // By the time, CCS results came back, in case the endpoint(s) have been removed for any reason (timeout, logout), we can't update the player connection mesh as 
                        // the mapping is already gone. In that case, simply free up the connectivity. 
                        if ((mPlayerConnectionMesh.getConnectionDetails((*it2)->getConsoleFirstConnGrpId(), (*it2)->getConsoleSecondConnGrpId()) == nullptr)
                            || (mPlayerConnectionMesh.getConnectionDetails((*it2)->getConsoleSecondConnGrpId(), (*it2)->getConsoleFirstConnGrpId()) == nullptr))
                        {
                            TRACE_LOG("[GameSessionMaster].ccsConnectivityResultsAvailable: game("<< getGameId() <<"). free hosted connectivity as at least one of the console is no longer in the game."
                                <<"console pair("<< (*it2)->getConsoleFirstConnGrpId() << "," << (*it2)->getConsoleSecondConnGrpId() <<")");
                            mPlayerConnectionMesh.freeHostedConnectivity((*it2)->getConsoleFirstConnGrpId(), (*it2)->getConsoleSecondConnGrpId(), gameConsoleId1, gameConsoleId2);
                        }   
                        else
                        {
                            eastl::string ipAddrStr(eastl::string::CtorSprintf(),"%s:%s", hostedConnResponse.getHostingServerIP(), hostedConnResponse.getHostingServerPort());
                            InetAddress ipAddr(ipAddrStr.c_str());

                            HostedConnectivityInfo info;
                            info.setLocalLowLevelConnectivityId(gameConsoleId1);
                            info.setRemoteLowLevelConnectivityId(gameConsoleId2);
                            info.getHostingServerNetworkAddress().switchActiveMember(NetworkAddress::MEMBER_IPADDRESS);
                            info.getHostingServerNetworkAddress().getIpAddress()->setIp(ipAddr.getIp(InetAddress::HOST));
                            info.getHostingServerNetworkAddress().getIpAddress()->setPort(ipAddr.getPort(InetAddress::HOST));
                            info.setHostingServerConnectivityId(hostedConnResponse.getHostingServerConnectivityId());

                            if (!hostedConnResponse.getConnectionSetIdAsTdfString().empty())
                                info.setHostingServerConnectionSetId(hostedConnResponse.getConnectionSetId());
                            else if (request.getRequestOut().getRequest().getV2Request())
                                info.setHostingServerConnectionSetId(request.getRequestOut().getRequest().getV2Request()->getConnectionSetId());
                            else if (request.getRequestOut().getRequest().getV1Request())
                                info.setHostingServerConnectionSetId(request.getRequestOut().getRequest().getV1Request()->getConnectionSetId());
                            ASSERT_COND_LOG(!info.getHostingServerConnectionSetIdAsTdfString().empty(), "No connection set id found from CCS request nor its response for HostedConnectivityInfo");

                            mPlayerConnectionMesh.setHostedConnectivityInfo((*it2)->getConsoleFirstConnGrpId(), (*it2)->getConsoleSecondConnGrpId(), info);
                            notifyHostedConnectivityAvailable((*it2)->getConsoleFirstConnGrpId(), (*it2)->getConsoleSecondConnGrpId(), info);
                        }
                    }
                }

                mCcsPingSite = hostedConnResponse.getDataCenter();
            }
            else
            {
                ERR_LOG("[GameSessionMaster].ccsConnectivityResultsAvailable: game("<< getGameId() <<"). mismatch in request and response for request id("<<request.getHostedConnectionResponse().getRequestId()<<"). Skipping the results processing.");
            }
        }
        else
        {
            // fake the connectionLost message that we skipped earlier as we tried to get the connectivity from CCS. This is to allow for a quick join timeout. 
            for (Blaze::GameManager::CCSRequestPairs::const_iterator it = request.getCCSRequestPairs().begin(), itEnd = request.getCCSRequestPairs().end(); it != itEnd; ++it)
            {
                UpdateMeshConnectionMasterRequest req;
                if (updatePlayersConnLostStatus(request.getGameId(), (*it)) == MeshEndpointsConnectionLostMasterError::Error::GAMEMANAGER_ERR_GAME_DESTROYED_BY_CONNECTION_UPDATE)
                    return CcsConnectivityResultsAvailableError::ERR_OK;

                
                if (getGameNetworkTopology() == NETWORK_DISABLED)
                {
                    // Disabled topology doesn't care about the connection lost above (doesn't even tell the clients), 
                    // so we need to send that information down here, so they'll complete whatever delayed actions (join) they might have. 
                    HostedConnectivityInfo info;
                    notifyHostedConnectivityAvailable((*it)->getConsoleFirstConnGrpId(), (*it)->getConsoleSecondConnGrpId(), info);
                }
            }
        }

        TRACE_LOG("[GameSessionMaster].ccsConnectivityResultsAvailable: game("<< getGameId() <<"). Number of hosted connections in the game(" << mGameDataMaster->getNumHostedConnections() << ")");
        
        if (mGameDataMaster->getNumHostedConnections() > 0 || !mGameDataMaster->getPendingCCSRequests().empty())
        {
            // Schedule lease extension if the request is successful. 
            // On successful addition of connections on an existing connection set, CCS renews the lease automatically and following call reschedules our lease extension
            // request timer. 
            // If the request is not successful (say some sort of unexpected error), let the lease extension kick in as previously scheduled (if applicable). 
            if (request.getSuccess()) 
            {
                scheduleCCSExtendLeaseTimer(request.getHostedConnectionResponse().getLeasetime());
            }
        }
        else
        {
            cleanupCCS();
        }
        return CcsConnectivityResultsAvailableError::ERR_OK;
    }
    
    /*! ************************************************************************************************/
    /*! \brief processes the results of ccs lease extension request and issues the notifications as appropriate.
    ***************************************************************************************************/
    CcsLeaseExtensionResultsAvailableError::Error GameSessionMaster::ccsLeaseExtensionResultsAvailable(const CCSLeaseExtensionRequestMaster& request)
    {
        if (request.getSuccess())
        {
            TRACE_LOG("[GameSessionMaster].ccsLeaseExtensionResultsAvailable: game(" << request.getGameId() << "). ccs lease extension succeeded at ccs service level for request id(" << request.getLeaseExtensionResponse().getRequestId() << "). " << request);
            scheduleCCSExtendLeaseTimer(request.getLeaseExtensionResponse().getLeasetime());
        }
        else
        {
            ERR_LOG("[GameSessionMaster].ccsLeaseExtensionResultsAvailable: game(" << request.getGameId() << ").  failed to extend lease. Details: " << request);
            cleanupCCS();
            return CcsLeaseExtensionResultsAvailableError::ERR_SYSTEM;
        }

        return CcsLeaseExtensionResultsAvailableError::ERR_OK;
    }
    
    /*! ************************************************************************************************/
    /*! \brief returns true if this game is set to leverage CCS for making connections
    ***************************************************************************************************/
    bool GameSessionMaster::doesLeverageCCS() const
    {
        if ((getCCSMode() > CCS_MODE_PEERONLY) && !isDedicatedHostedTopology(getGameNetworkTopology())
            && !((getGameNetworkTopology() == NETWORK_DISABLED) && (getVoipNetwork() == VOIP_DISABLED)))
        {
            return true;
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief add an account to the game's banned account list.  If the list is full, remove the oldest
                banned account.
    ***************************************************************************************************/
    void GameSessionMaster::banAccount(AccountId bannedAccount)
    {
        // ban list full and ban new account?
        if (mGameDataMaster->getBannedAccountMap().size() >= gGameManagerMaster->getBanListMax() &&
            mGameDataMaster->getBannedAccountMap().find(bannedAccount) == mGameDataMaster->getBannedAccountMap().end())
        {
            mGameDataMaster->getBannedAccountMap().erase(mGameDataMaster->getOldestBannedAccount());
            updateOldestBannedAccount();
        }

        // add it to the ban list
        if (mGameDataMaster->getBannedAccountMap().empty())
        {
            //ban list empty? need to list new oldest.
            mGameDataMaster->getBannedAccountMap()[bannedAccount] = TimeValue::getTimeOfDay();
            updateOldestBannedAccount();
        }
        else
        {
            // we can update existing ban with new ban time, or add a new one
            mGameDataMaster->getBannedAccountMap()[bannedAccount] = TimeValue::getTimeOfDay();
            if (bannedAccount == mGameDataMaster->getOldestBannedAccount())
            {
                //need to find new oldest banned player, since the oldest had his time updated
                updateOldestBannedAccount();
            }

        }
    }

    /*! ************************************************************************************************/
    /*! \brief return true if an account has been banned from this game.
    ***************************************************************************************************/
    bool GameSessionMaster::isAccountBanned(AccountId bannedAccount) const
    {
        return mGameDataMaster->getBannedAccountMap().find(bannedAccount) != mGameDataMaster->getBannedAccountMap().end();
    }

    void GameSessionMaster::updateOldestBannedAccount()
    {
        if (mGameDataMaster->getBannedAccountMap().empty())
        {
            mGameDataMaster->setOldestBannedAccount(INVALID_ACCOUNT_ID);
            return;
        }

        TimeValue oldestTime = TimeValue::getTimeOfDay();

        // Since change from list to map, entries in the map are not ordered by time.
        // Find the next oldest account
        GameDataMaster::BannedAccountMap::iterator bannedListIter = mGameDataMaster->getBannedAccountMap().begin();
        GameDataMaster::BannedAccountMap::iterator bannedListEnd = mGameDataMaster->getBannedAccountMap().end();
        AccountId oldestBannedAccountId = INVALID_ACCOUNT_ID;
        for(; bannedListIter != bannedListEnd; ++bannedListIter)
        {
            if (bannedListIter->second < oldestTime)
            {
                oldestBannedAccountId = bannedListIter->first;
                oldestTime = bannedListIter->second;
            }
        }

        if (oldestBannedAccountId == INVALID_ACCOUNT_ID && !mGameDataMaster->getBannedAccountMap().empty())
        {
            //  this would likely happen only if all banned accounts in the map were added at the current time of day.
            //  if so, then just pick the first one in the map since there's no way to know which account was added first.
            mGameDataMaster->setOldestBannedAccount(mGameDataMaster->getBannedAccountMap().begin()->first);
        }
        else
            mGameDataMaster->setOldestBannedAccount(oldestBannedAccountId);
    }

    /*! ************************************************************************************************/
    /*! \brief return true if this game allows players to join with the supplied joinMethod

        \param[in] joinMethod - the joining method to check.
        \return true if the game allows joins using the supplied joinMethod.
    ***************************************************************************************************/
    bool GameSessionMaster::isSupportedJoinMethod(const JoinMethod joinMethod, const JoinGameMasterRequest & req) const
    {
        // Spectator joins can skip the later join restrictions if this is set:
        if (getGameSettings().getSpectatorBypassClosedToJoin())
        {
            bool allJoiningAsSpectators = true;
            auto defaultSlotType = req.getJoinRequest().getPlayerJoinData().getDefaultSlotType();
            if (defaultSlotType != SLOT_PUBLIC_SPECTATOR && defaultSlotType != SLOT_PRIVATE_SPECTATOR)
                allJoiningAsSpectators = false;
            for (auto& curPlayer : req.getJoinRequest().getPlayerJoinData().getPlayerDataList())
            {
                auto playerSlotType = curPlayer->getSlotType();
                if (playerSlotType != INVALID_SLOT_TYPE && playerSlotType != SLOT_PUBLIC_SPECTATOR && playerSlotType != SLOT_PRIVATE_SPECTATOR)
                    allJoiningAsSpectators = false;
            }

            if (allJoiningAsSpectators)
                return true;
        }

        switch(joinMethod)
        {
        case JOIN_BY_BROWSING:
            return (!isLockedForPreferredJoins() && getGameSettings().getOpenToBrowsing());
        case JOIN_BY_INVITES:
            // NOTE:  The adminOnlyInvites still requires the openToInvites game setting to be set.
            return getGameSettings().getOpenToInvites() && (!getGameSettings().getAdminOnlyInvites() || hasPermission(req.getJoinRequest().getUser().getBlazeId(), GameAction::ALLOW_JOIN_BY_INVITE));
        case JOIN_BY_MATCHMAKING:
            return (!isLockedForPreferredJoins() && getGameSettings().getOpenToMatchmaking());
        case JOIN_BY_PERSISTED_GAME_ID:
            return getGameSettings().getEnablePersistedGameId();
        case JOIN_BY_PLAYER:

            if (!getGameSettings().getOpenToJoinByPlayer() && getGameSettings().getFriendsBypassClosedToJoinByPlayer())
            {
                ExternalSessionUtil* util = getExternalSessionUtilManager().getUtil(req.getUserJoinInfo().getUser().getUserInfo().getPlatformInfo().getClientPlatform());
                return util ? util->isFriendsOnlyRestrictionsChecked() : false;//actual 1st party friendship is checked elsewhere
            }

            return getGameSettings().getOpenToJoinByPlayer();
        case SYS_JOIN_BY_FOLLOWLEADER_CREATEGAME:
        case SYS_JOIN_BY_RESETDEDICATEDSERVER:
        case SYS_JOIN_BY_FOLLOWLEADER_RESETDEDICATEDSERVER:
        case SYS_JOIN_BY_FOLLOWLEADER_RESERVEDEXTERNALPLAYER:
        case SYS_JOIN_BY_FOLLOWLOCALUSER:
            return true;  // There are no join controls for these as they are system functions to track GEC.  They all are allowed.
        case SYS_JOIN_BY_CREATE:
        case SYS_JOIN_TYPE_INVALID:
            return false;
        default:
            // Unknown joinMethod
            ERR_LOG("[GameSessionMaster] Unknown join method sent from client. " << JoinMethodToString(joinMethod) << "(" << joinMethod << ") ");
        }

        return false;
    }

    /*!************************************************************************************************/
    /*! \brief changes the state of the player & send out a replication update for the player.
                WARNING - this could modify a roster iterator (players are cached by player state in the roster)!

        \param[in] pInfo updated information for the player.
        \param[in] newstate new state of the player,
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::changePlayerState(PlayerInfoMaster* pInfo, const PlayerState newstate)
    {
        // Not allowed to directly change a player from queued or reserved states.
        // Those are handled internally.
        if ( EA_UNLIKELY(!pInfo->isActive()) )
        {
            ERR_LOG("[GameSessionMaster] cannot change player state from QUEUED or RESERVED without being promoted or claiming the reservation.");
            return GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
        }
        return mPlayerRoster.updatePlayerState(pInfo, newstate);
    }

    /*! ************************************************************************************************/
    /*! \brief set this game's np session information.
    ***************************************************************************************************/
    void GameSessionMaster::setNpSessionInfo(BlazeId blazeId, const NpSessionId &npSessionid)
    {
        // This method is now only for backward compatible clients, when server driven external sessions is disabled
        return;
    }

    void GameSessionMaster::setNpSessionId(const char8_t* value)
    {
        TRACE_LOG("[GameSessionMaster].setNpSessionId: updating game(" << getGameId() << ", " << GameTypeToString(getGameType()) << ")'s NP session id from (" << getNpSessionId() << ") to (" << ((value != nullptr)? value : "<empty>") << ").");
        getExternalSessionIdentification().getPs4().setNpSessionId(value);
        newToOldReplicatedExternalSessionData(getGameData());//for back compat
    }

    /*! ************************************************************************************************/
    /*! \brief reset this game with the supplied game params/settings; makes player reservations but does not
            join players to the game.  joiningSession is added as a game admin.
        \param[in] persistedId persisted id to set on the game. Note: when reseting its the caller's responsibility
            to validate the persisted id. It is not validated by this function.
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::resetDedicatedServer(const CreateGameMasterRequest& masterRequest, const UserSessionSetupReasonMap &setupReasons,
        const PersistedGameId& persistedId, const PersistedGameIdSecret& persistedIdSecret)
    {
        const CreateGameRequest& createGameRequest = masterRequest.getCreateRequest();

        if (!hasDedicatedServerHost())
        {
            WARN_LOG("[GameSessionMaster] The game(" << getGameId() << ") is not a dedicated server to reset.");
            return GAMEMANAGER_ERR_DEDICATED_SERVER_ONLY_ACTION;
        }

        if (!isResetable())
        {
            WARN_LOG("[GameSessionMaster] The game(" << getGameId() << ") is not in a restable state.");
            return GAMEMANAGER_ERR_INVALID_GAME_STATE_TRANSITION;
        }

        // check if the uuid matches the secret if
        // the gamesetting's flag is true and uuid in the request is not empty
        getGameData().getPersistedGameIdSecret().setData(persistedIdSecret.getData(), persistedIdSecret.getCount());
        getGameData().setPersistedGameId(persistedId.c_str());

        const UserSessionInfo* hostSessionInfo = getHostSessionInfo(masterRequest.getUsersInfo());
        if (EA_UNLIKELY(hostSessionInfo == nullptr))
        {
            WARN_LOG("[GameSessionMaster] Missing host player in attempt to reset game(" << getGameId() << ").");
            return GAMEMANAGER_ERR_MISSING_PRIMARY_LOCAL_PLAYER;
        }

        if (getGameReportingId() == GameManager::INVALID_GAME_REPORTING_ID)
        {
            GameReportingId grId = gGameManagerMaster->getNextGameReportingId();
            // verify GR id generation worked
            if (grId == GameManager::INVALID_GAME_REPORTING_ID)
            {
                ERR_LOG("[GameSessionMaster] Resettable game(" << getGameId() << ") couldn't generate a GameReportingId.");
                return ERR_SYSTEM;
            }
            
            setGameReportingId(grId);
            
            NotifyGameReportingIdChange nGameReportingIdChange;
            nGameReportingIdChange.setGameId(getGameId());
            nGameReportingIdChange.setGameReportingId(getGameReportingId());
            GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
            gGameManagerMaster->sendNotifyGameReportingIdChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nGameReportingIdChange);
        }
        

        TRACE_LOG("[GameSessionMaster] Resetting dedicated server game(" << getGameId() << ").");

        // Rename only if a new name was specified, otherwise use existing
        const char8_t* gameName = createGameRequest.getGameCreationData().getGameName();
        if (gameName[0] != '\0')
        {
            getGameData().setGameName(gameName);
        }

        // clear any game groups from previous games (just in case they did not call return previously)
        mGameData->getSingleGroupMatchIds().clear();
        // Reserve single group join
        if (createGameRequest.getGameCreationData().getGameSettings().getEnforceSingleGroupJoin())
        {
            mGameData->getSingleGroupMatchIds().reserve(2);
        }

        BlazeRpcError err = initGameDataCommon(masterRequest, false);
        if (EA_UNLIKELY(err != ERR_OK))
        {
            return err;
        }

        // re-mark the dedicated server as occupying a slot
        markSlotOccupied(getGameData().getDedicatedServerHostInfo().getSlotId());
        // Assumes dedicated host does not have any MLU members
        markConnectionSlotOccupied(getGameData().getDedicatedServerHostInfo().getConnectionSlotId(),
            getGameData().getDedicatedServerHostInfo().getConnectionGroupId());


        // set game state and add admin player, the player who resets the dedicated server
        // game becomes an admin
        setGameState(INITIALIZING);

        if (getGamePermissionSystem().mEnableAutomaticAdminSelection)
        {
            const PlayerId noPlayer = 0;
            addAdminPlayer(hostSessionInfo->getUserInfo().getId(), noPlayer);
        }

        getGameData().setSharedSeed((uint32_t)Random::getRandomNumber(RAND_MAX)^(uint32_t)TimeValue::getTimeOfDay().getSec());

        // set up QoS validation list
        if (masterRequest.getFinalizingMatchmakingSessionId() != INVALID_MATCHMAKING_SESSION_ID)
        {
            addMatchmakingQosData(masterRequest.getUsersInfo(), masterRequest.getFinalizingMatchmakingSessionId());
        }

        // send the new game settings to the dedicated server (not necessary to send to active players)
        NotifyGameReset notifyGameReset;
        notifyGameReset.setGameData(getGameData());
        gGameManagerMaster->sendNotifyGameResetToUserSessionById(getDedicatedServerHostSessionId(), &notifyGameReset);

        // Note: Sending notifications to active players is unnecessary.  Game is empty during reset,
        // only possible to be reserved. On the client we assert if there are active players in the game.
        gGameManagerMaster->addGameToCensusCache(*this);

        mMetricsHandler.onDedicatedGameHostReset();

        StringBuilder buf;
        buf <<  "Resetting User Blaze Id: (" << hostSessionInfo->getUserInfo().getId() << ")";
        gGameManagerMaster->logDedicatedServerEvent("game reset", *this, buf.get());

        // submit game session created event
        CreatedGameSession gameSessionEvent;
        gameSessionEvent.setGameId(getGameId());
        gameSessionEvent.setGameReportingId(getGameReportingId());
        gameSessionEvent.setGameName(createGameRequest.getGameCreationData().getGameName());
        gameSessionEvent.setCreationReason(GameManager::GameUpdateReasonToString(GameManager::GAME_RESET));
        gameSessionEvent.setNetworkTopology(GameNetworkTopologyToString(getGameNetworkTopology()));
        gameSessionEvent.getTopologyHostInfo().setPlayerId(getTopologyHostInfo().getPlayerId());
        gameSessionEvent.getTopologyHostInfo().setSlotId(getTopologyHostInfo().getSlotId());
        gameSessionEvent.getPlatformHostInfo().setPlayerId(getPlatformHostInfo().getPlayerId());
        gameSessionEvent.getPlatformHostInfo().setSlotId(getPlatformHostInfo().getSlotId());
        getTopologyHostNetworkAddressList().copyInto(gameSessionEvent.getHostAddressList());
        gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_CREATEDGAMESESSIONEVENT), gameSessionEvent, true);



        return Blaze::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Assigns a player to the first open endpoint slot

        \param[in,out] player PlayerInfoMaster object to be assigned to a slot
    ***************************************************************************************************/
    void GameSessionMaster::assignPlayerToFirstOpenSlot(PlayerInfoMaster& player)
    {
        if (player.getSlotId() == DEFAULT_JOINING_SLOT)
        {
            EndpointSlotBitset& bits = asEndpointSlotBitset(mGameDataMaster->getOpenSlotIdList());
            size_t index = bits.find_first();
            if (index < bits.size())
            {
                SlotId slotId = (SlotId) index;
                markSlotOccupied(slotId);
                player.getPlayerData()->setSlotId(slotId);
            }
        }
        else
        {
            markSlotOccupied(player.getSlotId());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Frees an endpoint slot
        \param[in] slotId The slot to mark as free
    ***************************************************************************************************/
    void GameSessionMaster::markSlotOpen(SlotId slotId)
    {
        //set bits indicate open slots
        EndpointSlotBitset& bits = asEndpointSlotBitset(mGameDataMaster->getOpenSlotIdList());
        bits.set(slotId, true); // 1 is open
    }

    /*! ************************************************************************************************/
    /*! \brief Marks a slot as occupied
        \param[in] slotId The slot to be marked as occupied
    ***************************************************************************************************/
    void GameSessionMaster::markSlotOccupied(SlotId slotId)
    {
        //reset bits indicate slots in use
        EndpointSlotBitset& bits = asEndpointSlotBitset(mGameDataMaster->getOpenSlotIdList());
        bits.set(slotId, false); // 0 is occupied
    }

    /*! ************************************************************************************************/
    /*! \brief Assigns a players connection to the first open network slot.  If the players connection
        group already has a slot, just returns.

        \param[in,out] player PlayerInfoMaster object of the player being assigned a slot.
    ***************************************************************************************************/
    void GameSessionMaster::assignConnectionToFirstOpenSlot(PlayerInfoMaster& player)
    {
        GameDataMaster::ConnectionSlotMap::const_iterator iter = mGameDataMaster->getConnectionSlotMap().find(player.getConnectionGroupId());
        if (iter != mGameDataMaster->getConnectionSlotMap().end())
        {
            SlotId curId = iter->second;
            player.getPlayerData()->setConnectionSlotId(curId);
            return;
        }

        EndpointSlotBitset& bits = asEndpointSlotBitset(mGameDataMaster->getConnectionSlotIdList());
        size_t index = bits.find_first();
        if (index < bits.size())
        {
            SlotId slotId = (SlotId) index;
            player.getPlayerData()->setConnectionSlotId(slotId);
            markConnectionSlotOccupied(slotId, player.getConnectionGroupId());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Open up the given slot id.

        \param[in] slotId - the slot id to mark open
        \param[in] connGroupId - the conneciton group id in the provided slot.
    ***************************************************************************************************/
    void GameSessionMaster::markConnectionSlotOpen(SlotId slotId, ConnectionGroupId connGroupId)
    {
        EndpointSlotBitset& bits = asEndpointSlotBitset(mGameDataMaster->getConnectionSlotIdList());
        bits.set(slotId, true); // 1 means free
        if (connGroupId != 0)
            mGameDataMaster->getConnectionSlotMap().erase(connGroupId);
    }

    /*! ************************************************************************************************/
    /*! \brief Reserver the give slotid for the give connection group id.
        NOTE: that this does not check to see if the connection group already has a slot in the map.

        \param[in] slotId - the slot id to mark occupied.
        \param[in] connGroupId - the connection group occupying the slot.
    ***************************************************************************************************/
    void GameSessionMaster::markConnectionSlotOccupied(SlotId slotId, ConnectionGroupId connGroupId)
    {
        EndpointSlotBitset& bits = asEndpointSlotBitset(mGameDataMaster->getConnectionSlotIdList());
        bits.set(slotId, false); // 0 means occupied
        if (connGroupId != 0)
            mGameDataMaster->getConnectionSlotMap()[connGroupId] = slotId;
    }

    /*! ************************************************************************************************/
    /*! \brief select & return a new platform or topology host.  Topology hosts are sorted by NAT type & upstream
                bandwidth; platform hosts are not.

        \param[in] migrationType - are we selecting a new platform host or topology host or both
        \param[in] playerRemovedReason - The reason player was removed. INVALID if the host migration action is explicit and not due to player removal 
        \return the newly selected host (or nullptr)
    ***************************************************************************************************/
    PlayerInfoMaster* GameSessionMaster::selectNewHost(HostMigrationType migrationType, PlayerRemovedReason playerRemovedReason)
    {
        TRACE_LOG("GameSessionMaster::selectNewHost. Migration type - "<< migrationType <<". Player removed reason - " << playerRemovedReason 
            << ". Current Topology host player - " << getReplicatedGameSession().getTopologyHostInfo().getPlayerId() 
            << ". Current platform host player - "<< getReplicatedGameSession().getPlatformHostInfo().getPlayerId()
            << ". Current topology host player conngroup - "<< getReplicatedGameSession().getTopologyHostInfo().getConnectionGroupId()
            << ". Current platform host player conngroup - "<< getReplicatedGameSession().getPlatformHostInfo().getConnectionGroupId()
            );

        const ConnectionGroupId currentTopologyHostConnGroupId = getReplicatedGameSession().getTopologyHostInfo().getConnectionGroupId();
        const ConnectionGroupId currentPlatformHostConnGroupId = getReplicatedGameSession().getPlatformHostInfo().getConnectionGroupId();

        const bool topologyHostMigration = (migrationType == TOPOLOGY_HOST_MIGRATION || migrationType == TOPOLOGY_PLATFORM_HOST_MIGRATION);
        const bool platformHostMigration = (migrationType == PLATFORM_HOST_MIGRATION || migrationType == TOPOLOGY_PLATFORM_HOST_MIGRATION);

        PlayerInfoMaster* candidate = nullptr;

        // For network disabled (e.g. game groups), to simplify things for titles, pick the same longest-in player as admin migration would.
        if (getGameNetworkTopology() == NETWORK_DISABLED)
        {
            BlazeId previousHostId = (topologyHostMigration ? getReplicatedGameSession().getTopologyHostInfo().getPlayerId() : getReplicatedGameSession().getPlatformHostInfo().getPlayerId());
            ConnectionGroupId connectionGroupToSkip = INVALID_CONNECTION_GROUP_ID;
            if (removalKicksConngroup(playerRemovedReason))
            {
                connectionGroupToSkip = (topologyHostMigration ? currentTopologyHostConnGroupId : currentPlatformHostConnGroupId);
            }
            return mPlayerRoster.pickPossHostByTime(previousHostId, connectionGroupToSkip);
        }

        // choose a new host from all players who were fully connected to the game mesh,
        // we intentionally get the ACTIVE_PLAYERS list, because a we need to fix up any users marked as ACTIVE_KICK_PENDING
        // As the new host is one of the players, each one of them(except spectators) is capable to serve as a platform as well as topology host (unlike a dedicated server host who can not serve as a platform host).
        // NOTE: we COPY the active player list, since we (potentially) modify it inside the loop (by changing player's state)
        const PlayerRoster::PlayerInfoList allPlayerList = getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        TRACE_LOG("GameSessionMaster::selectNewHost. playerlist size(" << allPlayerList.size() << ")");
        
        PlayerRoster::PlayerInfoList potentialNewHostList;
        bool skipCurrentConnGroup = removalKicksConngroup(playerRemovedReason);
        if (skipCurrentConnGroup)
        {
            TRACE_LOG("GameSessionMaster::selectNewHost. Skip the current connection group for new host selection");
        }

        for (PlayerRoster::PlayerInfoList::const_iterator it = allPlayerList.begin(), itEnd = allPlayerList.end(); it != itEnd; ++it)
        {
            candidate = static_cast<PlayerInfoMaster*>(*it);
            TRACE_LOG("GameSessionMaster::selectNewHost. checking candidate(" << candidate->getPlayerId() << ") as potential host.");
            if (candidate->getPlayerState() == ACTIVE_CONNECTED || candidate->getPlayerState() == ACTIVE_MIGRATING || candidate->getPlayerState() == ACTIVE_KICK_PENDING)
            {
                if (candidate->getPlayerState() == ACTIVE_KICK_PENDING)
                {
                    // cancel any pending kick timers
                    TRACE_LOG("GameSessionMaster::selectNewHost. set player(" << (*it)->getPlayerId() << ") as migrating, cancel kick");
                    changePlayerState(candidate, ACTIVE_MIGRATING);
                    candidate->cancelPendingKickTimer();
                }

               if (skipCurrentConnGroup && (candidate->getConnectionGroupId() == currentTopologyHostConnGroupId || candidate->getConnectionGroupId() == currentPlatformHostConnGroupId))
                {
                    // Don't choose a candidate as potential host if this connection group is going to be kicked. 
                    TRACE_LOG("GameSessionMaster::selectNewHost. player(" << (*it)->getPlayerId() << ") is a member of previous host connection group,"
                        << " not allowed to become host candidate.");
                    continue;
                }


                if (candidate->hasHostPermission())
                {
                    // spectators can't be selected as the new host, so make sure the potential host is in a player slot
                    potentialNewHostList.push_back(candidate);
                    TRACE_LOG("GameSessionMaster::selectNewHost. candidate(" << candidate->getPlayerId() << ") is a potential host.");
                }
            }
        }

        if (potentialNewHostList.empty())
        {
            TRACE_LOG("GameSessionMaster::selectNewHost. Migration type - " << migrationType << " failed. No host candidates were available.");
            return nullptr;
        }

        TRACE_LOG("GameSessionMaster::selectNewHost. Migration type - " << migrationType << ". choosing from " << potentialNewHostList.size() << " candidates.");

        
        if (topologyHostMigration)
        {
            if (!skipCurrentConnGroup)
            {
                // See if we've got anyone from the same topology host conn group still in the game, as that can save us from having to establish new connections
                if ((candidate = selectNewHost(potentialNewHostList, currentTopologyHostConnGroupId)) != nullptr)
                {
                    TRACE_LOG("GameSessionMaster::selectNewHost. New host chosen from the old topology host conn group.");
                    return candidate;
                }
            }

            TRACE_LOG("GameSessionMaster::selectNewHost. Sort hosts based on the best network choice");
            eastl::sort(potentialNewHostList.begin(), potentialNewHostList.end(), BestNetworkHostComparitor());
            if ((candidate = selectNewHost(potentialNewHostList)) != nullptr)
            {
                TRACE_LOG("GameSessionMaster::selectNewHost. New host chosen based on the best network choice.");
                return candidate;
            }
        }
        
        if (platformHostMigration)
        {
            if (!skipCurrentConnGroup)
            {
                // We check by platform host that belongs to the same connection group
                if ((candidate = selectNewHost(potentialNewHostList, currentPlatformHostConnGroupId)) != nullptr)
                {
                    TRACE_LOG("GameSessionMaster::selectNewHost. New host chosen from the old platform host conn group.");
                    return candidate;
                }
            }
        }
        
        // If the connection groups are no longer around, find a candidate in the list that still has a viable session.
        if ((candidate = selectNewHost(potentialNewHostList)) != nullptr)
        {
            TRACE_LOG("GameSessionMaster::selectNewHost. Old connection groups not found. Choosing another viable alternative.");
            return candidate;
        }

        TRACE_LOG("GameSessionMaster::selectNewHost. Migration type -" << migrationType << " failed. No viable host sessions.");
        return nullptr;
    }
    

    PlayerInfoMaster* GameSessionMaster::selectNewHost(const PlayerRoster::PlayerInfoList& potentialHosts, ConnectionGroupId fromGroupId)
    {
        for (PlayerRoster::PlayerInfoList::const_iterator itr = potentialHosts.begin(), end = potentialHosts.end(); itr != end; ++itr)
        {
            PlayerInfoMaster* candidate = static_cast<PlayerInfoMaster*>(*itr);
            // GM_AUDIT: candidate->getSessionExists()  doesn't seem necessary, but the p4 logs say this was added explicitly to fix a bug when the chosen host has logged out...
            if (candidate->getSessionExists() 
                && (candidate->getConnectionGroupId() == fromGroupId || fromGroupId == INVALID_CONNECTION_GROUP_ID))
            {
                //new host is always considered connected
                changePlayerState(candidate, ACTIVE_CONNECTED);
                return candidate;
            }
        }

        return nullptr;
    }

    /*! \brief return whether host migration is done
    */
    bool GameSessionMaster::isHostMigrationDone() const
    {
        if (mGameDataMaster->getIsNpSessionUnavailable())
        {
            return false;
        }

        //otherwise check whether there are still player waiting to be connected
        return ( (getGameState() == MIGRATING) && (!mPlayerRoster.hasActiveMigratingPlayers()) );
    }

    /*! ************************************************************************************************/
    /*! \brief Starts the host migration process. Gives the game a new host, sets the game state to MIGRATING,
         updates the connection status of every player on the roster in terms on the new host, and schedules a
         timer callback.

        \param[in] nextHost The PlayerInfoMaster object of the host to migrate to or nullptr to indicate that
                            QoS data should be used to determine the next host.
        \param[in] migrationType Indicates whether or not we are only migrating the platform host.
        \param[in] playerRemovedReason - The reason player was removed. INVALID if the host migration action is explicit and not due to player removal 
        \param[in, out] gameDestroyed - Set to true if the game was destroyed. 

        \return true if the host migration process has started
    ***************************************************************************************************/
    bool GameSessionMaster::startHostMigration(PlayerInfoMaster *suggestedHost, HostMigrationType migrationType, PlayerRemovedReason playerRemovedReason, bool& gameDestroyed)
    {

        // we don't call mGameDataMaster->setIsNpSessionUnavailable(true); here because we would have marked it unavailable when it was last cleared.
        bool waitingOnNpSession = mGameDataMaster->getIsNpSessionUnavailable();

        PlayerInfoMaster *newHost = suggestedHost;
        // Store off the migration type to be used when the platform host client notifies
        // us the network has been migrated.
        mGameDataMaster->setHostMigrationType(migrationType);
        mGameDataMaster->setNumMigrationsReceived(0);

        // if the suggestedHost is invalid, we have to try to select new host
        if (newHost == nullptr)
        {
            newHost = selectNewHost(migrationType, playerRemovedReason);
            if (newHost == nullptr)
            {
                // Select host can return nullptr even if there are players in the game session.  Dedicated servers don't fail or reset here, but
                // other topologies check if there is no one left in the game, otherwise its a failure state.
                if (hasDedicatedServerHost() && (migrationType == PLATFORM_HOST_MIGRATION))
                {
                    // Kick all players that are actively connecting, because they may be unable to join now that there is no platform host.
                    // make a copy, so we won't crash when we modify it from removePlayer while iterating over the container
                    const PlayerRoster::PlayerInfoList connectedPlayers = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PLAYERS);
                    PlayerRoster::PlayerInfoList::const_iterator iter = connectedPlayers.begin();
                    PlayerRoster::PlayerInfoList::const_iterator end = connectedPlayers.end();
                    for(; iter != end; ++iter)
                    {
                        PlayerInfoMaster *playerInfo = static_cast<PlayerInfoMaster*>(*iter);
                        if (playerInfo->getPlayerState() == ACTIVE_CONNECTING)
                        {
                            TRACE_LOG("[GameSessionMaster] Removing connecting player(" << playerInfo->getPlayerId()
                                << ") due to host migration failing to find a new platform host.");
                            RemovePlayerMasterRequest removePlayerRequest;
                            removePlayerRequest.setGameId(getGameId());
                            removePlayerRequest.setPlayerId(playerInfo->getPlayerId());
                            removePlayerRequest.setPlayerRemovedReason(MIGRATION_FAILED);
                            RemovePlayerMasterError::Error removePlayerError;
                            // Game was destroyed due to player removal, no need to continue.
                            // (Shouldn't happen as dedicated server topology host always has a connection to self)
                            if (gGameManagerMaster->removePlayer(&removePlayerRequest, playerInfo->getPlayerId(), removePlayerError) == GameManagerMasterImpl::REMOVE_PLAYER_GAME_DESTROYED)
                            {
                                gameDestroyed = true;
                                return false;
                            }
                        }
                    }

                    // After the last person in the client server dedicated game leaves
                    // reset the platform host to be the topology host. The next player
                    // joining the game will be set to the platform host.
                    getGameData().getDedicatedServerHostInfo().copyInto( getGameData().getPlatformHostInfo() );

                    // For host migration completion, server managed queues need to pump the queue, since it was skipped
                    // on player removal.  Admin managed queues have already sent the notification.
                    processPlayerServerManagedQueue();
                    // Migration completes successfully, since the game is empty.
                    return true;
                }
                else if ((getGameNetworkTopology() == NETWORK_DISABLED) && mPlayerRoster.hasRosterPlayers() && (migrationType == PLATFORM_HOST_MIGRATION))
                {
                    // if we're only doing a platform host migration on a game with network disabled, it means that there isn't a topology host and never was
                    // it's safe to keep it around if the roster isn't empty.


                    // first check that there's going to be anyone left 
                    uint16_t additionalRemovedPlayersCount = 0; // if the old host isn't taking a group along, we've already removed everyone from the roster
                    if (removalKicksConngroup(playerRemovedReason) && (getPlatformHostInfo().getConnectionGroupId() != INVALID_CONNECTION_GROUP_ID))
                    {
                        if (getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(getPlatformHostInfo().getConnectionGroupId()) != getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
                        {
                            PlayerIdList* connectionGroupMembers = getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(getPlatformHostInfo().getConnectionGroupId())->second;
                            additionalRemovedPlayersCount = connectionGroupMembers->size();
                        }
                    }

                    if (additionalRemovedPlayersCount < mPlayerRoster.getRosterPlayerCount())
                    {
                        // clear out host info!
                        setHostInfo(nullptr);

                        TRACE_LOG("GameSessionMaster::startHostMigration: no valid new host is found for non-empty networkless game(" << getGameId() << "), clearing host info.");
                        return true; 
                    }
                    else 
                    {
                        // game is going to be empty, destroy
                        TRACE_LOG("GameSessionMaster::startHostMigration: no roster members remaining in networkless game (" << getGameId() << "), migration failed (destroying game).");
                        return false;
                    }
                }
                else
                {
                    TRACE_LOG("GameSessionMaster::startHostMigration: Failed since no valid new host is found for game(" << getGameId() << ").");
                    return false;
                }
            }
        }

        TRACE_LOG("[GameSessionMaster] Starting host migration for game(" << getGameId() << ") containing " << mPlayerRoster.getRosterPlayerCount()
                  << " players (migrationType: " << HostMigrationTypeToString(migrationType) << "). Next host(" << newHost->getPlayerId()
                  << ":" << newHost->getPlayerName() << "). Current TopologyHost(" << getTopologyHostInfo().getPlayerId()
                  << "). Current PlatformHost(" << getPlatformHostInfo().getPlayerId() << "). Game topology("
                  << GameNetworkTopologyToString(getGameNetworkTopology()) << ")");

        // exit early if the selected host is the current topology host
        // exit early if we are only migrating the platform host and it is the current platform host.
        if (((migrationType != PLATFORM_HOST_MIGRATION) && (newHost->getPlayerId() == getTopologyHostInfo().getPlayerId())) ||
            ((migrationType == PLATFORM_HOST_MIGRATION) && (newHost->getPlayerId() == getPlatformHostInfo().getPlayerId())))
        {
            return true;
        }

        if (!newHost->getSessionExists())
        {
            TRACE_LOG("[GameSessionMaster] Failed since chosen host no longer has a valid session for game(" << getGameId() << ").");
            return false;
        }

        if (getGameState() == INITIALIZING)
        {
            TRACE_LOG("[GameSessionMaster] Migration failed for game(" << getGameId() << ") as current state is still INITIALIZING.");
            return false;
        }

        //If we are starting migration while already migrating
        if (getGameState() != MIGRATING)
        {
            mGameDataMaster->setPreHostMigrationState(getGameState());
            if (setGameState(MIGRATING) != ERR_OK)
            {
                ERR_LOG("GameSessionMaster::startHostMigration - couldn't set game state to MIGRATING!");
                return false;
            }
        }

        // if the new host is on the same connection, network migration should be skipped
        // however, we still need to update information on the new host.
        bool hasNetworkMigration = true;
        if ((migrationType != PLATFORM_HOST_MIGRATION) &&  (getTopologyHostInfo().getConnectionGroupId() == newHost->getConnectionGroupId()))
        {
            hasNetworkMigration = false;
            waitingOnNpSession = false;
        }
        else if ((migrationType == PLATFORM_HOST_MIGRATION) && (getPlatformHostInfo().getConnectionGroupId() == newHost->getConnectionGroupId()))
        {
            hasNetworkMigration = false;
            waitingOnNpSession = false;
        }

        // In the future branch, we can probably remove the network disabled check, but leaving it here
        // to limit the effect to only those game sessions.
        if ((migrationType == PLATFORM_HOST_MIGRATION) && (getGameNetworkTopology() == NETWORK_DISABLED))
        {
            getGameData().getPlatformHostInfo().setPlayerId(newHost->getPlayerId());
            getGameData().getPlatformHostInfo().setUserSessionId(newHost->getUserInfo().getSessionId());
            getGameData().getPlatformHostInfo().setSlotId(newHost->getSlotId());
            getGameData().getPlatformHostInfo().setConnectionSlotId(newHost->getConnectionSlotId());
            getGameData().getPlatformHostInfo().setConnectionGroupId(newHost->getConnectionGroupId());
            TRACE_LOG("[GameSessionMaster] Player(" << getPlatformHostInfo().getPlayerId() << ") in slot " << getPlatformHostInfo().getSlotId()
                << " set as platform host of game(" << getGameId() << ")");
        }
        else
        {
            setHostInfo(&newHost->getUserInfo());
            // We need to establish new connections in the following cases:
            // A Topology Host migration has occurred, and we do not already have connections established for the other endpoints.
            // (Full connections are already established for Full/Partial mesh topologies)
            if ((migrationType != PLATFORM_HOST_MIGRATION) && 
                (getGameNetworkTopology() != PEER_TO_PEER_FULL_MESH))
            {
                getPlayerConnectionMesh()->addEndpoint(newHost->getConnectionGroupId(), newHost->getPlayerId(), newHost->getClientPlatform(), newHost->getConnectionJoinType(), hasNetworkMigration);
            }
        }

        if (((migrationType == TOPOLOGY_HOST_MIGRATION) ||
            (migrationType == TOPOLOGY_PLATFORM_HOST_MIGRATION)) && !hasDedicatedServerHost())
        {
            const char8_t* preMigrationPingSite = getGameData().getPingSiteAlias();
            const char8_t* newHostPingSite = getTopologyHostUserInfo().getBestPingSiteAlias();

            // Sanity check to avoid setting a ping site to some uninitialized value:
            if (!gUserSessionManager->getQosConfig().isAcceptablePingSiteAliasName(newHostPingSite))
            {
                newHostPingSite = INVALID_PINGSITE;
            }

            if (blaze_stricmp(preMigrationPingSite, newHostPingSite) != 0)
            {
                TRACE_LOG("GameSessionMaster::startHostMigration - game(" << getGameId() << ") pre migration ping site("<< getGameData().getPingSiteAlias() << ") "<< "New host ping site("<< newHostPingSite<<")");
                mMetricsHandler.onPingSiteChanged(preMigrationPingSite, newHostPingSite);
                getGameData().setPingSiteAlias(newHostPingSite);
            }
        }


        if (hasNetworkMigration && (getGameNetworkTopology() == CLIENT_SERVER_PEER_HOSTED))
        {
            // The adapter will not attempt to reconnect players who are already connected to the new host.
            // Players who are not currently connected to the new host (but were to the old one)
            // will be removed from the game.

            // We first mark everyone but the host as active_migrating.  This is to prevent removePlayer
            // from completing migration early on us.
            mPlayerRoster.initPlayerStatesForHostMigrationStart(*newHost);

            // update the player state for any users who are already connected to the 'new' host
            const PlayerRoster::PlayerInfoList connectedPlayers = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_MIGRATING_PLAYERS);
            PlayerRoster::PlayerInfoList::const_iterator iter = connectedPlayers.begin();
            PlayerRoster::PlayerInfoList::const_iterator end = connectedPlayers.end();
            for(; iter != end; ++iter)
            {
                PlayerInfoMaster *playerInfo = static_cast<PlayerInfoMaster*>(*iter);
                bool isConnected = mPlayerConnectionMesh.areEndpointsConnected(playerInfo->getConnectionGroupId(), newHost->getConnectionGroupId());
                
                if (isConnected && (playerInfo->getPlayerId() != newHost->getPlayerId()))
                {
                    // I am connected and not the host, move me back to connected. (host is already active_connected.)
                    mPlayerRoster.updatePlayerState(playerInfo, ACTIVE_CONNECTED);
                }
                // else other players have to establish connections
            }

            // reset the join timer(s) for anyone who's currently trying to join the game
            // since they need to connect to a new host now
            mPlayerRoster.resetConnectingMembersJoinTimer();
        }

        // send notification of platform host changed or host migration started

        NotifyHostMigrationStart notifyHostMigrationStart;
        notifyHostMigrationStart.setGameId(getGameId());
        notifyHostMigrationStart.setMigrationType(migrationType);


        if (migrationType == TOPOLOGY_HOST_MIGRATION)
        {
            notifyHostMigrationStart.setNewHostId(getTopologyHostInfo().getPlayerId());
            notifyHostMigrationStart.setNewHostSlotId(getTopologyHostInfo().getSlotId());
            notifyHostMigrationStart.setNewHostConnectionSlotId(getTopologyHostInfo().getConnectionSlotId());
        }
        else
        {
            // Either just platform host, or platform host is topology host.
            notifyHostMigrationStart.setNewHostId(getPlatformHostInfo().getPlayerId());
            notifyHostMigrationStart.setNewHostSlotId(getPlatformHostInfo().getSlotId());
            notifyHostMigrationStart.setNewHostConnectionSlotId(getPlatformHostInfo().getConnectionSlotId());
        }
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyHostMigrationStartToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &notifyHostMigrationStart);

        //cancel outstanding timer
        if (mGameDataMaster->getMigrationTimeout() != 0)
        {
            gGameManagerMaster->getMigrationTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setMigrationTimeout(0);
        }

        //For GGs, there is no ACTIVE_MIGRATING players but we might be waiting on NP Session. 
        if (!mPlayerRoster.hasActiveMigratingPlayers() && !waitingOnNpSession)
        {
            finishHostMigration();
            return true;
        }

        if (mGameDataMaster->getMigrationTimeout() == 0)
        {
            TimeValue timeout = TimeValue::getTimeOfDay() + gGameManagerMaster->getHostMigrationTimeout();
            if (gGameManagerMaster->getMigrationTimerSet().scheduleTimer(getGameId(), timeout))
                mGameDataMaster->setMigrationTimeout(timeout);
        }

        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief Finishes the host migration process. Returns the game to it's pre-migration state.
        ***************************************************************************************************/
    void GameSessionMaster::finishHostMigration()
    {
        setGameState(mGameDataMaster->getPreHostMigrationState());

        TRACE_LOG("[GameSessionMaster.finishHostMigration] Host migration finished for game(" << getGameId() << ")");

        //send a notification to everyone in game
        NotifyHostMigrationFinished notifyHostMigrationFinished;
        notifyHostMigrationFinished.setGameId(getGameId());
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyHostMigrationFinishedToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &notifyHostMigrationFinished);


        // If needed, pump the queue. When the platform or topology host leaves the game, we do not trigger queue joins
        // as they will fail.  We wait for the platform host for 360 to wait for the new session.
        // We pump the queue here now that the new host is established.
        if (getTotalPlayerCapacity() > mPlayerRoster.getRosterPlayerCount())
        {
            // only need to process server managed queue as admin managed will send notification immediately following
            // player removal.
            processPlayerServerManagedQueue();
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Called to indicate that all players have migrated to the new host. Cancels the timer callback
    and finished the host migration process.
        ***************************************************************************************************/
    void GameSessionMaster::hostMigrationCompleted()
    {
        if (mGameDataMaster->getMigrationTimeout() != 0)
        {
            gGameManagerMaster->getMigrationTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setMigrationTimeout(0);
        }
        finishHostMigration();
    }

    /*! ************************************************************************************************/
    /*! \brief Called a set time after the host migration process has started. Kicks all players who have not
    yet migrated to the new host and finishes the host migration process.
    ***************************************************************************************************/
    void GameSessionMaster::hostMigrationTimeout(GameId gameId)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(gameId);
        if (gameSession == nullptr)
            return;

        gameSession->mGameDataMaster->setMigrationTimeout(0);

        TRACE_LOG("[GameSessionMaster] Host migration timed out for game(" << gameId << ") - removing un-joined players.");

        //make a copy, so we won't crash when we modify it while iterating over the container
        const PlayerRoster::PlayerInfoList migratingPlayers = gameSession->mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_MIGRATING_PLAYERS);
        PlayerRoster::PlayerInfoList::const_iterator iter = migratingPlayers.begin();
        PlayerRoster::PlayerInfoList::const_iterator end = migratingPlayers.end();
        for(; iter != end; ++iter)
        {
            PlayerInfoMaster *playerInfo = static_cast<PlayerInfoMaster*>(*iter);
            RemovePlayerMasterRequest removePlayerRequest;
            removePlayerRequest.setGameId(gameId);
            removePlayerRequest.setPlayerId(playerInfo->getPlayerId());
            removePlayerRequest.setPlayerRemovedReason(MIGRATION_FAILED);
            RemovePlayerMasterError::Error removePlayerError;
            if (gGameManagerMaster->removePlayer(&removePlayerRequest, playerInfo->getPlayerId(), removePlayerError) == GameManagerMasterImpl::REMOVE_PLAYER_GAME_DESTROYED)
            {
                // game destroyed due to player removal, no need to continue
                return;
            }
        }
        gameSession->finishHostMigration();
    }

    bool GameSessionMaster::usesTrustedGameReporting() const
    {
        return (hasDedicatedServerHost() &&
                gGameManagerMaster->getTrustedDedicatedServerReports());
    }

    /*! ************************************************************************************************/
    /*! \brief Called when a game has finished

        \param[in] removedPlayerId - the id of a player who was removed from the game (but who actually finished the game).
                In other words, when the game is destroyed due to a player being kicked, we call out that player so we can mark them
                as someone who finished the game instead of a DNF player.
    ***************************************************************************************************/
    void GameSessionMaster::gameFinished(PlayerId removedPlayerId /* =0 */)
    {
        //A game isn't finished if it never started.
        if (!mGameDataMaster->getGameStarted())
        {
            return;
        }
        mGameDataMaster->setGameFinished(true);

        mMatchEndTime = TimeValue::getTimeOfDay();
        gGameManagerMaster->buildAndSendMatchInfoEvent(*this, RiverPoster::MATCH_INFO_STATUS_END);

        // Send off the HTTP event as well:
        buildAndSendTournamentGameEvent(getGameEndEventUri(), ERR_OK);

        //Reset for rematch
        mGameDataMaster->setGameStarted(false);

        mMetricsHandler.onGameFinished();

        if (hasDedicatedServerHost())
        {
            gGameManagerMaster->logDedicatedServerEvent("game finished", *this);
            // we'll log all the user info for the ended game when removing the players
        }
        //Trusted game reporting is currently performed on the gamereporting slave, there is no need to notify gamereporting master
        if (getGameReportingId() == GameManager::INVALID_GAME_REPORTING_ID)
        {
            return;
        }

        if (GetSliverIdFromSliverKey(getGameReportingId()) == INVALID_SLIVER_ID)
        {
            // Always following this code path means that the integrator isn't using game reporting at all; however, GM flows still require a "valid" GR ID.
            // Otherwise, there should be other problem indicators if we occasionally enter here, e.g. failing to connect to grSlave.
            TRACE_LOG("[GameSessionMaster].gameFinished: non-sliver-sharded game reporting id " << getGameReportingId());
            return;
        }

        //  new game reporting
        GameInfoPtr finishedGameInfo = BLAZE_NEW GameInfo();
        fillGameInfo(*finishedGameInfo);

        // see if we need to fixup a removed player
        GameInfo::PlayerInfoMap::iterator removedItr = finishedGameInfo->getPlayerInfoMap().find(removedPlayerId);
        if (removedItr != finishedGameInfo->getPlayerInfoMap().end())
        {
            // if (removedItr->second->getFinished() == false) then ...
            // the game was destroyed as a side effect of removing this player; since they were logically present when the game ended, we
            //    need to treat them as if they finished the game -- (even though they were technically removed from the roster before
            //    the game was actually destroyed).
            removedItr->second->setFinished(true);
        }

        FiberJobQueue::JobParams jobParams("GameSessionMaster::sendGameFinishedToGameReportingSlave");
        jobParams.subQueueId = GetSliverIdFromSliverKey(finishedGameInfo->getGameReportingId());
        gGameManagerMaster->getGameReportingRpcJobQueue().queueFiberJob(
            &GameSessionMaster::sendGameFinishedToGameReportingSlave, getGameId(), finishedGameInfo, jobParams);
    }

    void GameSessionMaster::sendGameFinishedToGameReportingSlave(GameId gameId, GameInfoPtr finishedGameInfo)
    {
#ifdef TARGET_gamereporting
        GameReporting::GameReportingSlave *grSlave = static_cast<GameReporting::GameReportingSlave *>(gController->getComponent(GameReporting::GameReportingSlave::COMPONENT_ID, false, true));
        if (grSlave == nullptr)
        {
            WARN_LOG("[GameSessionMaster].gameFinished: gamereporting slave not found, finished event for game: " << gameId << " not sent.");
        }
        else
        {
            RpcCallOptions options;
            options.ignoreReply = true; // don't block, can't do anything about it anyway
            BlazeRpcError rc = grSlave->gameFinished(*finishedGameInfo, options);
            if (rc != ERR_OK)
            {
                WARN_LOG("[GameSessionMaster].gameFinished: gamereporting slave not found, send finished event for game: " << gameId << " failed with err: " << ErrorHelp::getErrorName(rc));
            }
        }
#endif
    }

    void GameSessionMaster::fillGameInfo(GameInfo& gameInfo) const
    {
        TimeValue sessionLength = (TimeValue::getTimeOfDay() - mGameDataMaster->getGameStartTime());
        gameInfo.setGameDurationMs((uint32_t)(sessionLength.getMillis()));
        gameInfo.setGameReportingId(getGameReportingId());
        gameInfo.setGameReportName(getGameReportName());
        getGameAttribs().copyInto(gameInfo.getAttributeMap());
        gameInfo.getGameSettings() = getGameSettings();
        gameInfo.setTrustedGameReporting(usesTrustedGameReporting());
        gameInfo.setGameId(getGameId());
        getExternalSessionIdentification().copyInto(gameInfo.getExternalSessionIdentification());

        //add current players in game.
        GameInfo::PlayerInfoMap &playerInfoMap = gameInfo.getPlayerInfoMap();
        const PlayerRoster::PlayerInfoList& activePlayerList = mPlayerRoster.getPlayers(PlayerRoster::ROSTER_PARTICIPANTS);
        PlayerRoster::PlayerInfoList::const_iterator iter, end;
        iter = activePlayerList.begin();
        end = activePlayerList.end();

        playerInfoMap.reserve(activePlayerList.size() + mGameDataMaster->getDnfPlayerList().size());

        for(;iter != end; ++iter)
        {
            GamePlayerInfo *playerInfo = playerInfoMap.allocate_element();
            PlayerId playerId = (*iter)->getPlayerId();

            (*iter)->getPlayerAttribs().copyInto(playerInfo->getAttributeMap());
            playerInfo->setAccountLocale((*iter)->getAccountLocale());
            playerInfo->setAccountCountry((*iter)->getAccountCountry());
            playerInfo->setRemovedReason(GAME_ENDED);                // We don't attempt to merge DNF and Reserved player data.  So if you leave with a reservation, this removal reason will be used until the reservation expires (at which time the DNF value will be used).
            playerInfo->setFinished(!(*iter)->isReserved());
            playerInfo->setUserSessionId((*iter)->getPlayerSessionId());
            playerInfo->setEncryptedBlazeId((*iter)->getPlayerData()->getEncryptedBlazeId());
            playerInfo->setTeamIndex((*iter)->getTeamIndex());
            UserInfo::filloutUserIdentification((*iter)->getUserInfo().getUserInfo(), playerInfo->getUserIdentification());
            playerInfoMap[playerId] = playerInfo;
        }

        // add did not finish players
        GameDataMaster::DnfPlayerList::reverse_iterator ri = mGameDataMaster->getDnfPlayerList().rbegin();
        GameDataMaster::DnfPlayerList::reverse_iterator re = mGameDataMaster->getDnfPlayerList().rend();
        for (; ri != re; ++ri)
        {
            PlayerId playerId = (*ri)->getPlayerId();

            // don't insert this player twice
            if (playerInfoMap.find(playerId) == playerInfoMap.end())
            {
                GamePlayerInfo *playerInfo = playerInfoMap.allocate_element();
                (*ri)->getAttributes().copyInto(playerInfo->getAttributeMap());
                playerInfo->setAccountLocale((*ri)->getAccountLocale());
                playerInfo->setAccountCountry((*ri)->getAccountCountry());
                playerInfo->setRemovedReason((*ri)->getRemovedReason());
                playerInfo->setFinished(false);
                playerInfo->setUserSessionId((*ri)->getUserSessionId());
                playerInfo->setEncryptedBlazeId((*ri)->getEncryptedBlazeId());
                playerInfo->setTeamIndex((*ri)->getTeamIndex());
                (*ri)->getUserIdentification().copyInto(playerInfo->getUserIdentification());
                playerInfoMap[playerId] = playerInfo;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Called when a game has started
    ***************************************************************************************************/
    void GameSessionMaster::gameStarted()
    {
        mGameDataMaster->setGameStarted(true);
        mGameDataMaster->setGameStartTime(TimeValue::getTimeOfDay());
        mGameDataMaster->getDnfPlayerList().clear();
        // clear out the old end time if a new match starts
        mMatchEndTime = TimeValue(0);
        
        mMetricsHandler.onGameStarted();

        if (hasDedicatedServerHost())
        {
            gGameManagerMaster->logDedicatedServerEvent("game started", *this);
            const PlayerRoster::PlayerInfoList& activePlayerList = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PARTICIPANTS);
            PlayerRoster::PlayerInfoList::const_iterator iter, end;
            iter = activePlayerList.begin();
            end = activePlayerList.end();
            for(;iter != end; ++iter)
            {
                gGameManagerMaster->logUserInfoForDedicatedServerEvent("game started", (*iter)->getPlayerId(), (*iter)->getLatencyMap(), *this);
            }
        }

        if (GetSliverIdFromSliverKey(getGameReportingId()) != INVALID_SLIVER_ID)
        {
            StartedGameInfoPtr startedGameInfo = BLAZE_NEW StartedGameInfo();
            startedGameInfo->setGameReportingId(getGameReportingId());
            startedGameInfo->setGameReportName(getGameReportName());
            startedGameInfo->setTrustedGameReporting(usesTrustedGameReporting());
            startedGameInfo->setTopology(getGameNetworkTopology());
            startedGameInfo->setTopologyHostId(getTopologyHostInfo().getPlayerId());
            startedGameInfo->setGameId(getGameId());

            FiberJobQueue::JobParams jobParams("GameSessionMaster::sendGameStartedToGameReportingSlave");
            jobParams.subQueueId = GetSliverIdFromSliverKey(startedGameInfo->getGameReportingId());
            gGameManagerMaster->getGameReportingRpcJobQueue().queueFiberJob(
                &GameSessionMaster::sendGameStartedToGameReportingSlave, startedGameInfo, jobParams);
        }
        else if (getGameReportingId() != GameManager::INVALID_GAME_REPORTING_ID)
        {
            // Always following this code path means that the integrator isn't using game reporting at all; however, GM flows still require a "valid" GR ID.
            // Otherwise, there should be other problem indicators if we occasionally enter here, e.g. failing to connect to grSlave.
            TRACE_LOG("[GameSessionMaster].gameStarted: non-sliver-sharded game reporting id " << getGameReportingId());
        }

        refreshPINIsCrossplayGameFlag();

        gGameManagerMaster->buildAndSendMatchInfoEvent(*this, RiverPoster::MATCH_INFO_STATUS_BEGIN);

        // Spawn an Event, tracked via Redis, that sends the event 
        buildAndSendTournamentGameEvent(getGameStartEventUri(), ERR_OK);
    }

    void GameSessionMaster::buildAndSendTournamentGameEvent(const char8_t* eventUri, BlazeRpcError eventErrorCode) const
    {
        if (getGameEventAddress()[0] != '\0')
        {
            ExternalHttpGameEventDataPtr data = BLAZE_NEW ExternalHttpGameEventData();
            data->setGameId(getGameId());
            data->setGameReportingId(getGameReportingId());
            data->setError(eventErrorCode != ERR_OK ? ErrorHelp::getErrorName(eventErrorCode) : "");
            mGameDataMaster->getTournamentIdentification().copyInto(data->getTournamentIdentification());
            auto playersList = mPlayerRoster.getPlayers(PlayerRoster::ALL_PLAYERS);
            for (const auto& itr : playersList)
            {
                GameManagerMasterImpl::populateTournamentGameEventPlayer(data->getGameRoster(), itr->getPlayerData()->getSlotType(),
                    itr->getPlayerData()->getEncryptedBlazeId(), itr->getPlayerId(), itr->getPlayerData()->getPlayerState());
            }

            gFiberManager->scheduleCall<GameManagerMasterImpl, const ExternalHttpGameEventDataPtr>(gGameManagerMaster,
                &GameManagerMasterImpl::sendExternalTournamentGameEvent, data, eastl::string(getGameEventAddress()),
                eastl::string(eventUri), eastl::string(getGameEventContentType()), Blaze::Fiber::CreateParams());
        }
    }

    void GameSessionMaster::sendGameStartedToGameReportingSlave(StartedGameInfoPtr startedGameInfo)
    {
#ifdef TARGET_gamereporting
        GameReporting::GameReportingSlave *grSlave = static_cast<GameReporting::GameReportingSlave *>(gController->getComponent(GameReporting::GameReportingSlave::COMPONENT_ID, false, true));
        if (grSlave == nullptr)
        {
            WARN_LOG("[GameSessionMaster].gameStarted: gamereporting slave not found, game started event for game: " << startedGameInfo->getGameId() << " not sent.");
        }
        else
        {
            RpcCallOptions options;
            options.ignoreReply = true; // don't block, can't do anything about it anyway
            BlazeRpcError rc = grSlave->gameStarted(*startedGameInfo, options);
            if (rc != ERR_OK)
            {
                WARN_LOG("[GameSessionMaster].gameStarted: gamereporting slave not found, send game started event for game: " << startedGameInfo->getGameId() << " failed with err: " << ErrorHelp::getErrorName(rc));
            }
        }
#endif
    }


    /*! ************************************************************************************************/
    /*! \brief Check that the new desired slot vector has valid capacity against our defined slot
            types as well as the games max capacity.
        \param[in] newSlotCapcities - the new vector of player capacity per slot.
        \return BlazeRpcError - ERR_SYSTEM if the vector is completely invalid, or GAMEMANAGER_ERR_PLAYER_CAPACITY_IS_ZERO
            if capacity is 0, or GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_LARGE if capacity is larger than game capacity.
    ***************************************************************************************************/
    BlazeRpcError GameSessionMaster::validateSlotCapacities(const SlotCapacitiesVector &newSlotCapacities) const
    {
        // Major malfunction, number of slots should equal max number of slot types.
        if (newSlotCapacities.size() != MAX_SLOT_TYPE)
        {
            ERR_LOG("[GameSessionMaster] Attempt to set unknown slot types for new slot capacities in game(" << getGameId() << ") " << newSlotCapacities.size() <<" != "<< MAX_SLOT_TYPE );
            return ERR_SYSTEM;
        }

        // calculate the new total capacity & validate the new settings
        uint16_t currentMaxPlayerCapacity = getMaxPlayerCapacity();
        uint16_t currentMinPlayerCapacity = getMinPlayerCapacity();
        uint16_t newTotalSlotCapacity = 0;
        uint16_t newTotalParticipantCapacity = 0;
        for (int32_t slot = 0; slot < (int32_t)MAX_SLOT_TYPE; slot++)
        {
            auto slotCap = newSlotCapacities[slot];
            if (slotCap > currentMaxPlayerCapacity)
            {
                ERR_LOG("[GameSessionMaster] SlotType(" << SlotTypeToString((SlotType)slot) << ") capacity(" << slotCap << ") cannot be "
                    << "greater than the game maximum player capacity(" << currentMaxPlayerCapacity << "), for game(" << getGameId() << ")");
                return GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_LARGE;
            }
            if (newTotalSlotCapacity > (UINT16_MAX - slotCap))// prevent overflow
            {
                ERR_LOG("[GameSessionMaster] Total slotCapacities cannot be higher than capacity's numeric type's max, for game(" << getGameId() << ")");
                return GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_LARGE;
            }

            newTotalSlotCapacity = newTotalSlotCapacity + slotCap;
            if (isParticipantSlot((SlotType)slot))
                newTotalParticipantCapacity += slotCap;
        }

        // Participant capacity can't be 0.
        if ( newTotalParticipantCapacity == 0 )
        {
            TRACE_LOG("[GameSessionMaster] Zero participant capacity found while validating game(" << getGameId() << ") slot capacities.");
            return GAMEMANAGER_ERR_PLAYER_CAPACITY_IS_ZERO;
        }

        // Player capacity can't be below the min player capacity of the game
        if (newTotalSlotCapacity < currentMinPlayerCapacity)
        {
            TRACE_LOG("[GameSessionMaster] Total slotCapacities (" << newTotalSlotCapacity
                      << ") cannot be lower than the game minimum player capacity (" << currentMinPlayerCapacity
                      << ") for game(" << getGameId() << ")" );
            return GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_SMALL;
        }

        // Player capacity can't be above the max player capacity of the game
        if (newTotalSlotCapacity > currentMaxPlayerCapacity)
        {
            TRACE_LOG("[GameSessionMaster] Total slotCapacities (" << newTotalSlotCapacity
                      << ") cannot be higher than the game maximum player capacity (" << currentMaxPlayerCapacity
                      << ") for game(" << getGameId() << ")" );
            return GAMEMANAGER_ERR_PLAYER_CAPACITY_TOO_LARGE;
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief Check that the new desired slot vector has valid capacity against our defined slot
    types as well as the games max capacity.
    \param[in] newTeamIds - the new vector of team ids
    \param[in] totalPlayerCapacity - the total capacity of the game
    \return BlazeRpcError - ERR_SYSTEM if the vector is completely invalid, or GAMEMANAGER_ERR_PLAYER_CAPACITY_NOT_EVENLY_DIVISIBLE_BY_TEAMS if the team count won't divide
        the player capacity evenly. GAMEMANAGER_ERR_INVALID_TEAM_ID_IN_TEAM_CAPACITIES_VECTOR if INVALID_TEAM_ID is in the id vector.
    ***************************************************************************************************/
    Blaze::BlazeRpcError GameSessionMaster::validateTeams( const TeamIdVector &newTeamIds, const uint16_t &totalPlayerCapacity ) const
    {

        // early out if teamCapacities vector is empty
        if (newTeamIds.empty())
        {
            TRACE_LOG("[GameSessionMaster] TeamIdVector was empty for game(" << getGameId() << ")" );
            return GAMEMANAGER_ERR_INVALID_TEAM_CAPACITIES_VECTOR_SIZE;
        }

        uint16_t teamCount = (uint16_t)newTeamIds.size();

        // make sure the teams can divide the capacity evenly
        if ((teamCount != 0) && ((totalPlayerCapacity % teamCount) != 0))
        {
            TRACE_LOG("[GameSessionMaster] Team count (" << teamCount << ") must be able to evenly divide to the game maximum player capacity ("
                << totalPlayerCapacity << ") for game(" << getGameId() << ")" );
            return GAMEMANAGER_ERR_PLAYER_CAPACITY_NOT_EVENLY_DIVISIBLE_BY_TEAMS;
        }

        TeamIdVector::const_iterator teamsIter = newTeamIds.begin();
        TeamIdVector::const_iterator teamsEnd = newTeamIds.end();

        for(; teamsIter != teamsEnd; ++teamsIter)
        {
            if (EA_UNLIKELY(*teamsIter == INVALID_TEAM_ID))
            {
                TRACE_LOG("[GameSessionMaster] Team with id INVALID_TEAM_ID found while validating game(" << getGameId() << ") team capacities.");
                return GAMEMANAGER_ERR_INVALID_TEAM_ID_IN_TEAM_CAPACITIES_VECTOR;
            }

        }

        return ERR_OK;
    }

    BlazeRpcError GameSessionMaster::validateRoleInformation(const RoleInformation &newRoleInfo, const uint16_t teamCapacity) const
    {
        // early out if role criteria map is empty
        if (newRoleInfo.getRoleCriteriaMap().empty())
        {
            TRACE_LOG("[GameSessionMaster] RoleCriteriaMap was empty for game(" << getGameId() << ")" );
            return GAMEMANAGER_ERR_EMPTY_ROLE_CAPACITIES;
        }

        // check total role capacity, no individual role should have more space than the size of a team in the game
        // check if roles includes PLAYER_ROLE_NAME_DEFAULT, in which case it should be the only role in the game.
        bool hasPlayerRoleNameDefault = false;
        uint16_t totalRoleCapacity = 0;
        RoleCriteriaMap::const_iterator rolesIter = newRoleInfo.getRoleCriteriaMap().begin();
        RoleCriteriaMap::const_iterator rolesEnd = newRoleInfo.getRoleCriteriaMap().end();
        for(; rolesIter != rolesEnd; ++rolesIter)
        {
            uint16_t roleCapacity = rolesIter->second->getRoleCapacity();

            if (blaze_stricmp(rolesIter->first, GameManager::PLAYER_ROLE_NAME_ANY) == 0)
            {
                // 'PLAYER_ROLE_NAME_ANY' is reserved and not allowed as a joinable role in the game session
                TRACE_LOG("[GameSessionMaster] RoleCriteriaMap had role '" << rolesIter->first << "' with capacity of (" << roleCapacity
                    << ") this is not valid for game(" << getGameId() << ")" );
                return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;

            }

            if (EA_UNLIKELY(roleCapacity > teamCapacity))
            {
                TRACE_LOG("[GameSessionMaster] RoleCriteriaMap had role '" << rolesIter->first << "' with capacity of (" << roleCapacity
                    << ") larger than team capacity of (" << teamCapacity << ") for game(" << getGameId() << ")" );
                return GAMEMANAGER_ERR_ROLE_CAPACITY_TOO_LARGE;
            }

            totalRoleCapacity += roleCapacity;

            if (blaze_stricmp(rolesIter->first.c_str(), PLAYER_ROLE_NAME_DEFAULT) == 0)
                hasPlayerRoleNameDefault = true;
        }

        if (totalRoleCapacity < teamCapacity)
        {
            TRACE_LOG("[GameSessionMaster] Total role capacity  of (" << totalRoleCapacity
                << ") smaller than team capacity of (" << teamCapacity << ") for game(" << getGameId() << ")" );
            return GAMEMANAGER_ERR_ROLE_CAPACITY_TOO_SMALL;
        }

        if (hasPlayerRoleNameDefault && (newRoleInfo.getRoleCriteriaMap().size() > 1))
        {
            ERR_LOG("[GameSessionMaster] when RoleCriteriaMap includes default role name('" << PLAYER_ROLE_NAME_DEFAULT << "') its expected to be the only role available for the game RoleCriteriaMap, however the RoleCriteriaMap for game(" << getGameId() << "), also included other roles (" << newRoleInfo.getRoleCriteriaMap().size() << " total).");
            return GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
        }
        return ERR_OK;
    }

    BlazeRpcError GameSessionMaster::injectHost(const UserSessionInfo& hostInfo, const GameId& hostGameId, const NetworkAddressList& hostNetworkAddressList, const char8_t* joiningProtocolVersionString)
    {
        if (getGameData().getNetworkTopology() != CLIENT_SERVER_DEDICATED)
        {
            ERR_LOG("[GameSessionMaster] Unable to inject host in game(" << getGameId() << "), only allowed for dedicated servers.");
            EA_FAIL();
        }

        if (getGameReportingId() == GameManager::INVALID_GAME_REPORTING_ID)
        {
            WARN_LOG("[GameSessionMaster].injectHost Poss error at prior host ejection, no gamereporting id was generated for game(" << getGameId() << ". Attempting to generate new one.");
            setGameReportingId(gGameManagerMaster->getNextGameReportingId());
        }
        if (getGameReportingId() == GameManager::INVALID_GAME_REPORTING_ID)
        {
            ERR_LOG("[GameSessionMaster].injectHost failed to generate a valid game reporting id for game(" << getGameId());
            return ERR_SYSTEM;
        }

        // GameState is changed to PRE_GAME in here.
        setHostInfo(&hostInfo);
        initializeDedicatedGameHost(hostInfo);
        hostNetworkAddressList.copyInto(getGameData().getTopologyHostNetworkAddressList());
        hostNetworkAddressList.copyInto(getGameData().getDedicatedServerHostNetworkAddressList());

        if (blaze_strcmp(getGameProtocolVersionString(), GAME_PROTOCOL_VERSION_MATCH_ANY) == 0)
        {
            // use injecting user's protocol
            getGameData().setGameProtocolVersionString(joiningProtocolVersionString);
        }

        TRACE_LOG("[GameSessionMaster] Injecting host user(" << hostInfo.getUserInfo().getId() <<
            ") as host of VIRTUAL game(" << getGameId() << ").");

        StringBuilder buf;
        buf << " Using currently resettableGame: (" << hostGameId << ")";
        gGameManagerMaster->logDedicatedServerEvent("host injected", *this, buf.get());

        // GOS-28505 - fix to prevent NotifyGameSetup reaching injected host before NotifyGameRemoved
        if (gGameManagerMaster->getGameStorageTable().getSliverOwner().getOwnedSliver(GetSliverIdFromSliverKey(hostGameId)))
        {
            // send notification to topology host that the resettable game is removed
            // This is sent here and on the owning master; Client handles the redundant notificaiton gracefully.
            // Need to send both to eliminate race condition for remote injection where NotifyGameRemoved is received after
            // NotifyGameSetup, and the redundancy protects against a 'stuck' server in cases where this core master crashes.

            NotifyGameRemoved notifyGameRemoved;
            notifyGameRemoved.setGameId(hostGameId);
            notifyGameRemoved.setDestructionReason(HOST_INJECTION);
            gGameManagerMaster->sendNotifyGameRemovedToUserSessionById(getTopologyHostSessionId(), &notifyGameRemoved);
        }
        // end GOS-28507 fix

        // subscribe game members to the topology host session
        subscribePlayerRoster(getDedicatedServerHostSessionId(), ACTION_ADD);

        // associate the host's session with the game
        BlazeRpcError err = ERR_OK;
        err = gUserSessionManager->insertBlazeObjectIdForSession(hostInfo.getSessionId(), getBlazeObjectId());
        if (err != ERR_OK)
        {
            WARN_LOG("[GameSessionMaster].injectHost: " << (ErrorHelp::getErrorName(err)) << " error adding host session " << hostInfo.getSessionId() << " to game " << getGameId());
        }

        // Notify the injected host of the game.
        GameSetupReason setupReason;
        setupReason.switchActiveMember(GameSetupReason::MEMBER_DATALESSSETUPCONTEXT);
        setupReason.getDatalessSetupContext()->setSetupContext(HOST_INJECTION_SETUP_CONTEXT);
        NotifyGameSetup notifyJoinGame;
        initNotifyGameSetup(notifyJoinGame, setupReason, false, getDedicatedServerHostSessionId(), getDedicatedServerHostInfo().getConnectionGroupId(), common);
        gGameManagerMaster->sendNotifyGameSetupToUserSessionById(getDedicatedServerHostSessionId(), &notifyJoinGame);

        // send notification to initiate network connections to players in game session
        //send notification about the game name change to game members
        GameSetupReason initConnectionsSetupReason;
        initConnectionsSetupReason.switchActiveMember(GameSetupReason::MEMBER_DATALESSSETUPCONTEXT);
        initConnectionsSetupReason.getDatalessSetupContext()->setSetupContext(HOST_INJECTION_SETUP_CONTEXT);
        NotifyGameSetup notifyInitiateConnections;
        initNotifyGameSetup(notifyInitiateConnections, initConnectionsSetupReason, false);
        notifyInitiateConnections.getGameData().getPersistedGameIdSecret().setData(nullptr, 0);

        const PlayerRoster::PlayerInfoList& notifyPlayerList = getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        if (getPresenceDisabledList().empty())
        {
            gGameManagerMaster->sendNotifyJoiningPlayerInitiateConnectionsToUserSessionsById(notifyPlayerList.begin(), notifyPlayerList.end(), PlayerInfo::SessionIdGetFunc(), &notifyInitiateConnections);
        }
        else
        {
            UserSessionIdList presenceDisabledSessions;
            for (const auto& playerInfo : notifyPlayerList)
            {
                if (isPresenceDisabledForPlatform(playerInfo->getPlatformInfo().getClientPlatform()))
                    gGameManagerMaster->sendNotifyJoiningPlayerInitiateConnectionsToUserSessionById(playerInfo->getPlayerSessionId(), &notifyInitiateConnections);
                else
                    presenceDisabledSessions.push_back(playerInfo->getPlayerSessionId());
            }
            if (!presenceDisabledSessions.empty())
            {
                notifyInitiateConnections.getGameData().setPresenceMode(PRESENCE_MODE_NONE);
                gGameManagerMaster->sendNotifyJoiningPlayerInitiateConnectionsToUserSessionsById(presenceDisabledSessions.begin(), presenceDisabledSessions.end(), UserSessionIdIdentityFunc(), &notifyInitiateConnections);
            }
        }

        return ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief ejects the host user as the topology host of the dedicated server game, and
        returns the game to the virtual state.

        \param[in] hostSessionId - the hosts user session id
        \param[in] hostBlazeId - the hosts user id
        \param[in] replaceHost - should the host be replaced with a different server
    ***************************************************************************************************/
    void GameSessionMaster::ejectHost(UserSessionId hostSessionId, BlazeId hostBlazeId, bool replaceHost /*= false */ )
    {
        // If this is a non-virtual game, just 
        if (!replaceHost && !getGameSettings().getVirtualized())
        {
            destroyGame(HOST_LEAVING);   // Technically, the reason may be HOST_INJECTION_FAILED in some cases, but the end result is the same. 
            return;
        }

        gameFinished();

        gGameManagerMaster->logDedicatedServerEvent("host ejected", *this);

        // send last snapshot before clearing/reseting game's data
        updateFinalExternalSessionProperties();

        // reset for next match
        mGameDataMaster->setGameFinished(false);

        if (!replaceHost)
        {
            cleanUpMatchmakingQosDataMap();
            removeAllPlayers(HOST_EJECTED);
        }

        // need to change game state first so topology host info update is processed
        setGameState(INACTIVE_VIRTUAL);
        
        mMetricsHandler.onHostEjected();

        TRACE_LOG("[GameSessionMaster] Ejecting topology host(" << hostBlazeId <<
            ") as host of VIRTUAL game(" << getGameId() << ").");

        // Remove this host as an admin.
        removeAdminPlayer(hostBlazeId, INVALID_BLAZE_ID, INVALID_CONNECTION_GROUP_ID); // all players are already removed from the game. No new admin to choose.

        GameReportingId gameReportId = gGameManagerMaster->getNextGameReportingId();
        if (gameReportId == GameManager::INVALID_GAME_REPORTING_ID)
        {
            WARN_LOG("[GameSessionMaster].ejectHost failed to generate a new valid game reporting id for game(" << getGameId() );
            //note continue on below to ensure notifications and clean up old game reporting id via setGameReportingId() etc.
            //we will fail at the next injection attempt if there is no game reporting id available.
        }

        // Get a new game reporting id.
        setGameReportingId(gameReportId);

        // return game protocol version to its default/match any, in case last injection switched it
        getGameData().setGameProtocolVersionString(mGameDataMaster->getInitialGameProtocolVersionString());
        eastl::hash<const char8_t*> stringHash;
        getGameData().setGameProtocolVersionHash(stringHash(mGameDataMaster->getInitialGameProtocolVersionString()));

        // Clear the old hosts network information.
        getGameData().getTopologyHostNetworkAddressList().clear();
        getGameData().getDedicatedServerHostNetworkAddressList().clear();

        // Keep the 1st party data and groups if the host is just swapping
        if (!replaceHost)
        {
            // clear any game groups from previous games
            mGameData->getSingleGroupMatchIds().clear();
        }

        getGameData().getGameSettings().clearLockedAsBusy();

        if (mGameDataMaster->getLockedAsBusyTimeout() != 0)
        {
            gGameManagerMaster->getLockedAsBusyTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setLockedAsBusyTimeout(0);
        }

        // set this to false so the next host can finalize game creation
        mGameDataMaster->setIsGameCreationFinalized(false);

        // clear topology host
        mPlayerConnectionMesh.removeEndpoint(getGameData().getDedicatedServerHostInfo().getConnectionGroupId(), getGameData().getDedicatedServerHostInfo().getPlayerId(), HOST_EJECTED);
        // clear the connection slot for the host, assumes dedicated server host is not a MLU session.
        markConnectionSlotOpen(getGameData().getDedicatedServerHostInfo().getConnectionSlotId(), getGameData().getDedicatedServerHostInfo().getConnectionGroupId());
        setHostInfo(nullptr);
        // need to ensure that the next joining player doesn't take the host's spot.
        markConnectionSlotOccupied(ON_GAME_CREATE_HOST_SLOT_ID);

        // disassociate the host's session with the game
        BlazeRpcError err = ERR_OK;
        err = gUserSessionManager->removeBlazeObjectIdForSession(hostSessionId, getBlazeObjectId());
        if (err != ERR_OK)
        {
            WARN_LOG("[GameSessionMaster].ejectHost: " << (ErrorHelp::getErrorName(err)) << " error removing host session " << hostSessionId << " from game " << getGameId());
        }

        // Remove the topology hosts user session as associated with the game.
        gGameManagerMaster->removeUserSessionGameInfo(hostSessionId, *this);

        if (!replaceHost)
        {
            // for debug-ability clear external session data, so next injection uses new values (won't reuse)
            // Only Clear external data for cases where the Players are ALL gone.  (This makes sure that Disconnect Reservation players will stick around.)
            if (mPlayerRoster.getRosterPlayerCount() == 0)
                clearExternalSessionData();
        }
        else
        {
            mPlayerRoster.initPlayerStatesForHostReinjection();

            // Pick a player as joining user for host injection.
            PlayerRoster::PlayerInfoList players = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_CONNECTING_PLAYERS);
            size_t numPlayers = players.size();
            size_t playerIndex = 0;

            // Technically, if the randomly chosen player is disconnected, then the others will not have a chance to search, and the game will be destroyed.
            if (numPlayers > 0) 
            {
                playerIndex = Random::getRandomNumber(numPlayers);
                PlayerInfo* joiningPlayer = players.at(playerIndex);

                JoinGameMasterRequest joinGameRequest;
                joiningPlayer->getUserInfo().copyInto(joinGameRequest.getUserJoinInfo().getUser());

                gGameManagerMaster->addHostInjectionJob(*this, joinGameRequest);
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief removes players and completes clean up of a failed remote host injection attempt where no
            host was found.

    ***************************************************************************************************/
    void GameSessionMaster::cleanUpFailedRemoteInjection()
    {
        // we didn't change the active metrics or change the game state, so most metrics don't need an update

        // In the case of a tournaments Game, we want to keep all existing players in the Game, but push the player that initiated the injection back to a reserved state.
        if (getTournamentId()[0] == '\0')
        {
            removeAllPlayers(HOST_INJECTION_FAILED);
        }
        else
        {
            PlayerRoster::PlayerInfoList playerList = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PLAYERS);
            PlayerRemovalContext playerRemovalContext(HOST_INJECTION_FAILED, 0, nullptr);

            // removePlayer invalidates the iterator to playerList so just remove the first player every time.
            for (PlayerRoster::PlayerInfoList::iterator i = playerList.begin(), e = playerList.end(); i != e; ++i)
            {
                PlayerInfoMaster* player = static_cast<PlayerInfoMaster*>(*i);
                PlayerId playerId = (player != nullptr) ? player->getPlayerId() : 0;

                RemovePlayerMasterError::Error err = reserveForDisconnectedPlayer(player, HOST_INJECTION_FAILED);
                TRACE_LOG("[GameSessionMaster].cleanUpFailedRemoteInjection: Reserved player(" << playerId << ") for game("
                    << getGameId() << ") with error: " << (ErrorHelp::getErrorName((BlazeRpcError)err)));
            }
        }

        mMetricsHandler.calcPlayerSlotUpdatedMetrics(getTotalParticipantCapacity(), 0, getTotalSpectatorCapacity(), 0);

        setGameState(INACTIVE_VIRTUAL);

        GameReportingId gameReportId = gGameManagerMaster->getNextGameReportingId();
        if (gameReportId == GameManager::INVALID_GAME_REPORTING_ID)
        {
            WARN_LOG("[GameSessionMaster].cleanUpFailedRemoteInjection failed to generate a new valid game reporting id for game(" << getGameId() << ". No game reporting events will be reporting without a valid game reporting id.");
            //note continue on below to ensure notifications and clean up old game reporting id via setGameReportingId() etc.
            //we will fail at the next injection attempt if there is no game reporting id available.
        }

        // Get a new game reporting id.
        setGameReportingId(gameReportId);

        // return game protocol version to its default/match any, in case last injection switched it
        getGameData().setGameProtocolVersionString(mGameDataMaster->getInitialGameProtocolVersionString());
        eastl::hash<const char8_t*> stringHash;
        getGameData().setGameProtocolVersionHash(stringHash(mGameDataMaster->getInitialGameProtocolVersionString()));

        // Clear the old hosts network information.
        getGameData().getTopologyHostNetworkAddressList().clear();
        getGameData().getDedicatedServerHostNetworkAddressList().clear();
        // clear any game groups from previous games
        mGameData->getSingleGroupMatchIds().clear();

        getGameData().getGameSettings().clearLockedAsBusy();

        // set this to false so the next host can finalize game creation
        mGameDataMaster->setIsGameCreationFinalized(false);

        setHostInfo(nullptr);

        // If we get here for a non-virtualized game, we need to clean up the game itself, or it'll stick around in a INACTIVE_VIRTUAL state.
        if (!getGameSettings().getVirtualized())
        {
            destroyGame(HOST_LEAVING);
        }
    }

    ConnectionGroupId GameSessionMaster::getPlayerConnectionGroupId(PlayerId playerId) const
    {
        ConnectionGroupId connGroupId = INVALID_CONNECTION_GROUP_ID;
        PlayerInfo* playerInfo = getPlayerRoster()->getPlayer(playerId);
        if (playerInfo != nullptr)
        {
            connGroupId = playerInfo->getConnectionGroupId();
        }
        else if (hasDedicatedServerHost())
        {
            if (getDedicatedServerHostBlazeId() == playerId)
                connGroupId = getDedicatedServerHostInfo().getConnectionGroupId();
        }

        return connGroupId;
    }

    void GameSessionMaster::addToCensusEntry(CensusEntry& entry)
    {
        CensusEntrySet::insert_return_type ret = mCensusEntrySet.insert(&entry);
        if (ret.second)
        {
            ++entry.gameCount;
            PlayerRoster::PlayerInfoList players = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PARTICIPANTS);
            for (PlayerRoster::PlayerInfoList::const_iterator i = players.begin(), e = players.end(); i != e; ++i)
            {
                PlayerInfo* info = *i;
                PlayerInfoMaster* infoMaster = static_cast<PlayerInfoMaster*>(info);
                infoMaster->addToCensusEntry(entry);
            }
        }
    }

    void GameSessionMaster::removeFromCensusEntries()
    {
        // Even though we only add census entries for active participants we always try to remove entries from all roster players
        // for safety and future proofing to avoid dangling pointers in player objects we no longer mean to track.
        PlayerRoster::PlayerInfoList players = mPlayerRoster.getPlayers(PlayerRoster::ROSTER_PLAYERS);
        for (PlayerRoster::PlayerInfoList::const_iterator i = players.begin(), e = players.end(); i != e; ++i)
        {
            PlayerInfo* info = *i;
            PlayerInfoMaster* infoMaster = static_cast<PlayerInfoMaster*>(info);
            infoMaster->removeFromCensusEntries();
        }

        for (CensusEntrySet::const_iterator i = mCensusEntrySet.begin(), e = mCensusEntrySet.end(); i != e; ++i)
        {
            CensusEntry& entry = **i;
            if (entry.gameCount > 0)
                --entry.gameCount;
        }

        mCensusEntrySet.clear();
    }

    bool GameSessionMaster::isTeamPresentInGame(const TeamId& teamId) const
    {
        TeamIdVector::const_iterator teamsIter = getTeamIds().begin();
        TeamIdVector::const_iterator teamsEnd = getTeamIds().end();
        for(; teamsIter != teamsEnd; ++teamsIter)
        {
            if (teamId == (*teamsIter))
            {
                return true;
            }
        }

        return false;
    }

    

    
    JoinGameMasterError::Error GameSessionMaster::validateJoinTeamGameTeamCapacities(const TeamIndexRoleSizeMap &joiningTeamRoles, const BlazeId joiningPlayerId, uint16_t joiningPlayerCount, 
                                                                                     TeamIndex& outUnspecifiedTeamIndex, bool targetPlayerInQueue) const
    {
        // GM_AUDIT : wouldn't we need to check that *all* the joining players have reservations?
        // if the player already has a reservation, it doesn't matter if the team is already full
        bool hasActiveReservation = false;
        PlayerInfoMaster* existingPlayer = mPlayerRoster.getRosterPlayer(joiningPlayerId);  // Queued reservations are not allowed to skip the capacity checks. 
        if (existingPlayer != nullptr)  
        {
            // an existing player is ok, if it's just the playerReservation
            hasActiveReservation = (existingPlayer->isReserved());
        }

        // This checks if the joining player count would exceed the max member count of the game:
        JoinGameMasterError::Error retError = JoinGameMasterError::ERR_OK;
        if (!hasActiveReservation && ((joiningPlayerCount + mPlayerRoster.getParticipantCount()) > getTotalParticipantCapacity()))
        {
            TRACE_LOG("[GameSessionMaster].validateJoinTeamGameTeamCapacities: playerId " << joiningPlayerId << " unable to join gameId "
                << getGameId() << " because the game is full.");
            if (retError == JoinGameMasterError::ERR_OK)
            {
                retError = JoinGameMasterError::GAMEMANAGER_ERR_PARTICIPANT_SLOTS_FULL;
            }
        }

        // Check for invalid roles:
        TeamIndexRoleSizeMap::const_iterator teamIter = joiningTeamRoles.begin();
        TeamIndexRoleSizeMap::const_iterator teamEnd = joiningTeamRoles.end();
        for (; teamIter != teamEnd; ++teamIter)
        {
            
            TeamIndex joiningTeamIndex = teamIter->first;
            const RoleSizeMap &joiningRoles = teamIter->second;
            joiningPlayerCount = 0;  // Reset the joiningPlayerCount

            RoleSizeMap::const_iterator roleIter = joiningRoles.begin();
            RoleSizeMap::const_iterator roleEnd = joiningRoles.end();
            for (; roleIter != roleEnd; ++roleIter)
            {
                // We only want the joiningPlayerCount for the current team:
                joiningPlayerCount += roleIter->second;

                if (blaze_stricmp(roleIter->first, GameManager::PLAYER_ROLE_NAME_ANY) != 0)
                {
                    // Check that the role is valid:
                    if (getRoleInformation().getRoleCriteriaMap().find(roleIter->first) == getRoleInformation().getRoleCriteriaMap().end())
                    {
                        TRACE_LOG("[GameSessionMaster].validateJoinTeamGameTeamCapacities: playerId " << joiningPlayerId << " unable to join gameId "
                            << getGameId() << " because it does not have role '" << roleIter->first.c_str() << "'.");
                        if (isQueueableJoinError(retError) || retError == JoinGameMasterError::ERR_OK)
                        {
                            retError = JoinGameMasterError::GAMEMANAGER_ERR_ROLE_NOT_ALLOWED;
                        }
                    }
                }
            }

            if (joiningTeamIndex == TARGET_USER_TEAM_INDEX)
            {
                // If the target player is in a queue currently, he may be targeting someone else (recursive case)
                if (targetPlayerInQueue)
                {
                    if (retError == JoinGameMasterError::ERR_OK)
                        retError = JoinGameMasterError::GAMEMANAGER_ERR_TEAM_FULL;

                    continue;
                }

                // joiningTeamIndex should have been resolved at this point to point to target users team
                ERR_LOG("[GameSessionMaster].validateJoinTeamGameTeamCapacities: Player(" << joiningPlayerId << ") unable to join Game(" << getGameId()
                    << ") because team index (TARGET_USER_TEAM_INDEX) should have already been replaced with the looked up index.");
                return JoinGameMasterError::GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
            }
            else if (joiningTeamIndex != UNSPECIFIED_TEAM_INDEX)
            {
                if (joiningTeamIndex < getTeamCount())
                {
                    if (!hasActiveReservation)
                    {
                        // only do these checks if the player isn't already reserved
                        if ( (joiningPlayerCount + mPlayerRoster.getTeamSize(joiningTeamIndex)) > getTeamCapacity() )
                        {
                            // user has picked a valid team index, but it is a full team
                            TRACE_LOG("[GameSessionMaster].validateJoinTeamGameTeamCapacities: playerId " << joiningPlayerId << " unable to join gameId "
                                << getGameId() << " because team index " << joiningTeamIndex << " is full.");

                            if (retError == JoinGameMasterError::ERR_OK)
                            {
                                retError = JoinGameMasterError::GAMEMANAGER_ERR_TEAM_FULL;
                            }
                        }

                        BlazeRpcError err = checkJoinabilityByTeamIndex(joiningRoles, joiningTeamIndex);
                        if (err != ERR_OK)
                        {
                            TRACE_LOG("[GameSessionMaster].validateJoinTeamGameTeamCapacities: playerId " << joiningPlayerId << " unable to join gameId "
                                << getGameId() << " because requested role in team index " << joiningTeamIndex << " is full.");
                            JoinGameMasterError::Error tempError = JoinGameMasterError::commandErrorFromBlazeError(err);
                            if ((isQueueableJoinError(retError) && !isQueueableJoinError(tempError)) || retError == JoinGameMasterError::ERR_OK)
                            {
                                retError = tempError;
                            }
                        }
                    }
                }
                else
                {
                    // tried to join team with an out of range team index
                    TRACE_LOG("[GameSessionMaster].validateJoinTeamGameTeamCapacities: Player(" << joiningPlayerId << ") unable to join Game(" << getGameId()
                        << ") because team index(" << joiningTeamIndex << ") is not a valid; outside of team count(" << getTeamCount() << ").");
                    if (isQueueableJoinError(retError) || retError == JoinGameMasterError::ERR_OK)
                    {
                        retError = JoinGameMasterError::GAMEMANAGER_ERR_TEAM_NOT_ALLOWED;
                    }
                }
            }
            else
            {
                if (!hasActiveReservation)
                {
                    // Team Index is set when a team is found.
                    TeamSelectionCriteria teamSelectionCriteria;
                    teamSelectionCriteria.setRequiredSlotCount(joiningPlayerCount);

                    BlazeRpcError openTeamErr = findOpenTeam(teamSelectionCriteria, joiningRoles, ANY_TEAM_ID, nullptr, joiningTeamIndex);
                    if (openTeamErr != ERR_OK)
                    {
                        TRACE_LOG("[GameSessionMaster].validateJoinTeamGameTeamCapacities: Player(" << joiningPlayerId << ") unable to join game("
                            << getGameId() << ") because no team has room.");
                        JoinGameMasterError::Error tempError = JoinGameMasterError::commandErrorFromBlazeError(openTeamErr);
                        if ((isQueueableJoinError(retError) && !isQueueableJoinError(tempError)) || retError == JoinGameMasterError::ERR_OK)
                        {
                            retError = tempError;
                        }
                    }

                    outUnspecifiedTeamIndex = joiningTeamIndex;
                }
                else
                {
                    // Just update the outUnspecifiedTeamIndex with the reserved teamIndex
                    outUnspecifiedTeamIndex = existingPlayer->getTeamIndex();
                }
            }

            // Check the whole group against the multi-role criteria. (Needed because each player might individually pass the criteria, but fail together).
            EntryCriteriaName failedEntryCriteria;
            if (!mMultiRoleEntryCriteriaEvaluator.evaluateEntryCriteria(getPlayerRoster(), joiningRoles, joiningTeamIndex, failedEntryCriteria))
            {
                TRACE_LOG("[GameSessionMaster].validateJoinTeamGameTeamCapacities: Player Group failed multirole entry criteria to enter game(" << getGameId() << "), failedEntryCriteria(" << failedEntryCriteria.c_str() << ")");
                if (retError == JoinGameMasterError::ERR_OK)
                {
                    retError = JoinGameMasterError::GAMEMANAGER_ERR_MULTI_ROLE_CRITERIA_FAILED;
                }
            }
        }

        return retError;
    }

    void GameSessionMaster::updateGameName(const GameName &name)
    {
        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        getGameData().setGameName(name);

        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());
    }

    void GameSessionMaster::setPresenceMode(PresenceMode presenceMode)
    {
        getGameData().setPresenceMode(presenceMode);
        changePresenceState();
    }

    /*! ************************************************************************************************/
    /*! \brief submit player removed game session event
    ***************************************************************************************************/
    void GameSessionMaster::submitEventPlayerLeftGame(const PlayerInfoMaster& player,
        const GameSessionMaster& gameSession, const PlayerRemovalContext& removeContext)
    {
        // If the player was queued, we send a dequeue to match the queued event
        // received when they joined.
        if (player.isInQueue())
        {
            submitEventDequeuePlayer(player);
            return;
        }

        PlayerLeftGameSession gameSessionEvent;
        bool isPlatformHost = ( hasDedicatedServerHost() && (getPlatformHostInfo().getPlayerId() == player.getPlayerId()));
        gameSessionEvent.setPlayerId(player.getPlayerId());
        gameSessionEvent.setPersonaName(player.getPlayerName());
        gameSessionEvent.setIsPlatformHost(isPlatformHost);
        gameSessionEvent.setGameId(getGameId());
        gameSessionEvent.setGameReportingId(getGameReportingId());
        gameSessionEvent.setGameName(getGameName());
        gameSessionEvent.setSessionId(player.getPlayerSessionId());
        gameSessionEvent.setRemovalReason(GameManager::PlayerRemovedReasonToString(removeContext.mPlayerRemovedReason));
        gameSessionEvent.setTopology(Blaze::GameNetworkTopologyToString(getGameNetworkTopology()));
        gameSessionEvent.setPlayerRemovedTitleContext(removeContext.mPlayerRemovedTitleContext);
        gameSessionEvent.setTitleContextString(removeContext.mTitleContextString);
        gameSessionEvent.setJoinMethod(player.getJoinMethod());
        gameSessionEvent.setGameState(getGameState());
        gameSessionEvent.setPlayerCount(mPlayerRoster.getRosterPlayerCount() - 1); //the event is always submitted before the player is removed from the roster, so we - 1
        getGameAttribs().copyInto(gameSessionEvent.getGameAttribs());
        gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_PLAYERLEFTGAMESESSIONEVENT), gameSessionEvent, true);
    }

    void GameSessionMaster::submitEventDequeuePlayer(const PlayerInfoMaster& player)
    {
        PlayerQueuedGameSession gameSessionEvent;
        gameSessionEvent.setPlayerId(player.getPlayerId());
        gameSessionEvent.setGameId(getGameId());
        gameSessionEvent.setGameReportingId(getGameReportingId());
        gameSessionEvent.setGameName(getGameName());
        gameSessionEvent.setPersonaName(player.getPlayerName());
        mPlayerRoster.populateQueueEvent(gameSessionEvent.getQueuedPlayers());
        gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_PLAYERDEQUEUEDGAMESESSIONEVENT), gameSessionEvent, true);
    }

    void GameSessionMaster::sendGameStateUpdate()
    {
        NotifyGameStateChange nGameStateChange;
        nGameStateChange.setGameId(getGameId());
        nGameStateChange.setNewGameState(getGameState());
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        gGameManagerMaster->sendNotifyGameStateChangeToUserSessionsById(itPair.begin(), itPair.end(), UserSessionIdIdentityFunc(), &nGameStateChange );
    }

    void GameSessionMaster::initNotifyGameSetup(NotifyGameSetup &notifyJoinGame, const GameSetupReason &reason, bool performQosValidation, UserSessionId notifySessionId, ConnectionGroupId notifyConnectionGroupId, ClientPlatformType notifyPlatform)
    {
        // clear the games persisted id secret, to prevent hacked clients from being able to use it.
        // Topology host user gets to see it in case they had the server create their persisted id.
        // We don't send to dedicated server host, because it will be the same except for failover, and a failover server won't need the key.
        if ((notifySessionId != INVALID_USER_SESSION_ID) && ((notifySessionId != getTopologyHostSessionId())))
        {
            TRACE_LOG("[GameSessionMaster].initNotifyGameSetup() Clearing persisted gameId secret for ("
                << notifySessionId << ") NotifyGameSetup, TopologyHostSessionId is (" << getTopologyHostSessionId() << ").");
            // add game data to notification
            notifyJoinGame.setGameData(getGameData());
            notifyJoinGame.getGameData().getPersistedGameIdSecret().setData(nullptr, 0);
        }
        else
        {
            // topology/dedicated host, needs the secret
            // we prefer to use a referenced version of the replicated game data, so if there's either no persisted game id
            // or the secret is still intact, we send that down
            if(!getGameData().getGameSettings().getEnablePersistedGameId() ||
                ((getGameData().getPersistedGameId()[0] != '\0') && (getGameData().getPersistedGameIdSecret().getData() != nullptr)))
            {
                // persisted id unset or secret still intact
                TRACE_LOG("[GameSessionMaster].initNotifyGameSetup() Sending referenced ReplicatedGameData for topology host NotifyGameSetup, TopologyHostSessionId is ("
                    << getTopologyHostSessionId() << ").");
                // add game data to notification
                notifyJoinGame.setGameData(getGameData());
            }
            else
            {
                // persisted ID enabled, secret gone, only likely in remote host injection case
                TRACE_LOG("[GameSessionMaster].initNotifyGameSetup() Sending copied ReplicatedGameData for topology host NotifyGameSetup, TopologyHostSessionId is ("
                    << getTopologyHostSessionId() << ").");
                // copy game data into notification
                getGameData().copyInto(notifyJoinGame.getGameData());

                // copy cached secret into game data copy
                getGameData().getPersistedGameIdSecret().copyInto(notifyJoinGame.getGameData().getPersistedGameIdSecret());
            }
        }

        if (!getPresenceDisabledList().empty() && (notifyPlatform != INVALID || notifySessionId != INVALID_USER_SESSION_ID))
        {
            if (notifyPlatform == INVALID)
                notifyPlatform = gUserSessionManager->getPlatformInfo(notifySessionId).getClientPlatform();

            if (isPresenceDisabledForPlatform(notifyPlatform))
                notifyJoinGame.getGameData().setPresenceMode(PRESENCE_MODE_NONE);
        }
        
        bool hideNetworkAddress = (notifySessionId != getDedicatedServerHostSessionId()) && enableProtectedIP();
        ConnectionGroupId dedicatedHostConnectionGroupId = isPlayerHostedTopology(getGameNetworkTopology()) ? getDedicatedServerHostInfo().getConnectionGroupId() : INVALID_CONNECTION_GROUP_ID;
        // add roster members to the notification.
        const PlayerRoster::PlayerInfoList &notifyPlayerRoster = mPlayerRoster.getPlayers(PlayerRoster::ROSTER_PLAYERS);
        for ( PlayerRoster::PlayerInfoList::const_iterator notifyPlayerIter = notifyPlayerRoster.begin(), notifyPlayerEnd = notifyPlayerRoster.end();
            notifyPlayerIter != notifyPlayerEnd ; ++notifyPlayerIter )
        {
            PlayerInfo *player = *notifyPlayerIter;
            ReplicatedGamePlayer* repPlayer = notifyJoinGame.getGameRoster().pull_back();

            
            bool hideCurrentPlayerNetworkAddress = hideNetworkAddress;
            if (enablePartialProtectedIP() && (notifySessionId != getDedicatedServerHostSessionId()))
            {
                hideCurrentPlayerNetworkAddress = (notifyPlatform != player->getClientPlatform());
            }

            player->fillOutReplicatedGamePlayer(*repPlayer, ((player->getConnectionGroupId() != dedicatedHostConnectionGroupId && player->getConnectionGroupId() != notifyConnectionGroupId) && hideCurrentPlayerNetworkAddress));
        }

        const PlayerRoster::PlayerInfoList &notifyPlayerQueue = mPlayerRoster.getPlayers(PlayerRoster::QUEUED_PLAYERS);
        for (PlayerRoster::PlayerInfoList::const_iterator queueIter = notifyPlayerQueue.begin(), queueEnd = notifyPlayerQueue.end();
            queueIter != queueEnd ; ++queueIter )
        {
            PlayerInfo *player = *queueIter;
            ReplicatedGamePlayer* repPlayer = notifyJoinGame.getGameQueue().pull_back();


            bool hideCurrentPlayerNetworkAddress = hideNetworkAddress;
            if (enablePartialProtectedIP() && (notifySessionId != getDedicatedServerHostSessionId()))
            {
                hideCurrentPlayerNetworkAddress = (notifyPlatform != player->getClientPlatform());
            }

            player->fillOutReplicatedGamePlayer(*repPlayer, (player->getConnectionGroupId() != dedicatedHostConnectionGroupId && player->getConnectionGroupId() != notifyConnectionGroupId) && hideCurrentPlayerNetworkAddress);
        }

        if (hideNetworkAddress && !isDedicatedHostedTopology(getGameNetworkTopology()))
        {
            bool hideTopologyHostNetworkAddress = hideNetworkAddress;
            if (enablePartialProtectedIP() && (notifySessionId != getTopologyHostSessionId()) && (getTopologyHostUserInfo().getConnectionGroupId() != notifyConnectionGroupId))
            {
                hideTopologyHostNetworkAddress = (getTopologyHostUserInfo().getUserInfo().getPlatformInfo().getClientPlatform() != notifyPlatform);
            }

            // if it's not a dedicated server game, we need to blank out the topology host address list.
            if (hideTopologyHostNetworkAddress)
            {
                notifyJoinGame.getGameData().getTopologyHostNetworkAddressList().clear();
            }
        }

        reason.copyInto(notifyJoinGame.getGameSetupReason());

        notifyJoinGame.setIsLockableForPreferredJoins(gGameManagerMaster->getLockGamesForPreferredJoins());
        notifyJoinGame.setQosTelemetryInterval(gGameManagerMaster->getConfig().getGameSession().getQosTelemetryInterval());
        // this tells the user if they should perform QoS as they begin connecting.
        notifyJoinGame.setPerformQosValidation(performQosValidation);
        // this specifies the QoS pattern to use if subsequent joiners require a Qos validation
        QosValidationCriteriaMap::const_iterator qosIter = gGameManagerMaster->getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().find(getGameNetworkTopology());
        if ( qosIter !=
            gGameManagerMaster->getConfig().getMatchmakerSettings().getRules().getQosValidationRule().getConnectionValidationCriteriaMap().end())
        {
            qosIter->second->getQosSettings().copyInto(notifyJoinGame.getQosSettings());
        }
        else // this topology doesn't do qos validation for MM, supply settings that disable QoS checking ability
        {
            notifyJoinGame.getQosSettings().setDurationMs(0);
            notifyJoinGame.getQosSettings().setIntervalMs(0);
            notifyJoinGame.getQosSettings().setPacketSize(0);
        }

        // put the mode attrib in the notification so the SDK can know how to access 'mode' directly
        notifyJoinGame.setGameModeAttributeName(gGameManagerMaster->getConfig().getGameSession().getGameModeAttributeName());
    }

        /*! ************************************************************************************************/
    /*! \brief Send NotifyPlayerJoining to each member of the game (as well as any dedicated host client)
        \param[in] joiningPlayerData the replicated player data for the joining player
    ***************************************************************************************************/
    void GameSessionMaster::notifyExistingMembersOfJoiningPlayer(const PlayerInfo &joiningPlayer, PlayerRoster::RosterType rosterType, const GameSetupReason& reason)
    {
        // subscribe joining player to the user extended data (UED) of the game's active players
        // WARNING: order here is important (we need to subscribe to UED before we notify anyone of the game join.
        subscribePlayerRoster(joiningPlayer.getPlayerSessionId(), ACTION_ADD);

        bool performQosValidation = isPlayerInMatchmakingQosDataMap(joiningPlayer.getPlayerId());

        NotifyPlayerJoining notifyPlayerJoining;
        notifyPlayerJoining.setGameId(getGameData().getGameId());
        notifyPlayerJoining.setValidateQosForJoiningPlayer(performQosValidation);
        joiningPlayer.fillOutReplicatedGamePlayer(notifyPlayerJoining.getJoiningPlayer(), enableProtectedIP());
        
        // build out our session ids before we decide which notification to send
        // sessionIdSetIdentityFuncProtectedIP contains the session ids that won't get a notification w/ masked IP addresses
        SessionIdSetIdentityFunc sessionIdSetIdentityFuncProtectedIP;
        // sessionIdSetIdentityFuncForUnprotectedIP contains the session ids that won't get a notification w/ unmasked IP address
        SessionIdSetIdentityFunc sessionIdSetIdentityFuncForUnprotectedIP;

        chooseProtectedIPRecipients(joiningPlayer, sessionIdSetIdentityFuncProtectedIP, sessionIdSetIdentityFuncForUnprotectedIP);
        

        //Notify new player join to the existing members
        GameSessionMaster::SessionIdIteratorPair itPair = getSessionIdIteratorPair();
        if (rosterType == PlayerRoster::PLAYER_ROSTER)
        {
            if (enableProtectedIP())
            {
                gGameManagerMaster->sendNotifyPlayerJoiningToUserSessionsById(itPair.begin(false), itPair.end(), sessionIdSetIdentityFuncProtectedIP, &notifyPlayerJoining);

                //Add address back when sending to a host or same-platform players
                joiningPlayer.getNetworkAddress()->copyInto(notifyPlayerJoining.getJoiningPlayer().getNetworkAddress());

                if (enablePartialProtectedIP())
                {
                    //Add address back when sending to a host or same-platform players
                    gGameManagerMaster->sendNotifyPlayerJoiningToUserSessionsById(itPair.begin(true), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
                }
                else // dedicated server or CCS_MODE_HOSTED_ONLY
                { 
                    gGameManagerMaster->sendNotifyPlayerJoiningToUserSessionById(getDedicatedServerHostSessionId(), &notifyPlayerJoining);
                }
            }
            else // not protecting ip addresses
            {
                gGameManagerMaster->sendNotifyPlayerJoiningToUserSessionsById(itPair.begin(), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
            }
        }
        else
        {
            if (enableProtectedIP())
            {
                gGameManagerMaster->sendNotifyPlayerJoiningQueueToUserSessionsById(itPair.begin(false), itPair.end(), sessionIdSetIdentityFuncProtectedIP, &notifyPlayerJoining);

                //Add address back when sending to a host or same-platform players
                joiningPlayer.getNetworkAddress()->copyInto(notifyPlayerJoining.getJoiningPlayer().getNetworkAddress());
                if (enablePartialProtectedIP())
                {
                    //Add address back when sending to a host or same-platform players
                    gGameManagerMaster->sendNotifyPlayerJoiningToUserSessionsById(itPair.begin(true), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
                }
                else // dedicated server or CCS_MODE_HOSTED_ONLY
                {
                    gGameManagerMaster->sendNotifyPlayerJoiningQueueToUserSessionById(getDedicatedServerHostSessionId(), &notifyPlayerJoining);
                }
            }
            else // not protecting ip addresses
            {
                gGameManagerMaster->sendNotifyPlayerJoiningQueueToUserSessionsById(itPair.begin(), itPair.end(), sessionIdSetIdentityFuncForUnprotectedIP, &notifyPlayerJoining);
            }
        }

        //Notify the game setup to the player itself.
        NotifyGameSetup notifyJoinGame;
        initNotifyGameSetup(notifyJoinGame, reason, performQosValidation, joiningPlayer.getPlayerSessionId(), joiningPlayer.getConnectionGroupId(), joiningPlayer.getPlatformInfo().getClientPlatform());
        gGameManagerMaster->sendNotifyGameSetupToUserSessionById(joiningPlayer.getPlayerSessionId(), &notifyJoinGame);
    }

    // localConnGrpId is the replicated-to connection group id
    void GameSessionMaster::fillOutClientSpecificHostedConnectivityInfo(HostedConnectivityInfo& replicatedInfo, 
        ConnectionGroupId localConnGrpId, ConnectionGroupId remoteConnGrpId)
    {
        const HostedConnectivityInfo* info = mPlayerConnectionMesh.getHostedConnectivityInfo(localConnGrpId, remoteConnGrpId);
        if (info != nullptr)
        {
            info->copyInto(replicatedInfo);
        }
    }

    void GameSessionMaster::notifyHostedConnectivityAvailable(ConnectionGroupId console1ConnGrpId, ConnectionGroupId console2ConnGrpId, const HostedConnectivityInfo& info)
    {
        eastl::list<PlayerInfo*> console1Players;
        eastl::list<PlayerInfo*> console2Players;

        const PlayerRoster::PlayerInfoList& activePlayers = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PLAYERS);       
        for (PlayerInfo *player : activePlayers)
        {
            if (player != nullptr)
            {
                if (player->getConnectionGroupId() == console1ConnGrpId)
                {
                    console1Players.push_back(player);
                }
                if (player->getConnectionGroupId() == console2ConnGrpId)
                {
                    console2Players.push_back(player);
                }
            }
        }

        // Handle MLU by sending messages to each MLU user, from each users on an MLU connection:
        for (auto& player1 : console1Players)
        {
            for (auto& player2 : console2Players)
            {
                // notify first console
                {
                    TRACE_LOG("[GameSessionMaster].notifyHostedConnectivityAvailable: Notify console (" << console1ConnGrpId << ") of hosted connectivity wrt to console(" << console2ConnGrpId << ")");
                    NotifyHostedConnectivityAvailable hostedConnInfo;
                    hostedConnInfo.setGameId(getGameData().getGameId());
                    hostedConnInfo.setRemotePlayerId(player2->getPlayerId());
                    fillOutClientSpecificHostedConnectivityInfo(hostedConnInfo.getHostedConnectivityInfo(), console1ConnGrpId, console2ConnGrpId);
                    gGameManagerMaster->sendNotifyHostedConnectivityAvailableToUserSessionById(player1->getPlayerSessionId(), &hostedConnInfo);
                }
                // notify second console
                {
                    TRACE_LOG("[GameSessionMaster].notifyHostedConnectivityAvailable: Notify console (" << console2ConnGrpId << ") of hosted connectivity wrt to console(" << console1ConnGrpId << ")");
                    NotifyHostedConnectivityAvailable hostedConnInfo;
                    hostedConnInfo.setGameId(getGameData().getGameId());
                    hostedConnInfo.setRemotePlayerId(player1->getPlayerId());
                    fillOutClientSpecificHostedConnectivityInfo(hostedConnInfo.getHostedConnectivityInfo(), console2ConnGrpId, console1ConnGrpId);
                    gGameManagerMaster->sendNotifyHostedConnectivityAvailableToUserSessionById(player2->getPlayerSessionId(), &hostedConnInfo);
                }
            }
        }
    }

    PlayerInfoMaster* GameSessionMaster::replaceExternalPlayer(UserSessionId newSessionId, const UserSession& newUserData)
    {
        return mPlayerRoster.replaceExternalPlayer(newSessionId, newUserData.getBlazeId(), newUserData.getPlatformInfo(), newUserData.getAccountLocale(), newUserData.getAccountCountry(),
            newUserData.getServiceName(), newUserData.getProductName());
    }

    /*! ************************************************************************************************/
    /*! \brief lock game for preferred joins. While locked blaze will block finding or joining the game by
        matchmaking or game browsing, for up to the server configured lock for preferred joins timeout.
        If all active players have called joinGameByUserList or preferredJoinOptOut in response to the lock,
        or the game becomes full, the lock is removed.
        \return true if new lock was started. false otherwise
    ***************************************************************************************************/
    bool GameSessionMaster::lockForPreferredJoins()
    {
        if (!gGameManagerMaster->getLockGamesForPreferredJoins())
            return false;

        // update pending reply set
        mGameDataMaster->getPendingPreferredJoinList().clear();
        const PlayerRoster::PlayerInfoList& players = mPlayerRoster.getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        for (PlayerRoster::PlayerInfoList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
        {
            insertToPendingPreferredJoinSet((*itr)->getPlayerId());
        }
        if (mGameDataMaster->getPendingPreferredJoinList().empty())
            return false;//no one to do

        TRACE_LOG("[GameSessionMaster] New preferred joins lock for " << (gGameManagerMaster->getLockGamesForPreferredJoinsTimeout().getSec()) << "s, for game id " << getGameId());

        // notify search slaves to override open to matchmaking or browse
        mGameData->setIsLockedForJoins(true);

        // add timer
        if (mGameDataMaster->getLockForPreferredJoinsTimeout() != 0)
        {
            gGameManagerMaster->getLockForJoinsTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setLockForPreferredJoinsTimeout(0);
        }

        TimeValue timeout = TimeValue::getTimeOfDay() + gGameManagerMaster->getLockGamesForPreferredJoinsTimeout();
        if (gGameManagerMaster->getLockForJoinsTimerSet().scheduleTimer(getGameId(), timeout))
            mGameDataMaster->setLockForPreferredJoinsTimeout(timeout);

        return true;
    }

    void GameSessionMaster::clearLockForPreferredJoinsAndSendUpdate(GameId gameId)
    {
        GameSessionMasterPtr gameSession = gGameManagerMaster->getWritableGameSession(gameId);
        if (gameSession != nullptr)
            gameSession->clearLockForPreferredJoins(true);
    }

    /*! \brief Clear game's lock for preferred joins, and cancel timer. */
    void GameSessionMaster::clearLockForPreferredJoins(bool sendUpdate /*= true*/)
    {
        bool wasLockedForPreferredjoins = isLockedForPreferredJoins();

        // ensure we always remove timer
        if (mGameDataMaster->getLockForPreferredJoinsTimeout() != 0)
        {
            gGameManagerMaster->getLockForJoinsTimerSet().cancelTimer(getGameId());
            mGameDataMaster->setLockForPreferredJoinsTimeout(0);
        }

        if (!wasLockedForPreferredjoins)
            return;

        // If sendUpdate is false, we may be clearing the lock in the destructor, and the replicated data may already be deleted.
        TRACE_LOG("[GameSessionMaster] Clear lock for preferred joins for game(" << (sendUpdate ? getGameId() : INVALID_GAME_ID) << ").");

        // notify search slaves to stop overriding open to matchmaking or browse
        if (sendUpdate)
        {
            mGameData->setIsLockedForJoins(false);
        }

        // update pending reply set
        mGameDataMaster->getPendingPreferredJoinList().clear();
    }

    /*! \brief insert player to the pending join game by user list set. Pre: player is active. */
    void GameSessionMaster::insertToPendingPreferredJoinSet(PlayerId playerId)
    {
        if (!gGameManagerMaster->getLockGamesForPreferredJoins())
            return;
        bool found = false;
        for (GameDataMaster::PendingPreferredJoinList::iterator i = mGameDataMaster->getPendingPreferredJoinList().begin(), e = mGameDataMaster->getPendingPreferredJoinList().end(); i != e; ++i)
        {
            if (*i == playerId)
            {
                found = true;
                break;
            }
        }
        if (!found)
            mGameDataMaster->getPendingPreferredJoinList().push_back(playerId);
        bool inserted = !found;
        TRACE_LOG("[GameSessionMaster] Insert of player(" << playerId << ") to pending preferred joins set for game(" << getGameId() << ") " << ((inserted == false)? "no-opped - player was already in the set." : "succeeded") << ". Current size of pending preferred joins set is " << mGameDataMaster->getPendingPreferredJoinList().size() << ", number of pending game dequeue is " << mGameDataMaster->getQueuePumpsPending());
    }

    /*! \brief remove player from pending reply set. Cancel the lock, if there's no more pending joins/opt-outs we're awaiting. */
    void GameSessionMaster::eraseFromPendingPreferredJoinSet(PlayerId playerId)
    {
        bool found = false;
        for (GameDataMaster::PendingPreferredJoinList::iterator i = mGameDataMaster->getPendingPreferredJoinList().begin(), e = mGameDataMaster->getPendingPreferredJoinList().end(); i != e; ++i)
        {
            if (*i == playerId)
            {
                found = true;
                mGameDataMaster->getPendingPreferredJoinList().erase(i);
                break;
            }
        }
        TRACE_LOG("[GameSessionMaster] Removal of player(" << playerId << ") from pending preferred joins set for game(" << getGameId() << ") " << ((!found)? "no-opped - player was already removed or not currently in the set." : "succeeded") << ". Remaining size of pending preferred joins set is " << mGameDataMaster->getPendingPreferredJoinList().size() << ", number of pending game dequeue is " << mGameDataMaster->getQueuePumpsPending());

        // If there's pending dequeues, don't cancel lock since the dequeues may add to the pending reply set.
        if (mGameDataMaster->getPendingPreferredJoinList().empty() && (mGameDataMaster->getQueuePumpsPending() == 0))
            clearLockForPreferredJoins();
    }

    bool GameSessionMaster::updatePromotabilityOfQueuedPlayers()
    {
        // assume no queued player is promotable until proven otherwise
        mGameDataMaster->setIsAnyQueueMemberPromotable(false);

        // only do this scan for admin managed queues, since Blaze managed queues promotes all eligible players atomically
        if ((gGameManagerMaster->getGameSessionQueueMethod() == GAME_QUEUE_ADMIN_MANAGED) && (mGameDataMaster->getQueuePumpsPending() > 0))
        {
            mGameDataMaster->setIsAnyQueueMemberPromotable(isAnyQueueMemberPromotable());
        }

        return mGameDataMaster->getIsAnyQueueMemberPromotable();
    }

    bool GameSessionMaster::isAnyQueueMemberPromotable()
    {
        // check to see if anyone in the queue is capable of being dequeued with their current role, slot and team
        const PlayerRoster::PlayerInfoList& qPlayerList = mPlayerRoster.getPlayers(PlayerRoster::QUEUED_PLAYERS);
        PlayerRoster::PlayerInfoList::const_iterator it = qPlayerList.begin();
        PlayerRoster::PlayerInfoList::const_iterator end = qPlayerList.end();
        for (; it != end; ++it)
        {
            const PlayerInfoMaster *qPlayer = (const PlayerInfoMaster*)*it;

            SlotType slotType = qPlayer->getSlotType();
            // dump next queued player into game if there are additional public slots
            // or if they are requesting the type of slot that was opened
            bool desiredSlotFull = isSlotTypeFull(slotType);
            // make sure we're checking the correct desired slot type
            bool publicSlotFull = qPlayer->isParticipant() ? isSlotTypeFull(SLOT_PUBLIC_PARTICIPANT) : isSlotTypeFull(SLOT_PUBLIC_SPECTATOR);
            if (!desiredSlotFull || !publicSlotFull)
            {
                SlotType suggestedSlotType = slotType;

                TeamIndex joiningTeamIndex = qPlayer->getTeamIndex();
                if (joiningTeamIndex == TARGET_USER_TEAM_INDEX)
                {
                    // User attempted to join target user's team.
                    PlayerInfoMaster* targetPlayerInfoMaster = mPlayerRoster.getPlayer(qPlayer->getTargetPlayerId());
                    if (targetPlayerInfoMaster != nullptr)
                    {
                        joiningTeamIndex = targetPlayerInfoMaster->getTeamIndex();
                    }
                    
                    // The target player may not have a valid team set:
                    if (joiningTeamIndex == TARGET_USER_TEAM_INDEX)
                    {
                        joiningTeamIndex = UNSPECIFIED_TEAM_INDEX;
                    }
                }

                TeamIndex unspecifiedTeamIndex = UNSPECIFIED_TEAM_INDEX;
                BlazeRpcError err = validateJoinGameCapacities(qPlayer->getRoleName(), slotType, joiningTeamIndex, qPlayer->getPlayerId(), suggestedSlotType, unspecifiedTeamIndex);
                if (err == ERR_OK)
                {
                    TRACE_LOG("[GameSessionMaster] Found (" << qPlayer->getPlayerId() << ") eligible for queue promotion in game(" << getGameId() 
                        << "), setting mGameData->getIsAnyQueueMemberPromotable() to 'true', number of pending game dequeues is " << mGameDataMaster->getQueuePumpsPending());
                    return true;
                }
            }
            else
            {
                if (canQueuedPlayerSwapWithReservedInGamePlayer(qPlayer))
                    return true;
            }
        }

        return false;

    }

    bool GameSessionMaster::canQueuedPlayerSwapWithReservedInGamePlayer(const PlayerInfoMaster *qPlayer, PlayerId* playerToDemote)
    {
        if (!qPlayer->isReserved())
        {
            // The ugly thing is that I really can't think of anything to do other than dropping each player and then doing the check:
            eastl::vector<PlayerInfoPtr> reservedPlayersInGame;
            const PlayerRoster::PlayerInfoList& players = getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PLAYERS);
            for (PlayerRoster::PlayerInfoList::const_iterator curPlayer = players.begin(); curPlayer != players.end(); ++curPlayer)
            {
                if ((*curPlayer)->isReserved() && !(*curPlayer)->isInQueue())
                    reservedPlayersInGame.push_back(*curPlayer);
            }

            // Check if this player could join the game if a reserved player was dropped:
            for (eastl::vector<PlayerInfoPtr>::iterator curPlayer = reservedPlayersInGame.begin(); curPlayer != reservedPlayersInGame.end(); ++curPlayer)
            {
                const_cast<GameSessionMaster*>(this)->mPlayerRoster.erasePlayer((*curPlayer)->getPlayerId());


                SlotType suggestedSlotType = qPlayer->getSlotType();

                TeamIndex joiningTeamIndex = qPlayer->getTeamIndex();
                if (joiningTeamIndex == TARGET_USER_TEAM_INDEX)
                {
                    // User attempted to join target user's team.
                    PlayerInfoMaster* targetPlayerInfoMaster = mPlayerRoster.getPlayer(qPlayer->getTargetPlayerId());
                    if (targetPlayerInfoMaster != nullptr)
                    {
                        joiningTeamIndex = targetPlayerInfoMaster->getTeamIndex();
                    }
                    
                    // The target player may not have a valid team set:
                    if (joiningTeamIndex == TARGET_USER_TEAM_INDEX)
                    {
                        joiningTeamIndex = UNSPECIFIED_TEAM_INDEX;
                    }
                }

                bool success = false;

                TeamIndex unspecifiedTeamIndex = UNSPECIFIED_TEAM_INDEX;
                BlazeRpcError err = validateJoinGameCapacities(qPlayer->getRoleName(), qPlayer->getSlotType(), joiningTeamIndex, qPlayer->getPlayerId(), suggestedSlotType, unspecifiedTeamIndex);
                if (err == ERR_OK)
                {
                    success = true;
                }

                // Re-add the players we temporarily removed:  
                const_cast<GameSessionMaster*>(this)->mPlayerRoster.insertPlayer(**curPlayer);

                if (success)
                {
                    if (playerToDemote != nullptr)
                        *playerToDemote = (*curPlayer)->getPlayerId();

                    TRACE_LOG("[GameSessionMaster] Found (" << qPlayer->getPlayerId() << ") eligible for queue promotion in game(" << getGameId() 
                        << "), setting mGameData->getIsAnyQueueMemberPromotable() to 'true', number of pending game dequeues is " << mGameDataMaster->getQueuePumpsPending());
                    return true;
                }
            }
        }

        return false;
    }


    void GameSessionMaster::addMatchmakingQosData(const PlayerIdList& playerIds, MatchmakingSessionId matchmakingSessionId)
    {
        if (playerIds.empty() || matchmakingSessionId == INVALID_MATCHMAKING_SESSION_ID)
        {
            return;
        }

        PlayerIdList::const_iterator playerIter = playerIds.begin();
        PlayerIdList::const_iterator playerEnd = playerIds.end();
        for(; playerIter != playerEnd; ++playerIter)
        {
            if (mGameDataMaster->getMatchmakingSessionMap().insert(eastl::make_pair(*playerIter, matchmakingSessionId)).second)
            {
                GameDataMaster::MatchmakingQosDataPtr& mmQosData = mGameDataMaster->getMatchmakingQosDataMap()[matchmakingSessionId];
                if (mmQosData == nullptr)
                    mmQosData = mGameDataMaster->getMatchmakingQosDataMap().allocate_element();
                mmQosData->setRefCount(mmQosData->getRefCount()+1);
                PlayerQosData *newQosData = mmQosData->getQosDataMap().allocate_element();
                newQosData->setGameConnectionStatus(DISCONNECTED);
                mmQosData->getQosDataMap()[*playerIter] = newQosData; // this cannot double insert, because MatchmakingSessionMap guards against double inserts of PlayerId
            }
            else
            {
                // should never happen, as we don't put people into game multiple times
                ERR_LOG("[GameSessionMaster].addMatchmakingQosData: player " << *playerIter <<
                    " in add request was already member of the matchmaking qos data map for session " << matchmakingSessionId);
            }
        }
    }

    void GameSessionMaster::removeMatchmakingQosData(const PlayerIdList& playerIds)
    {
        PlayerIdList::const_iterator playerIter = playerIds.begin();
        PlayerIdList::const_iterator playerEnd = playerIds.end();
        for(; playerIter != playerEnd; ++playerIter)
        {
            PlayerId playerId = *playerIter;
            updateMatchmakingQosData(playerId, nullptr, 0);
        }
    }

    void GameSessionMaster::addMatchmakingQosData(const UserJoinInfoList &userJoinInfo, MatchmakingSessionId matchmakingSessionId)
    {
        PlayerIdList performQoSList;
        UserJoinInfoList::const_iterator iter = userJoinInfo.begin();
        UserJoinInfoList::const_iterator end = userJoinInfo.end();
        for (; iter != end; ++iter)
        {
            if ((*iter)->getPerformQosValidation())
                performQoSList.push_back((*iter)->getUser().getUserInfo().getId());
        }

        if (!performQoSList.empty())
        {
            addMatchmakingQosData(performQoSList, matchmakingSessionId);
        }
    }
    void GameSessionMaster::removeMatchmakingQosData(const UserJoinInfoList &userJoinInfo)
    {
        PlayerIdList performQoSList;
        UserJoinInfoList::const_iterator iter = userJoinInfo.begin();
        UserJoinInfoList::const_iterator end = userJoinInfo.end();
        for (; iter != end; ++iter)
        {
            if ((*iter)->getPerformQosValidation())
                performQoSList.push_back((*iter)->getUser().getUserInfo().getId());
        }

        if (!performQoSList.empty())
        {
            removeMatchmakingQosData(performQoSList);
        }
    }
    bool GameSessionMaster::isPerformQosValidationListEmpty(const UserJoinInfoList &userJoinInfo)
    {
        UserJoinInfoList::const_iterator iter = userJoinInfo.begin();
        UserJoinInfoList::const_iterator end = userJoinInfo.end();
        for (; iter != end; ++iter)
        {
            if ((*iter)->getPerformQosValidation())
                return false;
        }

        return true;
    }


    /*! ************************************************************************************************/
    /*! \brief Once this has been called for all the QoS matchmaking session's members their QoS measurements should be ready and
        this method will send the data to Matchmaker to complete QoS rule validation. See updateMeshConnection and EndPointsConnectionMesh::setConnectionStatus for details.
        \param[in] playerId - the member this is being called for
        \param[in] status - sets the member's game connection status to this. nullptr removes and avoids validating the member's qos data
        \param[in] connectionGroupId - the connection group id of the member this is being called for
    ***************************************************************************************************/
    bool GameSessionMaster::updateMatchmakingQosData(PlayerId playerId, const PlayerNetConnectionStatus* status, ConnectionGroupId connectionGroupId)
    {
        
        GameSessionConnectionCompletePtr connectionData = nullptr;

        GameDataMaster::MatchmakingSessionMap::iterator sessionIter = mGameDataMaster->getMatchmakingSessionMap().find(playerId);
        if (sessionIter != mGameDataMaster->getMatchmakingSessionMap().end())
        {
            GameDataMaster::MatchmakingQosMap::iterator mmQosIter = mGameDataMaster->getMatchmakingQosDataMap().find(sessionIter->second);
            if (mmQosIter != mGameDataMaster->getMatchmakingQosDataMap().end())
            {
                GameDataMaster::MatchmakingQosDataPtr mmQosData = mmQosIter->second;
                uint16_t count = mmQosData->getRefCount();
                if (count >= 1)
                {
                    --count;
                    mmQosData->setRefCount(count);
                    TRACE_LOG("[GameSessionMaster].updateMatchmakingQosData() : decremented refcount for game("
                        << getGameId() << ") and finalized matchmaking session(" << sessionIter->second <<"), refcount is now " << count);
                }
                else
                {
                    ERR_LOG("[GameSessionMaster].updateMatchmakingQosData() : MatchmakingQosData in game(" << getGameId()
                        << ") for finalized matchmaking session(" << sessionIter->second <<") attempt to underflow refcount.");
                }
                GameQosDataMap::iterator playerQosIter = mmQosData->getQosDataMap().find(playerId);
                if (playerQosIter != mmQosData->getQosDataMap().end())
                {
                    PlayerQosDataPtr playerQosData = playerQosIter->second;
                    if (status != nullptr)
                    {
                        // updates the mesh connection status and overall game connection status           
                        mGameNetwork->getMeshQos(connectionGroupId, *this, *playerQosData);
                        playerQosData->setGameConnectionStatus(*status);
                    }
                    else
                    {
                        mmQosData->getQosDataMap().erase(playerQosIter);
                    }
                }
                if (count == 0)
                {
                    // build our request
                    connectionData = BLAZE_NEW GameSessionConnectionComplete();
                    connectionData->setMatchmakingSessionId(sessionIter->second);
                    connectionData->setGameId(getGameId());
                    connectionData->setNetworkTopology(getGameNetworkTopology());
                    connectionData->getGameQosDataMap().reserve(mmQosData->getQosDataMap().size());

                    // iterate over the mmQosData to get latest full QoS info
                    // as a player may have been removed, or added since we became fully connected
                    GameQosDataMap::iterator mmQosDataIter = mmQosData->getQosDataMap().begin();
                    GameQosDataMap::iterator mmQosDataEnd = mmQosData->getQosDataMap().end();
                    for (; mmQosDataIter != mmQosDataEnd; ++mmQosDataIter)
                    {
                        PlayerId sourcePlayerId = mmQosDataIter->first;
                        ConnectionGroupId sourceConnGroupId = getPlayerConnectionGroupId(sourcePlayerId);
                        PlayerQosDataPtr playerQosData = connectionData->getGameQosDataMap().allocate_element();
                        playerQosData->setGameConnectionStatus(mmQosDataIter->second->getGameConnectionStatus());
                        connectionData->getGameQosDataMap().insert(eastl::make_pair(sourcePlayerId, playerQosData));

                        // worst case, in a full mesh, (or as a game host) there's one entry for every user who matchmade.
                        playerQosData->getLinkQosDataMap().reserve(mmQosData->getQosDataMap().size());
                        // refresh connected players
                        if (playerQosData->getGameConnectionStatus() == CONNECTED)
                        {
                            mGameNetwork->getMeshQos(sourceConnGroupId, *this, *playerQosData);
                        }
                        else
                        {
                            if (!mmQosDataIter->second->getLinkQosDataMap().empty())
                                mmQosDataIter->second->copyInto(*playerQosData);
                        }
                    }
                        
                    mGameDataMaster->getMatchmakingQosDataMap().erase(mmQosIter);

                    TRACE_LOG("[GameSessionMaster].updateMatchmakingQosData() : removed users from the mMatchmakingQosDataMap for game(" 
                        << getGameId() << ") and finalized matchmaking session(" << sessionIter->second <<").");
                }
            }
            
            mGameDataMaster->getMatchmakingSessionMap().erase(sessionIter);
        }

        if (connectionData != nullptr)
        {
            // tell MM about connection results
            TRACE_LOG("[GameManagerMasterImpl].updateMatchmakingQosData() : sending MatchmakingQosData in game(" << getGameId() 
                << ") for finalized matchmaking session(" << connectionData->getMatchmakingSessionId() <<").");

            Fiber::CreateParams params;
            params.groupId = gGameManagerMaster->getQosAuditFiberGroupId();
            // GameSessionMasterPtr pins this game from migrating while we complete the QoS operation
            gSelector->scheduleFiberCall(gGameManagerMaster, &GameManagerMasterImpl::sendQosDataToMatchmaker, GameSessionMasterPtr(this), connectionData, "GameManagerMasterImpl::sendQosDataToMatchmaker", params);
            
            return true;
        }

        return false;
    }

    bool GameSessionMaster::isConnectionGroupInMatchmakingQosDataMap(ConnectionGroupId connectionGroupId) const
    {
        GameDataMaster::PlayerIdListByConnectionGroupIdMap::const_iterator iter = getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connectionGroupId);
        if (iter != getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
        {
            const PlayerIdList* playerIdList = iter->second;
            for (PlayerIdList::const_iterator playerIter = playerIdList->begin(), playerEnd = playerIdList->end();
                 playerIter != playerEnd;
                 ++playerIter)
            {
                if (isPlayerInMatchmakingQosDataMap(*playerIter))
                    return true;
            }
        }

        return false;
    }

    /*! ************************************************************************************************/
    /*! \brief advance player state for users that passed QoS validation to ACTIVE_CONNECTED, and remove
    players that failed QoS validation from the game.
    ***************************************************************************************************/
    void GameSessionMaster::updateQosTestedPlayers(const PlayerIdList &connectedPlayers, const PlayerIdList &removedPlayers)
    {
        // process the provided lists
        BlazeIdList::const_iterator passedIter = connectedPlayers.begin();
        BlazeIdList::const_iterator passedEnd = connectedPlayers.end();
        for (; passedIter != passedEnd; ++passedIter)
        {
            PlayerInfoMaster *playerInfo = getPlayerRoster()->getPlayer(*passedIter);
            if (playerInfo == nullptr)
            {
                TRACE_LOG("[GameSessionMaster].updateQosTestedPlayers() : PlayerId(" << *passedIter << ") in game(" << getGameId()
                    << ") was no longer present to transition to ACTIVE_CONNECTED.");
                continue;
            }

            // client has been notified QoS validation succeeded. ensure player facing metrics will be updated
            mPlayerConnectionMesh.setQosValidationPassedMetricFlag(playerInfo->getConnectionGroupId());

            // cancel qos validation timer after successful validation
            playerInfo->cancelQosValidationTimer();

            if (playerInfo->getPlayerState() == ACTIVE_CONNECTING)
            {
                changePlayerState(playerInfo, ACTIVE_CONNECTED);
            }
            else
            {
                TRACE_LOG("[GameSessionMaster].updateQosTestedPlayers() : PlayerId(" << playerInfo->getPlayerId() << ") in game(" << getGameId() 
                    << ") was not in 'ACTIVE_CONNECTING' state, was in '" << PlayerStateToString(playerInfo->getPlayerState()) << "' state.");
            }

            if( playerInfo->isActive())
            {
                // send join complete notification for the player.
                sendPlayerJoinCompleted(*playerInfo);
            }
        }

        // process the provided lists
        RemovePlayerMasterRequest tempPlayerRemovalRequest;
        tempPlayerRemovalRequest.setGameId(getGameId());
        tempPlayerRemovalRequest.setPlayerRemovedReason(PLAYER_CONN_POOR_QUALITY);
        RemovePlayerMasterError::Error removePlayerError;
        BlazeIdList::const_iterator removedIter = removedPlayers.begin();
        BlazeIdList::const_iterator removedEnd = removedPlayers.end();
        for (; removedIter != removedEnd; ++removedIter)
        {
            PlayerInfoMaster *playerInfo = getPlayerRoster()->getPlayer(*removedIter);
            if (playerInfo == nullptr)
            {
                TRACE_LOG("[GameSessionMaster].updateQosTestedPlayers() : PlayerId(" << *removedIter << ") in game(" << getGameId()
                    << ") was no longer present to remove for QoS validation failure.");
                continue;
            }
            tempPlayerRemovalRequest.setPlayerId(playerInfo->getPlayerId());
            // note no need to cancel player's qos validation timeout, that will occur at player info dtor.

            if (gGameManagerMaster->removePlayer(&tempPlayerRemovalRequest, playerInfo->getPlayerId(), removePlayerError) == GameManagerMasterImpl::REMOVE_PLAYER_GAME_DESTROYED)
            {
                // game destroyed, we can bail.
                return;
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief return whether join game calls to get into this game should be suspended.
    ***************************************************************************************************/
    bool GameSessionMaster::isBusyGameState(GameState gameState) const
    {
        const GameStateList& gsList = gGameManagerMaster->getConfig().getGameSession().getJoinBusyGameStates();
        return (eastl::find(gsList.begin(), gsList.end(), gameState) != gsList.end());
    }

    /*! ************************************************************************************************/
    /*! \brief return whether all the game's current members have good reputation.
        \param[in] leavingBlazeId if a member's id is specified, ignore the member's reputation.
    ***************************************************************************************************/
    bool GameSessionMaster::areAllMembersGoodReputation(BlazeId leavingBlazeId /*= INVALID_BLAZE_ID*/) const
    {
        const PlayerRoster::PlayerInfoList& playerList = getPlayerRoster()->getPlayers(PlayerRoster::ALL_PLAYERS);
        for (PlayerRoster::PlayerInfoList::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
        {
            if ((leavingBlazeId != INVALID_BLAZE_ID) && (leavingBlazeId == (*itr)->getPlayerId()))
                continue;
            if ((*itr)->getUserInfo().getHasBadReputation())
                return false;
        }
        return true;
    }

    /*! ************************************************************************************************/
    /*! \brief if dynamic mode for reputation is enabled, update allowAnyReputation GameSetting as needed
    ***************************************************************************************************/
    void GameSessionMaster::updateAllowAnyReputationOnJoin(const UserSessionInfo& joiningUserInfo)
    {
        if (getGameSettings().getDynamicReputationRequirement() &&
            !getGameSettings().getAllowAnyReputation() && joiningUserInfo.getHasBadReputation())
        {
            // joiner is a 'bad apple', demote game to 'accept bad' rep (set allowAnyReputation)
            TRACE_LOG("[GameSessionMaster] dynamically updating game(" << getGameId() << ") allowAnyReputation to true, on player " << joiningUserInfo.getUserInfo().getId() << " join.");
            getGameData().getGameSettings().setAllowAnyReputation();
            sendGameSettingsUpdate();
        }
    }

    void GameSessionMaster::updateAllowAnyReputationOnRemove(const UserSessionInfo& leavingUserInfo)
    {
        if (getGameSettings().getDynamicReputationRequirement() &&
            getGameSettings().getAllowAnyReputation() && leavingUserInfo.getHasBadReputation() &&
            areAllMembersGoodReputation(leavingUserInfo.getUserInfo().getId()))
        {
            // leaving 'culprit' bad rep player, promote game to 'all good rep' (clear allowAnyReputation)
            TRACE_LOG("[GameSessionMaster] dynamically updating game(" << getGameId() << ") allowAnyReputation to false, on player " << leavingUserInfo.getUserInfo().getId() << " leave.");
            getGameData().getGameSettings().clearAllowAnyReputation();
            sendGameSettingsUpdate();
        }
    }


    ExternalSessionUpdateEventContext GameSessionMaster::mDefaultExternalSessionUpdateContext;

    /*! ************************************************************************************************/
    /*! \brief update external session's properties on game's update info changes.
        Done centrally on game's master here to avoid concurrent sources updating the properties
        (exception is game creation, where we init an external session's props at its creation (see create flow)).
        \param[in] origValues game's pre-change values. Compared vs current values to determine if updates needed.
    ***************************************************************************************************/
    void GameSessionMaster::updateExternalSessionProperties(const ExternalSessionUpdateInfo& origValues,
        const ExternalSessionUpdateInfo& newValues,
        const ExternalSessionUpdateEventContext& context /*= mDefaultExternalSessionUpdateContext*/)
    {
        // To avoid excess fibers only update if platform dictates need to.
        if (gGameManagerMaster->getExternalSessionUtilManager().isUpdateRequired(origValues, newValues, context))
        {
            // We create a copy of the params on heap for the queued job, to be owned by handleUpdateExternalSessionProperties()
            ExternalSessionUpdateInfoPtr origInfo = BLAZE_NEW ExternalSessionUpdateInfo(origValues);
            ExternalSessionUpdateEventContextPtr contextInfo = BLAZE_NEW ExternalSessionUpdateEventContext(context);
            gGameManagerMaster->getExternalSessionJobQueue().queueFiberJob(gGameManagerMaster, &GameManagerMasterImpl::handleUpdateExternalSessionProperties,
                getGameId(), origInfo, contextInfo, "GameManagerMasterImpl::handleUpdateExternalSessionProperties");
        }
    }

    /*! ************************************************************************************************/
    /*! \brief Call to send a snapshot of the game's values before the game is cleaned up, for any
            final external sessions properties handling.
    ***************************************************************************************************/
    void GameSessionMaster::updateFinalExternalSessionProperties()
    {
        // set value indicating this snapshot should be copied to the heap for the queued job to own, as orig data maybe already cleaned
        ExternalSessionUpdateEventContext context;
        getCurrentExternalSessionUpdateInfo().copyInto(context.getFinalUpdateContext());
        TRACE_LOG("[GameSessionMaster].updateFinalExternalSessionProperties: doing final update as needed: game(" << getGameId() << "), in state(" << GameStateToString(getGameState()) << ":" << SimplifiedGamePlayStateToString(context.getFinalUpdateContext().getSimplifiedState()) << ").");

        updateExternalSessionProperties(getCurrentExternalSessionUpdateInfo(), context.getFinalUpdateContext(), context);
    }

    /*! ************************************************************************************************/
    /*! \brief return game's current update info for updating its external session properties.
    ***************************************************************************************************/
    const ExternalSessionUpdateInfo& GameSessionMaster::getCurrentExternalSessionUpdateInfo() const
    {
        setExternalSessionUpdateInfoFromGame(mGameDataMaster->getExternalSessionUpdateInfo(), *this);
        return mGameDataMaster->getExternalSessionUpdateInfo();
    }

    bool GameSessionMaster::isLockedForPreferredJoins() const
    {
        return (gGameManagerMaster->getLockGamesForPreferredJoins() && (mGameDataMaster->getLockForPreferredJoinsTimeout() != 0));
    }

    void GameSessionMaster::sendGameUpdateEvent()
    {
        GameManager::UpdatedGameSessionData gameSessionData;
        GameManager::GameBrowserGameData &gameBrowserGameData = gameSessionData.getGameData();

        // set common data that we always send to the client
        gameBrowserGameData.setGameId(getGameId());
        gameBrowserGameData.setGameName(getGameName());
        gameBrowserGameData.setGameProtocolVersionString(getGameProtocolVersionString());
        gameBrowserGameData.setVoipTopology(getVoipNetwork());
        gameBrowserGameData.setPresenceMode(getPresenceMode());
        getPresenceDisabledList().copyInto(gameBrowserGameData.getPresenceDisabledList());
        gameBrowserGameData.setGameStatusUrl(getGameStatusUrl());

        gameBrowserGameData.setGameMode(getGameMode());

        gameBrowserGameData.setGameModRegister(getGameModRegister());

        gameBrowserGameData.setPingSiteAlias(getBestPingSiteAlias());
        gameBrowserGameData.setNetworkTopology(getGameNetworkTopology());
        gameBrowserGameData.setGameType(getGameType());

        gameBrowserGameData.getSlotCapacities().assign(getSlotCapacities().begin(), 
            getSlotCapacities().end());      

        // Populate slot player counts
        getPlayerRoster()->getSlotPlayerCounts(gameBrowserGameData.getPlayerCounts());


        GameManager::GameBrowserTeamInfoVector &teamInfoVector = gameBrowserGameData.getGameBrowserTeamInfoVector();
        GameManager::TeamIdVector::const_iterator teamIdsIter = getTeamIds().begin();
        GameManager::TeamIdVector::const_iterator teamIdsEnd = getTeamIds().end();
        teamInfoVector.clearVector(); // need to make sure we're not double-adding teams
        teamInfoVector.reserve(getTeamCount());
        for (GameManager::TeamIndex i = 0; teamIdsIter != teamIdsEnd; ++ teamIdsIter)
        {
            teamInfoVector.pull_back();
            teamInfoVector.back()->setTeamId((*teamIdsIter));
            teamInfoVector.back()->setTeamSize(getPlayerRoster()->getTeamSize(i));
            GameManager::RoleMap& gbRoleSizeMap = teamInfoVector.back()->getRoleSizeMap();
            GameManager::RoleSizeMap teamRoleSizes;
            getPlayerRoster()->getRoleSizeMap(teamRoleSizes, i);
            if (!teamRoleSizes.empty()) // we know the desired team index is 'in bounds' because iteration is controlled by the game's team vector
            {
                GameManager::RoleSizeMap::const_iterator teamRoleSizeIter = teamRoleSizes.begin();
                GameManager::RoleSizeMap::const_iterator teamRoleSizeEnd = teamRoleSizes.end();
                for (; teamRoleSizeIter != teamRoleSizeEnd; ++teamRoleSizeIter)
                {
                    gbRoleSizeMap[teamRoleSizeIter->first] = teamRoleSizeIter->second;
                }
            }

            ++i;
        }

        gameBrowserGameData.setTeamCapacity(getTeamCapacity());
        getRoleInformation().copyInto(gameBrowserGameData.getRoleInformation());

        gameBrowserGameData.setGameState(getGameState());
        gameBrowserGameData.getGameSettings() = getGameSettings();
        getTopologyHostNetworkAddressList().copyInto(gameBrowserGameData.getHostNetworkAddressList());
        gameBrowserGameData.setHostId(getTopologyHostInfo().getPlayerId());
        gameBrowserGameData.setTopologyHostSessionId(getTopologyHostSessionId());
        gameBrowserGameData.getAdminPlayerList().assign(getAdminPlayerList().begin(), getAdminPlayerList().end());
        gameBrowserGameData.setQueueCapacity(getQueueCapacity());
        const GameManager::PlayerRoster::PlayerInfoList& queuedPlayers = getPlayerRoster()->getPlayers(GameManager::PlayerRoster::QUEUED_PLAYERS);
        gameBrowserGameData.setQueueCount((uint16_t)queuedPlayers.size());

        getEntryCriteria().copyInto(gameBrowserGameData.getEntryCriteriaMap());
        getGameAttribs().copyInto(gameBrowserGameData.getGameAttribs());
        gameBrowserGameData.setPersistedGameId(getPersistedGameId());

        EA::TDF::TdfHashValue tdfHash = EA::StdC::kCRC32InitialValue;
        if (gameBrowserGameData.computeHash(tdfHash))
        {
            if (tdfHash == mLastGameUpdateEventHash)
                return; // event identical to last, early out
            mLastGameUpdateEventHash = tdfHash;
        }
        else
        {
            WARN_LOG("[GameSessionMaster].sendGameUpdateEvent: Failed to compute update event hash for game: " << getGameId() << ", submitting anyway.");
        }

        gEventManager->submitEvent(static_cast<uint32_t>(GameManagerMaster::EVENT_UPDATEDGAMESESSIONDATAEVENT), gameSessionData);
    }

    const ExternalSessionUtilManager& GameSessionMaster::getExternalSessionUtilManager() const
    {
        return gGameManagerMaster->getExternalSessionUtilManager();
    }

    bool GameSessionMaster::shouldEndpointsConnect(ConnectionGroupId source, ConnectionGroupId target) const
    {
        return mGameNetwork->shouldEndpointsConnect(this, source, target);
    }

    bool GameSessionMaster::isConnectionRequired(ConnectionGroupId source, ConnectionGroupId target) const
    {
        return mGameNetwork->isConnectionRequired(this, source, target);
    }
    void GameSessionMaster::subscribePlayerRoster(UserSessionId sessionId, SubscriptionAction action)
    {
        if (isPseudoGame())
            return;

        SessionIdIteratorPair itPair = getSessionIdIteratorPair();

        UserSessionIdList ids;
        UserSession::fillUserSessionIdList(ids, itPair.begin(), itPair.end(), UserSessionIdIdentityFunc());

        UserSession::mutualSubscribe(sessionId, ids, action);
    }

    /*! ************************************************************************************************/
    /*! \brief get the best ping site for the connection. For metrics.
    ***************************************************************************************************/
    const char8_t* GameSessionMaster::getBestPingSiteForConnection(ConnectionGroupId connGrpId) const
    {
        // If somehow connection group's players don't have one set, or we're fetching for a dedicated server host, use game's best ping site
        const char8_t* pingSite = getBestPingSiteAlias();

        GameDataMaster::PlayerIdListByConnectionGroupIdMap::const_iterator playerIdListItr = getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connGrpId);
        if (playerIdListItr != getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
        {
            for (PlayerIdList::const_iterator itr = playerIdListItr->second->begin(), end = playerIdListItr->second->end(); itr != end; ++itr)
            {
                const PlayerInfoMaster *playerInfo = getPlayerRoster()->getPlayer(*itr);
                if ((playerInfo != nullptr) && (playerInfo->getUserInfo().getBestPingSiteAlias() != nullptr) && (playerInfo->getUserInfo().getBestPingSiteAlias()[0] != '\0') &&
                    (blaze_strcmp(playerInfo->getUserInfo().getBestPingSiteAlias(), UNKNOWN_PINGSITE) != 0 && blaze_strcmp(playerInfo->getUserInfo().getBestPingSiteAlias(), INVALID_PINGSITE) != 0))
                {
                    pingSite = playerInfo->getUserInfo().getBestPingSiteAlias();
                    break;
                }
            }
        }
        if ((pingSite != nullptr) && (pingSite[0] == '\0'))
            pingSite = UNKNOWN_PINGSITE;
        return pingSite;
    }

    /*! ************************************************************************************************/
    /*! \brief get the client platform type for the users on the connection.
    ***************************************************************************************************/
    ClientPlatformType GameSessionMaster::getClientPlatformForConnection(ConnectionGroupId connGrpId) const
    {
        if (isDedicatedHostedTopology(getGameNetworkTopology()) && (connGrpId == getDedicatedServerHostInfo().getConnectionGroupId()))
        {
            // set the dedicated server platform
            return ClientPlatformType::common;
        }
        else if (connGrpId != INVALID_CONNECTION_GROUP_ID)
        {
            GameDataMaster::PlayerIdListByConnectionGroupIdMap::const_iterator playerIdListItr = getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().find(connGrpId);
            if (playerIdListItr != getGameDataMaster()->getPlayerIdListByConnectionGroupIdMap().end())
            {
                for (PlayerIdList::const_iterator itr = playerIdListItr->second->begin(), end = playerIdListItr->second->end(); itr != end; ++itr)
                {
                    const PlayerInfoMaster *playerInfo = getPlayerRoster()->getPlayer(*itr);
                    if (playerInfo != nullptr)
                    {
                        if (playerInfo->getClientPlatform() != ClientPlatformType::INVALID)
                        {
                            return playerInfo->getClientPlatform();
                        }
                    }
                }
            }
        }

        return ClientPlatformType::INVALID;
    }

    /*! ************************************************************************************************/
    /*! \brief store the game's external session info to the response
    ***************************************************************************************************/
    void GameSessionMaster::getExternalSessionInfoMaster(const UserIdentification &caller, GetExternalSessionInfoMasterResponse &response) const
    {
        setExternalSessionCreationInfoFromGame(response.getExternalSessionCreationInfo(), *this);
        response.setIsGameAdmin(isAdminPlayer(caller.getBlazeId()));
        response.setIsGameMember(getPlayerRoster()->getPlayer(caller.getBlazeId()) != nullptr);
    }

    /*! ************************************************************************************************/
    /*! \brief Called after adding user to a 1st party session, to correspondingly track the 1st party session memberships on the GameManager master.
    
        \info For PS4/PS5, due to no multiple NP/Player-session memberships (see DevNet ticket 58807), games can lose their 1st party session when
        users transfer advertisement to another session, as empty session's are automatically destroyed on the 1st party side.
        To keep game and such 1st party sessions in sync, we track users' 1st party session memberships centrally Blaze side.

        \param[in] request Specifies the user and expected 1st party session to track user as in. If game has a different 1st party session,
            call fails, returning GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION, and game's session will be in errorResponse.
    ***************************************************************************************************/
    TrackExtSessionMembershipMasterError::Error GameSessionMaster::trackExtSessionMembership(
        const TrackExtSessionMembershipRequest &request, GetExternalSessionInfoMasterResponse &response,
        GetExternalSessionInfoMasterResponse &errorResponse)
    {
        // On PS5 there's >1 poss 1st party session type (PSN PlayerSessions, Matches). Ensure updating for right type.
        const auto& forType = request.getExtSessType();

        // if unset, set the game's newly created session
        if (EA_LIKELY(isExtSessIdentSet(request.getExtSessionToAdvertise(), forType) &&
            !isExtSessIdentSame(request.getExtSessionToAdvertise(), getExternalSessionIdentification(), forType)))
        {
            if (isExtSessIdentSet(getExternalSessionIdentification(), forType))
            {
                TRACE_LOG("[GameSessionMaster].trackExtSessionMembership: could NOT assign session(" << toLogStr(request.getExtSessionToAdvertise(), forType) << ") to game(" << getGameId() << "), as game already had one(" << toLogStr(getExternalSessionIdentification(), forType) << ").");
                // copy game's current session info to response
                getExternalSessionInfoMaster(request.getUser().getUserIdentification(), errorResponse);
                return TrackExtSessionMembershipMasterError::GAMEMANAGER_ERR_INVALID_GAME_STATE_ACTION;
            }
            setExtSessIdent(getExternalSessionIdentification(), request.getExtSessionToAdvertise(), forType);
            newToOldReplicatedExternalSessionData(getGameData());//for back compat
        }
        if (!isExtSessIdentSet(getExternalSessionIdentification(), forType))
        {
            ERR_LOG("[GameSessionMaster:].trackExtSessionMembership: unexpected attempt to track empty session id for game(" << getGameId() << "). No op.");
            return TrackExtSessionMembershipMasterError::ERR_SYSTEM;
        }

        // track as member of the 1st party session
        auto& trackedMembers = getOrAddListInMap(getGameDataMaster()->getTrackedExternalSessionMembers(), forType);
        ExternalMemberInfoList::iterator itr = trackedMembers.end();
        if (!getUserInExternalMemberInfoList(request.getUser().getUserIdentification().getBlazeId(), trackedMembers, itr))
        {
            request.getUser().copyInto(*trackedMembers.pull_back());
            TRACE_LOG("[GameSessionMaster].trackExtSessionMembership: User(" << request.getUser().getUserIdentification().getBlazeId() << ") tracked as member of game(" << getGameId() << ") ext session(" << toLogStr(getExternalSessionIdentification(), forType) << "). Game now has " << trackedMembers.size() << " members(" << (trackedMembers.empty() ? "empty" : trackedMembers.front()->getUserIdentification().getName()) << "..)");
        }
        else if (EA_LIKELY(itr != trackedMembers.end()) && (request.getUser().getTeamIndex() != (*itr)->getTeamIndex()))
        {
            // just update tracked team set for member in 1P session (for ensuring PSN Match team membership synced)
            (*itr)->setTeamIndex(request.getUser().getTeamIndex());
        }
        // ensure new 1st party session's props are up to date, after its creation on slave (do this after updating tracked members:)
        updateExternalSessionProperties(request.getExtSessionOrigUpdateInfo(), getCurrentExternalSessionUpdateInfo());

        // copy game's updated 1st party session info to response
        getExternalSessionInfoMaster(request.getUser().getUserIdentification(), response);
        return TrackExtSessionMembershipMasterError::ERR_OK;
    }

    /*! ************************************************************************************************/
    /*! \brief If user was tracked as in game's 1st party session, track as removed. If no one is left in the 1st party session, and (depending on platform)
        would thus get removed by 1st party, disassociate it from the game. (A new one will be associated for game, when someone creates/advertises one for it, see above)

        \param[in] type If non-null, only update for the ExternalSessionActivityType. Else update for all platform's ExternalSessionActivityType's
    ***************************************************************************************************/
    void GameSessionMaster::untrackExtSessionMembership(BlazeId blazeId, ClientPlatformType platform, const ExternalSessionActivityType* type)
    {
        auto byPlatIt = getGameDataMaster()->getTrackedExternalSessionMembers().find(platform);
        if (byPlatIt == getGameDataMaster()->getTrackedExternalSessionMembers().end())
        {
            //NOOP if N/A or already empty for platform
            return;
        }
        if (type != nullptr)
        {
            auto byTypeIt = byPlatIt->second->find(*type);
            if (byTypeIt != byPlatIt->second->end())
                untrackExtSessionMembershipInternal(*byTypeIt->second, blazeId, platform, byTypeIt->first);
            return;
        }
        //remove for all types
        for (auto byTypeIt : *byPlatIt->second)
            untrackExtSessionMembershipInternal(*byTypeIt.second, blazeId, platform, byTypeIt.first);
    }

    void GameSessionMaster::untrackExtSessionMembershipInternal(ExternalMemberInfoList& trackedSessionMembers, BlazeId blazeId, ClientPlatformType plat, ExternalSessionActivityType type)
    {
        ExternalMemberInfoList::iterator itr = trackedSessionMembers.end();
        bool existed = getUserInExternalMemberInfoList(blazeId, trackedSessionMembers, itr);
        if (!existed)
        {
            TRACE_LOG("[GameSessionMaster].untrackExtSessionMembershipInternal: User(" << blazeId << ") was already tracked as removed from game(" << getGameId() << ") ext session(" << (isExtSessIdentSet(getExternalSessionIdentification()) ? toLogStr(getExternalSessionIdentification()) : "<n/a>") << ") for platform(" << ClientPlatformTypeToString(plat) << "), activityType(" << ExternalSessionActivityTypeToString(type) << "). No op.");
            return;
        }

        trackedSessionMembers.erase(itr);
        TRACE_LOG("[GameSessionMaster].untrackExtSessionMembershipInternal: User(" << blazeId << ") tracked as removed from game(" << getGameId() << ") ext session(" << toLogStr(getExternalSessionIdentification()) << ") for platform(" << ClientPlatformTypeToString(plat) << "), activityType(" << ExternalSessionActivityTypeToString(type) << "). Game now has " << trackedSessionMembers.size() << " members(" << (trackedSessionMembers.empty() ? "will be disassociated from game" : trackedSessionMembers.front()->getUserIdentification().getName()) << "..). GameType(" << GameTypeToString(getGameType()) << ").");

        // Disassociate an empty NP session. Prevents further joins to the old NP session, to guard vs any timing
        // dependencies occur due to delays between when Blaze removes player, and when 1st party service removes them
        if (trackedSessionMembers.empty())
        {
            clearExternalSessionIds(plat, type);
        }
    }

    void GameSessionMaster::untrackAllExtSessionMemberships()
    {
        // More efficient to just clear() the maps, but just to ensure all remove flows go through untrackExtSessionMembership():
        for (const auto& byPlatIt : getGameDataMaster()->getTrackedExternalSessionMembers())
        {
            for (auto byTypetIt : *byPlatIt.second)
            {
                ClientPlatformType plat = byPlatIt.first;
                ExternalSessionActivityType type = byTypetIt.first;
                ExternalMemberInfoList& trackedSessionMembers = *byTypetIt.second;
                // make copy of the ids, as the un-track loop invalidates the orig list
                BlazeIdVector idCopies;
                idCopies.reserve(trackedSessionMembers.size());
                for (auto itr : trackedSessionMembers)
                {
                    idCopies.push_back(itr->getUserIdentification().getBlazeId());
                }
                for (auto itr : idCopies)
                {
                    untrackExtSessionMembership(itr, plat, &type);
                }
            }
        }
    }

    /*! ************************************************************************************************/
    /*! \brief update external session status
    ***************************************************************************************************/
    UpdateExternalSessionStatusMasterError::Error GameSessionMaster::updateExternalSessionStatus(const Blaze::GameManager::UpdateExternalSessionStatusRequest &request)
    {
        if (!isAdminPlayer(UserSession::getCurrentUserBlazeId()))
        {
            TRACE_LOG("[GameSessionMaster].updateExternalSessionStatus: Non admin user(" << UserSession::getCurrentUserBlazeId() << ") can't set session status for game(" << getGameId() << ").");
            return UpdateExternalSessionStatusMasterError::GAMEMANAGER_ERR_PERMISSION_DENIED;
        }
        ExternalSessionUpdateInfo origUpdateInfo(getCurrentExternalSessionUpdateInfo());

        request.getExternalSessionStatus().copyInto(getGameDataMaster()->getExternalSessionStatus());

        updateExternalSessionProperties(origUpdateInfo, getCurrentExternalSessionUpdateInfo());

        // (these are just updates shown in first party shell UX, so we don't wait on them)
        return UpdateExternalSessionStatusMasterError::ERR_OK;
    }

    // if didn't yet have an active player as admin, add one
    void GameSessionMaster::updateAutomaticAdminSelection(const PlayerInfoMaster& joiningActivePlayer)
    {
        if (getGamePermissionSystem().mEnableAutomaticAdminSelection && joiningActivePlayer.isActive())
        {
            if (getAdminPlayerList().empty() ||
                (hasDedicatedServerHost() && (getAdminPlayerList().size() == 1) && (getAdminPlayerList().front() == getDedicatedServerHostBlazeId())))
            {
                const PlayerId noPlayer = INVALID_BLAZE_ID;
                addAdminPlayer(joiningActivePlayer.getPlayerId(), noPlayer);
            }
        }
    }

    void GameSessionMaster::chooseProtectedIPRecipients(const PlayerInfo &subjectPlayer, SessionIdSetIdentityFunc& sessionIdSetIdentityFuncProtectedIP, SessionIdSetIdentityFunc& sessionIdSetIdentityFuncUnprotectedIP)
    {
        // sessionIdSetIdentityFuncProtectedIP contains the session ids that won't get a notification w/ masked IP addresses
        sessionIdSetIdentityFuncProtectedIP.addSessionId(subjectPlayer.getPlayerSessionId());
        sessionIdSetIdentityFuncProtectedIP.addSessionId(getDedicatedServerHostSessionId());

        // sessionIdSetIdentityFuncForUnprotectedIP contains the session ids that won't get a notification w/ unmasked IP address
        // we omit the joining player from these notifications entirely
        sessionIdSetIdentityFuncUnprotectedIP.addSessionId(subjectPlayer.getPlayerSessionId());

        if (enablePartialProtectedIP())
        {
            ClientPlatformType playerClientPlatform = subjectPlayer.getClientPlatform();
            // iterate over all the players in the game
            for (auto currentPlayer : getPlayerRoster()->getPlayers(PlayerRoster::ALL_PLAYERS))
            {
                if (currentPlayer->getPlayerSessionId() != subjectPlayer.getPlayerSessionId())
                {
                    if (currentPlayer->getClientPlatform() != playerClientPlatform)
                    {
                        // platform mismatch, exclude currentPlayer from the set to receive unprotectedIPs.
                        sessionIdSetIdentityFuncUnprotectedIP.addSessionId(currentPlayer->getPlayerSessionId());
                    }
                    else // platforms match, this player can receive the network address
                    {
                        sessionIdSetIdentityFuncProtectedIP.addSessionId(currentPlayer->getPlayerSessionId());
                    }
                }
            }
        }
    }

    const ExternalMemberInfoList& GameSessionMaster::getTrackedExtSessionMembers(const ExternalSessionActivityTypeInfo& platformActivityType) const
    {
        auto* listToCheck = getListFromMap(getGameDataMaster()->getTrackedExternalSessionMembers(), platformActivityType);
        return (listToCheck ? *listToCheck : mSentinelEmptyExternalMembersList);
    }

    /*! ************************************************************************************************/
    /*! \brief get team name if it has one specified for it
    ***************************************************************************************************/
    const char8_t* GameSessionMaster::getTeamName(GameManager::TeamIndex teamIndex) const
    {
        if ((teamIndex == UNSPECIFIED_TEAM_INDEX) || (teamIndex >= getTeamCount()))
        {
            return nullptr;
        }
        auto teamId = getTeamIdByIndex(teamIndex);
        if (teamId == INVALID_TEAM_ID)
        {
            return nullptr;
        }
        auto toNameIt = gGameManagerMaster->getConfig().getGameSession().getTeamNameByTeamIdMap().find(teamId);
        return ((toNameIt != gGameManagerMaster->getConfig().getGameSession().getTeamNameByTeamIdMap().end()) ? toNameIt->second.c_str() : nullptr);
    }

    void GameSessionMaster::refreshPINIsCrossplayGameFlag()
    {
        // Flag should be only be updated when IN_GAME or GAME_GROUP_INITIALIZED and Crossplay is enabled. Don't change flag if already true. 
        if (!(getGameState() == IN_GAME || getGameState() == GAME_GROUP_INITIALIZED) || !getGameData().getIsCrossplayEnabled() || (getGameData().getPINIsCrossplayGame()))
        {
            return;
        }

        //Only care about ACTIVE_PLAYERS for this flag
        PlayerRoster::PlayerInfoList activePlayerInfoList = getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS);
        eastl::hash_set<ClientPlatformType> platformsInGameSet;
        for (auto player : activePlayerInfoList)
        {
            addToCrossplayPlatformSet(player->getPlatformInfo().getClientPlatform(), platformsInGameSet);
            if (platformsInGameSet.size() > 1)
            {
                getGameData().setPINIsCrossplayGame(true);
                break;
            }
        }
        platformsInGameSet.clear();
    }

    void GameSessionMaster::addToCrossplayPlatformSet(ClientPlatformType platform, eastl::hash_set<ClientPlatformType>& platformSet)
    {
        //If platform exists in platform set, do nothing
        if (platformSet.find(platform) != platformSet.end())
        {
            return;
        }
            
        const ClientPlatformSet* nonCrossplayPlatformSet = &gUserSessionManager->getNonCrossplayPlatformSet(platform);

        //Insert platform to platform set if not associated with any other platform for nonCrossplayPlatformSet
        if (nonCrossplayPlatformSet->empty())
        {
            platformSet.insert(platform);
            return;
        }

        //Check if first platform from nonCrossplayPlatformSet is in platformSet. If it does, do nothing
        if (platformSet.find(*nonCrossplayPlatformSet->begin()) != platformSet.end())
        {
            return;
        }

        //add first platform from nonCrossplayPlatformSet to platform set
        platformSet.insert(*nonCrossplayPlatformSet->begin());

    }
}// namespace GameManager
}// namespace Blaze

