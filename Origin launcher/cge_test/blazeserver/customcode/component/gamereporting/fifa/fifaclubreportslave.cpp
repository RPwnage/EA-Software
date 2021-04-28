/*************************************************************************************************/
/*!
    \file   fifaclubreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifaclubreportslave.h"

namespace Blaze
{

namespace GameReporting
{

namespace FIFA
{

/*************************************************************************************************/
/*! \brief FifaClubReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaClubReportSlave::FifaClubReportSlave(GameReportingSlaveImpl& component) :
FifaClubBaseReportSlave(component)
{
	mFifaSeasonalPlay.setExtension(&mFifaSeasonalPlayExtension);
	mEloRpgHybridSkill.setExtension(&mEloRpgHybridSkillExtension);
	mFifaClubsTagUpdater.setExtension(&mFifaSeasonalPlayExtension);
}
/*************************************************************************************************/
/*! \brief FifaClubReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaClubReportSlave::~FifaClubReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaClubReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaClubReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaClubReportSlave") FifaClubReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomClubGameTypeName
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
const char8_t* FifaClubReportSlave::getCustomClubGameTypeName() const
{
    return "gameType9";
}

/*************************************************************************************************/
/*! \brief updateCustomClubRecords
    A custom hook for game team to process for club records based on game report and cache the 
        result in mNewRecordArray, called during updateClubStats()

    \return bool - true, if club record update process is success
*/
/*************************************************************************************************/
bool FifaClubReportSlave::updateCustomClubRecords(int32_t iClubIndex, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap)
{
	// Data validation
	if ( iClubIndex >= MAX_CLUB_NUMBER )
	{
		WARN_LOG("[FifaClubReportSlave:" << this << "].updateCustomClubRecords() - iClubIndex[" << iClubIndex << "] >= MAX_CLUB_NUMBER[" << MAX_CLUB_NUMBER << "]");
		return false;
	}

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();
    
    for(; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());
        FifaClubReportBase::FifaClubsPlayerReport& clubCustomPlayerReport = static_cast<FifaClubReportBase::FifaClubsPlayerReport&>(*clubPlayerReport.getCustomClubPlayerReport());

        int32_t statPoints[CLUB_RECORD_MOST_SHOTS_SINGLE_GAME];
        memset(statPoints, 0, sizeof(statPoints));

        // This section updates the single game stat for the all time stat that is done in processUpdatedStatsCustom
        statPoints[CLUB_RECORD_MOST_GOALS_SINGLE_GAME - 1] = clubCustomPlayerReport.getCommonPlayerReport().getGoals();
        statPoints[CLUB_RECORD_MOST_SHOTS_SINGLE_GAME - 1] = clubCustomPlayerReport.getCommonPlayerReport().getShots();

		// TODO: Consider refactoring this. Wasting CPU on needless loop
		// See also: GameCustomClubReportSlave::updateCustomClubRecords
        int32_t iIndexRecords;
        for (iIndexRecords = 0; iIndexRecords < NUMBER_OF_TROPHIES; iIndexRecords++ )
        {
            if((iIndexRecords == CLUB_RECORD_MOST_GOALS_SINGLE_GAME - 1) || 
                (iIndexRecords == CLUB_RECORD_MOST_SHOTS_SINGLE_GAME - 1))
            {
                mNewRecordArray[iClubIndex][iIndexRecords].bNewRecord = mNewRecordArray[iClubIndex][iIndexRecords].bRecordExists ?
                    (statPoints[iIndexRecords] > mNewRecordArray[iClubIndex][iIndexRecords].currentRecordStat) : true;
                if (mNewRecordArray[iClubIndex][iIndexRecords].bNewRecord)
                {
                    mNewRecordArray[iClubIndex][iIndexRecords].bUpdateRecord = true;
                    mNewRecordArray[iClubIndex][iIndexRecords].uRecordHolder = playerIter->first;
                    mNewRecordArray[iClubIndex][iIndexRecords].currentRecordStat = statPoints[iIndexRecords];
                    mNewRecordArray[iClubIndex][iIndexRecords].bRecordExists = true;
                }
            }
        }
    }
    return true;
}

void FifaClubReportSlave::updateCustomGameStats()
{
	ClubsLastOpponent::ExtensionData data;
	data.mProcessedReport = mProcessedReport;

	mClubsLastOpponentExtension.setExtensionData( &data );
	mClubsLastOpponentExtension.UpdateEntityFilterList(mEntityFilterList);
	mFifaLastOpponent.setExtension( &mClubsLastOpponentExtension );
	mFifaLastOpponent.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
}

