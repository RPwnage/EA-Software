/*************************************************************************************************/
/*!
    \file   osdkseasonalplayslaveimpl.cpp

    $Header: //gosdev/games/Ping/V8_Gen4/ML/blazeserver/15.x/customcomponent/osdkseasonalplay/osdkseasonalplayslaveimpl.cpp#6 $
    $Change: 1204447 $
    $DateTime: 2016/06/22 06:45:38 $

    \attention
    (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class OSDKSeasonalPlaySlaveImpl

    OSDKSeasonalPlay Slave implementation.

    \note

*/
/*************************************************************************************************/

// TODO - need comment headers for the methods that describe what they do and any needed reasoning behind them

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "osdkseasonalplayslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/database/dbscheduler.h"
#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

#include "component/stats/statsconfig.h"
#include "component/stats/rpc/statsslave.h"
#include "component/stats/rpc/statsmaster.h"

// osdkseasonalplay includes
#include "osdkseasonalplay/osdkseasonalplaydb.h"

namespace Blaze
{
namespace OSDKSeasonalPlay
{

// static
const char8_t* OSDKSeasonalPlaySlaveImpl::OSDKSP_SEASON_ROLLOVER_TASKNAME_PREFIX = "OSDKSP_SEASON_ROLLOVER_TASK";

OSDKSeasonalPlaySlave* OSDKSeasonalPlaySlave::createImpl()
{
    return BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "OSDKSeasonalPlaySlaveImpl") OSDKSeasonalPlaySlaveImpl();
}


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief  constructor

    

*/
/*************************************************************************************************/
OSDKSeasonalPlaySlaveImpl::OSDKSeasonalPlaySlaveImpl() :
    mAutoUserRegistrationEnabled(false),
    mAutoClubRegistrationEnabled(false),
    mDbId(DbScheduler::INVALID_DB_ID),
    mMetrics(),
    mOldPeriodIds(),
    mScheduler(NULL)
{
}


/*************************************************************************************************/
/*!
    \brief  destructor

    

*/
/*************************************************************************************************/
OSDKSeasonalPlaySlaveImpl::~OSDKSeasonalPlaySlaveImpl()
{
    {
        SeasonConfigurationServerMap::const_iterator itr = mSeasonConfigurationServerMap.begin();
        SeasonConfigurationServerMap::const_iterator end = mSeasonConfigurationServerMap.end();
        for (; itr != end; ++itr)
        {
            delete itr->second;
        }
        mSeasonConfigurationServerMap.clear();
    }

    {
        AwardConfigurationMap::const_iterator itr = mAwardConfigurationMap.begin();
        AwardConfigurationMap::const_iterator end = mAwardConfigurationMap.end();
        for (; itr != end; ++itr)
        {
            delete itr->second;
        }
        mAwardConfigurationMap.clear();
    }
}


/*************************************************************************************************/
/*!
    \brief  getDbName

    gets the name of the database seasonal play should use

*/
/*************************************************************************************************/
const char8_t* OSDKSeasonalPlaySlaveImpl::getDbName() const
{
    const char8_t* dbName = getConfig().getDbName();

    if ('\0' == *dbName)
    {
        dbName = "main";
    }

    return dbName;
}


/*************************************************************************************************/
/*!
    \brief  registerClub

    registers the club in the clubs based season that matches the league id

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::registerClub(MemberId clubId, LeagueId leagueId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].registerClub. clubId = " << clubId <<
              ", leagueId = " << leagueId << "");

    BlazeRpcError error = Blaze::ERR_OK;

    if (false == mAutoClubRegistrationEnabled)
    {
        // automatic club registration is disabled. Nothing to do
        return error;
    }

    // find the season that corresponds to the club's league id;
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfigurationForLeague(leagueId, SEASONALPLAY_MEMBERTYPE_CLUB);

    // only register the club if a season for the club was found
    if (NULL != pSeasonConfig)
    {
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].registerClub. Registering Club in season. Club id = "
                  << clubId << ", season id = " << pSeasonConfig->getSeasonId() << "");

        // establish a database connection
        OSDKSeasonalPlayDb dbHelper(getDbId());
        error = dbHelper.getBlazeRpcError();
        if(Blaze::ERR_OK != error)
        {
            ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].registerClub: error " <<
                ErrorHelp::getErrorName(dbHelper.getBlazeRpcError()) << " - " << ErrorHelp::getErrorDescription(dbHelper.getBlazeRpcError()));
            return error;
        }

        error = registerForSeason(dbHelper, clubId, SEASONALPLAY_MEMBERTYPE_CLUB, leagueId, pSeasonConfig->getSeasonId());
    }
    else
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].registerClub. Unable to find a season to register the club in. clubId = " << clubId <<
                ", leagueId = " << leagueId); 
        error = OSDKSEASONALPLAY_ERR_INVALID_PARAMS;
    }

    return error;
}


/*************************************************************************************************/
/*!
    \brief  updateClubRegistration

    updates a club's registration to put the club in the season that matches the new
    league id. To be used when the club changes leagues

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::updateClubRegistration(MemberId clubId, Blaze::OSDKSeasonalPlay::LeagueId newLeagueId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].updateClubRegistration. clubId = "
              << clubId << ", leagueId = " << newLeagueId << "");

    BlazeRpcError error = Blaze::ERR_OK;

    // find the season that corresponds to the new league id
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfigurationForLeague(newLeagueId, SEASONALPLAY_MEMBERTYPE_CLUB);
    if (NULL == pSeasonConfig)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].updateClubRegistration. Unable to find a season to register the club in. clubId = "
                << clubId << ", leagueId = " << newLeagueId); 
        error = OSDKSEASONALPLAY_ERR_CONFIGURATION_ERROR;
        return error;
    }

    // establish a database connection
    OSDKSeasonalPlayDb dbHelper(getDbId());
    error = dbHelper.getBlazeRpcError();
    if(Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].updateClubRegistration: error " <<
            ErrorHelp::getErrorName(dbHelper.getBlazeRpcError()) << " - " << ErrorHelp::getErrorDescription(dbHelper.getBlazeRpcError()));
        return error;
    }

    // get the season that the club is registered in
    SeasonId registeredInSeasonId;
    error = dbHelper.fetchSeasonId(clubId, SEASONALPLAY_MEMBERTYPE_CLUB, registeredInSeasonId);

    if (Blaze::OSDKSEASONALPLAY_ERR_NOT_REGISTERED == error)
    {
        // club is not registered in a season so proceed as if this is a new registration
        error = registerForSeason(dbHelper, clubId, SEASONALPLAY_MEMBERTYPE_CLUB, newLeagueId, pSeasonConfig->getSeasonId());
    }
    else 
    {
        // update the club's registration
        error = dbHelper.updateRegistration(clubId, SEASONALPLAY_MEMBERTYPE_CLUB, pSeasonConfig->getSeasonId(), newLeagueId);

        // leave the existing tournament
        const SeasonConfigurationServer *pOldSeasonConfig = getSeasonConfiguration(registeredInSeasonId);
        if (NULL != pOldSeasonConfig)
        {
            TournamentId oldTournamentId = pOldSeasonConfig->getTournamentId();
            onLeaveTournament(clubId, SEASONALPLAY_MEMBERTYPE_CLUB, oldTournamentId);
        }

        // record metrics
        mMetrics.mClubRegistrationUpdates.add(1);
    }
    return error;
}


/*************************************************************************************************/
/*!
    \brief  registerForSeason

    registers a member in the specified season

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::registerForSeason(OSDKSeasonalPlayDb& dbHelper, MemberId memberId, MemberType memberType, LeagueId leagueId, SeasonId seasonId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].registerForSeason. memberId = " << memberId <<
              ", memberType = " << memberType << ", leagueId = " << leagueId << ", seasonId = " << seasonId << "");

    BlazeRpcError error = Blaze::ERR_OK;

    // check if the member is already registered in a season
    SeasonId registeredInSeasonId;
    error = dbHelper.fetchSeasonId(memberId, memberType, registeredInSeasonId);
    if (Blaze::OSDKSEASONALPLAY_ERR_NOT_REGISTERED != error)
    {
        error = Blaze::OSDKSEASONALPLAY_ERR_ALREADY_REGISTERED;
        return error;
    }

    // check that the season exists and the member type and league id match what is configured for the season
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL == pSeasonConfig || memberType != pSeasonConfig->getMemberType() || leagueId != pSeasonConfig->getLeagueId())
    {
        error = Blaze::OSDKSEASONALPLAY_ERR_INVALID_PARAMS;
        return error;
    }

    // register the member in the season
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].registerForSeason. Registering member in season. Member id = "
              << memberId << ", season id = " << seasonId << "");
    error = dbHelper.insertRegistration(memberId, memberType, seasonId, leagueId);   
    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].registerForSeason. Error registering member in season. Member id = "
                << memberId << ", season id = " << seasonId);
    }
    else
    {
        // log metrics
        if (SEASONALPLAY_MEMBERTYPE_CLUB == memberType)
        {
            mMetrics.mClubRegistrations.add(1);
        }
        else if (SEASONALPLAY_MEMBERTYPE_USER == memberType)
        {
            mMetrics.mUserRegistrations.add(1);
        }
    }

    return error;
}

/*************************************************************************************************/
/*!
    \brief  deleteRegistration

    delete the entity from the registration

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::deleteRegistration(MemberId memberId, MemberType memberType)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].deleteRegistration. memberId = " << memberId << "memberType = " << memberType << "");

    BlazeRpcError error = Blaze::ERR_OK;

    OSDKSeasonalPlayDb dbHelper(getDbId());
    error = dbHelper.getBlazeRpcError();
    if(Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].deleteRegistration: error " <<
            ErrorHelp::getErrorName(dbHelper.getBlazeRpcError()) << " - " << ErrorHelp::getErrorDescription(dbHelper.getBlazeRpcError()));
    }
    else
    {
        // delete the registration
        error = dbHelper.deleteRegistration(memberId, memberType);

        if (Blaze::ERR_OK != error)
        {
            ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].deleteRegistration. Error deleting registration. memberId = " << memberId << "memberType = " << memberType << "");
        }
        else
        {
            // log metrics
            if (SEASONALPLAY_MEMBERTYPE_CLUB == memberType)
            {
                mMetrics.mClubRegistrationsDeleted.add(1);
            }
            else if (SEASONALPLAY_MEMBERTYPE_USER == memberType)
            {
                mMetrics.mUserRegistrationsDeleted.add(1);
            }
        }
    }

    return error;
}

/*************************************************************************************************/
/*!
    \brief  getSeasonStartTime

    the time stamp of when the current season started

*/
/*************************************************************************************************/
int64_t OSDKSeasonalPlaySlaveImpl::getSeasonStartTime(SeasonId seasonId)
{
    int64_t iStartTime = 0;

    // get the period type for the season
    Stats::StatPeriodType periodType = getSeasonPeriodType(seasonId);
    if (Stats::STAT_PERIOD_ALL_TIME == periodType)
    {
        // have an invalid period type for the season
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getSeasonStartTime(): invalid season stat period type for season id " << seasonId);
        return iStartTime;
    }

    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL == pSeasonConfig)
    {
        // have an invalid period type for the season
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getSeasonStartTime(): unable to retrieve configuration for season id " << seasonId);
        return iStartTime;
    }

