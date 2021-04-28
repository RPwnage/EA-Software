/*************************************************************************************************/
/*!
    \file outboundmetricsmanager.cpp

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/metrics/outboundmetricsmanager.h"
#include "framework/controller/controller.h"
#include "framework/connection/outboundhttpservice.h"

namespace Blaze
{


OutboundMetricsManager::OutboundMetricsManager()
    : mOutboundMetricsMap(nullptr),
      mConfig(nullptr)
{
}


OutboundMetricsManager::~OutboundMetricsManager()
{
    if (mOutboundMetricsMap != nullptr)
    {
        delete mOutboundMetricsMap;
        mOutboundMetricsMap = nullptr;
    }
}

/*! ************************************************************************************************/
/*! \brief configure the http service

    \param[in] config = the config
    \return true if server can startup, false if error is fatal.
***************************************************************************************************/
bool OutboundMetricsManager::configure(const OutboundMetricsConfig& config)
{
    mConfig = &config.getOutboundMetrics();
    OutboundMetricsByTxnTypeMap* newOutboundMetricsMap = BLAZE_NEW OutboundMetricsByTxnTypeMap(BlazeStlAllocator("OutboundMetricsManager::OutboundMetricsMap"));

    OutboundMetricsConfigMap::const_iterator itr = config.getOutboundMetrics().begin();
    OutboundMetricsConfigMap::const_iterator end = config.getOutboundMetrics().end();

    // Add metrics tracking for specific resource patterns we are interested in.
    for (; itr != end; ++itr)
    {
        OutboundTransactionType txnType = itr->first;
        const OutboundMetricsConfigEntry& entry = *(itr->second);

        OutboundResourceList::const_iterator listItr = entry.getResources().begin();
        OutboundResourceList::const_iterator listEnd = entry.getResources().end();

        for (; listItr != listEnd; ++listItr)
        {
            const char8_t* resourceStr = (*listItr)->getPattern();
            TimeValue threshold = (*listItr)->getThreshold();
            trackResource(txnType, resourceStr, threshold, *newOutboundMetricsMap);
        }        
    }

    // Add metrics tracking for all the outbound services by default. 
    // gOutboundHttpService->configure() is called before gOutboundMetricsManager->configure() so we should now have the
    // most up to date registered http services to hook up for metrics tracking
    const OutboundHttpService::OutboundHttpServiceMap& httpServiceMap = gOutboundHttpService->getOutboundHttpServiceByNameMap();
    OutboundHttpService::OutboundHttpServiceMap::const_iterator httpItr = httpServiceMap.begin();
    OutboundHttpService::OutboundHttpServiceMap::const_iterator httpEnd = httpServiceMap.end();
    for (; httpItr != httpEnd; ++httpItr)
    {        
        const HttpConnectionManagerPtr& connMgr = httpItr->second;
        TimeValue httpServiceThreshold = connMgr.get()->getOutboundMetricsThreshold();

        // add tracking and persist existing metrics if appropriate
        eastl::string resource;
        const InetAddress& addr = connMgr.get()->getAddress();
#if defined(EA_PLATFORM_LINUX)
        // Linux supports regex and we use that for matching. So format our resource to match the regex.
        resource.append_sprintf("^%s%s.*", connMgr->isSecure() ? "https://" : "http://", addr.getHostname());
#else
        // Windows has no support for regex. See stubbed out regcomp et al at the top of InputFilter.cpp. So use exact string matching. Format the key such that
        // it closely matches above regex. 
        resource.append_sprintf("%s%s:%d", connMgr->isSecure() ? "https://" : "http://", addr.getHostname(), addr.getPort(InetAddress::Order::HOST));
#endif
        trackResource(HTTP, resource.c_str(), httpServiceThreshold, *newOutboundMetricsMap);
    }

    // clear out exiting metrics
    delete mOutboundMetricsMap;
    mOutboundMetricsMap = newOutboundMetricsMap;

    if (mConfig->empty())
    {
        BLAZE_TRACE_LOG(Log::SYSTEM, "[OutboundMetricsManager].configure(): OutboundMetricsManager config is emtpy" );
    }

    return true;
}

