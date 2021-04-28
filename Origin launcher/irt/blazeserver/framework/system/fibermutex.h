/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIBERMUTEX_H
#define BLAZE_FIBERMUTEX_H

/*** Include files *******************************************************************************/
#include "framework/system/fiber.h"
#include "framework/system/allocator/callstack.h"
#include "framework/system/frameworkresource.h"
#include "EASTL/priority_queue.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class FiberMutex : public FrameworkResource
{
    NON_COPYABLE(FiberMutex);

public:
    FiberMutex(bool registered = true);
    ~FiberMutex() override;

    BlazeRpcError lock();
    // (Renamed to avoid issue with converting from old ms times to TimeValue.  TimeValue constructor uses microseconds.)
    BlazeRpcError lockWithTimeout(EA::TDF::TimeValue waitTimeout, const char8_t* context = nullptr);
    void unlock();

    bool isInUse() const { return isOwnedByAnyFiberStack() || !mWaitersQueue.empty(); }
    bool isOwnedByAnyFiberStack() const { return mCurrentFiberId != Fiber::INVALID_FIBER_ID; }
    bool isOwnedByCurrentFiberStack() const { return mCurrentFiberId == Fiber::getFiberIdOfBlockingStack(); }

    int32_t getLockDepth() const { return mLockDepth; }

    const char8_t* getTypeName() const override { return "FiberMutex"; }

protected:
    void assignInternal() override;
    void releaseInternal() override;

private:
    void signalNext();

private:
    Fiber::FiberId mCurrentFiberId;
    int32_t mLockDepth;
    bool mRegistered; // whether the mutex is registered with the fiber manager 
    CallStack mLockCallstack;
    CallStack mUnlockCallstack;

    struct EventHandleNode
    {
        EventHandleNode(const Fiber::EventHandle& _eventHandle) : eventHandle(_eventHandle) {}

        Fiber::EventHandle eventHandle;
        operator const Fiber::EventHandle& () const { return eventHandle; }

        bool operator<(const EventHandleNode& rhs) const
        {
            return (eventHandle.priority > rhs.eventHandle.priority);
        }
    };

    typedef eastl::priority_queue<EventHandleNode> EventHandlePriorityQueue;
    EventHandlePriorityQueue mWaitersQueue;
};

struct FiberMutexUnregistered : public FiberMutex
{
public:
    FiberMutexUnregistered() : FiberMutex(false) {}
};

template <class T>
class FiberMutexWrapper : public T, public FiberMutex
{
};

struct FiberAutoMutex
{
    NON_COPYABLE(FiberAutoMutex);
public:
    FiberAutoMutex(FiberMutex& fiberMutex)
        : mFiberMutex(fiberMutex)
    {
        mFiberMutex.lock();
    }

    ~FiberAutoMutex()
    {
        if (hasLock())
            mFiberMutex.unlock();
    }

    bool hasLock() const { return mFiberMutex.isOwnedByCurrentFiberStack(); }

private:
    FiberMutex& mFiberMutex;
};

}

#endif // BLAZE_FIBERMUTEX_H
