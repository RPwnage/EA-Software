/*************************************************************************************************/
/*!
    \file   arsonbasicgamereportprocessor.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"

#include "arsonbasicgamereportprocessor.h"
#include "glickoskill.h"
#include "lobbyskill.h"
#include "gamereporting/gamereportingslaveimpl.h"

#include "gamereporting/util/gamehistoryutil.h"
#include "gamereporting/util/reportparserutil.h"
#include "gamereporting/util/skilldampingutil.h"
#include "gamereporting/util/achievementsutil.h"
#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

#include "framework/usersessions/usersessionmanager.h"

namespace Blaze
{
namespace GameReporting
{

GameReportProcessor* ArsonBasicGameReportProcessor::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("ArsonBasicGameReportProcessor") ArsonBasicGameReportProcessor(component);
}

ArsonBasicGameReportProcessor::ArsonBasicGameReportProcessor(GameReportingSlaveImpl& component)
    : GameReportProcessor(component)
{
}

ArsonBasicGameReportProcessor::~ArsonBasicGameReportProcessor()
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
BlazeRpcError ArsonBasicGameReportProcessor::validate(GameReport& report) const
{
    return ERR_OK;
}

/*! ****************************************************************************/
/*! \brief Deterimines a player's score.
********************************************************************************/

#ifdef TARGET_arson
int32_t ArsonBasicGameReportProcessor::getScore(const GameHistoryBasic::PlayerReport& playerReport, bool didPlayerFinish)
{
    if (!didPlayerFinish)
        return INT32_MIN;

    return (int32_t)(playerReport.getKills() - playerReport.getDeaths() + playerReport.getMoney());
}

int32_t ArsonBasicGameReportProcessor::getScore(const ArsonCTF_Custom::PlayerReport& playerReport, bool didPlayerFinish)
{
    if (!didPlayerFinish)
        return INT32_MIN;

    return (int32_t)(playerReport.getKills() - playerReport.getDeaths() + playerReport.getFlagsCaptured());
}

int32_t ArsonBasicGameReportProcessor::getWLT(const GameHistoryBasic::PlayerReport& playerReport)
{
    int32_t resultValue;

    if (playerReport.getWinner())
        resultValue = WLT_WIN;
    else if (playerReport.getLoser())
        resultValue = WLT_LOSS;
    else
        resultValue = WLT_TIE;

    return resultValue;
}

