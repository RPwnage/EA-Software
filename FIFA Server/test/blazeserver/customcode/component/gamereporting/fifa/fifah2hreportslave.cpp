/*************************************************************************************************/
/*!
    \file   fifah2hreportslave.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "fifah2hreportslave.h"

#include "fifa/tdf/fifah2hreport.h"

namespace Blaze
{

namespace GameReporting
{

#define ATTR_GUESTS0 "GUESTS0"
#define ATTR_GUESTS1 "GUESTS1"

static const size_t LiveCompStatCategoriesNo = 3;
static const size_t LiveCompExtraScopeIndex = 2;
static const char* LiveCompStatCategories[][LiveCompStatCategoriesNo] = {
	//AllTime
	{
		"LiveCompSPOverallStatsAllTime",
		"LiveCompSPDivisionalStatsAllTime",
		"LiveCompNormalStatsAllTime",
	},
	//Monthly
	{
		"LiveCompSPOverallStatsMonthly",
		"LiveCompSPDivisionalStatsMonthly",
		"LiveCompNormalStatsMonthly",
	},
	//Weekly
	{
		"LiveCompSPOverallStatsWeekly",
		"LiveCompSPDivisionalStatsWeekly",
		"LiveCompNormalStatsWeekly",
	},
	//Daily
	{
		"LiveCompSPOverallStatsDaily",
		"LiveCompSPDivisionalStatsDaily",
		"LiveCompNormalStatsDaily",
	},
};

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Constructor
*/
/*************************************************************************************************/
FifaH2HReportSlave::FifaH2HReportSlave(GameReportingSlaveImpl& component) :
FifaH2HBaseReportSlave(component)
{
	mSeasonalPlay.setExtension(&mSeasonalPlayExtension);
	mEloRpgHybridSkill.setExtension(&mEloRpgHybridSkillExtension);
}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Destructor
*/
/*************************************************************************************************/
FifaH2HReportSlave::~FifaH2HReportSlave()
{
}

/*************************************************************************************************/
/*! \brief FifaH2HReportSlave
    Create

    \return GameReportProcessor pointer
*/
/*************************************************************************************************/
GameReportProcessor* FifaH2HReportSlave::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("FifaH2HReportSlave") FifaH2HReportSlave(component);
}

/*************************************************************************************************/
/*! \brief getCustomH2HGameTypeName
    Return the game type name for head-to-head game used in gamereporting.cfg

    \return - the H2H game type name used in gamereporting.cfg
*/
/*************************************************************************************************/
const char8_t* FifaH2HReportSlave::getCustomH2HGameTypeName() const
{
    return "gameType0";
}

/*************************************************************************************************/
/*! \brief getCustomH2HStatsCategoryName
    Return the Stats Category name which the game report updates for

    \return - the stats category needs to be updated for H2H game
*/
/*************************************************************************************************/
const char8_t* FifaH2HReportSlave::getCustomH2HStatsCategoryName() const
{
    return "NormalGameStats";
}

const uint16_t FifaH2HReportSlave::getFifaControlsSetting(OSDKGameReportBase::OSDKPlayerReport& playerReport) const
{
	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
	return h2hPlayerReport.getH2HCustomPlayerData().getControls();
}

/*************************************************************************************************/
/*! \brief updateCustomPlayerStats
    Update custom stats that are regardless of the game result
*/
/*************************************************************************************************/
void FifaH2HReportSlave::updateCustomPlayerStats(GameManager::PlayerId playerId, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
	IntegerAttributeMap& map = playerReport.getPrivatePlayerReport().getPrivateIntAttributeMap();

	if(1== playerReport.getHome())
	{
		h2hPlayerReport.getH2HCustomPlayerData().setNbGuests(static_cast<uint16_t>(map[ATTR_GUESTS0]));
	}
	else
	{
		h2hPlayerReport.getH2HCustomPlayerData().setNbGuests(static_cast<uint16_t>(map[ATTR_GUESTS1]));
	}
}

/*************************************************************************************************/
/*! \brief updateCustomH2HSeasonalPlayerStats
    Update custom player stats for H2H Seasonal game
*/
/*************************************************************************************************/
void FifaH2HReportSlave::updateCustomH2HSeasonalPlayerStats(OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
    updateCustomPlayerStats(/*playerId*/0, playerReport);
}


