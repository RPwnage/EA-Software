/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FiberManager

    Managers all the fibers for a thread, including creating and destroying them.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/system/fibermanager.h"
#include "framework/system/fiber.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/tdf/controllertypes_server.h"


namespace Blaze
{

struct FiberAccountingRef
{
    const char8_t* mContext;
    const FiberAccountingInfo* mInfo;
    FiberAccountingRef(const char8_t* context = nullptr, const FiberAccountingInfo* info = nullptr)
        : mContext(context), mInfo(info) {}
};

struct PeriodTimeCompareDesc
{
    bool operator() (const FiberAccountingRef& a, const FiberAccountingRef& b)
    {
        return b.mInfo->getPeriodTime() < a.mInfo->getPeriodTime();
    }
};

typedef eastl::vector<FiberAccountingRef> FiberAccountingRefList;

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

// Amount of time between fiber memory reclamation 
#define FIBER_CLEANUP_PERIOD (60 * 1000 * 1000)

// minimum period between cpu budget logs
#define CPU_BUDGET_INTERVAL_MIN (1*1000*1000)

// process CPU usage measurement interval
#define CPU_USAGE_PER_PROCESS_MEASUREMENT_INTERVAL (15*1000*1000)

// maximum size of task report printed by cpu budget log
#define CPU_BUDGET_TASK_REPORT_SIZE_MAX (50)

/*** Public Methods ******************************************************************************/

FiberManager::FiberMetrics::FiberMetrics()
    : mCount(*Metrics::gFrameworkCollection, "fibers.count", Metrics::Tag::fiber_context)
    , mTime(*Metrics::gFrameworkCollection, "fibers.time", Metrics::Tag::fiber_context)
    , mDbCount(*Metrics::gFrameworkCollection, "fibers.dbcount", Metrics::Tag::fiber_context, Metrics::Tag::db_query_name)
    , mCpuBudgetCheckCount(*Metrics::gFrameworkCollection, "fibers.cpuBudgetCheckCount")
    , mCpuBudgetExceededCount(*Metrics::gFrameworkCollection, "fibers.cpuBudgetExceededCount")
    , mUserCpuTimeForProcess(*Metrics::gFrameworkCollection, "fibers.userCpuTimeForProcess", []() { TimeValue sys, usr; Selector::getProcessCpuUsageTime(sys, usr); return usr.getSec(); })
    , mSystemCpuTimeForProcess(*Metrics::gFrameworkCollection, "fibers.systemCpuTimeForProcess", []() { TimeValue sys, usr; Selector::getProcessCpuUsageTime(sys, usr); return sys.getSec(); })
    , mCpuUsageForProcessPercent(*Metrics::gFrameworkCollection, "fibers.cpuUsageForProcessPercent", []() { return (uint64_t)gFiberManager->getCpuUsageForProcessPercent(); })
{
}


FiberManager::FiberManager(Selector& selector)
    : mSelector(selector),
      mMaxFiberUseCount(100),
      mCleanupTimerId(INVALID_TIMER_ID),
      mSemaphoreIdCounter(1),
      mThread(nullptr),
      mAccountingInfoMap(BlazeStlAllocator("FiberManager::mAccountingInfoMap")),
      mCommandFiberCount(0),
      mLastProcessCpuUsagePercent(0),
      mLastProcessSystemCpuUsagePercent(0)
{
    //Init all the fiber pools    
    mFiberPools.reserve(Fiber::MAX_STACK_SIZES);
    for(size_t counter = 0; counter < Fiber::MAX_STACK_SIZES; ++counter) 
    {
        mFiberPools.push_back(BLAZE_NEW FiberPool(*this, (Fiber::StackSize) counter));
    }
}

FiberManager::~FiberManager()
{
    //Kill every semaphore
    while (!mUsedSemaphoreMap.empty())
    {
        releaseSemaphore(static_cast<FiberSemaphore&>(*(mUsedSemaphoreMap.begin())).getId());
    }

    while (!mUnusedSemaphoreList.empty())
    {
        FiberSemaphore &dangle = static_cast<FiberSemaphore&>(mUnusedSemaphoreList.front());
        mUnusedSemaphoreList.pop_front();
        delete &dangle;
    }

    for(FiberPoolVector::iterator itr = mFiberPools.begin(), end = mFiberPools.end(); itr != end; ++itr) 
    {
        delete *itr;
    }

    mAccountingInfoMap.clear();
    mMetrics.reset();
}

void FiberManager::initialize()
{   
    mThread = gCurrentThread;

    if (mCleanupTimerId == INVALID_TIMER_ID)
    {
        mCleanupTimerId = mSelector.scheduleTimerCall(
            TimeValue::getTimeOfDay() + FIBER_CLEANUP_PERIOD,
            this, &FiberManager::cleanupFiberPools,
            "FiberManager::cleanupFiberPools");
    }
}

void FiberManager::configure(const FiberConfig& config)
{
    config.copyInto(mFiberConfig);
    
    getFiberPool(Fiber::STACK_SMALL).setMaxPooledFibers(mFiberConfig.getPoolSizes().getSmall());
    getFiberPool(Fiber::STACK_MEDIUM).setMaxPooledFibers(mFiberConfig.getPoolSizes().getMedium());
    getFiberPool(Fiber::STACK_LARGE).setMaxPooledFibers(mFiberConfig.getPoolSizes().getLarge());
    getFiberPool(Fiber::STACK_HUGE).setMaxPooledFibers(mFiberConfig.getPoolSizes().getHuge());
}

/*! ***************************************************************************/
/*! \brief Gets a fiber from the manager.

    \params Parameters describing the type of fiber to get.
    \return The fiber. 
*******************************************************************************/
Fiber *FiberManager::getFiber(const Fiber::CreateParams& params)
{
    return getFiberPool(params.stackSize).get(params);
}


//Special job class to handle cleaning up the job memory.  If this job exectutes, it passes ownership
//of the job pointer onto the running fiber.  If it doesn't execute, it needs to clean up the job
//or else the memory is leaked.  A job will not always run if the server is shutting down, so we do this so 
//as not to leak.
class FiberManagerDelayCallJob : public MethodCall2Job<FiberManager, Job*, Fiber::CreateParams>
{
public:
    FiberManagerDelayCallJob(FiberManager* caller, MemberFunc func, Job* p1, Fiber::CreateParams p2) : 
      MethodCall2Job<FiberManager, Job*, Fiber::CreateParams>(caller, func, p1, p2) {}

