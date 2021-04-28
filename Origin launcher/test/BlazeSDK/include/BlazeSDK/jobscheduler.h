/*! ************************************************************************************************/
/*!
    \file jobscheduler.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*! ************************************************************************************************/

#ifndef BLAZE_JOB_SCHEDULER_H
#define BLAZE_JOB_SCHEDULER_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#include "BlazeSDK/blazesdk.h"
#include "BlazeSDK/callback.h"
#include "BlazeSDK/jobid.h"
#include "BlazeSDK/job.h"
#include "BlazeSDK/methodjob.h"
#include "BlazeSDK/idler.h"
#include "BlazeSDK/allocdefines.h"

#include "blaze_eastl/vector_map.h"
#include <EASTL/intrusive_list.h>

namespace Blaze
{
 
/*! ***************************************************************/
/*! \class JobScheduler

    \brief The JobScheduler allows any class to schedule events off the BlazeHub's idle
    function.  The basic unit of work is the Job, and helper functions are provided
    to specifically schedule functor callbacks and method calls.

    \details
    All jobs are owned by the scheduler after being scheduled.  The scheduler will handle
    deleting the jobs.  This means that the job MUST be created with the BLAZE_NEW macro, or
    the job will fail to be deleted from the proper heap.  If the scheduler is destroyed, any
    outstanding jobs will simply be deleted.  They will NOT be executed.  If a job is scheduled
    during an idle of the scheduler, it is always delayed until the next idle, no matter what the 
    delay passed time is.  Jobs scheduled with a delay of "0" will always be run the next time the 
    idler is run.

*/
class BLAZESDK_API JobScheduler : public Idler
{
public:
    /*! ***************************************************************************/
    /*!    \brief Constructor.
    
        \param[in] memGroupId mem group to be used by this class to allocate memory
    *******************************************************************************/
    JobScheduler(MemoryGroupId memGroupId = MEM_GROUP_FRAMEWORK_TEMP);

    /*! ***************************************************************************/
    /*!    \brief Destructor.
    *******************************************************************************/
    virtual ~JobScheduler();

    /*! ***************************************************************************/
    /*!    \brief Allocates a job id for use by a job or collection of jobs. 
    
        \return The job id.
    *******************************************************************************/
    JobId reserveJobId();

    /*! ***************************************************************************/
    /*!    \brief Schedules a job.
    
        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param job  Pointer to the job to schedule.  The scheduler takes ownership of the job memory.
        \param associatedObject  An object associated with the job.  Used to cancel by object.
        \param delayMS The amount (in milliseconds) to delay spawning a job.  "0" means next idle loop.
        \param reservedId The id to assign the job.  If the value is INVALID_JOB_ID, a new id is reserved. (Unused
                            if the job already has an id).
        \return The id of the job.
    *******************************************************************************/
    JobId scheduleJob(const char8_t* jobName, Job *job, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID);
    JobId scheduleJob(Job *job, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID);

    /*! ***************************************************************************/
    /*!    \brief Schedules a job that does not include an execution delay / timeout.
    
        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param job  Pointer to the job to schedule.  The scheduler takes ownership of the job memory.
        \param associatedObject  An object associated with the job.  Used to cancel by object.
        \param reservedId The id to assign the job.  If the value is INVALID_JOB_ID, a new id is reserved. (Unused
                            if the job already has an id).
        \return The id of the job.
    *******************************************************************************/
    JobId scheduleJobNoTimeout(const char8_t* jobName, Job *job, void *associatedObject = nullptr, JobId reservedId = INVALID_JOB_ID);
    JobId scheduleJobNoTimeout(Job *job, void *associatedObject = nullptr, JobId reservedId = INVALID_JOB_ID);


    /*! ***************************************************************************/
    /*!    \brief Gets the job with the given id.

        \param id The id of the job to get.
        \return The job or nullptr if not found.
    *******************************************************************************/
    Job* getJob(const JobId& id);

    /*! ***************************************************************************/
    /*!    \brief Gets a job with the given provider/provider id pair.

        \param provider The provider of the job.
        \param providerId The associated provider id of the job.
        \return The job, or nullptr if not found.
    *******************************************************************************/
    Job* getJob(JobProvider *provider, uint32_t providerId);