    // the season start time is the same as the previous rollover time + blackout
    iStartTime = mTimeUtils.getPreviousRollOverTime(periodType) + pSeasonConfig->getBlackOutDuration();

    return iStartTime;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonEndTime

    the time stamp of when the current season ends

*/
/*************************************************************************************************/
int64_t OSDKSeasonalPlaySlaveImpl::getSeasonEndTime(SeasonId seasonId)
{
    int64_t iEndTime = 0;

    // get the period type for the season
    Stats::StatPeriodType periodType = getSeasonPeriodType(seasonId);
    if (Stats::STAT_PERIOD_ALL_TIME == periodType)
    {
        // have an invalid period type for the season
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getSeasonEndTime(): invalid season stat period type for season id " << seasonId);
        return iEndTime;
    }

    // the season end time is the same as the rollover time
    iEndTime = mTimeUtils.getRollOverTime(periodType);

    return iEndTime;
}


/*************************************************************************************************/
/*!
    \brief  getPlayoffStartTime

    the time stamp of when the playoffs begin in the current season

*/
/*************************************************************************************************/
int64_t OSDKSeasonalPlaySlaveImpl::getPlayoffStartTime(SeasonId seasonId)
{
    int64_t iStartTime = 0;

    // get the period type for the season
    Stats::StatPeriodType periodType = getSeasonPeriodType(seasonId);
    if (Stats::STAT_PERIOD_ALL_TIME == periodType)
    {
        // have an invalid period type for the season
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getPlayoffStartTime(): invalid season stat period type for season id " << seasonId);
        return iStartTime;
    }

    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL == pSeasonConfig)
    {
        // have an invalid period type for the season
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getPlayoffStartTime(): unable to retrieve configuration for season id " << seasonId);
        return iStartTime;
    }


    // the playoff start time is the season end time - the playoff duration
    iStartTime = mTimeUtils.getRollOverTime(periodType) - pSeasonConfig->getPlayoffDuration();

    return iStartTime;
}


/*************************************************************************************************/
/*!
    \brief  getPlayoffEndTime

    the time stamp of when the playoffs end for the current season

*/
/*************************************************************************************************/
int64_t OSDKSeasonalPlaySlaveImpl::getPlayoffEndTime(SeasonId seasonId)
{
    // playoff end time is the same as the season end time
    return getSeasonEndTime(seasonId);
}


/*************************************************************************************************/
/*!
    \brief  getRegularSeasonEndTime

    the time stamp of when the regular season ends for the current season

*/
/*************************************************************************************************/
int64_t OSDKSeasonalPlaySlaveImpl::getRegularSeasonEndTime(SeasonId seasonId)
{
    // regular season end time is the same as the playoff start time
    return getPlayoffStartTime(seasonId);
}


/*************************************************************************************************/
/*!
    \brief  getNextSeasonRegularSeasonStartTime

    the time stamp of when the next regular season starts

*/
/*************************************************************************************************/
int64_t OSDKSeasonalPlaySlaveImpl::getNextSeasonRegularSeasonStartTime(SeasonId seasonId)
{
    int64_t iStartTime = 0;

    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL == pSeasonConfig)
    {
        // have an invalid period type for the season
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getNextSeasonRegularSeasonStartTime(): unable to retrieve configuration for season id " << seasonId);
        return iStartTime;
    }

    // the start time of the next regular season is the current season's end time + the black out period
    iStartTime = getSeasonEndTime(seasonId) + pSeasonConfig->getBlackOutDuration();

    return iStartTime;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonState

    gets the current state of the season

*/
/*************************************************************************************************/
SeasonState OSDKSeasonalPlaySlaveImpl::getSeasonState(SeasonId seasonId)
{
    SeasonState seasonState = SEASONALPLAY_SEASON_STATE_NONE;

    int64_t iCurrentTime = mTimeUtils.getCurrentTime();
    mTimeUtils.logTime("[getSeasonState] current Time", iCurrentTime);

    // note - season times will "reset" at the end of the season which is the
    //        playoff end time (season rollover). As such a new season starts
    //        with the black out period, then regular season and then playoffs

    if (getSeasonStartTime(seasonId) <= iCurrentTime &&
        getRegularSeasonEndTime(seasonId) > iCurrentTime)
    {
        // regular season
        seasonState = SEASONALPLAY_SEASON_STATE_REGULARSEASON;
    }
    else if (getPlayoffStartTime(seasonId) <= iCurrentTime &&
        getPlayoffEndTime(seasonId) > iCurrentTime)
    {
        // playoffs
        seasonState = SEASONALPLAY_SEASON_STATE_PLAYOFF;
    }
    else if (getSeasonStartTime(seasonId) > iCurrentTime)
    {
        // blackout
        seasonState = SEASONALPLAY_SEASON_STATE_BLACKOUT;
    }
    else
    {
        // something's gone wrong in determining season state
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getSeasonState(): unable to determine season state for season id " << seasonId);
    }

    return seasonState;
}


/*************************************************************************************************/
/*!
    \brief  isValidSeason

    validates that the season id is for a valid configured season

*/
/*************************************************************************************************/
bool8_t OSDKSeasonalPlaySlaveImpl::isValidSeason(SeasonId seasonId)
{
    bool8_t bValid = false;

    SeasonConfigurationServerMap::const_iterator it = mSeasonConfigurationServerMap.find(seasonId);

    if (it != mSeasonConfigurationServerMap.end())
    {
        // got a valid season
        bValid = true;
    }

    return bValid;
}

/*************************************************************************************************/
/*!
    \brief  getMemberTournamentStatus

    gets the tournament status of the member

*/
/*************************************************************************************************/
TournamentStatus OSDKSeasonalPlaySlaveImpl::getMemberTournamentStatus(OSDKSeasonalPlayDb& dbHelper, MemberId memberId, MemberType memberType, BlazeRpcError &outError)
{
    TournamentStatus tournStatus;
    outError = dbHelper.fetchTournamentStatus(memberId, memberType, tournStatus);

    // record metrics
    mMetrics.mGetTournamentStatusCalls.add(1);

    return tournStatus;
}


/*************************************************************************************************/
/*!
    \brief  setMemberTournamentStatus

    sets the tournament status of the member. Intended to be called when a member has won
    or has been eliminated from a tournament.

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::setMemberTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus tournamentStatus)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].setMemberTournamentStatus. memberId = " << memberId <<
              ", memberType = " << MemberTypeToString(memberType) << ", status = " << TournamentStatusToString(tournamentStatus) << "");

    BlazeRpcError error = Blaze::ERR_OK;

    OSDKSeasonalPlayDb dbHelper(getDbId());

    // get the existing tournament status as we only want to update if it hasn't changed
    TournamentStatus existingStatus;
    error = dbHelper.fetchTournamentStatus(memberId, memberType, existingStatus);
    if (Blaze::ERR_OK == error)
    {
        if (tournamentStatus != existingStatus)
        {
            error = dbHelper.updateMemberTournamentStatus(memberId, memberType, tournamentStatus);
            if (Blaze::ERR_OK == error)
            {
                // invoke the custom processing hook for tournament status updates
                error = onTournamentStatusUpdate(dbHelper, memberId, memberType, tournamentStatus);

                // record metrics
                mMetrics.mSetTournamentStatusCalls.add(1);
            }
            else
            {
                ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].setMemberTournamentStatus. Error updating tournament status. memberId = " << memberId <<
                        ", memberType = " << MemberTypeToString(memberType) << ", status = " << TournamentStatusToString(tournamentStatus));
            }
        }
    }
    else
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].setMemberTournamentStatus. Error retrieving existing tournament status for member. memberId = " << memberId <<
                ", memberType = " << MemberTypeToString(memberType));
    }

    return error;
}


/*************************************************************************************************/
/*!
    \brief  resetSeasonTournamentStatus

    resets the tournament status of all members in the season to "SEASONALPLAY_TOURNAMENT_STATUS_NONE"

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::resetSeasonTournamentStatus(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].resetSeasonTournamentStatus. seasonId = " << seasonId << "");

    BlazeRpcError error = Blaze::ERR_OK;

    error = dbHelper.updateSeasonTournamentStatus(seasonId, SEASONALPLAY_TOURNAMENT_STATUS_NONE);

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].resetSeasonTournamentStatus. Error reseting season tournament status. seasonId = " << seasonId);
    }
    else
    {
        // record metrics
        mMetrics.mResetTournamentStatusCalls.add(1);
    }

    return error;
}


/*************************************************************************************************/
/*!
    \brief  getIsTournamentEligible

    gets the member's tournament eligibility. Takes into account the current season state,
    the member's division, the member's tournament status and the division's tournament rule

*/
/*************************************************************************************************/
bool8_t OSDKSeasonalPlaySlaveImpl::getIsTournamentEligible(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId, MemberId memberId, MemberType memberType, BlazeRpcError &outError)
{
    bool8_t eligible = false;
    outError = ERR_OK;

    // get the season state
    SeasonState seasonState = getSeasonState(seasonId);

    // continue if the season state is in playoffs
    if (SEASONALPLAY_SEASON_STATE_PLAYOFF == seasonState)
    {
        // get the division the member is in. 
        uint32_t divisionRank = 0, divisionStartRank = 0, overallRank = 0; // these var are ignored
        uint8_t divisionNumber = 0;
        outError = calculateRankAndDivision(seasonId, memberId, divisionRank, divisionNumber, divisionStartRank, overallRank);
        if (Blaze::ERR_OK != outError)
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getIsTournamentEligible. Unable to calculate the division for member: memberId = " << memberId <<
                     ", memberType = " << MemberTypeToString(memberType) << ", error = " << Blaze::ErrorHelp::getErrorName(outError) << "");
            return eligible;
        }

        const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
        if (NULL == pSeasonConfig)
        {
            outError = OSDKSEASONALPLAY_ERR_SEASON_NOT_FOUND;
            return eligible;
        }

        // check playoff eligibility against the season requiring a regular season game to have been played
        // a regular season game is considered to have been played IF the overallRank is not 0
        if (true == pSeasonConfig->getPlayoffsRequireRegularSeasonGame() && 0 == overallRank)
        {
            // Entities with no games played while REGULAR season state should be prevented for playoffs of the current period
            // https://easojira.ea.com/browse/OSDKV5-10512
            BlazeRpcError error = setMemberTournamentStatus(memberId, memberType, SEASONALPLAY_TOURNAMENT_STATUS_PROHIBITED);
            if (Blaze::ERR_OK != error)
            {
                ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getIsTournamentEligible. Error setting tournament status. Member id = "
                    << memberId << ", Member type = " << memberType);
            }

            // a regular season game has not been played and the season requires it for playoffs
            return eligible;  // eligible is still false here
        }

        // if the division number is 0, could not determine which division the user is in so default them to the last division
        if (0 == divisionNumber)
        {
            const SeasonConfigurationServer::DivisionList &divisionConfig = pSeasonConfig->getDivisions();
            SeasonConfigurationServer::DivisionList::const_iterator divItr = divisionConfig.begin();
            SeasonConfigurationServer::DivisionList::const_iterator divItr_end = divisionConfig.end();
            for ( ; divItr != divItr_end; ++divItr)
            {
                const Division *division = *divItr;
                if (division->getNumber() > divisionNumber)
                {
                    divisionNumber = division->getNumber();
                }
            }
        }

        TournamentRule tournRule = getTournamentRule(seasonId, divisionNumber);

        TournamentStatus memberTournStatus = getMemberTournamentStatus(dbHelper, memberId, memberType, outError);
        if (Blaze::ERR_OK == outError)
        {
            if (SEASONALPLAY_TOURNAMENT_STATUS_PROHIBITED == memberTournStatus)
            {
                // entity was not qualified for playoffs
                eligible = false;
            }
            else if (SEASONALPLAY_TOURNAMENTRULE_UNLIMITED == tournRule)
            {
                // unlimited attempts
                eligible = true;
            }
            else if (SEASONALPLAY_TOURNAMENTRULE_RETRY_ON_LOSS == tournRule)
            {
                // eligible if status is not won
                eligible = (SEASONALPLAY_TOURNAMENT_STATUS_WON != memberTournStatus) ? true : false;
            }
            else if (SEASONALPLAY_TOURNAMENTRULE_ONE_ATTEMPT == tournRule)
            {
                // eligible if status is none
                eligible = (SEASONALPLAY_TOURNAMENT_STATUS_NONE == memberTournStatus) ? true : false;
            }
        }
        else
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getIsTournamentEligible. Unable to retrieve tournament status for member: memberId = " << memberId <<
                     ", memberType = " << MemberTypeToString(memberType) << ", error = " << Blaze::ErrorHelp::getErrorName(outError) << "");
        }
    }

    return eligible;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonTournamentId

    gets the tournament id associated with the season

