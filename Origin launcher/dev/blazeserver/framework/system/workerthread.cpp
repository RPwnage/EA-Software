/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/system/workerthread.h"
#include "framework/system/blazethread.h"

namespace Blaze
{

WorkerThread::WorkerThread(const char8_t* name, const ThreadType threadType, size_t stackSize)
    : BlazeThread(name, threadType, stackSize),
      mQueue(name),
      mShutdown(false),
      mFreeListHead(nullptr)
{
}

WorkerThread::~WorkerThread()
{
}

EA::Thread::ThreadId WorkerThread::start()
{
    return BlazeThread::start();
}

void WorkerThread::stop()
{
    mShutdown = true;
    mQueue.stop();
    waitForEnd();
}

void WorkerThread::run()
{
    while (!mShutdown)
    {
        WorkerThreadJobBase* job = (WorkerThreadJobBase*)mQueue.popOne(-1);
        if (job != nullptr)
        {
            *gLogContext = job->getLogContext();
            job->execute();
            Fiber::signal(job->getEventHandle(), job->getReturnValue());
            gLogContext->clear();
        }

        mFreeListMutex.Lock();
        WorkerThreadJobBase* freeJob = mFreeListHead;
        mFreeListHead = nullptr;
        mFreeListMutex.Unlock();

        while (freeJob != nullptr)
        {
            WorkerThreadJobBase* cur = freeJob;
            freeJob = (WorkerThreadJobBase*)freeJob->mNext;
            delete cur;
        }
    }
}

BlazeRpcError WorkerThread::queueJob(WorkerThreadJobBase* job)
{
    job->initialize(Fiber::getNextEventHandle(), *gLogContext, Fiber::getCurrentTimeout());

    mQueue.push(job);
    return Fiber::wait(job->getEventHandle(), "WorkerThread::queueJob");
}

void WorkerThread::cleanupJob(WorkerThreadJobBase* job)
{
    if (!mQueue.removeJob(job))
    {
        // WorkerThread may still be working on the job, so queue the job for destruction that will also
        // destroy the resource after the job is done.
        mFreeListMutex.Lock();
        job->mNext = mFreeListHead;
        mFreeListHead = job;
        mFreeListMutex.Unlock();
    }
}

} // namespace Blaze

