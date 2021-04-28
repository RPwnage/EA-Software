/*************************************************************************************************/
/*!
    \file   ssfminigamereportslave.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "ssfminigamereportslave.h"

namespace Blaze
{
namespace GameReporting
{
/*************************************************************************************************/
/*! \brief Constructor
*/
/*************************************************************************************************/
SSFMinigameReportSlave::SSFMinigameReportSlave(GameReportingSlaveImpl &component) :
	GameOTPReportSlave(component)
{
}

/*************************************************************************************************/
/*! \brief Destructor
*/
/*************************************************************************************************/
SSFMinigameReportSlave::~SSFMinigameReportSlave()
{
}

/*************************************************************************************************/
/*! \brief Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor *SSFMinigameReportSlave::create(GameReportingSlaveImpl &component)
{
	return BLAZE_NEW_NAMED("SSFMinigameReportSlave") SSFMinigameReportSlave(component);
}

/*************************************************************************************************/
/*! \brief Determine the game results.
*/
/*************************************************************************************************/
void SSFMinigameReportSlave::determineGameResult()
{
	const SSFMinigamesReportBase::ParticipantReportMap &participants = getParticipantReports();
	SSFMinigamesReportBase::ParticipantReportMap::const_iterator iter = participants.begin();
	mWinScore = -1;
	uint8_t cnt = 0;
	for ( ; iter != participants.end(); iter++)
	{
		int16_t score = iter->second->getScore();
		if (score > mWinScore)
		{
			cnt = 1;
			mWinScore = score;
		}
		else if (score == mWinScore)
		{
			cnt++;
		}
	}
	if (cnt == participants.size())
	{
		mTieGame = true;
	}
	else
	{
		mTieGame = false;
	}
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap &osdkPlayerReportsMap = getPlayerReports();
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter =
		osdkPlayerReportsMap.begin();
	for ( ; playerIter != osdkPlayerReportsMap.end(); playerIter++)
	{
		GameManager::PlayerId playerID = playerIter->first;
		if (mTieGame)
		{
			mTieSet.insert(playerID);
			continue;
		}
		OSDKGameReportBase::OSDKPlayerReport &playerReport = *playerIter->second;
		int32_t score = playerReport.getScore();
		if (score == mWinScore)
		{
			mWinnerSet.insert(playerID);
		}
		else
		{
			mLoserSet.insert(playerID);
		}
	}
}

/*************************************************************************************************/
/*! \brief Get custom OTP game type name.
*/
/*************************************************************************************************/
const char8_t *SSFMinigameReportSlave::getCustomOTPGameTypeName() const
{
	return "gameType35";
};

/*************************************************************************************************/
/*! \brief Get custom OTP stats category name.
*/
/*************************************************************************************************/
const char8_t *SSFMinigameReportSlave::getCustomOTPStatsCategoryName() const
{
	return "";
};

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.

	\param processedReport	Contains the final game report and information used by game history.
	\param playerIDs		List of players to distribute results to.

	\return ERR_OK on success. GameReporting-specific error on failure.