/*************************************************************************************************/
/*! \brief updateCustomClubAwards
    A custom hook for game team to give club awards based on game report, called during updateClubStats()

    \return bool - true, if club award update process is success
*/
/*************************************************************************************************/
bool FifaClubReportSlave::updateCustomClubAwards(Clubs::ClubId clubId,  OSDKClubGameReportBase::OSDKClubClubReport& clubReport, OSDKGameReportBase::OSDKReport& OsdkReport)
{
    bool bSuccess = true;

    //**************** Give award with for simply taking place in the event **************/
    //// so each club which finishes the game is awarded
    //bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(clubId, CLUB_AWARD_PLAY_GAME) : bSuccess;

    //***************** Give award for playing more than once in the last hour *****************/
    //GameHistoryUtil gameHistoryUtil(mComponent);
    //GameReportsList gameReportsList;
    //QueryVarValuesList queryVarValues;

    //char8_t strClubId[32];
    //blaze_snzprintf(strClubId, sizeof(strClubId), "%d", clubId);
    //queryVarValues.push_back(strClubId);
    //if (gameHistoryUtil.getGameHistory("club_games_in_last_hour_query", 1, queryVarValues, gameReportsList))
    //{
    //    if (gameReportsList.getGameReportList().size() > 0)
    //    {
    //        bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(clubId, CLUB_AWARD_QUICK_SUCCESSION) : bSuccess; 
    //    }
    //}
    //gameReportsList.getGameReportList().release();

    //************** Give award for playing with 10 club members *************/
    //OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    //OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    //playerIter = OsdkPlayerReportsMap.begin();
    //playerEnd = OsdkPlayerReportsMap.end();
    //uint16_t uMemberCount = 0;
    //for(; playerIter != playerEnd; ++playerIter)
    //{
    //    uMemberCount++;
    //}
    //if(10 <= uMemberCount)
    //{
    //    bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(mWinClubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_10_PLAYERS)) : bSuccess; 
    //}

    /***************** Give award for scoring more than 5 points in a game  ********************/
    if(5 < static_cast<int32_t>(clubReport.getScore()))
    {
		TRACE_LOG("[FifaClubReportSlave:" << this << "].updateCustomClubAwards()award score 5(" << clubReport.getScore() << ") for club " << clubId << " ");
        bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(clubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_SCORE_5)) : bSuccess; 
    }

    /**************** Awards only for winner ************************/
    if(static_cast<Clubs::ClubId>(clubId) == mWinClubId)
    {
        // award the winner the awardID 3 for winning a game 
		TRACE_LOG("[FifaClubReportSlave:" << this << "].updateCustomClubAwards()award win a game(" << mWinClubId << ") for club " << clubId << " ");
        bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(mWinClubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_WIN_GAME)) : bSuccess; 

        //// award shutout
        //if(0 == mLowestClubScore)
        //{
        //    bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(mWinClubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_SHUTOUT)) : bSuccess; 
        //}

        // beat by 5
        if(5 <= mHighestClubScore - mLowestClubScore)
        {
			TRACE_LOG("[FifaClubReportSlave:" << this << "].updateCustomClubAwards()award beat by 5(" << mHighestClubScore - mLowestClubScore << ") for club " << clubId << " ");
            bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(mWinClubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_BEAT_BY_5)) : bSuccess; 
        }
    }
    return bSuccess;
}

