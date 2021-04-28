/*************************************************************************************************/
/*!
    \file   stats_updater.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_USERACTIVITSTRACKER_STATS_UPDATER_H
#define BLAZE_USERACTIVITSTRACKER_STATS_UPDATER_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifastatsvalueutil.h"

#include "EASTL/string.h"
#include "EASTL/vector.h"
#include "EASTL/map.h"
#include "EASTL/list.h"

#include "useractivitytracker/tdf/useractivitytrackertypes_server.h"
#include "stats/tdf/stats_server.h"
#include "gamemanager/tdf/gamemanager.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
//#define	USERACTIVITYTRACKER_LOG_SOLO_STATS


namespace Blaze
{

namespace Stats
{
	class UpdateStatsRequestBuilder;
	class UpdateStatsHelper;
	struct UpdateRowKey;
}

namespace UserActivityTracker
{

/*!
    class StatsUpdater
*/
class StatsUpdater
{
public:
	StatsUpdater(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, const UserActivityTrackerConfig* config = nullptr);
	~StatsUpdater() {}

	void selectStats();
	void transformStats();

	void addUserUpdateInfo(int64_t targetUser, ActivityModeTypeKeyScope targetModeType, SubmitUserActivityRequest& targetRequest);
	void updateGamesPlayed(GameManager::PlayerIdList& playerIdList);

	static const uint16_t EXTENDED_DATA_TRACKSTATS;

private:

	struct StatsData
	{
	public:
		StatsData()
		{
			mUpdateRowKey = nullptr;
			mStatValueMap = nullptr;
		}

		Stats::UpdateRowKey*						mUpdateRowKey;
		GameReporting::StatValueUtil::StatValueMap* mStatValueMap;
	};

	struct UpdateInfo
	{
	public:
		UpdateInfo(int64_t targetId, ActivityModeTypeKeyScope targetModeType, SubmitUserActivityRequest& targetRequest)
		{
			mTargetEntityId = targetId;
			mTargetModeType = targetModeType;
			mTargetRequest = targetRequest;
		}

		int64_t						mTargetEntityId;
		ActivityModeTypeKeyScope	mTargetModeType;
		SubmitUserActivityRequest	mTargetRequest;

		StatsData	mUpdateRowKeyList[ACTIVITY_TRACKER_MAX][ACTIVITY_MODE_TYPE_MAX];
	};

	bool isUserOptedIn(BlazeId id);

	void configureStatsUpdater(const UserActivityTrackerConfig* config);
	Stats::UpdateRowKey* getUpgradeRowKey(int64_t entityId, const char* statCategory, ActivityTrackerTypeKeyScope trackerType, ActivityModeTypeKeyScope modeType);

	void readUserActivityStats(GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap, UpdateInfo& updateInfo);
	void writeUserActivityStats(GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap);

	void updateUserActivityStats(GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap, UpdateInfo& updateInfo);
	bool determineRollOverIndex(int64_t& rollOverIndex, int64_t& prevRollOverIndex, UpdateInfo& updateInfo);
	void doRollOver(int64_t rollOverIndex, int64_t prevRollOverIndex, GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap, UpdateInfo& updateInfo);
	void updateStats(int64_t rollOverIndex, int64_t prevRollOverIndex, GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap, UpdateInfo& updateInfo);

	void processActivity(int64_t rollOverIndex, int64_t prevRollOverIndex, const UserActivity& activity, UpdateInfo& updateInfo);
	void updateTotals(ActivityModeTypeKeyScope mode, UpdateInfo& updateInfo);
	void updateAccumulator(ActivityModeTypeKeyScope mode, UpdateInfo& updateInfo);
	void clearStatsValueMap(ActivityTrackerTypeKeyScope activityTrackerTypeKeyScope, UpdateInfo& updateInfo);
	void transferStatsValueMap(ActivityTrackerTypeKeyScope source, ActivityTrackerTypeKeyScope target, UpdateInfo& updateInfo);

	GameReporting::StatValueUtil::StatValueMap* getStatValueMap(Stats::UpdateRowKey& updateRowKey, GameReporting::StatValueUtil::StatUpdateMap& statUpdateMap);
	int64_t getKeyScopeValue(eastl::string& keyScopeName, const Stats::ScopeNameValueMap& keyScopeMap, int64_t defaultValue);
		
	Stats::UpdateStatsRequestBuilder*	mBuilder;
    Stats::UpdateStatsHelper*           mUpdateStatsHelper;
	
	eastl::string						mStatsCategory;
	eastl::string						mTrackerTypeKeyscope;
	eastl::string						mModeTypeKeyscope;
	
	Stats::StatPeriodType				mStatPeriodType;
	Stats::StatPeriodType				mRollOverPeriod;
	bool								mRollOverHourly;

	enum StatName
	{
		STATNAME_TIMEPLAYED,
		STATNAME_GAMESPLAYED,
		STATNNAME_ROLLOVER_NUMBER,
		STATSNAME_ROLLOVER_COUNTER,
		STATNAME_MAX
	};
	eastl::string						mStatNames[STATNAME_MAX];

	typedef eastl::map<int64_t, UpdateInfo> UpdateInfoMap;
	UpdateInfoMap	mUpdateInfoMap;
};


}   // namespace UserActivityTracker
}   // namespace Blaze

#endif  // BLAZE_USERACTIVITSTRACKER_STATS_UPDATER_H