    //special cleanup for the job pointer
    ~FiberManagerDelayCallJob() override
    {
        if (mArg1 != nullptr)
        {
            delete mArg1;
            mArg1 = nullptr;
        }
    }

    //clear our pointer if we actually execute
    void execute() override
    {
        MethodCall2Job<FiberManager, Job*, Fiber::CreateParams>::execute();
        mArg1 = nullptr;
    }
};


bool FiberManager::executeJob(Job& job, const Fiber::CreateParams& params, Fiber::FiberHandle* handle)
{
    if (gSelector != &mSelector)
    {
        //delay the call by putting it on our fiber queue.
        MethodCall2Job<FiberManager, Job*, Fiber::CreateParams> *delayJob = 
            BLAZE_NEW FiberManagerDelayCallJob(this, &FiberManager::executeDelayedJob, &job, params);
        delayJob->setPriority(params.priority);
        delayJob->setNamedContext(params.namedContext);
        mSelector.queueJob(delayJob);

        // the job is never completed, so this is always true
        return true;
    }

    bool result = false;
    Fiber* fiber = getFiber(params);
    if (fiber != nullptr)
    {
        if (handle != nullptr)
            *handle = fiber->getHandle(); //set handle now so its always on while the fiber executes
        result = fiber->call(job, params);
        if (!result) //execute returns true if the job has not been completed and the fiber is outstanding
        {
            if (handle != nullptr)
                handle->reset(); //fiber is no longer running, so reset
        }
    }

    return result;
}


/*! ***************************************************************************/
/*! \brief Get or create a fiber semaphore.

    \param name The name to use for this semaphore.
*******************************************************************************/
SemaphoreId FiberManager::getSemaphore(const char8_t* name/* = nullptr*/, 
                                       int32_t threshold/* = 1*/, int32_t initCount/* = 0*/)
{
    FiberSemaphore* semaphore = nullptr; 
    if (mUnusedSemaphoreList.empty())
    {
        semaphore = BLAZE_NEW FiberSemaphore(); 
    }
    else
    {
        semaphore = static_cast<FiberSemaphore*>(&mUnusedSemaphoreList.front());
        mUnusedSemaphoreList.pop_front();
    }

    semaphore->initialize(name, mSemaphoreIdCounter, threshold, initCount);
    // always increment by 2 so semaphoreIdCounter will always be odd.  saves a check for INVALID_SEMAPHORE_ID
    mSemaphoreIdCounter += 2;
    mUsedSemaphoreMap.insert(*semaphore);
    return semaphore->getId();
}

/*! ***************************************************************************/
/*! \brief schedules a job on the mSelector for FiberManager::signalSemaphore.

    \param id The id of the semaphore to signal.
           reason The BlazeRpcError to signal the semaphore with.
           priority The priority of the job to queue.  Only used if method called from different thread
*******************************************************************************/
void FiberManager::delaySignalSemaphore(SemaphoreId id, BlazeRpcError reason/* = Blaze::ERR_OK*/,
                                        int64_t priority)
{
    if (gCurrentThread == mThread)
    {
        SemaphoresByIdMap::iterator itr = mUsedSemaphoreMap.find(id);
        if (itr != mUsedSemaphoreMap.end())
        {
            FiberSemaphore& semaphore = static_cast<FiberSemaphore&>(*itr);
            Blaze::Job* job = BLAZE_NEW MethodCall2Job<FiberManager, SemaphoreId, BlazeRpcError>(this, &FiberManager::signalSemaphore, id, reason);
            job->setPriority(semaphore.getNextWaitingFiberPriority());
            mSelector.queueJob(job);
        }
    }
    else
    {
        // if delaySignalSemaphore is being invoked on a different thread, use the priority passed in
        // by default JOB_PRIORITY_FIFO will push it on the the regular FIFO queue which will get
        // emptied before the priority queue.
        Blaze::Job* job = BLAZE_NEW MethodCall2Job<FiberManager, SemaphoreId, BlazeRpcError>(this, &FiberManager::signalSemaphore, id, reason);
        job->setPriority(priority);
        mSelector.queueJob(job);
    }
}

/*! ***************************************************************************/
/*! \brief signals the semaphore corresponding to the id and wakes up waiting fiber
           if 1) count == 0 (threshold is met) or 2) reason != ERR_OK  

    \param id The id of the semaphore to signal.
           reason The BlazeRpcError to wake the fiber up with
*******************************************************************************/
void FiberManager::signalSemaphore(SemaphoreId id, BlazeRpcError reason/* = Blaze::ERR_OK*/)
{
    EA_ASSERT(gCurrentThread == mThread);

    // make sure the signaling of the semaphore happens before we resume running on the current fiber
    mSelector.scheduleDedicatedFiberCall(this, &FiberManager::doSignalSemaphore, id, reason, "FiberManager::doSignalSemaphore");
}

void FiberManager::doSignalSemaphore(SemaphoreId id, BlazeRpcError reason)
{
    SemaphoresByIdMap::iterator itr = mUsedSemaphoreMap.find(id);
    if (itr != mUsedSemaphoreMap.end())
    {
        FiberSemaphore& semaphore = static_cast<FiberSemaphore&>(*itr);
        semaphore.signal(reason);
    }
}

/*! ***************************************************************************/
/*! \brief wait on the semaphore corresponding to the id.

    \param id The id of the semaphore to wait on.
           threshold The threshold to set on the semaphore
*******************************************************************************/
BlazeRpcError FiberManager::waitSemaphore(SemaphoreId id)
{
    return doWaitSemaphore(id);
}

/*! ***************************************************************************/
/*! \brief wait on the semaphore corresponding to the id.

    \param id The id of the semaphore to wait on.
    \param delay The delay to wait before signalling semaphore with ERR_TIMEOUT
*******************************************************************************/
BlazeRpcError FiberManager::waitSemaphoreOrTimeout(SemaphoreId id, TimeValue delay)
{
    BlazeRpcError rc = ERR_SYSTEM;
    TimeValue wakeTime = TimeValue::getTimeOfDay() + delay;
    TimerId timerId = mSelector.scheduleTimerCall(wakeTime, this, &FiberManager::doSignalSemaphore, 
        id, ERR_TIMEOUT, "FiberManager::doSignalSemaphore(ERR_TIMEOUT)");
    if (timerId != INVALID_TIMER_ID)
    {
        rc = doWaitSemaphore(id);
        if (rc != ERR_TIMEOUT)
        {
            mSelector.cancelTimer(timerId);
        }
    }
    return rc;
}


BlazeRpcError FiberManager::getAndWaitSemaphore(const char8_t* name/* = nullptr*/,
                                                int32_t threshold/* = 1*/, int32_t initCount/* = 0*/)
{
    return doWaitSemaphore(getSemaphore(name));
}


/*! ***************************************************************************/
/*! \brief Releases the fiber semaphore back to the manager.

    \param id The id of the fiber semaphore to release.
*******************************************************************************/
void FiberManager::releaseSemaphore(SemaphoreId id)
{
    SemaphoresByIdMap::iterator itr = mUsedSemaphoreMap.find(id);
    if (itr != mUsedSemaphoreMap.end())
    {
        mUsedSemaphoreMap.erase(id);
        FiberSemaphore& semaphore = static_cast<FiberSemaphore&>(*itr);
        semaphore.clear();
        mUnusedSemaphoreList.push_back(semaphore);
    }
}
    
/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

/*************************************************************************************************/
/*!
     \brief cleanupFiberPools
 
     This function will clean the fiber pools using two mechanisms:

     1) If the pool size exceeds the maximum configured then destroy the appropriate number of
        fibers to trim it down to the configured value.
     2) Destroy any fibers from the head of the pool list that have exceeded their use count.

     Fiber pool cleaning is done for two reasons:
        1) Limit the amount of memory used by fibers.  While it will still be possible to
           have larger spikes in memory when there is a burst of activity, it will control
           the permanently allocated usage.
        2) By periodically destroying fibers, we can release resident memory back to the
           system that is generally not used.  Fibers on linux use mmap() to map their
           stacks.  mmap() will only page memory in when the pages are used.  If we allocate
           a 64K stack but the tasks running on it generally only consume 8K then only
           8K will actually occupy physical memory.  However if we have the odd command
           that needs all 64K then all 64K will be paged in.  By destroying the fibers
           periodically we can effectively reduce the typical amount of resident memory
           needed by the server.  While the OS can obviously handle paging unused memory
           out it will not do so until physical memory is exhausted.  Since we know that
           potentially large chunks of memory may be largely unused in the fiber stack
           space we can be more proactive in releasing it from physical memory.  If only
           1% of the commands need 64K of stack while the rest of them need at most 8K then
           there isn't any point in keeping a bunch of 64K stacks paged into physical
           memory.  This mechanism allows us to run with larger default stacks for RPCs yet
           only consume the amount of physical memory we actually need.
           
     It is necessary to do this cleanup in an idle function rather than as part of the
     releaseFiber() call because releaseFiber() is typically called from the fiber itself so
     destroying it would cause a crash because the active stack would be deallocated.
*/
/*************************************************************************************************/
void FiberManager::cleanupFiberPools()
{
    size_t poolTrimmed = 0;
    size_t useCountTrimmed = 0;

    for(FiberPoolVector::iterator itr = mFiberPools.begin(), end = mFiberPools.end(); itr != end; ++itr) 
    {
        (*itr)->cleanupFibers(useCountTrimmed, poolTrimmed, mFiberConfig.getMaxFiberUseCount());
    }


    BLAZE_TRACE_LOG(Log::SYSTEM, "[FiberManager].cleanupFiberPools: trimmed " << (poolTrimmed + useCountTrimmed) << " fibers from pools (" 
                    << poolTrimmed << " for pool size overage and " << useCountTrimmed << " for usage count overage).");

    mCleanupTimerId = mSelector.scheduleTimerCall(
            TimeValue::getTimeOfDay() + FIBER_CLEANUP_PERIOD,
            this, &FiberManager::cleanupFiberPools,
            "FiberManager::cleanupFiberPools");
}

/*! ***************************************************************************/
/*! \brief wait on the semaphore corresponding to the id.

    \param id The id of the semaphore to wait on.
           threshold The threshold to set on the semaphore
*******************************************************************************/
BlazeRpcError FiberManager::doWaitSemaphore(SemaphoreId id)
{
    VERIFY_WORKER_FIBER();

    SemaphoresByIdMap::iterator itr = mUsedSemaphoreMap.find(id);
    if (itr != mUsedSemaphoreMap.end())
    {
        FiberSemaphore& semaphore = static_cast<FiberSemaphore&>(*itr);
        bool releaseAfterWait = false;
        BlazeRpcError sleepError = semaphore.wait(&releaseAfterWait);
        if (releaseAfterWait)
            releaseSemaphore(id);

        return sleepError;
    }
    BLAZE_ERR_LOG(Log::SYSTEM, "[FiberManager].doWaitSemaphore: invalid semaphore id " << id);
    return Blaze::ERR_SYSTEM;
}

void FiberManager::getStatus(FiberManagerStatus& status) const
{
    status.getFiberPoolInfoList().reserve(mFiberPools.size());
    for(FiberPoolVector::const_iterator itr = mFiberPools.begin(), end = mFiberPools.end(); itr != end; ++itr) 
    {
        (*itr)->getStatus(status);
    }

    status.setSemaphoreIdCounter(mSemaphoreIdCounter);
    status.setUnusedSemaphoreListSize(mUnusedSemaphoreList.size());
    status.setUsedSemaphoreMapSize(mUsedSemaphoreMap.size());
    status.setCpuUsageForProcessPercent(mLastProcessCpuUsagePercent);
}

void FiberManager::clearFiberAccounting()
{
    mAccountingInfoMap.clear();
    Fiber::getMainFiber()->clearFiberTime();

    clearFiberPeriodAccounting();

    mMetrics.reset();
}

void FiberManager::clearFiberPeriodAccounting()
{
    for (FiberAccountingInfoMap::iterator i = mAccountingInfoMap.begin(), e = mAccountingInfoMap.end(); i != e; ++i)
    {
        i->second->setPeriodCount(0);
        i->second->setPeriodTime(0);
    }

    // go through running fibers and clear their period accounting    
    for(FiberPoolVector::iterator itr = mFiberPools.begin(), end = mFiberPools.end(); itr != end; ++itr) 
    {
        for (FiberPool::TaggedFiberMap::iterator mitr = (*itr)->getUsedFiberMap().begin(), mend =  (*itr)->getUsedFiberMap().end(); mitr != mend; ++mitr)
        {            
            for(FiberPoolList::iterator fitr = mitr->second.begin(), fend = mitr->second.end(); fitr != fend; ++fitr)
            {
                Fiber& fiber = static_cast<Fiber&>(*fitr);
                fiber.markFiberTime();
            }
        }
    }

    // clear main and dedicated fiber period accounting
    Fiber::getMainFiber()->markFiberTime();
}

TimeValue FiberManager::checkCpuBudget(bool forcePrintOutExceeded)
{
    TimeValue budgetInterval = mFiberConfig.getCpuBudgetCheckInterval();
    if (budgetInterval < CPU_BUDGET_INTERVAL_MIN)
        budgetInterval = CPU_BUDGET_INTERVAL_MIN;
    // It is possible that this function gets called from both the timer queue and selector loop explicitly (say if you are sitting on a breakpoint for 10s) right after each other. If the clock precision is not enough to detect 
    // that (reproduced on Windows), you could end up with a zero total time and run into a divide by zero error. So we early out. 
    int64_t totalTime = TimeValue::getTimeOfDay().getMicroSeconds() - mLastCpuBudgetCheckTime.getMicroSeconds(); 
    if (totalTime == 0)
        return budgetInterval;

    uint32_t maxTaskReportSize = mFiberConfig.getCpuBudgetTaskReportSize();
    if (maxTaskReportSize > CPU_BUDGET_TASK_REPORT_SIZE_MAX)
        maxTaskReportSize = CPU_BUDGET_TASK_REPORT_SIZE_MAX;

    if (mLastCpuBudgetCheckTime.getMicroSeconds() == 0)
    {
        // just take the usage
        updateSystemResourceCounters();
    }
    else
    {
        // check if CPU usage of main thread was above mFiberConfig.getCpuBudgetWarnLogCpuPct()
        SelectorStatus currentStatus;
        mSelector.getStatusInfo(currentStatus);
        int64_t networkPriorityTime = currentStatus.getNetworkPriorityTime() - mLastCpuBudgetSelectorStatus.getNetworkPriorityTime();
        int64_t networkTime = currentStatus.getNetworkTime() - mLastCpuBudgetSelectorStatus.getNetworkTime();
        int64_t timerTime = currentStatus.getTimerQueueTime() - mLastCpuBudgetSelectorStatus.getTimerQueueTime();
        int64_t jobTime = currentStatus.getJobQueueTime() - mLastCpuBudgetSelectorStatus.getJobQueueTime();
        int64_t selectorTime = networkPriorityTime + networkTime + timerTime + jobTime;
        
        int64_t networkPriorityTasks = currentStatus.getNetworkPriorityTasks() - mLastCpuBudgetSelectorStatus.getNetworkPriorityTasks();
        int64_t networkTasks = currentStatus.getNetworkTasks() - mLastCpuBudgetSelectorStatus.getNetworkTasks();
        int64_t timerTasks = currentStatus.getTimerQueueTasks() - mLastCpuBudgetSelectorStatus.getTimerQueueTasks();
        int64_t jobTasks = currentStatus.getJobQueueTasks() - mLastCpuBudgetSelectorStatus.getJobQueueTasks();
        int64_t selectorTasks = networkPriorityTasks + networkTasks + timerTasks + jobTasks;
        int64_t selectCount = currentStatus.getSelects() - mLastCpuBudgetSelectorStatus.getSelects();

        TimeValue oldCpuTime = mLastCpuUsageTime;
        TimeValue oldCpuSystemTime = mLastCpuSystemUsageTime;
        

        updateSystemResourceCounters();
        
        TimeValue cpuTimeSinceLastCheck = mLastCpuUsageTime - oldCpuTime;
        TimeValue cpuSystemTimeSinceLastCheck = mLastCpuSystemUsageTime - oldCpuSystemTime;

        // calculate average process CPU usage time in the last measurement interval, if the interval expired
        mProcessCpuUsageInterval += totalTime;
        mCurrentProcessCpuUsageTime += cpuTimeSinceLastCheck;
        mCurrentProcessSystemCpuUsageTime += cpuSystemTimeSinceLastCheck;

        if (mProcessCpuUsageInterval >= CPU_USAGE_PER_PROCESS_MEASUREMENT_INTERVAL)
        {
            mLastProcessCpuUsagePercent = (uint32_t)(mCurrentProcessCpuUsageTime * 100 / mProcessCpuUsageInterval);
            mLastProcessSystemCpuUsagePercent = (uint32_t)(mCurrentProcessSystemCpuUsageTime * 100 / mProcessCpuUsageInterval);
            mProcessCpuUsageInterval = 0;
            mCurrentProcessCpuUsageTime = 0;
            mCurrentProcessSystemCpuUsageTime = 0;
        }

        if (mCpuBudgetLastPrintout == 0)
            mCpuBudgetLastPrintout = TimeValue::getTimeOfDay();

        // printing out budget check when exceeded or if it wasn't printed for longer than 1 min
        const TimeValue sincePrintout = (TimeValue::getTimeOfDay() - mCpuBudgetLastPrintout);
        const bool printOutRegular = mFiberConfig.getCpuBudgetInfoLogPeriod().getMicroSeconds() > 0 && (sincePrintout >= mFiberConfig.getCpuBudgetInfoLogPeriod());
        const bool printOutExceeded = ((sincePrintout >= mFiberConfig.getCpuBudgetWarnLogPeriod()) 
            && (cpuTimeSinceLastCheck > (totalTime * mFiberConfig.getCpuBudgetWarnLogCpuPct()) / 100))
            || forcePrintOutExceeded;
            
        if (printOutRegular || printOutExceeded)
        {
            mCpuBudgetLastPrintout = TimeValue::getTimeOfDay();

            // add main fiber time
            FiberAccountingInfo& mainAccounting = getFiberAccountingInfo(Fiber::MAIN_FIBER_NAME);
            mainAccounting.setPeriodCount(mainAccounting.getPeriodCount() + 1);
            mainAccounting.setPeriodTime(mainAccounting.getPeriodTime() + Fiber::getMainFiber()->getFiberTimeSinceMark().getMicroSeconds());

            // go through fibers that have not yet completed to gather
            // their cpu time metrics for this period
            // go through running fibers and clear their period accounting    
            for(FiberPoolVector::iterator itr = mFiberPools.begin(), end = mFiberPools.end(); itr != end; ++itr) 
            {
                for (FiberPool::TaggedFiberMap::iterator mitr = (*itr)->getUsedFiberMap().begin(), mend =  (*itr)->getUsedFiberMap().end(); mitr != mend; ++mitr)
                {            
                    for(FiberPoolList::iterator fitr = mitr->second.begin(), fend = mitr->second.end(); fitr != fend; ++fitr)
                    {
                        const Fiber& fiber = static_cast<const Fiber&>(*fitr);
                        FiberAccountingInfo& accountingInfo = getFiberAccountingInfo(fiber.getNamedContext());
                        accountingInfo.setPeriodCount(accountingInfo.getPeriodCount() + 1);
                        accountingInfo.setPeriodTime(accountingInfo.getPeriodTime() + fiber.getFiberTimeSinceMark().getMicroSeconds());
                    }
                }
            }


            // reserve enough space for all the items in the map
            FiberAccountingRefList accList;
            accList.reserve(mAccountingInfoMap.size());
            for (FiberAccountingInfoMap::const_iterator i = mAccountingInfoMap.begin(), e = mAccountingInfoMap.end(); i != e; ++i)
            {
                if (i->second->getPeriodTime() > 0)
                {
                    // insert all the fiber accounting infos relevant to this budget period
                    accList.push_back(FiberAccountingRef((const char8_t*) i->first, i->second));
                }
            }
            // sort by cpu time during the period (in descending order)
            eastl::sort(accList.begin(), accList.end(), PeriodTimeCompareDesc());
            uint32_t maxListSize = (uint32_t) accList.size();
            if (maxListSize > maxTaskReportSize)
            {
                // list of fibers executed during this interval exceeds the limit set in the framework.cfg, clamp it
                maxListSize = maxTaskReportSize;
            }
            // NOTE: checkTime will usually also exceed the budget,
            // time because cpu was too busy to process this function
            StringBuilder sb1(nullptr);
            sb1.append(
                "cpu=%" PRId64 "%% sys=%" PRId64 "%% time=%" PRId64 " ival=%" PRId64 " selects=%" PRId64 " period=%" PRId64 "\n"
                "selector  netpri     net  timers    jobs   total\n"
                " cpu%%     %6" PRId64 "  %6" PRId64 "  %6" PRId64 "  %6" PRId64 "  %6" PRId64 "\n"
                " events   %6" PRId64 "  %6" PRId64 "  %6" PRId64 "  %6" PRId64 "  %6" PRId64 "\n",
                cpuTimeSinceLastCheck * 100 / totalTime, cpuSystemTimeSinceLastCheck * 100 / totalTime, selectorTime, budgetInterval.getMicroSeconds(), selectCount, mMetrics.mCpuBudgetCheckCount.getTotal(),
                networkPriorityTime * 100 / totalTime, networkTime * 100 / totalTime, timerTime * 100 / totalTime, jobTime * 100 / totalTime, cpuTimeSinceLastCheck * 100 / totalTime,
                networkPriorityTasks, networkTasks, timerTasks, jobTasks, selectorTasks);
            if (maxListSize > 0)
            {
                sb1.append(
                    "top tasks\n"
                    " %%   count  name\n");
                for (uint32_t i = 0; i < maxListSize; ++i)
                {
                    FiberAccountingRef& accRef = accList[i];
                    sb1.append("%3" PRId64 " %6" PRId64 "  %s\n", accRef.mInfo->getPeriodTime() * 100 / totalTime, accRef.mInfo->getPeriodCount(), accRef.mContext);
                }
            }

            if (printOutRegular)
            {
                BLAZE_INFO_LOG(Log::SYSTEM, 
                    "[FiberManager].checkCpuBudget: " << sb1.get());
            }
            else
            {
                BLAZE_WARN_LOG(Log::SYSTEM, 
                    "[FiberManager].checkCpuBudget: " << sb1.get());
                mMetrics.mCpuBudgetExceededCount.increment();
            }
        }
    }

    // clear fiber accounting data for this period
    clearFiberPeriodAccounting();

    // take selector metrics snapshot
    mSelector.getStatusInfo(mLastCpuBudgetSelectorStatus);
    // take main fiber totaltime snapshot
    mLastCpuBudgetCheckTime = TimeValue::getTimeOfDay();    
    // increment the period counter (used for tracking periods)
    mMetrics.mCpuBudgetCheckCount.increment();
    
    return budgetInterval;
}

void FiberManager::updateSystemResourceCounters()
{
    TimeValue sys, usr;
    Selector::getProcessCpuUsageTime(sys, usr);
    mLastCpuSystemUsageTime = sys;
    mLastCpuUsageTime = sys + usr;
}

FiberAccountingInfo& FiberManager::getFiberAccountingInfo(const char* userNamedContext)
{
    FiberAccountingInfoMap::iterator itr = mAccountingInfoMap.find((void *)userNamedContext);
    if (itr != mAccountingInfoMap.end())
    {
        return *itr->second;
    }
    else
    {
        FiberAccountingInfoPtr accInfo = BLAZE_NEW FiberAccountingInfo;
        mAccountingInfoMap.insert(eastl::make_pair((void *)userNamedContext, accInfo));
        return *accInfo;
    }
}

void FiberManager::filloutFiberTimings(FiberTimings& response)
{    
    response.setMainFiberCpuTime(Fiber::getMainFiber()->getFiberTime().getMicroSeconds());

    FiberTimings::FiberMetricsMap& metricsMap = response.getFiberMetrics();
    metricsMap.reserve(mAccountingInfoMap.size());
    
    for (auto& accInfoItr : mAccountingInfoMap)
    {
        auto respMetricsItr = metricsMap.find((const char8_t*)accInfoItr.first);
        FiberAccountingEntry* entry;
        
        if (respMetricsItr == metricsMap.end())
        {
            entry = metricsMap.allocate_element();
            entry->setFiberTimeMicroSecs(accInfoItr.second->getTotalTime());
            entry->setCount(accInfoItr.second->getTotalCount());
            metricsMap[(const char8_t*)accInfoItr.first] = entry;

            DbQueryCountMap& queryCounts = accInfoItr.second->getDbQueryCounts();
            queryCounts.copyInto(entry->getDbQueryCounts());
        }
        else
        {
            entry = respMetricsItr->second.get();
            entry->setFiberTimeMicroSecs(entry->getFiberTimeMicroSecs() + accInfoItr.second->getTotalTime());
            entry->setCount(entry->getCount() + accInfoItr.second->getTotalCount());

            auto& entryQueryCounts = entry->getDbQueryCounts();
            auto& queryCounts = accInfoItr.second->getDbQueryCounts();
            for (auto& queryCountItr : queryCounts)
            {
                entryQueryCounts[queryCountItr.first] += queryCountItr.second;
            }
        }
    }

    response.setCpuBudgetCheckCount(mMetrics.mCpuBudgetCheckCount.getTotal());
    response.setCpuBudgetExceededCount(mMetrics.mCpuBudgetExceededCount.getTotal());
}

BlazeError FiberManager::waitForCommandFibers(const TimeValue& absoluteTimeout, const char8_t* context)
{
    return mCommandFibersWaitList.wait(context, absoluteTimeout);
}


FiberManager::FiberPool::FiberPool(FiberManager& manager, Fiber::StackSize stackSize) : 
    mManager(manager), 
    mStackSize(stackSize), 
    mUsedLists(BlazeStlAllocator("FiberPool::mUsedLists")),
    mUsedUntaggedList(mUsedLists[(void*)FiberManager::FiberPool::UNTAGGED_MARKER]),
    mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::fiber_stack_size, stackSize)),
    mMaxPooledFibers(mMetricsCollection, "fibers.maxPooledFibers"),
    mFiberCount(mMetricsCollection, "fibers.numPooledFibers"),
    mUsedCount(mMetricsCollection, "fibers.usedFiberListSize")
{
    mMaxPooledFibers.set(1000);
}

