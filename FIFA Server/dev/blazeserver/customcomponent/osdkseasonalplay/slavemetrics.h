/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.h

    \Description
        Stores health check / monitoring statistics about the OSDKSeasonalPlay component (slaves)

    \Notes
        None

    \Copyright
        Copyright (c) Electronic Arts 2010.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2022/GenX/dev/blazeserver/customcomponent/osdkseasonalplay/slavemetrics.h#1 $
    $Change: 1627358 $
    $DateTime: 2020/10/22 16:54:02 $


*/
/*************************************************************************************************H*/

#ifndef BLAZE_OSDKSEASONALPLAY_SLAVEMETRICS_H
#define BLAZE_OSDKSEASONALPLAY_SLAVEMETRICS_H

/*** Include files *******************************************************************************/

#include "framework/util/counter.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class ComponentStatus;

namespace OSDKSeasonalPlay
{

class SlaveMetrics
{
    NON_COPYABLE(SlaveMetrics);

public: // Functions used by the OSDKSeasonalPlay slave impl.

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

    //! CounterEvent to group Club registrations counters
    CounterEvent        mClubRegistrations;

    //! CounterEvent to group Club registration updates
    CounterEvent        mClubRegistrationUpdates;

    //! CounterEvent to group Club registrations deleted counters
    CounterEvent        mClubRegistrationsDeleted;

    //! CounterEvent to group User registrations
    CounterEvent        mUserRegistrations;

    //! CounterEvent to group User registrations deleted counters
    CounterEvent        mUserRegistrationsDeleted;

    //! CounterEvent to group GetTournamentStatus calls
    CounterEvent        mGetTournamentStatusCalls;

    //! CounterEvent to group SetTournamentStatus calls
    CounterEvent        mSetTournamentStatusCalls;

    //! CounterEvent to group ResetTournamentStatus calls
    CounterEvent        mResetTournamentStatusCalls;

    //! CounterEent to group Rank and Division calculations
    CounterEvent        mRankDivCalcs;

    //! CounterEent to group Division Starting Rank calculations
    CounterEvent        mDivStartingRankCalcs;

    //! Total Awards assigned
    Counter             mTotalAwardsAssigned;

    //! Total Awards Updated
    Counter             mTotalAwardsUpdated;

    //! Total Awards Deleted
    Counter             mTotalAwardsDeleted;

    //! CounterEvent to group Get Award by Season calls
    CounterEvent        mGetAwardsBySeasonCalls;

    //! CounterEvent to group Get Award by Member calls
    CounterEvent        mGetAwardsByMemberCalls;

    //! Missed End of Season Processing Detected
    Counter             mMissedEndOfSeasonProcessingDetected;

    //! Total Process End of Season calls
    Counter             mEndOfSeasonProcessed;

    //! Number of seasons
    Counter             mTotalSeasons;

    //! Number of configured awards
    Counter             mTotalAwards;

    //! Amount of time in ms that the last end of season processing took
    Counter             mLastEndSeasonProcessingTimeMs;

private: // Metric variables. These are set by the public counter events above

    //! Total Club Registrations
    Counter             mTotalClubRegistrations;
    //! Club Registrations in last hour
    IntervalCounter     mClubRegistrationsPerHour;
    //! Club Registrations in last day
    IntervalCounter     mClubRegistrationsPerDay;

    //! Total Club Registration Updates
    Counter             mTotalClubRegistrationUpdates;
    //! Club Registrations updates in last hour
    IntervalCounter     mClubRegistrationUpdatesPerHour;
    //! Club Registrations updates in last day
    IntervalCounter     mClubRegistrationUpdatesPerDay;

    //! Total Club Registration Deleted
    Counter             mTotalClubRegistrationDeleted;
    //! Club Registrations deleted in last hour
    IntervalCounter     mClubRegistrationDeletedPerHour;
    //! Club Registrations deleted in last day
    IntervalCounter     mClubRegistrationDeletedPerDay;

    //! Total User Registrations
    Counter             mTotalUserRegistrations;
    //! User Registrations in last hour
    IntervalCounter     mUserRegistrationsPerHour;
    //! User Registrations in last day
    IntervalCounter     mUserRegistrationsPerDay;

    //! Total User Registrations Deleted
    Counter             mTotalUserRegistrationDeleted;
    //! User Registrations deleted in last hour
    IntervalCounter     mUserRegistrationDeletedPerHour;
    //! User Registrations deleted in last day
    IntervalCounter     mUserRegistrationDeletedPerDay;

    //! Total GetTournamentStatus calls
    Counter             mTotalGetTournamentStatusCalls;
    //! GetTournamentStatus calls in the last minute
    IntervalCounter     mGetTournamentStatusCallsPerMinute;
    //! GetTournamentStatus calls in the last hour
    IntervalCounter     mGetTournamentStatusCallsPerHour;

    //! Total SetTournamentStatus calls
    Counter             mTotalSetTournamentStatusCalls;
    //! SetTournamentStatus calls in the last minute
    IntervalCounter     mSetTournamentStatusCallsPerMinute;
    //! SetTournamentStatus calls in the last hour
    IntervalCounter     mSetTournamentStatusCallsPerHour;

    //! Total ResetTournamentStatus calls
    Counter             mTotalResetTournamentStatusCalls;
    //! ResetTournamentStatus calls in the last minute
    IntervalCounter     mResetTournamentStatusCallsPerMinute;
    //! ResetTournamentStatus calls in the last hour
    IntervalCounter     mResetTournamentStatusCallsPerHour;

    //! Total Rank and Division calculations
    Counter             mTotalRankDivCalcs;
    //! Rank and Division calculations per minute
    IntervalCounter     mRankDivCalcsPerMinute;
    //! Rank and Division calculations per hour
    IntervalCounter     mRankDivCalcsPerHour;

    //! Total Division Starting Rank calculations
    Counter             mTotalDivStartingRankCalcs;
    //! Division Starting Rank  calculations per minute
    IntervalCounter     mDivStartingRankCalcsPerMinute;
    //! Division Starting Rank  calculations per hour
    IntervalCounter     mDivStartingRankCalcsPerHour;

    //! Total Get Awards By Season calls
    Counter             mTotalGetAwardsBySeasonCalls;
    //! Get Awards By Season calls per minute
    IntervalCounter     mGetAwardsBySeasonCallsPerMinute;
    //! Get Awards By Season calls per hour
    IntervalCounter     mGetAwardsBySeasonCallsPerHour;

    //! Total Get Awards By Member calls
    Counter             mTotalGetAwardsByMemberCalls;
    //! Get Awards By Member calls per minute
    IntervalCounter     mGetAwardsByMemberCallsPerMinute;
    //! Get Awards By Member calls per hour
    IntervalCounter     mGetAwardsByMemberCallsPerHour;

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

} // namespace OSDKSeasonalPlay
} // namespace Blaze

#endif // BLAZE_OSDKSEASONALPLAY_SLAVEMETRICS_H
