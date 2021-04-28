/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/connection/selector.h"
#include "framework/connection/channel.h"
#include "EATDF/time.h"
#include "framework/system/fiber.h"
#include "framework/system/job.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/usersessions/usersession.h"
#include "framework/connection/connection.h"
#if defined(EA_PLATFORM_LINUX)
#include <sys/resource.h> 
#include "framework/connection/epollselector.h"
#else
#include "framework/connection/standardselector.h"
#endif

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


/*** Public Methods ******************************************************************************/

Selector* Selector::createSelector()
{
#ifdef EA_PLATFORM_LINUX
    return BLAZE_NEW EpollSelector();
#else
    return BLAZE_NEW StandardSelector();
#endif  
}


bool Selector::initialize(const SelectorConfig &config)
{
    mFiberManager->initialize();

    config.copyInto(mConfig);
    
    return true;
}

TimeValue Selector::getNextExpiry(const TimeValue& now) const
{
    // The returned TimeValue is used to determine how long the OS socket 'polling' call can block in case no network events are available.

    //  0 - immediate return
    // -1 - block indefinitely
    // else: wait until that TimeValue

    // If we have jobs to do, don't wait on the network. Just process what is available. 
    if (!mQueue.empty())
        return 0;

    //If timer queue is empty and we don't have any jobs (we already checked previously), then wait indefinitely for the network 
    TimeValue expiry = mTimerQueue.getNextExpiry();
    if (expiry < 0)
        return -1;
    
    // Otherwise, possibly block in the select call until the next timer is supposed to fire. 
    expiry -= now;
    if (expiry < 0) //catch the edge case 
        return 0;
    if (expiry > 0)
    {
        expiry += 1000; // IMPORTANT: Both the epoll and the standard selector use millisecond resolution, but the timer queue keeps track of time in microseconds; therefore, to avoid having the selector busy waiting on the timer queue by waking up too early to actually expire any timeouts, we need to round up the expiry time returned from the timer queue by 1 millisecond.
    }
    return expiry;
}

void Selector::run()
{
    bool moreWork = false;
    while (!mIsShutdown)
    {
        TimeValue currentTime = TimeValue::getTimeOfDay();

        uint32_t expired;
        
        // process any timers that were scheduled to fire before currentTime (limit to configured duration)
        mTimerQueue.expire(currentTime, mConfig.getMaxTimerQueueProcessingTime(), expired);

        TimeValue timerQueueTime = TimeValue::getTimeOfDay() - currentTime;
        mTimerQueueTasks.increment(expired);
        mTimerQueueTime.increment(timerQueueTime.getMicroSeconds());

        // Process the network, select below returns combined regular and priority network time.
        TimeValue networkTime = select();
        mSelects.increment();

        // Spend no more than the configurable amount of time processing jobs to ensure
        // that the network will be serviced frequently
        TimeValue jobStartTime = TimeValue::getTimeOfDay();
        do
        {
            moreWork = processJobQueue();            
            currentTime = TimeValue::getTimeOfDay();
        } while (moreWork && ((currentTime - jobStartTime) < mConfig.getMaxJobQueueProcessingTime()));
        
        TimeValue jobQueueTime = currentTime - jobStartTime;
        mJobQueueTime.increment(jobQueueTime.getMicroSeconds());
        if (timerQueueTime + networkTime + jobQueueTime > mConfig.getMaxRunLoopProcessingTime())
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[Selector].run: selector run loop exceeded overall configured max run time; " <<
                "network=" << networkTime.getMicroSeconds() << "us/" << networkTime.getSec() << "s (max=" << mConfig.getMaxNetworkProcessingTime().getMicroSeconds() << "us) "
                "jobQueue=" << jobQueueTime.getMicroSeconds() << "us/" << jobQueueTime.getSec() << "s (max=" << mConfig.getMaxJobQueueProcessingTime().getMicroSeconds() << "us) "
                "timerQueue=" << timerQueueTime.getMicroSeconds() << "us/" << timerQueueTime.getSec() << "s (max=" << mConfig.getMaxTimerQueueProcessingTime().getMicroSeconds() << "us) ");
            // Force log the hot fibers. 
            mFiberManager->checkCpuBudget(true);
        }


        if (mIsShutdown)
            break;
    }
}

void Selector::removeAllEvents()
{
    mTimerQueue.removeAllTimers();
    mQueue.removeAll();
    removePendingJobs();
}