********************************************************************************/
BlazeRpcError SSFMinigameReportSlave::process(ProcessedGameReport &processedReport,
												GameManager::PlayerIdList &playerIDs)
{
	const GameType &gameType = processedReport.getGameType();
	if (blaze_strcmp(gameType.getGameReportName().c_str(), getCustomOTPGameTypeName()))
	{
		return ERR_OK;
	}

	// Create the parser
	mReportParser = BLAZE_NEW Utilities::ReportParser(gameType, processedReport);
	mReportParser->setUpdateStatsRequestBuilder(&mBuilder);

	// Fill in report with values via configuration
	GameReport &report = processedReport.getGameReport();
	if (!mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_VALUES))
	{
		WARN_LOG("[SSFMinigameReportSlave::" << this <<
					"].process():  Error parsing report values.");
		BlazeRpcError processErr = mReportParser->getErrorCode();
		delete mReportParser;
		return processErr;
	}

	mProcessedReport = &processedReport;
	processCommon();
	initGameParams();
	if (skipProcess())
	{
		TRACE_LOG("[SSFMinigameReportSlave::" << this <<
					"].process():  Skipping report processing but sending back notification with proper end result.");
		determineGameResult();
		updatePlayerStats();
		updateNotificationReport();
		delete mReportParser;
		return ERR_OK;
	}
	determineGameResult();
	updatePlayerKeyscopes();

	// Parse the keyscopes from the configuration
	if (!mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES))
	{
		WARN_LOG("[SSFMinigameReportSlave::" << this <<
					"].process():  Error parsing report keyscopes.");
		BlazeRpcError processErr = mReportParser->getErrorCode();
		delete mReportParser;
		return processErr;
	}

	if (skipProcessConclusionType())
	{
		updateSkipProcessConclusionTypeStats();
		delete mReportParser;
		return ERR_OK;
	}
	updatePlayerStats();
	updateGameStats();
	selectCustomStats();
	UserActivityTracker::StatsUpdater statsUpdater(&mBuilder, &mUpdateStatsHelper);
	statsUpdater.updateGamesPlayed(playerIDs);
	statsUpdater.selectStats();

	// Extract and set stats
	if (!mReportParser->parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS))
	{
		WARN_LOG("[SSFMinigameReportSlave::" << this <<
					"].process():  Error parsing report stats.");
		BlazeRpcError processErr = mReportParser->getErrorCode();
		delete mReportParser;
		return processErr;
	}

	bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();
	BlazeRpcError processErr = mUpdateStatsHelper.initializeStatUpdate(mBuilder, strict);
	if (processErr == ERR_OK)
	{
		if (transformStats() == ERR_OK)
		{
			statsUpdater.transformStats();
		}
		processErr = mUpdateStatsHelper.commitStats();
	}
	if (processedReport.needsHistorySave() && processedReport.getReportType() !=
												REPORT_TYPE_TRUSTED_MID_GAME)
	{
		// Extract game history attributes from report
		GameHistoryReport &historyReport = processedReport.getGameHistoryReport();
		mReportParser->setGameHistoryReport(&historyReport);
		mReportParser->parse(*report.getReport(),
								Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
	}
	delete mReportParser;

	// Post game reporting processing
	processErr = processUpdatedStats() ? ERR_OK : ERR_SYSTEM;

	sendEndGameMail(playerIDs);
	return processErr;
}

/*************************************************************************************************/
/*! \brief Perform post stats update processing.

	\return - true if the post-game processing is performed successfully

	\customizable - processCustomUpdatedStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
bool SSFMinigameReportSlave::processUpdatedStats()
{
	bool success = processCustomUpdatedStats();
	updateNotificationReport();
	return success;
}

/*************************************************************************************************/
/*! \brief Calculate stats.

	\customizable - updateCustomTransformStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
BlazeRpcError SSFMinigameReportSlave::transformStats()
{
	if (mUpdateStatsHelper.fetchStats() != DB_ERR_OK)
	{
		return ERR_SYSTEM;
	}
	updateCustomTransformStats();
	return ERR_OK;
}

/*************************************************************************************************/
/*! \brief Update common stats regardless of the game result.

	\customizable - updateCustomPlayerStats() to provide additional custom behavior.
*/
/*************************************************************************************************/
void SSFMinigameReportSlave::updateCommonStats(GameManager::PlayerId playerID,
												OSDKGameReportBase::OSDKPlayerReport &playerReport)
{
	// Update player's DNF stat
	updatePlayerDNF(playerReport);

	updateCustomPlayerStats(playerID, playerReport);
}

