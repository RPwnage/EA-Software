/*! ************************************************************************************************/
/*!
    \file jobscheduler.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#include "BlazeSDK/internal/internal.h"
#include "BlazeSDK/jobscheduler.h"

#include "DirtySDK/dirtysock.h" //For NetTick()

namespace Blaze
{

    JobScheduler::JobScheduler(MemoryGroupId memGroupId) :
        mInIdle(false),
        mNextJobId(MIN_JOB_ID),
        mExecutingJob(nullptr),
        mJobExecuteTimeMax(0)
{
}

JobScheduler::~JobScheduler()
{
    //Kill any jobs we may have scheduled - they are not executed.
    while (!mNoDelayList.empty())
    {
        Job *job = &mNoDelayList.front();
        mNoDelayList.pop_front();
        //FIXME: we need a factory for job to control creating group.
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);
    }

    while (!mDelayList.empty())
    {
        Job *job = &mDelayList.front();
        mDelayList.pop_front();
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);
    }

    while (!mReentrantList.empty())
    {
        Job *job = &mReentrantList.front();
        mReentrantList.pop_front();
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);
    }

    while (!mNoExecuteList.empty())
    {
        Job *job = &mNoExecuteList.front();
        mNoExecuteList.pop_front();
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);
    }
}

JobId JobScheduler::reserveJobId()
{
    JobId result(mNextJobId++);
    if (mNextJobId > MAX_JOB_ID)
        mNextJobId = MIN_JOB_ID;
    return result;
}

JobId JobScheduler::scheduleJobNoTimeout(const char8_t* jobName, Job *job, void *associatedObject /*= nullptr*/, JobId reservedId /*= INVALID_JOB_ID*/)
{
    job->setJobName(jobName);
    return scheduleJobNoTimeout(job, associatedObject, reservedId);
}

JobId JobScheduler::scheduleJob(const char8_t* jobName, Job *job, void *associatedObject /* = nullptr */, uint32_t delayMS /* = 0 */, JobId reservedId /* = INVALID_JOB_ID */)
{
    job->setJobName(jobName);
    return scheduleJob(job, associatedObject, delayMS, reservedId);
}

JobId JobScheduler::scheduleJobNoTimeout(Job *job, void *associatedObject /*= nullptr*/, JobId reservedId /*= INVALID_JOB_ID*/)
{
    return scheduleJobInternal(job, associatedObject, false, 0, reservedId);
}

JobId JobScheduler::scheduleJob(Job *job, void *associatedObject /* = nullptr */, uint32_t delayMS /* = 0 */, JobId reservedId /* = INVALID_JOB_ID */)
{
    return scheduleJobInternal(job, associatedObject, true, delayMS, reservedId);
}