void Selector::removePendingJobs()
{
    // detach and delete the 
    // list of pending jobs
    Job* job = mNextJob;
    mNextJob = nullptr;
    while (job != nullptr)
    {
        Job* temp = job;
        job = job->mNext;
        delete temp;
    }
}

void Selector::removeJob(Job* job)
{
    // detach and delete the 
    // pending job if it exists
    Job* currJob = mNextJob;
    Job* prevJob = mNextJob;
    while (currJob != nullptr)
    {
        if (currJob == job)
        {
            if (prevJob == mNextJob)
                mNextJob = currJob->mNext;
            else
                prevJob->mNext = currJob->mNext;

            delete currJob;
            return;
        }

        prevJob = currJob;
        currJob = currJob->mNext;
    }

    // We didn't find the job in the pending list
    // so remove it from the queue if it exists
    mQueue.removeJob(job);
}

TimerId Selector::scheduleTimerDedicated(const TimeValue& timeoutAbsolute, Job* job, const void* associatedObject, const char *namedContext)
{
    Fiber::CreateParams params;
    params.namedContext = namedContext;
    params.blocking = false;
    params.stackSize = Fiber::STACK_HUGE;
    return scheduleTimer(timeoutAbsolute, job, associatedObject, params);
}


TimerId Selector::scheduleTimer(const TimeValue& timeoutAbsolute, Job* job, const void* associatedObject, const Fiber::CreateParams& fiberParams)
{
    TimerId id = mTimerQueue.schedule(timeoutAbsolute, job, associatedObject, fiberParams);
    wake();
    return id;
}


void Selector::updateTimer(TimerId id, const TimeValue& timeoutAbsolute)
{
    mTimerQueue.update(id, timeoutAbsolute);
}


/*! ***************************************************************************/
/*! \brief Schedule a fiber to go to sleep.

    The sleep can be cancelled by filling out the timer id and calling "cancelTimer",
    which will prematurely wake the fiber.

    \param duration The length of the sleep in relative time.
    \param [out, default nullptr] timerId Optional pointer to a timer id to fill out.
*******************************************************************************/
BlazeRpcError Selector::sleep(const TimeValue& duration, TimerId *timerId, Fiber::EventHandle* eventHandle, const char8_t* context)
{
    if (Fiber::isCurrentlyBlockable())
    {
        Fiber::EventHandle handle = Fiber::getNextEventHandle();
        if (eventHandle != nullptr)
            *eventHandle = handle;
        TimerId id = mTimerQueue.scheduleFiberSleep(TimeValue::getTimeOfDay() + duration, handle);
        if (timerId != nullptr)
            *timerId = id;
        wake();
        return Fiber::wait(handle, context != nullptr ? context : "Selector::sleep");
    }
    else
    {
        EA_FAIL_MSG("Cannot call Selector::sleep() from the main fiber!");
        return ERR_SYSTEM;
    }
}

bool Selector::cancelTimer(TimerId timerId)
{
    const bool canceled = mTimerQueue.cancel(timerId);
    if (canceled)
        wake();
    return canceled;
}

uint32_t Selector::cancelAllTimers(const void* associatedObject)
{
    const uint32_t count = mTimerQueue.cancel(associatedObject);
    if (count > 0)
        wake();
    return count;
}

/*** Protected Methods ***************************************************************************/

Selector::Selector()
    :   mTimerQueueTasks(*Metrics::gFrameworkCollection, "selector.timerQueueTasks"),
        mTimerQueueTime(*Metrics::gFrameworkCollection, "selector.timerQueueTime"),
        mTimerQueueSize(*Metrics::gFrameworkCollection, "selector.timerQueueSize", [this]() { return mTimerQueue.size(); }),
        mJobQueueTasks(*Metrics::gFrameworkCollection, "selector.jobQueueTasks"),
        mJobQueueTime(*Metrics::gFrameworkCollection, "selector.jobQueueTime"),
        mNetworkPolls(*Metrics::gFrameworkCollection, "selector.networkPolls"),
        mNetworkTasks(*Metrics::gFrameworkCollection, "selector.networkTasks"),
        mNetworkTime(*Metrics::gFrameworkCollection, "selector.networkTime"),
        mNetworkPriorityPolls(*Metrics::gFrameworkCollection, "selector.networkPriorityPolls"),
        mNetworkPriorityTasks(*Metrics::gFrameworkCollection, "selector.networkPriorityTasks"),
        mNetworkPriorityTime(*Metrics::gFrameworkCollection, "selector.networkPriorityTime"),
        mSelects(*Metrics::gFrameworkCollection, "selector.selects"),
        mWakes(*Metrics::gFrameworkCollection, "selector.wakes"),
        mIsShutdown(false),
        mTimerQueue("SelectorTimerQueue"),
        mQueue("Selector"),
        mNextJob(nullptr),
        mFiberManager(nullptr)
{
    mFiberManager = BLAZE_NEW FiberManager(*this);
}

