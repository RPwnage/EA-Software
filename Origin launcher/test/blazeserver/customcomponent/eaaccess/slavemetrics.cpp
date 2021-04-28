/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.cpp

    \Description
        Health metrics helper for EaAccess slaves

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2013.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2015/Gen4/DEV/blazeserver/13.x/customcomponent/eaaccess/slavemetrics.cpp#1 $

*/
/*************************************************************************************************H*/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "eaaccess/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace EaAccess
{

/*** Static C-Style Methods **********************************************************************/

/*** Public Methods ******************************************************************************/

//! Constructor. Allocates per-type counters and sets all data to default values.
SlaveMetrics::SlaveMetrics() :
    mOneDayInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_DAY),
    mOneHourInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_HOUR),
    mOneMinuteInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_MINUTE),
	mTotalGetEaAccessSubInfoCalls("TotalGetEaAccessSubInfoWin"),
	mGetEaAccessSubInfoCallsPerMinute("LastMinuteGetEaAccessSubInfoWin", mOneMinuteInterval),
	mGetEaAccessSubInfoCallsPerHour("LastHourGetEaAccessSubInfoWin", mOneHourInterval),
    mCounterList()
{
	mCounterList.push_back(&mTotalGetEaAccessSubInfoCalls);
	mCounterList.push_back(&mGetEaAccessSubInfoCallsPerMinute);
	mCounterList.push_back(&mGetEaAccessSubInfoCallsPerHour);

	mGetEaAccessSubInfo.addCounter(&mTotalGetEaAccessSubInfoCalls);
	mGetEaAccessSubInfo.addCounter(&mGetEaAccessSubInfoCallsPerMinute);
	mGetEaAccessSubInfo.addCounter(&mGetEaAccessSubInfoCallsPerHour);
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

} // namespace EaAccess
} // namespace Blaze
