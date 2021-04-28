/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/outboundrpcconnection.h"
#include "framework/connection/selector.h"
#include "framework/slivers/slivermanager.h"
#include "framework/slivers/sliverowner.h"
#include "framework/slivers/sliverlistener.h"
#include "framework/slivers/slivercoordinator.h"
#include "framework/controller/controller.h"
#include "framework/util/random.h"
#include "framework/util/timer.h"

#include "EASTL/algorithm.h"

namespace Blaze
{

// static
SliversSlave* SliversSlave::createImpl()
{
    return BLAZE_NEW_MGID(SliverManager::COMPONENT_MEMORY_GROUP, "Component") SliverManager();
}

SliverManager::SliverNamespaceMetrics::SliverNamespaceMetrics(SliverNamespaceData& owner)
    : mCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::sliver_namespace, gSliverManager->getSliverNamespaceStr(owner.mNamespace)))
    , mGaugeOwnedSliverCount(mCollection, "slivers.ownedSliverCount", [&owner]() { return owner.mCountTotalOwned; })
    , mGaugeUnownedSliverCount(mCollection, "slivers.unownedSliverCount", [&owner]() { return owner.mInfo.getConfig().getSliverCount() - owner.mCountTotalOwned; })
    , mGaugePartitionCount(mCollection, "slivers.partitionCount", [&owner]() { return owner.mPartitionDataList.size(); })
{
}

SliverManager::SliverManager() :
    mUpdateSliverCoordinatorFiberGroupId(Fiber::INVALID_FIBER_GROUP_ID),
    mUpdateSliverCoordinatorFiberId(Fiber::INVALID_FIBER_ID),
    mSliverCoordinatorUpdatePending(false),
    mNotificationQueueFibersCount(0),
    mUpdateSliverStateQueue("SliverManager::mUpdateSliverStateQueue"),
    mPeriodicHealthCheckTimerId(INVALID_TIMER_ID),
    mDelayedOnSlaveSessionAdded(INVALID_TIMER_ID)
{
    EA_ASSERT(gSliverManager == nullptr);
    gSliverManager = this;
    mUpdateSliverStateQueue.setMaximumWorkerFibers(100);
}

SliverManager::~SliverManager()
{
    EA_ASSERT(gSliverManager != nullptr);
    gSliverManager = nullptr;
}

bool SliverManager::onConfigure()
{
    mUpdateSliverCoordinatorFiberGroupId = Fiber::allocateFiberGroupId();
    return true;
}

bool SliverManager::onReconfigure()
{
    for (SliverNamespaceDataBySliverNamespaceMap::iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData& namespaceData = *it->second;
        updateSliverNamespaceConfig(namespaceData);
    }

    mSliverCoordinator.reconfigure();

    return true;
}

bool SliverManager::onPrepareForReconfigure(const SliversConfig& config)
{
    return true;
}

bool SliverManager::onResolve()
{
    return true;
}

bool SliverManager::onActivate()
{
    gController->addRemoteInstanceListener(*this);
    gController->addDrainListener(*this);
    gController->registerDrainStatusCheckHandler(SliversSlave::COMPONENT_INFO.name, *this);

    mSliverCoordinator.activate();

    periodicHealthCheck();

    return true;
}

void SliverManager::onShutdown()
{
    mUpdateSliverStateQueue.join();

    gSelector->cancelTimer(mDelayedOnSlaveSessionAdded);
    mDelayedOnSlaveSessionAdded = INVALID_TIMER_ID;

    gSelector->cancelTimer(mPeriodicHealthCheckTimerId);
    mPeriodicHealthCheckTimerId = INVALID_TIMER_ID;

    gController->removeRemoteInstanceListener(*this);
    gController->removeDrainListener(*this);
    gController->deregisterDrainStatusCheckHandler(SliversSlave::COMPONENT_INFO.name);

    for (WaitListByInstanceIdMap::iterator it = mSlaveWaitListMap.begin(), end = mSlaveWaitListMap.end(); it != end; ++it)
    {
        it->second.signal(ERR_CANCELED);
    }

    for (SliverNamespaceDataBySliverNamespaceMap::iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData*& namespaceData = it->second;
        for (SliverPtrBySliverIdMap::iterator sliverIt = namespaceData->mSliverPtrBySliverIdMap.begin(), sliverEnd = namespaceData->mSliverPtrBySliverIdMap.end(); sliverIt != sliverEnd; ++sliverIt)
        {
            if (sliverIt->second->mNotificationQueue.hasPendingWork())
            {
                BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.onShutdown: Sliver with SliverId(" << sliverIt->second->getSliverId() << ") still has pending work in the NotificationQueue. Cancelling all jobs.");
            }
            sliverIt->second->mNotificationQueue.cancelAllWork();
        }

        if (namespaceData->mSliverOwner != nullptr)
            deregisterSliverOwner(*namespaceData->mSliverOwner);

        if (namespaceData->mSliverListener != nullptr)
            deregisterSliverListener(*namespaceData->mSliverListener);

        delete namespaceData;
        namespaceData = nullptr;
    }

    // Clean up the updateSliverCoordinator() fiber, if it's running.
    if (Fiber::isFiberIdInUse(mUpdateSliverCoordinatorFiberId))
    {
        Fiber::join(mUpdateSliverCoordinatorFiberGroupId);
    }

    mSliverCoordinator.shutdown();
}

bool SliverManager::onValidateConfig(SliversConfig& config, const SliversConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    Blaze::SliverNamespaceConfigMap& namespaceMap = config.getSliverNamespaces();
    Blaze::SliverNamespaceConfigMap::iterator it = namespaceMap.begin();

    while(it != namespaceMap.end())
    {
        if (it->second->getSliverCount() > SLIVER_ID_MAX)
        {
            eastl::string msg;
            msg.sprintf("[SliverManager].onValidateConfig: sliverCount for namespace %s is %hu. sliverCount cannot be set above %hu", it->first.c_str() , it->second->getSliverCount() , SLIVER_ID_MAX);
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
        }
        it++;
    }

    return validationErrors.getErrorMessages().empty();
}

void SliverManager::onSlaveSessionAdded(SlaveSession& session)
{
    mSliverCoordinator.handleSlaveSessionAdded(session.getInstanceId());

    // When a SlaveSession is added, we want to tell it all about the *things* we've already told other
    // SlaveSessions while we've been running.

    for (SliverNamespaceDataBySliverNamespaceMap::iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData& data = *it->second;

        if (data.mSliverOwner != nullptr)
            data.mSliverOwner->syncSibling(session);

        if (data.mSliverListener != nullptr)
            data.mSliverListener->syncSibling(session);
    }

    WaitListByInstanceIdMap::iterator it = mSlaveWaitListMap.find(session.getInstanceId());
    if (it != mSlaveWaitListMap.end())
        it->second.signal(ERR_OK);
}

void SliverManager::onSlaveSessionRemoved(SlaveSession& session)
{
    handleInstanceRemoved(session.getInstanceId());
}

void SliverManager::handleInstanceRemoved(InstanceId instanceId)
{
    mSliverCoordinator.handleSlaveSessionRemoved(instanceId);

    PendingUpdateConfirmationBySliverIdentityMap::iterator confIt = mPendingUpdateConfirmationMap.begin();
    while (confIt != mPendingUpdateConfirmationMap.end())
    {
        PendingUpdateConfirmation& confVal = confIt->second;
        if (confVal.removePendingInstanceId(instanceId))
        {
            // When the above call returns true, it's possible that the mPendingUpdateConfirmationMap mutated.
            // Therefore, we need to reset confIt and scan again.
            confIt = mPendingUpdateConfirmationMap.begin();
            continue;
        }

        ++confIt;
    }

    for (SliverNamespaceDataBySliverNamespaceMap::const_iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        EA_ASSERT(it->second != nullptr);
        SliverNamespaceData& namespaceData = *it->second;

        SliverIdSetByInstanceIdMap::iterator listenerIt = namespaceData.mListenerSliverIdSets.find(instanceId);
        if (listenerIt != namespaceData.mListenerSliverIdSets.end())
        {
            listenerIt->second.clear();
            updateSliverListenerPartitionRanks(namespaceData, listenerIt);
            namespaceData.mListenerSliverIdSets.erase(listenerIt);
        }
    }

    SliverIdentitySetByInstanceId::iterator identIt = mSliverIdentitySetByInstanceId.find(instanceId);
    if (identIt != mSliverIdentitySetByInstanceId.end())
    {
        SliverIdentitySet& sliverIdentitySet = identIt->second;
        for (SliverIdentitySet::iterator it = sliverIdentitySet.begin(), end = sliverIdentitySet.end(); it != end; ++it)
        {
            SliverPtr sliver = getSliver(*it);
            if ((sliver != nullptr) && (sliver->getPendingStateUpdate().getSliverInstanceId() == instanceId))
            {
                resetSliverState(sliver, ERR_DISCONNECTED);
            }
        }
    }
}

void SliverManager::onControllerDrain()
{
    if (gController->getState() == State::ACTIVE)
    {
        for (SliverNamespaceDataBySliverNamespaceMap::iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
        {
            SliverNamespaceData& data = *it->second;
            if (data.mInfo.getConfig().getAutoMigrationOnShutdown())
            {
                if (data.mSliverOwner != nullptr)
                {
                    SliverOwner& owner = *data.mSliverOwner;
                    owner.beginDraining();
                }
            }
            else
            {
                BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager::onControllerDrain: Auto sliver migration on shutdown disabled "
                    "for SliverNamespace(" << gSliverManager->getSliverNamespaceStr(data.mNamespace) << ").");
            }
        }
    }
    else
    {
        // We're being told to drain before we've even gone active.  This normally happens if a component failed to configure
        // itself, and the bootstrap process failed, causing the controller to begin shutting down.  When this happens, there's
        // no hope of opening our inbound rpc endpoints - meaning we will never get any SlaveSessions. So we need to wakeup
        // anyone waiting to send a notification to a remote slave session.
        for (WaitListByInstanceIdMap::iterator it = mSlaveWaitListMap.begin(), end = mSlaveWaitListMap.end(); it != end; ++it)
        {
            it->second.signal(ERR_CANCELED);
        }
    }

}

