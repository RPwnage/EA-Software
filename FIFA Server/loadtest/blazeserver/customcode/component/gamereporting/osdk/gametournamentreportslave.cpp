/*************************************************************************************************/
/*!
    \file   gametournamentreportslave.cpp


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

#include "gametournamentreportslave.h"
#include "gamecommonlobbyskill.h"

#include "osdktournaments/tdf/osdktournamentstypes.h"

namespace Blaze
{

namespace GameReporting
{


GameReportProcessor* GameTournamentReportSlave::create(GameReportingSlaveImpl& component)
{
    // GameTournamentReportSlave should not be instantiated
    return NULL;
}

GameTournamentReportSlave::GameTournamentReportSlave(GameReportingSlaveImpl& component) :
    GameH2HReportSlave(component)
{
}

GameTournamentReportSlave::~GameTournamentReportSlave()
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
BlazeRpcError GameTournamentReportSlave::validate(GameReport& report) const
{
    return ERR_OK;
}

 /*! ****************************************************************************/
 /*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.

    \Customizable - Virtual function.
********************************************************************************/
BlazeRpcError GameTournamentReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* TOURNAMENT_GAME_TYPE_NAME = getCustomH2HGameTypeName();

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), TOURNAMENT_GAME_TYPE_NAME))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[GameTournamentReportSlave::" << this << "].process(): Error parsing values");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        // common processing
        processCommon();

        // initialize game parameters
        initGameParams();
        initCustomGameParams();

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
            WARN_LOG("[GameTournamentReportSlave::" << this << "].process(): Error parsing keyscopes");

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

        // update tournament stats
        updateTournamentStats();

        // extract and set stats
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
        {
            WARN_LOG("[GameTournamentReportSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (processErr == ERR_OK)
        {
            processErr = mUpdateStatsHelper.calcDerivedStats();
            TRACE_LOG("[GameTournamentReportSlave:" << this << "].process() - calcDerivedStats() returned: " << processErr);
        }

        if (processedReport.needsHistorySave() && processedReport.getReportType() != REPORT_TYPE_TRUSTED_MID_GAME)
        {
            // extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            mReportParser->setGameHistoryReport(&historyReport);
            mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
        }

        delete mReportParser;

        if(Blaze::ERR_OK == processErr)
        {
            // post game reporting processing()
            if (false == processUpdatedStats()) 
            {
                processErr = Blaze::ERR_SYSTEM;
                TRACE_LOG("[GameTournamentReportSlave:" << this << "].process() - processUpdatedStats() returned false. Failing with ERR_SYSTEM.");
            }
            else 
            {
                processErr = mUpdateStatsHelper.commitStats();  
                TRACE_LOG("[GameTournamentReportSlave:" << this << "].process() - commitStats() returned: " << processErr);
            }            
        }

        // send end game mail
        sendEndGameMail(playerIds);
    }

    return processErr;
}
 
/*******************************************************************************/
/*! \brief Triggered on server reconfiguration.

    \Customizable - Virtual function.
********************************************************************************/
void GameTournamentReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameTournamentReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
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
void GameTournamentReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateWinPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeLossPlayerStats
    Update winner's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameTournamentReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateLossPlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updateGameModeLossPlayerStats
    Update winner's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameTournamentReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateTiePlayerStats(playerId, playerReport);
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Set the value of player keyscopes defined in config and needed for config based stat update

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerKeyscopes() to provide additional custom behavior.
 */
/*************************************************************************************************/
void GameTournamentReportSlave::updatePlayerKeyscopes()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    updateCustomPlayerKeyscopes(OsdkPlayerReportsMap);
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameTournamentReportSlave::updatePlayerStats()
{
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
bool GameTournamentReportSlave::processUpdatedStats()
{
    bool bSuccess = processCustomUpdatedStats();
    updateNotificationReport();
    return bSuccess;
}

/*************************************************************************************************/
/*! \brief updateTournamentStats
    Update the tournament stats based on game result provided that this is a tournament game

    \Customizable - Virtual function to override default behavior.
    \Customizable - getCustomTournamentId() to provide additional custom behavior.
    \Customizable - updateCustomTournamentStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameTournamentReportSlave::updateTournamentStats()
{
    updateTournamentTeamStats();

    // extract the tournament id
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKGameReport& OsdkGameReport = OsdkReport.getGameReport();

    OSDKTournaments::TournamentId tournamentId;
    bool bValid = getCustomTournamentId(OsdkGameReport, tournamentId);

    if (true == bValid)
    {
        if (mPlayerReportMapSize != 2)
        {
            return; // EARLY RETURN
        }

        OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
        OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator player0Iter, player1Iter;
        player0Iter = player1Iter = OsdkPlayerReportsMap.begin();
        player1Iter++;

        Blaze::BlazeId userId0 = player0Iter->first;
        Blaze::BlazeId userId1 = player1Iter->first;

        OSDKGameReportBase::OSDKPlayerReport& player0Report = *player0Iter->second;
        OSDKGameReportBase::OSDKPlayerReport& player1Report = *player1Iter->second;

        uint32_t team0 = player0Report.getTeam();
        uint32_t team1 = player1Report.getTeam();

        uint32_t score0 = player0Report.getScore();
        uint32_t score1 = player1Report.getScore();

        // Tournament metadata is not supported until Blaze 2.5.
        char8_t metadata0[Blaze::OSDKTournaments::TOURN_MATCH_META_DATA_MAX_LENGTH];
        char8_t metadata1[Blaze::OSDKTournaments::TOURN_MATCH_META_DATA_MAX_LENGTH];
        memset(metadata0, 0, OSDKTournaments::TOURN_MATCH_META_DATA_MAX_LENGTH);
        memset(metadata1, 0, OSDKTournaments::TOURN_MATCH_META_DATA_MAX_LENGTH);

        updateCustomTournamentStats(tournamentId, userId0, userId1, team0, team1, score0, score1, metadata0, metadata1);

        TRACE_LOG("[GameTournamentReportSlave].updateTournamentStats() tournamentId: " << tournamentId << ", userId0: [" << userId0 << "], userId1: [" << userId1 << "], team0: " << team0 <<
                  ", team1: " << team1 << ", score0: " << score0 << ", score1: " << score1);
        mTournamentsUtil.reportMatchResult(tournamentId, userId0, userId1, team0, team1, score0, score1, metadata0, metadata1);
    }
}

/*************************************************************************************************/
/*! \brief updateTournamentTeamStats
    Update the tournament team stats

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomTournamentTeamStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameTournamentReportSlave::updateTournamentTeamStats()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    updateCustomTournamentTeamStats(OsdkPlayerReportsMap);
}

/*************************************************************************************************/
/*! \brief updateNotificationReport
    Update Notification Report.

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomNotificationReport() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameTournamentReportSlave::updateNotificationReport() 
{ 
    updateCustomNotificationReport(getCustomTournamentStatsCategoryName()); 
};

}   // namespace GameReporting

}   // namespace Blaze