uint32_t ArsonBasicGameReportProcessor::calcPlayerDampingPercent(GameManager::PlayerId playerId, bool isWinner, bool didAllPlayersFinish,
                                                             const GameHistoryBasic::Report::PlayerReportsMap &playerReportMap)
{
    //set damping to 100% as default case
    uint32_t dampingPercent = 100;

    GameHistoryUtil gameHistoryUtil(mComponent);
    QueryVarValuesList queryVarValues;
    GameReportsList gameReportsList;
    GameReportsList winningGameReportsList;

    char8_t strPlayerId[32];
    blaze_snzprintf(strPlayerId, sizeof(strPlayerId), "%" PRId64, playerId);
    queryVarValues.push_back(strPlayerId);

    if (gameHistoryUtil.getGameHistory("recent_gamehistorybasic_games_by_player", 5, queryVarValues, gameReportsList))
    {
        if (isWinner)
        {
            uint32_t rematchWins = 0;
            uint32_t dnfWins = 0;
            Collections::AttributeMap map;
            map["player_id"] = strPlayerId;
            map["winner"] = "1";

            gameHistoryUtil.getGameHistoryMatchingValues("player", map, gameReportsList, winningGameReportsList);
            if (winningGameReportsList.getGameReportList().size() > 0)
            {
                GameHistoryBasic::Report::PlayerReportsMap::const_iterator playerIter = playerReportMap.begin();
                GameHistoryBasic::Report::PlayerReportsMap::const_iterator playerEnd = playerReportMap.end();

                for (; playerIter != playerEnd; ++playerIter)
                {
                    if (playerIter->second->getWinner())
                    {
                        continue;
                    }

                    GameReportsList rematchedWinningReportsList;
                    Collections::AttributeMap oppoSearchMap;
                    char8_t strOppoPlayerId[32];
                    blaze_snzprintf(strOppoPlayerId, sizeof(strOppoPlayerId), "%" PRId64, playerIter->first);
                    map["player_id"] = strOppoPlayerId;
                    map["loser"] = "1";

                    //we keep the count of rematch wins for the most rematched player, alternately, the rematch wins could be summed
                    gameHistoryUtil.getGameHistoryConsecutiveMatchingValues("player", oppoSearchMap, gameReportsList, rematchedWinningReportsList);
                    uint32_t newRematchWins = rematchedWinningReportsList.getGameReportList().size();
                    if (newRematchWins > rematchWins)
                    {
                        rematchWins = newRematchWins;
                    }
                    rematchedWinningReportsList.getGameReportList().release();
                }

                // only check this if the current win is via DNF
                if (!didAllPlayersFinish)
                {
                    TRACE_LOG("[ArsonBasicGameReportProcessor].calcPlayerDampingPercent(): The current win is via DNF");
                    GameReportsList winnerByDnfReportsList;
                    Collections::AttributeMap winnerByDnfSearchMap;
                    winnerByDnfSearchMap["player_id"] = strPlayerId;
                    winnerByDnfSearchMap["winnerByDnf"] = "1";

                    // could use winningGameReportsList from above if the desire was to count consecutive wins by dnf in the player's last "x" winning games, rather than all consecutive games
                    gameHistoryUtil.getGameHistoryConsecutiveMatchingValues("player", winnerByDnfSearchMap, gameReportsList, winnerByDnfReportsList);
                    dnfWins = winnerByDnfReportsList.getGameReportList().size();
                    winnerByDnfReportsList.getGameReportList().release();
                }

                SkillDampingUtil skillDampingUtil(mComponent);
                uint32_t rematchDampingPercent = skillDampingUtil.lookupDampingPercent(rematchWins, "rematchDamping");
                uint32_t dnfWinDampingPercent = skillDampingUtil.lookupDampingPercent(dnfWins, "dnfDamping");

                dampingPercent = (rematchDampingPercent * dnfWinDampingPercent) / 100;
            }
        }
    }

    gameReportsList.getGameReportList().release();
    winningGameReportsList.getGameReportList().release();

    TRACE_LOG("[ArsonBasicGameReportProcessor].calcPlayerDampingPercent(): Skill damping value " << dampingPercent << " applied to user " << playerId);

    return dampingPercent;
}
#endif

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success.  GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError ArsonBasicGameReportProcessor::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();
   
    const char8_t* BASIC_GAME_TYPE = "gameHistoryBasic";
    const char8_t* BASIC_GAME_TYPE2 = "gameHistoryBasic2";
    const char8_t* CTF_ND_GAME_TYPE = "GR_ArsonCTF_Custom";

    if (blaze_strcmp(gameType.getGameReportName().c_str(), BASIC_GAME_TYPE) != 0 &&
        blaze_strcmp(gameType.getGameReportName().c_str(), BASIC_GAME_TYPE2) != 0 &&
        blaze_strcmp(gameType.getGameReportName().c_str(), CTF_ND_GAME_TYPE) != 0)
    {
        ERR_LOG("[ArsonBasicGameReportProcessor].process() : Reporting must be of type '" << BASIC_GAME_TYPE << "' or '" << BASIC_GAME_TYPE2 << "' or '" << CTF_ND_GAME_TYPE 
                << "' but is of type '" << gameType.getGameReportName().c_str() << "'.  Check gameType configuration inside gamereporting.cfg to verify this custom processor can handle the game type.");
        return GAMEREPORTING_ERR_INVALID_GAME_TYPE;
    }

    //  set stats for wins, losses
    Stats::UpdateStatsRequestBuilder builder;

    //  set up report parser to parse out keyscopes first so we can use the stats utility to set wins and losses for a player.
    Utilities::ReportParser reportParser(gameType,  processedReport, &playerIds);
    reportParser.setUpdateStatsRequestBuilder(&builder);

    reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES);