FiberManager::FiberPool::~FiberPool()
{
    for (TaggedFiberMap::iterator itr = mUsedLists.begin(), end = mUsedLists.end(); itr != end; ++itr)
    {
        FiberPoolList& list = itr->second;
        while (!list.empty())
        {
            Fiber &dangle = static_cast<Fiber&>(list.front());
            list.pop_front();
            delete &dangle;
        }
    }

    while (!mUnusedList.empty())
    {
        Fiber &dangle = static_cast<Fiber&>(mUnusedList.front());
        mUnusedList.pop_front();
        delete &dangle;
    }
}

Fiber* FiberManager::FiberPool::get(const Fiber::CreateParams& params)
{
    Fiber *result = nullptr;
    if (!mUnusedList.empty())   
    {
        //Pop off a fiber from the unused list
        result = static_cast<Fiber*>(&mUnusedList.front());
        mUnusedList.pop_front();
    }
    else
    {
        //We need to create a new one   
        if (EA_UNLIKELY(mManager.getForceStackMemoryProtection()))
        {
            Fiber::CreateParams copyParams(params);
            copyParams.useStackProtect = true;
            result = BLAZE_NEW Fiber(copyParams, this);
        }
        else
        {
            result = BLAZE_NEW Fiber(params, this);       
        }

        mFiberCount.increment();
    }

    //push into the appropriate list
    if (params.tagged)
    {
        mUsedLists[(void*) params.namedContext].push_back(*result);
    }
    else
    {
        mUsedUntaggedList.push_back(*result);
    }

    mUsedCount.increment();

    return result;
}


