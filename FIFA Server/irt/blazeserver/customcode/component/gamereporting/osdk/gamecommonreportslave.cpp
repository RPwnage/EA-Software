/*************************************************************************************************/
/*!
    \file   gamecommonreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "gamereporting/gamereportingslaveimpl.h"

#include "gamereporting/util/gamehistoryutil.h"
#include "gamereporting/util/reportparserutil.h"
#include "gamereporting/util/skilldampingutil.h"

#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

#include "gamereporting/osdk/gamecommonreportslave.h"

namespace Blaze
{

namespace GameReporting
{
    
/*************************************************************************************************/
/*! \brief GameCommonReportSlave
    Create

    \return NULL
*/
/*************************************************************************************************/
GameReportProcessor* GameCommonReportSlave::create(GameReportingSlaveImpl& component)
{
    // GameCommonReportSlave should not be instantiated
    return NULL;
}

/*************************************************************************************************/
/*! \brief GameCommonReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCommonReportSlave::GameCommonReportSlave(GameReportingSlaveImpl& component) :
    GameReportProcessor(component),
    mProcessedReport(NULL),
    mReportConfig(NULL),
    mIsOfflineReport(false),
    mIsRankedGame(false),
    mTieGame(false),
    mIsUpdatePlayerConclusionTypeStats(false),
    mGameTime(0),
    mCategoryId(0),
    mRoomId(0),
    mWinTeamId(UINT32_MAX),
    mPlayerReportMapSize(0),
    mLowestPlayerScore(INT32_MAX),
    mHighestPlayerScore(INT32_MIN)
{
    const EA::TDF::Tdf *customTdf = component.getCustomConfig();
    if (NULL != customTdf)
    {
        const OSDKGameReportBase::OSDKCustomGlobalConfig* customConfig = static_cast<const OSDKGameReportBase::OSDKCustomGlobalConfig* >(customTdf);
        mReportConfig = ReportConfig::createReportConfig(customConfig);
    }
}

/*************************************************************************************************/
/*! \brief GameCommonReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCommonReportSlave::~GameCommonReportSlave()
{
    if (NULL != mReportConfig)
    {
        delete mReportConfig;
    }
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted to master for collation or direct to 
    processing for offline or trusted reports. Behavior depends on the calling RPC.

    \param report Incoming game report from submit request
    \return ERR_OK on success. GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError GameCommonReportSlave::validate(GameReport& report) const
{
    return Blaze::ERR_OK;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.
 
    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success. GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError GameCommonReportSlave::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    return Blaze::ERR_OK;
}

/*! ****************************************************************************/
	/*! \brief Called after process() has been called

		\param processedReport Contains the final game report and information used by game history.
		\return ERR_OK on success.  GameReporting specific error on failure.
	********************************************************************************/
