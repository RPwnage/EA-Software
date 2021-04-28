/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_WORKERTHREADPOOL_H
#define BLAZE_WORKERTHREADPOOL_H

/*** Include files *******************************************************************************/

#include "framework/system/workerthread.h"
#include "EASTL/priority_queue.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class WorkerThreadPool
{
    NON_COPYABLE(WorkerThreadPool);

public:
    virtual ~WorkerThreadPool();

    void initialize();

    void start();
    void stop();

    void shutdown();

protected:

    WorkerThreadPool(uint32_t maxThreads);

    BlazeRpcError DEFINE_ASYNC_RET(getWorkerThread(WorkerThread*& thread));
    void releaseWorkerThread(WorkerThread& thread);

    virtual WorkerThread* createThread() = 0;

    virtual WorkerThread* activateThread();
    virtual void deactivateThread(WorkerThread& thread);

private:
    class ConnectionWaiter
    {
    public:
        ConnectionWaiter(WorkerThread** workerThreadPtr, const Fiber::EventHandle& eventHandle)
            : mWorkerThreadPtr(workerThreadPtr), mEventHandle(eventHandle) {}

        WorkerThread** getWorkerThreadPtr() const { return mWorkerThreadPtr; }
        const Fiber::EventHandle& getEventHandle() const { return mEventHandle; }

        bool operator<(const ConnectionWaiter& rhs) const
        {
            return (mEventHandle.priority > rhs.mEventHandle.priority);
        }

    private:
        WorkerThread** mWorkerThreadPtr;
        Fiber::EventHandle mEventHandle;
    };

    bool mShutdown;

    uint32_t mMaxThreads;

    typedef eastl::intrusive_list<WorkerThread> WorkerThreadList;

    WorkerThreadList mActivePool;
    uint32_t mActivePoolCount;
    WorkerThreadList mInactivePool;
    uint32_t mInactivePoolCount;

    // For thread-safety.
    mutable EA::Thread::Mutex mMutex;

    typedef eastl::priority_queue<ConnectionWaiter, eastl::deque<ConnectionWaiter> > GetThreadQueue;
    GetThreadQueue mWaiterQueue;
};

} // Blaze

#endif // BLAZE_WORKERTHREADPOOL_H

