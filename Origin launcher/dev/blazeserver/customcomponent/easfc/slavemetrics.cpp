/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.cpp

    \Description
        Health metrics helper for Easfc slaves

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2010.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/slavemetrics.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $


*/
/*************************************************************************************************H*/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "easfc/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace EASFC
{

/*** Static C-Style Methods **********************************************************************/

/*** Public Methods ******************************************************************************/

//! Constructor. Allocates per-type counters and sets all data to default values.
SlaveMetrics::SlaveMetrics() :
    mOneDayInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_DAY),
    mOneHourInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_HOUR),
    mOneMinuteInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_MINUTE),
	mTotalPurchaseGameWinCalls("TotalPurchaseGameWin"),
	mPurchaseGameWinCallsPerMinute("LastMinutePurchaseGameWin", mOneMinuteInterval),
	mPurchaseGameWinCallsPerHour("LastHourPurchaseGameWin", mOneHourInterval),
	mTotalPurchaseGameDrawCalls("TotalPurchaseGameDraw"),
	mPurchaseGameDrawCallsPerMinute("LastMinutePurchaseGameDraw", mOneMinuteInterval),
	mPurchaseGameDrawCallsPerHour("LastHourPurchaseGameDraw", mOneHourInterval),
	mTotalPurchaseGameLossCalls("TotalPurchaseGameLoss"),
	mPurchaseGameLossCallsPerMinute("LastMinutePurchaseGameLoss", mOneMinuteInterval),
	mPurchaseGameLossCallsPerHour("LastHourPurchaseGameLoss", mOneHourInterval),
	mTotalPurchaseGameMatchCalls("TotalPurchaseGameMatch"),
	mPurchaseGameMatchCallsPerMinute("LastMinutePurchaseGameLoss", mOneMinuteInterval),
	mPurchaseGameMatchCallsPerHour("LastHourPurchaseGameMatch", mOneHourInterval),
    mCounterList()
{
	mCounterList.push_back(&mTotalPurchaseGameWinCalls);
	mCounterList.push_back(&mPurchaseGameWinCallsPerMinute);
	mCounterList.push_back(&mPurchaseGameWinCallsPerHour);

	mCounterList.push_back(&mTotalPurchaseGameDrawCalls);
	mCounterList.push_back(&mPurchaseGameDrawCallsPerMinute);
	mCounterList.push_back(&mPurchaseGameDrawCallsPerHour);

	mCounterList.push_back(&mTotalPurchaseGameLossCalls);
	mCounterList.push_back(&mPurchaseGameLossCallsPerMinute);
	mCounterList.push_back(&mPurchaseGameLossCallsPerHour);

	mCounterList.push_back(&mTotalPurchaseGameMatchCalls);
	mCounterList.push_back(&mPurchaseGameMatchCallsPerMinute);
	mCounterList.push_back(&mPurchaseGameMatchCallsPerHour);

	mPurchaseGameWin.addCounter(&mTotalPurchaseGameWinCalls);
	mPurchaseGameWin.addCounter(&mPurchaseGameWinCallsPerMinute);
	mPurchaseGameWin.addCounter(&mPurchaseGameWinCallsPerHour);

	mPurchaseGameDraw.addCounter(&mTotalPurchaseGameDrawCalls);
	mPurchaseGameDraw.addCounter(&mPurchaseGameDrawCallsPerMinute);
	mPurchaseGameDraw.addCounter(&mPurchaseGameDrawCallsPerHour);

	mPurchaseGameLoss.addCounter(&mTotalPurchaseGameLossCalls);
	mPurchaseGameLoss.addCounter(&mPurchaseGameLossCallsPerMinute);
	mPurchaseGameLoss.addCounter(&mPurchaseGameLossCallsPerHour);

	mPurchaseGameLoss.addCounter(&mTotalPurchaseGameMatchCalls);
	mPurchaseGameLoss.addCounter(&mPurchaseGameMatchCallsPerMinute);
	mPurchaseGameLoss.addCounter(&mPurchaseGameMatchCallsPerHour);
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

} // namespace EASFC
} // namespace Blaze
