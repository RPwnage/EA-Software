/*************************************************************************************************/
/*!
    \file   fifa_elo_rpghybridskillextensions.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "fifa_elo_rpghybridskillextensions.h"
#include "fifa_elo_rpghybridskilldefines.h"

#include "customcode/component/gamereporting/osdk/gamecommon.h"
#include "customcode/component/gamereporting/fifa/tdf/fifah2hreport.h"
#include "customcode/component/gamereporting/fifa/tdf/fifaclubreport.h"
#include "customcode/component/gamereporting/fifa/fifalivecompetitionsdefines.h"

#include "component/clubs/clubsslaveimpl.h"
#include "component/clubs/tdf/clubs.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

HeadtoHeadEloExtension::HeadtoHeadEloExtension()
{
}

HeadtoHeadEloExtension::~HeadtoHeadEloExtension()
{
}

void HeadtoHeadEloExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

void HeadtoHeadEloExtension::getEloStatUpdateInfo(CategoryInfoList& categoryInfoList)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	int count = 0;
	CategoryInfo categoryInfo;
	EntityInfo* info = NULL;
	categoryInfo.category = "NormalGameStats";
	for(; playerIter != playerEnd; ++playerIter)
	{
		OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = playerIter->second;
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*osdkPlayerReport->getCustomPlayerReport());

		if (count % 2 == 0)
		{
			info = &categoryInfo.entityA;
		}
		else
		{
			info = &categoryInfo.entityB;
		}

		info->entityId = playerIter->first;

		ScopePair scopePair;
		scopePair.name = SCOPENAME_ACCOUNTCOUNTRY;
		scopePair.value = osdkPlayerReport->getAccountCountry();
		info->scopeList.push_back(scopePair);
		scopePair.name = SCOPENAME_CONTROLS;
		scopePair.value = h2hPlayerReport.getH2HCustomPlayerData().getControls();
		info->scopeList.push_back(scopePair);

		count++;
	}

	categoryInfoList.push_back(categoryInfo);
}

void HeadtoHeadEloExtension::generateAttributeMap(EntityId id, Collections::AttributeMap& map)
{
	OSDKGameReportBase::OSDKPlayerReport* playerReport = getOsdkPlayerReport(id);
	if (playerReport != NULL)
	{
		OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport->getCustomPlayerReport());

		eastl::string temp;
		map[ATTRIBNAME_GOALS]			= convertToString(h2hPlayerReport.getCommonPlayerReport().getGoals(), temp);
		map[ATTRIBNAME_PASSATTEMPTS]	= convertToString(h2hPlayerReport.getCommonPlayerReport().getPassAttempts(), temp);
		map[ATTRIBNAME_PASSESMADE]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getPassesMade(), temp);
		map[ATTRIBNAME_TACKLEATTEMPTS]	= convertToString(h2hPlayerReport.getCommonPlayerReport().getTackleAttempts(), temp);
		map[ATTRIBNAME_TACKLESMADE]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getTacklesMade(), temp);
		map[ATTRIBNAME_POSSESSION]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getPossession(), temp);
		map[ATTRIBNAME_SHOTS]			= convertToString(h2hPlayerReport.getCommonPlayerReport().getShots(), temp);
		map[ATTRIBNAME_SHOTSONGOAL]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getShotsOnGoal(), temp);
		map[ATTRIBNAME_FOULS]			= convertToString(h2hPlayerReport.getCommonPlayerReport().getFouls(), temp);
		map[ATTRIBNAME_YELLOWCARDS]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getYellowCard(), temp);
		map[ATTRIBNAME_REDCARDS]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getRedCard(), temp);
		map[ATTRIBNAME_CORNERS]			= convertToString(h2hPlayerReport.getCommonPlayerReport().getCorners(), temp);
		map[ATTRIBNAME_OFFSIDES]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getOffsides(), temp);
		map[ATTRIBNAME_TEAMRATING]		= convertToString(h2hPlayerReport.getH2HCustomPlayerData().getTeamrating(), temp);
		map[ATTRIBNAME_TEAM]			= convertToString(playerReport->getTeam(), temp);
		map[ATTRIBNAME_HOME]			= convertToString(playerReport->getHome(), temp);
		map[ATTRIBNAME_GAMETIME]		= convertToString(osdkReport.getGameReport().getGameTime(), temp);
		map[ATTRIBNAME_CLUBDISC]		= convertToString(0, temp);
		map[ATTRIBNAME_DISC]			= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::DISCONNECT], temp);
		map[ATTRIBNAME_STATPERC]		= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::STAT_PERC], temp);//calculated damping percent
		map[STATNAME_RESULT]			= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::RESULT], temp);//calculated WLT LobbySkill
	}
}

bool HeadtoHeadEloExtension::isClubs()
{
	return false;
}

OSDKGameReportBase::OSDKPlayerReport* HeadtoHeadEloExtension::getOsdkPlayerReport(EntityId entityId)
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

char* HeadtoHeadEloExtension::convertToString(int64_t value, eastl::string& stringResult)
{
	stringResult.clear();
	stringResult.sprintf("%" PRId64"", value);

	return const_cast<char*>(stringResult.c_str());
}

/* ===========================  Club ELO Extension =========================== */

