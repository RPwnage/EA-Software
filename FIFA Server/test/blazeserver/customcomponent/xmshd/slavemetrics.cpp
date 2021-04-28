/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.cpp

    \Description
        Health metrics helper for XmsHd slaves

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2013.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/slavemetrics.cpp#1 $

*/
/*************************************************************************************************H*/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "xmshd/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace XmsHd
{

/*** Static C-Style Methods **********************************************************************/

/*** Public Methods ******************************************************************************/

//! Constructor. Allocates per-type counters and sets all data to default values.
SlaveMetrics::SlaveMetrics() :
    mOneDayInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_DAY),
    mOneHourInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_HOUR),
    mOneMinuteInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_MINUTE),
	mTotalPublishDataCalls("TotalPublishDataWin"),
	mPublishDataCallsPerMinute("LastMinutePublishDataWin", mOneMinuteInterval),
	mPublishDataCallsPerHour("LastHourPublishDataWin", mOneHourInterval),
    mCounterList()
{
	mCounterList.push_back(&mTotalPublishDataCalls);
	mCounterList.push_back(&mPublishDataCallsPerMinute);
	mCounterList.push_back(&mPublishDataCallsPerHour);

	mPublishData.addCounter(&mTotalPublishDataCalls);
	mPublishData.addCounter(&mPublishDataCallsPerMinute);
	mPublishData.addCounter(&mPublishDataCallsPerHour);
}

//! Destructor. Cleans up allocated memory.
SlaveMetrics::~SlaveMetrics()
{
}

//! Reports all metrics to the specified status object.
void SlaveMetrics::report(Blaze::ComponentStatus* status) const
{
    Blaze::ComponentStatus::InfoMap &statusMap = status->getInfo();

    CounterList::const_iterator iterator = mCounterList.begin();
    CounterList::const_iterator end = mCounterList.end();
    while (iterator != end)
    {
        Counter *counter = *iterator;
        const char8_t *strKey = counter->getName();
        char8_t tempValueBuffer[64];
        counter->getCountAsString(tempValueBuffer, sizeof(tempValueBuffer));
        statusMap[strKey] = tempValueBuffer;

        ++iterator;
    }
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

} // namespace XmsHd
} // namespace Blaze
