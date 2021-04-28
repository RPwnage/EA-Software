/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "EASTL/fixed_list.h"

#include "framework/system/fiber.h"
#include "framework/system/fiberlocal.h"

#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/component/inboundrpccomponentglue.h"
#ifdef EA_PLATFORM_LINUX
#include <sys/mman.h>
#include <ucontext.h>

#define NUM_CALLEE_SAVED_REGISTERS 6

extern "C" void __attribute__ ((__noinline__, __regparm__(2))) start_fiber (fiber_context_t *prev, fiber_context_t *next);
// the 6 registers %rbp, %rbx, and %r12-%r15 are callee saved
asm(
    ".text\n"
    ".globl start_fiber\n"
    ".type start_fiber, @function\n"
    "start_fiber:\n"
    // save registers
    "push   %rbp\n"
    "push   %rbx\n"
    "push   %r12\n"
    "push   %r13\n"
    "push   %r14\n"
    "push   %r15\n"
    // save old stack pointer to *prev(%rdi)
    "mov    %rsp, (%rdi)\n"
    // load new stack pointer from *next(%rsi)
    "mov    (%rsi), %rsp\n"
    // we'll never return from this call.  Pop off the "this" arg for runfunc and call it
    "pop    %rbx\n"
    "pop    %rbp\n"
    "pop    %rdi\n"
    "pop    %rsi\n"
    "pop    %r14\n"
    "callq  *%r14\n"
    );


extern "C" void __attribute__ ((__noinline__, __regparm__(2))) switch_fiber (fiber_context_t *prev, fiber_context_t *next);
// the 6 registers %rbp, %rbx, and %r12-%r15 are callee saved
asm(
    ".text\n"
    ".globl switch_fiber\n"
    ".type switch_fiber, @function\n"
    "switch_fiber:\n"
    // save registers
    "push   %rbp\n"
    "push   %rbx\n"
    "push   %r12\n"
    "push   %r13\n"
    "push   %r14\n"
    "push   %r15\n"
    // save old stack pointer to *prev(%rdi)
    "mov    %rsp, (%rdi)\n"
    // load new stack pointer from *next(%rsi)
    "mov    (%rsi), %rsp\n"
    // restore registers
    "pop    %r15\n"
    "pop    %r14\n"
    "pop    %r13\n"
    "pop    %r12\n"
    "pop    %rbx\n"
    "pop    %rbp\n"
    "ret\n"
    );
#endif

