/*************************************************************************************************/
/*!
    \file   fifa_seasonalplayextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "fifa_seasonalplayextensions.h"
#include "coopseason/tdf/coopseasontypes_base.h"
#include <EAStdC/EADateTime.h>

#include "customcode/component/gamereporting/fifa/fifalivecompetitionsdefines.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

/*-------------------------------------------
		Head to Head Seasonal Play
---------------------------------------------*/

HeadtoHeadExtension::DivCountData HeadtoHeadExtension::mDivCountData;
EA::Thread::Mutex HeadtoHeadExtension::mMutex;
EA::Thread::Mutex CoopSeasonsExtension::mMutex;
EA::Thread::Mutex ClubSeasonsExtension::mMutex;
EA::Thread::Mutex LiveCompHeadtoHeadExtension::mMutex;

HeadtoHeadExtension::HeadtoHeadExtension()
{
}

HeadtoHeadExtension::~HeadtoHeadExtension()
{
}

void HeadtoHeadExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

const char8_t* HeadtoHeadExtension::getDivStatCategory()
{
	return "SPDivisionalPlayerStats";
}

const char8_t* HeadtoHeadExtension::getOverallStatCategory()
{
	return "SPOverallPlayerStats";
}

const char8_t* HeadtoHeadExtension::getDivCountStatCategory()
{
	return "SPDivisionCount";
}

const char8_t* HeadtoHeadExtension::getLastOpponentStatCategory()
{
	return "SPOverallPlayerStats";

}

void HeadtoHeadExtension::getEntityIds(EntityList& ids)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		ids.push_back(playerIter->first);
	}
}

FifaCups::MemberType HeadtoHeadExtension::getMemberType()
{
	return FifaCups::FIFACUPS_MEMBERTYPE_USER;
}
	
IFifaSeasonalPlayExtension::GameResult HeadtoHeadExtension::getGameResult(EntityId entityId)
{
	GameResult result = LOSS;

	if (mExtensionData.mTieGame)
	{
		result = TIE;
	}
	else if (mExtensionData.mWinnerSet->find(entityId) != mExtensionData.mWinnerSet->end())
	{
		result = WIN;
	}
	else
	{
		result = LOSS;
	}

	return result;
}

EntityId HeadtoHeadExtension::getOpponentId(EntityId entityId)
{
	EntityId opponentId = -1;

	// assumes mWinnerSet/mLoserSet each has a single entry
	if(mExtensionData.mWinnerSet->find(entityId) != mExtensionData.mWinnerSet->end())
	{
		opponentId = *(mExtensionData.mLoserSet->begin());
	}
	else
	if(mExtensionData.mLoserSet->find(entityId) != mExtensionData.mLoserSet->end())
	{
		opponentId = *(mExtensionData.mWinnerSet->begin());
	}
	else
	{
		// iterate through two-entry mTieSet to get opponent
		for(ResultSet::const_iterator it = mExtensionData.mTieSet->begin(); it != mExtensionData.mTieSet->end(); ++it)
		{
			if(*it == entityId)
			{
				// skip entry corresponding to self
				continue;
			}
			opponentId = *it;
		}
	}

	return opponentId;
}

uint16_t HeadtoHeadExtension::getGoalsFor(EntityId entityId)
{
	uint16_t result = 0;
	
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (osdkPlayerReport != NULL)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		result = h2hPlayerReport.getCommonPlayerReport().getGoals();
	}

	return result;
}

uint16_t HeadtoHeadExtension::getGoalsAgainst(EntityId entityId)
{
	uint16_t result = 0;

	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (osdkPlayerReport != NULL)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		result = h2hPlayerReport.getCommonPlayerReport().getGoalsConceded();
	}

	return result;
}

bool HeadtoHeadExtension::didUserFinish(EntityId entityId)
{
	bool result = false;

	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (osdkPlayerReport != NULL)
	{
		const CollatedGameReport::DnfStatusMap& DnfStatusMap = mExtensionData.mProcessedReport->getDnfStatusMap();
		result = (0 == DnfStatusMap.find(entityId)->second);
		uint32_t discResult = osdkPlayerReport->getUserResult();

		if (false == result && ((GAME_RESULT_COMPLETE_REGULATION == discResult) ||
			(GAME_RESULT_COMPLETE_OVERTIME == discResult)))
		{
			result = true;
		}
	}

	return result;
}

Stats::StatPeriodType HeadtoHeadExtension::getPeriodType()
{
	Stats::StatPeriodType statPeriod = Stats::STAT_PERIOD_ALL_TIME;    // default

	if(getDefinesHelper() != NULL)
	{
		statPeriod = getDefinesHelper()->GetPeriodType();
	}
	return statPeriod;
}

void HeadtoHeadExtension::getCupResult(CupResult& cupResult)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		GameManager::PlayerId playerId = playerIter->first;
		OSDKGameReportBase::OSDKPlayerReport* playerReport = playerIter->second;
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport->getCustomPlayerReport());
		//result = h2hPlayerReport.getCommonPlayerReport().getGoalsConceded();

		if (mExtensionData.mWinnerSet->find(playerId) != mExtensionData.mWinnerSet->end())
		{
			cupResult.mSideInfo[CUP_WINNER].id = playerId;
			cupResult.mSideInfo[CUP_WINNER].teamId = playerReport->getTeam();
			cupResult.mSideInfo[CUP_WINNER].score = playerReport->getScore();

			cupResult.mWentToPenalties = h2hPlayerReport.getH2HCustomPlayerData().getWentToPk() == 1;
		}
		else if (mExtensionData.mLoserSet->find(playerId) != mExtensionData.mLoserSet->end())
		{
			cupResult.mSideInfo[CUP_LOSER].id = playerId;
			cupResult.mSideInfo[CUP_LOSER].teamId = playerReport->getTeam();
			cupResult.mSideInfo[CUP_LOSER].score = playerReport->getScore();
		}
	}
}

