/*************************************************************************************************/
/*!
    \file   fifa_elo_rpghybridskill.cpp
    \attention
        (c) Electronic Arts Inc. 2012
*/
/*************************************************************************************************/

/*** Include files *******************************************************************************/
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskill.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskillextensions.h"
#include "gamereporting/fifa/fifa_elo_rpghybridskill/fifa_elo_rpghybridskilldefines.h"

#include "customcode/component/gamereporting/osdk/reportconfig.h"
#include "osdk/gamecommon.h"
#include "customcomponent/osdkseasonalplay/osdkseasonalplayslaveimpl.h"
#include "customcomponent/fifacups/fifacupsslaveimpl.h"
#include "customcomponent/xmshd/xmshdslaveimpl.h"

#include "stats/statsslaveimpl.h"
namespace Blaze
{
namespace GameReporting
{

/*** Defines *****************************************************************/

// New ranking formula tweaking parameters.
// Adjust only with *extreme caution*.
#define NEW_RANK_MAXIMUM        234999
#define NEW_RANK_MINIMUM        1
#define NEW_PROBABILITY_SCALE   4000.0
#define NEW_FADE_POWER          1.75
#define NEW_RANGE_POWER         2.5
#define NEW_POINTS_RANGE        0.4
#define NEW_PROBABILITY_UNFAIR  0.95
#define NEW_CHANGE_MAXIMUM      400
#define NEW_CHANGE_MINIMUM      100
#define NEW_POINTS_GUARANTEE    3000
#define NEW_ADJUSTMENT_SCALE    25
#define NEW_TEAMS_SCALE         50
#define NEW_OVERTIME_SCALE      0.5

#define NEW_SKILL_PCT_0         70
#define NEW_SKILL_PCT_1         80
#define NEW_SKILL_PCT_2         90
#define NEW_SKILL_PCT_3         100

#define LOW_COMP_PT_RATIO		6

#define PLAYER_RATING_VALUE		6

// Enable this to debug specific player counts in club matches.
//#define OVERRIDE_NUM_PLAYERS
#define SELF_PLAYERS_OVERRIDE   2
#define OPPO_PLAYERS_OVERRIDE   11

#define _CLAMP(iInput, iFloor, iCeiling) \
    ((iInput > iCeiling) ? iCeiling \
     : (iInput < iFloor) ? iFloor \
     : iInput)

static const char8_t STATNAME_CURRCOMPPTS[]    = "currCompPts";
static const char8_t STATNAME_CURRPERFPTS[]    = "currPerfPts";
static const char8_t STATNAME_PREVCOMPPTS[]    = "prevCompPts";
static const char8_t STATNAME_PREVPERFPTS[]    = "prevPerfPts";
// Points that would have been awarded given the teams were equal in strength
static const char8_t STATNAME_EQTCOMPPTS[]    = "eqTCompPts";
static const char8_t STATNAME_EQTPERFPTS[]    = "eqTPerfPts";
static const char8_t STATNAME_TILLNEXTLEVELPTS[]    = "ptsTillNextLevel";
static const char8_t STATNAME_GAINEDTHISLEVELPTS[]    = "ptsGainedThisLevel";
static const char8_t STATNAME_OPPONENTSKILLPTS[]    = "opponentSkillPoints";
static const char8_t STATNAME_OPPONENTLEVEL[]	= "opponentLevel";
static const char8_t STATNAME_PREVOPPONENTSKILLPTS[]    = "prevOpponentSkillPoints";
static const char8_t STATNAME_STAR_LEVEL[]		= "starLevel";
static const char8_t STATNAME_LEVEL[]			= "skill";
static const char8_t STATNAME_LEVEL_PREV[]		= "skillPrev";

static const int32_t STAR_LEVELS[] = {5, 10, 15, 20, 25, 30, 35, 40, 45, 99};
static const int32_t NUM_STAR_LEVELS = 10;

void FifaEloRpgHybridSkill::setExtension(IFifaEloRpgHybridSkillExtension* extension)
{
	mExtension = extension;
}

void FifaEloRpgHybridSkill::initialize(Stats::UpdateStatsRequestBuilder* builder, Stats::UpdateStatsHelper* updateStatsHelper, ProcessedGameReport* processedReport,  ReportConfig* reportConfig)
{
	mBuilder = builder;
	mUpdateStatsHelper = updateStatsHelper;
	mProcessedReport = processedReport;
	mReportConfig = reportConfig;
}

void FifaEloRpgHybridSkill::selectEloStats()
{
	IFifaEloRpgHybridSkillExtension::CategoryInfoList categoryInfoList;
	mExtension->getEloStatUpdateInfo(categoryInfoList);

	IFifaEloRpgHybridSkillExtension::CategoryInfoList::iterator categoryStatIter = categoryInfoList.begin();
	IFifaEloRpgHybridSkillExtension::CategoryInfoList::iterator categoryStatEnd = categoryInfoList.end();

	for(; categoryStatIter != categoryStatEnd; ++categoryStatIter)
	{
		IFifaEloRpgHybridSkillExtension::CategoryInfo* categoryInfo = categoryStatIter;

		IFifaEloRpgHybridSkillExtension::EntityInfo* info = NULL;
		for (int i = 0; i < 2; i++)
		{
			if (i % 2 == 0)
			{
				info = &categoryInfo->entityA;
			}
			else
			{
				info = &categoryInfo->entityB;		
			}

			Stats::ScopeNameValueMap scopeNameValueMap;
			IFifaEloRpgHybridSkillExtension::EntityInfo::ScopeList::iterator scopeListIter = info->scopeList.begin();
			IFifaEloRpgHybridSkillExtension::EntityInfo::ScopeList::iterator scopeListEnd = info->scopeList.end();
			for (; scopeListIter != scopeListEnd; ++scopeListIter)
			{
				scopeNameValueMap.insert(eastl::make_pair(scopeListIter->name, scopeListIter->value));
			}
			
			TRACE_LOG("[FifaEloRpgHybridSkill].selectEloStats() for " << categoryInfo->category << " " << " id " << info->entityId << " ");

			//select Elo related stats
			mBuilder->startStatRow(categoryInfo->category, info->entityId, info->scopeList.size() > 0? &scopeNameValueMap : NULL);
			mBuilder->selectStat(STATNAME_CURRCOMPPTS);
			mBuilder->selectStat(STATNAME_CURRPERFPTS);
			mBuilder->selectStat(STATNAME_PREVCOMPPTS);
			mBuilder->selectStat(STATNAME_PREVPERFPTS);
			mBuilder->selectStat(STATNAME_EQTCOMPPTS);
			mBuilder->selectStat(STATNAME_EQTPERFPTS);
			mBuilder->selectStat(STATNAME_OPPONENTLEVEL);
			mBuilder->selectStat(STATNAME_TILLNEXTLEVELPTS);
			mBuilder->selectStat(STATNAME_GAINEDTHISLEVELPTS);
			mBuilder->selectStat(STATNAME_OPPONENTSKILLPTS);
			mBuilder->selectStat(STATNAME_PREVOPPONENTSKILLPTS);
			//TODO do i need an if check for star level?
			mBuilder->selectStat(STATNAME_STAR_LEVEL);
			mBuilder->selectStat(STATNAME_LEVEL);
			mBuilder->selectStat(STATNAME_LEVEL_PREV);

			mBuilder->completeStatRow();
		}
	}
}

void FifaEloRpgHybridSkill::transformEloStats()
{
	IFifaEloRpgHybridSkillExtension::CategoryInfoList categoryInfoList;
	mExtension->getEloStatUpdateInfo(categoryInfoList);

	IFifaEloRpgHybridSkillExtension::CategoryInfoList::iterator categoryStatIter = categoryInfoList.begin();
	IFifaEloRpgHybridSkillExtension::CategoryInfoList::iterator categoryStatEnd = categoryInfoList.end();

	// get categories from stat config.
	BlazeRpcError waitResult = Blaze::ERR_OK;
	Blaze::Stats::StatCategoryList statCategoriesRsp;
	{
		UserSession::SuperUserPermissionAutoPtr autoPtr(true);
		OSDKSeasonalPlay::OSDKSeasonalPlaySlave *seasonalPlayComponent =
			static_cast<OSDKSeasonalPlay::OSDKSeasonalPlaySlave*>(gController->getComponent(OSDKSeasonalPlay::OSDKSeasonalPlaySlave::COMPONENT_ID, false));
		FifaCups::FifaCupsSlave *fifaCupsComponent =
			static_cast<FifaCups::FifaCupsSlave*>(gController->getComponent(FifaCups::FifaCupsSlave::COMPONENT_ID,false));
		Blaze::XmsHd::XmsHdSlave *xmsHdComponent = static_cast<Blaze::XmsHd::XmsHdSlave*>(gController->getComponent(Blaze::XmsHd::XmsHdSlave::COMPONENT_ID, false));

		Stats::StatsSlaveImpl* statsComponent = static_cast<Stats::StatsSlaveImpl*>(gController->getComponent(Stats::StatsSlaveImpl::COMPONENT_ID, false));

		if (seasonalPlayComponent != NULL && fifaCupsComponent != NULL && xmsHdComponent != NULL)
			TRACE_LOG("components not NULL" );

		if (statsComponent == NULL)
		{
			return;
		}
		else
		{
			waitResult = statsComponent->getStatCategoryList(statCategoriesRsp);
		}
	}

	Blaze::Stats::StatCategorySummary::StatPeriodTypeList defaultPeriodList;
	defaultPeriodList.push_back(Stats::STAT_PERIOD_ALL_TIME); // default to all-time stat;

	eastl::vector<UpdateRowKeyList> updateRowKeyListPerCategory;
	for(; categoryStatIter != categoryStatEnd; ++categoryStatIter)
	{
		IFifaEloRpgHybridSkillExtension::CategoryInfo* categoryInfo = categoryStatIter;

		UpdateRowKeyList updateKeyList;

		if(waitResult == Blaze::ERR_OK)
		{
			const Blaze::Stats::StatCategorySummary* statCategory = findInStatCategoryList(categoryInfo->category, statCategoriesRsp.getCategories());
			const Blaze::Stats::StatCategorySummary::StatPeriodTypeList &periodList = statCategory->getPeriodTypes();
			defaultPeriodList.assign(periodList.begin(), periodList.end());
		}
		
		Blaze::Stats::StatCategorySummary::StatPeriodTypeList::const_iterator periodItr = defaultPeriodList.begin();
		Blaze::Stats::StatCategorySummary::StatPeriodTypeList::const_iterator periodItrEnd = defaultPeriodList.end();

		TRACE_LOG("[FifaEloRpgHybridSkill].transformEloStats() generating keys for entitys: "<< categoryInfo->entityA.entityId <<" and " << categoryInfo->entityB.entityId <<" " );

		for(; periodItr != periodItrEnd; ++periodItr)
		{
			Stats::UpdateRowKey* key = mBuilder->getUpdateRowKey(categoryInfo->category, categoryInfo->entityA.entityId);
			if (key != NULL)
			{
				key->period = *periodItr;
				updateKeyList.push_back(*key);
			}
			key = mBuilder->getUpdateRowKey(categoryInfo->category, categoryInfo->entityB.entityId);
			if (key != NULL)
			{
				key->period = *periodItr;
				updateKeyList.push_back(*key);
			}
		}
		
		updateRowKeyListPerCategory.push_back(updateKeyList);
	}


	eastl::vector<UpdateRowKeyList>::iterator updateRowKeyListPerCategoryIter = updateRowKeyListPerCategory.begin();
	eastl::vector<UpdateRowKeyList>::iterator updateRowKeyListPerCategoryEnd = updateRowKeyListPerCategory.end();

	for (; updateRowKeyListPerCategoryIter != updateRowKeyListPerCategoryEnd; ++updateRowKeyListPerCategoryIter)
	{
		UpdateRowKeyList* eloKeys = updateRowKeyListPerCategoryIter;

		StatValueUtil::StatUpdateMap statUpdateMap;
		readEloStats(*eloKeys, statUpdateMap);
		preSetupForStatCalc(*eloKeys);

		UpdateRowKeyList::const_iterator eloKeyIter, eloKeyEnd;
		eloKeyIter = eloKeys->begin();
		eloKeyEnd = eloKeys->end();

		for (; eloKeyIter != eloKeyEnd; ++eloKeyIter)
		{
			doPreStatTransform(*eloKeyIter, *eloKeys, &statUpdateMap);
			updateEloRpgHybridSkillPoints(*eloKeyIter, *eloKeys, &statUpdateMap);
		}

		eloKeyIter = eloKeys->begin();
		for (; eloKeyIter != eloKeyEnd; ++eloKeyIter)
		{
			updateEloRpgHybridSkillLevel(*eloKeyIter, *eloKeys, &statUpdateMap);
			updateOpponentEloRpgHybridSkillLevel(*eloKeyIter, *eloKeys, &statUpdateMap);
		}

		eloKeyIter = eloKeys->begin();
		for (; eloKeyIter != eloKeyEnd; ++eloKeyIter)
		{
			updateStarLevel(*eloKeyIter, &statUpdateMap);
		}

		writeEloStats(statUpdateMap);
	}
}

void FifaEloRpgHybridSkill::readEloStats(UpdateRowKeyList& keyList, StatValueUtil::StatUpdateMap& statUpdateMap)
{
	UpdateRowKeyList::iterator iter = keyList.begin();
	UpdateRowKeyList::iterator end = keyList.end();

	for (; iter != end; iter++)
	{
		StatValueUtil::StatUpdate statUpdate;
		Stats::UpdateRowKey key = *iter;
		Stats::StatPeriodType periodType = static_cast<Stats::StatPeriodType>(key.period);

		TRACE_LOG("[FifaEloRpgHybridSkill].readEloStats() for id " << key.entityId << " ");

		insertStat(statUpdate, STATNAME_CURRCOMPPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CURRCOMPPTS, periodType, true));
		insertStat(statUpdate, STATNAME_CURRPERFPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_CURRPERFPTS, periodType, true));
		insertStat(statUpdate, STATNAME_PREVCOMPPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PREVCOMPPTS, periodType, true));
		insertStat(statUpdate, STATNAME_PREVPERFPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PREVPERFPTS, periodType, true));
		insertStat(statUpdate, STATNAME_EQTCOMPPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_EQTCOMPPTS, periodType, true));
		insertStat(statUpdate, STATNAME_EQTPERFPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_EQTPERFPTS, periodType, true));
		insertStat(statUpdate, STATNAME_OPPONENTLEVEL, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPONENTLEVEL, periodType, true));
		insertStat(statUpdate, STATNAME_TILLNEXTLEVELPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_TILLNEXTLEVELPTS, periodType, true));
		insertStat(statUpdate, STATNAME_GAINEDTHISLEVELPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_GAINEDTHISLEVELPTS, periodType, true));
		insertStat(statUpdate, STATNAME_OPPONENTSKILLPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_OPPONENTSKILLPTS, periodType, true));
		insertStat(statUpdate, STATNAME_PREVOPPONENTSKILLPTS, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_PREVOPPONENTSKILLPTS, periodType, true));
		//TODO make this optional?
		insertStat(statUpdate, STATNAME_STAR_LEVEL, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_STAR_LEVEL, periodType, true));
		insertStat(statUpdate, STATNAME_LEVEL, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LEVEL, periodType, true));
		insertStat(statUpdate, STATNAME_LEVEL_PREV, periodType, mUpdateStatsHelper->getValueInt(&key, STATNAME_LEVEL_PREV, periodType, true));

		statUpdateMap.insert(eastl::make_pair(key, statUpdate));
	}
}