/*************************************************************************************************/
/*! \brief processCustomUpdatedStats
    Perform custom process for post stats update, which include the club awards and record updates
        that should happen after stats for this game report is processed

    \return bool - true, if process is done successfully
*/
/*************************************************************************************************/
bool FifaClubReportSlave::processCustomUpdatedStats()
{

	bool bSuccess = true;

	// COMMENTED OUT TO FIX assert from getValueInt

    /***************** Give win streak awards to the winning club  ********************/
    // Obtain the win streak of the winning club
	//OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());

	//OSDKClubGameReportBase::OSDKClubReport* clubReports = reinterpret_cast<OSDKClubGameReportBase::OSDKClubReport*>(osdkReport.getTeamReports());
	//OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& clubReportsMap = clubReports->getClubReports();

	//OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubsIter, clubsEnd;
	//clubsIter = clubReportsMap.begin();
	//clubsEnd = clubReportsMap.end();
	//for(; clubsIter != clubsEnd; ++clubsIter)
	//{
	//	Clubs::ClubId currentClubId = clubsIter->first;
	//	Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(CATEGORYNAME_CLUB_RANKING, currentClubId);
	//	if (NULL != key)
	//	{
	//		if (currentClubId == mWinClubId)
	//		{
	//			//WINSTREAK
	//			int64_t iWinStreak = mUpdateStatsHelper.getValueInt(key, STATNAME_WSTREAK, Stats::STAT_PERIOD_ALL_TIME, false);
	//			if (3 == iWinStreak)
	//			{
	//				TRACE_LOG("[FifaClubReportSlave:" << this << "].processCustomUpdatedStats()award winstreak 3(" << iWinStreak << ") for club " << currentClubId << " ");
	//				bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
	//					static_cast<Blaze::Clubs::ClubId>(currentClubId), 
	//					static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_WIN_3_IN_ROW)) : bSuccess; 
	//			}
	//			else if (5 == iWinStreak)
	//			{
	//				TRACE_LOG("[FifaClubReportSlave:" << this << "].processCustomUpdatedStats()award winstreak 5(" << iWinStreak << ") for club " << currentClubId << " ");
	//				bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
	//					static_cast<Blaze::Clubs::ClubId>(currentClubId), 
	//					static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_WIN_5_IN_ROW)) : bSuccess; 
	//			}
	//			else if (10 == iWinStreak)
	//			{
	//				TRACE_LOG("[FifaClubReportSlave:" << this << "].processCustomUpdatedStats()award winstreak 10(" << iWinStreak << ") for club " << currentClubId << " ");
	//				bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
	//					static_cast<Blaze::Clubs::ClubId>(currentClubId), 
	//					static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_WIN_10_IN_ROW)) : bSuccess; 
	//			}

	//			//WINS
	//			int64_t iWins = mUpdateStatsHelper.getValueInt(key, STATNAME_WINS, Stats::STAT_PERIOD_ALL_TIME, false);
	//			if (25 == iWins)
	//			{
	//				TRACE_LOG("[FifaClubReportSlave:" << this << "].processCustomUpdatedStats()award win 25(" << iWins << ") for club " << currentClubId << " ");
	//				bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
	//					static_cast<Blaze::Clubs::ClubId>(currentClubId), 
	//					static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_WIN_25_GAMES)) : bSuccess; 
	//			}
	//			else if (50 == iWins)
	//			{
	//				TRACE_LOG("[FifaClubReportSlave:" << this << "].processCustomUpdatedStats()award win 50(" << iWins << ") for club " << currentClubId << " ");
	//				bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
	//					static_cast<Blaze::Clubs::ClubId>(currentClubId), 
	//					static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_WIN_50_GAMES)) : bSuccess; 
	//			}
	//		}

	//		//GAMES PLAYED
	//		int64_t iGamesPlayed = mUpdateStatsHelper.getValueInt(key, STATNAME_GAMES_PLAYED, Stats::STAT_PERIOD_ALL_TIME, false);
	//		if (100 == iGamesPlayed)
	//		{
	//			TRACE_LOG("[FifaClubReportSlave:" << this << "].processCustomUpdatedStats()award gameplayed 100(" << iGamesPlayed << ") for club " << currentClubId << " ");
	//			bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
	//				static_cast<Blaze::Clubs::ClubId>(currentClubId), 
	//				static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_PLAY_100_GAMES)) : bSuccess; 
	//		}

			//CLEAN SHEETS
	//		int64_t cleansheets = mUpdateStatsHelper.getValueInt(key, STATNAME_CLEANSHEETS, Stats::STAT_PERIOD_ALL_TIME, false);
	//		if (cleansheets == 10)
	//		{
	//			TRACE_LOG("[FifaClubReportSlave:" << this << "].processCustomUpdatedStats()award cleansheets 10(" << cleansheets << ") for club " << currentClubId << " ");
	//			bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
	//				static_cast<Blaze::Clubs::ClubId>(currentClubId),
	//				static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_10_CLEANSHEETS)) : bSuccess; 
	//		}
	//	}
	//}

    /******************** Update for Club Records  ************************/
    // Obtain the stats for each player in the player report map
    //OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
    //OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    //OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerFirst, playerEnd;
    //playerIter = playerFirst = OsdkPlayerReportsMap.begin();
    //playerEnd = OsdkPlayerReportsMap.end();
    //Stats::UpdateRowKey* playerKey = NULL;

    //for (; playerIter != playerEnd; ++playerIter)
    //{
     //   GameManager::PlayerId playerId = playerIter->first;
     //   playerKey = mBuilder.getUpdateRowKey(CATEGORYNAME_CLUB_MEMBERS, playerId);
     //   if (NULL != playerKey)
    //    {

	//		int32_t goals = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(playerKey, STATNAME_GOALS, Stats::STAT_PERIOD_ALL_TIME, false));
	//		updateClubRecords(playerId, goals, CLUB_RECORD_MOST_GOALS_ALL_TIME);

	//		int32_t games = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(playerKey, STATNAME_GAMES_PLAYED, Stats::STAT_PERIOD_ALL_TIME, false));
	//		updateClubRecords(playerId, games, CLUB_RECORD_MOST_GAMES);

	//		int32_t gameTime = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(playerKey, STATNAME_GAME_TIME, Stats::STAT_PERIOD_ALL_TIME, false));
	//		updateClubRecords(playerId, gameTime, CLUB_RECORD_MOST_MINUTES);

	//		int32_t shotsFor = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(playerKey, STATNAME_SHOTSFOR, Stats::STAT_PERIOD_ALL_TIME, false));
	//		updateClubRecords(playerId, shotsFor, CLUB_RECORD_MOST_SHOTS_ALL_TIME);
    //    }
    //}

    return bSuccess;
}

