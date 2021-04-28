/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_TIMERSET_H
#define BLAZE_TIMERSET_H

/*** Include files *******************************************************************************/
#include "EASTL/vector.h"
#include "EASTL/hash_map.h"
#include "framework/callback.h"
#include "framework/connection/selector.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{


/**
 *  /class TimerSet
 *  
 *  This class implements the ability to schedule/cancel/resume standard Blaze timers by using a 
 *  templated TimeoutParam argument as an index. This is useful when you need to schedule a timer
 *  for a given object but don't want to modify the object to store a timer id.
 *  It is also exceedingly useful when managing a bunch of timers associated with persistent
 *  objects (e.g: stored in Redis) which survive server restart. By making use of 
 *  queueTimer()/scheduleQueuedTimers() it is possible to cheaply re-schedule the necessary timers
 *  during server instance startup without having to store the _transient_ TimerIds inside the 
 *  persistent objects themselves.
 *
*/

template<typename P>
class TimerSet
{
    NON_COPYABLE(TimerSet);
public:
    typedef P TimeoutParam;
    typedef Functor1<TimeoutParam> TimerCb;
    
    TimerSet(const TimerCb& cb, const char8_t* timerContext);
    ~TimerSet();
    bool scheduleTimer(const TimeoutParam& param, const EA::TDF::TimeValue& timeout);
    bool cancelTimer(const TimeoutParam& param);
    void queueTimer(const TimeoutParam& param, const EA::TDF::TimeValue& timeout);
    void scheduleQueuedTimers();
    void cancelScheduledTimers();
    
private:
    void handleTimer(TimeoutParam param);
    
    typedef eastl::pair<TimeoutParam, EA::TDF::TimeValue> TimeoutPair;
    typedef eastl::vector<TimeoutPair> QueuedTimerList;
    typedef eastl::hash_map<TimeoutParam, TimerId> TimerIdByParamMap;
    
    QueuedTimerList mQueuedTimerList;
    TimerIdByParamMap mTimerIdMap;
    TimerCb mTimerCallback;
    const char8_t* mTimerContext;
};

template<typename P>
TimerSet<P>::TimerSet(const TimerCb& callback, const char8_t* timerContext) : mTimerIdMap(BlazeStlAllocator("TimerSet::mTimerIdMap"))
{
    EA_ASSERT_MSG(callback.isValid(), "Invalid callback");
    EA_ASSERT_MSG(timerContext != nullptr, "Invalid timer context");
    mTimerCallback = callback;
    mTimerContext = timerContext;
}

template<typename P>
TimerSet<P>::~TimerSet()
{
    EA_ASSERT_MSG(mTimerIdMap.empty(), "Resumable timers must be explicitly canceled before dtor executes");
}

template<typename P>
bool TimerSet<P>::scheduleTimer(const TimeoutParam& param, const EA::TDF::TimeValue& timeout)
{
    typename TimerIdByParamMap::insert_return_type ret = mTimerIdMap.insert(param);
    if (ret.second)
    {
        TimerId timerId = gSelector->scheduleFiberTimerCall(timeout, this, &TimerSet<P>::handleTimer, param, mTimerContext);
        if (timerId != INVALID_TIMER_ID)
        {
            ret.first->second = timerId;
        }
        else
        {
            mTimerIdMap.erase(param);
            ret.second = false;
        }
    }
    return ret.second;
}

template<typename P>
bool TimerSet<P>::cancelTimer(const TimeoutParam& param)
{
    bool result = true;
    typename TimerIdByParamMap::const_iterator it = mTimerIdMap.find(param);
    if (it != mTimerIdMap.end())
    {
        result = gSelector->cancelTimer(it->second);
        mTimerIdMap.erase(param);
    }
    return result;
}

template<typename P>
void TimerSet<P>::queueTimer(const TimeoutParam& param, const EA::TDF::TimeValue& timeout)
{
    TimeoutPair& p = mQueuedTimerList.push_back();
    p.first = param;
    p.second = timeout;
}

template<typename P>
void TimerSet<P>::scheduleQueuedTimers()
{
    for (typename QueuedTimerList::const_iterator i = mQueuedTimerList.begin(), e = mQueuedTimerList.end(); i != e; ++i)
    {
        scheduleTimer(i->first, i->second);
    }
    mQueuedTimerList.set_capacity(0); // clear out queued timers
}

template<typename P>
void TimerSet<P>::cancelScheduledTimers()
{
    for (typename TimerIdByParamMap::const_iterator i = mTimerIdMap.begin(), e = mTimerIdMap.end(); i != e; ++i)
    {
        gSelector->cancelTimer(i->second);
    }
    mTimerIdMap.clear();
}

template<typename P>
void TimerSet<P>::handleTimer(TimeoutParam param)
{
    typename TimerIdByParamMap::const_iterator it = mTimerIdMap.find(param);
    if (it != mTimerIdMap.end())
    {
        mTimerIdMap.erase(param);
        mTimerCallback(param);
    }
}


} // Blaze

#endif // BLAZE_TIMERSET_H
