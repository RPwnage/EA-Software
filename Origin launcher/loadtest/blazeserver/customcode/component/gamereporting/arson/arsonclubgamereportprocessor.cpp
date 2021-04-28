/*************************************************************************************************/
/*!
    \file   arsonclubgamereportprocessor.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

#include "gamereporting/gamereportingslaveimpl.h"
#include "gamereporting/util/reportparserutil.h"
#include "gamereporting/util/updateclubsutil.h"
#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"
#include "arsonclubgamereportprocessor.h"

namespace Blaze
{
namespace GameReporting
{

GameReportProcessor* ArsonClubGameReportProcessor::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("ArsonClubGameReportProcessor") ArsonClubGameReportProcessor(component);
}

ArsonClubGameReportProcessor::ArsonClubGameReportProcessor(GameReportingSlaveImpl& component) :
    GameReportProcessor(component)
{
}

ArsonClubGameReportProcessor::~ArsonClubGameReportProcessor()
{
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted to master for collation or direct to 
    processing for offline or trusted reports.   Behavior depends on the calling RPC.

    \param report Incoming game report from submit request
    \return ERR_OK on success.  GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError ArsonClubGameReportProcessor::validate(GameReport& report) const
{
    const GameType* gameType = mComponent.getGameTypeCollection().getGameType(report.getGameReportName());
    if (gameType == nullptr)
    {
        return Blaze::GAMEREPORTING_ERR_INVALID_GAME_TYPE;
    }
#ifdef TARGET_arson
    if (blaze_strcmp(gameType->getGameReportName().c_str(), "arsonClub") == 0)
    {
        ArsonClub::Report& arsonClubReport = static_cast<ArsonClub::Report&>(*report.getReport());
        ArsonClub::Report::ClubReportsMap::const_iterator clubIter = arsonClubReport.getClubReports().begin();
        ArsonClub::Report::ClubReportsMap::const_iterator clubEnd = arsonClubReport.getClubReports().end();

        UpdateClubsUtil updateClubsUtil;

        for (; clubIter != clubEnd; ++clubIter)
        {
            Clubs::ClubId clubId = static_cast<Clubs::ClubId>(clubIter->first);

            if (!updateClubsUtil.validateClubId(clubId))
            {
                WARN_LOG("[ArsonClubGameReportProcessor].validate(): Club id " << clubId << " found in club reports does not exist");
                return CLUBS_ERR_INVALID_CLUB_ID;
            }
        }
    }
#endif
    return ERR_OK;
}

/*! ****************************************************************************/
/*! \brief Deterimines a club's score.
********************************************************************************/