/*************************************************************************************************/
/*! \brief Update custom player keyscopes.
*/
/*************************************************************************************************/
void SSFMinigameReportSlave::updateCustomPlayerKeyscopes()
{
	const GameInfo *reportGameInfo = mProcessedReport->getGameInfo();
	if (reportGameInfo == nullptr)
	{
		return;
	}
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap &osdkPlayerReportsMap = getPlayerReports();
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter =
		osdkPlayerReportsMap.begin();
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerEnd =
		osdkPlayerReportsMap.end();
	for ( ; playerIter != playerEnd; playerIter++)
	{
		GameManager::PlayerId playerID = playerIter->first;
		GameInfo::PlayerInfoMap::const_iterator player =
			reportGameInfo->getPlayerInfoMap().find(playerID);
		if (player == reportGameInfo->getPlayerInfoMap().end())
		{
			continue;
		}

		// Adding player's country keyscope info to the player report for reports
		// that have set keyscope "accountcountry" in gamereporting.cfg
		OSDKGameReportBase::OSDKPlayerReport &playerReport = *playerIter->second;
		GameManager::GamePlayerInfo &playerInfo = *player->second;
		uint16_t accountLocale = LocaleTokenGetCountry(playerInfo.getAccountLocale());
		playerReport.setAccountCountry(accountLocale);
		TRACE_LOG("[SSFMinigameReportSlave:" << this <<
					"].updatePlayerKeyscopes():  Set account country " << accountLocale <<
					" for player " << playerID);
	}
}

/*************************************************************************************************/
/*! \brief Update loser's stats.
*/
/*************************************************************************************************/
void SSFMinigameReportSlave::updateGameModeLossPlayerStats(GameManager::PlayerId playerID,
															OSDKGameReportBase::OSDKPlayerReport &playerReport)
{
	updateLossPlayerStats(playerID, playerReport);
}

/*************************************************************************************************/
/*! \brief Update tied player's stats.
*/
/*************************************************************************************************/
void SSFMinigameReportSlave::updateGameModeTiePlayerStats(GameManager::PlayerId playerID,
															OSDKGameReportBase::OSDKPlayerReport &playerReport)
{
	updateTiePlayerStats(playerID, playerReport);
}

/*************************************************************************************************/
/*! \brief Update winner's stats.
*/
/*************************************************************************************************/
void SSFMinigameReportSlave::updateGameModeWinPlayerStats(GameManager::PlayerId playerID,
															OSDKGameReportBase::OSDKPlayerReport &playerReport)
{
	updateWinPlayerStats(playerID, playerReport);
}

/*************************************************************************************************/
/*! \brief Update winner's notification report.
*/
/*************************************************************************************************/
void SSFMinigameReportSlave::updateNotificationReport()
{
	// Obtain the custom data for report notification
	OSDKGameReportBase::OSDKNotifyReport *osdkReportNotification =
		static_cast<OSDKGameReportBase::OSDKNotifyReport *>(mProcessedReport->getCustomData());
	SSFMinigamesReportBase::NotificationCustomGameData *gameCustomData =
		BLAZE_NEW SSFMinigamesReportBase::NotificationCustomGameData();
	gameCustomData->setGameReportingID(mProcessedReport->getGameReport().getGameReportingId());
	OSDKGameReportBase::OSDKReport &osdkReport = getOsdkReport();
	gameCustomData->setMatchEndReason(osdkReport.getGameReport().getFinishedStatus());
	gameCustomData->setSecondsPlayed(static_cast<uint16_t>
										(osdkReport.getGameReport().getGameTime()));
	const SSFMinigamesReportBase::ParticipantReportMap &participants = getParticipantReports();
	SSFMinigamesReportBase::ParticipantReportMap::const_iterator iter = participants.begin();
	const OSDKReport::OSDKPlayerReportsMap &playerReportMap = osdkReport.getPlayerReports();
	for ( ; iter != participants.end(); iter++)
	{
		SSFMinigamesReportBase::ParticipantReportPtr notifyParticipantReport =
			BLAZE_NEW SSFMinigamesReportBase::ParticipantReport();
		notifyParticipantReport->setEndResult(iter->second->getEndResult());
		uint64_t blazeID = 0;
		for (const OSDKReport::OSDKPlayerReportsMap::value_type playerReportEntry : playerReportMap)
		{
			const OSDKPlayerReport *osdkPlayerReport = playerReportEntry.second;
			const SSFMinigamesReportBase::PlayerReport *playerReport =
				static_cast<const SSFMinigamesReportBase::PlayerReport *>
				(osdkPlayerReport->getCustomPlayerReport());
			if (playerReport->getParticipantID() == iter->first)
			{
				blazeID = playerReportEntry.first;
				break;
			}
		}
		notifyParticipantReport->setPersonaID(blazeID);
		notifyParticipantReport->setScore(iter->second->getScore());
		gameCustomData->getParticipantReports()[iter->first] = notifyParticipantReport;
	}
	osdkReportNotification->setCustomDataReport(*gameCustomData);
}

