/*************************************************************************************************/
/*!
    \file   ssfsoloreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "ssfsoloreportslave.h"

#include "fifa/fifa_shield/fifashieldgamereporthelper.h"

#include "fifa/tdf/ssfseasonsreport.h"
#include "fifa/tdf/ssfcommonreport.h"

#include "useractivitytracker/stats_updater.h"


namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief SSFSoloReportSlave
    Constructor
*/
/*************************************************************************************************/
SSFSoloReportSlave::SSFSoloReportSlave(GameReportingSlaveImpl& component) :
GameSoloReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief SSFSoloReportSlave
    Destructor
*/
/*************************************************************************************************/
SSFSoloReportSlave::~SSFSoloReportSlave()
{
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.

\param processedReport Contains the final game report and information used by game history.
\param playerIds list of players to distribute results to.
\return ERR_OK on success. GameReporting specific error on failure.

\Customizable - Virtual function.
********************************************************************************/
BlazeRpcError SSFSoloReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
	mProcessedReport = &processedReport;
	GameReport& report = processedReport.getGameReport();
	const GameType& gameType = processedReport.getGameType();
	BlazeRpcError processErr = Blaze::ERR_OK;

	// create the parser
	Utilities::ReportParser reportParser(gameType, processedReport);
	reportParser.setUpdateStatsRequestBuilder(&mBuilder);

	// fills in report with values via configuration
	if (false == reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
	{
		WARN_LOG("[GameSoloReportSlave::" << this << "].process(): Error parsing values");

		processErr = reportParser.getErrorCode();
		return processErr; // EARLY RETURN
	}

	// common processing
	processCommon();

	// initialize game parameters
	initGameParams();

	//setup the game status
	initGameStatus();

	// determine win/loss/tie
	determineGameResult();

	// update player keyscopes
	updatePlayerKeyscopes();

	//Do we need to update Squad keyscopes?
	//updateSquadKeyscopes();

	// parse the keyscopes from the configuration
	if (false == reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
	{
		WARN_LOG("[GameSoloReportSlave::" << this << "].process(): Error parsing keyscopes");

		processErr = reportParser.getErrorCode();
		return processErr; // EARLY RETURN
	}

	// update player stats
	updatePlayerStats();

	//Do we want to track DNF for solo games?
	//updateDNF();

	UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
#if defined (USERACTIVITYTRACKER_LOG_SOLO_STATS)
	statsUpdater.updateGamesPlayed(playerIds);
#endif
	statsUpdater.selectStats();

	// extract and set stats
	if (false == reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
	{
		WARN_LOG("[GameSoloReportSlave::" << this << "].process(): Error parsing stats");

		processErr = reportParser.getErrorCode();
		return processErr; // EARLY RETURN
	}

	// publish stats.
	bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
	processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);
	if (Blaze::ERR_OK != processErr)
	{
		TRACE_LOG("[GameSoloReportSlave:" << this << "].process() - initializeStatUpdate() returned: " << processErr);
	}

	processErr = mUpdateStatsHelper.fetchStats();
	if (Blaze::ERR_OK != processErr)
	{
		TRACE_LOG("[GameSoloReportSlave:" << this << "].process() - fetchStats() returned: " << processErr);
	}

	if (Blaze::ERR_OK == processErr)
	{
		processErr = mUpdateStatsHelper.calcDerivedStats();
		TRACE_LOG("[GameSoloReportSlave:" << this << "].process() - calcDerivedStats() returned: " << processErr);
	}

	if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
	{
		// extract game history attributes from report.
		GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
		reportParser.setGameHistoryReport(&historyReport);
		reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
	}

	if (Blaze::ERR_OK == processErr)
	{
		// post game reporting processing()
		if (false == processUpdatedStats())
		{
			processErr = Blaze::ERR_SYSTEM;
			TRACE_LOG("[GameSoloReportSlave:" << this << "].process() - processUpdatedStats() returned false. Failing with ERR_SYSTEM.");
		}
		else
		{
			statsUpdater.transformStats();

			if (Blaze::ERR_OK == processErr)
			{
				processErr = mUpdateStatsHelper.calcDerivedStats();
				TRACE_LOG("[GameSoloReportSlave:" << this << "].process() - calcDerivedStats() returned: " << processErr);
			}

			processErr = mUpdateStatsHelper.commitStats();
			TRACE_LOG("[GameSoloReportSlave:" << this << "].process() - commitStats() returned: " << processErr);
		}
	}

	// send end game mail
	sendEndGameMail(playerIds);
	
	// PC Anticheat, perform a Shield ClientChallenge on match end
	if (gCurrentUserSession->getClientPlatform() == pc)
	{
		Blaze::Gamereporting::Shield::doShieldClientChallenge(report.getGameReportName());
	}

	return processErr;
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
	Update the players' stats based on game result

	\Customizable - Virtual function to override default behavior.
	\Customizable - updateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void SSFSoloReportSlave::updatePlayerStats()
{
	TRACE_LOG("[SSFReportSlave:" << this << "].updatePlayerStats()");

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	for (; playerIter != playerEnd; ++playerIter)
	{
		GameManager::PlayerId playerId = playerIter->first;
		OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

		if (mTieGame)
		{
			updateTiePlayerStats(playerId, playerReport);
		}
		else if (mWinnerSet.find(playerId) != mWinnerSet.end())
		{
			updateWinPlayerStats(playerId, playerReport);
		}
		else if (mLoserSet.find(playerId) != mLoserSet.end())
		{
			updateLossPlayerStats(playerId, playerReport);
		}

		updatePlayerDNF(playerReport);
		updateCustomPlayerStats(playerId, playerReport);
	}
}

void SSFSoloReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	TRACE_LOG("[SSFSeasonsReportSlave:" << this << "].updateCustomPlayerStats()for player " << playerId << " ");
	SSFSeasonsReportBase::SSFSeasonsPlayerReport* ssfSeasonsPlayerReport = static_cast<SSFSeasonsReportBase::SSFSeasonsPlayerReport*>(playerReport.getCustomPlayerReport());

	//SSFTODO: Do we need these?
	//updateCleanSheets(ssfSeasonsPlayerReport->getPos(), ssfSeasonsPlayerReport->getTeamEntityId(), ssfSeasonsPlayerReport);
	//updatePlayerRating(&playerReport, ssfSeasonsPlayerReport);
	//updateMOM(&playerReport, ssfSeasonsPlayerReport);

	ssfSeasonsPlayerReport->setSsfEndResult(SSFGameResultHelper<SSFSoloReportSlave>::DetermineSSFUserResult(playerId, playerReport, true));
}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* SSFSoloReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("SSFSoloReportSlave") SSFSoloReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomSoloGameTypeName
    Return the game type name for solo game used in gamereporting.cfg

    \return - the Solo game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* SSFSoloReportSlave::getCustomSoloGameTypeName() const
{
    return "gameType28";
}

/*************************************************************************************************/
/*! \brief getCustomSoloStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for Solo game
*/
/*************************************************************************************************/
const char8_t* SSFSoloReportSlave::getCustomSoloStatsCategoryName() const
{
    return "SoloGameStats";
}

/*************************************************************************************************/
/*! \brief setCustomEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool SSFSoloReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
	return false;
}
*/
/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void SSFSoloReportSlave::updateCustomNotificationReport()
{
	// Obtain the custom data for report notification
	OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
	Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationCustomGameData *gameCustomData = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationCustomGameData();

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFSeasonsGameReport* ssfSeasonsGameReport = static_cast<SSFSeasonsReportBase::SSFSeasonsGameReport*>(osdkReport.getGameReport().getCustomGameReport());

	gameCustomData->setSecondsPlayed(osdkReport.getGameReport().getGameTime());
	gameCustomData->setMatchEndReason(osdkReport.getGameReport().getFinishedStatus());
	
	int64_t matchStatusHome = 0;
	int64_t matchStatusAway = 0;

	const OSDKReport::OSDKPlayerReportsMap& playerReportMap = osdkReport.getPlayerReports();
	for (const OSDKReport::OSDKPlayerReportsMap::value_type playerReportEntry : playerReportMap)
	{
		const OSDKPlayerReport * playerReport = playerReportEntry.second;
		Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationUserReportPtr notifyUserReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationUserReport();
		notifyUserReport->setPersonaId(playerReportEntry.first);
		notifyUserReport->setTeamSide(playerReport->getHome() ? 0 : 1);
		notifyUserReport->setSsfEndResult((static_cast<const SSFSeasonsReportBase::SSFSeasonsPlayerReport*>(playerReport->getCustomPlayerReport()))->getSsfEndResult());
		gameCustomData->getSSFUserReport().push_back(notifyUserReport);

		const OSDKGameReportBase::IntegerAttributeMap& privateIntMap = playerReport->getPrivatePlayerReport().getPrivateIntAttributeMap();
		OSDKGameReportBase::IntegerAttributeMap::const_iterator iter = privateIntMap.find("matchStatus");
		if (iter != privateIntMap.end())
		{
			if (playerReport->getHome())
			{
				matchStatusHome = iter->second;
			}
			else
			{
				matchStatusAway = iter->second;
			}
		}
	}

	eastl::string matchStatusStr;
	matchStatusStr.sprintf("%" PRId64 "_%" PRId64 "", matchStatusHome, matchStatusAway);
	gameCustomData->setMatchHash(matchStatusStr.c_str());

	for (SSFSeasonsReportBase::SSFTeamReportMap::value_type &teamReportMapEntry : ssfTeamSummaryReport->getSSFTeamReportMap())
	{
		SSFSeasonsReportBase::SSFTeamReport& teamReport = *(teamReportMapEntry.second);
		Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationTeamReportPtr notifyTeamReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationTeamReport();

		notifyTeamReport->setTeamId(teamReportMapEntry.first);
		notifyTeamReport->setGoals(teamReport.getCommonTeamReport().getGoals() + teamReport.getCommonTeamReport().getPkGoals());
		notifyTeamReport->setPlayOfTheMatch(isManOfMatchOnTheTeam(teamReport));

		for (SSFSeasonsReportBase::AvatarEntry* avatar : teamReport.getAvatarVector())
		{
			Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationAvatarStatsPtr notifyAvatarReport = BLAZE_NEW Blaze::GameReporting::SSFSeasonsReportBase::SSFNotificationAvatarStats();
			avatar->getAvatarId().copyInto(notifyAvatarReport->getAvatarId());
			notifyAvatarReport->setGoals(avatar->getAvatarStatReport().getGoals());
			notifyAvatarReport->setAssists(avatar->getAvatarStatReport().getAssists());
			notifyAvatarReport->setShots(avatar->getAvatarStatReport().getShots());
            notifyAvatarReport->setPasses(avatar->getAvatarStatReport().getPassesMade());
            notifyAvatarReport->setTackles(avatar->getAvatarStatReport().getTacklesMade());
            notifyAvatarReport->setBlocks(avatar->getAvatarStatReport().getBlocks());
			notifyAvatarReport->setAvatarRating(avatar->getAvatarStatReport().getRating());
			notifyAvatarReport->setAvatarType(-1); //SSFTODO 
			avatar->getSkillTimeBucketMap().copyInto(notifyAvatarReport->getSkillTimeBucketMap());
			notifyTeamReport->getAvatarVector().push_back(notifyAvatarReport);
		}

		int32_t teamSide = teamReport.getHome() ? 0 : 1;
		gameCustomData->getSSFTeamReport().insert(eastl::make_pair(teamSide, notifyTeamReport));

	}
	ssfSeasonsGameReport->getGoalSummary().copyInto(gameCustomData->getGoalSummary());

	// Set Report Id.
	gameCustomData->setGameReportingId(mProcessedReport->getGameReport().getGameReportingId());

	// Set the gameCustomData to the OsdkReportNotification
	OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

int32_t SSFSoloReportSlave::GetTeamGoals(SSFSeasonsReportBase::GoalEventVector& goalEvents, int64_t teamId) const
{
	int32_t goals = 0;
	for (SSFSeasonsReportBase::GoalEvent *goalEvent : goalEvents)
	{
		if (goalEvent->getScoringTeam() == teamId)
		{
			goals++;
		}
	}
	return goals;
}
bool SSFSoloReportSlave::isManOfMatchOnTheTeam(SSFSeasonsReportBase::SSFTeamReport& teamReport) const
{
	bool isMom = false;
	for (SSFSeasonsReportBase::AvatarEntry* avatar : teamReport.getAvatarVector())
	{
		isMom = isMom || avatar->getAvatarStatReport().getHasMOTM();
	}
	return isMom;
}

OSDKGameReportBase::OSDKReport& SSFSoloReportSlave::getOsdkReport()
{
	return static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
}
// FifaBaseReportSlave implementation
void SSFSoloReportSlave::determineGameResult()
{
	updateUnadjustedScore();

	adjustRedCardedPlayers();
	SSFGameResultHelper<SSFSoloReportSlave>::DetermineGameResult();
	adjustPlayerResult();
	SSFGameResultHelper<SSFSoloReportSlave>::updateSquadResults();
}

SSF::CommonGameReport* SSFSoloReportSlave::getGameReport()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport& osdkGameReport = osdkReport.getGameReport();
	SSFSeasonsReportBase::SSFSeasonsGameReport* ssfSeasonsGameReport = static_cast<SSFSeasonsReportBase::SSFSeasonsGameReport*>(osdkGameReport.getCustomGameReport());

	return &ssfSeasonsGameReport->getCommonGameReport();
}
SSFSeasonsReportBase::SSFTeamReportMap* SSFSoloReportSlave::getEntityReportMap()
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

	return &ssfTeamReportMap;
}

SSF::CommonStatsReport* SSFSoloReportSlave::getCommonPlayerReport(SSFSeasonsReportBase::SSFTeamReport& entityReport)
{
	return &entityReport.getCommonTeamReport();
}

bool SSFSoloReportSlave::didEntityFinish(int64_t entityId, SSFSeasonsReportBase::SSFTeamReport& entityReport)
{
	return (entityReport.getSquadDisc() == 0);
}

SSFSeasonsReportBase::SSFTeamReport* SSFSoloReportSlave::findEntityReport(int64_t entityId)
{
	EA::TDF::tdf_ptr<SSFSeasonsReportBase::SSFTeamReport> ssfTeamReport = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
	SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

	SSFSeasonsReportBase::SSFTeamReportMap::const_iterator teamIter;
	teamIter = ssfTeamReportMap.find(entityId);
	if (teamIter != ssfTeamReportMap.end())
	{
		ssfTeamReport = teamIter->second;
	}

	return ssfTeamReport;
}

int64_t SSFSoloReportSlave::getWinningEntity()
{
	return mWinEntityId;
}

int64_t SSFSoloReportSlave::getLosingEntity()
{
	return mLossEntityId;
}

bool SSFSoloReportSlave::isTieGame()
{
	return mTieGame;
}

bool SSFSoloReportSlave::isEntityWinner(int64_t entityId)
{
	return (mWinEntityId == entityId);
}

bool SSFSoloReportSlave::isEntityLoser(int64_t entityId)
{
	return (mLossEntityId == entityId);
}

int32_t SSFSoloReportSlave::getHighestEntityScore()
{
	return mHighestScore;
}

int32_t SSFSoloReportSlave::getLowestEntityScore()
{
	return mLowestScore;
}

void SSFSoloReportSlave::setCustomEntityResult(SSFSeasonsReportBase::SSFTeamReport& entityReport, StatGameResultE gameResult)
{
	if (gameResult == GAME_RESULT_CONCEDE_QUIT)
	{
		entityReport.setSquadDisc(1);
	}
}

void SSFSoloReportSlave::setCustomEntityScore(SSFSeasonsReportBase::SSFTeamReport& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst)
{
	SSF::CommonStatsReport& ssfCommonTeamReport = entityReport.getCommonTeamReport();
	ssfCommonTeamReport.setGoals(goalsFor);
	ssfCommonTeamReport.setGoalsConceded(goalsAgainst);
}

}   // namespace GameReporting

}   // namespace Blaze