#ifdef TARGET_arson
    const CollatedGameReport::DnfStatusMap &dnfStatus = processedReport.getDnfStatusMap();   
    
    if ((blaze_strcmp(gameType.getGameReportName().c_str(), BASIC_GAME_TYPE) == 0) || (blaze_strcmp(gameType.getGameReportName().c_str(), BASIC_GAME_TYPE2) == 0))
    {
        //  determine winner, loser and winnerByDnf based on the final report.
        GameHistoryBasic::Report& basicReport = static_cast<GameHistoryBasic::Report&>(*report.getReport());

        if (basicReport.getGameAttrs().getMapId() == 3)
        {
            // to override the flags and flag reason value in the processed game report.
            processedReport.getReportFlagInfo()->getFlag().setAbuse();
            processedReport.getReportFlagInfo()->setFlagReason("Just test");
        }

        // iterate through players to find the best and worst scores.
        int32_t lowestPlayerScore = INT32_MAX;
        int32_t highestPlayerScore = INT32_MIN;
        GameHistoryBasic::Report::PlayerReportsMap::const_iterator playerIter = basicReport.getPlayerReports().begin();
        GameHistoryBasic::Report::PlayerReportsMap::const_iterator playerEnd = basicReport.getPlayerReports().end();

        for (; playerIter != playerEnd; ++playerIter)
        {
            GameHistoryBasic::PlayerReport& playerReport = *playerIter->second;
            bool playerFinished = (processedReport.getReportType() == REPORT_TYPE_OFFLINE) ? true : (dnfStatus.find(playerIter->first)->second == 0);
            int32_t playerScore = getScore(playerReport, playerFinished);
            if (playerScore > highestPlayerScore)
            {
                // update the highest score
                highestPlayerScore = playerScore;
            }
            if (playerScore < lowestPlayerScore)
            {
                // update the lowest player score
                lowestPlayerScore = playerScore;
            }
        }

        //  determine winners and losers.
        bool tieGame = (basicReport.getPlayerReports().size() > 1) && (highestPlayerScore == lowestPlayerScore);
        playerIter = basicReport.getPlayerReports().begin();
        for (; playerIter != playerEnd; ++playerIter)
        {
            GameManager::PlayerId playerId = playerIter->first;
            GameHistoryBasic::PlayerReport& playerReport = *playerIter->second;
            bool playerFinished = (processedReport.getReportType() == REPORT_TYPE_OFFLINE) ? true : (dnfStatus.find(playerIter->first)->second == 0);
            int32_t playerScore = getScore(playerReport, playerFinished);

            TRACE_LOG("[ArsonBasicGameReportProcessor].process() : Report id=" << report.getGameReportingId() << ", player " << playerIter->first 
                      << " score=" << playerScore << ", dnf=" << (playerFinished ? 0 : 1));

            if (tieGame)
            {
                playerReport.setLoser(0);
                playerReport.setWinner(0);
                playerReport.setWinnerByDNF(0);
            }
            else
            {
                bool winner = (basicReport.getPlayerReports().size() == 1 && playerScore > 0) ||
                    (basicReport.getPlayerReports().size() > 1 && playerScore == highestPlayerScore);
                playerReport.setLoser(!winner);
                playerReport.setWinner(winner);
                playerReport.setWinnerByDNF(winner && !processedReport.didAllPlayersFinish());
            }

            //  set winner/loser/dnf stats!
            Stats::ScopeNameValueMap scopes;
            //  REPLACE arguments to getStatUpdate() match the config and StatViewName to use for the keyscope lookup.
            GameType::StatUpdate statUpdate;
            GameTypeReportConfig& playerConfig = *gameType.getConfig().getSubreports().find("playerReports")->second;
            gameType.getStatUpdate(playerConfig, "PlayerStats", statUpdate);
            reportParser.generateScopeIndexMap(scopes, playerId, statUpdate);

            if (!UserSessionManager::isStatelessUser(playerId))
            {
                if (processedReport.getReportType() == REPORT_TYPE_OFFLINE)
                {

                    builder.startStatRow("Sample_Offline", playerId, &scopes);
                }
                else
                {   

                    builder.startStatRow("Sample", playerId, &scopes);
                    builder.incrementStat("totalGamesPlayed", 1);
                    if (!playerFinished)
                        builder.incrementStat("totalGamesNotFinished", 1);
                }
                if (playerReport.getWinner())
                {
                    builder.incrementStat("wins", 1);
                }
                else
                {
                    builder.incrementStat("losses", 1);
                }

                builder.completeStatRow();
            }
        }
    }
    else if (blaze_strcmp(gameType.getGameReportName().c_str(), CTF_ND_GAME_TYPE) == 0)
    {
        ArsonCTF_Custom::Report& arsonCtfReport = static_cast<ArsonCTF_Custom::Report&>(*report.getReport());

        ArsonCTF_Custom::ResultNotification *notification = static_cast<ArsonCTF_Custom::ResultNotification*>(processedReport.getCustomData());
        
        GameManager::GameId gameId = processedReport.getGameId();
        notification->setGameId(gameId);
        
        // iterate through players to find the winner and players' scores.
        int32_t lowestPlayerScore = INT32_MAX;
        int32_t highestPlayerScore = INT32_MIN;    
        ArsonCTF_Custom::Report::PlayerReportsMap::const_iterator playerIter = arsonCtfReport.getPlayerReports().begin();
        ArsonCTF_Custom::Report::PlayerReportsMap::const_iterator playerEnd = arsonCtfReport.getPlayerReports().end();

        for (; playerIter != playerEnd; ++playerIter)
        {
            ArsonCTF_Custom::PlayerReport& playerReport = *playerIter->second;
            bool playerFinished = (processedReport.getReportType() == REPORT_TYPE_OFFLINE) ? true : (dnfStatus.find(playerIter->first)->second == 0);
            int32_t playerScore = getScore(playerReport, playerFinished);
            if (playerScore > highestPlayerScore)
            {
                // update the highest score
                highestPlayerScore = playerScore;
            }
            if (playerScore < lowestPlayerScore)
            {
                // update the lowest player score
                lowestPlayerScore = playerScore;
            }
        }

        //  determine winners and losers.
        playerIter = arsonCtfReport.getPlayerReports().begin();
        for (; playerIter != playerEnd; ++playerIter)
        {            
            ArsonCTF_Custom::PlayerReport& playerReport = *playerIter->second;
            bool playerFinished = (processedReport.getReportType() == REPORT_TYPE_OFFLINE) ? true : (dnfStatus.find(playerIter->first)->second == 0);
            int32_t playerScore = getScore(playerReport, playerFinished);

            TRACE_LOG("[ArsonBasicGameReportProcessor].process() : Report id=" << report.getGameReportingId() << ", player " 
                      << playerIter->first << " score=" << playerScore << ", dnf=" << (playerFinished ? 0 : 1));

            notification->getPlayerScores().insert(eastl::make_pair(playerIter->first, playerScore));

            bool winner = (arsonCtfReport.getPlayerReports().size() == 1 && playerScore > 0) ||
                (arsonCtfReport.getPlayerReports().size() > 1 && playerScore == highestPlayerScore);
            if (winner)
            {
                notification->getWinners().push_back(playerIter->first);
            }
        }
    }


    // set all other stats from the report.   
    bool success = reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS);

    if (success)
    {
        BlazeRpcError processErr = ERR_OK;

        if (!builder.empty())
        {
            //  publish stats.
            Stats::UpdateStatsHelper updateStatsHelper;

            processErr = updateStatsHelper.initializeStatUpdate(builder);
            if (processErr == ERR_OK)
            {
                if ((blaze_strcmp(gameType.getGameReportName().c_str(), BASIC_GAME_TYPE) == 0) || (blaze_strcmp(gameType.getGameReportName().c_str(), BASIC_GAME_TYPE2) == 0))
                {
                    if (updateStatsHelper.fetchStats() == DB_ERR_OK)
                    {
                        GlickoSkill glickoSkill(mComponent.getMaxSkillValue());
                        LobbySkill lobbySkill;

                        GameHistoryBasic::Report& basicReport = static_cast<GameHistoryBasic::Report&>(*report.getReport());
                        GameHistoryBasic::Report::PlayerReportsMap::const_iterator playerIter = basicReport.getPlayerReports().begin();
                        GameHistoryBasic::Report::PlayerReportsMap::const_iterator playerEnd = basicReport.getPlayerReports().end();

                        for (; playerIter != playerEnd; ++playerIter)
                        {
                            GameManager::PlayerId playerId = playerIter->first;
                            GameHistoryBasic::PlayerReport& playerReport = *playerIter->second;
                            int32_t wlt = getWLT(playerReport);
                            int32_t dampingPercent = calcPlayerDampingPercent(playerId, playerReport.getWinner(),
                                processedReport.didAllPlayersFinish(), basicReport.getPlayerReports());
                            int32_t teamId = static_cast<int32_t>(playerId);
                            int32_t periodsPassed = 1;
                            int32_t weight = 15;
                            int32_t skillLevel = 45;
                            bool home = false;

                            Stats::UpdateRowKey* key = builder.getUpdateRowKey("Sample", playerId);
                            if (key != nullptr)
                            {
                                int32_t currentGlickoSkill = (int32_t)updateStatsHelper.getValueInt(key, "glickoSkill", Stats::STAT_PERIOD_ALL_TIME, true);
                                int32_t currentGlickoRd = (int32_t)updateStatsHelper.getValueInt(key, "glickoRd", Stats::STAT_PERIOD_ALL_TIME, true);
                                glickoSkill.addGlickoSkillInfo(playerId, wlt, dampingPercent, teamId, periodsPassed, currentGlickoSkill, currentGlickoRd);

                                int32_t currentLobbySkill = (int32_t)updateStatsHelper.getValueInt(key, "skill", Stats::STAT_PERIOD_ALL_TIME, true);
                                lobbySkill.addLobbySkillInfo(playerId, wlt, dampingPercent, 0, weight, home, skillLevel, currentLobbySkill);
                            }
                        }

                        playerIter = basicReport.getPlayerReports().begin();

                        for (; playerIter != playerEnd; ++playerIter)
                        {
                            GameManager::PlayerId playerId = playerIter->first;

                            Stats::UpdateRowKey* key = builder.getUpdateRowKey("Sample", playerId);
                            if (key != nullptr)
                            {
                                int32_t newGlickoSkill = (int32_t)updateStatsHelper.getValueInt(key, "glickoSkill", Stats::STAT_PERIOD_ALL_TIME, true);
                                int32_t newGlickoRd = (int32_t)updateStatsHelper.getValueInt(key, "glickoRd", Stats::STAT_PERIOD_ALL_TIME, true);
                                glickoSkill.getNewGlickoSkillAndRd(playerId, newGlickoSkill, newGlickoRd);
                                updateStatsHelper.setValueInt(key, "glickoSkill", Stats::STAT_PERIOD_ALL_TIME, newGlickoSkill);
                                updateStatsHelper.setValueInt(key, "glickoRd", Stats::STAT_PERIOD_ALL_TIME, newGlickoRd);

                                int32_t newLobbySkill = lobbySkill.getNewLobbySkillPoints(playerId);
                                updateStatsHelper.setValueInt(key, "skill", Stats::STAT_PERIOD_ALL_TIME, newLobbySkill);                        
                            }
                        }
                    }
                }
                processErr = updateStatsHelper.commitStats();
            }


            //achievements update
            AchievementsUtil achievementUtil;
            reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_ACHIEVEMENTS);
            Utilities::ReportParser::GrantAchievementRequestList::const_iterator gaIter = reportParser.getGrantAchievementRequests().begin();
            for (; gaIter != reportParser.getGrantAchievementRequests().end(); ++gaIter)
            {
                Achievements::AchievementData result;
                achievementUtil.grantAchievement(**gaIter, result);
            }

            Utilities::ReportParser::PostEventsRequestList::const_iterator peIter = reportParser.getPostEventsRequests().begin();
            for (; peIter != reportParser.getPostEventsRequests().end(); ++peIter)
            {
                achievementUtil.postEvents(**peIter);
            }
        }

        //  extract game history attributes from report.
        GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
        reportParser.setGameHistoryReport(&historyReport);
        reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);

        processedReport.enableHistorySave(processedReport.getReportType() != REPORT_TYPE_TRUSTED_MID_GAME);

        return processErr;
    }  
#endif
    return reportParser.getErrorCode();
}

}   // namespace GameReporting
}   // namespace Blaze
