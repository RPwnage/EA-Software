/*************************************************************************************************/
/*!
    \file   futsoloreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "futsoloreportslave.h"

#include "fifa_shield/fifashieldgamereporthelper.h"

#include "fifa/tdf/fifasoloreport.h"

#include "useractivitytracker/stats_updater.h"


namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief FutSoloReportSlave
    Constructor
*/
/*************************************************************************************************/
FutSoloReportSlave::FutSoloReportSlave(GameReportingSlaveImpl& component) :
FifaSoloReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief FutSoloReportSlave
    Destructor
*/
/*************************************************************************************************/
FutSoloReportSlave::~FutSoloReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FutSoloReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FutSoloReportSlave::create(GameReportingSlaveImpl& component)
{
	return BLAZE_NEW_NAMED("FutSoloReportSlave") FutSoloReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomSoloGameTypeName
    Return the game type name for solo game used in gamereporting.cfg

    \return - the Solo game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FutSoloReportSlave::getCustomSoloGameTypeName() const
{
    return "gameType7";
}

/*************************************************************************************************/
/*! \brief getCustomSoloStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for Solo game
*/
/*************************************************************************************************/
const char8_t* FutSoloReportSlave::getCustomSoloStatsCategoryName() const
{
    return "SoloGameStats";
}



/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError FutSoloReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
	Blaze::BlazeRpcError processErr = Blaze::ERR_OK;

	mProcessedReport = &processedReport;
	
	GameReport& report = processedReport.getGameReport();
	const GameType& gameType = processedReport.getGameType();

	Utilities::ReportParser *reportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
	if (false == reportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
	{
		WARN_LOG("[FutH2HReportSlave::" << this << "].process(): Error parsing values");
		delete reportParser;
		reportParser = NULL;
	}

	// common processing
	processCommon();


	UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
#if defined (USERACTIVITYTRACKER_LOG_SOLO_STATS)
	statsUpdater.updateGamesPlayed(playerIds);
#endif
	statsUpdater.selectStats();

	// publish stats.
	bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
	processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);
	if (Blaze::ERR_OK != processErr)
	{
		TRACE_LOG("[FutSoloReportSlave:" << this << "].process() - initializeStatUpdate() returned: " << processErr);
	}

	processErr = mUpdateStatsHelper.fetchStats();
	if (Blaze::ERR_OK != processErr)
	{
		TRACE_LOG("[FutSoloReportSlave:" << this << "].process() - fetchStats() returned: " << processErr);
	}

	if (reportParser != NULL && processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
	{
		// extract game history attributes from report.
		GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
		reportParser->setGameHistoryReport(&historyReport);
		reportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
		delete reportParser;
		reportParser = NULL;
	}

	if (Blaze::ERR_OK == processErr)
	{
		statsUpdater.transformStats();
		processErr = mUpdateStatsHelper.commitStats();
		TRACE_LOG("[FutSoloReportSlave:" << this << "].process() - commitStats() returned: " << processErr);
	}

	// post game reporting processing()
	processErr = (true == processUpdatedStats()) ? Blaze::ERR_OK : Blaze::ERR_SYSTEM;

	// PC Anticheat, perform a Shield ClientChallenge on match end
	if (gController->isPlatformHosted(pc))
	{
		Blaze::Gamereporting::Shield::doShieldClientChallenge(report.getGameReportName());
	}

	return processErr;
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
bool FutSoloReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
	return false;
}
*/

/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void FutSoloReportSlave::updateCustomNotificationReport()
{
	// Obtain the custom data for report notification
	OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
	Fifa::SoloNotificationCustomGameData *gameCustomData = BLAZE_NEW Fifa::SoloNotificationCustomGameData();

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	TRACE_LOG(__FUNCTION__ << " osdkReport.getPlayerReports().size(): " << osdkReport.getPlayerReports().size());


	// Set Report Id for FUT.
	gameCustomData->setGameReportingId(mProcessedReport->getGameReport().getGameReportingId());

	// Set the gameCustomData to the OsdkReportNotification
	OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

}   // namespace GameReporting

}   // namespace Blaze