*/
/*************************************************************************************************/
TournamentId OSDKSeasonalPlaySlaveImpl::getSeasonTournamentId(SeasonId seasonId)
{
    TournamentId tournId = 0;
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);

    if (NULL != pSeasonConfig)
    {
        tournId = pSeasonConfig->getTournamentId();
    }

    return tournId;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonStatsCategory

    gets the name of the stats category that's associated with the season

*/
/*************************************************************************************************/
const char8_t* OSDKSeasonalPlaySlaveImpl::getSeasonStatsCategory(SeasonId seasonId)
{
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);

    if (NULL != pSeasonConfig)
    {
        return pSeasonConfig->getStatCategoryName();
    }
    else
    {
        return NULL;
    }
}


/*************************************************************************************************/
/*!
    \brief  getSeasonLeaderboardName

    gets the name of the leaderboard associated with the season

*/
/*************************************************************************************************/
const char8_t* OSDKSeasonalPlaySlaveImpl::getSeasonLeaderboardName(SeasonId seasonId)
{
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);

    if (NULL != pSeasonConfig)
    {
        return pSeasonConfig->getLeaderboardName();
    }
    else
    {
        return NULL;
    }

}


/*************************************************************************************************/
/*!
    \brief  calculateRankAndDivision

    for the specified member, calculates the overall rank, division, rank within the division 
    and the overall rank that the member's division starts at. Defaults to the current season
    but can do the calculation the previous season. Based on the regular season leaderboard

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::calculateRankAndDivision(SeasonId seasonId, MemberId memberId, uint32_t &outDivisionRank, uint8_t &outDivision, uint32_t &outDivisionStartingRank, uint32_t &outOverallRank, bool8_t fromPreviousSeason)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].calculateRankAndDivision: seasonId = "
              << seasonId << ", memberId = " << memberId << "");

    // record metrics - don't care about success/failure, just that it was attempted
    mMetrics.mRankDivCalcs.add(1);

    BlazeRpcError error = Blaze::ERR_OK;

    outDivisionRank = 0;
    outDivision = 0;
    outDivisionStartingRank = 0;
    outOverallRank = 0;

    // get the configuration for the season
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL == pSeasonConfig)
    {
        error = OSDKSEASONALPLAY_ERR_SEASON_NOT_FOUND;
        return error;
    }

    // setup the stats period offset to use. 0 = current season, 1 = previous season
    int32_t statsPeriodOffset = (true == fromPreviousSeason) ? 1 : 0;

    // get the number of ranked members
    uint32_t rankedMemberCount = getRankedMemberCount(seasonId, fromPreviousSeason);

    // if there are no entities in the leaderboard, bail out now since nobody is ranked
    if (0 == rankedMemberCount)
    {
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].calculateRankAndDivision: Leaderboard "
                  << pSeasonConfig->getLeaderboardName() << "... nothing to calculate since no entities ranked");
        return error;
    }

    // the the stats slave
    Stats::StatsSlave* statsComponent
        = static_cast<Stats::StatsSlave*>(
        gController->getComponent(Stats::StatsSlave::COMPONENT_ID, false));
    if (NULL == statsComponent)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].calculateRankAndDivision - Stats component not found.");
        error = OSDKSEASONALPLAY_ERR_GENERAL;
        return error;
    }

    // find out if the leaderboard has a keyscope.
    bool8_t leaderBoardHasScope = getLeaderboardHasScope(pSeasonConfig->getLeaderboardName());

    // now get the filtered leaderboard for the member so we can get the rank
    Stats::FilteredLeaderboardStatsRequest filteredLBRequest;
    filteredLBRequest.setBoardName(pSeasonConfig->getLeaderboardName());
    filteredLBRequest.setPeriodOffset(statsPeriodOffset);
    filteredLBRequest.getListOfIds().push_back(memberId);
    if (true == leaderBoardHasScope)
    {
        Stats::ScopeName scopeName = "season";
        Stats::ScopeValue scopeValue = seasonId;
        filteredLBRequest.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(scopeName, scopeValue));
    }
    Stats::LeaderboardStatValues filteredLBResponse;

    // to invoke this RPC will require authentication
    UserSession::pushSuperUserPrivilege();
    error = statsComponent->getFilteredLeaderboard(filteredLBRequest, filteredLBResponse);
    UserSession::popSuperUserPrivilege();

    if (Blaze::ERR_OK != error)
    {
        WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].calculateRankAndDivision(): Failed to retrieve filtered leaderboard:  leaderboard = "
                 << pSeasonConfig->getLeaderboardName() << ", memberId = " << memberId);
        return error;
    }

    Stats::LeaderboardStatValues::RowList::const_iterator rowItr = filteredLBResponse.getRows().begin();
    Stats::LeaderboardStatValues::RowList::const_iterator rowItr_end = filteredLBResponse.getRows().end();
    for ( ; rowItr != rowItr_end; ++rowItr)
    {
        const Stats::LeaderboardStatValuesRow *lbRow = *rowItr;

        if (lbRow->getUser().getBlazeId() == memberId)
        {
            outOverallRank = lbRow->getRank();
        }
    }

    // if the member does not have an overall rank, then bail out since there is nothing to calculate
    if (0 == outOverallRank)
    {
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].calculateRankAndDivision: Member not ranked in leaderboard, nothing to calculate.  leaderboard = "
                  << pSeasonConfig->getLeaderboardName() << ", memberId = " << memberId << "");
        return error;
    }

    // get the division starting ranks
    eastl::vector<uint32_t> divisionStartingRanks;
    uint8_t divisionCount = 0;
    calculateDivisionStartingRanks(seasonId, rankedMemberCount, divisionStartingRanks, divisionCount);

    // now find out what division the current user is in based on the division starting ranks
    for (uint8_t index = 0; index < divisionCount; ++index)
    {
        uint32_t currDivStartRank = divisionStartingRanks[index];
        uint32_t nextDivStartRank = 0;
        
        if (index == (divisionCount - 1))
        {
            // current is the last division so next div start = member count + 1
            nextDivStartRank = rankedMemberCount + 1;
        }
        else if (divisionStartingRanks[index+1] == 0)
        {
            // next division doesn't have a starting rank, so next div start = member count + 1
            nextDivStartRank = rankedMemberCount + 1;
        }
        else
        {
            nextDivStartRank = divisionStartingRanks[index+1];
        }

        if (currDivStartRank <= outOverallRank && outOverallRank < nextDivStartRank)
        {
            // the member is in the current division
            outDivision = index+1;
            outDivisionStartingRank = divisionStartingRanks[index];
            outDivisionRank = outOverallRank - outDivisionStartingRank + 1;
            break;
        }
    }

    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].calculateRankAndDivision: Leaderboard " << pSeasonConfig->getLeaderboardName() <<
              ", entity count = " << rankedMemberCount << ", overall rank = " << outOverallRank << ", division = " << outDivision << 
              ", division rank = " << outDivisionRank << ", division starting rank = " << outDivisionStartingRank << "");

    return error;
}


/*************************************************************************************************/
/*!
    \brief  getRankedMemberCount

    gets the number of members ranked in the regular season leaderboard for the season. Defaults
    to the current season.

*/
/*************************************************************************************************/
uint32_t OSDKSeasonalPlaySlaveImpl::getRankedMemberCount(SeasonId seasonId, bool8_t fromPreviousSeason)
{
    uint32_t count = 0;

    // get the configuration for the season
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL == pSeasonConfig)
    {
        return count;   // early return
    }

    // the the stats slave
    Stats::StatsSlave* statsComponent
        = static_cast<Stats::StatsSlave*>(
        gController->getComponent(Stats::StatsSlave::COMPONENT_ID,false));
    if (NULL == statsComponent)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getRankedMemberCount - Stats component not found.");
        return count;   // early return
    }

    // find out if the leaderboard has a keyscope.
    bool8_t leaderBoardHasScope = getLeaderboardHasScope(pSeasonConfig->getLeaderboardName());

    // setup the stats period offset to use. 0 = current season, 1 = previous season
    int32_t statsPeriodOffset = (true == fromPreviousSeason) ? 1 : 0;

    // get the total entity count
    Stats::LeaderboardEntityCountRequest entityCountReq;
    entityCountReq.setBoardName(pSeasonConfig->getLeaderboardName());
    entityCountReq.setPeriodOffset(statsPeriodOffset);
    if (true == leaderBoardHasScope)
    {
        Stats::ScopeName scopeName = "season";
        Stats::ScopeValue scopeValue = seasonId;
        entityCountReq.getKeyScopeNameValueMap().insert(Stats::ScopeNameValueMap::value_type(scopeName, scopeValue));
    }
    Stats::EntityCount entityCountResp;

    // to invoke this RPC will require authentication
    UserSession::pushSuperUserPrivilege();
    BlazeRpcError error = statsComponent->getLeaderboardEntityCount(entityCountReq, entityCountResp);
    UserSession::popSuperUserPrivilege();

    if (Blaze::ERR_OK != error && Blaze::STATS_ERR_DB_DATA_NOT_AVAILABLE != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getRankedMemberCount(): Failed to retrieve leaderboard entity count for season = " << seasonId <<
                ", leaderboard = " << pSeasonConfig->getLeaderboardName());
    }
    else
    {
        count = entityCountResp.getCount();
    }

    return count;
}


