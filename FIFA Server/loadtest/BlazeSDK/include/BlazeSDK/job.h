/*! **********************************************************************************************/
/*!
    \file

    The Job template classes provide a way to define a function call with parameters from
    with an object.  Difference from Functors is that parameters for the call are stored within
    the object versus supplied by the caller on the callback's execution.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*************************************************************************************************/

#ifndef BLAZE_JOB_H
#define BLAZE_JOB_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/jobid.h"
#include "BlazeSDK/blazeerrors.h"
#include "EASTL/intrusive_list.h"
#include "BlazeSDK/callback.h"

namespace Blaze
{

class JobScheduler;

/*! ***************************************************************/
/*! \class JobProvider

\brief Base interface for a class that schedules jobs and wants to track them 
       with an associated id.
*/
class BLAZESDK_API JobProvider
{
public:
    /* currently no methods */
};

/*! ***************************************************************/
/*! \class Job

    \brief The abstract base class for all jobs.
*/
class BLAZESDK_API Job : eastl::intrusive_list_node
{
protected:
    //Make ctor protected so that it only be called by functor defined in methodjob.h
    Job() : mId(INVALID_JOB_ID), mAssociatedObject(nullptr), mExpireTimeMS(0), mDelayMS(0), mProvider(nullptr), mProviderId(0), mIsExecutingCount(0), mAssociatedTitleCbObject(nullptr)
#if ENABLE_BLAZE_SDK_LOGGING
            , mJobName("")
#endif
    {
        mpNext = nullptr;
        mpPrev = nullptr;
    }

public:
    virtual ~Job() {}

    /*! ***************************************************************************/
    /*! \brief This function runs when the job is executed.
    *******************************************************************************/
    virtual void execute() = 0;

    /*! ***************************************************************************/
    /*! \brief This function runs when the job is cancelled.
    *******************************************************************************/
    virtual void cancel(BlazeError err = SDK_ERR_RPC_CANCELED) {}

    /*! ***************************************************************************/
    /*! \brief This function returns the updated expiry time.
    *******************************************************************************/
    virtual uint32_t getUpdatedExpiryTime() { return mExpireTimeMS; }

    /*! ***************************************************************************/
    /*! \brief Gets the id of the job.
    *******************************************************************************/
    JobId getId() const { return mId; }

    /*! ***************************************************************************/
    /*! \brief Sets the id of a job.
    *******************************************************************************/
    void setId(JobId id) { mId = id; }

    /*! ***************************************************************************/
    /*! \brief Specify the object pointer associated with the title callback for the rpc job
    *******************************************************************************/
    void setAssociatedTitleCbObject(const void *obj);

    /*! ***************************************************************************/
    /*! \brief Specify the object pointer associated with the title callback for the rpc job.
               Internally, only the object (this) pointer is stored, not the whole callback.
    *******************************************************************************/
    void setAssociatedTitleCb(const FunctorBase& titleCb);

    /*! ***************************************************************************/
    /*! \brief Gets the associated title callback for the function.  May be nullptr.  
       Generally only useful if the type is already known.
    *******************************************************************************/
    const FunctorBase& getAssociatedTitleCb() const { return mAssociatedTitleCb; }

    /*! ***************************************************************************/
    /*! \brief Determines whether the passed in object is linked to this job's external title cb object or internal cb object.
               Only override this function if you need more than the two default associated objects (AssociatedTitleCb & mAssociatedObject).
    *******************************************************************************/
    virtual bool isAssociatedObject(const void *obj) const;

    /*! ***************************************************************************/
    /*! \brief Link a title-based functor callback to an active job
        This is meant for calls where an API triggers a title callback upon return from 
        a single server call. Actions requiring multiple RPC calls per single job should
        create their own job classes and override isAssociatedObject accordingly. 
    *******************************************************************************/
    static void addTitleCbAssociatedObject(JobScheduler *jobScheduler, const JobId& jobId, const FunctorBase& titleCb);

