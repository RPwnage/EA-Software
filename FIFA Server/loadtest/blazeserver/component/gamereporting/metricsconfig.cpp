/*************************************************************************************************/
/*!
    \file   metricsconfig.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#include "framework/blaze.h"

#include "gamereporting/tdf/gamereporting_server.h"
#include "metricsconfig.h"
#include "gamereporting/gamereportingmetrics.h"

namespace Blaze
{
namespace GameReporting
{

HistoricMetrics::HistoricMetrics(Metrics::MetricsCollection& collection, const char8_t* name)
    : mName(name)
    , mMetric(collection, (eastl::string("Average_") + name).c_str(), [this]() -> uint64_t { return getAverage(); })
{
}

uint64_t HistoricMetrics::getAverage() const
{
    uint64_t avg = 0;
    size_t sz = mValues.size();
    if (sz > 0)
    {
        uint64_t total = 0;
        for (auto value : mValues)
            total += value;
        avg = total / sz;
    }
    return avg;
}

GameReportingMetricsCollection::~GameReportingMetricsCollection()
{
}

bool GameReportingMetricsCollection::init(Metrics::MetricsCollection& collection, const MetricsInfoConfig& metricsConfigMap)
{
    mReportsToStore = metricsConfigMap.getReportsToStore();

    for(auto& it : metricsConfigMap.getStoreAverage())
    {
        const char8_t* attribName = it->getName();

        if (attribName != nullptr && attribName[0] != '\0')
        {
            mAverageMetrics.emplace_back(BLAZE_NEW HistoricMetrics(collection, attribName));
        }
    }

    for(auto& it : metricsConfigMap.getStoreByValue())
    {
        const char8_t* attribName = it->getName();

        if (attribName != nullptr && attribName[0] != '\0')
        {
            char8_t metricName[256];
            blaze_snzprintf(metricName, sizeof(metricName), "Total_%s", attribName);

            mTotalMetrics.insert(eastl::make_pair(metricName, eastl::unique_ptr<Metrics::TaggedCounter<int64_t>>( BLAZE_NEW Metrics::TaggedCounter<int64_t>(collection, metricName, Metrics::Tag::game_report_attr_value))));
        }
    }

    return true;
}

void GameReportingMetricsCollection::updateMetrics(const char8_t* attributeName, int64_t attributeValue)
{
    char8_t metricName[128];

    blaze_snzprintf(metricName, sizeof(metricName), "Total_%s", attributeName);

    TotalMetricsMap::iterator metricIter = mTotalMetrics.find(metricName);
    if (metricIter != mTotalMetrics.end())
    {
        metricIter->second->increment(1, attributeValue);
    }
   
    for(auto& metrics : mAverageMetrics)
    {
        if (blaze_stricmp(metrics->mName.c_str(), attributeName)==0)
        {
            metrics->mValues.push_back(attributeValue);
            while (metrics->mValues.size() > mReportsToStore)
            {
                metrics->mValues.pop_front();
            }
            break;
        }
    }
}

void GameReportingMetricsCollection::addMetricsToStatusMap(ComponentStatus::InfoMap& gamereportingStatusMap) const
{

    for(auto& metrics : mAverageMetrics)
    {
        char8_t buf[64];
        blaze_snzprintf(buf, sizeof(buf), "%" PRId64, metrics->mMetric.get());
        gamereportingStatusMap[metrics->mMetric.getName().c_str()] = buf;
    }

    for(auto& metric : mTotalMetrics)
    {
        const char8_t* metricName = metric.second->getName().c_str();
        metric.second->iterate([&gamereportingStatusMap, &metricName](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
                char8_t nameBuf[128];
                blaze_snzprintf(nameBuf, sizeof(nameBuf), "%s[%s]", metricName, tagList[0].second.c_str());
                char8_t buf[64];
                blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value.getTotal());
                gamereportingStatusMap[nameBuf] = buf;

            });
    }
}

}
}