BlazeRpcError FifaH2HReportSlave::initializeExtensions()
{
	if(mIsValidSponsoredEventId)
	{
		LiveCompHeadtoHeadExtension::ExtensionData data;
		data.mProcessedReport = mProcessedReport;
		data.mTieSet = &mTieSet;
		data.mWinnerSet = &mWinnerSet;
		data.mLoserSet = &mLoserSet;
		data.mTieGame = mTieGame;
		data.mCompetitionId = mSponsoredEventId;
		mLiveCompSeasonalPlayExtension.setExtensionData(&data);

		char8_t strConfigName[LIVE_COMP_CONFIG_MAX_NAME_LEN];
		blaze_snzprintf(strConfigName, LIVE_COMP_CONFIG_MAX_NAME_LEN, LIVE_COMP_CONFIG_SECTION_PREFIX "%" PRId32, mSponsoredEventId);
		mLiveCompSeasonalPlayExtension.fetchConfig(strConfigName);
		if (Blaze::ERR_OK != mLiveCompSeasonalPlayExtension.configLoadStatus())
		{
			return Blaze::ERR_SYSTEM;
		}

		mSeasonalPlay.setExtension(&mLiveCompSeasonalPlayExtension);
		mEloRpgHybridSkill.setExtension(&mLiveCompEloRpgHybridSkillExtension);

		mSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	}
	else
	{
		HeadtoHeadExtension::ExtensionData data;
		data.mProcessedReport = mProcessedReport;
		data.mTieSet = &mTieSet;
		data.mWinnerSet = &mWinnerSet;
		data.mLoserSet = &mLoserSet;
		data.mTieGame = mTieGame;
		mSeasonalPlayExtension.setExtensionData(&data);

		mSeasonalPlay.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport);
	}
	
	return Blaze::ERR_OK;
}

void FifaH2HReportSlave::updateCustomGameStats(OSDKGameReportBase::OSDKReport& OsdkReport)
{
	if(mIsValidSponsoredEventId)
	{
		mSeasonalPlay.updateDivCounts();

		// while we are in here, update the DNF stats for sponsored events.
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
		playerIter = OsdkPlayerReportsMap.begin();
		playerEnd = OsdkPlayerReportsMap.end();
		
		for(; playerIter != playerEnd; ++playerIter)
		{
			GameManager::PlayerId playerId = playerIter->first;
			OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
			Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
			Stats::ScopeNameValueMap playerKeyScopeMap;
						
			playerKeyScopeMap[SCOPENAME_ACCOUNTCOUNTRY]= playerReport.getAccountCountry();
			playerKeyScopeMap[SCOPENAME_CONTROLS]= h2hPlayerReport.getH2HCustomPlayerData().getControls();
			
			TRACE_LOG("[FifaH2HReportSlave:" << this << "].updateCustomGameStats() for playerId: " << playerId << " with DNF status:" << playerReport.getCustomDnf());
			
			// for sponsored events we have to manually increment the games played and games not finished
			mBuilder.startStatRow(getCustomH2HStatsCategoryName(), playerId, &playerKeyScopeMap);
			mBuilder.incrementStat("totalGamesPlayed", 1);
			mBuilder.incrementStat("totalGamesNotFinished",	playerReport.getCustomDnf());
			mBuilder.completeStatRow();

			playerKeyScopeMap.clear();
			playerKeyScopeMap[SCOPENAME_COMPETITIONID] = mSponsoredEventId;;
			playerKeyScopeMap[SCOPENAME_TEAMID] = h2hPlayerReport.getH2HCustomPlayerData().getTeam();

			char8_t strBuffer[OSDKGameReportBase::SE_CATEGORY_MAX_NAME_LENGTH];
			getLiveCompetitionsStatsCategoryName(strBuffer, sizeof(strBuffer));

			int goals = h2hPlayerReport.getCommonPlayerReport().getGoals();
			int goalsAgainst = h2hPlayerReport.getCommonPlayerReport().getGoalsConceded();

			mBuilder.startStatRow(strBuffer, playerId, &playerKeyScopeMap);
			mBuilder.incrementStat("goals", goals);
			mBuilder.incrementStat("goalsAgainst", goalsAgainst);
			mBuilder.completeStatRow();
		}
	}
	else
	{
		mSeasonalPlay.updateDivCounts();
	}
}

