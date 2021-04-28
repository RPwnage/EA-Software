/*************************************************************************************************/
/*!
    \file   gameclubintrah2hreportslave.cpp

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

#include "gameclubintrah2hreportslave.h"
#include "gamecommonlobbyskill.h"

// seasonal play includes
#include "osdkseasonalplay/osdkseasonalplayslaveimpl.h"

namespace Blaze
{

namespace GameReporting
{

const char8_t* GameClubIntraH2HReportSlave::AGGREGATE_SCORE = "SCORE";

const char8_t* CATEGORYNAME_INTRACLUBH2H = "ClubIntraH2HStats";
const char8_t* ATTRIBNAME_INTRACLUBCLUBID = "OSDK_clubId";

/*************************************************************************************************/
/*! \brief GameClubIntraH2HReportSlave
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameClubIntraH2HReportSlave::create(GameReportingSlaveImpl& component)
{
    // GameClubIntraH2HReportSlave should not be instantiated
    return NULL;
}

/*************************************************************************************************/
/*! \brief GameClubIntraH2HReportSlave
    Constructor
*/
/*************************************************************************************************/
GameClubIntraH2HReportSlave::GameClubIntraH2HReportSlave(GameReportingSlaveImpl& component) :
    GameCommonReportSlave(component),
    mReportParser(NULL)
{
}