void HeadtoHeadExtension::incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div)
{
	// map between division number (vector position) and type of title(vector value) 
	int titleMapId = getDefinesHelper()->GetIntSetting(DefinesHelper::TITLE_MAP, div);

	Stats::StatPeriodType statPeriod=getPeriodType();
	// increment global titles won
	svSPOTitlesWon[0]->iAfter[statPeriod]++;

	// increment titles won of each type 
	svSPOTitlesWon[titleMapId]->iAfter[statPeriod]++;
	
	SetHookEntityWonCompetition(titleMapId, entityId);

	if(div == getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS))
	{
		// top division, this is the league titles
		SetHookEntityWonLeagueTitle(entityId);
	}
}
void HeadtoHeadExtension::incrementDivCount(int64_t div)
{
	mMutex.Lock();

	mDivCountData.mDivCounts[div-1][INCREMENT]++;

	mMutex.Unlock();
}

void HeadtoHeadExtension::decrementDivCount(int64_t div)
{
	mMutex.Lock();

	mDivCountData.mDivCounts[div-1][DECREMENT]++;

	mMutex.Unlock();
}

void HeadtoHeadExtension::getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts)
{
	TRACE_LOG("[getDivCounts] getting div counts\n");

	mMutex.Lock();

	int32_t spNumDivisions = getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS);

	for (int i = 0; i < spNumDivisions; i++)
	{
		incrementCounts.push_back(mDivCountData.mDivCounts[i][INCREMENT]);
		decrementCounts.push_back(mDivCountData.mDivCounts[i][DECREMENT]);

		mDivCountData.mDivCounts[i][INCREMENT] = 0;
		mDivCountData.mDivCounts[i][DECREMENT] = 0;
	}

	mMutex.Unlock();
}
void HeadtoHeadExtension::SetHookEntityCurrentDiv(int64_t div, EntityId entityId )
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setCurrentDivision(static_cast<uint16_t>(getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS)-div+1));
	}
}

void HeadtoHeadExtension::SetHookEntityUpdateDiv(int64_t update, EntityId entityId )
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());

		switch (update)
		{
			case HOOK_DIV_REL: h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_RELEGATION); break;
			case HOOK_NOTHING: h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE); break;
			case HOOK_DIV_REP: h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_HOLD); break;
			case HOOK_DIV_PRO: h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_PROMOTION); break;
			default:
				h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE); break;
		}
	}
}

void HeadtoHeadExtension::SetHookEntityWonCompetition(int32_t titleid, EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setWonCompetition(static_cast<uint16_t>(titleid));
	}
}

void HeadtoHeadExtension::SetHookEntityWonLeagueTitle(EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setWonLeagueTitle(true);
	}
}

void HeadtoHeadExtension::SetHookEntitySeasonId(int64_t seasonid, EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setSeasonId(static_cast<uint16_t>(seasonid));
	}
}
void HeadtoHeadExtension::SetHookEntityCupId(int64_t cupid, EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setCup_id(static_cast<uint16_t>(cupid));
	}
}

void HeadtoHeadExtension::SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setGameNumber(static_cast<uint16_t>(gameNumber));
	}
}


OSDKGameReportBase::OSDKPlayerReport* HeadtoHeadExtension::getOsdkPlayerReport(EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* playerReport = NULL;

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.find(entityId);
	playerEnd = OsdkPlayerReportsMap.end();

	if (playerIter != playerEnd)
	{
		playerReport = playerIter->second;
	}

	return playerReport;
}



/*-------------------------------------------
		Clubs Seasonal Play
---------------------------------------------*/

ClubSeasonsExtension::DivCountData ClubSeasonsExtension::mDivCountData;

ClubSeasonsExtension::ClubSeasonsExtension()
{
}

ClubSeasonsExtension::~ClubSeasonsExtension()
{
}

void ClubSeasonsExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

const char8_t* ClubSeasonsExtension::getDivStatCategory()
{
	return "SPDivisionalClubsStats";
}

const char8_t* ClubSeasonsExtension::getOverallStatCategory()
{
	return "SPOverallClubsStats";
}

const char8_t* ClubSeasonsExtension::getDivCountStatCategory()
{
	return "SPClubsDivisionCount";
}

const char8_t* ClubSeasonsExtension::getLastOpponentStatCategory()
{
	return "";

}

void ClubSeasonsExtension::getEntityIds(EntityList& ids)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubIter, clubIterEnd;
	clubIter = OSDKClubReportMap.begin();
	clubIterEnd = OSDKClubReportMap.end();

	for (; clubIter != clubIterEnd; ++clubIter)
	{
		if (NeedToFilter(clubIter->first) == false)
		{
			ids.push_back(clubIter->first);
		}		
	}
}