JobId JobScheduler::scheduleJobInternal(Job *job, void *associatedObject, bool executeAfterDelay, uint32_t delayMS, JobId reservedId)
{
    if (associatedObject != nullptr)
        job->setAssociatedObject(associatedObject);

    //Handle the id.
    if (job->getId() == INVALID_JOB_ID)
    {
        if (reservedId == INVALID_JOB_ID)
            job->setId(reserveJobId());
        else
            job->setId(reservedId);
    }

    //A 0 wait is a special case - there are a lot of these so we file them in a different list.
    if (executeAfterDelay)
    {
        if (delayMS == 0)
        {
            if (!mInIdle)
            {
                mNoDelayList.push_back(*job);
            }
            else //if we're currently idling, we want to push this on the special "reentrant" list
            {
                mReentrantList.push_back(*job);
            }

            //Make sure our wait flag is "0"
            JobId id = job->getId();
            id.clearWaitFlag();
            job->setId(id);
        }
        else
        {
            job->setExpireTimeMS(NetTick() + delayMS);
            job->setDelayMS(delayMS);
            if (!mInIdle)
            {
                insertDelayedJob(job);
            }
            else
            {
                mReentrantList.push_back(*job);
            }

            //Make sure our wait flag is "1"
            JobId id = job->getId();
            id.setWaitFlag();
            job->setId(id);
        }
    }
    else
    {
        mNoExecuteList.push_back(*job);

        // There's no good default here for the wait UI flag.
        // It's possible that the no-execute Job is waiting on another system to return (DS/1st party), 
        // but it also may be an Async job that just runs in the background with no need for the UI indicator. 
        JobId id = job->getId();
        id.clearWaitFlag();
        job->setId(id);
    }

    return job->getId();
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

//Predicate objects to use in remove/get functions.
struct IdPredicate
{
    IdPredicate(const JobId& id) : mId(id) {}
    bool operator() (Job &job) const { return job.getId() == mId; }
private:
    JobId mId;
};

struct AssociatedObjectPredicate
{
    AssociatedObjectPredicate(void *associatedObject) : mAssociatedObject(associatedObject) {}
    bool operator() (Job &job) const { return job.isAssociatedObject(mAssociatedObject); }
private:
    void *mAssociatedObject;
};

struct JobProviderIdPredicate
{
    JobProviderIdPredicate(JobProvider *provider, uint32_t providerId) : mProvider(provider), mProviderId(providerId) {}
    bool operator() (Job &job) const { return job.getProvider() == mProvider && job.getProviderId() == mProviderId; }
private:
    JobProvider *mProvider;
    uint32_t mProviderId;
};

struct JobProviderPredicate
{
    JobProviderPredicate(JobProvider *provider) : mProvider(provider) {}
    bool operator() (Job &job) const { return job.getProvider() == mProvider; }
private:
    JobProvider *mProvider;
};



Job *JobScheduler::getJob(const JobId& id)
{
    IdPredicate p(id);

    JobList::iterator i = eastl::find_if(mNoDelayList.begin(), mNoDelayList.end(), p);
    if (i != mNoDelayList.end()) return &(*i);

    i = eastl::find_if(mDelayList.begin(), mDelayList.end(), p);
    if (i != mDelayList.end()) return &(*i);

    i = eastl::find_if(mReentrantList.begin(), mReentrantList.end(), p);
    if (i != mReentrantList.end()) return &(*i);

    i = eastl::find_if(mNoExecuteList.begin(), mNoExecuteList.end(), p);
    if (i != mNoExecuteList.end()) return &(*i);

    if (mExecutingJob != nullptr && mExecutingJob->getId() == id)
        return mExecutingJob;

    return nullptr;
}

Job *JobScheduler::getJob(JobProvider *provider, uint32_t providerId)
{
    JobProviderIdPredicate p(provider, providerId);
    
    JobList::iterator i = eastl::find_if(mNoDelayList.begin(), mNoDelayList.end(), p);
    if (i != mNoDelayList.end()) return &(*i);
    
    i = eastl::find_if(mDelayList.begin(), mDelayList.end(), p);
    if (i != mDelayList.end()) return &(*i);
    
    i = eastl::find_if(mReentrantList.begin(), mReentrantList.end(), p);
    if (i != mReentrantList.end()) return &(*i);

    i = eastl::find_if(mNoExecuteList.begin(), mNoExecuteList.end(), p);
    if (i != mNoExecuteList.end()) return &(*i);

    if (mExecutingJob != nullptr && mExecutingJob->getProvider() == provider && mExecutingJob->getProviderId() == providerId)
        return mExecutingJob;

    return nullptr;
}


// erase & delete the job(s) matching the predicate
// if doCancel is true, we dispatch the job with err_cancel before deleting them
// if deleteAllInstances is false, we return after finding the first instance (when there can only be 1 instance in the list).
template <typename Predicate>
bool JobScheduler::erase_and_delete_if(JobList &list, Predicate p, bool doCancel, BlazeError cancelErr, bool deleteAllInstances)
{
    JobList::iterator itr = list.begin();
    bool found = false;

    for (; itr != list.end(); )
    {
        Job *job = &(*itr);
        if (p(*job))
        {
            if (!job->isExecuting())
            {
                if (doCancel)
                {
                    internalCancelJob(job, cancelErr);
                }

                BLAZE_SDK_DEBUGSB("[JobScheduler::" << this << "].erase_and_delete_if: erased job [" << job->getJobName() << "]");

                itr = list.erase(itr);

                size_t origSize = list.size();
                BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);
                if (origSize != list.size())
                {
                    // The above job's delete itself removed a job from list. To ensure itr valid, restart at begin.
                    itr = list.begin();
                }
            }
            else
            {
                ++itr;
            }
            if (!deleteAllInstances)
                return true;
            found = true;
        }
        else
        {
            ++itr;
        }
    }

    return found;
}  /*lint !e1746 Avoid lint to check whether parameter could be made const reference*/

