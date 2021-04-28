/*************************************************************************************************/
/*!
    \file   fifacupsslaveimpl.cpp

    $Header: //gosdev/games/FIFA/2012/Gen3/DEV/blazeserver/3.x/customcomponent/fifacups/fifacupsslaveimpl.cpp#2 $
    $Change: 289160 $
    $DateTime: 2011/03/08 12:38:30 $

    \attention
    (c) Electronic Arts 2010. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class FifaCupsSlaveImpl

    FifaCups Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "fifacupsslaveimpl.h"

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
#include "component/stats/tdf/stats.h"

// fifacups includes
#include "fifacups/rpc/fifacupsmaster.h"
#include "customcomponent/osdktournaments/rpc/osdktournamentsslave.h"
#include "customcomponent/osdktournaments/tdf/osdktournamentstypes_custom.h"
#include "osdktournaments/tdf/osdktournamentstypes_server.h"


namespace Blaze
{
namespace FifaCups
{


// static
FifaCupsSlave* FifaCupsSlave::createImpl()
{
    return BLAZE_NEW FifaCupsSlaveImpl();
}


/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief  constructor

    

*/
/*************************************************************************************************/
FifaCupsSlaveImpl::FifaCupsSlaveImpl() :
    mAutoUserRegistrationEnabled(false),
    mAutoClubRegistrationEnabled(false),
    mDbId(DbScheduler::INVALID_DB_ID),
    mMetrics(),
	mOldPeriodIds(),
	mRefreshPeriodIds(true)
{
}


/*************************************************************************************************/
/*!
    \brief  destructor

    

*/
/*************************************************************************************************/
FifaCupsSlaveImpl::~FifaCupsSlaveImpl()
{
}

/*************************************************************************************************/
/*!
    \brief  registerEntity

    registers the entity in the member type based season that matches the league id

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::registerEntity(MemberId entityId, MemberType memberType, LeagueId leagueId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].registerEntity. entityId = "<<entityId<<", leagueId = "<<leagueId <<" ");

    BlazeRpcError error = Blaze::ERR_OK;

    // find the season that corresponds to the entity's league id;
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfigurationForLeague(leagueId, memberType);

    // only register the club if a season for the entity was found
    if (NULL != pSeasonConfig)
    {
        TRACE_LOG("[FifaCupsSlaveImpl].registerEntity. Registering entity in season. entity id = "<<entityId<<", season id = "<<pSeasonConfig->getSeasonId()<<" ");
        error = registerForSeason(entityId, memberType, leagueId, pSeasonConfig->getSeasonId());
    }
    else
    {
        ERR_LOG("[FifaCupsSlaveImpl].registerEntity. Unable to find a season to register the entity in. entity = "<<entityId<<", leagueId = "<<leagueId<<" "); 
        error = FIFACUPS_ERR_INVALID_PARAMS;
    }

    return error;
}

/*************************************************************************************************/
/*!
    \brief  registerClub

    registers the club in the clubs based season that matches the league id

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::registerClub(MemberId clubId, LeagueId leagueId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].registerClub. clubId = "<<clubId<<", leagueId = "<<leagueId <<" ");

    BlazeRpcError error = Blaze::ERR_OK;

    if (false == mAutoClubRegistrationEnabled)
    {
        // automatic club registration is disabled. Nothing to do
        return error;
    }

    // find the season that corresponds to the club's league id;
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfigurationForLeague(leagueId, FIFACUPS_MEMBERTYPE_CLUB);

    // only register the club if a season for the club was found
    if (NULL != pSeasonConfig)
    {
        TRACE_LOG("[FifaCupsSlaveImpl].registerClub. Registering Club in season. Club id = "<<clubId<<", season id = "<<pSeasonConfig->getSeasonId()<<" ");
        error = registerForSeason(clubId, FIFACUPS_MEMBERTYPE_CLUB, leagueId, pSeasonConfig->getSeasonId());
    }
    else
    {
        ERR_LOG("[FifaCupsSlaveImpl].registerClub. Unable to find a season to register the club in. clubId = "<<clubId<<", leagueId = "<<leagueId<<" "); 
        error = FIFACUPS_ERR_INVALID_PARAMS;
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
BlazeRpcError FifaCupsSlaveImpl::updateClubRegistration(MemberId clubId, Blaze::FifaCups::LeagueId newLeagueId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].updateClubRegistration. clubId = "<<clubId<<", leagueId = "<<newLeagueId<<" ");

    BlazeRpcError error = Blaze::ERR_OK;

    // find the season that corresponds to the new league id
    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfigurationForLeague(newLeagueId, FIFACUPS_MEMBERTYPE_CLUB);
    if (NULL == pSeasonConfig)
    {
        ERR_LOG("[FifaCupsSlaveImpl].updateClubRegistration. Unable to find a season to register the club in. clubId = "<<clubId<<", leagueId = "<<newLeagueId<<" "); 
        error = FIFACUPS_ERR_CONFIGURATION_ERROR;
        return error;
    }

    FifaCupsDb dbHelper = FifaCupsDb(this);
    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
    error = dbHelper.setDbConnection(dbConn);

	if (Blaze::ERR_OK == error)
	{
		// get the season that the club is registered in
		SeasonId registeredInSeasonId;
		error = dbHelper.fetchSeasonId(clubId, FIFACUPS_MEMBERTYPE_CLUB, registeredInSeasonId);

		if (Blaze::FIFACUPS_ERR_NOT_REGISTERED == error)
		{
			// club is not registered in a season so proceed as if this is a new registration
			error = registerForSeason(clubId, FIFACUPS_MEMBERTYPE_CLUB, newLeagueId, pSeasonConfig->getSeasonId());
		}
		else 
		{
			// update the club's registration
			dbHelper.clearBlazeRpcError();
			error = dbHelper.updateRegistration(clubId, FIFACUPS_MEMBERTYPE_CLUB, pSeasonConfig->getSeasonId(), newLeagueId);

			// leave the existing tournament
			const SeasonConfigurationServer *pOldSeasonCOnfig = getSeasonConfiguration(registeredInSeasonId);
			if (NULL != pOldSeasonCOnfig)
			{
				TournamentId oldTournamentId = pOldSeasonCOnfig->getTournamentId();
				onLeaveTournament(clubId, FIFACUPS_MEMBERTYPE_CLUB, oldTournamentId);
			}

			// record metrics
			mMetrics.mClubRegistrationUpdates.add(1);
		}
	}

    return error;
}


/*************************************************************************************************/
/*!
    \brief  registerForSeason

    registers a member in the specified season

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::registerForSeason(MemberId memberId, MemberType memberType, LeagueId leagueId, SeasonId seasonId)
{
	TRACE_LOG("[FifaCupsSlaveImpl].registerForSeason. memberId = "<< memberId <<" memberType = "<<memberType<<", leagueId = "<<leagueId<<", seasonId = "<<seasonId<<" ");

    BlazeRpcError error = Blaze::ERR_OK;

    FifaCupsDb dbHelper = FifaCupsDb(this);

    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
    error = dbHelper.setDbConnection(dbConn);

	if (error == Blaze::ERR_OK)
	{
		// check if the member is already registered in a season
		SeasonId registeredInSeasonId;
		error = dbHelper.fetchSeasonId(memberId, memberType, registeredInSeasonId);
		if (Blaze::FIFACUPS_ERR_NOT_REGISTERED != error)
		{
			error = Blaze::FIFACUPS_ERR_ALREADY_REGISTERED;
			return error;
		}

		// check that the season exists and the member type and league id match what is configured for the season
		const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
		if (NULL == pSeasonConfig || memberType != pSeasonConfig->getMemberType() || leagueId != pSeasonConfig->getLeagueId())
		{
			error = Blaze::FIFACUPS_ERR_INVALID_PARAMS;
			return error;
		}

		// register the member in the season
		TRACE_LOG("[FifaCupsSlaveImpl].registerForSeason. Registering member in season. Member id = "<<memberId<<", season id = "<<seasonId<<" ");
		dbHelper.clearBlazeRpcError();
		error = dbHelper.insertRegistration(memberId, memberType, seasonId, leagueId, pSeasonConfig->getStartDivision());   
		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[FifaCupsSlaveImpl].registerForSeason. Error registering member in season. Member id = "<<memberId<<", season id = "<<seasonId<<" ");
		}
		else
		{
			// log metrics
			if (FIFACUPS_MEMBERTYPE_CLUB == memberType)
			{
				mMetrics.mClubRegistrations.add(1);
			}
			else if (FIFACUPS_MEMBERTYPE_USER == memberType)
			{
				mMetrics.mUserRegistrations.add(1);
			}
		}
	}

    return error;
}

/*************************************************************************************************/
/*!
    \brief  getCupStartTime

    the time stamp of when the cup begin in the current season

*/
/*************************************************************************************************/
int64_t FifaCupsSlaveImpl::getCupStartTime(SeasonId seasonId)
{
	int64_t iStartTime = 0;

	// get the period type for the season
	Stats::StatPeriodType periodType = getSeasonPeriodType(seasonId);
	if (Stats::STAT_PERIOD_ALL_TIME == periodType)
	{
		// have an invalid period type for the season
		INFO_LOG("[FifaCupsSlaveImpl].getCupStartTime(): season stat period type for season id "<<seasonId<<" returns 0 as perodType is ALL TIME ");
		return iStartTime;
	}

	const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		// have an invalid period type for the season
		ERR_LOG("[FifaCupsSlaveImpl].getCupStartTime(): unable to retrieve configuration for season id "<<seasonId<<" ");
		return iStartTime;
	}


	// the playoff start time is the season end time - the playoff duration
	iStartTime = mTimeUtils.getRollOverTime(periodType, pSeasonConfig->getStatMode()) - pSeasonConfig->getCupDuration();

	TRACE_LOG("[FifaCupsSlaveImpl].getCupStartTime. start time = "<<iStartTime<<" ");
	return iStartTime;
}