bool SliverManager::isDrainComplete()
{
    bool complete = true;

    for (SliverNamespaceDataBySliverNamespaceMap::const_iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData& data = *it->second;
        if (data.mSliverOwner != nullptr)
        {
            SliverOwner& owner = *data.mSliverOwner;
            if ((!owner.mOwnedSliverBySliverIdMap.empty() && data.mInfo.getConfig().getAutoMigrationOnShutdown()) || // we still own some slivers, or
                (owner.getOwnedObjectCount() > 0) ||                    // there are still objects pinned to slivers owned by this instance, or
                (owner.mMetrics.mGaugeActiveImportOperations.get() != 0) ||   // there are still ongoing import operations, or
                (owner.mMetrics.mGaugeActiveExportOperations.get() != 0) ||   // there are still ongoing export operations, or
                (owner.mHandleDrainRequestTimerId != INVALID_TIMER_ID)) // there is still a pending calls to the SliverCoordinator.
                
            {
                complete = false;
                BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.isDrainComplete: SliverOwner(" << getSliverNamespaceStr(owner.getSliverNamespace()) << ") not drained: "
                    "Owned object count(" << owner.getOwnedObjectCount() << "), "
                    "OwnedSliver count(" << owner.getOwnedSliverCount() << "), "
                    "Active imports(" << owner.mMetrics.mGaugeActiveImportOperations.get() << "), "
                    "Active exports(" << owner.mMetrics.mGaugeActiveExportOperations.get() << "), "
                    "SliverManager::ejectSlivers() call is " << (owner.mHandleDrainRequestTimerId != INVALID_TIMER_ID ? "" : "not ") << "pending.");
            }
        }
    }

    if (mNotificationQueueFibersCount > 0)
    {
        complete = false;
        BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.isDrainComplete: Not drained: NotificationQueueFibersCount(" << mNotificationQueueFibersCount << ")");
    }

    if (mUpdateSliverStateQueue.hasPendingWork())
    {
        complete = false;
        BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.isDrainComplete: The UpdateSliverStateQueue still has (" << mUpdateSliverStateQueue.getJobQueueSize() << ") jobs remaining, or 1 job currently being processed.");
    }

    if (!mSliverCoordinator.isDrainComplete())
        complete = false;

    return complete;
}

void SliverManager::onRemoteInstanceChanged(const RemoteServerInstance& instance)
{
    // Left in-place commented out for now.
    // InstanceId instanceId = instance.getInstanceId();
    // const RemoteServerInstance* remoteInstance = &instance;
    // const SlaveSession* slaveSession = mSlaveList.getSlaveSessionByInstanceId(instanceId);
    // BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.onRemoteInstanceChanged:\n"
    //     "InstanceId: " << instanceId << "\n"
    //     "remoteInstance: " << (remoteInstance == nullptr ? "nullptr" : (remoteInstance->isConnected() ? "Connected" : "Disconnected")) << "\n"
    //     "slaveSession: " << (slaveSession == nullptr ? "nullptr" : (slaveSession->getConnection() == nullptr ? "conn=nullptr" : (slaveSession->getConnection()->isConnected() ? "Connected" : "Disconnected"))));

    // Check if all instances are draining (a.k.a A full cluster shutdown.
    if (gController->isDraining() && (getComponentInstanceCount(false) == 0))
    {
        for (SliverNamespaceDataBySliverNamespaceMap::iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
        {
            SliverNamespaceData*& namespaceData = it->second;
            for (SliverPtrBySliverIdMap::iterator sliverIt = namespaceData->mSliverPtrBySliverIdMap.begin(), sliverEnd = namespaceData->mSliverPtrBySliverIdMap.end(); sliverIt != sliverEnd; ++sliverIt)
            {
                SliverPtr& sliver = sliverIt->second;
                sliver->mWaitingForOwnerList.signal(ERR_TIMEOUT);
            }
        }
    }

    // If we're no longer connected to this remote instance, AND we don't yet have a SlaveSession for him
    // then we can't rely on the onSlaveSessionRemoved() callback to trigger all the cleanup.  So,
    // we do it here.
    SlaveSession* slaveSession = mSlaveList.getSlaveSessionByInstanceId(instance.getInstanceId());
    if (!instance.isConnected())
    {
        ConnectionIdByInstanceIdMap::insert_return_type inserted = mSlaveConnectionTrackingMap.insert(eastl::make_pair(instance.getInstanceId(), UINT32_MAX /*Connection::INVALID_IDENT*/));
        if (slaveSession == nullptr)
            handleInstanceRemoved(instance.getInstanceId());
        else if ((inserted.second) && (slaveSession->getConnection() != nullptr))
            inserted.first->second = slaveSession->getConnection()->getIdent();
    }
    else
    {
        ConnectionIdByInstanceIdMap::iterator foundIt = mSlaveConnectionTrackingMap.find(instance.getInstanceId());
        if (foundIt != mSlaveConnectionTrackingMap.end())
        {
            if (((slaveSession != nullptr) && (slaveSession->getConnection() != nullptr)) && (foundIt->second == slaveSession->getConnection()->getIdent()))
            {
                // We delay the call to onSlaveSessionAdded() here because calling it directly can trigger
                // network I/O, which can wake up the Connection::writeLoop() fiber, which can (unfortunately) trigger
                // other unexpected network related callbacks.
                mDelayedOnSlaveSessionAdded = gSelector->scheduleFiberTimerCall<SliverManager, SlaveSessionPtr>(TimeValue::getTimeOfDay(),
                    this, &SliverManager::delayedOnSlaveSessionAdded, slaveSession, "SliverManager::delayedOnSlaveSessionAdded");
            }
        }
        mSlaveConnectionTrackingMap.erase(instance.getInstanceId());
    }
}

void SliverManager::delayedOnSlaveSessionAdded(SlaveSessionPtr slaveSession)
{
    onSlaveSessionAdded(*slaveSession);
}

void SliverManager::onRemoteInstanceDrainingStateChanged(const RemoteServerInstance& instance)
{
    onRemoteInstanceChanged(instance);
}

BlazeError SliverManager::waitForSlaveSession(InstanceId instanceId, const char8_t* context)
{
    // There are a couple of conditions we need to check before we even consider returning ERR_OK.
    // 1. If the controller entered an error state, or we're already shutting down, then we can't enter a wait because 
    if ((gController->getState() == State::ERROR_STATE) || gController->isShuttingDown())
        return ERR_CANCELED;

    // 2. If the controller hasn't yet gone ACTIVE, but we're already draining, then we can't enter a wait.
    if ((gController->getState() != State::ACTIVE) && gController->isDraining())
        return ERR_CANCELED;

    // If the specified slave instance is already connected, then we don't need to enter a wait
    if (mSlaveList.getSlaveSessionByInstanceId(instanceId) != nullptr)
        return ERR_OK;

    return mSlaveWaitListMap[instanceId].wait(context);
}

void SliverManager::updateSliverState(SliverPtr sliverPtr, InstanceId sendConfirmationToInstanceId)
{
    Sliver& sliver = *sliverPtr;

    BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(sliver) << "->updateSliverState: State update requested; " << LogCurrentState(sliver) << ", " << LogPendingState(sliver) << " [" << Fiber::getCurrentContext() << "]");

    if (sliver.getPendingStateUpdate().getReason() != ERR_OK)
    {
        // Signal any fibers that may be waiting for the InstanceId that previously owned the sliver.
        WaitListByInstanceIdMap::iterator it = mSlaveWaitListMap.find(sliver.getSliverInstanceId());
        if (it != mSlaveWaitListMap.end())
            it->second.signal(sliver.getPendingStateUpdate().getReason());
    }

    BlazeRpcError err = sliver.updateStateToPendingState();
    if (err == ERR_OK)
    {
        SliverListener* sliverListener = getSliverListener(sliver.getSliverNamespace());
        if (sliverListener != nullptr)
            sliverListener->notifySliverInstanceIdChanged(sliver.getSliverId());
    }

    if (sendConfirmationToInstanceId != INVALID_INSTANCE_ID)
    {
        SliverStateUpdateConfirmation confirm;
        confirm.setSourceInstanceId(getLocalInstanceId());
        confirm.setSliverNamespace(sliver.getSliverNamespace());
        confirm.setSliverId(sliver.getSliverId());
        confirm.setRevisionId(sliver.getState().getRevisionId());

        sendSliverStateUpdateConfirmationToInstanceById(sendConfirmationToInstanceId, &confirm);

        BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(sliver) << "->updateSliverState: Sent sliver state update confirmation to " << RoutingOptions(sendConfirmationToInstanceId) << "; " << LogCurrentState(sliver) << ", " << LogPendingState(sliver));
    }
}

void SliverManager::scheduleRequiredSliverStateUpdate(const SliverStateUpdateRequest& updateData)
{
    SliverPtr sliverPtr = getSliver(updateData.getSliverInfo().getSliverNamespace(), updateData.getSliverInfo().getSliverId());
    if (sliverPtr != nullptr)
    {
        Sliver& sliver = *sliverPtr;
        const SliverState& requested = updateData.getSliverInfo().getState();
        const SliverState& current = sliver.getState();
        SliverState& pending = sliver.getPendingStateUpdate();

        if (requested.getReason() != ERR_OK)
        {
            // Always accept a state update when the reason is an error.  These kinds of state updates should only come from the local
            // instance or from the coordinator.  In either case, this state change request is interpreted as a command coming from an
            // authoritative source.  E.g. The local instance knows what's going on locally, and the coordinator is in charge of the namespace.
            BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(sliver) << "->scheduleRequiredSliverStateUpdate: Forcing state update due to error " << BlazeError(requested.getReason()) << "; "
                << LogState(requested, "request") << "; " << LogState(current, "current") << "; " << LogState(pending, "pending"));
            requested.copyInto(pending);
        }
        else if ((requested.getSliverInstanceId() != INVALID_INSTANCE_ID) && (pending.getReason() != ERR_OK))
        {
            BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(sliver) << "->scheduleRequiredSliverStateUpdate: Forcing state update "
                << LogState(requested, "request") << "; " << LogState(current, "current") << "; " << LogState(pending, "pending"));
            requested.copyInto(pending);
        }
        else if (requested.getRevisionId() > pending.getRevisionId())
        {
            BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(sliver) << "->scheduleRequiredSliverStateUpdate: Updating pending state to "
                << LogState(requested, "request") << "; " << LogState(current, "current") << "; " << LogState(pending, "pending"));
            requested.copyInto(pending);
        }
        else if (requested.getRevisionId() < pending.getRevisionId())
        {
            BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(sliver) << "->scheduleRequiredSliverStateUpdate: Skipping requested state update to "
                << LogState(requested, "request") << "; " << LogState(current, "current") << "; " << LogState(pending, "pending"));
        }
        else
        {
            // The RevisionIds are equal, lets check to see if the SliverInstanceIds are equal at the given revision as well.
            if (pending.getSliverInstanceId() != requested.getSliverInstanceId())
            {
                // This means things are pretty unstable. Two different instances are trying to tell us they both currently own the same sliver.
                BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(sliver) << "->scheduleRequiredSliverStateUpdate: Conflicting owners detected! Discarding pending state update to "
                    << LogState(requested, "request") << "; " << LogState(current, "current") << "; " << LogState(pending, "pending"));
            }
        }

        if (sliver.mAccessRWMutex.isWriteLockOwnedByCurrentFiberStack())
            updateSliverState(&sliver, updateData.getSendConfirmationToInstanceId());
        else
        {
            mUpdateSliverStateQueue.queueFiberJob<SliverManager, SliverPtr, InstanceId>
                (this, &SliverManager::updateSliverState, &sliver, updateData.getSendConfirmationToInstanceId(), "SliverManager::updateSliverState");
        }
    }
}

