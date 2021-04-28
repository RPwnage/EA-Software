/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/fiberjobqueue.h"
#include "framework/system/fibermanager.h"

namespace Blaze
{

FiberJobQueue::FiberJobQueue(const char8_t* namedContext) :
    mJobQueueTimeout(0),
    mMaximumWorkerFibers(ONE_WORKER_LIMIT),
    mMaximumJobQueueSize(0),
    mJobQueueSizeHighWatermark(0),
    mWorkerFiberHighWatermark(0),
    mJobRejectedCount(0),
    mJobRejectedUniqueTagCount(0),
    mJobTimeoutsInQueueCount(0),
    mCancelAllCalled(false)
{
    EA_ASSERT_MSG(namedContext != nullptr && namedContext[0] != '\0', "FiberJobQueue must have a valid namedContext!");

    mJobFiberParams.blocksCaller = true;
    mJobFiberParams.groupId = Fiber::allocateFiberGroupId();
    mQueueFiberParams.groupId = mJobFiberParams.groupId;
    mQueueFiberParams.namedContext = namedContext;
    mQueueFiberParams.stackSize = Fiber::STACK_SMALL; // don't need a big stack for the queue processing code
}

FiberJobQueue::~FiberJobQueue()
{
    EA_ASSERT(!hasPendingWork());
}

void FiberJobQueue::pushJob(FiberJobQueue::InternalJob* job)
{
    if (mCancelAllCalled)
    {
        eastl::string callstack;
        CallStack::getCurrentStackSymbols(callstack);
        BLAZE_ASSERT_LOG(Log::SYSTEM, "FiberJobQueue.pushJob: cancelAllWork() has already been called, but we're pushing another job onto the queue...\n"
            "  context: " << Fiber::getCurrentContext() << "\n"
            "  callstack:\n" << SbFormats::PrefixAppender("  ", callstack.c_str()));
    }

    if (job->mUniqueJobTag != INVALID_UNIQUE_JOB_TAG)
    {
        // Insert the unique tag, but fail if it already exists (e.g. !insert_return_type::second)
        if (!mUniqueJobTagSet.insert(job->mUniqueJobTag).second)
        {
            BLAZE_TRACE_LOG(Log::SYSTEM, "[FiberJobQueue].pushJob: dropped incoming job { context: " << job->getNamedContext()
                << ", priority: " << job->getPriority() << " }; because unique tag is in use: " << job->mUniqueJobTag);
            ++mJobRejectedUniqueTagCount; // Possibly should have a different value for this. 
            delete job; // Don't call deleteInternalJob because we don't want to delete the existing tag in the set
            return;
        }
    }

    if (isJobQueueFull())
    {
        // delete the incoming job and bail
        BLAZE_WARN_LOG(Log::SYSTEM, "[FiberJobQueue].pushJob: dropped incoming job { context: " << job->getNamedContext()
            << ", priority: " << job->getPriority() << " }; queue at max capacity: " << mMaximumJobQueueSize);
        deleteInternalJob(job);
        ++mJobRejectedCount;
        return;
    }

    // Set the priority so we can test for timeouts
    if (mJobQueueTimeout != 0)
        job->setPriority(TimeValue::getTimeOfDay().getMicroSeconds());

    // Push the job onto the queue
    mJobList.push_back(job);
    mJobQueueSizeHighWatermark = eastl::max(mJobQueueSizeHighWatermark, (uint32_t)mJobList.size());

    // Schedule a worker fiber, as long as we aren't already running the max
    if (mFiberIdSet.size() < mMaximumWorkerFibers)
        gFiberManager->scheduleCall(this, &FiberJobQueue::processJobQueue, mQueueFiberParams);
}

/**
 * \brief 
 *  This function does not cancel work that has already started by existing fibers, 
 *  that work must still be waited on by doing a join()
 */
void FiberJobQueue::cancelQueuedJobs()
{
    while (!mJobList.empty())
    {
        InternalJob* j = mJobList.front();
        mJobList.pop_front();
        deleteInternalJob(j);
    }
}

void FiberJobQueue::deleteInternalJob(InternalJob* job)
{
    if (job->mUniqueJobTag != INVALID_UNIQUE_JOB_TAG)
        mUniqueJobTagSet.erase(job->mUniqueJobTag);

    delete job;
}

void FiberJobQueue::cancelAllWork()
{
    mCancelAllCalled = true;
    cancelQueuedJobs();
    if (hasPendingWork())
        Fiber::cancelGroup(mQueueFiberParams.groupId, ERR_CANCELED);
}

bool FiberJobQueue::hasPendingWork() const
{
    return !mFiberIdSet.empty();
}

BlazeRpcError FiberJobQueue::join(const TimeValue& absoluteTimeout)
{
    return Fiber::join(mQueueFiberParams.groupId, absoluteTimeout);
}

void FiberJobQueue::setMaximumWorkerFibers(uint32_t workerFiberCount)
{
    if (workerFiberCount > 0)
    {
        mMaximumWorkerFibers = workerFiberCount;
    }
    else
        EA_FAIL_MSG("Max # worker fibers must be > 0");
}

void FiberJobQueue::processJobQueue()
{
    const Fiber::FiberId fiberId = Fiber::getCurrentFiberId();
    const bool wasEmpty = mFiberIdSet.empty();
    mFiberIdSet.insert(fiberId);
    mWorkerFiberHighWatermark = eastl::max(mWorkerFiberHighWatermark, (uint32_t)mFiberIdSet.size());

    if (wasEmpty)
    {
        if (mQueueOpenedCb.isValid())
            mQueueOpenedCb();
    }

    // This is important, because we want to give the fiber that queued the job a chance to run,
    // to ensure that he queuer entertains no notions that his job may be executed immediately.
    Fiber::yield();

    Fiber::CreateParams jobFiberParams = mJobFiberParams;

    TimeValue delay = 0;
    JobList::iterator it = mJobList.begin();
    while (it != mJobList.end())
    {
        if (mFiberIdSet.size() > mMaximumWorkerFibers)
        {
            // The mMaximumWorkerFibers may have been lowered at any point this fiber was blocked, so break and let this fiber die.
            break;
        }

        InternalJob* currentJob = *it;
        const uint32_t currentSubQueueId = currentJob->mSubQueueId;
        const bool jobUsesSubQueue = currentSubQueueId != INVALID_SUBQUEUE_ID;
        if (jobUsesSubQueue && mSubQueueIdSet.find(currentSubQueueId) != mSubQueueIdSet.end())
        {
            ++it;
            continue;
        }

        mJobList.erase(it);

        // only execute the job if queue timeout is disabled or if it's not timed out
        if (mJobQueueTimeout == 0 || (delay = (TimeValue::getTimeOfDay() - currentJob->getPriority())) < mJobQueueTimeout)
        {
            if (jobUsesSubQueue)
                mSubQueueIdSet.insert(currentSubQueueId);

            jobFiberParams.namedContext = currentJob->getNamedContext();
            jobFiberParams.relativeTimeout = (currentJob->mRelativeTimeout != 0 ? currentJob->mRelativeTimeout : mDefaultJobTimeout);
            gFiberManager->scheduleCall(currentJob, &InternalJob::execute, jobFiberParams);

            if (jobUsesSubQueue)
                mSubQueueIdSet.erase(currentSubQueueId);
        }
        else
        {
            ++mJobTimeoutsInQueueCount;
            BLAZE_WARN_LOG(Log::SYSTEM, "[FiberJobQueue].processJobQueue: timed out job { context: " << currentJob->getNamedContext() << ", priority: "
                <<  currentJob->getPriority() << " }, delayed by: " << TimeValue(delay).getMillis() << "ms; configured timeout: " << mJobQueueTimeout.getMillis() << "ms");
        }
        deleteInternalJob(currentJob);

        Fiber::yieldOnAllottedTime(TimeValue(YIELD_DURATION_MS * 1000)); //Give a chance to other fibers in case we have been hogging cpu too much
        
        it = mJobList.begin(); // reset the iterator as the queue may have been modified while we were waiting
    }
    
    mFiberIdSet.erase(fiberId);
    if (mFiberIdSet.empty())
    {
        if (mQueueDrainedCb.isValid())
        {
            Fiber::BlockingSuppressionAutoPtr blockPtr;
            mQueueDrainedCb();
        }
    }
}

} // Blaze