namespace Blaze
{

EA_THREAD_LOCAL Fiber::FiberId Fiber::msNextFiberId = 0;
EA_THREAD_LOCAL Fiber::FiberIdMap* Fiber::msFiberIdMap = nullptr;

EA_THREAD_LOCAL Fiber::FiberGroupId Fiber::msNextFiberGroupId = 0;
EA_THREAD_LOCAL Fiber::FiberGroupIdListMap* Fiber::msFiberGroupIdMap = nullptr;
EA_THREAD_LOCAL Fiber::FiberGroupJoinMap* Fiber::msFiberGroupJoinMap = nullptr;

EA_THREAD_LOCAL Fiber::EventId Fiber::msNextEventId = 0;
EA_THREAD_LOCAL Fiber::EventMap* Fiber::msEventMap = nullptr;

EA_THREAD_LOCAL Fiber::FiberStack* Fiber::msFiberStack = nullptr;

EA_THREAD_LOCAL Fiber* Fiber::msCurrentFiber = nullptr;
EA_THREAD_LOCAL Fiber* Fiber::msMainFiber = nullptr;

const char8_t* Fiber::UNNAMED_CONTEXT = "UNNAMED_CONTEXT";
const char8_t* Fiber::MAIN_FIBER_NAME = "Main Fiber";


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
/*** Public Methods ******************************************************************************/

Fiber::FiberHandle::FiberHandle() : fiberGroupId(INVALID_FIBER_GROUP_ID), fiberId(INVALID_FIBER_ID), selector(gSelector) {}
Fiber::FiberHandle::FiberHandle(FiberId fId) : fiberGroupId(INVALID_FIBER_GROUP_ID), fiberId(fId),  selector(gSelector) {}
bool Fiber::FiberHandle::isLocalToThread() const { return selector == gSelector; }

Fiber::EventHandle::EventHandle() : eventId(INVALID_EVENT_ID), priority(Fiber::getCurrentPriority()), selector(gSelector) {}
Fiber::EventHandle::EventHandle(EventId eId) : eventId(eId), priority(Fiber::getCurrentPriority()), selector(gSelector) {}
bool Fiber::EventHandle::isLocalToThread() const { return selector == gSelector; }

bool Fiber::getFiberStorageInitialized()
{
    return msMainFiber != nullptr;
}

void Fiber::initThreadForFibers()
{
    //Create the fiber id map
    EA_ASSERT(msFiberIdMap == nullptr);
    msFiberIdMap = BLAZE_NEW_NAMED("Fiber Id Map") FiberIdMap();

    //Create the fiber group id map
    EA_ASSERT(msFiberGroupIdMap == nullptr);
    msFiberGroupIdMap = BLAZE_NEW_NAMED("Fiber Group Id List Map") FiberGroupIdListMap(BlazeStlAllocator("FiberGroupIdListMap"));

    //The group join map
    EA_ASSERT(msFiberGroupJoinMap == nullptr);
    msFiberGroupJoinMap = BLAZE_NEW_NAMED("Fiber Group Join Map") FiberGroupJoinMap(BlazeStlAllocator("FiberGroupJoinMap"));

    //First one in creates the event map
    EA_ASSERT(msEventMap == nullptr);
    msEventMap = BLAZE_NEW_NAMED("Fiber Event Map") EventMap();

    EA_ASSERT(msFiberStack == nullptr);
    msFiberStack = BLAZE_NEW_NAMED("Fiber Stack") FiberStack();

    //Create the new main fiber
    msMainFiber = BLAZE_NEW_NAMED(MAIN_FIBER_NAME) Fiber();
}

void Fiber::shutdownThreadForFibers()
{
    delete msMainFiber; 
    msMainFiber = nullptr;

    delete msFiberStack;
    msFiberStack = nullptr;

    delete msEventMap;
    msEventMap = nullptr;

    delete msFiberGroupJoinMap;
    msFiberGroupJoinMap = nullptr;

    delete msFiberGroupIdMap;
    msFiberGroupIdMap = nullptr;

    delete msFiberIdMap;
    msFiberIdMap = nullptr;
}

Fiber::Fiber() :
    mListener(nullptr),
    mInException(false), mIsMainFiber(true), 
    mStackSizeType(STACK_LARGE), 
    mSwappedFromFiberMemId(INVALID_MEMORY_GROUP_ID), 
    mCurrentJob(nullptr),
#ifdef EA_PLATFORM_LINUX
    mRealStackMem(nullptr), mRealStackSize(0),
#else
    mFiberPtr(nullptr),
#endif
    mWakeTimestamp(0), mFiberTime(0), mFiberTimeMarker(0),
    mPriority(0),
    mTimeoutAbsolute(0), mTimeoutId(INVALID_TIMER_ID),
    mCancelReason(ERR_OK), mCanceledEventContext(nullptr), mCanceledWhileCallingContext(nullptr), mNamedContext(UNNAMED_CONTEXT),
    mIsBlockable(false), mBlockingSuppressionCount(0), mBlocksCaller(false),
    mCurrentEventId(INVALID_EVENT_ID), mCurrentEventContext(nullptr), mCurrentEventError(ERR_OK),
    mCallCount(0),
    mTotalRedisLockAttempts(0),
    mCallstackSnapshotAtWaitCount(0)
{
    //create our private fiber allocator
    mMemId = MEM_GROUP_FRAMEWORK_DEFAULT;

    mFiberStackNode.mpNext = nullptr;
    mFiberStackNode.mpPrev = nullptr;

#ifdef EA_PLATFORM_LINUX
    // NOTE: The main fiber uses the thread's own stack,
    // there is no need to allocate a stack for it.
    memset(&mFiberContext, 0, sizeof(mFiberContext));
#else
    mFiberPtr = ConvertThreadToFiber(nullptr);
#endif

    mNamedContext = MAIN_FIBER_NAME; //will never change

    //Our accounting starts right now.
    mWakeTimestamp = TimeValue::getTimeOfDay();    

    msCurrentFiber = this;
    mFiberLocalStorage = FiberLocalVarBase::initFiberLocals(mMemId);
    FiberLocalVarBase::setFiberLocalStorageBase(mFiberLocalStorage);

    FiberGroupIdListNode::mpNext = FiberGroupIdListNode::mpPrev = nullptr;
    FiberPoolListNode::mpNext = FiberPoolListNode::mpPrev = nullptr;

    //We're the main fiber so we're always the bottom of the blocking stack and the fiber stack
    mFiberStackNode.mBlockingStack.push_front(*this);
    msFiberStack->push_front(mFiberStackNode);
    mOwningFiberStackNode = &mFiberStackNode;
    
    assignFiberId();

    mFiberDiagnosticsRing.reserve(10);
    // make sure there's one to start (mostly default values)
    auto& diag = mFiberDiagnosticsRing.push_back();
    diag.mFiberId = mHandle.fiberId;
}


Fiber::Fiber(const Fiber::CreateParams& createParams, FiberListener* listener) :            
    mListener(listener),
    mInException(false), mIsMainFiber(false), 
    mStackSizeType(createParams.stackSize),
    mSwappedFromFiberMemId(INVALID_MEMORY_GROUP_ID), 
    mCurrentJob(nullptr),
#ifdef EA_PLATFORM_LINUX
    mRealStackMem(nullptr), mRealStackSize(0),
#else
    mFiberPtr(nullptr),
#endif
    mWakeTimestamp(0), mFiberTime(0), mFiberTimeMarker(0),
    mPriority(createParams.priority),
    mTimeoutAbsolute(0), mTimeoutId(INVALID_TIMER_ID),
    mCancelReason(ERR_OK), mCanceledEventContext(nullptr), mCanceledWhileCallingContext(nullptr), mNamedContext(createParams.namedContext),
    mIsBlockable(createParams.blocking), mBlockingSuppressionCount(0), mBlocksCaller(false),
    mOwningFiberStackNode(nullptr),
    mCurrentEventId(INVALID_EVENT_ID), mCurrentEventContext(nullptr), mCurrentEventError(ERR_OK),
    mCallCount(0),
    mTotalRedisLockAttempts(0),
    mCallstackSnapshotAtWaitCount(0)
{
    size_t stackSizeBytes = 0;
    switch (mStackSizeType)
    {
#if ENABLE_CLANG_SANITIZERS
    // Sanitizers can increase the stack size usage by 2-3x due to creation of "red zones" around read/store operations. As we don't really care for the RAM
    // usage during testing, we override the fiber sizes for accommodating higher stack usage. 
    // The numbers below are a bit arbitrary but something that have worked reliably (no false positives) when running full Arson regression. They try to increase the
    // lower stack size more and higher stack size less (otherwise, we might end up blowing our over all thread stack size).
    // These numbers may end up getting tuned further in future when following outstanding bugs with ASan stuff get resolved.
        // 1. https://eadpjira.ea.com/browse/GOS-31380 
        // 2. https://eadpjira.ea.com/browse/GOS-31376 
    case STACK_MICRO: 
        stackSizeBytes = BUFFER_SIZE_16K * 6;
        break;
    case STACK_SMALL: 
        stackSizeBytes = BUFFER_SIZE_64K * 4;
        break;
    case STACK_MEDIUM: 
        stackSizeBytes = BUFFER_SIZE_128K * 4;
        break;
    case STACK_LARGE: 
        stackSizeBytes = BUFFER_SIZE_256K * 4; 
        break;
    case STACK_HUGE: 
        stackSizeBytes = BUFFER_SIZE_1024K * 2;
        break;
    default: 
        stackSizeBytes = BUFFER_SIZE_256K; 
        EA_FAIL(); 
        break;
#else
    case STACK_MICRO: 
        stackSizeBytes = BUFFER_SIZE_16K; 
        break;
    case STACK_SMALL: 
        stackSizeBytes = BUFFER_SIZE_64K; 
        break;
    case STACK_MEDIUM: 
        stackSizeBytes = BUFFER_SIZE_128K; 
        break;
    case STACK_LARGE: 
        stackSizeBytes = BUFFER_SIZE_256K; 
        break;
    case STACK_HUGE: 
        stackSizeBytes = BUFFER_SIZE_1024K; 
        break;
    default: 
        stackSizeBytes = BUFFER_SIZE_64K; 
        EA_FAIL(); 
        break;
#endif
    };

    mFiberStackNode.mpNext = nullptr;
    mFiberStackNode.mpPrev = nullptr;

    //create our private fiber allocator
    mMemId = MEM_GROUP_FRAMEWORK_DEFAULT;

    mFiberLocalStorage = FiberLocalVarBase::initFiberLocals(mMemId);

#ifdef EA_PLATFORM_LINUX
    allocateStack(stackSizeBytes, createParams.useStackProtect);

    //Temporary context used to swap in and back out of the fiber bootstrap.
    fiber_context_t tempFiberContext;
    
    // Set up the stack frame used to trampoline into the Fiber::RunFuncBootstrap method
    // when start_fiber is called on this fiber's context
    mFiberContext.stackPtr = (void **)((char *)mFiberContext.stackMem + stackSizeBytes);
    *--mFiberContext.stackPtr = (void *) 0; // pad for alignment, using a '0' ensures gdb will not unwind past this frame
    *--mFiberContext.stackPtr = (void *) 0; 
    *--mFiberContext.stackPtr = (void *)&Fiber::runFuncBootstrap; //address of func to call 
    *--mFiberContext.stackPtr = (void *)&tempFiberContext; // arg to RunFunc
    *--mFiberContext.stackPtr = (void *)this; // arg to RunFunc
    mFiberContext.stackPtr -= 2;

    start_fiber(&tempFiberContext, &mFiberContext);
#else
    mFiberPtr = CreateFiber(stackSizeBytes, &Fiber::runFuncBootstrap, this);
#endif    

    //Start out in the unused pool
    FiberPoolListNode::mpNext = FiberPoolListNode::mpPrev = nullptr;

    //We don't belong to any fiber stack
    FiberBlockingStackNode::mpNext = FiberBlockingStackNode::mpPrev = nullptr;
    mOwningFiberStackNode = nullptr;

    assignFiberId();
    assignFiberGroupId(createParams.groupId);    

    mFiberDiagnosticsRing.reserve(10);
    // make sure there's one to start (mostly default values)
    auto& diag = mFiberDiagnosticsRing.push_back();
    diag.mFiberGroupId = mHandle.fiberGroupId;
    diag.mFiberId = mHandle.fiberId;
}

Fiber::~Fiber()
{
    // clear out the timeout (if still active)
    clearTimeout();
    removeFromPool();

    delete mCurrentJob;

    //Clear our ids
    if (mHandle.fiberId != INVALID_FIBER_ID)
    {
        msFiberIdMap->erase(mHandle.fiberId);
        mHandle.fiberId = INVALID_FIBER_ID;
    }

    if (mHandle.fiberGroupId != INVALID_FIBER_GROUP_ID)
    {
        assignFiberGroupId(INVALID_FIBER_GROUP_ID);
    }

    //If we have any events, clear them out
    if (mCurrentEventId != INVALID_EVENT_ID)
    {
        msEventMap->erase(mCurrentEventId);
        mCurrentEventId = INVALID_EVENT_ID;
    }

#ifdef EA_PLATFORM_LINUX
    if (!mIsMainFiber)
    {
        deallocateStack();
    }
#else
    if (!mIsMainFiber)
    {
        DeleteFiber(mFiberPtr);
        mFiberPtr = nullptr;
    }
    else
    {
        ConvertFiberToThread();
    }
#endif
    

    FiberLocalVarBase::destroyFiberLocals(mFiberLocalStorage);    

    if (mFiberStackNode.mpNext != nullptr && mFiberStackNode.mpPrev != nullptr)
    {
        FiberStack::remove(mFiberStackNode);
        mFiberStackNode.mpNext = mFiberStackNode.mpPrev = nullptr;
    }
}

void Fiber::removeFromPool()
{
    if (FiberPoolListNode::mpNext != nullptr && FiberPoolListNode::mpPrev != nullptr)
    {
        FiberPoolList::remove(*this);
        FiberPoolListNode::mpNext = FiberPoolListNode::mpPrev = nullptr;
    }
}

Fiber* Fiber::getCaller()
{ 
    if (mOwningFiberStackNode != nullptr)
    {
        if (&mOwningFiberStackNode->mBlockingStack.front() == this)
        {
            return (Fiber*) &(((FiberStackNode*) mOwningFiberStackNode->mpPrev)->mBlockingStack.back());
        }
        else
        {
            return (Fiber*) (FiberBlockingStackNode*) FiberBlockingStackNode::mpPrev;
        }
    }
    else
    {
        return nullptr;
    }
}

Fiber* Fiber::getCallee()
{ 
    if (mOwningFiberStackNode != nullptr)
    {
        if (&mOwningFiberStackNode->mBlockingStack.back() == this)
        {
            //We're the end of our blocking stack, go to the next blocking stack
            if (&msFiberStack->back() != mOwningFiberStackNode)
            {
                return (Fiber*) &(((FiberStackNode*) mOwningFiberStackNode->mpNext)->mBlockingStack.front());
            }
            else
            {
                return nullptr;
            }            
        }
        else
        {
            return (Fiber*) (FiberBlockingStackNode*) FiberBlockingStackNode::mpNext;
        }
    }
    else
    {
        return nullptr;
    }
}

TimeValue Fiber::getFiberTime() const
{
    TimeValue result = mFiberTime;

    //if we're awake we need to add in the time we've spent since the last time we woke up
    if (isAwake())
    {
        result += getFiberTimeSinceWake();
    }

    return result;
}



BlazeRpcError Fiber::yieldOnAllottedTime(const TimeValue& allottedTime, bool includeCalledFibers /* = true */)
{    
    if ((includeCalledFibers ? msCurrentFiber->getFiberTimeSinceWait() : msCurrentFiber->getFiberTimeSinceWake()) > allottedTime)
    {
        return sleep(0, "Fiber::yieldOnAllottedTime");
    }

    return ERR_OK;
}

BlazeRpcError Fiber::join(Fiber::FiberGroupId groupId, const TimeValue& absoluteTimeout)
{
    if (EA_UNLIKELY(msCurrentFiber->getHandle().fiberGroupId == groupId))
    {
        //bad news - this would block forever if we allowed it.
        EA_FAIL_MESSAGE("Cannot call Fiber::join on the group the current fiber is running in!");
        return ERR_SYSTEM;
    }
    
    //If there are any threads tagged to the group, wait on them to finish
    if (isFiberGroupIdInUse(groupId))
    {
        EventHandle ev = Fiber::getNextEventHandle();
        msFiberGroupJoinMap->insert(FiberGroupJoinMap::value_type(groupId, ev));
        return wait(ev, "Fiber::join", absoluteTimeout);
    }
    else
    {
        return ERR_OK;
    }
}


bool Fiber::call(Job& job, const CreateParams& params)
{    
    EA_ASSERT(gSelector == mHandle.selector);

    if (EA_UNLIKELY(msCurrentFiber == this))
    {
        EA_FAIL_MSG("Fiber::call Fiber cannot call itself!");
        return false;
    }

    ++mCallCount;

    //Here's what we'll actually do.
    mCurrentJob = &job;

    TimeValue timeoutAbsolute = params.relativeTimeout > 0 ? TimeValue::getTimeOfDay() + params.relativeTimeout : 0;

    // Build the Diag info prior to the setTimeout call, so it's ready to be cached:
    auto& diag = mFiberDiagnosticsRing.push_back();
    diag.mFiberGroupId = mHandle.fiberGroupId;
    diag.mFiberId = mHandle.fiberId;
    diag.mScheduledTime = params.scheduledTime;
    diag.mTimeoutAbsolute = timeoutAbsolute;
    diag.mCallTime = TimeValue::getTimeOfDay();
    diag.mSchedulerContext = params.schedulerContext;
    diag.mCallerContext = getCurrentContext();
    diag.mNamedContext = params.namedContext;

    //Set our call settings based on the passed parameters
    mIsBlockable = params.blocking;
    setPriority(params.priority);
    setTimeout(timeoutAbsolute);
    setNamedContext(params.namedContext);
    mBlocksCaller = params.blocksCaller && params.blocking;
    assignFiberGroupId(params.groupId);

    //This is set here on the way "up", and reset after we wake up on an event
    //Calling into and waking up from a return of another fiber does NOT reset this.
    mWakeFromWaitTimestamp = TimeValue::getTimeOfDay(); 

    if (msCurrentFiber->isBlockable() && mBlocksCaller)
    {
        //We're joining the blocking stack of the calling fiber, so set our owning node to that
        mOwningFiberStackNode = msCurrentFiber->mOwningFiberStackNode;
    }
    else
    {        
        //We're the start of the blocking stack, so push our blocking stack onto the global fiber stack
        //and set our owning fiber stackNode to our own
        mOwningFiberStackNode = &mFiberStackNode;
        msFiberStack->push_back(mFiberStackNode);
    }

    //We go onto the back of the owning stack.
    mOwningFiberStackNode->mBlockingStack.push_back(*this);

    msCurrentFiber->switchToFiber(*this);

    return isWaiting();
}

void Fiber::returnToCaller()
{
    EA_ASSERT(!mIsMainFiber);

    //pop us off our blocking stack
    mOwningFiberStackNode->mBlockingStack.remove(*this);
    FiberBlockingStackNode::mpNext = FiberBlockingStackNode::mpPrev = nullptr;
    
    //if our blocking stack is empty pop that too
    if (mOwningFiberStackNode->mBlockingStack.empty())
    {
        msFiberStack->remove(*mOwningFiberStackNode);
        mOwningFiberStackNode->mpNext = mOwningFiberStackNode->mpPrev = nullptr;
    }
    mOwningFiberStackNode = nullptr;
        
    //Go to the top of the stack
    switchToTopFiber();
}


BlazeRpcError Fiber::wait(const Fiber::EventHandle& id, const char8_t* context, const TimeValue& absoluteTimeout)
{
    return msCurrentFiber->waitOnEvent(id.eventId, context, absoluteTimeout);
}

BlazeRpcError Fiber::getAndWait(Fiber::EventHandle& id, const char8_t* context, const TimeValue& absoluteTimeout)
{
    id = getNextEventHandle();
    BlazeRpcError rc = msCurrentFiber->waitOnEvent(id.eventId, context, absoluteTimeout);
    id.reset();

    return rc;
}

BlazeRpcError Fiber::sleep(const TimeValue& duration, const char8_t* context, Fiber::EventHandle* id)
{
    BlazeRpcError rc = gSelector->sleep(duration, nullptr, id, context);
    if (id != nullptr)
        id->reset();

    return rc;
}

void Fiber::cancel(const Fiber::FiberHandle& id, BlazeRpcError reason)
{
    if (EA_UNLIKELY(!id.isValid()))
        return;

    if (id.isLocalToThread())
    {
        cancelInternal(id.fiberId, reason);
    }
    else
    {
        //Send to the selector
        id.selector->scheduleCall(&Fiber::cancelInternal, id.fiberId, reason);
    }
}

void Fiber::cancelInternal(FiberId id, BlazeRpcError reason)
{
    FiberIdMap::iterator itr = msFiberIdMap->find(id);
    if (itr != msFiberIdMap->end())
    {
        Fiber& fiber = static_cast<Fiber&>(*itr);
        fiber.cancel(reason);
    }
}

void Fiber::cancel(BlazeRpcError reason)
{
    EA_ASSERT(!mIsMainFiber);

    if (!mIsMainFiber && !isCanceled())
    {
        mCancelReason = reason;
        mCancelTime = TimeValue::getTimeOfDay();

        //If the fiber isn't us, there are two possibilities - its waiting on an event or calling someone
        if (isWaiting())
        {
            Fiber& top = static_cast<Fiber&>(mOwningFiberStackNode->mBlockingStack.back());

            // We go in reverse through our blocking stack so that we can grab the context from
            // each fiber in the stack, and assign it to the caller diagnostics members.
            Fiber* blockingFiber = nullptr;
            FiberStackNode::FiberBlockingStack::reverse_iterator r_itr(mOwningFiberStackNode->mBlockingStack.rbegin()); // "reverse" begin() is really end()-1.
            FiberStackNode::FiberBlockingStack::reverse_iterator r_thisNode(FiberStackNode::FiberBlockingStack::iterator(this)); // 'this' intrusive node as a reverse iterator.
            for (; r_itr != r_thisNode; ++r_itr)
            {
                Fiber& blockedFiber = (static_cast<Fiber&>(*r_itr));
                blockedFiber.mCancelReason = reason;
                blockedFiber.mCancelTime = mCancelTime;
                blockedFiber.mCanceledEventStartTime = top.mCurrentEventStartTime;
                blockedFiber.mCanceledEventContext = top.mCurrentEventContext;
                if (blockingFiber != nullptr)
                    blockedFiber.mCanceledWhileCallingContext = blockingFiber->mNamedContext;
                blockingFiber = &blockedFiber;
            }

            if (top.mCurrentEventId != INVALID_EVENT_ID)
            {
                //The stack is waiting on an event, just signal the event, the rest of the chain will run and return back to us.
                top.wakeFromWaitOnEvent(reason);
            }
            else 
            {
                //The blocking stack is calling into a non-joined fiber which is either the current fiber or someone who called the current fiber.
                //Just like an event, we push the blocking stack to the top of the fiber stack and go from there
                msFiberStack->remove(*mOwningFiberStackNode);
                mOwningFiberStackNode->mpNext = mOwningFiberStackNode->mpPrev = nullptr;
                msFiberStack->push_back(*mOwningFiberStackNode);

                //wake the top of the stack, which is now canceled
                msCurrentFiber->switchToFiber(top);
            }
        }
        else
        {
            //We're canceling ourselves.  This is odd but not illegal. So, since the reason is already set, we just continue on.
        }
    }
}

void Fiber::uncancel()
{
    mCancelReason = ERR_OK;
    mCancelTime = TimeValue();
    mCanceledEventStartTime = TimeValue();
    mCanceledEventContext = nullptr;
    mCanceledWhileCallingContext = nullptr;
}

void Fiber::cancelGroup(Fiber::FiberGroupId groupId, BlazeRpcError reason)
{
    //First we fetch all the current fibers in this group and push their handles to a temp list. 
    //The reason for this is that canceling the fiber will eliminate it from our group list, and while a fiber
    //is canceling it could do something to the other fibers (restart them, end them, kick off other fibers, etc).
    //The handle indirection (as opposed to saving off pointers) ensures that only the unique instance of the 
    //fiber currently in the map is canceled, and not the instance of the fiber that is potentially recycled 

    //One important thing here is that we roughly order the fibers - We push the "blocking stack bottoms"
    //to the front which ensures that their entire blocking stack is canceled in a single call to cancel.  Then we add all the non-bottoms
    //to the end of the list. Any fiber in the group is considered canceled at this point in time, 
    //so canceling a later fiber in the stack first would allow a lower stack to run without being canceled.
    typedef eastl::fixed_list<FiberHandle, 8> HandleList;
    HandleList cancelHandles;
    FiberGroupIdListMap::iterator it = msFiberGroupIdMap->find(groupId);
    if (it != msFiberGroupIdMap->end())
    {
        for (FiberGroupIdList::iterator i = it->second.begin(), e = it->second.end(); i != e; ++i)
        {
            Fiber& toCancel = static_cast<Fiber&>(*i);
            if (toCancel.isBottomOfBlockingStack())
            {
                cancelHandles.push_front(toCancel.getHandle());
            }
            else
            {
                cancelHandles.push_back(toCancel.getHandle());
            }
        }
    }

    //Now cancel it all.
    for (HandleList::iterator itr = cancelHandles.begin(), end = cancelHandles.end(); itr != end; ++itr)
    {
        Fiber::cancel(*itr, reason);
    }
}

void Fiber::cancelAll(BlazeRpcError reason)
{
    //We cancel everything but the running fiber and the main fiber.
    typedef eastl::list<FiberHandle> HandleList;
    HandleList cancelHandles;    
    for (FiberIdMap::const_iterator itr = msFiberIdMap->begin(), end = msFiberIdMap->end(); itr != end; ++itr)
    {
        const Fiber& toCancel = static_cast<const Fiber&>(*itr);

        if (toCancel.isExecuting() && !toCancel.isAwake() && !toCancel.mIsMainFiber)
        {
            if (toCancel.isBottomOfBlockingStack())
            {
                cancelHandles.push_front(toCancel.getHandle());
            }
            else
            {
                cancelHandles.push_back(toCancel.getHandle());
            }
        }
    }

    //Now cancel it all.
    for (HandleList::iterator itr = cancelHandles.begin(), end = cancelHandles.end(); itr != end; ++itr)
    {
        Fiber::cancel(*itr, reason);
    }
}

bool Fiber::signal(const EventHandle& id, BlazeRpcError reason)
{
    if (EA_UNLIKELY(!id.isValid()))
        return false;

    bool result = true;
    if (id.isLocalToThread())
    {
        result = signalInternal(id.eventId, reason);
    }
    else
    {
        //If we're not on the thread, we have to delay
        delaySignal(id, reason);
    }
    
    return result;
}

void Fiber::delaySignal(const EventHandle& id, BlazeRpcError reason)
{    
    id.selector->scheduleCall(&Fiber::delaySignalInternal, id.eventId, reason, id.priority);
}

bool Fiber::signalInternal(EventId id, BlazeRpcError reason)
{
    bool result = false;
    EventMap::iterator itr = msEventMap->find(id);
    if (itr != msEventMap->end())
    {
        result = true;
        Fiber& fiber = static_cast<Fiber&>(*itr);
        fiber.wakeFromWaitOnEvent(reason);
    }

    return result;
}

BlazeRpcError Fiber::waitOnEvent(EventId id, const char8_t* context, const TimeValue& absoluteTimeout)
{
    if (EA_UNLIKELY(!isBlockable()))
    {
        EA_FAIL_MSG("[Fiber::waitOnEvent]: Attempting to wait on an event on a non-blockable thread!");
        return ERR_SYSTEM;
    }

    if (isCanceled())
    {
        return mCancelReason; 
    }

    TimeValue origTimeout = mTimeoutAbsolute;
    bool timeoutSupplied = (absoluteTimeout != 0 && (origTimeout == 0 || absoluteTimeout < origTimeout));
    if (timeoutSupplied)
    {
        setTimeout(absoluteTimeout);
    }

    //Set our current even information
    mCurrentEventId = id;
    mCurrentEventContext = context;
    mCurrentEventStartTime = TimeValue::getTimeOfDay();
    mCurrentEventError = ERR_OK;

    //insert us into the global map of waiting events.
    msEventMap->insert(*this);

    //Pop off the current fiber stack node, we'll wake up later
    msFiberStack->remove(*mOwningFiberStackNode);
    mOwningFiberStackNode->mpNext = mOwningFiberStackNode->mpPrev = nullptr;

    TimeValue sleepTime;
    if (gRpcContext != nullptr)
        sleepTime = TimeValue::getTimeOfDay();

    //Wake up the new top of the stack.
    switchToTopFiber();

    if (gRpcContext != nullptr)
    {
        auto& waitInfo = gRpcContext->mWaitTimes[context];
        waitInfo.first += (mWakeFromWaitTimestamp - sleepTime);
        waitInfo.second++;
    }

    //remove us from the global map of waiting events.
    // NOTE: intentionally erase by key, remove(*this) internally calls erase(iterator) which is *not* efficent on large empty intrusive hash maps
    msEventMap->erase(mCurrentEventId);

    //we've woken up, clear our event state and return any error.
    BlazeRpcError result = mCurrentEventError;
    mCurrentEventId = INVALID_EVENT_ID;
    mCurrentEventContext = nullptr;
    mCurrentEventStartTime = TimeValue();
    mCurrentEventError = ERR_OK;

    //reset the timer
    if (timeoutSupplied)
    {
        if (result == ERR_TIMEOUT && mTimeoutId == INVALID_TIMER_ID)
        {
            //we hit the timeout.  Uncancel us, return the error
            uncancel();
        }
        setTimeout(origTimeout);
    }

    if (isCanceled())
    {
        mCallstackSnapshotAtCancellation.getStack();
    }

    return result;
}

void Fiber::wakeFromWaitOnEvent(BlazeRpcError reason)
{
    msFiberStack->push_back(*mOwningFiberStackNode);

    //Set everyone in the blocking stack to the same wake timestamp.
    for (FiberStackNode::FiberBlockingStack::iterator itr = mOwningFiberStackNode->mBlockingStack.begin(),
        end = mOwningFiberStackNode->mBlockingStack.end(); itr != end; ++itr)
    {
        ((Fiber&) *itr).mWakeFromWaitTimestamp = TimeValue::getTimeOfDay();
    }

    mCurrentEventError = reason;
    msCurrentFiber->switchToFiber(*this);
}

void Fiber::switchToFiber(Fiber& fiberToWake)
{     
    if (EA_UNLIKELY(!isAwake()))
    {       
        BLAZE_ERR_LOG(Log::SYSTEM, "[Fiber].sleep() : INVALID OPERATION - trying to sleep a fiber that is already asleep.");
        EA_FAIL();
        return;
    }

    //Gets a little tricky here.  Because our stack contains our memory, we can't set it 
    //non-writable until we swap out. At the same time, the waking thread has the opposite issue.
    //So we set them writable, let them know who we are, and they do the same for us.  
    //Note that we CANNOT use the "mCallingFiber" pointer because this goes both ways - call and return
    //Allocator::getAllocator(fiberToWake.mMemId)->setMemoryWritable(true);
    fiberToWake.mSwappedFromFiberMemId = mMemId;

    TimeValue now = TimeValue::getTimeOfDay();

    //Take care of our time accounting.  
    mFiberTime += now - mWakeTimestamp;

    //Note that the time to swap fibers is assigned to the waking fiber. 
    fiberToWake.mWakeTimestamp = now;

    if (mCallstackSnapshotAtWaitCount > 0)
    {
        mCallstackSnapshotAtWaitTime = TimeValue::getTimeOfDay();
        mCallstackSnapshotAtWait.getStack();
    }

    //Switch to this fiber's context
#ifdef EA_PLATFORM_LINUX
    ::switch_fiber(&mFiberContext, &(fiberToWake.mFiberContext));
#else
    ::SwitchToFiber(fiberToWake.mFiberPtr);
#endif

    //Finish off in another method
    postWake();
}

//Second half of switchToFiber.  We need to call this from both switchToFiber and our bootstrap.
void Fiber::postWake()
{
    //The opposite action as above - the nice fiber that woke us up needs its memory shutdown,
    //so we oblige.  Then we forget about it as we don't want to reference that mem anymore.
    //Allocator::getAllocator(mSwappedFromFiberMemId)->setMemoryWritable(false);
    mSwappedFromFiberMemId = INVALID_MEMORY_GROUP_ID;

    //We're now the running fiber.
    msCurrentFiber = this;

    //Set the global fiber memory pointer to our own
    FiberLocalVarBase::setFiberLocalStorageBase(mFiberLocalStorage);
}


void Fiber::onException()
{
    mInException = true; 

    //Clean up fiber
    reset();

    //Clean us out of fiber id map.
    if ((msFiberIdMap != nullptr) && (mHandle.fiberId != INVALID_FIBER_ID))
        msFiberIdMap->erase(mHandle.fiberId);

    //Clean us up out of any pool tracking us
    removeFromPool();

    //Die back to the calling fiber
    returnToCaller();
}

void Fiber::attachResource(FrameworkResource& resource)
{
    resource.setOwningFiber(*this);
    mResourceList.push_back(resource);
}

void Fiber::detachResource(FrameworkResource& resource)
{
    mResourceList.remove(resource);
    resource.mpNext = resource.mpPrev = nullptr;
}

/*** Protected Methods ***************************************************************************/

#ifdef EA_PLATFORM_LINUX
void Fiber::runFuncBootstrap(void* __this, void* _tempContext)
#else
void Fiber::runFuncBootstrap(void* __this)
#endif
{   
    Fiber* _this = (Fiber*) __this;

#ifdef EA_PLATFORM_LINUX
    //Immediately swap back to the calling thread - the first wake is just to "prime the pump".  
    switch_fiber(&(_this->mFiberContext), (fiber_context_t*) _tempContext);
#endif

    _this->postWake(); //Go to sleep - the next wake will be from our first call()

    //Never returns from this
    _this->run();
}

void Fiber::run()
{
    //Running just executes a function call, then resets and waits for the next call
    //This allows us to reuse the memory for setting up the fiber call, but nothing 
    //would prevent us from just executing a single method and then killing the fiber
    while (true)
    {
        if (mCurrentJob != nullptr)
        {
            clearFiberTime();
            mCurrentJob->execute();
            delete mCurrentJob;
            mCurrentJob = nullptr;
        }

        if (mListener != nullptr)
        {
            mListener->onFiberComplete(*this);
        }

        reset();

        if (mListener != nullptr)
        {
            mListener->onFiberReset(*this);
        }

        returnToCaller();
    }
}

void Fiber::setTimeout(const TimeValue& timeout)
{
    if (timeout == mTimeoutAbsolute)
        return;

    if (timeout == 0)
    {
        clearTimeout();
        return;
    }

    mTimeoutAbsolute = timeout;
    if (mTimeoutId != INVALID_TIMER_ID)
    {
        gSelector->updateTimer(mTimeoutId, mTimeoutAbsolute);
    }
    else
    {
        // We pass in the mNamedContext as a sanity check measure, since something has been calling onTimeout after the Fiber was invalid. 
        mTimeoutId = gSelector->scheduleTimerCall(mTimeoutAbsolute, this, &Fiber::onTimeout, mFiberDiagnosticsRing.back(), "Fiber::onTimeout");
    }
}

void Fiber::clearTimeout()
{
    if (mTimeoutId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mTimeoutId);
    }