BlazeRpcError GameCommonReportSlave::postProcess(ProcessedGameReport& processedReport, bool reportWasSuccessful)
{
	const ServiceNameInfo* serviceNameInfo = gController->getServiceNameInfo(gController->getDefaultServiceName());
	if (serviceNameInfo->getPlatform() == ClientPlatformType::ps5)
	{
		// the code below is taken from BasicGameReportProcessor::process() and should be kept in sync with it.
		// if the code below is not working properly, check the class above to see if we need to update our code.
		const char8_t* matchId = "";
		if (processedReport.getGameInfo() != nullptr)
		{
			matchId = processedReport.getGameInfo()->getExternalSessionIdentification().getPs5().getMatch().getMatchId();
		}

		if (matchId[0] == '\0')
		{
			TRACE_LOG("[GameCommonReportSlave].postProcess: no associated PSN Match");
		}
		else
		{
			/// @todo [ps5-gr] async option ???  would need a copy of processed report if there's validation during parse (due to getMatchDetail call)
			// gameInfo can't be null if we're here
			TRACE_LOG("[GameCommonReportSlave].postProcess: PSN Match start (" << matchId << ")");
			Utilities::PsnMatchUtilPtr pmu = BLAZE_NEW Utilities::PsnMatchUtil(mComponent, processedReport.getReportType(), processedReport.getGameReport().getGameReportName(), *processedReport.getGameInfo());
			bool reportSubmitted = false;
			bool hasCancelled = false;
			if (reportWasSuccessful)
			{
				GameReport& report = processedReport.getGameReport();

				Utilities::ReportParser reportParser(processedReport.getGameType(), processedReport);
				reportParser.setPsnMatchUtil(pmu);
				bool success = reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_PSN_MATCH);
				if (success)
				{
					if (!didGameFinish())
					{
						TRACE_LOG("[GameCommonReportSlave].postProcess: canceling PSN Match (" << matchId << ") with reportSubmitted: " << reportSubmitted << "and didGameFinish: " << didGameFinish() << ".");
						if (pmu->cancelMatch() == ERR_OK)
						{
							hasCancelled = true;
						}
					}

					// the util will log any errors
					if (pmu->initializeReport() == ERR_OK)
					{
						Blaze::PSNServices::Matches::PsnGroupingTypeEnum groupingType = Blaze::PSNServices::Matches::PsnGroupingTypeEnum::INVALID_GROUPING_TYPE;
						Blaze::PSNServices::Matches::ParsePsnGroupingTypeEnum(pmu->getMatchDetail()->getGroupingType(), groupingType);

						setEntityScores(processedReport);

						if (groupingType == Blaze::PSNServices::Matches::PsnGroupingTypeEnum::TEAM_MATCH)
						{
							const int NUM_TEAMS = 2;
							if (pmu->getReportRequest().getMatchResults().getCompetitiveResult().getTeamResults().size() >= NUM_TEAMS)
							{
								int32_t score[NUM_TEAMS];
								Blaze::PSNServices::Matches::MatchTeamResultPtr teamResult[NUM_TEAMS];
								for (int teamIndex = 0; teamIndex < NUM_TEAMS; ++teamIndex)
								{
									score[teamIndex] = -1;
									teamResult[teamIndex] = pmu->getReportRequest().getMatchResults().getCompetitiveResult().getTeamResults().at(teamIndex);

									const Blaze::PSNServices::Matches::MatchTeamResult::MatchTeamMemberResultList& teamMemberResultList = teamResult[teamIndex]->getTeamMemberResults();
									for (const Blaze::PSNServices::Matches::MatchTeamMemberResultPtr teamMemberResult : teamMemberResultList)
									{
										score[teamIndex] = getEntityScore(teamMemberResult->getPlayerId());
										if (score[teamIndex] >= 0)
										{
											break;
										}
									}

									if (score[teamIndex] < 0)
									{
										score[teamIndex] = 0;
									}

									teamResult[teamIndex]->setScore(static_cast<float>(score[teamIndex]));
								}

								int32_t side1rank = (score[0] >= score[1]) ? 1 : 2;
								int32_t side2rank = (score[1] >= score[0]) ? 1 : 2;
								teamResult[0]->setRank(side1rank);
								teamResult[1]->setRank(side2rank);
							}
							else
							{
								WARN_LOG("[GameCommonReportSlave].postProcess: getTeamResults array isnt big enough for: " << matchId);
							}
						}
						else //If NOT Teams based, use getPlayerResults() instead of getTeamResults()
						{
							// Handle Volta minigames
							Blaze::PSNServices::Matches::MatchCompetitiveResult::MatchPlayerResultList &playerResults = pmu->getReportRequest().getMatchResults().getCompetitiveResult().getPlayerResults();
							eastl_size_t num = playerResults.size();
							int32_t score[22];
							for (eastl_size_t i = 0; i < num; i++)
							{
								Blaze::PSNServices::Matches::MatchPlayerResultPtr player = playerResults.at(i);
								score[i] = getEntityScore(player->getPlayerId());
								player->setScore(static_cast<float>(score[i]));
							}
							for (int32_t rank = 1; rank <= num; rank++)
							{
								int32_t maxIdx = rank - 1;
								int32_t max = score[maxIdx];
								for (size_t i = rank; i < sizeof(score) / sizeof(score[0]); i++)
								{
									if (score[i] > max)
									{
										max = score[i];
										maxIdx = i;
									}
								}
								int32_t temp = score[rank - 1];
								score[rank - 1] = max;
								score[maxIdx] = temp;
								Blaze::PSNServices::Matches::MatchPlayerResultPtr player = playerResults.at(maxIdx);
								player->setRank(rank);
							}
						}

						if (pmu->submitReport() == ERR_OK)
						{
							reportSubmitted = true;
						}
					}
				}
				else
				{
					WARN_LOG("[GameCommonReportSlave].postProcess: failed to parse PSN Match section for match: " << matchId);
				}

			}

			if (!reportSubmitted && !hasCancelled)
			{
				TRACE_LOG("[GameCommonReportSlave].postProcess: canceling PSN Match (" << matchId << ") with reportSubmitted: " << reportSubmitted << "and didGameFinish: " << didGameFinish() << ".");
				pmu->cancelMatch();
			}
			TRACE_LOG("[GameCommonReportSlave].postProcess: PSN Match finish (" << matchId << ")");
		}
	}

	return ERR_OK;
}
 
/*! ****************************************************************************/
/*! \brief Triggered on server reconfiguration.
********************************************************************************/
void GameCommonReportSlave::reconfigure() const
{
}

/*************************************************************************************************/
/*! \brief processCommon
    Perform common processing for all report processors

    \Customizable - None.
*/
/*************************************************************************************************/
void GameCommonReportSlave::processCommon()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());

    mIsOfflineReport = (REPORT_TYPE_OFFLINE == mProcessedReport->getReportType());
    if (false == mIsOfflineReport)
    {
        if (mProcessedReport->getGameInfo() != NULL)
        {
            mIsRankedGame = mProcessedReport->getGameInfo()->getGameSettings().getRanked();
        }
    }
    else
    {
        // Offline game does not have a game session associates with it, so set it as ranked game
        mIsRankedGame = true;
    }
    mGameTime = OsdkReport.getGameReport().getGameTime();
    mPlayerReportMapSize = OsdkReport.getPlayerReports().size();

    addDataForExternalSystems();
    updateGlobalStats();
}