/*************************************************************************************************/
/*!
    \brief  calculateDivisionStartingRanks

    calculates the overall rank that each division starts at. 

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::calculateDivisionStartingRanks(SeasonId seasonId, uint32_t lbEntityCount, eastl::vector<uint32_t> &outStartingRanks, uint8_t &outDivisionCount)
{
    // TODO - division starting ranks would be ideal to be cached

    // record metrics - don't care about success/failure, just that it was attempted
    mMetrics.mDivStartingRankCalcs.add(1);

    // get the configuration for the season
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL == pSeasonConfig)
    {
        return;
    }
	
	// get list of divisions from the season config.
	const SeasonConfigurationServer::DivisionList &divisionConfig = pSeasonConfig->getDivisions();
	

	outDivisionCount = static_cast<uint8_t>(divisionConfig.size());

	TRACE_LOG("OSDKSeasonalPlaySlaveImpl::calculateDivisionStartingRanks - outDivisionCount - "<<outDivisionCount<<" ");

	//--------------------
	// FIFA is not using the percentage base division, but a fixed size division
	// apoon@ea.com May 6 2010
	//--------------------
		// get the size percentages for each division. Don't assume that the divisions are in order in the config
		//eastl::vector<uint32_t>  divisionPercentages(outDivisionCount);    // vector of the division size percentages. Index 0 = division 1, index 1 = division 2, etc
		//SeasonConfigurationServer::DivisionList::const_iterator divItr = divisionConfig.begin();
		//SeasonConfigurationServer::DivisionList::const_iterator divItr_end = divisionConfig.end();
		//for ( ; divItr != divItr_end; ++divItr)
		//{
		//	Division *division = *divItr;
		//	divisionPercentages[division->getNumber()-1] = division->getSize();
		//	DEBUG3("OSDKSeasonalPlaySlaveImpl::calculateDivisionStartingRanks - divisionPercentages[division->getNumber()-1] - %d", fixedDivisionSize[division->getNumber()-1]);
		//}
	eastl::vector<uint32_t>  fixedDivisionSize(outDivisionCount);    // vector of the division size Index 0 = division 1, index 1 = division 2, etc
	SeasonConfigurationServer::DivisionList::const_iterator divItr = divisionConfig.begin();
	SeasonConfigurationServer::DivisionList::const_iterator divItr_end = divisionConfig.end();
	for ( ; divItr != divItr_end; ++divItr)
	{
		const Division *division = *divItr;
		fixedDivisionSize[division->getNumber()-1] = division->getSize();
		TRACE_LOG("OSDKSeasonalPlaySlaveImpl::calculateDivisionStartingRanks - fixedDivisionSize[division->getNumber()-1] - "<<fixedDivisionSize[division->getNumber()-1]<<" ");
	}
	
    // now calculate the starting ranks for each division
    outStartingRanks.resize(outDivisionCount);
    outStartingRanks[0] = 1; // first division has a starting rank of 1.
	
    for (uint32_t index = 1; index < outDivisionCount; ++index)
    {
		//--------------------
		// FIFA is not using the percentage base division, but a fixed size division
		// apoon@ea.com May 6 2010
		//--------------------

			//if (lbEntityCount > outDivisionCount)
			//{
				// divisions starting rank = (leaderboard entity count) * (size of previous division percentage size) * 0.01 + 0.5
				// the + 0.5 is for rounding so that when we do the static cast, don't end up rounding down when we should be rounding up
				//uint32_t previousDivSize = divisionPercentages[index-1];
				//uint32_t currentDivStartingRank = static_cast<uint32_t>((lbEntityCount * previousDivSize * 0.01) + 0.5) + outStartingRanks[index-1];
				//outStartingRanks[index] = currentDivStartingRank;
			//}
			//else
			//{
				// not enough entities in the leaderboard to go beyond the first division
			//    outStartingRanks[index] = 0;
			//}
		outStartingRanks[index] = outStartingRanks[index-1] + fixedDivisionSize[index];
		TRACE_LOG("OSDKSeasonalPlaySlaveImpl::calculateDivisionStartingRanks - outStartingRanks "<<index<<" "<<outStartingRanks[index]<<" ");

    }
}


/*************************************************************************************************/
/*!
    \brief  getRegisteredMembers

    gets a list of all the members registered in the season

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::getRegisteredMembers(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId,  eastl::vector<MemberId> &outMemberList)
{
    BlazeRpcError error = dbHelper.fetchSeasonMembers(seasonId, outMemberList);
    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getRegisteredMembers. Error retrieving member list for season id " << seasonId <<
                ". Error = " << ErrorHelp::getErrorName(error));
    }
    return error;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonMemberType

    gets the member type associated with the season

*/
/*************************************************************************************************/
MemberType OSDKSeasonalPlaySlaveImpl::getSeasonMemberType(SeasonId seasonId)
{
    MemberType memberType = SEASONALPLAY_MEMBERTYPE_USER;

    // get the configuration for the season
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL != pSeasonConfig)
    {
        memberType = pSeasonConfig->getMemberType();
    }
    
    return memberType;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonLeagueId

    gets the league id for the season

*/
/*************************************************************************************************/
LeagueId OSDKSeasonalPlaySlaveImpl::getSeasonLeagueId(SeasonId seasonId)
{
    LeagueId leagueId = 0;

    // get the configuration for the season
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL != pSeasonConfig)
    {
        leagueId = pSeasonConfig->getLeagueId();        
    }

    return leagueId;
}



///////////////////////////////////////////////////////////
// Awards
///////////////////////////////////////////////////////////

/*************************************************************************************************/
/*!
    \brief  assignAward

    assigns an award to a member. Set the "historical" parameter to true for awards that have
    "historical" significance.

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::assignAward(OSDKSeasonalPlayDb& dbHelper, AwardId awardId, MemberId memberId, MemberType memberType, SeasonId seasonId, LeagueId leagueId, uint32_t seasonNumber, bool8_t historical, const char8_t *metadata)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].assignAward. awardId = " << awardId << ", memberId = " << memberId << ", memberType = " << MemberTypeToString(memberType) <<
              ", seasonId = " << seasonId << ", leagueId = " << leagueId << ", seasonNumber = " << seasonNumber << "");

    // assign the award
    BlazeRpcError error = dbHelper.insertAward(awardId, memberId, memberType, seasonId, leagueId, seasonNumber, historical, metadata);

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].assignAward. Error assigning award. awardId = " << awardId << ", memberId =" << memberId <<
                ", memberType = " << MemberTypeToString(memberType) << ", seasonId = " << seasonId << ", leagueId = " << leagueId << ", seasonNumber = " << seasonNumber);
    }
    else
    {
        // record metrics
        mMetrics.mTotalAwardsAssigned.add(1);
    }

    return error;
}


/*************************************************************************************************/
/*!
    \brief  updateAwardMember

    updates the member that an award is assigned to. Requires the "assignment id" of the award

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::updateAwardMember(OSDKSeasonalPlayDb& dbHelper, uint32_t assignmentId, MemberId memberId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].updateAwardMember. assignmentId = "
              << assignmentId << ", memberId = " << memberId << "");

    // update the award
    BlazeRpcError error = dbHelper.updateAwardMember(assignmentId, memberId);

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].updateAwardMember. Error updating award Member. assignmentId = "
                << assignmentId << ", memberId = " << memberId);
    }
    else
    {
        // record metrics
        mMetrics.mTotalAwardsUpdated.add(1);
    }

    return error;
}


/*************************************************************************************************/
/*!
    \brief  deleteAward

    deletes an award assignment

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::deleteAward(OSDKSeasonalPlayDb& dbHelper, uint32_t assignmentId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].deleteAward. assignmentId = " << assignmentId << "");

    // delete the award
    BlazeRpcError error = dbHelper.deleteAward(assignmentId);

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].updateAwardMember. Error deleting award. assignmentId = " << assignmentId);
    }
    else
    {
        // record metrics
        mMetrics.mTotalAwardsDeleted.add(1);
    }

    return error;
}


/*************************************************************************************************/
/*!
    \brief  getAwardsBySeason

    gets all the awards assigned in a season by season number. Season number can be a specific
    season number or can be set to AWARD_ANY_SEASON_NUMBER for all season numbers. Can also be
    filtered by awards that have "historical" significance.

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::getAwardsBySeason(OSDKSeasonalPlayDb& dbHelper, SeasonId seasonId, uint32_t seasonNumber, AwardHistoricalFilter historicalFilter, AwardList &outAwardList)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getAwardsBySeason. seasonId = " << seasonId << ", seasonNumber = "
              << seasonNumber << ", historicalFilter = " << static_cast<uint32_t>(historicalFilter) << "");

    // record metrics - don't care about success/failure, just that it was attempted
    mMetrics.mGetAwardsBySeasonCalls.add(1);

    BlazeRpcError error = dbHelper.fetchAwardsBySeason(seasonId, seasonNumber, historicalFilter, outAwardList);

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getAwardsBySeason. Error fetching awards. seasonId = " << seasonId << ", seasonNumber = "
                << seasonNumber << ", historicalFilter = " << static_cast<uint32_t>(historicalFilter) << "");
    }

    return error;
}


/*************************************************************************************************/
/*!
    \brief  getAwardsByMember

    gets the awards assigned to the member. season number can be set to AWARD_ANY_SEASON_NUMBER 
    to retrieve all the awards a member has won in a season regardless of season number

*/
/*************************************************************************************************/
BlazeRpcError OSDKSeasonalPlaySlaveImpl::getAwardsByMember(OSDKSeasonalPlayDb& dbHelper, MemberId memberId, MemberType memberType, SeasonId seasonId, uint32_t seasonNumber, AwardList &outAwardList)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getAwardsByMember. memberId = " << memberId << ", memberType = "
              << MemberTypeToString(memberType) << ", seasonId = " << seasonId << ", seasonNumber = " << seasonNumber << "");

    // record metrics - don't care about success/failure, just that it was attempted
    mMetrics.mGetAwardsByMemberCalls.add(1);

    BlazeRpcError error = dbHelper.fetchAwardsByMember(memberId, memberType, seasonId, seasonNumber, outAwardList);

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getAwardsByMember. Error fetching awards. memberId = " << memberId << ", memberType = "
                << MemberTypeToString(memberType) << ", seasonId = " << seasonId << ", seasonNumber = " << seasonNumber);
    }

    return error;
}