void JobScheduler::cancelJob(const JobId& id, BlazeError err)
{
    // cancel a single job by id - the job can only be in a single list, so bail once we find it.
    IdPredicate p(id);
    if (erase_and_delete_if<IdPredicate>(mNoDelayList, p, true, err, false))
        return;
    if (erase_and_delete_if<IdPredicate>(mDelayList, p, true, err, false))
        return;
    if (erase_and_delete_if<IdPredicate>(mReentrantList, p, true, err, false))
        return;
    if (erase_and_delete_if<IdPredicate>(mNoExecuteList, p, true, err, false))
        return;
}

void JobScheduler::internalStartJobExecuting(Job *job)
{
    mExecutingJob = job;
#if ENABLE_BLAZE_SDK_PROFILING
    Debug::startTimer(job->getJobName());
#endif
    job->setExecuting(true);
}
void JobScheduler::internalEndJobExecuting(Job *job)
{
    job->setExecuting(false);
#if ENABLE_BLAZE_SDK_PROFILING
    Debug::stopTimer(); //  job->getJobName();
#endif
    mExecutingJob = nullptr;
}

void JobScheduler::internalCancelJob(Job *job, BlazeError err)
{
    internalStartJobExecuting(job);
    job->cancel(err);
    internalEndJobExecuting(job);
}

void JobScheduler::cancelJob(Job *job, BlazeError err, bool deleteAfterCancel)
{
    JobList::remove(*job);
    internalCancelJob(job, err);

    if (deleteAfterCancel)
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);
}

void JobScheduler::cancelJob(JobProvider *provider, uint32_t providerId, BlazeError err)
{
    // cancel all jobs for the supplied provider
    JobProviderIdPredicate p(provider, providerId);
    erase_and_delete_if<JobProviderIdPredicate>(mNoDelayList, p, true, err);
    erase_and_delete_if<JobProviderIdPredicate>(mDelayList, p, true, err);
    erase_and_delete_if<JobProviderIdPredicate>(mReentrantList, p, true, err);
    erase_and_delete_if<JobProviderIdPredicate>(mNoExecuteList, p, true, err);
}


void JobScheduler::cancelJob(JobProvider *provider, BlazeError err)
{
    // cancel all jobs for the supplied provider
    JobProviderPredicate p(provider);
    erase_and_delete_if<JobProviderPredicate>(mNoDelayList, p, true, err);
    erase_and_delete_if<JobProviderPredicate>(mDelayList, p, true, err);
    erase_and_delete_if<JobProviderPredicate>(mReentrantList, p, true, err);
    erase_and_delete_if<JobProviderPredicate>(mNoExecuteList, p, true, err);
}


void JobScheduler::cancelByAssociatedObject(void *associatedObject, BlazeError err)
{  
    // cancel all jobs on the assoicated obj
    AssociatedObjectPredicate p(associatedObject);
    erase_and_delete_if<AssociatedObjectPredicate>(mNoDelayList, p, true, err);
    erase_and_delete_if<AssociatedObjectPredicate>(mDelayList, p, true, err);
    erase_and_delete_if<AssociatedObjectPredicate>(mReentrantList, p, true, err);
    erase_and_delete_if<AssociatedObjectPredicate>(mNoExecuteList, p, true, err);
}

void JobScheduler::removeJob(const JobId& id)
{
    // remove a single job by id - the job can only be in a single list, so bail once we find it.
    IdPredicate p(id);
    if (erase_and_delete_if<IdPredicate>(mNoDelayList, p, false, ERR_OK, false))
        return;
    if (erase_and_delete_if<IdPredicate>(mDelayList, p, false, ERR_OK, false))
        return;
    if (erase_and_delete_if<IdPredicate>(mReentrantList, p, false, ERR_OK, false))
        return;
    if (erase_and_delete_if<IdPredicate>(mNoExecuteList, p, false, ERR_OK, false))
        return;
}

void JobScheduler::removeJob(Job *job, bool deleteAfterRemoval /* = true */) const
{
    //The static remove takes us out of all lists.
    JobList::remove(*job);

    if (deleteAfterRemoval)
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);    
}

