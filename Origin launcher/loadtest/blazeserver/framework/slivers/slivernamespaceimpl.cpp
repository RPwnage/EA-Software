/*************************************************************************************************/
/*!
    \file
        identity.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/timer.h"
#include "blazerpcerrors.h"

#include "framework/rpc/sliversslave.h"
#include "framework/slivers/slivernamespaceimpl.h"
#include "framework/slivers/slivercoordinator.h"
#include "framework/util/random.h"
#include "framework/connection/endpoint.h"
#include "framework/connection/selector.h"

namespace Blaze
{

static const double DEFAULT_WAIT_FOR_INSTANCE_TIMEOUT_FRACTION = 2.0/3.0;

SliverNamespaceImpl::SliverNamespaceImpl(SliverCoordinator& owner, SliverNamespace sliverNamespace) :
    mOwner(owner),
    mSliverNamespace(sliverNamespace),
    mPartitionCountUpdateDelayTimerId(INVALID_TIMER_ID),
    mMasterSliverStateChangeQueue("SliverNamespaceImpl::mMasterSliverStateChangeQueue"),
    mExportTaskQueue("SliverNamespaceImpl::mExportTaskQueue"),
    mMigrationTaskFiberGroupId(Fiber::allocateFiberGroupId()),
    mMigrationTaskTimerId(INVALID_TIMER_ID),
    mExitMigrationTaskFiber(false),
    mMigrationTaskFailed(false),
    mActiveMigrationTaskFibers(0),
    mRebalancingEnabled(false),
    mIsRebalanceJobScheduled(false)
{
    mMasterSliverStateChangeQueue.setQueueEventCallbacks(
        FiberJobQueue::QueueEventCb(this, &SliverNamespaceImpl::masterSliverStateChangeQueueStarted),
        FiberJobQueue::QueueEventCb(this, &SliverNamespaceImpl::masterSliverStateChangeQueueFinished));
}

SliverNamespaceImpl::~SliverNamespaceImpl()
{
}

void SliverNamespaceImpl::configure(const SliverNamespaceConfig& config)
{
    Fiber::BlockingSuppressionAutoPtr blockingSuppression;

    config.copyInto(mConfig);

    mExportTaskQueue.setMaximumWorkerFibers(config.getMaxConcurrentSliverExports());

    if (mMasterSliverStateBySliverIdList.size() != mConfig.getSliverCount())
    {
        EA_ASSERT_MSG(mMasterSliverStateBySliverIdList.empty(), "Reconfiguration of the SliverCount is not currently supported.");
        mMasterSliverStateBySliverIdList.resize(mConfig.getSliverCount());
    }

    updatePartitionCount();
}

TimeValue SliverNamespaceImpl::getWaitForInstanceIdTimeout()
{
    int64_t timeout = gController->getFrameworkConfigTdf().getConnectionManagerConfig().getRemoteInstanceRestartTimeout().getMicroSeconds();
    if (timeout == 0)
    {
        // Assuming average blazeserver is utilizing at least 50% CPU, after the blazeserver is back after n seconds, 
        // it needs n/2 seconds to process the held RPCs in full speed (100% CPU) and back to normal usage. 
        // We want the sum of rpc holding and processing time less than RPC timeout (n + n/2 < RPC_TIMEOUT), so we set the restart timeout to be 2/3 of rpc timeout.
        if (gController->getInternalEndpoint() != nullptr)
            timeout = static_cast<int64_t>(gController->getInternalEndpoint()->getCommandTimeout().getMicroSeconds() * DEFAULT_WAIT_FOR_INSTANCE_TIMEOUT_FRACTION);
        else
            timeout = (10 * 1000 * 1000); // Use 10 seconds if no other settings can be found.
    }
    return timeout;
}

SliverNamespaceImpl::MasterSliverState& SliverNamespaceImpl::getMasterSliverState(SliverId sliverId)
{
    EA_ASSERT(mMasterSliverStateBySliverIdList.size() >= sliverId);
    return mMasterSliverStateBySliverIdList[sliverId - SLIVER_ID_MIN];
}

bool SliverNamespaceImpl::isKnownOwnerOfSliver(SliverId sliverId, InstanceId instanceId)
{
    MasterSliverState& masterState = getMasterSliverState(sliverId);
    return (masterState.ownerStateMap.find(instanceId) != masterState.ownerStateMap.end());
}

void SliverNamespaceImpl::buildSliverIdSetByInstanceIdMap(SliverIdSetByInstanceIdMap& output)
{
    for (SliverId sliverId = SLIVER_ID_MIN; sliverId <= mConfig.getSliverCount(); ++sliverId)
    {
        MasterSliverState& masterState = getMasterSliverState(sliverId);
        output[masterState.assignedInstanceId].insert(sliverId);
    }
}

void SliverNamespaceImpl::activate()
{
    mMasterSliverStateChangeQueue.queueFiberJob(this, &SliverNamespaceImpl::job_enableRebalancing, true, "SliverNamespaceImpl::job_enableRebalancing - ACTIVATE");
}

void SliverNamespaceImpl::deactivate()
{
    mMasterSliverStateChangeQueue.queueFiberJob(this, &SliverNamespaceImpl::job_enableRebalancing, false, "SliverNamespaceImpl::job_enableRebalancing - DEACTIVATE");
}

void SliverNamespaceImpl::job_enableRebalancing(bool enable)
{
    mRebalancingEnabled = enable;
    job_rebalanceSliverOwners();
}

void SliverNamespaceImpl::updateSliverManagerInstanceState(InstanceId instanceId, SliverNamespaceStatePtr sliverNamespaceState)
{
    mMasterSliverStateChangeQueue.queueFiberJob(this, &SliverNamespaceImpl::job_updateSliverManagerInstanceState, instanceId, sliverNamespaceState, "SliverNamespaceImpl::job_updateSliverManagerInstanceState");

    bool checkAutoPartitionCountUpdate = false;
    InstanceIdSet::iterator listenerIt = mRegisteredListeners.find(instanceId);
    if ((listenerIt == mRegisteredListeners.end()) && (sliverNamespaceState != nullptr) && sliverNamespaceState->getSliverListenerState().getIsRegistered())
    {
        const SliverPartitionInfo& partitionInfo = sliverNamespaceState->getSliverListenerState().getSliverPartitionInfo();

        mRegisteredListeners.insert(instanceId);
        checkAutoPartitionCountUpdate = true;

        SliverPartitionIndex partitionIndex = findBestMatchPartitionIndex(partitionInfo);
        if (partitionIndex != INVALID_SLIVER_PARTITION_INDEX)
            mSliverPartitions[partitionIndex].mListeners.insert(instanceId);
        else
            mUnassignedListeners.insert(instanceId);
    }
    else if ((listenerIt != mRegisteredListeners.end()) && ((sliverNamespaceState == nullptr) || !sliverNamespaceState->getSliverListenerState().getIsRegistered()))
    {
        mRegisteredListeners.erase(instanceId);
        checkAutoPartitionCountUpdate = true;

        SliverPartitionIndex partitionIndex = findListenerPartitionIndex(instanceId);
        if (partitionIndex != INVALID_SLIVER_PARTITION_INDEX)
            mSliverPartitions[partitionIndex].mListeners.erase(instanceId);
        else
            mUnassignedListeners.erase(instanceId);
    }


    if (!mOwner.isActivating())
    {
        if (checkAutoPartitionCountUpdate && (mConfig.getPartitionCount() == 0))
        {
            if (mPartitionCountUpdateDelayTimerId != INVALID_TIMER_ID && getConfig().getPartitionCountUpdateDelayExtendOnChange())
            {
                gSelector->cancelTimer(mPartitionCountUpdateDelayTimerId);
                mPartitionCountUpdateDelayTimerId = INVALID_TIMER_ID;
            }

            if (mPartitionCountUpdateDelayTimerId == INVALID_TIMER_ID)
            {
                mPartitionCountUpdateDelayTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + getConfig().getPartitionCountUpdateDelay(),
                    this, &SliverNamespaceImpl::updatePartitionCount, "SliverNamespaceImpl::updatePartitionCount");
            }
        }
        else
        {
            rebalanceListeners();
        }
    }
}

void SliverNamespaceImpl::job_updateSliverManagerInstanceState(InstanceId instanceId, SliverNamespaceStatePtr sliverNamespaceState)
{
    EA_ASSERT(!Fiber::isFiberGroupIdInUse(mMigrationTaskFiberGroupId));

    SliverOwnerDataByInstanceIdMap::iterator ownerDataIt = mSliverOwnerDataByInstanceIdMap.find(instanceId);
    if ((ownerDataIt == mSliverOwnerDataByInstanceIdMap.end()) && (sliverNamespaceState != nullptr) && sliverNamespaceState->getSliverOwnerState().getIsRegistered())
    {
        mSliverOwnerDataByInstanceIdMap.insert(instanceId);

        OwnedSliverStateBySliverIdMap::const_iterator ownedSliverIt = sliverNamespaceState->getSliverOwnerState().getOwnedSliverStateBySliverIdMap().begin();
        OwnedSliverStateBySliverIdMap::const_iterator ownedSliverEnd = sliverNamespaceState->getSliverOwnerState().getOwnedSliverStateBySliverIdMap().end();
        for (; ownedSliverIt != ownedSliverEnd;  ++ownedSliverIt)
        {
            // The OwnedSliverStateBySliverIdMap was sent to us by 'instanceId'.  It contains a map of the sliverIds (the keys) that
            // the 'instanceId' thinks it owns.  The associated OwnedSliverState (the value or receivedState) is the current state of the sliver
            // as far as 'instanceId' is concerned.  The important part of the sliver state is the RevisionId.

            // 'sliverId' is the sliver that 'instanceId' is telling us he owns.
            SliverId sliverId = ownedSliverIt->first;

            // 'receivedState' is what 'instanceId' sent us, and is his understanding of the current state of the sliver that he is telling us he owns.
            OwnedSliverState& receivedState = *ownedSliverIt->second;

            // 'masterState' is the SliverCoordinator's authoritative *master* state of the sliver. Sliver distribution and balancing decisions
            // are made solely based on the current master state.  These decisions are NOT made here in this method, they are only made when job_rebalanceSliverOwners() is run.
            MasterSliverState& masterState = getMasterSliverState(sliverId);

            // Copy the state that was sent to us from 'instanceId' into the 'ownerStateMap', which is just a list/map of sliver states from
            // other 'instanceId's who also claim to own this same sliver.  In a healthy system, we should only receive one owning 'instanceId' per sliver.
            receivedState.copyInto(masterState.ownerStateMap[instanceId]);

            if (masterState.assignedRevisionId == INVALID_SLIVER_REVISION_ID)
            {
                // The master state has not yet been initialized.  The received values can just be accepted as-is.
                masterState.assignedInstanceId = instanceId;
                masterState.assignedRevisionId = receivedState.getRevisionId();
            }
            else if (masterState.assignedRevisionId == receivedState.getRevisionId())
            {
                // The received revisionId is the same as the one in the master state.  Generally, this scenario shouldn't happen because we're only
                // supposed to get one notification from all 'instanceId's in the cluster telling us what they own.  So, the fact that the master state
                // has an assigned revision for this sliver means something is off...
                if (masterState.assignedInstanceId != instanceId)
                {
                    // This is bad!  It means that more than one instance in the cluster think they own the same sliver at the same revision number.
                    // Things must have gotten really messed up for this to happen.
                    BLAZE_ERR_LOG(SliversSlave::LOGGING_CATEGORY, "SliverNamespaceImpl.job_updateSliverManagerInstanceState: Conflicting SliverOwners detected, current instance will re-import the sliver: "
                        "namespace=" << getSliverNamespaceStr() << ", sliverId=" << sliverId << ", "
                        "current state(instanceId=" << masterState.assignedInstanceId << ", revisionId=" << masterState.assignedRevisionId << "), "
                        "conflicting state(instanceId=" << instanceId << ", revisionId=" << receivedState.getRevisionId() << ")");

                    // We have no way of knowing which instanceId is *more* correct because mabey we were disconnected from the cluster for a while and
                    // some other coordinator took over, or some network interruption occurred that introduced some bad state, etc.  Given the uncertainty,
                    // we're just going to trust this new information, and let 'instanceId' maintain ownership.
                    masterState.assignedInstanceId = instanceId;
                    masterState.assignedRevisionId += 2;
                }
                else
                {
                    BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverNamespaceImpl.job_updateSliverManagerInstanceState: Existing SliverOwner found! "
                        "SliverId(" << sliverId << ") at RevisionId(" << masterState.assignedRevisionId << "); Assigned InstanceId=" << masterState.assignedInstanceId << ", Incoming InstanceId=" << instanceId << ". "
                        "Assigned instance will not be affected.");
                }
            }
            else if (masterState.assignedRevisionId > receivedState.getRevisionId())
            {
                BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverNamespaceImpl.job_updateSliverManagerInstanceState: Conflicting Sliver State Detected! "
                    "SliverId(" << sliverId << ") at Master RevisionId(" << masterState.assignedRevisionId << "), InstanceId=" << masterState.assignedInstanceId << "; Received RevisionId(" << receivedState.getRevisionId() << "), InstanceId(" << instanceId << "). "
                    "Master sliver state will be maintained, discarding received state.");
            }
            else if (masterState.assignedRevisionId < receivedState.getRevisionId())
            {
                BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverNamespaceImpl.job_updateSliverManagerInstanceState: Conflicting Sliver State Detected! "
                    "SliverId(" << sliverId << ") at Master RevisionId(" << masterState.assignedRevisionId << "), InstanceId=" << masterState.assignedInstanceId << "; Received RevisionId(" << receivedState.getRevisionId() << "), InstanceId(" << instanceId << "). "
                    "Received state will be maintained, discarding master state.");

                masterState.assignedInstanceId = instanceId;
                masterState.assignedRevisionId = receivedState.getRevisionId() + 2;
            }
        }
    }
    else if ((ownerDataIt != mSliverOwnerDataByInstanceIdMap.end()) && ((sliverNamespaceState == nullptr) || !sliverNamespaceState->getSliverOwnerState().getIsRegistered()))
    {
        for (MasterSliverStateBySliverIdList::iterator it = mMasterSliverStateBySliverIdList.begin(), end = mMasterSliverStateBySliverIdList.end(); it != end; ++it)
        {
            MasterSliverState& masterState = *it;
            if (masterState.assignedInstanceId == instanceId)
            {
                masterState.assignedInstanceId = INVALID_INSTANCE_ID;
                masterState.assignedRevisionId += 1;
            }

            masterState.ownerStateMap.erase(instanceId);
        }

        mSliverOwnerDataByInstanceIdMap.erase(instanceId);
    }

    // Call job_rebalanceSliverOwners() directly so that it runs in the context of this job.
    job_rebalanceSliverOwners();
}

struct SortByNumberOfAssignedSlivers
{
    SortByNumberOfAssignedSlivers(bool ascending) : mAscending(ascending) {}
    bool mAscending;

    bool operator()(const SliverIdSet* a, const SliverIdSet* b) const
    {
        if (mAscending)
            return (a->size() < b->size());
        else
            return (a->size() > b->size());
    }
};

void SliverNamespaceImpl::rebalanceSliverOwners()
{
    // There only needs to be one rebalancing job in the queue.
    if (mIsRebalanceJobScheduled)
        return;

    mIsRebalanceJobScheduled = true;
    mMasterSliverStateChangeQueue.queueFiberJob(this, &SliverNamespaceImpl::job_rebalanceSliverOwners, "SliverNamespaceImpl::job_rebalanceSliverOwners");
}

void SliverNamespaceImpl::job_rebalanceSliverOwners()
{
    EA_ASSERT(!Fiber::isFiberGroupIdInUse(mMigrationTaskFiberGroupId));

    Fiber::BlockingSuppressionAutoPtr nonBlocking;
    mIsRebalanceJobScheduled = false;

    if (!mRebalancingEnabled)
    {
        // Rebalancing is enabled by the SliverCoordinator after it is satisfied that it has a complete picture of the
        // current SliverOwners in the Blaze cluster.
        return;
    }

    SliverIdSetByInstanceIdMap assignmentMap;
    buildSliverIdSetByInstanceIdMap(assignmentMap);
    SliverIdSet& unassignedSliverIdSet = assignmentMap[INVALID_INSTANCE_ID];

    typedef eastl::vector<SliverIdSet*> AssignmentMapList;
    AssignmentMapList rebalanceTargets;
    rebalanceTargets.reserve(mSliverOwnerDataByInstanceIdMap.size());

    for (SliverOwnerDataByInstanceIdMap::iterator it = mSliverOwnerDataByInstanceIdMap.begin(), end = mSliverOwnerDataByInstanceIdMap.end(); it != end; ++it)
    {
        SliverOwnerData& sliverOwnerData = it->second;
        if (!sliverOwnerData.isEjected)
            rebalanceTargets.push_back(&assignmentMap.insert(it->first).first->second);
    }

    if (!rebalanceTargets.empty())
    {
        // First thing to do is randomize the order, otherwise the same targets will end up getting most of the 'extra' slivers
        // when handing out the excess slivers when the slivers are un-evenly balanced.  Note that, even though sort() is called below,
        // they'll maintain the randomness (caused here) of equal targets.
        eastl::random_shuffle(rebalanceTargets.begin(), rebalanceTargets.end(), Random::getRandomNumber);

        // Figure out the number of slivers we have to play with during this re-balancing.
        size_t rebalancingSliverCount = unassignedSliverIdSet.size();
        for (AssignmentMapList::iterator it = rebalanceTargets.begin(), end = rebalanceTargets.end(); it != end; ++it)
        {
            SliverIdSet& sliverIdSet = **it;
            rebalancingSliverCount += sliverIdSet.size();
        }

        // Calculate the preferred balance of slivers across the targets of this re-balancing.
        const int32_t preferredBalancedSliverCount = (int32_t)(rebalancingSliverCount / rebalanceTargets.size());

        // If we're allowed to remove slivers from there current SliverOwners, then...
        if (getConfig().getOwnedSliverMigration())
        {
            // Trim the number of assigned slivers for each SliverOwner involved in this re-balancing down to the preferredBalancedSliverCount.
            // The excessCount variable is used to spread the remainder of slivers across the SliverOwners.
            int32_t excessCount = (int32_t)(rebalancingSliverCount % rebalanceTargets.size());
            for (AssignmentMapList::iterator it = rebalanceTargets.begin(), end = rebalanceTargets.end(); it != end; ++it)
            {
                SliverIdSet& sliverIdSet = **it;

                int32_t numberOfSliversAbovePreferredCount = (int32_t)sliverIdSet.size() - preferredBalancedSliverCount;
                if ((numberOfSliversAbovePreferredCount > 0) && (excessCount > 0))
                {
                    --numberOfSliversAbovePreferredCount;
                    --excessCount;
                }

                // If this owner has more than the preferred sliver count, move the extras to the unassigned bucket.
                for (int32_t a = 0; (a < numberOfSliversAbovePreferredCount) && !sliverIdSet.empty(); ++a)
                {
                    unassignedSliverIdSet.insert(sliverIdSet.back());
                    sliverIdSet.pop_back();
                }
            }
        }

        // Now, we sort the targets involved in this re-balancing, such that the SliverOwners that will be assigned the smallest
        // number of new slivers will appear before the SliverOwners that will be assigned a greater number of slivers.
        eastl::sort(rebalanceTargets.begin(), rebalanceTargets.end(), SortByNumberOfAssignedSlivers(false));

        const size_t targetCount = rebalanceTargets.size();
        for (size_t i = 0; (i < targetCount) && !unassignedSliverIdSet.empty(); ++i)
        {
            int32_t preferredBalancedCountForRemainingOwners = (int32_t)(unassignedSliverIdSet.size() / (targetCount - i));

            // If the preferredBalancedCountForRemainingOwners is 0, then that means we're as even as we can get at this point!
            // So, break, and the following for() loop is responsible for properly spreading out the remainder.
            if (preferredBalancedCountForRemainingOwners == 0)
                break;

            SliverIdSet& sliverIdSet = *rebalanceTargets[i];

            int32_t preferredSliverCountDelta = preferredBalancedCountForRemainingOwners - (int32_t)sliverIdSet.size();
            for (int32_t a = 0; (a < preferredSliverCountDelta) && !unassignedSliverIdSet.empty(); ++a)
            {
                sliverIdSet.insert(unassignedSliverIdSet.back());
                unassignedSliverIdSet.pop_back();
            }
        }

        eastl::sort(rebalanceTargets.begin(), rebalanceTargets.end(), SortByNumberOfAssignedSlivers(true));

        // Spread the remaining slivers, if any, across the targets involved in this re-balancing.
        for (size_t i = 0; !unassignedSliverIdSet.empty(); ++i)
        {
            // Theoretically, at this point, the number of unassigned slivers should be < the number of targets.
            // And so, the mod(%) shouldn't be necessary. But, just in case, do the mod.
            SliverIdSet& sliverIdSet = *rebalanceTargets[i % rebalanceTargets.size()];

            int32_t numberOfSliversToAssign = (preferredBalancedSliverCount + (i / rebalanceTargets.size())) - (int32_t)sliverIdSet.size();
            for (int32_t a = 0; (a < numberOfSliversToAssign) && !unassignedSliverIdSet.empty(); ++a)
            {
                sliverIdSet.insert(unassignedSliverIdSet.back());
                unassignedSliverIdSet.pop_back();
            }

            // Sanity check for infinite loop
            if (i > SLIVER_ID_MAX*10)
            {
                BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "SliverNamespaceImpl.rebalanceSliverOwners: Infinite loop detected: "
                    "i=" << i << ", mUnassignedOwnerData.assignedIdList.size()=" << unassignedSliverIdSet.size() << ", rebalanceTargets.size()=" << rebalanceTargets.size());
                break;
            }
        }
    }

    // Now, commit the SliverIdSetByInstanceIdMap;
    for (SliverIdSetByInstanceIdMap::iterator instanceIt = assignmentMap.begin(), instanceEnd = assignmentMap.end(); instanceIt != instanceEnd; ++instanceIt)
    {
        SliverIdSet& sliverIdSet = instanceIt->second;
        for (SliverIdSet::iterator sliverIt = sliverIdSet.begin(), sliverEnd = sliverIdSet.end(); sliverIt != sliverEnd; ++sliverIt)
        {
            MasterSliverState& masterState = getMasterSliverState(*sliverIt);
            if (masterState.assignedInstanceId != instanceIt->first)
            {
                masterState.assignedInstanceId = instanceIt->first;
                masterState.assignedRevisionId += 2;
            }
        }
    }
}

void SliverNamespaceImpl::migrateSliver(SliverId sliverId, InstanceId toInstanceId)
{
    mMasterSliverStateChangeQueue.queueFiberJob(this, &SliverNamespaceImpl::job_migrateSliver, sliverId, toInstanceId, "SliverNamespaceImpl::job_migrateSliver");
}

void SliverNamespaceImpl::job_migrateSliver(SliverId sliverId, InstanceId toInstanceId)
{
    EA_ASSERT(!Fiber::isFiberGroupIdInUse(mMigrationTaskFiberGroupId));

    if (mSliverOwnerDataByInstanceIdMap.find(toInstanceId) != mSliverOwnerDataByInstanceIdMap.end())
    {
        MasterSliverState& masterState = getMasterSliverState(sliverId);
        if (masterState.assignedInstanceId != toInstanceId)
        {
            masterState.assignedInstanceId = toInstanceId;
            masterState.assignedRevisionId += (toInstanceId != INVALID_INSTANCE_ID ? 2 : 1);
        }
    }
}

void SliverNamespaceImpl::ejectAllSlivers(InstanceId fromInstanceId)
{
    mMasterSliverStateChangeQueue.queueFiberJob(this, &SliverNamespaceImpl::job_ejectAllSlivers, fromInstanceId, "SliverNamespaceImpl::job_ejectAllSlivers");
}

void SliverNamespaceImpl::job_ejectAllSlivers(InstanceId fromInstanceId)
{
    EA_ASSERT(!Fiber::isFiberGroupIdInUse(mMigrationTaskFiberGroupId));

    SliverOwnerDataByInstanceIdMap::iterator currentOwnerIt = mSliverOwnerDataByInstanceIdMap.find(fromInstanceId);
    if (currentOwnerIt != mSliverOwnerDataByInstanceIdMap.end())
    {
        SliverOwnerData& currentOwnerData = currentOwnerIt->second;
        currentOwnerData.isEjected = true;

        for (MasterSliverStateBySliverIdList::iterator it = mMasterSliverStateBySliverIdList.begin(), end = mMasterSliverStateBySliverIdList.end(); it != end; ++it)
        {
            MasterSliverState& masterState = *it;
            if (masterState.assignedInstanceId == fromInstanceId)
            {
                masterState.assignedInstanceId = INVALID_INSTANCE_ID;
                masterState.assignedRevisionId += 1;
            }
        }
    }

    // Call job_rebalanceSliverOwners() directly to that it runs in the context of this job.
    job_rebalanceSliverOwners();
}

void SliverNamespaceImpl::masterSliverStateChangeQueueStarted()
{
    if (mMigrationTaskTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mMigrationTaskTimerId);
        mMigrationTaskTimerId = INVALID_TIMER_ID;
    }

    mExitMigrationTaskFiber = true;

    BlazeError err;
    Timer waitTimer;
    do
    {
        err = Fiber::join(mMigrationTaskFiberGroupId, TimeValue::getTimeOfDay() + (5 * 1000 * 1000));
        if ((err != ERR_OK) && (waitTimer.getInterval() > (20 * 1000 * 2000)))
        {
            StringBuilder fiberGroupStatus;
            Fiber::dumpFiberGroupStatus(mMigrationTaskFiberGroupId, fiberGroupStatus);
            BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: New sliver state changes delayed, pending ongoing migration tasks; "
                "namespace=" << getSliverNamespaceStr() << ", delay=" << waitTimer << ", err=" << err << ", "
                "fiber status:\n" << SbFormats::PrefixAppender("    ", fiberGroupStatus.get()));
        }
    }
    while (err == ERR_TIMEOUT);
}

void SliverNamespaceImpl::masterSliverStateChangeQueueFinished()
{
    mExitMigrationTaskFiber = false;
    scheduleOrUpdateMigrationTask();
}

void SliverNamespaceImpl::scheduleOrUpdateMigrationTask()
{
    if (mMigrationTaskTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mMigrationTaskTimerId);

    mMigrationTaskTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (SLIVER_MIGRATION_TASK_DELAY_MS * 1000),
        this, &SliverNamespaceImpl::startMigrationTaskFibers, "SliverNamespaceImpl::startMigrationTaskFibers");
}

void SliverNamespaceImpl::startMigrationTaskFibers()
{
    mMigrationTaskTimerId = INVALID_TIMER_ID;

    Fiber::CreateParams params;
    params.groupId = mMigrationTaskFiberGroupId;

    gSelector->scheduleFiberCall<SliverNamespaceImpl, InstanceId>(
        this, &SliverNamespaceImpl::executeAssignedMigrationTasks, INVALID_INSTANCE_ID, SLIVER_ID_MIN, mConfig.getSliverCount(), "SliverNamespaceImpl::executeAssignedMigrationTasks - UNOWNED", params);

    uint16_t sliversPerMigrationTaskFiber = mConfig.getSliverCount();
    if (mSliverOwnerDataByInstanceIdMap.size() < mConfig.getMaxConcurrentSliverExports())
    {
        uint16_t maxExports = eastl::max<uint16_t>(mConfig.getMaxConcurrentSliverExports(), 1);
        sliversPerMigrationTaskFiber = mConfig.getSliverCount() / maxExports;
        if (mConfig.getSliverCount() % maxExports)
            sliversPerMigrationTaskFiber = sliversPerMigrationTaskFiber + 1;
    }

    for (SliverOwnerDataByInstanceIdMap::iterator it = mSliverOwnerDataByInstanceIdMap.begin(), end = mSliverOwnerDataByInstanceIdMap.end(); it != end; ++it)
    {
        for (SliverId fromSliverId = SLIVER_ID_MIN; fromSliverId <= mConfig.getSliverCount(); fromSliverId += sliversPerMigrationTaskFiber)
        {
            SliverId toSliverId = eastl::min<uint16_t>(fromSliverId + sliversPerMigrationTaskFiber - 1, mConfig.getSliverCount());
            gSelector->scheduleFiberCall<SliverNamespaceImpl, InstanceId>(
                this, &SliverNamespaceImpl::executeAssignedMigrationTasks, it->first, fromSliverId, toSliverId, "SliverNamespaceImpl::executeAssignedMigrationTasks - OWNED", params);
        }
    }
}

void SliverNamespaceImpl::executeAssignedMigrationTasks(InstanceId instanceId, const SliverId fromSliverId, const SliverId toSliverId)
{
    if (mActiveMigrationTaskFibers++ == 0)
    {
        // We are the first 'migration task' fiber to execute out of the current batch of
        // migration tasks.  So we reset the overall error state.
        mMigrationTaskFailed = false;
    }

    bool unownedSliversOnly = true;
    while (!mExitMigrationTaskFiber)
    {
        for (SliverId migratingSliverId = fromSliverId; (migratingSliverId <= toSliverId) && !mExitMigrationTaskFiber; ++migratingSliverId)
        {
            MasterSliverState& masterSliverState = getMasterSliverState(migratingSliverId);
            if (((masterSliverState.assignedInstanceId != instanceId) || !masterSliverState.requiresMigrationTask()) ||
                (unownedSliversOnly && !masterSliverState.ownerStateMap.empty()))
                continue;

            RoutingOptions targetSliver(getSliverNamespace(), migratingSliverId); // used only for logging

            OwnedSliverStateByInstanceIdMap::iterator ownerIt = masterSliverState.ownerStateMap.begin();
            while ((ownerIt != masterSliverState.ownerStateMap.end()) && !mExitMigrationTaskFiber)
            {
                Fiber::EventHandle completionEvent = Fiber::getNextEventHandle();
                mExportTaskQueue.queueFiberJob(this, &SliverNamespaceImpl::task_exportSliver,
                    migratingSliverId, ownerIt->first, completionEvent, "SliverNamespaceImpl::task_exportSliver");

                BlazeError err = Fiber::wait(completionEvent, "Wait for task_exportSliver completion event");
                if (err == ERR_OK)
                    ownerIt = masterSliverState.ownerStateMap.erase(ownerIt);
                else
                {
                    mMigrationTaskFailed = true;
                    ++ownerIt;
                }
            }

            if (masterSliverState.readyForImport() && !mExitMigrationTaskFiber)
            {
                SliverInfo sliverInfo;
                sliverInfo.setSliverNamespace(mSliverNamespace);
                sliverInfo.setSliverId(migratingSliverId);
                sliverInfo.getState().setSliverInstanceId(masterSliverState.assignedInstanceId);
                sliverInfo.getState().setRevisionId(masterSliverState.assignedRevisionId);
                sliverInfo.getState().setReason(ERR_OK);

                BlazeError err;
                Timer migrationTimer;
                RpcCallOptions opts;
                opts.routeTo.setInstanceId(masterSliverState.assignedInstanceId);
                do
                {
                    Timer waitForInstanceTimer;
                    err = gSliverManager->waitForInstanceId(masterSliverState.assignedInstanceId,
                        "waitForInstanceId() called for registered SliverOwner.", TimeValue::getTimeOfDay() + getWaitForInstanceIdTimeout());
                    if (err != ERR_OK)
                    {
                        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: Waited " << waitForInstanceTimer << " for " << opts.routeTo << " to become ACTIVE; err=" << err);
                        // Note, we just let things continue here.  The RPC below will likely fail with ERR_DISCONNECTED, or some other appropriate error.
                    }

                    BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: Calling importSliver() for " << targetSliver << " on " << opts.routeTo);

                    Timer rpcTimer;
                    err = gSliverManager->importSliver(sliverInfo, opts);
                    if (err == ERR_TIMEOUT)
                    {
                        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: Import " << targetSliver << " on " << opts.routeTo << " has taken " << migrationTimer << ". "
                            "The last importSliver() RPC returned " << err << " after " << rpcTimer << ". Will retry and continue waiting.");
                    }
                }
                while (err == ERR_TIMEOUT);

                BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: Import " << targetSliver << " on " << opts.routeTo << (err == ERR_OK ? " succeeded" : " FAILED") << " after " << migrationTimer << "; err=" << err);

                if (err != ERR_OK)
                    mMigrationTaskFailed = true;
                else
                {
                    OwnedSliverState& ownedSliverState = masterSliverState.ownerStateMap[masterSliverState.assignedInstanceId];
                    ownedSliverState.setRevisionId(masterSliverState.assignedRevisionId);
                }
            }
        }

        if (!unownedSliversOnly)
            break;
        unownedSliversOnly = false;
    }

    if ((--mActiveMigrationTaskFibers == 0) && mMigrationTaskFailed)
    {
        // We are the last 'migration task' fiber, and there was a failure.  So, we need to kick off another rebalancing.
        rebalanceSliverOwners();
    }
}

void SliverNamespaceImpl::task_exportSliver(SliverId sliverId, InstanceId fromInstanceId, Fiber::EventHandle completionEvent)
{
    RoutingOptions targetSliver(mSliverNamespace, sliverId); // used for logging

    BlazeError err = ERR_CANCELED;
    if (!mExitMigrationTaskFiber)
    {
        MasterSliverState& masterState = getMasterSliverState(sliverId);

        SliverRevisionId revisionId = masterState.assignedRevisionId;
        if (masterState.assignedInstanceId != INVALID_INSTANCE_ID)
            revisionId -= 1; // If the assignedInstanceId is valid, then the export revisionId should always be 1 less than the assignedRevisionId.

        SliverInfo sliverInfo;
        sliverInfo.setSliverNamespace(mSliverNamespace);
        sliverInfo.setSliverId(sliverId);
        sliverInfo.getState().setRevisionId(revisionId);
        sliverInfo.getState().setSliverInstanceId(INVALID_INSTANCE_ID);
        sliverInfo.getState().setReason(ERR_OK);

        Timer migrationTimer;
        RpcCallOptions opts;
        opts.routeTo.setInstanceId(fromInstanceId);
        do
        {
            Timer waitForInstanceTimer;
            err = gSliverManager->waitForInstanceId(opts.routeTo.getAsInstanceId(),
                "Export sliver pending; Waiting for instance", TimeValue::getTimeOfDay() + getWaitForInstanceIdTimeout());
            if (err != ERR_OK)
            {
                BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: Waited " << waitForInstanceTimer << " for " << opts.routeTo << " to become ACTIVE; err=" << err);
                // Note, we just let things continue here.  The RPC below will likely fail with ERR_DISCONNECTED, or some other appropriate error.
            }

            BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: Calling exportSliver() for " << targetSliver << " on " << opts.routeTo);

            Timer rpcTimer;
            err = gSliverManager->exportSliver(sliverInfo, opts);
            if (err == ERR_TIMEOUT)
            {
                BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: Export " << targetSliver << " on " << opts.routeTo << " has taken " << migrationTimer << ". "
                    "The last exportSliver() RPC returned " << err << " after " << rpcTimer << ". Will retry and continue waiting.");
            }
        }
        while (err == ERR_TIMEOUT);

        BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "Migration Task: Export " << targetSliver << " on " << opts.routeTo << (err == ERR_OK ? " succeeded" : " FAILED") << " after " << migrationTimer << "; err=" << err);
    }

    Fiber::signal(completionEvent, err);
}

void SliverNamespaceImpl::updatePartitionCount()
{
    if (mPartitionCountUpdateDelayTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mPartitionCountUpdateDelayTimerId);
        mPartitionCountUpdateDelayTimerId = INVALID_TIMER_ID;
    }

    for (SliverPartitionList::iterator it = mSliverPartitions.begin(), end = mSliverPartitions.end(); it != end; ++it)
    {
        mUnassignedListeners.insert(it->mListeners.begin(), it->mListeners.end());
    }

    uint16_t partitionCount = mConfig.getPartitionCount();
    if (partitionCount == 0)
    {
        size_t numListeners = eastl::max<size_t>(mUnassignedListeners.size(), 1);

        double shapeModifier = mConfig.getListenerMatrixShapeModifier();
        if (shapeModifier <= 0)
            partitionCount = (uint16_t)numListeners;
        else if (shapeModifier >= 10)
            partitionCount = 1;
        else
        {
            // The following is a little trigonometry to compute the best number of partitions given the configured shapeModifier.
            // The shapeModifier represents some ratio of a 90 degrees. E.g. shapeModifier = 5 = 45 degrees. That angle is then used
            // as A in triangle ABC where A=the shapeModifier angle, B=90 degrees, and side c=partition count, and the area of the
            // triangle is equal to half of the number of listeners (e.g. searchSlaves).
            double pi_2 = 3.14159265358979323846 / 2.0;
            double angleRadians = (shapeModifier / 10.0) * pi_2; // angle A
            double listenerArea_2 = (double)numListeners / 2.0; // triangle area
            // Use Heron's formula to compute triable height (see https://www.mathopenref.com/heronsformula.html)
            double listenersPerPartition = sqrt(listenerArea_2 / (sin(pi_2 - angleRadians) / (2.0 * sin(angleRadians))));
            // partitionCount is the number of listeners divided by the computed number of lsiterners per partition.
            partitionCount = (uint16_t)ceil(numListeners / round(listenersPerPartition));
        }

        partitionCount = eastl::min((uint16_t)numListeners, partitionCount);
        uint16_t minListenersPerPartition = numListeners / partitionCount;
        uint16_t numPartitionsWithExtraListener = numListeners % partitionCount;
        BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverNamespaceImpl.updatePartitionCount: based on " << mUnassignedListeners.size()
            << " listener(s) and a matrix modifier of " << shapeModifier << ", broadcast computed SliverNamespace(" << gSliverManager->getSliverNamespaceStr(getSliverNamespace()) << ") partitionCount (" << partitionCount << ")"
            << " resulting in at least " << minListenersPerPartition << " listeners per partition, and " << numPartitionsWithExtraListener << " partitions with 1 extra listener.");

        // send notification to sliver managers to update their partition data
        SliverListenerPartitionCountUpdate update;
        update.setSliverNamespace(mSliverNamespace);
        update.setPartitionCount(partitionCount);
        gSliverManager->sendUpdateSliverListenerPartitionCountToAllSlaves(&update, true);
    }
    else
    {
        BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverNamespaceImpl.updatePartitionCount: SliverNamespace(" << gSliverManager->getSliverNamespaceStr(getSliverNamespace())
            << ") partitionCount configured to " << partitionCount);
    }

    mSliverPartitions.clear();
    mSliverPartitions.resize(partitionCount);

    SliverId nextLowerBound = SLIVER_ID_MIN;
    for (SliverPartitionIndex partitionIndex = 0; partitionIndex < partitionCount; ++partitionIndex)
    {
        SliverPartition& partition = mSliverPartitions[partitionIndex];

        partition.mInfo.setLowerBoundSliverId(nextLowerBound);
        nextLowerBound += SliverManager::calcSliverBucketSize(mConfig.getSliverCount(), partitionCount, partitionIndex);
        partition.mInfo.setUpperBoundSliverId(nextLowerBound - 1);
    }

    rebalanceListeners();
}

SliverPartitionIndex SliverNamespaceImpl::getMostNeededPartitionIndex()
{
    SliverPartitionIndex result = INVALID_SLIVER_PARTITION_INDEX;

    for (SliverPartitionIndex partitionIndex = 0; partitionIndex < mSliverPartitions.size(); ++partitionIndex)
    {
        if ((result == INVALID_SLIVER_PARTITION_INDEX) || (mSliverPartitions[partitionIndex].mListeners.size() < mSliverPartitions[result].mListeners.size()))
        {
            result = partitionIndex;
        }
    }

    return result;
}

SliverPartitionIndex SliverNamespaceImpl::getLeastNeededPartitionIndex()
{
    SliverPartitionIndex result = INVALID_SLIVER_PARTITION_INDEX;

    if (!mUnassignedListeners.empty())
        return result;

    for (SliverPartitionIndex partitionIndex = 0; partitionIndex < mSliverPartitions.size(); ++partitionIndex)
    {
        if ((result == INVALID_SLIVER_PARTITION_INDEX) || (mSliverPartitions[partitionIndex].mListeners.size() > mSliverPartitions[result].mListeners.size()))
        {
            result = partitionIndex;
        }
    }

    return result;
}

SliverPartitionIndex SliverNamespaceImpl::findListenerPartitionIndex(InstanceId instanceId)
{
    SliverPartitionIndex result = INVALID_SLIVER_PARTITION_INDEX;

    Component::InstanceIdSet::iterator it = mUnassignedListeners.find(instanceId);
    if (it == mUnassignedListeners.end())
    {
        for (SliverPartitionIndex partitionIndex = 0; partitionIndex < mSliverPartitions.size(); ++partitionIndex)
        {
            Component::InstanceIdSet& listeners = mSliverPartitions[partitionIndex].mListeners;
            it = listeners.find(instanceId);
            if (it != listeners.end())
            {
                result = partitionIndex;
                break;
            }
        }
    }

    return result;
}

SliverPartitionIndex SliverNamespaceImpl::findBestMatchPartitionIndex(const SliverPartitionInfo& partitionInfo)
{
    SliverPartitionIndex bestMatchIndex = INVALID_SLIVER_PARTITION_INDEX;
    uint32_t bestMatchCount = 0;

    for (SliverPartitionIndex partitionIndex = 0; partitionIndex < mSliverPartitions.size(); ++partitionIndex)
    {
        SliverPartitionInfo& existingInfo = mSliverPartitions[partitionIndex].mInfo;

        SliverId lbound = eastl::max(partitionInfo.getLowerBoundSliverId(), existingInfo.getLowerBoundSliverId());
        SliverId ubound = eastl::min(partitionInfo.getUpperBoundSliverId(), existingInfo.getUpperBoundSliverId());

        uint32_t matchCount = 0;
        if (lbound <= ubound)
            matchCount = (ubound - lbound) + 1;

        if (matchCount > bestMatchCount)
        {
            bestMatchCount = matchCount;
            bestMatchIndex = partitionIndex;
        }
    }

    return bestMatchIndex;
}

void SliverNamespaceImpl::rebalanceListeners()
{
    for (;;)
    {
        SliverPartitionIndex mostNeededIndex = getMostNeededPartitionIndex();
        if (mostNeededIndex != INVALID_SLIVER_PARTITION_INDEX)
        {
            Component::InstanceIdSet* mostNeededListeners = &mSliverPartitions[mostNeededIndex].mListeners;
            Component::InstanceIdSet* leastNeededListeners = nullptr;

            InstanceId reassignInstanceId = INVALID_INSTANCE_ID;

            SliverPartitionIndex leastNeededIndex = getLeastNeededPartitionIndex();
            if (leastNeededIndex != INVALID_SLIVER_PARTITION_INDEX)
            {
                if (mSliverPartitions[leastNeededIndex].mListeners.size() > mSliverPartitions[mostNeededIndex].mListeners.size() + 1)
                {
                    leastNeededListeners = &mSliverPartitions[leastNeededIndex].mListeners;

                    Component::InstanceIdList worstRankedListeners;
                    worstRankedListeners.reserve(leastNeededListeners->size());

                    uint16_t worstRank = 0;
                    for (Component::InstanceIdSet::iterator it = leastNeededListeners->begin(), end = leastNeededListeners->end(); it != end; ++it)
                    {
                        uint16_t rank = gSliverManager->getSliverListenerRank(mSliverNamespace, leastNeededIndex, *it);
                        if (rank > worstRank)
                        {
                            worstRank = rank;
                            worstRankedListeners.clear();
                        }

                        if (rank == worstRank)
                            worstRankedListeners.push_back(*it);
                    }

                    if (!worstRankedListeners.empty())
                        reassignInstanceId = worstRankedListeners[gRandom->getRandomNumber(worstRankedListeners.size())];
                }
            }
            else if (!mUnassignedListeners.empty())
            {
                leastNeededListeners = &mUnassignedListeners;

                Component::InstanceIdList bestRankedListeners;
                bestRankedListeners.reserve(leastNeededListeners->size());

                uint16_t bestRank = UINT16_MAX;
                for (Component::InstanceIdSet::iterator it = leastNeededListeners->begin(), end = leastNeededListeners->end(); it != end; ++it)
                {
                    uint16_t rank = gSliverManager->getSliverListenerRank(mSliverNamespace, mostNeededIndex, *it);
                    if (rank < bestRank)
                    {
                        bestRank = rank;
                        bestRankedListeners.clear();
                    }

                    if (rank == bestRank)
                        bestRankedListeners.push_back(*it);
                }

                if (!bestRankedListeners.empty())
                    reassignInstanceId = bestRankedListeners[gRandom->getRandomNumber(bestRankedListeners.size())];
            }

            if (reassignInstanceId != INVALID_INSTANCE_ID)
            {
                leastNeededListeners->erase(reassignInstanceId);

                mostNeededListeners->insert(reassignInstanceId);

                SliverListenerPartitionSyncUpdate update;
                update.setSliverNamespace(mSliverNamespace);

                mSliverPartitions[mostNeededIndex].mInfo.copyInto(update.getSliverPartitionInfo());

                gSliverManager->sendUpdateSliverListenerPartitionToInstanceById(reassignInstanceId, &update);

                continue;
            }
        }

        break;
    }
}

bool SliverNamespaceImpl::isDrainComplete()
{
    return !Fiber::isFiberGroupIdInUse(mMigrationTaskFiberGroupId) && !mMasterSliverStateChangeQueue.hasPendingWork();
}

} //namespace Blaze