/*************************************************************************************************/
/*!
    \brief  onConfigure

    lifecycle method: called when the slave is to configure itself. registers for stats
    and usersession events. 

*/
/*************************************************************************************************/
bool OSDKSeasonalPlaySlaveImpl::onConfigure()
{
    bool success = configureHelper();

    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onConfigure: configuration " << (success ? "successful." : "failed."));
    if(false == success)
    {
        return false; // EARLY RETURN
    }

    mScheduler = static_cast<TaskSchedulerSlaveImpl*>
        (gController->getComponent(TaskSchedulerSlaveImpl::COMPONENT_ID, true, true));
    
    if (NULL == mScheduler)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onConfigure - TaskScheduler component not found.");
        return false; // EARLY RETURN
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief  onReconfigure

    lifecycle method: called when the slave is to reconfigure itself. unregisters from stats
    and usersession events before calling onConfigure

*/
/*************************************************************************************************/
bool OSDKSeasonalPlaySlaveImpl::onReconfigure()
{
    // check if auto user/club registration is enabled
    const OSDKSeasonalPlayConfig &config = getConfig();

    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onReconfigure: reconfiguration only support reloading autoUserRegistration and autoClubRegistration.");
    mAutoUserRegistrationEnabled = config.getAutoUserRegistration();
    mAutoClubRegistrationEnabled = config.getAutoClubRegistration();
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onReconfigure: automatic user registration set to " << (mAutoUserRegistrationEnabled ? "true" : "false") << "");
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onReconfigure: automatic club registration set to " << (mAutoClubRegistrationEnabled ? "true" : "false") << "");
    return true;
}


/*************************************************************************************************/
/*!
    \brief configureHelper

    Helper function which performs tasks common to onConfigure and onReconfigure

*/
/*************************************************************************************************/
bool8_t OSDKSeasonalPlaySlaveImpl::configureHelper()
{
    const char8_t* dbName = getDbName();
    mDbId = gDbScheduler->getDbId(dbName);
    if(DbScheduler::INVALID_DB_ID == mDbId)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].configureHelper: Failed to initialize db");
        return false;
    }

    OSDKSeasonalPlayDb::initialize(mDbId);
    
    if(false == parseConfig())
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].configureHelper: Failed to parse configuration");
        return false;
    }

    // check if auto user/club registration is enabled
    const OSDKSeasonalPlayConfig &config = getConfig();

    mAutoUserRegistrationEnabled = config.getAutoUserRegistration();
    mAutoClubRegistrationEnabled = config.getAutoClubRegistration();
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].configureHelper: automatic user registration set to " << (mAutoUserRegistrationEnabled ? "true" : "false") << "");
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].configureHelper: automatic club registration set to " << (mAutoClubRegistrationEnabled ? "true" : "false") << "");

    // hookup with StatsMasterListener event
    BlazeRpcError waitResult = ERR_OK;
    Stats::StatsMaster* statsComponent = static_cast<Stats::StatsMaster*>(gController->getComponent(Stats::StatsMaster::COMPONENT_ID, false, true, &waitResult));

    if (ERR_OK != waitResult)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].configureHelper - Stats component not found.");
        return false;
    }
    if (NULL != statsComponent)
    {
        statsComponent->addNotificationListener(*this);
        if (false == statsComponent->isLocal())
        {
            // subscribe for stats notifications
            BlazeRpcError subscribeError = statsComponent->notificationSubscribe();
            if (ERR_OK != subscribeError)
            {
                ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].configureHelper - Stats component couldn't subscribe for stats notifications.  Error="<<subscribeError);
                return false;
            }
        }

        // cache period ids
        // to invoke this RPC will require authentication
        UserSession::pushSuperUserPrivilege();
        statsComponent->getPeriodIdsMaster(mOldPeriodIds);
        UserSession::popSuperUserPrivilege();
    }
    else
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].configureHelper: OSDKSeasonalPlay requires the stats component to exist.");
        return false;
    }

    // listen for user session events
    gUserSessionManager->addSubscriber(*this); 

    return true;
}

/*************************************************************************************************/
/*!
    \brief  onResolve

    lifecycle method: called when the slave is to resolve itself. registers for master 
    notifications

*/
/*************************************************************************************************/
bool OSDKSeasonalPlaySlaveImpl::onResolve()
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onResolve");

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onShutdown

    lifecycle method: called when the slave is shutting down. need to unhook our event listeners

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::onShutdown()
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onShutdown");

    // unhook stats period rollover event
    Stats::StatsMaster* statsComponent
        = static_cast<Stats::StatsMaster*>(
        gController->getComponent(Stats::StatsMaster::COMPONENT_ID,false));
    if (NULL != statsComponent)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onShutdown - Stats component not found.");
        statsComponent->removeNotificationListener(*this);
    }

    // stop listening to user session events
    gUserSessionManager->removeSubscriber(*this);
}

