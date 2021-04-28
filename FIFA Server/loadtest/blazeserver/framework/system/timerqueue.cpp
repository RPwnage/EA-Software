/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/system/timerqueue.h"
#include "framework/system/job.h"
#include "framework/system/fibermanager.h"
#include "EASTL/utility.h"
#include "EASTL/heap.h"


namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

// Internal helper class - member are intentionally public to keep this simple
class TimerNode : public TimerMapNode
{
public:
    TimerNode() : mJob(nullptr) { }

    ~TimerNode()
    {
        delete mJob;
    }

    TimerNode(Job* job, const void* associatedObject, const Fiber::CreateParams& fiberParams)
        : mJob(job),
          mAssociatedObject(associatedObject),
          mFiberParams(fiberParams)
    {

    }

    TimerNode(Fiber::EventHandle& eventHandle, const void* associatedObject)
        : mJob(nullptr),
          mEventHandle(eventHandle),
          mAssociatedObject(associatedObject)
    {

    }

    // Public members
    Job* mJob;
    Fiber::EventHandle mEventHandle;
    const void* mAssociatedObject;
    Fiber::CreateParams mFiberParams;
    TimerId mTimerId;
};

const uint32_t TimerQueue::DISCARD_NODE_LOAD_FACTOR = 2;
const uint32_t TimerQueue::DISCARD_NODE_THRESHOLD = 16;

/*** Public Methods ******************************************************************************/
TimerQueue::TimerQueue(const eastl::string& name) : mDiscardPendingNodeCount(0), mNextTimerId(1), mName(name)
{
    mNodePool.Init(128, nullptr, SIZE_MAX, &TimerQueue::nodeAllocFunc, &TimerQueue::nodeFreeFunc, this);
}

TimerQueue::~TimerQueue()
{
    removeAllTimers();
}

void TimerQueue::removeAllTimers()
{
    for (PriorityQueue::iterator i = mPriorityQueue.begin(), e = mPriorityQueue.end(); i != e; ++i)
    {
        cleanupNode(*i->node);
    }
    
    mPriorityQueue.clear();
    mTimerNodesByTimerId.clear();
}

TimerId TimerQueue::scheduleFiberSleep(const TimeValue& timeoutAbsolute, Fiber::EventHandle& eventHandle)
{
    TimerId timerId = INVALID_TIMER_ID;

    TimerNode* newNode = new (mNodePool.Malloc()) TimerNode(eventHandle, nullptr);
    if (newNode != nullptr)
    {
        timerId = getTimerId();
        newNode->mTimerId = timerId;
        insertNode(*newNode, timeoutAbsolute.getMicroSeconds());
        mTimerNodesByTimerId.insert(*newNode);
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[TimerQueue::scheduleFiberSleep]: Failed to allocate timer for queue " << mName.c_str() << "." );
    }

    return timerId;
}


TimerId TimerQueue::schedule(const TimeValue& timeoutAbsolute, Job *job, const void* associatedObject, const Fiber::CreateParams& params)
{
    TimerId timerId = INVALID_TIMER_ID;
    TimerNode* newNode = new(mNodePool.Malloc()) TimerNode(job, associatedObject, params);
    if (newNode != nullptr)
    {
        timerId = getTimerId();
        newNode->mTimerId = timerId;
        insertNode(*newNode, timeoutAbsolute.getMicroSeconds());
        mTimerNodesByTimerId.insert(*newNode);
    }
    else
    {
        BLAZE_ERR_LOG(Log::SYSTEM, "[TimerQueue::schedule]: Failed to allocate timer for queue " << mName.c_str() << "." );
    }
    
    return timerId;
}

void TimerQueue::update(TimerId timerId, const TimeValue& newTime)
{
    TimerNodesByTimerId::iterator it = mTimerNodesByTimerId.find(timerId);
    if (it != mTimerNodesByTimerId.end())
    {
        TimerNode& node = (TimerNode&) (*it);
        TimerNode* newNode = new(mNodePool.Malloc()) TimerNode(node);
        if (newNode != nullptr)
        {
            // newNode now owns job or handle
            node.mJob = nullptr;
            node.mEventHandle.reset();
            mTimerNodesByTimerId.erase(it);
            insertNode(*newNode, newTime.getMicroSeconds());
            mTimerNodesByTimerId.insert(*newNode);
            discardNode(node);
        }
        else
        {
            BLAZE_ERR_LOG(Log::SYSTEM, "[TimerQueue::update]: Failed to allocate timer for queue " << mName.c_str() << "." );
        }
    }
}

