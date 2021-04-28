/*************************************************************************************************/
/*!
\file
gamereportingmetrics.h

\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_GAMEREPORTING_METRICS_H
#define BLAZE_GAMEREPORTING_METRICS_H

/*** Include files *******************************************************************************/
#include "gamereporting/tdf/gamereporting_server.h"
#include "framework/metrics/metrics.h"

namespace Blaze
{

namespace Metrics
{
    namespace Tag
    {
        extern TagInfo<GameReporting::ReportType>* report_type;
        extern TagInfo<const char8_t*>* game_report_name;
        extern TagInfo<int64_t>* game_report_attr_value;
    }
}
namespace GameReporting
{

class GameReportingSlaveImpl;

/*
    For a full description of the metrics defined for Gamereporting health checks, see Blaze documentation on Confluence.
*/
struct GameReportingSlaveMetrics
{
    //reset counter upon creation
    GameReportingSlaveMetrics(Metrics::MetricsCollection& collection, GameReportingSlaveImpl& component);

    Metrics::Counter mGames; // non-offline games started
    Metrics::Counter mOfflineGames; // offline games reported

    Metrics::Counter mReportQueueDelayMs;
    Metrics::Counter mReportProcessingDelayMs;

    // game report submission metrics (only applies to non-offline game types)
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mReportsReceived;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mReportsRejected;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mDuplicateReports;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mReportsForInvalidGames;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mReportsFromPlayersNotInGame;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mReportsAccepted;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mReportsExpected;

    // game report submission metrics (for all game types)
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mValidReports;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mInvalidReports;

    // game report collating metrics (only applies to non-offline game types)
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mGamesCollatedSuccessfully;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mGamesWithDiscrepencies;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mGamesWithNoValidReports;

    // game report processing metrics (for all game types, i.e., processing offline game reports and non-offline collated game reports)
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mGamesProcessed;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mGamesProcessedSuccessfully;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mGamesWithProcessingErrors;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mGamesWithStatUpdateFailure;

    Metrics::Gauge mGaugeActiveProcessReportCount;

    // game report Stats Service metrics (for all game types)
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mGamesWithStatsServiceUpdates;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mStatsServiceUpdateSuccesses;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mStatsServiceUpdateQueueFails; // unable to queue request (likely because the job queue is full)
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mStatsServiceUpdateSetupFails; // request won't be attempted
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mStatsServiceUpdateErrorResponses;
    Metrics::TaggedCounter<ReportType, Metrics::GameReportName> mStatsServiceUpdateRetries;

    Metrics::Gauge mActiveStatsServiceUpdates; // number of actively processing game reports with Stats Service updates, not individual Stats Service RPCs
    Metrics::PolledGauge mActiveStatsServiceRpcs; // number of individual Stats Service RPCs including queued
};

} // GameReporting
} // Blaze

#endif // BLAZE_GAMEREPORTING_METRICS_H