void ClubSeasonsExtension::UpdateEntityFilterList(eastl::vector<int64_t> filteredIds)
{
	mEntityFilterList.clear();
	mEntityFilterList = filteredIds;
}

bool ClubSeasonsExtension::NeedToFilter(int64_t entityId)
{
	bool result = false;
	for (int i = 0; i < mEntityFilterList.size(); i++)
	{
		if (entityId == mEntityFilterList[i])
		{
			result = true;
			break;
		}
	}
	return result;
}

FifaCups::MemberType ClubSeasonsExtension::getMemberType()
{
	return FifaCups::FIFACUPS_MEMBERTYPE_CLUB;
}

IFifaSeasonalPlayExtension::GameResult ClubSeasonsExtension::getGameResult(EntityId entityId)
{
	GameResult result = LOSS;

	if (mExtensionData.mTieGame)
	{
		result = TIE;
	}
	else if (entityId == mExtensionData.mWinClubId)
	{
		result = WIN;
	}
	else
	{
		result = LOSS;
	}

	return result;
}

uint16_t ClubSeasonsExtension::getGoalsFor(EntityId entityId)
{
	uint16_t result = 0;

	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubClubReport = getOsdkClubReport(entityId);
	if (osdkClubClubReport != NULL)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubClubReport->getCustomClubClubReport());
		result = fifaClubsClubsReport.getCommonClubReport().getGoals();
	}

	return result;
}

uint16_t ClubSeasonsExtension::getGoalsAgainst(EntityId entityId)
{
	uint16_t result = 0;

	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubClubReport = getOsdkClubReport(entityId);
	if (osdkClubClubReport != NULL)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubClubReport->getCustomClubClubReport());
		result = fifaClubsClubsReport.getCommonClubReport().getGoalsConceded();
	}

	return result;
}

bool ClubSeasonsExtension::didUserFinish(EntityId entityId)
{
	bool result = false;

	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubClubReport = getOsdkClubReport(entityId);
	if (osdkClubClubReport != NULL)
	{
		result = (0 == osdkClubClubReport->getClubDisc()? true : false);
	}

	return result;
}

Stats::StatPeriodType ClubSeasonsExtension::getPeriodType()
{
	Stats::StatPeriodType statPeriod = Stats::STAT_PERIOD_ALL_TIME;		// default

	if(getDefinesHelper() != NULL)
	{
		statPeriod = getDefinesHelper()->GetPeriodType();
	}
	return statPeriod;
}

void ClubSeasonsExtension::getCupResult(CupResult& cupResult)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::iterator clubIter, clubIterEnd;
	clubIter = OSDKClubReportMap.begin();
	clubIterEnd = OSDKClubReportMap.end();

	Clubs::ClubId winClubId = static_cast<Clubs::ClubId>(mExtensionData.mWinClubId);
	Clubs::ClubId lossClubid = static_cast<Clubs::ClubId>(mExtensionData.mLossClubId);

	OSDKGameReportBase::OSDKGameReport& osdkGameReport = OsdkReport.getGameReport();
	OSDKClubGameReportBase::OSDKClubGameReport* osdkClubGameReport = static_cast<OSDKClubGameReportBase::OSDKClubGameReport*>(osdkGameReport.getCustomGameReport());
	FifaClubReportBase::FifaClubsGameReport* fifaClubReport = static_cast<FifaClubReportBase::FifaClubsGameReport*>(osdkClubGameReport->getCustomClubGameReport());

	for (; clubIter != clubIterEnd; ++clubIter)
	{
		Clubs::ClubId clubId = static_cast<Clubs::ClubId>(clubIter->first);

		OSDKClubGameReportBase::OSDKClubClubReport* osdkClubClubReport = clubIter->second;
		
		if (clubId == winClubId)
		{
			cupResult.mSideInfo[CUP_WINNER].id = clubId;
			cupResult.mSideInfo[CUP_WINNER].teamId = osdkClubClubReport->getTeam();
			cupResult.mSideInfo[CUP_WINNER].score = osdkClubClubReport->getScore();

			cupResult.mWentToPenalties = (fifaClubReport->getCommonGameReport().getWentToPk() == 1);
		}
		else if (clubId == lossClubid)
		{
			cupResult.mSideInfo[CUP_LOSER].id = clubId;
			cupResult.mSideInfo[CUP_LOSER].teamId = osdkClubClubReport->getTeam();
			cupResult.mSideInfo[CUP_LOSER].score = osdkClubClubReport->getScore();
		}
	}
}

void ClubSeasonsExtension::incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div)
{
	int titleMapId = getDefinesHelper()->GetIntSetting(DefinesHelper::TITLE_MAP, div);

	Stats::StatPeriodType statPeriod=getPeriodType();
	// increment global titles won
	svSPOTitlesWon[0]->iAfter[statPeriod]++;

	// increment titles won of each type 
	svSPOTitlesWon[titleMapId]->iAfter[statPeriod]++;
	SetHookEntityWonCompetition(titleMapId, entityId);

	if(div == getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS))
	{
		// top division, this is the league titles
		SetHookEntityWonLeagueTitle(entityId);
	}
}
void ClubSeasonsExtension::incrementDivCount(int64_t div)
{
	mMutex.Lock();

	mDivCountData.mDivCounts[div-1][INCREMENT]++;

	mMutex.Unlock();
}

