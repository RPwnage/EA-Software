/*! ************************************************************************************************/
/*!
    \file gamesessionmetrics.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "framework/blaze.h"
#include "gamemanager/gamesessionmaster.h"
#include "gamemanager/gamemanagermasterimpl.h"

#include <ctype.h>

namespace Blaze
{

namespace Metrics
{
namespace Tag
{
TagInfo<const char8_t*>* game_mode = BLAZE_NEW TagInfo<const char8_t*>("game_mode");

TagInfo<GameManager::GameType>* game_type = BLAZE_NEW TagInfo<GameManager::GameType>("game_type", [](const GameManager::GameType& value, Metrics::TagValue&) { return GameManager::GameTypeToString(value); });

TagInfo<GameNetworkTopology>* network_topology = BLAZE_NEW TagInfo<GameNetworkTopology>("network_topology", [](const GameNetworkTopology& value, Metrics::TagValue&) { return GameNetworkTopologyToString(value); });

TagInfo<GameManager::JoinMethod>* join_method = BLAZE_NEW TagInfo<GameManager::JoinMethod>("join_method", [](const GameManager::JoinMethod& value, Metrics::TagValue&) { return JoinMethodToString(value); });

TagInfo<GameManager::PlayerRemovedReason>* player_removed_reason = BLAZE_NEW TagInfo<GameManager::PlayerRemovedReason>("player_removed_reason", [](const GameManager::PlayerRemovedReason& value, Metrics::TagValue&) { return PlayerRemovedReasonToString(value); });

TagInfo<GameManager::GameDestructionReason>* game_destruction_reason = BLAZE_NEW TagInfo<GameManager::GameDestructionReason>("game_destruction_reason", [](const GameManager::GameDestructionReason& value, Metrics::TagValue&) { return GameDestructionReasonToString(value); });

TagInfo<GamePingSite>* game_pingsite = BLAZE_NEW TagInfo<GamePingSite>("game_pingsite");

TagInfo<int32_t>* time_threshold = BLAZE_NEW TagInfo<int32_t>("time_threshold", Blaze::Metrics::int32ToString);

TagInfo<GameManager::DemanglerConnectionHealthcheck>* demangler_connection_healthcheck = BLAZE_NEW TagInfo<GameManager::DemanglerConnectionHealthcheck>("demangler_connection_healthcheck", [](const GameManager::DemanglerConnectionHealthcheck& value, Metrics::TagValue&) { return DemanglerConnectionHealthcheckToString(value); });

TagInfo<GameManager::ConnectionJoinType>* connection_join_type = BLAZE_NEW TagInfo<GameManager::ConnectionJoinType>("connection_join_type", [](const GameManager::ConnectionJoinType& value, Metrics::TagValue&) { return ConnectionJoinTypeToString(value); });

TagInfo<GameManager::ConnSuccessMetricEnum>* connection_result_type = BLAZE_NEW TagInfo<GameManager::ConnSuccessMetricEnum>("connection_result_type", [](const GameManager::ConnSuccessMetricEnum& value, Metrics::TagValue&) { return ConnSuccessMetricEnumToString(value); });

TagInfo<VoipTopology>* voip_topology = BLAZE_NEW TagInfo<VoipTopology>("voip_topology", [](const VoipTopology& value, Metrics::TagValue&) { return VoipTopologyToString(value); });

TagInfo<GameManager::ConnImpactingMetricEnum>* connection_impacting_type = BLAZE_NEW TagInfo<GameManager::ConnImpactingMetricEnum>("connection_impacting_type", [](const GameManager::ConnImpactingMetricEnum& value, Metrics::TagValue&) { return ConnImpactingMetricEnumToString(value); });

TagInfo<GameManager::ConnConciergeModeMetricEnum>* cc_mode = BLAZE_NEW TagInfo<GameManager::ConnConciergeModeMetricEnum>("cc_mode", [](const GameManager::ConnConciergeModeMetricEnum& value, Metrics::TagValue&) { return ConnConciergeModeMetricEnumToString(value); });

TagInfo<GameManager::ConnConciergeEligibleEnum>* cc_eligible_type = BLAZE_NEW TagInfo<GameManager::ConnConciergeEligibleEnum>("cc_eligible_type", [](const GameManager::ConnConciergeEligibleEnum& value, Metrics::TagValue&) { return ConnConciergeEligibleEnumToString(value); });

TagInfo<GameManager::ConnDemanglerMetricEnum>* connection_demangler_type = BLAZE_NEW TagInfo<GameManager::ConnDemanglerMetricEnum>("connection_demangler_type", [](const GameManager::ConnDemanglerMetricEnum& value, Metrics::TagValue&) { return ConnDemanglerMetricEnumToString(value); });

TagInfo<uint32_t>* index = BLAZE_NEW TagInfo<uint32_t>("index", Blaze::Metrics::uint32ToString);

TagInfo<GameManager::ScenarioName>* scenario_name = BLAZE_NEW TagInfo<GameManager::ScenarioName>("scenario_name", [](const GameManager::ScenarioName& value, Metrics::TagValue&) { return value.c_str(); });

TagInfo<GameManager::ScenarioVariantName>* scenario_variant_name = BLAZE_NEW TagInfo<GameManager::ScenarioVariantName>("scenario_variant_name", [](const GameManager::ScenarioVariantName& value, Metrics::TagValue&) { return value.c_str(); });

TagInfo<GameManager::ScenarioVersion>* scenario_version = BLAZE_NEW TagInfo<GameManager::ScenarioVersion>("scenario_version", Blaze::Metrics::uint32ToString);

TagInfo<GameManager::SubSessionName>* subsession_name = BLAZE_NEW TagInfo<GameManager::SubSessionName>("subsession_name", [](const GameManager::SubSessionName& value, Metrics::TagValue&) { return value.c_str(); });

TagInfo<GameManager::GameModeByJoinMethodType>* game_mode_by_join_method = BLAZE_NEW TagInfo<GameManager::GameModeByJoinMethodType>("game_mode_by_join_method", [](const GameManager::GameModeByJoinMethodType& value, Metrics::TagValue&) { return GameModeByJoinMethodTypeToString(value); });

const char8_t* NON_SCENARIO_NAME = "non-scenario";

const GameManager::ScenarioVersion NON_SCENARIO_VERSION = 0;
}
}

namespace GameManager
{
    const char8_t* UNKNOWN_GAMEMODE = "unknown_game_mode";
    
    /*! ************************************************************************************************/
    /*! \brief static array of time thresholds (in seconds) used by metrics.  We tally
            some metric events depending on which of these "time thresholds" the event falls into.

            Ex: we tally player leaves based on how long they were in the game ( # left before 1 min, # left before 5 min, etc).
    ***************************************************************************************************/
    const int GameSessionMetricsHandler::TIME_THRESHOLD[GameSessionMetricsHandler::MAX_THRESHOLD] = {
        60,  //1 minute
        300, //5 minute
        600, //10 minute
        900  //15 minute
    };

    GameSessionMetricsHandler::GameSessionMetricsHandler()
        : mGameSessionMaster(nullptr)
        , mGameCreateMetricsSet(false)
        , mActiveSlotMetricsSet(false)
        , mAllSlotMetricsSet(false)
    {

    }

    GameSessionMetricsHandler::~GameSessionMetricsHandler()
    {

    }
    
    void GameSessionMetricsHandler::setGameSessionMaster(GameSessionMaster& session)
    {
        EA_ASSERT(mGameSessionMaster == nullptr);
        mGameSessionMaster = &session;
    }

    void GameSessionMetricsHandler::onGameStarted()
    {
        gGameManagerMaster->getMetrics().mGamesStarted.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
    }

    void GameSessionMetricsHandler::onGameCreated()
    {
        calcGameCreatedMetrics();
    }

    void GameSessionMetricsHandler::onDedicatedGameHostInitialized(const GameSettings& gameSettings)
    {
        if (gameSettings.getVirtualized())
        {
            // virtual games aren't part of the ALL_GAMES metric, so inc here, because it is going active.
            gGameManagerMaster->getMetrics().mAllGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());

            incrementActiveVirtualGames();
            incrementActiveDedicatedServerMetrics();
            calcPlayerSlotUpdatedMetrics(0, mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity());
            calcActiveSlotUpdatedMetrics(0, mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity());
        }
        else if (getGameData().getServerNotResetable())
        {
            incrementActiveDedicatedServerMetrics();
            calcActiveSlotUpdatedMetrics(0, mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity());
        }
    }

    void GameSessionMetricsHandler::onDedicatedGameHostReturnedToPool()
    {
        decrementActiveDedicatedServerMetrics();
        calcActiveSlotUpdatedMetrics(mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity(), 0);
    }

    void GameSessionMetricsHandler::onDedicatedGameHostReset()
    {
        incrementActiveDedicatedServerMetrics();
        calcActiveSlotUpdatedMetrics(0, mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity());
    }

    void GameSessionMetricsHandler::onPlayerCapacityUpdated(uint16_t oldTotalParticipantCapacity, uint16_t newTotalParticipantCapacity, uint16_t oldTotalSpectatorCapacity, uint16_t newTotalSpectatorCapacity)
    {
        calcPlayerSlotUpdatedMetrics(oldTotalParticipantCapacity, newTotalParticipantCapacity, oldTotalSpectatorCapacity, newTotalSpectatorCapacity);
        
        if (isActiveGame())
            calcActiveSlotUpdatedMetrics(oldTotalParticipantCapacity, newTotalParticipantCapacity, oldTotalSpectatorCapacity, newTotalSpectatorCapacity);
    }

    void GameSessionMetricsHandler::onGameDestroyed(bool updateTotals, GameDestructionReason destructionReason)
    {
        //+ These functions track if the metrics need to be added/removed internally. 
        calcGameDestroyedMetrics(true, destructionReason);
        calcPlayerSlotUpdatedMetrics(mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity(), 0);
        //-

        // This is incremented only after successful initialization.  so only decrement if not destroying for failed create.
        if (destructionReason != SYS_CREATION_FAILED)
        {
            switch (getGameData().getNetworkTopology())
            {
            case CLIENT_SERVER_DEDICATED:
                // don't decrement resettable here as they are already decremented from this metric on return of the dedicated server.
                if (isActiveGame())
                {
                    decrementActiveDedicatedServerMetrics();
                    calcActiveSlotUpdatedMetrics(mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity(), 0);
                }

                break;
            default:
                break;
            }

            if (getGameData().getGameSettings().getVirtualized() && !mGameSessionMaster->isInactiveVirtual())
            {
                decrementActiveVirtualGames();
            }
        }

        if (getGameData().getGameState() == UNRESPONSIVE)
        {
            gGameManagerMaster->getMetrics().mUnresponsiveGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }
    
    const ReplicatedGameData& GameSessionMetricsHandler::getGameData() const
    { 
        return mGameSessionMaster->getReplicatedGameSession(); 
    }

    const char8_t* GameSessionMetricsHandler::getGameModeAsMetric(const char8_t* gameMode)
    {
        // gameMode should never be nullptr, since we force-insert the attribute into every game, but it can be the empty string if unset by clients during create/reset
        if (gameMode == nullptr || gameMode[0] == '\0')
        {
            gameMode = UNKNOWN_GAMEMODE;
        }
        else if (EA_UNLIKELY(blaze_stricmp(gameMode, UNKNOWN_GAMEMODE) == 0))
        {
            WARN_LOG("[GameSessionMetricsHandler].getGameModeAsMetric: UNKNOWN_GAMEMODE used as a real game mode attribute value."
                << " Please avoid using this attribute value in the configured game mode attribute to avoid potential conflicts.");
        }
        else
        {
            auto strLength = strlen(gameMode);
            for (auto i = 0; i < strLength; ++i)
            {
                if (!(isalnum(gameMode[i]) || gameMode[i] == '-' || gameMode[i] == '_' || gameMode[i] == ':'))
                {
                    WARN_LOG("[GameSessionMetricsHandler].getGameModeAsMetric: Non alpha numeric character other than - _ : found in the gamemode. Setting the game mode to unknown_game_mode for metrics purpose. "
                        << "The specified Game mode is (" << gameMode << ")."); 
                    gameMode = UNKNOWN_GAMEMODE;
                    break;
                }
            }
        }

        return gameMode;
    }

    const char8_t* GameSessionMetricsHandler::getGameMode() const
    {
        return getGameModeAsMetric(mGameSessionMaster->getGameMode());
    }

    const eastl::string GameSessionMetricsHandler::getPlatformForMetrics() const
    {
        return mGameSessionMaster->getPlatformForMetrics();
    }

    bool GameSessionMetricsHandler::isActiveGame() const
    {
        GameState currentGameState = mGameSessionMaster->getGameState();
        if (currentGameState != UNRESPONSIVE)
        {
            return ((currentGameState != INACTIVE_VIRTUAL) && (currentGameState != RESETABLE));
        }
        else
        {
            GameState preUnresponsiveState = mGameSessionMaster->getGameDataMaster()->getPreUnresponsiveState();
            return ((preUnresponsiveState != INACTIVE_VIRTUAL) && (preUnresponsiveState != RESETABLE));
        }
    }

    void GameSessionMetricsHandler::incrementActiveDedicatedServerMetrics()
    {
        if (isDedicatedHostedTopology(getGameData().getNetworkTopology()))
        {
            gGameManagerMaster->getMetrics().mActiveGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }

    void GameSessionMetricsHandler::decrementActiveDedicatedServerMetrics()
    {
        if (isDedicatedHostedTopology(getGameData().getNetworkTopology()))
        {
            gGameManagerMaster->getMetrics().mActiveGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }

    void GameSessionMetricsHandler::calcActiveSlotUpdatedMetrics(uint16_t oldTotalParticipantCapacity, uint16_t newTotalParticipantCapacity, uint16_t oldTotalSpectatorCapacity, uint16_t newTotalSpectatorCapacity)
    {
        if (isDedicatedHostedTopology(getGameData().getNetworkTopology()))
            calcSlotUpdatedMetrics(oldTotalParticipantCapacity, newTotalParticipantCapacity, oldTotalSpectatorCapacity, newTotalSpectatorCapacity, true);
    }

    void GameSessionMetricsHandler::incrementActiveVirtualGames()
    {
        if (getGameData().getNetworkTopology() == CLIENT_SERVER_DEDICATED && getGameData().getGameSettings().getVirtualized() )
        {
            gGameManagerMaster->getMetrics().mActiveVirtualGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }

    void GameSessionMetricsHandler::decrementActiveVirtualGames()
    {
        if (getGameData().getNetworkTopology() == CLIENT_SERVER_DEDICATED && getGameData().getGameSettings().getVirtualized() )
        {
            gGameManagerMaster->getMetrics().mActiveVirtualGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }

    void GameSessionMetricsHandler::incrementGamesByReputation(bool allowAnyReputation)
    {
        if (allowAnyReputation)
            gGameManagerMaster->getMetrics().mAllowAnyReputationGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        else
            gGameManagerMaster->getMetrics().mRequireGoodReputationGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
    }

    void GameSessionMetricsHandler::decrementGamesByReputation(bool allowAnyReputation)
    {
        if (allowAnyReputation)
            gGameManagerMaster->getMetrics().mAllowAnyReputationGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        else
            gGameManagerMaster->getMetrics().mRequireGoodReputationGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
    }

    void GameSessionMetricsHandler::incrementVirtualGames()
    {
        if (getGameData().getNetworkTopology() == CLIENT_SERVER_DEDICATED && getGameData().getGameSettings().getVirtualized() )
        {
            gGameManagerMaster->getMetrics().mAllVirtualGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }


    void GameSessionMetricsHandler::decrementVirtualGames()
    {
        if (getGameData().getNetworkTopology() == CLIENT_SERVER_DEDICATED && getGameData().getGameSettings().getVirtualized() )
        {
            gGameManagerMaster->getMetrics().mAllVirtualGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }

    /*! ************************************************************************************************/
    /*! \brief calc our metrics for the leaving player
    ***************************************************************************************************/
    void GameSessionMetricsHandler::onPlayerRemoved(const PlayerInfoMaster& leavingPlayer, const PlayerRemovedReason* playerRemovedReason, bool updateTotals)
    {
        if (leavingPlayer.isQueuedNotReserved()) // We do not modify the metrics for the queued player as we did not account for it when he was added.
        {
            return;
        }
        
        const bool isPlayerInQoSMMDataMap = mGameSessionMaster->isPlayerInMatchmakingQosDataMap(leavingPlayer.getPlayerId());
        int64_t playerSessionDurationSecs = 0;
        TimeValue joinedTime = leavingPlayer.getJoinedGameTimestamp();
        TimeValue removalTime = TimeValue::getTimeOfDay();

        if (updateTotals)
        {
            if (playerRemovedReason == nullptr)
            {
                EA_FAIL_MSG("Player removed reason must be valid when updating totals!");
                return;
            }
            // if the player never joined the game completely dont bother with the calculation
            if (joinedTime.getMicroSeconds() != JOINED_GAME_TIMESTAMP_NOT_JOINED)
            {
                TimeValue playDuration =  removalTime - leavingPlayer.getJoinedGameTimestamp();
                playerSessionDurationSecs = playDuration.getSec();
            }
            gGameManagerMaster->getMetrics().mGameSessionDurationSeconds.increment(playerSessionDurationSecs, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics(), *playerRemovedReason);
        }

        if (leavingPlayer.isActive())
        {
            if (updateTotals)
            {
                gGameManagerMaster->getMetrics().mActivePlayersRemoved.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), leavingPlayer.getUserInfo().getProductName(), leavingPlayer.getUserInfo().getBestPingSiteAlias());

                if (getGameData().getNetworkTopology() == Blaze::CLIENT_SERVER_DEDICATED)
                {
                    // braces are to avoid compiler errors for initialization of 'playerUserSession' & 'reason' being skipped by 'case' label
                    const char8_t *reason = ( *playerRemovedReason != GAME_ENDED ) ? "player left" : "game finished";
                    gGameManagerMaster->logDedicatedServerEvent(reason, *mGameSessionMaster);

                    StringBuilder removalBuf;
                    removalBuf << " Player session duration (" << playerSessionDurationSecs << ") seconds, Player removal reason '" <<  PlayerRemovedReasonToString(*playerRemovedReason) << "'";
                    gGameManagerMaster->logUserInfoForDedicatedServerEvent(reason, leavingPlayer.getPlayerId(), leavingPlayer.getLatencyMap(), *mGameSessionMaster, removalBuf.get() );                       
                }
            }

            gGameManagerMaster->getMetrics().mActivePlayers.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), leavingPlayer.getUserInfo().getProductName(), leavingPlayer.getUserInfo().getBestPingSiteAlias());

            if (leavingPlayer.isSpectator())
            {
                gGameManagerMaster->getMetrics().mActiveSpectators.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), leavingPlayer.getUserInfo().getProductName(), leavingPlayer.getUserInfo().getBestPingSiteAlias());
            }
        }
        else if (leavingPlayer.isReserved())
        {
            gGameManagerMaster->getMetrics().mReservedPlayers.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), leavingPlayer.getUserInfo().getProductName(), leavingPlayer.getUserInfo().getBestPingSiteAlias());

            if (leavingPlayer.isSpectator())
            {
                gGameManagerMaster->getMetrics().mReservedSpectators.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), leavingPlayer.getUserInfo().getProductName(), leavingPlayer.getUserInfo().getBestPingSiteAlias());
            }
        }

        if (updateTotals)
        {
            calcPlayerRemovedReasonMetrics(*playerRemovedReason, playerSessionDurationSecs, leavingPlayer, isPlayerInQoSMMDataMap);
            calcPlayerRemoveGameMetrics(*playerRemovedReason, leavingPlayer, removalTime);

            if (leavingPlayer.getJoinedViaMatchmaking())
            {
                calcPlayerRemoveScenarioMetrics(*playerRemovedReason, leavingPlayer, playerSessionDurationSecs, removalTime);
            }
        }
    }

    void GameSessionMetricsHandler::calcPlayerRemovedReasonMetrics(PlayerRemovedReason playerRemovedReason, int64_t playerSessionDurationSecs, const PlayerInfoMaster& leavingPlayer, bool playerInQoSMMDataMap)
    {
        //breakdown for each remove reasons
        gGameManagerMaster->getMetrics().mGameSessionDurationSeconds.increment(playerSessionDurationSecs, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics(), playerRemovedReason);
        gGameManagerMaster->getMetrics().mPlayersRemoved.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), leavingPlayer.getUserInfo().getProductName(), leavingPlayer.getUserInfo().getBestPingSiteAlias(), leavingPlayer.getJoinMethod(), playerRemovedReason);
        switch (playerRemovedReason)
        {
        case BLAZESERVER_CONN_LOST:
        case PLAYER_CONN_LOST:
            for (int i=0; i < MAX_THRESHOLD; ++i)
            {
                if (playerSessionDurationSecs < TIME_THRESHOLD[i] )
                {
                    gGameManagerMaster->getMetrics().mPlayerRemovedConnLostInTime.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), leavingPlayer.getUserInfo().getProductName(), leavingPlayer.getUserInfo().getBestPingSiteAlias(), TIME_THRESHOLD[i]);
                    break;
                }
            }
            break;
        case PLAYER_RESERVATION_TIMEOUT:
            if (leavingPlayer.hasExternalPlayerId() && (leavingPlayer.getOriginalPlayerSessionId() == 0))
            {
                gGameManagerMaster->getMetrics().mExternalPlayerRemovedReservationTimeout.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), leavingPlayer.getUserInfo().getProductName(), leavingPlayer.getUserInfo().getBestPingSiteAlias());
            }
            break;
        default:
            break;
        }
    }

    /*! ************************************************************************************************/
    /*! \brief calc added player metrics
    ***************************************************************************************************/
    void GameSessionMetricsHandler::onPlayerAdded(const PlayerInfoMaster& newPlayer, bool claimedReservation, bool updateTotals)
    {
        if (newPlayer.isQueuedNotReserved()) // We are not interested in queued player metrics when a player is added.
        {
            return;
        }

        if (newPlayer.isActive())
        {
            if (claimedReservation)
            {
                calcClaimedReservationMetrics(newPlayer);
            }
            if (updateTotals)
            {
                gGameManagerMaster->getMetrics().mPlayersJoined.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias(), newPlayer.getJoinMethod());
            }

            gGameManagerMaster->getMetrics().mActivePlayers.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());

            if (newPlayer.isSpectator())
            {
                gGameManagerMaster->getMetrics().mActiveSpectators.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
            }
        } 
        else if (newPlayer.isReserved())
        {
            onPlayerReservationAdded(newPlayer, false, updateTotals);

            if (newPlayer.isSpectator())
            {
                gGameManagerMaster->getMetrics().mReservedSpectators.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
            }
        }
    }

    void GameSessionMetricsHandler::calcClaimedReservationMetrics(const PlayerInfoMaster& newPlayer)
    {
        gGameManagerMaster->getMetrics().mReservedPlayers.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());

        if (newPlayer.isSpectator())
        {
            gGameManagerMaster->getMetrics().mReservedSpectators.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
        }

        if (newPlayer.getPlayerSettings().getHasDisconnectReservation())
        {
            gGameManagerMaster->getMetrics().mDisconnectReservationsClaimed.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
        }
        else
        {
            gGameManagerMaster->getMetrics().mPlayerReservationsClaimed.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
        }
    }

    void GameSessionMetricsHandler::onPlayerReservationClaimFailed(const PlayerInfoMaster* player, bool failedGameEntryCriteria)
    {
        const char8_t* product = "unset";
        const char8_t* pingsite = "unset";
        if (player != nullptr)
        {
            product = player->getUserInfo().getProductName();
            pingsite = player->getUserInfo().getBestPingSiteAlias();
        }

        gGameManagerMaster->getMetrics().mPlayerReservationsClaimedFailed.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), product, pingsite);
        if (failedGameEntryCriteria)
        {
            gGameManagerMaster->getMetrics().mPlayerReservationsClaimedFailedGameEntryCriteria.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), product, pingsite);
        }

        // if originally had no user session, reservation was for an external player.
        if ((player != nullptr) && player->hasExternalPlayerId() && (player->getOriginalPlayerSessionId() == 0))
        {
            gGameManagerMaster->getMetrics().mExternalPlayerReservationsClaimedFailed.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), product, pingsite);

            if (failedGameEntryCriteria)
            {
                gGameManagerMaster->getMetrics().mExternalPlayerReservationsClaimedFailedGameEntryCriteria.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), product, pingsite);
            }
        }
    }

    void GameSessionMetricsHandler::onGameSettingsChanged(const GameSettings& oldSettings, const GameSettings& newSettings)
    {
        bool oldSettingAnyReputation = oldSettings.getAllowAnyReputation();
        bool newSettingsAnyReputation = newSettings.getAllowAnyReputation();

        if (oldSettingAnyReputation != newSettingsAnyReputation)
        {
            decrementGamesByReputation(oldSettingAnyReputation);
            incrementGamesByReputation(newSettingsAnyReputation);
        }
    }

    void GameSessionMetricsHandler::onGameResponsivenessChanged(bool responsive)
    {
        if (responsive)
        {
            gGameManagerMaster->getMetrics().mUnresponsiveGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
        else
        {
            gGameManagerMaster->getMetrics().mUnresponsiveGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }

    void GameSessionMetricsHandler::onPlayerReservationAdded(const PlayerInfoMaster& newPlayer, bool wasActive, bool updateTotals)
    {
        if (updateTotals)
        {
            if (newPlayer.getPlayerSettings().getHasDisconnectReservation())
            {
                gGameManagerMaster->getMetrics().mDisconnectReservations.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
            }
            else
            {
                gGameManagerMaster->getMetrics().mPlayerReservations.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
            }

            if (newPlayer.hasExternalPlayerId() && ((newPlayer.getPlayerSessionId() == 0) || (newPlayer.getPlayerSessionId() == INVALID_USER_SESSION_ID)))
            {
                gGameManagerMaster->getMetrics().mExternalPlayerReservations.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
            }
        }

        gGameManagerMaster->getMetrics().mReservedPlayers.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());

        if (wasActive)
        {
            gGameManagerMaster->getMetrics().mActivePlayers.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());

            if (newPlayer.isSpectator())
            {
                gGameManagerMaster->getMetrics().mActiveSpectators.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlayer.getUserInfo().getProductName(), newPlayer.getUserInfo().getBestPingSiteAlias());
            }
        }
    }

    void GameSessionMetricsHandler::rebuildGameMetrics()
    {
        calcGameCreatedMetrics(false);
        calcPlayerSlotUpdatedMetrics(0, mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity());
        
        if (isDedicatedHostedTopology(getGameData().getNetworkTopology()) && isActiveGame())
        {
            incrementActiveDedicatedServerMetrics();
            calcActiveSlotUpdatedMetrics(0, mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity());
            if (mGameSessionMaster->getGameSettings().getVirtualized())
                incrementActiveVirtualGames();
        }

        if (getGameData().getGameState() == UNRESPONSIVE)
        {
            gGameManagerMaster->getMetrics().mUnresponsiveGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }

    void GameSessionMetricsHandler::teardownGameMetrics()
    {
        onGameDestroyed(false, SYS_GAME_ENDING);
    }

    void GameSessionMetricsHandler::teardownPlayerMetrics(const PlayerInfoMaster& leavingPlayer)
    {
        onPlayerRemoved(leavingPlayer, nullptr, false);
    }

    /*! ************************************************************************************************/
    /*! \brief calc metrics for game created. For dedicated servers, its inactive 'all' metric.
        For non dedicated servers, 'active' metric.
    ***************************************************************************************************/
    void GameSessionMetricsHandler::calcGameCreatedMetrics(bool updateTotals)
    {
        if (mGameCreateMetricsSet == true)
            return;

        mGameCreateMetricsSet = true;

        if (updateTotals)
        {
            gGameManagerMaster->getMetrics().mGamesCreated.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }

        if (mGameSessionMaster->isGameGroup())
        {
            gGameManagerMaster->getMetrics().mActiveGameGroups.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
            if (updateTotals && mGameSessionMaster->hasPersistedGameId())
            {
                gGameManagerMaster->getMetrics().mGamesCreatedByUniqueKey.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
            }
        }
        
        if (mGameSessionMaster->getGameSettings().getVirtualized())
        {
            incrementVirtualGames(); 
        }

        if (!mGameSessionMaster->isInactiveVirtual())
        { 
            // we don't count "VIRTUAL"-state games in these metrics.
            // peer mesh/hosted games go directly to active 
            if (!isDedicatedHostedTopology(getGameData().getNetworkTopology()))
            {
                gGameManagerMaster->getMetrics().mActiveGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
            }

            // all non-virtual games, regardless of topology are counted here
            gGameManagerMaster->getMetrics().mAllGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
        
        incrementGamesByReputation(getGameData().getGameSettings().getAllowAnyReputation());
    }

    /*! ************************************************************************************************/
    /*! \brief Update game metrics. For dedicated servers, its inactive 'all' metric.
        For non dedicated servers, 'active' metric.
    ***************************************************************************************************/
    void GameSessionMetricsHandler::calcGameDestroyedMetrics(bool updateTotals, GameDestructionReason destructionReason)
    {
        if (mGameCreateMetricsSet == false)
            return;

        mGameCreateMetricsSet = false;

        if (updateTotals)
        {
            gGameManagerMaster->getMetrics().mGamesDestroyed.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics(), destructionReason);
        }

        if (mGameSessionMaster->isGameGroup())
        {
            gGameManagerMaster->getMetrics().mActiveGameGroups.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }

        if (getGameData().getGameSettings().getVirtualized())
        {
            decrementVirtualGames();
        }

        // update metrics added from create
        if (!mGameSessionMaster->isInactiveVirtual())
        { 
            // we don't count "VIRTUAL"-state games in these metrics.
            // peer mesh/hosted games go directly to active 
            if (!isDedicatedHostedTopology(getGameData().getNetworkTopology()))
            {
                gGameManagerMaster->getMetrics().mActiveGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
            }

            // all non-virtual games, regardless of topology are counted here
            gGameManagerMaster->getMetrics().mAllGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }

        decrementGamesByReputation(getGameData().getGameSettings().getAllowAnyReputation());
    }

    /*! ****************************************************************************************************************************/
    /*! \brief update Slots metrics 
        \param[in] activeSlots: If true, update metrics for active slots (only applicable to dedicated topologies; 
                                  excludes slots for inactive virtual games and slots for games in the resettable pool).
                                If false, update metrics for all slots (applicable to all topologies;
                                  excludes only the slots for inactive virtual games).
    ********************************************************************************************************************************/
    void GameSessionMetricsHandler::calcSlotUpdatedMetrics(uint16_t oldTotalParticipantCapacity,
        uint16_t newTotalParticipantCapacity, uint16_t oldTotalSpectatorCapacity, uint16_t newTotalSpectatorCapacity, bool activeSlots)
    {
        auto& playerSlotsMetric = activeSlots ? gGameManagerMaster->getMetrics().mActivePlayerSlots : gGameManagerMaster->getMetrics().mAllPlayerSlots;
        auto& spectatorSlotsMetric = activeSlots ? gGameManagerMaster->getMetrics().mActiveSpectatorSlots : gGameManagerMaster->getMetrics().mAllSpectatorSlots;

        // the player slots metrics actually report total game capacity(participants + spectators)
        uint16_t newTotalPlayerCapacity = newTotalParticipantCapacity + newTotalSpectatorCapacity;
        uint16_t oldTotalPlayerCapacity = oldTotalParticipantCapacity + oldTotalSpectatorCapacity;

        bool& slotMetricsSet = activeSlots ? mActiveSlotMetricsSet : mAllSlotMetricsSet;
        if (newTotalPlayerCapacity == 0)
        {
            // Avoid double delete: 
            if (slotMetricsSet == false)
                return;

            slotMetricsSet = false;
        }
        else if (oldTotalPlayerCapacity == 0) 
        {
            // Avoid double add: 
            if (slotMetricsSet == true)
                return;

            slotMetricsSet = true;
        }

        if (oldTotalPlayerCapacity > newTotalPlayerCapacity)
        {
            const uint32_t difference = oldTotalPlayerCapacity - newTotalPlayerCapacity;
            playerSlotsMetric.decrement(difference, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
        else if (oldTotalPlayerCapacity < newTotalPlayerCapacity)
        {
            const uint32_t difference = newTotalPlayerCapacity - oldTotalPlayerCapacity;
            playerSlotsMetric.increment(difference, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }

        if (oldTotalSpectatorCapacity > newTotalSpectatorCapacity)
        {
            const uint32_t difference = oldTotalSpectatorCapacity - newTotalSpectatorCapacity;
            spectatorSlotsMetric.decrement(difference, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
        else if (oldTotalSpectatorCapacity < newTotalSpectatorCapacity)
        {
            const uint32_t difference = newTotalSpectatorCapacity - oldTotalSpectatorCapacity;
            spectatorSlotsMetric.increment(difference, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());
        }
    }

    void GameSessionMetricsHandler::calcPlayerSlotUpdatedMetrics(uint16_t oldTotalParticipantCapacity,
        uint16_t newTotalParticipantCapacity, uint16_t oldTotalSpectatorCapacity, uint16_t newTotalSpectatorCapacity)
    {
        if (!mGameSessionMaster->isInactiveVirtual())
            calcSlotUpdatedMetrics(oldTotalParticipantCapacity, newTotalParticipantCapacity, oldTotalSpectatorCapacity, newTotalSpectatorCapacity, false);
    }

    /*! ************************************************************************************************/
    /*! \brief update the gauge metrics that track game mode if the attribute is updated

        \param[in] oldGameMode  - the original game mode
        \param[in] newGameMode - the new game mode
    ***************************************************************************************************/
    void GameSessionMetricsHandler::onGameModeOrPlatformChanged(const char8_t* oldGameMode, const char8_t* newGameMode, const char8_t* oldPlatform, const char8_t* newPlatform)
    {
        // early out if we haven't initially created the metrics for this game session.
        if (!mGameCreateMetricsSet)
        {
            return;
        }

        oldGameMode = getGameModeAsMetric(oldGameMode);
        newGameMode = getGameModeAsMetric(newGameMode);

        if ((blaze_stricmp(oldGameMode, newGameMode) == 0) && (blaze_stricmp(oldPlatform, newPlatform) == 0))
        {
            return;
        }

        if (getGameData().getGameState() == UNRESPONSIVE)
        {
            gGameManagerMaster->getMetrics().mUnresponsiveGames.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
            gGameManagerMaster->getMetrics().mUnresponsiveGames.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);
        }

        // Slot counts can change when starting a new dedicated server game.  Be sure and update them there. 
        const uint16_t totalPlayerCapacity = mGameSessionMaster->getTotalPlayerCapacity();
        const uint16_t totalSpectatorCapacity = mGameSessionMaster->getTotalSpectatorCapacity();

        if (!mGameSessionMaster->isInactiveVirtual())
        {
            gGameManagerMaster->getMetrics().mAllPlayerSlots.decrement(totalPlayerCapacity, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
            gGameManagerMaster->getMetrics().mAllPlayerSlots.increment(totalPlayerCapacity, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);

            gGameManagerMaster->getMetrics().mAllSpectatorSlots.decrement(totalSpectatorCapacity, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
            gGameManagerMaster->getMetrics().mAllSpectatorSlots.increment(totalSpectatorCapacity, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);

            gGameManagerMaster->getMetrics().mAllGames.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
            gGameManagerMaster->getMetrics().mAllGames.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);
        }

        // update the active metrics even if unresponsive, as we don't dec those metrics as games go unresponsive/responsive
        if (isActiveGame())
        {
            if (getGameData().getGameType() == GAME_TYPE_GROUP)
            {
                gGameManagerMaster->getMetrics().mActiveGameGroups.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
                gGameManagerMaster->getMetrics().mActiveGameGroups.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);
            }

            // ACTIVE_GAMES includes game groups:
            gGameManagerMaster->getMetrics().mActiveGames.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
            gGameManagerMaster->getMetrics().mActiveGames.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);

            calcPlayerMetrics(oldGameMode, newGameMode, getGameData().getPingSiteAlias(), getGameData().getPingSiteAlias());
        }

        if (mGameSessionMaster->getGameSettings().getVirtualized())
        {
            gGameManagerMaster->getMetrics().mAllVirtualGames.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
            gGameManagerMaster->getMetrics().mAllVirtualGames.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);

            if (!mGameSessionMaster->isInactiveVirtual())
            {
                gGameManagerMaster->getMetrics().mActiveVirtualGames.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
                gGameManagerMaster->getMetrics().mActiveVirtualGames.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);
            }
        }

        if (getGameData().getGameSettings().getAllowAnyReputation())
        {
            gGameManagerMaster->getMetrics().mAllowAnyReputationGames.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
            gGameManagerMaster->getMetrics().mAllowAnyReputationGames.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);
        }
        else
        {
            gGameManagerMaster->getMetrics().mRequireGoodReputationGames.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), oldPlatform);
            gGameManagerMaster->getMetrics().mRequireGoodReputationGames.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), newPlatform);
        }
    }

    void GameSessionMetricsHandler::onPingSiteChanged(const char8_t* oldPingSite, const char8_t* newPingSite)
    {
        if (blaze_stricmp(oldPingSite, newPingSite) == 0)
        {
            return;
        }

        if (mGameSessionMaster->isInactiveVirtual() || !isActiveGame())
        {
            WARN_LOG("[GameSessionMetricsHandler].onPingSiteChanged: Attempting to change pingsite information on an inactive Game." <<
                     " This is currently unhandled, since the AllGames and ActiveGames metrics may not have been set yet.");
            return;
        }

        const uint16_t totalPlayerCapacity = mGameSessionMaster->getTotalPlayerCapacity();
        const uint16_t totalSpectatorCapacity = mGameSessionMaster->getTotalSpectatorCapacity();

        // decrement metrics for old ping site
        {
            if (mGameSessionMaster->isGameGroup())
            {
                gGameManagerMaster->getMetrics().mActiveGameGroups.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), oldPingSite, getPlatformForMetrics());
            }

            gGameManagerMaster->getMetrics().mAllGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), oldPingSite, getPlatformForMetrics());
            gGameManagerMaster->getMetrics().mActiveGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), oldPingSite, getPlatformForMetrics());
            gGameManagerMaster->getMetrics().mAllPlayerSlots.decrement(totalPlayerCapacity, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), oldPingSite, getPlatformForMetrics());
            gGameManagerMaster->getMetrics().mAllSpectatorSlots.decrement(totalSpectatorCapacity, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), oldPingSite, getPlatformForMetrics());
        }
        
        // increment metrics for new ping site
        {
            if (mGameSessionMaster->isGameGroup())
            {
                gGameManagerMaster->getMetrics().mActiveGameGroups.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), newPingSite, getPlatformForMetrics());
            }

            gGameManagerMaster->getMetrics().mAllGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), newPingSite, getPlatformForMetrics());
            gGameManagerMaster->getMetrics().mActiveGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), newPingSite, getPlatformForMetrics());
            gGameManagerMaster->getMetrics().mAllPlayerSlots.increment(totalPlayerCapacity, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), newPingSite, getPlatformForMetrics());
            gGameManagerMaster->getMetrics().mAllSpectatorSlots.increment(totalSpectatorCapacity, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), newPingSite, getPlatformForMetrics());
        }

        if (getGameData().getGameSettings().getAllowAnyReputation())
        {
            gGameManagerMaster->getMetrics().mAllowAnyReputationGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), oldPingSite, getPlatformForMetrics());
            gGameManagerMaster->getMetrics().mAllowAnyReputationGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), newPingSite, getPlatformForMetrics());
        }
        else
        {
            gGameManagerMaster->getMetrics().mRequireGoodReputationGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), oldPingSite, getPlatformForMetrics());
            gGameManagerMaster->getMetrics().mRequireGoodReputationGames.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), newPingSite, getPlatformForMetrics());
        }

        calcPlayerMetrics(getGameMode(), getGameMode(), oldPingSite, newPingSite);
    }

    void GameSessionMetricsHandler::calcPlayerMetrics(const char8_t* oldGameMode, const char8_t* newGameMode, const char8_t* oldPingSite, const char8_t* newPingSite)
    {
        if (blaze_stricmp(oldGameMode, newGameMode) == 0 && blaze_stricmp(oldPingSite, newPingSite) == 0)
        {
            return;
        }

        const PlayerRoster::PlayerInfoList activePlayers = mGameSessionMaster->getPlayerRoster()->getPlayers(PlayerRoster::ROSTER_PLAYERS);
        for( PlayerRoster::PlayerInfoList::const_iterator activeIter = activePlayers.begin(), activeEnd = activePlayers.end(); activeIter != activeEnd; ++activeIter)
        {
            const PlayerInfo& player = **activeIter;

            if (player.isActive())
            {
                gGameManagerMaster->getMetrics().mActivePlayers.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), oldPingSite, player.getUserInfo().getProductName(), player.getUserInfo().getBestPingSiteAlias());
                gGameManagerMaster->getMetrics().mActivePlayers.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), newPingSite, player.getUserInfo().getProductName(), player.getUserInfo().getBestPingSiteAlias());

                if (player.isSpectator())
                {
                    gGameManagerMaster->getMetrics().mActiveSpectators.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), oldPingSite, player.getUserInfo().getProductName(), player.getUserInfo().getBestPingSiteAlias());
                    gGameManagerMaster->getMetrics().mActiveSpectators.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), newPingSite, player.getUserInfo().getProductName(), player.getUserInfo().getBestPingSiteAlias());
                }
            }
            else if (player.isReserved())
            {
                gGameManagerMaster->getMetrics().mReservedPlayers.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), oldPingSite, player.getUserInfo().getProductName(), player.getUserInfo().getBestPingSiteAlias());
                gGameManagerMaster->getMetrics().mReservedPlayers.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), newPingSite, player.getUserInfo().getProductName(), player.getUserInfo().getBestPingSiteAlias());

                if (player.isSpectator())
                {
                    gGameManagerMaster->getMetrics().mReservedSpectators.decrement(1, getGameData().getGameType(), oldGameMode, getGameData().getNetworkTopology(), oldPingSite, player.getUserInfo().getProductName(), player.getUserInfo().getBestPingSiteAlias());
                    gGameManagerMaster->getMetrics().mReservedSpectators.increment(1, getGameData().getGameType(), newGameMode, getGameData().getNetworkTopology(), newPingSite, player.getUserInfo().getProductName(), player.getUserInfo().getBestPingSiteAlias());
                }
            }
        }
    }


    void GameSessionMetricsHandler::calcPlayerRemoveScenarioMetrics(PlayerRemovedReason playerRemovedReason, const PlayerInfoMaster& leavingPlayer, int64_t playerSessionDurationSecs, TimeValue removedTimestamp)
    {
        const char8_t* scenarioName = Metrics::Tag::NON_SCENARIO_NAME;
        const char8_t* variantName = Metrics::Tag::NON_SCENARIO_NAME;
        ScenarioVersion scenarioVersion = Metrics::Tag::NON_SCENARIO_VERSION;
        const char8_t* subsessionName = Metrics::Tag::NON_SCENARIO_NAME;

        ScenarioInfo scenarioInfo = leavingPlayer.getPlayerDataMaster()->getScenarioInfo();

        if (scenarioInfo.getScenarioName()[0] != '\0')
        {
            scenarioName = scenarioInfo.getScenarioName();
            variantName = scenarioInfo.getScenarioVariant();
            scenarioVersion = scenarioInfo.getScenarioVersion();
            subsessionName = scenarioInfo.getSubSessionName();
        }

        gGameManagerMaster->getMetrics().mPlayers.increment(1, scenarioName, variantName, scenarioVersion, subsessionName);
        gGameManagerMaster->getMetrics().mDurationInGameSession.increment((uint64_t)playerSessionDurationSecs, scenarioName, variantName, scenarioVersion, subsessionName);

        if (leavingPlayer.getPlayerDataMaster()->getFinishedFirstMatch())
        {
            gGameManagerMaster->getMetrics().mFinishedMatches.increment(1, scenarioName, variantName, scenarioVersion, subsessionName);
            gGameManagerMaster->getMetrics().mDurationInMatch.increment((uint64_t)leavingPlayer.getPlayerDataMaster()->getDurationInFirstMatchesSec(), scenarioName, variantName, scenarioVersion, subsessionName);
        }
        else
        {
            if (mGameSessionMaster->isGameInProgress())
            {
                int64_t matchDuration = (leavingPlayer.getJoinedGameTimestamp() > mGameSessionMaster->getGameDataMaster()->getGameStartTime().getMicroSeconds()) ? removedTimestamp.getSec() - leavingPlayer.getJoinedGameTimestamp().getSec() : removedTimestamp.getSec() - mGameSessionMaster->getGameDataMaster()->getGameStartTime().getSec();
                gGameManagerMaster->getMetrics().mDurationInMatch.increment((uint64_t)matchDuration, scenarioName, variantName, scenarioVersion, subsessionName);
            }
            gGameManagerMaster->getMetrics().mLeftMatchEarly.increment(1, scenarioName, variantName, scenarioVersion, subsessionName);
        }
    }

    void GameSessionMetricsHandler::calcPlayerRemoveGameMetrics(PlayerRemovedReason playerRemovedReason, const PlayerInfoMaster& leavingPlayer, TimeValue leavingTimestamp)
    {
        // Do not update metrics if player is leaving in POST_GAME or due to game destruction
        // This is because the player's metrics would have been updated already in calcGameFinishedMetrics()
        if (mGameSessionMaster->getGameState() != POST_GAME && playerRemovedReason != GAME_DESTROYED)
        {
            TimeValue sessionLength;
            if (mGameSessionMaster->isGameInProgress())
            {
                sessionLength = (leavingPlayer.getJoinedGameTimestamp() > mGameSessionMaster->getGameDataMaster()->getGameStartTime().getMicroSeconds()) ? leavingTimestamp - leavingPlayer.getJoinedGameTimestamp() : leavingTimestamp - mGameSessionMaster->getGameDataMaster()->getGameStartTime();
            }

            GameModeByJoinMethodType gameJoinMethod = GAMEMODE_BY_INVALID;
            if (leavingPlayer.getPlayerDataMaster()->getFinishedFirstMatch() && mGameSessionMaster->isGameInProgress())
            {
                // if this is not the first round, leaving in pre-game doesn't count as a left early (hence isGameInProgress check)
                gameJoinMethod = GAMEMODE_BY_REMATCH;
            }
            else if (!leavingPlayer.getPlayerDataMaster()->getFinishedFirstMatch())
            {
                if (leavingPlayer.getJoinedViaMatchmaking())
                {
                    gameJoinMethod = GAMEMODE_BY_MATCHMAKING;
                }
                else
                {
                    gameJoinMethod = GAMEMODE_BY_DIRECTJOIN;
                }
            }

            if (gameJoinMethod != GAMEMODE_BY_INVALID)
            {
                gGameManagerMaster->getMetrics().mGameModeStarts.increment(1, getGameMode(), gameJoinMethod);
                gGameManagerMaster->getMetrics().mGameModeLeftEarly.increment(1, getGameMode(), gameJoinMethod);
                gGameManagerMaster->getMetrics().mGameModeMatchDuration.increment(sessionLength.getSec(), getGameMode(), gameJoinMethod);
            }
        }
    }

    void GameSessionMetricsHandler::onGameFinished()
    {
        TimeValue now = TimeValue::getTimeOfDay();
        TimeValue sessionLength = now - mGameSessionMaster->getGameDataMaster()->getGameStartTime();
        for (int i=0; i<MAX_THRESHOLD; ++i)
        {
            if (sessionLength.getSec() < TIME_THRESHOLD[i])
            {
                gGameManagerMaster->getMetrics().mGamesFinishedInTime.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics(), TIME_THRESHOLD[i]);
                break;
            }
        }

        // increment total game count regardless of threshold
        gGameManagerMaster->getMetrics().mGamesFinished.increment(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());

        // update game mode metrics
        uint64_t matchmakingFinishedMatch = 0;
        uint64_t directJoinFinishedMatch = 0;
        uint64_t rematchFinishedMatch = 0;

        uint64_t matchmakingMatchesStarted = 0;
        uint64_t directJoinMatchesStarted = 0;
        uint64_t rematchMatchesStarted = 0;

        int64_t matchmakingMatchDuration = 0;
        int64_t directJoinMatchDuration = 0;
        int64_t rematchMatchDuration = 0;

        for (auto& activePlayerIter : mGameSessionMaster->getPlayerRoster()->getPlayers(PlayerRoster::ACTIVE_PLAYERS))
        {
            PlayerInfoMaster *activePlayer = static_cast<PlayerInfoMaster*>(activePlayerIter);
            TimeValue playerSessionLength = sessionLength;
            if (activePlayer->getJoinedGameTimestamp() > mGameSessionMaster->getGameDataMaster()->getGameStartTime().getMicroSeconds())
            {
                // Player entered game mid game.
                playerSessionLength = now - activePlayer->getJoinedGameTimestamp();
            }

            if (activePlayer->getPlayerDataMaster()->getFinishedFirstMatch())
            {
                ++rematchMatchesStarted;
                ++rematchFinishedMatch;
                rematchMatchDuration += playerSessionLength.getSec();
            }
            else if (activePlayer->getJoinedViaMatchmaking())
            {
                ++matchmakingMatchesStarted;
                ++matchmakingFinishedMatch;
                matchmakingMatchDuration += playerSessionLength.getSec();
                activePlayer->setDurationInFirstMatchesSec(playerSessionLength.getSec());
                activePlayer->setFinishedFirstMatch();
            }
            else
            {
                ++directJoinMatchesStarted;
                ++directJoinFinishedMatch;
                directJoinMatchDuration += playerSessionLength.getSec();
                activePlayer->setFinishedFirstMatch();
            }
        }

        gGameManagerMaster->getMetrics().mGameModeFinishedMatch.increment(matchmakingFinishedMatch, getGameMode(), GAMEMODE_BY_MATCHMAKING);
        gGameManagerMaster->getMetrics().mGameModeFinishedMatch.increment(directJoinFinishedMatch, getGameMode(), GAMEMODE_BY_DIRECTJOIN);
        gGameManagerMaster->getMetrics().mGameModeFinishedMatch.increment(rematchFinishedMatch, getGameMode(), GAMEMODE_BY_REMATCH);

        gGameManagerMaster->getMetrics().mGameModeMatchDuration.increment((uint64_t)matchmakingMatchDuration, getGameMode(), GAMEMODE_BY_MATCHMAKING);
        gGameManagerMaster->getMetrics().mGameModeMatchDuration.increment((uint64_t)directJoinMatchDuration, getGameMode(), GAMEMODE_BY_DIRECTJOIN);
        gGameManagerMaster->getMetrics().mGameModeMatchDuration.increment((uint64_t)rematchMatchDuration, getGameMode(), GAMEMODE_BY_REMATCH);

        gGameManagerMaster->getMetrics().mGameModeStarts.increment(matchmakingMatchesStarted, getGameMode(), GAMEMODE_BY_MATCHMAKING);
        gGameManagerMaster->getMetrics().mGameModeStarts.increment(directJoinMatchesStarted, getGameMode(), GAMEMODE_BY_DIRECTJOIN);
        gGameManagerMaster->getMetrics().mGameModeStarts.increment(rematchMatchesStarted, getGameMode(), GAMEMODE_BY_REMATCH);
    }

    void GameSessionMetricsHandler::onHostEjected()
    {
        calcPlayerSlotUpdatedMetrics(mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity(), 0);

        // virtual games aren't part of the ALL_GAMES metric, so dec here.
        gGameManagerMaster->getMetrics().mAllGames.decrement(1, getGameData().getGameType(), getGameMode(), getGameData().getNetworkTopology(), getGameData().getPingSiteAlias(), getPlatformForMetrics());

        decrementActiveVirtualGames();
        decrementActiveDedicatedServerMetrics();
        calcActiveSlotUpdatedMetrics(mGameSessionMaster->getTotalParticipantCapacity(), 0, mGameSessionMaster->getTotalSpectatorCapacity(), 0);

    }
}// namespace GameManager
}// namespace Blaze

