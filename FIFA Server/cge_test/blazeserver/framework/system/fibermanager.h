/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIBER_MANAGER_H
#define BLAZE_FIBER_MANAGER_H

/*** Include files *******************************************************************************/

#include "framework/system/fiber.h"
#include "framework/system/fibersemaphore.h"
#include "framework/system/blazethread.h"
#include "framework/system/job.h"
#include "framework/tdf/frameworkconfigdefinitions_server.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/metrics/metrics.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/intrusive_hash_map.h"
#include "EASTL/vector_set.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define SEMAPHORE_MAP_BUCKET_COUNT 373
#define INVALID_SEMAPHORE_ID 0

namespace Blaze
{

class Selector;
class FiberConfig;
class FiberManagerStatus;
class SelectorStatus;
class FiberTimings;
struct CommandFiberTracker;

typedef eastl::hash_map<void*, FiberAccountingInfoPtr> FiberAccountingInfoMap;

class FiberManager 
{
    NON_COPYABLE(FiberManager);
public:

    FiberManager(Selector& selector);
    virtual ~FiberManager();
    

    //These functions immediately schedule a call to a fiber.
    //
    //You may call this method and pass in a variable number of copyable arguments.  The generated parameters look like:
    // bool scheduleCall(Caller*, MethodAddress, P1 ... PN, const Fiber::CreateParams& params) 
    // bool scheduleCall(Caller*, MethodAddress,  P1 ... PN, ReturnValueAddress, const Fiber::CreateParams& params) 
    // bool scheduleCall(MethodAddress, P1 ... PN, const Fiber::CreateParams& params)
    //
    //Where     
    // Caller - Object the MethodAddress refers to if MethodAddress is a member function
    // MethodAddress - address of the member function/function to call
    // ReturnValueAddress - Address of an optional return value for a method that has a return.  Return type must be copyable, and the return address must be valid the lifetime of the call.
    // P1...PN - 0 or more parameters to call for the function.
    // params - Parameters used to select the fiber stack size and other settings.  
    // handle - Optional pointer to a handle which is filled out with the handle of the fiber assigned to run the task.
    //
    #define SCHEDULE_FIBER_MGR_CALL_DECL(_METHOD_CALL_TYPE_PARAMS, _METHOD_CALL_ARG_DECL, _METHOD_JOB_DECL, _CALLER) \
    _METHOD_CALL_TYPE_PARAMS \
    bool scheduleCall(_METHOD_CALL_ARG_DECL, const Fiber::CreateParams& params = Fiber::CreateParams(), Fiber::FiberHandle* handle = nullptr) { \
        return executeJob(*(Blaze::Job*)(_METHOD_JOB_DECL), params, handle); \
    }
    METHOD_JOB_FUNC_DECL(SCHEDULE_FIBER_MGR_CALL_DECL)
    METHOD_JOB_FUNC_DECL_RET(SCHEDULE_FIBER_MGR_CALL_DECL)

    bool executeJob(Job& job, const Fiber::CreateParams& params, Fiber::FiberHandle* handle = nullptr);
    Fiber *getFiber(const Fiber::CreateParams& params);

    Selector& getSelector() const { return mSelector; }

    void initialize();
    void configure(const FiberConfig& bootConfig);

    void clearFiberAccounting();
    void clearFiberPeriodAccounting();
    FiberAccountingInfo& getFiberAccountingInfo(const char* userNamedContext);
    void filloutFiberTimings(FiberTimings &response);

    void getStatus(FiberManagerStatus& status) const;
    uint32_t getCpuUsageForProcessPercent() const { return mLastProcessCpuUsagePercent; }
    uint32_t getSystemCpuUsageForProcessPercent() const { return mLastProcessSystemCpuUsagePercent; }
    EA::TDF::TimeValue checkCpuBudget(bool forcePrintOutExceeded = false);

    bool getForceStackMemoryProtection() const {return mFiberConfig.getForceStackMemoryProtection(); }

    uint32_t getCommandFiberCount() const { return mCommandFiberCount; }
    uint32_t getMaxCommandFiberCount() const { return mFiberConfig.getMaxCommandFiberCount(); }
    BlazeError waitForCommandFibers(const TimeValue& absoluteTimeout, const char8_t* context);
    /* 
        IMPORTANT: The following public methods will be deprecated. Instead of FiberSemaphores, use FiberEvents or Fiber::join.
    */

    SemaphoreId getSemaphore(const char8_t* name = nullptr, int32_t threshold = 1, int32_t initCount = 0);
    bool isValidSemaphore(SemaphoreId id) { return (mUsedSemaphoreMap.find(id) != mUsedSemaphoreMap.end()); }

    // queues a job on the mSelector to call signalSemaphore()
    void delaySignalSemaphore(SemaphoreId id, BlazeRpcError reason = Blaze::ERR_OK, 
        int64_t priority = EA::TDF::TimeValue::getTimeOfDay().getMicroSeconds());

    // signal the semaphore that corresponds to the id and wakes up waitingFiber if
    // 1) count == 0 (threshold is met) or 2) reason != ERR_OK  
    void signalSemaphore(SemaphoreId id, BlazeRpcError reason = Blaze::ERR_OK);

    // wait on the semaphore that corresponds to the id and puts the current fiber to sleep if count < 0
    BlazeRpcError DEFINE_ASYNC_RET(waitSemaphore(SemaphoreId id));
    BlazeRpcError DEFINE_ASYNC_RET(waitSemaphoreOrTimeout(SemaphoreId id, EA::TDF::TimeValue delay));
    BlazeRpcError DEFINE_ASYNC_RET(getAndWaitSemaphore(const char8_t* name = nullptr, int32_t threshold = 1, int32_t initCount = 0));
    