void SliverManager::onSliverStateUpdateRequired(const SliverStateUpdateRequest& data, UserSession*)
{
    scheduleRequiredSliverStateUpdate(data);
}

void SliverManager::onSliverStateUpdateConfirmation(const SliverStateUpdateConfirmation& data, UserSession*)
{
    PendingUpdateConfirmationBySliverIdentityMap::iterator it = mPendingUpdateConfirmationMap.find(MakeSliverIdentity(data.getSliverNamespace(), data.getSliverId()));
    if (it != mPendingUpdateConfirmationMap.end())
    {
        PendingUpdateConfirmation& pending = it->second;
        if (data.getRevisionId() >= pending.requiredRevisionId)
            pending.removePendingInstanceId(data.getSourceInstanceId());
    }
}

void SliverManager::onSliverListenerSynced(const SliverEventInfo& data, UserSession *)
{
    onSliverListenerEvent(data, EVENT_TYPE_SYNCED);
}

void SliverManager::onSliverListenerDesynced(const SliverEventInfo& data, UserSession *)
{
    onSliverListenerEvent(data, EVENT_TYPE_DESYNCED);
}

void SliverManager::onSliverListenerEvent(const SliverEventInfo& eventInfo, SliverListenerEventType eventType)
{
    SliverNamespaceData* namespaceData = getOrCreateSliverNamespace(eventInfo.getSliverNamespace());
    if (namespaceData != nullptr)
    {
        SliverIdSetByInstanceIdMap::insert_return_type insertRet = namespaceData->mListenerSliverIdSets.insert(eventInfo.getSourceInstanceId());

        SliverIdSet& syncedIds = insertRet.first->second;
        switch (eventType)
        {
        case EVENT_TYPE_SYNCED:
            syncedIds.insert(eventInfo.getSliverId());
            break;

        case EVENT_TYPE_DESYNCED:
            syncedIds.erase(eventInfo.getSliverId());
            break;
        }

        updateSliverListenerPartitionRanks(*namespaceData, insertRet.first);
    }
}

void SliverManager::updateSliverListenerPartitionRanks(SliverNamespaceData& namespaceData, const SliverIdSetByInstanceIdMap::iterator& instanceIdSliverIdsPair)
{
	const SliverIdSet& syncedIds = instanceIdSliverIdsPair->second;
	InstanceId instanceId = instanceIdSliverIdsPair->first;

    bool checkInstanceComplete = false;
    bool syncStateHasChanged = false;
    for (SliverPartitionDataList::iterator it = namespaceData.mPartitionDataList.begin(), end = namespaceData.mPartitionDataList.end(); it != end; ++it)
    {
        ListenerInfoByListenerRankMap& listenerRanks = it->mListenerRanks;
        SliverIdSet& partitionIds = it->mSliverIds;
        int64_t intersectionCount = 0;

		{
			// NOTE: This code is taken directly from the eastl::set_intersection() algorithm. However, that algorithm takes a back_inserted
			//       to append elements onto some container, which is inefficient for what is needed here.  In this case, all we need is an intersection 'count'.
			SliverIdSet::const_iterator first1 = syncedIds.begin();
			SliverIdSet::const_iterator last1 = syncedIds.end();
			SliverIdSet::const_iterator first2 = partitionIds.begin();
			SliverIdSet::const_iterator last2 = partitionIds.end();
			while((first1 != last1) && (first2 != last2))
			{
				if(*first1 < *first2)
					++first1;
				else if(*first2 < *first1)
					++first2;
				else
				{
					++intersectionCount;
					++first1;
					++first2;
				}
			}
		}

		SliverListenerSyncState prevSyncState =  SYNCSTATE_UNKNOWN;
		SliverListenerSyncState newSyncState =  SYNCSTATE_UNKNOWN;

		SliverListenerRank newRank = (SliverListenerRank)(partitionIds.size() - intersectionCount);

		bool foundExisting = false;
		bool doInsert = (intersectionCount > 0);
        for (ListenerInfoByListenerRankMap::iterator listenersIt = listenerRanks.begin(), listenersEnd = listenerRanks.end(); listenersIt != listenersEnd; ++listenersIt)
        {
            if (listenersIt->second.mInstanceId == instanceId)
            {
                foundExisting = true;
                prevSyncState = listenersIt->second.mSyncState;
                newSyncState = prevSyncState;

                SliverListenerRank oldRank = listenersIt->first;
                if (newRank != oldRank)
                {
                    if (newRank == 0)
                        newSyncState = SYNCSTATE_COMPLETE;
                    else if (newRank < oldRank)
                        newSyncState = SYNCSTATE_SYNC;
                    else if (newRank > oldRank)
                        newSyncState = (doInsert ? SYNCSTATE_DESYNC : SYNCSTATE_UNKNOWN);

                    listenerRanks.erase(listenersIt);
                }
                else
                    doInsert = false;

                break;
            }
        }

        if (doInsert)
        {
            if (!foundExisting)
                newSyncState = (newRank == 0 ? SYNCSTATE_COMPLETE : SYNCSTATE_SYNC);

            ListenerInfoByListenerRankMap::iterator listenerIt = listenerRanks.insert(newRank);
            listenerIt->second.mInstanceId = instanceId;
            listenerIt->second.mSyncState = newSyncState;
        }

        if (!syncStateHasChanged)
            syncStateHasChanged = (newSyncState != prevSyncState);

        if (foundExisting || doInsert)
        {
            if (newSyncState == SYNCSTATE_COMPLETE && prevSyncState != SYNCSTATE_COMPLETE)
            {
                // the instance has *just* completed sync'ing in (at least) one partition data
                checkInstanceComplete = true;
            }
        }
    }

    if (syncStateHasChanged)
    {
        printListenerTable(namespaceData);

        if (checkInstanceComplete)
        {
            // can we dispatch the sliver sync complete callback for the instanceId?
            SliverPartitionDataList::iterator it = namespaceData.mPartitionDataList.begin(), end = namespaceData.mPartitionDataList.end();
            for (; it != end; ++it)
            {
                ListenerInfoByListenerRankMap& listenerRanks = it->mListenerRanks;
                for (ListenerInfoByListenerRankMap::iterator listenersIt = listenerRanks.begin(), listenersEnd = listenerRanks.end(); listenersIt != listenersEnd; ++listenersIt)
                {
                    if (listenersIt->second.mInstanceId == instanceId && listenersIt->second.mSyncState != SYNCSTATE_COMPLETE)
                    {
                        // at least one partition data for this instanceId is not complete, so don't dispatch the callback
                        break;
                    }
                }
            }

            if (it == end)
            {
                mSliverSyncListenerDispatcher.dispatch<SliverNamespace, InstanceId>(&SliverSyncListener::onSliverSyncComplete, namespaceData.mNamespace, instanceId);
            }
        }
    }
}

void SliverManager::onUpdateSliverListenerPartition(const SliverListenerPartitionSyncUpdate& data, UserSession*)
{
    SliverNamespaceData* namespaceData = getOrCreateSliverNamespace(data.getSliverNamespace());
    if ((namespaceData != nullptr) && (namespaceData->mSliverListener != nullptr))
        namespaceData->mSliverListener->notifyUpdateAssignedSlivers(data.getSliverPartitionInfo());
}

void SliverManager::onSliverCoordinatorActivated(const SliverCoordinatorInfo& data, UserSession*)
{
    scheduleSliverCoordinatorUpdate();

    if (gController->isDraining())
    {
        // This will cause ejectSlivers() to be called if needed.
        onControllerDrain();
    }
}

void SliverManager::scheduleSliverCoordinatorUpdate()
{
    if (gController->isShuttingDown())
        return;

    mSliverCoordinatorUpdatePending = true;
    if (!Fiber::isFiberIdInUse(mUpdateSliverCoordinatorFiberId))
    {
        Fiber::CreateParams params;
        params.groupId = mUpdateSliverCoordinatorFiberGroupId;
        gSelector->scheduleFiberCall(this, &SliverManager::updateSliverCoordinator, "SliverManager::updateSliverCoordinator", params);
    }
}