/*************************************************************************************************/
/*! \brief updateCustomNotificationReport
    Update Notification Report.
*/
/*************************************************************************************************/
void FifaH2HReportSlave::updateCustomNotificationReport(const char8_t* statsCategory)
{
    // Obtain the custom data for report notification
    OSDKGameReportBase::OSDKNotifyReport *OsdkReportNotification = static_cast<OSDKGameReportBase::OSDKNotifyReport*>(mProcessedReport->getCustomData());
	Fifa::H2HNotificationCustomGameData *gameCustomData = BLAZE_NEW Fifa::H2HNotificationCustomGameData();
    Fifa::H2HNotificationCustomGameData::PlayerCustomDataMap &playerCustomDataMap = gameCustomData->getPlayerCustomDataReports();

    // Obtain the player report map
    OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

    OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
    playerIter = OsdkPlayerReportsMap.begin();
    playerEnd = OsdkPlayerReportsMap.end();

    //Stats were already committed and can be evaluated during that process. Fetching stats again
    //to ensure mUpdateStatsHelper cache is valid (and avoid crashing with assert)
    BlazeRpcError fetchError = mUpdateStatsHelper.fetchStats();
	for(; playerIter != playerEnd; ++playerIter)
    {
        GameManager::PlayerId playerId = playerIter->first;

        // Obtain the Skill Points
        Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(statsCategory, playerId);
        if (DB_ERR_OK == fetchError && NULL != key)
        {
            int64_t overallSkillPoints = static_cast<int32_t>(mUpdateStatsHelper.getValueInt(key, STATNAME_OVERALLPTS, Stats::STAT_PERIOD_ALL_TIME, false));

            // Update the report notification custom data with the overall SkillPoints
			Fifa::H2HNotificationPlayerCustomData* playerCustomData = BLAZE_NEW Fifa::H2HNotificationPlayerCustomData();
            playerCustomData->setSkillPoints(static_cast<uint32_t>(overallSkillPoints));

            // Insert the player custom data into the player custom data map
            playerCustomDataMap.insert(eastl::make_pair(playerId, playerCustomData));
        }
    }

    // Set the gameCustomData to the OsdkReportNotification
    OsdkReportNotification->setCustomDataReport(*gameCustomData);
}

