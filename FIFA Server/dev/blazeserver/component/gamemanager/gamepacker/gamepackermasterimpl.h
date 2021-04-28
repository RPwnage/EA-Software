/*! ************************************************************************************************/
/*!
    \file gamepackermasterimpl.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_GAMEPACKER_MASTER_IMPL_H
#define BLAZE_GAMEPACKER_MASTER_IMPL_H

#include <EASTL/array.h>
#include <EASTL/intrusive_list.h>
//#include <EASTL/bonus/ring_buffer.h> // Used this later when we implement buffered recency metrics
#include "gamemanager/tdf/matchmaker_types.h"
#include "gamemanager/tdf/matchmaker_server.h"
#include "gamemanager/rpc/gamepackermaster_stub.h"
#include "gamemanager/tdf/gamepacker_server.h"
#include "gamemanager/matchmakercomponent/timetomatchestimator.h"
#include "framework/usersessions/usersessionsubscriber.h" // for UserSessionSubscriber
#include "framework/usersessions/qosconfig.h" // for PingSiteUpdateSubscriber
#include "framework/controller/drainlistener.h"
#include "framework/redis/rediskeymap.h"
#include "framework/storage/storagetable.h"
#include "framework/system/fiberjobqueue.h"

namespace Blaze
{

class UserSession;

namespace GameManager
{
    namespace Rete
    {
        class StringTable;
    }
    class UserJoinInfo;
    class UserSessionInfo;
}

namespace GamePacker
{

// From gamepacker_server.tdf
using GameManager::PackerScenarioId;
using GameManager::PackerSessionId;
using GameManager::PackerSiloId;
using GameManager::PackerFinalizationJobId;

using GameManager::PackerPlayerId;
using GameManager::PackerPartyId;
using GameManager::PackerGameId;
using GameManager::PackerPinData;

using GameManager::INVALID_PACKER_SESSION_ID;


class GamePackerMasterImpl : public GamePackerMasterStub, 
                             private UserSessionSubscriber,
                             //private PingSiteUpdateSubscriber,
                             private DrainStatusCheckHandler
{
public:
    GamePackerMasterImpl();
    ~GamePackerMasterImpl() override;

private:

    struct PackerScenarioMaster;
    struct WorkerInfo;
    struct LayerInfo;

    struct SessionInfo
    {
        uint64_t mSiloCount = 0;
        uint64_t mEvictionCount = 0;
    };
    typedef eastl::hash_map<InstanceId, SessionInfo> SessionInfoByInstanceIdMap;


    /**
     *  Tracks a packing session assigned to a given level in the layer hierarchy.
      - Owned by mSessionByIdMap;
      - Intrusive list pointer held by mUnassignedSessionList + mIdle/WorkingSessionList (Layer)
      - Pointer held by mPackerSessionMasterList (Scenario)

        PACKER_TODO:  Move the Session related code in here, for functional isolation. 
     */
    struct PackerSessionMaster : public eastl::intrusive_list_node
    {
    public:
        PackerSessionMaster(PackerSessionId packerSessionId, PackerScenarioMaster& packerScenario, GameManager::PackerSessionDataPtr packerSessionData = nullptr);
        ~PackerSessionMaster();

        PackerSessionMaster(const PackerSessionMaster&) = delete;
        PackerSessionMaster(PackerSessionMaster&&) = default;
        PackerSessionMaster& operator=(const PackerSessionMaster&) = delete;

        // Accessor functions for PackerSessionData: 
        PackerSessionId getPackerSessionId() const { return mSessionData->getPackerSessionId(); }
        InstanceId getAssignedWorkerInstanceId() const { return mSessionData->getAssignedWorkerInstanceId(); }
        int32_t getAssignedLayerId() const { return mSessionData->getAssignedLayerId(); }
        TimeValue getStartedTimestamp() const { return mSessionData->getStartedTimestamp(); }
        TimeValue getAcquiredTimestamp() const { return mSessionData->getAcquiredTimestamp(); }
        TimeValue getReleasedTimestamp() const { return mSessionData->getReleasedTimestamp(); }
        TimeValue getUpdatedStatusTimestamp() const { return mSessionData->getUpdatedStatusTimestamp(); }
        TimeValue getImprovedScoreTimestamp() const { return mSessionData->getImprovedScoreTimestamp(); }
        TimeValue getPendingReleaseTimestamp() const { return mSessionData->getPendingReleaseTimestamp(); }
        TimeValue getUnassignedDuration() const { return mSessionData->getUnassignedDuration(); }
        uint64_t getReleaseCount() const { return mSessionData->getReleaseCount(); }
        GameManager::PackerSessionData::SessionState getState() const { return mSessionData->getState(); }
        GameManager::OverallScoringInfo& getOverallScoringInfo() const { return mSessionData->getOverallScoringInfo(); }
        uint64_t getMigrationCount() const { return mSessionData->getMigrationCount(); }
        uint64_t getTotalSiloCount() const { return mSessionData->getTotalSiloCount(); }
        uint64_t getMaxSiloPlayerCount() const { return mSessionData->getMaxSiloPlayerCount(); }
        RiverPoster::GqfDetailsList& getFactorDetailsList() const { return mSessionData->getFactorDetailsList(); }
        uint64_t getTotalEvictionCount() const { return mSessionData->getTotalEvictionCount(); }
        uint64_t getMatchedPlayersInGameCount() const { return mSessionData->getMatchedPlayersInGameCount(); }
        EA::TDF::TimeValue getRemainingLifetime(EA::TDF::TimeValue now) const;

        uint32_t getDurationPercentile() const;

        // Accessor functions for PackerSessionRequest: 
        const char8_t* getSubSessionName() const { return mSessionRequest->getSubSessionName(); }
        EA::TDF::TimeValue getStartDelay() const { return mSessionRequest->getMatchmakingSessionData().getStartDelay(); }
        const EA::TDF::TdfString& getCreateGameTemplateName() const { return mSessionRequest->getCreateGameTemplateNameAsTdfString(); }
        GameManager::MatchmakingFilterCriteria& getMatchmakingFilters() { return mSessionRequest->getMatchmakingFilters(); }    // Non-const values (changed by initializeAggregateProperties)
        GameManager::PropertyNameMap& getMatchmakingPropertyMap() { return mSessionRequest->getMatchmakingPropertyMap(); }

        // Helper functions for accessing Scenario owned data: 
        PackerScenarioMaster& getPackerScenarioMaster() { return *mParentPackerScenario; }
        PackerScenarioId getParentPackerScenarioId() const { return mParentPackerScenario->getPackerScenarioId(); }
        GameManager::MatchmakingScenarioId getOriginatingScenarioId() const { return mParentPackerScenario->getOriginatingScenarioId(); }
        UserSessionId getOwnerUserSessionId() const { return mParentPackerScenario->getOwnerUserSessionInfo().getSessionId(); }
        const GameManager::UserJoinInfo* getHostJoinInfo() const { return mParentPackerScenario->getHostJoinInfo(); }

     // Functions:
        void cancelDelayStartTimer();
        void workerClaimSession(WorkerInfo& worker);
        void releaseSessionFromWorker(GameManager::PackerSessionData::SessionState releaseReason);
        void updatePackerSession(const PackerPinData& pinData);

        // Intrusive lists in EASTL are dangerous, they don't set next/prev consistently, and can cause obscure issues when used with multiple lists simultaneously. 
        void addToIntrusiveList(eastl::intrusive_list<PackerSessionMaster>& list);

// private:
        // Packer Internal Data:
        TimerId mDelayedStartTimerId;
        PackerScenarioMaster* mParentPackerScenario = nullptr;
        GameManager::PackerSessionInternalRequest* mSessionRequest = nullptr;
        
        GameManager::PackerSessionDataPtr mSessionData; // PACKER_TODO: Rename to PackerSession
        SessionInfoByInstanceIdMap mSessionInfoByInstanceIdMap;
    };

    typedef eastl::intrusive_list<PackerSessionMaster> PackerSessionList;

    /** Tracks a packing scenario instance that owns one or more sessions - Tracked by mPackerScenarioByIdMap */
    struct PackerScenarioMaster
    {
    public:
        PackerScenarioMaster(PackerScenarioId scenarioId, GameManager::StartPackerScenarioRequestPtr scenarioRequest, GameManager::PackerScenarioDataPtr scenarioData = nullptr);
        ~PackerScenarioMaster();

        // Accessor functions for PackerScenarioData:
        PackerScenarioId getPackerScenarioId() const { return mScenarioData->getPackerScenarioId(); }
        TimeValue getStartedTimestamp() const { return mScenarioData->getStartedTimestamp(); }
        TimeValue getExpiresTimestamp() const { return mScenarioData->getExpiresTimestamp(); }
        TimeValue getLockedTimestamp() const { return mScenarioData->getLockedTimestamp(); }
        TimeValue getLockExpiresTimestamp() const { return mScenarioData->getLockExpiresTimestamp(); }
        PackerSessionId getLockedForFinalizationByPackerSessionId() const { return mScenarioData->getLockedForFinalizationByPackerSessionId(); }
        const EA::TDF::TdfString& getExternalSessionName() const { return mScenarioData->getExternalSessionNameAsTdfString(); }
        GameManager::PackerScenarioData::TerminationReason getTerminationReason() const { return mScenarioData->getTerminationReason(); }

        uint32_t getDurationPercentile() const;

        // Accessor functions for StartPackerScenarioRequest:
        const GameManager::StartPackerScenarioRequest& getScenarioRequest() const { return *mScenarioRequest; }

        TimeValue getScenarioDuration() const { return mScenarioRequest->getPackerScenarioRequest().getScenarioTimeoutDuration(); }
        GameManager::MatchmakingScenarioId getOriginatingScenarioId() const { return mScenarioRequest->getPackerScenarioRequest().getScenarioInfo().getOriginatingScenarioId(); }  // Avoid using this when possible.  PACKER_TODO - Remove.
        const GameManager::UserSessionInfo& getOwnerUserSessionInfo() const { return mScenarioRequest->getPackerScenarioRequest().getOwnerUserSessionInfo(); }
        const GameManager::UserJoinInfoList& getUserJoinInfoList() const { return mScenarioRequest->getPackerScenarioRequest().getUsersInfo(); }
        const GameManager::ScenarioName& getScenarioName() const { return mScenarioRequest->getPackerScenarioRequest().getScenarioInfo().getScenarioNameAsTdfString(); }
        // We hold Attributes on the Scenario, not the Session.  (Properties vary by Session)
        const GameManager::ScenarioAttributes& getScenarioAttributes() const { return mScenarioRequest->getPackerScenarioRequest().getScenarioInfo().getScenarioAttributes(); }
        const GameManager::UserJoinInfo* getHostJoinInfo() const;

    // Functions:
        // Add the Session, and associates it with Scenario-owned data (Request, etc.) - Note:  PackerScenario does not currently own the PackerSessionMaster
        bool assignPackerSession(PackerSessionMaster& packerSession);

        void cancelFinalizationTimer();
        void cancelExpiryTimer();

//  private:
        // Packer Internal Data:
        TimerId mExpiryTimerId;
        TimerId mFinalizationTimerId;

        eastl::vector<PackerSessionMaster*> mPackerSessionMasterList;
        GameManager::PackerScenarioDataPtr mScenarioData;

        // Request Data:
        GameManager::StartPackerScenarioRequestPtr mScenarioRequest;    // Original request sent to Packer - possibly modified by initializeAggregateProperties.  (PACKER_TODO - Moving that to Scenarios would allow this member to be const.)
    };

    /**
     *  Tracks a worker assigned to a given level in the layer hierarchy.
     */
    struct WorkerInfo : public eastl::intrusive_list_node
    {
        enum WorkerState {
            IDLE,
            WORKING,
            STATE_MAX
        };
        InstanceId mWorkerInstanceId;
        LayerInfo* mWorkerLayerInfo;
        uint32_t mWorkerSlotId;
        WorkerState mWorkerState;
        TimerId mWorkerResetStateTimerId;
        
        eastl::string mWorkerName;
        eastl::string mWorkerInstanceType; // PACKER_TODO: This will be used to support Scenario Partitioning by instance type name (see Hansoft)

        PackerSessionList mWorkingSessionList;   // Sessions in the SESSION_ASSIGNED / SESSION_SUSPENDED / SESSION_AWAITING_RELEASE / SESSION_COMPLETED / SESSION_ABORTED states. 

        uint32_t getLayerId() const { return mWorkerLayerInfo->mLayerId; }

        WorkerInfo(InstanceId workerInstanceId, LayerInfo* layerInfo, const Blaze::GameManager::WorkerLayerAssignmentRequest &request);
        ~WorkerInfo();
        void setWorkerState(GamePackerMasterImpl::WorkerInfo::WorkerState state);
        void assignWorkerSession(PackerSessionMaster& packerSession);

        static const char8_t* getWorkerStateString(WorkerState state);
        static const uint32_t RESET_BUSY_STATE_TIMEOUT_MS;
        static const uint32_t RESET_IDLE_STATE_TIMEOUT_MS;
        WorkerInfo(const WorkerInfo&) = delete;
        WorkerInfo(WorkerInfo&&) = delete;
        WorkerInfo& operator=(const WorkerInfo&) = delete;
    };

    typedef eastl::intrusive_list<WorkerInfo> WorkerInfoList;
    typedef eastl::vector_map<InstanceId, WorkerInfo*> WorkerByInstanceIdMap;
    typedef eastl::vector_map<uint32_t, WorkerInfo*> WorkerBySlotIdMap;
    typedef eastl::hash_map<WorkerInfo::WorkerState, WorkerInfo*> WorkerByStateMap;

    /**
     *  Tracks a layer assigned to a given level in the layer hierarchy.
     */
    struct LayerInfo
    {
        uint32_t mLayerId;
        mutable eastl::array<uint64_t, WorkerInfo::STATE_MAX> mWorkerMetricsByState;
        eastl::array<WorkerInfoList, WorkerInfo::STATE_MAX> mWorkerByState;
        PackerSessionList mIdleSessionList;      // Sessions in the SESSION_AWAITING_ASSIGNMENT state that were released from their Worker. 
        PackerSessionList mOtherSessionList;     // All other Sessions that were released from their Worker.  (not elligible for assignment)

        LayerInfo();
        ~LayerInfo();
        bool hasWorkers() const;
        static uint32_t layerIdFromSlotId(uint32_t slotId);
        static uint32_t slotIndexFromSlotId(uint32_t slotId);
        static const uint32_t INVALID_LAYER_ID;
        static const uint32_t INVALID_SLOT_ID;
        static const uint32_t INVALID_SLOT_VERSION;
        static const uint32_t MAX_LAYERS = 12; // defined in header because it's referenced in LayerInfoList declaration
        static const uint32_t MAX_SLOTS;
        // Disallow all copy/move operations because they cannot be defined correctly because LayerInfo contains intrusive data structures
        LayerInfo(const LayerInfo&) = delete;
        LayerInfo(LayerInfo&& other) = delete;
        LayerInfo& operator=(const LayerInfo&) = delete;
    };

    typedef eastl::array<LayerInfo, LayerInfo::MAX_LAYERS> LayerInfoList;
    typedef eastl::vector<uint64_t> LayerSlotVersionList;
    typedef eastl::hash_map<PackerSessionId, PackerSessionMaster*> PackerSessionMap;
    typedef eastl::hash_map<PackerScenarioId, PackerScenarioMaster*> PackerScenarioMasterMap;
    typedef eastl::hash_multimap<BlazeId, PackerScenarioId> PackerScenarioIdByBlazeIdMap;
    typedef eastl::vector<PackerScenarioId> PackerScenarioIdList;

    bool onValidateConfig(GamePackerConfig& config, const GamePackerConfig* referenceConfigPtr, ::Blaze::ConfigureValidationErrors& validationErrors) const override;
    bool onConfigure() override;
    bool onPrepareForReconfigure(const GamePackerConfig& newConfig) override;
    bool onReconfigure() override;
    bool onResolve() override;
    void onShutdown() override;
    void onSlaveSessionRemoved(SlaveSession& session) override;
    void getStatusInfo(ComponentStatus& status) const override;

    // rpc interface
    StartPackerScenarioError::Error processStartPackerScenario(const Blaze::GameManager::StartPackerScenarioRequest &req, Blaze::GameManager::StartPackerSessionResponse &response, Blaze::GameManager::StartPackerSessionError &error, const ::Blaze::Message* message) override;
    CancelPackerScenarioError::Error processCancelPackerScenario(const Blaze::GameManager::CancelPackerScenarioRequest &request, const ::Blaze::Message* message) override;
    WorkerObtainPackerSessionError::Error processWorkerObtainPackerSession(const Blaze::GameManager::WorkerObtainPackerSessionRequest &request, Blaze::GameManager::WorkerObtainPackerSessionResponse &response, const ::Blaze::Message* message) override;
    WorkerMigratePackerSessionError::Error processWorkerMigratePackerSession(const Blaze::GameManager::WorkerMigratePackerSessionRequest &request, const ::Blaze::Message* message) override;
    WorkerRelinquishPackerSessionError::Error processWorkerRelinquishPackerSession(const Blaze::GameManager::WorkerRelinquishPackerSessionRequest &request, const ::Blaze::Message* message) override;
    WorkerUpdatePackerSessionStatusError::Error processWorkerUpdatePackerSessionStatus(const Blaze::GameManager::WorkerUpdatePackerSessionStatusRequest &request, const ::Blaze::Message* message) override;
    WorkerAbortPackerSessionError::Error processWorkerAbortPackerSession(const Blaze::GameManager::WorkerAbortPackerSessionRequest &request, const ::Blaze::Message* message) override;
    WorkerCompletePackerSessionError::Error processWorkerCompletePackerSession(const Blaze::GameManager::WorkerCompletePackerSessionRequest &request, const ::Blaze::Message* message) override;
    WorkerLockPackerSessionError::Error processWorkerLockPackerSession(const Blaze::GameManager::WorkerLockPackerSessionRequest &request, const ::Blaze::Message* message) override;
    WorkerUnlockPackerSessionError::Error processWorkerUnlockPackerSession(const Blaze::GameManager::WorkerUnlockPackerSessionRequest &request, const ::Blaze::Message* message) override;
    WorkerClaimLayerAssignmentError::Error processWorkerClaimLayerAssignment(const Blaze::GameManager::WorkerLayerAssignmentRequest &request, Blaze::GameManager::WorkerLayerAssignmentResponse &response, const ::Blaze::Message* message) override;
    GetPackerMetricsError::Error processGetPackerMetrics(const Blaze::GameManager::GetPackerMetricsRequest &request, Blaze::GameManager::GetPackerMetricsResponse &response, const ::Blaze::Message* message) override;
    WorkerDrainInstanceError::Error processWorkerDrainInstance(const ::Blaze::Message* message) override;
    GetPackerInstanceStatusError::Error processGetPackerInstanceStatus(Blaze::GameManager::PackerInstanceStatusResponse &response, const ::Blaze::Message* message) override;

    // UserSessionProvider interface    
    void onUserSessionExtinction(const UserSession& userSession) override;
    // DrainStatusCheckHandler interface
    bool isDrainComplete() override;
    // Storage
    void onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot);
    void onExportStorageRecord(StorageRecordId recordId);
    // Internal
    bool wakeIdleWorker(WorkerInfo& worker);
    void drainWorkerInstance(InstanceId workerInstanceId, bool reactivateSessions = false);
    void setWorkerState(WorkerInfo& worker, WorkerInfo::WorkerState state);
    void assignWorkerSession(WorkerInfo& worker, PackerSessionMaster& packerSession);
    void onWorkerStateResetTimeout(InstanceId workerInstanceId, WorkerInfo::WorkerState state);
    const char8_t* getWorkerNameByInstanceId(InstanceId instanceId, const char8_t* unkownName = "<noworker>") const;
    LayerInfo& getLayerByWorker(const WorkerInfo& worker);
    bool getWorkersForLayer(uint32_t layerId, eastl::vector<GamePackerMasterImpl::WorkerInfo*>& workers, uint32_t& loadSum);

    void onScenarioExpiredTimeout(PackerScenarioId packerScenarioId);
    void onScenarioFinalizationTimeout(PackerScenarioId packerScenarioId);
    void startDelayedSession(PackerSessionId sessionId);
    void signalSessionAssignment(PackerSessionMaster& packerSession);
    const char8_t* getSubSessionNameByPackerSessionId(PackerSessionId packerSessionId) const;

    BlazeError initializeAggregateProperties(PackerSessionMaster& packerSession, GameManager::MatchmakingCriteriaError& criteriaError);
    void assignSessionToLayer(PackerSessionMaster& packerSession, uint32_t layerId);
    void resumeSuspendedSession(PackerSessionMaster& packerSession);
    void abortSession(PackerSessionMaster& packerSession);

    // Cleanup functions:
    void terminateScenarios(const PackerScenarioIdList& terminatedScenarioIds, GameManager::PackerScenarioData::TerminationReason terminationReason);
    void notifyAssignedSessionsTermination(PackerScenarioMaster& packerScenario);
    void removeScenarioAndSessions(PackerScenarioMaster& packerScenario, bool requestCompleted);
    void cleanupScenarioAndSessions(PackerScenarioMaster& packerScenario);
    void sendMatchmakingFailedNotification(const PackerSessionMaster& packerSession, GameManager::MatchmakingResult failureReason);

    void upsertSessionData(PackerSessionMaster& packerSession);
    void upsertScenarioRequest(PackerScenarioMaster& packerScenario);       // Request data only changes when Players do. 
    void upsertScenarioData(PackerScenarioMaster& packerScenario);
    EA::TDF::TimeValue getScenarioExpiryWithGracePeriod(EA::TDF::TimeValue expiry) const;

    PackerScenarioMaster* addPackerScenario(PackerScenarioId packerScenarioId, GameManager::StartPackerScenarioRequestPtr scenarioRequest, GameManager::PackerScenarioDataPtr packerScenarioData = nullptr);
    PackerSessionMaster* addPackerSession(PackerSessionId packerSessionId, PackerScenarioMaster& packerScenario, GameManager::PackerSessionDataPtr packerSessionData = nullptr);

    void activatePackerSession(PackerSessionMaster& packerSession, bool forceAssignNewWorker = false);
    void activatePackerScenario(PackerScenarioMaster& packerScenario);
    void addScenarioBlazeIdMapping(const GamePackerMasterImpl::PackerScenarioMaster& packerScenario);
    void removeScenarioBlazeIdMapping(const GamePackerMasterImpl::PackerScenarioMaster& packerScenario);

    LayerInfo* getLayerFromId(uint32_t layerId) { return (layerId != LayerInfo::INVALID_LAYER_ID && layerId < mLayerList.size()) ? &mLayerList[(layerId - 1)] : nullptr; }

