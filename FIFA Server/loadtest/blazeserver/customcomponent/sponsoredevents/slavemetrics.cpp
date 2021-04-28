/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.cpp

    \Description
        Health metrics helper for SponsoredEvents slaves

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2010.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/sponsoredevents/slavemetrics.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $


*/
/*************************************************************************************************H*/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/tdf/controllertypes_server.h"
#include "sponsoredevents/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace SponsoredEvents
{

/*** Static C-Style Methods **********************************************************************/

/*** Public Methods ******************************************************************************/
#define COUNTER_NAME(counter)	#counter
#define CONSTRUCT_COUNTER(counter)											\
	mTotal##counter(COUNTER_NAME(counter)),									\
	m##counter##PerHour("LastHour" COUNTER_NAME(counter), mOneHourInterval),\
	m##counter##PerDay("LastDay" COUNTER_NAME(counter), mOneDayInterval),

#define SETUP_COUNTER(counter)									\
	mCounterList.push_back(&mTotal##counter);					\
	mCounterList.push_back(&m##counter##PerHour);				\
	mCounterList.push_back(&m##counter##PerDay);				\
																\
	m##counter.addCounter(&mTotal##counter);					\
	m##counter.addCounter(&m##counter##PerHour);				\
	m##counter.addCounter(&m##counter##PerDay);					

//! Constructor. Allocates per-type counters and sets all data to default values.
SlaveMetrics::SlaveMetrics() :
    mOneDayInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_DAY),
    mOneHourInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_HOUR),
    mOneMinuteInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_MINUTE),

	CONSTRUCT_COUNTER(UserRegistrationChecks)
	CONSTRUCT_COUNTER(UserRegistrations)
	CONSTRUCT_COUNTER(UserBans)
	CONSTRUCT_COUNTER(NumUsers)
	CONSTRUCT_COUNTER(RemoveUser)
	CONSTRUCT_COUNTER(UpdateUserFlags)
	CONSTRUCT_COUNTER(WipeUserStats)
	CONSTRUCT_COUNTER(UpdateEventData)
	CONSTRUCT_COUNTER(EventsURLRequests)
	CONSTRUCT_COUNTER(ReturnUsers)

    mCounterList()
{
	SETUP_COUNTER(UserRegistrationChecks);
	SETUP_COUNTER(UserRegistrations);
	SETUP_COUNTER(UserBans);
	SETUP_COUNTER(NumUsers);
	SETUP_COUNTER(RemoveUser);
	SETUP_COUNTER(UpdateUserFlags);
	SETUP_COUNTER(WipeUserStats);
	SETUP_COUNTER(UpdateEventData);
	SETUP_COUNTER(EventsURLRequests);
	SETUP_COUNTER(ReturnUsers);

#undef SETUP_COUNTER
#undef CONSTRUCT_COUNTER
#undef COUNTER_NAME
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

} // namespace SponsoredEvents
} // namespace Blaze