    /*! ***************************************************************************/
    /*!    \brief Cancels the job with the given job id. The job's cancel method is executed.
    
        \param id The id of the job to cancel.
        \param err BlazeError return to callback.
    *******************************************************************************/
    void cancelJob(const JobId& id, BlazeError err = SDK_ERR_RPC_CANCELED);

    /*! ***************************************************************************/
    /*!    \brief Cancels the given job. The job's cancel method is executed.

        \param job The job to cancel.
        \param err BlazeError return to callback.
        \param deleteAfterCancel If true deletes the job after canceling.
    *******************************************************************************/
    void cancelJob(Job *job, BlazeError err = SDK_ERR_RPC_CANCELED, bool deleteAfterCancel = true);

    /*! ***************************************************************************/
    /*!    \brief Cancels the job with the matching provider/provider id. 
            The job's cancel method is executed.

    
        \param provider The job provider that started the job.
        \param providerId The associated provider id.
        \param err BlazeError return to callback.
    *******************************************************************************/
    void cancelJob(JobProvider *provider, uint32_t providerId, BlazeError err = SDK_ERR_RPC_CANCELED);

    /*! ***************************************************************************/
    /*!    \brief Cancels the job with the matching provider. 
            The job's cancel method is executed.

    
        \param provider The job provider that started the job.
        \param err BlazeError return to callback.
    *******************************************************************************/
    void cancelJob(JobProvider *provider, BlazeError err = SDK_ERR_RPC_CANCELED);

    /*! ***************************************************************************/
    /*!    \brief Cancels any job with the given associatedObject.  The job's cancel method is executed.

        \param associatedObject The associatedObject to cancel events for. Must be non-nullptr.
        \param err BlazeError return to callback.
    *******************************************************************************/
    void cancelByAssociatedObject(void *associatedObject, BlazeError err = SDK_ERR_RPC_CANCELED);

    /*! ***************************************************************************/
    /*!    \brief Removes the job with the given job id. The job's cancel method is not executed.

        \param id The id of the job to remove.
    *******************************************************************************/
    void removeJob(const JobId& id);

    /*! ***************************************************************************/
    /*!    \brief Removes the given job. The job's cancel method is not executed.

       \param job The job to remove.  
       \param deleteAfterRemoval Deletes the job after removing it.
    *******************************************************************************/
    void removeJob(Job *job, bool deleteAfterRemoval = true) const;

    /*! ***************************************************************************/
    /*!    \brief Removes the job with the matching provider/provider id. 
         The job's cancel method is not executed.

        \param provider The job provider that started the job.
        \param providerId The associated provider id. 
    *******************************************************************************/
    void removeJob(JobProvider *provider, uint32_t providerId);

    /*! ***************************************************************************/
    /*!    \brief Removes all jobs from the given provider.
    
        \param provider The provider for which to remove jobs.
    *******************************************************************************/
    void removeJob(JobProvider *provider);

    /*! ***************************************************************************/
    /*!    \brief Removes any job with the given associated object.  The job's cancel method is not executed.

        \param associatedObject The associated object to cancel jobs for. Must be non-nullptr.
    *******************************************************************************/
    void removeByAssociatedObject(void *associatedObject);


    /*! ***************************************************************************/
    /*!    \brief Schedules a no parameter functor for execution.
    
        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param f The functor to schedule.
        \param associatedObject An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \param reservedId The id to assign the job.  If the value is INVALID_JOB_ID, a new id is reserved.
        \return The id of the job.
    *******************************************************************************/
    JobId scheduleFunctor(const char8_t* jobName, Functor &f, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID);

    /*! ***************************************************************************/
    /*!    \brief Schedules a one parameter for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param f The functor to schedule.
        \param p1 The 1st parameter to be passed to the functor when executed.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \param reservedId The id to assign the job.  If the value is INVALID_JOB_ID, a new id is reserved.
        \return The id of the job.
    *******************************************************************************/
    template <typename P1> 
    JobId scheduleFunctor(const char8_t* jobName, const Functor1<P1> &f, P1 p1, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID);