ClubEloExtension::ClubEloExtension()
{
}

ClubEloExtension::~ClubEloExtension()
{
}

void ClubEloExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

void ClubEloExtension::getEloStatUpdateInfo(CategoryInfoList& categoryInfoList)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());

	OSDKClubGameReportBase::OSDKClubReport* clubReports = static_cast<OSDKClubGameReportBase::OSDKClubReport*>(osdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& clubReportsMap = clubReports->getClubReports();

	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubsIter, clubsEnd;
	clubsIter = clubReportsMap.begin();
	clubsEnd = clubReportsMap.end();

	int count = 0;
	CategoryInfo categoryInfo;
	EntityInfo* info = NULL;
	categoryInfo.category = "ClubRankStats";
	for(; clubsIter != clubsEnd; ++clubsIter)
	{
		if (count % 2 == 0)
		{
			info = &categoryInfo.entityA;
		}
		else
		{
			info = &categoryInfo.entityB;
		}

		info->entityId = clubsIter->first;
		count++;

		TRACE_LOG("[ClubEloExtension].getEloStatUpdateInfo() adding entityId " << info->entityId << " for category " << categoryInfo.category << " ");

	}
	categoryInfoList.push_back(categoryInfo);

	count = 0;
	info = NULL;
	categoryInfo.category = "ClubRankLeagueStats";
	categoryInfo.entityA.scopeList.clear();
	categoryInfo.entityB.scopeList.clear();
	clubsIter = clubReportsMap.begin();
	clubsEnd = clubReportsMap.end();
	for(; clubsIter != clubsEnd; ++clubsIter)
	{
		if (count % 2 == 0)
		{
			info = &categoryInfo.entityA;
		}
		else
		{
			info = &categoryInfo.entityB;
		}
		info->entityId = clubsIter->first;

		Clubs::ClubSettings clubSettings;
		if (mExtensionData.mUpdateClubsUtil->getClubSettings(clubsIter->first, clubSettings))
		{
			ScopePair scopePair;
			scopePair.name = SCOPENAME_SEASON;
			scopePair.value = clubSettings.getSeasonLevel();
			info->scopeList.push_back(scopePair);
		}

		TRACE_LOG("[ClubEloExtension].getEloStatUpdateInfo() adding entityId " << info->entityId << " for category " << categoryInfo.category << " ");

		count++;
	}

	categoryInfoList.push_back(categoryInfo);
}

