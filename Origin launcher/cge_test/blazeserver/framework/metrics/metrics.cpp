/*************************************************************************************************/ /*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/metrics/metrics.h"
#include "framework/connection/endpoint.h"
#include "framework/controller/metricsexporter.h"
#include "framework/util/inputfilter.h"
#include "framework/controller/controller.h"

namespace Blaze
{
namespace Metrics
{

#ifdef BLAZE_ENABLE_METRICS_DEBUG
EA_THREAD_LOCAL FILE* _schemaFp = nullptr;
#endif

#define STATSD_METRICS_PREFIX  "blaze."
#define STATSD_METRICS_PREFIX_LEN (sizeof(STATSD_METRICS_PREFIX)-1)

#define METRICS_IOV_ADD(info,value,len) { \
    EA_ASSERT_MSG((info).iovCount < (info).sNumIovecs, "Metric export exceeded number of reserved iovec slots."); \
    (info).iov[(info).iovCount].iov_base = (void*)(value); \
    (info).iov[(info).iovCount].iov_len = (len); \
    ++(info).iovCount; \
    (info).outputLength += len; \
}

#define METRICS_IOV_ADD_TAGS(info) { \
        METRICS_IOV_ADD(info, (info).tags->c_str(), (info).tags->length()); \
        if ((info).extraTags != nullptr) \
            METRICS_IOV_ADD(info, (info).extraTags->c_str(), (info).extraTags->length()); \
        if ((info).globalTags != nullptr) \
            METRICS_IOV_ADD(info, (info).globalTags->c_str(), (info).globalTags->length()); \
}

#define METRICS_IOV_MAKE_SPACE(info) { \
        uint32_t availableSpace = EAArrayCount((info).iov) - (info).iovCount; \
        if ((availableSpace < ExportInfo::sRequiredEntriesPerMetric) || ((info).outputLength >= (info).maxPacketSize)) \
            (info).inject(); \
}

// Define all the metrics tags that are available
// {----------------------------------------------------------------------
const char8_t* int64ToString(const int64_t& value, TagValue& buffer)
{
    buffer.sprintf("%" PRId64, value);
    return buffer.c_str();
}

const char8_t* uint32ToString(const uint32_t& value, TagValue& buffer)
{
    buffer.sprintf("%" PRIu32, value);
    return buffer.c_str();
}

const char8_t* int32ToString(const int32_t& value, TagValue& buffer)
{
    buffer.sprintf("%" PRId32, value);
    return buffer.c_str();
}

const char8_t* uint16ToString(const uint16_t& value, TagValue& buffer)
{
    buffer.sprintf("%" PRIu16, value);
    return buffer.c_str();
}

void addTaggedMetricToComponentStatusInfoMap(ComponentStatus::InfoMap& map, const char8_t* metricBaseName, const TagPairList& tagList, uint64_t value)
{
    char8_t buf[64];

    eastl::string metric = metricBaseName;
    for(auto& tag : tagList)
    {
        metric.append_sprintf("_%s", tag.second.c_str());
    }

    blaze_snzprintf(buf, sizeof(buf), "%" PRIu64, value);
    map[metric.c_str()] = buf;
}

namespace Tag
{
TagInfo<const char*>* environment = BLAZE_NEW TagInfo<const char*>("environment");
TagInfo<const char*>* service_name = BLAZE_NEW TagInfo<const char*>("service_name");
TagInfo<const char*>* instance_type = BLAZE_NEW TagInfo<const char*>("instance_type");
TagInfo<const char*>* instance_name = BLAZE_NEW TagInfo<const char*>("instance_name");
TagInfo<const char*>* component = BLAZE_NEW TagInfo<const char*>("component");
TagInfo<const char*>* command = BLAZE_NEW TagInfo<const char*>("command");
TagInfo<const char*>* endpoint = BLAZE_NEW TagInfo<const char*>("endpoint");
TagInfo<const char*>* inboundgrpc = BLAZE_NEW TagInfo<const char*>("inboundgrpc");
TagInfo<const char*>* outboundgrpcservice = BLAZE_NEW TagInfo<const char*>("outboundgrpcservice");
TagInfo<ClientType>* client_type = BLAZE_NEW TagInfo<ClientType>("client_type", [](const ClientType& value, TagValue&) { return Blaze::ClientTypeToString(value); });
TagInfo<BlazeRpcError>* rpc_error = BLAZE_NEW TagInfo<BlazeRpcError>("rpc_error", [](const BlazeRpcError& value, TagValue&) { return ErrorHelp::getErrorName(value); });
TagInfo<const char*>* blaze_sdk_version = BLAZE_NEW TagInfo<const char*>("blaze_sdk_version");
TagInfo<const char*>* product_name = BLAZE_NEW TagInfo<const char*>("product_name");
TagInfo<ClientPlatformType>* platform = BLAZE_NEW TagInfo<ClientPlatformType>("platform", [](const ClientPlatformType& value, TagValue&) { return Blaze::ClientPlatformTypeToString(value); });
TagInfo<PlatformList>* platform_list = BLAZE_NEW TagInfo<PlatformList>("platform_list", [](const PlatformList& value, Metrics::TagValue&) { return value.c_str(); });
TagInfo<const char*>* sandbox = BLAZE_NEW TagInfo<const char*>("sandbox");
TagInfo<UserSessionDisconnectType>* disconnect_type = BLAZE_NEW TagInfo<UserSessionDisconnectType>("disconnect_type", [](const UserSessionDisconnectType& value, TagValue&) { return UserSessionDisconnectTypeToString(value); });
TagInfo<ClientState::Status>* client_status = BLAZE_NEW TagInfo<ClientState::Status>("client_status", [](const ClientState::Status& value, TagValue&) { return ClientState::StatusToString(value); });
TagInfo<ClientState::Mode>* client_mode = BLAZE_NEW TagInfo<ClientState::Mode>("client_mode", [](const ClientState::Mode& value, TagValue&) { return ClientState::ModeToString(value); });
TagInfo<const char*>* pingsite = BLAZE_NEW TagInfo<const char*>("pingsite");
TagInfo<const char*>* fiber_context = BLAZE_NEW TagInfo<const char*>("fiber_context");
TagInfo<Fiber::StackSize>* fiber_stack_size = BLAZE_NEW TagInfo<Fiber::StackSize>("fiber_stack_size", [](const Fiber::StackSize& value, TagValue&) { return Fiber::StackSizeToString(value); });
TagInfo<const char*>* db_query_name = BLAZE_NEW TagInfo<const char*>("db_query_name");
TagInfo<const char*>* db_pool = BLAZE_NEW TagInfo<const char*>("db_pool");
TagInfo<Log::Category>* log_category = BLAZE_NEW TagInfo<Log::Category>("log_category", [](const Log::Category& value, TagValue&) { return Logger::getCategoryName(value); });
TagInfo<Logging::Level>* log_level = BLAZE_NEW TagInfo<Logging::Level>("log_level", [](const Logging::Level& value, TagValue&) { return Logging::LevelToString(value); });
TagInfo<const char*>* log_location = BLAZE_NEW TagInfo<const char*>("log_location");
TagInfo<const char*>* cluster_name = BLAZE_NEW TagInfo<const char*>("cluster_name");
TagInfo<const char*>* hostname = BLAZE_NEW TagInfo<const char*>("hostname");
TagInfo<const char*>* sliver_namespace = BLAZE_NEW TagInfo<const char*>("sliver_namespace");
TagInfo<const char*>* http_pool = BLAZE_NEW TagInfo<const char*>("http_pool");
TagInfo<uint32_t>* http_error = BLAZE_NEW TagInfo<uint32_t>("http_error", uint32ToString);
TagInfo<const char*>* memory_category = BLAZE_NEW TagInfo<const char*>("memory_category");
TagInfo<const char*>* memory_subcategory = BLAZE_NEW TagInfo<const char*>("memory_subcategory");
TagInfo<const char*>* node_pool = BLAZE_NEW TagInfo<const char*>("node_pool");
TagInfo<InetAddress>* ip_addr = BLAZE_NEW TagInfo<InetAddress>("ip_addr", [](const InetAddress& value, TagValue&) { return value.getIpAsString(); });
TagInfo<const char*>* external_service_name = BLAZE_NEW TagInfo<const char*>("external_service_name");
TagInfo<const char*>* command_uri = BLAZE_NEW TagInfo<const char*>("command_uri");
TagInfo<const char*>* response_code = BLAZE_NEW TagInfo<const char*>("response_code");
TagInfo<RedisError>* redis_error = BLAZE_NEW TagInfo<RedisError>("redis_error", [](const RedisError& value, TagValue&) { return RedisErrorHelper::getName(value); });
} // namespace Tag


// }----------------------------------------------------------------------


class Filter
{
public:
    bool initialize(const MetricFilterList& filterList)
    {
        bool success = true;

        mFilters.reserve(filterList.size());

        for(auto& entry : filterList)
        {
            mFilters.push_back();
            FilterEntry& f = mFilters.back();
            if (!f.first.initialize((*entry).getRegex(), false, REGEX_EXTENDED))
                success = false;

            for(auto& tag : (*entry).getTags())
                f.second.emplace_back(tag.first.c_str(), tag.second.c_str());
        }
        return success;
    }

    bool isMatch(const TagPairConstPtrList& tags, const char8_t* metricName) const
    {
        for(const auto& filter : mFilters)
        {
            // Check if the metric name matches
            if (filter.first.match(metricName))
            {
                const TagList& tagList = filter.second;
                if (tagList.empty())
                    return true;

                // Check if all the requested tags match
                size_t tagMatches = 0;
                for(const auto& tagPairEntry : tagList)
                {
                    for(const auto& tag : tags)
                    {
                        if ((blaze_strcmp(tag->first->getName(), tagPairEntry.first.c_str()) == 0)
                                && (blaze_strcmp(tag->second.c_str(), tagPairEntry.second.c_str()) == 0))
                        {
                            tagMatches++;
                            break;
                        }
                    }
                }
                if (tagMatches == tagList.size())
                    return true;
            }
        }
        return false;
    }
    bool hasFilters() const
    {
        return !mFilters.empty();
    }
private:
    using TagList = eastl::vector<eastl::pair<eastl::string, eastl::string> >;
    using FilterEntry = eastl::pair<InputFilter, TagList>;
    eastl::vector<FilterEntry> mFilters;
};

TagInfoBase::Registry::~Registry()
{
    EA::Thread::AutoMutex am(mMutex);
    for(auto& info : mTagInfoList)
        delete info;
}

// static
void TagInfoBase::registerInfo(TagInfoBase* info)
{
    static TagInfoBase::Registry sRegistry;

    EA::Thread::AutoMutex am(sRegistry.mMutex);
    sRegistry.mTagInfoList.push_back(info);
}

MetricBase::MetricBase(MetricsCollection& owner, const char* name, uint32_t flags, TagPairConstPtrList* extraTags)
    : mOwner(owner)
    , mName(name)
    , mIdleCount(0)
    , mFlags(flags)
{

    if (mFlags & FLAG_IN_COLLECTION)
    {
        mOwner.prependNamePrefixes(mName);
        mOwner.addMetric(*this);            
    }

    updateIsExported(extraTags);
}

void MetricBase::updateIsExported(TagPairConstPtrList* extraTags)
{
    TagPairConstPtrList tags;
    if (extraTags == nullptr)
        extraTags = &tags;
    if (mOwner.shouldExport(mName.c_str(), *extraTags))
        mFlags |= MetricBase::FLAG_IS_EXPORTED;
    else
        mFlags &= ~MetricBase::FLAG_IS_EXPORTED;
}

MetricBase::~MetricBase()
{
    if (mFlags & FLAG_IN_COLLECTION)
        mOwner.removeMetric(*this);
}

void MetricBase::addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType type)
{
#ifdef BLAZE_ENABLE_METRICS_DEBUG
    if (type != INVALID_METRIC_TYPE)
        fprintf(_schemaFp, "%s,%s", mName.c_str(), MetricTypeToString(type));
    else
        fprintf(_schemaFp, "%s", mName.c_str());

    for (auto& tag : tags)
        fprintf(_schemaFp, ",%s", tag);
    fprintf(_schemaFp, "\n");

#endif

    MetricInfo* metric;
    auto metricItr = response.getMetrics().find(getName().c_str());
    if (metricItr == response.getMetrics().end())
    {
        metric = response.getMetrics().allocate_element();
        response.getMetrics()[getName().c_str()] = metric;
        for(auto& tag : tags)
        {
            metric->getTags().push_back(tag);
        }
        if (type == INVALID_METRIC_TYPE)
        {
            BLAZE_WARN_LOG(Log::METRICS, "[MetricBase].addMetricsSchemaToResponse: type missing for adding " << getName().c_str());
        }
        else
        {
            metric->setType(type);
        }
    }
    else
    {
        // check that the tags & type are identical
        // while any mis-match is benign, your code should not be using duplicate tag names
        metric = metricItr->second;

        if (type == INVALID_METRIC_TYPE)
        {
            BLAZE_WARN_LOG(Log::METRICS, "[MetricBase].addMetricsSchemaToResponse: type missing for checking " << getName().c_str());
        }
        else if (type != metric->getType())
        {
            BLAZE_WARN_LOG(Log::METRICS, "[MetricBase].addMetricsSchemaToResponse: type mis-match for " << getName().c_str()
                << ": " << MetricTypeToString(type) << " vs " << MetricTypeToString(metric->getType()));
        }

        bool tagsMatch = (tags.size() == metric->getTags().size());
        if (tagsMatch)
        {
            // check contents
            TagNames::const_iterator end = tags.end();
            TagNames::const_iterator itr = tags.begin();
            MetricTagNameList::const_iterator itr2 = metric->getTags().begin();
            for(; itr != end; ++itr, ++itr2)
            {
                if (blaze_strcmp(*itr, *itr2) != 0)
                {
                    tagsMatch = false;
                    break;
                }
            }
        }
        if (!tagsMatch)
        {
            eastl::string tagNames;
            for(auto& tag : tags)
            {
                tagNames += tag;
                tagNames += " "; // don't care about any extraneous spacing for log line
            }
            eastl::string tagNames2;
            for(auto& tag2 : metric->getTags())
            {
                tagNames2 += tag2;
                tagNames2 += " "; // don't care about any extraneous spacing for log line
            }
            BLAZE_WARN_LOG(Log::METRICS, "[MetricBase].addMetricsSchemaToResponse: tag mis-match for " << getName().c_str()
                << ": (" << tagNames << ") vs (" << tagNames2 << ")");
        }
    }
}


void MetricBase::updateExportFlag(const Filter& exportIncludeFilter, const Filter& exportExcludeFilter, TagPairConstPtrList& tags)
{
    if (exportIncludeFilter.hasFilters() && !exportIncludeFilter.isMatch(tags, mName.c_str()))
        mFlags &= ~MetricBase::FLAG_IS_EXPORTED;
    else if (exportExcludeFilter.isMatch(tags, mName.c_str()))
        mFlags &= ~MetricBase::FLAG_IS_EXPORTED;
    else
        mFlags |= MetricBase::FLAG_IS_EXPORTED;
}

ExportInfo::ExportInfo(const OperationalInsightsExportConfig& config, bool lowFrequency, const InetAddress& agentAddress)
    : tags(nullptr)
    , extraTags(nullptr)
    , globalTags(nullptr)
    , iovCount(0)
    , outputLength(0)
    , isLowFrequencyExport(lowFrequency)
    , isGostatsd(config.getEnableGostatsdExtensions())
    , maxPacketSize(config.getMaxPacketSize())
      // Cull metrics that have been idle for 10 minutes
    , gcIdleCount((10 * 60) / (uint32_t)config.getExportInterval().getSec())
    , mInjector(agentAddress)
    , mBufferIdx(0)
#ifdef BLAZE_ENABLE_METRICS_DEBUG
    , mMetricsFp(nullptr)
#endif
{
    if (gcIdleCount < 1)
        gcIdleCount = 1;

    if (!config.getGlobalTags().empty())
    {
        globalTags = BLAZE_NEW eastl::string;
        for(auto& tag : config.getGlobalTags())
            globalTags->append_sprintf(",%s:%s", tag.first.c_str(), tag.second.c_str());
    }

    mInjector.connect();


#ifdef BLAZE_ENABLE_METRICS_DEBUG
    eastl::string fn;
    fn.sprintf("metrics-%s-%s.statsd", gController->getDefaultServiceName(), gController->getInstanceName());
    mMetricsFp = fopen(fn.c_str(), "a+");
    fprintf(mMetricsFp, "### %" PRId64 "\n", TimeValue::getTimeOfDay().getSec());
#endif
}

ExportInfo::~ExportInfo()
{
    // Make sure there isn't anything left to export
    inject();

    mInjector.disconnect();

#ifdef BLAZE_ENABLE_METRICS_DEBUG
    fclose(mMetricsFp);
#endif

    for(auto& buf : mBuffers)
        delete[] buf;

    delete globalTags;
}

void ExportInfo::inject()
{
    mInjector.inject(iov, iovCount);

#ifdef BLAZE_ENABLE_METRICS_DEBUG
    for(size_t idx = 0; idx < iovCount; idx++)
        fwrite(iov[idx].iov_base, iov[idx].iov_len, 1, mMetricsFp);
#endif

    iovCount = 0;
    outputLength = 0;
    mBufferIdx = 0;
}

char8_t* ExportInfo::getBuffer()
{
    char8_t* buf;
    if (mBufferIdx < mBuffers.size())
    {
        buf = mBuffers[mBufferIdx];
        mBufferIdx++;
    }
    else
    {
        buf = BLAZE_NEW_ARRAY(char8_t, sTempBufferSize);
        mBuffers.push_back(buf);
        mBufferIdx++;
    }
    return buf;
}

MetricsCollection::MetricsCollection()
    : mOwner(nullptr)
    , mNextExportTime(0)
    , mTagString(nullptr)
    , mExportIncludeFilter(nullptr)
    , mExportExcludeFilter(nullptr)
    , mConfigChanged(false)
    , mEnableNamePrefixing(false)
    , mAgentAddressResolutionPending(true)
    , mAgentAddressResolutionTimer(INVALID_TIMER_ID)
{
}

MetricsCollection::MetricsCollection(TagInfoBase* keyTag, const char8_t* keyValue, MetricsCollection& owner, bool enableNamePrefixing)
    : mOwner(&owner)
    , mKey(keyTag, keyValue)
    , mNextExportTime(0)
    , mTagString(nullptr)
    , mExportIncludeFilter(nullptr)
    , mExportExcludeFilter(nullptr)
    , mConfigChanged(false)
    , mEnableNamePrefixing(enableNamePrefixing)
    , mAgentAddressResolutionPending(true)
    , mAgentAddressResolutionTimer(INVALID_TIMER_ID)
{
}

bool MetricsCollection::configure(const OperationalInsightsExportConfig& config)
{
    if (mOwner != nullptr)
        return mOwner->configure(config);

    Filter* includeFilter = BLAZE_NEW Filter();
    if (!includeFilter->initialize(config.getInclude()))
    {
        delete includeFilter;
        return false;
    }

    delete mExportIncludeFilter;
    mExportIncludeFilter = includeFilter;

    Filter* excludeFilter = BLAZE_NEW Filter();
    if (!excludeFilter->initialize(config.getExclude()))
    {
        delete includeFilter;
        return false;
    }

    delete mExportExcludeFilter;
    mExportExcludeFilter = excludeFilter;


    if (blaze_strcmp(mAgentAddress.getHostname(), config.getAgentHostname()) != 0
        || mAgentAddress.getPort(InetAddress::Order::HOST) != config.getAgentPort())
    {
        mAgentAddress = InetAddress(config.getAgentHostname(), config.getAgentPort());
        mAgentAddressResolutionPending = true;

        // During a reconfigure we will likely have a periodic resolve job going,
        // cancel it for now because we want to immediately resolve the new agent address
        // which in turn will re-kick off the periodic job
        if (mAgentAddressResolutionTimer != INVALID_TIMER_ID)
        {
            gSelector->cancelTimer(mAgentAddressResolutionTimer);
            mAgentAddressResolutionTimer = INVALID_TIMER_ID;
        }
    }

    mConfigChanged = true;
    return true;
}

MetricsCollection::~MetricsCollection()
{
    for(auto& collection : mCollections)
        delete collection.second;
    delete mTagString;
    delete mExportIncludeFilter;
    delete mExportExcludeFilter;
}

bool MetricsCollection::addMetric(MetricBase& metric)
{
    if (mMetrics.find(metric.getName()) != mMetrics.end())
        return false;
    mMetrics.insert({ metric.getName(), metric });
    if (mTagString == nullptr)
    {
        mTagString = BLAZE_NEW eastl::string;
        for(MetricsCollection* c = this; c != nullptr; c = c->getOwner())
        {
            TagPair& key = c->mKey;
            if (key.first != nullptr)
            {
                char8_t normBuf[1024];
                mTagString->append_sprintf("%s%s:%s", mTagString->empty() ? "|#" : ",", key.first->getName(), sanitizeTagValue(key.second.c_str(), normBuf, sizeof(normBuf)));
            }
        }
    }
    return true;
}

void MetricsCollection::removeMetric(MetricBase& metric)
{
    mMetrics.erase(metric.getName());
    if (mMetrics.empty())
    {
        // No more metrics in this collection so destroy the cached tag string.
        delete mTagString;
        mTagString = nullptr;
    }
}

void MetricsCollection::exportMetrics()
{
    // Walk back up the tree and start the export from the root collection
    if (mOwner != nullptr)
    {
        mOwner->exportMetrics();
        return;
    }

    // If the config has been reloaded, then reflag all the metrics to in case the export
    // filters have been changed
    if (mConfigChanged)
    {
        mConfigChanged = false;
        TagPairConstPtrList tags;
        updateExportFlags(*mExportIncludeFilter, *mExportExcludeFilter, tags);
    }

    const OperationalInsightsExportConfig& config = gController->getFrameworkConfigTdf().getMetricsLoggingConfig().getOperationalInsightsExport();

    // See whether it's time to export yet
    TimeValue oiExportInterval = config.getExportInterval();
    if (oiExportInterval <= 0)
        return;

    TimeValue now = TimeValue::getTimeOfDay();
    bool isLowFrequencyExport = (mNextExportTime < now);

    if (mAgentAddressResolutionPending)
    {
        mAgentAddressResolutionPending = false;
        resolveAgent();
    }

    ExportInfo info(config, isLowFrequencyExport, mAgentAddress);
    exportStatsdInternal(info);

    // Setup when we should do the next export.  Align the interval such that they fire across all
    // blaze instances at the same time to ensure a consistent view of the system.
    mNextExportTime = ((now + oiExportInterval) / oiExportInterval) * oiExportInterval;
}

void MetricsCollection::resolveAgent()
{
    NameResolver::LookupJobId resolveId;
    BlazeRpcError sleepErr = gNameResolver->resolve(mAgentAddress, resolveId);
    if (sleepErr != ERR_OK)
    {
        BLAZE_ERR_LOG(Log::METRICS, "MetricsCollection.resolveAgent: failed to resolve '" << mAgentAddress.getHostname()
            << "': rc=" << ErrorHelp::getErrorName(sleepErr) << ".  Metrics export won't work.");
    }

    mAgentAddressResolutionTimer = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + METRICS_AGENT_RESOLUTION_PERIOD,
        this, &MetricsCollection::resolveAgent, "MetricsCollection::resolveAgent");
}

void MetricsCollection::updateExportFlags(const Filter& exportIncludeFilter, const Filter& exportExcludeFilter, TagPairConstPtrList& tags)
{
    if (mKey.first != nullptr)
        tags.emplace_back(&mKey);

    for(auto& m : mMetrics)
        m.second.updateExportFlag(exportIncludeFilter, exportExcludeFilter, tags);

    for(auto& collection : mCollections)
        collection.second->updateExportFlags(exportIncludeFilter, exportExcludeFilter, tags);

    if (mKey.first != nullptr)
        tags.pop_back();
}

bool MetricsCollection::shouldExport(const char8_t* name, TagPairConstPtrList& tags)
{
    bool rc = true;

    if (mKey.first != nullptr)
        tags.push_back(&mKey);
    if (mOwner != nullptr)
    {
        rc = mOwner->shouldExport(name, tags);
    }
    else
    {
        // In order to get exported, we have to be in the whitelist (if it exists):
        if (mExportIncludeFilter != nullptr && mExportIncludeFilter->hasFilters())
            rc = mExportIncludeFilter->isMatch(tags, name);
        
        // and not be in the black list:
        if (rc == true && mExportExcludeFilter != nullptr)
            rc = !mExportExcludeFilter->isMatch(tags, name);
    }

    if (mKey.first != nullptr)
        tags.pop_back();
    return rc;
}

void MetricsCollection::exportStatsdInternal(ExportInfo& info)
{
    const eastl::string* prevTags = info.tags;
    info.tags = mTagString;

    if (info.isLowFrequencyExport)
    {
        for(auto& m : mMetrics)
            m.second.exportStatsdInternal(info);
    }
    else
    {
        for(auto& m : mMetrics)
        {
            if ((m.second.mFlags & MetricBase::FLAG_HIGH_FREQUENCY_EXPORT) == MetricBase::FLAG_HIGH_FREQUENCY_EXPORT)
            {
                m.second.exportStatsdInternal(info);
            }
        }
    }

    for(auto& collection : mCollections)
        collection.second->exportStatsdInternal(info);

    info.tags = prevTags;
}

MetricsCollection& MetricsCollection::getTaglessCollection(const char8_t* value)
{
    return getCollectionInternal(nullptr, value, false);
}

MetricsCollection& MetricsCollection::getCollectionInternal(TagInfoBase* tag, const char8_t* value, bool enableNamePrefixing)
{
    TagPairRef key(tag, value);

    auto itr = mCollections.find_as(key, TagPairRefHash(), TagPairRefEqual());
    if (itr != mCollections.end())
        return *itr->second;

    MetricsCollection* collection = BLAZE_NEW MetricsCollection(tag, value, *this, enableNamePrefixing);
    mCollections.insert(eastl::make_pair(collection->mKey, collection));
    return *collection;
}

bool MetricsCollection::getMetrics(const GetMetricsRequest& request, GetMetricsResponse& response)
{
    if (mOwner != nullptr)
    {
        // Make sure we start at the root collection by walking back up the tree
        return mOwner->getMetrics(request, response);
    }

    Filter filter;
    if (!filter.initialize(request.getFilters()))
        return false;

    TagPairConstPtrList tags;
    getMetricsInternal(filter, tags, response);

    return true;
}

void MetricsCollection::getMetricsInternal(const Filter& exportFilter, TagPairConstPtrList& tags, GetMetricsResponse& response)
{
    if (mKey.first != nullptr)
        tags.emplace_back(&mKey);

    for(auto& m : mMetrics)
        m.second.addMetricsToResponse(exportFilter, response, tags);

    for(auto& collection : mCollections)
        collection.second->getMetricsInternal(exportFilter, tags, response);

    if (mKey.first != nullptr)
        tags.pop_back();
}

void MetricsCollection::getMetricsSchema(GetMetricsSchemaResponse& response)
{
    if (mOwner != nullptr)
    {
        // Make sure we start at the root collection by walking back up the tree
        mOwner->getMetricsSchema(response);
        return;
    }

#ifdef BLAZE_ENABLE_METRICS_DEBUG
    eastl::string fn;
    fn.sprintf("metrics-%s-%s.schema.csv", gController->getDefaultServiceName(), gController->getInstanceName());
    _schemaFp = fopen(fn.c_str(), "w");
#endif

    TagNames tags;
    getMetricsSchemaInternal(response, tags);

#ifdef BLAZE_ENABLE_METRICS_DEBUG
    fclose(_schemaFp);
#endif
}

void MetricsCollection::getMetricsSchemaInternal(GetMetricsSchemaResponse& response, TagNames& tags)
{
    if (mKey.first != nullptr)
        tags.emplace_back(mKey.first->getName());

    for(auto& m : mMetrics)
    {
        m.second.addMetricsSchemaToResponse(response, tags, INVALID_METRIC_TYPE);
    }

    for(auto& collection : mCollections)
    {
        collection.second->getMetricsSchemaInternal(response, tags);
    }

    if (mKey.first != nullptr)
        tags.pop_back();
}

void MetricsCollection::prependNamePrefixes(eastl::string& metricName)
{
    if (mEnableNamePrefixing)
    {
        if (!mKey.second.empty())
        {
            const char8_t* name = mKey.second.c_str();
            size_t len = mKey.second.length();
            metricName.insert(metricName.begin(), '.');
            metricName.insert(metricName.begin(), name, name + len);
        }
    }
    if (mOwner != nullptr)
        mOwner->prependNamePrefixes(metricName);
}

const char8_t* MetricsCollection::sanitizeTagValue(const char8_t* value, char8_t* outbuf, size_t outlen)
{
    if ((value == nullptr) || (*value == '\0'))
        return "<unset>";

    char8_t* out = outbuf;
    char8_t* outend = outbuf + outlen;
    char8_t ch = *value;
    while ((ch != '\0') && (out < outend))
    {
        switch (ch)
        {
            case ':': // Used as a separator between tag and value
            case '|': // Used as a special separator (For example, |c indicates counter, |# indicates dimension.
            case '#': // Covered by above comment.
            case ',': // Used to separate multiple dimensions
            case '@': // Used to represent frequency of a counter
                ch = '_';
                break;
            default: // Sanitize against rouge values that fall out of printable ascii characters.
                if (ch <= ' ' || ((unsigned char) (ch) >= 127))
                    ch = '_';
                break;
        }
        *out++ = ch;
        value++;
        ch = *value;
    }
    *out = '\0';
    return outbuf;
}

void Counter::exportStatsdInternal(ExportInfo& info)
{
    if (!isExported())
        return;

    uint64_t val = mCount - mLastExportedCount;
    if ((val > 0) || (mPrevExportedDelta != 0))
    {
        METRICS_IOV_MAKE_SPACE(info);

        METRICS_IOV_ADD(info, STATSD_METRICS_PREFIX, STATSD_METRICS_PREFIX_LEN);
        METRICS_IOV_ADD(info, mName.c_str(), mName.length());

        char8_t* buf = info.getBuffer();
        METRICS_IOV_ADD(info, buf, blaze_snzprintf(buf, ExportInfo::sTempBufferSize, ":%" PRIu64 "|c", val));

        METRICS_IOV_ADD_TAGS(info);
        METRICS_IOV_ADD(info, "\n", 1);

        mPrevExportedDelta = val;
        // Reset the counter in preparation for the next interval
        mLastExportedCount = mCount;
        mIdleCount = 0;
    }
    else
    {
        mIdleCount++;
    }
}

void Counter::addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags)
{
    if (!exportFilter.isMatch(tags, mName.c_str()))
        return;

    Metric* metric = response.getMetrics().pull_back();
    metric->setName(getName().c_str());
    metric->setType(METRIC_COUNTER);
    metric->getValue().setValue(mCount);
    for(auto& tag : tags)
        metric->getTags()[tag->first->getName()] = tag->second.c_str();
}

void Counter::addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType)
{
    MetricBase::addMetricsSchemaToResponse(response, tags, mMetricType);
}

void Gauge::exportStatsdInternal(ExportInfo& info)
{
    if (!isExported())
        return;

    // Cull exports where the value was 0 and hasn't changed since the last time
    if ((mValue != 0) || (mPrevValue != 0))
    {
        METRICS_IOV_MAKE_SPACE(info);

        METRICS_IOV_ADD(info, STATSD_METRICS_PREFIX, STATSD_METRICS_PREFIX_LEN);
        METRICS_IOV_ADD(info, mName.c_str(), mName.length());

        char8_t* buf = info.getBuffer();
        METRICS_IOV_ADD(info, buf, blaze_snzprintf(buf, ExportInfo::sTempBufferSize, ":%" PRIu64 "|g", mValue));

        METRICS_IOV_ADD_TAGS(info);
        METRICS_IOV_ADD(info, "\n", 1);

        mPrevValue = mValue;
        mIdleCount = 0;
    }
    else
    {
        mIdleCount++;
    }
}

void Gauge::addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags)
{
    if (!exportFilter.isMatch(tags, mName.c_str()))
        return;

    Metric* metric = response.getMetrics().pull_back();
    metric->setName(getName().c_str());
    metric->setType(METRIC_GAUGE);
    metric->getValue().setValue(mValue);
    for(auto& tag : tags)
        metric->getTags()[tag->first->getName()] = tag->second.c_str();
}

void Gauge::addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType)
{
    MetricBase::addMetricsSchemaToResponse(response, tags, mMetricType);
}

void PolledGauge::exportStatsdInternal(ExportInfo& info)
{
    if (!isExported())
        return;

    uint64_t value = get();

    // Cull exports where the value was 0 and hasn't changed since the last time
    if ((value != 0) || (mPrevValue != 0))
    {
        METRICS_IOV_MAKE_SPACE(info);

        METRICS_IOV_ADD(info, STATSD_METRICS_PREFIX, STATSD_METRICS_PREFIX_LEN);
        METRICS_IOV_ADD(info, mName.c_str(), mName.length());

        char8_t* buf = info.getBuffer();
        METRICS_IOV_ADD(info, buf, blaze_snzprintf(buf, ExportInfo::sTempBufferSize, ":%" PRIu64 "|g", value));

        METRICS_IOV_ADD_TAGS(info);
        METRICS_IOV_ADD(info, "\n", 1);

        mPrevValue = value;
        mIdleCount = 0;
    }
    else
    {
        mIdleCount++;
    }
}

void PolledGauge::addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags)
{
    if (!exportFilter.isMatch(tags, mName.c_str()))
        return;

    Metric* metric = response.getMetrics().pull_back();
    metric->setName(getName().c_str());
    metric->setType(METRIC_GAUGE);
    metric->getValue().setValue(get());
    for(auto& tag : tags)
        metric->getTags()[tag->first->getName()] = tag->second.c_str();
}

void PolledGauge::addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType)
{
    MetricBase::addMetricsSchemaToResponse(response, tags, mMetricType);
}

void Timer::exportStatsdInternal(ExportInfo& info)
{
    static const size_t sMaxTimerValueLength = sizeof(",18446744073709551615"); // UINT64_MAX

    if (!isExported())
        return;

    if (mTimes.size() > 0)
    {
        int32_t len = 0;
        size_t count = 0;
        char8_t* timerVals = nullptr;
        int32_t timerValIdx = 0;
        for(auto entry : mTimes)
        {
            // Export the timer if there isn't space in the buffer to add any more timer values
            // or the gostatsd agent isn't being used and therefore we need to export for each
            // timer value.
            bool shouldExportTimer =
                ((len + sMaxTimerValueLength) >= ExportInfo::sTempBufferSize)
                || (!info.isGostatsd && (count == 1));

            if ((len == 0) || shouldExportTimer)
            {
                if (shouldExportTimer)
                {
                    timerVals[0] = ':';
                    info.iov[timerValIdx].iov_len = len;

                    len = 0;
                    count = 0;
                }

                METRICS_IOV_MAKE_SPACE(info);

                METRICS_IOV_ADD(info, STATSD_METRICS_PREFIX, STATSD_METRICS_PREFIX_LEN);
                METRICS_IOV_ADD(info, mName.c_str(), mName.length());

                timerValIdx = info.iovCount;
                timerVals = info.getBuffer();
                METRICS_IOV_ADD(info, timerVals, 0);

                METRICS_IOV_ADD(info, "|ms", 3);
                METRICS_IOV_ADD_TAGS(info);
                METRICS_IOV_ADD(info, "\n", 1);
            }

            len += blaze_snzprintf(timerVals + len, ExportInfo::sTempBufferSize - len, ",%" PRIu64, entry);

            count++;
        }
        if (count > 0)
        {
            timerVals[0] = ':';
            info.iov[timerValIdx].iov_len = len;
        }

        // Reset the timer in preparation for the next interval
        mTimes.clear();
        mIdleCount = 0;
    }
    else
    {
        mIdleCount++;
    }
}

void Timer::addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags)
{
    if (!exportFilter.isMatch(tags, mName.c_str()))
        return;

    Metric* metric = response.getMetrics().pull_back();
    metric->setName(getName().c_str());
    metric->setType(METRIC_TIMER);
    metric->getValue().switchActiveMember(MetricValue::MEMBER_TIMER);
    MetricTimerValue* timer = metric->getValue().getTimer();
    timer->setSum(mTotalTime);
    timer->setCount(mTotalCount);
    timer->setMin(mTotalMin);
    timer->setMax(mTotalMax);
    timer->setAvg(mTotalCount == 0 ? 0 : mTotalTime / mTotalCount);
    for(auto& tag : tags)
        metric->getTags()[tag->first->getName()] = tag->second.c_str();
}

void Timer::addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType)
{
    MetricBase::addMetricsSchemaToResponse(response, tags, mMetricType);
}

} // namespace Metrics
} // namespace Blaze

