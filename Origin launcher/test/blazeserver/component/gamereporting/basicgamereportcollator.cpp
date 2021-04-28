/*************************************************************************************************/
/*!
    \file   basicgamereportcollator.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "basicgamereportcollator.h"
#include "util/collatorutil.h"
#include "gamereportcompare.h"
#include "gamereportingslaveimpl.h"
#include "util/psnmatchutil.h" //for PsnMatchUtil in timeout()

namespace Blaze
{
namespace GameReporting
{

BasicGameReportCollator::BasicGameReportCollator(ReportCollateData& gameReport, GameReportingSlaveImpl& component)
:   GameReportCollator(gameReport, component),
    mCollatedReport(nullptr),
    mCollectorUtil(gameReport.getCollectorData()),
    mFinishedGameInfo(gameReport.getGameInfo())
{
    if (gameReport.getCustomData() == nullptr)
    {
        gameReport.setCustomData(*(BLAZE_NEW CollatedGameReport));
    }

    if (gameReport.getCustomData()->getTdfId() == CollatedGameReport::TDF_ID)
    {
        mCollatedReport = static_cast<CollatedGameReport *>(gameReport.getCustomData());
        mCollatedReport->setGameId(getGameId());
    }
    // else it's the responsibility of any derived class to appropriately set my members (namely, mCollatedReport)!!!
}

GameReportCollator* BasicGameReportCollator::create(ReportCollateData& gameReport, GameReportingSlaveImpl& component)
{
    return BLAZE_NEW_NAMED("BasicGameReportCollator") BasicGameReportCollator(gameReport, component);
}

BasicGameReportCollator::~BasicGameReportCollator()
{
}

CollatedGameReport& BasicGameReportCollator::finalizeCollatedGameReport(ReportType /*collatedReportType*/) 
{
    //  define a collator utility that merges maps - meaning that players in source that are not in the target are merged in.
    CollatedGameReport::DnfStatusMap& dnfStatus = mCollatedReport->getDnfStatus();
    Utilities::Collator collator;

    ///////////////////////////////////////////////////////////////////////////
    //  CREATE COLLATED STAGE GAME REPORTS (midgame, finished and DNF)
    collator.setMergeMode(Utilities::Collator::MERGE_MAPS);

    //  generate collated mid-game report
    GameReport collatedFinalReport;
    bool baselineReport = false;

    if (!usesTrustedGameReporting())
    {
        //  update the dnf status map based on player info map
        GameInfo::PlayerInfoMap::const_iterator playerIt, playerEnd;
        playerIt = mFinishedGameInfo.getPlayerInfoMap().begin();
        playerEnd = mFinishedGameInfo.getPlayerInfoMap().end();
        for (; playerIt != playerEnd; ++playerIt)
        {
            GameManager::PlayerId playerId = playerIt->first;
            dnfStatus[playerId] = !playerIt->second->getFinished();
        }
    }

    //  go through all finished reports and merge them into a single post-game report.    
    Utilities::Collector::CategorizedGameReports midGameReports;
    mCollectorUtil.getGameReportsByCategory(midGameReports, Utilities::Collector::REPORT_TYPE_GAME_MIDGAME);
    if (!midGameReports.empty())
    {        
        Utilities::Collector::CategorizedGameReports::const_iterator reportIt = midGameReports.begin();
        Utilities::Collector::CategorizedGameReports::const_iterator reportItEnd = midGameReports.end();
        for (; reportIt != reportItEnd; ++reportIt)
        {
            GameReport *report = reportIt->second;
          
            if (!baselineReport)
            {
                //  construct the baseline report.
                report->copyInto(collatedFinalReport);
                baselineReport = true;
            }
            else
            {
                //  compare baseline with this report - if different then report error.
                GameReportCompare compare;
                const GameType* gameType = getComponent().getGameTypeCollection().getGameType(report->getGameReportName());
                if (!compare.compare(*gameType, *report->getReport(), *collatedFinalReport.getReport()))
                {
                    mCollatedReport->setError(Blaze::GAMEREPORTING_COLLATION_REPORTS_INCONSISTENT);
                    return *mCollatedReport;
                }
                else if (getComponent().getConfig().getBasicConfig().getMergeReports())
                {
                    collator.merge(collatedFinalReport, *report);
                }
            }
        }

        //  clear collected trusted mid game reports after collation so that the
        //  processed mid game reports will not be used again in the next collation.
        if (usesTrustedGameReporting())
        {
            mCollectorUtil.clearGameReportsByCategory(Utilities::Collector::REPORT_TYPE_GAME_MIDGAME);
        }
    }

    // generate finished player report.
    GameReport collatedFinishedReport;
    bool baselineFinishedReport = false;   

    //  go thorough all finished reports and merge them into a single post-game report.    
    Utilities::Collector::CategorizedGameReports finishedGameReports;
    mCollectorUtil.getGameReportsByCategory(finishedGameReports, Utilities::Collector::REPORT_TYPE_GAME_FINISHED);
    if (!finishedGameReports.empty())
    {        
        Utilities::Collector::CategorizedGameReports::const_iterator reportIt = finishedGameReports.begin();
        Utilities::Collector::CategorizedGameReports::const_iterator reportItEnd = finishedGameReports.end();
        for (; reportIt != reportItEnd; ++reportIt)
        {
            GameReport *report = reportIt->second;
            GameManager::PlayerId playerId = reportIt->first;

            if (dnfStatus.find(playerId) == dnfStatus.end())
            {
                dnfStatus[playerId] = false;
            }

            if (!baselineFinishedReport)
            {
                //  construct the baseline report.
                report->copyInto(collatedFinishedReport);
                baselineFinishedReport = true;
            }
            else
            {
                //  compare baseline with this report - if different then report error.
                GameReportCompare compare;
                const GameType* gameType = getComponent().getGameTypeCollection().getGameType(report->getGameReportName());
                if (!compare.compare(*gameType, *collatedFinishedReport.getReport(), *report->getReport()))
                {
                    mCollatedReport->setError(Blaze::GAMEREPORTING_COLLATION_REPORTS_INCONSISTENT);
                    return *mCollatedReport;
                }
                else if (getComponent().getConfig().getBasicConfig().getMergeReports())
                {
                    collator.merge(collatedFinishedReport, *report);
                }
            }
        }
    }

    //  collation requires at least one mid-game/finished report
    if (!baselineFinishedReport && !baselineReport)
    {
        if (forceDnfCollation())
        {
            INFO_LOG("[BasicGameReportCollator].finalizeCollatedGameReport() : collation requires at least one mid-game/finished report, continuing anyways ...");
        }
        else
        {
            mCollatedReport->setError(GAMEREPORTING_COLLATION_ERR_NO_REPORTS);
            return *mCollatedReport;
        }
    }

    //  generate collated dnf player report.
    GameReport collatedDnfReport;
    bool baselineDnfReport = false;

    Utilities::Collector::CategorizedGameReports dnfGameReports;
    mCollectorUtil.getGameReportsByCategory(dnfGameReports, Utilities::Collector::REPORT_TYPE_GAME_DNF);
    if (!dnfGameReports.empty())
    {
        Utilities::Collector::CategorizedGameReports::const_iterator reportIt = dnfGameReports.begin();
        Utilities::Collector::CategorizedGameReports::const_iterator reportItEnd = dnfGameReports.end();
        for (; reportIt != reportItEnd; ++reportIt)
        {
            GameReport *report = reportIt->second;
            GameManager::PlayerId playerId = reportIt->first;

            if (dnfStatus.find(playerId) == dnfStatus.end())
            {
                dnfStatus[playerId] = true;
            }

            if (!baselineDnfReport)
            {
                //  construct the baseline report.
                report->copyInto(collatedDnfReport);
                baselineDnfReport = true;
            }
            else
            {
                collator.merge(collatedDnfReport, *report);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    //  MERGE COLLATED STAGE REPORTS INTO FINAL REPORT
    collator.setMergeMode(Utilities::Collator::MERGE_MAPS_COPY);

    //  merge dnf report, overwriting overlapped entries in the current final (mid-game) report.
    if (baselineDnfReport)
    {
        if (baselineReport)
        {
            collator.merge(collatedFinalReport, collatedDnfReport);
        }
        else
        {
            collatedDnfReport.copyInto(collatedFinalReport);
            baselineReport = true;
        }
    }

    // Now merge the post-game finished reports into the DNF report (aka current final report.)
    if (baselineFinishedReport)
    { 
        if (baselineReport)
        {
            collator.merge(collatedFinalReport, collatedFinishedReport);
        }
        else
        {
            collatedFinishedReport.copyInto(collatedFinalReport);
            baselineReport = true;
        }
    }

    //  copy final report to class copy and return the class copy.
    collatedFinalReport.copyInto(mCollatedReport->getGameReport());

    if (!baselineReport)
    {
        WARN_LOG("[BasicGameReportCollator].finalizeCollatedGameReport() : No baseline report to return for game, reporting id = " << mFinishedGameInfo.getGameReportingId() << ".");
        mCollatedReport->setError(GAMEREPORTING_COLLATION_ERR_NO_REPORTS);
    }
    else
    {
        mCollatedReport->setError(ERR_OK);
    }

    return *mCollatedReport;
}

GameReportCollator::ReportResult BasicGameReportCollator::reportSubmitted(BlazeId playerId, const GameReport& report, const EA::TDF::Tdf* privateReport, GameReportPlayerFinishedStatus finishedStatus, ReportType reportType) 
{
    //    allow multiple mid-game trusted reports, otherwise allow no more than one report per user.
    if (mCollectorUtil.hasReport(playerId) && reportType != REPORT_TYPE_TRUSTED_MID_GAME && reportType != REPORT_TYPE_TRUSTED_END_GAME)
    {
        WARN_LOG("[BasicGameReportCollator].reportSubmitted() : Player " << playerId << " already has a report (report id=" << report.getGameReportingId() << ")");
        return RESULT_COLLATE_REJECT_DUPLICATE;
    }
    else
    {
        bool isDNF = false;
        bool isMidGame = mFinishedGameInfo.getGameReportingId() == GameManager::INVALID_GAME_REPORTING_ID;
        //  once game is finished, don't allow players who weren't in the game to submit reports.
        if (!isMidGame)
        {
            if (reportType != REPORT_TYPE_TRUSTED_MID_GAME && reportType != REPORT_TYPE_TRUSTED_END_GAME && !mCollectorUtil.isUserInGame(playerId, mFinishedGameInfo, isDNF))
            {
                return RESULT_COLLATE_REJECT_INVALID_PLAYER;
            }
        }

        uint32_t category;
        if (isDNF)
        {
            category = Utilities::Collector::REPORT_TYPE_GAME_DNF;
        }
        else if (isMidGame)
        {
            category = Utilities::Collector::REPORT_TYPE_GAME_MIDGAME; 
        }
        else 
        {
            category = Utilities::Collector::REPORT_TYPE_GAME_FINISHED;
        }

        //  override category if an explicit finished status is set.
        if (finishedStatus == GAMEREPORT_FINISHED_STATUS_FINISHED)
        {
            category = Utilities::Collector::REPORT_TYPE_GAME_FINISHED;
        }
        else if (finishedStatus == GAMEREPORT_FINISHED_STATUS_DNF)
        {
            category = Utilities::Collector::REPORT_TYPE_GAME_DNF; 
        }
        
        GameReporting::Utilities::Collector::CategorizedGameReports collectorReports;
        mCollectorUtil.getGameReportsByCategory(collectorReports, category);
        if (!collectorReports.empty())
        {
            GameReporting::Utilities::Collector::CategorizedGameReports::iterator itr = collectorReports.begin();
            if (blaze_strcmp(itr->second->getReport()->getFullClassName(), report.getReport()->getFullClassName()) != 0)
            {
                ASSERT_LOG("[BasicGameReportCollator].reportSubmitted(): submitted Tdf type failed due to incompatible TDF types - rejected submitted report:\n" <<
                    "  existing report is of type (" << itr->second->getReport()->getFullClassName() << "):\n" <<
                    SbFormats::PrefixAppender("    ", (StringBuilder() << itr->second->getReport()).get()) << "\n" <<
                    "  submitted report is of type (" << report.getReport()->getFullClassName() << "):\n" <<
                    SbFormats::PrefixAppender("    ", (StringBuilder() << report.getReport()).get()) << "\n"); 

                return RESULT_COLLATE_REJECTED;
            }
        }

        mCollectorUtil.addReport(playerId, report, category);

        if (privateReport != nullptr)
        {
            CollatedGameReport::PrivateReportsMap& privateReports = mCollatedReport->getPrivateReports();
            privateReports[playerId] = privateReports.allocate_element();
            privateReports[playerId]->setData(*(privateReport->clone()));
        }

        getCollectedGameReportsMap()[playerId] = getCollectedGameReportsMap().allocate_element();
        report.copyInto(*(getCollectedGameReportsMap()[playerId]));
    }

    //  trusted reports will always be processed immediately on submission.
    //  for other types of reports, submit report to slave when all needed reports have been submitted by other players.
    if (reportType == REPORT_TYPE_TRUSTED_MID_GAME || reportType == REPORT_TYPE_TRUSTED_END_GAME || mCollectorUtil.isFinished(mFinishedGameInfo))
    {
        return GameReportCollator::RESULT_COLLATE_COMPLETE;
    }

    return GameReportCollator::RESULT_COLLATE_CONTINUE;
}

GameReportCollator::ReportResult BasicGameReportCollator::gameFinished(const GameInfo& gameInfo)
{
    gameInfo.copyInto(mFinishedGameInfo);

    //  initialize collated report with bare minimum of data required by the slave when processing (even in an error case, this should be sent back.)
    mFinishedGameInfo.copyInto(mCollatedReport->getGameInfo());

    //  if there is a report and uses trusted game reporting, submit report for processing.
    if (usesTrustedGameReporting())
    {
        if (!mCollectorUtil.isEmpty())
        {
            return GameReportCollator::RESULT_COLLATE_COMPLETE;
        }
    }
    else
    {
        if (mCollectorUtil.isFinished(mFinishedGameInfo))
        {
            return GameReportCollator::RESULT_COLLATE_COMPLETE;
        }
    }

    return GameReportCollator::RESULT_COLLATE_CONTINUE;
}

GameReportCollator::ReportResult BasicGameReportCollator::timeout() const 
{
    if (mCollectorUtil.isEmpty())
    {
        if (!mFinishedGameInfo.getExternalSessionIdentification().getPs5().getMatch().getMatchIdAsTdfString().empty())
        {
            Utilities::PsnMatchUtilPtr pmu = BLAZE_NEW Utilities::PsnMatchUtil(getComponent(), REPORT_TYPE_STANDARD, getGameReportName(), mFinishedGameInfo);
            TRACE_LOG("[BasicGameReportCollator].timeout: canceling PSN Match(" << mFinishedGameInfo.getExternalSessionIdentification().getPs5().getMatch().getMatchId() << ") for game(" << getGameId() << "), gameReportingId(" << getGameReportingId() << ")");
            pmu->cancelMatch();//logged if err
        }
        return GameReportCollator::RESULT_COLLATE_NO_REPORTS;
    }

    return GameReportCollator::RESULT_COLLATE_COMPLETE;
}

void BasicGameReportCollator::postProcessCollatedReport()
{
    //  clear out collated game report so finalize can work with an empty base
    //  reuse finished game info if already specified.
    //  clear out collected reports.
    CollatedGameReport().copyInto(*mCollatedReport);
    mFinishedGameInfo.copyInto(mCollatedReport->getGameInfo());
    mCollectorUtil.clearReports();
    clearCollectedGameReportMap();
}

void BasicGameReportCollator::onGameTypeChanged()
{
    mCollectorUtil.clearReports();
    GameReportCollator::onGameTypeChanged();
}

}   // namespace GameReporting
}   // namespace Blaze