    /*! ***************************************************************************/
    /*!    \brief Schedules a two parameter functor for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param f The functor to schedule.
        \param p1 The 1st parameter to be passed to the functor.
        \param p2 The 2nd parameter to be passed to the functor.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \param reservedId The id to assign the job.  If the value is INVALID_JOB_ID, a new id is reserved.
        \return The id of the job.
    *******************************************************************************/
    template <typename P1, typename P2>
    JobId scheduleFunctor(const char8_t* jobName, const Functor2<P1, P2> &f, P1 p1, P2 p2, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID);

    /*! ***************************************************************************/
    /*!    \brief Schedules a three parameter functor for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param f The functor to schedule.
        \param p1 The 1st parameter to be passed to the functor.
        \param p2 The 2nd parameter to be passed to the functor.
        \param p3 The 3rd parameter to be passed to the functor.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \param reservedId The id to assign the job.  If the value is INVALID_JOB_ID, a new id is reserved.
        \return The id of the job.
    *******************************************************************************/
    template <typename P1, typename P2, typename P3>
    JobId scheduleFunctor(const char8_t* jobName, const Functor3<P1, P2, P3> &f, P1 p1, P2 p2, P3 p3, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID);

    /*! ***************************************************************************/
    /*!    \brief Schedules a four parameter functor for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
    \param f The functor to schedule.
    \param p1 The 1st parameter to be passed to the functor.
    \param p2 The 2nd parameter to be passed to the functor.
    \param p3 The 3rd parameter to be passed to the functor.
    \param p4 The 4th parameter to be passed to the functor.
    \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
    \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \param reservedId The id to assign the job.  If the value is INVALID_JOB_ID, a new id is reserved.
    \return The id of the job.
    *******************************************************************************/
    template <typename P1, typename P2, typename P3, typename P4>
    JobId scheduleFunctor(const char8_t* jobName, const Functor4<P1, P2, P3, P4> &f, P1 p1, P2 p2, P3 p3, P4 p4, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID);

    /*! ***************************************************************************/
    /*!    \brief Schedules a no parameter method for execution.
    
        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param obj The object to run the method on.
        \param mfp The method to execute.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \return The id of the job.
    *******************************************************************************/
    template <class T>
    JobId scheduleMethod(const char8_t* jobName, T *obj, void (T::*mfp)(), void *associatedObject = nullptr, uint32_t delayMS = 0);

    /*! ***************************************************************************/
    /*!    \brief Schedules a no parameter const method for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param obj The object to run the method on.
        \param mfp The method to execute.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \return The id of the job.
    *******************************************************************************/
    template <class T>
    JobId scheduleMethod(const char8_t* jobName, const T *obj, void (T::*mfp)() const, void *associatedObject = nullptr, uint32_t delayMS = 0);

    /*! ***************************************************************************/
    /*!    \brief Schedules a one parameter method for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param obj The object to run the method on.
        \param p1 The 1st parameter to pass to the method.
        \param mfp The method to execute.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \return The id of the job.
    *******************************************************************************/
    template <class T, typename P1>
    JobId scheduleMethod(const char8_t* jobName, T *obj, void (T::*mfp)(P1), P1 p1, void *associatedObject = nullptr, uint32_t delayMS = 0);

    /*! ***************************************************************************/
    /*!    \brief Schedules a one parameter const method for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param obj The object to run the method on.
        \param p1 The 1st parameter to pass to the method.
        \param mfp The method to execute.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \return The id of the job.
    *******************************************************************************/
    template <class T, typename P1>
    JobId scheduleMethod(const char8_t* jobName, const T *obj, void (T::*mfp)(P1) const, P1 p1, void *associatedObject = nullptr, uint32_t delayMS = 0);

    /*! ***************************************************************************/
    /*!    \brief Schedules a two parameter method for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param obj The object to run the method on.
        \param p1 The 1st parameter to pass to the method.
        \param p2 The 2nd parameter to pass to the method.
        \param mfp The method to execute.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \return The id of the job.
    *******************************************************************************/
    template <class T, typename P1, typename P2>
    JobId scheduleMethod(const char8_t* jobName, T *obj, void (T::*mfp)(P1, P2), P1 p1, P2 p2, void *associatedObject = nullptr, uint32_t delayMS = 0);