/*************************************************************************************************/
/*! \brief updateGlobalStats
    update global stats

    \Customizable - None.
*/
/*************************************************************************************************/
void GameCommonReportSlave::updateGlobalStats()
{
    if (false == mIsOfflineReport)
    {
        Stats::UpdateStatsRequestBuilder builder;
        Utilities::ReportParser reportParser(mProcessedReport->getGameType(),  *mProcessedReport);
        reportParser.setUpdateStatsRequestBuilder(&builder);

        // Use unique playerId 1 for GlobalStats update to track the total number of games player
        GameManager::PlayerId playerId = 1;
        Stats::ScopeNameValueMap scopes;

        builder.startStatRow("GlobalStats", playerId, &scopes);
        builder.incrementStat("totalOnlineMatches", 1);
        builder.completeStatRow();
    }
}

/*************************************************************************************************/
/*! \brief skipProcess
    Indicate if the rest of report processing should continue

    \return - true if the rest of report processing is not needed

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
bool GameCommonReportSlave::skipProcess()
{
    // skip stats update and history saving for online unranked games
    if (false == mIsOfflineReport && false == mIsRankedGame)
    {
        TRACE_LOG("[GameCommonReportSlave:" << this << "].skipProcess(): Skip stats update and history saving for online unranked games");
        mProcessedReport->enableHistorySave(false);
        return true;
    }

    if (false == mIsOfflineReport)
    {
        // stage 1 disconnect
        // no stats will be tracked
        if (mGameTime < mReportConfig->getEndPhaseMaxTime()[0])
        {
            TRACE_LOG("[GameCommonReportSlave:" << this << "].skipProcess(): End Phase 1 - Nothing counts");
            mProcessedReport->enableHistorySave(false);
            return true;
        }
        // stage 2 disconnect
        // all stats will be tracked
        else
        {
            TRACE_LOG("[GameCommonReportSlave:" << this << "].skipProcess(): End Phase 2 -  Everything counts");
            return false;
        }
    }

    return false;
}

/*************************************************************************************************/
/*! \brief skipProcessConclusionTypeCommon
    Check if stats update is skipped because of conclusion type

    \return - true if game report processing is to be skipped
    \return - false if game report should be processed

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
bool GameCommonReportSlave::skipProcessConclusionTypeCommon()
{
    bool skipProcess = false;
    uint32_t userResult = GAME_RESULT_COMPLETE_REGULATION;

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for(; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

        // Check the conclusion type based on the game report ATTRIBNAME_USERRESULT
        userResult = playerReport.getUserResult();
        switch (userResult)
        {
            case GAME_RESULT_DNF_QUIT:
            case GAME_RESULT_FRIENDLY_QUIT:
            case GAME_RESULT_DNF_DISCONNECT:
            case GAME_RESULT_FRIENDLY_DISCONNECT:
                skipProcess = true;
                break;

            default:
                skipProcess = false;
                break;
        }

        if(true == skipProcess)
        {
            break;
        }
    }

    // User does not want this game to be counted. Only assigned loss and DNF to early quitter
    if (true == skipProcess)
    {
        mProcessedReport->enableHistorySave(false);
        updatePlayerStatsConclusionType();
    }

    TRACE_LOG("[GameCommonReportSlave:" << this << "].skipProcessConclusionTypeCommon() skipProcess = " << skipProcess);
    return skipProcess;
}

/*************************************************************************************************/
/*! \brief updateSkipProcessConclusionTypeStats
    update the skip conclusion type stats

    \Customizable - Virtual function.
*/
/*************************************************************************************************/
void GameCommonReportSlave::updateSkipProcessConclusionTypeStats()
{
    if (true == mIsUpdatePlayerConclusionTypeStats)
    {
        // Check if there is any stats to update for the case when the game report processing
        // is skipped because of conclusion type
        bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
        BlazeRpcError processErr = mUpdateStatsHelper.initializeStatUpdate((Stats::UpdateStatsRequest&)mBuilder, strict);

        if (Blaze::ERR_OK == processErr)
        {
            TRACE_LOG("[GameCommonReportSlave:" << this << "].updateSkipProcessConclusionTypeStats() - commit stats");
            processErr = mUpdateStatsHelper.commitStats();
        }
    }
}

/*************************************************************************************************/
/*! \brief getInt
    Get the integer value

    \Customizable - None.
*/
/*************************************************************************************************/
int32_t GameCommonReportSlave::getInt(const Collections::AttributeMap *map, const char *key)
{
    Collections::AttributeMap::const_iterator iter = map->find(key);
    if(iter != map->end())
    {
        int val;
        if(sscanf(iter->second, "%d", &val))
        {
            return (int32_t)val;
        }
    }
    return 0;
}

