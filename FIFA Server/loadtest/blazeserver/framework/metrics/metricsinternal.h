/*************************************************************************************************/ /*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_METRICSINTERNAL_H
#define BLAZE_METRICSINTERNAL_H

#include "EATDF/time.h"

// This class tracks all state related to outputting metrics to the Cloud OI gostatsd agent and
// provides helper functions for the task.
//
// The metrics export mechanism is frequent and high volume and so some of the export implementation
// breaks from good coding practices to gain efficiency given that Blaze is typically deployed with
// debug builds and so we cannot depend on typically optimizations such as function inlining.
class ExportInfo
{
public:
    static const size_t sTempBufferSize = 256;
    static const uint32_t sRequiredEntriesPerMetric = 10;
    static const size_t sNumIovecs = sRequiredEntriesPerMetric * 10;

    ExportInfo(const OperationalInsightsExportConfig& config, bool lowFrequency, const InetAddress& agentAddress);
    ~ExportInfo();

    char8_t* getBuffer();

    void inject();

public:
    const eastl::string* tags;
    const eastl::string* extraTags;
    eastl::string* globalTags;
    iovec_blaze iov[sNumIovecs];
    size_t iovCount;
    size_t outputLength;
    bool isLowFrequencyExport;
    bool isGostatsd;
    uint32_t maxPacketSize;
    uint32_t gcIdleCount;

private:
    UdpMetricsInjector mInjector;
    eastl::vector<char8_t*> mBuffers;
    uint32_t mBufferIdx;
#ifdef BLAZE_ENABLE_METRICS_DEBUG
    FILE* mMetricsFp;
#endif
};

inline void TagPairList::buildTagString() const
{
    mTagString.clear();
    for(auto& tag : *this)
    {
        char8_t normBuf[1024];
        mTagString.append_sprintf(",%s:%s", tag.first->getName(), MetricsCollection::sanitizeTagValue(tag.second.c_str(), normBuf, sizeof(normBuf)));
    }
}

inline size_t TagPairHash::operator()(TagPair const& tag) const
{
    // Lifted from eastl::hash<string> since there is no fixed_string version
    const unsigned char* p = (const unsigned char*)tag.second.c_str();
    unsigned int c, result = 2166136261U; // We implement an FNV-like string hash. 
    while((c = *p++) != 0) // Using '!=' disables compiler warnings.
        result = (result * 16777619) ^ c;
    return (size_t)(result * 16777619) ^ (size_t)tag.first;
}

inline size_t TagValuesHash::operator()(TagValues const& values) const
{
    // Lifted from eastl::hash<string> since there is no fixed_string version
    uint32_t result = 2166136261U;
    for(auto& v : values)
    {
        unsigned int c;
        const unsigned char* p = (const unsigned char*)v;
        while((c = *p++) != 0) // Using '!=' disables compiler warnings.
            result = (result * 16777619) ^ c;
    }
    return (size_t)result;
}

inline bool TagValuesEqual::operator()(TagValues const& a, TagValues const& b) const
{
    if (a.size() != b.size())
        return false;
    TagValues::const_iterator aItr = a.begin();
    TagValues::const_iterator aEnd = a.end();
    TagValues::const_iterator bItr = b.begin();
    for(; aItr != aEnd; ++aItr, ++bItr)
    {
        if (strcmp(*aItr, *bItr) != 0)
            return false;
    }
    return true;
}

inline size_t TagPairRefHash::operator()(TagPairRef const& tag) const
{
    // Lifted from eastl::hash<string> since there is no fixed_string version
    const unsigned char* p = (const unsigned char*)tag.second;
    unsigned int c, result = 2166136261U; // We implement an FNV-like string hash. 
    while((c = *p++) != 0) // Using '!=' disables compiler warnings.
        result = (result * 16777619) ^ c;
    return (size_t)(result * 16777619) ^ (size_t)tag.first;
}

inline bool TagPairRefEqual::operator()(const TagPair& a, const TagPairRef& b)
{
    return (a.first == b.first) && (strcmp(a.second.c_str(), b.second) == 0);
}

inline size_t TagPairListHash::operator()(TagPairList const& tagPairs) const
{
    // Lifted from eastl::hash<string> since there is no fixed_string version
    uint32_t result = 2166136261U;
    for(auto& tag : tagPairs)
        result = (result * 16777619) ^ TagPairHash()(tag);
    return (size_t)result;
}

inline size_t TagPairRefListHash::operator()(TagPairRefList const& tagPairs) const
{
    // Lifted from eastl::hash<string> since there is no fixed_string version
    uint32_t result = 2166136261U;
    for(auto& tag : tagPairs)
        result = (result * 16777619) ^ TagPairRefHash()(tag);
    return (size_t)result;
}

inline bool TagPairRefListEqual::operator()(const TagPairList& a, const TagPairRefList& b)
{
    if (a.size() != b.size())
        return false;
    TagPairList::const_iterator aItr = a.begin();
    TagPairList::const_iterator aEnd = a.end();
    TagPairRefList::const_iterator bItr = b.begin();
    for(; aItr != aEnd; ++aItr, ++bItr)
    {
        if ((aItr->first != bItr->first) || (strcmp(aItr->second.c_str(), bItr->second) != 0))
            return false;
    }
    return true;
}


template<typename T_MetricType>
TaggedMetricBase<T_MetricType>::TaggedMetricBase(MetricsCollection& owner, const char* name, uint32_t flags, const TagInfoList& tagInfos)
    : MetricBase(owner, name, flags & (~MetricBase::FLAG_IN_COLLECTION), nullptr),
    mTagInfos(tagInfos),
    mAllowTagGroups(true)
{
    // Option 1 - We specifically do not include these Tag Group metrics in the Metrics collection.  Only the Base metric is used. 
 
#ifdef BLAZE_OI_METRICS_INCLUDE_TAG_GROUPS
    // Option 2 - In this case, we have duplicate some MetricBase code, after building a string from the tag names
    for(auto curTag : mTagInfos)  // Build the new name string.  (ex. gamemanager.activeplayers.game_pingsite.game_mode)
    { 
        mName.append("."); 
        mName.append(curTag->getName());
    }  

    mFlags |= FLAG_IN_COLLECTION;
    mOwner.addMetric(*this);
    updateIsExported(nullptr);
#endif
}

template<typename T_TagInfo>
void tagInfosToList(TagInfoList& tagInfos, T_TagInfo firstTagInfos)
{
    tagInfos.push_back(firstTagInfos);
}
template<typename T_TagInfo, typename ... T_TagInfoRest>
void tagInfosToList(TagInfoList& tagInfos, T_TagInfo firstTagInfos, T_TagInfoRest... tagInfosRest)
{
    tagInfos.push_back(firstTagInfos);
    tagInfosToList(tagInfos, tagInfosRest...);
}

template<typename T_MetricType>
template<typename T_TagInfo, typename ... T_TagInfoRest>
TaggedMetricBase<T_MetricType>::TaggedMetricBase(MetricsCollection& owner, const char* name, uint32_t flags, T_TagInfo firstTagInfos, T_TagInfoRest... tagInfos)
        : MetricBase(owner, name, flags | MetricBase::FLAG_IN_COLLECTION, nullptr), 
          mAllowTagGroups(true)
{
    // Fill out the tag infos:
    tagInfosToList(mTagInfos, firstTagInfos, tagInfos...);
}

template<typename T_MetricType>
TaggedMetricBase<T_MetricType>::~TaggedMetricBase()
{
    reset();

    for (auto& tagGroup : mTagGroups)
        delete tagGroup;

    mTagGroups.clear();
}

template<typename T_MetricType>
void TaggedMetricBase<T_MetricType>::defineTagGroups(const TagInfoLists& validTagLists)
{
    EA_ASSERT_MSG(mAllowTagGroups, "Tag groups are not allowed on this type of tagged metric.");
    EA_ASSERT_MSG(mTagGroups.empty(), "Tag groups should be empty before setting tag groups.");

    // create new tag groups using these tag lists:
    for (auto& tagList : validTagLists)
    {
        mTagGroups.push_back(BLAZE_NEW TaggedMetricBase<T_MetricType>(mOwner, mName.c_str(), mFlags, tagList));
    }
}
template<typename T_MetricType>
void TaggedMetricBase<T_MetricType>::disableTags(const TagInfoList& tags)
{
    EA_ASSERT_MSG(mAllowTagGroups, "Tag groups are not allowed on this type of tagged metric.");
    EA_ASSERT_MSG(mTagGroups.empty(), "Tag groups should be empty before setting tag groups.");

    TagInfoList tempTagList;
    for (auto& curTag : mTagInfos)
    {
        bool keepEnabled = true;
        for (auto& disableTag : tags)
        {
            if (disableTag == curTag)
                keepEnabled = false;
        }
        
        if (keepEnabled)
            tempTagList.push_back(curTag);
    }
    
    mTagGroups.push_back(BLAZE_NEW TaggedMetricBase<T_MetricType>(mOwner, mName.c_str(), mFlags, tempTagList));
}


template<typename T_MetricType>
void TaggedMetricBase<T_MetricType>::reset()
{
    for (auto& tagGroup : mTagGroups)
        tagGroup->reset();

    for(auto& metric : mChildren)
        delete metric.second;
    mChildren.clear();
}

template<typename T_MetricType>
bool TaggedMetricBase<T_MetricType>::checkAllTagsMatch(const TagPairList& tags) const
{
    for (auto& tag : tags)
    {
        bool foundKey = false;
        for (auto& tagInfo : mTagInfos)
        {
            if (tagInfo == tag.first)
            {
                foundKey = true;
                break;
            }
        }

        if (!foundKey)
            return false;
    }

    return true;
}

template<typename T_MetricType>
bool TaggedMetricBase<T_MetricType>::checkAllTagsMatch(const TagInfoList& tags) const
{
    for (auto& tag : tags)
    {
        bool foundKey = false;
        for (auto& tagInfo : mTagInfos)
        {
            if (tagInfo == tag)
            {
                foundKey = true;
                break;
            }
        }

        if (!foundKey)
            return false;
    }

    return true;
}

template<typename T_MetricType>
bool TaggedMetricBase<T_MetricType>::iterate(eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, void(const TagPairList& tags, const T_MetricType&)> const& func) const
{
    EA_ASSERT_MSG(mTagGroups.empty(), "Iterating over metrics with tag groups is not supported (unless you set a TagPairList to use).");

    // If no tags are provided, assume we want to iterate over anything/everything:
    TagPairList tags;
    return iterate(tags, func);
}


template<typename T_MetricType>
bool TaggedMetricBase<T_MetricType>::iterate(const TagPairList& tags, eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, void (const TagPairList& tags, const T_MetricType&)> const& func) const
{
    // Since we're using a slightly different functor, we make a wrapper here:
    return get(tags, false, [&func](const TagPairList& tagList, const T_MetricType& value, bool exactTagMatch) { func(tagList, value); });
}

template<typename T_MetricType>
bool TaggedMetricBase<T_MetricType>::get(const TagPairList& tags, bool exactTagMatch, eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, void(const TagPairList& tags, const T_MetricType&, bool exactTagMatch)> const& func) const
{
    for (auto& tagGroup : mTagGroups)
    {
        // iterating over all the groups will not have the same effect, since the groups will each have the same total. 
        // So we check for the first group that matches all of the tags and use that. (Ideally we'd order them by children count.)
        if (tagGroup->get(tags, exactTagMatch, func))
            return true;
    }

    // We have to check that all of the requested tags are part of this group:
    if (!checkAllTagsMatch(tags))
        return false;

    // Then we do the children iteration:
    // First, we check for an exact match, before iterating over everything
    bool foundExactMatch = false;
    if (exactTagMatch)
    {
        auto metric = mChildren.find(tags);
        if (metric != mChildren.end())
        {
            foundExactMatch = true;
            func(metric->first, *metric->second, foundExactMatch);
        }
    }
    if (!foundExactMatch)
    {
        for (auto& metric : mChildren)
            func(metric.first, *metric.second, foundExactMatch);
    }

    return true;
}


template<typename T_MetricType>
bool TaggedMetricBase<T_MetricType>::aggregateBase(const TagInfoList& keys, SumsByTagValue& sums, eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, uint64_t(const T_MetricType&)> const& func) const
{
    for (auto& tagGroup : mTagGroups)
    {
        // iterating over all the groups will not have the same effect, since the groups will each have the same total. 
        // So we check for the first group that matches all of the tags and use that. (Ideally we'd order them by children count.)
        if (tagGroup->aggregateBase(keys, sums, func))
            return true;
    }

    // Before we iterate over all the children, check that this is a valid tag check request:
    // Ex.  metric<A,B,C> can aggregate({A,C}), not aggregate({D,E,F}) or aggregate({A,X})
    if (!checkAllTagsMatch(keys))
        return false;

    for(auto& metric : mChildren)
    {
        uint64_t value = func(*metric.second);
        if (value == 0)
            continue;

        // build the tags for the aggregate key
        TagValues tags;
        for(auto& key : keys)
        {
            for(auto& tag : metric.first)
            {
                if (tag.first == key)
                {
                    tags.push_back(tag.second.c_str());
                    break;
                }
            }
        }

        sums[tags] += value;
    }
    return true;
}

template<typename T_MetricType>
void TaggedMetricBase<T_MetricType>::addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags)
{
    // If the metric isn't in the collection, we shouldn't add it to the metrics (applies to TagGroup recursion)
    if ((mFlags & FLAG_IN_COLLECTION) == 0)
        return;

    if (!mTagGroups.empty())
    {
        for (auto& tagGroup : mTagGroups)
            tagGroup->addMetricsToResponse(exportFilter, response, tags);

        // Option 1: Always return the base Schema
#ifdef BLAZE_METRICS_SKIP_FULL_TAG_GROUP
        return;
#endif
    }

    for(auto& metric : mChildren)
    {
        size_t tagSize = tags.size();

        for(auto& tag : metric.first)
            tags.emplace_back(&tag);

        metric.second->addMetricsToResponse(exportFilter, response, tags);

        tags.erase(tags.begin() + tagSize, tags.end());
    }
}

template<typename T_MetricType>
void TaggedMetricBase<T_MetricType>::addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType type)
{
    // If the metric isn't in the collection, we shouldn't add it to the metrics (applies to TagGroup recursion)
    if ((mFlags & FLAG_IN_COLLECTION) == 0)
        return;

    if (!mTagGroups.empty())
    {
        for (auto& tagGroup : mTagGroups)
            tagGroup->addMetricsSchemaToResponse(response, tags, type);

        // Option 1: Always return the base Schema
#ifdef BLAZE_METRICS_SKIP_FULL_TAG_GROUP
        return;
#endif
    }

    for (auto& tagInfo : mTagInfos)
        tags.emplace_back(tagInfo->getName());

    MetricBase::addMetricsSchemaToResponse(response, tags, T_MetricType::mMetricType);

    size_t numTags = mTagInfos.size();
    for (size_t i = 0; i < numTags; i++)
    {
        tags.pop_back();
    }
}

template<typename T_MetricType>
void TaggedMetricBase<T_MetricType>::getMetrics(eastl::list<T_MetricType*>& outValues, const TagPairRefList& tagList)
{
    // If we have groups, just send it to them rather than processing directly.
    if (!mTagGroups.empty())
    {
        for (auto& tagGroup : mTagGroups)
            tagGroup->getMetrics(outValues, tagList);

        // Option 1: Always return the base Schema
#ifdef BLAZE_METRICS_SKIP_FULL_TAG_GROUP
        return;
#endif
    }

    // Verify that the requested tag list applies to the metrics that we track, 
    // if it has more than our metrics, just make a new sublist that only has the tags we track: 
    // (Assumes the order matches:)
    TagPairRefList tempList;
    for (auto& tag : tagList)
    {
        for (auto& tagInfo : mTagInfos)
        {
            if (tagInfo == tag.first)
            {
                tempList.push_back(tag);
                break;
            }
        }
    }

    // If no tags match, then we can't help this.
    if (tempList.empty())
        return;

    // If we got to this point, all the requested metrics were found and have a value, so do the lookup 
    T_MetricType* metric;
    auto itr = mChildren.find_as(tempList, TagPairRefListHash(), TagPairRefListEqual());
    if (itr != mChildren.end())
    {
        metric = itr->second;
    }
    else
    {
        TagPairConstPtrList extraTags;

        // Entry doesn't exist so we need to create a non-ref version of the tag list
        TagPairList t;
        for (auto& entry : tempList)
        {
            t.emplace_back(entry.first, entry.second);
            const TagPair& p = t.back();
            extraTags.push_back(&p);
        }
        t.buildTagString();

        metric = BLAZE_NEW T_MetricType(0, this->mOwner, mName.c_str(), &extraTags);

        // Entry doesn't exist so we need to create a non-ref version of the tag list
        mChildren.insert(eastl::make_pair(t, metric));
    }

    outValues.push_back(metric);
    return;
}

template<typename T_MetricType>
template<typename TagType>
void TaggedMetricBase<T_MetricType>::getMetrics(eastl::list<T_MetricType*>& outValues, TagPairRefList& tagList, const TagType& tag)
{
    TagValue v;
    auto tagIndex = (mTagInfos.size() - 1);
    auto info = (TagInfo<TagType>*)mTagInfos[tagIndex];
    tagList.push_back({ info, info->getValue(tag, v) });
    return getMetrics(outValues, tagList);
}

template<typename T_MetricType>
template<typename TagType, typename ... TagTypeRest>
void TaggedMetricBase<T_MetricType>::getMetrics(eastl::list<T_MetricType*>& outValues, TagPairRefList& tagList, const TagType& firstTag, const TagTypeRest& ... restTags)
{
    TagValue v;
    auto tagIndex = (mTagInfos.size() - sizeof...(restTags) - 1);
    auto info = (TagInfo<TagType>*)mTagInfos[tagIndex];
    tagList.push_back({ info, info->getValue(firstTag, v) });
    return getMetrics(outValues, tagList, restTags...);
}

template<typename T_MetricType>
template<typename TagType, typename ... TagTypeRest>
void TaggedMetricBase<T_MetricType>::getMetrics(eastl::list<T_MetricType*>& outValues, const TagType& firstTag, const TagTypeRest& ... restTags)
{
    TagPairRefList tagList;
    getMetrics(outValues, tagList, firstTag, restTags...);
}

template<typename T_MetricType>
void TaggedMetricBase<T_MetricType>::exportStatsdInternal(ExportInfo& info)
{
    // If the metric isn't in the collection, we shouldn't add it to the metrics (applies to TagGroup recursion)
    if ((mFlags & FLAG_IN_COLLECTION) == 0)
        return;

    if (!mTagGroups.empty())
    {
        for (auto& tagGroup : mTagGroups)
            tagGroup->exportStatsdInternal(info);

        // Option 1: Export add metrics to the base Metric.  
#ifdef BLAZE_METRICS_SKIP_FULL_TAG_GROUP
        return;
#endif
    }

    for(auto& metric : mChildren)
    {
        info.extraTags = &metric.first.getTagString();
        metric.second->exportStatsdInternal(info);
        info.extraTags = nullptr;
    }

    if (info.isLowFrequencyExport)
    {
        if ((mFlags & MetricBase::FLAG_SUPPRESS_GARBAGE_COLLECTION) == 0)
        {
            uint32_t gcIdleCount = info.gcIdleCount;

            // Cull idle metrics
            auto itr = mChildren.begin();
            while (itr != mChildren.end())
            {
                if (itr->second->getIdleCount() >= gcIdleCount)
                {
                    delete itr->second;
                    itr = mChildren.erase(itr);
                }
                else
                    ++itr;
            }
        }
    }
}

template<typename MetricType>
void TaggedMetricBase<MetricType>::updateExportFlag(const Filter& exportIncludeFilter, const Filter& exportExcludeFilter, TagPairConstPtrList& tags)
{
    // If the metric isn't in the collection, we shouldn't add it to the metrics (applies to TagGroup recursion)
    if ((mFlags & FLAG_IN_COLLECTION) == 0)
        return;

    if (!mTagGroups.empty())
    {
        for (auto& tagGroup : mTagGroups)
            tagGroup->updateExportFlag(exportIncludeFilter, exportExcludeFilter, tags);

        // Option 1: Update export flag for the base Metric.  
#ifdef BLAZE_METRICS_SKIP_FULL_TAG_GROUP
        return;
#endif
    }

    mFlags |= MetricBase::FLAG_IS_EXPORTED;

    for(auto& metric : mChildren)
    {
        size_t tagSize = tags.size();

        for(auto& tag : metric.first)
            tags.emplace_back(&tag);

        metric.second->updateExportFlag(exportIncludeFilter, exportExcludeFilter, tags);

        tags.erase(tags.begin() + tagSize, tags.end());
    }
}

template<typename T_Tag, typename ... T_RestTags>
TaggedCounter<T_Tag, T_RestTags...>::TaggedCounter(MetricsCollection& owner, const char* name, TagInfo<T_Tag>* firstTag, TagInfo<T_RestTags>*... restTags)
    : TaggedMetricBase<Counter>(owner, name, 0, firstTag, restTags...),
    mTotal(0)
{
}

template<typename T_Tag, typename ... T_RestTags>
inline void TaggedCounter<T_Tag, T_RestTags...>::increment(uint64_t value, const T_Tag& firstTag, const T_RestTags& ... restTags)
{
    if (value == 0)
        return;

    mTotal += value;

    eastl::list<Counter*> outValues;
    getMetrics(outValues, firstTag, restTags...);
    for(auto& metric : outValues)
        metric->increment(value);
}

template<typename T_Tag, typename ... T_RestTags>
uint64_t TaggedCounter<T_Tag, T_RestTags...>::getTotal() const
{
    // Because getStatus() and some of the legacy metric polling RPCs require aggregate totals, we
    // cannot garbage collect idle metrics.  Once fully transitioned to the new metrics system,
    // this flag can be removed.
    this->mFlags |= MetricBase::FLAG_SUPPRESS_GARBAGE_COLLECTION;

    return mTotal;
}

template<typename T_Tag, typename ... T_RestTags>
void TaggedCounter<T_Tag, T_RestTags...>::reset()
{
    mTotal = 0;
    TaggedMetricBase<Counter>::reset();
}

template<typename T_Tag, typename ... T_RestTags>
uint64_t TaggedCounter<T_Tag, T_RestTags...>::getTotal(const TagPairList& query) const
{
    uint64_t sum = 0;
    this->get(query, true, [&sum, &query](const TagPairList& tags, const Counter& value, bool exactMatchFound) {
            bool match = true;
            if (!exactMatchFound)
            {
                for (auto& q : query)
                {
                    bool found = false;
                    for (auto& tag : tags)
                    {
                        if ((tag.first == q.first) && (tag.second == q.second))
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        match = false;
                        break;
                    }
                }
            }
            if (match)
                sum += value.getTotal();
        });

    // Because getStatus() and some of the legacy metric polling RPCs require aggregate totals, we
    // cannot garbage collect idle metrics.  Once fully transitioned to the new metrics system,
    // this flag can be removed.
    this->mFlags |= MetricBase::FLAG_SUPPRESS_GARBAGE_COLLECTION;

    return sum;
}

template<typename T_Tag, typename ... T_RestTags>
void TaggedCounter<T_Tag, T_RestTags...>::aggregate(const TagInfoList& keys, SumsByTagValue& sums) const
{
    this->aggregateBase(keys, sums, [](const Counter& value) { return value.getTotal(); });
}

template<typename T_Tag, typename ... T_RestTags>
TaggedGauge<T_Tag, T_RestTags...>::TaggedGauge(MetricsCollection& owner, const char* name, TagInfo<T_Tag>* firstTag, TagInfo<T_RestTags>*... restTags)
    : TaggedMetricBase<Gauge>(owner, name, 0, firstTag, restTags...),
    mTotal(0)
{
}

template<typename T_Tag, typename ... T_RestTags>
inline void TaggedGauge<T_Tag, T_RestTags...>::set(uint64_t value, const T_Tag& firstTag, const T_RestTags& ... restTags)
{
    bool setTotal = false;

    eastl::list<Gauge*> outValues;
    getMetrics(outValues, firstTag, restTags...);
    for (auto& metric : outValues)
    {
        // Still have to do the lookup, since updating the total requires us to know about the current value:
        if (!setTotal)
        {
            if (metric->get() > value)
                mTotal -= (metric->get() - value);
            else
                mTotal += (value - metric->get());
            setTotal = true;
        }

        metric->set(value);
    }
}

template<typename T_Tag, typename ... T_RestTags>
inline void TaggedGauge<T_Tag, T_RestTags...>::increment(uint64_t value, const T_Tag& firstTag, const T_RestTags& ... restTags)
{
    mTotal += value;

    eastl::list<Gauge*> outValues;
    getMetrics(outValues, firstTag, restTags...);
    for (auto& metric : outValues)
        metric->increment(value);
}

template<typename T_Tag, typename ... T_RestTags>
inline void TaggedGauge<T_Tag, T_RestTags...>::decrement(uint64_t value, const T_Tag& firstTag, const T_RestTags& ... restTags)
{
    mTotal -= value;

    eastl::list<Gauge*> outValues;
    getMetrics(outValues, firstTag, restTags...);
    for (auto& metric : outValues)
        metric->decrement(value);
}

template<typename T_Tag, typename ... T_RestTags>
uint64_t TaggedGauge<T_Tag, T_RestTags...>::get() const
{
    return mTotal;
}

template<typename T_Tag, typename ... T_RestTags>
void TaggedGauge<T_Tag, T_RestTags...>::reset()
{
    mTotal = 0;
    TaggedMetricBase<Gauge>::reset();
}

template<typename T_Tag, typename ... T_RestTags>
uint64_t TaggedGauge<T_Tag, T_RestTags...>::get(const TagPairList& query) const
{
    uint64_t sum = 0;
    TaggedMetricBase<Gauge>::get(query, true, [&sum, &query](const TagPairList& tags, const Gauge& value, bool exactMatchFound) {
        bool match = true;
        if (!exactMatchFound)
        {
            for (auto& q : query)
            {
                bool found = false;
                for (auto& tag : tags)
                {
                    if ((tag.first == q.first) && (tag.second == q.second))
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    match = false;
                    break;
                }
            }
        }
        if (match)
            sum += value.get();
    });
    return sum;
}

template<typename T_Tag, typename ... T_RestTags>
void TaggedGauge<T_Tag, T_RestTags...>::aggregate(const TagInfoList& keys, SumsByTagValue& sums) const
{
    this->aggregateBase(keys, sums, [](const Gauge& value) { return value.get(); });
}

template<typename T_Tag, typename ... T_RestTags>
TaggedPolledGauge<T_Tag, T_RestTags...>::TaggedPolledGauge(MetricsCollection& owner, const char* name, TagInfo<T_Tag>* firstTag, TagInfo<T_RestTags>*... restTags)
    : TaggedMetricBase<PolledGauge>(owner, name, 0, firstTag, restTags...)
{
    // Due to the complexity of efficiently tracking polling functions for aggregate metrics.
    mAllowTagGroups = false;
}

template<typename T_Tag, typename ... T_RestTags>
inline void TaggedPolledGauge<T_Tag, T_RestTags...>::startOrReconfigure(PollerFunction poller, const T_Tag& firstTag, const T_RestTags& ... restTags)
{
    eastl::list<PolledGauge*> outValues;
    getMetrics(outValues, firstTag, restTags...);
    for (auto& metric : outValues)
        metric->setPollerFunction(poller);
}

template<typename T_Tag, typename ... T_RestTags>
uint64_t TaggedPolledGauge<T_Tag, T_RestTags...>::get() const
{
    TagPairList tags;
    return this->get(tags);
}

template<typename T_Tag, typename ... T_RestTags>
uint64_t TaggedPolledGauge<T_Tag, T_RestTags...>::get(const TagPairList& query) const
{
    uint64_t sum = 0;
    TaggedMetricBase<PolledGauge>::get(query, true, [&sum, &query](const TagPairList& tags, const PolledGauge& value, bool exactMatchFound) {
        bool match = true;
        if (!exactMatchFound)
        {
            for (auto& q : query)
            {
                bool found = false;
                for (auto& tag : tags)
                {
                    if ((tag.first == q.first) && (tag.second == q.second))
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    match = false;
                    break;
                }
            }
        }
        if (match)
            sum += value.get();
    });
    return sum;
}

template<typename T_Tag, typename ... T_RestTags>
void TaggedPolledGauge<T_Tag, T_RestTags...>::aggregate(const TagInfoList& keys, SumsByTagValue& sums) const
{
    this->aggregateBase(keys, sums, [](const PolledGauge& value) { return value.get(); });
}




template<typename T_Tag, typename ... T_RestTags>
TaggedTimer<T_Tag, T_RestTags...>::TaggedTimer(MetricsCollection& owner, const char* name, TagInfo<T_Tag>* firstTag, TagInfo<T_RestTags>*... restTags)
    : TaggedMetricBase<Timer>(owner, name, MetricBase::FLAG_HIGH_FREQUENCY_EXPORT, firstTag, restTags...)
{
}

template<typename T_Tag, typename ... T_RestTags>
inline void TaggedTimer<T_Tag, T_RestTags...>::record(const EA::TDF::TimeValue& time, const T_Tag& firstTag, const T_RestTags& ... restTags)
{
    eastl::list<Timer*> outValues;
    getMetrics(outValues, firstTag, restTags...);
    for (auto& metric : outValues)
        metric->record(time);
}


#endif // BLAZE_METRICSINTERNAL_H

