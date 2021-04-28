/*************************************************************************************************/
/*!
    \file   fifa_lastopponent.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "fifa_lastopponent.h"

#define STATNAME_LASTOPPONENT		"lastOpponent"
#define STATNAME_LASTOPPONENT0		"lastOpponent0"
#define STATNAME_LASTOPPONENT1		"lastOpponent1"
#define STATNAME_LASTOPPONENT2		"lastOpponent2"
#define STATNAME_LASTOPPONENT3		"lastOpponent3"
#define STATNAME_LASTOPPONENT4		"lastOpponent4"
#define STATNAME_LASTOPPONENT5		"lastOpponent5"
#define STATNAME_LASTOPPONENT6		"lastOpponent6"
#define STATNAME_LASTOPPONENT7		"lastOpponent7"
#define STATNAME_LASTOPPONENT8		"lastOpponent8"
#define STATNAME_LASTOPPONENT9		"lastOpponent9"

namespace Blaze
{
namespace GameReporting
{

void FifaLastOpponent::initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport)
{
	mBuilder = builder;
    mUpdateStatsHelper = updateStatsHelper;
	mProcessedReport = processedReport;
}

void FifaLastOpponent::setExtension(IFifaLastOpponentExtension* lastOpponentExtension)
{
	mLastOpponentExtension = lastOpponentExtension;
}

void FifaLastOpponent::selectLastOpponentStats()
{
	IFifaLastOpponentExtension::EntityList ids;
	mLastOpponentExtension->getEntityIds(ids);

	IFifaLastOpponentExtension::EntityList::iterator iter = ids.begin();
	IFifaLastOpponentExtension::EntityList::iterator end = ids.end();

	for(; iter != end; ++iter)
	{
		TRACE_LOG("[FifaLastOpponent].selectLastOpponentStats() for " << mLastOpponentExtension->getStatCategory());

		//select last opponent stats
		mBuilder->startStatRow(mLastOpponentExtension->getStatCategory(), *iter, NULL);
		mBuilder->selectStat(STATNAME_LASTOPPONENT0);
		mBuilder->selectStat(STATNAME_LASTOPPONENT1);
		mBuilder->selectStat(STATNAME_LASTOPPONENT2);
		mBuilder->selectStat(STATNAME_LASTOPPONENT3);
		mBuilder->selectStat(STATNAME_LASTOPPONENT4);
		mBuilder->selectStat(STATNAME_LASTOPPONENT5);
		mBuilder->selectStat(STATNAME_LASTOPPONENT6);
		mBuilder->selectStat(STATNAME_LASTOPPONENT7);
		mBuilder->selectStat(STATNAME_LASTOPPONENT8);
		mBuilder->selectStat(STATNAME_LASTOPPONENT9);

		mBuilder->completeStatRow();
	}
}

void FifaLastOpponent::transformLastOpponentStats()
{
	UpdateRowKeyList SPLastOppKeys;

	IFifaLastOpponentExtension::EntityList ids;
	mLastOpponentExtension->getEntityIds(ids);

	IFifaLastOpponentExtension::EntityList::iterator iter = ids.begin();
	IFifaLastOpponentExtension::EntityList::iterator end = ids.end();

	for(; iter != end; ++iter)
	{
		TRACE_LOG("[FifaLastOpponent].transformLastOpponentStats() generating keys for entity: "<< *iter <<" " );

		Stats::UpdateRowKey* key = mBuilder->getUpdateRowKey(mLastOpponentExtension->getStatCategory(), *iter);
		key = mBuilder->getUpdateRowKey(mLastOpponentExtension->getStatCategory(), *iter);
		if (key != NULL)
			SPLastOppKeys.push_back(*key);
	}
	
	UpdateRowKeyList::const_iterator spLastOppKeyIter, spLastOppKeyEnd;

	spLastOppKeyIter = SPLastOppKeys.begin();
	spLastOppKeyEnd = SPLastOppKeys.end();

	StatValueUtil::StatUpdateMap statUpdateMap;
	readLastOpponentStats(SPLastOppKeys, statUpdateMap);

	Stats::UpdateRowKey temp;
	for (; spLastOppKeyIter != spLastOppKeyEnd; ++spLastOppKeyIter)
	{
		updateLastOpponent(*spLastOppKeyIter, &statUpdateMap);
	}

	writeLastOpponentStats(statUpdateMap);
}

void FifaLastOpponent::readLastOpponentStats(FifaLastOpponent::UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap)
{
	UpdateRowKeyList::iterator iter = keyList.begin();
	UpdateRowKeyList::iterator end = keyList.end();

	for (; iter != end; iter++)
	{
		StatValueUtil::StatUpdate statUpdate;
		Stats::UpdateRowKey key = *iter;
		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key.period);

		//check if already exists
		StatValueUtil::StatUpdate* update = &statUpdate;
		StatValueUtil::StatUpdateMap::iterator sumIter = statUpdateMap.find(key);
		bool alreadyExists = sumIter != statUpdateMap.end();
		if (alreadyExists)
		{
			update = &sumIter->second;
		}

		insertStat(*update, STATNAME_LASTOPPONENT0, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT0, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT1, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT2, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT3, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT4, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT5, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT5, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT6, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT6, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT7, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT7, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT8, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT8, periodType, true));
		insertStat(*update, STATNAME_LASTOPPONENT9, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTOPPONENT9, periodType, true));

		if (!alreadyExists)
		{
			statUpdateMap.insert(eastl::make_pair(key, statUpdate));
		}
	}
}

void FifaLastOpponent::writeLastOpponentStats(StatValueUtil::StatUpdateMap& statUpdateMap)
{
	StatValueUtil::StatUpdateMap::iterator iter = statUpdateMap.begin();
	StatValueUtil::StatUpdateMap::iterator end = statUpdateMap.end();

	for (; iter != end; iter++)
	{
		Stats::UpdateRowKey* key = &iter->first;
		StatValueUtil::StatUpdate* statUpdate = &iter->second;

		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key->period);

		StatValueUtil::StatValueMap::iterator statValueIter = statUpdate->stats.begin();
		StatValueUtil::StatValueMap::iterator statValueEnd = statUpdate->stats.end();

		for (; statValueIter != statValueEnd; statValueIter++)
		{
			StatValueUtil::StatValue* statValue = &statValueIter->second;
			mUpdateStatsHelper->setValueInt(key, statValueIter->first, periodType, statValue->iAfter[periodType]);

			TRACE_LOG("[FifaLastOpponent].writeLastOpponentStats() - Entity id:" << key->entityId <<  
																		" Category:" << key->category << 
																		" Period:" << key->period << 
																		" Stat Name:" << statValueIter->first << 
																		" Value:" << statValue->iAfter[periodType] << 
																		" ");
		}
	}
}

void FifaLastOpponent::updateLastOpponent(Stats::UpdateRowKey spLastOppKey, StatValueUtil::StatUpdateMap* statUpdateMap)
{
	if (statUpdateMap != NULL)
	{
		StatValueUtil::StatUpdate* statSPLastOppUpdate = NULL;
		StatValueUtil::StatValueMap* statSPLastOppValueMap = NULL;

		StatValueUtil::StatUpdateMap::iterator updateSPLastOppIter, updateSPLastOppEnd;
		updateSPLastOppIter = statUpdateMap->find(spLastOppKey);
		updateSPLastOppEnd = statUpdateMap->end();
		if (updateSPLastOppIter == updateSPLastOppEnd)
		{
			WARN_LOG("[FifaLastOpponent].updateSeasonPlayPlayerStats(): Last Opponent Cannot find key");
			return;
		}
		statSPLastOppUpdate = &(updateSPLastOppIter->second);
		statSPLastOppValueMap = &(statSPLastOppUpdate->stats);
	
		StatValueUtil::StatValue* lastMatchOpponents[NUM_LAST_MATCHES];
		for (int i = 0; i < NUM_LAST_MATCHES; i++)
		{
			char8_t lastMatchField[16];
			sprintf(lastMatchField, "%s%d", STATNAME_LASTOPPONENT, i);
			lastMatchOpponents[i] = GetStatValue(statSPLastOppValueMap, lastMatchField);
			TRACE_LOG("[FifaLastOpponent].updateSeasonPlayPoints() - Read:" << lastMatchField <<  
				" as:" << lastMatchOpponents[i]->iBefore[Stats::STAT_PERIOD_ALL_TIME] << 
				" ");
		}

		for(int i = NUM_LAST_MATCHES - 1; i > 0; i--)
		{
			TRACE_LOG("[FifaLastOpponent].updateSeasonPlayPoints() - Setting index:" << i <<  
				" was:" << lastMatchOpponents[i]->iAfter[Stats::STAT_PERIOD_ALL_TIME] << 
				" now:" << lastMatchOpponents[i - 1]->iBefore[Stats::STAT_PERIOD_ALL_TIME] <<
				" ");
			lastMatchOpponents[i]->iAfter[Stats::STAT_PERIOD_ALL_TIME] = lastMatchOpponents[i - 1]->iBefore[Stats::STAT_PERIOD_ALL_TIME];
		}

		// update last opponent
		lastMatchOpponents[0]->iAfter[Stats::STAT_PERIOD_ALL_TIME] = mLastOpponentExtension->getOpponentId(updateSPLastOppIter->first.entityId);
		TRACE_LOG("[FifaLastOpponent].updateSeasonPlayPoints() - Setting New Last Opponent:" << lastMatchOpponents[0]->iAfter[Stats::STAT_PERIOD_ALL_TIME] <<  " ");
	}
}

} //namespace GameReportingLegacy
} //namespace Blaze