#ifdef TARGET_arson
int32_t ArsonClubGameReportProcessor::getClubScore(const ArsonClub::ClubReport& clubReport)
{
    return (int32_t)(clubReport.getPoints());
}
#endif 

 /*! ****************************************************************************/
 /*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success.  GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError ArsonClubGameReportProcessor::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    bool success = false;

#ifdef TARGET_arson
    const GameType& gameType = processedReport.getGameType();
    GameReport& report = processedReport.getGameReport();
    Stats::UpdateStatsRequestBuilder builder;
    
    if (blaze_strcmp(gameType.getGameReportName().c_str(), "arsonClub") == 0)
    {
        success = processArsonClub(gameType, processedReport, static_cast<ArsonClub::Report&>(*report.getReport()), builder, playerIds);
    }
    else if (blaze_strcmp(gameType.getGameReportName().c_str(), "gameHistoryClubs_NonDerived") == 0)
    {
        success = processArsonGameHistoryClub(gameType, processedReport, static_cast<GameHistoryClubs_NonDerived::Report&>(*report.getReport()), builder, playerIds);
    }
    else
    {
        ERR_LOG("[ArsonClubGameReportProcessor].process(): Reporting must be of type 'arsonClub' or 'gameHistoryClubs_NonDerived' but is of type '" 
                << gameType.getGameReportName().c_str() << "'.  Check gameType configuration inside gamereporting.cfg to verify this custom processor can handle the game type.");
        return GAMEREPORTING_ERR_INVALID_GAME_TYPE;
    }

    // extract and set stats
    Utilities::ReportParser reportParser(gameType,  processedReport);

    if (success)
    {
        BlazeRpcError processErr = ERR_OK;

        reportParser.setUpdateStatsRequestBuilder(&builder);
        success = reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES | Utilities::ReportParser::REPORT_PARSE_STATS);

        if (success)
        {
            //  publish stats.
            Stats::UpdateStatsHelper updateStatsHelper;

            if ((processErr = updateStatsHelper.initializeStatUpdate(builder)) == ERR_OK)
            {
                //  commitStats requires stat updates - will return false if there are none.
                if (updateStatsHelper.getUpdateStatsRequest().getStatUpdates().size() > 0)
                {
                    processErr = updateStatsHelper.commitStats();
                }
            }

            // extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            reportParser.setGameHistoryReport(&historyReport);
            reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);

            processedReport.enableHistorySave(processedReport.getReportType() != REPORT_TYPE_TRUSTED_MID_GAME);
        }
        else
        {
            processErr = reportParser.getErrorCode();
        }

        return processErr;
    }
#endif
    return success ? ERR_OK : ERR_SYSTEM;
}

/////////////////////////////////////////////////////////////////////////////////
#ifdef TARGET_arson
bool ArsonClubGameReportProcessor::processArsonClub(const GameType& gameType, ProcessedGameReport& processedReport, ArsonClub::Report& report, Stats::UpdateStatsRequestBuilder& builder, GameManager::PlayerIdList& playerIds)
{
    // determine winner, loser and winnerByDnf based on the final report.
    ArsonClub::Report& arsonClubReport = report;
    //const CollatedGameReport::DnfStatusMap &dnfStatus = processedReport.getDnfStatusMap();   

    // iterate through clubs to find the best and worst scores.
    int32_t lowestClubScore = INT32_MAX;
    int32_t highestClubScore = INT32_MIN;

    ArsonClub::Report::ClubReportsMap::const_iterator clubIter = arsonClubReport.getClubReports().begin();
    ArsonClub::Report::ClubReportsMap::const_iterator clubEnd = arsonClubReport.getClubReports().end();

    Clubs::ClubId winningClubId = static_cast<Clubs::ClubId>(clubIter->first);
    Clubs::ClubIdList clubIdList; 

    for (; clubIter != clubEnd; ++clubIter)
    {
        ArsonClub::ClubReport& clubReport = *clubIter->second;
        int32_t clubScore = getClubScore(clubReport);

        if (clubScore > highestClubScore)
        {
            // update the winning club id and the highest club score
            winningClubId = static_cast<Clubs::ClubId>(clubIter->first);
            highestClubScore = clubScore;
        }
        if (clubScore < lowestClubScore)
        {
            // update the lowest club score
            lowestClubScore = clubScore;
        }

        clubIdList.push_back(clubIter->first);
    }

    bool tieGame = (arsonClubReport.getClubReports().size() > 1) && (highestClubScore == lowestClubScore);

    // set clubs and player stats
    UpdateClubsUtil updateClubsUtil;

    bool success = true;

    // update club stats
    if (updateClubsUtil.initialize(clubIdList))
    {
        clubIter = arsonClubReport.getClubReports().begin();
        clubEnd = arsonClubReport.getClubReports().end();

        int64_t currentRecordPoints = 0;
        BlazeId currentRecordHolder = 0;
        bool recordExists;
        bool newRecord = false;
        bool updateRecord = false;

        for (; clubIter != clubEnd; ++clubIter)
        {
            Clubs::ClubId clubId = static_cast<Clubs::ClubId>(clubIter->first);
            ArsonClub::ClubReport& clubReport = *clubIter->second;

            Clubs::ClubSettings clubSettings;
            updateClubsUtil.getClubSettings(clubId, clubSettings);

            clubReport.setClubRegion(clubSettings.getRegion());
            clubReport.setSeasonLevel(clubSettings.getSeasonLevel());

            // fetch all records for club
            if (!updateClubsUtil.fetchClubRecords(clubId))
            {
                WARN_LOG("[ArsonClubGameReportProcessor].process(): Failed to read records for club " << clubId);
                success = false;
                break;
            }
            else
            {
                Clubs::ClubRecordbook clubRecordbook;

                // record #1 is configured as highest number of points scored in a game
                recordExists = updateClubsUtil.getClubRecord(1, clubRecordbook);
                currentRecordPoints = recordExists ? clubRecordbook.getStatValueInt() : 0;

                ArsonClub::Report::PlayerReportsMap::const_iterator playerIter = arsonClubReport.getPlayerReports().begin();
                ArsonClub::Report::PlayerReportsMap::const_iterator playerEnd = arsonClubReport.getPlayerReports().end();

                for (; playerIter != playerEnd; ++playerIter)
                {
                    ArsonClub::PlayerReport& playerReport = *playerIter->second;

                    if (clubId == playerReport.getClubId())
                    {
                        newRecord = recordExists ? (playerReport.getPoints() > currentRecordPoints) : true;

                        if (newRecord)
                        {
                            updateRecord = true;
                            currentRecordHolder = playerIter->first;
                            currentRecordPoints = playerReport.getPoints();
                            recordExists = true;
                        }
                    }
                }

                if (success && updateRecord)
                {
                    success = updateClubsUtil.updateClubRecordInt(clubId, 1,
                        currentRecordHolder, currentRecordPoints);
                }

                // award with AwardId 1 is award for simply taking place in the event
                // so each club which finishes the game is awarded
                if (success)
                {
                    success = updateClubsUtil.awardClub(clubId, 1);
                }
            }

            // check if this was a game between rivals
            bool isRival = false;
            Clubs::ClubsComponentSettings clubConfig;
            if (!updateClubsUtil.getClubsComponentSettings(clubConfig))
            {
                WARN_LOG("[ArsonClubGameReportProcessor].process(): Failed to get club component settings");
                success = false;
                break;
            }

            Clubs::ClubRivalList rivalList;
            updateClubsUtil.listClubRivals(clubId, rivalList);

            if (rivalList.size() > 0)
            {
                Clubs::ClubRival *rival = *(rivalList.begin());
                if (static_cast<int64_t>(rival->getCreationTime()) > clubConfig.getSeasonStartTime())
                {
                    isRival = true;
                    clubReport.setRivalPoints(clubReport.getPoints());
                }
                else
                {
                    clubReport.setRivalPoints(0);
                }
            }
            else
            {
                clubReport.setRivalPoints(0);
            }

            if (tieGame)
            {
                clubReport.setWins(0);
                clubReport.setLosses(0);
                clubReport.setTies(1);
                clubReport.setRivalWins(0);
                clubReport.setRivalLosses(0);
                isRival ? clubReport.setRivalTies(1) : clubReport.setRivalTies(0);
            }
            else if (clubId == winningClubId)
            {
                clubReport.setWins(1);
                clubReport.setLosses(0);
                clubReport.setTies(0);
                clubReport.setRivalTies(0);
                clubReport.setRivalLosses(0);
                isRival ? clubReport.setRivalWins(1) : clubReport.setRivalWins(0);
            }
            else
            {
                clubReport.setWins(0);
                clubReport.setLosses(1);
                clubReport.setTies(0);
                clubReport.setRivalWins(0);
                clubReport.setRivalTies(0);
                isRival ? clubReport.setRivalLosses(1) : clubReport.setRivalLosses(0);
            }
        }

        // update clubs component on result of game
        if (success)
        {
            success = updateClubsUtil.updateClubs(
                arsonClubReport.getClubReports().begin()->first,
                arsonClubReport.getClubReports().begin()->second->getPoints(),
                (arsonClubReport.getClubReports().begin()+1)->first,
                (arsonClubReport.getClubReports().begin()+1)->second->getPoints());
        }

        updateClubsUtil.completeTransaction(success);
    }
    else
    {
        ERR_LOG("[ArsonClubGameReportProcessor].processArsonGameHistoryClub(): Failed to retieve clubs.");
        success = false;
    }

    return success;
}

/////////////////////////////////////////////////////////////////////////////////

bool ArsonClubGameReportProcessor::processArsonGameHistoryClub(const GameType& gameType, ProcessedGameReport &processedReport, GameHistoryClubs_NonDerived::Report& report, Stats::UpdateStatsRequestBuilder& builder, GameManager::PlayerIdList& playerIds)
{
    GameHistoryClubs_NonDerived::Report& arsonClubReport = report;

    // iterate through clubs to find the best and worst scores.
    int32_t lowestClubScore = INT32_MAX;
    int32_t highestClubScore = INT32_MIN;

    GameHistoryClubs_NonDerived::Report::ClubReportsMap::const_iterator clubIter = arsonClubReport.getClubReports().begin();
    GameHistoryClubs_NonDerived::Report::ClubReportsMap::const_iterator clubEnd = arsonClubReport.getClubReports().end();

    Clubs::ClubId winningClubId = static_cast<Clubs::ClubId>(clubIter->first);
    Clubs::ClubIdList clubIdList; 

    for (; clubIter != clubEnd; ++clubIter)
    {
        GameHistoryClubs_NonDerived::ClubReport& clubReport = *clubIter->second;

        GameHistoryClubs_NonDerived::Report::PlayerReportsMap::const_iterator playerIter = arsonClubReport.getPlayerReports().begin();
        GameHistoryClubs_NonDerived::Report::PlayerReportsMap::const_iterator playerEnd = arsonClubReport.getPlayerReports().end();

        int32_t clubScore = 0;

        for (; playerIter != playerEnd; ++playerIter)
        {
            GameHistoryClubs_NonDerived::PlayerReport& playerReport = *playerIter->second;
            if (playerReport.getClubId() == clubIter->first)
                clubScore += playerReport.getPoints();
        }

        clubReport.setPoints((uint16_t)clubScore);

        if (clubScore > highestClubScore)
        {
            // update the winning club id and the highest club score
            winningClubId = static_cast<Clubs::ClubId>(clubIter->first);
            highestClubScore = clubScore;
        }
        if (clubScore < lowestClubScore)
        {
            // update the lowest club score
            lowestClubScore = clubScore;
        }

        clubIdList.push_back(clubIter->first);
    }

    bool tieGame = (arsonClubReport.getClubReports().size() > 1) && (highestClubScore == lowestClubScore);

    // set clubs and player stats
    UpdateClubsUtil updateClubsUtil;
    bool success = true;

    // update club stats
    if (updateClubsUtil.initialize(clubIdList))
    {
        clubIter = arsonClubReport.getClubReports().begin();
        clubEnd = arsonClubReport.getClubReports().end();

        for (; clubIter != clubEnd; ++clubIter)
        {
            Clubs::ClubId clubId = static_cast<Clubs::ClubId>(clubIter->first);
            GameHistoryClubs_NonDerived::ClubReport& clubReport = *clubIter->second; 

            if (tieGame)
            {
                clubReport.setWins(0);
                clubReport.setLosses(0);
                clubReport.setTies(1);    
            }
            else if (clubId == winningClubId)
            {
                clubReport.setWins(1);
                clubReport.setLosses(0);
                clubReport.setTies(0);     
            }
            else
            {
                clubReport.setWins(0);
                clubReport.setLosses(1);
                clubReport.setTies(0);
            }
        }

        // update clubs component on result of game
        if (success)
        {
            success = updateClubsUtil.updateClubs(
                arsonClubReport.getClubReports().begin()->first,
                arsonClubReport.getClubReports().begin()->second->getPoints(),
                (arsonClubReport.getClubReports().begin()+1)->first,
                (arsonClubReport.getClubReports().begin()+1)->second->getPoints());
        }

        updateClubsUtil.completeTransaction(success);
    }

    return success;
}
#endif

}   // namespace GameReporting
}   // namespace Blaze