void FiberManager::FiberPool::cleanupFibers(size_t& useCountTrimmed, size_t& poolTrimmed, size_t maxFiberUseCount)
{
    // Destroy any fibers that have exceeded their max use count
    FiberPoolList::iterator itr = mUnusedList.begin();
    while (itr != mUnusedList.end())
    {
        Fiber& fiber = static_cast<Fiber&>(*itr);
        if (fiber.getCallCount() >= maxFiberUseCount)
        {
            itr = mUnusedList.erase(itr);
            delete &fiber;
            mFiberCount.decrement();
            ++useCountTrimmed;
        }
        else
        {
            ++itr;
        }
    }

    // Trim the pool down to the required maximum size
    while (mFiberCount.get() > mMaxPooledFibers.get() && !mUnusedList.empty())
    {
        Fiber* fiber = static_cast<Fiber*>(&mUnusedList.front());
        mUnusedList.pop_front();
        delete fiber;
        mFiberCount.decrement();
        ++poolTrimmed;
    }
}

void FiberManager::FiberPool::getStatus(FiberManagerStatus& status) const
{
    FiberManagerStatus::FiberPoolInfo* info = status.getFiberPoolInfoList().pull_back();

    info->setStackSize(mStackSize);
    info->setMaxPooledFibers((uint32_t)mMaxPooledFibers.get());
    info->setNumPooledFibers((uint32_t)mFiberCount.get());
    info->setUsedFiberListSize((uint32_t)mUsedCount.get());
}

