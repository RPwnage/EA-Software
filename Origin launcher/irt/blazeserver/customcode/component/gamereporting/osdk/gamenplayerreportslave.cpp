/*************************************************************************************************/
/*!
    \file   gamenplayerreportslave.cpp

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

#include "gamenplayerreportslave.h"
#include "gamecommonlobbyskill.h"
#include "ping/tdf/gamenplayerreport.h"

namespace Blaze
{

namespace GameReporting
{

const char8_t* CATEGORYNAME_NPLAYERGAMESTATS = "NPlayerGameStats";

/*************************************************************************************************/
/*! \brief GameNPlayerReportSlave
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameNPlayerReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameNPlayerReportSlave") GameNPlayerReportSlave(component);
}

/*************************************************************************************************/
/*! \brief GameNPlayerReportSlave
    Constructor
*/
/*************************************************************************************************/
GameNPlayerReportSlave::GameNPlayerReportSlave(GameReportingSlaveImpl& component) :
    GameH2HReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief GameNPlayerReportSlave
    Destructor
*/
/*************************************************************************************************/
GameNPlayerReportSlave::~GameNPlayerReportSlave()
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
BlazeRpcError GameNPlayerReportSlave::validate(GameReport& report) const
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
BlazeRpcError GameNPlayerReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* NPLAYER_GAME_TYPE_NAME = "gameType6";

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), NPLAYER_GAME_TYPE_NAME))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[GameNPlayerReportSlave::" << this << "].process(): Error parsing values");

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
            WARN_LOG("[GameNPlayerReportSlave::" << this << "].process(): Error parsing keyscopes");

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

        // extract and set stats
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
        {
            WARN_LOG("[GameNPlayerReportSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
            // NPlayer does not have skill points and skill stats so do not call transformStats

            processErr = mUpdateStatsHelper.calcDerivedStats();
            TRACE_LOG("[GameNPlayerReportSlave:" << this << "].process() - calcDerivedStats() returned: " << processErr);
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
                TRACE_LOG("[GameNPlayerReportSlave:" << this << "].process() - processUpdatedStats() returned false. Failing with ERR_SYSTEM.");
            }
            else 
            {
                processErr = mUpdateStatsHelper.commitStats();  
                TRACE_LOG("[GameNPlayerReportSlave:" << this << "].process() - commitStats() returned: " << processErr);
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
void GameNPlayerReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameNPlayerReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    playerReport.setPointsAgainst(mAggregatedPlayerStatsMap[AGGREGATE_SCORE] - playerReport.getScore());

    // update player's DNF stat
    updatePlayerDNF(playerReport);
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameNPlayerReportSlave::updatePlayerStats()
{
    TRACE_LOG("[GameNPlayerReportSlave:" << this << "].updatePlayerStats()");

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

        // No keyscope for nplayer stats category
        mBuilder.startStatRow(CATEGORYNAME_NPLAYERGAMESTATS, playerId);

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

        mBuilder.completeStatRow();
    }
}

/*************************************************************************************************/
/*! \brief updatePlayerStatsConclusionType
    Update the players' stats based on the conclusion type

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameNPlayerReportSlave::updatePlayerStatsConclusionType()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

        uint32_t userResult = playerReport.getUserResult(); // ATTRIBNAME_USERRESULT
        if (GAME_RESULT_DNF_QUIT == userResult || GAME_RESULT_DNF_DISCONNECT == userResult)
        {
            // No keyscope for coop stats category
            mBuilder.startStatRow(CATEGORYNAME_COOPGAMESTATS, playerId);
            mBuilder.incrementStat(STATNAME_TOTAL_GAMES_PLAYED, 1);
            mBuilder.incrementStat(STATNAME_GAMES_NOT_FINISHED, 1);
            mBuilder.incrementStat(STATNAME_LOSSES, 1);

            mBuilder.completeStatRow();
        }
    }
}

}   // namespace GameReporting

}   // namespace Blaze
