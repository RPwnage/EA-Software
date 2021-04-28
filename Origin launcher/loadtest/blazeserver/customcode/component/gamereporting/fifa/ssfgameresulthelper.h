/*************************************************************************************************/
/*!
	\file   ssfgameresulthelper.h

	\attention
		(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_SSFGAMERESULTHELPER_H
#define BLAZE_CUSTOM_SSFGAMERESULTHELPER_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
	class SSFGameResultHelper
*/
template <typename DERIVED>
class SSFGameResultHelper
{
protected:
	SSFGameResultHelper() :
		mWinEntityId(Blaze::INVALID_BLAZE_ID),
		mLossEntityId(Blaze::INVALID_BLAZE_ID),
		mGameFinish(false),
		mLowestScore(INVALID_HIGH_SCORE),
		mHighestScore(INT32_MIN),
        mGameTime(0)
	{
	}

	virtual ~SSFGameResultHelper()
	{
	}

protected:
	void DetermineGameResult()
	{
		int32_t prevSquadScore = INT32_MIN;
		mHighestScore = INT32_MIN;
		mLowestScore = INVALID_HIGH_SCORE;

		OSDKGameReportBase::OSDKReport& osdkReport = ((DERIVED*)this)->getOsdkReport();
		SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
		SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

        mGameTime = osdkReport.getGameReport().getGameTime();

		SSFSeasonsReportBase::SSFTeamReportMap::const_iterator squadIter, squadIterFirst, squadIterEnd;
		squadIter = squadIterFirst = ssfTeamReportMap.begin();
		squadIterEnd = ssfTeamReportMap.end();

		// set the first squad in the squad report map as winning squad
		mWinEntityId = squadIterFirst->first;
		mLossEntityId = squadIterFirst->first;

		mGameFinish = true;
		for (; squadIter != squadIterEnd; ++squadIter)
		{
			int64_t teamEntityId = squadIter->first;
			SSFSeasonsReportBase::SSFTeamReport& squadReportDetails = *squadIter->second;

			bool didSquadFinish = (0 == squadReportDetails.getSquadDisc()) ? true : false;
			int32_t squadScore = static_cast<int32_t>(squadReportDetails.getScore());
			TRACE_LOG("[SSFGameResultHelper:" << this << "].DetermineGameResult(): Team Entity Id:" << teamEntityId << " score:" << squadScore << " didSquadFinish:" << didSquadFinish << " ");

			if (true == didSquadFinish && squadScore > mHighestScore)
			{
				// update the winning team entity id and highest squad score
				mWinEntityId = teamEntityId;
				mHighestScore = squadScore;
			}
			if (true == didSquadFinish && squadScore < mLowestScore)
			{
				// update the losing team entity id and lowest squad score
				mLossEntityId = teamEntityId;
				mLowestScore = squadScore;
			}

			// if some of the scores are different, it isn't a tie.
			if (squadIter != squadIterFirst && squadScore != prevSquadScore)
			{
				((DERIVED*)this)->getTieGame() = false;
			}

			prevSquadScore = squadScore;
		}

		squadIter = squadIterFirst = ssfTeamReportMap.begin();
		for (; squadIter != squadIterEnd; ++squadIter)
		{
			SSFSeasonsReportBase::SSFTeamReport& squadReportDetails = *squadIter->second;
			if (1 == squadReportDetails.getSquadDisc())
			{
				mGameFinish = false;
				mLossEntityId = squadIter->first;
			}
		}

		AdjustSquadScore();

		//determine who the winners and losers are and write them into the report for stat update
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
		playerIter = osdkPlayerReportsMap.begin();
		playerEnd = osdkPlayerReportsMap.end();

		for (; playerIter != playerEnd; ++playerIter)
		{
			GameManager::PlayerId playerId = playerIter->first;
			OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
			SSFSeasonsReportBase::SSFSeasonsPlayerReport& ssfSeasonsPlayerReport = static_cast<SSFSeasonsReportBase::SSFSeasonsPlayerReport&>(*playerReport.getCustomPlayerReport());

			int64_t entityTeamId = ssfSeasonsPlayerReport.getTeamEntityId();
			bool winner = (entityTeamId == mWinEntityId);

			if (((DERIVED*)this)->getTieGame())
			{
				TRACE_LOG("[SSFGameResultHelper:" << this << "].DetermineGameResult(): tie set - Player Id:" << playerId << " ");
				((DERIVED*)this)->getTieSet().insert(playerId);
			}
			else if (winner)
			{
				TRACE_LOG("[SSFGameResultHelper:" << this << "].DetermineGameResult(): winner set - Player Id:" << playerId << " ");
				((DERIVED*)this)->getWinnerSet().insert(playerId);
			}
			else
			{
				TRACE_LOG("[SSFGameResultHelper:" << this << "].DetermineGameResult(): loser set - Player Id:" << playerId << " ");
				((DERIVED*)this)->getLoserSet().insert(playerId);
			}
		}
	}
	SSFSeasonsReportBase::SsfMatchEndResult DetermineSSFUserResult(GameManager::PlayerId playerId, const OSDKGameReportBase::OSDKPlayerReport& playerReport, bool isSoloMatch)
	{
		SSFSeasonsReportBase::SsfMatchEndResult retValue = SSFSeasonsReportBase::SSF_END_INVALID;
		uint32_t playerCalculatedResult = playerReport.getGameResult();
		uint32_t userReportedResult = playerReport.getUserResult();
		OSDKGameReportBase::IntegerAttributeKey key("user_end_subreason");
		OSDKGameReportBase::IntegerAttributeMap::const_iterator itr = playerReport.getPrivatePlayerReport().getPrivateIntAttributeMap().find(key);
		SSFSeasonsReportBase::SsfUserEndSubReason userReportedSubResult = SSFSeasonsReportBase::SSF_USER_RESULT_NONE;
		if (nullptr != itr)
		{
			userReportedSubResult = static_cast<SSFSeasonsReportBase::SsfUserEndSubReason>(itr->second);
		}
		else
		{
			WARN_LOG("[SSFGameResultHelper:" << this << "].DetermineSSFUserResult() could not find the key[" << key.c_str() << "] in the OSDKPrivatePlayerReport.mPrivateIntAttributeMap, so userReportedSubResult is defaulted to SSFSeasonsReportBase::SSF_USER_RESULT_NONE");
		}
		if (0 == mGameTime)
        {
			retValue = SSFSeasonsReportBase::SSF_END_NO_CONTEST;

			if (userReportedResult == GAME_RESULT_COMPLETE_REGULATION ||
				(userReportedResult == GAME_RESULT_OTHER && userReportedSubResult == SSFSeasonsReportBase::SSF_USER_RESULT_NOCONTEST)
			   )
            {
                retValue = SSFSeasonsReportBase::SSF_END_NO_CONTEST;
            }
			else if (userReportedResult == GAME_RESULT_QUITANDCOUNT)
			{
				retValue = SSFSeasonsReportBase::SSF_END_QUIT;
			}
		}
        else if (mGameFinish)
		{
			//check if the player quit before match ended
			if (userReportedResult == GAME_RESULT_QUITANDCOUNT || userReportedResult == GAME_RESULT_DISCONNECTANDCOUNT)
			{
				retValue = SSFSeasonsReportBase::SSF_END_QUIT;
			}
			else
			{
				if (playerCalculatedResult == WLT_WIN)
				{
					retValue = SSFSeasonsReportBase::SSF_END_WIN;
				}
				else if (playerCalculatedResult == WLT_LOSS)
				{
					retValue = SSFSeasonsReportBase::SSF_END_LOSS;
				}
				else if (playerCalculatedResult == WLT_TIE)
				{
					retValue = SSFSeasonsReportBase::SSF_END_DRAW;
				}
				else
				{
					// if any player did not finish/quit and the game still counts as finished the gameresult will have been modified by GameCommonReportSlave::updateWinPlayerStats and WLT_OPPOQCTAG OR'd to the result
					uint32_t discGameResultWin = WLT_WIN;
					uint32_t discGameResultLoss = WLT_LOSS;
					uint32_t discGameResultTie = WLT_TIE;
					switch (userReportedResult)
					{
					case GAME_RESULT_DNF_DISCONNECT:
					case GAME_RESULT_FRIENDLY_DISCONNECT:
					case GAME_RESULT_CONCEDE_DISCONNECT:
					case GAME_RESULT_MERCY_DISCONNECT:
					case GAME_RESULT_DISCONNECTANDCOUNT:
					{
						discGameResultWin |= WLT_DISC;
						discGameResultLoss |= WLT_DISC;
						discGameResultTie |= WLT_DISC;
					}
					break;

					case GAME_RESULT_DNF_QUIT:
					case GAME_RESULT_FRIENDLY_QUIT:
					case GAME_RESULT_CONCEDE_QUIT:
					case GAME_RESULT_MERCY_QUIT:
					case GAME_RESULT_QUITANDCOUNT:
					{
						discGameResultWin |= WLT_CONCEDE;
						discGameResultLoss |= WLT_CONCEDE;
						discGameResultTie |= WLT_CONCEDE;
					}
					break;

					default:
						break;
					}
					discGameResultWin |= WLT_OPPOQCTAG;

					if (playerCalculatedResult == discGameResultWin)
					{
						retValue = SSFSeasonsReportBase::SSF_END_WIN;
					}
					else if (playerCalculatedResult == discGameResultLoss)
					{
						retValue = SSFSeasonsReportBase::SSF_END_LOSS;
					}
					else if (playerCalculatedResult == discGameResultTie)
					{
						retValue = SSFSeasonsReportBase::SSF_END_DRAW;
					}

				}
			}
		}
		else if (isSoloMatch)
		{
			retValue = SSFSeasonsReportBase::SSF_END_QUIT;
			if (GAME_RESULT_QUITANDCOUNT != userReportedResult)
			{
				WARN_LOG("[SSFGameResultHelper:" << this << "].DetermineSSFUserResult() isSoloMatch but userReportedResult[" << userReportedResult << "] is not GAME_RESULT_QUITANDCOUNT[" << GAME_RESULT_QUITANDCOUNT << "]");
			}
		}
		else
		{
			if (userReportedResult == GAME_RESULT_DESYNCED)
			{
				retValue = SSFSeasonsReportBase::SSF_END_NO_CONTEST;
			}
			else if (userReportedResult == GAME_RESULT_FRIENDLY_DISCONNECT ||
				userReportedResult == GAME_RESULT_MERCY_DISCONNECT)
			{
				retValue = SSFSeasonsReportBase::SSF_END_DNF;
			}
			else if (userReportedResult == GAME_RESULT_FRIENDLY_QUIT ||
				userReportedResult == GAME_RESULT_MERCY_QUIT ||
				userReportedSubResult == SSFSeasonsReportBase::SSF_USER_RESULT_QUIT)
			{
				retValue = SSFSeasonsReportBase::SSF_END_QUIT;
			}
			else if (playerCalculatedResult & WLT_WIN)
			{
				retValue = SSFSeasonsReportBase::SSF_END_DNF_WIN;
			}
			else if (playerCalculatedResult & WLT_LOSS)
			{
				if (userReportedSubResult == SSFSeasonsReportBase::SSF_USER_RESULT_IDLE)
				{
					retValue = SSFSeasonsReportBase::SSF_END_DNF_AFK;
				}
				else if (userReportedSubResult == SSFSeasonsReportBase::SSF_USER_RESULT_OWNGOALS)
				{
					retValue = SSFSeasonsReportBase::SSF_END_DNF_OG;
				}
				else if (userReportedSubResult == SSFSeasonsReportBase::SSF_USER_RESULT_CONSTRAINED)
				{
					retValue = SSFSeasonsReportBase::SSF_END_DNF_CONSTRAINED;
				}
				else
				{
					retValue = SSFSeasonsReportBase::SSF_END_DNF_LOSS;
				}
			}
			else if (playerCalculatedResult & WLT_TIE)
			{
				retValue = SSFSeasonsReportBase::SSF_END_DNF_DRAW;
			}
		}
        TRACE_LOG("[SSFGameResultHelper:" << this << "].DetermineSSFUserResult() for player[" << playerId
            << "] retValue[" << retValue
            << "] playerCalculatedResult[" << playerCalculatedResult
            << "] userReportedResult[" << userReportedResult
            << "] mGameTime[" << mGameTime << "]");
        return retValue;
	}
	bool updateSquadResults()
	{
		bool success = true;

		OSDKGameReportBase::OSDKReport& osdkReport = ((DERIVED*)this)->getOsdkReport();
		SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
		SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();

		SSFSeasonsReportBase::SSFTeamReportMap::const_iterator squadIter, squadIterEnd;
		squadIter = ssfTeamReportMap.begin();
		squadIterEnd = ssfTeamReportMap.end();

		for (int32_t i = 0; squadIter != squadIterEnd; ++squadIter, i++)
		{
			int64_t teamEntityId = squadIter->first;
			SSFSeasonsReportBase::SSFTeamReport& squadReportDetails = *squadIter->second;

			//TODO - SSF Seasons - what stats we want to log?

			uint32_t resultValue = 0;
			bool didSquadFinish = (squadReportDetails.getSquadDisc() == 0) ? true : false;

			if (true == ((DERIVED*)this)->getTieGame())
			{
				squadReportDetails.setWins(0);
				squadReportDetails.setLosses(0);
				squadReportDetails.setTies(1);

				resultValue |= WLT_TIE;
			}
			else if (mWinEntityId == teamEntityId)
			{
				squadReportDetails.setWins(1);
				squadReportDetails.setLosses(0);
				squadReportDetails.setTies(0);

				resultValue |= WLT_WIN;
			}
			else
			{
				squadReportDetails.setWins(0);
				squadReportDetails.setLosses(1);
				squadReportDetails.setTies(0);

				resultValue |= WLT_LOSS;
			}

			if (false == didSquadFinish)
			{
				resultValue |= WLT_DISC;
			}

			// Track the win-by-dnf
			if ((mWinEntityId == teamEntityId) && (false == mGameFinish))
			{
				squadReportDetails.setWinnerByDnf(1);
				resultValue |= WLT_OPPOQCTAG;
			}
			else
			{
				squadReportDetails.setWinnerByDnf(0);
			}

			// Store the result value in the map
			squadReportDetails.setGameResult(resultValue);  // STATNAME_RESULT
		}

		TRACE_LOG("[SSFGameResultHelper].updateSquadResults() Processing complete");

		return success;
	}

