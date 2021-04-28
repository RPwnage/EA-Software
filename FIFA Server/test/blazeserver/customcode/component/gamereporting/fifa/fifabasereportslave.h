/*************************************************************************************************/
/*!
    \file   fifabasereportslave.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFABASEREPORT_SLAVE_H
#define BLAZE_CUSTOM_FIFABASEREPORT_SLAVE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

/*!
    class FifaBaseReportSlave
*/
template<typename entity_report_map, typename entity_report, typename common_game_report, typename common_player_report>
class FifaBaseReportSlave
{
public:
	FifaBaseReportSlave() {}
	virtual ~FifaBaseReportSlave() {}

	virtual void determineGameResult() = 0;

protected:
	virtual common_game_report*			getGameReport() = 0;
	virtual entity_report_map*			getEntityReportMap() = 0;
	virtual common_player_report*		getCommonPlayerReport(entity_report& entityReport) = 0;
	virtual bool						didEntityFinish(int64_t entityId, entity_report& entityReport) = 0;	
	virtual	entity_report*				findEntityReport(int64_t entityId) = 0;

	virtual int64_t						getWinningEntity() = 0;
	virtual int64_t						getLosingEntity() = 0;
	virtual bool						isTieGame() = 0;
	virtual bool						isEntityWinner(int64_t entityId) = 0;
	virtual bool						isEntityLoser(int64_t entityId) = 0;

	virtual int32_t						getHighestEntityScore() = 0;
	virtual int32_t						getLowestEntityScore() = 0;

	virtual void						setCustomEntityResult(entity_report& entityReport, StatGameResultE gameResult) {}
	virtual void						setCustomEntityScore(entity_report& entityReport, uint16_t score, uint16_t goalsFor, uint16_t goalsAgainst) {}


	static const int MAX_REDCARD_PER_GAME = 5;

	bool initGameStatus()
	{
		bool success = true;

		common_game_report* fifaCommonGameReport = getGameReport();
		if (fifaCommonGameReport != NULL)
		{
			mGameWentToPenalties = (fifaCommonGameReport->getWentToPk() == 1);
		}
		else
		{
			TRACE_LOG("[FifaBaseReportSlave:" << this << "].initGameStatus() - fifaCommonGameReport is NULL");
			success = false;
		}

		//determine if this was a red card game and if the game was a DNF game
		entity_report_map* entityReportMap = getEntityReportMap();
		typename entity_report_map::const_iterator entityIter, entityFirst, entityEnd;

		entityIter = entityFirst = entityReportMap->begin();
		entityEnd = entityReportMap->end();

		mRedCardRuleViolation = false;
		mGameDNF = false;

		mHighestPenaltyScore = INT32_MIN;
		mLowestPenaltyScore = INT32_MAX;

		for (; entityIter != entityEnd; ++entityIter)
		{
			common_player_report* commonPlayerReport = getCommonPlayerReport(*entityIter->second);
			if (commonPlayerReport != nullptr)
			{
				if (MAX_REDCARD_PER_GAME <= commonPlayerReport->getRedCard())
				{
					mRedCardRuleViolation = true;
				}

				bool entityFinished = didEntityFinish(entityIter->first, *entityIter->second);
				if (entityFinished)
				{
					uint16_t pkGoals = commonPlayerReport->getPkGoals();
					if (pkGoals > mHighestPenaltyScore)
					{
						mHighestPenaltyScore = pkGoals;
					}
					if (commonPlayerReport->getPkGoals() < mLowestPenaltyScore)
					{
						mLowestPenaltyScore = pkGoals;
					}
				}

				mGameDNF = !entityFinished || mGameDNF;
			}
			else
			{
				success = false;
			}
		}

		TRACE_LOG("[FifaBaseReportSlave:" << this << "].initGameStatus() - wentToPk:" << mGameWentToPenalties << " gameDNF:" << mGameDNF << " mRedCardRuleViolation:" << mRedCardRuleViolation << " success:" << success);
		return success;
	}

	void adjustRedCardedPlayers()
	{
		if(mRedCardRuleViolation)
		{
			TRACE_LOG("[FifaBaseReportSlave:" << this << "].adjustRedCardedPlayers() - game has been flagged as a redcard violation game adjust player user result");

			entity_report_map* entityReportMap = getEntityReportMap();
			typename entity_report_map::const_iterator entityIter, entityEnd;
			entityIter = entityReportMap->begin();
			entityEnd = entityReportMap->end();

			for (; entityIter != entityEnd; ++entityIter)
			{
				entity_report& entityReport = *entityIter->second;
				common_player_report* commonPlayerReport = getCommonPlayerReport(entityReport);
				StatGameResultE gameResult = GAME_RESULT_COMPLETE_REGULATION;
				if(MAX_REDCARD_PER_GAME <= commonPlayerReport->getRedCard())
				{
					gameResult = GAME_RESULT_CONCEDE_QUIT;
					TRACE_LOG("[FifaBaseReportSlave:" << this << "].adjustRedCardedPlayers() - entityId " << entityIter->first << " violated redcard rule setting userResult to " << gameResult);
				}

				setCustomEntityResult(entityReport, gameResult);
			}
		}
	}