    mTimeoutAbsolute = 0;
    mTimeoutId = INVALID_TIMER_ID;
}

void Fiber::assignFiberId()
{
    if (mHandle.fiberId != INVALID_FIBER_ID)
    {
        msFiberIdMap->erase(mHandle.fiberId);
    }

    mHandle.fiberId = msNextFiberId++;
    msFiberIdMap->insert(*this);
}

void Fiber::assignFiberGroupId(FiberGroupId id)
{
    if (mHandle.fiberGroupId == id)
        return;

    if (mHandle.fiberGroupId != INVALID_FIBER_GROUP_ID)
    {
        FiberGroupIdListMap::iterator it = msFiberGroupIdMap->find(mHandle.fiberGroupId);
        if (it != msFiberGroupIdMap->end())
        {
            it->second.remove(*this);
            FiberGroupIdListNode::mpNext = FiberGroupIdListNode::mpPrev = nullptr;
            if (it->second.empty())
            {
                msFiberGroupIdMap->erase(mHandle.fiberGroupId); // NOTE: Intentionally erase by key, erase by iterator in hashmaps is not efficient

                //Copy off the set of event handles.  If someone else starts into the group or wakes as a result of us signaling
                //the waiters, we want them independent
                eastl::vector<EventHandle> handles;
                eastl::pair<FiberGroupJoinMap::iterator, FiberGroupJoinMap::iterator> joinRange = msFiberGroupJoinMap->equal_range(mHandle.fiberGroupId);

                while (joinRange.first != joinRange.second)
                {
                    handles.push_back(joinRange.first->second);
                    joinRange.first = msFiberGroupJoinMap->erase(joinRange.first);                        
                }

                //Now signal our events
                for (eastl::vector<EventHandle>::iterator itr = handles.begin(), end = handles.end(); itr != end; ++itr)
                {
                    Fiber::signal(*itr, ERR_OK);
                }
            }
        }
    }

    mHandle.fiberGroupId = id;

    if (mHandle.fiberGroupId != INVALID_FIBER_GROUP_ID)
    {
        (*msFiberGroupIdMap)[mHandle.fiberGroupId].push_back(*this);
    }
}

