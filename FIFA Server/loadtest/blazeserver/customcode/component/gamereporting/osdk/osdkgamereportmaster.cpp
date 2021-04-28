/*************************************************************************************************/
/*!
    \file   osdkgamereportmaster.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"
#include "osdkgamereportmaster.h"
#include "gamereportingslaveimpl.h"
#include "gamereportcompare.h"

#include "gamereporting/tdf/gamereportingevents_server.h"
#include "framework/event/eventmanager.h"
#include "util/collatorutil.h"

#include "EAStdC/EAString.h"


namespace Blaze
{

namespace GameReporting
{

//! Called when collation has finalized. If this is an arena game and there were no reports,
//! submit a DNF report to Virgin Gaming.
GameReporting::CollatedGameReport& OsdkGameReportMaster::finalizeCollatedGameReport(ReportType collatedReportType)
{
    GameReporting::CollatedGameReport& collatedReport = BasicGameReportCollator::finalizeCollatedGameReport(collatedReportType);

    if (static_cast<uint32_t>(collatedReport.getError()) == GAMEREPORTING_COLLATION_ERR_NO_REPORTS)
    {
        submitDnfReport();
    }

    return collatedReport;
}

//! Called when the game is finished. Store the game info so that we can reference it later.
GameReportCollator::ReportResult OsdkGameReportMaster::gameFinished(const GameInfo& gameInfo)
{
    gameInfo.copyInto(mCachedGameInfo);

    // call the superclass's implementation of this so that we don't lose any
    // functionality by overriding it
    return BasicGameReportCollator::gameFinished(gameInfo);
}

//! Report timeout triggered. If it is a NO_REPORTS situation, send a DNF report.
GameReportCollator::ReportResult OsdkGameReportMaster::timeout() const
{
    GameReportCollator::ReportResult result = BasicGameReportCollator::timeout();
    
    if (GameReportCollator::RESULT_COLLATE_NO_REPORTS == result)
    {
        submitDnfReport();
    }

    return result;
}

//! This was a double DNF, so if it was an arena game, send it to the provider.
void OsdkGameReportMaster::submitDnfReport() const
{
	// Arena games are removed.
}

} // end namespace GameReporting
} // end namespace Blaze