void ClubEloExtension::generateAttributeMap(EntityId id, Collections::AttributeMap& map)
{
	OSDKClubGameReportBase::OSDKClubClubReport* clubReport = getOsdkClubClubReport(id);
	if (clubReport != NULL)
	{
		OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
		FifaClubReportBase::FifaClubsClubReport& fifaClubReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*clubReport->getCustomClubClubReport());
		int numOfPlayers = getNumberOfPlayersPlayed(clubReport->getTeam(), &osdkReport);

		eastl::string temp;
		map[ATTRIBNAME_GOALS]			= convertToString(fifaClubReport.getCommonClubReport().getGoals(), temp);
		map[ATTRIBNAME_PASSATTEMPTS]	= convertToString(fifaClubReport.getCommonClubReport().getPassAttempts(), temp);
		map[ATTRIBNAME_PASSESMADE]		= convertToString(fifaClubReport.getCommonClubReport().getPassesMade(), temp);
		map[ATTRIBNAME_TACKLEATTEMPTS]	= convertToString(fifaClubReport.getCommonClubReport().getTackleAttempts(), temp);
		map[ATTRIBNAME_TACKLESMADE]		= convertToString(fifaClubReport.getCommonClubReport().getTacklesMade(), temp);
		map[ATTRIBNAME_POSSESSION]		= convertToString(fifaClubReport.getCommonClubReport().getPossession(), temp);
		map[ATTRIBNAME_SHOTS]			= convertToString(fifaClubReport.getCommonClubReport().getShots(), temp);
		map[ATTRIBNAME_SHOTSONGOAL]		= convertToString(fifaClubReport.getCommonClubReport().getShotsOnGoal(), temp);
		map[ATTRIBNAME_FOULS]			= convertToString(fifaClubReport.getCommonClubReport().getFouls(), temp);
		map[ATTRIBNAME_YELLOWCARDS]		= convertToString(fifaClubReport.getCommonClubReport().getYellowCard(), temp);
		map[ATTRIBNAME_REDCARDS]		= convertToString(fifaClubReport.getCommonClubReport().getRedCard(), temp);
		map[ATTRIBNAME_CORNERS]			= convertToString(fifaClubReport.getCommonClubReport().getCorners(), temp);
		map[ATTRIBNAME_OFFSIDES]		= convertToString(fifaClubReport.getCommonClubReport().getOffsides(), temp);
		map[ATTRIBNAME_TEAMRATING]		= convertToString(fifaClubReport.getTeamrating(), temp);
		map[ATTRIBNAME_TEAM]			= convertToString(clubReport->getTeam(), temp);
		map[ATTRIBNAME_HOME]			= convertToString(clubReport->getHome(), temp);
		map[ATTRIBNAME_PLAYERS]			= convertToString(numOfPlayers, temp);
		map[ATTRIBNAME_GAMETIME]		= convertToString(osdkReport.getGameReport().getGameTime(), temp);
		map[ATTRIBNAME_CLUBDISC]		= convertToString(clubReport->getClubDisc(), temp);
		map[ATTRIBNAME_DISC]			= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::DISCONNECT], temp);
		map[ATTRIBNAME_STATPERC]		= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::STAT_PERC], temp);//calculated damping percent
		map[STATNAME_RESULT]			= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::RESULT], temp);//calculated WLT LobbySkill
	}
}

bool ClubEloExtension::isClubs()
{
	return true;
}

OSDKClubGameReportBase::OSDKClubClubReport* ClubEloExtension::getOsdkClubClubReport(EntityId entityId)
{
	OSDKClubGameReportBase::OSDKClubClubReport* clubReport = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());

	OSDKClubGameReportBase::OSDKClubReport* clubReports = static_cast<OSDKClubGameReportBase::OSDKClubReport*>(osdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& clubReportsMap = clubReports->getClubReports();

	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::iterator clubsIter, clubsEnd;
	clubsIter = clubReportsMap.find(entityId);
	clubsEnd = clubReportsMap.end();

	if (clubsIter != clubsEnd)
	{
		clubReport = clubsIter->second;
	}

	return clubReport;
}

char* ClubEloExtension::convertToString(int64_t value, eastl::string& stringResult)
{
	stringResult.clear();
	stringResult.sprintf("%" PRId64"", value);

	return const_cast<char*>(stringResult.c_str());
}

int ClubEloExtension::getNumberOfPlayersPlayed(uint32_t teamId, const OSDKGameReportBase::OSDKReport *osdkReport)
{
	const OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap &playerMap = osdkReport->getPlayerReports();
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerItr = playerMap.begin();
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerItrEnd = playerMap.end();

	int numPlayerPlayed = 0;
	for(; playerItr != playerItrEnd; playerItr++)
	{
		if(teamId == playerItr->second->getTeam())
		{
			numPlayerPlayed++;
		}
	}

	return numPlayerPlayed;
}