void FifaH2HReportSlave::updateCustomPlayerKeyscopes(OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportMap)
{
	if (!mIsValidSponsoredEventId)
	{
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
		playerIter = osdkPlayerReportMap.begin();
		playerEnd = osdkPlayerReportMap.end();

		for(; playerIter != playerEnd; ++playerIter)
		{
			GameManager::PlayerId playerId = playerIter->first;
			OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
			Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
			Stats::ScopeNameValueMap* playerKeyScopeMap = mPlayerKeyscopes[playerId];
			(*playerKeyScopeMap)[SCOPENAME_CONTROLS]= h2hPlayerReport.getH2HCustomPlayerData().getControls();
		}
	}
	else
	{
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
		playerIter = osdkPlayerReportMap.begin();
		playerEnd = osdkPlayerReportMap.end();

		for(; playerIter != playerEnd; ++playerIter)
		{
			GameManager::PlayerId playerId = playerIter->first;
			OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
			Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
			Stats::ScopeNameValueMap* playerKeyScopeMap = mPlayerKeyscopes[playerId];
			bool bHasLockedTeam = mSeasonalPlay.getExtension()->getDefinesHelper()->GetIntSetting(DefinesHelper::IS_TEAM_LOCKED) != 0;
			(*playerKeyScopeMap)[SCOPENAME_COMPETITIONID]= mSponsoredEventId;
			(*playerKeyScopeMap)[SCOPENAME_TEAMID]= bHasLockedTeam ? h2hPlayerReport.getH2HCustomPlayerData().getTeam() : UNIQUE_TEAM_ID_KEYSCOPE_VAL;
		}
	}
}
/*************************************************************************************************/
/*! \brief setCustomEndGameMailParam
    A custom hook for Game team to set parameters for the end game mail, return true if sending a mail

    \param mailParamList - the parameter list for the mail to send
    \param mailTemplateName - the template name of the email
    \return bool - true if to send an end game email
*/
/*************************************************************************************************/
/*
bool FifaH2HReportSlave::setCustomEndGameMailParam(Mail::HttpParamList* mailParamList, char8_t* mailTemplateName)
{
    HttpParam param;
    param.name = "gameName";
    param.value = "BlazeGame";
    param.encodeValue = true;

    mailParamList->push_back(param);
	blaze_snzprintf(mailTemplateName, MAX_MAIL_TEMPLATE_NAME_LEN, "%s", "h2h_gamereport");

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	OSDKGameReportBase::OSDKPlayerReport* playerReport = NULL;

	for(; playerIter != playerEnd; ++playerIter)
	{
		playerReport = playerIter->second;
		
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport->getCustomPlayerReport());

		Fifa::CommonPlayerReport& commonPlayerReport = h2hPlayerReport.getCommonPlayerReport();
		Fifa::H2HCustomPlayerData& h2hCustomPlayerData = h2hPlayerReport.getH2HCustomPlayerData();

		if(playerReport->getHome())
		{
			
			blaze_snzprintf(mName0Str, sizeof(mName0Str), "%s", playerReport->getName());
			HttpParam name0;
			name0.name = "name0";
			name0.value = mName0Str;
			name0.encodeValue = true;
			mailParamList->push_back(name0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding name0Str " << mName0Str);

			blaze_snzprintf(mTeam0Str, sizeof(mTeam0Str), "%s", h2hCustomPlayerData.getTeamName());
			HttpParam team0;
			team0.name = "team0";
			team0.value = mTeam0Str;
			team0.encodeValue = true;
			mailParamList->push_back(team0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding team0Str " << mTeam0Str);

			blaze_snzprintf(mResult0Str, sizeof(mResult0Str), "%d", playerReport->getUserResult());
			HttpParam result0;
			result0.name = "result0";
			result0.value = mResult0Str;
			result0.encodeValue = true;
			mailParamList->push_back(result0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding result0Str " << mResult0Str);

			blaze_snzprintf(mScore0Str, sizeof(mScore0Str), "%d", commonPlayerReport.getGoals());
			HttpParam score0;
			score0.name = "score0";
			score0.value = mScore0Str;
			score0.encodeValue = true;
			mailParamList->push_back(score0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding score0Str " << mScore0Str);

			blaze_snzprintf(mShotsongoal0Str, sizeof(mShotsongoal0Str), "%d", commonPlayerReport.getShotsOnGoal());
			HttpParam shotsongoal0;
			shotsongoal0.name = "shotsongoal0";
			shotsongoal0.value = mShotsongoal0Str;
			shotsongoal0.encodeValue = true;
			mailParamList->push_back(shotsongoal0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding shotsongoal0Str " << mShotsongoal0Str);

			blaze_snzprintf(mShots0Str, sizeof(mShots0Str), "%d", commonPlayerReport.getShots());
			HttpParam shots0;
			shots0.name = "shots0";
			shots0.value = mShots0Str;
			shots0.encodeValue = true;
			mailParamList->push_back(shots0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding shots0Str " <<  mShots0Str);

			blaze_snzprintf(mPossession0Str, sizeof(mPossession0Str), "%d", commonPlayerReport.getPossession());
			HttpParam possession0;
			possession0.name = "possession0";
			possession0.value = mPossession0Str;
			possession0.encodeValue = true;
			mailParamList->push_back(possession0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding possession0Str " << mPossession0Str);

			int32_t passesMade = commonPlayerReport.getPassesMade();
			int32_t passAttempts = commonPlayerReport.getPassAttempts();
			int32_t passPercent = (passAttempts <= 0) ? 0 : (100* passesMade / passAttempts);

			blaze_snzprintf(mPassing0Str, sizeof(mPassing0Str), "%d", passPercent);
			HttpParam passing0;
			passing0.name = "passing0";
			passing0.value = mPassing0Str;
			passing0.encodeValue = true;
			mailParamList->push_back(passing0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding passing0Str " << mPassing0Str);

			int32_t tacklesMade = commonPlayerReport.getTacklesMade();
			int32_t tackleAttempts = commonPlayerReport.getTackleAttempts();
			int32_t tacklePercent = (tackleAttempts <= 0) ? 0 : (100 * tacklesMade / tackleAttempts);

			blaze_snzprintf(mTackle0Str, sizeof(mTackle0Str), "%d", tacklePercent);
			HttpParam tackle0;
			tackle0.name = "tackle0";
			tackle0.value = mTackle0Str;
			tackle0.encodeValue = true;
			mailParamList->push_back(tackle0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding tackle0Str " << mTackle0Str);

			blaze_snzprintf(mCards0Str, sizeof(mCards0Str), "%d", commonPlayerReport.getYellowCard() + commonPlayerReport.getRedCard());
			HttpParam cards0;
			cards0.name = "cards0";
			cards0.value = mCards0Str;
			cards0.encodeValue = true;
			mailParamList->push_back(cards0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding cards0Str " << mCards0Str);

			blaze_snzprintf(mFouls0Str, sizeof(mFouls0Str), "%d", commonPlayerReport.getFouls());
			HttpParam fouls0;
			fouls0.name = "fouls0";
			fouls0.value = mFouls0Str;
			fouls0.encodeValue = true;
			mailParamList->push_back(fouls0);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding fouls0Str " << mFouls0Str);

			blaze_snzprintf(mCorners0Str, sizeof(mCorners0Str), "%d", commonPlayerReport.getCorners());
			HttpParam corners0;
			corners0.name = "corners0";
			corners0.value = mCorners0Str;
			corners0.encodeValue = true;
			mailParamList->push_back(corners0);    
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding corners0Str " << mCorners0Str);
		}
		else
		{
			blaze_snzprintf(mName1Str, sizeof(mName1Str), "%s", playerReport->getName());
			HttpParam name1;
			name1.name = "name1";
			name1.value = mName1Str;
			name1.encodeValue = true;
			mailParamList->push_back(name1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding name1Str " << mName1Str);

			blaze_snzprintf(mTeam1Str, sizeof(mTeam1Str), "%s", h2hCustomPlayerData.getTeamName());
			HttpParam team1;
			team1.name = "team1";
			team1.value = mTeam1Str;
			team1.encodeValue = true;
			mailParamList->push_back(team1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding team1Str " << mTeam1Str);

			blaze_snzprintf(mResult1Str, sizeof(mResult1Str), "%d", playerReport->getUserResult());
			HttpParam result1;
			result1.name = "result1";
			result1.value = mResult1Str;
			result1.encodeValue = true;
			mailParamList->push_back(result1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding result1Str " << mResult1Str);

			blaze_snzprintf(mScore1Str, sizeof(mScore1Str), "%d", commonPlayerReport.getGoals());
			HttpParam score1;
			score1.name = "score1";
			score1.value = mScore1Str;
			score1.encodeValue = true;
			mailParamList->push_back(score1);    
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding score1Str " << mScore1Str);

			blaze_snzprintf(mShotsongoal1Str, sizeof(mShotsongoal1Str), "%d", commonPlayerReport.getShotsOnGoal());
			HttpParam shotsongoal1;
			shotsongoal1.name = "shotsongoal1";
			shotsongoal1.value = mShotsongoal1Str;
			shotsongoal1.encodeValue = true;
			mailParamList->push_back(shotsongoal1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding shotsongoal1Str " << mShotsongoal1Str);

			blaze_snzprintf(mShots1Str, sizeof(mShots1Str), "%d", commonPlayerReport.getShots());
			HttpParam shots1;
			shots1.name = "shots1";
			shots1.value = mShots1Str;
			shots1.encodeValue = true;
			mailParamList->push_back(shots1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding shots1Str " << mShots1Str);

			blaze_snzprintf(mPossession1Str, sizeof(mPossession1Str), "%d", commonPlayerReport.getPossession());
			HttpParam possession1;
			possession1.name = "possession1";
			possession1.value = mPossession1Str;
			possession1.encodeValue = true;
			mailParamList->push_back(possession1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding possession1Str " << mPossession1Str);

			int32_t passesMade = commonPlayerReport.getPassesMade();
			int32_t passAttempts = commonPlayerReport.getPassAttempts();               
			int32_t passPercent = (passAttempts <= 0) ? 0 : (100* passesMade / passAttempts);

			blaze_snzprintf(mPassing1Str, sizeof(mPassing1Str), "%d", passPercent);
			HttpParam passing1;
			passing1.name = "passing1";
			passing1.value = mPassing1Str;
			passing1.encodeValue = true;
			mailParamList->push_back(passing1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding passing1Str " << mPassing1Str);

			int32_t tacklesMade = commonPlayerReport.getTacklesMade();
			int32_t tackleAttempts = commonPlayerReport.getTackleAttempts();
			int32_t tacklePercent = (tackleAttempts <= 0) ? 0 : (100 * tacklesMade / tackleAttempts);

			blaze_snzprintf(mTackle1Str, sizeof(mTackle1Str), "%d", tacklePercent);
			HttpParam tackle1;
			tackle1.name = "tackle1";
			tackle1.value = mTackle1Str;
			tackle1.encodeValue = true;
			mailParamList->push_back(tackle1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding tackle1Str " << mTackle1Str);

			blaze_snzprintf(mCards1Str, sizeof(mCards1Str), "%d", commonPlayerReport.getYellowCard() + commonPlayerReport.getRedCard());
			HttpParam cards1;
			cards1.name = "cards1";
			cards1.value = mCards1Str;
			cards1.encodeValue = true;
			mailParamList->push_back(cards1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding cards1Str " << mCards1Str);

			blaze_snzprintf(mFouls1Str, sizeof(mFouls1Str), "%d", commonPlayerReport.getFouls());
			HttpParam fouls1;
			fouls1.name = "fouls1";
			fouls1.value = mFouls1Str;
			fouls1.encodeValue = true;
			mailParamList->push_back(fouls1);    
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding fouls1Str " << mFouls1Str);

			blaze_snzprintf(mCorners1Str, sizeof(mCorners1Str), "%d", commonPlayerReport.getCorners());
			HttpParam corners1;
			corners1.name = "corners1";
			corners1.value = mCorners1Str;
			corners1.encodeValue = true;
			mailParamList->push_back(corners1);
			TRACE_LOG("[GameReporting].generateH2HEmail(): adding corners1Str " << mCorners1Str);
		}

	}

    return true;
}
*/
// Check for Sponsored Event, then call accordingly
void FifaH2HReportSlave::selectCustomStats()
{
	if(mIsValidSponsoredEventId)
	{
		selectLiveCompetitionCustomStats();
	}
	else
	{
		selectH2HCustomStats();
	}
}