/*************************************************************************************************/
/*! \brief GameClubReportSlave
    Destructor
*/
/*************************************************************************************************/
GameClubIntraH2HReportSlave::~GameClubIntraH2HReportSlave()
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
BlazeRpcError GameClubIntraH2HReportSlave::validate(GameReport& report) const
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
BlazeRpcError GameClubIntraH2HReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* INTRACLUBH2H_GAME_TYPE_NAME = getCustomClubIntraH2HGameTypeName();

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), INTRACLUBH2H_GAME_TYPE_NAME))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[GameClubIntraH2HReportSlave::" << this << "].process(): Error parsing values");

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
            WARN_LOG("[GameClubIntraH2HReportSlave::" << this << "].process(): Error parsing keyscopes");

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
            WARN_LOG("[GameClubIntraH2HReportSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
            transformStats(CATEGORYNAME_INTRACLUBH2H);

            TRACE_LOG("[GameClubIntraH2HReportSlave:" << this << "].process() - commit stats");
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

		//Stats were already committed and can be evaluated during that process. Fetching stats again
		//to ensure mUpdateStatsHelper cache is valid (and avoid crashing with assert)
		processErr = (DB_ERR_OK == mUpdateStatsHelper.fetchStats()) ? ERR_OK : ERR_SYSTEM;
		if(ERR_OK == processErr)
		{
			// post game reporting processing()
			processErr = (true == processUpdatedStats()) ? Blaze::ERR_OK : Blaze::ERR_SYSTEM;
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
void GameClubIntraH2HReportSlave::reconfigure() const
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
bool GameClubIntraH2HReportSlave::skipProcessConclusionType()
{
    return skipProcessConclusionTypeCommon();
}

/*************************************************************************************************/
/*! \brief getGameModeGameHistoryQuery
    Get the game history query name for the game mode

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::getGameModeGameHistoryQuery(char8_t *query, int32_t querySize)
{
    if (NULL != query)
    {
        blaze_snzprintf(query, querySize, "%s%s", mProcessedReport->getGameType().getGameReportName().c_str(), "_skill_damping_query");
        TRACE_LOG("[GameClubIntraH2HReportSlave:" << this << "].getGameModeGameHistoryQuery() query:" << query << ", querySize:" << querySize);
    }
}

/*************************************************************************************************/
/*! \brief determineGameResult
    Determine the game results

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::determineGameResult()
{
    determineWinPlayer();
}

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
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
void GameClubIntraH2HReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateWinPlayerStats(playerId, playerReport);

    if (true == mProcessedReport->didAllPlayersFinish())
    {
        playerReport.setWinnerByDnf(0);
    }
    else
    {
        playerReport.setWinnerByDnf(1);
    }

    mBuilder.incrementStat(STATNAME_WSTREAK, 1);
    mBuilder.assignStat(STATNAME_LSTREAK, static_cast<int64_t>(0));
}

/*************************************************************************************************/
/*! \brief updateGameModeLossPlayerStats
    Update loser's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateLossPlayerStats(playerId, playerReport);
    playerReport.setWinnerByDnf(0);

    mBuilder.assignStat(STATNAME_WSTREAK, static_cast<int64_t>(0));
    mBuilder.incrementStat(STATNAME_LSTREAK, 1);
}

/*************************************************************************************************/
/*! \brief updateGameModeTiePlayerStats
    Update tied player's stats

    \Customizable - Virtual function.
 */
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateTiePlayerStats(playerId, playerReport);
    playerReport.setWinnerByDnf(0);
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Set the value of player keyscopes defined in config and needed for config based stat update
 */
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::updatePlayerKeyscopes()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    updateCustomPlayerKeyscopes(OsdkPlayerReportsMap);
}

/*************************************************************************************************/
/*! \brief sendEndGameMail
    Send end of game email/sms
*/
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::sendEndGameMail(GameManager::PlayerIdList& playerIds)
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

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::updatePlayerStats()
{
    TRACE_LOG("[GameClubIntraH2HReportSlave:" << this << "].updatePlayerStats()");

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
        Stats::ScopeNameValueMap indexMap;

        // Parse out the keyscope (ATTRIBNAME_INTRACLUBCLUBID) using the report parser
        GameType::StatUpdate statUpdate;
        GameTypeReportConfig& playerConfig = *mProcessedReport->getGameType().getConfig().getSubreports().find("playerReports")->second;
        GameTypeReportConfig& customPlayerConfig = *playerConfig.getSubreports().find("customPlayerReport")->second;
        mProcessedReport->getGameType().getStatUpdate(customPlayerConfig, "PlayerStats", statUpdate);
        mReportParser->generateScopeIndexMap(indexMap, playerId, statUpdate);

        mBuilder.startStatRow(CATEGORYNAME_INTRACLUBH2H, playerId, &indexMap);

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
/*! \brief processUpdatedStats
    Perform post stats update processing

    \return - true if the post-game processing is performed successfully

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
bool GameClubIntraH2HReportSlave::processUpdatedStats()
{
    bool bSuccess = processCustomUpdatedStats();
    updateNotificationReport();
    return bSuccess;
}

/*************************************************************************************************/
/*! \brief transformStats
    calculate lobby skill, lobby skill points and opponent skill level

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::transformStats(const char8_t* statsCategory)
{
    if (DB_ERR_OK == mUpdateStatsHelper.fetchStats() && NULL != statsCategory)
    {
        TRACE_LOG("[GameClubIntraH2HReportSlave:" << this << "].transformStats() for category: " << statsCategory);

        GameCommonLobbySkill lobbySkill;

        OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
        OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

        OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
        playerIter = OsdkPlayerReportsMap.begin();
        playerEnd = OsdkPlayerReportsMap.end();

        for(; playerIter != playerEnd; ++playerIter)
        {
            GameManager::PlayerId playerId = playerIter->first;
            OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

            Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(statsCategory, playerId);
            if (NULL != key)
            {
                int32_t dampingPercent = static_cast<int32_t>(calcPlayerDampingPercent(playerId, playerReport)); /* set percentage to use if pro-rating stats */
                int32_t weight = 15;       /* set dummy player 'weight' used to determine iAdj factor */
                int32_t skill = 45;        /* set player 'skill' */

                int32_t wlt = static_cast<int32_t>(playerReport.getGameResult());
                int32_t teamId = static_cast<int32_t>(playerReport.getTeam());
                bool home = playerReport.getHome();

                lobbySkill.addLobbySkillInfo(playerId, wlt, dampingPercent, teamId, weight, home, skill);

                // Loop through all the supported stats period type
                for (uint32_t periodType = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodType < Stats::STAT_NUM_PERIODS; periodType++)
                {
                    int32_t currentSkillPoints = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_OVERALLPTS, static_cast<Stats::StatPeriodType>(periodType), true));
                    lobbySkill.addCurrentSkillPoints(playerId, currentSkillPoints, static_cast<Stats::StatPeriodType>(periodType));

                    int32_t currentSkillLevel = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_SKILL, static_cast<Stats::StatPeriodType>(periodType), true));
                    lobbySkill.addCurrentSkillLevel(playerId, currentSkillLevel, static_cast<Stats::StatPeriodType>(periodType));

                    int32_t currentOppSkillLevel = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_OPPO_LEVEL, static_cast<Stats::StatPeriodType>(periodType), true));
                    lobbySkill.addCurrentOppSkillLevel(playerId, currentOppSkillLevel, static_cast<Stats::StatPeriodType>(periodType));
                }
            }
        }

        playerIter = OsdkPlayerReportsMap.begin();
        for(; playerIter != playerEnd; ++playerIter)
        {
            GameManager::PlayerId playerId = playerIter->first;
            
            Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(statsCategory, playerId);
            if (NULL != key)
            {
                // Loop through all the supported stats period type
                for (uint32_t periodType = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodType < Stats::STAT_NUM_PERIODS; periodType++)
                {
                    int32_t newSkillPoints = lobbySkill.getNewLobbySkillPoints(playerId, static_cast<Stats::StatPeriodType>(periodType));
                    mUpdateStatsHelper.setValueInt(key, STATNAME_OVERALLPTS, static_cast<Stats::StatPeriodType>(periodType), newSkillPoints);

                    int32_t newSkillLevel = lobbySkill.getNewLobbySkillLevel(playerId, mReportConfig, newSkillPoints);
                    mUpdateStatsHelper.setValueInt(key, STATNAME_SKILL, static_cast<Stats::StatPeriodType>(periodType), newSkillLevel);

                    int32_t newOppSkillLevel = lobbySkill.getNewLobbyOppSkillLevel(playerId, static_cast<Stats::StatPeriodType>(periodType));
                    mUpdateStatsHelper.setValueInt(key, STATNAME_OPPO_LEVEL, static_cast<Stats::StatPeriodType>(periodType), newOppSkillLevel);

                    TRACE_LOG("[GameClubIntraH2HReportSlave:" << this << "].transformStats() for playerId: " << playerId << ", newSkillPoints:"
                              << newSkillPoints << ", newSkillLevel:" << newSkillLevel << ", newOppSkillLevel:" << newOppSkillLevel << ", periodType=" << periodType);
                }
            }
        }
    }
}

/*************************************************************************************************/
/*! \brief aggregatePlayerStats
    Calculate the aggregated value of player stats and store them in a map

    \Customizable - None.
*/
/*************************************************************************************************/
void GameClubIntraH2HReportSlave::aggregatePlayerStats()
{
    TRACE_LOG("[GameClubIntraH2HReportSlave:" << this << "].aggregatePlayerStats()");

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

}   // namespace GameReporting

}   // namespace Blaze