void ClubSeasonsExtension::decrementDivCount(int64_t div)
{
	mMutex.Lock();

	mDivCountData.mDivCounts[div-1][DECREMENT]++;

	mMutex.Unlock();
}

void ClubSeasonsExtension::getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts)
{
	TRACE_LOG("[getDivCounts] getting div counts\n");

	mMutex.Lock();

	int32_t spNumDivisions = getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS);

	for (int i = 0; i < spNumDivisions; i++)
	{
		incrementCounts.push_back(mDivCountData.mDivCounts[i][INCREMENT]);
		decrementCounts.push_back(mDivCountData.mDivCounts[i][DECREMENT]);

		mDivCountData.mDivCounts[i][INCREMENT] = 0;
		mDivCountData.mDivCounts[i][DECREMENT] = 0;
	}

	mMutex.Unlock();
}

int64_t& ClubSeasonsExtension::getDivisionReference(EntityId entityId)
{
	if (entityId == mExtensionData.mWinClubId)
	{
		return mExtensionData.mWinDivId;
	}
	else
	{
		return mExtensionData.mLossDivId;
	}
}

int64_t ClubSeasonsExtension::getDivision(EntityId entityId)
{
	return getDivisionReference(entityId);
}


void ClubSeasonsExtension::SetHookEntityCurrentDiv(int64_t div, EntityId entityId )
{
	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(entityId);
	if (NULL != osdkClubReport)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());
		fifaClubsClubsReport.getSeasonalPlayData().setCurrentDivision(static_cast<uint16_t>(getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS)-div+1));

		int64_t& toUpdate = getDivisionReference(entityId);
		toUpdate = div;
	}
}
void ClubSeasonsExtension::SetHookEntityUpdateDiv(int64_t update, EntityId entityId )
{
	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(entityId);
	if (NULL != osdkClubReport)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());

		int64_t& toUpdate = getDivisionReference(entityId);

		switch (update)
		{
		case HOOK_DIV_REL: fifaClubsClubsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_RELEGATION); toUpdate++; break;
		case HOOK_NOTHING: fifaClubsClubsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE); break;
		case HOOK_DIV_REP: fifaClubsClubsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_HOLD); break;
		case HOOK_DIV_PRO: fifaClubsClubsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_PROMOTION);  toUpdate--; break;
		default:
			fifaClubsClubsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE); break;
		}
	}
}
void ClubSeasonsExtension::SetHookEntityWonCompetition(int32_t titleid, EntityId entityId)
{
	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(entityId);
	if (NULL != osdkClubReport)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());
		fifaClubsClubsReport.getSeasonalPlayData().setWonCompetition(static_cast<uint16_t>(titleid));
	}
}

void ClubSeasonsExtension::SetHookEntityWonLeagueTitle(EntityId entityId)
{
	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(entityId);
	if (NULL != osdkClubReport)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());
		fifaClubsClubsReport.getSeasonalPlayData().setWonLeagueTitle(true);
	}
}

void ClubSeasonsExtension::SetHookEntityWonCup(EntityId entityId)
{
	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(entityId);
	if (NULL != osdkClubReport)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());
		fifaClubsClubsReport.getSeasonalPlayData().setWonClubCup(true);
	}
}
void ClubSeasonsExtension::SetHookEntitySeasonId(int64_t seasonid, EntityId entityId)
{
	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(entityId);
	if (NULL != osdkClubReport)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());
		fifaClubsClubsReport.getSeasonalPlayData().setSeasonId(static_cast<uint16_t>(seasonid));
	}
}

void ClubSeasonsExtension::SetHookEntityCupId(int64_t cupid, EntityId entityId)
{

	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(entityId);
	if (NULL != osdkClubReport)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());
		fifaClubsClubsReport.getSeasonalPlayData().setCup_id(static_cast<uint16_t>(cupid));
	}
}

void ClubSeasonsExtension::SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId)
{

	OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(entityId);
	if (NULL != osdkClubReport)
	{
		FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());
		fifaClubsClubsReport.getSeasonalPlayData().setGameNumber(static_cast<uint16_t>(gameNumber));
	}
}


OSDKClubGameReportBase::OSDKClubClubReport* ClubSeasonsExtension::getOsdkClubReport(EntityId entityId)
{
	OSDKClubGameReportBase::OSDKClubClubReport* resultClubReport = NULL;

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*OsdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& OSDKClubReportMap = clubReport.getClubReports();

	Clubs::ClubId clubId = static_cast<Clubs::ClubId>(entityId);
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::iterator clubIter, clubIterEnd;
	clubIter = OSDKClubReportMap.find(clubId);
	clubIterEnd = OSDKClubReportMap.end();

	if (clubIter != clubIterEnd)
	{
		resultClubReport = clubIter->second;
	}

	return resultClubReport;
}

/*-------------------------------------------
		Coop Seasonal Play
---------------------------------------------*/

CoopSeasonsExtension::DivCountData CoopSeasonsExtension::mDivCountData;

CoopSeasonsExtension::CoopSeasonsExtension()
{
}

CoopSeasonsExtension::~CoopSeasonsExtension()
{
}

void CoopSeasonsExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

const char8_t* CoopSeasonsExtension::getDivStatCategory()
{
	return "SPDivisionalCoopStats";
}

