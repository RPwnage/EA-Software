/*************************************************************************************************/
/*!
    \file   fifa_seasonalplay.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "fifa_seasonalplay.h"
#include "fifa_seasonalplaydefines.h"

#include "customcomponent/fifacups/fifacupsslaveimpl.h"
#include "customcode/component/gamereporting/osdk/osdktournamentsutil.h"

#include "framework/util/shared/blazestring.h"

namespace Blaze
{
namespace GameReporting
{

void FifaSeasonalPlay::initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport)
{
	mBuilder = builder;
    mUpdateStatsHelper = updateStatsHelper;
	mProcessedReport = processedReport;
}

void FifaSeasonalPlay::setExtension(IFifaSeasonalPlayExtension* seasonalPlayExtension)
{
	mStatPeriodType = Stats::STAT_PERIOD_ALL_TIME;				/// default

	mSeasonalPlayExtension = seasonalPlayExtension;
	mStatPeriodType = mSeasonalPlayExtension->getPeriodType();
	TRACE_LOG("[FifaSeasonalPlay].setExtension() extension Overall Stat Category/mStatPeriodType = " << mSeasonalPlayExtension->getOverallStatCategory() << "/" << mStatPeriodType << " ");
}

void FifaSeasonalPlay::selectSeasonalPlayStats()
{
	IFifaSeasonalPlayExtension::EntityList ids;
	mSeasonalPlayExtension->getEntityIds(ids);

	IFifaSeasonalPlayExtension::EntityList::iterator iter = ids.begin();
	IFifaSeasonalPlayExtension::EntityList::iterator end = ids.end();

	for(; iter != end; ++iter)
	{
		TRACE_LOG("[FifaSeasonalPlay].selectSeasonalPlayStats() for " << mSeasonalPlayExtension->getDivStatCategory() << " " << mSeasonalPlayExtension->getOverallStatCategory() << " id " << *iter << " ");

		// get keyscopes
		Stats::ScopeNameValueMap scopeNameValueMap;
		mSeasonalPlayExtension->getKeyScopes(scopeNameValueMap);

		//select divisional stats
		mBuilder->startStatRow(mSeasonalPlayExtension->getDivStatCategory(), *iter, scopeNameValueMap.empty() ? NULL : &scopeNameValueMap);
		mBuilder->selectStat(STATNAME_SEASONWINS);
		mBuilder->selectStat(STATNAME_SEASONLOSSES);
		mBuilder->selectStat(STATNAME_SEASONTIES);
		mBuilder->selectStat(STATNAME_GAMES_PLAYED);
		mBuilder->selectStat(STATNAME_GOALS);
		mBuilder->selectStat(STATNAME_GOALSAGAINST);
		mBuilder->selectStat(STATNAME_SPPOINTS);
		mBuilder->selectStat(STATNAME_SEASONS);
		mBuilder->selectStat(STATNAME_RANKINGPOINTS);
		mBuilder->selectStat(STATNAME_CURDIVISION);
		mBuilder->selectStat(STATNAME_CUPWINS);
		mBuilder->selectStat(STATNAME_CUPLOSSES);
		mBuilder->selectStat(STATNAME_CUPROUND);
		mBuilder->selectStat(STATNAME_PREVSEASONWINS);
		mBuilder->selectStat(STATNAME_PREVSEASONLOSSES);
		mBuilder->selectStat(STATNAME_PREVSEASONTIES);
		mBuilder->selectStat(STATNAME_PROJPTS);
		mBuilder->selectStat(STATNAME_PREVPROJPTS);
		mBuilder->selectStat(STATNAME_EXTRAMATCHES);
		mBuilder->selectStat(STATNAME_WINREDEEMED);
		mBuilder->selectStat(STATNAME_DRAWREDEEMED);
		mBuilder->selectStat(STATNAME_MATCHREDEEMED);

		mBuilder->completeStatRow();
		
		//select overall stats
		mBuilder->startStatRow(mSeasonalPlayExtension->getOverallStatCategory(), *iter, scopeNameValueMap.empty() ? NULL : &scopeNameValueMap);
		mBuilder->selectStat(STATNAME_SEASONS);
		mBuilder->selectStat(STATNAME_TITLESWON);
		mBuilder->selectStat(STATNAME_LEAGUESWON);
		mBuilder->selectStat(STATNAME_DIVSWON1);
		mBuilder->selectStat(STATNAME_DIVSWON2);
		mBuilder->selectStat(STATNAME_DIVSWON3);
		mBuilder->selectStat(STATNAME_DIVSWON4);
		mBuilder->selectStat(STATNAME_CUPSWON1);
		mBuilder->selectStat(STATNAME_CUPSWON2);
		mBuilder->selectStat(STATNAME_CUPSWON3);
		mBuilder->selectStat(STATNAME_CUPSWON4);
		mBuilder->selectStat(STATNAME_PROMOTIONS);
		mBuilder->selectStat(STATNAME_HOLDS);
		mBuilder->selectStat(STATNAME_RELEGATIONS );
		mBuilder->selectStat(STATNAME_RANKINGPOINTS);
		mBuilder->selectStat(STATNAME_CURDIVISION );
		mBuilder->selectStat(STATNAME_PREVDIVISION);
		mBuilder->selectStat(STATNAME_MAXDIVISION );
		mBuilder->selectStat(STATNAME_BESTDIVISION);
		mBuilder->selectStat(STATNAME_BESTPOINTS);
		mBuilder->selectStat(STATNAME_SEASONMOV );
		mBuilder->selectStat(STATNAME_LASTMATCH0);
		mBuilder->selectStat(STATNAME_LASTMATCH1);
		mBuilder->selectStat(STATNAME_LASTMATCH2);
		mBuilder->selectStat(STATNAME_LASTMATCH3);
		mBuilder->selectStat(STATNAME_LASTMATCH4);
		mBuilder->selectStat(STATNAME_LASTMATCH5);
		mBuilder->selectStat(STATNAME_LASTMATCH6);
		mBuilder->selectStat(STATNAME_LASTMATCH7);
		mBuilder->selectStat(STATNAME_LASTMATCH8);
		mBuilder->selectStat(STATNAME_LASTMATCH9);		
		mBuilder->selectStat(STATNAME_ALLTIMEGOALS);
		mBuilder->selectStat(STATNAME_ALLTIMEGOALSAGAINST);
		
		mBuilder->completeStatRow();

		if (mSeasonalPlayExtension->trackLastOpponentPlayed())
		{
			//select overall stats
			mBuilder->startStatRow(mSeasonalPlayExtension->getLastOpponentStatCategory(), *iter, scopeNameValueMap.empty() ? NULL : &scopeNameValueMap);
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
}

void FifaSeasonalPlay::transformSeasonalPlayStats()
{
	UpdateRowKeyList SPKeys, SPOKeys, SPLastOppKeys;

	IFifaSeasonalPlayExtension::EntityList ids;
	mSeasonalPlayExtension->getEntityIds(ids);

	IFifaSeasonalPlayExtension::EntityList::iterator iter = ids.begin();
	IFifaSeasonalPlayExtension::EntityList::iterator end = ids.end();

	for(; iter != end; ++iter)
	{
		TRACE_LOG("[FifaSeasonalPlay].transformSeasonalPlayStats() generating keys for entity: "<< *iter <<" " );

		Stats::UpdateRowKey* key = mBuilder->getUpdateRowKey(mSeasonalPlayExtension->getDivStatCategory(), *iter);
		if (key != NULL)
		{
			key->period = mStatPeriodType;
			TRACE_LOG("[FifaSeasonalPlay].transformSeasonalPlayStats() setting period for SPKeys: "<< mStatPeriodType <<" " );
			SPKeys.push_back(*key);
		}

		key = mBuilder->getUpdateRowKey(mSeasonalPlayExtension->getOverallStatCategory(), *iter);
		if (key != NULL)
		{
			key->period = mStatPeriodType;
			TRACE_LOG("[FifaSeasonalPlay].transformSeasonalPlayStats() setting period for SPOKeys: "<< mStatPeriodType <<" " );
			SPOKeys.push_back(*key);
		}

		key = mBuilder->getUpdateRowKey(mSeasonalPlayExtension->getLastOpponentStatCategory(), *iter);
		if (key != NULL)
			SPLastOppKeys.push_back(*key);
	}
	
	UpdateRowKeyList::const_iterator spdivKeyIter, spdivKeyEnd, spoKeyIter, spoKeyEnd, spLastOppKeyIter, spLastOppKeyEnd;

	spdivKeyIter = SPKeys.begin();
	spoKeyIter = SPOKeys.begin();
	spLastOppKeyIter = SPLastOppKeys.begin();

	spdivKeyEnd = SPKeys.end();
	spoKeyEnd = SPOKeys.end();
	spLastOppKeyEnd = SPLastOppKeys.end();

	StatValueUtil::StatUpdateMap statUpdateMap;

	readDivSeasonalPlayStats(SPKeys, statUpdateMap);
	readOverallSeasonalPlayStats(SPOKeys, statUpdateMap);
	readLastOpponentSeasonalPlayStats(SPLastOppKeys, statUpdateMap);

	Stats::UpdateRowKey temp;
	for (; spdivKeyIter != spdivKeyEnd && spoKeyIter != spoKeyEnd; ++spdivKeyIter, ++spoKeyIter)
	{
		Stats::UpdateRowKey lastOppKey = temp;
		if (spLastOppKeyIter != spLastOppKeyEnd)
		{
			lastOppKey = *spLastOppKeyIter;
		}
		updateSeasonPlayStats(*spdivKeyIter, *spoKeyIter, lastOppKey, SPKeys, &statUpdateMap);

		++spLastOppKeyIter;
	}

	writeSeasonalPlayStats(statUpdateMap);
}

void FifaSeasonalPlay::readDivSeasonalPlayStats(FifaSeasonalPlay::UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap)
{
	UpdateRowKeyList::iterator iter = keyList.begin();
	UpdateRowKeyList::iterator end = keyList.end();

	for (; iter != end; iter++)
	{
		StatValueUtil::StatUpdate statUpdate;
		Stats::UpdateRowKey key = *iter;
		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key.period);

		insertStat(statUpdate, STATNAME_SEASONWINS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SEASONWINS, periodType, true));
		insertStat(statUpdate, STATNAME_SEASONLOSSES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SEASONLOSSES, periodType, true));
		insertStat(statUpdate, STATNAME_SEASONTIES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SEASONTIES, periodType, true));
		insertStat(statUpdate, STATNAME_GAMES_PLAYED, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GAMES_PLAYED, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST, periodType, true));
		insertStat(statUpdate, STATNAME_SPPOINTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SPPOINTS, periodType, true));
		insertStat(statUpdate, STATNAME_SEASONS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SEASONS, periodType, true));
		insertStat(statUpdate, STATNAME_RANKINGPOINTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_RANKINGPOINTS, periodType, true));
		insertStat(statUpdate, STATNAME_CURDIVISION, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CURDIVISION, periodType, true));
		insertStat(statUpdate, STATNAME_CUPWINS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CUPWINS, periodType, true));
		insertStat(statUpdate, STATNAME_CUPLOSSES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CUPLOSSES, periodType, true));
		insertStat(statUpdate, STATNAME_CUPROUND, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CUPROUND, periodType, true));
		insertStat(statUpdate, STATNAME_PREVSEASONWINS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PREVSEASONWINS, periodType, true));
		insertStat(statUpdate, STATNAME_PREVSEASONLOSSES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PREVSEASONLOSSES, periodType, true));
		insertStat(statUpdate, STATNAME_PREVSEASONTIES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PREVSEASONTIES, periodType, true));
		insertStat(statUpdate, STATNAME_PROJPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PROJPTS, periodType, true));
		insertStat(statUpdate, STATNAME_PREVPROJPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PREVPROJPTS, periodType, true));
		insertStat(statUpdate, STATNAME_EXTRAMATCHES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_EXTRAMATCHES, periodType, true));
		insertStat(statUpdate, STATNAME_WINREDEEMED, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_WINREDEEMED, periodType, true));
		insertStat(statUpdate, STATNAME_DRAWREDEEMED, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_DRAWREDEEMED, periodType, true));
		insertStat(statUpdate, STATNAME_MATCHREDEEMED, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_MATCHREDEEMED, periodType, true));
		
		statUpdateMap.insert(eastl::make_pair(key, statUpdate));
	}
}

void FifaSeasonalPlay::readOverallSeasonalPlayStats(FifaSeasonalPlay::UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap)
{
	UpdateRowKeyList::iterator iter = keyList.begin();
	UpdateRowKeyList::iterator end = keyList.end();

	for (; iter != end; iter++)
	{
		StatValueUtil::StatUpdate statUpdate;
		Stats::UpdateRowKey key = *iter;
		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key.period);

		insertStat(statUpdate, STATNAME_SEASONS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SEASONS, periodType, true));
		insertStat(statUpdate, STATNAME_TITLESWON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TITLESWON, periodType, true));
		insertStat(statUpdate, STATNAME_LEAGUESWON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LEAGUESWON, periodType, true));
		insertStat(statUpdate, STATNAME_DIVSWON1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_DIVSWON1, periodType, true));
		insertStat(statUpdate, STATNAME_DIVSWON2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_DIVSWON2, periodType, true));
		insertStat(statUpdate, STATNAME_DIVSWON3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_DIVSWON3, periodType, true));
		insertStat(statUpdate, STATNAME_DIVSWON4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_DIVSWON4, periodType, true));
		insertStat(statUpdate, STATNAME_CUPSWON1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CUPSWON1, periodType, true));
		insertStat(statUpdate, STATNAME_CUPSWON2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CUPSWON2, periodType, true));
		insertStat(statUpdate, STATNAME_CUPSWON3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CUPSWON3, periodType, true));
		insertStat(statUpdate, STATNAME_CUPSWON4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CUPSWON4, periodType, true));
		insertStat(statUpdate, STATNAME_PROMOTIONS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PROMOTIONS, periodType, true));
		insertStat(statUpdate, STATNAME_HOLDS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_HOLDS, periodType, true));
		insertStat(statUpdate, STATNAME_RELEGATIONS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_RELEGATIONS, periodType, true));
		insertStat(statUpdate, STATNAME_RANKINGPOINTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_RANKINGPOINTS, periodType, true));
		insertStat(statUpdate, STATNAME_CURDIVISION, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CURDIVISION, periodType, true));
		insertStat(statUpdate, STATNAME_PREVDIVISION, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PREVDIVISION, periodType, true));
		insertStat(statUpdate, STATNAME_MAXDIVISION, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_MAXDIVISION, periodType, true));
		insertStat(statUpdate, STATNAME_BESTDIVISION, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_BESTDIVISION, periodType, true));
		insertStat(statUpdate, STATNAME_BESTPOINTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_BESTPOINTS, periodType, true));
		insertStat(statUpdate, STATNAME_SEASONMOV, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SEASONMOV, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH0, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH0, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH1, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH2, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH3, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH4, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH5, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH5, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH6, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH6, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH7, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH7, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH8, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH8, periodType, true));
		insertStat(statUpdate, STATNAME_LASTMATCH9, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH9, periodType, true));		
		insertStat(statUpdate, STATNAME_ALLTIMEGOALS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_ALLTIMEGOALS, periodType, true));
		insertStat(statUpdate, STATNAME_ALLTIMEGOALSAGAINST, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_ALLTIMEGOALSAGAINST, periodType, true));
		
		statUpdateMap.insert(eastl::make_pair(key, statUpdate));
	}
}

void FifaSeasonalPlay::readLastOpponentSeasonalPlayStats(FifaSeasonalPlay::UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap)
{
	UpdateRowKeyList::iterator iter = keyList.begin();
	UpdateRowKeyList::iterator end = keyList.end();

	for (; iter != end; iter++)
	{
		StatValueUtil::StatUpdate statUpdate;
		Stats::UpdateRowKey key = *iter;
		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key.period);

		if (mSeasonalPlayExtension->trackLastOpponentPlayed())
		{
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
}

void FifaSeasonalPlay::writeSeasonalPlayStats(StatValueUtil::StatUpdateMap& statUpdateMap)
{
	mSPOverallPlayerStatsXml.clear();
	mSPDivisionalPlayerStatsXml.clear();

	StatValueUtil::StatUpdateMap::iterator iter = statUpdateMap.begin();
	StatValueUtil::StatUpdateMap::iterator end = statUpdateMap.end();

	for (; iter != end; iter++)
	{
		Stats::UpdateRowKey* key = &iter->first;
		StatValueUtil::StatUpdate* statUpdate = &iter->second;

		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key->period);

		StatValueUtil::StatValueMap::iterator statValueIter = statUpdate->stats.begin();
		StatValueUtil::StatValueMap::iterator statValueEnd = statUpdate->stats.end();

		eastl::string elementStats;
		
		for (; statValueIter != statValueEnd; statValueIter++)
		{
			StatValueUtil::StatValue* statValue = &statValueIter->second;
			mUpdateStatsHelper->setValueInt(key, statValueIter->first, periodType, statValue->iAfter[periodType]);

			elementStats.append_sprintf("\t\t<%s>%d</%s>\r\n",statValueIter->first, statValue->iAfter[periodType], statValueIter->first);
			
			TRACE_LOG("[FifaSeasonalPlay].writeSeasonalPlayStats() - Entity id:" << key->entityId <<  
																		" Category:" << key->category << 
																		" Period:" << key->period << 
																		" Stat Name:" << statValueIter->first << 
																		" Value:" << statValue->iAfter[periodType] << 
																		" ");
		}

		if(blaze_stricmp(key->category, "SPOverallPlayerStats") == 0)
		{
			eastl::string overallPlayerStats;
			overallPlayerStats.sprintf("\t<spovrplsts>\r\n");
			overallPlayerStats.append(elementStats);
			overallPlayerStats.append_sprintf("\t</spovrplsts>\r\n");
			mSPOverallPlayerStatsXml.insert(eastl::make_pair(key->entityId, overallPlayerStats));
		}
		else if (blaze_stricmp(key->category, "SPDivisionalPlayerStats")== 0)
		{
			eastl::string divPlayerStats;
			divPlayerStats.sprintf("\t<spdivplsts>\r\n");
			divPlayerStats.append(elementStats);
			divPlayerStats.append_sprintf("\t</spdivplsts>\r\n");
			mSPDivisionalPlayerStatsXml.insert(eastl::make_pair(key->entityId, divPlayerStats));
		}
	}
}

void FifaSeasonalPlay::updateSeasonPlayStats(Stats::UpdateRowKey key, Stats::UpdateRowKey spoKey, Stats::UpdateRowKey spLastOppKey, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap)
{
	if (statUpdateMap != NULL)
	{
		// get stats update for player
		StatValueUtil::StatUpdateMap::iterator updateIter, updateEnd;
		updateIter = statUpdateMap->find(key);
		updateEnd = statUpdateMap->end();
		if (updateIter == updateEnd)
		{
			WARN_LOG("[FifaSeasonalPlay].updateSeasonPlayPlayerStats(): Div Cannot find key");
			return;
		}
		StatValueUtil::StatUpdate* statUpdate = &(updateIter->second);
		StatValueUtil::StatValueMap* statValueMap = &(statUpdate->stats);

		StatValueUtil::StatUpdateMap::iterator updateSPOIter, updateSPOEnd;
		updateSPOIter = statUpdateMap->find(spoKey);
		updateSPOEnd = statUpdateMap->end();
		if (updateSPOIter == updateSPOEnd)
		{
			WARN_LOG("[FifaSeasonalPlay].updateSeasonPlayPlayerStats(): Overall Cannot find key");
			return;
		}
		StatValueUtil::StatUpdate* statSPOUpdate = &(updateSPOIter->second);
		StatValueUtil::StatValueMap* statSPOValueMap = &(statSPOUpdate->stats);


		StatValueUtil::StatUpdate* statSPLastOppUpdate = NULL;
		StatValueUtil::StatValueMap* statSPLastOppValueMap = NULL;
		if (mSeasonalPlayExtension->trackLastOpponentPlayed())
		{
			StatValueUtil::StatUpdateMap::iterator updateSPLastOppIter, updateSPLastOppEnd;
			updateSPLastOppIter = statUpdateMap->find(spLastOppKey);
			updateSPLastOppEnd = statUpdateMap->end();
			if (updateSPLastOppIter == updateSPLastOppEnd)
			{
				WARN_LOG("[FifaSeasonalPlay].updateSeasonPlayPlayerStats(): Last Opponent Cannot find key");
				return;
			}
			statSPLastOppUpdate = &(updateSPLastOppIter->second);
			statSPLastOppValueMap = &(statSPLastOppUpdate->stats);

		}


		StatValueUtil::StatValue * svSPOSeasonMov = GetStatValue(statSPOValueMap, STATNAME_SEASONMOV);
		svSPOSeasonMov->iAfter[mStatPeriodType] = MID_SEASON_NXT_GAME;

		StatValueUtil::StatValue* temp = GetStatValue(statValueMap, STATNAME_CUPROUND);
		bool isCupGame = (temp->iBefore[mStatPeriodType] > 0);

		bool isCupStageOn = mSeasonalPlayExtension->getDefinesHelper()->GetBoolSetting(DefinesHelper::IS_CUPSTAGE_ON);
		if (isCupGame && isCupStageOn )
		{
			bool isCupOver = updateSeasonPlayCupStats(updateIter->first.entityId, statValueMap, statSPOValueMap);

			// Advance to next season
			triggerAdvanceToNextSeason(statValueMap, statSPOValueMap, updateIter->first.entityId, isCupOver);
		}
		else
		{
			updateSeasonPlayPoints(updateIter->first.entityId, statValueMap, statSPOValueMap, statSPLastOppValueMap);
			if (hasSeasonEnded(updateIter->first.entityId, statValueMap))
			{
				processSeasonEndStats(updateIter->first.entityId,statValueMap, statSPOValueMap);

				if (!isCupStageOn) 
					triggerAdvanceToNextSeason(statValueMap, statSPOValueMap, updateIter->first.entityId);
			}
		}

		StatValueUtil::StatValue * svSPORankPts = GetStatValue(statSPOValueMap, STATNAME_RANKINGPOINTS);
		mSeasonalPlayExtension->SetHookEntityRankingPoints(svSPORankPts->iAfter[mStatPeriodType], updateIter->first.entityId);
	}
}

//the stored division is not 
//an index - the divisions start at 1, the index at 0
//so we have to take precautions not to read out of the
//divisions arrays
#define DIVISION_TO_INDEX(division)			((division) - mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::FIRST_DIVISION))
#define INDEX_TO_DIVISION(idx)				((idx) + mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::FIRST_DIVISION))

bool FifaSeasonalPlay::hasSeasonEnded(EntityId entityId, StatValueUtil::StatValueMap* statValueMap)
{
	StatValueUtil::StatValue* seasonPoints = GetStatValue(statValueMap, STATNAME_SPPOINTS);
	StatValueUtil::StatValue * svGamePlyd = GetStatValue(statValueMap, STATNAME_GAMES_PLAYED);

	//get the current division entity is in
	StatValueUtil::StatValue * svCurDiv = GetStatValue(statValueMap, STATNAME_CURDIVISION);
	int64_t curDiv = svCurDiv->iBefore[mStatPeriodType];

	int64_t trgtPtsIndex = DIVISION_TO_INDEX(curDiv);
	int32_t spFirstDivision = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::FIRST_DIVISION);
	int32_t spLastDivision = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::LAST_DIVISION);
	if ( trgtPtsIndex < DIVISION_TO_INDEX(spFirstDivision) || trgtPtsIndex > DIVISION_TO_INDEX(spLastDivision))
	{
		ERR_LOG("[ReportProcessor:"<<this<<"].HasSeasonEnded - div is invalid "<<curDiv<<" with index "<<trgtPtsIndex<<" ")
	}

	int64_t curPoints = seasonPoints->iAfter[mStatPeriodType];
	int64_t numGamesLeft = (GetNumGamesInSeason(statValueMap) - svGamePlyd->iAfter[mStatPeriodType]);
	int64_t maxPoints = numGamesLeft * 3;  // * 3 for wins
	int64_t pointsAt10thGame = curPoints + maxPoints;

	mSeasonalPlayExtension->SetHookEntityCurrentDiv(mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::NUM_DIVISIONS)-curDiv+1, entityId);
	int32_t spTitleTargetPtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::TITLE_TRGT_PTS_TABLE, trgtPtsIndex);
	int32_t spPromoTargetPtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::PROMO_TRGT_PTS_TABLE, trgtPtsIndex);
	int32_t spRelegTargetPtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::RELEG_TRGT_PTS_TABLE, trgtPtsIndex);

	// reached league title
	if ( curPoints >= spTitleTargetPtsTable )
	{
		return true;
	}

	if ( svGamePlyd->iAfter[mStatPeriodType] >= GetNumGamesInSeason(statValueMap))
	{
		return true;
	}

	// reached promotion threshold and cannot reach league title
	else if ( curPoints >= spPromoTargetPtsTable && pointsAt10thGame < spTitleTargetPtsTable)
	{
		return true;
	}
	// max points will never avoid relegation
	else if ( pointsAt10thGame < spRelegTargetPtsTable)
	{
		return true;
	}
	// already passed relegation but max points will never promote them.
	else if ( curPoints >= spRelegTargetPtsTable  && pointsAt10thGame < spPromoTargetPtsTable)
	{
		return true;
	}

	setQualifyingDiv(entityId, static_cast<uint32_t>(curDiv));

	return false;
}

bool FifaSeasonalPlay::processSeasonEndStats(EntityId entityId, StatValueUtil::StatValueMap* statValueMap, StatValueUtil::StatValueMap* statSPOValueMap)
{
	StatValueUtil::StatValue * svSPOCurDiv = GetStatValue(statSPOValueMap, STATNAME_CURDIVISION);
	StatValueUtil::StatValue * svSPOSeason = GetStatValue(statSPOValueMap, STATNAME_SEASONS);
	StatValueUtil::StatValue * svSPORankPts = GetStatValue(statSPOValueMap, STATNAME_RANKINGPOINTS);
	StatValueUtil::StatValue * svSPOSeasonMov = GetStatValue(statSPOValueMap, STATNAME_SEASONMOV);

	StatValueUtil::StatValue * svRankPts = GetStatValue(statValueMap, STATNAME_RANKINGPOINTS);
	StatValueUtil::StatValue*  seasonPoints = GetStatValue(statValueMap, STATNAME_SPPOINTS);
	StatValueUtil::StatValue * svSeason = GetStatValue(statValueMap, STATNAME_SEASONS);
	StatValueUtil::StatValue * svCurDiv = GetStatValue(statValueMap, STATNAME_CURDIVISION);
	StatValueUtil::StatValue * svGamePlyd = GetStatValue(statValueMap, STATNAME_GAMES_PLAYED);

	StatValueUtil::StatValue * svSPOBestDiv = GetStatValue(statSPOValueMap, STATNAME_BESTDIVISION);
	StatValueUtil::StatValue * svSPOBestPts = GetStatValue(statSPOValueMap, STATNAME_BESTPOINTS);
	StatValueUtil::StatValue * svSeasonPts = GetStatValue(statValueMap, STATNAME_SPPOINTS);

	StatValueUtil::StatValue * svSPOTitlesWon[5];
	svSPOTitlesWon[0] = GetStatValue(statSPOValueMap, STATNAME_TITLESWON);
	svSPOTitlesWon[1] = GetStatValue(statSPOValueMap, STATNAME_LEAGUESWON);
	svSPOTitlesWon[2] = GetStatValue(statSPOValueMap, STATNAME_DIVSWON1);
	svSPOTitlesWon[3] = GetStatValue(statSPOValueMap, STATNAME_DIVSWON2);
	svSPOTitlesWon[4] = GetStatValue(statSPOValueMap, STATNAME_DIVSWON3);

	bool seasonEndedEarly = !(svGamePlyd->iAfter[mStatPeriodType] >= GetNumGamesInSeason(statValueMap));

	int64_t curDiv = svCurDiv->iBefore[mStatPeriodType];
	
	int64_t trgtPtsIndex = DIVISION_TO_INDEX(curDiv);
	int32_t spFirstDivision = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::FIRST_DIVISION);
	int32_t spLastDivision = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::LAST_DIVISION);

	if ( trgtPtsIndex < DIVISION_TO_INDEX(spFirstDivision) || trgtPtsIndex > DIVISION_TO_INDEX(spLastDivision))
	{
		ERR_LOG("[ReportProcessor:"<<this<<"].processSeasonEndStats - div is invalid "<<curDiv<<" with index "<<trgtPtsIndex<<" ");
	}

	bool newDivision = false;

	svSPOSeasonMov->iAfter[mStatPeriodType] = (seasonEndedEarly? START_CUPSTAGE_NXT_GAME_EARLY : START_CUPSTAGE_NXT_GAME);

	int32_t spTitlePtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::TITLE_TRGT_PTS_TABLE, trgtPtsIndex);
	int32_t spPromoPtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::PROMO_TRGT_PTS_TABLE, trgtPtsIndex);
	int32_t spRelegPtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::RELEG_TRGT_PTS_TABLE, trgtPtsIndex);
	
	// check for relegations
	if ( seasonPoints->iAfter[mStatPeriodType] < spRelegPtsTable )
	{
		int64_t ptsTableIndex = 0;
		if (svSPOCurDiv->iBefore[mStatPeriodType] > spFirstDivision)
		{
			// cache the next division value int the divisional table so that we can advance once cup stage is done
			svCurDiv->iAfter[mStatPeriodType] = curDiv-1;
			ptsTableIndex = DIVISION_TO_INDEX(curDiv);
			newDivision = true; 
			svSPOSeasonMov->iAfter[mStatPeriodType] = (seasonEndedEarly? SSN_END_RELEGATED_EARLY: SSN_END_RELEGATED);
			mSeasonalPlayExtension->SetHookEntityUpdateDiv(mSeasonalPlayExtension->HOOK_DIV_REL, entityId);
		}

		if ( ptsTableIndex < DIVISION_TO_INDEX(spFirstDivision) || ptsTableIndex > DIVISION_TO_INDEX(spLastDivision))
		{
			ERR_LOG("[ReportProcessor:"<<this<<"].processSeasonEndStats - curDivision is invalid "<<svCurDiv->iAfter[mStatPeriodType]<<" with index "<<ptsTableIndex<<" ");
		}

		int32_t relegPtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::RELEG_PTS_TABLE, ptsTableIndex);
		svRankPts->iAfter[mStatPeriodType] -= relegPtsTable;

		// If this poor user is less than 0 rank points, and overall rank points, set to ZERO
		if ( svRankPts->iAfter[mStatPeriodType] < 0 )
			svRankPts->iAfter[mStatPeriodType] = 0;
		svSPORankPts->iAfter[mStatPeriodType] -= relegPtsTable;
		if ( svSPORankPts->iAfter[mStatPeriodType] < 0 )
			svSPORankPts->iAfter[mStatPeriodType] = 0;
	}
	// check for promotions / league title
	else if ( seasonPoints->iAfter[mStatPeriodType] >= spPromoPtsTable )
	{
		bool isTitle = ( seasonPoints->iAfter[mStatPeriodType] >= spTitlePtsTable );
		int64_t ptsTableIndex = DIVISION_TO_INDEX(curDiv);
		if (svSPOCurDiv->iBefore[mStatPeriodType] < spLastDivision)
		{
			// cache the next division value int the divisional table so that we can advance once cup stage is done
			svCurDiv->iAfter[mStatPeriodType] = curDiv + 1;
			newDivision = true; 
			mSeasonalPlayExtension->SetHookEntityUpdateDiv(mSeasonalPlayExtension->HOOK_DIV_PRO, entityId);
		}

		if ( ptsTableIndex < DIVISION_TO_INDEX(spFirstDivision) || ptsTableIndex > DIVISION_TO_INDEX(spLastDivision))
		{
			ERR_LOG("[ReportProcessor:"<<this<<"].processSeasonEndStats - curDivision is invalid "<<svCurDiv->iAfter[mStatPeriodType]<<" with index "<<ptsTableIndex<<" ");
		}

		if ( isTitle )
		{
			svSPOSeasonMov->iAfter[mStatPeriodType] = (seasonEndedEarly? SSN_END_LGETITLE_EARLY: SSN_END_LGETITLE);

			// increment titles won
			mSeasonalPlayExtension->incrementTitlesWon(entityId, svSPOTitlesWon, svSPOCurDiv->iBefore[mStatPeriodType]);
			
			int32_t titlePtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::TITLE_PTS_TABLE, ptsTableIndex);
			svRankPts->iAfter[mStatPeriodType] += titlePtsTable;
			svSPORankPts->iAfter[mStatPeriodType] += titlePtsTable;
			
		}
		else
		{
			svSPOSeasonMov->iAfter[mStatPeriodType] = (seasonEndedEarly? SSN_END_PROMOTED_EARLY: SSN_END_PROMOTED);
		}
		
		int32_t promoPtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::PROMO_PTS_TABLE, ptsTableIndex);
		svRankPts->iAfter[mStatPeriodType] += promoPtsTable;
		svSPORankPts->iAfter[mStatPeriodType] += promoPtsTable;
	}
	else
	{
		int64_t ptsTableIndex = DIVISION_TO_INDEX(svCurDiv->iAfter[mStatPeriodType]);
		if ( ptsTableIndex < DIVISION_TO_INDEX(spFirstDivision) || ptsTableIndex > DIVISION_TO_INDEX(spLastDivision))
		{
			ERR_LOG("[ReportProcessor:"<<this<<"].processSeasonEndStats - curDivision is invalid "<<svCurDiv->iAfter[mStatPeriodType]<<" with index "<<ptsTableIndex<<" ");
		}
		
		int32_t holdPtsTable = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::HOLD_PTS_TABLE, ptsTableIndex);
		svRankPts->iAfter[mStatPeriodType] += holdPtsTable;
		svSPORankPts->iAfter[mStatPeriodType] += holdPtsTable;

		svSPOSeasonMov->iAfter[mStatPeriodType] = (seasonEndedEarly? SSN_END_HELD_EARLY: SSN_END_HELD);
		mSeasonalPlayExtension->SetHookEntityUpdateDiv(mSeasonalPlayExtension->HOOK_DIV_REP, entityId);
	}

	//update max div
	// new division is higher than current max or it's the user's first time
	if (svCurDiv->iBefore[mStatPeriodType] > svSPOBestDiv->iBefore[mStatPeriodType] ||
			svSPOBestDiv->iBefore[mStatPeriodType] == -1)
	{
		svSPOBestDiv->iAfter[mStatPeriodType] = svCurDiv->iBefore[mStatPeriodType];
		svSPOBestPts->iAfter[mStatPeriodType] = svSeasonPts->iAfter[mStatPeriodType];
	}
	// same division but more points
	else if (svCurDiv->iBefore[mStatPeriodType] == svSPOBestDiv->iBefore[mStatPeriodType] &&
				svSPOBestPts->iBefore[mStatPeriodType] < svSeasonPts->iAfter[mStatPeriodType])
	{
		svSPOBestPts->iAfter[mStatPeriodType] = svSeasonPts->iAfter[mStatPeriodType];
	}

	// reset stats
	svSPOSeason->iAfter[mStatPeriodType] += 1;
	if ( newDivision )
		svSeason->iAfter[mStatPeriodType] = 0;
	else
		svSeason->iAfter[mStatPeriodType] += 1;

	resetSeasonalStats(statValueMap, false);

	return newDivision;
}

//undefining the defines to prevent conflict with other defines 
//while bulk building
#undef DIVISION_TO_INDEX
#undef INDEX_TO_DIVISION

void FifaSeasonalPlay::updateSeasonPlayPoints(EntityId entityId, StatValueUtil::StatValueMap* statValueMap, StatValueUtil::StatValueMap* statSPOValueMap, StatValueUtil::StatValueMap* statSPLastOpponentValueMap)
{	
	int points = 0;
	//uint32_t oppPlayerId = 0;
	StatValueUtil::StatValue * svGamePlyd = GetStatValue(statValueMap, STATNAME_GAMES_PLAYED);
	svGamePlyd->iAfter[mStatPeriodType] += 1;

	IFifaSeasonalPlayExtension::GameResult result = mSeasonalPlayExtension->getGameResult(entityId);

	StatValueUtil::StatValue* seasonWins = GetStatValue(statValueMap, STATNAME_SEASONWINS);
	StatValueUtil::StatValue* seasonTies = GetStatValue(statValueMap, STATNAME_SEASONTIES);
	StatValueUtil::StatValue* seasonLosses = GetStatValue(statValueMap, STATNAME_SEASONLOSSES);

	StatValueUtil::StatValue* prevSeasonWins = GetStatValue(statValueMap, STATNAME_PREVSEASONWINS);
	StatValueUtil::StatValue* prevSeasonTies = GetStatValue(statValueMap, STATNAME_PREVSEASONTIES);
	StatValueUtil::StatValue* prevSeasonLosses = GetStatValue(statValueMap, STATNAME_PREVSEASONLOSSES);

	StatValueUtil::StatValue * svSeason = GetStatValue(statSPOValueMap, STATNAME_SEASONS);
	mSeasonalPlayExtension->SetHookEntitySeasonId(svSeason->iBefore[mStatPeriodType], entityId);
	mSeasonalPlayExtension->SetHookEntityGameNumber(seasonWins->iBefore[mStatPeriodType] + seasonTies->iBefore[mStatPeriodType] + seasonLosses->iBefore[mStatPeriodType], entityId);
	
	prevSeasonWins->iAfter[mStatPeriodType] = seasonWins->iBefore[mStatPeriodType];
	prevSeasonTies->iAfter[mStatPeriodType] = seasonTies->iBefore[mStatPeriodType];
	prevSeasonLosses->iAfter[mStatPeriodType] = seasonLosses->iBefore[mStatPeriodType];

	int32_t disconnectPenalty = 0;

	switch(result)
	{
	case IFifaSeasonalPlayExtension::WIN:
		points = 3;
		seasonWins->iAfter[mStatPeriodType] += 1;
		break;
	case IFifaSeasonalPlayExtension::TIE:
		points = 1;
		seasonTies->iAfter[mStatPeriodType] += 1;
		break;
	case IFifaSeasonalPlayExtension::LOSS:
	default:
		int32_t disconnectLossPenalty = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::DISCONNECT_LOSS_PENALTY);
		disconnectPenalty = mSeasonalPlayExtension->didUserFinish(entityId) ? 0 : disconnectLossPenalty;
		seasonLosses->iAfter[mStatPeriodType] += 1;
	}

	StatValueUtil::StatValue* seasonPoints = GetStatValue(statValueMap, STATNAME_SPPOINTS);
	seasonPoints->iAfter[mStatPeriodType] = (3 * seasonWins->iAfter[mStatPeriodType]) + seasonTies->iAfter[mStatPeriodType];
	
	StatValueUtil::StatValue* lastMatchStats[NUM_LAST_MATCHES];
	StatValueUtil::StatValue* lastMatchOpponents[NUM_LAST_MATCHES];

	bool shouldTrackOpponent = mSeasonalPlayExtension->trackLastOpponentPlayed();
	for (int i = 0; i < NUM_LAST_MATCHES; i++)
	{
		char8_t lastMatchField[16];
		sprintf(lastMatchField, "%s%d", STATNAME_LASTMATCH, i);
		lastMatchStats[i] = GetStatValue(statSPOValueMap, lastMatchField);

		if (shouldTrackOpponent)
		{
			sprintf(lastMatchField, "%s%d", STATNAME_LASTOPPONENT, i);
			lastMatchOpponents[i] = GetStatValue(statSPLastOpponentValueMap, lastMatchField);
			TRACE_LOG("[FifaSeasonalPlay].updateSeasonPlayPoints() - Read:" << lastMatchField <<  
				" as:" << lastMatchOpponents[i]->iBefore[mStatPeriodType] << 
				" ");
		}
	}

	for(int i = NUM_LAST_MATCHES - 1; i > 0; i--)
	{
		lastMatchStats[i]->iAfter[mStatPeriodType] = lastMatchStats[i - 1]->iBefore[mStatPeriodType];

		if (shouldTrackOpponent)
		{
			TRACE_LOG("[FifaSeasonalPlay].updateSeasonPlayPoints() - Setting index:" << i <<  
				" was:" << lastMatchOpponents[i]->iAfter[mStatPeriodType] << 
				" now:" << lastMatchOpponents[i - 1]->iBefore[mStatPeriodType] <<
				" ");
			lastMatchOpponents[i]->iAfter[mStatPeriodType] = lastMatchOpponents[i - 1]->iBefore[mStatPeriodType];
		}
	}

	// win/tie/loss/not played  2,1,0,-1
	lastMatchStats[0]->iAfter[mStatPeriodType] = result;
	// update last opponent
	if (shouldTrackOpponent)
	{
		lastMatchOpponents[0]->iAfter[mStatPeriodType] = mSeasonalPlayExtension->getOpponentId(entityId);
		TRACE_LOG("[FifaSeasonalPlay].updateSeasonPlayPoints() - Setting New Last Opponent:" << lastMatchOpponents[0]->iAfter[mStatPeriodType] <<  
			" ");
	}
	
	StatValueUtil::StatValue * svRankPts = GetStatValue(statValueMap, STATNAME_RANKINGPOINTS);
	StatValueUtil::StatValue * svSPORankPts = GetStatValue(statSPOValueMap, STATNAME_RANKINGPOINTS);
	svRankPts->iAfter[mStatPeriodType] += points;
	svSPORankPts->iAfter[mStatPeriodType] += points;
	svRankPts->iAfter[mStatPeriodType] -= disconnectPenalty;
	svSPORankPts->iAfter[mStatPeriodType] -= disconnectPenalty;

	svRankPts->iAfter[mStatPeriodType] = (svRankPts->iAfter[mStatPeriodType] < 0) ? 0 : svRankPts->iAfter[mStatPeriodType];
	svSPORankPts->iAfter[mStatPeriodType] = (svSPORankPts->iAfter[mStatPeriodType] < 0) ? 0 : svSPORankPts->iAfter[mStatPeriodType];

	StatValueUtil::StatValue * svGF = GetStatValue(statValueMap, STATNAME_GOALS);
	svGF->iAfter[mStatPeriodType] += mSeasonalPlayExtension->getGoalsFor(entityId);

	StatValueUtil::StatValue * svATGF = GetStatValue(statSPOValueMap, STATNAME_ALLTIMEGOALS);
	svATGF->iAfter[mStatPeriodType] += mSeasonalPlayExtension->getGoalsFor(entityId);

	StatValueUtil::StatValue * svGA = GetStatValue(statValueMap, STATNAME_GOALSAGAINST);
	svGA->iAfter[mStatPeriodType] += mSeasonalPlayExtension->getGoalsAgainst(entityId);

	StatValueUtil::StatValue * svATGA = GetStatValue(statSPOValueMap, STATNAME_ALLTIMEGOALSAGAINST);
	svATGA->iAfter[mStatPeriodType] += mSeasonalPlayExtension->getGoalsAgainst(entityId);

	//calculate projected points
	StatValueUtil::StatValue* projectedPoints = GetStatValue(statValueMap, STATNAME_PROJPTS);
	StatValueUtil::StatValue* prevProjectedPoints = GetStatValue(statValueMap, STATNAME_PREVPROJPTS);
	StatValueUtil::StatValue* extraMatches = GetStatValue(statValueMap, STATNAME_EXTRAMATCHES);

	projectedPoints->iAfter[mStatPeriodType] = projectedPoints->iBefore[mStatPeriodType];

	calculateProjectedPoints(projectedPoints->iAfter[mStatPeriodType],
							 prevProjectedPoints->iAfter[mStatPeriodType],
							 svGamePlyd->iAfter[mStatPeriodType],
							 extraMatches->iBefore[mStatPeriodType],
							 seasonPoints->iAfter[mStatPeriodType],
							 mSeasonalPlayExtension->getDefinesHelper());
}

bool FifaSeasonalPlay::updateSeasonPlayCupStats(EntityId entityId, StatValueUtil::StatValueMap* statValueMap, StatValueUtil::StatValueMap* statSPOValueMap)
{
	bool isCupDone = false;
	StatValueUtil::StatValue * svCupRound = GetStatValue(statValueMap, STATNAME_CUPROUND);

	
	StatValueUtil::StatValue * svSPOSeasonMov = GetStatValue(statSPOValueMap, STATNAME_SEASONMOV);
	StatValueUtil::StatValue * svSPOCupsWon = GetStatValue(statSPOValueMap, STATNAME_CUPSWON1);

	int64_t cupRound = svCupRound->iBefore[mStatPeriodType];
	mSeasonalPlayExtension->SetHookEntityGameNumber(cupRound, entityId);

	bool isWinner = false;

	int32_t cupLastRound = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::CUP_LAST_ROUND);

	IFifaSeasonalPlayExtension::GameResult result = mSeasonalPlayExtension->getGameResult(entityId);

	switch (result)
	{
	case IFifaSeasonalPlayExtension::TIE:
		ERR_LOG("[ReportProcessor::UpdateSeasonPlayCupStats]: no ties games allowed in season play cup matches "<<entityId<<" ");
		break;
	case IFifaSeasonalPlayExtension::WIN:
		isCupDone = (cupRound == cupLastRound);
		isWinner = true;
		break;
	case IFifaSeasonalPlayExtension::LOSS:
	default:
		isCupDone = true;
		isWinner = false;
	}

	StatValueUtil::StatValue * svSPOLastMatch = GetStatValue(statSPOValueMap, STATNAME_LASTMATCH);
	svSPOLastMatch->iAfter[mStatPeriodType] = (isWinner)? 1: -1;

	if ( isCupDone )
	{
		StatValueUtil::StatValue * svSPORankPts = GetStatValue(statSPOValueMap, STATNAME_RANKINGPOINTS);

		int32_t rankPoints = 0;
		svCupRound->iAfter[mStatPeriodType] = 0;
		svSPOSeasonMov->iAfter[mStatPeriodType] = CUP_END_NOPLACE;
		int32_t cupRankPtsFirst = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::CUP_RANKPTS_1ST);
		int32_t cupRankPtsSecond = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::CUP_RANKPTS_2ND);
		int32_t cupSemiFinalist = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::CUP_SEMIFINALIST);
		int32_t cupQuarterFinalist = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::CUP_QUARTERFINALIST);
		int32_t cupDoneLastRound = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::CUP_LAST_ROUND);
		
		if (cupRound == cupDoneLastRound)
		{
			if ( isWinner )
			{
				rankPoints = cupRankPtsFirst;
				svSPOSeasonMov->iAfter[mStatPeriodType] = CUP_END_1ST_PLACE;
				svSPOCupsWon->iAfter[mStatPeriodType] += 1;
			}
			else
			{
				rankPoints = cupRankPtsSecond;
				svSPOSeasonMov->iAfter[mStatPeriodType] = CUP_END_2ND_PLACE;
			}
		}
		else if (cupRound == cupDoneLastRound-1)
		{
			if ( !isWinner )
			{
				rankPoints = cupSemiFinalist;
				svSPOSeasonMov->iAfter[mStatPeriodType] = CUP_END_4TH_PLACE;
			}
		}
		else if (cupRound == cupDoneLastRound-2)
		{
			if ( !isWinner )
			{
				rankPoints = cupQuarterFinalist;
			}
		}
		svSPORankPts->iAfter[mStatPeriodType] += rankPoints;
		resetSeasonalStats(statValueMap, true);
	}
	// Advance to next round
	else
	{
		svCupRound->iAfter[mStatPeriodType] = cupRound+1;
	}
	return isCupDone;
}

void FifaSeasonalPlay::resetSeasonalStats(StatValueUtil::StatValueMap* statValueMap, bool isCupStage)
{
	StatValueUtil::StatValue * svGF = GetStatValue(statValueMap, STATNAME_GOALS);
	StatValueUtil::StatValue * svGA = GetStatValue(statValueMap, STATNAME_GOALSAGAINST);
	StatValueUtil::StatValue * svCWins = GetStatValue(statValueMap, STATNAME_CUPWINS);
	StatValueUtil::StatValue * svCLoss = GetStatValue(statValueMap, STATNAME_CUPLOSSES);
	StatValueUtil::StatValue * svWins = GetStatValue(statValueMap, STATNAME_SEASONWINS);
	StatValueUtil::StatValue * svLss = GetStatValue(statValueMap, STATNAME_SEASONLOSSES);
	StatValueUtil::StatValue * svTies = GetStatValue(statValueMap, STATNAME_SEASONTIES);
	StatValueUtil::StatValue * svCRnd = GetStatValue(statValueMap, STATNAME_CUPROUND);
	StatValueUtil::StatValue * svRankPts = GetStatValue(statValueMap, STATNAME_RANKINGPOINTS);
	StatValueUtil::StatValue * svGamesPlayed = GetStatValue(statValueMap, STATNAME_GAMES_PLAYED);
	StatValueUtil::StatValue * seasonPoints = GetStatValue(statValueMap, STATNAME_SPPOINTS);
	StatValueUtil::StatValue * projPoints = GetStatValue(statValueMap, STATNAME_PROJPTS);
	StatValueUtil::StatValue * extraMatches = GetStatValue(statValueMap, STATNAME_EXTRAMATCHES);
	StatValueUtil::StatValue * winRedeemed = GetStatValue(statValueMap, STATNAME_WINREDEEMED);
	StatValueUtil::StatValue * drawRedeemed = GetStatValue(statValueMap, STATNAME_DRAWREDEEMED);
	StatValueUtil::StatValue * matchRedeemed = GetStatValue(statValueMap, STATNAME_MATCHREDEEMED);
	svCWins->iAfter[mStatPeriodType] = 0;
	svCLoss->iAfter[mStatPeriodType] = 0;

	bool isCupStageOn = mSeasonalPlayExtension->getDefinesHelper()->GetBoolSetting(DefinesHelper::IS_CUPSTAGE_ON);

	if ( isCupStageOn  )
	{
		if (!isCupStage)
		{
			svCRnd->iAfter[mStatPeriodType] = 1;
		}
		else 
		{
			svGF->iAfter[mStatPeriodType] = 0;
			svGA->iAfter[mStatPeriodType] = 0;
			svWins->iAfter[mStatPeriodType] = 0;
			svLss->iAfter[mStatPeriodType] = 0;
			svTies->iAfter[mStatPeriodType] = 0;
			svCRnd->iAfter[mStatPeriodType] = 0;
			svRankPts->iAfter[mStatPeriodType] = 0;
			svGamesPlayed->iAfter[mStatPeriodType] = 0;
			seasonPoints->iAfter[mStatPeriodType] = 0;
			projPoints->iAfter[mStatPeriodType] = -1;
			extraMatches->iAfter[mStatPeriodType] = 0;
		}
	}
	else
	{
		svGF->iAfter[mStatPeriodType] = 0;
		svGA->iAfter[mStatPeriodType] = 0;
		svWins->iAfter[mStatPeriodType] = 0;
		svLss->iAfter[mStatPeriodType] = 0;
		svTies->iAfter[mStatPeriodType] = 0;
		svCRnd->iAfter[mStatPeriodType] = 0;
		svRankPts->iAfter[mStatPeriodType] = 0;
		svGamesPlayed->iAfter[mStatPeriodType] = 0;
		seasonPoints->iAfter[mStatPeriodType] = 0;
		projPoints->iAfter[mStatPeriodType] = -1;
		extraMatches->iAfter[mStatPeriodType] = 0;
		winRedeemed->iAfter[mStatPeriodType] = 0;
		drawRedeemed->iAfter[mStatPeriodType] = 0;
		matchRedeemed->iAfter[mStatPeriodType] = 0;
	}
}

void FifaSeasonalPlay::triggerAdvanceToNextSeason(StatValueUtil::StatValueMap* statValueMap, StatValueUtil::StatValueMap* statSPOValueMap, EntityId entity, bool isCupOver )
{
	if ( isCupOver )
	{
		// Advance to next season
		StatValueUtil::StatValue * svCurDiv = GetStatValue(statValueMap, STATNAME_CURDIVISION);
		StatValueUtil::StatValue * svSPOCurDiv = GetStatValue(statSPOValueMap, STATNAME_CURDIVISION);
		StatValueUtil::StatValue * svSPOPrevDiv = GetStatValue(statSPOValueMap, STATNAME_PREVDIVISION);
		StatValueUtil::StatValue * svSPOMaxDiv = GetStatValue(statSPOValueMap, STATNAME_MAXDIVISION);
		StatValueUtil::StatValue * svSPOSeasons = GetStatValue(statSPOValueMap, STATNAME_SEASONS);

		int64_t previousDiv = svSPOCurDiv->iBefore[mStatPeriodType];
		svSPOPrevDiv->iAfter[mStatPeriodType] = previousDiv;

		if ( svCurDiv->iAfter[mStatPeriodType] != previousDiv )
		{
			svSPOCurDiv->iAfter[mStatPeriodType] = svCurDiv->iAfter[mStatPeriodType];

			//check for max div
			if (svSPOCurDiv->iAfter[mStatPeriodType] > svSPOMaxDiv->iBefore[mStatPeriodType])
			{
				svSPOMaxDiv->iAfter[mStatPeriodType] = svSPOCurDiv->iAfter[mStatPeriodType];
			}

			//update promotion, relegation or held count
			if ( svCurDiv->iAfter[mStatPeriodType] < previousDiv )
			{
				StatValueUtil::StatValue * svSPORlgs = GetStatValue(statSPOValueMap, STATNAME_RELEGATIONS);
				svSPORlgs->iAfter[mStatPeriodType] += 1;
			}
			else if (svCurDiv->iAfter[mStatPeriodType] > previousDiv )
			{
				StatValueUtil::StatValue * svSPOPromo = GetStatValue(statSPOValueMap, STATNAME_PROMOTIONS);
				svSPOPromo->iAfter[mStatPeriodType] += 1;
			}
		
			//update div count stats
			if (svSPOSeasons->iAfter[mStatPeriodType] > 2)
			{
				TRACE_LOG("[triggerAdvanceToNextSeason] trying to decrement div count for div "<<previousDiv<<" ");
				mSeasonalPlayExtension->decrementDivCount(previousDiv);
			}

			TRACE_LOG("[triggerAdvanceToNextSeason] trying to incrementStat div count for div "<<svCurDiv->iAfter[mStatPeriodType]<<" ");
			mSeasonalPlayExtension->incrementDivCount(svCurDiv->iAfter[mStatPeriodType]);
		}
		else
		{
			//update div count stats
			if (svSPOSeasons->iAfter[mStatPeriodType] == 2)
			{
				mSeasonalPlayExtension->incrementDivCount(svCurDiv->iAfter[mStatPeriodType]);
				TRACE_LOG("[triggerAdvanceToNextSeason] trying to incrementStat div count for div "<<svCurDiv->iAfter[mStatPeriodType]<<" ");
			}

			//did not get promoted or relegated
			StatValueUtil::StatValue * svSPOHold = GetStatValue(statSPOValueMap, STATNAME_HOLDS);
			svSPOHold->iAfter[mStatPeriodType] += 1;
		}

		setQualifyingDiv(entity, static_cast<uint32_t>(svCurDiv->iAfter[mStatPeriodType]));
	}
}

void FifaSeasonalPlay::setQualifyingDiv(EntityId entityId, uint32_t div)
{
	TRACE_LOG("[FifaSeasonalPlay:" << this << "].setQualifyingDiv");
	FifaCups::FifaCupsSlave *fifaCupsComponent =
		static_cast<FifaCups::FifaCupsSlave*>(gController->getComponent(FifaCups::FifaCupsSlave::COMPONENT_ID,false));
	if (fifaCupsComponent != NULL)
	{
		BlazeRpcError error = Blaze::ERR_OK;

		FifaCups::SetQualifyingDivRequest request;
		request.setMemberId(static_cast<Blaze::FifaCups::MemberId>(entityId));
		request.setMemberType(mSeasonalPlayExtension->getMemberType());
		request.setDiv(div);

		UserSession::pushSuperUserPrivilege();
		error = fifaCupsComponent->setQualifyingDiv(request);
		UserSession::popSuperUserPrivilege();

		if (error != Blaze::ERR_OK)
		{
			TRACE_LOG("[FifaSeasonalPlay:" << this << "].setQualifyingDiv was not successful");
		}
	}
}

void FifaSeasonalPlay::updateCupStats()
{
	TRACE_LOG("[FifaSeasonalPlay:" << this << "].updateCupStats");
	FifaCups::FifaCupsSlave *fifaCupsComponent =
		static_cast<FifaCups::FifaCupsSlave*>(gController->getComponent(FifaCups::FifaCupsSlave::COMPONENT_ID,false));
	if (fifaCupsComponent == NULL)
	{
		ERR_LOG("[FifaSeasonalPlay:"<<this<<"].updateCupStats() - unable to request FifaCups component");
		return;
	}

	TRACE_LOG("[FifaSeasonalPlay:"<<this<<"] - Processing Cup stats");
	
	IFifaSeasonalPlayExtension::CupResult cupResult;
	mSeasonalPlayExtension->getCupResult(cupResult);

	const char8_t *winningMetaData, *losingMetaData;
	losingMetaData = "Penalties=0";
	if (cupResult.mWentToPenalties)
	{
		winningMetaData = "Penalties=1";
	}
	else
	{
		winningMetaData = "Penalties=0";
	}

	IFifaSeasonalPlayExtension::CupSideInfo* cupWinner = &cupResult.mSideInfo[IFifaSeasonalPlayExtension::CUP_WINNER];
	IFifaSeasonalPlayExtension::CupSideInfo* cupLoser = &cupResult.mSideInfo[IFifaSeasonalPlayExtension::CUP_LOSER];

	BlazeRpcError blazeError = ERR_OK;

	Blaze::FifaCups::GetCupInfoRequest request;
	Blaze::FifaCups::CupInfo cupInfoResponse;

	request.setMemberId(cupWinner->id);
	request.setMemberType(mSeasonalPlayExtension->getMemberType());

	UserSession::pushSuperUserPrivilege();
	blazeError = fifaCupsComponent->getCupInfo(request, cupInfoResponse);
	UserSession::popSuperUserPrivilege();
	if (Blaze::ERR_OK != blazeError)
	{
		ERR_LOG("[FifaSeasonalPlay:" << this << "].updateCupStats: unable to get cup info.");
	}

	if (blazeError == ERR_OK && cupInfoResponse.getCupState() == Blaze::FifaCups::FIFACUPS_CUP_STATE_ACTIVE 
					&& cupWinner->id != 0 && cupLoser->id != 0)
	{
		Blaze::OSDKTournaments::TournamentId tournamentId = cupInfoResponse.getTournamentId();

		if (tournamentId != 0)
		{
			TRACE_LOG("[FifaSeasonalPlay].updateCupStats() tournamentId: "<< tournamentId
				<<", winningId: "<< cupWinner->id
				<<", losingId: "<< cupLoser->id
				<<", winningTeamId: "<< cupWinner->teamId
				<<", losingTeamId: "<< cupLoser->teamId
				<<", winningScore: "<< cupWinner->score
				<<", losingScore: "<< cupLoser->score <<" ");

			OSDKTournamentsUtil osdkTournamentsUtil;

			// Values for cupWinnerRound/cupLoserRound are the same for the moment but, if matchmaking is changed to match users from other rounds they may end up with different values.
			OSDKTournaments::TournamentMemberData data;
			Blaze::DbError err = osdkTournamentsUtil.getMemberStatus(cupWinner->id, mSeasonalPlayExtension->getCupMode(), data);

			int cupWinnerRound = 0; 
			if (Blaze::DB_ERR_OK != err)
			{
				ERR_LOG("[FifaSeasonalPlay:" << this << "].updateCupStats: unable to get cup round.");
			}
			else
			{
				cupWinnerRound = data.getLevel();
			}
			
			err = osdkTournamentsUtil.getMemberStatus(cupLoser->id,  mSeasonalPlayExtension->getCupMode(), data);

			int cupLoserRound = 0; 
			if (Blaze::DB_ERR_OK != err)
			{
				ERR_LOG("[FifaSeasonalPlay:" << this << "].updateCupStats: unable to get cup round.");
			}
			else
			{
				cupLoserRound = data.getLevel(); 
			}

			blazeError = osdkTournamentsUtil.reportMatchResult(tournamentId,
														cupWinner->id, cupLoser->id, 
														cupWinner->teamId, cupLoser->teamId, 
														cupWinner->score, cupLoser->score, 
														winningMetaData, losingMetaData);

			if (blazeError == ERR_OK)
			{
				request.setMemberId(cupLoser->id);
				request.setMemberType(mSeasonalPlayExtension->getMemberType());

				UserSession::pushSuperUserPrivilege();
				blazeError = fifaCupsComponent->getCupInfo(request, cupInfoResponse);
				UserSession::popSuperUserPrivilege();

				mSeasonalPlayExtension->SetHookEntityCupId(cupInfoResponse.getCupId(), cupLoser->id);


				request.setMemberId(cupWinner->id);

				UserSession::pushSuperUserPrivilege();
				blazeError = fifaCupsComponent->getCupInfo(request, cupInfoResponse);
				UserSession::popSuperUserPrivilege();

				mSeasonalPlayExtension->SetHookEntityCupId(cupInfoResponse.getCupId(), cupWinner->id);
				

				mSeasonalPlayExtension->SetHookEntityGameNumber(cupWinnerRound, cupWinner->id);
				mSeasonalPlayExtension->SetHookEntityGameNumber(cupLoserRound,  cupLoser->id);

				if (cupInfoResponse.getTournamentStatus() == Blaze::FifaCups::FIFACUPS_TOURNAMENT_STATUS_WON)
				{
					//won cup tournament hook
					mSeasonalPlayExtension->SetHookEntityWonCompetition(cupInfoResponse.getSeasonId(),cupWinner->id);
					mSeasonalPlayExtension->SetHookEntityWonCup(cupWinner->id);
				}
			}
		}
	}
}

void FifaSeasonalPlay::updateDivCounts()
{
	static EA_THREAD_LOCAL uint32_t lastUpdateTime = 0;

	// Limit frequency of global stats updates, to avoid DB load issues (dead-lock)
	uint32_t currentTime = static_cast<uint32_t>(TimeValue::getTimeOfDay().getSec());
	if (currentTime - lastUpdateTime >= ONLINE_MATCH_STATS_INTERVAL)
	{
		lastUpdateTime = currentTime;
		
		eastl::vector<int> incrementCount, decrementCount;

		mSeasonalPlayExtension->getDivCounts(incrementCount, decrementCount);

		int size = incrementCount.size();

		for (int i = 0; i < size; i++)
		{
			int count = incrementCount[i] - decrementCount[i];
			TRACE_LOG("[updateDivCounts] div "<<(i + 1)<<" has count "<<count<<" (incr "<<incrementCount[i]<<" decr "<<decrementCount[i] <<")");

			if (count == 0)
			{
				continue;
			}

			Stats::ScopeNameValueMap scopeNameValueMap;
			mSeasonalPlayExtension->getKeyScopes(scopeNameValueMap);

			mBuilder->startStatRow(mSeasonalPlayExtension->getDivCountStatCategory(), (i + 1), scopeNameValueMap.empty() ? NULL : &scopeNameValueMap);
			if (count > 0)
			{
				TRACE_LOG("[updateDivCounts] incrementStat by count "<<count<<" ");
				mBuilder->incrementStat("count", count);
			}
			else
			{
				TRACE_LOG("[updateDivCounts] decrementStat by count "<<abs(count)<<" ");
				mBuilder->decrementStat("count", abs(count));
			}
			mBuilder->completeStatRow();
		}
	}
}

void FifaSeasonalPlay::calculateProjectedPoints(int64_t& projectedPoints,int64_t& prevProjectedPoints,int64_t gamesPlayed,int64_t extraMatches,int64_t seasonCurrentPoints,const GameReporting::DefinesHelper* pDefinesHelper)
{
	int32_t pointProjectionThreshold = pDefinesHelper->GetIntSetting(GameReporting::DefinesHelper::POINT_PROJECTION_THRESHOLD);
	prevProjectedPoints=projectedPoints;
	if (gamesPlayed >= pointProjectionThreshold)
	{
		float pointsAvg = 0.0f;
		if (gamesPlayed > 0)
		{
			pointsAvg = seasonCurrentPoints / (float)gamesPlayed;
		}
		float gamesRemaining = (float) (pDefinesHelper->GetIntSetting(GameReporting::DefinesHelper::MAX_NUM_GAMES_IN_SEASON) + extraMatches - gamesPlayed);
		projectedPoints = (int)(seasonCurrentPoints + (gamesRemaining * pointsAvg));
	}
	else
	{
		projectedPoints = -1;
	}
}

int64_t FifaSeasonalPlay::GetNumGamesInSeason(StatValueUtil::StatValueMap* statValueMap)
{
	int64_t retval = mSeasonalPlayExtension->getDefinesHelper()->GetIntSetting(DefinesHelper::MAX_NUM_GAMES_IN_SEASON);
	
	StatValueUtil::StatValue* extraMatches = GetStatValue(statValueMap, STATNAME_EXTRAMATCHES);

	retval = retval + extraMatches->iBefore[mStatPeriodType];

	return retval;
}

void FifaSeasonalPlay::getSPDivisionalStatXmlString(uint64_t entityId, eastl::string& buffer)
{
	buffer.append(mSPDivisionalPlayerStatsXml[entityId]);
}

void FifaSeasonalPlay::getSPOverallStatXmlString(uint64_t entityId, eastl::string& buffer)
{
	buffer.append(mSPOverallPlayerStatsXml[entityId]);
}

} //namespace GameReportingLegacy
} //namespace Blaze
