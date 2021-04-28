/*************************************************************************************************/
/*!
    \file   gamesoloreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/util/reportparserutil.h"

#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

#include "gamesoloreportslave.h"

#include "useractivitytracker/stats_updater.h"


namespace Blaze
{

namespace GameReporting
{

GameReportProcessor* GameSoloReportSlave::create(GameReportingSlaveImpl& component)
{
    // GameSoloReportSlave should not be instantiated
    return NULL;
}

GameSoloReportSlave::GameSoloReportSlave(GameReportingSlaveImpl& component) :
    GameCommonReportSlave(component)
{
}

GameSoloReportSlave::~GameSoloReportSlave()
{
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted to master for collation or direct to 
    processing for offline or trusted reports. Behavior depends on the calling RPC.

    \param report Incoming game report from submit request
    \return ERR_OK on success. GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError GameSoloReportSlave::validate(GameReport& report) const
{
    return Blaze::ERR_OK;
}


/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError GameSoloReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
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

    // update player keyscopes
    updatePlayerKeyscopes();

    // parse the keyscopes from the configuration
    if(false == reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
    {
        WARN_LOG("[GameSoloReportSlave::" << this << "].process(): Error parsing keyscopes");

        processErr = reportParser.getErrorCode();
        return processErr; // EARLY RETURN
    }

    // update player stats
    updatePlayerStats();

	UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
#if defined (USERACTIVITYTRACKER_LOG_SOLO_STATS)
	statsUpdater.updateGamesPlayed(playerIds);
#endif
	statsUpdater.selectStats();

    // extract and set stats
    if(false == reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
    {
        WARN_LOG("[GameSoloReportSlave::" << this << "].process(): Error parsing stats");

        processErr = reportParser.getErrorCode();
        return processErr; // EARLY RETURN
    }

    // publish stats.
    bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
    processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

    if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME  != processedReport.getReportType())
    {
        // extract game history attributes from report.
        GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
        reportParser.setGameHistoryReport(&historyReport);
        reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
    }

	//----------------------------------------------------------------------------------
    // Perform the fetching of the select statements
    //----------------------------------------------------------------------------------
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

	if(Blaze::ERR_OK == processErr)
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

			processErr = mUpdateStatsHelper.commitStats();  
			TRACE_LOG("[GameSoloReportSlave:" << this << "].process() - commitStats() returned: " << processErr);
		}            
	}

    // send end game mail
    sendEndGameMail(playerIds);

    return processErr;
}
 
/******************************************************************************/
/*! \brief Triggered on server reconfiguration.

    \Customizable - Virtual function.
********************************************************************************/
void GameSoloReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief skipProcessConclusionType
        Check if stats update is skipped because of conclusion type

    \return - true if game report processing is to be skipped
    \return - false if game report should be processed

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
bool GameSoloReportSlave::skipProcessConclusionType()
{
    // Conclusion type is only enabled for H2H game mode
    return false;
}

/*************************************************************************************************/
/*! \brief processUpdatedStats
    Perform post stats update processing

    \return - true if the post-game processing is performed successfully

    \Customizable - Virtual function to override default behavior.
    \Customizable - processCustomUpdatedStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool GameSoloReportSlave::processUpdatedStats()
{
    bool bSuccess = processCustomUpdatedStats();
    updateNotificationReport();
    return bSuccess;
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameSoloReportSlave::updatePlayerStats()
{
    TRACE_LOG("[GameSoloReportSlave:" << this << "].updatePlayerStats()");

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;

		if(playerId > 0)
		{
			OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
	
			mBuilder.startStatRow(getCustomSoloStatsCategoryName(), playerId);
		
			// update common player's stats
			updateCustomPlayerStats(playerId, playerReport);

			mBuilder.completeStatRow();
		}
    }
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Set the value of player keyscopes defined in config and needed for config based stat update

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerKeyscopes() to provide additional custom behavior.
 */
/*************************************************************************************************/
void GameSoloReportSlave::updatePlayerKeyscopes()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    updateCustomPlayerKeyscopes(OsdkPlayerReportsMap);
}

/*************************************************************************************************/
/*! \brief sendEndGameMail
    Send end of game email/sms

    \Customizable - Virtual function to override default behavior.
    \Customizable - setCustomEndGameMailParam() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameSoloReportSlave::sendEndGameMail(GameManager::PlayerIdList& playerIds)
{
    // if this is not an offline or not a normal game
    if(true == mIsOfflineReport)
    {
        return;
    }
/*
    // prepare mail generation
    // additional parameters are any additional template parameters that will be identical for all players
    Mail::HttpParamList mailParams;

    // allow game team to add custom parameters
    char8_t mailTemplateName[MAX_MAIL_TEMPLATE_NAME_LEN];
    bool bSendMail = setCustomEndGameMailParam(&mailParams, mailTemplateName);
	bool bSendMail = false;

    if(true == bSendMail)
    {
        // set which opt-in flags control preferences for this email.
        Mail::EmailOptInFlags gameReportingOptInFlags;
        gameReportingOptInFlags.setEmailOptinTitleFlags(EMAIL_OPTIN_GAMEREPORTING_FLAG);
        mGenerateMailUtil.generateMail(mailTemplateName, &mailParams, &gameReportingOptInFlags, playerIds);
    }
*/
}

}   // namespace GameReporting
}   // namespace Blaze
