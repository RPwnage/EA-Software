/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/system/workerthreadpool.h"

namespace Blaze
{

WorkerThreadPool::WorkerThreadPool(uint32_t maxThreads)
: mShutdown(false), mMaxThreads(maxThreads)
{
}

WorkerThreadPool::~WorkerThreadPool()
{
    while (!mInactivePool.empty())
    {
        WorkerThread* thread = &mInactivePool.front();
        mInactivePool.pop_front();
        delete thread;
    }
}

void WorkerThreadPool::initialize()
{
    for (uint32_t i = 0; i < mMaxThreads; ++i)
    {
        WorkerThread* thread = createThread();
        mInactivePool.push_back(*thread);
        ++mInactivePoolCount;
    }
}

void WorkerThreadPool::start()
{
    for (WorkerThreadList::iterator it = mInactivePool.begin(); it != mInactivePool.end(); ++it)
        it->start();
}

void WorkerThreadPool::stop()
{
    for (WorkerThreadList::iterator it = mInactivePool.begin(); it != mInactivePool.end(); ++it)
        it->stop();
}

void WorkerThreadPool::shutdown()
{
    mMutex.Lock();

    mShutdown = true;

    mMutex.Unlock();
}

BlazeRpcError WorkerThreadPool::getWorkerThread(WorkerThread*& thread)
{
    VERIFY_WORKER_FIBER();

    thread = nullptr;

    // Acquire mutex.
    mMutex.Lock();

    if (mShutdown)
        return ERR_SYSTEM;

    thread = activateThread();
    Fiber::EventHandle eventHandle;
    if (thread != nullptr)
    {
        mMutex.Unlock();
    }
    else
    {
        // No connections are available so queue the request
        BLAZE_TRACE_LOG(Log::DB, "[WorkerThreadPool].getWorkerThread: "
            "no connections available; queuing request for fiber " << Fiber::getCurrentContext() << ".");
        eventHandle = Fiber::getNextEventHandle();
        mWaiterQueue.push(ConnectionWaiter(&thread, eventHandle));
        mMutex.Unlock();

        // Sleep this fiber until a connection is available (or it times out).  The result
        // pointer should be set once the fiber awakens (or set to nullptr on timeout).
        BlazeRpcError error = Fiber::wait(eventHandle, "WorkerThreadPool::getWorkerThread");
        if (error != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(Log::DB, "[WorkerThreadPool].getWorkerThread: "
                "timed out fiber " << Fiber::getCurrentContext() << " waiting for a connection.");

            mMutex.Lock();
            GetThreadQueue::container_type::iterator itr = mWaiterQueue.get_container().begin();
            GetThreadQueue::container_type::iterator end = mWaiterQueue.get_container().end();
            for(; itr != end; ++itr)
            {
                if (itr->getEventHandle() == eventHandle)
                {
                    //Remove us
                    mWaiterQueue.get_container().erase(itr);
                    break;
                }
            }
            mMutex.Unlock();
            
            if (thread != nullptr)
            {
                // Release the connection back to the pool
                // since we cannot use it due to error.
                releaseWorkerThread(*thread);
            }
            
            return ERR_SYSTEM;
        }
    }

    if (thread != nullptr)
    {
        BLAZE_TRACE_LOG(Log::DB, "[WorkerThreadPool].getWorkerThread: acquired worker thread " << thread->getName()
            << "; current pool active count:" << mActivePoolCount << " and current pool inactive count:" << mInactivePoolCount << ".");
        return ERR_OK;
    }
    return ERR_SYSTEM;
}

void WorkerThreadPool::releaseWorkerThread(WorkerThread& thread)
{
    BLAZE_TRACE_LOG(Log::DB, "[WorkerThreadPool].release: releasing thread:" << thread.getName() << " to pool.");

    // Try to give the connection to someone waiting for one
    mMutex.Lock();
    
    Fiber::EventHandle eventHandle;
    if (!mWaiterQueue.empty())
    {
        ConnectionWaiter waiter = mWaiterQueue.top();
        *(waiter.getWorkerThreadPtr()) = &thread;
        eventHandle = waiter.getEventHandle();
        mWaiterQueue.pop();
    }

    if (eventHandle.isValid())
    {
        BLAZE_TRACE_LOG(Log::DB, "[WorkerPoolThread].release: thread" << thread.getName() << " now available; "
                "signaling event " << eventHandle.eventId);
      
        mMutex.Unlock();

        Fiber::signal(eventHandle, ERR_OK);
        
        // We took care of this conn, exit
        return;
    }

    // Nobody to give the connection to so put it back in the inactive list.
    deactivateThread(thread);

    mMutex.Unlock();

}

WorkerThread* WorkerThreadPool::activateThread()
{
    WorkerThread *result = nullptr;
    if (!mInactivePool.empty())
    {
        result = &mInactivePool.front();
        // result->activate(); TBD
        mInactivePool.pop_front();
        --mInactivePoolCount;

        mActivePool.push_back(*result);
        ++mActivePoolCount;
    }
    return result;
}

void WorkerThreadPool::deactivateThread(WorkerThread& thread)
{
    // thread.deactivate(); TBD
    mActivePool.remove(thread);
    --mActivePoolCount;

    // Put connected DbConns at the front of the queue and disconnected ones at the end
    //if (conn.isConnected())
        mInactivePool.push_front(thread);
    //else
        //mInactivePool.push_back(conn);
    ++mInactivePoolCount;
}

} // namespace Blaze