void Fiber::reset()
{
    gFiberManager->verifyBlockingFiberStackDoesNotExist(mHandle.fiberId);

    //clear if we were cancelled. Note must be uncancelled before releaseResources()
    uncancel();
    setPriority(0);
    clearTimeout();
    clearFiberTime();

    mBlockingSuppressionCount = 0;

    //Must be called in the context of this fiber
    FiberLocalVarBase::resetFiberLocals();

    //Hopefully we're not holding on to anything
    releaseResources();

    mLastRedisLockStartTime = 0;
    mLastRedisLockEndTime = 0;
    mTotalRedisLockWaitTime = 0;
    mTotalRedisLockAttempts = 0;

    //Give us a new fiber id.
    assignFiberGroupId(INVALID_FIBER_GROUP_ID);
    assignFiberId();

    //In case assignFiberGroupId() allowed a parent fiber to switch us to canceled, uncancel again.
    uncancel();

    mCallstackSnapshotAtWait.reset();
    mCallstackSnapshotAtCancellation.reset();
    if (isCanceled())
    {
        BLAZE_ASSERT_LOG(Log::SYSTEM, "Fiber.reset: Reseted fiber is in an invalid canceled state.");
    }

    setNamedContext(UNNAMED_CONTEXT); // Clear named context last because we want to retain it for logging until the end
}

