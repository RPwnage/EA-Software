/*! ************************************************************************************************/
/*!
    \file gamemanagermetrics.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_GAMEMANAGER_METRICS_H
#define BLAZE_GAMEMANAGER_GAMEMANAGER_METRICS_H

#include "framework/metrics/metrics.h"
#include "gamemanager/tdf/gamemanagermetrics_server.h"

namespace Blaze
{

namespace Metrics
{
    using GamePingSite = const char8_t*;

    namespace Tag
    {
        extern TagInfo<const char8_t*>* game_mode;
        extern TagInfo<GameManager::GameType>* game_type;
        extern TagInfo<GameNetworkTopology>* network_topology;
        extern TagInfo<GameManager::JoinMethod>* join_method;
        extern TagInfo<GameManager::PlayerRemovedReason>* player_removed_reason;
        extern TagInfo<GameManager::GameDestructionReason>* game_destruction_reason;

        extern TagInfo<GamePingSite>* game_pingsite;
        extern TagInfo<int32_t>* time_threshold;
        extern TagInfo<GameManager::DemanglerConnectionHealthcheck>* demangler_connection_healthcheck;

        extern TagInfo<GameManager::ConnectionJoinType>* connection_join_type;
        extern TagInfo<GameManager::ConnSuccessMetricEnum>* connection_result_type;
        extern TagInfo<VoipTopology>* voip_topology;
        extern TagInfo<GameManager::ConnImpactingMetricEnum>* connection_impacting_type;
        extern TagInfo<GameManager::ConnConciergeModeMetricEnum>* cc_mode;
        extern TagInfo<GameManager::ConnConciergeEligibleEnum>* cc_eligible_type;
        extern TagInfo<GameManager::ConnDemanglerMetricEnum>* connection_demangler_type;

        extern TagInfo<uint32_t>* index; // used for metrics that store an array of values

        extern TagInfo<GameManager::ScenarioName>* scenario_name;
        extern TagInfo<GameManager::ScenarioVariantName>* scenario_variant_name;
        extern TagInfo<GameManager::ScenarioVersion>* scenario_version;
        extern TagInfo<GameManager::SubSessionName>* subsession_name;

        extern TagInfo<GameManager::GameModeByJoinMethodType>* game_mode_by_join_method;

        extern const char8_t* NON_SCENARIO_NAME;
        extern const GameManager::ScenarioVersion NON_SCENARIO_VERSION;
    }
}

namespace GameManager
{
    /*! ************************************************************************************************/
    /*! \brief holding the healthcheck metrics captured for the gamemanager

        see https://docs.developer.ea.com/display/blaze/Blaze+Health+Check+Definitions for details
    ***************************************************************************************************/
    class GameManagerMasterMetrics
    {
    public:
#define STANDARD_TAGS Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite
#define STANDARD_GAME_TAGS STANDARD_TAGS, Metrics::Tag::platform_list
#define STANDARD_PLAYER_TAGS STANDARD_TAGS, Metrics::Tag::product_name, Metrics::Tag::pingsite
        GameManagerMasterMetrics(Metrics::MetricsCollection& collection)
            : mMetricsCollection(collection)
            , mAllGames(mMetricsCollection, "allGames", STANDARD_GAME_TAGS)
            , mActiveGames(mMetricsCollection, "activeGames", STANDARD_GAME_TAGS)
            , mActiveGameGroups(mMetricsCollection, "activeGameGroups", STANDARD_GAME_TAGS)
            , mUnresponsiveGames(mMetricsCollection, "unresponsiveGames", STANDARD_GAME_TAGS)
            , mAllVirtualGames(mMetricsCollection, "allVirtualGames", STANDARD_GAME_TAGS)
            , mActiveVirtualGames(mMetricsCollection, "activeVirtualGames", STANDARD_GAME_TAGS)
            , mGamesCreated(mMetricsCollection, "gamesCreated", STANDARD_GAME_TAGS)
            , mGamesStarted(mMetricsCollection, "gamesStarted", STANDARD_GAME_TAGS)
            , mGamesFinished(mMetricsCollection, "gamesFinished", STANDARD_GAME_TAGS)
            , mGamesDestroyed(mMetricsCollection, "gamesDestroyed", STANDARD_GAME_TAGS, Metrics::Tag::game_destruction_reason)
            , mGamesFinishedInTime(mMetricsCollection, "gamesFinishedInTime", STANDARD_GAME_TAGS, Metrics::Tag::time_threshold)
            , mGamesCreatedByUniqueKey(mMetricsCollection, "gamesCreatedByUniqueKey", STANDARD_GAME_TAGS)
            , mActivePlayers(mMetricsCollection, "activePlayers", STANDARD_PLAYER_TAGS)
            , mActiveSpectators(mMetricsCollection, "activeSpectators", STANDARD_PLAYER_TAGS)
            , mPlayersJoined(mMetricsCollection, "playersJoined", STANDARD_PLAYER_TAGS, Metrics::Tag::join_method)
            , mPlayersRemoved(mMetricsCollection, "playersRemoved", STANDARD_PLAYER_TAGS, Metrics::Tag::join_method, Metrics::Tag::player_removed_reason)
            , mActivePlayersRemoved(mMetricsCollection, "activePlayersRemoved", STANDARD_PLAYER_TAGS)
            , mReservedPlayers(mMetricsCollection, "reservedPlayers", STANDARD_PLAYER_TAGS)
            , mReservedSpectators(mMetricsCollection, "reservedSpectators", STANDARD_PLAYER_TAGS)
            , mPlayerReservations(mMetricsCollection, "playerReservations", STANDARD_PLAYER_TAGS)
            , mPlayerReservationsClaimed(mMetricsCollection, "playerReservationsClaimed", STANDARD_PLAYER_TAGS)
            , mPlayerReservationsClaimedFailed(mMetricsCollection, "playerReservationsClaimedFailed", STANDARD_PLAYER_TAGS)
            , mPlayerReservationsClaimedFailedGameEntryCriteria(mMetricsCollection, "playerReservationsClaimedFailedGameEntryCriteria", STANDARD_PLAYER_TAGS)
            , mExternalPlayerReservationsClaimedFailed(mMetricsCollection, "externalPlayerReservationsClaimedFailed", STANDARD_PLAYER_TAGS)
            , mExternalPlayerReservationsClaimedFailedGameEntryCriteria(mMetricsCollection, "externalPlayerReservationsClaimedFailedGameEntryCriteria", STANDARD_PLAYER_TAGS)
            , mExternalPlayerReservations(mMetricsCollection, "externalPlayerReservations", STANDARD_PLAYER_TAGS)
            , mDisconnectReservations(mMetricsCollection, "disconnectReservations", STANDARD_PLAYER_TAGS)
            , mDisconnectReservationsClaimed(mMetricsCollection, "disconnectReservationsClaimed", STANDARD_PLAYER_TAGS)
            , mAllPlayerSlots(mMetricsCollection, "allPlayerSlots", STANDARD_GAME_TAGS)
            , mAllSpectatorSlots(mMetricsCollection, "allSpectatorSlots", STANDARD_GAME_TAGS)
            , mActivePlayerSlots(mMetricsCollection, "activePlayerSlots", STANDARD_GAME_TAGS)
            , mActiveSpectatorSlots(mMetricsCollection, "activeSpectatorSlots", STANDARD_GAME_TAGS)
            , mAllowAnyReputationGames(mMetricsCollection, "allowAnyReputationGames", STANDARD_GAME_TAGS)
            , mRequireGoodReputationGames(mMetricsCollection, "requireGoodReputationGames", STANDARD_GAME_TAGS)
            , mGameSessionDurationSeconds(mMetricsCollection, "gameSessionDurationSeconds", STANDARD_GAME_TAGS, Metrics::Tag::player_removed_reason)
            , mPlayerRemovedConnLostInTime(mMetricsCollection, "playerRemovedConnLostInTime", STANDARD_PLAYER_TAGS, Metrics::Tag::time_threshold)
            , mExternalPlayerChangedToLoggedInPlayer(mMetricsCollection, "externalPlayerChangedToLoggedInPlayer")
            , mExternalPlayerRemovedReservationTimeout(mMetricsCollection, "externalPlayerRemovedReservationTimeout", STANDARD_PLAYER_TAGS)
            , mInGamePlayerSessions(mMetricsCollection, "inGamePlayerSessions", Metrics::Tag::game_type, Metrics::Tag::pingsite, Metrics::Tag::product_name)
            , mDedicatedServerLookups(mMetricsCollection, "dedicatedServerLookups")
            , mDedicatedServerLookupFailures(mMetricsCollection, "dedicatedServerLookupFailures")
            , mDedicatedServerLookupsNoGames(mMetricsCollection, "dedicatedServerLookupsNoGames")
            , mExternalSessionSyncChecks(mMetricsCollection, "externalSessionSyncChecks", STANDARD_GAME_TAGS)
            , mExternalSessionsDesynced(mMetricsCollection, "externalSessionsDesynced", STANDARD_GAME_TAGS)
            , mExternalSessionsDesyncedMembers(mMetricsCollection, "externalSessionsDesyncedMembers", STANDARD_GAME_TAGS)
            , mDemangler(mMetricsCollection, "demangler", Metrics::Tag::network_topology, Metrics::Tag::pingsite, Metrics::Tag::demangler_connection_healthcheck)
            , mQosRuleDemangler(mMetricsCollection, "qosRuleDemangler", Metrics::Tag::network_topology, Metrics::Tag::pingsite, Metrics::Tag::demangler_connection_healthcheck)
            , mExternalSessionFailuresNonImpacting(mMetricsCollection, "externalSessionFailuresNonImpacting", Metrics::Tag::platform)
            , mGameConns(mMetricsCollection, "gameConns", Metrics::Tag::connection_join_type, Metrics::Tag::game_pingsite, Metrics::Tag::connection_result_type, Metrics::Tag::network_topology, Metrics::Tag::connection_impacting_type, Metrics::Tag::cc_mode, Metrics::Tag::connection_demangler_type, Metrics::Tag::cc_eligible_type)
            , mVoipConns(mMetricsCollection, "voipConns", Metrics::Tag::connection_join_type, Metrics::Tag::game_pingsite, Metrics::Tag::connection_result_type, Metrics::Tag::voip_topology, Metrics::Tag::connection_impacting_type, Metrics::Tag::cc_mode, Metrics::Tag::connection_demangler_type, Metrics::Tag::cc_eligible_type)
            , mCurrentPacketLoss(mMetricsCollection, "currentPacketLoss", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mMaxConnectionLatency(mMetricsCollection, "maxConnectionLatency", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mAvgConnectionLatency(mMetricsCollection, "avgConnectionLatency", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mConnectionLatencySample(mMetricsCollection, "connectionLatencySample", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mConnectionPacketLoss(mMetricsCollection, "connectionPacketLoss", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mConnectionPacketLossSample(mMetricsCollection, "connectionPacketLossSample", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mConnectionLatencySampleQosRule(mMetricsCollection, "connectionLatencySampleQosRule", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mMaxConnectionLatencyQosRule(mMetricsCollection, "maxConnectionLatencySampleQosRule", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mAvgConnectionLatencyQosRule(mMetricsCollection, "avgConnectionLatencySampleQosRule", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mConnectionPacketLossQosRule(mMetricsCollection, "connectionPacketLossQosRule", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mConnectionPacketLossSampleQosRule(mMetricsCollection, "connectionPacketLossSampleQosRule", Metrics::Tag::index, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite)
            , mDurationInGameSession(mMetricsCollection, "durationInGameSession", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
            , mDurationInMatch(mMetricsCollection, "durationInMatch", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
            , mPlayers(mMetricsCollection, "players", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
            , mLeftMatchEarly(mMetricsCollection, "playersLeftMatchEarly", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
            , mFinishedMatches(mMetricsCollection, "playerFinishedMatches", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
            , mGameModeStarts(mMetricsCollection, "gameModeTotalStarts", Metrics::Tag::game_mode, Metrics::Tag::game_mode_by_join_method)
            , mGameModeMatchDuration(mMetricsCollection, "gameModeTotalMatchDuration", Metrics::Tag::game_mode, Metrics::Tag::game_mode_by_join_method)
            , mGameModeLeftEarly(mMetricsCollection, "gameModeTotalLeftEarly", Metrics::Tag::game_mode, Metrics::Tag::game_mode_by_join_method)
            , mGameModeFinishedMatch(mMetricsCollection, "gameModeTotalFinishedMatch", Metrics::Tag::game_mode, Metrics::Tag::game_mode_by_join_method)
        {
#undef STANDARD_TAGS
#undef STANDARD_GAME_TAGS
#undef STANDARD_PLAYER_TAGS

            mActiveGames.defineTagGroups({
                { Metrics::Tag::network_topology, Metrics::Tag::platform_list },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            mActiveGameGroups.defineTagGroups({
                { Metrics::Tag::game_mode },
                { Metrics::Tag::platform_list } });

            // The commented out tags are done because there's a balance to be had between the number of nodes created, and how much time the lookup takes.
            // 
            mActivePlayers.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type },
                //{ Metrics::Tag::game_type, Metrics::Tag::game_mode },
                //{ Metrics::Tag::game_type, Metrics::Tag::pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                // FIFA SPECIFIC CODE START
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite },
				// FIFA SPECIFIC CODE END
                { Metrics::Tag::game_type, Metrics::Tag::product_name },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::product_name },
                { Metrics::Tag::game_type, Metrics::Tag::network_topology, Metrics::Tag::product_name } });

            mActivePlayerSlots.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mActiveSpectators.defineTagGroups({
                { Metrics::Tag::network_topology, Metrics::Tag::product_name },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::product_name },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::product_name },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::product_name } });

            mActiveSpectatorSlots.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mActiveVirtualGames.defineTagGroups({
                { Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mAllowAnyReputationGames.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mAllGames.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_pingsite, Metrics::Tag::platform_list  } });

            mAllPlayerSlots.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mAllSpectatorSlots.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology,Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mAllVirtualGames.defineTagGroups({
                { Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mInGamePlayerSessions.defineTagGroups({
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::product_name } });

            mRequireGoodReputationGames.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mReservedPlayers.defineTagGroups({
                { Metrics::Tag::network_topology, Metrics::Tag::product_name },
                { Metrics::Tag::game_type, Metrics::Tag::product_name },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::product_name } });

            mReservedSpectators.defineTagGroups({
                { Metrics::Tag::network_topology, Metrics::Tag::product_name },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::product_name } });

            mUnresponsiveGames.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mDemangler.defineTagGroups({
                { Metrics::Tag::network_topology, Metrics::Tag::demangler_connection_healthcheck, Metrics::Tag::pingsite } });

            mExternalSessionSyncChecks.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mGamesCreated.defineTagGroups({
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mGamesDestroyed.defineTagGroups({
                { Metrics::Tag::game_destruction_reason },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mGamesFinished.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mGamesCreatedByUniqueKey.defineTagGroups({
                { Metrics::Tag::game_mode } });

            mGameSessionDurationSeconds.defineTagGroups({
                { Metrics::Tag::player_removed_reason },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::player_removed_reason, Metrics::Tag::game_mode } });

            mGamesFinishedInTime.defineTagGroups({
                { Metrics::Tag::time_threshold },
                { Metrics::Tag::time_threshold, Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mGamesStarted.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            mPlayersJoined.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type },
                { Metrics::Tag::join_method },
                //{ Metrics::Tag::game_type, Metrics::Tag::game_mode },
                //{ Metrics::Tag::game_type, Metrics::Tag::join_method },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::join_method } });

            mPlayerRemovedConnLostInTime.defineTagGroups({
                { Metrics::Tag::time_threshold },
                //{ Metrics::Tag::time_threshold, Metrics::Tag::pingsite },
                { Metrics::Tag::time_threshold, Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });


            mActivePlayersRemoved.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite } });

            mPlayersRemoved.defineTagGroups({
                { Metrics::Tag::player_removed_reason },
                //{ Metrics::Tag::pingsite, Metrics::Tag::player_removed_reason },
                //{ Metrics::Tag::network_topology, Metrics::Tag::player_removed_reason },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::player_removed_reason },
                { Metrics::Tag::join_method, Metrics::Tag::player_removed_reason },
                //{ Metrics::Tag::game_type, Metrics::Tag::player_removed_reason },
                //{ Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::player_removed_reason },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::player_removed_reason } });

            mPlayerReservations.defineTagGroups({
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mPlayerReservationsClaimed.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mPlayerReservationsClaimedFailed.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mPlayerReservationsClaimedFailedGameEntryCriteria.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mExternalPlayerReservations.defineTagGroups({
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mExternalPlayerReservationsClaimedFailed.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            mExternalPlayerReservationsClaimedFailedGameEntryCriteria.defineTagGroups({
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });
        }

    private:
        Metrics::MetricsCollection& mMetricsCollection;

    public:
#define STANDARD_TAGS GameType, Metrics::GameMode, GameNetworkTopology, Metrics::GamePingSite
#define STANDARD_GAME_TAGS STANDARD_TAGS, Metrics::PlatformList
#define STANDARD_PLAYER_TAGS STANDARD_TAGS, Metrics::ProductName, Metrics::PingSite

        using GameMetricCounter = Metrics::TaggedCounter<STANDARD_GAME_TAGS>;
        using GameMetricGauge = Metrics::TaggedGauge<STANDARD_GAME_TAGS>;

        using PlayerMetricCounter = Metrics::TaggedCounter<STANDARD_PLAYER_TAGS>;
        using PlayerMetricGauge = Metrics::TaggedGauge<STANDARD_PLAYER_TAGS>;

        GameMetricGauge mAllGames;
        GameMetricGauge mActiveGames;
        GameMetricGauge mActiveGameGroups;
        GameMetricGauge mUnresponsiveGames;
        GameMetricGauge mAllVirtualGames;
        GameMetricGauge mActiveVirtualGames;

        GameMetricCounter mGamesCreated;
        GameMetricCounter mGamesStarted;
        GameMetricCounter mGamesFinished;
        Metrics::TaggedCounter<STANDARD_GAME_TAGS, GameDestructionReason> mGamesDestroyed;
        Metrics::TaggedCounter<STANDARD_GAME_TAGS, int> mGamesFinishedInTime;
        GameMetricCounter mGamesCreatedByUniqueKey;

        Metrics::TaggedGauge<STANDARD_PLAYER_TAGS> mActivePlayers;
        PlayerMetricGauge mActiveSpectators;

        Metrics::TaggedCounter<STANDARD_PLAYER_TAGS, JoinMethod> mPlayersJoined;
        Metrics::TaggedCounter<STANDARD_PLAYER_TAGS, JoinMethod, PlayerRemovedReason> mPlayersRemoved; // regardless of status (active or not)
        PlayerMetricCounter mActivePlayersRemoved;

        PlayerMetricGauge mReservedPlayers;
        PlayerMetricGauge mReservedSpectators;

        PlayerMetricCounter mPlayerReservations;
        PlayerMetricCounter mPlayerReservationsClaimed;
        PlayerMetricCounter mPlayerReservationsClaimedFailed;
        PlayerMetricCounter mPlayerReservationsClaimedFailedGameEntryCriteria;
        PlayerMetricCounter mExternalPlayerReservationsClaimedFailed;
        PlayerMetricCounter mExternalPlayerReservationsClaimedFailedGameEntryCriteria;
        PlayerMetricCounter mExternalPlayerReservations;
        PlayerMetricCounter mDisconnectReservations;
        PlayerMetricCounter mDisconnectReservationsClaimed;

        GameMetricGauge mAllPlayerSlots;
        GameMetricGauge mAllSpectatorSlots;
        GameMetricGauge mActivePlayerSlots;
        GameMetricGauge mActiveSpectatorSlots;

        GameMetricGauge mAllowAnyReputationGames;
        GameMetricGauge mRequireGoodReputationGames;

        Metrics::TaggedCounter<STANDARD_GAME_TAGS, PlayerRemovedReason> mGameSessionDurationSeconds;
        Metrics::TaggedCounter<STANDARD_PLAYER_TAGS, int> mPlayerRemovedConnLostInTime;
        Metrics::Counter mExternalPlayerChangedToLoggedInPlayer;
        PlayerMetricCounter mExternalPlayerRemovedReservationTimeout;

        Metrics::TaggedGauge<GameType, Metrics::PingSite, Metrics::ProductName> mInGamePlayerSessions;

        Metrics::Counter mDedicatedServerLookups;
        Metrics::Counter mDedicatedServerLookupFailures;
        Metrics::Counter mDedicatedServerLookupsNoGames;

        GameMetricCounter mExternalSessionSyncChecks;
        GameMetricCounter mExternalSessionsDesynced;
        GameMetricCounter mExternalSessionsDesyncedMembers;

        Metrics::TaggedCounter<GameNetworkTopology, Metrics::PingSite, DemanglerConnectionHealthcheck> mDemangler;
        Metrics::TaggedCounter<GameNetworkTopology, Metrics::PingSite, DemanglerConnectionHealthcheck> mQosRuleDemangler;

        Metrics::TaggedCounter<ClientPlatformType> mExternalSessionFailuresNonImpacting;

        // connection success/failure counts
        Metrics::TaggedCounter<ConnectionJoinType, Metrics::GamePingSite, ConnSuccessMetricEnum, GameNetworkTopology, ConnImpactingMetricEnum, ConnConciergeModeMetricEnum, ConnDemanglerMetricEnum, ConnConciergeEligibleEnum> mGameConns;
        Metrics::TaggedCounter<ConnectionJoinType, Metrics::GamePingSite, ConnSuccessMetricEnum, VoipTopology, ConnImpactingMetricEnum, ConnConciergeModeMetricEnum, ConnDemanglerMetricEnum, ConnConciergeEligibleEnum> mVoipConns;

        using ConnectionMetricCounter = Metrics::TaggedCounter<uint32_t, GameNetworkTopology, Metrics::GamePingSite>;
        using ConnectionMetricGauge = Metrics::TaggedGauge<uint32_t, GameNetworkTopology, Metrics::GamePingSite>;

        // player-facing connection QoS performance
        ConnectionMetricGauge mCurrentPacketLoss;
        ConnectionMetricCounter mMaxConnectionLatency;
        ConnectionMetricCounter mAvgConnectionLatency;
        ConnectionMetricCounter mConnectionLatencySample;
        ConnectionMetricCounter mConnectionPacketLoss;
        ConnectionMetricCounter mConnectionPacketLossSample;

        // non-player-facing connection QoS performance
        ConnectionMetricCounter mConnectionLatencySampleQosRule;
        ConnectionMetricCounter mMaxConnectionLatencyQosRule;
        ConnectionMetricCounter mAvgConnectionLatencyQosRule;
        ConnectionMetricCounter mConnectionPacketLossQosRule;
        ConnectionMetricCounter mConnectionPacketLossSampleQosRule;

        using ScenarioMetricCounter = Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, SubSessionName>;

        ScenarioMetricCounter mDurationInGameSession; // Aggregate of the time spent in game session, in seconds
        ScenarioMetricCounter mDurationInMatch; // Aggregate of the time spent in game, in seconds
        ScenarioMetricCounter mPlayers; // Total number of games played per player
        ScenarioMetricCounter mLeftMatchEarly; // Total left match early
        ScenarioMetricCounter mFinishedMatches; // Total matches finishes

        using GameModeMetricCounter = Metrics::TaggedCounter<Metrics::GameMode, GameModeByJoinMethodType>;

        GameModeMetricCounter mGameModeStarts; // Total Number of Starts
        GameModeMetricCounter mGameModeMatchDuration; // Total Match Duration
        GameModeMetricCounter mGameModeLeftEarly; // Total Left Early
        GameModeMetricCounter mGameModeFinishedMatch; // Total Finished Match

#undef STANDARD_TAGS
#undef STANDARD_GAME_TAGS
#undef STANDARD_PLAYER_TAGS

    private:
        void addTaggedSums(ComponentStatus::InfoMap& map, const char8_t* metricBaseName, const Metrics::SumsByTagValue& sums) const
        {
            if (sums.empty())
                return;
            auto& internalMap = map.asMap();
            internalMap.reserve(map.size() + sums.size()); // use eastl::vector_map::reserve() to avoid TdfMap::reserve() calling clear() which is the default implementation
            StringBuilder sb(metricBaseName);
            const auto baseNameLen = sb.length();
            char8_t buf[64];
            for (auto& entry : sums)
            {
                for (auto& tagValue : entry.first)
                {
                    sb.appendN("_", 1);
                    sb.append(tagValue);
                }
                auto valueLen = blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, entry.second);
                auto& added = internalMap.push_back_unsorted(); // NOTE: we don't sort until after all the push_back_unsorted()'s have been done
                added.first.set(sb.c_str(), sb.length());
                added.second.set(buf, valueLen);
                sb.trim(sb.length() - baseNameLen); // reset builder to metricBaseName
            }
            // do a single batch sort, important for performance!
            map.fixupElementOrder();
        }

        template<typename TaggedMetric>
        void addAggregates(ComponentStatus::InfoMap& map, const TaggedMetric& metric, const char8_t* metricBaseName, const eastl::vector< eastl::vector<Metrics::TagInfoBase*> >& keysList) const
        {
            Metrics::SumsByTagValue sums;
            for (auto& keys : keysList)
            {
                sums.clear();
                metric.aggregate(keys, sums);
                addTaggedSums(map, metricBaseName, sums);
            }
        }

        void addPerProductTaggedSums(ComponentStatus& status, const char8_t* metricBaseName, const Metrics::SumsByTagValue& sums) const
        {
            auto& productMap = status.getInfoByProductName();
            char8_t buf[64];
            for (auto& entry : sums)
            {
                Blaze::ComponentStatus::InfoMap* infoByProduct = nullptr;
                eastl::string metric = metricBaseName;
                for (auto& tagValue : entry.first)
                {
                    if (infoByProduct == nullptr)
                    {
                        // the first tag MUST BE the product name
                        const char8_t* productName = tagValue;
                        auto prodItr = productMap.find(productName);
                        if (prodItr == productMap.end())
                        {
                            infoByProduct = productMap.allocate_element();
                            productMap[productName] = infoByProduct;
                        }
                        else
                        {
                            infoByProduct = prodItr->second;
                        }
                    }
                    else
                    {
                        metric += "_";
                        metric += tagValue;
                    }
                }
                blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, entry.second);
                (*infoByProduct)[metric.c_str()] = buf;
            }
        }

        template<typename TaggedMetric>
        void addPerProductAggregates(ComponentStatus& status, const TaggedMetric& metric, const char8_t* metricBaseName, const eastl::vector< eastl::vector<Metrics::TagInfoBase*> >& keysList) const
        {
            Metrics::SumsByTagValue sums;
            for (auto& keys : keysList)
            {
                sums.clear();

                // create a new keys list with the product name as the FIRST key
                // (the assumption here is that metrics going thru here have this product name tag)
                eastl::vector<Metrics::TagInfoBase*> keysWithProduct;
                keysWithProduct.push_back(Metrics::Tag::product_name);
                for (auto& key : keys)
                {
                    keysWithProduct.push_back(key);
                }

                metric.aggregate(keysWithProduct, sums);
                addPerProductTaggedSums(status, metricBaseName, sums);
            }
        }

    public:
        void getStatuses(ComponentStatus& outStatus) const
        {
            Blaze::ComponentStatus::InfoMap& map = outStatus.getInfo();
            char8_t buf[64];
            uint64_t val;

#define GM_ADD_METRIC_ALWAYS(name, value) \
    { \
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value); \
        map[name] = buf; \
    }
#define GM_ADD_METRIC(name, value) \
    { \
        val = value; \
        if (val != 0) \
            GM_ADD_METRIC_ALWAYS(name, val); \
    }

            GM_ADD_METRIC_ALWAYS("GMGauge_ACTIVE_GAMES", mActiveGames.get());

            // the aggregate queries should use the following order: (formerly found in GameManagerMetricHashes TDF)
            //   [tag = "a000"] ConnConciergeEligibleEnum ccEligible;
            //   [tag = "a001"] ConnectionJoinType joinType;
            //   [tag = "a002"] uint32_t index;                     // Used for metrics that store an array of values
            //   [tag = "a003"] GameType gameType;
            //   [tag = "a004"] GameNetworkTopology netTopology;
            //   [tag = "a005"] VoipTopology voipTopology;
            //   [tag = "a006"] JoinMethod joinMethod;
            //   [tag = "a007"] PlayerRemovedReason removeReason;
            //   [tag = "a008"] DemanglerConnectionHealthcheck dmConnHealthcheck;
            //   [tag = "a009"] PingSiteAlias pingSite;             // Since the ping sites aren't all known, we have to use a string.
            //   [tag = "a010"] Collections::AttributeValue gameMode; // populated by the well-known game mode attribute
            //   [tag = "a011"] string(MAX_ISP_LENGTH) geoIPData;
            //   [tag = "a012"] ConnSuccessMetricEnum success;
            //   [tag = "a013"] ConnImpactingMetricEnum impacting;
            //   [tag = "a014"] ConnConciergeModeMetricEnum cc;
            //   [tag = "a015"] ConnDemanglerMetricEnum demangler;
            //   [tag = "a016"] GameDestructionReason gameDestructionReason;

            // If new tags are added, the TagGroups at the top must be updated to include them (unless a larger tag group already includes the subset).

            addAggregates(map, mActiveGames, "GMGauge_ACTIVE_GAMES", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::network_topology, Metrics::Tag::platform_list },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMGauge_ACTIVE_GAME_GROUPS", mActiveGameGroups.get());

            addAggregates(map, mActiveGameGroups, "GMGauge_ACTIVE_GAME_GROUPS", {
                { Metrics::Tag::platform_list },
                { Metrics::Tag::game_mode },
                { Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMGauge_ACTIVE_PLAYERS", mActivePlayers.get());

            addAggregates(map, mActivePlayers, "GMGauge_ACTIVE_PLAYERS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            addPerProductAggregates(outStatus, mActivePlayers, "GMGauge_ACTIVE_PLAYERS", {
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::network_topology } });

            GM_ADD_METRIC("GMGauge_ACTIVE_PLAYER_SLOTS", mActivePlayerSlots.get());

            addAggregates(map, mActivePlayerSlots, "GMGauge_ACTIVE_PLAYER_SLOTS", { 
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::network_topology, Metrics::Tag::platform_list },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMGauge_ACTIVE_SPECTATORS", mActiveSpectators.get());

            addPerProductAggregates(outStatus, mActiveSpectators, "GMGauge_ACTIVE_SPECTATORS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMGauge_ACTIVE_SPECTATOR_SLOTS", mActiveSpectatorSlots.get());

            addAggregates(map, mActiveSpectatorSlots, "GMGauge_ACTIVE_SPECTATOR_SLOTS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::network_topology, Metrics::Tag::platform_list },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMGauge_ACTIVE_VIRTUAL_GAMES", mActiveVirtualGames.get());

            addAggregates(map, mActiveVirtualGames, "GMGauge_ACTIVE_VIRTUAL_GAMES", {
                { Metrics::Tag::game_pingsite },
                { Metrics::Tag::platform_list },
                { Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });


            GM_ADD_METRIC("GMGauge_ALLOW_ANY_REPUTATION_GAMES", mAllowAnyReputationGames.get());

            addAggregates(map, mAllowAnyReputationGames, "GMGauge_ALLOW_ANY_REPUTATION_GAMES", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC_ALWAYS("GMGauge_ALL_GAMES", mAllGames.get());

            addAggregates(map, mAllGames, "GMGauge_ALL_GAMES", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_pingsite } });

            GM_ADD_METRIC("GMGauge_ALL_PLAYER_SLOTS", mAllPlayerSlots.get());

            addAggregates(map, mAllPlayerSlots, "GMGauge_ALL_PLAYER_SLOTS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });


            GM_ADD_METRIC("GMGauge_ALL_SPECTATOR_SLOTS", mAllSpectatorSlots.get());

            addAggregates(map, mAllSpectatorSlots, "GMGauge_ALL_SPECTATOR_SLOTS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology,Metrics::Tag::game_pingsite },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });


            GM_ADD_METRIC("GMGauge_ALL_VIRTUAL_GAMES", mAllVirtualGames.get());

            addAggregates(map, mAllVirtualGames, "GMGauge_ALL_VIRTUAL_GAMES", {
                { Metrics::Tag::game_pingsite },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            //GM_ADD_METRIC("GMGauge_IN_GAME_PLAYER_SESSIONS", mInGamePlayerSessions.get());

            addAggregates(map, mInGamePlayerSessions, "GMGauge_IN_GAME_PLAYER_SESSIONS", {
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::pingsite } });

            addPerProductAggregates(outStatus, mInGamePlayerSessions, "GMGauge_IN_GAME_PLAYER_SESSIONS", {
                { Metrics::Tag::game_type } });

            GM_ADD_METRIC("GMGauge_REQUIRE_GOOD_REPUTATION_GAMES", mRequireGoodReputationGames.get());

            addAggregates(map, mRequireGoodReputationGames, "GMGauge_REQUIRE_GOOD_REPUTATION_GAMES", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMGauge_RESERVED_PLAYERS", mReservedPlayers.get());

            addAggregates(map, mReservedPlayers, "GMGauge_RESERVED_PLAYERS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            addPerProductAggregates(outStatus, mReservedPlayers, "GMGauge_RESERVED_PLAYERS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMGauge_RESERVED_SPECTATORS", mReservedSpectators.get());

            addAggregates(map, mReservedSpectators, "GMGauge_RESERVED_SPECTATORS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            addPerProductAggregates(outStatus, mReservedSpectators, "GMGauge_RESERVED_SPECTATORS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            //GM_ADD_METRIC("GMGauge_UNRESPONSIVE_GAMES", mUnresponsiveGames.get());

            addAggregates(map, mUnresponsiveGames, "GMGauge_UNRESPONSIVE_GAMES", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite },
                { Metrics::Tag::network_topology, Metrics::Tag::platform_list },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });


            GM_ADD_METRIC("GMTotal_DEDICATED_SERVER_LOOKUP", mDedicatedServerLookups.getTotal());

            mDemangler.getTotal(); // not surfacing this overall aggregate, but need to explicitly call getTotal() to suppress GC of tagged counters for backward-compatibility of getStatus

            addAggregates(map, mDemangler, "GMTotal_DEMANGLER", {
                { Metrics::Tag::network_topology, Metrics::Tag::demangler_connection_healthcheck, Metrics::Tag::pingsite } });

            GM_ADD_METRIC("GMTotal_EXTERNAL_SESSION_SYNC_CHECKS", mExternalSessionSyncChecks.getTotal());

            addAggregates(map, mExternalSessionSyncChecks, "GMTotal_EXTERNAL_SESSION_SYNC_CHECKS", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_GAME_CREATED", mGamesCreated.getTotal());

            addAggregates(map, mGamesCreated, "GMTotal_GAME_CREATED", {
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMTotal_GAME_DESTROYED_METRIC", mGamesDestroyed.getTotal());

            addAggregates(map, mGamesDestroyed, "GMTotal_GAME_DESTROYED_METRIC", {
                { Metrics::Tag::game_destruction_reason },
                { Metrics::Tag::game_destruction_reason, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMTotal_GAME_FINISHED", mGamesFinished.getTotal());

            addAggregates(map, mGamesFinished, "GMTotal_GAME_FINISHED", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::platform_list },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMTotal_GAMES_CREATED_BY_UNIQUE_KEY", mGamesCreatedByUniqueKey.getTotal());

            addAggregates(map, mGamesCreatedByUniqueKey, "GMTotal_GAMES_CREATED_BY_UNIQUE_KEY", {
                { Metrics::Tag::game_mode },
                { Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMTotal_GAME_SESSION_DURATION_SECONDS", mGameSessionDurationSeconds.getTotal());

            addAggregates(map, mGameSessionDurationSeconds, "GMTotal_GAME_SESSION_DURATION_SECONDS", {
                { Metrics::Tag::player_removed_reason },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::player_removed_reason, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::player_removed_reason, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            mGamesFinishedInTime.getTotal(); // not surfacing this overall aggregate, but need to explicitly call getTotal() to suppress GC of tagged counters for backward-compatibility of getStatus

            addAggregates(map, mGamesFinishedInTime, "GMTotal_GAME_SESSION_FINISHED_IN_TIME", {
                { Metrics::Tag::time_threshold },
                { Metrics::Tag::time_threshold, Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::time_threshold, Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMTotal_GAME_STARTED", mGamesStarted.getTotal());

            addAggregates(map, mGamesStarted, "GMTotal_GAME_STARTED", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::platform_list } });

            GM_ADD_METRIC("GMTotal_PLAYER_JOINED", mPlayersJoined.getTotal());

            addAggregates(map, mPlayersJoined, "GMTotal_PLAYER_JOINED", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type },
                { Metrics::Tag::join_method },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::game_type, Metrics::Tag::join_method },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::join_method } });

            mPlayerRemovedConnLostInTime.getTotal(); // not surfacing this overall aggregate, but need to explicitly call getTotal() to suppress GC of tagged counters for backward-compatibility of getStatus

            addAggregates(map, mPlayerRemovedConnLostInTime, "GMTotal_PLAYER_REMOVED_CONN_LOST_IN_TIME", {
                { Metrics::Tag::time_threshold },
                { Metrics::Tag::time_threshold, Metrics::Tag::game_pingsite },
                { Metrics::Tag::time_threshold, Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_PLAYER_REMOVED_METRIC", mActivePlayersRemoved.getTotal());

            addAggregates(map, mActivePlayersRemoved, "GMTotal_PLAYER_REMOVED_METRIC", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite } });

            // This must be called to ensure the GC is suppressed, for the 'GMTotal_PLAYER_REMOVED_METRIC' aggregates below (which are based off mPlayersRemoved, NOT mActivePlayersRemoved), see getTotal().
            mPlayersRemoved.getTotal();
            addAggregates(map, mPlayersRemoved, "GMTotal_PLAYER_REMOVED_METRIC", {
                { Metrics::Tag::player_removed_reason },
                { Metrics::Tag::game_pingsite, Metrics::Tag::player_removed_reason },
                { Metrics::Tag::network_topology, Metrics::Tag::player_removed_reason },
                { Metrics::Tag::network_topology, Metrics::Tag::game_pingsite, Metrics::Tag::player_removed_reason },
                { Metrics::Tag::join_method, Metrics::Tag::player_removed_reason },
                { Metrics::Tag::game_type, Metrics::Tag::player_removed_reason },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode, Metrics::Tag::player_removed_reason, },
                { Metrics::Tag::game_type, Metrics::Tag::game_pingsite, Metrics::Tag::game_mode, Metrics::Tag::player_removed_reason } });

            GM_ADD_METRIC("GMTotal_PLAYER_RESERVATIONS", mPlayerReservations.getTotal());

            addAggregates(map, mPlayerReservations, "GMTotal_PLAYER_RESERVATIONS", {
                { Metrics::Tag::network_topology },
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_PLAYER_RESERVATION_CLAIMED", mPlayerReservationsClaimed.getTotal());

            addAggregates(map, mPlayerReservationsClaimed, "GMTotal_PLAYER_RESERVATION_CLAIMED", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_PLAYER_RESERVATION_CLAIMED_FAILED", mPlayerReservationsClaimedFailed.getTotal());

            addAggregates(map, mPlayerReservationsClaimedFailed, "GMTotal_PLAYER_RESERVATION_CLAIMED_FAILED", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_PLAYER_RESERVATION_CLAIMED_FAILED_GAME_ENTRY_CRITERIA", mPlayerReservationsClaimedFailedGameEntryCriteria.getTotal());

            addAggregates(map, mPlayerReservationsClaimedFailedGameEntryCriteria, "GMTotal_PLAYER_RESERVATION_CLAIMED_FAILED_GAME_ENTRY_CRITERIA", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_EXTERNAL_PLAYER_RESERVATIONS", mExternalPlayerReservations.getTotal());

            addAggregates(map, mExternalPlayerReservations, "GMTotal_EXTERNAL_PLAYER_RESERVATIONS", {
                { Metrics::Tag::game_type },
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_EXTERNAL_PLAYER_RESERVATIONS_CLAIMED_FAILED", mExternalPlayerReservationsClaimedFailed.getTotal());

            addAggregates(map, mExternalPlayerReservationsClaimedFailed, "GMTotal_EXTERNAL_PLAYER_RESERVATIONS_CLAIMED_FAILED", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_EXTERNAL_PLAYER_RESERVATIONS_CLAIMED_FAILED_GAME_ENTRY_CRITERIA", mExternalPlayerReservationsClaimedFailedGameEntryCriteria.getTotal());

            addAggregates(map, mExternalPlayerReservationsClaimedFailedGameEntryCriteria, "GMTotal_EXTERNAL_PLAYER_RESERVATIONS_CLAIMED_FAILED_GAME_ENTRY_CRITERIA", {
                { Metrics::Tag::game_type, Metrics::Tag::game_mode } });

            GM_ADD_METRIC("GMTotal_EXTERNAL_PLAYER_CHANGED_TO_LOGGED_IN_PLAYER", mExternalPlayerChangedToLoggedInPlayer.getTotal());

            GM_ADD_METRIC("GMTotal_EXTERNAL_PLAYER_REMOVED_RESERVATION_TIMEOUT", mExternalPlayerRemovedReservationTimeout.getTotal());

#undef GM_ADD_METRIC
#undef GM_ADD_METRIC_ALWAYS
        }

        // Telemetry report metrics
        static const uint8_t PACKET_LOSS_ARRAY_SIZE = 101; // Covers 0%-100% range of packet loss
        struct PacketLossArray : public eastl::array<uint8_t, PACKET_LOSS_ARRAY_SIZE>
        {
        public:
            PacketLossArray()
            {
                memset(this->mValue, 0, PACKET_LOSS_ARRAY_SIZE * sizeof(uint8_t));
            }
        };

        typedef eastl::pair<ConnectionGroupId, ConnectionGroupId> ConnectionGroupIdPair;
        struct PacketLossValues
        {
            PacketLossValues() : mCurrentPacketLoss(0), mCurrentPacketLossCeil(0), mPrevPacketsRecv(0), mPrevPacketsSent(0)
            {}

            float mCurrentPacketLoss;
            uint8_t mCurrentPacketLossCeil;
            PacketLossArray mTotalSamplePacketLosses;
            uint32_t mPrevPacketsRecv;
            uint32_t mPrevPacketsSent;
        };
        typedef eastl::map<ConnectionGroupIdPair, PacketLossValues> PacketLossByConnGroupMap;
        typedef eastl::hash_map<GameId, PacketLossByConnGroupMap> PacketLossByConnGroupByGameIdMap;
        PacketLossByConnGroupByGameIdMap mCurrentPacketLossByConnGroup;

        // May be better to just add into PacketLossValues, and rename that struct. 
        typedef eastl::list<RiverPoster::ConnectionStatsEventPtr> ConnStatsPINEventList;
        typedef eastl::map<ConnectionGroupIdPair, ConnStatsPINEventList> ConnectionStatsEventListByConnGroupMap;
        typedef eastl::hash_map<GameId, ConnectionStatsEventListByConnGroupMap> ConnectionStatsEventListByConnGroupByGameIdMap;
        ConnectionStatsEventListByConnGroupByGameIdMap mConnectionStatsEventListByConnGroup;


        struct LatencyArray : public eastl::array<uint16_t, MAX_NUM_LATENCY_BUCKETS>
        {
        public:
            LatencyArray()
            {
                memset(mValue, 0, MAX_NUM_LATENCY_BUCKETS * sizeof(uint16_t));
            }
        };

        struct ConnectionGroupLatencyValues
        {
            ConnectionGroupLatencyValues() : mTotalLatency(0), mLatencyCount(0), mMaxLatency(0), mMinLatency(UINT32_MAX)
            {}

            uint64_t mTotalLatency;
            LatencyArray mTotalSampleLatencies;
            uint64_t mLatencyCount; // used to calculate the current average
            uint32_t mMaxLatency;
            uint32_t mMinLatency;
        };

        typedef eastl::map<ConnectionGroupIdPair, ConnectionGroupLatencyValues> LatencyByConnGroupMap;
        typedef eastl::hash_map<GameId, LatencyByConnGroupMap> LatencyByConnGroupByGameIdMap;
        LatencyByConnGroupByGameIdMap mLatencyByConnGroups;
    };

} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_GAMEMANAGER_METRICS_H
