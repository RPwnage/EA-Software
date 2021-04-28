/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Fiber

    --------------------------------------------------------------------------------
        Overview
    --------------------------------------------------------------------------------

    The fiber class represents a cooperative multi-threading environment that allows many lightweight 
    execution contexts to co-exist on a single thread without the overhead of OS task switching or
    worrying about synchronization of data.  Fibers are also known as co-routines and user threads.

    --------------------------------------------------------------------------------
        How Switching Fibers Works
    --------------------------------------------------------------------------------

    The "secret sauce" of the fiber system is an assembly routine called "switch_fiber".  Each fiber
    maintains its own stack (size configurable).  When the switch_fiber call is made, the call is 
    passed both the stack of the existing fiber as well as the stack of the new fiber.  The callee 
    saved registers are pushed onto the existing stack, and the current stack pointer is saved off into
    the fiber's internal memory.  Next the waking fiber has its stack set to the stack register.  At this point
    the waking fiber is now "active".  It undoes the work it did when _it_ called switch_fiber, and returns.  

    To the calling code, the switching operation appears to do nothing - you call a method, and it returns.  But 
    what actually happens is that in between calling switch_fiber and returning another execution context
    was actually executing.  Since the model is co-operative you must call switch_fiber to have someone else run.  This 
    ensures that until you actually execute a "blocking" call (a call that executes switch_fiber) no one else can run on the 
    thread or touch your data.

    --------------------------------------------------------------------------------
        The Fiber Stack and Calling Fibers
    --------------------------------------------------------------------------------
    
    Each thread has its own set of thread-local variables for tracking fibers.  These are created by the 
    static "initThreadForFibers" method.  This method creates all the global fiber variables that
    track handles of fiber ids, groups, and events to their internal objects, as well as initializes the "main"
    fiber.  The main fiber always exists in the fiber system.  Its stack is the stack of the thread that
    initialized the fiber system.  This main fiber can "call" other fibers.  If a fiber is called, the 
    fiber that called it is the "caller" and the fiber being called is the "callee".  When the fiber is done running
    it calls "returnToCaller".  A fiber cannot be called again once its in the "called" state.  Only after
    "returnToCaller" can it be called.  The main fiber is _always_ in the called state, and never returns, so it can never
    be called. 

    To track the relationships between callers and callees, a global "fiber stack" exists.  This stack points to 
    each fiber in a the current "call chain".  For instance, if the main fiber calls fiber A, and fiber A calls fiber B, the stack 
    looks like:

    ---------
    | B     |
    ---------
    | A     |
    ---------
    | Main  |
    ---------

    This is simple enough, so lets complicate things with events.

    --------------------------------------------------------------------------------
        Events, Waiting, and Signaling, and Blocking/Non-Blocking
    --------------------------------------------------------------------------------

    The event is the atomic blocking object of the fiber system.  Once a fiber is called and awake, it can 
    choose to wait on an event by allocating an event id and calling "wait".  The event is a non-repeating 
    uint64_t that starts at 0 when the system is initialized and never repeats.  When a fiber waits on an event
    it puts itself in an intrusive hash_map which is keyed by the event id.  It then switches back to the previous
    fiber on the fiber_stack.  Note that this is NOT the same as "returnToCaller".  The fiber is still in the "called"
    state, it is just asleep and NOT in the fiber stack.   

    Once a fiber is asleep, it can only be awoken by being signaled.  The signal call is made from another fiber.  This 
    call looks up the event in the event table, takes the fiber waiting on that event and pushes it to the top of the fiber stack 
    switches to it.  That event is no longer valid, and if signaled is called again it will have no effect because the table entry
    is empty and the event id is never reused.  

    As an example, say that the main fiber calls A which calls B as before:

    ---------
    | B     |
    ---------
    | A     |
    ---------
    | Main  |
    ---------

    Now B waits on event E.  It makes a mapping of E->B in the event table, and pops itself off the stack:

    ---------
    | A     |
    ---------
    | Main  |
    ---------

    Since A was next on the stack, A continues to run.  Lets say that A then signals event E, which wakes up B and pushes it back on the stack:

    ---------
    | B     |
    ---------
    | A     |
    ---------
    | Main  |
    ---------

    B will continue to run until it either returns its execution and exits the "called" state, or waits on another event.  

    Note that not all fibers can call "wait".  A fiber is set in blocking or non-blocking mode via its CreateParameters when it is called.  When
    a fiber is blocking it can call "wait" and stop for an event.  When a fiber is non-blocking, a call to "wait" will fail with ERR_SYSTEM.  The main
    fiber is always non-blocking.  This means that since you cannot return from the main fiber and since it cannot go to sleep, it is ALWAYS at the bottom 
    of the fiber stack.  A blocking fiber can be made non-blocking via use of the "suppress blocking".  This is useful to 
    guarantee that a section of foreign code (callbacks/listener pattern) will return immediately instead of blocking.

    --------------------------------------------------------------------------------
        Auto-Join and Blocking Stacks
    --------------------------------------------------------------------------------

    When a fiber calls another fiber, it has the option of "auto joining" before it continues.  (Note that this
    differs in implementation from the formal "join" method defined below).  This behavior alters how 
    our stack behaves.  When we push a fiber onto the fiber stack, we're actually pushing a "blocking stack" onto the fiber stack,
    such that the fiber stack is actually a "stack of stacks".  If a caller auto-joins the callee fiber, then the 
    callee goes into the callers blocking stack.  If the caller does not auto-join the callee, then the callee is 
    said to be independent of the caller and puts its own blocking stack on the fiber stack.  

    Lets first illustrate this by going back to our event sample above.  First lets say that A called B without specifying 
    auto-join.  In that case, the stack looks the same:

    ---------
    | B     |
    ---------
    | A     |
    ---------
    | Main  |
    ---------

    (Remember we now know that each entry in the fiber stack is actually a blocking stack with one fiber each).  Now lets say that B waits on an event E.   
    But this time, instead of A signaling E, it just finishes its job and lets someone else worry about it.  In that case our 
    fiber stack is back to just the main fiber:

    ---------
    | Main  |
    ---------

    Lets say that the main fiber then signals E.  In that case, the blocking stack for B (which is just B) is put onto the 
    fiber stack.  

    ---------
    | B     |
    ---------
    | Main  |
    ---------

    In this manner, B is totally independent from A, even though A initially called B.  When B returns, it will wake up whomever
    is lower than it on the fiber stack, which does not have to be its caller.

    Now lets look at a case where B is called with the auto-join flag set.  In this manner, B is not pushed onto the top of the 
    fiber stack, but is instead pushed onto the top of A's blocking stack.  This looks like:
    
    ------------------
    | A     | B      |
    ------------------
    | Main  |
    ---------

    B will continue to execute until it either returns back to A, which just pops it off the blocking stack and returns control
    to A, OR until it waits for an event.  If it waits for an event, it will pop the entire blocking stack off the fiber stack
    which puts Main back into control:

    ---------
    | Main  |
    ---------

    Note that A was not called.  Because of the auto-join, it will NEVER be rewoken until B has returned.  If Main then signals event E again, 
    the entire blocking stack is pushed back onto the fiber stack, and the top of the blocking stack is rewoken. 

    ------------------
    | A     | B      |
    ------------------
    | Main  |
    ---------

    B is back in control until it waits on another event or returns control to A.

    --------------------------------------------------------------------------------
        Cancellation and Timeouts
    --------------------------------------------------------------------------------

    Fibers have the option of being canceled.  When a fiber is canceled, it is passed an
    error and that error is saved off.  If the fiber is currently waiting on an event, that event 
    is signaled and the fiber is immediately woken up.  Now that the fiber is awake, it will continue
    execution.  Any time it tries to wait on an event, that wait will fail with the cancel error. 
    In this manner, you are guaranteed that once a fiber is canceled it will finish execution in a 
    non-blocking manner.  This is important when cleaning up objects and tasks associated with them.

    Note that if the fiber is calling another fiber via auto-join, then the entire blocking stack is canceled.  
    If the fiber is calling another fiber without auto-join and the callee cancels the caller, the canceled fiber will just cancel when control is returned 
    to it.  (This situation is rare).  

    The "overall timeout" allows a maximum wall-clock time for a fiber to run.  If a task must be accomplished within 
    X amount of seconds, then X can be set as the overall timeout.  If X seconds elapse before the fiber has returned, then
    the fiber is automatically canceled.
    
    --------------------------------------------------------------------------------
        Groups and Joining
    --------------------------------------------------------------------------------

    Fibers have the option to be assigned to a group.  The group is a unique id assigned by
    "allocateGroupId".  Any number of fibers may be in a group.  Putting fibers in a group
    allows for two operations - group cancellation and group joining.  Group cancellation 
    goes through any active fibers assigned to a group and cancels them.  This is useful if 
    a particular object has spawned a lot of tasks and is to be torn down.  Assigning all
    tasks the same group id for an object allows them all to be torn down at once and without
    needing to track and cancel them individually in a list.

    Groups also allow for the "group join" operation.  This allows a number of fibers to be called 
    without the auto-join flag.  Then the "join" call is made with the group number of those fibers.  
    This call internally waits the current fiber on an event and stores that event in a special table along
    with the group id.  When the last fiber with the specified group id has finished, the event is signaled. 
    This methodology differs from the auto-join feature by allowing a set of tasks to execute in parallel. 
    It also does not involve the blocking stack.  

    (You may be wondering why do we bother with the blocking stack at all?  The reason is that having the 
     auto-join fiber explicitly joined to the callee via the blocking stack vs. an event ensures that there is
     no way the caller can be activated without first finishing the callee.  This guarantee gives us the possibility
     of passing result and return information back from a callee to the caller without worrying about memory
     being wiped or removed in the meantime).