void Fiber::dumpFiberGroupStatus(Fiber::FiberGroupId id)
{
    StringBuilder fiberGroupStatus;
    Fiber::dumpFiberGroupStatus(id, fiberGroupStatus);
    BLAZE_TRACE_LOG(Log::CONTROLLER, fiberGroupStatus.get());
}

void Fiber::dumpFiberGroupStatus(Fiber::FiberGroupId id, StringBuilder& output)
{
    output << "Begin dump of fiber group id=" << id << "\n";
    size_t count = 0;
    FiberGroupIdListMap::iterator it = msFiberGroupIdMap->find(id);
    if (it != msFiberGroupIdMap->end())
    {
        for (FiberGroupIdList::iterator i = it->second.begin(), e = it->second.end(); i != e; ++i, ++count)
        {
            Fiber* current = &static_cast<Fiber&>(*i);
            while (current != nullptr)
            {
                if (current->isAwake())
                {
                    output << "Fiber id=" << current->getHandle().fiberId << ", context=\"" << current->mNamedContext << "\" is awake." << "\n";
                    break;
                }
                else if (current->mCurrentEventId != INVALID_EVENT_ID)
                {
                    output << "Fiber id=" << current->getHandle().fiberId << ", context=\"" << current->mNamedContext << "\" is waiting on event id=" << current->mCurrentEventId << ", event context=" << current->mCurrentEventContext << "\n";
                    break;
                }
                else 
                {
                    //We should be calling someone
                    Fiber* callee = current->getCallee();
                    if (callee != nullptr)
                    {
                        output << "Fiber id=" << current->getHandle().fiberId << ", context=\"" << current->mNamedContext << "\" is calling fiber id=" << callee->getHandle().fiberId << ", context=" << callee->mNamedContext << "\n";
                    }
                    else
                    {
                        EA_FAIL_MSG("Unkown fiber state!");
                        BLAZE_WARN_LOG(Log::CONTROLLER, "Fiber id=" << current->getHandle().fiberId << ", context=\"" << current->mNamedContext << "\" is not awake, not waiting on an event, and not calling anyone.  Fiber is in unknown state.");
                    }
                    current = callee;
                }
            }
        }
    }

    output << "End dump of fiber group id=" << id << ", total executing fiber count = " << count;
}