void FifaEloRpgHybridSkill::writeEloStats(StatValueUtil::StatUpdateMap& statUpdateMap)
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

int64_t FifaEloRpgHybridSkill::calcPerformancePointsEarned(Collections::AttributeMap* selfAttributes, Collections::AttributeMap* opponentAttributes,
													int64_t selfPerfPts, int64_t selfCompPts, int64_t oppoPerfPts, int64_t oppoCompPts, bool ignoreTeamWeighting)
{
	TRACE_LOG("[PerformanceTransform:"<<this<<"].calling calcPerformancePointsEarned");

	// Global constants for performance modified
	const int32_t	RANK_PT_DIFFERENCE_SCALE			= 4000;
	const double	MAX_PERFORMANCE_POINTS				= 3000.0;
	const double	PERFORMANCE_LEVEL_GLOBAL_WEIGHTING	= 1.4;

	// Modifier types
	const int32_t STANDARD_ONE_VALUE	= 0;
	const int32_t PERCENT_TWO_VALUES	= 1;
	const int32_t OPPONENT_ONE_VALUE	= 2;

	// Calculate performance points adjustment based on stats reported by the user.
	struct PerformanceModifier
	{
		const char8_t*	mAttribName1;
		const char8_t*	mAttribName2;
		int32_t			mModType;
		double			mMax;
		double			mMin;
		double			mPtsPerUnit;
		double			mCustomParam; // Varies its meaning based on the modtype.
	};
	const int MODIFIER_COUNT = 12;
	const PerformanceModifier modifiers[MODIFIER_COUNT] = 
	{ 
		{ ATTRIBNAME_GOALS,			"",							STANDARD_ONE_VALUE,		3,		0,		3,		0.0 },
		{ ATTRIBNAME_GOALS,			"",							OPPONENT_ONE_VALUE,		3,		0,		-1,		0.0 },
		{ ATTRIBNAME_PASSESMADE,	ATTRIBNAME_PASSATTEMPTS,	PERCENT_TWO_VALUES,		95,		50,		0.2,	10.0 },
		{ ATTRIBNAME_TACKLESMADE,	ATTRIBNAME_TACKLEATTEMPTS,	PERCENT_TWO_VALUES,		95,		50,		0.2,	10.0 },
		{ ATTRIBNAME_POSSESSION,	"",							STANDARD_ONE_VALUE,		100,	50,		0.5,	0.0 },
		{ ATTRIBNAME_SHOTS,			"",							STANDARD_ONE_VALUE,		15,		0,		0.4,	0.0 },
		{ ATTRIBNAME_SHOTSONGOAL,	"",							STANDARD_ONE_VALUE,		15,		0,		0.8,	0.0 },
		{ ATTRIBNAME_FOULS,			"",							STANDARD_ONE_VALUE,		20,		0,		-0.05,	0.0 },
		{ ATTRIBNAME_YELLOWCARDS,	"",							STANDARD_ONE_VALUE,		4,		0,		-0.5,	0.0 },
		{ ATTRIBNAME_REDCARDS,		"",							STANDARD_ONE_VALUE,		3,		0,		-1.0,	0.0 },
		{ ATTRIBNAME_CORNERS,		"",							STANDARD_ONE_VALUE,		10,		0,		0.4,	0.0 },
		{ ATTRIBNAME_OFFSIDES,		"",							STANDARD_ONE_VALUE,		10,		0,		-0.05,	0.0 }
	};

	// Execute the modifiers
	double performancePointTotal = 0;
	for ( int32_t i = 0; i < MODIFIER_COUNT; ++i)
	{
		const PerformanceModifier& mod = modifiers[i];
		double value = 0;
		switch(mod.mModType)
		{
		case STANDARD_ONE_VALUE:
			{
				value = _CLAMP(((double)StatValueUtil::getInt(selfAttributes, mod.mAttribName1)), mod.mMin, mod.mMax);
				break;
			}
		case OPPONENT_ONE_VALUE:
			{
				value = _CLAMP(((double)(StatValueUtil::getInt(opponentAttributes, mod.mAttribName1))),
									mod.mMin, mod.mMax);
				break;
			}
		case PERCENT_TWO_VALUES:
			{
				double base = (((double)StatValueUtil::getInt(selfAttributes, mod.mAttribName2)));
				if ( base == 0.0 || base < mod.mCustomParam )
				{
					value = 0.0; 
				}
				else
				{
					value = ((double)(100.0 * ((double)StatValueUtil::getInt(selfAttributes, mod.mAttribName1)) / base ));
				}
				value = _CLAMP(value, mod.mMin, mod.mMax);
				break;
			}
		default:
			{
				
				WARN_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() unhandled performance modifier type: "<<mod.mModType<<" ");
				break;
			}
		}
		double increase = ((double)mod.mPtsPerUnit)*((double)(value - mod.mMin));
		performancePointTotal += increase;
		//TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() Increasing perf by %f, due to (mod:%s_%s, value:%f) xtra: %d, %f, %f, %f)",
		//	this, increase, mod.mAttribName1, mod.mAttribName2, value, mod.mModType, mod.mMax, mod.mMin, mod.mPtsPerUnit);
	}

	// Scale performance increase based on currPerfPts, and the constants specified at the beginning of this method.

	int oppoPlayerCnt = 0;
	int selfPlayerCnt = 0;
	if ( mExtension->isClubs() )
	{
#if defined(OVERRIDE_NUM_PLAYERS)
		oppoPlayerCnt = OPPO_PLAYERS_OVERRIDE;
		selfPlayerCnt = SELF_PLAYERS_OVERRIDE;
#else
		if (StatValueUtil::getInt(opponentAttributes, ATTRIBNAME_TEAM) == mClubPlayerCnt[SIDE_HOME].mTeamID)
		{
			oppoPlayerCnt = mClubPlayerCnt[SIDE_HOME].mPlayerCnt;
			selfPlayerCnt = mClubPlayerCnt[SIDE_AWAY].mPlayerCnt;
		}
		else
		{
			oppoPlayerCnt = mClubPlayerCnt[SIDE_AWAY].mPlayerCnt;
			selfPlayerCnt = mClubPlayerCnt[SIDE_HOME].mPlayerCnt;
		}
#endif
	}

	// Average opponent values needed
	int64_t	opponentRankPointTotal	= 0;
	int64_t	opponentTeamRatingTotal	= 0;
	int64_t oppoTotalRankPts	= oppoCompPts + oppoPerfPts;
	TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() oppoTotalRankPts="<<oppoTotalRankPts<<" ");
	int oppoTeamOverall		= mExtension->isClubs() ? 100 : StatValueUtil::getInt(opponentAttributes, ATTRIBNAME_TEAMRATING);
	TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() oppoTeamOverall="<<oppoTeamOverall<<" ");
	int oppoTeamRating		= oppoTeamOverall - oppoPlayerCnt * PLAYER_RATING_VALUE;
	TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() oppoTeamRating="<<oppoTeamRating<<" ");
	opponentRankPointTotal	+= oppoTotalRankPts;
	opponentTeamRatingTotal	+= oppoTeamRating;

	// Calculate the adjustment
	int64_t playerRankPoint		= selfCompPts + selfPerfPts;
	TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() playerRankPoint="<<playerRankPoint);
	int playerTeamOverall	= mExtension->isClubs() ? 100 : StatValueUtil::getInt(selfAttributes, ATTRIBNAME_TEAMRATING);
	TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() playerTeamOverall="<< playerTeamOverall);
	int playerTeamRating	= playerTeamOverall - selfPlayerCnt * PLAYER_RATING_VALUE;
	TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() playerTeamRating="<<playerTeamRating);
	double rankAdjustment = ((double)(_CLAMP((opponentRankPointTotal - playerRankPoint), -RANK_PT_DIFFERENCE_SCALE/2, RANK_PT_DIFFERENCE_SCALE/2)))
					/ ((double)RANK_PT_DIFFERENCE_SCALE);
	double teamAdjustment = ((double)(_CLAMP((opponentTeamRatingTotal - playerTeamRating), -50, 50))) / 50.0;

	// If it's the equivalent teams rating adjustment then don't apply a team adjustment
	if ( ignoreTeamWeighting )
	{
		TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() teamAdjustment set to 0");
		teamAdjustment = 0.0;
	}

	// Create combined adjustment
	double combinedAdjustment = _CLAMP((rankAdjustment + 1.0 + teamAdjustment), 0.0, 2.0) * PERFORMANCE_LEVEL_GLOBAL_WEIGHTING;
	// Apply combined adjustment
	performancePointTotal *= combinedAdjustment;
	// Scale to max point range
	// Get current performance points
	performancePointTotal = ((MAX_PERFORMANCE_POINTS - selfPerfPts) / MAX_PERFORMANCE_POINTS) * performancePointTotal;

	TRACE_LOG("[ReportProcessor:"<<this<<"].processPerformanceAdjustment() Applying perf increase of "<<performancePointTotal
		<<"(intermediates:"<<playerRankPoint
		<<", "<<playerTeamRating
		<<", "<<opponentRankPointTotal
		<<", "<<opponentTeamRatingTotal
		<<", "<<rankAdjustment
		<<", "<<teamAdjustment
		<<", "<<combinedAdjustment
		<<", "<<selfPerfPts
		<<", "<<MAX_PERFORMANCE_POINTS<<" ");

	return (int64_t)performancePointTotal;
}