void OutboundMetricsManager::reconfigure(const OutboundMetricsConfig& config)
{
    configure(config);
}

void OutboundMetricsManager::validateConfig(const OutboundMetricsConfig& config, ConfigureValidationErrors& validationErrors) const
{
    OutboundMetricsConfigMap::const_iterator itr = config.getOutboundMetrics().begin();
    OutboundMetricsConfigMap::const_iterator end = config.getOutboundMetrics().end();

    for (; itr != end; ++itr)
    {
        OutboundTransactionType txnType = itr->first;
        const OutboundMetricsConfigEntry& entry = *(itr->second);

        if (entry.getBucketSize() == 0)
        {
            eastl::string msg;
            msg.sprintf("[OutboundMetricsManager].validateConfig:  cannot have bucketSize == 0 for OutboundTransactionType[%s]", 
                OutboundTransactionTypeToString(txnType));
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return;
        }

        if (entry.getBucketCount() == 0)
        {
             eastl::string msg;
            msg.sprintf("[OutboundMetricsManager].validateConfig:  cannot have bucketCount == 0 for OutboundTransactionType[%s]", 
                OutboundTransactionTypeToString(txnType));
            EA::TDF::TdfString& str = validationErrors.getErrorMessages().push_back();
            str.set(msg.c_str());
            return;
       }
        
    }
}

void OutboundMetricsManager::trackResource(OutboundTransactionType txnType, const char* resourceStr, TimeValue threshold, OutboundMetricsByTxnTypeMap& map)
{
    OutboundMetricsConfigMap::const_iterator itr = mConfig->find(txnType);
    if (itr == mConfig->end())
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[OutboundMetricsManager].trackResource(): txnType[" << OutboundTransactionTypeToString(txnType) << "] not configured for outbound metrics");
        return;
    }

    OutboundMetricsByResourceMap& resourceMap = map[txnType];
    OutboundMetricsByResourceMap::const_iterator rItr = resourceMap.find(resourceStr);
    if (rItr != resourceMap.end())
    {
        BLAZE_INFO_LOG(Log::SYSTEM, "[OutboundMetricsManager].trackResource(): txnType[" << OutboundTransactionTypeToString(txnType) << "], resourceStr[" << resourceStr << "] already tracked in OutboundMetricsByTxnTypeMap");
        return;
    }

    const OutboundMetricsEntryWithFilters* existingEntry = nullptr;
    if (mOutboundMetricsMap != nullptr)
    {
        OutboundMetricsByTxnTypeMap::const_iterator existingResourceItr = mOutboundMetricsMap->find(txnType);
        if (existingResourceItr != mOutboundMetricsMap->end())
        {
            OutboundMetricsByResourceMap::const_iterator existingEntryItr = existingResourceItr->second.find(resourceStr);
            if (existingEntryItr != existingResourceItr->second.end())
            {
                existingEntry = &existingEntryItr->second;
            }
        }
    }

    OutboundMetricsEntryWithFilters& entry = resourceMap[resourceStr];
    entry.setBucketSize(itr->second->getBucketSize());
    entry.setThreshold(threshold);
    entry.setResourcePattern(resourceStr);
    entry.getFilter().initialize(resourceStr, true, REGEX_EXTENDED);

    if (existingEntry != nullptr &&
        existingEntry->getBucketSize() == itr->second->getBucketSize() &&
        existingEntry->getCounts().size() == itr->second->getBucketCount())
    {
        // copy over existing metrics
        for (uint32_t i = 0; i < existingEntry->getCounts().size(); i++)
            entry.getCounts().push_back(existingEntry->getCounts()[i]);
    }
    else
    {
        for (uint32_t i = 0; i < itr->second->getBucketCount(); i++)
            entry.getCounts().push_back(0);
    }
}

