/*! ************************************************************************************************/
/*!
    \file gamepackerslaveimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/
#ifndef BLAZE_GAMEPACKER_SLAVE_IMPL_H
#define BLAZE_GAMEPACKER_SLAVE_IMPL_H

#include <gamepacker/handlers.h>
#include <gamepacker/packer.h>
#include "EASTL/hash_set.h"
#include "framework/controller/drainlistener.h"
#include "framework/system/fiberjobqueue.h"
#include "gamemanager/rete/reteruleprovider.h"
#include "gamemanager/rete/productionlistener.h"
#include "gamemanager/rpc/gamepackermaster.h"
#include "gamemanager/rpc/gamepackerslave_stub.h"
#include "gamemanager/tdf/gamepacker_server.h"
#include "gamemanager/tdf/matchmaker_server.h"
#include "gamemanager/tdf/gamepacker_config_server.h"
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/searchcomponent/searchshardingbroker.h"
#include "gamemanager/matchmakingfiltersutil.h"

namespace Blaze
{

namespace GameManager {
namespace Matchmaker {
class RuleDefinitionCollection;
class MatchmakingConfig;
class GameTemplateSilo;
class PropertyRuleDefinition;
}
namespace Rete {
class ReteNetwork;
}
class MatchmakingScenarioManager;
class MatchmakingCreateGameRequest;
class StartMatchmakingInternalRequest;
class CreateGameTemplate;
}

namespace GamePacker
{

typedef const char8_t* PackerGameTemplateName;
typedef const char8_t* RemovalReason;

// From gamepacker_server.tdf
using GameManager::PackerSessionId;
using GameManager::PackerSiloId;
using GameManager::PackerFinalizationJobId;

using GameManager::PackerPlayerId;
using GameManager::PackerPartyId;
using GameManager::PackerGameId;


class GamePackerConfig;

class GamePackerSlaveImpl : public GamePackerSlaveStub
                          , private GamePackerMasterListener
                          , private DrainStatusCheckHandler
                          //, private PingSiteUpdateSubscriber
                          , private Search::SearchShardingBroker
                          , public Packer::PackGameHandler
                          , public Packer::ReapGameHandler
                          , public GameManager::Rete::ProductionListener
                          , Packer::PackerUtilityMethods
{
public:
    GamePackerSlaveImpl();
    ~GamePackerSlaveImpl() override;

protected:
    struct PackerSession;
    struct GameTemplate;

    bool onConfigure() override;
    bool onPrepareForReconfigure(const GamePackerConfig& newConfig) override;
    bool onReconfigure() override;
    bool onResolve() override;
    bool onCompleteActivation() override;
    void onShutdown() override;
    bool isDrainComplete() override;
    bool onValidateConfig(GamePackerConfig& config, const GamePackerConfig* referenceConfigPtr, ::Blaze::ConfigureValidationErrors& validationErrors) const override;
    // GamePackerMasterListener
    void onNotifyWorkerPackerSessionAvailable(const Blaze::GameManager::NotifyWorkerPackerSessionAvailable& data, ::Blaze::UserSession *associatedUserSession) override;
    void onNotifyWorkerPackerSessionRelinquish(const Blaze::GameManager::NotifyWorkerPackerSessionRelinquish& data, ::Blaze::UserSession *associatedUserSession) override;
    void onNotifyWorkerPackerSessionTerminated(const Blaze::GameManager::NotifyWorkerPackerSessionTerminated& data, ::Blaze::UserSession *associatedUserSession) override;
    //{{
    // These notifications are never sent to the packer slaves, therefore these handlers should never be called.
    void onNotifyMatchmakingFailed(const Blaze::GameManager::NotifyMatchmakingFinished& data, ::Blaze::UserSession *associatedUserSession) override {}
    void onNotifyMatchmakingAsyncStatus(const Blaze::GameManager::NotifyMatchmakingAsyncStatus& data, ::Blaze::UserSession *associatedUserSession) override {}
    void onNotifyMatchmakingFinished(const Blaze::GameManager::NotifyMatchmakingFinished& data, ::Blaze::UserSession *associatedUserSession) override {}
    void onNotifyMatchmakingSessionConnectionValidated(const Blaze::GameManager::NotifyMatchmakingSessionConnectionValidated& data, ::Blaze::UserSession *associatedUserSession) override  {}
    void onNotifyServerMatchmakingReservedExternalPlayers(const Blaze::GameManager::NotifyServerMatchmakingReservedExternalPlayers &, Blaze::UserSession *) override {}
    //}}
    //{{
    // SearchShardingBroker overrides
    void onNotifyFindGameFinalizationUpdate(const Blaze::Search::NotifyFindGameFinalizationUpdate& data, UserSession *associatedUserSession) override {} // Packer should not trigger legacy MM search slave notifications
    void onNotifyGameListUpdate(const Blaze::Search::NotifyGameListUpdate& data, ::Blaze::UserSession *associatedUserSession) override;
    //}}

    GetDetailedMetricsError::Error processGetDetailedMetrics(Blaze::GameManager::GetDetailedPackerMetricsResponse &response, const ::Blaze::Message* message) override;
    uint32_t getBucketFromScore(float score);

    EA::TDF::TimeValue getTimeBetweenWorkSlotClaims() const;
    void onSubscribedForNotificationsForMaster(InstanceId instanceId);
    void scheduleClaimWorkSlotIdAndNotifyMasters(EA::TDF::TimeValue newDeadline);
    void claimWorkSlotIdAndNotifyMasters();
    uint32_t getWorkerLayerCount() const;
    uint32_t getAssignedLayerId() const;
    bool isRootLayer() const;
    static uint32_t getLayerIdFromSlotId(uint32_t slotId);
    void scheduleObtainSession(InstanceId instanceId);
    void obtainSession(InstanceId instanceId);
    void migrateSession(PackerSessionId packerSessionId);
    void abortSession(PackerSessionId packerSessionId, GameManager::MatchmakingScenarioId originatingScenarioId);
    void cleanupSession(PackerSessionId packerSessionId, GameManager::WorkerSessionCleanupReason removalReason, bool repack = true);
    void sendSessionStatus(PackerSessionId packerSessionId);
    bool addSessionToPackerSilo(PackerSessionId packerSessionId, PackerSiloId packerSiloId);
    void removeSessionFromPackerSilo(PackerSession& packerSession, Packer::PackerSilo& packerSilo, const char8_t* removalReason);
    void packGames(PackerSiloId packerSiloId);
    void scheduleReapGames(PackerSiloId packerSiloId);
    void reapGames(PackerSiloId packerSiloId);

    void handlePackerImproveGame(const Packer::Game& game, const Packer::Party& incomingParty) override;
    void handlePackerRetireGame(const Packer::Game& game) override;
    void handlePackerEvictParty(const Packer::Game& game, const Packer::Party& evictedParty, const Packer::Party& incomingParty) override;
    void handlePackerReapGame(const Packer::Game& game, bool viable) override;
    void handlePackerReapParty(const Packer::Game& game, const Packer::Party& party, bool expired) override;
    void finalizePackedGame(PackerFinalizationJobId jobId);

protected:
    struct SlotClaimInfo
    {
        SlotClaimInfo() = default;
        SlotClaimInfo(const SlotClaimInfo& other) = delete;
        uint32_t mSlotId = 0;
        uint64_t mSlotVersion = 0; // currently write-only, merely used to keep track of last communication to ease debugging
        uint64_t mInstanceRegistrationId = 0;
        eastl::string mInstanceName;
    };

    // PACKER_TODO: Get rid of this, each packerSiloInfo should manage this by PackerSession, each session should have an expiry timer, each game as well.
    // VALIDATE: We need to check if active games also currently respect the cooldown timer...
    struct PackerSiloReapData
    {
        TimerId mReapPackingsTimerId;
        EA::TDF::TimeValue mNextReapDeadline;
        EA::TDF::TimeValue mLastReapTime;
        uint64_t mReapCount;
        PackerSiloReapData() : mReapPackingsTimerId(INVALID_TIMER_ID), mReapCount(0) {}
        PackerSiloReapData(const PackerSiloReapData& other) = delete;
        ~PackerSiloReapData() {
            EA_ASSERT(mReapPackingsTimerId == INVALID_TIMER_ID);
        }
    };

    // PACKER_TODO: Move this wholesale into the PackerSiloInfo
    struct GameQueryTracker {
        GameManager::GameBrowserListId mGameBrowserListId = 0;
        Blaze::InstanceIdList mResolvedSearchSlaveList; // PACKER_TODO: how do we handle possible mutation of this set?
        eastl::vector_set<PackerSiloId> mInitiatorSiloIdSet; // packer silos that initiated this query
        struct GameTemplate* mGameTemplate = nullptr; // PACKER_TODO: eliminate this member, it should not be necessary once mGamePackerUtil->getPackerSilo(gameTemplateName, packerSiloId); does not take a template name.
    };

    struct GameTemplate
    {
        GameTemplate(const char8_t* gameTemplateName);
        GameTemplate(const GameTemplate& other) = delete;
        ~GameTemplate();
        eastl::string mGameTemplateName;
        // PACKER_TODO: for now mCreateGameTemplateConfig is only used for one thing, may be candidate for elimination in favor of a simple lookup
        EA::TDF::tdf_ptr<GameManager::CreateGameTemplate> mCreateGameTemplateConfig; // cached config ref

        struct InternalMetrics {
            uint64_t mTotalAddPartyPackingTime = 0;
            uint64_t mTotalRemovePartyPackingTime = 0;
            uint64_t mTotalReapGamePackingTime = 0;
            uint64_t mTotalReapPartySiblingsPackingTime = 0;
            uint64_t mTotalNewPartyCount = 0;
        } mMetrics;
    };
    friend struct GameTemplate;

    enum TimedFinalizationStage
    {
        JOB_QUEUED,
        JOB_LOCK_SCENARIOS,
        JOB_CREATE_GAME,
        JOB_FIND_GAME,
        JOB_FIND_DS,
        JOB_JOIN_EXTERNAL_SESSION,
        JOB_TIMED_STAGES_MAX // Sentinel
    };
    typedef eastl::array<EA::TDF::TimeValue, JOB_TIMED_STAGES_MAX> FinStageTimeList;
    typedef eastl::hash_map<EA::TDF::TdfString, eastl::vector<EA::TDF::GenericTdfType>, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> PropertyValuesMap;

    struct PackerSession : public GameManager::Rete::ReteRuleProvider
    {
        PackerSession(GameTemplate& parentTemplate);
        PackerSession(const PackerSession& other) = delete;
        ~PackerSession();
        void addToReteIndex();
        void removeFromReteIndex();
        void cancelUpdateTimer();
        void cancelMigrateTimer();
        void setCleanupReason(const char8_t* reason);
        void setRemoveFromSiloReason(const char8_t* reason);
        UserSessionId getLogContextOverrideUserSessionId() const;
        GameManager::Rete::ProductionId getProductionId() const override { return mPackerSessionId; }
        const GameManager::Rete::ReteRuleContainer& getReteRules() const override { return mReteRules; }
        GameManager::UserJoinInfo* getHostJoinInfo() const;
        GameManager::MatchmakingScenarioId getOriginatingScenarioId() const;
        EA::TDF::TimeValue getMigrateUnfulfilledTime() const;

        // PACKER_TODO: Adjust these functions when mMmInternalRequest is removed/replaced.
        GameManager::PropertyNameMap& getPropertyMap() const { return mMmInternalRequest->getMatchmakingPropertyMap(); }
        GameManager::ScenarioAttributes& getAttributeMap() const { return mMmInternalRequest->getScenarioAttributes(); }
        GameManager::MatchmakingFilterCriteriaMap& getFilterMap() const { return mMmInternalRequest->getMatchmakingFilters().getMatchmakingFilterCriteriaMap(); }

        GameTemplate& mGameTemplate;
        PackerSessionId mPackerSessionId = 0; // Slivered unique id
        EA::TDF::TimeValue mSessionStartTime;
        EA::TDF::TimeValue mSessionExpirationTime;
        TimerId mMigrateSessionTimerId = 0;
        TimerId mSendUpdateTimerId = 0;
        PackerFinalizationJobId mFinalizationJobId = 0;
        uint32_t mFinalizationAttempts = 0;
        EA::TDF::TimeValue mAddedToGameTimestamp;
        EA::TDF::TimeValue mTotalTimeInViableGames;
        uint64_t mAddedToJobNewPartyCount = 0;
        uint64_t mEvictedCount = 0;
        uint64_t mRequeuedCount = 0;
        uint64_t mUnpackedCount = 0;
        bool mRegisteredWithRete = false;
        bool mOrphaned = false; // removed by master while part of a finalization job
        enum PackerSessionState {
            PACKING,
            AWAITING_JOB_START,
            AWAITING_LOCK,
            LOCKED,
            AWAITING_RELEASE
        } mState = PACKING;
        GameManager::GameId mImmutableGameId = 0; // game id(active game from search slave)
        uint64_t mImmutableGameVersion = 0; // game version(from storage manager)
        uint64_t mImmutableGameListId = 0; // game list id(from search slave)
        uint32_t mImmutableTeamIndex = 0; // 
        GameManager::UserGroupId mImmutableGroupId;
        struct ImmutableScoringInfo {
            GameManager::ListString mKeys;
            GameManager::PropertyValueMap mScoringValueMap;
        };
        eastl::vector<ImmutableScoringInfo> mImmutableScoringInfoByFactor;

        struct ParentMetricsSnapshotAtCreation {
            uint64_t mAddedToGameNewPartyCount = 0; // number of new parties when this session's party was added to game PACKER_TODO: move this into packer lib where this info can be per silo...
            uint64_t mTotalNewPartyCount = 0; // PACKER_TODO: move this into packer lib where this info can be per silo...
        } mParentMetricsSnapshot;
        EA::TDF::tdf_ptr<GameManager::StartMatchmakingInternalRequest> mMmInternalRequest;
        GameManager::UserSessionIdList mUserSessionIds; // elements point back into mMmInternalRequest
        GameManager::Rete::ReteRuleContainer mReteRules; // currently contains only PropertyRule
        eastl::hash_set<PackerSiloId> mMatchedPackerSiloIdSet;
        eastl::hash_set<PackerSiloId> mRemovedPackerSiloIdSet;
        const char8_t* mLastRemovalReason = nullptr;
        const char8_t* mLastCleanupReason = nullptr;
        EA::TDF::TimeValue mCleanupTimestamp;
        EA::TDF::TimeValue mRemovalTimestamp;
        EA::TDF::TimeValue mDelayCleanupTimestamp;
        FinStageTimeList mFinStageTimes;

        static const uint32_t SEND_UPDATE_PERIOD_MS = 250;
    };

    /*
     * Manages a superset of information associated with a PackerSilo at the component level
     */ 
    struct PackerSiloInfo
    {
        // Set by addPackerSilo:
        Packer::PackerSilo mPackerSilo;
        GameManager::PropertyNameMap mPackerSiloProperties;
        PackerSiloReapData mReapData;
        GameManager::Rete::ProductionNodeId mProductionNodeId = GameManager::Rete::INVALID_PRODUCTION_NODE_ID;
        struct InternalMetrics {
            uint64_t mTotalAddPartyPackingTime = 0;
            uint64_t mTotalReapGamePackingTime = 0;
        } mMetrics;

        ~PackerSiloInfo();
    };

    friend struct PackerSiloInfo;

    //Adding GamePackerUtil functionalities here
    struct PackerDetailedMetrics
    {
        // PACKER_TODO(metrics): Add party exit packer metrics: We need to determine if the best place for these metrics is external to this file because reasons for removal of a party from the packer can be external (migrated, left, etc)
        //1. Metric of party time-to-last-improvement duration, dimensioned by outcome (completed/migrated/expired)
        //2. Metric of party evictions, dimensioned by outcome (completed/migrated/expired)
        //3. Metric of party potential candidates, dimensioned by outcome (completed/migrated/expired)

        // PACKER_TODO(metrics): come up with better name than breakdown
        eastl::hash_map<eastl::string, eastl::array<uint64_t, 101>> mFactorScoreBreakdownForAllGames;
        eastl::hash_map<eastl::string, eastl::array<uint64_t, 101>> mFactorScoreBreakdownForReapedGames;

        // PACKER_TODO(metrics): The current direct value mapping representation of frequencies can be problematic for 2 reasons 
        // 1. 1 second resolution may not provide sufficient resolution for analyzing interesting system behavior changes
        // 2. High dynamic range data such as eviction counts can result in unbounded memory usage required for tracking a large number of unique samples
        // Solution to both problems is the same, to add support for high dynamic range of data in histograms we can store a fixed point log representation of the value(or inverse value) instead of the value itself.
        // For example if we store milliseconds, by computing the log and rounding to 2 decimal places, 3000 fixed points are sufficient to represent 1 week worth of millisecond samples.
        //    milliseconds  fixed_point_log_base2  recovered_value
        //    1  0  1
        //    158  7.3  157.5864849
        //    257  8.01  257.7806208
        //    346  8.43  344.8917957
        //    1446  10.5  1448.154688
        //    1455  10.51  1458.2274
        //    80000  16.29  80126.95324
        //    100000  16.61  100024.9235
        //    604800000  29.17  604011174.7 // 1 week worth of milliseconds
        // NOTE: for values like score percentages it is likely more useful to have the highest resolution around the largest
        // sample values (difference between 99 and 97 is more important in scoring than between 1 and 3 because scores tend to cluster around higher values).
        // This means that for storign score representation an inverse of the score value's fixed point log is more useful.

        // 
        eastl::hash_map<uint64_t, uint64_t> mEvictionCountBySuccessfulParties;
        eastl::hash_map<uint64_t, uint64_t> mEvictionCountByParties;

        eastl::hash_map<uint64_t, uint64_t> mTimeSpentInViableGamesByParties;
        eastl::hash_map<uint64_t, uint64_t> mTimeSpentInViableGamesBySuccessfulParties;

        eastl::hash_map<uint64_t, uint64_t> mTimeSpentInPackerByParty; // PACKER_TODO(metrics): This does not seem very useful because for unsuccessful parties time spent will be equal to timeout which is known
        eastl::hash_map<uint64_t, uint64_t> mTimeSpentInPackerBySuccessfulParty;
    };

    typedef eastl::hash_map<PackerSiloId, PackerSiloInfo> PackerSiloInfoMap;
    PackerSiloInfoMap mPackerSiloInfoMap;
    typedef eastl::hash_set<PackerSiloId> PackerIdSet;
    PackerIdSet mPackersMarkedForRemoval;
    eastl::hash_map<eastl::string, PackerDetailedMetrics> mPackerDetailedMetricsByGameTemplateMap;
    TimerId mSweepEmptySilosTimerId;
    typedef eastl::hash_map<GameManager::Rete::ProductionNodeId, PackerSiloId> ProdNodeToPackerSiloIdMap;
    ProdNodeToPackerSiloIdMap mProdNodeIdToSiloIdMap;

    Packer::PackerSilo* getPackerSilo(PackerSiloId packerSiloId);
    PackerSiloInfo* getPackerSiloInfo(PackerSiloId packerSiloId);
    bool isPackerPartyInViableGame(PackerSiloId packerSiloId, Packer::PartyId partyId);
    void markPackerSiloForRemoval(PackerSiloId packerSiloId);
    bool unmarkPackerSiloForRemoval(PackerSiloId packerSiloId);
    bool addPackerSilo(PackerSession& packerSession, PackerSiloId packerSiloId, Blaze::GameManager::Rete::ProductionNode* node);
    bool initPackerSilo(Packer::PackerSilo& packer, PackerSession& packerSession, const Blaze::GameManager::CreateGameTemplate& cfg);
    
    void sweepEmptyPackerSilos();
    Packer::Time getPackerPartyExpiryTime(PackerSiloId packerSiloId, Packer::PartyId partyId);
    void getDetailedPackerMetrics(GameManager::GetDetailedPackerMetricsResponse& response);
    void getStatusInfo(ComponentStatus& status) const override;
    bool isMutableParty(Packer::PartyId partyId);

    Packer::Time utilPackerMeasureTime() override;
    Packer::Time utilPackerRetirementCooldown(Packer::PackerSilo& packer) override;
    Packer::GameId utilPackerMakeGameId(Packer::PackerSilo& packer) override;
    void utilPackerUpdateMetric(Packer::PackerSilo& packer, Packer::PackerMetricType type, int64_t delta) override;
    void utilPackerLog(Packer::PackerSilo& packer, Packer::LogLevel level, const char8_t* fmt, va_list args) override;
    bool utilPackerTraceEnabled(Packer::TraceMode mode) const override;
    void handlePackerReapGameAux(const Packer::Game& game, bool viable);
    void handlePackerReapPartyAux(const Packer::Game& game, const Packer::Party& party, bool expired);

    class GameManager::Matchmaker::PropertyRuleDefinition* getPropertyRuleDefinition();

    typedef eastl::vector_map<GameManager::MatchmakingScenarioId, PackerSession*> PackerSessionByScenarioIdMap; // IMPORTANT: Needs to be an ordered map to enforce global order for packer session lock acquisition!

    struct FinalizationJob
    {
        ~FinalizationJob();
        PackerFinalizationJobId mJobId = 0;
        PackerSessionId mInitiatorPackerSessionId = 0;
        GameManager::MatchmakingScenarioId mInitiatorScenarioId = 0;
        EA::TDF::tdf_ptr<GameManager::MatchmakingCreateGameRequest> mGmMasterRequest;
        GameManager::CreateGameMasterResponse mGmMasterResponse;
        PackerSessionByScenarioIdMap mPackerSessionByScenarioIdMap;
        GameManager::GameId mGameId = 0;
        PackerSiloId mGamePackerSiloId = 0; // DEBUGGING: packer silo that issued the game for this job
        uint64_t mGameRecordVersion = 0; // DEBUGGING: game record version (only valid if active game)
        uint64_t mGameImproveCount = 0; // DEBUGGING: number of times game was improved since creation 
        EA::TDF::TimeValue mGameCreatedTimestamp; // DEBUGGING: game creation time
        EA::TDF::TimeValue mGameImprovedTimestamp; // DEBUGGING: game improval time
        Packer::ScoreList mGameFactorScores;
        uint32_t mGameScore = 0;
        EA::TDF::TimeValue mJobCreatedTimestamp;
        FinStageTimeList mFinStageTimes;
        GameManager::PackerFinalizationTrigger mFinalizationTrigger;
        GameManager::PingSiteAliasList mPingsiteAliasList;
    };
    // PACKER_TODO: track finalization job processing metrics, number of jobs, outcomes.

    static const char8_t* getInstanceName(InstanceId instanceId, const eastl::string& instanceName = "");
    bool isJobValid(FinalizationJob& job, PackerSession::PackerSessionState state = PackerSession::LOCKED) const;
    void completeJob(BlazeError overallResult, PackerFinalizationJobId jobId, GameManager::MatchmakingResult result = GameManager::SESSION_ERROR_GAME_SETUP_FAILED, GameManager::PlayerRemovedReason reason = GameManager::PLAYER_LEFT_CANCELLED_MATCHMAKING);
    bool notifyOnTokenAdded(GameManager::Rete::ProductionInfo& info) const override;
    bool notifyOnTokenRemoved(GameManager::Rete::ProductionInfo& info) override;
    bool onTokenAdded(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override;
    void onTokenRemoved(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override;
    void onTokenUpdated(const GameManager::Rete::ProductionToken& token, GameManager::Rete::ProductionInfo& info) override;
    GameManager::Rete::ProductionId getProductionId() const override;

    GameManager::PropertyManager mPropertyManager;

    GameManager::ExternalSessionUtilManager* mExternalGameSessionServices = nullptr;
    GameManager::Rete::ReteNetwork* mReteNetwork = nullptr;
    GameManager::Matchmaker::RuleDefinitionCollection* mRuleDefinitionCollection = nullptr;
    eastl::hash_map<InstanceId, SlotClaimInfo> mSlotClaimByInstanceMap; // used to track which masters have been sent the latest slot version info
    eastl::hash_map<InstanceId, Fiber::FiberId> mFetcherFiberByInstanceMap;
    eastl::hash_map<eastl::string, GameTemplate*> mGameTemplateMap;
    eastl::hash_map<PackerSessionId, PackerSession*> mPackerSessions;
    eastl::hash_map<GameManager::GameBrowserListId, GameQueryTracker*> mActiveGameQueryByListIdMap; // active GB lists by GB list id, PACKER_TODO: Replace with mapping of GBListId -> PackerSiloId
    eastl::vector<PackerSessionId> mDelayCleanupSessions; // PACKER_TODO: #SIMPLIFY This is needed only because we sometimes try to cleanup PackerSession within reap(). Reap uses the game/party/player objects linked to the PackerSession being cleaned up. removePartyFromSilo() invoked from cleanupSession() also also tries to do this and stomps on the reap's state. The solution is to entirely do away with callbacks for triggering reaps instead of erasing games, parties that were reaped, just have the games switch from the working set to the issued set. Games probably should be using intrusive lists for this, also they should be made much smaller (change party, player arrays) to keep more data in the cache.
    eastl::hash_map<PackerFinalizationJobId, FinalizationJob> mFinalizationJobs;
    FiberJobQueue mFinalizationJobQueue;
    GameManager::Rete::ProductionId mProductionId;
    eastl::map<PackerSiloId, GameQueryTracker> mActiveGameQueryBySiloIdMap; // active GB lists by condition, owns the tracker, PACKER_TODO: remove this map, and store GBListId in PackerSiloInfo, condition string should probably not be stored at all, except for validation

    uint32_t mWorkSlotCount;
    uint32_t mAssignedWorkSlotId;
    uint64_t mAssignedWorkSlotVersion;
    TimerId mWorkSlotUpdateTimerId;
    Fiber::FiberId mWorkSlotUpdateFiberId;
    EA::TDF::TimeValue mWorkSlotUpdateDeadline;

    //Metrics::TaggedGauge<GameManager::ScenarioName, GameManager::SubSessionName> mSilosActive; // PACKER_TODO: is this more for diagnotics?
    Metrics::TaggedGauge<GameManager::ScenarioName, GameManager::SubSessionName> mSessionsActive;
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::SubSessionName> mSessionsAcquired;
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::SubSessionName> mSessionsAborted;
    Metrics::Counter mSessionsAcquireNone; // aka EOF
    Metrics::Counter mSessionsAcquireFailed; // PACKER_TODO: add reason if workerObtainPackerSession ever returns an error besides ERR_SYSTEM
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::SubSessionName, GamePacker::RemovalReason> mSessionsRemoved;
    Metrics::TaggedGauge<PackerGameTemplateName> mPlaceholderSessionsActive;
    Metrics::TaggedCounter<PackerGameTemplateName> mPlaceholderSessionsAcquired;
    Metrics::TaggedCounter<PackerGameTemplateName, RemovalReason> mPlaceholderSessionsRemoved;
    Metrics::TaggedCounter<PackerGameTemplateName, const char8_t*, uint32_t, GameManager::PackerFinalizationTrigger, const char8_t*> mGamesFinalized;
    Metrics::TaggedCounter<PackerGameTemplateName, const char8_t*, uint32_t, GameManager::PackerFinalizationTrigger> mGamesPacked;
    
    Metrics::TaggedGauge<eastl::string> mPackerPlayersGauge;
    Metrics::TaggedGauge<eastl::string> mPackerPartiesGauge;
    Metrics::TaggedGauge<eastl::string> mPackerViableGamesGauge;
    Metrics::TaggedGauge<eastl::string> mPackerGamesGauge; // PACKER_TODO: parmeterize by state (viable, nonviable)
    Metrics::TaggedGauge<eastl::string> mPackerSilosGauge; // PACKER_TODO: this should just return mPackerMap.size() once we implement  TaggedPolledGauge

    Metrics::TaggedCounter<eastl::string> mPackerSilosCreatedCounter;
    Metrics::TaggedCounter<eastl::string> mPackerSilosDestroyedCounter;
    Metrics::TaggedCounter<eastl::string> mGamesCreatedCounter;
    Metrics::TaggedCounter<eastl::string> mGamesReapedCounter; // PACKER_TODO: parameterize by state (viable, nonviable)
    Metrics::TaggedCounter<eastl::string> mViableGamesReapedCounter;
    Metrics::TaggedCounter<eastl::string, eastl::string, uint32_t> mFactorScoreFreq; // uint32_t here is the percent score 0-100
    
    Metrics::TaggedCounter<PackerGameTemplateName, const char8_t*> mPackerFoundGameFailed;
    Metrics::TaggedCounter<PackerGameTemplateName, const char8_t*> mTotalPackingTime;

};

} // namespace GamePacker
} // namespace Blaze


#endif // BLAZE_GAMEPACKER_SLAVE_IMPL_H