/*F***************************************************************************/
/*!
    calcPointsEarned

    \Description
    Calculate number of ranking points to add/subtract based on game result

    \Input iUserPts        - current ranking points for user
    \Input iOppoPts        - current ranking points for opponent
    \Input iWlt            - WLT_flag indicating users game result
    \Input iAdj            - number of extra points to add/sub to total
    \Input iSkill          - skill level of game (points modifier)
    \Input iTeamDifference - team skill level difference

    \Output int32_t - Number of points to give to user (positive or negative)

    \Version 1.0 11/20/2002 (GWS) First version
*/
/***************************************************************************F*/

int64_t FifaEloRpgHybridSkill::calcPointsEarned(int64_t iUserPts, int64_t iOppoPts, int32_t iWlt, int32_t iAdj, int32_t iSkill, int32_t iTeamDifference)
{
    int32_t iPct[4] = { NEW_SKILL_PCT_0, NEW_SKILL_PCT_1,
        NEW_SKILL_PCT_2, NEW_SKILL_PCT_3 };

    // Clamp points at the ranking maximum to prevent users from gaining
    // too many points in a single year since there is no longer an upper
    // maximum on points earned due to RPG-style levelling.
    iUserPts = _CLAMP(iUserPts, NEW_RANK_MINIMUM, NEW_RANK_MAXIMUM);
    iOppoPts = _CLAMP(iOppoPts, NEW_RANK_MINIMUM, NEW_RANK_MAXIMUM);

    // number of points to add or subtract
    int64_t iPts = 0;
    int32_t iPtsDifference = static_cast<int32_t>(iOppoPts - iUserPts);
    double fProbability = (1.0 / (1.0 + pow(10.0, (double) (iPtsDifference+iTeamDifference*NEW_TEAMS_SCALE+iAdj*NEW_ADJUSTMENT_SCALE) / NEW_PROBABILITY_SCALE)) - 0.5) / (2*NEW_PROBABILITY_UNFAIR-1) + 0.5;
    fProbability = _CLAMP(fProbability, 0.0, 1.0);
    double fFade = pow(1.0 - (double)(iUserPts < iOppoPts ? iUserPts : iOppoPts) / NEW_RANK_MAXIMUM, NEW_FADE_POWER);
    int64_t iPtsRange = (int64_t) (NEW_POINTS_RANGE * abs(iPtsDifference) * pow((double) (abs(iPtsDifference)) / NEW_RANK_MAXIMUM, NEW_RANGE_POWER));
    iPtsRange = _CLAMP(iPtsRange, NEW_CHANGE_MINIMUM, NEW_CHANGE_MAXIMUM);
    iPtsRange = (int64_t) (fFade * iPtsRange);
    iPtsRange = _CLAMP(iPtsRange, 1, NEW_CHANGE_MAXIMUM);

    if (iWlt & (WLT_LOSS|WLT_DISC|WLT_CHEAT|WLT_CONCEDE))
    {
        iPts = (int64_t) (-fProbability * iPtsRange);
        iPts = iPts * iPct[iSkill] / 100;
        if (iPtsDifference < -NEW_POINTS_GUARANTEE)
            iPts = _CLAMP(iPts, -NEW_CHANGE_MAXIMUM, -1);
        if (iPts > 0)
            iPts = 0;
    }
	else if (iWlt & WLT_TIE)
	{
        iPts = (int64_t) ((0.5 - fProbability) * iPtsRange);
        iPts = iPts * iPct[iSkill] / 100;
        if (iPtsDifference > NEW_POINTS_GUARANTEE)
            iPts = _CLAMP(iPts, 1, NEW_CHANGE_MAXIMUM);
        if (iPts < 0)
            iPts = 0;
	}
    else if (iWlt & WLT_WIN)
    {
        iPts = (int64_t) ((1.0 - fProbability) * iPtsRange);
        iPts = iPts * iPct[iSkill] / 100;
        if (iPtsDifference > NEW_POINTS_GUARANTEE)
            iPts = _CLAMP(iPts, 1, NEW_CHANGE_MAXIMUM);
        if (iPts < 0)
            iPts = 0;
    }

    // limit the points adjustments to valid range
    // We don't need to clamp the upper maximum points because there
    // is no longer an upper max due to the RPG-style levelling system.
    if (iUserPts + iPts < NEW_RANK_MINIMUM)
        iPts = NEW_RANK_MINIMUM - iUserPts;
    if (iOppoPts - iPts < NEW_RANK_MINIMUM)
        iPts = NEW_RANK_MINIMUM + iOppoPts;

	TRACE_LOG("[ReportProcessor:"<<this<<"].calcPointsEarned() TEAMDIF:"<<iTeamDifference
		<<" Applying comp increase of "<<iPts
		<<" iUserPts="<<iUserPts
		<<", iOppoPts="<<iOppoPts
		<<", (intermediates:"<<fProbability
		<<", "<<iPtsDifference
		<<", "<<iPtsRange
		<<", "<<fFade
		<<", "<<iWlt <<"");

    return iPts;
}

