/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_SLIVERMETRICS_H
#define BLAZE_SLIVERMETRICS_H

/*** Include files *******************************************************************************/
#include "framework/metrics/metrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

#define ADD_METRIC(map, name, value) {                                              \
        map[(StringBuilder() << name).get()] = (StringBuilder() << (value)).get();  \
    }

#define ADD_METRIC_IF_SET(map, name, value) {                                       \
        StringBuilder valSb;                                                        \
        valSb << (value);                                                           \
        if (!valSb.isEmpty() && ((valSb.length() > 1) || (valSb.get()[0] != '0')))  \
            map[(StringBuilder() << name).get()] = valSb.get();                     \
    }

class SliverOwner;

struct SliverOwnerMetrics
{
    SliverOwnerMetrics(SliverOwner& owner);

    Metrics::MetricsCollection& mCollection;
    Metrics::Gauge mGaugeActiveImportOperations;
    Metrics::Counter mTotalImportsSucceeded;
    Metrics::Counter mTotalImportsFailed;
    Metrics::Timer mImportTimer;

    Metrics::Gauge mGaugeActiveExportOperations;
    Metrics::Counter mTotalExportsSucceeded;
    Metrics::Counter mTotalExportsFailed;
    Metrics::Timer mExportTimer;

    Metrics::PolledGauge mGaugeOwnedSliverCount;
    Metrics::PolledGauge mGaugeOwnedObjectCount;
};

} // Blaze

#endif // BLAZE_SLIVERMETRICS_H