void Fiber::markRedisLockStartTime()
{
    mLastRedisLockStartTime = TimeValue::getTimeOfDay();
}

void Fiber::markRedisLockEndTime()
{
    mLastRedisLockEndTime = TimeValue::getTimeOfDay();
    mTotalRedisLockWaitTime += (mLastRedisLockEndTime - mLastRedisLockStartTime);
}

void Fiber::incRedisLockAttempts()
{
    ++mTotalRedisLockAttempts;
}

/*** Private Methods *****************************************************************************/

#if defined(EA_PLATFORM_LINUX)
void Fiber::allocateStack(size_t stackSizeBytes, bool useStackProtection)
{
    size_t pageSize = (size_t)sysconf(_SC_PAGESIZE);
    size_t barrierSize = pageSize;

    mRealStackSize = barrierSize + (pageSize * (stackSizeBytes / pageSize));
    if ((stackSizeBytes % pageSize) != 0)
        mRealStackSize += pageSize;

    mRealStackMem = (char8_t*)mmap(0, mRealStackSize, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    EA_ASSERT(mRealStackMem != MAP_FAILED);

    //lock the barrier page to generate an exception if its violated.
    if (useStackProtection)
        mprotect(mRealStackMem, barrierSize, PROT_NONE);

    mFiberContext.stackMem = mRealStackMem + barrierSize;
}

void Fiber::deallocateStack()
{
    munmap(mRealStackMem, mRealStackSize);
}
#endif

void Fiber::onTimeout(Fiber::FiberDiagnostics origFiberDiag)
{
    auto& diag = mFiberDiagnosticsRing.back();

    EA_ASSERT(!isAwake()); //This is pretty much impossible to fail, but who knows

    // This code has been crashing infrequently. 
    // To better indicate a problem, we check if the mNamedContext pointer has changed (since it shouldn't).
    // Worst case, the Fiber was deleted, but has not been reused, in which case the mNamedContext should still reference the original string.
    if (mNamedContext != origFiberDiag.mNamedContext)  // (Pointer comparison, since the pointer shouldn't change)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[Fiber].onTimeout : Fiber named context (" << origFiberDiag.mNamedContext << ") has changed since the Fiber was created.");
        BLAZE_ERR_LOG(Log::SYSTEM, "[Fiber].onTimeout : Fiber new context (" << mNamedContext << ") may be invalid.");    // This line should crash. 
    }


    if (EA_UNLIKELY(mTimeoutId == INVALID_TIMER_ID))
    {
        // This indicates that either the timer was canceled, but had already created a Job for onTimeout. 
        // Since this should be very rare, we log a warning, since it may indicate other issues. 
        BLAZE_WARN_LOG(Log::SYSTEM, "[Fiber].onTimeout : Fiber has already canceled the timer. Fiber id= " << getHandle().fiberId
            << ", Context= " << mNamedContext << ", Caller Context= " << diag.mCallerContext << ", Scheduler Context= " << diag.mSchedulerContext);
        return;
    }

    //Clear the timer id and cancel us.  Leave the timeout for comparison
    mTimeoutId = INVALID_TIMER_ID;

    cancel(ERR_TIMEOUT);
}

