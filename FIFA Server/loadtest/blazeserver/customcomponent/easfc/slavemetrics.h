/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.h

    \Description
        Stores health check / monitoring statistics about the Easfc component (slaves)

    \Notes
        None

    \Copyright
        Copyright (c) Electronic Arts 2010.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/easfc/slavemetrics.h#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $


*/
/*************************************************************************************************H*/

#ifndef BLAZE_EASFC_SLAVEMETRICS_H
#define BLAZE_EASFC_SLAVEMETRICS_H

/*** Include files *******************************************************************************/

#include "framework/util/counter.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ComponentStatus;

namespace EASFC
{

class SlaveMetrics
{
    NON_COPYABLE(SlaveMetrics);

public: // Functions used by the Easfc slave impl.

    //! Constructor. Allocates per-type counters and sets all data to default values.
    SlaveMetrics();

    //! Destructor. Cleans up allocated memory.
    ~SlaveMetrics();

    //! Reports all metrics to the specified status object.
    void report(ComponentStatus* status) const;

private: // These variables need to be declared first due to constructor initialization order.

    //! Interval used by the per-day IntervalCounter members.
    Interval mOneDayInterval;

    //! Interval used by the per-hour IntervalCounter members.
    Interval mOneHourInterval;

    //! Interval used by the per-minute IntervalCounter members
    Interval mOneMinuteInterval;

public: // Metric variables. These are the ones that should be set

	//! Purchase Game Win Event Counter
	CounterEvent        mPurchaseGameWin;

	//! Purchase Game Draw Event Counter
	CounterEvent        mPurchaseGameDraw;

	//! Purchase Game Loss Event Counter
	CounterEvent        mPurchaseGameLoss;

	//! Purchase Game Match Event Counter
	CounterEvent        mPurchaseGameMatch;

private: // Metric variables. These are set by the public counter events above

	//! Total Purchase Game Win By Member calls
	Counter             mTotalPurchaseGameWinCalls;
	//! Get Purchase Game Win calls per minute
	IntervalCounter     mPurchaseGameWinCallsPerMinute;
	//! Get Purchase Game Win calls per hour
	IntervalCounter     mPurchaseGameWinCallsPerHour;

	//! Total Purchase Game Draw By Member calls
	Counter             mTotalPurchaseGameDrawCalls;
	//! Get Purchase Game Draw calls per minute
	IntervalCounter     mPurchaseGameDrawCallsPerMinute;
	//! Get Purchase Game Draw calls per hour
	IntervalCounter     mPurchaseGameDrawCallsPerHour;

	//! Total Purchase Game Loss By Member calls
	Counter             mTotalPurchaseGameLossCalls;
	//! Get Purchase Game Loss calls per minute
	IntervalCounter     mPurchaseGameLossCallsPerMinute;
	//! Get Purchase Game Loss calls per hour
	IntervalCounter     mPurchaseGameLossCallsPerHour;

	//! Total Purchase Game Match By Member calls
	Counter             mTotalPurchaseGameMatchCalls;
	//! Get Purchase Game Match calls per minute
	IntervalCounter     mPurchaseGameMatchCallsPerMinute;
	//! Get Purchase Game Match calls per hour
	IntervalCounter     mPurchaseGameMatchCallsPerHour;

private:

    //! Number of microseconds in a single day
    static const int64_t MICROSECONDS_PER_DAY = 86400000000LL;
    //! Number of microseconds in a single hour.
    static const int64_t MICROSECONDS_PER_HOUR = 3600000000LL;

    //! Number of microseconds in a single minute
    static const int64_t MICROSECONDS_PER_MINUTE = 60000000LL;

    typedef eastl::vector<Counter*> CounterList;
    //! List of all metrics.
    CounterList mCounterList;

};

} // namespace EASFC
} // namespace Blaze

#endif // BLAZE_EASFC_SLAVEMETRICS_H