bool TimerQueue::cancel(TimerId timerId)
{
    TimerNodesByTimerId::iterator it = mTimerNodesByTimerId.find(timerId);
    if (it != mTimerNodesByTimerId.end())
    {
        TimerNode& node = (TimerNode&) (*it);
        mTimerNodesByTimerId.erase(it);
        //Cancelling a fiber sleep will wake it up.
        if (node.mEventHandle.isValid())
        {
            Fiber::signal(node.mEventHandle, ERR_OK);
        }
        else if (node.mJob != nullptr)
        {
            delete node.mJob;
            node.mJob = nullptr;
        }
        discardNode(node);
        return true;
    }
    return false;
}

uint32_t TimerQueue::cancel(const void* associatedObject)
{
    typedef eastl::vector<Fiber::EventHandle> FiberEventHandleList;
    typedef eastl::vector<TimerNode*> TimerNodeList;
    typedef eastl::vector<Job*> JobList;
    FiberEventHandleList eventHandles;
    TimerNodeList timerNodeList;
    JobList jobList;

    for (PriorityQueue::iterator i = mPriorityQueue.begin(), e = mPriorityQueue.end(); i != e; ++i)
    {
        TimerNode& node = *i->node;
        if (node.mTimerId != INVALID_TIMER_ID && node.mAssociatedObject == associatedObject)
        {
            mTimerNodesByTimerId.erase(node.mTimerId);
            if (node.mEventHandle.isValid())
            {
                eventHandles.push_back(node.mEventHandle);
            }
            else if (node.mJob != nullptr)
            {
                jobList.push_back(node.mJob);
                node.mJob = nullptr;
            }
            timerNodeList.push_back(&node);
        }
    }
    
    // discard nodes outside priority queue iteration (because discardNode() may mutate the queue)
    for (TimerNodeList::const_iterator i = timerNodeList.begin(), e = timerNodeList.end(); i != e; ++i)
        discardNode(**i);
    
    // signal all canceled events outside the priority queue iteration
    for (FiberEventHandleList::const_iterator i = eventHandles.begin(), e = eventHandles.end(); i != e; ++i)
        Fiber::signal(*i, ERR_OK);
    
    // delete all canceled jobs outside the priority queue iteration
    for (JobList::const_iterator i = jobList.begin(), e = jobList.end(); i != e; ++i)
        delete *i;
        
    return (uint32_t) timerNodeList.size();
}

TimeValue TimerQueue::getNextExpiry() const
{
    return TimeValue(!mPriorityQueue.empty() ? mPriorityQueue.front().time : -1);
}

TimerNode* TimerQueue::popExpiredNode(const TimeValue& currentTime)
{
    TimerNode* node = nullptr;
    while (!mPriorityQueue.empty())
    {
        const int64_t time = currentTime.getMicroSeconds();
        PriorityNode& priNode = mPriorityQueue.front();
        if (priNode.time <= time)
        {
            TimerNode* timerNode = priNode.node;
            eastl::pop_heap(mPriorityQueue.begin(), mPriorityQueue.end());
            mPriorityQueue.pop_back();
            if (timerNode->mTimerId != INVALID_TIMER_ID)
            {
                // the node is 'live' erase it and return
                mTimerNodesByTimerId.erase(timerNode->mTimerId);
                node = timerNode;
                break;
            }
            else
            {
                cleanupNode(*timerNode);
                --mDiscardPendingNodeCount;
            }
        }
        else
            break;
    }
    return node;
}