/*************************************************************************************************/
/*! \brief updateCustomClubRivalChallengeWinnerPlayerStats
    Update the club rival challenge player stats

    \param challengeType - the rival challenge type
    \param playerReport - the winner club's player report
*/
/*************************************************************************************************/
void FifaClubReportSlave::updateCustomClubRivalChallengeWinnerPlayerStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubPlayerReport& playerReport)
{
    switch (challengeType)
    {
    case CLUBS_RIVAL_TYPE_WIN:
        playerReport.setChallengePoints(1);
        break;

    case CLUBS_RIVAL_TYPE_SHUTOUT:
        if (mLowestClubScore == 0)
        {
            playerReport.setChallengePoints(1);
        }
        break;

    case CLUBS_RIVAL_TYPE_WINBY7:
        if ((mHighestClubScore - mLowestClubScore) >= 7)
        {
            playerReport.setChallengePoints(1);
        }
        break;

    case CLUBS_RIVAL_TYPE_WINBY5:
        if ((mHighestClubScore - mLowestClubScore) >= 5)
        {
            playerReport.setChallengePoints(1);
        }
        break;

    default:
        break;
    }
}

/*************************************************************************************************/
/*! \brief updateCustomClubRivalChallengeWinnerClubStats
    Update the club rival challenge club stats for the winning club

    \param challengeType - the rival challenge type
    \param clubReport - the winner club's report
*/
/*************************************************************************************************/
void FifaClubReportSlave::updateCustomClubRivalChallengeWinnerClubStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubClubReport& clubReport)
{
    switch (challengeType)
    {
    case CLUBS_RIVAL_TYPE_WIN:
        clubReport.setChallengePoints(1);
        break;

    case CLUBS_RIVAL_TYPE_SHUTOUT:
        if (mLowestClubScore == 0)
        {
            clubReport.setChallengePoints(1);
        }
        break;

    case CLUBS_RIVAL_TYPE_WINBY7:
        if ((mHighestClubScore - mLowestClubScore) >= 7)
        {
            clubReport.setChallengePoints(1);
        }
        break;

    case CLUBS_RIVAL_TYPE_WINBY5:
        if ((mHighestClubScore - mLowestClubScore) >= 5)
        {
            clubReport.setChallengePoints(1);
        }
        break;

    default:
        break;
    }
}

/*************************************************************************************************/
/*! \brief setCustomClubEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool FifaClubReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
    // Ping doesn't send any Club end game email
    return false;
}
*/