void SliverManager::updateSliverCoordinator()
{
    EA_ASSERT_MSG(Fiber::getCurrentFiberGroupId() == mUpdateSliverCoordinatorFiberGroupId, "SliverManager::updateSliverCoordinator() should only be runing as a result of calling SliverManager::scheduleSliverCoordinatorUpdate().");
    mUpdateSliverCoordinatorFiberId = Fiber::getCurrentFiberId();

    Timer waitTimer;
    while (mSliverCoordinatorUpdatePending && !Fiber::isCurrentlyCancelled() && !gController->isShuttingDown())
    {
        mSliverCoordinatorUpdatePending = false;
        if (gController->isReadyForService())
        {
            InstanceId sliverCoordinatorInstanceId = mSliverCoordinator.fetchActiveInstanceId();
            if ((sliverCoordinatorInstanceId != gController->getInstanceId()) && mSliverCoordinator.isDesignatedCoordinator())
            {
                BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.updateSliverCoordinator: A conflict exists with the SliverCoordinator resolution. "
                    "The local instance is thought to be the active coordinator, however Redis indicates " << RoutingOptions(sliverCoordinatorInstanceId) << " is the active coordinator.");

                sliverCoordinatorInstanceId = INVALID_INSTANCE_ID;
            }

            if (sliverCoordinatorInstanceId != INVALID_INSTANCE_ID)
            {
                SubmitSliverNamespaceStatesRequest request;
                request.setInstanceId(gController->getInstanceId());

                filloutSliverNamespaceStates(request.getStateBySliverNamespaceMap());

                RpcCallOptions opts;
                opts.routeTo.setInstanceId(sliverCoordinatorInstanceId);
                BlazeError err = submitSliverNamespaceStates(request, opts);
                if (err == ERR_OK)
                {
                    BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverCoordinator: Successfully updated the SliverCoordinator with the local state.");

                    // This resets the timer, in case we need to do another coordinator update.
                    waitTimer.start();

                    // mSliverCoordinatorUpdatePending may have been reset during any of the above blocking calls.  So, continue; and the while() condition
                    // will determine if we need to provide another update.
                    continue;
                }

                BLAZE_ERR_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverCoordinator: Call to submitSliverNamespaceStates() failed; err=" << err);
            }

            BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverCoordinator: Still trying to update the SliverCoordinator after " << waitTimer);
        }

        Fiber::sleep(2 * 1000 * 1000, "SliverManager::updateSliverCoordinator");

        // We only get all the way down here if we didn't actually update the SliverCoordinator yet, so reset
        // the mSliverCoordinatorUpdatePending value to satisfy the while() condition.
        mSliverCoordinatorUpdatePending = true;
    }

    mUpdateSliverCoordinatorFiberId = Fiber::INVALID_FIBER_ID;
}

void SliverManager::filloutSliverNamespaceStates(StateBySliverNamespaceMap& states)
{
    for (SliverNamespaceDataBySliverNamespaceMap::const_iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData* namespaceData = it->second;
        if (namespaceData != nullptr)
        {
            SliverNamespaceState& state = *states.allocate_element();
            states[it->first] = &state;

            if (namespaceData->mSliverOwner != nullptr)
            {
                state.getSliverOwnerState().setIsRegistered(true);

                SliverOwner::OwnedSliverBySliverIdMap::const_iterator ownedIt = namespaceData->mSliverOwner->getOwnedSliverBySliverIdMap().begin();
                SliverOwner::OwnedSliverBySliverIdMap::const_iterator ownedEnd = namespaceData->mSliverOwner->getOwnedSliverBySliverIdMap().end();
                for (; ownedIt != ownedEnd; ++ownedIt)
                {
                    OwnedSliverState& ownedSliverState = *state.getSliverOwnerState().getOwnedSliverStateBySliverIdMap().allocate_element();
                    state.getSliverOwnerState().getOwnedSliverStateBySliverIdMap()[ownedIt->first] = &ownedSliverState;

                    ownedSliverState.setRevisionId(ownedIt->second->getBaseSliver().getState().getRevisionId());
                    ownedSliverState.setLoadCounter(ownedIt->second->getLoad());
                }
            }

            if (namespaceData->mSliverListener != nullptr)
            {
                state.getSliverListenerState().setIsRegistered(true);
                state.getSliverListenerState().setSliverPartitionInfo(const_cast<SliverPartitionInfo&>(namespaceData->mSliverListener->getPartitionInfo()));
            }
        }
    }
}

void SliverManager::onUpdateSliverListenerPartitionCount(const SliverListenerPartitionCountUpdate& data, UserSession*)
{
    // this code path is used when dynamically updating the partition count
    SliverNamespaceData* sliverNamespaceData = getOrCreateSliverNamespace(data.getSliverNamespace());
    if (sliverNamespaceData != nullptr)
        updateSliverNamespacePartitionData(*sliverNamespaceData, data.getPartitionCount());
}

void SliverManager::registerSliverOwner(SliverOwner& sliverOwner)
{
    SliverNamespaceData* sliverNamespaceData = getOrCreateSliverNamespace(sliverOwner.getSliverNamespace());
    if (sliverNamespaceData != nullptr)
    {
        EA_ASSERT_MSG(sliverNamespaceData->mSliverOwner == nullptr || sliverNamespaceData->mSliverOwner == &sliverOwner, "More than one registered SliverOwner for a given SliverNamespace is not supported.");
        sliverNamespaceData->mSliverOwner = &sliverOwner;
        sliverNamespaceData->mSliverOwner->setUpdateCoalescePeriod(sliverNamespaceData->mInfo.getConfig().getUpdateCoalescePeriod());
        scheduleSliverCoordinatorUpdate();
    }
}

void SliverManager::deregisterSliverOwner(SliverOwner& sliverOwner)
{
    SliverNamespaceData* sliverNamespaceData = getOrCreateSliverNamespace(sliverOwner.getSliverNamespace());
    if (sliverNamespaceData != nullptr)
    {
        EA_ASSERT_MSG(sliverNamespaceData->mSliverOwner == nullptr || sliverNamespaceData->mSliverOwner == &sliverOwner, "More than one registered SliverOwner for a given SliverNamespace is not supported.");
        sliverNamespaceData->mSliverOwner = nullptr;
        scheduleSliverCoordinatorUpdate();
    }
}

void SliverManager::registerSliverListener(SliverListener& sliverListener)
{
    SliverNamespaceData* sliverNamespaceData = getOrCreateSliverNamespace(sliverListener.getSliverNamespace());
    if (sliverNamespaceData != nullptr)
    {
        EA_ASSERT_MSG(sliverNamespaceData->mSliverListener == nullptr || sliverNamespaceData->mSliverListener == &sliverListener, "More than one registered mSliverListener for a given SliverNamespace is not supported.");
        sliverNamespaceData->mSliverListener = &sliverListener;
        scheduleSliverCoordinatorUpdate();
    }
}

void SliverManager::deregisterSliverListener(SliverListener& sliverListener)
{
    SliverNamespaceData* sliverNamespaceData = getOrCreateSliverNamespace(sliverListener.getSliverNamespace());
    if (sliverNamespaceData != nullptr)
    {
        EA_ASSERT_MSG(sliverNamespaceData->mSliverListener == nullptr || sliverNamespaceData->mSliverListener == &sliverListener, "More than one registered mSliverListener for a given SliverNamespace is not supported.");
        sliverNamespaceData->mSliverListener = nullptr;
        scheduleSliverCoordinatorUpdate();
    }

    sliverListener.shutdown();
}

ImportSliverError::Error SliverManager::processImportSliver(const SliverInfo& request, const Message*)
{
    BlazeRpcError err = ERR_SYSTEM;

    if (gController->isDraining())
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.processImportSliver: A request to import SliverId(" << request.getSliverId() << ") in "
            "SliverNamespace(" << getSliverNamespaceStr(request.getSliverNamespace()) << ") received while this instance is already draining.");
    }

    SliverOwner* sliverOwner = getSliverOwner(request.getSliverNamespace());
    if (sliverOwner != nullptr)
    {
        err = sliverOwner->importSliver(request.getSliverId(), request.getState().getRevisionId());
    }
    else
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.processImportSliver: A request to import SliverId(" << request.getSliverId() << ") in "
            "SliverNamespace(" << getSliverNamespaceStr(request.getSliverNamespace()) << ") failed because there is no SliverOwner on this instance.");
    }

    return ImportSliverError::commandErrorFromBlazeError(err);
}

ExportSliverError::Error SliverManager::processExportSliver(const SliverInfo& request, const Message*)
{
    BlazeRpcError err = ERR_SYSTEM;

    SliverOwner* sliverOwner = getSliverOwner(request.getSliverNamespace());
    if (sliverOwner != nullptr)
    {
        err = sliverOwner->exportSliver(request.getSliverId(), request.getState().getRevisionId());
    }
    else
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.processExportSliver: A request to export SliverId(" << request.getSliverId() << ") in "
            "SliverNamespace(" << getSliverNamespaceStr(request.getSliverNamespace()) << ") failed because there is no SliverOwner on this instance.");
    }

    return ExportSliverError::commandErrorFromBlazeError(err);
}

DropSliverError::Error SliverManager::processDropSliver(const SliverInfo& request, const Message*)
{
    BlazeRpcError err = ERR_SYSTEM;

    SliverOwner* sliverOwner = getSliverOwner(request.getSliverNamespace());
    if (sliverOwner != nullptr)
    {
        //err = sliverOwner->dropSliver(request.getSliverId(), request.getState().getRevisionId());
    }
    else
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.processDropSliver: A request to export SliverId(" << request.getSliverId() << ") in "
            "SliverNamespace(" << getSliverNamespaceStr(request.getSliverNamespace()) << ") failed because there is no SliverOwner on this instance.");
    }

    return DropSliverError::commandErrorFromBlazeError(err);
}

void SliverManager::updateSliverOwnershipIndiciesAndCounts(SliverIdentity sliverIdentity, const SliverState& prevState, const SliverState& newState)
{
    // A Sliver's InstanceId should always transition through the INVALID_INSTANCE_ID (not owned) state before
    // being assigned a new owner.  If this is not happening, then something in the migration process is not working.

    const SliverNamespace sliverNamespace = GetSliverNamespaceFromSliverIdentity(sliverIdentity);
    const SliverId sliverId = GetSliverIdFromSliverIdentity(sliverIdentity);

    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
    {
        if (prevState.getSliverInstanceId() != INVALID_INSTANCE_ID)
        {
            bool erased = (mSliverIdentitySetByInstanceId[prevState.getSliverInstanceId()].erase(sliverIdentity) > 0);
            if (erased)
            {
                auto sliverIdsItr = data->mSliverIdsByInstanceIdMap.find(prevState.getSliverInstanceId());
                if (sliverIdsItr != data->mSliverIdsByInstanceIdMap.end())
                {
                    sliverIdsItr->second.erase(sliverId);
                    if (sliverIdsItr->second.empty())
                        data->mSliverIdsByInstanceIdMap.erase(sliverIdsItr);
                }
                --data->mCountTotalOwned;
            }
            else
            {
                BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.processUpdateSliverState: Unexpected release of sliver ownership! InstanceId(" << prevState.getSliverInstanceId() << ") is not known to be the owner of: "
                    "SliverNamespace(" << getSliverNamespaceStr(sliverNamespace) << "), "
                    "SliverId(" << sliverId << ")");
            }
        } 

        if (newState.getSliverInstanceId() != INVALID_INSTANCE_ID)
        {
            bool inserted = mSliverIdentitySetByInstanceId[newState.getSliverInstanceId()].insert(sliverIdentity).second;
            if (inserted)
            {
                data->mSliverIdsByInstanceIdMap[newState.getSliverInstanceId()].insert(sliverId);
                ++data->mCountTotalOwned;
            }
            else
            {
                BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.processUpdateSliverState: Unexpected declaration of sliver ownership! InstanceId(" << newState.getSliverInstanceId() << ") already known to be the owner of: "
                    "SliverNamespace(" << getSliverNamespaceStr(sliverNamespace) << "), "
                    "SliverId(" << sliverId << ")");
            }
        }
    }
}

