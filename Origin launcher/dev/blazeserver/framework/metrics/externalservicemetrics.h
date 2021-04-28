#ifndef EXTERNAL_SERVICE_METRICS_H
#define EXTERNAL_SERVICE_METRICS_H

#include <EASTL/hash_set.h>
#include "framework/metrics/metrics.h"
#include "framework/component/component.h"

namespace Blaze {

    // struct used to track the metrics information for communication with external services
    struct ExternalServiceMetrics
    {
    private:
        
        Metrics::TaggedCounter<Metrics::ExternalServiceName, Metrics::CommandUri> mCallsStarted;
        Metrics::TaggedCounter<Metrics::ExternalServiceName, Metrics::CommandUri> mCallsFinished;
        Metrics::TaggedCounter<Metrics::ExternalServiceName, Metrics::CommandUri> mRequestsSent;
        Metrics::TaggedCounter<Metrics::ExternalServiceName, Metrics::CommandUri> mRequestsFailed;
        Metrics::TaggedCounter<Metrics::ExternalServiceName, Metrics::CommandUri, Metrics::ResponseCode> mResponseCount;
        Metrics::TaggedCounter<Metrics::ExternalServiceName, Metrics::CommandUri, Metrics::ResponseCode> mResponseTime;

        void addStatusInfo(ComponentStatus::InfoMap& statusMap, const Metrics::TagPairList& tagList, const char* name, uint64_t value, bool bHasResponseCode) const;
        
    public:
        ExternalServiceMetrics(Metrics::MetricsCollection& collection)
            : mCallsStarted(collection, "ExtService.callsStarted", Metrics::Tag::external_service_name, Metrics::Tag::command_uri)
            , mCallsFinished(collection, "ExtService.callsFinished", Metrics::Tag::external_service_name, Metrics::Tag::command_uri)
            , mRequestsSent(collection, "ExtService.requestsSent", Metrics::Tag::external_service_name, Metrics::Tag::command_uri)
            , mRequestsFailed(collection, "ExtService.requestsFailed", Metrics::Tag::external_service_name, Metrics::Tag::command_uri)
            , mResponseCount(collection, "ExtService.responseCount", Metrics::Tag::external_service_name, Metrics::Tag::command_uri, Metrics::Tag::response_code)
            , mResponseTime(collection, "ExtService.responseTime", Metrics::Tag::external_service_name, Metrics::Tag::command_uri, Metrics::Tag::response_code)
        {
            //this sets MetricBase::FLAG_SUPPRESS_GARBAGE_COLLECTION
            mCallsStarted.getTotal(); //to suppress garbage collection of this tagged counter
            mCallsFinished.getTotal(); //to suppress garbage collection of this tagged counter
            mRequestsSent.getTotal(); //to suppress garbage collection of this tagged counter
            mRequestsFailed.getTotal(); //to suppress garbage collection of this tagged counter
            mResponseCount.getTotal(); //to suppress garbage collection of this tagged counter
            mResponseTime.getTotal(); //to suppress garbage collection of this tagged counter
        }

        void getStatusInfo(ComponentStatus& status) const;
        
        void incCallsStarted(const char* serviceName, const char* commandUri);
        void incCallsFinished(const char* serviceName, const char* commandUri);
        void incRequestsSent(const char* serviceName, const char* commandUri);
        void incRequestsFailed(const char* serviceName, const char* commandUri);
        void incResponseCount(const char* serviceName, const char* commandUri, const char* responseCode);
        void recordResponseTime(const char* serviceName, const char* commandUri, const char* responseCode, uint64_t value);
    };

}
#endif