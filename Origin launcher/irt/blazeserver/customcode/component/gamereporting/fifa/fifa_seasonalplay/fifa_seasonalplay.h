/*************************************************************************************************/
/*!
    \file   fifa_seasonalplay.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_SEASONALPLAY_H
#define BLAZE_CUSTOM_FIFA_SEASONALPLAY_H

/*** Include files *******************************************************************************/
#include "fifa_seasonalplayextensions.h"
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
    class FifaSeasonalPlay
*/
class FifaSeasonalPlay
{
public:
	FifaSeasonalPlay() {}
	~FifaSeasonalPlay() {}

	void initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport);
	void setExtension(IFifaSeasonalPlayExtension* seasonalPlayExtension);
	const IFifaSeasonalPlayExtension* getExtension() const { return mSeasonalPlayExtension; }

	void selectSeasonalPlayStats();
	void transformSeasonalPlayStats();

	void updateCupStats();
	void updateDivCounts();
	static void calculateProjectedPoints(int64_t& projectedPoints,int64_t& prevProjectedPoints,int64_t gamesPlayed,int64_t extraMatches,int64_t seasonCurrentPoints,const GameReporting::DefinesHelper* pDefinesHelper);

	void getSPOverallStatXmlString(uint64_t entityId, eastl::string& buffer);
	void getSPDivisionalStatXmlString(uint64_t entityId, eastl::string& buffer);

private:
	typedef eastl::list<Stats::UpdateRowKey> UpdateRowKeyList;

	void readDivSeasonalPlayStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap);
	void readOverallSeasonalPlayStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap);
	void readLastOpponentSeasonalPlayStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap);
	void writeSeasonalPlayStats(StatValueUtil::StatUpdateMap& statUpdateMap);

	void updateSeasonPlayStats(Stats::UpdateRowKey key, Stats::UpdateRowKey spoKey, Stats::UpdateRowKey spLastOppKey, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap);
	bool hasSeasonEnded(EntityId entityId, StatValueUtil::StatValueMap* statValueMap);
	bool processSeasonEndStats(EntityId entityId, StatValueUtil::StatValueMap* statValueMap, StatValueUtil::StatValueMap* statSPOValueMap);
	void updateSeasonPlayPoints(EntityId entityId, StatValueUtil::StatValueMap* statValueMap, StatValueUtil::StatValueMap* statSPOValueMap, StatValueUtil::StatValueMap* statSPLastOpponentValueMap);
	bool updateSeasonPlayCupStats(EntityId entitytId, StatValueUtil::StatValueMap* statValueMap, StatValueUtil::StatValueMap* statSPOValueMap);
	void resetSeasonalStats(StatValueUtil::StatValueMap* statValueMap, bool isCupStage);
	void triggerAdvanceToNextSeason(StatValueUtil::StatValueMap* statValueMap, StatValueUtil::StatValueMap* statSPOValueMap, EntityId entity, bool isCupOver = true);
	void setQualifyingDiv(EntityId entityId, uint32_t div);

	int64_t GetNumGamesInSeason(StatValueUtil::StatValueMap* statValueMap);

	static const int ONLINE_MATCH_STATS_INTERVAL = 300;
	
	Stats::UpdateStatsRequestBuilder*	mBuilder;
    Stats::UpdateStatsHelper*           mUpdateStatsHelper;
	ProcessedGameReport*				mProcessedReport;
	
	IFifaSeasonalPlayExtension*			mSeasonalPlayExtension;
	Stats::StatPeriodType				mStatPeriodType;

	eastl::map<uint64_t, eastl::string>		mSPOverallPlayerStatsXml;
	eastl::map<uint64_t, eastl::string>		mSPDivisionalPlayerStatsXml;
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_SEASONALPLAY_H

