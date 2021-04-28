/*************************************************************************************************/
/*!
    \file   gameotpreportslave.cpp

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

#include "gameotp.h"
#include "gameotpreportslave.h"
#include "gamecommonlobbyskill.h"
#include "ping/tdf/gameotpreport.h"

#include "useractivitytracker/stats_updater.h"


namespace Blaze
{

namespace GameReporting
{

const char8_t* GameOTPReportSlave::AGGREGATE_SCORE = "SCORE";

/*************************************************************************************************/
/*! \brief GameOTPReportSlave
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameOTPReportSlave::create(GameReportingSlaveImpl& component)
{
    // GameOTPReportSlave should not be instantiated
    return NULL;
}

/*************************************************************************************************/
/*! \brief GameOTPReportSlave
    Constructor
*/
/*************************************************************************************************/
GameOTPReportSlave::GameOTPReportSlave(GameReportingSlaveImpl& component) :
    GameCommonReportSlave(component),
    mReportParser(NULL)
{
}

/*************************************************************************************************/
/*! \brief GameOTPReportSlave
    Destructor
*/
/*************************************************************************************************/
GameOTPReportSlave::~GameOTPReportSlave()
{
    PlayerScopeIndexMap::const_iterator scopeIter = mFreeAgentPlayerKeyscopes.begin();
    PlayerScopeIndexMap::const_iterator scopeIterEnd = mFreeAgentPlayerKeyscopes.end();
    for (; scopeIter != scopeIterEnd; scopeIter++)
    {
        delete(scopeIter->second);
    }
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
BlazeRpcError GameOTPReportSlave::validate(GameReport& report) const
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
BlazeRpcError GameOTPReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    mProcessedReport = &processedReport;

    GameReport& report = processedReport.getGameReport();
    const GameType& gameType = processedReport.getGameType();

    BlazeRpcError processErr = Blaze::ERR_OK;

    const char8_t* OTP_GAME_TYPE_NAME = getCustomOTPGameTypeName();

    if (0 == blaze_strcmp(gameType.getGameReportName().c_str(), OTP_GAME_TYPE_NAME))
    {
        // create the parser
        mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
        mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

        // fills in report with values via configuration
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
        {
            WARN_LOG("[GameOTPReportSlave::" << this << "].process(): Error parsing values");

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
            WARN_LOG("[GameOTPReportSlave::" << this << "].process(): Error parsing keyscopes");

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

		selectCustomStats();

		UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
		statsUpdater.updateGamesPlayed(playerIds);
		statsUpdater.selectStats();

        // extract and set stats
        if (false == mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
        {
            WARN_LOG("[GameOTPReportSlave::" << this << "].process(): Error parsing stats");

            processErr = mReportParser->getErrorCode();
            delete mReportParser;
            return processErr; // EARLY RETURN
        }

        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();		
        processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);


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
                TRACE_LOG("[GameOTPReportSlave:" << this << "].process() - processUpdatedStats() returned false. Failing with ERR_SYSTEM.");
            }
            else 
            {
				// Do the custom transform for accomplishments
				if (DB_ERR_OK == mUpdateStatsHelper.fetchStats())
				{
					updateCustomTransformStats();
					statsUpdater.transformStats();
				}

				if (processErr == ERR_OK)
				{
					processErr = mUpdateStatsHelper.calcDerivedStats();
					TRACE_LOG("[GameOTPReportSlave:" << this << "].process() - calcDerivedStats() returned: " << processErr);
				}

                processErr = mUpdateStatsHelper.commitStats();  
                TRACE_LOG("[GameOTPReportSlave:" << this << "].process() - commitStats() returned: " << processErr);
            }            
        }

		if (false == processPostStatsCommit())
		{
			processErr = ERR_SYSTEM;
		}

        //send end game mail
        sendEndGameMail(playerIds);

		// Customize GameReports for PIN Event Reporting
		CustomizeGameReportForEventReporting(processedReport, playerIds);
    }

    return processErr;
}
 
