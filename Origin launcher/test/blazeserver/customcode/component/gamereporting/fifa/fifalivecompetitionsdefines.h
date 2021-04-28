/*************************************************************************************************/
/*!
    \file   fifalivecompetitionsdefines.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FIFALIVECOMP_DEFINES_H
#define BLAZE_CUSTOM_FIFALIVECOMP_DEFINES_H

#include "component/stats/tdf/stats.h"

namespace Blaze
{
namespace GameReporting
{

	const int INVALID_SPONSORED_EVENT_ID		= 0;
	
	const int UNIQUE_TEAM_ID_KEYSCOPE_VAL		= 200001;	// used for live competitions that don't force the user to lock to a team
															// in this case the value doesn't matter so we can use any value in the keyscope range
															// blaze does not let us use the aggregate value so there will always be 2 rows for each user in a competition

	const int LIVE_COMP_CONFIG_MAX_NAME_LEN		= 64;

#define LIVE_COMP_CONFIG_SECTION_PREFIX		"FIFA_LIVE_COMP_SEASONALPLAY_"

#define LIVE_COMP_SP_OVERALL_CATEGORY		"LiveCompSPOverallStats"
#define LIVE_COMP_SP_DIVISIONAL_CATEGORY	"LiveCompSPDivisionalStats"
#define LIVE_COMP_SP_DIVCOUNT_CATEGORY		"LiveCompSPDivisionCount"
#define LIVE_COMP_NORMAL_STATS_CATEGORY		"LiveCompNormalStats"

#define LIVE_COMP_CATEGORY_SUFFIX_ALLTIME	"AllTime"
#define LIVE_COMP_CATEGORY_SUFFIX_MONTHLY	"Monthly"
#define LIVE_COMP_CATEGORY_SUFFIX_WEEKLY	"Weekly"
#define LIVE_COMP_CATEGORY_SUFFIX_DAILY		"Daily"

	static const char* GetLCCategorySuffixFromPeriod(Stats::StatPeriodType period)
	{
		const char * rez = "";
		switch(period)
		{
		case Stats::STAT_PERIOD_ALL_TIME:
			rez = LIVE_COMP_CATEGORY_SUFFIX_ALLTIME;
			break;
		case Stats::STAT_PERIOD_MONTHLY:
			rez = LIVE_COMP_CATEGORY_SUFFIX_MONTHLY;
			break;
		case Stats::STAT_PERIOD_WEEKLY:
			rez = LIVE_COMP_CATEGORY_SUFFIX_WEEKLY;
			break;
		case Stats::STAT_PERIOD_DAILY:
			rez = LIVE_COMP_CATEGORY_SUFFIX_DAILY;
			break;
		default:
			break;
		}
		return rez;
	}

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FIFALIVECOMP_DEFINES_H