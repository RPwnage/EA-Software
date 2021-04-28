/*************************************************************************************************/
/*!
    \file   osdkseasonalplayslaveimpl.h

    $Header: //gosdev/games/FIFA/2022/GenX/irt/blazeserver/customcomponent/osdkseasonalplay/osdkseasonalplayslaveimpl.h#1 $
    $Change: 1646537 $
    $DateTime: 2021/02/01 04:49:23 $

    \attention
    (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OSDKSEASONALPLAY_SLAVEIMPL_H
#define BLAZE_OSDKSEASONALPLAY_SLAVEIMPL_H

/*** Include files *******************************************************************************/
#include "framework/replication/replicatedmap.h"
#include "framework/replication/replicationcallback.h"
#include "framework/usersessions/usersessionmanager.h"
#include "framework/taskscheduler/taskschedulerslaveimpl.h"

// for stats rollover event
#include "stats/statscommontypes.h"
#include "component/stats/rpc/statsmaster.h"

#include "osdkseasonalplay/rpc/osdkseasonalplayslave_stub.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes.h"
#include "osdkseasonalplay/tdf/osdkseasonalplaytypes_server.h"
#include "osdkseasonalplay/osdkseasonalplaydb.h"
#include "osdkseasonalplay/osdkseasonalplaytimeutils.h"
#include "osdkseasonalplay/slavemetrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace Stats{class StatsSlave;}
namespace OSDKSeasonalPlay
{

class OSDKSeasonalPlaySlaveImpl : 
    public OSDKSeasonalPlaySlaveStub,
    private Stats::StatsMasterListener,
    private UserSessionSubscriber
{
public:
    OSDKSeasonalPlaySlaveImpl();
    ~OSDKSeasonalPlaySlaveImpl();

    // Component Interface
    virtual uint16_t        getDbSchemaVersion() const  { return 1; }
    virtual const char8_t*  getDbName() const;

    // registers a club in the season that matches the club's league id
    BlazeRpcError registerClub(MemberId clubId, LeagueId leagueId);

    // updates a club's register due to the club changing the league it is in
    BlazeRpcError updateClubRegistration(MemberId clubId, LeagueId newLeagueId);

    // registers the entity in the specified season
    BlazeRpcError registerForSeason(OSDKSeasonalPlayDb& dbHelper, MemberId memberId, MemberType memberType, LeagueId leagueId, SeasonId seasonId);

    // delete the entity from the registration
    BlazeRpcError deleteRegistration(MemberId memberId, MemberType memberType);

    // gets the time that the current season started
    int64_t getSeasonStartTime(SeasonId seasonId);

    // gets the time that the current season ends
    int64_t getSeasonEndTime(SeasonId seasonId);

    // gets the time that the playoff period starts for the current season
    int64_t getPlayoffStartTime(SeasonId seasonId);

    // gets the time that the playoff period ends for the current season
    int64_t getPlayoffEndTime(SeasonId seasonId);

    // get the regular season end time
    int64_t getRegularSeasonEndTime(SeasonId seasonId);

    // get the regular season start time for the next season
    int64_t getNextSeasonRegularSeasonStartTime(SeasonId seasonId);

    // get the state of the specified season
    SeasonState getSeasonState(SeasonId seasonId);

    // checks if the specified season id represents a valid season
    bool8_t isValidSeason(SeasonId seasonId);

    // get the tournament status of a member
    TournamentStatus getMemberTournamentStatus(OSDKSeasonalPlayDb& dbHelper, MemberId memberId, MemberType memberType, BlazeRpcError &outError);

    // update the tournament status for a member
    BlazeRpcError setMemberTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus tournamentStatus);

    // reset the tournament status for all members of a season to SEASONALPLAY_TOURNAMENT_STATUS_NONE
    BlazeRpcError resetSeasonTournamentStatus(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId);

    // get the tournament id for a season. Returns 0 if tournament id for season cannot be found
    TournamentId getSeasonTournamentId(SeasonId seasonId);