void Fiber::releaseResources()
{
    //  each iteration of the while loop will pop the list's front node - but just to be safe, avoid infinite loops
    const uint32_t UNRELEASED_RESOURCES_THRESHOLD = 10000;
    uint32_t releaseCounter = UNRELEASED_RESOURCES_THRESHOLD;
    while (!mResourceList.empty())
    {
        FrameworkResource& resource = mResourceList.back();

        BLAZE_WARN_LOG(Log::SYSTEM, "[Fiber].releaseResources : type=" << resource.getTypeName() << " not released when fiber finished; refcount=" 
            << resource.getRefCount() << ", forcing release now; context=" << mNamedContext);

        //  This really should never happen unless the resource management code inside FrameworkResource breaks.  But if it does
        //  happen, clean up the list entry only.  Let the owning fiber take care of managing the resource.
        if (resource.getOwningFiber() == this)
        {
            //  FrameworkResource::Release removes the member from the fiber's resource list.
            resource.release();      
        }
        else
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[Fiber].releaseResources : type=" << resource.getTypeName() << " ownership mismatch between fiber and resource (owning_fiber="
                << resource.getOwningFiber() << "); finished; context=" << mNamedContext);
            mResourceList.pop_back();
        }
        --releaseCounter;
        if (releaseCounter == 0)
        {
            BLAZE_WARN_LOG(Log::SYSTEM, "[Fiber].releaseResources : " << UNRELEASED_RESOURCES_THRESHOLD << " resources released after finishing fiber.  Indicates a likely bug in the calling fiber.");
            releaseCounter = UNRELEASED_RESOURCES_THRESHOLD;
        }
    }
}

