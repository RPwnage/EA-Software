/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class ThreadPoolJobHandler

    This is an implementation of a job handler which queues jobs to be processed by a
    pool of worker threads.

    \notes

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/system/threadpooljobhandler.h"
#include "framework/system/job.h"
#include "framework/util/shared/blazestring.h"
#include "framework/system/threadlocal.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static const size_t WORKER_THREAD_SIZE = 256 * 1024;

/*** Public Methods ******************************************************************************/

ThreadPoolJobHandler::ThreadPoolJobHandler(size_t workerCount, const char8_t *poolName, BlazeThread::ThreadType threadType)
    : mQueue("ThreadPoolJobHandler"),
      mStarted(false),
      mInitialCount(workerCount),
      mExtraCount(0),
      mRunningJobCount(0),
      mPeakRunningJobCount(0),
      mWorkerThreadType(threadType),
      mIsQuitting(false)
{
    blaze_strnzcpy(mPoolName, poolName != nullptr ? poolName : "ThreadPool", sizeof(mPoolName));
}

ThreadPoolJobHandler::~ThreadPoolJobHandler()
{
    stop();

    //Delete all the active workers
    ThreadVector::iterator itr = mWorkers.begin();
    ThreadVector::iterator end = mWorkers.end();
    for (; itr != end; ++itr)
        delete *itr;
}

void ThreadPoolJobHandler::start()
{
    mWorkerMutex.Lock();

    mStarted = true;

    // Start all the worker threads
    for(size_t idx = 0; idx < mInitialCount; idx++)
    {
       activateWorker();
    }

    mWorkerMutex.Unlock();
}

void ThreadPoolJobHandler::stop()
{
    if (!mIsQuitting)
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[" << mPoolName << "].stop: Stopping all worker threads.");
        mWorkerMutex.Lock();

        mIsQuitting = true;

        mQueue.stop();
        
        ThreadVector::iterator itr = mWorkers.begin();
        ThreadVector::iterator end = mWorkers.end();
        for (; itr != end; ++itr)
        {
            (*itr)->stop();
        }
        mWorkerMutex.Unlock();

        BLAZE_TRACE_LOG(Log::SYSTEM, "[" << mPoolName << "].stop: Stopped all worker threads.");
    }
}


void ThreadPoolJobHandler::queueJob(Job& job)
{ 
    if (EA_UNLIKELY(!mIsQuitting))
    {
        mQueue.push(&job); 
    }
}

void ThreadPoolJobHandler::addWorkers(size_t growBy)
{
    mWorkerMutex.Lock();
    
    if (!mIsQuitting)
    {

        if (!mStarted)  
        {
            //If we haven't started yet, bump the initial count by growBy.
            mInitialCount += growBy;
        }
        else
        {
            //Otherwise start up some more threads
            for(size_t idx = 0; idx < growBy; idx++)
            {
                activateWorker();
            }
        }
    }

    mWorkerMutex.Unlock();
}

void ThreadPoolJobHandler::removeWorkers(size_t reduceBy)
{
    // We don't actually remove the worker thread itself, we only note
    // there is now an extra worker thread available.  The extra worker threads are then
    // used up the next time activateWorker() is called.
    mWorkerMutex.Lock();
    mExtraCount += reduceBy;
    mWorkerMutex.Unlock();
}

uint32_t ThreadPoolJobHandler::getNumWorkers() const
{
    mWorkerMutex.Lock();
    uint32_t workers = (uint32_t)mWorkers.size();
    mWorkerMutex.Unlock();
    return workers;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

void ThreadPoolJobHandler::activateWorker()
{
    mWorkerMutex.Lock();

    if (mExtraCount == 0)
    {
        int32_t workerId = (int32_t)mWorkers.size();
        Worker* worker = BLAZE_NEW Worker(*this, mWorkerThreadType, workerId);        
        EA::Thread::ThreadId threadId = worker->start();
        if (threadId != EA::Thread::kThreadIdInvalid)
            mWorkers.push_back(worker);
    }
    else
    {
        // use up an extra worker thread is one's available.
        --mExtraCount;
    }

    mWorkerMutex.Unlock();
}

void ThreadPoolJobHandler::reconfigure()
{
    mWorkerMutex.Lock();
    ThreadVector::iterator itr = mWorkers.begin();
    ThreadVector::iterator end = mWorkers.end();
    for (; itr != end; ++itr)
        (*itr)->setState(Worker::STATE_RECONFIGURE);
    mQueue.wakeAll();
    mWorkerMutex.Unlock();
}

ThreadPoolJobHandler::Worker::Worker(ThreadPoolJobHandler& owner, BlazeThread::ThreadType threadType, int32_t id)
    : BlazeThread(nullptr, threadType, WORKER_THREAD_SIZE),
      mId(id),
      mOwner(owner)
{
    char8_t threadName[64];
    blaze_snzprintf(threadName, sizeof(threadName), "%s.%d", mOwner.mPoolName, id);
    BlazeThread::setName(threadName);

    mState = STATE_NORMAL;
}

ThreadPoolJobHandler::Worker::~Worker()
{
}

void ThreadPoolJobHandler::Worker::setState(State state)
{
    mState = state;
}

void ThreadPoolJobHandler::Worker::run()
{
    BLAZE_TRACE_LOG(Log::SYSTEM, "[ThreadPoolJobHandler::Worker].run: Thread starting");

    mOwner.threadStart();
    
    // Loop.
    while (!mOwner.mIsQuitting)
    {
        if (mState == STATE_RECONFIGURE)
        {
            mState = STATE_NORMAL;
            mOwner.threadReconfigure(mId);
        }

        // Get the next job.
        Job* job = mOwner.mQueue.popOne(-1);

        // if job is not blank, work.
        if (EA_LIKELY(job != nullptr))
        {
            if (!mOwner.mIsQuitting)
            {
                BLAZE_TRACE_LOG(Log::SYSTEM, "[ThreadPoolJobHandler::Worker].run: Processing job " << job);

                ++mOwner.mRunningJobCount;
                if (mOwner.mRunningJobCount > mOwner.mPeakRunningJobCount)
                    mOwner.mPeakRunningJobCount = mOwner.mRunningJobCount;
                mOwner.executeJob(*job);
                mOwner.mRunningJobCount--;
            }
            else
            {
                delete job;  // If we're quiting, be sure to delete the job we just popped. 
            }
        }
    } // while.

    mOwner.threadEnd();

    // Quit.
    BLAZE_TRACE_LOG(Log::SYSTEM, "[ThreadPoolJobHandler::Worker].run: Thread finished");
}

void ThreadPoolJobHandler::Worker::stop()
{
    // since this worker might be blocked
    // waiting on the owner's queue, we
    // always signal it before waiting.
    mOwner.mQueue.wakeAll();

    waitForEnd();
}

/*! ***************************************************************************/
/*! \brief This function executes a job.  It can be overridden to provide specific
           pre/post job functionality.

    \param job  Job to execute.
*******************************************************************************/
void ThreadPoolJobHandler::executeJob(Job& job)
{
    job.execute();
}

/*! ***************************************************************************/
/*! \brief This function call called when a worker thread is asking to be
     reconfigured. It should be overridden to provide reconfiguration
     specific to what the worker thread is being used for.

    \param workerId  ID of the worker thread this call is running on
*******************************************************************************/
void ThreadPoolJobHandler::threadReconfigure(int32_t workerId)
{
}


} // namespace Blaze

