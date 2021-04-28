/*************************************************************************************************/
/*!
    \file   fifa_elo_rpghybridskill.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFA_ELORPGHYBRIDSKILL_H
#define BLAZE_CUSTOM_FIFA_ELORPGHYBRIDSKILL_H

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifastatsvalueutil.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskillextensions.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

namespace GameReporting
{
	class ProcessedGameReport;
	class ReportConfig;
	class IFifaEloRpgHybridSkillExtension;

/*!
    class FifaEloRpgHybridSkill
*/
class FifaEloRpgHybridSkill
{
public:
	FifaEloRpgHybridSkill() {}
	~FifaEloRpgHybridSkill() {}

	void setExtension(IFifaEloRpgHybridSkillExtension* extension);
	void initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport, ReportConfig* reportConfig);

	void selectEloStats();
	void transformEloStats();

private:
	typedef eastl::list<Stats::UpdateRowKey> UpdateRowKeyList;

	void readEloStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap);
	void writeEloStats(StatValueUtil::StatUpdateMap& statUpdateMap);

	void preSetupForStatCalc(UpdateRowKeyList& keyList);
	void doPreStatTransform(Stats::UpdateRowKey key, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap);

	void updateEloRpgHybridSkillPoints(Stats::UpdateRowKey key, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap);
	void updateEloRpgHybridSkillLevel(Stats::UpdateRowKey key, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap, bool isClubs = false);
	void updateOpponentEloRpgHybridSkillLevel(Stats::UpdateRowKey key, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap);
	void updateStarLevel(Stats::UpdateRowKey key, StatValueUtil::StatUpdateMap* statUpdateMap);

	int64_t calcPointsEarned(int64_t iUserPts, int64_t iOppoPts, int32_t iWlt, int32_t iAdj, int32_t iSkill, int32_t iTeamDifference);
	int64_t calcPerformancePointsEarned(Collections::AttributeMap* selfAttributes, Collections::AttributeMap* opponentAttributes,
		int64_t selfPerfPts, int64_t selfCompPts, int64_t oppoPerfPts, int64_t oppoCompPts, bool ignoreTeamWeighting);
	const Stats::StatCategorySummary* findInStatCategoryList(const char8_t* categoryName, const Stats::StatCategoryList::StatCategorySummaryList& categories) const;

	Stats::UpdateStatsRequestBuilder*	mBuilder;
	Stats::UpdateStatsHelper*           mUpdateStatsHelper;
	ProcessedGameReport*				mProcessedReport;
    ReportConfig*                       mReportConfig;

	IFifaEloRpgHybridSkillExtension*	mExtension;

	enum {SIDE_HOME, SIDE_AWAY, SIDE_NUM};
	int64_t mMatchPerformance[SIDE_NUM];	
	int64_t mTeamBonus[SIDE_NUM];	
	int64_t mMatchResult[SIDE_NUM];	
	int64_t mTotalpoints[SIDE_NUM];
	int64_t mCurrentLevel[SIDE_NUM];
	int64_t mPointsToNextLevel[SIDE_NUM];

	struct TeamPlayerCnt
	{
		int32_t		mTeamID;
		uint32_t	mPlayerCnt;
	};
	TeamPlayerCnt	mClubPlayerCnt[SIDE_NUM];
};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFA_ELORPGHYBRIDSKILL_H

