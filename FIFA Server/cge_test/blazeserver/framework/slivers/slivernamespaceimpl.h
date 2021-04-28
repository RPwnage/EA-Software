/*************************************************************************************************/
/*!
    \file
        sliversmasterimpl.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SLIVERNAMESPACEIMPL_H
#define BLAZE_SLIVERNAMESPACEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/redis/redisid.h"
#include "framework/tdf/slivermanagertypes_server.h"
#include "framework/slivers/slivermanager.h"

namespace Blaze
{

class SliverCoordinator;
class SliverNamespaceImpl;

typedef eastl::vector_set<SliverId> SliverIdSet;
typedef eastl::vector<SliverId> SliverIdVector;

struct SliverOwnerData
{
    SliverOwnerData() :
        isEjected(false)
    {}

    bool isEjected;
};

class SliverNamespaceImpl
{
    NON_COPYABLE(SliverNamespaceImpl);

    static const int64_t SLIVER_MIGRATION_TASK_DELAY_MS = 2000;

public:
    typedef eastl::vector_set<InstanceId> InstanceIdSet;

    SliverNamespaceImpl(SliverCoordinator& owner, SliverNamespace sliverNamespace);
    virtual ~SliverNamespaceImpl();

    void configure(const SliverNamespaceConfig& config);

    SliverNamespace getSliverNamespace() const { return mSliverNamespace; }
    const char8_t* getSliverNamespaceStr() const { return gSliverManager->getSliverNamespaceStr(mSliverNamespace); }

    const SliverNamespaceConfig& getConfig() const { return mConfig; }

    uint16_t getCurrentSliverPartitionCount() const { return (uint16_t)mSliverPartitions.size(); }

    bool isKnownOwnerOfSliver(SliverId sliverId, InstanceId instanceId);

    void activate();
    void deactivate();

    void updateSliverManagerInstanceState(InstanceId instanceId, SliverNamespaceStatePtr sliverNamespaceState);
    void migrateSliver(SliverId sliverId, InstanceId toInstanceId);
    void ejectAllSlivers(InstanceId fromInstanceId);
    void rebalanceSliverOwners();

protected:
    // DrainStatusCheckHandler interface
    bool isDrainComplete();

private:
    friend class SliverCoordinator;
    friend struct SortByNumberOfAssignedSlivers;

    // SliverOwner related jobs executed by the mMasterSliverStateChangeQueue fiber job queue.
    void job_enableRebalancing(bool enable);
    void job_updateSliverManagerInstanceState(InstanceId instanceId, SliverNamespaceStatePtr sliverNamespaceState);
    void job_migrateSliver(SliverId sliverId, InstanceId toInstanceId);
    void job_ejectAllSlivers(InstanceId fromInstanceId);
    void job_rebalanceSliverOwners();

    // SliverListener related methods.
    SliverPartitionIndex getMostNeededPartitionIndex();
    SliverPartitionIndex getLeastNeededPartitionIndex();
    SliverPartitionIndex findListenerPartitionIndex(InstanceId instanceId);
    SliverPartitionIndex findBestMatchPartitionIndex(const SliverPartitionInfo& partitionInfo);
    void updatePartitionCount();
    void rebalanceListeners();

    void scheduleOrUpdateMigrationTask();
    void startMigrationTaskFibers();

    void executeAssignedMigrationTasks(InstanceId instanceId, const SliverId fromSliverId, const SliverId toSliverId);

    void task_exportSliver(SliverId sliverId, InstanceId fromInstanceId, Fiber::EventHandle completionEvent);

    typedef eastl::hash_map<InstanceId, OwnedSliverState> OwnedSliverStateByInstanceIdMap;
    struct MasterSliverState
    {
        MasterSliverState() : assignedRevisionId(INVALID_SLIVER_REVISION_ID), assignedInstanceId(INVALID_INSTANCE_ID) {}

        // This *should* always just have a single entry.  However, if an ownership conflicts exists, due to
        // a desync (caused by a network or hardware failure, for example), it's possible that more than 1 instance
        // believes they currently own the same sliver.  The SliverCoordinator tries to resolve this conflict by
        // asking all known owners to export the sliver before the assigned owner imports the sliver.  This conflict
        // is resolved in executeAssignedMigrationTasks.
        OwnedSliverStateByInstanceIdMap ownerStateMap;

        // This is the latest revision number for this sliver.  Any instance claiming ownership at an earlier revision is
        // considered a false owner, and is asked to drop the sliver.
        SliverRevisionId assignedRevisionId;

        InstanceId assignedInstanceId;

        bool requiresMigrationTask()
        {
            return (ownerStateMap.size() != 1) || (ownerStateMap.begin()->first != assignedInstanceId) || (ownerStateMap.begin()->second.getRevisionId() != assignedRevisionId);
        }

        bool readyForImport()
        {
            return ownerStateMap.empty() && (assignedInstanceId != INVALID_INSTANCE_ID);
        }
    };
    MasterSliverState& getMasterSliverState(SliverId sliverId);

    typedef eastl::hash_map<InstanceId, SliverIdSet> SliverIdSetByInstanceIdMap;
    void buildSliverIdSetByInstanceIdMap(SliverIdSetByInstanceIdMap& output);
    void commitSliverIdSetByInstanceIdMap(SliverIdSetByInstanceIdMap& input);

    void masterSliverStateChangeQueueStarted();
    void masterSliverStateChangeQueueFinished();

    static EA::TDF::TimeValue getWaitForInstanceIdTimeout();

private:
    SliverCoordinator& mOwner;

    SliverNamespace mSliverNamespace;
    SliverNamespaceConfig mConfig;

    struct SliverPartition
    {
        SliverPartitionInfo mInfo;
        Component::InstanceIdSet mListeners;
    };

    typedef eastl::vector<SliverPartition> SliverPartitionList;
    SliverPartitionList mSliverPartitions;

    TimerId mPartitionCountUpdateDelayTimerId;
    InstanceIdSet mRegisteredListeners;
    InstanceIdSet mUnassignedListeners;

    typedef eastl::vector<MasterSliverState> MasterSliverStateBySliverIdList;
    MasterSliverStateBySliverIdList mMasterSliverStateBySliverIdList;

    typedef eastl::hash_map<InstanceId, SliverOwnerData> SliverOwnerDataByInstanceIdMap;
    SliverOwnerDataByInstanceIdMap mSliverOwnerDataByInstanceIdMap;

    FiberJobQueue mMasterSliverStateChangeQueue;
    FiberJobQueue mExportTaskQueue;

    Fiber::FiberGroupId mMigrationTaskFiberGroupId;
    TimerId mMigrationTaskTimerId;
    bool mExitMigrationTaskFiber;
    bool mMigrationTaskFailed;
    int32_t mActiveMigrationTaskFibers;

    bool mRebalancingEnabled;
    bool mIsRebalanceJobScheduled;
};

}

#endif // BLAZE_SLIVERNAMESPACEIMPL_H