    /*! ***************************************************************************/
    /*!    \brief Schedules a two parameter const method for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param obj The object to run the method on.
        \param p1 The 1st parameter to pass to the method.
        \param p2 The 2nd parameter to pass to the method.
        \param mfp The method to execute.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \return The id of the job.
    *******************************************************************************/
    template <class T, typename P1, typename P2>
    JobId scheduleMethod(const char8_t* jobName, const T *obj, void (T::*mfp)(P1, P2) const, P1 p1, P2 p2, void *associatedObject = nullptr, uint32_t delayMS = 0);

    /*! ***************************************************************************/
    /*!    \brief Schedules a three parameter method for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param obj The object to run the method on.
        \param p1 The 1st parameter to pass to the method.
        \param p2 The 2nd parameter to pass to the method.
        \param p3 The 3rd parameter to pass to the method.
        \param mfp The method to execute.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \return The id of the job.
    *******************************************************************************/
    template <class T, typename P1, typename P2, typename P3>
    JobId scheduleMethod(const char8_t* jobName, T *obj, void (T::*mfp)(P1, P2, P3), P1 p1, P2 p2, P3 p3, void *associatedObject = nullptr, uint32_t delayMS = 0);
    

    /*! ***************************************************************************/
    /*!    \brief Schedules a three parameter const method for execution.

        \param jobName  Const string name for the job.  No allocation is used internally, the string must *not* be dynamically allocated.
        \param obj The object to run the method on.
        \param p1 The 1st parameter to pass to the method.
        \param p2 The 2nd parameter to pass to the method.
        \param p3 The 3rd parameter to pass to the method.
        \param mfp The method to execute.
        \param associatedObject  An associated object for the job.  Used to cancel a set of jobs by object.
        \param delayMS The amount (in milliseconds) to delay before execution..  "0" means next idle loop.
        \return The id of the job.
    *******************************************************************************/
    template <class T, typename P1, typename P2, typename P3>
    JobId scheduleMethod(const char8_t* jobName, const T *obj, void (T::*mfp)(P1, P2, P3) const, P1 p1, P2 p2, P3 p3, void *associatedObject = nullptr, uint32_t delayMS = 0);

    /*idler interface*/
    void idle(const uint32_t currentTime, const uint32_t elapsedTime);

    /*! ***************************************************************************/
    /*!    \brief Helper function to determine if Blaze currently running any jobs. 

        \return True if any pending jobs exist, false otherwise.
    *******************************************************************************/
    bool hasPendingJobs() { return !mNoDelayList.empty() || !mDelayList.empty() || !mReentrantList.empty(); }

    void setMaxJobExecuteTime(uint32_t maxJobExecuteMs) { mJobExecuteTimeMax = maxJobExecuteMs; }

public:

    // Back compat versions of all the schedule functions, now that the defaults take a jobName:
    JobId scheduleFunctor(Functor &f, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID) 
    { return scheduleFunctor("unknownf0", f, associatedObject, delayMS, reservedId); }
    template <typename P1> 
    JobId scheduleFunctor(const Functor1<P1> &f, P1 p1, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID) 
    { return scheduleFunctor("unknownf1", f, p1, associatedObject, delayMS, reservedId); }
    template <typename P1, typename P2> 
    JobId scheduleFunctor(const Functor2<P1, P2> &f, P1 p1, P2 p2, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID) 
    { return scheduleFunctor("unknownf2", f, p1, p2, associatedObject, delayMS, reservedId); }
    template <typename P1, typename P2, typename P3> 
    JobId scheduleFunctor(const Functor3<P1, P2, P3> &f, P1 p1, P2 p2, P3 p3, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID) 
    { return scheduleFunctor("unknownf3", f, p1, p2, p3, associatedObject, delayMS, reservedId); }
    template <typename P1, typename P2, typename P3, typename P4> 
    JobId scheduleFunctor(const Functor4<P1, P2, P3, P4> &f, P1 p1, P2 p2, P3 p3, P4 p4, void *associatedObject = nullptr, uint32_t delayMS = 0, JobId reservedId = INVALID_JOB_ID) 
    { return scheduleFunctor("unknownf4", f, p1, p2, p3, p4, associatedObject, delayMS, reservedId); }

