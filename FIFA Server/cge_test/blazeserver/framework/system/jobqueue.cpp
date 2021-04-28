/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class JobQueue

    This is a thread-safe FIFO queue that can block the consumer if there are no jobs available.
    The queue maintains an ordered circular list of job buckets. The job bucket order is determined
    by the priority of the jobs contained in the bucket. Each job bucket stores a circular list of 
    inserted jobs of identical priority in FIFO order.
    
    - Consumer can block for a specific time duration by using popOne(TimeValue)
    - Consumer can retrieve a lowest priority singly-linked list of jobs by calling popMany()
    - Producer calling push() will signal the consuming thread

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/jobqueue.h"
#include "framework/system/job.h"

namespace Blaze
{

struct JobBucket
{
    JobBucket* mNext;
    Job* mJob;
};

JobQueue::JobQueue(const char8_t* name)
: mStopped(false), mBucket(nullptr), mFreeBucket(nullptr), mName(name)
{
#ifdef JOB_QUEUE_PERF_METRICS
    mQueueSize = 0;
    mQueueMax = 0;
    mQueueMaxReached = 0;
    mQueueBuckets = 0;
    mQueueAddBeforeHead = 0;
    mQueueAddAtHead = 0;
    mQueueAddAfterTail = 0;
    mQueueAddAtTail = 0;
    mQueueAddBetween = 0;
    mQueueAddBetweenDepth = 0;
    mQueueAddNew = 0;
#endif
}

JobQueue::~JobQueue()
{
    // delete all jobs and signal all threads that might be waiting
    stop();
    
    // delete all job buckets (safe without locking because stop()
    // will prevent job buckets from being added or removed)
    clearBuckets();
    
#ifdef JOB_QUEUE_PERF_METRICS
    BLAZE_INFO_LOG(Log::SYSTEM, "[JobQueue].dtor: QName: " << mName << ", QMax: " << mQueueMax << ", QMaxReached: " << mQueueMaxReached 
                   << ", QBuckets: " << mQueueBuckets << ", QAddNew: " << mQueueAddNew << ", QAddBeforeHead: " << mQueueAddBeforeHead << ", QAddAtHead: " 
                   << mQueueAddAtHead << ", QAddAtTail: " << mQueueAddAtTail << ", QAddAfterTail: " << mQueueAddAfterTail << ", QAddBetween: " 
                   << mQueueAddBetween << ", QAddBetweenDepth: " << mQueueAddBetweenDepth);
#endif
}

/*! ***************************************************************************/
/*! \brief  push
            The insertion algorithm was designed to perform best when the priority
            of the incoming job is:
            >= than the largest queued job priority, or
            <= than the smallest queued job priority
            In both those situations there is a maxumum of 2 comparisons before
            the job is inserted into the queue.
    \param  Job * job 
    \return void 
*******************************************************************************/
void JobQueue::push(Job* job)
{
    mMutex.Lock();
    
    if (!mStopped)
    {
        if (mBucket == nullptr)
        {
            mBucket = allocateBucket();
            mBucket->mNext = mBucket;
            mBucket->mJob = job->mNext = job;
#ifdef JOB_QUEUE_PERF_METRICS
            ++mQueueAddNew;
#endif
        }
        else
        {
            if (job->mPriority == mBucket->mJob->mPriority)
            {
                // add job to the tail of the tail bucket
                insertInto(mBucket, job);
#ifdef JOB_QUEUE_PERF_METRICS
                ++mQueueAddAtTail;
#endif
            }
            else if (job->mPriority > mBucket->mJob->mPriority)
            {
                // create a new bucket and add a job to it
                insertAfter(mBucket, job);
                // update the tail pointer
                mBucket = mBucket->mNext;
#ifdef JOB_QUEUE_PERF_METRICS
                ++mQueueAddAfterTail;
#endif
            }
            else
            {
#ifdef JOB_QUEUE_PERF_METRICS
                uint32_t addBetweenDepth = 0;
#endif
                JobBucket* prev = mBucket;
                JobBucket* bucket = mBucket->mNext;
                for(;;)
                {
                    if (job->mPriority == bucket->mJob->mPriority)
                    {
                        // add job to the tail of the head bucket
                        insertInto(bucket, job);
#ifdef JOB_QUEUE_PERF_METRICS
                        ++mQueueAddAtHead;
#endif
                        break;
                    }
                    if (job->mPriority < bucket->mJob->mPriority)
                    {
                        // job priority is less than head bucket job,
                        // create a new bucket and add the job to it
                        insertAfter(prev, job);
#ifdef JOB_QUEUE_PERF_METRICS
                        ++mQueueAddBeforeHead;
#endif
                        break;
                    }
                    prev = bucket;
                    bucket = bucket->mNext;
#ifdef JOB_QUEUE_PERF_METRICS
                    ++addBetweenDepth;
#endif
                }
#ifdef JOB_QUEUE_PERF_METRICS
                if (addBetweenDepth > 0)
                {
                    ++mQueueAddBetween;
                    mQueueAddBetweenDepth += addBetweenDepth;
                }
#endif
            }
        }
        
#ifdef JOB_QUEUE_PERF_METRICS
        ++mQueueSize;
        if (mQueueSize > mQueueMax)
        {
            mQueueMax = mQueueSize;
        } 
        else if (mQueueSize == mQueueMax)
        {
            ++mQueueMaxReached;
        }
#endif
    }
    else
    {
        delete job;
    }

    mMutex.Unlock();
    mCondition.Signal(false);
}

/*! ***************************************************************************/
/*! \brief  popOne
            Retrieves the job with the highest(earliest) available priority,
            or nullptr if none.
            When the queue is empty, waits for a period specified by timeout.
            If timeout < 0, waits an OS-specific quantum of time before waking.
            NOTE: We do not need to check mStopped because once the queue is 
            stopped all jobs are always deleted and no new jobs can be inserted.
    \param  TimeValue timeout 
    \return Job* 
*******************************************************************************/
Job* JobQueue::popOne(const TimeValue& timeout)
{
    Job* head = nullptr;
    
    mMutex.Lock();
    
    if (mBucket == nullptr)
    {
        int64_t time;
        if (timeout < 0)
            mCondition.Wait(&mMutex, EA::Thread::kTimeoutNone);
        else if ((time = timeout.getMillis() - TimeValue::getTimeOfDay().getMillis()) > 0)
            mCondition.Wait(&mMutex, EA::Thread::GetThreadTime() + EA::Thread::ThreadTime(time));
    }
    
    if (mBucket != nullptr)
    {
        // get last job from head bucket
        Job* tail = mBucket->mNext->mJob;
        // get first job from head bucket
        head = tail->mNext;
        // point tail to the new head
        tail->mNext = head->mNext;
        // check if the bucket is empty
        if (tail == head)
        {
            // get head bucket
            JobBucket* bucket = mBucket->mNext;
            if (mBucket != bucket)
            {
                // update mBucket to point to new head
                mBucket->mNext = bucket->mNext;
            }
            else
            {
                // update mBucket to point to nullptr
                mBucket = nullptr;
            }
            // attach bucket to the freelist
            bucket->mNext = mFreeBucket;
            mFreeBucket = bucket;
        }

#ifdef JOB_QUEUE_PERF_METRICS
        --mQueueSize;
#endif
    }
    
    mMutex.Unlock();
    
    return head;
}

/*! ***************************************************************************/
/*! \brief  popMany
            Retrieve all the jobs in the bucket with the highest(earliest)
            priority, without waiting or nullptr if none available.
            NOTE: We do not need to check mStopped because once the queue is
            stopped, all jobs are always deleted and no new jobs can be inserted.
    \return Job* 
*******************************************************************************/
Job* JobQueue::popMany()
{
    Job* head = nullptr;
    
    mMutex.Lock();

    if (mBucket != nullptr)
    {
        // get last job from head bucket
        Job* tail = mBucket->mNext->mJob;
        // get first job from head bucket
        head = tail->mNext;
        // point tail nullptr to break the cycle
        tail->mNext = nullptr;
        // get head bucket
        JobBucket* bucket = mBucket->mNext;
        if (mBucket != bucket)
        {
            // update mBucket to point to new head
            mBucket->mNext = bucket->mNext;
        }
        else
        {
            // update mBucket to point to nullptr
            mBucket = nullptr;
        }
        // attach bucket to the freelist
        bucket->mNext = mFreeBucket;
        mFreeBucket = bucket;
        
#ifdef JOB_QUEUE_PERF_METRICS
        Job* job = head;
        do
        {
            --mQueueSize;
        } while ((job = job->mNext) != nullptr);
#endif
    }

    mMutex.Unlock();
    
    return head;
}

JobBucket* JobQueue::allocateBucket()
{
    JobBucket* newBucket;
    if (mFreeBucket != nullptr)
    {
        // pull bucket from the freelist
        newBucket = mFreeBucket;
        mFreeBucket = mFreeBucket->mNext;
    }
    else
    {
        newBucket = BLAZE_NEW JobBucket;
#ifdef JOB_QUEUE_PERF_METRICS
        ++mQueueBuckets;
#endif
    }
    return newBucket;
}

void JobQueue::insertAfter(JobBucket* bucket, Job* job)
{
    JobBucket* newBucket = allocateBucket();
    newBucket->mNext = bucket->mNext;
    newBucket->mJob = job->mNext = job;
    bucket->mNext = newBucket;
}

void JobQueue::insertInto(JobBucket* bucket, Job* job)
{
    job->mNext = bucket->mJob->mNext;
    bucket->mJob = bucket->mJob->mNext = job;
}

/*! ***************************************************************************/
/*! \brief  stop
            Stops the job queue, wakes up any remaining threads, 
            and deletes any new jobs passed to it.
    \return void 
*******************************************************************************/
void JobQueue::stop()
{
    mMutex.Lock();
    mStopped = true;
    clearJobs();
    mMutex.Unlock();
    wakeAll();
}

/*! ***************************************************************************/
/*! \brief  wakeAll
            Wake up any blocked consumer threads
    \return void 
*******************************************************************************/
void JobQueue::wakeAll()
{
    mCondition.Signal(true);
}

bool JobQueue::removeJob(Job* job)
{
    bool jobFound = false;
    mMutex.Lock();
    if (mBucket != nullptr)
    {
        JobBucket* tail = mBucket;
        JobBucket* prev = tail;
        JobBucket* curr = mBucket->mNext;

        // Find the bucket that contains this job
        do
        {
            Job* jobCurr = curr->mJob->mNext;
            Job* jobTail = curr->mJob;
            Job* jobPrev = jobTail;
            do
            {
                if (job == jobCurr) // found the job to remove
                {
                    jobFound = true;
                    // check if the bucket is empty
                    if (jobPrev == jobCurr)
                    {
                        if (prev != curr)
                        {
                            // update prev to point to next bucket
                            prev->mNext = curr->mNext;
                            if (curr == mBucket)
                            {
                                mBucket = prev;
                            }
                        }
                        else // last bucket
                        {
                            // update mBucket to point to nullptr
                            mBucket = nullptr;
                        }
                        // attach bucket to the freelist
                        curr->mJob = nullptr;
                        curr->mNext = mFreeBucket;
                        mFreeBucket = curr;
                    }
                    else
                    {
                        jobPrev->mNext = jobCurr->mNext; // detach jobCurr
                        if (curr->mJob == jobCurr)
                        {
                            curr->mJob = jobPrev;
                        }
                    }
                    break;
                }

                jobPrev = jobCurr;
                jobCurr = jobCurr->mNext;
            }
            while (jobTail->mNext != jobCurr);
            prev = curr;
            curr = curr->mNext;
        } while (!jobFound && tail->mNext != curr);
    }
    mMutex.Unlock();

    if (jobFound)
        delete job;
    return jobFound;
}

/*! ***************************************************************************/
/*! \brief  removeAll
            Removes all jobs from the queue without canceling or running them.
    \return void 
*******************************************************************************/
void JobQueue::removeAll()
{
    mMutex.Lock();
    clearJobs();
    mMutex.Unlock();
}

/*! ***************************************************************************/
/*! \brief  clearJobs
            Deletes any pending jobs without executing them and adds remaining
            job buckets into the freelist to be removed by clearBuckets().
    \return void 
*******************************************************************************/
void JobQueue::clearJobs()
{
    if (mBucket != nullptr)
    {
        JobBucket* bucket = mBucket->mNext; // grab head
        mBucket->mNext = nullptr; // break cycle
        do
        {
            Job* job = bucket->mJob->mNext; // grab head
            bucket->mJob->mNext = nullptr; // break cycle
            do
            {
                Job* temp = job;
                job = job->mNext;
                delete temp;
            } while (job != nullptr);
            JobBucket* temp = bucket;
            bucket = bucket->mNext;
            // attach bucket to the freelist 
            // (clearBuckets() will dispose of them)
            temp->mNext = mFreeBucket;
            mFreeBucket = temp;
        } while (bucket != nullptr);
        mBucket = nullptr;
    }
}

/*! ***************************************************************************/
/*! \brief  clearBuckets
            Deletes all cached job buckets from the freelist and sets it to nullptr.
    \return void 
*******************************************************************************/
void JobQueue::clearBuckets()
{
    while (mFreeBucket != nullptr)
    {
        JobBucket* temp = mFreeBucket;
        mFreeBucket = mFreeBucket->mNext;
        delete temp;
    }
    mFreeBucket = nullptr;
}

bool JobQueue::empty() const
{
    // we avoid cost of a mutex
    // here because empty() checks
    // a temporary state.
    return (mBucket == nullptr);
}

} // Blaze
