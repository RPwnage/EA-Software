/*************************************************************************************************/
/*!
\file   futgamereportcollator.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_CUSTOM_FUTGAMEREPORT_COLLATOR_H
#define BLAZE_CUSTOM_FUTGAMEREPORT_COLLATOR_H

/*** Include files *******************************************************************************/
#include "gamereporting/basicgamereportcollator.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace GameReporting
{

struct FUTGameReportingStats
{
	FUTGameReportingStats()
		:mismatch(0)
	{}

	int64_t mismatch;
};

/*!
class FUTGameReportCollator

Extension of the basic game collator.  The goals of the FUT game collator as a follows.
1. Do not throw away game report data
2. Compare and report the differences to the game report slave
*/
class FUTGameReportCollator : public BasicGameReportCollator
{
public:
	FUTGameReportCollator(ReportCollateData& gameReport, GameReportingSlaveImpl& component);
	static GameReportCollator* create(ReportCollateData& gameReport, GameReportingSlaveImpl& component);

	virtual ~FUTGameReportCollator();

	ReportResult reportSubmitted(BlazeId playerId, const GameReport& report, const EA::TDF::Tdf* privateReport, GameReportPlayerFinishedStatus finishedStatus, ReportType reportType) override;
	CollatedGameReport& finalizeCollatedGameReport(ReportType collatedReportType) override;

private:

	eastl::map<BlazeId, FUTGameReportingStats> mReportRating;

};

}   // namespace GameReporting
}   // namespace Blaze

#endif  // BLAZE_CUSTOM_FUTGAMEREPORT_COLLATOR_H