*/
/*************************************************************************************************/


#ifndef BLAZE_FIBER_H
#define BLAZE_FIBER_H

/*** Include files *******************************************************************************/

#include "EASTL/intrusive_list.h"
#include "EASTL/intrusive_hash_map.h"
#include "EASTL/hash_map.h"
#include "EASTL/deque.h"
#include "EASTL/bonus/ring_buffer.h"
#include "framework/system/allocator/callstack.h"
#include "framework/blazebase.h"
#include "blazerpcerrors.h"

#include "framework/system/frameworkresource.h"

#include "EATDF/time.h"
#include "eathread/eathread_storage.h"

#if defined(EA_PLATFORM_LINUX)
#include <signal.h>
#include <execinfo.h>
#endif

#ifdef EA_COMPILER_GNUC
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#define WARN_UNUSED_RESULT
#endif

#if defined(EA_PLATFORM_LINUX)
#define THROW_WORKER_FIBER_ASSERT() \
    StringBuilder sb(nullptr); \
    void *btBuffer[16]; \
    char8_t **btStrings; \
    int32_t nptrs = backtrace(btBuffer, 16); \
    btStrings = backtrace_symbols(btBuffer, nptrs); \
    for (int32_t i = 0; i < nptrs; i++) \
        { \
        if (i == 0) \
            { \
            sb << "Attempt to call asynchronous function outside of a fiber."; \
            } \
            sb << "\n" << btStrings[i]; \
        } \
     EA_FAIL_MESSAGE(sb.get()); 
