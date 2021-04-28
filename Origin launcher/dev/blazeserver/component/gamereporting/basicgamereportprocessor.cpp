/*************************************************************************************************/
/*!
    \file   basicgamereportprocessor.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/event/eventmanager.h"

#include "basicgamereportprocessor.h"
#include "gamereportingslaveimpl.h"

#include "gamereporting/util/achievementsutil.h"
#include "gamereporting/util/psnmatchutil.h"
#include "gamereporting/util/reportparserutil.h"
#include "gamereporting/util/statsserviceutil.h"
#include "stats/updatestatsrequestbuilder.h"
#include "stats/updatestatshelper.h"

namespace Blaze
{
namespace GameReporting
{

BasicGameReportProcessor::BasicGameReportProcessor(GameReportingSlaveImpl& component)
    : GameReportProcessor(component)
{
}

BasicGameReportProcessor::~BasicGameReportProcessor()
{
}

GameReportProcessor* BasicGameReportProcessor::create(GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("BasicGameReportProcessor") BasicGameReportProcessor(component);
}

/*! ****************************************************************************/
/*! \brief Implementation performs simple validation and if necessary modifies 
        the game report.

    On success report may be submitted for collation or direct to processing
    for offline or trusted reports.   Behavior depends on the calling RPC.

    \param report Incoming game report from submit request
    \return ERR_OK on success.  GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError BasicGameReportProcessor::validate(GameReport& report) const
{
    return ERR_OK;
}

/*! ****************************************************************************/
/*! \brief Called when stats are reported following the process() call.

    \param processedReport Contains the final game report and information used by game history.
    \param playerIds list of players to distribute results to.
    \return ERR_OK on success.  GameReporting specific error on failure.
********************************************************************************/
BlazeRpcError BasicGameReportProcessor::process(ProcessedGameReport& processedReport, GameManager::PlayerIdList& playerIds)
{
    GameReport& report = processedReport.getGameReport();

    Stats::UpdateStatsRequestBuilder builder;
    const GameType& gameType = processedReport.getGameType();

    bool success = false;
    BlazeRpcError processErr = Blaze::ERR_OK;

    //  extract and set stats
    Utilities::ReportParser reportParser(gameType, processedReport, &playerIds, &mComponent.getConfig().getEventTypes());

    TRACE_LOG("[BasicGameReportProcessor].process: Stats Service start");
    Utilities::StatsServiceUtilPtr ssu = BLAZE_NEW Utilities::StatsServiceUtil(mComponent, processedReport.getReportType(), processedReport.getGameReport().getGameReportName());
    reportParser.setStatsServiceUtil(ssu);
    success = reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_STATS_SERVICE);
    if (success)
    {
        ssu->sendUpdates(); // (non-blocking) fire and forget
    }
    else
    {
        WARN_LOG("[BasicGameReportProcessor].process: failed to parse Stats Service section");
    }
    TRACE_LOG("[BasicGameReportProcessor].process: Stats Service finish");
    // regardless of the Stats Service outcome, proceed with processing the other sections of the report

    reportParser.setUpdateStatsRequestBuilder(&builder);
    success = reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_KEYSCOPES | Utilities::ReportParser::REPORT_PARSE_STATS | Utilities::ReportParser::REPORT_PARSE_EVENTS);

    if (success)
    {
        if (!builder.empty())
        {
            //  publish stats
            Stats::UpdateStatsHelper updateStatsHelper;

            bool strict = mComponent.getConfig().getBasicConfig().getStrictStatsUpdates();        
            processErr = updateStatsHelper.initializeStatUpdate(builder, strict);

            if (processErr == ERR_OK)
            {
                processErr = updateStatsHelper.commitStats();
            }
            success = (processErr == ERR_OK);
        }

        if (processedReport.needsHistorySave() && processedReport.getReportType() != REPORT_TYPE_TRUSTED_MID_GAME)
        {
            //  extract game history attributes from report.
            GameHistoryReport& historyReport = processedReport.getGameHistoryReport();
            reportParser.setGameHistoryReport(&historyReport);
            reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_GAME_HISTORY);
        }

        if (success)
        {
            const CustomEvents& allEvents = reportParser.getEvents();
            for (CustomEvents::const_iterator i = allEvents.begin(), e = allEvents.end();i != e; ++i)
            {
                CustomEvent& customEvent= *(i->second);
                gEventManager->submitEvent((uint32_t)GameReportingSlave::EVENT_GAMEREPORTCUSTOMEVENT, customEvent, true);
            }
        }

        processedReport.updatePINReports();

#ifdef TARGET_achievements
        if (success)
        {
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
        }
#endif

        const char8_t* matchId = "";
        if (processedReport.getGameInfo() != nullptr)
        {
            matchId = processedReport.getGameInfo()->getExternalSessionIdentification().getPs5().getMatch().getMatchId();
        }
        if (matchId[0] == '\0')
        {
            TRACE_LOG("[BasicGameReportProcessor].process: no associated PSN Match");
        }
        else
        {
            /// @todo [ps5-gr] async option ???  would need a copy of processed report if there's validation during parse (due to getMatchDetail call)
            // gameInfo can't be null if we're here
            TRACE_LOG("[BasicGameReportProcessor].process: PSN Match start (" << matchId << ")");
            Utilities::PsnMatchUtilPtr pmu = BLAZE_NEW Utilities::PsnMatchUtil(mComponent, processedReport.getReportType(), processedReport.getGameReport().getGameReportName(), *processedReport.getGameInfo());
            if (success)
            {
                reportParser.setPsnMatchUtil(pmu);
                success = reportParser.parse(*report.getReport(), Utilities::ReportParser::REPORT_PARSE_PSN_MATCH);
                if (success)
                {
                    // the util will log any errors
                    if (pmu->initializeReport() == ERR_OK)
                    {
                        pmu->submitReport();
                        // ignoring any return value since it won't affect core Blaze flow
                    }
                }
                else
                {
                    WARN_LOG("[BasicGameReportProcessor].process: failed to parse PSN Match section for match: " << matchId);
                }
            }
            else
            {
                TRACE_LOG("[BasicGameReportProcessor].process: canceling PSN Match (" << matchId << ")");
                pmu->cancelMatch();
            }
            TRACE_LOG("[BasicGameReportProcessor].process: PSN Match finish (" << matchId << ")");
        }

    }
    else
    {
        processErr = reportParser.getErrorCode();
    }

    return processErr;
}

}   // namespace GameReporting
}   // namespace Blaze