	int64_t				mWinEntityId;
	int64_t				mLossEntityId;
	bool				mGameFinish;
	int32_t				mLowestScore;
	int32_t				mHighestScore;
    uint32_t            mGameTime;

private:
	void AdjustSquadScore()
	{
		OSDKGameReportBase::OSDKReport& osdkReport = ((DERIVED*)this)->getOsdkReport();
		SSFSeasonsReportBase::SSFTeamSummaryReport* ssfTeamSummaryReport = static_cast<SSFSeasonsReportBase::SSFTeamSummaryReport*>(osdkReport.getTeamReports());
		SSFSeasonsReportBase::SSFTeamReportMap& ssfTeamReportMap = ssfTeamSummaryReport->getSSFTeamReportMap();


		SSFSeasonsReportBase::SSFTeamReportMap::const_iterator squadIter, squadIterEnd;
		squadIter = ssfTeamReportMap.begin();
		squadIterEnd = ssfTeamReportMap.end();

		bool zeroScore = false;

		TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore()");

		for (; squadIter != squadIterEnd; ++squadIter)
		{
			SSFSeasonsReportBase::SSFTeamReport& squadReportDetails = *squadIter->second;

			bool didSquadFinish = (0 == squadReportDetails.getSquadDisc()) ? true : false;
			TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - didSquadFinish: " << didSquadFinish << " ");
			if (false == didSquadFinish)
			{
				int32_t squadScore = static_cast<int32_t>(squadReportDetails.getScore());
				TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - squadScore: " << squadScore << " Lowest Score: " << mLowestScore << " ");

				if (mLowestScore != INVALID_HIGH_SCORE && ((squadScore >= mLowestScore) ||
					(mLowestScore - squadScore) < DISC_SCORE_MIN_DIFF))
				{
					((DERIVED*)this)->getTieGame() = false;
					if (mLowestScore - DISC_SCORE_MIN_DIFF <= 0)
					{
						// new score for cheaters or quitters is 0
						squadScore = 0;
						zeroScore = true;
						TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - zeroScore - setting squadScore to 0, zeroScore to true" << " ");
					}
					else
					{
						squadScore = mLowestScore - DISC_SCORE_MIN_DIFF;
						TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - zeroScore - setting squadScore to Lowest Score - DISC_SCORE_MIN_DIFF" << " ");
					}

					TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - setting squadScore: " << squadScore << " ");
					squadReportDetails.setScore(static_cast<uint32_t>(squadScore));
				}
			}
		}

		// need adjustment on scores of non-disconnected
		TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - zeroScore: " << zeroScore << " ");
		if (zeroScore)
		{
			squadIter = ssfTeamReportMap.begin();
			squadIterEnd = ssfTeamReportMap.end();

			for (; squadIter != squadIterEnd; ++squadIter)
			{
				SSFSeasonsReportBase::SSFTeamReport& squadReportDetails = *squadIter->second;

				bool didSquadFinish = (0 == squadReportDetails.getSquadDisc()) ? true : false;
				TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - zeroScore - didSquadFinish: " << didSquadFinish << " ");
				if (true == didSquadFinish)
				{
					int32_t squadScore = static_cast<int32_t>(squadReportDetails.getScore());
					TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - zeroScore - squadScore: " << squadScore << " ");

					if (squadScore < DISC_SCORE_MIN_DIFF)
					{
						TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - zeroScore - setting squadScore to DISC_SCORE_MIN_DIFF" << " ");
						squadScore = DISC_SCORE_MIN_DIFF;
					}

					if (squadScore > mHighestScore)
					{
						TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - zeroScore - setting mHighestScore to squadScore" << " ");
						mHighestScore = squadScore;
					}

					squadReportDetails.setScore(static_cast<int32_t>(squadScore));
					TRACE_LOG("[SSFGameResultHelper:" << this << "].adjustSquadScore() - zeroScore - setting squadScore: " << squadScore << " ");
				}
			}
		}
	}
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_SSFGAMERESULTHELPER_H

