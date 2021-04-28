/*************************************************************************************************/
/*!
    \file   fifa_seasonalplay.cpp
    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "stats_updater.h"

#include "stats/transformstats.h"
#include "stats/updatestatsrequestbuilder.h"

#include "timeutils.h"

#include "util/rpc/utilslave.h"


namespace Blaze
{
namespace UserActivityTracker
{

static const char* STATS_CATEGORY			= "UserActivityTracker_StatCategory";
static const char* STATS_TIME				= "timePlayed";
static const char* STATS_GAME				= "gamesPlayed";
static const char* STATS_ROLLOVERNUMBER		= "rollOverNum";
static const char* STATS_ROLLOVERCOUNTER	= "rollOverCount";

static const int32_t PERIOD_TYPE				= Stats::STAT_PERIOD_ALL_TIME;
static const int32_t ROLLOVER_PERIOD			= Stats::STAT_PERIOD_DAILY;
static const bool	 ROLLOVER_HOURLY			= true;

static const char* ACTIVITY_TYPE_KEYSCOPE_NAME			= "activitytrackertype";
static const char* ACTIVITY_MODE_TYPE_KEYSCOPE_NAME		= "activitymodetype";

const uint16_t StatsUpdater::EXTENDED_DATA_TRACKSTATS = 1;

StatsUpdater::StatsUpdater(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, const UserActivityTrackerConfig* config) :
		mBuilder(builder)
		, mUpdateStatsHelper(updateStatsHelper)
{
	configureStatsUpdater(config);
}

bool StatsUpdater::isUserOptedIn(BlazeId id)
{
	bool foundExtendedData = false;
	bool result = false;
	UserSessionExtendedData userSessionExtendedData;

	UserInfoData userInfo;
	userInfo.setId(id);
	if (gUserSessionManager->requestUserExtendedData(userInfo, userSessionExtendedData) == ERR_OK)
	{
		UserExtendedDataValue value;
		if (UserSession::getDataValue(userSessionExtendedData.getDataMap(), Blaze::UserActivityTracker::UserActivityTrackerSlaveImpl::COMPONENT_ID, StatsUpdater::EXTENDED_DATA_TRACKSTATS, value))
		{
			TRACE_LOG("[StatsUpdater].isUserOptedIn - got user " << id << " extended value " << value << " ");
			result = (value == 1);
			foundExtendedData = true;
		}
		else
		{
			TRACE_LOG("[StatsUpdater].isUserOptedIn unable to fetch extended data for user" << id << " ");
		}
	}
	else
	{
		TRACE_LOG("[StatsUpdater].isUserOptedIn unable to fetch extended data for user" << id << " ");
	}

	if (!foundExtendedData)
	{
		//try to look up the optin from user settings
		Blaze::Util::UtilSlave* utilSlave = static_cast<Blaze::Util::UtilSlave*>(gController->getComponent(Blaze::Util::UtilSlave::COMPONENT_ID, false));
		if (utilSlave != nullptr)
		{
			Blaze::Util::UserSettingsLoadRequest userSettingsLoadRequest;
			userSettingsLoadRequest.setBlazeId(id);
			userSettingsLoadRequest.setKey("UAT_SETTING");

			Blaze::Util::UserSettingsResponse userSettingsResponse;
			Blaze::BlazeRpcError error = utilSlave->userSettingsLoad(userSettingsLoadRequest, userSettingsResponse);
			if (error == Blaze::ERR_OK)
			{
				const char* optinData = userSettingsResponse.getData();
				if (optinData != nullptr)
				{
					int32_t optinValue = EA::StdC::AtoI32(optinData);
					TRACE_LOG("[StatsUpdater].isUserOptedIn - got user " << id << " user setting value " << optinValue << " ");
					result = (optinValue == 1);
				}
			}
		}
	}

	return result;
}


void StatsUpdater::configureStatsUpdater(const UserActivityTrackerConfig* config)
{
	if (config != nullptr)
	{
		mStatsCategory = config->getStatsCategory();

		mStatNames[STATNAME_TIMEPLAYED] = config->getStatsTime();
		mStatNames[STATNAME_GAMESPLAYED] = config->getStatsGames();
		mStatNames[STATNNAME_ROLLOVER_NUMBER] = config->getRollOverNumber();
		mStatNames[STATSNAME_ROLLOVER_COUNTER] = config->getRollOverCounter();

		mStatPeriodType = static_cast<Stats::StatPeriodType>(config->getPeriodType());
		mRollOverPeriod = static_cast<Stats::StatPeriodType>(config->getRollOverPeriod());
		mRollOverHourly = config->getRollOverHourly();

		mTrackerTypeKeyscope = config->getActivityTypeKeyscopeName();
		mModeTypeKeyscope = config->getActivityModeKeyscopeName();
	}
	else
	{
		mStatsCategory = STATS_CATEGORY;

		mStatNames[STATNAME_TIMEPLAYED] = STATS_TIME;
		mStatNames[STATNAME_GAMESPLAYED] = STATS_GAME;
		mStatNames[STATNNAME_ROLLOVER_NUMBER] = STATS_ROLLOVERNUMBER;
		mStatNames[STATSNAME_ROLLOVER_COUNTER] = STATS_ROLLOVERCOUNTER;

		mStatPeriodType = static_cast<Stats::StatPeriodType>(PERIOD_TYPE);
		mRollOverPeriod = static_cast<Stats::StatPeriodType>(ROLLOVER_PERIOD);
		mRollOverHourly = ROLLOVER_HOURLY;
 
		mTrackerTypeKeyscope = ACTIVITY_TYPE_KEYSCOPE_NAME;
		mModeTypeKeyscope = ACTIVITY_MODE_TYPE_KEYSCOPE_NAME;
	}
}

Stats::UpdateRowKey* StatsUpdater::getUpgradeRowKey(int64_t entityId, const char* statCategory, ActivityTrackerTypeKeyScope trackerType, ActivityModeTypeKeyScope modeType)
{
	Stats::ScopeNameValueMap keyscopes;
	keyscopes.insert(eastl::make_pair(mTrackerTypeKeyscope.c_str(), trackerType));
	keyscopes.insert(eastl::make_pair(mModeTypeKeyscope.c_str(), modeType));

	Stats::UpdateRowKey* key = const_cast<Stats::UpdateRowKey*>(mBuilder->getUpdateRowKey(statCategory, entityId, &keyscopes));
	if (key != NULL)
	{
		key->period = mStatPeriodType;
		TRACE_LOG("[StatsUpdater].getUpgradeRowKey() setting period for key: " << mStatPeriodType << " ");
	}

	return key;
}

void StatsUpdater::addUserUpdateInfo(int64_t targetUser, ActivityModeTypeKeyScope targetModeType, SubmitUserActivityRequest& targetRequest)
{
	UpdateInfo info(targetUser, targetModeType, targetRequest);
	mUpdateInfoMap.insert(eastl::make_pair(targetUser, info));
}

void StatsUpdater::updateGamesPlayed(GameManager::PlayerIdList& playerIdList)
{
	const EA::TDF::TimeValue currentTime = TimeValue::getTimeOfDay();
	UserActivityTracker::SubmitUserActivityRequest submitUserActivityRequest;
	submitUserActivityRequest.setTimeStamp(currentTime.getSec());

	UserActivityTracker::UserActivity* userActivity = submitUserActivityRequest.getUserActivityList().allocate_element();
	userActivity->setActivityPeriodType(UserActivityTracker::TYPE_CURRENT);
	userActivity->setNumMatches(1);

	submitUserActivityRequest.getUserActivityList().push_back(userActivity);

	GameManager::PlayerIdList::iterator iter, end;
	iter = playerIdList.begin();
	end = playerIdList.end();
	for (; iter != end; iter++)
	{
		TRACE_LOG("[StatsUpdater].updateGamesPlayed() for " << *iter << " ");

		if (isUserOptedIn(*iter))
		{
			addUserUpdateInfo(*iter, UserActivityTracker::ACTIVITY_MODE_TYPE_ONLINE, submitUserActivityRequest);
		}
	}
}

void StatsUpdater::selectStats()
{
	TRACE_LOG("[StatsUpdater].selectStats() for " << mStatsCategory.c_str() << " ");
	UpdateInfoMap::iterator iter, end;
	iter = mUpdateInfoMap.begin();
	end = mUpdateInfoMap.end();

	for (; iter != end; iter++)
	{
		//select stats for user activity tracker
		int64_t targetId = iter->second.mTargetEntityId;
		TRACE_LOG("[StatsUpdater].selectStats() generating keys for entity: " << targetId << " ");
		for (int i = ACTIVITY_TRACKER_TYPE_CURRENT; i < ACTIVITY_TRACKER_MAX; i++)
		{
			for (int j = ACTIVITY_MODE_TYPE_ONLINE; j < ACTIVITY_MODE_TYPE_MAX; j++)
			{
				Stats::ScopeNameValueMap keyscopes;
				keyscopes.insert(eastl::make_pair(mTrackerTypeKeyscope.c_str(), i));
				keyscopes.insert(eastl::make_pair(mModeTypeKeyscope.c_str(), j));
				mBuilder->startStatRow(mStatsCategory.c_str(), targetId, &keyscopes);	

	
				for (int statIndex = 0; statIndex < STATNAME_MAX; statIndex++)
				{
					mBuilder->selectStat(mStatNames[statIndex].c_str());
				}

				mBuilder->completeStatRow();
			}
		}
	}
}

void StatsUpdater::transformStats()
{
	UpdateInfoMap::iterator iter, end;
	iter = mUpdateInfoMap.begin();
	end = mUpdateInfoMap.end();

	for (; iter != end; iter++)
	{
		TRACE_LOG("[StatsUpdater].transformStats() generating keys for entity: " << iter->second.mTargetEntityId << " ");
		for (int i = ACTIVITY_TRACKER_TYPE_CURRENT; i < ACTIVITY_TRACKER_MAX; i++)
		{
			for (int j = ACTIVITY_MODE_TYPE_ONLINE; j < ACTIVITY_MODE_TYPE_MAX; j++)
			{
				iter->second.mUpdateRowKeyList[i][j].mUpdateRowKey = getUpgradeRowKey(iter->second.mTargetEntityId, mStatsCategory.c_str(), static_cast<ActivityTrackerTypeKeyScope>(i), static_cast<ActivityModeTypeKeyScope>(j));
			}
		}

		GameReporting::StatValueUtil::StatUpdateMap statUpdateMap;
		readUserActivityStats(statUpdateMap, iter->second);

		updateUserActivityStats(statUpdateMap, iter->second);

		writeUserActivityStats(statUpdateMap);
	}
}

void StatsUpdater::updateUserActivityStats(GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap, UpdateInfo& updateInfo)
{
	int64_t rollOverIndex = 0;
	int64_t prevRollOverIndex = 0;
	
	if (determineRollOverIndex(rollOverIndex, prevRollOverIndex, updateInfo))
	{
		//get the StatValueMap for each UpdateRowKey
		for (int i = ACTIVITY_TRACKER_TYPE_CURRENT; i < ACTIVITY_TRACKER_MAX; i++)
		{
			for (int j = ACTIVITY_MODE_TYPE_ONLINE; j < ACTIVITY_MODE_TYPE_MAX; j++)
			{
				updateInfo.mUpdateRowKeyList[i][j].mStatValueMap = getStatValueMap(*updateInfo.mUpdateRowKeyList[i][j].mUpdateRowKey, statUpdateMap);
			}
		}

		doRollOver(rollOverIndex, prevRollOverIndex, statUpdateMap, updateInfo);
		updateStats(rollOverIndex, prevRollOverIndex, statUpdateMap, updateInfo);
	}
	else
	{
		TRACE_LOG("[StatsUpdater].updateUserActivityStats() time period invalid! skipping update");
	}
}

GameReporting::StatValueUtil::StatValueMap* StatsUpdater::getStatValueMap(Stats::UpdateRowKey& updateRowKey, GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap)
{
	GameReporting::StatValueUtil::StatValueMap* statValueMap = nullptr;

	GameReporting::StatValueUtil::StatUpdateMap::iterator updateIter, updateEnd;
	updateEnd = statUpdateMap.end();

	updateIter = statUpdateMap.find(updateRowKey);
	if (updateIter != updateEnd)
	{
		GameReporting::StatValueUtil::StatUpdate* statUpdate = &(updateIter->second);
		statValueMap = &(statUpdate->stats);
	}

	const char* errorMessage = statValueMap == nullptr ? "NOT retrieved for" : "retrieved for";
	TRACE_LOG("[StatsUpdater].getStatValueMap() - " << errorMessage <<
		" Entity id:" << updateRowKey.entityId <<
		" Category:" << updateRowKey.category <<
		" Period:" << updateRowKey.period <<
		" KeyScope (Activity Type):" << getKeyScopeValue(mTrackerTypeKeyscope, *updateRowKey.scopeNameValueMap,  0) <<
		" KeyScope (Mode Type):" << getKeyScopeValue(mModeTypeKeyscope, *updateRowKey.scopeNameValueMap, 0) <<
		" ");

	return statValueMap;
}

int64_t StatsUpdater::getKeyScopeValue(eastl::string& keyScopeName, const Stats::ScopeNameValueMap& keyScopeMap, int64_t defaultValue)
{
	int64_t result = defaultValue;
	Stats::ScopeNameValueMap::const_iterator iterKeyScope, endKeyScope;
	iterKeyScope = keyScopeMap.find(keyScopeName.c_str());
	endKeyScope = keyScopeMap.end();
	if (iterKeyScope != endKeyScope)
	{
		result = iterKeyScope->second;
	}

	return result;
}

bool StatsUpdater::determineRollOverIndex(int64_t& rollOverIndex, int64_t& prevRollOverIndex, UpdateInfo& updateInfo)
{
	bool isValidTimePeriod = false;

	TimeUtils timeUtils;
	int64_t rollOverTime = timeUtils.getRollOverTime(mRollOverPeriod);
	int64_t previousRollOverTime = timeUtils.getPreviousRollOverTime(mRollOverPeriod);

	int64_t requestTime = updateInfo.mTargetRequest.getTimeStamp();

	if (mRollOverHourly)
	{
		//override the time to current hour
		const EA::TDF::TimeValue timeOfDay = TimeValue::getTimeOfDay();

		struct tm currentTime;
		TimeValue::gmTime(&currentTime, timeOfDay.getSec());
		currentTime.tm_min = 0;
		currentTime.tm_sec = 0;

		previousRollOverTime = TimeValue::mkgmTime(&currentTime, true);
		rollOverTime = previousRollOverTime + SECS_PER_HOUR;
	}

	if (requestTime > previousRollOverTime && requestTime <= rollOverTime)
	{
		isValidTimePeriod = true;
	}

	if (isValidTimePeriod)
	{
		struct tm tmRollOVerTime;
		TimeValue::gmTime(&tmRollOVerTime, rollOverTime);

		struct tm tmPrevRollOverTime;
		TimeValue::gmTime(&tmPrevRollOverTime, previousRollOverTime);

		if (mRollOverPeriod == Stats::STAT_PERIOD_WEEKLY)
		{
			tm tmCurrentRequesTime;
			TimeValue::gmTime(&tmCurrentRequesTime, requestTime);

			TimeValue startOfYear(tmCurrentRequesTime.tm_year + 1900, 1, 1, 12, 0, 0);
			tm tmStartOfYear;
			TimeValue::gmTime(&tmStartOfYear, startOfYear.getSec());
			int32_t startingDay = tmStartOfYear.tm_wday;
			Stats::PeriodIds pPeriodIds;
			timeUtils.getStatsPeriodIds(pPeriodIds);

			int32_t rollOverDay = pPeriodIds.getWeeklyDay();

			if (rollOverDay < startingDay)
			{
				rollOverDay += 7;
			}
			int32_t offset = rollOverDay - startingDay;

			rollOverIndex = ((tmRollOVerTime.tm_year + 1900) * 1000000) + (1 + ((tmRollOVerTime.tm_yday - 1 - offset) / 7));
			prevRollOverIndex = ((tmPrevRollOverTime.tm_year + 1900) * 1000000) + (1 + ((tmPrevRollOverTime.tm_yday - 1 - offset) / 7));

			TRACE_LOG("[StatsUpdater].determineRollOverIndex() weekly set an index of current rollover " << rollOverIndex << " prev rollover" << prevRollOverIndex << " offset " << offset << " ");
		}
		else if (mRollOverPeriod == Stats::STAT_PERIOD_MONTHLY)
		{
			rollOverIndex = ((tmRollOVerTime.tm_year + 1900) * 1000000) + tmRollOVerTime.tm_mon;
			prevRollOverIndex = ((tmPrevRollOverTime.tm_year + 1900) * 1000000) + tmPrevRollOverTime.tm_mon;

			TRACE_LOG("[StatsUpdater].determineRollOverIndex() monthly set an index of current rollover " << rollOverIndex << " prev rollover" << prevRollOverIndex << " ");
		}
		else
		{
			if (mRollOverHourly)
			{
				rollOverIndex = ((tmRollOVerTime.tm_year + 1900) * 1000000) + (tmRollOVerTime.tm_yday * 100) + tmRollOVerTime.tm_hour;
				prevRollOverIndex = ((tmPrevRollOverTime.tm_year + 1900) * 1000000) + (tmPrevRollOverTime.tm_yday * 100) + tmPrevRollOverTime.tm_hour;

				TRACE_LOG("[StatsUpdater].determineRollOverIndex() hourly set an index of current rollover " << rollOverIndex << " prev rollover" << prevRollOverIndex << " ");
			}
			else
			{
				rollOverIndex = ((tmRollOVerTime.tm_year + 1900) * 1000000) + tmRollOVerTime.tm_yday;
				prevRollOverIndex = ((tmPrevRollOverTime.tm_year + 1900) * 1000000) + tmPrevRollOverTime.tm_yday;

				TRACE_LOG("[StatsUpdater].determineRollOverIndex() daily set an index of current rollover " << rollOverIndex << " prev rollover" << prevRollOverIndex << " ");
			}
		}
	}

	return isValidTimePeriod;
}

void StatsUpdater::doRollOver(int64_t rollOverIndex, int64_t prevRollOverIndex, GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap, UpdateInfo& updateInfo)
{
	//check if currently on this rolloverindex
	GameReporting::StatValueUtil::StatValue * current_Online_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_CURRENT][ACTIVITY_MODE_TYPE_ONLINE].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * current_Offline_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_CURRENT][ACTIVITY_MODE_TYPE_OFFLINE].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	if (rollOverIndex != current_Online_RollOverNumber->iBefore[mStatPeriodType] && rollOverIndex != current_Offline_RollOverNumber->iBefore[mStatPeriodType] &&
		!(current_Online_RollOverNumber->iBefore[mStatPeriodType] == 0 && current_Offline_RollOverNumber->iBefore[mStatPeriodType] == 0))
	{
		//rollOverIndex doesn't match current online or offline, rollover!
		TRACE_LOG("[StatsUpdater].doRollOver()");

		//pull the prev week and prevWeek2 data into accum
		for (int i = ACTIVITY_MODE_TYPE_ONLINE; i < ACTIVITY_MODE_TYPE_MAX; i++)
		{
			updateAccumulator(static_cast<ActivityModeTypeKeyScope>(i), updateInfo);
		}

		//clear the prev week and prevweek2 data
		clearStatsValueMap(ACTIVITY_TRACKER_TYPE_PREV_WEEK, updateInfo);
		clearStatsValueMap(ACTIVITY_TRACKER_TYPE_PREV_WEEK_2, updateInfo);

		//transfer statsValue map from current to prev week or prev week 2
		ActivityTrackerTypeKeyScope target = ACTIVITY_TRACKER_TYPE_PREV_WEEK;
		if (prevRollOverIndex != current_Online_RollOverNumber->iBefore[mStatPeriodType] && prevRollOverIndex != current_Offline_RollOverNumber->iBefore[mStatPeriodType])
		{
			target = ACTIVITY_TRACKER_TYPE_PREV_WEEK_2;
		}
		transferStatsValueMap(ACTIVITY_TRACKER_TYPE_CURRENT, target, updateInfo);

		//clear the current
		clearStatsValueMap(ACTIVITY_TRACKER_TYPE_CURRENT, updateInfo);
	}
}

void StatsUpdater::updateStats(int64_t rollOverIndex, int64_t prevRollOverIndex, GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap, UpdateInfo& updateInfo)
{
	const UserActivityList userActivityList = updateInfo.mTargetRequest.getUserActivityList();
	UserActivityList::const_iterator iter, end;
	iter = userActivityList.begin();
	end = userActivityList.end();
	for (; iter != end; iter++)
	{
		processActivity(rollOverIndex, prevRollOverIndex, *iter->get(), updateInfo);
	}

	for (int i = ACTIVITY_MODE_TYPE_ONLINE; i < ACTIVITY_MODE_TYPE_MAX; i++)
	{
		updateTotals(static_cast< ActivityModeTypeKeyScope>(i), updateInfo);
	}
}

void StatsUpdater::processActivity(int64_t rollOverIndex, int64_t prevRollOverIndex, const UserActivity& activity, UpdateInfo& updateInfo)
{
	int64_t updatingRollOverIndex = rollOverIndex;
	ActivityModeTypeKeyScope writeModeTypeKeyscope = ACTIVITY_MODE_TYPE_ONLINE;
	ActivityModeTypeKeyScope readModeTypeKeyScope = ACTIVITY_MODE_TYPE_OFFLINE;
	if (updateInfo.mTargetModeType == ACTIVITY_MODE_TYPE_OFFLINE)
	{
		writeModeTypeKeyscope = ACTIVITY_MODE_TYPE_OFFLINE;
		readModeTypeKeyScope = ACTIVITY_MODE_TYPE_ONLINE;
	}

	ActivityTrackerTypeKeyScope updateActivtyTypeKeyScope = ACTIVITY_TRACKER_TYPE_CURRENT;
	if (activity.getActivityPeriodType() == TYPE_PREVIOUS)
	{
		updateActivtyTypeKeyScope = ACTIVITY_TRACKER_TYPE_PREV_WEEK;
		updatingRollOverIndex = prevRollOverIndex;
	}

	GameReporting::StatValueUtil::StatValue * current_Write_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][writeModeTypeKeyscope].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * current_Write_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][writeModeTypeKeyscope].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * current_Write_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][writeModeTypeKeyscope].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * current_Write_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][writeModeTypeKeyscope].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	current_Write_TimePlayed->iAfter[mStatPeriodType] = current_Write_TimePlayed->iBefore[mStatPeriodType] + activity.getTimeDuration();
	current_Write_GamesPlayed->iAfter[mStatPeriodType] = current_Write_GamesPlayed->iBefore[mStatPeriodType] + activity.getNumMatches();

	current_Write_RollOverNumber->iAfter[mStatPeriodType] = updatingRollOverIndex;
	current_Write_RollOverCounter->iAfter[mStatPeriodType] = 1;

	//update total for current
	GameReporting::StatValueUtil::StatValue * current_Read_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][readModeTypeKeyScope].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * current_Read_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][readModeTypeKeyScope].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * current_Read_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][readModeTypeKeyScope].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * current_Read_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][readModeTypeKeyScope].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	current_Read_RollOverNumber->iAfter[mStatPeriodType] = current_Read_RollOverNumber->iBefore[mStatPeriodType];
	current_Read_RollOverCounter->iAfter[mStatPeriodType] = current_Read_RollOverCounter->iBefore[mStatPeriodType];

	GameReporting::StatValueUtil::StatValue * current_Aggregate_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][ACTIVITY_MODE_TYPE_AGGREGATE].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * current_Aggregate_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][ACTIVITY_MODE_TYPE_AGGREGATE].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * current_Aggregate_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][ACTIVITY_MODE_TYPE_AGGREGATE].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * current_Aggregate_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[updateActivtyTypeKeyScope][ACTIVITY_MODE_TYPE_AGGREGATE].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	current_Aggregate_TimePlayed->iAfter[mStatPeriodType] = current_Read_TimePlayed->iAfter[mStatPeriodType] + current_Write_TimePlayed->iAfter[mStatPeriodType];
	current_Aggregate_GamesPlayed->iAfter[mStatPeriodType] = current_Read_GamesPlayed->iAfter[mStatPeriodType] + current_Write_GamesPlayed->iAfter[mStatPeriodType];
	current_Aggregate_RollOverNumber->iAfter[mStatPeriodType] = updatingRollOverIndex;
	current_Aggregate_RollOverCounter->iAfter[mStatPeriodType] = 1;
}

void StatsUpdater::updateTotals(ActivityModeTypeKeyScope mode, UpdateInfo& updateInfo)
{
	GameReporting::StatValueUtil::StatValue * current_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_CURRENT][mode].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * current_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_CURRENT][mode].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * current_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_CURRENT][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * current_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_CURRENT][mode].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	GameReporting::StatValueUtil::StatValue * prevWeek_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK][mode].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * prevWeek_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK][mode].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
//	GameReporting::StatValueUtil::StatValue * prevWeek_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * prevWeek_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK][mode].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	GameReporting::StatValueUtil::StatValue * prevWeek2_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK_2][mode].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * prevWeek2_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK_2][mode].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
//	GameReporting::StatValueUtil::StatValue * prevWeek2_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK_2][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * prevWeek2_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK_2][mode].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	GameReporting::StatValueUtil::StatValue * accum_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_ACCUMULATED][mode].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * accum_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_ACCUMULATED][mode].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
//	GameReporting::StatValueUtil::StatValue * accum_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_ACCUMULATED][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * accum_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_ACCUMULATED][mode].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	GameReporting::StatValueUtil::StatValue * total_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_TOTAL][mode].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * total_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_TOTAL][mode].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * total_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_TOTAL][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * total_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_TOTAL][mode].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	total_TimePlayed->iAfter[mStatPeriodType] = current_TimePlayed->iAfter[mStatPeriodType] + prevWeek_TimePlayed->iAfter[mStatPeriodType] + prevWeek2_TimePlayed->iAfter[mStatPeriodType] + accum_TimePlayed->iAfter[mStatPeriodType];
	total_GamesPlayed->iAfter[mStatPeriodType] = current_GamesPlayed->iAfter[mStatPeriodType] + prevWeek_GamesPlayed->iAfter[mStatPeriodType] + prevWeek2_GamesPlayed->iAfter[mStatPeriodType] + accum_GamesPlayed->iAfter[mStatPeriodType];
	total_RollOverCounter->iAfter[mStatPeriodType] = current_RollOverCounter->iAfter[mStatPeriodType] + prevWeek_RollOverCounter->iAfter[mStatPeriodType] + prevWeek2_RollOverCounter->iAfter[mStatPeriodType] + accum_RollOverCounter->iAfter[mStatPeriodType];
	total_RollOverNumber->iAfter[mStatPeriodType] = current_RollOverNumber->iAfter[mStatPeriodType];
}

void StatsUpdater::updateAccumulator(ActivityModeTypeKeyScope mode, UpdateInfo& updateInfo)
{
	GameReporting::StatValueUtil::StatValue * current_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_CURRENT][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());

	GameReporting::StatValueUtil::StatValue * prevWeek_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK][mode].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * prevWeek_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK][mode].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
	//	GameReporting::StatValueUtil::StatValue * prevWeek_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * prevWeek_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK][mode].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	GameReporting::StatValueUtil::StatValue * prevWeek2_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK_2][mode].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * prevWeek2_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK_2][mode].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
	//	GameReporting::StatValueUtil::StatValue * prevWeek2_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK_2][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * prevWeek2_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_PREV_WEEK_2][mode].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	GameReporting::StatValueUtil::StatValue * accum_TimePlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_ACCUMULATED][mode].mStatValueMap, mStatNames[STATNAME_TIMEPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * accum_GamesPlayed = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_ACCUMULATED][mode].mStatValueMap, mStatNames[STATNAME_GAMESPLAYED].c_str());
	GameReporting::StatValueUtil::StatValue * accum_RollOverNumber = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_ACCUMULATED][mode].mStatValueMap, mStatNames[STATNNAME_ROLLOVER_NUMBER].c_str());
	GameReporting::StatValueUtil::StatValue * accum_RollOverCounter = GetStatValue(updateInfo.mUpdateRowKeyList[ACTIVITY_TRACKER_TYPE_ACCUMULATED][mode].mStatValueMap, mStatNames[STATSNAME_ROLLOVER_COUNTER].c_str());

	accum_TimePlayed->iAfter[mStatPeriodType] = accum_TimePlayed->iBefore[mStatPeriodType] + prevWeek_TimePlayed->iBefore[mStatPeriodType] + prevWeek2_TimePlayed->iBefore[mStatPeriodType];
	accum_GamesPlayed->iAfter[mStatPeriodType] = accum_GamesPlayed->iBefore[mStatPeriodType] + prevWeek_GamesPlayed->iBefore[mStatPeriodType] + prevWeek2_GamesPlayed->iBefore[mStatPeriodType];
	accum_RollOverCounter->iAfter[mStatPeriodType] = accum_RollOverCounter->iBefore[mStatPeriodType] + prevWeek_RollOverCounter->iBefore[mStatPeriodType] + prevWeek2_RollOverCounter->iBefore[mStatPeriodType];
	accum_RollOverNumber->iAfter[mStatPeriodType] = current_RollOverNumber->iBefore[mStatPeriodType];
}

void StatsUpdater::clearStatsValueMap(ActivityTrackerTypeKeyScope activityTrackerTypeKeyScope, UpdateInfo& updateInfo)
{
	for (int i = ACTIVITY_MODE_TYPE_ONLINE; i < ACTIVITY_MODE_TYPE_MAX; i++)
	{
		for (int j = STATNAME_TIMEPLAYED; j < STATNAME_MAX; j++)
		{
			GameReporting::StatValueUtil::StatValue* statValue = GetStatValue(updateInfo.mUpdateRowKeyList[activityTrackerTypeKeyScope][i].mStatValueMap, mStatNames[j].c_str());
			statValue->iBefore[mStatPeriodType] = 0;
			statValue->iAfter[mStatPeriodType] = 0;
		}
	}
}

void StatsUpdater::transferStatsValueMap(ActivityTrackerTypeKeyScope source, ActivityTrackerTypeKeyScope target, UpdateInfo& updateInfo)
{
	for (int i = ACTIVITY_MODE_TYPE_ONLINE; i < ACTIVITY_MODE_TYPE_MAX; i++)
	{
		for (int j = STATNAME_TIMEPLAYED; j < STATNAME_MAX; j++)
		{
			GameReporting::StatValueUtil::StatValue* sourceStatValue = GetStatValue(updateInfo.mUpdateRowKeyList[source][i].mStatValueMap, mStatNames[j].c_str());
			GameReporting::StatValueUtil::StatValue* targetStatsValue = GetStatValue(updateInfo.mUpdateRowKeyList[target][i].mStatValueMap, mStatNames[j].c_str());
			targetStatsValue->iBefore[mStatPeriodType] = sourceStatValue->iBefore[mStatPeriodType];
			targetStatsValue->iAfter[mStatPeriodType] = sourceStatValue->iAfter[mStatPeriodType];
		}
	}
}

void StatsUpdater::readUserActivityStats(GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap, UpdateInfo& updateInfo)
{
	for (int i = ACTIVITY_TRACKER_TYPE_CURRENT; i < ACTIVITY_TRACKER_MAX; i++)
	{
		for (int j = ACTIVITY_MODE_TYPE_ONLINE; j < ACTIVITY_MODE_TYPE_MAX; j++)
		{
			GameReporting::StatValueUtil::StatUpdate statUpdate;
			Stats::UpdateRowKey* key = updateInfo.mUpdateRowKeyList[i][j].mUpdateRowKey;
			Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key->period);

			for (int statNameIndex = 0; statNameIndex < STATNAME_MAX; statNameIndex++)
			{
				insertStat(statUpdate, mStatNames[statNameIndex].c_str(), periodType, mUpdateStatsHelper->getValueInt(key, mStatNames[statNameIndex].c_str(), periodType, true));
			}

			statUpdateMap.insert(eastl::make_pair(*key, statUpdate));
		}
	}
}

void StatsUpdater::writeUserActivityStats(GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap)
{
	GameReporting::StatValueUtil::StatUpdateMap::iterator iter = statUpdateMap.begin();
	GameReporting::StatValueUtil::StatUpdateMap::iterator end = statUpdateMap.end();

	for (; iter != end; iter++)
	{
		Stats::UpdateRowKey* key = &iter->first;
		
		const Stats::ScopeNameValueMap* keyscopeMap = key->scopeNameValueMap;
		Stats::ScopeNameValueMap::const_iterator iterKeyScope, endKeyScope;
		endKeyScope = keyscopeMap->end();

		int64_t activityType = 0;
		iterKeyScope = keyscopeMap->find(mTrackerTypeKeyscope.c_str());
		if (iterKeyScope != endKeyScope)
		{
			activityType = iterKeyScope->second;
		}

		int64_t modeType = 0;
		iterKeyScope = keyscopeMap->find(mTrackerTypeKeyscope.c_str());
		if (iterKeyScope != endKeyScope)
		{
			modeType = iterKeyScope->second;
		}

		GameReporting::StatValueUtil::StatUpdate* statUpdate = &iter->second;

		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key->period);

		GameReporting::StatValueUtil::StatValueMap::iterator statValueIter = statUpdate->stats.begin();
		GameReporting::StatValueUtil::StatValueMap::iterator statValueEnd = statUpdate->stats.end();
		
		for (; statValueIter != statValueEnd; statValueIter++)
		{
			GameReporting::StatValueUtil::StatValue* statValue = &statValueIter->second;
			mUpdateStatsHelper->setValueInt(key, statValueIter->first, periodType, statValue->iAfter[periodType]);
			
			TRACE_LOG("[StatsUpdater].writeUserActivityStats() - Entity id:" << key->entityId <<  
																		" Category:" << key->category << 
																		" Period:" << key->period << 
																		" Stat Name:" << statValueIter->first << 
																		" Value:" << statValue->iAfter[periodType] << 
																		" KeyScope (Activity Type):" << activityType <<
																		" KeyScope (Mode Type):" << modeType <<
																		" ");
		}
	}
}

} //namespace UserActivityTracker
} //namespace Blaze
