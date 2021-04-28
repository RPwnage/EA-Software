/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/slivers/sliverowner.h"
#include "framework/slivers/slivermanager.h"
#include "framework/controller/controller.h"
#include "framework/uniqueid/uniqueidmanager.h"
#include "framework/util/timer.h"

#include "EASTL/algorithm.h"

namespace Blaze
{

SliverOwnerMetrics::SliverOwnerMetrics(SliverOwner& owner)
    : mCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::sliver_namespace, gSliverManager->getSliverNamespaceStr(owner.getSliverNamespace())))
    , mGaugeActiveImportOperations(mCollection, "slivers.activeImportOperations")
    , mTotalImportsSucceeded(mCollection, "slivers.importsSucceeded")
    , mTotalImportsFailed(mCollection, "slivers.importsFailed")
    , mImportTimer(mCollection, "slivers.imports")
    , mGaugeActiveExportOperations(mCollection, "slivers.activeExportOperations")
    , mTotalExportsSucceeded(mCollection, "slivers.exportsSucceeded")
    , mTotalExportsFailed(mCollection, "slivers.exportsFailed")
    , mExportTimer(mCollection, "slivers.exports")
    , mGaugeOwnedSliverCount(mCollection, "slivers.ownedSliverCount", [&owner]() { return owner.getOwnedSliverCount(); })
    , mGaugeOwnedObjectCount(mCollection, "slivers.ownedObjectCount", [&owner]() { return owner.getOwnedObjectCount(); })
{
}


SliverOwner::SliverOwner(SliverNamespace sliverNamespace) :
    mSliverNamespace(sliverNamespace),
    mHandleDrainRequestFiberId(Fiber::INVALID_FIBER_ID),
    mHandleDrainRequestTimerId(INVALID_TIMER_ID),
    mUpdateCoalescePeriod(50 * 1000), // hard-coded default of 50ms, overriden during configuration
    mMetrics(*this)
{
}

SliverOwner::~SliverOwner()
{
    EA_ASSERT(mHandleDrainRequestFiberId == Fiber::INVALID_FIBER_ID);

    if (mHandleDrainRequestTimerId != INVALID_TIMER_ID)
    {
        BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner.~SliverOwner: The SliverOwner for SliverNamespace(" << gSliverManager->getSliverNamespaceStr(mSliverNamespace) << ") is being destroyed "
            "while it has an outstanding 'eject all slivers' task. This should never happen. Cancelling the timer.");

        gSelector->cancelTimer(mHandleDrainRequestTimerId);
        mHandleDrainRequestTimerId = INVALID_TIMER_ID;
    }
}

void SliverOwner::setCallbacks(const ImportSliverCb& importCallback, const ExportSliverCb& exportCallback)
{
    mImportCallback = importCallback;
    mExportCallback = exportCallback;
}

void SliverOwner::setUpdateCoalescePeriod(TimeValue period)
{
    mUpdateCoalescePeriod = period;
}

const char8_t* SliverOwner::getSliverNamespaceStr() const
{
    return gSliverManager->getSliverNamespaceStr(mSliverNamespace);
}

OwnedSliver* SliverOwner::getOwnedSliver(SliverId sliverId)
{
    OwnedSliverBySliverIdMap::iterator it = mOwnedSliverBySliverIdMap.find(sliverId);
    if (it != mOwnedSliverBySliverIdMap.end())
        return it->second;

    return nullptr;
}

OwnedSliver* SliverOwner::getLeastLoadedOwnedSliver()
{
    if (!mOwnedSliverByPriorityMap.empty())
        return mOwnedSliverByPriorityMap.begin()->second;

    return nullptr;
}

int64_t SliverOwner::getOwnedObjectCount() const
{
    int64_t totalObjectCount = 0;
    for (OwnedSliverBySliverIdMap::const_iterator it = mOwnedSliverBySliverIdMap.begin(), end = mOwnedSliverBySliverIdMap.end(); it != end; ++it)
    {
        totalObjectCount += it->second->getLoad();
    }
    return totalObjectCount;
}

