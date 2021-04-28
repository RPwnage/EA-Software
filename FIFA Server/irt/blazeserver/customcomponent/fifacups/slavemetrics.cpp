/*H*************************************************************************************************/
/*!

    \File    SlaveMetrics.cpp

    \Description
        Health metrics helper for FifaCups slaves

    \Notes
        None.

    \Copyright
        Copyright (c) Electronic Arts 2010.
        ALL RIGHTS RESERVED.

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/slavemetrics.cpp#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $


*/
/*************************************************************************************************H*/

/*** Include files *******************************************************************************/

#include "framework/blaze.h"
#include "fifacups/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace FifaCups
{

/*** Static C-Style Methods **********************************************************************/

/*** Public Methods ******************************************************************************/

//! Constructor. Allocates per-type counters and sets all data to default values.
SlaveMetrics::SlaveMetrics() :
    mOneDayInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_DAY),
    mOneHourInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_HOUR),
    mOneMinuteInterval(TimeValue::getTimeOfDay(), MICROSECONDS_PER_MINUTE),
    mTotalAwardsAssigned("TotalAwardsAssigned"),
    mTotalAwardsUpdated("TotalAwardsUpdated"),
    mTotalAwardsDeleted("TotalAwardsDeleted"),
    mMissedEndOfSeasonProcessingDetected("MissedEndOfSeasonProcDetected"),
    mEndOfSeasonProcessed("TotalProcessEndOfSeason"),
    mTotalClubRegistrations("TotalClubRegistrations"),
    mClubRegistrationsPerHour("LastHourClubRegistrations", mOneHourInterval),
    mClubRegistrationsPerDay("LastDayClubRegistrations", mOneDayInterval),
    mTotalClubRegistrationUpdates("TotalClubRegUpdates"),
    mClubRegistrationUpdatesPerHour("LastHourClubRegUpdates", mOneHourInterval),
    mClubRegistrationUpdatesPerDay("LastDayClubRegUpdates", mOneDayInterval),
    mTotalUserRegistrations("TotalUserRegistrations"),
    mUserRegistrationsPerHour("LastHourUserRegistrations", mOneHourInterval),
    mUserRegistrationsPerDay("LastDayUserRegistrations", mOneDayInterval),
    mTotalGetTournamentStatusCalls("TotalGetTournStatus"),
    mGetTournamentStatusCallsPerMinute("LastMinuteGetTournStatus", mOneMinuteInterval),
    mGetTournamentStatusCallsPerHour("LastHourGetTournStatus", mOneHourInterval),
    mTotalSetTournamentStatusCalls("TotalSetTournStatus"),
    mSetTournamentStatusCallsPerMinute("LastMinuteSetTournStatus", mOneMinuteInterval),
    mSetTournamentStatusCallsPerHour("LastHourSetTournStatus", mOneHourInterval),
	
	mTotalSetQualifyingDivCalls("TotalSetQualifyingDiv"),
	mSetQualifyingDivCallsPerMinute("LastMinuteSetQualifyingDiv", mOneMinuteInterval),
	mSetQualifyingDivCallsPerHour("LastHourSetQualifyingDiv", mOneHourInterval),
	
	mTotalSetPendingDivCalls("TotalSetPendingDiv"),
	mSetPendingDivCallsPerMinute("LastMinuteSetPendingDiv", mOneMinuteInterval),
	mSetPendingDivCallsPerHour("LastHourSetPendingDiv", mOneHourInterval),
    
	mTotalSetActiveCupIdCalls("TotalSetActiveCupId"),
	mSetActiveCupIdCallsPerMinute("LastMinuteSetActiveCupId", mOneMinuteInterval),
	mSetActiveCupIdCallsPerHour("LastHourSetActiveCupId", mOneHourInterval),

	mTotalResetTournamentStatusCalls("TotalResetTournStatus"),
    mResetTournamentStatusCallsPerMinute("LastMinuteResetTournStatus", mOneMinuteInterval),
    mResetTournamentStatusCallsPerHour("LastHourResetTournStatus", mOneHourInterval),

	mTotalResetSeasonActiveCupIdCalls("TotalResetSeasonActiveCupId"),
	mResetSeasonActiveCupIdCallsPerMinute("LastMinuteResetSeasonActiveCupId", mOneMinuteInterval),
	mResetSeasonActiveCupIdCallsPerHour("LastHourResetSeasonActiveCupId", mOneHourInterval),

	mTotalCopyPendingDivCalls("TotalCopyPendingDiv"),
	mCopyPendingDivCallsPerMinute("LastMinuteCopyPendingDiv", mOneMinuteInterval),
	mCopyPendingDivCallsPerHour("LastHourCopyPendingDiv", mOneHourInterval),
	
	mTotalClearPendingDivCalls("TotalClearPendingDiv"),
	mClearPendingDivCallsPerMinute("LastMinuteClearPendingDiv", mOneMinuteInterval),
	mClearPendingDivCallsPerHour("LastHourClearPendingDiv", mOneHourInterval),
    
	mTotalRankDivCalcs("TotalRankDivCalcs"),
    mRankDivCalcsPerMinute("LastMinuteRankDivCalcs", mOneMinuteInterval),
    mRankDivCalcsPerHour("LastHourRankDivCalcs", mOneHourInterval),
    mTotalDivStartingRankCalcs("TotalDivStartRankCalcs"),
    mDivStartingRankCalcsPerMinute("LastMinuteDivStartRankCalcs", mOneMinuteInterval),
    mDivStartingRankCalcsPerHour("LastHourDivStartRankCalcs", mOneHourInterval),
    mTotalGetAwardsBySeasonCalls("TotalGetAwardsBySeason"),
    mGetAwardsBySeasonCallsPerMinute("LastMinuteGetAwardsBySeason", mOneMinuteInterval),
    mGetAwardsBySeasonCallsPerHour("LastHourGetAwardsBySeason", mOneHourInterval),
    mTotalGetAwardsByMemberCalls("TotalGetAwardsByMember"),
    mGetAwardsByMemberCallsPerMinute("LastMinuteGetAwardsByMember", mOneMinuteInterval),
    mGetAwardsByMemberCallsPerHour("LastHourGetAwardsByMember", mOneHourInterval),
    mCounterList()
{

    mCounterList.push_back(&mTotalClubRegistrations);
    mCounterList.push_back(&mClubRegistrationsPerHour);
    mCounterList.push_back(&mClubRegistrationsPerDay);
    mCounterList.push_back(&mTotalClubRegistrationUpdates);
    mCounterList.push_back(&mClubRegistrationUpdatesPerHour);
    mCounterList.push_back(&mClubRegistrationUpdatesPerDay);
    mCounterList.push_back(&mTotalUserRegistrations);
    mCounterList.push_back(&mUserRegistrationsPerHour);
    mCounterList.push_back(&mUserRegistrationsPerDay);
    mCounterList.push_back(&mTotalGetTournamentStatusCalls);
    mCounterList.push_back(&mGetTournamentStatusCallsPerMinute);
    mCounterList.push_back(&mGetTournamentStatusCallsPerHour);
    mCounterList.push_back(&mTotalSetTournamentStatusCalls);
    mCounterList.push_back(&mSetTournamentStatusCallsPerMinute);
    mCounterList.push_back(&mSetTournamentStatusCallsPerHour);

	mCounterList.push_back(&mTotalSetQualifyingDivCalls);
	mCounterList.push_back(&mSetQualifyingDivCallsPerMinute);
	mCounterList.push_back(&mSetQualifyingDivCallsPerHour);

	mCounterList.push_back(&mTotalSetPendingDivCalls);
	mCounterList.push_back(&mSetPendingDivCallsPerMinute);
	mCounterList.push_back(&mSetPendingDivCallsPerHour);

	mCounterList.push_back(&mTotalResetTournamentStatusCalls);
    mCounterList.push_back(&mResetTournamentStatusCallsPerMinute);
    mCounterList.push_back(&mResetTournamentStatusCallsPerHour);

	mCounterList.push_back(&mTotalResetSeasonActiveCupIdCalls);
	mCounterList.push_back(&mResetSeasonActiveCupIdCallsPerMinute);
	mCounterList.push_back(&mResetSeasonActiveCupIdCallsPerHour);

	mCounterList.push_back(&mTotalCopyPendingDivCalls);
	mCounterList.push_back(&mCopyPendingDivCallsPerMinute);
	mCounterList.push_back(&mCopyPendingDivCallsPerHour);
	
	mCounterList.push_back(&mTotalClearPendingDivCalls);
	mCounterList.push_back(&mClearPendingDivCallsPerMinute);
	mCounterList.push_back(&mClearPendingDivCallsPerHour);
    
	mCounterList.push_back(&mTotalRankDivCalcs);
    mCounterList.push_back(&mRankDivCalcsPerMinute);
    mCounterList.push_back(&mRankDivCalcsPerHour);
    mCounterList.push_back(&mTotalDivStartingRankCalcs);
    mCounterList.push_back(&mDivStartingRankCalcsPerMinute);
    mCounterList.push_back(&mDivStartingRankCalcsPerHour);
    mCounterList.push_back(&mTotalAwardsAssigned);
    mCounterList.push_back(&mTotalAwardsUpdated);
    mCounterList.push_back(&mTotalAwardsDeleted);
    mCounterList.push_back(&mTotalGetAwardsBySeasonCalls);
    mCounterList.push_back(&mGetAwardsBySeasonCallsPerMinute);
    mCounterList.push_back(&mGetAwardsBySeasonCallsPerHour);
    mCounterList.push_back(&mTotalGetAwardsByMemberCalls);
    mCounterList.push_back(&mGetAwardsByMemberCallsPerMinute);
    mCounterList.push_back(&mGetAwardsByMemberCallsPerHour);
    mCounterList.push_back(&mMissedEndOfSeasonProcessingDetected);
    mCounterList.push_back(&mEndOfSeasonProcessed);

    // Set up the Counter Events which group counters
    mClubRegistrations.addCounter(&mTotalClubRegistrations);
    mClubRegistrations.addCounter(&mClubRegistrationsPerHour);
    mClubRegistrations.addCounter(&mClubRegistrationsPerDay);

    mClubRegistrationUpdates.addCounter(&mTotalClubRegistrationUpdates);
    mClubRegistrationUpdates.addCounter(&mClubRegistrationUpdatesPerHour);
    mClubRegistrationUpdates.addCounter(&mClubRegistrationUpdatesPerDay);

    mUserRegistrations.addCounter(&mTotalUserRegistrations);
    mUserRegistrations.addCounter(&mUserRegistrationsPerHour);
    mUserRegistrations.addCounter(&mUserRegistrationsPerDay);

    mGetTournamentStatusCalls.addCounter(&mTotalGetTournamentStatusCalls);
    mGetTournamentStatusCalls.addCounter(&mGetTournamentStatusCallsPerMinute);
    mGetTournamentStatusCalls.addCounter(&mGetTournamentStatusCallsPerHour);

    mSetTournamentStatusCalls.addCounter(&mTotalSetTournamentStatusCalls);
    mSetTournamentStatusCalls.addCounter(&mSetTournamentStatusCallsPerMinute);
    mSetTournamentStatusCalls.addCounter(&mSetTournamentStatusCallsPerHour);
	
	mSetQualifyingDivCalls.addCounter(&mTotalSetQualifyingDivCalls);
	mSetQualifyingDivCalls.addCounter(&mSetQualifyingDivCallsPerMinute);
	mSetQualifyingDivCalls.addCounter(&mSetQualifyingDivCallsPerHour);

	mSetPendingDivCalls.addCounter(&mTotalSetPendingDivCalls);
	mSetPendingDivCalls.addCounter(&mSetPendingDivCallsPerMinute);
	mSetPendingDivCalls.addCounter(&mSetPendingDivCallsPerHour);

	mSetActiveCupIdCalls.addCounter(&mTotalSetActiveCupIdCalls);
	mSetActiveCupIdCalls.addCounter(&mSetActiveCupIdCallsPerMinute);
	mSetActiveCupIdCalls.addCounter(&mSetActiveCupIdCallsPerHour);

    mResetTournamentStatusCalls.addCounter(&mTotalResetTournamentStatusCalls);
    mResetTournamentStatusCalls.addCounter(&mResetTournamentStatusCallsPerMinute);
    mResetTournamentStatusCalls.addCounter(&mResetTournamentStatusCallsPerHour);

	mResetSeasonActiveCupIdCalls.addCounter(&mTotalResetSeasonActiveCupIdCalls);
	mResetSeasonActiveCupIdCalls.addCounter(&mResetSeasonActiveCupIdCallsPerMinute);
	mResetSeasonActiveCupIdCalls.addCounter(&mResetSeasonActiveCupIdCallsPerHour);

	mCopyPendingDiv.addCounter(&mTotalCopyPendingDivCalls);
	mCopyPendingDiv.addCounter(&mCopyPendingDivCallsPerMinute);
	mCopyPendingDiv.addCounter(&mCopyPendingDivCallsPerHour);

	mClearPendingDiv.addCounter(&mTotalClearPendingDivCalls);
	mClearPendingDiv.addCounter(&mClearPendingDivCallsPerMinute);
	mClearPendingDiv.addCounter(&mClearPendingDivCallsPerHour);

    mRankDivCalcs.addCounter(&mTotalRankDivCalcs);
    mRankDivCalcs.addCounter(&mRankDivCalcsPerMinute);
    mRankDivCalcs.addCounter(&mRankDivCalcsPerHour);

    mDivStartingRankCalcs.addCounter(&mTotalDivStartingRankCalcs);
    mDivStartingRankCalcs.addCounter(&mDivStartingRankCalcsPerMinute);
    mDivStartingRankCalcs.addCounter(&mDivStartingRankCalcsPerHour);

    mGetAwardsBySeasonCalls.addCounter(&mTotalGetAwardsBySeasonCalls);
    mGetAwardsBySeasonCalls.addCounter(&mGetAwardsBySeasonCallsPerMinute);
    mGetAwardsBySeasonCalls.addCounter(&mGetAwardsBySeasonCallsPerHour);

    mGetAwardsByMemberCalls.addCounter(&mTotalGetAwardsByMemberCalls);
    mGetAwardsByMemberCalls.addCounter(&mGetAwardsByMemberCallsPerMinute);
    mGetAwardsByMemberCalls.addCounter(&mGetAwardsByMemberCallsPerHour);
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

} // namespace FifaCups
} // namespace Blaze
