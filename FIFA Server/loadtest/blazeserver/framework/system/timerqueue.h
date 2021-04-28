/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_TIMERQUEUE_H
#define BLAZE_TIMERQUEUE_H

/*** Include files *******************************************************************************/
#include "EASTL/vector.h"
#include "EASTL/hash_map.h"
#include "PPMalloc/EAFixedAllocator.h"
#include "EATDF/time.h"
#include "framework/callback.h"
#include "framework/system/fiber.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

#define INVALID_TIMER_ID 0

class TimerNode;
class Job;

struct TimerMapNode : public eastl::intrusive_hash_node {};

class TimerQueue
{
    NON_COPYABLE(TimerQueue);

public:
    TimerQueue(const eastl::string& name);
    ~TimerQueue(); 

    TimerId scheduleFiberSleep(const EA::TDF::TimeValue& timeoutAbsolute, Fiber::EventHandle& eventHandle);
    TimerId schedule(const EA::TDF::TimeValue& timeoutAbsolute, Job* job, const void* associatedObject, const Fiber::CreateParams& params);

    void update(TimerId id, const EA::TDF::TimeValue& newTime);
    bool cancel(TimerId timerId);
    uint32_t cancel(const void* associatedObject);
    EA::TDF::TimeValue expire(const EA::TDF::TimeValue& currentTime, const EA::TDF::TimeValue& maxProcessingTime, uint32_t& expired);
    EA::TDF::TimeValue getNextExpiry() const;
    TimerNode* popExpiredNode(const EA::TDF::TimeValue& currentTime);
    void processNode(TimerNode& node) const;
    void cleanupNode(TimerNode& node);
    void removeAllTimers();
    size_t size() const {return mPriorityQueue.size();}

private:
    struct PriorityNode
    {
        int64_t time;
        TimerNode* node;
        // comparator is intentionally reversed because heap sorts largest to smallest
        bool operator < (const PriorityNode& other) { return other.time < time; }
    };

    typedef eastl::intrusive_hash_map<TimerId, TimerMapNode, 4099> TimerNodesByTimerId;
    typedef eastl::vector<PriorityNode> PriorityQueue;
    typedef EA::Allocator::FixedAllocator<TimerNode> TimerNodePool;
    
    TimerNodePool mNodePool;
    PriorityQueue mPriorityQueue;
    TimerNodesByTimerId mTimerNodesByTimerId;
    int32_t mDiscardPendingNodeCount;
    TimerId mNextTimerId;
    eastl::string mName;

    static const uint32_t DISCARD_NODE_LOAD_FACTOR;
    static const uint32_t DISCARD_NODE_THRESHOLD;

    TimerId getTimerId();
    void insertNode(TimerNode& node, int64_t timeout);
    void discardNode(TimerNode& node);
    void sweepNodes();
    static void* nodeAllocFunc(size_t size, void*);
    static void nodeFreeFunc(void* mem, void*);
};

inline TimerId TimerQueue::getTimerId()
{
    TimerId timerId = mNextTimerId++;
    if (timerId != INVALID_TIMER_ID)
        return timerId;

    // Update the timer id again so that we don't generate two timerIds with the same value (1).
    timerId = mNextTimerId++;
    return timerId;
}

} // Blaze

#endif // BLAZE_TIMERQUEUE_H
