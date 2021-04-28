/*************************************************************************************************/
/*!
    \file   ssfbasereportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "ssfbasereportslave.h"
#include "fifa/tdf/ssfseasonsreport.h"

#include "useractivitytracker/stats_updater.h"


namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief SSFBaseReportSlave
    Constructor
*/
/*************************************************************************************************/
SSFBaseReportSlave::SSFBaseReportSlave(GameReportingSlaveImpl& component) :
SSFReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief SSFBaseReportSlave
    Destructor
*/
/*************************************************************************************************/
SSFBaseReportSlave::~SSFBaseReportSlave()
{
}

/*************************************************************************************************/
/*! \brief SSFBaseReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* SSFBaseReportSlave::create(GameReportingSlaveImpl& component)
{
	// SSFBaseReportSlave should not be instantiated
	return NULL;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError SSFBaseReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    TRACE_LOG("[SSFBaseReportSlave::" << this << "].process()");
    mProcessedReport = &processedReport;

	GameReport& report = processedReport.getGameReport();
	const GameType& gameType = processedReport.getGameType();

	BlazeRpcError processErr = Blaze::ERR_OK;

	const char8_t* GAME_TYPE_NAME = getCustomSquadGameTypeName();

	if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), GAME_TYPE_NAME))
	{
		// create the parser
		mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
		mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

		// fills in report with values via configuration
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
		{
			WARN_LOG("[SSFBaseReportSlave::" << this << "].process(): Error parsing values");

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

            TRACE_LOG("[SSFBaseReportSlave::" << this << "].process(): skipping the process, but send back a notification with a proper SSF end result");
            SSFReportSlave::determineGameResult();
            updatePlayerStats();
            updateNotificationReport();

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
			WARN_LOG("[SSFBaseReportSlave::" << this << "].process(): Error parsing keyscopes");

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

		updateGameStats();

		//update DNF
		updateDNF();

		// update squad stats
		if (false == updateSquadStats())
		{
			// do not update the club stats if the game result is not committed
			WARN_LOG("[SSFBaseReportSlave::" << this << "].process() Failed to commit stats");
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
			WARN_LOG("[SSFBaseReportSlave::" << this << "].process(): Error parsing stats");

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

			TRACE_LOG("[SSFBaseReportSlave:" << this << "].process() - commit stats");
			processErr = mUpdateStatsHelper.commitStats();
		}

		if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
		{
			// extract game history attributes from report.
			GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
			mReportParser->setGameHistoryReport(&historyReport);
			mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
		}

        delete mReportParser;

		// post game reporting processing()
		processErr = (true == processUpdatedStats()) ? ERR_OK : ERR_SYSTEM;

		// send end game mail
		sendEndGameMail(playerIds);
	}

	return processErr;
}

void SSFBaseReportSlave::determineGameResult()
{
	updateUnadjustedScore();

	adjustRedCardedPlayers();
    SSFReportSlave::determineGameResult();
	adjustPlayerResult();
	SSFReportSlave::updateSquadResults();
}

SSF::CommonGameReport* SSFBaseReportSlave::getGameReport()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport.getGameReport();
	SSFSeasonsReportBase::SSFSeasonsGameReport* ssfSeasonsGameReport = static_cast<SSFSeasonsReportBase::SSFSeasonsGameReport*>(osdkGameReport.getCustomGameReport());

	return &ssfSeasonsGameReport->getCommonGameReport();
}

SSFSeasonsReportBase::SSFTeamReportMap* SSFBaseReportSlave::getEntityReportMap()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();
	
	return &ssfTeamReportMap;
}

SSF::CommonStatsReport* SSFBaseReportSlave::getCommonPlayerReport(SSFSeasonsReportBase::SSFTeamReport& entityReport)
{
	return &entityReport.getCommonTeamReport();
}

bool SSFBaseReportSlave::didEntityFinish(int64_t entityId, SSFSeasonsReportBase::SSFTeamReport& entityReport)
{
	return (entityReport.getSquadDisc() == 0);
}

SSFSeasonsReportBase::SSFTeamReport* SSFBaseReportSlave::findEntityReport(int64_t entityId)
{
	SSFSeasonsReportBase::SSFTeamReport* ssfTeamReport = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

	SSFSeasonsReportBase::SSFTeamReportMap::iterator teamIter;
	teamIter = ssfTeamReportMap.find(entityId);
	if (teamIter != ssfTeamReportMap.end())
	{
		ssfTeamReport = teamIter->second;
	}

	return ssfTeamReport;
}

int64_t SSFBaseReportSlave::getWinningEntity()
{
	return mWinEntityId;
}

int64_t SSFBaseReportSlave::getLosingEntity()
{
	return mLossEntityId;
}

bool SSFBaseReportSlave::isTieGame()
{
	return mTieGame;
}

bool SSFBaseReportSlave::isEntityWinner(int64_t entityId)
{
	return (mWinEntityId == entityId);
}

bool SSFBaseReportSlave::isEntityLoser(int64_t entityId)
{
	return (mLossEntityId == entityId);
}

int32_t SSFBaseReportSlave::getHighestEntityScore()
{
	return mHighestScore;
}

int32_t SSFBaseReportSlave::getLowestEntityScore()
{
	return mLowestScore;
}

void SSFBaseReportSlave::setCustomEntityResult(SSFSeasonsReportBase::SSFTeamReport& entityReport, StatGameResultE gameResult)
{
	if (gameResult == GAME_RESULT_CONCEDE_QUIT)
	{
		entityReport.setSquadDisc(1);
	}
}

void SSFBaseReportSlave::setCustomEntityScore(SSFSeasonsReportBase::SSFTeamReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst)
{
	SSF::CommonStatsReport& ssfCommonTeamReport = entityReport.getCommonTeamReport();
	ssfCommonTeamReport.setGoals(goalsFor);
	ssfCommonTeamReport.setGoalsConceded(goalsAgainst);
}

void SSFBaseReportSlave::updateDNF()
{
	int64_t discEntityId = Blaze::INVALID_BLAZE_ID;
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());

	//first check if the game was DNF
	if (mGameDNF)
	{
		//find the entity id that DNF'd
		SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
		SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

		//check the team entity ids in the map
		SSFSeasonsReportBase::SSFTeamReportMap::const_iterator iter, end;
		iter = ssfTeamReportMap.begin();
		end = ssfTeamReportMap.end();
		for (; iter != end; iter++)
		{
			const SSFSeasonsReportBase::SSFTeamReport* ssfTeamReport = iter->second;
			if (ssfTeamReport->getSquadDisc() == 1)
			{
				discEntityId = iter->first;
				break;
			}
		}
	}

	//check each player that belongs to DNF id
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& playerReportsMap = static_cast<OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap&>(osdkReport.getPlayerReports());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator iterPlayer, endPlayer;
	iterPlayer = playerReportsMap.begin();
	endPlayer = playerReportsMap.end();

	for (; iterPlayer != endPlayer; iterPlayer++)
	{
		OSDKGameReportBase::OSDKPlayerReport& playerReport = static_cast<OSDKGameReportBase::OSDKPlayerReport&>(*iterPlayer->second);

		Stats::ScopeNameValueMap playerKeyScopeMap;

		playerKeyScopeMap[SCOPENAME_ACCOUNTCOUNTRY]= playerReport.getAccountCountry();
		playerKeyScopeMap[SCOPENAME_CONTROLS]= DEFAULT_CONTROLS;

		mBuilder.startStatRow("NormalGameStats", iterPlayer->first, &playerKeyScopeMap);
		mBuilder.incrementStat(STATNAME_TOTAL_GAMES_PLAYED, 1);
		if (iterPlayer->first == discEntityId)
		{
			mBuilder.incrementStat(STATNAME_GAMES_NOT_FINISHED,	1);
		}
		mBuilder.completeStatRow();

		TRACE_LOG("[SSFReportSlave:" << this << "].updateDNF() for playerId: " << iterPlayer->first << " with DNF status");
	}
}

}   // namespace GameReporting

}   // namespace Blaze