/*************************************************************************************************/
/*!
    \brief  getStatusInfo

    Called periodically to collect status and statistics about this component that can be used 
    to determine what future improvements should be made.

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl].getStatusInfo");

    mMetrics.report(&status);
}

/*************************************************************************************************/
/*!
    \brief  checkMissedEndOfSeasonProcessing

    Checks if we missed processing the end of season for the seasons and initiates it
    if need be.
    Because processing are scheduled by period type per slave we will go through all
    the configured seasons and check their period types for scheduling. If need each period type
    will be scheduled for processing only once.

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::checkMissedEndOfSeasonProcessing()
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing.");

    BlazeRpcError error = Blaze::ERR_OK;

    // we don't want to do anything until the seasonal play slave is in it's activated state so 
    // put this fiber to sleep until that happens
    error = waitForState(Blaze::ComponentStub::ACTIVE, "OSDKSeasonalPlaySlaveImpl::checkMissedEndOfSeasonProcessing");
    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing. waitForState() error " << ErrorHelp::getErrorName(error));
            return;
    }

    // get the stats component for retrieving the period id's.
    Stats::StatsSlave* statsComponent
        = static_cast<Stats::StatsSlave*>(
        gController->getComponent(Stats::StatsSlave::COMPONENT_ID,false, true, &error));
    if (ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing - Stats component not found.");
        // early return because we don't have access to stats
        return;
    }

    // the current period id for the season's period type
    Stats::PeriodIds ids;

    // to invoke this RPC will require authentication
    UserSession::pushSuperUserPrivilege();
    error = statsComponent->getPeriodIds(ids);
    UserSession::popSuperUserPrivilege();

    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing. Unable to get PeriodIds");
        return;
    }

    // establish a database connection
    OSDKSeasonalPlayDb dbHelper(getDbId());
    error = dbHelper.getBlazeRpcError();
    if(Blaze::ERR_OK != error)
    {
        // early return because DB connection is not accessible.
        return;
    }

    // for tracking already scheduled period types
    // not scheduled by default
    bool scheduledPeriodTypes[Blaze::Stats::STAT_NUM_PERIODS];
    memset(scheduledPeriodTypes, false, sizeof(scheduledPeriodTypes));

    // iterate through all seasons schedule the custom season roll over processing  by period type.
    SeasonConfigurationServerMap::const_iterator itr = mSeasonConfigurationServerMap.begin();
    SeasonConfigurationServerMap::const_iterator itr_end = mSeasonConfigurationServerMap.end();

    for( ; itr != itr_end; ++itr)
    {
        // get the season configuration for the season
        const SeasonConfigurationServer *pSeason = itr->second;

        if(NULL == pSeason)
        {
            //early return because pSeason is not accessible.
            return;
        }

        SeasonId seasonId = pSeason->getSeasonId();
        Stats::StatPeriodType seasonPeriodType = pSeason->getStatPeriodtype();

        if(true == scheduledPeriodTypes[seasonPeriodType])
        {
            // period type for this season has been already scheduled on this instance, skipping
            break;
        }

        int32_t currentPeriodId = 0;
        switch (seasonPeriodType)
        {
        case Stats::STAT_PERIOD_DAILY:
            currentPeriodId = ids.getCurrentDailyPeriodId();
            break;
        case Stats::STAT_PERIOD_WEEKLY:
            currentPeriodId = ids.getCurrentWeeklyPeriodId();
            break;
        case Stats::STAT_PERIOD_MONTHLY:
            currentPeriodId = ids.getCurrentMonthlyPeriodId();
            break;
        default:
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing. Invalid stat period ID " << seasonPeriodType << "!");
            break;
        }

        // when end of season processing successfully completes, we update the season in the database with
        // the period id that we just rolled over to. By comparing this period id with the current period id
        // from the stats component, we can determine if we need to do end of season processing (which would
        // be the case if the server was down when the stats rollover happened).
        uint32_t seasonNumber = 0;
        int32_t seasonPeriodId = 0;

        error = dbHelper.fetchSeasonNumber(seasonId, seasonNumber, seasonPeriodId);

        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing. checking End of Season processing required for"
            << " period " << Stats::StatPeriodTypeToString(seasonPeriodType) << ", season id = " << seasonId
            << ", season period id = " << seasonPeriodId << ", current period = " << currentPeriodId);

        // if the current period id is greater than the season's period id, then we missed doing the end of season processing.
        if (Blaze::ERR_OK == error && currentPeriodId > seasonPeriodId)
        {
            // need to kick of end of season processing
            INFO_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing. Need end of season processing for season period type "
                << Stats::StatPeriodTypeToString(seasonPeriodType));

            gSelector->scheduleFiberCall(this, &OSDKSeasonalPlaySlaveImpl::onStatsRollover,
                seasonPeriodType, seasonPeriodId, currentPeriodId, "StatsSlaveImpl::onStatsRollover");

            scheduledPeriodTypes[seasonPeriodType] = true;

            // record metrics
            mMetrics.mMissedEndOfSeasonProcessingDetected.add(1);
        }
        else
        {
            TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing. End of Season processing NOT required for season period type " << seasonPeriodType);
        }
    }
}

/*************************************************************************************************/
/*!
    \brief  processEndOfSeason
    
    does the actual end of season processing

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::processEndOfSeason(Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId)
{
    INFO_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason: processing end of season. period = " << Stats::StatPeriodTypeToString(periodType) <<
             ", oldPeriodId = " << oldPeriodId << ", newPeriodId = " << newPeriodId);

    // record the end of season processing start time for metrics
    Blaze::TimeValue endOfSeasonProcStartTime = TimeValue::getTimeOfDay();

    // check if the old and new period id's match. If they do then we don't want to do end of season processing
    // (possible edge case during server startup when checking if end of season processing was missed)
    if (oldPeriodId == newPeriodId)
    {
        INFO_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason: skipping end of season processing as old and new period ids match.");
        return;
    }

    // record metrics - don't care about success/failure; only that it was attempted
    mMetrics.mEndOfSeasonProcessed.add(1);

    BlazeRpcError error = Blaze::ERR_OK;

    // find all seasons that match the period type and invoke the custom season roll over processing 
    SeasonConfigurationServerMap::const_iterator itr = mSeasonConfigurationServerMap.begin();
    SeasonConfigurationServerMap::const_iterator itr_end = mSeasonConfigurationServerMap.end();

    for( ; itr != itr_end; ++itr)
    {
        const SeasonConfigurationServer *pTempSeason = itr->second;
        SeasonId seasonId = pTempSeason->getSeasonId();

        if(pTempSeason->getStatPeriodtype() == periodType)
        {
            INFO_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason: processing period = " << Stats::StatPeriodTypeToString(periodType) <<
                ", seasonId = " << seasonId);

            // get the season number for the season
            uint32_t currentSeasonNumber = 0;
            int32_t lastRolloverStatPeriodId = 0;

            // establish a database connection
            OSDKSeasonalPlayDb dbHelper(getDbId());
            error = dbHelper.getBlazeRpcError();

            if (Blaze::ERR_OK != error)
            {
                ERR_LOG("[[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason error(" <<
                    ErrorHelp::getErrorName(dbHelper.getBlazeRpcError()) << "), " << ErrorHelp::getErrorDescription(dbHelper.getBlazeRpcError()));
                return;
            }

            DbConnPtr dbConn = dbHelper.getDbConnPtr();

            // use transaction to be able to lock and unlock modified table
            error = dbConn->startTxn();
            if (Blaze::ERR_OK != error)
            {
                ERR_LOG("[[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason Unable to start txn. error(" << getDbErrorString(error) << ")");
                return;
            }

            // lock season to prevent simultaneous modifying of season info
            error = dbHelper.lockSeason(seasonId);
            if (Blaze::ERR_OK != error)
            {
                ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason: Unable to lock season.");
                dbConn->rollback();
                return;
            }

            error = dbHelper.fetchSeasonNumber(seasonId, currentSeasonNumber, lastRolloverStatPeriodId);
            if (Blaze::ERR_OK == error)
            {
                if(lastRolloverStatPeriodId >= newPeriodId)
                {
                    // season processing was done while we were waiting for lock on the season
                    // (edge case - could be if next rollover occurs during processing missed rollover)
                    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason: season has been already updated. season id = " << seasonId <<
                        ", seasonNumber = " << currentSeasonNumber << ", lastPeriodId = " << lastRolloverStatPeriodId << "");
                    dbConn->rollback();
                    continue;
                }

                // invoke the custom processing for the season
                onSeasonRollover(dbHelper, seasonId, currentSeasonNumber, oldPeriodId, newPeriodId);

                // reset the members tournament status
                resetSeasonTournamentStatus(dbHelper, seasonId);

                // invoke the reset tournament custom hook
                onResetTournament(seasonId, pTempSeason->getTournamentId());

                // calculate the new season number
                uint32_t newSeasonNumber = currentSeasonNumber + 1;  // default to incrementing by 1
                if (lastRolloverStatPeriodId > 0)
                {
                    // calculated the number of seasons to increment from the difference of the current period id and the
                    // period id we had when we last updated on a rollover.
                    newSeasonNumber = currentSeasonNumber + newPeriodId - lastRolloverStatPeriodId;
                }

                TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason: updating season number. season id = " << seasonId <<
                            ", seasonNumber = " << newSeasonNumber << ", lastPeriodId = " << newPeriodId << "");
                error = dbHelper.updateSeasonNumber(seasonId, newSeasonNumber, newPeriodId);

                if (Blaze::ERR_OK != error)
                {
                    ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason: failed to update season number. season id "
                            << seasonId << ", seasonNumber = " << newSeasonNumber << ", lastPeriodId = " << newPeriodId);
                    dbConn->rollback();
                }

                error = dbConn->commit();
                if (Blaze::ERR_OK != error)
                {
                    ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason Unable to commit txn. error(" << getDbErrorString(error) << ")");
                }
            }
            else
            {
                ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].processEndOfSeason: error retrieving season number for season id " << seasonId);
                dbConn->rollback();
            }
        }
    }
    
    // record the amount of time that end of season processing took
    mMetrics.mLastEndSeasonProcessingTimeMs.setCount(
        static_cast<uint32_t>((TimeValue::getTimeOfDay() - endOfSeasonProcStartTime).getMillis()));
}

/*************************************************************************************************/
/*!
    \brief  onSetPeriodIdsNotification

    receives notification from the stats component that a stat period has rolled over

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::onSetPeriodIdsNotification(const Stats::PeriodIds& newIds, UserSession *)
{
    // update the cached ids
    Stats::PeriodIds oldIds;
    mOldPeriodIds.copyInto(oldIds);
    newIds.copyInto(mOldPeriodIds);

    INFO_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onSetPeriodIdsNotification.  daily: " << newIds.getCurrentDailyPeriodId() << "  weekly: " 
        << newIds.getCurrentWeeklyPeriodId() << " monthly: " << newIds.getCurrentMonthlyPeriodId());

    if (oldIds.getCurrentDailyPeriodId() != newIds.getCurrentDailyPeriodId())
    {
        gSelector->scheduleFiberCall(this, &OSDKSeasonalPlaySlaveImpl::onStatsRollover,
            Stats::STAT_PERIOD_DAILY, oldIds.getCurrentDailyPeriodId(), newIds.getCurrentDailyPeriodId(), "StatsSlaveImpl::onStatsRollover");
    }
    if (oldIds.getCurrentWeeklyPeriodId() != newIds.getCurrentWeeklyPeriodId())
    {
        gSelector->scheduleFiberCall(this, &OSDKSeasonalPlaySlaveImpl::onStatsRollover,
            Stats::STAT_PERIOD_WEEKLY, oldIds.getCurrentWeeklyPeriodId(), newIds.getCurrentWeeklyPeriodId(), "StatsSlaveImpl::onStatsRollover");
    }
    if (oldIds.getCurrentMonthlyPeriodId() != newIds.getCurrentMonthlyPeriodId())
    {
        gSelector->scheduleFiberCall(this, &OSDKSeasonalPlaySlaveImpl::onStatsRollover,
            Stats::STAT_PERIOD_MONTHLY, oldIds.getCurrentMonthlyPeriodId(), newIds.getCurrentMonthlyPeriodId(), "StatsSlaveImpl::onStatsRollover");
    }
}

/*************************************************************************************************/
/*!
    \brief  onStatsRollover

    handle period rollover

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::onStatsRollover(Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId)
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onStatsRollover: received stats rollover event. period = " << Stats::StatPeriodTypeToString(periodType) <<
              ", oldPeriodId = " << oldPeriodId << ", newPeriodId = " << newPeriodId << "");

    // since we are in a separate fiber we can freely sleep and execute blocking operations
    
    BlazeRpcError err = Blaze::ERR_OK;

    // delayed to give stats time to correctly rollover
    const static int64_t MICROSECONDS_PER_SECOND = 1000000LL;
    err = gSelector->sleep(MICROSECONDS_PER_SECOND * 120);
    if (Blaze::ERR_OK != err)
    {
        WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onStatsRollover Got sleep error " << ErrorHelp::getErrorName(err));
    }

    // schedule a task for processing
    if (NULL != mScheduler)
    {
        eastl::string taskName;
        taskName.append_sprintf("%s_%s_%d", OSDKSP_SEASON_ROLLOVER_TASKNAME_PREFIX, StatPeriodTypeToString(periodType), newPeriodId);

        // the TaskScheduler will sort out multiple slaves attempting to schedule the same task
        RunOneTimeTaskHelper helper(this, periodType, oldPeriodId, newPeriodId);
        err = mScheduler->runOneTimeTaskAndBlock(taskName.c_str(), COMPONENT_ID, TaskSchedulerSlaveImpl::RunOneTimeTaskCb(&helper, &RunOneTimeTaskHelper::execute));
        if (Blaze::ERR_OK != err)
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].onStatsRollover failed to run a season rollover task " << taskName);
        }
    }
}

/*************************************************************************************************/
/*!
    \brief  onUserSessionExistence

    called when a user logs into the slave. checks if we should register the user in
    seasonal play and does so if needed.

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::onUserSessionExistence(const UserSession& userSession)
{
    // only care about this login event if the user is logged into this slave from a console
    if (true == mAutoUserRegistrationEnabled && 
        true == userSession.isLocal())
    {
        gSelector->scheduleFiberCall(
            this, 
            &OSDKSeasonalPlaySlaveImpl::verifyUserRegistration,
            userSession.getUserId(),
            "OSDKSeasonalPlaySlaveImpl::verifyUserRegistration");
    }
}


/*************************************************************************************************/
/*!
    \brief  getSeasonConfiguration

    Gets the season configuration for the season matching the season id

*/
/*************************************************************************************************/
const SeasonConfigurationServer* OSDKSeasonalPlaySlaveImpl::getSeasonConfiguration(SeasonId seasonId)
{
    SeasonConfigurationServerMap::const_iterator it = mSeasonConfigurationServerMap.find(seasonId);
    if (it == mSeasonConfigurationServerMap.end())
    {
        return NULL;
    }

    return it->second;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonConfigurationForLeague

    Gets the season configuration for the season matching the league id and member type    

*/
/*************************************************************************************************/
const SeasonConfigurationServer* OSDKSeasonalPlaySlaveImpl::getSeasonConfigurationForLeague(LeagueId leagueId, MemberType memberType = SEASONALPLAY_MEMBERTYPE_CLUB)
{
    const SeasonConfigurationServer *pSeasonConfig = NULL;

    SeasonConfigurationServerMap::const_iterator itr = mSeasonConfigurationServerMap.begin();
    SeasonConfigurationServerMap::const_iterator itr_end = mSeasonConfigurationServerMap.end();

    for( ; itr != itr_end; ++itr)
    {
        const SeasonConfigurationServer *pTempSeason = itr->second;

        if (memberType == pTempSeason->getMemberType() && leagueId == pTempSeason->getLeagueId())
        {
            pSeasonConfig = pTempSeason;
            break;
        }
    }

    return pSeasonConfig;
}


/*************************************************************************************************/
/*!
    \brief  verifyUserRegistration

    Does the work of checking if a user is registered in a season and if not, registers the user

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::verifyUserRegistration(BlazeId userId)
{
    if (INVALID_BLAZE_ID == userId || gUserSessionManager->isStatelessUser(userId))
    {
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].verifyUserRegistration. User logged in with invalid or stateless UserId = " << userId);
    }
    else
    {
        TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].verifyUserRegistration. User logged in. User id = " << userId);

        BlazeRpcError error = Blaze::ERR_OK;

        OSDKSeasonalPlayDb dbHelper(getDbId());

        // is the user registered for user seasonal play?
        SeasonId seasonId = SEASON_ID_INVALID;
        error = dbHelper.fetchSeasonId(userId, SEASONALPLAY_MEMBERTYPE_USER, seasonId);

        if (Blaze::OSDKSEASONALPLAY_ERR_NOT_REGISTERED == error)
        {
            // user is not registered in seasonal play

            // determine the season id that corresponds to versus and register the user in it
            bool8_t bFoundUserSeason = false;
            LeagueId versusLeagueId = 0;
            SeasonConfigurationServerMap::const_iterator itr = mSeasonConfigurationServerMap.begin();
            SeasonConfigurationServerMap::const_iterator itr_end = mSeasonConfigurationServerMap.end();

            for( ; itr != itr_end; ++itr)
            {
                const SeasonConfigurationServer *pSeason = itr->second;
                
                if (SEASONALPLAY_MEMBERTYPE_USER == pSeason->getMemberType())
                {
                    seasonId = pSeason->getSeasonId();
                    versusLeagueId = pSeason->getLeagueId();
                    bFoundUserSeason = true;
                    break;
                }
            }

            // only register the user if there is a user season found
            if (true == bFoundUserSeason)
            {
                TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].verifyUserRegistration. Registering User in versus season. User id = " << userId << 
                            ", season id = " << seasonId << "");
                error = dbHelper.insertRegistration(userId, SEASONALPLAY_MEMBERTYPE_USER, seasonId, versusLeagueId);   
            }

            if (Blaze::ERR_OK != error)
            {
                ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].verifyUserRegistration. Error in registering user in seasonal play. Error = " << error);
            }
        }
    }
}


/*************************************************************************************************/
/*!
    \brief  getSeasonPeriodType

    gets the stats period type for the specified season

*/
/*************************************************************************************************/
Stats::StatPeriodType OSDKSeasonalPlaySlaveImpl::getSeasonPeriodType(SeasonId seasonId)
{
    Stats::StatPeriodType periodType = Stats::STAT_PERIOD_ALL_TIME; 

    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL != pSeasonConfig)
    {
        periodType = pSeasonConfig->getStatPeriodtype();
    }
    else
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getSeasonPeriodType(): unable to retrieve configuration for season id " << seasonId);
    }
    
    return periodType;   
}