#else
#define THROW_WORKER_FIBER_ASSERT() \
    char8_t buf[512]; \
    blaze_snzprintf(buf, sizeof(buf), "Attempt to call asynchronous function outside of a fiber. FiberContext(%s)", Fiber::getCurrentContext()); \
    EA_FAIL_MESSAGE(buf);
#endif

#define VERIFY_WORKER_FIBER() \
    if (!Fiber::isCurrentlyBlockable()) \
    { \
        THROW_WORKER_FIBER_ASSERT() \
        return ERR_SYSTEM; \
    }

#define VERIFY_WORKER_FIBER_BOOL() \
    if (!Fiber::isCurrentlyBlockable()) \
    { \
        THROW_WORKER_FIBER_ASSERT() \
        return false; \
    }

#define ASSERT_WORKER_FIBER() \
    if (!Fiber::isCurrentlyBlockable()) \
    { \
        THROW_WORKER_FIBER_ASSERT() \
    } 

#define DEFINE_ASYNC(func) Blaze::BlazeRpcError func WARN_UNUSED_RESULT
#define DEFINE_ASYNC_RET(func) func WARN_UNUSED_RESULT

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*! *****************************************************************************/
/*! \struct fiber_context_t
    \brief  Stores per/fiber context information used by the fiber switching routine.
    \note   IMPORTANT! The stack pointer member must be the first element of the fiber 
            context because the fiber switching routine casts fiber_context_t to stackPtr.
 */
struct fiber_context_t
{
    void** stackPtr; // stores the fiber's %rsp
    char8_t* stackMem; // stores the fiber's stack memory
};


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class Fiber;
class FiberLocalVarBase;
class Job;
class Selector;
class StringBuilder;

struct FiberIdMapNode : public eastl::intrusive_hash_node {} ; //always used to track us in the map of all fiber ids.
struct FiberGroupIdListNode : public eastl::intrusive_list_node {} ; //used when we're a member of a group
struct FiberEventMapNode : public eastl::intrusive_hash_node {} ; //used when we're blocking on an event
struct FiberPoolListNode : public eastl::intrusive_list_node {}; //used when we want to pool a fiber.
struct FiberBlockingStackNode : public eastl::intrusive_list_node {}; //used when we're active and have been called.
typedef eastl::intrusive_list<FiberPoolListNode> FiberPoolList;
typedef eastl::intrusive_list<FiberGroupIdListNode> FiberGroupIdList;

class FiberListener
{
public:
    virtual ~FiberListener() {}

    //Called when the fiber completes execution of an assigned call, prior to reset
    virtual void onFiberComplete(Fiber& fiber) = 0;

    //Called when the fiber has released all resources and can be reused.
    virtual void onFiberReset(Fiber& fiber) = 0;
};