Selector::~Selector()
{
    delete mFiberManager;
}

void Selector::getStatusInfo(SelectorStatus& status)
{
    status.setJobQueueTasks(mJobQueueTasks.getTotal());
    status.setJobQueueTime(mJobQueueTime.getTotal());
    status.setTimerQueueTasks(mTimerQueueTasks.getTotal());
    status.setTimerQueueTime(mTimerQueueTime.getTotal());
    status.setTimerQueueSize(mTimerQueue.size());
    status.setNetworkPolls(mNetworkPolls.getTotal());
    status.setNetworkTasks(mNetworkTasks.getTotal());
    status.setNetworkTime(mNetworkTime.getTotal());
    status.setNetworkPriorityPolls(mNetworkPriorityPolls.getTotal());
    status.setNetworkPriorityTasks(mNetworkPriorityTasks.getTotal());
    status.setNetworkPriorityTime(mNetworkPriorityTime.getTotal());
    status.setSelects(mSelects.getTotal());
    status.setWakes(mWakes.getTotal());
}

void Selector::getProcessCpuUsageTime(EA::TDF::TimeValue& sys, EA::TDF::TimeValue& usr)
{
    // TODO: we should split cpu measurement into core cpu usage(server thread) and support cpu usage(support threads) which is process cpu usage - core.
    // This is because we want to know overall cpu usage, but most of the load balancing decisions should focus on the core cpu usage since that is what we have most control over.
#if defined(EA_PLATFORM_WINDOWS)
#if 0
    HANDLE hProcess = GetCurrentProcess();
    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser);
    sys = TimeValue(ftKernel);
    usr = TimeValue(ftUser);
#else
    // NOTE: On windows we have to use per/thread cpu metrics since getting process metrics ends up generating a ton of false positive warnings about high cpu usage (due to everything running colocated).
    HANDLE hThread = GetCurrentThread();
    FILETIME ftCreation, ftExit, ftKernel, ftUser;
    GetThreadTimes(hThread, &ftCreation, &ftExit, &ftKernel, &ftUser);
    sys = TimeValue(ftKernel);
    usr = TimeValue(ftUser);
#endif
#elif defined(EA_PLATFORM_LINUX)
    rusage resourceUsage;
    getrusage(RUSAGE_SELF, &resourceUsage);
    sys = TimeValue(resourceUsage.ru_stime.tv_sec * 1000 * 1000 + resourceUsage.ru_stime.tv_usec);
    usr = TimeValue(resourceUsage.ru_utime.tv_sec * 1000 * 1000 + resourceUsage.ru_utime.tv_usec);
#else
    static_assert(false, "Unupported platform!");
#endif
}

/*** Private Methods *****************************************************************************/

bool Selector::processJobQueue()
{
    // Pull out a nullptr terminated list of jobs with the same priority
    // This is safe because any future jobs with that priority are
    // guaranteed to execute in FIFO order.
    Job* job = mQueue.popMany();
    while (job != nullptr)
    {
        mNextJob = job->mNext;
        job->execute();
        delete job;
        // NOTE: mNextJob may be set to nullptr 
        // if removeAllEvents() is called
        // from within job->execute()
        job = mNextJob;
        mJobQueueTasks.increment();
    }
    return !mQueue.empty();
}


void Selector::scheduleDedicatedFiberCallBase(Job* job, const char* fiberContext)
{
    Fiber::CreateParams params;
    params.blocking = false;
    params.stackSize =  Fiber::STACK_HUGE;
    params.namedContext = fiberContext;
   
    getFiberManager().executeJob(*job, params);
}

Job* Selector::scheduleCallBase(Job& job, Fiber::Priority priority)
{
    if (priority == 0)
    {
        if (Fiber::isCurrentlyInMainFiber())
            job.setPriority(TimeValue::getTimeOfDay().getMicroSeconds());
        else
            job.setPriority(Fiber::getCurrentPriority());
    }
    else
    {
        job.setPriority(priority);
    }

    queueJob(&job);
    return &job;
}

} // Blaze
