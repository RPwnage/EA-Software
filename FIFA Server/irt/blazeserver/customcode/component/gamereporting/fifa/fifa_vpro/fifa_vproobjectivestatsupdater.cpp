/*************************************************************************************************/
/*!
    \file   fifa_vproobjectivestatsupdater.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "fifa_vproobjectivestatsupdater.h"


#include "framework/util/shared/blazestring.h"

namespace Blaze
{
namespace GameReporting
{

void FifaVProObjectiveStatsUpdater::initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport, Stats::StatPeriodType periodType)
{
	mBuilder = builder;
    mUpdateStatsHelper = updateStatsHelper;
	mProcessedReport = processedReport;
	mStatPeriodType = periodType;
}

const char8_t* FifaVProObjectiveStatsUpdater::getObjectiveStatCategory()
{
	return "VProObjectiveStats";
}

void FifaVProObjectiveStatsUpdater::getEntityIds(IFifaSeasonalPlayExtension::EntityList& ids)
{
	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = osdkPlayerReportsMap.begin();
	playerEnd = osdkPlayerReportsMap.end();

	for (; playerIter != playerEnd; ++playerIter)
	{
		ids.push_back(playerIter->first);
	}
}

uint64_t FifaVProObjectiveStatsUpdater::getClubIdFromPlayerReport(EntityId id)
{
	uint64_t clubId = 0;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap& osdkPlayerReportsMap = osdkReport.getPlayerReports();

	OSDKGameReportBase::OSDKReport::OSDKPlayerReportsMap::const_iterator playerIter, playerEnd;
	playerIter = osdkPlayerReportsMap.begin();
	playerEnd = osdkPlayerReportsMap.end();

	for (; playerIter != playerEnd; ++playerIter)
	{
		if (playerIter->first == id)
		{
			OSDKGameReportBase::OSDKPlayerReport& playerReport = *playerIter->second;
			OSDKClubGameReportBase::OSDKClubPlayerReport& clubPlayerReport = static_cast<OSDKClubGameReportBase::OSDKClubPlayerReport&>(*playerReport.getCustomPlayerReport());
			
			clubId = clubPlayerReport.getClubId();
			break;
		}
	}

	return clubId;
}

void FifaVProObjectiveStatsUpdater::selectObjectiveStats()
{
	IFifaSeasonalPlayExtension::EntityList ids;
	getEntityIds(ids);

	IFifaSeasonalPlayExtension::EntityList::iterator iter = ids.begin();
	IFifaSeasonalPlayExtension::EntityList::iterator end = ids.end();

	for(; iter != end; ++iter)
	{
		mBuilder->startStatRow(getObjectiveStatCategory(), *iter, NULL);
					
		mBuilder->selectStat(STATNAME_LEAGUESWON);
		mBuilder->selectStat(STATNAME_PROMOTIONS);
		mBuilder->selectStat(STATNAME_TOTALCUPSWON);
			
		mBuilder->completeStatRow();
	}
}

void FifaVProObjectiveStatsUpdater::transformObjectiveStats()
{
	UpdateRowKeyList OSKey;

	IFifaSeasonalPlayExtension::EntityList ids;
	getEntityIds(ids);

	IFifaSeasonalPlayExtension::EntityList::iterator iter = ids.begin();
	IFifaSeasonalPlayExtension::EntityList::iterator end = ids.end();

	for(; iter != end; ++iter)
	{
		TRACE_LOG("[FifaVProObjectiveStatsUpdater].transformObjectiveStats() generating keys for entity: "<< *iter <<" " );

		Stats::UpdateRowKey* key = mBuilder->getUpdateRowKey(getObjectiveStatCategory(), *iter);
		if (key != NULL)
		{
			key->period = mStatPeriodType;
			OSKey.push_back(*key);
		}
	}
	
	UpdateRowKeyList::const_iterator objStatsKeyIter, objStatsKeyIterEnd;
	objStatsKeyIter = OSKey.begin();
	objStatsKeyIterEnd = OSKey.end();

	StatValueUtil::StatUpdateMap statUpdateMap;
	readObjectiveStats(OSKey, statUpdateMap);

	for (; objStatsKeyIter != objStatsKeyIterEnd; ++objStatsKeyIter)
	{
		Stats::UpdateRowKey rowKey = static_cast<Stats::UpdateRowKey> (*objStatsKeyIter);
		EntityId clubEntityId = static_cast<EntityId> (getClubIdFromPlayerReport(rowKey.entityId));

		updateObjectiveStats(clubEntityId, *objStatsKeyIter, OSKey, &statUpdateMap);
	}
	
	StatValueUtil::StatUpdateMap::iterator statUpdateIter = statUpdateMap.begin();
	StatValueUtil::StatUpdateMap::iterator statUpdateEnd = statUpdateMap.end();

	for (; statUpdateIter != statUpdateEnd; statUpdateIter++)
	{
		Stats::UpdateRowKey* key = &statUpdateIter->first;
		StatValueUtil::StatUpdate* statUpdate = &statUpdateIter->second;

		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key->period);

		StatValueUtil::StatValueMap::iterator statValueIter = statUpdate->stats.begin();
		StatValueUtil::StatValueMap::iterator statValueEnd = statUpdate->stats.end();

		for (; statValueIter != statValueEnd; statValueIter++)
		{
			StatValueUtil::StatValue* statValue = &statValueIter->second;
			mUpdateStatsHelper->setValueInt(key, statValueIter->first, periodType, statValue->iAfter[periodType]);
		}
	}
}

void FifaVProObjectiveStatsUpdater::readObjectiveStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap)
{
	UpdateRowKeyList::iterator iter = keyList.begin();
	UpdateRowKeyList::iterator end = keyList.end();

	for (; iter != end; iter++)
	{
		StatValueUtil::StatUpdate statUpdate;
		Stats::UpdateRowKey key = *iter;
		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key.period);

		insertStat(statUpdate, STATNAME_LEAGUESWON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LEAGUESWON, periodType, true));
		insertStat(statUpdate, STATNAME_PROMOTIONS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PROMOTIONS, periodType, true));
		insertStat(statUpdate, STATNAME_TOTALCUPSWON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TOTALCUPSWON, periodType, true));

		statUpdateMap.insert(eastl::make_pair(key, statUpdate));
	}
}

void FifaVProObjectiveStatsUpdater::updateObjectiveStats(EntityId clubEntityId, Stats::UpdateRowKey osKey, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap)
{
	if (statUpdateMap != NULL)
	{
		StatValueUtil::StatUpdateMap::iterator updateOSIter, updateOSEnd;
		updateOSIter = statUpdateMap->find(osKey);
		updateOSEnd = statUpdateMap->end();
		if (updateOSIter == updateOSEnd)
		{
			WARN_LOG("[FifaSeasonalPlay].updateObjectiveStats(): ObjectiveStats Cannot find key");
			return;
		}
		StatValueUtil::StatUpdate* statOSUpdate = &(updateOSIter->second);
		StatValueUtil::StatValueMap* statOSValueMap = &(statOSUpdate->stats);

		OSDKClubGameReportBase::OSDKClubClubReport* osdkClubReport = getOsdkClubReport(clubEntityId);
		if (NULL != osdkClubReport)
		{
			FifaClubReportBase::FifaClubsClubReport& fifaClubsClubsReport = static_cast<FifaClubReportBase::FifaClubsClubReport&>(*osdkClubReport->getCustomClubClubReport());
			if (fifaClubsClubsReport.getSeasonalPlayData().getUpdateDivision() == Fifa::SEASON_PROMOTION)
			{
				StatValueUtil::StatValue * svOSPromo = GetStatValue(statOSValueMap, STATNAME_PROMOTIONS);
				svOSPromo->iAfter[mStatPeriodType] += 1;
			}

			if (fifaClubsClubsReport.getSeasonalPlayData().getWonLeagueTitle())
			{
				StatValueUtil::StatValue * svOSLeagueWon = GetStatValue(statOSValueMap, STATNAME_LEAGUESWON);
				svOSLeagueWon->iAfter[mStatPeriodType] += 1;
			}

			if (fifaClubsClubsReport.getSeasonalPlayData().getWonClubCup())
			{
				StatValueUtil::StatValue * svOSLeagueWon = GetStatValue(statOSValueMap, STATNAME_TOTALCUPSWON);
				svOSLeagueWon->iAfter[mStatPeriodType] += 1;
			}
		}
	}
}

OSDKClubGameReportBase::OSDKClubClubReport* FifaVProObjectiveStatsUpdater::getOsdkClubReport(EntityId entityId)
{
	OSDKClubGameReportBase::OSDKClubClubReport* resultClubReport = NULL;

	OSDKGameReportBase::OSDKReport& osdkReport = static_cast<OSDKGameReportBase::OSDKReport&>(*mProcessedReport->getGameReport().getReport());
	OSDKClubGameReportBase::OSDKClubReport& clubReport = static_cast<OSDKClubGameReportBase::OSDKClubReport&>(*osdkReport.getTeamReports());
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap& osdkClubReportMap = clubReport.getClubReports();

	Clubs::ClubId clubId = static_cast<Clubs::ClubId>(entityId);
	OSDKClubGameReportBase::OSDKClubReport::OSDKClubReportsMap::iterator clubIter, clubIterEnd;
	clubIter = osdkClubReportMap.find(clubId);
	clubIterEnd = osdkClubReportMap.end();

	if (clubIter != clubIterEnd)
	{
		resultClubReport = clubIter->second;
	}

	return resultClubReport;
}


} //namespace GameReportingLegacy
} //namespace Blaze