    template <class T>
    JobId scheduleMethod(T *obj, void (T::*mfp)(), void *associatedObject = nullptr, uint32_t delayMS = 0)
    { return scheduleMethod("unknownm0", obj, mfp, associatedObject, delayMS); }
    template <class T>
    JobId scheduleMethod(const T *obj, void (T::*mfp)() const, void *associatedObject = nullptr, uint32_t delayMS = 0)
    { return scheduleMethod("unknownmc0", obj, mfp, associatedObject, delayMS); }
    template <class T, typename P1>
    JobId scheduleMethod(T *obj, void (T::*mfp)(P1), P1 p1, void *associatedObject = nullptr, uint32_t delayMS = 0)
    { return scheduleMethod("unknownm1", obj, mfp, p1, associatedObject, delayMS); }
    template <class T, typename P1>
    JobId scheduleMethod(const T *obj, void (T::*mfp)(P1) const, P1 p1, void *associatedObject = nullptr, uint32_t delayMS = 0) 
    { return scheduleMethod("unknownmc1", obj, mfp, p1, associatedObject, delayMS); }
    template <class T, typename P1, typename P2>
    JobId scheduleMethod(T *obj, void (T::*mfp)(P1, P2), P1 p1, P2 p2, void *associatedObject = nullptr, uint32_t delayMS = 0) 
    { return scheduleMethod("unknownm2", obj, mfp, p1, p2, associatedObject, delayMS); }
    template <class T, typename P1, typename P2>
    JobId scheduleMethod(const T *obj, void (T::*mfp)(P1, P2) const, P1 p1, P2 p2, void *associatedObject = nullptr, uint32_t delayMS = 0) 
    { return scheduleMethod("unknownmc2", obj, mfp, p1, p2, associatedObject, delayMS); }
    template <class T, typename P1, typename P2, typename P3>
    JobId scheduleMethod(T *obj, void (T::*mfp)(P1, P2, P3), P1 p1, P2 p2, P3 p3, void *associatedObject = nullptr, uint32_t delayMS = 0) 
    { return scheduleMethod("unknownm3", obj, mfp, p1, p2, p3, associatedObject, delayMS); }
    template <class T, typename P1, typename P2, typename P3>
    JobId scheduleMethod(const T *obj, void (T::*mfp)(P1, P2, P3) const, P1 p1, P2 p2, P3 p3, void *associatedObject = nullptr, uint32_t delayMS = 0) 
    { return scheduleMethod("unknownmc3", obj, mfp, p1, p2, p3, associatedObject, delayMS); }


private:
    void insertDelayedJob(Job *job);
    JobId scheduleJobInternal(Job *job, void *associatedObject, bool executeAfterDelay, uint32_t delayMS, JobId reservedId);

    void internalStartJobExecuting(Job *job);
    void internalExecuteJob(Job *job);
    void internalEndJobExecuting(Job *job);
    void internalCancelJob(Job *job, BlazeError err);

    template <typename Predicate>
    bool erase_and_delete_if(JobList &list, Predicate p, bool doCancel, BlazeError cancelErr, bool deleteAllInstances = true);

    friend class BlazeSender;
    JobList mNoExecuteList;     // These jobs do not count down or delay. They must be managed externally.
    JobList mNoDelayList;    
    JobList mDelayList;
    JobList mReentrantList;
    bool mInIdle;

    uint32_t mNextJobId;
    Job* mExecutingJob;
    uint32_t mJobExecuteTimeMax;
};


class FunctorCallJob : public Job
{
public:
    void execute()
    {
        if (mFun.isValid())
        {
            mFun();
            mFun.clear();
            setAssociatedObject(nullptr);
        }
    }

protected:
    friend class Blaze::JobScheduler;
    FunctorCallJob(Functor& fn)
    {
        mFun = fn;
        setAssociatedObject(mFun.getObject());  // Note: We do not use setAssociatedTitleCb() here because the callback may be owned by Blaze, not the title code.  
    }

private:
    Functor mFun;