/*************************************************************************************************/
/*! \brief Update the players' stats based on game result.
*/
/*************************************************************************************************/
void SSFMinigameReportSlave::updatePlayerStats()
{
	SSFMinigamesReportBase::ParticipantReportMap &participants = getParticipantReports();
	SSFMinigamesReportBase::ParticipantReportMap::iterator iter = participants.begin();
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap &osdkPlayerReportsMap = getPlayerReports();
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerEnd =
		osdkPlayerReportsMap.end();
	for ( ; iter != participants.end(); iter++)
	{
		SSFSeasonsReportBase::SsfMatchEndResult result = SSFSeasonsReportBase::SSF_END_INVALID;
		BlazeId personaID = 0;
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter =
			osdkPlayerReportsMap.begin();
		for ( ; playerIter != playerEnd; playerIter++)
		{
			OSDKGameReportBase::OSDKPlayerReport &playerReport = *playerIter->second;
			SSFMinigamesReportBase::PlayerReport &report =
				static_cast<SSFMinigamesReportBase::PlayerReport &>
				(*playerReport.getCustomPlayerReport());
			if (report.getParticipantID() == iter->first)
			{
				personaID = playerIter->first;
				break;
			}
		}
		if (playerIter != playerEnd)
		{
			GameManager::PlayerId playerID = playerIter->first;
			OSDKGameReportBase::OSDKPlayerReport &playerReport = *playerIter->second;
			if (mTieGame)
			{
				updateGameModeTiePlayerStats(playerID, playerReport);
			}
			else if (mWinnerSet.find(playerID) != mWinnerSet.end())
			{
				updateGameModeWinPlayerStats(playerID, playerReport);
			}
			else if (mLoserSet.find(playerID) != mLoserSet.end())
			{
				updateGameModeLossPlayerStats(playerID, playerReport);
			}
			updateCommonStats(playerID, playerReport);
			uint32_t userReportedResult = playerReport.getUserResult();
			uint32_t playerCalculatedResult = playerReport.getGameResult();
			if (mTieGame && mWinScore <= 0)
			{
				result = SSFSeasonsReportBase::SSF_END_NO_CONTEST;
				if (userReportedResult == GAME_RESULT_QUITANDCOUNT)
				{
					result = SSFSeasonsReportBase::SSF_END_QUIT;
				}
			}
			else if (userReportedResult == GAME_RESULT_DISCONNECTANDCOUNT ||
						userReportedResult == GAME_RESULT_QUITANDCOUNT)
			{
				result = SSFSeasonsReportBase::SSF_END_QUIT;
			}
			else if (playerCalculatedResult == WLT_LOSS)
			{
				result = SSFSeasonsReportBase::SSF_END_LOSS;
			}
			else if (playerCalculatedResult == WLT_TIE)
			{
				result = SSFSeasonsReportBase::SSF_END_DRAW;
			}
			else if (playerCalculatedResult == WLT_WIN)
			{
				result = SSFSeasonsReportBase::SSF_END_WIN;
			}
			else
			{
				// If any player quit or did not finish but the game still counts as finished,
				// the game result would have been modified in updateWinPlayerStats() and
				// WLT_OPPOQCTAG OR'd to the result
				uint32_t discGameResultLoss = WLT_LOSS;
				uint32_t discGameResultTie = WLT_TIE;
				uint32_t discGameResultWin = WLT_WIN;
				switch (userReportedResult)
				{
				case GAME_RESULT_CONCEDE_DISCONNECT:
				case GAME_RESULT_DISCONNECTANDCOUNT:
				case GAME_RESULT_DNF_DISCONNECT:
				case GAME_RESULT_FRIENDLY_DISCONNECT:
				case GAME_RESULT_MERCY_DISCONNECT:
				{
					discGameResultLoss |= WLT_DISC;
					discGameResultTie |= WLT_DISC;
					discGameResultWin |= WLT_DISC;
				}
				break;
				case GAME_RESULT_CONCEDE_QUIT:
				case GAME_RESULT_DNF_QUIT:
				case GAME_RESULT_FRIENDLY_QUIT:
				case GAME_RESULT_MERCY_QUIT:
				case GAME_RESULT_QUITANDCOUNT:
				{
					discGameResultLoss |= WLT_CONCEDE;
					discGameResultTie |= WLT_CONCEDE;
					discGameResultWin |= WLT_CONCEDE;
				}
				break;
				default:
				break;
				}
				discGameResultWin |= WLT_OPPOQCTAG;
				if (playerCalculatedResult == discGameResultLoss)
				{
					result = SSFSeasonsReportBase::SSF_END_LOSS;
				}
				else if (playerCalculatedResult == discGameResultTie)
				{
					result = SSFSeasonsReportBase::SSF_END_DRAW;
				}
				else if (playerCalculatedResult == discGameResultWin)
				{
					result = SSFSeasonsReportBase::SSF_END_WIN;
				}
			}
		}
		else if (mTieGame)
		{
			if (mWinScore <= 0)
			{
				result = SSFSeasonsReportBase::SSF_END_NO_CONTEST;
			}
			else
			{
				result = SSFSeasonsReportBase::SSF_END_DRAW;
			}
		}
		else if (iter->second->getScore() == mWinScore)
		{
			result = SSFSeasonsReportBase::SSF_END_WIN;
		}
		else
		{
			result = SSFSeasonsReportBase::SSF_END_LOSS;
		}
		iter->second->setEndResult(result);
		iter->second->setPersonaID(personaID);
	}
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted to master for collation or direct to 
    processing for offline or trusted reports. Behavior depends on the calling RPC.

    \param report Incoming game report from submit request.

    \return ERR_OK on success. GameReporting-specific error on failure.
********************************************************************************/
BlazeRpcError SSFMinigameReportSlave::validate(GameReport &report) const
{
    if (report.getReport() == nullptr)
    {
        WARN_LOG("[SSFMinigameReportSlave:" << this << "].validate():  Null report.");
        return ERR_SYSTEM;
    }
    return ERR_OK;
}

OSDKGameReportBase::OSDKReport &SSFMinigameReportSlave::getOsdkReport()
{
	return static_cast<OSDKGameReportBase::OSDKReport &>(*mProcessedReport->getGameReport().getReport());
}

SSFMinigamesReportBase::ParticipantReportMap &SSFMinigameReportSlave::getParticipantReports()
{
	OSDKGameReportBase::OSDKReport &osdkReport = getOsdkReport();
	SSFMinigamesReportBase::GameReport *gameReport =
		static_cast<SSFMinigamesReportBase::GameReport *>
		(osdkReport.getGameReport().getCustomGameReport());
	return gameReport->getParticipantReports();
}

OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap &SSFMinigameReportSlave::getPlayerReports()
{
	return getOsdkReport().getPlayerReports();
}
}   // namespace GameReporting
}   // namespace Blaze
