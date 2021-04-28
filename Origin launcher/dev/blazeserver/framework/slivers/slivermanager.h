/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SLIVERMANAGER_H
#define BLAZE_SLIVERMANAGER_H

/*** Include files *******************************************************************************/
#include "framework/tdf/slivermanagertypes_server.h"
#include "framework/rpc/sliversslave_stub.h"
#include "framework/slivers/slivercoordinator.h"
#include "framework/controller/drainlistener.h"
#include "framework/controller/remoteinstancelistener.h"
#include "EASTL/vector_multimap.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class SliverCoordinator;
class SliverOwner;
class SliverListener;

typedef uint16_t SliverPartitionIndex;
const SliverPartitionIndex INVALID_SLIVER_PARTITION_INDEX = UINT16_MAX;
const int64_t SLIVER_STATE_UPDATE_TIMEOUT_MS = (5 * 1000);
const int64_t SLIVER_STATE_UPDATE_RETRY_COUNT = (3);

class SliverSyncListener
{
public:
    virtual void onSliverSyncComplete(SliverNamespace sliverNamespace, InstanceId instanceId) = 0;
    virtual ~SliverSyncListener() {}
};

class SliverManager
    :
    public SliversSlaveStub,
    protected DrainListener,
    protected DrainStatusCheckHandler,
    protected RemoteServerInstanceListener
{
    NON_COPYABLE(SliverManager);
public:
    SliverManager();
    ~SliverManager() override;

    void registerSliverOwner(SliverOwner& sliverOwner);
    void deregisterSliverOwner(SliverOwner& sliverOwner);

    SliverPtr getSliver(SliverIdentity sliverIdentity);
    SliverPtr getSliver(SliverNamespace sliverNamespace, SliverId sliverId = INVALID_SLIVER_ID);

    SliverPtr getSliverFromSliverKey(SliverNamespace sliverNamespace, SliverKey _key);
    BlazeRpcError waitForSliverAccess(SliverIdentity sliverIdentity, Sliver::AccessRef& sliverAccess, const char8_t* context, bool priorityAccess = false);

    void registerSliverListener(SliverListener& sliverListener);
    void deregisterSliverListener(SliverListener& sliverListener);

    void getFullListenerCoverage(SliverNamespace sliverNamespace, Blaze::InstanceIdList& listeners);
    InstanceId getSliverInstanceId(SliverNamespace sliverNamespace, SliverId sliverId);
    uint16_t getSliverCount(SliverNamespace sliverNamespace);

    uint16_t getSliverNamespacePartitionCount(SliverNamespace sliverNamespace) const;
    int32_t getOwnedSliverCount(SliverNamespace sliverNamespace, InstanceId instanceId);
    int32_t getOwnedSliverCount(SliverNamespace sliverNamespace);
    const SliverIdSet* getOwnedSliverIdsByInstance(SliverNamespace sliverNamespace, InstanceId instanceId);
    SliverId getAnySliverIdByInstanceId(SliverNamespace sliverNamespace, InstanceId instanceId);

    const char8_t* getSliverNamespaceStr(SliverNamespace sliverNamespace) const;

    bool isSliverNamespaceAbandoned(SliverNamespace sliverNamespace);
    bool isAutoMigrationOnShutdownEnabled(SliverNamespace sliverNamespace);

    SliverCoordinator& getCoordinator() { return mSliverCoordinator; }

    void addSliverSyncListener(SliverSyncListener& listener) { mSliverSyncListenerDispatcher.addDispatchee(listener); }
    void removeSliverSyncListener(SliverSyncListener& listener) { mSliverSyncListenerDispatcher.removeDispatchee(listener); }

    bool canTransitionToInService();

    // DrainStatusCheckHandler interface
    bool isDrainComplete() override;
    SliverId getValidSliverID(uint16_t sliverCount);


protected:
    friend class SliverNamespaceImpl;
    friend class SliverOwner;
    friend class Sliver;

    friend void intrusive_ptr_add_ref(Sliver*);
    friend void intrusive_ptr_release(Sliver*);

    bool onConfigure() override;
    bool onReconfigure() override;
    bool onPrepareForReconfigure(const SliversConfig& config) override;
    bool onResolve() override;
    bool onActivate() override;
    void onShutdown() override;

    bool onValidateConfig(SliversConfig& config, const SliversConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const override;

    void onSlaveSessionAdded(SlaveSession& session) override;
    void onSlaveSessionRemoved(SlaveSession& session) override;
    void handleInstanceRemoved(InstanceId instanceId);

    void onSliverStateUpdateRequired(const SliverStateUpdateRequest& data, UserSession*) override;
    void onSliverStateUpdateConfirmation(const SliverStateUpdateConfirmation& data, UserSession*) override;

    void onSliverListenerSynced(const SliverEventInfo& data, UserSession*) override;
    void onSliverListenerDesynced(const SliverEventInfo& data, UserSession*) override;
    void onUpdateSliverListenerPartition(const SliverListenerPartitionSyncUpdate& data, UserSession*) override;
    void onSliverCoordinatorActivated(const SliverCoordinatorInfo& data, UserSession*) override;
    void onUpdateSliverListenerPartitionCount(const SliverListenerPartitionCountUpdate& data, UserSession*) override;
    void onSliverRepairNeeded(const SliverRepairParameters& data, ::Blaze::UserSession*) override;

    void getStatusInfo(ComponentStatus& status) const override;

    // DrainListener interface
    void onControllerDrain() override;

    // RemoteServerInstanceListener interface
    void onRemoteInstanceChanged(const RemoteServerInstance& instance) override;
    void onRemoteInstanceDrainingStateChanged(const RemoteServerInstance& instance) override;

    void scheduleSliverCoordinatorUpdate();
    void updateSliverCoordinator();
    void filloutSliverNamespaceStates(StateBySliverNamespaceMap& states);

    ImportSliverError::Error processImportSliver(const SliverInfo& request, const Message*) override;
    ExportSliverError::Error processExportSliver(const SliverInfo& request, const Message*) override;
    DropSliverError::Error processDropSliver(const SliverInfo& request, const Message*) override;

    MigrateSliverError::Error processMigrateSliver(const MigrateSliverRequest& request, MigrateSliverResponse& response, const Message*) override;
    EjectSliversError::Error processEjectSlivers(const EjectSliversRequest& request, const Message*) override;
    RebalanceSliversError::Error processRebalanceSlivers(const RebalanceSliversRequest& request, const Message*) override;
    GetSliversError::Error processGetSlivers(const GetSliversRequest& request, GetSliversResponse& response, const Message*) override;
    GetSliverInfosError::Error processGetSliverInfos(const GetSliverInfosRequest& request, GetSliverInfosResponse& response, const Message*) override;
    SubmitSliverNamespaceStatesError::Error processSubmitSliverNamespaceStates(const SubmitSliverNamespaceStatesRequest& request, const Message*) override;
    CheckHealthError::Error processCheckHealth(const CheckHealthRequest& request, CheckHealthResponse& response, const Message* message) override;
    CheckLocalHealthError::Error processCheckLocalHealth(const CheckHealthRequest& request, CheckHealthResponse& response, const Message* message) override;

private:
    friend class SliverCoordinator;

    enum SliverListenerSyncState { SYNCSTATE_UNKNOWN, SYNCSTATE_COMPLETE, SYNCSTATE_SYNC, SYNCSTATE_DESYNC };
    struct SliverListenerInfo
    {
        SliverListenerInfo() : mInstanceId(INVALID_INSTANCE_ID), mSyncState(SYNCSTATE_UNKNOWN) {}
        InstanceId mInstanceId;
        SliverListenerSyncState mSyncState;
    };
    typedef eastl::vector_multimap<SliverListenerRank, SliverListenerInfo> ListenerInfoByListenerRankMap;
    struct SliverPartitionData
    {
        SliverIdSet mSliverIds;
        ListenerInfoByListenerRankMap mListenerRanks;
    };
    typedef eastl::vector<SliverPartitionData> SliverPartitionDataList;
    typedef eastl::vector_map<InstanceId, SliverIdSet> SliverIdSetByInstanceIdMap;
    typedef eastl::vector_map<SliverId, SliverPtr> SliverPtrBySliverIdMap;

    struct SliverNamespaceData;
    struct SliverNamespaceMetrics
    {
        SliverNamespaceMetrics(SliverNamespaceData& owner);

        Metrics::MetricsCollection& mCollection;
        Metrics::PolledGauge mGaugeOwnedSliverCount;
        Metrics::PolledGauge mGaugeUnownedSliverCount;
        Metrics::PolledGauge mGaugePartitionCount;
    };

    struct SliverNamespaceData
    {
        SliverNamespaceData() :
            mNamespace(Component::INVALID_COMPONENT_ID), mIsConfigured(false), mSliverOwner(nullptr),
            mSliverListener(nullptr), mCountTotalOwned(0), mDestabilizedAt(EA::TDF::TimeValue::getTimeOfDay()), mIsStable(false), mMetrics(*this) {}

        SliverNamespace mNamespace;
        SliverNamespaceInfo mInfo;
        bool mIsConfigured;

        SliverOwner* mSliverOwner;
        SliverListener* mSliverListener;

        SliverIdSetByInstanceIdMap mListenerSliverIdSets;

        SliverPartitionDataList mPartitionDataList;

        SliverPtrBySliverIdMap mSliverPtrBySliverIdMap;

        SliverIdSetByInstanceIdMap mSliverIdsByInstanceIdMap;

        int32_t mCountTotalOwned;
        EA::TDF::TimeValue mLastStableAt;
        EA::TDF::TimeValue mStabilizedAt;
        EA::TDF::TimeValue mDestabilizedAt;
        EA::TDF::TimeValue mLastLogEntryAt;
        bool mIsStable;

        SliverNamespaceMetrics mMetrics;

        void healthCheck();
        void repairUnownedSlivers();
    };

    bool validateSliverNamespace(SliverNamespace sliverNamespace);
    void updateSliverNamespaceConfig(SliverNamespaceData& namespaceData);
    void updateSliverNamespacePartitionData(SliverNamespaceData& namespaceData, uint16_t partitionCount);
    void updateSliverOwnershipIndiciesAndCounts(SliverIdentity sliverIdentity, const SliverState& prevState, const SliverState& newState);
    void periodicHealthCheck();

    SliverNamespaceData* getOrCreateSliverNamespace(SliverNamespace sliverNamespace);
    SliverPtr getSliver(SliverNamespaceData* namespaceData, SliverId sliverId);

    SliverOwner* getSliverOwner(SliverNamespace sliverNamespace);
    SliverListener* getSliverListener(SliverNamespace sliverNamespace);

    void broadcastAndConfirmUpdateSliverState(Sliver& sliver, const SliverInfo& sliverInfo);
    void scheduleRequiredSliverStateUpdate(const SliverStateUpdateRequest& updateData);
    void updateSliverState(SliverPtr sliverPtr, InstanceId sendConfirmationToInstanceId = INVALID_INSTANCE_ID);

    enum SliverListenerEventType
    {
        EVENT_TYPE_SYNCED,
        EVENT_TYPE_DESYNCED
    };
    void onSliverListenerEvent(const SliverEventInfo& eventInfo, SliverListenerEventType eventType);
    void updateSliverListenerPartitionRanks(SliverNamespaceData& namespaceData, const SliverIdSetByInstanceIdMap::iterator& listenerIt);

    SliverListenerRank getSliverListenerRank(SliverNamespace sliverNamespace, SliverPartitionIndex partitionIndex, InstanceId listenerInstanceId);

    void addText(eastl::string& line, const eastl::string& text, bool last);
    void printListenerTable(const SliverNamespaceData& namespaceData);
    void generateAsciiListenerTable(const SliverNamespaceData& namespaceData, StringBuilder& tableStr);
    void generateLocalHealthReport(StringBuilder& status);
    void resetSliverState(SliverPtr sliver, BlazeRpcError reason);

    BlazeError waitForSlaveSession(InstanceId instanceId, const char8_t* context);
    void delayedOnSlaveSessionAdded(SlaveSessionPtr slaveSession);

    void incNotificationQueueFibersCount()
    {
        ++mNotificationQueueFibersCount;
    }
    void decNotificationQueueFibersCount()
    {
        EA_ASSERT(mNotificationQueueFibersCount > 0);
        --mNotificationQueueFibersCount;
    }

    static uint16_t calcSliverBucketSize(uint16_t sliverCount, uint16_t bucketCount, int16_t bucketIndex);

private:
    typedef eastl::hash_map<SliverNamespace, SliverNamespaceData*> SliverNamespaceDataBySliverNamespaceMap;
    SliverNamespaceDataBySliverNamespaceMap mSliverNamespaceMap;

    typedef eastl::hash_set<SliverIdentity> SliverIdentitySet;
    typedef eastl::hash_map<InstanceId, SliverIdentitySet> SliverIdentitySetByInstanceId;
    SliverIdentitySetByInstanceId mSliverIdentitySetByInstanceId;

    WaitListByInstanceIdMap mSlaveWaitListMap;

    Fiber::FiberGroupId mUpdateSliverCoordinatorFiberGroupId;
    Fiber::FiberId mUpdateSliverCoordinatorFiberId;
    bool mSliverCoordinatorUpdatePending;

    int32_t mNotificationQueueFibersCount;

    SliverCoordinator mSliverCoordinator;

    Dispatcher<SliverSyncListener> mSliverSyncListenerDispatcher;

    FiberJobQueue mUpdateSliverStateQueue;

    struct PendingUpdateConfirmation
    {
        SliverRevisionId requiredRevisionId;
        Component::InstanceIdSet pendingInstanceIds;
        Fiber::EventHandle eventHandle;

        PendingUpdateConfirmation() : requiredRevisionId(INVALID_SLIVER_REVISION_ID) {}

        bool removePendingInstanceId(InstanceId instanceId)
        {
            bool erased = (pendingInstanceIds.erase(instanceId) > 0);
            if (pendingInstanceIds.empty() && eventHandle.isValid())
                Fiber::signal(eventHandle, ERR_OK);
            return erased;
        }
    };

    typedef eastl::hash_map<SliverIdentity, PendingUpdateConfirmation> PendingUpdateConfirmationBySliverIdentityMap;
    PendingUpdateConfirmationBySliverIdentityMap mPendingUpdateConfirmationMap;

    typedef eastl::hash_map<InstanceId, ConnectionId> ConnectionIdByInstanceIdMap;
    ConnectionIdByInstanceIdMap mSlaveConnectionTrackingMap;
    TimerId mPeriodicHealthCheckTimerId;
    TimerId mDelayedOnSlaveSessionAdded;
};

extern EA_THREAD_LOCAL SliverManager* gSliverManager;

#define LOG_SIDENT(sliverIdentity) gSliverManager->getSliverNamespaceStr(GetSliverNamespaceFromSliverIdentity(sliverIdentity)) << ":" << GetSliverIdFromSliverIdentity(sliverIdentity)

} // Blaze

#endif // BLAZE_SLIVERMANAGER_H