const char8_t* CoopSeasonsExtension::getOverallStatCategory()
{
	return "SPOverallCoopStats";
}

const char8_t* CoopSeasonsExtension::getDivCountStatCategory()
{
	return "SPCoopDivisionCount";
}

const char8_t* CoopSeasonsExtension::getLastOpponentStatCategory()
{
	return "";

}

void CoopSeasonsExtension::getEntityIds(EntityList& ids)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport& fifaCoopSquadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaSquadReportsMap = fifaCoopSquadReport.getSquadReports();

	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::const_iterator squadIter, squadIterEnd;
	squadIter = fifaSquadReportsMap.begin();
	squadIterEnd = fifaSquadReportsMap.end();

	for (; squadIter != squadIterEnd; ++squadIter)
	{
		ids.push_back(squadIter->first);
	}
}

FifaCups::MemberType CoopSeasonsExtension::getMemberType()
{
	return FifaCups::FIFACUPS_MEMBERTYPE_COOP;
}

IFifaSeasonalPlayExtension::GameResult CoopSeasonsExtension::getGameResult(EntityId entityId)
{
	GameResult result = LOSS;

	if (mExtensionData.mTieGame)
	{
		result = TIE;
	}
	else if (entityId == mExtensionData.mWinCoopId)
	{
		result = WIN;
	}
	else
	{
		result = LOSS;
	}

	return result;
}

uint16_t CoopSeasonsExtension::getGoalsFor(EntityId entityId)
{
	uint16_t result = 0;

	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase != NULL)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());
		result = fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getGoals();
	}

	return result;
}

uint16_t CoopSeasonsExtension::getGoalsAgainst(EntityId entityId)
{
	uint16_t result = 0;

	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase != NULL)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());
		result = fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getGoalsConceded();
	}

	return result;
}

bool CoopSeasonsExtension::didUserFinish(EntityId entityId)
{
	bool result = false;

	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase != NULL)
	{
		result = (0 == fifaCoopSquadDetailsReportBase->getSquadDisc()? true : false);
	}

	return result;
}

Stats::StatPeriodType CoopSeasonsExtension::getPeriodType()
{
	Stats::StatPeriodType statPeriod = Stats::STAT_PERIOD_ALL_TIME;		// default

	if(getDefinesHelper() != NULL)
	{
		statPeriod = getDefinesHelper()->GetPeriodType();
	}
	return statPeriod;
}

void CoopSeasonsExtension::getCupResult(CupResult& cupResult)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport& fifaCoopSquadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaSquadReportsMap = fifaCoopSquadReport.getSquadReports();

	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::iterator squadIter, squadIterEnd;
	squadIter = fifaSquadReportsMap.begin();
	squadIterEnd = fifaSquadReportsMap.end();

	CoopSeason::CoopId winCoopId = static_cast<CoopSeason::CoopId>(mExtensionData.mWinCoopId);
	CoopSeason::CoopId lossCoopid = static_cast<CoopSeason::CoopId>(mExtensionData.mLossCoopId);

	OSDKGameReportBase::OSDKGameReport& osdkGameReport = OsdkReport.getGameReport();
	FifaCoopReportBase::FifaCoopGameReportBase* fifaCoopGameReportBase = static_cast<FifaCoopReportBase::FifaCoopGameReportBase*>(osdkGameReport.getCustomGameReport());
	FifaCoopSeasonsReport::FifaCoopSeasonsGameReport* fifaCoopSeasonsGameReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsGameReport*>(fifaCoopGameReportBase->getCustomCoopGameReport());

	for (; squadIter != squadIterEnd; ++squadIter)
	{
		CoopSeason::CoopId coopId = static_cast<CoopSeason::CoopId>(squadIter->first);

		FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = squadIter->second;
		
		if (coopId == winCoopId)
		{
			cupResult.mSideInfo[CUP_WINNER].id = coopId;
			cupResult.mSideInfo[CUP_WINNER].teamId = fifaCoopSquadDetailsReportBase->getTeam();
			cupResult.mSideInfo[CUP_WINNER].score = fifaCoopSquadDetailsReportBase->getScore();

			cupResult.mWentToPenalties = (fifaCoopSeasonsGameReport->getCommonGameReport().getWentToPk() == 1);
		}
		else if (coopId == lossCoopid)
		{
			cupResult.mSideInfo[CUP_LOSER].id = coopId;
			cupResult.mSideInfo[CUP_LOSER].teamId = fifaCoopSquadDetailsReportBase->getTeam();
			cupResult.mSideInfo[CUP_LOSER].score = fifaCoopSquadDetailsReportBase->getScore();
		}
	}
}

void CoopSeasonsExtension::incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div)
{
	int titleMapId = getDefinesHelper()->GetIntSetting(DefinesHelper::TITLE_MAP, div);

	Stats::StatPeriodType statPeriod=getPeriodType();
	// increment global titles won
	svSPOTitlesWon[0]->iAfter[statPeriod]++;

	// increment titles won of each type 
	svSPOTitlesWon[titleMapId]->iAfter[statPeriod]++;
	SetHookEntityWonCompetition(titleMapId, entityId);

	if(div == getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS))
	{
		// top division, this is the league titles
		SetHookEntityWonLeagueTitle(entityId);
	}
}
void CoopSeasonsExtension::incrementDivCount(int64_t div)
{
	mMutex.Lock();

	mDivCountData.mDivCounts[div-1][INCREMENT]++;

	mMutex.Unlock();
}