void SliverManager::periodicHealthCheck()
{
    for (SliverNamespaceDataBySliverNamespaceMap::const_iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData& namespaceData = *it->second;
        namespaceData.healthCheck();
    }

    mPeriodicHealthCheckTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + (5 * 1000 * 1000),
        this, &SliverManager::periodicHealthCheck, "SliverManager::periodicHealthCheck");
}

void SliverManager::SliverNamespaceData::healthCheck()
{
    TimeValue now = TimeValue::getTimeOfDay();
    if (mIsStable)
    {
        if (mCountTotalOwned != mInfo.getConfig().getSliverCount())
        {
            mIsStable = false;
            mDestabilizedAt = now;
            BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "Global SliverNamespace '" << gSliverManager->getSliverNamespaceStr(mNamespace) << "' has become UNSTABLE; "
                "Owned: " << mCountTotalOwned << ", Unowned: " << (mInfo.getConfig().getSliverCount() - mCountTotalOwned));
        }
    }
    else
    {
        if (mCountTotalOwned == mInfo.getConfig().getSliverCount())
        {
            mIsStable = true;
            mStabilizedAt = now;
            BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "Global SliverNamespace '" << gSliverManager->getSliverNamespaceStr(mNamespace) << "' is STABLE; "
                "Owned: " << mCountTotalOwned << ", Unowned: " << (mInfo.getConfig().getSliverCount() - mCountTotalOwned));
        }
        else if ((now - mLastLogEntryAt) > 30 * 1000 * 1000)
        {
            mLastLogEntryAt = now;
            BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Global SliverNamespace '" << gSliverManager->getSliverNamespaceStr(mNamespace) << "' has been UNSTABLE for " << Timer(mDestabilizedAt, now) << "; "
                "Owned: " << mCountTotalOwned << ", "
                "Unowned: " << (mInfo.getConfig().getSliverCount() - mCountTotalOwned) << ", "
                "Destabilized: " << mDestabilizedAt << ", "
                "Last Stable: " << (mLastStableAt > 0 ? (StringBuilder() << mLastStableAt).get() : "never"));

            repairUnownedSlivers();
        }
    }

    if (mIsStable)
        mLastStableAt = now;
}

void SliverManager::onSliverRepairNeeded(const SliverRepairParameters& data, ::Blaze::UserSession*)
{
    if (getState() != State::ACTIVE)
        return;

    SliverOwner* sliverOwner = getSliverOwner(data.getSliverNamespace());
    if (sliverOwner != nullptr)
        sliverOwner->repairSibling(data.getInstanceId(), data.getSliverIdList());
}

void SliverManager::broadcastAndConfirmUpdateSliverState(Sliver& sliver, const SliverInfo& sliverInfo)
{
    SliverIdentity sliverIdentity = MakeSliverIdentity(sliverInfo.getSliverNamespace(), sliverInfo.getSliverId());

    SliverStateUpdateRequest request;
    request.setSendConfirmationToInstanceId(getLocalInstanceId());
    request.setSliverInfo(const_cast<SliverInfo&>(sliverInfo));
    Notification notification(SliversSlave::NOTIFICATION_INFO_SLIVERSTATEUPDATEREQUIRED, &request, NotificationSendOpts());

    sliver.mAccessRWMutex.lockForWrite();

    if (!mSlaveList.empty())
    {
        sliver.mAccessRWMutex.allowPriorityLockForRead();

        PendingUpdateConfirmation& updateConfirmation = mPendingUpdateConfirmationMap[sliverIdentity];
        updateConfirmation.requiredRevisionId = sliverInfo.getState().getRevisionId();

        for (SlaveSessionList::iterator it = mSlaveList.begin(), end = mSlaveList.end(); it != end; ++it)
        {
            SlaveSession& slaveSession = *it->second;
            updateConfirmation.pendingInstanceIds.insert(slaveSession.getInstanceId());
        }

        Timer confirmationWaitTimer;
        for (int32_t attempt = 1; !Fiber::isCurrentlyCancelled(); ++attempt)
        {
            InstanceIdSet::iterator it = updateConfirmation.pendingInstanceIds.begin();
            while (it != updateConfirmation.pendingInstanceIds.end())
            {
                SlaveSession* slaveSession = mSlaveList.getSlaveSessionByInstanceId(*it);
                if ((slaveSession != nullptr) && (slaveSession->getConnection() != nullptr) && slaveSession->getConnection()->isConnected())
                {
                    slaveSession->sendNotification(notification);

                    const RemoteServerInstance* remoteServer = gController->getRemoteServerInstance(*it);
                    if ((remoteServer != nullptr) && (remoteServer->getConnection() != nullptr) && remoteServer->getConnection()->isConnected())
                    {
                        // We expect a confirmation from the remote server, so go to the next pendingInstance without erasing.
                        ++it;
                        continue;
                    }
                }

                // Either the SlaveSession is not actually connected, or there is no outbound connection to him that would allow him
                // to send us back a confirmation.  So, erase this entry so that we don't hold things up waiting for him to provide state confirmation.
                it = updateConfirmation.pendingInstanceIds.erase(it);
            }

            if (!updateConfirmation.pendingInstanceIds.empty())
            {
                updateConfirmation.eventHandle = Fiber::getNextEventHandle();
                BlazeError err = Fiber::wait(updateConfirmation.eventHandle, "SliverManager::broadcastAndConfirmUpdateSliverState(remote confirmation)",
                    TimeValue::getTimeOfDay() + (SLIVER_STATE_UPDATE_TIMEOUT_MS * 1000));
                if (err != ERR_OK)
                {
                    StringBuilder sb;
                    if (updateConfirmation.pendingInstanceIds.empty())
                        sb << "(no pending instances)";
                    else
                    {
                        for (Component::InstanceIdSet::iterator it2 = updateConfirmation.pendingInstanceIds.begin(), end2 = updateConfirmation.pendingInstanceIds.end(); it2 != end2; ++it2)
                        {
                            sb << *it2 << ", ";
                        }
                        sb.trim(2);
                    }

                    bool giveUp = confirmationWaitTimer.getInterval() > (60 * 1000 * 1000);

                    BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(sliver) << ": broadcastAndConfirmUpdateSliverState: No confirmation received after " << confirmationWaitTimer << " from instances " << sb.get() << ". "
                        << (giveUp ? "Waited too long, proceeding with state update; " : "Continuing to wait for confirmation; ")
                        << LogCurrentState(sliver) << ", " << LogPendingState(sliver) << ", err=" << err);

                    if (!giveUp)
                        continue;
                }
            }

            break;
        }

        mPendingUpdateConfirmationMap.erase(sliverIdentity);

        sliver.mAccessRWMutex.disallowPriorityLockForRead();
    }

    request.setSendConfirmationToInstanceId(INVALID_INSTANCE_ID);

    scheduleRequiredSliverStateUpdate(request);

    sliver.mAccessRWMutex.unlockForWrite();
}

bool SliverManager::validateSliverNamespace(SliverNamespace sliverNamespace)
{
    // Currently a SliverNamespace *is* a ComponentId.  However, in the future, a SliverNamespace will be an arbitrary
    // id, that can be bound to a ComponentId (via codegen, or some other means). This is left in place in anticipation of this future change.
    return BlazeRpcComponentDb::isValidComponentId(sliverNamespace);
}

SliverManager::SliverNamespaceData* SliverManager::getOrCreateSliverNamespace(SliverNamespace sliverNamespace)
{
    SliverNamespaceData* namespaceData = nullptr;

    if (validateSliverNamespace(sliverNamespace))
    {
        SliverNamespaceDataBySliverNamespaceMap::iterator it = mSliverNamespaceMap.find(sliverNamespace);
        if (it == mSliverNamespaceMap.end())
        {
            namespaceData = BLAZE_NEW SliverNamespaceData();
            namespaceData->mNamespace = sliverNamespace;
            namespaceData->mLastLogEntryAt = TimeValue::getTimeOfDay();

            updateSliverNamespaceConfig(*namespaceData);

            mSliverNamespaceMap[sliverNamespace] = namespaceData;
        }
        else
        {
            namespaceData = static_cast<SliverNamespaceData*>(it->second);
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.getOrCreateSliverNamespace: The provided SliverNamespace(" << sliverNamespace << ") is not valid.");
    }

    return namespaceData;
}

void SliverManager::updateSliverNamespaceConfig(SliverNamespaceData& namespaceData)
{
    const char8_t* componentName = BlazeRpcComponentDb::getComponentNameById(namespaceData.mNamespace);
    if (componentName == nullptr)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverNamespaceConfig: failed to update - SliverNamespace(" << namespaceData.mNamespace << ") is not valid.");
        return;
    }

    SliverNamespaceConfig& currentConfig = namespaceData.mInfo.getConfig();

    SliverNamespaceConfigMap::const_iterator it = getConfig().getSliverNamespaces().find(componentName);
    if (it == getConfig().getSliverNamespaces().end())
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverNamespaceConfig: failed to update - SliverNamespace(" << namespaceData.mNamespace
            << ") does not have component(" << componentName << ") config to use.");
        return;
    }
    const SliverNamespaceConfig* configToUse = it->second;

    // validate configToUse
    if (configToUse->getSliverCount() == 0)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverNamespaceConfig: failed to update - SliverNamespace(" << namespaceData.mNamespace
            << ")'s component(" << componentName << ") config specifies an invalid sliver count(" << configToUse->getSliverCount() << ").");
        return;
    }

    if ((configToUse->getMaxConcurrentSliverExports() <= 0) || (configToUse->getMaxConcurrentSliverExports() > 50))
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverNamespaceConfig: failed to update - SliverNamespace(" << namespaceData.mNamespace
            << ")'s component(" << componentName << ") config specifies an invalid maximum concurrent sliver exports setting(" << configToUse->getMaxConcurrentSliverExports() << ").");
        return;
    }

    if (namespaceData.mIsConfigured)
    {
        // validate reconfiguration
        if ((currentConfig.getSliverCount() != 0) && (currentConfig.getSliverCount() != configToUse->getSliverCount()))
        {
            BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverNamespaceConfig: failed to update - SliverNamespace(" << namespaceData.mNamespace
                << ")'s component(" << componentName << ") config is trying to reconfigure the sliver count; not currently supported.");
            return;
        }
    }

    // validation complete, save the new configuration
    configToUse->copyInto(currentConfig);
    namespaceData.mIsConfigured = true;

    if ((currentConfig.getPartitionCount() != 1) && (namespaceData.mNamespace == UserSessionsMaster::COMPONENT_ID))
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverNamespaceConfig: SliverNamespace(" << namespaceData.mNamespace
            << ")'s component(" << componentName << ") config specifies an invalid partition count(" << currentConfig.getPartitionCount()
            << "); must be 1. (overriding and ignoring this config setting)");
        currentConfig.setPartitionCount(1);
    }

    SliverPtrBySliverIdMap& slivers = namespaceData.mSliverPtrBySliverIdMap;
    if (slivers.size() != currentConfig.getSliverCount())
    {
        slivers.clear();
        slivers.reserve(currentConfig.getSliverCount());

        for (SliverId sliverId = SLIVER_ID_MIN; sliverId <= namespaceData.mInfo.getConfig().getSliverCount(); ++sliverId)
        {
            slivers[sliverId] = BLAZE_NEW Sliver(MakeSliverIdentity(namespaceData.mNamespace, sliverId));
        }
    }

    if (namespaceData.mSliverOwner != nullptr)
        namespaceData.mSliverOwner->setUpdateCoalescePeriod(currentConfig.getUpdateCoalescePeriod());

    if (currentConfig.getPartitionCount() != 0)
    {
        updateSliverNamespacePartitionData(namespaceData, currentConfig.getPartitionCount());
    }
    // Otherwise, the partition count will be dynamically updated.  Wait for the coordinator to tell us what the count is, and then update the namespace's partition data.
}

