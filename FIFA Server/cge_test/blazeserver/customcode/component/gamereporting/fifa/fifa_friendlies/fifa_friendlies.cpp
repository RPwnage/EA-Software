/*************************************************************************************************/
/*!
    \file   fifa_seasonalplay.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "fifa_friendlies.h"
#include "fifa_friendliesdefines.h"

#include "fifa/fifastatsvalueutil.h"
#include "component/stats/tdf/stats.h"

namespace Blaze
{
namespace GameReporting
{

void FifaFriendlies::initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport)
{
	mBuilder = builder;
    mUpdateStatsHelper = updateStatsHelper;
	mProcessedReport = processedReport;
}

void FifaFriendlies::setExtension(IFifaFriendliesExtension* seasonalPlayExtension)
{
	mFriendliesExtension = seasonalPlayExtension;
}

void FifaFriendlies::selectFriendliesStats()
{
	IFifaFriendliesExtension::FriendlyList friendlyList;
	mFriendliesExtension->getCategoryInfo(friendlyList);

	IFifaFriendliesExtension::FriendlyList::iterator iter = friendlyList.begin();
	IFifaFriendliesExtension::FriendlyList::iterator end = friendlyList.end();


	for(; iter != end; ++iter)
	{
		IFifaFriendliesExtension::FriendyPair* friendlyPair = iter;

		for (int i = 0; i < SIDE_NUM; ++i)
		{
			int64_t selfId = friendlyPair->entities[i];
			int64_t oppId = friendlyPair->entities[i == SIDE_HOME? SIDE_AWAY : SIDE_HOME];

			Stats::ScopeNameValueMap scopeNamevalueMap;
			scopeNamevalueMap.insert(eastl::make_pair(SCOPENAME_FRIEND, oppId));

			//select friendlies stats
			TRACE_LOG("[FifaFriendlies].selectFriendliesStats() Category " << friendlyPair->category << " for " << selfId << " versus" << oppId << " ");
			mBuilder->startStatRow(friendlyPair->category, selfId, &scopeNamevalueMap);

			mBuilder->selectStat(STATNAME_GOALS1);
			mBuilder->selectStat(STATNAME_GOALS2);
			mBuilder->selectStat(STATNAME_GOALS3);
			mBuilder->selectStat(STATNAME_GOALS4);
			mBuilder->selectStat(STATNAME_GOALS5);
			mBuilder->selectStat(STATNAME_GOALS6);
			mBuilder->selectStat(STATNAME_GOALS7);
			mBuilder->selectStat(STATNAME_GOALS8);
			mBuilder->selectStat(STATNAME_GOALS9);
			mBuilder->selectStat(STATNAME_GOALS10);
			mBuilder->selectStat(STATNAME_GOALSAGAINST1);
			mBuilder->selectStat(STATNAME_GOALSAGAINST2);
			mBuilder->selectStat(STATNAME_GOALSAGAINST3);
			mBuilder->selectStat(STATNAME_GOALSAGAINST4);
			mBuilder->selectStat(STATNAME_GOALSAGAINST5);
			mBuilder->selectStat(STATNAME_GOALSAGAINST6);
			mBuilder->selectStat(STATNAME_GOALSAGAINST7);
			mBuilder->selectStat(STATNAME_GOALSAGAINST8);
			mBuilder->selectStat(STATNAME_GOALSAGAINST9);
			mBuilder->selectStat(STATNAME_GOALSAGAINST10);
			mBuilder->selectStat(STATNAME_POINTS1);
			mBuilder->selectStat(STATNAME_POINTS2);
			mBuilder->selectStat(STATNAME_POINTS3);
			mBuilder->selectStat(STATNAME_POINTS4);
			mBuilder->selectStat(STATNAME_POINTS5);
			mBuilder->selectStat(STATNAME_POINTS6);
			mBuilder->selectStat(STATNAME_POINTS7);
			mBuilder->selectStat(STATNAME_POINTS8);
			mBuilder->selectStat(STATNAME_POINTS9);
			mBuilder->selectStat(STATNAME_POINTS10);
			mBuilder->selectStat(STATNAME_CURR_SEASON);
			mBuilder->selectStat(STATNAME_LOSSES_SEASON);
			mBuilder->selectStat(STATNAME_TIES_SEASON);
			mBuilder->selectStat(STATNAME_WINS_SEASON);
			mBuilder->selectStat(STATNAME_FOULS_SEASON);
			mBuilder->selectStat(STATNAME_FOULS_AGAINST_SEASON);
			mBuilder->selectStat(STATNAME_SHOTS_ON_GOAL_SEASON);
			mBuilder->selectStat(STATNAME_SHOTS_AGAINST_ON_GOAL_SEASON);
			mBuilder->selectStat(STATNAME_POSSESSION_SEASON);
			mBuilder->selectStat(STATNAME_TITLE);
			mBuilder->selectStat(STATNAME_TITLES);
			mBuilder->selectStat(STATNAME_TEAM1);
			mBuilder->selectStat(STATNAME_TEAM2);
			mBuilder->selectStat(STATNAME_TEAM3);
			mBuilder->selectStat(STATNAME_TEAM4);
			mBuilder->selectStat(STATNAME_TEAM5);
			mBuilder->selectStat(STATNAME_TEAM6);
			mBuilder->selectStat(STATNAME_TEAM7);
			mBuilder->selectStat(STATNAME_TEAM8);
			mBuilder->selectStat(STATNAME_TEAM9);
			mBuilder->selectStat(STATNAME_TEAM10);
			mBuilder->selectStat(STATNAME_OPPTEAM1);
			mBuilder->selectStat(STATNAME_OPPTEAM2);
			mBuilder->selectStat(STATNAME_OPPTEAM3);
			mBuilder->selectStat(STATNAME_OPPTEAM4);
			mBuilder->selectStat(STATNAME_OPPTEAM5);
			mBuilder->selectStat(STATNAME_OPPTEAM6);
			mBuilder->selectStat(STATNAME_OPPTEAM7);
			mBuilder->selectStat(STATNAME_OPPTEAM8);
			mBuilder->selectStat(STATNAME_OPPTEAM9);
			mBuilder->selectStat(STATNAME_OPPTEAM10);			
			mBuilder->selectStat(STATNAME_WINS);
			mBuilder->selectStat(STATNAME_TIES);
			mBuilder->selectStat(STATNAME_LOSSES);
			mBuilder->selectStat(STATNAME_FOULS);
			mBuilder->selectStat(STATNAME_FOULS_AGAINST);
			mBuilder->selectStat(STATNAME_SHOTS_ON_GOAL);
			mBuilder->selectStat(STATNAME_SHOTS_AGAINST_ON_GOAL);
			mBuilder->selectStat(STATNAME_POSSESSION);
			mBuilder->selectStat(STATNAME_LASTMATCH);

			mBuilder->selectStat(STATNAME_GOALS);
			mBuilder->selectStat(STATNAME_GOALS_AGAINST);

			mBuilder->completeStatRow();
		}
	}
}

void FifaFriendlies::transformFriendliesStats()
{
	IFifaFriendliesExtension::FriendlyList friendlyList;
	mFriendliesExtension->getCategoryInfo(friendlyList);
	
	IFifaFriendliesExtension::FriendlyList::iterator iter = friendlyList.begin();
	IFifaFriendliesExtension::FriendlyList::iterator end = friendlyList.end();

	eastl::vector<UpdateRowKeyList> updateRowKeyListPerCategory;
	for(; iter != end; ++iter)
	{
		IFifaFriendliesExtension::FriendyPair* friendlyPair = iter;
		UpdateRowKeyList updateKeyList;

		for (int i = 0; i < SIDE_NUM; ++i)
		{
			TRACE_LOG("[FifaFriendlies].transformFriendliesStats() generating keys for entity: "<< friendlyPair->entities[i] <<" " );

			Stats::UpdateRowKey* key = mBuilder->getUpdateRowKey(friendlyPair->category, friendlyPair->entities[i]);
			if (key != NULL)
				updateKeyList.push_back(*key);
		}

		updateRowKeyListPerCategory.push_back(updateKeyList);
	}
	
	eastl::vector<UpdateRowKeyList>::iterator updateRowKeyListPerCategoryIter = updateRowKeyListPerCategory.begin();
	eastl::vector<UpdateRowKeyList>::iterator updateRowKeyListPerCategoryEnd = updateRowKeyListPerCategory.end();

	for (; updateRowKeyListPerCategoryIter != updateRowKeyListPerCategoryEnd; ++updateRowKeyListPerCategoryIter)
	{
		UpdateRowKeyList* friendlyKeys = updateRowKeyListPerCategoryIter;
	
		StatValueUtil::StatUpdateMap statUpdateMap;

		readFriendliesStats(*friendlyKeys, statUpdateMap);

		updateFriendlySeasonStats(*friendlyKeys, &statUpdateMap);

		writeFriendliesStats(statUpdateMap);
	}
}

void FifaFriendlies::readFriendliesStats(FifaFriendlies::UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap)
{
	UpdateRowKeyList::iterator iter = keyList.begin();
	UpdateRowKeyList::iterator end = keyList.end();

	for (; iter != end; iter++)
	{
		StatValueUtil::StatUpdate statUpdate;
		Stats::UpdateRowKey key = *iter;
		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key.period);

		insertStat(statUpdate, STATNAME_GOALS1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS1, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS2, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS3, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS4, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS5, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS5, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS6, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS6, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS7, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS7, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS8, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS8, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS9, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS9, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS10, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS10, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST1, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST2, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST3, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST4, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST5, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST5, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST6, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST6, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST7, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST7, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST8, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST8, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST9, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST9, periodType, true));
		insertStat(statUpdate, STATNAME_GOALSAGAINST10, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALSAGAINST10, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS1, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS2, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS3, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS4, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS5, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS5, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS6, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS6, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS7, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS7, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS8, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS8, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS9, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS9, periodType, true));
		insertStat(statUpdate, STATNAME_POINTS10, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POINTS10, periodType, true));
		insertStat(statUpdate, STATNAME_CURR_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CURR_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_LOSSES_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LOSSES_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_TIES_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TIES_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_WINS_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_WINS_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_FOULS_AGAINST_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_FOULS_AGAINST_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_FOULS_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_FOULS_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_SHOTS_AGAINST_ON_GOAL_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SHOTS_AGAINST_ON_GOAL_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_SHOTS_ON_GOAL_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SHOTS_ON_GOAL_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_POSSESSION_SEASON, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POSSESSION_SEASON, periodType, true));
		insertStat(statUpdate, STATNAME_TITLE, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TITLE, periodType, true));
		insertStat(statUpdate, STATNAME_TITLES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TITLES, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM1, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM2, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM3, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM4, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM5, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM5, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM6, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM6, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM7, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM7, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM8, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM8, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM9, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM9, periodType, true));
		insertStat(statUpdate, STATNAME_TEAM10, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TEAM10, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM1, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM1, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM2, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM2, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM3, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM3, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM4, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM4, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM5, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM5, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM6, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM6, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM7, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM7, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM8, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM8, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM9, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM9, periodType, true));
		insertStat(statUpdate, STATNAME_OPPTEAM10, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPTEAM10, periodType, true));
		insertStat(statUpdate, STATNAME_WINS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_WINS, periodType, true));
		insertStat(statUpdate, STATNAME_TIES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TIES, periodType, true));
		insertStat(statUpdate, STATNAME_LOSSES, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LOSSES, periodType, true));
		insertStat(statUpdate, STATNAME_FOULS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_FOULS, periodType, true));
		insertStat(statUpdate, STATNAME_FOULS_AGAINST, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_FOULS_AGAINST, periodType, true));
		insertStat(statUpdate, STATNAME_SHOTS_ON_GOAL, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SHOTS_ON_GOAL, periodType, true));
		insertStat(statUpdate, STATNAME_SHOTS_AGAINST_ON_GOAL, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_SHOTS_AGAINST_ON_GOAL, periodType, true));
		insertStat(statUpdate, STATNAME_POSSESSION, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_POSSESSION, periodType, true));

		insertStat(statUpdate, STATNAME_LASTMATCH, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LASTMATCH, periodType, true));

		insertStat(statUpdate, STATNAME_GOALS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS, periodType, true));
		insertStat(statUpdate, STATNAME_GOALS_AGAINST, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GOALS_AGAINST, periodType, true));

		statUpdateMap.insert(eastl::make_pair(key, statUpdate));
	}
}

void FifaFriendlies::writeFriendliesStats(StatValueUtil::StatUpdateMap& statUpdateMap)
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

			TRACE_LOG("[FifaSeasonalPlay].writeSeasonalPlayStats() - Entity id:" << key->entityId <<  
																		" Category:" << key->category << 
																		" Period:" << key->period << 
																		" Stat Name:" << statValueIter->first << 
																		" Value:" << statValue->iAfter[periodType] << 
																		" ");
		}
	}
}


int64_t FifaFriendlies::update_possession(int64_t currentPossession, int64_t newPossession, int64_t numGames,int side)
{
	// calculate the round possession number and if the decimal part is 0.5 the decision is taken based on side(home/away) 
	int64_t totalPossession=currentPossession*numGames+newPossession;

	if( (2* (totalPossession%(numGames+1) ) > numGames+1) ||
		((2*(totalPossession%(numGames+1)) == numGames+1 ) && (side%2)) )
	{
		return totalPossession/(numGames+1)+1;
	}
	else
	{
		return totalPossession/(numGames+1);
	}

}

void FifaFriendlies::updateFriendlySeasonStats(UpdateRowKeyList keys, StatValueUtil::StatUpdateMap *statUpdateMap)
{
	int64_t goalsAgainst[SIDE_NUM][Stats::STAT_NUM_PERIODS];
	int64_t goals[SIDE_NUM][Stats::STAT_NUM_PERIODS];
	int64_t numGames[SIDE_NUM][Stats::STAT_NUM_PERIODS];
	int64_t points[SIDE_NUM][Stats::STAT_NUM_PERIODS];
	UpdateRowKeyList::const_iterator keyIter = keys.begin();
	UpdateRowKeyList::const_iterator keyEnd = keys.end();

	EntityId eId1 = (keyIter != keyEnd) ? keyIter->entityId:0;
	keyIter++;
	EntityId eId2 = (keyIter != keyEnd) ? keyIter->entityId:0;
	keyIter = keys.begin();

	for (int side = 0; keyIter != keyEnd && side < SIDE_NUM; keyIter++, side++)
	{
		StatValueUtil::StatUpdateMap::iterator updateIter = statUpdateMap->find(*keyIter);
		StatValueUtil::StatUpdateMap::iterator updateEnd = statUpdateMap->end();
		if (updateIter == updateEnd)
		{
			ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find key");
			return;
		}
		StatValueUtil::StatUpdate *statUpdate = &updateIter->second;
		StatValueUtil::StatValueMap *statValueMap = &statUpdate->stats;

		TRACE_LOG("keyIter->entityId = "<<keyIter->entityId<<" ");
		StatValueUtil::StatValueMap::const_iterator svmIter = statValueMap->begin();
		StatValueUtil::StatValueMap::const_iterator svmEnd = statValueMap->end();
		for (int i = 0; svmIter != svmEnd; i++, svmIter++)
		{
			TRACE_LOG("statValueMap["<<i<<"] = "<< svmIter->first <<", "<< svmIter->second.iBefore[Stats::STAT_PERIOD_ALL_TIME] <<" ");
		}

		const char *statNames[] = {
			STATNAME_WINS, 
			STATNAME_TIES, 
			STATNAME_LOSSES, 
			STATNAME_FOULS, 
			STATNAME_FOULS_AGAINST, 
			STATNAME_SHOTS_ON_GOAL, 
			STATNAME_SHOTS_AGAINST_ON_GOAL,
			STATNAME_POSSESSION};
		
		const char *statSeasonNames[] = {
			STATNAME_WINS_SEASON, 
			STATNAME_TIES_SEASON, 
			STATNAME_LOSSES_SEASON, 
			STATNAME_FOULS_SEASON, 
			STATNAME_FOULS_AGAINST_SEASON, 
			STATNAME_SHOTS_ON_GOAL_SEASON, 
			STATNAME_SHOTS_AGAINST_ON_GOAL_SEASON,
			STATNAME_POSSESSION_SEASON};

		Collections::AttributeMap attributeMap;
		mFriendliesExtension->generateAttributeMap(keyIter->entityId, attributeMap);

		Collections::AttributeMap oppAttributeMap;
		mFriendliesExtension->generateAttributeMap((keyIter->entityId == eId1)? eId2:eId1, oppAttributeMap);

		StatValueUtil::StatValue *statSeasonValues[sizeof(statSeasonNames) / sizeof(statSeasonNames[0])] = {NULL};
		for (size_t i = 0; i < sizeof(statSeasonNames) / sizeof(statSeasonNames[0]); i++)
		{
			StatValueUtil::StatValueMap::iterator iter = statValueMap->find(statSeasonNames[i]);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statSeasonNames[i]<<" ");
				return;
			}
			statSeasonValues[i] = &iter->second; 
		}
		StatValueUtil::StatValue *statOverallValues[sizeof(statSeasonNames) / sizeof(statSeasonNames[0])] = {NULL};
		for (size_t i = 0; i < sizeof(statNames) / sizeof(statNames[0]); i++)
		{
			StatValueUtil::StatValueMap::iterator iter = statValueMap->find(statNames[i]);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statNames[i]<<" ");
				return;
			}
			statOverallValues[i] = &iter->second; 
		}
		for (int32_t period = 0; period < Stats::STAT_NUM_PERIODS; period++)
		{
			numGames[side][period] = 1;
			StatValueUtil::StatValueMap::iterator iter = statValueMap->find(STATNAME_TITLE);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<STATNAME_TITLE<<" ");
			}
			else
			{
				StatValueUtil::StatValue *title = &iter->second;
				if (title->iBefore[period] == TITLE_NONE || title->iBefore[period] == TITLE_HELD)
				{
					// The 3 below referernce the first 3 stats in statNames array above. They are the win, tie & loss. Sum of them is the number of games played
					for (size_t i = 0; i < 3; i++)
					{
						numGames[side][period] += statSeasonValues[i]->iBefore[period];
					}
				}
				else
				{
					if (title->iBefore[period] == TITLE_WIN)
					{
						title->iAfter[period] = TITLE_HELD;
					}
					else
					{
						title->iAfter[period] = TITLE_NONE;
					}
				}
			}
			goals[side][period] = 0;
			for (int i = 1; i < numGames[side][period]; i++)
			{
				char statName[32];
				blaze_snzprintf(statName, sizeof(statName), "%s%d", STATNAME_GOALS, i);
				iter = statValueMap->find(statName);
				if (iter == statValueMap->end())
				{
					ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statName<<" ");
				}
				else
				{
					StatValueUtil::StatValue *goal = &iter->second;
					goals[side][period] += goal->iBefore[period];
				}
			}
			char statName[32];
			blaze_snzprintf(statName, sizeof(statName), "%s%" PRId64, STATNAME_GOALS, numGames[side][period]);
			iter = statValueMap->find(statName);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statName<<" ");
			}
			else
			{
				StatValueUtil::StatValue *goal = &iter->second;
				goal->iAfter[period] = StatValueUtil::getInt(&attributeMap, STATNAME_SCORE);
				goals[side][period] += goal->iAfter[period];
			}

			goalsAgainst[side][period] = 0;
			for (int i = 1; i < numGames[side][period]; i++)
			{
				char gaStatName[32];
				blaze_snzprintf(gaStatName, sizeof(gaStatName), "%s%d", STATNAME_GOALS_AGAINST, i);
				iter = statValueMap->find(gaStatName);
				if (iter == statValueMap->end())
				{
					ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<< gaStatName <<" ");
				}
				else
				{
					StatValueUtil::StatValue *goalAgainst = &iter->second;
					goalsAgainst[side][period] += goalAgainst->iBefore[period];
				}
			}
			blaze_snzprintf(statName, sizeof(statName), "%s%" PRId64, STATNAME_GOALS_AGAINST, numGames[side][period]);
			iter = statValueMap->find(statName);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statName<<" ");
			}
			else
			{
				StatValueUtil::StatValue *goalAgainst = &iter->second;
				goalAgainst->iAfter[period] = StatValueUtil::getInt(&attributeMap, STATNAME_GOALS_AGAINST);
				goalsAgainst[side][period] += goalAgainst->iAfter[period];
			}

			points[side][period] = StatValueUtil::getInt(&attributeMap, STATNAME_POINTS);
			if (numGames[side][period] > 1)
			{
				blaze_snzprintf(statName, sizeof(statName), "%s%" PRId64, STATNAME_POINTS, numGames[side][period] - 1);
				iter = statValueMap->find(statName);
				if (iter == statValueMap->end())
				{
					ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statName<<" ");
				}
				else
				{
					StatValueUtil::StatValue *point = &iter->second;
					points[side][period] += point->iBefore[period];
				}
				for (size_t i = 0; i < sizeof (statSeasonValues) / sizeof(statSeasonValues[0]); i++)
				{
					statSeasonValues[i]->iAfter[period] = statSeasonValues[i]->iBefore[period];
				}
			}
			else
			{
				for (size_t i = 0; i < sizeof (statSeasonValues) / sizeof(statSeasonValues[0]); i++)
				{
					statSeasonValues[i]->iAfter[period] = 0;
				}
			}
			blaze_snzprintf(statName, sizeof(statName), "%s%" PRId64, STATNAME_POINTS, numGames[side][period]);
			iter = statValueMap->find(statName);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statName<<" ");
			}
			else
			{
				StatValueUtil::StatValue *point = &iter->second;
				point->iAfter[period] = points[side][period];
			}
			for (size_t i = 0; i < sizeof (statSeasonValues) / sizeof(statSeasonValues[0]); i++)
			{
				if (strcmp(statNames[i], STATNAME_FOULS) == 0 )
				{
					statSeasonValues[i]->iAfter[period] += StatValueUtil::getInt(&attributeMap, STATNAME_FOULS);
					statOverallValues[i]->iAfter[period] += StatValueUtil::getInt(&attributeMap, STATNAME_FOULS);
				}
				else if (strcmp(statNames[i], STATNAME_FOULS_AGAINST) == 0 )
				{
					statSeasonValues[i]->iAfter[period] += StatValueUtil::getInt(&oppAttributeMap, STATNAME_FOULS);
					statOverallValues[i]->iAfter[period] += StatValueUtil::getInt(&oppAttributeMap, STATNAME_FOULS);
				}
				else if (strcmp(statNames[i], STATNAME_POSSESSION) == 0 )
				{
					int64_t possNumGames = statSeasonValues[0]->iBefore[period] + statSeasonValues[1]->iBefore[period]+ statSeasonValues[2]->iBefore[period];
					int newPossession = StatValueUtil::getInt(&attributeMap, STATNAME_POSSESSION);
					int64_t seasonValue = statSeasonValues[i]->iAfter[period];
					int64_t overallValue = statOverallValues[i]->iAfter[period];

					statSeasonValues[i]->iAfter[period] = update_possession(seasonValue,newPossession, possNumGames,side);
					statOverallValues[i]->iAfter[period] = update_possession(overallValue,newPossession, possNumGames,side);
				}
				else
				{
					statSeasonValues[i]->iAfter[period] += StatValueUtil::getInt(&attributeMap, statNames[i]);
					statOverallValues[i]->iAfter[period] += StatValueUtil::getInt(&attributeMap, statNames[i]);
				}
			}
			blaze_snzprintf(statName, sizeof(statName), "%s%" PRId64, STATNAME_TEAM, numGames[side][period]);
			iter = statValueMap->find(statName);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statName<<" ");
			}
			else
			{
				StatValueUtil::StatValue* pTeamId = &iter->second;
				pTeamId->iAfter[period] = StatValueUtil::getInt(&attributeMap, ATTRIBNAME_TEAM);
			}

			blaze_snzprintf(statName, sizeof(statName), "%s%" PRId64, STATNAME_OPPTEAM, numGames[side][period]);
			iter = statValueMap->find(statName);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<statName<<" ");
			}
			else
			{
				StatValueUtil::StatValue* pOppTeamId = &iter->second;
				pOppTeamId->iAfter[period] = StatValueUtil::getInt(&oppAttributeMap, ATTRIBNAME_TEAM);
			}

			//update time
			iter = statValueMap->find(STATNAME_LASTMATCH);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<< STATNAME_LASTMATCH <<" ");
			}
			else
			{
				time_t now = time(NULL);
				StatValueUtil::StatValue *lastmatch = &iter->second;
				lastmatch->iAfter[period] = now;
			}

			//update goals for and goal against
			iter = statValueMap->find(STATNAME_GOALS);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<< STATNAME_GOALS <<" ");
			}
			else
			{
				StatValueUtil::StatValue *goalsFor = &iter->second;
				goalsFor->iAfter[period] += StatValueUtil::getInt(&attributeMap, STATNAME_GOALS);
			}

			iter = statValueMap->find(STATNAME_GOALS_AGAINST);
			if (iter == statValueMap->end())
			{
				ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<< STATNAME_GOALS_AGAINST <<" ");
			}
			else
			{
				StatValueUtil::StatValue *goalsAgainstStat = &iter->second;
                goalsAgainstStat->iAfter[period] += StatValueUtil::getInt(&attributeMap, STATNAME_GOALS_AGAINST);
			}

		}
	}
	keyIter = keys.begin();
	for (int side = 0; keyIter != keyEnd && side < SIDE_NUM; keyIter++, side++)
	{
		StatValueUtil::StatUpdateMap::iterator updateIter = statUpdateMap->find(*keyIter);
		StatValueUtil::StatUpdateMap::iterator updateEnd = statUpdateMap->end();
		if (updateIter == updateEnd)
		{
			ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find key");
			return;
		}
		StatValueUtil::StatUpdate *statUpdate = &updateIter->second;
		StatValueUtil::StatValueMap *statValueMap = &statUpdate->stats;
		for (int32_t period = 0; period < Stats::STAT_NUM_PERIODS; period++)
		{
			bool isSeasonEnd = false;
			if (numGames[side][period] == FP_MAX_NUM_GAMES_IN_SEASON)
			{
				isSeasonEnd = true;
			}
			else
			{
				const int MAX_POINTS_GAME_FRIENDLY = 3;
				int64_t pointsLeft = MAX_POINTS_GAME_FRIENDLY * (FP_MAX_NUM_GAMES_IN_SEASON - numGames[side][period]);
				if (abs(static_cast<int32_t>(points[SIDE_HOME][period] - points[SIDE_AWAY][period])) > pointsLeft)
				{
					isSeasonEnd = true;
				}
			}
			if (isSeasonEnd)
			{
				int sideWin = SIDE_NUM;
				if (points[SIDE_HOME][period] == points[SIDE_AWAY][period])
				{
					if (goals[SIDE_HOME][period] != goals[SIDE_AWAY][period])
					{
						sideWin = goals[SIDE_HOME][period] > goals[SIDE_AWAY][period] ? SIDE_HOME : SIDE_AWAY;
					}
				}
				else
				{
					sideWin = points[SIDE_HOME][period] > points[SIDE_AWAY][period] ? SIDE_HOME : SIDE_AWAY;
				}
				int titleCode = TITLE_LOSS;
				if (sideWin == side)
				{
					titleCode = TITLE_WIN;
					StatValueUtil::StatValueMap::iterator iter = statValueMap->find(STATNAME_TITLES);
					if (iter == statValueMap->end())
					{
						ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<STATNAME_TITLES<<" ");
					}
					else
					{
						StatValueUtil::StatValue *titles = &iter->second;
						titles->iAfter[period] = titles->iBefore[period] + 1;
					}
				}
				else if (sideWin == SIDE_NUM)
				{
					titleCode = TITLE_DRAW;
				}
				StatValueUtil::StatValueMap::iterator iter = statValueMap->find(STATNAME_TITLE);
				if (iter == statValueMap->end())
				{
					ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<STATNAME_TITLE<<" ");
				}
				else
				{
					StatValueUtil::StatValue *title = &iter->second;
					title->iAfter[period] = titleCode;
				}
				// update current season
				iter = statValueMap->find(STATNAME_CURR_SEASON);
				if (iter == statValueMap->end())
				{
					ERR_LOG("[ReportProcessor].updateFriendlySeasonStats(): Cannot find stat "<<STATNAME_CURR_SEASON<<" ");
				}
				else
				{
					StatValueUtil::StatValue *season = &iter->second;
					season->iAfter[period] = season->iBefore[period] + 1;
				}
			}
		}
	}
}

} //namespace GameReporting
} //namespace Blaze