/*************************************************************************************************/
/*!
    \brief  getCupEndTime

    the time stamp of when the cup end for the current season

*/
/*************************************************************************************************/
int64_t FifaCupsSlaveImpl::getCupEndTime(SeasonId seasonId)
{
	//same as season end time
	return getSeasonEndTime(seasonId);
}

/*************************************************************************************************/
/*!
    \brief  getSeasonStartTime

    the time stamp of when the current season started

*/
/*************************************************************************************************/
int64_t FifaCupsSlaveImpl::getSeasonStartTime(SeasonId seasonId)
{
    int64_t iStartTime = 0;

    // get the period type for the season
    Stats::StatPeriodType periodType = getSeasonPeriodType(seasonId);
    if (Stats::STAT_PERIOD_ALL_TIME == periodType)
    {
        // have an invalid period type for the season
        ERR_LOG("[FifaCupsSlaveImpl].getSeasonStartTime(): invalid season stat period type for season id "<<seasonId<<" ");
        return iStartTime;
    }

	const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		// have an invalid period type for the season
		ERR_LOG("[FifaCupsSlaveImpl].getSeasonStartTime(): unable to retrieve configuration for season id "<<seasonId<<" ");
		return iStartTime;
	}

    // the season start time is the same as the previous rollover time + blackout
    iStartTime = mTimeUtils.getPreviousRollOverTime(periodType, pSeasonConfig->getStatMode());

	TRACE_LOG("[FifaCupsSlaveImpl].getSeasonStartTime. start time = "<<iStartTime<<" ");

    return iStartTime;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonEndTime

    the time stamp of when the current season ends

*/
/*************************************************************************************************/
int64_t FifaCupsSlaveImpl::getSeasonEndTime(SeasonId seasonId)
{
    int64_t iEndTime = 0;

    // get the period type for the season
    Stats::StatPeriodType periodType = getSeasonPeriodType(seasonId);
    if (Stats::STAT_PERIOD_ALL_TIME == periodType)
    {
        // have an invalid period type for the season
        ERR_LOG("[FifaCupsSlaveImpl].getSeasonEndTime(): invalid season stat period type for season id "<<seasonId<<" ");
        return iEndTime;
    }

	const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		// have an invalid period type for the season
		ERR_LOG("[FifaCupsSlaveImpl].getSeasonEndTime(): unable to retrieve configuration for season id "<<seasonId<<" ");
		return iEndTime;
	}

    // the season end time is the same as the rollover time
    iEndTime = mTimeUtils.getRollOverTime(periodType, pSeasonConfig->getStatMode());

	TRACE_LOG("[FifaCupsSlaveImpl].getSeasonEndTime. end time = "<<iEndTime<<" ");

    return iEndTime;
}

/*************************************************************************************************/
/*!
    \brief  getNextCupStartTime

    the time stamp of when the next cup starts

*/
/*************************************************************************************************/
int64_t FifaCupsSlaveImpl::getNextCupStartTime(SeasonId seasonId)
{
    int64_t iStartTime = 0;

	// the start time of the next regular season is the current season's end time + the black out period
    iStartTime = getSeasonEndTime(seasonId);

    return iStartTime;
}

/*************************************************************************************************/
/*!
    \brief  getSeasonState

    gets the current state of the cup

*/
/*************************************************************************************************/
CupState FifaCupsSlaveImpl::getCupState(SeasonId seasonId)
{
    CupState cupState = FIFACUPS_CUP_STATE_NONE;

    int64_t iCurrentTime = mTimeUtils.getCurrentTime();
    mTimeUtils.logTime("[getCupState] current Time", iCurrentTime);
	TRACE_LOG("[FifaCupsSlaveImpl].getCupState. current time = "<<iCurrentTime<<" ");

    if (getSeasonStartTime(seasonId) <= iCurrentTime &&
        getCupStartTime(seasonId) > iCurrentTime)
    {
        // cup window closed;
        cupState = FIFACUPS_CUP_STATE_INACTIVE;
    }
	else if (getCupStartTime(seasonId) <= iCurrentTime &&
		getCupEndTime(seasonId) > iCurrentTime)
	{
		// cup window closed;
		cupState = FIFACUPS_CUP_STATE_ACTIVE;
	}
    else
    {
        // something's gone wrong in determining season state
        ERR_LOG("[FifaCupsSlaveImpl].getCupState(): unable to determine season state for season id "<<seasonId<<" ");
    }

    return cupState;
}

/*************************************************************************************************/
/*!
    \brief  isValidSeason

    validates that the season id is for a valid configured season

*/
/*************************************************************************************************/
bool8_t FifaCupsSlaveImpl::isValidSeason(SeasonId seasonId)
{
    bool8_t bValid = false;

    // get the replicated configuration from the slave
    FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap *pConfigMap = getReplicatedSeasonConfigurationMap();

	SeasonConfigurationServer *pSeason = pConfigMap->findItem(seasonId);
	if (pSeason != NULL)
	{
		bValid = true;
	}

    return bValid;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonId

    gets the season id of the season that the member is registered in

*/
/*************************************************************************************************/
SeasonId FifaCupsSlaveImpl::getSeasonId(MemberId memberId, MemberType memberType, BlazeRpcError &outError)
{
	SeasonId seasonId = 0;

    FifaCupsDb dbHelper = FifaCupsDb(this);

    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
    outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.fetchSeasonId(memberId, memberType, seasonId);
	}

    return seasonId;
}