	void adjustPlayerResult()
	{
		entity_report_map* entityReportMap = getEntityReportMap();
		typename entity_report_map::const_iterator entityIter, entityEnd;
		entityIter = entityReportMap->begin();
		entityEnd = entityReportMap->end();

		uint16_t winGoals, lossGoals, winScore, lossScore, userGoalsFor, userGoalsAgainst, userScore;

		if (!mRedCardRuleViolation && !mGameDNF)
		{
			winGoals = static_cast<uint16_t>(getHighestEntityScore() - mHighestPenaltyScore);
			lossGoals = static_cast<uint16_t>(getLowestEntityScore() - mLowestPenaltyScore);

			winScore = mGameWentToPenalties? winGoals + 1 : winGoals;
			lossScore = lossGoals;
		}
		else
		{
			entity_report* winningEntity = findEntityReport(getWinningEntity());
			entity_report* losingEntity = findEntityReport(getLosingEntity());

			winScore = winGoals = lossScore = lossGoals = 0;
			if (winningEntity != NULL)
			{
				winScore = winGoals = static_cast<uint16_t>(winningEntity->getScore());
			}

			if (losingEntity != NULL)
			{
				lossScore = lossGoals = static_cast<uint16_t>(losingEntity->getScore());
			}
		}

		TRACE_LOG("[FifaBaseReportSlave:" << this << "].adjustPlayerGoals() - mHighestPlayerScore:" << getHighestEntityScore() <<
			" mHighestPenaltyScore:" << mHighestPenaltyScore <<
			" mLowestPlayerScore:" << getLowestEntityScore() <<
			" mLowestPenaltyScore:" << mLowestPenaltyScore <<
			" winScore:" << winScore <<
			" lossScore:" << lossScore);

		int64_t entityId = 0;
		userGoalsFor = userGoalsAgainst = userScore = 0;
		for (; entityIter != entityEnd; ++entityIter)
		{
			entityId = entityIter->first;
			entity_report& entityReport = *entityIter->second;
			common_player_report* fifaCommonPlayerReport = getCommonPlayerReport(entityReport);
			
			if (isTieGame())
			{
				userGoalsFor = winGoals;
				userGoalsAgainst = lossGoals;
				userScore = winScore;	
			}
			else
			{
				if (isEntityWinner(entityId))
				{
					userGoalsFor = winGoals;
					userGoalsAgainst = lossGoals;
					userScore = winScore;
				}
				else if (isEntityLoser(entityId))
				{
					userGoalsFor = lossGoals;
					userGoalsAgainst = winGoals;
					userScore = lossScore;
				}
			}

			TRACE_LOG("[FifaBaseReportSlave:" << this << "].adjustPlayerGoals() - player:" << entityId <<
				" Goals:" << userGoalsFor << " (prev: " <<  fifaCommonPlayerReport->getGoals() << ")" <<
				" Goals Against:" << userGoalsAgainst << " (prev: " <<  fifaCommonPlayerReport->getGoalsConceded() << ")" <<
				" Score:" << userScore << " (prev: " <<  entityReport.getScore() << ")");

			fifaCommonPlayerReport->setGoals(userGoalsFor);
			fifaCommonPlayerReport->setGoalsConceded(userGoalsAgainst);
			entityReport.setScore(userScore);

			setCustomEntityScore(entityReport, userScore, userGoalsFor, userGoalsAgainst); 
		} 
	}

	void updateUnadjustedScore()
	{
		mUnadjustedHighScore = INT32_MIN;
		mUnadjustedLowScore = INT32_MAX;

		entity_report_map* entityReportMap = getEntityReportMap();
		typename entity_report_map::const_iterator entityIter, entityEnd;
		entityIter = entityReportMap->begin();
		entityEnd = entityReportMap->end();


		int32_t entityScore = 0; 
		for (; entityIter != entityEnd; ++entityIter)
		{
			entity_report& entityReport = *entityIter->second;
			entityScore = static_cast<int32_t>(entityReport.getScore());
			TRACE_LOG("[FifaBaseReportSlave:" << this << "].cachePlayerScores(): entity Id:" << entityIter->first << " score:" << entityScore << " ");

			//grab the common stats report and update the unadjusted score
			common_player_report* fifaCommonPlayerReport = getCommonPlayerReport(entityReport);
			fifaCommonPlayerReport->setUnadjustedScore(static_cast<uint16_t>(entityScore));

			//track the high and low unadjusted score
			if (entityScore > mUnadjustedHighScore)
			{
				// update the highest score
				mUnadjustedHighScore = entityScore;
			}
			if (entityScore < mUnadjustedLowScore)
			{
				// update the lowest player score
				mUnadjustedLowScore = entityScore;
			}
		}

		TRACE_LOG("[FifaBaseReportSlave:" << this << "].updateUnadjustedScore(): mUnadjustedHighScore:" << mUnadjustedHighScore << " mUnadjustedLowScore:" << mUnadjustedLowScore << " ");
	}


	int32_t							mUnadjustedHighScore;
	int32_t							mUnadjustedLowScore;

	int32_t                         mHighestPenaltyScore;
	int32_t                         mLowestPenaltyScore;

	bool							mRedCardRuleViolation;
	bool							mGameDNF;
	bool							mGameWentToPenalties;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFABASEREPORT_SLAVE_H

