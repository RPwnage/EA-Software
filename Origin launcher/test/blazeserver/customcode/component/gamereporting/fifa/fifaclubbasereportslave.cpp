/*************************************************************************************************/
/*!
    \file   fifaclubbasereportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifaclubbasereportslave.h"
#include "fifa/tdf/fifaclubreport.h"

#include "useractivitytracker/stats_updater.h"

#include "gamereporting/util/updateclubsutil.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FifaClubBaseReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaClubBaseReportSlave::FifaClubBaseReportSlave(GameReportingSlaveImpl& component) :
GameClubReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief FifaClubBaseReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaClubBaseReportSlave::~FifaClubBaseReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaClubBaseReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaClubBaseReportSlave::create(GameReportingSlaveImpl& component)
{
	// FifaClubBaseReportSlave should not be instantiated
	return NULL;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError FifaClubBaseReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
	mProcessedReport = &processedReport;

	GameReport& report = processedReport.getGameReport();
	const GameType& gameType = processedReport.getGameType();

	BlazeRpcError processErr = Blaze::ERR_OK;

	const char8_t* CLUB_GAME_TYPE_NAME = getCustomClubGameTypeName();

	if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), CLUB_GAME_TYPE_NAME))
	{
		// create the parser
		mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
		mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

		// fills in report with values via configuration
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
		{
			WARN_LOG("[GameClubReportSlave::" << this << "].process(): Error parsing values");

			processErr = mReportParser->getErrorCode();
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

		// common processing
		processCommon();

		// initialize club utility with the Club IDs in the game report
		if (false == initClubUtil())
		{
			WARN_LOG("[GameClubReportSlave::" << this << "].process(): Error initializing club util");
			delete mReportParser;
			return Blaze::ERR_SYSTEM;  // EARLY RETURN
		}

		// initialize game parameters
		initGameParams();

		UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
		statsUpdater.updateGamesPlayed(playerIds);
		statsUpdater.selectStats();

		if (true == skipProcess())
		{
			if (!mIsRankedGame && !mIsOfflineReport && mGameTime >= mReportConfig->getEndPhaseMaxTime()[0])
			{
				TRACE_LOG("[GameClubReportSlave:" << this << "].process() - Unranked match that is past stage 1 disconnect count the match for UserActivityTracker");
				Blaze::BlazeRpcError error = Blaze::ERR_OK;
				bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
				error = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);
				if (Blaze::ERR_OK == error)
				{
					error = mUpdateStatsHelper.fetchStats();
					if (Blaze::ERR_OK == error)
					{
						statsUpdater.transformStats();
						error = mUpdateStatsHelper.commitStats();
						if (Blaze::ERR_OK != error)
						{
							WARN_LOG("[GameClubReportSlave::" << this << "].process(): commitStats for User Activity Tracker (unranked) failed!" << error << " (error)");
						}
					}
				}
			}

			delete mReportParser;
			return processErr;  // EARLY RETURN
		}

		//populate entity filter list
		populateEntityFilterList();

		//setup the game status
		initGameStatus();

		// determine win/loss/tie
		determineGameResult(); 

		// update player keyscopes
		updatePlayerKeyscopes();

		// TODO: add custom hook to guard seasonal play logic
		// find the season state for each club in the report
		//determineClubSeasonState();

		// update club keyscopes
		updateClubKeyscopes();

		// parse the keyscopes from the configuration
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
		{
			WARN_LOG("[GameClubReportSlave::" << this << "].process(): Error parsing keyscopes");

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

		// update club free agent player stats
		updateClubFreeAgentPlayerStats();

		// update club stats
		if (false == updateClubStats())
		{
			// do not update the club stats if the game result is not committed to clubs component
			WARN_LOG("[GameClubReportSlave::" << this << "].process() Failed to commit stats to club component");
			delete mReportParser;
			return Blaze::ERR_SYSTEM; // EARLY RETURN
		}

		updateCustomGameStats();

		selectCustomStats();

		// extract and set stats
		if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
		{
			WARN_LOG("[GameClubReportSlave::" << this << "].process(): Error parsing stats");

			processErr = mReportParser->getErrorCode();
			delete mReportParser;
			return processErr; // EARLY RETURN
		}

		bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
		processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

		if (Blaze::ERR_OK == processErr)
		{
			if (Blaze::ERR_OK == transformStats())  // Club Rank
			{
				//transformSeasonalClubStats();       // Club Seasonal Stats
				statsUpdater.transformStats();
			}

			TRACE_LOG("[GameClubReportSlave:" << this << "].process() - commit stats");
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

		if (false == processPostStatsCommit())
		{
			processErr = ERR_SYSTEM;
		}

		// send end game mail
		sendEndGameMail(playerIds);

		// Customize GameReports for PIN Event Reporting
		CustomizeGameReportForEventReporting(processedReport, playerIds);
	}

	return processErr;
}

void FifaClubBaseReportSlave::determineGameResult()
{
	updateUnadjustedScore();

	adjustRedCardedPlayers();
    determineWinClub();
	adjustPlayerResult();
}

Fifa::CommonGameReport* FifaClubBaseReportSlave::getGameReport()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport.getGameReport();
	OSDKClubGameReportBase::OSDKClubGameReport* osdkClubGameReport = static_cast<OSDKClubGameReportBase::OSDKClubGameReport*>(osdkGameReport.getCustomGameReport());
	FifaClubReportBase::FifaClubsGameReport* fifaClubReport = static_cast<FifaClubReportBase::FifaClubsGameReport*>(osdkClubGameReport->getCustomClubGameReport());

	return &fifaClubReport->getCommonGameReport();
}

OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap* FifaClubBaseReportSlave::getEntityReportMap()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport* clubReports = static_cast<OSDKClubGameReportBase::OSDKClubReport*>(osdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& osdkClubReportsMap = clubReports->getClubReports();
	
	return &osdkClubReportsMap;
}

Fifa::CommonPlayerReport* FifaClubBaseReportSlave::getCommonPlayerReport(OSDKClubGameReportBase::OSDKClubClubReport& entityReport)
{
	FifaClubReportBase::FifaClubsClubReport& fifaClubsClubReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*entityReport.getCustomClubClubReport());
	return &fifaClubsClubReport.getCommonClubReport();
}

bool FifaClubBaseReportSlave::didEntityFinish(int64_t entityId, OSDKClubGameReportBase::OSDKClubClubReport& entityReport)
{
	return (entityReport.getClubDisc() == 0);
}

OSDKClubGameReportBase::OSDKClubClubReport* FifaClubBaseReportSlave::findEntityReport(int64_t entityId)
{
	OSDKClubGameReportBase::OSDKClubClubReport* oskClubClubReport = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport* clubReports = static_cast<OSDKClubGameReportBase::OSDKClubReport*>(osdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& osdkClubReportsMap = clubReports->getClubReports();

	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::iterator clubIter;
	clubIter = osdkClubReportsMap.find(entityId);
	if (clubIter != osdkClubReportsMap.end())
	{
		oskClubClubReport = clubIter->second;
	}

	return oskClubClubReport;
}

int64_t FifaClubBaseReportSlave::getWinningEntity()
{
	return mWinClubId;
}

int64_t FifaClubBaseReportSlave::getLosingEntity()
{
	return mLossClubId;
}

bool FifaClubBaseReportSlave::isTieGame()
{
	return mTieGame;
}

bool FifaClubBaseReportSlave::isEntityWinner(int64_t entityId)
{
	return (mWinClubId == static_cast<uint64_t>(entityId));
}

bool FifaClubBaseReportSlave::isEntityLoser(int64_t entityId)
{
	return (mLossClubId == static_cast<uint64_t>(entityId));
}

int32_t FifaClubBaseReportSlave::getHighestEntityScore()
{
	return mHighestClubScore;
}

int32_t FifaClubBaseReportSlave::getLowestEntityScore()
{
	return mLowestClubScore;
}

void FifaClubBaseReportSlave::setCustomEntityResult(OSDKClubGameReportBase::OSDKClubClubReport& entityReport, StatGameResultE gameResult)
{
	if (gameResult == GAME_RESULT_CONCEDE_QUIT)
	{
		entityReport.setClubDisc(1);
	}
}

void FifaClubBaseReportSlave::setCustomEntityScore(OSDKClubGameReportBase::OSDKClubClubReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst)
{
	FifaClubReportBase::FifaClubsClubReport& fifaClubsClubReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*entityReport.getCustomClubClubReport());
	fifaClubsClubReport.setGoals(goalsFor);
	fifaClubsClubReport.setGoalsAgainst(goalsAgainst);
}

void FifaClubBaseReportSlave::populateEntityFilterList()
{
	mEntityFilterList.clear();

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport* clubReports = static_cast<OSDKClubGameReportBase::OSDKClubReport*>(osdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& clubReportsMap = clubReports->getClubReports();

	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubsIter, clubsEnd;
	clubsIter = clubReportsMap.begin();
	clubsEnd = clubReportsMap.end();

	UpdateClubsUtil updateClubsUtil;
	for (; clubsIter != clubsEnd; ++clubsIter)
	{
		if (updateClubsUtil.validateClubId(clubsIter->first) == false)
		{
			mEntityFilterList.push_back(clubsIter->first);
		}
	}
}

void FifaClubBaseReportSlave::sendEndGameMail(GameManager::PlayerIdList& playerIds) {};// feature silently removed from fifa14  but preserved it for future in the base class

void FifaClubBaseReportSlave::CustomizeGameReportForEventReporting(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
	// loop over all participants
	for (auto playerId : playerIds)
	{
		// Find the players specific PIN game report
		auto iter = processedReport.getPINGameReportMap().find(playerId);
		if (iter != processedReport.getPINGameReportMap().end())
		{
			OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*(iter->second.getReport()));
			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

			auto playReportiter = osdkPlayerReportsMap.begin();
			while (playReportiter != osdkPlayerReportsMap.end())
			{
				OSDKGameReportBase::OSDKPlayerReport& playerReport = *playReportiter->second;

				Collections::AttributeMap& map = playerReport.getPrivatePlayerReport().getPrivateAttributeMap();
				map.clearIsSet();

				playReportiter++;
			}
		}
	}
}


}   // namespace GameReporting

}   // namespace Blaze

