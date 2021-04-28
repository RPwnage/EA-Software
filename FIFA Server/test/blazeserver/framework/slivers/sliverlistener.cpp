/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/slivers/sliverlistener.h"
#include "framework/slivers/slivermanager.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"

#include "EASTL/algorithm.h"

namespace Blaze
{

SliverListener::SliverListener(SliverNamespace sliverNamespace) :
    mSliverNamespace(sliverNamespace),
    mSyncFiberGroupId(Fiber::INVALID_FIBER_GROUP_ID),
    mStopCurrentUpdateOperation(false),
    mShutdown(false)
{
}

SliverListener::~SliverListener()
{
}

void SliverListener::shutdown()
{
    mShutdown = true;
    if (mSyncFiberGroupId != Fiber::INVALID_FIBER_GROUP_ID)
    {
        Fiber::join(mSyncFiberGroupId);
    }
}

void SliverListener::notifyUpdateAssignedSlivers(const SliverPartitionInfo& partitionInfo)
{
    if (mShutdown)
        return;

    EA_ASSERT(mSyncSliverCb.isValid() && mDesyncSliverCb.isValid());

    if (!Fiber::isCurrentlyBlockable())
    {
        // This code path is basically always the one followed, because this method
        // is called from the SliverManagers notification handler.

        Fiber::CreateParams params;
        params.groupId = getSyncFiberGroupId();
        gSelector->scheduleFiberCall<SliverListener, const SliverPartitionInfo&>(
            this, &SliverListener::notifyUpdateAssignedSlivers, partitionInfo, "SliverListener::notifyUpdateAssignedSlivers", params);

        return;
    }

    partitionInfo.copyInto(mPartitionInfo);

    mStopCurrentUpdateOperation = true;
    BlazeRpcError err = mUpdateAssignedSliversMutex.lock();
    mStopCurrentUpdateOperation = false;
    if (err != ERR_OK)
    {
        EA_FAIL();
    }

    SliverEventInfo eventInfo;
    eventInfo.setSliverNamespace(mSliverNamespace);
    eventInfo.setSourceInstanceId(gController->getInstanceId());

    SliverIdSet assignedIds;
    SliverIdSet desyncIds;
    SliverIdSet syncIds;
    assignedIds.reserve(mPartitionInfo.getUpperBoundSliverId() - mPartitionInfo.getLowerBoundSliverId() + 1);
    desyncIds.reserve(mSyncedSliverIds.size());
    syncIds.reserve(assignedIds.size());

    for (SliverId sliverId = mPartitionInfo.getLowerBoundSliverId(); sliverId <= mPartitionInfo.getUpperBoundSliverId(); ++sliverId)
    {
        assignedIds.insert(assignedIds.end(), sliverId);
    }
    eastl::set_difference(mSyncedSliverIds.begin(), mSyncedSliverIds.end(), assignedIds.begin(), assignedIds.end(), eastl::inserter(desyncIds, desyncIds.end()));
    eastl::set_difference(assignedIds.begin(), assignedIds.end(), mSyncedSliverIds.begin(), mSyncedSliverIds.end(), eastl::inserter(syncIds, syncIds.end()));

    size_t maxIds = eastl::max(desyncIds.size(), syncIds.size());
    size_t syncActionFrequency = (syncIds.size() > 0 ? maxIds / syncIds.size() : 0);
    size_t desyncActionFrequency = (desyncIds.size() > 0 ? maxIds / desyncIds.size() : 0);

    size_t syncIndex = 0;
    size_t desyncIndex = 0;
    for (size_t index = 0; (index < maxIds) && !mStopCurrentUpdateOperation && !mShutdown; ++index)
    {
        if (!mStopCurrentUpdateOperation && ((syncActionFrequency > 0) && ((index % syncActionFrequency) == 0)) && (syncIndex < syncIds.size()))
        {
            SliverId sliverId = syncIds.at(syncIndex++);

            SliverPtr sliver = gSliverManager->getSliver(mSliverNamespace, sliverId);
            if (sliver != nullptr)
            {
                err = ERR_SYSTEM;
                mSyncSliverCb(sliver, err);
                if (err == ERR_OK)
                {
                    mSyncedSliverIds.insert(sliverId);

                    eventInfo.setSliverId(sliverId);
                    gSliverManager->sendSliverListenerSyncedToAllSlaves(&eventInfo);
                }
                else
                {
                    BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "SliverListener.notifyUpdateAssignedSlivers: The SyncSliver callback returned err(" << ErrorHelp::getErrorName(err) << ") "
                        "when asked to sync SliverId(" << sliverId << ") in SliverNamespace(" << gSliverManager->getSliverNamespaceStr(getSliverNamespace()) << ") "
                        "to the SliverInstanceId(" << sliver->getSliverInstanceId() << ")");
                }
            }
            else
                EA_FAIL(); // This really should never ever happen.
        }

        if (!mStopCurrentUpdateOperation && ((desyncActionFrequency > 0) && ((index % desyncActionFrequency) == 0)) && (desyncIndex < desyncIds.size()))
        {
            SliverId sliverId = desyncIds.at(desyncIndex++);

            eventInfo.setSliverId(sliverId);
            gSliverManager->sendSliverListenerDesyncedToAllSlaves(&eventInfo);

            SliverPtr sliver = gSliverManager->getSliver(mSliverNamespace, sliverId);
            if (sliver != nullptr)
            {
                err = ERR_SYSTEM;
                mDesyncSliverCb(sliver, err);
                if (err == ERR_OK)
                {
                    mSyncedSliverIds.erase(sliverId);
                }
                else
                {
                    BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "SliverListener.notifyUpdateAssignedSlivers: The DesyncSliver callback returned err(" << ErrorHelp::getErrorName(err) << ") "
                        "when asked to sync SliverId(" << sliverId << ") in SliverNamespace(" << gSliverManager->getSliverNamespaceStr(getSliverNamespace()) << ") "
                        "to the SliverInstanceId(" << sliver->getSliverInstanceId() << ")");
                }
            }
            else
                EA_FAIL(); // This really should never ever happen.
        }
    }

    mUpdateAssignedSliversMutex.unlock();
}