/*************************************************************************************************/
/*! \brief adjustPlayerScore
    Make sure there is a minimum spread between the player that disconnected and the other
    players find the lowest score of those who did not cheat or quit and assign a default
    score if necessary

    \Customizable - None.
*/
/*************************************************************************************************/
void GameCommonReportSlave::adjustPlayerScore()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerFirst, playerEnd;
    playerIter = playerFirst = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    bool zeroScore = false;
    bool playerFinished = false;
    int32_t playerScore = 0;

    for (; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        playerFinished = didPlayerFinish(playerIter->first, playerReport);
		
        if (false == playerFinished)
        {
            playerScore = static_cast<int32_t>(playerReport.getScore());

            if (INT32_MAX != mLowestPlayerScore && ((playerScore >= mLowestPlayerScore) ||
                (mLowestPlayerScore - playerScore) < DISC_SCORE_MIN_DIFF))
            {
                mTieGame = false;
                // new score for cheaters or quitters is 0
				playerScore = 0;
				zeroScore = true;
				mLowestPlayerScore = playerScore;
                playerReport.setScore(static_cast<uint32_t>(playerScore));
            }
        }
    }

    // need adjustment on scores of non-disconnected players
    if (zeroScore)
    {
        playerIter = playerFirst = OsdkPlayerReportsMap.begin();
        playerEnd = OsdkPlayerReportsMap.end();

        for (; playerIter != playerEnd; ++playerIter)
        {
            OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
            playerFinished = didPlayerFinish(playerIter->first, playerReport);
            if (true == playerFinished)
            {
                playerScore = static_cast<int32_t>(playerReport.getScore());

					playerScore = DISC_SCORE_MIN_DIFF;
				mHighestPlayerScore = playerScore;
                playerReport.setScore(static_cast<uint32_t>(playerScore));
            }
        }
    }
}

/*************************************************************************************************/
/*! \brief determineWinPlayer
    Determine the winning player by the highest score

    \Customizable - None.
*/
/*************************************************************************************************/
void GameCommonReportSlave::determineWinPlayer()
{
    int32_t prevPlayerScore = INT32_MIN;
    mHighestPlayerScore = INT32_MIN;
    mLowestPlayerScore = INT32_MAX;

    bool playerFinished = false;
    int32_t playerScore = 0;

    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerFirst, playerEnd;
    playerIter = playerFirst = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for (; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        playerFinished = didPlayerFinish(playerIter->first, playerReport);
        playerScore = static_cast<int32_t>(playerReport.getScore());
		TRACE_LOG("[GameCommonReportSlave:" << this << "].determineWinPlayer(): Player Id:" << playerIter->first << " score:" << playerScore << " playerFinish:" << playerFinished <<" ");

        // Set the client sent score to the player report map
        playerReport.setClientScore(playerScore);

        // Set player finish reason to the player report map
        determinePlayerFinishReason(playerIter->first, playerReport);

        if (true == playerFinished && playerScore > mHighestPlayerScore)
        {
            // update the highest score
            mHighestPlayerScore = playerScore;
        }
        if (true == playerFinished && playerScore < mLowestPlayerScore)
        {
            // update the lowest player score
            mLowestPlayerScore = playerScore;
        }

        // if some of the scores are different it isn't a tie. Or if someone did not finish
        if (playerIter != playerFirst && playerScore != prevPlayerScore)
        {
            mTieGame = false;
        }

        prevPlayerScore = playerScore;
    }

    adjustPlayerScore();

    // determine who the winners and losers are and add them to the result set
    playerIter = playerFirst = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();
    GameManager::PlayerId playerId = INVALID_BLAZE_ID;

    for (; playerIter != playerEnd; ++playerIter)
    {
        playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

        playerScore = static_cast<int32_t>(playerReport.getScore());

        bool winner = (playerScore > 0);
        if (mPlayerReportMapSize > 1)
            winner = (playerScore == mHighestPlayerScore);

        if (mTieGame)
        {
			TRACE_LOG("[GameCommonReportSlave:" << this << "].determineWinPlayer(): tie set - Player Id:" << playerIter->first << " ");
            mTieSet.insert(playerId);
        }
        else if (winner)
        {
			TRACE_LOG("[GameCommonReportSlave:" << this << "].determineWinPlayer(): winner set - Player Id:" << playerIter->first << " ");

            mWinnerSet.insert(playerId);
            mWinTeamId = playerReport.getTeam();
        }
        else
        {
			TRACE_LOG("[GameCommonReportSlave:" << this << "].determineWinPlayer(): Loser set - Player Id:" << playerIter->first << " ");
            mLoserSet.insert(playerId);
        }
    }
}

