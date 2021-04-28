/*************************************************************************************************/
/*!
    \file   fifa_vproobjectivestatsupdater.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_VPRO_OBJECTIVE_STATS_UPDATER_H
#define BLAZE_CUSTOM_FIFA_VPRO_OBJECTIVE_STATS_UPDATER_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifastatsvalueutil.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace Stats
{
	class UpdateStatsRequestBuilder;
}

namespace GameReporting
{
	class ProcessedGameReport;

/*!
    class FifaVProObjectiveStatsUpdater
*/
class FifaVProObjectiveStatsUpdater
{
public:
	FifaVProObjectiveStatsUpdater() {}
	~FifaVProObjectiveStatsUpdater() {}

	void initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport, Stats::StatPeriodType periodType = Stats::STAT_PERIOD_ALL_TIME);

	void selectObjectiveStats();
	void transformObjectiveStats();


private:
	typedef eastl::list<Stats::UpdateRowKey> UpdateRowKeyList;
	
	const char8_t* getObjectiveStatCategory();
	void getEntityIds(IFifaSeasonalPlayExtension::EntityList& ids);

	void readObjectiveStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap);
	void updateObjectiveStats(EntityId clubEntityId, Stats::UpdateRowKey osKey, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap);

	OSDKClubGameReportBase::OSDKClubClubReport* getOsdkClubReport(EntityId entityId);
	uint64_t getClubIdFromPlayerReport(EntityId id);

	Stats::UpdateStatsRequestBuilder*	mBuilder;
    Stats::UpdateStatsHelper*           mUpdateStatsHelper;
	ProcessedGameReport*				mProcessedReport;
	
	Stats::StatPeriodType				mStatPeriodType;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_VPRO_OBJECTIVE_STATS_UPDATER_H

