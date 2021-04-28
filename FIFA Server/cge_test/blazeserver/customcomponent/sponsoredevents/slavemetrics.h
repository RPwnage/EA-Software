/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.h

    \Description
        Stores health check / monitoring statistics about the SponsoredEvents component (slaves)

    \Notes
        None

    \Copyright
        Copyright (c) Electronic Arts 2010.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/sponsoredevents/slavemetrics.h#1 $
    $Change: 286819 $
    $DateTime: 2012/11/19 16:50:00 $


*/
/*************************************************************************************************H*/

#ifndef BLAZE_SPONSOREDEVENTS_SLAVEMETRICS_H
#define BLAZE_SPONSOREDEVENTS_SLAVEMETRICS_H

/*** Include files *******************************************************************************/

#include "framework/util/counter.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ComponentStatus;

namespace SponsoredEvents
{

class SlaveMetrics
{
    NON_COPYABLE(SlaveMetrics);

public: // Functions used by the SponsoredEvents slave impl.

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

#define DECLARE_COUNTER(counter)								\
	public:														\
		CounterEvent        m##counter;							\
	protected:													\
		Counter             mTotal##counter;					\
		IntervalCounter     m##counter##PerHour;				\
		IntervalCounter     m##counter##PerDay;

    //! CounterEvent to group user registration checks
	DECLARE_COUNTER(UserRegistrationChecks);

    //! CounterEvent to group user registrations
	DECLARE_COUNTER(UserRegistrations);
	
    //! CounterEvent to ban user calls
	DECLARE_COUNTER(UserBans);

    //! CounterEvent to num users calls
	DECLARE_COUNTER(NumUsers);

    //! CounterEvent to remove user calls
	DECLARE_COUNTER(RemoveUser);

    //! CounterEvent to update user flags calls
	DECLARE_COUNTER(UpdateUserFlags);

    //! CounterEvent to wipe user stats calls
	DECLARE_COUNTER(WipeUserStats);

    //! CounterEvent to update event data calls
	DECLARE_COUNTER(UpdateEventData);

	//! CounterEvent to group events URL requests
	DECLARE_COUNTER(EventsURLRequests);

	//! CounterEvent to return users requests
	DECLARE_COUNTER(ReturnUsers);

#undef DECLARE_COUNTER

private: // Metric variables. These are set by the public counter events above

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

} // namespace SponsoredEvents
} // namespace Blaze

#endif // BLAZE_SPONSOREDEVENTS_SLAVEMETRICS_H