void CoopSeasonsExtension::decrementDivCount(int64_t div)
{
	mMutex.Lock();

	mDivCountData.mDivCounts[div-1][DECREMENT]++;

	mMutex.Unlock();
}

void CoopSeasonsExtension::getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts)
{
	TRACE_LOG("[getDivCounts] getting div counts\n");

	mMutex.Lock();

	int32_t spNumDivisions = getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS);

	for (int i = 0; i < spNumDivisions; i++)
	{
		incrementCounts.push_back(mDivCountData.mDivCounts[i][INCREMENT]);
		decrementCounts.push_back(mDivCountData.mDivCounts[i][DECREMENT]);

		mDivCountData.mDivCounts[i][INCREMENT] = 0;
		mDivCountData.mDivCounts[i][DECREMENT] = 0;
	}

	mMutex.Unlock();
}

void CoopSeasonsExtension::SetHookEntityCurrentDiv(int64_t div, EntityId entityId )
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());
		fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setCurrentDivision(static_cast<uint16_t>(getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::NUM_DIVISIONS)-div+1));
	}
}
void CoopSeasonsExtension::SetHookEntityUpdateDiv(int64_t update, EntityId entityId )
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());

		switch (update)
		{
		case HOOK_DIV_REL: fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_RELEGATION); break;
		case HOOK_NOTHING: fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE); break;
		case HOOK_DIV_REP: fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_HOLD); break;
		case HOOK_DIV_PRO: fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_PROMOTION); break;
		default:
			fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE); break;
		}
	}
}
void CoopSeasonsExtension::SetHookEntityWonCompetition(int32_t titleid, EntityId entityId)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());
		fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setWonCompetition(static_cast<uint16_t>(titleid));
	}
}

void CoopSeasonsExtension::SetHookEntityWonLeagueTitle(EntityId entityId)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());
		fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setWonLeagueTitle(true);
	}
}


void CoopSeasonsExtension::SetHookEntitySeasonId(int64_t seasonid, EntityId entityId)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());
		fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setSeasonId(static_cast<uint16_t>(seasonid));
	}
}
void CoopSeasonsExtension::SetHookEntityCupId(int64_t cupid, EntityId entityId)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());
		fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setCup_id(static_cast<uint16_t>(cupid));
	}
}

void CoopSeasonsExtension::SetHookEntityGameNumber(int64_t gameNumber, EntityId entityId)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadReport(entityId);
	if (fifaCoopSquadDetailsReportBase)
	{
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());
		fifaCoopSeasonsSquadDetailsReport.getSeasonalPlayData().setGameNumber(static_cast<uint16_t>(gameNumber));
	}
}

FifaCoopReportBase::FifaCoopSquadDetailsReportBase* CoopSeasonsExtension::getSquadReport(EntityId entityId)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* resultClubReport = NULL;

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	FifaCoopReportBase::FifaCoopSquadReport& fifaCoopSquadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport&>(*OsdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaSquadReportsMap = fifaCoopSquadReport.getSquadReports();

	CoopSeason::CoopId coopId = static_cast<CoopSeason::CoopId>(entityId);
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::iterator squadIter, squadIterEnd;
	squadIter = fifaSquadReportsMap.find(coopId);
	squadIterEnd = fifaSquadReportsMap.end();

	if (squadIter != squadIterEnd)
	{
		resultClubReport = squadIter->second;
	}

	return resultClubReport;
}

/*============================== Live Competition Extension ==============================*/

LiveCompHeadtoHeadExtension::DivCountData LiveCompHeadtoHeadExtension::mDivCountData;

LiveCompHeadtoHeadExtension::LiveCompHeadtoHeadExtension()
{
}

LiveCompHeadtoHeadExtension::~LiveCompHeadtoHeadExtension()
{
}

void LiveCompHeadtoHeadExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

const char8_t* LiveCompHeadtoHeadExtension::getDivStatCategory()
{
	switch(mLiveCompDefinesHelper.GetPeriodType())
	{
	case Stats::STAT_PERIOD_ALL_TIME:
		return LIVE_COMP_SP_DIVISIONAL_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_ALLTIME;
		break;
	case Stats::STAT_PERIOD_MONTHLY:
		return LIVE_COMP_SP_DIVISIONAL_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_MONTHLY;
		break;
	case Stats::STAT_PERIOD_WEEKLY:
		return LIVE_COMP_SP_DIVISIONAL_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_WEEKLY;
		break;
	case Stats::STAT_PERIOD_DAILY:
		return LIVE_COMP_SP_DIVISIONAL_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_DAILY;
		break;
	default:
		break;
	}
	return "";
}

const char8_t* LiveCompHeadtoHeadExtension::getOverallStatCategory()
{
	switch(mLiveCompDefinesHelper.GetPeriodType())
	{
	case Stats::STAT_PERIOD_ALL_TIME:
		return LIVE_COMP_SP_OVERALL_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_ALLTIME;
		break;
	case Stats::STAT_PERIOD_MONTHLY:
		return LIVE_COMP_SP_OVERALL_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_MONTHLY;
		break;
	case Stats::STAT_PERIOD_WEEKLY:
		return LIVE_COMP_SP_OVERALL_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_WEEKLY;
		break;
	case Stats::STAT_PERIOD_DAILY:
		return LIVE_COMP_SP_OVERALL_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_DAILY;
		break;
	default:
		break;
	}
	return "";
}

