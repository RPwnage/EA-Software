/*************************************************************************************************/
/*!
    \file   fifacupsslaveimpl.h

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/fifacupsslaveimpl.h#1 $
    $Change: 286819 $
    $DateTime: 2011/02/24 16:14:33 $

    \attention
    (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_FIFACUPS_SLAVEIMPL_H
#define BLAZE_FIFACUPS_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicationcallback.h"
#include "framework/usersessions/usersessionmanager.h"

// for stats rollover event
#include "stats/statscommontypes.h"
#include "component/stats/rpc/statsmaster.h"

#include "fifacups/rpc/fifacupsslave_stub.h"
#include "fifacups/rpc/fifacupsmaster.h"
#include "fifacups/tdf/fifacupstypes.h"
#include "fifacups/tdf/fifacupstypes_server.h"
#include "fifacups/fifacupstimeutils.h"
#include "fifacups/slavemetrics.h"

#include "framework/database/dbconn.h"
#include "fifacups/fifacupsdb.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace Stats{class StatsSlave;}
namespace FifaCups
{

class FifaCupsSlaveImpl : 
    public FifaCupsSlaveStub,
    private Stats::StatsMasterListener,
    private UserSessionSubscriber,
    private FifaCupsMasterListener,
    private FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::Subscriber
{
public:
    FifaCupsSlaveImpl();
    ~FifaCupsSlaveImpl();

    // Component Interface
    virtual uint16_t getDbSchemaVersion() const { return 2; }
    
	// registers an entity in the season that matches the club's league id
    BlazeRpcError registerEntity(MemberId entityId, MemberType memberType, LeagueId leagueId);

    // registers a club in the season that matches the club's league id
    BlazeRpcError registerClub(MemberId clubId, LeagueId leagueId);

    // updates a club's register due to the club changing the league it is in
    BlazeRpcError updateClubRegistration(MemberId clubId, LeagueId newLeagueId);

    // registers the entity in the specified season
    BlazeRpcError registerForSeason(MemberId memberId, MemberType memberType, LeagueId leagueId, SeasonId seasonId);

	// gets the time that the cup period starts
	int64_t getCupStartTime(SeasonId seasonId);

	// gets the time that the cup period ends
	int64_t getCupEndTime(SeasonId seasonId);

	// gets the time that the current season started
	int64_t getSeasonStartTime(SeasonId seasonId);

	// gets the time that the current season ends
	int64_t getSeasonEndTime(SeasonId seasonId);

	// get the cup window start time for the next season
	int64_t getNextCupStartTime(SeasonId seasonId);

	// get the state of the specified cup
	CupState getCupState(SeasonId seasonId);

    // checks if the specified season id represents a valid season
    bool8_t isValidSeason(SeasonId seasonId);

    // get the season id that the member is registered in
    SeasonId getSeasonId(MemberId memberId, MemberType memberType, BlazeRpcError &outError);
    
	// get registration details of the member is registered in
	void getRegistrationDetails(MemberId memberId, MemberType memberType, SeasonId &outSeasonId, uint32_t &outQualifyingDiv, uint32_t &outPendingDiv, uint32_t &outActiveCupId, TournamentStatus &outTournamentStatus, BlazeRpcError &outError);

	// get registration details of the member is registered in
	void getTournamentDetails(SeasonId seasonId, uint32_t qualifyingDiv, uint32_t &outTournamentId, uint32_t &outCupId, TournamentRule &outTournamentRule, BlazeRpcError &outError);

	// obtain the list of cups within each season based on season id
	void getCupList(SeasonId seasonId, BlazeRpcError &outError, EA::TDF::TdfStructVector<Cup>& outCupList);

	// update the qualifying division for a member
	BlazeRpcError setMemberQualifyingDivision(MemberId memberId, MemberType memberType, uint32_t qualifyingDiv);

	// update the active cup id for a member
	BlazeRpcError setMemberActiveCupId(MemberId memberId, MemberType memberType, uint32_t activeCupId);

    // get the tournament status of a member
    TournamentStatus getMemberTournamentStatus(MemberId memberId, MemberType memberType, BlazeRpcError &outError);

    // update the tournament status for a member
    BlazeRpcError setMemberTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus tournamentStatus, DbConnPtr conn = DbConnPtr());

	// get cup details for memberType 
	void getCupDetailsForMemberType(MemberType memberType, SeasonId &outSeasonId, uint32_t &outStartDiv, uint32_t &outStartCupId, BlazeRpcError &outError);

	// update the cup count status for a member
	BlazeRpcError updateMemberCupCount(MemberId memberId, MemberType memberType, SeasonId seasonId);

	// update the stat CupsEntered for a member
	BlazeRpcError updateMemberCupsEntered(MemberId memberId, MemberType memberType, SeasonId seasonId);

	// update the cup count status for a member
	BlazeRpcError updateMemberElimCount(MemberId memberId, MemberType memberType, SeasonId seasonId);

    // reset the tournament status for all members of a season to SEASONALPLAY_TOURNAMENT_STATUS_NONE
    BlazeRpcError resetSeasonTournamentStatus(SeasonId seasonId);

	// resets the active cup id for all members of a season to 0
	BlazeRpcError resetSeasonActiveCupId(SeasonId seasonId);

	// copy the pending division for all members of a season to the qualifying division
	BlazeRpcError copySeasonPendingDivision(SeasonId seasonId);

	// clear the pending division for all members of a season to the supplied value
	BlazeRpcError clearSeasonPendingDivision(SeasonId seasonId, uint32_t clearValue);

    // get the tournament id for a season. Returns 0 if tournament id for season cannot be found
    TournamentId getSeasonTournamentId(SeasonId seasonId);

    // is the member eligible for seasonal tournament play?
    bool8_t getIsTournamentEligible(SeasonId seasonId, TournamentRule tournamentRule, TournamentStatus tournamentStatus);

    // inserts the member id's of all members registered in the specified season into the passed in vector
    void getRegisteredMembers(SeasonId seasonId, eastl::vector<MemberId> &outMemberList);

    // gets the MemberType for the season
    MemberType getSeasonMemberType(SeasonId seasonId);

    // gets the LeagueId for the season
    LeagueId getSeasonLeagueId(SeasonId seasonId);

private:

    // component life cycle events
    virtual bool onConfigureReplication();
    virtual bool onConfigure();
    virtual bool onReconfigure();
    virtual bool onResolve();
    virtual void onShutdown();

	//! Called to perform tasks common to onConfigure and onReconfigure
	bool configureHelper(Stats::StatsMaster* statsComponent = nullptr);

    //! Called periodically to collect status and statistics about this component that can be used to determine what future improvements should be made.
    virtual void getStatusInfo(ComponentStatus& status) const;


    // ReplicatedSeasonConfigurationMap::Subscriber interface
    virtual void onInsert(const ReplicatedSeasonConfigurationMap& replMap, 
        ObjectId key, ReplicatedMapItem<Blaze::FifaCups::SeasonConfigurationServer>& tdfObject);
    virtual void onUpdate(const ReplicatedSeasonConfigurationMap& replMap, 
        ObjectId key, ReplicatedMapItem<Blaze::FifaCups::SeasonConfigurationServer>& tdfObject) {}; // ignore this event
    virtual void onErase(const ReplicatedSeasonConfigurationMap& replMap, 
        ObjectId key, ReplicatedMapItem<Blaze::FifaCups::SeasonConfigurationServer>& tdfObject) {}; // ignore this event

    // check if we missed processing the last end of season for the season. Called during startup
    void checkMissedEndOfSeasonProcessing(SeasonId seasonId);

	// season roll over custom hook
	void processWinners(eastl::vector<MemberInfo>& wonMembers, const SeasonConfigurationServer *pSeasonConfig);

	// season roll over custom hook
	void processLosers(eastl::vector<MemberInfo>& lossMembers, const SeasonConfigurationServer *pSeasonConfig);

	// update the given stat for a member
	BlazeRpcError updateMemberCupStats(MemberId memberId, MemberType memberType, const char* userStatsCategory, const char* userStatsName);

    // leave tournament custom hook
    void onLeaveTournament(MemberId memberId, MemberType memberType, TournamentId tournamentId);

    // reset tournament custom hook
    void onResetTournament(SeasonId seasonId, TournamentId tournamentId);

    // member's tournament status updated - only called for win or being eliminated from a tournament
    void onTournamentStatusUpdate(MemberId memberId, MemberType memberType, TournamentStatus tournamentStatus);

    // notification from the master to begin end of season processing
    virtual void onProcessEndOfSeasonNotification(const EndOfSeasonRolloverData &rolloverData, UserSession *associatedUserSession);

    // do the actual end of season processing
    void processEndOfSeason(Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId);

	void fireRolloverEvent(Stats::PeriodIds newPeriodIds);
	void onStatsRollover(Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId);

	// StatsMasterListener events
	virtual void onUpdateCacheNotification(const Blaze::Stats::StatUpdateBroadcast& data,UserSession *associatedUserSession) {}
	virtual void onDeleteCategoryCacheNotification(const Blaze::Stats::StatDeleteAndUpdateBroadcast& data,UserSession *associatedUserSession) {}
	virtual void onDeleteAndUpdateCacheNotification(const Blaze::Stats::StatDeleteAndUpdateBroadcast& data,UserSession *associatedUserSession) {}
	virtual void onUpdateGlobalStatsNotification(const Blaze::Stats::UpdateStatsRequest& data,UserSession *associatedUserSession) {}
	virtual void onTrimPeriodNotification(const Blaze::Stats::StatPeriod& data,UserSession *associatedUserSession) {}
	virtual void onSetPeriodIdsNotification(const Blaze::Stats::PeriodIds& data,UserSession *associatedUserSession);
	virtual void onExecuteGlobalCacheInstruction(const Blaze::Stats::StatsGlobalCacheInstruction& data,UserSession *associatedUserSession) {}
	virtual void onDeleteByEntityCacheNotification(const Blaze::Stats::StatDeleteAndUpdateBroadcast& data,UserSession *associatedUserSession) {}

    // event handlers for User Session Manager events
    virtual void onUserSessionExistence(const UserSession& userSession);

    // helper to get a specific season from the season configuration map
    const SeasonConfigurationServer* getSeasonConfiguration(SeasonId seasonId);

    // helper to find the specific season that matches a league id
    const SeasonConfigurationServer* getSeasonConfigurationForLeague(LeagueId leagueId, MemberType memberType);

    // helper to register a user in versus season if not already registered
    void verifyUserRegistration(BlazeId userId);

    // helper to get the stats period time for the specified season
    Stats::StatPeriodType getSeasonPeriodType(SeasonId seasonId);
	
	// helper to get the cup object index from config based on the cupId
	int getCupIndexFromConfig(SeasonId seasonId, uint32_t cupId);

	// helper to get  the current active cup for current stat period (for cup rotation)
	int getCurrentActiveCupIndex(SeasonId seasonId);

	// helper to get information for future stat periods (for cup rotation)
	void calculateFutureStatPeriod(SeasonId seasonId, int periodsIntheFuture, TimeStamp& cupStart, TimeStamp& statPeriodStart, TimeStamp& statPeriodEnd);

    bool8_t                     mAutoUserRegistrationEnabled;
    bool8_t                     mAutoClubRegistrationEnabled;
    uint32_t                    mDbId;

    FifaCupsTimeUtils   mTimeUtils;

    //! Health monitoring statistics.
    SlaveMetrics mMetrics;

	//! Store the old period ids since after onSetPeriodIdsNotification is called, the RPC returns the new ids
	Blaze::Stats::PeriodIds mOldPeriodIds;

	bool mRefreshPeriodIds;
};

} // FifaCups
} // Blaze

#endif // BLAZE_FIFACUPS_SLAVEIMPL_H