// keys is a list of all the keys involved in this game.
// key is the current stat key we're transforming.
void FifaEloRpgHybridSkill::doPreStatTransform(Stats::UpdateRowKey key, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap)
{
    if (statUpdateMap != NULL)
    {
        StatValueUtil::StatUpdateMap::iterator updateIter, updateEnd;
        updateIter = statUpdateMap->find(key);
        updateEnd = statUpdateMap->end();
        if (updateIter == updateEnd)
        {
            WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find key");
            return;
        }

        StatValueUtil::StatUpdate* statUpdate = &(updateIter->second);

        StatValueUtil::StatValueMap* statValueMap = &(statUpdate->stats);

		// Get the curr competitive skill points.
        StatValueUtil:: StatValueMap::iterator currComp = statValueMap->find(STATNAME_CURRCOMPPTS);
        if (currComp == statValueMap->end()) { WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find statName "<< STATNAME_CURRCOMPPTS<<" "); return; }
		StatValueUtil::StatValue* svCurrComp = &(currComp->second); 

		// Get the curr performance skill points.
        StatValueUtil::StatValueMap::iterator currPerf = statValueMap->find(STATNAME_CURRPERFPTS);
        if (currPerf == statValueMap->end()) { WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find statName "<< STATNAME_CURRPERFPTS<<" "); return; }
		StatValueUtil::StatValue* svCurrPerf = &(currPerf->second); 

		// Get the prev competitive skill points.
        StatValueUtil::StatValueMap::iterator prevComp = statValueMap->find(STATNAME_PREVCOMPPTS);
        if (prevComp == statValueMap->end()) { WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find statName "<< STATNAME_CURRCOMPPTS<<" "); return; }
		StatValueUtil::StatValue* svPrevComp = &(prevComp->second); 

		// Get the prev performance skill points.
        StatValueUtil::StatValueMap::iterator prevPerf = statValueMap->find(STATNAME_PREVPERFPTS);
        if (prevPerf == statValueMap->end()) { WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find statName "<< STATNAME_CURRPERFPTS<<" "); return; }
		StatValueUtil::StatValue* svPrevPerf = &(prevPerf->second); 

		// Update all this keys stats
		int32_t period = key.period;
		{
			// Transfer some performance points to competitive points if the user is running out of comp points.
			// If the perf point total is LOW_COMP_PT_RATIO times greater or more than the comp total
			int64_t	ptsAvailableForTransfer =  (svCurrPerf->iBefore[period] * (LOW_COMP_PT_RATIO-1)) / LOW_COMP_PT_RATIO;
			int64_t ptsTransferThreshold		=  (svCurrPerf->iBefore[period]/LOW_COMP_PT_RATIO);
			if ( svCurrComp->iBefore[period] <  ptsTransferThreshold )
			{
				TRACE_LOG("[ReportProcessor:"<<this<<"].doPreStatTransform() CurrComp too low: "<<svCurrComp->iBefore[period] 
					<<". Borrowing from perf pts = "<<svCurrPerf->iBefore[period]
					<<"("<<LOW_COMP_PT_RATIO<<","<<ptsAvailableForTransfer<<","<<ptsTransferThreshold<<")");

				int64_t transferValue = ( ptsTransferThreshold - svCurrComp->iBefore[period]) / 2;
				svCurrPerf->iBefore[period] -= transferValue;
				svCurrComp->iBefore[period] += transferValue;
			}

			svPrevComp->iAfter[period] = svCurrComp->iBefore[period];
			svPrevPerf->iAfter[period] = svCurrPerf->iBefore[period];
			TRACE_LOG("[ReportProcessor:"<<this<<"].period: "<<period
				<<", prevComp set to "<<svCurrComp->iBefore[period]
				<<".  prevPerf set to "<<svCurrPerf->iBefore[period]<<" ");

		}
	}
}

void FifaEloRpgHybridSkill::updateEloRpgHybridSkillPoints(Stats::UpdateRowKey key, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap)
{
    if (statUpdateMap != NULL)
    {
        StatValueUtil::StatUpdateMap::iterator updateIter, updateEnd, oppoUpdateIter;
        updateIter = statUpdateMap->find(key);
        updateEnd = statUpdateMap->end();
        if (updateIter == updateEnd)
        {
            WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillPoints(): Cannot find key");
            return;
        }

        StatValueUtil::StatUpdate* statUpdate = &(updateIter->second);

        StatValueUtil::StatValueMap* statValueMap = &(statUpdate->stats);
		Collections::AttributeMap currentMap;
		mExtension->generateAttributeMap(key.entityId, currentMap);
        Collections::AttributeMap* map = &currentMap;

		// Get the disconnect field from the map.
		int32_t disc = StatValueUtil::getInt(map, ATTRIBNAME_DISC);
		if ( !disc )
		{
			// Check if this is a club that disconnected
			disc = StatValueUtil::getInt(map, ATTRIBNAME_CLUBDISC);
		}
		
		// Get the curr competitive skill points.
        StatValueUtil::StatValueMap::iterator currComp = statValueMap->find(STATNAME_CURRCOMPPTS);
        if (currComp == statValueMap->end()) { WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find statName "<< STATNAME_CURRCOMPPTS); return; }
		StatValueUtil::StatValue* svCurrComp = &(currComp->second); 

		// Get the curr performance skill points.
        StatValueUtil::StatValueMap::iterator currPerf = statValueMap->find(STATNAME_CURRPERFPTS);
        if (currPerf == statValueMap->end()) { WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find statName "<< STATNAME_CURRPERFPTS); return; }
		StatValueUtil::StatValue* svCurrPerf = &(currPerf->second); 

		// Get the team equivalent competitive skill points.
        StatValueUtil::StatValueMap::iterator eqtComp = statValueMap->find(STATNAME_EQTCOMPPTS);
        if (eqtComp == statValueMap->end()) { WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find statName "<< STATNAME_EQTCOMPPTS); return; }
		StatValueUtil::StatValue* svEqtComp = &(eqtComp->second); 

		// Get the team equivalent performance skill points.
        StatValueUtil::StatValueMap::iterator eqtPerf = statValueMap->find(STATNAME_EQTPERFPTS);
        if (eqtPerf == statValueMap->end()) { WARN_LOG("[ReportProcessor].saveCurrSkillPointsInPrev(): Cannot find statName "<< STATNAME_EQTPERFPTS); return; }
		StatValueUtil::StatValue* svEqtPerf = &(eqtPerf->second); 

		// Get the report values needed for this calculation
		int32_t statperc = StatValueUtil::getInt(map, ATTRIBNAME_STATPERC);
		// Get the team rating
		int32_t weight = StatValueUtil::getInt(map, ATTRIBNAME_TEAMRATING);
		int oppoPlayerCnt = 0;
		int selfPlayerCnt = 0;
		if ( mExtension->isClubs() )
		{
#if defined(OVERRIDE_NUM_PLAYERS)
			oppoPlayerCnt = OPPO_PLAYERS_OVERRIDE;
			selfPlayerCnt = SELF_PLAYERS_OVERRIDE;
#else
			if (StatValueUtil::getInt(map, ATTRIBNAME_TEAM) == mClubPlayerCnt[SIDE_HOME].mTeamID)
			{
				oppoPlayerCnt = mClubPlayerCnt[SIDE_AWAY].mPlayerCnt;
				selfPlayerCnt = mClubPlayerCnt[SIDE_HOME].mPlayerCnt;
			}
			else
			{
				oppoPlayerCnt = mClubPlayerCnt[SIDE_HOME].mPlayerCnt;
				selfPlayerCnt = mClubPlayerCnt[SIDE_AWAY].mPlayerCnt;
			}
#endif
			TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints: selfPlayerCnt: "<<selfPlayerCnt<<" oppoPlayerCnt: "<<oppoPlayerCnt<<" ");
			weight = 100 - ( selfPlayerCnt * PLAYER_RATING_VALUE );
		}
		TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints: weight: "<<weight<<" ");
		int32_t wlt = StatValueUtil::getInt(map, STATNAME_RESULT);

        UpdateRowKeyList::const_iterator keyIter, keyEnd;
        keyEnd = keys.end();
		keyIter = keys.begin();
        for (; keyIter != keyEnd; ++keyIter)
        {
            oppoUpdateIter = statUpdateMap->find(*keyIter);
			int32_t period = keyIter->period;

            if (oppoUpdateIter != updateEnd && updateIter != oppoUpdateIter && period == key.period)
            {
                // Find the opponents stats update
                StatValueUtil::StatUpdate* oppoStatUpdate = &(oppoUpdateIter->second);

                StatValueUtil::StatValueMap* oppoStatValueMap = &(oppoStatUpdate->stats);
				
				Collections::AttributeMap generateOppMap;
				Stats::UpdateRowKey oppoKey = oppoUpdateIter->first;
				mExtension->generateAttributeMap(oppoKey.entityId, generateOppMap);
                
				Collections::AttributeMap* oppoMap = &generateOppMap;

                int32_t oppoWeight = StatValueUtil::getInt(oppoMap, ATTRIBNAME_TEAMRATING);
				if ( mExtension->isClubs() )
				{
					oppoWeight = 100 - ( oppoPlayerCnt * PLAYER_RATING_VALUE );
				}
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints: oppoWeight: "<<oppoWeight<<" ");
				int32_t iTeamDifference = oppoWeight - weight;

                StatValueUtil::StatValueMap::iterator oppoCurrCompStatIter = oppoStatValueMap->find(STATNAME_CURRCOMPPTS);
                if (oppoCurrCompStatIter == oppoStatValueMap->end()) { continue; }
                StatValueUtil::StatValue* oppoCurrCompStatValue = &(oppoCurrCompStatIter->second);

                StatValueUtil::StatValueMap::iterator oppoCurrPerfStatIter = oppoStatValueMap->find(STATNAME_CURRPERFPTS);
                if (oppoCurrPerfStatIter == oppoStatValueMap->end()) { continue; }
                StatValueUtil::StatValue* oppoCurrPerfStatValue = &(oppoCurrPerfStatIter->second);

                {
					TRACE_LOG("[ReportProcessor:"<<this<<"].CalcPointsEarned with Team Rating. period: "<<period<<" ");
					
					// Do the ELO skill competitive points calculation
                    svCurrComp->iAfter[period] = svCurrComp->iBefore[period] + calcPointsEarned(
                        svCurrComp->iBefore[period], oppoCurrCompStatValue->iBefore[period],
                        wlt, 0, 1, iTeamDifference); // 0 for user weight difference, and 1 for 80% point rewards ( 0-3 is the valid range, and gives 70%-100% )

					// Do equivalent teams calculation
                    svEqtComp->iAfter[period] = svCurrComp->iBefore[period] + calcPointsEarned(
                        svCurrComp->iBefore[period], oppoCurrCompStatValue->iBefore[period],
                        wlt, 0, 1, 0); // 0 for user weight difference, and 1 for 80% point rewards ( 0-3 is the valid range, and gives 70%-100% )

					TRACE_LOG("[ReportProcessor:"<<this<<"].CalcPointsEarned with NO Team Rating. period: "<<period<<" ");

					// only award points if there was no disconnect
					if( disc == 0 )
					{
						// Do the performance points calculation
						svCurrPerf->iAfter[period] = svCurrPerf->iBefore[period] + calcPerformancePointsEarned( map, oppoMap, svCurrPerf->iBefore[period], svCurrComp->iBefore[period],
							oppoCurrPerfStatValue->iBefore[period], oppoCurrCompStatValue->iBefore[period], false );

						// Do equivalent teams calculation
						TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints eqtPerf before:"<<svCurrPerf->iBefore[period] <<" after:"<<svEqtPerf->iAfter[period] <<" ");
						svEqtPerf->iAfter[period] = svCurrPerf->iBefore[period] + calcPerformancePointsEarned( map, oppoMap, svCurrPerf->iBefore[period], svCurrComp->iBefore[period],
							oppoCurrPerfStatValue->iBefore[period], oppoCurrCompStatValue->iBefore[period], true );
					}
					else
					{
						TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints not awarding performance points because of disconnect");
					}
                }
            }
        }

		int32_t gameTime = StatValueUtil::getInt(map, ATTRIBNAME_GAMETIME);

        // prorate points, if necessary
		int32_t fullGame = 90*60;
        if ( ((statperc >= 0) && (statperc < 100)) || ((gameTime >=0) && (gameTime < fullGame)) )
        {
			TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints scaling points by "<<statperc<<" ");
			int32_t period = key.period;
            {
				int64_t deltaCurrComp = svCurrComp->iAfter[period] - svCurrComp->iBefore[period];
				int64_t deltaCurrPerf = svCurrPerf->iAfter[period] - svCurrPerf->iBefore[period];
				// eqT is always based off the curr values
				int64_t deltaEqtComp = svEqtComp->iAfter[period] - svCurrComp->iBefore[period];				
				int64_t deltaEqtPerf = svEqtPerf->iAfter[period] - svCurrPerf->iBefore[period];

				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints deltaCurrComp "<<deltaCurrComp<<" ");
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints deltaCurrPerf "<<deltaCurrPerf<<" ");
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints deltaEqtComp "<<deltaEqtComp<<" ");
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints deltaEqtPerf "<<deltaEqtPerf<<" ");

				// first prorate for statperc
				if( (statperc >= 0) && (statperc < 100) )
				{
					deltaCurrComp = static_cast<int32_t>(deltaCurrComp * (statperc / 100.0));
					deltaCurrPerf = static_cast<int32_t>(deltaCurrPerf * (statperc / 100.0));
					deltaEqtComp = static_cast<int32_t>(deltaEqtComp * (statperc / 100.0));
					deltaEqtPerf = static_cast<int32_t>(deltaEqtPerf * (statperc / 100.0));
				}

                // prorate for game time played - not prorating for disconnects in extra time
				if( (gameTime >=0) && (gameTime < fullGame) )
				{
					deltaCurrPerf = static_cast<int32_t>(deltaCurrPerf * (gameTime / ((float)(fullGame))));
					deltaEqtPerf = static_cast<int32_t>(deltaEqtPerf * (gameTime / ((float)(fullGame))));
				}

				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints deltaCurrComp prorate "<<deltaCurrComp<<" ");
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints deltaCurrPerf prorate "<<deltaCurrPerf<<" ");
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints deltaEqtComp prorate "<<deltaEqtComp<<" ");
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillPoints deltaEqtPerf prorate "<<deltaEqtPerf<<" ");

				svCurrComp->iAfter[period] = svCurrComp->iBefore[period] + deltaCurrComp;
				svCurrPerf->iAfter[period] = svCurrPerf->iBefore[period] + deltaCurrPerf;
				// eqT is always based off the curr values
				svEqtComp->iAfter[period] = svCurrComp->iBefore[period] + deltaEqtComp;
				svEqtPerf->iAfter[period] = svCurrPerf->iBefore[period] + deltaEqtPerf;
            }
        }
	}
}

void FifaEloRpgHybridSkill::updateEloRpgHybridSkillLevel(Stats::UpdateRowKey key, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap, bool isClubs)
{
    if (statUpdateMap != NULL)
    {
        StatValueUtil::StatUpdateMap::iterator updateIter, updateEnd, oppoUpdateIter;
        updateIter = statUpdateMap->find(key);
        updateEnd = statUpdateMap->end();
        if (updateIter == updateEnd)
        {
            WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find key");
            return;
        }

        StatValueUtil::StatUpdate* statUpdate = &(updateIter->second);
        
		StatValueUtil::StatValueMap* statValueMap = &(statUpdate->stats);
		Collections::AttributeMap map;
		mExtension->generateAttributeMap(key.entityId, map);

        StatValueUtil::StatValueMap::iterator statCompPointsIter = statValueMap->find(STATNAME_CURRCOMPPTS);
        if (statCompPointsIter == statValueMap->end())
        {
            WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statPointsName "<< STATNAME_CURRCOMPPTS<<" ");
            return;
        }

		StatValueUtil::StatValueMap::iterator statEQTCompPointsIter = statValueMap->find(STATNAME_EQTCOMPPTS);
		if (statEQTCompPointsIter == statValueMap->end())
		{
			WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statPointsName "<< STATNAME_EQTCOMPPTS<<" ");
			return;
		}
		
		StatValueUtil::StatValueMap::iterator statPrevCompPointsIter = statValueMap->find(STATNAME_PREVCOMPPTS);
		if (statPrevCompPointsIter == statValueMap->end())
		{
			WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statPointsName "<< STATNAME_PREVCOMPPTS<<" ");
			return;
		}

        StatValueUtil::StatValueMap::iterator statPerfPointsIter = statValueMap->find(STATNAME_CURRPERFPTS);
        if (statPerfPointsIter == statValueMap->end())
        {
            WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statPointsName "<< STATNAME_CURRPERFPTS<<" ");
            return;
        }

		StatValueUtil::StatValueMap::iterator statEQTPerfPointsIter = statValueMap->find(STATNAME_EQTPERFPTS);
		if (statEQTPerfPointsIter == statValueMap->end())
		{
			WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statPointsName "<< STATNAME_EQTPERFPTS<<" ");
			return;
		}

		StatValueUtil::StatValueMap::iterator statPrevPerfPointsIter = statValueMap->find(STATNAME_PREVPERFPTS);
		if (statPrevPerfPointsIter == statValueMap->end())
		{
			WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statPointsName "<< STATNAME_PREVPERFPTS<<" ");
			return;
		}

        StatValueUtil::StatValueMap::iterator statLevelIter = statValueMap->find(STATNAME_LEVEL);
        if (statLevelIter == statValueMap->end())
        {
            WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statLevelName "<< STATNAME_LEVEL);
            return;
        }
		StatValueUtil::StatValueMap::iterator statLevelPrevIter = statValueMap->find(STATNAME_LEVEL_PREV);

        StatValueUtil::StatValueMap::iterator statPointsTillNextLevelIter = statValueMap->find(STATNAME_TILLNEXTLEVELPTS);
        if (statPointsTillNextLevelIter == statValueMap->end())
        {
            WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statPointsName "<<STATNAME_TILLNEXTLEVELPTS<<" ");
            return;
        }

        StatValueUtil::StatValueMap::iterator statPointsSinceLastLevelIter = statValueMap->find(STATNAME_GAINEDTHISLEVELPTS);
        if (statPointsSinceLastLevelIter == statValueMap->end())
        {
            WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statLevelName "<<STATNAME_GAINEDTHISLEVELPTS<<" ");
            return;
        }


        StatValueUtil::StatValue* statCompPointsValue = &(statCompPointsIter->second);
		StatValueUtil::StatValue* statEQTCompPointsValue = &(statEQTCompPointsIter->second);
		StatValueUtil::StatValue* statPrevCompPointsValue = &(statPrevCompPointsIter->second);
        StatValueUtil::StatValue* statPerfPointsValue = &(statPerfPointsIter->second);
		StatValueUtil::StatValue* statEQTPerfPointsValue = &(statEQTPerfPointsIter->second);
		StatValueUtil::StatValue* statPrevPerfPointsValue = &(statPrevPerfPointsIter->second);
        StatValueUtil::StatValue* statLevelValue = &(statLevelIter->second);
		StatValueUtil::StatValue* statLevelPrevValue = NULL;
		StatValueUtil::StatValue* statPointsTillNextLevelValue = &(statPointsTillNextLevelIter->second);
		StatValueUtil::StatValue* statPointsSinceLastLevelValue = &(statPointsSinceLastLevelIter->second);

		StatValueUtil::StatValueMap::iterator statOppoPtsIter = statValueMap->find(STATNAME_OPPONENTSKILLPTS);
		StatValueUtil::StatValueMap::iterator statPrevOppoPtsIter = statValueMap->find(STATNAME_PREVOPPONENTSKILLPTS);
		StatValueUtil::StatValue* statOppoPtsValue = NULL;
		StatValueUtil::StatValue* statPrevOppoPtsValue = NULL;
		if ( !isClubs )
		{
			if (statOppoPtsIter == statValueMap->end())
			{
				WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statPointsName "<< STATNAME_OPPONENTSKILLPTS);
				return;
			}

			if (statPrevOppoPtsIter == statValueMap->end())
			{
				WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statLevelName "<<STATNAME_PREVOPPONENTSKILLPTS<<" ");
				return;
			}
			statOppoPtsValue = &(statOppoPtsIter->second);
			statPrevOppoPtsValue = &(statPrevOppoPtsIter->second);
		}
		else
		{
			if (statLevelPrevIter == statValueMap->end())
			{
				WARN_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel(): Cannot find statLevelName "<<STATNAME_LEVEL_PREV<<" ");
				return;
			}
			statLevelPrevValue = &(statLevelPrevIter->second);
		}

		{
			if (!isClubs)
			{

				// Get the opponent's before and after stats if this isn't clubs.
				// Clubs don't have these stats because Maria didn't use the extended session data.
				// Damn that would have been smart if I had done it that way in the first place.
				UpdateRowKeyList::const_iterator keyIter, keyEnd;
				keyEnd = keys.end();
				keyIter = keys.begin();
				for (; keyIter != keyEnd; ++keyIter)
				{
					int32_t period = keyIter->period;
					oppoUpdateIter = statUpdateMap->find(*keyIter);

					if (oppoUpdateIter != updateEnd && updateIter != oppoUpdateIter && period == key.period)
					{
						// Find the opponents stats update
						StatValueUtil::StatUpdate* oppoStatUpdate = &(oppoUpdateIter->second);
						StatValueUtil::StatValueMap* oppoStatValueMap = &(oppoStatUpdate->stats);

						StatValueUtil::StatValueMap::iterator oppoCurrCompStatIter = oppoStatValueMap->find(STATNAME_CURRCOMPPTS);
						if (oppoCurrCompStatIter == oppoStatValueMap->end()) { continue; }
						StatValueUtil::StatValue* oppoCurrCompStatValue = &(oppoCurrCompStatIter->second);

						StatValueUtil::StatValueMap::iterator oppoCurrPerfStatIter = oppoStatValueMap->find(STATNAME_CURRPERFPTS);
						if (oppoCurrPerfStatIter == oppoStatValueMap->end()) { continue; }
						StatValueUtil::StatValue* oppoCurrPerfStatValue = &(oppoCurrPerfStatIter->second);

						int64_t rankPointTotal = oppoCurrCompStatValue->iAfter[period] + oppoCurrPerfStatValue->iAfter[period];
						int64_t rankPointTotalPre = oppoCurrCompStatValue->iBefore[period] + oppoCurrPerfStatValue->iBefore[period];

						TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillLevel period:"<<period
							<<", oppo prev skill: "<<rankPointTotalPre
							<<". next skill:"<<rankPointTotal<<" "<<" entityid: "<<key.entityId<<" ");

						statOppoPtsValue->iAfter[period] = rankPointTotal;
						statPrevOppoPtsValue->iAfter[period] = rankPointTotalPre;
					}
				}
			}
			int32_t period = key.period;

            int64_t statPointsAfter = statCompPointsValue->iAfter[period] + statPerfPointsValue->iAfter[period];
            int64_t statLevelBefore = statLevelValue->iBefore[period];
			if(statLevelPrevValue != NULL)
			{
				statLevelPrevValue->iAfter[period] = statLevelBefore;
			}
			
            // Calculate the Skill level based on the skill points
            int64_t statLevel = mReportConfig->getSkillLevel(static_cast<uint32_t>(statPointsAfter));
            if (statLevel > statLevelBefore)
            {
                statLevelValue->iAfter[period] = statLevel;
            }

			// Special case stat level 1 to use 200 as the beginning of the level
			if ( statLevel == 1 )
			{
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillLevel period: "<<period
					<<", previous level: "<<mReportConfig->getPrevSkillLevelPoints(static_cast<uint32_t>(statPointsAfter))
					<<". next level: "<<mReportConfig->getNextSkillLevelPoints(static_cast<uint32_t>(statPointsAfter))
					<<". current stat points. "<< statPointsAfter <<"."<<" entityid: "<<key.entityId<<" ");
				statPointsTillNextLevelValue->iAfter[period] = static_cast<int64_t>(mReportConfig->getNextSkillLevelPoints(static_cast<uint32_t>(statPointsAfter)) - statPointsAfter);
				statPointsSinceLastLevelValue->iAfter[period] = (statPointsAfter - 200);
			}
			else
			{
				TRACE_LOG("[ReportProcessor:"<<this<<"].updateEloRpgHybridSkillLevel period: "<<period 
					<<"	previous level: "<<mReportConfig->getPrevSkillLevelPoints(static_cast<uint32_t>(statPointsAfter))
					<<". next level:"<< mReportConfig->getNextSkillLevelPoints(static_cast<uint32_t>(statPointsAfter))
					<<". current stat points. "<<statPointsAfter<<"."<<" entityid: "<<key.entityId<<" ");
				statPointsTillNextLevelValue->iAfter[period] = static_cast<int64_t>(mReportConfig->getNextSkillLevelPoints(static_cast<uint32_t>(statPointsAfter)) - statPointsAfter);
				statPointsSinceLastLevelValue->iAfter[period] = (statPointsAfter - mReportConfig->getPrevSkillLevelPoints(static_cast<uint32_t>(statPointsAfter)));
			}
        }
		
		const int32_t PERIOD0 = 0;
		int isHome = StatValueUtil::getInt(&map, ATTRIBNAME_HOME);
		int index = (isHome == 1) ? 0 : 1;
		TRACE_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel isHome = "<<isHome<<" ");

		mMatchPerformance[index] = statEQTPerfPointsValue->iAfter[PERIOD0] - statPrevPerfPointsValue->iAfter[PERIOD0];

		mTeamBonus[index] = (statCompPointsValue->iAfter[PERIOD0] - statEQTCompPointsValue->iAfter[PERIOD0]) + 
							(statPerfPointsValue->iAfter[PERIOD0] - statEQTPerfPointsValue->iAfter[PERIOD0]);

		mMatchResult[index] = statEQTCompPointsValue->iAfter[PERIOD0] - statPrevCompPointsValue->iAfter[PERIOD0]; 

		mCurrentLevel[index] = statLevelValue->iAfter[PERIOD0];
		mPointsToNextLevel[index] = statPointsTillNextLevelValue->iAfter[PERIOD0];
		mTotalpoints[index] = statCompPointsValue->iAfter[PERIOD0] + statPerfPointsValue->iAfter[PERIOD0];

		TRACE_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel mMatchPerformance["<<index<<"] = "<<mMatchPerformance[index]<<" ");
		TRACE_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel mTeamBonus["<<index<<"] = "<<mTeamBonus[index]<<" ");
		TRACE_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel mMatchResult["<<index<<"] = "<<mMatchResult[index]<<" ");
		TRACE_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel mCurrentLevel["<<index<<"] = "<<mCurrentLevel[index]<<" ");
		TRACE_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel mPointsToNextLevel["<<index<<"] = "<<mPointsToNextLevel[index]<<" ");
		TRACE_LOG("[ReportProcessor].updateEloRpgHybridSkillLevel mTotalpoints["<<index<<"] = "<<mTotalpoints[index]<<" ");

    }
}

void FifaEloRpgHybridSkill::updateOpponentEloRpgHybridSkillLevel(Stats::UpdateRowKey key, UpdateRowKeyList keys, StatValueUtil::StatUpdateMap* statUpdateMap)
{
    if (statUpdateMap != NULL)
    {
        StatValueUtil::StatUpdateMap::iterator updateIter, updateEnd, oppoUpdateIter;
        updateIter = statUpdateMap->find(key);
        updateEnd = statUpdateMap->end();
        if (updateIter == updateEnd)
        {
            WARN_LOG("[ReportProcessor].updateOpponentEloRpgHybridSkillLevel(): Cannot find key");
            return;
        }

        StatValueUtil::StatUpdate* statUpdate = &(updateIter->second);

        StatValueUtil::StatValueMap* statValueMap = &(statUpdate->stats);

        StatValueUtil::StatValueMap::iterator statTotalOppoLevelIter = statValueMap->find(STATNAME_OPPONENTLEVEL);
        if (statTotalOppoLevelIter == statValueMap->end())
        {
            WARN_LOG("[ReportProcessor].updateOpponentEloRpgHybridSkillLevel(): Cannot find statOppLevelName "<<STATNAME_OPPONENTLEVEL<<" ");
            return;
        }
        StatValueUtil::StatValue* statTotalOppoLevel = &(statTotalOppoLevelIter->second);

        UpdateRowKeyList::const_iterator keyIter, keyEnd;
        keyEnd = keys.end();

        {
            int64_t oppoTotalStatLevel = 0;

            keyIter = keys.begin();
            for (; keyIter != keyEnd; ++keyIter)
            {
                int32_t period = keyIter->period;
                oppoUpdateIter = statUpdateMap->find(*keyIter);
                if (oppoUpdateIter != updateEnd && updateIter != oppoUpdateIter && period == key.period)
                {
                    // Find the opponents stats update
                    StatValueUtil::StatUpdate* oppoStatUpdate = &(oppoUpdateIter->second);

                    StatValueUtil:: StatValueMap* oppoStatValueMap = &(oppoStatUpdate->stats);
                    StatValueUtil:: StatValueMap::iterator oppoStatLevelIter = oppoStatValueMap->find(STATNAME_LEVEL);
                    if (oppoStatLevelIter == oppoStatValueMap->end())
                    {
                        WARN_LOG("[ReportProcessor].updateOpponentEloRpgHybridSkillLevel(): Cannot find statLevelName "<<STATNAME_LEVEL<<" for opponent");
                        continue;
                    }

                    StatValueUtil::StatValue* oppoStatLevelValue = &(oppoStatLevelIter->second);
                    oppoTotalStatLevel += oppoStatLevelValue->iBefore[period];
                }
            }

			int32_t period = key.period;
			TRACE_LOG("[ReportProcessor:"<<this<<"].updateOpponentEloRpgHybridSkillLevel adding oppolevel: "<<oppoTotalStatLevel<<" ");
            statTotalOppoLevel->iAfter[period] = statTotalOppoLevel->iBefore[period] + oppoTotalStatLevel;
        }
    }
}

void FifaEloRpgHybridSkill::updateStarLevel(Stats::UpdateRowKey key, StatValueUtil::StatUpdateMap* statUpdateMap)
{
	if (statUpdateMap != NULL)
	{
		StatValueUtil::StatUpdateMap::iterator updateIter, updateEnd;
		updateIter = statUpdateMap->find(key);
		updateEnd = statUpdateMap->end();
		if (updateIter == updateEnd)
		{
			WARN_LOG("[ReportProcessor].updateStarLevel(): Cannot find key");
			return;
		}

		StatValueUtil::StatUpdate* statUpdate = &(updateIter->second);

		StatValueUtil::StatValueMap* statValueMap = &(statUpdate->stats);

		StatValueUtil::StatValueMap::iterator statSkillLevelIter = statValueMap->find(STATNAME_LEVEL);
		if (statSkillLevelIter == statValueMap->end())
		{
			WARN_LOG("[ReportProcessor].updateStarLevel(): Cannot find statSkillLevelIter "<<STATNAME_LEVEL<<" ");
			return;
		}

		StatValueUtil::StatValueMap::iterator statStarLevelIter = statValueMap->find(STATNAME_STAR_LEVEL);
		if (statStarLevelIter == statValueMap->end())
		{
			WARN_LOG("[ReportProcessor].updateStarLevel(): Cannot find statStarLevelIter "<<STATNAME_STAR_LEVEL<<" ");
			return;
		}

		StatValueUtil::StatValue* statSkillLevelValue = &(statSkillLevelIter->second);
		StatValueUtil::StatValue* statStarLevelValue = &(statStarLevelIter->second);

		int32_t period = key.period;
		{
			int64_t skillLevel = statSkillLevelValue->iAfter[period];
			int64_t starLevel = 0;

			TRACE_LOG("[ReportProcessor].skillLevel = "<<skillLevel<<" period = "<<period<<" ");
			for (int32_t i = 0; i < NUM_STAR_LEVELS; ++i)
			{
				if (skillLevel < STAR_LEVELS[i])
				{
					break;
				}
				else
				{
					++starLevel;
					TRACE_LOG("[ReportProcessor].starLevel incremented = "<<starLevel<<" ");
				}
			}

			statStarLevelValue->iAfter[period] = starLevel;
			TRACE_LOG("[ReportProcessor].starLevel = "<<starLevel<<" ");
		}

		TRACE_LOG("[ReportProcessor].updateStarLevel = "<<statStarLevelValue->iAfter[Stats::STAT_PERIOD_ALL_TIME]<<" ");
	}
}

void FifaEloRpgHybridSkill::preSetupForStatCalc(UpdateRowKeyList& keyList)
{
	UpdateRowKeyList::iterator iter = keyList.begin();
	UpdateRowKeyList::iterator end = keyList.end();

	for (; iter != end; iter++)
	{
		Stats::UpdateRowKey key = *iter;
		
		if(mExtension->isClubs() && key.period == 0) // only do for one period.
		{
			Collections::AttributeMap currentMap;
			mExtension->generateAttributeMap(key.entityId, currentMap);
			Collections::AttributeMap* map = &currentMap;

			int32_t teamId = StatValueUtil::getInt(map, ATTRIBNAME_TEAM);
			bool isHome = StatValueUtil::getInt(map, ATTRIBNAME_HOME) == 1;
			int32_t numPlayers = StatValueUtil::getInt(map, ATTRIBNAME_PLAYERS);
			int index = isHome ? SIDE_HOME : SIDE_AWAY;

			mClubPlayerCnt[index].mTeamID = teamId;
			mClubPlayerCnt[index].mPlayerCnt = numPlayers;
		}
	}
}

/*! \brief grab the StatCategorySummary from list */
const Stats::StatCategorySummary* FifaEloRpgHybridSkill::findInStatCategoryList(const char8_t* categoryName,
	const Stats::StatCategoryList::StatCategorySummaryList& categories) const
{
	if ((categoryName == NULL) || (categoryName[0] == '\0'))
		return NULL;
	Blaze::Stats::StatCategoryList::StatCategorySummaryList::const_iterator iter = categories.begin();
	Blaze::Stats::StatCategoryList::StatCategorySummaryList::const_iterator end = categories.end();
	for (; iter != end; ++iter)
	{
		if (blaze_strcmp(categoryName, (*iter)->getName()) == 0)
			return *iter;
	}
	return NULL;
}

} //namespace GameReporting
} //namespace Blaze
