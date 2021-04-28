/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Counter

    This class is used to create over interval counters.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/util/counter.h"
#include "framework/util/shared/blazestring.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

Counter::Counter(const char* counterName)
{
    mCount = 0;
    blaze_strnzcpy(mCounterName, counterName, BLAZE_COUNTER_NAME_SIZE);
}

// add to current count value
void Counter::add(int32_t val)
{
    mCount = (uint32_t)((int32_t)mCount + val);
}

void Counter::setCount(uint32_t val)
{
    mCount = val;
}

// returns count in last completed interval
uint32_t Counter::getCount() const
{
    return mCount;
}

const char8_t* Counter::getName() const
{
    return mCounterName;
}

void Counter::getCountAsString(char8_t* buffer, size_t len) const
{
    blaze_snzprintf(buffer, len, "%" PRIu32, getCount());
}

Interval::Interval(const TimeValue& intervalStart, const TimeValue& interval)
    : mInterval(interval), mIntervalStart(intervalStart), mIntervalCount(1)
{
}


void CounterEvent::addCounter(Counter *counter)
{
    if (counter != nullptr)
        mCounters.push_back(counter);
}

void CounterEvent::add(int32_t count)
{
    for (CounterVector::iterator it = mCounters.begin(); it != mCounters.end(); it++)
        (*it)->add(count);
}


void Interval::checkInterval(const TimeValue &current)
{
    TimeValue intervalEnd = mIntervalStart + mInterval;
    
    if (current > intervalEnd)
    {
        // update interval start
        TimeValue diff = intervalEnd - current;
        if (diff > mInterval)
        {
            uint32_t intervalCount = (uint32_t)(diff / mInterval + 1);
            mIntervalStart += intervalCount * mInterval;
            mIntervalCount += intervalCount;
        }
        else
        {
            mIntervalStart = intervalEnd;
            mIntervalCount++;
        }
    }
}

Interval::MomentType Interval::getMoment(TimeValue &moment)
{
    TimeValue current = TimeValue::getTimeOfDay();
    checkInterval(current);

    MomentType momentType = MOMENT_IN_PAST;
    
    if (moment < mIntervalStart - mInterval)
        momentType = MOMENT_IN_PAST;
    else if (moment < mIntervalStart)
        momentType = MOMENT_IN_LAST_INTERVAL;
    else if (moment < mIntervalStart + mInterval)
        momentType = MOMENT_IN_CURRENT_INTERVAL;
    else
        momentType = MOMENT_IN_FUTURE;
    
    moment = current;
    
    return momentType;    
}

    
// returns the time of the current interval start
TimeValue Interval::getIntervalStart()
{
    TimeValue current = TimeValue::getTimeOfDay();
    checkInterval(current);
    
    return mIntervalStart;
}

// returns the time when current interval ends
TimeValue Interval::getIntervalEnd()
{
    TimeValue current = TimeValue::getTimeOfDay();
    checkInterval(current);
    
    return mIntervalStart + mInterval;
}

IntervalCounter::IntervalCounter(const char8_t *counterName, Interval& interval, IntervalCounterType type)
    : Counter(counterName), mCounterType(type), mInterval(interval), mLastCount(0), mLastTick(0)
{
}
    
void IntervalCounter::checkInterval()
{
    Interval::MomentType moment = mInterval.getMoment(mLastTick);
        
    if (moment == Interval::MOMENT_IN_PAST)
    {
        mCount = 0;
        mLastCount = 0;
    } 
    else if (moment == Interval::MOMENT_IN_LAST_INTERVAL)
    {
        mLastCount = mCount;
        mCount = 0;
    }
}
    
// returns current count 
uint32_t IntervalCounter::getCount() const
{
    return (mCounterType == INTERVAL_COUNTER_FULL_INTERVAL) ? mLastCount : mCount;
}

// add to current count value
void IntervalCounter::add(int32_t val)
{
    checkInterval();
    Counter::add(val);
}

void IntervalCounter::setCount(uint32_t val)
{
    checkInterval();
    mLastCount = val;
    mCount = 0;
}

} // Blaze