/* ===========================  Coop ELO Extension =========================== */

CoopEloExtension::CoopEloExtension()
{
}

CoopEloExtension::~CoopEloExtension()
{
}

void CoopEloExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

void CoopEloExtension::getEloStatUpdateInfo(CategoryInfoList& categoryInfoList)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());

	FifaCoopReportBase::FifaCoopSquadReport* fifaCoopSquadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport*>(osdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaSquadReportsMap = fifaCoopSquadReport->getSquadReports();

	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::const_iterator squadIter, squadEnd;
	squadIter = fifaSquadReportsMap.begin();
	squadEnd = fifaSquadReportsMap.end();

	int count = 0;
	CategoryInfo categoryInfo;
	EntityInfo* info = NULL;
	categoryInfo.category = "CoopRankStats";
	for(; squadIter != squadEnd; ++squadIter)
	{
		if (count % 2 == 0)
		{
			info = &categoryInfo.entityA;
		}
		else
		{
			info = &categoryInfo.entityB;
		}

		info->entityId = squadIter->first;
		count++;

		TRACE_LOG("[CoopEloExtension].getEloStatUpdateInfo() adding entityId " << info->entityId << " for category " << categoryInfo.category << " ");

	}
	categoryInfoList.push_back(categoryInfo);

	//count = 0;
	//info = NULL;
	//categoryInfo.category = "ClubRankLeagueStats";
	//categoryInfo.entityA.scopeList.clear();
	//categoryInfo.entityB.scopeList.clear();
	//squadIter = fifaSquadReportsMap.begin();
	//squadEnd = fifaSquadReportsMap.end();
	//for(; squadIter != squadEnd; ++squadIter)
	//{
	//	if (count % 2 == 0)
	//	{
	//		info = &categoryInfo.entityA;
	//	}
	//	else
	//	{
	//		info = &categoryInfo.entityB;
	//	}
	//	info->entityId = squadIter->first;

	//	Clubs::ClubSettings clubSettings;
	//	if (mExtensionData.mUpdateClubsUtil->getClubSettings(clubsIter->first, clubSettings))
	//	{
	//		ScopePair scopePair;
	//		scopePair.name = SCOPENAME_SEASON;
	//		scopePair.value = clubSettings.getSeasonLevel();
	//		info->scopeList.push_back(scopePair);
	//	}

	//	TRACE_LOG("[CoopEloExtension].getEloStatUpdateInfo() adding entityId " << info->entityId << " for category " << categoryInfo.category << " ");

	//	count++;
	//}

	//categoryInfoList.push_back(categoryInfo);
}