const char8_t* LiveCompHeadtoHeadExtension::getDivCountStatCategory()
{
	switch(mLiveCompDefinesHelper.GetPeriodType())
	{
	case Stats::STAT_PERIOD_ALL_TIME:
		return LIVE_COMP_SP_DIVCOUNT_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_ALLTIME;
		break;
	case Stats::STAT_PERIOD_MONTHLY:
		return LIVE_COMP_SP_DIVCOUNT_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_MONTHLY;
		break;
	case Stats::STAT_PERIOD_WEEKLY:
		return LIVE_COMP_SP_DIVCOUNT_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_WEEKLY;
		break;
	case Stats::STAT_PERIOD_DAILY:
		return LIVE_COMP_SP_DIVCOUNT_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_DAILY;
		break;
	default:
		break;
	}
	return "";
}

const char8_t* LiveCompHeadtoHeadExtension::getLastOpponentStatCategory()
{
	return getOverallStatCategory();
}

void LiveCompHeadtoHeadExtension::getKeyScopes(Stats::ScopeNameValueMap& keyscopes)
{
	keyscopes.insert(eastl::make_pair(SCOPENAME_COMPETITIONID, mExtensionData.mCompetitionId));
}

void LiveCompHeadtoHeadExtension::getEntityIds(EntityList& ids)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		ids.push_back(playerIter->first);
	}
}

FifaCups::MemberType LiveCompHeadtoHeadExtension::getMemberType()
{
	return FifaCups::FIFACUPS_MEMBERTYPE_USER;
}

IFifaSeasonalPlayExtension::GameResult LiveCompHeadtoHeadExtension::getGameResult(EntityId entityId)
{
	GameResult result = LOSS;

	if (mExtensionData.mTieGame)
	{
		result = TIE;
	}
	else if (mExtensionData.mWinnerSet->find(entityId) != mExtensionData.mWinnerSet->end())
	{
		result = WIN;
	}
	else
	{
		result = LOSS;
	}

	return result;
}

EntityId LiveCompHeadtoHeadExtension::getOpponentId(EntityId entityId)
{
	EntityId opponentId = -1;

	// assumes mWinnerSet/mLoserSet each has a single entry
	if(mExtensionData.mWinnerSet->find(entityId) != mExtensionData.mWinnerSet->end())
	{
		opponentId = *(mExtensionData.mLoserSet->begin());
	}
	else
	if(mExtensionData.mLoserSet->find(entityId) != mExtensionData.mLoserSet->end())
	{
		opponentId = *(mExtensionData.mWinnerSet->begin());
	}
	else
	{
		// iterate through two-entry mTieSet to get opponent
		for(ResultSet::const_iterator it = mExtensionData.mTieSet->begin(); it != mExtensionData.mTieSet->end(); ++it)
		{
			if(*it == entityId)
			{
				// skip entry corresponding to self
				continue;
			}
			opponentId = *it;
		}
	}

	return opponentId;
}

uint16_t LiveCompHeadtoHeadExtension::getGoalsFor(EntityId entityId)
{
	uint16_t result = 0;

	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (osdkPlayerReport != NULL)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		result = h2hPlayerReport.getCommonPlayerReport().getGoals();
	}

	return result;
}

uint16_t LiveCompHeadtoHeadExtension::getGoalsAgainst(EntityId entityId)
{
	uint16_t result = 0;

	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (osdkPlayerReport != NULL)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		result = h2hPlayerReport.getCommonPlayerReport().getGoalsConceded();
	}

	return result;
}

bool LiveCompHeadtoHeadExtension::didUserFinish(EntityId entityId)
{
	bool result = false;

	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (osdkPlayerReport != NULL)
	{
		const CollatedGameReport::DnfStatusMap& DnfStatusMap = mExtensionData.mProcessedReport->getDnfStatusMap();
		result = (0 == DnfStatusMap.find(entityId)->second);
		uint32_t discResult = osdkPlayerReport->getUserResult();

		if (false == result && ((GAME_RESULT_COMPLETE_REGULATION == discResult) ||
			(GAME_RESULT_COMPLETE_OVERTIME == discResult)))
		{
			result = true;
		}
	}

	return result;
}

Stats::StatPeriodType LiveCompHeadtoHeadExtension::getPeriodType()
{
	Stats::StatPeriodType statPeriod = Stats::STAT_PERIOD_MONTHLY;			// default

	if(getDefinesHelper() != NULL)
	{
		statPeriod = getDefinesHelper()->GetPeriodType();
	}
	return statPeriod;
}