Fiber::BlockingSuppressionAutoPtr::BlockingSuppressionAutoPtr(bool enabled)
    : mEnabled(enabled)
{
    if (mEnabled)
        Fiber::suppressBlocking();
}

Fiber::BlockingSuppressionAutoPtr::~BlockingSuppressionAutoPtr()
{
    if (mEnabled)
        Fiber::unsuppressBlocking();
}


Fiber::CallstackSnapshotAtWaitAutoPtr::CallstackSnapshotAtWaitAutoPtr(bool enabled)
     : mEnabled(enabled)
{
    if (mEnabled)
        Fiber::enableCallstackSnapshotAtWaitOnCurrentFiber();
}

Fiber::CallstackSnapshotAtWaitAutoPtr::~CallstackSnapshotAtWaitAutoPtr()
{
    if (mEnabled)
        Fiber::disableCallstackSnapshotAtWaitOnCurrentFiber();
}

// static
const char8_t* Fiber::StackSizeToString(StackSize ss)
{
    switch (ss)
    {
        case STACK_MICRO:
            return "MICRO";
        case STACK_SMALL:
            return "SMALL";
        case STACK_MEDIUM:
            return "MEDIUM";
        case STACK_LARGE:
            return "LARGE";
        case STACK_HUGE:
            return "HUGE";
        case MAX_STACK_SIZES:
        default:
            break;
    }
    return "UNKNOWN";
}

// static
void Fiber::pauseMainFiberTimingAt(const TimeValue& time)
{
    EA_ASSERT(msCurrentFiber == msMainFiber);
    msMainFiber->mFiberTime += time - msMainFiber->mWakeTimestamp;
}

Fiber::WaitList::WaitList()
{
}

Fiber::WaitList::~WaitList()
{
    EA_ASSERT_MSG(!mDestroyed, "Double delete detected!");
    mDestroyed = true;
    if (mSignaled)
    {
        // NOTE: This means the caller is trying to destroy the WaitList from a 'waiting' fiber woken by WaitList::signal().
        // The 'waiting' fiber needs to yield control back to the 'signaling' fiber, to ensure the latter can complete its iteration 
        // loop in signal() before the 'waiting' fiber can safely resume and destroy the list.
        Fiber::yield();
    }
    EA_ASSERT_MSG(!mSignaled, "Signaled flag must be cleared!");
    EA_ASSERT_MSG(mEventHandleList.empty(), "Waitlist must be empty!");
}

int32_t Fiber::WaitList::getCount() const
{
    return (int32_t)mEventHandleList.size();
}

/**
 * Wait until this waiter or the entire list of waiters is signalled.
 */
Blaze::BlazeRpcError Fiber::WaitList::wait(const char8_t* context, EA::TDF::TimeValue absoluteTimeout /*= EA::TDF::TimeValue()*/)
{
    if (mDestroyed)
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[WaitList].wait: " << context << " attempted to wait on a destroyed list. Indicates a likely bug.");
        return ERR_SYSTEM;
    }
    auto eventHandle = Fiber::getNextEventHandle();
    mEventHandleList.push_back(eventHandle);
    auto result = Fiber::wait(eventHandle, context, absoluteTimeout);
    for (auto handleItr = mEventHandleList.begin(), handleEnd = mEventHandleList.end(); handleItr != handleEnd; ++handleItr)
    {
        if (handleItr->eventId == eventHandle.eventId)
        {
            mEventHandleList.erase(handleItr);
            break;
        }
    }
    if (mEventHandleList.empty())
        mEventHandleList.shrink_to_fit();
    return result;
}

/**
 * Unilaterally empty the wait list signaling all waiters.
 */
void Fiber::WaitList::signal(BlazeRpcError waitResult)
{
    if (mSignaled || mEventHandleList.empty())
        return; // already signaled, or there is nothing to signal
    mSignaled = true;
    do
    {
        auto eventHandle = mEventHandleList.front();
        mEventHandleList.pop_front();
        Fiber::signal(eventHandle, waitResult);
    }
    while (!mEventHandleList.empty());
    mEventHandleList.shrink_to_fit();
    mSignaled = false;
}

} // Blaze