    // get the regular season stats category name for a season
    const char8_t* getSeasonStatsCategory(SeasonId seasonId);

    // get the regular season leaderboard name for a season
    const char8_t* getSeasonLeaderboardName(SeasonId seasonId);

    // is the member eligible for seasonal tournament play?
    bool8_t getIsTournamentEligible(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId, MemberId memberId, MemberType memberType, BlazeRpcError &outError);

    // calculate the overall and divisional ranks for the specified member. Set fromPreviousSeason to true
    // during end of season processing to get the season that just rolled over.
    BlazeRpcError calculateRankAndDivision(SeasonId seasonId, MemberId memberId, uint32_t &outDivisionRank, uint8_t &outDivision, uint32_t &outDivisionStartingRank, uint32_t &outOverallRank, bool8_t fromPreviousSeason = false);

    // gets the count of members that are ranked on the season's regular season leaderboard
    uint32_t getRankedMemberCount(SeasonId seasonId, bool8_t fromPreviousSeason = false);

    // calculate the division start ranks number for the specified season. Requires the number of entities in the leaderboard. Returns an ordered vector of the division starting ranks
    void calculateDivisionStartingRanks(SeasonId seasonId, uint32_t lbEntityCount, eastl::vector<uint32_t> &outStartingRanks, uint8_t &outDivisionCount);

    // inserts the member id's of all members registered in the specified season into the passed in vector
    BlazeRpcError getRegisteredMembers(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId, eastl::vector<MemberId> &outMemberList);

    // gets the MemberType for the season
    MemberType getSeasonMemberType(SeasonId seasonId);

    // gets the LeagueId for the season
    LeagueId getSeasonLeagueId(SeasonId seasonId);

    typedef eastl::hash_map<int32_t, AwardConfiguration*> AwardConfigurationMap;
    AwardConfigurationMap& getAwardConfigurationMap() { return mAwardConfigurationMap; }

    typedef eastl::hash_map<SeasonId, SeasonConfigurationServer*> SeasonConfigurationServerMap;
    SeasonConfigurationServerMap& getSeasonConfigurationMap() { return mSeasonConfigurationServerMap; }

    ///////////////////////////////////////////////////////////
    // Awards
    ///////////////////////////////////////////////////////////

    // assign an award 
    BlazeRpcError assignAward(OSDKSeasonalPlayDb& dbHelper, AwardId awardId, MemberId memberId, MemberType memberType, SeasonId seasonId, LeagueId leagueId, uint32_t seasonNumber, bool8_t historical = false, const char8_t* metadata = NULL);

    // update the member an award is assigned to. Done by award assignment id, not the award id.
    BlazeRpcError updateAwardMember(OSDKSeasonalPlayDb& dbHelper, uint32_t assignmentId, MemberId memberId);

    // delete an award. Done by award assignment id
    BlazeRpcError deleteAward(OSDKSeasonalPlayDb& dbHelper, uint32_t assignmentId);

    // get awards by season id. Use AWARD_ANY_SEASON_NUMBER to get for all season numbers
    BlazeRpcError getAwardsBySeason(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId, uint32_t seasonNumber, AwardHistoricalFilter historicalFilter, AwardList &outAwardList);

    // get award by member. Use AWARD_ANY_SEASON_NUMBER to get for all season numbers
    BlazeRpcError getAwardsByMember(OSDKSeasonalPlayDb& dbHelper, MemberId memberId, MemberType memberType, SeasonId seasonId, uint32_t seasonNumber, AwardList &outAwardList);

private:

    // component life cycle events
    virtual bool onConfigure();
    virtual bool onReconfigure();
    virtual bool onResolve();
    virtual void onShutdown();

    //! Called to perform tasks common to onConfigure and onReconfigure
    bool8_t configureHelper();
    bool8_t parseTimeDuration(int32_t minutes, int32_t hours, int32_t days, TimeStamp &result);
    bool8_t parsePeriodType(int32_t iPeriodType, Stats::StatPeriodType &result);
    bool8_t parseDivisions(const ConfigInstanceDivisionList &divisions, SeasonConfigurationServer *seasonConfig, SeasonConfigurationServer::DivisionList& divisionList);