void FifaClubReportSlave::selectCustomStats()
{
	mFifaLastOpponent.selectLastOpponentStats();

	//Seasonal Play
	ClubSeasonsExtension::ExtensionData seasonData;
	seasonData.mProcessedReport = mProcessedReport;
	seasonData.mWinClubId = mWinClubId;
	seasonData.mLossClubId = mLossClubId;
	seasonData.mTieGame = mTieGame;
	mFifaSeasonalPlayExtension.setExtensionData(&seasonData);
	mFifaSeasonalPlayExtension.UpdateEntityFilterList(mEntityFilterList);
	mFifaSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);

	mFifaSeasonalPlay.updateDivCounts();
	mFifaSeasonalPlay.selectSeasonalPlayStats();

	mFifaVpro.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	mFifaVpro.setExtension(&mFifaVproExtension);

	mFifaVproObjectiveStatsUpdater.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	mFifaVproObjectiveStatsUpdater.selectObjectiveStats();

	//VPro
	FifaVpro::CategoryInfoVector categoryVector;
	
	FifaVpro::CategoryInfo clubOTPPlayer;
	clubOTPPlayer.statCategory = "ClubOTPPlayerStats";
	clubOTPPlayer.keyScopeType = FifaVpro::KEYSCOPE_POS;
	categoryVector.push_back(clubOTPPlayer);

	FifaVpro::CategoryInfo clubMember;
	clubMember.statCategory = "ClubMemberStats";
	clubMember.keyScopeType = FifaVpro::KEYSCOPE_CLUBID;
	categoryVector.push_back(clubMember);

	FifaVpro::CategoryInfo clubUserCareer;
	clubUserCareer.statCategory = "ClubUserCareerStats";
	clubUserCareer.keyScopeType = FifaVpro::KEYSCOPE_NONE;
	categoryVector.push_back(clubUserCareer);

	mFifaVpro.updateVproStats(categoryVector);
	
	//-------------------------------------------------------
	categoryVector.clear();

	FifaVpro::CategoryInfo vProCategory;
	vProCategory.statCategory = "VProStats";
	vProCategory.keyScopeType = FifaVpro::KEYSCOPE_NONE;
	categoryVector.push_back(vProCategory);

	mFifaVpro.selectStats(categoryVector);

	//Elo Hybrid Skill
	if (mEntityFilterList.size() == 0)
	{
		ClubEloExtension::ExtensionData data;
		data.mProcessedReport = mProcessedReport;
		data.mUpdateClubsUtil = &mUpdateClubsUtil;

		OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
		OSDKClubGameReportBase::OSDKClubReport* clubReports = static_cast<OSDKClubGameReportBase::OSDKClubReport*>(osdkReport.getTeamReports());
		OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& clubReportMap = clubReports->getClubReports();

		OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::iterator clubIter, clubEnd;
		clubIter = clubReportMap.begin();
		clubEnd = clubReportMap.end();

		for (; clubIter != clubEnd; ++clubIter)
		{
			Clubs::ClubId id = clubIter->first;
			OSDKClubGameReportBase::OSDKClubClubReport* clubReport = clubIter->second;

			uint32_t statPerc = calcClubDampingPercent(id);
			int32_t disc = clubReport->getClubDisc();
			uint32_t result = determineResult(id, *clubReport);

			data.mCalculatedStats[id].stats[ClubEloExtension::ExtensionData::STAT_PERC] = statPerc;
			data.mCalculatedStats[id].stats[ClubEloExtension::ExtensionData::DISCONNECT] = disc;
			data.mCalculatedStats[id].stats[ClubEloExtension::ExtensionData::RESULT] = result;

		}

		mEloRpgHybridSkillExtension.setExtensionData(&data);

		mEloRpgHybridSkill.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport, mReportConfig);
		mEloRpgHybridSkill.selectEloStats();
	}
}

void FifaClubReportSlave::updateCustomTransformStats()
{
	FifaVpro::CategoryInfoVector categoryVector;
	FifaVpro::CategoryInfo vProCategory;
	vProCategory.statCategory = "VProStats";
	vProCategory.keyScopeType = FifaVpro::KEYSCOPE_NONE;
	categoryVector.push_back(vProCategory);

	mFifaVpro.updateVproGameStats(categoryVector);

	mFifaLastOpponent.transformLastOpponentStats();
	mFifaSeasonalPlay.transformSeasonalPlayStats();
	mFifaVproObjectiveStatsUpdater.transformObjectiveStats();
	if (mEntityFilterList.size() == 0)
	{
		mEloRpgHybridSkill.transformEloStats();
	}
}

bool FifaClubReportSlave::processPostStatsCommit()
{
	// HACK do not fail the game report if the tag update fails
	// this occurs as updateClubSettings RPC fails
	mFifaClubsTagUpdater.transformTags();
	mFifaVpro.updatePlayerGrowth();
	return true;
}

void FifaClubReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	TRACE_LOG("[GameClubReportSlave:" << this << "].updateCustomPlayerStats()for player " << playerId << " ");

	OSDKClubGameReportBase::OSDKClubPlayerReport* clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport*>(playerReport.getCustomPlayerReport());
	FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport =  static_cast<FifaClubReportBase::FifaClubsPlayerReport*>(clubPlayerReport->getCustomClubPlayerReport());

	updateCleanSheets(clubPlayerReport->getPos(), clubPlayerReport->getClubId(), fifaClubPlayerReport);
	updatePlayerRating(&playerReport, fifaClubPlayerReport);
	updateMOM(&playerReport, fifaClubPlayerReport);
}

