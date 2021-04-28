/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/slivers/sliver.h"
#include "framework/slivers/slivermanager.h"
#include "framework/connection/connectionmanager.h"
#include "framework/util/timer.h"
#include "framework/component/notification.h"

namespace Blaze
{

// The amount of time (ms) a sliver can be in an unowned or write-locked state before it starts getting reported in calls to Slivers::checkHealth().
const int32_t Sliver::HEALTH_CHECK_GRACE_PERIOD_MS = 30 * 1000;
// The amount of time (ms) a sliver can be in an unowned or write-locked state before attempts to repair its state are initiated.
const int32_t Sliver::UNOWNED_GRACE_PERIOD_MS = 120 * 1000;

void LogIdentity::append(StringBuilder& sb) const { sb << "sliver:" << mSliverId << "/" << BlazeRpcComponentDb::getComponentNameById(mSliverNamespace); }
void LogState::append(StringBuilder& sb) const { sb << mKind << "={" << RoutingOptions(mSliverInstanceId) << ",rev=" << mSliverRevisionId << ",stat=" << BlazeError(mReason) << "}"; }

void intrusive_ptr_add_ref(Sliver* ptr)
{
    ptr->mRefCount++;
}

void intrusive_ptr_release(Sliver* ptr)
{
    if (--ptr->mRefCount == 0)
        delete ptr;
}

Sliver::Sliver(SliverIdentity sliverIdentity) :
    mRefCount(0),
    mNotificationQueueStartTime(),
    mNotificationQueue("Sliver::NotificationQueue"),
    mLastStateUpdateTime(TimeValue::getTimeOfDay()),
    mCurrentImporter(nullptr)
{
    EA_ASSERT(isCompleteIdentity(sliverIdentity));

    mSliverNamespace = GetSliverNamespaceFromSliverIdentity(sliverIdentity);
    mSliverId = GetSliverIdFromSliverIdentity(sliverIdentity);

    mNotificationQueue.setJobFiberStackSize(Fiber::STACK_SMALL);
    mNotificationQueue.setQueueEventCallbacks(
        FiberJobQueue::QueueEventCb(this, &Sliver::notificationQueueFiberStarted),
        FiberJobQueue::QueueEventCb(this, &Sliver::notificationQueueFiberFinished));
}

Sliver::~Sliver()
{
}

bool Sliver::isValidIdentity(SliverIdentity sliverIdentity)
{
    return BlazeRpcComponentDb::isValidComponentId(GetSliverNamespaceFromSliverIdentity(sliverIdentity));
}

bool Sliver::isCompleteIdentity(SliverIdentity sliverIdentity)
{
    return BlazeRpcComponentDb::isValidComponentId(GetSliverNamespaceFromSliverIdentity(sliverIdentity)) && (GetSliverIdFromSliverIdentity(sliverIdentity) != INVALID_SLIVER_ID);
}

BlazeRpcError Sliver::waitForAccess(AccessRef& access, const char8_t* context)
{
    if (access.hasSliver())
    {
        BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "Sliver.waitForAccess: AccessRef already has a sliver. "
            "SliverNamespace(" << BlazeRpcComponentDb::getComponentNameById(getSliverNamespace()) << "), "
            "SliverId(" << getSliverId() << "), "
            "SliverInstanceId(" << access.getSliverInstanceId() << ")");
        return ERR_SYSTEM;
    }

    BlazeError err;
    do
    {
        err = mAccessRWMutex.lockForRead(access.mReadLockContext, context);
        if (err == ERR_OK)
        {
            if (getSliverInstanceId() != INVALID_INSTANCE_ID)
            {
                if (!isOwnedByCurrentInstance())
                    err = gSliverManager->waitForSlaveSession(getSliverInstanceId(), context);

                if (err == ERR_OK)
                {
                    access.setSliver(*this);
                    break;
                }

                // We need to yield here because of the intricacies of the sliver access's FiberReadWriteMutex.
                // See the comment in SliverManager::updateSliverState() for more details.
                Fiber::yield();
            }
            // unlock and wait
            mAccessRWMutex.unlockForRead(access.mReadLockContext);
        }

        // It is important that we break out of this while loop *before* calling waitForOwnerId() if the
        // fiber has already been canceled.  This to preserve the value of 'err' for both logging and the return value.
        if (Fiber::isCurrentlyCancelled())
            break;

        err = waitForOwnerId(context);
    }
    while (err == ERR_OK);

    if ((err != ERR_OK) && (err != ERR_CANCELED))
    {
        StringBuilder debugInfo;
        fillDebugInfo(debugInfo);
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Sliver.waitForAccess: Failed to get access to a sliver, "
            "err(" << ErrorHelp::getErrorName(err) << ").\n" << debugInfo.get());
    }

    return err;
}

bool Sliver::getPriorityAccess(AccessRef& access)
{
    if (access.hasSliver())
    {
        BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "Sliver.getPriorityAccess: AccessRef already has a sliver; "
            "namespace=(" << BlazeRpcComponentDb::getComponentNameById(getSliverNamespace()) << "), "
            "SliverId(" << getSliverId() << "), "
            "SliverInstanceId(" << access.getSliverInstanceId() << ")");
        return false;
    }

    bool result = mAccessRWMutex.priorityLockForRead(access.mReadLockContext);
    if (result)
        access.setSliver(*this);
    return result;
}

