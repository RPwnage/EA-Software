/*************************************************************************************************/
/*!
    \file   basicstatsservicegamereportprocessor.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/event/eventmanager.h"

#include "basicstatsservicegamereportprocessor.h"
#include "gamereportingslaveimpl.h"

#include "gamereporting/util/achievementsutil.h"
#include "gamereporting/util/reportparserutil.h"
#include "gamereporting/util/statsserviceutil.h"
//#include "stats/updatestatsrequestbuilder.h"
//#include "stats/updatestatshelper.h"

namespace Blaze
{
namespace GameReporting
{

BasicStatsServiceGameReportProcessor::BasicStatsServiceGameReportProcessor(GameReportingSlaveImpl& component)
    : GameReportProcessor(component)
{
}

BasicStatsServiceGameReportProcessor::~BasicStatsServiceGameReportProcessor()
{
}

GameReportProcessor* BasicStatsServiceGameReportProcessor::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("BasicStatsServiceGameReportProcessor") BasicStatsServiceGameReportProcessor(component);
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted for collation or direct to processing
    for offline or trusted reports.   Behavior depends on the calling RPC.

    \param report Incoming game report from submit request
    \return ERR_OK on success.  GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError BasicStatsServiceGameReportProcessor::validate(GameReport& report) const
{
    return ERR_OK;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.

    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success.  GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError BasicStatsServiceGameReportProcessor::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    GameReport& report = processedReport.getGameReport();

    const GameType& gameType = processedReport.getGameType();

    bool success = false;
    BlazeRpcError processErr = Blaze::ERR_OK;

    //  extract and set stats
    Utilities::ReportParser reportParser(gameType, processedReport, &playerIds, &mComponent.getConfig().getEventTypes());

    TRACE_LOG("[BasicStatsServiceGameReportProcessor].process: Stats Service start");
    Utilities::StatsServiceUtilPtr ssu = BLAZE_NEW Utilities::StatsServiceUtil(mComponent, processedReport.getReportType(), processedReport.getGameReport().getGameReportName());
    reportParser.setStatsServiceUtil(ssu);
    success = reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS_SERVICE);
    if (!success)
    {
        WARN_LOG("[BasicStatsServiceGameReportProcessor].process: failed to parse Stats Service section");
        processErr = reportParser.getErrorCode();
    }
    else
    {
        processErr = ssu->sendUpdatesAndWait();

        if (processErr != Blaze::ERR_OK)
        {
            WARN_LOG("[BasicStatsServiceGameReportProcessor].process: error sending Stats Service update: " << ErrorHelp::getErrorName(processErr));
            success = false;
        }
        else
        {
            // should have a response for each request
            for (auto& itr : ssu->getUpdateStatsServiceMap())
            {
                eadp::stats::UpdateEntityStatsRequest* request = itr.second.request;
                Grpc::ResponsePtr& responsePtr = itr.second.responsePtr;
                if (request != nullptr && ((request->stats_size() > 0) || (request->dim_stats_size() > 0)))
                {
                    if (responsePtr == nullptr)
                    {
                        // integrators may need/want custom retry logic
                        // review log for StatsServiceUtil::sendUpdateEntityStats() to see if we received an error response or didn't receive a response at all
                        WARN_LOG("[BasicStatsServiceGameReportProcessor].process: did not receive success response for context: "
                            << itr.first.context << ", category: " << itr.first.category << ", entity: " << itr.first.entityId);
                        processErr = Blaze::ERR_SYSTEM;
                    }
                    // otherwise, we should only be tracking "success" responses
                }
                else
                {
                    if (responsePtr != nullptr)
                    {
                        // why do we have a response for a request that doesn't exist (or should not have been sent)?
                        WARN_LOG("[BasicStatsServiceGameReportProcessor].process: tracking unknown response (ignorable) for context: "
                            << itr.first.context << ", category: " << itr.first.category << ", entity: " << itr.first.entityId);
                    }
                }
            }
            if (processErr != Blaze::ERR_OK)
            {
                WARN_LOG("[BasicStatsServiceGameReportProcessor].process: unrecoverable validation result: " << ErrorHelp::getErrorName(processErr));
                success = false;
            }
            // otherwise, if there are only ignorable validation issues, then leaving it to integrators to handle with their own custom report processor
        }
    }
    TRACE_LOG("[BasicStatsServiceGameReportProcessor].process: Stats Service finish");
    // unlike with BasicGameReportProcessor (which considers Blaze Stats the authority), this report processor considers Stats Service the authority,
    // so we block and wait for the Stats Service result before proceeding with processing the other sections of the report

    if (success)
    {
        if (processedReport.needsHistorySave() && processedReport.getReportType() != REPORT_TYPE_TRUSTED_MID_GAME)
        {
            //  extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            reportParser.setGameHistoryReport(&historyReport);
            reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
        }

        const CustomEvents& allEvents = reportParser.getEvents();
        for (CustomEvents::const_iterator i = allEvents.begin(), e = allEvents.end();i != e; ++i)
        {
            CustomEvent& customEvent= *(i->second);
            gEventManager->submitEvent((uint32_t)GameReportingSlave::EVENT_GAMEREPORTCUSTOMEVENT, customEvent, true);
        }

        processedReport.updatePINReports();

#ifdef TARGET_achievements
        AchievementsUtil achievementUtil;
        reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_ACHIEVEMENTS);
        Utilities::ReportParser::GrantAchievementRequestList::const_iterator gaIter = reportParser.getGrantAchievementRequests().begin();
        for (; gaIter != reportParser.getGrantAchievementRequests().end(); ++gaIter)
        {
            Achievements::AchievementData result;
            achievementUtil.grantAchievement(**gaIter, result);
        }

        Utilities::ReportParser::PostEventsRequestList::const_iterator peIter = reportParser.getPostEventsRequests().begin();
        for (; peIter != reportParser.getPostEventsRequests().end(); ++peIter)
        {
            achievementUtil.postEvents(**peIter);
        }
#endif
    }

    return processErr;
}

}   // namespace GameReporting
}   // namespace Blaze