void CoopEloExtension::generateAttributeMap(EntityId id, Collections::AttributeMap& map)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* fifaCoopSquadDetailsReportBase = getSquadDetailsReport(id);
	if (fifaCoopSquadDetailsReportBase != NULL)
	{
		OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
		FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport& fifaCoopSeasonsSquadDetailsReport = static_cast<FifaCoopSeasonsReport::FifaCoopSeasonsSquadDetailsReport&>(*fifaCoopSquadDetailsReportBase->getCustomSquadDetailsReport());

		eastl::string temp;
		map[ATTRIBNAME_GOALS]			= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getGoals(), temp);
		map[ATTRIBNAME_PASSATTEMPTS]	= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getPassAttempts(), temp);
		map[ATTRIBNAME_PASSESMADE]		= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getPassesMade(), temp);
		map[ATTRIBNAME_TACKLEATTEMPTS]	= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getTackleAttempts(), temp);
		map[ATTRIBNAME_TACKLESMADE]		= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getTacklesMade(), temp);
		map[ATTRIBNAME_POSSESSION]		= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getPossession(), temp);
		map[ATTRIBNAME_SHOTS]			= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getShots(), temp);
		map[ATTRIBNAME_SHOTSONGOAL]		= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getShotsOnGoal(), temp);
		map[ATTRIBNAME_FOULS]			= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getFouls(), temp);
		map[ATTRIBNAME_YELLOWCARDS]		= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getYellowCard(), temp);
		map[ATTRIBNAME_REDCARDS]		= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getRedCard(), temp);
		map[ATTRIBNAME_CORNERS]			= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getCorners(), temp);
		map[ATTRIBNAME_OFFSIDES]		= convertToString(fifaCoopSeasonsSquadDetailsReport.getCommonClubReport().getOffsides(), temp);
		map[ATTRIBNAME_TEAMRATING]		= convertToString(fifaCoopSeasonsSquadDetailsReport.getTeamrating(), temp);
		map[ATTRIBNAME_TEAM]			= convertToString(fifaCoopSquadDetailsReportBase->getTeam(), temp);
		map[ATTRIBNAME_HOME]			= convertToString(fifaCoopSquadDetailsReportBase->getHome(), temp);
		map[ATTRIBNAME_GAMETIME]		= convertToString(osdkReport.getGameReport().getGameTime(), temp);
		map[ATTRIBNAME_CLUBDISC]		= convertToString(fifaCoopSquadDetailsReportBase->getSquadDisc(), temp);
		map[ATTRIBNAME_DISC]			= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::DISCONNECT], temp);
		map[ATTRIBNAME_STATPERC]		= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::STAT_PERC], temp);//calculated damping percent
		map[STATNAME_RESULT]			= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::RESULT], temp);//calculated WLT LobbySkill
	}
}

bool CoopEloExtension::isClubs()
{
	return true;
}

FifaCoopReportBase::FifaCoopSquadDetailsReportBase* CoopEloExtension::getSquadDetailsReport(EntityId entityId)
{
	FifaCoopReportBase::FifaCoopSquadDetailsReportBase* squadReport = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());

	FifaCoopReportBase::FifaCoopSquadReport* fifaCoopSquadReport = static_cast<FifaCoopReportBase::FifaCoopSquadReport*>(osdkReport.getTeamReports());
	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap& fifaSquadReportsMap = fifaCoopSquadReport->getSquadReports();

	FifaCoopReportBase::FifaCoopSquadReport::FifaSquadReportsMap::iterator squadIter, squadEnd;
	squadIter = fifaSquadReportsMap.find(entityId);
	squadEnd = fifaSquadReportsMap.end();

	if (squadIter != squadEnd)
	{
		squadReport = squadIter->second;
	}

	return squadReport;
}

char* CoopEloExtension::convertToString(int64_t value, eastl::string& stringResult)
{
	stringResult.clear();
	stringResult.sprintf("%" PRId64"", value);

	return const_cast<char*>(stringResult.c_str());
}

/* ===========================  Live Competition ELO Extension =========================== */
LiveCompEloExtension::LiveCompEloExtension()
{
}

LiveCompEloExtension::~LiveCompEloExtension()
{
}

void LiveCompEloExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

void LiveCompEloExtension::getEloStatUpdateInfo(CategoryInfoList& categoryInfoList)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	int count = 0;
	CategoryInfo categoryInfo;
	EntityInfo* info = NULL;

	switch(mExtensionData.mPeriodType)
	{
	case Stats::STAT_PERIOD_ALL_TIME:
		categoryInfo.category = LIVE_COMP_NORMAL_STATS_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_ALLTIME;
		break;
	case Stats::STAT_PERIOD_MONTHLY:
		categoryInfo.category = LIVE_COMP_NORMAL_STATS_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_MONTHLY;
		break;
	case Stats::STAT_PERIOD_WEEKLY:
		categoryInfo.category = LIVE_COMP_NORMAL_STATS_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_WEEKLY;
		break;
	case Stats::STAT_PERIOD_DAILY:
		categoryInfo.category = LIVE_COMP_NORMAL_STATS_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_DAILY;
		break;
	default:
		categoryInfo.category = LIVE_COMP_NORMAL_STATS_CATEGORY LIVE_COMP_CATEGORY_SUFFIX_ALLTIME;
		break;
	}
	
	for(; playerIter != playerEnd; ++playerIter)
	{
		const OSDKGameReportBase::OSDKPlayerReport* osdkPlayerReport = playerIter->second;
		
		if (count % 2 == 0)
		{
			info = &categoryInfo.entityA;
		}
		else
		{
			info = &categoryInfo.entityB;
		}

		info->entityId = playerIter->first;

		ScopePair scopePair;
		scopePair.name = SCOPENAME_COMPETITIONID;
		scopePair.value = mExtensionData.mSponsoredEventId;
		info->scopeList.push_back(scopePair);
		scopePair.name = SCOPENAME_TEAMID;
		scopePair.value = mExtensionData.mUseLockedTeam ? osdkPlayerReport->getTeam() : UNIQUE_TEAM_ID_KEYSCOPE_VAL;
		info->scopeList.push_back(scopePair);

		count++;
	}

	categoryInfoList.push_back(categoryInfo);
}