    // calls clear() on the semaphore corresponding to the id and removes it from mUsedSemaphoreMap
    void releaseSemaphore(SemaphoreId id);

    void addBlockingFiberStack(Fiber::FiberId fiberId);
    void removeBlockingFiberStack(Fiber::FiberId fiberId);
    void verifyBlockingFiberStackDoesNotExist(Fiber::FiberId fiberId);
    void tickMetrics(Fiber& fiber);

private:

    friend struct CommandFiberTracker;

    void executeDelayedJob(Job* job, Fiber::CreateParams params) { executeJob(*job, params, nullptr); }
    void cleanupFiberPools();

    class FiberPool : FiberListener
    {
        NON_COPYABLE(FiberPool);
        public:
            typedef eastl::hash_map<void*, FiberPoolList> TaggedFiberMap;

            FiberPool(FiberManager& manager, Fiber::StackSize stackSize);
            ~FiberPool() override;

            size_t getMaxPooledFibers() const { return mMaxPooledFibers.get(); }
            void setMaxPooledFibers(size_t max) { mMaxPooledFibers.set(max); }

            size_t getFiberCount() const { return mFiberCount.get(); }

            bool hasActiveFibers() const { return mUsedCount.get() > 0; }

            TaggedFiberMap& getUsedFiberMap() { return mUsedLists; }

            Fiber* get(const Fiber::CreateParams& params);

            void cleanupFibers(size_t& useCountTrimmed, size_t& poolTrimmed, size_t maxFiberUseCount);

            void getStatus(FiberManagerStatus& status) const;
        private:
            static const size_t UNTAGGED_MARKER = 0x0;

            void onFiberComplete(Fiber& fiber) override;
            void onFiberReset(Fiber& fiber) override;

            FiberManager& mManager;
            Fiber::StackSize mStackSize;

            TaggedFiberMap mUsedLists;
            FiberPoolList& mUsedUntaggedList;
            
            FiberPoolList mUnusedList;

            Metrics::MetricsCollection& mMetricsCollection;

            Metrics::Gauge mMaxPooledFibers;
            Metrics::Gauge mFiberCount;
            Metrics::Gauge mUsedCount;
    };
    void updateSystemResourceCounters();

    FiberPool& getFiberPool(Fiber::StackSize stackSize) { return *mFiberPools[stackSize]; }

private:

    BlazeRpcError DEFINE_ASYNC_RET(doWaitSemaphore(SemaphoreId id)); // deprecate
    void doSignalSemaphore(SemaphoreId id, BlazeRpcError reason = Blaze::ERR_OK); // deprecate

    FiberConfig mFiberConfig;

    Selector& mSelector;
    uint32_t mMaxFiberUseCount;
    TimerId mCleanupTimerId;
        
    typedef eastl::vector<FiberPool*> FiberPoolVector;
    FiberPoolVector mFiberPools;

    typedef eastl::intrusive_hash_map<SemaphoreId, SemaphoresByIdMapNode, SEMAPHORE_MAP_BUCKET_COUNT> SemaphoresByIdMap;
    SemaphoresByIdMap mUsedSemaphoreMap;
    typedef eastl::intrusive_list<SemaphoreListNode> SemaphoreList;
    SemaphoreList mUnusedSemaphoreList;
    SemaphoreId mSemaphoreIdCounter;
    
    BlazeThread* mThread;

    FiberAccountingInfoMap mAccountingInfoMap;
    uint32_t mCommandFiberCount;
    Fiber::WaitList mCommandFibersWaitList;
    EA::TDF::TimeValue mLastCpuBudgetCheckTime;
    EA::TDF::TimeValue mLastCpuUsageTime;
    EA::TDF::TimeValue mProcessCpuUsageInterval;
    EA::TDF::TimeValue mCurrentProcessCpuUsageTime;
    EA::TDF::TimeValue mCurrentProcessSystemCpuUsageTime;
    uint32_t mLastProcessCpuUsagePercent;
    uint32_t mLastProcessSystemCpuUsagePercent;
    EA::TDF::TimeValue mLastCpuSystemUsageTime;
    EA::TDF::TimeValue mCpuBudgetLastPrintout;
    SelectorStatus mLastCpuBudgetSelectorStatus;

    typedef eastl::vector_set<Fiber::FiberId> BlockingFiberStackSet;
    BlockingFiberStackSet mBlockingFiberStackSet;

    struct FiberMetrics
    {
        FiberMetrics();

        void reset()
        {
            mCount.reset();
            mTime.reset();
            mDbCount.reset();
        }

        Metrics::TaggedCounter<Metrics::FiberContext> mCount;
        Metrics::TaggedCounter<Metrics::FiberContext> mTime;
        Metrics::TaggedCounter<Metrics::FiberContext, Metrics::DbQueryName> mDbCount;
        Metrics::Counter mCpuBudgetCheckCount;
        Metrics::Counter mCpuBudgetExceededCount;
        Metrics::PolledGauge mUserCpuTimeForProcess;
        Metrics::PolledGauge mSystemCpuTimeForProcess;
        Metrics::PolledGauge mCpuUsageForProcessPercent;
    } mMetrics;
};

extern EA_THREAD_LOCAL FiberManager *gFiberManager;


struct CommandFiberTracker
{
public:
    CommandFiberTracker() 
    { 
        ++gFiberManager->mCommandFiberCount; 
    }

    ~CommandFiberTracker()
    {
        if (gFiberManager->mCommandFiberCount > 0)
        {
            if (--gFiberManager->mCommandFiberCount == 0)
                gFiberManager->mCommandFibersWaitList.signal(ERR_OK);
        }
    }
};


} // Blaze

#endif // BLAZE_FIBER_POOL_H