void FifaClubReportSlave::updateCleanSheets(uint32_t pos, Clubs::ClubId clubId, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport)
{
	uint16_t cleanSheetsAny = 0;
	uint16_t cleanSheetsDef = 0;
	uint16_t cleanSheetsGK = 0;

	if (pos == POS_ANY || pos == POS_DEF || pos == POS_GK)
	{
		uint32_t locScore = 0;
		Clubs::ClubId locTeam = 0;
		uint32_t oppScore = 0;
		Clubs::ClubId oppTeam = 0;

		locScore = mLowestClubScore;
		locTeam = clubId;
		oppScore = mHighestClubScore;
		oppTeam = mWinClubId;

		if (locTeam == oppTeam)
		{
			oppScore = locScore;
		}
		if (!oppScore)
		{
			if (pos == POS_ANY)
			{
				cleanSheetsAny = 1;
			}
			else if (pos == POS_DEF)
			{
				cleanSheetsDef = 1;
			}
			else
			{
				cleanSheetsGK = 1;
			}
		}
	}

	fifaClubPlayerReport->setCleanSheetsAny(cleanSheetsAny);
	fifaClubPlayerReport->setCleanSheetsDef(cleanSheetsDef);
	fifaClubPlayerReport->setCleanSheetsGoalKeeper(cleanSheetsGK);

	TRACE_LOG("[FifaClubReportSlave:" << this << "].updateCleanSheets() ANY:" << cleanSheetsAny << " DEF:" << cleanSheetsDef << " GK:" << cleanSheetsGK << " ");
}

void FifaClubReportSlave::updatePlayerRating(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport)
{
	const int DEF_PLAYER_RATING = 3;

	Collections::AttributeMap& map = playerReport->getPrivatePlayerReport().getPrivateAttributeMap();

	float rating = static_cast<float>(atof(map["VProRating"]));
	if (rating <= 0.0f)
	{
		rating = DEF_PLAYER_RATING;
	}

	fifaClubPlayerReport->getCommonPlayerReport().setRating(rating);

	TRACE_LOG("[FifaClubReportSlave:" << this << "].updatePlayerRating() rating:" << fifaClubPlayerReport->getCommonPlayerReport().getRating() << " ");
}

void FifaClubReportSlave::updateMOM(OSDKGameReportBase::OSDKPlayerReport* playerReport, FifaClubReportBase::FifaClubsPlayerReport* fifaClubPlayerReport)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKGameReport osdkGameReport = osdkReport.getGameReport();
	OSDKClubGameReportBase::OSDKClubGameReport* osdkClubGameReport = static_cast<OSDKClubGameReportBase::OSDKClubGameReport*>(osdkGameReport.getCustomGameReport());
	FifaClubReportBase::FifaClubsGameReport* fifaClubReport = static_cast<FifaClubReportBase::FifaClubsGameReport*>(osdkClubGameReport->getCustomClubGameReport());

	eastl::string playerName = playerReport->getName();
	eastl::string mom = fifaClubReport->getMom();
	
	uint16_t isMOM = 0;
	if (playerName == mom)
	{
		isMOM = 1;
	}

	fifaClubPlayerReport->setManOfTheMatch(isMOM);

	TRACE_LOG("[FifaClubReportSlave:" << this << "].updateMOM() isMOM:" << isMOM << " ");
}

uint32_t FifaClubReportSlave::determineResult(Clubs::ClubId id, OSDKClubGameReportBase::OSDKClubClubReport& clubReport)
{
	//first determine if the user won, lost or draw
	uint32_t resultValue = 0;
	if (mWinClubId == id)
	{
		resultValue = WLT_WIN;
	}
	else if (mLossClubId == id)
	{
		resultValue = WLT_LOSS;
	}
	else
	{
		resultValue = WLT_TIE;
	}

	if (clubReport.getClubDisc() == 1)
	{
		resultValue |= WLT_DISC;
	}

	// Track the win-by-dnf
	if (id == mWinClubId && !mClubGameFinish)
	{
		resultValue |= WLT_OPPOQCTAG;
	}

	return resultValue;
}

} // namespace FIFA
} // namespace GameReporting
}// namespace Blaze
