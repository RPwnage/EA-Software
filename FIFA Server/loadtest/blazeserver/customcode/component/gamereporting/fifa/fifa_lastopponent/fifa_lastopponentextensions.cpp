/*************************************************************************************************/
/*!
    \file   fifa_lastopponentextensions.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "fifa_lastopponentextensions.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{

/*-------------------------------------------
		Head to Head Play
---------------------------------------------*/

HeadToHeadLastOpponent::HeadToHeadLastOpponent()
{
}

HeadToHeadLastOpponent::~HeadToHeadLastOpponent()
{
}

void HeadToHeadLastOpponent::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

const char8_t* HeadToHeadLastOpponent::getStatCategory()
{
	return "SPOverallPlayerStats";
}

void HeadToHeadLastOpponent::getEntityIds(EntityList& ids)
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

EntityId HeadToHeadLastOpponent::getOpponentId(EntityId entityId)
{
	EntityId opponentId = -1;

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& OsdkPlayerReportsMap = OsdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = OsdkPlayerReportsMap.begin();
	playerEnd = OsdkPlayerReportsMap.end();

	for(; playerIter != playerEnd; ++playerIter)
	{
		if (entityId != playerIter->first)
		{
			opponentId = playerIter->first;
			break;
		}
	}

	return opponentId;
}


/*-------------------------------------------
		Clubs Play
---------------------------------------------*/

ClubsLastOpponent::ClubsLastOpponent()
{
}

ClubsLastOpponent::~ClubsLastOpponent()
{
}

void ClubsLastOpponent::setExtensionData(void* extensionData)
{
	if (extensionData != NULL)
	{
		ExtensionData* data = reinterpret_cast<ExtensionData*>(extensionData);
		mExtensionData = *data;
	}
}

const char8_t* ClubsLastOpponent::getStatCategory()
{
	return "SPOverallClubsStats";
}

void ClubsLastOpponent::getEntityIds(EntityList& ids)
{
	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport* clubReports = static_cast<OSDKClubGameReportBase::OSDKClubReport*>(OsdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& clubReportsMap = clubReports->getClubReports();

	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubsIter, clubsEnd;
	clubsIter = clubReportsMap.begin();
	clubsEnd = clubReportsMap.end();

	for(; clubsIter != clubsEnd; ++clubsIter)
	{
		if (NeedToFilter(clubsIter->first) == false)
		{
			ids.push_back(clubsIter->first);
		}		
	}
}

void ClubsLastOpponent::UpdateEntityFilterList(eastl::vector<int64_t> filteredIds)
{
	mEntityFilterList.clear();
	mEntityFilterList = filteredIds;
}

bool ClubsLastOpponent::NeedToFilter(int64_t entityId)
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

EntityId ClubsLastOpponent::getOpponentId(EntityId entityId)
{
	Clubs::ClubId opponentId = 0;
	Clubs::ClubId myId = static_cast<Clubs::ClubId>(entityId);

	OSDKGameReportBase::OSDKReport& OsdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mExtensionData.mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport* clubReports = static_cast<OSDKClubGameReportBase::OSDKClubReport*>(OsdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& clubReportsMap = clubReports->getClubReports();

	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::const_iterator clubsIter, clubsEnd;
	clubsIter = clubReportsMap.begin();
	clubsEnd = clubReportsMap.end();
	for(; clubsIter != clubsEnd; ++clubsIter)
	{
		if (myId != clubsIter->first)
		{
			opponentId = clubsIter->first;
			break;
		}
	}
	return static_cast<EntityId>(opponentId);
}



}   // namespace GameReporting
}   // namespace Blaze