/*************************************************************************************************/
/*!
    \brief  getTournamentRule

    gets the tournament rule for the specified season

*/
/*************************************************************************************************/
TournamentRule OSDKSeasonalPlaySlaveImpl::getTournamentRule(SeasonId seasonId, uint8_t divisionNumber)
{
    TournamentRule tournRule = SEASONALPLAY_TOURNAMENTRULE_UNLIMITED;

    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);

    if (NULL != pSeasonConfig)
    {
        SeasonConfigurationServer::DivisionList::const_iterator itr = pSeasonConfig->getDivisions().begin();
        SeasonConfigurationServer::DivisionList::const_iterator itr_end = pSeasonConfig->getDivisions().end();

        for ( ; itr != itr_end; ++itr)
        {
            const Division *division = *itr;
            if (divisionNumber == division->getNumber())
            {
                tournRule = division->getTournamentRule();
            }
        }
    }
    else
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getTournamentRule(): unable to retrieve configuration for season id " << seasonId);
    }


    return tournRule;
}

/*************************************************************************************************/
/*!
    \brief  getLeaderboardHasScope

    Whether a leaderboard has a key scope or not

*/
/*************************************************************************************************/
bool8_t OSDKSeasonalPlaySlaveImpl::getLeaderboardHasScope(const char8_t* leaderBoardName)
{
    bool8_t hasScope = false;

    Stats::StatsSlave* statsComponent
        = static_cast<Stats::StatsSlave*>(
        gController->getComponent(Stats::StatsSlave::COMPONENT_ID, false));
    if (NULL == statsComponent)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getLeaderboardHasScope - Stats component not found.");
        return hasScope;
    }

    Stats::LeaderboardGroupRequest  lbGroupRequest;
    lbGroupRequest.setBoardName(leaderBoardName);
    Stats::LeaderboardGroupResponse lbGroupResponse;
    
    // to invoke this RPC will require authentication
    UserSession::pushSuperUserPrivilege();
    BlazeRpcError error = statsComponent->getLeaderboardGroup(lbGroupRequest, lbGroupResponse);
    UserSession::popSuperUserPrivilege();
    
    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].getLeaderboardHasScope. Unable to get the leaderboard group for leaderboard "
            << leaderBoardName << ". Exiting end of season processing");
        return hasScope;
    }

    hasScope = (lbGroupResponse.getKeyScopeNameValueListMap().size()!=0);

    return hasScope;
}

/*************************************************************************************************/
/*!
    \brief  parseTimeDuration

    Takes in int parameters of minutes, hours, and days and saves TimeStamp in seconds in 'result' out parameter. Performs
    error checking on time parameters and returns false if an error is detected.

*/
/*************************************************************************************************/
bool8_t OSDKSeasonalPlaySlaveImpl::parseTimeDuration(int32_t minutes, int32_t hours, int32_t days, TimeStamp &result)
{
    result = 0;

    int32_t iMinutes = minutes;
    int32_t iHours = hours;
    int32_t iDays = days;

    if ((iMinutes < 0) ||
        (iMinutes > 59) ||
        (iHours < 0) ||
        (iHours > 23) ||
        (iDays < 0) ||
        (iDays > 31))
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseTimeDuration: invalid parameters for time duration. days = " << iDays << ", hours = " << iHours << ", minutes = " << iMinutes );
        return false;    // EARLY RETURN
    }

    result = OSDKSeasonalPlayTimeUtils::getDurationInSeconds(iDays, iHours, iMinutes);

    return true;
}


/*************************************************************************************************/
/*!
    \brief  parsePeriodType

    Takes in int value and saves the associated period type (monthly, weekly, or daily)
    in 'result' out parameter.
    Performs error checking and returns false if an error is detected.

*/
/*************************************************************************************************/
bool8_t OSDKSeasonalPlaySlaveImpl::parsePeriodType(int32_t iPeriodType, Stats::StatPeriodType &result)
{
    result = Blaze::Stats::STAT_NUM_PERIODS; // using num periods as the error condition

    // convert the iPerdiodType bit mask to the StatPeriodType enum
    switch (iPeriodType)
    {
    case 0x2:
        result = Blaze::Stats::STAT_PERIOD_MONTHLY;
        break;
    case 0x4:
        result = Blaze::Stats::STAT_PERIOD_WEEKLY;
        break;
    case 0x8:
        result = Blaze::Stats::STAT_PERIOD_DAILY;
        break;
    default:
        break;
    }
    if (Blaze::Stats::STAT_NUM_PERIODS == result)
    {
        // invalid stat period
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parsePeriodType: missing or invalid configuration parameter <StatPeriodType> periodType.");
        return false; // EARLY RETURN
    }

    return true;
}


/*************************************************************************************************/
/*!
    \brief  parseDivisions

    Parses the "division" configuration.
    Performs error checking and returns false if an error is detected.

    Similar to parseConfig, there are two different division data: the old "Division" and the new
    "ConfigInstanceDivision" which is used to implement Strict Config Parsing. For now, to preserve
    old behaviour, parseDivisions will use getters of ConfigInstanceDivision to grab values from
    .cfg to store into Divisions using its own setters.
    A complete refactoring of this component is a TODO item; for now this is done so that
    osdkseasonalplay.tdf and variable names in the tdf are left as is.
*/
/*************************************************************************************************/
bool8_t OSDKSeasonalPlaySlaveImpl::parseDivisions(const ConfigInstanceDivisionList &divisions, SeasonConfigurationServer *seasonConfig, SeasonConfigurationServer::DivisionList& divisionList)
{
    bool8_t bRet = true;
    int32_t iSizeTotal = 0;
    int32_t iDivisionCount = 0;

    ConfigInstanceDivisionList::const_iterator divisionIt = divisions.begin();
    ConfigInstanceDivisionList::const_iterator divisionItEnd = divisions.end();
    if (divisionIt == divisionItEnd)
    {
        WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseDivisions: divisions not found.");
        bRet = false;
    }
    while ((divisionIt != divisionItEnd) && bRet)
    {
        if ((*divisionIt) == NULL) // EA::TDF::tdf_ptr doesn't support prefixed NULL check
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseDivisions: missing or invalid division structure"
                " at element " << iDivisionCount << " of 'divisions'");
            bRet = false;
            break;
        }

        const ConfigInstanceDivision &thisDivision = *(*divisionIt);
        Division *division = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "Division") Division();

        // Warn about 0 values; likely missing or incorrect values
        if (0 == thisDivision.getDivisionNumber())
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseDivisions: Division Number should not be 0.");
        }
        if (0 == thisDivision.getDivisionSize())
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseDivisions: Division Size should not be 0.");
        }
        if (0 == thisDivision.getPlayoffRule())
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseDivisions: Playoff Rule should not be 0.");
        }

        division->setNumber(thisDivision.getDivisionNumber());
        division->setSize(thisDivision.getDivisionSize());
        division->setTournamentRule(static_cast<TournamentRule>(thisDivision.getPlayoffRule()));

        iSizeTotal += division->getSize();
        iDivisionCount++;

        divisionList.push_back(division);
        ++divisionIt;
    }

 	//--------------------
	// FIFA is not using the percentage base division, but a fixed size division
	// apoon@ea.com May 6 2010
	//--------------------
		// verify that the division sizes add up to 100
		//if (100 != iSizeTotal)
		//{
		//    ERR("[OSDKSeasonalPlayMasterImpl:%p].parseDivisions(): division sizes do not add up to 100", this);
		//    result = false;
		//}


    // TODO - verify that the divisions are in sequence (is this even necessary?)

    return bRet;
}