    /*! ***************************************************************************/
    /*! \brief Gets an object or value associated with this job.
    *******************************************************************************/
    void *getAssociatedObject() const { return mAssociatedObject; }
    
    /*! ***************************************************************************/
    /*! \brief Sets an object or value associated with this job.
    *******************************************************************************/
    void setAssociatedObject(void *obj) { mAssociatedObject = obj;}
    
    /*! ***************************************************************************/
    /*! \brief Gets the time that a job will expire at.    
    *******************************************************************************/
    uint32_t getExpireTimeMS() const { return mExpireTimeMS; }

    /*! ***************************************************************************/
    /*! \brief Gets the amount (in milliseconds) to delay spawning a job.
    *******************************************************************************/
    uint32_t getDelayMS() const { return mDelayMS; }

    /*! ***************************************************************************/
    /*! \brief Sets the time that a job will expire at.    
    *******************************************************************************/
    void setExpireTimeMS(uint32_t delayMS) { mExpireTimeMS = delayMS; }

    /*! ***************************************************************************/
    /*! \brief Sets the amount (in milliseconds) to delay spawning a job.   
    *******************************************************************************/
    void setDelayMS(uint32_t delayMS) { mDelayMS = delayMS; }

    /*! ***************************************************************************/
    /*! \brief Gets the provider of the job.  
    *******************************************************************************/
    JobProvider *getProvider() const { return mProvider; }
    
    /*! ***************************************************************************/
    /*! \brief Sets the provider of the job.  
    *******************************************************************************/
    void setProvider(JobProvider *provider) { mProvider = provider; }

    /*! ***************************************************************************/
    /*! \brief Gets an associated id that ties the job back contextually to its provider.
    *******************************************************************************/
    uint32_t getProviderId() const { return mProviderId; }

    /*! ***************************************************************************/
    /*! \brief Sets an associated id that ties the job back contextually to its provider.
    *******************************************************************************/
    void setProviderId(uint32_t id) { mProviderId = id; }

    /*! ***************************************************************************/
    /*! \brief Flags the job as being executed.  
    
        JobScheduler uses the execution state to prevent external code from cancelling 
        or removing jobs which are already running.
    *******************************************************************************/
    void setExecuting(bool isExecuting) 
    { 
        if (isExecuting)
        {
            ++mIsExecutingCount;
        }
        else if (mIsExecutingCount > 0)
        {
            --mIsExecutingCount; 
        }
    }

    /*! ***************************************************************************/
    /*! \brief Checks whether a job is executing.
    *******************************************************************************/
    bool isExecuting() const { return (mIsExecutingCount > 0); }

#if ENABLE_BLAZE_SDK_LOGGING
    void setJobName(const char8_t *jobName) { mJobName = jobName; }
    const char8_t* getJobName() const       { return mJobName;    }
#else
    void setJobName(const char8_t *jobName) { }
    const char8_t* getJobName() const       { return ""; }
#endif

private:
    friend class eastl::intrusive_list<Job>;
    friend class eastl::intrusive_list_iterator<Job, Job*, Job&>;    

    JobId mId;
    void *mAssociatedObject;
    uint32_t mExpireTimeMS;
    uint32_t mDelayMS;
    JobProvider *mProvider;
    uint32_t mProviderId;
    int32_t mIsExecutingCount;

protected:
    const void *mAssociatedTitleCbObject;   // This is the getObject() portion of the associated callback. May be nullptr if no callback is used. 
    FunctorBase mAssociatedTitleCb;         // This is the Functor itself. Only set by setAssociatedTitleCb(), nullptr otherwise. 

private:
#if ENABLE_BLAZE_SDK_LOGGING
    const char8_t *mJobName;   // Debug name used for profiling
#endif
};

typedef eastl::intrusive_list<Job> JobList;

}       // namespace Blaze

#endif  // BLAZE_JOB_H