void SliverOwner::doImportSliver(SliverId sliverId, SliverRevisionId revisionId)
{
    BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner::doImportSliver: Begin import; sliver:" << sliverId << "/" << getSliverNamespaceStr() << ", revisionId=" << revisionId);

    BlazeRpcError err = ERR_SYSTEM;
    Timer importTimer;

    uint32_t importedObjectCount = 0;

    SliverPtr sliver = gSliverManager->getSliver(mSliverNamespace, sliverId);
    if (sliver != nullptr)
    {
        if ((sliver->getPendingStateUpdate().getRevisionId() < revisionId) ||
            (sliver->getPendingStateUpdate().getReason() == ERR_DISCONNECTED))
        {
            if (sliver->getState().getSliverInstanceId() == INVALID_INSTANCE_ID)
            {
                OwnedSliver* ownedSliver = BLAZE_NEW OwnedSliver(*this, *sliver);

                mMetrics.mGaugeActiveImportOperations.increment();

                // Import the sliver...  This typically calls into the StorageTable which begins the
                // actual work of pulling all the objects from Redis that are pinned to this sliver.
                // 
                // We first call Sliver::setCurrentImporter() with a non-nullptr value in order to ensure that
                // the state of the sliver does not change while the import operation is in progress.
                // See comment on Sliver::mCurrentImporter for detailed explanation.
                sliver->setCurrentImporter(ownedSliver);

                mImportCallback(*ownedSliver, err);

                if (err == ERR_OK)
                {
                    // Add the OwnedSliver to the map of slivers owned by this instance.  This will allow
                    // the OwnedSliver to be returned by a call to getOwnedSliver().
                    mOwnedSliverBySliverIdMap[sliverId] = ownedSliver;

                    sliver->getPendingStateUpdate().setReason(ERR_OK);
                    sliver->getPendingStateUpdate().setRevisionId(revisionId);
                    sliver->getPendingStateUpdate().setSliverInstanceId(gController->getInstanceId());

                    gSliverManager->updateSliverState(sliver);

                    EA_ASSERT(ownedSliver->getBaseSliver().getState().getSliverInstanceId() == gController->getInstanceId());
                    EA_ASSERT(ownedSliver->getBaseSliver().getState().getRevisionId() == revisionId);

                    SliverStateUpdateRequest request;
                    request.setSendConfirmationToInstanceId(INVALID_INSTANCE_ID);
                    request.setSliverInfo(ownedSliver->getBaseSliver());
                    gSliverManager->sendSliverStateUpdateRequiredToRemoteSlaves(&request);

                    // Finally, set the OwnedSlivers priority queue, which will allow it to be returned
                    // by a call to getLeastLoadedOwnedSliver().
                    ownedSliver->setPriorityQueue(&mOwnedSliverByPriorityMap);

                    importedObjectCount = ownedSliver->getLoad();
                }

                // The import operation is done, reset the current importer to nullptr.  This should be
                // done before deleting ownedSliver (below) in the event an error occured during import.
                sliver->setCurrentImporter(nullptr);

                if (err != ERR_OK)
                {
                    BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner::doImportSliver: Import failed; sliver:" << sliverId << "/" << getSliverNamespaceStr() << ", err=" << BlazeError(err));
                    delete ownedSliver;
                }

                mMetrics.mGaugeActiveImportOperations.decrement();
            }
            else
                BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner::doImportSliver: Cannot import sliver:" << sliverId << "/" << getSliverNamespaceStr() << " due to ownership conflict; "
                    "current state[instanceId=" << sliver->getState().getSliverInstanceId() << ", revisionId=" << sliver->getState().getRevisionId() << "], "
                    "pending state[instanceId=" << sliver->getPendingStateUpdate().getSliverInstanceId() << ", revisionId=" << sliver->getPendingStateUpdate().getRevisionId() << "], "
                    "incoming revisionId=" << revisionId);
        }
        else
            BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner::doImportSliver: Ignoring request to import sliver:" << sliverId << "/" << getSliverNamespaceStr() << " due to a stale revisionId; "
                "current state[instanceId=" << sliver->getState().getSliverInstanceId() << ", revisionId=" << sliver->getState().getRevisionId() << "], "
                "pending state[instanceId=" << sliver->getPendingStateUpdate().getSliverInstanceId() << ", revisionId=" << sliver->getPendingStateUpdate().getRevisionId() << "], "
                "incoming revisionId=" << revisionId);
    }

    if (err == ERR_OK)
        mMetrics.mTotalImportsSucceeded.increment();
    else
        mMetrics.mTotalImportsFailed.increment();

    mMetrics.mImportTimer.record(importTimer.getInterval());

    BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner::doImportSliver: Done import; sliver:" << sliverId << "/" << getSliverNamespaceStr() << ", revisionId=" << revisionId << ", "
        "total objects=" << importedObjectCount << ", duration=" << importTimer << ", err=" << BlazeError(err));

    // The entry is pretty much guaranteed to be there, but lets do a find anyway (futureproof)
    FiberWaitListBySliverId::iterator it = mImportFiberWaitListBySliverId.find(sliverId);
    if (it != mImportFiberWaitListBySliverId.end())
    {
        it->second.signal(err);
        if (it->second.getCount() == 0)
            mImportFiberWaitListBySliverId.erase(sliverId);
    }
}