    NON_COPYABLE(FunctorCallJob);
};


template<typename P1>
class FunctorCallJob1: public Job
{
public:
    typedef Functor1<P1> TFunctorType;
    virtual void execute()
    {
        if (mFun.isValid())
        {
            mFun(mP1);
            mFun.clear();
            setAssociatedObject(nullptr);
        }
    }

protected:
    friend class Blaze::JobScheduler;
    FunctorCallJob1(const TFunctorType& fn, P1 p1) :
        mP1(p1)
    {
        mFun = fn;
        setAssociatedObject(mFun.getObject());
    }

private:
    P1 mP1;
    TFunctorType mFun;

    NON_COPYABLE(FunctorCallJob1);
};

template<typename P1, typename P2>
class FunctorCallJob2: public Job
{
public:
    typedef Functor2<P1, P2> TFunctorType;
    virtual void execute()
    {
        if (mFun.isValid())
        {
            mFun(mP1, mP2);
            mFun.clear();
            setAssociatedObject(nullptr);
        }
    }

protected:
    friend class Blaze::JobScheduler;
    FunctorCallJob2( const TFunctorType& fn, P1 p1, P2 p2):
        mP1(p1), mP2(p2)
    {
        mFun = fn;
        setAssociatedObject(mFun.getObject());
    }

private:
    P1 mP1;
    P2 mP2;
    TFunctorType mFun;

    NON_COPYABLE(FunctorCallJob2);
};

template<typename P1, typename P2, typename P3>
class FunctorCallJob3: public Job
{
public:
    typedef Functor3<P1, P2, P3> TFunctorType;
    virtual void execute()
    {
        if (mFun.isValid())
        {
            mFun(mP1, mP2, mP3);
            mFun.clear();
            setAssociatedObject(nullptr);
        }
    }

protected:
    friend class Blaze::JobScheduler;
    FunctorCallJob3(const TFunctorType& fn, P1 p1, P2 p2, P3 p3):
        mP1(p1), mP2(p2), mP3(p3)
    {
        mFun = fn;
        setAssociatedObject(mFun.getObject());
    }

private:
    P1 mP1;
    P2 mP2;
    P3 mP3;
    TFunctorType mFun;

    NON_COPYABLE(FunctorCallJob3);
};

template<typename P1, typename P2, typename P3, typename P4>
class FunctorCallJob4: public Job
{
public:
    typedef Functor4<P1, P2, P3, P4> TFunctorType;
    virtual void execute()
    {
        if (mFun.isValid())
        {
            mFun(mP1, mP2, mP3, mP4);
            mFun.clear();
            setAssociatedObject(nullptr);
        }
    }

protected:
    friend class Blaze::JobScheduler;
    FunctorCallJob4(const TFunctorType& fn, P1 p1, P2 p2, P3 p3, P4 p4):
        mP1(p1), mP2(p2), mP3(p3), mP4(p4)
    {
        mFun = fn;
        setAssociatedObject(mFun.getObject());
    }

private:
    P1 mP1;
    P2 mP2;
    P3 mP3;
    P4 mP4;
    TFunctorType mFun;