bool Sliver::isOwnedByCurrentInstance() const
{
    return (mState.getSliverInstanceId() == gController->getInstanceId());
}

BlazeError Sliver::updateStateToPendingState()
{
    BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(*this) << "->updateStateToPendingState: Wait for write lock; " << LogCurrentState(*this) << ", " << LogPendingState(*this));

    Timer waitTimer;
    BlazeError err = mAccessRWMutex.lockForWrite();
    if (err == ERR_OK)
    {
        BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(*this) << "->updateStateToPendingState: Obtained write lock, proceeding with state update; " << LogCurrentState(*this) << ", " << LogPendingState(*this)); 

        gSliverManager->updateSliverOwnershipIndiciesAndCounts(getSliverIdentity(), mState, mPendingStateUpdate);
        mLastStateUpdateTime = TimeValue::getTimeOfDay();
        mPendingStateUpdate.copyInto(mState);
        if (mState.getSliverInstanceId() != INVALID_INSTANCE_ID)
            mWaitingForOwnerList.signal(ERR_OK);
        mAccessRWMutex.unlockForWrite();

        BLAZE_TRACE_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(*this) << "->updateStateToPendingState: Done updating state, released the write lock; " << LogCurrentState(*this) << ", " << LogPendingState(*this)); 
    }
    else
    {
        BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(*this) << "->updateStateToPendingState: lockForWrite() returned " << err << " after waiting " << waitTimer << "; " << LogCurrentState(*this) << ", " << LogPendingState(*this)); 
    }

    return err;
}

BlazeError Sliver::waitForOwnerId(const char8_t* context)
{
    if (gController->isShuttingDown() || (gController->getState() == Controller::State::ERROR_STATE))
        return ERR_CANCELED;

    if (mState.getSliverInstanceId() != INVALID_INSTANCE_ID)
        return ERR_OK;

    if (gController->isDraining() && (gSliverManager->getComponentInstanceCount(false) == 0))
        return ERR_TIMEOUT;

    Timer waitTimer;
    BlazeError err = mWaitingForOwnerList.wait(context);
    if ((err != ERR_OK) && (err != ERR_CANCELED))
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, LogIdentity(*this) << "->waitForOwnerId: Still no owner after " << waitTimer << "; err=" << err << ", " << LogCurrentState(*this) << ", " << LogPendingState(*this)); 
    }

    return err;
}

void Sliver::notificationQueueFiberStarted()
{
    gSliverManager->incNotificationQueueFibersCount();

    mNotificationQueueStartTime = TimeValue::getTimeOfDay();

    BlazeRpcError err = waitForAccess(mNotificationAccess, "Sliver::notificationQueueFiberStarted");
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Sliver.notificationQueueFiberStarted: Failed to get sliver access for "
            "SliverNamespace(" << BlazeRpcComponentDb::getComponentNameById(getSliverNamespace()) << "), SliverId(" << getSliverId() << ")");
    }

    mNotificationQueueStartTime = 0;
}

void Sliver::notificationQueueFiberFinished()
{
    mNotificationAccess.release();

    gSliverManager->decNotificationQueueFibersCount();
}

bool Sliver::accessWouldBlock()
{
    return ((mState.getSliverInstanceId() == INVALID_INSTANCE_ID) ||
        ((mState.getSliverInstanceId() != gController->getInstanceId()) && gController->getConnectionManager().getSlaveSessionByInstanceId(mState.getSliverInstanceId()) == nullptr) ||
        (mAccessRWMutex.isLockedForWrite() && !mAccessRWMutex.isWriteLockOwnedByCurrentFiberStack()));
}

void Sliver::sendNotification(Notification& notification)
{
    if (EA_UNLIKELY(gController->isShuttingDown()))
    {
        eastl::string callstackStr;
        CallStack::getCurrentStackSymbols(callstackStr);
        BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "Sliver.sendNotification: Cannot send a notification to a sliver after shutdown has begun. Details: "
            "SliverNamespace(" << BlazeRpcComponentDb::getComponentNameById(getSliverNamespace()) << "), "
            "SliverId(" << getSliverId() << "), "
            "Context(" << Fiber::getCurrentContext() << "), "
            "Callstack:\n" << callstackStr.c_str());
        return;
    }

    if (!isOwnedByCurrentInstance() && accessWouldBlock())
    {
        NotificationPtr notificationSnapshot = notification.createSnapshot();

        if (mNotificationQueueStartTime != TimeValue(0))
        {
            TimeValue startDelay = TimeValue::getTimeOfDay() - mNotificationQueueStartTime;
            if (startDelay.getSec() > 20)
            {
                BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Sliver.sendNotification: Queue startup has taken " << startDelay.getSec() << " seconds. " <<
                    "This is may delay the incoming notification (" << BlazeRpcComponentDb::getNotificationNameById(notification.getComponentId(), notification.getNotificationId())  << "). " <<
                    "SliverNamespace(" << BlazeRpcComponentDb::getComponentNameById(getSliverNamespace()) << "), SliverId(" << getSliverId() << ")");
            }
        }

        mNotificationQueue.queueFiberJob<Sliver, NotificationPtr>
            (this, &Sliver::fulfillSendNotificationAsync, notificationSnapshot, "Sliver::fulfillSendNotificationAsync");
    }
    else
    {
        fulfillSendNotification(notification);
    }
}

