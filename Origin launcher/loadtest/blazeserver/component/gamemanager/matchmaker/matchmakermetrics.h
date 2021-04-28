/*! ************************************************************************************************/
/*!
    \file matchmakermetrics.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEMANAGER_MATCHMAKER_MATCHMAKER_METRICS_H
#define BLAZE_GAMEMANAGER_MATCHMAKER_MATCHMAKER_METRICS_H

#include "framework/metrics/metrics.h"
#include "gamemanager/gamemanagermetrics.h" // for Metrics tags

namespace Blaze
{
namespace Metrics
{
    using RuleCategory = const char8_t*; // note that rule category can include rule criteria
    using RuleCategoryValue = const char8_t*;

    namespace Tag
    {
        extern TagInfo<GameManager::QosValidationMetricResult>* qos_validation_metric_result;

        extern TagInfo<GameManager::RuleName>* rule_name;
        extern TagInfo<RuleCategory>* rule_category;
        extern TagInfo<RuleCategoryValue>* rule_category_value;
        extern TagInfo<GameManager::MatchmakingResult>* match_result;
    }
}

namespace GameManager
{
namespace Matchmaker
{
    /*! ************************************************************************************************/
    /*! \brief struct holding the healthcheck metrics captured for the matchmaker

        see http://online.ea.com/confluence/display/tech/Blaze+Health+Check+Definitions for details
    ***************************************************************************************************/
    struct MatchmakerMasterMetrics
    {
        MatchmakerMasterMetrics(const char8_t* metricNamePrefix, Metrics::MetricsCollection& collection)
            : mMetricsCollection(collection)
            , mPrefix(metricNamePrefix)
            , mGaugeMatchmakingUsersInSessions(mMetricsCollection, (mPrefix + "matchmakingUsersInSessions").c_str())
            , mCreateGameMatchmakingSessions(mMetricsCollection, (mPrefix + "createGameSessions").c_str())
            , mFindGameMatchmakingSessions(mMetricsCollection, (mPrefix + "findGameSessions").c_str())
            , mPackerMatchmakingSessions(mMetricsCollection, (mPrefix + "packerSessions").c_str())
            , mTotalMatchmakingScenarioSubsessionStarted(mMetricsCollection, (mPrefix + "scenarioSubsessionStarted").c_str())
            , mTotalMatchmakingScenarioSubsessionFinished(mMetricsCollection, (mPrefix + "scenarioSubsessionFinished").c_str())
            , mTotalMatchmakingSessionStarted(mMetricsCollection, (mPrefix + "sessionStarted").c_str())
            , mTotalMatchmakingSessionCanceled(mMetricsCollection, (mPrefix + "sessionCanceled").c_str())
            , mTotalMatchmakingSessionTimeout(mMetricsCollection, (mPrefix + "sessionTimeout").c_str())
            , mTotalMatchmakingSessionSuccess(mMetricsCollection, (mPrefix + "sessionSuccess").c_str())
            , mTotalMatchmakingSessionTerminated(mMetricsCollection, (mPrefix + "sessionTerminated").c_str())
            , mTotalMatchmakingSessionTerminatedReconfig(mMetricsCollection, (mPrefix + "sessionTerminatedReconfig").c_str())
            , mTotalMatchmakingSessionTerminatedExternalSession(mMetricsCollection, (mPrefix + "sessionTerminatedExternalSession").c_str())
            , mTotalMatchmakingSessionStartFailed(mMetricsCollection, (mPrefix + "sessionStartFailed").c_str())
            , mTotalMatchmakingSessionsEndedFailedQosValidation(mMetricsCollection, (mPrefix + "sessionsEndedFailedQosValidation").c_str())
            , mTotalMatchmakingSessionsRejectPoorReputation(mMetricsCollection, (mPrefix + "sessionsRejectPoorReputation").c_str())
            , mTotalMatchmakingSessionsAcceptAnyReputation(mMetricsCollection, (mPrefix + "sessionsAcceptAnyReputation").c_str())
            , mTotalMatchmakingSessionsMustAllowAnyReputation(mMetricsCollection, (mPrefix + "sessionsMustAllowAnyReputation").c_str())
            , mTotalMatchmakingCreateGameFinalizeAttempts(mMetricsCollection, (mPrefix + "createGameFinalizeAttempts").c_str())
            , mTotalMatchmakingSessionsCreatedNewGames(mMetricsCollection, (mPrefix + "sessionsCreatedNewGames").c_str())
            , mTotalMatchmakingSessionsJoinedNewGames(mMetricsCollection, (mPrefix + "sessionsJoinedNewGames").c_str())
            , mTotalMatchmakingFindGameFinalizeNotifsAccepted(mMetricsCollection, (mPrefix + "findGameFinalizeNotifsAccepted").c_str())
            , mTotalMatchmakingFindGameFinalizeNotifsOverwritten(mMetricsCollection, (mPrefix + "findGameFinalizeNotifsOverwritten").c_str())
            , mTotalMatchmakingFindGameFinalizeBatches(mMetricsCollection, (mPrefix + "findGameFinalizeBatches").c_str())
            , mTotalMatchmakingFindGameFinalizeMatches(mMetricsCollection, (mPrefix + "findGameFinalizeMatches").c_str())
            , mTotalMatchmakingFindGameFinalizeAttempts(mMetricsCollection, (mPrefix + "findGameFinalizeAttempts").c_str())
            , mTotalMatchmakingFindGameFinalizeSkippedAttempts(mMetricsCollection, (mPrefix + "findGameFinalizeSkippedAttempts").c_str())
            , mTotalMatchmakingFindGameFinalizeFailedToJoinFoundGame(mMetricsCollection, (mPrefix + "findGameFinalizeFailedToJoinFoundGame").c_str())
            , mTotalMatchmakingFindGameFinalizeFailedToJoinAnyGame(mMetricsCollection, (mPrefix + "findGameFinalizeFailedToJoinAnyGame").c_str())
            , mTotalMatchmakingSessionsJoinedExistingGames(mMetricsCollection, (mPrefix + "sessionsJoinedExistingGames").c_str())
            , mTotalMatchmakingCreateGameFinalizationsFailedToResetDedicatedServer(mMetricsCollection, (mPrefix + "createGameFinalizationsFailedToResetDedicatedServer").c_str())
            , mTotalMatchmakingSessionsDisabledCreateGameMode(mMetricsCollection, (mPrefix + "sessionsDisabledCreateGameMode").c_str())
            , mTotalMatchmakingCreateGameEvaluationsByNewSessions(mMetricsCollection, (mPrefix + "createGameEvaluationsByNewSessions").c_str())
            , mTotalMatchmakingSuccessDurationMs(mMetricsCollection, (mPrefix + "successDurationMs").c_str())
            , mTotalMatchmakingCancelDurationMs(mMetricsCollection, (mPrefix + "cancelDurationMs").c_str())
            , mTotalMatchmakingTerminatedDurationMs(mMetricsCollection, (mPrefix + "terminatedDurationMs").c_str())
            , mTotalMatchmakingEndedFailedQosValidationDurationMs(mMetricsCollection, (mPrefix + "endedFailedQosValidationDurationMs").c_str())
            , mTotalMatchmakingIdles(mMetricsCollection, (mPrefix + "idles").c_str())
            , mLastIdleLength(mMetricsCollection, (mPrefix + "lastIdleLength").c_str())
            , mLastIdleFGLength(mMetricsCollection, (mPrefix + "lastIdleFGLength").c_str())
            , mLastIdleDirtySessions(mMetricsCollection, (mPrefix + "lastIdleDirtySessions").c_str())
            , mQosValidationMetrics(metricNamePrefix, mMetricsCollection)
            , mTeamsBasedMetrics(metricNamePrefix, mMetricsCollection)
        {
        }

    private:
        Metrics::MetricsCollection& mMetricsCollection;
        eastl::string mPrefix;

    public:
        Metrics::Gauge mGaugeMatchmakingUsersInSessions;
        Metrics::Gauge mCreateGameMatchmakingSessions;
        Metrics::Gauge mFindGameMatchmakingSessions;
        Metrics::Gauge mPackerMatchmakingSessions;
        Metrics::Counter mTotalMatchmakingScenarioSubsessionStarted;
        Metrics::Counter mTotalMatchmakingScenarioSubsessionFinished;
        Metrics::Counter mTotalMatchmakingSessionStarted;
        Metrics::Counter mTotalMatchmakingSessionCanceled;
        Metrics::Counter mTotalMatchmakingSessionTimeout;
        Metrics::Counter mTotalMatchmakingSessionSuccess;
        Metrics::Counter mTotalMatchmakingSessionTerminated;
        Metrics::Counter mTotalMatchmakingSessionTerminatedReconfig;
        Metrics::Counter mTotalMatchmakingSessionTerminatedExternalSession;
        Metrics::Counter mTotalMatchmakingSessionStartFailed; // unable to start acquired session
        Metrics::Counter mTotalMatchmakingSessionsEndedFailedQosValidation;
        Metrics::Counter mTotalMatchmakingSessionsRejectPoorReputation;
        Metrics::Counter mTotalMatchmakingSessionsAcceptAnyReputation;
        Metrics::Counter mTotalMatchmakingSessionsMustAllowAnyReputation;
        Metrics::Counter mTotalMatchmakingCreateGameFinalizeAttempts;
        Metrics::Counter mTotalMatchmakingSessionsCreatedNewGames;
        Metrics::Counter mTotalMatchmakingSessionsJoinedNewGames;
        Metrics::Counter mTotalMatchmakingFindGameFinalizeNotifsAccepted;
        Metrics::Counter mTotalMatchmakingFindGameFinalizeNotifsOverwritten;
        Metrics::Counter mTotalMatchmakingFindGameFinalizeBatches;
        Metrics::Counter mTotalMatchmakingFindGameFinalizeMatches;
        Metrics::Counter mTotalMatchmakingFindGameFinalizeAttempts;
        Metrics::Counter mTotalMatchmakingFindGameFinalizeSkippedAttempts;
        Metrics::Counter mTotalMatchmakingFindGameFinalizeFailedToJoinFoundGame;
        Metrics::Counter mTotalMatchmakingFindGameFinalizeFailedToJoinAnyGame;
        Metrics::Counter mTotalMatchmakingSessionsJoinedExistingGames;
        Metrics::Counter mTotalMatchmakingCreateGameFinalizationsFailedToResetDedicatedServer;
        Metrics::Counter mTotalMatchmakingSessionsDisabledCreateGameMode;
        Metrics::Counter mTotalMatchmakingCreateGameEvaluationsByNewSessions;
        Metrics::Counter mTotalMatchmakingSuccessDurationMs;
        Metrics::Counter mTotalMatchmakingCancelDurationMs;
        Metrics::Counter mTotalMatchmakingTerminatedDurationMs;
        Metrics::Counter mTotalMatchmakingEndedFailedQosValidationDurationMs;
        Metrics::Counter mTotalMatchmakingIdles;
        Metrics::Gauge mLastIdleLength; // in milliseconds
        Metrics::Gauge mLastIdleFGLength; // in milliseconds
        Metrics::Gauge mLastIdleDirtySessions;

        // Health checks for Qos validation
        typedef struct QosValidationMetrics
        {
            QosValidationMetrics(const char8_t* metricNamePrefix, Metrics::MetricsCollection& collection)
                : mMetricsCollection(collection)
                , mPrefix(metricNamePrefix)
                , mTotalUsersInQosRequests(mMetricsCollection, (mPrefix + "usersInQosRequests").c_str(), Metrics::Tag::network_topology, Metrics::Tag::qos_validation_metric_result, Metrics::Tag::cc_mode)
                , mTotalUsers(mMetricsCollection, (mPrefix + "users").c_str(), Metrics::Tag::network_topology, Metrics::Tag::qos_validation_metric_result, Metrics::Tag::cc_mode)
                , mTotalMatchmakingSessions(mMetricsCollection, (mPrefix + "matchmakingSessions").c_str(), Metrics::Tag::network_topology, Metrics::Tag::qos_validation_metric_result, Metrics::Tag::index) // using 'index' tag for 'tier'
            {
            }

        private:
            Metrics::MetricsCollection& mMetricsCollection;
            eastl::string mPrefix;

        public:
            // per-user stats

            // total users in requests subject to the connectivity/latency/packet-loss checks
            // Note: we only care if CC is used or not, but ConnConciergeModeMetricEnum has more than one type of 'CC is used', so we'll arbitrarily use just one of those
            Metrics::TaggedCounter<GameNetworkTopology, QosValidationMetricResult, ConnConciergeModeMetricEnum> mTotalUsersInQosRequests;
            Metrics::TaggedCounter<GameNetworkTopology, QosValidationMetricResult, ConnConciergeModeMetricEnum> mTotalUsers;

            // per matchmaking session stats

            Metrics::TaggedCounter<GameNetworkTopology, QosValidationMetricResult, uint32_t> mTotalMatchmakingSessions;

        } QosValidationMetrics;

        QosValidationMetrics mQosValidationMetrics;

        // Health checks for teams based rules/finalization
        typedef struct TeamsBasedMatchmakingMetrics
        {
            TeamsBasedMatchmakingMetrics(const char8_t* metricNamePrefix, Metrics::MetricsCollection& collection)
                : mMetricsCollection(collection)
                , mPrefix(metricNamePrefix)
                , mTotalMatchmakingSessionTimeoutSessionSize(mMetricsCollection, (mPrefix + "matchmakingSessionTimeoutSessionSize").c_str())
                , mTotalMatchmakingSessionTimeoutSessionTeamUed(mMetricsCollection, (mPrefix + "matchmakingSessionTimeoutSessionTeamUed").c_str())
                , mTotalTeamFinalizeAttempt(mMetricsCollection, (mPrefix + "teamFinalizeAttempt").c_str())
                , mTotalTeamFinalizeSuccess(mMetricsCollection, (mPrefix + "teamFinalizeSuccess").c_str())
                , mTotalTeamFinalizeSuccessTeamCount(mMetricsCollection, (mPrefix + "teamFinalizeSuccessTeamCount").c_str())
                , mTotalTeamFinalizeSuccessPlayerCount(mMetricsCollection, (mPrefix + "teamFinalizeSuccessPlayerCount").c_str())
                , mTotalTeamFinalizeSuccessTeamUedDiff(mMetricsCollection, (mPrefix + "teamFinalizeSuccessTeamUedDiff").c_str())
                , mTotalTeamFinalizeSuccessGameMemberUedDelta(mMetricsCollection, (mPrefix + "teamFinalizeSuccessGameMemberUedDelta").c_str())
                , mTotalTeamFinalizeSuccessTeamMemberUedDelta(mMetricsCollection, (mPrefix + "teamFinalizeSuccessTeamMemberUedDelta").c_str())
                , mTotalTeamFinalizeSuccessRePicksLeft(mMetricsCollection, (mPrefix + "teamFinalizeSuccessRePicksLeft").c_str())
                , mTotalTeamFinalizeSuccessCompnsLeft(mMetricsCollection, (mPrefix + "teamFinalizeSuccessCompnsLeft").c_str())
                , mTotalTeamFinalizeCompnsTry(mMetricsCollection, (mPrefix + "teamFinalizeCompnsTry").c_str())
                , mTotalTeamFinalizePickSequenceTry(mMetricsCollection, (mPrefix + "teamFinalizePickSequenceTry").c_str())
                , mTotalTeamFinalizePickSequenceFail(mMetricsCollection, (mPrefix + "teamFinalizePickSequenceFail").c_str())
                , mTotalTeamFinalizePickSequenceFailTeamSizeDiff(mMetricsCollection, (mPrefix + "teamFinalizePickSequenceFailTeamSizeDiff").c_str())
                , mTotalTeamFinalizePickSequenceFailTeamUedDiff(mMetricsCollection, (mPrefix + "teamFinalizePickSequenceFailTeamUedDiff").c_str())
                , mTotalTeamFinalizeSuccessByGroupOfX(mMetricsCollection, (mPrefix + "teamFinalizeSuccessByGroupOfX").c_str(), Metrics::Tag::index) // using 'index' tag for 'group size'
                , mTotalTeamFinalizeSuccessByGroupOfXCompnSame(mMetricsCollection, (mPrefix + "teamFinalizeSuccessByGroupOfXCompnSame").c_str(), Metrics::Tag::index) // using 'index' tag for 'group size'
                , mTotalTeamFinalizeSuccessByGroupOfXCompnOver50FitPct(mMetricsCollection, (mPrefix + "teamFinalizeSuccessByGroupOfXCompnOver50FitPct").c_str(), Metrics::Tag::index) // using 'index' tag for 'group size'
            {
            }

        private:
            Metrics::MetricsCollection& mMetricsCollection;
            eastl::string mPrefix;

        public:
            Metrics::Counter mTotalMatchmakingSessionTimeoutSessionSize;
            Metrics::Counter mTotalMatchmakingSessionTimeoutSessionTeamUed;
            Metrics::Counter mTotalTeamFinalizeAttempt; //total MM session finalization attempts by below team criteria.
            Metrics::Counter mTotalTeamFinalizeSuccess; //number of the above attempts which succeeded (remainder of attempts failed).
            Metrics::Counter mTotalTeamFinalizeSuccessTeamCount; //total teams in the successful finalizations by teams criteria.
            Metrics::Counter mTotalTeamFinalizeSuccessPlayerCount; //total players in the successful finalization attempts by teams criteria.
            Metrics::Counter mTotalTeamFinalizeSuccessTeamUedDiff; //total of differences in team's (TeamUEDBalanceRule) ueds in successful finalizations.
            Metrics::Counter mTotalTeamFinalizeSuccessGameMemberUedDelta; //total of differences between top and bottom game member's (TeamUEDBalanceRule) ueds, in successful finalizations.
            Metrics::Counter mTotalTeamFinalizeSuccessTeamMemberUedDelta; //total of differences between top and bottom team member's (TeamUEDBalanceRule) ueds, in successful finalizations.
            Metrics::Counter mTotalTeamFinalizeSuccessRePicksLeft; //total remaining pick sequence retries in the successful finalization attempts by team criteria.
            Metrics::Counter mTotalTeamFinalizeSuccessCompnsLeft; //total remaining composition retries in the successful finalization attempts by team criteria.
            Metrics::Counter mTotalTeamFinalizeCompnsTry; //total team compositions tried over all finalization attempts.
            Metrics::Counter mTotalTeamFinalizePickSequenceTry; //total MM session pick sequences tried over all finalization attempts.
            Metrics::Counter mTotalTeamFinalizePickSequenceFail; //total MM session pick sequences failed over all finalization attempts.
            Metrics::Counter mTotalTeamFinalizePickSequenceFailTeamSizeDiff; //total of differences between team's sizes when pick sequences failed.
            Metrics::Counter mTotalTeamFinalizePickSequenceFailTeamUedDiff; //total of differences between team's (TeamUEDBalanceRule) ueds when pick sequences failed.

            // metrics broken down by group sizes. Note: Use these metrics to approximate stats but note not all account for non-pulling sessions (for efficiency).
            Metrics::TaggedCounter<uint32_t> mTotalTeamFinalizeSuccessByGroupOfX; //total successful finalizations by pulling session of this size.
            Metrics::TaggedCounter<uint32_t> mTotalTeamFinalizeSuccessByGroupOfXCompnSame; //total cases team compositions were identical, in successful finalizations.
            Metrics::TaggedCounter<uint32_t> mTotalTeamFinalizeSuccessByGroupOfXCompnOver50FitPct; //total with fit percent of the GameTeamComposition finalized > 50%. Helps get idea of typical team compositions (see rule fit table).

        } TeamsBasedMatchmakingMetrics;

        TeamsBasedMatchmakingMetrics mTeamsBasedMetrics;

    };

    /// @deprecated [metrics-refactor] MatchmakingMetrics & ScenarioMatchmakingMetricsMap TDFs can be removed when getMatchmakingMetrics RPC & GetMatchmakingMetricsResponse TDF are removed
    /// @deprecated [metrics-refactor] MatchmakingMetrics TDF is deprecated; and this MatchmakingMetricsNew struct should be renamed to MatchmakingMetrics after the TDF has been removed
    struct MatchmakingMetricsNew
    {
        MatchmakingMetricsNew(Metrics::MetricsCollection& collection)
            : mMetricsCollection(collection)
            , mMetrics(mMetricsCollection)
            , mSubsessionMetrics(mMetricsCollection)
            , mDiagnostics(mMetricsCollection)
        {
        }

    private:
        Metrics::MetricsCollection& mMetricsCollection;

    public:

        using ScenarioMetricCounter = Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion>;
        using ScenarioMetricGauge = Metrics::TaggedGauge<ScenarioName, ScenarioVariantName, ScenarioVersion>;

        // these ScenarioMetrics are separate from SubsessionMetrics because they are not 'necessarily' aggregates of the similar SubsessionMetrics counterparts
        // (non-scenario metrics use a special tag in ScenarioMetrics)
        typedef struct ScenarioMetrics
        {
            ScenarioMetrics(Metrics::MetricsCollection& collection)
                : mMetricsCollection(collection)
                , mTotalRequests(mMetricsCollection, "sn_requests", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::index) // using 'index' tag for 'group size'
                , mTotalResults(mMetricsCollection, "sn_results", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::match_result)
                , mTotalPlayersInGroupRequests(mMetricsCollection, "sn_playersInGroupRequests", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version)
                , mTotalSuccessDurationMs(mMetricsCollection, "sn_successDurationMs", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version)
                , mTotalPlayerCountInGame(mMetricsCollection, "sn_playerCountInGame", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version)
                , mTotalMaxPlayerCapacity(mMetricsCollection, "sn_maxPlayerCapacity", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version)
                , mTotalGamePercentFullCapacity(mMetricsCollection, "sn_gamePercentFullCapacity", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version)
                , mTotalFitScore(mMetricsCollection, "sn_fitScore", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version)
                , mExternalSessionFailuresImpacting(mMetricsCollection, "sn_externalSessionFailuresImpacting", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::platform)
                , mExternalSessionFailuresNonImpacting(mMetricsCollection, "sn_externalSessionFailuresNonImpacting", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::platform)
            {
            }

        private:
            Metrics::MetricsCollection& mMetricsCollection;

        public:
            Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, uint32_t> mTotalRequests; // Total number of MM requests by group size, including non-grouped single users (size=1)
            Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, MatchmakingResult> mTotalResults; // Total number of MM results
            ScenarioMetricCounter mTotalPlayersInGroupRequests; // Total number of players that matchmade via group requests
            ScenarioMetricCounter mTotalSuccessDurationMs; // Aggregate of the time of all the successful requests, in milliseconds
            ScenarioMetricCounter mTotalPlayerCountInGame; // Aggregate of the player count in game of all the successful requests
            ScenarioMetricCounter mTotalMaxPlayerCapacity; // Aggregate of the game capacity of all the successful requests
            ScenarioMetricCounter mTotalGamePercentFullCapacity; // Aggregate of game percentage full of all the successful requests
            ScenarioMetricCounter mTotalFitScore; // Aggregate Fit Score of all successful requests

            // The number of calls to ExternalSession APIs that failed and are considered 'player-impacting'. The external session errors for matchmaking found/created game fall in this category.
            Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, ClientPlatformType> mExternalSessionFailuresImpacting; 
            // The number of calls to ExternalSession APIs that failed and are considered 'non-player-impacting'. The external session errors for matchmaking session fall in this category.
            Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, ClientPlatformType> mExternalSessionFailuresNonImpacting; 

        } ScenarioMetrics;

        ScenarioMetrics mMetrics;

        using SubsessionMetricCounter = Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, SubSessionName>;
        using SubsessionMetricGauge = Metrics::TaggedGauge<ScenarioName, ScenarioVariantName, ScenarioVersion, SubSessionName>;

        // SubsessionMetrics that do not have ScenarioMetrics counterparts 'can be' aggregated to obtain the scenario metric values (but this may not always be the case)
        typedef struct SubsessionMetrics
        {
            SubsessionMetrics(Metrics::MetricsCollection& collection)
                : mMetricsCollection(collection)
                , mTotalRequests(mMetricsCollection, "sb_requests", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name, Metrics::Tag::index) // using 'index' tag for 'group size'
                , mTotalResults(mMetricsCollection, "sb_results", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name, Metrics::Tag::match_result)
                , mTotalPlayersInGroupRequests(mMetricsCollection, "sb_playersInGroupRequests", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mTotalSuccessDurationMs(mMetricsCollection, "sb_successDurationMs", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mTotalPlayerCountInGame(mMetricsCollection, "sb_playerCountInGame", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mTotalMaxPlayerCapacity(mMetricsCollection, "sb_maxPlayerCapacity", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mTotalGamePercentFullCapacity(mMetricsCollection, "sb_gamePercentFullCapacity", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mTotalFitScore(mMetricsCollection, "sb_fitScore", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mMaxFitScore(mMetricsCollection, "sb_maxFitScore", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
            {
            }

        private:
            Metrics::MetricsCollection& mMetricsCollection;

        public:
            Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, SubSessionName, uint32_t> mTotalRequests; // Total number of MM requests by group size, including non-grouped single users (size=1)
            Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, SubSessionName, MatchmakingResult> mTotalResults; // Total number of MM results
            SubsessionMetricCounter mTotalPlayersInGroupRequests; // Total number of players that matchmade via group requests
            SubsessionMetricCounter mTotalSuccessDurationMs; // Aggregate of the time of all the successful requests, in milliseconds
            SubsessionMetricCounter mTotalPlayerCountInGame; // Aggregate of the player count in game of all the successful requests
            SubsessionMetricCounter mTotalMaxPlayerCapacity; // Aggregate of the game capacity of all the successful requests
            SubsessionMetricCounter mTotalGamePercentFullCapacity; // Aggregate of game percentage full of all the successful requests
            SubsessionMetricCounter mTotalFitScore; // Aggregate Fit Score of all successful requests
            SubsessionMetricGauge mMaxFitScore; // Max Fit Score

        } SubsessionMetrics;

        SubsessionMetrics mSubsessionMetrics;

        typedef struct SubsessionDiagnostics
        {
            SubsessionDiagnostics(Metrics::MetricsCollection& collection)
                : mMetricsCollection(collection)
                , mSessions(mMetricsCollection, "sessions", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mCreateEvaluations(mMetricsCollection, "createEvaluations", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mCreateEvaluationsMatched(mMetricsCollection, "createEvaluationsMatched", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mFindRequestsGamesAvailable(mMetricsCollection, "findRequestsGamesAvailable", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name)
                , mRuleDiagnostics(mMetricsCollection)
            {
            }

        private:
            Metrics::MetricsCollection& mMetricsCollection;

        public:
            // these mSessions, mCreateEvaluations and mCreateEvaluationsMatched metrics are not aggregates of the similar DiagnosticsByRule versions
            SubsessionMetricCounter mSessions; // Count of Matchmaking sessions had diagnostics tracked for this subsession. Sessions that failed to start etc are not included
            SubsessionMetricCounter mCreateEvaluations; // For create mode matchmaking. Total create mode evaluations using this subsession
            SubsessionMetricCounter mCreateEvaluationsMatched; // For create mode matchmaking. Total create mode evaluations using this subsession, that matched
            SubsessionMetricCounter mFindRequestsGamesAvailable; // For find mode matchmaking.  Total open to matchmaking games present, at the time MM session was started

            typedef struct DiagnosticsByRule
            {
                DiagnosticsByRule(Metrics::MetricsCollection& collection)
                    : mMetricsCollection(collection)
                    , mSessions(mMetricsCollection, "sessionsByRule", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name, Metrics::Tag::rule_name, Metrics::Tag::rule_category, Metrics::Tag::rule_category_value)
                    , mCreateEvaluations(mMetricsCollection, "createEvaluationsByRule", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name, Metrics::Tag::rule_name, Metrics::Tag::rule_category, Metrics::Tag::rule_category_value)
                    , mCreateEvaluationsMatched(mMetricsCollection, "createEvaluationsMatchedByRule", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name, Metrics::Tag::rule_name, Metrics::Tag::rule_category, Metrics::Tag::rule_category_value)
                    , mFindRequests(mMetricsCollection, "findRequestsByRule", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name, Metrics::Tag::rule_name, Metrics::Tag::rule_category, Metrics::Tag::rule_category_value)
                    , mFindRequestsHadGames(mMetricsCollection, "findRequestsHadGamesByRule", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name, Metrics::Tag::rule_name, Metrics::Tag::rule_category, Metrics::Tag::rule_category_value)
                    , mFindRequestsGamesVisible(mMetricsCollection, "findRequestsGamesVisibleByRule", Metrics::Tag::scenario_name, Metrics::Tag::scenario_variant_name, Metrics::Tag::scenario_version, Metrics::Tag::subsession_name, Metrics::Tag::rule_name, Metrics::Tag::rule_category, Metrics::Tag::rule_category_value)
                {
                }

            private:
                Metrics::MetricsCollection& mMetricsCollection;

            public:
                using DiagnosticRuleMetricCounter = Metrics::TaggedCounter<ScenarioName, ScenarioVariantName, ScenarioVersion, SubSessionName, RuleName, Metrics::RuleCategory, Metrics::RuleCategoryValue>;

                DiagnosticRuleMetricCounter mSessions; // Count of requests/MMsessions that had this rule/criteria
                DiagnosticRuleMetricCounter mCreateEvaluations; // For create mode matchmaking. Count of MMsessions that were evaluated for this rule/criteria
                DiagnosticRuleMetricCounter mCreateEvaluationsMatched; // For create mode matchmaking. Count of MMsessions that were evaluated for this rule/criteria, that matched
                DiagnosticRuleMetricCounter mFindRequests; // For find mode matchmaking. Count of requests that tried to find games using the rule/criteria
                DiagnosticRuleMetricCounter mFindRequestsHadGames; // For find mode matchmaking. Count of the requests that could match at least one game, using the rule/criteria, at the time MM session was started
                DiagnosticRuleMetricCounter mFindRequestsGamesVisible; // For find mode matchmaking. Count of games that were matchable by the rule/criteria, for the request, at the time MM session was started

            } DiagnosticsByRule;

            DiagnosticsByRule mRuleDiagnostics; // Map of diagnostics broken down by rule

        } SubsessionDiagnostics;

        SubsessionDiagnostics mDiagnostics; // Aggregate Matchmaking subsession diagnostics

    };

} // namespace Matchmaker
} // namespace GameManager
} // namespace Blaze

#endif // BLAZE_GAMEMANAGER_MATCHMAKER_MATCHMAKER_METRICS_H
