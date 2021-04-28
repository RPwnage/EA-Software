
#include "framework/blaze.h"
#include "framework/metrics/externalservicemetrics.h"

namespace Blaze {

    void ExternalServiceMetrics::addStatusInfo(ComponentStatus::InfoMap& infoMap, const Metrics::TagPairList& tagList, const char* metricName, uint64_t value, bool bHasResponseCode) const
    {
        char8_t key[256];
        
        if (bHasResponseCode && tagList.size() > 2)
        {
            blaze_snzprintf(key, sizeof(key), "ExtService_%s_%s_%s_%s", tagList[0].second.c_str(), tagList[1].second.c_str(), tagList[2].second.c_str(), metricName);
        }
        else
        {
            blaze_snzprintf(key, sizeof(key), "ExtService_%s_%s_%s", tagList[0].second.c_str(), tagList[1].second.c_str(), metricName);
        }
        char8_t buf[64];
        blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value);
        infoMap[key] = buf;
    }

    void ExternalServiceMetrics::getStatusInfo(ComponentStatus& status) const
    {
        Blaze::ComponentStatus::InfoMap& infoMap = status.getInfo();

        mCallsStarted.iterate([this, &infoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            addStatusInfo(infoMap, tagList, "STARTED", value.getTotal(), false);
        }
        );

        mCallsFinished.iterate([this, &infoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            addStatusInfo(infoMap, tagList, "FINISHED", value.getTotal(), false);
        }
        );

        mRequestsSent.iterate([this, &infoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            addStatusInfo(infoMap, tagList, "REQSENT", value.getTotal(), false);
        }
        );

        mRequestsFailed.iterate([this, &infoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            addStatusInfo(infoMap, tagList, "REQFAILED", value.getTotal(), false);
        }
        );

        mResponseCount.iterate([this, &infoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            addStatusInfo(infoMap, tagList, "COUNT", value.getTotal(), true);
        }
        );

        mResponseTime.iterate([this, &infoMap](const Metrics::TagPairList& tagList, const Metrics::Counter& value) {
            addStatusInfo(infoMap, tagList, "TIME", value.getTotal(), true);
        }
        );

    }

    void ExternalServiceMetrics::incCallsStarted(const char* serviceName, const char* commandUri)
    {
        mCallsStarted.increment(1, serviceName, commandUri);
    }

    void ExternalServiceMetrics::incCallsFinished(const char* serviceName, const char* commandUri)
    {
        mCallsFinished.increment(1, serviceName, commandUri);
    }
    
    void ExternalServiceMetrics::incRequestsSent(const char* serviceName, const char* commandUri) 
    {
        mRequestsSent.increment(1, serviceName, commandUri);
    }

    void ExternalServiceMetrics::incRequestsFailed(const char* serviceName, const char* commandUri)
    {
        mRequestsFailed.increment(1, serviceName, commandUri);
    }

    void ExternalServiceMetrics::incResponseCount(const char* serviceName, const char* commandUri, const char* responseCode) 
    {
        mResponseCount.increment(1, serviceName, commandUri, responseCode);
    }

    void ExternalServiceMetrics::recordResponseTime(const char* serviceName, const char* commandUri, const char* responseCode, uint64_t value)
    {
        mResponseTime.increment(value, serviceName, commandUri, responseCode);
    }
}