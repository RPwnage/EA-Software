/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_TIMER_H
#define BLAZE_TIMER_H

/*** Include files *******************************************************************************/



/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

struct Timer
{
public:
    Timer() { start(); }
    Timer(const Timer& t) : mStartTime(t.mStartTime), mStopTime(t.mStopTime) {}
    Timer(const EA::TDF::TimeValue& startTime, const EA::TDF::TimeValue& stopTime) : mStartTime(startTime), mStopTime(stopTime) {}

    void start() { mStartTime = EA::TDF::TimeValue::getTimeOfDay(); mStopTime = 0; }
    void stop() { mStopTime = EA::TDF::TimeValue::getTimeOfDay(); }
    EA::TDF::TimeValue getInterval() const { return (mStopTime == 0 ? EA::TDF::TimeValue::getTimeOfDay() : mStopTime) - mStartTime; }

private:
    EA::TDF::TimeValue mStartTime;
    EA::TDF::TimeValue mStopTime;
};

struct TimerGraph
{
public:
    TimerGraph(const char8_t* name);

    void mark(const char8_t* timeSegmentLabel);
    void done();

    void generateReport(StringBuilder& report);

private:
    TimeValue mOverallBeginTime;
    TimeValue mOverallEndTime;
    struct TimeSegment
    {
        TimeSegment() : counter(0) {}
        TimeValue beginTime;
        TimeValue endTime;
        TimeValue totalTime;
        int64_t counter;
    };
    typedef eastl::hash_map<eastl::string, TimeSegment> TimeSegmentByStringMap;
    TimeSegmentByStringMap mTimeSegmentByNameMap;

    TimeSegment* mCurrentTimeSegment;
    TimeValue mCurrentMarkTime;

    struct SortByTotalTimeDecending
    {
        bool operator()(const eastl::pair<const char8_t*, const TimeSegment*>& a, const eastl::pair<const char8_t*, const TimeSegment*>& b) const
        {
            return (a.second->totalTime > b.second->totalTime);
        }
    };
};

} // Blaze

#endif // BLAZE_TIMER_H