private:
    WorkerByInstanceIdMap mWorkerByInstanceId;
    WorkerBySlotIdMap mWorkerBySlotId;

    LayerInfoList mLayerList;
    uint32_t mLayerCount = 0;
    LayerSlotVersionList mLayerSlotVersionList; // Tracks layer slot versions by layer slot index to handle conflicting slot assignments requested by multiple workers
    
    PackerScenarioMasterMap mPackerScenarioByIdMap;
    PackerScenarioIdByBlazeIdMap mPackerScenarioIdByBlazeIdMap;
    
    PackerSessionMap mSessionByIdMap;
    PackerSessionList mDelayedSessionList;    // Sessions currently in the SESSION_DELAYED states
    PackerSessionList mUnassignedSessionList; // Sessions currently in the SESSION_AWAITING_ASSIGNMENT states
    PackerSessionList mOtherSessionList;      // All other Sessions that were released from Layers without workers.  (not eligible for assignment)

    StorageTable mScenarioTable;
    RedisKeyMap<GameManager::PlayerId, GameManager::GameId> mDedicatedServerOverrideMap;

    Matchmaker::TimeToMatchEstimator mTimeToMatchEstimator;
    TimeValue mEstimatedTimeToMatch;

    // metrics
    // PACKER_TODO: currently can't enable mWorkersActive because WorkerInfo is a private struct that's inaccessible in TagInfo<WorkerInfo::...> definitions
    //Metrics::TaggedPolledGauge<WorkerInfo::WorkerLayer, WorkerInfo::WorkerState> mWorkersActive; // PACKER_TODO:  is this more for diagnotics?
    Metrics::TaggedGauge<GameManager::ScenarioName> mScenariosActive;
    Metrics::TaggedCounter<GameManager::ScenarioName> mScenariosStarted;
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::PackerScenarioData::TerminationReason, uint32_t> mScenariosCompleted;
    Metrics::TaggedGauge<GameManager::ScenarioName, GameManager::SubSessionName> mSessionsActive;
    Metrics::TaggedGauge<GameManager::ScenarioName, GameManager::SubSessionName> mUsersActive;
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::SubSessionName> mSessionsStarted;
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::SubSessionName, GameManager::PackerSessionData::SessionResult, uint32_t> mSessionsCompleted;
};

} // GamePacker
} // Blaze
#endif