/******************************************************************************/
/*! \brief Triggered on server reconfiguration.

    \Customizable - Virtual function.
********************************************************************************/
void GameOTPReportSlave::reconfigure() const
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
bool GameOTPReportSlave::skipProcessConclusionType()
{
    // Conclusion type is only enabled for H2H game mode
    return false;
}

/*! ****************************************************************************/
/*! \brief initGameParams
    Initialize game parameters that are needed for later processing

    \Customizable - Virtual function to override default behavior.
    \Customizable - initCustomGameParams() to provide additional custom behavior.
********************************************************************************/
void GameOTPReportSlave::initGameParams()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    mCategoryId = OsdkReport.getGameReport().getCategoryId();
    mRoomId = OsdkReport.getGameReport().getRoomId();

    // could be a tie game if there is more than one player
    mTieGame = (mPlayerReportMapSize > 1);

    initCustomGameParams();
}

/*************************************************************************************************/
/*! \brief determineGameResult
    Determine the game results

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameOTPReportSlave::determineGameResult()
{
    determineWinPlayer();
}

/*************************************************************************************************/
/*! \brief updatePlayerKeyscopes
    Update Player Keyscopes

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerKeyscopes() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameOTPReportSlave::updatePlayerKeyscopes() 
{
   updateCustomPlayerKeyscopes(); 
};

/*************************************************************************************************/
/*! \brief updateCommonStats
    Update common stats regardless of the game result

    \Customizable - Virtual function to override default behavior.
    \Customizable - updateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameOTPReportSlave::updateCommonStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    playerReport.setPointsAgainst(mAggregatedPlayerStatsMap[AGGREGATE_SCORE] - playerReport.getScore());
    playerReport.setOpponentCount(mPlayerReportMapSize - 1);

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
void GameOTPReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
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
void GameOTPReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
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
void GameOTPReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateTiePlayerStats(playerId, playerReport);
    playerReport.setWinnerByDnf(0);
}

/*************************************************************************************************/
/*! \brief updatePlayerStats
    Update the players' stats based on game result

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameOTPReportSlave::updatePlayerStats()
{
    TRACE_LOG("[GameOTPReportSlave:" << this << "].updatePlayerStats()");

    // aggregate players' stats
    aggregatePlayerStats();

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    GameType::StatUpdate statUpdate;

    for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        Stats::ScopeNameValueMap indexMap;

        // Pull the player's keyscopes from the gamereporting configuration file.
        GameTypeReportConfig& playerConfig = *mProcessedReport->getGameType().getConfig().getSubreports().find("playerReports")->second;
        GameTypeReportConfig& customPlayerConfig = *playerConfig.getSubreports().find("customPlayerReport")->second;
        mProcessedReport->getGameType().getStatUpdate(customPlayerConfig, "PlayerStats", statUpdate);
        mReportParser->generateScopeIndexMap(indexMap, playerId, statUpdate);

        // Pull the player's keyscopes from the mPlayerKeyscopes map.
        PlayerScopeIndexMap::const_iterator scopeIter = mPlayerKeyscopes.find(playerId);
        if(scopeIter != mPlayerKeyscopes.end())
        {
            Stats::ScopeNameValueMap *scopeMap = scopeIter->second;
            Stats::ScopeNameValueMap::iterator it = scopeMap->begin();

            for(; it != scopeMap->end(); it++)
            {
                // do NOT use insert here - generateScopeIndexMap inserts the value already (with 0
                // as the value), and calling insert a second time will not update this value. 
                indexMap[it->first] = it->second; 
            }
        }

        mBuilder.startStatRow(getCustomOTPStatsCategoryName(), playerId, &indexMap);

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
void GameOTPReportSlave::updatePlayerStatsConclusionType()
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

        Stats::ScopeNameValueMap indexMap;

        uint32_t userResult = playerReport.getUserResult(); // ATTRIBNAME_USERRESULT
        if (userResult == GAME_RESULT_DNF_QUIT || userResult == GAME_RESULT_DNF_DISCONNECT)
        {
            mIsUpdatePlayerConclusionTypeStats = true;

            // Pull the player's keyscopes from the gamereporting configuration file.
            GameType::StatUpdate statUpdate;
            GameTypeReportConfig& playerConfig = *mProcessedReport->getGameType().getConfig().getSubreports().find("playerReports")->second;
            GameTypeReportConfig& customPlayerConfig = *playerConfig.getSubreports().find("customPlayerReport")->second;
            mProcessedReport->getGameType().getStatUpdate(customPlayerConfig, "PlayerStats", statUpdate);
            mReportParser->generateScopeIndexMap(indexMap, playerId, statUpdate);

            // Pull the player's keyscopes from the mPlayerKeyscopes map.
            PlayerScopeIndexMap::const_iterator scopeIter = mPlayerKeyscopes.find(playerId);
            if(scopeIter != mPlayerKeyscopes.end())
            {
                Stats::ScopeNameValueMap *scopeMap = scopeIter->second;
                Stats::ScopeNameValueMap::iterator it = scopeMap->begin();

                for(; it != scopeMap->end(); it++)
                {
                    indexMap[it->first] = it->second; 
                }
            }

            mBuilder.startStatRow(getCustomOTPStatsCategoryName(), playerId, &indexMap);

            mBuilder.incrementStat(STATNAME_TOTAL_GAMES_PLAYED, 1);
            mBuilder.incrementStat(STATNAME_GAMES_NOT_FINISHED, 1);
            mBuilder.incrementStat(STATNAME_LOSSES, 1);

            mBuilder.completeStatRow();
        }
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
bool GameOTPReportSlave::processUpdatedStats()
{
    bool success = true;
    success = success ? processCustomUpdatedStats() : success;
    return success;
}

/*************************************************************************************************/
/*! \brief sendEndGameMail
    Send end of game email/sms

    \Customizable - Virtual function to override default behavior.
    \Customizable - setCustomEndGameMailParam() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameOTPReportSlave::sendEndGameMail(GameManager::PlayerIdList& playerIds)
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
/*! \brief transformStats
    calculate lobby skill, lobby skill points and opponent skill level

    \Customizable - Virtual function to override default behavior.
    \Customizable - setCustomLobbySkillInfo() to provide additional custom behavior.
    \Customizable - updateCustomTransformStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
BlazeRpcError GameOTPReportSlave::transformStats(const char8_t* statsCategory)
{
	BlazeRpcError processErr = Blaze::ERR_OK;

    if (DB_ERR_OK == mUpdateStatsHelper.fetchStats())
    {
		if (NULL != statsCategory)
		{
			TRACE_LOG("[GameOTPReportSlave:" << this << "].transformStats() for category: " << statsCategory);

			GameCommonLobbySkill* lobbySkill = getGameCommonLobbySkill();
			if (NULL != lobbySkill)
			{
				OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
				OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

				OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
				playerIter = OsdkPlayerReportsMap.begin();
				playerEnd = OsdkPlayerReportsMap.end();

				for (; playerIter != playerEnd; ++playerIter)
				{
					GameManager::PlayerId playerId = playerIter->first;
					OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

					Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(statsCategory, playerId);
					if (NULL != key)
					{
						// Setting the default value for Lobby Skill Info
						int32_t dampingPercent = static_cast<int32_t>(calcPlayerDampingPercent(playerId, playerReport)); /* set percentage to use if pro-rating stats */
						int32_t weight = 15;       /* set dummy player 'weight' used to determine iAdj factor */
						int32_t skill = 45;        /* set player 'skill' */
						int32_t wlt = static_cast<int32_t>(playerReport.getGameResult());
						int32_t teamId = static_cast<int32_t>(playerReport.getTeam());
						bool home = playerReport.getHome();
						GameCommonLobbySkill::LobbySkillInfo skillInfo;
						skillInfo.setLobbySkillInfo(wlt, dampingPercent, teamId, weight, home, skill);

						// Allow game team to modify
						setCustomLobbySkillInfo(&skillInfo);

						lobbySkill->addLobbySkillInfo(playerId, skillInfo.getWLT(), skillInfo.getDampingPercent(), skillInfo.getTeamId(), skillInfo.getWeight(), skillInfo.getHome(), skillInfo.getSkill());

						// Loop through all the supported stats period type
						for (uint32_t periodType = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodType < Stats::STAT_NUM_PERIODS; periodType++)
						{
							int32_t currentSkillPoints = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_OVERALLPTS, static_cast<Stats::StatPeriodType>(periodType), true));
							lobbySkill->addCurrentSkillPoints(playerId, currentSkillPoints, static_cast<Stats::StatPeriodType>(periodType));

							int32_t currentSkillLevel = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_SKILL, static_cast<Stats::StatPeriodType>(periodType), true));
							lobbySkill->addCurrentSkillLevel(playerId, currentSkillLevel, static_cast<Stats::StatPeriodType>(periodType));

							int32_t currentOppSkillLevel = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_OPPO_LEVEL, static_cast<Stats::StatPeriodType>(periodType), true));
							lobbySkill->addCurrentOppSkillLevel(playerId, currentOppSkillLevel, static_cast<Stats::StatPeriodType>(periodType));
						}
					}
				}

				playerIter = OsdkPlayerReportsMap.begin();
				for (; playerIter != playerEnd; ++playerIter)
				{
					GameManager::PlayerId playerId = playerIter->first;

					Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(statsCategory, playerId);
					if (NULL != key)
					{
						// Loop through all the supported stats period type
						for (uint32_t periodType = static_cast<uint32_t>(Stats::STAT_PERIOD_ALL_TIME); periodType < Stats::STAT_NUM_PERIODS; periodType++)
						{
							int32_t newSkillPoints = lobbySkill->getNewLobbySkillPoints(playerId, static_cast<Stats::StatPeriodType>(periodType));
							mUpdateStatsHelper.setValueInt(key, STATNAME_OVERALLPTS, static_cast<Stats::StatPeriodType>(periodType), newSkillPoints);

							int32_t newSkillLevel = lobbySkill->getNewLobbySkillLevel(playerId, mReportConfig, newSkillPoints);
							mUpdateStatsHelper.setValueInt(key, STATNAME_SKILL, static_cast<Stats::StatPeriodType>(periodType), newSkillLevel);

							int32_t newOppSkillLevel = lobbySkill->getNewLobbyOppSkillLevel(playerId, static_cast<Stats::StatPeriodType>(periodType));
							mUpdateStatsHelper.setValueInt(key, STATNAME_OPPO_LEVEL, static_cast<Stats::StatPeriodType>(periodType), newOppSkillLevel);

							TRACE_LOG("[GameOTPReportSlave:" << this << "].transformStats() for playerId: " << playerId << ", newSkillPoints:" << newSkillPoints <<
								", newSkillLevel:" << newSkillLevel << ", newOppSkillLevel:" << newOppSkillLevel << ", periodType=" << periodType);
						}
					}
				}
			}

			updateCustomTransformStats(statsCategory);
		}
    }
	else
	{
		processErr = Blaze::ERR_SYSTEM;
	}

	return processErr;
}

/*************************************************************************************************/
/*! \brief aggregatePlayerStats
    Calculate the aggregated value of player stats and store them in a map

    \Customizable - Virtual function to override default behavior.
    \Customizable - aggregateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void GameOTPReportSlave::aggregatePlayerStats()
{
    TRACE_LOG("[GameOTPReportSlave:" << this << "].aggregatePlayerStats()");

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