void FifaH2HReportSlave::selectLiveCompetitionCustomStats()
{
	mSeasonalPlay.selectSeasonalPlayStats();

	LiveCompEloExtension::ExtensionData data;
	data.mProcessedReport = mProcessedReport;
	data.mSponsoredEventId = mSponsoredEventId;
	data.mUseLockedTeam = mSeasonalPlay.getExtension()->getDefinesHelper()->GetIntSetting(DefinesHelper::IS_TEAM_LOCKED) != 0;
	data.mPeriodType = mSeasonalPlay.getExtension()->getDefinesHelper()->GetPeriodType();

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = osdkPlayerReportsMap.begin();
	playerEnd = osdkPlayerReportsMap.end();

	for (; playerIter != playerEnd; ++playerIter)
	{
		EntityId id = playerIter->first;
		EA::TDF::tdf_ptr<OSDKGameReportBase::OSDKPlayerReport> playerReport = playerIter->second;

		uint32_t statPerc = calcPlayerDampingPercent(id, *playerReport);
		int32_t disc = didPlayerFinish(playerIter->first, *playerReport)? 0 : 1;
		uint32_t result = determineResult(playerIter->first, *playerReport);

		data.mCalculatedStats[id].stats[LiveCompEloExtension::ExtensionData::STAT_PERC] = statPerc;
		data.mCalculatedStats[id].stats[LiveCompEloExtension::ExtensionData::DISCONNECT] = disc;
		data.mCalculatedStats[id].stats[LiveCompEloExtension::ExtensionData::RESULT] = result;
	}

	mLiveCompEloRpgHybridSkillExtension.setExtensionData(&data);

	mEloRpgHybridSkill.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport, mReportConfig);
	mEloRpgHybridSkill.selectEloStats();
}

