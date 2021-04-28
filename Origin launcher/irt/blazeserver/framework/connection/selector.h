/*************************************************************************************************/
/*!
    \file   selector.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SELECTOR_H
#define BLAZE_SELECTOR_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_storage.h"
#include "eathread/eathread_mutex.h"
#include "EASTL/vector.h"
#include "EATDF/time.h"

#include "framework/system/job.h"
#include "framework/system/jobqueue.h"
#include "framework/system/timerqueue.h"
#include "framework/system/fibermanager.h"
#include "framework/tdf/frameworkconfigdefinitions_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/


namespace Blaze
{
// Forward declarations.
class Channel;
class SelectionKey;
class Fiber;
class Message;
class Selector;
class SelectorStatus;

extern EA_THREAD_LOCAL Selector* gSelector;

class Selector
{
    NON_COPYABLE(Selector);

// To allow access to register()/unregister()
friend class Channel;
friend class SocketChannel;
friend class ServerSocketChannel;
friend class SelectionKey;

public:
    static Selector* createSelector();

    virtual ~Selector();
    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool isOpen() = 0;

    virtual bool initialize(const SelectorConfig &config);
    SelectorConfig& getSelectorConfig() { return mConfig; }

    void run();
    void shutdown() { mIsShutdown = true; wake(); }

    void updateTimer(TimerId id, const EA::TDF::TimeValue& timeoutAbsolute);

    BlazeRpcError DEFINE_ASYNC_RET(sleep(const EA::TDF::TimeValue& duration, TimerId *timerId = nullptr, Fiber::EventHandle* eventHandle = nullptr, const char8_t* context = nullptr));

    bool cancelTimer(TimerId timerId);
    uint32_t cancelAllTimers(const void* associatedObject);
    
    void queueJob(Job* job) { mQueue.push(job); wake(); }
    void removeJob(Job* job);
   
    FiberManager& getFiberManager() { return *mFiberManager; }

    void removeAllEvents();

    EA::TDF::TimeValue getMaxConnectionProcessingTime() const { return mConfig.getMaxConnectionProcessingTime(); }

    //These functions schedule a call directly on the selector by creating a MethodCallJob object and placing it 
    //into the selector's job queue.  
    //
    //You may call this method and pass in a variable number of copyable arguments.  The generated parameters look like:
    //Job *scheduleCall(Caller*, MethodAddress, P1 ... PN, Fiber::Priority priority) 
    //Job *scheduleCall(MethodAddress, P1 ... PN, Fiber::Priority priority)
    //
    //Where 
    // Caller - Object the MethodAddress refers to if MethodAddress is a member function
    // MethodAddress - address of the member function/function to call
    // P1...PN - 0 or more parameters to call for the function.
    // Priority - The priority of the scheduled job, where lower is higher priority    
    #define SCHEDULE_CALL_DECL(_METHOD_CALL_TYPE_PARAMS, _METHOD_CALL_ARG_DECL, _METHOD_JOB_DECL, _CALLER) \
        _METHOD_CALL_TYPE_PARAMS \
        Job* scheduleCall(_METHOD_CALL_ARG_DECL, Fiber::Priority priority = 0) { \
            return scheduleCallBase(*(Blaze::Job*)(_METHOD_JOB_DECL), priority); \
        }        
    METHOD_JOB_FUNC_DECL(SCHEDULE_CALL_DECL)

    //These functions schedule a delayed call on the selector by creating a MethodCallJob object and placing it 
    //into the selector's timer queue.  This call is made on a non-blocking fiber with stack size "HUGE".
    //
    //You may call this method and pass in a variable number of copyable arguments.  The generated parameters look like:
    //Job *scheduleCall(EA::TDF::TimeValue timeoutAbsolute, Caller*, MethodAddress, P1 ... PN, const char8_t* fiberContext) 
    //Job *scheduleCall(EA::TDF::TimeValue timeoutAbsolute, MethodAddress, P1 ... PN, const char8_t* fiberContext)
    //
    //Where 
    // timeoutAbsolute - The time this call should be made
    // Caller - Object the MethodAddress refers to if MethodAddress is a member function
    // MethodAddress - address of the member function/function to call
    // P1...PN - 0 or more parameters to call for the function.
    // fiberContext - the context description this task assumes when run
    #define SCHEDULE_TIMER_CALL_DECL(_METHOD_CALL_TYPE_PARAMS, _METHOD_CALL_ARG_DECL, _METHOD_JOB_DECL, _CALLER) \
    _METHOD_CALL_TYPE_PARAMS \
    TimerId scheduleTimerCall(EA::TDF::TimeValue timeoutAbsolute, _METHOD_CALL_ARG_DECL, const char8_t *fiberContext) { \
        return scheduleTimerDedicated(timeoutAbsolute, _METHOD_JOB_DECL, _CALLER, fiberContext); \
    }
    METHOD_JOB_FUNC_DECL(SCHEDULE_TIMER_CALL_DECL)


    //Deprecated.  Use Fiber::CreateParams directly.
    typedef Fiber::CreateParams FiberCallParams;


    
    //These functions immediately schedule a call to a fiber.
    //
    //You may call this method and pass in a variable number of copyable arguments.  The generated parameters look like:
    //Job *scheduleCall(Caller*, MethodAddress, P1 ... PN, const char8_t* fiberContext, const Fiber::CreateParams& params) 
    //Job *scheduleCall(MethodAddress, P1 ... PN, const char8_t* fiberContext, const Fiber::CreateParams& params)
    //
    //Where     
    // Caller - Object the MethodAddress refers to if MethodAddress is a member function
    // MethodAddress - address of the member function/function to call
    // P1...PN - 0 or more parameters to call for the function.
    // fiberContext - the context description this task assumes when run
    // params - Parameters used to select the fiber stack size and other settings.  The fiberContext argument overrides the context passed in params.
    //
    //Deprecated, use FiberManager::scheduleCall
    #define SCHEDULE_FIBER_CALL_DECL(_METHOD_CALL_TYPE_PARAMS, _METHOD_CALL_ARG_DECL, _METHOD_JOB_DECL, _CALLER) \
        _METHOD_CALL_TYPE_PARAMS \
        bool scheduleFiberCall(_METHOD_CALL_ARG_DECL, const char *fiberContext, const Fiber::CreateParams& params = Fiber::CreateParams()) { \
            return getFiberManager().executeJob(*(Blaze::Job*)(_METHOD_JOB_DECL), Fiber::CreateParams(fiberContext, params)); \
    }
    METHOD_JOB_FUNC_DECL(SCHEDULE_FIBER_CALL_DECL)

    //These functions schedule a delayed call on the selector by creating a MethodCallJob object and placing it 
    //into the selector's timer queue.  This call is made on fiber described by the passed in parameters.
    //
    //You may call this method and pass in a variable number of copyable arguments.  The generated parameters look like:
    //Job *scheduleCall(EA::TDF::TimeValue timeoutAbsolute, Caller*, MethodAddress, P1 ... PN, const char8_t* fiberContext, const Fiber::CreateParams& params) 
    //Job *scheduleCall(EA::TDF::TimeValue timeoutAbsolute, MethodAddress, P1 ... PN, const char8_t* fiberContext, const Fiber::CreateParams& params)
    //
    //Where 
    // timeoutAbsolute - The time this call should be made
    // Caller - Object the MethodAddress refers to if MethodAddress is a member function
    // MethodAddress - address of the member function/function to call
    // P1...PN - 0 or more parameters to call for the function.
    // fiberContext - the context description this task assumes when run
    // params - Parameters used to select the fiber stack size and other settings.  The fiberContext argument overrides the context passed in params.
    //
    #define SCHEDULE_FIBER_TIMER_CALL_DECL(_METHOD_CALL_TYPE_PARAMS, _METHOD_CALL_ARG_DECL, _METHOD_JOB_DECL, _CALLER) \
        _METHOD_CALL_TYPE_PARAMS \
        TimerId scheduleFiberTimerCall(EA::TDF::TimeValue timeoutAbsolute, _METHOD_CALL_ARG_DECL, const char *fiberContext, const Fiber::CreateParams& params = Fiber::CreateParams()) { \
            return scheduleTimer(timeoutAbsolute, _METHOD_JOB_DECL, _CALLER, Fiber::CreateParams(fiberContext, params)); \
        }
    METHOD_JOB_FUNC_DECL(SCHEDULE_FIBER_TIMER_CALL_DECL)

    //These functions immediately schedule a call to a non-blocking fiber with stack size "HUGE".
    //
    //You may call this method and pass in a variable number of copyable arguments.  The generated parameters look like:
    //Job *scheduleCall(Caller*, MethodAddress, P1 ... PN, const char8_t* fiberContext, const Fiber::CreateParams& params) 
    //Job *scheduleCall(MethodAddress, P1 ... PN, const char8_t* fiberContext, const Fiber::CreateParams& params)
    //
    //Where     
    // Caller - Object the MethodAddress refers to if MethodAddress is a member function
    // MethodAddress - address of the member function/function to call
    // P1...PN - 0 or more parameters to call for the function.
    // fiberContext - the context description this task assumes when run
    //
    //Deprecated, use FiberManager::scheduleCall
    #define SCHEDULE_DEDICATED_FIBER_CALL_DECL(_METHOD_CALL_TYPE_PARAMS, _METHOD_CALL_ARG_DECL, _METHOD_JOB_DECL, _CALLER) \
    _METHOD_CALL_TYPE_PARAMS \
    void scheduleDedicatedFiberCall(_METHOD_CALL_ARG_DECL, const char* fiberContext) { \
        scheduleDedicatedFiberCallBase(_METHOD_JOB_DECL, fiberContext); \
    }
    METHOD_JOB_FUNC_DECL(SCHEDULE_DEDICATED_FIBER_CALL_DECL)


    void getStatusInfo(SelectorStatus& status);

    static void getProcessCpuUsageTime(EA::TDF::TimeValue& sys, EA::TDF::TimeValue& usr);

protected:
    Selector();

    virtual EA::TDF::TimeValue select() = 0;  // Blocking select

    virtual void wake() = 0; // Wake a blocking select() call.

    virtual bool registerChannel(Channel& channel, uint32_t interestOps) = 0;
    virtual bool unregisterChannel(Channel& channel, bool removeFd = false) = 0;
    
    virtual bool updateOps(Channel& channel, uint32_t interestOps) const = 0;

    TimerId scheduleTimerDedicated(const EA::TDF::TimeValue& timeoutAbsolute, Job* job, const void* associatedObject, const char *namedContext);
    TimerId scheduleTimer(const EA::TDF::TimeValue& timeoutAbsolute, Job* job, const void* associatedObject, const Fiber::CreateParams& fiberParams);
    
    // The returned EA::TDF::TimeValue makes sure that we don't end up sleeping indefinitely in the network if local work is present.   
    EA::TDF::TimeValue getNextExpiry(const EA::TDF::TimeValue& now) const;


    bool processJobQueue();

    Job* scheduleCallBase(Job& job, Fiber::Priority priority);
    
    void scheduleDedicatedFiberCallBase(Job* job, const char* fiberContext);


    void removePendingJobs();

    Metrics::Counter mTimerQueueTasks;
    Metrics::Counter mTimerQueueTime;
    Metrics::PolledGauge mTimerQueueSize;
    Metrics::Counter mJobQueueTasks;
    Metrics::Counter mJobQueueTime;
    Metrics::Counter mNetworkPolls;
    Metrics::Counter mNetworkTasks;
    Metrics::Counter mNetworkTime;
    Metrics::Counter mNetworkPriorityPolls;
    Metrics::Counter mNetworkPriorityTasks;
    Metrics::Counter mNetworkPriorityTime;
    Metrics::Counter mSelects;
    Metrics::Counter mWakes;

private:  
    bool mIsShutdown;
    TimerQueue mTimerQueue;
    
    JobQueue mQueue;
    Job* mNextJob;

    SelectorConfig mConfig;
    FiberManager* mFiberManager;
};  // Selector


} // Blaze

#endif // BLAZE_SELECTOR_H