void FiberManager::FiberPool::onFiberComplete(Fiber& fiber)
{
    mManager.tickMetrics(fiber);
}

void FiberManager::FiberPool::onFiberReset(Fiber& fiber)
{
    FiberPoolList::remove(fiber);
    mUnusedList.push_front(fiber); //MRU routine will ensure that fibers are recycled efficiently        
    mUsedCount.decrement();
}

void FiberManager::addBlockingFiberStack(Fiber::FiberId fiberId)
{
    bool inserted = mBlockingFiberStackSet.insert(fiberId).second;
    EA_ASSERT(inserted);
}

void FiberManager::removeBlockingFiberStack(Fiber::FiberId fiberId)
{
    size_t count = mBlockingFiberStackSet.erase(fiberId);
    EA_ASSERT(count == 1);
}

void FiberManager::verifyBlockingFiberStackDoesNotExist(Fiber::FiberId fiberId)
{
    EA_ASSERT(mBlockingFiberStackSet.find(fiberId) == mBlockingFiberStackSet.end());
}

void FiberManager::tickMetrics(Fiber& fiber)
{
    const char8_t* context = fiber.getNamedContext();

    FiberAccountingInfo& accInfo = getFiberAccountingInfo(context);
    accInfo.setTotalCount(accInfo.getTotalCount() + 1);
    accInfo.setPeriodCount(accInfo.getPeriodCount() + 1);

    int64_t fiberTime = fiber.getFiberTime().getMicroSeconds();
    accInfo.setTotalTime(accInfo.getTotalTime() + fiberTime);
    accInfo.setPeriodTime(accInfo.getPeriodTime() + fiber.getFiberTimeSinceMark().getMicroSeconds());

    mMetrics.mCount.increment(1, context);
    mMetrics.mTime.increment(fiberTime, context);
    for(auto& queryCountItr : accInfo.getDbQueryCounts())
        mMetrics.mDbCount.increment(queryCountItr.second, context, queryCountItr.first.c_str());
}


} // Blaze

