/*************************************************************************************************/
/*!
    \file   fifa_seasonalplayextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "fifa_friendliesextensions.h"
#include "fifa_friendliesdefines.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{


HeadtoHeadFriendliesExtension::HeadtoHeadFriendliesExtension()
{
}

HeadtoHeadFriendliesExtension::~HeadtoHeadFriendliesExtension()
{
}

void HeadtoHeadFriendliesExtension::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

void HeadtoHeadFriendliesExtension::getCategoryInfo(FriendlyList& friendlyList)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	IFifaFriendliesExtension::FriendyPair friendlyPair;
	for(; playerIter != playerEnd; ++playerIter)
	{
		const OSDKGameReportBase::OSDKPlayerReport* playerReport = playerIter->second;
		if (playerReport->getHome() == 1)
		{
			friendlyPair.entities[SIDE_HOME] = playerIter->first;
		}
		else
		{
			friendlyPair.entities[SIDE_AWAY] = playerIter->first;	
		}
	}

	friendlyPair.category = "FriendlyGameStats";
	friendlyList.push_back(friendlyPair);
}

void HeadtoHeadFriendliesExtension::generateAttributeMap(EntityId id, Collections::AttributeMap& map)
{
	OSDKGameReportBase::OSDKPlayerReport* playerReport = getOsdkPlayerReport(id);
	if (playerReport != NULL)
	{
		Fifa::H2HPlayerReport& h2hPlayerReport = static_cast<Fifa::H2HPlayerReport&>(*playerReport->getCustomPlayerReport());

		//determine win/loss/draw
		int32_t win, lose, tie, points;
		if (mExtensionData.mWinnerSet->find(id) != mExtensionData.mWinnerSet->end())
		{
			win = 1;
			lose = 0;
			tie = 0;

			points = POINTS_FOR_WIN;

		}
		else if (mExtensionData.mLoserSet->find(id) != mExtensionData.mLoserSet->end())
		{
			win = 0;
			lose = 1;
			tie = 0;

			points = POINTS_FOR_LOSS;
		}
		else
		{
			win = 0;
			lose = 0;
			tie = 1;

			points = POINTS_FOR_TIE;
		}

		eastl::string temp;
		map[STATNAME_SCORE]		= convertToString(playerReport->getScore(), temp);
		map[STATNAME_POINTS]	= convertToString(points, temp);
		map[STATNAME_WINS]		= convertToString(win, temp);
		map[STATNAME_TIES]		= convertToString(tie, temp);
		map[STATNAME_LOSSES]	= convertToString(lose, temp);
		map[STATNAME_TEAM]		= convertToString(playerReport->getTeam(), temp);
		map[STATNAME_GOALS]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getGoals(), temp);
		map[STATNAME_GOALS_AGAINST]	= convertToString(h2hPlayerReport.getCommonPlayerReport().getGoalsConceded(), temp);
		map[STATNAME_FOULS]		= convertToString(h2hPlayerReport.getCommonPlayerReport().getFouls(), temp);
		map[STATNAME_SHOTS_ON_GOAL]	= convertToString(h2hPlayerReport.getCommonPlayerReport().getShotsOnGoal(), temp);
		map[STATNAME_SHOTS_AGAINST_ON_GOAL] = convertToString(h2hPlayerReport.getH2HCustomPlayerData().getShotsAgainstOnGoal(),temp);
		map[STATNAME_POSSESSION] = convertToString(h2hPlayerReport.getCommonPlayerReport().getPossession(), temp);
	}
}

OSDKGameReportBase::OSDKPlayerReport* HeadtoHeadFriendliesExtension::getOsdkPlayerReport(EntityId entityId)
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

char* HeadtoHeadFriendliesExtension::convertToString(int64_t value, eastl::string& stringResult)
{
	stringResult.clear();
	stringResult.sprintf("%" PRId64"", value);

	return const_cast<char*>(stringResult.c_str());
}

}   // namespace GameReporting
}   // namespace Blaze
