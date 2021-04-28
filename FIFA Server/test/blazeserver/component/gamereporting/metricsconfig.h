/*************************************************************************************************/
/*!
    \file   metricsconfig.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_GAMEREPORTING_METRICS_CONFIG_H
#define BLAZE_GAMEREPORTING_METRICS_CONFIG_H

/*** Include files *******************************************************************************/
#include "EASTL/map.h"
#include "EASTL/list.h"
#include "framework/controller/controller.h"
#include "framework/util/shared/blazestring.h"
#include "framework/metrics/metrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace GameReporting
{
struct HistoricMetrics
{
    HistoricMetrics(Metrics::MetricsCollection& collection, const char8_t* name);
    uint64_t getAverage() const;

    eastl::string mName;
    Metrics::PolledGauge mMetric;
    eastl::list<uint64_t> mValues;
};

typedef eastl::vector<eastl::unique_ptr<HistoricMetrics>> MetricsSet;
typedef eastl::hash_map<eastl::string, eastl::unique_ptr<Metrics::TaggedCounter<int64_t> > > TotalMetricsMap;

class GameReportingMetricsCollection
{
public:
    GameReportingMetricsCollection() {};
    ~GameReportingMetricsCollection();

    bool init(Metrics::MetricsCollection& collection, const MetricsInfoConfig& metricsConfigMap);
    void updateMetrics(const char8_t* attributeName, int64_t attributeValue);
    void addMetricsToStatusMap(ComponentStatus::InfoMap& gamereportingStatusMap) const;

private:
    MetricsSet mAverageMetrics;
    TotalMetricsMap mTotalMetrics;

    uint32_t mReportsToStore;
};

}
}

#endif //BLAZE_GAMEREPORTING_METRICS_CONFIG_H