void FifaH2HReportSlave::selectH2HCustomStats()
{
	mSeasonalPlay.selectSeasonalPlayStats();

	HeadtoHeadEloExtension::ExtensionData data;
	data.mProcessedReport = mProcessedReport;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = osdkPlayerReportsMap.begin();
	playerEnd = osdkPlayerReportsMap.end();

	for (; playerIter != playerEnd; ++playerIter)
	{
		EntityId id = playerIter->first;
		EA::TDF::tdf_ptr<OSDKGameReportBase::OSDKPlayerReport> playerReport = playerIter->second;

		uint32_t statPerc = calcPlayerDampingPercent(id, *playerReport);
		int32_t disc = didPlayerFinish(playerIter->first, *playerReport)? 0 : 1;
		uint32_t result = determineResult(playerIter->first, *playerReport);

		data.mCalculatedStats[id].stats[HeadtoHeadEloExtension::ExtensionData::STAT_PERC] = statPerc;
		data.mCalculatedStats[id].stats[HeadtoHeadEloExtension::ExtensionData::DISCONNECT] = disc;
		data.mCalculatedStats[id].stats[HeadtoHeadEloExtension::ExtensionData::RESULT] = result;

	}

	mEloRpgHybridSkillExtension.setExtensionData(&data);

	mEloRpgHybridSkill.initialize(&mBuilder, &mUpdateStatsHelper, mProcessedReport, mReportConfig);
	mEloRpgHybridSkill.selectEloStats();
}

void FifaH2HReportSlave::updateCustomTransformStats(const char8_t* statsCategory)
{
	mSeasonalPlay.transformSeasonalPlayStats();
	mEloRpgHybridSkill.transformEloStats();
	updateLiveCompetitionCustomStats(statsCategory);
	preStatCommitProcess();
}

void FifaH2HReportSlave::customFillNrmlXmlData(uint64_t entityId, eastl::string &buffer)
{
	mSeasonalPlay.getSPDivisionalStatXmlString(entityId, buffer);
	mSeasonalPlay.getSPOverallStatXmlString(entityId, buffer);
}

void FifaH2HReportSlave::updateLiveCompetitionCustomStats(const char8_t* statsCategory)
{
	if(mIsValidSponsoredEventId)
	{
		OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

		OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
		playerIter = OsdkPlayerReportsMap.begin();
		playerEnd = OsdkPlayerReportsMap.end();

		for(; playerIter != playerEnd; ++playerIter)
		{
			EntityId id = playerIter->first;
            OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
			
			Stats::ScopeNameValueMap* playerKeyScopeMap = mPlayerKeyscopes[id];
			
			int64_t rankingPoints = mLiveCompSeasonalPlayExtension.GetHookEntityRankingPoints(id);

			// update the stat
			Stats::UpdateRowKey* rowKey = mBuilder.getUpdateRowKey(statsCategory, id);
			if (rowKey != NULL)
			{
				rowKey->period = mLiveCompSeasonalPlayExtension.getPeriodType();
				rowKey->scopeNameValueMap = playerKeyScopeMap;
			}

			mUpdateStatsHelper.setValueInt(rowKey, STATNAME_SPRANKINGPOINTS, mLiveCompSeasonalPlayExtension.getPeriodType(), rankingPoints);

            Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport.getCustomPlayerReport());
            Fifa::H2HCustomPlayerData& h2hCustomPlayerData = h2hPlayerReport.getH2HCustomPlayerData();
            uint32_t teamId = h2hCustomPlayerData.getTeam();
            mUpdateStatsHelper.setValueInt(rowKey, STATNAME_TEAMID, mLiveCompSeasonalPlayExtension.getPeriodType(), teamId);

			TRACE_LOG("[FifaH2HReportSlave].updateCustomTransformStats() (LIVE COMP) - Entity id:" << rowKey->entityId <<  
																						" Category:" << rowKey->category << 
																						" Period:" << rowKey->period << 
																						" Stat Name:" << STATNAME_SPRANKINGPOINTS << 
																						" Value:" << rankingPoints << 
																						" ");
		}
	}
}