void SliverManager::updateSliverNamespacePartitionData(SliverNamespaceData& namespaceData, uint16_t partitionCount)
{
    if (partitionCount == 0)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.updateSliverNamespacePartitionData: failed to update - SliverNamespace(" << namespaceData.mNamespace
            << ") cannot be updated to zero partitions.");
        return;
    }

    SliverNamespaceConfig& currentConfig = namespaceData.mInfo.getConfig();

    SliverPartitionDataList& sliverPartitions = namespaceData.mPartitionDataList;
    if (sliverPartitions.size() != partitionCount)
    {
        sliverPartitions.clear();
        sliverPartitions.resize(partitionCount);

        SliverId sliverId = SLIVER_ID_MIN;
        for (uint16_t partitionIndex = 0; partitionIndex < partitionCount; ++partitionIndex)
        {
            int32_t numberOfSlivers = calcSliverBucketSize(currentConfig.getSliverCount(), partitionCount, partitionIndex);

            SliverIdSet& sliverIds = sliverPartitions[partitionIndex].mSliverIds;
            sliverIds.reserve(numberOfSlivers);

            for (int32_t counter = 0; counter < numberOfSlivers; ++counter)
            {
                sliverIds.insert(sliverId++);
            }

            EA_ASSERT((sliverId-1) <= currentConfig.getSliverCount());
        }
    }

    // Recalculate the ranks for the existing known SliverListeners.
    for (SliverIdSetByInstanceIdMap::iterator sisIt = namespaceData.mListenerSliverIdSets.begin(), end = namespaceData.mListenerSliverIdSets.end(); sisIt != end; ++sisIt)
    {
        updateSliverListenerPartitionRanks(namespaceData, sisIt);
    }
}

uint16_t SliverManager::getSliverCount(SliverNamespace sliverNamespace)
{
    SliverNamespaceData* namespaceData = getOrCreateSliverNamespace(sliverNamespace);
    if (namespaceData != nullptr)
        return namespaceData->mInfo.getConfig().getSliverCount();

    return 0;
}

uint16_t SliverManager::getSliverNamespacePartitionCount(SliverNamespace sliverNamespace) const
{
    uint16_t partitionCount = 0;
    SliverNamespaceDataBySliverNamespaceMap::const_iterator it = mSliverNamespaceMap.find(sliverNamespace);
    if (it != mSliverNamespaceMap.end())
    {
        const SliverNamespaceData& data = *it->second;
        partitionCount =  (uint16_t) data.mPartitionDataList.size();
    }
    return partitionCount;
}

const char8_t* SliverManager::getSliverNamespaceStr(SliverNamespace sliverNamespace) const
{
    return BlazeRpcComponentDb::getComponentNameById(sliverNamespace);
}

int32_t SliverManager::getOwnedSliverCount(SliverNamespace sliverNamespace, InstanceId instanceId)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
    {
        auto itr = data->mSliverIdsByInstanceIdMap.find(instanceId);
        if (itr != data->mSliverIdsByInstanceIdMap.end())
            return (int32_t) itr->second.size();
    }
    return 0;
}

int32_t SliverManager::getOwnedSliverCount(SliverNamespace sliverNamespace)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
        return data->mCountTotalOwned;
    return 0;
}

const Blaze::SliverIdSet* SliverManager::getOwnedSliverIdsByInstance(SliverNamespace sliverNamespace, InstanceId instanceId)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
    {
        auto itr = data->mSliverIdsByInstanceIdMap.find(instanceId);
        if (itr != data->mSliverIdsByInstanceIdMap.end())
            return &itr->second;
    }
    return 0;
}

SliverId SliverManager::getAnySliverIdByInstanceId(SliverNamespace sliverNamespace, InstanceId instanceId)
{
    auto* sliverIdSet = getOwnedSliverIdsByInstance(sliverNamespace, instanceId);
    if (sliverIdSet == nullptr || sliverIdSet->empty())
    {
        return INVALID_SLIVER_ID;
    }
    auto sliverCount = sliverIdSet->size();
    auto sliverIndex = gRandom->getRandomNumber(sliverCount);
    SliverId sliverId = sliverIdSet->at(sliverIndex);
    return sliverId;
}

bool SliverManager::isSliverNamespaceAbandoned(SliverNamespace sliverNamespace)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
    {
        for (auto& itr : data->mSliverIdsByInstanceIdMap)
        {
            if (!gController->isInstanceDraining(itr.first))
                return false;
        }
    }

    return true;
}

bool SliverManager::isAutoMigrationOnShutdownEnabled(SliverNamespace sliverNamespace)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
        return data->mInfo.getConfig().getAutoMigrationOnShutdown();

    return false;
}

SliverOwner* SliverManager::getSliverOwner(SliverNamespace sliverNamespace)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
        return data->mSliverOwner;

    return nullptr;
}

SliverListener* SliverManager::getSliverListener(SliverNamespace sliverNamespace)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
        return data->mSliverListener;

    return nullptr;
}


SliverPtr SliverManager::getSliver(SliverIdentity sliverIdentity)
{
    if (!Sliver::isValidIdentity(sliverIdentity))
        return nullptr;

    return getSliver(GetSliverNamespaceFromSliverIdentity(sliverIdentity), GetSliverIdFromSliverIdentity(sliverIdentity));
}

SliverPtr SliverManager::getSliver(SliverNamespace sliverNamespace, SliverId sliverId)
{
    SliverPtr sliver;

    SliverNamespaceData* namespaceData = getOrCreateSliverNamespace(sliverNamespace);
    if (namespaceData != nullptr)
    {
        if (sliverId == INVALID_SLIVER_ID)
            sliverId = getValidSliverID(namespaceData->mInfo.getConfig().getSliverCount());

        sliver = getSliver(namespaceData, sliverId);
        if (sliver == nullptr)
        {
            BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.getSliver: SliverId(" << sliverId << ") was not found in SliverNamespace(" << getSliverNamespaceStr(sliverNamespace) << "), "
                "which is configured to have (" << namespaceData->mInfo.getConfig().getSliverCount() << ") slivers. "
                "The current mSliverPtrBySliverIdMap size is (" << namespaceData->mSliverPtrBySliverIdMap.size() << ")");
        }
    }

    if (EA_UNLIKELY(sliver == nullptr))
    {
        BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager::getSliver: Could not resolve " << RoutingOptions(sliverNamespace, sliverId) << " to a valid sliver. "
            "This should not happen if the sliver id and namespace appear to be valid. Fiber context: " << Fiber::getCurrentContext());
    }

    return sliver;
}

SliverPtr SliverManager::getSliverFromSliverKey(SliverNamespace sliverNamespace, SliverKey _key)
{
    return getSliver(sliverNamespace, GetSliverIdFromSliverKey(_key));
}

SliverPtr SliverManager::getSliver(SliverNamespaceData* namespaceData, SliverId sliverId)
{
    SliverPtrBySliverIdMap::iterator it = namespaceData->mSliverPtrBySliverIdMap.find(sliverId);
    if (it != namespaceData->mSliverPtrBySliverIdMap.end())
        return it->second;

    return nullptr;
}

BlazeRpcError SliverManager::waitForSliverAccess(SliverIdentity sliverIdentity, Sliver::AccessRef& sliverAccess, const char8_t* context, bool priorityAccess)
{
    BlazeRpcError err;

    SliverPtr sliver = getSliver(sliverIdentity);
    if (sliver != nullptr)
    {
        if (priorityAccess)
            err = (sliver->getPriorityAccess(sliverAccess) ? ERR_OK : ERR_SYSTEM);
        else
            err = sliver->waitForAccess(sliverAccess, context);
    }
    else
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.waitForSliverAccess: Failed to find SliverId(" << GetSliverIdFromSliverIdentity(sliverIdentity) << ") in SliverNamespace(" << getSliverNamespaceStr(GetSliverNamespaceFromSliverIdentity(sliverIdentity)) << ")");
        err = ERR_SYSTEM;
    }

    return err;
}

InstanceId SliverManager::getSliverInstanceId(SliverNamespace sliverNamespace, SliverId sliverId)
{
    SliverPtr sliver = getSliver(sliverNamespace, sliverId);
    if (sliver != nullptr)
        return sliver->getState().getSliverInstanceId();

    return INVALID_INSTANCE_ID;
}

SliverListenerRank SliverManager::getSliverListenerRank(SliverNamespace sliverNamespace, SliverPartitionIndex partitionIndex, InstanceId listenerInstanceId)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr && !data->mPartitionDataList.empty())
    {
        ListenerInfoByListenerRankMap& listenerRankMap = data->mPartitionDataList[partitionIndex].mListenerRanks;
        for (ListenerInfoByListenerRankMap::iterator it = listenerRankMap.begin(), end = listenerRankMap.end(); it != end; ++it)
        {
            SliverListenerInfo& listenerInfo = it->second;
            if (listenerInfo.mInstanceId == listenerInstanceId)
                return it->first;
        }
    }

    return UINT16_MAX;
}