class Fiber : private FiberIdMapNode, private FiberGroupIdListNode, 
              private FiberEventMapNode, private FiberBlockingStackNode, public FiberPoolListNode              
{
    NON_COPYABLE(Fiber)
public:
    enum StackSize
    {
        STACK_MICRO = 0,
        STACK_SMALL = 1,
        STACK_MEDIUM = 2,
        STACK_LARGE = 3,
        STACK_HUGE = 4,
        MAX_STACK_SIZES //Sentinel, do not use!

    };

    static const char8_t* StackSizeToString(StackSize ss);
      
    typedef uint64_t FiberId;
    static const FiberId INVALID_FIBER_ID = UINT64_MAX;

    typedef uint64_t FiberGroupId;
    static const FiberGroupId INVALID_FIBER_GROUP_ID = UINT64_MAX;

    typedef uint64_t EventId;
    static const EventId INVALID_EVENT_ID = UINT64_MAX;

    typedef int64_t Priority;
    static const Priority INVALID_PRIORITY = 0;

    static const char8_t* UNNAMED_CONTEXT;

    static const char8_t* MAIN_FIBER_NAME;


    struct CreateParams
    {
        StackSize stackSize; //< The size stack this fiber should allocate
        bool blocking; //< The whether or not this fiber can block
        EA::TDF::TimeValue scheduledTime; //< The overall wall clock time when these parameters were created
        EA::TDF::TimeValue relativeTimeout; //< The overall wall clock time this fiber has to finish before being canceled
        Priority priority; //< The priority of the fiber against other fibers (lower is higher priority)
        const char8_t* schedulerContext;  //< The scheduling job/fiber.  This field not copied, use a static string.
        const char8_t* namedContext;  //< The type of job this fiber is doing.  This field not copied, use a static string.
        bool blocksCaller; //< If true and the calling fiber is blocking, the calling fiber will not execute until this fiber finishes
        FiberGroupId groupId; //< The group this fiber runs in.  Used in group cancellation and join methods.
        bool tagged; //< The fiber is "tagged" by its context name in the outer manager
        bool useStackProtect; //< The fiber's stack is "mprotected" with a barrier page.  If false the barrier page exists but is not protected.

        CreateParams(StackSize stacksize = STACK_LARGE, EA::TDF::TimeValue relativetimeout = 0)
            : stackSize(stacksize), 
            blocking(true),
            scheduledTime(EA::TDF::TimeValue::getTimeOfDay()),
            relativeTimeout(relativetimeout),
            priority(EA::TDF::TimeValue::getTimeOfDay().getMicroSeconds()),
            schedulerContext(getCurrentContext()),
            namedContext(UNNAMED_CONTEXT),
            blocksCaller(false),
            groupId(INVALID_FIBER_GROUP_ID),
            tagged(false),
            useStackProtect(false)
        {

        }

        //A lot of code has params + context, this allows an inline unification of those into a single params object
        CreateParams(const char8_t* context, const CreateParams& params = CreateParams())
        {
            *this = params;
            if (context != nullptr)
                namedContext = context;
        }

    };

    struct FiberHandle
    {
        FiberHandle();
        FiberHandle(FiberId id);

        bool isValid() const { return fiberId != INVALID_FIBER_ID; }
        bool isLocalToThread() const;

        void reset() { fiberId = INVALID_FIBER_ID; }

        bool operator==(const FiberHandle& other) { return other.selector == selector && other.fiberId == fiberId; }

        FiberGroupId fiberGroupId;
        FiberId fiberId;
       
    private:
        friend class Fiber;

        Selector* selector;
    };

    struct EventHandle
    {
        EventHandle();
        EventHandle(EventId id);

        bool isValid() const { return eventId != INVALID_EVENT_ID; }
        bool isLocalToThread() const;

        void reset() { eventId = INVALID_EVENT_ID; }

        bool operator==(const EventHandle& other) const { return other.selector == selector && other.eventId == eventId; }
        
        EventId eventId;
        Fiber::Priority priority;

        private:
        friend class Fiber;

        Selector* selector;
    };

    struct BlockingSuppressionAutoPtr
    {
    public:
        BlockingSuppressionAutoPtr(bool enabled = true);
        ~BlockingSuppressionAutoPtr();
    private:
        bool mEnabled;
    };

    struct CallstackSnapshotAtWaitAutoPtr
    {
    public:
        CallstackSnapshotAtWaitAutoPtr(bool enabled = true);
        ~CallstackSnapshotAtWaitAutoPtr();
    private:
        bool mEnabled;
    };

    class FiberStorageInitializer
    {
        NON_COPYABLE(FiberStorageInitializer)
    public:
        FiberStorageInitializer() { Fiber::initThreadForFibers(); };
        ~FiberStorageInitializer() { Fiber::shutdownThreadForFibers(); };
    };

    class WaitList
    {
        NON_COPYABLE(WaitList)
    public:
        WaitList();
        ~WaitList();
        int32_t getCount() const;
        BlazeRpcError wait(const char8_t* context, EA::TDF::TimeValue absoluteTimeout = EA::TDF::TimeValue());
        void signal(BlazeRpcError waitResult);

    private:
        typedef eastl::deque<Fiber::EventHandle> EventHandleList;
        EventHandleList mEventHandleList;
        bool mSignaled = false;
        bool mDestroyed = false;
    };

public:
    /* This method kicks off the fiber system.  This should be one of the first things called on 
       any thread, especially before any fiber locals are called. */

    /*! \brief Initialized the fiber system for a thread.  
        This must be called before any fiber methods are called on a thread*/
    static void initThreadForFibers();

    /*! \brief Cleanup any memory associated with the fiber system from a thread.
        Cannot be called from a fiber, and once called initThreadForFibers needs to be called before fibers are used again 
    */
    static void shutdownThreadForFibers();

    /*! \brief Used by relevant framework systems for checking that fiber storage has been correctly initialized */
    static bool getFiberStorageInitialized();

    /*! \brief Used by the framework to pause the main thread timing before waiting on the selector.
        NOTE: Without this call the msMainFiber->mFiberTime would include the wallclock time spent waiting for selector events to be signalled. 
    */
    static void pauseMainFiberTimingAt(const EA::TDF::TimeValue& time);

    /*! \brief Used by the framework to update the wake timestamp on the main thread when resuming from waiting on the selector.
        NOTE: Without this call the msMainFiber->mFiberTime would include the wallclock time spent waiting for selector events to be signalled. 
    */
    static void resumeMainFiberTimingAt(const EA::TDF::TimeValue& time)
    {
        EA_ASSERT(msCurrentFiber == msMainFiber);
        msMainFiber->mWakeTimestamp = time;
    }

    /*! \brief Accesses the main fiber. */
    static Fiber* getMainFiber() { return msMainFiber; }

    /*! \brief Gets a new event handle to use with "wait" */
    static EventHandle getNextEventHandle() { return EventHandle(msNextEventId++); }

    /*! \brief Waits the current fiber on an event to be triggered, with optional timeout. 
           
        Only one fiber can wait on any event at a time.  Waiting on an event already waited upon is undefined, so any
        event passed in should be allocated immediately prior to the wait with "getNextEventHandle"
        
        The timeout passed to this method will adjust the overall fiber timeout if the absolute timeout time is less than the 
        current fiber timeout, or if there is no overall timeout.  After the wait succeeds, the fiber timeout is readjusted.  If 
        absoluteTimeout time is reached before the event is signaled, the fiber is not canceled, but ERR_TIMEOUT is returned.  If 
        the overall fiber timeout is less than the timeout passed to the event, the timeout passed in is ignored.
    */
    static BlazeRpcError wait(const Fiber::EventHandle& id, const char8_t* context, const EA::TDF::TimeValue& absoluteTimeout = EA::TDF::TimeValue());

    /*! \brief Waits the current fiber on an event to be triggered, with optional timeout. 
        
        Identical to "wait" with the exception that the id is allocated internally to the call and set on the passed in reference.
    */
    static BlazeRpcError getAndWait(Fiber::EventHandle& id, const char8_t* context, const EA::TDF::TimeValue& absoluteTimeout = EA::TDF::TimeValue());

    /*! \brief Triggers the fiber waiting on the passed in event

        If any fiber is waiting on the passed in id, it is woken and the return code from "wait" or "getAndWait" is set to "reason"

        If no fiber is waiting on the event this is a no-op.

        This method is thread-safe.  If it is called on the thread where the event handle was created, then the 
        fiber waiting on the event is immediately woken up and begins executing from its "wait" call.  If the fiber
        is on another thread, then the selector of that thread is scheduled to wake up the fiber on the next pass.
    */
    static bool signal(const Fiber::EventHandle& id, BlazeRpcError reason);

    /*! \brief Triggers the fiber waiting on the passed in event at the beginning of the next selector loop.

        If any fiber is waiting on the passed in id, it is scheduled to woken and the return code from "wait" or "getAndWait" is set to "reason"

        If no fiber is waiting on the event this is a no-op.

        This method is thread-safe and can be called from another thread. The ehavior from another thread is identical
        to "signal", so for readability only use this method if you need to explicitly wake a fiber after the current fiber
        goes to sleep.
    */
    static void delaySignal(const Fiber::EventHandle&, BlazeRpcError reason);

    /*! \brief Puts the current fiber to sleep

        The current fiber will go to sleep for the duration specified.

        Internally the fiber is waiting on an event with context "context".  If id is non-null it will be filled
        out with a handle to the event the fiber is sleeping on.  If the fiber needs to be prematurely woken up
        that event can be signaled.
    */
    static BlazeRpcError sleep(const EA::TDF::TimeValue& duration, const char8_t* context, Fiber::EventHandle* id = nullptr);

    /*! \brief Puts the current fiber to sleep if its CPU time has exceeded a specific threshold

        Because fibers are cooperative multitasking, occasionally an especially hungry task must be put 
        to sleep to allow other fibers a chance to run.  This function facilitates that operation by checking
        the current wake time of the fiber and sleeping if it has exceeded the "allottedTime" passed in.  

        If includedCalledFibers is true then any time spent awake in fibers called by the current fiber will 
        be included in the calculation.  This is useful if the current fiber is a "master" fiber that schedules 
        a bunch of other jobs to run.  If none of the subtasks have blocked then the time they spent executing
        will be included in the calculation.
    */
    static BlazeRpcError yieldOnAllottedTime(const EA::TDF::TimeValue& allottedTime, bool includeCalledFibers = true);

    /*! \brief Yield the current fiber execution and allow other fibers to run.
    */
    static BlazeRpcError yield() { return sleep(0, "Fiber::yield"); }

    /* \brief Blocks the current fiber until all fibers in the given group have finished execution.

       When called this method will block until all fibers tagged with groupId have finished execution. 

       If no fibers are running the group, this method returns immediately.  If absoluteTimeout is reached before 
       execution is finished this method returns ERR_TIMEOUT.  

       If this method attempts to join a group the current fiber is in the method will assert and return ERR_SYSTEM. (Joining
       the current fiber will never return).
    */
    static BlazeRpcError join(FiberGroupId groupId, const EA::TDF::TimeValue& absoluteTimeout = EA::TDF::TimeValue());

    /* \brief Cancels a running fiber and any blocking fibers it has called.

       If the fiber pointed to by the given fiber handle is still running, it will be canceled with reason "reason".

       If the fiber is asleep waiting on an event, the event will be signaled with the given reason.  If the fiber is awake
       it will be marked as canceled.  Once a fiber is canceled, any further calls to "wait" will fail and return
       the error code given in "reason".  

       If a fiber blocking on a fiber, then that fiber will be canceled as well.  If another fiber is blocking on _this_ fiber
       it will _not_ be canceled. 

       The main fiber cannot be canceled.
    */
    static void cancel(const FiberHandle& id, BlazeRpcError reason);

    /* \brief Cancels any fiber tagged with the given group id */
    static void cancelGroup(FiberGroupId groupId, BlazeRpcError reason);

    /* \brief Cancels all currently executing fibers on the thread with the exception of the current fiber
       and the main fiber. */
    static void cancelAll(BlazeRpcError reason);

    /* \brief Returns a unique fiber group id that can be tagged to fibers and used in operations like cancelGroup and join */
    static FiberGroupId allocateFiberGroupId() { return msNextFiberGroupId++; }

    /* \brief Returns true if the current fiber is the main fiber of the thread */
    static bool isCurrentlyInMainFiber() { return msCurrentFiber == nullptr || msCurrentFiber->mIsMainFiber; }

    /* \brief Returns true if the current fiber is processing an exception */
    static bool isCurrentlyInException() { return msCurrentFiber->isInException(); }

    /* \brief Will cause subsequent calls to isCurrentlyInException() to return true.
              Use to prevent exception-processing code from running twice on the same fiber
              when exceptCurrentFiber() cannot be called immediately.
    */
    static void setIsInException() { return msCurrentFiber->setInException(true); }

    /* \brief Will cause subsequent calls to isCurrentlyInException() to return false.
              Use to undo a previous call to setIsInException(). Do not use if exceptCurrentFiber() has already been called.
    */
    static void unsetIsInException() { return msCurrentFiber->setInException(false); }

    /* \brief Returns true if the current fiber is canceled */
    static bool isCurrentlyCancelled() { return msCurrentFiber->isCanceled(); }

    /* \brief Returns the reason the current fiber was canceled. Otherwise, ERR_OK if not canceled. */
    static BlazeError getCurrentCancelReason() { return msCurrentFiber->getCancelReason(); }

    /* \brief Returns the time that the current fiber was canceled. Otherwise, TimeValue(0) if not canceled. */
    static EA::TDF::TimeValue getCurrentCancelTime() { return msCurrentFiber->getCancelTime(); }

    /* \brief Returns the time that the current fiber began waiting for an event that wasn't signaled in time, resulting in the fiber being canceled. Otherwise, returns TimeValue(0) if not canceled. */
    static EA::TDF::TimeValue getCurrentCanceledEventStartTime() { return msCurrentFiber->getCanceledEventStartTime(); }

    /* \brief Returns the context of the event that the current fiber was waiting for when it was canceled. Otherwise, returns TimeValue(0) if not canceled. */
    static const char8_t* getCurrentCanceledEventContext() { return msCurrentFiber->getCanceledEventContext(); }

    /* \brief Returns the context that the current fiber was calling *into* and blocked on, when the fiber was canceled. Otherwise, returns TimeValue(0) if not canceled. */
    static const char8_t* getCurrentCanceledWhileCallingContext() { return msCurrentFiber->getCanceledWhileCallingContext(); }

    /* \brief Returns true if the current fiber is blockable - that is allows blocking and has not been canceled */
    static bool isCurrentlyBlockable() { return msCurrentFiber != nullptr && msCurrentFiber->isBlockable(); }
    
    /* \brief Makes the current fiber unblockable */
    static void suppressBlocking() { ++msCurrentFiber->mBlockingSuppressionCount; }

    /* \brief Makes the current fiber blockable after supressBlocking has previously been called. */
    static void unsuppressBlocking() { if (msCurrentFiber->mBlockingSuppressionCount > 0) --msCurrentFiber->mBlockingSuppressionCount; }

    /* \brief Returns the overall timeout of the current fiber. */
    static EA::TDF::TimeValue getCurrentTimeout() { return msCurrentFiber->mTimeoutAbsolute; }

    /* \brief Returns the CPU time spent by the current fiber. */
    static EA::TDF::TimeValue getCurrentFiberTime() { return msCurrentFiber->getFiberTime(); }
    
    /* \brief Returns the priority of the current fiber. */
    static Priority getCurrentPriority() { return (msCurrentFiber != nullptr) ? msCurrentFiber->mPriority : 0; }

    /* \brief Attaches a resource to the current fiber. */
    static void attachResourceToCurrentFiber(FrameworkResource& resource) { msCurrentFiber->attachResource(resource); }

    /* \brief Signals that the currently running fiber has hit an exception. */
    static void exceptCurrentFiber() { msCurrentFiber->onException(); }

    /* \brief Sets the context of the running fiber. */
    static void setCurrentContext(const char* namedContext) { msCurrentFiber->setNamedContext(namedContext); }

    /* \brief Gets the context of the running fiber. */
    static const char* getCurrentContext() { return msCurrentFiber ? msCurrentFiber->getNamedContext() : "<no current fiber>"; }

    /* \brief Dumps the current state of any fibers running in the given group id */
    static void dumpFiberGroupStatus(Fiber::FiberGroupId id);
    static void dumpFiberGroupStatus(Fiber::FiberGroupId id, StringBuilder& output);

    /* \brief Returns the current fiber id. Use with isFiberIdInUse to check if a given fiber is running */
    static FiberId getCurrentFiberId() { return msCurrentFiber->mHandle.fiberId; }

    /* \brief Returns the current fiber group id. */
    static FiberGroupId getCurrentFiberGroupId() { return msCurrentFiber->mHandle.fiberGroupId; }

    /* \brief Checks if a fiber id is in use (fiber is running) */
    static bool isFiberIdInUse(FiberId fiberId) { return fiberId != Fiber::INVALID_FIBER_ID && msFiberIdMap->find(fiberId) != msFiberIdMap->end(); }

    /* \brief Returns number of active fibers for a specific FiberGroupId */
    static bool isFiberGroupIdInUse(Fiber::FiberGroupId id) { return msFiberGroupIdMap->find(id) != msFiberGroupIdMap->end(); }

    static FiberId getFiberIdOfBlockingStack() { return getFiberOfBlockingStack()->getHandle().fiberId; }

    static void markRedisLockStartTimeOnCurrentFiber() { getFiberOfBlockingStack()->markRedisLockStartTime(); }
    static void markRedisLockEndTimeOnCurrentFiber() { getFiberOfBlockingStack()->markRedisLockEndTime(); }
    static void incRedisLockAttemptsOnCurrentFiber() { getFiberOfBlockingStack()->incRedisLockAttempts(); }

    static void enableCallstackSnapshotAtWaitOnCurrentFiber() { msCurrentFiber->enableCallstackSnapshotAtWait(); }
    static void disableCallstackSnapshotAtWaitOnCurrentFiber() { msCurrentFiber->disableCallstackSnapshotAtWait(); }

public:
    /*! \brief Constructor */
    Fiber(const CreateParams& params, FiberListener* listener);

    /*! \brief Deconstructor */
    virtual ~Fiber();

    /*! \brief Gets the handle of the fiber */
    const FiberHandle& getHandle() const { return mHandle; }

    /*! \brief Executes a job on the fiber with the given runtime parameters */
    bool call(Job& job, const CreateParams& params);

    /*! \brief Returns the number of times "call" has been used on this fiber */
    uint32_t getCallCount() const { return mCallCount; }

    /*! \brief Gets the total amount of CPU time used by this fiber since "call" */
    EA::TDF::TimeValue getFiberTime() const;

    /*! \brief Gets the total amount of CPU time used by this fiber since "call" or since "markFiberTime" was called, which ever is later */
    EA::TDF::TimeValue getFiberTimeSinceMark() const { return getFiberTime() - mFiberTimeMarker; }

    /*! \brief Marks the fiber such that getFiberTimeSinceMark will only return the CPU time used since this was called */
    void markFiberTime() { mFiberTimeMarker = getFiberTime(); }

    /*! \brief Resets the timing variables returned by getFiberTime and getFiberTimeSinceMark */
    void clearFiberTime() { mFiberTime = mFiberTimeMarker = 0; }

    /*! \brief Attaches a resource to this fiber */
    void attachResource(FrameworkResource& resource);

    /*! \brief Detaches a resource from this fiber */
    void detachResource(FrameworkResource& resource);

    /*! \brief Gets the context this fiber is running under */
    const char8_t* getNamedContext() const { return mNamedContext; }

private:
    /*! \brief This is the constructor for the main fiber.  This should only ever be called by the initThreadForFibers method. */    
    Fiber(); 

    bool isAwake() const { return msCurrentFiber == this; }
    bool isWaiting() const { return !isAwake() && mOwningFiberStackNode != nullptr; }
    bool isExecuting() const { return mCurrentJob != nullptr || mIsMainFiber; }
   
    Fiber* getCaller();
    Fiber* getCallee();

    MemoryGroupId getFiberMemGroup() { return mMemId; }

    bool isInException() const { return mInException; }
    void setInException(bool inException) { mInException = inException; }
    void onException();
       
    bool isCanceled() const { return mCancelReason != ERR_OK; }
    BlazeError getCancelReason() const { return mCancelReason; }
    EA::TDF::TimeValue getCancelTime() const { return mCancelTime; }
    EA::TDF::TimeValue getCanceledEventStartTime() const { return mCanceledEventStartTime; }
    const char8_t* getCanceledEventContext() const { return mCanceledEventContext ? mCanceledEventContext : "nothing"; }
    const char8_t* getCanceledWhileCallingContext() const { return mCanceledWhileCallingContext ? mCanceledWhileCallingContext : "nothing"; }
    void cancel(BlazeRpcError reason);
    void uncancel();
    
    void setNamedContext(const char8_t* namedContext) { mNamedContext = namedContext; }

    void setPriority(Priority priority) { mPriority = priority; }

    /*! \brief Gets the total amount of CPU time used by this fiber since it was woken up */
    EA::TDF::TimeValue getFiberTimeSinceWake() const { return EA::TDF::TimeValue::getTimeOfDay() - mWakeTimestamp; }

    /*! \brief Gets the total amount of CPU time used by this fiber since it was called or since it (or a callee) woke up from an event, whichever is later  */
    EA::TDF::TimeValue getFiberTimeSinceWait() const { return EA::TDF::TimeValue::getTimeOfDay() - mWakeFromWaitTimestamp; }

    void setTimeout(const EA::TDF::TimeValue& timeout);
    void clearTimeout();

    void assignFiberGroupId(FiberGroupId id);
    void assignFiberId();

    void reset();
    void releaseResources();
    void removeFromPool();

    bool isBlockable() const { return mIsBlockable && mBlockingSuppressionCount == 0; }
    bool isBottomOfBlockingStack() const { return &mOwningFiberStackNode->mBlockingStack.front() == this;}

    void returnToCaller();

    BlazeRpcError waitOnEvent(EventId id, const char8_t* context, const EA::TDF::TimeValue& absoluteTimeout);
    void wakeFromWaitOnEvent(BlazeRpcError reason);

    void switchToTopFiber() { switchToFiber((Fiber&) msFiberStack->back().mBlockingStack.back()); }
    void switchToFiber(Fiber& fiberToWake);
    void postWake();

#ifdef EA_PLATFORM_LINUX
    static void runFuncBootstrap(void *_fiber, void* _tempContext);
#else
    static void runFuncBootstrap(void *_fiber);    
#endif

    void run();

    static void cancelInternal(FiberId id, BlazeRpcError reason);
    static bool signalInternal(EventId, BlazeRpcError);
    static void delaySignalInternal(EventId id, BlazeRpcError reason) { signalInternal(id, reason); }

    static Fiber* getFiberOfBlockingStack() { return (Fiber*)&msCurrentFiber->mOwningFiberStackNode->mBlockingStack.front(); }

    void markRedisLockStartTime();
    void markRedisLockEndTime();
    void incRedisLockAttempts();

    void enableCallstackSnapshotAtWait() { ++mCallstackSnapshotAtWaitCount; }
    void disableCallstackSnapshotAtWait() { --mCallstackSnapshotAtWaitCount; }

#ifdef EA_PLATFORM_LINUX
    void allocateStack(size_t stackSizeBytes, bool useStackProtection);
    void deallocateStack();
#endif

private:
    struct FiberStackNode : public eastl::intrusive_list_node
    {
        FiberStackNode() 
        {
            mpPrev = mpNext = nullptr;
        }

        typedef eastl::intrusive_list<FiberBlockingStackNode> FiberBlockingStack;
        FiberBlockingStack mBlockingStack;
    };

    struct FiberDiagnostics
    {
        FiberDiagnostics()
            : mFiberGroupId(INVALID_FIBER_GROUP_ID)
            , mFiberId(INVALID_FIBER_ID)
            , mScheduledTime(0)
            , mTimeoutAbsolute(0)
            , mCallTime(0)
            , mSchedulerContext(UNNAMED_CONTEXT)
            , mCallerContext(UNNAMED_CONTEXT)
            , mNamedContext(UNNAMED_CONTEXT)
        {
        }

        FiberGroupId mFiberGroupId;
        FiberId mFiberId;
        EA::TDF::TimeValue mScheduledTime;
        EA::TDF::TimeValue mTimeoutAbsolute;
        EA::TDF::TimeValue mCallTime;
        const char8_t* mSchedulerContext;
        const char8_t* mCallerContext;
        const char8_t* mNamedContext;
    };

    void onTimeout( FiberDiagnostics origFiberDiag);


    typedef eastl::intrusive_list<FrameworkResource> ResourceList;
    friend struct eastl::use_intrusive_key<Blaze::FiberIdMapNode, Blaze::Fiber::FiberId>;
    friend struct eastl::use_intrusive_key<Blaze::FiberEventMapNode, Blaze::Fiber::EventId>;
    friend class FiberLocalVarBase;
    friend class Component;
    friend struct ThreadLocalInfo; //Give friend access so we can track our thread locals

    FiberListener* mListener;

    bool mInException;
    bool mIsMainFiber;

    StackSize mStackSizeType;
    
    MemoryGroupId mMemId;
    MemoryGroupId mSwappedFromFiberMemId;

    Job* mCurrentJob;

    //Storage of our stack
#ifdef EA_PLATFORM_LINUX
    fiber_context_t mFiberContext;
    char8_t* mRealStackMem;
    size_t mRealStackSize;
#else
    void* mFiberPtr;
#endif

    //Timing related functionality
    EA::TDF::TimeValue mWakeTimestamp; //The thread cpu time that I woke up on (call or wait)
    EA::TDF::TimeValue mWakeFromWaitTimestamp; //The thread cpu time that I or a callee woke up from a wait
    EA::TDF::TimeValue mFiberTime; //Total CPU time taken by this fiber
    EA::TDF::TimeValue mFiberTimeMarker; //Used for "how much time since I marked this".

    Priority mPriority; //Useful for scheduling, a settable priority allows a scheduler to tag this fiber
                        //as more or less important when deciding when to run it.

    EA::TDF::TimeValue mTimeoutAbsolute;
    TimerId mTimeoutId;

    ResourceList mResourceList;

    BlazeRpcError mCancelReason;
    EA::TDF::TimeValue mCancelTime;
    EA::TDF::TimeValue mCanceledEventStartTime;
    const char8_t* mCanceledEventContext;
    const char8_t* mCanceledWhileCallingContext;

    const char8_t* mNamedContext;

    bool mIsBlockable;
    uint32_t mBlockingSuppressionCount;
    bool mBlocksCaller;

    eastl::ring_buffer<FiberDiagnostics> mFiberDiagnosticsRing;

    FiberStackNode mFiberStackNode;
    FiberStackNode *mOwningFiberStackNode;

    FiberHandle mHandle;

    EventId mCurrentEventId; 
    EA::TDF::TimeValue mCurrentEventStartTime;
    const char8_t* mCurrentEventContext;
    BlazeRpcError mCurrentEventError;

    uint32_t mCallCount;

    uint8_t* mFiberLocalStorage;

    EA::TDF::TimeValue mLastRedisLockStartTime;
    EA::TDF::TimeValue mLastRedisLockEndTime;
    EA::TDF::TimeValue mTotalRedisLockWaitTime;
    int64_t mTotalRedisLockAttempts;

    int64_t mCallstackSnapshotAtWaitCount;
    EA::TDF::TimeValue mCallstackSnapshotAtWaitTime;
    CallStack mCallstackSnapshotAtWait;
    CallStack mCallstackSnapshotAtCancellation;

private:
    static EA_THREAD_LOCAL FiberId msNextFiberId;
    typedef eastl::intrusive_hash_map<FiberId, FiberIdMapNode, 9973> FiberIdMap;
    static EA_THREAD_LOCAL FiberIdMap *msFiberIdMap;

    static EA_THREAD_LOCAL FiberGroupId msNextFiberGroupId;
    typedef eastl::hash_map<FiberGroupId, FiberGroupIdList> FiberGroupIdListMap;
    static EA_THREAD_LOCAL FiberGroupIdListMap *msFiberGroupIdMap;

    typedef eastl::hash_multimap<FiberGroupId, EventHandle> FiberGroupJoinMap; //This is a rare operation, so we shouldn't incur a cost on every fiber.
    static EA_THREAD_LOCAL FiberGroupJoinMap *msFiberGroupJoinMap;

    static EA_THREAD_LOCAL EventId msNextEventId;
    typedef eastl::intrusive_hash_map<EventId, FiberEventMapNode, 9973> EventMap;
    static EA_THREAD_LOCAL EventMap *msEventMap;

    typedef eastl::intrusive_list<FiberStackNode> FiberStack;
    static EA_THREAD_LOCAL FiberStack* msFiberStack;

    static EA_THREAD_LOCAL Fiber *msMainFiber;
    static EA_THREAD_LOCAL Fiber *msCurrentFiber;
};


} // Blaze


namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::FiberEventMapNode, Blaze::Fiber::EventId>
    {
        Blaze::Fiber::EventId operator() (const Blaze::FiberEventMapNode& x) const
        {
            return static_cast<const Blaze::Fiber&>(x).mCurrentEventId;
        }
    };

    template <>
    struct use_intrusive_key<Blaze::FiberIdMapNode, Blaze::Fiber::FiberId>
    {
        Blaze::Fiber::FiberId operator() (const Blaze::FiberIdMapNode& x) const
        {
            return static_cast<const Blaze::Fiber&>(x).mHandle.fiberId;
        }
    };
}
#endif // BLAZE_FIBER_H

