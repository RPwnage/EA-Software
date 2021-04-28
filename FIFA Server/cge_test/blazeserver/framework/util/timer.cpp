/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/util/timer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

TimerGraph::TimerGraph(const char8_t* name)
    : mCurrentTimeSegment(nullptr)
{        
}

void TimerGraph::mark(const char8_t* timeSegmentLabel)
{
    TimeValue currentTime = TimeValue::getTimeOfDay();
    if (mOverallBeginTime == 0)
        mOverallBeginTime = currentTime;
    TimeSegmentByStringMap::iterator found = mTimeSegmentByNameMap.find_as(timeSegmentLabel);
    if (found == mTimeSegmentByNameMap.end())
        found = mTimeSegmentByNameMap.insert(timeSegmentLabel).first;
    TimeSegment& newTimeSegment = found->second;
    if (newTimeSegment.beginTime == 0)
        newTimeSegment.beginTime = currentTime;
    if (mCurrentTimeSegment != nullptr)
    {
        mCurrentTimeSegment->endTime = currentTime;
        mCurrentTimeSegment->totalTime += currentTime - mCurrentMarkTime;
    }
    ++newTimeSegment.counter;
    mCurrentTimeSegment = &newTimeSegment;
    mCurrentMarkTime = currentTime;
}

void TimerGraph::done()
{
    TimeValue currentTime = TimeValue::getTimeOfDay();
    if (mOverallBeginTime == 0)
        mOverallBeginTime = currentTime;
    if (mOverallEndTime == 0)
        mOverallEndTime = currentTime;
    if (mCurrentTimeSegment != nullptr)
    {
        mCurrentTimeSegment->endTime = currentTime;
        mCurrentTimeSegment->totalTime += currentTime - mCurrentMarkTime;
    }
    mCurrentTimeSegment = nullptr;
    mCurrentMarkTime = currentTime;
}

void TimerGraph::generateReport(StringBuilder& report)
{
    typedef eastl::vector<eastl::pair<const char8_t*, const TimeSegment*>> TimeSegmentList;
    TimeSegmentList times;
    for (auto& i : mTimeSegmentByNameMap)
        times.push_back(eastl::pair<const char8_t*, const TimeSegment*>(i.first.c_str(), &i.second));
    eastl::sort(times.begin(), times.end(), SortByTotalTimeDecending());

    for (auto i : times)
    {
        report << i.second->totalTime.getMicroSeconds() << " : " << i.first << "\n";
    }
}

} // Blaze