void SliverManager::getFullListenerCoverage(SliverNamespace sliverNamespace, Blaze::InstanceIdList& listeners)
{
    SliverNamespaceData* data = getOrCreateSliverNamespace(sliverNamespace);
    if (data != nullptr)
    {
        if (!data->mPartitionDataList.empty())
        {
            Component::InstanceIdSet instanceIdSet;
            instanceIdSet.reserve(data->mPartitionDataList.size());
            listeners.reserve(data->mPartitionDataList.size());

            for (size_t partitionIndex = 0; partitionIndex < data->mPartitionDataList.size(); ++partitionIndex)
            {
                InstanceId instanceId = INVALID_INSTANCE_ID;

                ListenerInfoByListenerRankMap& listenerRankMap = data->mPartitionDataList[partitionIndex].mListenerRanks;
                if (!listenerRankMap.empty())
                {
                    typedef eastl::pair<ListenerInfoByListenerRankMap::iterator, ListenerInfoByListenerRankMap::iterator> PriorityRange;

                    SliverListenerRank rank = listenerRankMap.front().first;

                    PriorityRange range = listenerRankMap.equal_range(rank);
                    size_t count = eastl::distance(range.first, range.second);

                    ListenerInfoByListenerRankMap::iterator selected = range.first;
                    eastl::advance(selected, gRandom->getRandomNumber(count));
                    for (size_t i = 0; i < count; ++i, ++selected)
                    {
                        if (selected == range.second)
                            selected = range.first;

                        InstanceId tmpId = selected->second.mInstanceId;

                        const RemoteServerInstance* remoteServerInstance = nullptr;
                        if (tmpId != gController->getInstanceId())
                            remoteServerInstance = gController->getRemoteServerInstance(tmpId);

                        if ((tmpId == gController->getInstanceId()) ||
                            ((remoteServerInstance != nullptr) && remoteServerInstance->isConnected() && !remoteServerInstance->isDraining()))
                        {
                            instanceId = tmpId;
                            break;
                        }
                    }
                }

                if (instanceId != INVALID_INSTANCE_ID)
                {
                    // Only insert into the resulting 'listeners' arg if we haven't added it already (e.g. no duplicates)
                    if (instanceIdSet.insert(instanceId).second)
                        listeners.push_back(instanceId);
                }
                else
                {
                    BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.getFullListenerCoverage: Partition index(" << partitionIndex << ") has no coverage. "
                        "Consider starting more listeners in the '" << getSliverNamespaceStr(sliverNamespace) << "' namespace, or reducing the number of partitions.");
                }
            }
        }
    }
    else
    {
        BLAZE_ASSERT_LOG(getLogCategory(), "SliverManager.getFullListenerCoverage: Call to getOrCreateSliverNamespace(" << sliverNamespace << ") failed.");
    }
}

void SliverManager::addText(eastl::string& line, const eastl::string& text, bool last)
{
    int32_t width = 15;
    int32_t spaces = width - text.length();
    int32_t left = spaces / 2;
    int32_t right = left + (spaces % 2);

    line.append_sprintf("|%*s%s%*s%s", left, "", text.c_str(), right, "", (last ? "|" : ""));
}

void SliverManager::printListenerTable(const SliverNamespaceData& namespaceData)
{
    // This method only prints the listener table to the logs if the current instance is the
    // active SliverCoordinator, and TRACE logging is enabled for the Slivers component.
    if (mSliverCoordinator.isDesignatedCoordinator() && !BLAZE_IS_LOGGING_ENABLED(getLogCategory(), Logging::INFO))
    {
        StringBuilder tableStr;
        generateAsciiListenerTable(namespaceData, tableStr);
        BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, tableStr.get());
    }
}

void SliverManager::generateAsciiListenerTable(const SliverNamespaceData& namespaceData, StringBuilder& tableStr)
{
    const eastl::string emptyLine("");
    const eastl::string horizLine("---------------");

    const SliverPartitionDataList& partitionDataList = namespaceData.mPartitionDataList;
    eastl::vector<eastl::string> lines;
    lines.resize(6);

    size_t rowCount = 0;
    for (size_t x = 0; x < partitionDataList.size(); ++x)
    {
        rowCount = eastl::max(partitionDataList[x].mListenerRanks.size(), rowCount);
    }

    lines.resize((rowCount * 5) + lines.size());

    for (size_t x = 0; x < partitionDataList.size(); ++x)
    {
        int32_t lineNum = 0;
        eastl::string text;
        const SliverPartitionData& partitionData = partitionDataList[x];
        const ListenerInfoByListenerRankMap& listeners = partitionData.mListenerRanks;

        bool last = (x == (partitionDataList.size() - 1));

        addText(lines[lineNum++], horizLine, last);

        text.sprintf("Partition %d", x + 1);
        addText(lines[lineNum++], text, last);

        text.sprintf("%d-%d", partitionData.mSliverIds.front(), partitionData.mSliverIds.back());
        addText(lines[lineNum++], text, last);

        addText(lines[lineNum++], horizLine, last);

        for (int32_t y = (int32_t)rowCount - 1; y >= 0; --y)
        {
            eastl::string instanceName;
            eastl::string instanceRank;
            if (y < (int32_t)listeners.size())
            {
                const SliverListenerInfo& listenerInfo = listeners[y].second;

                const RemoteServerInstance* remoteServerInstance = gController->getRemoteServerInstance(listenerInfo.mInstanceId);
                if (remoteServerInstance != nullptr)
                    instanceName = remoteServerInstance->getInstanceName();
                else
                    instanceName.sprintf("(instance:%" PRIu16 ")", listenerInfo.mInstanceId);

                switch (listenerInfo.mSyncState)
                {
                case SYNCSTATE_COMPLETE:instanceRank = "COMPLETE";  break;
                case SYNCSTATE_SYNC:    instanceRank = "SYNCING";   break;
                case SYNCSTATE_DESYNC:  instanceRank = "DESYNCING"; break;
                case SYNCSTATE_UNKNOWN: instanceRank = "UNKNOWN";   break;
                }
            }

            addText(lines[lineNum++], emptyLine, last);
            addText(lines[lineNum++], instanceName, last);
            addText(lines[lineNum++], instanceRank, last);
            addText(lines[lineNum++], emptyLine, last);
            addText(lines[lineNum++], horizLine, last);
        }
    }

    eastl::string listenerMatrix;
    for (size_t lineNum = 0; lineNum < lines.size(); ++lineNum)
    {
        listenerMatrix.append(lines[lineNum]);
        listenerMatrix.append("\n\n");
    }

    tableStr << "Sliver Listener Matrix for: " << getSliverNamespaceStr(namespaceData.mNamespace) << "\n" << listenerMatrix;
}

void SliverManager::generateLocalHealthReport(StringBuilder& status)
{
    status << "Slivers Status on " << gController->getInstanceName() << ":\n";

    for (SliverNamespaceDataBySliverNamespaceMap::iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData* namespaceData = it->second;

        for (SliverPtrBySliverIdMap::iterator sliverIt = namespaceData->mSliverPtrBySliverIdMap.begin(), sliverEnd = namespaceData->mSliverPtrBySliverIdMap.end(); sliverIt != sliverEnd; ++sliverIt)
        {
            SliverPtr& sliver = sliverIt->second;
            sliver->checkHealth(status);
        }
    }
}

struct SliverInstanceIdFetcher
{
    SliverInstanceIdFetcher() {}
    InstanceId operator()(InstanceId instanceId) { return instanceId; }
};

void SliverManager::SliverNamespaceData::repairUnownedSlivers()
{
    if (gSliverManager->isSliverNamespaceAbandoned(mNamespace))
    {
        // If we think there simply *can't* be any sliver owners, then there's no point in proceeding.
        return;
    }

    int32_t unownedCount = mInfo.getConfig().getSliverCount() - mCountTotalOwned;
    if (unownedCount > 0)
    {
        SliverRepairParameters repairParams;
        TimeValue now = TimeValue::getTimeOfDay();

        repairParams.setInstanceId(gController->getInstanceId());
        repairParams.setSliverNamespace(mNamespace);

        SliverIdList& sliverIdList = repairParams.getSliverIdList();
        sliverIdList.clear();
        sliverIdList.reserve(unownedCount);

        StringBuilder sliverListStr;
        for (SliverPtrBySliverIdMap::iterator sliverIt = mSliverPtrBySliverIdMap.begin(), sliverEnd = mSliverPtrBySliverIdMap.end(); sliverIt != sliverEnd; ++sliverIt)
        {
            SliverPtr& sliver = sliverIt->second;
            if (!sliver->isImportingToCurrentInstance() // We never want to reset the state of a sliver that we are currently importing.  (see https://eadpjira.ea.com/browse/GOS-31857)
                && (sliver->getSliverInstanceId() == INVALID_INSTANCE_ID)
                && ((now - sliver->getLastStateUpdateTime()) > Sliver::UNOWNED_GRACE_PERIOD_MS * 1000))
            {
                // If the sliver has been unowned for more than UNOWNED_GRACE_PERIOD_MS milliseconds (currently 2 minutes), then we can presume something is
                // really really wrong.  We reset the sliver's state, enabling it to accept the state that we are about to ask for from the Sliver Coordinator.
                gSliverManager->resetSliverState(sliver, ERR_DISCONNECTED);

                sliverIdList.push_back(sliver->getSliverId());
                sliverListStr << sliver->getSliverId() << ", ";

                // Early out if we know we've already found all of our unowned slivers. Also, don't repair more than 50 at a time.
                if ((sliverIdList.size() == (size_t)unownedCount) || (sliverIdList.size() >= 50))
                    break;
            }
        }
        sliverListStr.trim(2); // trim the ", "

        Component* component = gController->getComponentManager().getComponent(mNamespace);
        Component::InstanceIdList instances;
        component->getComponentInstances(instances, true, true);

        if (!instances.empty() && !sliverIdList.empty())
        {
            BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverNamespaceData.repairUnownedSlivers: Attempting to repair the following unowned slivers: "
                << gSliverManager->getSliverNamespaceStr(mNamespace) << "[" << sliverListStr << "]");
            gSliverManager->sendSliverRepairNeededToInstancesById(instances.begin(), instances.end(), SliverInstanceIdFetcher(), &repairParams);
        }
    }
}