void FifaH2HReportSlave::getLiveCompetitionsStatsCategoryName(char8_t* pOutputBuffer, uint32_t uOutputBufferSize) const
{
	const char * prefix = LIVE_COMP_NORMAL_STATS_CATEGORY;
	const char * suffix = GetLCCategorySuffixFromPeriod(mSeasonalPlay.getExtension()->getDefinesHelper()->GetPeriodType());
	blaze_snzprintf(pOutputBuffer, uOutputBufferSize, "%s%s", prefix, suffix);
}

uint32_t FifaH2HReportSlave::determineResult(EntityId id, OSDKGameReportBase::OSDKPlayerReport& playerReport)
{
	//first determine if the user won, lost or draw
	uint32_t resultValue = 0;
	if (mWinnerSet.find(id) != mWinnerSet.end())
	{
		resultValue = WLT_WIN;
	}
	else if (mLoserSet.find(id) != mLoserSet.end())
	{
		resultValue = WLT_LOSS;
	}
	else
	{
		resultValue = WLT_TIE;
	}

	if (!didPlayerFinish(id, playerReport))
	{
		switch(playerReport.getFinishReason())
		{
		case PLAYER_FINISH_REASON_DISCONNECT:
			resultValue |= WLT_DISC;
			break;
		case PLAYER_FINISH_REASON_QUIT:
			resultValue |= WLT_CONCEDE;
			break;
		}
	}

	return resultValue;
}

void FifaH2HReportSlave::preStatCommitProcess()
{
	//Need to check if game capping is enabled; if yes and one player has somehow bypassed client side protections, we need to "undo" her stats
	if (mIsSponsoredEventReport)
	{
		const DefinesHelper* defines = mSeasonalPlay.getExtension()->getDefinesHelper();

		//TODO: config should already be fetched. Should we do any checks here?
		Stats::StatPeriodType periodType = mSeasonalPlay.getExtension()->getDefinesHelper()->GetPeriodType();
		bool alltimeCappingEnabled = defines->GetBoolSetting(DefinesHelper::ALLTIME_GAME_CAPPING_ENABLED);
		bool monthlyCappingEnabled = defines->GetBoolSetting(DefinesHelper::MONTHLY_GAME_CAPPING_ENABLED);
		bool weeklyCappingEnabled = defines->GetBoolSetting(DefinesHelper::WEEKLY_GAME_CAPPING_ENABLED);
		bool dailyCappingEnabled = defines->GetBoolSetting(DefinesHelper::DAILY_GAME_CAPPING_ENABLED);

		int64_t maxGamesAllTime = defines->GetIntSetting(DefinesHelper::MAX_GAMES_ALL_TIME);
		int64_t maxGamesPerMonth = defines->GetIntSetting(DefinesHelper::MAX_GAMES_PER_MONTH);
		int64_t maxGamesPerWeek = defines->GetIntSetting(DefinesHelper::MAX_GAMES_PER_WEEK);
		int64_t maxGamesPerDay = defines->GetIntSetting(DefinesHelper::MAX_GAMES_PER_DAY);

		if (alltimeCappingEnabled || monthlyCappingEnabled || weeklyCappingEnabled || dailyCappingEnabled)
		{
			OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

			OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
			playerIter = OsdkPlayerReportsMap.begin();
			playerEnd = OsdkPlayerReportsMap.end();

			for(; playerIter != playerEnd; ++playerIter)
			{
				Stats::ScopeNameValueMap playerKeyScopeMap;
				playerKeyScopeMap[SCOPENAME_COMPETITIONID] = mSponsoredEventId;
				
				bool shouldRevertStats = false;
				BlazeId playerId = playerIter->first;
				
				const Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey("LiveCompGamesPlayedAllTime", playerId, &playerKeyScopeMap);
				int64_t gamesPlayedAllTime = mUpdateStatsHelper.getValueInt(key, "gamesPlayedAllTime", Stats::STAT_PERIOD_ALL_TIME, true);
				shouldRevertStats = shouldRevertStats || (alltimeCappingEnabled && gamesPlayedAllTime >= maxGamesAllTime);

				key = mBuilder.getUpdateRowKey("LiveCompGamesPlayedMonthly", playerId, &playerKeyScopeMap);
				int64_t gamesPlayedThisMonth = mUpdateStatsHelper.getValueInt(key, "gamesPlayedThisMonth", Stats::STAT_PERIOD_MONTHLY, true);
				shouldRevertStats = shouldRevertStats || (monthlyCappingEnabled && gamesPlayedThisMonth >= maxGamesPerMonth);

				key = mBuilder.getUpdateRowKey("LiveCompGamesPlayedWeekly", playerId, &playerKeyScopeMap);
				int64_t gamesPlayedThisWeek = mUpdateStatsHelper.getValueInt(key, "gamesPlayedThisWeek", Stats::STAT_PERIOD_WEEKLY, true);
				shouldRevertStats = shouldRevertStats || (weeklyCappingEnabled && gamesPlayedThisWeek >= maxGamesPerWeek);

				key = mBuilder.getUpdateRowKey("LiveCompGamesPlayedDaily", playerId, &playerKeyScopeMap);
				int64_t gamesPlayedToday = mUpdateStatsHelper.getValueInt(key, "gamesPlayedToday", Stats::STAT_PERIOD_DAILY, true);
				shouldRevertStats = shouldRevertStats || (dailyCappingEnabled && gamesPlayedToday >= maxGamesPerDay);

				if (shouldRevertStats)
				{
					StatCategoryContainer stats;
					buildStatRevertList(stats, periodType);
					undoStats(playerId, stats, periodType);
				}
			}
		}
	}
}

