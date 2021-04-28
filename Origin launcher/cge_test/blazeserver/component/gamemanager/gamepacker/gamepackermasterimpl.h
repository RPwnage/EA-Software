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
using GameManager::PackerSessionId;
using GameManager::PackerSiloId;
using GameManager::PackerFinalizationJobId;

using GameManager::PackerPlayerId;
using GameManager::PackerPartyId;
using GameManager::PackerGameId;

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

    struct ScenarioInfo;

    /**
     *  Tracks a packing session assigned to a given level in the layer hierarchy.
     */
    struct SessionInfo : public eastl::intrusive_list_node
    {
        // PACKER_TODO: Think about getting rid of these states?
        enum SessionState {
            IDLE,
            WORKING,
            STATE_MAX
        };
        ScenarioInfo* mParentScenario = nullptr;
        EA::TDF::tdf_ptr<GameManager::PackerSessionData> mSessionData; // PACKER_TODO: Rename to PackerSession
        EA::TDF::tdf_ptr<GameManager::StartMatchmakingInternalRequest> mRequestInfo; // PACKER_TODO: Aggressively trim this type to avoid depending on the bloated mostrosity which is the legacy mm request
        eastl::string mSessionInfoDesc; // PACKER_TODO: format descriptor as (sessId=12345/scenId=5345)
        TimerId mDelayedStartTimerId;

        SessionInfo();
        ~SessionInfo();
        EA::TDF::TimeValue getRemainingLifetime(EA::TDF::TimeValue now) const;
        PackerSessionId getPackerSessionId() const;
        GameManager::UserJoinInfo* getHostJoinInfo() const;
        const GameManager::ScenarioInfo* getScenarioInfo() const;
        const char8_t* getSubSessionName(const char8_t* unknownName = "<nosubsession>") const;
        void cancelDelayStartTimer();
        SessionInfo(const SessionInfo&) = delete;
        SessionInfo(SessionInfo&&) = default;
        SessionInfo& operator=(const SessionInfo&) = delete;
    };

    typedef eastl::intrusive_list<SessionInfo> SessionInfoList;

    /** Tracks a packing scenario instance that owns one or more sessions */
    struct ScenarioInfo
    {
        /*enum CleanupReason {
            SCENARIO_OWNER_LEFT,
            SCENARIO_EXPIRED,
            SCENARIO_CANCELLED_BY_OWNER,
            SCENARIO_ABORTED_ON_ERROR
        };*/

        uint64_t mScenarioId;
        TimerId mExpiryTimerId;
        TimerId mFinalizationTimerId;
        eastl::vector_set<PackerSessionId> mSessionByIdSet;
        EA::TDF::tdf_ptr<GameManager::PackerScenarioData> mScenarioData;
        EA::TDF::TdfStructVector<GameManager::UserJoinInfo> mUserJoinInfoList;
        EA::TDF::tdf_ptr<GameManager::UserSessionInfo> mOwnerUserSessionInfo;
        const GameManager::ScenarioInfo* getScenarioInfo() const;
        void cancelFinalizationTimer();
        void cancelExpiryTimer();
        ScenarioInfo();
        ~ScenarioInfo();
    };

    /**
     *  Tracks a worker assigned to a given level in the layer hierarchy.
     */
    struct WorkerInfo : public eastl::intrusive_list_node
    {
        //using WorkerLayer = uint32_t;
        enum WorkerState {
            IDLE = SessionInfo::IDLE,
            WORKING = SessionInfo::WORKING,
            SIGNALLED,
            BUSY,
            STATE_MAX
        };
        InstanceId mWorkerInstanceId;
        uint32_t mWorkerLayerId;
        uint32_t mWorkerSlotId;
        WorkerState mWorkerState;
        TimerId mWorkerResetStateTimerId;
        
        eastl::string mWorkerName;
        eastl::string mWorkerInstanceType; // PACKER_TODO: This will be used to support Scenario Partitioning by instance type name (see Hansoft)
        WorkerInfo();
        ~WorkerInfo();
        void startResetStateTimer();
        void cancelResetStateTimer();
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
        mutable eastl::array<uint64_t, WorkerInfo::STATE_MAX> mWorkerMetricsByState; // PACKER_TODO: this is currently unused, either add metrics or remove it!
        eastl::array<WorkerInfoList, WorkerInfo::STATE_MAX> mWorkerByState;
        eastl::array<SessionInfoList, SessionInfo::STATE_MAX> mSessionListByState; // PACKER_TODO: Change this into a priority map of sessions ala: east::array<east::multi_map<ExpiryTime,SessionInfo>,SessionInfo::STATE_MAX>
        LayerInfo();
        ~LayerInfo();
        bool hasWorkers() const;
        static uint32_t layerIndexFromLayerId(uint32_t layerId);
        static uint32_t layerIdFromLayerIndex(uint32_t layerIndex);
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
    typedef eastl::hash_map<PackerSessionId, SessionInfo> SessionInfoMap;
    typedef eastl::hash_map<GameManager::MatchmakingScenarioId, ScenarioInfo> ScenarioInfoMap;
    typedef eastl::hash_multimap<BlazeId, GameManager::MatchmakingScenarioId> ScenarioIdByBlazeIdMap;

    bool onValidateConfig(GamePackerConfig& config, const GamePackerConfig* referenceConfigPtr, ::Blaze::ConfigureValidationErrors& validationErrors) const override;
    bool onConfigure() override;
    bool onPrepareForReconfigure(const GamePackerConfig& newConfig) override;
    bool onReconfigure() override;
    bool onResolve() override;
    void onShutdown() override;
    void onSlaveSessionRemoved(SlaveSession& session) override;
    void getStatusInfo(ComponentStatus& status) const override;
    const char8_t* getWorkerNameByInstanceId(InstanceId instanceId, const char8_t* unkownName = "<noworker>") const;
    const char8_t* getSubSessionNameByPackerSessionId(PackerSessionId packerSessionId, const char8_t* unkownName = "<nosubsession>") const;
    // rpc interface
    StartPackerSessionError::Error processStartPackerSession(const Blaze::GameManager::StartPackerSessionRequest &request, Blaze::GameManager::StartPackerSessionResponse &response, Blaze::GameManager::StartPackerSessionError &error, const ::Blaze::Message* message) override;
    CancelPackerSessionError::Error processCancelPackerSession(const Blaze::GameManager::CancelPackerSessionRequest &request, const ::Blaze::Message* message) override;
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

    // UserSessionProvider interface    
    void onUserSessionExtinction(const UserSession& userSession) override;
    // DrainStatusCheckHandler interface
    bool isDrainComplete() override;
    // Storage
    void onImportStorageRecord(OwnedSliver& ownedSliver, const StorageRecordSnapshot& snapshot);
    void onExportStorageRecord(StorageRecordId recordId);
    // Internal
    void setWorkerState(WorkerInfo& worker, WorkerInfo::WorkerState state);
    void assignWorkerSession(WorkerInfo& worker, SessionInfo& sessionInfo);
    void onWorkerStateResetTimeout(InstanceId workerInstanceId, WorkerInfo::WorkerState state);
    void onScenarioExpiredTimeout(GameManager::MatchmakingScenarioId scenarioId);
    void onScenarioFinalizationTimeout(GameManager::MatchmakingScenarioId scenarioId);
    void startDelayedSession(PackerSessionId sessionId);
    void signalSessionAssignment(uint32_t layerId);
    bool wakeIdleWorker(WorkerInfo& worker);
    LayerInfo& getLayerByWorker(const WorkerInfo& worker);

    BlazeError initializeAggregateProperties(GameManager::StartMatchmakingInternalRequest& internalRequest, GameManager::MatchmakingCriteriaError& criteriaError);
    void assignSession(SessionInfo& sessionInfo);
    void abortSession(SessionInfo& sessionInfo);
    void cleanupScenario(ScenarioInfo& scenarioInfo);
    void upsertSessionData(SessionInfo& sessionInfo);
    void upsertRequestData(SessionInfo& sessionInfo);
    void upsertScenarioData(ScenarioInfo& scenarioInfo);
    EA::TDF::TimeValue getScenarioExpiryWithGracePeriod(EA::TDF::TimeValue expiry) const;

    WorkerByInstanceIdMap mWorkerByInstanceId;
    WorkerBySlotIdMap mWorkerBySlotId;
    LayerInfoList mLayerList;
    uint32_t mLayerCount = 0;
    LayerSlotVersionList mLayerSlotVersionList; // Tracks layer slot versions by layer slot index to handle conflicting slot assignments requested by multiple workers
    ScenarioInfoMap mScenarioByIdMap;
    ScenarioIdByBlazeIdMap mScenarioIdByBlazeIdMap;
    SessionInfoMap mSessionByIdMap;
    SessionInfoList mUnassignedSessionList; // TODO: change this to a map<deadline,SessionInfo> because master should process sessions in order of their priority
    StorageTable mScenarioTable;
    RedisKeyMap<GameManager::PlayerId, GameManager::GameId> mDedicatedServerOverrideMap;

    // metrics
    // PACKER_TODO: currently can't enable mWorkersActive because WorkerInfo is a private struct that's inaccessible in TagInfo<WorkerInfo::...> definitions
    //Metrics::TaggedPolledGauge<WorkerInfo::WorkerLayer, WorkerInfo::WorkerState> mWorkersActive; // PACKER_TODO:  is this more for diagnotics?
    Metrics::TaggedGauge<GameManager::ScenarioName> mScenariosActive;
    Metrics::TaggedCounter<GameManager::ScenarioName> mScenariosStarted;
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::PackerScenarioData::TerminationReason, uint32_t> mScenariosCompleted;
    Metrics::TaggedGauge<GameManager::ScenarioName, GameManager::SubSessionName> mSessionsActive;
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::SubSessionName> mSessionsStarted;
    Metrics::TaggedCounter<GameManager::ScenarioName, GameManager::SubSessionName, GameManager::PackerSessionData::SessionResult, uint32_t> mSessionsCompleted;
};

} // GamePacker
} // Blaze
#endif
