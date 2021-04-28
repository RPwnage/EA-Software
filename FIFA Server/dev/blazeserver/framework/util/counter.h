/*************************************************************************************************/
/*!
    \file

    Time Interval Counter class.


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_COUNTER_H
#define BLAZE_COUNTER_H

/*** Include files *******************************************************************************/

#include "EABase/eabase.h"
#include "EATDF/time.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

const size_t BLAZE_COUNTER_NAME_SIZE = 32;

/*************************************************************************************************/
/*!
    \class Counter

    Simple integer counter with name used for health check and performance counters.
    Base class for other counters such as interval counters.

*/
/*************************************************************************************************/

class Counter
{
NON_COPYABLE(Counter)
public:
    Counter(const char* counterName);
    virtual ~Counter() { }
    
    const char8_t* getName() const;
    
    // returns count in last completed interval
    virtual uint32_t getCount() const;

    // sets count
    virtual void setCount(uint32_t val);
    
    // fills buffer with count converted to string
    virtual void getCountAsString(char8_t* buffer, size_t len) const;
    
    // add to current count value
    virtual void add(int32_t val);
    
protected:
    uint32_t mCount;
    char8_t mCounterName[BLAZE_COUNTER_NAME_SIZE];
};

/*************************************************************************************************/
/*!
    \class CounterEvent

    Groups counters triggered for the same event

*/
/*************************************************************************************************/

class CounterEvent
{
public:
    CounterEvent() { }
    virtual ~CounterEvent() { };

    void addCounter(Counter *counter);
    void add(int32_t count);
private:
    typedef eastl::vector<Counter*> CounterVector;
    CounterVector mCounters;
};


/*************************************************************************************************/
/*!
    \class Interval

    Defines intervals used by interval counters. Current interval is adjusted in each method call.

*/
/*************************************************************************************************/

class Interval
{
public:    
    Interval(const EA::TDF::TimeValue& intervalStart, const EA::TDF::TimeValue& interval);
    virtual ~Interval() {}
    
    enum MomentType {
        MOMENT_IN_PAST = 0,
        MOMENT_IN_LAST_INTERVAL,
        MOMENT_IN_CURRENT_INTERVAL,
        MOMENT_IN_FUTURE
    };
    
    // returns the moment classification as it compares to 
    // current interval and updates moment to current time
    MomentType getMoment(EA::TDF::TimeValue &moment);

    // returns the time of the end of last completed interval
    // which is also a start of the new interval
    EA::TDF::TimeValue getIntervalStart();

    // returns the time when current interval ends
    EA::TDF::TimeValue getIntervalEnd();
    
    // returns the count of intervals elapsed since
    // this Intrval was created
    inline uint32_t getIntervalCount() const { return mIntervalCount; }
    
protected:
    EA::TDF::TimeValue mInterval;    
    EA::TDF::TimeValue mIntervalStart;  
    uint32_t mIntervalCount;
    
    inline void checkInterval(const EA::TDF::TimeValue &current);
};

/*************************************************************************************************/
/*!
    \class IntervalCounters

    Keeps counts in last completed and current intervals. Use this class in performance counters.

*/
/*************************************************************************************************/

class IntervalCounter : public Counter
{   
public:
    enum IntervalCounterType
    {
        INTERVAL_COUNTER_FULL_INTERVAL = 0,  // getCount returns value of last completed interval
        INTERVAL_COUNTER_CURRENT             // getCount returns current value
    };
    
    // keeps pointer reference to interval
    IntervalCounter(
        const char8_t *counterName, 
        Interval& interval, 
        IntervalCounterType type = INTERVAL_COUNTER_FULL_INTERVAL);
        
    ~IntervalCounter() override { }
    
    // returns count in last completed interval or current count depending on interval
    // counter type value for this interval counter
    uint32_t getCount() const override;
    
    // sets count
    void setCount(uint32_t val) override;
    
    // add to current count value
    void add(int32_t val) override;
    
private:

    inline void checkInterval();

    IntervalCounterType mCounterType;
    Interval& mInterval;    
    uint32_t mLastCount;
    EA::TDF::TimeValue mLastTick;
};

} // namespace Blaze

#endif // BLAZE_COUNTER_H