    NON_COPYABLE(FunctorCallJob4);
};

inline JobId JobScheduler::scheduleFunctor(const char8_t* jobName, Functor &f, void *associatedObject, uint32_t delayMS, JobId reservedId)
{
    if (associatedObject == nullptr)
        associatedObject = f.getObject();
    JobId jobId = scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") FunctorCallJob(f), associatedObject, delayMS, reservedId);
    return jobId;
}

template <typename P1> 
inline JobId JobScheduler::scheduleFunctor(const char8_t* jobName, const Functor1<P1> &f, P1 p1, void *associatedObject, uint32_t delayMS, JobId reservedId)
{ 
    if (associatedObject == nullptr)
        associatedObject = f.getObject();
    JobId jobId = scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") FunctorCallJob1<P1>(f, p1), associatedObject, delayMS, reservedId);
    return jobId;
}

template <typename P1, typename P2>
inline JobId JobScheduler::scheduleFunctor(const char8_t* jobName, const Functor2<P1, P2> &f, P1 p1, P2 p2, void *associatedObject, uint32_t delayMS, JobId reservedId)
{ 
    if (associatedObject == nullptr)
        associatedObject = f.getObject();
    JobId jobId = scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") FunctorCallJob2<P1,P2> (f, p1, p2), associatedObject, delayMS, reservedId);
    return jobId;
}

template <typename P1, typename P2, typename P3>
inline JobId JobScheduler::scheduleFunctor(const char8_t* jobName, const Functor3<P1, P2, P3> &f, P1 p1, P2 p2, P3 p3, void *associatedObject, uint32_t delayMS, JobId reservedId)
{ 
    if (associatedObject == nullptr)
        associatedObject = f.getObject();
    JobId jobId = scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") FunctorCallJob3<P1,P2,P3>(f, p1, p2, p3), associatedObject, delayMS, reservedId);
    return jobId;
}

template <typename P1, typename P2, typename P3, typename P4>
inline JobId JobScheduler::scheduleFunctor(const char8_t* jobName, const Functor4<P1, P2, P3, P4> &f, P1 p1, P2 p2, P3 p3, P4 p4, void *associatedObject, uint32_t delayMS, JobId reservedId)
{ 
    if (associatedObject == nullptr)
        associatedObject = f.getObject();
    JobId jobId = scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") FunctorCallJob4<P1,P2,P3,P4>(f, p1, p2, p3, p4), associatedObject, delayMS, reservedId);
    return jobId;
}

template <class T>
inline JobId JobScheduler::scheduleMethod(const char8_t* jobName, T *obj, void (T::*mfp)(), void *associatedObject, uint32_t delayMS) {
    return scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") MethodCallJob<T>(obj, mfp), associatedObject, delayMS); }

template <class T>
inline JobId JobScheduler::scheduleMethod(const char8_t* jobName, const T *obj, void (T::*mfp)() const, void *associatedObject, uint32_t delayMS) {
    return scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") MethodCallJobConst<T>(obj, mfp), associatedObject, delayMS); }

template <class T, typename P1>
inline JobId JobScheduler::scheduleMethod(const char8_t* jobName, T *obj, void (T::*mfp)(P1), P1 p1, void *associatedObject, uint32_t delayMS) {
    return scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") MethodCallJob1<T, P1>(obj, mfp, p1), associatedObject, delayMS); }

template <class T, typename P1>
inline JobId JobScheduler::scheduleMethod(const char8_t* jobName, const T *obj, void (T::*mfp)(P1) const, P1 p1, void *associatedObject, uint32_t delayMS) {
    return scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") MethodCallJob1Const<T, P1>(obj, mfp, p1), associatedObject, delayMS); }

template <class T, typename P1, typename P2>
inline JobId JobScheduler::scheduleMethod(const char8_t* jobName, T *obj, void (T::*mfp)(P1, P2), P1 p1, P2 p2, void *associatedObject, uint32_t delayMS) {
    return scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") MethodCallJob2<T, P1, P2>(obj, mfp, p1, p2), associatedObject, delayMS); }

template <class T, typename P1, typename P2>
inline JobId JobScheduler::scheduleMethod(const char8_t* jobName, const T *obj, void (T::*mfp)(P1, P2) const, P1 p1, P2 p2, void *associatedObject, uint32_t delayMS) {
    return scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") MethodCallJob2Const<T, P1, P2>(obj, mfp, p1, p2), associatedObject, delayMS); }

template <class T, typename P1, typename P2, typename P3>
inline JobId JobScheduler::scheduleMethod(const char8_t* jobName, T *obj, void (T::*mfp)(P1, P2, P3), P1 p1, P2 p2, P3 p3, void *associatedObject, uint32_t delayMS) {
    return scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") MethodCallJob3<T, P1, P2, P3>(obj, mfp, p1, p2, p3), associatedObject, delayMS); }

template <class T, typename P1, typename P2, typename P3>
inline JobId JobScheduler::scheduleMethod(const char8_t* jobName, const T *obj, void (T::*mfp)(P1, P2, P3) const, P1 p1, P2 p2, P3 p3, void *associatedObject, uint32_t delayMS) {
    return scheduleJob(jobName, BLAZE_NEW(MEM_GROUP_FRAMEWORK_TEMP, "FunctorJob") MethodCallJob3Const<T, P1, P2, P3>(obj, mfp, p1, p2, p3), associatedObject, delayMS); }


}        // namespace Blaze

#endif    // BLAZE_JOB_SCHEDULER_H