void OutboundMetricsManager::tickAndCheckThreshold(OutboundTransactionType txnType, const char* resourceStr, TimeValue responseTime)
{
    if (!isInitialized())
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[OutboundMetricsManager].tickAndCheckThreshold(): method called before OutboundMetricsManager is configured" );
        return;
    }

    if (mConfig->empty())
    {
        return;
    }

    OutboundMetricsConfigMap::const_iterator itr = mConfig->find(txnType);
    if (itr == mConfig->end())
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[OutboundMetricsManager].tickAndCheckThreshold(): " << OutboundTransactionTypeToString(txnType) << " txn type is not found in configurations, bail out early" );
        return;
    }

    const OutboundMetricsConfigEntry* txnTypeConfig = itr->second;

    if (responseTime < TimeValue(0))
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[OutboundMetricsManager].tickAndCheckThreshold(): Response time value is a negative number (" <<  responseTime.getMicroSeconds() << "). Clamping to 0 to avoid a crash here. Check ntp logs for the likely root cause." );
        responseTime = TimeValue(0);
    }

    OutboundMetricsByResourceMap& resourceMap = (*mOutboundMetricsMap)[txnType];
    OutboundMetricsByResourceMap::iterator  rItr = resourceMap.begin();
    OutboundMetricsByResourceMap::iterator  rEnd = resourceMap.end();
    for (; rItr != rEnd; ++rItr)
    {
        OutboundMetricsEntryWithFilters& entry = rItr->second;
        if (entry.getFilter().match(resourceStr))
        {
            if (responseTime >= entry.getThreshold())
            {
                BLAZE_WARN_LOG(Log::SYSTEM, "[OutboundMetricsManager].tickAndCheckThreshold(): " << OutboundTransactionTypeToString(txnType) << " txn took " <<  responseTime.getMicroSeconds() << "us for " << resourceStr );
            }        

            uint32_t bucketCount = txnTypeConfig->getBucketCount();
            TimeValue bucketSize = txnTypeConfig->getBucketSize();
            if (responseTime >= (bucketCount * bucketSize))
            {
                BLAZE_SPAM_LOG(Log::SYSTEM, "[OutboundMetricsManager].tickAndCheckThreshold(): "<< OutboundTransactionTypeToString(txnType) << " txn incrementing bucket(" << bucketCount -1 << ") for resource(" << entry.getResourcePattern() << ")");
                entry.getCounts()[bucketCount-1] += 1;
            }
            else
            {
                uint32_t idx = (uint32_t)(responseTime/bucketSize);
                BLAZE_SPAM_LOG(Log::SYSTEM, "[OutboundMetricsManager].tickAndCheckThreshold(): "<< OutboundTransactionTypeToString(txnType) << " txn incrementing bucket(" << idx << ") for resource(" << entry.getResourcePattern() << ")");
                entry.getCounts()[idx] += 1;
            }
            entry.setTotalTime(entry.getTotalTime() + responseTime);
        }
        else
        {
            BLAZE_SPAM_LOG(Log::SYSTEM, "[OutboundMetricsManager].tickAndCheckThreshold(): filter(" << entry.getFilter().getInput() << ") did not match (" << resourceStr << ")");
        }
    }
}

void OutboundMetricsManager::filloutOutboundMetrics(OutboundMetrics& response) const
{
    if (!isInitialized())
    {
        BLAZE_WARN_LOG(Log::SYSTEM, "[OutboundMetricsManager].filloutOutboundMetrics(): method called before OutboundMetricsManager is configured" );
        return;
    }

    OutboundMetricsByTxnTypeMap::const_iterator itr = mOutboundMetricsMap->begin();
    OutboundMetricsByTxnTypeMap::const_iterator end = mOutboundMetricsMap->end();
    response.getOutboundMetricsMap().reserve(mOutboundMetricsMap->size());
    for (; itr != end; ++itr)
    {
        const OutboundMetricsByResourceMap& resourceMap = itr->second;
        OutboundMetricsList* newList = response.getOutboundMetricsMap().allocate_element();
        OutboundMetricsByResourceMap::const_iterator rItr = resourceMap.begin();
        OutboundMetricsByResourceMap::const_iterator rEnd = resourceMap.end();
        newList->reserve(resourceMap.size());
        for (; rItr != rEnd; ++rItr)
        {
            OutboundMetricsEntry* newEntry = rItr->second.clone();
            newList->push_back(newEntry);
        }
        response.getOutboundMetricsMap()[itr->first] = newList;
    }
}


} // end namespace Blaze