    bool8_t parseConfig();
    void debugPrintConfig();

    //! Called periodically to collect status and statistics about this component that can be used to determine what future improvements should be made.
    virtual void getStatusInfo(ComponentStatus& status) const;

    // check if we missed processing the last end of season for the seasons. Called during startup.
    void checkMissedEndOfSeasonProcessing();

    // season roll over custom hook
    void onSeasonRollover(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId, uint32_t oldSeasonNumber, int32_t oldPeriodId, int32_t newPeriodId);

    // leave tournament custom hook
    void onLeaveTournament(MemberId memberId, MemberType memberType, TournamentId tournamentId);

    // reset tournament custom hook
    void onResetTournament(SeasonId seasonId, TournamentId tournamentId);

    // member's tournament status updated - only called for win or being eliminated from a tournament
    BlazeRpcError onTournamentStatusUpdate(OSDKSeasonalPlayDb& dbHelper, MemberId memberId, MemberType memberType, TournamentStatus tournamentStatus);

    // do the actual end of season processing
    void processEndOfSeason(Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId);

    void onStatsRollover(Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId);

    // StatsMasterListener events
	virtual void onUpdateCacheNotification(const Blaze::Stats::StatUpdateNotification& data, UserSession *associatedUserSession) {}
    virtual void onTrimPeriodNotification(const Blaze::Stats::StatPeriod& data,UserSession *associatedUserSession) {}
    virtual void onSetPeriodIdsNotification(const Blaze::Stats::PeriodIds& data,UserSession *associatedUserSession);
	virtual void onUpdateGlobalStatsNotification(const Blaze::Stats::UpdateStatsRequest& data, UserSession *associatedUserSession) {}
    virtual void onExecuteGlobalCacheInstruction(const Blaze::Stats::StatsGlobalCacheInstruction& data,UserSession *associatedUserSession) {}

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

    TournamentRule getTournamentRule(SeasonId seasonId, uint8_t divisionNumber);

    // helper to get leaderboard keyscope info from stats component
    bool8_t getLeaderboardHasScope(const char8_t* leaderBoardName);

    // Holds arguments for a task since TaskScheduler's RunOneTimeTaskCb doesn't provide argument list
    class RunOneTimeTaskHelper
    {
    public:
        RunOneTimeTaskHelper(OSDKSeasonalPlaySlaveImpl* component, Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId):
            mComponent(component), mPeriodType(periodType), mOldPeriodId(oldPeriodId), mNewPeriodId(newPeriodId) {}

        void execute(BlazeRpcError& error)
        {
            mComponent->processEndOfSeason(mPeriodType, mOldPeriodId, mNewPeriodId);
        }
        
    private:
        OSDKSeasonalPlaySlaveImpl* mComponent;
        Stats::StatPeriodType mPeriodType;
        int32_t mOldPeriodId;
        int32_t mNewPeriodId;
    };

    bool8_t                     mAutoUserRegistrationEnabled;
    bool8_t                     mAutoClubRegistrationEnabled;
    uint32_t                    mDbId;

    OSDKSeasonalPlayTimeUtils   mTimeUtils;

    //! Health monitoring statistics.
    SlaveMetrics mMetrics;

    //! Store the old period ids since after onSetPeriodIdsNotification is called, the RPC returns the new ids
    Blaze::Stats::PeriodIds mOldPeriodIds;
    
    SeasonConfigurationServerMap mSeasonConfigurationServerMap;
    AwardConfigurationMap mAwardConfigurationMap;

    TaskSchedulerSlaveImpl *mScheduler;
    
    static const char8_t* OSDKSP_SEASON_ROLLOVER_TASKNAME_PREFIX;
};

} // OSDKSeasonalPlay
} // Blaze

#endif // BLAZE_OSDKSEASONALPLAY_SLAVEIMPL_H

