/*************************************************************************************************/
/*!
    \file   gamecustomclubreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "gamecustomclubreportslave.h"

namespace Blaze
{

namespace GameReporting
{

/*************************************************************************************************/
/*! \brief GameCustomClubReportSlave
    Constructor
*/
/*************************************************************************************************/
GameCustomClubReportSlave::GameCustomClubReportSlave(GameReportingSlaveImpl& component) :
GameClubReportSlave(component)
{
}
/*************************************************************************************************/
/*! \brief GameCustomClubReportSlave
    Destructor
*/
/*************************************************************************************************/
GameCustomClubReportSlave::~GameCustomClubReportSlave()
{
}

/*************************************************************************************************/
/*! \brief GameCustomClubReportSlave
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
GameReportProcessor* GameCustomClubReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("GameCustomClubReportSlave") GameCustomClubReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomClubGameTypeName
    Create

    \return GameReportSlave pointer
*/
/*************************************************************************************************/
const char8_t* GameCustomClubReportSlave::getCustomClubGameTypeName() const
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
bool GameCustomClubReportSlave::updateCustomClubRecords(int32_t iClubIndex, OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap)
{
	// Data validation
	if ( iClubIndex >= MAX_CLUB_NUMBER )
	{
		WARN_LOG("[GameCustomClubReportSlave:" << this << "].updateClubStats() - OSDKClubReportMap.size[" << iClubIndex << "] >= MAX_CLUB_NUMBER[" << MAX_CLUB_NUMBER << "]");
		return false;
	}

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();
    
    for(; playerIter != playerEnd; ++playerIter)
    {
        OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
        OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());
        GameClubReportBase::PlayerReport& clubCustomPlayerReport = static_cast<GameClubReportBase::PlayerReport&>(*clubPlayerReport.getCustomClubPlayerReport());

		// TODO: Refactor - Bad code.
		// This always equals 4. Need to be careful of array out of bounds
        const int32_t NUM_STAT_POINTS = (CLUB_RECORD_MOST_GOALS_SINGLE_GAME > CLUB_RECORD_MOST_HITS_SINGLE_GAME) ? 
            CLUB_RECORD_MOST_GOALS_SINGLE_GAME : CLUB_RECORD_MOST_HITS_SINGLE_GAME;
        uint32_t statPoints[NUM_STAT_POINTS];
        memset(statPoints, 0, sizeof(statPoints));

        // This section updates the single game stat for the all time stat that is done in processUpdatedStatsCustom
        statPoints[CLUB_RECORD_MOST_GOALS_SINGLE_GAME - 1] = static_cast<uint32_t>(playerReport.getScore());
        statPoints[CLUB_RECORD_MOST_HITS_SINGLE_GAME - 1] = static_cast<uint32_t>(clubCustomPlayerReport.getHits());

		// TODO: Consider refactoring this. Wasting CPU on needless loop
		// See also: FifaClubReportSlave::updateCustomClubRecords
        int32_t iIndexRecords;
        for (iIndexRecords = 0; iIndexRecords < NUMBER_OF_TROPHIES; iIndexRecords++ )
        {
            if((iIndexRecords == CLUB_RECORD_MOST_GOALS_SINGLE_GAME - 1) || 
                (iIndexRecords == CLUB_RECORD_MOST_HITS_SINGLE_GAME - 1))
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

/*************************************************************************************************/
/*! \brief updateCustomClubAwards
    A custom hook for game team to give club awards based on game report, called during updateClubStats()

    \return bool - true, if club award update process is success
*/
/*************************************************************************************************/
bool GameCustomClubReportSlave::updateCustomClubAwards(Clubs::ClubId clubId,  OSDKClubGameReportBase::OSDKClubClubReport& clubReport, OSDKGameReportBase::OSDKReport& OsdkReport)
{
    bool bSuccess = true;

    /**************** Give award with for simply taking place in the event **************/
    // so each club which finishes the game is awarded
    bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(clubId, CLUB_AWARD_PLAY_GAME) : bSuccess;

    /***************** Give award for playing more than once in the last hour *****************/
    GameHistoryUtil gameHistoryUtil(mComponent);
    GameReportsList gameReportsList;
    QueryVarValuesList queryVarValues;

    char8_t strClubId[32];
    blaze_snzprintf(strClubId, sizeof(strClubId), "%" PRIu64, clubId);
    queryVarValues.push_back(strClubId);
    if (gameHistoryUtil.getGameHistory("club_games_in_last_hour_query", 1, queryVarValues, gameReportsList))
    {
        if (gameReportsList.getGameReportList().size() > 0)
        {
            bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(clubId, CLUB_AWARD_QUICK_SUCCESSION) : bSuccess; 
        }
    }
    gameReportsList.getGameReportList().release();

    /************** Give award for playing with 10 club members *************/
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();
    uint16_t uMemberCount = 0;
    for(; playerIter != playerEnd; ++playerIter)
    {
        uMemberCount++;
    }
    if(10 <= uMemberCount)
    {
        bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(mWinClubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_10_PLAYERS)) : bSuccess; 
    }

    /***************** Give award for scoring more than 5 points in a game  ********************/
    if(5 < static_cast<int32_t>(clubReport.getScore()))
    {
        bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(clubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_SCORE_5)) : bSuccess; 
    }

    /**************** Awards only for winner ************************/
    if(static_cast<Clubs::ClubId>(clubId) == mWinClubId)
    {
        // award the winner the awardID 3 for winning a game 
        bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(mWinClubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_WIN_GAME)) : bSuccess; 

        // award shutout
        if(0 == mLowestClubScore)
        {
            bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(mWinClubId, static_cast<Clubs::ClubAwardId>(CLUB_AWARD_SHUTOUT)) : bSuccess; 
        }

        // beat by 5
        if(5 <= mHighestClubScore - mLowestClubScore)
        {
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
bool GameCustomClubReportSlave::processCustomUpdatedStats()
{
    bool bSuccess = true;

    /***************** Give win streak awards to the winning club  ********************/
    // Obtain the win streak of the winning club
    Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(CATEGORYNAME_CLUB_RANKING, mWinClubId);
    if (NULL != key)
    {
        int32_t iWinStreak = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_WSTREAK, Stats::STAT_PERIOD_ALL_TIME, false));
        if (3 == iWinStreak)
        {
            bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
                static_cast<Blaze::Clubs::ClubId>(mWinClubId), 
                static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_WIN_3_IN_ROW)) : bSuccess; 
        }
        else if (5 == iWinStreak)
        {
            bSuccess = bSuccess ? mUpdateClubsUtil.awardClub(
                static_cast<Blaze::Clubs::ClubId>(mWinClubId), 
                static_cast<Blaze::Clubs::ClubAwardId>(CLUB_AWARD_WIN_5_IN_ROW)) : bSuccess; 
        }
    }

    /******************** Update for Club Records  ************************/
    // Obtain the stats for each player in the player report map
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());;
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerFirst, playerEnd;
    playerIter = playerFirst = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();
    Stats::UpdateRowKey* playerKey = NULL;

    for (; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;
        playerKey = mBuilder.getUpdateRowKey(CATEGORYNAME_CLUB_MEMBERS, playerId);
        if (NULL != playerKey)
        {
            int32_t iPoints = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(playerKey, STATNAME_POINTS, Stats::STAT_PERIOD_ALL_TIME, false));
            updateClubRecords(playerId, iPoints, CLUB_RECORD_MOST_GOALS_ALL_TIME);

            int32_t iHits = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(playerKey, STATNAME_HITS, Stats::STAT_PERIOD_ALL_TIME, false));
            updateClubRecords(playerId, iHits, CLUB_RECORD_MOST_HITS_ALL_TIME);

            int32_t iGamesPlayed = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(playerKey, STATNAME_GAMES_PLAYED, Stats::STAT_PERIOD_ALL_TIME, false));
            updateClubRecords(playerId, iGamesPlayed, CLUB_RECORD_MOST_GAMES);

            int32_t iGameTime = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(playerKey, STATNAME_GAME_TIME, Stats::STAT_PERIOD_ALL_TIME, false));
            updateClubRecords(playerId, iGameTime, CLUB_RECORD_MOST_MINUTES);
        }
    }
    return bSuccess;
}

/*************************************************************************************************/
/*! \brief updateCustomClubRivalChallengeWinnerPlayerStats
    Update the club rival challenge player stats

    \param challengeType - the rival challenge type
    \param playerReport - the winner club's player report
*/
/*************************************************************************************************/
void GameCustomClubReportSlave::updateCustomClubRivalChallengeWinnerPlayerStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubPlayerReport& playerReport)
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
void GameCustomClubReportSlave::updateCustomClubRivalChallengeWinnerClubStats(uint64_t challengeType, OSDKClubGameReportBase::OSDKClubClubReport& clubReport)
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

} // namespace GameReporting
}// namespace Blaze
