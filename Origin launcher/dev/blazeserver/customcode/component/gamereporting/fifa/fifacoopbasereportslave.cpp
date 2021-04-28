/*************************************************************************************************/
/*!
    \file   fifacoopbasereportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifacoopbasereportslave.h"
#include "fifa/tdf/fifacoopreport.h"

#include "coopseason/rpc/coopseasonslave.h"
#include "coopseason/tdf/coopseasontypes.h"

#include "fifacups/tdf/fifacupstypes.h"
#include "fifacups/fifacupsslaveimpl.h"

#include "useractivitytracker/stats_updater.h"


namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FifaCoopBaseReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaCoopBaseReportSlave::FifaCoopBaseReportSlave(GameReportingSlaveImpl& component) :
FifaCoopReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief FifaCoopBaseReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaCoopBaseReportSlave::~FifaCoopBaseReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaCoopBaseReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaCoopBaseReportSlave::create(GameReportingSlaveImpl& component)
{
	// FifaCoopBaseReportSlave should not be instantiated
	return NULL;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError FifaCoopBaseReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
	mProcessedReport = &processedReport;

	GameReport& report = processedReport.getGameReport();
	const GameType& gameType = processedReport.getGameType();

	BlazeRpcError processErr = Blaze::ERR_OK;

	const char8_t* COOP_GAME_TYPE_NAME = getCustomSquadGameTypeName();

	if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), COOP_GAME_TYPE_NAME))
	{
		//setup coopids
		if(false == setupCoopIds())
		{
			TRACE_LOG("[FifaCoopBaseReportSlave:" << this << "].process() - setupCoopIds() failed");
			delete mReportParser;
			return processErr;  // EARLY RETURN
		}

		//verify if any there are any solo coop teams
		checkForSoloCoop();

		// create the parser
		mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
		mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

		// fills in report with values via configuration
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
		{
			WARN_LOG("[FifaCoopBaseReportSlave::" << this << "].process(): Error parsing values");

			processErr = mReportParser->getErrorCode();
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

		// common processing
		processCommon();

		// initialize game parameters
		initGameParams();

		if (true == skipProcess())
		{
			delete mReportParser;
			return processErr;  // EARLY RETURN
		}

		//setup the game status
		initGameStatus();

		// determine win/loss/tie
		determineGameResult(); 

		// update player keyscopes
		updatePlayerKeyscopes();

		// update squad keyscopes
		updateSquadKeyscopes();

		// parse the keyscopes from the configuration
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
		{
			WARN_LOG("[FifaCoopBaseReportSlave::" << this << "].process(): Error parsing keyscopes");

			processErr = mReportParser->getErrorCode();
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

		// determine conclusion type
		if(true == skipProcessConclusionType())
		{
			updateSkipProcessConclusionTypeStats();
			delete mReportParser;
			return processErr;  // EARLY RETURN
		}

		// update player stats
		updatePlayerStats();

		//update coop DNF
		updateCoopDNF();

		// update squad stats
		if (false == updateSquadStats())
		{
			// do not update the club stats if the game result is not committed
			WARN_LOG("[FifaCoopBaseReportSlave::" << this << "].process() Failed to commit stats");
			delete mReportParser;
			return Blaze::ERR_SYSTEM; // EARLY RETURN
		}

		selectCustomStats();

		UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
		statsUpdater.updateGamesPlayed(playerIds);
		statsUpdater.selectStats();

		// extract and set stats
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
		{
			WARN_LOG("[FifaCoopBaseReportSlave::" << this << "].process(): Error parsing stats");

			processErr = mReportParser->getErrorCode();
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

		bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
		processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

		if (Blaze::ERR_OK == processErr)
		{
			if (Blaze::ERR_OK == transformStats()) 
			{
				statsUpdater.transformStats();
			}

			TRACE_LOG("[FifaCoopBaseReportSlave:" << this << "].process() - commit stats");
			processErr = mUpdateStatsHelper.commitStats();
		}

		if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
		{
			// extract game history attributes from report.
			GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
			mReportParser->setGameHistoryReport(&historyReport);
			mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
			delete mReportParser;
		}

		// post game reporting processing()
		processErr = (true == processUpdatedStats()) ? ERR_OK : ERR_SYSTEM;

		// send end game mail
		sendEndGameMail(playerIds);
	}

	return processErr;
}

void FifaCoopBaseReportSlave::determineGameResult()
{
	updateUnadjustedScore();

	adjustRedCardedPlayers();
    determineWinSquad();
	adjustPlayerResult();
}

Fifa::CommonGameReport* FifaCoopBaseReportSlave::getGameReport()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport.getGameReport();
	FifaCoopReportBase::FifaCoopGameReportBase* fifaCoopGameReportBase = static_cast<FifaCoopReportBase::FifaCoopGameReportBase*>(osdkGameReport.getCustomGameReport());
	FifaCoopSeasonsReport::FifaCoopSeasonsGameReport* fifaCoopSeasonsGameReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsGameReport*>(fifaCoopGameReportBase->getCustomCoopGameReport());

	return &fifaCoopSeasonsGameReport->getCommonGameReport();
}

FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap* FifaCoopBaseReportSlave::getEntityReportMap()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport* squadReports = static_cast<FifaCoopReportBase::FifaCoopSquadReport*>(osdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaSquadReportsMap = squadReports->getSquadReports();
	
	return &fifaSquadReportsMap;
}

Fifa::CommonPlayerReport* FifaCoopBaseReportSlave::getCommonPlayerReport(FifaCoopReportBase::FifaCoopSquadDetailsReportBase& entityReport)
{
	FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*entityReport.getCustomSquadDetailsReport());
	return &fifaCoopSeasonsSquadDetailsReport.getCommonClubReport();
}

bool FifaCoopBaseReportSlave::didEntityFinish(int64_t entityId, FifaCoopReportBase::FifaCoopSquadDetailsReportBase& entityReport)
{
	return (entityReport.getSquadDisc() == 0);
}

FifaCoopReportBase::FifaCoopSquadDetailsReportBase* FifaCoopBaseReportSlave::findEntityReport(int64_t entityId)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport* fifaCoopSquadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport*>(osdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaSquadReportsMap = fifaCoopSquadReport->getSquadReports();

	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::iterator squadIter;
	squadIter = fifaSquadReportsMap.find(entityId);
	if (squadIter != fifaSquadReportsMap.end())
	{
		fifaCoopSquadDetailsReportBase = squadIter->second;
	}

	return fifaCoopSquadDetailsReportBase;
}

int64_t FifaCoopBaseReportSlave::getWinningEntity()
{
	return mWinCoopId;
}

int64_t FifaCoopBaseReportSlave::getLosingEntity()
{
	return mLossCoopId;
}

bool FifaCoopBaseReportSlave::isTieGame()
{
	return mTieGame;
}

bool FifaCoopBaseReportSlave::isEntityWinner(int64_t entityId)
{
	return (mWinCoopId == static_cast<uint64_t>(entityId));
}

bool FifaCoopBaseReportSlave::isEntityLoser(int64_t entityId)
{
	return (mLossCoopId == static_cast<uint64_t>(entityId));
}

int32_t FifaCoopBaseReportSlave::getHighestEntityScore()
{
	return mHighestCoopScore;
}

int32_t FifaCoopBaseReportSlave::getLowestEntityScore()
{
	return mLowestCoopScore;
}

void FifaCoopBaseReportSlave::setCustomEntityResult(FifaCoopReportBase::FifaCoopSquadDetailsReportBase& entityReport, StatGameResultE gameResult)
{
	if (gameResult == GAME_RESULT_CONCEDE_QUIT)
	{
		entityReport.setSquadDisc(1);
	}
}

void FifaCoopBaseReportSlave::setCustomEntityScore(FifaCoopReportBase::FifaCoopSquadDetailsReportBase& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst)
{
	FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*entityReport.getCustomSquadDetailsReport());
	fifaCoopSeasonsSquadDetailsReport.setGoals(goalsFor);
	fifaCoopSeasonsSquadDetailsReport.setGoalsAgainst(goalsAgainst);
}

bool FifaCoopBaseReportSlave::setupCoopIds()
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaCoopSquadReportMap = squadReport.getSquadReports();

	if (2 != fifaCoopSquadReportMap.size())
	{
		WARN_LOG("[FifaCoopReportSlave:" << this << "].validate() : There must have been exactly two squads in this game!");
		return false; // EARLY RETURN
	}

	CoopSeason::CoopSeasonSlave *fifaCoopSeasonComponent =
		static_cast<CoopSeason::CoopSeasonSlave*>(gController->getComponent(CoopSeason::CoopSeasonSlave::COMPONENT_ID,false));
	if (fifaCoopSeasonComponent == NULL)
	{
		WARN_LOG("[FifaCoopReportSlave:"<<this<<"].validate() - unable to request CoopSeason component");
		return false; // EARLY RETURN
	}

	//check the coop ids in the map
	bool result = true;

	bool shouldDeleteHome = false;
	bool shouldDeleteAway = false;
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* homeCoopSquadReport = NULL;
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* awayCoopSquadReport = NULL;
	CoopSeason::CoopId homeCoopId = 0;
	CoopSeason::CoopId awayCoopId = 0;

	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::iterator iter, end;
	iter = fifaCoopSquadReportMap.begin();
	end = fifaCoopSquadReportMap.end();
	for (; iter != end; iter++)
	{
		CoopSeason::CoopId coopId = iter->first;
		TRACE_LOG("[FifaCoopReportSlave:" << this << "].validate() - found coopid " << coopId << " ");
		if (coopId < CoopSeason::RESERVED_COOP_ID_ALL)
		{
			//send RPC request to coop seasons component to get the coop id.
			BlazeRpcError blazeError = ERR_OK;

			CoopSeason::SetCoopIdDataRequest request;
			CoopSeason::SetCoopIdDataResponse response;

			//coop id invalid get the two blaze ids of the users to assign the coopid
			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& playerReportsMap = static_cast<OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap&>(OsdkReport.getPlayerReports());
			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator iterPlayer, endPlayer;
			iterPlayer = playerReportsMap.begin();
			endPlayer = playerReportsMap.end();

			FifaCoopReportBase::FifaCoopPlayerReportBase* memberOne = NULL;
			FifaCoopReportBase::FifaCoopPlayerReportBase* memberTwo = NULL;

			int i = 0;
			bool noRebroadcaster = false;
			for (; iterPlayer != endPlayer; iterPlayer++)
			{
				OSDKGameReportBase::OSDKPlayerReport& playerReport = static_cast<OSDKGameReportBase::OSDKPlayerReport&>(*iterPlayer->second);
				FifaCoopReportBase::FifaCoopPlayerReportBase* playerReportBase = static_cast<FifaCoopReportBase::FifaCoopPlayerReportBase*>(playerReport.getCustomPlayerReport());
				if (playerReportBase->getCoopId() == coopId)
				{
					if (i % 2 == 0)
					{
						request.setMemberOneBlazeId(iterPlayer->first);
						memberOne = playerReportBase;
					}
					else
					{
						request.setMemberTwoBlazeId(iterPlayer->first);
						memberTwo = playerReportBase;
					}
					TRACE_LOG("[FifaCoopReportSlave:" << this << "].validate() - register member " << i << " for blaze id " << iterPlayer->first << " ");

					OSDKGameReportBase::IntegerAttributeMap& privateIntMap = playerReport.getPrivatePlayerReport().getPrivateIntAttributeMap();
					noRebroadcaster = (privateIntMap["NO_REBROADCASTER"] == 1);
					if (noRebroadcaster)
					{
						TRACE_LOG("[FifaCoopReportSlave:" << this << "].validate() - not using rebroadcaster override member 2 with member 1");

						request.setMemberTwoBlazeId((iterPlayer->first));
						memberTwo = playerReportBase;
					}

					i++;
				}
			}

			if (i == 2 || noRebroadcaster)
			{
				UserSession::pushSuperUserPrivilege();
				blazeError = fifaCoopSeasonComponent->setCoopIdData(request, response);
				UserSession::popSuperUserPrivilege();

				TRACE_LOG("[FifaCoopReportSlave:" << this << "].validate() - setCoopIdData err:" << blazeError << " response:" << response.getResultSuccess() << " ");

				if (response.getResultSuccess())
				{
					TRACE_LOG("[FifaCoopReportSlave:" << this << "].validate() - successfully registered member 1 and member 2 with coopid:" << response.getCoopId() << " ");

					if (coopId == CoopSeason::COOP_ID_HOME)
					{
						shouldDeleteHome = true;
						homeCoopId = response.getCoopId();
						homeCoopSquadReport = reinterpret_cast<FifaCoopReportBase::FifaCoopSquadDetailsReportBase*>(iter->second.get());
					}
					else if (coopId == CoopSeason::COOP_ID_AWAY)
					{
						shouldDeleteAway = true;
						awayCoopId = response.getCoopId();
						awayCoopSquadReport = reinterpret_cast<FifaCoopReportBase::FifaCoopSquadDetailsReportBase*>(iter->second.get());
					}

					if (memberOne != NULL)
					{	
						memberOne->setCoopId(response.getCoopId());
					}
					
					if (memberTwo != NULL)
					{					
						memberTwo->setCoopId(response.getCoopId());
					}

					//register the coop id for cup play
					Blaze::FifaCups::FifaCupsSlave *fifaCupsComponent =
						static_cast<FifaCups::FifaCupsSlave*>(gController->getComponent(FifaCups::FifaCupsSlave::COMPONENT_ID,false));

					if (fifaCupsComponent != NULL)
					{
						Blaze::FifaCups::MemberId memberId = static_cast<Blaze::FifaCups::MemberId>(response.getCoopId());
						Blaze::FifaCups::LeagueId leagueId = static_cast<Blaze::FifaCups::LeagueId>(1);
						Blaze::FifaCups::MemberType memberType = Blaze::FifaCups::FIFACUPS_MEMBERTYPE_COOP;

						Blaze::FifaCups::RegisterEntityRequest req;
						req.setMemberId(memberId);
						req.setMemberType(memberType);
						req.setLeagueId(leagueId);
						BlazeRpcError error = fifaCupsComponent->registerEntity(req);

						if (error != Blaze::ERR_OK)
						{
							ERR_LOG("[FifaCoopReportSlave:" << this << "] error registering entity " << memberId << " for fifacups");
							result = false;
						}
					}
					else
					{
						ERR_LOG("[FifaCoopReportSlave:" << this << "].setupCoopIds() - fifacups component is not available");
						result = false;
					}
				}
				else
				{
					result = false;
				}
			}
			else
			{
				result = false;
			}
		}
	}

	//re-map valid coop id to the report data
	if (shouldDeleteHome)
	{
		homeCoopSquadReport->setCoopId(homeCoopId);
		fifaCoopSquadReportMap.insert(eastl::make_pair(homeCoopId, homeCoopSquadReport));
		fifaCoopSquadReportMap.erase(CoopSeason::COOP_ID_HOME);
	}
	if (shouldDeleteAway)
	{
		awayCoopSquadReport->setCoopId(awayCoopId);
		fifaCoopSquadReportMap.insert(eastl::make_pair(awayCoopId, awayCoopSquadReport));
		fifaCoopSquadReportMap.erase(CoopSeason::COOP_ID_AWAY);
	}

	return result;
}

void FifaCoopBaseReportSlave::checkForSoloCoop()
{
	//coop id invalid get the two blaze ids of the users to assign the coopid
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& playerReportsMap = static_cast<OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap&>(OsdkReport.getPlayerReports());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator iterPlayer, endPlayer;
	iterPlayer = playerReportsMap.begin();
	endPlayer = playerReportsMap.end();

	int homeCoopUserCount, awayCoopUserCount;
	homeCoopUserCount = awayCoopUserCount = 0;

	CoopSeason::CoopId homeCoopId, awayCoopId;
	homeCoopId = awayCoopId = 0;

	for (; iterPlayer != endPlayer; iterPlayer++)
	{
		BlazeId playerBlazeId = iterPlayer->first;
		OSDKGameReportBase::OSDKPlayerReport& playerReport = static_cast<OSDKGameReportBase::OSDKPlayerReport&>(*iterPlayer->second);
		FifaCoopReportBase::FifaCoopPlayerReportBase* playerReportBase = static_cast<FifaCoopReportBase::FifaCoopPlayerReportBase*>(playerReport.getCustomPlayerReport());
		if (playerReport.getHome() == 1)
		{
			homeCoopId = playerReportBase->getCoopId();
			updateUpperLowerBlazeIds(playerBlazeId, mHomeUpperBlazeId, mHomeLowerBlazeId);
			homeCoopUserCount++;
		}
		else
		{
			awayCoopId = playerReportBase->getCoopId();
			updateUpperLowerBlazeIds(playerBlazeId, mAwayUpperBlazeId, mAwayLowerBlazeId);
			awayCoopUserCount++;
		}
	}

	updateSoloFlags(homeCoopUserCount, mIsHomeCoopSolo, homeCoopId, mHomeUpperBlazeId, mHomeLowerBlazeId);
	updateSoloFlags(awayCoopUserCount, mIsAwayCoopSolo, awayCoopId, mAwayUpperBlazeId, mAwayLowerBlazeId);
}

void FifaCoopBaseReportSlave::updateCoopDNF()
{
	CoopSeason::CoopId discCoopId = CoopSeason::COOP_ID_HOME;
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());

	//first check if the game was DNF
	if (mGameDNF)
	{
		//find the coop id that DNF'd
		FifaCoopReportBase::FifaCoopSquadReport& squadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
		FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaCoopSquadReportMap = squadReport.getSquadReports();

		//check the coop ids in the map
		FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::iterator iter, end;
		iter = fifaCoopSquadReportMap.begin();
		end = fifaCoopSquadReportMap.end();
		for (; iter != end; iter++)
		{
			FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = iter->second;
			if (fifaCoopSquadDetailsReportBase->getSquadDisc() == 1)
			{
				discCoopId = iter->first;
				break;
			}
		}
	}

	//check each player that belongs to DNF coop id
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& playerReportsMap = static_cast<OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap&>(OsdkReport.getPlayerReports());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator iterPlayer, endPlayer;
	iterPlayer = playerReportsMap.begin();
	endPlayer = playerReportsMap.end();

	for (; iterPlayer != endPlayer; iterPlayer++)
	{
		OSDKGameReportBase::OSDKPlayerReport& playerReport = static_cast<OSDKGameReportBase::OSDKPlayerReport&>(*iterPlayer->second);
		FifaCoopReportBase::FifaCoopPlayerReportBase* playerReportBase = static_cast<FifaCoopReportBase::FifaCoopPlayerReportBase*>(playerReport.getCustomPlayerReport());

		Stats::ScopeNameValueMap playerKeyScopeMap;

		playerKeyScopeMap[SCOPENAME_ACCOUNTCOUNTRY]= playerReport.getAccountCountry();
		playerKeyScopeMap[SCOPENAME_CONTROLS]= DEFAULT_CONTROLS;

		mBuilder.startStatRow("NormalGameStats", iterPlayer->first, &playerKeyScopeMap);
		mBuilder.incrementStat(STATNAME_TOTAL_GAMES_PLAYED, 1);
		if (playerReportBase->getCoopId() == discCoopId)
		{
			mBuilder.incrementStat(STATNAME_GAMES_NOT_FINISHED,	1);
		}
		mBuilder.completeStatRow();

		TRACE_LOG("[FifaCoopReportSlave:" << this << "].updateCoopDNF() for playerId: " << iterPlayer->first << " with DNF status");
	}
}

void FifaCoopBaseReportSlave::updateUpperLowerBlazeIds(Blaze::BlazeId playerId, Blaze::BlazeId& upperBlazeId, Blaze::BlazeId& lowerBlazeId)
{
	if (playerId > upperBlazeId)
	{
		lowerBlazeId = upperBlazeId;
		upperBlazeId = playerId;
	}
	else
	{
		lowerBlazeId = playerId;
	}
}

void FifaCoopBaseReportSlave::updateSoloFlags(int count, bool& soloFlag, CoopSeason::CoopId coopId, Blaze::BlazeId& upperBlazeId, Blaze::BlazeId& lowerBlazeId)
{
	soloFlag = false;

	CoopSeason::CoopSeasonSlave *fifaCoopSeasonComponent =
		static_cast<CoopSeason::CoopSeasonSlave*>(gController->getComponent(CoopSeason::CoopSeasonSlave::COMPONENT_ID,false));
	if (fifaCoopSeasonComponent == NULL)
	{
		WARN_LOG("[FifaCoopReportSlave:"<<this<<"].updateSoloFlags() - unable to request CoopSeason component");
		return; // EARLY RETURN
	}

	if (count < 2)
	{
		soloFlag = true;

		//look up the coopId and get the upper and lower blaze ids;
		CoopSeason::GetCoopIdDataRequest request;
		CoopSeason::GetCoopIdDataResponse response;
		request.setCoopId(coopId);

		UserSession::pushSuperUserPrivilege();
		BlazeRpcError blazeError = fifaCoopSeasonComponent->getCoopIdData(request, response);
		UserSession::popSuperUserPrivilege();

		if (blazeError == ERR_OK)
		{
			upperBlazeId = response.getCoopPairData().getMemberOneBlazeId();
			lowerBlazeId = response.getCoopPairData().getMemberTwoBlazeId();
		}
		else
		{
			WARN_LOG("[FifaCoopReportSlave:"<<this<<"].updateSoloFlags() - unable to request CoopSeason data");
		}
	}
}

}   // namespace GameReporting

}   // namespace Blaze

