/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIBERJOBQUEUE_H
#define BLAZE_FIBERJOBQUEUE_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_condition.h"
#include "eathread/eathread_mutex.h"
#include "framework/system/job.h"
#include "framework/system/fiber.h"
#include "EATDF/time.h"

namespace Blaze
{

class FiberJobQueue
{
    NON_COPYABLE(FiberJobQueue);
public:
    typedef Functor QueueEventCb;

    /*! \brief  These are standard values that limit the number of concurrent worker fibers spawned by the job queue (when multiple fibers are required).
     *!         They are chosen to reduce the amount of memory and fiber context switching used by the system as a whole in case of job stalls. 
     *!         Deviation from these settings should be explained in the code. */
    static const uint32_t ONE_WORKER_LIMIT      = 1; // Default
    static const uint32_t SMALL_WORKER_LIMIT    = 10;
    static const uint32_t MEDIUM_WORKER_LIMIT   = 20;
    static const uint32_t LARGE_WORKER_LIMIT    = 50;

    /*! \brief  Jobs queued with INVALID_SUBQUEUE_ID are not subject to sequential execution. */
    static const uint32_t INVALID_SUBQUEUE_ID   = 0;
    static const uint32_t INVALID_UNIQUE_JOB_TAG= 0;

    /*! \brief  Yield to other fibers if the job queue exceeds the duration. */
    static const uint32_t YIELD_DURATION_MS   = 50;

    struct JobParams
    {
        JobParams(const char8_t* jobContext) : 
            namedContext(jobContext), 
            subQueueId(INVALID_SUBQUEUE_ID),
            uniqueJobTag(INVALID_UNIQUE_JOB_TAG)
        {
            EA_ASSERT_MSG(namedContext != nullptr && namedContext[0] != '\0', "Invalid named context!");
        }
        const char8_t* namedContext; // Provides a named context for fiber scheduled to execute the job integrating with framework fiber timings and execution count tracking
        uint32_t subQueueId; // Used to organize jobs into separate sub-queues serviced by the worker fibers in the pool; jobs queued to the same subQueueId are always executed sequentially
        uint64_t uniqueJobTag; // Used to indicate the job's id as part of a unique set.  
        EA::TDF::TimeValue relativeTimeout; // Specifies the amount of time the job has to run once it is started.  Note, this is not an "absolute" time because there is no way of knowing when the job will be pulled off the queue and run.
    };

    class InternalJob : public Job
    {
    public:
        InternalJob() : mSubQueueId(INVALID_SUBQUEUE_ID), mUniqueJobTag(INVALID_UNIQUE_JOB_TAG) {}
        void execute() override = 0;
        uint32_t mSubQueueId;
        uint64_t mUniqueJobTag;
        EA::TDF::TimeValue mRelativeTimeout;
    };

    FiberJobQueue(const char8_t* namedContext);
    virtual ~FiberJobQueue();

    #define FIBER_JOB_QUEUE_PUSH_CALL_DECL1(_METHOD_CALL_TYPE_PARAMS, _METHOD_CALL_ARG_DECL, _METHOD_JOB_DECL, _CALLER) \
    _METHOD_CALL_TYPE_PARAMS \
    void queueFiberJob(_METHOD_CALL_ARG_DECL, const JobParams& params) { \
        InternalJob* job = _METHOD_JOB_DECL; \
        job->setNamedContext(params.namedContext); \
        job->mSubQueueId = params.subQueueId; \
        job->mUniqueJobTag = params.uniqueJobTag; \
        job->mRelativeTimeout = params.relativeTimeout; \
        pushJob(job); \
    }
    METHOD_JOB_FUNC_DECL_JOB_PARENT(FIBER_JOB_QUEUE_PUSH_CALL_DECL1, InternalJob);

    void setQueueEventCallbacks(const QueueEventCb& queueOpenedCb, const QueueEventCb& queueDrainedCb) { mQueueOpenedCb = queueOpenedCb; mQueueDrainedCb = queueDrainedCb; }
    void setQueuedJobTimeout(EA::TDF::TimeValue relativeTimeout) { mJobQueueTimeout = relativeTimeout; }
    void setDefaultJobTimeout(EA::TDF::TimeValue relativeTimeout) { mDefaultJobTimeout = relativeTimeout; }
    void setJobQueueCapacity(uint32_t maximumJobQueueSize) { mMaximumJobQueueSize = maximumJobQueueSize; }
    void setMaximumWorkerFibers(uint32_t workerFiberCount);
    void setJobFiberStackSize(Fiber::StackSize stackSize) { mJobFiberParams.stackSize = stackSize; }
    void cancelQueuedJobs();
    void cancelAllWork();
    bool hasPendingWork() const;
    BlazeRpcError join(const EA::TDF::TimeValue& absoluteTimeout = EA::TDF::TimeValue());

    uint32_t getMaximumWorkerFibers() const { return mMaximumWorkerFibers; }
    uint32_t getWorkerFiberCount() const { return mFiberIdSet.size(); }
    uint32_t getWorkerFiberCountHighWatermark() const { return mWorkerFiberHighWatermark; }
    uint32_t getJobQueueSize() const { return mJobList.size(); }
    uint32_t getJobQueueSizeHighWatermark() const { return mJobQueueSizeHighWatermark; }
    uint64_t getJobRejectedCount() const { return mJobRejectedCount; }
    uint64_t getJobRejectedUniqueTagCount() const { return mJobRejectedUniqueTagCount; }
    uint64_t getJobTimeoutsInQueueCount() const { return mJobTimeoutsInQueueCount; }

    bool isJobQueueFull() const { return (mMaximumJobQueueSize != 0) && (getJobQueueSize() >= mMaximumJobQueueSize); }
    bool isJobQueueEmpty() const { return getJobQueueSize() == 0; }
    bool isWorkerAvailable() const { return mFiberIdSet.size() < mMaximumWorkerFibers; }

private:
    void pushJob(InternalJob* job);
    void deleteInternalJob(InternalJob* job);
    void processJobQueue();

private:
    typedef eastl::vector_set<Fiber::FiberId> FiberIdSet;
    typedef eastl::vector_set<uint32_t> SubQueueIdSet;
    typedef eastl::list<InternalJob*> JobList;
    typedef eastl::vector_set<uint64_t> UniqueJobTagSet;

    Fiber::CreateParams mQueueFiberParams;
    Fiber::CreateParams mJobFiberParams;
    EA::TDF::TimeValue mJobQueueTimeout;
    EA::TDF::TimeValue mDefaultJobTimeout;
    uint32_t mMaximumWorkerFibers;
    uint32_t mMaximumJobQueueSize;

    // Usage counters
    uint32_t mJobQueueSizeHighWatermark;
    uint32_t mWorkerFiberHighWatermark;
    uint64_t mJobRejectedCount;
    uint64_t mJobRejectedUniqueTagCount;
    uint64_t mJobTimeoutsInQueueCount;

    QueueEventCb mQueueOpenedCb;
    QueueEventCb mQueueDrainedCb;

    JobList mJobList;
    FiberIdSet mFiberIdSet;
    SubQueueIdSet mSubQueueIdSet; // invariant: mSubQueueIdSet.size() <= mFiberIdSet.size()
    UniqueJobTagSet mUniqueJobTagSet;

    bool mCancelAllCalled;
};

} // Blaze

#endif // BLAZE_FIBERJOBQUEUE_H