void JobScheduler::removeJob(JobProvider *provider, uint32_t providerId)
{
    // remove all jobs for the supplied provider
    JobProviderIdPredicate p(provider, providerId);
    erase_and_delete_if<JobProviderIdPredicate>(mNoDelayList, p, false, ERR_OK);
    erase_and_delete_if<JobProviderIdPredicate>(mDelayList, p, false, ERR_OK);
    erase_and_delete_if<JobProviderIdPredicate>(mReentrantList, p, false, ERR_OK);
    erase_and_delete_if<JobProviderIdPredicate>(mNoExecuteList, p, false, ERR_OK);
}

void JobScheduler::removeJob(JobProvider *provider)
{
    // remove all jobs for the supplied provider
    JobProviderPredicate p(provider);
    erase_and_delete_if<JobProviderPredicate>(mNoDelayList, p, false, ERR_OK);
    erase_and_delete_if<JobProviderPredicate>(mDelayList, p, false, ERR_OK);
    erase_and_delete_if<JobProviderPredicate>(mReentrantList, p, false, ERR_OK);
    erase_and_delete_if<JobProviderPredicate>(mNoExecuteList, p, false, ERR_OK);
}

void JobScheduler::removeByAssociatedObject(void *associatedObject)
{  
    // remove all jobs with the associated object
    AssociatedObjectPredicate p(associatedObject);
    erase_and_delete_if<AssociatedObjectPredicate>(mNoDelayList, p, false, ERR_OK);
    erase_and_delete_if<AssociatedObjectPredicate>(mDelayList, p, false, ERR_OK);
    erase_and_delete_if<AssociatedObjectPredicate>(mReentrantList, p, false, ERR_OK);
    erase_and_delete_if<AssociatedObjectPredicate>(mNoExecuteList, p, false, ERR_OK);
}

void JobScheduler::internalExecuteJob(Job *job)
{
    uint32_t jobStartTime = NetTick();
    internalStartJobExecuting(job);
    job->execute();
    internalEndJobExecuting(job);

    uint32_t timePassedMs = NetTickDiff(NetTick(), jobStartTime);
    if (timePassedMs > mJobExecuteTimeMax && mJobExecuteTimeMax != 0)
    {
        BLAZE_SDK_DEBUGF("BlazeHub::Idle >> Excessive idle in job '%s', Processing time - %d ms.\n", job->getJobName(), timePassedMs);
    }
}

void JobScheduler::idle(const uint32_t currentTime, const uint32_t elapsedTime)
{
    mInIdle = true;

    //First execute all the 0 delay jobs.
    while (!mNoDelayList.empty())
    {
        Job *job = &mNoDelayList.front();
        mNoDelayList.pop_front();
        internalExecuteJob(job);
        BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);
    }

    //Now execute all the delayed jobs who have expired
    while (!mDelayList.empty() && (NetTickDiff(currentTime, mDelayList.front().getExpireTimeMS()) > 0))
    {
        Job *job = &mDelayList.front();
        mDelayList.pop_front();

        // Extend the expiry time and re-insert the job into the list if its expiry time is renewed
        if (NetTickDiff(currentTime, job->getUpdatedExpiryTime()) < 0)
        {
            job->setExpireTimeMS(job->getUpdatedExpiryTime());
            insertDelayedJob(job);
        }
        else
        {
            internalExecuteJob(job);
            BLAZE_DELETE(MEM_GROUP_FRAMEWORK_TEMP, job);
        }
    }

    //Finally lets sort out any jobs scheduled during these job executions
    while (!mReentrantList.empty())
    {
        Job *job = &mReentrantList.front();
        mReentrantList.pop_front();
        if (job->getExpireTimeMS() == 0)
        {
            mNoDelayList.push_back(*job);
        }
        else
        {
            insertDelayedJob(job);
        }
    }

    mInIdle = false;
}

/*! ***************************************************************************/
/*!    \brief Puts a delayed job into the sorted delayed job list.

    \param job The job to insert.
*******************************************************************************/
void JobScheduler::insertDelayedJob(Job *job)
{
    //Insert the job into the list in sorted order.
    JobList::iterator itr = mDelayList.begin();
    JobList::iterator end = mDelayList.end();
    for (; itr != end; ++itr)
    {
        if (NetTickDiff(job->getExpireTimeMS(), (*itr).getExpireTimeMS()) < 0)
        {
            mDelayList.insert(itr, *job);
            break;
        }
    }
    if (itr == end)
    {
        mDelayList.push_back(*job);
    }
}

}        // namespace Blaze
