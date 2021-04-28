/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_JOBQUEUE_H
#define BLAZE_JOBQUEUE_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_condition.h"
#include "eathread/eathread_mutex.h"
#include "framework/system/job.h"
#include "EATDF/time.h"

// #define JOB_QUEUE_PERF_METRICS // uncomment to enable metrics

namespace Blaze
{

struct JobBucket;

class JobQueue
{
    NON_COPYABLE(JobQueue);
public:
    JobQueue(const char8_t* name);
    ~JobQueue();
    void push(Job* job);
    Job* popOne(const EA::TDF::TimeValue& timeout);
    Job* popMany();
    void stop();
    void wakeAll();
    bool removeJob(Job* job);
    void removeAll();
    bool empty() const;
    
private:
    void clearJobs();
    void clearBuckets();
    JobBucket* allocateBucket();
    void insertAfter(JobBucket* bucket, Job* job);
    static void insertInto(JobBucket* bucket, Job* job);

private:
    bool mStopped;
    JobBucket* mBucket;
    JobBucket* mFreeBucket;
    mutable EA::Thread::Mutex mMutex;
    EA::Thread::Condition mCondition;
    const char8_t* mName;
    
#ifdef JOB_QUEUE_PERF_METRICS
    uint32_t mQueueSize;
    uint32_t mQueueMax;
    uint32_t mQueueMaxReached;
    uint32_t mQueueBuckets;
    uint32_t mQueueAddBeforeHead;
    uint32_t mQueueAddAtHead;
    uint32_t mQueueAddAfterTail;
    uint32_t mQueueAddAtTail;
    uint32_t mQueueAddBetween;
    uint32_t mQueueAddBetweenDepth;
    uint32_t mQueueAddNew;
#endif
};

} // Blaze

#endif // BLAZE_JOBQUEUE_H