void TimerQueue::processNode(TimerNode& node) const
{
    if (node.mEventHandle.isValid())
    {
        Fiber::signal(node.mEventHandle, ERR_OK);
    }
    else if (gFiberManager != nullptr)
    {   
        // The return value here indicates that either the job was not started, or that the job completed. Therefore it cannot be used to skip the mJob = nullptr.
        gFiberManager->executeJob(*node.mJob, node.mFiberParams);
        node.mJob = nullptr;   // job owned or freed by fiber. (We set this nullptr here to avoid a double delete by cleanupNode)
    }
    else
    {
        node.mJob->execute();
        //all done with the job, we can clean up here (technically, cleanupNode would do this too)
        delete node.mJob;
        node.mJob = nullptr;
    }
}

void TimerQueue::cleanupNode(TimerNode& node)
{
    node.~TimerNode();
    mNodePool.Free(&node);
}

// Returns the amount of time until the next event expires
TimeValue TimerQueue::expire(const TimeValue& currentTime, const TimeValue& maxProcessingTime, uint32_t& expired)
{
    TimerNode* node;
    uint32_t count = 0;
    
    while ((node = popExpiredNode(currentTime)) != nullptr)
    {
        processNode(*node);
        cleanupNode(*node);
        ++count;

        if (TimeValue::getTimeOfDay() - currentTime > maxProcessingTime)
            break;
    }
    
    expired = count;
    
    return getNextExpiry();
}

void TimerQueue::insertNode(TimerNode& node, int64_t timeout)
{
    PriorityNode& priorityNode = mPriorityQueue.push_back();
    priorityNode.time = timeout;
    priorityNode.node = &node;
    eastl::push_heap(mPriorityQueue.begin(), mPriorityQueue.end());
}

void TimerQueue::discardNode(TimerNode& node)
{
    node.mTimerId = INVALID_TIMER_ID;
    
    if (mPriorityQueue.back().node == &node)
    {
        mPriorityQueue.pop_back();
        cleanupNode(node);
        return;
    }
    
    if (mPriorityQueue.front().node == &node)
    {
        eastl::pop_heap(mPriorityQueue.begin(), mPriorityQueue.end());
        mPriorityQueue.pop_back();
        cleanupNode(node);
        return;
    }

    ++mDiscardPendingNodeCount;

    uint32_t queueSize = (uint32_t) mPriorityQueue.size();
    if ((mDiscardPendingNodeCount * DISCARD_NODE_LOAD_FACTOR < queueSize) || 
        (queueSize < DISCARD_NODE_THRESHOLD))
    {
        return;
    }

    // NOTE: We only trigger a compacting node collection in the event when
    // the ratio of discarded nodes exceeds 1/2 of total nodes as well as
    // the minimum threshold. This results in amortized O(log(N)) cleaning.
    sweepNodes();
}

void TimerQueue::sweepNodes()
{
    PriorityQueue newPriorityQueue;
    newPriorityQueue.reserve(mPriorityQueue.size());

    // compact priority nodes with valid timers into the new heap and clean nodes with invalid timers
    for (PriorityQueue::iterator i = mPriorityQueue.begin(), e = mPriorityQueue.end(); i != e; ++i)
    {
        if (i->node->mTimerId == INVALID_TIMER_ID)
        {
            cleanupNode(*i->node);
            --mDiscardPendingNodeCount;
        }
        else
        {
            PriorityNode& priorityNode = newPriorityQueue.push_back();
            priorityNode.time = i->time;
            priorityNode.node = i->node;
            eastl::push_heap(newPriorityQueue.begin(), newPriorityQueue.end());
        }
    }

    // transfer the newly compacted heap into mPriorityQueue
    newPriorityQueue.swap(mPriorityQueue);
}

void* TimerQueue::nodeAllocFunc(size_t size, void* _this)
{
    return BLAZE_ALLOC_MGID(size, MEM_GROUP_FRAMEWORK_DEFAULT, ((TimerQueue *)_this)->mName.c_str());
}

void TimerQueue::nodeFreeFunc(void* mem, void*)
{
    BLAZE_FREE(mem);
}


/*** Private Methods *****************************************************************************/

} // Blaze

namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::TimerMapNode, ::TimerId>
    {
        const ::TimerId operator()(const Blaze::TimerMapNode& x) const
        { 
            return static_cast<const Blaze::TimerNode &>(x).mTimerId;
        }
    };
}