void LiveCompHeadtoHeadExtension::getCupResult(CupResult& cupResult)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		GameManager::PlayerId playerId = playerIter->first;
		OSDKGameReportBase::OSDKPlayerReport* playerReport = playerIter->second;
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport->getCustomPlayerReport());
		//result = h2hPlayerReport.getCommonPlayerReport().getGoalsConceded();

		if (mExtensionData.mWinnerSet->find(playerId) != mExtensionData.mWinnerSet->end())
		{
			cupResult.mSideInfo[CUP_WINNER].id = playerId;
			cupResult.mSideInfo[CUP_WINNER].teamId = playerReport->getTeam();
			cupResult.mSideInfo[CUP_WINNER].score = playerReport->getScore();

			cupResult.mWentToPenalties = h2hPlayerReport.getH2HCustomPlayerData().getWentToPk() == 1;
		}
		else if (mExtensionData.mLoserSet->find(playerId) != mExtensionData.mLoserSet->end())
		{
			cupResult.mSideInfo[CUP_LOSER].id = playerId;
			cupResult.mSideInfo[CUP_LOSER].teamId = playerReport->getTeam();
			cupResult.mSideInfo[CUP_LOSER].score = playerReport->getScore();
		}
	}
}

void LiveCompHeadtoHeadExtension::incrementTitlesWon(EntityId entityId, StatValueUtil::StatValue ** svSPOTitlesWon, int64_t div)
{
	int titleMapId = getDefinesHelper()->GetIntSetting(DefinesHelper::TITLE_MAP, div);

	Stats::StatPeriodType statPeriod=getPeriodType();
	// increment global titles won
	svSPOTitlesWon[0]->iAfter[statPeriod]++;

	// increment titles won of each type 
	svSPOTitlesWon[titleMapId]->iAfter[statPeriod]++;
	SetHookEntityWonCompetition(titleMapId, entityId);

	if(div == getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS))
	{
		// top division, this is the league titles
		SetHookEntityWonLeagueTitle(entityId);
	}
}
void LiveCompHeadtoHeadExtension::incrementDivCount(int64_t div)
{
	mMutex.Lock();

	mDivCountData.mDivCounts[div-1][INCREMENT]++;

	mMutex.Unlock();
}

void LiveCompHeadtoHeadExtension::decrementDivCount(int64_t div)
{
	mMutex.Lock();

	mDivCountData.mDivCounts[div-1][DECREMENT]++;

	mMutex.Unlock();
}

void LiveCompHeadtoHeadExtension::getDivCounts(eastl::vector<int>& incrementCounts, eastl::vector<int>& decrementCounts)
{
	TRACE_LOG("[getDivCounts] getting div counts\n");

	mMutex.Lock();

	int32_t spNumDivisions = getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS);

	for (int i = 0; i < spNumDivisions; i++)
	{
		incrementCounts.push_back(mDivCountData.mDivCounts[i][INCREMENT]);
		decrementCounts.push_back(mDivCountData.mDivCounts[i][DECREMENT]);

		mDivCountData.mDivCounts[i][INCREMENT] = 0;
		mDivCountData.mDivCounts[i][DECREMENT] = 0;
	}

	mMutex.Unlock();
}

void LiveCompHeadtoHeadExtension::SetHookEntityCurrentDiv(int64_t div, EntityId entityId )
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setCurrentDivision(static_cast<uint16_t>(getDefinesHelper()->GetIntSetting(Blaze::GameReporting::DefinesHelper::NUM_DIVISIONS)-div+1));
	}
}

void LiveCompHeadtoHeadExtension::SetHookEntityUpdateDiv(int64_t update, EntityId entityId )
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());

		switch (update)
		{
		case HOOK_DIV_REL: h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_RELEGATION); break;
		case HOOK_NOTHING: h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE); break;
		case HOOK_DIV_REP: h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_HOLD); break;
		case HOOK_DIV_PRO: h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::SEASON_PROMOTION); break;
		default:
			h2hPlayerReport.getSeasonalPlayData().setUpdateDivision(Blaze::GameReporting::Fifa::NO_UPDATE); break;
		}

	}
}

void LiveCompHeadtoHeadExtension::SetHookEntityWonCompetition(int32_t titleid, EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setWonCompetition(static_cast<uint16_t>(titleid));
	}
}

void LiveCompHeadtoHeadExtension::SetHookEntityWonLeagueTitle(EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setWonLeagueTitle(true);
	}
}

void LiveCompHeadtoHeadExtension::SetHookEntitySeasonId(int64_t seasonid, EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = getOsdkPlayerReport(entityId);
	if (NULL != osdkPlayerReport)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());
		h2hPlayerReport.getSeasonalPlayData().setSeasonId(static_cast<uint16_t>(seasonid));
	}
}

void LiveCompHeadtoHeadExtension::SetHookEntityRankingPoints(int64_t value, EntityId entityId)
{
	mRankPointsMap.insert(eastl::make_pair(entityId, value));
}

int64_t LiveCompHeadtoHeadExtension::GetHookEntityRankingPoints(EntityId entityId)
{
	int64_t rankingPoints = 0;
	
	for(RankingPointsMap::iterator it = mRankPointsMap.begin(); it != mRankPointsMap.end(); ++it)
	{
		if(entityId == it->first)
		{
			rankingPoints = it->second;
			break;
		}
	}
		
	return rankingPoints;
}

OSDKGameReportBase::OSDKPlayerReport* LiveCompHeadtoHeadExtension::getOsdkPlayerReport(EntityId entityId)
{
	OSDKGameReportBase::OSDKPlayerReport* playerReport = NULL;

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.find(entityId);
	playerEnd = OsdkPlayerReportsMap.end();

	if (playerIter != playerEnd)
	{
		playerReport = playerIter->second;
	}

	return playerReport;
}

}   // namespace GameReporting
}   // namespace Blaze