void LiveCompEloExtension::generateAttributeMap(EntityId id, Collections::AttributeMap& map)
{
	OSDKGameReportBase::OSDKPlayerReport* playerReport = getOsdkPlayerReport(id);
	if (playerReport != NULL)
	{
		OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport->getCustomPlayerReport());

		eastl::string temp;
		map[ATTRIBNAME_GOALS]			= convertToString(h2hPlayerReport.getCommonPlayerReport().getGoals(), temp);
		map[ATTRIBNAME_PASSATTEMPTS]	= convertToString(h2hPlayerReport.getCommonPlayerReport().getPassAttempts(), temp);
		map[ATTRIBNAME_PASSESMADE]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getPassesMade(), temp);
		map[ATTRIBNAME_TACKLEATTEMPTS]	= convertToString(h2hPlayerReport.getCommonPlayerReport().getTackleAttempts(), temp);
		map[ATTRIBNAME_TACKLESMADE]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getTacklesMade(), temp);
		map[ATTRIBNAME_POSSESSION]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getPossession(), temp);
		map[ATTRIBNAME_SHOTS]			= convertToString(h2hPlayerReport.getCommonPlayerReport().getShots(), temp);
		map[ATTRIBNAME_SHOTSONGOAL]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getShotsOnGoal(), temp);
		map[ATTRIBNAME_FOULS]			= convertToString(h2hPlayerReport.getCommonPlayerReport().getFouls(), temp);
		map[ATTRIBNAME_YELLOWCARDS]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getYellowCard(), temp);
		map[ATTRIBNAME_REDCARDS]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getRedCard(), temp);
		map[ATTRIBNAME_CORNERS]			= convertToString(h2hPlayerReport.getCommonPlayerReport().getCorners(), temp);
		map[ATTRIBNAME_OFFSIDES]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getOffsides(), temp);
		map[ATTRIBNAME_TEAMRATING]		= convertToString(h2hPlayerReport.getH2HCustomPlayerData().getTeamrating(), temp);
		map[ATTRIBNAME_TEAM]			= convertToString(playerReport->getTeam(), temp);
		map[ATTRIBNAME_HOME]			= convertToString(playerReport->getHome(), temp);
		map[ATTRIBNAME_GAMETIME]		= convertToString(osdkReport.getGameReport().getGameTime(), temp);
		map[ATTRIBNAME_CLUBDISC]		= convertToString(0, temp);
		map[ATTRIBNAME_DISC]			= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::DISCONNECT], temp);
		map[ATTRIBNAME_STATPERC]		= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::STAT_PERC], temp);//calculated damping percent
		map[STATNAME_RESULT]			= convertToString(mExtensionData.mCalculatedStats[id].stats[ExtensionData::RESULT], temp);//calculated WLT LobbySkill
	}
}

bool LiveCompEloExtension::isClubs()
{
	return false;
}

OSDKGameReportBase::OSDKPlayerReport* LiveCompEloExtension::getOsdkPlayerReport(EntityId entityId)
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

char* LiveCompEloExtension::convertToString(int64_t value, eastl::string& stringResult)
{
	stringResult.clear();
	stringResult.sprintf("%" PRId64"", value);

	return const_cast<char*>(stringResult.c_str());
}


}   // namespace GameReporting
}   // namespace Blaze