void SliverListener::notifySliverInstanceIdChanged(SliverId sliverId)
{
    if (mShutdown)
        return;

    if (!Fiber::isCurrentlyBlockable())
    {
        // This code path is basically always the one followed, because this method
        // is called from the SliverManagers notification handler.

        Fiber::CreateParams params;
        params.groupId = getSyncFiberGroupId();
        gSelector->scheduleFiberCall(this, &SliverListener::notifySliverInstanceIdChanged, sliverId, "SliverListener::notifySliverInstanceIdChanged", params);

        return;
    }

    if (isSliverSynced(sliverId))
    {
        EA_ASSERT(mSyncSliverCb.isValid() && mDesyncSliverCb.isValid());

        SliverPtr sliver = gSliverManager->getSliver(mSliverNamespace, sliverId);
        if (sliver != nullptr)
        {
            BlazeRpcError err = ERR_SYSTEM;
            mSyncSliverCb(sliver, err);
            if (err != ERR_OK)
            {
                BLAZE_ERR_LOG(SliversSlave::LOGGING_CATEGORY, "SliverListener.notifySliverInstanceIdChanged: Call to the SyncSliver callback return err(" << LOG_ENAME(err) << ")");
            }
        }
        else
            EA_FAIL(); // This should really never happen.
    }
}

void SliverListener::syncSibling(SlaveSession& slaveSession)
{
    SliverEventInfo eventInfo;
    eventInfo.setSliverNamespace(mSliverNamespace);
    eventInfo.setSourceInstanceId(gController->getInstanceId());

    for (SliverIdSet::iterator it = mSyncedSliverIds.begin(), end = mSyncedSliverIds.end(); it != end; ++it)
    {
        eventInfo.setSliverId(*it);
        gSliverManager->sendSliverListenerSyncedToSlaveSession(&slaveSession, &eventInfo);
    }
}

} // namespace Blaze
