/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.h

    \Description
        Stores health check / monitoring statistics about the XmsHd component (slaves)

    \Notes
        None

    \Copyright
        Copyright (c) Electronic Arts 2013.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2014/Gen3/DEV/blazeserver/13.x/customcomponent/xmshd/slavemetrics.h#1 $


*/
/*************************************************************************************************H*/

#ifndef BLAZE_XMSHD_SLAVEMETRICS_H
#define BLAZE_XMSHD_SLAVEMETRICS_H

/*** Include files *******************************************************************************/

#include "framework/util/counter.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ComponentStatus;

namespace XmsHd
{

class SlaveMetrics
{
    NON_COPYABLE(SlaveMetrics);

public: // Functions used by the XmsHd slave impl.

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
	CounterEvent        mPublishData;

private: // Metric variables. These are set by the public counter events above

	//! Total Publish Data By Member calls
	Counter             mTotalPublishDataCalls;
	//! Get Publish Data calls per minute
	IntervalCounter     mPublishDataCallsPerMinute;
	//! Get Publish Data calls per hour
	IntervalCounter     mPublishDataCallsPerHour;

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

} // namespace XmsHd
} // namespace Blaze

#endif // BLAZE_XMSHD_SLAVEMETRICS_H