/*************************************************************************************************/
/*!
    \brief determinePlayerFinishReason
    Determine how the player finish the game (e.g. player finished, disconnected or quit)

    \Customizable - None.
*/
/*************************************************************************************************/
void GameCommonReportSlave::determinePlayerFinishReason(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    // set default
    playerReport.setFinishReason(PLAYER_FINISH_REASON_OTHER);

    if (didPlayerFinish(playerId, playerReport))
    {
        playerReport.setFinishReason(PLAYER_FINISH);
    }
    else
    {
        // if the player submitted a report, they quit
        GameManager::PlayerIdList::const_iterator submitterIdItr = mProcessedReport->getSubmitterIds().begin();
        GameManager::PlayerIdList::const_iterator submitterIdEnd = mProcessedReport->getSubmitterIds().end();
        for ( ; submitterIdItr != submitterIdEnd; submitterIdItr++)
        {
            if (playerId == (*submitterIdItr))
            {
                playerReport.setFinishReason(PLAYER_FINISH_REASON_QUIT);
                return;
            }
        }

        // otherwise the player must have disconnected
        playerReport.setFinishReason(PLAYER_FINISH_REASON_DISCONNECT);
    }
}

/*************************************************************************************************/
/*! \brief updateWinPlayerStats
    Update winner's stats

    \Customizable - None.
 */
/*************************************************************************************************/
void GameCommonReportSlave::updateWinPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    uint32_t discResult = playerReport.getUserResult(); // ATTRIBNAME_USERRESULT
    TRACE_LOG("[GameCommonReportSlave].updateWinPlayerStats() for playerId: " << playerId << ", discResult: " << discResult);

    playerReport.setWins(1);
    playerReport.setLosses(0);
	playerReport.setTies(0);

    uint32_t resultValue = WLT_WIN;
    switch (discResult)
    {
        case GAME_RESULT_DNF_DISCONNECT:
        case GAME_RESULT_FRIENDLY_DISCONNECT:
        case GAME_RESULT_CONCEDE_DISCONNECT:
        case GAME_RESULT_MERCY_DISCONNECT:
        case GAME_RESULT_DISCONNECTANDCOUNT:
            resultValue |= WLT_DISC;
            break;

        case GAME_RESULT_DNF_QUIT:
        case GAME_RESULT_FRIENDLY_QUIT:
        case GAME_RESULT_CONCEDE_QUIT:
        case GAME_RESULT_MERCY_QUIT:
        case GAME_RESULT_QUITANDCOUNT:
            resultValue |= WLT_CONCEDE;
            break;

        default:
            break;
    }

    // Check if there is any player didn't finish, mark the winner with WLT_OPPOQCTAG (opponent quit and count). So it's a DNF win.
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter = OsdkPlayerReportsMap.begin();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerEnd = OsdkPlayerReportsMap.end();
    for (; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReportInner = *playerIter->second;
        if( false == didPlayerFinish(playerIter->first, playerReportInner) )
        {
            resultValue |= WLT_OPPOQCTAG;
            break;
        }
    }
    // Store the result value in the map
    playerReport.setGameResult(resultValue);    // STATNAME_RESULT
}

/*************************************************************************************************/
/*! \brief updateLossPlayerStats
    Update loser's stats

    \Customizable - None.
 */
/*************************************************************************************************/
void GameCommonReportSlave::updateLossPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    uint32_t discResult = playerReport.getUserResult(); // ATTRIBNAME_USERRESULT
    TRACE_LOG("[GameCommonReportSlave].updateLossPlayerStats() for playerId: " << playerId << ", discResult: " << discResult);

    playerReport.setWins(0);
    playerReport.setLosses(1);
	playerReport.setTies(0);

    uint32_t resultValue = WLT_LOSS;
    switch (discResult)
    {
        case GAME_RESULT_DNF_DISCONNECT:
        case GAME_RESULT_FRIENDLY_DISCONNECT:
        case GAME_RESULT_CONCEDE_DISCONNECT:
        case GAME_RESULT_MERCY_DISCONNECT:
        case GAME_RESULT_DISCONNECTANDCOUNT:
            resultValue |= WLT_DISC;
            break;

        case GAME_RESULT_DNF_QUIT:
        case GAME_RESULT_FRIENDLY_QUIT:
        case GAME_RESULT_CONCEDE_QUIT:
        case GAME_RESULT_MERCY_QUIT:
        case GAME_RESULT_QUITANDCOUNT:
            resultValue |= WLT_CONCEDE;
            break;

        default:
            break;
    }

    // Store the result value in the map
    playerReport.setGameResult(resultValue);    // STATNAME_RESULT
}

/*************************************************************************************************/
/*! \brief updateTiePlayerStats
    Update tied player's stats

    \Customizable - None.
 */
