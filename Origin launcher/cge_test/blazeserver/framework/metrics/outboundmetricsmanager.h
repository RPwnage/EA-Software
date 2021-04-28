/*************************************************************************************************/
/*!
    \file outboundmetricsmanager.h

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDMETRICSMANAGER_H
#define BLAZE_OUTBOUNDMETRICSMANAGER_H

#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/util/inputfilter.h"

namespace Blaze
{
    class OutboundMetricsEntryWithFilters :
        public OutboundMetricsEntry

    {
    public:
        OutboundMetricsEntryWithFilters() {}
        ~OutboundMetricsEntryWithFilters() override {}

    private:
        friend class OutboundMetricsManager;

        InputFilter& getFilter() { return mInputFilter; }

        InputFilter mInputFilter;
    };

    class OutboundMetricsManager
    {

    public:

        OutboundMetricsManager();
        virtual ~OutboundMetricsManager();

        bool configure(const OutboundMetricsConfig& config);
        void reconfigure(const OutboundMetricsConfig& config);
        void validateConfig(const OutboundMetricsConfig& config, ConfigureValidationErrors& validationErrors) const;

        void tickAndCheckThreshold(OutboundTransactionType txnType, const char* resourceStr, EA::TDF::TimeValue responseTime);
        void filloutOutboundMetrics(OutboundMetrics& response) const;

    private:

        typedef eastl::hash_map<eastl::string, OutboundMetricsEntryWithFilters> OutboundMetricsByResourceMap;
        typedef eastl::hash_map<OutboundTransactionType, OutboundMetricsByResourceMap, eastl::hash<uint32_t> > OutboundMetricsByTxnTypeMap;

        bool isInitialized() const { return mConfig != nullptr; }
        void trackResource(OutboundTransactionType txnType, const char* resourceStr, EA::TDF::TimeValue threshold, OutboundMetricsByTxnTypeMap& map);

        OutboundMetricsByTxnTypeMap* mOutboundMetricsMap;
        const OutboundMetricsConfigMap* mConfig;
    };

extern EA_THREAD_LOCAL OutboundMetricsManager* gOutboundMetricsManager;
}

#endif