void SliverManager::resetSliverState(SliverPtr sliver, BlazeRpcError reason)
{
    SliverStateUpdateRequest updateData;
    updateData.setSendConfirmationToInstanceId(INVALID_INSTANCE_ID);
    updateData.getSliverInfo().setSliverNamespace(sliver->getSliverNamespace());
    updateData.getSliverInfo().setSliverId(sliver->getSliverId());
    updateData.getSliverInfo().getState().setSliverInstanceId(INVALID_INSTANCE_ID);
    updateData.getSliverInfo().getState().setRevisionId(INVALID_SLIVER_REVISION_ID);
    updateData.getSliverInfo().getState().setReason(reason);

    scheduleRequiredSliverStateUpdate(updateData);
}

uint16_t SliverManager::calcSliverBucketSize(uint16_t sliverCount, uint16_t bucketCount, int16_t bucketIndex)
{
    uint16_t chunk = sliverCount / bucketCount;
    uint16_t extra = sliverCount % bucketCount;

    return chunk + (extra - bucketIndex > 0 ? 1 : 0);
}

void SliverManager::getStatusInfo(ComponentStatus& status) const
{
    ComponentStatus::InfoMap& map = status.getInfo();

    for (SliverNamespaceDataBySliverNamespaceMap::const_iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData* namespaceData = it->second;
        if (namespaceData != nullptr)
        {
            const char8_t* namespaceStr = getSliverNamespaceStr(it->first);

            SliverOwner* sliverOwner = namespaceData->mSliverOwner;
            if (sliverOwner != nullptr)
            {
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_GaugeActiveImportOperations", sliverOwner->mMetrics.mGaugeActiveImportOperations.get());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_TotalImportsSucceeded", sliverOwner->mMetrics.mTotalImportsSucceeded.getTotal());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_TotalImportsFailed", sliverOwner->mMetrics.mTotalImportsFailed.getTotal());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_ImportDurationMaxUsec", sliverOwner->mMetrics.mImportTimer.getTotalMax());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_ImportDurationMinUsec", sliverOwner->mMetrics.mImportTimer.getTotalMin());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_ImportDurationAverageUsec", sliverOwner->mMetrics.mImportTimer.getTotalAverage());

                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_GaugeActiveExportOperations", sliverOwner->mMetrics.mGaugeActiveImportOperations.get());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_TotalExportsSucceeded", sliverOwner->mMetrics.mTotalExportsSucceeded.getTotal());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_TotalExportsFailed", sliverOwner->mMetrics.mTotalExportsFailed.getTotal());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_ExportDurationMaxUsec", sliverOwner->mMetrics.mExportTimer.getTotalMax());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_ExportDurationMinUsec", sliverOwner->mMetrics.mExportTimer.getTotalMin());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_ExportDurationAverageUsec", sliverOwner->mMetrics.mExportTimer.getTotalAverage());

                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_GaugeOwnedSliverCount", sliverOwner->mMetrics.mGaugeOwnedSliverCount.get());
                ADD_METRIC(map, "SliverOwner_" << namespaceStr << "_GaugeOwnedObjectCount", sliverOwner->mMetrics.mGaugeOwnedObjectCount.get());
            }

            ADD_METRIC(map, "SliverNamespace_" << namespaceStr << "_GaugeOwnedSliverCount", namespaceData->mMetrics.mGaugeOwnedSliverCount.get());
            ADD_METRIC(map, "SliverNamespace_" << namespaceStr << "_GaugeUnownedSliverCount", namespaceData->mMetrics.mGaugeUnownedSliverCount.get());
            ADD_METRIC(map, "SliverNamespace_" << namespaceStr << "_GaugePartitionCount", namespaceData->mMetrics.mGaugePartitionCount.get());
        }
    }
}

bool SliverManager::canTransitionToInService()
{
    if (getState() < ACTIVE)
        return false;

    for (SliverNamespaceDataBySliverNamespaceMap::const_iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData* namespaceData = it->second;
        if ((namespaceData != nullptr) && (namespaceData->mSliverOwner != nullptr) &&
            namespaceData->mInfo.getConfig().getRequireSliversForInServiceState())
        {
            SliverOwner& sliverOwner = *namespaceData->mSliverOwner;
            if (sliverOwner.getOwnedSliverCount() == 0)
                return false;
        }
    }

    return true;
}


MigrateSliverError::Error SliverManager::processMigrateSliver(const MigrateSliverRequest& request, MigrateSliverResponse& response, const Message*)
{
    BlazeRpcError err = mSliverCoordinator.migrateSliver(request, response);
    return MigrateSliverError::commandErrorFromBlazeError(err);
}

EjectSliversError::Error SliverManager::processEjectSlivers(const EjectSliversRequest& request, const Message*)
{
    BlazeRpcError err = mSliverCoordinator.ejectSlivers(request);
    return EjectSliversError::commandErrorFromBlazeError(err);
}

RebalanceSliversError::Error SliverManager::processRebalanceSlivers(const RebalanceSliversRequest& request, const Message*)
{
    BlazeRpcError err = mSliverCoordinator.rebalanceSlivers(request);
    return RebalanceSliversError::commandErrorFromBlazeError(err);
}

GetSliversError::Error SliverManager::processGetSlivers(const GetSliversRequest& request, GetSliversResponse& response, const Message*)
{
    BlazeRpcError err = mSliverCoordinator.getSlivers(request, response);
    return GetSliversError::commandErrorFromBlazeError(err);
}

GetSliverInfosError::Error SliverManager::processGetSliverInfos(const GetSliverInfosRequest& request, GetSliverInfosResponse& response, const Message*)
{
    BlazeRpcError err = ERR_OK;    
    SliverNamespace sliverNamespace = BlazeRpcComponentDb::getComponentIdByName(request.getSliverNamespace());
    if (sliverNamespace != Component::INVALID_COMPONENT_ID)
    {
        SliverManager::SliverNamespaceData* namespaceData = gSliverManager->getOrCreateSliverNamespace(sliverNamespace);
        if (namespaceData != nullptr)
        {
            const SliverIdList &list = request.getSliverIdList();
            bool fetchAllSliver = false;
            uint16_t sliverStartIndex = 0;
            uint16_t sliverSize = list.size();
            if (sliverSize == 0)
            {               
                // return sliverInfo for all slivers if list is empty
                sliverSize = namespaceData->mInfo.getConfig().getSliverCount() + 1;
                sliverStartIndex = 1;
                fetchAllSliver = true;
            }
            
            for (uint16_t i = sliverStartIndex; i < sliverSize; i++)
            {
                SliverId id = (fetchAllSliver) ? i : list.at(i);
                SliverPtr sliver = getSliver(sliverNamespace, id);
                if (sliver != nullptr)
                {
                    SliverInfo *info = response.getSliverInfoMap().allocate_element();
                    info->setSliverId(id);
                    info->setSliverNamespace(sliverNamespace);
                    info->getState().setReason(sliver->getState().getReason());
                    info->getState().setRevisionId(sliver->getState().getRevisionId());
                    info->getState().setSliverInstanceId(sliver->getState().getSliverInstanceId());
                    response.getSliverInfoMap()[id] = info;
                }
                else
                {
                    BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.processGetSliverInfos: Sliver(" << id << ") not found in SliverNamespace(" << request.getSliverNamespace());
                }
            }
        }
        else
        {
            err = ERR_SYSTEM;
        }
    }
    else
    {
        BLAZE_ERR_LOG(SliversSlave::LOGGING_CATEGORY, "SliverManager.processGetSliverInfos: SliverNamespace(" << request.getSliverNamespace() << ") not found.");
        err = ERR_SYSTEM;
    }
    
    return GetSliverInfosError::commandErrorFromBlazeError(err);
}

SubmitSliverNamespaceStatesError::Error SliverManager::processSubmitSliverNamespaceStates(const SubmitSliverNamespaceStatesRequest& request, const Message*)
{
    BlazeRpcError err = mSliverCoordinator.updateSliverNamespaceStates(request.getInstanceId(), request.getStateBySliverNamespaceMap());
    return SubmitSliverNamespaceStatesError::commandErrorFromBlazeError(err);
}

CheckLocalHealthError::Error SliverManager::processCheckLocalHealth(const CheckHealthRequest& request, CheckHealthResponse& response, const Message* message)
{
    StringBuilder status;
    generateLocalHealthReport(status);
    response.setStatus(status.get());

    return CheckLocalHealthError::ERR_OK;
}

CheckHealthError::Error SliverManager::processCheckHealth(const CheckHealthRequest& request, CheckHealthResponse& response, const Message* message)
{
    StringBuilder status;

    generateLocalHealthReport(status);

    InstanceIdList instances;
    getComponentInstances(instances, false, true);
    MultiRequestResponseHelper<CheckHealthResponse, void> responses(instances, false);
    BlazeError result = sendMultiRequest(Blaze::SliversSlave::CMD_INFO_CHECKLOCALHEALTH, &request, responses);
    if (result != ERR_OK)
        status << "Some instance(s) returned " << result << ".  Details below.\n";

    for (Component::MultiRequestResponseList::const_iterator multiIt = responses.begin(), multiEnd = responses.end(); multiIt != multiEnd; ++multiIt)
    {
        const Component::MultiRequestResponse& multiResp = *(*multiIt);
        if (multiResp.error == ERR_OK)
        {
            const CheckHealthResponse& instanceResponse = (const CheckHealthResponse&)(*multiResp.response);
            status << instanceResponse.getStatus();
        }
        else
        {
            status << RoutingOptions(multiResp.instanceId) << " returned " << BlazeError(multiResp.error) << "\n";
        }
    }

    for (SliverNamespaceDataBySliverNamespaceMap::iterator it = mSliverNamespaceMap.begin(), end = mSliverNamespaceMap.end(); it != end; ++it)
    {
        SliverNamespaceData* namespaceData = it->second;

        StringBuilder tableStr;
        generateAsciiListenerTable(*namespaceData, tableStr);
        status << tableStr.get() << "\n";
    }

    response.setStatus(status.get());

    return CheckHealthError::ERR_OK;
}

SliverId SliverManager::getValidSliverID(uint16_t sliverCount)
{
    return (SliverId)(gRandom->getRandomNumber(sliverCount) + SLIVER_ID_MIN);
}

} // namespace Blaze