/*************************************************************************************************/
void GameCommonReportSlave::updateTiePlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    uint32_t discResult = playerReport.getUserResult(); // ATTRIBNAME_USERRESULT
    TRACE_LOG("[GameCommonReportSlave].updateTiePlayerStats() for playerId: " << playerId << ", discResult: " << discResult);

    playerReport.setWins(0);
    playerReport.setLosses(0);
	playerReport.setTies(1);

    uint32_t resultValue = WLT_TIE;
    switch (discResult)
    {
        case GAME_RESULT_DNF_DISCONNECT:
        case GAME_RESULT_FRIENDLY_DISCONNECT:
        case GAME_RESULT_CONCEDE_DISCONNECT:
        case GAME_RESULT_MERCY_DISCONNECT:
        case GAME_RESULT_DISCONNECTANDCOUNT:
            resultValue |= WLT_DISC;
            break;

        case GAME_RESULT_DNF_QUIT:
        case GAME_RESULT_FRIENDLY_QUIT:
        case GAME_RESULT_CONCEDE_QUIT:
        case GAME_RESULT_MERCY_QUIT:
        case GAME_RESULT_QUITANDCOUNT:
            resultValue |= WLT_CONCEDE;
            break;

        default:
            break;
    }

    // Store the result value in the map
    playerReport.setGameResult(resultValue);    // STATNAME_RESULT
}

/*************************************************************************************************/
/*! \brief calcPlayerDampingPercent
    Calculate the player damping skills "wins" vs "losses"

    \return - damping skill percentage

    \Customizable - None.
*/
/*************************************************************************************************/
uint32_t GameCommonReportSlave::calcPlayerDampingPercent(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    //set damping to 100% as default case
    uint32_t dampingPercent = 100;

    char8_t query[64];
    getGameModeGameHistoryQuery(query, sizeof(query));
    TRACE_LOG("[GameCommonReportSlave].calcPlayerDampingPercent() query: " << query);

    GameHistoryUtil gameHistoryUtil(mComponent);
    QueryVarValuesList queryVarValues;
    GameReportsList gameReportsList;

    char8_t strPlayerId[32];
    blaze_snzprintf(strPlayerId, sizeof(strPlayerId), "%" PRId64, playerId);
    queryVarValues.push_back(strPlayerId);

    if (gameHistoryUtil.getGameHistory(query, 10, queryVarValues, gameReportsList))
    {
        if (mWinnerSet.find(playerId) != mWinnerSet.end())
        {
            uint32_t rematchWins = 0;
            uint32_t dnfWins = 0;
            GameReportsList winningGameReportsList;

            Collections::AttributeMap map;
            (map)["player_id"] = strPlayerId;
            (map)[STATNAME_WINS] = "1";

            gameHistoryUtil.getGameHistoryMatchingValues("player", map, gameReportsList, winningGameReportsList);
            if (winningGameReportsList.getGameReportList().size() > 0)
            {
                ResultSet::iterator loserIter = mLoserSet.begin();
                ResultSet::iterator loserEnd = mLoserSet.end();

                for (; loserIter != loserEnd; ++loserIter)
                {
                    GameReportsList rematchedWinningReportsList;
                    Collections::AttributeMap oppoSearchMap;
                    char8_t strOppoPlayerId[32];
                    blaze_snzprintf(strOppoPlayerId, sizeof(strOppoPlayerId), "%" PRId64, (*loserIter));
                    oppoSearchMap["player_id"] = strOppoPlayerId;
                    oppoSearchMap[STATNAME_LOSSES] = "1";

                    // we keep the count of rematch wins for the most rematched player, alternately, the rematch wins could be summed
                    gameHistoryUtil.getGameHistoryConsecutiveMatchingValues("player", oppoSearchMap, gameReportsList, rematchedWinningReportsList);
                    uint32_t newRematchWins = rematchedWinningReportsList.getGameReportList().size();

                    if (newRematchWins > rematchWins)
                    {
                        rematchWins = newRematchWins;
                    }

                    // only check this if the current win is via DNF
                    // if (!mReport->didGameFinish())
                    // TODO: Check did game finish
                    if (false == mProcessedReport->didAllPlayersFinish())
                    {
                        GameReportsList winnerByDnfReportsList;
                        Collections::AttributeMap winnerByDnfSearchMap;
                        winnerByDnfSearchMap["player_id"] = strPlayerId;
                        winnerByDnfSearchMap[ATTRIBNAME_WINNERBYDNF] = "1";

                        GameReportsList consecutiveMatchReportList = getGameReportListForConsecutiveMatchCount(winnerByDnfReportsList, gameReportsList);
                        // could use winningGameReportsList from above if the desire was to count consecutive wins by dnf in the player's last "x" winning games, rather than all consecutive games
                        gameHistoryUtil.getGameHistoryConsecutiveMatchingValues("player", winnerByDnfSearchMap, consecutiveMatchReportList, winnerByDnfReportsList);
                        dnfWins = winnerByDnfReportsList.getGameReportList().size();
                        winnerByDnfReportsList.getGameReportList().release();
                    }
                }

                SkillDampingUtil skillDampingUtil(mComponent);
                uint32_t rematchDampingPercent = skillDampingUtil.lookupDampingPercent(rematchWins, "rematchDamping");
                uint32_t dnfWinDampingPercent = skillDampingUtil.lookupDampingPercent(dnfWins, "dnfDamping");

                TRACE_LOG("[GameCommonReportSlave].calcPlayerDampingPercent(): rematchWins: " << rematchWins << " - rematchDampingPercent: " << rematchDampingPercent);
                TRACE_LOG("[GameCommonReportSlave].calcPlayerDampingPercent(): dnfWins: " << dnfWins << " - dnfWinDampingPercent: " << dnfWinDampingPercent);

                dampingPercent = (rematchDampingPercent * dnfWinDampingPercent) / 100; // int division is fine here because the stat transform is expecting an int
                TRACE_LOG("[GameCommonReportSlave:" << this << "].calcPlayerDampingPercent(): Skill damping value " << dampingPercent << " applied to user [" << playerId << "]");
            }

            winningGameReportsList.getGameReportList().release();
        }
    }

    gameReportsList.getGameReportList().release();

    return dampingPercent;
}