void Sliver::fulfillSendNotificationAsync(NotificationPtr notification)
{
    fulfillSendNotification(*notification);
}

void Sliver::fulfillSendNotification(Notification& notification)
{
    if (mState.getSliverInstanceId() == INVALID_INSTANCE_ID)
    {
        BLAZE_WARN_LOG(SliversSlave::LOGGING_CATEGORY, "Sliver.fulfillSendNotification: Notification send attempt aborted due to no available "
            "sliver owner for SliverNamespace(" << BlazeRpcComponentDb::getComponentNameById(getSliverNamespace()) << "), SliverId(" << getSliverId() << ")");
        return;
    }

    if (mState.getSliverInstanceId() == gController->getInstanceId())
    {
        notification.executeLocal();
    }
    else
    {
        SlaveSessionPtr slaveSession = gController->getConnectionManager().getSlaveSessionByInstanceId(mState.getSliverInstanceId());
        if (slaveSession != nullptr)
        {
            slaveSession->sendNotification(notification);
        }
        else
        {
            BLAZE_ASSERT_LOG(SliversSlave::LOGGING_CATEGORY, "Sliver.fulfillSendNotification: Failed to get the SlaveSession for instanceId(" << mState.getSliverInstanceId() << ")");
        }
    }
}

void Sliver::fillDebugInfo(StringBuilder& sb) const
{
    StringBuilder accessRWMutexDebugInfo;
    mAccessRWMutex.fillDebugInfo(accessRWMutexDebugInfo);

    sb << "Debug Info for SliverId(" << getSliverId() << ") in SliverNamespace(" << gSliverManager->getSliverNamespaceStr(getSliverNamespace()) << "):\n";
    sb << "  Context: " << Fiber::getCurrentContext() << "\n";
    sb << "  Cancelled: " << (Fiber::isCurrentlyCancelled() ? "Yes" : "No") << "\n";
    sb << "  Member Values:\n";
    sb << "    RefCount: " << mRefCount << "\n";
    sb << "    WaitingForOwnerList::getCount: " << mWaitingForOwnerList.getCount() << "\n";
    sb << "    NotificationQueue::getJobQueueSize: " << mNotificationQueue.getJobQueueSize() << "\n";

    sb << "  Current State:\n";
    sb << SbFormats::PrefixAppender("    ", (StringBuilder() << mState).get()) << "\n";

    sb << "  Pending State:\n";
    sb << SbFormats::PrefixAppender("    ", (StringBuilder() << mPendingStateUpdate).get()) << "\n";

    sb << "  SliverAccess Details:\n";
    sb << SbFormats::PrefixAppender("    ", accessRWMutexDebugInfo.get()) << "\n";
    sb << "\n\n";
}

void Sliver::setCurrentImporter(OwnedSliver* currentImporter)
{
    // If this instance tries to bind itself to this Sliver twice, then something is really really
    // messed up, and this instance should probably just crash. I cannot think of a way to recover with
    // any sort of grace if we get into this sort of situation.
    EA_ASSERT(currentImporter != mCurrentImporter && (currentImporter == nullptr || mCurrentImporter == nullptr));
    mCurrentImporter = currentImporter;
}

bool Sliver::isImportingToCurrentInstance() const
{
    return (mCurrentImporter != nullptr);
}

bool Sliver::isUnhealthy() const
{
    TimeValue now = TimeValue::getTimeOfDay();

    // A sliver is considered "unhealthy" if...
    // 1. It has been un-owned for more than HEALTH_CHECK_GRACE_PERIOD_MS.
    // 2. There are people (fibers) that have been waiting for a read lock for longer than the standard command timeout.
    // 3. The write lock has been held for more than HEALTH_CHECK_GRACE_PERIOD_MS.  This really shouldn't happen because the write lock
    //    doesn't protect any blocking calls.
    return
        ((mState.getSliverInstanceId() == INVALID_INSTANCE_ID) && (now - getLastStateUpdateTime() > HEALTH_CHECK_GRACE_PERIOD_MS * 1000)) ||
        ((mAccessRWMutex.getReaderCount() > 0) && (now - mAccessRWMutex.getOldestReadLockTime() > gController->getInternalEndpoint()->getCommandTimeout())) ||
        (mAccessRWMutex.isLockedForWrite() && (now - mAccessRWMutex.getWriteLockObtainedTime() > HEALTH_CHECK_GRACE_PERIOD_MS * 1000));
}

bool Sliver::checkHealth(StringBuilder& healthReport) const
{
    if (isUnhealthy())
    {
        fillDebugInfo(healthReport);
        return false; // unhealthy
    }

    return true; // healthy
}

} // namespace Blaze