void FifaH2HReportSlave::buildStatRevertList(StatCategoryContainer& outParam, Stats::StatPeriodType periodType)
{
	Blaze::BlazeRpcError error = Blaze::ERR_OK;
	Blaze::Stats::StatsSlave *statsComponent =
		static_cast<Blaze::Stats::StatsSlave*>(gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID,false));
	if (statsComponent == NULL)
	{
		ERR_LOG("[FifaH2HReportSlave].buildStatRevertList() (LIVE COMP) - Couldn't access stats component");
		return;
	}

	for (size_t i = 0; i < LiveCompStatCategoriesNo; ++i)
	{
		Stats::GetStatDescsRequest getStatsDescRequest;
		getStatsDescRequest.setCategory(LiveCompStatCategories[periodType][i]);
		Stats::StatDescs getStatsDescsResponse;
		error = statsComponent->getStatDescs(getStatsDescRequest, getStatsDescsResponse);

		if (error == Blaze::ERR_OK)
		{
			StatCategoryInfo statCategory;
			statCategory.name = LiveCompStatCategories[periodType][i];
			statCategory.keyscopes.push_back(SCOPENAME_COMPETITIONID);
			if (LiveCompExtraScopeIndex == i)
			{
				statCategory.keyscopes.push_back(SCOPENAME_TEAMID);
			}
			Stats::StatDescSummaryList& stats = getStatsDescsResponse.getStatDescs();
			for (auto it = stats.begin(); it != stats.end(); ++it)
			{
				statCategory.stats.push_back((*it)->getName());
			}
			outParam.push_back(statCategory);
		}
	}
}

void FifaH2HReportSlave::undoStats(BlazeId playerId, const StatCategoryContainer& stats, Stats::StatPeriodType periodType)
{
	INFO_LOG("[FifaH2HReportSlave:" << this << "].undoStats() for playerId: " << playerId );
	for (auto categoryIt = stats.begin(); categoryIt != stats.end(); ++categoryIt)
	{
		Stats::ScopeNameValueMap snvm;
		for (auto scopeIt = categoryIt->keyscopes.begin(); scopeIt != categoryIt->keyscopes.end(); ++scopeIt)
		{
			PlayerScopeIndexMap::iterator playerIterator = mPlayerKeyscopes.find(playerId);
			if (playerIterator != mPlayerKeyscopes.end() && playerIterator->second != NULL)
			{
				Stats::ScopeNameValueMap::iterator it = playerIterator->second->find(scopeIt->c_str());
				if (it != playerIterator->second->end())
				{
					snvm.insert(eastl::make_pair(it->first, it->second));
				}
			}
		}
		const Stats::UpdateRowKey* key = mBuilder.getUpdateRowKey(categoryIt->name.c_str(), playerId, &snvm);
		for (auto statIt = categoryIt->stats.begin(); statIt != categoryIt->stats.end(); ++statIt)
		{
			mUpdateStatsHelper.setValueInt(key, statIt->c_str(), periodType, 
				mUpdateStatsHelper.getValueInt(key, statIt->c_str(), periodType, true));
		}
	}
}

}   // namespace GameReporting

}   // namespace Blaze