/*************************************************************************************************/
/*! \brief updatePlayerDNF
    Check if the player should be given DNF based on the userResult flag

    \Customizable - None.
*/
/*************************************************************************************************/
void GameCommonReportSlave::updatePlayerDNF(OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    uint32_t userResult = playerReport.getUserResult();

    switch(userResult)
    {
        case GAME_RESULT_DNF_QUIT:              // Game was terminated by opponent quits, no stats counted. DNF/loss counted.
        case GAME_RESULT_QUITANDCOUNT:          // Game was terminated when opponent quit and user wants to count game, stats/DNF/loss are counted
        case GAME_RESULT_DNF_DISCONNECT:        // Game was terminated by opponent disconnects, no stats counted. DNF/loss counted.
        case GAME_RESULT_DISCONNECTANDCOUNT:    // Game was terminated when opponent disconnects, user wants to give Loss/DNF to opponent. All stats count
        case GAME_RESULT_DESYNCED:              // Game was terminated because of a desync
        case GAME_RESULT_OTHER:                 // Game was terminated because of other reason
            playerReport.setCustomDnf(1);
            break;

        case GAME_RESULT_COMPLETE_REGULATION:   // Game completed normally in regulation time
        case GAME_RESULT_COMPLETE_OVERTIME:     // Game completed normally in overtime time
        case GAME_RESULT_FRIENDLY_QUIT:         // Game was terminated by consensual quit, no stats/DNF/loss are counted
        case GAME_RESULT_CONCEDE_QUIT:          // Game was terminated when the user Conceded Defeat(Opponent agreed). Stats/loss are counted, no DNF given
        case GAME_RESULT_MERCY_QUIT:            // Game was terminated when the user Granted Mercy(Opponent agreed). Stats/loss are counted, no DNF given
        case GAME_RESULT_FRIENDLY_DISCONNECT:   // Game was terminated when opponent disconnects, user wants no stats/DNF/loss are counted
        case GAME_RESULT_CONCEDE_DISCONNECT:    // Game was terminated when the user Conceded Defeat(Opponent agreed). Stats/loss are counted, no DNF given
        case GAME_RESULT_MERCY_DISCONNECT:      // Game was terminated when the user Granted Mercy(Opponent agreed). Stats/loss are counted, no DNF given
        default:
            playerReport.setCustomDnf(0);
            break;
    }
}

/*************************************************************************************************/
/*!
    \brief addDataForExternalSystems
     Adds attributes -- mostly  about players, that external subscribers to the game events will need. 
     This avoids further "requests" from say, an RS4/webserver back to Blaze to get 
     further "user" info on receipt of a game report. 

    \return - none

    \Customizable - None.
*/
/*************************************************************************************************/
void GameCommonReportSlave::addDataForExternalSystems()
{
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    // Set the game reporting ID
    GameManager::GameReportingId gameReportingId = mProcessedReport->getGameReport().getGameReportingId();
    OsdkReport.getGameReport().setGameReportId(gameReportingId);

    // Set the nucleus ID, external ID and persona for each player
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerFirst, playerEnd;
    playerIter = playerFirst = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    for (; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;

		if (!playerId && mProcessedReport->getReportType() == REPORT_TYPE_OFFLINE)
		{
			continue;
		}

        // Lookup the user information from the user session
        UserInfoPtr user;
        BlazeRpcError lookupErr = gUserSessionManager->lookupUserInfoByBlazeId(static_cast<BlazeId>(playerId), user);
        if(Blaze::ERR_OK != lookupErr)
        {
            ERR_LOG("[GameCommonReportSlave:" << this << "].addDataForExternalSystems() Error looking up user " << playerId <<
                    ", lookupUserInfoByBlazeId() returned " << ErrorHelp::getErrorName(lookupErr));
            continue; // should still attempt against other player ids
        }

        if (NULL != user)
        {
            playerReport.setNucleusId(user->getPlatformInfo().getEaIds().getNucleusAccountId());
            playerReport.setExternalId(getExternalIdFromPlatformInfo(user->getPlatformInfo()));
            playerReport.setPersona(user->getPersonaName());
        }
    }
}

