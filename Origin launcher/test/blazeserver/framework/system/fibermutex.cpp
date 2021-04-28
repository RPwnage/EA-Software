/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/fibermutex.h"
#include "framework/system/fibermanager.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/**
 * Most component-level mutexes *must* be registered because a blocking fiber stack holding a mutex should never acquire another
 * to avoid deadlocks. The exception are specific framework-level mutexes that are always taken/released in a guaranteed order
 * and are thus fibers that take them can be excempt from the one-mutex-per-blocking stack requirement.
 * */
FiberMutex::FiberMutex(bool registered)
    : mCurrentFiberId(Fiber::INVALID_FIBER_ID),
      mLockDepth(0),
      mRegistered(registered)
{
}

FiberMutex::~FiberMutex()
{
    EA_ASSERT(mLockDepth == 0);
}

BlazeRpcError FiberMutex::lock()
{
    return lockWithTimeout(EA::TDF::TimeValue(0));
}

BlazeRpcError FiberMutex::lockWithTimeout(EA::TDF::TimeValue waitTimeout, const char8_t* context)
{
    BlazeRpcError err = ERR_OK;
    if (mCurrentFiberId == Fiber::getFiberIdOfBlockingStack())
    {
        mLockDepth++;
        return err;
    }

    TimeValue absoluteTimeout = (waitTimeout == EA::TDF::TimeValue(0) ? 0 : TimeValue::getTimeOfDay() + waitTimeout);

    if ((mCurrentFiberId != Fiber::INVALID_FIBER_ID) || !mWaitersQueue.empty())
    {
        EventHandleNode waitHandle = Fiber::getNextEventHandle();
        mWaitersQueue.push(waitHandle);
        err = Fiber::wait(waitHandle, (context == nullptr ? "FiberMutex::lock" : context), absoluteTimeout);

        if (err != ERR_OK)
        {
            EventHandlePriorityQueue::container_type::iterator itr = mWaitersQueue.get_container().begin();
            EventHandlePriorityQueue::container_type::iterator end = mWaitersQueue.get_container().end();
            for(; itr != end; ++itr)
            {
                if (itr->eventHandle == waitHandle.eventHandle)
                {
                    //Remove us
                    mWaitersQueue.get_container().erase(itr);
                    break;
                }
            }

            eastl::string lockStackTrace;
            mLockCallstack.getSymbols(lockStackTrace);
            eastl::string unlockStackTrace;
            mUnlockCallstack.getSymbols(unlockStackTrace);

            if (Fiber::isCurrentlyCancelled())
            {
                BLAZE_ASSERT_LOG(Log::SYSTEM, "FiberMutex.lock: "
                    "Failed to get the lock currently owned by fiber(" << mCurrentFiberId << ") because this fiber(" << Fiber::getCurrentFiberId() << ") has been cancelled. "
                    "The call to Fiber::getAndWait() returned error " << ErrorHelp::getErrorName(err));
            }
            else
            {
                BLAZE_ASSERT_LOG(Log::SYSTEM, "FiberMutex.lock: "
                    "Failed to get the lock currently owned by fiber id(" << mCurrentFiberId << ") within waitTimeout(" << waitTimeout << ") time. "
                    "The call to Fiber::getAndWait() returned error " << ErrorHelp::getErrorName(err));
            }

            return err;
        }
    }

    mCurrentFiberId = Fiber::getFiberIdOfBlockingStack();
    if (mRegistered)
    {
        gFiberManager->addBlockingFiberStack(mCurrentFiberId);
    }

    mLockDepth = 1;

    assign();

    mLockCallstack.getStack();

    return err;
}

void FiberMutex::unlock()
{
    EA_ASSERT(mCurrentFiberId == Fiber::getFiberIdOfBlockingStack());
    EA_ASSERT(mLockDepth > 0);

    if (--mLockDepth == 0)
    {
        release();
    }
}

void FiberMutex::assignInternal()
{
    // nothing to do here!
}

void FiberMutex::releaseInternal()
{
    if (mRegistered)
    {
        gFiberManager->removeBlockingFiberStack(mCurrentFiberId);
    }
    mCurrentFiberId = Fiber::INVALID_FIBER_ID;
    mLockDepth = 0;

    mUnlockCallstack.getStack();

    signalNext();
}

void FiberMutex::signalNext()
{
    if (!mWaitersQueue.empty())
    {
        Fiber::EventHandle waitHandle = mWaitersQueue.top().eventHandle;
        mWaitersQueue.pop();
        Fiber::signal(waitHandle, ERR_OK);
    }
}

/*** Private Methods *****************************************************************************/

} // Blaze