BlazeRpcError SliverOwner::importSliver(SliverId sliverId, SliverRevisionId revisionId)
{
    BlazeRpcError err = ERR_OK;

    OwnedSliver* ownedSliver = getOwnedSliver(sliverId);
    if ((ownedSliver == nullptr) || !ownedSliver->isInPriorityQueue())
    {
        FiberWaitListBySliverId::insert_return_type inserted = mImportFiberWaitListBySliverId.insert(sliverId);
        Fiber::WaitList& importWaitList = inserted.first->second;
        if (inserted.second)
            gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay(), this, &SliverOwner::doImportSliver, sliverId, revisionId, "SliverOwner::doImportSliver");

        // The timer call above never blocks, and so it's safe to access importWaitList, even though importWaitList may be invalid
        // after the wait() call because it will get deleted by doImportSliver().
        err = importWaitList.wait("SliverOwner::importSliver");
    }

    return err;
}

void SliverOwner::doExportSliver(OwnedSliver* ownedSliver, SliverRevisionId revisionId)
{
    Fiber::yield();

    BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner.doExportSliver: Begin export; sliver:" << ownedSliver->getSliverId() << "/" << getSliverNamespaceStr() << ", revisionId=" << revisionId << ", total objects=" << ownedSliver->getLoad());

    BlazeRpcError err = ERR_OK;

    Timer exportTimer;
    mMetrics.mGaugeActiveExportOperations.increment();

    // Clear out the OwnedSliver's priority queue.  The OwnedSliver will remove itself from the
    // list of available slivers returned by a call to getLeastLoadedOwnedSliver()
    ownedSliver->setPriorityQueue(nullptr);

    ownedSliver->getBaseSliver().mAccessRWMutex.lockForWrite();

    SliverInfo sliverInfo;
    sliverInfo.setSliverNamespace(mSliverNamespace);
    sliverInfo.setSliverId(ownedSliver->getSliverId());
    sliverInfo.getState().setRevisionId(revisionId);
    sliverInfo.getState().setSliverInstanceId(INVALID_INSTANCE_ID);

    gSliverManager->broadcastAndConfirmUpdateSliverState(ownedSliver->getBaseSliver(), sliverInfo);

    // Remove the OwnedSliver from the map of slivers owned by this instance.  It will no longer
    // be returned by a call to getOwnedSliver().
    mOwnedSliverBySliverIdMap.erase(ownedSliver->getSliverId());

    ownedSliver->getBaseSliver().mAccessRWMutex.unlockForWrite();

    // Scope for BlockingSuppressionAutoPtr
    {
        Fiber::BlockingSuppressionAutoPtr suppressBlockingPtr;

        // Call the callback.
        mExportCallback(*ownedSliver, err);
        if (err != ERR_OK)
        {
            BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner.doExportSliver: The export callback for SliverId(" << ownedSliver->getSliverId() << ") "
                "in SliverNamespace(" << gSliverManager->getSliverNamespaceStr(mSliverNamespace) << ") returned err(" << ErrorHelp::getErrorName(err) << ")");
        }

        if (err == ERR_OK)
            mMetrics.mTotalExportsSucceeded.increment();
        else
            mMetrics.mTotalExportsFailed.increment();
    }

    mMetrics.mGaugeActiveExportOperations.decrement();

    mMetrics.mExportTimer.record(exportTimer.getInterval());

    ownedSliver->getExportCompletedWaitList().signal(ERR_OK); // Intentionally pass ERR_OK here, rather than 'err'.  The waiters check the actual state of things rather than rely on a error code.

    BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner.doExportSliver: Done export; sliver:" << ownedSliver->getSliverId() << "/" << getSliverNamespaceStr() << ", duration=" << exportTimer << ", err=" << BlazeError(err));

    delete ownedSliver;
}

BlazeRpcError SliverOwner::exportSliver(SliverId sliverId, SliverRevisionId revisionId)
{
    BlazeRpcError err;

    OwnedSliver* ownedSliver = getOwnedSliver(sliverId);
    if (ownedSliver != nullptr)
    {
        if (ownedSliver->isInPriorityQueue())
            gSelector->scheduleFiberCall(this, &SliverOwner::doExportSliver, ownedSliver, revisionId, "SliverOwner::doExportSliver");

        err = ownedSliver->getExportCompletedWaitList().wait("SliverOwner::exportSliver");
    }
    else
    {
        BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner.exportSliver: sliver:" << sliverId << "/" << getSliverNamespaceStr() << " is not owned by this instance");
        err = ERR_OK; // Return ERR_OK because the coordinator wants us to *not* own this sliver, and we in fact do *not* own the sliver.
    }

    return err;
}