/*************************************************************************************************/
/*!
    \brief setEntityScores
     Pull the scores out of the processed game report and stores them for later use

    \return - score

    \Customizable - None.
*/
/*************************************************************************************************/
void GameCommonReportSlave::setEntityScores(ProcessedGameReport& processedReport)
{

    const ServiceNameInfo* serviceNameInfo = gController->getServiceNameInfo(gController->getDefaultServiceName());
    if (serviceNameInfo->getPlatform() == ClientPlatformType::ps5)
    {
        if (processedReport.getGameInfo() != nullptr)
        {
            const Blaze::GameManager::GameInfo::PlayerInfoMap& playerInfoMap = processedReport.getGameInfo()->getPlayerInfoMap();

            OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
            const OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = OsdkReport.getPlayerReports();

            for (OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter = osdkPlayerReportsMap.begin(); playerIter != osdkPlayerReportsMap.end(); ++playerIter)
            {
                const Blaze::GameManager::PlayerId playerId = playerIter->first;
                Blaze::GameManager::GameInfo::PlayerInfoMap::const_iterator playerInfoIter = playerInfoMap.find(playerId);

                if (playerInfoIter != playerInfoMap.end())
                {
                    const Blaze::GameManager::GamePlayerInfo& gamePlayerInfo = *playerInfoIter->second;
                    const Blaze::ExternalPsnAccountId psnId = gamePlayerInfo.getUserIdentification().getPlatformInfo().getExternalIds().getPsnAccountId();
                    
                    const OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
					int32_t score = 0;
					if (playerReport.getWins() > 0)
					{
						score = getUnAdjustedHighScore();
					}
					else if (playerReport.getLosses())
					{
						score = getUnAdjustedLowScore();
					}
					else if (playerReport.getTies() > 0)
					{
						score = getUnAdjustedHighScore();
					}

                    if (score < 0)
                    {
                        score = playerReport.getScore();
                    }

                    mEntityScores[psnId] = static_cast<uint32_t>(score);
                }
            }
        }
    }
}

/*************************************************************************************************/
/*!
    \brief getScore
     Get the stored score out of the map based on the based in index

    \return - score

    \Customizable - None.
*/
/*************************************************************************************************/
int32_t GameCommonReportSlave::getEntityScore(const char* scoreIndex)
{
    int32_t score = -1;

    if (scoreIndex != NULL && strlen(scoreIndex) > 0)
    {
        uint64_t mapIndex = EA::StdC::AtoU64(scoreIndex);
        EntityScoreMap::const_iterator scoreIter = mEntityScores.find(mapIndex);
        if (scoreIter != mEntityScores.end())
        {
            score = scoreIter->second;
        }
    }

    if (score < 0)
    {
        //set as 0 but log it
        WARN_LOG("[GameCommonReportSlave].getEntityScore: missing scores for psn match reporting for entity: " << scoreIndex);
    }

    return score;
}

/*************************************************************************************************/
/*! \brief didPlayerFinish
    Check if the player has finished by checking if the player is marked as dnf in the
    dnfStatusMap and the userResult flag

    This function is needed because of GOSOPS-24426
    "GameReporting Component: User marked as DNF in mDnfStatus even after normal end game"

    \return - true or false

    \Customizable - None.
*/
/*************************************************************************************************/
bool GameCommonReportSlave::didPlayerFinish(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
	OSDKGameReportBase::OSDKGameReport& OsdkGameReport = OsdkReport.getGameReport();

	bool playerFinished = false;
	if (OsdkGameReport.getFinishedStatus() == 0)
	{
		const CollatedGameReport::DnfStatusMap& DnfStatusMap = mProcessedReport->getDnfStatusMap();
		playerFinished = (true == mIsOfflineReport) ? true : (0 == DnfStatusMap.find(playerId)->second);
	}

    // Update the player finish based on the game report ATTRIBNAME_USERRESULT
    uint32_t discResult = playerReport.getUserResult();

    if (false == playerFinished && ((GAME_RESULT_COMPLETE_REGULATION == discResult) ||
        (GAME_RESULT_COMPLETE_OVERTIME == discResult || GAME_RESULT_DESYNCED == discResult)))
    {
        // Blaze server determines the player has not finished but client game report determines the player has finished,
        // return the player did finish to true.
        playerFinished = true;
        TRACE_LOG("[GameCommonReportSlave].didPlayerFinish(): update didPlayerFinish to true for [" << playerId << "]");
    }

    return playerFinished;
}

}   // namespace GameReporting

}   // namespace Blaze