/*************************************************************************************************/
/*!
    \brief  getRegistrationDetails

    gets the season id and qualifying div the member is registered in

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::getRegistrationDetails(MemberId memberId, MemberType memberType, SeasonId &outSeasonId, uint32_t &outQualifyingDiv, uint32_t &outPendingDiv, uint32_t &outActiveCupId, TournamentStatus &outTournamentStatus, BlazeRpcError &outError)
{
    FifaCupsDb dbHelper = FifaCupsDb(this);

    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
    outError = dbHelper.setDbConnection(dbConn);

	if (outError == Blaze::ERR_OK)
	{
		outError = dbHelper.fetchRegistrationDetails(memberId, memberType, outSeasonId, outQualifyingDiv, outPendingDiv, outActiveCupId, outTournamentStatus);
	}
}


/*************************************************************************************************/
/*!
    \brief  getTournamentDetails

    gets the cup id and tournament rule for the corresponding cup

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::getTournamentDetails(SeasonId seasonId, uint32_t qualifyingDiv, uint32_t &outTournamentId, uint32_t &outCupId, TournamentRule &outTournamentRule, BlazeRpcError &outError)
{
	outError = ERR_OK;

	outTournamentId = 0;
	outCupId = 0;
	outTournamentRule = FIFACUPS_TOURNAMENTRULE_ONE_ATTEMPT;

	const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		outError = FIFACUPS_ERR_SEASON_NOT_FOUND;
		return;
	}

	outTournamentId = static_cast<uint32_t>(pSeasonConfig->getTournamentId());

	const SeasonConfigurationServer::CupList &cupConfig = pSeasonConfig->getCups();
	SeasonConfigurationServer::CupList::const_iterator cupItr = cupConfig.begin();
	SeasonConfigurationServer::CupList::const_iterator cupItr_end = cupConfig.end();

	for (; cupItr != cupItr_end; cupItr++)
	{
		const Cup* cup = *cupItr;
		if (cup->getStartDiv() <= qualifyingDiv && cup->getEndDiv() >= qualifyingDiv)
		{
			// this returns strictly the cup for which the qualifying div satisfies both thresholds; 
			// in reality the user could select a cup of lower level (surfaced in activeCupId in fifacups_registration)
			outCupId = cup->getCupId();
			outTournamentRule = static_cast<TournamentRule>(cup->getTournamentRule());
		}
	}
}

int FifaCupsSlaveImpl::getCupIndexFromConfig(SeasonId seasonId, uint32_t cupId)
{
	const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		return -1;
	}
	const SeasonConfigurationServer::CupList &cupConfig = pSeasonConfig->getCups();

	int out = -1;
	for (uint32_t m = 0; m < cupConfig.size(); m++)
	{
		if (cupConfig[m]->getCupId() == cupId)
		{
			out = m;
			break;
		}
	}
	return out;
}

void FifaCupsSlaveImpl::calculateFutureStatPeriod(SeasonId seasonId, int periodsIntheFuture, TimeStamp& cupStartTime, TimeStamp& statPeriodStart, TimeStamp& statPeriodEnd)
{
	cupStartTime = getCupStartTime(seasonId);

	const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		return;
	}
	Stats::StatPeriodType seasonPeriodType = pSeasonConfig->getStatPeriodtype();
	statPeriodStart = mTimeUtils.getPreviousRollOverTime(seasonPeriodType);
	statPeriodEnd = mTimeUtils.getRollOverTime(seasonPeriodType);

	switch (seasonPeriodType)
	{
	case Stats::STAT_PERIOD_DAILY:
	{
		int64_t timeFutureAdjustment = periodsIntheFuture * mTimeUtils.getDurationInSeconds(1, 0, 0);
		cupStartTime += timeFutureAdjustment;
		statPeriodStart += timeFutureAdjustment;
		statPeriodEnd += timeFutureAdjustment;
		break;
	}
	case Stats::STAT_PERIOD_WEEKLY:
	{
		int64_t timeFutureAdjustment = periodsIntheFuture * mTimeUtils.getDurationInSeconds(7, 0, 0);
		cupStartTime += timeFutureAdjustment;
		statPeriodStart += timeFutureAdjustment;
		statPeriodEnd += timeFutureAdjustment;
		break;
	}
	case Stats::STAT_PERIOD_MONTHLY:
	{
		struct tm tM;
		TimeValue::gmTime(&tM, cupStartTime);
		tM.tm_mon += periodsIntheFuture;
		cupStartTime = TimeValue::mkgmTime(&tM);

		TimeValue::gmTime(&tM, statPeriodStart);
		tM.tm_mon += periodsIntheFuture;
		statPeriodStart = TimeValue::mkgmTime(&tM);

		TimeValue::gmTime(&tM, statPeriodEnd);
		tM.tm_mon += periodsIntheFuture;
		statPeriodEnd = TimeValue::mkgmTime(&tM);
		break;
	}
	default:
		break;
	}
}

int FifaCupsSlaveImpl::getCurrentActiveCupIndex(SeasonId seasonId)
{
	const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		return 0;
	}

	const SeasonConfigurationServer::CupList &cupConfig = pSeasonConfig->getCups();
	if (cupConfig.size() == 0)
	{
		return 0;
	}

	int activeCupIndex = 0;
	Stats::StatPeriodType seasonPeriodType = pSeasonConfig->getStatPeriodtype();
	struct tm tMCurrentTime;
	TimeValue::gmTime(&tMCurrentTime, mTimeUtils.getPreviousRollOverTime(pSeasonConfig->getStatPeriodtype(), pSeasonConfig->getStatMode()));  // converts the iEpochTime into the tm struct

	switch (seasonPeriodType)
	{
	case Stats::STAT_PERIOD_DAILY:
	{
		activeCupIndex = (tMCurrentTime.tm_yday % static_cast<int>(cupConfig.size()));
		break;
	}
	case Stats::STAT_PERIOD_WEEKLY:
	{
		int dayOfYear = tMCurrentTime.tm_yday;
		int weekOfYear = (int)(dayOfYear / 52);
		activeCupIndex = (weekOfYear % static_cast<int>(cupConfig.size()));
		break;
	}
	case Stats::STAT_PERIOD_MONTHLY:
	{
		activeCupIndex = (tMCurrentTime.tm_mon % static_cast<int>(cupConfig.size()));
		break;
	}
	default:
		WARN_LOG("[OSDKSeasonalPlaySlaveImpl].getCurrentActiveCupIndex. Invalid stat period ID " << seasonPeriodType << "!");
		break;
	}

	return activeCupIndex;
}

void FifaCupsSlaveImpl::getCupList(SeasonId seasonId, BlazeRpcError &outError, EA::TDF::TdfStructVector<Cup>& outCupList)
{
	outError = ERR_OK;

	const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		outError = FIFACUPS_ERR_SEASON_NOT_FOUND;
		return;
	}

	outCupList.clear();
	if (pSeasonConfig->getRotatingCupsCount() > 0)
	{
		const SeasonConfigurationServer::CupList &cupConfig = pSeasonConfig->getCups();

		int indexOfCups = getCurrentActiveCupIndex(seasonId);	//keeps track of the cups on rotation 

		int periodIndex = 0;					//keeps track of the period in the future
		TimeStamp cupStartTime = 0;
		TimeStamp statPeriodStart = 0;
		TimeStamp statPeriodEnd = 0;
		int exceptionOverrideCupIndex = -1;	
		while (outCupList.size() < pSeasonConfig->getRotatingCupsCount())
		{
			calculateFutureStatPeriod(seasonId, periodIndex, cupStartTime, statPeriodStart, statPeriodEnd);

			const Blaze::FifaCups::Instances::CupRotationExceptionList exceptionList = pSeasonConfig->getRotationExceptions();
			for (uint32_t k = 0; k < exceptionList.size(); k++)
			{
				TimeStamp exceptionTime = exceptionList[k]->getTimeInSeconds();
				if (exceptionTime >= statPeriodStart && exceptionTime < statPeriodEnd)
				{
					if (exceptionList[k]->getCupId() == cupConfig[indexOfCups]->getCupId())
					{
						exceptionOverrideCupIndex = k;
						continue;	//this case is overriding cup state, will be handled later 
					}
					else if (exceptionList[k]->getState() == Blaze::FifaCups::FIFACUPS_CUP_STATE_ACTIVE)
					{
						//add this exception in front of normal rotation cups
						int configIndex = getCupIndexFromConfig(seasonId, exceptionList[k]->getCupId());
						if (configIndex >= 0)
						{
							Blaze::FifaCups::Cup* pNewCup = outCupList.allocate_element();
							pNewCup->setState((periodIndex == 0) ? Blaze::FifaCups::FIFACUPS_CUP_STATE_ACTIVE : Blaze::FifaCups::FIFACUPS_CUP_STATE_INACTIVE);
							pNewCup->setTimeOpen(cupStartTime);
							pNewCup->setCupId(cupConfig[configIndex]->getCupId());
							pNewCup->setStartDiv(cupConfig[configIndex]->getStartDiv());
							pNewCup->setEndDiv(cupConfig[configIndex]->getEndDiv());
							pNewCup->setRuleId(cupConfig[configIndex]->getRuleId());
							outCupList.push_back(pNewCup);
						}
						else
						{
							WARN_LOG("[FifaCupsSlaveImpl:" << this << "].getCupList. Could not find cup Id from config: " << exceptionList[k]->getCupId() << "!");
						}
					}
				}
				else if (exceptionTime >= statPeriodEnd)
				{
					//handle in next stat period, exceptions are sorted from lowest to highest, so there wont be any processing needed for the rest of the list.
					break;
				}
			}

			bool insertCupToList = false;
			//if previously, we found this cup from the exception list, override state with exception list state
			if (exceptionList.size() > 0 && exceptionOverrideCupIndex >= 0)
			{
				if (exceptionList[exceptionOverrideCupIndex]->getState() == Blaze::FifaCups::FIFACUPS_CUP_STATE_ACTIVE)
				{
					insertCupToList = true;
				}
				//else dont add the regular rotation cup in the list, because its forced inactive
			}
			else 
			{
				//there is no exception override
				insertCupToList = true;
			}

			if (insertCupToList)
			{
				Blaze::FifaCups::Cup* pNewCup = outCupList.allocate_element();
				pNewCup->setState((periodIndex == 0) ? Blaze::FifaCups::FIFACUPS_CUP_STATE_ACTIVE : Blaze::FifaCups::FIFACUPS_CUP_STATE_INACTIVE);
				pNewCup->setTimeOpen(cupStartTime);
				pNewCup->setCupId(cupConfig[indexOfCups]->getCupId());
				pNewCup->setStartDiv(cupConfig[indexOfCups]->getStartDiv());
				pNewCup->setEndDiv(cupConfig[indexOfCups]->getEndDiv());
				pNewCup->setRuleId(cupConfig[indexOfCups]->getRuleId());
				outCupList.push_back(pNewCup);
			}
			
			indexOfCups++;
			if (indexOfCups >= static_cast<int32_t>(cupConfig.size()))		//take care of the cup rotation
			{
				indexOfCups = 0;
			}
			periodIndex++;							//next stat period for next cup
			exceptionOverrideCupIndex = -1;			//reset for next period
		}
	}

}

/*************************************************************************************************/
/*!
    \brief  getCupDetailsForMemberType

    gets the cup id and tournament rule for the corresponding cup

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::getCupDetailsForMemberType(MemberType memberType, SeasonId &outSeasonId, uint32_t &outStartDiv, uint32_t &outStartCupId, BlazeRpcError &outError)
{
	outError = ERR_OK;

	outSeasonId = 0;
	outStartDiv = 0;
	outStartCupId = 0;

	const SeasonConfigurationServer *pSeason = NULL;

	FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap *pConfigMap = getReplicatedSeasonConfigurationMap();
	if (pConfigMap->size() > 0)
	{
		FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr = pConfigMap->begin();
		FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr_end = pConfigMap->end();

		for( ; itr != itr_end; ++itr)
		{
			const SeasonConfigurationServer *pTempSeason = *itr;

			if (memberType == pTempSeason->getMemberType())
			{
				pSeason = pTempSeason;
				break;
			}
		}
	}

	if (NULL == pSeason)
	{
		outError = FIFACUPS_ERR_SEASON_NOT_FOUND;
		return;
	}

	outSeasonId = pSeason->getSeasonId();
	outStartDiv = pSeason->getStartDivision();

	const SeasonConfigurationServer::CupList &cupConfig = pSeason->getCups();
	SeasonConfigurationServer::CupList::const_iterator cupItr = cupConfig.begin();
	SeasonConfigurationServer::CupList::const_iterator cupItr_end = cupConfig.end();

	for (; cupItr != cupItr_end; cupItr++)
	{
		const Cup* cup = *cupItr;
		if (cup->getStartDiv() <= outStartDiv && cup->getEndDiv() >= outStartDiv)
		{
			// this returns strictly the cup for which the qualifying div satisfies both thresholds; 
			// in reality the user could select a cup of lower level (surfaced in activeCupId in fifacups_registration)
			outStartCupId = cup->getCupId();
		}
	}
}

/*************************************************************************************************/
/*!
    \brief  setMemberQualifyingDivision

    sets the tournament status of the member. Intended to be called when a member has changed divisions.

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::setMemberQualifyingDivision(MemberId memberId, MemberType memberType, uint32_t qualifyingDiv)
{
    TRACE_LOG("[FifaCupsSlaveImpl].setMemberQualifyingDivision. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", qualifyingDiv = "<<qualifyingDiv<<" ");

    BlazeRpcError error = Blaze::ERR_OK;

	//only allow div updates when cups window is inactive
	SeasonId seasonId = 0;
	uint32_t currrentQualifyingDiv = 0;
	uint32_t currentPendingDiv = 0;
	uint32_t activeCupId = 0;
	TournamentStatus status = FIFACUPS_TOURNAMENT_STATUS_NONE;
	getRegistrationDetails(memberId, memberType, seasonId, currrentQualifyingDiv, currentPendingDiv, activeCupId, status, error);

	if (Blaze::ERR_OK == error)
	{
		if (getCupState(seasonId) == FIFACUPS_CUP_STATE_INACTIVE)
		{
			//only want to update if it hasn't changed
			if (qualifyingDiv != currrentQualifyingDiv)
			{
				FifaCupsDb dbHelper = FifaCupsDb(this);
				DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
				error = dbHelper.setDbConnection(dbConn);

				if (Blaze::ERR_OK == error)
				{
					error = dbHelper.updateMemberQualifyingDiv(memberId, memberType, qualifyingDiv);
					if (Blaze::ERR_OK == error)
					{
						// record metrics
						mMetrics.mSetQualifyingDivCalls.add(1);
					}
					else
					{
						ERR_LOG("[FifaCupsSlaveImpl].setMemberQualifyingDivision. Error updating qualifying division. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", qualifyingDiv = "<<qualifyingDiv<<" ");
					}
				}
			}
		}
		else
		{
			if (qualifyingDiv != currentPendingDiv)
			{
				FifaCupsDb dbHelper = FifaCupsDb(this);
				DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
				error = dbHelper.setDbConnection(dbConn);

				if (Blaze::ERR_OK == error)
				{
					error = dbHelper.updateMemberPendingDiv(memberId, memberType, qualifyingDiv);
					if (Blaze::ERR_OK == error)
					{
						// record metrics
						mMetrics.mSetPendingDivCalls.add(1);
					}
					else
					{
						ERR_LOG("[FifaCupsSlaveImpl].setMemberQualifyingDivision. Error updating pending division. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", qualifyingDiv = "<<qualifyingDiv<<" ");
					}
				}
			}
		}
	}
	else
	{
		ERR_LOG("[FifaCupsSlaveImpl].setMemberQualifyingDivision. Error retrieving existing qualifying division for member. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<" ");
	}

    return error;
}

/*************************************************************************************************/
/*!
    \brief  setMemberActiveCupId

    sets the active cup id

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::setMemberActiveCupId(MemberId memberId, MemberType memberType, uint32_t activeCupId)
{
	TRACE_LOG("[FifaCupsSlaveImpl].setMemberActiveCupId. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", activeCupId = "<<activeCupId<<" ");

	BlazeRpcError error = Blaze::ERR_OK;

	FifaCupsDb dbHelper = FifaCupsDb(this);
	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	error = dbHelper.setDbConnection(dbConn);

	if (Blaze::ERR_OK == error)
	{
		error = dbHelper.updateMemberActiveCupId(memberId, memberType, activeCupId);
		if (Blaze::ERR_OK == error)
		{
			//// invoke the custom processing hook for tournament status updates
			//onTournamentStatusUpdate(memberId, memberType, tournamentStatus);

			// record metrics
			mMetrics.mSetActiveCupIdCalls.add(1);
		}
		else
		{
			ERR_LOG("[FifaCupsSlaveImpl].updateMemberActiveCupId. Error updating active cup id. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", activeCupId = "<<activeCupId<<" ");
		}
	}

	return error;
}


/*************************************************************************************************/
/*!
    \brief  getMemberTournamentStatus

    gets the tournament status of the member

*/
/*************************************************************************************************/
TournamentStatus FifaCupsSlaveImpl::getMemberTournamentStatus(MemberId memberId, MemberType memberType, BlazeRpcError &outError)
{
    TournamentStatus tournStatus = FIFACUPS_TOURNAMENT_STATUS_NONE;

	FifaCupsDb dbHelper = FifaCupsDb(this);

    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
    outError = dbHelper.setDbConnection(dbConn);

	if (Blaze::ERR_OK == outError)
	{
	    outError = dbHelper.fetchTournamentStatus(memberId, memberType, tournStatus);
	}

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
BlazeRpcError FifaCupsSlaveImpl::setMemberTournamentStatus(MemberId memberId, MemberType memberType, TournamentStatus tournamentStatus, DbConnPtr conn)
{
    TRACE_LOG("[FifaCupsSlaveImpl].setMemberTournamentStatus. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", status = "<<TournamentStatusToString(tournamentStatus)<<" ");

    BlazeRpcError error = Blaze::ERR_OK;

	FifaCupsDb dbHelper = FifaCupsDb(this);
	DbConnPtr dbConn = conn;
	if (dbConn == NULL)
	{
		dbConn = gDbScheduler->getConnPtr(getDbId());
	}
    
	error = dbHelper.setDbConnection(dbConn);
	if (Blaze::ERR_OK == error)
	{
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
					onTournamentStatusUpdate(memberId, memberType, tournamentStatus);

					// record metrics
					mMetrics.mSetTournamentStatusCalls.add(1);
				}
				else
				{
					ERR_LOG("[FifaCupsSlaveImpl].setMemberTournamentStatus. Error updating tournament status. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", status = "<<TournamentStatusToString(tournamentStatus)<<" ");
				}
			}
		}
		else
		{
			ERR_LOG("[FifaCupsSlaveImpl].setMemberTournamentStatus. Error retrieving existing tournament status for member.memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<" ");
		}
	}

    return error;
}


/*************************************************************************************************/
/*!
    \brief  updateMemberCupCount

    increment the cup stat for the given user.

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::updateMemberCupCount(MemberId memberId, MemberType memberType, SeasonId seasonId)
{
	const SeasonConfigurationServer* config = getSeasonConfiguration(seasonId);
	return updateMemberCupStats(memberId, memberType, config->getStatCategoryName(), "cupsWon");
}


/*************************************************************************************************/
/*!
    \brief  updateMemberElimCount

    increment the elim stat for the given user.

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::updateMemberElimCount(MemberId memberId, MemberType memberType, SeasonId seasonId)
{
	const SeasonConfigurationServer* config = getSeasonConfiguration(seasonId);
	return updateMemberCupStats(memberId, memberType, config->getStatCategoryName(), "cupsElim");
}


/*************************************************************************************************/
/*!
    \brief  updateMemberCupStats

    increment the given stat for the given user.

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::updateMemberCupStats(MemberId memberId, MemberType memberType, const char* userStatsCategory, const char* userStatsName)
{
	TRACE_LOG("[FifaCupsSlaveImpl].updateMemberCupStats. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<" ");

	BlazeRpcError error = Blaze::ERR_OK;

	Blaze::OSDKTournaments::TournamentModes tournamentMode = Blaze::OSDKTournaments::INVALID_MODE;
	if (memberType == FIFACUPS_MEMBERTYPE_USER)
	{
		tournamentMode = Blaze::OSDKTournaments::MODE_FIFA_CUPS_USER;
	}
	else if (memberType == FIFACUPS_MEMBERTYPE_CLUB)
	{
		tournamentMode = Blaze::OSDKTournaments::MODE_FIFA_CUPS_CLUB;
	}
	else if (memberType == FIFACUPS_MEMBERTYPE_COOP)
	{
		tournamentMode = Blaze::OSDKTournaments::MODE_FIFA_CUPS_COOP;
	}

	FifaCupsDb dbHelper = FifaCupsDb(this);
	DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
	error = dbHelper.setDbConnection(dbConn);

	Blaze::Stats::StatsSlave *statsComponent =
		static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));

	if (statsComponent == NULL)
	{
		TRACE_LOG("[FifaCupsSlaveImpl].updateMemberCupStats. stats component not found.");
		error = FIFACUPS_ERR_GENERAL;
	}
	
	Blaze::OSDKTournaments::OSDKTournamentsSlave *osdkTournamentComponent =
		static_cast<Blaze::OSDKTournaments::OSDKTournamentsSlave*>(gController->getComponent(Blaze::OSDKTournaments::OSDKTournamentsSlave::COMPONENT_ID,false));

	if (osdkTournamentComponent == NULL)
	{
		TRACE_LOG("[FifaCupsSlaveImpl].updateMemberCupStats. osdkTournament component not found.");
		error = FIFACUPS_ERR_GENERAL;
	}

	if (Blaze::ERR_OK == error)
	{
		// get the qualifying division of the user
		SeasonId seasonId;
		uint32_t qualifyingDiv;
		uint32_t pendingDiv;
		uint32_t activeCupId;
		int32_t ruleId = -1;

		TournamentStatus status;

		error = dbHelper.fetchRegistrationDetails(memberId, memberType, seasonId, qualifyingDiv, pendingDiv, activeCupId, status);

		const SeasonConfigurationServer *pSeasonConfig = NULL;
		if (Blaze::ERR_OK == error)
		{
			pSeasonConfig = getSeasonConfiguration(seasonId);
			if (NULL == pSeasonConfig)
			{
				error = Blaze::ERR_SYSTEM;
			}
		}

		if (Blaze::ERR_OK == error)
		{
			if (memberType == FIFACUPS_MEMBERTYPE_CLUB)
			{
				SeasonConfigurationServer::CupList::const_iterator cupIter = pSeasonConfig->getCups().begin();
				SeasonConfigurationServer::CupList::const_iterator cupEnd = pSeasonConfig->getCups().end();
				for (; cupIter != cupEnd; cupIter++)
				{
					const Cup* cup = *cupIter;
					if (cup && cup->getCupId() == activeCupId)
					{
						ruleId = cup->getRuleId();
						break;
					}
				}
			}

			eastl::string tempStatName;
			tempStatName.sprintf("%s%d", userStatsName, ruleId>=0 ? ruleId : activeCupId);

			char8_t buffer[BUFFER_SIZE_64];

			Stats::StatRowUpdate* statRow;
			Stats::StatUpdate* statUpdate;

			statRow = BLAZE_NEW Stats::StatRowUpdate;
			statRow->setCategory(userStatsCategory);
			statRow->setEntityId(memberId);

			statUpdate = BLAZE_NEW Stats::StatUpdate;
			statUpdate->setName(tempStatName.c_str());
			statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_INCREMENT);
			blaze_snzprintf(buffer, BUFFER_SIZE_64, "%d", 1);
			statUpdate->setValue(buffer);
			statRow->getUpdates().push_back(statUpdate);

			if (tournamentMode != Blaze::OSDKTournaments::INVALID_MODE && blaze_stricmp(userStatsName, "cupsElim") == 0)
			{
				Blaze::OSDKTournaments::TournamentMemberData getMemStatusResponse;
				Blaze::OSDKTournaments::GetMemberStatusRequest getMemStatusRequest;
				getMemStatusRequest.setMemberId(memberId);
				getMemStatusRequest.setMode(tournamentMode);
				osdkTournamentComponent->getMemberStatus(getMemStatusRequest, getMemStatusResponse);

				eastl::string tempStatNameRound;
				tempStatNameRound.sprintf("%s%dR%d", userStatsName, ruleId >= 0 ? ruleId : activeCupId, getMemStatusResponse.getLevel());
				
				Stats::StatUpdate* statUpdateRound;
				statUpdateRound = BLAZE_NEW Stats::StatUpdate;
				statUpdateRound->setName(tempStatNameRound.c_str());
				statUpdateRound->setUpdateType(Stats::STAT_UPDATE_TYPE_INCREMENT);					
				statUpdateRound->setValue(buffer);

				statRow->getUpdates().push_back(statUpdateRound);
			}

			Blaze::Stats::UpdateStatsRequest request;
			request.getStatUpdates().push_back(statRow);

			UserSession::pushSuperUserPrivilege();
			error = statsComponent->updateStats(request);
			UserSession::popSuperUserPrivilege();

			if (Blaze::ERR_OK != error)
			{
				ERR_LOG("[FifaCupsSlaveImpl].updateMemberCupStats. update stats RPC failed.");
			}
		}
		else
		{
			ERR_LOG("[FifaCupsSlaveImpl].updateMemberCupStats. Error retrieving qualifying division or season config for member. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", cat = "<<userStatsCategory<<", statname = "<<userStatsName <<" ");
		}
	}

	return error;
}

/*************************************************************************************************/
/*!
    \brief  updateMemberCupsEntered

    increment the  stat CupsEntered for the given user.

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::updateMemberCupsEntered(MemberId memberId, MemberType memberType, SeasonId seasonId)
{
	TRACE_LOG("[FifaCupsSlaveImpl].updateMemberCupsEntered. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<" ");

	BlazeRpcError error = Blaze::ERR_OK;

	FifaCupsDb dbHelper = FifaCupsDb(this);
	DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
	error = dbHelper.setDbConnection(dbConn);

	Blaze::Stats::StatsSlave *statsComponent =
		static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));

	if (statsComponent == NULL)
	{
		TRACE_LOG("[FifaCupsSlaveImpl].updateMemberCupStats. stats component not found.");
		error = FIFACUPS_ERR_GENERAL;
	}

	const SeasonConfigurationServer *pSeasonConfig = NULL;
	pSeasonConfig = getSeasonConfiguration(seasonId);
	if (NULL == pSeasonConfig)
	{
		TRACE_LOG("[FifaCupsSlaveImpl].updateMemberCupsEntered. season configuration not found.");
		error = Blaze::FIFACUPS_ERR_GENERAL;
	}

	if (Blaze::ERR_OK == error)
	{
		char8_t buffer[BUFFER_SIZE_64];

		Stats::StatRowUpdate* statRow;
		Stats::StatUpdate* statUpdate;

		statRow = BLAZE_NEW Stats::StatRowUpdate;
		statRow->setCategory(pSeasonConfig->getStatCategoryName());
		statRow->setEntityId(memberId);

		statUpdate = BLAZE_NEW Stats::StatUpdate;
		statUpdate->setName("cupsEntered");
		statUpdate->setUpdateType(Stats::STAT_UPDATE_TYPE_INCREMENT);
		blaze_snzprintf(buffer, BUFFER_SIZE_64, "%d", 1);
		statUpdate->setValue(buffer);
		statRow->getUpdates().push_back(statUpdate);

		Blaze::Stats::UpdateStatsRequest request;
		request.getStatUpdates().push_back(statRow);

		UserSession::pushSuperUserPrivilege();
		error = statsComponent->updateStats(request);
		UserSession::popSuperUserPrivilege();

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[FifaCupsSlaveImpl].updateMemberCupsEntered. update stats RPC failed.");
		}
	}
	else
	{
		ERR_LOG("[FifaCupsSlaveImpl].updateMemberCupsEntered. Error retrieving qualifying division or season config for member. memberId = "<<memberId<<", memberType = "<<MemberTypeToString(memberType)<<", cat = SPOverallPlayerStats, statname = cupsEntered");
	}
	return error;
}




/*************************************************************************************************/
/*!
    \brief  resetSeasonTournamentStatus

    resets the tournament status of all members in the season to "SEASONALPLAY_TOURNAMENT_STATUS_NONE"

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::resetSeasonTournamentStatus(SeasonId seasonId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].resetSeasonTournamentStatus. seasonId = "<<seasonId<<" ");

    BlazeRpcError error = Blaze::ERR_OK;

    FifaCupsDb dbHelper = FifaCupsDb(this);

    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());

    error = dbHelper.setDbConnection(dbConn);
	if (Blaze::ERR_OK == error)
	{
		error = dbHelper.updateSeasonTournamentStatus(seasonId, FIFACUPS_TOURNAMENT_STATUS_NONE);

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[FifaCupsSlaveImpl].resetSeasonTournamentStatus. Error reseting season tournament status. seasonId ="<<seasonId<<" ");
		}
		else
		{
			// record metrics
			mMetrics.mResetTournamentStatusCalls.add(1);
		}
	}

    return error;
}

/*************************************************************************************************/
/*!
    \brief  resetSeasonActiveCupId

    resets the active cup id status of all members in the season to 0

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::resetSeasonActiveCupId(SeasonId seasonId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].resetSeasonActiveCupId. seasonId = "<<seasonId<<" ");

    BlazeRpcError error = Blaze::ERR_OK;

    FifaCupsDb dbHelper = FifaCupsDb(this);

    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());

    error = dbHelper.setDbConnection(dbConn);
	if (Blaze::ERR_OK == error)
	{
		error = dbHelper.updateSeasonsActiveCupId(seasonId, 0);

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[FifaCupsSlaveImpl].resetSeasonActiveCupId. Error reseting season active cup id. seasonId ="<<seasonId<<" ");
		}
		else
		{
			// record metrics
			mMetrics.mResetSeasonActiveCupIdCalls.add(1);
		}
	}

    return error;
}

/*************************************************************************************************/
/*!
    \brief  copySeasonPendingDivision

    copy the pending division for all members of a season to the qualifying division

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::copySeasonPendingDivision(SeasonId seasonId)
{
	TRACE_LOG("[FifaCupsSlaveImpl].copySeasonPendingDivision. seasonId = "<<seasonId<<" ");

	BlazeRpcError error = Blaze::ERR_OK;

	FifaCupsDb dbHelper = FifaCupsDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	error = dbHelper.setDbConnection(dbConn);

	if (Blaze::ERR_OK == error)
	{
		error = dbHelper.updateCopyPendingDiv(seasonId);

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[FifaCupsSlaveImpl].copySeasonPendingDivision. Error copying the pending division - "<<seasonId<<" ");
		}
		else
		{
			// record metrics
			mMetrics.mCopyPendingDiv.add(1);
		}
	}

	return error;
}

/*************************************************************************************************/
/*!
    \brief  clearSeasonPendingDivision

    clear the pending division for all members of a season to the supplied value

*/
/*************************************************************************************************/
BlazeRpcError FifaCupsSlaveImpl::clearSeasonPendingDivision(SeasonId seasonId, uint32_t clearValue)
{
	TRACE_LOG("[FifaCupsSlaveImpl].clearSeasonPendingDivision. seasonId = "<<seasonId<<" ");

	BlazeRpcError error = Blaze::ERR_OK;

	FifaCupsDb dbHelper = FifaCupsDb(this);

	DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
	error = dbHelper.setDbConnection(dbConn);

	if (Blaze::ERR_OK == error)
	{
		error = dbHelper.updateClearPendingDiv(seasonId, clearValue);

		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[FifaCupsSlaveImpl].updateClearPendingDiv. Error clear pending division. seasonId = "<<seasonId<<" ");
		}
		else
		{
			// record metrics
			mMetrics.mClearPendingDiv.add(1);
		}
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
bool8_t FifaCupsSlaveImpl::getIsTournamentEligible(SeasonId seasonId, TournamentRule tournamentRule, TournamentStatus tournamentStatus)
{
    bool8_t eligible = false;

    // get the season state
    CupState cupState = getCupState(seasonId);

    // continue if the season state is in playoffs
    if (FIFACUPS_CUP_STATE_ACTIVE == cupState)
    {
		TournamentRule tournRule = tournamentRule;
		TournamentStatus memberTournStatus = tournamentStatus;
		if (FIFACUPS_TOURNAMENTRULE_UNLIMITED == tournRule)
		{
			// unlimited attempts
			eligible = true;
		}
		else if (FIFACUPS_TOURNAMENTRULE_RETRY_ON_LOSS == tournRule)
		{
			// eligible if status is not won
			eligible = (FIFACUPS_TOURNAMENT_STATUS_WON != memberTournStatus) ? true : false;
		}
		else if (FIFACUPS_TOURNAMENTRULE_ONE_ATTEMPT == tournRule)
		{
			// eligible if status is none
			eligible = (FIFACUPS_TOURNAMENT_STATUS_NONE == memberTournStatus) ? true : false;
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
TournamentId FifaCupsSlaveImpl::getSeasonTournamentId(SeasonId seasonId)
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
    \brief  getRegisteredMembers

    gets a list of all the members registered in the season

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::getRegisteredMembers(SeasonId seasonId,  eastl::vector<MemberId> &outMemberList)
{
    BlazeRpcError error = Blaze::ERR_OK;

    FifaCupsDb dbHelper = FifaCupsDb(this);
    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
    error = dbHelper.setDbConnection(dbConn);

	if (Blaze::ERR_OK == error)
	{
		error = dbHelper.fetchSeasonMembers(seasonId, outMemberList);
		if (Blaze::ERR_OK != error)
		{
			ERR_LOG("[FifaCupsSlaveImpl].getRegisteredMembers. Error retrieving member list for season id "<<seasonId<<" Error = "<<ErrorHelp::getErrorName(error)<<" ");
		}
	}
}


/*************************************************************************************************/
/*!
    \brief  getSeasonMemberType

    gets the member type associated with the season

*/
/*************************************************************************************************/
MemberType FifaCupsSlaveImpl::getSeasonMemberType(SeasonId seasonId)
{
    MemberType memberType = FIFACUPS_MEMBERTYPE_USER;

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
LeagueId FifaCupsSlaveImpl::getSeasonLeagueId(SeasonId seasonId)
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

/*************************************************************************************************/
/*!
    \brief  onConfigureReplication

    lifecycle method: called when the component is ready to listen to replicated events.
    Register as a ReplicatedSeasonConfigurationMap subscriber.

*/
/*************************************************************************************************/
bool FifaCupsSlaveImpl::onConfigureReplication()
{
    TRACE_LOG("[FifaCupsSlaveImpl].onConfigureReplication");

    getReplicatedSeasonConfigurationMap()->addSubscriber(*this);

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onConfigure

    lifecycle method: called when the slave is to configure itself. registers for stats
    and usersession events. 

*/
/*************************************************************************************************/
bool FifaCupsSlaveImpl::onConfigure()
{
	bool success = configureHelper();
	TRACE_LOG("[FifaCupsSlaveImpl:" << this << "].onConfigure: configuration " << (success ? "successful." : "failed."));
	return success;
}


/*************************************************************************************************/
/*!
    \brief  onReconfigure

    lifecycle method: called when the slave is to reconfigure itself. unregisters from stats
    and usersession events before calling onConfigure

*/
/*************************************************************************************************/
bool FifaCupsSlaveImpl::onReconfigure()
{
    TRACE_LOG("[FifaCupsSlaveImpl].onReconfigure");

	// stop listening to rollover and user session events as we'll re-subscribe to them in onConfigure

	// unhook stats period rollover event
	Stats::StatsMaster* statsComponent
		= static_cast<Stats::StatsMaster*>(
		gController->getComponent(Stats::StatsMaster::COMPONENT_ID, false));
	if (NULL != statsComponent)
	{
		ERR_LOG("[FifaCupsSlaveImpl:" << this << "].onReconfigure - Stats component not found.");
		statsComponent->removeNotificationListener(*this);
	}

	// stop listening to user session events
	gUserSessionManager->removeSubscriber(*this);

	bool success = configureHelper(statsComponent);
	TRACE_LOG("[FifaCupsSlaveImpl:" << this << "].onReconfigure: reconfiguration " << (success ? "successful." : "failed."));
	return success;
}

/*************************************************************************************************/
/*!
    \brief configureHelper

    Helper function which performs tasks common to onConfigure and onReconfigure

*/
/*************************************************************************************************/
bool FifaCupsSlaveImpl::configureHelper(Stats::StatsMaster* statsComponent)
{
	TRACE_LOG("[FifaCupsSlaveImpl].onConfigure");

	const char8_t* dbName = getConfig().getDbName();
	mDbId = gDbScheduler->getDbId(dbName);
	if(mDbId == DbScheduler::INVALID_DB_ID)
	{
		ERR_LOG("[FifaCupsSlaveImpl].onConfigure(): Failed to initialize db");
		return false;
	}

	FifaCupsDb::initialize(mDbId);

	// check if auto user/club registration is enabled
	FifaCupsConfig config = getConfig();

	mAutoUserRegistrationEnabled = config.getAutoUserRegistration();
	mAutoClubRegistrationEnabled = config.getAutoClubRegistration();

	// hookup with StatsMasterListener event
	if (statsComponent == nullptr)
	{
		BlazeRpcError waitResult = ERR_OK;
		statsComponent = static_cast<Stats::StatsMaster*>(gController->getComponent(Stats::StatsMaster::COMPONENT_ID, false, true, &waitResult));
		if (ERR_OK != waitResult)
		{
			ERR_LOG("[FifaCupsSlaveImpl:" << this << "].configureHelper - Stats component not found.");
			return false;
		}
	}

	if (NULL != statsComponent)
	{
		statsComponent->addNotificationListener(*this);
		if (!statsComponent->isLocal())
		{
			// subscribe for stats notifications
			BlazeRpcError subscribeError = statsComponent->notificationSubscribe();
			if (ERR_OK != subscribeError)
			{
				ERR_LOG("[FifaCupsSlaveImpl:" << this << "].configureHelper - Stats component couldn't subscribe for stats notifications.  Error="<<subscribeError);
				return false;
			}
		}

		mRefreshPeriodIds = true;
	}
	else
	{
		ERR_LOG("[FifaCupsSlaveImpl:" << this << "].configureHelper: FifaCups requires the stats component to exist.");
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
bool FifaCupsSlaveImpl::onResolve()
{
    TRACE_LOG("[FifaCupsSlaveImpl].onResolve");

    // register for master notifications
    getMaster()->addNotificationListener(*this);

    return true;
}


/*************************************************************************************************/
/*!
    \brief  onShutdown

    lifecycle method: called when the slave is shutting down. need to unhook our event listeners

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onShutdown()
{
    TRACE_LOG("[FifaCupsSlaveImpl].onShutdown");

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
    \brief  getStatusINFO_LOG(

    Called periodically to collect status and statistics about this component that can be used 
    to determine what future improvements should be made.

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::getStatusInfo(ComponentStatus& status) const
{
    TRACE_LOG("[FifaCupsSlaveImpl].getStatusInfo");

    mMetrics.report(&status);
}


/*************************************************************************************************/
/*!
    \brief  onInsert

    Called when the slave receives a new SeasonConfiguration entry in the replicated season
    configuration map

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onInsert(const ReplicatedSeasonConfigurationMap& replMap, ObjectId key, ReplicatedMapItem<Blaze::FifaCups::SeasonConfigurationServer>& tdfObject)
{
    TRACE_LOG("[FifaCupsSlaveImpl].onInsert. Got replicated config for season id "<< tdfObject.getSeasonId()<<" ");

    gSelector->scheduleFiberCall<FifaCupsSlaveImpl, SeasonId>(
        this, 
		&FifaCupsSlaveImpl::checkMissedEndOfSeasonProcessing,
        tdfObject.getSeasonId(), "FifaCupsSlaveImpl::onInsert");
}


/*************************************************************************************************/
/*!
    \brief  checkMissedEndOfSeasonProcessing

    Checks if we missed processing the end of season for the specified season and initiates it
    if need be

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::checkMissedEndOfSeasonProcessing(SeasonId seasonId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].checkMissedEndOfSeasonProcessing. Season id "<<seasonId<<" ");

    BlazeRpcError error = Blaze::ERR_OK;

    // we don't want to do anything until the seasonal play slave is in it's activated state so 
    // put this fiber to sleep until that happens
    while(getState() != Blaze::ComponentStub::ACTIVE)
    {
        TRACE_LOG("[FifaCupsSlaveImpl].checkMissedEndOfSeasonProcessing. seasonal play component not activated... sleeping");
        error = gSelector->sleep(1000 * 1000);
        if (error != ERR_OK)
        {
            ERR_LOG("[FifaCupsSlaveImpl].checkMissedEndOfSeasonProcessing. Got sleep error "<<ErrorHelp::getErrorName(error)<<"!");
            return;
        }
    }

    // get the stats component for retrieving the period id's.
    Stats::StatsSlave* statsComponent
        = static_cast<Stats::StatsSlave*>(
        gController->getComponent(Stats::StatsSlave::COMPONENT_ID, false));
    if (NULL == statsComponent)
    {
        // early return because we don't have access to stats
        return;
    }

    // get the season configuration for the season
    const SeasonConfigurationServer *pSeason = getSeasonConfiguration(seasonId);
	
	if(NULL == pSeason)
	{
		//early return because pSeason is not accessible.
		return;
	}

	// the current period id for the season's period type
	Stats::PeriodIds ids;
	error = statsComponent->getPeriodIds(ids);
	if (Blaze::ERR_OK != error)
	{
		ERR_LOG("[OSDKSeasonalPlaySlaveImpl:" << this << "].checkMissedEndOfSeasonProcessing. Unable to get PeriodIds");
		return;
	}

    // the current period id for the season's period type
    Stats::StatPeriodType seasonPeriodType = pSeason->getStatPeriodtype();
	uint32_t currentPeriodId = 0;
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

    FifaCupsDb dbHelper = FifaCupsDb(this);
    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(getDbId());
    error = dbHelper.setDbConnection(dbConn);
	if (error != ERR_OK)
	{
		ERR_LOG("[FifaCupsSlaveImpl].checkMissedEndOfSeasonProcessing - was not able to get read conn ptr!");
		return;
	}

    // when end of season processing successfully completes, we update the season in the database with
    // the period id that we just rolled over to. By comparing this period id with the current period id
    // from the stats component, we can determine if we need to do end of season processing (which would
    // be the case if the server was down when the stats rollover happened).
    uint32_t seasonNumber = 0;
    uint32_t seasonPeriodId = 0;
    error = dbHelper.fetchSeasonNumber(seasonId, seasonNumber, seasonPeriodId);

    TRACE_LOG("[FifaCupsSlaveImpl].checkMissedEndOfSeasonProcessing. checking End of Season processing required. season id = "<<seasonId<<", season period id = "<<seasonPeriodId<<", current period = "<<currentPeriodId<<" ");

    // if the current period id is greater than the season's period id, then we missed doing the end of season processing.
    if (Blaze::ERR_OK == error && currentPeriodId > seasonPeriodId)
    {
        // need to kick of end of season processing
        INFO_LOG("[FifaCupsSlaveImpl].checkMissedEndOfSeasonProcessing. Need end of season processing for season id "<<seasonId<<" ");

        // INFO_LOG(rm the master that end of season processing is needed.
        EndOfSeasonRolloverData rolloverData;

        rolloverData.setStatPeriodType(seasonPeriodType);
        rolloverData.setOldPeriodId(seasonPeriodId);
        rolloverData.setNewPeriodId(currentPeriodId);

        error = getMaster()->startEndOfSeasonProcessing(rolloverData);
        if (Blaze::ERR_OK != error)
        {
            ERR_LOG("[FifaCupsSlaveImpl].checkMissedEndOfSeasonProcessing: failed to notify master to start end of season processing. Error = "<<ErrorHelp::getErrorName(error)<<" "); 
        }

        // record metrics
        mMetrics.mMissedEndOfSeasonProcessingDetected.add(1);
    }
    else
    {
        TRACE_LOG("[FifaCupsSlaveImpl].checkMissedEndOfSeasonProcessing. End of Season processing NOT required for season id "<<seasonId<<" ");
    }
}


/*************************************************************************************************/
/*!
    \brief  onProcessEndOfSeasonNotification

    receives notification from the master to begin end of season processing

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onProcessEndOfSeasonNotification(const EndOfSeasonRolloverData &rolloverData, UserSession* /*associatedUserSession*/)
{
    INFO_LOG("[FifaCupsSlaveImpl].onProcessEndOfSeasonNotification: received process end of season notification from master. period = "<<Stats::StatPeriodTypeToString(rolloverData.getStatPeriodType())<<", oldPeriodId = "<<rolloverData.getOldPeriodId()<<", newPeriodId = "<<rolloverData.getNewPeriodId()<<" ");

    // kick off the actual end of season processing in a fiber.
    gSelector->scheduleFiberCall<FifaCupsSlaveImpl, Stats::StatPeriodType, int32_t, int32_t>(
        this, 
        &FifaCupsSlaveImpl::processEndOfSeason,
        rolloverData.getStatPeriodType(),
        rolloverData.getOldPeriodId(),
        rolloverData.getNewPeriodId(),
		"FifaCupsSlaveImpl::onProcessEndOfSeasonNotification");
}


/*************************************************************************************************/
/*!
    \brief  processEndOfSeason
    
    does the actual end of season processing

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::processEndOfSeason(Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId)
{
    INFO_LOG("[FifaCupsSlaveImpl].processEndOfSeason: processing end of season. period = "<<Stats::StatPeriodTypeToString(periodType)<<", oldPeriodId = "<<oldPeriodId<<", newPeriodId = "<<newPeriodId<<" ");

    // check if the old and new period id's match. If they do then we don't want to do end of season processing
    // (possible edge case during server startup when checking if end of season processing was missed)
    if (oldPeriodId == newPeriodId)
    {
        INFO_LOG("[FifaCupsSlaveImpl].processEndOfSeason: skipping end of season processing as old and new period ids match.");
        return;
    }

    // record metrics - don't care about success/failure; only that it was attempted
    mMetrics.mEndOfSeasonProcessed.add(1);

    BlazeRpcError error = Blaze::ERR_OK;

	// find all seasons that match the period type and invoke the custom season roll over processing 
	FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap *pConfigMap = getReplicatedSeasonConfigurationMap();
	if (pConfigMap->size() > 0)
	{
		FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr = pConfigMap->begin();
		FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr_end = pConfigMap->end();

		for( ; itr != itr_end; ++itr)
		{
			const SeasonConfigurationServer *pTempSeason = *itr;

			if(pTempSeason->getStatPeriodtype() == periodType)
			{
				// copy over any pending division entries
				copySeasonPendingDivision(pTempSeason->getSeasonId());

				// clear the pending division entries
				clearSeasonPendingDivision(pTempSeason->getSeasonId(), 0);

				// reset the members tournament status
				resetSeasonTournamentStatus(pTempSeason->getSeasonId());

				// reset the active cup ids
				resetSeasonActiveCupId(pTempSeason->getSeasonId());

				// invoke the reset tournament custom hook
				onResetTournament(pTempSeason->getSeasonId(), pTempSeason->getTournamentId());
			}
			else
			{
				ERR_LOG("[FifaCupsSlaveImpl].onStatsRollover: error retrieving season number for season id "<<pTempSeason->getSeasonId()<<" ");
			}
		}
	}

	// notify the master that end of season processing is done.
    EndOfSeasonRolloverData rolloverData;

    rolloverData.setStatPeriodType(periodType);
    rolloverData.setOldPeriodId(oldPeriodId);
    rolloverData.setNewPeriodId(newPeriodId);

    error = getMaster()->finishedEndOfSeasonProcessing(rolloverData);
    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[FifaCupsSlaveImpl].processEndOfSeason: failed to notify master that end of season processing finished. Error = "<<ErrorHelp::getErrorName(error)<<" "); 
    }
}

/*************************************************************************************************/
/*!
    \brief  onSetPeriodIdsNotification

    receives notification from the stats component that a stat period has rolled over

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onSetPeriodIdsNotification(const Stats::PeriodIds& data, UserSession *)
{
    gSelector->scheduleFiberCall(this, &FifaCupsSlaveImpl::fireRolloverEvent, (Stats::PeriodIds)data, "FifaCupsSlaveImpl::fireRolloverEvent");
}

/*************************************************************************************************/
/*!
    \brief  fireRolloverEvent

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::fireRolloverEvent(Stats::PeriodIds newIds)
{
	if (mRefreshPeriodIds == true)
	{
		mRefreshPeriodIds = false;

		Stats::StatsMaster* statsComponent	= static_cast<Stats::StatsMaster*>(gController->getComponent(Stats::StatsMaster::COMPONENT_ID, false));
		if (NULL != statsComponent)
		{
			// cache period ids
			UserSession::pushSuperUserPrivilege();
			statsComponent->getPeriodIdsMaster(mOldPeriodIds);
			UserSession::popSuperUserPrivilege();
		}
	}

	// update the cached ids
	Stats::PeriodIds oldIds;
	mOldPeriodIds.copyInto(oldIds);
	newIds.copyInto(mOldPeriodIds);

	INFO_LOG("[FifaCupsSlaveImpl:" << this << "].fireRolloverEvent.  daily: " << newIds.getCurrentDailyPeriodId() << "  weekly: " 
		<< newIds.getCurrentWeeklyPeriodId() << " monthly: " << newIds.getCurrentMonthlyPeriodId());

	if (oldIds.getCurrentDailyPeriodId() != newIds.getCurrentDailyPeriodId())
		onStatsRollover(Stats::STAT_PERIOD_DAILY, oldIds.getCurrentDailyPeriodId(), newIds.getCurrentDailyPeriodId());
	if (oldIds.getCurrentWeeklyPeriodId() != newIds.getCurrentWeeklyPeriodId())
		onStatsRollover(Stats::STAT_PERIOD_WEEKLY, oldIds.getCurrentWeeklyPeriodId(), newIds.getCurrentWeeklyPeriodId());
	if (oldIds.getCurrentMonthlyPeriodId() != newIds.getCurrentMonthlyPeriodId())
		onStatsRollover(Stats::STAT_PERIOD_MONTHLY, oldIds.getCurrentMonthlyPeriodId(), newIds.getCurrentMonthlyPeriodId());
}

/*************************************************************************************************/
/*!
    \brief  onStatsRollover

    receives notification from the stats component that a stat period has rolled over

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onStatsRollover(Stats::StatPeriodType periodType, int32_t oldPeriodId, int32_t newPeriodId)
{
    TRACE_LOG("[FifaCupsSlaveImpl].onStatsRollover: received stats rollover event. period = "<<Stats::StatPeriodTypeToString(periodType)<<", oldPeriodId = "<<oldPeriodId<<", newPeriodId = "<<newPeriodId<<" ");

    // INFO_LOG(rm the master that a stats rollover has occurred
    EndOfSeasonRolloverData rolloverData;

    rolloverData.setStatPeriodType(periodType);
    rolloverData.setOldPeriodId(oldPeriodId);
    rolloverData.setNewPeriodId(newPeriodId);

    BlazeRpcError error = getMaster()->startEndOfSeasonProcessing(rolloverData);
    if (Blaze::ERR_OK != error)
    {
        ERR_LOG("[FifaCupsSlaveImpl].onStatsRollover: failed to notify master to start end of season processing. Error = "<<ErrorHelp::getErrorName(error)<<" "); 
    }
}

/*************************************************************************************************/
/*!
    \brief  onUserSessionExistence

    called when a user logs into the slave. checks if we should register the user in
    seasonal play and does so if needed.

*/
/*************************************************************************************************/
void FifaCupsSlaveImpl::onUserSessionExistence(const UserSession& userSession)
{
	//TRACE_LOG("[FifaCupsSlaveImpl].onUserSessionLogin. mAutoUserRegistrationEnabled "<<mAutoUserRegistrationEnabled<<"  user.isLocal() "<<user.isLocal()<<" CLIENT_TYPE_CONSOLE_USER == user.getClientType() "<<CLIENT_TYPE_GAMEPLAY_USER == user.getClientType()<<" " );

    // only care about this login event if the user is logged into this slave from a console
    if (true == mAutoUserRegistrationEnabled && 
        true == userSession.isLocal() && 
        CLIENT_TYPE_GAMEPLAY_USER == userSession.getClientType())
    {
        gSelector->scheduleFiberCall<FifaCupsSlaveImpl, BlazeId>(
            this, 
            &FifaCupsSlaveImpl::verifyUserRegistration,
            userSession.getUserId(),
			"FifaCupsSlaveImpl::onUserSessionLogin");
    }
}

/*************************************************************************************************/
/*!
    \brief  getSeasonConfiguration

    Gets the season configuration for the season matching the season id

*/
/*************************************************************************************************/
const SeasonConfigurationServer* FifaCupsSlaveImpl::getSeasonConfiguration(SeasonId seasonId)
{
    const SeasonConfigurationServer *pSeason = NULL;

    FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap *pConfigMap = getReplicatedSeasonConfigurationMap();
    if (pConfigMap->size() > 0)
    {
        FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr = pConfigMap->begin();
        FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr_end = pConfigMap->end();

        for( ; itr != itr_end; ++itr)
        {
            const SeasonConfigurationServer *pTempSeason = *itr;

            if (seasonId == pTempSeason->getSeasonId())
            {
                pSeason = pTempSeason;
                break;
            }
        }
    }

    return pSeason;
}


/*************************************************************************************************/
/*!
    \brief  getSeasonConfigurationForLeague

    Gets the season configuration for the season matching the league id and member type    

*/
/*************************************************************************************************/
const SeasonConfigurationServer* FifaCupsSlaveImpl::getSeasonConfigurationForLeague(LeagueId leagueId, MemberType memberType = FIFACUPS_MEMBERTYPE_CLUB)
{
    const SeasonConfigurationServer *pSeasonConfig = NULL;

    FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap *pConfigMap = getReplicatedSeasonConfigurationMap();
    if (pConfigMap->size() > 0)
    {
        FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr = pConfigMap->begin();
        FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr_end = pConfigMap->end();

        for( ; itr != itr_end; ++itr)
        {
            const SeasonConfigurationServer *pTempSeason = *itr;

            if (memberType == pTempSeason->getMemberType() && leagueId == pTempSeason->getLeagueId())
            {
                pSeasonConfig = pTempSeason;
                break;
            }
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
void FifaCupsSlaveImpl::verifyUserRegistration(BlazeId userId)
{
	TRACE_LOG("[FifaCupsSlaveImpl].verifyUserRegistration. User logged in. User id = "<<userId);

    BlazeRpcError error = Blaze::ERR_OK;

    FifaCupsDb dbHelper = FifaCupsDb(this);

    DbConnPtr dbConn = gDbScheduler->getConnPtr(getDbId());
    error = dbHelper.setDbConnection(dbConn);

	if (error != Blaze::ERR_OK)
	{
		ERR_LOG("[FifaCupsSlaveImpl].onUserSessionLogin. gDbScheduler was not able to get conn ptr");
		return;
	}

    // is the user registered for user seasonal play?
    SeasonId seasonId;
    error = dbHelper.fetchSeasonId(userId, FIFACUPS_MEMBERTYPE_USER, seasonId);

    if (Blaze::FIFACUPS_ERR_NOT_REGISTERED == error)
    {
        // user is not registered in seasonal play

        // determine the season id that corresponds to versus and register the user in it
        FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap *pConfigMap = getReplicatedSeasonConfigurationMap();
        if (pConfigMap->size() > 0)
        {
            bool8_t bFoundUserSeason = false;
            LeagueId versusLeagueId = 0;
            FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr = pConfigMap->begin();
            FifaCupsSlaveStub::ReplicatedSeasonConfigurationMap::const_iterator itr_end = pConfigMap->end();

			int startDivision = 0;

            for( ; itr != itr_end; ++itr)
            {
                const SeasonConfigurationServer *pSeason = *itr;
                
                if (FIFACUPS_MEMBERTYPE_USER == pSeason->getMemberType())
                {
                    seasonId = pSeason->getSeasonId();
                    versusLeagueId = pSeason->getLeagueId();
                    bFoundUserSeason = true;
					startDivision = pSeason->getStartDivision();
                    break;
                }
            }

            // only register the user if there is a user season found
            if (bFoundUserSeason)
            {
                TRACE_LOG("[FifaCupsSlaveImpl].verifyUserRegistration. Registering User in versus season. User id = "<<userId<<", season id = "<<seasonId<<" startDivision "<<startDivision<<" ");
                dbHelper.clearBlazeRpcError();
                error = dbHelper.insertRegistration(userId, FIFACUPS_MEMBERTYPE_USER, seasonId, versusLeagueId, startDivision);   
            }
        }
        else
        {
            error = Blaze::FIFACUPS_ERR_CONFIGURATION_ERROR;
        }

        if (Blaze::ERR_OK != error)
        {
            ERR_LOG("[FifaCupsSlaveImpl].onUserSessionLogin. Error in registering user in seasonal play. Error = "<<error<<" ");
        }
    }
}


/*************************************************************************************************/
/*!
    \brief  getSeasonPeriodType

    gets the stats period type for the specified season

*/
/*************************************************************************************************/
Stats::StatPeriodType FifaCupsSlaveImpl::getSeasonPeriodType(SeasonId seasonId)
{
    Stats::StatPeriodType periodType = Stats::STAT_PERIOD_ALL_TIME; 

    const SeasonConfigurationServer *pSeasonConfig = getSeasonConfiguration(seasonId);
    if (NULL != pSeasonConfig)
    {
        periodType = pSeasonConfig->getStatPeriodtype();
    }
    else
    {
        ERR_LOG("[FifaCupsSlaveImpl].getSeasonPeriodType(): unable to retrieve configuration for season id "<<seasonId<<" ");
    }
    
    return periodType;   
}

} // FifaCups
} // Blaze
