/*************************************************************************************************/
/*!
\file   fut_utility.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FUT_UTILITY_H
#define BLAZE_CUSTOM_FUT_UTILITY_H

namespace Blaze
{
namespace GameReporting
{
namespace FUT
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

const char* MATCHREPORTING_FUTMatchReportStats = "FUTMatchReportStats";
const char* MATCHREPORTING_FUTReputationStats = "FUTReputationStats";
const char* MATCHREPORTING_gameType = "gameType";

const char* MATCHREPORTING_version = "version";
const char* MATCHREPORTING_total = "total";
const char* MATCHREPORTING_mismatch = "mismatch";
const char* MATCHREPORTING_singleReport = "singleReport";
const char* MATCHREPORTING_dnfMatchResult = "dnfMatchResult";
const char* MATCHREPORTING_critMissMatch = "critMissMatch";
const char* MATCHREPORTING_stdRating = "stdRating";
const char* MATCHREPORTING_cmpRating = "cmpRating";

// This enum should match the order defined in etc\component\stats\stats.cfg
// This is used to reference the columns by index
enum FUTMatchReportStatsColumns
{
	MATCHREPORTSTATS_IDX_VERSION = 0,
	MATCHREPORTSTATS_IDX_TOTAL,
	MATCHREPORTSTATS_IDX_MISMATCH,
	MATCHREPORTSTATS_IDX_SINGLEREPORT,
	MATCHREPORTSTATS_IDX_DNFMATCHRESULT,
	MATCHREPORTSTATS_IDX_CRITMISSMATCH
};

enum FUTReputationStatsColumns
{
	REPUTATIONSTATS_IDX_VERSION = 0,
	REPUTATIONSTATS_IDX_STDRATING,
	REPUTATIONSTATS_IDX_CMPRATING
};

int32_t GetGameTypeId(const char8_t* gameTypeName);

}   // namespace FUT
}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FUT_UTILITY_H
