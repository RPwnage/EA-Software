/*************************************************************************************************/
/*!
    \file   gameh2hplayoffreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/util/reportparserutil.h"

#include "gamereporting/osdk/osdkseasonalplayutil.h"

#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

#include "gameh2hplayoffreportslave.h"
#include "gamecommonlobbyskill.h"

namespace Blaze
{

namespace GameReporting
{

const char8_t* GameH2HPlayoffSlave::AGGREGATE_SCORE = "SCORE";

/*************************************************************************************************/
/*! \brief GameH2HPlayoffSlave
    Create
    O
    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameH2HPlayoffSlave::create(GameReportingSlaveImpl& component)
{
    // GameH2HPlayoffSlave should not be instantiated
    return NULL;
}

/*************************************************************************************************/
/*! \brief GameH2HPlayoffSlave
    Constructor
*/
/*************************************************************************************************/
GameH2HPlayoffSlave::GameH2HPlayoffSlave(GameReportingSlaveImpl& component) :
    GameCommonReportSlave(component),
    mReportParser(NULL)
{
}

/*************************************************************************************************/
/*! \brief GameH2HPlayoffSlave
    Destructor
*/
/*************************************************************************************************/
GameH2HPlayoffSlave::~GameH2HPlayoffSlave()
{
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted to master for collation or direct to 
    processing for offline or trusted reports. Behavior depends on the calling RPC.

    \param report Incoming game report from submit request
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError GameH2HPlayoffSlave::validate(GameReport& report) const
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
BlazeRpcError GameH2HPlayoffSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* H2HPLAYOFF_GAME_TYPE_NAME = getCustomH2HPlayoffGameTypeName();

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), H2HPLAYOFF_GAME_TYPE_NAME))
    {
        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[GameH2HPlayoffSlave::" << this << "].process(): Error parsing values");

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

        // determine win/loss/tie
        determineGameResult(); 

        // update player keyscopes
        updatePlayerKeyscopes();

        // parse the keyscopes from the configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
        {
            WARN_LOG("[GameH2HPlayoffSlave::" << this << "].process(): Error parsing keyscopes");

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

        // update seasonal stats
        updateH2HSeasonalStats();

        // extract and set stats
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
        {
            WARN_LOG("[GameH2HPlayoffSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
            // H2H Playoff game does not have skill points and skill stats so do not call transformStats

            processErr = mUpdateStatsHelper.calcDerivedStats();
            TRACE_LOG("[GameH2HPlayoffSlave:" << this << "].process() - calcDerivedStats() returned: " << processErr);
        }

        if (processedReport.needsHistorySave() && REPORT_TYPE_TRUSTED_MID_GAME != processedReport.getReportType())
        {
            // extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            mReportParser->setGameHistoryReport(&historyReport);
            mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
            delete mReportParser;
        }

        if(Blaze::ERR_OK == processErr)
        {
            // post game reporting processing()
            if (false == processUpdatedStats()) 
            {
                processErr = Blaze::ERR_SYSTEM;
                TRACE_LOG("[GameH2HPlayoffSlave:" << this << "].process() - processUpdatedStats() returned false. Failing with ERR_SYSTEM.");
            }
            else
            {
                processErr = mUpdateStatsHelper.commitStats();  
                TRACE_LOG("[GameH2HPlayoffSlave:" << this << "].process() - commitStats() returned: " << processErr);
            }            
        }

        // send end game mail
        sendEndGameMail(playerIds);

        // delete the report parser
        delete mReportParser;
    }

    return processErr;
}
 
/*******************************************************************************/
/*! \brief Triggered on server reconfiguration.

    \Customizable - Virtual function.
********************************************************************************/
void GameH2HPlayoffSlave::reconfigure() const
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
bool GameH2HPlayoffSlave::skipProcessConclusionType()
{
    return skipProcessConclusionTypeCommon();
}

/*************************************************************************************************/
/*! \brief determineGameResult
    Determine the game results

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameH2HPlayoffSlave::determineGameResult()
{
    determineWinPlayer();
}

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameH2HPlayoffSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    playerReport.setPointsAgainst(mAggregatedPlayerStatsMap[AGGREGATE_SCORE] - playerReport.getScore());

    // update player's DNF stat
    updatePlayerDNF(playerReport);

    updateCustomPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeWinPlayerStats
    Update winner's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameH2HPlayoffSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateWinPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeLossPlayerStats
    Update loser's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameH2HPlayoffSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateLossPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeTiePlayerStats
    Update tied player's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameH2HPlayoffSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateTiePlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameH2HPlayoffSlave::updatePlayerStats()
{
    TRACE_LOG("[GameH2HPlayoffSlave:" << this << "].updatePlayerStats()");

    // aggregate players' stats
    aggregatePlayerStats();

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

        if (mTieGame)
        {
            updateGameModeTiePlayerStats(playerId, playerReport);
        }
        else if (mWinnerSet.find(playerId) != mWinnerSet.end())
        {
            updateGameModeWinPlayerStats(playerId, playerReport);
        }
        else if (mLoserSet.find(playerId) != mLoserSet.end())
        {
            updateGameModeLossPlayerStats(playerId, playerReport);
        }

        // update common player's stats
        updateCommonStats(playerId, playerReport);
    }
}

/*************************************************************************************************/
/*! \brief processUpdatedStats
    Perform post stats update processing

    \return - true if the post-game processing is performed successfully

    \Customizable - Virtual function to override default behavior.
    \Customizable - processCustomUpdatedStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool GameH2HPlayoffSlave::processUpdatedStats()
{
    bool bSuccess = processCustomUpdatedStats();
    updateNotificationReport();
    return bSuccess;
}

/*************************************************************************************************/
/*! \brief aggregatePlayerStats
    Calculate the aggregated value of player stats and store them in a map

    \Customizable - aggregateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameH2HPlayoffSlave::aggregatePlayerStats()
{
    TRACE_LOG("[GameH2HPlayoffSlave:" << this << "].aggregatePlayerStats()");

    mAggregatedPlayerStatsMap[AGGREGATE_SCORE] = 0;

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

        mAggregatedPlayerStatsMap[AGGREGATE_SCORE] += playerReport.getScore();
    }

    aggregateCustomPlayerStats(OsdkPlayerReportsMap);
}

/*************************************************************************************************/
/*! \brief updatePlayerSeasonalStats
    Update seasonal stats for players.
    TODO: this should become part of Seasonal Play util

    \Customizable - None
*/
/*************************************************************************************************/
void GameH2HPlayoffSlave::updateH2HSeasonalStats()
{
    TRACE_LOG("[GameH2HPlayoffSlave:" << this << "].updateH2HSeasonalStats()");

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    OSDKSeasonalPlayUtil seasonalPlayUtil;
    for(; playerIter != playerEnd; ++playerIter)
    {
        seasonalPlayUtil.setMember(static_cast<OSDKSeasonalPlay::MemberId>(playerIter->first), OSDKSeasonalPlay::SEASONALPLAY_MEMBERTYPE_USER);

        // only playoff stats are reported
        if (OSDKSeasonalPlay::SEASONALPLAY_SEASON_STATE_PLAYOFF == seasonalPlayUtil.getSeasonState())
        {
            TRACE_LOG("[GameH2HPlayoffSlave:" << this << "].updateH2HSeasonalStats()");
            updatePlayoffStats(seasonalPlayUtil.getSeasonTournamentId());
        }
    }
}

/*************************************************************************************************/
/*!
    \brief updatePlayoffStats
    Update the playoff stats based on game result provided that this is a playoff game

    \Customizable - None
*/
/*************************************************************************************************/
void GameH2HPlayoffSlave::updatePlayoffStats(OSDKTournaments::TournamentId tournamentId)
{
    if (OSDKTournaments::INVALID_TOURNAMENT_ID != tournamentId)
    {
        if (2 != mPlayerReportMapSize)
        {
            return; // EARLY RETURN
        }

        OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
        OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

        OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator player0Iter, player1Iter;
        player0Iter = player1Iter = OsdkPlayerReportsMap.begin();
        ++player1Iter;

        Blaze::BlazeId userId0 = player0Iter->first;
        Blaze::BlazeId userId1 = player1Iter->first;

        OSDKGameReportBase::OSDKPlayerReport& player0Report = *player0Iter->second;
        OSDKGameReportBase::OSDKPlayerReport& player1Report = *player1Iter->second;

        uint32_t team0 = player0Report.getTeam();
        uint32_t team1 = player1Report.getTeam();

        uint32_t score0 = player0Report.getScore();
        uint32_t score1 = player1Report.getScore();

        const char8_t* metadata0 = "";
        const char8_t* metadata1 = "";

        TRACE_LOG("[GameH2HPlayoffSlave].updatePlayoffStats() tournamentId: " << tournamentId << ", userId0: [" << userId0 << "], userId1: [" << userId1 <<
                  "], team0: " << team0 << ", team1: " << team1 << ", score0: " << score0 << ", score1: " << score1);
        mTournamentsUtil.reportMatchResult(tournamentId, userId0, userId1, team0, team1, score0, score1, metadata0, metadata1);
    }
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Set the value of player keyscopes defined in config and needed for config based stat update

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerKeyscopes() to provide additional custom behavior.
 */
/*************************************************************************************************/
void GameH2HPlayoffSlave::updatePlayerKeyscopes()
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
void GameH2HPlayoffSlave::sendEndGameMail(GameManager::PlayerIdList& playerIds)
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