void SliverOwner::syncSibling(SlaveSession& slaveSession)
{
    SliverStateUpdateRequest request;
    request.setSendConfirmationToInstanceId(INVALID_SLIVER_INSTANCE_ID);
    for (OwnedSliverBySliverIdMap::iterator it = mOwnedSliverBySliverIdMap.begin(), end = mOwnedSliverBySliverIdMap.end(); it != end; ++it)
    {
        // Not being in the priority queue means this OwnedSliver is in the process of
        // being exported, so we don't want to tell other instances that we own it.
        if (!it->second->isInPriorityQueue())
            continue;

        request.setSliverInfo(it->second->getBaseSliver());
        gSliverManager->sendSliverStateUpdateRequiredToSlaveSession(&slaveSession, &request);
    }
}

void SliverOwner::repairSibling(InstanceId siblingInstanceId, const SliverIdList& sliverIdList)
{
    SliverStateUpdateRequest request;
    request.setSendConfirmationToInstanceId(INVALID_SLIVER_INSTANCE_ID);
    for (SliverId sliverId : sliverIdList)
    {
        OwnedSliver* ownedSliver = getOwnedSliver(sliverId);

        // Not being in the priority queue means this OwnedSliver is in the process of
        // being exported, so we don't want to tell other instances that we own it.
        if ((ownedSliver == nullptr) || !ownedSliver->isInPriorityQueue())
            continue;

        request.setSliverInfo(ownedSliver->getBaseSliver());
        gSliverManager->sendSliverStateUpdateRequiredToInstanceById(siblingInstanceId, &request);
    }
}

void SliverOwner::beginDraining()
{
    if (mHandleDrainRequestTimerId == INVALID_TIMER_ID)
    {
        mHandleDrainRequestTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay(), this, &SliverOwner::executeDrainTasks, "SliverOwner::executeDrainTasks");
    }
}

void SliverOwner::executeDrainTasks()
{
    mHandleDrainRequestFiberId = Fiber::getCurrentFiberId();

    if ((getOwnedSliverCount() == 0) &&
        (mMetrics.mGaugeActiveImportOperations.get() == 0) && 
        (mMetrics.mGaugeActiveExportOperations.get() == 0))
    {
        // This check basically says, if we don't own any slivers right now, and we're not actively importing/exporting anything: bypass draining.
        mHandleDrainRequestTimerId = INVALID_TIMER_ID;
    }
    else
    {
        // Safeguard in case this was called outside of a scheduled fiber timer call (which shouldn't be happening).
        gSelector->cancelTimer(mHandleDrainRequestTimerId);

        EjectSliversRequest request;
        request.setSliverNamespace(getSliverNamespaceStr());
        request.setFromInstanceId(gController->getInstanceId());

        BlazeError err = gSliverManager->ejectSlivers(request);
        if (err != ERR_OK)
        {
            if (err == ERR_DISCONNECTED)
            {
                // This scenario is expected. The SliversCoordinator component (normally located on the configMaster) is not
                // currently available, probably because it is restarting.
                BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner.executeDrainTasks: Request to eject owned slivers for "
                    "SliverNamespace(" << getSliverNamespaceStr() << ") failed with err(" << err << "). Retrying in " << EJECT_RETRY_INTERVAL_MS << "ms.");
            }
            else
            {
                // All other errors would generally be unexpected.
                BLAZE_ERR_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner.executeDrainTasks: Request to eject owned slivers for "
                    "SliverNamespace(" << getSliverNamespaceStr() << ") failed with err(" << err << "). Retrying in " << EJECT_RETRY_INTERVAL_MS << "ms.");
            }

            mHandleDrainRequestTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (EJECT_RETRY_INTERVAL_MS * 1000), this, &SliverOwner::executeDrainTasks, "SliverOwner::executeDrainTasks");
        }
        else
        {
            BLAZE_INFO_LOG(SliversSlave::LOGGING_CATEGORY, "SliverOwner.executeDrainTasks: Successfully requested all slivers be ejected from this instance for SliverNamespace(" << getSliverNamespaceStr() << ")");
            mHandleDrainRequestTimerId = INVALID_TIMER_ID;
        }
    }

    mHandleDrainRequestFiberId = Fiber::INVALID_FIBER_ID;
}

} // namespace Blaze