/*************************************************************************************************/
/*!
    \brief  parseConfig

    Parses the configuration as defined in osdkseasonalplay.cfg.

    "OSDKSeasonalPlayConfig" and "ConfigInstance" are objects used to implement Strict Config
    Parsing. They are used to enforce proper values in the .cfg file but some variable names vary
    from older objects such as seasonConfig.
    A complete refactoring of this component is a TODO item; for now this is done so that
    osdkseasonalplay.tdf and variable names in the .cfg are left as is.
*/
/*************************************************************************************************/
bool8_t OSDKSeasonalPlaySlaveImpl::parseConfig()
{
    bool8_t bResult = true;
    const OSDKSeasonalPlayConfig& config = getConfig();
    bool8_t bInstanceSuccess = true;

    // establish a database connection
    OSDKSeasonalPlayDb dbHelper(mDbId);
    if(Blaze::ERR_OK != dbHelper.getBlazeRpcError())
    {
        ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: error " <<
            ErrorHelp::getErrorName(dbHelper.getBlazeRpcError()) << " - " << ErrorHelp::getErrorDescription(dbHelper.getBlazeRpcError()));
        bResult = false;
    }

    ConfigInstanceList::const_iterator instanceIt = config.getInstances().begin();
    ConfigInstanceList::const_iterator instanceItEnd = config.getInstances().end();
    if (instanceIt == instanceItEnd)
    {
        WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: no instances found.");
    }
    while ((instanceIt != instanceItEnd) && bInstanceSuccess)
    {
        const ConfigInstance& instanceConfig = *(*instanceIt);

        SeasonConfigurationServer *seasonConfig = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "SeasonConfigurationServer") SeasonConfigurationServer();

        // Warn about empty values or 0; likely missing or incorrect values
        if (0 == instanceConfig.getId())
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Instance ID should not be 0.");
        }
        if (0 == instanceConfig.getMemberType())
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Member Type should not be 0.");
        }
        if (0 == instanceConfig.getTournamentId())
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Tournament ID should not be 0.");
        }
        if ('\0' == *(instanceConfig.getStatCategory()))
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Stat Category should not be an empty string.");
        }
        if ('\0' == *(instanceConfig.getLeaderboard()))
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Leaderboard should not be an empty string.");
        }

        seasonConfig->setSeasonId(instanceConfig.getId());
        seasonConfig->setMemberType((MemberType)(instanceConfig.getMemberType()));
        seasonConfig->setLeagueId(instanceConfig.getLeagueId());
        seasonConfig->setTournamentId(instanceConfig.getTournamentId());
        seasonConfig->setStatCategoryName(instanceConfig.getStatCategory());
        seasonConfig->setLeaderboardName(instanceConfig.getLeaderboard());
        
        Blaze::Stats::StatPeriodType statPeriodType;
        if(true == bInstanceSuccess)
        {
            bInstanceSuccess = parsePeriodType(instanceConfig.getPeriodType(), statPeriodType);
            seasonConfig->setStatPeriodtype(statPeriodType);
        }

        if(true == bInstanceSuccess)
        {
            const ConfigInstancePlayoffDuration& pd = instanceConfig.getPlayoffDuration();
            TimeStamp ts;
            bInstanceSuccess = parseTimeDuration(pd.getMinutes(), pd.getHours(), pd.getDays(), ts);
            seasonConfig->setPlayoffDuration(ts);
        }

        if(true == bInstanceSuccess)
        {
            const ConfigInstanceEndOfSeasonBlackOutDuration& bod = instanceConfig.getEndOfSeasonBlackOutDuration();
            TimeStamp ts;
            parseTimeDuration(bod.getMinutes(), bod.getHours(), bod.getDays(), ts);
            seasonConfig->setBlackOutDuration(ts);
        }
        seasonConfig->setPlayoffsRequireRegularSeasonGame(instanceConfig.getPlayoffsRequireRegularSeasonGame());
        seasonConfig->setFixedDivisions(instanceConfig.getFixedDivisions());

        if(true == bInstanceSuccess)
        {
            const ConfigInstanceDivisionList &divisions = instanceConfig.getDivisions();
            bInstanceSuccess = parseDivisions(divisions, seasonConfig, seasonConfig->getDivisions());
        }

        if (true == bInstanceSuccess)
        {
            // insert the season into the database (will fail silently if the season already exists)
            BlazeRpcError dbError = dbHelper.insertNewSeason(seasonConfig->getSeasonId());

            if (ERR_OK != dbError)
            {
                ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: error inserting season " << seasonConfig->getSeasonId() << " into database");
                bResult = false;
                delete seasonConfig;
            }
            else
            {
                mSeasonConfigurationServerMap[seasonConfig->getSeasonId()] = seasonConfig;
                mMetrics.mTotalSeasons.add(1);
            }
            
            ++instanceIt;
        }
        else
        {
            bResult = false;
            delete seasonConfig;
        }

        if (false == bInstanceSuccess)
        {
            ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: check element with id: " << instanceConfig.getId() << " of the 'instances' sequence");
        }
    }

    // schedule checking of missed season rollover processing
    gSelector->scheduleFiberCall(
        this, 
        &OSDKSeasonalPlaySlaveImpl::checkMissedEndOfSeasonProcessing,
        "OSDKSeasonalPlaySlaveImpl::checkMissedEndOfSeasonProcessing");

    // parse awards
    ConfigAwardList::const_iterator awardIt = config.getAwards().begin();
    ConfigAwardList::const_iterator awardItEnd = config.getAwards().end();
    if (awardIt == awardItEnd)
    {
        WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: no awards found.");
        bResult = false;
    }
    while (awardIt != awardItEnd)
    {
        const ConfigAward &thisAward = *(*awardIt);
        AwardConfiguration *award = BLAZE_NEW_MGID(COMPONENT_MEMORY_GROUP, "AwardConfiguration") AwardConfiguration();

        // Warn about empty values or 0; likely missing or incorrect values
        if (0 == thisAward.getId())
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Award ID should not be 0.");
        }        
        if ('\0' == *(thisAward.getName()))
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Award Name should not be 0.");
        }
        if ('\0' == *(thisAward.getDescription()))
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Award Description should not be an empty string.");
        }
        if ('\0' == *(thisAward.getAsset()))
        {
            WARN_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].parseConfig: Award Asset should not be an empty string.");
        }

        award->setAwardId(thisAward.getId());
        award->setName(thisAward.getName()); // this is the unlocalized string id
        award->setDescription(thisAward.getDescription()); // this is the unlocalized string id
        award->setAssetPath(thisAward.getAsset());

        mAwardConfigurationMap[award->getAwardId()] = award;
        mMetrics.mTotalAwards.add(1);

        ++awardIt;
    }

    debugPrintConfig();

    return bResult;
}


/*************************************************************************************************/
/*!
    \brief  debugPrintConfig

    Prints out configuration contents for debugging purposes.

*/
/*************************************************************************************************/
void OSDKSeasonalPlaySlaveImpl::debugPrintConfig()
{
    TRACE_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].debugPrintConfig: print configuration");

    int32_t iSize = 0;
    char8_t strDuration[16];    // for printing duration

    // print the season configuration
    iSize = mSeasonConfigurationServerMap.size();
    INFO_LOG("\tNumber of Configured Seasons: " << iSize);

    if (iSize > 0)
    {
        int32_t iCount = 1;

        SeasonConfigurationServerMap::const_iterator itr = mSeasonConfigurationServerMap.begin();
        SeasonConfigurationServerMap::const_iterator itr_end = mSeasonConfigurationServerMap.end();

        for ( ; itr != itr_end; ++itr)
        {
            const SeasonConfigurationServer *season = itr->second;
            INFO_LOG("\t  Instance #" << iCount);
            INFO_LOG("\t\tId                = " << season->getSeasonId());
            INFO_LOG("\t\tmMemberType       = " << MemberTypeToString(season->getMemberType()));
            INFO_LOG("\t\tmLeagueId         = " << season->getLeagueId());
            INFO_LOG("\t\tmTournamentId     = " << season->getTournamentId());
            INFO_LOG("\t\tmStatCategoryName = " << season->getStatCategoryName());
            INFO_LOG("\t\tmLeaderboardName  = " << season->getLeaderboardName());
            INFO_LOG("\t\tmStatPeriodtype   = " << Blaze::Stats::StatPeriodTypeToString(season->getStatPeriodtype()));
            OSDKSeasonalPlayTimeUtils::getDurationAsString(season->getPlayoffDuration(), strDuration, sizeof(strDuration));
            INFO_LOG("\t\tmPlayoffDuration  = " << strDuration);
            OSDKSeasonalPlayTimeUtils::getDurationAsString(season->getBlackOutDuration(), strDuration, sizeof(strDuration));
            INFO_LOG("\t\tmBlackoutDuration = " << strDuration);
            INFO_LOG("\t\tmPlayoffsRequireRegularSeasonGame    = " << season->getPlayoffsRequireRegularSeasonGame());
            
            // print out the divisions
            INFO_LOG("\t\tDivisions         =");
            SeasonConfigurationServer::DivisionList::const_iterator divItr = season->getDivisions().begin();
            SeasonConfigurationServer::DivisionList::const_iterator divItr_end = season->getDivisions().end();
            for ( ; divItr != divItr_end; ++divItr)
            {
                const Division *division = *divItr;
                INFO_LOG("\t\t\tmNumber         = " << division->getNumber());
                INFO_LOG("\t\t\tmSize           = " << division->getSize());
                INFO_LOG("\t\t\tmTournamentRule = " << TournamentRuleToString(division->getTournamentRule()));
            }

            ++iCount;
        }
    }

    // print the award configuration
    iSize = mAwardConfigurationMap.size();
    INFO_LOG("\tNumber of configured Awards: " << iSize);

    if (iSize > 0)
    {
        int32_t iCount = 1;

        AwardConfigurationMap::const_iterator itr = mAwardConfigurationMap.begin();
        AwardConfigurationMap::const_iterator itr_end = mAwardConfigurationMap.end();

        for ( ; itr != itr_end; ++itr)
        {
            const AwardConfiguration *award = itr->second;
            INFO_LOG("\t  Award #" << iCount);
            INFO_LOG("\t\tId            = " << award->getAwardId());
            INFO_LOG("\t\tName          = " << award->getName());
            INFO_LOG("\t\tDescription   = " << award->getDescription());
            INFO_LOG("\t\tAsset         = " << award->getAssetPath());

            ++iCount;
        }
    }
}


} // OSDKSeasonalPlay
} // Blaze
